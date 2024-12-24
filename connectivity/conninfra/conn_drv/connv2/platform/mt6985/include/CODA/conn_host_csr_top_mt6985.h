// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[Revision time]   : Wed Mar 16 14:03:09 2022

#ifndef __CONN_HOST_CSR_TOP_REGS_MT6985_H__
#define __CONN_HOST_CSR_TOP_REGS_MT6985_H__

//****************************************************************************
//
//                     CONN_HOST_CSR_TOP CR Definitions
//
//****************************************************************************

#define CONN_HOST_CSR_TOP_BASE                                 (CONN_HOST_CSR_TOP_BASE_ADDR_MT6985) /* 0x1806_0000 */

#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR                  (CONN_HOST_CSR_TOP_BASE + 0x010) // 0010
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_ADDR               (CONN_HOST_CSR_TOP_BASE + 0x014) // 0014
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_ADDR                (CONN_HOST_CSR_TOP_BASE + 0x018) // 0018
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_ADDR                  (CONN_HOST_CSR_TOP_BASE + 0x020) // 0020
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_STAT_ADDR               (CONN_HOST_CSR_TOP_BASE + 0x024) // 0024
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_ADDR                (CONN_HOST_CSR_TOP_BASE + 0x028) // 0028
#define CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR                       (CONN_HOST_CSR_TOP_BASE + 0x030) // 0030
#define CONN_HOST_CSR_TOP_BGF_IRQ_STAT_ADDR                    (CONN_HOST_CSR_TOP_BASE + 0x034) // 0034
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_ADDR                     (CONN_HOST_CSR_TOP_BASE + 0x038) // 0038
#define CONN_HOST_CSR_TOP_BGF_FW_OWN_IRQ_ADDR                  (CONN_HOST_CSR_TOP_BASE + 0x03C) // 003C
#define CONN_HOST_CSR_TOP_GPS_LPCTL_ADDR                       (CONN_HOST_CSR_TOP_BASE + 0x040) // 0040
#define CONN_HOST_CSR_TOP_GPS_IRQ_STAT_ADDR                    (CONN_HOST_CSR_TOP_BASE + 0x044) // 0044
#define CONN_HOST_CSR_TOP_GPS_IRQ_ENA_ADDR                     (CONN_HOST_CSR_TOP_BASE + 0x048) // 0048
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR                     (CONN_HOST_CSR_TOP_BASE + 0x050) // 0050
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_STAT_ADDR                  (CONN_HOST_CSR_TOP_BASE + 0x054) // 0054
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_ENA_ADDR                   (CONN_HOST_CSR_TOP_BASE + 0x058) // 0058
#define CONN_HOST_CSR_TOP_BT0_MCU_JTAG_CTRL_ADDR               (CONN_HOST_CSR_TOP_BASE + 0x080) // 0080
#define CONN_HOST_CSR_TOP_BT1_MCU_JTAG_CTRL_ADDR               (CONN_HOST_CSR_TOP_BASE + 0x084) // 0084
#define CONN_HOST_CSR_TOP_WF_MCU_JTAG_CTRL_ADDR                (CONN_HOST_CSR_TOP_BASE + 0x088) // 0088
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_ADDR                 (CONN_HOST_CSR_TOP_BASE + 0x08C) // 008C
#define CONN_HOST_CSR_TOP_CONN_ON_MISC_ADDR                    (CONN_HOST_CSR_TOP_BASE + 0x0F0) // 00F0
#define CONN_HOST_CSR_TOP_CONN_SYSSTRAP_ADDR                   (CONN_HOST_CSR_TOP_BASE + 0x0F8) // 00F8
#define CONN_HOST_CSR_TOP_CSR_DEADFEED_EN_ADDR                 (CONN_HOST_CSR_TOP_BASE + 0x124) // 0124
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR (CONN_HOST_CSR_TOP_BASE + 0x15C) // 015C
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR (CONN_HOST_CSR_TOP_BASE + 0x160) // 0160
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR           (CONN_HOST_CSR_TOP_BASE + 0x1A0) // 01A0
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_ADDR            (CONN_HOST_CSR_TOP_BASE + 0x1A4) // 01A4
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_BT_ADDR            (CONN_HOST_CSR_TOP_BASE + 0x1A8) // 01A8
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_ADDR           (CONN_HOST_CSR_TOP_BASE + 0x1AC) // 01AC
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_FM_ADDR            (CONN_HOST_CSR_TOP_BASE + 0x1B0) // 01B0
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_0_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x1B4) // 01B4
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_1_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x1B8) // 01B8
#define CONN_HOST_CSR_TOP_CONN_SEMA_M2_SW_RST_B_ADDR           (CONN_HOST_CSR_TOP_BASE + 0x1F0) // 01F0
#define CONN_HOST_CSR_TOP_CONN_SEMA_M3_SW_RST_B_ADDR           (CONN_HOST_CSR_TOP_BASE + 0x1F4) // 01F4
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x300) // 0300
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x304) // 0304
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x314) // 0314
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x320) // 0320
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x354) // 0354
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x35C) // 035C
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_ADDR (CONN_HOST_CSR_TOP_BASE + 0x360) // 0360
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_EMI_LOCK_ADDR          (CONN_HOST_CSR_TOP_BASE + 0x364) // 0364
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MODE_SWITCH_ADDR       (CONN_HOST_CSR_TOP_BASE + 0x368) // 0368
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_ADDR     (CONN_HOST_CSR_TOP_BASE + 0x380) // 0380
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR (CONN_HOST_CSR_TOP_BASE + 0x384) // 0384
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR       (CONN_HOST_CSR_TOP_BASE + 0x388) // 0388
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR     (CONN_HOST_CSR_TOP_BASE + 0x38C) // 038C
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x700) // 0700
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x704) // 0704
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x708) // 0708
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x720) // 0720
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x724) // 0724
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_ADDR        (CONN_HOST_CSR_TOP_BASE + 0x728) // 0728
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_ADDR       (CONN_HOST_CSR_TOP_BASE + 0x740) // 0740
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_ADDR       (CONN_HOST_CSR_TOP_BASE + 0x744) // 0744
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_ADDR       (CONN_HOST_CSR_TOP_BASE + 0x748) // 0748
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_ADDR          (CONN_HOST_CSR_TOP_BASE + 0x780) // 0780
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_ADDR          (CONN_HOST_CSR_TOP_BASE + 0x784) // 0784
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_ADDR         (CONN_HOST_CSR_TOP_BASE + 0x790) // 0790
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_ADDR         (CONN_HOST_CSR_TOP_BASE + 0x794) // 0794
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_W_R_ADDR_ADDR      (CONN_HOST_CSR_TOP_BASE + 0x7B0) // 07B0
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_SET_ADDR_ADDR      (CONN_HOST_CSR_TOP_BASE + 0x7B4) // 07B4
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_CLR_ADDR_ADDR      (CONN_HOST_CSR_TOP_BASE + 0x7B8) // 07B8
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_W_R_ADDR_ADDR            (CONN_HOST_CSR_TOP_BASE + 0x7C0) // 07C0
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_SET_ADDR_ADDR            (CONN_HOST_CSR_TOP_BASE + 0x7C4) // 07C4
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_CLR_ADDR_ADDR            (CONN_HOST_CSR_TOP_BASE + 0x7C8) // 07C8
#define CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT_ADDR         (CONN_HOST_CSR_TOP_BASE + 0xA00) // 0A00
#define CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR           (CONN_HOST_CSR_TOP_BASE + 0xA04) // 0A04
#define CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR           (CONN_HOST_CSR_TOP_BASE + 0xA08) // 0A08
#define CONN_HOST_CSR_TOP_CONN_INFRA_CLKGEN_ON_DBG_ADDR        (CONN_HOST_CSR_TOP_BASE + 0xA0C) // 0A0C
#define CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR              (CONN_HOST_CSR_TOP_BASE + 0xA10) // 0A10
#define CONN_HOST_CSR_TOP_CONN_INFRA_ON_MONFLAG_ADDR           (CONN_HOST_CSR_TOP_BASE + 0xA14) // 0A14
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_ADDR          (CONN_HOST_CSR_TOP_BASE + 0xB00) // 0B00
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_ADDR         (CONN_HOST_CSR_TOP_BASE + 0xB04) // 0B04
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_EN_FR_HIF_ADDR         (CONN_HOST_CSR_TOP_BASE + 0xB08) // 0B08
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_SEL_FR_HIF_ADDR        (CONN_HOST_CSR_TOP_BASE + 0xB0C) // 0B0C
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_ADDR                (CONN_HOST_CSR_TOP_BASE + 0xB10) // 0B10
#define CONN_HOST_CSR_TOP_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_ADDR  (CONN_HOST_CSR_TOP_BASE + 0xB14) // 0B14
#define CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR               (CONN_HOST_CSR_TOP_BASE + 0xC00) // 0C00
#define CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR       (CONN_HOST_CSR_TOP_BASE + 0xC04) // 0C04




