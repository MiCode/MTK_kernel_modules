// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/pm_runtime.h>

#include "osal.h"
#include "connv3_hw.h"
#include "coredump/connv3_dump_mng.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define PLATFORM_SOC_CHIP		0x6985
#define CONN_HW_VER			0x6639
#define CONN_ADIE_ID			0x6639
/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

u32 connv3_soc_get_chipid_mt6985(void);
static u32 connv3_get_adie_chipid_mt6985(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct connv3_hw_ops_struct g_connv3_hw_ops_mt6985 = {
	.connsys_plt_get_chipid = connv3_soc_get_chipid_mt6985,
	.connsys_plt_get_adie_chipid = connv3_get_adie_chipid_mt6985,
};

const struct connv3_coredump_platform_ops g_connv3_dump_ops_mt6985 = {
	.connv3_dump_plt_get_chipid = connv3_get_adie_chipid_mt6985,
};

extern struct connv3_hw_ops_struct g_consys_hw_ops_mt6985;
extern struct connv3_platform_pmic_ops g_connv3_platform_pmic_ops_mt6985;
extern struct connv3_platform_pinctrl_ops g_connv3_platform_pinctrl_ops_mt6985;
extern const struct connv3_platform_dbg_ops g_connv3_hw_dbg_mt6639;

const struct connv3_plat_data g_connv3_mt6985_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP,
	.consys_hw_version = CONN_HW_VER,
	.hw_ops = &g_connv3_hw_ops_mt6985,
	.platform_pmic_ops = &g_connv3_platform_pmic_ops_mt6985,
	.platform_pinctrl_ops = &g_connv3_platform_pinctrl_ops_mt6985,
	.platform_coredump_ops = &g_connv3_dump_ops_mt6985,
	.platform_dbg_ops = &g_connv3_hw_dbg_mt6639,
};

u32 connv3_soc_get_chipid_mt6985(void)
{
	return PLATFORM_SOC_CHIP;
}

u32 connv3_get_adie_chipid_mt6985(void)
{
	return CONN_ADIE_ID;
}
