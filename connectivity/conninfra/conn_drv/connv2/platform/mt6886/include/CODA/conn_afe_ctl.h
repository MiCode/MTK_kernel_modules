/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_afe_ctl.h
//[Revision time]   : Fri Apr  9 11:48:25 2021

#ifndef __CONN_AFE_CTL_REGS_H__
#define __CONN_AFE_CTL_REGS_H__

//****************************************************************************
//
//                     CONN_AFE_CTL CR Definitions                     
//
//****************************************************************************

#define CONN_AFE_CTL_BASE                                      (CONN_REG_CONN_AFE_CTL_ADDR)

#define CONN_AFE_CTL_RG_DIG_EN_01_ADDR                         (CONN_AFE_CTL_BASE + 0x000) // 1000
#define CONN_AFE_CTL_RG_DIG_EN_02_ADDR                         (CONN_AFE_CTL_BASE + 0x004) // 1004
#define CONN_AFE_CTL_RG_DIG_EN_03_ADDR                         (CONN_AFE_CTL_BASE + 0x008) // 1008
#define CONN_AFE_CTL_RG_DIG_TOP_01_ADDR                        (CONN_AFE_CTL_BASE + 0x00C) // 100C
#define CONN_AFE_CTL_RG_WBG_AFE_01_ADDR                        (CONN_AFE_CTL_BASE + 0x010) // 1010
#define CONN_AFE_CTL_RG_WBG_AFE_02_ADDR                        (CONN_AFE_CTL_BASE + 0x014) // 1014
#define CONN_AFE_CTL_RG_WBG_RCK_01_ADDR                        (CONN_AFE_CTL_BASE + 0x018) // 1018
#define CONN_AFE_CTL_RG_WBG_RCK_02_ADDR                        (CONN_AFE_CTL_BASE + 0x01C) // 101C
#define CONN_AFE_CTL_RGS_WBG_RO_ADDR                           (CONN_AFE_CTL_BASE + 0x020) // 1020
#define CONN_AFE_CTL_RG_WBG_PLL_01_ADDR                        (CONN_AFE_CTL_BASE + 0x024) // 1024
#define CONN_AFE_CTL_RG_WBG_PLL_02_ADDR                        (CONN_AFE_CTL_BASE + 0x028) // 1028
#define CONN_AFE_CTL_RG_WBG_PLL_03_ADDR                        (CONN_AFE_CTL_BASE + 0x02C) // 102C
#define CONN_AFE_CTL_RG_WBG_PLL_04_ADDR                        (CONN_AFE_CTL_BASE + 0x030) // 1030
#define CONN_AFE_CTL_RG_WBG_PLL_05_ADDR                        (CONN_AFE_CTL_BASE + 0x034) // 1034
#define CONN_AFE_CTL_RG_WBG_PLL_06_ADDR                        (CONN_AFE_CTL_BASE + 0x038) // 1038
#define CONN_AFE_CTL_RG_WBG_PLL_07_ADDR                        (CONN_AFE_CTL_BASE + 0x03C) // 103C
#define CONN_AFE_CTL_RG_WBG_GL1_01_ADDR                        (CONN_AFE_CTL_BASE + 0x040) // 1040
#define CONN_AFE_CTL_RG_WBG_GL1_02_ADDR                        (CONN_AFE_CTL_BASE + 0x044) // 1044
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x048) // 1048
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x04C) // 104C
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x050) // 1050
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x054) // 1054
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x058) // 1058
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR                     (CONN_AFE_CTL_BASE + 0x05C) // 105C
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR                    (CONN_AFE_CTL_BASE + 0x060) // 1060
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x068) // 1068
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x06C) // 106C
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x070) // 1070
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x074) // 1074
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x078) // 1078
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR                     (CONN_AFE_CTL_BASE + 0x07C) // 107C
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR                    (CONN_AFE_CTL_BASE + 0x080) // 1080
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x084) // 1084
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x088) // 1088
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x08C) // 108C
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x090) // 1090
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x094) // 1094
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR                     (CONN_AFE_CTL_BASE + 0x098) // 1098
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR                    (CONN_AFE_CTL_BASE + 0x09C) // 109C
#define CONN_AFE_CTL_RG_WBG_WF2_RX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x0A0) // 10A0
#define CONN_AFE_CTL_RG_WBG_WF2_RX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x0A4) // 10A4
#define CONN_AFE_CTL_RG_WBG_WF2_TX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x0A8) // 10A8
#define CONN_AFE_CTL_RG_WBG_WF2_TX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x0AC) // 10AC
#define CONN_AFE_CTL_RG_WBG_WF2_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x0B0) // 10B0
#define CONN_AFE_CTL_RG_WBG_WF2_TX_04_ADDR                     (CONN_AFE_CTL_BASE + 0x0B4) // 10B4
#define CONN_AFE_CTL_RGS_WBG_WF2_TXCAL_ADDR                    (CONN_AFE_CTL_BASE + 0x0B8) // 10B8
#define CONN_AFE_CTL_RG_WBG_WF3_RX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x0BC) // 10BC
#define CONN_AFE_CTL_RG_WBG_WF3_RX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x0C0) // 10C0
#define CONN_AFE_CTL_RG_WBG_WF3_TX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x0C4) // 10C4
#define CONN_AFE_CTL_RG_WBG_WF3_TX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x0C8) // 10C8
#define CONN_AFE_CTL_RG_WBG_WF3_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x0CC) // 10CC
#define CONN_AFE_CTL_RG_WBG_WF3_TX_04_ADDR                     (CONN_AFE_CTL_BASE + 0x0D0) // 10D0
#define CONN_AFE_CTL_RGS_WBG_WF3_TXCAL_ADDR                    (CONN_AFE_CTL_BASE + 0x0D4) // 10D4
#define CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR                       (CONN_AFE_CTL_BASE + 0x0D8) // 10D8
#define CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR                       (CONN_AFE_CTL_BASE + 0x0DC) // 10DC
#define CONN_AFE_CTL_RG_SIN_GEN_01_ADDR                        (CONN_AFE_CTL_BASE + 0x0E0) // 10E0
#define CONN_AFE_CTL_RG_SIN_GEN_02_ADDR                        (CONN_AFE_CTL_BASE + 0x0E4) // 10E4
#define CONN_AFE_CTL_RG_SIN_GEN_03_ADDR                        (CONN_AFE_CTL_BASE + 0x0E8) // 10E8
#define CONN_AFE_CTL_RG_SIN_GEN_04_ADDR                        (CONN_AFE_CTL_BASE + 0x0EC) // 10EC
#define CONN_AFE_CTL_RG_SIN_GEN_05_ADDR                        (CONN_AFE_CTL_BASE + 0x0F0) // 10F0
#define CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR                      (CONN_AFE_CTL_BASE + 0x0F4) // 10F4
#define CONN_AFE_CTL_RG_WBG_DFS_RX_01_ADDR                     (CONN_AFE_CTL_BASE + 0x0F8) // 10F8
#define CONN_AFE_CTL_RG_WBG_DFS_RX_02_ADDR                     (CONN_AFE_CTL_BASE + 0x0FC) // 10FC
#define CONN_AFE_CTL_RG_WBG_GL5_01_ADDR                        (CONN_AFE_CTL_BASE + 0x100) // 1100
#define CONN_AFE_CTL_RG_WBG_GL5_02_ADDR                        (CONN_AFE_CTL_BASE + 0x104) // 1104
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR                       (CONN_AFE_CTL_BASE + 0x124) // 1124
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_ADDR                       (CONN_AFE_CTL_BASE + 0x128) // 1128
#define CONN_AFE_CTL_RG_PLL_RFSOC_M_ADDR                       (CONN_AFE_CTL_BASE + 0x12C) // 112C
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_ADDR                       (CONN_AFE_CTL_BASE + 0x130) // 1130
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB00_ADDR                    (CONN_AFE_CTL_BASE + 0x134) // 1134
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB10_ADDR                    (CONN_AFE_CTL_BASE + 0x13C) // 113C




/* =====================================================================================

  ---RG_DIG_EN_01 (0x18041000 + 0x000)---

    RG_WBG_EN_RCK[0]             - (RW) RC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF3_DA[1]          - (RW) Enable WF3 DA control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF3_ADPREON[2]     - (RW) Enable WF3ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF3_AD[3]          - (RW) Enable WF3 AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_WF3[4]         - (RW) Set WF3 AD/DA enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_WF2_DA[5]          - (RW) Enable WF2 DA control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF2_ADPREON[6]     - (RW) Enable WF2ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF2_AD[7]          - (RW) Enable WF2 AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_WF2[8]         - (RW) Set WF2 AD/DA enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_WF1_DA[9]          - (RW) Enable WF1 DA control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF1_ADPREON[10]    - (RW) Enable WF1ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF1_AD[11]         - (RW) Enable WF1 AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_WF1[12]        - (RW) Set WF1 AD/DA enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_WF0_DA[13]         - (RW) Enable WF0 DA control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF0_ADPREON[14]    - (RW) Enable WF0ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WF0_AD[15]         - (RW) Enable WF0 AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_WF0[16]        - (RW) Set WF0 AD/DA enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_TXCAL_WF3[17]      - (RW) WF3 DAC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_TXCAL_WF2[18]      - (RW) WF2 DAC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_TXCAL_WF1[19]      - (RW) WF1 DAC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_TXCAL_WF0[20]      - (RW) WF0 DAC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_TXCAL_BT0[21]      - (RW) BT0 DAC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_GL1_ADPREON[22]    - (RW) Enable GL1ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_GL1AD[23]          - (RW) Enable GL1AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_GL1[24]        - (RW) Set GL1 AD enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_BT0_DA[25]         - (RW) Enable BT0DA control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BT0_ADPREON[26]    - (RW) Enable BT0ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BT0_AD[27]         - (RW) Enable BT0AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_BT0[28]        - (RW) Set BT0 AD/DA enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_DFS_ADPREON[29]    - (RW) Enable DFS ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_DFS_AD[30]         - (RW) Enable DFS AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_DFS[31]        - (RW) Set DFS AD enable control to manual
                                     0: Auto
                                     1: Manual

 =====================================================================================*/
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_DFS_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_DFS_MASK       0x80000000                // RG_WBG_EN_MAN_DFS[31]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_DFS_SHFT       31
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_DFS_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_DFS_AD_MASK        0x40000000                // RG_WBG_EN_DFS_AD[30]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_DFS_AD_SHFT        30
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_DFS_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_DFS_ADPREON_MASK   0x20000000                // RG_WBG_EN_DFS_ADPREON[29]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_DFS_ADPREON_SHFT   29
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_BT0_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_BT0_MASK       0x10000000                // RG_WBG_EN_MAN_BT0[28]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_BT0_SHFT       28
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_AD_MASK        0x08000000                // RG_WBG_EN_BT0_AD[27]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_AD_SHFT        27
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_ADPREON_MASK   0x04000000                // RG_WBG_EN_BT0_ADPREON[26]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_ADPREON_SHFT   26
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_DA_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_DA_MASK        0x02000000                // RG_WBG_EN_BT0_DA[25]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_BT0_DA_SHFT        25
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_GL1_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_GL1_MASK       0x01000000                // RG_WBG_EN_MAN_GL1[24]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_GL1_SHFT       24
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_GL1AD_ADDR         CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_GL1AD_MASK         0x00800000                // RG_WBG_EN_GL1AD[23]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_GL1AD_SHFT         23
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_GL1_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_GL1_ADPREON_MASK   0x00400000                // RG_WBG_EN_GL1_ADPREON[22]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_GL1_ADPREON_SHFT   22
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_BT0_ADDR     CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_BT0_MASK     0x00200000                // RG_WBG_EN_TXCAL_BT0[21]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_BT0_SHFT     21
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF0_ADDR     CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF0_MASK     0x00100000                // RG_WBG_EN_TXCAL_WF0[20]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF0_SHFT     20
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF1_ADDR     CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF1_MASK     0x00080000                // RG_WBG_EN_TXCAL_WF1[19]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF1_SHFT     19
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF2_ADDR     CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF2_MASK     0x00040000                // RG_WBG_EN_TXCAL_WF2[18]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF2_SHFT     18
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF3_ADDR     CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF3_MASK     0x00020000                // RG_WBG_EN_TXCAL_WF3[17]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF3_SHFT     17
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF0_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF0_MASK       0x00010000                // RG_WBG_EN_MAN_WF0[16]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF0_SHFT       16
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_AD_MASK        0x00008000                // RG_WBG_EN_WF0_AD[15]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_AD_SHFT        15
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_ADPREON_MASK   0x00004000                // RG_WBG_EN_WF0_ADPREON[14]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_ADPREON_SHFT   14
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_DA_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_DA_MASK        0x00002000                // RG_WBG_EN_WF0_DA[13]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF0_DA_SHFT        13
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF1_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF1_MASK       0x00001000                // RG_WBG_EN_MAN_WF1[12]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF1_SHFT       12
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_AD_MASK        0x00000800                // RG_WBG_EN_WF1_AD[11]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_AD_SHFT        11
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_ADPREON_MASK   0x00000400                // RG_WBG_EN_WF1_ADPREON[10]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_ADPREON_SHFT   10
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_DA_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_DA_MASK        0x00000200                // RG_WBG_EN_WF1_DA[9]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF1_DA_SHFT        9
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF2_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF2_MASK       0x00000100                // RG_WBG_EN_MAN_WF2[8]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF2_SHFT       8
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_AD_MASK        0x00000080                // RG_WBG_EN_WF2_AD[7]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_AD_SHFT        7
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_ADPREON_MASK   0x00000040                // RG_WBG_EN_WF2_ADPREON[6]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_ADPREON_SHFT   6
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_DA_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_DA_MASK        0x00000020                // RG_WBG_EN_WF2_DA[5]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF2_DA_SHFT        5
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF3_ADDR       CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF3_MASK       0x00000010                // RG_WBG_EN_MAN_WF3[4]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_MAN_WF3_SHFT       4
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_AD_MASK        0x00000008                // RG_WBG_EN_WF3_AD[3]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_AD_SHFT        3
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_ADPREON_MASK   0x00000004                // RG_WBG_EN_WF3_ADPREON[2]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_ADPREON_SHFT   2
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_DA_ADDR        CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_DA_MASK        0x00000002                // RG_WBG_EN_WF3_DA[1]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_WF3_DA_SHFT        1
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_RCK_ADDR           CONN_AFE_CTL_RG_DIG_EN_01_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_RCK_MASK           0x00000001                // RG_WBG_EN_RCK[0]
#define CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_RCK_SHFT           0

/* =====================================================================================

  ---RG_DIG_EN_02 (0x18041000 + 0x004)---

    RG_WBG_EN_WF_PLL[1..0]       - (RW) PLL configuration for WF_SYS
                                     00: NA
                                     01: Enable BPLL only
                                     10: Enable WPLL only
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_MCU_PLL_BGF[3..2]  - (RW) PLL configuration for MCU_SYS
                                     00: NA
                                     01: Enable BPLL only 
                                     10: Enable WPLL only 
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_GPS_PLL[5..4]      - (RW) PLL configuration for GPS_SYS
                                     00: NA
                                     01: Enable BPLL only (POR)
                                     10: Enable WPLL only
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_BT_PLL[7..6]       - (RW) PLL configuration for BT_SYS
                                     00: NA
                                     01: Enable BPLL only 
                                     10: Enable WPLL only 
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_CASC_FM[8]         - (RW) Directly trigger BPLL->WPL (cascade) in auto mode (for FM)
                                     0: Disable
                                     1: Enable
    RG_WBG_RSV_ADCKSEL[10..9]    - (RW) RSV ADC clock selection in manual mode
    RG_WBG_EN_MAN_RSVADCK[11]    - (RW) Set RSV AD ck control to manual
    RG_WBG_GPS_ADCKSEL[13..12]   - (RW) GPS ADC clock selection in manual mode
                                     00: 16MHz
                                     01: 64MHz
                                     10: 128MHz
    RG_WBG_EN_MAN_GPSADCK[14]    - (RW) Set GPS AD ck control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_CASC_UP[15]        - (RW) Directly trigger BPLL, then WPLL in auto mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MCU_PLL_WF[17..16] - (RW) PLL configuration for MCU_SYS (WF)
                                     00: NA
                                     01: Enable BPLL only 
                                     10: Enable WPLL only 
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_RSV0_PLL[19..18]   - (RW) PLL configuration for RSV0
                                     00: NA
                                     01: Enable BPLL only 
                                     10: Enable WPLL only 
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_RSV1_PLL[21..20]   - (RW) PLL configuration for RSV1
                                     00: NA
                                     01: Enable BPLL only 
                                     10: Enable WPLL only 
                                     11: Enable BPLL, then enable WPLL
    RG_WBG_EN_BPLL_FM[22]        - (RW) Directly trigger BPLL in auto mode (for FM)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WPLL_FM[23]        - (RW) Directly trigger WPLL in auto mode (for FM)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_TXCAL_BT1[24]      - (RW) BT1 DAC calibration enable
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BT1_DA[25]         - (RW) Enable BT1DA control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BT1_ADPREON[26]    - (RW) Enable BT1ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BT1_AD[27]         - (RW) Enable BT1AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_BT1[28]        - (RW) Set BT1 AD/DA enable control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_GL5_ADPREON[29]    - (RW) Enable GL5ADPREON control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_GL5AD[30]          - (RW) Enable GL5AD control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_GL5[31]        - (RW) Set GL5 AD enable control to manual
                                     0: Auto
                                     1: Manual

 =====================================================================================*/
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_GL5_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_GL5_MASK       0x80000000                // RG_WBG_EN_MAN_GL5[31]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_GL5_SHFT       31
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GL5AD_ADDR         CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GL5AD_MASK         0x40000000                // RG_WBG_EN_GL5AD[30]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GL5AD_SHFT         30
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GL5_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GL5_ADPREON_MASK   0x20000000                // RG_WBG_EN_GL5_ADPREON[29]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GL5_ADPREON_SHFT   29
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_BT1_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_BT1_MASK       0x10000000                // RG_WBG_EN_MAN_BT1[28]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_BT1_SHFT       28
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_AD_ADDR        CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_AD_MASK        0x08000000                // RG_WBG_EN_BT1_AD[27]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_AD_SHFT        27
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_ADPREON_ADDR   CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_ADPREON_MASK   0x04000000                // RG_WBG_EN_BT1_ADPREON[26]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_ADPREON_SHFT   26
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_DA_ADDR        CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_DA_MASK        0x02000000                // RG_WBG_EN_BT1_DA[25]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT1_DA_SHFT        25
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_TXCAL_BT1_ADDR     CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_TXCAL_BT1_MASK     0x01000000                // RG_WBG_EN_TXCAL_BT1[24]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_TXCAL_BT1_SHFT     24
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WPLL_FM_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WPLL_FM_MASK       0x00800000                // RG_WBG_EN_WPLL_FM[23]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WPLL_FM_SHFT       23
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BPLL_FM_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BPLL_FM_MASK       0x00400000                // RG_WBG_EN_BPLL_FM[22]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BPLL_FM_SHFT       22
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_RSV1_PLL_ADDR      CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_RSV1_PLL_MASK      0x00300000                // RG_WBG_EN_RSV1_PLL[21..20]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_RSV1_PLL_SHFT      20
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_RSV0_PLL_ADDR      CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_RSV0_PLL_MASK      0x000C0000                // RG_WBG_EN_RSV0_PLL[19..18]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_RSV0_PLL_SHFT      18
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF_ADDR    CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF_MASK    0x00030000                // RG_WBG_EN_MCU_PLL_WF[17..16]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF_SHFT    16
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_CASC_UP_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_CASC_UP_MASK       0x00008000                // RG_WBG_EN_CASC_UP[15]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_CASC_UP_SHFT       15
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_GPSADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_GPSADCK_MASK   0x00004000                // RG_WBG_EN_MAN_GPSADCK[14]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_GPSADCK_SHFT   14
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_GPS_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_GPS_ADCKSEL_MASK      0x00003000                // RG_WBG_GPS_ADCKSEL[13..12]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_GPS_ADCKSEL_SHFT      12
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_RSVADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_RSVADCK_MASK   0x00000800                // RG_WBG_EN_MAN_RSVADCK[11]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MAN_RSVADCK_SHFT   11
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_RSV_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_RSV_ADCKSEL_MASK      0x00000600                // RG_WBG_RSV_ADCKSEL[10..9]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_RSV_ADCKSEL_SHFT      9
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_CASC_FM_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_CASC_FM_MASK       0x00000100                // RG_WBG_EN_CASC_FM[8]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_CASC_FM_SHFT       8
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL_ADDR        CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL_MASK        0x000000C0                // RG_WBG_EN_BT_PLL[7..6]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL_SHFT        6
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GPS_PLL_ADDR       CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GPS_PLL_MASK       0x00000030                // RG_WBG_EN_GPS_PLL[5..4]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_GPS_PLL_SHFT       4
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF_ADDR   CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF_MASK   0x0000000C                // RG_WBG_EN_MCU_PLL_BGF[3..2]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF_SHFT   2
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL_ADDR        CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL_MASK        0x00000003                // RG_WBG_EN_WF_PLL[1..0]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL_SHFT        0

/* =====================================================================================

  ---RG_DIG_EN_03 (0x18041000 + 0x008)---

    RG_WBG_WF3_ADCKSEL[2..0]     - (RW) WF3 ADC sample rate selection in manual mode
                                     001: 80MHz
                                     010: 160MHz
                                     100: 320MHz
                                     110: 640MHz
    RG_WBG_EN_MAN_WF3ADCK[3]     - (RW) Set WF3 AD sampe rate control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_WF2_ADCKSEL[6..4]     - (RW) WF2 ADC sample rate selection in manual mode
                                     001: 80MHz
                                     010: 160MHz
                                     100: 320MHz
                                     110: 640MHz
    RG_WBG_EN_MAN_WF2ADCK[7]     - (RW) Set WF2 AD sampe rate control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_WF1_ADCKSEL[10..8]    - (RW) WF1 ADC sample rate selection in manual mode
                                     001: 80MHz
                                     010: 160MHz
                                     100: 320MHz
                                     110: 640MHz
    RG_WBG_EN_MAN_WF1ADCK[11]    - (RW) Set WF1 AD sampe rate control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_WF0_ADCKSEL[14..12]   - (RW) WF0 ADC sample rate selection in manual mode
                                     001: 80MHz
                                     010: 160MHz
                                     100: 320MHz
                                     110: 640MHz
    RG_WBG_EN_MAN_WF0ADCK[15]    - (RW) Set WF0 AD ck control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_BT1_ADCKSEL[16]       - (RW) BT1 ADC clock selection in manual mode
                                     0: 32MHz
                                     1: 64MHz
    RG_WBG_EN_MAN_BT1ADCK[17]    - (RW) Set BT1 AD ck control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_BT0_ADCKSEL[18]       - (RW) BT0 ADC clock selection in manual mode
                                     0: 32MHz
                                     1: 64MHz
    RG_WBG_EN_MAN_BT0ADCK[19]    - (RW) Set BT0 AD ck control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_WPLL_UP[20]        - (RW) Directly trigger WPLL in auto mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BPLL_UP[21]        - (RW) Directly trigger BPLL in auto mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WPLL[22]           - (RW) Enable WPLL control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BPLL[23]           - (RW) Enable BPLL control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_PLL[24]        - (RW) Set PLL control to manual
                                     0: Auto
                                     1: Manual
    RG_WBG_EN_WFTX_CKO[25]       - (RW) Enable WFTX CKO in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WFRX_CKO[26]       - (RW) Enable WFRX CKO in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_MAN_WF_CKO[27]     - (RW) Enable WF AD/DA CKO control in manual mode
                                     0: Disable
                                     1: Enable
    RG_WBG_DFS_ADCKSEL[30..28]   - (RW) DFS ADC sample rate selection in manual mode
                                     001: 80MHz
                                     010: 160MHz
                                     100: 320MHz
                                     110: 640MHz
    RG_WBG_EN_MAN_DFSADCK[31]    - (RW) Set DFS AD ck control to manual
                                     0: Auto
                                     1: Manual

 =====================================================================================*/
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_DFSADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_DFSADCK_MASK   0x80000000                // RG_WBG_EN_MAN_DFSADCK[31]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_DFSADCK_SHFT   31
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_DFS_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_DFS_ADCKSEL_MASK      0x70000000                // RG_WBG_DFS_ADCKSEL[30..28]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_DFS_ADCKSEL_SHFT      28
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF_CKO_ADDR    CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF_CKO_MASK    0x08000000                // RG_WBG_EN_MAN_WF_CKO[27]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF_CKO_SHFT    27
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WFRX_CKO_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WFRX_CKO_MASK      0x04000000                // RG_WBG_EN_WFRX_CKO[26]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WFRX_CKO_SHFT      26
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WFTX_CKO_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WFTX_CKO_MASK      0x02000000                // RG_WBG_EN_WFTX_CKO[25]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WFTX_CKO_SHFT      25
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_PLL_ADDR       CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_PLL_MASK       0x01000000                // RG_WBG_EN_MAN_PLL[24]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_PLL_SHFT       24
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_ADDR          CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_MASK          0x00800000                // RG_WBG_EN_BPLL[23]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_SHFT          23
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_ADDR          CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_MASK          0x00400000                // RG_WBG_EN_WPLL[22]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_SHFT          22
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_UP_ADDR       CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_UP_MASK       0x00200000                // RG_WBG_EN_BPLL_UP[21]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_UP_SHFT       21
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_UP_ADDR       CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_UP_MASK       0x00100000                // RG_WBG_EN_WPLL_UP[20]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_UP_SHFT       20
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_BT0ADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_BT0ADCK_MASK   0x00080000                // RG_WBG_EN_MAN_BT0ADCK[19]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_BT0ADCK_SHFT   19
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_BT0_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_BT0_ADCKSEL_MASK      0x00040000                // RG_WBG_BT0_ADCKSEL[18]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_BT0_ADCKSEL_SHFT      18
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_BT1ADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_BT1ADCK_MASK   0x00020000                // RG_WBG_EN_MAN_BT1ADCK[17]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_BT1ADCK_SHFT   17
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_BT1_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_BT1_ADCKSEL_MASK      0x00010000                // RG_WBG_BT1_ADCKSEL[16]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_BT1_ADCKSEL_SHFT      16
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF0ADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF0ADCK_MASK   0x00008000                // RG_WBG_EN_MAN_WF0ADCK[15]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF0ADCK_SHFT   15
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF0_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF0_ADCKSEL_MASK      0x00007000                // RG_WBG_WF0_ADCKSEL[14..12]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF0_ADCKSEL_SHFT      12
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF1ADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF1ADCK_MASK   0x00000800                // RG_WBG_EN_MAN_WF1ADCK[11]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF1ADCK_SHFT   11
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF1_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF1_ADCKSEL_MASK      0x00000700                // RG_WBG_WF1_ADCKSEL[10..8]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF1_ADCKSEL_SHFT      8
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF2ADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF2ADCK_MASK   0x00000080                // RG_WBG_EN_MAN_WF2ADCK[7]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF2ADCK_SHFT   7
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF2_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF2_ADCKSEL_MASK      0x00000070                // RG_WBG_WF2_ADCKSEL[6..4]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF2_ADCKSEL_SHFT      4
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF3ADCK_ADDR   CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF3ADCK_MASK   0x00000008                // RG_WBG_EN_MAN_WF3ADCK[3]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_MAN_WF3ADCK_SHFT   3
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF3_ADCKSEL_ADDR      CONN_AFE_CTL_RG_DIG_EN_03_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF3_ADCKSEL_MASK      0x00000007                // RG_WBG_WF3_ADCKSEL[2..0]
#define CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_WF3_ADCKSEL_SHFT      0

/* =====================================================================================

  ---RG_DIG_TOP_01 (0x18041000 + 0x00C)---

    RG_AFE_DBG_SEL[1..0]         - (RW) AFE debug monitor source switch
                                     2'b00: afe_dbg = 32'd0;
                                     2'b01: afe_dbg = afe_dbg1;
                                     2'b10: afe_dbg = afe_dbg2;
                                     2'b11: NA (32'd0)
    RG_DIG_RSV1[12..2]           - (RW) RSV1 in digital
    RG_AFE_ICAP_X2_MODE[13]      - (RW) Internal capture with 2x data bit-width mode
                                     1: 1/2X adc clock and serial to parallel 48 bits I/Q data
                                     0: 1/1X adc clock and 24 bits I/Q data
    RG_AFE_RXDS_SRC_SEL_SW[14]   - (RW) ADC monitor path source selection, Switch Mode (RG_AFE_RXDS_SRC_SEL_SW=1):
                                     000: RSV
                                     001: Select WBG BT1-ADC data/clock
                                     010: RSV
                                     011: RSV
                                     100: RSV
                                     101: RSV
                                     110: Select RFSOC WF0TSSI data/clock
                                     111: Select RFSOC GPS-ADC data/clock
    RG_DIG_RSV2[18..15]          - (RW) RSV2 in digital
    RG_AFE_ICAP_X4_MODE[19]      - (RW) Internal capture with 4x data bit-width mode
                                     1: 1/4X adc clock and serial to parallel 96 bits I/Q data
                                     0: 1/2X adc clock and serial to parallel 48 bits I/Q data or 1/1X of 24bits
    RG_AFE_RXDS_EN[20]           - (RW) ADC monitor path down-sample enable signal (need to set GPIO register for output to PAD)
                                     0: Disable
                                     1: Enable
    RG_AFE_RXDS_AFIFO_MODE[21]   - (RW) ADC monitor path output mux selection
                                     0: normal path 
                                     1: AFIFO path
    RG_AFE_RXDS_AFIFO_EN[22]     - (RW) ADC monitor path AFIFO enable signal
                                     0: Disable
                                     1: Enable
    RG_AFE_RXDS_MUX_SEL[25..23]  - (RW) ADC monitor path output format (clock is placed at bit-0 of monitor-out)
                                     000: all 0
                                     001: Clock and full I, Q data (I: bit 1~12; Q: bit 13~24)
                                     010: Clock and I data only (bit 1~12)
                                     011: Clock and Q data only (bit 1~12)
                                     100: Clock/2 and serial-to-parallel out of I data
                                     (T(1) at bit 13~24; T(2) at bit 1~12)
                                     101: Clock/2 and serial-to-parallel out of Q data
                                     (T(1) at bit 13~24; T(2) at bit 1~12)
                                     110~111: all 0 (RSV)
    RG_AFE_RXDS_RATE_SEL[28..26] - (RW) ADC monitor path down-sample rate selection
                                     (duty of downsampled clock is 50-50)
                                     000: Normal
                                     001: Down data/ck 1/2
                                     010: Down data/ck 1/3
                                     011: Down data/ck 1/4
                                     100: Down data/ck 1/5
                                     101: Down data/ck 1/8
                                     110: Down data/ck 1/15
                                     111: Down data/ck 1/16
    RG_AFE_RXDS_SRC_SEL[31..29]  - (RW) ADC monitor path source selection, Normal Mode (RG_AFE_RXDS_SRC_SEL_SW=0):
                                     000: Select WBG GL1-ADC data and GPS shared clock
                                     001: Select WBG BT0-ADC data/clock
                                     010: Select WBG GL5-ADC data and GPS shared clock
                                     011: Select WBG DFS-ADC data/clock
                                     100: Select WBG WF0-ADC data and WF shared clock
                                     101: Select WBG WF1-ADC data and WF shared clock 
                                     110: Select WBG WF2-ADC data and WF shared clock
                                     111: Select WBG WF3-ADC data and WF shared clock

 =====================================================================================*/
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_SRC_SEL_ADDR    CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_SRC_SEL_MASK    0xE0000000                // RG_AFE_RXDS_SRC_SEL[31..29]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_SRC_SEL_SHFT    29
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_RATE_SEL_ADDR   CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_RATE_SEL_MASK   0x1C000000                // RG_AFE_RXDS_RATE_SEL[28..26]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_RATE_SEL_SHFT   26
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_MUX_SEL_ADDR    CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_MUX_SEL_MASK    0x03800000                // RG_AFE_RXDS_MUX_SEL[25..23]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_MUX_SEL_SHFT    23
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_AFIFO_EN_ADDR   CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_AFIFO_EN_MASK   0x00400000                // RG_AFE_RXDS_AFIFO_EN[22]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_AFIFO_EN_SHFT   22
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_AFIFO_MODE_ADDR CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_AFIFO_MODE_MASK 0x00200000                // RG_AFE_RXDS_AFIFO_MODE[21]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_AFIFO_MODE_SHFT 21
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_EN_ADDR         CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_EN_MASK         0x00100000                // RG_AFE_RXDS_EN[20]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_EN_SHFT         20
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_ICAP_X4_MODE_ADDR    CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_ICAP_X4_MODE_MASK    0x00080000                // RG_AFE_ICAP_X4_MODE[19]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_ICAP_X4_MODE_SHFT    19
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2_ADDR            CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2_MASK            0x00078000                // RG_DIG_RSV2[18..15]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2_SHFT            15
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_SRC_SEL_SW_ADDR CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_SRC_SEL_SW_MASK 0x00004000                // RG_AFE_RXDS_SRC_SEL_SW[14]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_RXDS_SRC_SEL_SW_SHFT 14
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_ICAP_X2_MODE_ADDR    CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_ICAP_X2_MODE_MASK    0x00002000                // RG_AFE_ICAP_X2_MODE[13]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_ICAP_X2_MODE_SHFT    13
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV1_ADDR            CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV1_MASK            0x00001FFC                // RG_DIG_RSV1[12..2]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV1_SHFT            2
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_DBG_SEL_ADDR         CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_DBG_SEL_MASK         0x00000003                // RG_AFE_DBG_SEL[1..0]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_AFE_DBG_SEL_SHFT         0

