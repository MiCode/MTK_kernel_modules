/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef C2PS_COMMON_INCLUDE_C2PS_USEDEXT_H_
#define C2PS_COMMON_INCLUDE_C2PS_USEDEXT_H_

extern int (*c2ps_notify_init_fp)(
	int cfg_camfps, int max_uclamp_cluster0,
	int max_uclamp_cluster1, int max_uclamp_cluster2,
	int ineff_cpu_ceiling_freq0,
	int ineff_cpu_ceiling_freq1, int ineff_cpu_ceiling_freq2,
	int lcore_mcore_um_ratio, int um_floor);
extern int (*c2ps_notify_uninit_fp)(void);
extern int (*c2ps_notify_add_task_fp)(
	u32 task_id, u32 task_target_time, u32 default_uclamp,
	int group_head, u32 task_group_target_time,
	bool is_vip_task, bool is_dynamic_tid,
	bool is_enable_dep_thread, const char *task_name);
extern int (*c2ps_notify_task_start_fp)(int pid, int task_id);
extern int (*c2ps_notify_task_end_fp)(int pid, int task_id);
extern int (*c2ps_notify_vsync_fp)(void);
extern int (*c2ps_notify_camfps_fp)(int camfps);
extern int (*c2ps_notify_task_scene_change_fp)(int task_id, int scene_mode);
extern int (*c2ps_notify_single_shot_control_fp)(
	int pid, int *uclamp_max, int idle_rate_alert, int vip_prior,
	u32 vip_throttle_time, int *uclamp_max_placeholder1,
	int *uclamp_max_placeholder2, int *uclamp_max_placeholder3,
	bool reset_param, bool set_task_idle_prefer,
	int *critical_task_ids, int *critical_task_uclamp, u32 util_margin,
	u32 um_placeholder1, u32 um_placeholder2, u32 um_placeholder3,
	bool enable_ineff_cpufreq, bool switch_um_idle_rate_mode,
	int reserved_1, int reserved_2, int reserved_3);
extern int (*c2ps_notify_single_shot_task_start_fp)(int pid, u32 uclamp);
extern int (*c2ps_notify_single_shot_task_end_fp)(int pid);
extern int (*c2ps_notify_anchor_fp)(int anc_id, bool register_fixed,
	u32 anchor_type, u32 anc_order, u32 order, u32 latency_spec, u32 jitter_spec);

#endif  // C2PS_COMMON_INCLUDE_C2PS_USEDEXT_H_
