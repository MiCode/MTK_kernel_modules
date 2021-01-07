/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __CONN_PTA6_REGS_H__
#define __CONN_PTA6_REGS_H__

#define CONN_PTA6_BASE                                         0x1800C000

#define CONN_PTA6_WFSET_PTA_CTRL_ADDR                          (CONN_PTA6_BASE + 0x0000)
#define CONN_PTA6_PTA_CLK_CFG_ADDR                             (CONN_PTA6_BASE + 0x0004)
#define CONN_PTA6_BTSET_PTA_CTRL_ADDR                          (CONN_PTA6_BASE + 0x0008)
#define CONN_PTA6_RO_PTA_CTRL_ADDR                             (CONN_PTA6_BASE + 0x000C)
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_ADDR                    (CONN_PTA6_BASE + 0x0010)
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_ADDR                    (CONN_PTA6_BASE + 0x0014)
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_ADDR                    (CONN_PTA6_BASE + 0x0018)
#define CONN_PTA6_RO_LTE_CMD_RX_0_ADDR                         (CONN_PTA6_BASE + 0x0020)
#define CONN_PTA6_RO_LTE_CMD_RX_1_ADDR                         (CONN_PTA6_BASE + 0x0024)
#define CONN_PTA6_RO_LTE_CMD_RX_2_ADDR                         (CONN_PTA6_BASE + 0x0028)
#define CONN_PTA6_LTE_CMD_TX_CFG_0_ADDR                        (CONN_PTA6_BASE + 0x0030)
#define CONN_PTA6_LTE_CMD_TX_CFG_1_ADDR                        (CONN_PTA6_BASE + 0x0034)
#define CONN_PTA6_LTE_CMD_TX_CFG_2_ADDR                        (CONN_PTA6_BASE + 0x0038)
#define CONN_PTA6_RO_LTE_CMD_TX_SEQ_ADDR                       (CONN_PTA6_BASE + 0x003C)
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_ADDR                     (CONN_PTA6_BASE + 0x0040)
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_ADDR                     (CONN_PTA6_BASE + 0x0044)
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_ADDR                     (CONN_PTA6_BASE + 0x0048)
#define CONN_PTA6_RO_LTE_CMD_TX_0_ADDR                         (CONN_PTA6_BASE + 0x0050)
#define CONN_PTA6_RO_LTE_CMD_TX_1_ADDR                         (CONN_PTA6_BASE + 0x0054)
#define CONN_PTA6_RO_LTE_CMD_TX_2_ADDR                         (CONN_PTA6_BASE + 0x0058)
#define CONN_PTA6_RO_RX_DEC_DBG_0_ADDR                         (CONN_PTA6_BASE + 0x0060)
#define CONN_PTA6_RO_RX_DEC_DBG_1_ADDR                         (CONN_PTA6_BASE + 0x0064)
#define CONN_PTA6_RO_RX_DEC_DBG_2_ADDR                         (CONN_PTA6_BASE + 0x0068)
#define CONN_PTA6_RO_RX_DEC_DBG_3_ADDR                         (CONN_PTA6_BASE + 0x006C)
#define CONN_PTA6_RO_RX_DEC_DBG_4_ADDR                         (CONN_PTA6_BASE + 0x0070)
#define CONN_PTA6_RO_RX_DEC_DBG_5_ADDR                         (CONN_PTA6_BASE + 0x0074)
#define CONN_PTA6_RO_RX_DEC_DBG_6_ADDR                         (CONN_PTA6_BASE + 0x0078)
#define CONN_PTA6_RO_RX_DEC_DBG_7_ADDR                         (CONN_PTA6_BASE + 0x007C)
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_ADDR                    (CONN_PTA6_BASE + 0x0080)
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR                    (CONN_PTA6_BASE + 0x0084)
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR                    (CONN_PTA6_BASE + 0x0088)
#define CONN_PTA6_REQ_GEN_CTRL_ADDR                            (CONN_PTA6_BASE + 0x008C)
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR                     (CONN_PTA6_BASE + 0x0090)
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR                   (CONN_PTA6_BASE + 0x0094)
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_ADDR                   (CONN_PTA6_BASE + 0x009C)
#define CONN_PTA6_RO_VER_CODE_ADDR                             (CONN_PTA6_BASE + 0x00B0)
#define CONN_PTA6_LTE_PRI_MAP_ADDR                             (CONN_PTA6_BASE + 0x00C0)
#define CONN_PTA6_LTE_PRI_CFG_ADDR                             (CONN_PTA6_BASE + 0x00C4)
#define CONN_PTA6_PRI_EXT_ADDR                                 (CONN_PTA6_BASE + 0x00C8)
#define CONN_PTA6_ARB_CTRL_ADDR                                (CONN_PTA6_BASE + 0x00D0)
#define CONN_PTA6_ARB_COEX_LUT_CFG_0_ADDR                      (CONN_PTA6_BASE + 0x00D4)
#define CONN_PTA6_ARB_COEX_LUT_CFG_1_ADDR                      (CONN_PTA6_BASE + 0x00D8)
#define CONN_PTA6_ARB_COEX_LUT_CFG_2_ADDR                      (CONN_PTA6_BASE + 0x00DC)
#define CONN_PTA6_ARB_COEX_LUT_CFG_3_ADDR                      (CONN_PTA6_BASE + 0x00E0)
#define CONN_PTA6_ARB_COEX_LUT_CFG_4_ADDR                      (CONN_PTA6_BASE + 0x00E4)
#define CONN_PTA6_ARB_COEX_LUT_CFG_5_ADDR                      (CONN_PTA6_BASE + 0x00E8)
#define CONN_PTA6_ARB_COEX_LUT_CFG_6_ADDR                      (CONN_PTA6_BASE + 0x00EC)
#define CONN_PTA6_ARB_COEX_LUT_CFG_7_ADDR                      (CONN_PTA6_BASE + 0x00F0)
#define CONN_PTA6_ARB_COEX_LUT_CFG_8_ADDR                      (CONN_PTA6_BASE + 0x00F4)
#define CONN_PTA6_ARB_COEX_LUT_CFG_9_ADDR                      (CONN_PTA6_BASE + 0x00F8)
#define CONN_PTA6_ARB_COEX_LUT_CFG_10_ADDR                     (CONN_PTA6_BASE + 0x00FC)
#define CONN_PTA6_ARB_COEX_LUT_CFG_11_ADDR                     (CONN_PTA6_BASE + 0x0100)
#define CONN_PTA6_ARB_COEX_LUT_CFG_12_ADDR                     (CONN_PTA6_BASE + 0x0104)
#define CONN_PTA6_ARB_COEX_LUT_CFG_13_ADDR                     (CONN_PTA6_BASE + 0x0108)
#define CONN_PTA6_ARB_COEX_LUT_CFG_14_ADDR                     (CONN_PTA6_BASE + 0x010C)
#define CONN_PTA6_ARB_COEX_LUT_CFG_15_ADDR                     (CONN_PTA6_BASE + 0x0110)
#define CONN_PTA6_ARB_COEX_LUT_CFG_16_ADDR                     (CONN_PTA6_BASE + 0x0114)
#define CONN_PTA6_ARB_COEX_LUT_CFG_17_ADDR                     (CONN_PTA6_BASE + 0x0118)
#define CONN_PTA6_ARB_COEX_LUT_CFG_18_ADDR                     (CONN_PTA6_BASE + 0x011C)
#define CONN_PTA6_ARB_COEX_LUT_CFG_19_ADDR                     (CONN_PTA6_BASE + 0x0120)
#define CONN_PTA6_ARB_DBG_GRNT_CFG_ADDR                        (CONN_PTA6_BASE + 0x0140)
#define CONN_PTA6_ARB_RO_COEX_LUT_0_ADDR                       (CONN_PTA6_BASE + 0x0150)
#define CONN_PTA6_ARB_RO_COEX_LUT_1_ADDR                       (CONN_PTA6_BASE + 0x0154)
#define CONN_PTA6_ARB_RO_COEX_LUT_2_ADDR                       (CONN_PTA6_BASE + 0x0158)
#define CONN_PTA6_ARB_RO_COEX_LUT_3_ADDR                       (CONN_PTA6_BASE + 0x015C)
#define CONN_PTA6_ARB_RO_COEX_LUT_4_ADDR                       (CONN_PTA6_BASE + 0x0160)
#define CONN_PTA6_ARB_RO_COEX_LUT_5_ADDR                       (CONN_PTA6_BASE + 0x0164)
#define CONN_PTA6_ARB_RO_COEX_LUT_6_ADDR                       (CONN_PTA6_BASE + 0x0168)
#define CONN_PTA6_ARB_RO_COEX_LUT_7_ADDR                       (CONN_PTA6_BASE + 0x016C)
#define CONN_PTA6_ARB_RO_COEX_LUT_8_ADDR                       (CONN_PTA6_BASE + 0x0170)
#define CONN_PTA6_ARB_RO_COEX_LUT_9_ADDR                       (CONN_PTA6_BASE + 0x0174)
#define CONN_PTA6_ARB_RO_COEX_LUT_10_ADDR                      (CONN_PTA6_BASE + 0x0178)
#define CONN_PTA6_ARB_RO_COEX_LUT_11_ADDR                      (CONN_PTA6_BASE + 0x017C)
#define CONN_PTA6_ARB_RO_COEX_LUT_12_ADDR                      (CONN_PTA6_BASE + 0x0180)
#define CONN_PTA6_ARB_RO_COEX_LUT_13_ADDR                      (CONN_PTA6_BASE + 0x0184)
#define CONN_PTA6_ARB_RO_COEX_LUT_14_ADDR                      (CONN_PTA6_BASE + 0x0188)
#define CONN_PTA6_ARB_RO_COEX_LUT_15_ADDR                      (CONN_PTA6_BASE + 0x018C)
#define CONN_PTA6_ARB_RO_COEX_LUT_16_ADDR                      (CONN_PTA6_BASE + 0x0190)
#define CONN_PTA6_ARB_RO_COEX_LUT_17_ADDR                      (CONN_PTA6_BASE + 0x0194)
#define CONN_PTA6_ARB_RO_COEX_LUT_18_ADDR                      (CONN_PTA6_BASE + 0x0198)
#define CONN_PTA6_ARB_RO_COEX_LUT_19_ADDR                      (CONN_PTA6_BASE + 0x019C)
#define CONN_PTA6_ARB_RO_REQ_ADDR                              (CONN_PTA6_BASE + 0x01BC)
#define CONN_PTA6_ARB_RO_GRNT_ADDR                             (CONN_PTA6_BASE + 0x01C0)
#define CONN_PTA6_ARB_RO_PRI_IN_0_ADDR                         (CONN_PTA6_BASE + 0x01C4)
#define CONN_PTA6_ARB_RO_PRI_IN_1_ADDR                         (CONN_PTA6_BASE + 0x01C8)
#define CONN_PTA6_ARB_RO_PRI_IN_2_ADDR                         (CONN_PTA6_BASE + 0x01CC)
#define CONN_PTA6_ARB_RO_DBG_0_ADDR                            (CONN_PTA6_BASE + 0x01D0)
#define CONN_PTA6_ARB_RO_DBG_1_ADDR                            (CONN_PTA6_BASE + 0x01D4)
#define CONN_PTA6_ARB_RO_DBG_2_ADDR                            (CONN_PTA6_BASE + 0x01D8)
#define CONN_PTA6_ARB_RO_DBG_3_ADDR                            (CONN_PTA6_BASE + 0x01DC)
#define CONN_PTA6_ARB_RO_DBG_4_ADDR                            (CONN_PTA6_BASE + 0x01E0)
#define CONN_PTA6_ARB_RO_DBG_5_ADDR                            (CONN_PTA6_BASE + 0x01E4)
#define CONN_PTA6_ARB_RO_DBG_6_ADDR                            (CONN_PTA6_BASE + 0x01E8)
#define CONN_PTA6_RW_CTRL_ADDR                                 (CONN_PTA6_BASE + 0x0200)
#define CONN_PTA6_RW_CFG_0_ADDR                                (CONN_PTA6_BASE + 0x0204)
#define CONN_PTA6_RW_CFG_1_ADDR                                (CONN_PTA6_BASE + 0x0208)
#define CONN_PTA6_RW_RO_STATUS_0_ADDR                          (CONN_PTA6_BASE + 0x020C)
#define CONN_PTA6_RW_RO_STATUS_1_ADDR                          (CONN_PTA6_BASE + 0x0210)
#define CONN_PTA6_RW_TDM_THLD_0_ADDR                           (CONN_PTA6_BASE + 0x0214)
#define CONN_PTA6_RW_TDM_THLD_1_ADDR                           (CONN_PTA6_BASE + 0x0218)
#define CONN_PTA6_RW_TDM_THLD_2_ADDR                           (CONN_PTA6_BASE + 0x021C)
#define CONN_PTA6_MISC_BT_CTRL_0_ADDR                          (CONN_PTA6_BASE + 0x0230)
#define CONN_PTA6_MISC_BT_CTRL_1_ADDR                          (CONN_PTA6_BASE + 0x0234)
#define CONN_PTA6_MISC_WF_CTRL_ADDR                            (CONN_PTA6_BASE + 0x0238)
#define CONN_PTA6_MISC_SX_SEL_ADDR                             (CONN_PTA6_BASE + 0x023C)
#define CONN_PTA6_GPS_BLANK_CFG_ADDR                           (CONN_PTA6_BASE + 0x0240)
#define CONN_PTA6_TMR_CTRL_0_ADDR                              (CONN_PTA6_BASE + 0x02C0)
#define CONN_PTA6_TMR_CTRL_1_ADDR                              (CONN_PTA6_BASE + 0x02C4)
#define CONN_PTA6_TMR_CTRL_2_ADDR                              (CONN_PTA6_BASE + 0x02C8)
#define CONN_PTA6_TMR_CTRL_3_ADDR                              (CONN_PTA6_BASE + 0x02CC)
#define CONN_PTA6_INT_MASK_0_ADDR                              (CONN_PTA6_BASE + 0x02D0)
#define CONN_PTA6_INT_MASK_1_ADDR                              (CONN_PTA6_BASE + 0x02D4)
#define CONN_PTA6_WF_INT_MASK_2_ADDR                           (CONN_PTA6_BASE + 0x02D8)
#define CONN_PTA6_BT_INT_MASK_2_ADDR                           (CONN_PTA6_BASE + 0x02DC)
#define CONN_PTA6_INT_STATUS_0_ADDR                            (CONN_PTA6_BASE + 0x02E0)
#define CONN_PTA6_INT_STATUS_1_ADDR                            (CONN_PTA6_BASE + 0x02E4)
#define CONN_PTA6_WF_INT_STATUS_2_ADDR                         (CONN_PTA6_BASE + 0x02E8)
#define CONN_PTA6_BT_INT_STATUS_2_ADDR                         (CONN_PTA6_BASE + 0x02EC)
#define CONN_PTA6_INT_CLR_0_ADDR                               (CONN_PTA6_BASE + 0x02F0)
#define CONN_PTA6_INT_CLR_1_ADDR                               (CONN_PTA6_BASE + 0x02F4)
#define CONN_PTA6_WF_INT_CLR_2_ADDR                            (CONN_PTA6_BASE + 0x02F8)
#define CONN_PTA6_BT_INT_CLR_2_ADDR                            (CONN_PTA6_BASE + 0x02FC)
#define CONN_PTA6_DBG_CNTR_CLR_ADDR                            (CONN_PTA6_BASE + 0x0300)
#define CONN_PTA6_RO_DBG_CNTR_0_ADDR                           (CONN_PTA6_BASE + 0x0304)
#define CONN_PTA6_RO_DBG_CNTR_1_ADDR                           (CONN_PTA6_BASE + 0x0308)
#define CONN_PTA6_RO_DBG_CNTR_2_ADDR                           (CONN_PTA6_BASE + 0x030C)
#define CONN_PTA6_RO_DBG_CNTR_3_ADDR                           (CONN_PTA6_BASE + 0x0310)
#define CONN_PTA6_RO_DBG_CNTR_4_ADDR                           (CONN_PTA6_BASE + 0x0314)
#define CONN_PTA6_RO_DBG_CNTR_5_ADDR                           (CONN_PTA6_BASE + 0x0318)
#define CONN_PTA6_RO_DBG_CNTR_6_ADDR                           (CONN_PTA6_BASE + 0x031C)
#define CONN_PTA6_RO_DBG_CNTR_8_ADDR                           (CONN_PTA6_BASE + 0x0324)
#define CONN_PTA6_RO_DBG_CNTR_9_ADDR                           (CONN_PTA6_BASE + 0x0328)
#define CONN_PTA6_RO_DBG_CNTR_10_ADDR                          (CONN_PTA6_BASE + 0x032C)
#define CONN_PTA6_RO_DBG_CNTR_11_ADDR                          (CONN_PTA6_BASE + 0x0330)
#define CONN_PTA6_RO_DBG_CNTR_12_ADDR                          (CONN_PTA6_BASE + 0x0334)
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_ADDR                     (CONN_PTA6_BASE + 0x0360)
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_ADDR                     (CONN_PTA6_BASE + 0x0364)
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_ADDR                     (CONN_PTA6_BASE + 0x0368)
#define CONN_PTA6_DBGMON_EN_ADDR                               (CONN_PTA6_BASE + 0x0370)
#define CONN_PTA6_DBGMON_SEL_ADDR                              (CONN_PTA6_BASE + 0x0374)
#define CONN_PTA6_DBGMON_BIT_SEL_0_ADDR                        (CONN_PTA6_BASE + 0x0378)
#define CONN_PTA6_DBGMON_BIT_SEL_1_ADDR                        (CONN_PTA6_BASE + 0x037C)
#define CONN_PTA6_DBGMON_BIT_SEL_2_ADDR                        (CONN_PTA6_BASE + 0x0380)
#define CONN_PTA6_DBGMON_BIT_SEL_3_ADDR                        (CONN_PTA6_BASE + 0x0384)
#define CONN_PTA6_DBGMON_BIT_SEL_4_ADDR                        (CONN_PTA6_BASE + 0x0388)
#define CONN_PTA6_DBGMON_BIT_SEL_5_ADDR                        (CONN_PTA6_BASE + 0x038C)
#define CONN_PTA6_DBGMON_BIT_SEL_6_ADDR                        (CONN_PTA6_BASE + 0x0390)
#define CONN_PTA6_DBGMON_BIT_SEL_7_ADDR                        (CONN_PTA6_BASE + 0x0394)
#define CONN_PTA6_DBGMON_OUT_ADDR                              (CONN_PTA6_BASE + 0x0398)
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ADDR                     (CONN_PTA6_BASE + 0x0400)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_0_ADDR                 (CONN_PTA6_BASE + 0x0404)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_1_ADDR                 (CONN_PTA6_BASE + 0x0408)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_2_ADDR                 (CONN_PTA6_BASE + 0x040C)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_0_ADDR                 (CONN_PTA6_BASE + 0x0410)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_1_ADDR                 (CONN_PTA6_BASE + 0x0414)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_2_ADDR                 (CONN_PTA6_BASE + 0x0418)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_0_ADDR                 (CONN_PTA6_BASE + 0x041C)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_1_ADDR                 (CONN_PTA6_BASE + 0x0420)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_2_ADDR                 (CONN_PTA6_BASE + 0x0424)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_0_ADDR                 (CONN_PTA6_BASE + 0x0428)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_1_ADDR                 (CONN_PTA6_BASE + 0x042C)
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_2_ADDR                 (CONN_PTA6_BASE + 0x0430)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_0_ADDR                   (CONN_PTA6_BASE + 0x0434)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_1_ADDR                   (CONN_PTA6_BASE + 0x0438)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_2_ADDR                   (CONN_PTA6_BASE + 0x043C)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_0_ADDR                   (CONN_PTA6_BASE + 0x0440)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_1_ADDR                   (CONN_PTA6_BASE + 0x0444)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_2_ADDR                   (CONN_PTA6_BASE + 0x0448)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_0_ADDR                   (CONN_PTA6_BASE + 0x044C)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_1_ADDR                   (CONN_PTA6_BASE + 0x0450)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_2_ADDR                   (CONN_PTA6_BASE + 0x0454)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_0_ADDR                   (CONN_PTA6_BASE + 0x0458)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_1_ADDR                   (CONN_PTA6_BASE + 0x045C)
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_2_ADDR                   (CONN_PTA6_BASE + 0x0460)
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ADDR                     (CONN_PTA6_BASE + 0x0464)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_0_ADDR                 (CONN_PTA6_BASE + 0x0468)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_1_ADDR                 (CONN_PTA6_BASE + 0x046C)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_2_ADDR                 (CONN_PTA6_BASE + 0x0470)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_0_ADDR                 (CONN_PTA6_BASE + 0x0474)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_1_ADDR                 (CONN_PTA6_BASE + 0x0478)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_2_ADDR                 (CONN_PTA6_BASE + 0x047C)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_0_ADDR                 (CONN_PTA6_BASE + 0x0480)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_1_ADDR                 (CONN_PTA6_BASE + 0x0484)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_2_ADDR                 (CONN_PTA6_BASE + 0x0488)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_0_ADDR                 (CONN_PTA6_BASE + 0x048C)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_1_ADDR                 (CONN_PTA6_BASE + 0x0490)
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_2_ADDR                 (CONN_PTA6_BASE + 0x0494)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_0_ADDR                   (CONN_PTA6_BASE + 0x0498)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_1_ADDR                   (CONN_PTA6_BASE + 0x049C)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_2_ADDR                   (CONN_PTA6_BASE + 0x04A0)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_0_ADDR                   (CONN_PTA6_BASE + 0x04A4)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_1_ADDR                   (CONN_PTA6_BASE + 0x04A8)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_2_ADDR                   (CONN_PTA6_BASE + 0x04AC)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_0_ADDR                   (CONN_PTA6_BASE + 0x04B0)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_1_ADDR                   (CONN_PTA6_BASE + 0x04B4)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_2_ADDR                   (CONN_PTA6_BASE + 0x04B8)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_0_ADDR                   (CONN_PTA6_BASE + 0x04BC)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_1_ADDR                   (CONN_PTA6_BASE + 0x04C0)
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_2_ADDR                   (CONN_PTA6_BASE + 0x04C4)
#define CONN_PTA6_IDC_CMD_FDD_CFG_ADDR                         (CONN_PTA6_BASE + 0x04D0)
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ADDR                   (CONN_PTA6_BASE + 0x04E0)
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ADDR                   (CONN_PTA6_BASE + 0x04E4)
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ADDR                   (CONN_PTA6_BASE + 0x04E8)
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ADDR                   (CONN_PTA6_BASE + 0x04EC)
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ADDR                   (CONN_PTA6_BASE + 0x04F0)
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ADDR                   (CONN_PTA6_BASE + 0x04F4)
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ADDR                   (CONN_PTA6_BASE + 0x04F8)
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ADDR                   (CONN_PTA6_BASE + 0x04FC)
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR                  (CONN_PTA6_BASE + 0x0500)
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_ADDR                  (CONN_PTA6_BASE + 0x0504)
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_ADDR                  (CONN_PTA6_BASE + 0x0508)
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_ADDR                  (CONN_PTA6_BASE + 0x050C)
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_ADDR                  (CONN_PTA6_BASE + 0x0510)
#define CONN_PTA6_WF_TX_IND_CFG_ADDR                           (CONN_PTA6_BASE + 0x0520)
#define CONN_PTA6_WF_IND_DBG_ADDR                              (CONN_PTA6_BASE + 0x0524)
#define CONN_PTA6_BT_IND_DBG_ADDR                              (CONN_PTA6_BASE + 0x0528)