/* =====================================================================================

  ---RG_WBG_AFE_01 (0x18041000 + 0x010)---

    RG_WBG_AFE_BG[7..0]          - (RW) Bandgap Register
    RG_WBG_TSTDC_EN[8]           - (RW) Enable global switch for QS_VMON_VDC
                                     0: Disable 
                                     1: Enable
    RG_WBG_TSTCK2PAD_EN[9]       - (RW) Enable global open drain for NS_VMON_CK2PAD
                                     0: Disable
                                     1: Enable
    RG_WBG_TSTPAD2CK_EN[10]      - (RW) Enable global ext-CK SW for NS_VMON_PAD2CK
                                     0: Disable
                                     1: Enable
    RG_WBG_TSTPAD_HZ[11]         - (RW) Release GL1 Q  input PAD to HZ mode due to share testing PAD
                                     0: Normal mode (RX mode)
                                     1: HZ mode (testing mode)
    RG_WBG_AFE_RSV1[24..12]      - (RW) RSV
    RG_WBG_AFE_RXCKO_MAN[27..25] - (RW) WF1 RX CKO_EN manual control(msb: WF3)
                                     000: tie low
                                     001: control by DA_WBG_EN_WFRX_CKO
    RG_WBG_AFE_TXCKO_MAN[30..28] - (RW) WF1 TX CKO_EN manual control(msb: WF3)
                                     000: tie low
                                     001: control by DA_WBG_EN_WFTX_CKO
    RG_WBG_AFE_RSV0[31]          - (RW) RSV

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV0_ADDR        CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV0_MASK        0x80000000                // RG_WBG_AFE_RSV0[31]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV0_SHFT        31
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_TXCKO_MAN_ADDR   CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_TXCKO_MAN_MASK   0x70000000                // RG_WBG_AFE_TXCKO_MAN[30..28]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_TXCKO_MAN_SHFT   28
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RXCKO_MAN_ADDR   CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RXCKO_MAN_MASK   0x0E000000                // RG_WBG_AFE_RXCKO_MAN[27..25]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RXCKO_MAN_SHFT   25
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV1_ADDR        CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV1_MASK        0x01FFF000                // RG_WBG_AFE_RSV1[24..12]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV1_SHFT        12
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD_HZ_ADDR       CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD_HZ_MASK       0x00000800                // RG_WBG_TSTPAD_HZ[11]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD_HZ_SHFT       11
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD2CK_EN_ADDR    CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD2CK_EN_MASK    0x00000400                // RG_WBG_TSTPAD2CK_EN[10]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD2CK_EN_SHFT    10
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTCK2PAD_EN_ADDR    CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTCK2PAD_EN_MASK    0x00000200                // RG_WBG_TSTCK2PAD_EN[9]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTCK2PAD_EN_SHFT    9
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTDC_EN_ADDR        CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTDC_EN_MASK        0x00000100                // RG_WBG_TSTDC_EN[8]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTDC_EN_SHFT        8
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_BG_ADDR          CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_BG_MASK          0x000000FF                // RG_WBG_AFE_BG[7..0]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_BG_SHFT          0

/* =====================================================================================

  ---RG_WBG_AFE_02 (0x18041000 + 0x014)---

    RG_WBG_AFE_XTAL_CFG[2..0]    - (RW) XTAL Input Configuration
                                     000: 26MHz
                                     001: 25MHz
                                     010: 40MHz
                                     011: 24MHz
                                     100: 52MHz
    RG_WBG_AFE_WF3_RXDCOC[3]     - (RW) WF3 RX DCOC enable on AFE
                                     Short input PAD to VCM for WF3 RX_DCOC
                                     0: disable
                                     1: enable
    RG_WBG_AFE_WF2_RXDCOC[4]     - (RW) WF2 RX DCOC enable on AFE
                                     Short input PAD to VCM for WF2 RX_DCOC
                                     0: disable
                                     1: enable
    RG_WBG_AFE_WF1_RXDCOC[5]     - (RW) WF1 RX DCOC enable on AFE
                                     Short input PAD to VCM for WF1 RX_DCOC
                                     0: disable
                                     1: enable
    RG_WBG_AFE_WF0_RXDCOC[6]     - (RW) WF0 RX DCOC enable on AFE
                                     Short input PAD to VCM for WF0 RX_DCOC
                                     0: disable
                                     1: enable
    RG_WBG_AFE_WF3_CAL[7]        - (RW) WF3 CAL enable
                                     0: Disable (WF3 RX from WF3 pad)
                                     1: Enable (WF3 RX from WF2 pad)
    RG_WBG_AFE_WF2_CAL[8]        - (RW) WF2 CAL enable
                                     0: Disable (WF2 RX from WF2 pad)
                                     1: Enable (WF2 RX from WF3 pad)
    RG_WBG_AFE_WF1_CAL[9]        - (RW) WF1 CAL enable
                                     0: Disable (WF1 RX from WF1 pad)
                                     1: Enable (WF1 RX from WF0 pad)
    RG_WBG_AFE_WF0_CAL[11..10]   - (RW) WF0 CAL enable
                                     00: Disable (WF0 RX from WF0 pad)
                                     01: Enable (WF0 RX from BT pad)
                                     10: Enable (WF0 RX from WF1 pad)
                                     11: NA
    RG_WBG_AFE_BT0_CAL[13..12]   - (RW) BT0 CAL enable
                                     00: Disable (BT0 RX from BT0 pad)
                                     01: Enable (BT0 RX from WF0 pad)
                                     10: Enable (BT0 RX from BT1 pad)
                                     11: NA
    RG_WBG_AFE_DFS_RXDCOC[14]    - (RW) DFS RX DCOC enable on AFE
                                     Short input PAD to VCM for DFS RX_DCOC
                                     0: disable
                                     1: enable
    RG_WBG_AFE_DFS_CAL[15]       - (RW) DFS CAL enable
                                     0: Disable (DFS RX from WF3 pad)
                                     1: Enable (DFS RX from WF3 pad)
    RG_WBG_AFE_BT1_CAL[16]       - (RW) BT1 CAL enable
                                     0: Disable (BT1 RX from BT1 pad)
                                     1: Enable (BT1 RX from BT0 pad)
    RG_WBG_AFE_RSV2[31..17]      - (RW) RSV

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_RSV2_ADDR        CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_RSV2_MASK        0xFFFE0000                // RG_WBG_AFE_RSV2[31..17]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_RSV2_SHFT        17
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_BT1_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_BT1_CAL_MASK     0x00010000                // RG_WBG_AFE_BT1_CAL[16]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_BT1_CAL_SHFT     16
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_DFS_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_DFS_CAL_MASK     0x00008000                // RG_WBG_AFE_DFS_CAL[15]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_DFS_CAL_SHFT     15
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_DFS_RXDCOC_ADDR  CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_DFS_RXDCOC_MASK  0x00004000                // RG_WBG_AFE_DFS_RXDCOC[14]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_DFS_RXDCOC_SHFT  14
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_BT0_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_BT0_CAL_MASK     0x00003000                // RG_WBG_AFE_BT0_CAL[13..12]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_BT0_CAL_SHFT     12
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF0_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF0_CAL_MASK     0x00000C00                // RG_WBG_AFE_WF0_CAL[11..10]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF0_CAL_SHFT     10
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF1_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF1_CAL_MASK     0x00000200                // RG_WBG_AFE_WF1_CAL[9]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF1_CAL_SHFT     9
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF2_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF2_CAL_MASK     0x00000100                // RG_WBG_AFE_WF2_CAL[8]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF2_CAL_SHFT     8
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF3_CAL_ADDR     CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF3_CAL_MASK     0x00000080                // RG_WBG_AFE_WF3_CAL[7]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF3_CAL_SHFT     7
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF0_RXDCOC_ADDR  CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF0_RXDCOC_MASK  0x00000040                // RG_WBG_AFE_WF0_RXDCOC[6]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF0_RXDCOC_SHFT  6
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF1_RXDCOC_ADDR  CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF1_RXDCOC_MASK  0x00000020                // RG_WBG_AFE_WF1_RXDCOC[5]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF1_RXDCOC_SHFT  5
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF2_RXDCOC_ADDR  CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF2_RXDCOC_MASK  0x00000010                // RG_WBG_AFE_WF2_RXDCOC[4]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF2_RXDCOC_SHFT  4
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF3_RXDCOC_ADDR  CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF3_RXDCOC_MASK  0x00000008                // RG_WBG_AFE_WF3_RXDCOC[3]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_WF3_RXDCOC_SHFT  3
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_XTAL_CFG_ADDR    CONN_AFE_CTL_RG_WBG_AFE_02_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_XTAL_CFG_MASK    0x00000007                // RG_WBG_AFE_XTAL_CFG[2..0]
#define CONN_AFE_CTL_RG_WBG_AFE_02_RG_WBG_AFE_XTAL_CFG_SHFT    0

/* =====================================================================================

  ---RG_WBG_RCK_01 (0x18041000 + 0x018)---

    RG_WBG_RCK_RSV[14..0]        - (RW) Reserve
    RG_WBG_RCK_CSEL_MEN[15]      - (RW) WiFi TX  tuning bandwidth control sel
                                     0: Control by RC calibration
                                     1: Control by RG
    RG_WBG_RCK_CSEL[20..16]      - (RW) WiFi TX tuning bandwidth control in manual mode
                                     00000: max bandwidth
                                     11111: min bandwidth
    RG_WBG_RCK_CALTARGET_MEN[21] - (RW) RC Calibration target sel
                                     0: Hardware control by CFG
                                     1: Manual control by RG
    RG_WBG_RCK_CALTARGET[31..22] - (RW) RC Calibration target count, 
                                     00_1001_1001 (153): for XTAL=26M

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CALTARGET_ADDR   CONN_AFE_CTL_RG_WBG_RCK_01_ADDR
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CALTARGET_MASK   0xFFC00000                // RG_WBG_RCK_CALTARGET[31..22]
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CALTARGET_SHFT   22
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CALTARGET_MEN_ADDR CONN_AFE_CTL_RG_WBG_RCK_01_ADDR
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CALTARGET_MEN_MASK 0x00200000                // RG_WBG_RCK_CALTARGET_MEN[21]
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CALTARGET_MEN_SHFT 21
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CSEL_ADDR        CONN_AFE_CTL_RG_WBG_RCK_01_ADDR
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CSEL_MASK        0x001F0000                // RG_WBG_RCK_CSEL[20..16]
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CSEL_SHFT        16
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CSEL_MEN_ADDR    CONN_AFE_CTL_RG_WBG_RCK_01_ADDR
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CSEL_MEN_MASK    0x00008000                // RG_WBG_RCK_CSEL_MEN[15]
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_CSEL_MEN_SHFT    15
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_RSV_ADDR         CONN_AFE_CTL_RG_WBG_RCK_01_ADDR
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_RSV_MASK         0x00007FFF                // RG_WBG_RCK_RSV[14..0]
#define CONN_AFE_CTL_RG_WBG_RCK_01_RG_WBG_RCK_RSV_SHFT         0

/* =====================================================================================

  ---RG_WBG_RCK_02 (0x18041000 + 0x01C)---

    RG_WBG_RCK_RSV2[15..0]       - (RW) Reserve

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_RCK_02_RG_WBG_RCK_RSV2_ADDR        CONN_AFE_CTL_RG_WBG_RCK_02_ADDR
#define CONN_AFE_CTL_RG_WBG_RCK_02_RG_WBG_RCK_RSV2_MASK        0x0000FFFF                // RG_WBG_RCK_RSV2[15..0]
#define CONN_AFE_CTL_RG_WBG_RCK_02_RG_WBG_RCK_RSV2_SHFT        0

/* =====================================================================================

  ---RGS_WBG_RO (0x18041000 + 0x020)---

    RGS_WBG_BPLL_RO[11..0]       - (RO) RGS reserve for BPLL
    RGS_WBG_WPLL_RO[23..12]      - (RO) RGS reserve for WPLL
    RGS_WBG_RCK_CSEL_RO[28..24]  - (RO) Auto calibration word  write to register
    wpll_rdy_sync_RO[29]         - (RO)  xxx 
    bpll_rdy_sync_RO[30]         - (RO)  xxx 
    RESERVED31[31]               - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RGS_WBG_RO_bpll_rdy_sync_RO_ADDR          CONN_AFE_CTL_RGS_WBG_RO_ADDR
#define CONN_AFE_CTL_RGS_WBG_RO_bpll_rdy_sync_RO_MASK          0x40000000                // bpll_rdy_sync_RO[30]
#define CONN_AFE_CTL_RGS_WBG_RO_bpll_rdy_sync_RO_SHFT          30
#define CONN_AFE_CTL_RGS_WBG_RO_wpll_rdy_sync_RO_ADDR          CONN_AFE_CTL_RGS_WBG_RO_ADDR
#define CONN_AFE_CTL_RGS_WBG_RO_wpll_rdy_sync_RO_MASK          0x20000000                // wpll_rdy_sync_RO[29]
#define CONN_AFE_CTL_RGS_WBG_RO_wpll_rdy_sync_RO_SHFT          29
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_RCK_CSEL_RO_ADDR       CONN_AFE_CTL_RGS_WBG_RO_ADDR
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_RCK_CSEL_RO_MASK       0x1F000000                // RGS_WBG_RCK_CSEL_RO[28..24]
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_RCK_CSEL_RO_SHFT       24
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_WPLL_RO_ADDR           CONN_AFE_CTL_RGS_WBG_RO_ADDR
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_WPLL_RO_MASK           0x00FFF000                // RGS_WBG_WPLL_RO[23..12]
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_WPLL_RO_SHFT           12
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_BPLL_RO_ADDR           CONN_AFE_CTL_RGS_WBG_RO_ADDR
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_BPLL_RO_MASK           0x00000FFF                // RGS_WBG_BPLL_RO[11..0]
#define CONN_AFE_CTL_RGS_WBG_RO_RGS_WBG_BPLL_RO_SHFT           0

/* =====================================================================================

  ---RG_WBG_PLL_01 (0x18041000 + 0x024)---

    RG_BPLL_FC_DLY[1..0]         - (RW) PLL Fast Charge time selection
    RG_BPLL_RST_DLY[3..2]        - (RW) PLL reset time selection
                                     2'b00: 30*Tref
                                     2'b01: 62*Tref
                                     2'b10: 126*Tref
                                     2'b11: 254*Tref
    RG_BPLL_BC_SEL[5..4]         - (RW) Set loop parameters Cz
                                     2'b00:  5pF
                                     2'b01:  6.25pF
                                     2'b10:  7.5pF
                                     2'b11:  8.75pF
    RG_BPLL_BPB_SEL[7..6]        - (RW) Set loop parameters CpB
                                     2'b00:  200fF
                                     ... (200fF/step)
                                     2'b11:  800fF
    RG_BPLL_BPA_SEL[10..8]       - (RW) Set loop parameters Cp
                                     3'b000:  40fF
                                     ... (40fF/step)
                                     3'b111:  320fF
    RG_BPLL_MON_VCTRL_EN[11]     - (RW) Enable monitor Vctrl Voltage for debug
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_POSDIV_MAN[13..12]   - (RW) Post-divider ratio for single-end clock output
                                     2'bx0: VCO/1
                                     2'bx1: VCO/2
    RG_BPLL_PREDIV_MAN[15..14]   - (RW) Pre divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b1x: /4
    RG_BPLL_FBKDIV_MAN[23..16]   - (RW) Set feedback divider ratio in digital macro
    RG_BPLL_FBSEL_MAN[24]        - (RW) Set feedback divider ratio in PLL
                                     1'b0: /1
                                     1'b1: /2
    RG_BPLL_BRSEL_MAN[27..25]    - (RW) Resistance adjustment for Bandwidth
                                     3'b000:  105K
                                     ... (10.5K/step)
                                     3'b111:  31.5K
    RG_BPLL_CPSEL_MAN[31..28]    - (RW) CHP current select
                                     4'b0001=5uA
                                     ...(5uA/step)
                                     4'b1111=75uA

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_CPSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_CPSEL_MAN_MASK      0xF0000000                // RG_BPLL_CPSEL_MAN[31..28]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_CPSEL_MAN_SHFT      28
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BRSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BRSEL_MAN_MASK      0x0E000000                // RG_BPLL_BRSEL_MAN[27..25]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BRSEL_MAN_SHFT      25
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FBSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FBSEL_MAN_MASK      0x01000000                // RG_BPLL_FBSEL_MAN[24]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FBSEL_MAN_SHFT      24
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FBKDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FBKDIV_MAN_MASK     0x00FF0000                // RG_BPLL_FBKDIV_MAN[23..16]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FBKDIV_MAN_SHFT     16
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_PREDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_PREDIV_MAN_MASK     0x0000C000                // RG_BPLL_PREDIV_MAN[15..14]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_PREDIV_MAN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_POSDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_POSDIV_MAN_MASK     0x00003000                // RG_BPLL_POSDIV_MAN[13..12]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_POSDIV_MAN_SHFT     12
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_MON_VCTRL_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_MON_VCTRL_EN_MASK   0x00000800                // RG_BPLL_MON_VCTRL_EN[11]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_MON_VCTRL_EN_SHFT   11
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BPA_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BPA_SEL_MASK        0x00000700                // RG_BPLL_BPA_SEL[10..8]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BPA_SEL_SHFT        8
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BPB_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BPB_SEL_MASK        0x000000C0                // RG_BPLL_BPB_SEL[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BPB_SEL_SHFT        6
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BC_SEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BC_SEL_MASK         0x00000030                // RG_BPLL_BC_SEL[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_BC_SEL_SHFT         4
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_RST_DLY_ADDR        CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_RST_DLY_MASK        0x0000000C                // RG_BPLL_RST_DLY[3..2]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_RST_DLY_SHFT        2
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FC_DLY_ADDR         CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FC_DLY_MASK         0x00000003                // RG_BPLL_FC_DLY[1..0]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_BPLL_FC_DLY_SHFT         0

/* =====================================================================================

  ---RG_WBG_PLL_02 (0x18041000 + 0x028)---

    RG_BPLL_RSV[0]               - (RW) reserve
    RG_BPLL_RLPSEL[1]            - (RW) IBAND Rmos select
                                     1'b0: BW*1
                                     1'b1: BW*2
    RG_BPLL_MANSEL_EN[2]         - (RW) Enable RG Manual mode
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_TCL_EN[3]            - (RW) Enable TCL
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_DIG_DEBUG_SEL[4]     - (RW) read RGS selection
    RG_BPLL_DIG_LOAD_EN[5]       - (RW) Enable load IBAND
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_KBAND_PREDIV[7..6]   - (RW) Kband pre-divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b1x: /4
    RG_BPLL_DIG_IBAND_MAN[17..8] - (RW) IBAND loading code
    RG_BPLL_RICO_SEL[18]         - (RW) Rkico selection
                                     1'b0: 10.5K
                                     1'b1: 17K
    RG_BPLL_MON_LDO_EN[21..19]   - (RW) monitor BPLL LDO Voltage for debug
                                     3'b000: Disable
                                     3'b001: LDO_PFD
                                     3'b010: LDO_CHP
                                     3'b011: LDO_LPF
                                     3'b100: LDO_VCO
    RG_BPLL_LDO32M_SEL[23..22]   - (RW) reserve
    RG_BPLL_LDOVCO_SEL[25..24]   - (RW) LDO_VCO Voltage selection
                                     (same as LDO_PFD)
    RG_BPLL_LDOLPF_SEL[27..26]   - (RW) LDO_LPF Voltage selection
                                     2'b00: 1.175V
                                     2'b01: 1.200V
                                     2'b10: 1.225V
                                     2'b11: 1.250V
    RG_BPLL_LDOCHP_SEL[29..28]   - (RW) LDO_CHP Voltage selection
                                     (same as LDO_PFD)
    RG_BPLL_LDOPFD_SEL[31..30]   - (RW) LDO_PFD Voltage selection
                                     2'b00: 0.875V
                                     2'b01: 0.900V
                                     2'b10: 0.925V
                                     2'b11: 0.950V

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOPFD_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOPFD_SEL_MASK     0xC0000000                // RG_BPLL_LDOPFD_SEL[31..30]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOPFD_SEL_SHFT     30
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOCHP_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOCHP_SEL_MASK     0x30000000                // RG_BPLL_LDOCHP_SEL[29..28]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOCHP_SEL_SHFT     28
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOLPF_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOLPF_SEL_MASK     0x0C000000                // RG_BPLL_LDOLPF_SEL[27..26]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOLPF_SEL_SHFT     26
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOVCO_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOVCO_SEL_MASK     0x03000000                // RG_BPLL_LDOVCO_SEL[25..24]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDOVCO_SEL_SHFT     24
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDO32M_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDO32M_SEL_MASK     0x00C00000                // RG_BPLL_LDO32M_SEL[23..22]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_LDO32M_SEL_SHFT     22
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_MON_LDO_EN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_MON_LDO_EN_MASK     0x00380000                // RG_BPLL_MON_LDO_EN[21..19]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_MON_LDO_EN_SHFT     19
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RICO_SEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RICO_SEL_MASK       0x00040000                // RG_BPLL_RICO_SEL[18]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RICO_SEL_SHFT       18
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_IBAND_MAN_ADDR  CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_IBAND_MAN_MASK  0x0003FF00                // RG_BPLL_DIG_IBAND_MAN[17..8]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_IBAND_MAN_SHFT  8
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_KBAND_PREDIV_ADDR   CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_KBAND_PREDIV_MASK   0x000000C0                // RG_BPLL_KBAND_PREDIV[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_KBAND_PREDIV_SHFT   6
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_LOAD_EN_ADDR    CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_LOAD_EN_MASK    0x00000020                // RG_BPLL_DIG_LOAD_EN[5]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_LOAD_EN_SHFT    5
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_DEBUG_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_DEBUG_SEL_MASK  0x00000010                // RG_BPLL_DIG_DEBUG_SEL[4]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_DIG_DEBUG_SEL_SHFT  4
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_TCL_EN_ADDR         CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_TCL_EN_MASK         0x00000008                // RG_BPLL_TCL_EN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_TCL_EN_SHFT         3
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_MANSEL_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_MANSEL_EN_MASK      0x00000004                // RG_BPLL_MANSEL_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_MANSEL_EN_SHFT      2
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RLPSEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RLPSEL_MASK         0x00000002                // RG_BPLL_RLPSEL[1]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RLPSEL_SHFT         1
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RSV_ADDR            CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RSV_MASK            0x00000001                // RG_BPLL_RSV[0]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_BPLL_RSV_SHFT            0

/* =====================================================================================

  ---RG_WBG_PLL_03 (0x18041000 + 0x02C)---

    RG_WPLL_FC_DLY[1..0]         - (RW) PLL Fast Charge time selection
    RG_WPLL_RST_DLY[3..2]        - (RW) PLL reset time selection
                                     2'b00: 30*Tref
                                     2'b01: 62*Tref
                                     2'b10: 126*Tref
                                     2'b11: 254*Tref
    RG_WPLL_BC_SEL[5..4]         - (RW) Set loop parameters Cz
                                     2'b00:  5pF
                                     2'b01:  6.25pF
                                     2'b10:  7.5pF
                                     2'b11:  8.75pF
    RG_WPLL_BPB_SEL[7..6]        - (RW) Set loop parameters CpB
                                     2'b00:  200fF
                                     ... (200fF/step)
                                     2'b11:  800fF
    RG_WPLL_BPA_SEL[10..8]       - (RW) Set loop parameters Cp
                                     3'b000:  40fF
                                     ... (40fF/step)
                                     3'b111:  320fF
    RG_WPLL_MON_VCTRL_EN[11]     - (RW) Enable monitor Vctrl Voltage for debug
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WPLL_POSDIV_MAN[13..12]   - (RW) Post-divider ratio for single-end clock output
                                     2'bx0: VCO/1
                                     2'bx1: VCO/2
    RG_WPLL_PREDIV_MAN[15..14]   - (RW) Pre divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b1x: /4
    RG_WPLL_FBKDIV_MAN[23..16]   - (RW) Set feedback divider ratio in digital macro
    RG_WPLL_FBSEL_MAN[24]        - (RW) Set feedback divider ratio in PLL
                                     1'b0: /1
                                     1'b1: /2
    RG_WPLL_BRSEL_MAN[27..25]    - (RW) Resistance adjustment for Bandwidth
                                     3'b000:  105K
                                     ... (10.5K/step)
                                     3'b111:  31.5K
    RG_WPLL_CPSEL_MAN[31..28]    - (RW) CHP current select
                                     4'b0001=5uA
                                     ...(5uA/step)
                                     4'b1111=75uA

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_CPSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_CPSEL_MAN_MASK      0xF0000000                // RG_WPLL_CPSEL_MAN[31..28]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_CPSEL_MAN_SHFT      28
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BRSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BRSEL_MAN_MASK      0x0E000000                // RG_WPLL_BRSEL_MAN[27..25]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BRSEL_MAN_SHFT      25
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FBSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FBSEL_MAN_MASK      0x01000000                // RG_WPLL_FBSEL_MAN[24]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FBSEL_MAN_SHFT      24
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FBKDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FBKDIV_MAN_MASK     0x00FF0000                // RG_WPLL_FBKDIV_MAN[23..16]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FBKDIV_MAN_SHFT     16
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_PREDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_PREDIV_MAN_MASK     0x0000C000                // RG_WPLL_PREDIV_MAN[15..14]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_PREDIV_MAN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_POSDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_POSDIV_MAN_MASK     0x00003000                // RG_WPLL_POSDIV_MAN[13..12]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_POSDIV_MAN_SHFT     12
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_MON_VCTRL_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_MON_VCTRL_EN_MASK   0x00000800                // RG_WPLL_MON_VCTRL_EN[11]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_MON_VCTRL_EN_SHFT   11
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPA_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPA_SEL_MASK        0x00000700                // RG_WPLL_BPA_SEL[10..8]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPA_SEL_SHFT        8
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPB_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPB_SEL_MASK        0x000000C0                // RG_WPLL_BPB_SEL[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPB_SEL_SHFT        6
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BC_SEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BC_SEL_MASK         0x00000030                // RG_WPLL_BC_SEL[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BC_SEL_SHFT         4
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_RST_DLY_ADDR        CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_RST_DLY_MASK        0x0000000C                // RG_WPLL_RST_DLY[3..2]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_RST_DLY_SHFT        2
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FC_DLY_ADDR         CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FC_DLY_MASK         0x00000003                // RG_WPLL_FC_DLY[1..0]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_FC_DLY_SHFT         0

/* =====================================================================================

  ---RG_WBG_PLL_04 (0x18041000 + 0x030)---

    RG_WPLL_1920_960_MAN[0]      - (RW) when RG_WF_62_61_MAN=1
                                     1'b0: 960MHz
                                     1'b1: 1920MHz
    RG_WPLL_RLPSEL[1]            - (RW) IBAND Rmos select
                                     1'b0: BW*1
                                     1'b1: BW*2
    RG_WPLL_MANSEL_EN[2]         - (RW) Enable RG Manual mode
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WPLL_TCL_EN[3]            - (RW) Enable TCL
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WPLL_DIG_DEBUG_SEL[4]     - (RW) read RGS selection
    RG_WPLL_DIG_LOAD_EN[5]       - (RW) Enable load IBAND
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WPLL_KBAND_PREDIV[7..6]   - (RW) Kband pre-divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b1x: /4
    RG_WPLL_DIG_IBAND_MAN[17..8] - (RW) IBAND loading code
    RG_WPLL_RICO_SEL[18]         - (RW) Rkico selection
                                     1'b0: 10.5K
                                     1'b1: 17K
    RG_WPLL_MON_LDO_EN[21..19]   - (RW) monitor WPLL LDO Voltage for debug
                                     3'b000: Disable
                                     3'b001: LDO_PFD
                                     3'b010: LDO_CHP
                                     3'b011: LDO_LPF
                                     3'b100: LDO_VCO
                                     3'b101: LDO_32M
    RG_WPLL_LDO32M_SEL[23..22]   - (RW) reserve
    RG_WPLL_LDOVCO_SEL[25..24]   - (RW) LDO_VCO Voltage selection
                                     (same as LDO_PFD)
    RG_WPLL_LDOLPF_SEL[27..26]   - (RW) LDO_LPF Voltage selection
                                     2'b00: 1.175V
                                     2'b01: 1.200V
                                     2'b10: 1.225V
                                     2'b11: 1.250V
    RG_WPLL_LDOCHP_SEL[29..28]   - (RW) LDO_CHP Voltage selection
                                     (same as LDO_PFD)
    RG_WPLL_LDOPFD_SEL[31..30]   - (RW) LDO_PFD Voltage selection
                                     2'b00: 0.875V
                                     2'b01: 0.900V
                                     2'b10: 0.925V
                                     2'b11: 0.950V

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOPFD_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOPFD_SEL_MASK     0xC0000000                // RG_WPLL_LDOPFD_SEL[31..30]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOPFD_SEL_SHFT     30
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOCHP_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOCHP_SEL_MASK     0x30000000                // RG_WPLL_LDOCHP_SEL[29..28]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOCHP_SEL_SHFT     28
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOLPF_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOLPF_SEL_MASK     0x0C000000                // RG_WPLL_LDOLPF_SEL[27..26]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOLPF_SEL_SHFT     26
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOVCO_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOVCO_SEL_MASK     0x03000000                // RG_WPLL_LDOVCO_SEL[25..24]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDOVCO_SEL_SHFT     24
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDO32M_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDO32M_SEL_MASK     0x00C00000                // RG_WPLL_LDO32M_SEL[23..22]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LDO32M_SEL_SHFT     22
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MON_LDO_EN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MON_LDO_EN_MASK     0x00380000                // RG_WPLL_MON_LDO_EN[21..19]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MON_LDO_EN_SHFT     19
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RICO_SEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RICO_SEL_MASK       0x00040000                // RG_WPLL_RICO_SEL[18]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RICO_SEL_SHFT       18
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_IBAND_MAN_ADDR  CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_IBAND_MAN_MASK  0x0003FF00                // RG_WPLL_DIG_IBAND_MAN[17..8]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_IBAND_MAN_SHFT  8
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_PREDIV_ADDR   CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_PREDIV_MASK   0x000000C0                // RG_WPLL_KBAND_PREDIV[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_PREDIV_SHFT   6
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_LOAD_EN_ADDR    CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_LOAD_EN_MASK    0x00000020                // RG_WPLL_DIG_LOAD_EN[5]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_LOAD_EN_SHFT    5
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_DEBUG_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_DEBUG_SEL_MASK  0x00000010                // RG_WPLL_DIG_DEBUG_SEL[4]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DIG_DEBUG_SEL_SHFT  4
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_TCL_EN_ADDR         CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_TCL_EN_MASK         0x00000008                // RG_WPLL_TCL_EN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_TCL_EN_SHFT         3
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MANSEL_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MANSEL_EN_MASK      0x00000004                // RG_WPLL_MANSEL_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MANSEL_EN_SHFT      2
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RLPSEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RLPSEL_MASK         0x00000002                // RG_WPLL_RLPSEL[1]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RLPSEL_SHFT         1
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_1920_960_MAN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_1920_960_MAN_MASK   0x00000001                // RG_WPLL_1920_960_MAN[0]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_1920_960_MAN_SHFT   0

/* =====================================================================================

  ---RG_WBG_PLL_05 (0x18041000 + 0x034)---

    RG_PLL05_RSV[1..0]           - (RW) reserve
    RG_WFRX_DIV2_EN[2]           - (RW) Enable CK_WFRX div2 funtion
                                     1'b0: prediv /1
                                     1'b1: prediv /2
    RG_WF_62_61_MAN[3]           - (RW) Enable PLLDIV RG Manual mode
                                     1'b0: Disable
                                     1'b1: Enable
    RG_PLLDIV_DIV2_EN_MAN[4]     - (RW) when RG_WF_62_61_MAN=1
                                     1'b0: PLLDIV prediv /1
                                     1'b1: PLLDIV prediv /2
                                     except WFRX
    RG_PLLTOP_DCTST_EN[5]        - (RW) Enable PLLTOP DCMON switch
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BTTXCAL_DIV2_ENB[6]       - (RW) Enable CK_BTCAL div2 funtion
                                     1'b0: 416M
                                     1'b1: 832M
    RG_CK_PLL_TST_EN[8..7]       - (RW) CK_PLL_TST selection
                                     2'b00: GND/CKSQ
                                     2'b01:BPLL
                                     2'b10:WPLL
                                     2'b11:CK128M
    RG_CLKSQ_CKTST_EN[9]         - (RW) Enable monitor CLKSQ clock for debug
                                     1'b0: Disable
                                     1'b1: Enable
    RG_PLLDIV_LDO_VSEL[11..10]   - (RW) PLLDIV LDO group Voltage selection
                                     2'b00: 0.875V
                                     2'b01: 0.900V
                                     2'b10: 0.925V
                                     2'b11: 0.950V
    RG_PLLDIV_MONDC_LDO[14..12]  - (RW) monitor PLLDIV_LDO Voltage for debug
                                     3'b000: Disable
                                     3'b001: LDO_BDIV
                                     3'b010: LDO_WTX480
                                     3'b011: LDO_WTX960
                                     3'b100: LDO_WRX_VALID
                                     3'b101: LDO_WRX
    RG_CLKSQ_MONDC_EN[16..15]    - (RW) monitor CLKSQ_LDO Voltage for debug
                                     2'b00: Disable
                                     2'b01: CLKSQ_LDO1
                                     2'b10: CLKSQ_LDO2
    RG_CLKSQ_MONCK_SEL[17]       - (RW) selection of monitor CLKSQ clock
                                     1'b0: X1
                                     1'b1: X2
    RG_CLKSQ_MONCK_EN[18]        - (RW) Enable monitor CLKSQ clock for debug
                                     1'b0: Disable
                                     1'b1: Enable
    RG_CLKSQ_LDO2_VSEL[20..19]   - (RW) CKLSQ_LDO2 Voltage selection
                                     2'b00: 0.725V
                                     2'b01: 0.850V
                                     2'b10: 0.875V
                                     2'b11: 0.900V
    RG_CLKSQ_LDO1_VSEL[22..21]   - (RW) CLKSQ_LDO1 Voltage selection
                                     2'b00: 1.175V
                                     2'b01: 1.200V
                                     2'b10: 1.225V
                                     2'b11: 1.250V
    RG_BDIV_force_EN[23]         - (RW)  xxx 
    RG_GDIV_force_EN[24]         - (RW)  xxx 
    RG_WFTX_DIV_force_EN[25]     - (RW)  xxx 
    RG_WFRX_DIV_force_EN[26]     - (RW)  xxx 
    RG_WPLL_CK2DIV_force_EN[27]  - (RW)  xxx 
    RG_BPLL_CK2DIV_force_EN[28]  - (RW)  xxx 
    RG_WFTX_DIV480_EN[29]        - (RW) Enable CK_WFTX_480M
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WFRX_VALID4_EN[30]        - (RW) Enable CK_WFRX_VALID4
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WPLL_62_61[31]            - (RW) WiFi mode selection
                                     1'b0: 61
                                     1'b1: 62

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WPLL_62_61_ADDR          CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WPLL_62_61_MASK          0x80000000                // RG_WPLL_62_61[31]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WPLL_62_61_SHFT          31
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_VALID4_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_VALID4_EN_MASK      0x40000000                // RG_WFRX_VALID4_EN[30]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_VALID4_EN_SHFT      30
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFTX_DIV480_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFTX_DIV480_EN_MASK      0x20000000                // RG_WFTX_DIV480_EN[29]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFTX_DIV480_EN_SHFT      29
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_CK2DIV_force_EN_ADDR CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_CK2DIV_force_EN_MASK 0x10000000                // RG_BPLL_CK2DIV_force_EN[28]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_CK2DIV_force_EN_SHFT 28
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WPLL_CK2DIV_force_EN_ADDR CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WPLL_CK2DIV_force_EN_MASK 0x08000000                // RG_WPLL_CK2DIV_force_EN[27]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WPLL_CK2DIV_force_EN_SHFT 27
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_DIV_force_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_DIV_force_EN_MASK   0x04000000                // RG_WFRX_DIV_force_EN[26]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_DIV_force_EN_SHFT   26
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFTX_DIV_force_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFTX_DIV_force_EN_MASK   0x02000000                // RG_WFTX_DIV_force_EN[25]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFTX_DIV_force_EN_SHFT   25
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_GDIV_force_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_GDIV_force_EN_MASK       0x01000000                // RG_GDIV_force_EN[24]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_GDIV_force_EN_SHFT       24
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BDIV_force_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BDIV_force_EN_MASK       0x00800000                // RG_BDIV_force_EN[23]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BDIV_force_EN_SHFT       23
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_LDO1_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_LDO1_VSEL_MASK     0x00600000                // RG_CLKSQ_LDO1_VSEL[22..21]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_LDO1_VSEL_SHFT     21
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_LDO2_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_LDO2_VSEL_MASK     0x00180000                // RG_CLKSQ_LDO2_VSEL[20..19]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_LDO2_VSEL_SHFT     19
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONCK_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONCK_EN_MASK      0x00040000                // RG_CLKSQ_MONCK_EN[18]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONCK_EN_SHFT      18
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONCK_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONCK_SEL_MASK     0x00020000                // RG_CLKSQ_MONCK_SEL[17]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONCK_SEL_SHFT     17
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONDC_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONDC_EN_MASK      0x00018000                // RG_CLKSQ_MONDC_EN[16..15]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_MONDC_EN_SHFT      15
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_MONDC_LDO_ADDR    CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_MONDC_LDO_MASK    0x00007000                // RG_PLLDIV_MONDC_LDO[14..12]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_MONDC_LDO_SHFT    12
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_LDO_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_LDO_VSEL_MASK     0x00000C00                // RG_PLLDIV_LDO_VSEL[11..10]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_LDO_VSEL_SHFT     10
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_CKTST_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_CKTST_EN_MASK      0x00000200                // RG_CLKSQ_CKTST_EN[9]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CLKSQ_CKTST_EN_SHFT      9
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CK_PLL_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CK_PLL_TST_EN_MASK       0x00000180                // RG_CK_PLL_TST_EN[8..7]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_CK_PLL_TST_EN_SHFT       7
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BTTXCAL_DIV2_ENB_ADDR    CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BTTXCAL_DIV2_ENB_MASK    0x00000040                // RG_BTTXCAL_DIV2_ENB[6]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BTTXCAL_DIV2_ENB_SHFT    6
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLTOP_DCTST_EN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLTOP_DCTST_EN_MASK     0x00000020                // RG_PLLTOP_DCTST_EN[5]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLTOP_DCTST_EN_SHFT     5
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_DIV2_EN_MAN_ADDR  CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_DIV2_EN_MAN_MASK  0x00000010                // RG_PLLDIV_DIV2_EN_MAN[4]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLLDIV_DIV2_EN_MAN_SHFT  4
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WF_62_61_MAN_ADDR        CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WF_62_61_MAN_MASK        0x00000008                // RG_WF_62_61_MAN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WF_62_61_MAN_SHFT        3
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_DIV2_EN_ADDR        CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_DIV2_EN_MASK        0x00000004                // RG_WFRX_DIV2_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_WFRX_DIV2_EN_SHFT        2
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLL05_RSV_ADDR           CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLL05_RSV_MASK           0x00000003                // RG_PLL05_RSV[1..0]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_PLL05_RSV_SHFT           0

/* =====================================================================================

  ---RG_WBG_PLL_06 (0x18041000 + 0x038)---

    RG_PLL06_RSV[31..0]          - (RW) reserve

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_PLL06_RSV_ADDR           CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_PLL06_RSV_MASK           0xFFFFFFFF                // RG_PLL06_RSV[31..0]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_PLL06_RSV_SHFT           0

/* =====================================================================================

  ---RG_WBG_PLL_07 (0x18041000 + 0x03C)---

    RG_PLL07_RSV[31..0]          - (RW) reserve

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_PLL07_RSV_ADDR           CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_PLL07_RSV_MASK           0xFFFFFFFF                // RG_PLL07_RSV[31..0]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_PLL07_RSV_SHFT           0

/* =====================================================================================

  ---RG_WBG_GL1_01 (0x18041000 + 0x040)---

    RG_GL1_AD_LDO0P8_VSEL[2..0]  - (RW) 0.8V LDO tuning
                                     111:0.860
                                     110: 0.851
                                     101: 0.838
                                     100: 0.828 (POR)
                                     011: 0.815
                                     010: 0.804
                                     001: 0.792
                                     000: 0.782
    RG_GL1_AD_VREF_SEL[5..3]     - (RW) VREF selection
                                     111: 0.861
                                     110: 0.850
                                     101: 0.838
                                     100: 0.827
                                     011: 0.814
                                     010: 0.803 (POR)
                                     001: 0.791
                                     000: 0.780
    RG_GL1_DUODRI[6]             - (RW) 1: reference gen doubled current
                                     0: reference gen normal current
    RG_GL1_RAW_OUT_EN[7]         - (RW) 1: Raw data out
                                     0: Normal data out
    RG_GL1_OUT_CLK_DIV_H4_L2[8]  - (RW) 1: output clock divided by 4
                                     0: output clock divided by 2
    RG_GL1_OUT_CLK_DIV_EN[9]     - (RW) 1: output clock divider enable 
                                     0: disable
    RG_GL1_AD_OUTPUT_EN[10]      - (RW) 1: output enable 
                                     0: output disable
    RG_GL1_AD_CK_OUT_RISE[11]    - (RW) 1: Output clock uses positive polarity
                                     0: Output clock uses negative polarity
    RG_GL1_AD_CH_DFF_RISE[12]    - (RW) 1: DFF sync clock uses positive polarity
                                     0: DFF sync clock uses negative polarity
    RG_GL1_AD_LDO_PUMP_EN[13]    - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_GL1_AD_COMP_TAIL_MSB[14]  - (RW) 1: enlarge comparator tail transistor size for first 3 MSBs
                                     0: default size
    RG_GL1_AD_COMP_TAIL[15]      - (RW) 1: enlarge comparator tail transistor size
                                     0: default size
    RG_GL1_MODE_L1L5_SYNC_CLK[16] - (RW) 1: L1 and L5 dual band use external post-divider clock
                                     0: Internal
    RG_GL1_MODE_BT_GPS[17]       - (RW) 1: BT Mode for CLK Selection
                                     0: GPS Mode for CLK Selection (always 0 for GPS RX)
    RG_GL1_AD_EXT_CLK_SEL[18]    - (RW) ADC clock select 
                                     (keep internal PLL clock, clock source is controled by RG<21>)
                                     1: External clock source
                                     0: Internal PLL as clock
    RG_GL1_AD_QCH_EN[19]         - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_GL1_AD_ICH_EN[20]         - (RW) 1: I channel enable
                                     0: I channel disable
    RG_GL1_AD_VAR_CLKS[21]       - (RW) 1: variable clks
                                     0: fixed clks
    RG_GL1_AD_LSB_EN[22]         - (RW) 1: 10bit cycles conversion
                                     0: 11 bit cycles conversion
    RG_GL1_AD_VGEN_PUMP_EN[23]   - (RW) 1: reference gen pump enable
                                     0: reference gen pump disable
    RG_GL1_CK_CH_RISE[24]        - (RW) 1: Decoder output clock uses positive polarity
                                     0: Decoder output clock uses negative polarity
    RG_GPS_EXT_CLK_SEL[25]       - (RW) GPS TOPLV clock select
                                     0: Internal PLL as clock
                                     1: External clock source
    RG_GL1_INTERNAL_ADCKSEL[26]  - (RW) Internal ADC Clock Divider control (for BT)
                                     0: ADC normal mode clock=32MHz
                                     1: ADC normal mode clock=64MHz
    RG_GL1_RG_DA_Mode_ADCKSEL[27] - (RW) 1: RG Mode
                                     0: DA Mode
    RG_GPS_ADCKSEL[29..28]       - (RW) ADC Clock Divider control
                                     00: GPSADC normal mode clock=16MHz
                                     01: GPSADC normal mode clock=64MHz
                                     10: GPSADC normal mode clock=128MHz
    RG_GL1_AD_EN[30]             - (RW) 1: Enable ADC
                                     0: Disable ADC
    RG_GL1_RG_DA_Mode[31]        - (RW) 1: RG Mode
                                     0: DA Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RG_DA_Mode_ADDR      CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RG_DA_Mode_MASK      0x80000000                // RG_GL1_RG_DA_Mode[31]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RG_DA_Mode_SHFT      31
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EN_ADDR           CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EN_MASK           0x40000000                // RG_GL1_AD_EN[30]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EN_SHFT           30
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GPS_ADCKSEL_ADDR         CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GPS_ADCKSEL_MASK         0x30000000                // RG_GPS_ADCKSEL[29..28]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GPS_ADCKSEL_SHFT         28
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RG_DA_Mode_ADCKSEL_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RG_DA_Mode_ADCKSEL_MASK 0x08000000                // RG_GL1_RG_DA_Mode_ADCKSEL[27]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RG_DA_Mode_ADCKSEL_SHFT 27
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_INTERNAL_ADCKSEL_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_INTERNAL_ADCKSEL_MASK 0x04000000                // RG_GL1_INTERNAL_ADCKSEL[26]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_INTERNAL_ADCKSEL_SHFT 26
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GPS_EXT_CLK_SEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GPS_EXT_CLK_SEL_MASK     0x02000000                // RG_GPS_EXT_CLK_SEL[25]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GPS_EXT_CLK_SEL_SHFT     25
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_CK_CH_RISE_ADDR      CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_CK_CH_RISE_MASK      0x01000000                // RG_GL1_CK_CH_RISE[24]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_CK_CH_RISE_SHFT      24
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VGEN_PUMP_EN_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VGEN_PUMP_EN_MASK 0x00800000                // RG_GL1_AD_VGEN_PUMP_EN[23]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VGEN_PUMP_EN_SHFT 23
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LSB_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LSB_EN_MASK       0x00400000                // RG_GL1_AD_LSB_EN[22]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LSB_EN_SHFT       22
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VAR_CLKS_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VAR_CLKS_MASK     0x00200000                // RG_GL1_AD_VAR_CLKS[21]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VAR_CLKS_SHFT     21
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_ICH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_ICH_EN_MASK       0x00100000                // RG_GL1_AD_ICH_EN[20]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_ICH_EN_SHFT       20
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_QCH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_QCH_EN_MASK       0x00080000                // RG_GL1_AD_QCH_EN[19]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_QCH_EN_SHFT       19
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EXT_CLK_SEL_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EXT_CLK_SEL_MASK  0x00040000                // RG_GL1_AD_EXT_CLK_SEL[18]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EXT_CLK_SEL_SHFT  18
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_BT_GPS_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_BT_GPS_MASK     0x00020000                // RG_GL1_MODE_BT_GPS[17]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_BT_GPS_SHFT     17
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_L1L5_SYNC_CLK_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_L1L5_SYNC_CLK_MASK 0x00010000                // RG_GL1_MODE_L1L5_SYNC_CLK[16]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_L1L5_SYNC_CLK_SHFT 16
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_ADDR    CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MASK    0x00008000                // RG_GL1_AD_COMP_TAIL[15]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_SHFT    15
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MSB_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MSB_MASK 0x00004000                // RG_GL1_AD_COMP_TAIL_MSB[14]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MSB_SHFT 14
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO_PUMP_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO_PUMP_EN_MASK  0x00002000                // RG_GL1_AD_LDO_PUMP_EN[13]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO_PUMP_EN_SHFT  13
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CH_DFF_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CH_DFF_RISE_MASK  0x00001000                // RG_GL1_AD_CH_DFF_RISE[12]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CH_DFF_RISE_SHFT  12
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CK_OUT_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CK_OUT_RISE_MASK  0x00000800                // RG_GL1_AD_CK_OUT_RISE[11]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CK_OUT_RISE_SHFT  11
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_OUTPUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_OUTPUT_EN_MASK    0x00000400                // RG_GL1_AD_OUTPUT_EN[10]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_OUTPUT_EN_SHFT    10
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_OUT_CLK_DIV_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_OUT_CLK_DIV_EN_MASK  0x00000200                // RG_GL1_OUT_CLK_DIV_EN[9]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_OUT_CLK_DIV_EN_SHFT  9
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_OUT_CLK_DIV_H4_L2_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_OUT_CLK_DIV_H4_L2_MASK 0x00000100                // RG_GL1_OUT_CLK_DIV_H4_L2[8]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_OUT_CLK_DIV_H4_L2_SHFT 8
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RAW_OUT_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RAW_OUT_EN_MASK      0x00000080                // RG_GL1_RAW_OUT_EN[7]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RAW_OUT_EN_SHFT      7
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DUODRI_ADDR          CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DUODRI_MASK          0x00000040                // RG_GL1_DUODRI[6]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DUODRI_SHFT          6
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VREF_SEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VREF_SEL_MASK     0x00000038                // RG_GL1_AD_VREF_SEL[5..3]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VREF_SEL_SHFT     3
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO0P8_VSEL_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO0P8_VSEL_MASK  0x00000007                // RG_GL1_AD_LDO0P8_VSEL[2..0]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO0P8_VSEL_SHFT  0

/* =====================================================================================

  ---RG_WBG_GL1_02 (0x18041000 + 0x044)---

    RG_WBG_GL1_02_RSV3[0]        - (RW)  xxx 
    RG_GL1_V098_LDO_AD_VSEL[3..1] - (RW) 0.98V LDO tuning
                                     111: 1.0202
                                     110: 1.0058
                                     101: 0.9976
                                     100: 0.9832  (POR)
                                     011: 0.9726
                                     010: 0.9583
                                     001: 0.9500
                                     000: 0.9357
    RG_WBG_GL1_CKO_INV_EN[4]     - (RW) 1: AD CKO uses decoder CKO
                                     0: AD CKO uses decoder CKO_B
    RG_GPS_DIV_EN[5]             - (RW) GPS_DIV on/off manual control
                                     0: GPS CK DIV manual off
                                     1: GPS CK DIV manual on
    RG_GPS_DIV_EN_MAN[6]         - (RW) GPS CK DIV on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_LDO_GPSDIV_VSEL[8..7]     - (RW) LDO_GPSDIV
                                     00: 0.875V
                                     01: 0.900V(POR)
                                     10: 0.925V
                                     11: 0.950V
    RG_WBG_GL1_02_RSV2[9]        - (RW)  xxx 
    RG_GL1_RX_TST_SEL[10]        - (RW) RX DC test
                                     0: MON_BUF
                                     1: MON_AD
    RG_GL1_RX_TST_EN[11]         - (RW) 1: for RX DC monitor enable
                                     0: disable
    RG_GL1_BUF_TSTSEL[13..12]    - (RW) ADBUF DC test
                                     00: LDO_0P98
                                     01: VICM
                                     10: VCMREF
                                     11: NO_USED
    RG_GL1_BUF_TST_EN[14]        - (RW) 1: for ADBUF DC monitor enable
                                     0: disable
    RG_WBG_GL1_02_RSV1[15]       - (RW)  xxx 
    RG_GL1_BUF_EN[16]            - (RW) ADBUF on/off manual control
                                     0: ADBUF manual off
                                     1: ADBUF manual on
    RG_GL1_BUFEN_MAN[17]         - (RW) ADBUF on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_GL1_V098_LDO_ADBUF_VSEL[20..18] - (RW) 0.98V LDO tuning
                                     111: 1.0202
                                     110: 1.0058
                                     101: 0.9976
                                     100: 0.9832  (POR)
                                     011: 0.9726
                                     010: 0.9583
                                     001: 0.9500
                                     000: 0.9357
    RG_GL1_V125_LDO_VSEL[23..21] - (RW) 1.25V LDO tuning
                                     111: 1.2916
                                     110: 1.2769
                                     101: 1.2686
                                     100: 1.2540 (POR)
                                     011: 1.2437
                                     010: 1.2290
                                     001: 1.2208
                                     000: 1.2061
    RG_GL1_ADBUF_VCM[25..24]     - (RW) ADBUF n-side output Voltage
                                     11: 0.5V
                                     10: 0.55V(POR)
                                     01: 0.6V
                                     00: 0.65V
    RG_GL1_BUF_IGEN_SEL[27..26]  - (RW) RX Top bias current tuning
                                     11:  I5u=4u (0.8X)
                                     10: I5u=5u (1X, POR)
                                     01: I5u=6.68u (1.33X)
                                     00: I5u=10u (2X)
    RG_GL1_AD_TST_EN[28]         - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_GL1_AD_TST_SEL[31..29]    - (RW) AD DC monitored voltage selection
                                     000:VR branch 1 full reference
                                     001: VR branch 1 half reference
                                     010: VR branch 2 full reference
                                     011: VR branch 2  half reference   
                                     100: LDO_0P9V

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_TST_SEL_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_TST_SEL_MASK      0xE0000000                // RG_GL1_AD_TST_SEL[31..29]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_TST_SEL_SHFT      29
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_TST_EN_MASK       0x10000000                // RG_GL1_AD_TST_EN[28]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_TST_EN_SHFT       28
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_IGEN_SEL_ADDR    CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_IGEN_SEL_MASK    0x0C000000                // RG_GL1_BUF_IGEN_SEL[27..26]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_IGEN_SEL_SHFT    26
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_ADBUF_VCM_ADDR       CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_ADBUF_VCM_MASK       0x03000000                // RG_GL1_ADBUF_VCM[25..24]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_ADBUF_VCM_SHFT       24
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V125_LDO_VSEL_ADDR   CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V125_LDO_VSEL_MASK   0x00E00000                // RG_GL1_V125_LDO_VSEL[23..21]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V125_LDO_VSEL_SHFT   21
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V098_LDO_ADBUF_VSEL_ADDR CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V098_LDO_ADBUF_VSEL_MASK 0x001C0000                // RG_GL1_V098_LDO_ADBUF_VSEL[20..18]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V098_LDO_ADBUF_VSEL_SHFT 18
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUFEN_MAN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUFEN_MAN_MASK       0x00020000                // RG_GL1_BUFEN_MAN[17]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUFEN_MAN_SHFT       17
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_EN_ADDR          CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_EN_MASK          0x00010000                // RG_GL1_BUF_EN[16]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_EN_SHFT          16
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV1_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV1_MASK     0x00008000                // RG_WBG_GL1_02_RSV1[15]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV1_SHFT     15
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_TST_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_TST_EN_MASK      0x00004000                // RG_GL1_BUF_TST_EN[14]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_TST_EN_SHFT      14
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_TSTSEL_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_TSTSEL_MASK      0x00003000                // RG_GL1_BUF_TSTSEL[13..12]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_BUF_TSTSEL_SHFT      12
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RX_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RX_TST_EN_MASK       0x00000800                // RG_GL1_RX_TST_EN[11]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RX_TST_EN_SHFT       11
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RX_TST_SEL_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RX_TST_SEL_MASK      0x00000400                // RG_GL1_RX_TST_SEL[10]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RX_TST_SEL_SHFT      10
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV2_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV2_MASK     0x00000200                // RG_WBG_GL1_02_RSV2[9]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV2_SHFT     9
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_LDO_GPSDIV_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_LDO_GPSDIV_VSEL_MASK     0x00000180                // RG_LDO_GPSDIV_VSEL[8..7]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_LDO_GPSDIV_VSEL_SHFT     7
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MAN_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MAN_MASK      0x00000040                // RG_GPS_DIV_EN_MAN[6]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MAN_SHFT      6
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_ADDR          CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MASK          0x00000020                // RG_GPS_DIV_EN[5]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_SHFT          5
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_CKO_INV_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_CKO_INV_EN_MASK  0x00000010                // RG_WBG_GL1_CKO_INV_EN[4]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_CKO_INV_EN_SHFT  4
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V098_LDO_AD_VSEL_ADDR CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V098_LDO_AD_VSEL_MASK 0x0000000E                // RG_GL1_V098_LDO_AD_VSEL[3..1]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_V098_LDO_AD_VSEL_SHFT 1
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV3_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV3_MASK     0x00000001                // RG_WBG_GL1_02_RSV3[0]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_GL1_02_RSV3_SHFT     0

/* =====================================================================================

  ---RG_WBG_BT0_RX_01 (0x18041000 + 0x048)---

    RG_BT_LDO0P8_VSEL[2..0]      - (RW) 0.8V LDO tuning
                                     111:0.860
                                     110: 0.851
                                     101: 0.838
                                     100: 0.828 (POR)
                                     011: 0.815
                                     010: 0.804
                                     001: 0.792
                                     000: 0.782
    RG_BT_AD_VREF_SEL[5..3]      - (RW) VREF selection
                                     111: 0.861
                                     110: 0.850
                                     101: 0.838
                                     100: 0.827
                                     011: 0.814
                                     010: 0.803 (POR)
                                     001: 0.791
                                     000: 0.780
    RG_BT_DUODRI[6]              - (RW) 1: reference gen doubled current
                                     0: reference gen normal current
    RG_BT_RAW_OUT_EN[7]          - (RW) 1: Raw data out
                                     0: Normal data out
    RG_BT_OUT_CLK_DIV_H4_L2[8]   - (RW) 1: output clock divided by 4
                                     0: output clock divided by 2
    RG_BT_OUT_CLK_DIV_EN[9]      - (RW) 1: output clock divider enable 
                                     0: disable
    RG_BT_AD_OUTPUT_EN[10]       - (RW) 1: output enable 
                                     0: output disable
    RG_BT_AD_CK_OUT_RISE[11]     - (RW) 1: Output clock uses positive polarity
                                     0: Output clock uses negative polarity
    RG_BT_AD_CH_DFF_RISE[12]     - (RW) 1: DFF sync clock uses positive polarity
                                     0: DFF sync clock uses negative polarity
    RG_BT_AD_LDO_PUMP_EN[13]     - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_BT_AD_COMP_TAIL_MSB[14]   - (RW) 1: enlarge comparator tail transistor size for first 3 MSBs
                                     0: default size
    RG_BT_AD_COMP_TAIL[15]       - (RW) 1: enlarge comparator tail transistor size
                                     0: default size
    RG_BT_MODE_L1L5_SYNC_CLK[16] - (RW) 1: L1 and L5 dual band use external post-divider clock
                                     0: Internal (Always 0 for BT RX)
    RG_BT_MODE_BT_GPS[17]        - (RW) 1: BT Mode for CLK Selection (Always 1 for BT RX)
                                     0: GPS Mode for CLK Selection
    RG_BT_AD_EXT_CLK_SEL[18]     - (RW) 1: External clock source
                                     0: Internal PLL as clock
    RG_BT_AD_QCH_EN[19]          - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_BT_AD_ICH_EN[20]          - (RW) 1: I channel enable
                                     0: I channel disable
    RG_BT_AD_VAR_CLKS[21]        - (RW) 1: variable clks
                                     0: fixed clks
    RG_BT_AD_LSB_EN[22]          - (RW) 1: 10bit cycles conversion
                                     0: 11 bit cycles conversion
    RG_BT_AD_VGEN_PUMP_EN[23]    - (RW) 1: reference gen pump enable
                                     0: reference gen pump disable
    RG_BT_CK_CH_RISE[24]         - (RW) 1: Decoder output clock uses positive polarity
                                     0: Decoder output clock uses negative polarity
    RG_WBG_CKO_INV_EN[25]        - (RW) 1: AD CKO uses decoder CKO
                                     0: AD CKO uses decoder CKO_B
    RG_WBG_BT_RX_01_RSV1[26]     - (RW)  xxx 
    RG_BT_RG_DA_Mode_ADCKSEL[27] - (RW) 1: RG Mode
                                     0: DA Mode
    RG_BT_ADCKSEL[28]            - (RW) ADC Clock Divider control
                                     0: BTADC normal mode clock=32MHz
                                     1: BTADC normal mode clock=64MHz
    RG_WBG_BT_RX_01_RSV0[29]     - (RW) (n7:RG_BT_AD_REF_EN)
    RG_BT_AD_EN[30]              - (RW) 1: Enable ADC
                                     0: Disable ADC
    RG_BT_RG_DA_Mode_EN[31]      - (RW) 1: RG Mode
                                     0: DA Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RG_DA_Mode_EN_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RG_DA_Mode_EN_MASK 0x80000000                // RG_BT_RG_DA_Mode_EN[31]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RG_DA_Mode_EN_SHFT 31
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_EN_ADDR         CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_EN_MASK         0x40000000                // RG_BT_AD_EN[30]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_EN_SHFT         30
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_BT_RX_01_RSV0_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_BT_RX_01_RSV0_MASK 0x20000000                // RG_WBG_BT_RX_01_RSV0[29]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_BT_RX_01_RSV0_SHFT 29
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_ADCKSEL_ADDR       CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_ADCKSEL_MASK       0x10000000                // RG_BT_ADCKSEL[28]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_ADCKSEL_SHFT       28
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RG_DA_Mode_ADCKSEL_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RG_DA_Mode_ADCKSEL_MASK 0x08000000                // RG_BT_RG_DA_Mode_ADCKSEL[27]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RG_DA_Mode_ADCKSEL_SHFT 27
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_BT_RX_01_RSV1_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_BT_RX_01_RSV1_MASK 0x04000000                // RG_WBG_BT_RX_01_RSV1[26]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_BT_RX_01_RSV1_SHFT 26
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_CKO_INV_EN_ADDR   CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_CKO_INV_EN_MASK   0x02000000                // RG_WBG_CKO_INV_EN[25]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_WBG_CKO_INV_EN_SHFT   25
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_CK_CH_RISE_ADDR    CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_CK_CH_RISE_MASK    0x01000000                // RG_BT_CK_CH_RISE[24]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_CK_CH_RISE_SHFT    24
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VGEN_PUMP_EN_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VGEN_PUMP_EN_MASK 0x00800000                // RG_BT_AD_VGEN_PUMP_EN[23]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VGEN_PUMP_EN_SHFT 23
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_LSB_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_LSB_EN_MASK     0x00400000                // RG_BT_AD_LSB_EN[22]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_LSB_EN_SHFT     22
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VAR_CLKS_ADDR   CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VAR_CLKS_MASK   0x00200000                // RG_BT_AD_VAR_CLKS[21]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VAR_CLKS_SHFT   21
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_ICH_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_ICH_EN_MASK     0x00100000                // RG_BT_AD_ICH_EN[20]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_ICH_EN_SHFT     20
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_QCH_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_QCH_EN_MASK     0x00080000                // RG_BT_AD_QCH_EN[19]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_QCH_EN_SHFT     19
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_EXT_CLK_SEL_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_EXT_CLK_SEL_MASK 0x00040000                // RG_BT_AD_EXT_CLK_SEL[18]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_EXT_CLK_SEL_SHFT 18
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_MODE_BT_GPS_ADDR   CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_MODE_BT_GPS_MASK   0x00020000                // RG_BT_MODE_BT_GPS[17]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_MODE_BT_GPS_SHFT   17
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_MODE_L1L5_SYNC_CLK_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_MODE_L1L5_SYNC_CLK_MASK 0x00010000                // RG_BT_MODE_L1L5_SYNC_CLK[16]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_MODE_L1L5_SYNC_CLK_SHFT 16
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_COMP_TAIL_ADDR  CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_COMP_TAIL_MASK  0x00008000                // RG_BT_AD_COMP_TAIL[15]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_COMP_TAIL_SHFT  15
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_COMP_TAIL_MSB_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_COMP_TAIL_MSB_MASK 0x00004000                // RG_BT_AD_COMP_TAIL_MSB[14]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_COMP_TAIL_MSB_SHFT 14
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_LDO_PUMP_EN_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_LDO_PUMP_EN_MASK 0x00002000                // RG_BT_AD_LDO_PUMP_EN[13]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_LDO_PUMP_EN_SHFT 13
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_CH_DFF_RISE_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_CH_DFF_RISE_MASK 0x00001000                // RG_BT_AD_CH_DFF_RISE[12]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_CH_DFF_RISE_SHFT 12
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_CK_OUT_RISE_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_CK_OUT_RISE_MASK 0x00000800                // RG_BT_AD_CK_OUT_RISE[11]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_CK_OUT_RISE_SHFT 11
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_OUTPUT_EN_ADDR  CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_OUTPUT_EN_MASK  0x00000400                // RG_BT_AD_OUTPUT_EN[10]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_OUTPUT_EN_SHFT  10
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_OUT_CLK_DIV_EN_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_OUT_CLK_DIV_EN_MASK 0x00000200                // RG_BT_OUT_CLK_DIV_EN[9]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_OUT_CLK_DIV_EN_SHFT 9
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_OUT_CLK_DIV_H4_L2_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_OUT_CLK_DIV_H4_L2_MASK 0x00000100                // RG_BT_OUT_CLK_DIV_H4_L2[8]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_OUT_CLK_DIV_H4_L2_SHFT 8
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RAW_OUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RAW_OUT_EN_MASK    0x00000080                // RG_BT_RAW_OUT_EN[7]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_RAW_OUT_EN_SHFT    7
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_DUODRI_ADDR        CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_DUODRI_MASK        0x00000040                // RG_BT_DUODRI[6]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_DUODRI_SHFT        6
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VREF_SEL_ADDR   CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VREF_SEL_MASK   0x00000038                // RG_BT_AD_VREF_SEL[5..3]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_AD_VREF_SEL_SHFT   3
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_LDO0P8_VSEL_ADDR   CONN_AFE_CTL_RG_WBG_BT0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_LDO0P8_VSEL_MASK   0x00000007                // RG_BT_LDO0P8_VSEL[2..0]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_01_RG_BT_LDO0P8_VSEL_SHFT   0

/* =====================================================================================

  ---RG_WBG_BT0_RX_02 (0x18041000 + 0x04C)---

    RG_WBG_BT_RX_02_RSV1[5..0]   - (RW)  xxx 
    RG_BT_V098_LDO_AD_VSEL[8..6] - (RW) 0.98V LDO tuning
                                     111: 1.0202
                                     110: 1.0058
                                     101: 0.9976
                                     100: 0.9832  (POR)
                                     011: 0.9726
                                     010: 0.9583
                                     001: 0.9500
                                     000: 0.9357
    RG_BT_RXDCOC[9]              - (RW) 0: normal 
                                     1: Short input PAD to VCM for RX_DCOC
    RG_BTRX_TST_SEL[10]          - (RW) RX DC test
                                     0: MON_BUF
                                     1: MON_AD
    RG_BTRX_TST_EN[11]           - (RW) 1: for RX DC monitor enable
                                     0: disable
    RG_BT_BUF_TSTSEL[13..12]     - (RW) ADBUF DC test
                                     00: LDO_0P98
                                     01: NO_USED
                                     10: VCMREF
                                     11: NO_USED
    RG_BT_BUF_TST_EN[14]         - (RW) 1: for ADBUF DC monitor enable
                                     0: disable
    RG_BT_AD_IN_SEL[15]          - (RW) 0: Buffer output as ADC input
                                     1: PAD input as ADC input
    RG_BT_BUF_EN[16]             - (RW) ADBUF on/off manual control
                                     0: ADBUF manual off
                                     1: ADBUF manual on
    RG_BT_BUFEN_MAN[17]          - (RW) ADBUF on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_BT_V098_LDO_ADBUF_VSEL[20..18] - (RW) 0.98V LDO tuning
                                     111: 1.0202
                                     110: 1.0058
                                     101: 0.9976
                                     100: 0.9832  (POR)
                                     011: 0.9726
                                     010: 0.9583
                                     001: 0.9500
                                     000: 0.9357
    RG_BT_V125_LDO_VSEL[23..21]  - (RW) 1.25V LDO tuning
                                     111: 1.2916
                                     110: 1.2769
                                     101: 1.2686
                                     100: 1.2540 (POR)
                                     011: 1.2437
                                     010: 1.2290
                                     001: 1.2208
                                     000: 1.2061
    RG_BT_ADBUF_VCM[25..24]      - (RW) ADBUF output VCM
                                     11: 0.5V
                                     10: 0.55V(POR)
                                     01: 0.6V
                                     00: 0.65V
    RG_BT_IGEN_SEL[27..26]       - (RW) RX Top bias current tuning
                                     11:  I5u=4u (0.8X)
                                     10: I5u=5u (1X, POR)
                                     01: I5u=6.68u (1.33X)
                                     00: I5u=10u (2X)
    RG_BT_AD_TST_EN[28]          - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_BT_AD_TST_SEL[31..29]     - (RW) AD DC monitored voltage selection
                                     000:VR1<4>
                                     001:VR1<2>
                                     010:VR1<1>
                                     011:VR2<4>   
                                     100:VR2<2>
                                     101:VR2<1>
                                     110:LDO_0P9V
                                     111:LDO_V098

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_TST_SEL_ADDR    CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_TST_SEL_MASK    0xE0000000                // RG_BT_AD_TST_SEL[31..29]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_TST_SEL_SHFT    29
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_TST_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_TST_EN_MASK     0x10000000                // RG_BT_AD_TST_EN[28]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_TST_EN_SHFT     28
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_IGEN_SEL_ADDR      CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_IGEN_SEL_MASK      0x0C000000                // RG_BT_IGEN_SEL[27..26]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_IGEN_SEL_SHFT      26
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_ADBUF_VCM_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_ADBUF_VCM_MASK     0x03000000                // RG_BT_ADBUF_VCM[25..24]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_ADBUF_VCM_SHFT     24
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V125_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V125_LDO_VSEL_MASK 0x00E00000                // RG_BT_V125_LDO_VSEL[23..21]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V125_LDO_VSEL_SHFT 21
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V098_LDO_ADBUF_VSEL_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V098_LDO_ADBUF_VSEL_MASK 0x001C0000                // RG_BT_V098_LDO_ADBUF_VSEL[20..18]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V098_LDO_ADBUF_VSEL_SHFT 18
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUFEN_MAN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUFEN_MAN_MASK     0x00020000                // RG_BT_BUFEN_MAN[17]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUFEN_MAN_SHFT     17
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_EN_ADDR        CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_EN_MASK        0x00010000                // RG_BT_BUF_EN[16]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_EN_SHFT        16
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_IN_SEL_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_IN_SEL_MASK     0x00008000                // RG_BT_AD_IN_SEL[15]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_AD_IN_SEL_SHFT     15
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_TST_EN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_TST_EN_MASK    0x00004000                // RG_BT_BUF_TST_EN[14]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_TST_EN_SHFT    14
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_TSTSEL_ADDR    CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_TSTSEL_MASK    0x00003000                // RG_BT_BUF_TSTSEL[13..12]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_BUF_TSTSEL_SHFT    12
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BTRX_TST_EN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BTRX_TST_EN_MASK      0x00000800                // RG_BTRX_TST_EN[11]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BTRX_TST_EN_SHFT      11
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BTRX_TST_SEL_ADDR     CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BTRX_TST_SEL_MASK     0x00000400                // RG_BTRX_TST_SEL[10]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BTRX_TST_SEL_SHFT     10
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_RXDCOC_ADDR        CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_RXDCOC_MASK        0x00000200                // RG_BT_RXDCOC[9]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_RXDCOC_SHFT        9
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V098_LDO_AD_VSEL_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V098_LDO_AD_VSEL_MASK 0x000001C0                // RG_BT_V098_LDO_AD_VSEL[8..6]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_BT_V098_LDO_AD_VSEL_SHFT 6
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_WBG_BT_RX_02_RSV1_ADDR CONN_AFE_CTL_RG_WBG_BT0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_WBG_BT_RX_02_RSV1_MASK 0x0000003F                // RG_WBG_BT_RX_02_RSV1[5..0]
#define CONN_AFE_CTL_RG_WBG_BT0_RX_02_RG_WBG_BT_RX_02_RSV1_SHFT 0

/* =====================================================================================

  ---RG_WBG_BT0_TX_01 (0x18041000 + 0x050)---

    RG_B_TX01_RSV[1..0]          - (RW)  xxx 
    RG_B_B0W1_MODE[2]            - (RW) P2S Mode Selection
                                     0:  P2S bypass mode,
                                           I=I0, Q=Q0
                                           BTDA default setting
                                     1:  P2S function mode, 
                                           I=I0->I1->I0->I1, Q=Q0->Q1->Q0->Q1
                                           WFDA default setting
    RG_B_NPSEL[5..3]             - (RW) WBTX DAC calibration point:
                                     000:64
                                     001:128
                                     010:256
                                     011:512
                                     100:1024
                                     101:2048
                                     11x :512
    RG_B_DIN_SIGN_EN[6]          - (RW) WBTX input data format
                                     1: unsinged code
                                     0: 2's compliment code
    RG_B_COR_IQEN[7]             - (RW) WBTX I/Q channel correction code enable:
                                     1: enable
                                     0: disable
    RG_B_ESMAN_QSEL[11..8]       - (RW) Selection for Q Channel calibration value: E0-E15
    RG_B_ESMAN_QEN[12]           - (RW) Manually assign calibration value mode:
                                     0: disable
                                     1: enable
    RG_B_ESMAN_QDATA[19..13]     - (RW) Manually assigned calibration value
    RG_B_ESMAN_ISEL[23..20]      - (RW) Selection for I channel calibration value: E0-E15
    RG_B_ESMAN_IEN[24]           - (RW) Manually assign calibration value mode:
                                     0: disable
                                     1: enable
    RG_B_ESMAN_IDATA[31..25]     - (RW) Manually assigned calibration value

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_IDATA_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_IDATA_MASK    0xFE000000                // RG_B_ESMAN_IDATA[31..25]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_IDATA_SHFT    25
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_IEN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_IEN_MASK      0x01000000                // RG_B_ESMAN_IEN[24]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_IEN_SHFT      24
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_ISEL_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_ISEL_MASK     0x00F00000                // RG_B_ESMAN_ISEL[23..20]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_ISEL_SHFT     20
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QDATA_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QDATA_MASK    0x000FE000                // RG_B_ESMAN_QDATA[19..13]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QDATA_SHFT    13
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QEN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QEN_MASK      0x00001000                // RG_B_ESMAN_QEN[12]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QEN_SHFT      12
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QSEL_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QSEL_MASK     0x00000F00                // RG_B_ESMAN_QSEL[11..8]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_ESMAN_QSEL_SHFT     8
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_COR_IQEN_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_COR_IQEN_MASK       0x00000080                // RG_B_COR_IQEN[7]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_COR_IQEN_SHFT       7
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_DIN_SIGN_EN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_DIN_SIGN_EN_MASK    0x00000040                // RG_B_DIN_SIGN_EN[6]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_DIN_SIGN_EN_SHFT    6
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_NPSEL_ADDR          CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_NPSEL_MASK          0x00000038                // RG_B_NPSEL[5..3]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_NPSEL_SHFT          3
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_B0W1_MODE_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_B0W1_MODE_MASK      0x00000004                // RG_B_B0W1_MODE[2]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_B0W1_MODE_SHFT      2
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_TX01_RSV_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_TX01_RSV_MASK       0x00000003                // RG_B_TX01_RSV[1..0]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_01_RG_B_TX01_RSV_SHFT       0

/* =====================================================================================

  ---RG_WBG_BT0_TX_02 (0x18041000 + 0x054)---

    RG_B_TX02_RSV2[1..0]         - (RW)  xxx 
    RG_B_DATA[11..2]             - (RW) BTTX IQ DAC input manual mode DATA
    RG_B_DATA_MAN[12]            - (RW) BTTX  DAC input manual mode
                                     0: default, TX input from DA_WBG_*_DA*<9:0>
                                     1: Tx input from RG_*_DATA<9:0>
    RG_B_WBG_TX_CKINV_EN[13]     - (RW) NS_CK_BTTX_SYNC phase selection (2nd pipe stage)
                                     0: Original Phase
                                     1: Inverse Phase
    RG_B_TX02_RSV[21..14]        - (RW)  xxx 
    RG_B_DEM_EN[22]              - (RW) MSB DEM enable
                                     0 : disable
                                     1 : enable
    RG_B_DEM_BI_EN[23]           - (RW) Binary DEM enable
                                     0 : disable
                                     1 : enable
    RG_B_DEM_8CAL_EN[24]         - (RW) 8cal DEM enable
                                     0 : disable
                                     1 : enable
    RG_B_DEM_8A_EN[25]           - (RW) 8A DEM enable
                                     0 : disable
                                     1 : enable
    RG_B_DEMCK_CTRL[26]          - (RW) DEM CLK ctrl
                                     0 : 480MHz
                                     1 : 240MHz
    RG_B_FC_EN_DLY[28..27]       - (RW) Delay from DA_WBG_EN_*DA to DA_FC_EN
                                     00:8T
                                     01:16T
                                     10:24T
                                     11:32T
    RG_B_CAL_EN_DLY[30..29]      - (RW) Delay time from RG_WBG_EN_TXCAL_*  to DA_CAL_EN
                                     00:32T 
                                     01:64T
                                     10:96T
                                     11:128T
                                     (1T=1/fxtal)
    RG_B_WBG_EN_SCAN[31]         - (RW) Scan mode DLDO enable control
                                     0 : LP/NORMAL/OLT Mode
                                     1 : SCAN Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_WBG_EN_SCAN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_WBG_EN_SCAN_MASK    0x80000000                // RG_B_WBG_EN_SCAN[31]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_WBG_EN_SCAN_SHFT    31
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_CAL_EN_DLY_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_CAL_EN_DLY_MASK     0x60000000                // RG_B_CAL_EN_DLY[30..29]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_CAL_EN_DLY_SHFT     29
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_FC_EN_DLY_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_FC_EN_DLY_MASK      0x18000000                // RG_B_FC_EN_DLY[28..27]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_FC_EN_DLY_SHFT      27
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEMCK_CTRL_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEMCK_CTRL_MASK     0x04000000                // RG_B_DEMCK_CTRL[26]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEMCK_CTRL_SHFT     26
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_8A_EN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_8A_EN_MASK      0x02000000                // RG_B_DEM_8A_EN[25]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_8A_EN_SHFT      25
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_8CAL_EN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_8CAL_EN_MASK    0x01000000                // RG_B_DEM_8CAL_EN[24]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_8CAL_EN_SHFT    24
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_BI_EN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_BI_EN_MASK      0x00800000                // RG_B_DEM_BI_EN[23]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_BI_EN_SHFT      23
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_EN_ADDR         CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_EN_MASK         0x00400000                // RG_B_DEM_EN[22]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DEM_EN_SHFT         22
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_TX02_RSV_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_TX02_RSV_MASK       0x003FC000                // RG_B_TX02_RSV[21..14]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_TX02_RSV_SHFT       14
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_WBG_TX_CKINV_EN_ADDR CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_WBG_TX_CKINV_EN_MASK 0x00002000                // RG_B_WBG_TX_CKINV_EN[13]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_WBG_TX_CKINV_EN_SHFT 13
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DATA_MAN_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DATA_MAN_MASK       0x00001000                // RG_B_DATA_MAN[12]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DATA_MAN_SHFT       12
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DATA_ADDR           CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DATA_MASK           0x00000FFC                // RG_B_DATA[11..2]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_DATA_SHFT           2
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_TX02_RSV2_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_TX02_RSV2_MASK      0x00000003                // RG_B_TX02_RSV2[1..0]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_02_RG_B_TX02_RSV2_SHFT      0

/* =====================================================================================

  ---RG_WBG_BT0_TX_03 (0x18041000 + 0x058)---

    RG_B_TX03_RSV[4..0]          - (RW)  xxx 
    RG_B_NBIAS_EXTRA_EN[5]       - (RW) Enable extra TIA DC current to create different Vicm/Vocm
                                     0: disable
                                     1: enable
    RG_B_SWPAD[6]                - (RW) TIA bypass switch, used for DAC current measurement
                                     0: disable
                                     1: enable (bypass TIA)
    RG_B_CALC_SEL[8..7]          - (RW) C_integrator sel:
                                     00: open
                                     01: 250fF
                                     10:250fF
                                     11: 500fF
    RG_B_GAIN_CTRL[12..9]        - (RW) BTTX IQ DAC output swing adjustment
                                     <3:2>  for MSB V2I adjustment
                                     <1:0> for LSB V2I current adjustment
                                     00:0%
                                     01:0.65%
                                     10:0.91%
                                     11:1.52%
    RG_B_LPF_BIAS_SEL[14..13]    - (RW) BTTX buffer OP current adjustment:
                                     00: 1.33x
                                     01: 1x
                                     10: 0.8x
                                     11: 0.67x
    RG_B_IREF_SEL[15]            - (RW) BTTX buffer Iref adjustment:
                                     0: WF=40uA
                                     1: BT=10uA
    RG_B_VCM_VGEAR[19..16]       - (RW) BTTX buffer CMFB Vref adjustment: 
                                     (Real Vcm,o=Vcm,ref*(6/5))
                                     0000: 460mV
                                     0001: 470mV
                                     0010: 480mV
                                     0011: 490mV
                                     0100: 500mV
                                     0101: 510mV
                                     0110: 520mV
                                     0111: 530mV 
                                     1000: 400mV
                                     1001: 420mV
                                     1010: 440mV
    RG_B_REF_VGEAR[21..20]       - (RW) BTTX V2I reference voltage
                                     00: 730mV
                                     01: 710mV
                                     10: 700mV
                                     11: 690mV
    RG_B_LDOLV_VGEAR[23..22]     - (RW) BTTX LDOLV voltage adjustment:
                                     00: 0.8V
                                     01: 0.75V
                                     10: 0.9V
                                     11: 0.85V
    RG_B_LDOLPF_VGEAR[25..24]    - (RW) BTTX LDOLPF voltage adjustment:
                                     00: 1.19V
                                     01: 1.26V
                                     10: 1.34V
                                     11: 1.46V
    RG_B_LDODAC_VGEAR[27..26]    - (RW) BTTX LDODAC voltage adjustment:
                                     00: 1.19V
                                     01: 1.26V
                                     10: 1.34V
                                     11: 1.46V
    RG_B_TST_SEL[30..28]         - (RW) BTTX monitor signal selection:
                                     000: VDLDO_07
                                     001: VLDO_DAC_1P25
                                     010: VLDO_LPF_1P25
                                     011: VLDO_IQLAT_08
                                     100: VLDO_CK_08
                                     101: VCM
                                     110: FB_MSB_res
                                     111: FB_LSB_res
    RG_B_TST_EN[31]              - (RW) BTTX dc monitor switch enable
                                     0: disable
                                     1: enable

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TST_EN_ADDR         CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TST_EN_MASK         0x80000000                // RG_B_TST_EN[31]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TST_EN_SHFT         31
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TST_SEL_ADDR        CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TST_SEL_MASK        0x70000000                // RG_B_TST_SEL[30..28]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TST_SEL_SHFT        28
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDODAC_VGEAR_ADDR   CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDODAC_VGEAR_MASK   0x0C000000                // RG_B_LDODAC_VGEAR[27..26]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDODAC_VGEAR_SHFT   26
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDOLPF_VGEAR_ADDR   CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDOLPF_VGEAR_MASK   0x03000000                // RG_B_LDOLPF_VGEAR[25..24]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDOLPF_VGEAR_SHFT   24
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDOLV_VGEAR_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDOLV_VGEAR_MASK    0x00C00000                // RG_B_LDOLV_VGEAR[23..22]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LDOLV_VGEAR_SHFT    22
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_REF_VGEAR_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_REF_VGEAR_MASK      0x00300000                // RG_B_REF_VGEAR[21..20]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_REF_VGEAR_SHFT      20
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_VCM_VGEAR_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_VCM_VGEAR_MASK      0x000F0000                // RG_B_VCM_VGEAR[19..16]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_VCM_VGEAR_SHFT      16
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_IREF_SEL_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_IREF_SEL_MASK       0x00008000                // RG_B_IREF_SEL[15]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_IREF_SEL_SHFT       15
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LPF_BIAS_SEL_ADDR   CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LPF_BIAS_SEL_MASK   0x00006000                // RG_B_LPF_BIAS_SEL[14..13]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_LPF_BIAS_SEL_SHFT   13
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_GAIN_CTRL_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_GAIN_CTRL_MASK      0x00001E00                // RG_B_GAIN_CTRL[12..9]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_GAIN_CTRL_SHFT      9
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_CALC_SEL_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_CALC_SEL_MASK       0x00000180                // RG_B_CALC_SEL[8..7]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_CALC_SEL_SHFT       7
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_SWPAD_ADDR          CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_SWPAD_MASK          0x00000040                // RG_B_SWPAD[6]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_SWPAD_SHFT          6
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_NBIAS_EXTRA_EN_ADDR CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_NBIAS_EXTRA_EN_MASK 0x00000020                // RG_B_NBIAS_EXTRA_EN[5]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_NBIAS_EXTRA_EN_SHFT 5
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TX03_RSV_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TX03_RSV_MASK       0x0000001F                // RG_B_TX03_RSV[4..0]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_B_TX03_RSV_SHFT       0

/* =====================================================================================

  ---RG_WBG_BT0_TX_04 (0x18041000 + 0x05C)---

    RG_B_CAL_CK_EN[0]            - (RW) 0: CAL clk is controlled by CAL_EN
                                     1: manual enable CAL CLK for Es readout debug
    RG_B_CAL_MAN_SEL[1]          - (RW) BTTX manual-mode control signal selection
                                     0: default
                                     1: RG_CAL_MAN_EN15 selected
    RG_B_CAL_MAN_EN[2]           - (RW) BTTX manually control signal
                                     DA_CAL_EN15
    RG_B_MAN_SEL[3]              - (RW) BTTX manual-mode control signal selection
                                     0: default
                                     1: RG_MAN value selected
    RG_B_MAN_CLKIQ_EN[4]         - (RW) BTTX manually control signal
                                     DA_CLKIQ_EN15
    RG_B_MAN_DAC_EN[5]           - (RW) BTTX manually control signal
                                     DA_DAC_EN15
    RG_B_MAN_LPF_EN[6]           - (RW) BTTX manually control signal
                                     DA_LPF_IQ_EN15
    RG_B_MAN_ALDO_EN[7]          - (RW) BTTX manually control signal
                                     DA_ALDO_EN15
    RG_B_INNERMODE_EN[8]         - (RW) 1: Enable 8A/8B SDM feedback loop from analog comparator 
                                     0: 8A/8B controlled by digital macro
    RG_B_WF480CLK_EN[9]          - (RW) no function in BTTX mode
    RG_B_CKREV_EN[10]            - (RW) Reverse clk phase of clk receiver
    RG_B_CLK_CAL_DIV[12..11]     - (RW) BTTX calibration clock rate:
                                     00: 0
                                     01: 832M/4
                                     10: 832M/8
                                     11: 832M/16
    RG_B_EXTCLK_EN[13]           - (RW) BBTX  external clock ENABLE:
                                     0: disable
                                     1: enable
    RG_B_CKAC_EN[14]             - (RW) CK RCV AC couple enable:
                                     1: enable ac couple
                                     0: disable ac couple use dc path ( test mode only)
    RG_B_QREV_Q[15]              - (RW) Invert the ploarity of quantizer output in Q channel
                                     0: non-inverted
                                     1: inverted
    RG_B_QREV_I[16]              - (RW) Invert the ploarity of quantizer output in I channel
                                     0: non-inverted
                                     1: inverted
    RG_B_TX04_RSV[31..17]        - (RW)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_TX04_RSV_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_TX04_RSV_MASK       0xFFFE0000                // RG_B_TX04_RSV[31..17]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_TX04_RSV_SHFT       17
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_QREV_I_ADDR         CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_QREV_I_MASK         0x00010000                // RG_B_QREV_I[16]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_QREV_I_SHFT         16
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_QREV_Q_ADDR         CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_QREV_Q_MASK         0x00008000                // RG_B_QREV_Q[15]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_QREV_Q_SHFT         15
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CKAC_EN_ADDR        CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CKAC_EN_MASK        0x00004000                // RG_B_CKAC_EN[14]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CKAC_EN_SHFT        14
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_EXTCLK_EN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_EXTCLK_EN_MASK      0x00002000                // RG_B_EXTCLK_EN[13]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_EXTCLK_EN_SHFT      13
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CLK_CAL_DIV_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CLK_CAL_DIV_MASK    0x00001800                // RG_B_CLK_CAL_DIV[12..11]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CLK_CAL_DIV_SHFT    11
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CKREV_EN_ADDR       CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CKREV_EN_MASK       0x00000400                // RG_B_CKREV_EN[10]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CKREV_EN_SHFT       10
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_WF480CLK_EN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_WF480CLK_EN_MASK    0x00000200                // RG_B_WF480CLK_EN[9]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_WF480CLK_EN_SHFT    9
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_INNERMODE_EN_ADDR   CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_INNERMODE_EN_MASK   0x00000100                // RG_B_INNERMODE_EN[8]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_INNERMODE_EN_SHFT   8
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_ALDO_EN_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_ALDO_EN_MASK    0x00000080                // RG_B_MAN_ALDO_EN[7]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_ALDO_EN_SHFT    7
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_LPF_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_LPF_EN_MASK     0x00000040                // RG_B_MAN_LPF_EN[6]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_LPF_EN_SHFT     6
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_DAC_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_DAC_EN_MASK     0x00000020                // RG_B_MAN_DAC_EN[5]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_DAC_EN_SHFT     5
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_CLKIQ_EN_ADDR   CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_CLKIQ_EN_MASK   0x00000010                // RG_B_MAN_CLKIQ_EN[4]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_CLKIQ_EN_SHFT   4
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_SEL_ADDR        CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_SEL_MASK        0x00000008                // RG_B_MAN_SEL[3]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_MAN_SEL_SHFT        3
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_MAN_EN_ADDR     CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_MAN_EN_MASK     0x00000004                // RG_B_CAL_MAN_EN[2]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_MAN_EN_SHFT     2
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_MAN_SEL_ADDR    CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_MAN_SEL_MASK    0x00000002                // RG_B_CAL_MAN_SEL[1]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_MAN_SEL_SHFT    1
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_CK_EN_ADDR      CONN_AFE_CTL_RG_WBG_BT0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_CK_EN_MASK      0x00000001                // RG_B_CAL_CK_EN[0]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_04_RG_B_CAL_CK_EN_SHFT      0

/* =====================================================================================

  ---RGS_WBG_BT0_TXCAL (0x18041000 + 0x060)---

    RGS_B_ES_READOUT_I_RO[11..0] - (RO) BTTX I channel MSB error sorce readout bit
    RGS_B_ES_OVERFLOW_I_RO[12]   - (RO) BTTX I channel overflow flag
    RGS_B_CAL_DONE_I_RO[13]      - (RO) BTTX I channel calibraton done flag
    RGS_B_DMY2_RO[15..14]        - (RO)  xxx 
    RGS_B_ES_READOUT_Q_RO[27..16] - (RO) BTTX Q channel MSB error sorce readout bit
    RGS_B_ES_OVERFLOW_Q_RO[28]   - (RO) BTTX Q channel overflow flag
    RGS_B_CAL_DONE_Q_RO[29]      - (RO) BTTX Q channel calibraton done flag
    RGS_B_DMY1_RO[31..30]        - (RO)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_DMY1_RO_ADDR      CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_DMY1_RO_MASK      0xC0000000                // RGS_B_DMY1_RO[31..30]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_DMY1_RO_SHFT      30
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_CAL_DONE_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_CAL_DONE_Q_RO_MASK 0x20000000                // RGS_B_CAL_DONE_Q_RO[29]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_CAL_DONE_Q_RO_SHFT 29
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_OVERFLOW_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_OVERFLOW_Q_RO_MASK 0x10000000                // RGS_B_ES_OVERFLOW_Q_RO[28]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_OVERFLOW_Q_RO_SHFT 28
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_READOUT_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_READOUT_Q_RO_MASK 0x0FFF0000                // RGS_B_ES_READOUT_Q_RO[27..16]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_READOUT_Q_RO_SHFT 16
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_DMY2_RO_ADDR      CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_DMY2_RO_MASK      0x0000C000                // RGS_B_DMY2_RO[15..14]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_DMY2_RO_SHFT      14
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_CAL_DONE_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_CAL_DONE_I_RO_MASK 0x00002000                // RGS_B_CAL_DONE_I_RO[13]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_CAL_DONE_I_RO_SHFT 13
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_OVERFLOW_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_OVERFLOW_I_RO_MASK 0x00001000                // RGS_B_ES_OVERFLOW_I_RO[12]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_OVERFLOW_I_RO_SHFT 12
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_READOUT_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_READOUT_I_RO_MASK 0x00000FFF                // RGS_B_ES_READOUT_I_RO[11..0]
#define CONN_AFE_CTL_RGS_WBG_BT0_TXCAL_RGS_B_ES_READOUT_I_RO_SHFT 0

/* =====================================================================================

  ---RG_WBG_WF0_RX_01 (0x18041000 + 0x068)---

    RG_WF0_CKO_INV_EN[0]         - (RW) RXOUT_DFF
                                     1: Output clock uses positive edge to receive data
                                     0: Output clock uses negative edge to receive data
    RG_WF0_AD_TST_EN[1]          - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_WF0_CKO_ADCFS_EN[2]       - (RW) 0: CKO is the fastest clk
                                     1: CKO is ADC sampling clk
    RG_WF0_OUTCLK_DIV_SEL[4..3]  - (RW) 11: output clock divided by 2
                                     10: output clock divided by 4
                                     01: output clock divided by 8
                                     00: Not define
    RG_WF0_AD_MSBQ[7..5]         - (RW) ADCQ MSB Bit Weighting
                                     111: +3
                                     110: +2
                                     101: +1
                                     100: +0 (POR,496)
                                     011: -1
                                     010: -2
                                     001: -3
                                     000: -4
    RG_WF0_AD_MSBI[10..8]        - (RW) ADCI MSB Bit Weighting
                                     111: +3
                                     110: +2
                                     101: +1
                                     100: +0 (POR,496)
                                     011: -1
                                     010: -2
                                     001: -3
                                     000: -4
    RG_WF0_AD_CK_OUT_RISE[11]    - (RW) DEC
                                     1: Output clock uses positive edge to receive data
                                     0: Output clock uses negative edge to receive data
    RG_WF0_RAWOUT_EN[12]         - (RW) 1: Raw data out
                                     0: Normal data out
    RG_WF0_OUT_CLK_DIV_EN[13]    - (RW) 1: Output clock divider enable 
                                     0: Disable
    RG_WF0_ADOUT_EN[14]          - (RW) DEC
                                     1:enable DATA out
                                     0: disable DATA out
    RG_WF0_BUFF_DYM_BIAS[15]     - (RW) 0: RG control->normal @ 640MS/s and 320MS/s 
                                     1: RG control->dynamic bias for buffer @ 160 MS/s
    RG_WF0_QCH_EN[16]            - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_WF0_ICH_EN[17]            - (RW) 1: I channel enable
                                     0: I channel disable
    RG_WF0_LSB_EN[18]            - (RW) 1: ADC LSB enable (12cycles)
                                     0: ADC LSB disable (11cycles)
    RG_WF0_AD_EN_09_MAN[19]      - (RW) 1: manual control by RG
                                     0: normal control by DA
    RG_WF0_AD_VAR_CLKS[20]       - (RW) 1: Variable clks
                                     0: Fixed clks
    RG_WF0_LDO_PUMP_EN[21]       - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_WF0_CK_FIX_EN[22]         - (RW) 1: all mode fix CK=640M
                                     0: disable
    RG_WF0_TI_EN[23]             - (RW) 1: RG control->Enable time interleaving
                                     0: RG control->Disable time interleaving
    RG_WF0_EXTCK_EN[24]          - (RW) 1: External clock 
                                     0: Intermal clock
    RG_WF0_LDO_V09_SEL[27..25]   - (RW) 111:0.9187
                                     110:0.9018
                                     101:0.8831
                                     100:0.8664
                                     011:0.8456
                                     010:0.8288
                                     001:0.8102
                                     000:0.7934
    RG_WF0_DUODRI[28]            - (RW) 1: Reference gen doubled current (600uA)
                                     0: Reference gen normal current (300uA)
    RG_WF0_ADVREF_SEL[31..29]    - (RW) 111:0.8986
                                     110:0.8846
                                     101:0.8678
                                     100:0.8539
                                     011:0.8339
                                     010:0.82
                                     001:0.8034
                                     000:0.7896

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ADVREF_SEL_ADDR   CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ADVREF_SEL_MASK   0xE0000000                // RG_WF0_ADVREF_SEL[31..29]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ADVREF_SEL_SHFT   29
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_DUODRI_ADDR       CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_DUODRI_MASK       0x10000000                // RG_WF0_DUODRI[28]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_DUODRI_SHFT       28
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LDO_V09_SEL_ADDR  CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LDO_V09_SEL_MASK  0x0E000000                // RG_WF0_LDO_V09_SEL[27..25]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LDO_V09_SEL_SHFT  25
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_EXTCK_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_EXTCK_EN_MASK     0x01000000                // RG_WF0_EXTCK_EN[24]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_EXTCK_EN_SHFT     24
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_TI_EN_ADDR        CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_TI_EN_MASK        0x00800000                // RG_WF0_TI_EN[23]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_TI_EN_SHFT        23
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CK_FIX_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CK_FIX_EN_MASK    0x00400000                // RG_WF0_CK_FIX_EN[22]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CK_FIX_EN_SHFT    22
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LDO_PUMP_EN_ADDR  CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LDO_PUMP_EN_MASK  0x00200000                // RG_WF0_LDO_PUMP_EN[21]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LDO_PUMP_EN_SHFT  21
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_VAR_CLKS_ADDR  CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_VAR_CLKS_MASK  0x00100000                // RG_WF0_AD_VAR_CLKS[20]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_VAR_CLKS_SHFT  20
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_EN_09_MAN_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_EN_09_MAN_MASK 0x00080000                // RG_WF0_AD_EN_09_MAN[19]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_EN_09_MAN_SHFT 19
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LSB_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LSB_EN_MASK       0x00040000                // RG_WF0_LSB_EN[18]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_LSB_EN_SHFT       18
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ICH_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ICH_EN_MASK       0x00020000                // RG_WF0_ICH_EN[17]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ICH_EN_SHFT       17
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_QCH_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_QCH_EN_MASK       0x00010000                // RG_WF0_QCH_EN[16]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_QCH_EN_SHFT       16
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_BUFF_DYM_BIAS_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_BUFF_DYM_BIAS_MASK 0x00008000                // RG_WF0_BUFF_DYM_BIAS[15]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_BUFF_DYM_BIAS_SHFT 15
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ADOUT_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ADOUT_EN_MASK     0x00004000                // RG_WF0_ADOUT_EN[14]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_ADOUT_EN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_OUT_CLK_DIV_EN_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_OUT_CLK_DIV_EN_MASK 0x00002000                // RG_WF0_OUT_CLK_DIV_EN[13]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_OUT_CLK_DIV_EN_SHFT 13
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_RAWOUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_RAWOUT_EN_MASK    0x00001000                // RG_WF0_RAWOUT_EN[12]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_RAWOUT_EN_SHFT    12
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_CK_OUT_RISE_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_CK_OUT_RISE_MASK 0x00000800                // RG_WF0_AD_CK_OUT_RISE[11]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_CK_OUT_RISE_SHFT 11
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_MSBI_ADDR      CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_MSBI_MASK      0x00000700                // RG_WF0_AD_MSBI[10..8]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_MSBI_SHFT      8
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_MSBQ_ADDR      CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_MSBQ_MASK      0x000000E0                // RG_WF0_AD_MSBQ[7..5]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_MSBQ_SHFT      5
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_OUTCLK_DIV_SEL_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_OUTCLK_DIV_SEL_MASK 0x00000018                // RG_WF0_OUTCLK_DIV_SEL[4..3]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_OUTCLK_DIV_SEL_SHFT 3
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CKO_ADCFS_EN_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CKO_ADCFS_EN_MASK 0x00000004                // RG_WF0_CKO_ADCFS_EN[2]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CKO_ADCFS_EN_SHFT 2
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_TST_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_TST_EN_MASK    0x00000002                // RG_WF0_AD_TST_EN[1]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_AD_TST_EN_SHFT    1
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CKO_INV_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CKO_INV_EN_MASK   0x00000001                // RG_WF0_CKO_INV_EN[0]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_01_RG_WF0_CKO_INV_EN_SHFT   0

/* =====================================================================================

  ---RG_WBG_WF0_RX_02 (0x18041000 + 0x06C)---

    RG_WF0_AD_EN_09[0]           - (RW) 1:enable AD_09
                                     0:disable AD_09
    RG_WF0_RX_TSTSEL[3..1]       - (RW) RX DC monitored voltage selection
                                     111:none
                                     110:none
                                     101:none
                                     100:V086_C2_Q
                                     011:V098_1_Q
                                     010:V098_2_I
                                     001:AD MUX out
                                     000:BUF MUX out
    RG_WF0_RX_TST_EN[4]          - (RW) 1: RX DC monitor enable 
                                     0: RX DC monitor disable
    RG_WF0_AD_V098_LDO_VSEL[7..5] - (RW) 0.98V LDO tuning
                                     111:1.015 
                                     110:1.005 
                                     101:0.9925 
                                     100:0.9821 
                                     011:0.9675 
                                     010:0.9572 
                                     001:0.9449 
                                     000:0.9346
    RG_WF0_BUF_BWSEL[8]          - (RW) 0: 640M
                                     1: 320M
    RG_WF0_BUF_BWSEL_MODE[9]     - (RW) 0: DA control
                                     1: RX_02<8> control
    RG_WF0_BUF_DYN_BIAS_MODE[10] - (RW) 0: DA control
                                     1: RX_01<15> control
    RG_WF0_TI_EN_MODE[11]        - (RW) 0: DA control
                                     1: RX_01<23> control
    RG_WF0_BUF_TSTSEL[13..12]    - (RW) ADBUF DC test
                                     00: LDO_0P98
                                     01: LDO_1P55
                                     10: VCMREF
                                     11: NO_USED
    RG_WF0_BUF_TST_EN[14]        - (RW) 1: for ADBUF DC monitor enable
                                     0: disable
    RG_WF0_AD_IN_SEL[15]         - (RW) 0: Buffer output as ADC input
                                     1: PAD input as ADC input
    RG_WF0_BUF_EN[16]            - (RW) ADBUF on/off manual control
                                     0: ADBUF manual off
                                     1: ADBUF manual on
    RG_WF0_BUFEN_MAN[17]         - (RW) ADBUF on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_WF0_V125_LDO_VSEL[20..18] - (RW) 1.55V LDO tuning
                                     111: 1.63
                                     110: 1.6
                                     101: 1.58
                                     100: 1.55 (POR)
                                     011: 1.53
                                     010: 1.5 
                                     001: 1.48
                                     000: 1.45
    RG_WF0_V098_LDO_VSEL[23..21] - (RW) 0.98V LDO tuning
                                     111: 1.04
                                     110: 1.02
                                     101: 0.999
                                     100: 0.98  (POR)
                                     011: 0.958
                                     010: 0.941
                                     001: 0.92
                                     000: 0.9
    RG_WF0_ADBUF_VCM[25..24]     - (RW) ADBUF output VCM
                                     11: 0.5V
                                     10: 0.55V(POR)
                                     01: 0.6V
                                     00: 0.65V
    RG_WF0_IGEN_SEL[27..26]      - (RW) RX Top bias current tuning
                                     11:  I5u=4.06u (0.8X)
                                     10: I5u=5.07u (1X, POR)
                                     01: I5u=6.75u (1.33X)
                                     00: I5u=10.1u (2X)
    RG_WF0_AD_TST_SEL[31..28]    - (RW) AD DC monitored voltage selection
                                     1111:V086_CK
                                     1110:V086_F_Q
                                     1101:V086_C1_Q
                                     1100:V086_F_I
                                     1011:V086_C2_I
                                     1010:V086_C1_I
                                     1001:V098_2_Q
                                     1000:V098_1_I
                                     0111:VR2_Q<2>
                                     0110:VR1_Q<2>
                                     0101:VR2_I<0>
                                     0100:VR2_I<1>
                                     0011:VR2_I<2>
                                     0010:VR1_I<0>
                                     0001:VR1_I<1>
                                     0000:VR1_I<2>

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_TST_SEL_ADDR   CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_TST_SEL_MASK   0xF0000000                // RG_WF0_AD_TST_SEL[31..28]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_TST_SEL_SHFT   28
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_IGEN_SEL_ADDR     CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_IGEN_SEL_MASK     0x0C000000                // RG_WF0_IGEN_SEL[27..26]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_IGEN_SEL_SHFT     26
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_ADBUF_VCM_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_ADBUF_VCM_MASK    0x03000000                // RG_WF0_ADBUF_VCM[25..24]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_ADBUF_VCM_SHFT    24
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_V098_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_V098_LDO_VSEL_MASK 0x00E00000                // RG_WF0_V098_LDO_VSEL[23..21]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_V098_LDO_VSEL_SHFT 21
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_V125_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_V125_LDO_VSEL_MASK 0x001C0000                // RG_WF0_V125_LDO_VSEL[20..18]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_V125_LDO_VSEL_SHFT 18
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUFEN_MAN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUFEN_MAN_MASK    0x00020000                // RG_WF0_BUFEN_MAN[17]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUFEN_MAN_SHFT    17
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_EN_MASK       0x00010000                // RG_WF0_BUF_EN[16]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_EN_SHFT       16
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_IN_SEL_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_IN_SEL_MASK    0x00008000                // RG_WF0_AD_IN_SEL[15]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_IN_SEL_SHFT    15
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_TST_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_TST_EN_MASK   0x00004000                // RG_WF0_BUF_TST_EN[14]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_TST_EN_SHFT   14
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_TSTSEL_ADDR   CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_TSTSEL_MASK   0x00003000                // RG_WF0_BUF_TSTSEL[13..12]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_TSTSEL_SHFT   12
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_TI_EN_MODE_ADDR   CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_TI_EN_MODE_MASK   0x00000800                // RG_WF0_TI_EN_MODE[11]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_TI_EN_MODE_SHFT   11
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_DYN_BIAS_MODE_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_DYN_BIAS_MODE_MASK 0x00000400                // RG_WF0_BUF_DYN_BIAS_MODE[10]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_DYN_BIAS_MODE_SHFT 10
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_BWSEL_MODE_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_BWSEL_MODE_MASK 0x00000200                // RG_WF0_BUF_BWSEL_MODE[9]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_BWSEL_MODE_SHFT 9
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_BWSEL_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_BWSEL_MASK    0x00000100                // RG_WF0_BUF_BWSEL[8]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_BUF_BWSEL_SHFT    8
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_V098_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_V098_LDO_VSEL_MASK 0x000000E0                // RG_WF0_AD_V098_LDO_VSEL[7..5]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_V098_LDO_VSEL_SHFT 5
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_RX_TST_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_RX_TST_EN_MASK    0x00000010                // RG_WF0_RX_TST_EN[4]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_RX_TST_EN_SHFT    4
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_RX_TSTSEL_ADDR    CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_RX_TSTSEL_MASK    0x0000000E                // RG_WF0_RX_TSTSEL[3..1]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_RX_TSTSEL_SHFT    1
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_EN_09_ADDR     CONN_AFE_CTL_RG_WBG_WF0_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_EN_09_MASK     0x00000001                // RG_WF0_AD_EN_09[0]
#define CONN_AFE_CTL_RG_WBG_WF0_RX_02_RG_WF0_AD_EN_09_SHFT     0

/* =====================================================================================

  ---RG_WBG_WF0_TX_01 (0x18041000 + 0x070)---

    RG_W0_TX01_RSV[1..0]         - (RW)  xxx 
    RG_W0_B0W1_MODE[2]           - (RW) P2S Mode Selection
                                     0:  P2S bypass mode,
                                           I=I0, Q=Q0
                                           BTDA default setting
                                     1:  P2S function mode, 
                                           I=I0->I1->I0->I1, Q=Q0->Q1->Q0->Q1
                                           WFDA default setting
    RG_W0_NPSEL[5..3]            - (RW) WBTX DAC calibration point:
                                     000:64
                                     001:128
                                     010:256
                                     011:512
                                     100:1024
                                     101:2048
                                     11x :512
    RG_W0_DIN_SIGN_EN[6]         - (RW) WBTX input data format
                                     1: unsinged code
                                     0: 2's compliment code
    RG_W0_COR_IQEN[7]            - (RW) WBTX I/Q channel correction code enable:
                                     1: enable
                                     0: disable
    RG_W0_ESMAN_QSEL[11..8]      - (RW) Selection for Q Channel calibration value: E0-E15
    RG_W0_ESMAN_QEN[12]          - (RW) Manually assign calibration value mode:
                                     0: disable
                                     1: enable
    RG_W0_ESMAN_QDATA[19..13]    - (RW) Manually assigned calibration value
    RG_W0_ESMAN_ISEL[23..20]     - (RW) Selection for I channel calibration value: E0-E15
    RG_W0_ESMAN_IEN[24]          - (RW) Manually assign calibration value mode:
                                     0: disable
                                     1: enable
    RG_W0_ESMAN_IDATA[31..25]    - (RW) Manually assigned calibration value

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_IDATA_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_IDATA_MASK   0xFE000000                // RG_W0_ESMAN_IDATA[31..25]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_IDATA_SHFT   25
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_IEN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_IEN_MASK     0x01000000                // RG_W0_ESMAN_IEN[24]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_IEN_SHFT     24
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_ISEL_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_ISEL_MASK    0x00F00000                // RG_W0_ESMAN_ISEL[23..20]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_ISEL_SHFT    20
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QDATA_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QDATA_MASK   0x000FE000                // RG_W0_ESMAN_QDATA[19..13]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QDATA_SHFT   13
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QEN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QEN_MASK     0x00001000                // RG_W0_ESMAN_QEN[12]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QEN_SHFT     12
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QSEL_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QSEL_MASK    0x00000F00                // RG_W0_ESMAN_QSEL[11..8]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_ESMAN_QSEL_SHFT    8
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_COR_IQEN_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_COR_IQEN_MASK      0x00000080                // RG_W0_COR_IQEN[7]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_COR_IQEN_SHFT      7
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_DIN_SIGN_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_DIN_SIGN_EN_MASK   0x00000040                // RG_W0_DIN_SIGN_EN[6]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_DIN_SIGN_EN_SHFT   6
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_NPSEL_ADDR         CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_NPSEL_MASK         0x00000038                // RG_W0_NPSEL[5..3]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_NPSEL_SHFT         3
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_B0W1_MODE_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_B0W1_MODE_MASK     0x00000004                // RG_W0_B0W1_MODE[2]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_B0W1_MODE_SHFT     2
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_TX01_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_TX01_RSV_MASK      0x00000003                // RG_W0_TX01_RSV[1..0]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_01_RG_W0_TX01_RSV_SHFT      0

/* =====================================================================================

  ---RG_WBG_WF0_TX_02 (0x18041000 + 0x074)---

    RG_W0_DATA[11..0]            - (RW) WFTX IQ DAC input manual mode DATA
    RG_W0_DATA_MAN[12]           - (RW) WFTX  DAC input manual mode
                                     0: default, TX input from DA_WBG_WF*_DA*<11:0>
                                     1: Tx input from RG_W*_DATA<11:0>
    RG_W0_WBG_TX_CKINV_EN[13]    - (RW) NS_CK_WFTX_SYNC phase selection (2nd pipe stage)
                                     0: Original Phase
                                     1: Inverse Phase
    RG_W0_TX02_RSV[21..14]       - (RW)  xxx 
    RG_W0_DEM_EN[22]             - (RW) MSB DEM enable
                                     0 : disable
                                     1 : enable
    RG_W0_DEM_BI_EN[23]          - (RW) Binary DEM enable
                                     0 : disable
                                     1 : enable
    RG_W0_DEM_8CAL_EN[24]        - (RW) 8cal DEM enable
                                     0 : disable
                                     1 : enable
    RG_W0_DEM_8A_EN[25]          - (RW) 8A DEM enable
                                     0 : disable
                                     1 : enable
    RG_W0_DEMCK_CTRL[26]         - (RW) DEM CLK ctrl
                                     0 : 480MHz
                                     1 : 240MHz
    RG_W0_FC_EN_DLY[28..27]      - (RW) Delay from DA_WBG_EN_WF*DA to DA_FC_EN
                                     00:8T
                                     01:16T
                                     10:24T
                                     11:32T
    RG_W0_CAL_EN_DLY[30..29]     - (RW) Delay time from RG_WBG_EN_TXCAL_WF*  to DA_CAL_EN
                                     00:32T 
                                     01:64T
                                     10:96T
                                     11:128T
                                     (1T=1/fxtal)
    RG_W0_WBG_EN_SCAN[31]        - (RW) Scan mode DLDO enable control
                                     0 : LP/NORMAL/OLT Mode
                                     1 : SCAN Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_WBG_EN_SCAN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_WBG_EN_SCAN_MASK   0x80000000                // RG_W0_WBG_EN_SCAN[31]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_WBG_EN_SCAN_SHFT   31
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_CAL_EN_DLY_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_CAL_EN_DLY_MASK    0x60000000                // RG_W0_CAL_EN_DLY[30..29]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_CAL_EN_DLY_SHFT    29
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_FC_EN_DLY_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_FC_EN_DLY_MASK     0x18000000                // RG_W0_FC_EN_DLY[28..27]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_FC_EN_DLY_SHFT     27
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEMCK_CTRL_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEMCK_CTRL_MASK    0x04000000                // RG_W0_DEMCK_CTRL[26]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEMCK_CTRL_SHFT    26
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_8A_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_8A_EN_MASK     0x02000000                // RG_W0_DEM_8A_EN[25]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_8A_EN_SHFT     25
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_8CAL_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_8CAL_EN_MASK   0x01000000                // RG_W0_DEM_8CAL_EN[24]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_8CAL_EN_SHFT   24
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_BI_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_BI_EN_MASK     0x00800000                // RG_W0_DEM_BI_EN[23]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_BI_EN_SHFT     23
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_EN_ADDR        CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_EN_MASK        0x00400000                // RG_W0_DEM_EN[22]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DEM_EN_SHFT        22
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_TX02_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_TX02_RSV_MASK      0x003FC000                // RG_W0_TX02_RSV[21..14]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_TX02_RSV_SHFT      14
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_WBG_TX_CKINV_EN_ADDR CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_WBG_TX_CKINV_EN_MASK 0x00002000                // RG_W0_WBG_TX_CKINV_EN[13]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_WBG_TX_CKINV_EN_SHFT 13
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DATA_MAN_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DATA_MAN_MASK      0x00001000                // RG_W0_DATA_MAN[12]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DATA_MAN_SHFT      12
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DATA_ADDR          CONN_AFE_CTL_RG_WBG_WF0_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DATA_MASK          0x00000FFF                // RG_W0_DATA[11..0]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_02_RG_W0_DATA_SHFT          0

/* =====================================================================================

  ---RG_WBG_WF0_TX_03 (0x18041000 + 0x078)---

    RG_W0_TX03_RSV[4..0]         - (RW)  xxx 
    RG_W0_NBIAS_EXTRA_EN[5]      - (RW) Enable extra TIA DC current to create different Vicm/Vocm
                                     0: disable
                                     1: enable
    RG_W0_SWPAD[6]               - (RW) TIA bypass switch, used for DAC current measurement
                                     0: disable
                                     1: enable (bypass TIA)
    RG_W0_CALC_SEL[8..7]         - (RW) C_integrator sel:
                                     00: open
                                     01: 250fF
                                     10:250fF
                                     11: 500fF
    RG_W0_GAIN_CTRL[12..9]       - (RW) WFTX IQ DAC output swing adjustment
                                     <3:2>  for MSB V2I adjustment
                                     <1:0> for LSB V2I current adjustment
                                     00:0%
                                     01:0.65%
                                     10:0.91%
                                     11:1.52%
    RG_W0_LPF_BIAS_SEL[14..13]   - (RW) WFTX buffer OP current adjustment:
                                     00: 1.33x
                                     01: 1x
                                     10: 0.8x
                                     11: 0.67x
    RG_W0_IREF_SEL[15]           - (RW) WFTX buffer Iref adjustment:
                                     0: WF=40uA
                                     1: BT=10uA
    RG_W0_VCM_VGEAR[19..16]      - (RW) WFTX buffer CMFB Vref adjustment: 
                                     (Real Vcm,o=Vcm,ref*(6/5))
                                     0000: 460mV
                                     0001: 470mV
                                     0010: 480mV
                                     0011: 490mV
                                     0100: 500mV
                                     0101: 510mV
                                     0110: 520mV
                                     0111: 530mV 
                                     1000: 400mV
                                     1001: 420mV
                                     1010: 440mV
    RG_W0_REF_VGEAR[21..20]      - (RW) WFTX V2I reference voltage
                                     00: 730mV
                                     01: 710mV
                                     10: 700mV
                                     11: 690mV
    RG_W0_LDOLV_VGEAR[23..22]    - (RW) WFTX LDOLV voltage adjustment:
                                     00: 0.8V
                                     01: 0.75V
                                     10: 0.9V
                                     11: 0.85V
    RG_W0_LDOLPF_VGEAR[25..24]   - (RW) WFTX LDOLPF voltage adjustment:
                                     00: 1.19V
                                     01: 1.26V
                                     10: 1.34V
                                     11: 1.46V
    RG_W0_LDODAC_VGEAR[27..26]   - (RW) WFTX LDODAC voltage adjustment:
                                     00: 1.19V
                                     01: 1.26V
                                     10: 1.34V
                                     11: 1.46V
    RG_W0_TST_SEL[30..28]        - (RW) WFTX monitor signal selection:
                                     000: VDLDO_07
                                     001: VLDO_DAC_1P25
                                     010: VLDO_LPF_1P25
                                     011: VLDO_IQLAT_08
                                     100: VLDO_CK_08
                                     101: VCM
                                     110: FB_MSB_res
                                     111: FB_LSB_res
    RG_W0_TST_EN[31]             - (RW) WFTX dc monitor switch enable
                                     0: disable
                                     1: enable

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TST_EN_ADDR        CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TST_EN_MASK        0x80000000                // RG_W0_TST_EN[31]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TST_EN_SHFT        31
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TST_SEL_ADDR       CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TST_SEL_MASK       0x70000000                // RG_W0_TST_SEL[30..28]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TST_SEL_SHFT       28
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDODAC_VGEAR_ADDR  CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDODAC_VGEAR_MASK  0x0C000000                // RG_W0_LDODAC_VGEAR[27..26]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDODAC_VGEAR_SHFT  26
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDOLPF_VGEAR_ADDR  CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDOLPF_VGEAR_MASK  0x03000000                // RG_W0_LDOLPF_VGEAR[25..24]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDOLPF_VGEAR_SHFT  24
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDOLV_VGEAR_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDOLV_VGEAR_MASK   0x00C00000                // RG_W0_LDOLV_VGEAR[23..22]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LDOLV_VGEAR_SHFT   22
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_REF_VGEAR_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_REF_VGEAR_MASK     0x00300000                // RG_W0_REF_VGEAR[21..20]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_REF_VGEAR_SHFT     20
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_VCM_VGEAR_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_VCM_VGEAR_MASK     0x000F0000                // RG_W0_VCM_VGEAR[19..16]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_VCM_VGEAR_SHFT     16
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_IREF_SEL_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_IREF_SEL_MASK      0x00008000                // RG_W0_IREF_SEL[15]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_IREF_SEL_SHFT      15
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LPF_BIAS_SEL_ADDR  CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LPF_BIAS_SEL_MASK  0x00006000                // RG_W0_LPF_BIAS_SEL[14..13]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_LPF_BIAS_SEL_SHFT  13
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_GAIN_CTRL_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_GAIN_CTRL_MASK     0x00001E00                // RG_W0_GAIN_CTRL[12..9]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_GAIN_CTRL_SHFT     9
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_CALC_SEL_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_CALC_SEL_MASK      0x00000180                // RG_W0_CALC_SEL[8..7]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_CALC_SEL_SHFT      7
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_SWPAD_ADDR         CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_SWPAD_MASK         0x00000040                // RG_W0_SWPAD[6]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_SWPAD_SHFT         6
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_NBIAS_EXTRA_EN_ADDR CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_NBIAS_EXTRA_EN_MASK 0x00000020                // RG_W0_NBIAS_EXTRA_EN[5]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_NBIAS_EXTRA_EN_SHFT 5
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TX03_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TX03_RSV_MASK      0x0000001F                // RG_W0_TX03_RSV[4..0]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_TX03_RSV_SHFT      0

/* =====================================================================================

  ---RG_WBG_WF0_TX_04 (0x18041000 + 0x07C)---

    RG_W0_CAL_CK_EN[0]           - (RW) 0: CAL clk is controlled by CAL_EN
                                     1: manual enable CAL CLK for Es readout debug
    RG_W0_CAL_MAN_SEL[1]         - (RW) WFTX manual-mode control signal selection
                                     0: default
                                     1: RG_CAL_MAN_EN15 selected
    RG_W0_CAL_MAN_EN[2]          - (RW) WFTX manually control signal
                                     DA_CAL_EN15
    RG_W0_MAN_SEL[3]             - (RW) WFTX manual-mode control signal selection
                                     0: default
                                     1: RG_MAN value selected
    RG_W0_MAN_CLKIQ_EN[4]        - (RW) WFTX manually control signal
                                     DA_CLKIQ_EN15
    RG_W0_MAN_DAC_EN[5]          - (RW) WFTX manually control signal
                                     DA_DAC_EN15
    RG_W0_MAN_LPF_EN[6]          - (RW) WFTX manually control signal
                                     DA_LPF_IQ_EN15
    RG_W0_MAN_ALDO_EN[7]         - (RW) WFTX manually control signal
                                     DA_ALDO_EN15
    RG_W0_INNERMODE_EN[8]        - (RW) 1: Enable 8A/8B SDM feedback loop from analog comparator 
                                     0: 8A/8B controlled by digital macro
    RG_W0_WF480CLK_EN[9]         - (RW) WBG 480 CLK for 2x2 4x4 enable
                                     0: 480MHz clk disable   
                                     1: 480MHz clk enable
    RG_W0_CKREV_EN[10]           - (RW) Reverse clk phase of clk receiver
    RG_W0_CLK_CAL_DIV[12..11]    - (RW) WFTX calibration clock rate:
                                     00: 0
                                     01: 960M/4
                                     10: 960M/8
                                     11: 960M/16
    RG_W0_EXTCLK_EN[13]          - (RW) BBTX  external clock ENABLE:
                                     0: disable
                                     1: enable
    RG_W0_CKAC_EN[14]            - (RW) CK RCV AC couple enable:
                                     1: enable ac couple
                                     0: disable ac couple use dc path ( test mode only)
    RG_W0_QREV_Q[15]             - (RW) Invert the ploarity of quantizer output in Q channel
                                     0: non-inverted
                                     1: inverted
    RG_W0_QREV_I[16]             - (RW) Invert the ploarity of quantizer output in I channel
                                     0: non-inverted
                                     1: inverted
    RG_W0_TX04_RSV[31..17]       - (RW)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_TX04_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_TX04_RSV_MASK      0xFFFE0000                // RG_W0_TX04_RSV[31..17]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_TX04_RSV_SHFT      17
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_QREV_I_ADDR        CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_QREV_I_MASK        0x00010000                // RG_W0_QREV_I[16]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_QREV_I_SHFT        16
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_QREV_Q_ADDR        CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_QREV_Q_MASK        0x00008000                // RG_W0_QREV_Q[15]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_QREV_Q_SHFT        15
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CKAC_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CKAC_EN_MASK       0x00004000                // RG_W0_CKAC_EN[14]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CKAC_EN_SHFT       14
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_EXTCLK_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_EXTCLK_EN_MASK     0x00002000                // RG_W0_EXTCLK_EN[13]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_EXTCLK_EN_SHFT     13
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CLK_CAL_DIV_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CLK_CAL_DIV_MASK   0x00001800                // RG_W0_CLK_CAL_DIV[12..11]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CLK_CAL_DIV_SHFT   11
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CKREV_EN_ADDR      CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CKREV_EN_MASK      0x00000400                // RG_W0_CKREV_EN[10]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CKREV_EN_SHFT      10
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_WF480CLK_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_WF480CLK_EN_MASK   0x00000200                // RG_W0_WF480CLK_EN[9]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_WF480CLK_EN_SHFT   9
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_INNERMODE_EN_ADDR  CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_INNERMODE_EN_MASK  0x00000100                // RG_W0_INNERMODE_EN[8]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_INNERMODE_EN_SHFT  8
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_ALDO_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_ALDO_EN_MASK   0x00000080                // RG_W0_MAN_ALDO_EN[7]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_ALDO_EN_SHFT   7
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_LPF_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_LPF_EN_MASK    0x00000040                // RG_W0_MAN_LPF_EN[6]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_LPF_EN_SHFT    6
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_DAC_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_DAC_EN_MASK    0x00000020                // RG_W0_MAN_DAC_EN[5]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_DAC_EN_SHFT    5
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_CLKIQ_EN_ADDR  CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_CLKIQ_EN_MASK  0x00000010                // RG_W0_MAN_CLKIQ_EN[4]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_CLKIQ_EN_SHFT  4
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_SEL_ADDR       CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_SEL_MASK       0x00000008                // RG_W0_MAN_SEL[3]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_MAN_SEL_SHFT       3
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_MAN_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_MAN_EN_MASK    0x00000004                // RG_W0_CAL_MAN_EN[2]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_MAN_EN_SHFT    2
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_MAN_SEL_ADDR   CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_MAN_SEL_MASK   0x00000002                // RG_W0_CAL_MAN_SEL[1]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_MAN_SEL_SHFT   1
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_CK_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF0_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_CK_EN_MASK     0x00000001                // RG_W0_CAL_CK_EN[0]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_04_RG_W0_CAL_CK_EN_SHFT     0

/* =====================================================================================

  ---RGS_WBG_WF0_TXCAL (0x18041000 + 0x080)---

    RGS_W0_ES_READOUT_I_RO[11..0] - (RO) WFTX I channel MSB error sorce readout bit
    RGS_W0_ES_OVERFLOW_I_RO[12]  - (RO) WFTX I channel overflow flag
    RGS_W0_CAL_DONE_I_RO[13]     - (RO) WFTX I channel calibraton done flag
    RGS_W0_DMY2_RO[15..14]       - (RO)  xxx 
    RGS_W0_ES_READOUT_Q_RO[27..16] - (RO) WFTX Q channel MSB error sorce readout bit
    RGS_W0_ES_OVERFLOW_Q_RO[28]  - (RO) WFTX Q channel overflow flag
    RGS_W0_CAL_DONE_Q_RO[29]     - (RO) WFTX Q channel calibraton done flag
    RGS_W0_DMY1_RO[31..30]       - (RO)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_DMY1_RO_ADDR     CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_DMY1_RO_MASK     0xC0000000                // RGS_W0_DMY1_RO[31..30]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_DMY1_RO_SHFT     30
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_CAL_DONE_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_CAL_DONE_Q_RO_MASK 0x20000000                // RGS_W0_CAL_DONE_Q_RO[29]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_CAL_DONE_Q_RO_SHFT 29
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_OVERFLOW_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_OVERFLOW_Q_RO_MASK 0x10000000                // RGS_W0_ES_OVERFLOW_Q_RO[28]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_OVERFLOW_Q_RO_SHFT 28
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_READOUT_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_READOUT_Q_RO_MASK 0x0FFF0000                // RGS_W0_ES_READOUT_Q_RO[27..16]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_READOUT_Q_RO_SHFT 16
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_DMY2_RO_ADDR     CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_DMY2_RO_MASK     0x0000C000                // RGS_W0_DMY2_RO[15..14]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_DMY2_RO_SHFT     14
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_CAL_DONE_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_CAL_DONE_I_RO_MASK 0x00002000                // RGS_W0_CAL_DONE_I_RO[13]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_CAL_DONE_I_RO_SHFT 13
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_OVERFLOW_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_OVERFLOW_I_RO_MASK 0x00001000                // RGS_W0_ES_OVERFLOW_I_RO[12]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_OVERFLOW_I_RO_SHFT 12
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_READOUT_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_READOUT_I_RO_MASK 0x00000FFF                // RGS_W0_ES_READOUT_I_RO[11..0]
#define CONN_AFE_CTL_RGS_WBG_WF0_TXCAL_RGS_W0_ES_READOUT_I_RO_SHFT 0

/* =====================================================================================

  ---RG_WBG_WF1_RX_01 (0x18041000 + 0x084)---

    RG_WF1_CKO_INV_EN[0]         - (RW) RXOUT_DFF
                                     1: Output clock uses positive edge to receive data
                                     0: Output clock uses negative edge to receive data
    RG_WF1_AD_TST_EN[1]          - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_WF1_CKO_ADCFS_EN[2]       - (RW) 0: CKO is the fastest clk
                                     1: CKO is ADC sampling clk
    RG_WF1_OUTCLK_DIV_SEL[4..3]  - (RW) 11: output clock divided by 2
                                     10: output clock divided by 4
                                     01: output clock divided by 8
                                     00: Not define
    RG_WF1_AD_MSBQ[7..5]         - (RW) ADCQ MSB Bit Weighting
                                     111: +3
                                     110: +2
                                     101: +1
                                     100: +0 (POR,496)
                                     011: -1
                                     010: -2
                                     001: -3
                                     000: -4
    RG_WF1_AD_MSBI[10..8]        - (RW) ADCI MSB Bit Weighting
                                     111: +3
                                     110: +2
                                     101: +1
                                     100: +0 (POR,496)
                                     011: -1
                                     010: -2
                                     001: -3
                                     000: -4
    RG_WF1_AD_CK_OUT_RISE[11]    - (RW) DEC
                                     1: Output clock uses positive edge to receive data
                                     0: Output clock uses negative edge to receive data
    RG_WF1_RAWOUT_EN[12]         - (RW) 1: Raw data out
                                     0: Normal data out
    RG_WF1_OUT_CLK_DIV_EN[13]    - (RW) 1: Output clock divider enable 
                                     0: Disable
    RG_WF1_ADOUT_EN[14]          - (RW) DEC
                                     1:enable DATA out
                                     0: disable DATA out
    RG_WF1_BUFF_DYM_BIAS[15]     - (RW) 0: RG control->normal @ 640MS/s and 320MS/s 
                                     1: RG control->dynamic bias for buffer @ 160 MS/s
    RG_WF1_QCH_EN[16]            - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_WF1_ICH_EN[17]            - (RW) 1: I channel enable
                                     0: I channel disable
    RG_WF1_LSB_EN[18]            - (RW) 1: ADC LSB enable (12cycles)
                                     0: ADC LSB disable (11cycles)
    RG_WF1_AD_EN_09_MAN[19]      - (RW) 1: manual control by RG
                                     0: normal control by DA
    RG_WF1_AD_VAR_CLKS[20]       - (RW) 1: Variable clks
                                     0: Fixed clks
    RG_WF1_LDO_PUMP_EN[21]       - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_WF1_CK_FIX_EN[22]         - (RW) 1: all mode fix CK=640M
                                     0: disable
    RG_WF1_TI_EN[23]             - (RW) 1: RG control->Enable time interleaving
                                     0: RG control->Disable time interleaving
    RG_WF1_EXTCK_EN[24]          - (RW) 1: External clock 
                                     0: Intermal clock
    RG_WF1_LDO_V09_SEL[27..25]   - (RW) 111:0.9187
                                     110:0.9018
                                     101:0.8831
                                     100:0.8664
                                     011:0.8456
                                     010:0.8288
                                     001:0.8102
                                     000:0.7934
    RG_WF1_DUODRI[28]            - (RW) 1: Reference gen doubled current (600uA)
                                     0: Reference gen normal current (300uA)
    RG_WF1_ADVREF_SEL[31..29]    - (RW) 111:0.8986
                                     110:0.8846
                                     101:0.8678
                                     100:0.8539
                                     011:0.8339
                                     010:0.82
                                     001:0.8034
                                     000:0.7896

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ADVREF_SEL_ADDR   CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ADVREF_SEL_MASK   0xE0000000                // RG_WF1_ADVREF_SEL[31..29]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ADVREF_SEL_SHFT   29
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_DUODRI_ADDR       CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_DUODRI_MASK       0x10000000                // RG_WF1_DUODRI[28]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_DUODRI_SHFT       28
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LDO_V09_SEL_ADDR  CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LDO_V09_SEL_MASK  0x0E000000                // RG_WF1_LDO_V09_SEL[27..25]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LDO_V09_SEL_SHFT  25
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_EXTCK_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_EXTCK_EN_MASK     0x01000000                // RG_WF1_EXTCK_EN[24]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_EXTCK_EN_SHFT     24
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_TI_EN_ADDR        CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_TI_EN_MASK        0x00800000                // RG_WF1_TI_EN[23]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_TI_EN_SHFT        23
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CK_FIX_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CK_FIX_EN_MASK    0x00400000                // RG_WF1_CK_FIX_EN[22]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CK_FIX_EN_SHFT    22
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LDO_PUMP_EN_ADDR  CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LDO_PUMP_EN_MASK  0x00200000                // RG_WF1_LDO_PUMP_EN[21]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LDO_PUMP_EN_SHFT  21
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_VAR_CLKS_ADDR  CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_VAR_CLKS_MASK  0x00100000                // RG_WF1_AD_VAR_CLKS[20]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_VAR_CLKS_SHFT  20
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_EN_09_MAN_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_EN_09_MAN_MASK 0x00080000                // RG_WF1_AD_EN_09_MAN[19]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_EN_09_MAN_SHFT 19
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LSB_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LSB_EN_MASK       0x00040000                // RG_WF1_LSB_EN[18]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_LSB_EN_SHFT       18
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ICH_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ICH_EN_MASK       0x00020000                // RG_WF1_ICH_EN[17]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ICH_EN_SHFT       17
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_QCH_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_QCH_EN_MASK       0x00010000                // RG_WF1_QCH_EN[16]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_QCH_EN_SHFT       16
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_BUFF_DYM_BIAS_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_BUFF_DYM_BIAS_MASK 0x00008000                // RG_WF1_BUFF_DYM_BIAS[15]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_BUFF_DYM_BIAS_SHFT 15
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ADOUT_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ADOUT_EN_MASK     0x00004000                // RG_WF1_ADOUT_EN[14]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_ADOUT_EN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_OUT_CLK_DIV_EN_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_OUT_CLK_DIV_EN_MASK 0x00002000                // RG_WF1_OUT_CLK_DIV_EN[13]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_OUT_CLK_DIV_EN_SHFT 13
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_RAWOUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_RAWOUT_EN_MASK    0x00001000                // RG_WF1_RAWOUT_EN[12]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_RAWOUT_EN_SHFT    12
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_CK_OUT_RISE_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_CK_OUT_RISE_MASK 0x00000800                // RG_WF1_AD_CK_OUT_RISE[11]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_CK_OUT_RISE_SHFT 11
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_MSBI_ADDR      CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_MSBI_MASK      0x00000700                // RG_WF1_AD_MSBI[10..8]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_MSBI_SHFT      8
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_MSBQ_ADDR      CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_MSBQ_MASK      0x000000E0                // RG_WF1_AD_MSBQ[7..5]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_MSBQ_SHFT      5
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_OUTCLK_DIV_SEL_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_OUTCLK_DIV_SEL_MASK 0x00000018                // RG_WF1_OUTCLK_DIV_SEL[4..3]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_OUTCLK_DIV_SEL_SHFT 3
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CKO_ADCFS_EN_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CKO_ADCFS_EN_MASK 0x00000004                // RG_WF1_CKO_ADCFS_EN[2]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CKO_ADCFS_EN_SHFT 2
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_TST_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_TST_EN_MASK    0x00000002                // RG_WF1_AD_TST_EN[1]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_AD_TST_EN_SHFT    1
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CKO_INV_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CKO_INV_EN_MASK   0x00000001                // RG_WF1_CKO_INV_EN[0]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_01_RG_WF1_CKO_INV_EN_SHFT   0

/* =====================================================================================

  ---RG_WBG_WF1_RX_02 (0x18041000 + 0x088)---

    RG_WF1_AD_EN_09[0]           - (RW) 1:enable AD_09
                                     0:disable AD_09
    RG_WF1_RX_TSTSEL[3..1]       - (RW) RX DC monitored voltage selection
                                     111:none
                                     110:none
                                     101:none
                                     100:V086_C2_Q
                                     011:V098_1_Q
                                     010:V098_2_I
                                     001:AD MUX out
                                     000:BUF MUX out
    RG_WF1_RX_TST_EN[4]          - (RW) 1: RX DC monitor enable 
                                     0: RX DC monitor disable
    RG_WF1_AD_V098_LDO_VSEL[7..5] - (RW) 0.98V LDO tuning
                                     111:1.015 
                                     110:1.005 
                                     101:0.9925 
                                     100:0.9821 
                                     011:0.9675 
                                     010:0.9572 
                                     001:0.9449 
                                     000:0.9346
    RG_WF1_BUF_BWSEL[8]          - (RW) 0: 640M
                                     1: 320M
    RG_WF1_BUF_BWSEL_MODE[9]     - (RW) 0: DA control
                                     1: RX_02<8> control
    RG_WF1_BUF_DYN_BIAS_MODE[10] - (RW) 0: DA control
                                     1: RX_01<15> control
    RG_WF1_TI_EN_MODE[11]        - (RW) 0: DA control
                                     1: RX_01<23> control
    RG_WF1_BUF_TSTSEL[13..12]    - (RW) ADBUF DC test
                                     00: LDO_0P98
                                     01: LDO_1P55
                                     10: VCMREF
                                     11: NO_USED
    RG_WF1_BUF_TST_EN[14]        - (RW) 1: for ADBUF DC monitor enable
                                     0: disable
    RG_WF1_AD_IN_SEL[15]         - (RW) 0: Buffer output as ADC input
                                     1: PAD input as ADC input
    RG_WF1_BUF_EN[16]            - (RW) ADBUF on/off manual control
                                     0: ADBUF manual off
                                     1: ADBUF manual on
    RG_WF1_BUFEN_MAN[17]         - (RW) ADBUF on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_WF1_V125_LDO_VSEL[20..18] - (RW) 1.55V LDO tuning
                                     111: 1.63
                                     110: 1.6
                                     101: 1.58
                                     100: 1.55 (POR)
                                     011: 1.53
                                     010: 1.5 
                                     001: 1.48
                                     000: 1.45
    RG_WF1_V098_LDO_VSEL[23..21] - (RW) 0.98V LDO tuning
                                     111: 1.04
                                     110: 1.02
                                     101: 0.999
                                     100: 0.98  (POR)
                                     011: 0.958
                                     010: 0.941
                                     001: 0.92
                                     000: 0.9
    RG_WF1_ADBUF_VCM[25..24]     - (RW) ADBUF output VCM
                                     11: 0.5V
                                     10: 0.55V(POR)
                                     01: 0.6V
                                     00: 0.65V
    RG_WF1_IGEN_SEL[27..26]      - (RW) RX Top bias current tuning
                                     11:  I5u=4.06u (0.8X)
                                     10: I5u=5.07u (1X, POR)
                                     01: I5u=6.75u (1.33X)
                                     00: I5u=10.1u (2X)
    RG_WF1_AD_TST_SEL[31..28]    - (RW) AD DC monitored voltage selection
                                     1111:V086_CK
                                     1110:V086_F_Q
                                     1101:V086_C1_Q
                                     1100:V086_F_I
                                     1011:V086_C2_I
                                     1010:V086_C1_I
                                     1001:V098_2_Q
                                     1000:V098_1_I
                                     0111:VR2_Q<2>
                                     0110:VR1_Q<2>
                                     0101:VR2_I<0>
                                     0100:VR2_I<1>
                                     0011:VR2_I<2>
                                     0010:VR1_I<0>
                                     0001:VR1_I<1>
                                     0000:VR1_I<2>

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_TST_SEL_ADDR   CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_TST_SEL_MASK   0xF0000000                // RG_WF1_AD_TST_SEL[31..28]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_TST_SEL_SHFT   28
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_IGEN_SEL_ADDR     CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_IGEN_SEL_MASK     0x0C000000                // RG_WF1_IGEN_SEL[27..26]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_IGEN_SEL_SHFT     26
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_ADBUF_VCM_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_ADBUF_VCM_MASK    0x03000000                // RG_WF1_ADBUF_VCM[25..24]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_ADBUF_VCM_SHFT    24
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_V098_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_V098_LDO_VSEL_MASK 0x00E00000                // RG_WF1_V098_LDO_VSEL[23..21]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_V098_LDO_VSEL_SHFT 21
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_V125_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_V125_LDO_VSEL_MASK 0x001C0000                // RG_WF1_V125_LDO_VSEL[20..18]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_V125_LDO_VSEL_SHFT 18
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUFEN_MAN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUFEN_MAN_MASK    0x00020000                // RG_WF1_BUFEN_MAN[17]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUFEN_MAN_SHFT    17
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_EN_MASK       0x00010000                // RG_WF1_BUF_EN[16]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_EN_SHFT       16
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_IN_SEL_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_IN_SEL_MASK    0x00008000                // RG_WF1_AD_IN_SEL[15]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_IN_SEL_SHFT    15
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_TST_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_TST_EN_MASK   0x00004000                // RG_WF1_BUF_TST_EN[14]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_TST_EN_SHFT   14
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_TSTSEL_ADDR   CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_TSTSEL_MASK   0x00003000                // RG_WF1_BUF_TSTSEL[13..12]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_TSTSEL_SHFT   12
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_TI_EN_MODE_ADDR   CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_TI_EN_MODE_MASK   0x00000800                // RG_WF1_TI_EN_MODE[11]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_TI_EN_MODE_SHFT   11
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_DYN_BIAS_MODE_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_DYN_BIAS_MODE_MASK 0x00000400                // RG_WF1_BUF_DYN_BIAS_MODE[10]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_DYN_BIAS_MODE_SHFT 10
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_BWSEL_MODE_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_BWSEL_MODE_MASK 0x00000200                // RG_WF1_BUF_BWSEL_MODE[9]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_BWSEL_MODE_SHFT 9
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_BWSEL_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_BWSEL_MASK    0x00000100                // RG_WF1_BUF_BWSEL[8]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_BUF_BWSEL_SHFT    8
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_V098_LDO_VSEL_ADDR CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_V098_LDO_VSEL_MASK 0x000000E0                // RG_WF1_AD_V098_LDO_VSEL[7..5]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_V098_LDO_VSEL_SHFT 5
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_RX_TST_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_RX_TST_EN_MASK    0x00000010                // RG_WF1_RX_TST_EN[4]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_RX_TST_EN_SHFT    4
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_RX_TSTSEL_ADDR    CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_RX_TSTSEL_MASK    0x0000000E                // RG_WF1_RX_TSTSEL[3..1]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_RX_TSTSEL_SHFT    1
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_EN_09_ADDR     CONN_AFE_CTL_RG_WBG_WF1_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_EN_09_MASK     0x00000001                // RG_WF1_AD_EN_09[0]
#define CONN_AFE_CTL_RG_WBG_WF1_RX_02_RG_WF1_AD_EN_09_SHFT     0

/* =====================================================================================

  ---RG_WBG_WF1_TX_01 (0x18041000 + 0x08C)---

    RG_W1_TX01_RSV[1..0]         - (RW)  xxx 
    RG_W1_B0W1_MODE[2]           - (RW) P2S Mode Selection
                                     0:  P2S bypass mode,
                                           I=I0, Q=Q0
                                           BTDA default setting
                                     1:  P2S function mode, 
                                           I=I0->I1->I0->I1, Q=Q0->Q1->Q0->Q1
                                           WFDA default setting
    RG_W1_NPSEL[5..3]            - (RW) WBTX DAC calibration point:
                                     000:64
                                     001:128
                                     010:256
                                     011:512
                                     100:1024
                                     101:2048
                                     11x :512
    RG_W1_DIN_SIGN_EN[6]         - (RW) WBTX input data format
                                     1: unsinged code
                                     0: 2's compliment code
    RG_W1_COR_IQEN[7]            - (RW) WBTX I/Q channel correction code enable:
                                     1: enable
                                     0: disable
    RG_W1_ESMAN_QSEL[11..8]      - (RW) Selection for Q Channel calibration value: E0-E15
    RG_W1_ESMAN_QEN[12]          - (RW) Manually assign calibration value mode:
                                     0: disable
                                     1: enable
    RG_W1_ESMAN_QDATA[19..13]    - (RW) Manually assigned calibration value
    RG_W1_ESMAN_ISEL[23..20]     - (RW) Selection for I channel calibration value: E0-E15
    RG_W1_ESMAN_IEN[24]          - (RW) Manually assign calibration value mode:
                                     0: disable
                                     1: enable
    RG_W1_ESMAN_IDATA[31..25]    - (RW) Manually assigned calibration value

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_IDATA_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_IDATA_MASK   0xFE000000                // RG_W1_ESMAN_IDATA[31..25]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_IDATA_SHFT   25
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_IEN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_IEN_MASK     0x01000000                // RG_W1_ESMAN_IEN[24]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_IEN_SHFT     24
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_ISEL_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_ISEL_MASK    0x00F00000                // RG_W1_ESMAN_ISEL[23..20]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_ISEL_SHFT    20
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QDATA_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QDATA_MASK   0x000FE000                // RG_W1_ESMAN_QDATA[19..13]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QDATA_SHFT   13
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QEN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QEN_MASK     0x00001000                // RG_W1_ESMAN_QEN[12]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QEN_SHFT     12
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QSEL_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QSEL_MASK    0x00000F00                // RG_W1_ESMAN_QSEL[11..8]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_ESMAN_QSEL_SHFT    8
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_COR_IQEN_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_COR_IQEN_MASK      0x00000080                // RG_W1_COR_IQEN[7]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_COR_IQEN_SHFT      7
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_DIN_SIGN_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_DIN_SIGN_EN_MASK   0x00000040                // RG_W1_DIN_SIGN_EN[6]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_DIN_SIGN_EN_SHFT   6
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_NPSEL_ADDR         CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_NPSEL_MASK         0x00000038                // RG_W1_NPSEL[5..3]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_NPSEL_SHFT         3
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_B0W1_MODE_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_B0W1_MODE_MASK     0x00000004                // RG_W1_B0W1_MODE[2]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_B0W1_MODE_SHFT     2
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_TX01_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_TX01_RSV_MASK      0x00000003                // RG_W1_TX01_RSV[1..0]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_01_RG_W1_TX01_RSV_SHFT      0

/* =====================================================================================

  ---RG_WBG_WF1_TX_02 (0x18041000 + 0x090)---

    RG_W1_DATA[11..0]            - (RW) WFTX IQ DAC input manual mode DATA
    RG_W1_DATA_MAN[12]           - (RW) WFTX  DAC input manual mode
                                     0: default, TX input from DA_WBG_WF*_DA*<11:0>
                                     1: Tx input from RG_W*_DATA<11:0>
    RG_W1_WBG_TX_CKINV_EN[13]    - (RW) NS_CK_WFTX_SYNC phase selection (2nd pipe stage)
                                     0: Original Phase
                                     1: Inverse Phase
    RG_W1_TX02_RSV[21..14]       - (RW)  xxx 
    RG_W1_DEM_EN[22]             - (RW) MSB DEM enable
                                     0 : disable
                                     1 : enable
    RG_W1_DEM_BI_EN[23]          - (RW) Binary DEM enable
                                     0 : disable
                                     1 : enable
    RG_W1_DEM_8CAL_EN[24]        - (RW) 8cal DEM enable
                                     0 : disable
                                     1 : enable
    RG_W1_DEM_8A_EN[25]          - (RW) 8A DEM enable
                                     0 : disable
                                     1 : enable
    RG_W1_DEMCK_CTRL[26]         - (RW) DEM CLK ctrl
                                     0 : 480MHz
                                     1 : 240MHz
    RG_W1_FC_EN_DLY[28..27]      - (RW) Delay from DA_WBG_EN_WF*DA to DA_FC_EN
                                     00:8T
                                     01:16T
                                     10:24T
                                     11:32T
    RG_W1_CAL_EN_DLY[30..29]     - (RW) Delay time from RG_WBG_EN_TXCAL_WF*  to DA_CAL_EN
                                     00:32T 
                                     01:64T
                                     10:96T
                                     11:128T
                                     (1T=1/fxtal)
    RG_W1_WBG_EN_SCAN[31]        - (RW) Scan mode DLDO enable control
                                     0 : LP/NORMAL/OLT Mode
                                     1 : SCAN Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_WBG_EN_SCAN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_WBG_EN_SCAN_MASK   0x80000000                // RG_W1_WBG_EN_SCAN[31]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_WBG_EN_SCAN_SHFT   31
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_CAL_EN_DLY_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_CAL_EN_DLY_MASK    0x60000000                // RG_W1_CAL_EN_DLY[30..29]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_CAL_EN_DLY_SHFT    29
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_FC_EN_DLY_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_FC_EN_DLY_MASK     0x18000000                // RG_W1_FC_EN_DLY[28..27]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_FC_EN_DLY_SHFT     27
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEMCK_CTRL_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEMCK_CTRL_MASK    0x04000000                // RG_W1_DEMCK_CTRL[26]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEMCK_CTRL_SHFT    26
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_8A_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_8A_EN_MASK     0x02000000                // RG_W1_DEM_8A_EN[25]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_8A_EN_SHFT     25
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_8CAL_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_8CAL_EN_MASK   0x01000000                // RG_W1_DEM_8CAL_EN[24]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_8CAL_EN_SHFT   24
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_BI_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_BI_EN_MASK     0x00800000                // RG_W1_DEM_BI_EN[23]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_BI_EN_SHFT     23
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_EN_ADDR        CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_EN_MASK        0x00400000                // RG_W1_DEM_EN[22]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DEM_EN_SHFT        22
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_TX02_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_TX02_RSV_MASK      0x003FC000                // RG_W1_TX02_RSV[21..14]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_TX02_RSV_SHFT      14
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_WBG_TX_CKINV_EN_ADDR CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_WBG_TX_CKINV_EN_MASK 0x00002000                // RG_W1_WBG_TX_CKINV_EN[13]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_WBG_TX_CKINV_EN_SHFT 13
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DATA_MAN_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DATA_MAN_MASK      0x00001000                // RG_W1_DATA_MAN[12]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DATA_MAN_SHFT      12
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DATA_ADDR          CONN_AFE_CTL_RG_WBG_WF1_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DATA_MASK          0x00000FFF                // RG_W1_DATA[11..0]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_02_RG_W1_DATA_SHFT          0

/* =====================================================================================

  ---RG_WBG_WF1_TX_03 (0x18041000 + 0x094)---

    RG_W1_TX03_RSV[4..0]         - (RW)  xxx 
    RG_W1_NBIAS_EXTRA_EN[5]      - (RW) Enable extra TIA DC current to create different Vicm/Vocm
                                     0: disable
                                     1: enable
    RG_W1_SWPAD[6]               - (RW) TIA bypass switch, used for DAC current measurement
                                     0: disable
                                     1: enable (bypass TIA)
    RG_W1_CALC_SEL[8..7]         - (RW) C_integrator sel:
                                     00: open
                                     01: 250fF
                                     10:250fF
                                     11: 500fF
    RG_W1_GAIN_CTRL[12..9]       - (RW) WFTX IQ DAC output swing adjustment
                                     <3:2>  for MSB V2I adjustment
                                     <1:0> for LSB V2I current adjustment
                                     00:0%
                                     01:0.65%
                                     10:0.91%
                                     11:1.52%
    RG_W1_LPF_BIAS_SEL[14..13]   - (RW) WFTX buffer OP current adjustment:
                                     00: 1.33x
                                     01: 1x
                                     10: 0.8x
                                     11: 0.67x
    RG_W1_IREF_SEL[15]           - (RW) WFTX buffer Iref adjustment:
                                     0: WF=40uA
                                     1: BT=10uA
    RG_W1_VCM_VGEAR[19..16]      - (RW) WFTX buffer CMFB Vref adjustment: 
                                     (Real Vcm,o=Vcm,ref*(6/5))
                                     0000: 460mV
                                     0001: 470mV
                                     0010: 480mV
                                     0011: 490mV
                                     0100: 500mV
                                     0101: 510mV
                                     0110: 520mV
                                     0111: 530mV 
                                     1000: 400mV
                                     1001: 420mV
                                     1010: 440mV
    RG_W1_REF_VGEAR[21..20]      - (RW) WFTX V2I reference voltage
                                     00: 730mV
                                     01: 710mV
                                     10: 700mV
                                     11: 690mV
    RG_W1_LDOLV_VGEAR[23..22]    - (RW) WFTX LDOLV voltage adjustment:
                                     00: 0.8V
                                     01: 0.75V
                                     10: 0.9V
                                     11: 0.85V
    RG_W1_LDOLPF_VGEAR[25..24]   - (RW) WFTX LDOLPF voltage adjustment:
                                     00: 1.19V
                                     01: 1.26V
                                     10: 1.34V
                                     11: 1.46V
    RG_W1_LDODAC_VGEAR[27..26]   - (RW) WFTX LDODAC voltage adjustment:
                                     00: 1.19V
                                     01: 1.26V
                                     10: 1.34V
                                     11: 1.46V
    RG_W1_TST_SEL[30..28]        - (RW) WFTX monitor signal selection:
                                     000: VDLDO_07
                                     001: VLDO_DAC_1P25
                                     010: VLDO_LPF_1P25
                                     011: VLDO_IQLAT_08
                                     100: VLDO_CK_08
                                     101: VCM
                                     110: FB_MSB_res
                                     111: FB_LSB_res
    RG_W1_TST_EN[31]             - (RW) WFTX dc monitor switch enable
                                     0: disable
                                     1: enable

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TST_EN_ADDR        CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TST_EN_MASK        0x80000000                // RG_W1_TST_EN[31]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TST_EN_SHFT        31
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TST_SEL_ADDR       CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TST_SEL_MASK       0x70000000                // RG_W1_TST_SEL[30..28]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TST_SEL_SHFT       28
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDODAC_VGEAR_ADDR  CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDODAC_VGEAR_MASK  0x0C000000                // RG_W1_LDODAC_VGEAR[27..26]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDODAC_VGEAR_SHFT  26
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDOLPF_VGEAR_ADDR  CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDOLPF_VGEAR_MASK  0x03000000                // RG_W1_LDOLPF_VGEAR[25..24]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDOLPF_VGEAR_SHFT  24
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDOLV_VGEAR_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDOLV_VGEAR_MASK   0x00C00000                // RG_W1_LDOLV_VGEAR[23..22]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LDOLV_VGEAR_SHFT   22
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_REF_VGEAR_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_REF_VGEAR_MASK     0x00300000                // RG_W1_REF_VGEAR[21..20]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_REF_VGEAR_SHFT     20
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_VCM_VGEAR_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_VCM_VGEAR_MASK     0x000F0000                // RG_W1_VCM_VGEAR[19..16]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_VCM_VGEAR_SHFT     16
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_IREF_SEL_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_IREF_SEL_MASK      0x00008000                // RG_W1_IREF_SEL[15]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_IREF_SEL_SHFT      15
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LPF_BIAS_SEL_ADDR  CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LPF_BIAS_SEL_MASK  0x00006000                // RG_W1_LPF_BIAS_SEL[14..13]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_LPF_BIAS_SEL_SHFT  13
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_GAIN_CTRL_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_GAIN_CTRL_MASK     0x00001E00                // RG_W1_GAIN_CTRL[12..9]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_GAIN_CTRL_SHFT     9
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_CALC_SEL_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_CALC_SEL_MASK      0x00000180                // RG_W1_CALC_SEL[8..7]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_CALC_SEL_SHFT      7
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_SWPAD_ADDR         CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_SWPAD_MASK         0x00000040                // RG_W1_SWPAD[6]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_SWPAD_SHFT         6
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_NBIAS_EXTRA_EN_ADDR CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_NBIAS_EXTRA_EN_MASK 0x00000020                // RG_W1_NBIAS_EXTRA_EN[5]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_NBIAS_EXTRA_EN_SHFT 5
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TX03_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TX03_RSV_MASK      0x0000001F                // RG_W1_TX03_RSV[4..0]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_TX03_RSV_SHFT      0

/* =====================================================================================

  ---RG_WBG_WF1_TX_04 (0x18041000 + 0x098)---

    RG_W1_CAL_CK_EN[0]           - (RW) 0: CAL clk is controlled by CAL_EN
                                     1: manual enable CAL CLK for Es readout debug
    RG_W1_CAL_MAN_SEL[1]         - (RW) WFTX manual-mode control signal selection
                                     0: default
                                     1: RG_CAL_MAN_EN15 selected
    RG_W1_CAL_MAN_EN[2]          - (RW) WFTX manually control signal
                                     DA_CAL_EN15
    RG_W1_MAN_SEL[3]             - (RW) WFTX manual-mode control signal selection
                                     0: default
                                     1: RG_MAN value selected
    RG_W1_MAN_CLKIQ_EN[4]        - (RW) WFTX manually control signal
                                     DA_CLKIQ_EN15
    RG_W1_MAN_DAC_EN[5]          - (RW) WFTX manually control signal
                                     DA_DAC_EN15
    RG_W1_MAN_LPF_EN[6]          - (RW) WFTX manually control signal
                                     DA_LPF_IQ_EN15
    RG_W1_MAN_ALDO_EN[7]         - (RW) WFTX manually control signal
                                     DA_ALDO_EN15
    RG_W1_INNERMODE_EN[8]        - (RW) 1: Enable 8A/8B SDM feedback loop from analog comparator 
                                     0: 8A/8B controlled by digital macro
    RG_W1_WF480CLK_EN[9]         - (RW) WBG 480 CLK for 2x2 4x4 enable
                                     0: 480MHz clk disable   
                                     1: 480MHz clk enable
    RG_W1_CKREV_EN[10]           - (RW) Reverse clk phase of clk receiver
    RG_W1_CLK_CAL_DIV[12..11]    - (RW) WFTX calibration clock rate:
                                     00: 0
                                     01: 960M/4
                                     10: 960M/8
                                     11: 960M/16
    RG_W1_EXTCLK_EN[13]          - (RW) BBTX  external clock ENABLE:
                                     0: disable
                                     1: enable
    RG_W1_CKAC_EN[14]            - (RW) CK RCV AC couple enable:
                                     1: enable ac couple
                                     0: disable ac couple use dc path ( test mode only)
    RG_W1_QREV_Q[15]             - (RW) Invert the ploarity of quantizer output in Q channel
                                     0: non-inverted
                                     1: inverted
    RG_W1_QREV_I[16]             - (RW) Invert the ploarity of quantizer output in I channel
                                     0: non-inverted
                                     1: inverted
    RG_W1_TX04_RSV[31..17]       - (RW)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_TX04_RSV_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_TX04_RSV_MASK      0xFFFE0000                // RG_W1_TX04_RSV[31..17]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_TX04_RSV_SHFT      17
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_QREV_I_ADDR        CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_QREV_I_MASK        0x00010000                // RG_W1_QREV_I[16]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_QREV_I_SHFT        16
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_QREV_Q_ADDR        CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_QREV_Q_MASK        0x00008000                // RG_W1_QREV_Q[15]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_QREV_Q_SHFT        15
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CKAC_EN_ADDR       CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CKAC_EN_MASK       0x00004000                // RG_W1_CKAC_EN[14]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CKAC_EN_SHFT       14
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_EXTCLK_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_EXTCLK_EN_MASK     0x00002000                // RG_W1_EXTCLK_EN[13]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_EXTCLK_EN_SHFT     13
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CLK_CAL_DIV_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CLK_CAL_DIV_MASK   0x00001800                // RG_W1_CLK_CAL_DIV[12..11]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CLK_CAL_DIV_SHFT   11
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CKREV_EN_ADDR      CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CKREV_EN_MASK      0x00000400                // RG_W1_CKREV_EN[10]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CKREV_EN_SHFT      10
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_WF480CLK_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_WF480CLK_EN_MASK   0x00000200                // RG_W1_WF480CLK_EN[9]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_WF480CLK_EN_SHFT   9
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_INNERMODE_EN_ADDR  CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_INNERMODE_EN_MASK  0x00000100                // RG_W1_INNERMODE_EN[8]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_INNERMODE_EN_SHFT  8
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_ALDO_EN_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_ALDO_EN_MASK   0x00000080                // RG_W1_MAN_ALDO_EN[7]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_ALDO_EN_SHFT   7
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_LPF_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_LPF_EN_MASK    0x00000040                // RG_W1_MAN_LPF_EN[6]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_LPF_EN_SHFT    6
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_DAC_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_DAC_EN_MASK    0x00000020                // RG_W1_MAN_DAC_EN[5]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_DAC_EN_SHFT    5
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_CLKIQ_EN_ADDR  CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_CLKIQ_EN_MASK  0x00000010                // RG_W1_MAN_CLKIQ_EN[4]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_CLKIQ_EN_SHFT  4
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_SEL_ADDR       CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_SEL_MASK       0x00000008                // RG_W1_MAN_SEL[3]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_MAN_SEL_SHFT       3
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_MAN_EN_ADDR    CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_MAN_EN_MASK    0x00000004                // RG_W1_CAL_MAN_EN[2]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_MAN_EN_SHFT    2
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_MAN_SEL_ADDR   CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_MAN_SEL_MASK   0x00000002                // RG_W1_CAL_MAN_SEL[1]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_MAN_SEL_SHFT   1
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_CK_EN_ADDR     CONN_AFE_CTL_RG_WBG_WF1_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_CK_EN_MASK     0x00000001                // RG_W1_CAL_CK_EN[0]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_04_RG_W1_CAL_CK_EN_SHFT     0

/* =====================================================================================

  ---RGS_WBG_WF1_TXCAL (0x18041000 + 0x09C)---

    RGS_W1_ES_READOUT_I_RO[11..0] - (RO) WFTX I channel MSB error sorce readout bit
    RGS_W1_ES_OVERFLOW_I_RO[12]  - (RO) WFTX I channel overflow flag
    RGS_W1_CAL_DONE_I_RO[13]     - (RO) WFTX I channel calibraton done flag
    RGS_W1_DMY2_RO[15..14]       - (RO)  xxx 
    RGS_W1_ES_READOUT_Q_RO[27..16] - (RO) WFTX Q channel MSB error sorce readout bit
    RGS_W1_ES_OVERFLOW_Q_RO[28]  - (RO) WFTX Q channel overflow flag
    RGS_W1_CAL_DONE_Q_RO[29]     - (RO) WFTX Q channel calibraton done flag
    RGS_W1_DMY1_RO[31..30]       - (RO)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_DMY1_RO_ADDR     CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_DMY1_RO_MASK     0xC0000000                // RGS_W1_DMY1_RO[31..30]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_DMY1_RO_SHFT     30
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_CAL_DONE_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_CAL_DONE_Q_RO_MASK 0x20000000                // RGS_W1_CAL_DONE_Q_RO[29]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_CAL_DONE_Q_RO_SHFT 29
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_OVERFLOW_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_OVERFLOW_Q_RO_MASK 0x10000000                // RGS_W1_ES_OVERFLOW_Q_RO[28]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_OVERFLOW_Q_RO_SHFT 28
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_READOUT_Q_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_READOUT_Q_RO_MASK 0x0FFF0000                // RGS_W1_ES_READOUT_Q_RO[27..16]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_READOUT_Q_RO_SHFT 16
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_DMY2_RO_ADDR     CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_DMY2_RO_MASK     0x0000C000                // RGS_W1_DMY2_RO[15..14]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_DMY2_RO_SHFT     14
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_CAL_DONE_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_CAL_DONE_I_RO_MASK 0x00002000                // RGS_W1_CAL_DONE_I_RO[13]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_CAL_DONE_I_RO_SHFT 13
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_OVERFLOW_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_OVERFLOW_I_RO_MASK 0x00001000                // RGS_W1_ES_OVERFLOW_I_RO[12]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_OVERFLOW_I_RO_SHFT 12
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_READOUT_I_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_READOUT_I_RO_MASK 0x00000FFF                // RGS_W1_ES_READOUT_I_RO[11..0]
#define CONN_AFE_CTL_RGS_WBG_WF1_TXCAL_RGS_W1_ES_READOUT_I_RO_SHFT 0

/* =====================================================================================

  ---RG_WBG_WF2_RX_01 (0x18041000 + 0x0A0)---

    RG_WBG_WF2_RX_01[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF2_RX_01_RG_WBG_WF2_RX_01_ADDR    CONN_AFE_CTL_RG_WBG_WF2_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF2_RX_01_RG_WBG_WF2_RX_01_MASK    0xFFFFFFFF                // RG_WBG_WF2_RX_01[31..0]
#define CONN_AFE_CTL_RG_WBG_WF2_RX_01_RG_WBG_WF2_RX_01_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF2_RX_02 (0x18041000 + 0x0A4)---

    RG_WBG_WF2_RX_02[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF2_RX_02_RG_WBG_WF2_RX_02_ADDR    CONN_AFE_CTL_RG_WBG_WF2_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF2_RX_02_RG_WBG_WF2_RX_02_MASK    0xFFFFFFFF                // RG_WBG_WF2_RX_02[31..0]
#define CONN_AFE_CTL_RG_WBG_WF2_RX_02_RG_WBG_WF2_RX_02_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF2_TX_01 (0x18041000 + 0x0A8)---

    RG_WBG_WF2_TX_01[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF2_TX_01_RG_WBG_WF2_TX_01_ADDR    CONN_AFE_CTL_RG_WBG_WF2_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF2_TX_01_RG_WBG_WF2_TX_01_MASK    0xFFFFFFFF                // RG_WBG_WF2_TX_01[31..0]
#define CONN_AFE_CTL_RG_WBG_WF2_TX_01_RG_WBG_WF2_TX_01_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF2_TX_02 (0x18041000 + 0x0AC)---

    RG_WBG_WF2_TX_02[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF2_TX_02_RG_WBG_WF2_TX_02_ADDR    CONN_AFE_CTL_RG_WBG_WF2_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF2_TX_02_RG_WBG_WF2_TX_02_MASK    0xFFFFFFFF                // RG_WBG_WF2_TX_02[31..0]
#define CONN_AFE_CTL_RG_WBG_WF2_TX_02_RG_WBG_WF2_TX_02_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF2_TX_03 (0x18041000 + 0x0B0)---

    RG_WBG_WF2_TX_03[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF2_TX_03_RG_WBG_WF2_TX_03_ADDR    CONN_AFE_CTL_RG_WBG_WF2_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF2_TX_03_RG_WBG_WF2_TX_03_MASK    0xFFFFFFFF                // RG_WBG_WF2_TX_03[31..0]
#define CONN_AFE_CTL_RG_WBG_WF2_TX_03_RG_WBG_WF2_TX_03_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF2_TX_04 (0x18041000 + 0x0B4)---

    RG_WBG_WF2_TX_04[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF2_TX_04_RG_WBG_WF2_TX_04_ADDR    CONN_AFE_CTL_RG_WBG_WF2_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF2_TX_04_RG_WBG_WF2_TX_04_MASK    0xFFFFFFFF                // RG_WBG_WF2_TX_04[31..0]
#define CONN_AFE_CTL_RG_WBG_WF2_TX_04_RG_WBG_WF2_TX_04_SHFT    0

/* =====================================================================================

  ---RGS_WBG_WF2_TXCAL (0x18041000 + 0x0B8)---

    RGS_WBG_WF2_TXCAL_RO[31..0]  - (RO) None

 =====================================================================================*/
