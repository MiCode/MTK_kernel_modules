// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "consys_reg_util.h"
#include "connsys_debug_utility.h"
#include "connsys_coredump_hw_config.h"
#include "coredump_mng.h"

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
        .start_offset = 0x0004f000,
#else
        .start_offset = 0x027f000,
#endif
        .size = 0x18000,
        .seg1_cr = 0x1800112c,
        .seg1_value_end = 0x187fffff,
        .seg1_start_addr = 0x18001120,
        .seg1_phy_addr = 0x18500000,
        .task_table_size = sizeof(wifi_task_str)/sizeof(char*),
        .task_map_table = wifi_task_str,
        .exception_tag_name = "combo_wifi",
    },
    /* BT config */
    {
        .name = "BTSYS",
        .start_offset = 0x33000,
        .size = 0x18000,
        .seg1_cr = 0x18001110,
        .seg1_value_end = 0x18bfffff,
        .seg1_start_addr = 0x18001104,
        .seg1_phy_addr = 0x18900000,
        .task_table_size = sizeof(bt_task_str)/sizeof(char*),
        .task_map_table = bt_task_str,
        .exception_tag_name = "combo_bt",
    },
};

static struct coredump_hw_config* consys_plt_coredump_get_platform_config(int conn_type);
static unsigned int consys_plt_coredump_get_platform_chipid(void);
static char* consys_plt_coredump_get_task_string(int conn_type, unsigned int task_id);
static char* consys_plt_coredump_get_sys_name(int conn_type);
static bool consys_plt_coredump_is_host_view_cr(unsigned int addr, unsigned int* host_view);
static bool consys_plt_coredump_is_host_csr_readable(void);
static enum cr_category consys_plt_coredump_get_cr_category(unsigned int addr);

struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6885 = {
    .consys_coredump_get_platform_config = consys_plt_coredump_get_platform_config,
    .consys_coredump_get_platform_chipid = consys_plt_coredump_get_platform_chipid,
    .consys_coredump_get_task_string = consys_plt_coredump_get_task_string,
    .consys_coredump_get_sys_name = consys_plt_coredump_get_sys_name,
    .consys_coredump_is_host_view_cr = consys_plt_coredump_is_host_view_cr,
    .consys_coredump_is_host_csr_readable = consys_plt_coredump_is_host_csr_readable,
    .consys_coredump_get_cr_category = consys_plt_coredump_get_cr_category,
};

static struct coredump_hw_config* consys_plt_coredump_get_platform_config(int conn_type)
{
    if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
        pr_err("Incorrect type: %d\n", conn_type);
        return NULL;
    }
    return &g_coredump_config[conn_type];
}

static unsigned int consys_plt_coredump_get_platform_chipid(void)
{
    return 0x6885;
}

static char* consys_plt_coredump_get_task_string(int conn_type, unsigned int task_id)
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

static char* consys_plt_coredump_get_sys_name(int conn_type)
{
    if (conn_type < 0 || conn_type >= CONN_DEBUG_TYPE_END) {
        pr_err("Incorrect type: %d\n", conn_type);
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
    void __iomem *vir_addr_tx = NULL;
    void __iomem *vir_addr_rx = NULL;
    bool ret = false;
    unsigned int rx, tx;

    /* AP2CONN_INFRA ON
     * 1. Check ap2conn gals sleep protect status
     *  - 0x1000_1724 [2] / 0x1000_1228 [13] (infracfg_ao)(rx/tx)
     *  (sleep protect enable ready)
     *  both of them should be 1'b0  (CR at ap side)
     */
    vir_addr_rx = ioremap(0x10001724, 0x4);
    vir_addr_tx = ioremap(0x10001228, 0x4);
    if (vir_addr_tx && vir_addr_rx) {
        rx = CONSYS_REG_READ_BIT(vir_addr_rx, (0x1 << 2));
        tx = CONSYS_REG_READ_BIT(vir_addr_tx, (0x1 << 13));
        if ((rx == 0) && (tx == 0)) {
            ret = true;
        }
    } else
        pr_info("[%s] remap fail, [%p][%p]", __func__, vir_addr_rx, vir_addr_tx);

    if (vir_addr_rx)
        iounmap(vir_addr_rx);
    if (vir_addr_tx)
        iounmap(vir_addr_tx);
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
