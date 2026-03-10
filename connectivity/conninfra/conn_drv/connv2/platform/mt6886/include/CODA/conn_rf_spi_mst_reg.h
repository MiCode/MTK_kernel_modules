
/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_rf_spi_mst_reg.h
//[Revision time]   : Thu Apr  8 11:23:15 2021

#ifndef __CONN_RF_SPI_MST_REG_REGS_H__
#define __CONN_RF_SPI_MST_REG_REGS_H__

//****************************************************************************
//
//                     CONN_RF_SPI_MST_REG CR Definitions                     
//
//****************************************************************************

#define CONN_RF_SPI_MST_REG_BASE                               (CONN_REG_CONN_RF_SPI_MST_REG_ADDR)

#define CONN_RF_SPI_MST_REG_SPI_STA_ADDR                       (CONN_RF_SPI_MST_REG_BASE + 0x000) // 2000
#define CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x004) // 2004
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_ADDR                     (CONN_RF_SPI_MST_REG_BASE + 0x008) // 2008
#define CONN_RF_SPI_MST_REG_FM_CTRL_ADDR                       (CONN_RF_SPI_MST_REG_BASE + 0x00C) // 200C
#define CONN_RF_SPI_MST_REG_SPI_WF_ADDR_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x010) // 2010
#define CONN_RF_SPI_MST_REG_SPI_WF_WDAT_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x014) // 2014
#define CONN_RF_SPI_MST_REG_SPI_WF_RDAT_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x018) // 2018
#define CONN_RF_SPI_MST_REG_SPI_BT_ADDR_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x020) // 2020
#define CONN_RF_SPI_MST_REG_SPI_BT_WDAT_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x024) // 2024
#define CONN_RF_SPI_MST_REG_SPI_BT_RDAT_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x028) // 2028
#define CONN_RF_SPI_MST_REG_SPI_FM_ADDR_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x030) // 2030
#define CONN_RF_SPI_MST_REG_SPI_FM_WDAT_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x034) // 2034
#define CONN_RF_SPI_MST_REG_SPI_FM_RDAT_ADDR                   (CONN_RF_SPI_MST_REG_BASE + 0x038) // 2038
#define CONN_RF_SPI_MST_REG_SPI_GPS_ADDR_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x040) // 2040
#define CONN_RF_SPI_MST_REG_SPI_GPS_WDAT_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x044) // 2044
#define CONN_RF_SPI_MST_REG_SPI_GPS_RDAT_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x048) // 2048
#define CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x050) // 2050
#define CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x054) // 2054
#define CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x058) // 2058
#define CONN_RF_SPI_MST_REG_SPI_SLP_ADDR_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x060) // 2060
#define CONN_RF_SPI_MST_REG_SLP_WDAT_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x064) // 2064
#define CONN_RF_SPI_MST_REG_SLP_RDAT_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x068) // 2068
#define CONN_RF_SPI_MST_REG_THM_ADDR_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x070) // 2070
#define CONN_RF_SPI_MST_REG_THM_WDAT_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x074) // 2074
#define CONN_RF_SPI_MST_REG_THM_RDAT_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x078) // 2078
#define CONN_RF_SPI_MST_REG_SPI_INT_ADDR                       (CONN_RF_SPI_MST_REG_BASE + 0x080) // 2080
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_ADDR                (CONN_RF_SPI_MST_REG_BASE + 0x084) // 2084
#define CONN_RF_SPI_MST_REG_SPI_DBG_ADDR                       (CONN_RF_SPI_MST_REG_BASE + 0x090) // 2090
#define CONN_RF_SPI_MST_REG_SPI_STA1_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x094) // 2094
#define CONN_RF_SPI_MST_REG_SPI_STA2_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x098) // 2098
#define CONN_RF_SPI_MST_REG_SPI_STA3_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x09C) // 209C
#define CONN_RF_SPI_MST_REG_SPI_STA4_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x100) // 2100
#define CONN_RF_SPI_MST_REG_SPI_STA5_ADDR                      (CONN_RF_SPI_MST_REG_BASE + 0x104) // 2104
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR                  (CONN_RF_SPI_MST_REG_BASE + 0x108) // 2108
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR              (CONN_RF_SPI_MST_REG_BASE + 0x10C) // 210C
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR              (CONN_RF_SPI_MST_REG_BASE + 0x110) // 2110
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_ADDR              (CONN_RF_SPI_MST_REG_BASE + 0x210) // 2210
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_ADDR              (CONN_RF_SPI_MST_REG_BASE + 0x214) // 2214
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_ADDR              (CONN_RF_SPI_MST_REG_BASE + 0x218) // 2218
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_ADDR_ADDR            (CONN_RF_SPI_MST_REG_BASE + 0x220) // 2220
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_WDAT_ADDR            (CONN_RF_SPI_MST_REG_BASE + 0x224) // 2224
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_RDAT_ADDR            (CONN_RF_SPI_MST_REG_BASE + 0x228) // 2228
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_0_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x300) // 2300
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_1_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x304) // 2304
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_2_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x308) // 2308
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_3_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x30C) // 230C
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_0_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x310) // 2310
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_1_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x314) // 2314
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_2_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x318) // 2318
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_3_ADDR         (CONN_RF_SPI_MST_REG_BASE + 0x31C) // 231C
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_ADDR            (CONN_RF_SPI_MST_REG_BASE + 0x320) // 2320




