// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include "../include/emi_mng.h"
#include "include/mt6886_emi.h"
#include "include/mt6886_pos.h"

struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6886_atf = {
	.consys_ic_emi_mpu_set_region_protection = consys_emi_mpu_set_region_protection_mt6886,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg_mt6886_atf,
	.consys_ic_emi_get_md_shared_emi = consys_emi_get_md_shared_emi_mt6886,
};


