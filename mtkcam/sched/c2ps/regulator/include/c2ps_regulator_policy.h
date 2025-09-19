/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef C2PS_REGULATOR_INCLUDE_C2PS_REGULATOR_POLICY_H_
#define C2PS_REGULATOR_INCLUDE_C2PS_REGULATOR_POLICY_H_

#include "c2ps_common.h"
#include <linux/sched/cputime.h>
#include <sched/sched.h>

// critical task policy
void c2ps_regulator_policy_fix_uclamp(struct regulator_req *req);
void c2ps_regulator_policy_simple(struct regulator_req *req);
void c2ps_regulator_policy_debug_uclamp(struct regulator_req *req);

// background uclamp max policy
void c2ps_regulator_bgpolicy_simple(struct regulator_req *req);
// background um policy
void c2ps_regulator_bgpolicy_um_stable_default(struct regulator_req *req);
void c2ps_regulator_bgpolicy_um_stable(struct regulator_req *req);
void c2ps_regulator_bgpolicy_um_transient(struct regulator_req *req);


enum c2ps_regulator_mode : int
{
	// Uclamp control
	C2PS_REGULATOR_MODE_FIX = 0,
	C2PS_REGULATOR_MODE_SIMPLE,
	C2PS_REGULATOR_MODE_DEBUG,
	C2PS_REGULATOR_BGMODE_SIMPLE,

	// Util margin control
	C2PS_REGULATOR_BGMODE_UM_STABLE_DEFAULT,
	C2PS_REGULATOR_BGMODE_UM_STABLE,
	C2PS_REGULATOR_BGMODE_UM_TRANSIENT,
};

#endif  // C2PS_REGULATOR_INCLUDE_C2PS_REGULATOR_POLICY_H_
