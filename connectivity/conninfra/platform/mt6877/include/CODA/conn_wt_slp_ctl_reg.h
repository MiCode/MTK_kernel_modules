/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_wt_slp_ctl_reg.h
//[Revision time]   : Mon Nov  9 19:43:08 2020

#ifndef __CONN_WT_SLP_CTL_REG_REGS_H__
#define __CONN_WT_SLP_CTL_REG_REGS_H__

//****************************************************************************
//
//                     CONN_WT_SLP_CTL_REG CR Definitions (0x1800_5000)
//
//****************************************************************************

#define CONN_WT_SLP_CTL_REG_BASE                               (CONN_REG_CONN_WT_SLP_CTL_REG_ADDR)

#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x004) // 5004
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x010) // 5010
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x014) // 5014
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x018) // 5018
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x01C) // 501C
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x020) // 5020
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x024) // 5024
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x028) // 5028
#define CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_ADDR                   (CONN_WT_SLP_CTL_REG_BASE + 0x02C) // 502C
#define CONN_WT_SLP_CTL_REG_WB_BG_ON1_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x030) // 5030
#define CONN_WT_SLP_CTL_REG_WB_BG_ON2_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x034) // 5034
#define CONN_WT_SLP_CTL_REG_WB_BG_ON3_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x038) // 5038
#define CONN_WT_SLP_CTL_REG_WB_BG_ON4_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x03C) // 503C
#define CONN_WT_SLP_CTL_REG_WB_BG_ON5_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x040) // 5040
#define CONN_WT_SLP_CTL_REG_WB_BG_ON6_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x044) // 5044
#define CONN_WT_SLP_CTL_REG_WB_BG_ON7_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x048) // 5048
#define CONN_WT_SLP_CTL_REG_WB_BG_ON8_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x04C) // 504C
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF1_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x050) // 5050
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF2_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x054) // 5054
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF3_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x058) // 5058
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF4_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x05C) // 505C
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF5_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x060) // 5060
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF6_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x064) // 5064
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF7_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x068) // 5068
#define CONN_WT_SLP_CTL_REG_WB_BG_OFF8_ADDR                    (CONN_WT_SLP_CTL_REG_BASE + 0x06C) // 506C
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_ADDR                 (CONN_WT_SLP_CTL_REG_BASE + 0x070) // 5070
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x074) // 5074
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x078) // 5078
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_ADDR                 (CONN_WT_SLP_CTL_REG_BASE + 0x07C) // 507C
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x080) // 5080
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x084) // 5084
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_ADDR                (CONN_WT_SLP_CTL_REG_BASE + 0x088) // 5088
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x08c) // 508C
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x090) // 5090
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_ADDR              (CONN_WT_SLP_CTL_REG_BASE + 0x094) // 5094
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_ADDR             (CONN_WT_SLP_CTL_REG_BASE + 0x098) // 5098
#define CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR                     (CONN_WT_SLP_CTL_REG_BASE + 0x0A8) // 50A8
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x120) // 5120
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x124) // 5124
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x128) // 5128
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x12C) // 512C
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x130) // 5130
#define CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR               (CONN_WT_SLP_CTL_REG_BASE + 0x134) // 5134

/* =====================================================================================
  ---WB_SLP_CTL (0x18005000 + 0x004)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH_ADDR         CONN_WT_SLP_CTL_REG_WB_SLP_CTL_ADDR
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH_MASK         0x0000001F                // CMD_LENGTH[4..0]
#define CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH_SHFT         0

/* =====================================================================================
  ---WB_WF_CK_ADDR (0x18005000 + 0x070)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR_MASK 0x0FFF0000                // WB_WF_B1_CK_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR_ADDR   CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR_MASK   0x00000FFF                // WB_WF_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR_SHFT   0


/* =====================================================================================
  ---WB_WF_WAKE_ADDR (0x18005000 + 0x074)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR_MASK 0x0FFF0000                // WB_WF_B1_WAKE_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR_MASK 0x00000FFF                // WB_WF_WAKE_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR_SHFT 0

/* =====================================================================================
  ---WB_WF_ZPS_ADDR (0x18005000 + 0x078)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR_MASK 0x0FFF0000                // WB_WF_B1_ZPS_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR_MASK 0x00000FFF                // WB_WF_ZPS_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR_SHFT 0

/* =====================================================================================
  ---WB_BT_CK_ADDR (0x18005000 + 0x07C)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR_ADDR   CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR_MASK   0x00000FFF                // WB_BT_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR_SHFT   0


/* =====================================================================================
  ---WB_BT_WAKE_ADDR (0x18005000 + 0x080)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR_MASK 0x00000FFF                // WB_BT_WAKE_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR_SHFT 0


/* =====================================================================================
  ---WB_TOP_CK_ADDR (0x18005000 + 0x084)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR_MASK 0x00000FFF                // WB_TOP_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR_SHFT 0


/* =====================================================================================
  ---WB_GPS_CK_ADDR (0x18005000 + 0x088)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR_MASK 0x0FFF0000                // WB_GPS_L5_CK_ADDR[27..16]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR_SHFT 16
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR_MASK 0x00000FFF                // WB_GPS_CK_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR_SHFT 0

/* =====================================================================================
  ---WB_WF_B0_CMD_ADDR (0x18005000 + 0x08c)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR_MASK 0x00000FFF                // WB_WF_B0_CMD_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR_SHFT 0


/* =====================================================================================
  ---WB_WF_B1_CMD_ADDR (0x18005000 + 0x090)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR_MASK 0x00000FFF                // WB_WF_B1_CMD_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR_SHFT 0


/* =====================================================================================
  ---WB_GPS_RFBUF_ADR (0x18005000 + 0x094)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR_MASK 0x00000FFF                // WB_GPS_RFBUF_ADR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR_SHFT 0

/* =====================================================================================
  ---WB_GPS_L5_EN_ADDR (0x18005000 + 0x098)---
 =====================================================================================*/
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR_ADDR CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_ADDR
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR_MASK 0x00000FFF                // WB_GPS_L5_EN_ADDR[11..0]
#define CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR_SHFT 0


#endif // __CONN_WT_SLP_CTL_REG_REGS_H__