#define CONN_AFE_CTL_RGS_WBG_WF2_TXCAL_RGS_WBG_WF2_TXCAL_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF2_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF2_TXCAL_RGS_WBG_WF2_TXCAL_RO_MASK 0xFFFFFFFF                // RGS_WBG_WF2_TXCAL_RO[31..0]
#define CONN_AFE_CTL_RGS_WBG_WF2_TXCAL_RGS_WBG_WF2_TXCAL_RO_SHFT 0

/* =====================================================================================

  ---RG_WBG_WF3_RX_01 (0x18041000 + 0x0BC)---

    RG_WBG_WF3_RX_01[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF3_RX_01_RG_WBG_WF3_RX_01_ADDR    CONN_AFE_CTL_RG_WBG_WF3_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF3_RX_01_RG_WBG_WF3_RX_01_MASK    0xFFFFFFFF                // RG_WBG_WF3_RX_01[31..0]
#define CONN_AFE_CTL_RG_WBG_WF3_RX_01_RG_WBG_WF3_RX_01_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF3_RX_02 (0x18041000 + 0x0C0)---

    RG_WBG_WF3_RX_02[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF3_RX_02_RG_WBG_WF3_RX_02_ADDR    CONN_AFE_CTL_RG_WBG_WF3_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF3_RX_02_RG_WBG_WF3_RX_02_MASK    0xFFFFFFFF                // RG_WBG_WF3_RX_02[31..0]
#define CONN_AFE_CTL_RG_WBG_WF3_RX_02_RG_WBG_WF3_RX_02_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF3_TX_01 (0x18041000 + 0x0C4)---

    RG_WBG_WF3_TX_01[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF3_TX_01_RG_WBG_WF3_TX_01_ADDR    CONN_AFE_CTL_RG_WBG_WF3_TX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_WF3_TX_01_RG_WBG_WF3_TX_01_MASK    0xFFFFFFFF                // RG_WBG_WF3_TX_01[31..0]
#define CONN_AFE_CTL_RG_WBG_WF3_TX_01_RG_WBG_WF3_TX_01_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF3_TX_02 (0x18041000 + 0x0C8)---

    RG_WBG_WF3_TX_02[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF3_TX_02_RG_WBG_WF3_TX_02_ADDR    CONN_AFE_CTL_RG_WBG_WF3_TX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_WF3_TX_02_RG_WBG_WF3_TX_02_MASK    0xFFFFFFFF                // RG_WBG_WF3_TX_02[31..0]
#define CONN_AFE_CTL_RG_WBG_WF3_TX_02_RG_WBG_WF3_TX_02_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF3_TX_03 (0x18041000 + 0x0CC)---

    RG_WBG_WF3_TX_03[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF3_TX_03_RG_WBG_WF3_TX_03_ADDR    CONN_AFE_CTL_RG_WBG_WF3_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF3_TX_03_RG_WBG_WF3_TX_03_MASK    0xFFFFFFFF                // RG_WBG_WF3_TX_03[31..0]
#define CONN_AFE_CTL_RG_WBG_WF3_TX_03_RG_WBG_WF3_TX_03_SHFT    0

/* =====================================================================================

  ---RG_WBG_WF3_TX_04 (0x18041000 + 0x0D0)---

    RG_WBG_WF3_TX_04[31..0]      - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF3_TX_04_RG_WBG_WF3_TX_04_ADDR    CONN_AFE_CTL_RG_WBG_WF3_TX_04_ADDR
#define CONN_AFE_CTL_RG_WBG_WF3_TX_04_RG_WBG_WF3_TX_04_MASK    0xFFFFFFFF                // RG_WBG_WF3_TX_04[31..0]
#define CONN_AFE_CTL_RG_WBG_WF3_TX_04_RG_WBG_WF3_TX_04_SHFT    0

/* =====================================================================================

  ---RGS_WBG_WF3_TXCAL (0x18041000 + 0x0D4)---

    RGS_WBG_WF3_TXCAL_RO[31..0]  - (RO) None

 =====================================================================================*/
