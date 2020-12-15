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

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "conninfra.h"
#include "emi_mng.h"
#include "conninfra_core.h"
#include "consys_hw.h"

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

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

struct conninfra_rst_data {
	struct work_struct rst_worker;
	enum consys_drv_type drv;
	char *reason;
};

struct conninfra_rst_data rst_data;

void conninfra_get_phy_addr(unsigned int *addr, unsigned int *size)
{
	struct consys_emi_addr_info* addr_info = emi_mng_get_phy_addr();

	if (!addr_info) {
		pr_err("Get EMI info fail!");
		return;
	}

	if (addr)
		*addr = addr_info->emi_ap_phy_addr;
	if (size)
		*size = addr_info->emi_size;
	return;
}
EXPORT_SYMBOL(conninfra_get_phy_addr);

int conninfra_pwr_on(enum consys_drv_type drv_type)
{
	return conninfra_core_power_on(drv_type);
}
EXPORT_SYMBOL(conninfra_pwr_on);

int conninfra_pwr_off(enum consys_drv_type drv_type)
{
	return conninfra_core_power_off(drv_type);
}
EXPORT_SYMBOL(conninfra_pwr_off);

static void conninfra_rst_handler(struct work_struct *work)
{
	int ret;

	pr_info("[%s] +++++++++++", __func__);
	ret = conninfra_core_trg_chip_rst(rst_data.drv, rst_data.reason);
	if (ret)
		pr_err("[%s] trg_chip_rst fail [%d]", __func__, ret);

	osal_sleep_ms(1000);

	ret = conninfra_core_unlock_rst();
	if (ret)
		pr_err("[%s] trg_chip_rst fail [%d]", __func__, ret);
	pr_info("[%s] -----------", __func__);
}

int conninfra_trigger_whole_chip_rst(enum consys_drv_type who, char *reason)
{
	/* use schedule worker to trigger ??? */
	/* so that function can be returned immediately */
	int r;

	r = conninfra_core_lock_rst();
	pr_info("[%s] rst lock [%d]", __func__, r);
	if (r == 0) {
		/* reset is ongoing */
		pr_warn("[%s] r=[%d]", __func__, r);
		return 1;
	}

	memset(&rst_data, 0, osal_sizeof(struct conninfra_rst_data));

	pr_info("[%s] drv=[%d] reason=[%s]", __func__, who, reason);
	rst_data.drv = who;
	rst_data.reason = reason;

	INIT_WORK(&rst_data.rst_worker, conninfra_rst_handler);
	schedule_work(&rst_data.rst_worker);

	return 0;
}
EXPORT_SYMBOL(conninfra_trigger_whole_chip_rst);

int conninfra_sub_drv_ops_register(enum consys_drv_type type,
				struct sub_drv_ops_cb *cb)
{
	/* type validation */
	if (type < 0 || type >= CONNDRV_TYPE_MAX) {
		pr_err("[%s] incorrect drv type [%d]", __func__, type);
		return -1;
	}
	pr_info("[%s] ----", __func__);
	conninfra_core_subsys_ops_reg(type, cb);
	return 0;
}
EXPORT_SYMBOL(conninfra_sub_drv_ops_register);

int conninfra_sub_drv_ops_unregister(enum consys_drv_type type)
{
	/* type validation */
	if (type < 0 || type >= CONNDRV_TYPE_MAX) {
		pr_err("[%s] incorrect drv type [%d]", __func__, type);
		return -1;
	}
	pr_info("[%s] ----", __func__);
	conninfra_core_subsys_ops_unreg(type);
	return 0;
}
EXPORT_SYMBOL(conninfra_sub_drv_ops_unregister);


