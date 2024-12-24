/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_therm_ctl.h
//[Revision time]   : Fri Oct 21 08:49:51 2022

#ifndef __CONN_THERM_CTL_REGS_H__
#define __CONN_THERM_CTL_REGS_H__

//****************************************************************************
//
//                     CONN_THERM_CTL CR Definitions                     
//
//****************************************************************************

#define CONN_THERM_CTL_BASE                                    (CONN_REG_CONN_THERM_CTL_ADDR)

#define CONN_THERM_CTL_THERMCR0_ADDR                           (CONN_THERM_CTL_BASE + 0x000) // 0000
#define CONN_THERM_CTL_THERMCR1_ADDR                           (CONN_THERM_CTL_BASE + 0x004) // 0004
#define CONN_THERM_CTL_THERMCR2_ADDR                           (CONN_THERM_CTL_BASE + 0x008) // 0008
#define CONN_THERM_CTL_THERMISR_ADDR                           (CONN_THERM_CTL_BASE + 0x00C) // 000C
#define CONN_THERM_CTL_THERM_ACL_ADDR                          (CONN_THERM_CTL_BASE + 0x010) // 0010
#define CONN_THERM_CTL_THERM_ATHERM_ADDR                       (CONN_THERM_CTL_BASE + 0x014) // 0014
#define CONN_THERM_CTL_THERM_AADDR_ADDR                        (CONN_THERM_CTL_BASE + 0x018) // 0018
#define CONN_THERM_CTL_THERMEN1_ADDR                           (CONN_THERM_CTL_BASE + 0x01C) // 001C
#define CONN_THERM_CTL_THERMEN2_ADDR                           (CONN_THERM_CTL_BASE + 0x020) // 0020
#define CONN_THERM_CTL_THERMEN3_ADDR                           (CONN_THERM_CTL_BASE + 0x024) // 0024
#define CONN_THERM_CTL_THERMEN4_ADDR                           (CONN_THERM_CTL_BASE + 0x028) // 0028




