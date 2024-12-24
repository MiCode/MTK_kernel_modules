// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/of_device.h>
#include <linux/regmap.h>

#include "connv3_hw.h"
#include "connv3_pmic_mng.h"

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

#if COMMON_KERNEL_PMIC_SUPPORT
static int connv3_mt6373_probe(struct platform_device *pdev);
#endif

static int pmic_mng_register_device(void);
static int pmic_mng_unregister_device(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

const struct connv3_platform_pmic_ops* g_connv3_platform_pmic_ops = NULL;
#if COMMON_KERNEL_PMIC_SUPPORT
struct regmap *g_connv3_regmap_mt6373;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

#if COMMON_KERNEL_PMIC_SUPPORT
#ifdef CONFIG_OF
const struct of_device_id connv3_pmic_mt6373_of_ids[] = {
	{.compatible = "mediatek,mt6373-connv3",},
	{}
};
#endif

static struct platform_driver connv3_mt6373_dev_drv = {
	.probe = connv3_mt6373_probe,
	.driver = {
		.name = "mt6373-connv3",
#ifdef CONFIG_OF
		.of_match_table = connv3_pmic_mt6373_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
		},
};
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int connv3_pmic_mng_common_power_ctrl(unsigned int enable)
{
	int ret = 0;
	if (g_connv3_platform_pmic_ops &&
		g_connv3_platform_pmic_ops->pmic_common_power_ctrl)
		ret = g_connv3_platform_pmic_ops->pmic_common_power_ctrl(enable);

	return ret;
}

int connv3_pmic_mng_vsel_ctrl(u32 enable)
{
	int ret = 0;
	if (g_connv3_platform_pmic_ops &&
		g_connv3_platform_pmic_ops->pmic_vsel_ctrl)
		ret = g_connv3_platform_pmic_ops->pmic_vsel_ctrl(enable);

	return ret;
}

int connv3_pmic_mng_parse_state(char *buffer, int buf_sz)
{
	int ret = 0;
	if (g_connv3_platform_pmic_ops &&
		g_connv3_platform_pmic_ops->pmic_parse_state)
		ret = g_connv3_platform_pmic_ops->pmic_parse_state(buffer, buf_sz);
	return ret;
}

int connv3_pmic_mng_set_pmic_state(void)
{

	return 0;
}

int connv3_pmic_mng_antenna_power_ctrl(u32 radio, unsigned int enable)
{
	int ret = 0;

	if (g_connv3_platform_pmic_ops &&
		g_connv3_platform_pmic_ops->pmic_antenna_power_ctrl)
		ret = g_connv3_platform_pmic_ops->pmic_antenna_power_ctrl(radio, enable);

	return ret;
}

int connv3_pmic_mng_get_connsys_chip_info(char *connsys_ecid, int connsys_ecid_size)
{
	int ret = 0;

	if (g_connv3_platform_pmic_ops &&
                g_connv3_platform_pmic_ops->pmic_get_connsys_chip_info)
                ret = g_connv3_platform_pmic_ops->pmic_get_connsys_chip_info(connsys_ecid, connsys_ecid_size);

        return ret;
}

int connv3_pmic_mng_get_pmic_chip_info(char *pmic_ecid, int pmic_ecid_size)
{
	int ret = 0;

	if (g_connv3_platform_pmic_ops &&
                g_connv3_platform_pmic_ops->pmic_get_pmic_chip_info)
                ret = g_connv3_platform_pmic_ops->pmic_get_pmic_chip_info(pmic_ecid, pmic_ecid_size);

        return ret;
}


#if COMMON_KERNEL_PMIC_SUPPORT
static int connv3_mt6373_probe(struct platform_device *pdev)
{
	g_connv3_regmap_mt6373 = dev_get_regmap(pdev->dev.parent, NULL);

	if (!g_connv3_regmap_mt6373)
		pr_notice("[%s] fail to get g_connv3_regmap_mt6373\n", __func__);
	else
		pr_info("[%s] get g_connv3_regmap_mt6373 successfully!\n", __func__);

	return 0;
}
#endif

static int pmic_mng_register_device(void)
{
#if COMMON_KERNEL_PMIC_SUPPORT
	int ret;

	ret = platform_driver_register(&connv3_mt6373_dev_drv);
	if (ret)
		pr_notice("[%s] connv3 pmic mt6373 registered failed(%d)\n", __func__, ret);
	else
		pr_info("[%s] connv3 pmic mt6373 registered successfully!\n", __func__);
#endif
	return 0;
}

static int pmic_mng_unregister_device(void)
{
#if COMMON_KERNEL_PMIC_SUPPORT
	if (g_connv3_regmap_mt6373 != NULL) {
		platform_driver_unregister(&connv3_mt6373_dev_drv);
		g_connv3_regmap_mt6373 = NULL;
	}
#endif
	return 0;
}

int connv3_pmic_mng_init(
	struct platform_device *pdev,
	struct connv3_dev_cb* dev_cb,
	const struct connv3_plat_data* plat_data)
{
	int ret = 0;

	if (g_connv3_platform_pmic_ops == NULL)
		g_connv3_platform_pmic_ops =
			(const struct connv3_platform_pmic_ops*)plat_data->platform_pmic_ops;

	pmic_mng_register_device();

	if (g_connv3_platform_pmic_ops &&
		g_connv3_platform_pmic_ops->pmic_initial_setting)
		ret = g_connv3_platform_pmic_ops->pmic_initial_setting(pdev, dev_cb);

	return ret;
}

int connv3_pmic_mng_deinit(void)
{
	pmic_mng_unregister_device();
	g_connv3_platform_pmic_ops = NULL;
	return 0;
}
