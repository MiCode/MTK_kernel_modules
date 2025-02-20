/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "gps_dl_config.h"

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#include "gps_dl_info_node.h"
#include "gps_dl_osal.h"
#include "gps_dl_hw_dep_api.h"

#if GPS_DL_GET_INFO_FROM_NODE

#define G_GPS_INFO_LENGTH 256
bool g_gps_dl_get_ecid_info_ready;
bool g_gps_dl_get_adie_info_ready;
char g_gps_dl_get_info[G_GPS_INFO_LENGTH];
char *g_gps_dl_get_info_ptr;
unsigned int g_gps_dl_info_length;
void gps_dl_info_node_set_ecid_info(unsigned long ecid_data1,
	unsigned long ecid_data2)
{
	int ret;

	if (g_gps_dl_get_ecid_info_ready == false) {
		ret = snprintf(g_gps_dl_get_info, G_GPS_INFO_LENGTH,
			"[MT6686P_ECID][0x%lx, 0x%lx]", ecid_data1, ecid_data2);
		if (ret <= 0)
			GDL_LOGW("snprintf fail");
		else
			g_gps_dl_get_ecid_info_ready = true;
	}
}

void gps_dl_info_node_set_adie_info(unsigned int adie_info)
{
	int ret;

	if (g_gps_dl_get_adie_info_ready == false) {
		ret = snprintf(g_gps_dl_get_info, G_GPS_INFO_LENGTH,
			"[MT6686_ADIE][0x%x]", adie_info);
		if (ret <= 0)
			GDL_LOGW("snprintf fail");
		else
			g_gps_dl_get_adie_info_ready = true;
	}
}

static struct proc_dir_entry *g_gps_dl_node_info_entry;
#define GPS_DL_PROCFS_NAME "driver/gpsdl_chip_info"

enum gps_dl_log_info_type {
	GPS_DL_ECID_INFO = 0,
	GPS_DL_ADIE_ID_INFO = 1,
	GPS_DL_MAX,
};

ssize_t gps_dl_info_node_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[256];
	char *pBuf;
	int x = 0, y = 0, z = 0;
	char *pToken = NULL;
	char *pDelimiter = " \t";
	long res = 0;
	int ret = 0;

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	GDL_LOGD("write parameter data = %s\n\r", buf);

	pBuf = buf;
	pToken = gps_dl_osal_strsep(&pBuf, pDelimiter);
	if (pToken != NULL) {
		if (gps_dl_osal_strtol(pToken, 16, &res) < 0)
			x = 0;
		else
			x = (int)res;
	} else {
		x = 0;
	}

	pToken = gps_dl_osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		gps_dl_osal_strtol(pToken, 16, &res);
		y = (int)res;
		GDL_LOGE("y = 0x%08x\n\r", y);
	}

	pToken = gps_dl_osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		gps_dl_osal_strtol(pToken, 16, &res);
		z = (int)res;
	}
	GDL_LOGW("x(0x%08x), y(0x%08x), z(0x%08x)", x, y, z);

	switch (x) {
	case GPS_DL_ECID_INFO:
	{
		if (g_gps_dl_get_ecid_info_ready) {
			g_gps_dl_get_info[G_GPS_INFO_LENGTH-1] = '\0';
			g_gps_dl_info_length = strlen(g_gps_dl_get_info);
		} else
			GDL_LOGW("g_gps_dl_get_ecid_info_ready is not set");
		break;
	}
	case GPS_DL_ADIE_ID_INFO:
	{
		if (g_gps_dl_get_adie_info_ready) {
			g_gps_dl_get_info[G_GPS_INFO_LENGTH-1] = '\0';
			g_gps_dl_info_length = strlen(g_gps_dl_get_info);
		} else
			GDL_LOGW("g_gps_dl_get_ecid_info_ready is not set");
		break;
	}
	default:
		ret = -1;
		GDL_LOGE("gps_dl_log_node_write,ERROR input y = %d, z = %d", y, z);
		break;
	}

	return len;
}

ssize_t gps_dl_info_node_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int ret = 0;
	int dump_len;

	if (g_gps_dl_info_length == 0)
		goto exit;

	if (*f_pos == 0)
		g_gps_dl_get_info_ptr = g_gps_dl_get_info;

	dump_len = g_gps_dl_info_length >= count ? count : g_gps_dl_info_length;
	ret = copy_to_user(buf, g_gps_dl_get_info_ptr, dump_len);
	if (ret) {
		GDL_LOGE("copy to dump info buffer failed, ret:%d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	*f_pos += dump_len;
	g_gps_dl_info_length -= dump_len;
	g_gps_dl_get_info_ptr += dump_len;
	GDL_LOGI("gps_dbg:after read,gps for dump info buffer len(%d)\n", g_gps_dl_info_length);

	ret = dump_len;
exit:
	return ret;
}

static const struct proc_ops gps_dl_log_node_fops = {
	.proc_read = gps_dl_info_node_read,
	.proc_write = gps_dl_info_node_write,
};

int gps_dl_info_node_setup(void)
{

	int i_ret = 0;

	g_gps_dl_node_info_entry = proc_create(GPS_DL_PROCFS_NAME, 0600,
		NULL, &gps_dl_log_node_fops);

	if (g_gps_dl_node_info_entry == NULL) {
		GDL_LOGE("Unable to create gps log node entry");
		i_ret = -1;
	}

	return i_ret;
}

int gps_dl_info_node_remove(void)
{
	if (g_gps_dl_node_info_entry != NULL)
		proc_remove(g_gps_dl_node_info_entry);
	return 0;
}
#endif

