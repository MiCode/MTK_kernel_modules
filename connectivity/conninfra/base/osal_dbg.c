// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/version.h>

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include "aee.h"
#endif

void osal_dbg_common_exception_api(
	const char *assert_type, const int *log,
	int log_size, const int *phy, int phy_size,
	const char *detail, const int db_opt)
{
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
	aed_common_exception_api(
		assert_type, log, log_size, phy, phy_size, detail, db_opt);
#endif
}

