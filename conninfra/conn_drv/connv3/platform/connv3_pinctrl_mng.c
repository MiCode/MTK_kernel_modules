// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>

#include "connv3_hw.h"
#include "connv3_pinctrl_mng.h"

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

const struct connv3_platform_pinctrl_ops* g_connv3_platform_pinctrl_ops = NULL;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int connv3_pinctrl_mng_setup_pre(void)
{
	int ret = 0;

	if (g_connv3_platform_pinctrl_ops &&
		g_connv3_platform_pinctrl_ops->pinctrl_setup_pre)
		ret = g_connv3_platform_pinctrl_ops->pinctrl_setup_pre();

	return ret;

}

int connv3_pinctrl_mng_setup_done(void)
{
	int ret = 0;

	if (g_connv3_platform_pinctrl_ops &&
		g_connv3_platform_pinctrl_ops->pinctrl_setup_done)
		ret = g_connv3_platform_pinctrl_ops->pinctrl_setup_done();

	return ret;
}

int connv3_pinctrl_mng_remove(void)
{
	int ret = 0;

	if (g_connv3_platform_pinctrl_ops &&
		g_connv3_platform_pinctrl_ops->pinctrl_remove)
		ret = g_connv3_platform_pinctrl_ops->pinctrl_remove();

	return ret;
}

int connv3_pinctrl_mng_ext_32k_ctrl(bool on)
{

	int ret = 0;

	if (g_connv3_platform_pinctrl_ops &&
		g_connv3_platform_pinctrl_ops->pinctrl_ext_32k_ctrl)
		ret = g_connv3_platform_pinctrl_ops->pinctrl_ext_32k_ctrl(on);

	return ret;
}

int connv3_pinctrl_mng_init(
	struct platform_device *pdev,
	const struct connv3_plat_data* plat_data)
{
	int ret = 0;

	if (g_connv3_platform_pinctrl_ops == NULL)
		g_connv3_platform_pinctrl_ops =
			(const struct connv3_platform_pinctrl_ops*)plat_data->platform_pinctrl_ops;
	if (g_connv3_platform_pinctrl_ops != NULL &&
	    g_connv3_platform_pinctrl_ops->pinctrl_init)
		ret = g_connv3_platform_pinctrl_ops->pinctrl_init(pdev);

	return ret;
}

int connv3_pinctrl_mng_deinit(void)
{
	if (g_connv3_platform_pinctrl_ops != NULL &&
	    g_connv3_platform_pinctrl_ops->pinctrl_deinit)
		g_connv3_platform_pinctrl_ops->pinctrl_deinit();

	g_connv3_platform_pinctrl_ops = NULL;
	return 0;
}