#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_ap_en_ADDR    CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_ap_en_MASK    0x03F00000
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_ap_en_SHFT    20
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_uart_apb_hw_en_ADDR   CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_uart_apb_hw_en_MASK   0x00010000
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_uart_apb_hw_en_SHFT   16
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_wifi1_pta_en_ADDR     CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_wifi1_pta_en_MASK     0x00000200
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_wifi1_pta_en_SHFT     9
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_wifi0_pta_en_ADDR     CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_wifi0_pta_en_MASK     0x00000100
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_wifi0_pta_en_SHFT     8
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_en_ADDR       CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_en_MASK       0x000000FC
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_en_SHFT       2
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_en_pta_arb_ADDR       CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_en_pta_arb_MASK       0x00000002
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_en_pta_arb_SHFT       1
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_pta_en_ADDR           CONN_PTA6_WFSET_PTA_CTRL_ADDR
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_pta_en_MASK           0x00000001
#define CONN_PTA6_WFSET_PTA_CTRL_r_wfset_pta_en_SHFT           0

#define CONN_PTA6_PTA_CLK_CFG_r_pta_1m_cnt_ADDR                CONN_PTA6_PTA_CLK_CFG_ADDR
#define CONN_PTA6_PTA_CLK_CFG_r_pta_1m_cnt_MASK                0x000000FF
#define CONN_PTA6_PTA_CLK_CFG_r_pta_1m_cnt_SHFT                0

#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_lte_pta_ap_en_ADDR    CONN_PTA6_BTSET_PTA_CTRL_ADDR
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_lte_pta_ap_en_MASK    0x03F00000
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_lte_pta_ap_en_SHFT    20
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_uart_apb_hw_en_ADDR   CONN_PTA6_BTSET_PTA_CTRL_ADDR
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_uart_apb_hw_en_MASK   0x00010000
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_uart_apb_hw_en_SHFT   16
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_bt_pta_en_ADDR        CONN_PTA6_BTSET_PTA_CTRL_ADDR
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_bt_pta_en_MASK        0x00000400
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_bt_pta_en_SHFT        10
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_lte_pta_en_ADDR       CONN_PTA6_BTSET_PTA_CTRL_ADDR
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_lte_pta_en_MASK       0x000000FC
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_lte_pta_en_SHFT       2
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_en_pta_arb_ADDR       CONN_PTA6_BTSET_PTA_CTRL_ADDR
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_en_pta_arb_MASK       0x00000002
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_en_pta_arb_SHFT       1
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_pta_en_ADDR           CONN_PTA6_BTSET_PTA_CTRL_ADDR
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_pta_en_MASK           0x00000001
#define CONN_PTA6_BTSET_PTA_CTRL_r_btset_pta_en_SHFT           0

#define CONN_PTA6_RO_PTA_CTRL_ro_lte_pta_ap_en_ADDR            CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_lte_pta_ap_en_MASK            0x03F00000
#define CONN_PTA6_RO_PTA_CTRL_ro_lte_pta_ap_en_SHFT            20
#define CONN_PTA6_RO_PTA_CTRL_ro_uart_apb_hw_en_ADDR           CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_uart_apb_hw_en_MASK           0x00010000
#define CONN_PTA6_RO_PTA_CTRL_ro_uart_apb_hw_en_SHFT           16
#define CONN_PTA6_RO_PTA_CTRL_ro_bt_pta_en_ADDR                CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_bt_pta_en_MASK                0x00000400
#define CONN_PTA6_RO_PTA_CTRL_ro_bt_pta_en_SHFT                10
#define CONN_PTA6_RO_PTA_CTRL_ro_wifi1_pta_en_ADDR             CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_wifi1_pta_en_MASK             0x00000200
#define CONN_PTA6_RO_PTA_CTRL_ro_wifi1_pta_en_SHFT             9
#define CONN_PTA6_RO_PTA_CTRL_ro_wifi0_pta_en_ADDR             CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_wifi0_pta_en_MASK             0x00000100
#define CONN_PTA6_RO_PTA_CTRL_ro_wifi0_pta_en_SHFT             8
#define CONN_PTA6_RO_PTA_CTRL_ro_lte_pta_en_ADDR               CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_lte_pta_en_MASK               0x000000FC
#define CONN_PTA6_RO_PTA_CTRL_ro_lte_pta_en_SHFT               2
#define CONN_PTA6_RO_PTA_CTRL_ro_en_pta_arb_ADDR               CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_en_pta_arb_MASK               0x00000002
#define CONN_PTA6_RO_PTA_CTRL_ro_en_pta_arb_SHFT               1
#define CONN_PTA6_RO_PTA_CTRL_ro_pta_en_ADDR                   CONN_PTA6_RO_PTA_CTRL_ADDR
#define CONN_PTA6_RO_PTA_CTRL_ro_pta_en_MASK                   0x00000001
#define CONN_PTA6_RO_PTA_CTRL_ro_pta_en_SHFT                   0

#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_4b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_4b_MASK    0xFF000000
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_4b_SHFT    24
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_3b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_3b_MASK    0x00FF0000
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_3b_SHFT    16
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_2b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_2b_MASK    0x0000FF00
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_2b_SHFT    8
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_1b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_1b_MASK    0x000000FF
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_0_r_idc_cmd_in_1b_SHFT    0

#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_8b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_8b_MASK    0xFF000000
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_8b_SHFT    24
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_7b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_7b_MASK    0x00FF0000
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_7b_SHFT    16
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_6b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_6b_MASK    0x0000FF00
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_6b_SHFT    8
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_5b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_5b_MASK    0x000000FF
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_1_r_idc_cmd_in_5b_SHFT    0

#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_r_idc_cmd_in_gen_ADDR   CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_r_idc_cmd_in_gen_MASK   0x00000100
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_r_idc_cmd_in_gen_SHFT   8
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_r_idc_cmd_in_9b_ADDR    CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_ADDR
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_r_idc_cmd_in_9b_MASK    0x000000FF
#define CONN_PTA6_LTE_CMD_RX_DBG_GEN_2_r_idc_cmd_in_9b_SHFT    0

#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_4b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_4b_MASK            0xFF000000
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_4b_SHFT            24
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_3b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_3b_MASK            0x00FF0000
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_3b_SHFT            16
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_2b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_2b_MASK            0x0000FF00
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_2b_SHFT            8
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_1b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_1b_MASK            0x000000FF
#define CONN_PTA6_RO_LTE_CMD_RX_0_ro_rx_idc_1b_SHFT            0

#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_8b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_8b_MASK            0xFF000000
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_8b_SHFT            24
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_7b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_7b_MASK            0x00FF0000
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_7b_SHFT            16
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_6b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_6b_MASK            0x0000FF00
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_6b_SHFT            8
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_5b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_5b_MASK            0x000000FF
#define CONN_PTA6_RO_LTE_CMD_RX_1_ro_rx_idc_5b_SHFT            0

#define CONN_PTA6_RO_LTE_CMD_RX_2_ro_uart_rx_err_sts_ADDR      CONN_PTA6_RO_LTE_CMD_RX_2_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_2_ro_uart_rx_err_sts_MASK      0x0FFF0000
#define CONN_PTA6_RO_LTE_CMD_RX_2_ro_uart_rx_err_sts_SHFT      16
#define CONN_PTA6_RO_LTE_CMD_RX_2_ro_rx_idc_9b_ADDR            CONN_PTA6_RO_LTE_CMD_RX_2_ADDR
#define CONN_PTA6_RO_LTE_CMD_RX_2_ro_rx_idc_9b_MASK            0x000000FF
#define CONN_PTA6_RO_LTE_CMD_RX_2_ro_rx_idc_9b_SHFT            0

#define CONN_PTA6_LTE_CMD_TX_CFG_0_r_seq_num_roll_stop_ADDR    CONN_PTA6_LTE_CMD_TX_CFG_0_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_0_r_seq_num_roll_stop_MASK    0x00001000
#define CONN_PTA6_LTE_CMD_TX_CFG_0_r_seq_num_roll_stop_SHFT    12
#define CONN_PTA6_LTE_CMD_TX_CFG_0_r_idc_tx_block_bmp_ADDR     CONN_PTA6_LTE_CMD_TX_CFG_0_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_0_r_idc_tx_block_bmp_MASK     0x000007FF
#define CONN_PTA6_LTE_CMD_TX_CFG_0_r_idc_tx_block_bmp_SHFT     0

#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_set_ADDR              CONN_PTA6_LTE_CMD_TX_CFG_1_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_set_MASK              0x00040000
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_set_SHFT              18
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_rsvd_ADDR             CONN_PTA6_LTE_CMD_TX_CFG_1_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_rsvd_MASK             0x00020000
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_rsvd_SHFT             17
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_dur_ADDR              CONN_PTA6_LTE_CMD_TX_CFG_1_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_dur_MASK              0x0001FFE0
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_dur_SHFT              5
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_owner_ADDR            CONN_PTA6_LTE_CMD_TX_CFG_1_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_owner_MASK            0x0000001F
#define CONN_PTA6_LTE_CMD_TX_CFG_1_r_tdm_owner_SHFT            0

#define CONN_PTA6_LTE_CMD_TX_CFG_2_r_seq_num_str_upd_ADDR      CONN_PTA6_LTE_CMD_TX_CFG_2_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_2_r_seq_num_str_upd_MASK      0x00000010
#define CONN_PTA6_LTE_CMD_TX_CFG_2_r_seq_num_str_upd_SHFT      4
#define CONN_PTA6_LTE_CMD_TX_CFG_2_r_seq_num_str_ADDR          CONN_PTA6_LTE_CMD_TX_CFG_2_ADDR
#define CONN_PTA6_LTE_CMD_TX_CFG_2_r_seq_num_str_MASK          0x00000007
#define CONN_PTA6_LTE_CMD_TX_CFG_2_r_seq_num_str_SHFT          0

#define CONN_PTA6_RO_LTE_CMD_TX_SEQ_ro_seq_num_ADDR            CONN_PTA6_RO_LTE_CMD_TX_SEQ_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_SEQ_ro_seq_num_MASK            0x00000007
#define CONN_PTA6_RO_LTE_CMD_TX_SEQ_ro_seq_num_SHFT            0

#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_4b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_4b_MASK 0xFF000000
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_4b_SHFT 24
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_3b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_3b_MASK 0x00FF0000
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_3b_SHFT 16
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_2b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_2b_MASK 0x0000FF00
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_2b_SHFT 8
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_1b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_1b_MASK 0x000000FF
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_0_r_wf_idc_cmd_out_1b_SHFT 0

#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_8b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_8b_MASK 0xFF000000
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_8b_SHFT 24
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_7b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_7b_MASK 0x00FF0000
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_7b_SHFT 16
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_6b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_6b_MASK 0x0000FF00
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_6b_SHFT 8
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_5b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_5b_MASK 0x000000FF
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_1_r_wf_idc_cmd_out_5b_SHFT 0

#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_r_wf_idc_cmd_out_gen_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_2_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_r_wf_idc_cmd_out_gen_MASK 0x00000100
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_r_wf_idc_cmd_out_gen_SHFT 8
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_r_wf_idc_cmd_out_9b_ADDR CONN_PTA6_WF2LTE_CMD_TX_GEN_2_ADDR
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_r_wf_idc_cmd_out_9b_MASK 0x000000FF
#define CONN_PTA6_WF2LTE_CMD_TX_GEN_2_r_wf_idc_cmd_out_9b_SHFT 0

#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_4b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_4b_MASK            0xFF000000
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_4b_SHFT            24
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_3b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_3b_MASK            0x00FF0000
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_3b_SHFT            16
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_2b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_2b_MASK            0x0000FF00
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_2b_SHFT            8
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_1b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_0_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_1b_MASK            0x000000FF
#define CONN_PTA6_RO_LTE_CMD_TX_0_ro_tx_idc_1b_SHFT            0

#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_8b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_8b_MASK            0xFF000000
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_8b_SHFT            24
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_7b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_7b_MASK            0x00FF0000
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_7b_SHFT            16
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_6b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_6b_MASK            0x0000FF00
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_6b_SHFT            8
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_5b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_1_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_5b_MASK            0x000000FF
#define CONN_PTA6_RO_LTE_CMD_TX_1_ro_tx_idc_5b_SHFT            0

#define CONN_PTA6_RO_LTE_CMD_TX_2_ro_tx_idc_9b_ADDR            CONN_PTA6_RO_LTE_CMD_TX_2_ADDR
#define CONN_PTA6_RO_LTE_CMD_TX_2_ro_tx_idc_9b_MASK            0x000000FF
#define CONN_PTA6_RO_LTE_CMD_TX_2_ro_tx_idc_9b_SHFT            0

#define CONN_PTA6_RO_RX_DEC_DBG_0_ro_rx_dec_dbg_info_0_ADDR    CONN_PTA6_RO_RX_DEC_DBG_0_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_0_ro_rx_dec_dbg_info_0_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_0_ro_rx_dec_dbg_info_0_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_1_ro_rx_dec_dbg_info_1_ADDR    CONN_PTA6_RO_RX_DEC_DBG_1_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_1_ro_rx_dec_dbg_info_1_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_1_ro_rx_dec_dbg_info_1_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_2_ro_rx_dec_dbg_info_2_ADDR    CONN_PTA6_RO_RX_DEC_DBG_2_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_2_ro_rx_dec_dbg_info_2_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_2_ro_rx_dec_dbg_info_2_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_3_ro_rx_dec_dbg_info_3_ADDR    CONN_PTA6_RO_RX_DEC_DBG_3_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_3_ro_rx_dec_dbg_info_3_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_3_ro_rx_dec_dbg_info_3_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_4_ro_rx_dec_dbg_info_4_ADDR    CONN_PTA6_RO_RX_DEC_DBG_4_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_4_ro_rx_dec_dbg_info_4_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_4_ro_rx_dec_dbg_info_4_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_5_ro_rx_dec_dbg_info_5_ADDR    CONN_PTA6_RO_RX_DEC_DBG_5_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_5_ro_rx_dec_dbg_info_5_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_5_ro_rx_dec_dbg_info_5_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_6_ro_rx_dec_dbg_info_6_ADDR    CONN_PTA6_RO_RX_DEC_DBG_6_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_6_ro_rx_dec_dbg_info_6_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_6_ro_rx_dec_dbg_info_6_SHFT    0

#define CONN_PTA6_RO_RX_DEC_DBG_7_ro_rx_dec_dbg_info_7_ADDR    CONN_PTA6_RO_RX_DEC_DBG_7_ADDR
#define CONN_PTA6_RO_RX_DEC_DBG_7_ro_rx_dec_dbg_info_7_MASK    0xFFFFFFFF
#define CONN_PTA6_RO_RX_DEC_DBG_7_ro_rx_dec_dbg_info_7_SHFT    0

#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_vld_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_vld_MASK         0x80000000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_vld_SHFT         31
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_type_ADDR        CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_type_MASK        0x40000000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_type_SHFT        30
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_dur_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_dur_MASK         0x3FFF0000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_prd0_dur_SHFT         16
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_lte_frmup_cmp_ADDR    CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_lte_frmup_cmp_MASK    0x0000003F
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_0_r_lte_frmup_cmp_SHFT    0

#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_vld_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_vld_MASK         0x80000000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_vld_SHFT         31
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_type_ADDR        CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_type_MASK        0x40000000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_type_SHFT        30
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_dur_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_dur_MASK         0x3FFF0000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd2_dur_SHFT         16
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_vld_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_vld_MASK         0x00008000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_vld_SHFT         15
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_type_ADDR        CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_type_MASK        0x00004000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_type_SHFT        14
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_dur_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_dur_MASK         0x00003FFF
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_1_r_prd1_dur_SHFT         0

#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_vld_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_vld_MASK         0x80000000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_vld_SHFT         31
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_type_ADDR        CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_type_MASK        0x40000000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_type_SHFT        30
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_dur_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_dur_MASK         0x3FFF0000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd4_dur_SHFT         16
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_vld_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_vld_MASK         0x00008000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_vld_SHFT         15
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_type_ADDR        CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_type_MASK        0x00004000
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_type_SHFT        14
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_dur_ADDR         CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_ADDR
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_dur_MASK         0x00003FFF
#define CONN_PTA6_LTE_RX_REQ_GEN_CFG_2_r_prd3_dur_SHFT         0

