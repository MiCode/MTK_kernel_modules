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
static enum connv3_radio_off_mode connv3_hw_get_radio_off_mode(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

const struct connv3_hw_ops_struct *connv3_hw_ops;
struct platform_device *g_connv3_pdev;

const struct connv3_plat_data *g_connv3_plat_data = NULL;

static enum connv3_radio_off_mode g_radio_off_mode
		= CONNV3_RADIO_OFF_MODE_PMIC_OFF;

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

unsigned int connv3_hw_get_reset_type_support(void)
{
	if (connv3_hw_ops->connsys_plt_reset_type_support)
		return connv3_hw_ops->connsys_plt_reset_type_support();
	else
		pr_notice("connsys_plt_reset_type_support not supproted\n");

	/* 0 means "shutdown PMIC" */
	return 0;
}

unsigned int connv3_hw_get_connsys_ic_info(uint8_t *buf, u32 buf_sz)
{
	int ret;

	ret = connv3_pmic_mng_get_connsys_chip_info(buf, buf_sz);

	return ret;
}

unsigned int connv3_hw_get_pmic_ic_info(uint8_t *buf, u32 buf_sz)
{
	int ret;

	ret = connv3_pmic_mng_get_pmic_chip_info(buf, buf_sz);

	return ret;
}

unsigned int connv3_hw_get_connsys_adie_ic_info(uint8_t *buf, u32 buf_sz)
{
	int ret;

	ret = connv3_pmic_mng_get_connsys_adie_chip_info(buf, buf_sz);
	return ret;
}

unsigned int connv3_hw_pre_cal_blocking_enable(void)
{
#if defined(CONFIG_FPGA_EARLY_PORTING)
	/* For FPGA environment (host), disable pre-cal blocking */
	return 0;
#else
	/* ASIC, default enable */
	if (connv3_hw_ops->connsys_plt_pre_cal_blocking_enable)
		return connv3_hw_ops->connsys_plt_pre_cal_blocking_enable();
	return 1;
#endif
}

int connv3_hw_pwr_off(unsigned int curr_status, unsigned int off_radio, unsigned int *pmic_state)
{
	/*
	 * +------+---------------- +------------+-----------+-------------------+
	 * | Mode | pwr_on          | last       | rst       | pre_cal           |
	 * |      | (pwr recycle)   | subdrv off |           | (last subdrv off) |
	 * +------+---------------- +------------+-----------+-------------------+
	 * | UDS  | pmic_en=0       | vsel=0     | pmic_en=0 | vsel=0            |
	 * |      | vsel=0          |            | vsel=0    |                   |
	 * +------+---------------- +------------+-----------+-------------------+
	 * | PMIC | (X)             | pmic_en=0  | pmic_en=0 | pmic_en=0         |
	 * | OFF  | Same as UDS for | vsel=0     | vsel=0    | vsel=0            |
	 * |      | simplicity      |            |           |                   |
	 * +------+---------------- +------------+-----------+-------------------+
	 */
	int ret;

	/* Init return value */
	if (pmic_state)
		*pmic_state = 0;

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

		if (g_radio_off_mode == CONNV3_RADIO_OFF_MODE_PMIC_OFF
			|| (off_radio == CONNV3_DRV_TYPE_MAX)) {
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
			if (pmic_state != NULL)
				*pmic_state = 1;
			pr_info("[%s] force PMIC off, ret = %d\n", __func__, ret);
		}

		ret = connv3_pmic_mng_vsel_ctrl(0);
		if (ret) {
			pr_err("[%s] pmic vsel fail, ret = %d", __func__, ret);
			return ret;
		}
	}

	return 0;
}

int connv3_hw_pwr_on(unsigned int curr_status, unsigned int on_radio)
{
	int ret;

	if (curr_status == 0) {
		ret = connv3_pmic_mng_vsel_ctrl(1);
		if (ret)
			return ret;

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

	pr_info("[%s] --- [%p] of_node[%s][%s]", __func__,
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

int connv3_hw_pwr_rst(void)
{
	return connv3_pmic_mng_pwr_rst();
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

static enum connv3_radio_off_mode connv3_hw_get_radio_off_mode(void)
{
	enum connv3_radio_off_mode mode = CONNV3_RADIO_OFF_MODE_PMIC_OFF;
	struct device_node *node;
	u32 data;
	int ret;

	node = g_connv3_pdev->dev.of_node;
	ret = of_property_read_u32(node, "radio-off-mode", &data);
	if (ret < 0)
		pr_notice("[%s] property (radio-off-mode) read fail: %d", __func__, ret);
	else {
		if (data < CONNV3_RADIO_OFF_MODE_MAX) {
			mode = data;
			pr_info("[%s] get: %d, mode=%d\n", __func__, data, mode);
		} else {
			pr_notice("[%s] wrong data: %d\n", __func__, data);
		}
	}

	pr_info("[%s] return mode (%d)\n", __func__, mode);
	return mode;
}

int connv3_hw_dfd_trigger(bool enable)
{
	return connv3_pinctrl_mng_dfd_trigger(enable);
}

u8* connv3_hw_get_custom_option(u32 *size)
{
	if (connv3_hw_ops->connsys_plt_get_custom_option)
		return connv3_hw_ops->connsys_plt_get_custom_option(size);

	if (size)
		*size = 0;

	return NULL;
}

int connv3_hw_clk_init(struct platform_device *pdev, struct connv3_dev_cb *dev_cb)
{
	if (connv3_hw_ops->connsys_plt_clk_init)
		return connv3_hw_ops->connsys_plt_clk_init(pdev, dev_cb);

	return 0;
}

int connv3_hw_check_status(void)
{
	if (connv3_hw_ops->connsys_plt_check_status)
		return connv3_hw_ops->connsys_plt_check_status();

	return CONNV3_PLT_STATE_READY;
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

	ret = connv3_hw_clk_init(pdev, dev_cb);
	if (ret)
		pr_notice("[%s] connv3_hw_clk_init fail, ret = %d\n", __func__, ret);

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

	g_radio_off_mode = connv3_hw_get_radio_off_mode();

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