#define CONN_AFE_CTL_RGS_WBG_WF3_TXCAL_RGS_WBG_WF3_TXCAL_RO_ADDR CONN_AFE_CTL_RGS_WBG_WF3_TXCAL_ADDR
#define CONN_AFE_CTL_RGS_WBG_WF3_TXCAL_RGS_WBG_WF3_TXCAL_RO_MASK 0xFFFFFFFF                // RGS_WBG_WF3_TXCAL_RO[31..0]
#define CONN_AFE_CTL_RGS_WBG_WF3_TXCAL_RGS_WBG_WF3_TXCAL_RO_SHFT 0

/* =====================================================================================

  ---RG_RXDS_FLT_01 (0x18041000 + 0x0D8)---

    RG_AFE_RXDS_AVG_DC_MANU_VALUE[11..0] - (RW) manual DC offset value
    RG_AFE_RXDS_AVG_DC_MANU_IQ_SEL[12] - (RW) set manual DC offset value of IQ select. 0: for Q; 1: for I
    RG_AFE_RXDS_AVG_DC_MANU_PATH_SEL[13] - (RW) set manual DC offset value of path select. 
                                     0: path 0 for BW20/40/80/160; 1: path1 for BW160
    RG_AFE_RXDS_AVG_DC_MANU[14]  - (RW) apply manual DC offset value
    RG_AFE_RXDS_AVG_DC_EN[15]    - (RW) enable dc offset calculation module
    RG_AFE_RXDS_AVG_DC_RST[16]   - (RW) reset dc offset calculation result and re-cal
    RG_AFE_RXDS_FLT_EN[17]       - (RW) enable data path of filter
    RG_AFE_RXDS_FLT_RST[18]      - (RW) reset date path of filter
    RG_AFE_RXDS_FLT_RSV0[22..19] - (RW) RSVD
    RG_AFE_RXDS_CKGEN_CONFG_EN[23] - (RW) set clock generate config
    RG_AFE_RXDS_CKGEN_CONFG_SEL[31..24] - (RW) [intlv_en,cic4_en,cic5_en]
                                     60978:  [1,0,0][1,1,0][1,0,1]@640MS/s     adcin->8'b00_01_01_00
                                             [0,1,0][0,0,1]       @320MS/s     adcin->8'b01_01_01_01  
                                             [0,1,0][0,0,1]       @160MS/s     adcin->8'b10_10_10_10
                                             [1,1,1]              @640MS/s     adcin->8'b00_01_01_01
                                             [0,1,1][0,1,0]       @320MS/s     adcin->8'b01_01_10_10
                                             [0,1,1][0,1,0]       @160MS/s     adcin->8'b10_10_11_11
                                     X19/P18:[0,0,1]              @320/160MS/s adcin->8'b00_00_00_00
                                             [0,1,0][0,0,1]       @160/80MS/s  adcin->8'b01_01_01_01  
                                             [0,1,0][0,0,1]       @80/40MS/s   adcin->8'b10_10_10_10
                                             [0,1,1]              @320/160MS/s adcin->8'b00_01_01_01
                                             [0,1,1][0,1,0]       @160/80MS/s  adcin->8'b01_01_10_10
                                             [0,1,1][0,1,0]       @80/40MS/s   adcin->8'b10_10_11_11

 =====================================================================================*/
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_CKGEN_CONFG_SEL_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_CKGEN_CONFG_SEL_MASK 0xFF000000                // RG_AFE_RXDS_CKGEN_CONFG_SEL[31..24]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_CKGEN_CONFG_SEL_SHFT 24
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_CKGEN_CONFG_EN_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_CKGEN_CONFG_EN_MASK 0x00800000                // RG_AFE_RXDS_CKGEN_CONFG_EN[23]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_CKGEN_CONFG_EN_SHFT 23
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_RSV0_ADDR  CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_RSV0_MASK  0x00780000                // RG_AFE_RXDS_FLT_RSV0[22..19]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_RSV0_SHFT  19
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_RST_ADDR   CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_RST_MASK   0x00040000                // RG_AFE_RXDS_FLT_RST[18]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_RST_SHFT   18
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_EN_ADDR    CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_EN_MASK    0x00020000                // RG_AFE_RXDS_FLT_EN[17]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_FLT_EN_SHFT    17
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_RST_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_RST_MASK 0x00010000                // RG_AFE_RXDS_AVG_DC_RST[16]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_RST_SHFT 16
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_EN_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_EN_MASK 0x00008000                // RG_AFE_RXDS_AVG_DC_EN[15]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_EN_SHFT 15
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_MASK 0x00004000                // RG_AFE_RXDS_AVG_DC_MANU[14]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_SHFT 14
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_PATH_SEL_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_PATH_SEL_MASK 0x00002000                // RG_AFE_RXDS_AVG_DC_MANU_PATH_SEL[13]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_PATH_SEL_SHFT 13
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_IQ_SEL_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_IQ_SEL_MASK 0x00001000                // RG_AFE_RXDS_AVG_DC_MANU_IQ_SEL[12]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_IQ_SEL_SHFT 12
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_VALUE_ADDR CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_VALUE_MASK 0x00000FFF                // RG_AFE_RXDS_AVG_DC_MANU_VALUE[11..0]
#define CONN_AFE_CTL_RG_RXDS_FLT_01_RG_AFE_RXDS_AVG_DC_MANU_VALUE_SHFT 0

