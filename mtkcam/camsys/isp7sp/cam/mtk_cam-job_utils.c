// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.


#include "mtk_cam.h"
#include "mtk_cam-fmt_utils.h"
#include "mtk_cam-job_utils.h"
#include "mtk_cam-ufbc-def.h"
#include "mtk_cam-raw_ctrl.h"
#include "mtk_cam-video.h"

static unsigned int sv_pure_raw = 1;
module_param(sv_pure_raw, uint, 0644);
MODULE_PARM_DESC(sv_pure_raw, "enable pure raw dump with casmsv");

static unsigned int debug_dram_ring_mode;
module_param(debug_dram_ring_mode, uint, 0644);
MODULE_PARM_DESC(debug_dram_ring_mode, "enable ringbuffer on dram");

#define buf_printk(fmt, arg...)					\
	do {							\
		if (unlikely(CAM_DEBUG_ENABLED(IPI_BUF)))	\
			pr_info("%s: " fmt, __func__, ##arg);	\
	} while (0)


static struct mtk_cam_resource_v2 *_get_job_res(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_resource_v2 *res = NULL;

	if (ctx->has_raw_subdev) {
		struct mtk_raw_ctrl_data *ctrl;

		ctrl = get_raw_ctrl_data(job);
		if (!ctrl)
			return NULL;

		res = &ctrl->resource.user_data;
	}

	return res;
}

static struct mtk_cam_resource_sensor_v2 *
_get_job_sensor_res(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_resource_sensor_v2 *sensor_res = NULL;

	if (ctx->has_raw_subdev) {
		struct mtk_raw_ctrl_data *ctrl;

		ctrl = get_raw_ctrl_data(job);
		if (!ctrl)
			return NULL;

		sensor_res = &ctrl->resource.user_data.sensor_res;
	} else {
		struct mtk_camsv_device *sv_dev;

		if (ctx->hw_sv == NULL)
			return NULL;
		sv_dev = dev_get_drvdata(ctx->hw_sv);

		sensor_res = &sv_dev->sensor_res;
	}

	return sensor_res;
}

u32 get_used_raw_num(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_engines *eng = &ctx->cam->engines;
	unsigned long mask;
	u32 raw_cnt = 0;
	int i;

	mask = bit_map_subset_of(MAP_HW_RAW, ctx->used_engine);
	for (i = 0; i < eng->num_raw_devices && mask; i++, mask >>= 1)
		if (mask & 0x1)
			++raw_cnt;

	return raw_cnt;
}

u64 get_line_time(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_sensor_v2 *sensor_res;
	u64 linet = 0;

	sensor_res = _get_job_sensor_res(job);
	if (sensor_res) {
		linet = 1000000000L * sensor_res->interval.numerator
			/ sensor_res->interval.denominator
			/ (sensor_res->height + sensor_res->vblank);
	}

	return linet;
}

u32 get_sensor_h(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_sensor_v2 *sensor_res;

	sensor_res = _get_job_sensor_res(job);
	if (sensor_res)
		return sensor_res->height;

	return 0;
}

u32 get_sensor_vb(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_sensor_v2 *sensor_res;

	sensor_res = _get_job_sensor_res(job);
	if (sensor_res)
		return sensor_res->vblank;

	return 0;
}

u32 get_sensor_fps(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_sensor_v2 *sensor_res;
	u32 fps = 0;

	sensor_res = _get_job_sensor_res(job);
	if (sensor_res) {
		fps = sensor_res->interval.denominator /
			sensor_res->interval.numerator;
		return fps;
	}

	return 0;
}

u32 get_sensor_interval_us(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_sensor_v2 *sensor_res;

	sensor_res = _get_job_sensor_res(job);
	if (!sensor_res) {
		pr_info_ratelimited("%s: warn. assume 30fps\n", __func__);
		return 33333;
	}

	return (u32)(1000000ULL * sensor_res->interval.numerator /
		     sensor_res->interval.denominator);
}

u8 get_sensor_data_pattern(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_sensor_v2 *sensor_res;

	sensor_res = _get_job_sensor_res(job);
	if (sensor_res)
		return sensor_res->pattern;

	return MTK_CAM_PATTERN_BAYER;
}

void _set_timestamp(struct mtk_cam_job *job,
	u64 time_boot, u64 time_mono)
{
	job->timestamp = time_boot;
	job->timestamp_mono = time_mono;
}

int get_raw_subdev_idx(unsigned long used_pipe)
{
	unsigned long used_raw = bit_map_subset_of(MAP_SUBDEV_RAW, used_pipe);

	return ffs(used_raw) - 1;
}

int get_sv_subdev_idx(unsigned long used_pipe)
{
	unsigned long used_sv = bit_map_subset_of(MAP_SUBDEV_CAMSV, used_pipe);

	return ffs(used_sv) - 1;
}

int get_sv_tag_idx_hdr(unsigned int exp_no, unsigned int tag_order, bool is_w)
{
	struct mtk_camsv_tag_param img_tag_param[SVTAG_IMG_END];
	unsigned int hw_scen, req_amount;
	int i, tag_idx = -1;

	hw_scen = 1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER);
	req_amount = (exp_no < 3) ? exp_no * 2 : exp_no;
	if (mtk_cam_sv_get_tag_param(img_tag_param, hw_scen, exp_no, req_amount))
		goto EXIT;
	else {
		for (i = 0; i < req_amount; i++) {
			if (img_tag_param[i].tag_order == tag_order &&
				img_tag_param[i].is_w == is_w) {
				tag_idx = img_tag_param[i].tag_idx;
				break;
			}
		}
	}

EXIT:
	return tag_idx;
}

int get_hw_scenario(struct mtk_cam_job *job)
{
	struct mtk_cam_scen *scen = &job->job_scen;
	int is_dc = is_dc_mode(job);
	int is_w = is_rgbw(job);
	int hard_scenario = MTKCAM_IPI_HW_PATH_ON_THE_FLY;
	int is_sv_only = job->job_type == JOB_TYPE_ONLY_SV;

	if (is_sv_only) {
		hard_scenario = MTKCAM_IPI_HW_PATH_ON_THE_FLY;
		goto END;
	}

	switch (scen->id) {
	case MTK_CAM_SCEN_NORMAL:
		if (is_w) {
			hard_scenario = (is_dc) ? MTKCAM_IPI_HW_PATH_DC_RGBW :
				MTKCAM_IPI_HW_PATH_OTF_RGBW;
		} else if (scen->scen.normal.exp_num > 1) {
			if (is_dc) {
				hard_scenario = MTKCAM_IPI_HW_PATH_DC_STAGGER;
			} else if (scen->scen.normal.stagger_type ==
					   MTK_CAM_STAGGER_DCG_AP_MERGE) {
				// TODO: OTF RGBW
				// OTF DCG AP merge
				hard_scenario = MTKCAM_IPI_HW_PATH_OTF_STAGGER_LN_INTL;
			} else {
				hard_scenario = MTKCAM_IPI_HW_PATH_STAGGER;
			}
		} else
			hard_scenario = is_dc ?
				MTKCAM_IPI_HW_PATH_DC_STAGGER :
				MTKCAM_IPI_HW_PATH_ON_THE_FLY;
		break;
	case MTK_CAM_SCEN_MSTREAM:
		hard_scenario = MTKCAM_IPI_HW_PATH_MSTREAM;
		break;
	case MTK_CAM_SCEN_ODT_NORMAL:
	case MTK_CAM_SCEN_M2M_NORMAL:
		if (is_m2m_apu(job)) {
			struct mtk_raw_ctrl_data *ctrl;

			ctrl = get_raw_ctrl_data(job);
			if (WARN_ON(!ctrl || !ctrl->valid_apu_info))
				return -1;

			if (ctrl->apu_info.apu_path == APU_FRAME_MODE)
				hard_scenario = MTKCAM_IPI_HW_PATH_OFFLINE_ADL;
			else if (ctrl->apu_info.apu_path == APU_DC_RAW)
				hard_scenario = MTKCAM_IPI_HW_PATH_DC_ADL;
			else {
				pr_info("%s: error. apu_path = %d\n",
					__func__, ctrl->apu_info.apu_path);
				return -1;
			}
		} else if (is_w) {
			hard_scenario = MTKCAM_IPI_HW_PATH_OFFLINE_RGBW;
		} else if (is_vhdr(job) && !is_dcg_sensor_merge(job))
			hard_scenario = MTKCAM_IPI_HW_PATH_OFFLINE_STAGGER;
		else
			hard_scenario = MTKCAM_IPI_HW_PATH_OFFLINE;
		break;
	case MTK_CAM_SCEN_ODT_MSTREAM:
		hard_scenario = MTKCAM_IPI_HW_PATH_OFFLINE_STAGGER;
		break;
	case MTK_CAM_SCEN_SMVR:
		hard_scenario = MTKCAM_IPI_HW_PATH_ON_THE_FLY;
		break;
	case MTK_CAM_SCEN_EXT_ISP:
		hard_scenario = MTKCAM_IPI_HW_PATH_ON_THE_FLY;
		break;
	default:
		pr_info("[%s] failed. un-support scen id:%d",
			__func__, scen->id);
		break;
	}

END:
	return hard_scenario;
}

int get_sw_feature(struct mtk_cam_job *job)
{
	return is_vhdr(job) ?
		MTKCAM_IPI_SW_FEATURE_VHDR : MTKCAM_IPI_SW_FEATURE_NORMAL;
}

static int scen_exp_num(struct mtk_cam_scen *scen)
{
	int exp = 1;

	switch (scen->id) {
	case MTK_CAM_SCEN_NORMAL:
	case MTK_CAM_SCEN_ODT_NORMAL:
	case MTK_CAM_SCEN_M2M_NORMAL:
		if (scen->scen.normal.exp_num == 0)
			pr_info("%s: error: NORMAL SCEN(%d) w/o setting exp_num",
					__func__, scen->id);
		else
			exp = scen->scen.normal.exp_num;
		break;
	case MTK_CAM_SCEN_MSTREAM:
	case MTK_CAM_SCEN_ODT_MSTREAM:
		switch (scen->scen.mstream.type) {
		case MTK_CAM_MSTREAM_NE_SE:
		case MTK_CAM_MSTREAM_SE_NE:
			exp = 2;
			break;
		case MTK_CAM_MSTREAM_1_EXPOSURE:
			exp = 1;
			break;
		default:
			break;
		}
		break;
	case MTK_CAM_SCEN_EXT_ISP:
		exp = 1;
		break;
	case MTK_CAM_SCEN_SMVR:
	default:
		break;
	}

	return exp;
}

int job_prev_exp_num_seamless(struct mtk_cam_job *job)
{
	int prev;

	//NOTE: for legacy issue, it is assumed that
	//in stagger scenario, it will start from the sensor mode of max exp

	if (job->first_job || job->raw_switch)
		prev = scen_max_exp_num(&job->job_scen);
	else
		prev = job_prev_exp_num(job);

	return prev;
}

int job_prev_exp_num(struct mtk_cam_job *job)
{
	struct mtk_cam_scen *scen = &job->prev_scen;

	//NOTE: prev_scen of first job comes from req
	//which is equal to job_scen of first req

	return scen_exp_num(scen);
}