#define CONN_PTA6_REQ_GEN_CTRL_r_wifi_ext_cyc_ADDR             CONN_PTA6_REQ_GEN_CTRL_ADDR
#define CONN_PTA6_REQ_GEN_CTRL_r_wifi_ext_cyc_MASK             0x0F000000
#define CONN_PTA6_REQ_GEN_CTRL_r_wifi_ext_cyc_SHFT             24
#define CONN_PTA6_REQ_GEN_CTRL_r_en_bt_req_gen_ADDR            CONN_PTA6_REQ_GEN_CTRL_ADDR
#define CONN_PTA6_REQ_GEN_CTRL_r_en_bt_req_gen_MASK            0x00010000
#define CONN_PTA6_REQ_GEN_CTRL_r_en_bt_req_gen_SHFT            16
#define CONN_PTA6_REQ_GEN_CTRL_r_en_wf1_req_gen_ADDR           CONN_PTA6_REQ_GEN_CTRL_ADDR
#define CONN_PTA6_REQ_GEN_CTRL_r_en_wf1_req_gen_MASK           0x00002000
#define CONN_PTA6_REQ_GEN_CTRL_r_en_wf1_req_gen_SHFT           13
#define CONN_PTA6_REQ_GEN_CTRL_r_en_wf0_req_gen_ADDR           CONN_PTA6_REQ_GEN_CTRL_ADDR
#define CONN_PTA6_REQ_GEN_CTRL_r_en_wf0_req_gen_MASK           0x00001000
#define CONN_PTA6_REQ_GEN_CTRL_r_en_wf0_req_gen_SHFT           12
#define CONN_PTA6_REQ_GEN_CTRL_r_en_lte_req_gen_ADDR           CONN_PTA6_REQ_GEN_CTRL_ADDR
#define CONN_PTA6_REQ_GEN_CTRL_r_en_lte_req_gen_MASK           0x00000FF0
#define CONN_PTA6_REQ_GEN_CTRL_r_en_lte_req_gen_SHFT           4
#define CONN_PTA6_REQ_GEN_CTRL_r_en_req_gen_ADDR               CONN_PTA6_REQ_GEN_CTRL_ADDR
#define CONN_PTA6_REQ_GEN_CTRL_r_en_req_gen_MASK               0x00000001
#define CONN_PTA6_REQ_GEN_CTRL_r_en_req_gen_SHFT               0

#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_bt_rx_req_ADDR         CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_bt_rx_req_MASK         0x00200000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_bt_rx_req_SHFT         21
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_bt_tx_req_ADDR         CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_bt_tx_req_MASK         0x00100000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_bt_tx_req_SHFT         20
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi1_rx_req_ADDR      CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi1_rx_req_MASK      0x00080000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi1_rx_req_SHFT      19
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi1_tx_req_ADDR      CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi1_tx_req_MASK      0x00040000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi1_tx_req_SHFT      18
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi0_rx_req_ADDR      CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi0_rx_req_MASK      0x00020000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi0_rx_req_SHFT      17
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi0_tx_req_ADDR      CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi0_tx_req_MASK      0x00010000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_wifi0_tx_req_SHFT      16
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte7_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte7_rx_req_MASK       0x00002000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte7_rx_req_SHFT       13
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte6_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte6_rx_req_MASK       0x00001000
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte6_rx_req_SHFT       12
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte5_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte5_rx_req_MASK       0x00000800
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte5_rx_req_SHFT       11
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte5_tx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte5_tx_req_MASK       0x00000400
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte5_tx_req_SHFT       10
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte4_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte4_rx_req_MASK       0x00000200
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte4_rx_req_SHFT       9
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte4_tx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte4_tx_req_MASK       0x00000100
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte4_tx_req_SHFT       8
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte3_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte3_rx_req_MASK       0x00000080
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte3_rx_req_SHFT       7
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte3_tx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte3_tx_req_MASK       0x00000040
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte3_tx_req_SHFT       6
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte2_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte2_rx_req_MASK       0x00000020
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte2_rx_req_SHFT       5
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte2_tx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte2_tx_req_MASK       0x00000010
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte2_tx_req_SHFT       4
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte1_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte1_rx_req_MASK       0x00000008
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte1_rx_req_SHFT       3
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte1_tx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte1_tx_req_MASK       0x00000004
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte1_tx_req_SHFT       2
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte0_rx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte0_rx_req_MASK       0x00000002
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte0_rx_req_SHFT       1
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte0_tx_req_ADDR       CONN_PTA6_REQ_GEN_DBG_CFG_REQ_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte0_tx_req_MASK       0x00000001
#define CONN_PTA6_REQ_GEN_DBG_CFG_REQ_r_lte0_tx_req_SHFT       0

#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte7_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte7_rx_2b_pri_MASK 0x0C000000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte7_rx_2b_pri_SHFT 26
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte6_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte6_rx_2b_pri_MASK 0x03000000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte6_rx_2b_pri_SHFT 24
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte5_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte5_rx_2b_pri_MASK 0x00C00000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte5_rx_2b_pri_SHFT 22
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte5_tx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte5_tx_2b_pri_MASK 0x00300000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte5_tx_2b_pri_SHFT 20
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte4_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte4_rx_2b_pri_MASK 0x000C0000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte4_rx_2b_pri_SHFT 18
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte4_tx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte4_tx_2b_pri_MASK 0x00030000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte4_tx_2b_pri_SHFT 16
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte3_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte3_rx_2b_pri_MASK 0x0000C000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte3_rx_2b_pri_SHFT 14
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte3_tx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte3_tx_2b_pri_MASK 0x00003000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte3_tx_2b_pri_SHFT 12
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte2_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte2_rx_2b_pri_MASK 0x00000C00
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte2_rx_2b_pri_SHFT 10
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte2_tx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte2_tx_2b_pri_MASK 0x00000300
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte2_tx_2b_pri_SHFT 8
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte1_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte1_rx_2b_pri_MASK 0x000000C0
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte1_rx_2b_pri_SHFT 6
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte1_tx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte1_tx_2b_pri_MASK 0x00000030
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte1_tx_2b_pri_SHFT 4
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte0_rx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte0_rx_2b_pri_MASK 0x0000000C
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte0_rx_2b_pri_SHFT 2
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte0_tx_2b_pri_ADDR CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte0_tx_2b_pri_MASK 0x00000003
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_0_r_sw_lte0_tx_2b_pri_SHFT 0

#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi1_pri_upd_ADDR   CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi1_pri_upd_MASK   0x80000000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi1_pri_upd_SHFT   31
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi0_pri_upd_ADDR   CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi0_pri_upd_MASK   0x40000000
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi0_pri_upd_SHFT   30
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_bt_pri_tag_ADDR      CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_bt_pri_tag_MASK      0x00000F00
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_bt_pri_tag_SHFT      8
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi1_pri_tag_ADDR   CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi1_pri_tag_MASK   0x000000F0
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi1_pri_tag_SHFT   4
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi0_pri_tag_ADDR   CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_ADDR
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi0_pri_tag_MASK   0x0000000F
#define CONN_PTA6_REQ_GEN_DBG_CFG_PRI_1_r_wifi0_pri_tag_SHFT   0

#define CONN_PTA6_RO_VER_CODE_ro_ver_a_ADDR                    CONN_PTA6_RO_VER_CODE_ADDR
#define CONN_PTA6_RO_VER_CODE_ro_ver_a_MASK                    0xFF000000
#define CONN_PTA6_RO_VER_CODE_ro_ver_a_SHFT                    24
#define CONN_PTA6_RO_VER_CODE_ro_ver_b_ADDR                    CONN_PTA6_RO_VER_CODE_ADDR
#define CONN_PTA6_RO_VER_CODE_ro_ver_b_MASK                    0x00FF0000
#define CONN_PTA6_RO_VER_CODE_ro_ver_b_SHFT                    16
#define CONN_PTA6_RO_VER_CODE_ro_ver_c_ADDR                    CONN_PTA6_RO_VER_CODE_ADDR
#define CONN_PTA6_RO_VER_CODE_ro_ver_c_MASK                    0x0000FF00
#define CONN_PTA6_RO_VER_CODE_ro_ver_c_SHFT                    8
#define CONN_PTA6_RO_VER_CODE_ro_ver_d_ADDR                    CONN_PTA6_RO_VER_CODE_ADDR
#define CONN_PTA6_RO_VER_CODE_ro_ver_d_MASK                    0x000000FF
#define CONN_PTA6_RO_VER_CODE_ro_ver_d_SHFT                    0

#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri3_map_ADDR              CONN_PTA6_LTE_PRI_MAP_ADDR
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri3_map_MASK              0x0000F000
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri3_map_SHFT              12
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri2_map_ADDR              CONN_PTA6_LTE_PRI_MAP_ADDR
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri2_map_MASK              0x00000F00
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri2_map_SHFT              8
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri1_map_ADDR              CONN_PTA6_LTE_PRI_MAP_ADDR
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri1_map_MASK              0x000000F0
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri1_map_SHFT              4
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri0_map_ADDR              CONN_PTA6_LTE_PRI_MAP_ADDR
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri0_map_MASK              0x0000000F
#define CONN_PTA6_LTE_PRI_MAP_r_lte_pri0_map_SHFT              0

#define CONN_PTA6_LTE_PRI_CFG_r_lte5_rx_2b_pri_ADDR            CONN_PTA6_LTE_PRI_CFG_ADDR
#define CONN_PTA6_LTE_PRI_CFG_r_lte5_rx_2b_pri_MASK            0x00300000
#define CONN_PTA6_LTE_PRI_CFG_r_lte5_rx_2b_pri_SHFT            20
#define CONN_PTA6_LTE_PRI_CFG_r_lte4_rx_2b_pri_ADDR            CONN_PTA6_LTE_PRI_CFG_ADDR
#define CONN_PTA6_LTE_PRI_CFG_r_lte4_rx_2b_pri_MASK            0x00030000
#define CONN_PTA6_LTE_PRI_CFG_r_lte4_rx_2b_pri_SHFT            16
#define CONN_PTA6_LTE_PRI_CFG_r_lte3_rx_2b_pri_ADDR            CONN_PTA6_LTE_PRI_CFG_ADDR
#define CONN_PTA6_LTE_PRI_CFG_r_lte3_rx_2b_pri_MASK            0x00003000
#define CONN_PTA6_LTE_PRI_CFG_r_lte3_rx_2b_pri_SHFT            12
#define CONN_PTA6_LTE_PRI_CFG_r_lte2_rx_2b_pri_ADDR            CONN_PTA6_LTE_PRI_CFG_ADDR
#define CONN_PTA6_LTE_PRI_CFG_r_lte2_rx_2b_pri_MASK            0x00000300
#define CONN_PTA6_LTE_PRI_CFG_r_lte2_rx_2b_pri_SHFT            8
#define CONN_PTA6_LTE_PRI_CFG_r_lte1_rx_2b_pri_ADDR            CONN_PTA6_LTE_PRI_CFG_ADDR
#define CONN_PTA6_LTE_PRI_CFG_r_lte1_rx_2b_pri_MASK            0x00000030
#define CONN_PTA6_LTE_PRI_CFG_r_lte1_rx_2b_pri_SHFT            4
#define CONN_PTA6_LTE_PRI_CFG_r_lte0_rx_2b_pri_ADDR            CONN_PTA6_LTE_PRI_CFG_ADDR
#define CONN_PTA6_LTE_PRI_CFG_r_lte0_rx_2b_pri_MASK            0x00000003
#define CONN_PTA6_LTE_PRI_CFG_r_lte0_rx_2b_pri_SHFT            0

#define CONN_PTA6_PRI_EXT_r_ext1_pri_ext_ADDR                  CONN_PTA6_PRI_EXT_ADDR
#define CONN_PTA6_PRI_EXT_r_ext1_pri_ext_MASK                  0x00070000
#define CONN_PTA6_PRI_EXT_r_ext1_pri_ext_SHFT                  16
#define CONN_PTA6_PRI_EXT_r_ext0_pri_ext_ADDR                  CONN_PTA6_PRI_EXT_ADDR
#define CONN_PTA6_PRI_EXT_r_ext0_pri_ext_MASK                  0x00007000
#define CONN_PTA6_PRI_EXT_r_ext0_pri_ext_SHFT                  12
#define CONN_PTA6_PRI_EXT_r_bt_pri_ext_ADDR                    CONN_PTA6_PRI_EXT_ADDR
#define CONN_PTA6_PRI_EXT_r_bt_pri_ext_MASK                    0x00000700
#define CONN_PTA6_PRI_EXT_r_bt_pri_ext_SHFT                    8
#define CONN_PTA6_PRI_EXT_r_wf_pri_ext_ADDR                    CONN_PTA6_PRI_EXT_ADDR
#define CONN_PTA6_PRI_EXT_r_wf_pri_ext_MASK                    0x00000070
#define CONN_PTA6_PRI_EXT_r_wf_pri_ext_SHFT                    4
#define CONN_PTA6_PRI_EXT_r_lte_pri_ext_ADDR                   CONN_PTA6_PRI_EXT_ADDR
#define CONN_PTA6_PRI_EXT_r_lte_pri_ext_MASK                   0x00000007
#define CONN_PTA6_PRI_EXT_r_lte_pri_ext_SHFT                   0

#define CONN_PTA6_ARB_CTRL_r_coex_lut_upd_logic_ADDR           CONN_PTA6_ARB_CTRL_ADDR
#define CONN_PTA6_ARB_CTRL_r_coex_lut_upd_logic_MASK           0x00000010
#define CONN_PTA6_ARB_CTRL_r_coex_lut_upd_logic_SHFT           4
#define CONN_PTA6_ARB_CTRL_r_wf0_bt_same_pri_en_ADDR           CONN_PTA6_ARB_CTRL_ADDR
#define CONN_PTA6_ARB_CTRL_r_wf0_bt_same_pri_en_MASK           0x00000008
#define CONN_PTA6_ARB_CTRL_r_wf0_bt_same_pri_en_SHFT           3
#define CONN_PTA6_ARB_CTRL_r_wf1_bt_same_pri_en_ADDR           CONN_PTA6_ARB_CTRL_ADDR
#define CONN_PTA6_ARB_CTRL_r_wf1_bt_same_pri_en_MASK           0x00000004
#define CONN_PTA6_ARB_CTRL_r_wf1_bt_same_pri_en_SHFT           2
#define CONN_PTA6_ARB_CTRL_r_en_sw_grnt_ADDR                   CONN_PTA6_ARB_CTRL_ADDR
#define CONN_PTA6_ARB_CTRL_r_en_sw_grnt_MASK                   0x00000002
#define CONN_PTA6_ARB_CTRL_r_en_sw_grnt_SHFT                   1
#define CONN_PTA6_ARB_CTRL_r_coex_lut_upd_ADDR                 CONN_PTA6_ARB_CTRL_ADDR
#define CONN_PTA6_ARB_CTRL_r_coex_lut_upd_MASK                 0x00000001
#define CONN_PTA6_ARB_CTRL_r_coex_lut_upd_SHFT                 0

#define CONN_PTA6_ARB_COEX_LUT_CFG_0_r_coex_lte_tx0_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_0_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_0_r_coex_lte_tx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_0_r_coex_lte_tx0_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_1_r_coex_lte_rx0_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_1_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_1_r_coex_lte_rx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_1_r_coex_lte_rx0_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_2_r_coex_lte_tx1_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_2_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_2_r_coex_lte_tx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_2_r_coex_lte_tx1_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_3_r_coex_lte_rx1_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_3_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_3_r_coex_lte_rx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_3_r_coex_lte_rx1_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_4_r_coex_lte_tx2_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_4_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_4_r_coex_lte_tx2_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_4_r_coex_lte_tx2_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_5_r_coex_lte_rx2_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_5_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_5_r_coex_lte_rx2_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_5_r_coex_lte_rx2_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_6_r_coex_lte_tx3_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_6_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_6_r_coex_lte_tx3_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_6_r_coex_lte_tx3_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_7_r_coex_lte_rx3_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_7_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_7_r_coex_lte_rx3_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_7_r_coex_lte_rx3_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_8_r_coex_lte_tx4_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_8_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_8_r_coex_lte_tx4_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_8_r_coex_lte_tx4_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_9_r_coex_lte_rx4_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_9_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_9_r_coex_lte_rx4_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_9_r_coex_lte_rx4_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_10_r_coex_lte_tx5_ADDR      CONN_PTA6_ARB_COEX_LUT_CFG_10_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_10_r_coex_lte_tx5_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_10_r_coex_lte_tx5_SHFT      0

#define CONN_PTA6_ARB_COEX_LUT_CFG_11_r_coex_lte_rx5_ADDR      CONN_PTA6_ARB_COEX_LUT_CFG_11_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_11_r_coex_lte_rx5_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_11_r_coex_lte_rx5_SHFT      0

#define CONN_PTA6_ARB_COEX_LUT_CFG_12_r_coex_lte_rx6_ADDR      CONN_PTA6_ARB_COEX_LUT_CFG_12_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_12_r_coex_lte_rx6_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_12_r_coex_lte_rx6_SHFT      0

#define CONN_PTA6_ARB_COEX_LUT_CFG_13_r_coex_lte_rx7_ADDR      CONN_PTA6_ARB_COEX_LUT_CFG_13_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_13_r_coex_lte_rx7_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_13_r_coex_lte_rx7_SHFT      0

#define CONN_PTA6_ARB_COEX_LUT_CFG_14_r_coex_wf_tx0_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_14_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_14_r_coex_wf_tx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_14_r_coex_wf_tx0_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_15_r_coex_wf_rx0_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_15_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_15_r_coex_wf_rx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_15_r_coex_wf_rx0_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_16_r_coex_wf_tx1_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_16_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_16_r_coex_wf_tx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_16_r_coex_wf_tx1_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_17_r_coex_wf_rx1_ADDR       CONN_PTA6_ARB_COEX_LUT_CFG_17_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_17_r_coex_wf_rx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_17_r_coex_wf_rx1_SHFT       0

#define CONN_PTA6_ARB_COEX_LUT_CFG_18_r_coex_bt_tx_ADDR        CONN_PTA6_ARB_COEX_LUT_CFG_18_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_18_r_coex_bt_tx_MASK        0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_18_r_coex_bt_tx_SHFT        0

#define CONN_PTA6_ARB_COEX_LUT_CFG_19_r_coex_bt_rx_ADDR        CONN_PTA6_ARB_COEX_LUT_CFG_19_ADDR
#define CONN_PTA6_ARB_COEX_LUT_CFG_19_r_coex_bt_rx_MASK        0x00FFFFFF
#define CONN_PTA6_ARB_COEX_LUT_CFG_19_r_coex_bt_rx_SHFT        0

#define CONN_PTA6_ARB_DBG_GRNT_CFG_r_sw_grnt_ADDR              CONN_PTA6_ARB_DBG_GRNT_CFG_ADDR
#define CONN_PTA6_ARB_DBG_GRNT_CFG_r_sw_grnt_MASK              0x00FFFFFF
#define CONN_PTA6_ARB_DBG_GRNT_CFG_r_sw_grnt_SHFT              0

#define CONN_PTA6_ARB_RO_COEX_LUT_0_ro_coex_lte_tx0_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_0_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_0_ro_coex_lte_tx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_0_ro_coex_lte_tx0_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_1_ro_coex_lte_rx0_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_1_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_1_ro_coex_lte_rx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_1_ro_coex_lte_rx0_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_2_ro_coex_lte_tx1_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_2_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_2_ro_coex_lte_tx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_2_ro_coex_lte_tx1_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_3_ro_coex_lte_rx1_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_3_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_3_ro_coex_lte_rx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_3_ro_coex_lte_rx1_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_4_ro_coex_lte_tx2_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_4_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_4_ro_coex_lte_tx2_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_4_ro_coex_lte_tx2_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_5_ro_coex_lte_rx2_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_5_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_5_ro_coex_lte_rx2_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_5_ro_coex_lte_rx2_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_6_ro_coex_lte_tx3_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_6_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_6_ro_coex_lte_tx3_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_6_ro_coex_lte_tx3_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_7_ro_coex_lte_rx3_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_7_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_7_ro_coex_lte_rx3_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_7_ro_coex_lte_rx3_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_8_ro_coex_lte_tx4_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_8_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_8_ro_coex_lte_tx4_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_8_ro_coex_lte_tx4_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_9_ro_coex_lte_rx4_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_9_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_9_ro_coex_lte_rx4_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_9_ro_coex_lte_rx4_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_10_ro_coex_lte_tx5_ADDR      CONN_PTA6_ARB_RO_COEX_LUT_10_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_10_ro_coex_lte_tx5_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_10_ro_coex_lte_tx5_SHFT      0

