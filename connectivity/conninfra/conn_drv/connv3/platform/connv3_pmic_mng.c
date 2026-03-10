// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/of_device.h>

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

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

const struct connv3_platform_pmic_ops* g_connv3_platform_pmic_ops = NULL;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/


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

int connv3_pmic_mng_init(
	struct platform_device *pdev,
	struct connv3_dev_cb* dev_cb,
	const struct connv3_plat_data* plat_data)
{
	int ret = 0;

	if (g_connv3_platform_pmic_ops == NULL)
		g_connv3_platform_pmic_ops =
			(const struct connv3_platform_pmic_ops*)plat_data->platform_pmic_ops;

	if (g_connv3_platform_pmic_ops &&
		g_connv3_platform_pmic_ops->pmic_initial_setting)
		ret = g_connv3_platform_pmic_ops->pmic_initial_setting(pdev, dev_cb);

	return ret;
}

int connv3_pmic_mng_deinit(void)
{
	g_connv3_platform_pmic_ops = NULL;
	return 0;
}