/* =====================================================================================

  ---RG_RXDS_FLT_02 (0x18041000 + 0x0DC)---

    RO_AFE_RXDS_AVG_DC_VALUE_RO[11..0] - (RO) read out calculation dc offset result
    RG_AFE_RXDS_AVG_DC_RO_IQ_SEL[12] - (RW) set read out avg dc IQ
    RG_AFE_RXDS_AVG_DC_RO_PATH_SEL[13] - (RW) set read out avg dc path
    RG_AFE_RXDS_RXADC_OUT_FORMAT[14] - (RW) 1: mapping to signed rxds_flt out formate
    RG_AFE_RXDS_RXADC_IN_FORMAT[15] - (RW) 1: mapping to signed adc formate
    RG_AFE_RXDS_RXADC_IQ_SWAP[16] - (RW) swap I/Q
    RG_AFE_RXDS_RXADC_CK_INV[17] - (RW) ck inveter
    RG_AFE_RXDS_FLT_INTLV_SWH[18] - (RW) switch p1/p0 interleaving path
    RG_AFE_RXDS_FLT_INTLV_EN[19] - (RW) dcod use 2 path for interleaving
    RG_AFE_RXDS_DCOCOM_EN[20]    - (RW) enable dc offset compensate
    RG_AFE_RXDS_FLT_CIC4_EN[21]  - (RW) 1'b1: use CIC4 filter; 1'b0: bypass CIC4 filter
    RG_AFE_RXDS_FLT_CIC5_EN[22]  - (RW) 1'b1: use CIC5 filter; 1'b0: bypass CIC5 filter
    RG_AFE_RXDS_FLT_RSV1[31..23] - (RW) RSVD

 =====================================================================================*/
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_RSV1_ADDR  CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_RSV1_MASK  0xFF800000                // RG_AFE_RXDS_FLT_RSV1[31..23]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_RSV1_SHFT  23
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_CIC5_EN_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_CIC5_EN_MASK 0x00400000                // RG_AFE_RXDS_FLT_CIC5_EN[22]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_CIC5_EN_SHFT 22
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_CIC4_EN_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_CIC4_EN_MASK 0x00200000                // RG_AFE_RXDS_FLT_CIC4_EN[21]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_CIC4_EN_SHFT 21
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_DCOCOM_EN_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_DCOCOM_EN_MASK 0x00100000                // RG_AFE_RXDS_DCOCOM_EN[20]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_DCOCOM_EN_SHFT 20
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_INTLV_EN_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_INTLV_EN_MASK 0x00080000                // RG_AFE_RXDS_FLT_INTLV_EN[19]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_INTLV_EN_SHFT 19
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_INTLV_SWH_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_INTLV_SWH_MASK 0x00040000                // RG_AFE_RXDS_FLT_INTLV_SWH[18]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_FLT_INTLV_SWH_SHFT 18
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_CK_INV_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_CK_INV_MASK 0x00020000                // RG_AFE_RXDS_RXADC_CK_INV[17]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_CK_INV_SHFT 17
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_IQ_SWAP_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_IQ_SWAP_MASK 0x00010000                // RG_AFE_RXDS_RXADC_IQ_SWAP[16]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_IQ_SWAP_SHFT 16
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_IN_FORMAT_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_IN_FORMAT_MASK 0x00008000                // RG_AFE_RXDS_RXADC_IN_FORMAT[15]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_IN_FORMAT_SHFT 15
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_OUT_FORMAT_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_OUT_FORMAT_MASK 0x00004000                // RG_AFE_RXDS_RXADC_OUT_FORMAT[14]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_RXADC_OUT_FORMAT_SHFT 14
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_AVG_DC_RO_PATH_SEL_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_AVG_DC_RO_PATH_SEL_MASK 0x00002000                // RG_AFE_RXDS_AVG_DC_RO_PATH_SEL[13]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_AVG_DC_RO_PATH_SEL_SHFT 13
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_AVG_DC_RO_IQ_SEL_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_AVG_DC_RO_IQ_SEL_MASK 0x00001000                // RG_AFE_RXDS_AVG_DC_RO_IQ_SEL[12]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RG_AFE_RXDS_AVG_DC_RO_IQ_SEL_SHFT 12
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RO_AFE_RXDS_AVG_DC_VALUE_RO_ADDR CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RO_AFE_RXDS_AVG_DC_VALUE_RO_MASK 0x00000FFF                // RO_AFE_RXDS_AVG_DC_VALUE_RO[11..0]
#define CONN_AFE_CTL_RG_RXDS_FLT_02_RO_AFE_RXDS_AVG_DC_VALUE_RO_SHFT 0

