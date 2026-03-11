// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */
#include <dt-bindings/interconnect/mtk,mmqos.h>
#include "mtk_imgsys-engine.h"
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-cmdq-plat.h"
#include "mtk_imgsys-cmdq-ext.h"
#include "mtk_imgsys-cmdq-qos.h"
#include "mtk_imgsys-trace.h"

#if DVFS_QOS_READY
#include "mtk-smi-dbg.h"
#endif

enum {
	SMI_COMMON_ID_31 = 0,
	SMI_COMMON_ID_32
};

enum {
	SMI_READ = 0,
	SMI_WRITE
};

#define IMGSYS_QOS_SYNC_OWNER	(0x412d454d5f53)
#define IMGSYS_QOS_MAX_PERF	(MTK_MMQOS_MAX_SMI_FREQ_BW >> 1)

#define IMGSYS_QOS_UPDATE_FREQ	4
#define IMGSYS_QOS_BLANK_INT	100
#define IMGSYS_QOS_FACTOR		5

#if DVFS_QOS_READY
static struct mtk_imgsys_qos_path imgsys_qos_path[IMGSYS_M4U_PORT_MAX] = {
	{NULL, "l10_common_r_0", 0},
	{NULL, "l22_common_w_0", 0},
	{NULL, "l9_common_r_1", 0},
	{NULL, "l15_common_w_1", 0}
};
#endif


#if DVFS_QOS_READY
void mtk_imgsys_mmqos_init_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	struct mtk_imgsys_qos *qos_info = &imgsys_dev->qos_info;
	//struct icc_path *path;
	int idx = 0;
	u32 ret = 0;

	memset((void *)qos_info, 0x0, sizeof(struct mtk_imgsys_qos));
	qos_info->dev = imgsys_dev->dev;
	qos_info->qos_path = imgsys_qos_path;

#ifndef CONFIG_FPGA_EARLY_PORTING
	if (of_property_read_u32(qos_info->dev->of_node,
		"mediatek,imgsys-qos-sc-motr", &ret) != 0) {
		dev_info(qos_info->dev, "mmqos monitor is not exist\n");
	} else {
		qos_info->sc_monitor = ret;

		if (of_property_read_u32(qos_info->dev->of_node,
			"mediatek,imgsys-qos-sc-nums", &ret) != 0) {
			dev_info(qos_info->dev, "mmqos sc num is not exist\n");
		} else {
			qos_info->sc_nums = ret;
			dev_info(qos_info->dev, "mmqos monitor: %d, sc_nums: %d\n",
				 qos_info->sc_monitor, qos_info->sc_nums);

			if (qos_info->sc_nums > 0 &&
				qos_info->sc_nums < MTK_IMGSYS_QOS_SC_MAX_ID) {
				for (idx = 0; idx < qos_info->sc_nums; idx++) {
					if (of_property_read_u32_index(qos_info->dev->of_node,
					"mediatek,imgsys-qos-sc-id", idx, &ret) == 0)
						qos_info->sc_id[idx] = ret;
					dev_info(qos_info->dev, "mmqos sc_id[%d]: %d\n",
						idx, qos_info->sc_id[idx]);
				}
			}
		}
	}

	for (idx = 0; idx < IMGSYS_M4U_PORT_MAX; idx++) {
		qos_info->qos_path[idx].path =
			of_mtk_icc_get(qos_info->dev, qos_info->qos_path[idx].dts_name);
		qos_info->qos_path[idx].bw = 0;
		dev_info(qos_info->dev, "[%s] idx=%d, path=%p, name=%s, bw=%llu\n",
			__func__, idx,
			qos_info->qos_path[idx].path,
			qos_info->qos_path[idx].dts_name,
			qos_info->qos_path[idx].bw);
	}
#endif
	mtk_imgsys_mmqos_reset_plat7sp(imgsys_dev);
}

