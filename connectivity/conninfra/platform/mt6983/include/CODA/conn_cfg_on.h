/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_cfg_on.h
//[Revision time]   : Fri Apr  9 11:04:46 2021

#ifndef __CONN_CFG_ON_REGS_H__
#define __CONN_CFG_ON_REGS_H__

//****************************************************************************
//
//                     CONN_CFG_ON CR Definitions                     
//
//****************************************************************************

#define CONN_CFG_ON_BASE                                       (CONN_REG_CONN_INFRA_CFG_ON_ADDR)

#define CONN_CFG_ON_BOOT_ADDR                                  (CONN_CFG_ON_BASE + 0x000) // 1000
#define CONN_CFG_ON_CONN_INFRA_CFG_ON_XTAL_CTL_ADDR            (CONN_CFG_ON_BASE + 0x004) // 1004
#define CONN_CFG_ON_ADIE_CTL_ADDR                              (CONN_CFG_ON_BASE + 0x010) // 1010
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_ADDR                (CONN_CFG_ON_BASE + 0x020) // 1020
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_ADDR         (CONN_CFG_ON_BASE + 0x024) // 1024
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_ADDR              (CONN_CFG_ON_BASE + 0x028) // 1028
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_ADDR             (CONN_CFG_ON_BASE + 0x02C) // 102C
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_ADDR             (CONN_CFG_ON_BASE + 0x030) // 1030
#define CONN_CFG_ON_CONN_INFRA_SLP_TIMER_ADDR                  (CONN_CFG_ON_BASE + 0x034) // 1034
#define CONN_CFG_ON_CONN_INFRA_SLP_COUNTER_ADDR                (CONN_CFG_ON_BASE + 0x038) // 1038
#define CONN_CFG_ON_CONN_INFRA_CFG_CONN2AP_MAILBOX_ADDR        (CONN_CFG_ON_BASE + 0x100) // 1100
#define CONN_CFG_ON_CONN_INFRA_CFG_AP2CONN_MAILBOX_ADDR        (CONN_CFG_ON_BASE + 0x104) // 1104
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR         (CONN_CFG_ON_BASE + 0x108) // 1108
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR        (CONN_CFG_ON_BASE + 0x10C) // 110C
#define CONN_CFG_ON_BGF_DUMMY_CR_0_ADDR                        (CONN_CFG_ON_BASE + 0x110) // 1110
#define CONN_CFG_ON_BGF_DUMMY_CR_1_ADDR                        (CONN_CFG_ON_BASE + 0x114) // 1114
#define CONN_CFG_ON_BGF_DUMMY_CR_2_ADDR                        (CONN_CFG_ON_BASE + 0x118) // 1118
#define CONN_CFG_ON_BGF_DUMMY_CR_3_ADDR                        (CONN_CFG_ON_BASE + 0x11C) // 111C
#define CONN_CFG_ON_WF_DUMMY_CR_0_ADDR                         (CONN_CFG_ON_BASE + 0x120) // 1120
#define CONN_CFG_ON_WF_DUMMY_CR_1_ADDR                         (CONN_CFG_ON_BASE + 0x124) // 1124
#define CONN_CFG_ON_WF_DUMMY_CR_2_ADDR                         (CONN_CFG_ON_BASE + 0x128) // 1128
#define CONN_CFG_ON_WF_DUMMY_CR_3_ADDR                         (CONN_CFG_ON_BASE + 0x12C) // 112C
#define CONN_CFG_ON_CONN_FPGA_DUMMY0_ADDR                      (CONN_CFG_ON_BASE + 0x130) // 1130
#define CONN_CFG_ON_CONN_FPGA_DUMMY1_ADDR                      (CONN_CFG_ON_BASE + 0x134) // 1134
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR               (CONN_CFG_ON_BASE + 0x200) // 1200
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_ADDR            (CONN_CFG_ON_BASE + 0x204) // 1204
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_ADDR            (CONN_CFG_ON_BASE + 0x208) // 1208
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_ADDR           (CONN_CFG_ON_BASE + 0x20C) // 120C
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_ADDR               (CONN_CFG_ON_BASE + 0x210) // 1210
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_ADDR               (CONN_CFG_ON_BASE + 0x214) // 1214
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR              (CONN_CFG_ON_BASE + 0x220) // 1220
#define CONN_CFG_ON_OSC_CTL_0_ADDR                             (CONN_CFG_ON_BASE + 0x300) // 1300
#define CONN_CFG_ON_OSC_CTL_1_ADDR                             (CONN_CFG_ON_BASE + 0x304) // 1304
#define CONN_CFG_ON_OSC_MASK_ADDR                              (CONN_CFG_ON_BASE + 0x308) // 1308
#define CONN_CFG_ON_OSC_STATUS_ADDR                            (CONN_CFG_ON_BASE + 0x30C) // 130C
#define CONN_CFG_ON_OSC_2X_CTL_0_ADDR                          (CONN_CFG_ON_BASE + 0x310) // 1310
#define CONN_CFG_ON_OSC_2X_CTL_1_ADDR                          (CONN_CFG_ON_BASE + 0x314) // 1314
#define CONN_CFG_ON_OSC_2X_MASK_ADDR                           (CONN_CFG_ON_BASE + 0x318) // 1318
#define CONN_CFG_ON_OSC_2X_STATUS_ADDR                         (CONN_CFG_ON_BASE + 0x31C) // 131C
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR              (CONN_CFG_ON_BASE + 0x320) // 1320
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_ADDR              (CONN_CFG_ON_BASE + 0x330) // 1330
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR               (CONN_CFG_ON_BASE + 0x340) // 1340
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR              (CONN_CFG_ON_BASE + 0x344) // 1344
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR               (CONN_CFG_ON_BASE + 0x348) // 1348
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR           (CONN_CFG_ON_BASE + 0x350) // 1350
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR           (CONN_CFG_ON_BASE + 0x354) // 1354
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR            (CONN_CFG_ON_BASE + 0x360) // 1360
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR            (CONN_CFG_ON_BASE + 0x364) // 1364
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR            (CONN_CFG_ON_BASE + 0x370) // 1370
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR            (CONN_CFG_ON_BASE + 0x374) // 1374
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR           (CONN_CFG_ON_BASE + 0x380) // 1380
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR           (CONN_CFG_ON_BASE + 0x384) // 1384
#define CONN_CFG_ON_EMI_CTL_0_ADDR                             (CONN_CFG_ON_BASE + 0x3A0) // 13A0
#define CONN_CFG_ON_RF_LDO_CTRL_0_ADDR                         (CONN_CFG_ON_BASE + 0x3B0) // 13B0
#define CONN_CFG_ON_RF_LDO_CTRL_1_ADDR                         (CONN_CFG_ON_BASE + 0x3B4) // 13B4
#define CONN_CFG_ON_RF_LDO_STATUS_ADDR                         (CONN_CFG_ON_BASE + 0x3B8) // 13B8
#define CONN_CFG_ON_RF_LDO_TIMER_0_ADDR                        (CONN_CFG_ON_BASE + 0x3C0) // 13C0
#define CONN_CFG_ON_RF_LDO_TIMER_1_ADDR                        (CONN_CFG_ON_BASE + 0x3C4) // 13C4
#define CONN_CFG_ON_RF_LDO_TIMER_2_ADDR                        (CONN_CFG_ON_BASE + 0x3C8) // 13C8
#define CONN_CFG_ON_RF_LDO_TIMER_3_ADDR                        (CONN_CFG_ON_BASE + 0x3CC) // 13CC
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR           (CONN_CFG_ON_BASE + 0x400) // 1400
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR         (CONN_CFG_ON_BASE + 0x404) // 1404
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_ADDR     (CONN_CFG_ON_BASE + 0x408) // 1408
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR           (CONN_CFG_ON_BASE + 0x410) // 1410
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_ADDR         (CONN_CFG_ON_BASE + 0x414) // 1414
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_ADDR            (CONN_CFG_ON_BASE + 0x420) // 1420
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_STATUS_ADDR          (CONN_CFG_ON_BASE + 0x424) // 1424
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_ADDR           (CONN_CFG_ON_BASE + 0x430) // 1430
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_STATUS_ADDR         (CONN_CFG_ON_BASE + 0x434) // 1434
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_ADDR                (CONN_CFG_ON_BASE + 0x440) // 1440
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR              (CONN_CFG_ON_BASE + 0x444) // 1444
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR           (CONN_CFG_ON_BASE + 0x450) // 1450
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR               (CONN_CFG_ON_BASE + 0x454) // 1454
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR           (CONN_CFG_ON_BASE + 0x460) // 1460
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR               (CONN_CFG_ON_BASE + 0x464) // 1464
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR          (CONN_CFG_ON_BASE + 0x470) // 1470
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR              (CONN_CFG_ON_BASE + 0x474) // 1474
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR          (CONN_CFG_ON_BASE + 0x480) // 1480
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR              (CONN_CFG_ON_BASE + 0x484) // 1484
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR                       (CONN_CFG_ON_BASE + 0x600) // 1600
#define CONN_CFG_ON_CSR_BGF_ON_FW_OWN_IRQ_ADDR                 (CONN_CFG_ON_BASE + 0x604) // 1604
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_ADDR                  (CONN_CFG_ON_BASE + 0x610) // 1610
#define CONN_CFG_ON_CSR_MD_ON_HOST_CSR_MISC_ADDR               (CONN_CFG_ON_BASE + 0x614) // 1614
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_ADDR               (CONN_CFG_ON_BASE + 0x620) // 1620
#define CONN_CFG_ON_CSR_WF_B0_ON_HOST_CSR_MISC_ADDR            (CONN_CFG_ON_BASE + 0x624) // 1624
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_ADDR               (CONN_CFG_ON_BASE + 0x630) // 1630
#define CONN_CFG_ON_CSR_WF_B1_ON_HOST_CSR_MISC_ADDR            (CONN_CFG_ON_BASE + 0x634) // 1634
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_ADDR                 (CONN_CFG_ON_BASE + 0x640) // 1640
#define CONN_CFG_ON_CSR_BGF_ON_HOST_CSR_MISC_ADDR              (CONN_CFG_ON_BASE + 0x644) // 1644
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_ADDR                 (CONN_CFG_ON_BASE + 0x650) // 1650
#define CONN_CFG_ON_CSR_GPS_ON_HOST_CSR_MISC_ADDR              (CONN_CFG_ON_BASE + 0x654) // 1654




/* =====================================================================================

  ---BOOT (0x18001000 + 0x000)---

    AP2CONN_BOOT_CPU_SEL[0]      - (RO) connsys boot by wfsys or bgfsys cpu selection
                                     1'h0: connsys is booted by wfsys cpu
                                     1'h1: connsys is booted by bgfsys cpu
    RESERVED1[29..1]             - (RO) Reserved bits
    BGF_CPU_BOOT_DONE[30]        - (RW) bgfsys cpu boot done, this bit should be set to 1 by bgfsys cpu when connsys boot is completed by bgfsys cpu
                                     1'h0: not done
                                     1'h1: done
    WF_CPU_BOOT_DONE[31]         - (RW) wfsys cpu boot done, this bit should be set to 1 by wfsys cpu when connsys boot is completed by wfsys cpu
                                     1'h0: not done
                                     1'h1: done

 =====================================================================================*/
#define CONN_CFG_ON_BOOT_WF_CPU_BOOT_DONE_ADDR                 CONN_CFG_ON_BOOT_ADDR
#define CONN_CFG_ON_BOOT_WF_CPU_BOOT_DONE_MASK                 0x80000000                // WF_CPU_BOOT_DONE[31]
#define CONN_CFG_ON_BOOT_WF_CPU_BOOT_DONE_SHFT                 31
#define CONN_CFG_ON_BOOT_BGF_CPU_BOOT_DONE_ADDR                CONN_CFG_ON_BOOT_ADDR
#define CONN_CFG_ON_BOOT_BGF_CPU_BOOT_DONE_MASK                0x40000000                // BGF_CPU_BOOT_DONE[30]
#define CONN_CFG_ON_BOOT_BGF_CPU_BOOT_DONE_SHFT                30
#define CONN_CFG_ON_BOOT_AP2CONN_BOOT_CPU_SEL_ADDR             CONN_CFG_ON_BOOT_ADDR
#define CONN_CFG_ON_BOOT_AP2CONN_BOOT_CPU_SEL_MASK             0x00000001                // AP2CONN_BOOT_CPU_SEL[0]
#define CONN_CFG_ON_BOOT_AP2CONN_BOOT_CPU_SEL_SHFT             0