#define CONN_PTA6_ARB_RO_COEX_LUT_11_ro_coex_lte_rx5_ADDR      CONN_PTA6_ARB_RO_COEX_LUT_11_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_11_ro_coex_lte_rx5_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_11_ro_coex_lte_rx5_SHFT      0

#define CONN_PTA6_ARB_RO_COEX_LUT_12_ro_coex_lte_rx6_ADDR      CONN_PTA6_ARB_RO_COEX_LUT_12_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_12_ro_coex_lte_rx6_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_12_ro_coex_lte_rx6_SHFT      0

#define CONN_PTA6_ARB_RO_COEX_LUT_13_ro_coex_lte_rx7_ADDR      CONN_PTA6_ARB_RO_COEX_LUT_13_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_13_ro_coex_lte_rx7_MASK      0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_13_ro_coex_lte_rx7_SHFT      0

#define CONN_PTA6_ARB_RO_COEX_LUT_14_ro_coex_wf_tx0_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_14_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_14_ro_coex_wf_tx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_14_ro_coex_wf_tx0_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_15_ro_coex_wf_rx0_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_15_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_15_ro_coex_wf_rx0_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_15_ro_coex_wf_rx0_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_16_ro_coex_wf_tx1_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_16_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_16_ro_coex_wf_tx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_16_ro_coex_wf_tx1_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_17_ro_coex_wf_rx1_ADDR       CONN_PTA6_ARB_RO_COEX_LUT_17_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_17_ro_coex_wf_rx1_MASK       0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_17_ro_coex_wf_rx1_SHFT       0

#define CONN_PTA6_ARB_RO_COEX_LUT_18_ro_coex_bt_tx_ADDR        CONN_PTA6_ARB_RO_COEX_LUT_18_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_18_ro_coex_bt_tx_MASK        0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_18_ro_coex_bt_tx_SHFT        0

#define CONN_PTA6_ARB_RO_COEX_LUT_19_ro_coex_bt_rx_ADDR        CONN_PTA6_ARB_RO_COEX_LUT_19_ADDR
#define CONN_PTA6_ARB_RO_COEX_LUT_19_ro_coex_bt_rx_MASK        0x00FFFFFF
#define CONN_PTA6_ARB_RO_COEX_LUT_19_ro_coex_bt_rx_SHFT        0

#define CONN_PTA6_ARB_RO_REQ_ro_arb_req_in_ADDR                CONN_PTA6_ARB_RO_REQ_ADDR
#define CONN_PTA6_ARB_RO_REQ_ro_arb_req_in_MASK                0x00FFFFFF
#define CONN_PTA6_ARB_RO_REQ_ro_arb_req_in_SHFT                0

#define CONN_PTA6_ARB_RO_GRNT_ro_arb_grnt_out_ADDR             CONN_PTA6_ARB_RO_GRNT_ADDR
#define CONN_PTA6_ARB_RO_GRNT_ro_arb_grnt_out_MASK             0x00FFFFFF
#define CONN_PTA6_ARB_RO_GRNT_ro_arb_grnt_out_SHFT             0

#define CONN_PTA6_ARB_RO_PRI_IN_0_ro_arb_pri_in_0_ADDR         CONN_PTA6_ARB_RO_PRI_IN_0_ADDR
#define CONN_PTA6_ARB_RO_PRI_IN_0_ro_arb_pri_in_0_MASK         0xFFFFFFFF
#define CONN_PTA6_ARB_RO_PRI_IN_0_ro_arb_pri_in_0_SHFT         0

#define CONN_PTA6_ARB_RO_PRI_IN_1_ro_arb_pri_in_1_ADDR         CONN_PTA6_ARB_RO_PRI_IN_1_ADDR
#define CONN_PTA6_ARB_RO_PRI_IN_1_ro_arb_pri_in_1_MASK         0xFFFFFFFF
#define CONN_PTA6_ARB_RO_PRI_IN_1_ro_arb_pri_in_1_SHFT         0

#define CONN_PTA6_ARB_RO_PRI_IN_2_ro_arb_pri_in_2_ADDR         CONN_PTA6_ARB_RO_PRI_IN_2_ADDR
#define CONN_PTA6_ARB_RO_PRI_IN_2_ro_arb_pri_in_2_MASK         0xFFFFFFFF
#define CONN_PTA6_ARB_RO_PRI_IN_2_ro_arb_pri_in_2_SHFT         0

#define CONN_PTA6_ARB_RO_DBG_0_ro_arb_dbg_0_ADDR               CONN_PTA6_ARB_RO_DBG_0_ADDR
#define CONN_PTA6_ARB_RO_DBG_0_ro_arb_dbg_0_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_0_ro_arb_dbg_0_SHFT               0

#define CONN_PTA6_ARB_RO_DBG_1_ro_arb_dbg_1_ADDR               CONN_PTA6_ARB_RO_DBG_1_ADDR
#define CONN_PTA6_ARB_RO_DBG_1_ro_arb_dbg_1_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_1_ro_arb_dbg_1_SHFT               0

#define CONN_PTA6_ARB_RO_DBG_2_ro_arb_dbg_2_ADDR               CONN_PTA6_ARB_RO_DBG_2_ADDR
#define CONN_PTA6_ARB_RO_DBG_2_ro_arb_dbg_2_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_2_ro_arb_dbg_2_SHFT               0

#define CONN_PTA6_ARB_RO_DBG_3_ro_arb_dbg_3_ADDR               CONN_PTA6_ARB_RO_DBG_3_ADDR
#define CONN_PTA6_ARB_RO_DBG_3_ro_arb_dbg_3_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_3_ro_arb_dbg_3_SHFT               0

#define CONN_PTA6_ARB_RO_DBG_4_ro_arb_dbg_4_ADDR               CONN_PTA6_ARB_RO_DBG_4_ADDR
#define CONN_PTA6_ARB_RO_DBG_4_ro_arb_dbg_4_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_4_ro_arb_dbg_4_SHFT               0

#define CONN_PTA6_ARB_RO_DBG_5_ro_arb_dbg_5_ADDR               CONN_PTA6_ARB_RO_DBG_5_ADDR
#define CONN_PTA6_ARB_RO_DBG_5_ro_arb_dbg_5_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_5_ro_arb_dbg_5_SHFT               0

#define CONN_PTA6_ARB_RO_DBG_6_ro_arb_dbg_6_ADDR               CONN_PTA6_ARB_RO_DBG_6_ADDR
#define CONN_PTA6_ARB_RO_DBG_6_ro_arb_dbg_6_MASK               0xFFFFFFFF
#define CONN_PTA6_ARB_RO_DBG_6_ro_arb_dbg_6_SHFT               0

#define CONN_PTA6_RW_CTRL_r_md2wf_rw1_mode_ADDR                CONN_PTA6_RW_CTRL_ADDR
#define CONN_PTA6_RW_CTRL_r_md2wf_rw1_mode_MASK                0x00C00000
#define CONN_PTA6_RW_CTRL_r_md2wf_rw1_mode_SHFT                22
#define CONN_PTA6_RW_CTRL_r_md2wf_rw0_mode_ADDR                CONN_PTA6_RW_CTRL_ADDR
#define CONN_PTA6_RW_CTRL_r_md2wf_rw0_mode_MASK                0x00300000
#define CONN_PTA6_RW_CTRL_r_md2wf_rw0_mode_SHFT                20
#define CONN_PTA6_RW_CTRL_r_sync_exp_rw_map_ADDR               CONN_PTA6_RW_CTRL_ADDR
#define CONN_PTA6_RW_CTRL_r_sync_exp_rw_map_MASK               0x0003F000
#define CONN_PTA6_RW_CTRL_r_sync_exp_rw_map_SHFT               12
#define CONN_PTA6_RW_CTRL_r_tdm2rw_map_ADDR                    CONN_PTA6_RW_CTRL_ADDR
#define CONN_PTA6_RW_CTRL_r_tdm2rw_map_MASK                    0x000001F0
#define CONN_PTA6_RW_CTRL_r_tdm2rw_map_SHFT                    4
#define CONN_PTA6_RW_CTRL_r_rw1_en_ADDR                        CONN_PTA6_RW_CTRL_ADDR
#define CONN_PTA6_RW_CTRL_r_rw1_en_MASK                        0x00000002
#define CONN_PTA6_RW_CTRL_r_rw1_en_SHFT                        1
#define CONN_PTA6_RW_CTRL_r_rw0_en_ADDR                        CONN_PTA6_RW_CTRL_ADDR
#define CONN_PTA6_RW_CTRL_r_rw0_en_MASK                        0x00000001
#define CONN_PTA6_RW_CTRL_r_rw0_en_SHFT                        0

#define CONN_PTA6_RW_CFG_0_r_tdm2rw_en_ADDR                    CONN_PTA6_RW_CFG_0_ADDR
#define CONN_PTA6_RW_CFG_0_r_tdm2rw_en_MASK                    0x10000000
#define CONN_PTA6_RW_CFG_0_r_tdm2rw_en_SHFT                    28
#define CONN_PTA6_RW_CFG_0_r_rw0_set_ADDR                      CONN_PTA6_RW_CFG_0_ADDR
#define CONN_PTA6_RW_CFG_0_r_rw0_set_MASK                      0x01000000
#define CONN_PTA6_RW_CFG_0_r_rw0_set_SHFT                      24
#define CONN_PTA6_RW_CFG_0_r_rw0_val_ADDR                      CONN_PTA6_RW_CFG_0_ADDR
#define CONN_PTA6_RW_CFG_0_r_rw0_val_MASK                      0x007FFFFF
#define CONN_PTA6_RW_CFG_0_r_rw0_val_SHFT                      0

#define CONN_PTA6_RW_CFG_1_r_rw1_set_ADDR                      CONN_PTA6_RW_CFG_1_ADDR
#define CONN_PTA6_RW_CFG_1_r_rw1_set_MASK                      0x01000000
#define CONN_PTA6_RW_CFG_1_r_rw1_set_SHFT                      24
#define CONN_PTA6_RW_CFG_1_r_rw1_val_ADDR                      CONN_PTA6_RW_CFG_1_ADDR
#define CONN_PTA6_RW_CFG_1_r_rw1_val_MASK                      0x007FFFFF
#define CONN_PTA6_RW_CFG_1_r_rw1_val_SHFT                      0

#define CONN_PTA6_RW_RO_STATUS_0_ro_rw0_status_ADDR            CONN_PTA6_RW_RO_STATUS_0_ADDR
#define CONN_PTA6_RW_RO_STATUS_0_ro_rw0_status_MASK            0x0F000000
#define CONN_PTA6_RW_RO_STATUS_0_ro_rw0_status_SHFT            24
#define CONN_PTA6_RW_RO_STATUS_0_ro_rw0_cnt_ADDR               CONN_PTA6_RW_RO_STATUS_0_ADDR
#define CONN_PTA6_RW_RO_STATUS_0_ro_rw0_cnt_MASK               0x007FFFFF
#define CONN_PTA6_RW_RO_STATUS_0_ro_rw0_cnt_SHFT               0

#define CONN_PTA6_RW_RO_STATUS_1_ro_rw1_status_ADDR            CONN_PTA6_RW_RO_STATUS_1_ADDR
#define CONN_PTA6_RW_RO_STATUS_1_ro_rw1_status_MASK            0x0F000000
#define CONN_PTA6_RW_RO_STATUS_1_ro_rw1_status_SHFT            24
#define CONN_PTA6_RW_RO_STATUS_1_ro_rw1_cnt_ADDR               CONN_PTA6_RW_RO_STATUS_1_ADDR
#define CONN_PTA6_RW_RO_STATUS_1_ro_rw1_cnt_MASK               0x007FFFFF
#define CONN_PTA6_RW_RO_STATUS_1_ro_rw1_cnt_SHFT               0

#define CONN_PTA6_RW_TDM_THLD_0_r_rw_tdm_wifi1_thld_ADDR       CONN_PTA6_RW_TDM_THLD_0_ADDR
#define CONN_PTA6_RW_TDM_THLD_0_r_rw_tdm_wifi1_thld_MASK       0x7FFF0000
#define CONN_PTA6_RW_TDM_THLD_0_r_rw_tdm_wifi1_thld_SHFT       16
#define CONN_PTA6_RW_TDM_THLD_0_r_rw_tdm_wifi0_thld_ADDR       CONN_PTA6_RW_TDM_THLD_0_ADDR
#define CONN_PTA6_RW_TDM_THLD_0_r_rw_tdm_wifi0_thld_MASK       0x00007FFF
#define CONN_PTA6_RW_TDM_THLD_0_r_rw_tdm_wifi0_thld_SHFT       0

#define CONN_PTA6_RW_TDM_THLD_1_r_rw_tdm_bt_thld_ADDR          CONN_PTA6_RW_TDM_THLD_1_ADDR
#define CONN_PTA6_RW_TDM_THLD_1_r_rw_tdm_bt_thld_MASK          0x00007FFF
#define CONN_PTA6_RW_TDM_THLD_1_r_rw_tdm_bt_thld_SHFT          0

#define CONN_PTA6_RW_TDM_THLD_2_r_rw_tdm_ext1_thld_ADDR        CONN_PTA6_RW_TDM_THLD_2_ADDR
#define CONN_PTA6_RW_TDM_THLD_2_r_rw_tdm_ext1_thld_MASK        0x7FFF0000
#define CONN_PTA6_RW_TDM_THLD_2_r_rw_tdm_ext1_thld_SHFT        16
#define CONN_PTA6_RW_TDM_THLD_2_r_rw_tdm_ext0_thld_ADDR        CONN_PTA6_RW_TDM_THLD_2_ADDR
#define CONN_PTA6_RW_TDM_THLD_2_r_rw_tdm_ext0_thld_MASK        0x00007FFF
#define CONN_PTA6_RW_TDM_THLD_2_r_rw_tdm_ext0_thld_SHFT        0

#define CONN_PTA6_MISC_BT_CTRL_0_r_lterx_2bttx_en_ADDR         CONN_PTA6_MISC_BT_CTRL_0_ADDR
#define CONN_PTA6_MISC_BT_CTRL_0_r_lterx_2bttx_en_MASK         0x0000FF00
#define CONN_PTA6_MISC_BT_CTRL_0_r_lterx_2bttx_en_SHFT         8
#define CONN_PTA6_MISC_BT_CTRL_0_r_ltetx_2btrx_en_ADDR         CONN_PTA6_MISC_BT_CTRL_0_ADDR
#define CONN_PTA6_MISC_BT_CTRL_0_r_ltetx_2btrx_en_MASK         0x0000003F
#define CONN_PTA6_MISC_BT_CTRL_0_r_ltetx_2btrx_en_SHFT         0

#define CONN_PTA6_MISC_BT_CTRL_1_r_fsync_2bt_sel_ADDR          CONN_PTA6_MISC_BT_CTRL_1_ADDR
#define CONN_PTA6_MISC_BT_CTRL_1_r_fsync_2bt_sel_MASK          0x00070000
#define CONN_PTA6_MISC_BT_CTRL_1_r_fsync_2bt_sel_SHFT          16
#define CONN_PTA6_MISC_BT_CTRL_1_r_fsync_2bt_offset_ADDR       CONN_PTA6_MISC_BT_CTRL_1_ADDR
#define CONN_PTA6_MISC_BT_CTRL_1_r_fsync_2bt_offset_MASK       0x00003FFF
#define CONN_PTA6_MISC_BT_CTRL_1_r_fsync_2bt_offset_SHFT       0

#define CONN_PTA6_MISC_WF_CTRL_r_wf1_sel_bt_modem_trx_ADDR     CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_wf1_sel_bt_modem_trx_MASK     0x00000200
#define CONN_PTA6_MISC_WF_CTRL_r_wf1_sel_bt_modem_trx_SHFT     9
#define CONN_PTA6_MISC_WF_CTRL_r_wf0_sel_bt_modem_trx_ADDR     CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_wf0_sel_bt_modem_trx_MASK     0x00000100
#define CONN_PTA6_MISC_WF_CTRL_r_wf0_sel_bt_modem_trx_SHFT     8
#define CONN_PTA6_MISC_WF_CTRL_r_sw_btrx_dis_wfrx_ADDR         CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_sw_btrx_dis_wfrx_MASK         0x00000080
#define CONN_PTA6_MISC_WF_CTRL_r_sw_btrx_dis_wfrx_SHFT         7
#define CONN_PTA6_MISC_WF_CTRL_r_sw_bttx_dis_wfrx_ADDR         CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_sw_bttx_dis_wfrx_MASK         0x00000040
#define CONN_PTA6_MISC_WF_CTRL_r_sw_bttx_dis_wfrx_SHFT         6
#define CONN_PTA6_MISC_WF_CTRL_r_wf1_ch_band_ADDR              CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_wf1_ch_band_MASK              0x00000020
#define CONN_PTA6_MISC_WF_CTRL_r_wf1_ch_band_SHFT              5
#define CONN_PTA6_MISC_WF_CTRL_r_btrx_dis_wfrx1_en_ADDR        CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_btrx_dis_wfrx1_en_MASK        0x00000010
#define CONN_PTA6_MISC_WF_CTRL_r_btrx_dis_wfrx1_en_SHFT        4
#define CONN_PTA6_MISC_WF_CTRL_r_bttx_dis_wfrx1_en_ADDR        CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_bttx_dis_wfrx1_en_MASK        0x00000008
#define CONN_PTA6_MISC_WF_CTRL_r_bttx_dis_wfrx1_en_SHFT        3
#define CONN_PTA6_MISC_WF_CTRL_r_wf0_ch_band_ADDR              CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_wf0_ch_band_MASK              0x00000004
#define CONN_PTA6_MISC_WF_CTRL_r_wf0_ch_band_SHFT              2
#define CONN_PTA6_MISC_WF_CTRL_r_btrx_dis_wfrx0_en_ADDR        CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_btrx_dis_wfrx0_en_MASK        0x00000002
#define CONN_PTA6_MISC_WF_CTRL_r_btrx_dis_wfrx0_en_SHFT        1
#define CONN_PTA6_MISC_WF_CTRL_r_bttx_dis_wfrx0_en_ADDR        CONN_PTA6_MISC_WF_CTRL_ADDR
#define CONN_PTA6_MISC_WF_CTRL_r_bttx_dis_wfrx0_en_MASK        0x00000001
#define CONN_PTA6_MISC_WF_CTRL_r_bttx_dis_wfrx0_en_SHFT        0

#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx1_sel_ADDR            CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx1_sel_MASK            0x00040000
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx1_sel_SHFT            18
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx0_sel_ADDR            CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx0_sel_MASK            0x00020000
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx0_sel_SHFT            17
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx_sel_2bt_ADDR         CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx_sel_2bt_MASK         0x00004000
#define CONN_PTA6_MISC_SX_SEL_r_sw_pta_sx_sel_2bt_SHFT         14
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx1_sel_en_ADDR        CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx1_sel_en_MASK        0x00002000
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx1_sel_en_SHFT        13
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx0_sel_en_ADDR        CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx0_sel_en_MASK        0x00001000
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx0_sel_en_SHFT        12
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx_sel_2bt_en_ADDR     CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx_sel_2bt_en_MASK     0x00000400
#define CONN_PTA6_MISC_SX_SEL_r_man_pta_sx_sel_2bt_en_SHFT     10
#define CONN_PTA6_MISC_SX_SEL_r_sx1_w_bt_ADDR                  CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_sx1_w_bt_MASK                  0x00000200
#define CONN_PTA6_MISC_SX_SEL_r_sx1_w_bt_SHFT                  9
#define CONN_PTA6_MISC_SX_SEL_r_sx0_w_bt_ADDR                  CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_sx0_w_bt_MASK                  0x00000100
#define CONN_PTA6_MISC_SX_SEL_r_sx0_w_bt_SHFT                  8
#define CONN_PTA6_MISC_SX_SEL_r_pta_sx0_sel_gt2_ADDR           CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_pta_sx0_sel_gt2_MASK           0x000000F0
#define CONN_PTA6_MISC_SX_SEL_r_pta_sx0_sel_gt2_SHFT           4
#define CONN_PTA6_MISC_SX_SEL_r_pta_sx0_sel_gt1_ADDR           CONN_PTA6_MISC_SX_SEL_ADDR
#define CONN_PTA6_MISC_SX_SEL_r_pta_sx0_sel_gt1_MASK           0x0000000F
#define CONN_PTA6_MISC_SX_SEL_r_pta_sx0_sel_gt1_SHFT           0

