/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef EMI_MNG_H
#define EMI_MNG_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "consys_hw.h"
#include "osal.h"


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

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

struct consys_emi_addr_info {
	/* This include BT/WF FW and WFDMA */
	phys_addr_t emi_ap_phy_addr;
	unsigned int emi_size;
	/* MCIF EMI get from MD */
	phys_addr_t md_emi_phy_addr;
	unsigned int md_emi_size;
	/* GPS EMI */
	phys_addr_t gps_emi_phy_addr;
	unsigned int gps_emi_size;
};

typedef int(*CONSYS_IC_EMI_MPU_SET_REGION_PROTECTION) (void);
typedef unsigned int(*CONSYS_IC_EMI_SET_REMAPPING_REG) (phys_addr_t, phys_addr_t, phys_addr_t);
typedef void(*CONSYS_IC_EMI_GET_MD_SHARED_EMI) (phys_addr_t* phy_addr, unsigned int *size);

struct consys_platform_emi_ops {
	CONSYS_IC_EMI_MPU_SET_REGION_PROTECTION consys_ic_emi_mpu_set_region_protection;
	CONSYS_IC_EMI_SET_REMAPPING_REG consys_ic_emi_set_remapping_reg;
	CONSYS_IC_EMI_GET_MD_SHARED_EMI consys_ic_emi_get_md_shared_emi;
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int emi_mng_init(struct platform_device *pdev, const struct conninfra_plat_data* plat_data);
int emi_mng_deinit(void);

int emi_mng_set_region_protection(void);
int emi_mng_set_remapping_reg(void);
struct consys_emi_addr_info* emi_mng_get_phy_addr(void);


/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif	/* EMI_MNG_H */

