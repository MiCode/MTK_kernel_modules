// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#define pr_fmt(fmt) "conninfra_test@(%s:%d) " fmt, __func__, __LINE__

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#include "connv3.h"
#include "connv3_test.h"
#include "osal.h"

#include "connv3_dump_test.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define CONNV3_TEST_PROCNAME "driver/connv3_test"

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
static ssize_t connv3_test_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t connv3_test_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

static int core_tc(int par1, int par2, int par3);
static int cal_tc(int par1, int par2, int par3);
static int chip_rst_tc(int par1, int par2, int par3);
static int v3_coredump_tc(int par1, int par2, int par3);
static int v3_hif_dump_tc(int par1, int par2, int par3);
static int v3_dfd_rst_tc(int par1, int par2, int par3);
static int v3_custom_config_tc(int par1, int par2, int par3);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

typedef int(*CONNINFRA_TEST_FUNC) (int par1, int par2, int par3);

static struct proc_dir_entry *g_connv3_test_entry;

static const CONNINFRA_TEST_FUNC connv3_test_func[] = {
	[0x01] = core_tc,
	//[0x02] = conf_tc,
	[0x04] = chip_rst_tc,
	[0x05] = cal_tc,
	//[0x07] = emi_tc,
	//[0x08] = log_tc,
	//[0x09] = thermal_tc,
	//[0x0a] = bus_hang_tc,
	[0x0b] = v3_coredump_tc,
	//[0x0c] = v3_bus_dump_tc,
	//[0x0d] = ap_resume_tc,
	[0xe] = v3_hif_dump_tc,
	[0xf] = v3_dfd_rst_tc,
	[0x10] = v3_custom_config_tc,
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int wifi_pre_power_on(void)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

int wifi_power_on_noitfy(void)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

int wifi_pre_chip_rst(enum connv3_drv_type drv, char *reason, unsigned int reset_type)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

int wifi_post_chip_rst(void)
{
	pr_info("[%s] ===", __func__);
	return 0;
}


int bt_pre_power_on(void)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

int bt_power_on_noitfy(void)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

int bt_pre_chip_rst(enum connv3_drv_type drv, char *reason, unsigned int reset_type)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

int bt_post_chip_rst(void)
{
	pr_info("[%s] ===", __func__);
	return 0;
}

bool g_sub_drv_init = false;

static void connv3_test_drv_init(void)
{
	struct connv3_sub_drv_ops_cb g_wifi_drv_ops = {
		.pwr_on_cb = {
			.pre_power_on = wifi_pre_power_on,
			.power_on_notify = wifi_power_on_noitfy,
		},
		.rst_cb = {
			.pre_whole_chip_rst = wifi_pre_chip_rst,
			.post_whole_chip_rst = wifi_post_chip_rst,
		},
	};
	struct connv3_sub_drv_ops_cb g_bt_drv_ops = {
		.pwr_on_cb = {
			.pre_power_on = bt_pre_power_on,
			.power_on_notify = bt_power_on_noitfy,
		},
		.rst_cb = {
			.pre_whole_chip_rst = bt_pre_chip_rst,
			.post_whole_chip_rst = bt_post_chip_rst,
		},
	};

	if (g_sub_drv_init)
		return;

	connv3_sub_drv_ops_register(CONNV3_DRV_TYPE_WIFI, &g_wifi_drv_ops);
	connv3_sub_drv_ops_register(CONNV3_DRV_TYPE_BT, &g_bt_drv_ops);
	g_sub_drv_init = true;
}

int core_tc(int par1, int par2, int par3)
{
	int ret;

	connv3_test_drv_init();

	if (par2 == 0) {
		/* power on */
		ret = connv3_pwr_on(CONNV3_DRV_TYPE_WIFI);
		pr_info("[%s] power on ret=[%d]", __func__, ret);

		msleep(100);
		ret = connv3_pwr_on_done(CONNV3_DRV_TYPE_WIFI);
		pr_info("[%s] power on done ret=[%d]", __func__, ret);

		ret = connv3_pwr_on(CONNV3_DRV_TYPE_BT);
		pr_info("[%s] ret=[%d]", __func__, ret);

		msleep(100);
		connv3_pwr_on_done(CONNV3_DRV_TYPE_BT);
		pr_info("[%s] power on done ret=[%d]", __func__, ret);

	} else if (par2 == 1) {
		/* power off */
		ret = connv3_pwr_off(CONNV3_DRV_TYPE_BT);
		pr_info("[%s] wifi power off ret=[%d]", __func__, ret);

		msleep(100);

		ret = connv3_pwr_off(CONNV3_DRV_TYPE_WIFI);
		pr_info("[%s] wifi power off ret=[%d]", __func__, ret);

	} else if (par2 == 2) {
		/* whole chip reset */
		ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_WIFI, "reset test");
		pr_info("[%s] trigger chip reset ret=[%d]", __func__, ret);
	}

	return 0;
}