void mtk_imgsys_mmqos_uninit_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	struct mtk_imgsys_qos *qos_info = &imgsys_dev->qos_info;
	int idx = 0;

	for (idx = 0; idx < IMGSYS_M4U_PORT_MAX; idx++) {
		if (IS_ERR_OR_NULL(qos_info->qos_path[idx].path)) {
            if (imgsys_cmdq_dbg_enable_plat7sp())
			dev_dbg(qos_info->dev, "[%s] path of idx(%d) is NULL\n", __func__, idx);
			continue;
		}
        if (imgsys_cmdq_dbg_enable_plat7sp()) {
		dev_dbg(qos_info->dev, "[%s] idx=%d, path=%p, bw=%llu\n",
			__func__, idx,
			qos_info->qos_path[idx].path,
			qos_info->qos_path[idx].bw);
		qos_info->qos_path[idx].bw = 0;
        }
#ifndef CONFIG_FPGA_EARLY_PORTING
		mtk_icc_set_bw(qos_info->qos_path[idx].path, 0, 0);
#endif
	}
}


void mtk_imgsys_mmqos_monitor_plat7sp(struct mtk_imgsys_dev *imgsys_dev, u32 state)
{
	struct mtk_imgsys_qos *qos_info = &imgsys_dev->qos_info;

	u32 common_port[MAX_MON_REQ] = { 0 };
	u32 flag[MAX_MON_REQ] = { 0 };
	u32 bw0[MAX_MON_REQ] = { 0 };
	u32 bw1[MAX_MON_REQ] = { 0 };
	u32 sbc_lid0 = 0;
	u32 sbc_lid1 = 0;

	if (unlikely(!qos_info->sc_monitor))
		return;

	sbc_lid0 = qos_info->sc_id[0];
	sbc_lid1 = qos_info->sc_id[1];

	common_port[0] = 0;
	common_port[1] = 0;
	common_port[2] = 0;
	common_port[3] = 0;

	flag[0] = 1;
	flag[1] = 2;
	flag[2] = 1;
	flag[3] = 2;

	if (state == SMI_MONITOR_STOP_STATE ||
	    state == SMI_MONITOR_ACQUIRE_STATE) {
#ifndef CONFIG_FPGA_EARLY_PORTING
		smi_monitor_stop(NULL, sbc_lid0, bw0, SMI_BW_MET);
		if (qos_info->sc_monitor > 1)
			smi_monitor_stop(NULL, sbc_lid1, bw1, SMI_BW_MET);
#endif
	}

	if (state == SMI_MONITOR_START_STATE || state == SMI_MONITOR_STOP_STATE) {
		qos_info->bw_total[SMI_COMMON_ID_31][SMI_READ] = 0;
		qos_info->bw_total[SMI_COMMON_ID_31][SMI_WRITE] = 0;
		qos_info->bw_total[SMI_COMMON_ID_32][SMI_READ] = 0;
		qos_info->bw_total[SMI_COMMON_ID_32][SMI_WRITE] = 0;
	} else if (state == SMI_MONITOR_ACQUIRE_STATE) {
		if (qos_info->req_cnt == 0) { //Initial setting
			qos_info->bw_total[SMI_COMMON_ID_31][SMI_READ] = IMGSYS_QOS_MAX_PERF >> 6;
			qos_info->bw_total[SMI_COMMON_ID_31][SMI_WRITE] = IMGSYS_QOS_MAX_PERF >> 6;
			qos_info->bw_total[SMI_COMMON_ID_32][SMI_READ] = IMGSYS_QOS_MAX_PERF >> 6;
			qos_info->bw_total[SMI_COMMON_ID_32][SMI_WRITE] = IMGSYS_QOS_MAX_PERF >> 6;
		} else {
			qos_info->bw_total[SMI_COMMON_ID_31][SMI_READ] = bw0[0] >> 20;
			qos_info->bw_total[SMI_COMMON_ID_31][SMI_WRITE] = bw0[1] >> 20;

			if (qos_info->sc_monitor > 1) {
				qos_info->bw_total[SMI_COMMON_ID_32][SMI_READ] = bw1[0] >> 20;
				qos_info->bw_total[SMI_COMMON_ID_32][SMI_WRITE] = bw1[1] >> 20;
			} else {
				qos_info->bw_total[SMI_COMMON_ID_32][SMI_READ] = bw0[2] >> 20;
				qos_info->bw_total[SMI_COMMON_ID_32][SMI_WRITE] = bw0[3] >> 20;
			}
		}
	}

	if (state == SMI_MONITOR_START_STATE ||
	    state == SMI_MONITOR_ACQUIRE_STATE) {
#ifndef CONFIG_FPGA_EARLY_PORTING
		smi_monitor_start(qos_info->dev, sbc_lid0, common_port, flag, SMI_BW_MET);
		if (qos_info->sc_monitor > 1)
			smi_monitor_start(qos_info->dev, sbc_lid1, common_port, flag, SMI_BW_MET);
#endif
	}

}

