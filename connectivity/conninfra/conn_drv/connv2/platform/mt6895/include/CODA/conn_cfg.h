/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_cfg.h
//[Revision time]   : Mon Aug 30 16:01:52 2021

#ifndef __CONN_CFG_REGS_H__
#define __CONN_CFG_REGS_H__

//****************************************************************************
//
//                     CONN_CFG CR Definitions                     
//
//****************************************************************************

#define CONN_CFG_BASE                                          (CONN_REG_CONN_INFRA_CFG_ADDR)

#define CONN_CFG_IP_VERSION_ADDR                               (CONN_CFG_BASE + 0x000) // 1000
#define CONN_CFG_CFG_VERSION_ADDR                              (CONN_CFG_BASE + 0x004) // 1004
#define CONN_CFG_STRAP_STATUS_ADDR                             (CONN_CFG_BASE + 0x010) // 1010
#define CONN_CFG_EFUSE_ADDR                                    (CONN_CFG_BASE + 0x020) // 1020
#define CONN_CFG_PLL_STATUS_ADDR                               (CONN_CFG_BASE + 0x030) // 1030
#define CONN_CFG_BUS_STATUS_ADDR                               (CONN_CFG_BASE + 0x034) // 1034
#define CONN_CFG_CFG_RSV0_ADDR                                 (CONN_CFG_BASE + 0x040) // 1040
#define CONN_CFG_CFG_RSV1_ADDR                                 (CONN_CFG_BASE + 0x044) // 1044
#define CONN_CFG_CMDBT_FETCH_START_ADDR0_ADDR                  (CONN_CFG_BASE + 0x050) // 1050
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_ADDR                 (CONN_CFG_BASE + 0x060) // 1060
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_ADDR                 (CONN_CFG_BASE + 0x064) // 1064
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_2_ADDR                 (CONN_CFG_BASE + 0x068) // 1068
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_ADDR                 (CONN_CFG_BASE + 0x06C) // 106C
#define CONN_CFG_EMI_CTL_0_ADDR                                (CONN_CFG_BASE + 0x100) // 1100
#define CONN_CFG_EMI_CTL_1_ADDR                                (CONN_CFG_BASE + 0x104) // 1104
#define CONN_CFG_EMI_CTL_TOP_ADDR                              (CONN_CFG_BASE + 0x110) // 1110
#define CONN_CFG_EMI_CTL_WF_ADDR                               (CONN_CFG_BASE + 0x114) // 1114
#define CONN_CFG_EMI_CTL_BT_ADDR                               (CONN_CFG_BASE + 0x118) // 1118
#define CONN_CFG_EMI_CTL_GPS_ADDR                              (CONN_CFG_BASE + 0x11C) // 111C
#define CONN_CFG_EMI_CTL_GPS_L1_ADDR                           (CONN_CFG_BASE + 0X120) // 1120
#define CONN_CFG_EMI_CTL_GPS_L5_ADDR                           (CONN_CFG_BASE + 0X124) // 1124
#define CONN_CFG_EMI_PROBE_ADDR                                (CONN_CFG_BASE + 0x130) // 1130
#define CONN_CFG_EMI_PROBE_1_ADDR                              (CONN_CFG_BASE + 0x134) // 1134




/* =====================================================================================

  ---IP_VERSION (0x18011000 + 0x000)---

    IP_VERSION[31..0]            - (RO) IP_VERSION

 =====================================================================================*/
#define CONN_CFG_IP_VERSION_IP_VERSION_ADDR                    CONN_CFG_IP_VERSION_ADDR
#define CONN_CFG_IP_VERSION_IP_VERSION_MASK                    0xFFFFFFFF                // IP_VERSION[31..0]
#define CONN_CFG_IP_VERSION_IP_VERSION_SHFT                    0

/* =====================================================================================

  ---CFG_VERSION (0x18011000 + 0x004)---

    CFG_VERSION[31..0]           - (RO) CFG_VERSION

 =====================================================================================*/
#define CONN_CFG_CFG_VERSION_CFG_VERSION_ADDR                  CONN_CFG_CFG_VERSION_ADDR
#define CONN_CFG_CFG_VERSION_CFG_VERSION_MASK                  0xFFFFFFFF                // CFG_VERSION[31..0]
#define CONN_CFG_CFG_VERSION_CFG_VERSION_SHFT                  0

