/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_afe_ctl.h
//[Revision time]   : Thu Nov 12 11:00:16 2020

#ifndef __CONN_AFE_CTL_REGS_H__
#define __CONN_AFE_CTL_REGS_H__

//****************************************************************************
//
//                     CONN_AFE_CTL CR Definitions
//
//****************************************************************************

#define CONN_AFE_CTL_BASE                                      (CONN_REG_CONN_AFE_CTL_ADDR) //0x18003000

#define CONN_AFE_CTL_RG_DIG_EN_01_ADDR                         (CONN_AFE_CTL_BASE + 0x000) // 3000
#define CONN_AFE_CTL_RG_DIG_EN_02_ADDR                         (CONN_AFE_CTL_BASE + 0x004) // 3004
#define CONN_AFE_CTL_RG_DIG_EN_03_ADDR                         (CONN_AFE_CTL_BASE + 0x008) // 3008
#define CONN_AFE_CTL_RG_DIG_TOP_01_ADDR                        (CONN_AFE_CTL_BASE + 0x00C) // 300C
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x058) // 3058
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x078) // 3078
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR                     (CONN_AFE_CTL_BASE + 0x094) // 3094
#define CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR                      (CONN_AFE_CTL_BASE + 0x0F4) // 30F4

#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF_ADDR    CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF_MASK    0x00030000                // RG_WBG_EN_MCU_PLL_WF[17..16]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF_SHFT    16
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL_ADDR        CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL_MASK        0x000000C0                // RG_WBG_EN_BT_PLL[7..6]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL_SHFT        6
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF_ADDR   CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF_MASK   0x0000000C                // RG_WBG_EN_MCU_PLL_BGF[3..2]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF_SHFT   2
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL_ADDR        CONN_AFE_CTL_RG_DIG_EN_02_ADDR
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL_MASK        0x00000003                // RG_WBG_EN_WF_PLL[1..0]
#define CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL_SHFT        0

#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2_ADDR            CONN_AFE_CTL_RG_DIG_TOP_01_ADDR
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2_MASK            0x00078000                // RG_DIG_RSV2[18..15]
#define CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2_SHFT            15

/* =====================================================================================
  ---RG_WBG_BT0_TX_03 (0x18003000 + 0x058)---
 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER_ADDR CONN_AFE_CTL_RG_WBG_BT0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER_MASK 0x03C00000                // RG_BT0_BBTX_VCM_VGER[25..22]
#define CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER_SHFT 22

/* =====================================================================================
  ---RG_WBG_WF0_TX_03 (0x18003000 + 0x078)---
 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER_ADDR CONN_AFE_CTL_RG_WBG_WF0_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER_MASK 0x03C00000                // RG_W0_BBTX_VCM_VGER[25..22]
#define CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER_SHFT 22

/* =====================================================================================
  ---RG_WBG_WF1_TX_03 (0x18003000 + 0x094)---
 =====================================================================================*/
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER_ADDR CONN_AFE_CTL_RG_WBG_WF1_TX_03_ADDR
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER_MASK 0x03C00000                // RG_W1_BBTX_VCM_VGER[25..22]
#define CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER_SHFT 22


#endif // __CONN_AFE_CTL_REGS_H__