#define CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l5_blank_src_ADDR    CONN_PTA6_GPS_BLANK_CFG_ADDR
#define CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l5_blank_src_MASK    0x00000018
#define CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l5_blank_src_SHFT    3
#define CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l1_blank_src_ADDR    CONN_PTA6_GPS_BLANK_CFG_ADDR
#define CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l1_blank_src_MASK    0x00000006
#define CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l1_blank_src_SHFT    1
#define CONN_PTA6_GPS_BLANK_CFG_r_gps_blank_src_ADDR           CONN_PTA6_GPS_BLANK_CFG_ADDR
#define CONN_PTA6_GPS_BLANK_CFG_r_gps_blank_src_MASK           0x00000001
#define CONN_PTA6_GPS_BLANK_CFG_r_gps_blank_src_SHFT           0

#define CONN_PTA6_TMR_CTRL_0_r_pta_rx_ind_s_tmr_thld_ADDR      CONN_PTA6_TMR_CTRL_0_ADDR
#define CONN_PTA6_TMR_CTRL_0_r_pta_rx_ind_s_tmr_thld_MASK      0x1F000000
#define CONN_PTA6_TMR_CTRL_0_r_pta_rx_ind_s_tmr_thld_SHFT      24
#define CONN_PTA6_TMR_CTRL_0_r_pta_rx_ind_l_tmr_thld_ADDR      CONN_PTA6_TMR_CTRL_0_ADDR
#define CONN_PTA6_TMR_CTRL_0_r_pta_rx_ind_l_tmr_thld_MASK      0x003FFF00
#define CONN_PTA6_TMR_CTRL_0_r_pta_rx_ind_l_tmr_thld_SHFT      8
#define CONN_PTA6_TMR_CTRL_0_r_pta_sync_tmr_thld_ADDR          CONN_PTA6_TMR_CTRL_0_ADDR
#define CONN_PTA6_TMR_CTRL_0_r_pta_sync_tmr_thld_MASK          0x0000003F
#define CONN_PTA6_TMR_CTRL_0_r_pta_sync_tmr_thld_SHFT          0

#define CONN_PTA6_TMR_CTRL_1_r_idc_2nd_byte_tmout_ADDR         CONN_PTA6_TMR_CTRL_1_ADDR
#define CONN_PTA6_TMR_CTRL_1_r_idc_2nd_byte_tmout_MASK         0xFF000000
#define CONN_PTA6_TMR_CTRL_1_r_idc_2nd_byte_tmout_SHFT         24
#define CONN_PTA6_TMR_CTRL_1_r_pta_rx_ind_lte_tmr_thld_ADDR    CONN_PTA6_TMR_CTRL_1_ADDR
#define CONN_PTA6_TMR_CTRL_1_r_pta_rx_ind_lte_tmr_thld_MASK    0x003FFF00
#define CONN_PTA6_TMR_CTRL_1_r_pta_rx_ind_lte_tmr_thld_SHFT    8
#define CONN_PTA6_TMR_CTRL_1_r_pta_tx_ind_tmr_thld_ADDR        CONN_PTA6_TMR_CTRL_1_ADDR
#define CONN_PTA6_TMR_CTRL_1_r_pta_tx_ind_tmr_thld_MASK        0x0000003F
#define CONN_PTA6_TMR_CTRL_1_r_pta_tx_ind_tmr_thld_SHFT        0

#define CONN_PTA6_TMR_CTRL_2_r_resend_tout_tmr_thld_ADDR       CONN_PTA6_TMR_CTRL_2_ADDR
#define CONN_PTA6_TMR_CTRL_2_r_resend_tout_tmr_thld_MASK       0xFF000000
#define CONN_PTA6_TMR_CTRL_2_r_resend_tout_tmr_thld_SHFT       24
#define CONN_PTA6_TMR_CTRL_2_r_pta_rx_ind_nr_tmr_thld_ADDR     CONN_PTA6_TMR_CTRL_2_ADDR
#define CONN_PTA6_TMR_CTRL_2_r_pta_rx_ind_nr_tmr_thld_MASK     0x003FFF00
#define CONN_PTA6_TMR_CTRL_2_r_pta_rx_ind_nr_tmr_thld_SHFT     8

#define CONN_PTA6_TMR_CTRL_3_r_gps_l5_blank_tmr_thld_ADDR      CONN_PTA6_TMR_CTRL_3_ADDR
#define CONN_PTA6_TMR_CTRL_3_r_gps_l5_blank_tmr_thld_MASK      0x0000FF00
#define CONN_PTA6_TMR_CTRL_3_r_gps_l5_blank_tmr_thld_SHFT      8
#define CONN_PTA6_TMR_CTRL_3_r_gps_l1_blank_tmr_thld_ADDR      CONN_PTA6_TMR_CTRL_3_ADDR
#define CONN_PTA6_TMR_CTRL_3_r_gps_l1_blank_tmr_thld_MASK      0x000000FF
#define CONN_PTA6_TMR_CTRL_3_r_gps_l1_blank_tmr_thld_SHFT      0

#define CONN_PTA6_INT_MASK_0_r_sync_tmr_exp_int_msk_ADDR       CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_sync_tmr_exp_int_msk_MASK       0x001F8000
#define CONN_PTA6_INT_MASK_0_r_sync_tmr_exp_int_msk_SHFT       15
#define CONN_PTA6_INT_MASK_0_r_md_pat_sync_offset_err_msk_ADDR CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_md_pat_sync_offset_err_msk_MASK 0x00007E00
#define CONN_PTA6_INT_MASK_0_r_md_pat_sync_offset_err_msk_SHFT 9
#define CONN_PTA6_INT_MASK_0_r_nr_set_rw1_int_msk_ADDR         CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_nr_set_rw1_int_msk_MASK         0x00000100
#define CONN_PTA6_INT_MASK_0_r_nr_set_rw1_int_msk_SHFT         8
#define CONN_PTA6_INT_MASK_0_r_nr_set_rw0_int_msk_ADDR         CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_nr_set_rw0_int_msk_MASK         0x00000080
#define CONN_PTA6_INT_MASK_0_r_nr_set_rw0_int_msk_SHFT         7
#define CONN_PTA6_INT_MASK_0_r_lte_set_rw1_int_msk_ADDR        CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_lte_set_rw1_int_msk_MASK        0x00000040
#define CONN_PTA6_INT_MASK_0_r_lte_set_rw1_int_msk_SHFT        6
#define CONN_PTA6_INT_MASK_0_r_lte_set_rw0_int_msk_ADDR        CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_lte_set_rw0_int_msk_MASK        0x00000020
#define CONN_PTA6_INT_MASK_0_r_lte_set_rw0_int_msk_SHFT        5
#define CONN_PTA6_INT_MASK_0_r_uart_rx_err_int_msk_ADDR        CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_uart_rx_err_int_msk_MASK        0x00000010
#define CONN_PTA6_INT_MASK_0_r_uart_rx_err_int_msk_SHFT        4
#define CONN_PTA6_INT_MASK_0_r_dis_ap_all_int_msk_ADDR         CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_dis_ap_all_int_msk_MASK         0x00000008
#define CONN_PTA6_INT_MASK_0_r_dis_ap_all_int_msk_SHFT         3
#define CONN_PTA6_INT_MASK_0_r_resend_tmr_exp_int_msk_ADDR     CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_resend_tmr_exp_int_msk_MASK     0x00000004
#define CONN_PTA6_INT_MASK_0_r_resend_tmr_exp_int_msk_SHFT     2
#define CONN_PTA6_INT_MASK_0_r_idc_cmd_tx_int_msk_ADDR         CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_idc_cmd_tx_int_msk_MASK         0x00000002
#define CONN_PTA6_INT_MASK_0_r_idc_cmd_tx_int_msk_SHFT         1
#define CONN_PTA6_INT_MASK_0_r_idc_cmd_rx_int_msk_ADDR         CONN_PTA6_INT_MASK_0_ADDR
#define CONN_PTA6_INT_MASK_0_r_idc_cmd_rx_int_msk_MASK         0x00000001
#define CONN_PTA6_INT_MASK_0_r_idc_cmd_rx_int_msk_SHFT         0

#define CONN_PTA6_INT_MASK_1_r_tdm_rw_tout_msk_ADDR            CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_tdm_rw_tout_msk_MASK            0x001F0000
#define CONN_PTA6_INT_MASK_1_r_tdm_rw_tout_msk_SHFT            16
#define CONN_PTA6_INT_MASK_1_r_gpsl5_blank_tmr_exp_int_msk_ADDR CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_gpsl5_blank_tmr_exp_int_msk_MASK 0x00008000
#define CONN_PTA6_INT_MASK_1_r_gpsl5_blank_tmr_exp_int_msk_SHFT 15
#define CONN_PTA6_INT_MASK_1_r_gpsl1_blank_tmr_exp_int_msk_ADDR CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_gpsl1_blank_tmr_exp_int_msk_MASK 0x00004000
#define CONN_PTA6_INT_MASK_1_r_gpsl1_blank_tmr_exp_int_msk_SHFT 14
#define CONN_PTA6_INT_MASK_1_r_nr_blk_consys_tx_tmr_exp_int_msk_ADDR CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_nr_blk_consys_tx_tmr_exp_int_msk_MASK 0x00002000
#define CONN_PTA6_INT_MASK_1_r_nr_blk_consys_tx_tmr_exp_int_msk_SHFT 13
#define CONN_PTA6_INT_MASK_1_r_lte_blk_consys_tx_tmr_exp_int_msk_ADDR CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_lte_blk_consys_tx_tmr_exp_int_msk_MASK 0x00001000
#define CONN_PTA6_INT_MASK_1_r_lte_blk_consys_tx_tmr_exp_int_msk_SHFT 12
#define CONN_PTA6_INT_MASK_1_r_rx_ind_tmr_exp_int_msk_ADDR     CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_rx_ind_tmr_exp_int_msk_MASK     0x00000FC0
#define CONN_PTA6_INT_MASK_1_r_rx_ind_tmr_exp_int_msk_SHFT     6
#define CONN_PTA6_INT_MASK_1_r_tx_ind_tmr_exp_int_msk_ADDR     CONN_PTA6_INT_MASK_1_ADDR
#define CONN_PTA6_INT_MASK_1_r_tx_ind_tmr_exp_int_msk_MASK     0x0000003F
#define CONN_PTA6_INT_MASK_1_r_tx_ind_tmr_exp_int_msk_SHFT     0

#define CONN_PTA6_WF_INT_MASK_2_r_wf_wf_ppdu_frmex_rx_over_dw_msk_ADDR CONN_PTA6_WF_INT_MASK_2_ADDR
#define CONN_PTA6_WF_INT_MASK_2_r_wf_wf_ppdu_frmex_rx_over_dw_msk_MASK 0x000000C0
#define CONN_PTA6_WF_INT_MASK_2_r_wf_wf_ppdu_frmex_rx_over_dw_msk_SHFT 6
#define CONN_PTA6_WF_INT_MASK_2_r_wf_wf_ppdu_frmex_tx_over_dw_msk_ADDR CONN_PTA6_WF_INT_MASK_2_ADDR
#define CONN_PTA6_WF_INT_MASK_2_r_wf_wf_ppdu_frmex_tx_over_dw_msk_MASK 0x00000030
#define CONN_PTA6_WF_INT_MASK_2_r_wf_wf_ppdu_frmex_tx_over_dw_msk_SHFT 4
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_seq_num_skip_int_msk_ADDR CONN_PTA6_WF_INT_MASK_2_ADDR
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_seq_num_skip_int_msk_MASK 0x00000008
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_seq_num_skip_int_msk_SHFT 3
#define CONN_PTA6_WF_INT_MASK_2_r_wf_undef_cmd_rcvd_int_msk_ADDR CONN_PTA6_WF_INT_MASK_2_ADDR
#define CONN_PTA6_WF_INT_MASK_2_r_wf_undef_cmd_rcvd_int_msk_MASK 0x00000004
#define CONN_PTA6_WF_INT_MASK_2_r_wf_undef_cmd_rcvd_int_msk_SHFT 2
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_cmd_nr_slp_dur_rcvd_int_msk_ADDR CONN_PTA6_WF_INT_MASK_2_ADDR
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_cmd_nr_slp_dur_rcvd_int_msk_MASK 0x00000002
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_cmd_nr_slp_dur_rcvd_int_msk_SHFT 1
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_cmd_lte_slp_dur_rcvd_int_msk_ADDR CONN_PTA6_WF_INT_MASK_2_ADDR
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_cmd_lte_slp_dur_rcvd_int_msk_MASK 0x00000001
#define CONN_PTA6_WF_INT_MASK_2_r_wf_idc_cmd_lte_slp_dur_rcvd_int_msk_SHFT 0

#define CONN_PTA6_BT_INT_MASK_2_r_bt_wf_ppdu_frmex_rx_over_dw_msk_ADDR CONN_PTA6_BT_INT_MASK_2_ADDR
#define CONN_PTA6_BT_INT_MASK_2_r_bt_wf_ppdu_frmex_rx_over_dw_msk_MASK 0x000000C0
#define CONN_PTA6_BT_INT_MASK_2_r_bt_wf_ppdu_frmex_rx_over_dw_msk_SHFT 6
#define CONN_PTA6_BT_INT_MASK_2_r_bt_wf_ppdu_frmex_tx_over_dw_msk_ADDR CONN_PTA6_BT_INT_MASK_2_ADDR
#define CONN_PTA6_BT_INT_MASK_2_r_bt_wf_ppdu_frmex_tx_over_dw_msk_MASK 0x00000030
#define CONN_PTA6_BT_INT_MASK_2_r_bt_wf_ppdu_frmex_tx_over_dw_msk_SHFT 4
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_seq_num_skip_int_msk_ADDR CONN_PTA6_BT_INT_MASK_2_ADDR
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_seq_num_skip_int_msk_MASK 0x00000008
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_seq_num_skip_int_msk_SHFT 3
#define CONN_PTA6_BT_INT_MASK_2_r_bt_undef_cmd_rcvd_int_msk_ADDR CONN_PTA6_BT_INT_MASK_2_ADDR
#define CONN_PTA6_BT_INT_MASK_2_r_bt_undef_cmd_rcvd_int_msk_MASK 0x00000004
#define CONN_PTA6_BT_INT_MASK_2_r_bt_undef_cmd_rcvd_int_msk_SHFT 2
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_cmd_nr_slp_dur_rcvd_int_msk_ADDR CONN_PTA6_BT_INT_MASK_2_ADDR
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_cmd_nr_slp_dur_rcvd_int_msk_MASK 0x00000002
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_cmd_nr_slp_dur_rcvd_int_msk_SHFT 1
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_cmd_lte_slp_dur_rcvd_int_msk_ADDR CONN_PTA6_BT_INT_MASK_2_ADDR
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_cmd_lte_slp_dur_rcvd_int_msk_MASK 0x00000001
#define CONN_PTA6_BT_INT_MASK_2_r_bt_idc_cmd_lte_slp_dur_rcvd_int_msk_SHFT 0

#define CONN_PTA6_INT_STATUS_0_ro_sync_tmr_exp_int_stat_ADDR   CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_sync_tmr_exp_int_stat_MASK   0x001F8000
#define CONN_PTA6_INT_STATUS_0_ro_sync_tmr_exp_int_stat_SHFT   15
#define CONN_PTA6_INT_STATUS_0_ro_md_pat_sync_offset_err_stat_ADDR CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_md_pat_sync_offset_err_stat_MASK 0x00007E00
#define CONN_PTA6_INT_STATUS_0_ro_md_pat_sync_offset_err_stat_SHFT 9
#define CONN_PTA6_INT_STATUS_0_ro_nr_set_rw1_int_stat_ADDR     CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_nr_set_rw1_int_stat_MASK     0x00000100
#define CONN_PTA6_INT_STATUS_0_ro_nr_set_rw1_int_stat_SHFT     8
#define CONN_PTA6_INT_STATUS_0_ro_nr_set_rw0_int_stat_ADDR     CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_nr_set_rw0_int_stat_MASK     0x00000080
#define CONN_PTA6_INT_STATUS_0_ro_nr_set_rw0_int_stat_SHFT     7
#define CONN_PTA6_INT_STATUS_0_ro_lte_set_rw1_int_stat_ADDR    CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_lte_set_rw1_int_stat_MASK    0x00000040
#define CONN_PTA6_INT_STATUS_0_ro_lte_set_rw1_int_stat_SHFT    6
#define CONN_PTA6_INT_STATUS_0_ro_lte_set_rw0_int_stat_ADDR    CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_lte_set_rw0_int_stat_MASK    0x00000020
#define CONN_PTA6_INT_STATUS_0_ro_lte_set_rw0_int_stat_SHFT    5
#define CONN_PTA6_INT_STATUS_0_ro_uart_rx_err_int_stat_ADDR    CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_uart_rx_err_int_stat_MASK    0x00000010
#define CONN_PTA6_INT_STATUS_0_ro_uart_rx_err_int_stat_SHFT    4
#define CONN_PTA6_INT_STATUS_0_ro_dis_ap_all_int_stat_ADDR     CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_dis_ap_all_int_stat_MASK     0x00000008
#define CONN_PTA6_INT_STATUS_0_ro_dis_ap_all_int_stat_SHFT     3
#define CONN_PTA6_INT_STATUS_0_ro_resend_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_resend_tmr_exp_int_stat_MASK 0x00000004
#define CONN_PTA6_INT_STATUS_0_ro_resend_tmr_exp_int_stat_SHFT 2
#define CONN_PTA6_INT_STATUS_0_ro_idc_cmd_tx_int_stat_ADDR     CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_idc_cmd_tx_int_stat_MASK     0x00000002
#define CONN_PTA6_INT_STATUS_0_ro_idc_cmd_tx_int_stat_SHFT     1
#define CONN_PTA6_INT_STATUS_0_ro_idc_cmd_rx_int_stat_ADDR     CONN_PTA6_INT_STATUS_0_ADDR
#define CONN_PTA6_INT_STATUS_0_ro_idc_cmd_rx_int_stat_MASK     0x00000001
#define CONN_PTA6_INT_STATUS_0_ro_idc_cmd_rx_int_stat_SHFT     0

#define CONN_PTA6_INT_STATUS_1_ro_extif1_err_int_stat_ADDR     CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_extif1_err_int_stat_MASK     0x00800000
#define CONN_PTA6_INT_STATUS_1_ro_extif1_err_int_stat_SHFT     23
#define CONN_PTA6_INT_STATUS_1_ro_extif0_sco_start_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_extif0_sco_start_int_stat_MASK 0x00400000
#define CONN_PTA6_INT_STATUS_1_ro_extif0_sco_start_int_stat_SHFT 22
#define CONN_PTA6_INT_STATUS_1_ro_extif0_err_int_stat_ADDR     CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_extif0_err_int_stat_MASK     0x00200000
#define CONN_PTA6_INT_STATUS_1_ro_extif0_err_int_stat_SHFT     21
#define CONN_PTA6_INT_STATUS_1_ro_tdm_rw_tout_stat_ADDR        CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_tdm_rw_tout_stat_MASK        0x001F0000
#define CONN_PTA6_INT_STATUS_1_ro_tdm_rw_tout_stat_SHFT        16
#define CONN_PTA6_INT_STATUS_1_ro_gpsl5_blank_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_gpsl5_blank_tmr_exp_int_stat_MASK 0x00008000
#define CONN_PTA6_INT_STATUS_1_ro_gpsl5_blank_tmr_exp_int_stat_SHFT 15
#define CONN_PTA6_INT_STATUS_1_ro_gpsl1_blank_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_gpsl1_blank_tmr_exp_int_stat_MASK 0x00004000
#define CONN_PTA6_INT_STATUS_1_ro_gpsl1_blank_tmr_exp_int_stat_SHFT 14
#define CONN_PTA6_INT_STATUS_1_ro_nr_blk_consys_tx_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_nr_blk_consys_tx_tmr_exp_int_stat_MASK 0x00002000
#define CONN_PTA6_INT_STATUS_1_ro_nr_blk_consys_tx_tmr_exp_int_stat_SHFT 13
#define CONN_PTA6_INT_STATUS_1_ro_lte_blk_consys_tx_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_lte_blk_consys_tx_tmr_exp_int_stat_MASK 0x00001000
#define CONN_PTA6_INT_STATUS_1_ro_lte_blk_consys_tx_tmr_exp_int_stat_SHFT 12
#define CONN_PTA6_INT_STATUS_1_ro_rx_ind_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_rx_ind_tmr_exp_int_stat_MASK 0x00000FC0
#define CONN_PTA6_INT_STATUS_1_ro_rx_ind_tmr_exp_int_stat_SHFT 6
#define CONN_PTA6_INT_STATUS_1_ro_tx_ind_tmr_exp_int_stat_ADDR CONN_PTA6_INT_STATUS_1_ADDR
#define CONN_PTA6_INT_STATUS_1_ro_tx_ind_tmr_exp_int_stat_MASK 0x0000003F
#define CONN_PTA6_INT_STATUS_1_ro_tx_ind_tmr_exp_int_stat_SHFT 0