/* =====================================================================================

  ---STRAP_STATUS (0x18011000 + 0x010)---

    OSC_IS_20M[0]                - (RO) OSC is 20MHz strap status (active-high)
    OSC_IS_24M[1]                - (RO) OSC is 24MHz strap status (active-high)
    OSC_IS_25M[2]                - (RO) OSC is 25MHz strap status (active-high)
    OSC_IS_26M[3]                - (RO) OSC is 26MHz strap status (active-high)
    OSC_IS_40M[4]                - (RO) OSC is 40MHz strap status (active-high)
    OSC_IS_52M[5]                - (RO) OSC is 52MHz strap status (active-high)
    RESERVED6[7..6]              - (RO) Reserved bits
    OLT_BLT_MODE[8]              - (RO) OLT BLT mode strap status (active-high)
    SYSSTRAP_MODE[9]             - (RO) conn_infra system debug mode strap status (active-high)
    WFSYSSTRAP_MODE[10]          - (RO) wfsys system debug mode strap status (active-high)
    BGFSYSSTRAP_MODE[11]         - (RO) bgfsys system debug mode strap status (active-high)
    RBIST_MODE[12]               - (RO) connsys RBIST mode strap status (active-high)
    CONN_SPI2AHB_MODE[13]        - (RO) connsys SPI to AHB mode strap status (active-high)
    CONN_TEST_MODE[14]           - (RO) connsys test mode strap status, which indicates SPI2AHB, RBIST, ATPG mode (active-high)
    CONN_FORCE_PWR_ON_MODE[15]   - (RO) force connsys MTCMOS and memory all on mode strap status (active-high)
    CONN_EXTCK_MODE[16]          - (RO) connsys external clock mode strap status (active-high)
    CONN_BYPASS_ROM_MODE[17]     - (RO) connsys bypass rom code mode strap status (active-high)
    CONN_SPEEDUP_OSC_STABLE_MODE[18] - (RO) connsys speedup OSC stable mode strap status (active-high)
    RESERVED19[31..19]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_STRAP_STATUS_CONN_SPEEDUP_OSC_STABLE_MODE_ADDR CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_CONN_SPEEDUP_OSC_STABLE_MODE_MASK 0x00040000                // CONN_SPEEDUP_OSC_STABLE_MODE[18]
#define CONN_CFG_STRAP_STATUS_CONN_SPEEDUP_OSC_STABLE_MODE_SHFT 18
#define CONN_CFG_STRAP_STATUS_CONN_BYPASS_ROM_MODE_ADDR        CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_CONN_BYPASS_ROM_MODE_MASK        0x00020000                // CONN_BYPASS_ROM_MODE[17]
#define CONN_CFG_STRAP_STATUS_CONN_BYPASS_ROM_MODE_SHFT        17
#define CONN_CFG_STRAP_STATUS_CONN_EXTCK_MODE_ADDR             CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_CONN_EXTCK_MODE_MASK             0x00010000                // CONN_EXTCK_MODE[16]
#define CONN_CFG_STRAP_STATUS_CONN_EXTCK_MODE_SHFT             16
#define CONN_CFG_STRAP_STATUS_CONN_FORCE_PWR_ON_MODE_ADDR      CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_CONN_FORCE_PWR_ON_MODE_MASK      0x00008000                // CONN_FORCE_PWR_ON_MODE[15]
#define CONN_CFG_STRAP_STATUS_CONN_FORCE_PWR_ON_MODE_SHFT      15
#define CONN_CFG_STRAP_STATUS_CONN_TEST_MODE_ADDR              CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_CONN_TEST_MODE_MASK              0x00004000                // CONN_TEST_MODE[14]
#define CONN_CFG_STRAP_STATUS_CONN_TEST_MODE_SHFT              14
#define CONN_CFG_STRAP_STATUS_CONN_SPI2AHB_MODE_ADDR           CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_CONN_SPI2AHB_MODE_MASK           0x00002000                // CONN_SPI2AHB_MODE[13]
#define CONN_CFG_STRAP_STATUS_CONN_SPI2AHB_MODE_SHFT           13
#define CONN_CFG_STRAP_STATUS_RBIST_MODE_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_RBIST_MODE_MASK                  0x00001000                // RBIST_MODE[12]
#define CONN_CFG_STRAP_STATUS_RBIST_MODE_SHFT                  12
#define CONN_CFG_STRAP_STATUS_BGFSYSSTRAP_MODE_ADDR            CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_BGFSYSSTRAP_MODE_MASK            0x00000800                // BGFSYSSTRAP_MODE[11]
#define CONN_CFG_STRAP_STATUS_BGFSYSSTRAP_MODE_SHFT            11
#define CONN_CFG_STRAP_STATUS_WFSYSSTRAP_MODE_ADDR             CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_WFSYSSTRAP_MODE_MASK             0x00000400                // WFSYSSTRAP_MODE[10]
#define CONN_CFG_STRAP_STATUS_WFSYSSTRAP_MODE_SHFT             10
#define CONN_CFG_STRAP_STATUS_SYSSTRAP_MODE_ADDR               CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_SYSSTRAP_MODE_MASK               0x00000200                // SYSSTRAP_MODE[9]
#define CONN_CFG_STRAP_STATUS_SYSSTRAP_MODE_SHFT               9
#define CONN_CFG_STRAP_STATUS_OLT_BLT_MODE_ADDR                CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OLT_BLT_MODE_MASK                0x00000100                // OLT_BLT_MODE[8]
#define CONN_CFG_STRAP_STATUS_OLT_BLT_MODE_SHFT                8
#define CONN_CFG_STRAP_STATUS_OSC_IS_52M_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OSC_IS_52M_MASK                  0x00000020                // OSC_IS_52M[5]
#define CONN_CFG_STRAP_STATUS_OSC_IS_52M_SHFT                  5
#define CONN_CFG_STRAP_STATUS_OSC_IS_40M_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OSC_IS_40M_MASK                  0x00000010                // OSC_IS_40M[4]
#define CONN_CFG_STRAP_STATUS_OSC_IS_40M_SHFT                  4
#define CONN_CFG_STRAP_STATUS_OSC_IS_26M_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OSC_IS_26M_MASK                  0x00000008                // OSC_IS_26M[3]
#define CONN_CFG_STRAP_STATUS_OSC_IS_26M_SHFT                  3
#define CONN_CFG_STRAP_STATUS_OSC_IS_25M_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OSC_IS_25M_MASK                  0x00000004                // OSC_IS_25M[2]
#define CONN_CFG_STRAP_STATUS_OSC_IS_25M_SHFT                  2
#define CONN_CFG_STRAP_STATUS_OSC_IS_24M_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OSC_IS_24M_MASK                  0x00000002                // OSC_IS_24M[1]
#define CONN_CFG_STRAP_STATUS_OSC_IS_24M_SHFT                  1
#define CONN_CFG_STRAP_STATUS_OSC_IS_20M_ADDR                  CONN_CFG_STRAP_STATUS_ADDR
#define CONN_CFG_STRAP_STATUS_OSC_IS_20M_MASK                  0x00000001                // OSC_IS_20M[0]
#define CONN_CFG_STRAP_STATUS_OSC_IS_20M_SHFT                  0

/* =====================================================================================

  ---EFUSE (0x18011000 + 0x020)---

    AP2CONN_EFUSE_DATA[15..0]    - (RO) efuse data
    AP2CONN_EFUSE_DATA_SECURITY[25..16] - (RO) efuse data
    RESERVED26[31..26]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EFUSE_AP2CONN_EFUSE_DATA_SECURITY_ADDR        CONN_CFG_EFUSE_ADDR
#define CONN_CFG_EFUSE_AP2CONN_EFUSE_DATA_SECURITY_MASK        0x03FF0000                // AP2CONN_EFUSE_DATA_SECURITY[25..16]
#define CONN_CFG_EFUSE_AP2CONN_EFUSE_DATA_SECURITY_SHFT        16
#define CONN_CFG_EFUSE_AP2CONN_EFUSE_DATA_ADDR                 CONN_CFG_EFUSE_ADDR
#define CONN_CFG_EFUSE_AP2CONN_EFUSE_DATA_MASK                 0x0000FFFF                // AP2CONN_EFUSE_DATA[15..0]
#define CONN_CFG_EFUSE_AP2CONN_EFUSE_DATA_SHFT                 0

/* =====================================================================================

  ---PLL_STATUS (0x18011000 + 0x030)---

    WPLL_RDY[0]                  - (RO) WPLL ready
                                     1'h0: not ready
                                     1'h1: ready
    BPLL_RDY[1]                  - (RO) BPLL ready
                                     1'h0: not ready
                                     1'h1: ready
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_PLL_STATUS_BPLL_RDY_ADDR                      CONN_CFG_PLL_STATUS_ADDR
#define CONN_CFG_PLL_STATUS_BPLL_RDY_MASK                      0x00000002                // BPLL_RDY[1]
#define CONN_CFG_PLL_STATUS_BPLL_RDY_SHFT                      1
#define CONN_CFG_PLL_STATUS_WPLL_RDY_ADDR                      CONN_CFG_PLL_STATUS_ADDR
#define CONN_CFG_PLL_STATUS_WPLL_RDY_MASK                      0x00000001                // WPLL_RDY[0]
#define CONN_CFG_PLL_STATUS_WPLL_RDY_SHFT                      0

/* =====================================================================================

  ---BUS_STATUS (0x18011000 + 0x034)---

    BUS_OSC_SW_RDY[0]            - (RO) conn_infra BUS clock switch to OSC clock ready indicator(hclk_switch_rdy[0])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_32K_SW_RDY[1]            - (RO) conn_infra BUS clock switch to 32KHz clock ready indicator(hclk_switch_rdy[1])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_BGFSYS_CK_SW_RDY_0[2]    - (RO) conn_infra BUS clock switch to bgfsys BUS clock ready indicator(hclk_switch_rdy[2])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_BGFSYS_CK_SW_RDY_1[3]    - (RO) conn_infra BUS clock switch to bgfsys BUS clock ready indicator(hclk_switch_rdy[3])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_WFSYS_CK_SW_RDY_0[4]     - (RO) conn_infra BUS clock switch to wfsys BUS clock ready indicator(hclk_switch_rdy[4])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_WFSYS_CK_SW_RDY_1[5]     - (RO) conn_infra BUS clock switch to wfsys BUS clock ready indicator(hclk_switch_rdy[5])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_WFSYS_CK_SW_RDY_2[6]     - (RO) conn_infra BUS clock switch to wfsys BUS clock ready indicator(hclk_switch_rdy[6])
                                     1'h0: not ready
                                     1'h1: ready
    BUS_ICAP_CK_SW_RDY[7]        - (RO) conn_infra BUS clock switch to ICAP BUS clock ready indicator(hclk_switch_rdy[7])
                                     1'h0: not ready
                                     1'h1: ready
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_BUS_STATUS_BUS_ICAP_CK_SW_RDY_ADDR            CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_ICAP_CK_SW_RDY_MASK            0x00000080                // BUS_ICAP_CK_SW_RDY[7]
#define CONN_CFG_BUS_STATUS_BUS_ICAP_CK_SW_RDY_SHFT            7
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_2_ADDR         CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_2_MASK         0x00000040                // BUS_WFSYS_CK_SW_RDY_2[6]
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_2_SHFT         6
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_1_ADDR         CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_1_MASK         0x00000020                // BUS_WFSYS_CK_SW_RDY_1[5]
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_1_SHFT         5
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_0_ADDR         CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_0_MASK         0x00000010                // BUS_WFSYS_CK_SW_RDY_0[4]
#define CONN_CFG_BUS_STATUS_BUS_WFSYS_CK_SW_RDY_0_SHFT         4
#define CONN_CFG_BUS_STATUS_BUS_BGFSYS_CK_SW_RDY_1_ADDR        CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_BGFSYS_CK_SW_RDY_1_MASK        0x00000008                // BUS_BGFSYS_CK_SW_RDY_1[3]
#define CONN_CFG_BUS_STATUS_BUS_BGFSYS_CK_SW_RDY_1_SHFT        3
#define CONN_CFG_BUS_STATUS_BUS_BGFSYS_CK_SW_RDY_0_ADDR        CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_BGFSYS_CK_SW_RDY_0_MASK        0x00000004                // BUS_BGFSYS_CK_SW_RDY_0[2]
#define CONN_CFG_BUS_STATUS_BUS_BGFSYS_CK_SW_RDY_0_SHFT        2
#define CONN_CFG_BUS_STATUS_BUS_32K_SW_RDY_ADDR                CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_32K_SW_RDY_MASK                0x00000002                // BUS_32K_SW_RDY[1]
#define CONN_CFG_BUS_STATUS_BUS_32K_SW_RDY_SHFT                1
#define CONN_CFG_BUS_STATUS_BUS_OSC_SW_RDY_ADDR                CONN_CFG_BUS_STATUS_ADDR
#define CONN_CFG_BUS_STATUS_BUS_OSC_SW_RDY_MASK                0x00000001                // BUS_OSC_SW_RDY[0]
#define CONN_CFG_BUS_STATUS_BUS_OSC_SW_RDY_SHFT                0

/* =====================================================================================

  ---CFG_RSV0 (0x18011000 + 0x040)---

    RSV0[31..0]                  - (RW) reserved CR

 =====================================================================================*/
