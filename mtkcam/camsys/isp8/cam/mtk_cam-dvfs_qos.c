// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include <linux/gcd.h>

#include <mtk-interconnect.h>
#include <soc/mediatek/mmdvfs_v3.h>
#include <soc/mediatek/mmqos.h>
#include <media/v4l2-subdev.h>

#include "mtk_cam.h"
#include "mtk_cam-defs.h"
#include "mtk_cam-raw_pipeline.h"
#include "mtk_cam-job.h"
#include "mtk_cam-job_utils.h"
#include "mtk_cam-fmt_utils.h"
#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-ufbc-def.h"
#include "mtk_cam-qof.h"
#include "mtk_cam-plat.h"
#include "mtk_cam-bwr.h"
#if KERNEL_VERSION(6, 6, 0) == LINUX_VERSION_CODE
#include "subsys/swpm_isp_wrapper.h"
#endif

#define BOOST_DVFS_OPP    2
#define ICCPATH_NAME_SIZE 32
#define LCM(a,b) ((a) / gcd(a, b) * (b))

struct dvfs_stream_info {
	int opp_idx;
	bool boostable;

	int switching_opp_idx;
	bool switching_boostable;
};
static unsigned int cam_sspm_en;
module_param(cam_sspm_en, int, 0644);
MODULE_PARM_DESC(cam_sspm_en, "sspm report enable");

static int mtk_cam_build_freq_table(struct device *dev,
				    struct camsys_opp_table *tbl,
				    int size)
{
	int count = dev_pm_opp_get_opp_count(dev);
	struct dev_pm_opp *opp;
	int i, index = 0;
	unsigned long freq = 0;

	if (WARN(count > size,
		 "opp table is being truncated\n"))
		count = size;

	for (i = 0; i < count; i++) {
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp))
			break;

		tbl[index].freq_hz = freq;
		tbl[index].volt_uv = dev_pm_opp_get_voltage(opp);

		dev_pm_opp_put(opp);
		index++;
		freq++;
	}

	return index;
}

int mtk_cam_dvfs_probe(struct device *dev,
		       struct mtk_camsys_dvfs *dvfs, int max_stream_num)
{
	int ret;
	int i;

	dvfs->dev = dev;
	mutex_init(&dvfs->dvfs_lock);

	if (max_stream_num <= 0) {
		dev_info(dev, "invalid stream num %d\n", max_stream_num);
		return -1;
	}

	dvfs->max_stream_num = max_stream_num;
	dvfs->stream_infos = devm_kcalloc(dev, max_stream_num,
					  sizeof(*dvfs->stream_infos),
					  GFP_KERNEL);
	if (!dvfs->stream_infos)
		return -ENOMEM;

	ret = dev_pm_opp_of_add_table(dev);
	if (ret < 0) {
		dev_info(dev, "fail to init opp table: %d\n", ret);
		goto opp_default_table;
	}

	dvfs->opp_num = mtk_cam_build_freq_table(dev, dvfs->opp,
						 ARRAY_SIZE(dvfs->opp));

	for (i = 0; i < dvfs->opp_num; i++)
		dev_info(dev, "[%s] idx=%d, clk=%d volt=%d\n", __func__,
			 i, dvfs->opp[i].freq_hz, dvfs->opp[i].volt_uv);
	dvfs->mmdvfs_clk = devm_clk_get(dev, !mmdvfs_get_version() ?
					"mmdvfs_clk" : "mmdvfs_mux");
	if (IS_ERR(dvfs->mmdvfs_clk)) {
		dvfs->mmdvfs_clk = NULL;
		dev_info(dev, "failed to get mmdvfs_clk\n");
	}

	return 0;

opp_default_table:
	dvfs->opp_num = 1;
	dvfs->opp[0] = (struct camsys_opp_table) {
		.freq_hz = GET_PLAT_HW(default_opp_freq_hz),
		.volt_uv = GET_PLAT_HW(default_opp_volt_uv),
	};
	return 0;
}

int mtk_cam_dvfs_remove(struct mtk_camsys_dvfs *dvfs)
{
	dev_pm_opp_of_remove_table(dvfs->dev);
	return 0;
}

void mtk_cam_dvfs_reset_runtime_info(struct mtk_camsys_dvfs *dvfs)
{
	dvfs->cur_opp_idx = -1;

	memset(dvfs->stream_infos, 0,
	       dvfs->max_stream_num * sizeof(*dvfs->stream_infos));
}

unsigned int mtk_cam_dvfs_query(struct mtk_camsys_dvfs *dvfs, int opp_idx)
{
	unsigned int idx;

	idx = clamp_t(unsigned int, opp_idx, 0, dvfs->opp_num);
	return dvfs->opp[idx].freq_hz;
}

static int freq_to_oppidx(struct mtk_camsys_dvfs *dvfs,
			  unsigned int freq)
{
	int i;

	for (i = 0; i < dvfs->opp_num; i++)
		if (freq <= dvfs->opp[i].freq_hz)
			break;

	if (i == dvfs->opp_num) {
		dev_info(dvfs->dev, "failed to find idx for freq %u\n", freq);
		return -1;
	}
	return i;
}

static int mtk_cam_dvfs_update_opp(struct mtk_camsys_dvfs *dvfs,
					  int opp_idx)
{
	int ret;

	if (!dvfs->mmdvfs_clk) {
		dev_info(dvfs->dev, "%s: mmdvfs_clk is not ready\n", __func__);
		return -1;
	}

	if (opp_idx < 0 || opp_idx >= ARRAY_SIZE(dvfs->opp)) {
		dev_info(dvfs->dev, "%s: invalid opp_idx %d\n", __func__,
			 opp_idx);
		return -1;
	}

	ret = clk_set_rate(dvfs->mmdvfs_clk, dvfs->opp[opp_idx].freq_hz);
	if (ret < 0) {
		dev_info(dvfs->dev, "[%s] clk set rate %u fail",
			 __func__, dvfs->opp[opp_idx].freq_hz);
		return -1;
	}

	return 0;
}

static int mtk_cam_dvfs_adjust_opp(struct mtk_camsys_dvfs *dvfs,
				   int opp_idx)
{
	if (opp_idx == dvfs->cur_opp_idx)
		return 0;

	if (mtk_cam_dvfs_update_opp(dvfs, opp_idx))
		return -1;

	dvfs->cur_opp_idx = opp_idx;
	return 0;
}

static struct dvfs_stream_info *get_stream_info(struct mtk_camsys_dvfs *dvfs,
						int stream_id)
{
	if (WARN_ON(stream_id < 0 || stream_id >= dvfs->max_stream_num))
		return NULL;

	return &dvfs->stream_infos[stream_id];
}

static void stream_info_set(struct dvfs_stream_info *s_info,
			    int opp_idx, bool boostable,
			    bool is_switching)
{
	if (!is_switching) {
		s_info->opp_idx = opp_idx;
		s_info->boostable = boostable;

		s_info->switching_opp_idx = 0;
		s_info->switching_boostable = 0;
	} else {
		s_info->switching_opp_idx = opp_idx;
		s_info->switching_boostable = boostable;
	}
}

static void stream_info_do_switch(struct dvfs_stream_info *s_info)
{
	s_info->opp_idx = s_info->switching_opp_idx;
	s_info->boostable = s_info->switching_boostable;

	s_info->switching_opp_idx = 0;
	s_info->switching_boostable = 0;
}