/* =====================================================================================

  ---THERMCR0 (0x18040000 + 0x000)---

    THERM_RAW_VAL[6..0]          - (RO) This value is the MSB 7-bit of the average of Thermal ADC output data with 6-bits each 8 samples. The average value = SUM(8 samples) / 4. It is valid only when THERM_CAL_EN is asserted and THERM_BUSY is not asserted.
    RESERVED7[7]                 - (RO) Reserved bits
    THERM_CAL_VAL[14..8]         - (RW) The written data is used as the prior value y(n-1). The read value is the  y(n) of AR Model.
    RESERVED15[15]               - (RO) Reserved bits
    THERM_BUSY[16]               - (RO) 1: MT6628 is waiting for Thermal stable or sampling Thermal value or calculating now.
                                     0: MT6628 is idle now.
    RESERVED17[17]               - (RO) Reserved bits
    THERM_TRIGGER[18]            - (RW) Trigger MT6628 to do thermal calculation right now
                                     When THERM_CAL_EN is asserted and write 1 will trigger Thermal sample procedure, and write 0 has no meaning. Read always return 0.
    THERM_CAL_EN[19]             - (RW) This field is used to enable the Automatic Detection. The Thermal periodic of Automatic Detection is set by UPDATE_TIME in THERMCR3 If software driver attempt to program or change the value of THERMCR0, this bit should be set to 0.
    RESERVED20[23..20]           - (RO) Reserved bits
    THERM_MEASURE_CNT[28..24]    - (RW) Guard time to prevent busy signal de-assert as mcu sleep and busy assert simultaneously. count by therm_x1_fr_ck
    RESERVED29[31..29]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMCR0_THERM_MEASURE_CNT_ADDR         CONN_THERM_CTL_THERMCR0_ADDR
#define CONN_THERM_CTL_THERMCR0_THERM_MEASURE_CNT_MASK         0x1F000000                // THERM_MEASURE_CNT[28..24]
#define CONN_THERM_CTL_THERMCR0_THERM_MEASURE_CNT_SHFT         24
#define CONN_THERM_CTL_THERMCR0_THERM_CAL_EN_ADDR              CONN_THERM_CTL_THERMCR0_ADDR
#define CONN_THERM_CTL_THERMCR0_THERM_CAL_EN_MASK              0x00080000                // THERM_CAL_EN[19]
#define CONN_THERM_CTL_THERMCR0_THERM_CAL_EN_SHFT              19
#define CONN_THERM_CTL_THERMCR0_THERM_TRIGGER_ADDR             CONN_THERM_CTL_THERMCR0_ADDR
#define CONN_THERM_CTL_THERMCR0_THERM_TRIGGER_MASK             0x00040000                // THERM_TRIGGER[18]
#define CONN_THERM_CTL_THERMCR0_THERM_TRIGGER_SHFT             18
#define CONN_THERM_CTL_THERMCR0_THERM_BUSY_ADDR                CONN_THERM_CTL_THERMCR0_ADDR
#define CONN_THERM_CTL_THERMCR0_THERM_BUSY_MASK                0x00010000                // THERM_BUSY[16]
#define CONN_THERM_CTL_THERMCR0_THERM_BUSY_SHFT                16
#define CONN_THERM_CTL_THERMCR0_THERM_CAL_VAL_ADDR             CONN_THERM_CTL_THERMCR0_ADDR
#define CONN_THERM_CTL_THERMCR0_THERM_CAL_VAL_MASK             0x00007F00                // THERM_CAL_VAL[14..8]
#define CONN_THERM_CTL_THERMCR0_THERM_CAL_VAL_SHFT             8
#define CONN_THERM_CTL_THERMCR0_THERM_RAW_VAL_ADDR             CONN_THERM_CTL_THERMCR0_ADDR
#define CONN_THERM_CTL_THERMCR0_THERM_RAW_VAL_MASK             0x0000007F                // THERM_RAW_VAL[6..0]
#define CONN_THERM_CTL_THERMCR0_THERM_RAW_VAL_SHFT             0

/* =====================================================================================

  ---THERMCR1 (0x18040000 + 0x004)---

    THERM_MAX[6..0]              - (RW) This value is set to be the upper bound  y(n) of AR model. If  y(n) of transmit power is larger than(>=) THERM_MAX, interrupt will issued
    THERM_EXCEED_EN[7]           - (RW) To enable thermal exceed function
                                     As enable, rf_paon will be gated when THERMCR0.THERM_CAL_VAL >= THERMCR1.THERM_MAX
    THERM_MIN[14..8]             - (RW) This value is set to be the lower bound  y(n) of AR model. If y(n)  of transmit power is smaller than (<=)THERM_MIN, interrupt will issued
    RESERVED15[15]               - (RO) Reserved bits
    THERM_AR_FACTOR[17..16]      - (RW) This field is used to set the parameter for the MT6628 Thermal measure. The formula is  y(n)=(1-a)*y(n-1) + a*x(n).
                                       is the MSB 7-bit value of ALC
                                     2'b11:  a=1
                                     2'b10:  a=1/4
                                     2'b01:  a=1/16
                                     2'b00:  a=1/32
    RESERVED18[23..18]           - (RO) Reserved bits
    THERM_2ND_MAX[30..24]        - (RW) This value is set to be the upper bound  y(n) of AR model. If  y(n) of transmit power is larger than (>=)THERM_2ND MAX, interrupt will issued
    RESERVED31[31]               - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMCR1_THERM_2ND_MAX_ADDR             CONN_THERM_CTL_THERMCR1_ADDR
#define CONN_THERM_CTL_THERMCR1_THERM_2ND_MAX_MASK             0x7F000000                // THERM_2ND_MAX[30..24]
#define CONN_THERM_CTL_THERMCR1_THERM_2ND_MAX_SHFT             24
#define CONN_THERM_CTL_THERMCR1_THERM_AR_FACTOR_ADDR           CONN_THERM_CTL_THERMCR1_ADDR
#define CONN_THERM_CTL_THERMCR1_THERM_AR_FACTOR_MASK           0x00030000                // THERM_AR_FACTOR[17..16]
#define CONN_THERM_CTL_THERMCR1_THERM_AR_FACTOR_SHFT           16
#define CONN_THERM_CTL_THERMCR1_THERM_MIN_ADDR                 CONN_THERM_CTL_THERMCR1_ADDR
#define CONN_THERM_CTL_THERMCR1_THERM_MIN_MASK                 0x00007F00                // THERM_MIN[14..8]
#define CONN_THERM_CTL_THERMCR1_THERM_MIN_SHFT                 8
#define CONN_THERM_CTL_THERMCR1_THERM_EXCEED_EN_ADDR           CONN_THERM_CTL_THERMCR1_ADDR
#define CONN_THERM_CTL_THERMCR1_THERM_EXCEED_EN_MASK           0x00000080                // THERM_EXCEED_EN[7]
#define CONN_THERM_CTL_THERMCR1_THERM_EXCEED_EN_SHFT           7
#define CONN_THERM_CTL_THERMCR1_THERM_MAX_ADDR                 CONN_THERM_CTL_THERMCR1_ADDR
#define CONN_THERM_CTL_THERMCR1_THERM_MAX_MASK                 0x0000007F                // THERM_MAX[6..0]
#define CONN_THERM_CTL_THERMCR1_THERM_MAX_SHFT                 0

/* =====================================================================================

  ---THERMCR2 (0x18040000 + 0x008)---

    UPDATE_TIME[5..0]            - (RW) This value specify how long we update the value of thermal sensor y(n)
    RESERVED6[7..6]              - (RO) Reserved bits
    RG_THAD_RSV[15..8]           - (RW) reserve bit
    THERMAL_MIN_INT_EN[16]       - (RW) enable interruption for Thermal_min_int
    RESERVED17[19..17]           - (RO) Reserved bits
    THERMAL_SCND_MAX_INT_EN[20]  - (RW) enable interruption for Thermal_scnd_max_int
    RESERVED21[23..21]           - (RO) Reserved bits
    THERMAL_MAX_INT_EN[24]       - (RW) enable interruption for Thermal_max_int
    RESERVED25[31..25]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMCR2_THERMAL_MAX_INT_EN_ADDR        CONN_THERM_CTL_THERMCR2_ADDR
#define CONN_THERM_CTL_THERMCR2_THERMAL_MAX_INT_EN_MASK        0x01000000                // THERMAL_MAX_INT_EN[24]
#define CONN_THERM_CTL_THERMCR2_THERMAL_MAX_INT_EN_SHFT        24
#define CONN_THERM_CTL_THERMCR2_THERMAL_SCND_MAX_INT_EN_ADDR   CONN_THERM_CTL_THERMCR2_ADDR
#define CONN_THERM_CTL_THERMCR2_THERMAL_SCND_MAX_INT_EN_MASK   0x00100000                // THERMAL_SCND_MAX_INT_EN[20]
#define CONN_THERM_CTL_THERMCR2_THERMAL_SCND_MAX_INT_EN_SHFT   20
#define CONN_THERM_CTL_THERMCR2_THERMAL_MIN_INT_EN_ADDR        CONN_THERM_CTL_THERMCR2_ADDR
#define CONN_THERM_CTL_THERMCR2_THERMAL_MIN_INT_EN_MASK        0x00010000                // THERMAL_MIN_INT_EN[16]
#define CONN_THERM_CTL_THERMCR2_THERMAL_MIN_INT_EN_SHFT        16
#define CONN_THERM_CTL_THERMCR2_RG_THAD_RSV_ADDR               CONN_THERM_CTL_THERMCR2_ADDR
#define CONN_THERM_CTL_THERMCR2_RG_THAD_RSV_MASK               0x0000FF00                // RG_THAD_RSV[15..8]
#define CONN_THERM_CTL_THERMCR2_RG_THAD_RSV_SHFT               8
#define CONN_THERM_CTL_THERMCR2_UPDATE_TIME_ADDR               CONN_THERM_CTL_THERMCR2_ADDR
#define CONN_THERM_CTL_THERMCR2_UPDATE_TIME_MASK               0x0000003F                // UPDATE_TIME[5..0]
#define CONN_THERM_CTL_THERMCR2_UPDATE_TIME_SHFT               0

/* =====================================================================================

  ---THERMISR (0x18040000 + 0x00C)---

    RESERVED0[15..0]             - (RO) Reserved bits
    THERMAL_MIN_INT[16]          - (W1C) If value of thermal sensor y(n) is smaller THERM_MIN
    RESERVED17[19..17]           - (RO) Reserved bits
    THERMAL_SCND_MAX_INT[20]     - (W1C) If value of thermal sensor y(n) is larger THERM_2ND_MAX
    RESERVED21[23..21]           - (RO) Reserved bits
    THERMAL_MAX_INT[24]          - (W1C) If value of thermal sensor y(n) is larger THERM_MAX
    RESERVED25[31..25]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMISR_THERMAL_MAX_INT_ADDR           CONN_THERM_CTL_THERMISR_ADDR
#define CONN_THERM_CTL_THERMISR_THERMAL_MAX_INT_MASK           0x01000000                // THERMAL_MAX_INT[24]
#define CONN_THERM_CTL_THERMISR_THERMAL_MAX_INT_SHFT           24
#define CONN_THERM_CTL_THERMISR_THERMAL_SCND_MAX_INT_ADDR      CONN_THERM_CTL_THERMISR_ADDR
#define CONN_THERM_CTL_THERMISR_THERMAL_SCND_MAX_INT_MASK      0x00100000                // THERMAL_SCND_MAX_INT[20]
#define CONN_THERM_CTL_THERMISR_THERMAL_SCND_MAX_INT_SHFT      20
#define CONN_THERM_CTL_THERMISR_THERMAL_MIN_INT_ADDR           CONN_THERM_CTL_THERMISR_ADDR
#define CONN_THERM_CTL_THERMISR_THERMAL_MIN_INT_MASK           0x00010000                // THERMAL_MIN_INT[16]
#define CONN_THERM_CTL_THERMISR_THERMAL_MIN_INT_SHFT           16

/* =====================================================================================

  ---THERM_ACL (0x18040000 + 0x010)---

    ADIE_TOPCLK_EN[31..0]        - (RW) A-die top clock enable setting.

 =====================================================================================*/