/* =====================================================================================

  ---CONN_INFRA_CFG_ON_XTAL_CTL (0x18001000 + 0x004)---

    CONN_INFRA_CFG_ON_XTAL_CTL[0] - (RW) 1'b0: connsys digital xtal = XTAL
                                     1'b1: connsys digital xtal = XTAL/2
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_ON_XTAL_CTL_CONN_INFRA_CFG_ON_XTAL_CTL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_ON_XTAL_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_ON_XTAL_CTL_CONN_INFRA_CFG_ON_XTAL_CTL_MASK 0x00000001                // CONN_INFRA_CFG_ON_XTAL_CTL[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_ON_XTAL_CTL_CONN_INFRA_CFG_ON_XTAL_CTL_SHFT 0

/* =====================================================================================

  ---ADIE_CTL (0x18001000 + 0x010)---

    ADIE_RSTB[0]                 - (RW) a-die reset (active-low)
                                     1'h0: reset
                                     1'h1: not reset
    ADIE_TOP_CKEN[1]             - (RW) a-die top clock enable
                                     1'h0: disable
                                     1'h1: enable
    INST2_ADIE_RSTB[2]           - (RW) a-die reset (active-low)
                                     1'h0: reset
                                     1'h1: not reset
    INST2_ADIE_TOP_CKEN[3]       - (RW) a-die top clock enable
                                     1'h0: disable
                                     1'h1: enable
    INST2_ADIE_BN_SEL[4]         - (RW)  xxx 
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_BN_SEL_ADDR            CONN_CFG_ON_ADIE_CTL_ADDR
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_BN_SEL_MASK            0x00000010                // INST2_ADIE_BN_SEL[4]
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_BN_SEL_SHFT            4
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_TOP_CKEN_ADDR          CONN_CFG_ON_ADIE_CTL_ADDR
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_TOP_CKEN_MASK          0x00000008                // INST2_ADIE_TOP_CKEN[3]
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_TOP_CKEN_SHFT          3
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_RSTB_ADDR              CONN_CFG_ON_ADIE_CTL_ADDR
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_RSTB_MASK              0x00000004                // INST2_ADIE_RSTB[2]
#define CONN_CFG_ON_ADIE_CTL_INST2_ADIE_RSTB_SHFT              2
#define CONN_CFG_ON_ADIE_CTL_ADIE_TOP_CKEN_ADDR                CONN_CFG_ON_ADIE_CTL_ADDR
#define CONN_CFG_ON_ADIE_CTL_ADIE_TOP_CKEN_MASK                0x00000002                // ADIE_TOP_CKEN[1]
#define CONN_CFG_ON_ADIE_CTL_ADIE_TOP_CKEN_SHFT                1
#define CONN_CFG_ON_ADIE_CTL_ADIE_RSTB_ADDR                    CONN_CFG_ON_ADIE_CTL_ADDR
#define CONN_CFG_ON_ADIE_CTL_ADIE_RSTB_MASK                    0x00000001                // ADIE_RSTB[0]
#define CONN_CFG_ON_ADIE_CTL_ADIE_RSTB_SHFT                    0

/* =====================================================================================

  ---CONN_INFRA_SLP_CNT_CTL (0x18001000 + 0x020)---

    SLP_COUNTER_EN[0]            - (RW) Sleep counter enable:
                                     1'h0: Disable
                                     1'h1: Enable
    SLP_COUNTER_SEL[3..1]        - (RW) Select sleep counter type:
                                     3'h0: conn_infra sleep counter
                                     3'h1: wfsys sleep counter
                                     3'h2: bgfsys sleep counter
                                     3'h3: gpssys sleep counter
    SLP_COUNTER_RD_TRIGGER[4]    - (RW) Trigger sleep counter update to CONN_INFRA_SLP_COUNTER/CONN_INFRA_SLP_TIMER(positive edge):
                                     First: Write 1'h0
                                     Then: Write 1'h1
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_RD_TRIGGER_ADDR CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_RD_TRIGGER_MASK 0x00000010                // SLP_COUNTER_RD_TRIGGER[4]
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_RD_TRIGGER_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_SEL_ADDR CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_SEL_MASK 0x0000000E                // SLP_COUNTER_SEL[3..1]
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_SEL_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_EN_ADDR CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_EN_MASK 0x00000001                // SLP_COUNTER_EN[0]
#define CONN_CFG_ON_CONN_INFRA_SLP_CNT_CTL_SLP_COUNTER_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CONN_INFRA_SLP_CNT (0x18001000 + 0x024)---

    CONN_INFRA_SLP_COUNTER_STOP[0] - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    CONN_INFRA_SLP_COUNTER_CLR[1] - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    CONN_INFRA_IN_SLEEP[2]       - (RO) CONN_INFRA is in sleeping
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_IN_SLEEP_ADDR CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_IN_SLEEP_MASK 0x00000004                // CONN_INFRA_IN_SLEEP[2]
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_IN_SLEEP_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_SLP_COUNTER_CLR_ADDR CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_SLP_COUNTER_CLR_MASK 0x00000002                // CONN_INFRA_SLP_COUNTER_CLR[1]
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_SLP_COUNTER_CLR_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_SLP_COUNTER_STOP_ADDR CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_SLP_COUNTER_STOP_MASK 0x00000001                // CONN_INFRA_SLP_COUNTER_STOP[0]
#define CONN_CFG_ON_CONN_INFRA_CONN_INFRA_SLP_CNT_CONN_INFRA_SLP_COUNTER_STOP_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WFSYS_SLP_CNT (0x18001000 + 0x028)---

    WFSYS_SLP_COUNTER_STOP[0]    - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    WFSYS_SLP_COUNTER_CLR[1]     - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    WFSYS_IN_SLEEP[2]            - (RO) WFSYS is in sleeping
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_IN_SLEEP_ADDR CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_IN_SLEEP_MASK 0x00000004                // WFSYS_IN_SLEEP[2]
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_IN_SLEEP_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_SLP_COUNTER_CLR_ADDR CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_SLP_COUNTER_CLR_MASK 0x00000002                // WFSYS_SLP_COUNTER_CLR[1]
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_SLP_COUNTER_CLR_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_SLP_COUNTER_STOP_ADDR CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_SLP_COUNTER_STOP_MASK 0x00000001                // WFSYS_SLP_COUNTER_STOP[0]
#define CONN_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT_WFSYS_SLP_COUNTER_STOP_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_BGFSYS_SLP_CNT (0x18001000 + 0x02C)---

    BGFSYS_SLP_COUNTER_STOP[0]   - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    BGFSYS_SLP_COUNTER_CLR[1]    - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    BGFSYS_IN_SLEEP[2]           - (RO) BGFSYS is in sleeping
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_IN_SLEEP_ADDR CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_IN_SLEEP_MASK 0x00000004                // BGFSYS_IN_SLEEP[2]
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_IN_SLEEP_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_SLP_COUNTER_CLR_ADDR CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_SLP_COUNTER_CLR_MASK 0x00000002                // BGFSYS_SLP_COUNTER_CLR[1]
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_SLP_COUNTER_CLR_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_SLP_COUNTER_STOP_ADDR CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_SLP_COUNTER_STOP_MASK 0x00000001                // BGFSYS_SLP_COUNTER_STOP[0]
#define CONN_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT_BGFSYS_SLP_COUNTER_STOP_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_GPSSYS_SLP_CNT (0x18001000 + 0x030)---

    GPSSYS_SLP_COUNTER_STOP[0]   - (RW) Sleep counter stop:
                                     1'h1: Stop
                                     1'h0: Keep going
    GPSSYS_SLP_COUNTER_CLR[1]    - (RW) Sleep counter clear:
                                     1'h1: Clean to 0
                                     1'h0: Keep going
    GPSSYS_IN_SLEEP[2]           - (RO) GPSSYS is in sleeping
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_IN_SLEEP_ADDR CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_IN_SLEEP_MASK 0x00000004                // GPSSYS_IN_SLEEP[2]
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_IN_SLEEP_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_SLP_COUNTER_CLR_ADDR CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_SLP_COUNTER_CLR_MASK 0x00000002                // GPSSYS_SLP_COUNTER_CLR[1]
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_SLP_COUNTER_CLR_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_SLP_COUNTER_STOP_ADDR CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_SLP_COUNTER_STOP_MASK 0x00000001                // GPSSYS_SLP_COUNTER_STOP[0]
#define CONN_CFG_ON_CONN_INFRA_GPSSYS_SLP_CNT_GPSSYS_SLP_COUNTER_STOP_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_SLP_TIMER (0x18001000 + 0x034)---

    SLP_TIMER[31..0]             - (RO) Setected sleep timer result

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_SLP_TIMER_SLP_TIMER_ADDR        CONN_CFG_ON_CONN_INFRA_SLP_TIMER_ADDR
#define CONN_CFG_ON_CONN_INFRA_SLP_TIMER_SLP_TIMER_MASK        0xFFFFFFFF                // SLP_TIMER[31..0]
#define CONN_CFG_ON_CONN_INFRA_SLP_TIMER_SLP_TIMER_SHFT        0

/* =====================================================================================

  ---CONN_INFRA_SLP_COUNTER (0x18001000 + 0x038)---

    SLP_COUNTER[31..0]           - (RO) Setected sleep counter result

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_SLP_COUNTER_SLP_COUNTER_ADDR    CONN_CFG_ON_CONN_INFRA_SLP_COUNTER_ADDR
#define CONN_CFG_ON_CONN_INFRA_SLP_COUNTER_SLP_COUNTER_MASK    0xFFFFFFFF                // SLP_COUNTER[31..0]
#define CONN_CFG_ON_CONN_INFRA_SLP_COUNTER_SLP_COUNTER_SHFT    0

/* =====================================================================================

  ---CONN_INFRA_CFG_CONN2AP_MAILBOX (0x18001000 + 0x100)---

    CONN2AP_MAILBOX[31..0]       - (RW) conn2ap mailbox

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_CONN2AP_MAILBOX_CONN2AP_MAILBOX_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CONN2AP_MAILBOX_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CONN2AP_MAILBOX_CONN2AP_MAILBOX_MASK 0xFFFFFFFF                // CONN2AP_MAILBOX[31..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_CONN2AP_MAILBOX_CONN2AP_MAILBOX_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_AP2CONN_MAILBOX (0x18001000 + 0x104)---

    AP2CONN_MAILBOX[31..0]       - (RO) ap2conn mailbox

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_AP2CONN_MAILBOX_AP2CONN_MAILBOX_ADDR CONN_CFG_ON_CONN_INFRA_CFG_AP2CONN_MAILBOX_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_AP2CONN_MAILBOX_AP2CONN_MAILBOX_MASK 0xFFFFFFFF                // AP2CONN_MAILBOX[31..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_AP2CONN_MAILBOX_AP2CONN_MAILBOX_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_BT_MANUAL_CTRL (0x18001000 + 0x108)---

    BT_MANUAL_CTRL_RESERVED[25..0] - (RW) btsys manual control cr(reserved)
    CR_FORCE_XTAL2X_CK_RDY_HOST[26] - (RW) 1'b1: force xtal2x_ck_rdy =1
    CINF_CR_BT_MTCMOS_PWR_ACK_MASK[27] - (RW) mask off domain  mtcmos ~pwr_ack  & ~pwr_ack_s:
                                     1'b1: legacy sleep need setting
    CR_FORCE_OSC_EN[28]          - (RW) 1'b1: force osc_en_all =1
    HOST2BT_SELECT_TOP_MANUAL_CONTROL[29] - (RW) select gps or bt control bgfsys manual pwr control module
                                     1'b1: gps control
    HOST2BT_SELECT_MONFLG_ON_CONTROL[30] - (RW) select gps or bt control bgf_monflg_on mux
                                     1'b1: gps control
    CR_FORCE_BT_OSC_RDY[31]      - (RW) 1'b1: force bt_osc_rdy =1

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_BT_OSC_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_BT_OSC_RDY_MASK 0x80000000                // CR_FORCE_BT_OSC_RDY[31]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_BT_OSC_RDY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_HOST2BT_SELECT_MONFLG_ON_CONTROL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_HOST2BT_SELECT_MONFLG_ON_CONTROL_MASK 0x40000000                // HOST2BT_SELECT_MONFLG_ON_CONTROL[30]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_HOST2BT_SELECT_MONFLG_ON_CONTROL_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_HOST2BT_SELECT_TOP_MANUAL_CONTROL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_HOST2BT_SELECT_TOP_MANUAL_CONTROL_MASK 0x20000000                // HOST2BT_SELECT_TOP_MANUAL_CONTROL[29]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_HOST2BT_SELECT_TOP_MANUAL_CONTROL_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_OSC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_OSC_EN_MASK 0x10000000                // CR_FORCE_OSC_EN[28]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_OSC_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CINF_CR_BT_MTCMOS_PWR_ACK_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CINF_CR_BT_MTCMOS_PWR_ACK_MASK_MASK 0x08000000                // CINF_CR_BT_MTCMOS_PWR_ACK_MASK[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CINF_CR_BT_MTCMOS_PWR_ACK_MASK_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_XTAL2X_CK_RDY_HOST_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_XTAL2X_CK_RDY_HOST_MASK 0x04000000                // CR_FORCE_XTAL2X_CK_RDY_HOST[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_CR_FORCE_XTAL2X_CK_RDY_HOST_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_BT_MANUAL_CTRL_RESERVED_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_BT_MANUAL_CTRL_RESERVED_MASK 0x03FFFFFF                // BT_MANUAL_CTRL_RESERVED[25..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_MANUAL_CTRL_BT_MANUAL_CTRL_RESERVED_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_GPS_MANUAL_CTRL (0x18001000 + 0x10C)---

    FORCE_HOST_CTL[15..0]        - (RW) gps_pwr_ctl in gpssys
    GPS_MANUAL_CTRL[19..16]      - (RW) gpssys manual control cr(reserved)
    HOST2GPS_SELECT_TOP_MANUAL_CONTROL[20] - (RW) select gps or bt control bgfsys manual pwr control module
                                     1'b1: gps control
    HOST2GPS_SELECT_MONFLG_ON_CONTROL[21] - (RW) select gps or bt control bgf_monflg_on mux
                                     1'b1: gps control
    CINF_CR_GPS_MTCMOS_PWR_ACK_MASK[22] - (RW) mask off domain mtcmos ~pwr_ack  & ~pwr_ack_s:
                                     1'b1: legacy sleep need setting
    FORCE_GPS_BUS_CLK_EN[23]     - (RW) 1'b1: enable gps bus clk
    SW_GPS_HW_CONTROL_CLR[24]    - (RW) 1'b1: force clear gps_hw_control
    SW_GPS_OSC_EN_ALL[25]        - (RW) 1'b1: force keep gps_osc_en_all
    SW_GPS_TOP_OFF_PWR_CTL[26]   - (RW) 1'b1:force gps top off pwr ctl fsm jump from state SLEEP to state PWR3
    SW_GPS_OSC_STL[27]           - (RW) 1'b1:force gps osc ctl fsm jump from state ST3 to state ST4
    SW_GPS_L5_SLP_CTL[28]        - (RW) 1'b1:force gps l5 slp_ctl fsm jump from state SLEEP to state PON1
    SW_GPS_L1_SLP_CTL[29]        - (RW) 1'b1:force gps l1 slp_ctl fsm jump from state SLEEP to state PON1
    SW_GPS_OSC_ON[30]            - (RW) 1'b1:force gps_osc_on = 1, require osc clock
    SW_GPS_OSC_RDY[31]           - (RW) 1'b1:force gps_osc_rdy = 1

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_RDY_MASK 0x80000000                // SW_GPS_OSC_RDY[31]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_RDY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_ON_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_ON_MASK 0x40000000                // SW_GPS_OSC_ON[30]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_ON_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_L1_SLP_CTL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_L1_SLP_CTL_MASK 0x20000000                // SW_GPS_L1_SLP_CTL[29]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_L1_SLP_CTL_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_L5_SLP_CTL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_L5_SLP_CTL_MASK 0x10000000                // SW_GPS_L5_SLP_CTL[28]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_L5_SLP_CTL_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_STL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_STL_MASK 0x08000000                // SW_GPS_OSC_STL[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_STL_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_TOP_OFF_PWR_CTL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_TOP_OFF_PWR_CTL_MASK 0x04000000                // SW_GPS_TOP_OFF_PWR_CTL[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_TOP_OFF_PWR_CTL_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_EN_ALL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_EN_ALL_MASK 0x02000000                // SW_GPS_OSC_EN_ALL[25]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_OSC_EN_ALL_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_HW_CONTROL_CLR_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_HW_CONTROL_CLR_MASK 0x01000000                // SW_GPS_HW_CONTROL_CLR[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_SW_GPS_HW_CONTROL_CLR_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_FORCE_GPS_BUS_CLK_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_FORCE_GPS_BUS_CLK_EN_MASK 0x00800000                // FORCE_GPS_BUS_CLK_EN[23]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_FORCE_GPS_BUS_CLK_EN_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_CINF_CR_GPS_MTCMOS_PWR_ACK_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_CINF_CR_GPS_MTCMOS_PWR_ACK_MASK_MASK 0x00400000                // CINF_CR_GPS_MTCMOS_PWR_ACK_MASK[22]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_CINF_CR_GPS_MTCMOS_PWR_ACK_MASK_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_HOST2GPS_SELECT_MONFLG_ON_CONTROL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_HOST2GPS_SELECT_MONFLG_ON_CONTROL_MASK 0x00200000                // HOST2GPS_SELECT_MONFLG_ON_CONTROL[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_HOST2GPS_SELECT_MONFLG_ON_CONTROL_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_HOST2GPS_SELECT_TOP_MANUAL_CONTROL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_HOST2GPS_SELECT_TOP_MANUAL_CONTROL_MASK 0x00100000                // HOST2GPS_SELECT_TOP_MANUAL_CONTROL[20]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_HOST2GPS_SELECT_TOP_MANUAL_CONTROL_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_GPS_MANUAL_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_GPS_MANUAL_CTRL_MASK 0x000F0000                // GPS_MANUAL_CTRL[19..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_GPS_MANUAL_CTRL_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_FORCE_HOST_CTL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_FORCE_HOST_CTL_MASK 0x0000FFFF                // FORCE_HOST_CTL[15..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_FORCE_HOST_CTL_SHFT 0

/* =====================================================================================

  ---BGF_DUMMY_CR_0 (0x18001000 + 0x110)---

    BGF_DUMMY_0[31..0]           - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_BGF_DUMMY_CR_0_BGF_DUMMY_0_ADDR            CONN_CFG_ON_BGF_DUMMY_CR_0_ADDR
#define CONN_CFG_ON_BGF_DUMMY_CR_0_BGF_DUMMY_0_MASK            0xFFFFFFFF                // BGF_DUMMY_0[31..0]
#define CONN_CFG_ON_BGF_DUMMY_CR_0_BGF_DUMMY_0_SHFT            0

/* =====================================================================================

  ---BGF_DUMMY_CR_1 (0x18001000 + 0x114)---

    BGF_DUMMY_1[31..0]           - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_BGF_DUMMY_CR_1_BGF_DUMMY_1_ADDR            CONN_CFG_ON_BGF_DUMMY_CR_1_ADDR
#define CONN_CFG_ON_BGF_DUMMY_CR_1_BGF_DUMMY_1_MASK            0xFFFFFFFF                // BGF_DUMMY_1[31..0]
#define CONN_CFG_ON_BGF_DUMMY_CR_1_BGF_DUMMY_1_SHFT            0

/* =====================================================================================

  ---BGF_DUMMY_CR_2 (0x18001000 + 0x118)---

    BGF_DUMMY_2[31..0]           - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_BGF_DUMMY_CR_2_BGF_DUMMY_2_ADDR            CONN_CFG_ON_BGF_DUMMY_CR_2_ADDR
#define CONN_CFG_ON_BGF_DUMMY_CR_2_BGF_DUMMY_2_MASK            0xFFFFFFFF                // BGF_DUMMY_2[31..0]
#define CONN_CFG_ON_BGF_DUMMY_CR_2_BGF_DUMMY_2_SHFT            0

/* =====================================================================================

  ---BGF_DUMMY_CR_3 (0x18001000 + 0x11C)---

    BGF_DUMMY_3[31..0]           - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_BGF_DUMMY_CR_3_BGF_DUMMY_3_ADDR            CONN_CFG_ON_BGF_DUMMY_CR_3_ADDR
#define CONN_CFG_ON_BGF_DUMMY_CR_3_BGF_DUMMY_3_MASK            0xFFFFFFFF                // BGF_DUMMY_3[31..0]
#define CONN_CFG_ON_BGF_DUMMY_CR_3_BGF_DUMMY_3_SHFT            0

/* =====================================================================================

  ---WF_DUMMY_CR_0 (0x18001000 + 0x120)---

    WF_DUMMY_0[31..0]            - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_WF_DUMMY_CR_0_WF_DUMMY_0_ADDR              CONN_CFG_ON_WF_DUMMY_CR_0_ADDR
#define CONN_CFG_ON_WF_DUMMY_CR_0_WF_DUMMY_0_MASK              0xFFFFFFFF                // WF_DUMMY_0[31..0]
#define CONN_CFG_ON_WF_DUMMY_CR_0_WF_DUMMY_0_SHFT              0

/* =====================================================================================

  ---WF_DUMMY_CR_1 (0x18001000 + 0x124)---

    WF_DUMMY_1[31..0]            - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_WF_DUMMY_CR_1_WF_DUMMY_1_ADDR              CONN_CFG_ON_WF_DUMMY_CR_1_ADDR
#define CONN_CFG_ON_WF_DUMMY_CR_1_WF_DUMMY_1_MASK              0xFFFFFFFF                // WF_DUMMY_1[31..0]
#define CONN_CFG_ON_WF_DUMMY_CR_1_WF_DUMMY_1_SHFT              0

/* =====================================================================================

  ---WF_DUMMY_CR_2 (0x18001000 + 0x128)---

    WF_DUMMY_2[31..0]            - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_WF_DUMMY_CR_2_WF_DUMMY_2_ADDR              CONN_CFG_ON_WF_DUMMY_CR_2_ADDR
#define CONN_CFG_ON_WF_DUMMY_CR_2_WF_DUMMY_2_MASK              0xFFFFFFFF                // WF_DUMMY_2[31..0]
#define CONN_CFG_ON_WF_DUMMY_CR_2_WF_DUMMY_2_SHFT              0

/* =====================================================================================

  ---WF_DUMMY_CR_3 (0x18001000 + 0x12C)---

    WF_DUMMY_3[31..0]            - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_WF_DUMMY_CR_3_WF_DUMMY_3_ADDR              CONN_CFG_ON_WF_DUMMY_CR_3_ADDR
#define CONN_CFG_ON_WF_DUMMY_CR_3_WF_DUMMY_3_MASK              0xFFFFFFFF                // WF_DUMMY_3[31..0]
#define CONN_CFG_ON_WF_DUMMY_CR_3_WF_DUMMY_3_SHFT              0

/* =====================================================================================

  ---CONN_FPGA_DUMMY0 (0x18001000 + 0x130)---

    R_CONN_FPGA_DUMMY0[31..0]    - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_FPGA_DUMMY0_R_CONN_FPGA_DUMMY0_ADDR   CONN_CFG_ON_CONN_FPGA_DUMMY0_ADDR
#define CONN_CFG_ON_CONN_FPGA_DUMMY0_R_CONN_FPGA_DUMMY0_MASK   0xFFFFFFFF                // R_CONN_FPGA_DUMMY0[31..0]
#define CONN_CFG_ON_CONN_FPGA_DUMMY0_R_CONN_FPGA_DUMMY0_SHFT   0

/* =====================================================================================

  ---CONN_FPGA_DUMMY1 (0x18001000 + 0x134)---

    R_CONN_FPGA_DUMMY1[31..0]    - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_FPGA_DUMMY1_R_CONN_FPGA_DUMMY1_ADDR   CONN_CFG_ON_CONN_FPGA_DUMMY1_ADDR
#define CONN_CFG_ON_CONN_FPGA_DUMMY1_R_CONN_FPGA_DUMMY1_MASK   0xFFFFFFFF                // R_CONN_FPGA_DUMMY1[31..0]
#define CONN_CFG_ON_CONN_FPGA_DUMMY1_R_CONN_FPGA_DUMMY1_SHFT   0

/* =====================================================================================

  ---CONN_INFRA_CFG_PWRCTRL0 (0x18001000 + 0x200)---

    HW_CONTROL[0]                - (RW) conn_infra_off power control by hw
    LEGACY_SLEEP[1]              - (RW) legacy sleep enable
                                     1'b1: enable(keep conn_infra off MTCMOS on)
                                     1'b0: disable
    CMBDT_BYPASS_EN[2]           - (RW) bypass cmdbt backup flow @no CR update
    CMDBT_BP_BK[3]               - (RW) bypass cmdbt backup
    CMDBT_BP_RS[4]               - (RW) bypass cmdbt restore
    BP_CONN_INFRA_BUS_CK_EN[5]   - (RW) keep conn_infra bus ck enable
    WK_NEED_RS[6]                - (RW) conn_infra wakeup flow need do restore for direct write
    SLP_2_THE_END[7]             - (RW) conn_infra sleep to the end, it will not be break
    RESERVED8[8]                 - (RO) Reserved bits
    HWCTL_OSC_ON_CHECK_TOP_PWR_EN[9] - (RW) hwctl_osc_on control check with top_pwr_enable_d1 from conn_infra_rgu
                                     1'b0: diable
                                     1'b1:enable
    CONN_INFRA_CFG_PWRCTRL0_RSV_0[11..10] - (RW) reserved CR
    CONN_INFRA_CFG_SLP_RDY_MASK[15..12] - (RW) conn_infra_bus_slp_rdy(low -> high) mask for conn_infra_cfg_slp_ctl
                                     1'b0: no mask
                                     1'b1: mask
    CONN_INFRA_CFG_SLP_RDY_DIS_MASK[19..16] - (RW) conn_infra_bus_slp_rdy(high -> low) mask for conn_infra_cfg_slp_ctl
                                     1'b0: no mask
                                     1'b1: mask
    CONN_INFRA_CFG_SLP_RDY_CONN2AP_DIS_MASK[23..20] - (RW) conn2ap slp protect rdy tx and rx (high -> low) mask for conn_infra_cfg_slp_ctl
                                     bit-20: if 1'b1, on2off disable rdy  mask
                                     bit-21: if 1'b1, off2on disable rdy  mask
                                     bit-22: if 1'b1, conn2ap tx disable rdy  mask
                                     bit-23: if 1'b1, conn2ap tx disable rdy  mask
    CONN_INFRA_CFG_SLP_AP2CONN_EN_MASK[26..24] - (RW) conn_infra sleep protect enable by ap2conn rx en mask
                                     bit-24: if 1'b1, conn2ap tx and rx control by ap2conn rx en mask
                                     bit-25:  if 1'b1, on2off control by ap2conn rx en mask
                                     bit-26:  if 1'b1,  off2on control by ap2conn rx en mask
    XTAL_FILTER_BYPASS[27]       - (RW) Bypass XTAL_RDY filter for slp control
    XTAL_FILTER_TIMEOUT[31..28]  - (RW) XTAL_RDY filter timeout value for slp control

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_XTAL_FILTER_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_XTAL_FILTER_TIMEOUT_MASK 0xF0000000                // XTAL_FILTER_TIMEOUT[31..28]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_XTAL_FILTER_TIMEOUT_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_XTAL_FILTER_BYPASS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_XTAL_FILTER_BYPASS_MASK 0x08000000                // XTAL_FILTER_BYPASS[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_XTAL_FILTER_BYPASS_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_AP2CONN_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_AP2CONN_EN_MASK_MASK 0x07000000                // CONN_INFRA_CFG_SLP_AP2CONN_EN_MASK[26..24]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_AP2CONN_EN_MASK_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_CONN2AP_DIS_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_CONN2AP_DIS_MASK_MASK 0x00F00000                // CONN_INFRA_CFG_SLP_RDY_CONN2AP_DIS_MASK[23..20]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_CONN2AP_DIS_MASK_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_DIS_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_DIS_MASK_MASK 0x000F0000                // CONN_INFRA_CFG_SLP_RDY_DIS_MASK[19..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_DIS_MASK_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_MASK_MASK 0x0000F000                // CONN_INFRA_CFG_SLP_RDY_MASK[15..12]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_MASK_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_PWRCTRL0_RSV_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_PWRCTRL0_RSV_0_MASK 0x00000C00                // CONN_INFRA_CFG_PWRCTRL0_RSV_0[11..10]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_PWRCTRL0_RSV_0_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_HWCTL_OSC_ON_CHECK_TOP_PWR_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_HWCTL_OSC_ON_CHECK_TOP_PWR_EN_MASK 0x00000200                // HWCTL_OSC_ON_CHECK_TOP_PWR_EN[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_HWCTL_OSC_ON_CHECK_TOP_PWR_EN_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_SLP_2_THE_END_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_SLP_2_THE_END_MASK 0x00000080                // SLP_2_THE_END[7]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_SLP_2_THE_END_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_WK_NEED_RS_ADDR    CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_WK_NEED_RS_MASK    0x00000040                // WK_NEED_RS[6]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_WK_NEED_RS_SHFT    6
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_BP_CONN_INFRA_BUS_CK_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_BP_CONN_INFRA_BUS_CK_EN_MASK 0x00000020                // BP_CONN_INFRA_BUS_CK_EN[5]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_BP_CONN_INFRA_BUS_CK_EN_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMDBT_BP_RS_ADDR   CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMDBT_BP_RS_MASK   0x00000010                // CMDBT_BP_RS[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMDBT_BP_RS_SHFT   4
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMDBT_BP_BK_ADDR   CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMDBT_BP_BK_MASK   0x00000008                // CMDBT_BP_BK[3]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMDBT_BP_BK_SHFT   3
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMBDT_BYPASS_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMBDT_BYPASS_EN_MASK 0x00000004                // CMBDT_BYPASS_EN[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_CMBDT_BYPASS_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_LEGACY_SLEEP_ADDR  CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_LEGACY_SLEEP_MASK  0x00000002                // LEGACY_SLEEP[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_LEGACY_SLEEP_SHFT  1
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_HW_CONTROL_ADDR    CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_HW_CONTROL_MASK    0x00000001                // HW_CONTROL[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL0_HW_CONTROL_SHFT    0

/* =====================================================================================

  ---CONN_INFRA_CFG_FM_PWRCTRL0 (0x18001000 + 0x204)---

    FMSYS_OSC_EN[0]              - (RW) fmsys osc enable
    RESERVED1[7..1]              - (RO) Reserved bits
    CONN_INFRA_CFG_FM_PWRCTRL0_RSV[31..8] - (RW) reserved CR

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_CONN_INFRA_CFG_FM_PWRCTRL0_RSV_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_CONN_INFRA_CFG_FM_PWRCTRL0_RSV_MASK 0xFFFFFF00                // CONN_INFRA_CFG_FM_PWRCTRL0_RSV[31..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_CONN_INFRA_CFG_FM_PWRCTRL0_RSV_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_FMSYS_OSC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_FMSYS_OSC_EN_MASK 0x00000001                // FMSYS_OSC_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_FM_PWRCTRL0_FMSYS_OSC_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_BT_PWRCTRL0 (0x18001000 + 0x208)---

    BT_FUNCTION_EN[0]            - (RW) btsys function enable -> release reset
    BT1_FUNCTION_EN[1]           - (RW) btsys1 function enable -> release reset
    RESERVED2[7..2]              - (RO) Reserved bits
    CONN_INFRA_CFG_BT_PWRCTRL0_RSV[31..8] - (RW) reserved CR

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_CONN_INFRA_CFG_BT_PWRCTRL0_RSV_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_CONN_INFRA_CFG_BT_PWRCTRL0_RSV_MASK 0xFFFFFF00                // CONN_INFRA_CFG_BT_PWRCTRL0_RSV[31..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_CONN_INFRA_CFG_BT_PWRCTRL0_RSV_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_BT1_FUNCTION_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_BT1_FUNCTION_EN_MASK 0x00000002                // BT1_FUNCTION_EN[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_BT1_FUNCTION_EN_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_BT_FUNCTION_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_BT_FUNCTION_EN_MASK 0x00000001                // BT_FUNCTION_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_BT_PWRCTRL0_BT_FUNCTION_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_GPS_PWRCTRL0 (0x18001000 + 0x20C)---

    GPS_FUNCTION_EN[0]           - (RW) gpssys function enable -> release reset
    RESERVED1[7..1]              - (RO) Reserved bits
    CONN_INFRA_CFG_GPS_PWRCTRL0_RSV[31..8] - (RW) reserved CR

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_CONN_INFRA_CFG_GPS_PWRCTRL0_RSV_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_CONN_INFRA_CFG_GPS_PWRCTRL0_RSV_MASK 0xFFFFFF00                // CONN_INFRA_CFG_GPS_PWRCTRL0_RSV[31..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_CONN_INFRA_CFG_GPS_PWRCTRL0_RSV_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_GPS_FUNCTION_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_GPS_FUNCTION_EN_MASK 0x00000001                // GPS_FUNCTION_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_GPS_PWRCTRL0_GPS_FUNCTION_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_PWRCTRL1 (0x18001000 + 0x210)---

    CONN_INFRA_PWR_STAT[9..0]    - (RO) conn_infra power state
    RESERVED10[15..10]           - (RO) Reserved bits
    CONN_INFRA_RDY[16]           - (RO) conn_infra power on & restore done
    RESERVED17[31..17]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY_MASK 0x00010000                // CONN_INFRA_RDY[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_PWR_STAT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_PWR_STAT_MASK 0x000003FF                // CONN_INFRA_PWR_STAT[9..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_PWR_STAT_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_PWRCTRL2 (0x18001000 + 0x214)---

    XTAL_OFF_TIMEOUT[3..0]       - (RW) conn_infra_slp_ctl xtal off timer value
    CONN_INFRA_CFG_PWRCTRL0_RSV_1[7..4] - (RW) reserved CR
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_CONN_INFRA_CFG_PWRCTRL0_RSV_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_CONN_INFRA_CFG_PWRCTRL0_RSV_1_MASK 0x000000F0                // CONN_INFRA_CFG_PWRCTRL0_RSV_1[7..4]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_CONN_INFRA_CFG_PWRCTRL0_RSV_1_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_XTAL_OFF_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_XTAL_OFF_TIMEOUT_MASK 0x0000000F                // XTAL_OFF_TIMEOUT[3..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL2_XTAL_OFF_TIMEOUT_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_CMDBT_CTL (0x18001000 + 0x220)---

    CMDBT_JMP_RST_EN[0]          - (RW)  xxx 
    CMDBT_FIX_EXTRA_FETCH_EN[1]  - (RW) fix extra_fetch enable
                                     1'b1: enable
                                     1'b0: disable
    CMDBT_AHB_DIS[2]             - (RW) ahb_write function disable
    CMDBT_MEM_WRITE_DIS[3]       - (RW) mem_write function disable
    CONN_INFRA_CMDBT_CG_BYPASS[4] - (RW) cmdbt_cg bypass
    CMDBT_RSV[7..5]              - (RW) reserved CR
    CMDBT_CK_RDY_TIMER[11..8]    - (RW) CMDBT clock ready mask timer (unit: period of xtal)
    RESERVED12[31..12]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_CK_RDY_TIMER_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_CK_RDY_TIMER_MASK 0x00000F00                // CMDBT_CK_RDY_TIMER[11..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_CK_RDY_TIMER_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_RSV_ADDR    CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_RSV_MASK    0x000000E0                // CMDBT_RSV[7..5]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_RSV_SHFT    5
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CONN_INFRA_CMDBT_CG_BYPASS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CONN_INFRA_CMDBT_CG_BYPASS_MASK 0x00000010                // CONN_INFRA_CMDBT_CG_BYPASS[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CONN_INFRA_CMDBT_CG_BYPASS_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_MEM_WRITE_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_MEM_WRITE_DIS_MASK 0x00000008                // CMDBT_MEM_WRITE_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_MEM_WRITE_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_AHB_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_AHB_DIS_MASK 0x00000004                // CMDBT_AHB_DIS[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_AHB_DIS_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_FIX_EXTRA_FETCH_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_FIX_EXTRA_FETCH_EN_MASK 0x00000002                // CMDBT_FIX_EXTRA_FETCH_EN[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_FIX_EXTRA_FETCH_EN_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_JMP_RST_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_JMP_RST_EN_MASK 0x00000001                // CMDBT_JMP_RST_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_CMDBT_CTL_CMDBT_JMP_RST_EN_SHFT 0

/* =====================================================================================

  ---OSC_CTL_0 (0x18001000 + 0x300)---

    XO_XTAL_RDY_STABLE_TIME[7..0] - (RW) vcore ready stable time counter (40 x 1/32kHz)
                                     this stable time is for fake "source clock enable acknowledgement" information masking
    XO_INI_STABLE_TIME[15..8]    - (RW) OSC clock source stable time counter (150 x 1/32KHz)
    XO_BG_STABLE_TIME[23..16]    - (RW) bandgap stable time counter (151 x 1/32KHz)
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_OSC_CTL_0_XO_BG_STABLE_TIME_ADDR           CONN_CFG_ON_OSC_CTL_0_ADDR
#define CONN_CFG_ON_OSC_CTL_0_XO_BG_STABLE_TIME_MASK           0x00FF0000                // XO_BG_STABLE_TIME[23..16]
#define CONN_CFG_ON_OSC_CTL_0_XO_BG_STABLE_TIME_SHFT           16
#define CONN_CFG_ON_OSC_CTL_0_XO_INI_STABLE_TIME_ADDR          CONN_CFG_ON_OSC_CTL_0_ADDR
#define CONN_CFG_ON_OSC_CTL_0_XO_INI_STABLE_TIME_MASK          0x0000FF00                // XO_INI_STABLE_TIME[15..8]
#define CONN_CFG_ON_OSC_CTL_0_XO_INI_STABLE_TIME_SHFT          8
#define CONN_CFG_ON_OSC_CTL_0_XO_XTAL_RDY_STABLE_TIME_ADDR     CONN_CFG_ON_OSC_CTL_0_ADDR
#define CONN_CFG_ON_OSC_CTL_0_XO_XTAL_RDY_STABLE_TIME_MASK     0x000000FF                // XO_XTAL_RDY_STABLE_TIME[7..0]
#define CONN_CFG_ON_OSC_CTL_0_XO_XTAL_RDY_STABLE_TIME_SHFT     0

/* =====================================================================================

  ---OSC_CTL_1 (0x18001000 + 0x304)---

    SWCTL_SRCCLKENA[0]           - (RW) Manual control conn_srcclkena (request osc clock source)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_SRCCLKENA
    SWCTL_EN_BG[1]               - (RW) Manual control da_wbg_en_bg (connsys bandgap enable siganl)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_EN_BG
    SWCTL_EN_XBUF[2]             - (RW) Manual control da_wbg_en_xbuf (connsys xtal buffer enable signal)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_EN_XBUF
    RESERVED3[3]                 - (RO) Reserved bits
    SWCTL_OSC_RDY[4]             - (RW) Manual control osc_rdy (connsys osc ready signal)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_OSC_RDY
    RESERVED5[7..5]              - (RO) Reserved bits
    SW_SRCCLKENA[8]              - (RW) Request osc source clock
                                     1'b0: disable
                                     1'b1: enable
    SW_EN_BG[9]                  - (RW) Enable bandgap
                                     1'b0: disable
                                     1'b1: enable
    SW_EN_XBUF[10]               - (RW) Enable xtal buffer
                                     1'b0: disable
                                     1'b1: enable
    RESERVED11[11]               - (RO) Reserved bits
    SW_OSC_RDY[12]               - (RW) Enable osc reay
                                     1'b0: disable
                                     1'b1: enable
    RESERVED13[15..13]           - (RO) Reserved bits
    ACK_FOR_XO_STATE_MASK[16]    - (RW) mask source clock enable ack to OSC FSM operation
                                     1'h0: un-mask (speed-up sleep-wakeup time when co-clock)
                                     1'h1: mask
    RF_LDO_MASK_FOR_XO[17]       - (RW) mask rf_ldo_rdy  to turn-on OSC
                                     1'h0: un-mask
    RESERVED18[30..18]           - (RO) Reserved bits
    XO_NO_OFF[31]                - (RW) it's a option that not turn-off OSC
                                     1'h0: not force OSC turn-on, OSC turn-on/-off is controlled by normal OSC enable H/W procedure
                                     1'h1: force OSC turn-on

 =====================================================================================*/
#define CONN_CFG_ON_OSC_CTL_1_XO_NO_OFF_ADDR                   CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_XO_NO_OFF_MASK                   0x80000000                // XO_NO_OFF[31]
#define CONN_CFG_ON_OSC_CTL_1_XO_NO_OFF_SHFT                   31
#define CONN_CFG_ON_OSC_CTL_1_RF_LDO_MASK_FOR_XO_ADDR          CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_RF_LDO_MASK_FOR_XO_MASK          0x00020000                // RF_LDO_MASK_FOR_XO[17]
#define CONN_CFG_ON_OSC_CTL_1_RF_LDO_MASK_FOR_XO_SHFT          17
#define CONN_CFG_ON_OSC_CTL_1_ACK_FOR_XO_STATE_MASK_ADDR       CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_ACK_FOR_XO_STATE_MASK_MASK       0x00010000                // ACK_FOR_XO_STATE_MASK[16]
#define CONN_CFG_ON_OSC_CTL_1_ACK_FOR_XO_STATE_MASK_SHFT       16
#define CONN_CFG_ON_OSC_CTL_1_SW_OSC_RDY_ADDR                  CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SW_OSC_RDY_MASK                  0x00001000                // SW_OSC_RDY[12]
#define CONN_CFG_ON_OSC_CTL_1_SW_OSC_RDY_SHFT                  12
#define CONN_CFG_ON_OSC_CTL_1_SW_EN_XBUF_ADDR                  CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SW_EN_XBUF_MASK                  0x00000400                // SW_EN_XBUF[10]
#define CONN_CFG_ON_OSC_CTL_1_SW_EN_XBUF_SHFT                  10
#define CONN_CFG_ON_OSC_CTL_1_SW_EN_BG_ADDR                    CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SW_EN_BG_MASK                    0x00000200                // SW_EN_BG[9]
#define CONN_CFG_ON_OSC_CTL_1_SW_EN_BG_SHFT                    9
#define CONN_CFG_ON_OSC_CTL_1_SW_SRCCLKENA_ADDR                CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SW_SRCCLKENA_MASK                0x00000100                // SW_SRCCLKENA[8]
#define CONN_CFG_ON_OSC_CTL_1_SW_SRCCLKENA_SHFT                8
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_OSC_RDY_ADDR               CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_OSC_RDY_MASK               0x00000010                // SWCTL_OSC_RDY[4]
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_OSC_RDY_SHFT               4
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_EN_XBUF_ADDR               CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_EN_XBUF_MASK               0x00000004                // SWCTL_EN_XBUF[2]
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_EN_XBUF_SHFT               2
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_EN_BG_ADDR                 CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_EN_BG_MASK                 0x00000002                // SWCTL_EN_BG[1]
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_EN_BG_SHFT                 1
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_SRCCLKENA_ADDR             CONN_CFG_ON_OSC_CTL_1_ADDR
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_SRCCLKENA_MASK             0x00000001                // SWCTL_SRCCLKENA[0]
#define CONN_CFG_ON_OSC_CTL_1_SWCTL_SRCCLKENA_SHFT             0