static int find_max_oppidx(struct mtk_camsys_dvfs *dvfs,
			   struct dvfs_stream_info *s_info,
			   bool is_switching)
{
	int max_opp = 0;
	int boost_opp;
	int i;

	for (i = 0; i < dvfs->max_stream_num; i++)
		max_opp = max3(max_opp,
			dvfs->stream_infos[i].opp_idx,
			dvfs->stream_infos[i].switching_opp_idx);

	if (!is_switching)
		return max_opp;

	boost_opp = s_info->boostable ?
		min(s_info->opp_idx + BOOST_DVFS_OPP, (int)dvfs->opp_num) :
		s_info->opp_idx;

	max_opp = max3(max_opp, boost_opp, s_info->switching_opp_idx);
	return max_opp;
}

static int dvfs_update(struct mtk_camsys_dvfs *dvfs, int stream_id,
		       unsigned int freq_hz, bool boostable,
		       bool is_switching, const char *caller)
{
	struct dvfs_stream_info *s_info;
	int opp_idx = -1;
	int max_opp_idx = -1;
	int prev_opp_idx;
	int ret = 0;

	s_info = get_stream_info(dvfs, stream_id);
	if (!s_info)
		return -1;

	opp_idx = freq_to_oppidx(dvfs, freq_hz);
	if (opp_idx < 0)
		return -1;

	mutex_lock(&dvfs->dvfs_lock);

	stream_info_set(s_info, opp_idx, boostable, is_switching);
	max_opp_idx = find_max_oppidx(dvfs, s_info, is_switching);

	prev_opp_idx = dvfs->cur_opp_idx;
	ret = mtk_cam_dvfs_adjust_opp(dvfs, max_opp_idx);
	if (ret)
		goto EXIT_UNLOCK;

	mutex_unlock(&dvfs->dvfs_lock);

	dev_info(dvfs->dev,
		 "%s: stream %d freq %u boostable %d, opp_idx: %d->%d\n",
		 caller, stream_id, freq_hz, boostable,
		 prev_opp_idx, max_opp_idx);
	return 0;

EXIT_UNLOCK:
	mutex_unlock(&dvfs->dvfs_lock);
	return ret;
}

int mtk_cam_dvfs_update(struct mtk_camsys_dvfs *dvfs, int stream_id,
			unsigned int freq_hz, bool boostable)
{
	return dvfs_update(dvfs, stream_id, freq_hz, boostable, false,
			   __func__);
}

int mtk_cam_dvfs_switch_begin(struct mtk_camsys_dvfs *dvfs, int stream_id,
			      unsigned int freq_hz, bool boostable)
{
	return dvfs_update(dvfs, stream_id, freq_hz, boostable, true,
			   __func__);
}

int mtk_cam_dvfs_switch_end(struct mtk_camsys_dvfs *dvfs, int stream_id)
{
	struct dvfs_stream_info *s_info;
	int prev_opp_idx;
	int max_opp_idx = -1;
	int ret = 0;

	s_info = get_stream_info(dvfs, stream_id);
	if (!s_info)
		return -1;

	mutex_lock(&dvfs->dvfs_lock);

	stream_info_do_switch(s_info);
	max_opp_idx = find_max_oppidx(dvfs, s_info, false);

	prev_opp_idx = dvfs->cur_opp_idx;
	ret = mtk_cam_dvfs_adjust_opp(dvfs, max_opp_idx);
	if (ret)
		goto EXIT_UNLOCK;

	mutex_unlock(&dvfs->dvfs_lock);

	dev_info(dvfs->dev,
		 "%s: stream %d opp_idx: %d->%d\n",
		 __func__, stream_id, prev_opp_idx, max_opp_idx);
	return 0;

EXIT_UNLOCK:
	mutex_unlock(&dvfs->dvfs_lock);
	return ret;
}

struct mtk_camsys_qos_path {
	char name[ICCPATH_NAME_SIZE];
	struct icc_path *path;
	s64 applied_bw;
	s64 pending_bw;
};

int mtk_cam_qos_probe(struct device *dev,
		      struct mtk_camsys_qos *qos, int qos_num)
{
	const char *names[32];
	struct mtk_camsys_qos_path *cam_path;
	int i;
	int n;
	int ret = 0;

	n = of_property_count_strings(dev->of_node, "interconnect-names");
	if (n <= 0) {
		dev_info(dev, "skip without interconnect-names\n");
		return 0;
	}

	qos->n_path = n;
	qos->cam_path = devm_kcalloc(dev, qos->n_path,
				     sizeof(*qos->cam_path), GFP_KERNEL);
	if (!qos->cam_path)
		return -ENOMEM;

	dev_info(dev, "icc_path num %d\n", qos->n_path);
	if (qos->n_path > ARRAY_SIZE(names)) {
		dev_info(dev, "%s: array size of names is not enough.\n",
			 __func__);
		return -EINVAL;
	}

	if (qos->n_path != qos_num) {
		dev_info(dev, "icc num(%d) mismatch with qos num(%d)\n",
			 qos->n_path, qos_num);
		return -EINVAL;
	}

	of_property_read_string_array(dev->of_node, "interconnect-names",
				      names, qos->n_path);

	for (i = 0, cam_path = qos->cam_path; i < qos->n_path; i++, cam_path++) {
		dev_info(dev, "interconnect: idx %d [%s]\n", i, names[i]);
		cam_path->path = of_mtk_icc_get(dev, names[i]);
		if (IS_ERR_OR_NULL(cam_path->path)) {
			dev_info(dev, "failed to get icc of %s\n",
				 names[i]);
			ret = -EINVAL;
		}
		strscpy(cam_path->name, names[i], ICCPATH_NAME_SIZE);

		cam_path->applied_bw = -1;
		cam_path->pending_bw = -1;
	}

	return ret;
}

int mtk_cam_qos_remove(struct mtk_camsys_qos *qos)
{
	int i;

	if (!qos->cam_path)
		return 0;

	for (i = 0; i < qos->n_path; i++)
		icc_put(qos->cam_path[i].path);

	return 0;
}

static inline u32 apply_ufo_com_ratio(u32 avg_bw)
{
	return avg_bw / 10 * 7;
}

/* unit : KB/s */
static inline u32 calc_bw(u32 size, u64 linet, u32 active_h)
{
	//pr_info("%s: size:%d linet:%llu, active_h:%d\n",
	//		__func__, size, linet, active_h);

	return to_qos_icc((1000000000L*size)/(linet * active_h));
}

static struct mtkcam_qos_desc *find_qos_desc_by_uid(
				struct mtkcam_qos_desc *mmqos_table, int tbl_size, int uid)
{
	int i = 0;

	for (i = 0; i < tbl_size; i++) {
		if (uid == mmqos_table[i].id)
			return &mmqos_table[i];
	}

	return NULL;
}

