/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _C2PS_MONITOR_C2PS_STAT_SELECTOR_H
#define _C2PS_MONITOR_C2PS_STAT_SELECTOR_H
#include "c2ps_common.h"


enum c2ps_env_status determine_cur_system_state(struct global_info *glb_info);
enum c2ps_env_status c2ps_stat_selector_v1(struct global_info *glb_info);

enum c2ps_stat_selector_mode : int
{
	C2PS_STAT_SELECTOR_STABLE = 0,
	C2PS_STAT_SELECTOR_V1
};

#endif
