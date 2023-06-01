/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_clkgen_top.h
//[Revision time]   : Fri Apr  9 11:52:33 2021

#ifndef __CONN_CLKGEN_TOP_REGS_H__
#define __CONN_CLKGEN_TOP_REGS_H__

//****************************************************************************
//
//                     CONN_CLKGEN_TOP CR Definitions                     
//
//****************************************************************************

#define CONN_CLKGEN_TOP_BASE                                   (CONN_REG_CONN_INFRA_CLKGEN_TOP_ADDR)

#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_ADDR              (CONN_CLKGEN_TOP_BASE + 0x000) // 2000
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_ADDR              (CONN_CLKGEN_TOP_BASE + 0x004) // 2004
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_ADDR              (CONN_CLKGEN_TOP_BASE + 0x008) // 2008
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_ADDR              (CONN_CLKGEN_TOP_BASE + 0x00C) // 200C
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR                (CONN_CLKGEN_TOP_BASE + 0x020) // 2020
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_ADDR              (CONN_CLKGEN_TOP_BASE + 0x030) // 2030
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR              (CONN_CLKGEN_TOP_BASE + 0x038) // 2038
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_ADDR           (CONN_CLKGEN_TOP_BASE + 0x03C) // 203C
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR         (CONN_CLKGEN_TOP_BASE + 0x040) // 2040
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR         (CONN_CLKGEN_TOP_BASE + 0x044) // 2044
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR     (CONN_CLKGEN_TOP_BASE + 0x048) // 2048
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR     (CONN_CLKGEN_TOP_BASE + 0x04C) // 204C
#define CONN_CLKGEN_TOP_CKGEN_BUS_ADDR                         (CONN_CLKGEN_TOP_BASE + 0x050) // 2050
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_ADDR                 (CONN_CLKGEN_TOP_BASE + 0x0054) // 2054
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR                      (CONN_CLKGEN_TOP_BASE + 0x060) // 2060
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR                  (CONN_CLKGEN_TOP_BASE + 0x064) // 2064
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR                  (CONN_CLKGEN_TOP_BASE + 0x068) // 2068
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR                      (CONN_CLKGEN_TOP_BASE + 0x070) // 2070
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR                  (CONN_CLKGEN_TOP_BASE + 0x074) // 2074
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR                  (CONN_CLKGEN_TOP_BASE + 0x078) // 2078
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR               (CONN_CLKGEN_TOP_BASE + 0x080) // 2080
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR           (CONN_CLKGEN_TOP_BASE + 0x084) // 2084
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR           (CONN_CLKGEN_TOP_BASE + 0x088) // 2088
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR                  (CONN_CLKGEN_TOP_BASE + 0x090) // 2090
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_ADDR                  (CONN_CLKGEN_TOP_BASE + 0x094) // 2094