static int get_ufbc_size(int ipi_fmt, int ufbc_type, int img_w, int img_h)
{
	int size = 0, aligned_w = 0, stride = 0;

	if (ipifmt_is_yuv_ufo(ipi_fmt) || ipifmt_is_raw_ufo(ipi_fmt)) {
		aligned_w = ALIGN(img_w, 64);
		stride = aligned_w * mtk_cam_get_pixel_bits(ipi_fmt) / 8;

		switch (ufbc_type) {
		case UFBC_BITSTREAM_0:
			size = apply_ufo_com_ratio(stride * img_h);
		break;
		case UFBC_BITSTREAM_1:
			size = apply_ufo_com_ratio(stride * img_h / 2);
		break;
		case UFBC_TABLE_0:
			size = ALIGN((aligned_w / 64),
				ipifmt_is_raw_ufo(ipi_fmt) ? 16 : 8) * img_h;
		break;
		case UFBC_TABLE_1:
			size = ALIGN((aligned_w / 64),
				ipifmt_is_raw_ufo(ipi_fmt) ? 16 : 8) * img_h / 2;
		break;
		default:
		break;
		}
	}

	return size;
}

//assuming max image size 16000*12000*10/8
#define MMQOS_SIZE_WARNING 240000000
static int fill_raw_out_qos(struct mtk_cam_job *job,
					struct mtkcam_ipi_img_output *out,
					u32 sensor_h, u32 sensor_vb, u64 linet)
{
	struct mtkcam_qos_desc *qos_desc;
	unsigned int size;
	u32 peak_bw, avg_bw, active_h;
	int i, dst_port;

	qos_desc = find_qos_desc_by_uid(
			mmqos_img_table, ARRAY_SIZE(mmqos_img_table), out->uid.id);
	if (!qos_desc) {
		if (CAM_DEBUG_ENABLED(MMQOS))
			pr_info("%s: can't find qos desc in table uid:%d", __func__, out->uid.id);
		return 0;
	}

	for (i = 0; i < qos_desc->desc_size &&
				i < ARRAY_SIZE(out->fmt.stride); i++) {
		size = get_ufbc_size(out->fmt.format,
					qos_desc->dma_desc[i].ufbc_type,
					out->fmt.s.w, out->fmt.s.h);
		size = (size == 0) ? out->buf[0][i].size : size;
		if (!size)
			break;

		if (WARN_ON(size > MMQOS_SIZE_WARNING)) {
			pr_info("%s: %s: req_seq(%d) uid:%d size too large(%d), please check\n",
				__func__,
				qos_desc->dma_desc[i].dma_name,
				job->req_seq, out->uid.id, size);
			continue;
		}

		dst_port = CALL_PLAT_HW(query_icc_path_idx,
			qos_desc->dma_desc[i].domain, qos_desc->dma_desc[i].dst_port);
		if (dst_port < 0)
			continue;

		active_h = sensor_h + sensor_vb;
		avg_bw = calc_bw(size, linet, active_h);
		active_h = (out->crop.s.h == 0) ? out->fmt.s.h : out->crop.s.h;
		peak_bw = (is_dc_mode(job) || is_m2m(job)) ?
				0 : calc_bw(size, linet, active_h);

		switch (qos_desc->dma_desc[i].domain) {
		case RAW_DOMAIN:
			job->raw_mmqos[dst_port].peak_bw += peak_bw;
			job->raw_mmqos[dst_port].avg_bw += avg_bw;
		break;
		case RAW_W_DOMAIN:
			job->raw_w_mmqos[dst_port].peak_bw += peak_bw;
			job->raw_w_mmqos[dst_port].avg_bw += avg_bw;
		break;
		case YUV_DOMAIN:
			job->yuv_mmqos[dst_port].peak_bw += peak_bw;
			job->yuv_mmqos[dst_port].avg_bw += avg_bw;
		break;
		default:
			pr_info("%s: unsupport domain(%d)\n", __func__,
				qos_desc->dma_desc[i].domain);
		break;
		}

		if (CAM_DEBUG_ENABLED(MMQOS))
			pr_info("%s: %s: req_seq(%d) dst_port:%d uid:%d avg_bw:%d KB/s, peak_bw:%d KB/s (size:%d active_h:%d vb:%d)\n",
				__func__, qos_desc->dma_desc[i].dma_name, job->req_seq,
				dst_port, out->uid.id, avg_bw, peak_bw,
				size, active_h, sensor_vb);
	}

	return 0;
}

static int fill_raw_in_qos(struct mtk_cam_job *job,
					struct mtkcam_ipi_img_input *in,
					u32 sensor_h, u32 sensor_vb, u64 linet)
{
	struct mtkcam_qos_desc *qos_desc = NULL;
	struct mtkcam_qos_desc *imgo_qos_desc = NULL;
	unsigned int size;
	u32 peak_bw, avg_bw;
	int i, dst_port;

	qos_desc = find_qos_desc_by_uid(
			mmqos_img_table, ARRAY_SIZE(mmqos_img_table), in->uid.id);
	if (!qos_desc) {
		pr_info("%s: can't find qos desc in table uid:%d", __func__, in->uid.id);
		return 0;
	}

	/* for mstream 1st ipi update */
	if (job->job_type == JOB_TYPE_MSTREAM &&
		in->uid.id == MTKCAM_IPI_RAW_RAWI_2) {
		imgo_qos_desc = find_qos_desc_by_uid(
			mmqos_img_table, ARRAY_SIZE(mmqos_img_table), MTKCAM_IPI_RAW_IMGO);
	}

	for (i = 0; i < qos_desc->desc_size && i < ARRAY_SIZE(in->fmt.stride); i++) {
		size = get_ufbc_size(in->fmt.format,
					qos_desc->dma_desc[i].ufbc_type,
					in->fmt.s.w, in->fmt.s.h);
		size = (size == 0) ? in->buf[i].size : size;
		if (!size)
			break;

		if (WARN_ON(size > MMQOS_SIZE_WARNING)) {
			pr_info("%s: %s: req_seq(%d) uid:%d size too large(%d), please check\n",
				__func__,
				qos_desc->dma_desc[i].dma_name,
				job->req_seq, in->uid.id, size);
			continue;
		}

		dst_port = CALL_PLAT_HW(query_icc_path_idx,
				qos_desc->dma_desc[i].domain, qos_desc->dma_desc[i].dst_port);
		if (dst_port < 0)
			continue;

		avg_bw = calc_bw(size, linet, sensor_h + sensor_vb);
		peak_bw = (is_dc_mode(job) || is_m2m(job)) ?
				0 : calc_bw(size, linet, sensor_h);

		switch (qos_desc->dma_desc[i].domain) {
		case RAW_DOMAIN:
		case ADL_DOMAIN:
			job->raw_mmqos[dst_port].peak_bw += peak_bw;
			job->raw_mmqos[dst_port].avg_bw += avg_bw;
		break;
		case RAW_W_DOMAIN:
			job->raw_w_mmqos[dst_port].peak_bw += peak_bw;
			job->raw_w_mmqos[dst_port].avg_bw += avg_bw;
		break;
		default:
			pr_info("%s: unsupport domain(%d)\n", __func__,
				qos_desc->dma_desc[i].domain);
		break;
		}

		if (CAM_DEBUG_ENABLED(MMQOS))
			pr_info("%s: %s: req_seq(%d) dst_port:%d uid:%d avg_bw:%d KB/s, peak_bw:%d KB/s (size:%d height:%d vb:%d)\n",
				__func__, qos_desc->dma_desc[i].dma_name, job->req_seq,
				dst_port, in->uid.id, avg_bw, peak_bw, size, sensor_h, sensor_vb);

		/* for mstream 1st ipi */
		if (imgo_qos_desc) {
			dst_port = imgo_qos_desc->dma_desc[i].dst_port;
			if (dst_port < 0)
				continue;
			job->raw_mmqos[dst_port].peak_bw = peak_bw;
			job->raw_mmqos[dst_port].avg_bw = avg_bw;

			if (CAM_DEBUG_ENABLED(MMQOS))
				pr_info("%s: %s: req_seq(%d) dst_port:%d uid:%d avg_bw:%d KB/s, peak_bw:%d KB/s (size:%d height:%d vb:%d)\n",
					__func__, imgo_qos_desc->dma_desc[i].dma_name,
					job->req_seq, dst_port, in->uid.id, avg_bw, peak_bw,
					size, sensor_h, sensor_vb);
		}
	}

	return 0;
}