/* =====================================================================================

  ---OSC_MASK (0x18001000 + 0x308)---

    OSC_EN_MASK[15..0]           - (RW) mask OSC enable request
                                     OSC_EN_MASK[12]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for FM driver (from conn_host_csr_top)
                                     OSC_EN_MASK[11]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for GPS driver (from conn_host_csr_top)
                                     OSC_EN_MASK[10]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for BT driver (from conn_host_csr_top)
                                     OSC_EN_MASK[9]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for WF driver (from conn_host_csr_top)
                                     OSC_EN_MASK[8]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for TOP driver (from conn_host_csr_top)
                                     OSC_EN_MASK[7]: set 1'b1 to mask "AP2CONN_OSC_EN" (from TOP)
                                     OSC_EN_MASK[4]: set 1'b1 to mask "CONN_INFRA_BUS_OSC_EN" (from conn_infra bus)
                                     OSC_EN_MASK[3]: set 1'b1 to mask "FMSYS_OSC_ON" (from conn_infra_cfg_reg)
                                     OSC_EN_MASK[2]: set 1'b1 to mask "GPSSYS_OSC_ON" (from gpssys)
                                     OSC_EN_MASK[1]: set 1'b1 to mask "BGFSYS_OSC_ON" (from bgfsys)
                                     OSC_EN_MASK[0]: set 1'b1 to mask "WFSYS_OSC_ON" (from wfsys)
    WFSYS_OSC_ON[16]             - (RO) indicator: "WFSYS_OSC_ON" (from wfsys)
    BGFSYS_OSC_ON[17]            - (RO) indicator: "BGFSYS_OSC_ON" (from bgfsys)
    GPFSYS_OSC_ON[18]            - (RO) indicator: "GPSSYS_OSC_ON" (from gpssys)
    FMSYS_OSC_ON[19]             - (RO) indicator: "FMSYS_OSC_ON" (from conn_infra_cfg_reg)
    CONN_INFRA_BUS_OSC_EN[20]    - (RO) indicator: "CONN_INFRA_BUS_OSC_EN" (from conn_infra bus)
    RESERVED21[22..21]           - (RO) Reserved bits
    AP2CONN_OSC_EN[23]           - (RO) indicator: "AP2CONN_OSC_EN" (from TOP)
    CONN_INFRA_WAKEUP_TOP[24]    - (RO) indicator: "CONN_INFRA_WAKEUP_TOP"(from host_csr)
    CONN_INFRA_WAKEUP_WF[25]     - (RO) indicator: "CONN_INFRA_WAKEUP_WF"(from host_csr)
    CONN_INFRA_WAKEUP_BT[26]     - (RO) indicator: "CONN_INFRA_WAKEUP_BT"(from host_csr)
    CONN_INFRA_WAKEUP_GPS[27]    - (RO) indicator: "CONN_INFRA_WAKEUP_GPS"(from host_csr)
    CONN_INFRA_WAKEUP_FM[28]     - (RO) indicator: "CONN_INFRA_WAKEUP_FM"(from host_csr)
    RESERVED29[31..29]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_FM_ADDR         CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_FM_MASK         0x10000000                // CONN_INFRA_WAKEUP_FM[28]
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_FM_SHFT         28
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_GPS_ADDR        CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_GPS_MASK        0x08000000                // CONN_INFRA_WAKEUP_GPS[27]
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_GPS_SHFT        27
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_BT_ADDR         CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_BT_MASK         0x04000000                // CONN_INFRA_WAKEUP_BT[26]
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_BT_SHFT         26
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_WF_ADDR         CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_WF_MASK         0x02000000                // CONN_INFRA_WAKEUP_WF[25]
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_WF_SHFT         25
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_TOP_ADDR        CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_TOP_MASK        0x01000000                // CONN_INFRA_WAKEUP_TOP[24]
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_WAKEUP_TOP_SHFT        24
#define CONN_CFG_ON_OSC_MASK_AP2CONN_OSC_EN_ADDR               CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_AP2CONN_OSC_EN_MASK               0x00800000                // AP2CONN_OSC_EN[23]
#define CONN_CFG_ON_OSC_MASK_AP2CONN_OSC_EN_SHFT               23
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_BUS_OSC_EN_ADDR        CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_BUS_OSC_EN_MASK        0x00100000                // CONN_INFRA_BUS_OSC_EN[20]
#define CONN_CFG_ON_OSC_MASK_CONN_INFRA_BUS_OSC_EN_SHFT        20
#define CONN_CFG_ON_OSC_MASK_FMSYS_OSC_ON_ADDR                 CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_FMSYS_OSC_ON_MASK                 0x00080000                // FMSYS_OSC_ON[19]
#define CONN_CFG_ON_OSC_MASK_FMSYS_OSC_ON_SHFT                 19
#define CONN_CFG_ON_OSC_MASK_GPFSYS_OSC_ON_ADDR                CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_GPFSYS_OSC_ON_MASK                0x00040000                // GPFSYS_OSC_ON[18]
#define CONN_CFG_ON_OSC_MASK_GPFSYS_OSC_ON_SHFT                18
#define CONN_CFG_ON_OSC_MASK_BGFSYS_OSC_ON_ADDR                CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_BGFSYS_OSC_ON_MASK                0x00020000                // BGFSYS_OSC_ON[17]
#define CONN_CFG_ON_OSC_MASK_BGFSYS_OSC_ON_SHFT                17
#define CONN_CFG_ON_OSC_MASK_WFSYS_OSC_ON_ADDR                 CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_WFSYS_OSC_ON_MASK                 0x00010000                // WFSYS_OSC_ON[16]
#define CONN_CFG_ON_OSC_MASK_WFSYS_OSC_ON_SHFT                 16
#define CONN_CFG_ON_OSC_MASK_OSC_EN_MASK_ADDR                  CONN_CFG_ON_OSC_MASK_ADDR
#define CONN_CFG_ON_OSC_MASK_OSC_EN_MASK_MASK                  0x0000FFFF                // OSC_EN_MASK[15..0]
#define CONN_CFG_ON_OSC_MASK_OSC_EN_MASK_SHFT                  0

/* =====================================================================================

  ---OSC_STATUS (0x18001000 + 0x30C)---

    RESERVED0[7..0]              - (RO) Reserved bits
    XO_STATE[10..8]              - (RO) OSC FSM state
                                     3'h0: XO_XTAL_RDY
                                     3'h1: XO_INI
                                     3'h2: XO_PON_BG
                                     3'h3: XO_PON_XBUF
                                     3'h4: XO_POWERON
                                     3'h5: XO_POFF_RDY
                                     3'h6: XO_POFF_BG
                                     3'h7: XO_POWEROFF
    XO_EN[11]                    - (RO) OSC FSM enable
                                     1'h0: disable
                                     1'h1: enable
    HW_OSC_RDY[12]               - (RO) OSC FSM: osc ready
                                     1'h0: not ready
                                     1'h1: ready
    HW_DA_WBG_EN_XBUF[13]        - (RO) OSC FSM: wbg xtal buf enable
                                     1'h0: disable
                                     1'h1: enable
    HW_DA_WBG_EN_BG[14]          - (RO) OSC FSM: wbg bandgap enable
                                     1'h0: disable
                                     1'h1: enable
    RESERVED15[15]               - (RO) Reserved bits
    HW_CONNSRCCLKENA[16]         - (RO) OSC FSM: srcclkena
                                     1'h0: disable
                                     1'h1: enable
    RESERVED17[31..17]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_OSC_STATUS_HW_CONNSRCCLKENA_ADDR           CONN_CFG_ON_OSC_STATUS_ADDR
#define CONN_CFG_ON_OSC_STATUS_HW_CONNSRCCLKENA_MASK           0x00010000                // HW_CONNSRCCLKENA[16]
#define CONN_CFG_ON_OSC_STATUS_HW_CONNSRCCLKENA_SHFT           16
#define CONN_CFG_ON_OSC_STATUS_HW_DA_WBG_EN_BG_ADDR            CONN_CFG_ON_OSC_STATUS_ADDR
#define CONN_CFG_ON_OSC_STATUS_HW_DA_WBG_EN_BG_MASK            0x00004000                // HW_DA_WBG_EN_BG[14]
#define CONN_CFG_ON_OSC_STATUS_HW_DA_WBG_EN_BG_SHFT            14
#define CONN_CFG_ON_OSC_STATUS_HW_DA_WBG_EN_XBUF_ADDR          CONN_CFG_ON_OSC_STATUS_ADDR
#define CONN_CFG_ON_OSC_STATUS_HW_DA_WBG_EN_XBUF_MASK          0x00002000                // HW_DA_WBG_EN_XBUF[13]
#define CONN_CFG_ON_OSC_STATUS_HW_DA_WBG_EN_XBUF_SHFT          13
#define CONN_CFG_ON_OSC_STATUS_HW_OSC_RDY_ADDR                 CONN_CFG_ON_OSC_STATUS_ADDR
#define CONN_CFG_ON_OSC_STATUS_HW_OSC_RDY_MASK                 0x00001000                // HW_OSC_RDY[12]
#define CONN_CFG_ON_OSC_STATUS_HW_OSC_RDY_SHFT                 12
#define CONN_CFG_ON_OSC_STATUS_XO_EN_ADDR                      CONN_CFG_ON_OSC_STATUS_ADDR
#define CONN_CFG_ON_OSC_STATUS_XO_EN_MASK                      0x00000800                // XO_EN[11]
#define CONN_CFG_ON_OSC_STATUS_XO_EN_SHFT                      11
#define CONN_CFG_ON_OSC_STATUS_XO_STATE_ADDR                   CONN_CFG_ON_OSC_STATUS_ADDR
#define CONN_CFG_ON_OSC_STATUS_XO_STATE_MASK                   0x00000700                // XO_STATE[10..8]
#define CONN_CFG_ON_OSC_STATUS_XO_STATE_SHFT                   8

/* =====================================================================================

  ---OSC_2X_CTL_0 (0x18001000 + 0x310)---

    XO_XTAL_RDY_STABLE_TIME[7..0] - (RW) vcore ready stable time counter (40 x 1/32kHz)
                                     this stable time is for fake "source clock enable acknowledgement" information masking
    XO_INI_STABLE_TIME[15..8]    - (RW) OSC clock source stable time counter (150 x 1/32KHz)
    XO_BG_STABLE_TIME[23..16]    - (RW) bandgap stable time counter (151 x 1/32KHz)
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_BG_STABLE_TIME_ADDR        CONN_CFG_ON_OSC_2X_CTL_0_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_BG_STABLE_TIME_MASK        0x00FF0000                // XO_BG_STABLE_TIME[23..16]
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_BG_STABLE_TIME_SHFT        16
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_INI_STABLE_TIME_ADDR       CONN_CFG_ON_OSC_2X_CTL_0_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_INI_STABLE_TIME_MASK       0x0000FF00                // XO_INI_STABLE_TIME[15..8]
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_INI_STABLE_TIME_SHFT       8
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_XTAL_RDY_STABLE_TIME_ADDR  CONN_CFG_ON_OSC_2X_CTL_0_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_XTAL_RDY_STABLE_TIME_MASK  0x000000FF                // XO_XTAL_RDY_STABLE_TIME[7..0]
#define CONN_CFG_ON_OSC_2X_CTL_0_XO_XTAL_RDY_STABLE_TIME_SHFT  0

/* =====================================================================================

  ---OSC_2X_CTL_1 (0x18001000 + 0x314)---

    SWCTL_SRCCLKENA[0]           - (RW) Manual control conn_srcclkena (request osc clock source)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_SRCCLKENA
    SWCTL_EN_BG[1]               - (RW) Manual control da_wbg_en_bg (connsys bandgap enable siganl)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_EN_BG
    SWCTL_EN_XBUF[2]             - (RW) Manual control da_wbg_en_xbuf (connsys xtal buffer enable signal)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_EN_XBUF
    SWCTL_XTAL_RDY[3]            - (RW) Manual control osc_rdy (connsys vcore ready signal)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_XTAL_RDY
    SWCTL_OSC_RDY[4]             - (RW) Manual control osc_rdy (connsys osc ready signal)
                                     1'h0: HW control mode
                                     1'h1: manual mode, value is set by SW_OSC_RDY
    RESERVED5[7..5]              - (RO) Reserved bits
    SW_SRCCLKENA[8]              - (RW) Request osc source clock
                                     1'b0: disable
                                     1'b1: enable
    SW_EN_BG[9]                  - (RW) Enable bandgap
                                     1'b0: disable
                                     1'b1: enable
    SW_EN_XBUF[10]               - (RW) Enable xtal buffer
                                     1'b0: disable
                                     1'b1: enable
    SW_XTAL_RDY[11]              - (RW) Enable vcore reay
                                     1'b0: disable
                                     1'b1: enable
    SW_OSC_RDY[12]               - (RW) Enable osc reay
                                     1'b0: disable
                                     1'b1: enable
    RESERVED13[15..13]           - (RO) Reserved bits
    ACK_FOR_XO_STATE_MASK[16]    - (RW) mask source clock enable ack to OSC FSM operation
                                     1'h0: un-mask (speed-up sleep-wakeup time when co-clock)
                                     1'h1: mask
    RF_LDO_MASK_FOR_XO[17]       - (RW) mask rf_ldo_rdy  to turn-on OSC
                                     1'h0: un-mask
    RESERVED18[30..18]           - (RO) Reserved bits
    XO_NO_OFF[31]                - (RW) it's a option that not turn-off OSC
                                     1'h0: not force OSC turn-on, OSC turn-on/-off is controlled by normal OSC enable H/W procedure
                                     1'h1: force OSC turn-on

 =====================================================================================*/
#define CONN_CFG_ON_OSC_2X_CTL_1_XO_NO_OFF_ADDR                CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_XO_NO_OFF_MASK                0x80000000                // XO_NO_OFF[31]
#define CONN_CFG_ON_OSC_2X_CTL_1_XO_NO_OFF_SHFT                31
#define CONN_CFG_ON_OSC_2X_CTL_1_RF_LDO_MASK_FOR_XO_ADDR       CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_RF_LDO_MASK_FOR_XO_MASK       0x00020000                // RF_LDO_MASK_FOR_XO[17]
#define CONN_CFG_ON_OSC_2X_CTL_1_RF_LDO_MASK_FOR_XO_SHFT       17
#define CONN_CFG_ON_OSC_2X_CTL_1_ACK_FOR_XO_STATE_MASK_ADDR    CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_ACK_FOR_XO_STATE_MASK_MASK    0x00010000                // ACK_FOR_XO_STATE_MASK[16]
#define CONN_CFG_ON_OSC_2X_CTL_1_ACK_FOR_XO_STATE_MASK_SHFT    16
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_OSC_RDY_ADDR               CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_OSC_RDY_MASK               0x00001000                // SW_OSC_RDY[12]
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_OSC_RDY_SHFT               12
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_XTAL_RDY_ADDR              CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_XTAL_RDY_MASK              0x00000800                // SW_XTAL_RDY[11]
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_XTAL_RDY_SHFT              11
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_EN_XBUF_ADDR               CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_EN_XBUF_MASK               0x00000400                // SW_EN_XBUF[10]
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_EN_XBUF_SHFT               10
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_EN_BG_ADDR                 CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_EN_BG_MASK                 0x00000200                // SW_EN_BG[9]
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_EN_BG_SHFT                 9
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_SRCCLKENA_ADDR             CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_SRCCLKENA_MASK             0x00000100                // SW_SRCCLKENA[8]
#define CONN_CFG_ON_OSC_2X_CTL_1_SW_SRCCLKENA_SHFT             8
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_OSC_RDY_ADDR            CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_OSC_RDY_MASK            0x00000010                // SWCTL_OSC_RDY[4]
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_OSC_RDY_SHFT            4
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_XTAL_RDY_ADDR           CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_XTAL_RDY_MASK           0x00000008                // SWCTL_XTAL_RDY[3]
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_XTAL_RDY_SHFT           3
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_EN_XBUF_ADDR            CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_EN_XBUF_MASK            0x00000004                // SWCTL_EN_XBUF[2]
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_EN_XBUF_SHFT            2
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_EN_BG_ADDR              CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_EN_BG_MASK              0x00000002                // SWCTL_EN_BG[1]
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_EN_BG_SHFT              1
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_SRCCLKENA_ADDR          CONN_CFG_ON_OSC_2X_CTL_1_ADDR
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_SRCCLKENA_MASK          0x00000001                // SWCTL_SRCCLKENA[0]
#define CONN_CFG_ON_OSC_2X_CTL_1_SWCTL_SRCCLKENA_SHFT          0