/* =====================================================================================

  ---CKGEN_BUS_BPLL_DIV_1 (0x18012000 + 0x000)---

    BPLL_DIV_1_DIV_EN[0]         - (RW) conn_infra bus bpll clock divider_1's enable setting
    BPLL_DIV_1_DIV_CG_BYPASS[1]  - (RW) conn_infra bus bpll clock divider_1's gating bypass setting
    BPLL_DIV_1_SEL[7..2]         - (RW) conn_infra bus bpll clock divider_1 setting (bpll = 832/640Mhz)
                                     6'h0: div by 1
                                     6'h1: div by 1.5
                                     6'h2: div by 2
                                     6'h3: div by 2.5
                                     6'h4: div by 3
                                     . . .
                                     6'h3f: forbidden setting
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_SEL_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_SEL_MASK 0x000000FC                // BPLL_DIV_1_SEL[7..2]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_SEL_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_CG_BYPASS_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_CG_BYPASS_MASK 0x00000002                // BPLL_DIV_1_DIV_CG_BYPASS[1]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_CG_BYPASS_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_EN_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_EN_MASK 0x00000001                // BPLL_DIV_1_DIV_EN[0]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_EN_SHFT 0

/* =====================================================================================

  ---CKGEN_BUS_BPLL_DIV_2 (0x18012000 + 0x004)---

    BPLL_DIV_2_DIV_EN[0]         - (RW) conn_infra bus bpll clock divider_2's enable setting
    BPLL_DIV_2_DIV_CG_BYPASS[1]  - (RW) conn_infra bus bpll clock divider_2's gating bypass setting
    BPLL_DIV_2_SEL[7..2]         - (RW) conn_infra bus bpll clock divider_2 setting (bpll = 832/640Mhz)
                                     6'h0: div by 1
                                     6'h1: div by 1.5
                                     6'h2: div by 2
                                     6'h3: div by 2.5
                                     6'h4: div by 3
                                     . . .
                                     6'h3f: forbidden setting
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_SEL_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_SEL_MASK 0x000000FC                // BPLL_DIV_2_SEL[7..2]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_SEL_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_CG_BYPASS_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_CG_BYPASS_MASK 0x00000002                // BPLL_DIV_2_DIV_CG_BYPASS[1]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_CG_BYPASS_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_EN_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_EN_MASK 0x00000001                // BPLL_DIV_2_DIV_EN[0]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_EN_SHFT 0

/* =====================================================================================

  ---CKGEN_BUS_WPLL_DIV_1 (0x18012000 + 0x008)---

    WPLL_DIV_1_DIV_EN[0]         - (RW) conn_infra bus wpll clock divider_1's enable setting
    WPLL_DIV_1_DIV_CG_BYPASS[1]  - (RW) conn_infra bus wpll clock divider_1's gating bypass setting
    WPLL_DIV_1_SEL[7..2]         - (RW) conn_infra bus wpll clock divider_1 setting
                                     6'h0: (640/1   =640MHz)
                                     6'h1: (640/1.5   =426.66MHz)
                                     6'h2: div by 2    (640/2   =320MHz)
                                     6'h3: div by 2.5 (640/2.5=256MHz)
                                     6'h4: div by 3    (640/3   =213.33MHz)
                                     . . .
                                     6'h3f: forbidden setting
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_SEL_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_SEL_MASK 0x000000FC                // WPLL_DIV_1_SEL[7..2]
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_SEL_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_CG_BYPASS_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_CG_BYPASS_MASK 0x00000002                // WPLL_DIV_1_DIV_CG_BYPASS[1]
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_CG_BYPASS_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_EN_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_EN_MASK 0x00000001                // WPLL_DIV_1_DIV_EN[0]
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_EN_SHFT 0

/* =====================================================================================

  ---CKGEN_BUS_WPLL_DIV_2 (0x18012000 + 0x00C)---

    WPLL_DIV_2_DIV_EN[0]         - (RW) conn_infra bus wpll clock divider_2's enable setting
    WPLL_DIV_2_DIV_CG_BYPASS[1]  - (RW) conn_infra bus wpll clock divider_2's gating bypass setting
    WPLL_DIV_2_SEL[7..2]         - (RW) conn_infra bus wpll clock divider_2 setting
                                     6'h0: (640/1   =640MHz)
                                     6'h1: (640/1.5   =426.66MHz)
                                     6'h2: div by 2    (640/2   =320MHz)
                                     6'h3: div by 2.5 (640/2.5=256MHz)
                                     6'h4: div by 3    (640/3   =213.33MHz)
                                     . . .
                                     6'h3f: forbidden setting
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_SEL_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_SEL_MASK 0x000000FC                // WPLL_DIV_2_SEL[7..2]
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_SEL_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_CG_BYPASS_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_CG_BYPASS_MASK 0x00000002                // WPLL_DIV_2_DIV_CG_BYPASS[1]
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_CG_BYPASS_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_EN_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_EN_MASK 0x00000001                // WPLL_DIV_2_DIV_EN[0]
#define CONN_CLKGEN_TOP_CKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_EN_SHFT 0

/* =====================================================================================

  ---CKGEN_SRC_CK_CG_EN (0x18012000 + 0x020)---

    WFSYS_CKEN[0]                - (RW) conn_infra wfsys clock CG enable
    WFDAM_CKEN[1]                - (RW) conn_infra wfdma host_ck CG enable
    BPLL_CKEN[2]                 - (RW) conn_infra BPLL CG enable
    WPLL_CKEN[3]                 - (RW) conn_infra WPLL CG enable
    BTDMA_CKEN[4]                - (RW) conn_infra BTDMA CG enable
    WFSYS_FR_CKEN[5]             - (RW) conn_infra wfsys_icap clock CG enable
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFSYS_FR_CKEN_ADDR  CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFSYS_FR_CKEN_MASK  0x00000020                // WFSYS_FR_CKEN[5]
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFSYS_FR_CKEN_SHFT  5
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_BTDMA_CKEN_ADDR     CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_BTDMA_CKEN_MASK     0x00000010                // BTDMA_CKEN[4]
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_BTDMA_CKEN_SHFT     4
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WPLL_CKEN_ADDR      CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WPLL_CKEN_MASK      0x00000008                // WPLL_CKEN[3]
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WPLL_CKEN_SHFT      3
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_BPLL_CKEN_ADDR      CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_BPLL_CKEN_MASK      0x00000004                // BPLL_CKEN[2]
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_BPLL_CKEN_SHFT      2
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFDAM_CKEN_ADDR     CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFDAM_CKEN_MASK     0x00000002                // WFDAM_CKEN[1]
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFDAM_CKEN_SHFT     1
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFSYS_CKEN_ADDR     CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_ADDR
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFSYS_CKEN_MASK     0x00000001                // WFSYS_CKEN[0]
#define CONN_CLKGEN_TOP_CKGEN_SRC_CK_CG_EN_WFSYS_CKEN_SHFT     0

/* =====================================================================================

  ---CKGEN_RFSPI_WPLL_DIV (0x18012000 + 0x030)---

    WPLL_DIV_DIV_EN[0]           - (RW) conn_infra rfspi wpll clock divider_1's enable setting
    WPLL_DIV_DIV_CG_BYPASS[1]    - (RW) conn_infra rfspi wpll clock divider_1's gating bypass setting
    WPLL_DIV_SEL[7..2]           - (RW) conn_infra rfspi wpll clock divider setting (source clock wpll div 2 640/2=320Mhz)
                                     6'h0: div by 1
                                     6'h1: div by 1.5
                                     6'h2: div by 2
                                     6'h3: div by 2.5
                                     6'h4: div by 3
                                     . . .
                                     6'h3f: forbidden setting
    RESERVED8[31..8]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_SEL_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_SEL_MASK 0x000000FC                // WPLL_DIV_SEL[7..2]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_SEL_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_DIV_CG_BYPASS_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_DIV_CG_BYPASS_MASK 0x00000002                // WPLL_DIV_DIV_CG_BYPASS[1]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_DIV_CG_BYPASS_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_DIV_EN_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_DIV_EN_MASK 0x00000001                // WPLL_DIV_DIV_EN[0]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_DIV_EN_SHFT 0

/* =====================================================================================

  ---CLKGEN_RFSPI_CK_CTRL (0x18012000 + 0x038)---

    CK_SEL_SW_MODE_EN[0]         - (RW) 1'b1: enable sw mode rfspi clock selection
                                     1'b0: disable sw mode rfspi clock selection
    SW_CK_SEL[2..1]              - (RW) 2'b00: wpll program div (320Mhz Source, default div 5)
                                     2'b01: wpll program div (320Mhz Source, default div 5)
                                     2'b10: bpll fix div 13 (Source bpll 832Mhz)
                                     2'b11: bpll fix div 10 (Source bpll 640Mhz)
    OSC_DIV2_EN[3]               - (RW) 1'b1 : enable osc div2 divider
                                     1'b0 :disable osc div2 divider
    BPLL_DIV13_EN[4]             - (RW) 1'b1 : enable bpll div13 divider
                                     1'b0 :disable bpll div13 divider
    BPLL_DIV10_EN[5]             - (RW) 1'b1 : enable bpll div10 divider
                                     1'b0 :disable bpll div10 divider
    UCLK_PLL_SEL[6]              - (RW) 1'b1: uclk pll source will be bpll
                                     1'b0: uclk pll source will be wpll
    UCLK_BPLL_SEL[7]             - (RW) 1'b1: uclk bpll source will be bpll div 10
                                     1'b0: uclk bpll source will be bpll div 13
    RESERVED8[15..8]             - (RO) Reserved bits
    BPLL_REQ_DEBOUNCE[20..16]    - (RW)  xxx 
    RESERVED21[23..21]           - (RO) Reserved bits
    WPLL_REQ_DEBOUNCE[28..24]    - (RW)  xxx 
    RESERVED29[31..29]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_WPLL_REQ_DEBOUNCE_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_WPLL_REQ_DEBOUNCE_MASK 0x1F000000                // WPLL_REQ_DEBOUNCE[28..24]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_WPLL_REQ_DEBOUNCE_SHFT 24
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_REQ_DEBOUNCE_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_REQ_DEBOUNCE_MASK 0x001F0000                // BPLL_REQ_DEBOUNCE[20..16]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_REQ_DEBOUNCE_SHFT 16
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_BPLL_SEL_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_BPLL_SEL_MASK 0x00000080                // UCLK_BPLL_SEL[7]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_BPLL_SEL_SHFT 7
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_PLL_SEL_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_PLL_SEL_MASK 0x00000040                // UCLK_PLL_SEL[6]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_PLL_SEL_SHFT 6
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV10_EN_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV10_EN_MASK 0x00000020                // BPLL_DIV10_EN[5]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV10_EN_SHFT 5
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV13_EN_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV13_EN_MASK 0x00000010                // BPLL_DIV13_EN[4]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV13_EN_SHFT 4
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_OSC_DIV2_EN_ADDR  CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_OSC_DIV2_EN_MASK  0x00000008                // OSC_DIV2_EN[3]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_OSC_DIV2_EN_SHFT  3
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_SW_CK_SEL_ADDR    CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_SW_CK_SEL_MASK    0x00000006                // SW_CK_SEL[2..1]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_SW_CK_SEL_SHFT    1
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_CK_SEL_SW_MODE_EN_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_CK_SEL_SW_MODE_EN_MASK 0x00000001                // CK_SEL_SW_MODE_EN[0]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_CK_CTRL_CK_SEL_SW_MODE_EN_SHFT 0

/* =====================================================================================

  ---CLKGEN_RFSPI_MUX_STATUS (0x18012000 + 0x03C)---

    UCLK_MUX_SWITCH_RDY[3..0]    - (RO)  xxx 
    OSC_MUX_SWITCH_RDY[5..4]     - (RO)  xxx 
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_OSC_MUX_SWITCH_RDY_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_OSC_MUX_SWITCH_RDY_MASK 0x00000030                // OSC_MUX_SWITCH_RDY[5..4]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_OSC_MUX_SWITCH_RDY_SHFT 4
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_UCLK_MUX_SWITCH_RDY_ADDR CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_UCLK_MUX_SWITCH_RDY_MASK 0x0000000F                // UCLK_MUX_SWITCH_RDY[3..0]
#define CONN_CLKGEN_TOP_CLKGEN_RFSPI_MUX_STATUS_UCLK_MUX_SWITCH_RDY_SHFT 0

/* =====================================================================================

  ---CLKGEN_BUS_PLL_REQ_CTRL_0 (0x18012000 + 0x040)---

    WPLL_REQ_DEBOUNCE[4..0]      - (RW)  xxx 
    BPLL_REQ_DEBOUNCE[9..5]      - (RW)  xxx 
    WPLL_REQ[10]                 - (RO)  xxx 
    BPLL_REQ[11]                 - (RO)  xxx 
    WF_PLL_REQ[12]               - (RO)  xxx 
    BT_PLL_REQ[13]               - (RO)  xxx 
    GPS_PLL_REQ[14]              - (RO)  xxx 
    LEGACY_WF_PLL_REQ[15]        - (RO)  xxx 
    LEGACY_BT_PLL_REQ[16]        - (RO)  xxx 
    LEGACY_GPS_PLL_REQ[17]       - (RO)  xxx 
    RESERVED18[31..18]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_GPS_PLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_GPS_PLL_REQ_MASK 0x00020000                // LEGACY_GPS_PLL_REQ[17]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_GPS_PLL_REQ_SHFT 17
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_BT_PLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_BT_PLL_REQ_MASK 0x00010000                // LEGACY_BT_PLL_REQ[16]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_BT_PLL_REQ_SHFT 16
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_WF_PLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_WF_PLL_REQ_MASK 0x00008000                // LEGACY_WF_PLL_REQ[15]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_LEGACY_WF_PLL_REQ_SHFT 15
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_GPS_PLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_GPS_PLL_REQ_MASK 0x00004000                // GPS_PLL_REQ[14]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_GPS_PLL_REQ_SHFT 14
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BT_PLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BT_PLL_REQ_MASK 0x00002000                // BT_PLL_REQ[13]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BT_PLL_REQ_SHFT 13
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WF_PLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WF_PLL_REQ_MASK 0x00001000                // WF_PLL_REQ[12]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WF_PLL_REQ_SHFT 12
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BPLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BPLL_REQ_MASK 0x00000800                // BPLL_REQ[11]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BPLL_REQ_SHFT 11
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WPLL_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WPLL_REQ_MASK 0x00000400                // WPLL_REQ[10]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WPLL_REQ_SHFT 10
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BPLL_REQ_DEBOUNCE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BPLL_REQ_DEBOUNCE_MASK 0x000003E0                // BPLL_REQ_DEBOUNCE[9..5]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_BPLL_REQ_DEBOUNCE_SHFT 5
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WPLL_REQ_DEBOUNCE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WPLL_REQ_DEBOUNCE_MASK 0x0000001F                // WPLL_REQ_DEBOUNCE[4..0]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_0_WPLL_REQ_DEBOUNCE_SHFT 0

/* =====================================================================================

  ---CLKGEN_BUS_PLL_REQ_CTRL_1 (0x18012000 + 0x044)---

    WPLL_SW_REQ[0]               - (RW) wpll sw mode request
                                     1'b0: un-request
                                     1'b1 : request
    WPLL_SW_MODE[1]              - (RW) mode setting of wpll request
                                     1'b0: hw mode
                                     1'b1: sw mode
    BPLL_SW_REQ[2]               - (RW) bpll sw mode request
                                     1'b0: un-request
                                     1'b1 : request
    BPLL_SW_MODE[3]              - (RW) mode setting of mode bpll request
                                     1'b0: hw mode
                                     1'b1: sw mode
    WF_PLL_REQ_SW_MODE[4]        - (RW) mode setting of conn_infra bus speed up to pll by wf request
                                     1'b0: hw mode
                                     1'b1: sw mode
    WF_PLL_SW_REQ[5]             - (RW) conn_infra bus speed up to pll by wf request sw mode
                                     1'b0: un-request
                                     1'b1 : request
    BT_PLL_REQ_SW_MODE[6]        - (RW) mode setting of conn_infra bus speed up to pll by bt request
                                     1'b0: hw mode
                                     1'b1: sw mode
    BT_PLL_SW_REQ[7]             - (RW) conn_infra bus speed up to pll by bt request sw mode
                                     1'b0: un-request
                                     1'b1 : request
    GPS_PLL_REQ_SW_MODE[8]       - (RW) mode setting of conn_infra bus speed up to pll by gps request
                                     1'b0: hw mode
                                     1'b1: sw mode
    GPS_PLL_SW_REQ[9]            - (RW) conn_infra bus speed up to pll by gps request sw mode
                                     1'b0: un-request
                                     1'b1 : request
    WF_PLL_REQ_LEGACY_MODE[10]   - (RW) mode setting of conn_infra bus speed up source from wf
                                     1'b0: normal mode (from subsys trigger)
                                     1'b1: legacy mode (use trigger which is subsy for afe)
    BT_PLL_REQ_LEGACY_MODE[11]   - (RW) mode setting of conn_infra bus speed up source from bt
                                     1'b0: normal mode (from subsys trigger)
                                     1'b1: legacy mode (use trigger which is subsy for afe)
    GPS_PLL_REQ_LEGACY_MODE[12]  - (RW) mode setting of conn_infra bus speed up source from gps
                                     1'b0: normal mode (from subsys trigger)
                                     1'b1: legacy mode (use trigger which is subsy for afe)
    RESERVED13[31..13]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_REQ_LEGACY_MODE_MASK 0x00001000                // GPS_PLL_REQ_LEGACY_MODE[12]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_REQ_LEGACY_MODE_SHFT 12
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_REQ_LEGACY_MODE_MASK 0x00000800                // BT_PLL_REQ_LEGACY_MODE[11]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_REQ_LEGACY_MODE_SHFT 11
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_REQ_LEGACY_MODE_MASK 0x00000400                // WF_PLL_REQ_LEGACY_MODE[10]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_REQ_LEGACY_MODE_SHFT 10
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_SW_REQ_MASK 0x00000200                // GPS_PLL_SW_REQ[9]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_SW_REQ_SHFT 9
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_REQ_SW_MODE_MASK 0x00000100                // GPS_PLL_REQ_SW_MODE[8]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_GPS_PLL_REQ_SW_MODE_SHFT 8
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_SW_REQ_MASK 0x00000080                // BT_PLL_SW_REQ[7]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_SW_REQ_SHFT 7
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_REQ_SW_MODE_MASK 0x00000040                // BT_PLL_REQ_SW_MODE[6]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BT_PLL_REQ_SW_MODE_SHFT 6
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_SW_REQ_MASK 0x00000020                // WF_PLL_SW_REQ[5]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_SW_REQ_SHFT 5
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_REQ_SW_MODE_MASK 0x00000010                // WF_PLL_REQ_SW_MODE[4]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WF_PLL_REQ_SW_MODE_SHFT 4
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BPLL_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BPLL_SW_MODE_MASK 0x00000008                // BPLL_SW_MODE[3]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BPLL_SW_MODE_SHFT 3
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BPLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BPLL_SW_REQ_MASK 0x00000004                // BPLL_SW_REQ[2]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_BPLL_SW_REQ_SHFT 2
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WPLL_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WPLL_SW_MODE_MASK 0x00000002                // WPLL_SW_MODE[1]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WPLL_SW_MODE_SHFT 1
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WPLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WPLL_SW_REQ_MASK 0x00000001                // WPLL_SW_REQ[0]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_WPLL_SW_REQ_SHFT 0

/* =====================================================================================

  ---CLKGEN_BUS_PLL_REQ_CTRL_1_SET (0x18012000 + 0x048)---

    WPLL_SW_REQ[0]               - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[0]
    WPLL_SW_MODE[1]              - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[1]
    BPLL_SW_REQ[2]               - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[2]
    BPLL_SW_MODE[3]              - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[3]
    WF_PLL_REQ_SW_MODE[4]        - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[4]
    WF_PLL_SW_REQ[5]             - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[5]
    BT_PLL_REQ_SW_MODE[6]        - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[6]
    BT_PLL_SW_REQ[7]             - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[7]
    GPS_PLL_REQ_SW_MODE[8]       - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[8]
    GPS_PLL_SW_REQ[9]            - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[9]
    WF_PLL_REQ_LEGACY_MODE[10]   - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[10]
    BT_PLL_REQ_LEGACY_MODE[11]   - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[11]
    GPS_PLL_REQ_LEGACY_MODE[12]  - (WO) write 1 to set CLKGEN_BUS_PLL_REQ_CTRL_1[12]
    RESERVED13[31..13]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_REQ_LEGACY_MODE_MASK 0x00001000                // GPS_PLL_REQ_LEGACY_MODE[12]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_REQ_LEGACY_MODE_SHFT 12
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_REQ_LEGACY_MODE_MASK 0x00000800                // BT_PLL_REQ_LEGACY_MODE[11]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_REQ_LEGACY_MODE_SHFT 11
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_REQ_LEGACY_MODE_MASK 0x00000400                // WF_PLL_REQ_LEGACY_MODE[10]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_REQ_LEGACY_MODE_SHFT 10
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_SW_REQ_MASK 0x00000200                // GPS_PLL_SW_REQ[9]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_SW_REQ_SHFT 9
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_REQ_SW_MODE_MASK 0x00000100                // GPS_PLL_REQ_SW_MODE[8]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_GPS_PLL_REQ_SW_MODE_SHFT 8
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_SW_REQ_MASK 0x00000080                // BT_PLL_SW_REQ[7]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_SW_REQ_SHFT 7
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_REQ_SW_MODE_MASK 0x00000040                // BT_PLL_REQ_SW_MODE[6]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BT_PLL_REQ_SW_MODE_SHFT 6
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_SW_REQ_MASK 0x00000020                // WF_PLL_SW_REQ[5]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_SW_REQ_SHFT 5
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_REQ_SW_MODE_MASK 0x00000010                // WF_PLL_REQ_SW_MODE[4]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WF_PLL_REQ_SW_MODE_SHFT 4
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BPLL_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BPLL_SW_MODE_MASK 0x00000008                // BPLL_SW_MODE[3]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BPLL_SW_MODE_SHFT 3
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BPLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BPLL_SW_REQ_MASK 0x00000004                // BPLL_SW_REQ[2]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_BPLL_SW_REQ_SHFT 2
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WPLL_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WPLL_SW_MODE_MASK 0x00000002                // WPLL_SW_MODE[1]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WPLL_SW_MODE_SHFT 1
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WPLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WPLL_SW_REQ_MASK 0x00000001                // WPLL_SW_REQ[0]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_SET_WPLL_SW_REQ_SHFT 0

/* =====================================================================================

  ---CLKGEN_BUS_PLL_REQ_CTRL_1_CLR (0x18012000 + 0x04C)---

    WPLL_SW_REQ[0]               - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[0]
    WPLL_SW_MODE[1]              - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[1]
    BPLL_SW_REQ[2]               - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[2]
    BPLL_SW_MODE[3]              - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[3]
    WF_PLL_REQ_SW_MODE[4]        - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[4]
    WF_PLL_SW_REQ[5]             - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[5]
    BT_PLL_REQ_SW_MODE[6]        - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[6]
    BT_PLL_SW_REQ[7]             - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[7]
    GPS_PLL_REQ_SW_MODE[8]       - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[8]
    GPS_PLL_SW_REQ[9]            - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[9]
    WF_PLL_REQ_LEGACY_MODE[10]   - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[10]
    BT_PLL_REQ_LEGACY_MODE[11]   - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[11]
    GPS_PLL_REQ_LEGACY_MODE[12]  - (WO) write 1 to clr CLKGEN_BUS_PLL_REQ_CTRL_1[12]
    RESERVED13[31..13]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_REQ_LEGACY_MODE_MASK 0x00001000                // GPS_PLL_REQ_LEGACY_MODE[12]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_REQ_LEGACY_MODE_SHFT 12
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_REQ_LEGACY_MODE_MASK 0x00000800                // BT_PLL_REQ_LEGACY_MODE[11]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_REQ_LEGACY_MODE_SHFT 11
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_REQ_LEGACY_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_REQ_LEGACY_MODE_MASK 0x00000400                // WF_PLL_REQ_LEGACY_MODE[10]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_REQ_LEGACY_MODE_SHFT 10
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_SW_REQ_MASK 0x00000200                // GPS_PLL_SW_REQ[9]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_SW_REQ_SHFT 9
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_REQ_SW_MODE_MASK 0x00000100                // GPS_PLL_REQ_SW_MODE[8]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_GPS_PLL_REQ_SW_MODE_SHFT 8
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_SW_REQ_MASK 0x00000080                // BT_PLL_SW_REQ[7]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_SW_REQ_SHFT 7
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_REQ_SW_MODE_MASK 0x00000040                // BT_PLL_REQ_SW_MODE[6]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BT_PLL_REQ_SW_MODE_SHFT 6
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_SW_REQ_MASK 0x00000020                // WF_PLL_SW_REQ[5]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_SW_REQ_SHFT 5
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_REQ_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_REQ_SW_MODE_MASK 0x00000010                // WF_PLL_REQ_SW_MODE[4]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WF_PLL_REQ_SW_MODE_SHFT 4
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BPLL_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BPLL_SW_MODE_MASK 0x00000008                // BPLL_SW_MODE[3]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BPLL_SW_MODE_SHFT 3
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BPLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BPLL_SW_REQ_MASK 0x00000004                // BPLL_SW_REQ[2]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_BPLL_SW_REQ_SHFT 2
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WPLL_SW_MODE_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WPLL_SW_MODE_MASK 0x00000002                // WPLL_SW_MODE[1]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WPLL_SW_MODE_SHFT 1
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WPLL_SW_REQ_ADDR CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WPLL_SW_REQ_MASK 0x00000001                // WPLL_SW_REQ[0]
#define CONN_CLKGEN_TOP_CLKGEN_BUS_PLL_REQ_CTRL_1_CLR_WPLL_SW_REQ_SHFT 0

/* =====================================================================================

  ---CKGEN_BUS (0x18012000 + 0x050)---

    HCLK_CKSEL_SWCTL[0]          - (RW) conn_infra BUS clock software control
                                     1'h0: control by hardware
                                     1'h1: control by software (default)
    HCLK_CKSEL_SW_MODE_UPDATE_EN[1] - (RW) 1'b1: hclk cksel update trigger sw mode
                                     1'b0: hclk cksel update trigger hw mode
    HCLK_CKSEL_SW_MODE_UPDATE[2] - (RW) write 0 after write 1 to trigger cksel update pulse
    FORCE_HCLK_CKSEL_VLD[3]      - (RW) Enable Wait clk sel stable feature
    HCLK_CKSEL[6..4]             - (RW) when conn_infra BUS clock software control=1
                                     conn_infra BUS clock selection
                                     3'h0: OSC clock
                                     3'h1: 32KHz clock
                                     3'h2: bgfsys BUS clock
                                     3'h3: wfsys BUS clock
    RESERVED7[7]                 - (RO) Reserved bits
    BUS_CK_SOURCE_SW_EN[8]       - (RW) conn_infra_osc_switch_en
    RESERVED9[11..9]             - (RO) Reserved bits
    HCLK_CK_MUX_RDY[19..12]      - (RO) HCLK clock MUX result
                                     8'b10000000: WPLL640_DIV3 is selected
                                     8'b01000000: WPLL640_PROG_DIV2 is selected
                                     8'b00100000: WPLL640_PROG_DIV2 is selected
                                     8'b00010000: WPLL640_PROG_DIV1 is selected
                                     8'b00001000: BPLL_PROG_DIV2 is selected
                                     8'b00000100: BPLL_PROG_DIV1 is selected
                                     8'b00000010: F32k is selected
                                     8'b00000001: OSC_1X_2X_CK is selected
    RESERVED20[31..20]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CK_MUX_RDY_ADDR         CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CK_MUX_RDY_MASK         0x000FF000                // HCLK_CK_MUX_RDY[19..12]
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CK_MUX_RDY_SHFT         12
#define CONN_CLKGEN_TOP_CKGEN_BUS_BUS_CK_SOURCE_SW_EN_ADDR     CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_BUS_CK_SOURCE_SW_EN_MASK     0x00000100                // BUS_CK_SOURCE_SW_EN[8]
#define CONN_CLKGEN_TOP_CKGEN_BUS_BUS_CK_SOURCE_SW_EN_SHFT     8
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_ADDR              CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_MASK              0x00000070                // HCLK_CKSEL[6..4]
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SHFT              4
#define CONN_CLKGEN_TOP_CKGEN_BUS_FORCE_HCLK_CKSEL_VLD_ADDR    CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_FORCE_HCLK_CKSEL_VLD_MASK    0x00000008                // FORCE_HCLK_CKSEL_VLD[3]
#define CONN_CLKGEN_TOP_CKGEN_BUS_FORCE_HCLK_CKSEL_VLD_SHFT    3
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SW_MODE_UPDATE_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SW_MODE_UPDATE_MASK 0x00000004                // HCLK_CKSEL_SW_MODE_UPDATE[2]
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SW_MODE_UPDATE_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SW_MODE_UPDATE_EN_ADDR CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SW_MODE_UPDATE_EN_MASK 0x00000002                // HCLK_CKSEL_SW_MODE_UPDATE_EN[1]
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SW_MODE_UPDATE_EN_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SWCTL_ADDR        CONN_CLKGEN_TOP_CKGEN_BUS_ADDR
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SWCTL_MASK        0x00000001                // HCLK_CKSEL_SWCTL[0]
#define CONN_CLKGEN_TOP_CKGEN_BUS_HCLK_CKSEL_SWCTL_SHFT        0

/* =====================================================================================

  ---CKGEN_OSC_MUX_SEL (0x18012000 + 0x0054)---

    OSC_MUX_SEL[0]               - (RW) OSC 1X/2X clock source select
                                     1'h0: OSC_CK
                                     1'h1: OSC_2X_CK
    OSC_MUX_RDY[2..1]            - (RO) OSC 1X/2X clock MUX result
                                     2'b01: OSC_CK is selected
                                     2'b10: OSC_2X_CK is selected
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_OSC_MUX_RDY_ADDR     CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_ADDR
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_OSC_MUX_RDY_MASK     0x00000006                // OSC_MUX_RDY[2..1]
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_OSC_MUX_RDY_SHFT     1
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_OSC_MUX_SEL_ADDR     CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_ADDR
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_OSC_MUX_SEL_MASK     0x00000001                // OSC_MUX_SEL[0]
#define CONN_CLKGEN_TOP_CKGEN_OSC_MUX_SEL_OSC_MUX_SEL_SHFT     0

/* =====================================================================================

  ---CKGEN_COEX_0 (0x18012000 + 0x060)---

    CONN_CO_EXT_UART_PTA_OSC_CKEN_M0[0] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M1[1] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M2[2] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M3[3] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M4[4] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M5[5] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M6[6] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M7[7] - (RW) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0[8] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1[9] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2[10] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3[11] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4[12] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5[13] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6[14] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7[15] - (RW) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M0[16] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M1[17] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M2[18] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M3[19] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M4[20] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M5[21] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M6[22] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M7[23] - (RW) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M0[24] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M1[25] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M2[26] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M3[27] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M4[28] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M5[29] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M6[30] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M7[31] - (RW) conn_co_ext_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M7_MASK 0x80000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M7[31]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M7_SHFT 31
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M6_MASK 0x40000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M6[30]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M6_SHFT 30
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M5_MASK 0x20000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M5[29]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M5_SHFT 29
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M4_MASK 0x10000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M4[28]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M4_SHFT 28
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M3_MASK 0x08000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M3[27]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M3_SHFT 27
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M2_MASK 0x04000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M2[26]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M2_SHFT 26
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M1_MASK 0x02000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M1[25]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M1_SHFT 25
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M0_MASK 0x01000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M0[24]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_HCLK_CKEN_M0_SHFT 24
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M7_MASK 0x00800000                // CONN_CO_EXT_PTA_OSC_CKEN_M7[23]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M7_SHFT 23
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M6_MASK 0x00400000                // CONN_CO_EXT_PTA_OSC_CKEN_M6[22]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M6_SHFT 22
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M5_MASK 0x00200000                // CONN_CO_EXT_PTA_OSC_CKEN_M5[21]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M5_SHFT 21
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M4_MASK 0x00100000                // CONN_CO_EXT_PTA_OSC_CKEN_M4[20]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M4_SHFT 20
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M3_MASK 0x00080000                // CONN_CO_EXT_PTA_OSC_CKEN_M3[19]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M3_SHFT 19
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M2_MASK 0x00040000                // CONN_CO_EXT_PTA_OSC_CKEN_M2[18]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M2_SHFT 18
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M1_MASK 0x00020000                // CONN_CO_EXT_PTA_OSC_CKEN_M1[17]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M1_SHFT 17
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M0_MASK 0x00010000                // CONN_CO_EXT_PTA_OSC_CKEN_M0[16]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_PTA_OSC_CKEN_M0_SHFT 16
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_MASK 0x00008000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7[15]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_SHFT 15
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_MASK 0x00004000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6[14]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_SHFT 14
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_MASK 0x00002000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5[13]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_SHFT 13
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_MASK 0x00001000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4[12]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_SHFT 12
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_MASK 0x00000800                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3[11]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_SHFT 11
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_MASK 0x00000400                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2[10]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_SHFT 10
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_MASK 0x00000200                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1[9]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_SHFT 9
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_MASK 0x00000100                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0[8]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_SHFT 8
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_MASK 0x00000080                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_MASK 0x00000040                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_MASK 0x00000020                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_MASK 0x00000010                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_MASK 0x00000008                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_MASK 0x00000004                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_MASK 0x00000002                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_MASK 0x00000001                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_COEX_0_SET (0x18012000 + 0x064)---

    CONN_CO_EXT_UART_PTA_OSC_CKEN_M0[0] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M1[1] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M2[2] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M3[3] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M4[4] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M5[5] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M6[6] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M7[7] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0[8] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1[9] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2[10] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3[11] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4[12] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5[13] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6[14] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7[15] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M0[16] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M1[17] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M2[18] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M3[19] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M4[20] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M5[21] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M6[22] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M7[23] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M0[24] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M1[25] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M2[26] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M3[27] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M4[28] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M5[29] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M6[30] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M7[31] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M7_MASK 0x80000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M7[31]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M7_SHFT 31
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M6_MASK 0x40000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M6[30]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M6_SHFT 30
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M5_MASK 0x20000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M5[29]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M5_SHFT 29
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M4_MASK 0x10000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M4[28]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M4_SHFT 28
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M3_MASK 0x08000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M3[27]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M3_SHFT 27
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M2_MASK 0x04000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M2[26]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M2_SHFT 26
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M1_MASK 0x02000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M1[25]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M1_SHFT 25
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M0_MASK 0x01000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M0[24]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_HCLK_CKEN_M0_SHFT 24
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M7_MASK 0x00800000                // CONN_CO_EXT_PTA_OSC_CKEN_M7[23]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M7_SHFT 23
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M6_MASK 0x00400000                // CONN_CO_EXT_PTA_OSC_CKEN_M6[22]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M6_SHFT 22
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M5_MASK 0x00200000                // CONN_CO_EXT_PTA_OSC_CKEN_M5[21]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M5_SHFT 21
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M4_MASK 0x00100000                // CONN_CO_EXT_PTA_OSC_CKEN_M4[20]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M4_SHFT 20
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M3_MASK 0x00080000                // CONN_CO_EXT_PTA_OSC_CKEN_M3[19]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M3_SHFT 19
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M2_MASK 0x00040000                // CONN_CO_EXT_PTA_OSC_CKEN_M2[18]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M2_SHFT 18
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M1_MASK 0x00020000                // CONN_CO_EXT_PTA_OSC_CKEN_M1[17]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M1_SHFT 17
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M0_MASK 0x00010000                // CONN_CO_EXT_PTA_OSC_CKEN_M0[16]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_PTA_OSC_CKEN_M0_SHFT 16
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_MASK 0x00008000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7[15]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_SHFT 15
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_MASK 0x00004000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6[14]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_SHFT 14
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_MASK 0x00002000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5[13]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_SHFT 13
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_MASK 0x00001000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4[12]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_SHFT 12
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_MASK 0x00000800                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3[11]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_SHFT 11
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_MASK 0x00000400                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2[10]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_SHFT 10
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_MASK 0x00000200                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1[9]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_SHFT 9
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_MASK 0x00000100                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0[8]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_SHFT 8
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_MASK 0x00000080                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_MASK 0x00000040                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_MASK 0x00000020                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_MASK 0x00000010                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_MASK 0x00000008                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_MASK 0x00000004                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_MASK 0x00000002                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_MASK 0x00000001                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_SET_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_COEX_0_CLR (0x18012000 + 0x068)---

    CONN_CO_EXT_UART_PTA_OSC_CKEN_M0[0] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M1[1] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M2[2] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M3[3] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M4[4] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M5[5] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M6[6] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_OSC_CKEN_M7[7] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0[8] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1[9] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2[10] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3[11] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4[12] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5[13] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6[14] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7[15] - (WO) conn_co_ext_uart_pta_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M0[16] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M1[17] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M2[18] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M3[19] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M4[20] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M5[21] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M6[22] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_OSC_CKEN_M7[23] - (WO) conn_co_ext_pta_osc_ck clock enable
                                     1'h0: dkeep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M0[24] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M1[25] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M2[26] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M3[27] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M4[28] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M5[29] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M6[30] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_PTA_HCLK_CKEN_M7[31] - (WO) conn_co_ext_uart_pta_osc_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M7_MASK 0x80000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M7[31]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M7_SHFT 31
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M6_MASK 0x40000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M6[30]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M6_SHFT 30
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M5_MASK 0x20000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M5[29]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M5_SHFT 29
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M4_MASK 0x10000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M4[28]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M4_SHFT 28
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M3_MASK 0x08000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M3[27]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M3_SHFT 27
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M2_MASK 0x04000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M2[26]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M2_SHFT 26
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M1_MASK 0x02000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M1[25]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M1_SHFT 25
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M0_MASK 0x01000000                // CONN_CO_EXT_PTA_HCLK_CKEN_M0[24]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_HCLK_CKEN_M0_SHFT 24
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M7_MASK 0x00800000                // CONN_CO_EXT_PTA_OSC_CKEN_M7[23]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M7_SHFT 23
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M6_MASK 0x00400000                // CONN_CO_EXT_PTA_OSC_CKEN_M6[22]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M6_SHFT 22
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M5_MASK 0x00200000                // CONN_CO_EXT_PTA_OSC_CKEN_M5[21]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M5_SHFT 21
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M4_MASK 0x00100000                // CONN_CO_EXT_PTA_OSC_CKEN_M4[20]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M4_SHFT 20
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M3_MASK 0x00080000                // CONN_CO_EXT_PTA_OSC_CKEN_M3[19]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M3_SHFT 19
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M2_MASK 0x00040000                // CONN_CO_EXT_PTA_OSC_CKEN_M2[18]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M2_SHFT 18
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M1_MASK 0x00020000                // CONN_CO_EXT_PTA_OSC_CKEN_M1[17]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M1_SHFT 17
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M0_MASK 0x00010000                // CONN_CO_EXT_PTA_OSC_CKEN_M0[16]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_PTA_OSC_CKEN_M0_SHFT 16
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_MASK 0x00008000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7[15]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M7_SHFT 15
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_MASK 0x00004000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6[14]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M6_SHFT 14
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_MASK 0x00002000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5[13]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M5_SHFT 13
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_MASK 0x00001000                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4[12]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M4_SHFT 12
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_MASK 0x00000800                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3[11]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M3_SHFT 11
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_MASK 0x00000400                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2[10]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M2_SHFT 10
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_MASK 0x00000200                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1[9]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M1_SHFT 9
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_MASK 0x00000100                // CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0[8]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_HCLK_CKEN_M0_SHFT 8
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_MASK 0x00000080                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_MASK 0x00000040                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_MASK 0x00000020                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_MASK 0x00000010                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_MASK 0x00000008                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_MASK 0x00000004                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_MASK 0x00000002                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_MASK 0x00000001                // CONN_CO_EXT_UART_PTA_OSC_CKEN_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_COEX_0_CLR_CONN_CO_EXT_UART_PTA_OSC_CKEN_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_COEX_1 (0x18012000 + 0x070)---

    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0[0] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1[1] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2[2] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3[3] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4[4] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5[5] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6[6] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7[7] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M0[8] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M1[9] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M2[10] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M3[11] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M4[12] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M5[13] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M6[14] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M7[15] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M7_MASK 0x00008000                // CONN_BSI_CNS_HCLK_CKEN_M7[15]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M7_SHFT 15
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M6_MASK 0x00004000                // CONN_BSI_CNS_HCLK_CKEN_M6[14]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M6_SHFT 14
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M5_MASK 0x00002000                // CONN_BSI_CNS_HCLK_CKEN_M5[13]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M5_SHFT 13
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M4_MASK 0x00001000                // CONN_BSI_CNS_HCLK_CKEN_M4[12]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M4_SHFT 12
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M3_MASK 0x00000800                // CONN_BSI_CNS_HCLK_CKEN_M3[11]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M3_SHFT 11
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M2_MASK 0x00000400                // CONN_BSI_CNS_HCLK_CKEN_M2[10]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M2_SHFT 10
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M1_MASK 0x00000200                // CONN_BSI_CNS_HCLK_CKEN_M1[9]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M1_SHFT 9
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M0_MASK 0x00000100                // CONN_BSI_CNS_HCLK_CKEN_M0[8]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_BSI_CNS_HCLK_CKEN_M0_SHFT 8
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_MASK 0x00000080                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_MASK 0x00000040                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_MASK 0x00000020                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_MASK 0x00000010                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_MASK 0x00000008                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_MASK 0x00000004                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_MASK 0x00000002                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_MASK 0x00000001                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_COEX_1_SET (0x18012000 + 0x074)---

    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0[0] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1[1] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2[2] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3[3] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4[4] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5[5] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6[6] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7[7] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M0[8] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M1[9] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M2[10] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M3[11] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M4[12] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M5[13] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M6[14] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M7[15] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M7_MASK 0x00008000                // CONN_BSI_CNS_HCLK_CKEN_M7[15]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M7_SHFT 15
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M6_MASK 0x00004000                // CONN_BSI_CNS_HCLK_CKEN_M6[14]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M6_SHFT 14
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M5_MASK 0x00002000                // CONN_BSI_CNS_HCLK_CKEN_M5[13]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M5_SHFT 13
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M4_MASK 0x00001000                // CONN_BSI_CNS_HCLK_CKEN_M4[12]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M4_SHFT 12
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M3_MASK 0x00000800                // CONN_BSI_CNS_HCLK_CKEN_M3[11]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M3_SHFT 11
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M2_MASK 0x00000400                // CONN_BSI_CNS_HCLK_CKEN_M2[10]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M2_SHFT 10
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M1_MASK 0x00000200                // CONN_BSI_CNS_HCLK_CKEN_M1[9]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M1_SHFT 9
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M0_MASK 0x00000100                // CONN_BSI_CNS_HCLK_CKEN_M0[8]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_BSI_CNS_HCLK_CKEN_M0_SHFT 8
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_MASK 0x00000080                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_MASK 0x00000040                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_MASK 0x00000020                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_MASK 0x00000010                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_MASK 0x00000008                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_MASK 0x00000004                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_MASK 0x00000002                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_MASK 0x00000001                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_SET_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_COEX_1_CLR (0x18012000 + 0x078)---

    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0[0] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1[1] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2[2] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3[3] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4[4] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5[5] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6[6] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7[7] - (RW) conn_co_ext_fdd_coex_hclk_ck clock enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M0[8] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M1[9] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M2[10] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M3[11] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M4[12] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M5[13] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M6[14] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    CONN_BSI_CNS_HCLK_CKEN_M7[15] - (RW) co-antenna original source hclk clock  enable
                                     1'h0: keep value
                                     1'h1: disable
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M7_MASK 0x00008000                // CONN_BSI_CNS_HCLK_CKEN_M7[15]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M7_SHFT 15
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M6_MASK 0x00004000                // CONN_BSI_CNS_HCLK_CKEN_M6[14]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M6_SHFT 14
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M5_MASK 0x00002000                // CONN_BSI_CNS_HCLK_CKEN_M5[13]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M5_SHFT 13
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M4_MASK 0x00001000                // CONN_BSI_CNS_HCLK_CKEN_M4[12]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M4_SHFT 12
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M3_MASK 0x00000800                // CONN_BSI_CNS_HCLK_CKEN_M3[11]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M3_SHFT 11
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M2_MASK 0x00000400                // CONN_BSI_CNS_HCLK_CKEN_M2[10]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M2_SHFT 10
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M1_MASK 0x00000200                // CONN_BSI_CNS_HCLK_CKEN_M1[9]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M1_SHFT 9
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M0_MASK 0x00000100                // CONN_BSI_CNS_HCLK_CKEN_M0[8]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_BSI_CNS_HCLK_CKEN_M0_SHFT 8
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_MASK 0x00000080                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_MASK 0x00000040                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_MASK 0x00000020                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_MASK 0x00000010                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_MASK 0x00000008                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_MASK 0x00000004                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_MASK 0x00000002                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_ADDR CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_MASK 0x00000001                // CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_COEX_1_CLR_CONN_CO_EXT_FDD_COEX_HCLKCKEN_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_RFSPI_PLL_REQ (0x18012000 + 0x080)---

    CONN_RFSPI_BPLL_REQ_M0[0]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M1[1]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M2[2]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M3[3]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M4[4]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M5[5]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M6[6]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M7[7]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    RESERVED8[15..8]             - (RO) Reserved bits
    CONN_RFSPI_WPLL_REQ_M0[16]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M1[17]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M2[18]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M3[19]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M4[20]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M5[21]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M6[22]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M7[23]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M7_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M7_MASK 0x00800000                // CONN_RFSPI_WPLL_REQ_M7[23]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M7_SHFT 23
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M6_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M6_MASK 0x00400000                // CONN_RFSPI_WPLL_REQ_M6[22]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M6_SHFT 22
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M5_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M5_MASK 0x00200000                // CONN_RFSPI_WPLL_REQ_M5[21]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M5_SHFT 21
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M4_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M4_MASK 0x00100000                // CONN_RFSPI_WPLL_REQ_M4[20]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M4_SHFT 20
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M3_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M3_MASK 0x00080000                // CONN_RFSPI_WPLL_REQ_M3[19]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M3_SHFT 19
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M2_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M2_MASK 0x00040000                // CONN_RFSPI_WPLL_REQ_M2[18]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M2_SHFT 18
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M1_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M1_MASK 0x00020000                // CONN_RFSPI_WPLL_REQ_M1[17]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M1_SHFT 17
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M0_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M0_MASK 0x00010000                // CONN_RFSPI_WPLL_REQ_M0[16]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_WPLL_REQ_M0_SHFT 16
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M7_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M7_MASK 0x00000080                // CONN_RFSPI_BPLL_REQ_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M6_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M6_MASK 0x00000040                // CONN_RFSPI_BPLL_REQ_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M5_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M5_MASK 0x00000020                // CONN_RFSPI_BPLL_REQ_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M4_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M4_MASK 0x00000010                // CONN_RFSPI_BPLL_REQ_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M3_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M3_MASK 0x00000008                // CONN_RFSPI_BPLL_REQ_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M2_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M2_MASK 0x00000004                // CONN_RFSPI_BPLL_REQ_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M1_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M1_MASK 0x00000002                // CONN_RFSPI_BPLL_REQ_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M0_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M0_MASK 0x00000001                // CONN_RFSPI_BPLL_REQ_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CONN_RFSPI_BPLL_REQ_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_RFSPI_PLL_REQ_SET (0x18012000 + 0x084)---

    CONN_RFSPI_BPLL_REQ_M0[0]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M1[1]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M2[2]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M3[3]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M4[4]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M5[5]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M6[6]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M7[7]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    RESERVED8[15..8]             - (RO) Reserved bits
    CONN_RFSPI_WPLL_REQ_M0[16]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M1[17]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M2[18]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M3[19]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M4[20]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M5[21]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M6[22]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M7[23]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M7_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M7_MASK 0x00800000                // CONN_RFSPI_WPLL_REQ_M7[23]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M7_SHFT 23
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M6_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M6_MASK 0x00400000                // CONN_RFSPI_WPLL_REQ_M6[22]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M6_SHFT 22
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M5_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M5_MASK 0x00200000                // CONN_RFSPI_WPLL_REQ_M5[21]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M5_SHFT 21
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M4_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M4_MASK 0x00100000                // CONN_RFSPI_WPLL_REQ_M4[20]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M4_SHFT 20
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M3_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M3_MASK 0x00080000                // CONN_RFSPI_WPLL_REQ_M3[19]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M3_SHFT 19
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M2_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M2_MASK 0x00040000                // CONN_RFSPI_WPLL_REQ_M2[18]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M2_SHFT 18
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M1_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M1_MASK 0x00020000                // CONN_RFSPI_WPLL_REQ_M1[17]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M1_SHFT 17
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M0_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M0_MASK 0x00010000                // CONN_RFSPI_WPLL_REQ_M0[16]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_WPLL_REQ_M0_SHFT 16
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M7_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M7_MASK 0x00000080                // CONN_RFSPI_BPLL_REQ_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M6_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M6_MASK 0x00000040                // CONN_RFSPI_BPLL_REQ_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M5_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M5_MASK 0x00000020                // CONN_RFSPI_BPLL_REQ_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M4_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M4_MASK 0x00000010                // CONN_RFSPI_BPLL_REQ_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M3_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M3_MASK 0x00000008                // CONN_RFSPI_BPLL_REQ_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M2_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M2_MASK 0x00000004                // CONN_RFSPI_BPLL_REQ_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M1_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M1_MASK 0x00000002                // CONN_RFSPI_BPLL_REQ_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M0_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M0_MASK 0x00000001                // CONN_RFSPI_BPLL_REQ_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_RFSPI_PLL_REQ_CLR (0x18012000 + 0x088)---

    CONN_RFSPI_BPLL_REQ_M0[0]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M1[1]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M2[2]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M3[3]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M4[4]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M5[5]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M6[6]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    CONN_RFSPI_BPLL_REQ_M7[7]    - (WO) conn_rfspi_bpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet bpll
    RESERVED8[15..8]             - (RO) Reserved bits
    CONN_RFSPI_WPLL_REQ_M0[16]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M1[17]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M2[18]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M3[19]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M4[20]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M5[21]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M6[22]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    CONN_RFSPI_WPLL_REQ_M7[23]   - (WO) conn_rfspi_wpll_req
                                     1'b0: keep value
                                     1'b1: disable reuqaet wpll
    RESERVED24[31..24]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M7_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M7_MASK 0x00800000                // CONN_RFSPI_WPLL_REQ_M7[23]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M7_SHFT 23
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M6_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M6_MASK 0x00400000                // CONN_RFSPI_WPLL_REQ_M6[22]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M6_SHFT 22
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M5_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M5_MASK 0x00200000                // CONN_RFSPI_WPLL_REQ_M5[21]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M5_SHFT 21
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M4_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M4_MASK 0x00100000                // CONN_RFSPI_WPLL_REQ_M4[20]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M4_SHFT 20
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M3_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M3_MASK 0x00080000                // CONN_RFSPI_WPLL_REQ_M3[19]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M3_SHFT 19
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M2_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M2_MASK 0x00040000                // CONN_RFSPI_WPLL_REQ_M2[18]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M2_SHFT 18
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M1_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M1_MASK 0x00020000                // CONN_RFSPI_WPLL_REQ_M1[17]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M1_SHFT 17
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M0_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M0_MASK 0x00010000                // CONN_RFSPI_WPLL_REQ_M0[16]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_WPLL_REQ_M0_SHFT 16
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M7_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M7_MASK 0x00000080                // CONN_RFSPI_BPLL_REQ_M7[7]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M7_SHFT 7
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M6_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M6_MASK 0x00000040                // CONN_RFSPI_BPLL_REQ_M6[6]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M6_SHFT 6
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M5_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M5_MASK 0x00000020                // CONN_RFSPI_BPLL_REQ_M5[5]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M5_SHFT 5
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M4_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M4_MASK 0x00000010                // CONN_RFSPI_BPLL_REQ_M4[4]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M4_SHFT 4
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M3_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M3_MASK 0x00000008                // CONN_RFSPI_BPLL_REQ_M3[3]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M3_SHFT 3
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M2_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M2_MASK 0x00000004                // CONN_RFSPI_BPLL_REQ_M2[2]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M2_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M1_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M1_MASK 0x00000002                // CONN_RFSPI_BPLL_REQ_M1[1]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M1_SHFT 1
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M0_ADDR CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_ADDR
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M0_MASK 0x00000001                // CONN_RFSPI_BPLL_REQ_M0[0]
#define CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M0_SHFT 0

/* =====================================================================================

  ---CKGEN_FREQ_METER (0x18012000 + 0x090)---

    FREQ_METER_CAL_CK_MUX_CKSEL[1..0] - (RW) To select freq meter target clock:
                                     2'b00: osc_ck
                                     2'b01: hclk_ck
                                     2'b10: bpll_div_ck
                                     2'b11: wpll_div_ck
    FREQ_METER_CAL_CK_CKEN[2]    - (RW) clock gating of freq meter target clock
    RESERVED3[3]                 - (RO) Reserved bits
    DET_CNT_CYCLE[6..4]          - (RW) reference cycle count  by 32KHz, count range = 1 ~ 6
    DET_EN[7]                    - (RW) detection enable
                                     1'h0: disable
                                     1'h1: enable
    DET_CAL_COUNTER[23..8]       - (RO) calculation cycle count  by calculation clock
    DET_VLD[24]                  - (RO) calculation clock detection done indicator
                                     1'h0 = detection is not completed
                                     1'h1 = detection is completed
    RESERVED25[31..25]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_VLD_ADDR          CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_VLD_MASK          0x01000000                // DET_VLD[24]
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_VLD_SHFT          24
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_CAL_COUNTER_ADDR  CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_CAL_COUNTER_MASK  0x00FFFF00                // DET_CAL_COUNTER[23..8]
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_CAL_COUNTER_SHFT  8
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_EN_ADDR           CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_EN_MASK           0x00000080                // DET_EN[7]
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_EN_SHFT           7
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_CNT_CYCLE_ADDR    CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_CNT_CYCLE_MASK    0x00000070                // DET_CNT_CYCLE[6..4]
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_DET_CNT_CYCLE_SHFT    4
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_FREQ_METER_CAL_CK_CKEN_ADDR CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_FREQ_METER_CAL_CK_CKEN_MASK 0x00000004                // FREQ_METER_CAL_CK_CKEN[2]
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_FREQ_METER_CAL_CK_CKEN_SHFT 2
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_FREQ_METER_CAL_CK_MUX_CKSEL_ADDR CONN_CLKGEN_TOP_CKGEN_FREQ_METER_ADDR
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_FREQ_METER_CAL_CK_MUX_CKSEL_MASK 0x00000003                // FREQ_METER_CAL_CK_MUX_CKSEL[1..0]
#define CONN_CLKGEN_TOP_CKGEN_FREQ_METER_FREQ_METER_CAL_CK_MUX_CKSEL_SHFT 0

/* =====================================================================================

  ---CKGEN_CLK_DETECT (0x18012000 + 0x094)---

    CLK_DETECT_BUS_ENABLE[0]     - (RW) clk detect enable
                                     1'b0: disable
                                     1'b1: enable
    BPLL_CLK_DETECT[1]           - (RO) Connsys bpll_div clock detection status
                                     0 : bpll_div clock non-alive
                                     1 : bpll_div clock alive
                                     User could write 0 to clear detection status.
                                     Wrie 1 to enable clock detect and then read back to check clock status. It can check bpll_div clock alive or not.
    WPLL_CLK_DETECT[2]           - (RO) Connsys wpll_div clock detection status
                                     0 : wpll_div clock non-alive
                                     1 : wpll_div clock alive
                                     User could write 0 to clear detection status.
                                     Wrie 1 to enable clock detect and then read back to check clock status. It can check wpll_div clock alive or not.
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_WPLL_CLK_DETECT_ADDR  CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_ADDR
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_WPLL_CLK_DETECT_MASK  0x00000004                // WPLL_CLK_DETECT[2]
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_WPLL_CLK_DETECT_SHFT  2
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_BPLL_CLK_DETECT_ADDR  CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_ADDR
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_BPLL_CLK_DETECT_MASK  0x00000002                // BPLL_CLK_DETECT[1]
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_BPLL_CLK_DETECT_SHFT  1
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_CLK_DETECT_BUS_ENABLE_ADDR CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_ADDR
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_CLK_DETECT_BUS_ENABLE_MASK 0x00000001                // CLK_DETECT_BUS_ENABLE[0]
#define CONN_CLKGEN_TOP_CKGEN_CLK_DETECT_CLK_DETECT_BUS_ENABLE_SHFT 0

#endif // __CONN_CLKGEN_TOP_REGS_H__
