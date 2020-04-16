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

#define pr_fmt(fmt) "conninfra_test@(%s:%d) " fmt, __func__, __LINE__

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include "conninfra_test.h"
#include "osal.h"

#include "conninfra.h"
#include "conninfra_core.h"
#include "conf_test.h"
#include "cal_test.h"
#include "msg_evt_test.h"
#include "chip_rst_test.h"
#include "mailbox_test.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define CONNINFRA_TEST_PROCNAME "driver/conninfra_test"

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

static ssize_t conninfra_test_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t conninfra_test_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

static int core_tc(int par1, int par2, int par3);
static int conf_tc(int par1, int par2, int par3);
static int cal_tc(int par1, int par2, int par3);
static int msg_evt_tc(int par1, int par2, int par3);
static int chip_rst_tc(int par1, int par2, int par3);
static int mailbox_tc(int par1, int par2, int par3);
static int emi_tc(int par1, int par2, int par3);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

static struct proc_dir_entry *gConninfraTestEntry;

static const CONNINFRA_TEST_FUNC conninfra_test_func[] = {
	[0x01] = core_tc,
	[0x02] = conf_tc,
	[0x03] = msg_evt_tc,
	[0x04] = chip_rst_tc,
	[0x05] = cal_tc,
	[0x06] = mailbox_tc,
	[0x07] = emi_tc,
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

int core_tc_pwr_on()
{
	int iret = 0;

	pr_info("Power on test start");
	iret = conninfra_core_power_on(CONNDRV_TYPE_BT);
	pr_info("BT power on %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(100);
	iret = conninfra_core_power_on(CONNDRV_TYPE_FM);
	pr_info("FM power on %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(100);
	iret = conninfra_core_power_on(CONNDRV_TYPE_GPS);
	pr_info("GPS power on %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(100);
	iret = conninfra_core_power_on(CONNDRV_TYPE_WIFI);
	pr_info("Wi-Fi power on %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(200);

	return iret;
}

int core_tc_pwr_off()
{
	int iret = 0;

	iret = conninfra_core_power_off(CONNDRV_TYPE_WIFI);
	pr_info("Wi-Fi power off %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(100);
	iret = conninfra_core_power_off(CONNDRV_TYPE_GPS);
	pr_info("GPS power off %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(100);
	iret = conninfra_core_power_off(CONNDRV_TYPE_BT);
	pr_info("BT power off %s (result = %d)", iret? "fail" : "pass", iret);
	osal_sleep_ms(100);
	iret = conninfra_core_power_off(CONNDRV_TYPE_FM);
	pr_info("FM power off %s (result = %d)", iret? "fail" : "pass", iret);

	return iret;
}


int core_tc(int par1, int par2, int par3)
{
	int iret = 0;

	if (par2 == 0) {
		iret = core_tc_pwr_on();
		iret = core_tc_pwr_off();
	} else if (par2 == 1) {
		iret = core_tc_pwr_on();
	} else if (par2 == 2) {
		iret = core_tc_pwr_off();
	}
	//pr_info("core_tc %s (result = %d)", iret? "fail" : "pass", iret);
	return 0;
}

static int conf_tc(int par1, int par2, int par3)
{
	return conninfra_conf_test();
}

static int msg_evt_tc(int par1, int par2, int par3)
{
	return msg_evt_test();
}

static int chip_rst_tc(int par1, int par2, int par3)
{
	pr_info("test start");
	return chip_rst_test();
}

static int cal_tc(int par1, int par2, int par3)
{
	pr_info("test start");
	return calibration_test();
}


static int mailbox_tc(int par1, int par2, int par3)
{
	return mailbox_test();
}

static int emi_tc(int par1, int par2, int par3)
{
	unsigned int addr = 0;
	unsigned int size = 0;
	int ret = 0;

	pr_info("[%s] start", __func__);
	conninfra_get_phy_addr(&addr, &size);
	if (addr == 0 || size == 0) {
		pr_err("[%s] fail! addr=[0x%x] size=[%u]", __func__, addr, size);
		ret = -1;
	} else
		pr_info("[%s] pass. addr=[0x%x] size=[%u]", __func__, addr, size);

	pr_info("[%s] end", __func__);

	return ret;
}

ssize_t conninfra_test_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t conninfra_test_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[256];
	char *pBuf;
	char *pDelimiter = " \t";
	int x = 0, y = 0, z = 0;
	char *pToken = NULL;
	long res;
	//static bool testEnabled;

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

	/* For eng and userdebug load, have to enable wmt_dbg by
	 * writing 0xDB9DB9 to * "/proc/driver/wmt_dbg" to avoid
	 * some malicious use
	 */
#if 0
	if (x == 0xDB9DB9) {
		dbgEnabled = 1;
		return len;
	}
#endif

	if (osal_array_size(conninfra_test_func) > x &&
		NULL != conninfra_test_func[x])
		(*conninfra_test_func[x]) (x, y, z);
	else
		pr_warn("no handler defined for command id(0x%08x)\n\r", x);

	return len;

}


int conninfra_test_setup(void)
{
	static const struct file_operations conninfra_test_fops = {
		.owner = THIS_MODULE,
		.read = conninfra_test_read,
		.write = conninfra_test_write,
	};
	int i_ret = 0;

	gConninfraTestEntry = proc_create(CONNINFRA_TEST_PROCNAME,
					0664, NULL, &conninfra_test_fops);
	if (gConninfraTestEntry == NULL) {
		pr_err("Unable to create / wmt_aee proc entry\n\r");
		i_ret = -1;
	}

	return i_ret;
}

int conninfra_test_remove(void)
{
	if (gConninfraTestEntry != NULL)
		proc_remove(gConninfraTestEntry);
	return 0;
}

