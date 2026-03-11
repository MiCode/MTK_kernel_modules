// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/ratelimit.h>
#include <linux/alarmtimer.h>
#include <linux/suspend.h>
#include <linux/io.h>

#include "conninfra.h"
#include "metlog.h"
#include "osal.h"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

struct conn_metlog_data {
	OSAL_THREAD met_thread;
	struct conn_metlog_info info;
};

struct conn_metlog_table {
	struct conn_metlog_data sys_wifi;
	struct conn_metlog_data sys_bt;
	struct conn_metlog_data sys_gps;
};

#define CONSYS_MET_WAIT	(1000*20) /* ms */

static struct conn_metlog_table g_table;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
static int met_addr_is_valid(unsigned int addr, struct conn_metlog_info *info)
{
	if (addr >= info->met_base_fw && addr < (info->met_base_fw + info->met_size))
		return 1;
	else
		return 0;
}

static unsigned int met_get_next_ptr(unsigned int addr, struct conn_metlog_info *info)
{
	unsigned int next_addr;

	if (info->output_len == 0)
		return 0;

	next_addr = addr + info->output_len / 8;

	return next_addr;
}

static void met_print_data(unsigned int addr, struct conn_metlog_info *info, unsigned char *met_base)
{
	unsigned int value = 0, value2 = 0;

	if (info->output_len == 32) {
		if (!met_addr_is_valid(addr + 3, info)) {
			pr_info("%s, addr(0x%08x) is invalid", __func__, addr + 4);
			return;
		}
		value = readl(met_base + addr - info->met_base_fw);
		pr_info("MCU_MET_DATA:0x%08x", value);
	} else if (info->output_len == 64) {
		if (!met_addr_is_valid(addr + 7, info)) {
			pr_info("%s, addr(0x%08x) is invalid", __func__, addr + 8);
			return;
		}
		value = readl(met_base + addr - info->met_base_fw);
		value2 = readl(met_base + addr - info->met_base_fw + 4);
		pr_info("MCU_MET_DATA:0x%08x%08x", value2, value);
	}
}

static int met_thread(void *pvData)
{
	struct conn_metlog_data *data = (struct conn_metlog_data *) pvData;
	struct conn_metlog_info *info;
	unsigned char *met_read_cr = NULL;
	unsigned char *met_write_cr = NULL;
	unsigned char *met_base = NULL;
	unsigned int read_ptr = 0, next_read_ptr = 0;
	unsigned int write_ptr = 0;
	int ret = -1;

	if (data == NULL) {
		pr_info("p_metlog_data(NULL)\n");
		goto met_exit;
	}

	info = &data->info;
	pr_info("type %d\n", info->type);
	pr_info("read_cr %llx, write_cr %llx\n", info->read_cr, info->write_cr);
	pr_info("met_base_ap %llx met_base_fw 0x%x, met_size 0x%x, output_len %d\n",
		info->met_base_ap, info->met_base_fw, info->met_size, info->output_len);

	if (info->read_cr == 0 || info->write_cr == 0 || info->met_base_ap == 0 ||
		info->met_base_fw == 0 || info->met_size == 0) {
		pr_info("invalid parameter\n");
		goto met_exit;
	}

	if (info->output_len % 32 != 0 || info->output_len == 0) {
		pr_info("met output length(%d) is wrong\n", info->output_len);
		goto met_exit;
	}

	met_read_cr = ioremap(info->read_cr, 0x10);
	if (!met_read_cr) {
		pr_info("met_read_cr ioremap fail\n");
		goto met_exit;
	}

	met_write_cr = ioremap(info->write_cr, 0x10);
	if (!met_write_cr) {
		pr_info("met_write_cr ioremap fail\n");
		goto met_exit;
	}

	met_base = ioremap(info->met_base_ap, info->met_size);
	if (!met_base) {
		pr_info("met_base ioremap fail\n");
		goto met_exit;
	}

	/* We have to check whether write_ptr is valid which means HW starts to write MET data. */
	/* write_cr: recording the address for "next" write */
	/* read_cr: recording the address for "last" read */
	while (1) {
		if (osal_thread_should_stop(&data->met_thread)) {
			pr_info("met thread should stop now...\n");
			ret = 0;
			goto met_exit;
		}

		write_ptr = readl(met_write_cr);
		if (write_ptr > info->met_base_fw && write_ptr < info->met_base_fw + info->met_size) {
			/* update read_ptr to end of met data */
			read_ptr = (info->met_base_fw + info->met_size - info->output_len/8);
			break;
		}
		osal_usleep_range(CONSYS_MET_WAIT, CONSYS_MET_WAIT);
	}

	while (1) {
		if (osal_thread_should_stop(&data->met_thread)) {
			pr_info("met thread should stop now...\n");
			ret = 0;
			goto met_exit;
		}

		if (!met_addr_is_valid(read_ptr, info) || !met_addr_is_valid(write_ptr, info))
			pr_info("read_ptr(0x%x) or write_ptr(0x%x) is invalid!!!\n",
				read_ptr, write_ptr);
		else {
			next_read_ptr = met_get_next_ptr(read_ptr, info);
			if (next_read_ptr == write_ptr)
				pr_info("no met data need to dump!!!\n");
			else {
				if (next_read_ptr > write_ptr) {
					while (next_read_ptr < (info->met_base_fw + info->met_size)) {
						met_print_data(next_read_ptr, info, met_base);
						read_ptr = next_read_ptr;
						next_read_ptr = met_get_next_ptr(next_read_ptr, info);
					}
					next_read_ptr = info->met_base_fw;
				}
				while (next_read_ptr <= (write_ptr - info->output_len / 8)) {
					met_print_data(next_read_ptr, info, met_base);
					read_ptr = next_read_ptr;
					next_read_ptr = met_get_next_ptr(next_read_ptr, info);
				}
				writel(read_ptr, met_read_cr);
			}
		}
		osal_usleep_range(CONSYS_MET_WAIT, CONSYS_MET_WAIT);

		write_ptr = readl(met_write_cr);
	}
	ret = 0;

met_exit:
	if (met_read_cr)
		iounmap(met_read_cr);
	if (met_write_cr)
		iounmap(met_write_cr);
	if (met_base)
		iounmap(met_base);
	pr_info("met thread exits succeed\n");

	return ret;
}