#define CONN_THERM_CTL_THERM_ACL_ADIE_TOPCLK_EN_ADDR           CONN_THERM_CTL_THERM_ACL_ADDR
#define CONN_THERM_CTL_THERM_ACL_ADIE_TOPCLK_EN_MASK           0xFFFFFFFF                // ADIE_TOPCLK_EN[31..0]
#define CONN_THERM_CTL_THERM_ACL_ADIE_TOPCLK_EN_SHFT           0

/* =====================================================================================

  ---THERM_ATHERM (0x18040000 + 0x014)---

    ADIE_THERM_CTL[31..0]        - (RW) A-die thermal control setting

 =====================================================================================*/
#define CONN_THERM_CTL_THERM_ATHERM_ADIE_THERM_CTL_ADDR        CONN_THERM_CTL_THERM_ATHERM_ADDR
#define CONN_THERM_CTL_THERM_ATHERM_ADIE_THERM_CTL_MASK        0xFFFFFFFF                // ADIE_THERM_CTL[31..0]
#define CONN_THERM_CTL_THERM_ATHERM_ADIE_THERM_CTL_SHFT        0

/* =====================================================================================

  ---THERM_AADDR (0x18040000 + 0x018)---

    ADIE_CLK_ADDR[11..0]         - (RW) Adie clk_en address: a00
    ADIE_CLK_GRP[14..12]         - (RW) Adie clk_en spi group: 5
    RESERVED15[15]               - (RO) Reserved bits
    ADIE_THERM_ADDR[27..16]      - (RW) Adie therm_ctl address: 030
    ADIE_THERM_GRP[30..28]       - (RW) Adie therm_ctl spi group: 5
    RESERVED31[31]               - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERM_AADDR_ADIE_THERM_GRP_ADDR         CONN_THERM_CTL_THERM_AADDR_ADDR
#define CONN_THERM_CTL_THERM_AADDR_ADIE_THERM_GRP_MASK         0x70000000                // ADIE_THERM_GRP[30..28]
#define CONN_THERM_CTL_THERM_AADDR_ADIE_THERM_GRP_SHFT         28
#define CONN_THERM_CTL_THERM_AADDR_ADIE_THERM_ADDR_ADDR        CONN_THERM_CTL_THERM_AADDR_ADDR
#define CONN_THERM_CTL_THERM_AADDR_ADIE_THERM_ADDR_MASK        0x0FFF0000                // ADIE_THERM_ADDR[27..16]
#define CONN_THERM_CTL_THERM_AADDR_ADIE_THERM_ADDR_SHFT        16
#define CONN_THERM_CTL_THERM_AADDR_ADIE_CLK_GRP_ADDR           CONN_THERM_CTL_THERM_AADDR_ADDR
#define CONN_THERM_CTL_THERM_AADDR_ADIE_CLK_GRP_MASK           0x00007000                // ADIE_CLK_GRP[14..12]
#define CONN_THERM_CTL_THERM_AADDR_ADIE_CLK_GRP_SHFT           12
#define CONN_THERM_CTL_THERM_AADDR_ADIE_CLK_ADDR_ADDR          CONN_THERM_CTL_THERM_AADDR_ADDR
#define CONN_THERM_CTL_THERM_AADDR_ADIE_CLK_ADDR_MASK          0x00000FFF                // ADIE_CLK_ADDR[11..0]
#define CONN_THERM_CTL_THERM_AADDR_ADIE_CLK_ADDR_SHFT          0

/* =====================================================================================

  ---THERMEN1 (0x18040000 + 0x01C)---

    THERM_RAW_VAL_1[6..0]        - (RO) raw value for subsys 1
    RESERVED7[7]                 - (RO) Reserved bits
    THERM_CAL_VAL_1[14..8]       - (RO) cal value for subsys 1, RO
    RESERVED15[15]               - (RO) Reserved bits
    THERM_BUSY_1[16]             - (RO) bust bit for subsys 1
    RESERVED17[17]               - (RO) Reserved bits
    THERM_TRIGGER_1[18]          - (RW) trigger for subsys 1
    THERM_CAL_EN_1[19]           - (RW) enable for subsys_1
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMEN1_THERM_CAL_EN_1_ADDR            CONN_THERM_CTL_THERMEN1_ADDR
#define CONN_THERM_CTL_THERMEN1_THERM_CAL_EN_1_MASK            0x00080000                // THERM_CAL_EN_1[19]
#define CONN_THERM_CTL_THERMEN1_THERM_CAL_EN_1_SHFT            19
#define CONN_THERM_CTL_THERMEN1_THERM_TRIGGER_1_ADDR           CONN_THERM_CTL_THERMEN1_ADDR
#define CONN_THERM_CTL_THERMEN1_THERM_TRIGGER_1_MASK           0x00040000                // THERM_TRIGGER_1[18]
#define CONN_THERM_CTL_THERMEN1_THERM_TRIGGER_1_SHFT           18
#define CONN_THERM_CTL_THERMEN1_THERM_BUSY_1_ADDR              CONN_THERM_CTL_THERMEN1_ADDR
#define CONN_THERM_CTL_THERMEN1_THERM_BUSY_1_MASK              0x00010000                // THERM_BUSY_1[16]
#define CONN_THERM_CTL_THERMEN1_THERM_BUSY_1_SHFT              16
#define CONN_THERM_CTL_THERMEN1_THERM_CAL_VAL_1_ADDR           CONN_THERM_CTL_THERMEN1_ADDR
#define CONN_THERM_CTL_THERMEN1_THERM_CAL_VAL_1_MASK           0x00007F00                // THERM_CAL_VAL_1[14..8]
#define CONN_THERM_CTL_THERMEN1_THERM_CAL_VAL_1_SHFT           8
#define CONN_THERM_CTL_THERMEN1_THERM_RAW_VAL_1_ADDR           CONN_THERM_CTL_THERMEN1_ADDR
#define CONN_THERM_CTL_THERMEN1_THERM_RAW_VAL_1_MASK           0x0000007F                // THERM_RAW_VAL_1[6..0]
#define CONN_THERM_CTL_THERMEN1_THERM_RAW_VAL_1_SHFT           0

/* =====================================================================================

  ---THERMEN2 (0x18040000 + 0x020)---

    THERM_RAW_VAL_2[6..0]        - (RO) raw value for subsys 2
    RESERVED7[7]                 - (RO) Reserved bits
    THERM_CAL_VAL_2[14..8]       - (RO) cal value for subsys 2, RO
    RESERVED15[15]               - (RO) Reserved bits
    THERM_BUSY_2[16]             - (RO) bust bit for subsys 2
    RESERVED17[17]               - (RO) Reserved bits
    THERM_TRIGGER_2[18]          - (RW) trigger for subsys 2
    THERM_CAL_EN_2[19]           - (RW) enable for subsys_2
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMEN2_THERM_CAL_EN_2_ADDR            CONN_THERM_CTL_THERMEN2_ADDR
#define CONN_THERM_CTL_THERMEN2_THERM_CAL_EN_2_MASK            0x00080000                // THERM_CAL_EN_2[19]
#define CONN_THERM_CTL_THERMEN2_THERM_CAL_EN_2_SHFT            19
#define CONN_THERM_CTL_THERMEN2_THERM_TRIGGER_2_ADDR           CONN_THERM_CTL_THERMEN2_ADDR
#define CONN_THERM_CTL_THERMEN2_THERM_TRIGGER_2_MASK           0x00040000                // THERM_TRIGGER_2[18]
#define CONN_THERM_CTL_THERMEN2_THERM_TRIGGER_2_SHFT           18
#define CONN_THERM_CTL_THERMEN2_THERM_BUSY_2_ADDR              CONN_THERM_CTL_THERMEN2_ADDR
#define CONN_THERM_CTL_THERMEN2_THERM_BUSY_2_MASK              0x00010000                // THERM_BUSY_2[16]
#define CONN_THERM_CTL_THERMEN2_THERM_BUSY_2_SHFT              16
#define CONN_THERM_CTL_THERMEN2_THERM_CAL_VAL_2_ADDR           CONN_THERM_CTL_THERMEN2_ADDR
#define CONN_THERM_CTL_THERMEN2_THERM_CAL_VAL_2_MASK           0x00007F00                // THERM_CAL_VAL_2[14..8]
#define CONN_THERM_CTL_THERMEN2_THERM_CAL_VAL_2_SHFT           8
#define CONN_THERM_CTL_THERMEN2_THERM_RAW_VAL_2_ADDR           CONN_THERM_CTL_THERMEN2_ADDR
#define CONN_THERM_CTL_THERMEN2_THERM_RAW_VAL_2_MASK           0x0000007F                // THERM_RAW_VAL_2[6..0]
#define CONN_THERM_CTL_THERMEN2_THERM_RAW_VAL_2_SHFT           0

/* =====================================================================================

  ---THERMEN3 (0x18040000 + 0x024)---

    THERM_RAW_VAL_3[6..0]        - (RO) raw value for subsys 3
    RESERVED7[7]                 - (RO) Reserved bits
    THERM_CAL_VAL_3[14..8]       - (RO) cal value for subsys 3, RO
    RESERVED15[15]               - (RO) Reserved bits
    THERM_BUSY_3[16]             - (RO) bust bit for subsys 3
    RESERVED17[17]               - (RO) Reserved bits
    THERM_TRIGGER_3[18]          - (RW) trigger for subsys 3
    THERM_CAL_EN_3[19]           - (RW) enable for subsys_3
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMEN3_THERM_CAL_EN_3_ADDR            CONN_THERM_CTL_THERMEN3_ADDR
#define CONN_THERM_CTL_THERMEN3_THERM_CAL_EN_3_MASK            0x00080000                // THERM_CAL_EN_3[19]
#define CONN_THERM_CTL_THERMEN3_THERM_CAL_EN_3_SHFT            19
#define CONN_THERM_CTL_THERMEN3_THERM_TRIGGER_3_ADDR           CONN_THERM_CTL_THERMEN3_ADDR
#define CONN_THERM_CTL_THERMEN3_THERM_TRIGGER_3_MASK           0x00040000                // THERM_TRIGGER_3[18]
#define CONN_THERM_CTL_THERMEN3_THERM_TRIGGER_3_SHFT           18
#define CONN_THERM_CTL_THERMEN3_THERM_BUSY_3_ADDR              CONN_THERM_CTL_THERMEN3_ADDR
#define CONN_THERM_CTL_THERMEN3_THERM_BUSY_3_MASK              0x00010000                // THERM_BUSY_3[16]
#define CONN_THERM_CTL_THERMEN3_THERM_BUSY_3_SHFT              16
#define CONN_THERM_CTL_THERMEN3_THERM_CAL_VAL_3_ADDR           CONN_THERM_CTL_THERMEN3_ADDR
#define CONN_THERM_CTL_THERMEN3_THERM_CAL_VAL_3_MASK           0x00007F00                // THERM_CAL_VAL_3[14..8]
#define CONN_THERM_CTL_THERMEN3_THERM_CAL_VAL_3_SHFT           8
#define CONN_THERM_CTL_THERMEN3_THERM_RAW_VAL_3_ADDR           CONN_THERM_CTL_THERMEN3_ADDR
#define CONN_THERM_CTL_THERMEN3_THERM_RAW_VAL_3_MASK           0x0000007F                // THERM_RAW_VAL_3[6..0]
#define CONN_THERM_CTL_THERMEN3_THERM_RAW_VAL_3_SHFT           0

/* =====================================================================================

  ---THERMEN4 (0x18040000 + 0x028)---

    THERM_RAW_VAL_4[6..0]        - (RO) raw value for subsys 4
    RESERVED7[7]                 - (RO) Reserved bits
    THERM_CAL_VAL_4[14..8]       - (RO) cal value for subsys 4, RO
    RESERVED15[15]               - (RO) Reserved bits
    THERM_BUSY_4[16]             - (RO) bust bit for subsys 4
    RESERVED17[17]               - (RO) Reserved bits
    THERM_TRIGGER_4[18]          - (RW) trigger for subsys 4
    THERM_CAL_EN_4[19]           - (RW) enable for subsys_4
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_THERM_CTL_THERMEN4_THERM_CAL_EN_4_ADDR            CONN_THERM_CTL_THERMEN4_ADDR
#define CONN_THERM_CTL_THERMEN4_THERM_CAL_EN_4_MASK            0x00080000                // THERM_CAL_EN_4[19]
#define CONN_THERM_CTL_THERMEN4_THERM_CAL_EN_4_SHFT            19
#define CONN_THERM_CTL_THERMEN4_THERM_TRIGGER_4_ADDR           CONN_THERM_CTL_THERMEN4_ADDR
#define CONN_THERM_CTL_THERMEN4_THERM_TRIGGER_4_MASK           0x00040000                // THERM_TRIGGER_4[18]
#define CONN_THERM_CTL_THERMEN4_THERM_TRIGGER_4_SHFT           18
#define CONN_THERM_CTL_THERMEN4_THERM_BUSY_4_ADDR              CONN_THERM_CTL_THERMEN4_ADDR
#define CONN_THERM_CTL_THERMEN4_THERM_BUSY_4_MASK              0x00010000                // THERM_BUSY_4[16]
#define CONN_THERM_CTL_THERMEN4_THERM_BUSY_4_SHFT              16
#define CONN_THERM_CTL_THERMEN4_THERM_CAL_VAL_4_ADDR           CONN_THERM_CTL_THERMEN4_ADDR
#define CONN_THERM_CTL_THERMEN4_THERM_CAL_VAL_4_MASK           0x00007F00                // THERM_CAL_VAL_4[14..8]
#define CONN_THERM_CTL_THERMEN4_THERM_CAL_VAL_4_SHFT           8
#define CONN_THERM_CTL_THERMEN4_THERM_RAW_VAL_4_ADDR           CONN_THERM_CTL_THERMEN4_ADDR
#define CONN_THERM_CTL_THERMEN4_THERM_RAW_VAL_4_MASK           0x0000007F                // THERM_RAW_VAL_4[6..0]
#define CONN_THERM_CTL_THERMEN4_THERM_RAW_VAL_4_SHFT           0

#endif // __CONN_THERM_CTL_REGS_H__