static int fill_raw_stats_qos(struct req_buffer_helper *helper,
					u32 sensor_h, u32 sensor_vb, u64 linet)
{
	struct mtk_cam_job *job = helper->job;
	struct mtkcam_qos_desc *qos_desc;
	unsigned int size = 0;
	void *meta_va = NULL;
	u32 peak_bw, avg_bw;
	int i, j, dst_port;

	for (i = 0; i < ARRAY_SIZE(mmqos_stats_table); i++) {
		qos_desc = &mmqos_stats_table[i];

		if (qos_desc->id == MTKCAM_IPI_RAW_META_STATS_CFG)
			meta_va = helper->meta_cfg_buf_va;
		else if (qos_desc->id == MTKCAM_IPI_RAW_META_STATS_0)
			meta_va = helper->meta_stats0_buf_va;
		else if (qos_desc->id == MTKCAM_IPI_RAW_META_STATS_1)
			meta_va = helper->meta_stats1_buf_va;
		else
			continue;

		for (j = 0; j < qos_desc->desc_size; j++) {
			/* for multi exposure */
			if (qos_desc->dma_desc[j].exp_num > job_exp_num(job))
				continue;

			CALL_PLAT_V4L2(get_meta_stats_port_size,
				qos_desc->id, meta_va, qos_desc->dma_desc[j].src_port, &size);
			if (!size)
				continue;

			if (WARN_ON(size > MMQOS_SIZE_WARNING)) {
				pr_info("%s: %s: req_seq(%d) size too large(%d), please check\n",
					__func__,
					qos_desc->dma_desc[i].dma_name,
					job->req_seq, size);
				continue;
			}

			dst_port = CALL_PLAT_HW(query_icc_path_idx,
				qos_desc->dma_desc[j].domain, qos_desc->dma_desc[j].dst_port);
			if (dst_port < 0)
				continue;

			avg_bw = calc_bw(size, linet, sensor_h + sensor_vb);
			peak_bw = (is_dc_mode(job) || is_m2m(job)) ? 0 : avg_bw;

			switch (qos_desc->dma_desc[j].domain) {
			case RAW_DOMAIN:
				job->raw_mmqos[dst_port].peak_bw += peak_bw;
				job->raw_mmqos[dst_port].avg_bw += avg_bw;
			break;
			case YUV_DOMAIN:
				job->yuv_mmqos[dst_port].peak_bw += peak_bw;
				job->yuv_mmqos[dst_port].avg_bw += avg_bw;
			break;
			default:
				pr_info("%s: unsupport domain(%d)\n", __func__,
					qos_desc->dma_desc[j].domain);
			break;
			}

			if (CAM_DEBUG_ENABLED(MMQOS))
				pr_info("%s: %s: req_seq(%d) dst_port:%d avg_bw:%d KB/s, peak_bw:%d KB/s (size:%d height:%d)\n",
					__func__, qos_desc->dma_desc[j].dma_name, job->req_seq,
					dst_port, avg_bw, peak_bw, size, sensor_h);
		}
	}

	return 0;
}

static int fill_sv_qos(struct mtk_cam_job *job,
						struct mtkcam_ipi_frame_param *fp,
						u32 sensor_h, u32 sensor_vb, u64 linet,
						u32 sensor_fps)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtkcam_ipi_camsv_frame_param *sv_param;
	struct mtkcam_ipi_img_output *in;
	struct mtk_camsv_device *sv_dev;
	unsigned int i, x_size, img_h, sv_id;
	u64 avg_bw, peak_bw, stash_avg_bw, stash_peak_bw;
	unsigned int is_two_smi_out = 0;
	bool is_smmu_enabled = true;

	/* cqi */
	avg_bw = peak_bw = to_qos_icc(CQ_BUF_SIZE * sensor_fps);
	job->sv_mmqos[SMI_PORT_SV_CQI].avg_bw += avg_bw;
	job->sv_mmqos[SMI_PORT_SV_CQI].peak_bw += peak_bw;
	if (CAM_DEBUG_ENABLED(MMQOS))
		pr_info("%s: sensor_h:%u/vb:%u/linet:%llu/fps:%u avg_bw:%llu_%u/peak_bw:%llu_%u\n",
			__func__,
			sensor_h, sensor_vb, linet, sensor_fps,
			avg_bw, job->sv_mmqos[SMI_PORT_SV_CQI].avg_bw,
			peak_bw, job->sv_mmqos[SMI_PORT_SV_CQI].peak_bw);

	if (bit_map_subset_of(MAP_HW_CAMSV, job->used_engine) == 0)
		return 0;
	sv_id = get_master_sv_id(job->used_engine);
	sv_dev = dev_get_drvdata(ctx->cam->engines.sv_devs[sv_id]);

	CALL_PLAT_V4L2(get_is_smmu_enabled, &is_smmu_enabled);

	/* wdma */
	for (i = 0; i < CAMSV_MAX_TAGS; i++) {
		stash_peak_bw = stash_avg_bw = 0;
		sv_param = &fp->camsv_param[0][i];
		in = &sv_param->camsv_img_outputs[0];

		x_size = in->fmt.stride[0];
		img_h = in->fmt.s.h;

		if (x_size >= 4096)
			sv_dev->enable_stash_eco_fun = true;

		CALL_PLAT_V4L2(
			get_sv_smi_setting, sv_dev->id, &is_two_smi_out);

		if (i < SVTAG_IMG_END) {
			avg_bw =
				calc_bw(x_size * img_h, linet, sensor_h + sensor_vb);
			peak_bw =
				calc_bw(x_size * img_h, linet, sensor_h);
			if (ipifmt_is_raw_ufo(in->fmt.format)) {
				/* compression ratio: 0.7x */
				avg_bw = avg_bw * 7 / 10;
				/* table */
				avg_bw = avg_bw +
					calc_bw(DIV_ROUND_UP(in->fmt.s.w, 64) * img_h,
						linet, img_h + sensor_vb);
			}
			/* camsv stash fixed at 5ostdl = 5mb */
			if (is_smmu_enabled) {
				if (avg_bw || peak_bw)
					stash_peak_bw = stash_avg_bw = to_qos_icc(2097152);
			}

		} else {
			avg_bw =
				calc_bw(x_size * img_h, linet, sensor_h + sensor_vb);
			peak_bw =
				calc_bw(x_size * img_h, linet, sensor_h);
			/* camsv stash fixed at 5ostdl = 5mb */
			if (avg_bw || peak_bw)
				stash_peak_bw = stash_avg_bw = to_qos_icc(2097152);
		}

		if (is_two_smi_out) {
			job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].avg_bw += avg_bw / 2;
			job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].peak_bw += peak_bw / 2;
			job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].avg_bw += avg_bw / 2;
			job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].peak_bw += peak_bw / 2;
			if (is_smmu_enabled) {
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_0].avg_bw += stash_avg_bw;
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_0].peak_bw += stash_peak_bw;
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_1].avg_bw += stash_avg_bw;
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_1].peak_bw += stash_peak_bw;
			}
		} else {
			job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].avg_bw += avg_bw;
			job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].peak_bw += peak_bw;
			if (is_smmu_enabled) {
				job->sv_mmqos[SMI_PORT_SV_DISP_STG_0].avg_bw += stash_avg_bw;
				job->sv_mmqos[SMI_PORT_SV_DISP_STG_0].peak_bw += stash_peak_bw;
			}
		}
		if (CAM_DEBUG_ENABLED(MMQOS) && is_smmu_enabled)
			pr_info("%s: xsize:%u/height:%u sensor_h:%u/vb:%u/linet:%llu/fps:%u avg_bw:%llu_%u_%u_%u_%u_%u_%u_%u_%u/peak_bw:%llu_%u_%u_%u_%u_%u_%u_%u_%u\n",
				__func__, x_size, img_h,
				sensor_h, sensor_vb, linet, sensor_fps,
				avg_bw, job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_STG_0].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_0].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_1].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_STG_1].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_1].avg_bw,
				peak_bw, job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_STG_0].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_0].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_1].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_STG_1].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_STG_1].peak_bw);
		if (CAM_DEBUG_ENABLED(MMQOS) && !is_smmu_enabled)
			pr_info("%s: xsize:%u/height:%u sensor_h:%u/vb:%u/linet:%llu/fps:%u avg_bw:%llu_%u_%u_%u_%u/peak_bw:%llu_%u_%u_%u_%u\n",
				__func__, x_size, img_h,
				sensor_h, sensor_vb, linet, sensor_fps,
				avg_bw, job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_1].avg_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].avg_bw,
				peak_bw, job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_1].peak_bw,
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].peak_bw);
	}

	return 0;
}

