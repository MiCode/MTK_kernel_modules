/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <linux/types.h>

typedef void (*heavy_fp)(int jank, int tgid);
typedef void (*ux_heavy_fp)(int jank, int tgid, int pid, unsigned long long frame_id);
int register_heavy_callback(heavy_fp cb);
int unregister_heavy_callback(heavy_fp cb);
int register_jank_ux_callback(ux_heavy_fp cb);
int unregister_jank_ux_callback(ux_heavy_fp cb);
void enable_ux_jank_detection(bool enable, const char *info, int tgid, int pid);
void sbe_frame_hint(int frame_start, int perf_index, int capacity_area, int buffer_count, unsigned long long frame_id);


