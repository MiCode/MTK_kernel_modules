/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MET_POWER
#define MET_POWER

enum {
	_PM_QOS_RESERVED = 0,
	_PM_QOS_CPU_DMA_LATENCY,
	_PM_QOS_NETWORK_LATENCY,
	_PM_QOS_NETWORK_THROUGHPUT,
	_PM_QOS_MEMORY_BANDWIDTH,

	_PM_QOS_CPU_MEMORY_BANDWIDTH,
	_PM_QOS_GPU_MEMORY_BANDWIDTH,
	_PM_QOS_MM_MEMORY_BANDWIDTH,
	_PM_QOS_OTHER_MEMORY_BANDWIDTH,
	_PM_QOS_MM0_BANDWIDTH_LIMITER,
	_PM_QOS_MM1_BANDWIDTH_LIMITER,

	_PM_QOS_DDR_OPP,
	_PM_QOS_VCORE_OPP,
	_PM_QOS_SCP_VCORE_REQUEST,
	_PM_QOS_POWER_MODEL_DDR_REQUEST,
	_PM_QOS_POWER_MODEL_VCORE_REQUEST,
	_PM_QOS_VCORE_DVFS_FORCE_OPP,

	_PM_QOS_DISP_FREQ,
	_PM_QOS_MDP_FREQ,
	_PM_QOS_VDEC_FREQ,
	_PM_QOS_VENC_FREQ,
	_PM_QOS_IMG_FREQ,
	_PM_QOS_CAM_FREQ,
	_PM_QOS_VVPU_OPP,
	_PM_QOS_VMDLA_OPP,
	_PM_QOS_ISP_HRT_BANDWIDTH,
	_PM_QOS_APU_MEMORY_BANDWIDTH,
	/* insert new class ID */
	_PM_QOS_NUM_CLASSES,
};
/* Action requested to pm_qos_update_target */
enum _pm_qos_req_action {
	_PM_QOS_ADD_REQ,		/* Add a new request */
	_PM_QOS_UPDATE_REQ,	/* Update an existing request */
	_PM_QOS_REMOVE_REQ	/* Remove an existing request */
};

extern void pm_qos_update_request(int pm_qos_class, s32 value, char *owner);
extern void pm_qos_update_target(unsigned int action, int prev_value, int curr_value);

#endif	/* MET_DRV */

