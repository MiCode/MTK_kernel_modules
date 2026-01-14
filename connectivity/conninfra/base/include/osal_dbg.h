/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _OSAL_DBG_H_
#define _OSAL_DBG_H_

#include <linux/param.h>

void osal_dbg_kernel_exception(const char *module, const char *msg, ...);

void osal_dbg_common_exception_api(const char *assert_type, const int *log,
			int log_size, const int *phy, int phy_size,
			const char *detail, const int db_opt);

#endif /* _OSAL_DBG_H_ */
