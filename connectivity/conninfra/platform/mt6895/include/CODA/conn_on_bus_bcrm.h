/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
//[File]            : conn_on_bus_bcrm.h
//[Revision time]   : Fri Apr  9 11:19:28 2021


#ifndef __CONN_ON_BUS_BCRM_REGS_H__
#define __CONN_ON_BUS_BCRM_REGS_H__

//****************************************************************************
//
//                     CONN_ON_BUS_BCRM CR Definitions                     
//
//****************************************************************************

#define CONN_ON_BUS_BCRM_BASE                                  (CONN_REG_CONN_INFRA_ON_BUS_BCRM_ADDR)

#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX_CTRL_0_ADDR (CONN_ON_BUS_BCRM_BASE + 0x000) // B000
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh0_CTRL_0_ADDR (CONN_ON_BUS_BCRM_BASE + 0x004) // B004
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh1_CTRL_0_ADDR (CONN_ON_BUS_BCRM_BASE + 0x008) // B008
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX_CTRL_0_ADDR (CONN_ON_BUS_BCRM_BASE + 0x00c) // B00C
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc0_CTRL_0_ADDR (CONN_ON_BUS_BCRM_BASE + 0x010) // B010
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc1_CTRL_0_ADDR (CONN_ON_BUS_BCRM_BASE + 0x014) // B014




/* =====================================================================================

  ---conn_infra_on_host_bus_u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX_CTRL_0 (0x1803B000 + 0x000)---

    conn_infra_on_host_bus__u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX__rx_clock_cg_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX_CTRL_0_conn_infra_on_host_bus__u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX__rx_clock_cg_en_ADDR CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX_CTRL_0_ADDR
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX_CTRL_0_conn_infra_on_host_bus__u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX__rx_clock_cg_en_MASK 0x00000001                // conn_infra_on_host_bus__u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX__rx_clock_cg_en[0]
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX_CTRL_0_conn_infra_on_host_bus__u_lnk_von2on_apb_to_conn_infra_on_host_bus_kh0_APB_GALS_RX__rx_clock_cg_en_SHFT 0

/* =====================================================================================

  ---conn_infra_on_host_bus_u_kh0_CTRL_0 (0x1803B000 + 0x004)---

    conn_infra_on_host_bus__u_kh0__way_en[1..0] - (RW)  xxx 
    RESERVED2[31..2]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh0_CTRL_0_conn_infra_on_host_bus__u_kh0__way_en_ADDR CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh0_CTRL_0_ADDR
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh0_CTRL_0_conn_infra_on_host_bus__u_kh0__way_en_MASK 0x00000003                // conn_infra_on_host_bus__u_kh0__way_en[1..0]
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh0_CTRL_0_conn_infra_on_host_bus__u_kh0__way_en_SHFT 0

/* =====================================================================================

  ---conn_infra_on_host_bus_u_kh1_CTRL_0 (0x1803B000 + 0x008)---

    conn_infra_on_host_bus__u_kh1__way_en[15..0] - (RW)  xxx 
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh1_CTRL_0_conn_infra_on_host_bus__u_kh1__way_en_ADDR CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh1_CTRL_0_ADDR
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh1_CTRL_0_conn_infra_on_host_bus__u_kh1__way_en_MASK 0x0000FFFF                // conn_infra_on_host_bus__u_kh1__way_en[15..0]
#define CONN_ON_BUS_BCRM_conn_infra_on_host_bus_u_kh1_CTRL_0_conn_infra_on_host_bus__u_kh1__way_en_SHFT 0

/* =====================================================================================

  ---conn_infra_on_conn_bus_u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX_CTRL_0 (0x1803B000 + 0x00c)---

    conn_infra_on_conn_bus__u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX__rx_clock_cg_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX_CTRL_0_conn_infra_on_conn_bus__u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX__rx_clock_cg_en_ADDR CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX_CTRL_0_ADDR
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX_CTRL_0_conn_infra_on_conn_bus__u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX__rx_clock_cg_en_MASK 0x00000001                // conn_infra_on_conn_bus__u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX__rx_clock_cg_en[0]
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX_CTRL_0_conn_infra_on_conn_bus__u_lnk_off2on_apb_to_conn_infra_on_conn_bus_kc0_APB_GALS_RX__rx_clock_cg_en_SHFT 0

/* =====================================================================================

  ---conn_infra_on_conn_bus_u_kc0_CTRL_0 (0x1803B000 + 0x010)---

    conn_infra_on_conn_bus__u_kc0__way_en[0] - (RW)  xxx 
    RESERVED1[31..1]             - (RO) Reserved bits

 =====================================================================================*/
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc0_CTRL_0_conn_infra_on_conn_bus__u_kc0__way_en_ADDR CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc0_CTRL_0_ADDR
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc0_CTRL_0_conn_infra_on_conn_bus__u_kc0__way_en_MASK 0x00000001                // conn_infra_on_conn_bus__u_kc0__way_en[0]
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc0_CTRL_0_conn_infra_on_conn_bus__u_kc0__way_en_SHFT 0

/* =====================================================================================

  ---conn_infra_on_conn_bus_u_kc1_CTRL_0 (0x1803B000 + 0x014)---

    conn_infra_on_conn_bus__u_kc1__way_en[15..0] - (RW)  xxx 
    RESERVED16[31..16]           - (RO) Reserved bits

 =====================================================================================*/
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc1_CTRL_0_conn_infra_on_conn_bus__u_kc1__way_en_ADDR CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc1_CTRL_0_ADDR
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc1_CTRL_0_conn_infra_on_conn_bus__u_kc1__way_en_MASK 0x0000FFFF                // conn_infra_on_conn_bus__u_kc1__way_en[15..0]
#define CONN_ON_BUS_BCRM_conn_infra_on_conn_bus_u_kc1_CTRL_0_conn_infra_on_conn_bus__u_kc1__way_en_SHFT 0

#endif // __CONN_ON_BUS_BCRM_REGS_H__
