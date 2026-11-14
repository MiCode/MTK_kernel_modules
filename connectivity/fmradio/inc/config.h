/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 MediaTek Inc.
 *
 */


#ifndef CONFIG_MTK_WCN_ARM64
#if IS_ENABLED(CONFIG_ARM64)
#define CONFIG_MTK_WCN_ARM64
#endif
#endif /* CONFIG_MTK_WCN_ARM64 */

#ifndef WMT_IDC_SUPPORT
#if IS_ENABLED(CONFIG_MTK_CONN_LTE_IDC_SUPPORT)
#define WMT_IDC_SUPPORT 1
#else
#define WMT_IDC_SUPPORT 0
#endif
#endif /* WMT_IDC_SUPPORT */

#ifndef CONFIG_MTK_USER_BUILD
#ifdef TARGET_BUILD_VARIANT_USER
#define CONFIG_MTK_USER_BUILD 1
#else
#define CONFIG_MTK_USER_BUILD 0
#endif
#endif /* CONFIG_MTK_USER_BUILD */