static int chip_rst_tc(int par1, int par2, int par3)
{
	pr_info("test start");
	//return chip_rst_test();
	return 0;
}

static int cal_tc(int par1, int par2, int par3)
{
	pr_info("test start");
	//return calibration_test();
	return 0;
}

static int v3_coredump_tc(int par1, int par2, int par3)
{
	pr_info("[%s][%d][%d][%d]", __func__, par1, par2, par3);

	connv3_dump_test(par1, par2, par3);
	return 0;
}

static int v3_hif_dump_tc(int par1, int par2, int par3)
{
#define TEST_ADDR	0x7C05B060
#define TEST_WRITE_VALUE	0xffffffff
	int ret, ret1, ret2;
	unsigned int cr_read = 0;

	ret = connv3_hif_dbg_start(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI);
	if (!ret) {
		ret = connv3_hif_dbg_read(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI, 0x7c011000, &cr_read);
		pr_info("[%s] read 0x7c011000=0x%08x, ret = %d\n", __func__, cr_read, ret);
		ret1 = connv3_hif_dbg_write(
			CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI,
			TEST_ADDR, TEST_WRITE_VALUE);
		ret2 = connv3_hif_dbg_read(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI, TEST_ADDR, &cr_read);
		if (cr_read != TEST_WRITE_VALUE)
			pr_info("[%s] write 0x%08x to 0x%08x fail, ret=[%d, %d]", __func__, TEST_ADDR, TEST_WRITE_VALUE, ret1, ret2);
		ret1 = connv3_hif_dbg_write_mask(
			CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI,
			TEST_ADDR, 0xff, 0x0);
		ret2 = connv3_hif_dbg_read(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI, TEST_ADDR, &cr_read);
		if (cr_read != 0xffffff00)
			pr_info("[%s] write mask fail, 0x%08x should be 0xffffff00, ret=[%d, %d]",
				__func__, TEST_ADDR, ret1, ret2);
		ret = connv3_hif_dbg_end(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI);
		pr_info("[%s] connv3_hif_dbg_end=%d\n", __func__, ret);
	}

	return 0;
}

extern int connv3_core_pmic_event_cb(unsigned int id, unsigned int event);

static int v3_dfd_rst_tc(int par1, int par2, int par3)
{
	int ret;

	pr_info("[%s][%d][%d][%d]", __func__, par1, par2, par3);

	if (par2 == 1) {
		ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_CONNV3, "reset test-1-1");
		pr_info("[%s] reset test-1-1, ret=%d\n", __func__, ret);
		ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_CONNV3, "reset test-1-2");
		pr_info("[%s] reset test-1-2, ret=%d\n", __func__, ret);
		osal_sleep_ms(2000);

		if (par3 >= 2) {
			ret = connv3_core_pmic_event_cb(0, 1);
			pr_info("[%s] reset test-2, ret=%d\n", __func__, ret);
			osal_sleep_ms(2000);
		}

		if (par3 >= 3) {
			ret = connv3_core_pmic_event_cb(1, 1);
			pr_info("[%s] reset test-3, ret=%d\n", __func__, ret);
		}
	} else if (par2 == 2) {
		ret = connv3_core_pmic_event_cb(1, 1);
		pr_info("[%s] reset test-2-1, ret=%d\n", __func__, ret);
		ret = connv3_core_pmic_event_cb(1, 1);
		pr_info("[%s] reset test-2-2, ret=%d\n", __func__, ret);

		if (par3 >= 2) {
			ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_CONNV3, "reset test-2-3");
			pr_info("[%s] reset test-2-3, ret=%d\n", __func__, ret);
		}
	} else if (par2 == 3) {
		ret = connv3_trigger_pmic_irq(CONNV3_DRV_TYPE_CONNV3, "reset test-3-1");
		pr_info("[%s] reset test-3-1, ret=%d\n", __func__, ret);
		ret = connv3_trigger_pmic_irq(CONNV3_DRV_TYPE_CONNV3, "reset test-3-2");
		pr_info("[%s] reset test-3-2, ret=%d\n", __func__, ret);
		ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_CONNV3, "reset test-3-3");
		pr_info("[%s] reset test-3-3, ret=%d\n", __func__, ret);
	} else if (par2 == 4) {
		ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_CONNV3, "reset test-4-1");
		pr_info("[%s] reset test-4-1, ret=%d\n", __func__, ret);
		ret = connv3_trigger_pmic_irq(CONNV3_DRV_TYPE_CONNV3, "reset test-4-2");
		pr_info("[%s] reset test-4-2, ret=%d\n", __func__, ret);
	}

	return 0;
}

