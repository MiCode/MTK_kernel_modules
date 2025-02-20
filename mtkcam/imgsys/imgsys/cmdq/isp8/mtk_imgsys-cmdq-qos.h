/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_CMDQ_QOS_8_H_
#define _MTK_IMGSYS_CMDQ_QOS_8_H_

enum SMI_MONITOR_STATE {
	SMI_MONITOR_IDLE_STATE = 0,
	SMI_MONITOR_START_STATE,
	SMI_MONITOR_ACQUIRE_STATE,
	SMI_MONITOR_STOP_STATE,
};

#if DVFS_QOS_READY
void mtk_imgsys_mmqos_init_plat8(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_mmqos_uninit_plat8(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_mmqos_set_by_scen_plat8(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				bool isSet);
void mtk_imgsys_mmqos_reset_plat8(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_mmqos_bw_cal_plat8(struct mtk_imgsys_dev *imgsys_dev,
				void *smi_port, uint32_t hw_comb,
				uint32_t port_st, uint32_t port_num, uint32_t port_id);
void mtk_imgsys_mmqos_ts_cal_plat8(struct mtk_imgsys_dev *imgsys_dev,
				struct mtk_imgsys_cb_param *cb_param, uint32_t hw_comb);
void mtk_imgsys_mmqos_monitor_plat8(struct mtk_imgsys_dev *imgsys_dev, u32 state);


#endif

#endif /* _MTK_IMGSYS_CMDQ_QOS_8_H_ */