static int fill_mraw_qos(struct mtk_cam_job *job,
						struct mtkcam_ipi_frame_param *fp,
						u32 sensor_h, u32 sensor_vb, u64 linet,
						u32 sensor_fps)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtkcam_ipi_mraw_frame_param *mraw_param;
	struct mtkcam_ipi_img_output *in;
	struct mtk_mraw_device *mraw_dev;
	unsigned int i, j, k, port_id, x_size, img_h;
	u64 avg_bw, peak_bw;
	u64 stash_avg_bw, stash_peak_bw, strideLCM;
	bool is_smmu_enabled = true;

	CALL_PLAT_V4L2(get_is_smmu_enabled, &is_smmu_enabled);

	for (i = 0 ; i < MRAW_MAX_PIPE_USED; i++) {
		mraw_param = &fp->mraw_param[i];

		if (!is_mraw_subdev(mraw_param->pipe_id))
			continue;

		for (j = 0; j < MAX_MRAW_PIPES_PER_STREAM; j++) {
			if (ctx->hw_mraw[j] == NULL)
				continue;

			mraw_dev = dev_get_drvdata(ctx->hw_mraw[j]);
			if (mraw_dev->pipeline->id == mraw_param->pipe_id) {
				/* wdma */
				for (k = 0; k < MRAW_MAX_IMAGE_OUTPUT; k++) {

					port_id = SMI_PORT_MRAW_IMGO;
					stash_peak_bw = stash_avg_bw = 0;

					in = &mraw_param->mraw_img_outputs[k];
					x_size = in->fmt.stride[0];
					img_h = in->fmt.s.h;
					avg_bw =
						calc_bw(x_size * img_h, linet,
							sensor_h + sensor_vb);
					peak_bw =
						calc_bw(x_size * img_h, linet, sensor_h);

					job->mraw_mmqos[j][port_id].avg_bw += avg_bw;
					job->mraw_mmqos[j][port_id].peak_bw += peak_bw;

					/* stash */
					if (is_smmu_enabled && x_size != 0) {
						strideLCM = LCM(x_size, 4096);
						stash_peak_bw = stash_avg_bw =
							to_qos_icc(img_h / (strideLCM/x_size) * 16 * sensor_fps);
					}
					if (is_smmu_enabled) {
					job->mraw_mmqos[j][SMI_PORT_MRAW_STG].avg_bw += stash_avg_bw;
					job->mraw_mmqos[j][SMI_PORT_MRAW_STG].peak_bw += stash_peak_bw;
					}
					if (CAM_DEBUG_ENABLED(MMQOS) && is_smmu_enabled)
						pr_info("%s: id:%d port_id:%d xsize:%u/height:%u sensor_h:%u/vb:%u/linet:%llu avg_bw:%llu_%u_%u/peak_bw:%llu_%u_%u\n",
							__func__, k, port_id, x_size, img_h,
							sensor_h, sensor_vb, linet,
							avg_bw,
							job->mraw_mmqos[j][port_id].avg_bw,
							job->mraw_mmqos[j][SMI_PORT_MRAW_STG].avg_bw,
							peak_bw,
							job->mraw_mmqos[j][port_id].peak_bw,
							job->mraw_mmqos[j][SMI_PORT_MRAW_STG].peak_bw);
					if (CAM_DEBUG_ENABLED(MMQOS) && !is_smmu_enabled)
						pr_info("%s: id:%d port_id:%d xsize:%u/height:%u sensor_h:%u/vb:%u/linet:%llu avg_bw:%llu_%u/peak_bw:%llu_%u\n",
							__func__, k, port_id, x_size, img_h,
							sensor_h, sensor_vb, linet,
							avg_bw,
							job->mraw_mmqos[j][port_id].avg_bw,
							peak_bw,
							job->mraw_mmqos[j][port_id].peak_bw);

				}

				/* cqi */
				avg_bw = peak_bw = to_qos_icc(CQ_BUF_SIZE * sensor_fps);
				job->mraw_mmqos[j][SMI_PORT_MRAW_CQI].avg_bw += avg_bw;
				job->mraw_mmqos[j][SMI_PORT_MRAW_CQI].peak_bw += peak_bw;
				if (CAM_DEBUG_ENABLED(MMQOS))
					pr_info("%s: sensor_h:%u/vb:%u/linet:%llu/fps:%u avg_bw:%llu_%u/peak_bw:%llu_%u\n",
						__func__,
						sensor_h, sensor_vb, linet, sensor_fps,
						avg_bw,
						job->mraw_mmqos[j][SMI_PORT_MRAW_CQI].avg_bw,
						peak_bw,
						job->mraw_mmqos[j][SMI_PORT_MRAW_CQI].peak_bw);
				break;
			}
		}
	}

	return 0;
}

