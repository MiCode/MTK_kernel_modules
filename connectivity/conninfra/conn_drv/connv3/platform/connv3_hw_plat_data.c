// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/of_device.h>
#include "connv3_hw.h"

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/* Platform data */
struct connv3_plat_data __weak g_connv3_mt6983_plat_data = {.chip_id=0xff};
struct connv3_plat_data __weak g_connv3_mt6985_plat_data = {.chip_id=0xff};
struct connv3_plat_data __weak g_connv3_mt6989_plat_data = {.chip_id=0xff};

#ifdef CONFIG_OF
const struct of_device_id connv3_of_ids[] = {
	{
		.compatible = "mediatek,mt6983-connv3",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6983)
		.data = (void*)&g_connv3_mt6983_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6985-connv3",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6985)
		.data = (void*)&g_connv3_mt6985_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6989-connv3",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6989)
		.data = (void*)&g_connv3_mt6989_plat_data,
	#endif
	},
	{}
};
#endif
