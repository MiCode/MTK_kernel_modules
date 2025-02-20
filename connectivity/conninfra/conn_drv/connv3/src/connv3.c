// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "connv3.h"
#include "connv3_core.h"
#include "connv3_hw.h"
#include <linux/ratelimit.h>

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

#define CONNINFRA_RST_RATE_LIMIT 0

#if CONNINFRA_RST_RATE_LIMIT
DEFINE_RATELIMIT_STATE(g_rs, HZ, 1);

#define DUMP_LOG() if (__ratelimit(&g_rs)) \
			pr_info("rst is ongoing")

#else
#define DUMP_LOG()
#endif

int connv3_pwr_on(enum connv3_drv_type drv_type)
{
	pr_info("[%s] drv=[%d]", __func__, drv_type);
	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}
	if (connv3_core_is_fmd_locking()) {
		return CONNV3_ERR_FMD_MODE;
	}

	connv3_core_pre_cal_blocking();

	return connv3_core_power_on(drv_type);
}
EXPORT_SYMBOL(connv3_pwr_on);

int connv3_pwr_on_done(enum connv3_drv_type drv_type)
{
	pr_info("[%s] drv=[%d]", __func__, drv_type);
	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}
	if (connv3_core_is_fmd_locking()) {
		return CONNV3_ERR_FMD_MODE;
	}

	connv3_core_pre_cal_blocking();

	return connv3_core_power_on_done(drv_type);
}
EXPORT_SYMBOL(connv3_pwr_on_done);


int connv3_pwr_off(enum connv3_drv_type drv_type)
{
	pr_info("[%s] drv=[%d]", __func__, drv_type);
	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}
	if (connv3_core_is_fmd_locking()) {
		return CONNV3_ERR_FMD_MODE;
	}

	connv3_core_pre_cal_blocking();

	return connv3_core_power_off(drv_type);
}
EXPORT_SYMBOL(connv3_pwr_off);

int connv3_ext_32k_on(void)
{
	return connv3_core_ext_32k_on();
}
EXPORT_SYMBOL(connv3_ext_32k_on);

int connv3_trigger_whole_chip_rst(enum connv3_drv_type who, char *reason)
{
	/* use schedule worker to trigger ??? */
	/* so that function can be returned immediately */
	int r;

	/* for normal L0 reset flow, only do it once */
	r = connv3_core_lock_rst(NULL);
	if (r >= CHIP_RST_START) {
		/* reset is ongoing */
		pr_warn("[%s] r=[%d] chip rst is ongoing\n", __func__, r);
		return 1;
	}
	pr_info("[%s] rst lock [%d] [%d] reason=%s", __func__, r, who, reason);

	connv3_core_trg_chip_rst(CONNV3_CHIP_RST_SOURCE_NORMAL, who, reason);

	return 0;
}
EXPORT_SYMBOL(connv3_trigger_whole_chip_rst);

int connv3_trigger_pmic_irq(enum connv3_drv_type who, char *reason)
{
	int r;
	unsigned int rst_source = 0;

	r = connv3_core_lock_rst(&rst_source);
	if (r >= CHIP_RST_START && rst_source > CONNV3_CHIP_RST_SOURCE_NORMAL) {
		/* pmic reset is ongoing */
		pr_notice("[%s] r=[%d] rst_source=[%d] pmic rst is ongoing\n", __func__, r, rst_source);
		return 1;
	}
	pr_info("[%s] rst lock [%d] [%d] [%d] reason=%s",
		__func__, r, who, rst_source, reason);

	connv3_core_trg_chip_rst(CONNV3_CHIP_RST_SOURCE_PMIC_IRQ_B, who, reason);
	return 0;
}
EXPORT_SYMBOL(connv3_trigger_pmic_irq);

int connv3_conninfra_bus_dump(enum connv3_drv_type drv_type)
{
	int ret;

	/* Check invalid parameter */
	if (drv_type >= CONNV3_DRV_TYPE_MAX) {
		pr_err("[%s] invalid parameter: drv_type=[%d]",
			__func__, drv_type);
		return -EINVAL;
	}

	if (connv3_core_is_rst_power_off_stage()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}

	ret = connv3_core_bus_dump(drv_type);
	return ret;
}
EXPORT_SYMBOL(connv3_conninfra_bus_dump);

