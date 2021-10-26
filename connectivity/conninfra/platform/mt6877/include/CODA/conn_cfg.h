/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_cfg.h
//[Revision time]   : Mon Nov  9 19:12:08 2020

#ifndef __CONN_CFG_REGS_H__
#define __CONN_CFG_REGS_H__


//****************************************************************************
//
//                     CONN_CFG CR Definitions
//
//****************************************************************************

#define CONN_CFG_BASE                                          (CONN_REG_CONN_INFRA_CFG_ADDR) //0x1800_1000

#define CONN_CFG_IP_VERSION_ADDR                               (CONN_CFG_BASE + 0x000) // 1000
#define CONN_CFG_EFUSE_ADDR                                    (CONN_CFG_BASE + 0x020) // 1020
#define CONN_CFG_ADIE_CTL_ADDR                                 (CONN_CFG_BASE + 0x030) // 1030
#define CONN_CFG_CONN_INFRA_CFG_PWRCTRL0_ADDR                  (CONN_CFG_BASE + 0x200) // 1200
#define CONN_CFG_OSC_CTL_0_ADDR                                (CONN_CFG_BASE + 0x300) // 1300
#define CONN_CFG_OSC_CTL_1_ADDR                                (CONN_CFG_BASE + 0x304) // 1304
#define CONN_CFG_PLL_STATUS_ADDR                               (CONN_CFG_BASE + 0x320) // 1320
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR                  (CONN_CFG_BASE + 0x380) // 1380
#define CONN_CFG_CONN_INFRA_CFG_RC_STATUS_ADDR                 (CONN_CFG_BASE + 0x384) // 1384
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_ADDR                  (CONN_CFG_BASE + 0x388) // 1388
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR              (CONN_CFG_BASE + 0x390) // 1390
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR              (CONN_CFG_BASE + 0x394) // 1394
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR               (CONN_CFG_BASE + 0x3A0) // 13A0
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR               (CONN_CFG_BASE + 0x3A4) // 13A4
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR               (CONN_CFG_BASE + 0x3B0) // 13B0
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR               (CONN_CFG_BASE + 0x3B4) // 13B4
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR              (CONN_CFG_BASE + 0x3C0) // 13C0
#define CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR              (CONN_CFG_BASE + 0x3C4) // 13C4
#define CONN_CFG_EMI_CTL_0_ADDR                                (CONN_CFG_BASE + 0x400) // 1400
#define CONN_CFG_EMI_CTL_1_ADDR                                (CONN_CFG_BASE + 0x404) // 1404
#define CONN_CFG_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR            (CONN_CFG_BASE + 0x504) // 1504
#define CONN_CFG_CONN_INFRA_AP2CONN_SLP_STATUS_ADDR            (CONN_CFG_BASE + 0x514) // 1514
#define CONN_CFG_CONN_INFRA_ON_BUS_SLP_STATUS_ADDR             (CONN_CFG_BASE + 0x524) // 1524
#define CONN_CFG_CONN_INFRA_OFF_BUS_SLP_STATUS_ADDR            (CONN_CFG_BASE + 0x534) // 1534
#define CONN_CFG_CONN_INFRA_WF_SLP_STATUS_ADDR                 (CONN_CFG_BASE + 0x544) // 1544
#define CONN_CFG_GALS_CONN2BT_SLP_STATUS_ADDR                  (CONN_CFG_BASE + 0x554) // 1554
#define CONN_CFG_GALS_BT2CONN_SLP_STATUS_ADDR                  (CONN_CFG_BASE + 0x564) // 1564
#define CONN_CFG_GALS_CONN2GPS_SLP_STATUS_ADDR                 (CONN_CFG_BASE + 0x574) // 1574
#define CONN_CFG_GALS_GPS2CONN_SLP_STATUS_ADDR                 (CONN_CFG_BASE + 0x584) // 1584


/* =====================================================================================
  ---EMI_CTL_0 (0x18001000 + 0x400)---
 =====================================================================================*/
#define CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_ADDR            CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK            0x00600000                // EMI_CTL_DEBUG_1_SEL[22..21]
#define CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT            21
#define CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT_ADDR                  CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT_MASK                  0x00007FF0                // DDR_CNT_LIMIT[14..4]
#define CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT_SHFT                  4


#endif // __CONN_CFG_REGS_H__
