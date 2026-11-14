/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include "connv3.h"
#include "connv3_core.h"
#include "connv3_dbg.h"
#include "osal.h"


#define CONNV3_DBG_SUPPORT_READ 0 /* TBD */
#define CONNV3_DBG_PROCNAME "driver/connv3_dbg"


static struct proc_dir_entry *g_connv3_dbg_entry;;

#if CONNV3_DBG_SUPPORT_READ
#define CONNINFRA_DBG_DUMP_BUF_SIZE 1024
char g_dump_buf[CONNINFRA_DBG_DUMP_BUF_SIZE];
char *g_dump_buf_ptr;
int g_dump_buf_len;
static OSAL_SLEEPABLE_LOCK g_dump_lock;
#endif /* CONNV3_DBG_SUPPORT_READ */


#if CONNINFRA_DBG_SUPPORT
int connv3_dbg_hwver_get(int par1, int par2, int par3); /* connv3_dev_dbg_func[0x0] */
int connv3_dbg_set_pmic_en0(int par1, int par2, int par3); /* connv3_dev_dbg_func[0x1] */
int connv3_dbg_set_pmic_en1(int par1, int par2, int par3); /* connv3_dev_dbg_func[0x2] */
int connv3_dbg_toggle_conn_rst(int par1, int par2, int par3); /* connv3_dev_dbg_func[0x3] */
#endif /* CONNINFRA_DBG_SUPPORT */


static const CONNV3_DEV_DBG_FUNC connv3_dev_dbg_func[] = {
#if CONNINFRA_DBG_SUPPORT
	[0x0] = connv3_dbg_hwver_get,
	[0x1] = connv3_dbg_set_pmic_en0,
	[0x2] = connv3_dbg_set_pmic_en1,
	[0x3] = connv3_dbg_toggle_conn_rst,
#endif /* CONNINFRA_DBG_SUPPORT */
};

#if CONNINFRA_DBG_SUPPORT
int connv3_dbg_hwver_get(int par1, int par2, int par3)
{
	pr_info("%s[%d], query chip version\n", __func__, __LINE__);
	/* TODO: */

	return 0;
}

int connv3_dbg_set_pmic_en0(int par1, int par2, int par3)
{
	connv3_core_set_pmic_en0(par2);
	pr_info("%s[%d], en = %d\n", __func__, __LINE__, par2);

	return 0;
}

int connv3_dbg_set_pmic_en1(int par1, int par2, int par3)
{
	connv3_core_set_pmic_en1(par2);
	pr_info("%s[%d], en = %d\n", __func__, __LINE__, par2);

	return 0;
}

int connv3_dbg_toggle_conn_rst(int par1, int par2, int par3)
{
	connv3_core_toggle_conn_rst();
	pr_info("%s[%d]\n", __func__, __LINE__);

	return 0;
}


#endif /* CONNINFRA_DBG_SUPPORT */

#if CONNV3_DBG_SUPPORT_READ
ssize_t connv3_dbg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret = 0;
	int dump_len;

	ret = osal_lock_sleepable_lock(&g_dump_lock);
	if (ret) {
		pr_info("dump_lock fail!!");
		return ret;
	}

	if (g_dump_buf_len == 0)
		goto exit;

	if (*f_pos == 0)
		g_dump_buf_ptr = g_dump_buf;

	dump_len = g_dump_buf_len >= count ? count : g_dump_buf_len;
	ret = copy_to_user(buf, g_dump_buf_ptr, dump_len);
	if (ret) {
		pr_info("copy to dump info buffer failed, ret:%d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	*f_pos += dump_len;
	g_dump_buf_len -= dump_len;
	g_dump_buf_ptr += dump_len;
	pr_info("connv3_dbg:after read,wmt for dump info buffer len(%d)\n", g_dump_buf_len);

	ret = dump_len;
exit:

	osal_unlock_sleepable_lock(&g_dump_lock);
	return ret;
}
#endif /* CONNV3_DBG_SUPPORT_READ */

ssize_t connv3_dbg_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[256];
	char* pBuf;
	int x = 0, y = 0, z = 0;
	char* pToken = NULL;
	char* pDelimiter = " \t";
	long res = 0;
	static char dbg_enabled;

	pr_info("write parameter len = %d\n\r", (int) len);
	if (len >= osal_sizeof(buf)) {
		pr_info("input handling fail!\n");
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
		y = 0;
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		z = (int)res;
	} else {
		z = 0;
	}

	pr_info("x(0x%08x), y(0x%08x), z(0x%08x)\n\r", x, y, z);

	/* For eng and userdebug load, have to enable wmt_dbg by writing 0xDB9DB9 to
	 * "/proc/driver/wmt_dbg" to avoid some malicious use
	 */
#if CONNINFRA_DBG_SUPPORT
	if (0xDB9DB9 == x) {
		dbg_enabled = 1;
		return len;
	}
#endif /* CONNINFRA_DBG_SUPPORT */
	/* For user load, only 0x13, 0x14 and 0x40 is allowed to execute */
	/* allow command 0x2e to enable catch connsys log on userload  */
	if (0 == dbg_enabled && (x != 0x13) && (x != 0x14) && (x != 0x40)) {
		pr_info("please enable connv3 debug first\n\r");
		return len;
	}

	if (osal_array_size(connv3_dev_dbg_func) > x && NULL != connv3_dev_dbg_func[x])
		(*connv3_dev_dbg_func[x]) (x, y, z);
	else
		pr_info("no handler defined for command id(0x%08x)\n\r", x);

	return len;
}

int connv3_dev_dbg_init(void)
{
	static const struct proc_ops connv3_dbg_fops = {
#if CONNV3_DBG_SUPPORT_READ
		.proc_read = connv3_dbg_read,
#endif /* CONNV3_DBG_SUPPORT_READ */
		.proc_write = connv3_dbg_write,
	};

	int i_ret = 0;

	g_connv3_dbg_entry = proc_create(CONNV3_DBG_PROCNAME, 0664, NULL, &connv3_dbg_fops);
	if (g_connv3_dbg_entry == NULL) {
		pr_info("Unable to create connv3_dbg proc entry\n\r");
		i_ret = -1;
	}

	return i_ret;
}

int connv3_dev_dbg_deinit(void)
{
	return 0;
}