static int v3_custom_config_tc(int par1, int par2, int par3)
{
	u32 size = 0;
	u8 *data = NULL;
	u32 i;

	data = connv3_get_plat_config(&size);

	pr_info("[%s] data = %p size = %d\n", __func__, data, size);
	if (size > 0 && data != NULL) {
		for (i = 0; i < size; i++) {
			pr_info("data[%d]=%x\n", i, data[i]);
		}
	}
	return 0;
}

#if 0
static int log_tc(int par1, int par2, int par3)
{
	/* 0: initial state
	 * 1: log has been init.
	 */
	static int log_status = 0;
	int ret = 0;

	if (par2 == 0) {
		if (log_status != 0) {
			pr_info("log has been init.\n");
			return 0;
		}
		/* init */
		ret = connlog_test_init();
		if (ret)
			pr_err("FW log init fail! ret=%d\n", ret);
		else {
			log_status = 1;
			pr_info("FW log init finish. Check result on EMI.\n");
		}
	} else if (par2 == 1) {
		/* add fake log */
		/* read log */
		connlog_test_read();
	} else if (par2 == 2) {
		/* deinit */
		if (log_status == 0) {
			pr_info("log didn't init\n");
			return 0;
		}
		ret = connlog_test_deinit();
		if (ret)
			pr_err("FW log deinit fail! ret=%d\n", ret);
		else
			log_status = 0;
	}
	return ret;
}
#endif

ssize_t connv3_test_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t connv3_test_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[256];
	char *pBuf;
	char *pDelimiter = " \t";
	int x = 0, y = 0, z = 0;
	char *pToken = NULL;
	long res = 0;
	static bool test_enabled = false;

	pr_info("write parameter len = %d\n\r", (int) len);
	if (len >= osal_sizeof(buf)) {
		pr_err("input handling fail!\n");
		len = osal_sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n\r", buf);

	pBuf = buf;
	pToken = osal_strsep(&pBuf, pDelimiter);
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		x = (int)res;
	} else {
		x = 0;
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		y = (int)res;
		pr_info("y = 0x%08x\n\r", y);
	} else {
		y = 3000;
		/*efuse, register read write default value */
		if (0x11 == x || 0x12 == x || 0x13 == x)
			y = 0x80000000;
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		z = (int)res;
	} else {
		z = 10;
		/*efuse, register read write default value */
		if (0x11 == x || 0x12 == x || 0x13 == x)
			z = 0xffffffff;
	}

	pr_info("x(0x%08x), y(0x%08x), z(0x%08x)\n\r", x, y, z);

	/* For eng and userdebug load, have to enable connv3_test by
	 * writing 0xDB9DB9 to "/proc/driver/connv3_test" to avoid
	 * some malicious use
	 */
	if (x == 0xDB9DB9) {
		test_enabled = true;
		return len;
	}

	if (!test_enabled)
		return 0;

	if (osal_array_size(connv3_test_func) > x &&
		NULL != connv3_test_func[x])
		(*connv3_test_func[x]) (x, y, z);
	else
		pr_warn("no handler defined for command id(0x%08x)\n\r", x);

	return len;

}


int connv3_test_setup(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	static const struct proc_ops connv3_test_fops = {
		.proc_read = connv3_test_read,
		.proc_write = connv3_test_write,
	};
#else
	static const struct file_operations connv3_test_fops = {
		.owner = THIS_MODULE,
		.read = connv3_test_read,
		.write = connv3_test_write,
	};
#endif
	int i_ret = 0;

	g_connv3_test_entry = proc_create(CONNV3_TEST_PROCNAME,
					0664, NULL, &connv3_test_fops);
	if (g_connv3_test_entry == NULL) {
		pr_err("Unable to create / wmt_aee proc entry\n\r");
		i_ret = -1;
	}

	return i_ret;
}

int connv3_test_remove(void)
{
	if (g_connv3_test_entry != NULL)
		proc_remove(g_connv3_test_entry);
	return 0;
}

