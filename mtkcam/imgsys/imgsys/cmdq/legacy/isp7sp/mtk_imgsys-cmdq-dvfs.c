// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */
#include "mtk_imgsys-engine.h"
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-cmdq-plat.h"
#include "mtk_imgsys-cmdq-dvfs.h"


#if DVFS_QOS_READY
void mtk_imgsys_mmdvfs_init_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
	u64 freq = 0;
	int ret = 0, opp_num = 0, opp_idx = 0, idx = 0, volt;
	struct device_node *np, *child_np = NULL;
	struct of_phandle_iterator it;

	memset((void *)dvfs_info, 0x0, sizeof(struct mtk_imgsys_dvfs));
	dvfs_info->dev = imgsys_dev->dev;
	dvfs_info->reg = NULL;
	ret = dev_pm_opp_of_add_table(dvfs_info->dev);
	if (ret < 0) {
		dev_info(dvfs_info->dev,
			"%s: [ERROR] fail to init opp table: %d\n", __func__, ret);
		return;
	}
	dvfs_info->reg = devm_regulator_get_optional(dvfs_info->dev, "dvfsrc-vmm");
	if (IS_ERR_OR_NULL(dvfs_info->reg)) {
		dev_info(dvfs_info->dev,
			"%s: [ERROR] Failed to get dvfsrc-vmm\n", __func__);
		dvfs_info->reg = NULL;
		if (!mmdvfs_get_version())
			dvfs_info->mmdvfs_clk = devm_clk_get(dvfs_info->dev, "mmdvfs_clk");
		else {
			dvfs_info->mmdvfs_clk = devm_clk_get(dvfs_info->dev, "mmdvfs_mux");
			dvfs_info->mmdvfs_clk_smi = devm_clk_get(dvfs_info->dev, "mmdvfs_mux_smi");
		}
		if (IS_ERR_OR_NULL(dvfs_info->mmdvfs_clk)) {
			dev_info(dvfs_info->dev,
				"%s: [ERROR] Failed to get mmdvfs_clk\n", __func__);
			dvfs_info->mmdvfs_clk = NULL;
			return;
		}
		if (IS_ERR_OR_NULL(dvfs_info->mmdvfs_clk_smi)) {
			dev_info(dvfs_info->dev,
				"%s: [ERROR] Failed to get mmdvfs_clk_smi\n", __func__);
			dvfs_info->mmdvfs_clk_smi = NULL;
		}
	}

	if (dvfs_info->reg)
		opp_num = regulator_count_voltages(dvfs_info->reg);
	of_for_each_phandle(
		&it, ret, dvfs_info->dev->of_node, "operating-points-v2", NULL, 0) {
		np = of_node_get(it.node);
		if (!np) {
			dev_info(dvfs_info->dev, "%s: [ERROR] of_node_get fail\n", __func__);
			return;
		}

		do {
			child_np = of_get_next_available_child(np, child_np);
			if (child_np) {
				of_property_read_u64(child_np, "opp-hz", &freq);
				of_property_read_u32(child_np, "opp-microvolt", &volt);
				if ((freq == 0) || (volt == 0)) {
					dev_info(
						dvfs_info->dev,
						"%s: [ERROR] parsing zero freq/volt(%llu/%d) at idx(%d)\n",
						__func__, freq, volt, idx);
					continue;
				} else if (volt > IMGSYS_DVFS_MAX_VOLT) {
					dev_info(
						dvfs_info->dev,
						"%s: [ERROR] volt(%d) is over maximum(%d) at idx(%d)\n",
						__func__, volt, IMGSYS_DVFS_MAX_VOLT, idx);
					break;
				}
				dvfs_info->clklv_dts[opp_idx][idx] = freq;
				dvfs_info->voltlv_dts[opp_idx][idx] = volt;
				idx++;
			}
		} while (child_np);
		dvfs_info->clklv_num_dts[opp_idx] = idx;
		dvfs_info->clklv_target[opp_idx] = dvfs_info->clklv_dts[opp_idx][0];
		dvfs_info->clklv_idx[opp_idx] = 0;
		idx = 0;
		opp_idx++;
		of_node_put(np);
	}

	opp_num = opp_idx;
	for (opp_idx = 0; opp_idx < opp_num; opp_idx++) {
		for (idx = 0; idx < dvfs_info->clklv_num_dts[opp_idx]; idx++) {
			dev_info(dvfs_info->dev, "[%s] opp=%d, idx_dts=%d, clk_dts=%d volt_dts=%d\n",
				__func__, opp_idx, idx, dvfs_info->clklv_dts[opp_idx][idx],
				dvfs_info->voltlv_dts[opp_idx][idx]);
		}
	}

	/* For fine grain dvfs */
	for (opp_idx = 0; opp_idx < opp_num; opp_idx++) {
		idx = 0;
		dvfs_info->clklv_num_fg[opp_idx] =
			dvfs_info->clklv_num_dts[opp_idx] * 2 - 1;
		dvfs_info->clklv_fg[opp_idx][idx] = dvfs_info->clklv_dts[opp_idx][idx];
		dvfs_info->voltlv_fg[opp_idx][idx] = dvfs_info->voltlv_dts[opp_idx][idx];
		dev_info(dvfs_info->dev, "[%s] opp=%d, idx_fg=%d, clk_fg=%d volt_fg=%d\n",
			__func__, opp_idx, idx,
			dvfs_info->clklv_fg[opp_idx][idx],
			dvfs_info->voltlv_fg[opp_idx][idx]);
		for (idx = 1; idx < dvfs_info->clklv_num_dts[opp_idx]; idx++) {
			dvfs_info->clklv_fg[opp_idx][idx*2-1] =
				(((dvfs_info->clklv_dts[opp_idx][idx-1] / IMGSYS_DVFS_MHz) *
				IMGSYS_DVFS_RATIO_L +
				(dvfs_info->clklv_dts[opp_idx][idx] / IMGSYS_DVFS_MHz) *
				IMGSYS_DVFS_RATIO_H) /
				(IMGSYS_DVFS_RATIO_L + IMGSYS_DVFS_RATIO_H)) * IMGSYS_DVFS_MHz;
			dvfs_info->voltlv_fg[opp_idx][idx*2-1] =
				(dvfs_info->voltlv_dts[opp_idx][idx-1] * IMGSYS_DVFS_RATIO_L +
				dvfs_info->voltlv_dts[opp_idx][idx] * IMGSYS_DVFS_RATIO_H) /
				(IMGSYS_DVFS_RATIO_L + IMGSYS_DVFS_RATIO_H);
			dvfs_info->clklv_fg[opp_idx][idx*2] =
				dvfs_info->clklv_dts[opp_idx][idx];
			dvfs_info->voltlv_fg[opp_idx][idx*2] =
				dvfs_info->voltlv_dts[opp_idx][idx];
			dev_info(dvfs_info->dev, "[%s] opp=%d, idx_fg=%d, clk_fg=%d volt_fg=%d\n",
				__func__, opp_idx, idx*2-1,
				dvfs_info->clklv_fg[opp_idx][idx*2-1],
				dvfs_info->voltlv_fg[opp_idx][idx*2-1]);
			dev_info(dvfs_info->dev, "[%s] opp=%d, idx_fg=%d, clk_fg=%d volt_fg=%d\n",
				__func__, opp_idx, idx*2,
				dvfs_info->clklv_fg[opp_idx][idx*2],
				dvfs_info->voltlv_fg[opp_idx][idx*2]);
		}
	}

	if (of_property_read_u32(dvfs_info->dev->of_node,
		"mediatek,imgsys-dvfs-pix-mode", &ret) != 0) {
		dev_info(dvfs_info->dev, "mmdvfs pix mode is not exist\n");
	} else {
		dvfs_info->pix_mode = ret;
	}

	dvfs_info->cur_volt = 0;
	dvfs_info->cur_freq = 0;
	dvfs_info->cur_freq_smi = 0;
	dvfs_info->vss_task_cnt = 0;
	dvfs_info->smvr_task_cnt = 0;
	dvfs_info->opp_num = opp_num;

}

