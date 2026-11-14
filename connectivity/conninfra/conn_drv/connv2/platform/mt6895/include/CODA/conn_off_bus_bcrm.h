/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_off_bus_bcrm.h
//[Revision time]   : Mon Aug 30 15:58:41 2021

#ifndef __CONN_OFF_BUS_BCRM_REGS_H__
#define __CONN_OFF_BUS_BCRM_REGS_H__

//****************************************************************************
//
//                     CONN_OFF_BUS_BCRM CR Definitions                     
//
//****************************************************************************

#define CONN_OFF_BUS_BCRM_BASE                                 (CONN_REG_CONN_INFRA_OFF_BUS_BCRM_ADDR)

#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x000) // F000
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x004) // F004
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x008) // F008
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x00c) // F00C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x010) // F010
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x014) // F014
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x018) // F018
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x01c) // F01C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x020) // F020
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x024) // F024
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x028) // F028
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x02c) // F02C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x030) // F030
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x034) // F034
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x038) // F038
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x03c) // F03C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x040) // F040
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x044) // F044
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x048) // F048
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x04c) // F04C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x050) // F050
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x054) // F054
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x058) // F058
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x05c) // F05C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x060) // F060
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x064) // F064
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x068) // F068
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c14_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x06c) // F06C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x070) // F070
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x074) // F074
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x078) // F078
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x07c) // F07C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x080) // F080
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_4_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x084) // F084
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_5_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x088) // F088
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_6_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x08c) // F08C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_7_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x090) // F090
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x094) // F094
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x098) // F098
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x09c) // F09C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0a0) // F0A0
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0a4) // F0A4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0a8) // F0A8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0ac) // F0AC
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0b0) // F0B0
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0b4) // F0B4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0b8) // F0B8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0bc) // F0BC
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0c0) // F0C0
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0c4) // F0C4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_4_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0c8) // F0C8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_5_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0cc) // F0CC
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_6_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0d0) // F0D0
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_7_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0d4) // F0D4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_8_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0d8) // F0D8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_9_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0dc) // F0DC
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_10_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0e0) // F0E0
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_11_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0e4) // F0E4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_12_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0e8) // F0E8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_13_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0ec) // F0EC
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_14_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0f0) // F0F0
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_15_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0f4) // F0F4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_16_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0f8) // F0F8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_17_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x0fc) // F0FC
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x100) // F100
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_1_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x104) // F104
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_2_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x108) // F108
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_3_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x10c) // F10C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_4_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x110) // F110
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_5_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x114) // F114
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_6_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x118) // F118
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_7_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x11c) // F11C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_8_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x120) // F120
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_9_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x124) // F124
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_10_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x128) // F128
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_11_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x12c) // F12C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_12_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x130) // F130
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_13_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x134) // F134
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_14_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x138) // F138
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_15_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x13c) // F13C
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_16_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x140) // F140
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_17_ADDR (CONN_OFF_BUS_BCRM_BASE + 0x144) // F144