int job_exp_num(struct mtk_cam_job *job)
{
	struct mtk_cam_scen *scen = &job->job_scen;

	return scen_exp_num(scen);
}

int scen_max_exp_num(struct mtk_cam_scen *scen)
{
	int exp = 1;

	switch (scen->id) {
	case MTK_CAM_SCEN_NORMAL:
	case MTK_CAM_SCEN_ODT_NORMAL:
	case MTK_CAM_SCEN_M2M_NORMAL:
		exp = scen->scen.normal.max_exp_num;
		break;
	case MTK_CAM_SCEN_MSTREAM:
	case MTK_CAM_SCEN_ODT_MSTREAM:
		exp = 2;
		break;
	//case MTK_CAM_SCEN_SMVR:
	default:
		break;
	}
	return exp;
}

int get_subsample_ratio(struct mtk_cam_scen *scen)
{
	if (scen->id == MTK_CAM_SCEN_SMVR) {
		int sub_num = scen->scen.smvr.subsample_num;

		if (sub_num > 32 || (sub_num & (sub_num - 1))) {
			pr_info("%s: error. wrong subsample_num %d\n",
				__func__, sub_num);
			return 1;
		}
		return sub_num;
	}
	return 1;
}

#define SENSOR_I2C_TIME_NS		(6 * 1000000ULL)
#define SENSOR_I2C_TIME_NS_60FPS	(6 * 1000000ULL)
#define SENSOR_I2C_TIME_NS_HIGH_FPS	(3 * 1000000ULL)
#define CQ_PROCESSING_TIME_NS	(2 * 1000000ULL) // 2ms

#define INTERVAL_NS(fps)	(1000000000ULL / fps)

static u64 reserved_i2c_time(u64 frame_interval_ns)
{
	u64 i2c_time;

	/* > 60fps */
	if (frame_interval_ns < INTERVAL_NS(60))
		i2c_time = SENSOR_I2C_TIME_NS_HIGH_FPS;
	else if (INTERVAL_NS(60) <= frame_interval_ns &&
		 frame_interval_ns < INTERVAL_NS(30))
		i2c_time = SENSOR_I2C_TIME_NS_60FPS;
	else
		i2c_time = SENSOR_I2C_TIME_NS;
	return i2c_time;
}

u64 infer_i2c_deadline_ns(struct mtk_cam_job *job, u64 frame_interval_ns)
{
	struct mtk_cam_scen *scen = &job->job_scen;

	/* consider vsync is subsampled */
	if (scen->id == MTK_CAM_SCEN_SMVR)
		return frame_interval_ns * (scen->scen.smvr.subsample_num - 1);
	/* temp to frame/2 */
	else if (is_stagger_lbmf(job))
		return frame_interval_ns / 2 - reserved_i2c_time(frame_interval_ns);
	else
		return frame_interval_ns - reserved_i2c_time(frame_interval_ns);
}
u64 infer_cq_trigger_deadline_ns(struct mtk_cam_job *job, u64 frame_interval_ns)
{
	struct mtk_cam_scen *scen = &job->job_scen;

	/* consider vsync is subsampled */
	if (scen->id == MTK_CAM_SCEN_SMVR)
		return frame_interval_ns * (scen->scen.smvr.subsample_num - 1) - CQ_PROCESSING_TIME_NS;
	else
		return frame_interval_ns * 3 / 4 - CQ_PROCESSING_TIME_NS;
}

unsigned int get_master_engines(unsigned int used_engine)
{
	unsigned int master_engine = used_engine & ~bit_map_subset_mask(MAP_HW_RAW);
	int master_raw_id = get_master_raw_id(used_engine);

	if (master_raw_id != -1)
		master_engine |= bit_map_bit(MAP_HW_RAW, master_raw_id);

	return master_engine;
}

unsigned int get_master_raw_id(unsigned int used_engine)
{
	used_engine = bit_map_subset_of(MAP_HW_RAW, used_engine);

	return ffs(used_engine) - 1;
}

unsigned int get_master_sv_id(unsigned int used_engine)
{
	used_engine = bit_map_subset_of(MAP_HW_CAMSV, used_engine);

	return ffs(used_engine) - 1;
}

static int mtk_cam_fill_img_in_buf(struct mtkcam_ipi_img_input *ii,
				    struct mtk_cam_buffer *buf)
{
	struct mtk_cam_cached_image_info *img_info = &buf->image_info;
	dma_addr_t daddr;
	int i;

	ii->buf[0].ccd_fd = buf->vbb.vb2_buf.planes[0].m.fd;

	daddr = buf->daddr;
	for (i = 0; i < ARRAY_SIZE(img_info->bytesperline); i++) {
		unsigned int size = img_info->size[i];

		if (!size)
			break;

		ii->buf[i].iova = daddr;
		ii->buf[i].size = size;
		daddr += size;
	}

	return 0;
}

static int fill_img_in_driver_buf(struct mtkcam_ipi_img_input *ii,
				  struct mtkcam_ipi_uid uid,
				  struct mtk_cam_buf_fmt_desc *fmt_desc,
				  struct mtk_cam_pool_buffer *buf)
{
	int i;

	/* uid */
	ii->uid = uid;

	/* fmt */
	ii->fmt.format = fmt_desc->ipi_fmt;
	ii->fmt.s = (struct mtkcam_ipi_size) {
		.w = fmt_desc->width,
		.h = fmt_desc->height,
	};

	for (i = 0; i < ARRAY_SIZE(ii->fmt.stride); i++)
		ii->fmt.stride[i] = i < ARRAY_SIZE(fmt_desc->stride) ?
			fmt_desc->stride[i] : 0;

	/* buf */
	ii->buf[0].size = fmt_desc->size;
	ii->buf[0].iova = buf->daddr;
	ii->buf[0].ccd_fd = -1;

	buf_printk("%dx%d sz %zu/%d iova %pad\n",
		   fmt_desc->width, fmt_desc->height,
		   fmt_desc->size, buf->size, &buf->daddr);

	return 0;
}

static int fill_img_out_driver_buf(struct mtkcam_ipi_img_output *io,
				  struct mtkcam_ipi_uid uid,
				  struct mtk_cam_buf_fmt_desc *fmt_desc,
				  struct mtk_cam_pool_buffer *buf)
{
	int i;

	/* uid */
	io->uid = uid;

	/* fmt */
	io->fmt.format = fmt_desc->ipi_fmt;
	io->fmt.s = (struct mtkcam_ipi_size) {
		.w = fmt_desc->width,
		.h = fmt_desc->height,
	};

	for (i = 0; i < ARRAY_SIZE(io->fmt.stride); i++)
		io->fmt.stride[i] = i < ARRAY_SIZE(fmt_desc->stride) ?
			fmt_desc->stride[i] : 0;

	/* buf */
	io->buf[0][0].size = fmt_desc->size;
	io->buf[0][0].iova = buf->daddr;
	io->buf[0][0].ccd_fd = -1;

	/* crop */
	io->crop = (struct mtkcam_ipi_crop) {
		.p = (struct mtkcam_ipi_point) {
			.x = 0,
			.y = 0,
		},
		.s = (struct mtkcam_ipi_size) {
			.w = fmt_desc->width,
			.h = fmt_desc->height,
		},
	};

	buf_printk("%dx%d sz %zu/%d iova %pad\n",
		   fmt_desc->width, fmt_desc->height,
		   fmt_desc->size, buf->size, &buf->daddr);

	return 0;
}

static int fill_sv_img_fp_working_buffer(struct req_buffer_helper *helper,
	struct mtk_cam_buf_fmt_desc *fmt_desc,
	struct mtk_cam_pool_buffer *buf, int exp_no, bool is_w)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtk_cam_job *job = helper->job;
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_camsv_device *sv_dev;
	struct mtkcam_ipi_img_output *out;
	struct mtkcam_ipi_uid uid;
	unsigned int tag_idx;
	unsigned int job_exp_no = 0;
	int ret = 0;

	if (ctx->hw_sv == NULL)
		goto EXIT;

	sv_dev = dev_get_drvdata(ctx->hw_sv);

	job_exp_no = job_exp_num(job);
	if (job_exp_no > 3) {
		ret = -1;
		pr_info("%s: over maximum exposure number(exp_no:%d)",
			__func__, job_exp_no);
		goto EXIT;
	}

	tag_idx = (is_dc_mode(job) && job_exp_no > 1 && (exp_no + 1) == job_exp_no) ?
		get_sv_tag_idx_hdr(job_exp_no, MTKCAM_IPI_ORDER_LAST_TAG, is_w) :
		get_sv_tag_idx_hdr(job_exp_no, exp_no, is_w);
	if (tag_idx == -1) {
		ret = -1;
		pr_info("%s: tag_idx not found(exp_no:%d)", __func__, job_exp_no);
		goto EXIT;
	}

	uid.pipe_id = sv_dev->id + MTKCAM_SUBDEV_CAMSV_START;
	uid.id = MTKCAM_IPI_CAMSV_MAIN_OUT;

	fp->camsv_param[0][tag_idx].pipe_id = uid.pipe_id;
	fp->camsv_param[0][tag_idx].tag_id = tag_idx;
	fp->camsv_param[0][tag_idx].hardware_scenario = get_hw_scenario(job);

	out = &fp->camsv_param[0][tag_idx].camsv_img_outputs[0];
	ret = fill_img_out_driver_buf(out, uid, fmt_desc, buf);

EXIT:
	return ret;
}

static const int otf_2exp_rawi[1] = {
	MTKCAM_IPI_RAW_RAWI_2
};
static const int otf_3exp_rawi[2] = {
	MTKCAM_IPI_RAW_RAWI_2, MTKCAM_IPI_RAW_RAWI_3
};
static const int dc_1exp_rawi[1] = {
	MTKCAM_IPI_RAW_RAWI_5
};
static const int dc_2exp_rawi[2] = {
	MTKCAM_IPI_RAW_RAWI_2, MTKCAM_IPI_RAW_RAWI_5
};
static const int dc_3exp_rawi[3] = {
	MTKCAM_IPI_RAW_RAWI_2, MTKCAM_IPI_RAW_RAWI_3, MTKCAM_IPI_RAW_RAWI_5
};

int raw_video_id_w_port(int rawi_id)
{
	switch (rawi_id) {
	case MTKCAM_IPI_RAW_IMGO:
		return MTKCAM_IPI_RAW_IMGO_W;
	case MTKCAM_IPI_RAW_RAWI_2:
		return MTKCAM_IPI_RAW_RAWI_2_W;
	case MTKCAM_IPI_RAW_RAWI_3:
		return MTKCAM_IPI_RAW_RAWI_3_W;
	case MTKCAM_IPI_RAW_RAWI_5:
		return MTKCAM_IPI_RAW_RAWI_5_W;
	default:
		WARN_ON(1);
		return MTKCAM_IPI_RAW_RAWI_2_W;
	}
}

