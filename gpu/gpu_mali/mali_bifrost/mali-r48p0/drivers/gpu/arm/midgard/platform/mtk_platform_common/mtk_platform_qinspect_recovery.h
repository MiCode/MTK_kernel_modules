/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_QINSPECT_RECOVERY_H__
#define __MTK_PLATFORM_QINSPECT_RECOVERY_H__
#include <platform/mtk_platform_common/mtk_platform_qinspect.h>

extern struct mutex recovery_lock;

void mtk_qinspect_recovery(struct kbase_context *kctx,
	enum mtk_qinspect_queue_type queue_type, void *queue);

#endif /* __MTK_PLATFORM_QINSPECT_RECOVERY_H__ */
