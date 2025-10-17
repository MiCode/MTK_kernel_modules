/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _C2PS_MONITOR_C2PS_MONITOR_H
#define _C2PS_MONITOR_C2PS_MONITOR_H

#include "c2ps_common.h"

int monitor_task_start(int pid, int task_id);
int monitor_task_end(int pid, int task_id);
int monitor_vsync(u64 ts);
int monitor_camfps(int camfps);
void monitor_system_info(void);
int monitor_task_scene_change(int task_id, int scene_mode);
int monitor_anchor(
	int anc_id, u32 order, bool register_fixed, u32 anc_type, u32 anc_order,
	u64 cur_ts, u32 latency_spec, u32 jitter_spec);

#endif