static int fill_sv_to_rawi_wbuf(struct req_buffer_helper *helper,
		__u8 pipe_id, __u8 ipi, int exp_no, bool is_w,
		struct mtk_cam_buf_fmt_desc *fmt_desc,
		struct mtk_cam_pool_buffer *buf)
{
	int ret = 0;
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_input *ii;
	struct mtkcam_ipi_uid uid;

	uid.pipe_id = pipe_id;
	uid.id = ipi;
	ii = &fp->img_ins[helper->ii_idx];
	++helper->ii_idx;

	ret = fill_img_in_driver_buf(ii, uid, fmt_desc, buf);

	if (helper->job->job_type != JOB_TYPE_MSTREAM) {
		/* HS_TODO: dc? */
		ret = ret || fill_sv_img_fp_working_buffer(helper, fmt_desc, buf, exp_no, is_w);
	}

	return ret;
}

void get_stagger_rawi_table(struct mtk_cam_job *job,
		const int **rawi_table, int *cnt)
{
	int exp_num_cur = job_exp_num(job);
	int hw_scen = get_hw_scenario(job);
	bool without_tg = is_dc_mode(job) || is_m2m(job);

	switch (exp_num_cur) {
	case 1:
		(*rawi_table) = without_tg ? dc_1exp_rawi : NULL;
		*cnt = without_tg ? ARRAY_SIZE(dc_1exp_rawi) : 0;
		break;
	case 2:
		if (hw_scen == MTKCAM_IPI_HW_PATH_OTF_STAGGER_LN_INTL) {
			(*rawi_table) = NULL;
			*cnt = 0;
		} else {
			(*rawi_table) = without_tg ? dc_2exp_rawi : otf_2exp_rawi;
			*cnt = without_tg ?
				ARRAY_SIZE(dc_2exp_rawi) : ARRAY_SIZE(otf_2exp_rawi);
		}
		break;
	case 3:
		(*rawi_table) = without_tg ? dc_3exp_rawi : otf_3exp_rawi;
		*cnt = without_tg ?
			ARRAY_SIZE(dc_3exp_rawi) : ARRAY_SIZE(otf_3exp_rawi);
		break;
	default:
		break;
	}
}

static int update_dcif_param_to_ipi_frame(struct mtk_cam_job *job,
					  struct mtkcam_ipi_frame_param *fp,
					  unsigned int frame_size,
					  unsigned int ring_buffer_size)
{
	struct mtkcam_ipi_dcif_ring_param *p = &fp->dcif_param;
	struct mtk_cam_ctx *ctx = job->src_ctx;

	p->ring_mode_en = 1;
	p->ring_start_offset = ctx->ring_start_offset;
	p->dc_path_type = ctx->slb_used_size ? DC_SLB_EMI : DC_DRAM;

	ctx->ring_start_offset =
		(ctx->ring_start_offset + frame_size) % ring_buffer_size;
	return 0;
}

static int update_work_buffer_by_slb(struct req_buffer_helper *helper,
				     const int *rawi_table, int rawi_table_size)
{
	struct mtk_cam_job *job = helper->job;
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_buf_fmt_desc slb_fmt_desc, *fmt_desc;
	struct mtk_cam_pool_buffer img_work_buf;
	int ret;
	int i = 0;

	if (!ctx->slb_size)
		return -1;

	/* support 1exp only now */
	if (rawi_table_size != 1)
		return -1;

	fmt_desc = &ctx->img_work_buf_desc.fmt_desc[MTKCAM_BUF_FMT_TYPE_BAYER];
	slb_fmt_desc = *fmt_desc;

	if (!ctx->slb_used_size) {
		int stride = slb_fmt_desc.stride[0];

		ctx->slb_used_size = (ctx->slb_size / stride) * stride;
		dev_info(ctx->cam->dev, "%s: stride %d slb used %u\n",
			 __func__, stride, ctx->slb_used_size);
	}

	slb_fmt_desc.size = ctx->slb_used_size;

	img_work_buf.daddr = ctx->slb_iova;
	img_work_buf.vaddr = 0;
	img_work_buf.size = ctx->slb_used_size;

	ret = fill_sv_to_rawi_wbuf(helper, get_raw_subdev_idx(ctx->used_pipe),
				   rawi_table[i], i, false,
				   &slb_fmt_desc, &img_work_buf);

	ret = ret || update_dcif_param_to_ipi_frame(job, helper->fp,
						    fmt_desc->size,
						    ctx->slb_used_size);
	return ret;
}

static int
update_work_buffer_by_workbuf(struct req_buffer_helper *helper,
			      const int *rawi_table, int rawi_table_size)
{
	struct mtk_cam_job *job = helper->job;
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_buf_fmt_desc *fmt_desc;
	struct mtk_cam_pool_buffer img_work_buf;
	int ret = 0;
	int i;

	if (!job->img_wbuf_pool_wrapper) {
		pr_info("[%s] fail to fetch, img_wbuf_pool_wrapper is NULL\n", __func__);
		return -ENOMEM;
	}

	fmt_desc = get_fmt_desc(&ctx->img_work_buf_desc);
	for (i = 0 ; i < rawi_table_size; i++) {

		ret = mtk_cam_buffer_pool_fetch(&job->img_wbuf_pool_wrapper->pool,
						&img_work_buf);
		if (ret) {
			pr_info("[%s] fail to fetch\n", __func__);
			return ret;
		}

		ret = fill_sv_to_rawi_wbuf(helper, get_raw_subdev_idx(ctx->used_pipe),
				rawi_table[i], i, false,
				fmt_desc, &img_work_buf);

		mtk_cam_buffer_pool_return(&img_work_buf);

		if (!ret && job->job_scen.scen.normal.w_chn_enabled) {
			ret = mtk_cam_buffer_pool_fetch(&job->img_wbuf_pool_wrapper->pool,
							&img_work_buf);
			if (ret) {
				pr_info("[%s] fail to fetch\n", __func__);
				return ret;
			}

			ret = fill_sv_to_rawi_wbuf(helper, get_raw_subdev_idx(ctx->used_pipe),
					raw_video_id_w_port(rawi_table[i]), i, true,
					fmt_desc, &img_work_buf);

			mtk_cam_buffer_pool_return(&img_work_buf);
		}
	}

	if (unlikely(debug_dram_ring_mode))
		update_dcif_param_to_ipi_frame(job, helper->fp,
				fmt_desc->size + fmt_desc->stride[0] * debug_dram_ring_mode,
				fmt_desc->size);

	return ret;
}

int update_work_buffer_to_ipi_frame(struct req_buffer_helper *helper)
{
	struct mtk_cam_job *job = helper->job;
	const int *rawi_table = NULL;
	int rawi_table_size = 0;

	if (helper->filled_hdr_buffer)
		return 0;

	get_stagger_rawi_table(job, &rawi_table, &rawi_table_size);

	/* no need img working buffer */
	if (!rawi_table || !rawi_table_size)
		return 0;

	if (!update_work_buffer_by_slb(helper, rawi_table, rawi_table_size))
		return 0;

	if (!update_work_buffer_by_workbuf(helper, rawi_table, rawi_table_size))
		return 0;

	return -1;
}

int update_sensor_meta_buffer_to_ipi_frame(struct mtk_cam_job *job,
	struct mtkcam_ipi_frame_param *fp)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_buf_fmt_desc fmt_desc;
	struct mtk_camsv_device *sv_dev;
	struct mtkcam_ipi_img_output *out;
	struct mtkcam_ipi_uid uid;
	unsigned int tag_idx;
	int ret = 0;

	if (!job->is_sensor_meta_dump)
		goto EXIT;

	if (ctx->hw_sv == NULL)
		goto EXIT;

	ret = mtk_cam_buffer_pool_fetch(
			&ctx->sensor_meta_pool, &job->sensor_meta_buf);
	if (ret) {
		pr_info("[%s] fail to fetch\n", __func__);
		return ret;
	}

	fmt_desc = job->seninf_meta_buf_desc.fmt_desc[MTKCAM_BUF_FMT_TYPE_BAYER];

	sv_dev = dev_get_drvdata(ctx->hw_sv);
	tag_idx = SVTAG_SENSOR_META;

	uid.pipe_id = sv_dev->id + MTKCAM_SUBDEV_CAMSV_START;
	uid.id = MTKCAM_IPI_CAMSV_MAIN_OUT;

	fp->camsv_param[0][tag_idx].pipe_id = uid.pipe_id;
	fp->camsv_param[0][tag_idx].tag_id = tag_idx;
	fp->camsv_param[0][tag_idx].hardware_scenario = 0;

	out = &fp->camsv_param[0][tag_idx].camsv_img_outputs[0];
	ret = fill_img_out_driver_buf(out, uid,
			&fmt_desc, &job->sensor_meta_buf);

	mtk_cam_buffer_pool_return(&job->sensor_meta_buf);

EXIT:
	return ret;
}

static int fill_ufbc_header_bayer(void *vaddr,
			struct mtkcam_ipi_img_ufo_param *ufo_param)
{
	struct UFO_META_INFO *header = vaddr;
	struct UFD_META_INFO *ufd_header;

	if (!header) {
		pr_info("[%s] fail to get buf va\n", __func__);
		return -1;
	}

	header->AUWriteBySW = 1;

	ufd_header = &header->UFD.UFD;
	ufd_header->bUF = 1;

	// Note: ufd_bond_mode[0] is 1 only when it is twin/triple mode
	// therefore, pure raw with camsv will always go to else.
	// (mtkcam_ipi_img_ufo_param is reset in reset_img_ufd_io_param().)
	if (ufo_param && ufo_param->ufd_bond_mode[0]) {
		ufd_header->UFD_BITSTREAM_OFST_ADDR[0] = ufo_param->ufd_bitstream_ofst_addr[0];
		ufd_header->UFD_BITSTREAM_OFST_ADDR[1] = ufo_param->ufd_bitstream_ofst_addr[1];
		ufd_header->UFD_BS_AU_START[0] = ufo_param->ufd_bs_au_start[0];
		ufd_header->UFD_BS_AU_START[1] = ufo_param->ufd_bs_au_start[1];
		ufd_header->UFD_AU2_SIZE[0] = ufo_param->ufd_au2_size[0];
		ufd_header->UFD_AU2_SIZE[1] = ufo_param->ufd_au2_size[1];
		ufd_header->UFD_BOND_MODE = ufo_param->ufd_bond_mode[0];
	} else {
		ufd_header->UFD_BITSTREAM_OFST_ADDR[0] = 0;
		ufd_header->UFD_BITSTREAM_OFST_ADDR[1] = 0;
		ufd_header->UFD_BS_AU_START[0] = 0;
		ufd_header->UFD_BS_AU_START[1] = 0;
		ufd_header->UFD_AU2_SIZE[0] = 0;
		ufd_header->UFD_AU2_SIZE[1] = 0;
		ufd_header->UFD_BOND_MODE = 0;
	}

	buf_printk("[%s] vaddr 0x%p ufd_bond_mode %d", __func__,
			   vaddr, ufd_header->UFD_BOND_MODE);

	return 0;
}

static int fill_ufbc_header_yuvo(void *vaddr,
			struct mtkcam_ipi_img_ufo_param *ufo_param)
{
	struct YUFO_META_INFO *yuvo_meta = vaddr;
	struct YUFD_META_INFO *yufd_meta;

