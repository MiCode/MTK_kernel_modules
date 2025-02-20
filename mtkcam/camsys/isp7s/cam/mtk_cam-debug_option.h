/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_CAM_DEBUG_OPTION_H
#define __MTK_CAM_DEBUG_OPTION_H

#include <linux/compiler.h>

/*
 * To debug format/crop related
 */
#define CAM_DEBUG_V4L2		0
#define CAM_DEBUG_V4L2_TRY	1
#define CAM_DEBUG_V4L2_EVENT	2
#define CAM_DEBUG_IPI_BUF	3

#define CAM_DEBUG_RAW_INT	4

#define CAM_DEBUG_CTRL		8
#define CAM_DEBUG_JOB		9
#define CAM_DEBUG_STATE		10

#define CAM_DEBUG_AA        31

unsigned int cam_debug_opts(void);

static inline bool cam_debug_enabled(unsigned int type)
{
	return cam_debug_opts() & (1U << type);
}

#define CAM_DEBUG_ENABLED(type) \
	unlikely(cam_debug_enabled(CAM_DEBUG_ ## type))

#endif /* __MTK_CAM_DEBUG_OPTION_H */
