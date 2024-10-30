/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regmap.h>
#include "gps_dl_linux_clock_mng.h"
#include "gps_dl_config.h"


/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int gps_dl_clock_mng_register_device(void);
static int gps_dl_clock_mng_unregister_device(void);
static int gps_dl_mt6685_probe(struct platform_device *pdev);


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static struct regmap *g_gps_dl_regmap_mt6685;
static const struct of_device_id gps_dl_clock_mt6685_of_ids[] = {
	{.compatible = "mediatek,mt6685-gps",},
	{}
};


static struct platform_driver gps_dl_mt6685_dev_drv = {
	.probe = gps_dl_mt6685_probe,
	.driver = {
		.name = "mt6685-gps",
		.of_match_table = gps_dl_clock_mt6685_of_ids,
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
};


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int gps_dl_clock_mng_init(void)
{
	return gps_dl_clock_mng_register_device();
}

int gps_dl_clock_mng_deinit(void)
{
	g_gps_dl_regmap_mt6685 = NULL;
	gps_dl_clock_mng_unregister_device();
	return 0;
}

static int gps_dl_mt6685_probe(struct platform_device *pdev)
{
	g_gps_dl_regmap_mt6685 = dev_get_regmap(pdev->dev.parent, NULL);

	if (!g_gps_dl_regmap_mt6685) {
		GDL_LOGE("failed to get g_regmap_mt6685\n");
		return 0;
	}

	return 0;
}

struct regmap *gps_dl_clock_mng_get_regmap(void)
{
	return g_gps_dl_regmap_mt6685;
}

static int gps_dl_clock_mng_register_device(void)
{
	int ret;

	ret = platform_driver_register(&gps_dl_mt6685_dev_drv);
	if (ret)
		GDL_LOGE("gps clock ic mt6685 driver registered failed(%d)\n", ret);
	else
		GDL_LOGI("mt6685 ok.\n");

	return 0;
}

static int gps_dl_clock_mng_unregister_device(void)
{
	platform_driver_unregister(&gps_dl_mt6685_dev_drv);
	return 0;
}

#define DCXO_DIGCLK_ELR 0x7f4

int gps_dl_clock_mng_get_platform_clock(void)
{
	struct regmap *map = gps_dl_clock_mng_get_regmap();
	int value = 0;
	int retval = 0;

	if (!map) {
		GDL_LOGE("failed to get regmap.\n");
		return -1;
	}

	regmap_read(map, DCXO_DIGCLK_ELR, &value);
	if (value & 0x1)
		retval = 1; /*52M*/
	else
		retval = 0; /*26M*/

	GDL_LOGW("get clock retval = %d\n", retval);
	return retval;
}


