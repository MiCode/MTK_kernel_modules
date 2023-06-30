/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_von_bus_bcrm.h
//[Revision time]   : Mon Aug 30 15:58:30 2021

#ifndef __CONN_VON_BUS_BCRM_REGS_H__
#define __CONN_VON_BUS_BCRM_REGS_H__

//****************************************************************************
//
//                     CONN_VON_BUS_BCRM CR Definitions                     
//
//****************************************************************************

#define CONN_VON_BUS_BCRM_BASE                                 (CONN_REG_CONN_VON_BUS_BCRM_ADDR)

#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_ADDR (CONN_VON_BUS_BCRM_BASE + 0x000) // 0000
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_ADDR (CONN_VON_BUS_BCRM_BASE + 0x004) // 0004
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_1_ADDR (CONN_VON_BUS_BCRM_BASE + 0x008) // 0008
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_2_ADDR (CONN_VON_BUS_BCRM_BASE + 0x00c) // 000C
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX_CTRL_0_ADDR (CONN_VON_BUS_BCRM_BASE + 0x010) // 0010
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x014) // 0014
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x018) // 0018
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_1_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x01c) // 001C
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_2_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x020) // 0020
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_3_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x024) // 0024
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x028) // 0028
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x02c) // 002C
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x030) // 0030
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_1_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x034) // 0034
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_2_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x038) // 0038
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_3_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x03c) // 003C
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_4_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x040) // 0040
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_5_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x044) // 0044
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_6_ADDR        (CONN_VON_BUS_BCRM_BASE + 0x048) // 0048