/* =====================================================================================

  ---OSC_2X_MASK (0x18001000 + 0x318)---

    OSC_2X_EN_MASK_7_0[7..0]     - (RW) mask OSC_2X enable request
                                     OSC_EN_MASK[7]: set 1'b1 to mask "AP2CONN_OSC_EN" (from TOP)
                                     OSC_EN_MASK[6]: set 1'b1 to mask "CONN_THM_OSC_EN" (from conn_therm_ctl)
                                     OSC_EN_MASK[5]: set 1'b1 to mask "CONN_PTA_OSC_EN" (from conn_pta6)
                                     OSC_EN_MASK[4]: set 1'b1 to mask "CONN_INFRA_BUS_OSC_EN" (from conn_infra bus)
                                     OSC_EN_MASK[3]: set 1'b1 to mask "FMSYS_OSC_ON" (from conn_infra_cfg_reg)
                                     OSC_EN_MASK[2]: set 1'b1 to mask "GPSSYS_OSC_ON" (from gpssys)
                                     OSC_EN_MASK[1]: set 1'b1 to mask "BGFSYS_OSC_ON" (from bgfsys)
                                     OSC_EN_MASK[0]: set 1'b1 to mask "WFSYS_OSC_ON" (from wfsys)
    RESERVED8[15..8]             - (RO) Reserved bits
    OSC_2X_EN_MASK_12_8[20..16]  - (RW) mask OSC_2X enable request
                                     OSC_EN_MASK[12]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for FM driver (from conn_host_csr_top)
                                     OSC_EN_MASK[11]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for GPS driver (from conn_host_csr_top)
                                     OSC_EN_MASK[10]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for BT driver (from conn_host_csr_top)
                                     OSC_EN_MASK[9]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for WF driver (from conn_host_csr_top)
                                     OSC_EN_MASK[8]: set 1'b1 to mask "cr_wakeup_conn_infra_off" used for TOP driver (from conn_host_csr_top)
    OSC_2X_EN_MASK_13[21]        - (RW) mask OSC_2X enable request
                                     OSC_EN_MASK[13]: set 1'b1 to mask "BGFSYS_OSC_2X_ON" (from bgfsys)
    RESERVED22[31..22]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_13_ADDR         CONN_CFG_ON_OSC_2X_MASK_ADDR
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_13_MASK         0x00200000                // OSC_2X_EN_MASK_13[21]
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_13_SHFT         21
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_12_8_ADDR       CONN_CFG_ON_OSC_2X_MASK_ADDR
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_12_8_MASK       0x001F0000                // OSC_2X_EN_MASK_12_8[20..16]
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_12_8_SHFT       16
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_7_0_ADDR        CONN_CFG_ON_OSC_2X_MASK_ADDR
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_7_0_MASK        0x000000FF                // OSC_2X_EN_MASK_7_0[7..0]
#define CONN_CFG_ON_OSC_2X_MASK_OSC_2X_EN_MASK_7_0_SHFT        0

/* =====================================================================================

  ---OSC_2X_STATUS (0x18001000 + 0x31C)---

    BUS_OSC_SW_RDY[0]            - (RO) conn_infra BUS clock switch to OSC clock ready indicator
                                     1'h0: not ready
                                     1'h1: ready
    BUS_32K_SW_RDY[1]            - (RO) conn_infra BUS clock switch to 32KHz clock ready indicator
                                     1'h0: not ready
                                     1'h1: ready
    BUS_BGFSYS_CK_SW_RDY[2]      - (RO) conn_infra BUS clock switch to bgfsys BUS clock ready indicator
                                     1'h0: not ready
                                     1'h1: ready
    BUS_WFSYS_CK_SW_RDY[3]       - (RO) conn_infra BUS clock switch to wfsys BUS clock ready indicator
                                     1'h0: not ready
                                     1'h1: ready
    RESERVED4[7..4]              - (RO) Reserved bits
    XO_STATE[10..8]              - (RO) OSC FSM state
                                     3'h0: XO_XTAL_RDY
                                     3'h1: XO_INI
                                     3'h2: XO_PON_BG
                                     3'h3: XO_PON_XBUF
                                     3'h4: XO_POWERON
                                     3'h5: XO_POFF_RDY
                                     3'h6: XO_POFF_BG
                                     3'h7: XO_POWEROFF
    XO_EN[11]                    - (RO) OSC FSM enable
                                     1'h0: disable
                                     1'h1: enable
    HW_OSC_RDY[12]               - (RO) OSC FSM: osc ready
                                     1'h0: not ready
                                     1'h1: ready
    HW_DA_WBG_EN_XBUF[13]        - (RO) OSC FSM: wbg xtal buf enable
                                     1'h0: disable
                                     1'h1: enable
    HW_DA_WBG_EN_BG[14]          - (RO) OSC FSM: wbg bandgap enable
                                     1'h0: disable
                                     1'h1: enable
    HW_XTAL_RDY[15]              - (RO) OSC FSM: vcore ready
                                     1'h0: not ready
                                     1'h1: ready
    HW_CONNSRCCLKENA[16]         - (RO) OSC FSM: srcclkena
                                     1'h0: disable
                                     1'h1: enable
    RESERVED17[31..17]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_OSC_2X_STATUS_HW_CONNSRCCLKENA_ADDR        CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_HW_CONNSRCCLKENA_MASK        0x00010000                // HW_CONNSRCCLKENA[16]
#define CONN_CFG_ON_OSC_2X_STATUS_HW_CONNSRCCLKENA_SHFT        16
#define CONN_CFG_ON_OSC_2X_STATUS_HW_XTAL_RDY_ADDR             CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_HW_XTAL_RDY_MASK             0x00008000                // HW_XTAL_RDY[15]
#define CONN_CFG_ON_OSC_2X_STATUS_HW_XTAL_RDY_SHFT             15
#define CONN_CFG_ON_OSC_2X_STATUS_HW_DA_WBG_EN_BG_ADDR         CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_HW_DA_WBG_EN_BG_MASK         0x00004000                // HW_DA_WBG_EN_BG[14]
#define CONN_CFG_ON_OSC_2X_STATUS_HW_DA_WBG_EN_BG_SHFT         14
#define CONN_CFG_ON_OSC_2X_STATUS_HW_DA_WBG_EN_XBUF_ADDR       CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_HW_DA_WBG_EN_XBUF_MASK       0x00002000                // HW_DA_WBG_EN_XBUF[13]
#define CONN_CFG_ON_OSC_2X_STATUS_HW_DA_WBG_EN_XBUF_SHFT       13
#define CONN_CFG_ON_OSC_2X_STATUS_HW_OSC_RDY_ADDR              CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_HW_OSC_RDY_MASK              0x00001000                // HW_OSC_RDY[12]
#define CONN_CFG_ON_OSC_2X_STATUS_HW_OSC_RDY_SHFT              12
#define CONN_CFG_ON_OSC_2X_STATUS_XO_EN_ADDR                   CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_XO_EN_MASK                   0x00000800                // XO_EN[11]
#define CONN_CFG_ON_OSC_2X_STATUS_XO_EN_SHFT                   11
#define CONN_CFG_ON_OSC_2X_STATUS_XO_STATE_ADDR                CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_XO_STATE_MASK                0x00000700                // XO_STATE[10..8]
#define CONN_CFG_ON_OSC_2X_STATUS_XO_STATE_SHFT                8
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_WFSYS_CK_SW_RDY_ADDR     CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_WFSYS_CK_SW_RDY_MASK     0x00000008                // BUS_WFSYS_CK_SW_RDY[3]
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_WFSYS_CK_SW_RDY_SHFT     3
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_BGFSYS_CK_SW_RDY_ADDR    CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_BGFSYS_CK_SW_RDY_MASK    0x00000004                // BUS_BGFSYS_CK_SW_RDY[2]
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_BGFSYS_CK_SW_RDY_SHFT    2
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_32K_SW_RDY_ADDR          CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_32K_SW_RDY_MASK          0x00000002                // BUS_32K_SW_RDY[1]
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_32K_SW_RDY_SHFT          1
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_OSC_SW_RDY_ADDR          CONN_CFG_ON_OSC_2X_STATUS_ADDR
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_OSC_SW_RDY_MASK          0x00000001                // BUS_OSC_SW_RDY[0]
#define CONN_CFG_ON_OSC_2X_STATUS_BUS_OSC_SW_RDY_SHFT          0

/* =====================================================================================

  ---CONN_INFRA_CFG_OSC_MASK1 (0x18001000 + 0x320)---

    func_req_mask[7..0]          - (RO) mask FUNC request
                                     FUNC_REQ_MASK[4]: set 1'b1 to mask "conn_thm_osc_en_1" (from conn_thm_ctl_1)
                                     FUNC_REQ_MASK[3]: set 1'b1 to mask "conn_thm_osc_en" (from conn_thm_ctl)
                                     FUNC_REQ_MASK[2]: set 1'b1 to mask "conn_spi_osc_en_1" (from conn_rfspi_ctl_1)
                                     FUNC_REQ_MASK[1]: set 1'b1 to mask "conn_spi_osc_en" (from conn_rfspi_ctl)
                                     FUNC_REQ_MASK[0]: set 1'b1 to mask "conn_pta_osc_en" (from conn_pta)
    RESERVED8[15..8]             - (RO) Reserved bits
    conn_pta_osc_en[16]          - (RO) indicator: "conn_pta_osc_en" (from conn_pta)
    conn_spi_osc_en[17]          - (RO) indicator: "conn_spi_osc_en" (from conn_rfspi_ctl)
    conn_spi_osc_en_1[18]        - (RO) indicator: "conn_spi_osc_en_1" (from conn_rfspi_ctl_1)
    conn_thm_osc_en[19]          - (RO) indicator: "conn_thm_osc_en" (from conn_thm_ctl)
    conn_thm_osc_en_1[20]        - (RO) indicator: "conn_thm_osc_en_1" (from conn_thm_ctl_1)
    RESERVED21[31..21]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_thm_osc_en_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_thm_osc_en_1_MASK 0x00100000                // conn_thm_osc_en_1[20]
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_thm_osc_en_1_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_thm_osc_en_ADDR CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_thm_osc_en_MASK 0x00080000                // conn_thm_osc_en[19]
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_thm_osc_en_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_spi_osc_en_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_spi_osc_en_1_MASK 0x00040000                // conn_spi_osc_en_1[18]
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_spi_osc_en_1_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_spi_osc_en_ADDR CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_spi_osc_en_MASK 0x00020000                // conn_spi_osc_en[17]
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_spi_osc_en_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_pta_osc_en_ADDR CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_pta_osc_en_MASK 0x00010000                // conn_pta_osc_en[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_conn_pta_osc_en_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_func_req_mask_ADDR CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_func_req_mask_MASK 0x000000FF                // func_req_mask[7..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_OSC_MASK1_func_req_mask_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_FUNC_GRNT (0x18001000 + 0x330)---

    conn_pta_grnt_filter_timeout[3..0] - (RW) conn_pta_grnt filter timeout setting(@osc_ck)
    conn_spi_grnt_filter_timeout[7..4] - (RW) conn_spi_grnt filter timeout setting(@osc_ck)
    conn_spi_grnt_filter_timeout_1[11..8] - (RW) conn_spi_grnt_1 filter timeout setting(@osc_ck)
    conn_thm_grnt_filter_timeout[15..12] - (RW) conn_thm_grnt filter timeout setting(@osc_ck)
    conn_thm_grnt_filter_timeout_1[19..16] - (RW) conn_thm_grnt_1 filter timeout setting(@osc_ck)
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_thm_grnt_filter_timeout_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_thm_grnt_filter_timeout_1_MASK 0x000F0000                // conn_thm_grnt_filter_timeout_1[19..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_thm_grnt_filter_timeout_1_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_thm_grnt_filter_timeout_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_thm_grnt_filter_timeout_MASK 0x0000F000                // conn_thm_grnt_filter_timeout[15..12]
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_thm_grnt_filter_timeout_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_spi_grnt_filter_timeout_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_spi_grnt_filter_timeout_1_MASK 0x00000F00                // conn_spi_grnt_filter_timeout_1[11..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_spi_grnt_filter_timeout_1_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_spi_grnt_filter_timeout_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_spi_grnt_filter_timeout_MASK 0x000000F0                // conn_spi_grnt_filter_timeout[7..4]
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_spi_grnt_filter_timeout_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_pta_grnt_filter_timeout_ADDR CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_pta_grnt_filter_timeout_MASK 0x0000000F                // conn_pta_grnt_filter_timeout[3..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_FUNC_GRNT_conn_pta_grnt_filter_timeout_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_0 (0x18001000 + 0x340)---

    OSC_LEGACY_EN[0]             - (RW) connsys osc legacy control enable
    SW_OSC_ON_ALL[1]             - (RW) sw control osc_on_all (with OSC_LEGACY_EN = 1'b0)
    SW_OSC2X_ON_ALL[2]           - (RW) sw control osc2x_on_all (with OSC2X_LEGACY_EN = 1'b0)
    OSC_LEGACY_EN_2X[3]          - (RW) connsys osc_2x legacy control enable
    GPSSYS_OSC_RC_EN[4]          - (RW) gpssys osc RC control enable
    BTSYS_OSC_RC_EN[5]           - (RW) btsys osc RC control enable
    WFSYS_OSC_RC_EN[6]           - (RW) wfsys osc RC control enable
    CONN_INFRA_OSC_RC_EN[7]      - (RW) conn_infra osc RC control enable
    SW_GPSSYS_OSC_ON_RC[8]       - (RW) sw control gpssys_osc_on(with gpssys_osc_rc_en = 1'b0)
    SW_BTSYS_OSC_ON_RC[9]        - (RW) sw control btsys_osc_on(with btsys_osc_rc_en = 1'b0)
    SW_WFSYS_OSC_ON_RC[10]       - (RW) sw control wfsys_osc_on(with wfsys_osc_rc_en = 1'b0)
    SW_CONN_INFRA_OSC_ON_RC[11]  - (RW) sw control conn_infra_osc_on(with conn_infra_osc_rc_en = 1'b0)
    GPSSYS_BBLPM_EN[12]          - (RW) conn_srcclkena_3_fpm/bblpm selection
                                     1'b0: control srcclkena_3_fpm
                                     1'b1: control srcclkena_3_bblpm
    BTSYS_BBLPM_EN[13]           - (RW) conn_srcclkena_3_fpm/bblpm selection
                                     1'b0: control srcclkena_3_fpm
                                     1'b1: control srcclkena_3_bblpm
    WFSYS_BBLPM_EN[14]           - (RW) conn_srcclkena_3_fpm/bblpm selection
                                     1'b0: control srcclkena_3_fpm
                                     1'b1: control srcclkena_3_bblpm
    CONN_INFRA_BBLPM_EN[15]      - (RW) conn_srcclkena_3_fpm/bblpm selection
                                     1'b0: control srcclkena_3_fpm
                                     1'b1: control srcclkena_3_bblpm
    GPSSYS_OSC_RST_B_SEL[16]     - (RW) gpssys_osc_on_ack Flip-Flop reset selection
                                     1'b0: gpssys_osc_on_ack reset with gpssys_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
                                     1'b1: gpssys_osc_on_ack reset without gpssys_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
    BTSYS_OSC_RST_B_SEL[17]      - (RW) btsys_osc_on_ack Flip-Flop reset selection
                                     1'b0: btsys_osc_on_ack reset with btsys_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
                                     1'b1: btsys_osc_on_ack reset without btsys_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
    WFSYS_OSC_RST_B_SEL[18]      - (RW) wfsys_osc_on_ack Flip-Flop reset selection
                                     1'b0: wfsys_osc_on_ack reset with wfsys_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
                                     1'b1: wfsys_osc_on_ack reset without wfsys_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
    CONN_INFRA_OSC_RST_B_SEL[19] - (RW) conn_infra_osc_on_ack Flip-Flop reset selection (no used)
                                     1'b0: conn_infra_osc_on_ack reset with conn_infra_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
                                     1'b1: conn_infra_osc_on_ack reset without conn_infra_osc_on/rgu_conn_infra_mem_rst_b/conn_infra_off_active
    CONN_BT_ONLY_RC_EN[20]       - (RW) conn_infra_osc_ctl configure as conn_bt_only mode
                                     conn_srcclkena_1 enable by conn_infra_osc_on
    GPSSYS_XO_FL_RST_B_SEL[21]   - (RW) gpssys_XTAL_RDY Flip-Flop reset selection
                                     1'b0: gpssys_XTAL_RDY reset with gpssys_osc_on
                                     1'b1: gpssys_XTAL_RDY reset without gpssys_osc_on
    BTSYS_XO_FL_RST_B_SEL[22]    - (RW) btsys_XTAL_RDY Flip-Flop reset selection
                                     1'b0: btsys_XTAL_RDY reset with btsys_osc_on
                                     1'b1: btsys_XTAL_RDY reset without btsys_osc_on
    WFSYS_XO_FL_RST_B_SEL[23]    - (RW) wfsys_XTAL_RDY Flip-Flop reset selection
                                     1'b0: wfsys_XTAL_RDY reset with wfsys_osc_on
                                     1'b1: wfsys_XTAL_RDY reset without wfsys_osc_on
    CONN_INFRA_XO_FL_RST_B_SEL[24] - (RW) conn_infra_XTAL_RDY Flip-Flop reset selection (no used)
                                     1'b0: conn_infra_XTAL_RDY reset with conn_infra_osc_on
                                     1'b1: conn_infra_XTAL_RDY reset without conn_infra_osc_on
    CONN_BT_ONLY_WGF_RC_EN[25]   - (RW) conn_infra_osc_ctl configure as conn_bt_only mode
                                     conn_srcclkena enable by wgf_osc_en(wf/gps/fm)
    CONN_RC1_SEL[26]             - (RW) conn_srcclkena_1  source selection
                                     1'b0: conn_srcclkena_1
                                     1'b1: conn_srcclkena_3
    CONN_RC3_SEL[27]             - (RW) conn_srcclkena_3  source selection
                                     1'b0: conn_srcclkena_3
                                     1'b1: conn_srcclkena_1
    CONN_BT_ONLY_RC_OUT_EN[28]   - (RW) conn_infra_osc_ctl configure as conn_bt_only mode
                                     1'b1: osc_ack_xxx/XTAL_RDY_xxx source is osc_ack_1/XTAL_RDY_1
    OSC_EN_SLP_CTL_MASK[29]      - (RW) osc_en_slp_ctl mask for XTAL_RDY/osc_on_ack reset
                                     1'b0: unmask; conn_infra_cfg_slp osc_en will be the reset signal for XTAL_RDY/osc_on_ack
                                     1'b1: mask;
    RC_BK_EN[30]                 - (RW) conn_infra_osc_ctl configure as rc_backup mode
    OSC_LEGACY_OUT_EN[31]        - (RW) osc legacy control output enable

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_OUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_OUT_EN_MASK 0x80000000                // OSC_LEGACY_OUT_EN[31]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_OUT_EN_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_RC_BK_EN_ADDR      CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_RC_BK_EN_MASK      0x40000000                // RC_BK_EN[30]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_RC_BK_EN_SHFT      30
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_EN_SLP_CTL_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_EN_SLP_CTL_MASK_MASK 0x20000000                // OSC_EN_SLP_CTL_MASK[29]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_EN_SLP_CTL_MASK_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_RC_OUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_RC_OUT_EN_MASK 0x10000000                // CONN_BT_ONLY_RC_OUT_EN[28]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_RC_OUT_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_RC3_SEL_ADDR  CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_RC3_SEL_MASK  0x08000000                // CONN_RC3_SEL[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_RC3_SEL_SHFT  27
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_RC1_SEL_ADDR  CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_RC1_SEL_MASK  0x04000000                // CONN_RC1_SEL[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_RC1_SEL_SHFT  26
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_WGF_RC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_WGF_RC_EN_MASK 0x02000000                // CONN_BT_ONLY_WGF_RC_EN[25]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_WGF_RC_EN_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_XO_FL_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_XO_FL_RST_B_SEL_MASK 0x01000000                // CONN_INFRA_XO_FL_RST_B_SEL[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_XO_FL_RST_B_SEL_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_XO_FL_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_XO_FL_RST_B_SEL_MASK 0x00800000                // WFSYS_XO_FL_RST_B_SEL[23]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_XO_FL_RST_B_SEL_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_XO_FL_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_XO_FL_RST_B_SEL_MASK 0x00400000                // BTSYS_XO_FL_RST_B_SEL[22]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_XO_FL_RST_B_SEL_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_XO_FL_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_XO_FL_RST_B_SEL_MASK 0x00200000                // GPSSYS_XO_FL_RST_B_SEL[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_XO_FL_RST_B_SEL_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_RC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_RC_EN_MASK 0x00100000                // CONN_BT_ONLY_RC_EN[20]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_BT_ONLY_RC_EN_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RST_B_SEL_MASK 0x00080000                // CONN_INFRA_OSC_RST_B_SEL[19]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RST_B_SEL_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_OSC_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_OSC_RST_B_SEL_MASK 0x00040000                // WFSYS_OSC_RST_B_SEL[18]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_OSC_RST_B_SEL_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_OSC_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_OSC_RST_B_SEL_MASK 0x00020000                // BTSYS_OSC_RST_B_SEL[17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_OSC_RST_B_SEL_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_OSC_RST_B_SEL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_OSC_RST_B_SEL_MASK 0x00010000                // GPSSYS_OSC_RST_B_SEL[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_OSC_RST_B_SEL_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_BBLPM_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_BBLPM_EN_MASK 0x00008000                // CONN_INFRA_BBLPM_EN[15]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_BBLPM_EN_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_BBLPM_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_BBLPM_EN_MASK 0x00004000                // WFSYS_BBLPM_EN[14]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_BBLPM_EN_SHFT 14
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_BBLPM_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_BBLPM_EN_MASK 0x00002000                // BTSYS_BBLPM_EN[13]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_BBLPM_EN_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_BBLPM_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_BBLPM_EN_MASK 0x00001000                // GPSSYS_BBLPM_EN[12]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_BBLPM_EN_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_CONN_INFRA_OSC_ON_RC_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_CONN_INFRA_OSC_ON_RC_MASK 0x00000800                // SW_CONN_INFRA_OSC_ON_RC[11]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_CONN_INFRA_OSC_ON_RC_SHFT 11
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_WFSYS_OSC_ON_RC_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_WFSYS_OSC_ON_RC_MASK 0x00000400                // SW_WFSYS_OSC_ON_RC[10]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_WFSYS_OSC_ON_RC_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_BTSYS_OSC_ON_RC_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_BTSYS_OSC_ON_RC_MASK 0x00000200                // SW_BTSYS_OSC_ON_RC[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_BTSYS_OSC_ON_RC_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_GPSSYS_OSC_ON_RC_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_GPSSYS_OSC_ON_RC_MASK 0x00000100                // SW_GPSSYS_OSC_ON_RC[8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_GPSSYS_OSC_ON_RC_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RC_EN_MASK 0x00000080                // CONN_INFRA_OSC_RC_EN[7]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RC_EN_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_OSC_RC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_OSC_RC_EN_MASK 0x00000040                // WFSYS_OSC_RC_EN[6]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WFSYS_OSC_RC_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_OSC_RC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_OSC_RC_EN_MASK 0x00000020                // BTSYS_OSC_RC_EN[5]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BTSYS_OSC_RC_EN_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_OSC_RC_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_OSC_RC_EN_MASK 0x00000010                // GPSSYS_OSC_RC_EN[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPSSYS_OSC_RC_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_EN_2X_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_EN_2X_MASK 0x00000008                // OSC_LEGACY_EN_2X[3]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_EN_2X_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_OSC2X_ON_ALL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_OSC2X_ON_ALL_MASK 0x00000004                // SW_OSC2X_ON_ALL[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_OSC2X_ON_ALL_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_OSC_ON_ALL_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_OSC_ON_ALL_MASK 0x00000002                // SW_OSC_ON_ALL[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_SW_OSC_ON_ALL_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_EN_MASK 0x00000001                // OSC_LEGACY_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_OSC_LEGACY_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_STATUS (0x18001000 + 0x344)---

    GPSSYS_OSC_ON_D1[0]          - (RO)  xxx 
    BTSYS_OSC_ON_D1[1]           - (RO)  xxx 
    WFSYS_OSC_ON_D1[2]           - (RO)  xxx 
    CONN_INFRA_OSC_ON_D1[3]      - (RO)  xxx 
    CONN_SRCCLKENA_0_BBLPM[4]    - (RO)  xxx 
    CONN_SRCCLKENA_0_FPM[5]      - (RO)  xxx 
    CONN_SRCCLKENA_0_BBLPM_ACK[6] - (RO)  xxx 
    CONN_SRCCLKENA_0_FPM_ACK[7]  - (RO)  xxx 
    CONN_SRCCLKENA_1_BBLPM[8]    - (RO)  xxx 
    CONN_SRCCLKENA_1_FPM[9]      - (RO)  xxx 
    CONN_SRCCLKENA_1_BBLPM_ACK[10] - (RO)  xxx 
    CONN_SRCCLKENA_1_FPM_ACK[11] - (RO)  xxx 
    CONN_SRCCLKENA_2_BBLPM[12]   - (RO)  xxx 
    CONN_SRCCLKENA_2_FPM[13]     - (RO)  xxx 
    CONN_SRCCLKENA_2_BBLPM_ACK[14] - (RO)  xxx 
    CONN_SRCCLKENA_2_FPM_ACK[15] - (RO)  xxx 
    CONN_SRCCLKENA_3_BBLPM[16]   - (RO)  xxx 
    CONN_SRCCLKENA_3_FPM[17]     - (RO)  xxx 
    CONN_SRCCLKENA_3_BBLPM_ACK[18] - (RO)  xxx 
    CONN_SRCCLKENA_3_FPM_ACK[19] - (RO)  xxx 
    GPSSYS_OSC_ON_ACK[20]        - (RO)  xxx 
    RESERVED21[21]               - (RO) Reserved bits
    BTSYS_OSC_ON_ACK[22]         - (RO)  xxx 
    RESERVED23[23]               - (RO) Reserved bits
    WFSYS_OSC_ON_ACK[24]         - (RO)  xxx 
    RESERVED25[25]               - (RO) Reserved bits
    CONN_INFRA_OSC_ON_ACK[26]    - (RO)  xxx 
    RESERVED27[27]               - (RO) Reserved bits
    CONN_INFRA_OSC_RDY[28]       - (RO)  xxx 
    RESERVED29[31..29]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_RDY_MASK 0x10000000                // CONN_INFRA_OSC_RDY[28]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_RDY_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_ON_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_ON_ACK_MASK 0x04000000                // CONN_INFRA_OSC_ON_ACK[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_ON_ACK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_WFSYS_OSC_ON_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_WFSYS_OSC_ON_ACK_MASK 0x01000000                // WFSYS_OSC_ON_ACK[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_WFSYS_OSC_ON_ACK_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_BTSYS_OSC_ON_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_BTSYS_OSC_ON_ACK_MASK 0x00400000                // BTSYS_OSC_ON_ACK[22]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_BTSYS_OSC_ON_ACK_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_GPSSYS_OSC_ON_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_GPSSYS_OSC_ON_ACK_MASK 0x00100000                // GPSSYS_OSC_ON_ACK[20]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_GPSSYS_OSC_ON_ACK_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_FPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_FPM_ACK_MASK 0x00080000                // CONN_SRCCLKENA_3_FPM_ACK[19]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_FPM_ACK_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_BBLPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_BBLPM_ACK_MASK 0x00040000                // CONN_SRCCLKENA_3_BBLPM_ACK[18]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_BBLPM_ACK_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_FPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_FPM_MASK 0x00020000                // CONN_SRCCLKENA_3_FPM[17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_FPM_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_BBLPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_BBLPM_MASK 0x00010000                // CONN_SRCCLKENA_3_BBLPM[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_3_BBLPM_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_FPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_FPM_ACK_MASK 0x00008000                // CONN_SRCCLKENA_2_FPM_ACK[15]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_FPM_ACK_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_BBLPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_BBLPM_ACK_MASK 0x00004000                // CONN_SRCCLKENA_2_BBLPM_ACK[14]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_BBLPM_ACK_SHFT 14
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_FPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_FPM_MASK 0x00002000                // CONN_SRCCLKENA_2_FPM[13]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_FPM_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_BBLPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_BBLPM_MASK 0x00001000                // CONN_SRCCLKENA_2_BBLPM[12]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_2_BBLPM_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_FPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_FPM_ACK_MASK 0x00000800                // CONN_SRCCLKENA_1_FPM_ACK[11]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_FPM_ACK_SHFT 11
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_BBLPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_BBLPM_ACK_MASK 0x00000400                // CONN_SRCCLKENA_1_BBLPM_ACK[10]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_BBLPM_ACK_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_FPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_FPM_MASK 0x00000200                // CONN_SRCCLKENA_1_FPM[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_FPM_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_BBLPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_BBLPM_MASK 0x00000100                // CONN_SRCCLKENA_1_BBLPM[8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_1_BBLPM_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_FPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_FPM_ACK_MASK 0x00000080                // CONN_SRCCLKENA_0_FPM_ACK[7]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_FPM_ACK_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_BBLPM_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_BBLPM_ACK_MASK 0x00000040                // CONN_SRCCLKENA_0_BBLPM_ACK[6]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_BBLPM_ACK_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_FPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_FPM_MASK 0x00000020                // CONN_SRCCLKENA_0_FPM[5]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_FPM_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_BBLPM_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_BBLPM_MASK 0x00000010                // CONN_SRCCLKENA_0_BBLPM[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_SRCCLKENA_0_BBLPM_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_ON_D1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_ON_D1_MASK 0x00000008                // CONN_INFRA_OSC_ON_D1[3]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_CONN_INFRA_OSC_ON_D1_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_WFSYS_OSC_ON_D1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_WFSYS_OSC_ON_D1_MASK 0x00000004                // WFSYS_OSC_ON_D1[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_WFSYS_OSC_ON_D1_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_BTSYS_OSC_ON_D1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_BTSYS_OSC_ON_D1_MASK 0x00000002                // BTSYS_OSC_ON_D1[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_BTSYS_OSC_ON_D1_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_GPSSYS_OSC_ON_D1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_GPSSYS_OSC_ON_D1_MASK 0x00000001                // GPSSYS_OSC_ON_D1[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_GPSSYS_OSC_ON_D1_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_1 (0x18001000 + 0x348)---

    GPSSYS_OSC_RC_OUT_EN[0]      - (RW) gpssys osc RC control output enable
    BTSYS_OSC_RC_OUT_EN[1]       - (RW) btsys osc RC control output enable
    WFSYS_OSC_RC_OUT_EN[2]       - (RW) wfsys osc RC control output enable
    CONN_INFRA_OSC_RC_OUT_EN[3]  - (RW) conn_infra osc RC control output enable
    OSC_EN_MASK_RC_BK[8..4]      - (RW) OSC_EN_MASK_RC_BK[3]: set 1'b1 to mask "FMSYS_OSC_ON" (from conn_infra_cfg_reg)
                                     OSC_EN_MASK_RC_BK[2]: set 1'b1 to mask "GPSSYS_OSC_ON" (from gpssys)
                                     OSC_EN_MASK_RC_BK[1]: set 1'b1 to mask "BGFSYS_OSC_ON" (from bgfsys)
                                     OSC_EN_MASK_RC_BK[0]: set 1'b1 to mask "WFSYS_OSC_ON" (from wfsys)
    RC_RSV[15..9]                - (RW) reserved CR for RC_CTL
    CONN_INFRA_FORCE_VCORE_ON[16] - (RW) conn_infra_force_vcore_on
                                     1'b1: force vocre_on
    WFSYS_OSC_ON_ACK_W_RS[17]    - (RW) wfsys_osc_on_ack with conn_infra restore done
    BTSYS_OSC_ON_ACK_W_RS[18]    - (RW) bgfsys_osc_on_ack with conn_infra restore done
    GPSSYS_OSC_ON_ACK_W_RS[19]   - (RW) gpssys_osc_on_ack with conn_infra restore done
    EMI_RC_EN[20]                - (RW) conn_emi_rc_en
                                     1'b1: conn_srcclkena_rc = conn_srcclkena_emi(from conn_infra_emi_ctl)
                                     1'b0: conn_srcclkena_rc = conn_srcclkena(from conn_infra_cfg_on)
    EMI_RC_BK_EN[21]             - (RW) conn_emi_rc_bk_en
                                     1'b1: conn_srcclkena = conn_srcclkena_rc | conn_srcclkena_emi
                                     1'b0: conn_srcclkena_rc
    RESERVED22[31..22]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_EMI_RC_BK_EN_ADDR  CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_EMI_RC_BK_EN_MASK  0x00200000                // EMI_RC_BK_EN[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_EMI_RC_BK_EN_SHFT  21
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_EMI_RC_EN_ADDR     CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_EMI_RC_EN_MASK     0x00100000                // EMI_RC_EN[20]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_EMI_RC_EN_SHFT     20
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPSSYS_OSC_ON_ACK_W_RS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPSSYS_OSC_ON_ACK_W_RS_MASK 0x00080000                // GPSSYS_OSC_ON_ACK_W_RS[19]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPSSYS_OSC_ON_ACK_W_RS_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BTSYS_OSC_ON_ACK_W_RS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BTSYS_OSC_ON_ACK_W_RS_MASK 0x00040000                // BTSYS_OSC_ON_ACK_W_RS[18]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BTSYS_OSC_ON_ACK_W_RS_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WFSYS_OSC_ON_ACK_W_RS_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WFSYS_OSC_ON_ACK_W_RS_MASK 0x00020000                // WFSYS_OSC_ON_ACK_W_RS[17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WFSYS_OSC_ON_ACK_W_RS_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_CONN_INFRA_FORCE_VCORE_ON_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_CONN_INFRA_FORCE_VCORE_ON_MASK 0x00010000                // CONN_INFRA_FORCE_VCORE_ON[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_CONN_INFRA_FORCE_VCORE_ON_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_RC_RSV_ADDR        CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_RC_RSV_MASK        0x0000FE00                // RC_RSV[15..9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_RC_RSV_SHFT        9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_OSC_EN_MASK_RC_BK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_OSC_EN_MASK_RC_BK_MASK 0x000001F0                // OSC_EN_MASK_RC_BK[8..4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_OSC_EN_MASK_RC_BK_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_CONN_INFRA_OSC_RC_OUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_CONN_INFRA_OSC_RC_OUT_EN_MASK 0x00000008                // CONN_INFRA_OSC_RC_OUT_EN[3]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_CONN_INFRA_OSC_RC_OUT_EN_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WFSYS_OSC_RC_OUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WFSYS_OSC_RC_OUT_EN_MASK 0x00000004                // WFSYS_OSC_RC_OUT_EN[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WFSYS_OSC_RC_OUT_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BTSYS_OSC_RC_OUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BTSYS_OSC_RC_OUT_EN_MASK 0x00000002                // BTSYS_OSC_RC_OUT_EN[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BTSYS_OSC_RC_OUT_EN_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPSSYS_OSC_RC_OUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPSSYS_OSC_RC_OUT_EN_MASK 0x00000001                // GPSSYS_OSC_RC_OUT_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPSSYS_OSC_RC_OUT_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_0_GPS (0x18001000 + 0x350)---

    SWCTL_CONN_SRCCLKENA_0[0]    - (RW) conn_srcclkena_0 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_BG_0[1]      - (RW) da_wbg_en_bg_0 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_XBUF_0[2]    - (RW) da_wbg_en_xbuf_0 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RESERVED3[3]                 - (RO) Reserved bits
    SWCTL_OSC_RDY_0[4]           - (RW) osc_rdy_0 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RC_GPS_RSV[7..5]             - (RW) reserved CR for RC_GPS
    SW_CONN_SRCCLKENA_0[8]       - (RW) sw control conn_srcclkena_0
    SW_DA_WBG_EN_BG_0[9]         - (RW) sw control da_wbg_en_bg_0
    SW_DA_WBG_EN_XBUF_0[10]      - (RW) sw control da_wbg_en_xbuf_0
    RESERVED11[11]               - (RO) Reserved bits
    SW_OSC_RDY_0[12]             - (RW) sw control osc_rdy_0
    GPS_XTAL_RDY_EN_MASK[13]     - (RW) osc_on_ack gps fake ack filter mask
                                     1'b0: osc_on_ack filter by gps fake ack filter
                                     1'b1: osc_on_ack bypass gps fake ack filter
    GPS_OSC_ON_ACK_EN_MASK[14]   - (RW) XTAL_RDY gps fake ack filter mask
                                     1'b0: XTAL_RDY filter by gps fake ack filter
                                     1'b1: XTAL_RDY bypass gps fake ack filter
    ACK_FOR_XO_STATE_MASK_0[15]  - (RW) mask conn_srcclkena_0_bblpm/fpm_ack for xo_state
                                     1'b0: check conn_srcclkena_0_bblpm/fpm_ack
                                     1'b1: mask conn_srcclkena_0_bblpm/fpm_ack
    XO_EN_0[16]                  - (RO)  xxx 
    XO_STATE_0[20..17]           - (RO)  xxx 
    HW_CONN_SRCCLKENA_0[21]      - (RO)  xxx 
    RESERVED22[22]               - (RO) Reserved bits
    HW_DA_WBG_EN_BG_0[23]        - (RO)  xxx 
    HW_DA_WBG_EN_XBUF_0[24]      - (RO)  xxx 
    HW_OSC_RDY_0[25]             - (RO)  xxx 
    GPS_OSC_ACK_BP_PWR_ACK[26]   - (RW) gpsys_osc_on_ack Flip-Flop reset bypass conn_infra_off_active
                                     1'b0:  check conn_infra_off_active
                                     1'b1: bypass conn_infra_off_active
    GPS_OSC_ACK_BP_MEM_PON[27]   - (RW) gpsys_osc_on_ack Flip-Flop reset bypass conn_infra_mem_rst_b
                                     1'b0:  check conn_infra_mem_rst_b
                                     1'b1: bypass conn_infra_mem_rst_b
    GPS_XTAL_FILTER_TIMEOUT[31..28] - (RW) gps fake ack filter timeout value

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_XTAL_FILTER_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_XTAL_FILTER_TIMEOUT_MASK 0xF0000000                // GPS_XTAL_FILTER_TIMEOUT[31..28]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_XTAL_FILTER_TIMEOUT_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ACK_BP_MEM_PON_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ACK_BP_MEM_PON_MASK 0x08000000                // GPS_OSC_ACK_BP_MEM_PON[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ACK_BP_MEM_PON_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ACK_BP_PWR_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ACK_BP_PWR_ACK_MASK 0x04000000                // GPS_OSC_ACK_BP_PWR_ACK[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ACK_BP_PWR_ACK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_OSC_RDY_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_OSC_RDY_0_MASK 0x02000000                // HW_OSC_RDY_0[25]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_OSC_RDY_0_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_DA_WBG_EN_XBUF_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_DA_WBG_EN_XBUF_0_MASK 0x01000000                // HW_DA_WBG_EN_XBUF_0[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_DA_WBG_EN_XBUF_0_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_DA_WBG_EN_BG_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_DA_WBG_EN_BG_0_MASK 0x00800000                // HW_DA_WBG_EN_BG_0[23]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_DA_WBG_EN_BG_0_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_CONN_SRCCLKENA_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_CONN_SRCCLKENA_0_MASK 0x00200000                // HW_CONN_SRCCLKENA_0[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_HW_CONN_SRCCLKENA_0_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_XO_STATE_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_XO_STATE_0_MASK 0x001E0000                // XO_STATE_0[20..17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_XO_STATE_0_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_XO_EN_0_ADDR   CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_XO_EN_0_MASK   0x00010000                // XO_EN_0[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_XO_EN_0_SHFT   16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ACK_FOR_XO_STATE_MASK_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ACK_FOR_XO_STATE_MASK_0_MASK 0x00008000                // ACK_FOR_XO_STATE_MASK_0[15]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ACK_FOR_XO_STATE_MASK_0_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ON_ACK_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ON_ACK_EN_MASK_MASK 0x00004000                // GPS_OSC_ON_ACK_EN_MASK[14]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_OSC_ON_ACK_EN_MASK_SHFT 14
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_XTAL_RDY_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_XTAL_RDY_EN_MASK_MASK 0x00002000                // GPS_XTAL_RDY_EN_MASK[13]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_GPS_XTAL_RDY_EN_MASK_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_OSC_RDY_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_OSC_RDY_0_MASK 0x00001000                // SW_OSC_RDY_0[12]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_OSC_RDY_0_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_DA_WBG_EN_XBUF_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_DA_WBG_EN_XBUF_0_MASK 0x00000400                // SW_DA_WBG_EN_XBUF_0[10]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_DA_WBG_EN_XBUF_0_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_DA_WBG_EN_BG_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_DA_WBG_EN_BG_0_MASK 0x00000200                // SW_DA_WBG_EN_BG_0[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_DA_WBG_EN_BG_0_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_CONN_SRCCLKENA_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_CONN_SRCCLKENA_0_MASK 0x00000100                // SW_CONN_SRCCLKENA_0[8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SW_CONN_SRCCLKENA_0_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_RC_GPS_RSV_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_RC_GPS_RSV_MASK 0x000000E0                // RC_GPS_RSV[7..5]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_RC_GPS_RSV_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_OSC_RDY_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_OSC_RDY_0_MASK 0x00000010                // SWCTL_OSC_RDY_0[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_OSC_RDY_0_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_DA_WBG_EN_XBUF_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_DA_WBG_EN_XBUF_0_MASK 0x00000004                // SWCTL_DA_WBG_EN_XBUF_0[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_DA_WBG_EN_XBUF_0_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_DA_WBG_EN_BG_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_DA_WBG_EN_BG_0_MASK 0x00000002                // SWCTL_DA_WBG_EN_BG_0[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_DA_WBG_EN_BG_0_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_CONN_SRCCLKENA_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_CONN_SRCCLKENA_0_MASK 0x00000001                // SWCTL_CONN_SRCCLKENA_0[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_GPS_SWCTL_CONN_SRCCLKENA_0_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_1_GPS (0x18001000 + 0x354)---

    XO_XTAL_RDY_STABLE_TIME_0[7..0] - (RW) xo_XTAL_RDY_stable_time_0 for conn_srcclkena_0_ack valid
    XO_INI_STABLE_TIME_0[15..8]  - (RW) xo_ini_stable_time_0 for xo_ini stable
    XO_BG_STABLE_TIME_0[23..16]  - (RW) xo_bg_stable_time_0 for xo_bg stable
    XO_XTAL_OFF_STABLE_TIME_0[31..24] - (RW) xo_xtal_off_stable_time_0 for conn_srcclkena_0_ack high to low

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_XTAL_OFF_STABLE_TIME_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_XTAL_OFF_STABLE_TIME_0_MASK 0xFF000000                // XO_XTAL_OFF_STABLE_TIME_0[31..24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_XTAL_OFF_STABLE_TIME_0_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_BG_STABLE_TIME_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_BG_STABLE_TIME_0_MASK 0x00FF0000                // XO_BG_STABLE_TIME_0[23..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_BG_STABLE_TIME_0_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_INI_STABLE_TIME_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_INI_STABLE_TIME_0_MASK 0x0000FF00                // XO_INI_STABLE_TIME_0[15..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_INI_STABLE_TIME_0_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_XTAL_RDY_STABLE_TIME_0_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_XTAL_RDY_STABLE_TIME_0_MASK 0x000000FF                // XO_XTAL_RDY_STABLE_TIME_0[7..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_GPS_XO_XTAL_RDY_STABLE_TIME_0_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_0_BT (0x18001000 + 0x360)---

    SWCTL_CONN_SRCCLKENA_1[0]    - (RW) conn_srcclkena_1 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_BG_1[1]      - (RW) da_wbg_en_bg_1 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_XBUF_1[2]    - (RW) da_wbg_en_xbuf_1 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RESERVED3[3]                 - (RO) Reserved bits
    SWCTL_OSC_RDY_1[4]           - (RW) osc_rdy_1 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RC_BT_RSV[7..5]              - (RW) reserved CR for RC_BT
    SW_CONN_SRCCLKENA_1[8]       - (RW) sw control conn_srcclkena_1
    SW_DA_WBG_EN_BG_1[9]         - (RW) sw control da_wbg_en_bg_1
    SW_DA_WBG_EN_XBUF_1[10]      - (RW) sw control da_wbg_en_xbuf_1
    RESERVED11[11]               - (RO) Reserved bits
    SW_OSC_RDY_1[12]             - (RW) sw control osc_rdy_1
    BT_XTAL_RDY_EN_MASK[13]      - (RW) osc_on_ack bt fake ack filter mask
                                     1'b0: osc_on_ack filter by bt fake ack filter
                                     1'b1: osc_on_ack bypass bt fake ack filter
    BT_OSC_ON_ACK_EN_MASK[14]    - (RW) XTAL_RDY bt fake ack filter mask
                                     1'b0: XTAL_RDY filter by bt fake ack filter
                                     1'b1: XTAL_RDY bypass bt fake ack filter
    ACK_FOR_XO_STATE_MASK_1[15]  - (RW) mask conn_srcclkena_1_bblpm/fpm_ack for xo_state
                                     1'b0: check conn_srcclkena_1_bblpm/fpm_ack
                                     1'b1: mask conn_srcclkena_1_bblpm/fpm_ack
    XO_EN_1[16]                  - (RO)  xxx 
    XO_STATE_1[20..17]           - (RO)  xxx 
    HW_CONN_SRCCLKENA_1[21]      - (RO)  xxx 
    RESERVED22[22]               - (RO) Reserved bits
    HW_DA_WBG_EN_BG_1[23]        - (RO)  xxx 
    HW_DA_WBG_EN_XBUF_1[24]      - (RO)  xxx 
    HW_OSC_RDY_1[25]             - (RO)  xxx 
    BT_OSC_ACK_BP_PWR_ACK[26]    - (RW) btsys_osc_on_ack Flip-Flop reset bypass conn_infra_off_active
                                     1'b0:  check conn_infra_off_active
                                     1'b1: bypass conn_infra_off_active
    BT_OSC_ACK_BP_MEM_PON[27]    - (RW) btsys_osc_on_ack Flip-Flop reset bypass conn_infra_mem_rst_b
                                     1'b0:  check conn_infra_mem_rst_b
                                     1'b1: bypass conn_infra_mem_rst_b
    BT_XTAL_FILTER_TIMEOUT[31..28] - (RW) bt fake ack filter timeout value

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_XTAL_FILTER_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_XTAL_FILTER_TIMEOUT_MASK 0xF0000000                // BT_XTAL_FILTER_TIMEOUT[31..28]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_XTAL_FILTER_TIMEOUT_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ACK_BP_MEM_PON_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ACK_BP_MEM_PON_MASK 0x08000000                // BT_OSC_ACK_BP_MEM_PON[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ACK_BP_MEM_PON_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ACK_BP_PWR_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ACK_BP_PWR_ACK_MASK 0x04000000                // BT_OSC_ACK_BP_PWR_ACK[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ACK_BP_PWR_ACK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_OSC_RDY_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_OSC_RDY_1_MASK 0x02000000                // HW_OSC_RDY_1[25]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_OSC_RDY_1_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_DA_WBG_EN_XBUF_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_DA_WBG_EN_XBUF_1_MASK 0x01000000                // HW_DA_WBG_EN_XBUF_1[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_DA_WBG_EN_XBUF_1_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_DA_WBG_EN_BG_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_DA_WBG_EN_BG_1_MASK 0x00800000                // HW_DA_WBG_EN_BG_1[23]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_DA_WBG_EN_BG_1_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_CONN_SRCCLKENA_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_CONN_SRCCLKENA_1_MASK 0x00200000                // HW_CONN_SRCCLKENA_1[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_HW_CONN_SRCCLKENA_1_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_XO_STATE_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_XO_STATE_1_MASK 0x001E0000                // XO_STATE_1[20..17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_XO_STATE_1_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_XO_EN_1_ADDR    CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_XO_EN_1_MASK    0x00010000                // XO_EN_1[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_XO_EN_1_SHFT    16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ACK_FOR_XO_STATE_MASK_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ACK_FOR_XO_STATE_MASK_1_MASK 0x00008000                // ACK_FOR_XO_STATE_MASK_1[15]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ACK_FOR_XO_STATE_MASK_1_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ON_ACK_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ON_ACK_EN_MASK_MASK 0x00004000                // BT_OSC_ON_ACK_EN_MASK[14]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_OSC_ON_ACK_EN_MASK_SHFT 14
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_XTAL_RDY_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_XTAL_RDY_EN_MASK_MASK 0x00002000                // BT_XTAL_RDY_EN_MASK[13]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_BT_XTAL_RDY_EN_MASK_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_OSC_RDY_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_OSC_RDY_1_MASK 0x00001000                // SW_OSC_RDY_1[12]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_OSC_RDY_1_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_DA_WBG_EN_XBUF_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_DA_WBG_EN_XBUF_1_MASK 0x00000400                // SW_DA_WBG_EN_XBUF_1[10]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_DA_WBG_EN_XBUF_1_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_DA_WBG_EN_BG_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_DA_WBG_EN_BG_1_MASK 0x00000200                // SW_DA_WBG_EN_BG_1[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_DA_WBG_EN_BG_1_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_CONN_SRCCLKENA_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_CONN_SRCCLKENA_1_MASK 0x00000100                // SW_CONN_SRCCLKENA_1[8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SW_CONN_SRCCLKENA_1_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_RC_BT_RSV_ADDR  CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_RC_BT_RSV_MASK  0x000000E0                // RC_BT_RSV[7..5]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_RC_BT_RSV_SHFT  5
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_OSC_RDY_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_OSC_RDY_1_MASK 0x00000010                // SWCTL_OSC_RDY_1[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_OSC_RDY_1_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_DA_WBG_EN_XBUF_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_DA_WBG_EN_XBUF_1_MASK 0x00000004                // SWCTL_DA_WBG_EN_XBUF_1[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_DA_WBG_EN_XBUF_1_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_DA_WBG_EN_BG_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_DA_WBG_EN_BG_1_MASK 0x00000002                // SWCTL_DA_WBG_EN_BG_1[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_DA_WBG_EN_BG_1_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_CONN_SRCCLKENA_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_CONN_SRCCLKENA_1_MASK 0x00000001                // SWCTL_CONN_SRCCLKENA_1[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_BT_SWCTL_CONN_SRCCLKENA_1_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_1_BT (0x18001000 + 0x364)---

    XO_XTAL_RDY_STABLE_TIME_1[7..0] - (RW) xo_XTAL_RDY_stable_time_1 for conn_srcclkena_1_ack valid
    XO_INI_STABLE_TIME_1[15..8]  - (RW) xo_ini_stable_time_1 for xo_ini stable
    XO_BG_STABLE_TIME_1[23..16]  - (RW) xo_bg_stable_time_1 for xo_bg stable
    XO_XTAL_OFF_STABLE_TIME_1[31..24] - (RW) xo_xtal_off_stable_time_1 for conn_srcclkena_1_ack high to low

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_XTAL_OFF_STABLE_TIME_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_XTAL_OFF_STABLE_TIME_1_MASK 0xFF000000                // XO_XTAL_OFF_STABLE_TIME_1[31..24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_XTAL_OFF_STABLE_TIME_1_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_BG_STABLE_TIME_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_BG_STABLE_TIME_1_MASK 0x00FF0000                // XO_BG_STABLE_TIME_1[23..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_BG_STABLE_TIME_1_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_INI_STABLE_TIME_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_INI_STABLE_TIME_1_MASK 0x0000FF00                // XO_INI_STABLE_TIME_1[15..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_INI_STABLE_TIME_1_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_XTAL_RDY_STABLE_TIME_1_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_XTAL_RDY_STABLE_TIME_1_MASK 0x000000FF                // XO_XTAL_RDY_STABLE_TIME_1[7..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_BT_XO_XTAL_RDY_STABLE_TIME_1_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_0_WF (0x18001000 + 0x370)---

    SWCTL_CONN_SRCCLKENA_2[0]    - (RW) conn_srcclkena_2 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_BG_2[1]      - (RW) da_wbg_en_bg_2 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_XBUF_2[2]    - (RW) da_wbg_en_xbuf_2 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RESERVED3[3]                 - (RO) Reserved bits
    SWCTL_OSC_RDY_2[4]           - (RW) osc_rdy_2 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RC_WF_RSV[7..5]              - (RW) reserved CR for RC_WF
    SW_CONN_SRCCLKENA_2[8]       - (RW) sw control conn_srcclkena_2
    SW_DA_WBG_EN_BG_2[9]         - (RW) sw control da_wbg_en_bg_2
    SW_DA_WBG_EN_XBUF_2[10]      - (RW) sw control da_wbg_en_xbuf_2
    RESERVED11[11]               - (RO) Reserved bits
    SW_OSC_RDY_2[12]             - (RW) sw control osc_rdy_2
    WF_XTAL_RDY_EN_MASK[13]      - (RW) osc_on_ack wf fake ack filter mask
                                     1'b0: osc_on_ack filter by wf fake ack filter
                                     1'b1: osc_on_ack bypass wf fake ack filter
    WF_OSC_ON_ACK_EN_MASK[14]    - (RW) XTAL_RDY wf fake ack filter mask
                                     1'b0: XTAL_RDY filter by wf fake ack filter
                                     1'b1: XTAL_RDY bypass wf fake ack filter
    ACK_FOR_XO_STATE_MASK_2[15]  - (RW) mask conn_srcclkena_2_bblpm/fpm_ack for xo_state
                                     1'b0: check conn_srcclkena_2_bblpm/fpm_ack
                                     1'b1: mask conn_srcclkena_2_bblpm/fpm_ack
    XO_EN_2[16]                  - (RO)  xxx 
    XO_STATE_2[20..17]           - (RO)  xxx 
    HW_CONN_SRCCLKENA_2[21]      - (RO)  xxx 
    RESERVED22[22]               - (RO) Reserved bits
    HW_DA_WBG_EN_BG_2[23]        - (RO)  xxx 
    HW_DA_WBG_EN_XBUF_2[24]      - (RO)  xxx 
    HW_OSC_RDY_2[25]             - (RO)  xxx 
    WF_OSC_ACK_BP_PWR_ACK[26]    - (RW) wfsys_osc_on_ack Flip-Flop reset bypass conn_infra_off_active
                                     1'b0:  check conn_infra_off_active
                                     1'b1: bypass conn_infra_off_active
    WF_OSC_ACK_BP_MEM_PON[27]    - (RW) wfsys_osc_on_ack Flip-Flop reset bypass conn_infra_mem_rst_b
                                     1'b0:  check conn_infra_mem_rst_b
                                     1'b1: bypass conn_infra_mem_rst_b
    WF_XTAL_FILTER_TIMEOUT[31..28] - (RW) wf fake ack filter timeout value

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_XTAL_FILTER_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_XTAL_FILTER_TIMEOUT_MASK 0xF0000000                // WF_XTAL_FILTER_TIMEOUT[31..28]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_XTAL_FILTER_TIMEOUT_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ACK_BP_MEM_PON_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ACK_BP_MEM_PON_MASK 0x08000000                // WF_OSC_ACK_BP_MEM_PON[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ACK_BP_MEM_PON_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ACK_BP_PWR_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ACK_BP_PWR_ACK_MASK 0x04000000                // WF_OSC_ACK_BP_PWR_ACK[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ACK_BP_PWR_ACK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_OSC_RDY_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_OSC_RDY_2_MASK 0x02000000                // HW_OSC_RDY_2[25]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_OSC_RDY_2_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_DA_WBG_EN_XBUF_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_DA_WBG_EN_XBUF_2_MASK 0x01000000                // HW_DA_WBG_EN_XBUF_2[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_DA_WBG_EN_XBUF_2_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_DA_WBG_EN_BG_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_DA_WBG_EN_BG_2_MASK 0x00800000                // HW_DA_WBG_EN_BG_2[23]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_DA_WBG_EN_BG_2_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_CONN_SRCCLKENA_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_CONN_SRCCLKENA_2_MASK 0x00200000                // HW_CONN_SRCCLKENA_2[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_HW_CONN_SRCCLKENA_2_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_XO_STATE_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_XO_STATE_2_MASK 0x001E0000                // XO_STATE_2[20..17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_XO_STATE_2_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_XO_EN_2_ADDR    CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_XO_EN_2_MASK    0x00010000                // XO_EN_2[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_XO_EN_2_SHFT    16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ACK_FOR_XO_STATE_MASK_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ACK_FOR_XO_STATE_MASK_2_MASK 0x00008000                // ACK_FOR_XO_STATE_MASK_2[15]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ACK_FOR_XO_STATE_MASK_2_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ON_ACK_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ON_ACK_EN_MASK_MASK 0x00004000                // WF_OSC_ON_ACK_EN_MASK[14]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_OSC_ON_ACK_EN_MASK_SHFT 14
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_XTAL_RDY_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_XTAL_RDY_EN_MASK_MASK 0x00002000                // WF_XTAL_RDY_EN_MASK[13]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_WF_XTAL_RDY_EN_MASK_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_OSC_RDY_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_OSC_RDY_2_MASK 0x00001000                // SW_OSC_RDY_2[12]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_OSC_RDY_2_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_DA_WBG_EN_XBUF_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_DA_WBG_EN_XBUF_2_MASK 0x00000400                // SW_DA_WBG_EN_XBUF_2[10]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_DA_WBG_EN_XBUF_2_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_DA_WBG_EN_BG_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_DA_WBG_EN_BG_2_MASK 0x00000200                // SW_DA_WBG_EN_BG_2[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_DA_WBG_EN_BG_2_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_CONN_SRCCLKENA_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_CONN_SRCCLKENA_2_MASK 0x00000100                // SW_CONN_SRCCLKENA_2[8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SW_CONN_SRCCLKENA_2_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_RC_WF_RSV_ADDR  CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_RC_WF_RSV_MASK  0x000000E0                // RC_WF_RSV[7..5]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_RC_WF_RSV_SHFT  5
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_OSC_RDY_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_OSC_RDY_2_MASK 0x00000010                // SWCTL_OSC_RDY_2[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_OSC_RDY_2_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_DA_WBG_EN_XBUF_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_DA_WBG_EN_XBUF_2_MASK 0x00000004                // SWCTL_DA_WBG_EN_XBUF_2[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_DA_WBG_EN_XBUF_2_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_DA_WBG_EN_BG_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_DA_WBG_EN_BG_2_MASK 0x00000002                // SWCTL_DA_WBG_EN_BG_2[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_DA_WBG_EN_BG_2_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_CONN_SRCCLKENA_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_CONN_SRCCLKENA_2_MASK 0x00000001                // SWCTL_CONN_SRCCLKENA_2[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_WF_SWCTL_CONN_SRCCLKENA_2_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_1_WF (0x18001000 + 0x374)---

    XO_XTAL_RDY_STABLE_TIME_2[7..0] - (RW) xo_XTAL_RDY_stable_time_2 for conn_srcclkena_2_ack valid
    XO_INI_STABLE_TIME_2[15..8]  - (RW) xo_ini_stable_time_2 for xo_ini stable
    XO_BG_STABLE_TIME_2[23..16]  - (RW) xo_bg_stable_time_2 for xo_bg stable
    XO_XTAL_OFF_STABLE_TIME_2[31..24] - (RW) xo_xtal_off_stable_time_2 for conn_srcclkena_2_ack high to low

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_XTAL_OFF_STABLE_TIME_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_XTAL_OFF_STABLE_TIME_2_MASK 0xFF000000                // XO_XTAL_OFF_STABLE_TIME_2[31..24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_XTAL_OFF_STABLE_TIME_2_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_BG_STABLE_TIME_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_BG_STABLE_TIME_2_MASK 0x00FF0000                // XO_BG_STABLE_TIME_2[23..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_BG_STABLE_TIME_2_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_INI_STABLE_TIME_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_INI_STABLE_TIME_2_MASK 0x0000FF00                // XO_INI_STABLE_TIME_2[15..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_INI_STABLE_TIME_2_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_XTAL_RDY_STABLE_TIME_2_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_XTAL_RDY_STABLE_TIME_2_MASK 0x000000FF                // XO_XTAL_RDY_STABLE_TIME_2[7..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_WF_XO_XTAL_RDY_STABLE_TIME_2_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_0_TOP (0x18001000 + 0x380)---

    SWCTL_CONN_SRCCLKENA_3[0]    - (RW) conn_srcclkena_3 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_BG_3[1]      - (RW) da_wbg_en_bg_3 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    SWCTL_DA_WBG_EN_XBUF_3[2]    - (RW) da_wbg_en_xbuf_3 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RESERVED3[3]                 - (RO) Reserved bits
    SWCTL_OSC_RDY_3[4]           - (RW) osc_rdy_3 control selection
                                     1'b0: hw control
                                     1'b1: sw control
    RC_TOP_RSV[7..5]             - (RW) reserved CR for RC_TOP
    SW_CONN_SRCCLKENA_3[8]       - (RW) sw control conn_srcclkena_3
    SW_DA_WBG_EN_BG_3[9]         - (RW) sw control da_wbg_en_bg_3
    SW_DA_WBG_EN_XBUF_3[10]      - (RW) sw control da_wbg_en_xbuf_3
    RESERVED11[11]               - (RO) Reserved bits
    SW_OSC_RDY_3[12]             - (RW) sw control osc_rdy_3
    TOP_XTAL_RDY_EN_MASK[13]     - (RW) osc_on_ack conn_infra fake ack filter mask
                                     1'b0: osc_on_ack filter by conn_infra fake ack filter
                                     1'b1: osc_on_ack bypass conn_infra fake ack filter
    TOP_OSC_ON_ACK_EN_MASK[14]   - (RW) XTAL_RDY conn_infra fake ack filter mask
                                     1'b0: XTAL_RDY filter by conn_infra fake ack filter
                                     1'b1: XTAL_RDY bypass conn_infra fake ack filter
    ACK_FOR_XO_STATE_MASK_3[15]  - (RW) mask conn_srcclkena_3_bblpm/fpm_ack for xo_state
                                     1'b0: check conn_srcclkena_3_bblpm/fpm_ack
                                     1'b1: mask conn_srcclkena_3_bblpm/fpm_ack
    XO_EN_3[16]                  - (RO)  xxx 
    XO_STATE_3[20..17]           - (RO)  xxx 
    HW_CONN_SRCCLKENA_3[21]      - (RO)  xxx 
    RESERVED22[22]               - (RO) Reserved bits
    HW_DA_WBG_EN_BG_3[23]        - (RO)  xxx 
    HW_DA_WBG_EN_XBUF_3[24]      - (RO)  xxx 
    HW_OSC_RDY_3[25]             - (RO)  xxx 
    TOP_OSC_ACK_BP_PWR_ACK[26]   - (RW) conn_infra_osc_on_ack Flip-Flop reset bypass conn_infra_off_active
                                     1'b0:  check conn_infra_off_active
                                     1'b1: bypass conn_infra_off_active
    TOP_OSC_ACK_BP_MEM_PON[27]   - (RW) conn_infra_osc_on_ack Flip-Flop reset bypass conn_infra_mem_rst_b
                                     1'b0:  check conn_infra_mem_rst_b
                                     1'b1: bypass conn_infra_mem_rst_b
    TOP_XTAL_FILTER_TIMEOUT[31..28] - (RW) conn_infra fake ack filter timeout value

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_XTAL_FILTER_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_XTAL_FILTER_TIMEOUT_MASK 0xF0000000                // TOP_XTAL_FILTER_TIMEOUT[31..28]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_XTAL_FILTER_TIMEOUT_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ACK_BP_MEM_PON_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ACK_BP_MEM_PON_MASK 0x08000000                // TOP_OSC_ACK_BP_MEM_PON[27]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ACK_BP_MEM_PON_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ACK_BP_PWR_ACK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ACK_BP_PWR_ACK_MASK 0x04000000                // TOP_OSC_ACK_BP_PWR_ACK[26]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ACK_BP_PWR_ACK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_OSC_RDY_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_OSC_RDY_3_MASK 0x02000000                // HW_OSC_RDY_3[25]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_OSC_RDY_3_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_DA_WBG_EN_XBUF_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_DA_WBG_EN_XBUF_3_MASK 0x01000000                // HW_DA_WBG_EN_XBUF_3[24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_DA_WBG_EN_XBUF_3_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_DA_WBG_EN_BG_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_DA_WBG_EN_BG_3_MASK 0x00800000                // HW_DA_WBG_EN_BG_3[23]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_DA_WBG_EN_BG_3_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_CONN_SRCCLKENA_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_CONN_SRCCLKENA_3_MASK 0x00200000                // HW_CONN_SRCCLKENA_3[21]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_HW_CONN_SRCCLKENA_3_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_XO_STATE_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_XO_STATE_3_MASK 0x001E0000                // XO_STATE_3[20..17]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_XO_STATE_3_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_XO_EN_3_ADDR   CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_XO_EN_3_MASK   0x00010000                // XO_EN_3[16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_XO_EN_3_SHFT   16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ACK_FOR_XO_STATE_MASK_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ACK_FOR_XO_STATE_MASK_3_MASK 0x00008000                // ACK_FOR_XO_STATE_MASK_3[15]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ACK_FOR_XO_STATE_MASK_3_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ON_ACK_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ON_ACK_EN_MASK_MASK 0x00004000                // TOP_OSC_ON_ACK_EN_MASK[14]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_OSC_ON_ACK_EN_MASK_SHFT 14
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_XTAL_RDY_EN_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_XTAL_RDY_EN_MASK_MASK 0x00002000                // TOP_XTAL_RDY_EN_MASK[13]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_TOP_XTAL_RDY_EN_MASK_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_OSC_RDY_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_OSC_RDY_3_MASK 0x00001000                // SW_OSC_RDY_3[12]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_OSC_RDY_3_SHFT 12
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_DA_WBG_EN_XBUF_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_DA_WBG_EN_XBUF_3_MASK 0x00000400                // SW_DA_WBG_EN_XBUF_3[10]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_DA_WBG_EN_XBUF_3_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_DA_WBG_EN_BG_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_DA_WBG_EN_BG_3_MASK 0x00000200                // SW_DA_WBG_EN_BG_3[9]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_DA_WBG_EN_BG_3_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_CONN_SRCCLKENA_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_CONN_SRCCLKENA_3_MASK 0x00000100                // SW_CONN_SRCCLKENA_3[8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SW_CONN_SRCCLKENA_3_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_RC_TOP_RSV_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_RC_TOP_RSV_MASK 0x000000E0                // RC_TOP_RSV[7..5]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_RC_TOP_RSV_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_OSC_RDY_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_OSC_RDY_3_MASK 0x00000010                // SWCTL_OSC_RDY_3[4]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_OSC_RDY_3_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_DA_WBG_EN_XBUF_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_DA_WBG_EN_XBUF_3_MASK 0x00000004                // SWCTL_DA_WBG_EN_XBUF_3[2]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_DA_WBG_EN_XBUF_3_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_DA_WBG_EN_BG_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_DA_WBG_EN_BG_3_MASK 0x00000002                // SWCTL_DA_WBG_EN_BG_3[1]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_DA_WBG_EN_BG_3_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_CONN_SRCCLKENA_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_CONN_SRCCLKENA_3_MASK 0x00000001                // SWCTL_CONN_SRCCLKENA_3[0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_0_TOP_SWCTL_CONN_SRCCLKENA_3_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CFG_RC_CTL_1_TOP (0x18001000 + 0x384)---

    XO_XTAL_RDY_STABLE_TIME_3[7..0] - (RW) xo_XTAL_RDY_stable_time_3 for conn_srcclkena_3_ack valid
    XO_INI_STABLE_TIME_3[15..8]  - (RW) xo_ini_stable_time_3 for xo_ini stable
    XO_BG_STABLE_TIME_3[23..16]  - (RW) xo_bg_stable_time_3 for xo_bg stable
    XO_XTAL_OFF_STABLE_TIME_3[31..24] - (RW) xo_xtal_off_stable_time_3 for conn_srcclkena_3_ack high to low

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_XTAL_OFF_STABLE_TIME_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_XTAL_OFF_STABLE_TIME_3_MASK 0xFF000000                // XO_XTAL_OFF_STABLE_TIME_3[31..24]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_XTAL_OFF_STABLE_TIME_3_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_BG_STABLE_TIME_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_BG_STABLE_TIME_3_MASK 0x00FF0000                // XO_BG_STABLE_TIME_3[23..16]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_BG_STABLE_TIME_3_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_INI_STABLE_TIME_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_INI_STABLE_TIME_3_MASK 0x0000FF00                // XO_INI_STABLE_TIME_3[15..8]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_INI_STABLE_TIME_3_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_XTAL_RDY_STABLE_TIME_3_ADDR CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_XTAL_RDY_STABLE_TIME_3_MASK 0x000000FF                // XO_XTAL_RDY_STABLE_TIME_3[7..0]
#define CONN_CFG_ON_CONN_INFRA_CFG_RC_CTL_1_TOP_XO_XTAL_RDY_STABLE_TIME_3_SHFT 0

/* =====================================================================================

  ---EMI_CTL_0 (0x18001000 + 0x3A0)---

    CONN_AP_BUS_REQ_SW_MODE[0]   - (RW) conn_ap_bus_req sw mode
                                     1'b1: sw mode
                                     1'b0: hw mode
    CONN_APSRC_REQ_SW_MODE[1]    - (RW) conn_apsrc_req sw mode
                                     1'b1: sw mode
                                     1'b0: hw mode
    CONN_DDR_EN_SW_MODE[2]       - (RW) conn_ddr_en sw mode
                                     1'b1: sw mode
                                     1'b0: hw mode
    RESERVED3[3]                 - (RO) Reserved bits
    SW_CONN_AP_BUS_REQ[4]        - (RW) conn_ap_bus_req sw control
    SW_CONN_APSRC_REQ[5]         - (RW) conn_apsrc_req sw control
    SW_CONN_DDR_EN[6]            - (RW) conn_ddr_en sw control
    RESERVED7[7]                 - (RO) Reserved bits
    EMI_CTL_SLPPROT_DIS[8]       - (RW) disable slpprot control for emi_ctl
    RESERVED9[31..9]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_EMI_CTL_0_EMI_CTL_SLPPROT_DIS_ADDR         CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_EMI_CTL_SLPPROT_DIS_MASK         0x00000100                // EMI_CTL_SLPPROT_DIS[8]
#define CONN_CFG_ON_EMI_CTL_0_EMI_CTL_SLPPROT_DIS_SHFT         8
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_DDR_EN_ADDR              CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_DDR_EN_MASK              0x00000040                // SW_CONN_DDR_EN[6]
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_DDR_EN_SHFT              6
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_APSRC_REQ_ADDR           CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_APSRC_REQ_MASK           0x00000020                // SW_CONN_APSRC_REQ[5]
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_APSRC_REQ_SHFT           5
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_AP_BUS_REQ_ADDR          CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_AP_BUS_REQ_MASK          0x00000010                // SW_CONN_AP_BUS_REQ[4]
#define CONN_CFG_ON_EMI_CTL_0_SW_CONN_AP_BUS_REQ_SHFT          4
#define CONN_CFG_ON_EMI_CTL_0_CONN_DDR_EN_SW_MODE_ADDR         CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_CONN_DDR_EN_SW_MODE_MASK         0x00000004                // CONN_DDR_EN_SW_MODE[2]
#define CONN_CFG_ON_EMI_CTL_0_CONN_DDR_EN_SW_MODE_SHFT         2
#define CONN_CFG_ON_EMI_CTL_0_CONN_APSRC_REQ_SW_MODE_ADDR      CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_CONN_APSRC_REQ_SW_MODE_MASK      0x00000002                // CONN_APSRC_REQ_SW_MODE[1]
#define CONN_CFG_ON_EMI_CTL_0_CONN_APSRC_REQ_SW_MODE_SHFT      1
#define CONN_CFG_ON_EMI_CTL_0_CONN_AP_BUS_REQ_SW_MODE_ADDR     CONN_CFG_ON_EMI_CTL_0_ADDR
#define CONN_CFG_ON_EMI_CTL_0_CONN_AP_BUS_REQ_SW_MODE_MASK     0x00000001                // CONN_AP_BUS_REQ_SW_MODE[0]
#define CONN_CFG_ON_EMI_CTL_0_CONN_AP_BUS_REQ_SW_MODE_SHFT     0

/* =====================================================================================

  ---RF_LDO_CTRL_0 (0x18001000 + 0x3B0)---

    SW_RF_LDO_RDY_0[0]           - (RW) rf_ldo_rdy_0 sw control
                                     1'b0: rf_ldo_rdy_0 = 1'b0
                                     1'b1: rf_ldo_rdy_0 =1'b1
    SW_VSRC_REQ_0[1]             - (RW) conn_vsrc_req_0 sw control
                                     1'b0: conn_vsrc_req_0 = 1'b0
                                     1'b1: conn_vsrc_req_0 =1'b1
    SWCTL_RF_LDO_RDY_0[2]        - (RW) sw control rf_ldo_rdy_0
                                     1'b0: control by hw
                                     1'b1: control by CR
    SWCTL_VSRC_REQ_0[3]          - (RW) sw control conn_vsrc_req_0
                                     1'b0: control by hw
                                     1'b1: control by CR
    CR_ACK_LDO_STATE_MASK_0[4]   - (RW) mask conn_vsrc_req_ack_0 for rf ldo control FSM
    RESERVED5[7..5]              - (RO) Reserved bits
    SW_RF_LDO_RDY_1[8]           - (RW) rf_ldo_rdy_1 sw control
                                     1'b0: rf_ldo_rdy_1 = 1'b0
                                     1'b1: rf_ldo_rdy_1 =1'b1
    SW_VSRC_REQ_1[9]             - (RW) conn_vsrc_req_1 sw control
                                     1'b0: conn_vsrc_req_1 = 1'b0
                                     1'b1: conn_vsrc_req_1 =1'b1
    SWCTL_RF_LDO_RDY_1[10]       - (RW) sw control rf_ldo_rdy_1
                                     1'b0: control by hw
                                     1'b1: control by CR
    SWCTL_VSRC_REQ_1[11]         - (RW) sw control conn_vsrc_req_1
                                     1'b0: control by hw
                                     1'b1: control by CR
    CR_ACK_LDO_STATE_MASK_1[12]  - (RW) mask conn_vsrc_req_ack_1 for rf ldo control FSM
    RESERVED13[15..13]           - (RO) Reserved bits
    SW_RF_LDO_RDY_2[16]          - (RW) rf_ldo_rdy_2 sw control
                                     1'b0: rf_ldo_rdy_2 = 1'b0
                                     1'b1: rf_ldo_rdy_2 =1'b1
    SW_VSRC_REQ_2[17]            - (RW) conn_vsrc_req_2 sw control
                                     1'b0: conn_vsrc_req_2 = 1'b0
                                     1'b1: conn_vsrc_req_2 =1'b1
    SWCTL_RF_LDO_RDY_2[18]       - (RW) sw control rf_ldo_rdy_2
                                     1'b0: control by hw
                                     1'b1: control by CR
    SWCTL_VSRC_REQ_2[19]         - (RW) sw control conn_vsrc_req_2
                                     1'b0: control by hw
                                     1'b1: control by CR
    CR_ACK_LDO_STATE_MASK_2[20]  - (RW) mask conn_vsrc_req_ack_2 for rf ldo control FSM
    RESERVED21[23..21]           - (RO) Reserved bits
    SW_RF_LDO_RDY_3[24]          - (RW) rf_ldo_rdy_3 sw control
                                     1'b0: rf_ldo_rdy_3 = 1'b0
                                     1'b1: rf_ldo_rdy_3 =1'b1
    SW_VSRC_REQ_3[25]            - (RW) conn_vsrc_req_3 sw control
                                     1'b0: conn_vsrc_req_3 = 1'b0
                                     1'b1: conn_vsrc_req_3 =1'b1
    SWCTL_RF_LDO_RDY_3[26]       - (RW) sw control rf_ldo_rdy_3
                                     1'b0: control by hw
                                     1'b1: control by CR
    SWCTL_VSRC_REQ_3[27]         - (RW) sw control conn_vsrc_req_3
                                     1'b0: control by hw
                                     1'b1: control by CR
    CR_ACK_LDO_STATE_MASK_3[28]  - (RW) mask conn_vsrc_req_ack_3 for rf ldo control FSM
    RESERVED29[31..29]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_3_ADDR CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_3_MASK 0x10000000                // CR_ACK_LDO_STATE_MASK_3[28]
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_3_SHFT 28
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_3_ADDR        CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_3_MASK        0x08000000                // SWCTL_VSRC_REQ_3[27]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_3_SHFT        27
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_3_ADDR      CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_3_MASK      0x04000000                // SWCTL_RF_LDO_RDY_3[26]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_3_SHFT      26
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_3_ADDR           CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_3_MASK           0x02000000                // SW_VSRC_REQ_3[25]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_3_SHFT           25
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_3_ADDR         CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_3_MASK         0x01000000                // SW_RF_LDO_RDY_3[24]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_3_SHFT         24
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_2_ADDR CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_2_MASK 0x00100000                // CR_ACK_LDO_STATE_MASK_2[20]
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_2_SHFT 20
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_2_ADDR        CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_2_MASK        0x00080000                // SWCTL_VSRC_REQ_2[19]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_2_SHFT        19
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_2_ADDR      CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_2_MASK      0x00040000                // SWCTL_RF_LDO_RDY_2[18]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_2_SHFT      18
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_2_ADDR           CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_2_MASK           0x00020000                // SW_VSRC_REQ_2[17]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_2_SHFT           17
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_2_ADDR         CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_2_MASK         0x00010000                // SW_RF_LDO_RDY_2[16]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_2_SHFT         16
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_1_ADDR CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_1_MASK 0x00001000                // CR_ACK_LDO_STATE_MASK_1[12]
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_1_SHFT 12
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_1_ADDR        CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_1_MASK        0x00000800                // SWCTL_VSRC_REQ_1[11]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_1_SHFT        11
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_1_ADDR      CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_1_MASK      0x00000400                // SWCTL_RF_LDO_RDY_1[10]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_1_SHFT      10
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_1_ADDR           CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_1_MASK           0x00000200                // SW_VSRC_REQ_1[9]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_1_SHFT           9
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_1_ADDR         CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_1_MASK         0x00000100                // SW_RF_LDO_RDY_1[8]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_1_SHFT         8
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_0_ADDR CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_0_MASK 0x00000010                // CR_ACK_LDO_STATE_MASK_0[4]
#define CONN_CFG_ON_RF_LDO_CTRL_0_CR_ACK_LDO_STATE_MASK_0_SHFT 4
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_0_ADDR        CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_0_MASK        0x00000008                // SWCTL_VSRC_REQ_0[3]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_VSRC_REQ_0_SHFT        3
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_0_ADDR      CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_0_MASK      0x00000004                // SWCTL_RF_LDO_RDY_0[2]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SWCTL_RF_LDO_RDY_0_SHFT      2
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_0_ADDR           CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_0_MASK           0x00000002                // SW_VSRC_REQ_0[1]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_VSRC_REQ_0_SHFT           1
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_0_ADDR         CONN_CFG_ON_RF_LDO_CTRL_0_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_0_MASK         0x00000001                // SW_RF_LDO_RDY_0[0]
#define CONN_CFG_ON_RF_LDO_CTRL_0_SW_RF_LDO_RDY_0_SHFT         0

/* =====================================================================================

  ---RF_LDO_CTRL_1 (0x18001000 + 0x3B4)---

    RF_LDO_EN_MASK_0[0]          - (RW) RF LDO enable mask for VCN13
                                     [0]: top rf_ldo_en
    RESERVED1[3..1]              - (RO) Reserved bits
    RF_LDO_EN_MASK_1[4]          - (RW) RF LDO enable mask for VCN15/18
                                     [0]: top rf_ldo_en
    RESERVED5[7..5]              - (RO) Reserved bits
    RF_LDO_EN_MASK_2[10..8]      - (RW) RF LDO enable mask for VCN33_1
                                     [2]: btsys_rf_ldo_en
                                     [1]: bn1 wfsys rf_ldo_en
                                     [0]: bn0 wfsys rf_ldo_en
    RESERVED11[11]               - (RO) Reserved bits
    RF_LDO_EN_MASK_3[14..12]     - (RW) RF LDO enable mask for VCN33_2
                                     [2]: btsys_rf_ldo_en
                                     [1]: bn1 wfsys rf_ldo_en
                                     [0]: bn0 wfsys rf_ldo_en
    RESERVED15[31..15]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_3_ADDR        CONN_CFG_ON_RF_LDO_CTRL_1_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_3_MASK        0x00007000                // RF_LDO_EN_MASK_3[14..12]
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_3_SHFT        12
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_2_ADDR        CONN_CFG_ON_RF_LDO_CTRL_1_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_2_MASK        0x00000700                // RF_LDO_EN_MASK_2[10..8]
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_2_SHFT        8
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_1_ADDR        CONN_CFG_ON_RF_LDO_CTRL_1_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_1_MASK        0x00000010                // RF_LDO_EN_MASK_1[4]
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_1_SHFT        4
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_0_ADDR        CONN_CFG_ON_RF_LDO_CTRL_1_ADDR
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_0_MASK        0x00000001                // RF_LDO_EN_MASK_0[0]
#define CONN_CFG_ON_RF_LDO_CTRL_1_RF_LDO_EN_MASK_0_SHFT        0

/* =====================================================================================

  ---RF_LDO_STATUS (0x18001000 + 0x3B8)---

    HW_RF_LDO_RDY_0[0]           - (RO) hw_rf_ldo_rdy_0
    HW_CONN_VSRC_REQ_0[1]        - (RO) hw_conn_vsrc_req_0
    LDO_STATE_0[3..2]            - (RO) rf ldo_0 state
    CONN_VSRC_REQ_ACK_0[4]       - (RO) conn_vsrc_req_ack_0
    CONN_VSRC_REQ_0[5]           - (RO) conn_vsrc_req_0
    RF_LDO_RDY_0[6]              - (RO) rf ldo ready_0
    RF_LDO_EN_0[7]               - (RO) rf ldo_0 enable
    HW_RF_LDO_RDY_1[8]           - (RO) hw_rf_ldo_rdy_1
    HW_CONN_VSRC_REQ_1[9]        - (RO) hw_conn_vsrc_req_1
    LDO_STATE_1[11..10]          - (RO) rf ldo_1 state
    CONN_VSRC_REQ_ACK_1[12]      - (RO) conn_vsrc_req_ack_1
    CONN_VSRC_REQ_1[13]          - (RO) conn_vsrc_req_1
    RF_LDO_RDY_1[14]             - (RO) rf ldo ready_1
    RF_LDO_EN_1[15]              - (RO) rf ldo_1 enable
    HW_RF_LDO_RDY_2[16]          - (RO) hw_rf_ldo_rdy_2
    HW_CONN_VSRC_REQ_2[17]       - (RO) hw_conn_vsrc_req_2
    LDO_STATE_2[19..18]          - (RO) rf ldo_2 state
    CONN_VSRC_REQ_ACK_2[20]      - (RO) conn_vsrc_req_ack_2
    CONN_VSRC_REQ_2[21]          - (RO) conn_vsrc_req_2
    RF_LDO_RDY_2[22]             - (RO) rf ldo ready_2
    RF_LDO_EN_2[23]              - (RO) rf ldo_2 enable
    HW_RF_LDO_RDY_3[24]          - (RO) hw_rf_ldo_rdy_3
    HW_CONN_VSRC_REQ_3[25]       - (RO) hw_conn_vsrc_req_3
    LDO_STATE_3[27..26]          - (RO) rf ldo_3 state
    CONN_VSRC_REQ_ACK_3[28]      - (RO) conn_vsrc_req_ack_3
    CONN_VSRC_REQ_3[29]          - (RO) conn_vsrc_req_3
    RF_LDO_RDY_3[30]             - (RO) rf ldo ready_3
    RF_LDO_EN_3[31]              - (RO) rf ldo_3 enable

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_3_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_3_MASK             0x80000000                // RF_LDO_EN_3[31]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_3_SHFT             31
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_3_ADDR            CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_3_MASK            0x40000000                // RF_LDO_RDY_3[30]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_3_SHFT            30
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_3_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_3_MASK         0x20000000                // CONN_VSRC_REQ_3[29]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_3_SHFT         29
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_3_ADDR     CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_3_MASK     0x10000000                // CONN_VSRC_REQ_ACK_3[28]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_3_SHFT     28
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_3_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_3_MASK             0x0C000000                // LDO_STATE_3[27..26]
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_3_SHFT             26
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_3_ADDR      CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_3_MASK      0x02000000                // HW_CONN_VSRC_REQ_3[25]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_3_SHFT      25
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_3_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_3_MASK         0x01000000                // HW_RF_LDO_RDY_3[24]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_3_SHFT         24
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_2_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_2_MASK             0x00800000                // RF_LDO_EN_2[23]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_2_SHFT             23
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_2_ADDR            CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_2_MASK            0x00400000                // RF_LDO_RDY_2[22]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_2_SHFT            22
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_2_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_2_MASK         0x00200000                // CONN_VSRC_REQ_2[21]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_2_SHFT         21
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_2_ADDR     CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_2_MASK     0x00100000                // CONN_VSRC_REQ_ACK_2[20]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_2_SHFT     20
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_2_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_2_MASK             0x000C0000                // LDO_STATE_2[19..18]
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_2_SHFT             18
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_2_ADDR      CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_2_MASK      0x00020000                // HW_CONN_VSRC_REQ_2[17]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_2_SHFT      17
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_2_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_2_MASK         0x00010000                // HW_RF_LDO_RDY_2[16]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_2_SHFT         16
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_1_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_1_MASK             0x00008000                // RF_LDO_EN_1[15]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_1_SHFT             15
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_1_ADDR            CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_1_MASK            0x00004000                // RF_LDO_RDY_1[14]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_1_SHFT            14
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_1_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_1_MASK         0x00002000                // CONN_VSRC_REQ_1[13]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_1_SHFT         13
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_1_ADDR     CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_1_MASK     0x00001000                // CONN_VSRC_REQ_ACK_1[12]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_1_SHFT     12
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_1_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_1_MASK             0x00000C00                // LDO_STATE_1[11..10]
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_1_SHFT             10
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_1_ADDR      CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_1_MASK      0x00000200                // HW_CONN_VSRC_REQ_1[9]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_1_SHFT      9
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_1_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_1_MASK         0x00000100                // HW_RF_LDO_RDY_1[8]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_1_SHFT         8
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_0_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_0_MASK             0x00000080                // RF_LDO_EN_0[7]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_EN_0_SHFT             7
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_0_ADDR            CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_0_MASK            0x00000040                // RF_LDO_RDY_0[6]
#define CONN_CFG_ON_RF_LDO_STATUS_RF_LDO_RDY_0_SHFT            6
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_0_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_0_MASK         0x00000020                // CONN_VSRC_REQ_0[5]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_0_SHFT         5
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_0_ADDR     CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_0_MASK     0x00000010                // CONN_VSRC_REQ_ACK_0[4]
#define CONN_CFG_ON_RF_LDO_STATUS_CONN_VSRC_REQ_ACK_0_SHFT     4
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_0_ADDR             CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_0_MASK             0x0000000C                // LDO_STATE_0[3..2]
#define CONN_CFG_ON_RF_LDO_STATUS_LDO_STATE_0_SHFT             2
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_0_ADDR      CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_0_MASK      0x00000002                // HW_CONN_VSRC_REQ_0[1]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_CONN_VSRC_REQ_0_SHFT      1
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_0_ADDR         CONN_CFG_ON_RF_LDO_STATUS_ADDR
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_0_MASK         0x00000001                // HW_RF_LDO_RDY_0[0]
#define CONN_CFG_ON_RF_LDO_STATUS_HW_RF_LDO_RDY_0_SHFT         0

/* =====================================================================================

  ---RF_LDO_TIMER_0 (0x18001000 + 0x3C0)---

    LDO_ON_STABLE_TIME[15..0]    - (RW) rf ldo timer for off stable time(unit: period of xtal)
    LDO_OFF_STABLE_TIME[31..16]  - (RW) rf ldo timer for off stable time(unit: period of xtal)

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_TIMER_0_LDO_OFF_STABLE_TIME_ADDR    CONN_CFG_ON_RF_LDO_TIMER_0_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_0_LDO_OFF_STABLE_TIME_MASK    0xFFFF0000                // LDO_OFF_STABLE_TIME[31..16]
#define CONN_CFG_ON_RF_LDO_TIMER_0_LDO_OFF_STABLE_TIME_SHFT    16
#define CONN_CFG_ON_RF_LDO_TIMER_0_LDO_ON_STABLE_TIME_ADDR     CONN_CFG_ON_RF_LDO_TIMER_0_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_0_LDO_ON_STABLE_TIME_MASK     0x0000FFFF                // LDO_ON_STABLE_TIME[15..0]
#define CONN_CFG_ON_RF_LDO_TIMER_0_LDO_ON_STABLE_TIME_SHFT     0

/* =====================================================================================

  ---RF_LDO_TIMER_1 (0x18001000 + 0x3C4)---

    LDO_ON_STABLE_TIME[15..0]    - (RW) rf ldo timer for off stable time(unit: period of xtal)
    LDO_OFF_STABLE_TIME[31..16]  - (RW) rf ldo timer for off stable time(unit: period of xtal)

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_TIMER_1_LDO_OFF_STABLE_TIME_ADDR    CONN_CFG_ON_RF_LDO_TIMER_1_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_1_LDO_OFF_STABLE_TIME_MASK    0xFFFF0000                // LDO_OFF_STABLE_TIME[31..16]
#define CONN_CFG_ON_RF_LDO_TIMER_1_LDO_OFF_STABLE_TIME_SHFT    16
#define CONN_CFG_ON_RF_LDO_TIMER_1_LDO_ON_STABLE_TIME_ADDR     CONN_CFG_ON_RF_LDO_TIMER_1_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_1_LDO_ON_STABLE_TIME_MASK     0x0000FFFF                // LDO_ON_STABLE_TIME[15..0]
#define CONN_CFG_ON_RF_LDO_TIMER_1_LDO_ON_STABLE_TIME_SHFT     0

/* =====================================================================================

  ---RF_LDO_TIMER_2 (0x18001000 + 0x3C8)---

    LDO_ON_STABLE_TIME[15..0]    - (RW) rf ldo timer for off stable time(unit: period of xtal)
    LDO_OFF_STABLE_TIME[31..16]  - (RW) rf ldo timer for off stable time(unit: period of xtal)

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_TIMER_2_LDO_OFF_STABLE_TIME_ADDR    CONN_CFG_ON_RF_LDO_TIMER_2_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_2_LDO_OFF_STABLE_TIME_MASK    0xFFFF0000                // LDO_OFF_STABLE_TIME[31..16]
#define CONN_CFG_ON_RF_LDO_TIMER_2_LDO_OFF_STABLE_TIME_SHFT    16
#define CONN_CFG_ON_RF_LDO_TIMER_2_LDO_ON_STABLE_TIME_ADDR     CONN_CFG_ON_RF_LDO_TIMER_2_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_2_LDO_ON_STABLE_TIME_MASK     0x0000FFFF                // LDO_ON_STABLE_TIME[15..0]
#define CONN_CFG_ON_RF_LDO_TIMER_2_LDO_ON_STABLE_TIME_SHFT     0

/* =====================================================================================

  ---RF_LDO_TIMER_3 (0x18001000 + 0x3CC)---

    LDO_ON_STABLE_TIME[15..0]    - (RW) rf ldo timer for off stable time(unit: period of xtal)
    LDO_OFF_STABLE_TIME[31..16]  - (RW) rf ldo timer for off stable time(unit: period of xtal)

 =====================================================================================*/
