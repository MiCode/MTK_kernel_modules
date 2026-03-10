/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_rgu_on.h
//[Revision time]   : Fri Apr  9 13:47:20 2021

#ifndef __CONN_RGU_ON_REGS_H__
#define __CONN_RGU_ON_REGS_H__

//****************************************************************************
//
//                     CONN_RGU_ON CR Definitions                     
//
//****************************************************************************

#define CONN_RGU_ON_BASE                                       (CONN_REG_CONN_INFRA_RGU_ON_ADDR)

#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR            (CONN_RGU_ON_BASE + 0x000) // 0000
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_ADDR        (CONN_RGU_ON_BASE + 0x004) // 0004
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR                  (CONN_RGU_ON_BASE + 0x010) // 0010
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ON_S_EN_ADDR              (CONN_RGU_ON_BASE + 0x014) // 0014
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR                  (CONN_RGU_ON_BASE + 0x020) // 0020
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ON_S_EN_ADDR             (CONN_RGU_ON_BASE + 0x024) // 0024
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_1_ADDR                (CONN_RGU_ON_BASE + 0x028) // 0028
#define CONN_RGU_ON_MEM_PWR_CTL_ADDR                           (CONN_RGU_ON_BASE + 0x040) // 0040
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ADDR                (CONN_RGU_ON_BASE + 0x044) // 0044
#define CONN_RGU_ON_SYSRAM_HWCTL_PDN_ADDR                      (CONN_RGU_ON_BASE + 0x050) // 0050
#define CONN_RGU_ON_SYSRAM_HWCTL_SLP_ADDR                      (CONN_RGU_ON_BASE + 0x054) // 0054
#define CONN_RGU_ON_SYSRAM_SWCTL_PDN_ADDR                      (CONN_RGU_ON_BASE + 0x058) // 0058
#define CONN_RGU_ON_SYSRAM_SWCTL_SLP_ADDR                      (CONN_RGU_ON_BASE + 0x05C) // 005C
#define CONN_RGU_ON_SYSRAM_SWCTL_CKISO_ADDR                    (CONN_RGU_ON_BASE + 0x060) // 0060
#define CONN_RGU_ON_SYSRAM_SWCTL_ISOINT_ADDR                   (CONN_RGU_ON_BASE + 0x064) // 0064
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_PDN_ADDR                  (CONN_RGU_ON_BASE + 0x070) // 0070
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_SLP_ADDR                  (CONN_RGU_ON_BASE + 0x074) // 0074
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_PDN_ADDR                  (CONN_RGU_ON_BASE + 0x078) // 0078
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_SLP_ADDR                  (CONN_RGU_ON_BASE + 0x07C) // 007C
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_CKISO_ADDR                (CONN_RGU_ON_BASE + 0x080) // 0080
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_ISOINT_ADDR               (CONN_RGU_ON_BASE + 0x084) // 0084
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_PDN_ADDR                 (CONN_RGU_ON_BASE + 0x090) // 0090
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_SLP_ADDR                 (CONN_RGU_ON_BASE + 0x094) // 0094
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_PDN_ADDR                 (CONN_RGU_ON_BASE + 0x098) // 0098
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_SLP_ADDR                 (CONN_RGU_ON_BASE + 0x09C) // 009C
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_CKISO_ADDR               (CONN_RGU_ON_BASE + 0x0A0) // 00A0
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_ISOINT_ADDR              (CONN_RGU_ON_BASE + 0x0A4) // 00A4
#define CONN_RGU_ON_WFSYS_WM_WDT_EN_ADDR                       (CONN_RGU_ON_BASE + 0x100) // 0100
#define CONN_RGU_ON_WFSYS_WA_WDT_EN_ADDR                       (CONN_RGU_ON_BASE + 0x104) // 0104
#define CONN_RGU_ON_BGFSYS_WDT_EN_ADDR                         (CONN_RGU_ON_BASE + 0x108) // 0108
#define CONN_RGU_ON_BGFSYS1_WDT_EN_ADDR                        (CONN_RGU_ON_BASE + 0x10C) // 010C
#define CONN_RGU_ON_WFSYS_CPU_SW_RST_B_ADDR                    (CONN_RGU_ON_BASE + 0x120) // 0120
#define CONN_RGU_ON_BGFSYS_CPU_SW_RST_B_ADDR                   (CONN_RGU_ON_BASE + 0x124) // 0124
#define CONN_RGU_ON_BGFSYS1_CPU_SW_RST_B_ADDR                  (CONN_RGU_ON_BASE + 0x128) // 0128
#define CONN_RGU_ON_WFSYS_SW_RST_B_ADDR                        (CONN_RGU_ON_BASE + 0x140) // 0140
#define CONN_RGU_ON_BGFSYS_SW_RST_B_ADDR                       (CONN_RGU_ON_BASE + 0x144) // 0144
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_ADDR                      (CONN_RGU_ON_BASE + 0x148) // 0148
#define CONN_RGU_ON_SEMA_M2_SW_RST_B_ADDR                      (CONN_RGU_ON_BASE + 0x160) // 0160
#define CONN_RGU_ON_SEMA_M3_SW_RST_B_ADDR                      (CONN_RGU_ON_BASE + 0x164) // 0164
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_ADDR                   (CONN_RGU_ON_BASE + 0x400) // 0400
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_ADDR                  (CONN_RGU_ON_BASE + 0x404) // 0404
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_ADDR             (CONN_RGU_ON_BASE + 0x408) // 0408
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_ADDR               (CONN_RGU_ON_BASE + 0x410) // 0410
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_ADDR              (CONN_RGU_ON_BASE + 0x414) // 0414
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_ADDR         (CONN_RGU_ON_BASE + 0x418) // 0418
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_ADDR              (CONN_RGU_ON_BASE + 0x420) // 0420
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_ADDR             (CONN_RGU_ON_BASE + 0x424) // 0424
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_ADDR            (CONN_RGU_ON_BASE + 0x428) // 0428
#define CONN_RGU_ON_SYSRAM_PDN_ST_ADDR                         (CONN_RGU_ON_BASE + 0x440) // 0440
#define CONN_RGU_ON_SYSRAM_SLP_ST_ADDR                         (CONN_RGU_ON_BASE + 0x444) // 0444
#define CONN_RGU_ON_SYSRAM_CKISO_ST_ADDR                       (CONN_RGU_ON_BASE + 0x448) // 0448
#define CONN_RGU_ON_SYSRAM_ISOINT_ST_ADDR                      (CONN_RGU_ON_BASE + 0x44C) // 044C
#define CONN_RGU_ON_CO_EXT_MEM_PDN_ST_ADDR                     (CONN_RGU_ON_BASE + 0x450) // 0450
#define CONN_RGU_ON_CO_EXT_MEM_SLP_ST_ADDR                     (CONN_RGU_ON_BASE + 0x454) // 0454
#define CONN_RGU_ON_CO_EXT_MEM_CKISO_ST_ADDR                   (CONN_RGU_ON_BASE + 0x458) // 0458
#define CONN_RGU_ON_CO_EXT_MEM_ISOINT_ST_ADDR                  (CONN_RGU_ON_BASE + 0x45C) // 045C
#define CONN_RGU_ON_FM_CTRL_MEM_PDN_ST_ADDR                    (CONN_RGU_ON_BASE + 0x460) // 0460
#define CONN_RGU_ON_FM_CTRL_MEM_SLP_ST_ADDR                    (CONN_RGU_ON_BASE + 0x464) // 0464
#define CONN_RGU_ON_FM_CTRL_MEM_CKISO_ST_ADDR                  (CONN_RGU_ON_BASE + 0x468) // 0468
#define CONN_RGU_ON_FM_CTRL_MEM_ISOINT_ST_ADDR                 (CONN_RGU_ON_BASE + 0x46C) // 046C




