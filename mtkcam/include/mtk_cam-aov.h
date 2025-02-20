/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_AOV_H
#define __MTK_CAM_AOV_H

enum AOV_DEINIT_TYPE {
	DEINIT_NORMAL = 0,
	DEINIT_ABNORMAL_USR_FD_KILL,
	DEINIT_ABNORMAL_SCP_STOP,
};

enum AOV_INIT_TYPE {
	INIT_NORMAL = 0,
	INIT_ABNORMAL_SCP_READY,
};

extern int mtk_cam_seninf_s_aov_param(unsigned int sensor_id,
	void *param, enum AOV_INIT_TYPE aov_seninf_init_type);

extern int mtk_cam_seninf_aov_runtime_suspend(unsigned int sensor_id);

extern int mtk_cam_seninf_aov_runtime_resume(unsigned int sensor_id,
	enum AOV_DEINIT_TYPE aov_seninf_deinit_type);

extern int mtk_cam_seninf_aov_reset_sensor(unsigned int sensor_id);
extern int mtk_cam_seninf_aov_sensor_set_mclk(unsigned int sensor_id, bool enable);

#endif /* __MTK_CAM_AOV_H */