/* =====================================================================================

  ---RG_SIN_GEN_01 (0x18041000 + 0x0E0)---

    RG_AFE_TXPAT_GAIN_Q[2..0]    - (RW) Log:     0: 1    1: 1/2    2: 1/4    3: 1/8    4: 1/16   5: 1/32   6: 1/32   7: 1/32
                                     Linear: 0: 1    1: 7/8    2: 6/8    3: 5/8    4: 4/8     5: 3/8     6: 2/8     7: 1/8
    RESERVED3[3]                 - (RO) Reserved bits
    RG_AFE_TXPAT_GAIN_I[6..4]    - (RW) Log:     0: 1    1: 1/2    2: 1/4    3: 1/8    4: 1/16   5: 1/32   6: 1/32   7: 1/32
                                     Linear: 0: 1    1: 7/8    2: 6/8    3: 5/8    4: 4/8     5: 3/8     6: 2/8     7: 1/8
    RESERVED7[7]                 - (RO) Reserved bits
    RG_AFE_TXPAT_MODE_Q[11..8]   - (RW) Sine_gen mode for q
                                     0: single-tone
                                     1: two-tone
                                     2: ramp up
                                     3: ramp down
                                     4:  lfsr24
                                     5:  lfsr16
                                     6:  lfsr12
                                     7:  lfsr8
    RG_AFE_TXPAT_MODE_I[15..12]  - (RW) Sine_gen mode for i
                                     0: single-tone
                                     1: two-tone
                                     2: ramp up
                                     3: ramp down
                                     4:  lfsr24
                                     5:  lfsr16
                                     6:  lfsr12
                                     7:  lfsr8
    RG_AFE_TXPAT_GAIN_MODE[16]   - (RW) GAIN mode(Set 0 for ACD DAC test)
                                     0: Linear
                                     1: Log
    RG_AFE_TXPAT_S2P_EN[17]      - (RW) S2P mode enable
                                     0: 12 bit sine_gen
                                     1: 24 bit sine_gen
    RG_AFE_TXPAT_CK_SEL[19..18]  - (RW) SINGEN_CK_SEL
                                     2'b00: AD_WBG_WF_DA_CKO_TXSIN
                                     2'b01: AD_WBG_BT0_DA_CKO_TXSIN
                                     2'b10: AD_WBG_BT1_DA_CKO
                                     2'b11: 0
    RG_AFE_TXPAT_CLKDIV_N[21..20] - (RW) global freq div by 1024^n
    RESERVED22[23..22]           - (RO) Reserved bits
    RG_AFE_TXPAT_FIX_EN_Q[24]    - (RW) Output fix value singen_fix[9:0]
    RG_AFE_TXPAT_FIX_EN_I[25]    - (RW) Output fix value singen_fix[9:0]
    RG_AFE_TXPAT_EN[26]          - (RW) Sine_gen enable
    RG_SINGEN_EN_MUX_WF3[27]     - (RW) Sine_gen enable for WF3 mux
                                     0: from modem
                                     1: from AFE_TXPAT
    RG_SINGEN_EN_MUX_WF2[28]     - (RW) Sine_gen enable for WF2 mux
                                     0: from modem
                                     1: from AFE_TXPAT
    RG_SINGEN_EN_MUX_WF1[29]     - (RW) Sine_gen enable for WF1 mux
                                     0: from modem
                                     1: from AFE_TXPAT
    RG_SINGEN_EN_MUX_WF0[30]     - (RW) Sine_gen enable for WF0 mux
                                     0: from modem
                                     1: from AFE_TXPAT
    RG_SINGEN_EN_MUX_BT[31]      - (RW) Sine_gen enable for BT mux
                                     0: from modem
                                     1: from AFE_TXPAT

 =====================================================================================*/
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_BT_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_BT_MASK    0x80000000                // RG_SINGEN_EN_MUX_BT[31]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_BT_SHFT    31
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF0_ADDR   CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF0_MASK   0x40000000                // RG_SINGEN_EN_MUX_WF0[30]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF0_SHFT   30
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF1_ADDR   CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF1_MASK   0x20000000                // RG_SINGEN_EN_MUX_WF1[29]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF1_SHFT   29
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF2_ADDR   CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF2_MASK   0x10000000                // RG_SINGEN_EN_MUX_WF2[28]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF2_SHFT   28
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF3_ADDR   CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF3_MASK   0x08000000                // RG_SINGEN_EN_MUX_WF3[27]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_SINGEN_EN_MUX_WF3_SHFT   27
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_EN_ADDR        CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_EN_MASK        0x04000000                // RG_AFE_TXPAT_EN[26]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_EN_SHFT        26
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_FIX_EN_I_ADDR  CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_FIX_EN_I_MASK  0x02000000                // RG_AFE_TXPAT_FIX_EN_I[25]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_FIX_EN_I_SHFT  25
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_FIX_EN_Q_ADDR  CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_FIX_EN_Q_MASK  0x01000000                // RG_AFE_TXPAT_FIX_EN_Q[24]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_FIX_EN_Q_SHFT  24
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_CLKDIV_N_ADDR  CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_CLKDIV_N_MASK  0x00300000                // RG_AFE_TXPAT_CLKDIV_N[21..20]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_CLKDIV_N_SHFT  20
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_CK_SEL_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_CK_SEL_MASK    0x000C0000                // RG_AFE_TXPAT_CK_SEL[19..18]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_CK_SEL_SHFT    18
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_S2P_EN_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_S2P_EN_MASK    0x00020000                // RG_AFE_TXPAT_S2P_EN[17]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_S2P_EN_SHFT    17
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_MODE_ADDR CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_MODE_MASK 0x00010000                // RG_AFE_TXPAT_GAIN_MODE[16]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_MODE_SHFT 16
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_MODE_I_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_MODE_I_MASK    0x0000F000                // RG_AFE_TXPAT_MODE_I[15..12]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_MODE_I_SHFT    12
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_MODE_Q_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_MODE_Q_MASK    0x00000F00                // RG_AFE_TXPAT_MODE_Q[11..8]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_MODE_Q_SHFT    8
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_I_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_I_MASK    0x00000070                // RG_AFE_TXPAT_GAIN_I[6..4]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_I_SHFT    4
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_Q_ADDR    CONN_AFE_CTL_RG_SIN_GEN_01_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_Q_MASK    0x00000007                // RG_AFE_TXPAT_GAIN_Q[2..0]
#define CONN_AFE_CTL_RG_SIN_GEN_01_RG_AFE_TXPAT_GAIN_Q_SHFT    0

