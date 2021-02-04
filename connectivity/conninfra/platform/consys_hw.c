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

#include <linux/of_reserved_mem.h>
#include <linux/delay.h>
#include "osal.h"

#include "consys_hw.h"
#include "emi_mng.h"
#include "pmic_mng.h"
#include "consys_reg_mng.h"

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

static int mtk_conninfra_probe(struct platform_device *pdev);
static int mtk_conninfra_remove(struct platform_device *pdev);
static int mtk_conninfra_suspend(struct platform_device *pdev, pm_message_t state);
static int mtk_conninfra_resume(struct platform_device *pdev);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

#ifdef CONFIG_OF
const struct of_device_id apconninfra_of_ids[] = {
	{.compatible = "mediatek,mt6789-consys",},
	{}
};
#endif

static struct platform_driver mtk_conninfra_dev_drv = {
	.probe = mtk_conninfra_probe,
	.remove = mtk_conninfra_remove,
	.suspend = mtk_conninfra_suspend,
	.resume = mtk_conninfra_resume,
	.driver = {
		   .name = "mtk_conninfra",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = apconninfra_of_ids,
#endif
		   },
};

struct consys_hw_ops_struct *consys_hw_ops;
struct platform_device *g_pdev;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
struct consys_hw_ops_struct* __weak get_consys_platform_ops(void)
{
	pr_err("Miss platform ops !!\n");
	return NULL;
}


struct platform_device *get_consys_device(void)
{
	return g_pdev;
}


int consys_hw_pwr_off(void)
{
#if 0
	if (wmt_consys_ic_ops->consys_ic_ahb_clock_ctrl)
			wmt_consys_ic_ops->consys_ic_ahb_clock_ctrl(DISABLE);
		if (wmt_consys_ic_ops->consys_ic_hw_power_ctrl)
			wmt_consys_ic_ops->consys_ic_hw_power_ctrl(DISABLE);
		if (co_clock_type) {
			if (wmt_consys_ic_ops->consys_ic_clock_buffer_ctrl)
				wmt_consys_ic_ops->consys_ic_clock_buffer_ctrl(DISABLE);
		}

		if (co_clock_type == 0) {
			if (wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl)
				wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl(DISABLE);
			/*turn off VCN28 LDO (with PMIC_WRAP API)" */
			if (wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl)
				wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl(DISABLE);
		}

		if (wmt_consys_ic_ops->consys_ic_set_if_pinmux)
			wmt_consys_ic_ops->consys_ic_set_if_pinmux(DISABLE);

		if (wmt_consys_ic_ops->consys_ic_hw_vcn18_ctrl)
			wmt_consys_ic_ops->consys_ic_hw_vcn18_ctrl(DISABLE);
#endif
	return 0;
}

int consys_hw_pwr_on(void)
{
	int ret;

	/* POS PART 1:
	 * Set PMIC to turn on the power that AFE WBG circuit in D-die,
	 * OSC or crystal component, and A-die need.
	 */
	ret = pmic_mng_common_power_ctrl(1);
	if (consys_hw_ops->consys_plt_clock_buffer_ctrl)
		consys_hw_ops->consys_plt_clock_buffer_ctrl(1);

	/* POS PART 2:
	 * 1. Pinmux setting
	 * 2. Turn on MTCMOS
	 * 3. Enable AXI bus (AP2CONN slpprot)
	 */
	if (consys_hw_ops->consys_plt_set_if_pinmux)
		consys_hw_ops->consys_plt_set_if_pinmux(1);

	udelay(150);
	if (consys_hw_ops->consys_plt_conninfra_on_power_ctrl)
		consys_hw_ops->consys_plt_conninfra_on_power_ctrl(1);

	/* Wait 5ms for CONNSYS XO clock ready */
	mdelay(5);

	if (consys_hw_ops->consys_plt_polling_consys_chipid)
		consys_hw_ops->consys_plt_polling_consys_chipid();

	/* POS PART 3:
	 * 1. d_die_cfg
	 * 2. spi_master_cfg
	 * 3. a_die_cfg
	 * 4. afe_wbg_cal
	 * 5. patch default value
	 * 6. CONN_INFRA low power setting (srcclken wait time, mtcmos HW ctl...)
	 */
	if (consys_hw_ops->consys_plt_d_die_cfg)
		consys_hw_ops->consys_plt_d_die_cfg();
	if (consys_hw_ops->consys_plt_spi_master_cfg)
		consys_hw_ops->consys_plt_spi_master_cfg();
	if (consys_hw_ops->consys_plt_a_die_cfg)
		consys_hw_ops->consys_plt_a_die_cfg();
	if (consys_hw_ops->consys_plt_afe_wbg_cal)
		consys_hw_ops->consys_plt_afe_wbg_cal();
	if (consys_hw_ops->consys_plt_low_power_setting)
		consys_hw_ops->consys_plt_low_power_setting();
	return 0;
}

