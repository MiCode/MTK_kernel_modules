// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/printk.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "consys_reg_util.h"
#include "connsys_debug_utility.h"
#include "connsys_coredump_hw_config.h"
#include "coredump_mng.h"
#include "conninfra.h"

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
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

static struct coredump_hw_config g_coredump_config[CONN_DEBUG_TYPE_END] = {
    /* Wi-Fi config */
    {
        .name = "WFSYS",
#ifdef CONFIG_FPGA_EARLY_PORTING
	.start_offset = 0x8000,
#else
        .start_offset = 0x0495400,
#endif
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
        .seg1_cr = 0x18816024,
        .seg1_value_end = 0x18bfffff,
        .seg1_start_addr = 0x18816014,
        .seg1_phy_addr = 0x18900000,
        .task_table_size = sizeof(bt_task_str)/sizeof(char*),
        .task_map_table = bt_task_str,
        .exception_tag_name = "combo_bt",
    },
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static struct coredump_hw_config* consys_plt_coredump_get_platform_config(int conn_type);
static unsigned int consys_plt_coredump_get_platform_chipid(void);
static char* consys_plt_coredump_get_task_string(int conn_type, unsigned int task_id);
static char* consys_plt_coredump_get_sys_name(int conn_type);
static bool consys_plt_coredump_is_host_view_cr(unsigned int addr, unsigned int* host_view);
static bool consys_plt_coredump_is_host_csr_readable(void);
static enum cr_category consys_plt_coredump_get_cr_category(unsigned int addr);

static unsigned int consys_plt_coredump_get_emi_offset(int conn_type);
static void consys_plt_coredump_get_emi_phy_addr(phys_addr_t* base, unsigned int *size);
static void consys_plt_coredump_get_mcif_emi_phy_addr(phys_addr_t* base, unsigned int *size);
static unsigned int consys_plt_coredump_setup_dynamic_remap(int conn_type, unsigned int idx, unsigned int base, unsigned int length);
static void __iomem* consys_plt_coredump_remap(int conn_type, unsigned int base, unsigned int length);
static void consys_plt_coredump_unmap(void __iomem* vir_addr);
static char* consys_plt_coredump_get_tag_name(int conn_type);

struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6895 = {
    .consys_coredump_get_platform_config = consys_plt_coredump_get_platform_config,
    .consys_coredump_get_platform_chipid = consys_plt_coredump_get_platform_chipid,
    .consys_coredump_get_task_string = consys_plt_coredump_get_task_string,
    .consys_coredump_get_sys_name = consys_plt_coredump_get_sys_name,
    .consys_coredump_is_host_view_cr = consys_plt_coredump_is_host_view_cr,
    .consys_coredump_is_host_csr_readable = consys_plt_coredump_is_host_csr_readable,
    .consys_coredump_get_cr_category = consys_plt_coredump_get_cr_category,
    .consys_coredump_get_emi_offset = consys_plt_coredump_get_emi_offset,
    .consys_coredump_get_emi_phy_addr = consys_plt_coredump_get_emi_phy_addr,
    .consys_coredump_get_mcif_emi_phy_addr = consys_plt_coredump_get_mcif_emi_phy_addr,
    .consys_coredump_setup_dynamic_remap = consys_plt_coredump_setup_dynamic_remap,
    .consys_coredump_remap = consys_plt_coredump_remap,
    .consys_coredump_unmap = consys_plt_coredump_unmap,
    .consys_coredump_get_tag_name = consys_plt_coredump_get_tag_name,
};

static struct coredump_hw_config* consys_plt_coredump_get_platform_config(int conn_type)
{
    if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
        pr_notice("Incorrect type: %d\n", conn_type);
        return NULL;
    }
    return &g_coredump_config[conn_type];
}

static unsigned int consys_plt_coredump_get_platform_chipid(void)
{
    return 0x6895;
}

