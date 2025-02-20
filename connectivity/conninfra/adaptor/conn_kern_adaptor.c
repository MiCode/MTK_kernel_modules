// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "osal.h"
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
#include "wmt_build_in_adapter.h"
#endif

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CONN_ADAPTOR_DBG_DUMP_BUF_SIZE 1024

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

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
static ssize_t conn_adaptor_dbg_write(struct file *, const char __user *, size_t, loff_t *);
static ssize_t conn_adaptor_dbg_read(struct file *, char __user *, size_t, loff_t *);
#endif

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
int (*g_kern_dbg_handler)(int x, int y, int z, char* buf, int buf_sz);

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
/* dbg */
static const struct wmt_platform_dbg_bridge g_plat_dbg_bridge = {
	.write_cb = conn_adaptor_dbg_write,
	.read_cb = conn_adaptor_dbg_read,
};

static char g_conn_adaptor_dump_buf[CONN_ADAPTOR_DBG_DUMP_BUF_SIZE];
static OSAL_SLEEPABLE_LOCK g_conn_adaptor_dump_lock;
static int g_dump_buf_len;
static char *g_dump_buf_ptr;

#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
static ssize_t conn_adaptor_dbg_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[256];
	char* pBuf;
	int x = 0, y = 0, z = 0;
	char* pToken = NULL;
	char* pDelimiter = " \t";
	long res = 0;

	pr_info("[%s] write parameter len = %d", __func__, (int) len);
	if (len >= osal_sizeof(buf)) {
		pr_err("input handling fail!\n");
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
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		z = (int)res;
	}
	pr_info("[%s] x(0x%08x), y(0x%08x), z(0x%08x)", __func__, x, y, z);

	{
		int offset = 0;
		char* buf = g_conn_adaptor_dump_buf;
		int ret, sz;

		ret = osal_lock_sleepable_lock(&g_conn_adaptor_dump_lock);
		if (ret) {
			pr_err("dump_lock fail!!");
			return ret;
		}

		if (g_kern_dbg_handler) {
			sz = (*g_kern_dbg_handler)(x, y, z, buf, CONN_ADAPTOR_DBG_DUMP_BUF_SIZE);
			if (sz > 0)
				offset += sz;
		}
		g_dump_buf_len = offset;
		osal_unlock_sleepable_lock(&g_conn_adaptor_dump_lock);
	}

	return len;
}

static ssize_t conn_adaptor_dbg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret = 0;
	int dump_len;

	ret = osal_lock_sleepable_lock(&g_conn_adaptor_dump_lock);
	if (ret) {
		pr_err("dump_lock fail!!");
		return ret;
	}

	if (g_dump_buf_len == 0)
		goto exit;

	if (*f_pos == 0)
		g_dump_buf_ptr = g_conn_adaptor_dump_buf;

	dump_len = g_dump_buf_len >= count ? count : g_dump_buf_len;
	ret = copy_to_user(buf, g_dump_buf_ptr, dump_len);
	if (ret) {
		pr_err("copy to dump info buffer failed, ret:%d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	*f_pos += dump_len;
	g_dump_buf_len -= dump_len;
	g_dump_buf_ptr += dump_len;
	pr_info("conn_dbg:after read,wmt for dump info buffer len(%d)\n", g_dump_buf_len);

	ret = dump_len;
exit:
	osal_unlock_sleepable_lock(&g_conn_adaptor_dump_lock);
	return ret;
}
#endif

int conn_kern_adaptor_init(int (*dbg_handler)(int x, int y, int z, char *buf, int buf_sz))
{
	g_kern_dbg_handler = dbg_handler;

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	osal_sleepable_lock_init(&g_conn_adaptor_dump_lock);
	wmt_export_platform_dbg_bridge_register(&g_plat_dbg_bridge);
#endif
	return 0;
}

int conn_kern_adaptor_deinit(void)
{
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	wmt_export_platform_dbg_bridge_unregister();
#endif
	return 0;
}
