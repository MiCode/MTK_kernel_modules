/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
//[File]            : conn_afe_ctl.h
//[Revision time]   : Thu Aug 31 15:23:28 2023

#ifndef __CONN_AFE_CTL_REGS_H__
#define __CONN_AFE_CTL_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif


//****************************************************************************
//
//                     CONN_AFE_CTL CR Definitions                     
//
//****************************************************************************

#define CONN_AFE_CTL_BASE                                      (CONN_AFE_CTL_BASE_ADDR_MT6991) // 0x40041000

#define CONN_AFE_CTL_RG_DIG_EN_01_ADDR                         (CONN_AFE_CTL_BASE + 0x000) // 1000
#define CONN_AFE_CTL_RG_DIG_EN_02_ADDR                         (CONN_AFE_CTL_BASE + 0x004) // 1004
#define CONN_AFE_CTL_RG_DIG_EN_03_ADDR                         (CONN_AFE_CTL_BASE + 0x008) // 1008
#define CONN_AFE_CTL_RG_DIG_TOP_01_ADDR                        (CONN_AFE_CTL_BASE + 0x00C) // 100C
#define CONN_AFE_CTL_RG_WBG_AFE_01_ADDR                        (CONN_AFE_CTL_BASE + 0x010) // 1010
#define CONN_AFE_CTL_RG_WBG_AFE_02_ADDR                        (CONN_AFE_CTL_BASE + 0x014) // 1014
#define CONN_AFE_CTL_RG_WBG_AFE_03_ADDR                        (CONN_AFE_CTL_BASE + 0x018) // 1018
#define CONN_AFE_CTL_RG_WBG_AFE_04_ADDR                        (CONN_AFE_CTL_BASE + 0x01C) // 101C
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
#define CONN_AFE_CTL_RG_RXDS_FLT_01_ADDR                       (CONN_AFE_CTL_BASE + 0x0D8) // 10D8
#define CONN_AFE_CTL_RG_RXDS_FLT_02_ADDR                       (CONN_AFE_CTL_BASE + 0x0DC) // 10DC
#define CONN_AFE_CTL_RG_SIN_GEN_01_ADDR                        (CONN_AFE_CTL_BASE + 0x0E0) // 10E0
#define CONN_AFE_CTL_RG_SIN_GEN_02_ADDR                        (CONN_AFE_CTL_BASE + 0x0E4) // 10E4
#define CONN_AFE_CTL_RG_SIN_GEN_03_ADDR                        (CONN_AFE_CTL_BASE + 0x0E8) // 10E8
#define CONN_AFE_CTL_RG_SIN_GEN_04_ADDR                        (CONN_AFE_CTL_BASE + 0x0EC) // 10EC
#define CONN_AFE_CTL_RG_SIN_GEN_05_ADDR                        (CONN_AFE_CTL_BASE + 0x0F0) // 10F0
#define CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR                      (CONN_AFE_CTL_BASE + 0x0F4) // 10F4
#define CONN_AFE_CTL_RG_WBG_GL5_01_ADDR                        (CONN_AFE_CTL_BASE + 0x100) // 1100
#define CONN_AFE_CTL_RG_WBG_GL5_02_ADDR                        (CONN_AFE_CTL_BASE + 0x104) // 1104
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR                       (CONN_AFE_CTL_BASE + 0x124) // 1124
#define CONN_AFE_CTL_RG_DIG_EN_04_ADDR                         (CONN_AFE_CTL_BASE + 0x015C) // 115C
#define CONN_AFE_CTL_RG_WBG_AFE_GPIO_RO_ADDR                   (CONN_AFE_CTL_BASE + 0x160) // 1160
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ADDR                       (CONN_AFE_CTL_BASE + 0x0164) // 1164




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
    RG_WBG_TSTVBG_EN[9]          - (RW) Enable global switch for QS_VMON_VBG
                                     0: Disable 
                                     1: Enable
    RG_WBG_TSTPAD2CK_EN[10]      - (RW) Enable global ext-CK SW for NS_VMON_PAD2CK
                                     0: Disable
                                     1: Enable
    RG_WBG_TSTPAD_HZ[11]         - (RW) Release GL1 Q  input PAD to HZ mode due to share testing PAD
                                     0: Normal mode (RX mode)
                                     1: HZ mode (testing mode)
    RG_TSTCK_XTAL_EN[12]         - (RW) Enable global open drain for NS_VMON_CK2PAD and mux to CKSQ
                                     0: Disable
                                     1: Enable
    RG_TSTCK_BPLL_EN[13]         - (RW) Enable global open drain for NS_VMON_CK2PAD and mux to BPLL
                                     0: Disable
                                     1: Enable
    RG_TST_ADA_EN[14]            - (RW) Enable AGPIO                                                                         0:Disable                                                                           1:Enable
    RG_WBG_AFE_RSV1[24..15]      - (RW) RSV
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
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV1_MASK        0x01FF8000                // RG_WBG_AFE_RSV1[24..15]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_AFE_RSV1_SHFT        15
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TST_ADA_EN_ADDR          CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TST_ADA_EN_MASK          0x00004000                // RG_TST_ADA_EN[14]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TST_ADA_EN_SHFT          14
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TSTCK_BPLL_EN_ADDR       CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TSTCK_BPLL_EN_MASK       0x00002000                // RG_TSTCK_BPLL_EN[13]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TSTCK_BPLL_EN_SHFT       13
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TSTCK_XTAL_EN_ADDR       CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TSTCK_XTAL_EN_MASK       0x00001000                // RG_TSTCK_XTAL_EN[12]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_TSTCK_XTAL_EN_SHFT       12
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD_HZ_ADDR       CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD_HZ_MASK       0x00000800                // RG_WBG_TSTPAD_HZ[11]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD_HZ_SHFT       11
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD2CK_EN_ADDR    CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD2CK_EN_MASK    0x00000400                // RG_WBG_TSTPAD2CK_EN[10]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTPAD2CK_EN_SHFT    10
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTVBG_EN_ADDR       CONN_AFE_CTL_RG_WBG_AFE_01_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTVBG_EN_MASK       0x00000200                // RG_WBG_TSTVBG_EN[9]
#define CONN_AFE_CTL_RG_WBG_AFE_01_RG_WBG_TSTVBG_EN_SHFT       9
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

  ---RG_WBG_AFE_03 (0x18041000 + 0x018)---

    RG_WBG_CLKSQ_MANSEL_EN[0]    - (RW) Enable manual mode 
                                     1'b0: Disable  (control by DA_WBG_EN_B/WPLL)
                                     1'b1: Enable
    RG_WBG_CLKSQ_CK2BWPLL_EN_force[1] - (RW) Force enable CK2BWPLL at manual mode
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WBG_CLKSQ_RSCH_SEL[3..2]  - (RW) select R_schmitt for window width
                                     2'b00: 7R
                                     2'b01: 8R
                                     2'b10: 5R
                                     2'b11: 6R
    RG_WBG_CLKSQ_MONDC_EN[4]     - (RW) monitor CLKSQ_LDO Voltage for debug
                                     1'b0: Disable
                                     1'b1: CLKSQ_LDO
    RG_WBG_CLKSQ_MONCK_EN[5]     - (RW) Enable monitor CLKSQ clock for debug
                                     1'b0: Disable
                                     1'b1: Enable
    RG_WBG_CLKSQ_LDO_VSEL[7..6]  - (RW) CLKSQ_LDO Voltage selection
                                     2'b00: ?V
                                     2'b01: ?V
                                     2'b10: ?V
                                     2'b11: ?V
    RG_WBG_RCK_RSV0[14..8]       - (RW) Reserve
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
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CALTARGET_ADDR   CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CALTARGET_MASK   0xFFC00000                // RG_WBG_RCK_CALTARGET[31..22]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CALTARGET_SHFT   22
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CALTARGET_MEN_ADDR CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CALTARGET_MEN_MASK 0x00200000                // RG_WBG_RCK_CALTARGET_MEN[21]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CALTARGET_MEN_SHFT 21
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CSEL_ADDR        CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CSEL_MASK        0x001F0000                // RG_WBG_RCK_CSEL[20..16]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CSEL_SHFT        16
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CSEL_MEN_ADDR    CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CSEL_MEN_MASK    0x00008000                // RG_WBG_RCK_CSEL_MEN[15]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_CSEL_MEN_SHFT    15
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_RSV0_ADDR        CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_RSV0_MASK        0x00007F00                // RG_WBG_RCK_RSV0[14..8]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_RCK_RSV0_SHFT        8
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_LDO_VSEL_ADDR  CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_LDO_VSEL_MASK  0x000000C0                // RG_WBG_CLKSQ_LDO_VSEL[7..6]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_LDO_VSEL_SHFT  6
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MONCK_EN_ADDR  CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MONCK_EN_MASK  0x00000020                // RG_WBG_CLKSQ_MONCK_EN[5]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MONCK_EN_SHFT  5
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MONDC_EN_ADDR  CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MONDC_EN_MASK  0x00000010                // RG_WBG_CLKSQ_MONDC_EN[4]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MONDC_EN_SHFT  4
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_RSCH_SEL_ADDR  CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_RSCH_SEL_MASK  0x0000000C                // RG_WBG_CLKSQ_RSCH_SEL[3..2]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_RSCH_SEL_SHFT  2
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_CK2BWPLL_EN_force_ADDR CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_CK2BWPLL_EN_force_MASK 0x00000002                // RG_WBG_CLKSQ_CK2BWPLL_EN_force[1]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_CK2BWPLL_EN_force_SHFT 1
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MANSEL_EN_ADDR CONN_AFE_CTL_RG_WBG_AFE_03_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MANSEL_EN_MASK 0x00000001                // RG_WBG_CLKSQ_MANSEL_EN[0]
#define CONN_AFE_CTL_RG_WBG_AFE_03_RG_WBG_CLKSQ_MANSEL_EN_SHFT 0