	if (!yuvo_meta) {
		pr_info("[%s] fail to get buf va\n", __func__);
		return -1;
	}

	yufd_meta = &yuvo_meta->YUFD.YUFD;

	yuvo_meta->AUWriteBySW = 1;

	yufd_meta->bYUF = 1;
	yufd_meta->YUFD_BITSTREAM_OFST_ADDR[0] = ufo_param->ufd_bitstream_ofst_addr[0];
	yufd_meta->YUFD_BITSTREAM_OFST_ADDR[1] = ufo_param->ufd_bitstream_ofst_addr[1];
	yufd_meta->YUFD_BS_AU_START[0] = ufo_param->ufd_bs_au_start[0];
	yufd_meta->YUFD_BS_AU_START[1] = ufo_param->ufd_bs_au_start[1];
	yufd_meta->YUFD_AU2_SIZE[0] = ufo_param->ufd_au2_size[0];
	yufd_meta->YUFD_AU2_SIZE[1] = ufo_param->ufd_au2_size[1];
	yufd_meta->YUFD_BOND_MODE = ufo_param->ufd_bond_mode[0];

	return 0;
}

int write_ufbc_header_to_buf(struct mtk_cam_ufbc_header *ufbc_header)
{
	int i;

	for (i = 0; i < ufbc_header->used; i++) {
		struct mtk_cam_ufbc_header_entry *param = &ufbc_header->entry[i];

		switch (param->ipi_id) {
		case MTKCAM_IPI_RAW_IMGO:
		case MTKCAM_IPI_RAW_IMGO_W:
		case MTKCAM_IPI_CAMSV_MAIN_OUT:
			fill_ufbc_header_bayer(param->vaddr, param->param);
			break;
		case MTKCAM_IPI_RAW_YUVO_1:
		case MTKCAM_IPI_RAW_YUVO_3:
			fill_ufbc_header_yuvo(param->vaddr, param->param);
			break;
		default:
			pr_info("%s: unknown IPI(%d) to handle ufbc header",
					__func__, param->ipi_id);
			break;
		}
	}

	return 0;
}

static inline
int _add_entry_to_ufbc_header(struct mtk_cam_ufbc_header *ufbc_header,
		int ipi, void *vaddr, struct mtkcam_ipi_img_ufo_param *param)
{
	struct mtk_cam_ufbc_header_entry *hdr = NULL;

	if (!ufbc_header) {
		pr_info("%s: ERROR: NULL job_ufbc_header_info", __func__);
		return -1;
	}

	if (ufbc_header->used >= UBFC_HEADER_PARAM_MAX || ufbc_header->used < 0) {
		pr_info("%s: ERROR: info->used(%d) out of size",
				__func__, ufbc_header->used);
		return -1;
	}

	hdr = &ufbc_header->entry[ufbc_header->used++];

	hdr->ipi_id = ipi;
	hdr->vaddr = vaddr;
	hdr->param = param;

	return 0;
}

int add_ufbc_header_entry(struct req_buffer_helper *helper,
		unsigned int pixelformat, int ipi_video_id,
		struct mtk_cam_buffer *buf, int plane, unsigned int offset)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	void *vaddr;
	int ret = 0;

	vaddr = vb2_plane_vaddr(&buf->vbb.vb2_buf, plane);
	if (!vaddr)
		return -1;

	if (is_raw_ufo(pixelformat)) {
		switch (ipi_video_id) {
		case MTKCAM_IPI_RAW_IMGO:
		case MTKCAM_IPI_RAW_IMGO_W:
			ret = _add_entry_to_ufbc_header(helper->ufbc_header, ipi_video_id,
							vaddr + offset,
							&fp->img_ufdo_params.imgo);
			break;
		case MTKCAM_IPI_CAMSV_MAIN_OUT:
			ret = _add_entry_to_ufbc_header(helper->ufbc_header, ipi_video_id,
							vaddr + offset,
							NULL);
			break;
		default:
			break;
		}
	} else if (is_yuv_ufo(pixelformat)) {
		switch (ipi_video_id) {
		case MTKCAM_IPI_RAW_YUVO_1:
			ret = _add_entry_to_ufbc_header(helper->ufbc_header, ipi_video_id,
							vaddr + offset,
							&fp->img_ufdo_params.yuvo1);
			break;
		case MTKCAM_IPI_RAW_YUVO_3:
			ret = _add_entry_to_ufbc_header(helper->ufbc_header, ipi_video_id,
							vaddr + offset,
							&fp->img_ufdo_params.yuvo3);
			break;
		default:
			break;
		}
	}

	return ret;
}



static int fill_img_fmt(struct mtkcam_ipi_pix_fmt *ipi_pfmt,
			struct mtk_cam_buffer *buf)
{
	struct mtk_cam_cached_image_info *info = &buf->image_info;
	int i;

	ipi_pfmt->format = mtk_cam_get_img_fmt(info->v4l2_pixelformat);
	ipi_pfmt->s = (struct mtkcam_ipi_size) {
		.w = info->width,
		.h = info->height,
	};

	for (i = 0; i < ARRAY_SIZE(ipi_pfmt->stride); i++)
		ipi_pfmt->stride[i] = i < ARRAY_SIZE(info->bytesperline) ?
			info->bytesperline[i] : 0;
	return 0;
}

int fill_mp_img_in_hdr(struct mtkcam_ipi_img_input *ii,
		       struct mtk_cam_buffer *buf,
		       struct mtk_cam_video_device *node, int id,
		       unsigned int plane,
		       unsigned int plane_per_exp,
		       unsigned int plane_buf_offset)
{
	unsigned int size = buf->image_info.size[0];
	unsigned int valid_plane = 0;
	dma_addr_t daddr = 0, buf_offset = 0;
	bool is_valid_mp_buf;

	/* uid */
	ii->uid.pipe_id = node->uid.pipe_id;
	ii->uid.id = id;

	/* fmt */
	fill_img_fmt(&ii->fmt, buf);

	/* addr */
	is_valid_mp_buf = mtk_cam_buf_is_valid_mp(buf);

	valid_plane = is_valid_mp_buf ? plane : 0;
	buf_offset = (dma_addr_t)(size) *
		get_buf_offset_idx(plane, plane_per_exp, plane_buf_offset,
				   is_valid_mp_buf);

	daddr = mtk_cam_buf_is_mp(buf) ? buf->mdaddr[valid_plane] : buf->daddr;

	/* FIXME: porting workaround */
	ii->buf[0].size = size;
	ii->buf[0].iova = daddr + buf_offset;
	ii->buf[0].ccd_fd = buf->vbb.vb2_buf.planes[valid_plane].m.fd;

	buf_printk("id:%d buf->daddr:0x%llx, ii->buf[0].iova:0x%llx, size:%d",
		   id, buf->daddr, ii->buf[0].iova, ii->buf[0].size);

	return 0;
}

static int get_exp_order_ipi_normal(struct mtk_cam_scen_normal *n)
{
	int exp_order;

	// n->exp_order is undefined when 1 exp
	if (n->exp_num <= 1)
		exp_order = MTKCAM_IPI_ORDER_NE_SE;
	else {
		switch (n->exp_order) {
		case MTK_CAM_EXP_SE_ME_LE:
			pr_info("%s: ERROR: MTK_CAM_EXP_SE_ME_LE not support",
					__func__);
			exp_order = MTKCAM_IPI_ORDER_NE_ME_SE;
			break;
		case MTK_CAM_EXP_LE_ME_SE:
			exp_order = MTKCAM_IPI_ORDER_NE_ME_SE;
			break;
		case MTK_CAM_EXP_SE_LE:
			exp_order = MTKCAM_IPI_ORDER_SE_NE;
			break;
		case MTK_CAM_EXP_LE_SE:
		default:
			exp_order = MTKCAM_IPI_ORDER_NE_SE;
			break;
		}
	}

	return exp_order;
}

static int get_exp_order_ipi_mstream(struct mtk_cam_scen_mstream *m)
{
	int exp_order;

	switch (m->type) {
	case MTK_CAM_MSTREAM_1_EXPOSURE:
		exp_order = MTKCAM_IPI_ORDER_NE_SE;
		break;
	case MTK_CAM_MSTREAM_NE_SE:
		exp_order = MTKCAM_IPI_ORDER_NE_SE;
		break;
	case MTK_CAM_MSTREAM_SE_NE:
		exp_order = MTKCAM_IPI_ORDER_SE_NE;
		break;
	default:
		pr_info("%s: warn. unknown type %d\n", __func__, m->type);
		exp_order = MTKCAM_IPI_ORDER_NE_SE;
		break;
	}

	return exp_order;
}

int get_exp_order(struct mtk_cam_scen *scen)
{
	int exp_order;

	switch (scen->id) {
	case MTK_CAM_SCEN_NORMAL:
	case MTK_CAM_SCEN_ODT_NORMAL:
	case MTK_CAM_SCEN_M2M_NORMAL:
		exp_order = get_exp_order_ipi_normal(&scen->scen.normal);
		break;
	case MTK_CAM_SCEN_MSTREAM:
	case MTK_CAM_SCEN_ODT_MSTREAM:
		exp_order = get_exp_order_ipi_mstream(&scen->scen.mstream);
		break;
	default:
		exp_order = MTKCAM_IPI_ORDER_NE_SE;
		break;
	}

	return exp_order;
}

static int ne_se_offset_in_buf[2] = {0, 1};
static int se_ne_offset_in_buf[2] = {1, 0};
static int ne_me_se_offset_in_buf[3] = {0, 1, 2};

int get_buf_plane(int exp_order_ipi, int exp_seq_num)
{
	int *idx_off_tbl = NULL, tbl_cnt = -1;

	switch (exp_order_ipi) {
	case MTKCAM_IPI_ORDER_NE_SE:
		idx_off_tbl = ne_se_offset_in_buf;
		tbl_cnt = ARRAY_SIZE(ne_se_offset_in_buf);
		break;
	case MTKCAM_IPI_ORDER_SE_NE:
		idx_off_tbl = se_ne_offset_in_buf;
		tbl_cnt = ARRAY_SIZE(se_ne_offset_in_buf);
		break;
	case MTKCAM_IPI_ORDER_NE_ME_SE:
		idx_off_tbl = ne_me_se_offset_in_buf;
		tbl_cnt = ARRAY_SIZE(ne_me_se_offset_in_buf);
		break;
	}

	if (exp_seq_num >= tbl_cnt || !idx_off_tbl) {
		pr_info("%s: idx(%d) is out of table size(%d)",
			__func__, exp_seq_num, tbl_cnt);
		return 0;
	}

	return idx_off_tbl[exp_seq_num];
}

int get_plane_per_exp(bool is_rgbw)
{
	return (is_rgbw) ? 2 : 1;
}

int get_plane_buf_offset(bool w_path)
{
	return (w_path) ? 1 : 0;
}

