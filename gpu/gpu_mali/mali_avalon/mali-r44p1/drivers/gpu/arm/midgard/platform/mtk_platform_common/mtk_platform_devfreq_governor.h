// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_GPU_DEVFREQ_GOVERNOR_H__
#define __MTK_GPU_DEVFREQ_GOVERNOR_H__

#include <linux/devfreq.h>

/* DEVFREQ governor name */
#define MTK_GPU_DEVFREQ_GOVERNOR_DUMMY "dummy"

extern struct devfreq_governor mtk_devfreq_governor_dummy;

void mtk_devfreq_governor_update_profile(struct devfreq_dev_profile *dp);
int mtk_devfreq_governor_init(struct kbase_device *kbdev);
int mtk_devfreq_governor_term(struct kbase_device *kbdev);

#endif /* __MTK_GPU_DEVFREQ_GOVERNOR_H__ */
