// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_DEBUG_H__
#define __MTK_PLATFORM_DEBUG_H__

#include <mali_kbase_defs.h>

void mtk_common_debug_dump_status(void);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
int mtk_common_debug_init(void);
int mtk_common_debug_term(void);
bool mtk_common_debug_logbuf_is_empty(struct mtk_debug_logbuf *logbuf);
bool mtk_common_debug_logbuf_is_full(struct mtk_debug_logbuf *logbuf);
void mtk_common_debug_logbuf_clear(struct mtk_debug_logbuf *logbuf);
void mtk_common_debug_logbuf_print(struct mtk_debug_logbuf *logbuf, const char *fmt, ...);
void mtk_common_debug_logbuf_dump(struct mtk_debug_logbuf *logbuf, struct seq_file *seq);
#endif
void mtk_common_gpu_fence_debug_dump(int fd, int pid, int type, int timeouts);

extern void (*mtk_gpu_fence_debug_dump_fp)(int fd, int pid, int type, int timeouts);

#endif /* __MTK_PLATFORM_DEBUG_H__ */