/* =====================================================================================

  ---RG_WBG_AFE_04 (0x18041000 + 0x01C)---

    RG_WBG_RCK_RSV2[15..0]       - (RW) Reserve
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_AFE_04_RG_WBG_RCK_RSV2_ADDR        CONN_AFE_CTL_RG_WBG_AFE_04_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_04_RG_WBG_RCK_RSV2_MASK        0x0000FFFF                // RG_WBG_RCK_RSV2[15..0]
#define CONN_AFE_CTL_RG_WBG_AFE_04_RG_WBG_RCK_RSV2_SHFT        0

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

    RG_IBAND_RGS_SEL[0]          - (RW) IBAND RGS SEL
                                     1'b0: RGS=CPLL IBAND<11:0>
                                     1'b1: RGS=WPLL IBAND<11:0>
    RG_CPLL_IBAND_MSB_EN[1]      - (RW) CPLL IBAND MSB EN
                                     1'b0: IBAND MSB = 0mA
                                     1'b1: IBAND MSB = 0.8mA
    RG_CPLL_IBAND_BWS_EN[2]      - (RW) CPLL IBAND BWS EN
    RG_CPLL_PARAMETER_MAN[3]     - (RW) CPLL parameter manul
                                     1'b0: control by default
                                     1'b1: control by RG
    RG_CPLL_BC_SEL[5..4]         - (RW) Set loop parameters Cz
                                     2'b00:  3pF
                                     2'b01:  4pF
                                     2'b10:  5pF
                                     2'b11:  6pF
    RG_CPLL_BPB_SEL[7..6]        - (RW) Set loop parameters CpB
                                     2'b00:  200fF
                                     ... (200fF/step)
                                     2'b11:  800fF
    RG_CPLL_BPA_SEL[10..8]       - (RW) Set loop parameters Cp
                                     3'b000:  40fF
                                     ... (40fF/step)
                                     3'b111:  320fF
    RG_CPLL_RST_DLYL_SEL[12..11] - (RW) CPLL_RST_DLYL_SEL  FC1+RSTB
                                     2'b00: 7T+9T
                                     2'b01: 15T+17T
                                     2'b10: 31T+33T
                                     2'b11: 63T+65T
    RG_CPLL_freq_sel[13]         - (RW) CPLL_freq_sel
                                     1'b0: 1664Mhz
                                     1'b1: 3328Mhz
    RG_CPLL_PREDIV_MAN[15..14]   - (RW) Pre divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b1x: /4
    RG_CPLL_FBKDIV_MAN[23..16]   - (RW) Set feedback divider ratio in digital macro
    RG_CPLL_FBSEL_MAN[24]        - (RW) Set feedback divider ratio in PLL
                                     1'b0: /1
                                     1'b1: /2
    RG_CPLL_BRSEL_MAN[27..25]    - (RW) Resistance adjustment for Bandwidth
                                     3'b000:  144K
                                     ... (12K/step)
                                     3'b111:  60K
    RG_CPLL_CPSEL_MAN[31..28]    - (RW) CHP current select
                                     4'b0001=5uA
                                     ...(5uA/step)
                                     4'b1111=75uA

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_CPSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_CPSEL_MAN_MASK      0xF0000000                // RG_CPLL_CPSEL_MAN[31..28]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_CPSEL_MAN_SHFT      28
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BRSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BRSEL_MAN_MASK      0x0E000000                // RG_CPLL_BRSEL_MAN[27..25]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BRSEL_MAN_SHFT      25
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_FBSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_FBSEL_MAN_MASK      0x01000000                // RG_CPLL_FBSEL_MAN[24]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_FBSEL_MAN_SHFT      24
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_FBKDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_FBKDIV_MAN_MASK     0x00FF0000                // RG_CPLL_FBKDIV_MAN[23..16]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_FBKDIV_MAN_SHFT     16
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_PREDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_PREDIV_MAN_MASK     0x0000C000                // RG_CPLL_PREDIV_MAN[15..14]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_PREDIV_MAN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_freq_sel_ADDR       CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_freq_sel_MASK       0x00002000                // RG_CPLL_freq_sel[13]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_freq_sel_SHFT       13
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_RST_DLYL_SEL_ADDR   CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_RST_DLYL_SEL_MASK   0x00001800                // RG_CPLL_RST_DLYL_SEL[12..11]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_RST_DLYL_SEL_SHFT   11
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BPA_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BPA_SEL_MASK        0x00000700                // RG_CPLL_BPA_SEL[10..8]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BPA_SEL_SHFT        8
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BPB_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BPB_SEL_MASK        0x000000C0                // RG_CPLL_BPB_SEL[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BPB_SEL_SHFT        6
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BC_SEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BC_SEL_MASK         0x00000030                // RG_CPLL_BC_SEL[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_BC_SEL_SHFT         4
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_PARAMETER_MAN_ADDR  CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_PARAMETER_MAN_MASK  0x00000008                // RG_CPLL_PARAMETER_MAN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_PARAMETER_MAN_SHFT  3
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_IBAND_BWS_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_IBAND_BWS_EN_MASK   0x00000004                // RG_CPLL_IBAND_BWS_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_IBAND_BWS_EN_SHFT   2
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_IBAND_MSB_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_IBAND_MSB_EN_MASK   0x00000002                // RG_CPLL_IBAND_MSB_EN[1]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_CPLL_IBAND_MSB_EN_SHFT   1
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_IBAND_RGS_SEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_01_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_IBAND_RGS_SEL_MASK       0x00000001                // RG_IBAND_RGS_SEL[0]
#define CONN_AFE_CTL_RG_WBG_PLL_01_RG_IBAND_RGS_SEL_SHFT       0

/* =====================================================================================

  ---RG_WBG_PLL_02 (0x18041000 + 0x028)---

    RG_CPLL_CK2PAD_SEL[1..0]     - (RW) CPLL CK2PAD SEL
                                     2'b00: OFF
                                     2'b01: CPLL
                                     2'b10: CPLL/2
                                     2'b11: CPLL/4
    RG_CPLL_DEBUG_SEL[2]         - (RW) CPLL DEBUG SEL
    RG_CPLL_LOAD_EN[3]           - (RW) CPLL_IBAND LOAD EN
    RG_CPLL_KBAND_PREDIV[5..4]   - (RW) Kband pre-divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b10: /4
                                     2'b11: /8
    RG_CPLL_KBAND_KFC[7..6]      - (RW) CPLL KBAND FC SEL
    RG_CPLL_RKICO_SEL[8]         - (RW) CPLL_RKICO_SEL
                                     1'b0: 5K
                                     1'b1: 10K
    RG_CPLL_DCMON_SEL[11..9]     - (RW) CPLL_DCMON_SEL
                                     3'b000:High Z
                                     3'b001: LDO_PFD
                                     3'b010: LDO_CHP
                                     3'b011: LDO_LPF
                                     3'b100: LDO_VCO
                                     3'b101: Vctrl
                                     3'b110: IBAND_20U25K
    RG_PLL_IBAND_LPBW_SEL[12]    - (RW) C/WPLL IBAND LPF BW select
                                     1'b0: Default LPF BW
                                     1'b1: LPF BW *2
    RG_PLL_TCL_EN[13]            - (RW) C/WPLL TCL EN
    RG_PLL_LDOVCO_SEL[15..14]    - (RW) C/WPLL LDO_VCO Voltage sel
                                     (same as LDO_PFD)
    RG_PLL_LDOLPF_SEL[17..16]    - (RW) C/WPLL LDO_LPF Voltage sel
                                     (same as LDO_PFD)
    RG_PLL_LDOCHP_SEL[19..18]    - (RW) C/WPLL LDO_CHP Voltage sel
                                     (same as LDO_PFD)
    RG_PLL_LDOPFD_SEL[21..20]    - (RW) C/WPLL LDO_PFD Voltage sel
                                     2'b00: 0.875
                                     2'b01: 0.900
                                     2'b10: 0.925
                                     2'b11: 0.950
    RG_PLL_IBAND[31..22]         - (RW) C/WPLL fixed IBAND

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_IBAND_ADDR           CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_IBAND_MASK           0xFFC00000                // RG_PLL_IBAND[31..22]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_IBAND_SHFT           22
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOPFD_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOPFD_SEL_MASK      0x00300000                // RG_PLL_LDOPFD_SEL[21..20]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOPFD_SEL_SHFT      20
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOCHP_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOCHP_SEL_MASK      0x000C0000                // RG_PLL_LDOCHP_SEL[19..18]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOCHP_SEL_SHFT      18
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOLPF_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOLPF_SEL_MASK      0x00030000                // RG_PLL_LDOLPF_SEL[17..16]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOLPF_SEL_SHFT      16
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOVCO_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOVCO_SEL_MASK      0x0000C000                // RG_PLL_LDOVCO_SEL[15..14]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_LDOVCO_SEL_SHFT      14
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_TCL_EN_ADDR          CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_TCL_EN_MASK          0x00002000                // RG_PLL_TCL_EN[13]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_TCL_EN_SHFT          13
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_IBAND_LPBW_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_IBAND_LPBW_SEL_MASK  0x00001000                // RG_PLL_IBAND_LPBW_SEL[12]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_PLL_IBAND_LPBW_SEL_SHFT  12
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_DCMON_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_DCMON_SEL_MASK      0x00000E00                // RG_CPLL_DCMON_SEL[11..9]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_DCMON_SEL_SHFT      9
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_RKICO_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_RKICO_SEL_MASK      0x00000100                // RG_CPLL_RKICO_SEL[8]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_RKICO_SEL_SHFT      8
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_KBAND_KFC_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_KBAND_KFC_MASK      0x000000C0                // RG_CPLL_KBAND_KFC[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_KBAND_KFC_SHFT      6
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_KBAND_PREDIV_ADDR   CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_KBAND_PREDIV_MASK   0x00000030                // RG_CPLL_KBAND_PREDIV[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_KBAND_PREDIV_SHFT   4
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_LOAD_EN_ADDR        CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_LOAD_EN_MASK        0x00000008                // RG_CPLL_LOAD_EN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_LOAD_EN_SHFT        3
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_DEBUG_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_DEBUG_SEL_MASK      0x00000004                // RG_CPLL_DEBUG_SEL[2]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_DEBUG_SEL_SHFT      2
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_CK2PAD_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_02_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_CK2PAD_SEL_MASK     0x00000003                // RG_CPLL_CK2PAD_SEL[1..0]
#define CONN_AFE_CTL_RG_WBG_PLL_02_RG_CPLL_CK2PAD_SEL_SHFT     0

/* =====================================================================================

  ---RG_WBG_PLL_03 (0x18041000 + 0x02C)---

    RG_PLL03_RSV02[0]            - (RW) RSV
    RG_WPLL_IBAND_MSB_EN[1]      - (RW) WPLL IBAND MSB EN
                                     1'b0: IBAND MSB = 0mA
                                     1'b1: IBAND MSB = 0.8mA
    RG_WPLL_IBAND_BWS_EN[2]      - (RW) WPLL IBAND BWS EN
    RG_WPLL_PARAMETER_MAN[3]     - (RW) CPLL parameter manul
                                     1'b0: control by default
                                     1'b1: control by RG
    RG_WPLL_BC_SEL[5..4]         - (RW) Set loop parameters Cz
                                     2'b00:  3pF
                                     2'b01:  4pF
                                     2'b10:  5pF
                                     2'b11:  6pF
    RG_WPLL_BPB_SEL[7..6]        - (RW) Set loop parameters CpB
                                     2'b00:  200fF
                                     ... (200fF/step)
                                     2'b11:  800fF
    RG_WPLL_BPA_SEL[10..8]       - (RW) Set loop parameters Cp
                                     3'b000:  40fF
                                     ... (40fF/step)
                                     3'b111:  320fF
    RG_WPLL_RST_DLYL_SEL[12..11] - (RW) WPLL_RST_DLYL_SEL  FC1+RSTB
                                     2'b00: 7T+9T
                                     2'b01: 15T+17T
                                     2'b10: 31T+33T
                                     2'b11: 63T+65T
    RG_PLL03_RSV01[13]           - (RW) RSV
    RG_WPLL_PREDIV_MAN[15..14]   - (RW) Pre divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b1x: /4
    RG_WPLL_FBKDIV_MAN[23..16]   - (RW) Set feedback divider ratio in digital macro
    RG_WPLL_FBSEL_MAN[24]        - (RW) Set feedback divider ratio in PLL
                                     1'b0: /1
                                     1'b1: /2
    RG_WPLL_BRSEL_MAN[27..25]    - (RW) Resistance adjustment for Bandwidth
                                     3'b000:  144K
                                     ... (12K/step)
                                     3'b111:  60K
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
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_PLL03_RSV01_ADDR         CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_PLL03_RSV01_MASK         0x00002000                // RG_PLL03_RSV01[13]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_PLL03_RSV01_SHFT         13
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_RST_DLYL_SEL_ADDR   CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_RST_DLYL_SEL_MASK   0x00001800                // RG_WPLL_RST_DLYL_SEL[12..11]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_RST_DLYL_SEL_SHFT   11
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPA_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPA_SEL_MASK        0x00000700                // RG_WPLL_BPA_SEL[10..8]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPA_SEL_SHFT        8
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPB_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPB_SEL_MASK        0x000000C0                // RG_WPLL_BPB_SEL[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BPB_SEL_SHFT        6
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BC_SEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BC_SEL_MASK         0x00000030                // RG_WPLL_BC_SEL[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_BC_SEL_SHFT         4
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_PARAMETER_MAN_ADDR  CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_PARAMETER_MAN_MASK  0x00000008                // RG_WPLL_PARAMETER_MAN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_PARAMETER_MAN_SHFT  3
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_IBAND_BWS_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_IBAND_BWS_EN_MASK   0x00000004                // RG_WPLL_IBAND_BWS_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_IBAND_BWS_EN_SHFT   2
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_IBAND_MSB_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_IBAND_MSB_EN_MASK   0x00000002                // RG_WPLL_IBAND_MSB_EN[1]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_WPLL_IBAND_MSB_EN_SHFT   1
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_PLL03_RSV02_ADDR         CONN_AFE_CTL_RG_WBG_PLL_03_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_PLL03_RSV02_MASK         0x00000001                // RG_PLL03_RSV02[0]
#define CONN_AFE_CTL_RG_WBG_PLL_03_RG_PLL03_RSV02_SHFT         0

/* =====================================================================================

  ---RG_WBG_PLL_04 (0x18041000 + 0x030)---

    RG_WPLL_CK2PAD_SEL[1..0]     - (RW) WPLL CK2PAD SEL
                                     2'b00: OFF
                                     2'b01: WPLL
                                     2'b10: WPLL/2
                                     2'b11: WPLL/4
    RG_WPLL_DEBUG_SEL[2]         - (RW) WPLL DEBUG SEL
    RG_WPLL_LOAD_EN[3]           - (RW) WPLL_IBAND LOAD EN
    RG_WPLL_KBAND_PREDIV[5..4]   - (RW) Kband pre-divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b10: /4
                                     2'b11: /8
    RG_WPLL_KBAND_KFC[7..6]      - (RW) WPLL KBAND FC SEL
    RG_WPLL_RKICO_SEL[8]         - (RW) WPLL_RKICO_SEL
                                     1'b0: 5K
                                     1'b1: 10K
    RG_WPLL_DCMON_SEL[11..9]     - (RW) CPLL_DCMON_SEL
                                     3'b000:High Z
                                     3'b001: LDO_PFD
                                     3'b010: LDO_CHP
                                     3'b011: LDO_LPF
                                     3'b100: LDO_VCO
                                     3'b101: Vctrl
                                     3'b110: IBAND_20U25K
    RG_WPLL_PAD2CK_AC_EN[12]     - (RW) WPLL PAD2CK AC Enable
                                     1'b0: PAD2CK DC couple
                                     1'b1: PAD2CK AC couple
    RG_WPLL_PAD2CK_EN[13]        - (RW) WPLL PAD2CK Enable
    RG_CASCADE_EN_DLY_SEL[15..14] - (RW) Cascade PLL enable delay SEL
                                     2'b00: 127T
                                     2'b01: 255T
                                     2'b10: 511T
                                     2'b11: 1023T
    RG_CWPLL_EN_MAN_SEL[16]      - (RW) Enable manual select
    RG_CPLL_MAN_EN[17]           - (RW) CPLL manual enable
    RG_WPLL_MAN_EN[18]           - (RW) WPLL manual enable
    RG_PLL04_RSV[19]             - (RW) RSV
    RG_WPLL_DCTST_EN[20]         - (RW) Enable WPLL DC test
    RG_PLLDIV_DCMON_SEL[23..21]  - (RW) WPLL_DCMON_SEL
                                     3'b000:High Z
                                     3'b001: LDO_WRX
                                     3'b010: LDO_WRX_VALID
                                     3'b011: LDO_WTX
                                     3'b100: LDO_WTX_VALID
                                     3'b101: REF_0P65
    RG_PLLDIV_LDO_SEL[25..24]    - (RW) PLLDIV LDO Voltage sel
                                     2'b00: 0.875
                                     2'b01: 0.900
                                     2'b10: 0.925
                                     2'b11: 0.950
    RG_PLLDIV_DIV2_EN[26]        - (RW) WPLL_74_72 control by DIV
                                     1'b0: TRX_DIV /1
                                     1'b1:TRX_DIV /2
    RG_WPLL_74_72[27]            - (RW) WPLL_74_72 control by ICO
                                     1'b0: ICO@1920Mhz 
                                     (need disable WPLL_IBAND_MSB)
                                     1'b1: ICO@3840Mhz
    RG_WFTXDIV_VALID4_EN[28]     - (RW) WFTXDIV VALID4 enable
    RG_WFRXDIV_VALID4_EN[29]     - (RW) WFRXDIV VALID4 enable
    RG_WFTXDIV_EN[30]            - (RW) WFTXDIV EN
                                     1'b0: DIV depend on TXCKO/TXK
                                     1'b1: always on
    RG_WFRXDIV_EN[31]            - (RW) WFRXDIV EN
                                     1'b0: DIV depend on RXCKO/R0AD
                                     1'b1: always on

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFRXDIV_EN_ADDR          CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFRXDIV_EN_MASK          0x80000000                // RG_WFRXDIV_EN[31]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFRXDIV_EN_SHFT          31
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFTXDIV_EN_ADDR          CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFTXDIV_EN_MASK          0x40000000                // RG_WFTXDIV_EN[30]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFTXDIV_EN_SHFT          30
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFRXDIV_VALID4_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFRXDIV_VALID4_EN_MASK   0x20000000                // RG_WFRXDIV_VALID4_EN[29]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFRXDIV_VALID4_EN_SHFT   29
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFTXDIV_VALID4_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFTXDIV_VALID4_EN_MASK   0x10000000                // RG_WFTXDIV_VALID4_EN[28]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WFTXDIV_VALID4_EN_SHFT   28
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_74_72_ADDR          CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_74_72_MASK          0x08000000                // RG_WPLL_74_72[27]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_74_72_SHFT          27
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_DIV2_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_DIV2_EN_MASK      0x04000000                // RG_PLLDIV_DIV2_EN[26]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_DIV2_EN_SHFT      26
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_LDO_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_LDO_SEL_MASK      0x03000000                // RG_PLLDIV_LDO_SEL[25..24]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_LDO_SEL_SHFT      24
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_DCMON_SEL_ADDR    CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_DCMON_SEL_MASK    0x00E00000                // RG_PLLDIV_DCMON_SEL[23..21]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLLDIV_DCMON_SEL_SHFT    21
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DCTST_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DCTST_EN_MASK       0x00100000                // RG_WPLL_DCTST_EN[20]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DCTST_EN_SHFT       20
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLL04_RSV_ADDR           CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLL04_RSV_MASK           0x00080000                // RG_PLL04_RSV[19]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_PLL04_RSV_SHFT           19
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MAN_EN_ADDR         CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MAN_EN_MASK         0x00040000                // RG_WPLL_MAN_EN[18]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_MAN_EN_SHFT         18
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CPLL_MAN_EN_ADDR         CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CPLL_MAN_EN_MASK         0x00020000                // RG_CPLL_MAN_EN[17]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CPLL_MAN_EN_SHFT         17
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CWPLL_EN_MAN_SEL_ADDR    CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CWPLL_EN_MAN_SEL_MASK    0x00010000                // RG_CWPLL_EN_MAN_SEL[16]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CWPLL_EN_MAN_SEL_SHFT    16
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CASCADE_EN_DLY_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CASCADE_EN_DLY_SEL_MASK  0x0000C000                // RG_CASCADE_EN_DLY_SEL[15..14]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_CASCADE_EN_DLY_SEL_SHFT  14
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_PAD2CK_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_PAD2CK_EN_MASK      0x00002000                // RG_WPLL_PAD2CK_EN[13]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_PAD2CK_EN_SHFT      13
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_PAD2CK_AC_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_PAD2CK_AC_EN_MASK   0x00001000                // RG_WPLL_PAD2CK_AC_EN[12]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_PAD2CK_AC_EN_SHFT   12
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DCMON_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DCMON_SEL_MASK      0x00000E00                // RG_WPLL_DCMON_SEL[11..9]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DCMON_SEL_SHFT      9
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RKICO_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RKICO_SEL_MASK      0x00000100                // RG_WPLL_RKICO_SEL[8]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_RKICO_SEL_SHFT      8
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_KFC_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_KFC_MASK      0x000000C0                // RG_WPLL_KBAND_KFC[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_KFC_SHFT      6
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_PREDIV_ADDR   CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_PREDIV_MASK   0x00000030                // RG_WPLL_KBAND_PREDIV[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_KBAND_PREDIV_SHFT   4
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LOAD_EN_ADDR        CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LOAD_EN_MASK        0x00000008                // RG_WPLL_LOAD_EN[3]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_LOAD_EN_SHFT        3
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DEBUG_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DEBUG_SEL_MASK      0x00000004                // RG_WPLL_DEBUG_SEL[2]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_DEBUG_SEL_SHFT      2
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_CK2PAD_SEL_ADDR     CONN_AFE_CTL_RG_WBG_PLL_04_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_CK2PAD_SEL_MASK     0x00000003                // RG_WPLL_CK2PAD_SEL[1..0]
#define CONN_AFE_CTL_RG_WBG_PLL_04_RG_WPLL_CK2PAD_SEL_SHFT     0

/* =====================================================================================

  ---RG_WBG_PLL_05 (0x18041000 + 0x034)---

    RG_BPLL_MANSEL_EN[0]         - (RW) Enable RG Manual mode
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_RLPSEL[1]            - (RW) IBAND Rmos select
                                     1'b0: BW*1
                                     1'b1: BW*2
    RG_BPLL_TCL_EN[2]            - (RW) Enable TCL
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_RICO_SEL[3]          - (RW) Rkico selection
                                     1'b0: 10K
                                     1'b1: 15K
    RG_BPLL_KBAND_PREDIV[5..4]   - (RW) Kband pre-divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b10: /4
                                     2'b11: /8
    RG_BPLL_LDO_SEL[7..6]        - (RW) Core+DIV_LDO Voltage selection
                                     2'b00: 0.825V
                                     2'b01: 0.850V
                                     2'b10: 0.875V
                                     2'b11: 0.900V
    RG_BPLL_V2I_ISEL[9..8]       - (RW) Set V2I current
                                     2'b01:
    RG_BPLL_V2I_VSEL[11..10]     - (RW) Set V2I voltage
                                     2'b01:
    RG_BPLL_ALPHA_SEL[12]        - (RW) ALPHA select
                                     1'b0 : alpha=2
                                     1'b1 : alpha=1
    RG_BPLL_RST_DLY[13]          - (RW) PLL reset time selection
                                     1'b0: 16*T_dig
                                     1'b1: 64*T_dig
    RG_BPLL_FBKDIV_MAN[19..14]   - (RW) Set feedback divider ratio in digital macro
    RG_BPLL_FBSEL_MAN[20]        - (RW) Set fb pre-divider ratio in PLL
                                     1'b0:  /1
                                     1'b1:  /2
    RG_BPLL_BPB_SEL[22..21]      - (RW) Set loop parameters CpB
                                     2'b00:  200fF
                                     ... (200fF/step)
                                     2'b11:  800fF
    RG_BPLL_BPA_SEL[24..23]      - (RW) Set loop parameters Cp
                                     2'b00:  40fF
                                     ... (40fF/step)
                                     2'b11:  160fF
    RG_BPLL_BR_SEL[26..25]       - (RW) Resistance adjustment for Bandwidth
                                     2'b00:  150K
                                     ... (15K/step)
                                     2'b11:  105K
    RG_BPLL_PREDIV_MAN[27]       - (RW) Pre divider ratio
                                     1'b0: /1
                                     1'b'1:/2
    RG_BPLL_CPSEL_MAN_1[31..28]  - (RW) CHP current select
                                     4'b0001=2.5uA
                                     4'b0010=5uA
                                     ...(2.5u/step)
                                     4'b1111=37.5uA

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_CPSEL_MAN_1_ADDR    CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_CPSEL_MAN_1_MASK    0xF0000000                // RG_BPLL_CPSEL_MAN_1[31..28]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_CPSEL_MAN_1_SHFT    28
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_PREDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_PREDIV_MAN_MASK     0x08000000                // RG_BPLL_PREDIV_MAN[27]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_PREDIV_MAN_SHFT     27
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BR_SEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BR_SEL_MASK         0x06000000                // RG_BPLL_BR_SEL[26..25]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BR_SEL_SHFT         25
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BPA_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BPA_SEL_MASK        0x01800000                // RG_BPLL_BPA_SEL[24..23]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BPA_SEL_SHFT        23
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BPB_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BPB_SEL_MASK        0x00600000                // RG_BPLL_BPB_SEL[22..21]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_BPB_SEL_SHFT        21
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_FBSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_FBSEL_MAN_MASK      0x00100000                // RG_BPLL_FBSEL_MAN[20]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_FBSEL_MAN_SHFT      20
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_FBKDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_FBKDIV_MAN_MASK     0x000FC000                // RG_BPLL_FBKDIV_MAN[19..14]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_FBKDIV_MAN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RST_DLY_ADDR        CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RST_DLY_MASK        0x00002000                // RG_BPLL_RST_DLY[13]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RST_DLY_SHFT        13
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_ALPHA_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_ALPHA_SEL_MASK      0x00001000                // RG_BPLL_ALPHA_SEL[12]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_ALPHA_SEL_SHFT      12
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_V2I_VSEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_V2I_VSEL_MASK       0x00000C00                // RG_BPLL_V2I_VSEL[11..10]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_V2I_VSEL_SHFT       10
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_V2I_ISEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_V2I_ISEL_MASK       0x00000300                // RG_BPLL_V2I_ISEL[9..8]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_V2I_ISEL_SHFT       8
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_LDO_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_LDO_SEL_MASK        0x000000C0                // RG_BPLL_LDO_SEL[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_LDO_SEL_SHFT        6
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_KBAND_PREDIV_ADDR   CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_KBAND_PREDIV_MASK   0x00000030                // RG_BPLL_KBAND_PREDIV[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_KBAND_PREDIV_SHFT   4
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RICO_SEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RICO_SEL_MASK       0x00000008                // RG_BPLL_RICO_SEL[3]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RICO_SEL_SHFT       3
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_TCL_EN_ADDR         CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_TCL_EN_MASK         0x00000004                // RG_BPLL_TCL_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_TCL_EN_SHFT         2
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RLPSEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RLPSEL_MASK         0x00000002                // RG_BPLL_RLPSEL[1]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_RLPSEL_SHFT         1
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_MANSEL_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_05_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_MANSEL_EN_MASK      0x00000001                // RG_BPLL_MANSEL_EN[0]
#define CONN_AFE_CTL_RG_WBG_PLL_05_RG_BPLL_MANSEL_EN_SHFT      0

/* =====================================================================================

  ---RG_WBG_PLL_06 (0x18041000 + 0x038)---

    RG_LPLL_MANSEL_EN[0]         - (RW) Enable RG Manual mode
                                     1'b0: Disable
                                     1'b1: Enable
    RG_LPLL_RLPSEL[1]            - (RW) IBAND Rmos select
                                     1'b0: BW*1
                                     1'b1: BW*2
    RG_LPLL_TCL_EN[2]            - (RW) Enable TCL
                                     1'b0: Disable
                                     1'b1: Enable
    RG_LPLL_RICO_SEL[3]          - (RW) Rkico selection
                                     1'b0: 10K
                                     1'b1: 15K
    RG_LPLL_KBAND_PREDIV[5..4]   - (RW) Kband pre-divider ratio
                                     2'b00: /1
                                     2'b'01:/2
                                     2'b10: /4
                                     2'b11: /8
    RG_LPLL_LDO_SEL[7..6]        - (RW) Core+DIV_LDO Voltage selection
                                     2'b00: 0.825V
                                     2'b01: 0.850V
                                     2'b10: 0.875V
                                     2'b11: 0.900V
    RG_LPLL_V2I_ISEL[9..8]       - (RW) Set V2I current
                                     2'b01:
    RG_LPLL_V2I_VSEL[11..10]     - (RW) Set V2I voltage
                                     2'b01:
    RG_LPLL_ALPHA_SEL[12]        - (RW) ALPHA select
                                     1'b0 : alpha=2
                                     1'b1 : alpha=1
    RG_LPLL_RST_DLY[13]          - (RW) PLL reset time selection
                                     1'b0: 16*T_dig
                                     1'b1: 64*T_dig
    RG_LPLL_FBKDIV_MAN[19..14]   - (RW) Set feedback divider ratio in digital macro
    RG_LPLL_FBSEL_MAN[20]        - (RW) Set fb pre-divider ratio in PLL
                                     1'b0:  /1
                                     1'b1:  /2
    RG_LPLL_BPB_SEL[22..21]      - (RW) Set loop parameters CpB
                                     2'b00:  200fF
                                     ... (200fF/step)
                                     2'b11:  800fF
    RG_LPLL_BPA_SEL[24..23]      - (RW) Set loop parameters Cp
                                     2'b00:  40fF
                                     ... (40fF/step)
                                     2'b11:  160fF
    RG_LPLL_BR_SEL[26..25]       - (RW) Resistance adjustment for Bandwidth
                                     2'b00:  150K
                                     ... (15K/step)
                                     2'b11:  105K
    RG_LPLL_PREDIV_MAN[27]       - (RW) Pre divider ratio
                                     1'b0: /1
                                     1'b'1:/2
    RG_LPLL_CPSEL_MAN_1[31..28]  - (RW) CHP current select
                                     4'b0001=2.5uA
                                     4'b0010=5uA
                                     ...(2.5u/step)
                                     4'b1111=37.5uA

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_CPSEL_MAN_1_ADDR    CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_CPSEL_MAN_1_MASK    0xF0000000                // RG_LPLL_CPSEL_MAN_1[31..28]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_CPSEL_MAN_1_SHFT    28
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_PREDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_PREDIV_MAN_MASK     0x08000000                // RG_LPLL_PREDIV_MAN[27]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_PREDIV_MAN_SHFT     27
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BR_SEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BR_SEL_MASK         0x06000000                // RG_LPLL_BR_SEL[26..25]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BR_SEL_SHFT         25
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BPA_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BPA_SEL_MASK        0x01800000                // RG_LPLL_BPA_SEL[24..23]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BPA_SEL_SHFT        23
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BPB_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BPB_SEL_MASK        0x00600000                // RG_LPLL_BPB_SEL[22..21]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_BPB_SEL_SHFT        21
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_FBSEL_MAN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_FBSEL_MAN_MASK      0x00100000                // RG_LPLL_FBSEL_MAN[20]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_FBSEL_MAN_SHFT      20
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_FBKDIV_MAN_ADDR     CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_FBKDIV_MAN_MASK     0x000FC000                // RG_LPLL_FBKDIV_MAN[19..14]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_FBKDIV_MAN_SHFT     14
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RST_DLY_ADDR        CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RST_DLY_MASK        0x00002000                // RG_LPLL_RST_DLY[13]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RST_DLY_SHFT        13
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_ALPHA_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_ALPHA_SEL_MASK      0x00001000                // RG_LPLL_ALPHA_SEL[12]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_ALPHA_SEL_SHFT      12
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_V2I_VSEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_V2I_VSEL_MASK       0x00000C00                // RG_LPLL_V2I_VSEL[11..10]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_V2I_VSEL_SHFT       10
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_V2I_ISEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_V2I_ISEL_MASK       0x00000300                // RG_LPLL_V2I_ISEL[9..8]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_V2I_ISEL_SHFT       8
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_LDO_SEL_ADDR        CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_LDO_SEL_MASK        0x000000C0                // RG_LPLL_LDO_SEL[7..6]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_LDO_SEL_SHFT        6
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_KBAND_PREDIV_ADDR   CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_KBAND_PREDIV_MASK   0x00000030                // RG_LPLL_KBAND_PREDIV[5..4]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_KBAND_PREDIV_SHFT   4
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RICO_SEL_ADDR       CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RICO_SEL_MASK       0x00000008                // RG_LPLL_RICO_SEL[3]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RICO_SEL_SHFT       3
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_TCL_EN_ADDR         CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_TCL_EN_MASK         0x00000004                // RG_LPLL_TCL_EN[2]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_TCL_EN_SHFT         2
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RLPSEL_ADDR         CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RLPSEL_MASK         0x00000002                // RG_LPLL_RLPSEL[1]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_RLPSEL_SHFT         1
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_MANSEL_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_06_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_MANSEL_EN_MASK      0x00000001                // RG_LPLL_MANSEL_EN[0]
#define CONN_AFE_CTL_RG_WBG_PLL_06_RG_LPLL_MANSEL_EN_SHFT      0

/* =====================================================================================

  ---RG_WBG_PLL_07 (0x18041000 + 0x03C)---

    RG_BPLL_TSTCK_SEL[0]         - (RW) PAD2CK 0:DIVCK  1:PLLCK
    RG_BPLL_TSTCK_EN[1]          - (RW) BPLL CK2PAD enable
    RG_BPLL_GB128M_force_en[2]   - (RW) BTDA/GPS CK128M force enable
    RG_BPLL_BLADDIV_forec_en[3]  - (RW) BTAD/LRAD DIVCK force enable
    RG_BPLL_PAD2CK_AC_EN[4]      - (RW) External CK AC enable
    RG_BPLL_PAD2CK_EN[5]         - (RW) External CK enable
    RG_BPLL_DCMON_EN[6]          - (RW) BPLL DC monitor enable
    RG_BPLL_DIG_DEBUG_SEL[7]     - (RW) read RGS selection
    RG_BPLL_DIG_LOAD_EN[8]       - (RW) Enable load IBAND
                                     1'b0: Disable
                                     1'b1: Enable
    RG_BPLL_FC_DLY[10..9]        - (RW) PLL Fast Charge time selection
    RG_LPLL_TSTCK_SEL[11]        - (RW) PAD2CK 0:DIVCK  1:PLLCK
    RG_LPLL_TSTCK_EN[12]         - (RW) LPLL CK2PAD enable
    RG_LPLL_GB128M_force_en[13]  - (RW) BTDA/GPS CK128M force enable
    RG_LPLL_BLADDIV_forec_en[14] - (RW) BTAD/LRAD DIVCK force enable
    RG_LPLL_PAD2CK_AC_EN[15]     - (RW) External CK AC enable
    RG_LPLL_PAD2CK_EN[16]        - (RW) External CK enable
    RG_LPLL_DCMON_EN[17]         - (RW) LPLL DC monitor enable
    RG_LPLL_DIG_DEBUG_SEL[18]    - (RW) read RGS selection
    RG_LPLL_DIG_LOAD_EN[19]      - (RW) Enable load IBAND
                                     1'b0: Disable
                                     1'b1: Enable
    RG_LPLL_FC_DLY[21..20]       - (RW) PLL Fast Charge time selection
    RG_BPLL_CKSQ_EDGE_SEL[22]    - (RW) CKSQ2PLL clock edge select
                                     0:rising edge
                                     1:falling edge
    RG_BLPLL_MON_LDO_EN[25..23]  - (RW) monitor BLPLL LDO Voltage for debug
                                     3'b000: Disable
                                     3'b001: LDO_PFD
                                     3'b010: LDO_VCO
                                     3'b011: LDO_LPF
                                     3'b100: VCTRL
                                     3'b101: N/A
                                     3'b110: N/A
                                     3'b111: LDO_BDIV
    RG_BLPLL_DIG_IBAND_MAN[31..26] - (RW) BLPLL IBAND loading code

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BLPLL_DIG_IBAND_MAN_ADDR CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BLPLL_DIG_IBAND_MAN_MASK 0xFC000000                // RG_BLPLL_DIG_IBAND_MAN[31..26]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BLPLL_DIG_IBAND_MAN_SHFT 26
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BLPLL_MON_LDO_EN_ADDR    CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BLPLL_MON_LDO_EN_MASK    0x03800000                // RG_BLPLL_MON_LDO_EN[25..23]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BLPLL_MON_LDO_EN_SHFT    23
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_CKSQ_EDGE_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_CKSQ_EDGE_SEL_MASK  0x00400000                // RG_BPLL_CKSQ_EDGE_SEL[22]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_CKSQ_EDGE_SEL_SHFT  22
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_FC_DLY_ADDR         CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_FC_DLY_MASK         0x00300000                // RG_LPLL_FC_DLY[21..20]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_FC_DLY_SHFT         20
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DIG_LOAD_EN_ADDR    CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DIG_LOAD_EN_MASK    0x00080000                // RG_LPLL_DIG_LOAD_EN[19]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DIG_LOAD_EN_SHFT    19
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DIG_DEBUG_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DIG_DEBUG_SEL_MASK  0x00040000                // RG_LPLL_DIG_DEBUG_SEL[18]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DIG_DEBUG_SEL_SHFT  18
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DCMON_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DCMON_EN_MASK       0x00020000                // RG_LPLL_DCMON_EN[17]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_DCMON_EN_SHFT       17
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_PAD2CK_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_PAD2CK_EN_MASK      0x00010000                // RG_LPLL_PAD2CK_EN[16]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_PAD2CK_EN_SHFT      16
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_PAD2CK_AC_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_PAD2CK_AC_EN_MASK   0x00008000                // RG_LPLL_PAD2CK_AC_EN[15]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_PAD2CK_AC_EN_SHFT   15
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_BLADDIV_forec_en_ADDR CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_BLADDIV_forec_en_MASK 0x00004000                // RG_LPLL_BLADDIV_forec_en[14]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_BLADDIV_forec_en_SHFT 14
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_GB128M_force_en_ADDR CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_GB128M_force_en_MASK 0x00002000                // RG_LPLL_GB128M_force_en[13]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_GB128M_force_en_SHFT 13
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_TSTCK_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_TSTCK_EN_MASK       0x00001000                // RG_LPLL_TSTCK_EN[12]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_TSTCK_EN_SHFT       12
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_TSTCK_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_TSTCK_SEL_MASK      0x00000800                // RG_LPLL_TSTCK_SEL[11]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_LPLL_TSTCK_SEL_SHFT      11
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_FC_DLY_ADDR         CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_FC_DLY_MASK         0x00000600                // RG_BPLL_FC_DLY[10..9]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_FC_DLY_SHFT         9
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DIG_LOAD_EN_ADDR    CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DIG_LOAD_EN_MASK    0x00000100                // RG_BPLL_DIG_LOAD_EN[8]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DIG_LOAD_EN_SHFT    8
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DIG_DEBUG_SEL_ADDR  CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DIG_DEBUG_SEL_MASK  0x00000080                // RG_BPLL_DIG_DEBUG_SEL[7]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DIG_DEBUG_SEL_SHFT  7
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DCMON_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DCMON_EN_MASK       0x00000040                // RG_BPLL_DCMON_EN[6]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_DCMON_EN_SHFT       6
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_PAD2CK_EN_ADDR      CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_PAD2CK_EN_MASK      0x00000020                // RG_BPLL_PAD2CK_EN[5]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_PAD2CK_EN_SHFT      5
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_PAD2CK_AC_EN_ADDR   CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_PAD2CK_AC_EN_MASK   0x00000010                // RG_BPLL_PAD2CK_AC_EN[4]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_PAD2CK_AC_EN_SHFT   4
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_BLADDIV_forec_en_ADDR CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_BLADDIV_forec_en_MASK 0x00000008                // RG_BPLL_BLADDIV_forec_en[3]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_BLADDIV_forec_en_SHFT 3
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_GB128M_force_en_ADDR CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_GB128M_force_en_MASK 0x00000004                // RG_BPLL_GB128M_force_en[2]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_GB128M_force_en_SHFT 2
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_TSTCK_EN_ADDR       CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_TSTCK_EN_MASK       0x00000002                // RG_BPLL_TSTCK_EN[1]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_TSTCK_EN_SHFT       1
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_TSTCK_SEL_ADDR      CONN_AFE_CTL_RG_WBG_PLL_07_ADDR
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_TSTCK_SEL_MASK      0x00000001                // RG_BPLL_TSTCK_SEL[0]
#define CONN_AFE_CTL_RG_WBG_PLL_07_RG_BPLL_TSTCK_SEL_SHFT      0

/* =====================================================================================

  ---RG_WBG_GL1_01 (0x18041000 + 0x040)---

    RG_GL1_AD_TST_SEL[2..0]      - (RW) AD DC monitored voltage selection
                                     000:VR branch 1 full reference
                                     001: VR branch 1 half reference
                                     010: VR branch 2 full reference
                                     011: VR branch 2  half reference   
                                     100: LDO_0P9V
    RG_GL1_AD_TST_EN[3]          - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_GL1_AD_VREF_SEL[6..4]     - (RW) VREF selection
                                     111: 0.861
                                     110: 0.850
                                     101: 0.838
                                     100: 0.827
                                     011: 0.814
                                     010: 0.803 (POR)
                                     001: 0.790
                                     000: 0.779
    RG_GL1_DUODRI[7]             - (RW) 1: reference gen doubled current
                                     0: reference gen normal current
    RG_GL1_LDO0P98_VSEL[10..8]   - (RW) 0.98V LDO tuning
                                     111: 1.017
                                     110: 1.007
                                     101: 0.995
                                     100: 0.984  (POR)
                                     011: 0.969
                                     010: 0.959
                                     001: 0.945
                                     000: 0.933
    RG_GL1_LDO0P8_VSEL[13..11]   - (RW) 0.8V LDO tuning
                                     111:0.901
                                     110: 0.890
                                     101: 0.876
                                     100: 0.865 (POR)
                                     011: 0.849
                                     010: 0.838
                                     001: 0.823
                                     000: 0.812
    RG_GL1_CK_CH_RISE[14]        - (RW) 1: Decoder output clock uses positive polarity
                                     0: Decoder output clock uses negative polarity
    RG_GL1_DIV_H4_L2[15]         - (RW) 1: output clock divided by 4
                                     0: output clock divided by 2
    RG_GL1_AD_CK_OUT_RISE[16]    - (RW) 1: Output clock uses positive polarity
                                     0: Output clock uses negative polarity
    RG_GL1_AD_CH_DFF_RISE[17]    - (RW) 1: DFF sync clock uses positive polarity
                                     0: DFF sync clock uses negative polarity
    RG_GL1_RAW_OUT_EN[18]        - (RW) 1: Raw data out
                                     0: Normal data out
    RG_GL1_DIV_EN[19]            - (RW) 1: output clock divider enable 
                                     0: disable
    RG_GL1_AD_OUTPUT_EN[20]      - (RW) 1: output enable 
                                     0: output disable
    RG_GL1_AD_COMP_TAIL_MSB[21]  - (RW) 1: enlarge comparator tail transistor size for first 3 MSBs
                                     0: default size
    RG_GL1_AD_COMP_TAIL[22]      - (RW) 1: enlarge comparator tail transistor size
                                     0: default size
    RG_GL1_AD_VGEN_PUMP_EN[23]   - (RW) 1: reference gen pump enable
                                     0: reference gen pump disable
    RG_GL1_AD_LDO_PUMP_EN[24]    - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_GL1_AD_VAR_CLKS[25]       - (RW) 1: variable clks
                                     0: fixed clks
    RG_GL1_AD_LSB_EN[26]         - (RW) 1: 10bit cycles conversion
                                     0: 11 bit cycles conversion
    RG_GL1_MODE_L1L5_SYNC_CLK[27] - (RW) 1: GL1 use external post-divider clock
                                     0: Internal
    RG_GL1_MODE_BT_GPS[28]       - (RW) 1: BT Mode for CLK Selection
                                     0: GPS Mode for CLK Selection (always 0 for GPS RX)
    RG_GL1_AD_EXT_CLK_SEL[29]    - (RW) ADC clock select 
                                     (keep internal PLL clock, clock source is controled by RG<21>)
                                     1: External clock source
                                     0: Internal PLL as clock
    RG_GL1_AD_QCH_EN[30]         - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_GL1_AD_ICH_EN[31]         - (RW) 1: I channel enable
                                     0: I channel disable

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_ICH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_ICH_EN_MASK       0x80000000                // RG_GL1_AD_ICH_EN[31]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_ICH_EN_SHFT       31
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_QCH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_QCH_EN_MASK       0x40000000                // RG_GL1_AD_QCH_EN[30]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_QCH_EN_SHFT       30
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EXT_CLK_SEL_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EXT_CLK_SEL_MASK  0x20000000                // RG_GL1_AD_EXT_CLK_SEL[29]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_EXT_CLK_SEL_SHFT  29
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_BT_GPS_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_BT_GPS_MASK     0x10000000                // RG_GL1_MODE_BT_GPS[28]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_BT_GPS_SHFT     28
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_L1L5_SYNC_CLK_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_L1L5_SYNC_CLK_MASK 0x08000000                // RG_GL1_MODE_L1L5_SYNC_CLK[27]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_MODE_L1L5_SYNC_CLK_SHFT 27
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LSB_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LSB_EN_MASK       0x04000000                // RG_GL1_AD_LSB_EN[26]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LSB_EN_SHFT       26
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VAR_CLKS_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VAR_CLKS_MASK     0x02000000                // RG_GL1_AD_VAR_CLKS[25]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VAR_CLKS_SHFT     25
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO_PUMP_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO_PUMP_EN_MASK  0x01000000                // RG_GL1_AD_LDO_PUMP_EN[24]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_LDO_PUMP_EN_SHFT  24
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VGEN_PUMP_EN_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VGEN_PUMP_EN_MASK 0x00800000                // RG_GL1_AD_VGEN_PUMP_EN[23]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VGEN_PUMP_EN_SHFT 23
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_ADDR    CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MASK    0x00400000                // RG_GL1_AD_COMP_TAIL[22]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_SHFT    22
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MSB_ADDR CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MSB_MASK 0x00200000                // RG_GL1_AD_COMP_TAIL_MSB[21]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_COMP_TAIL_MSB_SHFT 21
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_OUTPUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_OUTPUT_EN_MASK    0x00100000                // RG_GL1_AD_OUTPUT_EN[20]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_OUTPUT_EN_SHFT    20
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DIV_EN_ADDR          CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DIV_EN_MASK          0x00080000                // RG_GL1_DIV_EN[19]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DIV_EN_SHFT          19
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RAW_OUT_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RAW_OUT_EN_MASK      0x00040000                // RG_GL1_RAW_OUT_EN[18]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_RAW_OUT_EN_SHFT      18
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CH_DFF_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CH_DFF_RISE_MASK  0x00020000                // RG_GL1_AD_CH_DFF_RISE[17]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CH_DFF_RISE_SHFT  17
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CK_OUT_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CK_OUT_RISE_MASK  0x00010000                // RG_GL1_AD_CK_OUT_RISE[16]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_CK_OUT_RISE_SHFT  16
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DIV_H4_L2_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DIV_H4_L2_MASK       0x00008000                // RG_GL1_DIV_H4_L2[15]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DIV_H4_L2_SHFT       15
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_CK_CH_RISE_ADDR      CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_CK_CH_RISE_MASK      0x00004000                // RG_GL1_CK_CH_RISE[14]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_CK_CH_RISE_SHFT      14
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_LDO0P8_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_LDO0P8_VSEL_MASK     0x00003800                // RG_GL1_LDO0P8_VSEL[13..11]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_LDO0P8_VSEL_SHFT     11
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_LDO0P98_VSEL_ADDR    CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_LDO0P98_VSEL_MASK    0x00000700                // RG_GL1_LDO0P98_VSEL[10..8]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_LDO0P98_VSEL_SHFT    8
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DUODRI_ADDR          CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DUODRI_MASK          0x00000080                // RG_GL1_DUODRI[7]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_DUODRI_SHFT          7
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VREF_SEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VREF_SEL_MASK     0x00000070                // RG_GL1_AD_VREF_SEL[6..4]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_VREF_SEL_SHFT     4
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_TST_EN_MASK       0x00000008                // RG_GL1_AD_TST_EN[3]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_TST_EN_SHFT       3
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_TST_SEL_ADDR      CONN_AFE_CTL_RG_WBG_GL1_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_TST_SEL_MASK      0x00000007                // RG_GL1_AD_TST_SEL[2..0]
#define CONN_AFE_CTL_RG_WBG_GL1_01_RG_GL1_AD_TST_SEL_SHFT      0

/* =====================================================================================

  ---RG_WBG_GL1_02 (0x18041000 + 0x044)---

    RG_GL1_02_RSV1[7..0]         - (RW)  xxx 
    RG_V2I_VICM_SEL[10..8]       - (RW) VICM SEL
    RG_LDO_GPSDIV_VSEL[12..11]   - (RW) LDO_GPSDIV
                                     00: 0.875V
                                     01: 0.900V(POR)
                                     10: 0.925V
                                     11: 0.950V
    RG_WBG_PRCKSEL[13]           - (RW) PRCKSEL 
                                     0: Pump Ref with 128MHz
                                     1: Pump Ref with ADC Clock
    RG_GPS_EXT_CLK_SEL[14]       - (RW) GPS TOPLV clock select
                                     0: Internal PLL as clock
                                     1: External clock source
    RG_V2I_ADBUF_VCM[16..15]     - (RW) ADBUF n-side output Voltage
                                     11: 0.5V
                                     10: 0.55V(POR)
                                     01: 0.6V
                                     00: 0.65V
    RG_V2I_BUF_IGEN_SEL[18..17]  - (RW) RX Top bias current tuning
                                     11:  I5u=4u (0.8X)
                                     10: I5u=5u (1X, POR)
                                     01: I5u=6.68u (1.33X)
                                     00: I5u=10u (2X)
    RG_GL5_INTERNAL_ADCKSEL[19]  - (RW) Internal ADC Clock Divider control (for BT)
                                     0: ADC normal mode clock=32MHz
                                     1: ADC normal mode clock=64MHz
    RG_GL1_INTERNAL_ADCKSEL[20]  - (RW) Internal ADC Clock Divider control (for BT)
                                     0: ADC normal mode clock=32MHz
                                     1: ADC normal mode clock=64MHz
    RG_WBG_SYNC_CLK_INV[21]      - (RW) 0: With SYNC_CLK Registed
                                     1: With SYNC_CLK Inverse Registed
    RG_GPS_CKO_INV_EN[22]        - (RW) 1: AD CKO uses decoder CKO
                                     0: AD CKO uses decoder CKO_B
    RG_GPS_ADCKSEL[24..23]       - (RW) ADC Clock Divider control
                                     00: GPSADC normal mode clock=16MHz
                                     01: GPSADC normal mode clock=64MHz
                                     10: GPSADC normal mode clock=128MHz
    RG_DA_Mode_ADCKSEL[25]       - (RW) 0: DA Mode
                                     1: RG Mode
    RG_GPS_DIV_EN[26]            - (RW) GPS_DIV on/off manual control
                                     0: GPS CK DIV manual off
                                     1: GPS CK DIV manual on
    RG_GPS_DIV_EN_MAN[27]        - (RW) GPS CK DIV on/off manual mode enable
                                     0: normal mode, control by DA
                                     1: manual mode, control by RG
    RG_GL5_AD_EN[28]             - (RW) 0: Disable ADC
                                     1: Enable ADC
    RG_GL5_RG_DA_Mode[29]        - (RW) 0: DA Mode
                                     1: RG Mode
    RG_GL1_AD_EN[30]             - (RW) 0: Disable ADC
                                     1: Enable ADC
    RG_GL1_RG_DA_Mode[31]        - (RW) 0: DA Mode
                                     1: RG Mode

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RG_DA_Mode_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RG_DA_Mode_MASK      0x80000000                // RG_GL1_RG_DA_Mode[31]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_RG_DA_Mode_SHFT      31
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_EN_ADDR           CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_EN_MASK           0x40000000                // RG_GL1_AD_EN[30]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_AD_EN_SHFT           30
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_RG_DA_Mode_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_RG_DA_Mode_MASK      0x20000000                // RG_GL5_RG_DA_Mode[29]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_RG_DA_Mode_SHFT      29
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_AD_EN_ADDR           CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_AD_EN_MASK           0x10000000                // RG_GL5_AD_EN[28]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_AD_EN_SHFT           28
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MAN_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MAN_MASK      0x08000000                // RG_GPS_DIV_EN_MAN[27]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MAN_SHFT      27
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_ADDR          CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_MASK          0x04000000                // RG_GPS_DIV_EN[26]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_DIV_EN_SHFT          26
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_DA_Mode_ADCKSEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_DA_Mode_ADCKSEL_MASK     0x02000000                // RG_DA_Mode_ADCKSEL[25]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_DA_Mode_ADCKSEL_SHFT     25
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_ADCKSEL_ADDR         CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_ADCKSEL_MASK         0x01800000                // RG_GPS_ADCKSEL[24..23]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_ADCKSEL_SHFT         23
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_CKO_INV_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_CKO_INV_EN_MASK      0x00400000                // RG_GPS_CKO_INV_EN[22]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_CKO_INV_EN_SHFT      22
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_SYNC_CLK_INV_ADDR    CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_SYNC_CLK_INV_MASK    0x00200000                // RG_WBG_SYNC_CLK_INV[21]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_SYNC_CLK_INV_SHFT    21
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_INTERNAL_ADCKSEL_ADDR CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_INTERNAL_ADCKSEL_MASK 0x00100000                // RG_GL1_INTERNAL_ADCKSEL[20]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_INTERNAL_ADCKSEL_SHFT 20
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_INTERNAL_ADCKSEL_ADDR CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_INTERNAL_ADCKSEL_MASK 0x00080000                // RG_GL5_INTERNAL_ADCKSEL[19]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL5_INTERNAL_ADCKSEL_SHFT 19
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_BUF_IGEN_SEL_ADDR    CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_BUF_IGEN_SEL_MASK    0x00060000                // RG_V2I_BUF_IGEN_SEL[18..17]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_BUF_IGEN_SEL_SHFT    17
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_ADBUF_VCM_ADDR       CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_ADBUF_VCM_MASK       0x00018000                // RG_V2I_ADBUF_VCM[16..15]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_ADBUF_VCM_SHFT       15
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_EXT_CLK_SEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_EXT_CLK_SEL_MASK     0x00004000                // RG_GPS_EXT_CLK_SEL[14]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GPS_EXT_CLK_SEL_SHFT     14
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_PRCKSEL_ADDR         CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_PRCKSEL_MASK         0x00002000                // RG_WBG_PRCKSEL[13]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_WBG_PRCKSEL_SHFT         13
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_LDO_GPSDIV_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_LDO_GPSDIV_VSEL_MASK     0x00001800                // RG_LDO_GPSDIV_VSEL[12..11]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_LDO_GPSDIV_VSEL_SHFT     11
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_VICM_SEL_ADDR        CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_VICM_SEL_MASK        0x00000700                // RG_V2I_VICM_SEL[10..8]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_V2I_VICM_SEL_SHFT        8
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_02_RSV1_ADDR         CONN_AFE_CTL_RG_WBG_GL1_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_02_RSV1_MASK         0x000000FF                // RG_GL1_02_RSV1[7..0]
#define CONN_AFE_CTL_RG_WBG_GL1_02_RG_GL1_02_RSV1_SHFT         0

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

  ---RG_WBG_GL5_01 (0x18041000 + 0x100)---

    RG_GL5_AD_TST_SEL[2..0]      - (RW) AD DC monitored voltage selection
                                     000:VR branch 1 full reference
                                     001: VR branch 1 half reference
                                     010: VR branch 2 full reference
                                     011: VR branch 2  half reference   
                                     100: LDO_0P9V
    RG_GL5_AD_TST_EN[3]          - (RW) 1: AD DC monitor enable 
                                     0: AD DC monitor disable
    RG_GL5_AD_VREF_SEL[6..4]     - (RW) VREF selection
                                     111: 0.861
                                     110: 0.850
                                     101: 0.838
                                     100: 0.827
                                     011: 0.814
                                     010: 0.803 (POR)
                                     001: 0.790
                                     000: 0.779
    RG_GL5_DUODRI[7]             - (RW) 1: reference gen doubled current
                                     0: reference gen normal current
    RG_GL5_LDO0P98_VSEL[10..8]   - (RW) 0.98V LDO tuning
                                     111: 1.017
                                     110: 1.007
                                     101: 0.995
                                     100: 0.984  (POR)
                                     011: 0.969
                                     010: 0.959
                                     001: 0.945
                                     000: 0.933
    RG_GL5_LDO0P8_VSEL[13..11]   - (RW) 0.8V LDO tuning
                                     111:0.901
                                     110: 0.890
                                     101: 0.876
                                     100: 0.865 (POR)
                                     011: 0.849
                                     010: 0.838
                                     001: 0.823
                                     000: 0.812
    RG_GL5_CK_CH_RISE[14]        - (RW) 1: Decoder output clock uses positive polarity
                                     0: Decoder output clock uses negative polarity
    RG_GL5_DIV_H4_L2[15]         - (RW) 1: output clock divided by 4
                                     0: output clock divided by 2
    RG_GL5_AD_CK_OUT_RISE[16]    - (RW) 1: Output clock uses positive polarity
                                     0: Output clock uses negative polarity
    RG_GL5_AD_CH_DFF_RISE[17]    - (RW) 1: DFF sync clock uses positive polarity
                                     0: DFF sync clock uses negative polarity
    RG_GL5_RAW_OUT_EN[18]        - (RW) 1: Raw data out
                                     0: Normal data out
    RG_GL5_DIV_EN[19]            - (RW) 1: output clock divider enable 
                                     0: disable
    RG_GL5_AD_OUTPUT_EN[20]      - (RW) 1: output enable 
                                     0: output disable
    RG_GL5_AD_COMP_TAIL_MSB[21]  - (RW) 1: enlarge comparator tail transistor size for first 3 MSBs
                                     0: default size
    RG_GL5_AD_COMP_TAIL[22]      - (RW) 1: enlarge comparator tail transistor size
                                     0: default size
    RG_GL5_AD_VGEN_PUMP_EN[23]   - (RW) 1: reference gen pump enable
                                     0: reference gen pump disable
    RG_GL5_AD_LDO_PUMP_EN[24]    - (RW) 1: LDO pump enable
                                     0: LDO pump disable
    RG_GL5_AD_VAR_CLKS[25]       - (RW) 1: variable clks
                                     0: fixed clks
    RG_GL5_AD_LSB_EN[26]         - (RW) 1: 10bit cycles conversion
                                     0: 11 bit cycles conversion
    RG_GL5_MODE_L1L5_SYNC_CLK[27] - (RW) 1: GL5 use external post-divider clock
                                     0: Internal
    RG_GL5_MODE_BT_GPS[28]       - (RW) 1: BT Mode for CLK Selection
                                     0: GPS Mode for CLK Selection (always 0 for GPS RX)
    RG_GL5_AD_EXT_CLK_SEL[29]    - (RW) ADC clock select 
                                     (keep internal PLL clock, clock source is controled)
                                     1: External clock source
                                     0: Internal PLL as clock
    RG_GL5_AD_QCH_EN[30]         - (RW) 1: Q channel enable
                                     0: Q channel disable
    RG_GL5_AD_ICH_EN[31]         - (RW) 1: I channel enable
                                     0: I channel disable

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_ICH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_ICH_EN_MASK       0x80000000                // RG_GL5_AD_ICH_EN[31]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_ICH_EN_SHFT       31
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_QCH_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_QCH_EN_MASK       0x40000000                // RG_GL5_AD_QCH_EN[30]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_QCH_EN_SHFT       30
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EXT_CLK_SEL_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EXT_CLK_SEL_MASK  0x20000000                // RG_GL5_AD_EXT_CLK_SEL[29]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_EXT_CLK_SEL_SHFT  29
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_BT_GPS_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_BT_GPS_MASK     0x10000000                // RG_GL5_MODE_BT_GPS[28]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_BT_GPS_SHFT     28
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_L1L5_SYNC_CLK_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_L1L5_SYNC_CLK_MASK 0x08000000                // RG_GL5_MODE_L1L5_SYNC_CLK[27]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_MODE_L1L5_SYNC_CLK_SHFT 27
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LSB_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LSB_EN_MASK       0x04000000                // RG_GL5_AD_LSB_EN[26]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LSB_EN_SHFT       26
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VAR_CLKS_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VAR_CLKS_MASK     0x02000000                // RG_GL5_AD_VAR_CLKS[25]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VAR_CLKS_SHFT     25
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO_PUMP_EN_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO_PUMP_EN_MASK  0x01000000                // RG_GL5_AD_LDO_PUMP_EN[24]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_LDO_PUMP_EN_SHFT  24
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VGEN_PUMP_EN_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VGEN_PUMP_EN_MASK 0x00800000                // RG_GL5_AD_VGEN_PUMP_EN[23]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VGEN_PUMP_EN_SHFT 23
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_ADDR    CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MASK    0x00400000                // RG_GL5_AD_COMP_TAIL[22]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_SHFT    22
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MSB_ADDR CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MSB_MASK 0x00200000                // RG_GL5_AD_COMP_TAIL_MSB[21]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_COMP_TAIL_MSB_SHFT 21
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_OUTPUT_EN_ADDR    CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_OUTPUT_EN_MASK    0x00100000                // RG_GL5_AD_OUTPUT_EN[20]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_OUTPUT_EN_SHFT    20
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DIV_EN_ADDR          CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DIV_EN_MASK          0x00080000                // RG_GL5_DIV_EN[19]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DIV_EN_SHFT          19
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RAW_OUT_EN_ADDR      CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RAW_OUT_EN_MASK      0x00040000                // RG_GL5_RAW_OUT_EN[18]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_RAW_OUT_EN_SHFT      18
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CH_DFF_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CH_DFF_RISE_MASK  0x00020000                // RG_GL5_AD_CH_DFF_RISE[17]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CH_DFF_RISE_SHFT  17
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CK_OUT_RISE_ADDR  CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CK_OUT_RISE_MASK  0x00010000                // RG_GL5_AD_CK_OUT_RISE[16]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_CK_OUT_RISE_SHFT  16
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DIV_H4_L2_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DIV_H4_L2_MASK       0x00008000                // RG_GL5_DIV_H4_L2[15]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DIV_H4_L2_SHFT       15
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_CK_CH_RISE_ADDR      CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_CK_CH_RISE_MASK      0x00004000                // RG_GL5_CK_CH_RISE[14]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_CK_CH_RISE_SHFT      14
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_LDO0P8_VSEL_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_LDO0P8_VSEL_MASK     0x00003800                // RG_GL5_LDO0P8_VSEL[13..11]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_LDO0P8_VSEL_SHFT     11
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_LDO0P98_VSEL_ADDR    CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_LDO0P98_VSEL_MASK    0x00000700                // RG_GL5_LDO0P98_VSEL[10..8]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_LDO0P98_VSEL_SHFT    8
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DUODRI_ADDR          CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DUODRI_MASK          0x00000080                // RG_GL5_DUODRI[7]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_DUODRI_SHFT          7
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VREF_SEL_ADDR     CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VREF_SEL_MASK     0x00000070                // RG_GL5_AD_VREF_SEL[6..4]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_VREF_SEL_SHFT     4
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_TST_EN_ADDR       CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_TST_EN_MASK       0x00000008                // RG_GL5_AD_TST_EN[3]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_TST_EN_SHFT       3
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_TST_SEL_ADDR      CONN_AFE_CTL_RG_WBG_GL5_01_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_TST_SEL_MASK      0x00000007                // RG_GL5_AD_TST_SEL[2..0]
#define CONN_AFE_CTL_RG_WBG_GL5_01_RG_GL5_AD_TST_SEL_SHFT      0

/* =====================================================================================

  ---RG_WBG_GL5_02 (0x18041000 + 0x104)---

    RG_WBG_GL5_02_RSV[31..0]     - (RW)  xxx 

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV_ADDR      CONN_AFE_CTL_RG_WBG_GL5_02_ADDR
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV_MASK      0xFFFFFFFF                // RG_WBG_GL5_02_RSV[31..0]
#define CONN_AFE_CTL_RG_WBG_GL5_02_RG_WBG_GL5_02_RSV_SHFT      0

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
    RG_WBG_EN_AD_WF0TSSI_OCC[4]  - (RW) Directly trigger AD_WBG_AD_WF0TSSI_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_AD_DFS_OCC[5]      - (RW) Directly trigger AD_WBG_AD_DFS_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_DA_WF_OCC[6]       - (RW) Directly trigger AD_WBG_DA_WF_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_AD_WF_OCC[7]       - (RW) Directly trigger AD_WBG_AD_WF_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_DA_BT1_OCC[8]      - (RW) Directly trigger AD_WBG_DA_BT1_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_WPLL_BG_ER_OCC[9]  - (RW) Directly trigger WPLL_BG_EN in OCC mode (when RG_OCC_PLL_SW = 1'b1, RFSOC only)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_BPLL_BG_ER_OCC[10] - (RW) Directly trigger BPLL_BG_EN in OCC mode (when RG_OCC_PLL_SW = 1'b1, RFSOC only)
                                     0: Disable
                                     1: Enable
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
    RG_WBG_EN_AD_BT1_OCC[20]     - (RW) Directly trigger AD_WBG_BT1_DA_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_DA_BT0_OCC[21]     - (RW) Directly trigger AD_WBG_BT0_DA_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_AD_BT0_OCC[22]     - (RW) Directly trigger AD_WBG_BT0_AD_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
    RG_WBG_CG_EN_AD_GPS[23]      - (RW) Source clock enable - GPS AD
                                     0: Disable
                                     1: Enable
    RG_WBG_EN_AD_GPS_OCC[24]     - (RW) Directly trigger AD_WBG_GPS_AD_CKO in OCC mode  (when RG_OCC_PLL_SW = 1'b1)
                                     0: Disable
                                     1: Enable
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
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_GPS_OCC_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_GPS_OCC_MASK  0x01000000                // RG_WBG_EN_AD_GPS_OCC[24]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_GPS_OCC_SHFT  24
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_GPS_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_GPS_MASK   0x00800000                // RG_WBG_CG_EN_AD_GPS[23]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_CG_EN_AD_GPS_SHFT   23
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_BT0_OCC_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_BT0_OCC_MASK  0x00400000                // RG_WBG_EN_AD_BT0_OCC[22]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_BT0_OCC_SHFT  22
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_BT0_OCC_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_BT0_OCC_MASK  0x00200000                // RG_WBG_EN_DA_BT0_OCC[21]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_BT0_OCC_SHFT  21
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_BT1_OCC_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_BT1_OCC_MASK  0x00100000                // RG_WBG_EN_AD_BT1_OCC[20]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_BT1_OCC_SHFT  20
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
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BPLL_BG_ER_OCC_ADDR CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BPLL_BG_ER_OCC_MASK 0x00000400                // RG_WBG_EN_BPLL_BG_ER_OCC[10]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_BPLL_BG_ER_OCC_SHFT 10
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WPLL_BG_ER_OCC_ADDR CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WPLL_BG_ER_OCC_MASK 0x00000200                // RG_WBG_EN_WPLL_BG_ER_OCC[9]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_WPLL_BG_ER_OCC_SHFT 9
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_BT1_OCC_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_BT1_OCC_MASK  0x00000100                // RG_WBG_EN_DA_BT1_OCC[8]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_BT1_OCC_SHFT  8
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_WF_OCC_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_WF_OCC_MASK   0x00000080                // RG_WBG_EN_AD_WF_OCC[7]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_WF_OCC_SHFT   7
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_WF_OCC_ADDR   CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_WF_OCC_MASK   0x00000040                // RG_WBG_EN_DA_WF_OCC[6]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_DA_WF_OCC_SHFT   6
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_DFS_OCC_ADDR  CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_DFS_OCC_MASK  0x00000020                // RG_WBG_EN_AD_DFS_OCC[5]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_DFS_OCC_SHFT  5
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_WF0TSSI_OCC_ADDR CONN_AFE_CTL_RG_WBG_CTL_OCC_ADDR
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_WF0TSSI_OCC_MASK 0x00000010                // RG_WBG_EN_AD_WF0TSSI_OCC[4]
#define CONN_AFE_CTL_RG_WBG_CTL_OCC_RG_WBG_EN_AD_WF0TSSI_OCC_SHFT 4
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

  ---RG_DIG_EN_04 (0x18041000 + 0x015C)---

    RG_WBG_EN_RSV0_PLL[1..0]     - (RW) For reserved pll mode selection (2'b01: bpll, 2'b11: casc, 2'b10:wpll)
    RG_WBG_EN_RSV1_PLL[3..2]     - (RW) For reserved pll mode selection (2'b01: bpll, 2'b11: casc, 2'b10:wpll)
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_AFE_CTL_RG_DIG_EN_04_RG_WBG_EN_RSV1_PLL_ADDR      CONN_AFE_CTL_RG_DIG_EN_04_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_04_RG_WBG_EN_RSV1_PLL_MASK      0x0000000C                // RG_WBG_EN_RSV1_PLL[3..2]
#define CONN_AFE_CTL_RG_DIG_EN_04_RG_WBG_EN_RSV1_PLL_SHFT      2
#define CONN_AFE_CTL_RG_DIG_EN_04_RG_WBG_EN_RSV0_PLL_ADDR      CONN_AFE_CTL_RG_DIG_EN_04_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_04_RG_WBG_EN_RSV0_PLL_MASK      0x00000003                // RG_WBG_EN_RSV0_PLL[1..0]
#define CONN_AFE_CTL_RG_DIG_EN_04_RG_WBG_EN_RSV0_PLL_SHFT      0

/* =====================================================================================

  ---RG_WBG_AFE_GPIO_RO (0x18041000 + 0x160)---

    RG_WBG_AFE_GPIO_RO[31..0]    - (RO) afe_gpio

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_AFE_GPIO_RO_RG_WBG_AFE_GPIO_RO_ADDR CONN_AFE_CTL_RG_WBG_AFE_GPIO_RO_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_GPIO_RO_RG_WBG_AFE_GPIO_RO_MASK 0xFFFFFFFF                // RG_WBG_AFE_GPIO_RO[31..0]
#define CONN_AFE_CTL_RG_WBG_AFE_GPIO_RO_RG_WBG_AFE_GPIO_RO_SHFT 0

/* =====================================================================================

  ---RG_WBG_AFE_CRC (0x18041000 + 0x0164)---

    rg_wbg_afe_crc_en[0]         - (RW) 0: Disable CRC
                                     1: Enable CRC
    rg_wbg_afe_crc[21..1]        - (RW) Reserved
    ro_wbg_afe_crc_err[22]       - (RO) 0: CRC calibration doesn't have error
                                     1: CRC calibration has error
    ro_wbg_afe_crc_done[23]      - (RO) 0: CRC calibration not done
                                     1: CRC calibration done (4096 cycles)
    ro_wbg_afe_crc[31..24]       - (RO) Reserved

 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_ADDR        CONN_AFE_CTL_RG_WBG_AFE_CRC_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_MASK        0xFF000000                // ro_wbg_afe_crc[31..24]
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_SHFT        24
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_done_ADDR   CONN_AFE_CTL_RG_WBG_AFE_CRC_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_done_MASK   0x00800000                // ro_wbg_afe_crc_done[23]
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_done_SHFT   23
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_err_ADDR    CONN_AFE_CTL_RG_WBG_AFE_CRC_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_err_MASK    0x00400000                // ro_wbg_afe_crc_err[22]
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_ro_wbg_afe_crc_err_SHFT    22
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_rg_wbg_afe_crc_ADDR        CONN_AFE_CTL_RG_WBG_AFE_CRC_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_rg_wbg_afe_crc_MASK        0x003FFFFE                // rg_wbg_afe_crc[21..1]
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_rg_wbg_afe_crc_SHFT        1
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_rg_wbg_afe_crc_en_ADDR     CONN_AFE_CTL_RG_WBG_AFE_CRC_ADDR
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_rg_wbg_afe_crc_en_MASK     0x00000001                // rg_wbg_afe_crc_en[0]
#define CONN_AFE_CTL_RG_WBG_AFE_CRC_rg_wbg_afe_crc_en_SHFT     0

#ifdef __cplusplus
}
#endif

#endif // __CONN_AFE_CTL_REGS_H__
