// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/printk.h>

#include "../../../../include/conninfra.h"
#include "../../debug_utility/coredump/connsys_coredump_hw_config.h"
#include "../../debug_utility/coredump/coredump_mng.h"
#include "../include/connsys_debug_utility.h"
#include "../include/consys_hw.h"
#include "../include/consys_reg_util.h"
#include "include/mt6989_coredump.h"
#include "include/mt6989_consys_reg_offset.h"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
static char *gps_task_str[] = {
	"Task_GPS",
	"Task_TST_GPSSYS",
	"Task_Idle_GPSSYS",
};

static struct coredump_hw_config g_coredump_config[CONN_DEBUG_TYPE_END] = {
	/* Wi-Fi config */
	{
	},
	/* BT config */
	{
	},
	/* GPS config */
	{
		.name = "GPSSYS",
		.start_offset = 0x4a8000,
		.size = 0x18000,
		.seg1_cr = 0x18c16024,
		.seg1_value_end = 0x18ffffff,
		.seg1_start_addr = 0x18c16014,
		.seg1_phy_addr = 0x18d00000, // start of dynamic remapping 0
		.task_table_size = sizeof(gps_task_str)/sizeof(char *),
		.task_map_table = gps_task_str,
		.exception_tag_name = "combo_gps",
	},
};

struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6989 = {
	.consys_coredump_get_platform_config = consys_plt_coredump_get_platform_config_mt6989,
	.consys_coredump_get_platform_chipid = consys_plt_coredump_get_platform_chipid_mt6989,
	.consys_coredump_get_task_string = consys_plt_coredump_get_task_string_mt6989,
	.consys_coredump_get_sys_name = consys_plt_coredump_get_sys_name_mt6989,
	.consys_coredump_is_host_view_cr = consys_plt_coredump_is_host_view_cr_mt6989,
	.consys_coredump_is_host_csr_readable = consys_plt_coredump_is_host_csr_readable_mt6989,
	.consys_coredump_get_cr_category = consys_plt_coredump_get_cr_category_mt6989,
	.consys_coredump_get_emi_offset = consys_plt_coredump_get_emi_offset_mt6989,
	.consys_coredump_get_emi_phy_addr = consys_plt_coredump_get_emi_phy_addr_mt6989,
	.consys_coredump_get_mcif_emi_phy_addr = consys_plt_coredump_get_mcif_emi_phy_addr_mt6989,
	.consys_coredump_setup_dynamic_remap = consys_plt_coredump_setup_dynamic_remap_mt6989,
	.consys_coredump_remap = consys_plt_coredump_remap_mt6989,
	.consys_coredump_unmap = consys_plt_coredump_unmap_mt6989,
	.consys_coredump_get_tag_name = consys_plt_coredump_get_tag_name_mt6989,
	.consys_coredump_is_supported = consys_plt_coredump_is_supported_mt6989,
	.consys_coredump_get_emi_dump_offset = consys_plt_coredump_get_emi_dump_offset_mt6989,
};

struct coredump_hw_config *consys_plt_coredump_get_platform_config_mt6989(int conn_type)
{
	if (conn_type != CONN_DEBUG_TYPE_GPS) {
		pr_notice("[%s] incorrect type: %d\n", __func__, conn_type);
		return NULL;
	}

	return &g_coredump_config[conn_type];
}

unsigned int consys_plt_coredump_get_platform_chipid_mt6989(void)
{
	return 0x6989;
}

char *consys_plt_coredump_get_task_string_mt6989(int conn_type, unsigned int task_id)
{
	if (conn_type != CONN_DEBUG_TYPE_GPS) {
		pr_notice("[%s] incorrect type: %d\n", __func__, conn_type);
		return NULL;
	}

	if (task_id > g_coredump_config[conn_type].task_table_size) {
		pr_err("[%s] Incorrect task: %d\n",
			g_coredump_config[conn_type].name, task_id);
		return NULL;
	}

	return g_coredump_config[conn_type].task_map_table[task_id];
}

char *consys_plt_coredump_get_sys_name_mt6989(int conn_type)
{
	if (conn_type != CONN_DEBUG_TYPE_GPS) {
		pr_notice("[%s] incorrect type: %d\n", __func__, conn_type);
		return NULL;
	}
	return g_coredump_config[conn_type].name;
}

bool consys_plt_coredump_is_host_view_cr_mt6989(unsigned int addr, unsigned int *host_view)
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

