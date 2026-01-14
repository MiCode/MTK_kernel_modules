// SPDX-License-Identifier: GPL-2.0
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
int monitor_task_scene_change(int task_id, int scene_mode);

extern void set_task_basic_vip(int pid);
extern void unset_task_basic_vip(int pid);

#endif