int get_buf_offset_idx(int plane, int plane_per_exp, int plane_buf_offset,
		       bool is_valid_mp_buf)
{
	int idx = 0;

	/* multi fd for (b + w) and shift w-channel only */
	/* plane 0 fd only, b1 + w1 + b2 + w2 + ... */
	if (is_valid_mp_buf)
		idx = plane_buf_offset;
	else
		idx = (plane * plane_per_exp) + plane_buf_offset;

	if (CAM_DEBUG_ENABLED(JOB))
		pr_info("%s: plane(%d)/plane_per_exp(%d)/plane_buf_offset(%d)/is_valid_mp_buf(%d) => idx(%d)",
			__func__, plane, plane_per_exp, plane_buf_offset,
			is_valid_mp_buf, idx);

	return idx;
}

static int get_exp_support(u32 raw_dev)
{
	int r = 0;
	int min_exp = INT_MAX;

	while (raw_dev) {
		if (raw_dev & 1 << r)
			min_exp =
				min(min_exp, CALL_PLAT_HW(query_max_exp_support, r));

		raw_dev &=~(1 << r);
		++r;
	}

	return (min_exp == INT_MAX) ? 0 : min_exp;
}

static u16 get_raw_dev(struct mtkcam_ipi_config_param *ipi_cfg)
{
	int i = 0;
	u16 raw_dev = 0;

	for (i = 0; i < ipi_cfg->n_maps; i++) {
		if (is_raw_subdev(ipi_cfg->maps[i].pipe_id)) {
			raw_dev = ipi_cfg->maps[i].dev_mask;
			break;
		}
	}

	return raw_dev;
}

int fill_img_in_by_exposure(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	int ret = 0;
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_input *in;
	struct mtk_cam_job *job = helper->job;
	bool is_w = is_rgbw(job) ? true : false;//for coverity...
	const int *rawi_table = NULL;
	int i = 0, e = 0, rawi_cnt = 0;
	int exp_order = get_exp_order(&job->job_scen);
	int job_exp = job_exp_num(job);
	int exp_support = get_exp_support(get_raw_dev(&job->src_ctx->ipi_config));
	bool first_last_exp_only = (exp_support) ? (job_exp > exp_support) : false;

	// the order rawi is in exposure sequence
	get_stagger_rawi_table(job, &rawi_table, &rawi_cnt);
	for (i = 0, e = job_exp; i < rawi_cnt; i++, e--) {
		if (first_last_exp_only && (e > 1) && (e < job_exp))
			continue;

		in = &fp->img_ins[helper->ii_idx++];

		/* handle mdaddr */
		ret = fill_mp_img_in_hdr(in, buf, node, rawi_table[i],
					 get_buf_plane(exp_order, i),
					 get_plane_per_exp(is_w),
					 get_plane_buf_offset(false));

		if (!ret && is_w) {
			in = &fp->img_ins[helper->ii_idx++];
			ret = fill_mp_img_in_hdr(in, buf, node,
						 raw_video_id_w_port(rawi_table[i]),
						 get_buf_plane(exp_order, i),
						 get_plane_per_exp(is_w),
						 get_plane_buf_offset(true));
		}
	}

	return ret;
}

int fill_m2m_rawi_to_img_in_ipi(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	int ret = 0;
	struct mtk_cam_job *job = helper->job;
	bool is_apu;

#ifdef RUN_ADL_FRAME_MODE_FROM_RAWI
	is_apu = is_m2m_apu_dc(job);
#else
	is_apu = is_m2m_apu(job);
#endif
	if (is_apu) {
		struct mtkcam_ipi_frame_param *fp = helper->fp;
		struct mtkcam_ipi_img_input *in;

		in = &fp->img_ins[helper->ii_idx++];

		ret = fill_img_in(in, buf, node, MTKCAM_IPI_RAW_IPUI);
	} else
		ret = fill_img_in_by_exposure(helper, buf, node);

	return ret;
}

struct mtkcam_ipi_crop
v4l2_rect_to_ipi_crop(const struct v4l2_rect *r)
{
	return (struct mtkcam_ipi_crop) {
		.p = (struct mtkcam_ipi_point) {
			.x = r->left,
			.y = r->top,
		},
		.s = (struct mtkcam_ipi_size) {
			.w = r->width,
			.h = r->height,
		},
	};
}

bool ipi_crop_eq(const struct mtkcam_ipi_crop *s,
				 const struct mtkcam_ipi_crop *d)
{
	return ((s->p.x == d->p.x) && (s->p.y == d->p.y) &&
		(s->s.w == d->s.w) && (s->s.h == d->s.h));
}

int fill_imgo_out_subsample(struct req_buffer_helper *helper,
			struct mtkcam_ipi_img_output *io,
			struct mtk_cam_buffer *buf,
			struct mtk_cam_video_device *node,
			int subsample_ratio)
{
	dma_addr_t daddr;
	int i;

	/* uid */
	io->uid = node->uid;

	/* fmt */
	fill_img_fmt(&io->fmt, buf);

	add_ufbc_header_entry(helper, buf->image_info.v4l2_pixelformat,
					  io->uid.id, buf, 0, 0);

	/* addr, 1-plane OR N-plane has same layout */
	for (i = 0; i < subsample_ratio; i++) {
		/* FIXME: porting workaround */
		io->buf[i][0].size = buf->image_info.size[0];
		io->buf[i][0].ccd_fd = buf->vbb.vb2_buf.planes[0].m.fd;

		daddr = mtk_cam_buf_is_mp(buf) ? buf->mdaddr[0] : buf->daddr;
		io->buf[i][0].iova = daddr + i * (dma_addr_t) io->buf[i][0].size;

		buf_printk("i=%d: buf->daddr:0x%llx, io->buf[i][0].iova:0x%llx, size:%d",
			   i, daddr, io->buf[i][0].iova, io->buf[i][0].size);
	}

	/* crop */
	io->crop = v4l2_rect_to_ipi_crop(&buf->image_info.crop);

	buf_printk("%s %dx%d @%d,%d-%dx%d\n",
		   node->desc.name,
		   io->fmt.s.w, io->fmt.s.h,
		   io->crop.p.x, io->crop.p.y, io->crop.s.w, io->crop.s.h);

	return 0;
}

int fill_mp_img_out_hdr(struct req_buffer_helper *helper,
			struct mtkcam_ipi_img_output *io,
			struct mtk_cam_buffer *buf,
			struct mtk_cam_video_device *node, int id,
			unsigned int plane,
			unsigned int plane_per_exp,
			unsigned int plane_buf_offset)
{
	unsigned int size = buf->image_info.size[0];
	unsigned int valid_plane = 0;
	dma_addr_t daddr = 0, buf_offset = 0;
	bool is_valid_mp_buf;

	/* uid */
	io->uid.pipe_id = node->uid.pipe_id;
	io->uid.id = id;

	/* fmt */
	fill_img_fmt(&io->fmt, buf);

	/* addr */
	is_valid_mp_buf = mtk_cam_buf_is_valid_mp(buf);

	valid_plane = is_valid_mp_buf ? plane : 0;
	buf_offset = (dma_addr_t)(size) *
		get_buf_offset_idx(plane, plane_per_exp, plane_buf_offset,
				   is_valid_mp_buf);

	daddr = mtk_cam_buf_is_mp(buf) ? buf->mdaddr[valid_plane] : buf->daddr;

	add_ufbc_header_entry(helper, buf->image_info.v4l2_pixelformat,
						  id, buf, valid_plane, buf_offset);

	/* FIXME: porting workaround */
	io->buf[0][0].size = size;
	io->buf[0][0].iova = daddr + buf_offset;
	io->buf[0][0].ccd_fd = buf->vbb.vb2_buf.planes[valid_plane].m.fd;

	/* crop */
	io->crop = v4l2_rect_to_ipi_crop(&buf->image_info.crop);

	buf_printk("daddr:0x%llx, io->buf[0][0].iova:0x%llx, size:%d",
		   daddr, io->buf[0][0].iova, io->buf[0][0].size);
	buf_printk("%s %dx%d @%d,%d-%dx%d\n",
		   node->desc.name,
		   io->fmt.s.w, io->fmt.s.h,
		   io->crop.p.x, io->crop.p.y, io->crop.s.w, io->crop.s.h);

	return 0;
}

static int mtk_cam_fill_img_out_buf_subsample(struct mtkcam_ipi_img_output *io,
					      struct mtk_cam_buffer *buf,
					      int sub_ratio)
{
	struct mtk_cam_cached_image_info *img_info = &buf->image_info;
	dma_addr_t daddr;
	int i;
	int j;

	io->buf[0][0].ccd_fd = buf->vbb.vb2_buf.planes[0].m.fd;

	daddr = buf->daddr;
	for (j = 0; j < sub_ratio; j++) {
		for (i = 0; i < ARRAY_SIZE(img_info->bytesperline); i++) {
			unsigned int size = img_info->size[i];

			if (!size)
				break;

			io->buf[j][i].iova = daddr;
			io->buf[j][i].size = size;
			io->buf[j][i].ccd_fd = buf->vbb.vb2_buf.planes[0].m.fd;
			daddr += size;
#ifdef DEBUG_SUBSAMPLE_INFO
			buf_printk("sub/plane:%d/%d (iova,size):(0x%x/0x%x)\n",
				j, i,
				io->buf[j][i].iova, io->buf[j][i].size);
#endif
		}
	}

	return 0;
}
int fill_yuvo_out_subsample(struct req_buffer_helper *helper,
			struct mtkcam_ipi_img_output *io,
			struct mtk_cam_buffer *buf,
			struct mtk_cam_video_device *node,
			int sub_ratio)
{
	/* uid */
	io->uid = node->uid;

	/* fmt */
	fill_img_fmt(&io->fmt, buf);

	add_ufbc_header_entry(helper, buf->image_info.v4l2_pixelformat,
						  io->uid.id, buf, 0, 0);

	mtk_cam_fill_img_out_buf_subsample(io, buf, sub_ratio);

	/* crop */
	io->crop = v4l2_rect_to_ipi_crop(&buf->image_info.crop);

	buf_printk("%s %dx%d @%d,%d-%dx%d\n",
		   node->desc.name,
		   io->fmt.s.w, io->fmt.s.h,
		   io->crop.p.x, io->crop.p.y, io->crop.s.w, io->crop.s.h);
	return 0;
}

int fill_img_in(struct mtkcam_ipi_img_input *ii,
		struct mtk_cam_buffer *buf,
		struct mtk_cam_video_device *node,
		int id_overwite)
{
	/* uid */
	ii->uid = node->uid;

	if (id_overwite >= 0)
		ii->uid.id = (u8)id_overwite;

	/* fmt */
	fill_img_fmt(&ii->fmt, buf);

	mtk_cam_fill_img_in_buf(ii, buf);

	buf_printk("%s %dx%d id_overwrite=%d\n",
		   node->desc.name,
		   ii->fmt.s.w, ii->fmt.s.h,
		   id_overwite);
	return 0;
}

