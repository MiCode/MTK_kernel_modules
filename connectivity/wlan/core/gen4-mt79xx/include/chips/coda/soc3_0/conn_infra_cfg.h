/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
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

#if defined(_HIF_AXI)
#define CONN_INFRA_CFG_BASE                                    0x18001000
#define CONN_INFRA_CFG_AP2WF_BUS_ADDR                          0x18500000
#endif

#if defined(_HIF_PCIE)
#define CONN_INFRA_CFG_BASE                                    0x7C001000
#define CONN_INFRA_CFG_AP2WF_BUS_ADDR                          0x7C500000
#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR_DE_HARDCODE        0x18501844
#endif

#define CONN_INFRA_CFG_CONN_HW_VER_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0000) /* 1000 */
#define CONN_INFRA_CFG_CONN_CFG_ID_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0004) /* 1004 */
#define CONN_INFRA_CFG_CONN_FPGA_DUMMY0_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0010) /* 1010 */
#define CONN_INFRA_CFG_CONN_FPGA_DUMMY1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0014) /* 1014 */
#define CONN_INFRA_CFG_EASY_SECURITY_WF_IRQ_CLR_ADDR \
	(CONN_INFRA_CFG_BASE + 0x00E0) /* 10E0 */
#define CONN_INFRA_CFG_EASY_SECURITY_BGF_IRQ_CLR_ADDR \
	(CONN_INFRA_CFG_BASE + 0x00E4) /* 10E4 */
#define CONN_INFRA_CFG_LIGHT_SECURITY_CTRL_ADDR \
	(CONN_INFRA_CFG_BASE + 0x00F0) /* 10F0 */
#define CONN_INFRA_CFG_BUS_DEAD_CR_ADDRESS_ADDR \
	(CONN_INFRA_CFG_BASE + 0x00FC) /* 10FC */
#define CONN_INFRA_CFG_AP2BGF_REMAP_0_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0100) /* 1100 */
#define CONN_INFRA_CFG_AP2BGF_REMAP_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0104) /* 1104 */
#define CONN_INFRA_CFG_AP2BGF_REMAP_2_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0108) /* 1108 */
#define CONN_INFRA_CFG_AP2BGF_REMAP_SEG_0_ADDR \
	(CONN_INFRA_CFG_BASE + 0x010C) /* 110C */
#define CONN_INFRA_CFG_AP2BGF_REMAP_SEG_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0110) /* 1110 */
#define CONN_INFRA_CFG_SUBSYS2AP_REMAP_0_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0114) /* 1114 */
#define CONN_INFRA_CFG_AP2BGF_REMAP_3_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0118) /* 1118 */
#define CONN_INFRA_CFG_AP2WF_REMAP_0_ADDR \
	(CONN_INFRA_CFG_BASE + 0x011C) /* 111C */

/**********************************************
* Only define referenced CRs
***********************************************
*/

#define CONN_INFRA_CFG_AP2WF_REMAP_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0120) /* 1120 */

#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0198) /* 1198 */

#ifdef __cplusplus
}
#endif

#endif /* __CONN_INFRA_CFG_REGS_H__ */