void mtk_imgsys_mmdvfs_uninit_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
	int volt = 0, ret = 0;
	unsigned long freq = 0;

	dvfs_info->cur_volt = volt;
	dvfs_info->cur_freq = freq;
	dvfs_info->cur_freq_smi = freq;

	if (dvfs_info->reg) {
		ret = regulator_set_voltage(dvfs_info->reg, volt, INT_MAX);
		if (ret)
			dev_info(dvfs_info->dev,
				"[%s] Failed to set regulator voltage(%d) with ret(%d)\n",
				__func__, volt, ret);
	} else if (dvfs_info->mmdvfs_clk) {
		ret = clk_set_rate(dvfs_info->mmdvfs_clk, freq);
		if (ret)
			dev_info(dvfs_info->dev,
				"[%s] Failed to set mmdvfs rate(%ld) with ret(%d)\n",
				__func__, freq, ret);
		if (dvfs_info->mmdvfs_clk_smi) {
			ret = clk_set_rate(dvfs_info->mmdvfs_clk_smi, freq);
			if (ret)
				dev_info(dvfs_info->dev,
					"[%s] Failed to set mmdvfs_smi rate(%ld) with ret(%d)\n",
					__func__, freq, ret);
		}
	} else
		dev_info(dvfs_info->dev,
			"%s: [ERROR] reg and clk is err or null\n", __func__);

}

