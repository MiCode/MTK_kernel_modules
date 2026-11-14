/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#ifndef _WMT_DEF_H_
#define _WMT_DEF_H_

#if IS_ENABLED(CONFIG_ARM64)
#define CONFIG_MTK_WCN_ARM64
#endif

#if IS_ENABLED(CONFIG_CONN_WMT_DEBUG)
#define WMT_DBG_SUPPORT 1
#else
#define WMT_DBG_SUPPORT 0
#endif

#if IS_ENABLED(CONFIG_DEVICE_MODULES_MTK_DEVAPC)
#define WMT_DEVAPC_DBG_SUPPORT 1
#else
#define WMT_DEVAPC_DBG_SUPPORT 0
#endif

#if IS_ENABLED(CONFIG_MTK_CONN_LTE_IDC_SUPPORT)
#define WMT_IDC_SUPPORT 1
#else
#define WMT_IDC_SUPPORT 0
#endif

#endif /* _WMT_DEF_H_ */
