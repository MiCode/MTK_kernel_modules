// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/of_device.h>

#include "consys_hw.h"

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/* Platform data */
struct conninfra_plat_data __weak mt6885_plat_data = {};
struct conninfra_plat_data __weak mt6893_plat_data = {};
struct conninfra_plat_data __weak mt6877_plat_data = {};
struct conninfra_plat_data __weak mt6983_plat_data = {};
struct conninfra_plat_data __weak mt6879_plat_data = {};
struct conninfra_plat_data __weak mt6895_plat_data = {};

#ifdef CONFIG_OF
const struct of_device_id apconninfra_of_ids[] = {
	{
		.compatible = "mediatek,mt6885-consys",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6885)
		.data = (void*)&mt6885_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6893-consys",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6893)
		.data = (void*)&mt6893_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6877-consys",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6877)
		.data = (void*)&mt6877_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6983-consys",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6983)
		.data = (void*)&mt6983_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6879-consys",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6879)
		.data = (void*)&mt6879_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6895-consys",
	#if IS_ENABLED(CONFIG_MTK_COMBO_CHIP_CONSYS_6895)
		.data = (void*)&mt6895_plat_data,
	#endif
	},
	{}
};
#endif