static int _fill_mp_img_out(struct req_buffer_helper *helper,
			    struct mtkcam_ipi_img_output *io,
			    struct mtk_cam_buffer *buf,
			    struct mtk_cam_video_device *node,
			    int ipi_video_id,
			    unsigned int plane, unsigned int offset)
{
	struct mtk_cam_cached_image_info *img_info = &buf->image_info;
	dma_addr_t daddr;
	unsigned int valid_plane = 0;
	int i;

	/* check param validity */

	/* uid */
	io->uid = node->uid;
	io->uid.id = ipi_video_id;

	/* fmt */
	fill_img_fmt(&io->fmt, buf);

	/* addr */
	if (mtk_cam_buf_is_mp(buf)) {
		valid_plane = mtk_cam_buf_is_valid_mp(buf) ? plane : 0;
		daddr = buf->mdaddr[valid_plane];
	} else {
		daddr = buf->daddr;
	}

	add_ufbc_header_entry(helper, img_info->v4l2_pixelformat, ipi_video_id,
						  buf, valid_plane, offset);

	/* pixel format information, yuv has multi plane img info */
	for (i = 0; i < ARRAY_SIZE(img_info->bytesperline); i++) {
		unsigned int size = img_info->size[i];

		if (!size)
			break;

		io->buf[0][i].size = size;
		io->buf[0][i].iova = daddr + (dma_addr_t)(offset);
		io->buf[0][i].ccd_fd = buf->vbb.vb2_buf.planes[valid_plane].m.fd;

		daddr += (offset + size);
	}

	/* crop */
	io->crop = v4l2_rect_to_ipi_crop(&buf->image_info.crop);

	buf_printk("%s plane %d/%d, offset %u, iova %llx/%llx/%llx, size %u/%u/%u\n",
		   node->desc.name, plane, valid_plane, offset,
		   io->buf[0][0].iova, io->buf[0][1].iova, io->buf[0][2].iova,
		   io->buf[0][0].size, io->buf[0][1].size, io->buf[0][2].size);
	buf_printk("%s %dx%d @%d,%d-%dx%d\n",
		   node->desc.name,
		   io->fmt.s.w, io->fmt.s.h,
		   io->crop.p.x, io->crop.p.y, io->crop.s.w, io->crop.s.h);

	return 0;
}

int fill_img_out(struct req_buffer_helper *helper,
		 struct mtkcam_ipi_img_output *io,
		 struct mtk_cam_buffer *buf,
		 struct mtk_cam_video_device *node)
{
	return  _fill_mp_img_out(helper, io, buf, node, node->uid.id, 0, 0);
}

int fill_img_out_w(struct req_buffer_helper *helper,
		   struct mtkcam_ipi_img_output *io,
		   struct mtk_cam_buffer *buf,
		   struct mtk_cam_video_device *node)
{
	return _fill_mp_img_out(helper, io, buf, node,
				raw_video_id_w_port(node->uid.id), 0, buf->image_info.size[0]);
}

static int fill_sv_mp_fp(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node, unsigned int tag_idx,
	unsigned int pipe_id, unsigned int plane,
	unsigned int plane_per_exp, unsigned int plane_buf_offset)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_output *out =
		&fp->camsv_param[0][tag_idx].camsv_img_outputs[0];
	dma_addr_t buf_offset = 0;
	unsigned int size = buf->image_info.size[0];
	int valid_plane = 0, ret = -1;
	bool is_valid_mp_buf;

	is_valid_mp_buf = mtk_cam_buf_is_valid_mp(buf);

	valid_plane = is_valid_mp_buf ? plane : 0;
	buf_offset = (dma_addr_t)(size) *
		get_buf_offset_idx(plane, plane_per_exp, plane_buf_offset,
				   is_valid_mp_buf);

	ret = _fill_mp_img_out(helper, out, buf, node,
			MTKCAM_IPI_CAMSV_MAIN_OUT, valid_plane, buf_offset);
	out->uid.pipe_id = pipe_id;

	fp->camsv_param[0][tag_idx].pipe_id = pipe_id;
	fp->camsv_param[0][tag_idx].tag_id = tag_idx;
	fp->camsv_param[0][tag_idx].hardware_scenario = 0;

	buf_printk("%s: tag_idx %d, iova %llx, size %u",
		   __func__, tag_idx, out->buf[0][0].iova, out->buf[0][0].size);

	return ret;
}

static bool
is_stagger_2_exposure(struct mtk_cam_scen *scen)
{
	return (scen->id == MTK_CAM_SCEN_NORMAL &&
		scen->scen.normal.exp_num == 2);
}

static bool
is_stagger_3_exposure(struct mtk_cam_scen *scen)
{
	return (scen->id == MTK_CAM_SCEN_NORMAL &&
		scen->scen.normal.exp_num == 3);
}

static bool
is_mstream_2_exposure(struct mtk_cam_scen *scen)
{
	return (scen->id == MTK_CAM_SCEN_MSTREAM &&
		(scen->scen.mstream.type == MTK_CAM_MSTREAM_NE_SE ||
		scen->scen.mstream.type == MTK_CAM_MSTREAM_SE_NE));
}

int fill_sv_img_fp(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	struct mtk_cam_job *job = helper->job;
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_scen *scen = &job->job_scen;
	struct mtk_camsv_device *sv_dev;
	unsigned int pipe_id, exp_no, buf_cnt = 0;
	int exp_order = get_exp_order(&job->job_scen);
	int tag_idx, i, j, ret = 0;
	bool is_w, is_mstream = false;

	if (!is_pure_raw_node(job, node))
		goto EXIT;

	if (ctx->hw_sv == NULL)
		goto EXIT;

	sv_dev = dev_get_drvdata(ctx->hw_sv);
	pipe_id = sv_dev->id + MTKCAM_SUBDEV_CAMSV_START;

	if (is_stagger_2_exposure(scen)) {
		exp_no = 2;
		buf_cnt = is_rgbw(job) ? 2 : 1;
	} else if (is_stagger_3_exposure(scen)) {
		exp_no = 3;
		buf_cnt = 1;
		if (is_rgbw(job)) {
			ret = -1;
			pr_info("%s: rgbw not supported under 3-exp stagger case",
				__func__);
			goto EXIT;
		}
	} else if (is_mstream_2_exposure(scen)) {
		exp_no = 2;
		buf_cnt = 1;
		is_mstream = true;
	} else {
		exp_no = 1;
		buf_cnt = is_rgbw(job) ? 2 : 1;
	}

	for (i = 0; i < exp_no; i++) {
		if (!is_sv_pure_raw(job) &&
			!is_dc_mode(job) &&
			(i + 1) == exp_no)
			continue;
		/* skip first exp under mstream case */
		if (is_mstream &&
			exp_no == 2 &&
			i == 0)
			continue;
		for (j = 0; j < buf_cnt; j++) {
			is_w = (j % 2) ? true : false;
			tag_idx = (exp_no > 1 && (i + 1) == exp_no) ?
				get_sv_tag_idx(exp_no, MTKCAM_IPI_ORDER_LAST_TAG, is_w) :
				get_sv_tag_idx(exp_no, i, is_w);
			if (tag_idx == -1) {
				ret = -1;
				pr_info("%s: tag_idx not found(exp_no:%d is_w:%d)",
					__func__, exp_no, (is_w) ? 1 : 0);
				goto EXIT;
			}
			ret = fill_sv_mp_fp(helper, buf, node, tag_idx, pipe_id,
					    get_buf_plane(exp_order, i),
					    get_plane_per_exp((buf_cnt == 2)),
					    get_plane_buf_offset(is_w));
		}
	}

EXIT:
	return ret;
}

int fill_imgo_buf_as_working_buf(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_output *out;
	struct mtk_cam_job *job = helper->job;
	bool is_w = is_rgbw(job);
	bool is_otf = !is_dc_mode(job);
	int ii_inc = 0, plane_inc;
	int ret = 0;
	bool sv_pure_raw;

	if (!is_pure_raw_node(job, node)) {
		pr_info("%s: expect PURE-RAW node only", __func__);
		WARN_ON(1);
		return -1;
	}

	helper->filled_hdr_buffer = true;

	sv_pure_raw = is_sv_pure_raw(job);

	ii_inc = helper->ii_idx;
	fill_img_in_by_exposure(helper, buf, node);  /* handle mdaddr */
	ii_inc = helper->ii_idx - ii_inc;
	plane_inc = ii_inc / get_plane_per_exp(is_w);

	if (is_otf && !sv_pure_raw) {
		// OTF, raw outputs last exp
		out = &fp->img_outs[helper->io_idx++];
		ret = fill_mp_img_out_hdr(helper, out, buf, node, MTKCAM_IPI_RAW_IMGO,
					  (is_w ? 2 : 1), plane_inc, 0);

		if (!ret && is_w) {
			out = &fp->img_outs[helper->io_idx++];
			ret = fill_mp_img_out_hdr(helper, out, buf, node,
						  raw_video_id_w_port(MTKCAM_IPI_RAW_IMGO),
						  2, plane_inc, 1);
		}
	}

	if (sv_pure_raw && CAM_DEBUG_ENABLED(JOB))
		pr_info("%s:req:%s bypass pure raw node\n",
			__func__, job->req->debug_str);
	/* fill sv image fp */
	ret = ret || fill_sv_img_fp(helper, buf, node);

	return ret;
}

int get_sv_tag_idx(unsigned int exp_no, unsigned int tag_order, bool is_w)
{
	struct mtk_camsv_tag_param img_tag_param[SVTAG_IMG_END];
	unsigned int hw_scen, req_amount;
	int i, tag_idx = -1;

	hw_scen = 1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER);
	req_amount = (exp_no < 3) ? exp_no * 2 : exp_no;
	if (mtk_cam_sv_get_tag_param(img_tag_param, hw_scen, exp_no, req_amount))
		goto EXIT;
	else {
		for (i = 0; i < req_amount; i++) {
			if (img_tag_param[i].tag_order == tag_order &&
				img_tag_param[i].is_w == is_w) {
				tag_idx = img_tag_param[i].tag_idx;
				break;
			}
		}
	}

EXIT:
	return tag_idx;
}

bool is_sv_pure_raw(struct mtk_cam_job *job)
{
	if (!job)
		return false;

	return job->is_sv_pure_raw && sv_pure_raw;
}

bool is_vhdr(struct mtk_cam_job *job)
{
	return scen_is_vhdr(&job->job_scen);
}

bool is_dc_mode(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_v2 *res;

	res = _get_job_res(job);
	if (!res)
		return false;

	return res_raw_is_dc_mode(&res->raw_res);
}

bool is_rgbw(struct mtk_cam_job *job)
{
	return scen_is_rgbw(&job->job_scen);
}

bool is_extisp(struct mtk_cam_job *job)
{
	return scen_is_extisp(&job->job_scen);
}

bool is_dcg_sensor_merge(struct mtk_cam_job *job)
{
	return scen_is_dcg_sensor_merge(&job->job_scen);
}

bool is_dcg_ap_merge(struct mtk_cam_job *job)
{
	return scen_is_dcg_ap_merge(&job->job_scen);
}

bool is_m2m(struct mtk_cam_job *job)
{
	return scen_is_m2m(&job->job_scen);
}

bool is_m2m_apu(struct mtk_cam_job *job)
{
	struct mtk_raw_ctrl_data *ctrl;

	ctrl = get_raw_ctrl_data(job);
	if (!ctrl || !ctrl->valid_apu_info)
		return 0;

	return scen_is_m2m_apu(&job->job_scen, &ctrl->apu_info);
}

