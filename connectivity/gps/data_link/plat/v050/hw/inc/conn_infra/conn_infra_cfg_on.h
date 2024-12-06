/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __CONN_INFRA_CFG_ON_REGS_H__
#define __CONN_INFRA_CFG_ON_REGS_H__

#define CONN_INFRA_CFG_ON_BASE                                 0x18001000

#define CONN_INFRA_CFG_ON_ADIE_CTL_ADDR                        (CONN_INFRA_CFG_ON_BASE + 0x0010)
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_ADDR     (CONN_INFRA_CFG_ON_BASE + 0x020C)
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_ADDR         (CONN_INFRA_CFG_ON_BASE + 0x0210)
#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR    (CONN_INFRA_CFG_ON_BASE + 0x0470)
#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR        (CONN_INFRA_CFG_ON_BASE + 0x0474)
#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR    (CONN_INFRA_CFG_ON_BASE + 0x0480)
#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR        (CONN_INFRA_CFG_ON_BASE + 0x0484)


#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_GPS_FUNCTION_EN_ADDR \
	CONN_INFRA_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_ADDR
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_GPS_FUNCTION_EN_MASK 0x00000001
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_GPS_FUNCTION_EN_SHFT 0

#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY_ADDR \
	CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_ADDR
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY_MASK 0x00010000
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY_SHFT 16

#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_ADDR \
	CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010
#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_ADDR \
	CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001
#define CONN_INFRA_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_SHFT 0

#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_ADDR \
	CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000
#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_ADDR \
	CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000
#define CONN_INFRA_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_SHFT 22

#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR \
	CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010
#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR \
	CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001
#define CONN_INFRA_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 0

#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_RDY_ADDR \
	CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000
#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_RDY_ADDR \
	CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000
#define CONN_INFRA_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_RDY_SHFT 22

#endif /* __CONN_INFRA_CFG_ON_REGS_H__ */