/* =====================================================================================

  ---CONN_INFRA_OFF_TOP_PWR_CTL (0x18000000 + 0x000)---

    CONN_INFRA_OFF_TOP_SW_RST_B[0] - (RW) when CONN_INFRA_OFF_TOP mtcmos control flow selection = 0, CONN_INFRA_OFF_TOP power domian reset manual control
                                     0: reset CONN_INFRA_OFF_TOP power domian
                                     1: not reset CONN_INFRA_OFF_TOP power domian
    CONN_INFRA_OFF_TOP_SW_ISO_EN[1] - (RW) when CONN_INFRA_OFF_TOP mtcmos control flow selection = 0, CONN_INFRA_OFF_TOP power domain isolation manual control
                                     0: not isolating CONN_INFRA_OFF_TOP power domain output signals
                                     1: isolating CONN_INFRA_OFF_TOP power domain output signals
    CONN_INFRA_OFF_TOP_SW_PWR_ON_S[2] - (RW) when CONN_INFRA_OFF_TOP mtcmos control flow selection = 0, CONN_INFRA_OFF_TOP mtcmos secondary power switch manual control
                                     0: power off CONN_INFRA_OFF_TOP mtcmos
                                     1: power on CONN_INFRA_OFF_TOP mtcmos
    CONN_INFRA_OFF_TOP_SW_PWR_ON[3] - (RW) when CONN_INFRA_OFF_TOP mtcmos control flow selection = 0, CONN_INFRA_OFF_TOP mtcmos primary power switch manual control
                                     0: power off CONN_INFRA_OFF_TOP mtcmos
                                     1: power on CONN_INFRA_OFF_TOP mtcmos
    CONN_INFRA_OFF_TOP_HWCTL[4]  - (RW) CONN_INFRA_OFF_TOP mtcmos control flow selection
                                     0: SW flow (by CR)
                                     1: H/W flow (by mtcmos FSM)
    CONN_INFRA_OFF_TOP_PWR_CHECK_WITH_MEM_PER_EN[5] - (RW) CONN_INFRA_OFF_TOP power on/off check with mem power state
                                     1'b0: conn_infra_off power off without check mem power off
                                     1'b1: conn_infra_off power off after memory power off
    CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK[6] - (RW) Mask CONN_INFRA_OFF_TOP (pwr_ack == 0  )&(pwr_ack_s == 0) check beforce CONN_INFRA_OFF_TOP pwr_on
                                     0: unmask, rgu check  (pwr_ack == 0  )&(pwr_ack_s == 0) beforce CONN_INFRA_OFF_TOP pwr_on
                                     1: mask, rgu trun on CONN_INFRA_OFF_TOP without check  (pwr_ack == 0  )&(pwr_ack_s == 0)
    RESERVED7[15..7]             - (RO) Reserved bits
    CONN_INFRA_OFF_TOP_WRITE_KEY[31..16] - (WO) when user wants to write any bit of this CR "CONN_INFRA_OFF_TOP_PWR_CTL"
                                     [31:16] must be set to 16'h494E

 =====================================================================================*/
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_WRITE_KEY_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_WRITE_KEY_MASK 0xFFFF0000                // CONN_INFRA_OFF_TOP_WRITE_KEY[31..16]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_WRITE_KEY_SHFT 16
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK_MASK 0x00000040                // CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK[6]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK_SHFT 6
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_PWR_CHECK_WITH_MEM_PER_EN_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_PWR_CHECK_WITH_MEM_PER_EN_MASK 0x00000020                // CONN_INFRA_OFF_TOP_PWR_CHECK_WITH_MEM_PER_EN[5]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_PWR_CHECK_WITH_MEM_PER_EN_SHFT 5
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_HWCTL_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_HWCTL_MASK 0x00000010                // CONN_INFRA_OFF_TOP_HWCTL[4]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_HWCTL_SHFT 4
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_PWR_ON_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_PWR_ON_MASK 0x00000008                // CONN_INFRA_OFF_TOP_SW_PWR_ON[3]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_PWR_ON_SHFT 3
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_PWR_ON_S_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_PWR_ON_S_MASK 0x00000004                // CONN_INFRA_OFF_TOP_SW_PWR_ON_S[2]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_PWR_ON_S_SHFT 2
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_ISO_EN_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_ISO_EN_MASK 0x00000002                // CONN_INFRA_OFF_TOP_SW_ISO_EN[1]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_ISO_EN_SHFT 1
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_RST_B_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_RST_B_MASK 0x00000001                // CONN_INFRA_OFF_TOP_SW_RST_B[0]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_CTL_CONN_INFRA_OFF_TOP_SW_RST_B_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_OFF_TOP_PWR_ON_S_EN (0x18000000 + 0x004)---

    CONN_INFRA_OFF_TOP_PWR_ON_S_EN[0] - (RW) CONN_INFRA_OFF_TOP mtcmos secondary power switch control enable
                                     0: disbale
                                     1: enable
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_MASK 0x00000001                // CONN_INFRA_OFF_TOP_PWR_ON_S_EN[0]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_CONN_INFRA_OFF_TOP_PWR_ON_S_EN_SHFT 0

/* =====================================================================================

  ---WFSYS_ON_TOP_PWR_CTL (0x18000000 + 0x010)---

    WFSYS_ON_TOP_SW_RST_B[0]     - (RW) when WFSYS_ON_TOP mtcmos control flow selection = 0, WFSYS_ON_TOP power domian reset manual control
                                     0: reset WFSYS_ON_TOP power domian
                                     1: not reset WFSYS_ON_TOP power domian
    WFSYS_ON_TOP_SW_ISO_EN[1]    - (RW) when WFSYS_ON_TOP mtcmos control flow selection = 0, WFSYS_ON_TOP power domain isolation manual control
                                     0: not isolating WFSYS_ON_TOP power domain output signals
                                     1: isolating WFSYS_ON_TOP power domain output signals
    WFSYS_ON_TOP_SW_PWR_ON_S[2]  - (RW) when WFSYS_ON_TOP mtcmos control flow selection = 0, WFSYS_ON_TOP mtcmos secondary power switch manual control
                                     0: power off WFSYS_ON_TOP mtcmos
                                     1: power on WFSYS_ON_TOP mtcmos
    WFSYS_ON_TOP_SW_PWR_ON[3]    - (RW) when WFSYS_ON_TOP mtcmos control flow selection = 0, WFSYS_ON_TOP mtcmos primary power switch manual control
                                     0: power off WFSYS_ON_TOP mtcmos
                                     1: power on WFSYS_ON_TOP mtcmos
    WFSYS_ON_TOP_HWCTL[4]        - (RW) WFSYS_ON_TOP mtcmos control flow selection
                                     0: SW flow (by CR)
                                     1: H/W flow (by mtcmos FSM)
    RESERVED5[5]                 - (RO) Reserved bits
    WFSYS_ON_TOP_PWR_ACK_S_MASK[6] - (RW) Mask WFSYS_ON_TOP (pwr_ack == 0  )&(pwr_ack_s == 0) check beforce WFSYS_ON_TOP pwr_on
                                     0: unmask, rgu check  (pwr_ack == 0  )&(pwr_ack_s == 0) beforce WFSYS_ON_TOP pwr_on
                                     1: mask, rgu trun on WFSYS_ON_TOP without check  (pwr_ack == 0  )&(pwr_ack_s == 0)
    WFSYS_ON_TOP_PWR_ON[7]       - (RW) when WFSYS_ON_TOP mtcmos FSM trigger selection = 0
                                     0: trigger FSM to power off WFSYS_ON_TOP mtcmos
                                     1: trigger FSM to power on WFSYS_ON_TOP mtcmos
    WFSYS_ON_TOP_PWR_ON_SRC[8]   - (RW) WFSYS_ON_TOP mtcmos control sorce selection
                                     0: trigger source from WFSYS_ON_TOP_PWR_ON
                                     1: trigger source from chip top
    RESERVED9[15..9]             - (RO) Reserved bits
    WFSYS_ON_TOP_WRITE_KEY[31..16] - (WO) when user wants to write any bit of this CR "WFSYS_ON_TOP_PWR_CTL"
                                     [31:16] must be set to 16'h5746

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_WRITE_KEY_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_WRITE_KEY_MASK 0xFFFF0000                // WFSYS_ON_TOP_WRITE_KEY[31..16]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_WRITE_KEY_SHFT 16
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ON_SRC_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ON_SRC_MASK 0x00000100                // WFSYS_ON_TOP_PWR_ON_SRC[8]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ON_SRC_SHFT 8
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ON_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ON_MASK 0x00000080                // WFSYS_ON_TOP_PWR_ON[7]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ON_SHFT 7
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ACK_S_MASK_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ACK_S_MASK_MASK 0x00000040                // WFSYS_ON_TOP_PWR_ACK_S_MASK[6]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_PWR_ACK_S_MASK_SHFT 6
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_HWCTL_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_HWCTL_MASK 0x00000010                // WFSYS_ON_TOP_HWCTL[4]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_HWCTL_SHFT 4
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_PWR_ON_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_PWR_ON_MASK 0x00000008                // WFSYS_ON_TOP_SW_PWR_ON[3]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_PWR_ON_SHFT 3
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_PWR_ON_S_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_PWR_ON_S_MASK 0x00000004                // WFSYS_ON_TOP_SW_PWR_ON_S[2]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_PWR_ON_S_SHFT 2
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_ISO_EN_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_ISO_EN_MASK 0x00000002                // WFSYS_ON_TOP_SW_ISO_EN[1]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_ISO_EN_SHFT 1
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_RST_B_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_RST_B_MASK 0x00000001                // WFSYS_ON_TOP_SW_RST_B[0]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_CTL_WFSYS_ON_TOP_SW_RST_B_SHFT 0

/* =====================================================================================

  ---WFSYS_ON_TOP_PWR_ON_S_EN (0x18000000 + 0x014)---

    WFSYS_ON_TOP_PWR_ON_S_EN[0]  - (RW) WFSYS_ON_TOP mtcmos secondary power switch control enable
                                     0: disbale
                                     1: enable
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ON_S_EN_WFSYS_ON_TOP_PWR_ON_S_EN_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ON_S_EN_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ON_S_EN_WFSYS_ON_TOP_PWR_ON_S_EN_MASK 0x00000001                // WFSYS_ON_TOP_PWR_ON_S_EN[0]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ON_S_EN_WFSYS_ON_TOP_PWR_ON_S_EN_SHFT 0

/* =====================================================================================

  ---BGFYS_ON_TOP_PWR_CTL (0x18000000 + 0x020)---

    BGFSYS_ON_TOP_SW_RST_B[0]    - (RW) when BGFSYS_ON_TOP mtcmos control flow selection = 0, BGFSYS_ON_TOP power domian reset manual control
                                     0: reset BGFSYS_ON_TOP power domian
                                     1: not reset BGFSYS_ON_TOP power domian
    BGFSYS_ON_TOP_SW_ISO_EN[1]   - (RW) when BGFSYS_ON_TOP mtcmos control flow selection = 0, BGFSYS_ON_TOP power domain isolation manual control
                                     0: not isolating BGFSYS_ON_TOP power domain output signals
                                     1: isolating BGFSYS_ON_TOP power domain output signals
    BGFSYS_ON_TOP_SW_PWR_ON_S[2] - (RW) when BGFSYS_ON_TOP mtcmos control flow selection = 0, BGFSYS_ON_TOP mtcmos secondary power switch manual control
                                     0: power off BGFSYS_ON_TOP mtcmos
                                     1: power on BGFSYS_ON_TOP mtcmos
    BGFSYS_ON_TOP_SW_PWR_ON[3]   - (RW) when BGFSYS_ON_TOP mtcmos control flow selection = 0, BGFSYS_ON_TOP mtcmos primary power switch manual control
                                     0: power off BGFSYS_ON_TOP mtcmos
                                     1: power on BGFSYS_ON_TOP mtcmos
    BGFSYS_ON_TOP_HWCTL[4]       - (RW) BGFSYS_ON_TOP mtcmos control flow selection
                                     0: SW flow (by CR)
                                     1: H/W flow (by mtcmos FSM)
    RESERVED5[5]                 - (RO) Reserved bits
    BGFSYS_ON_TOP_PWR_ACK_S_MASK[6] - (RW) Mask BGFSYS_ON_TOP (pwr_ack == 0  )&(pwr_ack_s == 0) check beforce BGFSYS_ON_TOP pwr_on
                                     0: unmask, rgu check  (pwr_ack == 0  )&(pwr_ack_s == 0) beforce BGFSYS_ON_TOP pwr_on
                                     1: mask, rgu trun on BGFSYS_ON_TOP without check  (pwr_ack == 0  )&(pwr_ack_s == 0)
    BGFSYS_ON_TOP_PWR_ON[7]      - (RW) when BGFSYS_ON_TOP mtcmos FSM trigger selection = 0  (For BT driver use)
                                     0: trigger FSM to power off BGFSYS_ON_TOP mtcmos
                                     1: trigger FSM to power on BGFSYS_ON_TOP mtcmos
    RESERVED8[15..8]             - (RO) Reserved bits
    BGFSYS_ON_TOP_WRITE_KEY[31..16] - (WO) when user wants to write any bit of this CR "BGFSYS_ON_TOP_PWR_CTL"
                                     [31:16] must be set to 16'h4254

 =====================================================================================*/
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_WRITE_KEY_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_WRITE_KEY_MASK 0xFFFF0000                // BGFSYS_ON_TOP_WRITE_KEY[31..16]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_WRITE_KEY_SHFT 16
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ON_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ON_MASK 0x00000080                // BGFSYS_ON_TOP_PWR_ON[7]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ON_SHFT 7
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ACK_S_MASK_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ACK_S_MASK_MASK 0x00000040                // BGFSYS_ON_TOP_PWR_ACK_S_MASK[6]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ACK_S_MASK_SHFT 6
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_HWCTL_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_HWCTL_MASK 0x00000010                // BGFSYS_ON_TOP_HWCTL[4]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_HWCTL_SHFT 4
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_PWR_ON_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_PWR_ON_MASK 0x00000008                // BGFSYS_ON_TOP_SW_PWR_ON[3]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_PWR_ON_SHFT 3
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_PWR_ON_S_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_PWR_ON_S_MASK 0x00000004                // BGFSYS_ON_TOP_SW_PWR_ON_S[2]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_PWR_ON_S_SHFT 2
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_ISO_EN_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_ISO_EN_MASK 0x00000002                // BGFSYS_ON_TOP_SW_ISO_EN[1]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_ISO_EN_SHFT 1
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_RST_B_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_RST_B_MASK 0x00000001                // BGFSYS_ON_TOP_SW_RST_B[0]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_SW_RST_B_SHFT 0