#define CONN_CFG_CFG_RSV0_RSV0_ADDR                            CONN_CFG_CFG_RSV0_ADDR
#define CONN_CFG_CFG_RSV0_RSV0_MASK                            0xFFFFFFFF                // RSV0[31..0]
#define CONN_CFG_CFG_RSV0_RSV0_SHFT                            0

/* =====================================================================================

  ---CFG_RSV1 (0x18011000 + 0x044)---

    RSV1[31..0]                  - (RW) reserved CR

 =====================================================================================*/
#define CONN_CFG_CFG_RSV1_RSV1_ADDR                            CONN_CFG_CFG_RSV1_ADDR
#define CONN_CFG_CFG_RSV1_RSV1_MASK                            0xFFFFFFFF                // RSV1[31..0]
#define CONN_CFG_CFG_RSV1_RSV1_SHFT                            0

/* =====================================================================================

  ---CMDBT_FETCH_START_ADDR0 (0x18011000 + 0x050)---

    cmdbt_fetch_start_addr0[19..0] - (RW) cmdbt fetch start address 0 for top backup instruction address
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_CMDBT_FETCH_START_ADDR0_cmdbt_fetch_start_addr0_ADDR CONN_CFG_CMDBT_FETCH_START_ADDR0_ADDR
#define CONN_CFG_CMDBT_FETCH_START_ADDR0_cmdbt_fetch_start_addr0_MASK 0x000FFFFF                // cmdbt_fetch_start_addr0[19..0]
#define CONN_CFG_CMDBT_FETCH_START_ADDR0_cmdbt_fetch_start_addr0_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_SYSRAM_CTRL_0 (0x18011000 + 0x060)---

    CONN_INFRA_SYSRAM_DELSEL_UMS[19..0] - (RW)  xxx 
    CONN_INFRA_SYSRAM_MBIST_USE_DEFAULT_DESEL[20] - (RW)  xxx 
    CONN_INFRA_SYSRAM_HDEN[23..21] - (RW)  xxx 
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_HDEN_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_HDEN_MASK 0x00E00000                // CONN_INFRA_SYSRAM_HDEN[23..21]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_HDEN_SHFT 21
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_MBIST_USE_DEFAULT_DESEL_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_MBIST_USE_DEFAULT_DESEL_MASK 0x00100000                // CONN_INFRA_SYSRAM_MBIST_USE_DEFAULT_DESEL[20]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_MBIST_USE_DEFAULT_DESEL_SHFT 20
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_DELSEL_UMS_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_DELSEL_UMS_MASK 0x000FFFFF                // CONN_INFRA_SYSRAM_DELSEL_UMS[19..0]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_0_CONN_INFRA_SYSRAM_DELSEL_UMS_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_SYSRAM_CTRL_1 (0x18011000 + 0x064)---

    CONN_INFRA_SYSRAM_PD_SEL_SCAN[3..0] - (RW)  xxx 
    CONN_INFRA_SYSRAM_AWT[6..4]  - (RW)  xxx 
    CONN_INFRA_SYSRAM_ERR_CLR[7] - (RW)  xxx 
    CONN_INFRA_SYSRAM_ERR_ADR[23..8] - (RO)  xxx 
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_ERR_ADR_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_ERR_ADR_MASK 0x00FFFF00                // CONN_INFRA_SYSRAM_ERR_ADR[23..8]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_ERR_ADR_SHFT 8
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_ERR_CLR_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_ERR_CLR_MASK 0x00000080                // CONN_INFRA_SYSRAM_ERR_CLR[7]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_ERR_CLR_SHFT 7
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_AWT_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_AWT_MASK 0x00000070                // CONN_INFRA_SYSRAM_AWT[6..4]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_AWT_SHFT 4
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_PD_SEL_SCAN_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_PD_SEL_SCAN_MASK 0x0000000F                // CONN_INFRA_SYSRAM_PD_SEL_SCAN[3..0]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_1_CONN_INFRA_SYSRAM_PD_SEL_SCAN_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_SYSRAM_CTRL_2 (0x18011000 + 0x068)---

    CONN_INFRA_SYSRAM_ERROR_INFO[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_2_CONN_INFRA_SYSRAM_ERROR_INFO_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_2_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_2_CONN_INFRA_SYSRAM_ERROR_INFO_MASK 0xFFFFFFFF                // CONN_INFRA_SYSRAM_ERROR_INFO[31..0]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_2_CONN_INFRA_SYSRAM_ERROR_INFO_SHFT 0

/* =====================================================================================

  ---CONN_INFRA_SYSRAM_CTRL_3 (0x18011000 + 0x06C)---

    CONN_INFRA_SYSRAM_Y_DELSEL_UMS[19..0] - (RW)  xxx 
    CONN_INFRA_SYSRAM_Y_MBIST_USE_DEFAULT_DESEL[20] - (RW)  xxx 
    RESERVED21[31..21]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_CONN_INFRA_SYSRAM_Y_MBIST_USE_DEFAULT_DESEL_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_CONN_INFRA_SYSRAM_Y_MBIST_USE_DEFAULT_DESEL_MASK 0x00100000                // CONN_INFRA_SYSRAM_Y_MBIST_USE_DEFAULT_DESEL[20]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_CONN_INFRA_SYSRAM_Y_MBIST_USE_DEFAULT_DESEL_SHFT 20
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_CONN_INFRA_SYSRAM_Y_DELSEL_UMS_ADDR CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_ADDR
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_CONN_INFRA_SYSRAM_Y_DELSEL_UMS_MASK 0x000FFFFF                // CONN_INFRA_SYSRAM_Y_DELSEL_UMS[19..0]
#define CONN_CFG_CONN_INFRA_SYSRAM_CTRL_3_CONN_INFRA_SYSRAM_Y_DELSEL_UMS_SHFT 0

/* =====================================================================================

  ---EMI_CTL_0 (0x18011000 + 0x100)---

    RESERVED0[0]                 - (RO) Reserved bits
    EMI_CONN2AP_BUS_SLPPROT_BYPASS[1] - (RW) bypass CONN2AP bus slpprotect rdy check 
                                     1'b0: emi control signal deassert after conn2ap_bus_slpprot_rdy = 1
                                     1'b1: emi control signal deassert don't need check conn2ap_bus_slpprot_rdy
    CONN2AP_EMI_REQ[2]           - (RO) conn2ap BUS request
    CONN2AP_EMI_ONLY_REQ[3]      - (RO) conn2emi BUS request
    DDR_CNT_LIMIT[14..4]         - (RW) counter limit for ddr_en auto timeout 
                                     set 0, ddr_en auto turn off function is disable
                                     set others: ddr_en is auto turn off when ddr_cnt is count to ddr_cnt_limit
    DDR_EN_CNT_UPDATE[15]        - (RW) counter limit for ddr_en timeout update signal
                                     1'b1: ddr_en timeout value = CR value(need set low after value has updated). Only update value at 1'b0 -> 1'b1 (rising edge)
                                     1'b0: ddr_em timeout value don't update
    RESERVED16[16]               - (RO) Reserved bits
    DDR_EN_BP_PROT[17]           - (RW) ddr_en sw control bypass cr_emi_req limitation
                                     1'b0: When cr_emi_req_xxx high , ddr_en can't be disable by sw control
                                     1'b1: When cr_emi_req_xxx high , ddr_en can be disable by sw control
    EMI_SLPPROT_BP_DDR_EN[18]    - (RW) sleep protect control bypass ddr_en
                                     1'b0: ddr_en_ack  = low  -> sleep protect enable
                                     1'b1: sleep protect control bypass ddr_en_ack status
    EMI_SLPPROT_BP_APSRC_REQ[19] - (RW) sleep protect control bypass apsrc_req
                                     1'b0:apsrc_ack = low  -> sleep protect enable
                                     1'b1: sleep protect control bypass apsrc_ack status
    INFRA_ONLY_MODE[20]          - (RW) emi_ctl infra only mode
                                     1'b0: conn2infra request will turn on emi
                                     1'b1: conn2infra request wiill only turon infra bus, emi keep power off
    EMI_CTL_DEBUG_1_SEL[22..21]  - (RW) conn_infra_emi_ctl_1 debug selection
    CONN_INFRA_OFF2ON_REQ_MASK[23] - (RW) conn_infra off2on req mask
                                     1'b0: unmask, conn_infra bus off2on request will trigger emi_ctl enable infra mtcmos
                                     1'b1: mask
    CONN_INFRA_OFF2ON_REQ[24]    - (RO) conn_infra off2on BUS request
    RESERVED25[25]               - (RO) Reserved bits
    CR_PATH_SLP_PROT_DIS_BP[26]  - (RW) bypass check conn2ap cr path sleep protect disable
    EMI_PATH_SLP_PROT_DIS_BP[27] - (RW) bypass check conn2ap emi path sleep protect disable
    EMI_PATH_SLP_PROT_EN_BP[28]  - (RW) bypass check conn2ap emi path sleep protect enable
    EMI_CTL_RSV_0[31..29]        - (RW) Reserved CR

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_0_EMI_CTL_RSV_0_ADDR                  CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_CTL_RSV_0_MASK                  0xE0000000                // EMI_CTL_RSV_0[31..29]
#define CONN_CFG_EMI_CTL_0_EMI_CTL_RSV_0_SHFT                  29
#define CONN_CFG_EMI_CTL_0_EMI_PATH_SLP_PROT_EN_BP_ADDR        CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_PATH_SLP_PROT_EN_BP_MASK        0x10000000                // EMI_PATH_SLP_PROT_EN_BP[28]
#define CONN_CFG_EMI_CTL_0_EMI_PATH_SLP_PROT_EN_BP_SHFT        28
#define CONN_CFG_EMI_CTL_0_EMI_PATH_SLP_PROT_DIS_BP_ADDR       CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_PATH_SLP_PROT_DIS_BP_MASK       0x08000000                // EMI_PATH_SLP_PROT_DIS_BP[27]
#define CONN_CFG_EMI_CTL_0_EMI_PATH_SLP_PROT_DIS_BP_SHFT       27
#define CONN_CFG_EMI_CTL_0_CR_PATH_SLP_PROT_DIS_BP_ADDR        CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_CR_PATH_SLP_PROT_DIS_BP_MASK        0x04000000                // CR_PATH_SLP_PROT_DIS_BP[26]
#define CONN_CFG_EMI_CTL_0_CR_PATH_SLP_PROT_DIS_BP_SHFT        26
#define CONN_CFG_EMI_CTL_0_CONN_INFRA_OFF2ON_REQ_ADDR          CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_CONN_INFRA_OFF2ON_REQ_MASK          0x01000000                // CONN_INFRA_OFF2ON_REQ[24]
#define CONN_CFG_EMI_CTL_0_CONN_INFRA_OFF2ON_REQ_SHFT          24
#define CONN_CFG_EMI_CTL_0_CONN_INFRA_OFF2ON_REQ_MASK_ADDR     CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_CONN_INFRA_OFF2ON_REQ_MASK_MASK     0x00800000                // CONN_INFRA_OFF2ON_REQ_MASK[23]
#define CONN_CFG_EMI_CTL_0_CONN_INFRA_OFF2ON_REQ_MASK_SHFT     23
#define CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_ADDR            CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_MASK            0x00600000                // EMI_CTL_DEBUG_1_SEL[22..21]
#define CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL_SHFT            21
#define CONN_CFG_EMI_CTL_0_INFRA_ONLY_MODE_ADDR                CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_INFRA_ONLY_MODE_MASK                0x00100000                // INFRA_ONLY_MODE[20]
#define CONN_CFG_EMI_CTL_0_INFRA_ONLY_MODE_SHFT                20
#define CONN_CFG_EMI_CTL_0_EMI_SLPPROT_BP_APSRC_REQ_ADDR       CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_SLPPROT_BP_APSRC_REQ_MASK       0x00080000                // EMI_SLPPROT_BP_APSRC_REQ[19]
#define CONN_CFG_EMI_CTL_0_EMI_SLPPROT_BP_APSRC_REQ_SHFT       19
#define CONN_CFG_EMI_CTL_0_EMI_SLPPROT_BP_DDR_EN_ADDR          CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_SLPPROT_BP_DDR_EN_MASK          0x00040000                // EMI_SLPPROT_BP_DDR_EN[18]
#define CONN_CFG_EMI_CTL_0_EMI_SLPPROT_BP_DDR_EN_SHFT          18
#define CONN_CFG_EMI_CTL_0_DDR_EN_BP_PROT_ADDR                 CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_DDR_EN_BP_PROT_MASK                 0x00020000                // DDR_EN_BP_PROT[17]
#define CONN_CFG_EMI_CTL_0_DDR_EN_BP_PROT_SHFT                 17
#define CONN_CFG_EMI_CTL_0_DDR_EN_CNT_UPDATE_ADDR              CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_DDR_EN_CNT_UPDATE_MASK              0x00008000                // DDR_EN_CNT_UPDATE[15]
#define CONN_CFG_EMI_CTL_0_DDR_EN_CNT_UPDATE_SHFT              15
#define CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT_ADDR                  CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT_MASK                  0x00007FF0                // DDR_CNT_LIMIT[14..4]
#define CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT_SHFT                  4
#define CONN_CFG_EMI_CTL_0_CONN2AP_EMI_ONLY_REQ_ADDR           CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_CONN2AP_EMI_ONLY_REQ_MASK           0x00000008                // CONN2AP_EMI_ONLY_REQ[3]
#define CONN_CFG_EMI_CTL_0_CONN2AP_EMI_ONLY_REQ_SHFT           3
#define CONN_CFG_EMI_CTL_0_CONN2AP_EMI_REQ_ADDR                CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_CONN2AP_EMI_REQ_MASK                0x00000004                // CONN2AP_EMI_REQ[2]
#define CONN_CFG_EMI_CTL_0_CONN2AP_EMI_REQ_SHFT                2
#define CONN_CFG_EMI_CTL_0_EMI_CONN2AP_BUS_SLPPROT_BYPASS_ADDR CONN_CFG_EMI_CTL_0_ADDR
#define CONN_CFG_EMI_CTL_0_EMI_CONN2AP_BUS_SLPPROT_BYPASS_MASK 0x00000002                // EMI_CONN2AP_BUS_SLPPROT_BYPASS[1]
#define CONN_CFG_EMI_CTL_0_EMI_CONN2AP_BUS_SLPPROT_BYPASS_SHFT 1

/* =====================================================================================

  ---EMI_CTL_1 (0x18011000 + 0x104)---

    SRCCLKENA_PROT_EN[0]         - (RW) enable srcclkena protect for conn_ddr_en
    SRCCLKENA_ACK_BYPASS[1]      - (RW) bypass srcclkena_ack check for conn2ap bus sleep protect
    SRCCLKENA_ACK[2]             - (RO) srcclkena_ack
    SRCCLKENA_ACK_ERR[3]         - (RO) SRCCLKENA ACK error drop low when request is active
    SRCCLKENA_PROT_LIMIT[7..4]   - (RW) srcclkena prortect cycles to avoid using ack signal  after ddr_req is rising (unit xtal clock), because spm is under metastable
    AP_BUS_PROT_EN[8]            - (RW) enable ap_bus protect for conn_ddr_en
    AP_BUS_ACK_BYPASS[9]         - (RW) bypass ap_bus_ack check for conn2ap bus sleep protect
    AP_BUS_ACK[10]               - (RO) ap_bus_ack
    AP_BUS_ACK_ERR[11]           - (RO) AP_BUS_ACK error drop low when request is active
    AP_BUS_PROT_LIMIT[15..12]    - (RW) AP BUS prortect cycles to avoid using ack signal  after ddr_req is rising (unit xtal clock), because spm is under metastable
    APSRC_PROT_EN[16]            - (RW) enable ddr protect for apsrc_en
    APSRC_ACK_BYPASS[17]         - (RW) bypass apsrc_ack check for conn2ap bus sleep protect
    APSRC_ACK[18]                - (RO) apsrc_ack
    APSRC_ACK_ERR[19]            - (RO) APSRC ACK error drop low when request is active
    APSRC_PROT_LIMIT[23..20]     - (RW) APSRC prortect cycles to avoid using ack signal  after ddr_req is rising (unit xtal clock), because spm is under metastable
    DDR_PROT_EN[24]              - (RW) enable ddr protect for conn_ddr_en
    DDR_EN_ACK_BYPASS[25]        - (RW) bypass ddr_en_ack check for conn2ap bus sleep protect
    DDR_EN_ACK[26]               - (RO) ddr_en_ack
    DDR_EN_ACK_ERR[27]           - (RO) DDR ACK error drop low when request is active
    DDR_PROT_LIMIT[31..28]       - (RW) ddr prortect cycles to avoid using ack signal  after ddr_req is rising (unit xtal clock), because spm is under metastable

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_1_DDR_PROT_LIMIT_ADDR                 CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_DDR_PROT_LIMIT_MASK                 0xF0000000                // DDR_PROT_LIMIT[31..28]
#define CONN_CFG_EMI_CTL_1_DDR_PROT_LIMIT_SHFT                 28
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_ERR_ADDR                 CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_ERR_MASK                 0x08000000                // DDR_EN_ACK_ERR[27]
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_ERR_SHFT                 27
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_ADDR                     CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_MASK                     0x04000000                // DDR_EN_ACK[26]
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_SHFT                     26
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_BYPASS_ADDR              CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_BYPASS_MASK              0x02000000                // DDR_EN_ACK_BYPASS[25]
#define CONN_CFG_EMI_CTL_1_DDR_EN_ACK_BYPASS_SHFT              25
#define CONN_CFG_EMI_CTL_1_DDR_PROT_EN_ADDR                    CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_DDR_PROT_EN_MASK                    0x01000000                // DDR_PROT_EN[24]
#define CONN_CFG_EMI_CTL_1_DDR_PROT_EN_SHFT                    24
#define CONN_CFG_EMI_CTL_1_APSRC_PROT_LIMIT_ADDR               CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_APSRC_PROT_LIMIT_MASK               0x00F00000                // APSRC_PROT_LIMIT[23..20]
#define CONN_CFG_EMI_CTL_1_APSRC_PROT_LIMIT_SHFT               20
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_ERR_ADDR                  CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_ERR_MASK                  0x00080000                // APSRC_ACK_ERR[19]
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_ERR_SHFT                  19
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_ADDR                      CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_MASK                      0x00040000                // APSRC_ACK[18]
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_SHFT                      18
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_BYPASS_ADDR               CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_BYPASS_MASK               0x00020000                // APSRC_ACK_BYPASS[17]
#define CONN_CFG_EMI_CTL_1_APSRC_ACK_BYPASS_SHFT               17
#define CONN_CFG_EMI_CTL_1_APSRC_PROT_EN_ADDR                  CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_APSRC_PROT_EN_MASK                  0x00010000                // APSRC_PROT_EN[16]
#define CONN_CFG_EMI_CTL_1_APSRC_PROT_EN_SHFT                  16
#define CONN_CFG_EMI_CTL_1_AP_BUS_PROT_LIMIT_ADDR              CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_AP_BUS_PROT_LIMIT_MASK              0x0000F000                // AP_BUS_PROT_LIMIT[15..12]
#define CONN_CFG_EMI_CTL_1_AP_BUS_PROT_LIMIT_SHFT              12
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_ERR_ADDR                 CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_ERR_MASK                 0x00000800                // AP_BUS_ACK_ERR[11]
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_ERR_SHFT                 11
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_ADDR                     CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_MASK                     0x00000400                // AP_BUS_ACK[10]
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_SHFT                     10
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_BYPASS_ADDR              CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_BYPASS_MASK              0x00000200                // AP_BUS_ACK_BYPASS[9]
#define CONN_CFG_EMI_CTL_1_AP_BUS_ACK_BYPASS_SHFT              9
#define CONN_CFG_EMI_CTL_1_AP_BUS_PROT_EN_ADDR                 CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_AP_BUS_PROT_EN_MASK                 0x00000100                // AP_BUS_PROT_EN[8]
#define CONN_CFG_EMI_CTL_1_AP_BUS_PROT_EN_SHFT                 8
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_PROT_LIMIT_ADDR           CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_PROT_LIMIT_MASK           0x000000F0                // SRCCLKENA_PROT_LIMIT[7..4]
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_PROT_LIMIT_SHFT           4
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_ERR_ADDR              CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_ERR_MASK              0x00000008                // SRCCLKENA_ACK_ERR[3]
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_ERR_SHFT              3
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_ADDR                  CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_MASK                  0x00000004                // SRCCLKENA_ACK[2]
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_SHFT                  2
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_BYPASS_ADDR           CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_BYPASS_MASK           0x00000002                // SRCCLKENA_ACK_BYPASS[1]
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_ACK_BYPASS_SHFT           1
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_PROT_EN_ADDR              CONN_CFG_EMI_CTL_1_ADDR
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_PROT_EN_MASK              0x00000001                // SRCCLKENA_PROT_EN[0]
#define CONN_CFG_EMI_CTL_1_SRCCLKENA_PROT_EN_SHFT              0

/* =====================================================================================

  ---EMI_CTL_TOP (0x18011000 + 0x110)---

    EMI_REQ_TOP[0]               - (RW) TOP emi request
                                     1'b1: need use EMI
                                     1'b0: don't need use EMI, and disable emi
    SW_CONN_SRCCLKENA_TOP[1]     - (RW) software srcclkena control by top
    SW_CONN_AP_BUS_REQ_TOP[2]    - (RW) software ap_bus_req control by top
    SW_CONN_APSRC_REQ_TOP[3]     - (RW) software apsrc_req control by top
    SW_CONN_DDR_EN_TOP[4]        - (RW) software ddr_en control by top
    INFRA_REQ_TOP[5]             - (RW) TOP infra request
                                     1'b1: need use INFRA
                                     1'b0: don't need use INFRA, and disable INFRA
    EMI_CTL_RSV_TOP[15..6]       - (RW) reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_TOP_EMI_CTL_RSV_TOP_ADDR              CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_EMI_CTL_RSV_TOP_MASK              0x0000FFC0                // EMI_CTL_RSV_TOP[15..6]
#define CONN_CFG_EMI_CTL_TOP_EMI_CTL_RSV_TOP_SHFT              6
#define CONN_CFG_EMI_CTL_TOP_INFRA_REQ_TOP_ADDR                CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_INFRA_REQ_TOP_MASK                0x00000020                // INFRA_REQ_TOP[5]
#define CONN_CFG_EMI_CTL_TOP_INFRA_REQ_TOP_SHFT                5
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_DDR_EN_TOP_ADDR           CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_DDR_EN_TOP_MASK           0x00000010                // SW_CONN_DDR_EN_TOP[4]
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_DDR_EN_TOP_SHFT           4
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_APSRC_REQ_TOP_ADDR        CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_APSRC_REQ_TOP_MASK        0x00000008                // SW_CONN_APSRC_REQ_TOP[3]
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_APSRC_REQ_TOP_SHFT        3
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_AP_BUS_REQ_TOP_ADDR       CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_AP_BUS_REQ_TOP_MASK       0x00000004                // SW_CONN_AP_BUS_REQ_TOP[2]
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_AP_BUS_REQ_TOP_SHFT       2
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_SRCCLKENA_TOP_ADDR        CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_SRCCLKENA_TOP_MASK        0x00000002                // SW_CONN_SRCCLKENA_TOP[1]
#define CONN_CFG_EMI_CTL_TOP_SW_CONN_SRCCLKENA_TOP_SHFT        1
#define CONN_CFG_EMI_CTL_TOP_EMI_REQ_TOP_ADDR                  CONN_CFG_EMI_CTL_TOP_ADDR
#define CONN_CFG_EMI_CTL_TOP_EMI_REQ_TOP_MASK                  0x00000001                // EMI_REQ_TOP[0]
#define CONN_CFG_EMI_CTL_TOP_EMI_REQ_TOP_SHFT                  0

/* =====================================================================================

  ---EMI_CTL_WF (0x18011000 + 0x114)---

    EMI_REQ_WF[0]                - (RW) wf emi request
                                     1'b1: need use EMI
                                     1'b0: don't need use EMI, and disable emi
    SW_CONN_SRCCLKENA_WF[1]      - (RW) software srcclkena control by wf
    SW_CONN_AP_BUS_REQ_WF[2]     - (RW) software ap_bus_req control by wf
    SW_CONN_APSRC_REQ_WF[3]      - (RW) software apsrc_req control by wf
    SW_CONN_DDR_EN_WF[4]         - (RW) software ddr_en control by wf
    INFRA_REQ_WF[5]              - (RW) WF infra request
                                     1'b1: need use INFRA
                                     1'b0: don't need use INFRA, and disable INFRA
    EMI_CTL_RSV_WF[15..6]        - (RW) reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_WF_EMI_CTL_RSV_WF_ADDR                CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_EMI_CTL_RSV_WF_MASK                0x0000FFC0                // EMI_CTL_RSV_WF[15..6]
#define CONN_CFG_EMI_CTL_WF_EMI_CTL_RSV_WF_SHFT                6
#define CONN_CFG_EMI_CTL_WF_INFRA_REQ_WF_ADDR                  CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_INFRA_REQ_WF_MASK                  0x00000020                // INFRA_REQ_WF[5]
#define CONN_CFG_EMI_CTL_WF_INFRA_REQ_WF_SHFT                  5
#define CONN_CFG_EMI_CTL_WF_SW_CONN_DDR_EN_WF_ADDR             CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_SW_CONN_DDR_EN_WF_MASK             0x00000010                // SW_CONN_DDR_EN_WF[4]
#define CONN_CFG_EMI_CTL_WF_SW_CONN_DDR_EN_WF_SHFT             4
#define CONN_CFG_EMI_CTL_WF_SW_CONN_APSRC_REQ_WF_ADDR          CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_SW_CONN_APSRC_REQ_WF_MASK          0x00000008                // SW_CONN_APSRC_REQ_WF[3]
#define CONN_CFG_EMI_CTL_WF_SW_CONN_APSRC_REQ_WF_SHFT          3
#define CONN_CFG_EMI_CTL_WF_SW_CONN_AP_BUS_REQ_WF_ADDR         CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_SW_CONN_AP_BUS_REQ_WF_MASK         0x00000004                // SW_CONN_AP_BUS_REQ_WF[2]
#define CONN_CFG_EMI_CTL_WF_SW_CONN_AP_BUS_REQ_WF_SHFT         2
#define CONN_CFG_EMI_CTL_WF_SW_CONN_SRCCLKENA_WF_ADDR          CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_SW_CONN_SRCCLKENA_WF_MASK          0x00000002                // SW_CONN_SRCCLKENA_WF[1]
#define CONN_CFG_EMI_CTL_WF_SW_CONN_SRCCLKENA_WF_SHFT          1
#define CONN_CFG_EMI_CTL_WF_EMI_REQ_WF_ADDR                    CONN_CFG_EMI_CTL_WF_ADDR
#define CONN_CFG_EMI_CTL_WF_EMI_REQ_WF_MASK                    0x00000001                // EMI_REQ_WF[0]
#define CONN_CFG_EMI_CTL_WF_EMI_REQ_WF_SHFT                    0

/* =====================================================================================

  ---EMI_CTL_BT (0x18011000 + 0x118)---

    EMI_REQ_BT[0]                - (RW) bt emi request
                                     1'b1: need use EMI
                                     1'b0: don't need use EMI, and disable emi
    SW_CONN_SRCCLKENA_BT[1]      - (RW) software srcclkena control by bt
    SW_CONN_AP_BUS_REQ_BT[2]     - (RW) software ap_bus_req control by bt
    SW_CONN_APSRC_REQ_BT[3]      - (RW) software apsrc_req control by bt
    SW_CONN_DDR_EN_BT[4]         - (RW) software ddr_en control by bt
    INFRA_REQ_BT[5]              - (RW) BT infra request
                                     1'b1: need use INFRA
                                     1'b0: don't need use INFRA, and disable INFRA
    EMI_CTL_RSV_BT[15..6]        - (RW) reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_BT_EMI_CTL_RSV_BT_ADDR                CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_EMI_CTL_RSV_BT_MASK                0x0000FFC0                // EMI_CTL_RSV_BT[15..6]
#define CONN_CFG_EMI_CTL_BT_EMI_CTL_RSV_BT_SHFT                6
#define CONN_CFG_EMI_CTL_BT_INFRA_REQ_BT_ADDR                  CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_INFRA_REQ_BT_MASK                  0x00000020                // INFRA_REQ_BT[5]
#define CONN_CFG_EMI_CTL_BT_INFRA_REQ_BT_SHFT                  5
#define CONN_CFG_EMI_CTL_BT_SW_CONN_DDR_EN_BT_ADDR             CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_SW_CONN_DDR_EN_BT_MASK             0x00000010                // SW_CONN_DDR_EN_BT[4]
#define CONN_CFG_EMI_CTL_BT_SW_CONN_DDR_EN_BT_SHFT             4
#define CONN_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT_ADDR          CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT_MASK          0x00000008                // SW_CONN_APSRC_REQ_BT[3]
#define CONN_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT_SHFT          3
#define CONN_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT_ADDR         CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT_MASK         0x00000004                // SW_CONN_AP_BUS_REQ_BT[2]
#define CONN_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT_SHFT         2
#define CONN_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT_ADDR          CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT_MASK          0x00000002                // SW_CONN_SRCCLKENA_BT[1]
#define CONN_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT_SHFT          1
#define CONN_CFG_EMI_CTL_BT_EMI_REQ_BT_ADDR                    CONN_CFG_EMI_CTL_BT_ADDR
#define CONN_CFG_EMI_CTL_BT_EMI_REQ_BT_MASK                    0x00000001                // EMI_REQ_BT[0]
#define CONN_CFG_EMI_CTL_BT_EMI_REQ_BT_SHFT                    0

/* =====================================================================================

  ---EMI_CTL_GPS (0x18011000 + 0x11C)---

    EMI_REQ_GPS[0]               - (RW) gps emi request
                                     1'b1: need use EMI
                                     1'b0: don't need use EMI, and disable emi
    SW_CONN_SRCCLKENA_GPS[1]     - (RW) software srcclkena control by gps
    SW_CONN_AP_BUS_REQ_GPS[2]    - (RW) software ap_bus_req control by gps
    SW_CONN_APSRC_REQ_GPS[3]     - (RW) software apsrc_req control by gps
    SW_CONN_DDR_EN_GPS[4]        - (RW) software ddr_en control by gps
    INFRA_REQ_GPS[5]             - (RW) GPS infra request
                                     1'b1: need use INFRA
                                     1'b0: don't need use INFRA, and disable INFRA
    EMI_CTL_RSV_GPS[15..6]       - (RW) reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_GPS_EMI_CTL_RSV_GPS_ADDR              CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_EMI_CTL_RSV_GPS_MASK              0x0000FFC0                // EMI_CTL_RSV_GPS[15..6]
#define CONN_CFG_EMI_CTL_GPS_EMI_CTL_RSV_GPS_SHFT              6
#define CONN_CFG_EMI_CTL_GPS_INFRA_REQ_GPS_ADDR                CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_INFRA_REQ_GPS_MASK                0x00000020                // INFRA_REQ_GPS[5]
#define CONN_CFG_EMI_CTL_GPS_INFRA_REQ_GPS_SHFT                5
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_DDR_EN_GPS_ADDR           CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_DDR_EN_GPS_MASK           0x00000010                // SW_CONN_DDR_EN_GPS[4]
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_DDR_EN_GPS_SHFT           4
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_APSRC_REQ_GPS_ADDR        CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_APSRC_REQ_GPS_MASK        0x00000008                // SW_CONN_APSRC_REQ_GPS[3]
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_APSRC_REQ_GPS_SHFT        3
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_AP_BUS_REQ_GPS_ADDR       CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_AP_BUS_REQ_GPS_MASK       0x00000004                // SW_CONN_AP_BUS_REQ_GPS[2]
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_AP_BUS_REQ_GPS_SHFT       2
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS_ADDR        CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS_MASK        0x00000002                // SW_CONN_SRCCLKENA_GPS[1]
#define CONN_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS_SHFT        1
#define CONN_CFG_EMI_CTL_GPS_EMI_REQ_GPS_ADDR                  CONN_CFG_EMI_CTL_GPS_ADDR
#define CONN_CFG_EMI_CTL_GPS_EMI_REQ_GPS_MASK                  0x00000001                // EMI_REQ_GPS[0]
#define CONN_CFG_EMI_CTL_GPS_EMI_REQ_GPS_SHFT                  0

/* =====================================================================================

  ---EMI_CTL_GPS_L1 (0x18011000 + 0X120)---

    EMI_REQ_GPS_L1[0]            - (RW) GPS_L1 emi request
                                     1'b1: need use EMI
                                     1'b0: don't need use EMI, and disable emi
    SW_CONN_SRCCLKENA_GPS_L1[1]  - (RW) software srcclkena control by GPS_L1
    SW_CONN_AP_BUS_REQ_GPS_L1[2] - (RW) software ap_bus_req control by GPS_L1
    SW_CONN_APSRC_REQ_GPS_L1[3]  - (RW) software apsrc_req control by GPS_L1
    SW_CONN_DDR_EN_GPS_L1[4]     - (RW) software ddr_en control by GPS_L1
    INFRA_REQ_GPS_L1[5]          - (RW) GPS_L1 infra request
                                     1'b1: need use INFRA
                                     1'b0: don't need use INFRA, and disable INFRA
    EMI_CTL_RSV_GPS_L1[15..6]    - (RW) reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_GPS_L1_EMI_CTL_RSV_GPS_L1_ADDR        CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_EMI_CTL_RSV_GPS_L1_MASK        0x0000FFC0                // EMI_CTL_RSV_GPS_L1[15..6]
#define CONN_CFG_EMI_CTL_GPS_L1_EMI_CTL_RSV_GPS_L1_SHFT        6
#define CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1_ADDR          CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1_MASK          0x00000020                // INFRA_REQ_GPS_L1[5]
#define CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1_SHFT          5
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_DDR_EN_GPS_L1_ADDR     CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_DDR_EN_GPS_L1_MASK     0x00000010                // SW_CONN_DDR_EN_GPS_L1[4]
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_DDR_EN_GPS_L1_SHFT     4
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_APSRC_REQ_GPS_L1_ADDR  CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_APSRC_REQ_GPS_L1_MASK  0x00000008                // SW_CONN_APSRC_REQ_GPS_L1[3]
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_APSRC_REQ_GPS_L1_SHFT  3
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_AP_BUS_REQ_GPS_L1_ADDR CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_AP_BUS_REQ_GPS_L1_MASK 0x00000004                // SW_CONN_AP_BUS_REQ_GPS_L1[2]
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_AP_BUS_REQ_GPS_L1_SHFT 2
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_SRCCLKENA_GPS_L1_ADDR  CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_SRCCLKENA_GPS_L1_MASK  0x00000002                // SW_CONN_SRCCLKENA_GPS_L1[1]
#define CONN_CFG_EMI_CTL_GPS_L1_SW_CONN_SRCCLKENA_GPS_L1_SHFT  1
#define CONN_CFG_EMI_CTL_GPS_L1_EMI_REQ_GPS_L1_ADDR            CONN_CFG_EMI_CTL_GPS_L1_ADDR
#define CONN_CFG_EMI_CTL_GPS_L1_EMI_REQ_GPS_L1_MASK            0x00000001                // EMI_REQ_GPS_L1[0]
#define CONN_CFG_EMI_CTL_GPS_L1_EMI_REQ_GPS_L1_SHFT            0

/* =====================================================================================

  ---EMI_CTL_GPS_L5 (0x18011000 + 0X124)---

    EMI_REQ_GPS_L5[0]            - (RW) GPS_L5 emi request
                                     1'b1: need use EMI
                                     1'b0: don't need use EMI, and disable emi
    SW_CONN_SRCCLKENA_GPS_L5[1]  - (RW) software srcclkena control by GPS_L5
    SW_CONN_AP_BUS_REQ_GPS_L5[2] - (RW) software ap_bus_req control by GPS_L5
    SW_CONN_APSRC_REQ_GPS_L5[3]  - (RW) software apsrc_req control by GPS_L5
    SW_CONN_DDR_EN_GPS_L5[4]     - (RW) software ddr_en control by GPS_L5
    INFRA_REQ_GPS_L5[5]          - (RW) GPS_L5 infra request
                                     1'b1: need use INFRA
                                     1'b0: don't need use INFRA, and disable INFRA
    EMI_CTL_RSV_GPS_L5[15..6]    - (RW) reserved CR
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CFG_EMI_CTL_GPS_L5_EMI_CTL_RSV_GPS_L5_ADDR        CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_EMI_CTL_RSV_GPS_L5_MASK        0x0000FFC0                // EMI_CTL_RSV_GPS_L5[15..6]
#define CONN_CFG_EMI_CTL_GPS_L5_EMI_CTL_RSV_GPS_L5_SHFT        6
#define CONN_CFG_EMI_CTL_GPS_L5_INFRA_REQ_GPS_L5_ADDR          CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_INFRA_REQ_GPS_L5_MASK          0x00000020                // INFRA_REQ_GPS_L5[5]
#define CONN_CFG_EMI_CTL_GPS_L5_INFRA_REQ_GPS_L5_SHFT          5
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_DDR_EN_GPS_L5_ADDR     CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_DDR_EN_GPS_L5_MASK     0x00000010                // SW_CONN_DDR_EN_GPS_L5[4]
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_DDR_EN_GPS_L5_SHFT     4
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_APSRC_REQ_GPS_L5_ADDR  CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_APSRC_REQ_GPS_L5_MASK  0x00000008                // SW_CONN_APSRC_REQ_GPS_L5[3]
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_APSRC_REQ_GPS_L5_SHFT  3
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_AP_BUS_REQ_GPS_L5_ADDR CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_AP_BUS_REQ_GPS_L5_MASK 0x00000004                // SW_CONN_AP_BUS_REQ_GPS_L5[2]
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_AP_BUS_REQ_GPS_L5_SHFT 2
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_SRCCLKENA_GPS_L5_ADDR  CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_SRCCLKENA_GPS_L5_MASK  0x00000002                // SW_CONN_SRCCLKENA_GPS_L5[1]
#define CONN_CFG_EMI_CTL_GPS_L5_SW_CONN_SRCCLKENA_GPS_L5_SHFT  1
#define CONN_CFG_EMI_CTL_GPS_L5_EMI_REQ_GPS_L5_ADDR            CONN_CFG_EMI_CTL_GPS_L5_ADDR
#define CONN_CFG_EMI_CTL_GPS_L5_EMI_REQ_GPS_L5_MASK            0x00000001                // EMI_REQ_GPS_L5[0]
#define CONN_CFG_EMI_CTL_GPS_L5_EMI_REQ_GPS_L5_SHFT            0

/* =====================================================================================

  ---EMI_PROBE (0x18011000 + 0x130)---

    CONN_SRCCCLKENA[0]           - (RO)  xxx 
    CONN_AP_BUS[1]               - (RO)  xxx 
    CONN_APSRC[2]                - (RO)  xxx 
    CONN_DDR_EN[3]               - (RO)  xxx 
    CONN_SRCCCLKENA_ACK[4]       - (RO)  xxx 
    CONN_AP_BUS_ACK[5]           - (RO)  xxx 
    CONN_APSRC_ACK[6]            - (RO)  xxx 
    CONN_DDR_EN_ACK[7]           - (RO)  xxx 
    SRCCLKENA_ACK_SYNC[8]        - (RO)  xxx 
    AP_BUS_ACK_SYNC[9]           - (RO)  xxx 
    APSRC_ACK_SYNC[10]           - (RO)  xxx 
    DDR_EN_ACK_SYNC[11]          - (RO)  xxx 
    CONN2AP_BUS_IDLE[12]         - (RO)  xxx 
    CONN_OFF2ON_REQ[13]          - (RO)  xxx 
    CONN2AP_BUS_SLPPROT_EN[14]   - (RO)  xxx 
    CONN2AP_BUS_EMI_SLPPROT_EN[15] - (RO)  xxx 
    SRCCLKENA_STABLE_REG[16]     - (RO)  xxx 
    APSRC_STABLE_REG[17]         - (RO)  xxx 
    AP_BUS_STABLE_REG[18]        - (RO)  xxx 
    DDR_EN_STABLE_REG[19]        - (RO)  xxx 
    SRCCLKENA_STABLE[20]         - (RO)  xxx 
    APSRC_STABLE[21]             - (RO)  xxx 
    AP_BUS_STABLE[22]            - (RO)  xxx 
    DDR_EN_STABLE[23]            - (RO)  xxx 
    EMI_REQ_ALL[24]              - (RO)  xxx 
    EMI_REQ_DIS[25]              - (RO)  xxx 
    DDR_TIMEOUT[26]              - (RO)  xxx 
    CONN2AP_EMI_ONLY_REQ[27]     - (RO)  xxx 
    HW_CONN_SRCCLKENA_EMI_DIS[28] - (RO)  xxx 
    HW_CONN_AP_BUS_REQ_DIS[29]   - (RO)  xxx 
    HW_CONN_APSRC_REQ_DIS[30]    - (RO)  xxx 
    HW_CONN_DDR_EN_DIS[31]       - (RO)  xxx 

 =====================================================================================*/