void mtk_imgsys_mmdvfs_set_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				bool isSet)
{
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
	int volt = 0, ret = 0, idx = 0, opp_idx = 0;
	unsigned long freq = 0, freq_smi = 0;
	/* u32 hw_comb = frm_info->user_info[0].hw_comb; */

	freq = dvfs_info->freq;

	if (IS_ERR_OR_NULL(dvfs_info->reg) && IS_ERR_OR_NULL(dvfs_info->mmdvfs_clk)) {
        if (imgsys_cmdq_dbg_enable_plat7sp())
		dev_dbg(dvfs_info->dev, "%s: [ERROR] reg and clk is err or null\n", __func__);
	}
	else {
		/* Choose for IPESYS */
		/* if (hw_comb & IMGSYS_ENG_ME) */
			/* opp_idx = 1; */

		for (idx = 0; idx < dvfs_info->clklv_num[opp_idx]; idx++) {
			if (freq <= dvfs_info->clklv[opp_idx][idx])
				break;
		}
		if (idx == dvfs_info->clklv_num[opp_idx])
			idx--;
		volt = dvfs_info->voltlv[opp_idx][idx];

		freq = dvfs_info->clklv[opp_idx][idx]; // signed-off

		if (dvfs_info->cur_volt != volt) {
			if (imgsys_dvfs_dbg_enable_plat7sp())
				dev_info(dvfs_info->dev, "[%s] volt change opp=%d, idx=%d, clk=%d volt=%d\n",
					__func__, opp_idx, idx, dvfs_info->clklv[opp_idx][idx],
					dvfs_info->voltlv[opp_idx][idx]);
			if (dvfs_info->reg) {
				ret = regulator_set_voltage(dvfs_info->reg, volt, INT_MAX);
				if (ret)
					dev_info(dvfs_info->dev,
						"[%s] Failed to set regulator voltage(%d) with ret(%d)\n",
						__func__, volt, ret);
			} else if (dvfs_info->mmdvfs_clk) {
				/* imgsys to mminfra clk mapping table */
				if (freq >= IMGSYS_IMG_CLK_LV1)
					freq_smi = IMGSYS_SMI_CLK_LV1;
				else if (freq >= IMGSYS_IMG_CLK_LV2)
					freq_smi = IMGSYS_SMI_CLK_LV2;
				else if (freq >= IMGSYS_IMG_CLK_LV3)
					freq_smi = IMGSYS_SMI_CLK_LV3;
				else
					freq_smi = IMGSYS_SMI_CLK_LV4;
				if (dvfs_info->cur_volt < volt) { // increase: mminfra -> img
					if (dvfs_info->mmdvfs_clk_smi) {
						ret = clk_set_rate(dvfs_info->mmdvfs_clk_smi, freq_smi);
						if (ret)
							dev_info(dvfs_info->dev,
								"[%s] Failed to set mmdvfs_smi rate(%ld) with ret(%d)\n",
								__func__, freq, ret);
					}
					if (dvfs_info->mmdvfs_clk) {
						ret = clk_set_rate(dvfs_info->mmdvfs_clk, freq);
						if (ret)
							dev_info(dvfs_info->dev,
								"[%s] Failed to set mmdvfs rate(%ld) with ret(%d)\n",
								__func__, freq, ret);
					}
				} else { //decrease: img -> mminfra
					if (dvfs_info->mmdvfs_clk) {
						ret = clk_set_rate(dvfs_info->mmdvfs_clk, freq);
						if (ret)
							dev_info(dvfs_info->dev,
								"[%s] Failed to set mmdvfs rate(%ld) with ret(%d)\n",
								__func__, freq, ret);
					}
					if (dvfs_info->mmdvfs_clk_smi) {
						ret = clk_set_rate(dvfs_info->mmdvfs_clk_smi, freq_smi);
						if (ret)
							dev_info(dvfs_info->dev,
								"[%s] Failed to set mmdvfs_smi rate(%ld) with ret(%d)\n",
								__func__, freq, ret);
					}
				}
			}
			dvfs_info->cur_volt = volt;
			dvfs_info->cur_freq = freq;
			dvfs_info->cur_freq_smi = freq_smi;
		}
	}
}