#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_wf_ppdu_frmex_rx_over_dw_stat_ADDR CONN_PTA6_WF_INT_STATUS_2_ADDR
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_wf_ppdu_frmex_rx_over_dw_stat_MASK 0x000000C0
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_wf_ppdu_frmex_rx_over_dw_stat_SHFT 6
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_wf_ppdu_frmex_tx_over_dw_stat_ADDR CONN_PTA6_WF_INT_STATUS_2_ADDR
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_wf_ppdu_frmex_tx_over_dw_stat_MASK 0x00000030
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_wf_ppdu_frmex_tx_over_dw_stat_SHFT 4
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_seq_num_skip_int_stat_ADDR CONN_PTA6_WF_INT_STATUS_2_ADDR
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_seq_num_skip_int_stat_MASK 0x00000008
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_seq_num_skip_int_stat_SHFT 3
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_undef_cmd_rcvd_int_stat_ADDR CONN_PTA6_WF_INT_STATUS_2_ADDR
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_undef_cmd_rcvd_int_stat_MASK 0x00000004
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_undef_cmd_rcvd_int_stat_SHFT 2
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_cmd_nr_slp_dur_rcvd_int_stat_ADDR CONN_PTA6_WF_INT_STATUS_2_ADDR
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_cmd_nr_slp_dur_rcvd_int_stat_MASK 0x00000002
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_cmd_nr_slp_dur_rcvd_int_stat_SHFT 1
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_cmd_lte_slp_dur_rcvd_int_stat_ADDR CONN_PTA6_WF_INT_STATUS_2_ADDR
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_cmd_lte_slp_dur_rcvd_int_stat_MASK 0x00000001
#define CONN_PTA6_WF_INT_STATUS_2_ro_wf_idc_cmd_lte_slp_dur_rcvd_int_stat_SHFT 0

#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_wf_ppdu_frmex_rx_over_dw_stat_ADDR CONN_PTA6_BT_INT_STATUS_2_ADDR
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_wf_ppdu_frmex_rx_over_dw_stat_MASK 0x000000C0
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_wf_ppdu_frmex_rx_over_dw_stat_SHFT 6
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_wf_ppdu_frmex_tx_over_dw_stat_ADDR CONN_PTA6_BT_INT_STATUS_2_ADDR
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_wf_ppdu_frmex_tx_over_dw_stat_MASK 0x00000030
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_wf_ppdu_frmex_tx_over_dw_stat_SHFT 4
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_seq_num_skip_int_stat_ADDR CONN_PTA6_BT_INT_STATUS_2_ADDR
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_seq_num_skip_int_stat_MASK 0x00000008
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_seq_num_skip_int_stat_SHFT 3
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_undef_cmd_rcvd_int_stat_ADDR CONN_PTA6_BT_INT_STATUS_2_ADDR
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_undef_cmd_rcvd_int_stat_MASK 0x00000004
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_undef_cmd_rcvd_int_stat_SHFT 2
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_cmd_nr_slp_dur_rcvd_int_stat_ADDR CONN_PTA6_BT_INT_STATUS_2_ADDR
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_cmd_nr_slp_dur_rcvd_int_stat_MASK 0x00000002
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_cmd_nr_slp_dur_rcvd_int_stat_SHFT 1
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_cmd_lte_slp_dur_rcvd_int_stat_ADDR CONN_PTA6_BT_INT_STATUS_2_ADDR
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_cmd_lte_slp_dur_rcvd_int_stat_MASK 0x00000001
#define CONN_PTA6_BT_INT_STATUS_2_ro_bt_idc_cmd_lte_slp_dur_rcvd_int_stat_SHFT 0

#define CONN_PTA6_INT_CLR_0_r_sync_tmr_exp_int_clr_ADDR        CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_sync_tmr_exp_int_clr_MASK        0x001F8000
#define CONN_PTA6_INT_CLR_0_r_sync_tmr_exp_int_clr_SHFT        15
#define CONN_PTA6_INT_CLR_0_r_md_pat_sync_offset_err_clr_ADDR  CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_md_pat_sync_offset_err_clr_MASK  0x00007E00
#define CONN_PTA6_INT_CLR_0_r_md_pat_sync_offset_err_clr_SHFT  9
#define CONN_PTA6_INT_CLR_0_r_nr_set_rw1_int_clr_ADDR          CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_nr_set_rw1_int_clr_MASK          0x00000100
#define CONN_PTA6_INT_CLR_0_r_nr_set_rw1_int_clr_SHFT          8
#define CONN_PTA6_INT_CLR_0_r_nr_set_rw0_int_clr_ADDR          CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_nr_set_rw0_int_clr_MASK          0x00000080
#define CONN_PTA6_INT_CLR_0_r_nr_set_rw0_int_clr_SHFT          7
#define CONN_PTA6_INT_CLR_0_r_lte_set_rw1_int_clr_ADDR         CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_lte_set_rw1_int_clr_MASK         0x00000040
#define CONN_PTA6_INT_CLR_0_r_lte_set_rw1_int_clr_SHFT         6
#define CONN_PTA6_INT_CLR_0_r_lte_set_rw0_int_clr_ADDR         CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_lte_set_rw0_int_clr_MASK         0x00000020
#define CONN_PTA6_INT_CLR_0_r_lte_set_rw0_int_clr_SHFT         5
#define CONN_PTA6_INT_CLR_0_r_uart_rx_err_int_clr_ADDR         CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_uart_rx_err_int_clr_MASK         0x00000010
#define CONN_PTA6_INT_CLR_0_r_uart_rx_err_int_clr_SHFT         4
#define CONN_PTA6_INT_CLR_0_r_dis_ap_all_int_clr_ADDR          CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_dis_ap_all_int_clr_MASK          0x00000008
#define CONN_PTA6_INT_CLR_0_r_dis_ap_all_int_clr_SHFT          3
#define CONN_PTA6_INT_CLR_0_r_resend_tmr_exp_int_clr_ADDR      CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_resend_tmr_exp_int_clr_MASK      0x00000004
#define CONN_PTA6_INT_CLR_0_r_resend_tmr_exp_int_clr_SHFT      2
#define CONN_PTA6_INT_CLR_0_r_idc_cmd_tx_int_clr_ADDR          CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_idc_cmd_tx_int_clr_MASK          0x00000002
#define CONN_PTA6_INT_CLR_0_r_idc_cmd_tx_int_clr_SHFT          1
#define CONN_PTA6_INT_CLR_0_r_idc_cmd_rx_int_clr_ADDR          CONN_PTA6_INT_CLR_0_ADDR
#define CONN_PTA6_INT_CLR_0_r_idc_cmd_rx_int_clr_MASK          0x00000001
#define CONN_PTA6_INT_CLR_0_r_idc_cmd_rx_int_clr_SHFT          0

#define CONN_PTA6_INT_CLR_1_r_extif1_err_int_clr_ADDR          CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_extif1_err_int_clr_MASK          0x00800000
#define CONN_PTA6_INT_CLR_1_r_extif1_err_int_clr_SHFT          23
#define CONN_PTA6_INT_CLR_1_r_extif0_sco_start_int_clr_ADDR    CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_extif0_sco_start_int_clr_MASK    0x00400000
#define CONN_PTA6_INT_CLR_1_r_extif0_sco_start_int_clr_SHFT    22
#define CONN_PTA6_INT_CLR_1_r_extif0_err_int_clr_ADDR          CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_extif0_err_int_clr_MASK          0x00200000
#define CONN_PTA6_INT_CLR_1_r_extif0_err_int_clr_SHFT          21
#define CONN_PTA6_INT_CLR_1_r_tdm_rw_tout_clr_ADDR             CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_tdm_rw_tout_clr_MASK             0x001F0000
#define CONN_PTA6_INT_CLR_1_r_tdm_rw_tout_clr_SHFT             16
#define CONN_PTA6_INT_CLR_1_r_gpsl5_blank_tmr_exp_int_clr_ADDR CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_gpsl5_blank_tmr_exp_int_clr_MASK 0x00008000
#define CONN_PTA6_INT_CLR_1_r_gpsl5_blank_tmr_exp_int_clr_SHFT 15
#define CONN_PTA6_INT_CLR_1_r_gpsl1_blank_tmr_exp_int_clr_ADDR CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_gpsl1_blank_tmr_exp_int_clr_MASK 0x00004000
#define CONN_PTA6_INT_CLR_1_r_gpsl1_blank_tmr_exp_int_clr_SHFT 14
#define CONN_PTA6_INT_CLR_1_r_nr_blk_consys_tx_tmr_exp_int_clr_ADDR CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_nr_blk_consys_tx_tmr_exp_int_clr_MASK 0x00002000
#define CONN_PTA6_INT_CLR_1_r_nr_blk_consys_tx_tmr_exp_int_clr_SHFT 13
#define CONN_PTA6_INT_CLR_1_r_lte_blk_consys_tx_tmr_exp_int_clr_ADDR CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_lte_blk_consys_tx_tmr_exp_int_clr_MASK 0x00001000
#define CONN_PTA6_INT_CLR_1_r_lte_blk_consys_tx_tmr_exp_int_clr_SHFT 12
#define CONN_PTA6_INT_CLR_1_r_rx_ind_tmr_exp_int_clr_ADDR      CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_rx_ind_tmr_exp_int_clr_MASK      0x00000FC0
#define CONN_PTA6_INT_CLR_1_r_rx_ind_tmr_exp_int_clr_SHFT      6
#define CONN_PTA6_INT_CLR_1_r_tx_ind_tmr_exp_int_clr_ADDR      CONN_PTA6_INT_CLR_1_ADDR
#define CONN_PTA6_INT_CLR_1_r_tx_ind_tmr_exp_int_clr_MASK      0x0000003F
#define CONN_PTA6_INT_CLR_1_r_tx_ind_tmr_exp_int_clr_SHFT      0

#define CONN_PTA6_WF_INT_CLR_2_r_wf_wf_ppdu_frmex_rx_over_dw_clr_ADDR CONN_PTA6_WF_INT_CLR_2_ADDR
#define CONN_PTA6_WF_INT_CLR_2_r_wf_wf_ppdu_frmex_rx_over_dw_clr_MASK 0x000000C0
#define CONN_PTA6_WF_INT_CLR_2_r_wf_wf_ppdu_frmex_rx_over_dw_clr_SHFT 6
#define CONN_PTA6_WF_INT_CLR_2_r_wf_wf_ppdu_frmex_tx_over_dw_clr_ADDR CONN_PTA6_WF_INT_CLR_2_ADDR
#define CONN_PTA6_WF_INT_CLR_2_r_wf_wf_ppdu_frmex_tx_over_dw_clr_MASK 0x00000030
#define CONN_PTA6_WF_INT_CLR_2_r_wf_wf_ppdu_frmex_tx_over_dw_clr_SHFT 4
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_seq_num_skip_int_clr_ADDR CONN_PTA6_WF_INT_CLR_2_ADDR
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_seq_num_skip_int_clr_MASK 0x00000008
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_seq_num_skip_int_clr_SHFT 3
#define CONN_PTA6_WF_INT_CLR_2_r_wf_undef_cmd_rcvd_int_clr_ADDR CONN_PTA6_WF_INT_CLR_2_ADDR
#define CONN_PTA6_WF_INT_CLR_2_r_wf_undef_cmd_rcvd_int_clr_MASK 0x00000004
#define CONN_PTA6_WF_INT_CLR_2_r_wf_undef_cmd_rcvd_int_clr_SHFT 2
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_cmd_nr_slp_dur_rcvd_int_clr_ADDR CONN_PTA6_WF_INT_CLR_2_ADDR
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_cmd_nr_slp_dur_rcvd_int_clr_MASK 0x00000002
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_cmd_nr_slp_dur_rcvd_int_clr_SHFT 1
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_cmd_lte_slp_dur_rcvd_int_clr_ADDR CONN_PTA6_WF_INT_CLR_2_ADDR
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_cmd_lte_slp_dur_rcvd_int_clr_MASK 0x00000001
#define CONN_PTA6_WF_INT_CLR_2_r_wf_idc_cmd_lte_slp_dur_rcvd_int_clr_SHFT 0

#define CONN_PTA6_BT_INT_CLR_2_r_bt_wf_ppdu_frmex_rx_over_dw_clr_ADDR CONN_PTA6_BT_INT_CLR_2_ADDR
#define CONN_PTA6_BT_INT_CLR_2_r_bt_wf_ppdu_frmex_rx_over_dw_clr_MASK 0x000000C0
#define CONN_PTA6_BT_INT_CLR_2_r_bt_wf_ppdu_frmex_rx_over_dw_clr_SHFT 6
#define CONN_PTA6_BT_INT_CLR_2_r_bt_wf_ppdu_frmex_tx_over_dw_clr_ADDR CONN_PTA6_BT_INT_CLR_2_ADDR
#define CONN_PTA6_BT_INT_CLR_2_r_bt_wf_ppdu_frmex_tx_over_dw_clr_MASK 0x00000030
#define CONN_PTA6_BT_INT_CLR_2_r_bt_wf_ppdu_frmex_tx_over_dw_clr_SHFT 4
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_seq_num_skip_int_clr_ADDR CONN_PTA6_BT_INT_CLR_2_ADDR
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_seq_num_skip_int_clr_MASK 0x00000008
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_seq_num_skip_int_clr_SHFT 3
#define CONN_PTA6_BT_INT_CLR_2_r_bt_undef_cmd_rcvd_int_clr_ADDR CONN_PTA6_BT_INT_CLR_2_ADDR
#define CONN_PTA6_BT_INT_CLR_2_r_bt_undef_cmd_rcvd_int_clr_MASK 0x00000004
#define CONN_PTA6_BT_INT_CLR_2_r_bt_undef_cmd_rcvd_int_clr_SHFT 2
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_cmd_nr_slp_dur_rcvd_int_clr_ADDR CONN_PTA6_BT_INT_CLR_2_ADDR
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_cmd_nr_slp_dur_rcvd_int_clr_MASK 0x00000002
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_cmd_nr_slp_dur_rcvd_int_clr_SHFT 1
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_cmd_lte_slp_dur_rcvd_int_clr_ADDR CONN_PTA6_BT_INT_CLR_2_ADDR
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_cmd_lte_slp_dur_rcvd_int_clr_MASK 0x00000001
#define CONN_PTA6_BT_INT_CLR_2_r_bt_idc_cmd_lte_slp_dur_rcvd_int_clr_SHFT 0

#define CONN_PTA6_DBG_CNTR_CLR_r_dbg_cnt_mode_ADDR             CONN_PTA6_DBG_CNTR_CLR_ADDR
#define CONN_PTA6_DBG_CNTR_CLR_r_dbg_cnt_mode_MASK             0x00000100
#define CONN_PTA6_DBG_CNTR_CLR_r_dbg_cnt_mode_SHFT             8
#define CONN_PTA6_DBG_CNTR_CLR_r_rx_idc_cnt_clr_ADDR           CONN_PTA6_DBG_CNTR_CLR_ADDR
#define CONN_PTA6_DBG_CNTR_CLR_r_rx_idc_cnt_clr_MASK           0x00000010
#define CONN_PTA6_DBG_CNTR_CLR_r_rx_idc_cnt_clr_SHFT           4
#define CONN_PTA6_DBG_CNTR_CLR_r_arb_req_cnt_clr_ADDR          CONN_PTA6_DBG_CNTR_CLR_ADDR
#define CONN_PTA6_DBG_CNTR_CLR_r_arb_req_cnt_clr_MASK          0x00000008
#define CONN_PTA6_DBG_CNTR_CLR_r_arb_req_cnt_clr_SHFT          3
#define CONN_PTA6_DBG_CNTR_CLR_r_tx_idc_cnt_clr_ADDR           CONN_PTA6_DBG_CNTR_CLR_ADDR
#define CONN_PTA6_DBG_CNTR_CLR_r_tx_idc_cnt_clr_MASK           0x00000004
#define CONN_PTA6_DBG_CNTR_CLR_r_tx_idc_cnt_clr_SHFT           2
#define CONN_PTA6_DBG_CNTR_CLR_r_tdm_ntf_cnt_clr_ADDR          CONN_PTA6_DBG_CNTR_CLR_ADDR
#define CONN_PTA6_DBG_CNTR_CLR_r_tdm_ntf_cnt_clr_MASK          0x00000002
#define CONN_PTA6_DBG_CNTR_CLR_r_tdm_ntf_cnt_clr_SHFT          1
#define CONN_PTA6_DBG_CNTR_CLR_r_grnt_ntf_cnt_clr_ADDR         CONN_PTA6_DBG_CNTR_CLR_ADDR
#define CONN_PTA6_DBG_CNTR_CLR_r_grnt_ntf_cnt_clr_MASK         0x00000001
#define CONN_PTA6_DBG_CNTR_CLR_r_grnt_ntf_cnt_clr_SHFT         0

#define CONN_PTA6_RO_DBG_CNTR_0_ro_tdm_ntf_cnt_ADDR            CONN_PTA6_RO_DBG_CNTR_0_ADDR
#define CONN_PTA6_RO_DBG_CNTR_0_ro_tdm_ntf_cnt_MASK            0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_0_ro_tdm_ntf_cnt_SHFT            16
#define CONN_PTA6_RO_DBG_CNTR_0_ro_tx_grnt_ntf_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_0_ADDR
#define CONN_PTA6_RO_DBG_CNTR_0_ro_tx_grnt_ntf_cnt_MASK        0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_0_ro_tx_grnt_ntf_cnt_SHFT        8
#define CONN_PTA6_RO_DBG_CNTR_0_ro_rx_grnt_ntf_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_0_ADDR
#define CONN_PTA6_RO_DBG_CNTR_0_ro_rx_grnt_ntf_cnt_MASK        0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_0_ro_rx_grnt_ntf_cnt_SHFT        0

#define CONN_PTA6_RO_DBG_CNTR_1_ro_bt_tx_idc_cnt_ADDR          CONN_PTA6_RO_DBG_CNTR_1_ADDR
#define CONN_PTA6_RO_DBG_CNTR_1_ro_bt_tx_idc_cnt_MASK          0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_1_ro_bt_tx_idc_cnt_SHFT          24
#define CONN_PTA6_RO_DBG_CNTR_1_ro_wf_tx_idc_cnt_ADDR          CONN_PTA6_RO_DBG_CNTR_1_ADDR
#define CONN_PTA6_RO_DBG_CNTR_1_ro_wf_tx_idc_cnt_MASK          0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_1_ro_wf_tx_idc_cnt_SHFT          16
#define CONN_PTA6_RO_DBG_CNTR_1_ro_rx_idc_cnt_ADDR             CONN_PTA6_RO_DBG_CNTR_1_ADDR
#define CONN_PTA6_RO_DBG_CNTR_1_ro_rx_idc_cnt_MASK             0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_1_ro_rx_idc_cnt_SHFT             8
#define CONN_PTA6_RO_DBG_CNTR_1_ro_tx_idc_cnt_ADDR             CONN_PTA6_RO_DBG_CNTR_1_ADDR
#define CONN_PTA6_RO_DBG_CNTR_1_ro_tx_idc_cnt_MASK             0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_1_ro_tx_idc_cnt_SHFT             0