/* =====================================================================================

  ---BGFSYS_ON_TOP_PWR_ON_S_EN (0x18000000 + 0x024)---

    BGFSYS_ON_TOP_PWR_ON_S_EN[0] - (RW) BGFSYS_ON_TOP mtcmos secondary power switch control enable
                                     0: disbale
                                     1: enable
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ON_S_EN_BGFSYS_ON_TOP_PWR_ON_S_EN_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ON_S_EN_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ON_S_EN_BGFSYS_ON_TOP_PWR_ON_S_EN_MASK 0x00000001                // BGFSYS_ON_TOP_PWR_ON_S_EN[0]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ON_S_EN_BGFSYS_ON_TOP_PWR_ON_S_EN_SHFT 0

/* =====================================================================================

  ---BGFYS_ON_TOP_PWR_CTL_1 (0x18000000 + 0x028)---

    RESERVED0[6..0]              - (RO) Reserved bits
    BGFSYS_ON_TOP_PWR_ON_1[7]    - (RW) when BGFSYS_ON_TOP mtcmos FSM trigger selection = 0 (For GPS driver use)
                                     0: trigger FSM to power off BGFSYS_ON_TOP mtcmos
                                     1: trigger FSM to power on BGFSYS_ON_TOP mtcmos
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_1_BGFSYS_ON_TOP_PWR_ON_1_ADDR CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_1_ADDR
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_1_BGFSYS_ON_TOP_PWR_ON_1_MASK 0x00000080                // BGFSYS_ON_TOP_PWR_ON_1[7]
#define CONN_RGU_ON_BGFYS_ON_TOP_PWR_CTL_1_BGFSYS_ON_TOP_PWR_ON_1_SHFT 7

/* =====================================================================================

  ---MEM_PWR_CTL (0x18000000 + 0x040)---

    OSC_RDY_MASK_FOR_MEM_ON[0]   - (RW) conn_infra memory power on check
                                     1'b0: conn_infra mem power on after osc_rdy = 1
                                     1'b1: don't care osc_rdy value
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_MEM_PWR_CTL_OSC_RDY_MASK_FOR_MEM_ON_ADDR   CONN_RGU_ON_MEM_PWR_CTL_ADDR
#define CONN_RGU_ON_MEM_PWR_CTL_OSC_RDY_MASK_FOR_MEM_ON_MASK   0x00000001                // OSC_RDY_MASK_FOR_MEM_ON[0]
#define CONN_RGU_ON_MEM_PWR_CTL_OSC_RDY_MASK_FOR_MEM_ON_SHFT   0

/* =====================================================================================

  ---SYSRAM_SHIFT_CHAIN_CTL (0x18000000 + 0x044)---

    ISO_PRE[7..0]                - (RW) For conn_infra_off memory(conn_infra sysram/co_ex mem/fm_ctrl mem) power-on sequence, the initial delay cycles numbers.
                                     From SYSRAM_PWR_ON 0->1 to RGU hardware generate CS/CK isolation 1->0
                                     (number below 8 are invalid)
    PDN_PRE[15..8]               - (RW) For conn_infra_off memory(conn_infra sysram/co_ex mem/fm_ctrl mem) power-off sequence, the initial delay cycles numbers.
                                     From SYSRAM_PWR_ON 1->0 to RGU hardware generate power down delay chain enable 0->1
    PON_PRE[23..16]              - (RW) For conn_infra_off memory(conn_infra sysram/co_ex mem/fm_ctrl mem) power-on sequence, the initial delay cycles numbers.
                                     From SYSRAM_PWR_ON 0->1 to RGU hardware generate power down delay chain enable 1->0
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_PON_PRE_ADDR        CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ADDR
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_PON_PRE_MASK        0x00FF0000                // PON_PRE[23..16]
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_PON_PRE_SHFT        16
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_PDN_PRE_ADDR        CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ADDR
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_PDN_PRE_MASK        0x0000FF00                // PDN_PRE[15..8]
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_PDN_PRE_SHFT        8
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ISO_PRE_ADDR        CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ADDR
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ISO_PRE_MASK        0x000000FF                // ISO_PRE[7..0]
#define CONN_RGU_ON_SYSRAM_SHIFT_CHAIN_CTL_ISO_PRE_SHFT        0

/* =====================================================================================

  ---SYSRAM_HWCTL_PDN (0x18000000 + 0x050)---

    SYSRAM_HWCTL_PDN[1..0]       - (RW) 0: power-on/power-down control by S/W CR *_SWCTL_*
                                     1: power-on/power-down control by H/W
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_HWCTL_PDN_SYSRAM_HWCTL_PDN_ADDR     CONN_RGU_ON_SYSRAM_HWCTL_PDN_ADDR
#define CONN_RGU_ON_SYSRAM_HWCTL_PDN_SYSRAM_HWCTL_PDN_MASK     0x00000003                // SYSRAM_HWCTL_PDN[1..0]
#define CONN_RGU_ON_SYSRAM_HWCTL_PDN_SYSRAM_HWCTL_PDN_SHFT     0

/* =====================================================================================

  ---SYSRAM_HWCTL_SLP (0x18000000 + 0x054)---

    SYSRAM_HWCTL_SLP[1..0]       - (RW) 0: wakeup/sleep control by S/W CR *_SWCTL_*
                                     1: wakeup/sleep control by H/W
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_HWCTL_SLP_SYSRAM_HWCTL_SLP_ADDR     CONN_RGU_ON_SYSRAM_HWCTL_SLP_ADDR
#define CONN_RGU_ON_SYSRAM_HWCTL_SLP_SYSRAM_HWCTL_SLP_MASK     0x00000003                // SYSRAM_HWCTL_SLP[1..0]
#define CONN_RGU_ON_SYSRAM_HWCTL_SLP_SYSRAM_HWCTL_SLP_SHFT     0

/* =====================================================================================

  ---SYSRAM_SWCTL_PDN (0x18000000 + 0x058)---

    SYSRAM_SWCTL_PDN[1..0]       - (RW) 0: power-down=0
                                     1: power-down=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_SWCTL_PDN_SYSRAM_SWCTL_PDN_ADDR     CONN_RGU_ON_SYSRAM_SWCTL_PDN_ADDR
#define CONN_RGU_ON_SYSRAM_SWCTL_PDN_SYSRAM_SWCTL_PDN_MASK     0x00000003                // SYSRAM_SWCTL_PDN[1..0]
#define CONN_RGU_ON_SYSRAM_SWCTL_PDN_SYSRAM_SWCTL_PDN_SHFT     0

/* =====================================================================================

  ---SYSRAM_SWCTL_SLP (0x18000000 + 0x05C)---

    SYSRAM_SWCTL_SLP[1..0]       - (RW) 0: sleep=0
                                     1: sleep=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_SWCTL_SLP_SYSRAM_SWCTL_SLP_ADDR     CONN_RGU_ON_SYSRAM_SWCTL_SLP_ADDR
#define CONN_RGU_ON_SYSRAM_SWCTL_SLP_SYSRAM_SWCTL_SLP_MASK     0x00000003                // SYSRAM_SWCTL_SLP[1..0]
#define CONN_RGU_ON_SYSRAM_SWCTL_SLP_SYSRAM_SWCTL_SLP_SHFT     0

/* =====================================================================================

  ---SYSRAM_SWCTL_CKISO (0x18000000 + 0x060)---

    SYSRAM_SWCTL_CKISO[1..0]     - (RW) 0: ckiso=0
                                     1: ckiso=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_SWCTL_CKISO_SYSRAM_SWCTL_CKISO_ADDR CONN_RGU_ON_SYSRAM_SWCTL_CKISO_ADDR
#define CONN_RGU_ON_SYSRAM_SWCTL_CKISO_SYSRAM_SWCTL_CKISO_MASK 0x00000003                // SYSRAM_SWCTL_CKISO[1..0]
#define CONN_RGU_ON_SYSRAM_SWCTL_CKISO_SYSRAM_SWCTL_CKISO_SHFT 0

/* =====================================================================================

  ---SYSRAM_SWCTL_ISOINT (0x18000000 + 0x064)---

    SYSRAM_SWCTL_ISOINT[1..0]    - (RW) 0: isoint=0
                                     1: isoint=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_SWCTL_ISOINT_SYSRAM_SWCTL_ISOINT_ADDR CONN_RGU_ON_SYSRAM_SWCTL_ISOINT_ADDR
#define CONN_RGU_ON_SYSRAM_SWCTL_ISOINT_SYSRAM_SWCTL_ISOINT_MASK 0x00000003                // SYSRAM_SWCTL_ISOINT[1..0]
#define CONN_RGU_ON_SYSRAM_SWCTL_ISOINT_SYSRAM_SWCTL_ISOINT_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_HWCTL_PDN (0x18000000 + 0x070)---

    CO_EXT_MEM_HWCTL_PDN[0]      - (RW) 0: power-on/power-down control by S/W CR *_SWCTL_*
                                     1: power-on/power-down control by H/W
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_PDN_CO_EXT_MEM_HWCTL_PDN_ADDR CONN_RGU_ON_CO_EXT_MEM_HWCTL_PDN_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_PDN_CO_EXT_MEM_HWCTL_PDN_MASK 0x00000001                // CO_EXT_MEM_HWCTL_PDN[0]
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_PDN_CO_EXT_MEM_HWCTL_PDN_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_HWCTL_SLP (0x18000000 + 0x074)---

    CO_EXT_MEM_HWCTL_SLP[0]      - (RW) 0: wakeup/sleep control by S/W CR *_SWCTL_*
                                     1: wakeup/sleep control by H/W
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_SLP_CO_EXT_MEM_HWCTL_SLP_ADDR CONN_RGU_ON_CO_EXT_MEM_HWCTL_SLP_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_SLP_CO_EXT_MEM_HWCTL_SLP_MASK 0x00000001                // CO_EXT_MEM_HWCTL_SLP[0]
#define CONN_RGU_ON_CO_EXT_MEM_HWCTL_SLP_CO_EXT_MEM_HWCTL_SLP_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_SWCTL_PDN (0x18000000 + 0x078)---

    CO_EXT_MEM_SWCTL_PDN[0]      - (RW) 0: power-down=0
                                     1: power-down=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_PDN_CO_EXT_MEM_SWCTL_PDN_ADDR CONN_RGU_ON_CO_EXT_MEM_SWCTL_PDN_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_PDN_CO_EXT_MEM_SWCTL_PDN_MASK 0x00000001                // CO_EXT_MEM_SWCTL_PDN[0]
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_PDN_CO_EXT_MEM_SWCTL_PDN_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_SWCTL_SLP (0x18000000 + 0x07C)---

    CO_EXT_MEM_SWCTL_SLP[0]      - (RW) 0: sleep=0
                                     1: sleep=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_SLP_CO_EXT_MEM_SWCTL_SLP_ADDR CONN_RGU_ON_CO_EXT_MEM_SWCTL_SLP_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_SLP_CO_EXT_MEM_SWCTL_SLP_MASK 0x00000001                // CO_EXT_MEM_SWCTL_SLP[0]
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_SLP_CO_EXT_MEM_SWCTL_SLP_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_SWCTL_CKISO (0x18000000 + 0x080)---

    CO_EXT_MEM_SWCTL_CKISO[0]    - (RW) 0: ckiso=0
                                     1: ckiso=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_CKISO_CO_EXT_MEM_SWCTL_CKISO_ADDR CONN_RGU_ON_CO_EXT_MEM_SWCTL_CKISO_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_CKISO_CO_EXT_MEM_SWCTL_CKISO_MASK 0x00000001                // CO_EXT_MEM_SWCTL_CKISO[0]
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_CKISO_CO_EXT_MEM_SWCTL_CKISO_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_SWCTL_ISOINT (0x18000000 + 0x084)---

    CO_EXT_MEM_SWCTL_ISOINT[0]   - (RW) 0: isoint=0
                                     1: isoint=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_ISOINT_CO_EXT_MEM_SWCTL_ISOINT_ADDR CONN_RGU_ON_CO_EXT_MEM_SWCTL_ISOINT_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_ISOINT_CO_EXT_MEM_SWCTL_ISOINT_MASK 0x00000001                // CO_EXT_MEM_SWCTL_ISOINT[0]
#define CONN_RGU_ON_CO_EXT_MEM_SWCTL_ISOINT_CO_EXT_MEM_SWCTL_ISOINT_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_HWCTL_PDN (0x18000000 + 0x090)---

    FM_CTRL_MEM_HWCTL_PDN[0]     - (RW) 0: power-on/power-down control by S/W CR *_SWCTL_*
                                     1: power-on/power-down control by H/W
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_PDN_FM_CTRL_MEM_HWCTL_PDN_ADDR CONN_RGU_ON_FM_CTRL_MEM_HWCTL_PDN_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_PDN_FM_CTRL_MEM_HWCTL_PDN_MASK 0x00000001                // FM_CTRL_MEM_HWCTL_PDN[0]
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_PDN_FM_CTRL_MEM_HWCTL_PDN_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_HWCTL_SLP (0x18000000 + 0x094)---

    FM_CTRL_MEM_HWCTL_SLP[0]     - (RW) 0: wakeup/sleep control by S/W CR *_SWCTL_*
                                     1: wakeup/sleep control by H/W
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_SLP_FM_CTRL_MEM_HWCTL_SLP_ADDR CONN_RGU_ON_FM_CTRL_MEM_HWCTL_SLP_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_SLP_FM_CTRL_MEM_HWCTL_SLP_MASK 0x00000001                // FM_CTRL_MEM_HWCTL_SLP[0]
#define CONN_RGU_ON_FM_CTRL_MEM_HWCTL_SLP_FM_CTRL_MEM_HWCTL_SLP_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_SWCTL_PDN (0x18000000 + 0x098)---

    FM_CTRL_MEM_SWCTL_PDN[0]     - (RW) 0: power-down=0
                                     1: power-down=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_PDN_FM_CTRL_MEM_SWCTL_PDN_ADDR CONN_RGU_ON_FM_CTRL_MEM_SWCTL_PDN_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_PDN_FM_CTRL_MEM_SWCTL_PDN_MASK 0x00000001                // FM_CTRL_MEM_SWCTL_PDN[0]
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_PDN_FM_CTRL_MEM_SWCTL_PDN_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_SWCTL_SLP (0x18000000 + 0x09C)---

    FM_CTRL_MEM_SWCTL_SLP[0]     - (RW) 0: sleep=0
                                     1: sleep=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_SLP_FM_CTRL_MEM_SWCTL_SLP_ADDR CONN_RGU_ON_FM_CTRL_MEM_SWCTL_SLP_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_SLP_FM_CTRL_MEM_SWCTL_SLP_MASK 0x00000001                // FM_CTRL_MEM_SWCTL_SLP[0]
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_SLP_FM_CTRL_MEM_SWCTL_SLP_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_SWCTL_CKISO (0x18000000 + 0x0A0)---

    FM_CTRL_MEM_SWCTL_CKISO[0]   - (RW) 0: ckiso=0
                                     1: ckiso=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_CKISO_FM_CTRL_MEM_SWCTL_CKISO_ADDR CONN_RGU_ON_FM_CTRL_MEM_SWCTL_CKISO_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_CKISO_FM_CTRL_MEM_SWCTL_CKISO_MASK 0x00000001                // FM_CTRL_MEM_SWCTL_CKISO[0]
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_CKISO_FM_CTRL_MEM_SWCTL_CKISO_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_SWCTL_ISOINT (0x18000000 + 0x0A4)---

    FM_CTRL_MEM_SWCTL_ISOINT[0]  - (RW) 0: isoint=0
                                     1: isoint=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_ISOINT_FM_CTRL_MEM_SWCTL_ISOINT_ADDR CONN_RGU_ON_FM_CTRL_MEM_SWCTL_ISOINT_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_ISOINT_FM_CTRL_MEM_SWCTL_ISOINT_MASK 0x00000001                // FM_CTRL_MEM_SWCTL_ISOINT[0]
#define CONN_RGU_ON_FM_CTRL_MEM_SWCTL_ISOINT_FM_CTRL_MEM_SWCTL_ISOINT_SHFT 0

/* =====================================================================================

  ---WFSYS_WM_WDT_EN (0x18000000 + 0x100)---

    WFSYS_WM_WDT_RESET_ENABLE[3..0] - (RW) WFSYS WM WDT reset to CONN_INFRA, WFSYS and BGFSYS enable, disable
                                     Note that these 3-bit CR do not be reset by WDT reset, they are reset by power-on reset only
                                     [3]
                                     0: disable WFSYS WM WDT reset to "CONN_INFRA" (SUBSYS only)
                                     1: enable WFSYS WM WDT reset to "CONN_INFRA" (SUBSYS only)
                                     [2]
                                     0: disable WFSYS WM WDT reset to "BGFSYS"
                                     1: enable WFSYS WM WDT reset to "BGFSYS"
                                     [1]
                                     0: disable WFSYS WM WDT reset to "WFSYS"
                                     1: enable WFSYS WM WDT reset to "WFSYS"
                                     [0]
                                     0: disable WFSYS WM WDT reset to "CONN_INFRA"
                                     1: enable WFSYS WM WDT reset to "CONN_INFRA"
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_WM_WDT_EN_WFSYS_WM_WDT_RESET_ENABLE_ADDR CONN_RGU_ON_WFSYS_WM_WDT_EN_ADDR
#define CONN_RGU_ON_WFSYS_WM_WDT_EN_WFSYS_WM_WDT_RESET_ENABLE_MASK 0x0000000F                // WFSYS_WM_WDT_RESET_ENABLE[3..0]
#define CONN_RGU_ON_WFSYS_WM_WDT_EN_WFSYS_WM_WDT_RESET_ENABLE_SHFT 0

/* =====================================================================================

  ---WFSYS_WA_WDT_EN (0x18000000 + 0x104)---

    WFSYS_WA_WDT_RESET_ENABLE[3..0] - (RW) WFSYS WA WDT reset to CONN_INFRA, WFSYS and BGFSYS enable, disable
                                     Note that these 3-bit CR do not be reset by WDT reset, they are reset by power-on reset only
                                     [3]
                                     0: disable WFSYS WA WDT reset to "CONN_INFRA" (SUBSYS only)
                                     1: enable WFSYS WA WDT reset to "CONN_INFRA" (SUBSYS only)
                                     [2]
                                     0: disable WFSYS WA WDT reset to "BGFSYS"
                                     1: enable WFSYS WA WDT reset to "BGFSYS"
                                     [1]
                                     0: disable WFSYS WA WDT reset to "WFSYS"
                                     1: enable WFSYS WA WDT reset to "WFSYS"
                                     [0]
                                     0: disable WFSYS WA WDT reset to "CONN_INFRA"
                                     1: enable WFSYS WA WDT reset to "CONN_INFRA"
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_WA_WDT_EN_WFSYS_WA_WDT_RESET_ENABLE_ADDR CONN_RGU_ON_WFSYS_WA_WDT_EN_ADDR
#define CONN_RGU_ON_WFSYS_WA_WDT_EN_WFSYS_WA_WDT_RESET_ENABLE_MASK 0x0000000F                // WFSYS_WA_WDT_RESET_ENABLE[3..0]
#define CONN_RGU_ON_WFSYS_WA_WDT_EN_WFSYS_WA_WDT_RESET_ENABLE_SHFT 0

/* =====================================================================================

  ---BGFSYS_WDT_EN (0x18000000 + 0x108)---

    BGFSYS_WDT_RESET_ENABLE[3..0] - (RW) BGFSYS WDT reset to "CONN_INFRA, WFSYS and BGFSYS" enable, disable
                                     Note that these 3-bit CR do not be reset by WDT reset, they are reset by power-on reset only
                                     [3]
                                     0: disable BGFSYS WDT reset to "CONN_INFRA" (SUBSYS only)
                                     1: enable BGFSYS WDT reset to "CONN_INFRA" (SUBSYS only)
                                     [2]
                                     0: disable BGFSYS WDT reset to "BGFSYS"
                                     1: enable BGFSYS WDT reset to "BGFSYS"
                                     [1]
                                     0: disable BGFSYS WDT reset to "WFSYS"
                                     1: enable BGFSYS WDT reset to "WFSYS"
                                     [0]
                                     0: disable BGFSYS WDT reset to "CONN_INFRA"
                                     1: enable BGFSYS WDT reset to "CONN_INFRA"
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_WDT_EN_BGFSYS_WDT_RESET_ENABLE_ADDR CONN_RGU_ON_BGFSYS_WDT_EN_ADDR
#define CONN_RGU_ON_BGFSYS_WDT_EN_BGFSYS_WDT_RESET_ENABLE_MASK 0x0000000F                // BGFSYS_WDT_RESET_ENABLE[3..0]
#define CONN_RGU_ON_BGFSYS_WDT_EN_BGFSYS_WDT_RESET_ENABLE_SHFT 0

/* =====================================================================================

  ---BGFSYS1_WDT_EN (0x18000000 + 0x10C)---

    BGFSYS1_WDT_RESET_ENABLE[3..0] - (RW) BGFSYS1 WDT reset to "CONN_INFRA, WFSYS and BGFSYS" enable, disable
                                     Note that these 3-bit CR do not be reset by WDT reset, they are reset by power-on reset only
                                     [3]
                                     0: disable BGFSYS1 WDT reset to "CONN_INFRA" (SUBSYS only)
                                     1: enable BGFSYS1 WDT reset to "CONN_INFRA" (SUBSYS only)
                                     [2]
                                     0: disable BGFSYS1 WDT reset to "BGFSYS"
                                     1: enable BGFSYS1 WDT reset to "BGFSYS"
                                     [1]
                                     0: disable BGFSYS1 WDT reset to "WFSYS"
                                     1: enable BGFSYS1 WDT reset to "WFSYS"
                                     [0]
                                     0: disable BGFSYS1 WDT reset to "CONN_INFRA"
                                     1: enable BGFSYS1 WDT reset to "CONN_INFRA"
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS1_WDT_EN_BGFSYS1_WDT_RESET_ENABLE_ADDR CONN_RGU_ON_BGFSYS1_WDT_EN_ADDR
#define CONN_RGU_ON_BGFSYS1_WDT_EN_BGFSYS1_WDT_RESET_ENABLE_MASK 0x0000000F                // BGFSYS1_WDT_RESET_ENABLE[3..0]
#define CONN_RGU_ON_BGFSYS1_WDT_EN_BGFSYS1_WDT_RESET_ENABLE_SHFT 0

/* =====================================================================================

  ---WFSYS_CPU_SW_RST_B (0x18000000 + 0x120)---

    WFSYS_CPU_SW_RST_B[0]        - (RW) WFSYS CPU software reset (active-low) to WFSYS CPU (depends on MCYSYS of each project, this bit is for WFSYS "WM" CPU only in Harrier project)
                                     0: reset
                                     1: not reset
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_CPU_SW_RST_B_WFSYS_CPU_SW_RST_B_ADDR CONN_RGU_ON_WFSYS_CPU_SW_RST_B_ADDR
#define CONN_RGU_ON_WFSYS_CPU_SW_RST_B_WFSYS_CPU_SW_RST_B_MASK 0x00000001                // WFSYS_CPU_SW_RST_B[0]
#define CONN_RGU_ON_WFSYS_CPU_SW_RST_B_WFSYS_CPU_SW_RST_B_SHFT 0

/* =====================================================================================

  ---BGFSYS_CPU_SW_RST_B (0x18000000 + 0x124)---

    BGFSYS_CPU_SW_RST_B[0]       - (RW) BGFSYS CPU software reset (active-low) to BGFSYS CPU
                                     0: reset
                                     1: not reset
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_CPU_SW_RST_B_BGFSYS_CPU_SW_RST_B_ADDR CONN_RGU_ON_BGFSYS_CPU_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS_CPU_SW_RST_B_BGFSYS_CPU_SW_RST_B_MASK 0x00000001                // BGFSYS_CPU_SW_RST_B[0]
#define CONN_RGU_ON_BGFSYS_CPU_SW_RST_B_BGFSYS_CPU_SW_RST_B_SHFT 0

/* =====================================================================================

  ---BGFSYS1_CPU_SW_RST_B (0x18000000 + 0x128)---

    BGFSYS1_CPU_SW_RST_B[0]      - (RW) BGFSYS1 CPU software reset (active-low) to BGFSYS1 CPU
                                     0: reset
                                     1: not reset
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS1_CPU_SW_RST_B_BGFSYS1_CPU_SW_RST_B_ADDR CONN_RGU_ON_BGFSYS1_CPU_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS1_CPU_SW_RST_B_BGFSYS1_CPU_SW_RST_B_MASK 0x00000001                // BGFSYS1_CPU_SW_RST_B[0]
#define CONN_RGU_ON_BGFSYS1_CPU_SW_RST_B_BGFSYS1_CPU_SW_RST_B_SHFT 0

/* =====================================================================================

  ---WFSYS_SW_RST_B (0x18000000 + 0x140)---

    WFSYS_SW_RST_B[0]            - (RW) WFSYS software reset (active-low)
                                     0: reset
                                     1: not reset
    WFSYS_SW_RST_BYPASS[1]       - (RW) WFSYS software reset bypass waiting "HW auto-switch bus clock" and "HW auto-control slpprot"
                                     0: not bypass
                                     1: bypass
    RESERVED2[3..2]              - (RO) Reserved bits
    WFSYS_SW_INIT_DONE[4]        - (RW) Used for SW to indicate "WFSYS initial done after software reset"
                                     0: SW INIT. On-going
                                     1: SW INIT. Done
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_INIT_DONE_ADDR     CONN_RGU_ON_WFSYS_SW_RST_B_ADDR
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_INIT_DONE_MASK     0x00000010                // WFSYS_SW_INIT_DONE[4]
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_INIT_DONE_SHFT     4
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_RST_BYPASS_ADDR    CONN_RGU_ON_WFSYS_SW_RST_B_ADDR
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_RST_BYPASS_MASK    0x00000002                // WFSYS_SW_RST_BYPASS[1]
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_RST_BYPASS_SHFT    1
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_RST_B_ADDR         CONN_RGU_ON_WFSYS_SW_RST_B_ADDR
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_RST_B_MASK         0x00000001                // WFSYS_SW_RST_B[0]
#define CONN_RGU_ON_WFSYS_SW_RST_B_WFSYS_SW_RST_B_SHFT         0

/* =====================================================================================

  ---BGFSYS_SW_RST_B (0x18000000 + 0x144)---

    BGFSYS_SW_RST_B[0]           - (RW) BGFSYS software reset (active-low)
                                     0: reset
                                     1: not reset
    BGFSYS_SW_RST_BYPASS[1]      - (RW) BGFSYS software reset bypass waiting "HW auto-switch bus clock" and "HW auto-control slpprot"
                                     0: not bypass
                                     1: bypass
    RESERVED2[3..2]              - (RO) Reserved bits
    BGFSYS_SW_INIT_DONE[4]       - (RW) Used for SW to indicate "BGFSYS initial done after software reset"
                                     0: SW INIT. On-going
                                     1: SW INIT. Done
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_INIT_DONE_ADDR   CONN_RGU_ON_BGFSYS_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_INIT_DONE_MASK   0x00000010                // BGFSYS_SW_INIT_DONE[4]
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_INIT_DONE_SHFT   4
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_RST_BYPASS_ADDR  CONN_RGU_ON_BGFSYS_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_RST_BYPASS_MASK  0x00000002                // BGFSYS_SW_RST_BYPASS[1]
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_RST_BYPASS_SHFT  1
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_RST_B_ADDR       CONN_RGU_ON_BGFSYS_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_RST_B_MASK       0x00000001                // BGFSYS_SW_RST_B[0]
#define CONN_RGU_ON_BGFSYS_SW_RST_B_BGFSYS_SW_RST_B_SHFT       0

/* =====================================================================================

  ---BGFSYS1_SW_RST_B (0x18000000 + 0x148)---

    BGFSYS1_SW_RST_B[0]          - (RW) BGFSYS1 software reset (active-low)
                                     0: reset
                                     1: not reset
    RESERVED1[3..1]              - (RO) Reserved bits
    BGFSYS1_SW_INIT_DONE[4]      - (RW) Used for SW to indicate "BGFSYS1 initial done after software reset"
                                     0: SW INIT. On-going
                                     1: SW INIT. Done
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_BGFSYS1_SW_INIT_DONE_ADDR CONN_RGU_ON_BGFSYS1_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_BGFSYS1_SW_INIT_DONE_MASK 0x00000010                // BGFSYS1_SW_INIT_DONE[4]
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_BGFSYS1_SW_INIT_DONE_SHFT 4
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_BGFSYS1_SW_RST_B_ADDR     CONN_RGU_ON_BGFSYS1_SW_RST_B_ADDR
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_BGFSYS1_SW_RST_B_MASK     0x00000001                // BGFSYS1_SW_RST_B[0]
#define CONN_RGU_ON_BGFSYS1_SW_RST_B_BGFSYS1_SW_RST_B_SHFT     0

/* =====================================================================================

  ---SEMA_M2_SW_RST_B (0x18000000 + 0x160)---

    SEMA_M2_SW_RST_B[0]          - (RW) Semaphore M2 software reset (active-low)
                                     0: reset
                                     1: not reset
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SEMA_M2_SW_RST_B_SEMA_M2_SW_RST_B_ADDR     CONN_RGU_ON_SEMA_M2_SW_RST_B_ADDR
#define CONN_RGU_ON_SEMA_M2_SW_RST_B_SEMA_M2_SW_RST_B_MASK     0x00000001                // SEMA_M2_SW_RST_B[0]
#define CONN_RGU_ON_SEMA_M2_SW_RST_B_SEMA_M2_SW_RST_B_SHFT     0

/* =====================================================================================

  ---SEMA_M3_SW_RST_B (0x18000000 + 0x164)---

    SEMA_M3_SW_RST_B[0]          - (RW) Semaphore M3 software reset (active-low)
                                     0: reset
                                     1: not reset
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SEMA_M3_SW_RST_B_SEMA_M3_SW_RST_B_ADDR     CONN_RGU_ON_SEMA_M3_SW_RST_B_ADDR
#define CONN_RGU_ON_SEMA_M3_SW_RST_B_SEMA_M3_SW_RST_B_MASK     0x00000001                // SEMA_M3_SW_RST_B[0]
#define CONN_RGU_ON_SEMA_M3_SW_RST_B_SEMA_M3_SW_RST_B_SHFT     0

/* =====================================================================================

  ---WFSYS_ON_TOP_PWR_ST (0x18000000 + 0x400)---

    WFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST[0] - (RO) WFSYS_ON_TOP mtcmos FSM ON state
    WFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST[1] - (RO) WFSYS_ON_TOP mtcmos FSM OFF state
    WFSYS_ON_TOP_MTCMOS_PWR_FSM[4..2] - (RO) WFSYS_ON_TOP mtcmos FSM state
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_MASK 0x0000001C                // WFSYS_ON_TOP_MTCMOS_PWR_FSM[4..2]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_SHFT 2
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST_MASK 0x00000002                // WFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST[1]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST_SHFT 1
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST_MASK 0x00000001                // WFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST[0]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ST_WFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST_SHFT 0

/* =====================================================================================

  ---BGFSYS_ON_TOP_PWR_ST (0x18000000 + 0x404)---

    BGFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST[0] - (RO) BGFSYS_ON_TOP mtcmos FSM ON state
    BGFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST[1] - (RO) BGFSYS_ON_TOP mtcmos FSM OFF state
    BGFSYS_ON_TOP_MTCMOS_PWR_FSM[4..2] - (RO) BGFSYS_ON_TOP mtcmos FSM state
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_MASK 0x0000001C                // BGFSYS_ON_TOP_MTCMOS_PWR_FSM[4..2]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_SHFT 2
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST_MASK 0x00000002                // BGFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST[1]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_OFF_ST_SHFT 1
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST_MASK 0x00000001                // BGFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST[0]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ST_BGFSYS_ON_TOP_MTCMOS_PWR_FSM_ON_ST_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_OFF_TOP_PWR_ST (0x18000000 + 0x408)---

    CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_ON_ST[0] - (RO) CONN_INFRA_OFF_TOP mtcmos FSM ON state
    CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_OFF_ST[1] - (RO) CONN_INFRA_OFF_TOP mtcmos FSM OFF state
    CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM[4..2] - (RO) CONN_INFRA_OFF_TOP mtcmos FSM state
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_MASK 0x0000001C                // CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM[4..2]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_SHFT 2
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_OFF_ST_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_OFF_ST_MASK 0x00000002                // CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_OFF_ST[1]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_OFF_ST_SHFT 1
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_ON_ST_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_ON_ST_MASK 0x00000001                // CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_ON_ST[0]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ST_CONN_INFRA_OFF_TOP_MTCMOS_PWR_FSM_ON_ST_SHFT 0

/* =====================================================================================

  ---WFSYS_ON_TOP_PWR_ACK_ST (0x18000000 + 0x410)---

    WFSYS_ON_TOP_PWR_ACK_S[23..0] - (RO) WFSYS_ON_TOP secondary power ack (multi-bit)
    WFSYS_ON_TOP_PWR_ACK[24]     - (RO) WFSYS_ON_TOP primary power ack
    AN_WFSYS_ON_TOP_PWR_ACK_S[25] - (RO) WFSYS_ON_TOP secondary power ack (multi-bit AND to 1-bit)
    RESERVED26[31..26]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_AN_WFSYS_ON_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_AN_WFSYS_ON_TOP_PWR_ACK_S_MASK 0x02000000                // AN_WFSYS_ON_TOP_PWR_ACK_S[25]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_AN_WFSYS_ON_TOP_PWR_ACK_S_SHFT 25
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_WFSYS_ON_TOP_PWR_ACK_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_WFSYS_ON_TOP_PWR_ACK_MASK 0x01000000                // WFSYS_ON_TOP_PWR_ACK[24]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_WFSYS_ON_TOP_PWR_ACK_SHFT 24
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_WFSYS_ON_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_WFSYS_ON_TOP_PWR_ACK_S_MASK 0x00FFFFFF                // WFSYS_ON_TOP_PWR_ACK_S[23..0]
#define CONN_RGU_ON_WFSYS_ON_TOP_PWR_ACK_ST_WFSYS_ON_TOP_PWR_ACK_S_SHFT 0

/* =====================================================================================

  ---BGFSYS_ON_TOP_PWR_ACK_ST (0x18000000 + 0x414)---

    BGFSYS_ON_TOP_PWR_ACK_S[0]   - (RO) BGFSYS_ON_TOP secondary power ack (multi-bit)
    RESERVED1[23..1]             - (RO) Reserved bits
    BGFSYS_ON_TOP_PWR_ACK[24]    - (RO) BGFSYS_ON_TOP primary power ack
    AN_BGFSYS_ON_TOP_PWR_ACK_S[25] - (RO) BGFSYS_ON_TOP secondary power ack (multi-bit AND to 1-bit)
    RESERVED26[31..26]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_AN_BGFSYS_ON_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_AN_BGFSYS_ON_TOP_PWR_ACK_S_MASK 0x02000000                // AN_BGFSYS_ON_TOP_PWR_ACK_S[25]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_AN_BGFSYS_ON_TOP_PWR_ACK_S_SHFT 25
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK_MASK 0x01000000                // BGFSYS_ON_TOP_PWR_ACK[24]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK_SHFT 24
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK_S_MASK 0x00000001                // BGFSYS_ON_TOP_PWR_ACK_S[0]
#define CONN_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK_S_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_OFF_TOP_PWR_ACK_ST (0x18000000 + 0x418)---

    CONN_INFRA_OFF_TOP_PWR_ACK_S[0] - (RO) CONN_INFRA_OFF_TOP secondary power ack (multi-bit)
    RESERVED1[23..1]             - (RO) Reserved bits
    CONN_INFRA_OFF_TOP_PWR_ACK[24] - (RO) CONN_INFRA_OFF_TOP primary power ack
    AN_CONN_INFRA_OFF_TOP_PWR_ACK_S[25] - (RO) CONN_INFRA_OFF_TOP secondary power ack (multi-bit AND to 1-bit)
    RESERVED26[31..26]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_AN_CONN_INFRA_OFF_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_AN_CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK 0x02000000                // AN_CONN_INFRA_OFF_TOP_PWR_ACK_S[25]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_AN_CONN_INFRA_OFF_TOP_PWR_ACK_S_SHFT 25
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_CONN_INFRA_OFF_TOP_PWR_ACK_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_CONN_INFRA_OFF_TOP_PWR_ACK_MASK 0x01000000                // CONN_INFRA_OFF_TOP_PWR_ACK[24]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_CONN_INFRA_OFF_TOP_PWR_ACK_SHFT 24
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_CONN_INFRA_OFF_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_CONN_INFRA_OFF_TOP_PWR_ACK_S_MASK 0x00000001                // CONN_INFRA_OFF_TOP_PWR_ACK_S[0]
#define CONN_RGU_ON_CONN_INFRA_OFF_TOP_PWR_ACK_ST_CONN_INFRA_OFF_TOP_PWR_ACK_S_SHFT 0

/* =====================================================================================

  ---WFSYS_OFF_TOP_PWR_ACK_ST (0x18000000 + 0x420)---

    WFSYS_OFF_TOP_PWR_ACK_S[23..0] - (RO) WFSYS_OFF_TOP secondary power ack (multi-bit)
    WFSYS_OFF_TOP_PWR_ACK[24]    - (RO) WFSYS_OFF_TOP primary power ack
    RESERVED25[31..25]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_WFSYS_OFF_TOP_PWR_ACK_ADDR CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_WFSYS_OFF_TOP_PWR_ACK_MASK 0x01000000                // WFSYS_OFF_TOP_PWR_ACK[24]
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_WFSYS_OFF_TOP_PWR_ACK_SHFT 24
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_WFSYS_OFF_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_WFSYS_OFF_TOP_PWR_ACK_S_MASK 0x00FFFFFF                // WFSYS_OFF_TOP_PWR_ACK_S[23..0]
#define CONN_RGU_ON_WFSYS_OFF_TOP_PWR_ACK_ST_WFSYS_OFF_TOP_PWR_ACK_S_SHFT 0

/* =====================================================================================

  ---BGFSYS_OFF_TOP_PWR_ACK_ST (0x18000000 + 0x424)---

    BGFSYS_OFF_TOP_PWR_ACK_S[1..0] - (RO) BGFSYS_OFF_TOP secondary power ack (multi-bit)
    RESERVED2[23..2]             - (RO) Reserved bits
    BGFSYS_OFF_TOP_PWR_ACK[24]   - (RO) BGFSYS_OFF_TOP primary power ack
    RESERVED25[31..25]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_ADDR CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_MASK 0x01000000                // BGFSYS_OFF_TOP_PWR_ACK[24]
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_SHFT 24
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_S_MASK 0x00000003                // BGFSYS_OFF_TOP_PWR_ACK_S[1..0]
#define CONN_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_S_SHFT 0

/* =====================================================================================

  ---BGFSYS_OFF1_TOP_PWR_ACK_ST (0x18000000 + 0x428)---

    BGFSYS_OFF1_TOP_PWR_ACK_S[1..0] - (RO) BGFSYS_OFF1_TOP secondary power ack (multi-bit)
    RESERVED2[23..2]             - (RO) Reserved bits
    BGFSYS_OFF1_TOP_PWR_ACK[24]  - (RO) BGFSYS_OFF1_TOP primary power ack
    RESERVED25[31..25]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_BGFSYS_OFF1_TOP_PWR_ACK_ADDR CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_BGFSYS_OFF1_TOP_PWR_ACK_MASK 0x01000000                // BGFSYS_OFF1_TOP_PWR_ACK[24]
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_BGFSYS_OFF1_TOP_PWR_ACK_SHFT 24
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_BGFSYS_OFF1_TOP_PWR_ACK_S_ADDR CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_ADDR
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_BGFSYS_OFF1_TOP_PWR_ACK_S_MASK 0x00000003                // BGFSYS_OFF1_TOP_PWR_ACK_S[1..0]
#define CONN_RGU_ON_BGFSYS_OFF1_TOP_PWR_ACK_ST_BGFSYS_OFF1_TOP_PWR_ACK_S_SHFT 0

/* =====================================================================================

  ---SYSRAM_PDN_ST (0x18000000 + 0x440)---

    SYSRAM_PDN_ST[1..0]          - (RO) 0: power-down=0
                                     1: power-down=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_PDN_ST_SYSRAM_PDN_ST_ADDR           CONN_RGU_ON_SYSRAM_PDN_ST_ADDR
#define CONN_RGU_ON_SYSRAM_PDN_ST_SYSRAM_PDN_ST_MASK           0x00000003                // SYSRAM_PDN_ST[1..0]
#define CONN_RGU_ON_SYSRAM_PDN_ST_SYSRAM_PDN_ST_SHFT           0

/* =====================================================================================

  ---SYSRAM_SLP_ST (0x18000000 + 0x444)---

    SYSRAM_SLP_ST[1..0]          - (RO) 0: sleep=0
                                     1: sleep=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_SLP_ST_SYSRAM_SLP_ST_ADDR           CONN_RGU_ON_SYSRAM_SLP_ST_ADDR
#define CONN_RGU_ON_SYSRAM_SLP_ST_SYSRAM_SLP_ST_MASK           0x00000003                // SYSRAM_SLP_ST[1..0]
#define CONN_RGU_ON_SYSRAM_SLP_ST_SYSRAM_SLP_ST_SHFT           0

/* =====================================================================================

  ---SYSRAM_CKISO_ST (0x18000000 + 0x448)---

    SYSRAM_CKISO_ST[1..0]        - (RO) 0: ckiso=0
                                     1: ckiso=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_CKISO_ST_SYSRAM_CKISO_ST_ADDR       CONN_RGU_ON_SYSRAM_CKISO_ST_ADDR
#define CONN_RGU_ON_SYSRAM_CKISO_ST_SYSRAM_CKISO_ST_MASK       0x00000003                // SYSRAM_CKISO_ST[1..0]
#define CONN_RGU_ON_SYSRAM_CKISO_ST_SYSRAM_CKISO_ST_SHFT       0

/* =====================================================================================

  ---SYSRAM_ISOINT_ST (0x18000000 + 0x44C)---

    SYSRAM_ISOINT_ST[1..0]       - (RO) 0: isoint=0
                                     1: isoint=1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_SYSRAM_ISOINT_ST_SYSRAM_ISOINT_ST_ADDR     CONN_RGU_ON_SYSRAM_ISOINT_ST_ADDR
#define CONN_RGU_ON_SYSRAM_ISOINT_ST_SYSRAM_ISOINT_ST_MASK     0x00000003                // SYSRAM_ISOINT_ST[1..0]
#define CONN_RGU_ON_SYSRAM_ISOINT_ST_SYSRAM_ISOINT_ST_SHFT     0

/* =====================================================================================

  ---CO_EXT_MEM_PDN_ST (0x18000000 + 0x450)---

    CO_EXT_MEM_PDN_ST[0]         - (RO) 0: power-down=0
                                     1: power-down=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_PDN_ST_CO_EXT_MEM_PDN_ST_ADDR   CONN_RGU_ON_CO_EXT_MEM_PDN_ST_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_PDN_ST_CO_EXT_MEM_PDN_ST_MASK   0x00000001                // CO_EXT_MEM_PDN_ST[0]
#define CONN_RGU_ON_CO_EXT_MEM_PDN_ST_CO_EXT_MEM_PDN_ST_SHFT   0

/* =====================================================================================

  ---CO_EXT_MEM_SLP_ST (0x18000000 + 0x454)---

    CO_EXT_MEM_SLP_ST[0]         - (RO) 0: sleep=0
                                     1: sleep=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_SLP_ST_CO_EXT_MEM_SLP_ST_ADDR   CONN_RGU_ON_CO_EXT_MEM_SLP_ST_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_SLP_ST_CO_EXT_MEM_SLP_ST_MASK   0x00000001                // CO_EXT_MEM_SLP_ST[0]
#define CONN_RGU_ON_CO_EXT_MEM_SLP_ST_CO_EXT_MEM_SLP_ST_SHFT   0

/* =====================================================================================

  ---CO_EXT_MEM_CKISO_ST (0x18000000 + 0x458)---

    CO_EXT_MEM_CKISO_ST[0]       - (RO) 0: ckiso=0
                                     1: ckiso=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_CKISO_ST_CO_EXT_MEM_CKISO_ST_ADDR CONN_RGU_ON_CO_EXT_MEM_CKISO_ST_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_CKISO_ST_CO_EXT_MEM_CKISO_ST_MASK 0x00000001                // CO_EXT_MEM_CKISO_ST[0]
#define CONN_RGU_ON_CO_EXT_MEM_CKISO_ST_CO_EXT_MEM_CKISO_ST_SHFT 0

/* =====================================================================================

  ---CO_EXT_MEM_ISOINT_ST (0x18000000 + 0x45C)---

    CO_EXT_MEM_ISOINT_ST[0]      - (RO) 0: isoint=0
                                     1: isoint=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_CO_EXT_MEM_ISOINT_ST_CO_EXT_MEM_ISOINT_ST_ADDR CONN_RGU_ON_CO_EXT_MEM_ISOINT_ST_ADDR
#define CONN_RGU_ON_CO_EXT_MEM_ISOINT_ST_CO_EXT_MEM_ISOINT_ST_MASK 0x00000001                // CO_EXT_MEM_ISOINT_ST[0]
#define CONN_RGU_ON_CO_EXT_MEM_ISOINT_ST_CO_EXT_MEM_ISOINT_ST_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_PDN_ST (0x18000000 + 0x460)---

    FM_CTRL_MEM_PDN_ST[0]        - (RO) 0: power-down=0
                                     1: power-down=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_PDN_ST_FM_CTRL_MEM_PDN_ST_ADDR CONN_RGU_ON_FM_CTRL_MEM_PDN_ST_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_PDN_ST_FM_CTRL_MEM_PDN_ST_MASK 0x00000001                // FM_CTRL_MEM_PDN_ST[0]
#define CONN_RGU_ON_FM_CTRL_MEM_PDN_ST_FM_CTRL_MEM_PDN_ST_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_SLP_ST (0x18000000 + 0x464)---

    FM_CTRL_MEM_SLP_ST[0]        - (RO) 0: sleep=0
                                     1: sleep=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_SLP_ST_FM_CTRL_MEM_SLP_ST_ADDR CONN_RGU_ON_FM_CTRL_MEM_SLP_ST_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_SLP_ST_FM_CTRL_MEM_SLP_ST_MASK 0x00000001                // FM_CTRL_MEM_SLP_ST[0]
#define CONN_RGU_ON_FM_CTRL_MEM_SLP_ST_FM_CTRL_MEM_SLP_ST_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_CKISO_ST (0x18000000 + 0x468)---

    FM_CTRL_MEM_CKISO_ST[0]      - (RO) 0: ckiso=0
                                     1: ckiso=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_CKISO_ST_FM_CTRL_MEM_CKISO_ST_ADDR CONN_RGU_ON_FM_CTRL_MEM_CKISO_ST_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_CKISO_ST_FM_CTRL_MEM_CKISO_ST_MASK 0x00000001                // FM_CTRL_MEM_CKISO_ST[0]
#define CONN_RGU_ON_FM_CTRL_MEM_CKISO_ST_FM_CTRL_MEM_CKISO_ST_SHFT 0

/* =====================================================================================

  ---FM_CTRL_MEM_ISOINT_ST (0x18000000 + 0x46C)---

    FM_CTRL_MEM_ISOINT_ST[0]     - (RO) 0: isoint=0
                                     1: isoint=1
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RGU_ON_FM_CTRL_MEM_ISOINT_ST_FM_CTRL_MEM_ISOINT_ST_ADDR CONN_RGU_ON_FM_CTRL_MEM_ISOINT_ST_ADDR
#define CONN_RGU_ON_FM_CTRL_MEM_ISOINT_ST_FM_CTRL_MEM_ISOINT_ST_MASK 0x00000001                // FM_CTRL_MEM_ISOINT_ST[0]
#define CONN_RGU_ON_FM_CTRL_MEM_ISOINT_ST_FM_CTRL_MEM_ISOINT_ST_SHFT 0

#endif // __CONN_RGU_ON_REGS_H__
