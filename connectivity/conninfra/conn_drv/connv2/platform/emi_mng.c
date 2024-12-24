// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/types.h>
#include <linux/version.h>
#include "emi_mng.h"
#include "osal.h"

#if IS_ENABLED(CONFIG_MTK_EMI)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <soc/mediatek/emi.h>
#else
#include <memory/mediatek/emi.h>
#endif
#endif

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
#define ALLOCATE_CONNSYS_EMI_FROM_DTS 1
#endif

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


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

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
phys_addr_t g_con_emi_phy_base;
unsigned long long g_con_emi_size;

const struct consys_platform_emi_ops* consys_platform_emi_ops = NULL;

struct consys_emi_addr_info connsys_emi_addr_info = {
	.emi_ap_phy_addr = 0,
	.emi_size = 0,
	.md_emi_phy_addr = 0,
	.md_emi_size = 0,
	.gps_emi_phy_addr = 0,
	.gps_emi_size = 0,
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
int emi_mng_set_region_protection(void)
{
	if (consys_platform_emi_ops &&
		consys_platform_emi_ops->consys_ic_emi_mpu_set_region_protection)
		return consys_platform_emi_ops->consys_ic_emi_mpu_set_region_protection();
	return -1;
}

int emi_mng_set_remapping_reg(void)
{
	if (consys_platform_emi_ops &&
		consys_platform_emi_ops->consys_ic_emi_set_remapping_reg)
		return consys_platform_emi_ops->consys_ic_emi_set_remapping_reg(
			connsys_emi_addr_info.emi_ap_phy_addr,
			connsys_emi_addr_info.md_emi_phy_addr,
			connsys_emi_addr_info.gps_emi_phy_addr);
	return -1;
}

struct consys_emi_addr_info* emi_mng_get_phy_addr(void)
{
	return &connsys_emi_addr_info;
}

static void emi_mng_get_gps_emi(struct platform_device *pdev)
{
	struct device_node *node;
	u64 phy_addr = 0;
	unsigned int phy_size = 0;

	node = of_find_node_by_path("/gps@18c00000");
	if (!node) {
		pr_notice("%s failed to find gps node\n", __func__);
		return;
	}

	if (of_property_read_u64(node, "emi-addr", &phy_addr)) {
		pr_info("%s: unable to get emi_addr\n", __func__);
		return;
	}

	if (of_property_read_u32(node, "emi-size", &phy_size)) {
		pr_info("%s: unable to get emi_size\n", __func__);
		return;
	}

	connsys_emi_addr_info.gps_emi_phy_addr = phy_addr;
	connsys_emi_addr_info.gps_emi_size = phy_size;
	pr_info("%s emi_addr %x, emi_size %x\n", __func__, phy_addr, phy_size);
}

#ifdef ALLOCATE_CONNSYS_EMI_FROM_DTS
static int emi_mng_allocate_connsys_emi(struct platform_device *pdev)
{
	struct device_node *np;
	struct reserved_mem *rmem;

	np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!np) {
		pr_info("no memory-region, np is NULL\n");
		return -1;
	}

	rmem = of_reserved_mem_lookup(np);
	of_node_put(np);

	if (!rmem) {
		pr_info("no memory-region\n");
		return -1;
	}

	g_con_emi_phy_base = rmem->base;
	g_con_emi_size = rmem->size;

	return 0;
}

static int emi_mng_get_emi_allocated_by_lk2(struct platform_device *pdev)
{
	struct device_node *node;
	u64 phy_addr = 0;
	unsigned int phy_size = 0;

	node = pdev->dev.of_node;
	if (!node) {
		pr_info("%s: unable to get consys node\n", __func__);
		return -1;
	}

	if (of_property_read_u64(node, "emi-addr", &phy_addr)) {
		pr_info("%s: unable to get emi_addr\n", __func__);
		return -1;
	}

	if (of_property_read_u32(node, "emi-size", &phy_size)) {
		pr_info("%s: unable to get emi_size\n", __func__);
		return -1;
	}

	pr_info("%s emi_addr %x, emi_size %x\n", __func__, phy_addr, phy_size);
	g_con_emi_phy_base = phy_addr;
	g_con_emi_size = phy_size;

	return 0;
}
#endif

int emi_mng_init(struct platform_device *pdev, const struct conninfra_plat_data* plat_data)
{
	/* EMI should be allocated either from ko or lk2 */
#ifdef ALLOCATE_CONNSYS_EMI_FROM_DTS
	if (emi_mng_allocate_connsys_emi(pdev) < 0)
		emi_mng_get_emi_allocated_by_lk2(pdev);
#endif
	if (consys_platform_emi_ops == NULL)
		consys_platform_emi_ops = (const struct consys_platform_emi_ops*)plat_data->platform_emi_ops;

	pr_info("[emi_mng_init] g_con_emi_phy_base = [0x%llx] size = [%llx] ops=[%p]",
			g_con_emi_phy_base, g_con_emi_size, consys_platform_emi_ops);

	if (g_con_emi_phy_base) {
		connsys_emi_addr_info.emi_ap_phy_addr = g_con_emi_phy_base;
		connsys_emi_addr_info.emi_size = g_con_emi_size;
	} else {
		pr_err("consys emi memory address g_con_emi_phy_base invalid\n");
	}

	if (consys_platform_emi_ops &&
		consys_platform_emi_ops->consys_ic_emi_get_md_shared_emi)
		consys_platform_emi_ops->consys_ic_emi_get_md_shared_emi(
			&connsys_emi_addr_info.md_emi_phy_addr,
			&connsys_emi_addr_info.md_emi_size);

	if (consys_platform_emi_ops &&
		consys_platform_emi_ops->consys_ic_emi_mpu_set_region_protection)
		consys_platform_emi_ops->consys_ic_emi_mpu_set_region_protection();

	emi_mng_get_gps_emi(pdev);

	return 0;
}

int emi_mng_deinit(void)
{
	consys_platform_emi_ops = NULL;
	return 0;
}

