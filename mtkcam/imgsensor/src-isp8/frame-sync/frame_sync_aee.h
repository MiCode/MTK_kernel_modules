/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef _FRAME_SYNC_AEE_H
#define _FRAME_SYNC_AEE_H


#ifndef FS_UT
#include <aee.h>

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#define FS_WRAP_AEE_EXCEPTION(module, msg)				\
	aee_kernel_exception_api(__FILE__, __LINE__,			\
				 DB_OPT_DEFAULT | DB_OPT_FTRACE,	\
				 module, msg)

#else
#define FS_WRAP_AEE_EXCEPTION(module, msg)	\
	WARN_ON(1, "<%s:%d> %s: %s\n", __FILE__, __LINE__, module, msg)

#endif

#else
#define FS_WRAP_AEE_EXCEPTION(module, msg)
#endif

#endif
