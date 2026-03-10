/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pinctrl/consumer.h>
#include <linux/suspend.h>

#include "connv3_hw.h"
#include "connv3_hw_dbg.h"
#include "connv3_pmic_mng.h"
#include "connv3_pinctrl_mng.h"
#include "coredump/connv3_dump_mng.h"

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

static int get_connv3_platform_ops(struct platform_device *pdev);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

const struct connv3_hw_ops_struct *connv3_hw_ops;
struct platform_device *g_connv3_pdev;

const struct connv3_plat_data *g_connv3_plat_data = NULL;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

struct platform_device *get_connv3_device(void)
{
	return g_connv3_pdev;
}

unsigned int connv3_hw_get_chipid(void)
{
	if (connv3_hw_ops->connsys_plt_get_chipid)
		return connv3_hw_ops->connsys_plt_get_chipid();
	else
		pr_err("consys_plt_soc_chipid_get not supported\n");

	return 0;
}

unsigned int connv3_hw_get_adie_chipid(void)
{
	if (connv3_hw_ops->connsys_plt_get_adie_chipid)
		return connv3_hw_ops->connsys_plt_get_adie_chipid();
	else
		pr_err("connsys_plt_get_adie_chipid not supported\n");
	return 0;
}

int connv3_hw_pwr_off(unsigned int curr_status, unsigned int off_radio)
{
	int ret;

	ret = connv3_pmic_mng_antenna_power_ctrl(off_radio, 0);
	if (ret)
		pr_notice("[%s] antenna power ctrl fail, ret = %d",
			__func__, ret);

	if ((curr_status & (~(0x1 << off_radio))) == 0) {
		ret = connv3_pinctrl_mng_remove();
		if (ret) {
			pr_err("[%s] remove pinctrl fail, ret = %d", __func__, ret);
			return ret;
		}
	}

	if (off_radio == CONNV3_DRV_TYPE_MAX) {
		ret = connv3_pinctrl_mng_ext_32k_ctrl(false);
		if (ret) {
			pr_err("[%s] turn off ext 32k fail, ret = %d", __func__, ret);
			return ret;
		}
		ret = connv3_pmic_mng_common_power_ctrl(0);
		if (ret) {
			pr_err("[%s] pmic off fail, ret = %d", __func__, ret);
			return ret;
		}
	}

	return 0;
}

int connv3_hw_pwr_on(unsigned int curr_status, unsigned int on_radio)
{
	int ret;

	if (curr_status == 0) {
		ret = connv3_pmic_mng_common_power_ctrl(1);
		if (ret)
			return ret;

		ret = connv3_pinctrl_mng_setup_pre();
		if (ret)
			return ret;
	}

	ret = connv3_pmic_mng_antenna_power_ctrl(on_radio, 1);
	if (ret)
		pr_notice("[%s] antenna power control fail, ret = %d",
			__func__, ret);

	return 0;
}


int connv3_hw_pwr_on_done(unsigned int radio)
{
	int ret = 0;

	ret = connv3_pinctrl_mng_setup_done();
	return ret;
}

int connv3_hw_pmic_parse_state(char *buffer, int buf_sz)
{
	return connv3_pmic_mng_parse_state(buffer, buf_sz);
}

int get_connv3_platform_ops(struct platform_device *pdev)
{

	pr_info("[%s] --- [%x]of_node[%s][%s]", __func__,
				pdev->dev.driver->of_match_table,
				(pdev->dev.of_node != NULL ? pdev->dev.of_node->name : ""),
				(pdev->dev.of_node != NULL ? pdev->dev.of_node->full_name : ""));

	g_connv3_plat_data = (const struct connv3_plat_data*)of_device_get_match_data(&pdev->dev);
	if (g_connv3_plat_data == NULL) {
		pr_err("[%s] Get platform data fail.", __func__);
		return -1;
	}

	pr_info("[%s] chipid=[%x] hw_ops=[%p]", __func__, g_connv3_plat_data->chip_id,
						g_connv3_plat_data->hw_ops);
	if (connv3_hw_ops == NULL)
		connv3_hw_ops = (const struct connv3_hw_ops_struct*)g_connv3_plat_data->hw_ops;

	if (connv3_hw_ops == NULL) {
		pr_err("[%s] Get HW op fail", __func__);
		return -1;
	}
	return 0;
}

int connv3_hw_ext_32k_onoff(bool on)
{
	return connv3_pinctrl_mng_ext_32k_ctrl(on);
}

int connv3_hw_bus_dump(enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	return connv3_hw_dbg_bus_dump(drv_type, cb);
}

int connv3_hw_power_info_dump(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size)
{
	return connv3_hw_dbg_power_info_dump(drv_type, cb, buf, size);
}

int connv3_hw_power_info_reset(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	return connv3_hw_dbg_power_info_reset(drv_type, cb);
}

int connv3_hw_init(struct platform_device *pdev, struct connv3_dev_cb *dev_cb)
{
	int ret = 0;

	pr_info("[%s] ++++++++++++++++", __func__);
	ret = get_connv3_platform_ops(pdev);
	if (ret) {
		pr_err("[%s] get platform ops fail", __func__);
		return -2;
	}

	ret = connv3_pmic_mng_init(pdev, dev_cb, g_connv3_plat_data);
	if (ret) {
		pr_err("[%s] init pmic fail", __func__);
		return -3;
	}

	ret = connv3_pinctrl_mng_init(pdev, g_connv3_plat_data);
	if (ret) {
		pr_err("[%s] init pinctrl fail", __func__);
		return -3;
	}

	ret = connv3_dump_mng_init((void*)g_connv3_plat_data->platform_coredump_ops);

	ret = connv3_hw_dbg_init(pdev, g_connv3_plat_data);

	g_connv3_pdev = pdev;

	pr_info("[%s] result [%d]\n", __func__, ret);
	return ret;
}

int connv3_hw_deinit(void)
{
	int ret;

	ret = connv3_hw_dbg_deinit();

	if (g_connv3_pdev)
		g_connv3_pdev = NULL;

	return 0;
}