#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_rx1_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_2_ADDR
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_rx1_req_cnt_MASK        0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_rx1_req_cnt_SHFT        24
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_tx1_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_2_ADDR
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_tx1_req_cnt_MASK        0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_tx1_req_cnt_SHFT        16
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_rx0_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_2_ADDR
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_rx0_req_cnt_MASK        0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_rx0_req_cnt_SHFT        8
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_tx0_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_2_ADDR
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_tx0_req_cnt_MASK        0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_2_ro_lte_tx0_req_cnt_SHFT        0

#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_rx3_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_3_ADDR
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_rx3_req_cnt_MASK        0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_rx3_req_cnt_SHFT        24
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_tx3_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_3_ADDR
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_tx3_req_cnt_MASK        0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_tx3_req_cnt_SHFT        16
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_rx2_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_3_ADDR
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_rx2_req_cnt_MASK        0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_rx2_req_cnt_SHFT        8
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_tx2_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_3_ADDR
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_tx2_req_cnt_MASK        0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_3_ro_lte_tx2_req_cnt_SHFT        0

#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_rx5_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_4_ADDR
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_rx5_req_cnt_MASK        0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_rx5_req_cnt_SHFT        24
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_tx5_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_4_ADDR
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_tx5_req_cnt_MASK        0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_tx5_req_cnt_SHFT        16
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_rx4_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_4_ADDR
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_rx4_req_cnt_MASK        0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_rx4_req_cnt_SHFT        8
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_tx4_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_4_ADDR
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_tx4_req_cnt_MASK        0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_4_ro_lte_tx4_req_cnt_SHFT        0

#define CONN_PTA6_RO_DBG_CNTR_5_ro_wf_rx0_req_cnt_ADDR         CONN_PTA6_RO_DBG_CNTR_5_ADDR
#define CONN_PTA6_RO_DBG_CNTR_5_ro_wf_rx0_req_cnt_MASK         0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_5_ro_wf_rx0_req_cnt_SHFT         24
#define CONN_PTA6_RO_DBG_CNTR_5_ro_wf_tx0_req_cnt_ADDR         CONN_PTA6_RO_DBG_CNTR_5_ADDR
#define CONN_PTA6_RO_DBG_CNTR_5_ro_wf_tx0_req_cnt_MASK         0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_5_ro_wf_tx0_req_cnt_SHFT         16
#define CONN_PTA6_RO_DBG_CNTR_5_ro_lte_rx7_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_5_ADDR
#define CONN_PTA6_RO_DBG_CNTR_5_ro_lte_rx7_req_cnt_MASK        0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_5_ro_lte_rx7_req_cnt_SHFT        8
#define CONN_PTA6_RO_DBG_CNTR_5_ro_lte_rx6_req_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_5_ADDR
#define CONN_PTA6_RO_DBG_CNTR_5_ro_lte_rx6_req_cnt_MASK        0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_5_ro_lte_rx6_req_cnt_SHFT        0

#define CONN_PTA6_RO_DBG_CNTR_6_ro_bt_rx_req_cnt_ADDR          CONN_PTA6_RO_DBG_CNTR_6_ADDR
#define CONN_PTA6_RO_DBG_CNTR_6_ro_bt_rx_req_cnt_MASK          0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_6_ro_bt_rx_req_cnt_SHFT          24
#define CONN_PTA6_RO_DBG_CNTR_6_ro_bt_tx_req_cnt_ADDR          CONN_PTA6_RO_DBG_CNTR_6_ADDR
#define CONN_PTA6_RO_DBG_CNTR_6_ro_bt_tx_req_cnt_MASK          0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_6_ro_bt_tx_req_cnt_SHFT          16
#define CONN_PTA6_RO_DBG_CNTR_6_ro_wf_rx1_req_cnt_ADDR         CONN_PTA6_RO_DBG_CNTR_6_ADDR
#define CONN_PTA6_RO_DBG_CNTR_6_ro_wf_rx1_req_cnt_MASK         0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_6_ro_wf_rx1_req_cnt_SHFT         8
#define CONN_PTA6_RO_DBG_CNTR_6_ro_wf_tx1_req_cnt_ADDR         CONN_PTA6_RO_DBG_CNTR_6_ADDR
#define CONN_PTA6_RO_DBG_CNTR_6_ro_wf_tx1_req_cnt_MASK         0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_6_ro_wf_tx1_req_cnt_SHFT         0

#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_rx1_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_8_ADDR
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_rx1_grnt_cnt_MASK       0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_rx1_grnt_cnt_SHFT       24
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_tx1_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_8_ADDR
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_tx1_grnt_cnt_MASK       0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_tx1_grnt_cnt_SHFT       16
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_rx0_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_8_ADDR
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_rx0_grnt_cnt_MASK       0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_rx0_grnt_cnt_SHFT       8
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_tx0_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_8_ADDR
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_tx0_grnt_cnt_MASK       0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_8_ro_lte_tx0_grnt_cnt_SHFT       0

#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_rx3_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_9_ADDR
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_rx3_grnt_cnt_MASK       0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_rx3_grnt_cnt_SHFT       24
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_tx3_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_9_ADDR
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_tx3_grnt_cnt_MASK       0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_tx3_grnt_cnt_SHFT       16
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_rx2_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_9_ADDR
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_rx2_grnt_cnt_MASK       0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_rx2_grnt_cnt_SHFT       8
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_tx2_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_9_ADDR
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_tx2_grnt_cnt_MASK       0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_9_ro_lte_tx2_grnt_cnt_SHFT       0

#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_rx5_grnt_cnt_ADDR      CONN_PTA6_RO_DBG_CNTR_10_ADDR
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_rx5_grnt_cnt_MASK      0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_rx5_grnt_cnt_SHFT      24
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_tx5_grnt_cnt_ADDR      CONN_PTA6_RO_DBG_CNTR_10_ADDR
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_tx5_grnt_cnt_MASK      0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_tx5_grnt_cnt_SHFT      16
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_rx4_grnt_cnt_ADDR      CONN_PTA6_RO_DBG_CNTR_10_ADDR
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_rx4_grnt_cnt_MASK      0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_rx4_grnt_cnt_SHFT      8
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_tx4_grnt_cnt_ADDR      CONN_PTA6_RO_DBG_CNTR_10_ADDR
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_tx4_grnt_cnt_MASK      0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_10_ro_lte_tx4_grnt_cnt_SHFT      0

#define CONN_PTA6_RO_DBG_CNTR_11_ro_wf_rx0_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_11_ADDR
#define CONN_PTA6_RO_DBG_CNTR_11_ro_wf_rx0_grnt_cnt_MASK       0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_11_ro_wf_rx0_grnt_cnt_SHFT       24
#define CONN_PTA6_RO_DBG_CNTR_11_ro_wf_tx0_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_11_ADDR
#define CONN_PTA6_RO_DBG_CNTR_11_ro_wf_tx0_grnt_cnt_MASK       0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_11_ro_wf_tx0_grnt_cnt_SHFT       16
#define CONN_PTA6_RO_DBG_CNTR_11_ro_lte_rx7_grnt_cnt_ADDR      CONN_PTA6_RO_DBG_CNTR_11_ADDR
#define CONN_PTA6_RO_DBG_CNTR_11_ro_lte_rx7_grnt_cnt_MASK      0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_11_ro_lte_rx7_grnt_cnt_SHFT      8
#define CONN_PTA6_RO_DBG_CNTR_11_ro_lte_rx6_grnt_cnt_ADDR      CONN_PTA6_RO_DBG_CNTR_11_ADDR
#define CONN_PTA6_RO_DBG_CNTR_11_ro_lte_rx6_grnt_cnt_MASK      0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_11_ro_lte_rx6_grnt_cnt_SHFT      0

#define CONN_PTA6_RO_DBG_CNTR_12_ro_bt_rx_grnt_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_12_ADDR
#define CONN_PTA6_RO_DBG_CNTR_12_ro_bt_rx_grnt_cnt_MASK        0xFF000000
#define CONN_PTA6_RO_DBG_CNTR_12_ro_bt_rx_grnt_cnt_SHFT        24
#define CONN_PTA6_RO_DBG_CNTR_12_ro_bt_tx_grnt_cnt_ADDR        CONN_PTA6_RO_DBG_CNTR_12_ADDR
#define CONN_PTA6_RO_DBG_CNTR_12_ro_bt_tx_grnt_cnt_MASK        0x00FF0000
#define CONN_PTA6_RO_DBG_CNTR_12_ro_bt_tx_grnt_cnt_SHFT        16
#define CONN_PTA6_RO_DBG_CNTR_12_ro_wf_rx1_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_12_ADDR
#define CONN_PTA6_RO_DBG_CNTR_12_ro_wf_rx1_grnt_cnt_MASK       0x0000FF00
#define CONN_PTA6_RO_DBG_CNTR_12_ro_wf_rx1_grnt_cnt_SHFT       8
#define CONN_PTA6_RO_DBG_CNTR_12_ro_wf_tx1_grnt_cnt_ADDR       CONN_PTA6_RO_DBG_CNTR_12_ADDR
#define CONN_PTA6_RO_DBG_CNTR_12_ro_wf_tx1_grnt_cnt_MASK       0x000000FF
#define CONN_PTA6_RO_DBG_CNTR_12_ro_wf_tx1_grnt_cnt_SHFT       0

#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_4b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_4b_MASK 0xFF000000
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_4b_SHFT 24
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_3b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_3b_MASK 0x00FF0000
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_3b_SHFT 16
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_2b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_2b_MASK 0x0000FF00
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_2b_SHFT 8
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_1b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_0_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_1b_MASK 0x000000FF
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_0_r_bt_idc_cmd_out_1b_SHFT 0

#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_8b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_8b_MASK 0xFF000000
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_8b_SHFT 24
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_7b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_7b_MASK 0x00FF0000
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_7b_SHFT 16
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_6b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_6b_MASK 0x0000FF00
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_6b_SHFT 8
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_5b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_1_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_5b_MASK 0x000000FF
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_1_r_bt_idc_cmd_out_5b_SHFT 0

#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_r_bt_idc_cmd_out_gen_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_2_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_r_bt_idc_cmd_out_gen_MASK 0x00000100
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_r_bt_idc_cmd_out_gen_SHFT 8
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_r_bt_idc_cmd_out_9b_ADDR CONN_PTA6_BT2LTE_CMD_TX_GEN_2_ADDR
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_r_bt_idc_cmd_out_9b_MASK 0x000000FF
#define CONN_PTA6_BT2LTE_CMD_TX_GEN_2_r_bt_idc_cmd_out_9b_SHFT 0

#define CONN_PTA6_DBGMON_EN_r_dbgmon_en_ADDR                   CONN_PTA6_DBGMON_EN_ADDR
#define CONN_PTA6_DBGMON_EN_r_dbgmon_en_MASK                   0x00000001
#define CONN_PTA6_DBGMON_EN_r_dbgmon_en_SHFT                   0

#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_3_ADDR               CONN_PTA6_DBGMON_SEL_ADDR
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_3_MASK               0x1F000000
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_3_SHFT               24
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_2_ADDR               CONN_PTA6_DBGMON_SEL_ADDR
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_2_MASK               0x001F0000
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_2_SHFT               16
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_1_ADDR               CONN_PTA6_DBGMON_SEL_ADDR
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_1_MASK               0x00001F00
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_1_SHFT               8
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_0_ADDR               CONN_PTA6_DBGMON_SEL_ADDR
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_0_MASK               0x0000001F
#define CONN_PTA6_DBGMON_SEL_r_dbgmon_sel_0_SHFT               0

#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_3_ADDR     CONN_PTA6_DBGMON_BIT_SEL_0_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_3_MASK     0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_3_SHFT     24
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_2_ADDR     CONN_PTA6_DBGMON_BIT_SEL_0_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_2_MASK     0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_2_SHFT     16
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_1_ADDR     CONN_PTA6_DBGMON_BIT_SEL_0_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_1_MASK     0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_1_SHFT     8
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_0_ADDR     CONN_PTA6_DBGMON_BIT_SEL_0_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_0_MASK     0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_0_r_dbgmon_bit_sel_0_SHFT     0

#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_7_ADDR     CONN_PTA6_DBGMON_BIT_SEL_1_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_7_MASK     0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_7_SHFT     24
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_6_ADDR     CONN_PTA6_DBGMON_BIT_SEL_1_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_6_MASK     0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_6_SHFT     16
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_5_ADDR     CONN_PTA6_DBGMON_BIT_SEL_1_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_5_MASK     0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_5_SHFT     8
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_4_ADDR     CONN_PTA6_DBGMON_BIT_SEL_1_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_4_MASK     0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_1_r_dbgmon_bit_sel_4_SHFT     0

#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_11_ADDR    CONN_PTA6_DBGMON_BIT_SEL_2_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_11_MASK    0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_11_SHFT    24
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_10_ADDR    CONN_PTA6_DBGMON_BIT_SEL_2_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_10_MASK    0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_10_SHFT    16
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_9_ADDR     CONN_PTA6_DBGMON_BIT_SEL_2_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_9_MASK     0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_9_SHFT     8
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_8_ADDR     CONN_PTA6_DBGMON_BIT_SEL_2_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_8_MASK     0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_2_r_dbgmon_bit_sel_8_SHFT     0

#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_15_ADDR    CONN_PTA6_DBGMON_BIT_SEL_3_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_15_MASK    0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_15_SHFT    24
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_14_ADDR    CONN_PTA6_DBGMON_BIT_SEL_3_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_14_MASK    0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_14_SHFT    16
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_13_ADDR    CONN_PTA6_DBGMON_BIT_SEL_3_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_13_MASK    0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_13_SHFT    8
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_12_ADDR    CONN_PTA6_DBGMON_BIT_SEL_3_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_12_MASK    0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_3_r_dbgmon_bit_sel_12_SHFT    0

#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_19_ADDR    CONN_PTA6_DBGMON_BIT_SEL_4_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_19_MASK    0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_19_SHFT    24
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_18_ADDR    CONN_PTA6_DBGMON_BIT_SEL_4_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_18_MASK    0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_18_SHFT    16
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_17_ADDR    CONN_PTA6_DBGMON_BIT_SEL_4_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_17_MASK    0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_17_SHFT    8
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_16_ADDR    CONN_PTA6_DBGMON_BIT_SEL_4_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_16_MASK    0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_4_r_dbgmon_bit_sel_16_SHFT    0

#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_23_ADDR    CONN_PTA6_DBGMON_BIT_SEL_5_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_23_MASK    0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_23_SHFT    24
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_22_ADDR    CONN_PTA6_DBGMON_BIT_SEL_5_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_22_MASK    0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_22_SHFT    16
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_21_ADDR    CONN_PTA6_DBGMON_BIT_SEL_5_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_21_MASK    0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_21_SHFT    8
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_20_ADDR    CONN_PTA6_DBGMON_BIT_SEL_5_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_20_MASK    0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_5_r_dbgmon_bit_sel_20_SHFT    0

#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_27_ADDR    CONN_PTA6_DBGMON_BIT_SEL_6_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_27_MASK    0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_27_SHFT    24
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_26_ADDR    CONN_PTA6_DBGMON_BIT_SEL_6_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_26_MASK    0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_26_SHFT    16
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_25_ADDR    CONN_PTA6_DBGMON_BIT_SEL_6_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_25_MASK    0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_25_SHFT    8
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_24_ADDR    CONN_PTA6_DBGMON_BIT_SEL_6_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_24_MASK    0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_6_r_dbgmon_bit_sel_24_SHFT    0

#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_31_ADDR    CONN_PTA6_DBGMON_BIT_SEL_7_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_31_MASK    0x3F000000
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_31_SHFT    24
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_30_ADDR    CONN_PTA6_DBGMON_BIT_SEL_7_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_30_MASK    0x003F0000
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_30_SHFT    16
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_29_ADDR    CONN_PTA6_DBGMON_BIT_SEL_7_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_29_MASK    0x00003F00
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_29_SHFT    8
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_28_ADDR    CONN_PTA6_DBGMON_BIT_SEL_7_ADDR
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_28_MASK    0x0000003F
#define CONN_PTA6_DBGMON_BIT_SEL_7_r_dbgmon_bit_sel_28_SHFT    0

#define CONN_PTA6_DBGMON_OUT_ro_dbgmon_out_ADDR                CONN_PTA6_DBGMON_OUT_ADDR
#define CONN_PTA6_DBGMON_OUT_ro_dbgmon_out_MASK                0xFFFFFFFF
#define CONN_PTA6_DBGMON_OUT_ro_dbgmon_out_SHFT                0

#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_wf_def_rb_update_bmp_ADDR CONN_PTA6_WF_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_wf_def_rb_update_bmp_MASK 0x0000F000
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_wf_def_rb_update_bmp_SHFT 12
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_def_idc_rbuf_widx_ADDR CONN_PTA6_WF_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_def_idc_rbuf_widx_MASK 0x00000300
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_def_idc_rbuf_widx_SHFT 8
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_wf_undef_rb_update_bmp_ADDR CONN_PTA6_WF_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_wf_undef_rb_update_bmp_MASK 0x000000F0
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_wf_undef_rb_update_bmp_SHFT 4
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_undef_idc_rbuf_widx_ADDR CONN_PTA6_WF_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_undef_idc_rbuf_widx_MASK 0x00000003
#define CONN_PTA6_WF_IDC_RX_RB_STATUS_ro_undef_idc_rbuf_widx_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_0_ro_undef_idc_rbuf_slot0_0_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_0_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_0_ro_undef_idc_rbuf_slot0_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_0_ro_undef_idc_rbuf_slot0_0_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_1_ro_undef_idc_rbuf_slot0_1_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_1_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_1_ro_undef_idc_rbuf_slot0_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_1_ro_undef_idc_rbuf_slot0_1_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_2_ro_undef_idc_rbuf_slot0_2_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_2_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_2_ro_undef_idc_rbuf_slot0_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT0_2_ro_undef_idc_rbuf_slot0_2_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_0_ro_undef_idc_rbuf_slot1_0_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_0_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_0_ro_undef_idc_rbuf_slot1_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_0_ro_undef_idc_rbuf_slot1_0_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_1_ro_undef_idc_rbuf_slot1_1_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_1_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_1_ro_undef_idc_rbuf_slot1_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_1_ro_undef_idc_rbuf_slot1_1_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_2_ro_undef_idc_rbuf_slot1_2_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_2_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_2_ro_undef_idc_rbuf_slot1_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT1_2_ro_undef_idc_rbuf_slot1_2_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_0_ro_undef_idc_rbuf_slot2_0_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_0_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_0_ro_undef_idc_rbuf_slot2_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_0_ro_undef_idc_rbuf_slot2_0_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_1_ro_undef_idc_rbuf_slot2_1_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_1_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_1_ro_undef_idc_rbuf_slot2_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_1_ro_undef_idc_rbuf_slot2_1_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_2_ro_undef_idc_rbuf_slot2_2_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_2_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_2_ro_undef_idc_rbuf_slot2_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT2_2_ro_undef_idc_rbuf_slot2_2_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_0_ro_undef_idc_rbuf_slot3_0_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_0_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_0_ro_undef_idc_rbuf_slot3_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_0_ro_undef_idc_rbuf_slot3_0_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_1_ro_undef_idc_rbuf_slot3_1_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_1_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_1_ro_undef_idc_rbuf_slot3_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_1_ro_undef_idc_rbuf_slot3_1_SHFT 0