static void update_sensor_active_info(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;

	if (!job->seninf)
		return;

	/* get active line time */
	if (ctx->act_line_info.avg_linetime_in_ns == 0 ||
		job->seamless_switch || job->raw_switch) {
		struct mtk_raw_sink_data *sink = get_raw_sink_data(job);

		if (!sink) {
			pr_info("%s: raw_data not found: ctx-%d job %d\n",
				 __func__, ctx->stream_id, job->frame_seq_no);
			return;
		}

		mtk_cam_seninf_get_active_line_info(
			job->seninf, sink->mbus_code, &ctx->act_line_info);
	}
}

void mtk_cam_fill_qos(struct req_buffer_helper *helper)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtk_cam_job *job = helper->job;
	struct mtk_cam_ctx *ctx = job->src_ctx;
#if KERNEL_VERSION(6, 6, 0) == LINUX_VERSION_CODE
	struct ISP_P1 idx;
#endif
	u32 senser_vb, sensor_h, sensor_fps;
	u64 avg_linet;
	int i;

	memset(job->raw_mmqos, 0, sizeof(job->raw_mmqos));
	memset(job->raw_w_mmqos, 0, sizeof(job->raw_w_mmqos));
	memset(job->yuv_mmqos, 0, sizeof(job->yuv_mmqos));
	memset(job->sv_mmqos, 0, sizeof(job->sv_mmqos));
	memset(job->mraw_mmqos, 0, sizeof(job->mraw_mmqos));

	update_sensor_active_info(job);
	avg_linet =  ctx->act_line_info.avg_linetime_in_ns ? : get_line_time(job);
	sensor_h = ctx->act_line_info.active_line_num ? : get_sensor_h(job);
	senser_vb = get_sensor_vb(job);
	sensor_fps = get_sensor_fps(job);
#if KERNEL_VERSION(6, 6, 0) == LINUX_VERSION_CODE
	if (cam_sspm_en && (job->frame_seq_no % 10 == 0)) {
		memset(&idx, 0, sizeof(idx));
		idx.raw_num = get_used_raw_num(job);
		idx.exposure_num = job_exp_num(job);
		idx.fps = sensor_fps;
		idx.data = get_sensor_w(job) * sensor_h;
		dev_info(job->src_ctx->cam->dev,
			"[%s:p1_pmidx] FPS:%u, raw_num:%u, exp_num:%u, data:%u\n",
			__func__, idx.fps, idx.raw_num,
			idx.exposure_num, idx.data);
		set_p1_idx(idx);
	}
#endif
	if (avg_linet == 0 || sensor_h == 0 || sensor_fps == 0) {
		pr_info("%s: wrong sensor param h/vb/linetime/fps: %d/%d/%llu/%d",
			__func__, sensor_h, senser_vb, avg_linet, sensor_fps);
		return;
	}

	for (i = 0; i < helper->io_idx; i++)
		fill_raw_out_qos(job, &fp->img_outs[i], sensor_h, senser_vb, avg_linet);

	for (i = 0; i < helper->ii_idx; i++)
		fill_raw_in_qos(job, &fp->img_ins[i], sensor_h, senser_vb, avg_linet);

	fill_raw_stats_qos(helper, sensor_h, senser_vb, avg_linet);

	/* camsv */
	fill_sv_qos(job, fp, sensor_h, senser_vb, avg_linet, sensor_fps);

	/* mraw */
	fill_mraw_qos(job, fp, sensor_h, senser_vb, avg_linet, sensor_fps);
}

static bool apply_qos_chk(
		u32 new_avg, u32 new_peak, s64 *applied, s64 *pending)
{
	s64 bw = (new_peak > 0) ? new_peak : new_avg;
	bool ret = false;

	if (bw > 0 && bw > *applied) {
		*applied = bw;
		*pending = -1;
		ret = true;
	} else if (bw < *applied) {
		if (*pending >= 0 && bw >= *pending) {
			*applied = (bw == 0) ? -1 : bw;
			*pending = -1;
			ret = true;
		} else {
			*pending = bw;
			ret = false;
		}
	} else if (bw > 0 && bw == *applied) {
		*pending = -1;
		ret = false;
	}

	return ret;
}

static void apply_raw_qos(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_engines *eng = &cam->engines;
	struct mtk_raw_device *raw_dev;
	struct mtk_yuv_device *yuv_dev;
	unsigned long submask;
	u32 a_bw, p_bw, used_raw_num;
	bool apply, apply_bwr, is_wchannel, is_w_port;
	int i, j, raw_num;
	int raw_avg_bw_r = 0, raw_avg_bw_w = 0, raw_peak_bw_r = 0, raw_peak_bw_w = 0;
	int yuv_avg_bw_r = 0, yuv_avg_bw_w = 0, yuv_peak_bw_r = 0, yuv_peak_bw_w = 0;
	int a_bw_ttl = 0, p_bw_ttl = 0;

	raw_num = eng->num_raw_devices;
	submask = bit_map_subset_of(MAP_HW_RAW, ctx->used_engine);

	for (i = 0; i < raw_num && submask; i++, submask >>= 1) {
		used_raw_num = is_rgbw(job) ? 1 : get_used_raw_num(job);
		if (WARN_ON(used_raw_num == 0)) {
			pr_info("%s: req_seq(%d) wrong used raw number\n",
				__func__, job->req_seq);
			continue;
		}

		if (!(submask & 0x1))
			continue;

		raw_dev = dev_get_drvdata(eng->raw_devs[i]);
		is_wchannel = (is_rgbw(job) && raw_dev->is_slave) ? true : false;
		for (j = 0; j < raw_dev->qos.n_path; j++) {
			if (is_adl_port(j))
				continue;

			is_w_port = is_w_merge_port(j, RAW_DOMAIN);
			a_bw = is_wchannel ?
				(job->raw_w_mmqos[j].avg_bw) :
				(job->raw_mmqos[j].avg_bw / used_raw_num);
			p_bw = is_wchannel ?
				(job->raw_w_mmqos[j].peak_bw) :
				(job->raw_mmqos[j].peak_bw / used_raw_num);
			raw_peak_bw_w += is_w_port ? p_bw : 0;
			raw_avg_bw_w += is_w_port ? a_bw : 0;
			raw_peak_bw_r += is_w_port ? 0 : p_bw;
			raw_avg_bw_r += is_w_port ? 0 : a_bw;
			a_bw_ttl += a_bw;
			p_bw_ttl += p_bw;

			apply = apply_qos_chk(a_bw, p_bw,
					&raw_dev->qos.cam_path[j].applied_bw,
					&raw_dev->qos.cam_path[j].pending_bw);
			if (apply) {
				apply_bwr = true;
				mtk_icc_set_bw(raw_dev->qos.cam_path[j].path, a_bw, p_bw);
			}

			if (CAM_DEBUG_ENABLED(MMQOS))
				pr_info("%s: req_seq:%d %s raw-%d icc_path:%s avg/peak:%u/%u(KB/s) applied/pending:%lld/%lld(KB/s)\n",
						__func__, job->req_seq,
						apply ? "APPLY" : "BYPASS", i,
						raw_dev->qos.cam_path[j].name, a_bw, p_bw,
						raw_dev->qos.cam_path[j].applied_bw,
						raw_dev->qos.cam_path[j].pending_bw);
		}

		if (!is_wchannel) {
			yuv_dev = dev_get_drvdata(eng->yuv_devs[i]);
			for (j = 0; j < yuv_dev->qos.n_path; j++) {
				is_w_port = is_w_merge_port(j, YUV_DOMAIN);
				a_bw = job->yuv_mmqos[j].avg_bw / used_raw_num;
				p_bw = job->yuv_mmqos[j].peak_bw / used_raw_num;
				yuv_peak_bw_w += is_w_port ? p_bw : 0;
				yuv_avg_bw_w += is_w_port ? a_bw : 0;
				yuv_peak_bw_r += is_w_port ? 0 : p_bw;
				yuv_avg_bw_r += is_w_port ? 0 : a_bw;
				a_bw_ttl += a_bw;
				p_bw_ttl += p_bw;

				apply = apply_qos_chk(a_bw, p_bw,
					&yuv_dev->qos.cam_path[j].applied_bw,
					&yuv_dev->qos.cam_path[j].pending_bw);

				if (apply) {
					apply_bwr = true;
					mtk_icc_set_bw(yuv_dev->qos.cam_path[j].path, a_bw, p_bw);
				}

				if (CAM_DEBUG_ENABLED(MMQOS))
					pr_info("%s: req_seq:%d %s yuv-%d icc_path:%s avg/peak:%u/%u(KB/s) applied/pending:%lld/%lld(KB/s)\n",
							__func__, job->req_seq,
							apply ? "APPLY" : "BYPASS", i,
							yuv_dev->qos.cam_path[j].name, a_bw, p_bw,
							yuv_dev->qos.cam_path[j].applied_bw,
							yuv_dev->qos.cam_path[j].pending_bw);
			}
		}

		if (apply_bwr) {
			mtk_cam_bwr_set_chn_bw(cam->bwr,
				get_bwr_engine(raw_dev->id), get_axi_port(raw_dev->id, true),
				KBps_to_bwr(raw_avg_bw_r), KBps_to_bwr(raw_avg_bw_w),
				KBps_to_bwr(raw_peak_bw_r), KBps_to_bwr(raw_peak_bw_w), true);

			mtk_cam_bwr_set_chn_bw(cam->bwr,
				get_bwr_engine(yuv_dev->id), get_axi_port(yuv_dev->id, false),
				KBps_to_bwr(yuv_avg_bw_r), KBps_to_bwr(yuv_avg_bw_w),
				KBps_to_bwr(yuv_peak_bw_r), KBps_to_bwr(yuv_peak_bw_w), true);

			mtk_cam_bwr_set_ttl_bw(cam->bwr,
				get_bwr_engine(raw_dev->id),
				KBps_to_bwr(a_bw_ttl), KBps_to_bwr(p_bw_ttl), true);

			raw_avg_bw_r = raw_avg_bw_w = raw_peak_bw_r = raw_peak_bw_w = 0;
			yuv_avg_bw_r = yuv_avg_bw_w = yuv_peak_bw_r = yuv_peak_bw_w = 0;
			a_bw_ttl = p_bw_ttl = 0;
		}
	}
}