/* =====================================================================================

  ---RG_SIN_GEN_02 (0x18041000 + 0x0E4)---

    RG_AFE_TXPAT_FIX_Q[11..0]    - (RW) Output fix value
    RESERVED12[15..12]           - (RO) Reserved bits
    RG_AFE_TXPAT_FIX_I[27..16]   - (RW) Output fix value
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_SIN_GEN_02_RG_AFE_TXPAT_FIX_I_ADDR     CONN_AFE_CTL_RG_SIN_GEN_02_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_02_RG_AFE_TXPAT_FIX_I_MASK     0x0FFF0000                // RG_AFE_TXPAT_FIX_I[27..16]
#define CONN_AFE_CTL_RG_SIN_GEN_02_RG_AFE_TXPAT_FIX_I_SHFT     16
#define CONN_AFE_CTL_RG_SIN_GEN_02_RG_AFE_TXPAT_FIX_Q_ADDR     CONN_AFE_CTL_RG_SIN_GEN_02_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_02_RG_AFE_TXPAT_FIX_Q_MASK     0x00000FFF                // RG_AFE_TXPAT_FIX_Q[11..0]
#define CONN_AFE_CTL_RG_SIN_GEN_02_RG_AFE_TXPAT_FIX_Q_SHFT     0

/* =====================================================================================

  ---RG_SIN_GEN_03 (0x18041000 + 0x0E8)---

    RG_AFE_TXPAT_CLKDIV_Q[9..0]  - (RW) Sine table data update freqency = WB_DAC_CK/clkdiv
    RESERVED10[15..10]           - (RO) Reserved bits
    RG_AFE_TXPAT_CLKDIV_I[25..16] - (RW) Sine table data update freqency = WB_DAC_CK/clkdiv
    RESERVED26[31..26]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_SIN_GEN_03_RG_AFE_TXPAT_CLKDIV_I_ADDR  CONN_AFE_CTL_RG_SIN_GEN_03_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_03_RG_AFE_TXPAT_CLKDIV_I_MASK  0x03FF0000                // RG_AFE_TXPAT_CLKDIV_I[25..16]
#define CONN_AFE_CTL_RG_SIN_GEN_03_RG_AFE_TXPAT_CLKDIV_I_SHFT  16
#define CONN_AFE_CTL_RG_SIN_GEN_03_RG_AFE_TXPAT_CLKDIV_Q_ADDR  CONN_AFE_CTL_RG_SIN_GEN_03_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_03_RG_AFE_TXPAT_CLKDIV_Q_MASK  0x000003FF                // RG_AFE_TXPAT_CLKDIV_Q[9..0]
#define CONN_AFE_CTL_RG_SIN_GEN_03_RG_AFE_TXPAT_CLKDIV_Q_SHFT  0

/* =====================================================================================

  ---RG_SIN_GEN_04 (0x18041000 + 0x0EC)---

    RG_AFE_TXPAT_INC_STEP0_Q[11..0] - (RW) Sine Tone Freq = WB_DAC_CK*inc_step/clkdiv/4096
    RESERVED12[15..12]           - (RO) Reserved bits
    RG_AFE_TXPAT_INC_STEP0_I[27..16] - (RW) Sine Tone Freq = WB_DAC_CK*inc_step/clkdiv/4096
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_SIN_GEN_04_RG_AFE_TXPAT_INC_STEP0_I_ADDR CONN_AFE_CTL_RG_SIN_GEN_04_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_04_RG_AFE_TXPAT_INC_STEP0_I_MASK 0x0FFF0000                // RG_AFE_TXPAT_INC_STEP0_I[27..16]
#define CONN_AFE_CTL_RG_SIN_GEN_04_RG_AFE_TXPAT_INC_STEP0_I_SHFT 16
#define CONN_AFE_CTL_RG_SIN_GEN_04_RG_AFE_TXPAT_INC_STEP0_Q_ADDR CONN_AFE_CTL_RG_SIN_GEN_04_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_04_RG_AFE_TXPAT_INC_STEP0_Q_MASK 0x00000FFF                // RG_AFE_TXPAT_INC_STEP0_Q[11..0]
#define CONN_AFE_CTL_RG_SIN_GEN_04_RG_AFE_TXPAT_INC_STEP0_Q_SHFT 0

/* =====================================================================================

  ---RG_SIN_GEN_05 (0x18041000 + 0x0F0)---

    RG_AFE_TXPAT_START_ADDR0_Q[11..0] - (RW) Tx sine wave auto generate start address for q
    RESERVED12[15..12]           - (RO) Reserved bits
    RG_AFE_TXPAT_START_ADDR0_I[27..16] - (RW) Tx sine wave auto generate start address for i
    RESERVED28[31..28]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_SIN_GEN_05_RG_AFE_TXPAT_START_ADDR0_I_ADDR CONN_AFE_CTL_RG_SIN_GEN_05_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_05_RG_AFE_TXPAT_START_ADDR0_I_MASK 0x0FFF0000                // RG_AFE_TXPAT_START_ADDR0_I[27..16]
#define CONN_AFE_CTL_RG_SIN_GEN_05_RG_AFE_TXPAT_START_ADDR0_I_SHFT 16
#define CONN_AFE_CTL_RG_SIN_GEN_05_RG_AFE_TXPAT_START_ADDR0_Q_ADDR CONN_AFE_CTL_RG_SIN_GEN_05_ADDR
#define CONN_AFE_CTL_RG_SIN_GEN_05_RG_AFE_TXPAT_START_ADDR0_Q_MASK 0x00000FFF                // RG_AFE_TXPAT_START_ADDR0_Q[11..0]
#define CONN_AFE_CTL_RG_SIN_GEN_05_RG_AFE_TXPAT_START_ADDR0_Q_SHFT 0

/* =====================================================================================

  ---RG_PLL_STB_TIME (0x18041000 + 0x0F4)---

    RG_WBG_WPLL_STB_TIME[14..0]  - (RW) Default : 2101 osc_ck cycle (80us @ 26MHz)
                                     (about 80us + 1 % margin)
    PLL_STB_RSV2[15]             - (RW) STB_reserved2
    RG_WBG_BPLL_STB_TIME[30..16] - (RW) Default : 788 osc_ck cycle (30us @ 26MHz)
                                     (about 30us + 1 % margin)
    PLL_STB_RSV1[31]             - (RW) STB_reserved1

 =====================================================================================*/