#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_2_ro_undef_idc_rbuf_slot3_2_ADDR CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_2_ADDR
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_2_ro_undef_idc_rbuf_slot3_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_UNDEF_IDC_RB_SLOT3_2_ro_undef_idc_rbuf_slot3_2_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_0_ro_def_idc_rbuf_slot0_0_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT0_0_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_0_ro_def_idc_rbuf_slot0_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_0_ro_def_idc_rbuf_slot0_0_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_1_ro_def_idc_rbuf_slot0_1_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT0_1_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_1_ro_def_idc_rbuf_slot0_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_1_ro_def_idc_rbuf_slot0_1_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_2_ro_def_idc_rbuf_slot0_2_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT0_2_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_2_ro_def_idc_rbuf_slot0_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT0_2_ro_def_idc_rbuf_slot0_2_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_0_ro_def_idc_rbuf_slot1_0_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT1_0_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_0_ro_def_idc_rbuf_slot1_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_0_ro_def_idc_rbuf_slot1_0_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_1_ro_def_idc_rbuf_slot1_1_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT1_1_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_1_ro_def_idc_rbuf_slot1_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_1_ro_def_idc_rbuf_slot1_1_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_2_ro_def_idc_rbuf_slot1_2_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT1_2_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_2_ro_def_idc_rbuf_slot1_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT1_2_ro_def_idc_rbuf_slot1_2_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_0_ro_def_idc_rbuf_slot2_0_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT2_0_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_0_ro_def_idc_rbuf_slot2_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_0_ro_def_idc_rbuf_slot2_0_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_1_ro_def_idc_rbuf_slot2_1_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT2_1_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_1_ro_def_idc_rbuf_slot2_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_1_ro_def_idc_rbuf_slot2_1_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_2_ro_def_idc_rbuf_slot2_2_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT2_2_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_2_ro_def_idc_rbuf_slot2_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT2_2_ro_def_idc_rbuf_slot2_2_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_0_ro_def_idc_rbuf_slot3_0_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT3_0_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_0_ro_def_idc_rbuf_slot3_0_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_0_ro_def_idc_rbuf_slot3_0_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_1_ro_def_idc_rbuf_slot3_1_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT3_1_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_1_ro_def_idc_rbuf_slot3_1_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_1_ro_def_idc_rbuf_slot3_1_SHFT 0

#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_2_ro_def_idc_rbuf_slot3_2_ADDR CONN_PTA6_WF_DEF_IDC_RB_SLOT3_2_ADDR
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_2_ro_def_idc_rbuf_slot3_2_MASK 0xFFFFFFFF
#define CONN_PTA6_WF_DEF_IDC_RB_SLOT3_2_ro_def_idc_rbuf_slot3_2_SHFT 0

#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_bt_def_rb_update_bmp_ADDR CONN_PTA6_BT_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_bt_def_rb_update_bmp_MASK 0x0000F000
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_bt_def_rb_update_bmp_SHFT 12
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_def_idc_rbuf_widx_ADDR CONN_PTA6_BT_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_def_idc_rbuf_widx_MASK 0x00000300
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_def_idc_rbuf_widx_SHFT 8
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_bt_undef_rb_update_bmp_ADDR CONN_PTA6_BT_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_bt_undef_rb_update_bmp_MASK 0x000000F0
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_bt_undef_rb_update_bmp_SHFT 4
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_undef_idc_rbuf_widx_ADDR CONN_PTA6_BT_IDC_RX_RB_STATUS_ADDR
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_undef_idc_rbuf_widx_MASK 0x00000003
#define CONN_PTA6_BT_IDC_RX_RB_STATUS_ro_undef_idc_rbuf_widx_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_0_ro_undef_idc_rbuf_slot0_0_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_0_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_0_ro_undef_idc_rbuf_slot0_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_0_ro_undef_idc_rbuf_slot0_0_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_1_ro_undef_idc_rbuf_slot0_1_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_1_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_1_ro_undef_idc_rbuf_slot0_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_1_ro_undef_idc_rbuf_slot0_1_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_2_ro_undef_idc_rbuf_slot0_2_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_2_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_2_ro_undef_idc_rbuf_slot0_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT0_2_ro_undef_idc_rbuf_slot0_2_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_0_ro_undef_idc_rbuf_slot1_0_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_0_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_0_ro_undef_idc_rbuf_slot1_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_0_ro_undef_idc_rbuf_slot1_0_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_1_ro_undef_idc_rbuf_slot1_1_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_1_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_1_ro_undef_idc_rbuf_slot1_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_1_ro_undef_idc_rbuf_slot1_1_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_2_ro_undef_idc_rbuf_slot1_2_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_2_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_2_ro_undef_idc_rbuf_slot1_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT1_2_ro_undef_idc_rbuf_slot1_2_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_0_ro_undef_idc_rbuf_slot2_0_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_0_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_0_ro_undef_idc_rbuf_slot2_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_0_ro_undef_idc_rbuf_slot2_0_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_1_ro_undef_idc_rbuf_slot2_1_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_1_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_1_ro_undef_idc_rbuf_slot2_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_1_ro_undef_idc_rbuf_slot2_1_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_2_ro_undef_idc_rbuf_slot2_2_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_2_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_2_ro_undef_idc_rbuf_slot2_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT2_2_ro_undef_idc_rbuf_slot2_2_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_0_ro_undef_idc_rbuf_slot3_0_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_0_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_0_ro_undef_idc_rbuf_slot3_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_0_ro_undef_idc_rbuf_slot3_0_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_1_ro_undef_idc_rbuf_slot3_1_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_1_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_1_ro_undef_idc_rbuf_slot3_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_1_ro_undef_idc_rbuf_slot3_1_SHFT 0

#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_2_ro_undef_idc_rbuf_slot3_2_ADDR CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_2_ADDR
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_2_ro_undef_idc_rbuf_slot3_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_UNDEF_IDC_RB_SLOT3_2_ro_undef_idc_rbuf_slot3_2_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_0_ro_def_idc_rbuf_slot0_0_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT0_0_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_0_ro_def_idc_rbuf_slot0_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_0_ro_def_idc_rbuf_slot0_0_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_1_ro_def_idc_rbuf_slot0_1_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT0_1_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_1_ro_def_idc_rbuf_slot0_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_1_ro_def_idc_rbuf_slot0_1_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_2_ro_def_idc_rbuf_slot0_2_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT0_2_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_2_ro_def_idc_rbuf_slot0_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT0_2_ro_def_idc_rbuf_slot0_2_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_0_ro_def_idc_rbuf_slot1_0_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT1_0_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_0_ro_def_idc_rbuf_slot1_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_0_ro_def_idc_rbuf_slot1_0_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_1_ro_def_idc_rbuf_slot1_1_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT1_1_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_1_ro_def_idc_rbuf_slot1_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_1_ro_def_idc_rbuf_slot1_1_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_2_ro_def_idc_rbuf_slot1_2_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT1_2_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_2_ro_def_idc_rbuf_slot1_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT1_2_ro_def_idc_rbuf_slot1_2_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_0_ro_def_idc_rbuf_slot2_0_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT2_0_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_0_ro_def_idc_rbuf_slot2_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_0_ro_def_idc_rbuf_slot2_0_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_1_ro_def_idc_rbuf_slot2_1_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT2_1_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_1_ro_def_idc_rbuf_slot2_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_1_ro_def_idc_rbuf_slot2_1_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_2_ro_def_idc_rbuf_slot2_2_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT2_2_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_2_ro_def_idc_rbuf_slot2_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT2_2_ro_def_idc_rbuf_slot2_2_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_0_ro_def_idc_rbuf_slot3_0_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT3_0_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_0_ro_def_idc_rbuf_slot3_0_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_0_ro_def_idc_rbuf_slot3_0_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_1_ro_def_idc_rbuf_slot3_1_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT3_1_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_1_ro_def_idc_rbuf_slot3_1_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_1_ro_def_idc_rbuf_slot3_1_SHFT 0

#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_2_ro_def_idc_rbuf_slot3_2_ADDR CONN_PTA6_BT_DEF_IDC_RB_SLOT3_2_ADDR
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_2_ro_def_idc_rbuf_slot3_2_MASK 0xFFFFFFFF
#define CONN_PTA6_BT_DEF_IDC_RB_SLOT3_2_ro_def_idc_rbuf_slot3_2_SHFT 0

#define CONN_PTA6_IDC_CMD_FDD_CFG_r_pretx2tx_guard_time_ADDR   CONN_PTA6_IDC_CMD_FDD_CFG_ADDR
#define CONN_PTA6_IDC_CMD_FDD_CFG_r_pretx2tx_guard_time_MASK   0x000000FF
#define CONN_PTA6_IDC_CMD_FDD_CFG_r_pretx2tx_guard_time_SHFT   0

#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ro_wf_bn0_tx_ex_time_ADDR CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ro_wf_bn0_tx_ex_time_MASK 0x7FFF0000
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ro_wf_bn0_tx_ex_time_SHFT 16
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ro_wf_bn0_tx_exceed_dw_time_ADDR CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ro_wf_bn0_tx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_0_ro_wf_bn0_tx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ro_wf_bn0_tx_exceed_dw_int_cnt_ADDR CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ro_wf_bn0_tx_exceed_dw_int_cnt_MASK 0x03FF0000
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ro_wf_bn0_tx_exceed_dw_int_cnt_SHFT 16
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ro_max_wf_bn0_tx_exceed_dw_time_ADDR CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ro_max_wf_bn0_tx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN0_TX_EXCEED_DW_1_ro_max_wf_bn0_tx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_doze_ADDR CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_doze_MASK 0x80000000
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_doze_SHFT 31
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_ex_time_ADDR CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_ex_time_MASK 0x7FFF0000
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_ex_time_SHFT 16
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_decoded_ADDR CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_decoded_MASK 0x00008000
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_decoded_SHFT 15
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_exceed_dw_time_ADDR CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_0_ro_wf_bn0_rx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ro_wf_bn0_rx_exceed_dw_int_cnt_ADDR CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ro_wf_bn0_rx_exceed_dw_int_cnt_MASK 0x03FF0000
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ro_wf_bn0_rx_exceed_dw_int_cnt_SHFT 16
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ro_max_wf_bn0_rx_exceed_dw_time_ADDR CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ro_max_wf_bn0_rx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN0_RX_EXCEED_DW_1_ro_max_wf_bn0_rx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ro_wf_bn1_tx_ex_time_ADDR CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ro_wf_bn1_tx_ex_time_MASK 0x7FFF0000
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ro_wf_bn1_tx_ex_time_SHFT 16
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ro_wf_bn1_tx_exceed_dw_time_ADDR CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ro_wf_bn1_tx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_0_ro_wf_bn1_tx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ro_wf_bn1_tx_exceed_dw_int_cnt_ADDR CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ro_wf_bn1_tx_exceed_dw_int_cnt_MASK 0x03FF0000
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ro_wf_bn1_tx_exceed_dw_int_cnt_SHFT 16
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ro_max_wf_bn1_tx_exceed_dw_time_ADDR CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ro_max_wf_bn1_tx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN1_TX_EXCEED_DW_1_ro_max_wf_bn1_tx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_doze_ADDR CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_doze_MASK 0x80000000
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_doze_SHFT 31
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_ex_time_ADDR CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_ex_time_MASK 0x7FFF0000
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_ex_time_SHFT 16
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_decoded_ADDR CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_decoded_MASK 0x00008000
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_decoded_SHFT 15
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_exceed_dw_time_ADDR CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ADDR
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_0_ro_wf_bn1_rx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ro_wf_bn1_rx_exceed_dw_int_cnt_ADDR CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ro_wf_bn1_rx_exceed_dw_int_cnt_MASK 0x03FF0000
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ro_wf_bn1_rx_exceed_dw_int_cnt_SHFT 16
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ro_max_wf_bn1_rx_exceed_dw_time_ADDR CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ADDR
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ro_max_wf_bn1_rx_exceed_dw_time_MASK 0x00007FFF
#define CONN_PTA6_WF_BN1_RX_EXCEED_DW_1_ro_max_wf_bn1_rx_exceed_dw_time_SHFT 0

#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn1_rx_exceed_dw_int_cnt_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn1_rx_exceed_dw_int_cnt_clr_MASK 0x00008000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn1_rx_exceed_dw_int_cnt_clr_SHFT 15
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn1_tx_exceed_dw_int_cnt_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn1_tx_exceed_dw_int_cnt_clr_MASK 0x00004000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn1_tx_exceed_dw_int_cnt_clr_SHFT 14
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn0_rx_exceed_dw_int_cnt_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn0_rx_exceed_dw_int_cnt_clr_MASK 0x00002000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn0_rx_exceed_dw_int_cnt_clr_SHFT 13
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn0_tx_exceed_dw_int_cnt_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn0_tx_exceed_dw_int_cnt_clr_MASK 0x00001000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_wf_bn0_tx_exceed_dw_int_cnt_clr_SHFT 12
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn1_rx_exceed_dw_time_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn1_rx_exceed_dw_time_clr_MASK 0x00000800
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn1_rx_exceed_dw_time_clr_SHFT 11
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn1_tx_exceed_dw_time_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn1_tx_exceed_dw_time_clr_MASK 0x00000400
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn1_tx_exceed_dw_time_clr_SHFT 10
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn0_rx_exceed_dw_time_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn0_rx_exceed_dw_time_clr_MASK 0x00000200
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn0_rx_exceed_dw_time_clr_SHFT 9
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn0_tx_exceed_dw_time_clr_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn0_tx_exceed_dw_time_clr_MASK 0x00000100
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_max_wf_bn0_tx_exceed_dw_time_clr_SHFT 8
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_doze_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_doze_MASK 0x00000080
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_doze_SHFT 7
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_decoded_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_decoded_MASK 0x00000040
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_decoded_SHFT 6
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_doze_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_doze_MASK 0x00000020
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_doze_SHFT 5
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_decoded_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_decoded_MASK 0x00000010
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_decoded_SHFT 4
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_exceed_dw_en_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_exceed_dw_en_MASK 0x00000008
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_rx_exceed_dw_en_SHFT 3
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_tx_exceed_dw_en_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_tx_exceed_dw_en_MASK 0x00000004
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn1_tx_exceed_dw_en_SHFT 2
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_exceed_dw_en_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_exceed_dw_en_MASK 0x00000002
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_rx_exceed_dw_en_SHFT 1
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_tx_exceed_dw_en_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_tx_exceed_dw_en_MASK 0x00000001
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_0_r_man_wf_bn0_tx_exceed_dw_en_SHFT 0

#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_ex_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_ex_time_MASK 0xFFFE0000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_ex_time_SHFT 17
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_valid_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_valid_MASK 0x00010000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_valid_SHFT 16
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_exceed_dw_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_exceed_dw_time_MASK 0x0000FFFE
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_exceed_dw_time_SHFT 1
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_exceed_dw_int_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_exceed_dw_int_MASK 0x00000001
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_1_r_man_wf_bn0_tx_exceed_dw_int_SHFT 0

#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_ex_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_ex_time_MASK 0xFFFE0000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_ex_time_SHFT 17
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_valid_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_valid_MASK 0x00010000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_valid_SHFT 16
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_exceed_dw_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_exceed_dw_time_MASK 0x0000FFFE
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_exceed_dw_time_SHFT 1
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_exceed_dw_int_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_exceed_dw_int_MASK 0x00000001
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_2_r_man_wf_bn0_rx_exceed_dw_int_SHFT 0

#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_ex_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_ex_time_MASK 0xFFFE0000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_ex_time_SHFT 17
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_valid_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_valid_MASK 0x00010000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_valid_SHFT 16
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_exceed_dw_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_exceed_dw_time_MASK 0x0000FFFE
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_exceed_dw_time_SHFT 1
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_exceed_dw_int_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_exceed_dw_int_MASK 0x00000001
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_3_r_man_wf_bn1_tx_exceed_dw_int_SHFT 0

#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_ex_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_ex_time_MASK 0xFFFE0000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_ex_time_SHFT 17
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_valid_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_valid_MASK 0x00010000
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_valid_SHFT 16
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_exceed_dw_time_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_exceed_dw_time_MASK 0x0000FFFE
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_exceed_dw_time_SHFT 1
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_exceed_dw_int_ADDR CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_ADDR
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_exceed_dw_int_MASK 0x00000001
#define CONN_PTA6_WF_TRX_EXCEED_DW_DBG_4_r_man_wf_bn1_rx_exceed_dw_int_SHFT 0

#define CONN_PTA6_WF_TX_IND_CFG_r_md_exp_wf_tx_pwr_step_ADDR   CONN_PTA6_WF_TX_IND_CFG_ADDR
#define CONN_PTA6_WF_TX_IND_CFG_r_md_exp_wf_tx_pwr_step_MASK   0x00000030
#define CONN_PTA6_WF_TX_IND_CFG_r_md_exp_wf_tx_pwr_step_SHFT   4
#define CONN_PTA6_WF_TX_IND_CFG_r_md_exp_wf_tx_pwr_min_ADDR    CONN_PTA6_WF_TX_IND_CFG_ADDR
#define CONN_PTA6_WF_TX_IND_CFG_r_md_exp_wf_tx_pwr_min_MASK    0x0000000F
#define CONN_PTA6_WF_TX_IND_CFG_r_md_exp_wf_tx_pwr_min_SHFT    0

#define CONN_PTA6_WF_IND_DBG_r_man_wf_rx_active1_ADDR          CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_rx_active1_MASK          0x02000000
#define CONN_PTA6_WF_IND_DBG_r_man_wf_rx_active1_SHFT          25
#define CONN_PTA6_WF_IND_DBG_r_man_wf_rx_active0_ADDR          CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_rx_active0_MASK          0x01000000
#define CONN_PTA6_WF_IND_DBG_r_man_wf_rx_active0_SHFT          24
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr1_ADDR             CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr1_MASK             0x00FF0000
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr1_SHFT             16
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr0_ADDR             CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr0_MASK             0x0000FF00
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr0_SHFT             8
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr_vld1_ADDR         CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr_vld1_MASK         0x00000080
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr_vld1_SHFT         7
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr_vld0_ADDR         CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr_vld0_MASK         0x00000040
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_pwr_vld0_SHFT         6
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_active1_ADDR          CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_active1_MASK          0x00000020
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_active1_SHFT          5
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_active0_ADDR          CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_active0_MASK          0x00000010
#define CONN_PTA6_WF_IND_DBG_r_man_wf_tx_active0_SHFT          4
#define CONN_PTA6_WF_IND_DBG_r_man_idc_wf_ind_en_ADDR          CONN_PTA6_WF_IND_DBG_ADDR
#define CONN_PTA6_WF_IND_DBG_r_man_idc_wf_ind_en_MASK          0x00000001
#define CONN_PTA6_WF_IND_DBG_r_man_idc_wf_ind_en_SHFT          0

#define CONN_PTA6_BT_IND_DBG_r_man_bt_rw_ADDR                  CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_rw_MASK                  0xFFC00000
#define CONN_PTA6_BT_IND_DBG_r_man_bt_rw_SHFT                  22
#define CONN_PTA6_BT_IND_DBG_r_man_bt_center_ADDR              CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_center_MASK              0x003F8000
#define CONN_PTA6_BT_IND_DBG_r_man_bt_center_SHFT              15
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_pwr_ADDR              CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_pwr_MASK              0x00007800
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_pwr_SHFT              11
#define CONN_PTA6_BT_IND_DBG_r_man_bt_next_state_ADDR          CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_next_state_MASK          0x00000600
#define CONN_PTA6_BT_IND_DBG_r_man_bt_next_state_SHFT          9
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_pwr_dbm_en_ADDR       CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_pwr_dbm_en_MASK       0x00000100
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_pwr_dbm_en_SHFT       8
#define CONN_PTA6_BT_IND_DBG_r_man_bt_rx_active_ADDR           CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_rx_active_MASK           0x00000020
#define CONN_PTA6_BT_IND_DBG_r_man_bt_rx_active_SHFT           5
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_active_ADDR           CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_active_MASK           0x00000010
#define CONN_PTA6_BT_IND_DBG_r_man_bt_tx_active_SHFT           4
#define CONN_PTA6_BT_IND_DBG_r_man_idc_bt_ind_en_ADDR          CONN_PTA6_BT_IND_DBG_ADDR
#define CONN_PTA6_BT_IND_DBG_r_man_idc_bt_ind_en_MASK          0x00000001
#define CONN_PTA6_BT_IND_DBG_r_man_idc_bt_ind_en_SHFT          0

#endif /* __CONN_PTA6_REGS_H__ */