bool consys_plt_coredump_is_host_csr_readable_mt6989(void)
{
	unsigned int tx, rx;

	/* Check ap2conn slpprot
	 * 0x1002_C1CC[0] / 0x1002_C00C[25](rx/tx) (sleep protect enable ready)
	 * both of them should be 1'b0
	 */
	rx = CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR + 0x1cc); // connsys_protect_rdy
	tx = CONSYS_REG_READ(INFRABUS_AO_REG_BASE_ADDR + 0xc); // infrasys_protect_rdy

	if (rx & (0x1 << 0) || tx & (0x1 << 25))
		return false;

	return true;
}

enum cr_category consys_plt_coredump_get_cr_category_mt6989(unsigned int addr)
{
	if (addr >= 0x7c000000 && addr <= 0x7c3fffff) {
		if (addr >= 0x7c060000 && addr <= 0x7c06ffff) {
			return CONN_HOST_CSR;
		}
		return CONN_INFRA_CR;
	}

	return SUBSYS_CR;
}

unsigned int consys_plt_coredump_get_emi_offset_mt6989(int conn_type)
{
	struct coredump_hw_config *config = coredump_mng_get_platform_config(conn_type);

	if (config)
		return config->start_offset;
	return 0;

}

void consys_plt_coredump_get_emi_phy_addr_mt6989(phys_addr_t *base, unsigned int *size)
{
	conninfra_get_emi_phy_addr(CONNSYS_EMI_FW, base, size);
}

void consys_plt_coredump_get_mcif_emi_phy_addr_mt6989(phys_addr_t *base, unsigned int *size)
{
	conninfra_get_emi_phy_addr(CONNSYS_EMI_MCIF, base, size);
}

unsigned int consys_plt_coredump_setup_dynamic_remap_mt6989(int conn_type, unsigned int idx,
	unsigned int base, unsigned int length)
{
#define DYNAMIC_MAP_MAX_SIZE	0x300000
	unsigned int map_len = (length > DYNAMIC_MAP_MAX_SIZE ? DYNAMIC_MAP_MAX_SIZE : length);
	void __iomem *vir_addr = 0;

	if (conn_type != CONN_DEBUG_TYPE_GPS) {
		pr_notice("[%s] incorrect type: %d\n", __func__, conn_type);
		return 0;
	}

	if (coredump_mng_is_host_view_cr(base, NULL)) {
		return length;
	}

	/* Expand to request size */
	vir_addr = ioremap(g_coredump_config[conn_type].seg1_cr, 4);
	if (vir_addr) {
		iowrite32(g_coredump_config[conn_type].seg1_phy_addr + map_len, vir_addr);
		iounmap(vir_addr);
	} else {
		return 0;
	}
	/* Setup map base */
	vir_addr = ioremap(g_coredump_config[conn_type].seg1_start_addr, 4);
	if (vir_addr) {
		iowrite32(base, vir_addr);
		iounmap(vir_addr);
	} else {
		return 0;
	}
	return map_len;
}

void __iomem *consys_plt_coredump_remap_mt6989(int conn_type, unsigned int base,
	unsigned int length)
{
	void __iomem *vir_addr = 0;
	unsigned int host_cr;

	if (conn_type != CONN_DEBUG_TYPE_GPS) {
		pr_notice("[%s] incorrect type: %d\n", __func__, conn_type);
		return NULL;
	}

	if (coredump_mng_is_host_view_cr(base, &host_cr)) {
		vir_addr = ioremap(host_cr, length);
	} else {
		vir_addr = ioremap(g_coredump_config[conn_type].seg1_phy_addr, length);
	}
	return vir_addr;
}

void consys_plt_coredump_unmap_mt6989(void __iomem *vir_addr)
{
	if (vir_addr)
		iounmap(vir_addr);
}

char *consys_plt_coredump_get_tag_name_mt6989(int conn_type)
{
	if (conn_type != CONN_DEBUG_TYPE_GPS)
		return "TAG_ERROR";
	return g_coredump_config[conn_type].exception_tag_name;
}

bool consys_plt_coredump_is_supported_mt6989(unsigned int drv)
{
	bool ret = false;

	if (drv == CONN_DEBUG_TYPE_GPS)
		ret = true;

	return ret;
}

void consys_plt_coredump_get_emi_dump_offset_mt6989(unsigned int *start, unsigned int *end)
{
	if (start)
		*start = 0x003C0000;
	if (end)
		*end = 0x00610000;
}
