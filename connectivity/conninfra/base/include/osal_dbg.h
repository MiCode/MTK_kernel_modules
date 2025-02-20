/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _OSAL_DBG_H_
#define _OSAL_DBG_H_

#include <linux/param.h>
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include "aee.h"
#endif

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#define osal_dbg_kernel_exception aee_kernel_exception
#else
#define osal_dbg_kernel_exception(module, msg...) WARN(1, msg)
#endif

void osal_dbg_common_exception_api(const char *assert_type, const int *log,
			int log_size, const int *phy, int phy_size,
			const char *detail, const int db_opt);

#endif /* _OSAL_DBG_H_ */