#define CONN_CFG_ON_RF_LDO_TIMER_3_LDO_OFF_STABLE_TIME_ADDR    CONN_CFG_ON_RF_LDO_TIMER_3_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_3_LDO_OFF_STABLE_TIME_MASK    0xFFFF0000                // LDO_OFF_STABLE_TIME[31..16]
#define CONN_CFG_ON_RF_LDO_TIMER_3_LDO_OFF_STABLE_TIME_SHFT    16
#define CONN_CFG_ON_RF_LDO_TIMER_3_LDO_ON_STABLE_TIME_ADDR     CONN_CFG_ON_RF_LDO_TIMER_3_ADDR
#define CONN_CFG_ON_RF_LDO_TIMER_3_LDO_ON_STABLE_TIME_MASK     0x0000FFFF                // LDO_ON_STABLE_TIME[15..0]
#define CONN_CFG_ON_RF_LDO_TIMER_3_LDO_ON_STABLE_TIME_SHFT     0

/* =====================================================================================

  ---CONN_INFRA_CONN2AP_SLP_CTRL (0x18001000 + 0x400)---

    CFG_CONN2AP_GALS_TX_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_gals_conn2ap_tx sleep protect en
                                     0: conn_infra_gals_conn2ap_tx sleep protect en depends on disable control or hardware
    CFG_CONN2AP_GALS_TX_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2ap_tx sleep protect disable
                                     0: conn_infra_gals_conn2ap_tx  sleep protect will control by hardware
    CSR_CONN2AP_GALS_TX_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_gals_conn2ap_tx sleep protect en
                                     0: conn_infra_gals_conn2ap_tx sleep protect en depends on disable control or hardware
    CSR_CONN2AP_GALS_TX_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2ap_tx sleep protect disable
                                     0: conn_infra_gals_conn2ap_tx  sleep protect will control by hardware
    CFG_CONN2AP_GALS_RX_SLP_PROT_SW_EN[4] - (RW) 1: force conn_infra_gals_conn2ap_rx sleep protect en
                                     0: conn_infra_gals_conn2ap_rx sleep protect en depends on disable control or hardware
    CFG_CONN2AP_GALS_RX_SLP_PROT_SW_DIS[5] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2ap_rx sleep protect disable
                                     0: conn_infra_gals_conn2ap_rx  sleep protect will control by hardware
    CSR_CONN2AP_GALS_RX_SLP_PROT_SW_EN[6] - (RO) 1: force conn_infra_gals_conn2ap_rx sleep protect en
                                     0: conn_infra_gals_conn2ap_rx sleep protect en depends on disable control or hardware
    CSR_CONN2AP_GALS_RX_SLP_PROT_SW_DIS[7] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2ap_rx sleep protect disable
                                     0: conn_infra_gals_conn2ap_rx  sleep protect will control by hardware
    CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_EN[8] - (RW) timeout function enable
                                     1: enable
                                     0: disable
    CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9] - (RW) clear timeout irq, write 1 to clear, and need to write 0 after write 1
    CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10] - (RW) timeout timing set, count with 32k clock
    CONN2AP_GALS_SLPPROT_CTRL_FSM_RST[16] - (RW) fsm_reset, 1:reset, 0: release
    CONN2AP_GALS_SLPPROT_CTRL_TX_EN_MODE[17] - (RW) tx sleep protect en mode, 1: sw mode, 0: hw mode
    CONN2AP_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18] - (RW) tx sleep protect sw ctrl
    CONN2AP_GALS_SLPPROT_CTRL_RX_EN_MODE[19] - (RW) rx sleep protect en mode, 1: sw mode, 0: hw mode
    CONN2AP_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20] - (RW) rx sleep protect sw ctrl
    CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MODE[21] - (RW) tx sleep protect rdy mode, 1: sw mode, 0: hw mode
    CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22] - (RW) tx sleep protect rdy sw ctrl
    CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MASK[23] - (RW) tx sleep protect rdy mask
    CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MODE[24] - (RW) rx sleep protect rdy mode, 1: sw mode, 0: hw mode
    CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25] - (RW) rx sleep protect rdy sw ctrl
    CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MASK[26] - (RW) rx sleep protect rdy mask
    CONN2AP_GALS_SLPPROT_CTRL_MODE[27] - (RW) gals sleep protect mode, 1: sw, 0: hw
    CONN2AP_GALS_SLPPROT_CTRL_SW_EN[28] - (RW) sleep protect sw en
    CONN2AP_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29] - (RW) hw enable signal  0 mask
    CONN2AP_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30] - (RW) hw enable signal  1 mask
    CONN2AP_GALS_SLPPROT_CTRL_DUMMY[31] - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_DUMMY_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_DUMMY_MASK 0x80000000                // CONN2AP_GALS_SLPPROT_CTRL_DUMMY[31]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_DUMMY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_HW_EN_1_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_HW_EN_1_MASK_MASK 0x40000000                // CONN2AP_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_HW_EN_1_MASK_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_HW_EN_0_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_HW_EN_0_MASK_MASK 0x20000000                // CONN2AP_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_HW_EN_0_MASK_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_SW_EN_MASK 0x10000000                // CONN2AP_GALS_SLPPROT_CTRL_SW_EN[28]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_SW_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_MODE_MASK 0x08000000                // CONN2AP_GALS_SLPPROT_CTRL_MODE[27]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_MODE_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MASK_MASK 0x04000000                // CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MASK[26]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MASK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_MASK 0x02000000                // CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MODE_MASK 0x01000000                // CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MODE[24]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_RDY_MODE_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MASK_MASK 0x00800000                // CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MASK[23]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MASK_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_MASK 0x00400000                // CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MODE_MASK 0x00200000                // CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MODE[21]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_RDY_MODE_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_MASK 0x00100000                // CONN2AP_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_EN_MODE_MASK 0x00080000                // CONN2AP_GALS_SLPPROT_CTRL_RX_EN_MODE[19]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_RX_EN_MODE_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_MASK 0x00040000                // CONN2AP_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_EN_MODE_MASK 0x00020000                // CONN2AP_GALS_SLPPROT_CTRL_TX_EN_MODE[17]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TX_EN_MODE_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_FSM_RST_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_FSM_RST_MASK 0x00010000                // CONN2AP_GALS_SLPPROT_CTRL_FSM_RST[16]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_FSM_RST_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_TIME_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_TIME_MASK 0x0000FC00                // CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_TIME_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_CLR_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_CLR_MASK 0x00000200                // CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_CLR_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_EN_MASK 0x00000100                // CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_EN[8]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CONN2AP_GALS_SLPPROT_CTRL_TIMEOUT_EN_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000080                // CSR_CONN2AP_GALS_RX_SLP_PROT_SW_DIS[7]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_RX_SLP_PROT_SW_DIS_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000040                // CSR_CONN2AP_GALS_RX_SLP_PROT_SW_EN[6]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_RX_SLP_PROT_SW_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000020                // CFG_CONN2AP_GALS_RX_SLP_PROT_SW_DIS[5]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_RX_SLP_PROT_SW_DIS_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010                // CFG_CONN2AP_GALS_RX_SLP_PROT_SW_EN[4]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_CONN2AP_GALS_TX_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_TX_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_CONN2AP_GALS_TX_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CSR_CONN2AP_GALS_TX_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN2AP_GALS_TX_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_TX_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN2AP_GALS_TX_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_CTRL_CFG_CONN2AP_GALS_TX_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_CONN2AP_SLP_STATUS (0x18001000 + 0x404)---

    RESERVED0[11..0]             - (RO) Reserved bits
    CONN2AP_GALS_CTRL_DIS_TIMEOUT[12] - (RO) conn2ap gals sleep protect contorol sleep protect disable timeout
    CONN2AP_GALS_CTRL_EN_TIMEOUT[13] - (RO) conn2ap gals sleep protect contorol sleep protect enable timeout
    RESERVED14[14]               - (RO) Reserved bits
    CONN2AP_GALS_CTRL_TIMEOUT[15] - (RO) conn2ap gals sleep protect contorol timeout
    RESERVED16[18..16]           - (RO) Reserved bits
    CONN2AP_GALS_CTRL_PROT_RDY_OUT[19] - (RO) conn2ap gals sleep protect control protect ready
    RESERVED20[21..20]           - (RO) Reserved bits
    CONN2AP_GALS_CTRL_PROT_RX_RDY[22] - (RO) conn2ap gals sleep protect rx ready
    CONN2AP_GALS_CTRL_PROT_TX_RDY[23] - (RO) conn2ap gals sleep protect tx ready
    RESERVED24[27..24]           - (RO) Reserved bits
    CONN2AP_GALS_CTRL_PROT_RX_EN[28] - (RO) conn2ap gals sleep protect control sleep protect rx enable
    CONN2AP_GALS_CTRL_PROT_TX_EN[29] - (RO) conn2ap gals sleep protect control sleep protect tx enable
    RESERVED30[30]               - (RO) Reserved bits
    CONN2AP_GALS_CTRL_PROT_EN[31] - (RO) conn2ap gals sleep protect control sleep protect enable

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_EN_MASK 0x80000000                // CONN2AP_GALS_CTRL_PROT_EN[31]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_EN_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_TX_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_TX_EN_MASK 0x20000000                // CONN2AP_GALS_CTRL_PROT_TX_EN[29]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_TX_EN_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RX_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RX_EN_MASK 0x10000000                // CONN2AP_GALS_CTRL_PROT_RX_EN[28]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RX_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_TX_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000                // CONN2AP_GALS_CTRL_PROT_TX_RDY[23]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RX_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000                // CONN2AP_GALS_CTRL_PROT_RX_RDY[22]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RX_RDY_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RDY_OUT_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RDY_OUT_MASK 0x00080000                // CONN2AP_GALS_CTRL_PROT_RDY_OUT[19]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_PROT_RDY_OUT_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_TIMEOUT_MASK 0x00008000                // CONN2AP_GALS_CTRL_TIMEOUT[15]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_TIMEOUT_SHFT 15
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_EN_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_EN_TIMEOUT_MASK 0x00002000                // CONN2AP_GALS_CTRL_EN_TIMEOUT[13]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_EN_TIMEOUT_SHFT 13
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_DIS_TIMEOUT_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_DIS_TIMEOUT_MASK 0x00001000                // CONN2AP_GALS_CTRL_DIS_TIMEOUT[12]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_SLP_STATUS_CONN2AP_GALS_CTRL_DIS_TIMEOUT_SHFT 12

/* =====================================================================================

  ---CONN_INFRA_CONN2AP_EMI_SLP_STATUS (0x18001000 + 0x408)---

    CFG_CONN2TOP_EMI_SLP_PROT_SW_EN[0] - (RW) 1: force conn2top_emi_path sleep protect en
                                     0: conn2top_emi_path sleep protect en depends on disable control or hardware
    CFG_CONN2TOP_EMI_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn2top_emi_path sleep protect disable
                                     0: conn2top_emi_path  sleep protect will control by hardware
    CONNTOP_EMI_PATH_SLP_PROT_RDY[2] - (RO) conn2top_emi_path_slp_prot_rdy
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CONNTOP_EMI_PATH_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CONNTOP_EMI_PATH_SLP_PROT_RDY_MASK 0x00000004                // CONNTOP_EMI_PATH_SLP_PROT_RDY[2]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CONNTOP_EMI_PATH_SLP_PROT_RDY_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CFG_CONN2TOP_EMI_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CFG_CONN2TOP_EMI_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN2TOP_EMI_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CFG_CONN2TOP_EMI_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CFG_CONN2TOP_EMI_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CFG_CONN2TOP_EMI_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN2TOP_EMI_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CONN2AP_EMI_SLP_STATUS_CFG_CONN2TOP_EMI_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_AP2CONN_SLP_CTRL (0x18001000 + 0x410)---

    CFG_AP2CONN_GALS_TX_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_gals_ap2conn_tx sleep protect en
                                     0: conn_infra_gals_ap2conn_tx sleep protect en depends on disable control or hardware
    CFG_AP2CONN_GALS_TX_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_ap2conn_tx sleep protect disable
                                     0: conn_infra_gals_ap2conn_tx  sleep protect will control by hardware
    CSR_AP2CONN_GALS_TX_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_gals_ap2conn_tx sleep protect en
                                     0: conn_infra_gals_ap2conn_tx sleep protect en depends on disable control or hardware
    CSR_AP2CONN_GALS_TX_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_ap2conn_tx sleep protect disable
                                     0: conn_infra_gals_ap2conn_tx  sleep protect will control by hardware
    CFG_AP2CONN_GALS_RX_SLP_PROT_SW_EN[4] - (RW) 1: force conn_infra_gals_ap2conn_rx sleep protect en
                                     0: conn_infra_gals_ap2conn_rx sleep protect en depends on disable control or hardware
    CFG_AP2CONN_GALS_RX_SLP_PROT_SW_DIS[5] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_ap2conn_rx sleep protect disable
                                     0: conn_infra_gals_ap2conn_rx  sleep protect will control by hardware
    CSR_AP2CONN_GALS_RX_SLP_PROT_SW_EN[6] - (RO) 1: force conn_infra_gals_ap2conn_rx sleep protect en
                                     0: conn_infra_gals_ap2conn_rx sleep protect en depends on disable control or hardware
    CSR_AP2CONN_GALS_RX_SLP_PROT_SW_DIS[7] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_ap2conn_rx sleep protect disable
                                     0: conn_infra_gals_ap2conn_rx  sleep protect will control by hardware
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000080                // CSR_AP2CONN_GALS_RX_SLP_PROT_SW_DIS[7]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_RX_SLP_PROT_SW_DIS_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000040                // CSR_AP2CONN_GALS_RX_SLP_PROT_SW_EN[6]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000020                // CFG_AP2CONN_GALS_RX_SLP_PROT_SW_DIS[5]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_RX_SLP_PROT_SW_DIS_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010                // CFG_AP2CONN_GALS_RX_SLP_PROT_SW_EN[4]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_AP2CONN_GALS_TX_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_TX_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_AP2CONN_GALS_TX_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CSR_AP2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_AP2CONN_GALS_TX_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_TX_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_AP2CONN_GALS_TX_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_CTRL_CFG_AP2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_AP2CONN_SLP_STATUS (0x18001000 + 0x414)---

    RESERVED0[28..0]             - (RO) Reserved bits
    AP2CONN_GALS_RX_SLP_PROT_RDY[29] - (RO) ap2conn gals rx sleep protect ready
    AP2CONN_GALS_RX_SLP_PROT_HW_EN[30] - (RO) ap2conn gals rx sleep protect enable from top
    AP2CONN_GALS_TX_SLP_PROT_RDY[31] - (RO) ap2conn gals tx sleep protect ready

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_TX_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_TX_SLP_PROT_RDY_MASK 0x80000000                // AP2CONN_GALS_TX_SLP_PROT_RDY[31]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_TX_SLP_PROT_RDY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_RX_SLP_PROT_HW_EN_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_RX_SLP_PROT_HW_EN_MASK 0x40000000                // AP2CONN_GALS_RX_SLP_PROT_HW_EN[30]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_RX_SLP_PROT_HW_EN_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_RX_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_RX_SLP_PROT_RDY_MASK 0x20000000                // AP2CONN_GALS_RX_SLP_PROT_RDY[29]
#define CONN_CFG_ON_CONN_INFRA_AP2CONN_SLP_STATUS_AP2CONN_GALS_RX_SLP_PROT_RDY_SHFT 29

/* =====================================================================================

  ---CONN_INFRA_ON_BUS_SLP_CTRL (0x18001000 + 0x420)---

    CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_on2off sleep protect en
                                     0: conn_infra_on2off sleep protect en depends on disable control or hardware
    CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_on2off sleep protect disable
                                     0: conn_infra_on2off  sleep protect will control by hardware
    CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_on2off sleep protect en
                                     0: conn_infra_on2off sleep protect en depends on disable control or hardware
    CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_on2off sleep protect disable
                                     0: conn_infra_on2off  sleep protect will control by hardware
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CSR_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_CTRL_CFG_CONN_INFRA_ON2OFF_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_ON_BUS_SLP_STATUS (0x18001000 + 0x424)---

    R_CONN_INFRA_ON_BUS_SLP_STATUS[31..0] - (RO)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_STATUS_R_CONN_INFRA_ON_BUS_SLP_STATUS_ADDR CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_STATUS_R_CONN_INFRA_ON_BUS_SLP_STATUS_MASK 0xFFFFFFFF                // R_CONN_INFRA_ON_BUS_SLP_STATUS[31..0]
#define CONN_CFG_ON_CONN_INFRA_ON_BUS_SLP_STATUS_R_CONN_INFRA_ON_BUS_SLP_STATUS_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_OFF_BUS_SLP_CTRL (0x18001000 + 0x430)---

    CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_off2on sleep protect en
                                     0: conn_infra_off2on sleep protect en depends on disable control or hardware
    CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_off2on sleep protect disable
                                     0: conn_infra_off2on  sleep protect will control by hardware
    CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_off2on sleep protect en
                                     0: conn_infra_off2on sleep protect en depends on disable control or hardware
    CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_off2on sleep protect disable
                                     0: conn_infra_off2on  sleep protect will control by hardware
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CSR_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_CTRL_CFG_CONN_INFRA_OFF2ON_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_OFF_BUS_SLP_STATUS (0x18001000 + 0x434)---

    R_CONN_INFRA_OFF_BUS_SLP_STATUS[31..0] - (RO)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_STATUS_R_CONN_INFRA_OFF_BUS_SLP_STATUS_ADDR CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_STATUS_R_CONN_INFRA_OFF_BUS_SLP_STATUS_MASK 0xFFFFFFFF                // R_CONN_INFRA_OFF_BUS_SLP_STATUS[31..0]
#define CONN_CFG_ON_CONN_INFRA_OFF_BUS_SLP_STATUS_R_CONN_INFRA_OFF_BUS_SLP_STATUS_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WF_SLP_CTRL (0x18001000 + 0x440)---

    CFG_CONN_WF_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_wf sleep protect en
                                     0: conn_infra_wf sleep protect en depends on disable control or hardware
    CFG_CONN_WF_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_wf sleep protect disable
                                     0: conn_infra_wf  sleep protect will control by hardware
    CSR_CONN_WF_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_wf sleep protect en
                                     0: conn_infra_wf sleep protect en depends on disable control or hardware
    CSR_CONN_WF_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_wf sleep protect disable
                                     0: conn_infra_wf  sleep protect will control by hardware
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CSR_CONN_WF_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CSR_CONN_WF_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_CONN_WF_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CSR_CONN_WF_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CSR_CONN_WF_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CSR_CONN_WF_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_CONN_WF_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CSR_CONN_WF_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CFG_CONN_WF_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CFG_CONN_WF_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN_WF_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CFG_CONN_WF_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CFG_CONN_WF_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CFG_CONN_WF_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN_WF_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_CTRL_CFG_CONN_WF_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_WF_SLP_STATUS (0x18001000 + 0x444)---

    RESERVED0[23..0]             - (RO) Reserved bits
    WFDMA2CONN_SLP_PROT_HW_EN[24] - (RO)  xxx 
    WFDMA2CONN_SLP_PROT_RDY[25]  - (RO)  xxx 
    WF_ICAP2CONN_SLP_PROT_HW_EN[26] - (RO)  xxx 
    WF_ICAP2CONN_SLP_PROT_RDY[27] - (RO)  xxx 
    CONN2WF_SLP_PROT_HW_EN[28]   - (RO)  xxx 
    CONN2WF_SLP_PROT_RDY[29]     - (RO)  xxx 
    WF2CONN_SLP_PROT_HW_EN[30]   - (RO)  xxx 
    WF2CONN_SLP_PROT_RDY[31]     - (RO)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF2CONN_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF2CONN_SLP_PROT_RDY_MASK 0x80000000                // WF2CONN_SLP_PROT_RDY[31]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF2CONN_SLP_PROT_RDY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF2CONN_SLP_PROT_HW_EN_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF2CONN_SLP_PROT_HW_EN_MASK 0x40000000                // WF2CONN_SLP_PROT_HW_EN[30]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF2CONN_SLP_PROT_HW_EN_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_CONN2WF_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_CONN2WF_SLP_PROT_RDY_MASK 0x20000000                // CONN2WF_SLP_PROT_RDY[29]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_CONN2WF_SLP_PROT_RDY_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_CONN2WF_SLP_PROT_HW_EN_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_CONN2WF_SLP_PROT_HW_EN_MASK 0x10000000                // CONN2WF_SLP_PROT_HW_EN[28]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_CONN2WF_SLP_PROT_HW_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF_ICAP2CONN_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF_ICAP2CONN_SLP_PROT_RDY_MASK 0x08000000                // WF_ICAP2CONN_SLP_PROT_RDY[27]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF_ICAP2CONN_SLP_PROT_RDY_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF_ICAP2CONN_SLP_PROT_HW_EN_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF_ICAP2CONN_SLP_PROT_HW_EN_MASK 0x04000000                // WF_ICAP2CONN_SLP_PROT_HW_EN[26]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WF_ICAP2CONN_SLP_PROT_HW_EN_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WFDMA2CONN_SLP_PROT_RDY_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WFDMA2CONN_SLP_PROT_RDY_MASK 0x02000000                // WFDMA2CONN_SLP_PROT_RDY[25]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WFDMA2CONN_SLP_PROT_RDY_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WFDMA2CONN_SLP_PROT_HW_EN_ADDR CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_ADDR
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WFDMA2CONN_SLP_PROT_HW_EN_MASK 0x01000000                // WFDMA2CONN_SLP_PROT_HW_EN[24]
#define CONN_CFG_ON_CONN_INFRA_WF_SLP_STATUS_WFDMA2CONN_SLP_PROT_HW_EN_SHFT 24

/* =====================================================================================

  ---CONN_INFRA_CONN2BT_SLP_CTRL (0x18001000 + 0x450)---

    CFG_CONN2BT_GALS_TX_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_gals_conn2bt_tx sleep protect en
                                     0: conn_infra_gals_conn2bt_tx sleep protect en depends on disable control or hardware
    CFG_CONN2BT_GALS_TX_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2bt_tx sleep protect disable
                                     0: conn_infra_gals_conn2bt_tx  sleep protect will control by hardware
    CSR_CONN2BT_GALS_TX_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_gals_conn2bt_tx sleep protect en
                                     0: conn_infra_gals_conn2bt_tx sleep protect en depends on disable control or hardware
    CSR_CONN2BT_GALS_TX_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2bt_tx sleep protect disable
                                     0: conn_infra_gals_conn2bt_tx  sleep protect will control by hardware
    CFG_CONN2BT_GALS_RX_SLP_PROT_SW_EN[4] - (RW) 1: force conn_infra_gals_conn2bt_rx sleep protect en
                                     0: conn_infra_gals_conn2bt_rx sleep protect en depends on disable control or hardware
    CFG_CONN2BT_GALS_RX_SLP_PROT_SW_DIS[5] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2bt_rx sleep protect disable
                                     0: conn_infra_gals_conn2bt_rx  sleep protect will control by hardware
    CSR_CONN2BT_GALS_RX_SLP_PROT_SW_EN[6] - (RO) 1: force conn_infra_gals_conn2bt_rx sleep protect en
                                     0: conn_infra_gals_conn2bt_rx sleep protect en depends on disable control or hardware
    CSR_CONN2BT_GALS_RX_SLP_PROT_SW_DIS[7] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2bt_rx sleep protect disable
                                     0: conn_infra_gals_conn2bt_rx  sleep protect will control by hardware
    CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_EN[8] - (RW) timeout function enable
                                     1: enable
                                     0: disable
    CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9] - (RW) clear timeout irq, write 1 to clear, and need to write 0 after write 1
    CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10] - (RW) timeout timing set, count with 32k clock
    CONN2BT_GALS_SLPPROT_CTRL_FSM_RST[16] - (RW) fsm_reset, 1:reset, 0: release
    CONN2BT_GALS_SLPPROT_CTRL_TX_EN_MODE[17] - (RW) tx sleep protect en mode, 1: sw mode, 0: hw mode
    CONN2BT_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18] - (RW) tx sleep protect sw ctrl
    CONN2BT_GALS_SLPPROT_CTRL_RX_EN_MODE[19] - (RW) rx sleep protect en mode, 1: sw mode, 0: hw mode
    CONN2BT_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20] - (RW) rx sleep protect sw ctrl
    CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MODE[21] - (RW) tx sleep protect rdy mode, 1: sw mode, 0: hw mode
    CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22] - (RW) tx sleep protect rdy sw ctrl
    CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MASK[23] - (RW) tx sleep protect rdy mask
    CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MODE[24] - (RW) rx sleep protect rdy mode, 1: sw mode, 0: hw mode
    CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25] - (RW) rx sleep protect rdy sw ctrl
    CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MASK[26] - (RW) rx sleep protect rdy mask
    CONN2BT_GALS_SLPPROT_CTRL_MODE[27] - (RW) gals sleep protect mode, 1: sw, 0: hw
    CONN2BT_GALS_SLPPROT_CTRL_SW_EN[28] - (RW) sleep protect sw en
    CONN2BT_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29] - (RW) hw enable signal  0 mask
    CONN2BT_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30] - (RW) hw enable signal  1 mask
    CONN2BT_GALS_SLPPROT_CTRL_DUMMY[31] - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_DUMMY_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_DUMMY_MASK 0x80000000                // CONN2BT_GALS_SLPPROT_CTRL_DUMMY[31]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_DUMMY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_HW_EN_1_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_HW_EN_1_MASK_MASK 0x40000000                // CONN2BT_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_HW_EN_1_MASK_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_HW_EN_0_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_HW_EN_0_MASK_MASK 0x20000000                // CONN2BT_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_HW_EN_0_MASK_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_SW_EN_MASK 0x10000000                // CONN2BT_GALS_SLPPROT_CTRL_SW_EN[28]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_SW_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_MODE_MASK 0x08000000                // CONN2BT_GALS_SLPPROT_CTRL_MODE[27]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_MODE_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MASK_MASK 0x04000000                // CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MASK[26]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MASK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_MASK 0x02000000                // CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MODE_MASK 0x01000000                // CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MODE[24]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_RDY_MODE_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MASK_MASK 0x00800000                // CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MASK[23]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MASK_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_MASK 0x00400000                // CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MODE_MASK 0x00200000                // CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MODE[21]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_RDY_MODE_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_MASK 0x00100000                // CONN2BT_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_EN_MODE_MASK 0x00080000                // CONN2BT_GALS_SLPPROT_CTRL_RX_EN_MODE[19]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_RX_EN_MODE_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_MASK 0x00040000                // CONN2BT_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_EN_MODE_MASK 0x00020000                // CONN2BT_GALS_SLPPROT_CTRL_TX_EN_MODE[17]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TX_EN_MODE_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_FSM_RST_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_FSM_RST_MASK 0x00010000                // CONN2BT_GALS_SLPPROT_CTRL_FSM_RST[16]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_FSM_RST_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_TIME_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_TIME_MASK 0x0000FC00                // CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_TIME_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_CLR_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_CLR_MASK 0x00000200                // CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_CLR_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_EN_MASK 0x00000100                // CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_EN[8]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CONN2BT_GALS_SLPPROT_CTRL_TIMEOUT_EN_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000080                // CSR_CONN2BT_GALS_RX_SLP_PROT_SW_DIS[7]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_RX_SLP_PROT_SW_DIS_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000040                // CSR_CONN2BT_GALS_RX_SLP_PROT_SW_EN[6]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_RX_SLP_PROT_SW_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000020                // CFG_CONN2BT_GALS_RX_SLP_PROT_SW_DIS[5]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_RX_SLP_PROT_SW_DIS_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010                // CFG_CONN2BT_GALS_RX_SLP_PROT_SW_EN[4]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_CONN2BT_GALS_TX_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_TX_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_CONN2BT_GALS_TX_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CSR_CONN2BT_GALS_TX_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN2BT_GALS_TX_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_TX_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN2BT_GALS_TX_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CONN2BT_SLP_CTRL_CFG_CONN2BT_GALS_TX_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---GALS_CONN2BT_SLP_STATUS (0x18001000 + 0x454)---

    RESERVED0[11..0]             - (RO) Reserved bits
    CONN2BT_GALS_CTRL_DIS_TIMEOUT[12] - (RO) conn2bt gals sleep protect contorol sleep protect disable timeout
    CONN2BT_GALS_CTRL_EN_TIMEOUT[13] - (RO) conn2bt gals sleep protect contorol sleep protect enable timeout
    RESERVED14[14]               - (RO) Reserved bits
    CONN2BT_GALS_CTRL_TIMEOUT[15] - (RO) conn2bt gals sleep protect contorol timeout
    RESERVED16[18..16]           - (RO) Reserved bits
    CONN2BT_GALS_CTRL_PROT_RDY_OUT[19] - (RO) conn2bt gals sleep protect control protect ready
    RESERVED20[21..20]           - (RO) Reserved bits
    CONN2BT_GALS_CTRL_PROT_RX_RDY[22] - (RO) conn2bt gals sleep protect rx ready
    CONN2BT_GALS_CTRL_PROT_TX_RDY[23] - (RO) conn2bt gals sleep protect tx ready
    RESERVED24[27..24]           - (RO) Reserved bits
    CONN2BT_GALS_CTRL_PROT_RX_EN[28] - (RO) conn2bt gals sleep protect control sleep protect rx enable
    CONN2BT_GALS_CTRL_PROT_TX_EN[29] - (RO) conn2bt gals sleep protect control sleep protect tx enable
    RESERVED30[30]               - (RO) Reserved bits
    CONN2BT_GALS_CTRL_PROT_EN[31] - (RO) conn2bt gals sleep protect control sleep protect enable

 =====================================================================================*/
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_EN_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_EN_MASK 0x80000000                // CONN2BT_GALS_CTRL_PROT_EN[31]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_EN_SHFT 31
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_TX_EN_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_TX_EN_MASK 0x20000000                // CONN2BT_GALS_CTRL_PROT_TX_EN[29]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_TX_EN_SHFT 29
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RX_EN_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RX_EN_MASK 0x10000000                // CONN2BT_GALS_CTRL_PROT_RX_EN[28]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RX_EN_SHFT 28
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_TX_RDY_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000                // CONN2BT_GALS_CTRL_PROT_TX_RDY[23]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RX_RDY_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000                // CONN2BT_GALS_CTRL_PROT_RX_RDY[22]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RX_RDY_SHFT 22
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RDY_OUT_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RDY_OUT_MASK 0x00080000                // CONN2BT_GALS_CTRL_PROT_RDY_OUT[19]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_PROT_RDY_OUT_SHFT 19
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_TIMEOUT_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_TIMEOUT_MASK 0x00008000                // CONN2BT_GALS_CTRL_TIMEOUT[15]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_TIMEOUT_SHFT 15
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_EN_TIMEOUT_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_EN_TIMEOUT_MASK 0x00002000                // CONN2BT_GALS_CTRL_EN_TIMEOUT[13]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_EN_TIMEOUT_SHFT 13
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_DIS_TIMEOUT_ADDR CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_DIS_TIMEOUT_MASK 0x00001000                // CONN2BT_GALS_CTRL_DIS_TIMEOUT[12]
#define CONN_CFG_ON_GALS_CONN2BT_SLP_STATUS_CONN2BT_GALS_CTRL_DIS_TIMEOUT_SHFT 12

