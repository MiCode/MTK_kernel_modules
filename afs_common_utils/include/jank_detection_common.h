/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <linux/types.h>

typedef void (*heavy_fp)(int jank, int pid);
int register_heavy_callback(heavy_fp cb);
int unregister_heavy_callback(heavy_fp cb);
int register_jank_ux_callback(heavy_fp cb);
int unregister_jank_ux_callback(heavy_fp cb);
void enable_ux_jank_detection(bool enable, const char *info);

