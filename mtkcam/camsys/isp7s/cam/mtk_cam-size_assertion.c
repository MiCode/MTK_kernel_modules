// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2022 MediaTek Inc.

#include "mtk_cam.h"

#define PRINT_SIZE(_st)	\
	static int (*b)[sizeof(_st)] = 1

#define SIZE_STATIC_ASSERT(_st, _size) \
static_assert(sizeof(_st) < _size, "check if need to update size")

SIZE_STATIC_ASSERT(struct mtk_cam_request, 2 * 1024);
//PRINT_SIZE(struct mtk_cam_request);
SIZE_STATIC_ASSERT(struct mtk_cam_ctx, 10 * 1024);
//PRINT_SIZE(struct mtk_cam_ctx);
SIZE_STATIC_ASSERT(struct mtk_cam_job, 2 * 1024);
//PRINT_SIZE(struct mtk_cam_job);