void mtk_imgsys_mmqos_set_by_scen_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				bool isSet)
{
	struct mtk_imgsys_qos *qos_info = &imgsys_dev->qos_info;
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
	u32 hw_comb = 0;
	u64 pixel_sz = 0;
	u64 cur_interval = 0;
	u32 fps = 0;
	u64 frame_duration = 0;
	u32 frm_num = 0;
	u64 bw_final[4] = {0};
	u32 sidx = 0;
	const u32 step = imgsys_qos_update_freq;

	frm_num = frm_info->total_frmnum;
	hw_comb = frm_info->user_info[frm_num-1].hw_comb;
	pixel_sz = frm_info->user_info[frm_num-1].pixel_bw;
	fps = frm_info->fps;
	sidx = frm_info->user_info[0].subfrm_idx;

	if (is_stream_off == 0 && isSet == 1) {
		if (imgsys_qos_dbg_enable_plat7sp())
			dev_info(qos_info->dev,
				 "imgsys_qos: frame_no:%d req_cnt:%lu fps:%d vss:%d\n",
				 frm_info->frame_no, qos_info->req_cnt,
				 fps, dvfs_info->vss_task_cnt);

		if (dvfs_info->vss_task_cnt > 0 &&
		    qos_info->qos_path[IMGSYS_COMMON_0_R].bw < IMGSYS_QOS_MAX_PERF) {
			qos_info->qos_path[IMGSYS_COMMON_0_R].bw = IMGSYS_QOS_MAX_PERF;
			qos_info->qos_path[IMGSYS_COMMON_0_W].bw = IMGSYS_QOS_MAX_PERF;
			qos_info->qos_path[IMGSYS_COMMON_1_R].bw = IMGSYS_QOS_MAX_PERF;
			qos_info->qos_path[IMGSYS_COMMON_1_W].bw = IMGSYS_QOS_MAX_PERF;

			bw_final[0] = qos_info->qos_path[IMGSYS_COMMON_0_R].bw;
			bw_final[1] = qos_info->qos_path[IMGSYS_COMMON_0_W].bw;
			bw_final[2] = qos_info->qos_path[IMGSYS_COMMON_1_R].bw;
			bw_final[3] = qos_info->qos_path[IMGSYS_COMMON_1_W].bw;

			bw_final[0] = (bw_final[0] * imgsys_qos_factor) >> 2;
			bw_final[1] = (bw_final[1] * imgsys_qos_factor) >> 2;
			bw_final[2] = (bw_final[2] * imgsys_qos_factor) >> 2;
			bw_final[3] = (bw_final[3] * imgsys_qos_factor) >> 2;
			if (imgsys_qos_dbg_enable_plat7sp())
				dev_info(qos_info->dev,
					"imgsys_qos: frame_no:%d-sc0_r-%llu sc0_w-%llu, sc1_r-%llu sc0_w-%llu\n",
					frm_info->frame_no,
					bw_final[0], bw_final[1], bw_final[2], bw_final[3]);

			IMGSYS_CMDQ_SYSTRACE_BEGIN("SetQos");
#ifndef CONFIG_FPGA_EARLY_PORTING
			mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_0_R].path,
					MBps_to_icc(bw_final[0]),
					0);
			mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_0_W].path,
					MBps_to_icc(bw_final[1]),
					0);
			mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_1_R].path,
					MBps_to_icc(bw_final[2]),
					0);
			mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_1_W].path,
					MBps_to_icc(bw_final[3]),
					0);