void connv3_update_pmic_state(enum connv3_drv_type drv, char *buffer, int buf_sz)
{
	connv3_core_update_pmic_status(drv, buffer, buf_sz);
}
EXPORT_SYMBOL(connv3_update_pmic_state);

int connv3_hif_dbg_start(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv)
{
	/* Check invalid parameter */
	if (from_drv >= CONNV3_DRV_TYPE_MAX || to_drv >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid parameter: drv_type=[%d][%d]",
			__func__, from_drv, to_drv);
		return -EINVAL;
	}

	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}

	return connv3_core_hif_dbg_start(from_drv, to_drv);
}
EXPORT_SYMBOL(connv3_hif_dbg_start);

int connv3_hif_dbg_end(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv)
{
	/* Check invalid parameter */
	if (from_drv >= CONNV3_DRV_TYPE_MAX || to_drv >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid parameter: drv_type=[%d][%d]",
			__func__, from_drv, to_drv);
		return -EINVAL;
	}

	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}

	return connv3_core_hif_dbg_end(from_drv, to_drv);
}
EXPORT_SYMBOL(connv3_hif_dbg_end);

int connv3_hif_dbg_read(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int *value)
{
	/* Check invalid parameter */
	if (from_drv >= CONNV3_DRV_TYPE_MAX || to_drv >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid parameter: drv_type=[%d][%d]",
			__func__, from_drv, to_drv);
		return -EINVAL;
	}

	if (value == NULL) {
		pr_notice("[%s] value is NULL", __func__);
		return -EINVAL;
	}

	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}

	return connv3_core_hif_dbg_read(from_drv, to_drv,
		addr, value);
}
EXPORT_SYMBOL(connv3_hif_dbg_read);

int connv3_hif_dbg_write(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int value)
{
	/* Check invalid parameter */
	if (from_drv >= CONNV3_DRV_TYPE_MAX || to_drv >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid parameter: drv_type=[%d][%d]",
			__func__, from_drv, to_drv);
		return -EINVAL;
	}

	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}

	return connv3_core_hif_dbg_write(from_drv, to_drv, addr, value);
}
EXPORT_SYMBOL(connv3_hif_dbg_write);


int connv3_hif_dbg_write_mask(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int mask, unsigned int value)
{
	/* Check invalid parameter */
	if (from_drv >= CONNV3_DRV_TYPE_MAX || to_drv >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid parameter: drv_type=[%d][%d]",
			__func__, from_drv, to_drv);
		return -EINVAL;
	}

	if (connv3_core_is_rst_locking()) {
		DUMP_LOG();
		return CONNV3_ERR_RST_ONGOING;
	}

	return connv3_core_hif_dbg_write_mask(from_drv, to_drv,
		addr, mask, value);
}
EXPORT_SYMBOL(connv3_hif_dbg_write_mask);

u8* connv3_get_plat_config(u32 *size)
{
	return connv3_hw_get_custom_option(size);
}
EXPORT_SYMBOL(connv3_get_plat_config);

int connv3_enter_fmd_mode(void)
{
	return connv3_core_enter_fmd_mode();
}
EXPORT_SYMBOL(connv3_enter_fmd_mode);

int connv3_sub_drv_ops_register(enum connv3_drv_type type, struct connv3_sub_drv_ops_cb *cb)
{
	/* type validation */
	if (type < 0 || type >= CONNV3_DRV_TYPE_MAX) {
		pr_err("[%s] incorrect drv type [%d]", __func__, type);
		return -EINVAL;
	}
	connv3_core_subsys_ops_reg(type, cb);
	return 0;
}
EXPORT_SYMBOL(connv3_sub_drv_ops_register);

int connv3_sub_drv_ops_unregister(enum connv3_drv_type type)
{
	/* type validation */
	if (type < 0 || type >= CONNV3_DRV_TYPE_MAX) {
		pr_err("[%s] incorrect drv type [%d]", __func__, type);
		return -EINVAL;
	}
	connv3_core_subsys_ops_unreg(type);
	return 0;
}
EXPORT_SYMBOL(connv3_sub_drv_ops_unregister);