static char* consys_plt_coredump_get_task_string(int conn_type, unsigned int task_id)
{
    if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
        pr_notice("Incorrect type: %d\n", conn_type);
        return NULL;
    }

    if (task_id > g_coredump_config[conn_type].task_table_size) {
        pr_notice("[%s] Incorrect task: %d\n",
            g_coredump_config[conn_type].name, task_id);
        return NULL;
    }

    return g_coredump_config[conn_type].task_map_table[task_id];
}

static char* consys_plt_coredump_get_sys_name(int conn_type)
{
    if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
        pr_notice("Incorrect type: %d\n", conn_type);
        return NULL;
    }
    return g_coredump_config[conn_type].name;
}

static bool consys_plt_coredump_is_host_view_cr(unsigned int addr, unsigned int* host_view)
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

static bool consys_plt_coredump_is_host_csr_readable(void)
{
    void __iomem *vir_addr = NULL;
    bool ret = false;
    unsigned int r;

    /* AP2CONN_INFRA ON
     * 1. Check ap2conn gals sleep protect status
     *  - 0x1000_1C5C [12] tx
     *  - 0x1000_1C9C [0]  rx
     *  (sleep protect enable ready)
     *  both of them should be 1'b0  (CR at ap side)
     */
    vir_addr = ioremap(0x10001c5c, 0x44);
    if (vir_addr) {
        r = CONSYS_REG_READ_BIT(vir_addr, 0x1 << 12) |
		CONSYS_REG_READ_BIT(vir_addr + 0x40, 0x1 << 0);
        if (r == 0) {
            ret = true;
        }
        iounmap(vir_addr);
    } else
        pr_info("[%s] remap 0x10001c5c fail", __func__);

    return ret;
}

static enum cr_category consys_plt_coredump_get_cr_category(unsigned int addr)
{
    if (addr >= 0x7c000000 && addr <= 0x7c3fffff) {
        if (addr >= 0x7c060000 && addr <= 0x7c06ffff) {
            return CONN_HOST_CSR;
        }
        return CONN_INFRA_CR;
    }

    return SUBSYS_CR;
}

static unsigned int consys_plt_coredump_get_emi_offset(int conn_type)
{
	struct coredump_hw_config *config = coredump_mng_get_platform_config(conn_type);

	if (config)
		return config->start_offset;
	return 0;

}

static void consys_plt_coredump_get_emi_phy_addr(phys_addr_t* base, unsigned int *size)
{
	conninfra_get_emi_phy_addr(CONNSYS_EMI_FW, base, size);
}

static void consys_plt_coredump_get_mcif_emi_phy_addr(phys_addr_t* base, unsigned int *size)
{
	conninfra_get_emi_phy_addr(CONNSYS_EMI_MCIF, base, size);
}

static unsigned int consys_plt_coredump_setup_dynamic_remap(int conn_type, unsigned int idx, unsigned int base, unsigned int length)
{
#define DYNAMIC_MAP_MAX_SIZE	0x300000
	unsigned int map_len = (length > DYNAMIC_MAP_MAX_SIZE? DYNAMIC_MAP_MAX_SIZE : length);
	void __iomem* vir_addr = 0;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("[%s] conn_type %d is invalid\n", __func__, conn_type);
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

static void __iomem* consys_plt_coredump_remap(int conn_type, unsigned int base, unsigned int length)
{
	void __iomem* vir_addr = 0;
	unsigned int host_cr;

	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("[%s] conn_type %d is invalid\n", __func__, conn_type);
		return NULL;
	}

	if (coredump_mng_is_host_view_cr(base, &host_cr)) {
		vir_addr = ioremap(host_cr, length);
	} else {
		vir_addr = ioremap(g_coredump_config[conn_type].seg1_phy_addr, length);
	}
	return vir_addr;
}

static void consys_plt_coredump_unmap(void __iomem* vir_addr)
{
	iounmap(vir_addr);
}

static char* consys_plt_coredump_get_tag_name(int conn_type)
{
	if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
		pr_notice("%s conn_type %d is invalid\n", __func__, conn_type);
		return NULL;
	}
	return g_coredump_config[conn_type].exception_tag_name;
}

