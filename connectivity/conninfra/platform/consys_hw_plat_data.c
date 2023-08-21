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
#if defined(CONFIG_MACH_MT6885)
extern struct conninfra_plat_data mt6885_plat_data;
#elif defined(CONFIG_MACH_MT6893)
extern struct conninfra_plat_data mt6893_plat_data;
#elif defined(CONFIG_MACH_MT6877)
extern struct conninfra_plat_data mt6877_plat_data;
#endif

#ifdef CONFIG_OF
const struct of_device_id apconninfra_of_ids[] = {
	{
		.compatible = "mediatek,mt6885-consys",
	#if defined(CONFIG_MACH_MT6885)
		.data = (void*)&mt6885_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6893-consys",
	#if defined(CONFIG_MACH_MT6893)
		.data = (void*)&mt6893_plat_data,
	#endif
	},
	{
		.compatible = "mediatek,mt6877-consys",
	#if defined(CONFIG_MACH_MT6877)
		.data = (void*)&mt6877_plat_data,
	#endif
	},
	{}
};
#endif
