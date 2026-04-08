// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_device.h>

int imgsys_dbg_en;
module_param(imgsys_dbg_en, int, 0644);

int imgsys_slc_dbg_en;
module_param(imgsys_slc_dbg_en, int, 0644);

bool imgsys_dbg_enable(void)
{
	return imgsys_dbg_en;
}
EXPORT_SYMBOL(imgsys_dbg_enable);

bool imgsys_slc_dbg_enable(void)
{
	return imgsys_slc_dbg_en;
}
EXPORT_SYMBOL(imgsys_slc_dbg_enable);