/* =====================================================================================

  ---SPI_STA (0x18042000 + 0x000)---

    RSPI_BUSY[0]                 - (RO) It means SPI bus is busy. This is "OR" function of GPS, FM, BT, WF and HW(sleep control) busy flag.
    WF_BUSY[1]                   - (RO) When MCU/APB set WF write data register, this bit will be high. It will clear to low until read or write instruction finished.
    BT_BUSY[2]                   - (RO) When MCU/APB set BT write data register, this bit will be high. It will clear to low until read or write instruction finished.
    FM_BUSY[3]                   - (RO) When MCU/APB set FM write data register, this bit will be high. It will clear to low until read or write instruction finished.
    GPS_BUSY[4]                  - (RO) When GPS/DSP set GPS write data register, this bit will be high. It will clear to low until read or write instruction finished.
    TOP_BUSY[5]                  - (RO) When MCU/APB set TOP_MISC write data register, this bit will be high. It will clear to low until read or write instruction finished.
    HW_BUSY[6]                   - (RO) When Sleep Control set write data register, this bit will be high. It will clear to low until read or write instruction finished.
    THM_BUSY[7]                  - (RO) When Thermal Control set write data register, this bit will be high. It will clear to low until read or write instruction finished.
    RFDIG_BUSY[8]                - (RO) When GPS DSP set the GPS RF Low Write Byte register, this bit will be high. It will clear to low until read or write instruction finished.
    FMAUD_BUSY[9]                - (RO) When FM AUD set the FM AUD Low Write Byte register, this bit will be high. It will clear to low until read or write instruction finished.
    RESERVED10[31..10]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_STA_FMAUD_BUSY_ADDR            CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_FMAUD_BUSY_MASK            0x00000200                // FMAUD_BUSY[9]
#define CONN_RF_SPI_MST_REG_SPI_STA_FMAUD_BUSY_SHFT            9
#define CONN_RF_SPI_MST_REG_SPI_STA_RFDIG_BUSY_ADDR            CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_RFDIG_BUSY_MASK            0x00000100                // RFDIG_BUSY[8]
#define CONN_RF_SPI_MST_REG_SPI_STA_RFDIG_BUSY_SHFT            8
#define CONN_RF_SPI_MST_REG_SPI_STA_THM_BUSY_ADDR              CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_THM_BUSY_MASK              0x00000080                // THM_BUSY[7]
#define CONN_RF_SPI_MST_REG_SPI_STA_THM_BUSY_SHFT              7
#define CONN_RF_SPI_MST_REG_SPI_STA_HW_BUSY_ADDR               CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_HW_BUSY_MASK               0x00000040                // HW_BUSY[6]
#define CONN_RF_SPI_MST_REG_SPI_STA_HW_BUSY_SHFT               6
#define CONN_RF_SPI_MST_REG_SPI_STA_TOP_BUSY_ADDR              CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_TOP_BUSY_MASK              0x00000020                // TOP_BUSY[5]
#define CONN_RF_SPI_MST_REG_SPI_STA_TOP_BUSY_SHFT              5
#define CONN_RF_SPI_MST_REG_SPI_STA_GPS_BUSY_ADDR              CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_GPS_BUSY_MASK              0x00000010                // GPS_BUSY[4]
#define CONN_RF_SPI_MST_REG_SPI_STA_GPS_BUSY_SHFT              4
#define CONN_RF_SPI_MST_REG_SPI_STA_FM_BUSY_ADDR               CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_FM_BUSY_MASK               0x00000008                // FM_BUSY[3]
#define CONN_RF_SPI_MST_REG_SPI_STA_FM_BUSY_SHFT               3
#define CONN_RF_SPI_MST_REG_SPI_STA_BT_BUSY_ADDR               CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_BT_BUSY_MASK               0x00000004                // BT_BUSY[2]
#define CONN_RF_SPI_MST_REG_SPI_STA_BT_BUSY_SHFT               2
#define CONN_RF_SPI_MST_REG_SPI_STA_WF_BUSY_ADDR               CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_WF_BUSY_MASK               0x00000002                // WF_BUSY[1]
#define CONN_RF_SPI_MST_REG_SPI_STA_WF_BUSY_SHFT               1
#define CONN_RF_SPI_MST_REG_SPI_STA_RSPI_BUSY_ADDR             CONN_RF_SPI_MST_REG_SPI_STA_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA_RSPI_BUSY_MASK             0x00000001                // RSPI_BUSY[0]
#define CONN_RF_SPI_MST_REG_SPI_STA_RSPI_BUSY_SHFT             0

/* =====================================================================================

  ---SPI_CRTL (0x18042000 + 0x004)---

    SPI_HS_EN[0]                 - (RO) spi clock switch
    AUTO_K_EN[1]                 - (RW) auto calibration enable
    OSC_CAP_NEXT[2]              - (RW) Spi2_di capture phase in osc clock
    CLK_ACTIVE_STS[4..3]         - (RO) Glitch free mux status
    HS_CAP_NEXT[5]               - (RW) Spi2_di capture phase in high speed clock
    RESERVED0[8..6]              - (RW) Reserved bit. Do not used.
    DIS_REQ_ON[9]                - (RW) Enable Req
    RESERVED2[10]                - (RW) Reserved bit. Do not used.
    REQ_RST[11]                  - (RW) Arbiter slot manual reset. Write "1" to reset and "0" to release.
    AUTO_RST_EN[12]              - (RW) Arbiter slot automatically reset to initial value.
    RESERVED1[13]                - (RW) Reserved bit. Do not used.
    FIX_HP_EN[14]                - (RW) Reset arbiter slot zero flag.
    MASTER_EN[15]                - (RW) RFSPI Master Enable
    SPI_CK_MANUAL_MODE[16]       - (RW) SPI CK manual mode enable
    SPI_DO_MANUAL_MODE[17]       - (RW) SPI CK manual mode enable
    SPI_CK_MANUAL[18]            - (RW) rf_spi_ck output value
    SPI_DO_MANUAL[19]            - (RW) rf_spi_do output value
    FAKE_NACK_INT[20]            - (W1C) status of CR trigger fake nack interrupt. Write "1" can clear the status.
    INT_MASK[24..21]             - (RW) Interrupt mask
                                     [0]: arb slot int
                                     [1]: addr nack int
                                     [2]: data nack int
                                     [3]: auto k int
    OSC_DIV_CK_EN[25]            - (RW) xtal clock sel
    OSC_CLK_ACTIVE_STS[27..26]   - (RO) OSC Glitch free mux status
    SPI_OE_MANUAL[28]            - (RW) rf_spi_di_oe output value
    SPI_DI_LATCH[29]             - (RO) rf_spi_di value, latch by rf_spi_uclk
    AUTOK_BYPASS_64M[30]         - (RW) auto-K flow bypass 64MHz setting
    MCU_BUSY_CLR[31]             - (W1C) force clear all busy flags.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_CRTL_MCU_BUSY_CLR_ADDR         CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_MCU_BUSY_CLR_MASK         0x80000000                // MCU_BUSY_CLR[31]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_MCU_BUSY_CLR_SHFT         31
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTOK_BYPASS_64M_ADDR     CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTOK_BYPASS_64M_MASK     0x40000000                // AUTOK_BYPASS_64M[30]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTOK_BYPASS_64M_SHFT     30
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DI_LATCH_ADDR         CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DI_LATCH_MASK         0x20000000                // SPI_DI_LATCH[29]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DI_LATCH_SHFT         29
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_OE_MANUAL_ADDR        CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_OE_MANUAL_MASK        0x10000000                // SPI_OE_MANUAL[28]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_OE_MANUAL_SHFT        28
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_CLK_ACTIVE_STS_ADDR   CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_CLK_ACTIVE_STS_MASK   0x0C000000                // OSC_CLK_ACTIVE_STS[27..26]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_CLK_ACTIVE_STS_SHFT   26
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_DIV_CK_EN_ADDR        CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_DIV_CK_EN_MASK        0x02000000                // OSC_DIV_CK_EN[25]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_DIV_CK_EN_SHFT        25
#define CONN_RF_SPI_MST_REG_SPI_CRTL_INT_MASK_ADDR             CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_INT_MASK_MASK             0x01E00000                // INT_MASK[24..21]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_INT_MASK_SHFT             21
#define CONN_RF_SPI_MST_REG_SPI_CRTL_FAKE_NACK_INT_ADDR        CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_FAKE_NACK_INT_MASK        0x00100000                // FAKE_NACK_INT[20]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_FAKE_NACK_INT_SHFT        20
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DO_MANUAL_ADDR        CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DO_MANUAL_MASK        0x00080000                // SPI_DO_MANUAL[19]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DO_MANUAL_SHFT        19
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_CK_MANUAL_ADDR        CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_CK_MANUAL_MASK        0x00040000                // SPI_CK_MANUAL[18]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_CK_MANUAL_SHFT        18
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DO_MANUAL_MODE_ADDR   CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DO_MANUAL_MODE_MASK   0x00020000                // SPI_DO_MANUAL_MODE[17]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_DO_MANUAL_MODE_SHFT   17
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_CK_MANUAL_MODE_ADDR   CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_CK_MANUAL_MODE_MASK   0x00010000                // SPI_CK_MANUAL_MODE[16]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_CK_MANUAL_MODE_SHFT   16
#define CONN_RF_SPI_MST_REG_SPI_CRTL_MASTER_EN_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_MASTER_EN_MASK            0x00008000                // MASTER_EN[15]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_MASTER_EN_SHFT            15
#define CONN_RF_SPI_MST_REG_SPI_CRTL_FIX_HP_EN_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_FIX_HP_EN_MASK            0x00004000                // FIX_HP_EN[14]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_FIX_HP_EN_SHFT            14
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED1_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED1_MASK            0x00002000                // RESERVED1[13]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED1_SHFT            13
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTO_RST_EN_ADDR          CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTO_RST_EN_MASK          0x00001000                // AUTO_RST_EN[12]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTO_RST_EN_SHFT          12
#define CONN_RF_SPI_MST_REG_SPI_CRTL_REQ_RST_ADDR              CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_REQ_RST_MASK              0x00000800                // REQ_RST[11]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_REQ_RST_SHFT              11
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED2_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED2_MASK            0x00000400                // RESERVED2[10]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED2_SHFT            10
#define CONN_RF_SPI_MST_REG_SPI_CRTL_DIS_REQ_ON_ADDR           CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_DIS_REQ_ON_MASK           0x00000200                // DIS_REQ_ON[9]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_DIS_REQ_ON_SHFT           9
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED0_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED0_MASK            0x000001C0                // RESERVED0[8..6]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_RESERVED0_SHFT            6
#define CONN_RF_SPI_MST_REG_SPI_CRTL_HS_CAP_NEXT_ADDR          CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_HS_CAP_NEXT_MASK          0x00000020                // HS_CAP_NEXT[5]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_HS_CAP_NEXT_SHFT          5
#define CONN_RF_SPI_MST_REG_SPI_CRTL_CLK_ACTIVE_STS_ADDR       CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_CLK_ACTIVE_STS_MASK       0x00000018                // CLK_ACTIVE_STS[4..3]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_CLK_ACTIVE_STS_SHFT       3
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_CAP_NEXT_ADDR         CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_CAP_NEXT_MASK         0x00000004                // OSC_CAP_NEXT[2]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_OSC_CAP_NEXT_SHFT         2
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTO_K_EN_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTO_K_EN_MASK            0x00000002                // AUTO_K_EN[1]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_AUTO_K_EN_SHFT            1
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_HS_EN_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_HS_EN_MASK            0x00000001                // SPI_HS_EN[0]
#define CONN_RF_SPI_MST_REG_SPI_CRTL_SPI_HS_EN_SHFT            0

/* =====================================================================================

  ---SPI_CRTL2 (0x18042000 + 0x008)---

    RESERVED0[7..0]              - (RO) Reserved bits
    MST_PHASE_CK1[15..8]         - (RW) For SPI_CLK64M=1 (CK = 64MH). software control of delay chain selection.
                                     The setting will be applied if AUTO_K_EN=0.
    MST_PHASE_CK0[23..16]        - (RW) For SPI_CLK64M=0 (CK = 26MH).software control of delay chain selection.
                                     The setting will be applied if AUTO_K_EN=0.
    PHASE_TH[28..24]             - (RW) sufficient threshold of phase difference
                                     at least 1
    RESERVED29[31..29]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_PHASE_TH_ADDR            CONN_RF_SPI_MST_REG_SPI_CRTL2_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_PHASE_TH_MASK            0x1F000000                // PHASE_TH[28..24]
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_PHASE_TH_SHFT            24
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_MST_PHASE_CK0_ADDR       CONN_RF_SPI_MST_REG_SPI_CRTL2_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_MST_PHASE_CK0_MASK       0x00FF0000                // MST_PHASE_CK0[23..16]
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_MST_PHASE_CK0_SHFT       16
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_MST_PHASE_CK1_ADDR       CONN_RF_SPI_MST_REG_SPI_CRTL2_ADDR
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_MST_PHASE_CK1_MASK       0x0000FF00                // MST_PHASE_CK1[15..8]
#define CONN_RF_SPI_MST_REG_SPI_CRTL2_MST_PHASE_CK1_SHFT       8

/* =====================================================================================

  ---FM_CTRL (0x18042000 + 0x00C)---

    FM_RD_EXT_CNT[7..0]          - (RW) FM read extension count.
    RESERVED8[14..8]             - (RO) Reserved bits
    FM_RD_EXT_EN[15]             - (RW) FM Read data extension enable.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_EN_ADDR          CONN_RF_SPI_MST_REG_FM_CTRL_ADDR
#define CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_EN_MASK          0x00008000                // FM_RD_EXT_EN[15]
#define CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_EN_SHFT          15
#define CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_CNT_ADDR         CONN_RF_SPI_MST_REG_FM_CTRL_ADDR
#define CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_CNT_MASK         0x000000FF                // FM_RD_EXT_CNT[7..0]
#define CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_CNT_SHFT         0

/* =====================================================================================

  ---SPI_WF_ADDR (0x18042000 + 0x010)---

    WF_ADDR[15..0]               - (RW) SPI WF address. The 16-bit instruction define below:
                                     {SUB_SYS, RW, CR_ADDR}
                                     
                                     SUB_SYS: 3 bits
                                        "001" : For WF RF CR
                                        "010" : For BT RF CR
                                        "011" : For FM RF CR
                                        "100" : For GPS RF CR 
                                        "101" : For TOP_MISC CR 
                                     RW : 1 bit
                                        "1" means read
                                        "0" means write
                                     CR_ADDR : 12 bits
                                        SUB_SYS RF CR address.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_WF_ADDR_WF_ADDR_ADDR           CONN_RF_SPI_MST_REG_SPI_WF_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_WF_ADDR_WF_ADDR_MASK           0x0000FFFF                // WF_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_WF_ADDR_WF_ADDR_SHFT           0

/* =====================================================================================

  ---SPI_WF_WDAT (0x18042000 + 0x014)---

    WF_WDAT[31..0]               - (RW) WF write data. When MCU/APB write this register, it will set WF_BUSY to "1" and start to arbitrate to transfer instruction.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_WF_WDAT_WF_WDAT_ADDR           CONN_RF_SPI_MST_REG_SPI_WF_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_WF_WDAT_WF_WDAT_MASK           0xFFFFFFFF                // WF_WDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_WF_WDAT_WF_WDAT_SHFT           0

/* =====================================================================================

  ---SPI_WF_RDAT (0x18042000 + 0x018)---

    WF_RDAT[31..0]               - (RO) WF read data. When set a read instruction, the WF_BUSY will set to high. Do not read the WF_RDAT regieter until WF_BUSY is low.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_WF_RDAT_WF_RDAT_ADDR           CONN_RF_SPI_MST_REG_SPI_WF_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_WF_RDAT_WF_RDAT_MASK           0xFFFFFFFF                // WF_RDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_WF_RDAT_WF_RDAT_SHFT           0

/* =====================================================================================

  ---SPI_BT_ADDR (0x18042000 + 0x020)---

    BT_ADDR[15..0]               - (RW) RFSPI BT Address.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_BT_ADDR_BT_ADDR_ADDR           CONN_RF_SPI_MST_REG_SPI_BT_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_BT_ADDR_BT_ADDR_MASK           0x0000FFFF                // BT_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_BT_ADDR_BT_ADDR_SHFT           0

/* =====================================================================================

  ---SPI_BT_WDAT (0x18042000 + 0x024)---

    BT_WDAT[31..0]               - (RW) BT write data. When MCU/APB write this register, it will set BT_BUSY to "1" and start to arbitrate to transfer instruction.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_BT_WDAT_BT_WDAT_ADDR           CONN_RF_SPI_MST_REG_SPI_BT_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_BT_WDAT_BT_WDAT_MASK           0xFFFFFFFF                // BT_WDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_BT_WDAT_BT_WDAT_SHFT           0

/* =====================================================================================

  ---SPI_BT_RDAT (0x18042000 + 0x028)---

    BT_RDAT[31..0]               - (RO) BT read data. When set a read instruction, the BT_BUSY will set to high. Do not read the BT_RDAT regieter until BT_BUSY is low.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_BT_RDAT_BT_RDAT_ADDR           CONN_RF_SPI_MST_REG_SPI_BT_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_BT_RDAT_BT_RDAT_MASK           0xFFFFFFFF                // BT_RDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_BT_RDAT_BT_RDAT_SHFT           0

/* =====================================================================================

  ---SPI_FM_ADDR (0x18042000 + 0x030)---

    FM_ADDR[15..0]               - (RW) RFSPI FM Address.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_FM_ADDR_FM_ADDR_ADDR           CONN_RF_SPI_MST_REG_SPI_FM_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_FM_ADDR_FM_ADDR_MASK           0x0000FFFF                // FM_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_FM_ADDR_FM_ADDR_SHFT           0

/* =====================================================================================

  ---SPI_FM_WDAT (0x18042000 + 0x034)---

    FM_WDAT[15..0]               - (RW) FM write data. When MCU/APB write this register, it will set FM_BUSY to "1" and start to arbitrate to transfer instruction.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_FM_WDAT_FM_WDAT_ADDR           CONN_RF_SPI_MST_REG_SPI_FM_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_FM_WDAT_FM_WDAT_MASK           0x0000FFFF                // FM_WDAT[15..0]
#define CONN_RF_SPI_MST_REG_SPI_FM_WDAT_FM_WDAT_SHFT           0

/* =====================================================================================

  ---SPI_FM_RDAT (0x18042000 + 0x038)---

    FM_RDAT[15..0]               - (RO) FM read data. When set a read instruction, the FM_BUSY will set to high. Do not read the FM_RDAT regieter until FM_BUSY is low.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_FM_RDAT_FM_RDAT_ADDR           CONN_RF_SPI_MST_REG_SPI_FM_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_FM_RDAT_FM_RDAT_MASK           0x0000FFFF                // FM_RDAT[15..0]
#define CONN_RF_SPI_MST_REG_SPI_FM_RDAT_FM_RDAT_SHFT           0

/* =====================================================================================

  ---SPI_GPS_ADDR (0x18042000 + 0x040)---

    GPS_ADDR[15..0]              - (RO) GPS Address. This is read-only register. It porgrammed by GPS/DSP.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_ADDR_GPS_ADDR_ADDR         CONN_RF_SPI_MST_REG_SPI_GPS_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_ADDR_GPS_ADDR_MASK         0x0000FFFF                // GPS_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_ADDR_GPS_ADDR_SHFT         0

/* =====================================================================================

  ---SPI_GPS_WDAT (0x18042000 + 0x044)---

    GPS_WDAT[15..0]              - (RO) GPS write data. This is read-only register. It porgrammed by GPS/DSP.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_WDAT_GPS_WDAT_ADDR         CONN_RF_SPI_MST_REG_SPI_GPS_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_WDAT_GPS_WDAT_MASK         0x0000FFFF                // GPS_WDAT[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_WDAT_GPS_WDAT_SHFT         0

/* =====================================================================================

  ---SPI_GPS_RDAT (0x18042000 + 0x048)---

    GPS_RDAT[15..0]              - (RO) GPS read data.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_RDAT_GPS_RDAT_ADDR         CONN_RF_SPI_MST_REG_SPI_GPS_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_RDAT_GPS_RDAT_MASK         0x0000FFFF                // GPS_RDAT[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_RDAT_GPS_RDAT_SHFT         0

/* =====================================================================================

  ---SPI_TOP_ADDR (0x18042000 + 0x050)---

    TOP_ADDR[15..0]              - (RW) RFSPI TOP_MISC Address.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_TOP_ADDR_ADDR         CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_TOP_ADDR_MASK         0x0000FFFF                // TOP_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_TOP_ADDR_SHFT         0

/* =====================================================================================

  ---SPI_TOP_WDAT (0x18042000 + 0x054)---

    TOP_WDAT[31..0]              - (RW) TOP_MISC write data. When MCU/APB write this register, it will set TOP_BUSY to "1" and start to arbitrate to transfer instruction.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_TOP_WDAT_ADDR         CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_TOP_WDAT_MASK         0xFFFFFFFF                // TOP_WDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_TOP_WDAT_SHFT         0

/* =====================================================================================

  ---SPI_TOP_RDAT (0x18042000 + 0x058)---

    TOP_RDAT[31..0]              - (RO) TOP_MISC read data. When set a read instruction, the TOP_BUSY will set to high. Do not read the TOP_RDAT regieter until BT_BUSY is low.

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_TOP_RDAT_ADDR         CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_TOP_RDAT_MASK         0xFFFFFFFF                // TOP_RDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_TOP_RDAT_SHFT         0

/* =====================================================================================

  ---SPI_SLP_ADDR (0x18042000 + 0x060)---

    SLP_ADDR[15..0]              - (RO) Sleep Control Address
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_SLP_ADDR_SLP_ADDR_ADDR         CONN_RF_SPI_MST_REG_SPI_SLP_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_SLP_ADDR_SLP_ADDR_MASK         0x0000FFFF                // SLP_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_SLP_ADDR_SLP_ADDR_SHFT         0

/* =====================================================================================

  ---SLP_WDAT (0x18042000 + 0x064)---

    SLP_WDAT[31..0]              - (RO) Sleep Control write data

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SLP_WDAT_SLP_WDAT_ADDR             CONN_RF_SPI_MST_REG_SLP_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SLP_WDAT_SLP_WDAT_MASK             0xFFFFFFFF                // SLP_WDAT[31..0]
#define CONN_RF_SPI_MST_REG_SLP_WDAT_SLP_WDAT_SHFT             0

/* =====================================================================================

  ---SLP_RDAT (0x18042000 + 0x068)---

    SLP_RDAT[31..0]              - (RO) Sleep Control read data

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SLP_RDAT_SLP_RDAT_ADDR             CONN_RF_SPI_MST_REG_SLP_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SLP_RDAT_SLP_RDAT_MASK             0xFFFFFFFF                // SLP_RDAT[31..0]
#define CONN_RF_SPI_MST_REG_SLP_RDAT_SLP_RDAT_SHFT             0

/* =====================================================================================

  ---THM_ADDR (0x18042000 + 0x070)---

    THM_ADDR[15..0]              - (RO) Thermal Control Address
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_THM_ADDR_THM_ADDR_ADDR             CONN_RF_SPI_MST_REG_THM_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_THM_ADDR_THM_ADDR_MASK             0x0000FFFF                // THM_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_THM_ADDR_THM_ADDR_SHFT             0

/* =====================================================================================

  ---THM_WDAT (0x18042000 + 0x074)---

    THM_WDAT[31..0]              - (RO) Thermal Control write data

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_THM_WDAT_THM_WDAT_ADDR             CONN_RF_SPI_MST_REG_THM_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_THM_WDAT_THM_WDAT_MASK             0xFFFFFFFF                // THM_WDAT[31..0]
#define CONN_RF_SPI_MST_REG_THM_WDAT_THM_WDAT_SHFT             0

/* =====================================================================================

  ---THM_RDAT (0x18042000 + 0x078)---

    THM_RDAT[31..0]              - (RO) Thermal Control read data

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_THM_RDAT_THM_RDAT_ADDR             CONN_RF_SPI_MST_REG_THM_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_THM_RDAT_THM_RDAT_MASK             0xFFFFFFFF                // THM_RDAT[31..0]
#define CONN_RF_SPI_MST_REG_THM_RDAT_THM_RDAT_SHFT             0

/* =====================================================================================

  ---SPI_INT (0x18042000 + 0x080)---

    ARB_SLOT_INT[0]              - (W1C) status of arb slot abnormal interrupt. Write "1" can clear the status.
    ADDR_NACK_INT[1]             - (W1C) status of addr nack interrupt. Write "1" can clear the status.
    DATA_NACK_INT[2]             - (W1C) status of data nack interrupt. Write "1" can clear the status.
    AUTO_K_LOCK_INT[3]           - (W1C) status of auto k lock interrupt. Write "1" can clear the status.
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_INT_AUTO_K_LOCK_INT_ADDR       CONN_RF_SPI_MST_REG_SPI_INT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_INT_AUTO_K_LOCK_INT_MASK       0x00000008                // AUTO_K_LOCK_INT[3]
#define CONN_RF_SPI_MST_REG_SPI_INT_AUTO_K_LOCK_INT_SHFT       3
#define CONN_RF_SPI_MST_REG_SPI_INT_DATA_NACK_INT_ADDR         CONN_RF_SPI_MST_REG_SPI_INT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_INT_DATA_NACK_INT_MASK         0x00000004                // DATA_NACK_INT[2]
#define CONN_RF_SPI_MST_REG_SPI_INT_DATA_NACK_INT_SHFT         2
#define CONN_RF_SPI_MST_REG_SPI_INT_ADDR_NACK_INT_ADDR         CONN_RF_SPI_MST_REG_SPI_INT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_INT_ADDR_NACK_INT_MASK         0x00000002                // ADDR_NACK_INT[1]
#define CONN_RF_SPI_MST_REG_SPI_INT_ADDR_NACK_INT_SHFT         1
#define CONN_RF_SPI_MST_REG_SPI_INT_ARB_SLOT_INT_ADDR          CONN_RF_SPI_MST_REG_SPI_INT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_INT_ARB_SLOT_INT_MASK          0x00000001                // ARB_SLOT_INT[0]
#define CONN_RF_SPI_MST_REG_SPI_INT_ARB_SLOT_INT_SHFT          0

/* =====================================================================================

  ---SPI_HW_DMY_CMD (0x18042000 + 0x084)---

    HW_DMY_CMD_TRIG[0]           - (W1C) trig hw dummy command
    HW_DMY_CMD_BSY[1]            - (RO) When MCU/APB set HW_DMY_CMD_TRIG, this bit will be high. It will clear to low until dummy command finished.
    RESERVED2[7..2]              - (RO) Reserved bits
    HW_DMY_CMD_CYCLE[15..8]      - (RW) dummy cycle for hw dummy cmd
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_CYCLE_ADDR CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_CYCLE_MASK 0x0000FF00                // HW_DMY_CMD_CYCLE[15..8]
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_CYCLE_SHFT 8
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_BSY_ADDR CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_BSY_MASK 0x00000002                // HW_DMY_CMD_BSY[1]
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_BSY_SHFT 1
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_TRIG_ADDR CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_TRIG_MASK 0x00000001                // HW_DMY_CMD_TRIG[0]
#define CONN_RF_SPI_MST_REG_SPI_HW_DMY_CMD_HW_DMY_CMD_TRIG_SHFT 0

/* =====================================================================================

  ---SPI_DBG (0x18042000 + 0x090)---

    DBG_SEL[2..0]                - (RW) SPI debug flag group select.
    RESERVED3[6..3]              - (RO) Reserved bits
    DBG_EN[7]                    - (RW) SPI debug flag enable
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_DBG_DBG_EN_ADDR                CONN_RF_SPI_MST_REG_SPI_DBG_ADDR
#define CONN_RF_SPI_MST_REG_SPI_DBG_DBG_EN_MASK                0x00000080                // DBG_EN[7]
#define CONN_RF_SPI_MST_REG_SPI_DBG_DBG_EN_SHFT                7
#define CONN_RF_SPI_MST_REG_SPI_DBG_DBG_SEL_ADDR               CONN_RF_SPI_MST_REG_SPI_DBG_ADDR
#define CONN_RF_SPI_MST_REG_SPI_DBG_DBG_SEL_MASK               0x00000007                // DBG_SEL[2..0]
#define CONN_RF_SPI_MST_REG_SPI_DBG_DBG_SEL_SHFT               0

/* =====================================================================================

  ---SPI_STA1 (0x18042000 + 0x094)---

    ARB_GNT[14..0]               - (RO) Arbiter Grnat :
                                     Bit 14 : Sleep Control Grant
                                     Bit 12 : WiFi Grant
                                     Bit 10 : BT Grant
                                     Bit 8 : FM Grant
                                     Bit 6 : GPS Grant
                                     Bit 4 : TOP Grant
                                     Bit 2 : Thermal Control Grant
                                     Bit 0 : GPS RF_DIG Grant
                                     Bit 1,3,5,7,9,11,13: FM AUDIO Grant
    RESERVED15[15]               - (RO) Reserved bits
    ARB_SLOT[29..16]             - (RO) Arbiter slot values.
    RESERVED30[31..30]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_STA1_ARB_SLOT_ADDR             CONN_RF_SPI_MST_REG_SPI_STA1_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA1_ARB_SLOT_MASK             0x3FFF0000                // ARB_SLOT[29..16]
#define CONN_RF_SPI_MST_REG_SPI_STA1_ARB_SLOT_SHFT             16
#define CONN_RF_SPI_MST_REG_SPI_STA1_ARB_GNT_ADDR              CONN_RF_SPI_MST_REG_SPI_STA1_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA1_ARB_GNT_MASK              0x00007FFF                // ARB_GNT[14..0]
#define CONN_RF_SPI_MST_REG_SPI_STA1_ARB_GNT_SHFT              0

/* =====================================================================================

  ---SPI_STA2 (0x18042000 + 0x098)---

    SPI_STA2[1..0]               - (RO) SPI sleep control FSM
                                     "00" : SLP_IDLE  
                                     "01" : SLP_WAIT
                                     "10" : SLP_LOAD
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_STA2_SPI_STA2_ADDR             CONN_RF_SPI_MST_REG_SPI_STA2_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA2_SPI_STA2_MASK             0x00000003                // SPI_STA2[1..0]
#define CONN_RF_SPI_MST_REG_SPI_STA2_SPI_STA2_SHFT             0

/* =====================================================================================

  ---SPI_STA3 (0x18042000 + 0x09C)---

    SPI_STATE[2..0]              - (RO) SPI protocol FSM : 
                                     "000" : SPI_IDLE
                                     "001" : SPI_ADR
                                     "010" : SPI_DMY
                                     "011" : SPI_WDATA
                                     "100" : SPI_RDATA
                                     "101" : SPI_DLY1
                                     "110" : SPI_DLY2
                                     "111" : SPI_DLY3
    RESERVED3[3]                 - (RO) Reserved bits
    ARB_REQ[12..4]               - (RO) ARBiter request.
                                     Bit 8 : Sleep Control Request
                                     Bit 7 : WiFi Request
                                     Bit 6 : BT Request
                                     Bit 5 : FM Request
                                     Bit 4 : GPS Request
                                     Bit 3 : TOP Request
                                     Bit 2 : Thermal Control Request
                                     Bit 1: FM AUDIO request
                                     Bit 0 : GPS RF_DIG Request
    GNT_T2[15..13]               - (RO) Grant trigger enevt.
                                     Bit[14:12]
    GNT_T0[23..16]               - (RO) Grant trigger enevt.
                                     Bit[7:0]
    RLD_TRG[24]                  - (RO) Re-load trigger.
    ALL_REQ_EN[25]               - (RO) All request signal.
    ALL_ACK[26]                  - (RO) All acknowledge signal.
    ARB_ZREO[27]                 - (RO) Arbiter slot zero occurred.
    GNT_T1[31..28]               - (RO) Grant trigger enevt.
                                     Bit[11:8]

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T1_ADDR               CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T1_MASK               0xF0000000                // GNT_T1[31..28]
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T1_SHFT               28
#define CONN_RF_SPI_MST_REG_SPI_STA3_ARB_ZREO_ADDR             CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_ARB_ZREO_MASK             0x08000000                // ARB_ZREO[27]
#define CONN_RF_SPI_MST_REG_SPI_STA3_ARB_ZREO_SHFT             27
#define CONN_RF_SPI_MST_REG_SPI_STA3_ALL_ACK_ADDR              CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_ALL_ACK_MASK              0x04000000                // ALL_ACK[26]
#define CONN_RF_SPI_MST_REG_SPI_STA3_ALL_ACK_SHFT              26
#define CONN_RF_SPI_MST_REG_SPI_STA3_ALL_REQ_EN_ADDR           CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_ALL_REQ_EN_MASK           0x02000000                // ALL_REQ_EN[25]
#define CONN_RF_SPI_MST_REG_SPI_STA3_ALL_REQ_EN_SHFT           25
#define CONN_RF_SPI_MST_REG_SPI_STA3_RLD_TRG_ADDR              CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_RLD_TRG_MASK              0x01000000                // RLD_TRG[24]
#define CONN_RF_SPI_MST_REG_SPI_STA3_RLD_TRG_SHFT              24
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T0_ADDR               CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T0_MASK               0x00FF0000                // GNT_T0[23..16]
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T0_SHFT               16
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T2_ADDR               CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T2_MASK               0x0000E000                // GNT_T2[15..13]
#define CONN_RF_SPI_MST_REG_SPI_STA3_GNT_T2_SHFT               13
#define CONN_RF_SPI_MST_REG_SPI_STA3_ARB_REQ_ADDR              CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_ARB_REQ_MASK              0x00001FF0                // ARB_REQ[12..4]
#define CONN_RF_SPI_MST_REG_SPI_STA3_ARB_REQ_SHFT              4
#define CONN_RF_SPI_MST_REG_SPI_STA3_SPI_STATE_ADDR            CONN_RF_SPI_MST_REG_SPI_STA3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA3_SPI_STATE_MASK            0x00000007                // SPI_STATE[2..0]
#define CONN_RF_SPI_MST_REG_SPI_STA3_SPI_STATE_SHFT            0

/* =====================================================================================

  ---SPI_STA4 (0x18042000 + 0x100)---

    MST_SHIFT[3..0]              - (RO) state of mst_shift
    RESERVED4[5..4]              - (RO) Reserved bits
    AUTOK_LOCK[6]                - (RO) auto calibration done.
                                     1: autok done.
    AUTO_K_FAIL[7]               - (RO) state of auto_k_fail
    AUTOK_PHASE_SEL[11..8]       - (RO) state of auto_k_phase_sel
    RESERVED12[13..12]           - (RO) Reserved bits
    AUTO_K_MAX_SHIFT[14]         - (RO) state of auto_k_max_shift
    AUTO_K_PASS[15]              - (RO) state of auto_k_pass
    MST_PHASE_SEL[19..16]        - (RO) SPI phase selection
    RESERVED20[23..20]           - (RO) Reserved bits
    AUTOK_STATE[27..24]          - (RO) AUTOK FSM:
                                     0000: AUTOK_IDLE
                                     0001: AUTOK_CAP0_TRIG
                                     0010: AUTOK_CAP0_CMD
                                     0011: AUTOK_CAP0_DONE
                                     0100: AUTOK_CAP1_TRIG
                                     0101: AUTOK_CAP1_CMD
                                     0110: AUTOK_CAP1_DONE
                                     0111: AUTOK_LOCK
    CLK64M_EN[28]                - (RO) spi_uclk_ck setting in auto_k module
                                     0: 26MHz
                                     1: 64MHz
    RSPI_cap_next[29]            - (RO) capture edge setting in RFSPI design
                                     0: original edge
                                     1: next edge
    RSPI_HS_EN_HW[30]            - (RO) spi_uclk_ck setting in RFSPI design
                                     0: 26MHz
                                     1: High speed clock
    RESERVED31[31]               - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_STA4_RSPI_HS_EN_HW_ADDR        CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_RSPI_HS_EN_HW_MASK        0x40000000                // RSPI_HS_EN_HW[30]
#define CONN_RF_SPI_MST_REG_SPI_STA4_RSPI_HS_EN_HW_SHFT        30
#define CONN_RF_SPI_MST_REG_SPI_STA4_RSPI_cap_next_ADDR        CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_RSPI_cap_next_MASK        0x20000000                // RSPI_cap_next[29]
#define CONN_RF_SPI_MST_REG_SPI_STA4_RSPI_cap_next_SHFT        29
#define CONN_RF_SPI_MST_REG_SPI_STA4_CLK64M_EN_ADDR            CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_CLK64M_EN_MASK            0x10000000                // CLK64M_EN[28]
#define CONN_RF_SPI_MST_REG_SPI_STA4_CLK64M_EN_SHFT            28
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_STATE_ADDR          CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_STATE_MASK          0x0F000000                // AUTOK_STATE[27..24]
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_STATE_SHFT          24
#define CONN_RF_SPI_MST_REG_SPI_STA4_MST_PHASE_SEL_ADDR        CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_MST_PHASE_SEL_MASK        0x000F0000                // MST_PHASE_SEL[19..16]
#define CONN_RF_SPI_MST_REG_SPI_STA4_MST_PHASE_SEL_SHFT        16
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_PASS_ADDR          CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_PASS_MASK          0x00008000                // AUTO_K_PASS[15]
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_PASS_SHFT          15
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_MAX_SHIFT_ADDR     CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_MAX_SHIFT_MASK     0x00004000                // AUTO_K_MAX_SHIFT[14]
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_MAX_SHIFT_SHFT     14
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_PHASE_SEL_ADDR      CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_PHASE_SEL_MASK      0x00000F00                // AUTOK_PHASE_SEL[11..8]
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_PHASE_SEL_SHFT      8
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_FAIL_ADDR          CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_FAIL_MASK          0x00000080                // AUTO_K_FAIL[7]
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTO_K_FAIL_SHFT          7
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_LOCK_ADDR           CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_LOCK_MASK           0x00000040                // AUTOK_LOCK[6]
#define CONN_RF_SPI_MST_REG_SPI_STA4_AUTOK_LOCK_SHFT           6
#define CONN_RF_SPI_MST_REG_SPI_STA4_MST_SHIFT_ADDR            CONN_RF_SPI_MST_REG_SPI_STA4_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA4_MST_SHIFT_MASK            0x0000000F                // MST_SHIFT[3..0]
#define CONN_RF_SPI_MST_REG_SPI_STA4_MST_SHIFT_SHFT            0

/* =====================================================================================

  ---SPI_STA5 (0x18042000 + 0x104)---

    RESERVED0[3..0]              - (RO) Reserved bits
    K_FAIL_PHASE_HW[7..4]        - (RO) auto k fail phase
    RESERVED8[9..8]              - (RO) Reserved bits
    K_PASS_PHASE_HW[13..10]      - (RO) auto k pass phase
    RESERVED14[31..14]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_STA5_K_PASS_PHASE_HW_ADDR      CONN_RF_SPI_MST_REG_SPI_STA5_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA5_K_PASS_PHASE_HW_MASK      0x00003C00                // K_PASS_PHASE_HW[13..10]
#define CONN_RF_SPI_MST_REG_SPI_STA5_K_PASS_PHASE_HW_SHFT      10
#define CONN_RF_SPI_MST_REG_SPI_STA5_K_FAIL_PHASE_HW_ADDR      CONN_RF_SPI_MST_REG_SPI_STA5_ADDR
#define CONN_RF_SPI_MST_REG_SPI_STA5_K_FAIL_PHASE_HW_MASK      0x000000F0                // K_FAIL_PHASE_HW[7..4]
#define CONN_RF_SPI_MST_REG_SPI_STA5_K_FAIL_PHASE_HW_SHFT      4

/* =====================================================================================

  ---SPI_HSCK_CTL (0x18042000 + 0x108)---

    TOP_HS_EN[0]                 - (RO) TOP enable high speed clock
    WF_HS_EN[1]                  - (RO) WF enable high speed clock
    BT_HS_EN[2]                  - (RO) BT enable high speed clock
    GPS_HS_EN[3]                 - (RO) GPS enable high speed clock
    FM_HS_EN[4]                  - (RO) FM enable high speed clock
    FM_SPI_CK_CTL[5]             - (RO) FM CTL has the first pirorty to control clk
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL_ADDR    CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL_MASK    0x00000020                // FM_SPI_CK_CTL[5]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL_SHFT    5
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_HS_EN_ADDR         CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_HS_EN_MASK         0x00000010                // FM_HS_EN[4]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_HS_EN_SHFT         4
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_GPS_HS_EN_ADDR        CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_GPS_HS_EN_MASK        0x00000008                // GPS_HS_EN[3]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_GPS_HS_EN_SHFT        3
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_BT_HS_EN_ADDR         CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_BT_HS_EN_MASK         0x00000004                // BT_HS_EN[2]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_BT_HS_EN_SHFT         2
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_WF_HS_EN_ADDR         CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_WF_HS_EN_MASK         0x00000002                // WF_HS_EN[1]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_WF_HS_EN_SHFT         1
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_TOP_HS_EN_ADDR        CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_TOP_HS_EN_MASK        0x00000001                // TOP_HS_EN[0]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_TOP_HS_EN_SHFT        0

/* =====================================================================================

  ---SPI_HSCK_CTL_SET (0x18042000 + 0x10C)---

    TOP_HS_EN[0]                 - (WO) TOP enable high speed clock
    WF_HS_EN[1]                  - (WO) WF enable high speed clock
    BT_HS_EN[2]                  - (WO) BT enable high speed clock
    GPS_HS_EN[3]                 - (WO) GPS enable high speed clock
    FM_HS_EN[4]                  - (WO) FM enable high speed clock
    FM_SPI_CK_CTL[5]             - (WO) FM CTL has the first pirorty to control clk
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_SPI_CK_CTL_ADDR CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_SPI_CK_CTL_MASK 0x00000020                // FM_SPI_CK_CTL[5]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_SPI_CK_CTL_SHFT 5
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_HS_EN_ADDR     CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_HS_EN_MASK     0x00000010                // FM_HS_EN[4]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_HS_EN_SHFT     4
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_GPS_HS_EN_ADDR    CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_GPS_HS_EN_MASK    0x00000008                // GPS_HS_EN[3]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_GPS_HS_EN_SHFT    3
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_BT_HS_EN_ADDR     CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_BT_HS_EN_MASK     0x00000004                // BT_HS_EN[2]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_BT_HS_EN_SHFT     2
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_WF_HS_EN_ADDR     CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_WF_HS_EN_MASK     0x00000002                // WF_HS_EN[1]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_WF_HS_EN_SHFT     1
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_TOP_HS_EN_ADDR    CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_TOP_HS_EN_MASK    0x00000001                // TOP_HS_EN[0]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_TOP_HS_EN_SHFT    0

/* =====================================================================================

  ---SPI_HSCK_CTL_CLR (0x18042000 + 0x110)---

    TOP_HS_EN[0]                 - (WO) TOP enable high speed clock
    WF_HS_EN[1]                  - (WO) WF enable high speed clock
    BT_HS_EN[2]                  - (WO) BT enable high speed clock
    GPS_HS_EN[3]                 - (WO) GPS enable high speed clock
    FM_HS_EN[4]                  - (WO) FM enable high speed clock
    FM_SPI_CK_CTL[5]             - (WO) FM CTL has the first pirorty to control clk
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_SPI_CK_CTL_ADDR CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_SPI_CK_CTL_MASK 0x00000020                // FM_SPI_CK_CTL[5]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_SPI_CK_CTL_SHFT 5
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_HS_EN_ADDR     CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_HS_EN_MASK     0x00000010                // FM_HS_EN[4]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_HS_EN_SHFT     4
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_GPS_HS_EN_ADDR    CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_GPS_HS_EN_MASK    0x00000008                // GPS_HS_EN[3]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_GPS_HS_EN_SHFT    3
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_BT_HS_EN_ADDR     CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_BT_HS_EN_MASK     0x00000004                // BT_HS_EN[2]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_BT_HS_EN_SHFT     2
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_WF_HS_EN_ADDR     CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_WF_HS_EN_MASK     0x00000002                // WF_HS_EN[1]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_WF_HS_EN_SHFT     1
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_TOP_HS_EN_ADDR    CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_TOP_HS_EN_MASK    0x00000001                // TOP_HS_EN[0]
#define CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_TOP_HS_EN_SHFT    0

/* =====================================================================================

  ---SPI_GPS_GPS_ADDR (0x18042000 + 0x210)---

    G_GPS_ADDR[15..0]            - (RW) GPS Address.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_G_GPS_ADDR_ADDR   CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_G_GPS_ADDR_MASK   0x0000FFFF                // G_GPS_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_G_GPS_ADDR_SHFT   0

/* =====================================================================================

  ---SPI_GPS_GPS_WDAT (0x18042000 + 0x214)---

    G_GPS_WDAT[15..0]            - (RW) GPS write data.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_G_GPS_WDAT_ADDR   CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_G_GPS_WDAT_MASK   0x0000FFFF                // G_GPS_WDAT[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_G_GPS_WDAT_SHFT   0

/* =====================================================================================

  ---SPI_GPS_GPS_RDAT (0x18042000 + 0x218)---

    G_GPS_RDAT[15..0]            - (RO) GPS read data.
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_G_GPS_RDAT_ADDR   CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_G_GPS_RDAT_MASK   0x0000FFFF                // G_GPS_RDAT[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_G_GPS_RDAT_SHFT   0

/* =====================================================================================

  ---SPI_GPS_RFDIG_ADDR (0x18042000 + 0x220)---

    G_RFDIG_ADDR[15..0]          - (RW) TOP Address. -> GPS access ATOP CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_ADDR_G_RFDIG_ADDR_ADDR CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_ADDR_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_ADDR_G_RFDIG_ADDR_MASK 0x0000FFFF                // G_RFDIG_ADDR[15..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_ADDR_G_RFDIG_ADDR_SHFT 0

/* =====================================================================================

  ---SPI_GPS_RFDIG_WDAT (0x18042000 + 0x224)---

    G_RFDIG_WDAT[31..0]          - (RW) TOP write data. -> GPS access ATOP CR

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_WDAT_G_RFDIG_WDAT_ADDR CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_WDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_WDAT_G_RFDIG_WDAT_MASK 0xFFFFFFFF                // G_RFDIG_WDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_WDAT_G_RFDIG_WDAT_SHFT 0

/* =====================================================================================

  ---SPI_GPS_RFDIG_RDAT (0x18042000 + 0x228)---

    G_RFDIG_RDAT[31..0]          - (RO) TOP read data.  -> GPS access ATOP CR

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_RDAT_G_RFDIG_RDAT_ADDR CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_RDAT_ADDR
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_RDAT_G_RFDIG_RDAT_MASK 0xFFFFFFFF                // G_RFDIG_RDAT[31..0]
#define CONN_RF_SPI_MST_REG_SPI_GPS_RFDIG_RDAT_G_RFDIG_RDAT_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_ADDR_0 (0x18042000 + 0x300)---

    RECORD_ADDR_0[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_0_RECORD_ADDR_0_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_0_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_0_RECORD_ADDR_0_MASK 0xFFFFFFFF                // RECORD_ADDR_0[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_0_RECORD_ADDR_0_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_ADDR_1 (0x18042000 + 0x304)---

    RECORD_ADDR_1[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_1_RECORD_ADDR_1_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_1_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_1_RECORD_ADDR_1_MASK 0xFFFFFFFF                // RECORD_ADDR_1[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_1_RECORD_ADDR_1_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_ADDR_2 (0x18042000 + 0x308)---

    RECORD_ADDR_2[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_2_RECORD_ADDR_2_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_2_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_2_RECORD_ADDR_2_MASK 0xFFFFFFFF                // RECORD_ADDR_2[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_2_RECORD_ADDR_2_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_ADDR_3 (0x18042000 + 0x30C)---

    RECORD_ADDR_3[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_3_RECORD_ADDR_3_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_3_RECORD_ADDR_3_MASK 0xFFFFFFFF                // RECORD_ADDR_3[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_ADDR_3_RECORD_ADDR_3_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_DATA_0 (0x18042000 + 0x310)---

    RECORD_DATA_0[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_0_RECORD_DATA_0_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_0_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_0_RECORD_DATA_0_MASK 0xFFFFFFFF                // RECORD_DATA_0[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_0_RECORD_DATA_0_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_DATA_1 (0x18042000 + 0x314)---

    RECORD_DATA_1[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_1_RECORD_DATA_1_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_1_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_1_RECORD_DATA_1_MASK 0xFFFFFFFF                // RECORD_DATA_1[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_1_RECORD_DATA_1_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_DATA_2 (0x18042000 + 0x318)---

    RECORD_DATA_2[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_2_RECORD_DATA_2_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_2_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_2_RECORD_DATA_2_MASK 0xFFFFFFFF                // RECORD_DATA_2[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_2_RECORD_DATA_2_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_DATA_3 (0x18042000 + 0x31C)---

    RECORD_DATA_3[31..0]         - (RO)  xxx 

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_3_RECORD_DATA_3_ADDR CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_3_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_3_RECORD_DATA_3_MASK 0xFFFFFFFF                // RECORD_DATA_3[31..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_DATA_3_RECORD_DATA_3_SHFT 0

/* =====================================================================================

  ---SPI_MCU_RECORD_CTL (0x18042000 + 0x320)---

    SUB_SEL[8..0]                - (RW)  xxx 
    RESERVED9[11..9]             - (RO) Reserved bits
    KIND_SEL[13..12]             - (RW)  xxx 
    RESERVED14[31..14]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_KIND_SEL_ADDR   CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_KIND_SEL_MASK   0x00003000                // KIND_SEL[13..12]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_KIND_SEL_SHFT   12
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_SUB_SEL_ADDR    CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_ADDR
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_SUB_SEL_MASK    0x000001FF                // SUB_SEL[8..0]
#define CONN_RF_SPI_MST_REG_SPI_MCU_RECORD_CTL_SUB_SEL_SHFT    0

#endif // __CONN_RF_SPI_MST_REG_REGS_H__
