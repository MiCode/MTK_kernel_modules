// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>

#include <mtk-interconnect.h>
#include <soc/mediatek/mmdvfs_v3.h>
#include <soc/mediatek/mmqos.h>
#include "mtk_cam-dvfs_qos.h"

struct dvfs_stream_info {
	unsigned int freq_hz;
};

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

	spin_lock_init(&dvfs->lock);

	return 0;

opp_default_table:
	dvfs->opp_num = 1;
	dvfs->opp[0] = (struct camsys_opp_table) {
		.freq_hz = 546000000,
		.volt_uv = 650000,
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

static int mtk_cam_dvfs_get_clkidx(struct mtk_camsys_dvfs *dvfs,
				   unsigned int freq)
{
	int i;

	for (i = 0; i < dvfs->opp_num; i++)
		if (freq == dvfs->opp[i].freq_hz)
			break;

	if (i == dvfs->opp_num) {
		dev_info(dvfs->dev, "failed to find idx for freq %u\n", freq);
		return -1;
	}
	return i;
}

static bool to_update_stream_opp_idx(struct mtk_camsys_dvfs *dvfs,
				     int stream_id, unsigned int freq,
				     unsigned int *max_freq)
{
	int i;
	unsigned int fmax = 0;
	bool change;

	spin_lock(&dvfs->lock);

	change = !(dvfs->stream_infos[stream_id].freq_hz == freq);

	dvfs->stream_infos[stream_id].freq_hz = freq;
	for (i = 0; i < dvfs->max_stream_num; i++)
		fmax = max(fmax, dvfs->stream_infos[i].freq_hz);

	spin_unlock(&dvfs->lock);

	if (max_freq)
		*max_freq = fmax;

	dev_info(dvfs->dev, "%s: change %u, max %u\n", __func__, change, fmax);
	return change;
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

/* TODO:
 *  1. temporal adjustment
 */

int mtk_cam_dvfs_update(struct mtk_camsys_dvfs *dvfs,
			int stream_id, unsigned int target_freq_hz)
{
	int opp_idx = -1;
	int max_freq = 0, tar_freq = 0;
	int ret;

	if (WARN_ON(stream_id < 0 || stream_id >= dvfs->max_stream_num))
		return -1;

	/* dvfs reset */
	if (target_freq_hz == 0 && ARRAY_SIZE(dvfs->opp) > 0)
		tar_freq = dvfs->opp[0].freq_hz;
	else
		tar_freq = target_freq_hz;

	/* check if freq is changed */
	if (!to_update_stream_opp_idx(dvfs,
				     stream_id, tar_freq, &max_freq))
		return 0;

	opp_idx = mtk_cam_dvfs_get_clkidx(dvfs, max_freq);
	if (opp_idx < 0)
		return -1;

	/* TODO(Roy): dynamic change to lower dvfs level */
	if (dvfs->cur_opp_idx != -1 &&
			target_freq_hz != 0 &&
			opp_idx <= dvfs->cur_opp_idx)
		return 0;

	ret = mtk_cam_dvfs_update_opp(dvfs, opp_idx);
	if (ret)
		return ret;

	dev_info(dvfs->dev,
		 "dvfs_update: stream %d freq %u/max %u, opp_idx: %d->%d\n",
		 stream_id, target_freq_hz, max_freq,
		 dvfs->cur_opp_idx, opp_idx);

	dvfs->cur_opp_idx = opp_idx;

	return 0;
}

struct mtk_camsys_qos_path {
	int id;
	struct icc_path *path;
};

int mtk_cam_qos_probe(struct device *dev,
		      struct mtk_camsys_qos *qos,
		      int *ids, int n_id)
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

	//dev_info(dev, "icc_path num %d\n", qos->n_path);
	if (qos->n_path > ARRAY_SIZE(names)) {
		dev_info(dev, "%s: array size of names is not enough.\n",
			 __func__);
		return -EINVAL;
	}

	if (qos->n_path != n_id) {
		dev_info(dev, "icc num(%d) mismatch with ids num(%d)\n",
			 qos->n_path, n_id);
		return -EINVAL;
	}

	of_property_read_string_array(dev->of_node, "interconnect-names",
				      names, qos->n_path);

	for (i = 0, cam_path = qos->cam_path; i < qos->n_path; i++, cam_path++) {
		//dev_info(dev, "interconnect: idx %d [%s id = %d]\n",
		//	 i, names[i], ids[i]);

		cam_path->id = ids[i];
		cam_path->path = of_mtk_icc_get(dev, names[i]);
		if (IS_ERR_OR_NULL(cam_path->path)) {
			dev_info(dev, "failed to get icc of %s\n",
				 names[i]);
			ret = -EINVAL;
		}
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

static struct mtk_camsys_qos_path *find_qos_path(struct mtk_camsys_qos *qos,
						 int id)
{
	struct mtk_camsys_qos_path *cam_path;
	int i;

	for (i = 0, cam_path = qos->cam_path; i < qos->n_path; i++, cam_path++)
		if (cam_path->id == id)
			return cam_path;
	return NULL;
}

int mtk_cam_qos_update(struct mtk_camsys_qos *qos,
		       int path_id, u32 avg_bw, u32 peak_bw)
{
	struct mtk_camsys_qos_path *cam_path;

	cam_path = find_qos_path(qos, path_id);
	if (!cam_path)
		return -1;

	/* TODO
	 * 1. detect change
	 * 2. temporal adjustment
	 */
	return mtk_icc_set_bw(cam_path->path, avg_bw, peak_bw);
}

int mtk_cam_qos_reset_all(struct mtk_camsys_qos *qos)
{
	struct mtk_camsys_qos_path *cam_path;
	int i;

	for (i = 0, cam_path = qos->cam_path; i < qos->n_path; i++, cam_path++)
		mtk_icc_set_bw(cam_path->path, 0, 0);
	return 0;
}

int mtk_cam_qos_wait_throttle_done(void)
{
	mtk_mmqos_wait_throttle_done();
	return 0;
}