void mtk_imgsys_mmdvfs_reset_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
	unsigned int *clklv = NULL, *voltlv = NULL;
	int volt = 0, ret = 0, idx = 0, opp_idx = 0;
	unsigned long freq = 0;

	for (opp_idx = 0; opp_idx < dvfs_info->opp_num; opp_idx++) {
		if (imgsys_fine_grain_dvfs_enable_plat7sp()) {
			dvfs_info->clklv_num[opp_idx] = dvfs_info->clklv_num_fg[opp_idx];
			clklv = &dvfs_info->clklv_fg[opp_idx][0];
			voltlv = &dvfs_info->voltlv_fg[opp_idx][0];
		} else {
			dvfs_info->clklv_num[opp_idx] = dvfs_info->clklv_num_dts[opp_idx];
			clklv = &dvfs_info->clklv_dts[opp_idx][0];
			voltlv = &dvfs_info->voltlv_dts[opp_idx][0];
		}
		dev_info(dvfs_info->dev, "[%s] fine grain(%d), opp=%d, clk_num=%d\n",
			__func__, imgsys_fine_grain_dvfs_enable_plat7sp(),
			opp_idx, dvfs_info->clklv_num[opp_idx]);
		for (idx = 0; idx < dvfs_info->clklv_num[opp_idx]; idx++) {
			dvfs_info->clklv[opp_idx][idx] = clklv[idx];
			dvfs_info->voltlv[opp_idx][idx] = voltlv[idx];
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			dev_dbg(dvfs_info->dev, "[%s] opp=%d, idx=%d, clk=%d volt=%d\n",
				__func__, opp_idx, idx,
				dvfs_info->clklv[opp_idx][idx],
				dvfs_info->voltlv[opp_idx][idx]);
		}
	}
	}

	for (idx = 0; idx < MTK_IMGSYS_DVFS_GROUP; idx++)
		dvfs_info->pixel_size[idx] = 0;

	if (IS_ERR_OR_NULL(dvfs_info->reg) && IS_ERR_OR_NULL(dvfs_info->mmdvfs_clk)) {
        if (imgsys_cmdq_dbg_enable_plat7sp())
		dev_dbg(dvfs_info->dev, "%s: [ERROR] reg and clk is err or null\n", __func__);
	}
	else {
		if (dvfs_info->cur_volt != volt) {
			dev_info(dvfs_info->dev, "[%s] volt change clk=%ld volt=%d\n",
				__func__, freq, volt);
			if (dvfs_info->reg) {
				ret = regulator_set_voltage(dvfs_info->reg, volt, INT_MAX);
				if (ret)
					dev_info(dvfs_info->dev,
						"[%s] Failed to set regulator voltage(%d) with ret(%d)\n",
						__func__, volt, ret);
			} else if (dvfs_info->mmdvfs_clk) {
				ret = clk_set_rate(dvfs_info->mmdvfs_clk, freq);
				if (ret)
					dev_info(dvfs_info->dev,
						"[%s] Failed to set mmdvfs rate(%ld) with ret(%d)\n",
						__func__, freq, ret);
				if (dvfs_info->mmdvfs_clk_smi) {
					ret = clk_set_rate(dvfs_info->mmdvfs_clk_smi, freq);
					if (ret)
						dev_info(dvfs_info->dev,
							"[%s] Failed to set mmdvfs_smi rate(%ld) with ret(%d)\n",
							__func__, freq, ret);
				}
			}
		}
	}

	dvfs_info->cur_volt = volt;
	dvfs_info->cur_freq = freq;
	dvfs_info->cur_freq_smi = freq;
	dvfs_info->vss_task_cnt = 0;
	dvfs_info->smvr_task_cnt = 0;
}


