/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

typedef void (*tracepoint_fp)(void *p, unsigned long long *regs, long id);
int register_tracepoint_callback(tracepoint_fp cb);
int unregister_tracepoint_callback(tracepoint_fp cb);
int is_feature_enabled(unsigned int feature);

enum feature_flags {
    FEATURE_FPSGO = 1 << 0,
    FEATURE_SBE   = 1 << 1,
    FEATURE_OFF = 1 << 3
};
