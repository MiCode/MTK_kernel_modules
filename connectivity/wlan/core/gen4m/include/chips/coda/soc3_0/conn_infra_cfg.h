/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __CONN_INFRA_CFG_REGS_H__
#define __CONN_INFRA_CFG_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************
*
*                     CONN_INFRA_CFG CR Definitions
*
*****************************************************************************
*/
#define CONN_INFRA_CFG_BASE                                    0x7C001000

#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0198)
#define CONN_INFRA_CFG_AP2WF_REMAP_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0120)

#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_ADDR \
	CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR
#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_MASK \
	0xFFFF0000
#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_SHFT \
	16

#define CONN_INFRA_CFG_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR \
	CONN_INFRA_CFG_AP2WF_REMAP_1_ADDR
#define CONN_INFRA_CFG_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK \
	0xFFFFFFFF
#define CONN_INFRA_CFG_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT \
	0

#ifdef __cplusplus
}
#endif

#endif /* __CONN_INFRA_CFG_REGS_H__ */
