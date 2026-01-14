/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef C2PS_COMMON_INCLUDE_C2PS_USEDEXT_H_
#define C2PS_COMMON_INCLUDE_C2PS_USEDEXT_H_

extern int (*c2ps_notify_init_fp)(
	int cfg_camfps, int max_uclamp_cluster0,
	int max_uclamp_cluster1, int max_uclamp_cluster2);
extern int (*c2ps_notify_uninit_fp)(void);
extern int (*c2ps_notify_add_task_fp)(
	u32 task_id, u32 task_target_time, u32 default_uclamp,
	int group_head, u32 task_group_target_time,
	bool is_vip_task, bool is_dynamic_tid,
	const char *task_name);
extern int (*c2ps_notify_task_start_fp)(int pid, int task_id);
extern int (*c2ps_notify_task_end_fp)(int pid, int task_id);
extern int (*c2ps_notify_vsync_fp)(void);
extern int (*c2ps_notify_camfps_fp)(int camfps);
extern int (*c2ps_notify_task_scene_change_fp)(int task_id, int scene_mode);
extern int (*c2ps_notify_task_single_shot_fp)(
	int *uclamp_max, int idle_rate_alert, int timeout,
	int *uclamp_max_placeholder1, int *uclamp_max_placeholder2,
	int *uclamp_max_placeholder3, bool resetParam);

#endif  // C2PS_COMMON_INCLUDE_C2PS_USEDEXT_H_