#define CONN_AFE_CTL_RG_PLL_STB_TIME_PLL_STB_RSV1_ADDR         CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR
#define CONN_AFE_CTL_RG_PLL_STB_TIME_PLL_STB_RSV1_MASK         0x80000000                // PLL_STB_RSV1[31]
#define CONN_AFE_CTL_RG_PLL_STB_TIME_PLL_STB_RSV1_SHFT         31
#define CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_BPLL_STB_TIME_ADDR CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR
#define CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_BPLL_STB_TIME_MASK 0x7FFF0000                // RG_WBG_BPLL_STB_TIME[30..16]
#define CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_BPLL_STB_TIME_SHFT 16
#define CONN_AFE_CTL_RG_PLL_STB_TIME_PLL_STB_RSV2_ADDR         CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR
#define CONN_AFE_CTL_RG_PLL_STB_TIME_PLL_STB_RSV2_MASK         0x00008000                // PLL_STB_RSV2[15]
#define CONN_AFE_CTL_RG_PLL_STB_TIME_PLL_STB_RSV2_SHFT         15
#define CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_WPLL_STB_TIME_ADDR CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR
#define CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_WPLL_STB_TIME_MASK 0x00007FFF                // RG_WBG_WPLL_STB_TIME[14..0]
#define CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_WPLL_STB_TIME_SHFT 0

/* =====================================================================================

  ---RG_WBG_DFS_RX_01 (0x18041000 + 0x0F8)---

    RG_WBG_DFS_01[31..0]         - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_DFS_RX_01_RG_WBG_DFS_01_ADDR       CONN_AFE_CTL_RG_WBG_DFS_RX_01_ADDR
#define CONN_AFE_CTL_RG_WBG_DFS_RX_01_RG_WBG_DFS_01_MASK       0xFFFFFFFF                // RG_WBG_DFS_01[31..0]
#define CONN_AFE_CTL_RG_WBG_DFS_RX_01_RG_WBG_DFS_01_SHFT       0

/* =====================================================================================

  ---RG_WBG_DFS_RX_02 (0x18041000 + 0x0FC)---

    RG_WBG_DFS_02[31..0]         - (RW) to be update

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_DFS_RX_02_RG_WBG_DFS_02_ADDR       CONN_AFE_CTL_RG_WBG_DFS_RX_02_ADDR
#define CONN_AFE_CTL_RG_WBG_DFS_RX_02_RG_WBG_DFS_02_MASK       0xFFFFFFFF                // RG_WBG_DFS_02[31..0]
#define CONN_AFE_CTL_RG_WBG_DFS_RX_02_RG_WBG_DFS_02_SHFT       0

/* =====================================================================================

  ---RG_WBG_GL5_01 (0x18041000 + 0x100)---

    RG_GL5_AD_LDO0P8_VSEL[2..0]  - (RW) 0.8V LDO tuning
                                     111:0.860
                                     110: 0.851
                                     101: 0.838
                                     100: 0.828 (POR)
                                     011: 0.815
                                     010: 0.804
                                     001: 0.792
                                     000: 0.782
    RG_GL5_AD_VREF_SEL[5..3]     - (RW) VREF selection
                                     111: 0.861
                                     110: 0.850
                                     101: 0.838
                                     100: 0.827
                                     011: 0.814
                                     010: 0.803 (POR)
                                     001: 0.791
                                     000: 0.780
    RG_GL5_DUODRI[6]             - (RW) 1: reference gen doubled current
                                     0: reference gen normal current
    RG_GL5_RAW_OUT_EN[7]         - (RW) 1: Raw data out
                                     0: Normal data out
    RG_GL5_OUT_CLK_DIV_H4_L2[8]  - (RW) 1: output clock divided by 4
                                     0: output clock divided by 2
    RG_GL5_OUT_CLK_DIV_EN[9]     - (RW) 1: output clock divider enable 
                                     0: disable
    RG_GL5_AD_OUTPUT_EN[10]      - (RW) 1: output enable 
                                     0: output disable
    RG_GL5_AD_CK_OUT_RISE[11]    - (RW) 1: Output clock uses positive polarity
                                     0: Output clock uses negative polarity
    RG_GL5_AD_CH_DFF_RISE[12]    - (RW) 1: DFF sync clock uses positive polarity
                                     0: DFF sync clock uses negative polarity
    RG_GL5_AD_LDO_PUMP_EN[13]    - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_GL5_AD_COMP_TAIL_MSB[14]  - (RW) 1: enlarge comparator tail transistor size for first 3 MSBs
                                     0: default size
    RG_GL5_AD_COMP_TAIL[15]      - (RW) 1: enlarge comparator tail transistor size
                                     0: default size
    RG_GL5_MODE_L1L5_SYNC_CLK[16] - (RW) 1: L1 and L5 dual band use external post-divider clock
                                     0: Internal
    RG_GL5_MODE_BT_GPS[17]       - (RW) 1: BT Mode for CLK Selection
                                     0: GPS Mode for CLK Selection (always 0 for GPS RX)
    RG_GL5_AD_EXT_CLK_SEL[18]    - (RW) 1: External clock source
                                     0: Internal PLL as clock
    RG_GL5_AD_QCH_EN[19]         - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_GL5_AD_ICH_EN[20]         - (RW) 1: I channel enable
                                     0: I channel disable
    RG_GL5_AD_VAR_CLKS[21]       - (RW) 1: variable clks
                                     0: fixed clks
    RG_GL5_AD_LSB_EN[22]         - (RW) 1: 10bit cycles conversion
                                     0: 11 bit cycles conversion
    RG_GL5_AD_VGEN_PUMP_EN[23]   - (RW) 1: reference gen pump enable
                                     0: reference gen pump disable
    RG_GL5_CK_CH_RISE[24]        - (RW) 1: Decoder output clock uses positive polarity
                                     0: Decoder output clock uses negative polarity
    RG_WBG_GL5_01_RSV1[25]       - (RW)  xxx 
    RG_GL5_INTERNAL_ADCKSEL[26]  - (RW) Internal ADC Clock Divider control (for BT)
                                     0: ADC normal mode clock=32MHz
                                     1: ADC normal mode clock=64MHz
    RG_WBG_GL5_01_RSV0[29..27]   - (RW)  xxx 
    RG_GL5_AD_EN[30]             - (RW) 1: Enable ADC
                                     0: Disable ADC
    RG_GL5_RG_DA_Mode[31]        - (RW) 1: RG Mode
                                     0: DA Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RG_DA_Mode_ADDR      CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RG_DA_Mode_MASK      0x80000000                // RG_GL5_RG_DA_Mode[31]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RG_DA_Mode_SHFT      31
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EN_ADDR           CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EN_MASK           0x40000000                // RG_GL5_AD_EN[30]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EN_SHFT           30
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_WBG_GL5_01_RSV0_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_WBG_GL5_01_RSV0_MASK     0x38000000                // RG_WBG_GL5_01_RSV0[29..27]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_WBG_GL5_01_RSV0_SHFT     27
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_INTERNAL_ADCKSEL_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_INTERNAL_ADCKSEL_MASK 0x04000000                // RG_GL5_INTERNAL_ADCKSEL[26]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_INTERNAL_ADCKSEL_SHFT 26
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_WBG_GL5_01_RSV1_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_WBG_GL5_01_RSV1_MASK     0x02000000                // RG_WBG_GL5_01_RSV1[25]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_WBG_GL5_01_RSV1_SHFT     25
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_CK_CH_RISE_ADDR      CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_CK_CH_RISE_MASK      0x01000000                // RG_GL5_CK_CH_RISE[24]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_CK_CH_RISE_SHFT      24
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VGEN_PUMP_EN_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VGEN_PUMP_EN_MASK 0x00800000                // RG_GL5_AD_VGEN_PUMP_EN[23]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VGEN_PUMP_EN_SHFT 23
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LSB_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LSB_EN_MASK       0x00400000                // RG_GL5_AD_LSB_EN[22]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LSB_EN_SHFT       22
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VAR_CLKS_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VAR_CLKS_MASK     0x00200000                // RG_GL5_AD_VAR_CLKS[21]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VAR_CLKS_SHFT     21
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_ICH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_ICH_EN_MASK       0x00100000                // RG_GL5_AD_ICH_EN[20]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_ICH_EN_SHFT       20
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_QCH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_QCH_EN_MASK       0x00080000                // RG_GL5_AD_QCH_EN[19]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_QCH_EN_SHFT       19
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EXT_CLK_SEL_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EXT_CLK_SEL_MASK  0x00040000                // RG_GL5_AD_EXT_CLK_SEL[18]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EXT_CLK_SEL_SHFT  18
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_BT_GPS_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_BT_GPS_MASK     0x00020000                // RG_GL5_MODE_BT_GPS[17]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_BT_GPS_SHFT     17
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_L1L5_SYNC_CLK_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_L1L5_SYNC_CLK_MASK 0x00010000                // RG_GL5_MODE_L1L5_SYNC_CLK[16]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_L1L5_SYNC_CLK_SHFT 16
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_ADDR    CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MASK    0x00008000                // RG_GL5_AD_COMP_TAIL[15]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_SHFT    15
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MSB_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MSB_MASK 0x00004000                // RG_GL5_AD_COMP_TAIL_MSB[14]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MSB_SHFT 14
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO_PUMP_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO_PUMP_EN_MASK  0x00002000                // RG_GL5_AD_LDO_PUMP_EN[13]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO_PUMP_EN_SHFT  13
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CH_DFF_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CH_DFF_RISE_MASK  0x00001000                // RG_GL5_AD_CH_DFF_RISE[12]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CH_DFF_RISE_SHFT  12
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CK_OUT_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CK_OUT_RISE_MASK  0x00000800                // RG_GL5_AD_CK_OUT_RISE[11]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CK_OUT_RISE_SHFT  11
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_OUTPUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_OUTPUT_EN_MASK    0x00000400                // RG_GL5_AD_OUTPUT_EN[10]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_OUTPUT_EN_SHFT    10
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_OUT_CLK_DIV_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_OUT_CLK_DIV_EN_MASK  0x00000200                // RG_GL5_OUT_CLK_DIV_EN[9]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_OUT_CLK_DIV_EN_SHFT  9
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_OUT_CLK_DIV_H4_L2_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_OUT_CLK_DIV_H4_L2_MASK 0x00000100                // RG_GL5_OUT_CLK_DIV_H4_L2[8]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_OUT_CLK_DIV_H4_L2_SHFT 8
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RAW_OUT_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RAW_OUT_EN_MASK      0x00000080                // RG_GL5_RAW_OUT_EN[7]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RAW_OUT_EN_SHFT      7
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DUODRI_ADDR          CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DUODRI_MASK          0x00000040                // RG_GL5_DUODRI[6]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DUODRI_SHFT          6
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VREF_SEL_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VREF_SEL_MASK     0x00000038                // RG_GL5_AD_VREF_SEL[5..3]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VREF_SEL_SHFT     3
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO0P8_VSEL_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO0P8_VSEL_MASK  0x00000007                // RG_GL5_AD_LDO0P8_VSEL[2..0]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO0P8_VSEL_SHFT  0

/* =====================================================================================

  ---RG_WBG_GL5_02 (0x18041000 + 0x104)---

    RG_WBG_GL5_02_RSV4[9..0]     - (RW)  xxx 
    RG_GL5_RX_TST_SEL[10]        - (RW) RX DC test
                                     0: MON_BUF
                                     1: MON_AD
    RG_GL5_RX_TST_EN[11]         - (RW) 1: for RX DC monitor enable
                                     0: disable
    RG_GL5_BUF_TSTSEL[13..12]    - (RW) ADBUF DC test
                                     00: LDO_0P98
                                     01: VICM
                                     10: VCMREF
                                     11: NO_USED
    RG_GL5_BUF_TST_EN[14]        - (RW) 1: for ADBUF DC monitor enable
                                     0: disable
    RG_WBG_GL5_02_RSV3[15]       - (RW)  xxx 
    RG_GL5_BUF_EN[16]            - (RW) ADBUF on/off manual control
                                     0: ADBUF manual off
                                     1: ADBUF manual on
    RG_GL5_BUFEN_MAN[17]         - (RW) ADBUF on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_GL5_V098_LDO_ADBUF_VSEL[20..18] - (RW) 0.98V LDO tuning
                                     111: 1.0202
                                     110: 1.0058
                                     101: 0.9976
                                     100: 0.9832  (POR)
                                     011: 0.9726
                                     010: 0.9583
                                     001: 0.9500
                                     000: 0.9357
    RG_GL5_V125_LDO_VSEL[23..21] - (RW) 1.25V LDO tuning
                                     111: 1.2916
                                     110: 1.2769
                                     101: 1.2686
                                     100: 1.2540 (POR)
                                     011: 1.2437
                                     010: 1.2290
                                     001: 1.2208
                                     000: 1.2061
    RG_WBG_GL5_02_RSV2[24]       - (RW)  xxx 
    RG_GL5_V098_LDO_AD_VSEL[27..25] - (RW) 0.98V LDO tuning
                                     111: 1.0202
                                     110: 1.0058
                                     101: 0.9976
                                     100: 0.9832  (POR)
                                     011: 0.9726
                                     010: 0.9583
                                     001: 0.9500
                                     000: 0.9357
    RG_GL5_AD_TST_EN[28]         - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_GL5_AD_TST_SEL[31..29]    - (RW) AD DC monitored voltage selection
                                     000:VR branch 1 full reference
                                     001: VR branch 1 half reference
                                     010: VR branch 2 full reference
                                     011: VR branch 2  half reference   
                                     100: LDO_0P9V

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_AD_TST_SEL_ADDR      CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_AD_TST_SEL_MASK      0xE0000000                // RG_GL5_AD_TST_SEL[31..29]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_AD_TST_SEL_SHFT      29
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_AD_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_AD_TST_EN_MASK       0x10000000                // RG_GL5_AD_TST_EN[28]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_AD_TST_EN_SHFT       28
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V098_LDO_AD_VSEL_ADDR CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V098_LDO_AD_VSEL_MASK 0x0E000000                // RG_GL5_V098_LDO_AD_VSEL[27..25]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V098_LDO_AD_VSEL_SHFT 25
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV2_ADDR     CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV2_MASK     0x01000000                // RG_WBG_GL5_02_RSV2[24]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV2_SHFT     24
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V125_LDO_VSEL_ADDR   CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V125_LDO_VSEL_MASK   0x00E00000                // RG_GL5_V125_LDO_VSEL[23..21]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V125_LDO_VSEL_SHFT   21
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V098_LDO_ADBUF_VSEL_ADDR CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V098_LDO_ADBUF_VSEL_MASK 0x001C0000                // RG_GL5_V098_LDO_ADBUF_VSEL[20..18]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_V098_LDO_ADBUF_VSEL_SHFT 18
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUFEN_MAN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUFEN_MAN_MASK       0x00020000                // RG_GL5_BUFEN_MAN[17]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUFEN_MAN_SHFT       17
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_EN_ADDR          CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_EN_MASK          0x00010000                // RG_GL5_BUF_EN[16]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_EN_SHFT          16
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV3_ADDR     CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV3_MASK     0x00008000                // RG_WBG_GL5_02_RSV3[15]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV3_SHFT     15
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_TST_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_TST_EN_MASK      0x00004000                // RG_GL5_BUF_TST_EN[14]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_TST_EN_SHFT      14
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_TSTSEL_ADDR      CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_TSTSEL_MASK      0x00003000                // RG_GL5_BUF_TSTSEL[13..12]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_BUF_TSTSEL_SHFT      12
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_RX_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_RX_TST_EN_MASK       0x00000800                // RG_GL5_RX_TST_EN[11]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_RX_TST_EN_SHFT       11
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_RX_TST_SEL_ADDR      CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_RX_TST_SEL_MASK      0x00000400                // RG_GL5_RX_TST_SEL[10]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_GL5_RX_TST_SEL_SHFT      10
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV4_ADDR     CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV4_MASK     0x000003FF                // RG_WBG_GL5_02_RSV4[9..0]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV4_SHFT     0

/* =====================================================================================

  ---RG_WBG_CTL_OCC (0x18041000 + 0x124)---

    RG_WBG_EN_WPLL_OCC[0]        - (RW) Directly trigger WPLL in OCC mode (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BPLL_OCC[1]        - (RW) Directly trigger BPLL in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_XBUF_OCC[2]        - (RW) Directly trigger XBUF in OCC mode (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BG_OCC[3]          - (RW) Directly trigger BG in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_CTL_OCC_RSV1[10..4]   - (RW) Reserved bits
    RG_WBG_EN_WBPLL_OCC[11]      - (RW) Directly trigger WBPLL in OCC mode (when RG_OCC_PLL_SW = 1'b1, RFSOC only)
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_AD_WF0TSSI[12]  - (RW) Source clock enable - WF0TSSI AD
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_AD_DFS[13]      - (RW) Source clock enable - DFS AD
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_DA_WF[14]       - (RW) Source clock enable - WF DA
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_AD_WF[15]       - (RW) Source clock enable - WF AD
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_DA_BT1[16]      - (RW) Source clock enable - BT1 DA
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_AD_BT1[17]      - (RW) Source clock enable - BT1 AD
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_DA_BT0[18]      - (RW) Source clock enable - BT0 DA
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_AD_BT0[19]      - (RW) Source clock enable - BT0 AD
                                     0: Disable
                                     1: Enable
    RG_WBG_CTL_OCC_RSV2[21..20]  - (RW) Reserved bits
    RG_WBG_CTL_OCC_RSV3[22]      - (RW) Reserved bits
    RG_WBG_CG_EN_AD_GPS[23]      - (RW) Source clock enable - GPS AD
                                     0: Disable
                                     1: Enable
    RG_WBG_CTL_OCC_RSV4[24]      - (RW) Reserved bits
    RG_WBG_CG_EN_WPLL960[25]     - (RW) Source clock enable - WPLL960
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_WPLL640[26]     - (RW) Source clock enable - WPLL640
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_BPLL[27]        - (RW) Source clock enable - BPLL
                                     0: Disable
                                     1: Enable
    RG_WBG_AFE_XTAL_CFG_OCC[30..28] - (RW) XTAL Input Configuration for OCC mode (when RG_OCC_PLL_SW = 1'b1)
                                     000: 26MHz
                                     001: 25MHz
                                     010: 40MHz
                                     011: 24MHz
                                     1XX: NA
    CR_OCC_PLL_SW[31]            - (RW) BG/XBUF/XTAL_CFG/BPLL/WBPLL/WPLL source switch
                                     0: Source comes from normal path
                                     1: Source comes from reserved path (as shown below)

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_CR_OCC_PLL_SW_ADDR         CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_CR_OCC_PLL_SW_MASK         0x80000000                // CR_OCC_PLL_SW[31]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_CR_OCC_PLL_SW_SHFT         31
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_AFE_XTAL_CFG_OCC_ADDR CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_AFE_XTAL_CFG_OCC_MASK 0x70000000                // RG_WBG_AFE_XTAL_CFG_OCC[30..28]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_AFE_XTAL_CFG_OCC_SHFT 28
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_BPLL_ADDR     CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_BPLL_MASK     0x08000000                // RG_WBG_CG_EN_BPLL[27]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_BPLL_SHFT     27
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_WPLL640_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_WPLL640_MASK  0x04000000                // RG_WBG_CG_EN_WPLL640[26]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_WPLL640_SHFT  26
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_WPLL960_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_WPLL960_MASK  0x02000000                // RG_WBG_CG_EN_WPLL960[25]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_WPLL960_SHFT  25
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV4_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV4_MASK   0x01000000                // RG_WBG_CTL_OCC_RSV4[24]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV4_SHFT   24
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_GPS_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_GPS_MASK   0x00800000                // RG_WBG_CG_EN_AD_GPS[23]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_GPS_SHFT   23
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV3_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV3_MASK   0x00400000                // RG_WBG_CTL_OCC_RSV3[22]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV3_SHFT   22
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV2_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV2_MASK   0x00300000                // RG_WBG_CTL_OCC_RSV2[21..20]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV2_SHFT   20
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_BT0_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_BT0_MASK   0x00080000                // RG_WBG_CG_EN_AD_BT0[19]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_BT0_SHFT   19
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_BT0_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_BT0_MASK   0x00040000                // RG_WBG_CG_EN_DA_BT0[18]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_BT0_SHFT   18
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_BT1_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_BT1_MASK   0x00020000                // RG_WBG_CG_EN_AD_BT1[17]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_BT1_SHFT   17
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_BT1_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_BT1_MASK   0x00010000                // RG_WBG_CG_EN_DA_BT1[16]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_BT1_SHFT   16
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_WF_ADDR    CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_WF_MASK    0x00008000                // RG_WBG_CG_EN_AD_WF[15]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_WF_SHFT    15
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_WF_ADDR    CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_WF_MASK    0x00004000                // RG_WBG_CG_EN_DA_WF[14]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_DA_WF_SHFT    14
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_DFS_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_DFS_MASK   0x00002000                // RG_WBG_CG_EN_AD_DFS[13]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_DFS_SHFT   13
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_WF0TSSI_ADDR CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_WF0TSSI_MASK 0x00001000                // RG_WBG_CG_EN_AD_WF0TSSI[12]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_WF0TSSI_SHFT 12
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WBPLL_OCC_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WBPLL_OCC_MASK   0x00000800                // RG_WBG_EN_WBPLL_OCC[11]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WBPLL_OCC_SHFT   11
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV1_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV1_MASK   0x000007F0                // RG_WBG_CTL_OCC_RSV1[10..4]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CTL_OCC_RSV1_SHFT   4
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BG_OCC_ADDR      CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BG_OCC_MASK      0x00000008                // RG_WBG_EN_BG_OCC[3]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BG_OCC_SHFT      3
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_XBUF_OCC_ADDR    CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_XBUF_OCC_MASK    0x00000004                // RG_WBG_EN_XBUF_OCC[2]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_XBUF_OCC_SHFT    2
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BPLL_OCC_ADDR    CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BPLL_OCC_MASK    0x00000002                // RG_WBG_EN_BPLL_OCC[1]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BPLL_OCC_SHFT    1
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WPLL_OCC_ADDR    CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WPLL_OCC_MASK    0x00000001                // RG_WBG_EN_WPLL_OCC[0]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WPLL_OCC_SHFT    0

/* =====================================================================================

  ---RG_PLL_RFSOC_S (0x18041000 + 0x128)---

    RG_WBG_WBPLL_STB_TIME[14..0] - (RW) Default : 788 osc_ck cycle
                                     (about 30us + 1 % margin)
    RESERVED15[15]               - (RO) Reserved bits
    RG_WBG_EN_WBPLL[16]          - (RW) Enable WBPLL control in manual mode
                                     0: Disable
                                     1: Enable
    RESERVED17[31..17]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_RG_WBG_EN_WBPLL_ADDR       CONN_AFE_CTL_RG_PLL_RFSOC_S_ADDR
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_RG_WBG_EN_WBPLL_MASK       0x00010000                // RG_WBG_EN_WBPLL[16]
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_RG_WBG_EN_WBPLL_SHFT       16
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_RG_WBG_WBPLL_STB_TIME_ADDR CONN_AFE_CTL_RG_PLL_RFSOC_S_ADDR
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_RG_WBG_WBPLL_STB_TIME_MASK 0x00007FFF                // RG_WBG_WBPLL_STB_TIME[14..0]
#define CONN_AFE_CTL_RG_PLL_RFSOC_S_RG_WBG_WBPLL_STB_TIME_SHFT 0

/* =====================================================================================

  ---RG_PLL_RFSOC_M (0x18041000 + 0x12C)---

    RGS_WBG_WBPLL_RO[11..0]      - (RO) RGS reserve for WBPLL
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_PLL_RFSOC_M_RGS_WBG_WBPLL_RO_ADDR      CONN_AFE_CTL_RG_PLL_RFSOC_M_ADDR
#define CONN_AFE_CTL_RG_PLL_RFSOC_M_RGS_WBG_WBPLL_RO_MASK      0x00000FFF                // RGS_WBG_WBPLL_RO[11..0]
#define CONN_AFE_CTL_RG_PLL_RFSOC_M_RGS_WBG_WBPLL_RO_SHFT      0

/* =====================================================================================

  ---RG_WBG_RFSOC_0 (0x18041000 + 0x130)---

    RESERVED0[27..0]             - (RO) Reserved bits
    RG_WBG_BT1_ADCKSEL_[29..28]  - (RW) RFSOC BT1 ADC clock selection in manual mode
                                     00: 32MHz
                                     01: 64MHz
                                     10: 16MHz
    RG_WBG_BT0_ADCKSEL_[31..30]  - (RW) RFSOC BT0 ADC clock selection in manual mode
                                     00: 32MHz
                                     01: 64MHz
                                     10: 16MHz

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_RG_WBG_BT0_ADCKSEL__ADDR   CONN_AFE_CTL_RG_WBG_RFSOC_0_ADDR
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_RG_WBG_BT0_ADCKSEL__MASK   0xC0000000                // RG_WBG_BT0_ADCKSEL_[31..30]
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_RG_WBG_BT0_ADCKSEL__SHFT   30
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_RG_WBG_BT1_ADCKSEL__ADDR   CONN_AFE_CTL_RG_WBG_RFSOC_0_ADDR
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_RG_WBG_BT1_ADCKSEL__MASK   0x30000000                // RG_WBG_BT1_ADCKSEL_[29..28]
#define CONN_AFE_CTL_RG_WBG_RFSOC_0_RG_WBG_BT1_ADCKSEL__SHFT   28

/* =====================================================================================

  ---RG_WBG_RFSOC_MB00 (0x18041000 + 0x134)---

    RG_WBG_BT0_AFE_01[31..0]     - (RW) BTRF0 AFE 01

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB00_RG_WBG_BT0_AFE_01_ADDR  CONN_AFE_CTL_RG_WBG_RFSOC_MB00_ADDR
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB00_RG_WBG_BT0_AFE_01_MASK  0xFFFFFFFF                // RG_WBG_BT0_AFE_01[31..0]
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB00_RG_WBG_BT0_AFE_01_SHFT  0

/* =====================================================================================

  ---RG_WBG_RFSOC_MB10 (0x18041000 + 0x13C)---

    RG_WBG_BT1_AFE_01[31..0]     - (RW) BTRF1 AFE 01

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB10_RG_WBG_BT1_AFE_01_ADDR  CONN_AFE_CTL_RG_WBG_RFSOC_MB10_ADDR
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB10_RG_WBG_BT1_AFE_01_MASK  0xFFFFFFFF                // RG_WBG_BT1_AFE_01[31..0]
#define CONN_AFE_CTL_RG_WBG_RFSOC_MB10_RG_WBG_BT1_AFE_01_SHFT  0

#endif // __CONN_AFE_CTL_REGS_H__
