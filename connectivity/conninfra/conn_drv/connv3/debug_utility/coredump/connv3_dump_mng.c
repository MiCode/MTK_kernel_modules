// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/printk.h>

#include "connv3_debug_utility.h"
#include "coredump/connv3_dump_mng.h"

#define COMBO_WIFI	"combo_wifi"
#define COMBO_BT	"combo_bt"

#define CONNV3_SUBSYS_WIFI	"WFSYS"
#define CONNV3_SUBSYS_BT	"BTSYS"

static struct connv3_coredump_platform_ops *g_dump_plat_data = NULL;

unsigned int connv3_dump_mng_get_platform_chipid(void)
{
	if (g_dump_plat_data &&
	    g_dump_plat_data->connv3_dump_plt_get_chipid)
		return g_dump_plat_data->connv3_dump_plt_get_chipid();

	return 0x0;
}

char* connv3_dump_mng_get_exception_tag_name(int conn_type)
{
	if (conn_type == CONNV3_DEBUG_TYPE_WIFI)
		return COMBO_WIFI;
	else if (conn_type == CONNV3_DEBUG_TYPE_BT)
		return COMBO_BT;
	else
		return NULL;
}

char* connv3_dump_mng_get_subsys_tag(int conn_type)
{
	if (conn_type == CONNV3_DEBUG_TYPE_WIFI)
		return CONNV3_SUBSYS_WIFI;
	else if (conn_type == CONNV3_DEBUG_TYPE_BT)
		return CONNV3_SUBSYS_BT;
	else
		return NULL;
}

unsigned int connv3_dump_mng_exception_filter(char* exp_log)
{
	if (g_dump_plat_data &&
		g_dump_plat_data->connv3_dump_plt_exception_filter)
		return g_dump_plat_data->connv3_dump_plt_exception_filter(exp_log);

	return 0;
}

int connv3_dump_mng_init(void* plat_data)
{

	if (plat_data == NULL) {
		pr_err("[%s] invalid input", __func__);
		return -1;
	}

	g_dump_plat_data = (struct connv3_coredump_platform_ops*)plat_data;
	return 0;
}