/* =====================================================================================

  ---WF_BAND0_LPCTL (0x18060000 + 0x010)---

    WF_B0_AP_HOST_SET_FW_OWN_HS_PULSE[0] - (A0) Host set this bit to transfer ownership to FW (WF Band 0). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_wf_b0_host_csr_fw_own_sts[0x1800_1404] bit[0] will be set to 1.
    WF_B0_AP_HOST_CLR_FW_OWN_HS_PULSE[1] - (A0) Host set this bit to request ownership back to HOST from FW (WF Band 0). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_wf_b0_host_csr_fw_own_sts[0x1800_1404] bit[1] will be set to 1.
    WF_B0_AP_HOST_OWNER_STATE_SYNC[2] - (RO) [WiFi_Driver_Own_DBG] (WF Band 0)
                                     ap host_csr firmware own status (0: driver own, 1:firmware own)
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_ADDR CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_MASK 0x00000004                // WF_B0_AP_HOST_OWNER_STATE_SYNC[2]
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_SHFT 2
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_CLR_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_CLR_FW_OWN_HS_PULSE_MASK 0x00000002                // WF_B0_AP_HOST_CLR_FW_OWN_HS_PULSE[1]
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_CLR_FW_OWN_HS_PULSE_SHFT 1
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_SET_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_SET_FW_OWN_HS_PULSE_MASK 0x00000001                // WF_B0_AP_HOST_SET_FW_OWN_HS_PULSE[0]
#define CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_SET_FW_OWN_HS_PULSE_SHFT 0

/* =====================================================================================

  ---WF_BAND0_IRQ_STAT (0x18060000 + 0x014)---

    WF_B0_HOST_LPCR_FW_OWN_CLR_STAT[0] - (W1C) Host AP own interrupt. (cause by WF Band 0)
                                     Write 1 clear interrupt status.
                                     0 : no interrupt
                                     1: host_own interrupt
                                     
                                     When firmware write 1 clear u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_wf_b0_host_lpcr_fw_own [0x1800_1408] bit[0] . Hardware would trigger host side host_own_int. It means firmware transfer ownership to driver.
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_WF_B0_HOST_LPCR_FW_OWN_CLR_STAT_ADDR CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_WF_B0_HOST_LPCR_FW_OWN_CLR_STAT_MASK 0x00000001                // WF_B0_HOST_LPCR_FW_OWN_CLR_STAT[0]
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_STAT_WF_B0_HOST_LPCR_FW_OWN_CLR_STAT_SHFT 0

/* =====================================================================================

  ---WF_BAND0_IRQ_ENA (0x18060000 + 0x018)---

    WF_B0_IRQ_ENA[3..0]          - (RW) host AP own interrupt enable(only bit0 used) (cause by WF Band 0)
                                     0 : interrupt disable
                                     1 : interrupt enable
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_WF_B0_IRQ_ENA_ADDR  CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_WF_B0_IRQ_ENA_MASK  0x0000000F                // WF_B0_IRQ_ENA[3..0]
#define CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_WF_B0_IRQ_ENA_SHFT  0

/* =====================================================================================

  ---WF_BAND1_LPCTL (0x18060000 + 0x020)---

    WF_B1_AP_HOST_SET_FW_OWN_HS_PULSE[0] - (A0) Host set this bit to transfer ownership to FW (WF Band 1). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_wf_b1_host_csr_fw_own_sts[0x1800_140C] bit[0] will be set to 1.
    WF_B1_AP_HOST_CLR_FW_OWN_HS_PULSE[1] - (A0) Host set this bit to request ownership back to HOST from FW (WF Band 1). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_wf_b1_host_csr_fw_own_sts[0x1800_140C] bit[1] will be set to 1.
    WF_B1_AP_HOST_OWNER_STATE_SYNC[2] - (RO) [WiFi_Driver_Own_DBG] (WF Band 1)
                                     ap host_csr firmware own status (0: driver own, 1:firmware own)
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_OWNER_STATE_SYNC_ADDR CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_OWNER_STATE_SYNC_MASK 0x00000004                // WF_B1_AP_HOST_OWNER_STATE_SYNC[2]
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_OWNER_STATE_SYNC_SHFT 2
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_CLR_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_CLR_FW_OWN_HS_PULSE_MASK 0x00000002                // WF_B1_AP_HOST_CLR_FW_OWN_HS_PULSE[1]
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_CLR_FW_OWN_HS_PULSE_SHFT 1
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_SET_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_SET_FW_OWN_HS_PULSE_MASK 0x00000001                // WF_B1_AP_HOST_SET_FW_OWN_HS_PULSE[0]
#define CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_WF_B1_AP_HOST_SET_FW_OWN_HS_PULSE_SHFT 0

/* =====================================================================================

  ---WF_BAND1_IRQ_STAT (0x18060000 + 0x024)---

    WF_B1_HOST_LPCR_FW_OWN_CLR_STAT[0] - (W1C) Host AP own interrupt. (cause by WF Band 1)
                                     Write 1 clear interrupt status.
                                     0 : no interrupt
                                     1: host_own interrupt
                                     
                                     When firmware write 1 clear u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_wf_b1_host_lpcr_fw_own [0x1800_1410] bit[0] . Hardware would trigger host side host_own_int. It means firmware transfer ownership to driver.
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_STAT_WF_B1_HOST_LPCR_FW_OWN_CLR_STAT_ADDR CONN_HOST_CSR_TOP_WF_BAND1_IRQ_STAT_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_STAT_WF_B1_HOST_LPCR_FW_OWN_CLR_STAT_MASK 0x00000001                // WF_B1_HOST_LPCR_FW_OWN_CLR_STAT[0]
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_STAT_WF_B1_HOST_LPCR_FW_OWN_CLR_STAT_SHFT 0

/* =====================================================================================

  ---WF_BAND1_IRQ_ENA (0x18060000 + 0x028)---

    WF_B1_IRQ_ENA[3..0]          - (RW) host AP own interrupt enable(only bit0 used) (cause by WF Band 1)
                                     0 : interrupt disable
                                     1 : interrupt enable
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_WF_B1_IRQ_ENA_ADDR  CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_ADDR
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_WF_B1_IRQ_ENA_MASK  0x0000000F                // WF_B1_IRQ_ENA[3..0]
#define CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_WF_B1_IRQ_ENA_SHFT  0

/* =====================================================================================

  ---BGF_LPCTL (0x18060000 + 0x030)---

    BGF_AP_HOST_SET_FW_OWN_HS_PULSE[0] - (A0) Host set this bit to transfer ownership to FW (WF Band 1). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_bgf_host_csr_fw_own_sts[0x1800_141C] bit[0] will be set to 1.
    BGF_AP_HOST_CLR_FW_OWN_HS_PULSE[1] - (A0) Host set this bit to request ownership back to HOST from FW (BGF). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_bgf_host_csr_fw_own_sts[0x1800_141C] bit[1] will be set to 1.
    BGF_AP_HOST_OWNER_STATE_SYNC[2] - (RO) [BGF_Driver_Own_DBG] 
                                     ap host_csr firmware own status (0: driver own, 1:firmware own)
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC_ADDR CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC_MASK 0x00000004                // BGF_AP_HOST_OWNER_STATE_SYNC[2]
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC_SHFT 2
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_CLR_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_CLR_FW_OWN_HS_PULSE_MASK 0x00000002                // BGF_AP_HOST_CLR_FW_OWN_HS_PULSE[1]
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_CLR_FW_OWN_HS_PULSE_SHFT 1
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_SET_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_SET_FW_OWN_HS_PULSE_MASK 0x00000001                // BGF_AP_HOST_SET_FW_OWN_HS_PULSE[0]
#define CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_SET_FW_OWN_HS_PULSE_SHFT 0

/* =====================================================================================

  ---BGF_IRQ_STAT (0x18060000 + 0x034)---

    BGF_HOST_LPCR_FW_OWN_CLR_STAT[0] - (W1C) Host AP own interrupt. (cause by BGF)
                                     Write 1 clear interrupt status.
                                     0 : no interrupt
                                     1: host_own interrupt
                                     
                                     When firmware write 1 clear u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_bgf_host_lpcr_fw_own [0x1800_1420] bit[0] . Hardware would trigger host side host_own_int. It means firmware transfer ownership to driver.
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_IRQ_STAT_BGF_HOST_LPCR_FW_OWN_CLR_STAT_ADDR CONN_HOST_CSR_TOP_BGF_IRQ_STAT_ADDR
#define CONN_HOST_CSR_TOP_BGF_IRQ_STAT_BGF_HOST_LPCR_FW_OWN_CLR_STAT_MASK 0x00000001                // BGF_HOST_LPCR_FW_OWN_CLR_STAT[0]
#define CONN_HOST_CSR_TOP_BGF_IRQ_STAT_BGF_HOST_LPCR_FW_OWN_CLR_STAT_SHFT 0

/* =====================================================================================

  ---BGF_IRQ_ENA (0x18060000 + 0x038)---

    BGF_IRQ_DRIVER_OWN_ENA[0]    - (RW) host AP own interrupt enable for driver own(cause by BGF)
                                     0 : interrupt disable
                                     1 : interrupt enable
    BGF_IRQ_FW_OWN_ENA[1]        - (RW) host AP own interrupt enable for fw own (cause by BGF)
                                     0 : interrupt disable
                                     1 : interrupt enable
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_BGF_IRQ_FW_OWN_ENA_ADDR  CONN_HOST_CSR_TOP_BGF_IRQ_ENA_ADDR
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_BGF_IRQ_FW_OWN_ENA_MASK  0x00000002                // BGF_IRQ_FW_OWN_ENA[1]
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_BGF_IRQ_FW_OWN_ENA_SHFT  1
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_BGF_IRQ_DRIVER_OWN_ENA_ADDR CONN_HOST_CSR_TOP_BGF_IRQ_ENA_ADDR
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_BGF_IRQ_DRIVER_OWN_ENA_MASK 0x00000001                // BGF_IRQ_DRIVER_OWN_ENA[0]
#define CONN_HOST_CSR_TOP_BGF_IRQ_ENA_BGF_IRQ_DRIVER_OWN_ENA_SHFT 0

/* =====================================================================================

  ---BGF_FW_OWN_IRQ (0x18060000 + 0x03C)---

    BGF_FW_OWN_IRQ[0]            - (W1C) FW own interrupt. (cause by BGF)
                                     Write 1 clear interrupt status.
                                     0 : no interrupt
                                     1: fw_own interrupt
                                     
                                     HOST write 1 to this bit to clear irq from bgf fw own
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_FW_OWN_IRQ_BGF_FW_OWN_IRQ_ADDR   CONN_HOST_CSR_TOP_BGF_FW_OWN_IRQ_ADDR
#define CONN_HOST_CSR_TOP_BGF_FW_OWN_IRQ_BGF_FW_OWN_IRQ_MASK   0x00000001                // BGF_FW_OWN_IRQ[0]
#define CONN_HOST_CSR_TOP_BGF_FW_OWN_IRQ_BGF_FW_OWN_IRQ_SHFT   0

/* =====================================================================================

  ---GPS_LPCTL (0x18060000 + 0x040)---

    GPS_AP_HOST_SET_FW_OWN_HS_PULSE[0] - (A0) Host set this bit to transfer ownership to FW (WF Band 1). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_gps_host_csr_fw_own_sts[0x1800_141C] bit[0] will be set to 1.
    GPS_AP_HOST_CLR_FW_OWN_HS_PULSE[1] - (A0) Host set this bit to request ownership back to HOST from FW (GPS). 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_gps_host_csr_fw_own_sts[0x1800_141C] bit[1] will be set to 1.
    GPS_AP_HOST_OWNER_STATE_SYNC[2] - (RO) [GPS_Driver_Own_DBG] 
                                     ap host_csr firmware own status (0: driver own, 1:firmware own)
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_OWNER_STATE_SYNC_ADDR CONN_HOST_CSR_TOP_GPS_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_OWNER_STATE_SYNC_MASK 0x00000004                // GPS_AP_HOST_OWNER_STATE_SYNC[2]
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_OWNER_STATE_SYNC_SHFT 2
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_CLR_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_GPS_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_CLR_FW_OWN_HS_PULSE_MASK 0x00000002                // GPS_AP_HOST_CLR_FW_OWN_HS_PULSE[1]
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_CLR_FW_OWN_HS_PULSE_SHFT 1
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_SET_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_GPS_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_SET_FW_OWN_HS_PULSE_MASK 0x00000001                // GPS_AP_HOST_SET_FW_OWN_HS_PULSE[0]
#define CONN_HOST_CSR_TOP_GPS_LPCTL_GPS_AP_HOST_SET_FW_OWN_HS_PULSE_SHFT 0

/* =====================================================================================

  ---GPS_IRQ_STAT (0x18060000 + 0x044)---

    GPS_HOST_LPCR_FW_OWN_CLR_STAT[0] - (W1C) Host AP own interrupt. (cause by GPS)
                                     Write 1 clear interrupt status.
                                     0 : no interrupt
                                     1: host_own interrupt
                                     
                                     When firmware write 1 clear u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_gps_host_lpcr_fw_own [0x1800_1420] bit[0] . Hardware would trigger host side host_own_int. It means firmware transfer ownership to driver.
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_GPS_IRQ_STAT_GPS_HOST_LPCR_FW_OWN_CLR_STAT_ADDR CONN_HOST_CSR_TOP_GPS_IRQ_STAT_ADDR
#define CONN_HOST_CSR_TOP_GPS_IRQ_STAT_GPS_HOST_LPCR_FW_OWN_CLR_STAT_MASK 0x00000001                // GPS_HOST_LPCR_FW_OWN_CLR_STAT[0]
#define CONN_HOST_CSR_TOP_GPS_IRQ_STAT_GPS_HOST_LPCR_FW_OWN_CLR_STAT_SHFT 0

/* =====================================================================================

  ---GPS_IRQ_ENA (0x18060000 + 0x048)---

    GPS_IRQ_ENA[3..0]            - (RW) host AP own interrupt enable(only bit0 used) (cause by GPS)
                                     0 : interrupt disable
                                     1 : interrupt enable
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_GPS_IRQ_ENA_GPS_IRQ_ENA_ADDR         CONN_HOST_CSR_TOP_GPS_IRQ_ENA_ADDR
#define CONN_HOST_CSR_TOP_GPS_IRQ_ENA_GPS_IRQ_ENA_MASK         0x0000000F                // GPS_IRQ_ENA[3..0]
#define CONN_HOST_CSR_TOP_GPS_IRQ_ENA_GPS_IRQ_ENA_SHFT         0

/* =====================================================================================

  ---WF_MD_LPCTL (0x18060000 + 0x050)---

    WF_MD_HOST_SET_FW_OWN_HS_PULSE[0] - (A0) MD set this bit to transfer ownership to FW. 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_md_host_csr_fw_own_sts[0x1800_1414] bit[0] will be set to 1.
    WF_MD_HOST_CLR_FW_OWN_HS_PULSE[1] - (A0) MD set this bit to request ownership back to MD from FW. 
                                     This will introduce an interrupt to FW and u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_md_host_csr_fw_own_sts[0x1800_1414] bit[1] will be set to 1.
    WF_MD_HOST_OWNER_STATE_SYNC[2] - (RO) [MD_Driver_Own_DBG] 
                                     MD host_csr firmware own status (0: driver own, 1:firmware own)
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_OWNER_STATE_SYNC_ADDR CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_OWNER_STATE_SYNC_MASK 0x00000004                // WF_MD_HOST_OWNER_STATE_SYNC[2]
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_OWNER_STATE_SYNC_SHFT 2
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_CLR_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_CLR_FW_OWN_HS_PULSE_MASK 0x00000002                // WF_MD_HOST_CLR_FW_OWN_HS_PULSE[1]
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_CLR_FW_OWN_HS_PULSE_SHFT 1
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_SET_FW_OWN_HS_PULSE_ADDR CONN_HOST_CSR_TOP_WF_MD_LPCTL_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_SET_FW_OWN_HS_PULSE_MASK 0x00000001                // WF_MD_HOST_SET_FW_OWN_HS_PULSE[0]
#define CONN_HOST_CSR_TOP_WF_MD_LPCTL_WF_MD_HOST_SET_FW_OWN_HS_PULSE_SHFT 0

/* =====================================================================================

  ---WF_MD_IRQ_STAT (0x18060000 + 0x054)---

    WF_MD_LPCR_FW_OWN_CLR_STAT[0] - (W1C) Modem own interrupt.
                                     Write 1 clear interrupt status.
                                     0 : no interrupt
                                     1: host_own interrupt
                                     
                                     When firmware write 1 clear u_conn_infra_cfg.u_conn_infra_csr_ctrl.conn_md_host_lpcr_fw_own [0x1800_1418] bit[0] . Hardware would trigger host side host_own_int. It means firmware transfer ownership to driver.
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_STAT_WF_MD_LPCR_FW_OWN_CLR_STAT_ADDR CONN_HOST_CSR_TOP_WF_MD_IRQ_STAT_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_STAT_WF_MD_LPCR_FW_OWN_CLR_STAT_MASK 0x00000001                // WF_MD_LPCR_FW_OWN_CLR_STAT[0]
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_STAT_WF_MD_LPCR_FW_OWN_CLR_STAT_SHFT 0

/* =====================================================================================

  ---WF_MD_IRQ_ENA (0x18060000 + 0x058)---

    MD_IRQ_ENA[3..0]             - (RW) Modem own interrupt enable(only bit0 used)
                                     0 : interrupt disable
                                     1 : interrupt enable
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_ENA_MD_IRQ_ENA_ADDR        CONN_HOST_CSR_TOP_WF_MD_IRQ_ENA_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_ENA_MD_IRQ_ENA_MASK        0x0000000F                // MD_IRQ_ENA[3..0]
#define CONN_HOST_CSR_TOP_WF_MD_IRQ_ENA_MD_IRQ_ENA_SHFT        0

/* =====================================================================================

  ---BT0_MCU_JTAG_CTRL (0x18060000 + 0x080)---

    JTAG_DISABLE[0]              - (RW) BT0 MCU jtag disable control
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BT0_MCU_JTAG_CTRL_JTAG_DISABLE_ADDR  CONN_HOST_CSR_TOP_BT0_MCU_JTAG_CTRL_ADDR
#define CONN_HOST_CSR_TOP_BT0_MCU_JTAG_CTRL_JTAG_DISABLE_MASK  0x00000001                // JTAG_DISABLE[0]
#define CONN_HOST_CSR_TOP_BT0_MCU_JTAG_CTRL_JTAG_DISABLE_SHFT  0

/* =====================================================================================

  ---BT1_MCU_JTAG_CTRL (0x18060000 + 0x084)---

    JTAG_DISABLE[0]              - (RW) BT1 MCU jtag disable control
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BT1_MCU_JTAG_CTRL_JTAG_DISABLE_ADDR  CONN_HOST_CSR_TOP_BT1_MCU_JTAG_CTRL_ADDR
#define CONN_HOST_CSR_TOP_BT1_MCU_JTAG_CTRL_JTAG_DISABLE_MASK  0x00000001                // JTAG_DISABLE[0]
#define CONN_HOST_CSR_TOP_BT1_MCU_JTAG_CTRL_JTAG_DISABLE_SHFT  0

/* =====================================================================================

  ---WF_MCU_JTAG_CTRL (0x18060000 + 0x088)---

    JTAG_DISABLE[0]              - (RW) WF MCU jtag disable control
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MCU_JTAG_CTRL_JTAG_DISABLE_ADDR   CONN_HOST_CSR_TOP_WF_MCU_JTAG_CTRL_ADDR
#define CONN_HOST_CSR_TOP_WF_MCU_JTAG_CTRL_JTAG_DISABLE_MASK   0x00000001                // JTAG_DISABLE[0]
#define CONN_HOST_CSR_TOP_WF_MCU_JTAG_CTRL_JTAG_DISABLE_SHFT   0

/* =====================================================================================

  ---MCU_JTAG_STATUS (0x18060000 + 0x08C)---

    WM_MCU_JTAG_STATUS[0]        - (RO) WM MCU JTAG STATUS
    BT0_MCU_JTAG_STATUS[1]       - (RO) BT0 MCU JTAG STATUS
    BT1_MCU_JTAG_STATUS[2]       - (RO) BT1 MCU JTAG STATUS
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_BT1_MCU_JTAG_STATUS_ADDR CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_ADDR
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_BT1_MCU_JTAG_STATUS_MASK 0x00000004                // BT1_MCU_JTAG_STATUS[2]
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_BT1_MCU_JTAG_STATUS_SHFT 2
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_BT0_MCU_JTAG_STATUS_ADDR CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_ADDR
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_BT0_MCU_JTAG_STATUS_MASK 0x00000002                // BT0_MCU_JTAG_STATUS[1]
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_BT0_MCU_JTAG_STATUS_SHFT 1
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_WM_MCU_JTAG_STATUS_ADDR CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_ADDR
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_WM_MCU_JTAG_STATUS_MASK 0x00000001                // WM_MCU_JTAG_STATUS[0]
#define CONN_HOST_CSR_TOP_MCU_JTAG_STATUS_WM_MCU_JTAG_STATUS_SHFT 0

/* =====================================================================================

  ---CONN_ON_MISC (0x18060000 + 0x0F0)---

    DRV_FW_STAT_SYNC[2..0]       - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_ADDR   CONN_HOST_CSR_TOP_CONN_ON_MISC_ADDR
#define CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_MASK   0x00000007                // DRV_FW_STAT_SYNC[2..0]
#define CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_SHFT   0

/* =====================================================================================

  ---CONN_SYSSTRAP (0x18060000 + 0x0F8)---

    CONN_SYSSTRAP[31..0]         - (RW)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_SYSSTRAP_CONN_SYSSTRAP_ADDR     CONN_HOST_CSR_TOP_CONN_SYSSTRAP_ADDR
#define CONN_HOST_CSR_TOP_CONN_SYSSTRAP_CONN_SYSSTRAP_MASK     0xFFFFFFFF                // CONN_SYSSTRAP[31..0]
#define CONN_HOST_CSR_TOP_CONN_SYSSTRAP_CONN_SYSSTRAP_SHFT     0

/* =====================================================================================

  ---CSR_DEADFEED_EN (0x18060000 + 0x124)---

    CR_AP2CONN_DEADFEED_EN[4..0] - (RW) [0] deadfeed en
                                     [1] osc_rdy en
                                     [2] reserved
                                     [3] reserved
                                     [4] force deadfeed en
                                     0 : disable
                                     1 : enable
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CSR_DEADFEED_EN_CR_AP2CONN_DEADFEED_EN_ADDR CONN_HOST_CSR_TOP_CSR_DEADFEED_EN_ADDR
#define CONN_HOST_CSR_TOP_CSR_DEADFEED_EN_CR_AP2CONN_DEADFEED_EN_MASK 0x0000001F                // CR_AP2CONN_DEADFEED_EN[4..0]
#define CONN_HOST_CSR_TOP_CSR_DEADFEED_EN_CR_AP2CONN_DEADFEED_EN_SHFT 0

/* =====================================================================================

  ---CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL (0x18060000 + 0x15C)---

    CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL[2..0] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_ADDR
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_MASK 0x00000007                // CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL[2..0]
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_SHFT 0

/* =====================================================================================

  ---CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL (0x18060000 + 0x160)---

    CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL[3..0] - (RW)  xxx 
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_ADDR
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_MASK 0x0000000F                // CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL[3..0]
#define CONN_HOST_CSR_TOP_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_CR_CONN_INFRA_RGU_ON_DBG_MUX_SEL_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_TOP (0x18060000 + 0x1A0)---

    CONN_INFRA_WAKEPU_TOP[0]     - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_CONN_INFRA_WAKEPU_TOP_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_CONN_INFRA_WAKEPU_TOP_MASK 0x00000001                // CONN_INFRA_WAKEPU_TOP[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_CONN_INFRA_WAKEPU_TOP_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_WF (0x18060000 + 0x1A4)---

    CONN_INFRA_WAKEPU_WF[0]      - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_CONN_INFRA_WAKEPU_WF_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_CONN_INFRA_WAKEPU_WF_MASK 0x00000001                // CONN_INFRA_WAKEPU_WF[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_CONN_INFRA_WAKEPU_WF_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_BT (0x18060000 + 0x1A8)---

    CONN_INFRA_WAKEPU_BT[0]      - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_BT_CONN_INFRA_WAKEPU_BT_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_BT_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_BT_CONN_INFRA_WAKEPU_BT_MASK 0x00000001                // CONN_INFRA_WAKEPU_BT[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_BT_CONN_INFRA_WAKEPU_BT_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_GPS (0x18060000 + 0x1AC)---

    CONN_INFRA_WAKEPU_GPS[0]     - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS_MASK 0x00000001                // CONN_INFRA_WAKEPU_GPS[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_FM (0x18060000 + 0x1B0)---

    CONN_INFRA_WAKEPU_FM[0]      - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_FM_CONN_INFRA_WAKEPU_FM_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_FM_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_FM_CONN_INFRA_WAKEPU_FM_MASK 0x00000001                // CONN_INFRA_WAKEPU_FM[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_FM_CONN_INFRA_WAKEPU_FM_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_MAWD_0 (0x18060000 + 0x1B4)---

    CONN_INFRA_WAKEPU_MAWD_0[0]  - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_0_CONN_INFRA_WAKEPU_MAWD_0_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_0_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_0_CONN_INFRA_WAKEPU_MAWD_0_MASK 0x00000001                // CONN_INFRA_WAKEPU_MAWD_0[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_0_CONN_INFRA_WAKEPU_MAWD_0_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WAKEPU_MAWD_1 (0x18060000 + 0x1B8)---

    CONN_INFRA_WAKEPU_MAWD_1[0]  - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_1_CONN_INFRA_WAKEPU_MAWD_1_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_1_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_1_CONN_INFRA_WAKEPU_MAWD_1_MASK 0x00000001                // CONN_INFRA_WAKEPU_MAWD_1[0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_MAWD_1_CONN_INFRA_WAKEPU_MAWD_1_SHFT 0

/* =====================================================================================

  ---CONN_SEMA_M2_SW_RST_B (0x18060000 + 0x1F0)---

    CONN_SEMA_M2_SW_RST_B[0]     - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_SEMA_M2_SW_RST_B_CONN_SEMA_M2_SW_RST_B_ADDR CONN_HOST_CSR_TOP_CONN_SEMA_M2_SW_RST_B_ADDR
#define CONN_HOST_CSR_TOP_CONN_SEMA_M2_SW_RST_B_CONN_SEMA_M2_SW_RST_B_MASK 0x00000001                // CONN_SEMA_M2_SW_RST_B[0]
#define CONN_HOST_CSR_TOP_CONN_SEMA_M2_SW_RST_B_CONN_SEMA_M2_SW_RST_B_SHFT 0

/* =====================================================================================

  ---CONN_SEMA_M3_SW_RST_B (0x18060000 + 0x1F4)---

    CONN_SEMA_M3_SW_RST_B[0]     - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_SEMA_M3_SW_RST_B_CONN_SEMA_M3_SW_RST_B_ADDR CONN_HOST_CSR_TOP_CONN_SEMA_M3_SW_RST_B_ADDR
#define CONN_HOST_CSR_TOP_CONN_SEMA_M3_SW_RST_B_CONN_SEMA_M3_SW_RST_B_MASK 0x00000001                // CONN_SEMA_M3_SW_RST_B[0]
#define CONN_HOST_CSR_TOP_CONN_SEMA_M3_SW_RST_B_CONN_SEMA_M3_SW_RST_B_SHFT 0

/* =====================================================================================

  ---CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR (0x18060000 + 0x300)---

    CR_CSR_ON2OFF_TX_SLP_PROT_EN[0] - (RW) force enable sleep protect for conn_infra_on2off tx
    CR_CSR_ON2OFF_TX_SLP_PROT_DIS[1] - (RW) force disable sleep protect for conn_infra_on2off tx
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_ON2OFF_TX_SLP_PROT_DIS_ADDR CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_ON2OFF_TX_SLP_PROT_DIS_MASK 0x00000002                // CR_CSR_ON2OFF_TX_SLP_PROT_DIS[1]
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_ON2OFF_TX_SLP_PROT_DIS_SHFT 1
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_ON2OFF_TX_SLP_PROT_EN_ADDR CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_ON2OFF_TX_SLP_PROT_EN_MASK 0x00000001                // CR_CSR_ON2OFF_TX_SLP_PROT_EN[0]
#define CONN_HOST_CSR_TOP_CONN_ON2OFF_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_ON2OFF_TX_SLP_PROT_EN_SHFT 0

/* =====================================================================================

  ---CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR (0x18060000 + 0x304)---

    CR_CSR_OFF2ON_TX_SLP_PROT_EN[0] - (RW) force enable sleep protect for conn_infra_off2on tx
    CR_CSR_OFF2ON_TX_SLP_PROT_DIS[1] - (RW) force disable sleep protect for conn_infra_off2on tx
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_OFF2ON_TX_SLP_PROT_DIS_ADDR CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_OFF2ON_TX_SLP_PROT_DIS_MASK 0x00000002                // CR_CSR_OFF2ON_TX_SLP_PROT_DIS[1]
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_OFF2ON_TX_SLP_PROT_DIS_SHFT 1
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_OFF2ON_TX_SLP_PROT_EN_ADDR CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_OFF2ON_TX_SLP_PROT_EN_MASK 0x00000001                // CR_CSR_OFF2ON_TX_SLP_PROT_EN[0]
#define CONN_HOST_CSR_TOP_CONN_OFF2ON_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_OFF2ON_TX_SLP_PROT_EN_SHFT 0

/* =====================================================================================

  ---CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR (0x18060000 + 0x314)---

    CR_CSR_CONN2TOP_TX_SLP_PROT_EN[0] - (RW) force enable sleep protect for conn2top tx
    CR_CSR_CONN2TOP_TX_SLP_PROT_DIS[1] - (RW) force disable sleep protect for conn2top tx
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_TX_SLP_PROT_DIS_ADDR CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_TX_SLP_PROT_DIS_MASK 0x00000002                // CR_CSR_CONN2TOP_TX_SLP_PROT_DIS[1]
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_TX_SLP_PROT_DIS_SHFT 1
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_TX_SLP_PROT_EN_ADDR CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_TX_SLP_PROT_EN_MASK 0x00000001                // CR_CSR_CONN2TOP_TX_SLP_PROT_EN[0]
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_TX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_TX_SLP_PROT_EN_SHFT 0

/* =====================================================================================

  ---CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR (0x18060000 + 0x320)---

    CR_CSR_CONN2TOP_RX_SLP_PROT_EN[0] - (RW) force enable sleep protect for conn2top rx
    CR_CSR_CONN2TOP_RX_SLP_PROT_DIS[1] - (RW) force disable sleep protect for conn2top rx
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_RX_SLP_PROT_DIS_ADDR CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_RX_SLP_PROT_DIS_MASK 0x00000002                // CR_CSR_CONN2TOP_RX_SLP_PROT_DIS[1]
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_RX_SLP_PROT_DIS_SHFT 1
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_RX_SLP_PROT_EN_ADDR CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_ADDR
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_RX_SLP_PROT_EN_MASK 0x00000001                // CR_CSR_CONN2TOP_RX_SLP_PROT_EN[0]
#define CONN_HOST_CSR_TOP_CONN_CONN2TOP_RX_SLEEP_PROTECT_CTRL_CSR_CR_CSR_CONN2TOP_RX_SLP_PROT_EN_SHFT 0

/* =====================================================================================

  ---CONN2AP_REMAP_MCU_EMI_BASE_ADDR (0x18060000 + 0x354)---

    CONN2AP_REMAP_MCU_EMI_BASE_ADDR[19..0] - (RW)  xxx 
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_ADDR CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_ADDR
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_MASK 0x000FFFFF                // CONN2AP_REMAP_MCU_EMI_BASE_ADDR[19..0]
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_SHFT 0

/* =====================================================================================

  ---CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR (0x18060000 + 0x35C)---

    CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR[19..0] - (RW)  xxx 
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_ADDR CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_ADDR
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_MASK 0x000FFFFF                // CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR[19..0]
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_SHFT 0

/* =====================================================================================

  ---CONN2AP_REMAP_GPS_EMI_BASE_ADDR (0x18060000 + 0x360)---

    CONN2AP_REMAP_GPS_EMI_BASE_ADDR[19..0] - (RW)  xxx 
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_ADDR CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_ADDR
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_MASK 0x000FFFFF                // CONN2AP_REMAP_GPS_EMI_BASE_ADDR[19..0]
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_SHFT 0

/* =====================================================================================

  ---CONN2AP_REMAP_EMI_LOCK (0x18060000 + 0x364)---

    CONN2AP_REMAP_EMI_LOCK[0]    - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_EMI_LOCK_CONN2AP_REMAP_EMI_LOCK_ADDR CONN_HOST_CSR_TOP_CONN2AP_REMAP_EMI_LOCK_ADDR
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_EMI_LOCK_CONN2AP_REMAP_EMI_LOCK_MASK 0x00000001                // CONN2AP_REMAP_EMI_LOCK[0]
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_EMI_LOCK_CONN2AP_REMAP_EMI_LOCK_SHFT 0

/* =====================================================================================

  ---CONN2AP_REMAP_MODE_SWITCH (0x18060000 + 0x368)---

    CONN2AP_REMAP_MODE_SWITCH[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MODE_SWITCH_CONN2AP_REMAP_MODE_SWITCH_ADDR CONN_HOST_CSR_TOP_CONN2AP_REMAP_MODE_SWITCH_ADDR
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MODE_SWITCH_CONN2AP_REMAP_MODE_SWITCH_MASK 0x00000001                // CONN2AP_REMAP_MODE_SWITCH[0]
#define CONN_HOST_CSR_TOP_CONN2AP_REMAP_MODE_SWITCH_CONN2AP_REMAP_MODE_SWITCH_SHFT 0

/* =====================================================================================

  ---HOST_CONN_INFRA_SLP_CNT_CTL (0x18060000 + 0x380)---

    HOST_SLP_COUNTER_EN[0]       - (RW) Sleep counter enable:
                                     1'h0: Disable
                                     1'h1: Enable
    HOST_SLP_COUNTER_SEL[3..1]   - (RW) Select sleep counter type:
                                     3'h0: conn_infra sleep counter
                                     3'h1: wf_mcu sleep counter
                                     3'h2: bgfsys sleep counter
                                     3'h3: wfsys sleep counter
                                     3'h4: gpssys sleep counter
    HOST_SLP_COUNTER_RD_TRIGGER[4] - (RW) Trigger sleep counter update to CONN_INFRA_SLP_COUNTER/CONN_INFRA_SLP_TIMER(positive edge):
                                     First: Write 1'h0
                                     Then: Write 1'h1
    RESERVED5[30..5]             - (RO) Reserved bits
    HOST_SLP_COUNTER_CTL_EN[31]  - (RW) Sleep counter control enable:
                                     1'h0: Control by conn_infra_cfg
                                     1'h1: Control by conn_host_csr_top

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_CTL_EN_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_CTL_EN_MASK 0x80000000                // HOST_SLP_COUNTER_CTL_EN[31]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_CTL_EN_SHFT 31
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER_MASK 0x00000010                // HOST_SLP_COUNTER_RD_TRIGGER[4]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER_SHFT 4
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL_MASK 0x0000000E                // HOST_SLP_COUNTER_SEL[3..1]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL_SHFT 1
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_EN_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_EN_MASK 0x00000001                // HOST_SLP_COUNTER_EN[0]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_EN_SHFT 0

/* =====================================================================================

  ---HOST_CONN_INFRA_SLP_CNT_SLP_STOP (0x18060000 + 0x384)---

    HOST_CR_GPS_SLEEP_CNT_STOP[0] - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    HOST_CR_WF_SLEEP_CNT_STOP[1] - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    HOST_CR_BT_SLEEP_CNT_STOP[2] - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    RESERVED3[6..3]              - (RO) Reserved bits
    HOST_CR_CONN_INFRA_SLEEP_CNT_STOP[7] - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    HOST_CR_GPS_SLEEP_CNT_CLR[8] - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    HOST_CR_WF_SLEEP_CNT_CLR[9]  - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    HOST_CR_BT_SLEEP_CNT_CLR[10] - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    RESERVED11[14..11]           - (RO) Reserved bits
    HOST_CR_CONN_INFRA_SLEEP_CNT_CLR[15] - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    GPS_IN_SLEEP[16]             - (RO) GPS is in sleeping
    WF_IN_SLEEP[17]              - (RO) WF is in sleeping
    BT_IN_SLEEP[18]              - (RO) BT is in sleeping
    RESERVED19[22..19]           - (RO) Reserved bits
    CONN_INFRA_IN_SLEEP[23]      - (RO) CONN_INFRA is in sleeping
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_CONN_INFRA_IN_SLEEP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_CONN_INFRA_IN_SLEEP_MASK 0x00800000                // CONN_INFRA_IN_SLEEP[23]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_CONN_INFRA_IN_SLEEP_SHFT 23
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_BT_IN_SLEEP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_BT_IN_SLEEP_MASK 0x00040000                // BT_IN_SLEEP[18]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_BT_IN_SLEEP_SHFT 18
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_WF_IN_SLEEP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_WF_IN_SLEEP_MASK 0x00020000                // WF_IN_SLEEP[17]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_WF_IN_SLEEP_SHFT 17
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_GPS_IN_SLEEP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_GPS_IN_SLEEP_MASK 0x00010000                // GPS_IN_SLEEP[16]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_GPS_IN_SLEEP_SHFT 16
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR_MASK 0x00008000                // HOST_CR_CONN_INFRA_SLEEP_CNT_CLR[15]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR_SHFT 15
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR_MASK 0x00000400                // HOST_CR_BT_SLEEP_CNT_CLR[10]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR_SHFT 10
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR_MASK 0x00000200                // HOST_CR_WF_SLEEP_CNT_CLR[9]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR_SHFT 9
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR_MASK 0x00000100                // HOST_CR_GPS_SLEEP_CNT_CLR[8]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR_SHFT 8
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_STOP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_STOP_MASK 0x00000080                // HOST_CR_CONN_INFRA_SLEEP_CNT_STOP[7]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_STOP_SHFT 7
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_STOP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_STOP_MASK 0x00000004                // HOST_CR_BT_SLEEP_CNT_STOP[2]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_STOP_SHFT 2
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_STOP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_STOP_MASK 0x00000002                // HOST_CR_WF_SLEEP_CNT_STOP[1]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_STOP_SHFT 1
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_STOP_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_STOP_MASK 0x00000001                // HOST_CR_GPS_SLEEP_CNT_STOP[0]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_STOP_SHFT 0

/* =====================================================================================

  ---HOST_CONN_INFRA_SLP_TIMER (0x18060000 + 0x388)---

    HOST_SLP_TIMER[31..0]        - (RO) Setected sleep timer result

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_HOST_SLP_TIMER_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_HOST_SLP_TIMER_MASK 0xFFFFFFFF                // HOST_SLP_TIMER[31..0]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_HOST_SLP_TIMER_SHFT 0

/* =====================================================================================

  ---HOST_CONN_INFRA_SLP_COUNTER (0x18060000 + 0x38C)---

    HOST_SLP_COUNTER[31..0]      - (RO) Setected sleep counter result

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_HOST_SLP_COUNTER_ADDR CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_HOST_SLP_COUNTER_MASK 0xFFFFFFFF                // HOST_SLP_COUNTER[31..0]
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_HOST_SLP_COUNTER_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR (0x18060000 + 0x700)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR[31..0] - (RW) Host to conn_host_csr_top (reg) to subsys (WF)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_W_R_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR (0x18060000 + 0x704)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR[31..0] - (W1S) bit set

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_SET_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR (0x18060000 + 0x708)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR[31..0] - (W1C) bit clear

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_WF_CLR_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR (0x18060000 + 0x720)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR[31..0] - (RW) Host to conn_host_csr_top (reg) to subsys (BT)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_W_R_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR (0x18060000 + 0x724)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR[31..0] - (W1S) bit set

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_SET_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR (0x18060000 + 0x728)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR[31..0] - (W1C) bit clear

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_BT_CLR_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR (0x18060000 + 0x740)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR[31..0] - (RW) Host to conn_host_csr_top (reg) to subsys (GPS)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_W_R_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR (0x18060000 + 0x744)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR[31..0] - (W1S) bit set

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_SET_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR (0x18060000 + 0x748)---

    CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR[31..0] - (W1C) bit clear

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_ADDR CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_ADDR
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR[31..0]
#define CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_CONN_HOST_CSR_TOP_HOST_MAILBOX_GPS_CLR_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR (0x18060000 + 0x780)---

    CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR[31..0] - (RO) conn_infra_cfg_on to conn_host_csr_top (bus read)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_ADDR CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_ADDR
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR[31..0]
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_0_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR (0x18060000 + 0x784)---

    CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR[31..0] - (RO) conn_infra_cfg_on to conn_host_csr_top (bus read)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_ADDR CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_ADDR
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR[31..0]
#define CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_CONN_HOST_CSR_TOP_WF_CSR_DUMMY_CR_1_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR (0x18060000 + 0x790)---

    CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR[31..0] - (RO) conn_infra_cfg_on to conn_host_csr_top (bus read)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_ADDR CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_ADDR
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR[31..0]
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_0_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR (0x18060000 + 0x794)---

    CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR[31..0] - (RO) conn_infra_cfg_on to conn_host_csr_top (bus read)

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_ADDR CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_ADDR
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_MASK 0xFFFFFFFF                // CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR[31..0]
#define CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_CONN_HOST_CSR_TOP_BGF_CSR_DUMMY_CR_1_ADDR_SHFT 0

/* =====================================================================================

  ---WF_MD_SRATUS_SYNC_W_R_ADDR (0x18060000 + 0x7B0)---

    WF_MD_SRATUS_SYNC_W_R_ADDR[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_W_R_ADDR_WF_MD_SRATUS_SYNC_W_R_ADDR_ADDR CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_W_R_ADDR_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_W_R_ADDR_WF_MD_SRATUS_SYNC_W_R_ADDR_MASK 0xFFFFFFFF                // WF_MD_SRATUS_SYNC_W_R_ADDR[31..0]
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_W_R_ADDR_WF_MD_SRATUS_SYNC_W_R_ADDR_SHFT 0

/* =====================================================================================

  ---WF_MD_SRATUS_SYNC_SET_ADDR (0x18060000 + 0x7B4)---

    WF_MD_SRATUS_SYNC_SET_ADDR[31..0] - (W1S)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_SET_ADDR_WF_MD_SRATUS_SYNC_SET_ADDR_ADDR CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_SET_ADDR_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_SET_ADDR_WF_MD_SRATUS_SYNC_SET_ADDR_MASK 0xFFFFFFFF                // WF_MD_SRATUS_SYNC_SET_ADDR[31..0]
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_SET_ADDR_WF_MD_SRATUS_SYNC_SET_ADDR_SHFT 0

/* =====================================================================================

  ---WF_MD_SRATUS_SYNC_CLR_ADDR (0x18060000 + 0x7B8)---

    WF_MD_SRATUS_SYNC_CLR_ADDR[31..0] - (W1C)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_CLR_ADDR_WF_MD_SRATUS_SYNC_CLR_ADDR_ADDR CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_CLR_ADDR_ADDR
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_CLR_ADDR_WF_MD_SRATUS_SYNC_CLR_ADDR_MASK 0xFFFFFFFF                // WF_MD_SRATUS_SYNC_CLR_ADDR[31..0]
#define CONN_HOST_CSR_TOP_WF_MD_SRATUS_SYNC_CLR_ADDR_WF_MD_SRATUS_SYNC_CLR_ADDR_SHFT 0

/* =====================================================================================

  ---RSV0_BACKUP_W_R_ADDR (0x18060000 + 0x7C0)---

    RSV0_BACKUP_W_R_ADDR[31..0]  - (RW)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_W_R_ADDR_RSV0_BACKUP_W_R_ADDR_ADDR CONN_HOST_CSR_TOP_RSV0_BACKUP_W_R_ADDR_ADDR
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_W_R_ADDR_RSV0_BACKUP_W_R_ADDR_MASK 0xFFFFFFFF                // RSV0_BACKUP_W_R_ADDR[31..0]
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_W_R_ADDR_RSV0_BACKUP_W_R_ADDR_SHFT 0

/* =====================================================================================

  ---RSV0_BACKUP_SET_ADDR (0x18060000 + 0x7C4)---

    RSV0_BACKUP_SET_ADDR[31..0]  - (W1S)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_SET_ADDR_RSV0_BACKUP_SET_ADDR_ADDR CONN_HOST_CSR_TOP_RSV0_BACKUP_SET_ADDR_ADDR
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_SET_ADDR_RSV0_BACKUP_SET_ADDR_MASK 0xFFFFFFFF                // RSV0_BACKUP_SET_ADDR[31..0]
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_SET_ADDR_RSV0_BACKUP_SET_ADDR_SHFT 0

/* =====================================================================================

  ---RSV0_BACKUP_CLR_ADDR (0x18060000 + 0x7C8)---

    RSV0_BACKUP_CLR_ADDR[31..0]  - (W1C)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_CLR_ADDR_RSV0_BACKUP_CLR_ADDR_ADDR CONN_HOST_CSR_TOP_RSV0_BACKUP_CLR_ADDR_ADDR
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_CLR_ADDR_RSV0_BACKUP_CLR_ADDR_MASK 0xFFFFFFFF                // RSV0_BACKUP_CLR_ADDR[31..0]
#define CONN_HOST_CSR_TOP_RSV0_BACKUP_CLR_ADDR_RSV0_BACKUP_CLR_ADDR_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_SYSSTRAP_OUT (0x18060000 + 0xA00)---

    CONN_INFRA_SYSSTRAP_OUT[31..0] - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT_CONN_INFRA_SYSSTRAP_OUT_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT_CONN_INFRA_SYSSTRAP_OUT_MASK 0xFFFFFFFF                // CONN_INFRA_SYSSTRAP_OUT[31..0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT_CONN_INFRA_SYSSTRAP_OUT_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_ON_DBG (0x18060000 + 0xA04)---

    CONN_INFRA_CFG_ON_DBG[31..0] - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_CONN_INFRA_CFG_ON_DBG_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_CONN_INFRA_CFG_ON_DBG_MASK 0xFFFFFFFF                // CONN_INFRA_CFG_ON_DBG[31..0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_CONN_INFRA_CFG_ON_DBG_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_RGU_ON_DBG (0x18060000 + 0xA08)---

    CONN_INFRA_RGU_ON_DBG[31..0] - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_CONN_INFRA_RGU_ON_DBG_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_CONN_INFRA_RGU_ON_DBG_MASK 0xFFFFFFFF                // CONN_INFRA_RGU_ON_DBG[31..0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_RGU_ON_DBG_CONN_INFRA_RGU_ON_DBG_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CLKGEN_ON_DBG (0x18060000 + 0xA0C)---

    CONN_INFRA_CLKGEN_ON_DBG[15..0] - (RO)  xxx 
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_CLKGEN_ON_DBG_CONN_INFRA_CLKGEN_ON_DBG_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_CLKGEN_ON_DBG_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_CLKGEN_ON_DBG_CONN_INFRA_CLKGEN_ON_DBG_MASK 0x0000FFFF                // CONN_INFRA_CLKGEN_ON_DBG[15..0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_CLKGEN_ON_DBG_CONN_INFRA_CLKGEN_ON_DBG_SHFT 0

/* =====================================================================================

  ---CONNSYS_PWR_STATES (0x18060000 + 0xA10)---

    CONNSYS_PWR_STATES[31..0]    - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_CONNSYS_PWR_STATES_ADDR CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR
#define CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_CONNSYS_PWR_STATES_MASK 0xFFFFFFFF                // CONNSYS_PWR_STATES[31..0]
#define CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_CONNSYS_PWR_STATES_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_ON_MONFLAG (0x18060000 + 0xA14)---

    CONN_INFRA_ON_MONFLAG_OUT[31..0] - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CONN_INFRA_ON_MONFLAG_CONN_INFRA_ON_MONFLAG_OUT_ADDR CONN_HOST_CSR_TOP_CONN_INFRA_ON_MONFLAG_ADDR
#define CONN_HOST_CSR_TOP_CONN_INFRA_ON_MONFLAG_CONN_INFRA_ON_MONFLAG_OUT_MASK 0xFFFFFFFF                // CONN_INFRA_ON_MONFLAG_OUT[31..0]
#define CONN_HOST_CSR_TOP_CONN_INFRA_ON_MONFLAG_CONN_INFRA_ON_MONFLAG_OUT_SHFT 0

/* =====================================================================================

  ---WF_ON_MONFLG_EN_FR_HIF (0x18060000 + 0xB00)---

    WF_ON_MONFLG_EN_FR_HIF[0]    - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_WF_ON_MONFLG_EN_FR_HIF_ADDR CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_ADDR
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_WF_ON_MONFLG_EN_FR_HIF_MASK 0x00000001                // WF_ON_MONFLG_EN_FR_HIF[0]
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_WF_ON_MONFLG_EN_FR_HIF_SHFT 0

/* =====================================================================================

  ---WF_ON_MONFLG_SEL_FR_HIF (0x18060000 + 0xB04)---

    WF_ON_MONFLG_SEL_FR_HIF[4..0] - (RW)  xxx 
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_WF_ON_MONFLG_SEL_FR_HIF_ADDR CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_ADDR
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_WF_ON_MONFLG_SEL_FR_HIF_MASK 0x0000001F                // WF_ON_MONFLG_SEL_FR_HIF[4..0]
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_WF_ON_MONFLG_SEL_FR_HIF_SHFT 0

/* =====================================================================================

  ---WF_OFF_MONFLG_EN_FR_HIF (0x18060000 + 0xB08)---

    WF_OFF_MONFLG_EN_FR_HIF[0]   - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_EN_FR_HIF_WF_OFF_MONFLG_EN_FR_HIF_ADDR CONN_HOST_CSR_TOP_WF_OFF_MONFLG_EN_FR_HIF_ADDR
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_EN_FR_HIF_WF_OFF_MONFLG_EN_FR_HIF_MASK 0x00000001                // WF_OFF_MONFLG_EN_FR_HIF[0]
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_EN_FR_HIF_WF_OFF_MONFLG_EN_FR_HIF_SHFT 0

/* =====================================================================================

  ---WF_OFF_MONFLG_SEL_FR_HIF (0x18060000 + 0xB0C)---

    WF_OFF_MONFLG_SEL_FR_HIF[4..0] - (RW)  xxx 
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_SEL_FR_HIF_WF_OFF_MONFLG_SEL_FR_HIF_ADDR CONN_HOST_CSR_TOP_WF_OFF_MONFLG_SEL_FR_HIF_ADDR
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_SEL_FR_HIF_WF_OFF_MONFLG_SEL_FR_HIF_MASK 0x0000001F                // WF_OFF_MONFLG_SEL_FR_HIF[4..0]
#define CONN_HOST_CSR_TOP_WF_OFF_MONFLG_SEL_FR_HIF_WF_OFF_MONFLG_SEL_FR_HIF_SHFT 0

/* =====================================================================================

  ---WF_ON_MONFLG_OUT (0x18060000 + 0xB10)---

    WF_ON_MONFLG_OUT[31..0]      - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_WF_ON_MONFLG_OUT_ADDR CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_ADDR
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_WF_ON_MONFLG_OUT_MASK 0xFFFFFFFF                // WF_ON_MONFLG_OUT[31..0]
#define CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_WF_ON_MONFLG_OUT_SHFT 0

/* =====================================================================================

  ---WF_MCUSYS_ON_MODULE_SEL_FR_HIF (0x18060000 + 0xB14)---

    WF_MCUSYS_ON_MODULE_SEL_FR_HIF[7..0] - (RW)  xxx 
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_ADDR CONN_HOST_CSR_TOP_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_ADDR
#define CONN_HOST_CSR_TOP_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_MASK 0x000000FF                // WF_MCUSYS_ON_MODULE_SEL_FR_HIF[7..0]
#define CONN_HOST_CSR_TOP_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_WF_MCUSYS_ON_MODULE_SEL_FR_HIF_SHFT 0

/* =====================================================================================

  ---BGF_MONFLG_ON_OUT (0x18060000 + 0xC00)---

    BGF_MONFLG_ON_OUT[31..0]     - (RO)  xxx 

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_BGF_MONFLG_ON_OUT_ADDR CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR
#define CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_BGF_MONFLG_ON_OUT_MASK 0xFFFFFFFF                // BGF_MONFLG_ON_OUT[31..0]
#define CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_BGF_MONFLG_ON_OUT_SHFT 0

/* =====================================================================================

  ---CR_HOSTCSR2BGF_ON_DBG_SEL (0x18060000 + 0xC04)---

    CR_HOSTCSR2BGF_ON_DBG_SEL[21..0] - (RW)  xxx 
    RESERVED22[31..22]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR
#define CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_CR_HOSTCSR2BGF_ON_DBG_SEL_MASK 0x003FFFFF                // CR_HOSTCSR2BGF_ON_DBG_SEL[21..0]
#define CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_CR_HOSTCSR2BGF_ON_DBG_SEL_SHFT 0

#endif // __CONN_HOST_CSR_TOP_REGS_H__
