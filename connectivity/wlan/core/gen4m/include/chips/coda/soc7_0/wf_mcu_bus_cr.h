/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __WF_MCU_BUS_CR_REGS_H__
#define __WF_MCU_BUS_CR_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
*
*                     WF_MCU_BUS_CR CR Definitions
*
*****************************************************************************
*/

#define WF_MCU_BUS_CR_BASE                                     0x830C0000

#define WF_MCU_BUS_CR_AP2WF_REMAP_1_ADDR \
	(WF_MCU_BUS_CR_BASE + 0x0120)

#define WF_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_ADDR \
	WF_MCU_BUS_CR_AP2WF_REMAP_1_ADDR
#define WF_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_MASK \
	0xFFFFFFFF
#define WF_MCU_BUS_CR_AP2WF_REMAP_1_R_AP2WF_PUBLIC_REMAPPING_0_START_ADDRESS_SHFT \
	0

#ifdef __cplusplus
}
#endif

#endif