/* =====================================================================================

  ---conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0 (0x1804F000 + 0x000)---

    conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__postwrite_dis[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__RG_FIFO_THRE[2..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__ahb_flush_thre[4..3] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__err_flag_en[5] - (RW)  xxx 
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__err_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__err_flag_en_MASK 0x00000020                // conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__err_flag_en[5]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__err_flag_en_SHFT 5
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__ahb_flush_thre_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__ahb_flush_thre_MASK 0x00000018                // conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__ahb_flush_thre[4..3]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__ahb_flush_thre_SHFT 3
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__RG_FIFO_THRE_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__RG_FIFO_THRE_MASK 0x00000006                // conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__RG_FIFO_THRE[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__RG_FIFO_THRE_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__postwrite_dis_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__postwrite_dis_MASK 0x00000001                // conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__postwrite_dis[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_von2off_to_conn_infra_off_bus_m_c3_ASYNC__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0 (0x1804F000 + 0x004)---

    conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__error_flag_en[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_monitor_mode[2] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_monitor_mode_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_monitor_mode_MASK 0x00000004                // conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_monitor_mode[2]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_monitor_mode_SHFT 2
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_cmd_cnt_clr_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_cmd_cnt_clr_MASK 0x00000002                // conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__rg_cmd_cnt_clr_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_m1_to_conn_infra_off_bus_m_c0_AHB_S_PWR_PROT__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0 (0x1804F000 + 0x008)---

    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__error_flag_en[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_monitor_mode[2] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_monitor_mode_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_monitor_mode_MASK 0x00000004                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_monitor_mode[2]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_monitor_mode_SHFT 2
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_cmd_cnt_clr_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_cmd_cnt_clr_MASK 0x00000002                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__rg_cmd_cnt_clr_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_AHB_S_PWR_PROT__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0 (0x1804F000 + 0x00c)---

    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__postwrite_dis[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__RG_FIFO_THRE[2..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__ahb_flush_thre[4..3] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__err_flag_en[5] - (RW)  xxx 
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__err_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__err_flag_en_MASK 0x00000020                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__err_flag_en[5]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__err_flag_en_SHFT 5
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__ahb_flush_thre_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__ahb_flush_thre_MASK 0x00000018                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__ahb_flush_thre[4..3]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__ahb_flush_thre_SHFT 3
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__RG_FIFO_THRE_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__RG_FIFO_THRE_MASK 0x00000006                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__RG_FIFO_THRE[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__RG_FIFO_THRE_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__postwrite_dis_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__postwrite_dis_MASK 0x00000001                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__postwrite_dis[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_off2von_ASYNC__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0 (0x1804F000 + 0x010)---

    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__error_flag_en[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_monitor_mode[2] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_monitor_mode_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_monitor_mode_MASK 0x00000004                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_monitor_mode[2]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_monitor_mode_SHFT 2
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_cmd_cnt_clr_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_cmd_cnt_clr_MASK 0x00000002                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__rg_cmd_cnt_clr_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_s1_AHB_S_PWR_PROT__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0 (0x1804F000 + 0x014)---

    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__postwrite_dis[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__RG_FIFO_THRE[2..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__ahb_flush_thre[4..3] - (RW)  xxx 
    conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__err_flag_en[5] - (RW)  xxx 
    RESERVED6[31..6]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__err_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__err_flag_en_MASK 0x00000020                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__err_flag_en[5]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__err_flag_en_SHFT 5
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__ahb_flush_thre_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__ahb_flush_thre_MASK 0x00000018                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__ahb_flush_thre[4..3]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__ahb_flush_thre_SHFT 3
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__RG_FIFO_THRE_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__RG_FIFO_THRE_MASK 0x00000006                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__RG_FIFO_THRE[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__RG_FIFO_THRE_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__postwrite_dis_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__postwrite_dis_MASK 0x00000001                // conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__postwrite_dis[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC_CTRL_0_conn_infra_off_bus_m__u_lnk_conn_infra_off_bus_m_c6_to_conn_infra_off_bus_s_c8_ASYNC__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c3_CTRL_0 (0x1804F000 + 0x018)---

    conn_infra_off_bus_m__u_c3__ctrl_update_status[0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c3__reg_slave_way_en[2..1] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_conn_infra_off_bus_m__u_c3__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_conn_infra_off_bus_m__u_c3__reg_slave_way_en_MASK 0x00000006                // conn_infra_off_bus_m__u_c3__reg_slave_way_en[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_conn_infra_off_bus_m__u_c3__reg_slave_way_en_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_conn_infra_off_bus_m__u_c3__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_conn_infra_off_bus_m__u_c3__ctrl_update_status_MASK 0x00000001                // conn_infra_off_bus_m__u_c3__ctrl_update_status[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_0_conn_infra_off_bus_m__u_c3__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c3_CTRL_1 (0x1804F000 + 0x01c)---

    conn_infra_off_bus_m__u_c3__device_apc_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_1_conn_infra_off_bus_m__u_c3__device_apc_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_1_conn_infra_off_bus_m__u_c3__device_apc_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c3__device_apc_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_1_conn_infra_off_bus_m__u_c3__device_apc_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c3_CTRL_2 (0x1804F000 + 0x020)---

    conn_infra_off_bus_m__u_c3__device_apc_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_2_conn_infra_off_bus_m__u_c3__device_apc_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_2_conn_infra_off_bus_m__u_c3__device_apc_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c3__device_apc_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_2_conn_infra_off_bus_m__u_c3__device_apc_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c3_CTRL_3 (0x1804F000 + 0x024)---

    conn_infra_off_bus_m__u_c3__error_flag_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_3_conn_infra_off_bus_m__u_c3__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_3_conn_infra_off_bus_m__u_c3__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_c3__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c3_CTRL_3_conn_infra_off_bus_m__u_c3__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c4_CTRL_0 (0x1804F000 + 0x028)---

    conn_infra_off_bus_m__u_c4__ctrl_update_status[1..0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c4__reg_layer_way_en[6..2] - (RW)  xxx 
    conn_infra_off_bus_m__u_c4__reg_slave_way_en[7] - (RW)  xxx 
    conn_infra_off_bus_m__u_c4__reg_qos_en[8] - (RW)  xxx 
    conn_infra_off_bus_m__u_c4__error_flag_en[9] - (RW)  xxx 
    RESERVED10[31..10]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__error_flag_en_MASK 0x00000200                // conn_infra_off_bus_m__u_c4__error_flag_en[9]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__error_flag_en_SHFT 9
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_qos_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_qos_en_MASK 0x00000100                // conn_infra_off_bus_m__u_c4__reg_qos_en[8]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_qos_en_SHFT 8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_slave_way_en_MASK 0x00000080                // conn_infra_off_bus_m__u_c4__reg_slave_way_en[7]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_slave_way_en_SHFT 7
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_layer_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_layer_way_en_MASK 0x0000007C                // conn_infra_off_bus_m__u_c4__reg_layer_way_en[6..2]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__reg_layer_way_en_SHFT 2
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__ctrl_update_status_MASK 0x00000003                // conn_infra_off_bus_m__u_c4__ctrl_update_status[1..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c4_CTRL_0_conn_infra_off_bus_m__u_c4__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c12_CTRL_0 (0x1804F000 + 0x02c)---

    conn_infra_off_bus_m__u_c12__ctrl_update_status[1..0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c12__reg_layer_way_en[5..2] - (RW)  xxx 
    conn_infra_off_bus_m__u_c12__reg_slave_way_en[6] - (RW)  xxx 
    conn_infra_off_bus_m__u_c12__reg_qos_en[7] - (RW)  xxx 
    conn_infra_off_bus_m__u_c12__error_flag_en[8] - (RW)  xxx 
    RESERVED9[31..9]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__error_flag_en_MASK 0x00000100                // conn_infra_off_bus_m__u_c12__error_flag_en[8]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__error_flag_en_SHFT 8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_qos_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_qos_en_MASK 0x00000080                // conn_infra_off_bus_m__u_c12__reg_qos_en[7]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_qos_en_SHFT 7
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_slave_way_en_MASK 0x00000040                // conn_infra_off_bus_m__u_c12__reg_slave_way_en[6]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_slave_way_en_SHFT 6
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_layer_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_layer_way_en_MASK 0x0000003C                // conn_infra_off_bus_m__u_c12__reg_layer_way_en[5..2]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__reg_layer_way_en_SHFT 2
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__ctrl_update_status_MASK 0x00000003                // conn_infra_off_bus_m__u_c12__ctrl_update_status[1..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c12_CTRL_0_conn_infra_off_bus_m__u_c12__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c0_CTRL_0 (0x1804F000 + 0x030)---

    conn_infra_off_bus_m__u_c0__ctrl_update_status[0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c0__reg_slave_way_en[2..1] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_conn_infra_off_bus_m__u_c0__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_conn_infra_off_bus_m__u_c0__reg_slave_way_en_MASK 0x00000006                // conn_infra_off_bus_m__u_c0__reg_slave_way_en[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_conn_infra_off_bus_m__u_c0__reg_slave_way_en_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_conn_infra_off_bus_m__u_c0__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_conn_infra_off_bus_m__u_c0__ctrl_update_status_MASK 0x00000001                // conn_infra_off_bus_m__u_c0__ctrl_update_status[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_0_conn_infra_off_bus_m__u_c0__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c0_CTRL_1 (0x1804F000 + 0x034)---

    conn_infra_off_bus_m__u_c0__device_apc_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_1_conn_infra_off_bus_m__u_c0__device_apc_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_1_conn_infra_off_bus_m__u_c0__device_apc_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c0__device_apc_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_1_conn_infra_off_bus_m__u_c0__device_apc_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c0_CTRL_2 (0x1804F000 + 0x038)---

    conn_infra_off_bus_m__u_c0__device_apc_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_2_conn_infra_off_bus_m__u_c0__device_apc_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_2_conn_infra_off_bus_m__u_c0__device_apc_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c0__device_apc_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_2_conn_infra_off_bus_m__u_c0__device_apc_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c0_CTRL_3 (0x1804F000 + 0x03c)---

    conn_infra_off_bus_m__u_c0__error_flag_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_3_conn_infra_off_bus_m__u_c0__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_3_conn_infra_off_bus_m__u_c0__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_c0__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c0_CTRL_3_conn_infra_off_bus_m__u_c0__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_ct0_CTRL_0 (0x1804F000 + 0x040)---

    conn_infra_off_bus_m__u_ct0__postwrite_dis[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_ct0__ahb_flush_thre[2..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_ct0__err_flag_en[3] - (RW)  xxx 
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__err_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__err_flag_en_MASK 0x00000008                // conn_infra_off_bus_m__u_ct0__err_flag_en[3]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__err_flag_en_SHFT 3
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__ahb_flush_thre_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__ahb_flush_thre_MASK 0x00000006                // conn_infra_off_bus_m__u_ct0__ahb_flush_thre[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__ahb_flush_thre_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__postwrite_dis_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__postwrite_dis_MASK 0x00000001                // conn_infra_off_bus_m__u_ct0__postwrite_dis[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct0_CTRL_0_conn_infra_off_bus_m__u_ct0__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c1_CTRL_0 (0x1804F000 + 0x044)---

    conn_infra_off_bus_m__u_c1__ctrl_update_status[0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c1__reg_slave_way_en[2..1] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_conn_infra_off_bus_m__u_c1__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_conn_infra_off_bus_m__u_c1__reg_slave_way_en_MASK 0x00000006                // conn_infra_off_bus_m__u_c1__reg_slave_way_en[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_conn_infra_off_bus_m__u_c1__reg_slave_way_en_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_conn_infra_off_bus_m__u_c1__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_conn_infra_off_bus_m__u_c1__ctrl_update_status_MASK 0x00000001                // conn_infra_off_bus_m__u_c1__ctrl_update_status[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_0_conn_infra_off_bus_m__u_c1__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c1_CTRL_1 (0x1804F000 + 0x048)---

    conn_infra_off_bus_m__u_c1__device_apc_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_1_conn_infra_off_bus_m__u_c1__device_apc_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_1_conn_infra_off_bus_m__u_c1__device_apc_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c1__device_apc_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_1_conn_infra_off_bus_m__u_c1__device_apc_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c1_CTRL_2 (0x1804F000 + 0x04c)---

    conn_infra_off_bus_m__u_c1__device_apc_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_2_conn_infra_off_bus_m__u_c1__device_apc_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_2_conn_infra_off_bus_m__u_c1__device_apc_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c1__device_apc_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_2_conn_infra_off_bus_m__u_c1__device_apc_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c1_CTRL_3 (0x1804F000 + 0x050)---

    conn_infra_off_bus_m__u_c1__error_flag_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_3_conn_infra_off_bus_m__u_c1__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_3_conn_infra_off_bus_m__u_c1__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_c1__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c1_CTRL_3_conn_infra_off_bus_m__u_c1__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_ct1_CTRL_0 (0x1804F000 + 0x054)---

    conn_infra_off_bus_m__u_ct1__postwrite_dis[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_ct1__ahb_flush_thre[2..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_ct1__err_flag_en[3] - (RW)  xxx 
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__err_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__err_flag_en_MASK 0x00000008                // conn_infra_off_bus_m__u_ct1__err_flag_en[3]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__err_flag_en_SHFT 3
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__ahb_flush_thre_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__ahb_flush_thre_MASK 0x00000006                // conn_infra_off_bus_m__u_ct1__ahb_flush_thre[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__ahb_flush_thre_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__postwrite_dis_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__postwrite_dis_MASK 0x00000001                // conn_infra_off_bus_m__u_ct1__postwrite_dis[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct1_CTRL_0_conn_infra_off_bus_m__u_ct1__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c2_CTRL_0 (0x1804F000 + 0x058)---

    conn_infra_off_bus_m__u_c2__ctrl_update_status[0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c2__reg_slave_way_en[2..1] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_conn_infra_off_bus_m__u_c2__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_conn_infra_off_bus_m__u_c2__reg_slave_way_en_MASK 0x00000006                // conn_infra_off_bus_m__u_c2__reg_slave_way_en[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_conn_infra_off_bus_m__u_c2__reg_slave_way_en_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_conn_infra_off_bus_m__u_c2__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_conn_infra_off_bus_m__u_c2__ctrl_update_status_MASK 0x00000001                // conn_infra_off_bus_m__u_c2__ctrl_update_status[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_0_conn_infra_off_bus_m__u_c2__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c2_CTRL_1 (0x1804F000 + 0x05c)---

    conn_infra_off_bus_m__u_c2__device_apc_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_1_conn_infra_off_bus_m__u_c2__device_apc_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_1_conn_infra_off_bus_m__u_c2__device_apc_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c2__device_apc_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_1_conn_infra_off_bus_m__u_c2__device_apc_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c2_CTRL_2 (0x1804F000 + 0x060)---

    conn_infra_off_bus_m__u_c2__device_apc_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_2_conn_infra_off_bus_m__u_c2__device_apc_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_2_conn_infra_off_bus_m__u_c2__device_apc_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c2__device_apc_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_2_conn_infra_off_bus_m__u_c2__device_apc_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c2_CTRL_3 (0x1804F000 + 0x064)---

    conn_infra_off_bus_m__u_c2__error_flag_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_3_conn_infra_off_bus_m__u_c2__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_3_conn_infra_off_bus_m__u_c2__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_c2__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c2_CTRL_3_conn_infra_off_bus_m__u_c2__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c13_CTRL_0 (0x1804F000 + 0x068)---

    conn_infra_off_bus_m__u_c13__read_ahead_bw[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_c13__fifo_threshold[3..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_c13__merge_en[4] - (RW)  xxx 
    conn_infra_off_bus_m__u_c13__buffer_en[5] - (RW)  xxx 
    conn_infra_off_bus_m__u_c13__hbstrb_en[6] - (RW)  xxx 
    conn_infra_off_bus_m__u_c13__hsecur_en[7] - (RW) OPT
    conn_infra_off_bus_m__u_c13__hflush_thres[9..8] - (RW)  xxx 
    RESERVED10[31..10]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hflush_thres_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hflush_thres_MASK 0x00000300                // conn_infra_off_bus_m__u_c13__hflush_thres[9..8]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hflush_thres_SHFT 8
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hsecur_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hsecur_en_MASK 0x00000080                // conn_infra_off_bus_m__u_c13__hsecur_en[7]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hsecur_en_SHFT 7
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hbstrb_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hbstrb_en_MASK 0x00000040                // conn_infra_off_bus_m__u_c13__hbstrb_en[6]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__hbstrb_en_SHFT 6
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__buffer_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__buffer_en_MASK 0x00000020                // conn_infra_off_bus_m__u_c13__buffer_en[5]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__buffer_en_SHFT 5
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__merge_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__merge_en_MASK 0x00000010                // conn_infra_off_bus_m__u_c13__merge_en[4]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__merge_en_SHFT 4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__fifo_threshold_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__fifo_threshold_MASK 0x0000000E                // conn_infra_off_bus_m__u_c13__fifo_threshold[3..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__fifo_threshold_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__read_ahead_bw_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__read_ahead_bw_MASK 0x00000001                // conn_infra_off_bus_m__u_c13__read_ahead_bw[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c13_CTRL_0_conn_infra_off_bus_m__u_c13__read_ahead_bw_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c14_CTRL_0 (0x1804F000 + 0x06c)---

    conn_infra_off_bus_m__u_c14__no_dummy_read_en[0] - (RW) support no dummy read
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c14_CTRL_0_conn_infra_off_bus_m__u_c14__no_dummy_read_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c14_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c14_CTRL_0_conn_infra_off_bus_m__u_c14__no_dummy_read_en_MASK 0x00000001                // conn_infra_off_bus_m__u_c14__no_dummy_read_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c14_CTRL_0_conn_infra_off_bus_m__u_c14__no_dummy_read_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_ct2_CTRL_0 (0x1804F000 + 0x070)---

    conn_infra_off_bus_m__u_ct2__postwrite_dis[0] - (RW)  xxx 
    conn_infra_off_bus_m__u_ct2__ahb_flush_thre[2..1] - (RW)  xxx 
    conn_infra_off_bus_m__u_ct2__err_flag_en[3] - (RW)  xxx 
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__err_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__err_flag_en_MASK 0x00000008                // conn_infra_off_bus_m__u_ct2__err_flag_en[3]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__err_flag_en_SHFT 3
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__ahb_flush_thre_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__ahb_flush_thre_MASK 0x00000006                // conn_infra_off_bus_m__u_ct2__ahb_flush_thre[2..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__ahb_flush_thre_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__postwrite_dis_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__postwrite_dis_MASK 0x00000001                // conn_infra_off_bus_m__u_ct2__postwrite_dis[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_ct2_CTRL_0_conn_infra_off_bus_m__u_ct2__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_0 (0x1804F000 + 0x074)---

    conn_infra_off_bus_m__u_c6__ctrl_update_status[0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c6__reg_slave_way_en[6..1] - (RW)  xxx 
    RESERVED7[31..7]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_conn_infra_off_bus_m__u_c6__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_conn_infra_off_bus_m__u_c6__reg_slave_way_en_MASK 0x0000007E                // conn_infra_off_bus_m__u_c6__reg_slave_way_en[6..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_conn_infra_off_bus_m__u_c6__reg_slave_way_en_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_conn_infra_off_bus_m__u_c6__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_conn_infra_off_bus_m__u_c6__ctrl_update_status_MASK 0x00000001                // conn_infra_off_bus_m__u_c6__ctrl_update_status[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_0_conn_infra_off_bus_m__u_c6__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_1 (0x1804F000 + 0x078)---

    conn_infra_off_bus_m__u_c6__device_apc_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_1_conn_infra_off_bus_m__u_c6__device_apc_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_1_conn_infra_off_bus_m__u_c6__device_apc_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c6__device_apc_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_1_conn_infra_off_bus_m__u_c6__device_apc_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_2 (0x1804F000 + 0x07c)---

    conn_infra_off_bus_m__u_c6__device_apc_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_2_conn_infra_off_bus_m__u_c6__device_apc_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_2_conn_infra_off_bus_m__u_c6__device_apc_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c6__device_apc_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_2_conn_infra_off_bus_m__u_c6__device_apc_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_3 (0x1804F000 + 0x080)---

    conn_infra_off_bus_m__u_c6__device_apc_con_bit95_bit64[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_3_conn_infra_off_bus_m__u_c6__device_apc_con_bit95_bit64_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_3_conn_infra_off_bus_m__u_c6__device_apc_con_bit95_bit64_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c6__device_apc_con_bit95_bit64[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_3_conn_infra_off_bus_m__u_c6__device_apc_con_bit95_bit64_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_4 (0x1804F000 + 0x084)---

    conn_infra_off_bus_m__u_c6__device_apc_con_bit127_bit96[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_4_conn_infra_off_bus_m__u_c6__device_apc_con_bit127_bit96_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_4_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_4_conn_infra_off_bus_m__u_c6__device_apc_con_bit127_bit96_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c6__device_apc_con_bit127_bit96[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_4_conn_infra_off_bus_m__u_c6__device_apc_con_bit127_bit96_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_5 (0x1804F000 + 0x088)---

    conn_infra_off_bus_m__u_c6__device_apc_con_bit159_bit128[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_5_conn_infra_off_bus_m__u_c6__device_apc_con_bit159_bit128_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_5_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_5_conn_infra_off_bus_m__u_c6__device_apc_con_bit159_bit128_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c6__device_apc_con_bit159_bit128[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_5_conn_infra_off_bus_m__u_c6__device_apc_con_bit159_bit128_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_6 (0x1804F000 + 0x08c)---

    conn_infra_off_bus_m__u_c6__device_apc_con_bit191_bit160[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_6_conn_infra_off_bus_m__u_c6__device_apc_con_bit191_bit160_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_6_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_6_conn_infra_off_bus_m__u_c6__device_apc_con_bit191_bit160_MASK 0xFFFFFFFF                // conn_infra_off_bus_m__u_c6__device_apc_con_bit191_bit160[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_6_conn_infra_off_bus_m__u_c6__device_apc_con_bit191_bit160_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c6_CTRL_7 (0x1804F000 + 0x090)---

    conn_infra_off_bus_m__u_c6__error_flag_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_7_conn_infra_off_bus_m__u_c6__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_7_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_7_conn_infra_off_bus_m__u_c6__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_m__u_c6__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c6_CTRL_7_conn_infra_off_bus_m__u_c6__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_m_u_c15_CTRL_0 (0x1804F000 + 0x094)---

    conn_infra_off_bus_m__u_c15__ctrl_update_status[1..0] - (RO)  xxx 
    conn_infra_off_bus_m__u_c15__reg_layer_way_en[3..2] - (RW)  xxx 
    conn_infra_off_bus_m__u_c15__reg_slave_way_en[4] - (RW)  xxx 
    conn_infra_off_bus_m__u_c15__reg_qos_en[5] - (RW)  xxx 
    conn_infra_off_bus_m__u_c15__error_flag_en[6] - (RW)  xxx 
    RESERVED7[31..7]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__error_flag_en_MASK 0x00000040                // conn_infra_off_bus_m__u_c15__error_flag_en[6]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__error_flag_en_SHFT 6
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_qos_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_qos_en_MASK 0x00000020                // conn_infra_off_bus_m__u_c15__reg_qos_en[5]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_qos_en_SHFT 5
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_slave_way_en_MASK 0x00000010                // conn_infra_off_bus_m__u_c15__reg_slave_way_en[4]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_slave_way_en_SHFT 4
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_layer_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_layer_way_en_MASK 0x0000000C                // conn_infra_off_bus_m__u_c15__reg_layer_way_en[3..2]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__reg_layer_way_en_SHFT 2
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__ctrl_update_status_MASK 0x00000003                // conn_infra_off_bus_m__u_c15__ctrl_update_status[1..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_m_u_c15_CTRL_0_conn_infra_off_bus_m__u_c15__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0 (0x1804F000 + 0x098)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_ready[0] - (RO)  xxx 
    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__force_pready_high[1] - (RW)  xxx 
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__force_pready_high_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__force_pready_high_MASK 0x00000002                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__force_pready_high[1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__force_pready_high_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_ready_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_ready_MASK 0x00000001                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_ready[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_ready_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_1 (0x1804F000 + 0x09c)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__device_APC_con[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_1_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__device_APC_con_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_1_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__device_APC_con_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__device_APC_con[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_1_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__device_APC_con_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_2 (0x1804F000 + 0x0a0)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_2_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_2_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_MASK 0x00000001                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB_CTRL_2_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off_2_sf_SBUS2APB__way_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0 (0x1804F000 + 0x0a4)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_ready[0] - (RO)  xxx 
    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__force_pready_high[1] - (RW)  xxx 
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__force_pready_high_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__force_pready_high_MASK 0x00000002                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__force_pready_high[1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__force_pready_high_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_ready_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_ready_MASK 0x00000001                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_ready[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_ready_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_1 (0x1804F000 + 0x0a8)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__device_APC_con[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_1_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__device_APC_con_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_1_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__device_APC_con_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__device_APC_con[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_1_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__device_APC_con_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_2 (0x1804F000 + 0x0ac)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_2_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_2_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_MASK 0x00000001                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB_CTRL_2_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_SBUS2APB__way_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX_CTRL_0 (0x1804F000 + 0x0b0)---

    conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX__error_flag_en[0] - (RW) Enables error flag, controls the time-out counter
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX__error_flag_en_MASK 0x00000001                // conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX__error_flag_en[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX_CTRL_0_conn_infra_off_bus_s__u_lnk_conn_infra_off_bus_s_c8_to_off2on_apb_APB_GALS_TX__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c8_CTRL_0 (0x1804F000 + 0x0b4)---

    conn_infra_off_bus_s__u_c8__ctrl_update_status[0] - (RO)  xxx 
    conn_infra_off_bus_s__u_c8__reg_slave_way_en[4..1] - (RW)  xxx 
    conn_infra_off_bus_s__u_c8__reg_force_slast[5] - (RW)  xxx 
    conn_infra_off_bus_s__u_c8__error_flag_en[6] - (RW)  xxx 
    RESERVED7[31..7]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__error_flag_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__error_flag_en_MASK 0x00000040                // conn_infra_off_bus_s__u_c8__error_flag_en[6]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__error_flag_en_SHFT 6
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__reg_force_slast_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__reg_force_slast_MASK 0x00000020                // conn_infra_off_bus_s__u_c8__reg_force_slast[5]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__reg_force_slast_SHFT 5
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__reg_slave_way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__reg_slave_way_en_MASK 0x0000001E                // conn_infra_off_bus_s__u_c8__reg_slave_way_en[4..1]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__reg_slave_way_en_SHFT 1
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__ctrl_update_status_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__ctrl_update_status_MASK 0x00000001                // conn_infra_off_bus_s__u_c8__ctrl_update_status[0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c8_CTRL_0_conn_infra_off_bus_s__u_c8__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_0 (0x1804F000 + 0x0b8)---

    conn_infra_off_bus_s__u_c9__way_en_ready[15..0] - (RO)  xxx 
    conn_infra_off_bus_s__u_c9__force_pready_high[31..16] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_conn_infra_off_bus_s__u_c9__force_pready_high_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_conn_infra_off_bus_s__u_c9__force_pready_high_MASK 0xFFFF0000                // conn_infra_off_bus_s__u_c9__force_pready_high[31..16]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_conn_infra_off_bus_s__u_c9__force_pready_high_SHFT 16
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_conn_infra_off_bus_s__u_c9__way_en_ready_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_conn_infra_off_bus_s__u_c9__way_en_ready_MASK 0x0000FFFF                // conn_infra_off_bus_s__u_c9__way_en_ready[15..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_0_conn_infra_off_bus_s__u_c9__way_en_ready_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_1 (0x1804F000 + 0x0bc)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_1_conn_infra_off_bus_s__u_c9__device_APC_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_1_conn_infra_off_bus_s__u_c9__device_APC_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_1_conn_infra_off_bus_s__u_c9__device_APC_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_2 (0x1804F000 + 0x0c0)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_2_conn_infra_off_bus_s__u_c9__device_APC_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_2_conn_infra_off_bus_s__u_c9__device_APC_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_2_conn_infra_off_bus_s__u_c9__device_APC_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_3 (0x1804F000 + 0x0c4)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit95_bit64[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_3_conn_infra_off_bus_s__u_c9__device_APC_con_bit95_bit64_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_3_conn_infra_off_bus_s__u_c9__device_APC_con_bit95_bit64_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit95_bit64[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_3_conn_infra_off_bus_s__u_c9__device_APC_con_bit95_bit64_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_4 (0x1804F000 + 0x0c8)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit127_bit96[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_4_conn_infra_off_bus_s__u_c9__device_APC_con_bit127_bit96_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_4_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_4_conn_infra_off_bus_s__u_c9__device_APC_con_bit127_bit96_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit127_bit96[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_4_conn_infra_off_bus_s__u_c9__device_APC_con_bit127_bit96_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_5 (0x1804F000 + 0x0cc)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit159_bit128[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_5_conn_infra_off_bus_s__u_c9__device_APC_con_bit159_bit128_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_5_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_5_conn_infra_off_bus_s__u_c9__device_APC_con_bit159_bit128_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit159_bit128[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_5_conn_infra_off_bus_s__u_c9__device_APC_con_bit159_bit128_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_6 (0x1804F000 + 0x0d0)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit191_bit160[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_6_conn_infra_off_bus_s__u_c9__device_APC_con_bit191_bit160_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_6_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_6_conn_infra_off_bus_s__u_c9__device_APC_con_bit191_bit160_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit191_bit160[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_6_conn_infra_off_bus_s__u_c9__device_APC_con_bit191_bit160_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_7 (0x1804F000 + 0x0d4)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit223_bit192[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_7_conn_infra_off_bus_s__u_c9__device_APC_con_bit223_bit192_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_7_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_7_conn_infra_off_bus_s__u_c9__device_APC_con_bit223_bit192_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit223_bit192[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_7_conn_infra_off_bus_s__u_c9__device_APC_con_bit223_bit192_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_8 (0x1804F000 + 0x0d8)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit255_bit224[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_8_conn_infra_off_bus_s__u_c9__device_APC_con_bit255_bit224_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_8_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_8_conn_infra_off_bus_s__u_c9__device_APC_con_bit255_bit224_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit255_bit224[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_8_conn_infra_off_bus_s__u_c9__device_APC_con_bit255_bit224_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_9 (0x1804F000 + 0x0dc)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit287_bit256[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_9_conn_infra_off_bus_s__u_c9__device_APC_con_bit287_bit256_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_9_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_9_conn_infra_off_bus_s__u_c9__device_APC_con_bit287_bit256_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit287_bit256[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_9_conn_infra_off_bus_s__u_c9__device_APC_con_bit287_bit256_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_10 (0x1804F000 + 0x0e0)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit319_bit288[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_10_conn_infra_off_bus_s__u_c9__device_APC_con_bit319_bit288_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_10_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_10_conn_infra_off_bus_s__u_c9__device_APC_con_bit319_bit288_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit319_bit288[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_10_conn_infra_off_bus_s__u_c9__device_APC_con_bit319_bit288_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_11 (0x1804F000 + 0x0e4)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit351_bit320[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_11_conn_infra_off_bus_s__u_c9__device_APC_con_bit351_bit320_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_11_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_11_conn_infra_off_bus_s__u_c9__device_APC_con_bit351_bit320_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit351_bit320[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_11_conn_infra_off_bus_s__u_c9__device_APC_con_bit351_bit320_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_12 (0x1804F000 + 0x0e8)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit383_bit352[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_12_conn_infra_off_bus_s__u_c9__device_APC_con_bit383_bit352_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_12_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_12_conn_infra_off_bus_s__u_c9__device_APC_con_bit383_bit352_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit383_bit352[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_12_conn_infra_off_bus_s__u_c9__device_APC_con_bit383_bit352_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_13 (0x1804F000 + 0x0ec)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit415_bit384[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_13_conn_infra_off_bus_s__u_c9__device_APC_con_bit415_bit384_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_13_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_13_conn_infra_off_bus_s__u_c9__device_APC_con_bit415_bit384_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit415_bit384[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_13_conn_infra_off_bus_s__u_c9__device_APC_con_bit415_bit384_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_14 (0x1804F000 + 0x0f0)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit447_bit416[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_14_conn_infra_off_bus_s__u_c9__device_APC_con_bit447_bit416_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_14_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_14_conn_infra_off_bus_s__u_c9__device_APC_con_bit447_bit416_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit447_bit416[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_14_conn_infra_off_bus_s__u_c9__device_APC_con_bit447_bit416_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_15 (0x1804F000 + 0x0f4)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit479_bit448[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_15_conn_infra_off_bus_s__u_c9__device_APC_con_bit479_bit448_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_15_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_15_conn_infra_off_bus_s__u_c9__device_APC_con_bit479_bit448_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit479_bit448[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_15_conn_infra_off_bus_s__u_c9__device_APC_con_bit479_bit448_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_16 (0x1804F000 + 0x0f8)---

    conn_infra_off_bus_s__u_c9__device_APC_con_bit511_bit480[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_16_conn_infra_off_bus_s__u_c9__device_APC_con_bit511_bit480_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_16_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_16_conn_infra_off_bus_s__u_c9__device_APC_con_bit511_bit480_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c9__device_APC_con_bit511_bit480[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_16_conn_infra_off_bus_s__u_c9__device_APC_con_bit511_bit480_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c9_CTRL_17 (0x1804F000 + 0x0fc)---

    conn_infra_off_bus_s__u_c9__way_en[15..0] - (RW)  xxx 
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_17_conn_infra_off_bus_s__u_c9__way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_17_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_17_conn_infra_off_bus_s__u_c9__way_en_MASK 0x0000FFFF                // conn_infra_off_bus_s__u_c9__way_en[15..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c9_CTRL_17_conn_infra_off_bus_s__u_c9__way_en_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_0 (0x1804F000 + 0x100)---

    conn_infra_off_bus_s__u_c10__way_en_ready[15..0] - (RO)  xxx 
    conn_infra_off_bus_s__u_c10__force_pready_high[31..16] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_conn_infra_off_bus_s__u_c10__force_pready_high_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_conn_infra_off_bus_s__u_c10__force_pready_high_MASK 0xFFFF0000                // conn_infra_off_bus_s__u_c10__force_pready_high[31..16]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_conn_infra_off_bus_s__u_c10__force_pready_high_SHFT 16
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_conn_infra_off_bus_s__u_c10__way_en_ready_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_conn_infra_off_bus_s__u_c10__way_en_ready_MASK 0x0000FFFF                // conn_infra_off_bus_s__u_c10__way_en_ready[15..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_0_conn_infra_off_bus_s__u_c10__way_en_ready_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_1 (0x1804F000 + 0x104)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_1_conn_infra_off_bus_s__u_c10__device_APC_con_bit31_bit0_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_1_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_1_conn_infra_off_bus_s__u_c10__device_APC_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit31_bit0[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_1_conn_infra_off_bus_s__u_c10__device_APC_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_2 (0x1804F000 + 0x108)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_2_conn_infra_off_bus_s__u_c10__device_APC_con_bit63_bit32_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_2_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_2_conn_infra_off_bus_s__u_c10__device_APC_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit63_bit32[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_2_conn_infra_off_bus_s__u_c10__device_APC_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_3 (0x1804F000 + 0x10c)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit95_bit64[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_3_conn_infra_off_bus_s__u_c10__device_APC_con_bit95_bit64_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_3_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_3_conn_infra_off_bus_s__u_c10__device_APC_con_bit95_bit64_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit95_bit64[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_3_conn_infra_off_bus_s__u_c10__device_APC_con_bit95_bit64_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_4 (0x1804F000 + 0x110)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit127_bit96[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_4_conn_infra_off_bus_s__u_c10__device_APC_con_bit127_bit96_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_4_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_4_conn_infra_off_bus_s__u_c10__device_APC_con_bit127_bit96_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit127_bit96[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_4_conn_infra_off_bus_s__u_c10__device_APC_con_bit127_bit96_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_5 (0x1804F000 + 0x114)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit159_bit128[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_5_conn_infra_off_bus_s__u_c10__device_APC_con_bit159_bit128_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_5_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_5_conn_infra_off_bus_s__u_c10__device_APC_con_bit159_bit128_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit159_bit128[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_5_conn_infra_off_bus_s__u_c10__device_APC_con_bit159_bit128_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_6 (0x1804F000 + 0x118)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit191_bit160[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_6_conn_infra_off_bus_s__u_c10__device_APC_con_bit191_bit160_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_6_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_6_conn_infra_off_bus_s__u_c10__device_APC_con_bit191_bit160_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit191_bit160[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_6_conn_infra_off_bus_s__u_c10__device_APC_con_bit191_bit160_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_7 (0x1804F000 + 0x11c)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit223_bit192[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_7_conn_infra_off_bus_s__u_c10__device_APC_con_bit223_bit192_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_7_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_7_conn_infra_off_bus_s__u_c10__device_APC_con_bit223_bit192_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit223_bit192[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_7_conn_infra_off_bus_s__u_c10__device_APC_con_bit223_bit192_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_8 (0x1804F000 + 0x120)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit255_bit224[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_8_conn_infra_off_bus_s__u_c10__device_APC_con_bit255_bit224_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_8_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_8_conn_infra_off_bus_s__u_c10__device_APC_con_bit255_bit224_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit255_bit224[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_8_conn_infra_off_bus_s__u_c10__device_APC_con_bit255_bit224_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_9 (0x1804F000 + 0x124)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit287_bit256[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_9_conn_infra_off_bus_s__u_c10__device_APC_con_bit287_bit256_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_9_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_9_conn_infra_off_bus_s__u_c10__device_APC_con_bit287_bit256_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit287_bit256[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_9_conn_infra_off_bus_s__u_c10__device_APC_con_bit287_bit256_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_10 (0x1804F000 + 0x128)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit319_bit288[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_10_conn_infra_off_bus_s__u_c10__device_APC_con_bit319_bit288_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_10_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_10_conn_infra_off_bus_s__u_c10__device_APC_con_bit319_bit288_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit319_bit288[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_10_conn_infra_off_bus_s__u_c10__device_APC_con_bit319_bit288_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_11 (0x1804F000 + 0x12c)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit351_bit320[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_11_conn_infra_off_bus_s__u_c10__device_APC_con_bit351_bit320_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_11_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_11_conn_infra_off_bus_s__u_c10__device_APC_con_bit351_bit320_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit351_bit320[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_11_conn_infra_off_bus_s__u_c10__device_APC_con_bit351_bit320_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_12 (0x1804F000 + 0x130)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit383_bit352[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_12_conn_infra_off_bus_s__u_c10__device_APC_con_bit383_bit352_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_12_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_12_conn_infra_off_bus_s__u_c10__device_APC_con_bit383_bit352_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit383_bit352[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_12_conn_infra_off_bus_s__u_c10__device_APC_con_bit383_bit352_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_13 (0x1804F000 + 0x134)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit415_bit384[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_13_conn_infra_off_bus_s__u_c10__device_APC_con_bit415_bit384_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_13_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_13_conn_infra_off_bus_s__u_c10__device_APC_con_bit415_bit384_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit415_bit384[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_13_conn_infra_off_bus_s__u_c10__device_APC_con_bit415_bit384_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_14 (0x1804F000 + 0x138)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit447_bit416[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_14_conn_infra_off_bus_s__u_c10__device_APC_con_bit447_bit416_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_14_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_14_conn_infra_off_bus_s__u_c10__device_APC_con_bit447_bit416_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit447_bit416[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_14_conn_infra_off_bus_s__u_c10__device_APC_con_bit447_bit416_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_15 (0x1804F000 + 0x13c)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit479_bit448[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_15_conn_infra_off_bus_s__u_c10__device_APC_con_bit479_bit448_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_15_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_15_conn_infra_off_bus_s__u_c10__device_APC_con_bit479_bit448_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit479_bit448[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_15_conn_infra_off_bus_s__u_c10__device_APC_con_bit479_bit448_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_16 (0x1804F000 + 0x140)---

    conn_infra_off_bus_s__u_c10__device_APC_con_bit511_bit480[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_16_conn_infra_off_bus_s__u_c10__device_APC_con_bit511_bit480_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_16_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_16_conn_infra_off_bus_s__u_c10__device_APC_con_bit511_bit480_MASK 0xFFFFFFFF                // conn_infra_off_bus_s__u_c10__device_APC_con_bit511_bit480[31..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_16_conn_infra_off_bus_s__u_c10__device_APC_con_bit511_bit480_SHFT 0

/* =====================================================================================

  ---conn_infra_off_bus_s_u_c10_CTRL_17 (0x1804F000 + 0x144)---

    conn_infra_off_bus_s__u_c10__way_en[15..0] - (RW)  xxx 
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_17_conn_infra_off_bus_s__u_c10__way_en_ADDR CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_17_ADDR
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_17_conn_infra_off_bus_s__u_c10__way_en_MASK 0x0000FFFF                // conn_infra_off_bus_s__u_c10__way_en[15..0]
#define CONN_OFF_BUS_BCRM_conn_infra_off_bus_s_u_c10_CTRL_17_conn_infra_off_bus_s__u_c10__way_en_SHFT 0

#endif // __CONN_OFF_BUS_BCRM_REGS_H__
