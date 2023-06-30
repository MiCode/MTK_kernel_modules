// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include "emi_mng.h"
#include "mt6895.h"
#include "mt6895_pos.h"

/* For EMI MPU */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <soc/mediatek/emi.h>
#else
#include <memory/mediatek/emi.h>
#endif
/* For MCIF */
#include <mtk_ccci_common.h>
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define	REGION_CONN	27

#define	DOMAIN_AP	0
#define	DOMAIN_CONN	2
#define	DOMAIN_SCP	3

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
static int consys_emi_mpu_set_region_protection_mt6895(void);
static void consys_emi_get_md_shared_emi_mt6895(phys_addr_t* base, unsigned int* size);
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

extern unsigned long long gConEmiSize;
extern phys_addr_t gConEmiPhyBase;

struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6895 = {
	.consys_ic_emi_mpu_set_region_protection = consys_emi_mpu_set_region_protection_mt6895,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg_mt6895,
	.consys_ic_emi_get_md_shared_emi = consys_emi_get_md_shared_emi_mt6895,
};

static int consys_emi_mpu_set_region_protection_mt6895(void)
{
	pr_info("[%s] is not supported. MPU is set in lk\n", __func__);
	return 0;
}

void consys_emi_get_md_shared_emi_mt6895(phys_addr_t* base, unsigned int* size)
{
	phys_addr_t mdPhy = 0;
	int ret = 0;

#ifndef CONFIG_FPGA_EARLY_PORTING
#if IS_ENABLED(CONFIG_MTK_ECCCI_DRIVER)
	mdPhy = get_smem_phy_start_addr(MD_SYS1, SMEM_USER_RAW_MD_CONSYS, &ret);
#else
	pr_info("[%s] ECCCI Driver is not supported.\n", __func__);
#endif
#else
	pr_info("[%s] not implement on FPGA\n", __func__);
#endif
	if (ret && mdPhy) {
		pr_info("MCIF base=0x%llx size=0x%x", mdPhy, ret);
		if (base)
			*base = mdPhy;
		if (size)
			*size = ret;
	} else {
		pr_info("MCIF is not supported");
		if (base)
			*base = 0;
		if (size)
			*size = 0;
	}
}