static void apply_adl_qos(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_raw_device *raw_dev;
	int raw_id = get_master_raw_id(job->used_engine);
	int a_bw, p_bw, is_apu;
	int avg_bw_r = 0, avg_bw_w = 0, peak_bw_r = 0, peak_bw_w = 0;
	int a_bw_ttl = 0, p_bw_ttl = 0;
	int i;
	bool apply, is_w_port;

#ifdef RUN_ADL_FRAME_MODE_FROM_RAWI
	is_apu = is_m2m_apu_dc(job);
#else
	is_apu = is_m2m_apu(job);
#endif
	if (!is_apu)
		return;

	raw_dev = dev_get_drvdata(cam->engines.raw_devs[raw_id]);
	for (i = 0; i < raw_dev->qos.n_path; i++) {
		if (!is_adl_port(i))
			continue;

		is_w_port = is_w_merge_port(i, RAW_DOMAIN);
		a_bw = job->raw_mmqos[i].avg_bw;
		p_bw = job->raw_mmqos[i].peak_bw;
		avg_bw_r += is_w_port ? 0 : a_bw;
		avg_bw_w += is_w_port ? a_bw : 0;
		peak_bw_r += is_w_port ? 0 : p_bw;
		peak_bw_w += is_w_port ? p_bw : 0;
		a_bw_ttl += a_bw;
		p_bw_ttl += p_bw;

		apply = apply_qos_chk(a_bw, p_bw,
			&raw_dev->qos.cam_path[i].applied_bw,
			&raw_dev->qos.cam_path[i].pending_bw);
		if (apply) {
			mtk_icc_set_bw(raw_dev->qos.cam_path[i].path, a_bw, p_bw);

			mtk_cam_bwr_set_chn_bw(cam->bwr, ENGINE_CAM_MAIN, MDP0_PORT,
				KBps_to_bwr(avg_bw_r), KBps_to_bwr(avg_bw_w),
				KBps_to_bwr(peak_bw_r), KBps_to_bwr(peak_bw_w), true);

			mtk_cam_bwr_set_ttl_bw(cam->bwr, ENGINE_CAM_MAIN,
				KBps_to_bwr(a_bw_ttl), KBps_to_bwr(p_bw_ttl), true);
		}

		if (CAM_DEBUG_ENABLED(MMQOS))
			pr_info("%s: req_seq:%d %s raw-%d icc_path:%s avg/peak:%u/%u(KB/s) applied/pending:%lld/%lld(KB/s)\n",
					__func__, job->req_seq,
					apply ? "APPLY" : "BYPASS", i,
					raw_dev->qos.cam_path[i].name, a_bw, p_bw,
					raw_dev->qos.cam_path[i].applied_bw,
					raw_dev->qos.cam_path[i].pending_bw);
	}
}

