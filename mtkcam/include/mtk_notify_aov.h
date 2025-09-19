/* SPDX-License-Identifier: GPL-2.0 */
//
// Copyright (c) 2018 MediaTek Inc.

#ifndef __MTK_NOTIFY_AOV_H__
#define __MTK_NOTIFY_AOV_H__

typedef int (*aov_notify)(struct platform_device *pdev,
					uint32_t notify,
					uint32_t status);

void aov_notify_register(aov_notify aov_notify_fn);

#endif /*__MTK_NOTIFY_AOV_H__*/