void mtk_imgsys_mmdvfs_mmqos_cal_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				bool isSet)
{
	struct mtk_imgsys_dvfs *dvfs_info = NULL;
	struct mtk_imgsys_qos *qos_info = NULL;
	unsigned long pixel_size[MTK_IMGSYS_DVFS_GROUP] = {0};
	int frm_num = 0, frm_idx = 0, g_idx;
	u32 hw_comb = 0;
	u32 batch_num = 0;
	u32 fps = 0;
	u32 fps_smvr = 0;
	u32 bw_exe = 0;
	unsigned long freq = 0;
	#if IMGSYS_DVFS_ENABLE
	unsigned long pixel_max = 0, pixel_total_max = 0;
	unsigned long smvr_size = 0, smvr_freq_floor = 0;
	/* struct timeval curr_time; */
	u64 ts_fps = 0;
	#endif

	dvfs_info = &imgsys_dev->dvfs_info;
	qos_info = &imgsys_dev->qos_info;
	frm_num = frm_info->total_frmnum;
	batch_num = frm_info->batchnum;
	fps = frm_info->fps;
	fps_smvr = fps * batch_num;
	/* fps = (batch_num)?(frm_info->fps*batch_num):frm_info->fps; */
	bw_exe = fps;
	hw_comb = frm_info->user_info[0].hw_comb;

	/* Calculate DVFS*/
	if (fps != 0) {
		for (frm_idx = 0; frm_idx < frm_num; frm_idx++) {
			for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++) {
				if (frm_info->user_info[frm_idx].hw_comb & dvfs_group[g_idx].g_hw)
					pixel_size[g_idx] += frm_info->user_info[frm_idx].pixel_bw;
			}
			/* Using ME for SMVR size check */
			if ((batch_num > 1) &&
				(frm_info->user_info[frm_idx].hw_comb & IMGSYS_ENG_ME))
				smvr_size = frm_info->user_info[frm_idx].pixel_bw;
		}
	}
	#if IMGSYS_DVFS_ENABLE
	if (fps != 0)
		ts_fps = 1000000 / fps;
	else
		ts_fps = 0;

	if (isSet == 1) {
		if (fps != 0) {
			for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++) {
				dvfs_info->pixel_size[g_idx] += (pixel_size[g_idx]*fps);
				if (pixel_size[g_idx] > pixel_max)
					pixel_max = pixel_size[g_idx];
				if (dvfs_info->pixel_size[g_idx] > pixel_total_max)
					pixel_total_max = dvfs_info->pixel_size[g_idx];
			}

			if (batch_num > 1) {
				dvfs_info->smvr_task_cnt++;
				if (dvfs_info->pix_mode == 4) {
					if ((fps_smvr >= IMGSYS_SMVR_FPS_FLOOR4) ||
						((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR2) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR3)))
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR3;
					else if ((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR3) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR1))
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR2;
					else if (fps_smvr >= IMGSYS_SMVR_FPS_FLOOR1)
						smvr_freq_floor = dvfs_info->freq;
					else
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR1;
				} else if (dvfs_info->pix_mode == 2) {
					if (((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR2) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR2)) ||
						((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR1) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR3)))
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR2;
					else if (fps_smvr >= IMGSYS_SMVR_FPS_FLOOR1)
						smvr_freq_floor = dvfs_info->freq;
					else
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR1;
				} else
					smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR3;
				if (pixel_total_max < smvr_freq_floor)
					freq = smvr_freq_floor;
				else
					freq = pixel_total_max;
			} else if (dvfs_info->smvr_task_cnt == 0)
				freq = pixel_total_max;

			if (dvfs_info->vss_task_cnt == 0)
				dvfs_info->freq = freq;
		} else {
			dvfs_info->vss_task_cnt++;
			freq = IMGSYS_VSS_FREQ_FLOOR; /* Forcing highest frequency if fps is 0 */
			if (freq > dvfs_info->freq)
				dvfs_info->freq = freq;
		}
	} else if (isSet == 0) {
		if (fps != 0) {
			for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++) {
				dvfs_info->pixel_size[g_idx] -= (pixel_size[g_idx]*fps);
				if (pixel_size[g_idx] > pixel_max)
					pixel_max = pixel_size[g_idx];
				if (dvfs_info->pixel_size[g_idx] > pixel_total_max)
					pixel_total_max = dvfs_info->pixel_size[g_idx];
			}

			if (batch_num > 1) {
				dvfs_info->smvr_task_cnt--;
				if (dvfs_info->pix_mode == 4) {
					if ((fps_smvr >= IMGSYS_SMVR_FPS_FLOOR4) ||
						((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR2) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR3)))
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR3;
					else if ((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR3) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR1))
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR2;
					else if (fps_smvr >= IMGSYS_SMVR_FPS_FLOOR1)
						smvr_freq_floor = dvfs_info->freq;
					else
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR1;
				} else if (dvfs_info->pix_mode == 2) {
					if (((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR2) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR2)) ||
						((smvr_size >= IMGSYS_SMVR_SIZE_FLOOR1) &&
						(fps_smvr >= IMGSYS_SMVR_FPS_FLOOR3)))
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR2;
					else if (fps_smvr >= IMGSYS_SMVR_FPS_FLOOR1)
						smvr_freq_floor = dvfs_info->freq;
					else
						smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR1;
				} else
					smvr_freq_floor = IMGSYS_SMVR_FREQ_FLOOR3;
				if (pixel_total_max < smvr_freq_floor)
					freq = smvr_freq_floor;
				else
					freq = pixel_total_max;
			} else if (dvfs_info->smvr_task_cnt == 0)
				freq = pixel_total_max;

			if (dvfs_info->vss_task_cnt == 0)
				dvfs_info->freq = freq;

			//if (pixel_total_max == 0) {
			//	freq = 0;
			//	dvfs_info->freq = freq;
			//}
		} else {
			dvfs_info->vss_task_cnt--;
			if (dvfs_info->vss_task_cnt == 0) {
				for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++)
					if (dvfs_info->pixel_size[g_idx] > pixel_total_max)
						pixel_total_max = dvfs_info->pixel_size[g_idx];
				freq = pixel_total_max;
				dvfs_info->freq = freq;
			}
		}
	}

	if (imgsys_dvfs_dbg_enable_plat7sp())
		dev_info(qos_info->dev,
		"[%s] isSet(%d) fps(%d/%d/%d) batchNum(%d) bw_exe(%d) vss(%d) smvr(%d/%lu/%lu) freq(%lu/%lu) local_pix_sz(%lu/%lu/%lu/%lu) global_pix_sz(%lu/%lu/%lu/%lu)\n",
		__func__, isSet, fps, frm_info->fps, fps_smvr, batch_num, bw_exe,
		dvfs_info->vss_task_cnt, dvfs_info->smvr_task_cnt, smvr_size,
		smvr_freq_floor, freq, dvfs_info->freq,
		pixel_size[0], pixel_size[1], pixel_size[2], pixel_max,
		dvfs_info->pixel_size[0], dvfs_info->pixel_size[1],
		dvfs_info->pixel_size[2], pixel_total_max
		);
	#else
	if (isSet == 1) {
		for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++)
			dvfs_info->pixel_size[0] += pixel_size[g_idx];
		freq = 650000000;
		dvfs_info->freq = freq;
	} else if (isSet == 0) {
		for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++)
			dvfs_info->pixel_size[0] -= pixel_size[g_idx];
		if (dvfs_info->pixel_size[0] == 0) {
			freq = 0;
			dvfs_info->freq = freq;
		}
	}
	#endif

}

#endif

