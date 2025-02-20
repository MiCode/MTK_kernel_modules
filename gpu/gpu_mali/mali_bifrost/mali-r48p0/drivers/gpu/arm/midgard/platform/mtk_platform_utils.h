// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_UTILS_H__
#define __MTK_PLATFORM_UTILS_H__

// Note: Please do not include in the header file
#if IS_ENABLED(CONFIG_MALI_MTK_PREVENT_PRINTK_TOO_MUCH)
#undef dev_dbg
#define dev_dbg dev_vdbg
#endif /* CONFIG_MALI_MTK_PREVENT_PRINTK_TOO_MUCH */

#endif /* __MTK_PLATFORM_UTILS_H__ */