/* =====================================================================================

  ---CONN_INFRA_BT2CONN_SLP_CTRL (0x18001000 + 0x460)---

    CFG_BT2CONN_GALS_TX_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_gals_bt2conn_tx sleep protect en
                                     0: conn_infra_gals_bt2conn_tx sleep protect en depends on disable control or hardware
    CFG_BT2CONN_GALS_TX_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_bt2conn_tx sleep protect disable
                                     0: conn_infra_gals_bt2conn_tx  sleep protect will control by hardware
    CSR_BT2CONN_GALS_TX_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_gals_bt2conn_tx sleep protect en
                                     0: conn_infra_gals_bt2conn_tx sleep protect en depends on disable control or hardware
    CSR_BT2CONN_GALS_TX_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_bt2conn_tx sleep protect disable
                                     0: conn_infra_gals_bt2conn_tx  sleep protect will control by hardware
    CFG_BT2CONN_GALS_RX_SLP_PROT_SW_EN[4] - (RW) 1: force conn_infra_gals_bt2conn_rx sleep protect en
                                     0: conn_infra_gals_bt2conn_rx sleep protect en depends on disable control or hardware
    CFG_BT2CONN_GALS_RX_SLP_PROT_SW_DIS[5] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_bt2conn_rx sleep protect disable
                                     0: conn_infra_gals_bt2conn_rx  sleep protect will control by hardware
    CSR_BT2CONN_GALS_RX_SLP_PROT_SW_EN[6] - (RO) 1: force conn_infra_gals_bt2conn_rx sleep protect en
                                     0: conn_infra_gals_bt2conn_rx sleep protect en depends on disable control or hardware
    CSR_BT2CONN_GALS_RX_SLP_PROT_SW_DIS[7] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_bt2conn_rx sleep protect disable
                                     0: conn_infra_gals_bt2conn_rx  sleep protect will control by hardware
    BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN[8] - (RW) timeout function enable
                                     1: enable
                                     0: disable
    BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9] - (RW) clear timeout irq, write 1 to clear, and need to write 0 after write 1
    BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10] - (RW) timeout timing set, count with 32k clock
    BT2CONN_GALS_SLPPROT_CTRL_FSM_RST[16] - (RW) fsm_reset, 1:reset, 0: release
    BT2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE[17] - (RW) tx sleep protect en mode, 1: sw mode, 0: hw mode
    BT2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18] - (RW) tx sleep protect sw ctrl
    BT2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE[19] - (RW) rx sleep protect en mode, 1: sw mode, 0: hw mode
    BT2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20] - (RW) rx sleep protect sw ctrl
    BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE[21] - (RW) tx sleep protect rdy mode, 1: sw mode, 0: hw mode
    BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22] - (RW) tx sleep protect rdy sw ctrl
    BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK[23] - (RW) tx sleep protect rdy mask
    BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE[24] - (RW) rx sleep protect rdy mode, 1: sw mode, 0: hw mode
    BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25] - (RW) rx sleep protect rdy sw ctrl
    BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK[26] - (RW) rx sleep protect rdy mask
    BT2CONN_GALS_SLPPROT_CTRL_MODE[27] - (RW) gals sleep protect mode, 1: sw, 0: hw
    BT2CONN_GALS_SLPPROT_CTRL_SW_EN[28] - (RW) sleep protect sw en
    BT2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29] - (RW) hw enable signal  0 mask
    BT2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30] - (RW) hw enable signal  1 mask
    BT2CONN_GALS_SLPPROT_CTRL_DUMMY[31] - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_DUMMY_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_DUMMY_MASK 0x80000000                // BT2CONN_GALS_SLPPROT_CTRL_DUMMY[31]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_DUMMY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK_MASK 0x40000000                // BT2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK_MASK 0x20000000                // BT2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_SW_EN_MASK 0x10000000                // BT2CONN_GALS_SLPPROT_CTRL_SW_EN[28]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_SW_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_MODE_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_MODE_MASK 0x08000000                // BT2CONN_GALS_SLPPROT_CTRL_MODE[27]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_MODE_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK_MASK 0x04000000                // BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK[26]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_MASK 0x02000000                // BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE_MASK 0x01000000                // BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE[24]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK_MASK 0x00800000                // BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK[23]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_MASK 0x00400000                // BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE_MASK 0x00200000                // BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE[21]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_MASK 0x00100000                // BT2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE_MASK 0x00080000                // BT2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE[19]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_MASK 0x00040000                // BT2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE_MASK 0x00020000                // BT2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE[17]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_FSM_RST_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_FSM_RST_MASK 0x00010000                // BT2CONN_GALS_SLPPROT_CTRL_FSM_RST[16]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_FSM_RST_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME_MASK 0x0000FC00                // BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR_MASK 0x00000200                // BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN_MASK 0x00000100                // BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN[8]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_BT2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000080                // CSR_BT2CONN_GALS_RX_SLP_PROT_SW_DIS[7]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_RX_SLP_PROT_SW_DIS_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000040                // CSR_BT2CONN_GALS_RX_SLP_PROT_SW_EN[6]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000020                // CFG_BT2CONN_GALS_RX_SLP_PROT_SW_DIS[5]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_RX_SLP_PROT_SW_DIS_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010                // CFG_BT2CONN_GALS_RX_SLP_PROT_SW_EN[4]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_BT2CONN_GALS_TX_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_TX_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_BT2CONN_GALS_TX_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CSR_BT2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_BT2CONN_GALS_TX_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_TX_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_BT2CONN_GALS_TX_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_BT2CONN_SLP_CTRL_CFG_BT2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---GALS_BT2CONN_SLP_STATUS (0x18001000 + 0x464)---

    RESERVED0[11..0]             - (RO) Reserved bits
    BT2CONN_GALS_CTRL_DIS_TIMEOUT[12] - (RO) bt2conn gals sleep protect contorol sleep protect disable timeout
    BT2CONN_GALS_CTRL_EN_TIMEOUT[13] - (RO) bt2conn gals sleep protect contorol sleep protect enable timeout
    RESERVED14[14]               - (RO) Reserved bits
    BT2CONN_GALS_CTRL_TIMEOUT[15] - (RO) bt2conn gals sleep protect contorol timeout
    RESERVED16[18..16]           - (RO) Reserved bits
    BT2CONN_GALS_CTRL_PROT_RDY_OUT[19] - (RO) bt2conn gals sleep protect control protect ready
    RESERVED20[21..20]           - (RO) Reserved bits
    BT2CONN_GALS_CTRL_PROT_RX_RDY[22] - (RO) bt2conn gals sleep protect rx ready
    BT2CONN_GALS_CTRL_PROT_TX_RDY[23] - (RO) bt2conn gals sleep protect tx ready
    RESERVED24[27..24]           - (RO) Reserved bits
    BT2CONN_GALS_CTRL_PROT_RX_EN[28] - (RO) bt2conn gals sleep protect control sleep protect rx enable
    BT2CONN_GALS_CTRL_PROT_TX_EN[29] - (RO) bt2conn gals sleep protect control sleep protect tx enable
    RESERVED30[30]               - (RO) Reserved bits
    BT2CONN_GALS_CTRL_PROT_EN[31] - (RO) bt2conn gals sleep protect control sleep protect enable

 =====================================================================================*/
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_EN_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_EN_MASK 0x80000000                // BT2CONN_GALS_CTRL_PROT_EN[31]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_EN_SHFT 31
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_TX_EN_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_TX_EN_MASK 0x20000000                // BT2CONN_GALS_CTRL_PROT_TX_EN[29]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_TX_EN_SHFT 29
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RX_EN_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RX_EN_MASK 0x10000000                // BT2CONN_GALS_CTRL_PROT_RX_EN[28]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RX_EN_SHFT 28
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_TX_RDY_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000                // BT2CONN_GALS_CTRL_PROT_TX_RDY[23]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RX_RDY_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000                // BT2CONN_GALS_CTRL_PROT_RX_RDY[22]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RX_RDY_SHFT 22
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RDY_OUT_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RDY_OUT_MASK 0x00080000                // BT2CONN_GALS_CTRL_PROT_RDY_OUT[19]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_PROT_RDY_OUT_SHFT 19
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_TIMEOUT_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_TIMEOUT_MASK 0x00008000                // BT2CONN_GALS_CTRL_TIMEOUT[15]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_TIMEOUT_SHFT 15
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_EN_TIMEOUT_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_EN_TIMEOUT_MASK 0x00002000                // BT2CONN_GALS_CTRL_EN_TIMEOUT[13]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_EN_TIMEOUT_SHFT 13
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_DIS_TIMEOUT_ADDR CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_DIS_TIMEOUT_MASK 0x00001000                // BT2CONN_GALS_CTRL_DIS_TIMEOUT[12]
#define CONN_CFG_ON_GALS_BT2CONN_SLP_STATUS_BT2CONN_GALS_CTRL_DIS_TIMEOUT_SHFT 12

