/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#include <linux/of_device.h>
#include <linux/regmap.h>
#include "clock_mng.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


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
static int consys_mt6685_probe(struct platform_device *pdev);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static struct regmap *g_regmap_mt6685;
#ifdef CONFIG_OF
static const struct of_device_id consys_clock_mt6685_of_ids[] = {
	{.compatible = "mediatek,mt6685-consys",},
	{}
};
#endif


static struct platform_driver consys_mt6685_dev_drv = {
	.probe = consys_mt6685_probe,
	.driver = {
		.name = "mt6685-consys",
#ifdef CONFIG_OF
		.of_match_table = consys_clock_mt6685_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
		},
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
static int consys_mt6685_probe(struct platform_device *pdev)
{
	g_regmap_mt6685 = dev_get_regmap(pdev->dev.parent, NULL);

	if (!g_regmap_mt6685) {
		pr_info("%s failed to get g_regmap_mt6685\n", __func__);
		return 0;
	}

	return 0;
}

struct regmap* consys_clock_mng_get_regmap(void)
{
	return g_regmap_mt6685;
}

int clock_mng_register_device(void)
{
	int ret;

	ret = platform_driver_register(&consys_mt6685_dev_drv);
	if (ret)
		pr_err("Conninfra clock ic mt6685 driver registered failed(%d)\n", ret);
	else
		pr_info("%s mt6685 ok.\n", __func__);

	return 0;
}