bool is_m2m_apu_dc(struct mtk_cam_job *job)
{
	struct mtk_raw_ctrl_data *ctrl;

	ctrl = get_raw_ctrl_data(job);
	if (!ctrl || !ctrl->valid_apu_info)
		return 0;

	return scen_is_m2m_apu(&job->job_scen, &ctrl->apu_info)
		&& apu_info_is_dc(&ctrl->apu_info);
}

bool is_stagger_lbmf(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;

	if (ctx->has_raw_subdev)
		return scen_is_stagger_lbmf(&job->job_scen);
	else {
		struct mtk_cam_seninf_sentest_param *param;

		param = mtk_cam_get_sentest_param(ctx);

		return (param && param->is_lbmf) ? true : false;
	}
}

bool is_camsv_16p(struct mtk_cam_job *job)
{
	struct mtk_cam_resource_v2 *res;

	res = _get_job_res(job);
	if (!res)
		return false;

	return res->raw_res.camsv_pixel_mode == 16;
}

int map_ipi_vpu_point(int vpu_point)
{
	switch (vpu_point) {
	case AFTER_SEP_R1: return MTKCAM_IPI_ADL_AFTER_SEP_R1;
	case AFTER_BPC: return MTKCAM_IPI_ADL_AFTER_BPC;
	case AFTER_LTM: return MTKCAM_IPI_ADL_AFTER_LTM;
	default:
		pr_info("%s: error. not supported point %d\n",
			__func__, vpu_point);
		break;
	}
	return -1;
}

int map_ipi_imgo_path(int v4l2_raw_path)
{
	switch (v4l2_raw_path) {
	case V4L2_MTK_CAM_RAW_PATH_SELECT_BPC: return MTKCAM_IPI_IMGO_AFTER_BPC;
	case V4L2_MTK_CAM_RAW_PATH_SELECT_FUS: return MTKCAM_IPI_IMGO_AFTER_FUS;
	case V4L2_MTK_CAM_RAW_PATH_SELECT_DGN: return MTKCAM_IPI_IMGO_AFTER_DGN;
	case V4L2_MTK_CAM_RAW_PATH_SELECT_LSC: return MTKCAM_IPI_IMGO_AFTER_LSC;
	case V4L2_MTK_CAM_RAW_PATH_SELECT_LTM: return MTKCAM_IPI_IMGO_AFTER_LTM;
	default:
		break;
	}
	/* un-processed raw frame */
	return MTKCAM_IPI_IMGO_UNPROCESSED;
}

bool find_video_node(struct mtk_cam_job *job, int node_id)
{
	struct mtk_cam_video_device *node;
	struct mtk_cam_request *req = job->req;
	struct mtk_cam_buffer *buf;

	list_for_each_entry(buf, &req->buf_list, list) {
		node = mtk_cam_buf_to_vdev(buf);

		if (node->desc.id == node_id &&
		    belong_to_current_ctx(job, node->uid.pipe_id)) {
			return true;
		}
	}

	return false;
}

bool is_pure_raw_node(struct mtk_cam_job *job,
		      struct mtk_cam_video_device *node)
{
	(void) job;
	return node->desc.id == MTK_RAW_PURE_RAW_OUT;
}

bool is_processed_raw_node(struct mtk_cam_job *job,
			    struct mtk_cam_video_device *node)
{
#ifdef PURERAW_DEVICE
	struct mtk_raw_ctrl_data *ctrl = get_raw_ctrl_data(job);

	if (ctrl &&
	    map_ipi_imgo_path(ctrl->raw_path) == MTKCAM_IPI_IMGO_UNPROCESSED)
		pr_info("%s unexpected raw path", __func__);
#else
	(void) job;
#endif

	return node->desc.id == MTK_RAW_MAIN_STREAM_OUT;
}

struct mtk_raw_ctrl_data *get_raw_ctrl_data(struct mtk_cam_job *job)
{
	struct mtk_cam_request *req = job->req;
	int raw_pipe_idx;

	raw_pipe_idx = get_raw_subdev_idx(job->src_ctx->used_pipe);
	if (raw_pipe_idx < 0)
		return NULL;

	return &req->raw_data[raw_pipe_idx].ctrl;
}

struct mtk_raw_sink_data *get_raw_sink_data(struct mtk_cam_job *job)
{
	struct mtk_cam_request *req = job->req;
	int raw_pipe_idx;

	raw_pipe_idx = get_raw_subdev_idx(job->src_ctx->used_pipe);
	if (raw_pipe_idx < 0)
		return NULL;

	return &req->raw_data[raw_pipe_idx].sink;
}

struct mtk_camsv_sink_data *get_sv_sink_data(struct mtk_cam_job *job)
{
	struct mtk_cam_request *req = job->req;
	int sv_pipe_idx;

	sv_pipe_idx = get_sv_subdev_idx(job->src_ctx->used_pipe);
	if (sv_pipe_idx < 0)
		return NULL;

	return &req->sv_data[sv_pipe_idx].sink;
}

bool has_valid_mstream_exp(struct mtk_cam_job *job)
{
	struct mtk_raw_ctrl_data *ctrl;

	ctrl = get_raw_ctrl_data(job);
	if (!ctrl)
		return false;

	return ctrl->valid_mstream_exp;
}

void mtk_cam_sv_reset_tag_info(struct mtk_cam_job *job)
{
	struct mtk_camsv_tag_info *tag_info;
	int i;

	job->used_tag_cnt = 0;
	job->enabled_tags = 0;
	for (i = SVTAG_START; i < SVTAG_END; i++) {
		tag_info = &job->tag_info[i];
		tag_info->sv_pipe = NULL;
		tag_info->seninf_padidx = 0;
		tag_info->hw_scen = 0;
		tag_info->tag_order = MTKCAM_IPI_ORDER_FIRST_TAG;
	}
}

int update_sensor_meta_buf_desc(struct mtk_cam_job *job,
	unsigned int mbus_code,
	struct mtk_seninf_pad_data_info *pad_data_info)
{
	struct mtk_cam_driver_buf_desc *desc = &job->seninf_meta_buf_desc;

	if (has_embedded_parser(job->seninf) &&
		mtk_cam_seninf_get_ebd_info_by_scenario(job->seninf,
			mbus_code, pad_data_info) == 0) {
		desc->fmt_desc[0].ipi_fmt =
			sensor_mbus_to_ipi_fmt(pad_data_info->mbus_code);
		if (WARN_ON_ONCE(desc->fmt_desc[0].ipi_fmt ==
			MTKCAM_IPI_BAYER_PXL_ID_UNKNOWN))
			return 0;

		desc->fmt_desc[0].width = pad_data_info->exp_hsize;
		desc->fmt_desc[0].height = pad_data_info->exp_vsize;
		desc->fmt_desc[0].stride[0] =
			mtk_cam_dmao_xsize(
				desc->fmt_desc[0].width, desc->fmt_desc[0].ipi_fmt, 4);
		desc->fmt_desc[0].stride[1] = 0;
		desc->fmt_desc[0].stride[2] = 0;
		desc->fmt_desc[0].size =
			desc->fmt_desc[0].stride[0] * desc->fmt_desc[0].height;

		return 1;
	}

	return 0;
}

int handle_sv_tag(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_raw_sink_data *raw_sink;
	struct mtk_camsv_pipeline *sv_pipe;
	struct mtk_camsv_sink_data *sv_sink;
	struct mtk_camsv_tag_param img_tag_param[SVTAG_IMG_END];
	struct mtk_camsv_tag_param meta_tag_param;
	struct mtk_seninf_pad_data_info pad_data_info;
	unsigned int tag_idx, sv_pipe_idx, hw_scen;
	unsigned int exp_no, req_amount;
	int ret = 0, i;

	/* reset tag info */
	mtk_cam_sv_reset_tag_info(job);

	/* img tag(s) */
	if (job->job_scen.scen.normal.max_exp_num == 2) {
		exp_no = req_amount = 2;
		req_amount *= is_rgbw(job) ? 2 : 1;
		hw_scen = is_dc_mode(job) ?
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC_STAGGER)) :
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER));
	} else if (job->job_scen.scen.normal.max_exp_num == 3) {
		exp_no = req_amount = 3;
		if (is_rgbw(job)) {
			pr_info("[%s] rgbw not supported under 3-exp stagger case",
				__func__);
			return 1;
		}
		hw_scen = is_dc_mode(job) ?
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC_STAGGER)) :
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_STAGGER));
	} else {
		exp_no = req_amount = 1;
		req_amount *= is_rgbw(job) ? 2 : 1;
		hw_scen = is_dc_mode(job) ?
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_DC_STAGGER)) :
			(1 << HWPATH_ID(MTKCAM_IPI_HW_PATH_ON_THE_FLY));
	}
	pr_info("[%s] hw_scen:%d exp_no:%d req_amount:%d",
			__func__, hw_scen, exp_no, req_amount);
	if (mtk_cam_sv_get_tag_param(img_tag_param, hw_scen, exp_no, req_amount))
		return 1;

	raw_sink = get_raw_sink_data(job);
	if (WARN_ON(!raw_sink))
		return -1;

	for (i = 0; i < req_amount; i++) {
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&img_tag_param[i], hw_scen,
			is_camsv_16p(job) ? 4 : 3,
			job->sub_ratio,
			raw_sink->width, raw_sink->height,
			raw_sink->mbus_code, NULL);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << img_tag_param[i].tag_idx);

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d pixel_mode:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			img_tag_param[i].tag_idx,
			img_tag_param[i].seninf_padidx,
			img_tag_param[i].tag_order,
			is_camsv_16p(job) ? 4 : 3,
			raw_sink->width,
			raw_sink->height,
			raw_sink->mbus_code);
	}

	/* meta tag(s) */
	tag_idx = SVTAG_META_START;
	for (i = 0; i < ctx->num_sv_subdevs; i++) {
		if (tag_idx >= SVTAG_END)
			return 1;
		sv_pipe_idx = ctx->sv_subdev_idx[i];
		if (sv_pipe_idx >= ctx->cam->pipelines.num_camsv)
			return 1;

		sv_pipe = &ctx->cam->pipelines.camsv[sv_pipe_idx];
		sv_sink = &job->req->sv_data[sv_pipe_idx].sink;
		meta_tag_param.tag_idx = tag_idx;
		meta_tag_param.seninf_padidx = sv_pipe->seninf_padidx;
		meta_tag_param.tag_order = mtk_cam_seninf_get_tag_order(
			job->seninf, sv_sink->mbus_code, sv_pipe->seninf_padidx);
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&meta_tag_param, 1,
			is_camsv_16p(job) ? 4 : 3,
			job->sub_ratio,
			sv_sink->width, sv_sink->height,
			sv_sink->mbus_code, sv_pipe);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << tag_idx);
		tag_idx++;

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d pixel_mode:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			meta_tag_param.tag_idx,
			meta_tag_param.seninf_padidx,
			meta_tag_param.tag_order,
			is_camsv_16p(job) ? 4 : 3,
			sv_sink->width,
			sv_sink->height,
			sv_sink->mbus_code);
	}

	/* sensor meta */
	if (ctx->enable_sensor_meta_dump &&
		tag_idx < SVTAG_END &&
		update_sensor_meta_buf_desc(
			job, raw_sink->mbus_code, &pad_data_info)) {
		pr_info("[%s] sensor meta dump is on\n", __func__);
		job->is_sensor_meta_dump = true;

		tag_idx = SVTAG_SENSOR_META;

		meta_tag_param.tag_idx = tag_idx;
		meta_tag_param.seninf_padidx = PAD_SRC_GENERAL0;
		meta_tag_param.tag_order = mtk_cam_seninf_get_tag_order(
			job->seninf, raw_sink->mbus_code,
			meta_tag_param.seninf_padidx);
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&meta_tag_param, 1,
			is_camsv_16p(job) ? 4 : 3,
			job->sub_ratio,
			pad_data_info.exp_hsize,
			pad_data_info.exp_vsize,
			pad_data_info.mbus_code,
			NULL);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << tag_idx);

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d pixel_mode:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			meta_tag_param.tag_idx,
			meta_tag_param.seninf_padidx,
			meta_tag_param.tag_order,
			is_camsv_16p(job) ? 4 : 3,
			pad_data_info.exp_hsize,
			pad_data_info.exp_vsize,
			pad_data_info.mbus_code);
	} else {
		job->is_sensor_meta_dump = false;
		pr_info("[%s] sensor meta dump is off(enable:%d/tag_idx:%d)\n",
			__func__,
			(ctx->enable_sensor_meta_dump) ? 1 : 0,
			tag_idx);
	}

	ctx->is_sensor_meta_dump = job->is_sensor_meta_dump;
	ctx->seninf_meta_buf_desc = job->seninf_meta_buf_desc;
	ctx->used_tag_cnt = job->used_tag_cnt;
	ctx->enabled_tags = job->enabled_tags;
	memcpy(ctx->tag_info, job->tag_info,
		sizeof(struct mtk_camsv_tag_info) * CAMSV_MAX_TAGS);

	return ret;
}