/* =====================================================================================

  ---CONN_INFRA_CONN2GPS_SLP_CTRL (0x18001000 + 0x470)---

    CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_gals_conn2gps_tx sleep protect en
                                     0: conn_infra_gals_conn2gps_tx sleep protect en depends on disable control or hardware
    CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2gps_tx sleep protect disable
                                     0: conn_infra_gals_conn2gps_tx  sleep protect will control by hardware
    CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_gals_conn2gps_tx sleep protect en
                                     0: conn_infra_gals_conn2gps_tx sleep protect en depends on disable control or hardware
    CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2gps_tx sleep protect disable
                                     0: conn_infra_gals_conn2gps_tx  sleep protect will control by hardware
    CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN[4] - (RW) 1: force conn_infra_gals_conn2gps_rx sleep protect en
                                     0: conn_infra_gals_conn2gps_rx sleep protect en depends on disable control or hardware
    CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS[5] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2gps_rx sleep protect disable
                                     0: conn_infra_gals_conn2gps_rx  sleep protect will control by hardware
    CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_EN[6] - (RO) 1: force conn_infra_gals_conn2gps_rx sleep protect en
                                     0: conn_infra_gals_conn2gps_rx sleep protect en depends on disable control or hardware
    CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS[7] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_conn2gps_rx sleep protect disable
                                     0: conn_infra_gals_conn2gps_rx  sleep protect will control by hardware
    CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_EN[8] - (RW) timeout function enable
                                     1: enable
                                     0: disable
    CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9] - (RW) clear timeout irq, write 1 to clear, and need to write 0 after write 1
    CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10] - (RW) timeout timing set, count with 32k clock
    CONN2GPS_GALS_SLPPROT_CTRL_FSM_RST[16] - (RW) fsm_reset, 1:reset, 0: release
    CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_MODE[17] - (RW) tx sleep protect en mode, 1: sw mode, 0: hw mode
    CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18] - (RW) tx sleep protect sw ctrl
    CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_MODE[19] - (RW) rx sleep protect en mode, 1: sw mode, 0: hw mode
    CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20] - (RW) rx sleep protect sw ctrl
    CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MODE[21] - (RW) tx sleep protect rdy mode, 1: sw mode, 0: hw mode
    CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22] - (RW) tx sleep protect rdy sw ctrl
    CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MASK[23] - (RW) tx sleep protect rdy mask
    CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MODE[24] - (RW) rx sleep protect rdy mode, 1: sw mode, 0: hw mode
    CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25] - (RW) rx sleep protect rdy sw ctrl
    CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MASK[26] - (RW) rx sleep protect rdy mask
    CONN2GPS_GALS_SLPPROT_CTRL_MODE[27] - (RW) gals sleep protect mode, 1: sw, 0: hw
    CONN2GPS_GALS_SLPPROT_CTRL_SW_EN[28] - (RW) sleep protect sw en
    CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29] - (RW) hw enable signal  0 mask
    CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30] - (RW) hw enable signal  1 mask
    CONN2GPS_GALS_SLPPROT_CTRL_DUMMY[31] - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_DUMMY_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_DUMMY_MASK 0x80000000                // CONN2GPS_GALS_SLPPROT_CTRL_DUMMY[31]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_DUMMY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_1_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_1_MASK_MASK 0x40000000                // CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_1_MASK_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_0_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_0_MASK_MASK 0x20000000                // CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_HW_EN_0_MASK_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_SW_EN_MASK 0x10000000                // CONN2GPS_GALS_SLPPROT_CTRL_SW_EN[28]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_SW_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_MODE_MASK 0x08000000                // CONN2GPS_GALS_SLPPROT_CTRL_MODE[27]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_MODE_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MASK_MASK 0x04000000                // CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MASK[26]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MASK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_MASK 0x02000000                // CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MODE_MASK 0x01000000                // CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MODE[24]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_RDY_MODE_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MASK_MASK 0x00800000                // CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MASK[23]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MASK_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_MASK 0x00400000                // CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MODE_MASK 0x00200000                // CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MODE[21]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_RDY_MODE_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_MASK 0x00100000                // CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_MODE_MASK 0x00080000                // CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_MODE[19]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_RX_EN_MODE_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_MASK 0x00040000                // CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_MODE_MASK 0x00020000                // CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_MODE[17]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TX_EN_MODE_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_FSM_RST_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_FSM_RST_MASK 0x00010000                // CONN2GPS_GALS_SLPPROT_CTRL_FSM_RST[16]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_FSM_RST_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_TIME_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_TIME_MASK 0x0000FC00                // CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_TIME_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_CLR_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_CLR_MASK 0x00000200                // CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_CLR_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_EN_MASK 0x00000100                // CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_EN[8]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CONN2GPS_GALS_SLPPROT_CTRL_TIMEOUT_EN_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000080                // CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS[7]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000040                // CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_EN[6]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000020                // CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS[5]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_DIS_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010                // CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN[4]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CSR_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_CONN2GPS_SLP_CTRL_CFG_CONN2GPS_GALS_TX_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---GALS_CONN2GPS_SLP_STATUS (0x18001000 + 0x474)---

    RESERVED0[11..0]             - (RO) Reserved bits
    CONN2GPS_GALS_CTRL_DIS_TIMEOUT[12] - (RO) conn2gps gals sleep protect contorol sleep protect disable timeout
    CONN2GPS_GALS_CTRL_EN_TIMEOUT[13] - (RO) conn2gps gals sleep protect contorol sleep protect enable timeout
    RESERVED14[14]               - (RO) Reserved bits
    CONN2GPS_GALS_CTRL_TIMEOUT[15] - (RO) conn2gps gals sleep protect contorol timeout
    RESERVED16[18..16]           - (RO) Reserved bits
    CONN2GPS_GALS_CTRL_PROT_RDY_OUT[19] - (RO) conn2gps gals sleep protect control protect ready
    RESERVED20[21..20]           - (RO) Reserved bits
    CONN2GPS_GALS_CTRL_PROT_RX_RDY[22] - (RO) conn2gps gals sleep protect rx ready
    CONN2GPS_GALS_CTRL_PROT_TX_RDY[23] - (RO) conn2gps gals sleep protect tx ready
    RESERVED24[27..24]           - (RO) Reserved bits
    CONN2GPS_GALS_CTRL_PROT_RX_EN[28] - (RO) conn2gps gals sleep protect control sleep protect rx enable
    CONN2GPS_GALS_CTRL_PROT_TX_EN[29] - (RO) conn2gps gals sleep protect control sleep protect tx enable
    RESERVED30[30]               - (RO) Reserved bits
    CONN2GPS_GALS_CTRL_PROT_EN[31] - (RO) conn2gps gals sleep protect control sleep protect enable

 =====================================================================================*/
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_EN_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_EN_MASK 0x80000000                // CONN2GPS_GALS_CTRL_PROT_EN[31]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_EN_SHFT 31
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_EN_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_EN_MASK 0x20000000                // CONN2GPS_GALS_CTRL_PROT_TX_EN[29]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_EN_SHFT 29
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_EN_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_EN_MASK 0x10000000                // CONN2GPS_GALS_CTRL_PROT_RX_EN[28]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_EN_SHFT 28
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000                // CONN2GPS_GALS_CTRL_PROT_TX_RDY[23]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000                // CONN2GPS_GALS_CTRL_PROT_RX_RDY[22]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_SHFT 22
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RDY_OUT_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RDY_OUT_MASK 0x00080000                // CONN2GPS_GALS_CTRL_PROT_RDY_OUT[19]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RDY_OUT_SHFT 19
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_TIMEOUT_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_TIMEOUT_MASK 0x00008000                // CONN2GPS_GALS_CTRL_TIMEOUT[15]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_TIMEOUT_SHFT 15
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_EN_TIMEOUT_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_EN_TIMEOUT_MASK 0x00002000                // CONN2GPS_GALS_CTRL_EN_TIMEOUT[13]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_EN_TIMEOUT_SHFT 13
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_DIS_TIMEOUT_ADDR CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_DIS_TIMEOUT_MASK 0x00001000                // CONN2GPS_GALS_CTRL_DIS_TIMEOUT[12]
#define CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_DIS_TIMEOUT_SHFT 12

