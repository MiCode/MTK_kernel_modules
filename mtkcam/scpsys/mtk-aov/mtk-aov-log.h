/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MTK_AOV_LOG_H
#define MTK_AOV_LOG_H

#include <linux/printk.h>

#define AOV_LOG(fmt, arg...) pr_info(fmt, ##arg)

#define AOV_DEBUG_LOG(aov_log_enable, fmt, arg...) \
do { \
	if (aov_log_enable) \
		AOV_LOG(fmt, ##arg); \
} while (0)

#endif /* MTK_AOV_LOG_H */