#define CONN_CFG_EMI_PROBE_HW_CONN_DDR_EN_DIS_ADDR             CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_HW_CONN_DDR_EN_DIS_MASK             0x80000000                // HW_CONN_DDR_EN_DIS[31]
#define CONN_CFG_EMI_PROBE_HW_CONN_DDR_EN_DIS_SHFT             31
#define CONN_CFG_EMI_PROBE_HW_CONN_APSRC_REQ_DIS_ADDR          CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_HW_CONN_APSRC_REQ_DIS_MASK          0x40000000                // HW_CONN_APSRC_REQ_DIS[30]
#define CONN_CFG_EMI_PROBE_HW_CONN_APSRC_REQ_DIS_SHFT          30
#define CONN_CFG_EMI_PROBE_HW_CONN_AP_BUS_REQ_DIS_ADDR         CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_HW_CONN_AP_BUS_REQ_DIS_MASK         0x20000000                // HW_CONN_AP_BUS_REQ_DIS[29]
#define CONN_CFG_EMI_PROBE_HW_CONN_AP_BUS_REQ_DIS_SHFT         29
#define CONN_CFG_EMI_PROBE_HW_CONN_SRCCLKENA_EMI_DIS_ADDR      CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_HW_CONN_SRCCLKENA_EMI_DIS_MASK      0x10000000                // HW_CONN_SRCCLKENA_EMI_DIS[28]
#define CONN_CFG_EMI_PROBE_HW_CONN_SRCCLKENA_EMI_DIS_SHFT      28
#define CONN_CFG_EMI_PROBE_CONN2AP_EMI_ONLY_REQ_ADDR           CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN2AP_EMI_ONLY_REQ_MASK           0x08000000                // CONN2AP_EMI_ONLY_REQ[27]
#define CONN_CFG_EMI_PROBE_CONN2AP_EMI_ONLY_REQ_SHFT           27
#define CONN_CFG_EMI_PROBE_DDR_TIMEOUT_ADDR                    CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_DDR_TIMEOUT_MASK                    0x04000000                // DDR_TIMEOUT[26]
#define CONN_CFG_EMI_PROBE_DDR_TIMEOUT_SHFT                    26
#define CONN_CFG_EMI_PROBE_EMI_REQ_DIS_ADDR                    CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_EMI_REQ_DIS_MASK                    0x02000000                // EMI_REQ_DIS[25]
#define CONN_CFG_EMI_PROBE_EMI_REQ_DIS_SHFT                    25
#define CONN_CFG_EMI_PROBE_EMI_REQ_ALL_ADDR                    CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_EMI_REQ_ALL_MASK                    0x01000000                // EMI_REQ_ALL[24]
#define CONN_CFG_EMI_PROBE_EMI_REQ_ALL_SHFT                    24
#define CONN_CFG_EMI_PROBE_DDR_EN_STABLE_ADDR                  CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_DDR_EN_STABLE_MASK                  0x00800000                // DDR_EN_STABLE[23]
#define CONN_CFG_EMI_PROBE_DDR_EN_STABLE_SHFT                  23
#define CONN_CFG_EMI_PROBE_AP_BUS_STABLE_ADDR                  CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_AP_BUS_STABLE_MASK                  0x00400000                // AP_BUS_STABLE[22]
#define CONN_CFG_EMI_PROBE_AP_BUS_STABLE_SHFT                  22
#define CONN_CFG_EMI_PROBE_APSRC_STABLE_ADDR                   CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_APSRC_STABLE_MASK                   0x00200000                // APSRC_STABLE[21]
#define CONN_CFG_EMI_PROBE_APSRC_STABLE_SHFT                   21
#define CONN_CFG_EMI_PROBE_SRCCLKENA_STABLE_ADDR               CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_SRCCLKENA_STABLE_MASK               0x00100000                // SRCCLKENA_STABLE[20]
#define CONN_CFG_EMI_PROBE_SRCCLKENA_STABLE_SHFT               20
#define CONN_CFG_EMI_PROBE_DDR_EN_STABLE_REG_ADDR              CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_DDR_EN_STABLE_REG_MASK              0x00080000                // DDR_EN_STABLE_REG[19]
#define CONN_CFG_EMI_PROBE_DDR_EN_STABLE_REG_SHFT              19
#define CONN_CFG_EMI_PROBE_AP_BUS_STABLE_REG_ADDR              CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_AP_BUS_STABLE_REG_MASK              0x00040000                // AP_BUS_STABLE_REG[18]
#define CONN_CFG_EMI_PROBE_AP_BUS_STABLE_REG_SHFT              18
#define CONN_CFG_EMI_PROBE_APSRC_STABLE_REG_ADDR               CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_APSRC_STABLE_REG_MASK               0x00020000                // APSRC_STABLE_REG[17]
#define CONN_CFG_EMI_PROBE_APSRC_STABLE_REG_SHFT               17
#define CONN_CFG_EMI_PROBE_SRCCLKENA_STABLE_REG_ADDR           CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_SRCCLKENA_STABLE_REG_MASK           0x00010000                // SRCCLKENA_STABLE_REG[16]
#define CONN_CFG_EMI_PROBE_SRCCLKENA_STABLE_REG_SHFT           16
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_EMI_SLPPROT_EN_ADDR     CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_EMI_SLPPROT_EN_MASK     0x00008000                // CONN2AP_BUS_EMI_SLPPROT_EN[15]
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_EMI_SLPPROT_EN_SHFT     15
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_SLPPROT_EN_ADDR         CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_SLPPROT_EN_MASK         0x00004000                // CONN2AP_BUS_SLPPROT_EN[14]
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_SLPPROT_EN_SHFT         14
#define CONN_CFG_EMI_PROBE_CONN_OFF2ON_REQ_ADDR                CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_OFF2ON_REQ_MASK                0x00002000                // CONN_OFF2ON_REQ[13]
#define CONN_CFG_EMI_PROBE_CONN_OFF2ON_REQ_SHFT                13
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_IDLE_ADDR               CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_IDLE_MASK               0x00001000                // CONN2AP_BUS_IDLE[12]
#define CONN_CFG_EMI_PROBE_CONN2AP_BUS_IDLE_SHFT               12
#define CONN_CFG_EMI_PROBE_DDR_EN_ACK_SYNC_ADDR                CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_DDR_EN_ACK_SYNC_MASK                0x00000800                // DDR_EN_ACK_SYNC[11]
#define CONN_CFG_EMI_PROBE_DDR_EN_ACK_SYNC_SHFT                11
#define CONN_CFG_EMI_PROBE_APSRC_ACK_SYNC_ADDR                 CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_APSRC_ACK_SYNC_MASK                 0x00000400                // APSRC_ACK_SYNC[10]
#define CONN_CFG_EMI_PROBE_APSRC_ACK_SYNC_SHFT                 10
#define CONN_CFG_EMI_PROBE_AP_BUS_ACK_SYNC_ADDR                CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_AP_BUS_ACK_SYNC_MASK                0x00000200                // AP_BUS_ACK_SYNC[9]
#define CONN_CFG_EMI_PROBE_AP_BUS_ACK_SYNC_SHFT                9
#define CONN_CFG_EMI_PROBE_SRCCLKENA_ACK_SYNC_ADDR             CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_SRCCLKENA_ACK_SYNC_MASK             0x00000100                // SRCCLKENA_ACK_SYNC[8]
#define CONN_CFG_EMI_PROBE_SRCCLKENA_ACK_SYNC_SHFT             8
#define CONN_CFG_EMI_PROBE_CONN_DDR_EN_ACK_ADDR                CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_DDR_EN_ACK_MASK                0x00000080                // CONN_DDR_EN_ACK[7]
#define CONN_CFG_EMI_PROBE_CONN_DDR_EN_ACK_SHFT                7
#define CONN_CFG_EMI_PROBE_CONN_APSRC_ACK_ADDR                 CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_APSRC_ACK_MASK                 0x00000040                // CONN_APSRC_ACK[6]
#define CONN_CFG_EMI_PROBE_CONN_APSRC_ACK_SHFT                 6
#define CONN_CFG_EMI_PROBE_CONN_AP_BUS_ACK_ADDR                CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_AP_BUS_ACK_MASK                0x00000020                // CONN_AP_BUS_ACK[5]
#define CONN_CFG_EMI_PROBE_CONN_AP_BUS_ACK_SHFT                5
#define CONN_CFG_EMI_PROBE_CONN_SRCCCLKENA_ACK_ADDR            CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_SRCCCLKENA_ACK_MASK            0x00000010                // CONN_SRCCCLKENA_ACK[4]
#define CONN_CFG_EMI_PROBE_CONN_SRCCCLKENA_ACK_SHFT            4
#define CONN_CFG_EMI_PROBE_CONN_DDR_EN_ADDR                    CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_DDR_EN_MASK                    0x00000008                // CONN_DDR_EN[3]
#define CONN_CFG_EMI_PROBE_CONN_DDR_EN_SHFT                    3
#define CONN_CFG_EMI_PROBE_CONN_APSRC_ADDR                     CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_APSRC_MASK                     0x00000004                // CONN_APSRC[2]
#define CONN_CFG_EMI_PROBE_CONN_APSRC_SHFT                     2
#define CONN_CFG_EMI_PROBE_CONN_AP_BUS_ADDR                    CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_AP_BUS_MASK                    0x00000002                // CONN_AP_BUS[1]
#define CONN_CFG_EMI_PROBE_CONN_AP_BUS_SHFT                    1
#define CONN_CFG_EMI_PROBE_CONN_SRCCCLKENA_ADDR                CONN_CFG_EMI_PROBE_ADDR
#define CONN_CFG_EMI_PROBE_CONN_SRCCCLKENA_MASK                0x00000001                // CONN_SRCCCLKENA[0]
#define CONN_CFG_EMI_PROBE_CONN_SRCCCLKENA_SHFT                0

/* =====================================================================================

  ---EMI_PROBE_1 (0x18011000 + 0x134)---

    EMI_PROBE_1[31..0]           - (RO)  xxx 

 =====================================================================================*/
#define CONN_CFG_EMI_PROBE_1_EMI_PROBE_1_ADDR                  CONN_CFG_EMI_PROBE_1_ADDR
#define CONN_CFG_EMI_PROBE_1_EMI_PROBE_1_MASK                  0xFFFFFFFF                // EMI_PROBE_1[31..0]
#define CONN_CFG_EMI_PROBE_1_EMI_PROBE_1_SHFT                  0

#endif // __CONN_CFG_REGS_H__