#endif
			IMGSYS_CMDQ_SYSTRACE_END();
		}

		if (fps) {
			frame_duration = 1000 / (fps << 1);
			cur_interval = (ktime_get_boottime_ns()/1000000) - qos_info->time_prev_req;

			if (frm_info->frm_owner == IMGSYS_QOS_SYNC_OWNER && sidx == 0 &&
			    frm_info->frame_no == 0) {
				qos_info->req_cnt = 0;
				qos_info->avg_cnt = 0;
			}

			if (cur_interval >= frame_duration &&
			    frm_info->frame_no >= qos_info->req_cnt) {

				mtk_imgsys_mmqos_monitor_plat7sp(imgsys_dev,
								SMI_MONITOR_ACQUIRE_STATE);
				qos_info->req_cnt = frm_info->frame_no + 1;
				qos_info->time_prev_req = ktime_get_boottime_ns()/1000000;

				bw_final[0] = (qos_info->bw_total[0][0] * fps);
				bw_final[1] = (qos_info->bw_total[0][1] * fps);
				bw_final[2] = (qos_info->bw_total[1][0] * fps);
				bw_final[3] = (qos_info->bw_total[1][1] * fps);

				if (imgsys_qos_dbg_enable_plat7sp())
					dev_info(qos_info->dev,
						 "imgsys_qos: ori frame_no:%d-sc0_r-%llu sc0_w-%llu, sc1_r-%llu sc0_w-%llu\n",
						 frm_info->frame_no,
						 bw_final[0], bw_final[1],
						 bw_final[2], bw_final[3]);

				qos_info->bw_avg[0][0] += bw_final[0];
				qos_info->bw_avg[0][1] += bw_final[1];
				qos_info->bw_avg[1][0] += bw_final[2];
				qos_info->bw_avg[1][1] += bw_final[3];
				qos_info->avg_cnt++;

				if (dvfs_info->vss_task_cnt == 0 &&
					((qos_info->avg_cnt >= step) ||
					 (qos_info->req_cnt <= 1))) {
					/* unit is MB/s */
					bw_final[0] = qos_info->bw_avg[0][0] / qos_info->avg_cnt;
					bw_final[1] = qos_info->bw_avg[0][1] / qos_info->avg_cnt;
					bw_final[2] = qos_info->bw_avg[1][0] / qos_info->avg_cnt;
					bw_final[3] = qos_info->bw_avg[1][1] / qos_info->avg_cnt;

					if (imgsys_qos_dbg_enable_plat7sp())
						dev_info(qos_info->dev,
							 "imgsys_qos: avg frame_no:%d-sc0_r-%llu sc0_w-%llu, sc1_r-%llu sc0_w-%llu\n",
							 frm_info->frame_no,
							 bw_final[0], bw_final[1],
							 bw_final[2], bw_final[3]);

					qos_info->qos_path[IMGSYS_COMMON_0_R].bw = bw_final[0];
					qos_info->qos_path[IMGSYS_COMMON_0_W].bw = bw_final[1];
					qos_info->qos_path[IMGSYS_COMMON_1_R].bw = bw_final[2];
					qos_info->qos_path[IMGSYS_COMMON_1_W].bw = bw_final[3];

					bw_final[0] = (bw_final[0] * imgsys_qos_factor) >> 2;
					bw_final[1] = (bw_final[1] * imgsys_qos_factor) >> 2;
					bw_final[2] = (bw_final[2] * imgsys_qos_factor) >> 2;
					bw_final[3] = (bw_final[3] * imgsys_qos_factor) >> 2;

					if (imgsys_qos_dbg_enable_plat7sp())
						dev_info(qos_info->dev,
							 "imgsys_qos: frame_no:%d-sc0_r-%llu sc0_w-%llu, sc1_r-%llu sc0_w-%llu\n",
							 frm_info->frame_no,
							 bw_final[0], bw_final[1],
							 bw_final[2], bw_final[3]);

					IMGSYS_CMDQ_SYSTRACE_BEGIN("SetQos");
#ifndef CONFIG_FPGA_EARLY_PORTING
					mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_0_R].path,
							MBps_to_icc(bw_final[0]),
							0);
					mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_0_W].path,
							MBps_to_icc(bw_final[1]),
							0);
					mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_1_R].path,
							MBps_to_icc(bw_final[2]),
							0);
					mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_1_W].path,
							MBps_to_icc(bw_final[3]),
							0);