/* =====================================================================================

  ---conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0 (0x18020000 + 0x000)---

    conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__error_flag_en[0] - (RW)  xxx 
    conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1] - (RW)  xxx 
    conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_monitor_mode[2] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_monitor_mode_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_monitor_mode_MASK 0x00000004                // conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_monitor_mode[2]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_monitor_mode_SHFT 2
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_cmd_cnt_clr_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_cmd_cnt_clr_MASK 0x00000002                // conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_cmd_cnt_clr[1]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__rg_cmd_cnt_clr_SHFT 1
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__error_flag_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__error_flag_en_MASK 0x00000001                // conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__error_flag_en[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h1_to_von2off_AHB_S_PWR_PROT__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0 (0x18020000 + 0x004)---

    conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_ready[0] - (RO)  xxx 
    conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__force_pready_high[1] - (RW)  xxx 
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__force_pready_high_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__force_pready_high_MASK 0x00000002                // conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__force_pready_high[1]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__force_pready_high_SHFT 1
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_ready_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_ready_MASK 0x00000001                // conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_ready[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_ready_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_1 (0x18020000 + 0x008)---

    conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__device_APC_con[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_1_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__device_APC_con_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_1_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_1_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__device_APC_con_MASK 0xFFFFFFFF                // conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__device_APC_con[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_1_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__device_APC_con_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_2 (0x18020000 + 0x00c)---

    conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_2_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_2_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_2_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_MASK 0x00000001                // conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB_CTRL_2_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_SBUS2APB__way_en_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX_CTRL_0 (0x18020000 + 0x010)---

    conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX__error_flag_en[0] - (RW) Enables error flag, controls the time-out counter
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX__error_flag_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX__error_flag_en_MASK 0x00000001                // conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX__error_flag_en[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX_CTRL_0_conn_von_bus__u_lnk_conn_von_bus_h3_to_von2on_apb_APB_GALS_TX__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h0_CTRL_0 (0x18020000 + 0x014)---

    conn_von_bus__u_h0__postwrite_dis[0] - (RW)  xxx 
    conn_von_bus__u_h0__ahb_flush_thre[2..1] - (RW)  xxx 
    conn_von_bus__u_h0__err_flag_en[3] - (RW)  xxx 
    RESERVED4[31..4]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__err_flag_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__err_flag_en_MASK 0x00000008                // conn_von_bus__u_h0__err_flag_en[3]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__err_flag_en_SHFT 3
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__ahb_flush_thre_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__ahb_flush_thre_MASK 0x00000006                // conn_von_bus__u_h0__ahb_flush_thre[2..1]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__ahb_flush_thre_SHFT 1
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__postwrite_dis_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__postwrite_dis_MASK 0x00000001                // conn_von_bus__u_h0__postwrite_dis[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h0_CTRL_0_conn_von_bus__u_h0__postwrite_dis_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h1_CTRL_0 (0x18020000 + 0x018)---

    conn_von_bus__u_h1__ctrl_update_status[0] - (RO)  xxx 
    conn_von_bus__u_h1__reg_slave_way_en[2..1] - (RW)  xxx 
    RESERVED3[31..3]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_conn_von_bus__u_h1__reg_slave_way_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_conn_von_bus__u_h1__reg_slave_way_en_MASK 0x00000006                // conn_von_bus__u_h1__reg_slave_way_en[2..1]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_conn_von_bus__u_h1__reg_slave_way_en_SHFT 1
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_conn_von_bus__u_h1__ctrl_update_status_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_conn_von_bus__u_h1__ctrl_update_status_MASK 0x00000001                // conn_von_bus__u_h1__ctrl_update_status[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_0_conn_von_bus__u_h1__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h1_CTRL_1 (0x18020000 + 0x01c)---

    conn_von_bus__u_h1__device_apc_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_1_conn_von_bus__u_h1__device_apc_con_bit31_bit0_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_1_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_1_conn_von_bus__u_h1__device_apc_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_von_bus__u_h1__device_apc_con_bit31_bit0[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_1_conn_von_bus__u_h1__device_apc_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h1_CTRL_2 (0x18020000 + 0x020)---

    conn_von_bus__u_h1__device_apc_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_2_conn_von_bus__u_h1__device_apc_con_bit63_bit32_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_2_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_2_conn_von_bus__u_h1__device_apc_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_von_bus__u_h1__device_apc_con_bit63_bit32[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_2_conn_von_bus__u_h1__device_apc_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h1_CTRL_3 (0x18020000 + 0x024)---

    conn_von_bus__u_h1__error_flag_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_3_conn_von_bus__u_h1__error_flag_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_3_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_3_conn_von_bus__u_h1__error_flag_en_MASK 0x00000001                // conn_von_bus__u_h1__error_flag_en[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h1_CTRL_3_conn_von_bus__u_h1__error_flag_en_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h2_CTRL_0 (0x18020000 + 0x028)---

    conn_von_bus__u_h2__ctrl_update_status[1..0] - (RO)  xxx 
    conn_von_bus__u_h2__reg_layer_way_en[3..2] - (RW)  xxx 
    conn_von_bus__u_h2__reg_slave_way_en[4] - (RW)  xxx 
    conn_von_bus__u_h2__reg_qos_en[5] - (RW)  xxx 
    conn_von_bus__u_h2__error_flag_en[6] - (RW)  xxx 
    RESERVED7[31..7]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__error_flag_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__error_flag_en_MASK 0x00000040                // conn_von_bus__u_h2__error_flag_en[6]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__error_flag_en_SHFT 6
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_qos_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_qos_en_MASK 0x00000020                // conn_von_bus__u_h2__reg_qos_en[5]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_qos_en_SHFT 5
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_slave_way_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_slave_way_en_MASK 0x00000010                // conn_von_bus__u_h2__reg_slave_way_en[4]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_slave_way_en_SHFT 4
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_layer_way_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_layer_way_en_MASK 0x0000000C                // conn_von_bus__u_h2__reg_layer_way_en[3..2]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__reg_layer_way_en_SHFT 2
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__ctrl_update_status_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__ctrl_update_status_MASK 0x00000003                // conn_von_bus__u_h2__ctrl_update_status[1..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h2_CTRL_0_conn_von_bus__u_h2__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h3_CTRL_0 (0x18020000 + 0x02c)---

    conn_von_bus__u_h3__ctrl_update_status[0] - (RO)  xxx 
    conn_von_bus__u_h3__reg_slave_way_en[2..1] - (RW)  xxx 
    conn_von_bus__u_h3__reg_force_slast[3] - (RW)  xxx 
    conn_von_bus__u_h3__error_flag_en[4] - (RW)  xxx 
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__error_flag_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__error_flag_en_MASK 0x00000010                // conn_von_bus__u_h3__error_flag_en[4]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__error_flag_en_SHFT 4
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__reg_force_slast_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__reg_force_slast_MASK 0x00000008                // conn_von_bus__u_h3__reg_force_slast[3]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__reg_force_slast_SHFT 3
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__reg_slave_way_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__reg_slave_way_en_MASK 0x00000006                // conn_von_bus__u_h3__reg_slave_way_en[2..1]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__reg_slave_way_en_SHFT 1
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__ctrl_update_status_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__ctrl_update_status_MASK 0x00000001                // conn_von_bus__u_h3__ctrl_update_status[0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h3_CTRL_0_conn_von_bus__u_h3__ctrl_update_status_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_0 (0x18020000 + 0x030)---

    conn_von_bus__u_h4__way_en_ready[4..0] - (RO)  xxx 
    conn_von_bus__u_h4__force_pready_high[9..5] - (RW)  xxx 
    RESERVED10[31..10]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_conn_von_bus__u_h4__force_pready_high_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_conn_von_bus__u_h4__force_pready_high_MASK 0x000003E0                // conn_von_bus__u_h4__force_pready_high[9..5]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_conn_von_bus__u_h4__force_pready_high_SHFT 5
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_conn_von_bus__u_h4__way_en_ready_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_conn_von_bus__u_h4__way_en_ready_MASK 0x0000001F                // conn_von_bus__u_h4__way_en_ready[4..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_0_conn_von_bus__u_h4__way_en_ready_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_1 (0x18020000 + 0x034)---

    conn_von_bus__u_h4__device_APC_con_bit31_bit0[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_1_conn_von_bus__u_h4__device_APC_con_bit31_bit0_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_1_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_1_conn_von_bus__u_h4__device_APC_con_bit31_bit0_MASK 0xFFFFFFFF                // conn_von_bus__u_h4__device_APC_con_bit31_bit0[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_1_conn_von_bus__u_h4__device_APC_con_bit31_bit0_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_2 (0x18020000 + 0x038)---

    conn_von_bus__u_h4__device_APC_con_bit63_bit32[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_2_conn_von_bus__u_h4__device_APC_con_bit63_bit32_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_2_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_2_conn_von_bus__u_h4__device_APC_con_bit63_bit32_MASK 0xFFFFFFFF                // conn_von_bus__u_h4__device_APC_con_bit63_bit32[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_2_conn_von_bus__u_h4__device_APC_con_bit63_bit32_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_3 (0x18020000 + 0x03c)---

    conn_von_bus__u_h4__device_APC_con_bit95_bit64[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_3_conn_von_bus__u_h4__device_APC_con_bit95_bit64_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_3_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_3_conn_von_bus__u_h4__device_APC_con_bit95_bit64_MASK 0xFFFFFFFF                // conn_von_bus__u_h4__device_APC_con_bit95_bit64[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_3_conn_von_bus__u_h4__device_APC_con_bit95_bit64_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_4 (0x18020000 + 0x040)---

    conn_von_bus__u_h4__device_APC_con_bit127_bit96[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_4_conn_von_bus__u_h4__device_APC_con_bit127_bit96_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_4_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_4_conn_von_bus__u_h4__device_APC_con_bit127_bit96_MASK 0xFFFFFFFF                // conn_von_bus__u_h4__device_APC_con_bit127_bit96[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_4_conn_von_bus__u_h4__device_APC_con_bit127_bit96_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_5 (0x18020000 + 0x044)---

    conn_von_bus__u_h4__device_APC_con_bit159_bit128[31..0] - (RW)  xxx 

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_5_conn_von_bus__u_h4__device_APC_con_bit159_bit128_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_5_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_5_conn_von_bus__u_h4__device_APC_con_bit159_bit128_MASK 0xFFFFFFFF                // conn_von_bus__u_h4__device_APC_con_bit159_bit128[31..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_5_conn_von_bus__u_h4__device_APC_con_bit159_bit128_SHFT 0

/* =====================================================================================

  ---conn_von_bus_u_h4_CTRL_6 (0x18020000 + 0x048)---

    conn_von_bus__u_h4__way_en[4..0] - (RW)  xxx 
    RESERVED5[31..5]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_6_conn_von_bus__u_h4__way_en_ADDR CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_6_ADDR
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_6_conn_von_bus__u_h4__way_en_MASK 0x0000001F                // conn_von_bus__u_h4__way_en[4..0]
#define CONN_VON_BUS_BCRM_conn_von_bus_u_h4_CTRL_6_conn_von_bus__u_h4__way_en_SHFT 0

#endif // __CONN_VON_BUS_BCRM_REGS_H__
