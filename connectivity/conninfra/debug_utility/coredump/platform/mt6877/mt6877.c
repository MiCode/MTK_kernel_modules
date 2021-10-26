// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/printk.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "consys_reg_util.h"
#include "connsys_debug_utility.h"
#include "connsys_coredump_hw_config.h"

static char* wifi_task_str[] = {
	"Task_WIFI",
	"Task_TST_WFSYS",
	"Task_Idle_WFSYS",
};

static char* bt_task_str[] = {
	"Task_WMT",
	"Task_BT",
	"Task_TST_BTSYS",
	"Task_BT2",
	"Task_Idle_BTSYS",
};

struct coredump_hw_config g_coredump_config[CONN_DEBUG_TYPE_END] = {
	/* Wi-Fi config */
	{
		.name = "WFSYS",
		.start_offset = 0x0495400,
		.size = 0x18000,
		.seg1_cr = 0x1840012c,
		.seg1_value_end = 0x187fffff,
		.seg1_start_addr = 0x18400120,
		.seg1_phy_addr = 0x18500000,
		.task_table_size = sizeof(wifi_task_str)/sizeof(char*),
		.task_map_table = wifi_task_str,
		.exception_tag_name = "combo_wifi",
	},
	/* BT config */
	{
		.name = "BTSYS",
		.start_offset = 0x43C00,
		.size = 0x18000,
		.seg1_cr = 0x18823024,
		.seg1_value_end = 0x18bfffff,
		.seg1_start_addr = 0x18823014,
		.seg1_phy_addr = 0x18900000,
		.task_table_size = sizeof(bt_task_str)/sizeof(char*),
		.task_map_table = bt_task_str,
		.exception_tag_name = "combo_bt",
	},
};

struct coredump_hw_config* get_coredump_platform_config(int conn_type)
{
	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_err("Incorrect type: %d\n", conn_type);
		return NULL;
	}
	return &g_coredump_config[conn_type];
}

unsigned int get_coredump_platform_chipid(void)
{
	return 0x6877;
}

char* get_task_string(int conn_type, unsigned int task_id)
{
	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_err("Incorrect type: %d\n", conn_type);
		return NULL;
	}

	if (task_id > g_coredump_config[conn_type].task_table_size) {
		pr_err("[%s] Incorrect task: %d\n",
			g_coredump_config[conn_type].name, task_id);
		return NULL;
	}

	return g_coredump_config[conn_type].task_map_table[task_id];
}

char* get_sys_name(int conn_type)
{
	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_err("Incorrect type: %d\n", conn_type);
		return NULL;
	}
	return g_coredump_config[conn_type].name;
}

bool is_host_view_cr(unsigned int addr, unsigned int* host_view)
{
	if (addr >= 0x7C000000 && addr <= 0x7Cffffff) {
		if (host_view) {
			*host_view = ((addr - 0x7c000000) + 0x18000000);
		}
		return true;
	} else if (addr >= 0x18000000 && addr <= 0x18ffffff) {
		if (host_view) {
			*host_view = addr;
		}
		return true;
	}
	return false;
}

bool is_host_csr_readable(void)
{
	void __iomem *vir_addr = NULL;
	bool ret = false;
	unsigned int r;

	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1228 [19] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0  (CR at ap side)
	 */
	vir_addr = ioremap(0x10001228, 0x4);
	if (vir_addr) {
		r = CONSYS_REG_READ_BIT(vir_addr, ((0x1 << 19) | (0x1 << 13)));
		if (r == 0) {
			ret = true;
		}
		iounmap(vir_addr);
	} else
		pr_info("[%s] remap 0x10001228 fail", __func__);

	return ret;
}

enum cr_category get_cr_category(unsigned int addr)
{
	if (addr >= 0x7c000000 && addr <= 0x7c3fffff) {
		if (addr >= 0x7c060000 && addr <= 0x7c06ffff) {
			return CONN_HOST_CSR;
		}
		return CONN_INFRA_CR;
	}

	return SUBSYS_CR;
}