int handle_sv_tag_display_ic(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_camsv_pipeline *sv_pipe;
	struct mtk_camsv_tag_param tag_param[3];
	struct v4l2_format *img_fmt;
	unsigned int width, height, mbus_code;
	unsigned int hw_scen;
	int ret = 0, i, sv_pipe_idx;

	/* reset tag info */
	mtk_cam_sv_reset_tag_info(job);

	if (ctx->num_sv_subdevs != 1)
		return 1;

	sv_pipe_idx = ctx->sv_subdev_idx[0];
	sv_pipe = &ctx->cam->pipelines.camsv[sv_pipe_idx];
	hw_scen = (1 << MTKCAM_SV_SPECIAL_SCENARIO_DISPLAY_IC);
	ret = mtk_cam_sv_get_tag_param(tag_param, hw_scen, 1, 3);

	for (i = 0; i < ARRAY_SIZE(tag_param); i++) {
		if (tag_param[i].tag_idx == SVTAG_0) {
			img_fmt = &sv_pipe->vdev_nodes[
				MTK_CAMSV_MAIN_STREAM_OUT - MTK_CAMSV_SINK_NUM].active_fmt;
			width = img_fmt->fmt.pix_mp.width;
			height = img_fmt->fmt.pix_mp.height;
			if (img_fmt->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV21)
				mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8;
			else
				mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10;
		} else if (tag_param[i].tag_idx == SVTAG_1) {
			img_fmt = &sv_pipe->vdev_nodes[
				MTK_CAMSV_MAIN_STREAM_OUT - MTK_CAMSV_SINK_NUM].active_fmt;
			width = img_fmt->fmt.pix_mp.width;
			height = img_fmt->fmt.pix_mp.height / 2;
			if (img_fmt->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV21)
				mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8;
			else
				mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10;
		} else {
			img_fmt = &sv_pipe->vdev_nodes[
				MTK_CAMSV_EXT_STREAM_OUT - MTK_CAMSV_SINK_NUM].active_fmt;
			width = img_fmt->fmt.pix_mp.width;
			height = img_fmt->fmt.pix_mp.height;
			mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8;
		}
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&tag_param[i], 1, 3, job->sub_ratio,
			width, height,
			mbus_code, sv_pipe);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << tag_param[i].tag_idx);

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			tag_param[i].tag_idx,
			tag_param[i].seninf_padidx,
			tag_param[i].tag_order,
			width,
			height,
			mbus_code);
	}

	ctx->is_sensor_meta_dump = job->is_sensor_meta_dump = false;
	ctx->used_tag_cnt = job->used_tag_cnt;
	ctx->enabled_tags = job->enabled_tags;
	memcpy(ctx->tag_info, job->tag_info,
		sizeof(struct mtk_camsv_tag_info) * CAMSV_MAX_TAGS);

	return ret;
}

int handle_sv_tag_only_sv(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_camsv_pipeline *sv_pipe;
	struct mtk_camsv_sink_data *sv_sink = NULL;
	struct mtk_camsv_tag_param tag_param;
	struct mtk_seninf_pad_data_info pad_data_info;
	unsigned int tag_idx, sv_pipe_idx;
	int ret = 0, i;

	/* reset tag info */
	mtk_cam_sv_reset_tag_info(job);

	/* img tag(s) */
	tag_idx = SVTAG_START;
	for (i = 0; i < ctx->num_sv_subdevs; i++) {
		sv_pipe_idx = ctx->sv_subdev_idx[i];
		if (sv_pipe_idx >= ctx->cam->pipelines.num_camsv)
			return 1;
		sv_pipe = &ctx->cam->pipelines.camsv[sv_pipe_idx];
		sv_sink = &job->req->sv_data[sv_pipe_idx].sink;
		tag_param.tag_idx = tag_idx;
		tag_param.seninf_padidx = sv_pipe->seninf_padidx;
		tag_param.tag_order = mtk_cam_seninf_get_tag_order(
			job->seninf, sv_sink->mbus_code, sv_pipe->seninf_padidx);
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&tag_param, 1, 3, job->sub_ratio,
			sv_sink->width, sv_sink->height,
			sv_sink->mbus_code, sv_pipe);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << tag_idx);
		tag_idx++;

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			tag_param.tag_idx,
			tag_param.seninf_padidx,
			tag_param.tag_order,
			sv_sink->width,
			sv_sink->height,
			sv_sink->mbus_code);
	}

	/* sensor meta */
	if (ctx->enable_sensor_meta_dump &&
		tag_idx < SVTAG_END &&
		sv_sink &&
		update_sensor_meta_buf_desc(
			job, sv_sink->mbus_code, &pad_data_info)) {
		pr_info("[%s] sensor meta dump is on\n", __func__);
		job->is_sensor_meta_dump = true;

		tag_idx = SVTAG_SENSOR_META;

		tag_param.tag_idx = tag_idx;
		tag_param.seninf_padidx = PAD_SRC_GENERAL0;
		tag_param.tag_order = mtk_cam_seninf_get_tag_order(
			job->seninf, sv_sink->mbus_code,
			tag_param.seninf_padidx);
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&tag_param, 1, 3, job->sub_ratio,
			pad_data_info.exp_hsize,
			pad_data_info.exp_vsize,
			pad_data_info.mbus_code,
			NULL);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << tag_idx);

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			tag_param.tag_idx,
			tag_param.seninf_padidx,
			tag_param.tag_order,
			pad_data_info.exp_hsize,
			pad_data_info.exp_vsize,
			pad_data_info.mbus_code);
	} else {
		job->is_sensor_meta_dump = false;
		pr_info("[%s] sensor meta dump is off(enable:%d/tag_idx:%d)\n",
			__func__,
			(ctx->enable_sensor_meta_dump) ? 1 : 0,
			tag_idx);
	}

	ctx->is_sensor_meta_dump = job->is_sensor_meta_dump;
	ctx->seninf_meta_buf_desc = job->seninf_meta_buf_desc;
	ctx->used_tag_cnt = job->used_tag_cnt;
	ctx->enabled_tags = job->enabled_tags;
	memcpy(ctx->tag_info, job->tag_info,
		sizeof(struct mtk_camsv_tag_info) * CAMSV_MAX_TAGS);

	return ret;
}

bool is_sv_img_tag_used(struct mtk_cam_job *job)
{
	bool rst = false;

	if (job->job_scen.id == MTK_CAM_SCEN_NORMAL &&
		job->job_scen.scen.normal.exp_num > 1)
		rst = !is_m2m(job) &&
			get_hw_scenario(job) != MTKCAM_IPI_HW_PATH_OTF_STAGGER_LN_INTL;
	if (is_dc_mode(job))
		rst = true;
	if (is_sv_pure_raw(job))
		rst = true;
	if (is_extisp(job))
		rst = true;
	if (job->is_sensor_meta_dump)
		rst = true;

	return rst;
}

bool belong_to_current_ctx(struct mtk_cam_job *job, int ipi_pipe_id)
{
	unsigned long ctx_used_pipe;

	ctx_used_pipe = job->src_ctx->used_pipe;
	return ctx_used_pipe & ipi_pipe_id_to_bit(ipi_pipe_id);
}

void fill_hdr_timestamp(struct mtk_cam_job *job,
				   struct mtk_cam_ctrl_runtime_info *info)
{
	int exp_order = get_exp_order(&job->job_scen);

	switch (job->job_type) {
	case JOB_TYPE_STAGGER:
		if (exp_order == MTKCAM_IPI_ORDER_SE_NE) {
			job->hdr_ts_cache.le = info->sof_l_ts_ns;
			job->hdr_ts_cache.le_mono = info->sof_l_ts_mono_ns;
			job->hdr_ts_cache.se = info->sof_ts_ns;
			job->hdr_ts_cache.se_mono = info->sof_ts_mono_ns;
		} else {
			job->hdr_ts_cache.le = info->sof_ts_ns;
			job->hdr_ts_cache.le_mono = info->sof_ts_mono_ns;
			job->hdr_ts_cache.se = info->sof_l_ts_ns;
			job->hdr_ts_cache.se_mono = info->sof_l_ts_mono_ns;
		}
		break;
	case JOB_TYPE_MSTREAM:
		if (exp_order == MTKCAM_IPI_ORDER_SE_NE) {
			if (!job->hdr_ts_cache.se && !job->hdr_ts_cache.le) {
				job->hdr_ts_cache.se = info->sof_ts_ns;
				job->hdr_ts_cache.se_mono = info->sof_ts_mono_ns;
			} else {
				job->hdr_ts_cache.le = info->sof_ts_ns;
				job->hdr_ts_cache.le_mono = info->sof_ts_mono_ns;
			}
		} else {
			if (!job->hdr_ts_cache.se && !job->hdr_ts_cache.le) {
				job->hdr_ts_cache.le = info->sof_ts_ns;
				job->hdr_ts_cache.le_mono = info->sof_ts_mono_ns;
			} else {
				job->hdr_ts_cache.se = info->sof_ts_ns;
				job->hdr_ts_cache.se_mono = info->sof_ts_mono_ns;
			}
		}
		break;
	default:
		break;
	}
}
