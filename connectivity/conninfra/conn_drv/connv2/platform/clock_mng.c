// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/of_device.h>
#include <linux/regmap.h>
#include "conninfra.h"
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
static int clock_mng_register_device(void);
static int clock_mng_unregister_device(void);
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

static const char *g_clock_name[CONNSYS_CLOCK_SCHEMATIC_MAX] = {
	"26M co-clock",
	"52M co-clock",
	"26M tcxo",
	"52M tcxo",
};


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int clock_mng_init(struct platform_device *pdev, const struct conninfra_plat_data* plat_data)
{
	return clock_mng_register_device();
}

int clock_mng_deinit(void)
{
	clock_mng_unregister_device();
	return 0;
}

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

static int clock_mng_register_device(void)
{
	int ret;

	ret = platform_driver_register(&consys_mt6685_dev_drv);
	if (ret)
		pr_err("Conninfra clock ic mt6685 driver registered failed(%d)\n", ret);
	else
		pr_info("%s mt6685 ok.\n", __func__);

	return 0;
}

static int clock_mng_unregister_device(void)
{
	if (g_regmap_mt6685 != NULL) {
		platform_driver_unregister(&consys_mt6685_dev_drv);
		g_regmap_mt6685 = NULL;
	}

	return 0;
}

const char* clock_mng_get_schematic_name(enum connsys_clock_schematic type)
{
	if (type < 0 || type >= CONNSYS_CLOCK_SCHEMATIC_MAX) {
		return "Wrong clock schematic";
	}

	return g_clock_name[type];
}
