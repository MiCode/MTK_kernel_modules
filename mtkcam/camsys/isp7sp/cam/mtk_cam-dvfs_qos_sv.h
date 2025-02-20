/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_CAM_DVFS_QOS_SV_H
#define __MTK_CAM_DVFS_QOS_SV_H

/* type0: multiple smi out / type1: no multiple smi out */
enum SMI_SV_MERGE_PORT_ID {
	SMI_PORT_SV_CQI = 0,
	SMI_PORT_SV_DISP_WDMA_0,
	SMI_PORT_SV_TYPE1_NUM,
	SMI_PORT_SV_MDP_WDMA_0 = SMI_PORT_SV_TYPE1_NUM,
	SMI_PORT_SV_DISP_WDMA_1,
	SMI_PORT_SV_MDP_WDMA_1,
	SMI_PORT_SV_TYPE0_NUM,
	SMI_PORT_SV_NUM = SMI_PORT_SV_TYPE0_NUM,
};

#endif /* __MTK_CAM_DVFS_QOS_SV_H */
