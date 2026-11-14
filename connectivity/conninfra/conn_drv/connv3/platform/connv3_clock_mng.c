// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include "connv3_clock_mng.h"

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
static int connv3_clock_mng_register_device(void);
static int connv3_clock_mng_unregister_device(void);
static int connv3_clock_mt6687_probe(struct platform_device *pdev);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
const struct connv3_platform_clock_ops* g_connv3_platform_clock_ops = NULL;
static struct regmap *g_regmap_mt6687 = NULL;
#ifdef CONFIG_OF
static const struct of_device_id connv3_clock_mt6687_of_ids[] = {
	{.compatible = "mediatek,mt6687-connv3",},
	{}
};
#endif

static struct platform_driver connv3_clock_mt6687_dev_drv = {
	.probe = connv3_clock_mt6687_probe,
	.driver = {
		.name = "mt6687-connv3",
#ifdef CONFIG_OF
		.of_match_table = connv3_clock_mt6687_of_ids,
#endif
	},
};


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int connv3_clock_mng_init(
	struct platform_device *pdev,
	struct connv3_dev_cb* dev_cb,
	const struct connv3_plat_data* plat_data)
{
	int ret = 0;

	if (g_connv3_platform_clock_ops == NULL)
		g_connv3_platform_clock_ops =
			(const struct connv3_platform_clock_ops*)plat_data->platform_clock_ops;

	ret = connv3_clock_mng_register_device();
	if (ret)
		pr_notice("[%s] register device failed! ret = %d\n", __func__, ret);

	if (g_connv3_platform_clock_ops &&
		g_connv3_platform_clock_ops->clk_initial_setting)
		ret = g_connv3_platform_clock_ops->clk_initial_setting(pdev, dev_cb);
	else
		pr_info("[%s] clk_initial_setting not found\n", __func__);

	return ret;
}

int connv3_clock_mng_deinit(void)
{
	connv3_clock_mng_unregister_device();
	return 0;
}

static int connv3_clock_mt6687_probe(struct platform_device *pdev)
{
	pr_info("%s enter\n", __func__);

	g_regmap_mt6687 = dev_get_regmap(pdev->dev.parent, NULL);

	if (!g_regmap_mt6687) {
		pr_info("%s failed to get g_regmap_mt6687\n", __func__);
		return 0;
	}

	return 0;
}

struct regmap* connv3_clock_mng_get_regmap(void)
{
	if (g_regmap_mt6687 != NULL)
		return g_regmap_mt6687;
	return NULL;
}

static int connv3_clock_mng_register_device(void)
{
	int ret;

	ret = platform_driver_register(&connv3_clock_mt6687_dev_drv);
	if (ret)
		pr_notice("Conninfra clock ic mt6687 driver registered failed(%d)\n", ret);
	else
		pr_info("%s mt6687 ok.\n", __func__);

	return 0;
}

static int connv3_clock_mng_unregister_device(void)
{
	if (g_regmap_mt6687 != NULL) {
		platform_driver_unregister(&connv3_clock_mt6687_dev_drv);
		g_regmap_mt6687 = NULL;
	}

	return 0;
}