int consys_hw_wifi_power_ctl(unsigned int enable)
{
	return 0;
}

int consys_hw_bt_power_ctl(unsigned int enable)
{
	return 0;
}

int consys_hw_gps_power_ctl(unsigned int enable)
{
	return 0;
}

int consys_hw_fm_power_ctl(unsigned int enable)
{
	return 0;
}

int mtk_conninfra_probe(struct platform_device *pdev)
{
	int ret = -1;

	/* Read device node */

	if (consys_reg_mng_init(pdev) != 0) {
		pr_err("consys_plt_read_reg_from_dts fail");
		return -1;
	}

	if (consys_hw_ops->consys_plt_clk_get_from_dts)
		consys_hw_ops->consys_plt_clk_get_from_dts(pdev);
	else {
		pr_err("consys_plt_clk_get_from_dtsfail");
		return -2;
	}

	/* emi mng init */
	ret = emi_mng_init();
	if (ret) {
		pr_err("emi_mng init fail, %d\n", ret);
		return -3;
	}

	ret = pmic_mng_init(pdev);
	if (ret) {
		pr_err("pmic_mng init fail, %d\n", ret);
		return -4;
	}

	if (pdev)
		g_pdev = pdev;

	return 0;
}

int mtk_conninfra_remove(struct platform_device *pdev)
{
#if 0
	if (wmt_consys_ic_ops->consys_ic_need_store_pdev) {
		if (wmt_consys_ic_ops->consys_ic_need_store_pdev() == MTK_WCN_BOOL_TRUE)
			pm_runtime_disable(&pdev->dev);
	}

	if (wmt_consys_ic_ops->consys_ic_dedicated_log_path_deinit)
		wmt_consys_ic_ops->consys_ic_dedicated_log_path_deinit();
	if (wmt_consys_ic_ops->consys_ic_emi_coredump_remapping)
		wmt_consys_ic_ops->consys_ic_emi_coredump_remapping(&pEmibaseaddr, 0);
#endif
	if (g_pdev)
		g_pdev = NULL;

	return 0;
}

int mtk_conninfra_suspend(struct platform_device *pdev, pm_message_t state)
{
	//WMT_STEP_DO_ACTIONS_FUNC(STEP_TRIGGER_POINT_WHEN_AP_SUSPEND);

	return 0;
}

int mtk_conninfra_resume(struct platform_device *pdev)
{
	//WMT_STEP_DO_ACTIONS_FUNC(STEP_TRIGGER_POINT_WHEN_AP_RESUME);

	//if (wmt_consys_ic_ops->consys_ic_resume_dump_info)
	//	wmt_consys_ic_ops->consys_ic_resume_dump_info();

	return 0;
}


int consys_hw_init(void)
{
	int iRet = 0;

	if (consys_hw_ops == NULL)
		consys_hw_ops = get_consys_platform_ops();

	iRet = platform_driver_register(&mtk_conninfra_dev_drv);
	if (iRet)
		pr_err("Conninfra platform driver registered failed(%d)\n", iRet);

	pr_info("[consys_hw_init] result [%d]\n", iRet);
	return iRet;
}

int consys_hw_deinit(void)
{
	platform_driver_unregister(&mtk_conninfra_dev_drv);
	return 0;
}
