// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_DEBUG_H__
#define __MTK_PLATFORM_DEBUG_H__

void mtk_common_debug_dump_status(void);
void mtk_common_gpu_fence_debug_dump(int fd, int pid, int type);

extern void (*mtk_gpu_fence_debug_dump_fp)(int fd, int pid, int type);

#endif /* __MTK_PLATFORM_DEBUG_H__ */