#endif
					IMGSYS_CMDQ_SYSTRACE_END();
					qos_info->bw_avg[0][0] = 0;
					qos_info->bw_avg[0][1] = 0;
					qos_info->bw_avg[1][0] = 0;
					qos_info->bw_avg[1][1] = 0;
					qos_info->avg_cnt = 0;
				}

			}
		}
	}
}

void mtk_imgsys_mmqos_reset_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	u32 dvfs_idx = 0, qos_idx = 0;
	struct mtk_imgsys_qos *qos_info = NULL;

	qos_info = &imgsys_dev->qos_info;

	qos_info->qos_path[IMGSYS_COMMON_0_R].bw = 0;
	qos_info->qos_path[IMGSYS_COMMON_0_W].bw = 0;

	qos_info->qos_path[IMGSYS_COMMON_1_R].bw = 0;
	qos_info->qos_path[IMGSYS_COMMON_1_W].bw = 0;

#ifndef CONFIG_FPGA_EARLY_PORTING
	mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_0_R].path, 0, 0);
	mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_0_W].path, 0, 0);
	mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_1_R].path, 0, 0);
	mtk_icc_set_bw(qos_info->qos_path[IMGSYS_COMMON_1_W].path, 0, 0);
#endif

	for (dvfs_idx = 0; dvfs_idx < MTK_IMGSYS_DVFS_GROUP; dvfs_idx++) {
		for (qos_idx = 0; qos_idx < MTK_IMGSYS_QOS_GROUP; qos_idx++) {
			qos_info->bw_total[dvfs_idx][qos_idx] = 0;
			qos_info->bw_avg[dvfs_idx][qos_idx] = 0;
		}
		qos_info->ts_total[dvfs_idx] = 0;
	}
	qos_info->req_cnt = 0;
	qos_info->avg_cnt = 0;
	qos_info->time_prev_req = 0;
	qos_info->isIdle = 0;

	if (imgsys_qos_update_freq == 0)
		imgsys_qos_update_freq = IMGSYS_QOS_UPDATE_FREQ;
	if (imgsys_qos_blank_int == 0)
		imgsys_qos_blank_int = IMGSYS_QOS_BLANK_INT;
	if (imgsys_qos_factor == 0)
		imgsys_qos_factor = IMGSYS_QOS_FACTOR;

}


void mtk_imgsys_mmqos_bw_cal_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
					void *smi_port, uint32_t hw_comb,
					uint32_t port_st, uint32_t port_num, uint32_t port_id)
{
	struct mtk_imgsys_qos *qos_info = NULL;
	struct smi_port_t *smi = NULL;
	uint32_t port_idx = 0, g_idx = 0;

	qos_info = &imgsys_dev->qos_info;
	smi = (struct smi_port_t *)smi_port;
	for (port_idx = port_st; port_idx < (port_num + port_st); port_idx++)
		for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++)
			if (hw_comb & dvfs_group[g_idx].g_hw)
				qos_info->bw_total[g_idx][port_id] += smi[port_idx-port_st].portbw;
}

void mtk_imgsys_mmqos_ts_cal_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
				struct mtk_imgsys_cb_param *cb_param, uint32_t hw_comb)
{
	struct mtk_imgsys_qos *qos_info = NULL;
	uint32_t g_idx = 0;
	u64 ts_hw = 0;

	if (is_stream_off == 0) {
		qos_info = &imgsys_dev->qos_info;
		ts_hw = cb_param->cmdqTs.tsCmdqCbStart-cb_param->cmdqTs.tsFlushStart;
		for (g_idx = 0; g_idx < MTK_IMGSYS_DVFS_GROUP; g_idx++)
			if (hw_comb & dvfs_group[g_idx].g_hw)
				qos_info->ts_total[g_idx] += ts_hw;
	}
}

#endif

