/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef PLAT_LIBRARY_H
#define PLAT_LIBRARY_H

#include <asm/atomic.h>
#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irqflags.h>
#include <linux/jiffies.h>
#include <linux/math64.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/printk.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/version.h>

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
#include <connectivity_build_in_adapter.h>
#include <mtk_ccci_common.h>
#endif

#endif	/* PLAT_LIBRARY_H */