static void apply_sv_qos(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_camsv_device *sv_dev = NULL;
	unsigned int fifo_img_p1, fifo_img_p2, fifo_len_p1, fifo_len_p2;
	unsigned int is_two_smi_out = 0;
	u32 a_bw, p_bw;
	int i, port_num;
	unsigned int leading_line_cnt, raw_id;
	bool apply, apply_sv_th = false, apply_bwr = false;
	int sv_avg_bw_w = 0, sv_peak_bw_w = 0, sv_avg_diff_bw_w = 0, sv_peak_diff_bw_w = 0;
	u64 avg_linet;

	if (ctx->has_raw_subdev) {
		raw_id = get_master_raw_id(job->used_engine);
		sv_dev = dev_get_drvdata(cam->engines.sv_devs[raw_id]);
	} else {
		if (ctx->hw_sv)
			sv_dev = dev_get_drvdata(ctx->hw_sv);
	}

	if (sv_dev) {
		CALL_PLAT_V4L2(
			get_sv_smi_setting, sv_dev->id, &is_two_smi_out);
		port_num = (is_two_smi_out) ?
			SMI_PORT_SV_TYPE0_NUM : SMI_PORT_SV_TYPE1_NUM;

		port_num = sv_dev->qos.n_path ? port_num : 0;

		for (i = 0; i < port_num; i++) {
			a_bw = job->sv_mmqos[i].avg_bw;
			p_bw = job->sv_mmqos[i].peak_bw;
			sv_avg_bw_w += a_bw;
			sv_peak_bw_w += p_bw;

			apply = apply_qos_chk(a_bw, p_bw,
					&sv_dev->qos.cam_path[i].applied_bw,
					&sv_dev->qos.cam_path[i].pending_bw);
			if (apply) {
				mtk_icc_set_bw(sv_dev->qos.cam_path[i].path, a_bw, p_bw);
				apply_sv_th = true;
				apply_bwr = true;
			}

			if (CAM_DEBUG_ENABLED(MMQOS))
				pr_info("%s: req_seq:%d %s sv-%d icc_path:%s avg/peak:%u/%u applied/pending:%lld/%lld\n",
						__func__, job->req_seq,
						apply ? "APPLY" : "BYPASS", sv_dev->id,
						sv_dev->qos.cam_path[i].name, a_bw, p_bw,
						sv_dev->qos.cam_path[i].applied_bw,
						sv_dev->qos.cam_path[i].pending_bw);
		}

		if (apply_bwr) {
			int a_bw_w_KB = KBps_to_bwr(sv_avg_bw_w);
			int p_bw_w_KB = KBps_to_bwr(sv_peak_bw_w);

			sv_avg_diff_bw_w = a_bw_w_KB - sv_dev->sv_avg_applied_bw_w;
			sv_peak_diff_bw_w = p_bw_w_KB - sv_dev->sv_peak_applied_bw_w;
			sv_dev->sv_avg_applied_bw_w = a_bw_w_KB;
			sv_dev->sv_peak_applied_bw_w = p_bw_w_KB;
			mtk_cam_bwr_set_chn_bw(cam->bwr,
				get_sv_bwr_engine(sv_dev->id), get_sv_axi_port(sv_dev->id),
				0, sv_avg_diff_bw_w, 0, sv_peak_diff_bw_w, false);

			mtk_cam_bwr_set_ttl_bw(cam->bwr,
				get_sv_bwr_engine(sv_dev->id), sv_avg_diff_bw_w, sv_peak_diff_bw_w, false);

		}

		if (apply_sv_th) {
			/* apply fifo setting according to bandwidth */
			fifo_img_p1 =
				(job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_0].peak_bw +
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_0].peak_bw) * 64 * 12 / 1000000;
			fifo_img_p2 =
				(job->sv_mmqos[SMI_PORT_SV_DISP_WDMA_1].peak_bw +
				job->sv_mmqos[SMI_PORT_SV_MDP_WDMA_1].peak_bw) * 64 * 12 / 1000000;
			fifo_len_p1 = fifo_img_p1 / 80;
			fifo_len_p2 = fifo_img_p2 / 80;
			avg_linet = ctx->act_line_info.avg_linetime_in_ns ? : get_line_time(job);
			leading_line_cnt = (avg_linet && avg_linet < 12500) ? 12500 / avg_linet : 1;
			if (leading_line_cnt > 8)
				pr_info("%s: unexpected leading_line_cnt:%d", __func__, leading_line_cnt);
			mtk_cam_sv_dmao_common_config(sv_dev, fifo_img_p1, fifo_img_p2, fifo_len_p1, fifo_len_p2,
				(leading_line_cnt - 1) & 0x7, sv_dev->enable_stash_eco_fun);

			/* apply golden setting */
			mtk_cam_sv_golden_set(sv_dev, is_dc_mode(job) ? true : false);
		}
	}

}

static void apply_mraw_qos(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_mraw_device *mraw_dev;
	u32 a_bw, p_bw;
	int mraw_avg_bw_w = 0, mraw_peak_bw_w = 0;
	int mraw_avg_diff_bw_w = 0, mraw_peak_diff_bw_w = 0;
	int i, j;
	bool apply, apply_bwr = false;

	for (i = 0; i < MAX_MRAW_PIPES_PER_STREAM; i++) {
		if (ctx->hw_mraw[i]) {
			mraw_dev = dev_get_drvdata(ctx->hw_mraw[i]);
			for (j = 0; j < mraw_dev->qos.n_path; j++) {
				a_bw = job->mraw_mmqos[i][j].avg_bw;
				p_bw = job->mraw_mmqos[i][j].peak_bw;
				mraw_peak_bw_w += p_bw;
				mraw_avg_bw_w += a_bw;

				apply = apply_qos_chk(a_bw, p_bw,
						&mraw_dev->qos.cam_path[j].applied_bw,
						&mraw_dev->qos.cam_path[j].pending_bw);
				if (apply) {
					mtk_icc_set_bw(mraw_dev->qos.cam_path[j].path, a_bw, p_bw);
					apply_bwr = true;
				}

				if (CAM_DEBUG_ENABLED(MMQOS))
					pr_info("%s: req_seq:%d %s mraw-%d icc_path:%s avg/peak:%u/%u applied/pending:%lld/%lld\n",
							__func__, job->req_seq,
							apply ? "APPLY" : "BYPASS", mraw_dev->id,
							mraw_dev->qos.cam_path[j].name, a_bw, p_bw,
							mraw_dev->qos.cam_path[j].applied_bw,
							mraw_dev->qos.cam_path[j].pending_bw);
			}
			if (apply_bwr) {
				int a_bw_w_KB = KBps_to_bwr(mraw_avg_bw_w);
				int p_bw_w_KB = KBps_to_bwr(mraw_peak_bw_w);

				mraw_avg_diff_bw_w = a_bw_w_KB - mraw_dev->mraw_avg_applied_bw_w;
				mraw_peak_diff_bw_w = p_bw_w_KB - mraw_dev->mraw_peak_applied_bw_w;
				mraw_dev->mraw_avg_applied_bw_w = a_bw_w_KB;
				mraw_dev->mraw_peak_applied_bw_w = p_bw_w_KB;

				mtk_cam_bwr_set_chn_bw(cam->bwr,
					ENGINE_MRAW, get_mraw_axi_port(mraw_dev->id),
					0, mraw_avg_diff_bw_w, 0, mraw_peak_diff_bw_w, false);

				mtk_cam_bwr_set_ttl_bw(cam->bwr, ENGINE_MRAW,
					mraw_avg_diff_bw_w, mraw_peak_diff_bw_w, false);
			}
		}
	}
}

/* threaded irq context */
int mtk_cam_apply_qos(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;

	qof_mtcmos_voter(&cam->engines, job->used_engine, true);
	apply_raw_qos(job);
	apply_adl_qos(job);
	qof_mtcmos_voter(&cam->engines, job->used_engine, false);

	apply_sv_qos(job);
	apply_mraw_qos(job);

	if (CAM_DEBUG_ENABLED(MMQOS))
		mtk_cam_bwr_dbg_dump(cam->bwr);

	/* note: may sleep */
	mtk_mmqos_wait_throttle_done();

	return 0;
}

/* reset after disable irq */
int mtk_cam_reset_qos(struct device *dev, struct mtk_camsys_qos *qos)
{
	struct mtk_camsys_qos_path *cam_path;
	int i;

	for (i = 0, cam_path = qos->cam_path; i < qos->n_path; i++, cam_path++) {
		if (cam_path->applied_bw >= 0) {
			mtk_icc_set_bw(cam_path->path, 0, 0);
			cam_path->applied_bw = -1;
			cam_path->pending_bw = -1;
		}
	}

	if (CAM_DEBUG_ENABLED(MMQOS))
		dev_info(dev, "mmqos reset done\n");

	return 0;
}