/* =====================================================================================

  ---CONN_INFRA_GPS2CONN_SLP_CTRL (0x18001000 + 0x480)---

    CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN[0] - (RW) 1: force conn_infra_gals_gps2conn_tx sleep protect en
                                     0: conn_infra_gals_gps2conn_tx sleep protect en depends on disable control or hardware
    CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS[1] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_gps2conn_tx sleep protect disable
                                     0: conn_infra_gals_gps2conn_tx  sleep protect will control by hardware
    CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_EN[2] - (RO) 1: force conn_infra_gals_gps2conn_tx sleep protect en
                                     0: conn_infra_gals_gps2conn_tx sleep protect en depends on disable control or hardware
    CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS[3] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_gps2conn_tx sleep protect disable
                                     0: conn_infra_gals_gps2conn_tx  sleep protect will control by hardware
    CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN[4] - (RW) 1: force conn_infra_gals_gps2conn_rx sleep protect en
                                     0: conn_infra_gals_gps2conn_rx sleep protect en depends on disable control or hardware
    CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS[5] - (RW) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_gps2conn_rx sleep protect disable
                                     0: conn_infra_gals_gps2conn_rx  sleep protect will control by hardware
    CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_EN[6] - (RO) 1: force conn_infra_gals_gps2conn_rx sleep protect en
                                     0: conn_infra_gals_gps2conn_rx sleep protect en depends on disable control or hardware
    CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS[7] - (RO) if force en == 1'b0 (SE_EN == 1'b0)
                                     1: conn_infra_gals_gps2conn_rx sleep protect disable
                                     0: conn_infra_gals_gps2conn_rx  sleep protect will control by hardware
    GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN[8] - (RW) timeout function enable
                                     1: enable
                                     0: disable
    GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9] - (RW) clear timeout irq, write 1 to clear, and need to write 0 after write 1
    GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10] - (RW) timeout timing set, count with 32k clock
    GPS2CONN_GALS_SLPPROT_CTRL_FSM_RST[16] - (RW) fsm_reset, 1:reset, 0: release
    GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE[17] - (RW) tx sleep protect en mode, 1: sw mode, 0: hw mode
    GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18] - (RW) tx sleep protect sw ctrl
    GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE[19] - (RW) rx sleep protect en mode, 1: sw mode, 0: hw mode
    GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20] - (RW) rx sleep protect sw ctrl
    GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE[21] - (RW) tx sleep protect rdy mode, 1: sw mode, 0: hw mode
    GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22] - (RW) tx sleep protect rdy sw ctrl
    GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK[23] - (RW) tx sleep protect rdy mask
    GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE[24] - (RW) rx sleep protect rdy mode, 1: sw mode, 0: hw mode
    GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25] - (RW) rx sleep protect rdy sw ctrl
    GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK[26] - (RW) rx sleep protect rdy mask
    GPS2CONN_GALS_SLPPROT_CTRL_MODE[27] - (RW) gals sleep protect mode, 1: sw, 0: hw
    GPS2CONN_GALS_SLPPROT_CTRL_SW_EN[28] - (RW) sleep protect sw en
    GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29] - (RW) hw enable signal  0 mask
    GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30] - (RW) hw enable signal  1 mask
    GPS2CONN_GALS_SLPPROT_CTRL_DUMMY[31] - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_DUMMY_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_DUMMY_MASK 0x80000000                // GPS2CONN_GALS_SLPPROT_CTRL_DUMMY[31]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_DUMMY_SHFT 31
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK_MASK 0x40000000                // GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK[30]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_1_MASK_SHFT 30
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK_MASK 0x20000000                // GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK[29]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_HW_EN_0_MASK_SHFT 29
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_SW_EN_MASK 0x10000000                // GPS2CONN_GALS_SLPPROT_CTRL_SW_EN[28]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_SW_EN_SHFT 28
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_MODE_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_MODE_MASK 0x08000000                // GPS2CONN_GALS_SLPPROT_CTRL_MODE[27]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_MODE_SHFT 27
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK_MASK 0x04000000                // GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK[26]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MASK_SHFT 26
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_MASK 0x02000000                // GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL[25]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_SW_CTRL_SHFT 25
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE_MASK 0x01000000                // GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE[24]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_RDY_MODE_SHFT 24
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK_MASK 0x00800000                // GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK[23]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MASK_SHFT 23
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_MASK 0x00400000                // GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL[22]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_SW_CTRL_SHFT 22
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE_MASK 0x00200000                // GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE[21]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_RDY_MODE_SHFT 21
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_MASK 0x00100000                // GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL[20]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_SW_CTRL_SHFT 20
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE_MASK 0x00080000                // GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE[19]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_RX_EN_MODE_SHFT 19
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_MASK 0x00040000                // GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL[18]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_SW_CTRL_SHFT 18
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE_MASK 0x00020000                // GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE[17]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TX_EN_MODE_SHFT 17
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_FSM_RST_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_FSM_RST_MASK 0x00010000                // GPS2CONN_GALS_SLPPROT_CTRL_FSM_RST[16]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_FSM_RST_SHFT 16
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME_MASK 0x0000FC00                // GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME[15..10]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_TIME_SHFT 10
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR_MASK 0x00000200                // GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR[9]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_CLR_SHFT 9
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN_MASK 0x00000100                // GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN[8]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_GPS2CONN_GALS_SLPPROT_CTRL_TIMEOUT_EN_SHFT 8
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000080                // CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS[7]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS_SHFT 7
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000040                // CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_EN[6]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 6
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS_MASK 0x00000020                // CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS[5]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_DIS_SHFT 5
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_MASK 0x00000010                // CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN[4]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_RX_SLP_PROT_SW_EN_SHFT 4
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000008                // CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS[3]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS_SHFT 3
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000004                // CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_EN[2]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CSR_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 2
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS_MASK 0x00000002                // CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS[1]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_DIS_SHFT 1
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_ADDR CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_ADDR
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_MASK 0x00000001                // CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN[0]
#define CONN_CFG_ON_CONN_INFRA_GPS2CONN_SLP_CTRL_CFG_GPS2CONN_GALS_TX_SLP_PROT_SW_EN_SHFT 0

/* =====================================================================================

  ---GALS_GPS2CONN_SLP_STATUS (0x18001000 + 0x484)---

    RESERVED0[11..0]             - (RO) Reserved bits
    GPS2CONN_GALS_CTRL_DIS_TIMEOUT[12] - (RO) gps2conn gals sleep protect contorol sleep protect disable timeout
    GPS2CONN_GALS_CTRL_EN_TIMEOUT[13] - (RO) gps2conn gals sleep protect contorol sleep protect enable timeout
    RESERVED14[14]               - (RO) Reserved bits
    GPS2CONN_GALS_CTRL_TIMEOUT[15] - (RO) gps2conn gals sleep protect contorol timeout
    RESERVED16[18..16]           - (RO) Reserved bits
    GPS2CONN_GALS_CTRL_PROT_RDY_OUT[19] - (RO) gps2conn gals sleep protect control protect ready
    RESERVED20[21..20]           - (RO) Reserved bits
    GPS2CONN_GALS_CTRL_PROT_RX_RDY[22] - (RO) gps2conn gals sleep protect rx ready
    GPS2CONN_GALS_CTRL_PROT_TX_RDY[23] - (RO) gps2conn gals sleep protect tx ready
    RESERVED24[27..24]           - (RO) Reserved bits
    GPS2CONN_GALS_CTRL_PROT_RX_EN[28] - (RO) gps2conn gals sleep protect control sleep protect rx enable
    GPS2CONN_GALS_CTRL_PROT_TX_EN[29] - (RO) gps2conn gals sleep protect control sleep protect tx enable
    RESERVED30[30]               - (RO) Reserved bits
    GPS2CONN_GALS_CTRL_PROT_EN[31] - (RO) gps2conn gals sleep protect control sleep protect enable

 =====================================================================================*/
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_EN_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_EN_MASK 0x80000000                // GPS2CONN_GALS_CTRL_PROT_EN[31]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_EN_SHFT 31
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_EN_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_EN_MASK 0x20000000                // GPS2CONN_GALS_CTRL_PROT_TX_EN[29]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_EN_SHFT 29
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_EN_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_EN_MASK 0x10000000                // GPS2CONN_GALS_CTRL_PROT_RX_EN[28]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_EN_SHFT 28
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_RDY_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_RDY_MASK 0x00800000                // GPS2CONN_GALS_CTRL_PROT_TX_RDY[23]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_TX_RDY_SHFT 23
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_RDY_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_RDY_MASK 0x00400000                // GPS2CONN_GALS_CTRL_PROT_RX_RDY[22]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RX_RDY_SHFT 22
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RDY_OUT_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RDY_OUT_MASK 0x00080000                // GPS2CONN_GALS_CTRL_PROT_RDY_OUT[19]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_PROT_RDY_OUT_SHFT 19
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_TIMEOUT_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_TIMEOUT_MASK 0x00008000                // GPS2CONN_GALS_CTRL_TIMEOUT[15]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_TIMEOUT_SHFT 15
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_EN_TIMEOUT_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_EN_TIMEOUT_MASK 0x00002000                // GPS2CONN_GALS_CTRL_EN_TIMEOUT[13]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_EN_TIMEOUT_SHFT 13
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_DIS_TIMEOUT_ADDR CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_ADDR
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_DIS_TIMEOUT_MASK 0x00001000                // GPS2CONN_GALS_CTRL_DIS_TIMEOUT[12]
#define CONN_CFG_ON_GALS_GPS2CONN_SLP_STATUS_GPS2CONN_GALS_CTRL_DIS_TIMEOUT_SHFT 12

/* =====================================================================================

  ---HOST_CSR_IRQ_EN (0x18001000 + 0x600)---

    CONN_WF_B0_CR_HOST_SET_FW_OWN_IRQ_EN[0] - (RW) 1'b0 : Host set fw own irq to wf band0 disable
                                     1'b1 : Host set fw own irq to bgf band0 enable
    CONN_WF_B0_CR_HOST_CLR_FW_OWN_IRQ_EN[1] - (RW) 1'b0 : Host clear fw own irq to wf band0 disable
                                     1'b1 : Host clear fw own irq to bgf band0 enable
    CONN_WF_B1_CR_HOST_SET_FW_OWN_IRQ_EN[2] - (RW) 1'b0 : Host set fw own irq to wf band1 disable
                                     1'b1 : Host set fw own irq to bgf band1 enable
    CONN_WF_B1_CR_HOST_CLR_FW_OWN_IRQ_EN[3] - (RW) 1'b0 : Host clear fw own irq to wf band1 disable
                                     1'b1 : Host clear fw own irq to bgf band1 enable
    CONN_CR_MD_SET_FW_OWN_IRQ_EN[4] - (RW)  xxx 
    CONN_CR_MD_CLR_FW_OWN_IRQ_EN[5] - (RW)  xxx 
    CONN_BGF_CR_HOST_SET_FW_OWN_IRQ_EN[6] - (RW) 1'b0 : Host set fw own irq to bgf disable
                                     1'b1 : Host set fw own irq to bgf enable
    CONN_BGF_CR_HOST_CLR_FW_OWN_IRQ_EN[7] - (RW) 1'b0 : Host clear fw own irq to bgf disable
                                     1'b1 : Host clear fw own irq to bgf enable
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_BGF_CR_HOST_CLR_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_BGF_CR_HOST_CLR_FW_OWN_IRQ_EN_MASK 0x00000080                // CONN_BGF_CR_HOST_CLR_FW_OWN_IRQ_EN[7]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_BGF_CR_HOST_CLR_FW_OWN_IRQ_EN_SHFT 7
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_BGF_CR_HOST_SET_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_BGF_CR_HOST_SET_FW_OWN_IRQ_EN_MASK 0x00000040                // CONN_BGF_CR_HOST_SET_FW_OWN_IRQ_EN[6]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_BGF_CR_HOST_SET_FW_OWN_IRQ_EN_SHFT 6
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_CR_MD_CLR_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_CR_MD_CLR_FW_OWN_IRQ_EN_MASK 0x00000020                // CONN_CR_MD_CLR_FW_OWN_IRQ_EN[5]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_CR_MD_CLR_FW_OWN_IRQ_EN_SHFT 5
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_CR_MD_SET_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_CR_MD_SET_FW_OWN_IRQ_EN_MASK 0x00000010                // CONN_CR_MD_SET_FW_OWN_IRQ_EN[4]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_CR_MD_SET_FW_OWN_IRQ_EN_SHFT 4
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B1_CR_HOST_CLR_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B1_CR_HOST_CLR_FW_OWN_IRQ_EN_MASK 0x00000008                // CONN_WF_B1_CR_HOST_CLR_FW_OWN_IRQ_EN[3]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B1_CR_HOST_CLR_FW_OWN_IRQ_EN_SHFT 3
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B1_CR_HOST_SET_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B1_CR_HOST_SET_FW_OWN_IRQ_EN_MASK 0x00000004                // CONN_WF_B1_CR_HOST_SET_FW_OWN_IRQ_EN[2]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B1_CR_HOST_SET_FW_OWN_IRQ_EN_SHFT 2
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B0_CR_HOST_CLR_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B0_CR_HOST_CLR_FW_OWN_IRQ_EN_MASK 0x00000002                // CONN_WF_B0_CR_HOST_CLR_FW_OWN_IRQ_EN[1]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B0_CR_HOST_CLR_FW_OWN_IRQ_EN_SHFT 1
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B0_CR_HOST_SET_FW_OWN_IRQ_EN_ADDR CONN_CFG_ON_HOST_CSR_IRQ_EN_ADDR
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B0_CR_HOST_SET_FW_OWN_IRQ_EN_MASK 0x00000001                // CONN_WF_B0_CR_HOST_SET_FW_OWN_IRQ_EN[0]
#define CONN_CFG_ON_HOST_CSR_IRQ_EN_CONN_WF_B0_CR_HOST_SET_FW_OWN_IRQ_EN_SHFT 0

/* =====================================================================================

  ---CSR_BGF_ON_FW_OWN_IRQ (0x18001000 + 0x604)---

    CSR_BGF_ON_FW_OWN_IRQ[0]     - (W1C) Fw write 1 to generate irq to host (bgf)
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_BGF_ON_FW_OWN_IRQ_CSR_BGF_ON_FW_OWN_IRQ_ADDR CONN_CFG_ON_CSR_BGF_ON_FW_OWN_IRQ_ADDR
#define CONN_CFG_ON_CSR_BGF_ON_FW_OWN_IRQ_CSR_BGF_ON_FW_OWN_IRQ_MASK 0x00000001                // CSR_BGF_ON_FW_OWN_IRQ[0]
#define CONN_CFG_ON_CSR_BGF_ON_FW_OWN_IRQ_CSR_BGF_ON_FW_OWN_IRQ_SHFT 0

/* =====================================================================================

  ---CSR_MD_ON_IRQ_STATUS (0x18001000 + 0x610)---

    CONN_MD_HOST_SET_FW_OWN_STS[0] - (W1C)  xxx 
    CONN_MD_HOST_CLR_FW_OWN_STS[1] - (W1C)  xxx 
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_CONN_MD_HOST_CLR_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_CONN_MD_HOST_CLR_FW_OWN_STS_MASK 0x00000002                // CONN_MD_HOST_CLR_FW_OWN_STS[1]
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_CONN_MD_HOST_CLR_FW_OWN_STS_SHFT 1
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_CONN_MD_HOST_SET_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_CONN_MD_HOST_SET_FW_OWN_STS_MASK 0x00000001                // CONN_MD_HOST_SET_FW_OWN_STS[0]
#define CONN_CFG_ON_CSR_MD_ON_IRQ_STATUS_CONN_MD_HOST_SET_FW_OWN_STS_SHFT 0

/* =====================================================================================

  ---CSR_MD_ON_HOST_CSR_MISC (0x18001000 + 0x614)---

    CONN_MD_HOST_LPCR_FW_OWN[0]  - (W1C)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_MD_ON_HOST_CSR_MISC_CONN_MD_HOST_LPCR_FW_OWN_ADDR CONN_CFG_ON_CSR_MD_ON_HOST_CSR_MISC_ADDR
#define CONN_CFG_ON_CSR_MD_ON_HOST_CSR_MISC_CONN_MD_HOST_LPCR_FW_OWN_MASK 0x00000001                // CONN_MD_HOST_LPCR_FW_OWN[0]
#define CONN_CFG_ON_CSR_MD_ON_HOST_CSR_MISC_CONN_MD_HOST_LPCR_FW_OWN_SHFT 0

/* =====================================================================================

  ---CSR_WF_B0_ON_IRQ_STATUS (0x18001000 + 0x620)---

    CONN_WF_B0_HOST_SET_FW_OWN_STS[0] - (W1C) Fw write 1 to clr irq which cause by host set fw own to wf band0
    CONN_WF_B0_HOST_CLR_FW_OWN_STS[1] - (W1C) Fw write 1 to clr irq which cause by host clr fw own to wf band0
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_CONN_WF_B0_HOST_CLR_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_CONN_WF_B0_HOST_CLR_FW_OWN_STS_MASK 0x00000002                // CONN_WF_B0_HOST_CLR_FW_OWN_STS[1]
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_CONN_WF_B0_HOST_CLR_FW_OWN_STS_SHFT 1
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_CONN_WF_B0_HOST_SET_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_CONN_WF_B0_HOST_SET_FW_OWN_STS_MASK 0x00000001                // CONN_WF_B0_HOST_SET_FW_OWN_STS[0]
#define CONN_CFG_ON_CSR_WF_B0_ON_IRQ_STATUS_CONN_WF_B0_HOST_SET_FW_OWN_STS_SHFT 0

/* =====================================================================================

  ---CSR_WF_B0_ON_HOST_CSR_MISC (0x18001000 + 0x624)---

    CONN_WF_B0_HOST_LPCR_FW_OWN[0] - (W1C) Fw write 1 to set driver own state cr (band0) and generate irq to host
                                     Fw could read this cr to check state
                                     1'b0:Driver own
                                     1'b1:Fw own
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_WF_B0_ON_HOST_CSR_MISC_CONN_WF_B0_HOST_LPCR_FW_OWN_ADDR CONN_CFG_ON_CSR_WF_B0_ON_HOST_CSR_MISC_ADDR
#define CONN_CFG_ON_CSR_WF_B0_ON_HOST_CSR_MISC_CONN_WF_B0_HOST_LPCR_FW_OWN_MASK 0x00000001                // CONN_WF_B0_HOST_LPCR_FW_OWN[0]
#define CONN_CFG_ON_CSR_WF_B0_ON_HOST_CSR_MISC_CONN_WF_B0_HOST_LPCR_FW_OWN_SHFT 0

/* =====================================================================================

  ---CSR_WF_B1_ON_IRQ_STATUS (0x18001000 + 0x630)---

    CONN_WF_B1_HOST_SET_FW_OWN_STS[0] - (W1C) Fw write 1 to clr irq which cause by host set fw own to wf band1
    CONN_WF_B1_HOST_CLR_FW_OWN_STS[1] - (W1C) Fw write 1 to clr irq which cause by host clr fw own to wf band1
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_CONN_WF_B1_HOST_CLR_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_CONN_WF_B1_HOST_CLR_FW_OWN_STS_MASK 0x00000002                // CONN_WF_B1_HOST_CLR_FW_OWN_STS[1]
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_CONN_WF_B1_HOST_CLR_FW_OWN_STS_SHFT 1
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_CONN_WF_B1_HOST_SET_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_CONN_WF_B1_HOST_SET_FW_OWN_STS_MASK 0x00000001                // CONN_WF_B1_HOST_SET_FW_OWN_STS[0]
#define CONN_CFG_ON_CSR_WF_B1_ON_IRQ_STATUS_CONN_WF_B1_HOST_SET_FW_OWN_STS_SHFT 0

/* =====================================================================================

  ---CSR_WF_B1_ON_HOST_CSR_MISC (0x18001000 + 0x634)---

    CONN_WF_B1_HOST_LPCR_FW_OWN[0] - (W1C) Fw write 1 to set driver own state cr (band1) and generate irq to host
                                     Fw could read this cr to check state
                                     1'b0:Driver own
                                     1'b1:Fw own
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_WF_B1_ON_HOST_CSR_MISC_CONN_WF_B1_HOST_LPCR_FW_OWN_ADDR CONN_CFG_ON_CSR_WF_B1_ON_HOST_CSR_MISC_ADDR
#define CONN_CFG_ON_CSR_WF_B1_ON_HOST_CSR_MISC_CONN_WF_B1_HOST_LPCR_FW_OWN_MASK 0x00000001                // CONN_WF_B1_HOST_LPCR_FW_OWN[0]
#define CONN_CFG_ON_CSR_WF_B1_ON_HOST_CSR_MISC_CONN_WF_B1_HOST_LPCR_FW_OWN_SHFT 0

/* =====================================================================================

  ---CSR_BGF_ON_IRQ_STATUS (0x18001000 + 0x640)---

    CONN_BGF_HOST_SET_FW_OWN_STS[0] - (W1C) Fw write 1 to clr irq which cause by host set fw own to bgf
    CONN_BGF_HOST_CLR_FW_OWN_STS[1] - (W1C) Fw write 1 to clr irq which cause by host clr fw own to bgf
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_CONN_BGF_HOST_CLR_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_CONN_BGF_HOST_CLR_FW_OWN_STS_MASK 0x00000002                // CONN_BGF_HOST_CLR_FW_OWN_STS[1]
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_CONN_BGF_HOST_CLR_FW_OWN_STS_SHFT 1
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_CONN_BGF_HOST_SET_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_CONN_BGF_HOST_SET_FW_OWN_STS_MASK 0x00000001                // CONN_BGF_HOST_SET_FW_OWN_STS[0]
#define CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_CONN_BGF_HOST_SET_FW_OWN_STS_SHFT 0

/* =====================================================================================

  ---CSR_BGF_ON_HOST_CSR_MISC (0x18001000 + 0x644)---

    CONN_BGF_HOST_LPCR_FW_OWN[0] - (W1C) Fw write 1 to set driver own state cr (bgf) and generate irq to host
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_BGF_ON_HOST_CSR_MISC_CONN_BGF_HOST_LPCR_FW_OWN_ADDR CONN_CFG_ON_CSR_BGF_ON_HOST_CSR_MISC_ADDR
#define CONN_CFG_ON_CSR_BGF_ON_HOST_CSR_MISC_CONN_BGF_HOST_LPCR_FW_OWN_MASK 0x00000001                // CONN_BGF_HOST_LPCR_FW_OWN[0]
#define CONN_CFG_ON_CSR_BGF_ON_HOST_CSR_MISC_CONN_BGF_HOST_LPCR_FW_OWN_SHFT 0

/* =====================================================================================

  ---CSR_GPS_ON_IRQ_STATUS (0x18001000 + 0x650)---

    CONN_GPS_HOST_SET_FW_OWN_STS[0] - (W1C) Fw write 1 to clr irq which cause by host set fw own to gps
    CONN_GPS_HOST_CLR_FW_OWN_STS[1] - (W1C) Fw write 1 to clr irq which cause by host clr fw own to gps
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_CONN_GPS_HOST_CLR_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_CONN_GPS_HOST_CLR_FW_OWN_STS_MASK 0x00000002                // CONN_GPS_HOST_CLR_FW_OWN_STS[1]
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_CONN_GPS_HOST_CLR_FW_OWN_STS_SHFT 1
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_CONN_GPS_HOST_SET_FW_OWN_STS_ADDR CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_ADDR
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_CONN_GPS_HOST_SET_FW_OWN_STS_MASK 0x00000001                // CONN_GPS_HOST_SET_FW_OWN_STS[0]
#define CONN_CFG_ON_CSR_GPS_ON_IRQ_STATUS_CONN_GPS_HOST_SET_FW_OWN_STS_SHFT 0

/* =====================================================================================

  ---CSR_GPS_ON_HOST_CSR_MISC (0x18001000 + 0x654)---

    CONN_GPS_HOST_LPCR_FW_OWN[0] - (W1C) Fw write 1 to set driver own state cr (gps) and generate irq to host
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_ON_CSR_GPS_ON_HOST_CSR_MISC_CONN_GPS_HOST_LPCR_FW_OWN_ADDR CONN_CFG_ON_CSR_GPS_ON_HOST_CSR_MISC_ADDR
#define CONN_CFG_ON_CSR_GPS_ON_HOST_CSR_MISC_CONN_GPS_HOST_LPCR_FW_OWN_MASK 0x00000001                // CONN_GPS_HOST_LPCR_FW_OWN[0]
#define CONN_CFG_ON_CSR_GPS_ON_HOST_CSR_MISC_CONN_GPS_HOST_LPCR_FW_OWN_SHFT 0

#endif // __CONN_CFG_ON_REGS_H__
