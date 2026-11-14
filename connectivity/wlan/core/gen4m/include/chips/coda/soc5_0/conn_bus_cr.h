/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __CONN_BUS_CR_REGS_H__
#define __CONN_BUS_CR_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
*
*                     CONN_BUS_CR CR Definitions
*
*****************************************************************************
*/

#define CONN_BUS_CR_BASE                                       (0x1800E000 + CONN_INFRA_REMAPPING_OFFSET)

#define CONN_BUS_CR_PCIE2AP_REMAP_2_ADDR \
	(CONN_BUS_CR_BASE + 0x24C)

#define CONN_BUS_CR_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_ADDR \
	CONN_BUS_CR_PCIE2AP_REMAP_2_ADDR
#define CONN_BUS_CR_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_MASK \
	0xFFFF0000
#define CONN_BUS_CR_PCIE2AP_REMAP_2_R_PCIE2AP_PUBLIC_REMAPPING_5_SHFT \
	16

#ifdef __cplusplus
}
#endif

#endif