/*****************************************************************************
* FUNCTION
*  conn_metlog_start
* DESCRIPTION
*  XXXXXXXXXXXXXXXXXXXXX
* PARAMETERS
*  struct conn_metlog_info *info
* RETURNS
*  0 means successful, or negative value will be returned.
*****************************************************************************/
int conn_metlog_start(struct conn_metlog_info *info)
{
	int ret = 0;
	P_OSAL_THREAD p_thread;
	struct conn_metlog_data *data;
	char thread_name[20];

	if (!info) {
		pr_info("%s: info is null.\n", __func__);
		return -1;
	}

	if (info->type == CONNDRV_TYPE_WIFI) {
		data = &g_table.sys_wifi;
		ret = osal_snprintf(thread_name, sizeof(thread_name), "conn_metlog_wifi");
	} else if (info->type == CONNDRV_TYPE_BT) {
		data = &g_table.sys_bt;
		ret = osal_snprintf(thread_name, sizeof(thread_name), "conn_metlog_bt");
	} else if (info->type == CONNDRV_TYPE_GPS) {
		data = &g_table.sys_gps;
		ret = osal_snprintf(thread_name, sizeof(thread_name), "conn_metlog_gps");
	} else {
		pr_info("%s: type %d is invalid.\n", __func__, info->type);
		return -1;
	}

	if (ret < 0) {
		pr_info("snprintf fail! ret = %d, type = %d\n", ret, info->type);
		return -1;
	}

	p_thread = &data->met_thread;
	p_thread->pThreadData = (void *)data;
	p_thread->pThreadFunc = (void *)met_thread;

	osal_memcpy(&data->info, info, sizeof(struct conn_metlog_info));
	osal_strncpy(p_thread->threadName, thread_name, sizeof(p_thread->threadName));

	ret = osal_thread_create(p_thread);
	if (ret) {
		pr_info("osal_thread_create(0x%p) fail(%d)\n", p_thread, ret);
		return -2;
	}

	ret = osal_thread_run(p_thread);
	if (ret) {
		pr_info("osal_thread_run(evt_thread 0x%p) fail(%d)\n", p_thread, ret);
		return -3;
	}

	return 0;
}
EXPORT_SYMBOL(conn_metlog_start);

int conn_metlog_stop(int type)
{
	P_OSAL_THREAD p_thread;
	int ret;

	if (type == CONNDRV_TYPE_WIFI)
		p_thread = &g_table.sys_wifi.met_thread;
	else if (type == CONNDRV_TYPE_BT)
		p_thread = &g_table.sys_bt.met_thread;
	else if (type == CONNDRV_TYPE_GPS)
		p_thread = &g_table.sys_gps.met_thread;
	else {
		pr_info("%s: type %d is invalid.\n", __func__, type);
		return -1;
	}

	ret = osal_thread_stop(p_thread);
	if (ret) {
		pr_info("osal_thread_stop(0x%p) fail(%d)\n", p_thread, ret);
		return -2;
	}

	pr_info("%s, type = %d\n", __func__, type);
	return 0;
}
EXPORT_SYMBOL(conn_metlog_stop);
