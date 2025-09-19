/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __CONN_INFRA_BUS_CR_ON_REGS_H__
#define __CONN_INFRA_BUS_CR_ON_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
*
*                     CONN_INFRA_BUS_CR_ON CR Definitions
*
*****************************************************************************
*/

#define CONN_INFRA_BUS_CR_ON_BASE                              0x7C00E000

#define CONN_INFRA_BUS_CR_ON_PCIE2AP_REMAP_2_ADDR \
	(CONN_INFRA_BUS_CR_ON_BASE + 0x068)

#define CONN_INFRA_BUS_CR_ON_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_ADDR \
	CONN_INFRA_BUS_CR_ON_PCIE2AP_REMAP_2_ADDR
#define CONN_INFRA_BUS_CR_ON_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_MASK \
	0xFFFF0000
#define CONN_INFRA_BUS_CR_ON_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_SHFT \
	16

#ifdef __cplusplus
}
#endif

#endif
