/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_bcrm_on.h
//[Revision time]   : Wed Dec 30 22:00:52 2020

#ifndef __CONN_BCRM_ON_REGS_H__
#define __CONN_BCRM_ON_REGS_H__

//****************************************************************************
//
//                     CONN_BCRM_ON CR Definitions
//
//****************************************************************************

#define CONN_BCRM_ON_BASE                                      (CONN_REG_CONN_BCRM_ON_ADDR) //0x1802E000

#define CONN_BCRM_ON_m4_AHB_S_PWR_PROT_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x000) // E000
#define CONN_BCRM_ON_m4_ASYNC_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x004) // E004
#define CONN_BCRM_ON_HOST_CSR_SBUS2APB_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x008) // E008
#define CONN_BCRM_ON_ASYNC_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x00c) // E00C
#define CONN_BCRM_ON_ht0_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x010) // E010
#define CONN_BCRM_ON_h1_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x014) // E014
#define CONN_BCRM_ON_h2_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x018) // E018
#define CONN_BCRM_ON_ht1_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x01c) // E01C
#define CONN_BCRM_ON_h3_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x020) // E020
#define CONN_BCRM_ON_h5_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x024) // E024
#define CONN_BCRM_ON_h6_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x028) // E028
#define CONN_BCRM_ON_n5_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x02c) // E02C
#define CONN_BCRM_ON_n5_CTRL_1_ADDR (CONN_BCRM_ON_BASE + 0x030) // E030
#define CONN_BCRM_ON_n6_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x034) // E034
#define CONN_BCRM_ON_n6_CTRL_1_ADDR (CONN_BCRM_ON_BASE + 0x038) // E038
#define CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_0_ADDR (CONN_BCRM_ON_BASE + 0x03c) // E03C
#define CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_1_ADDR (CONN_BCRM_ON_BASE + 0x040) // E040
#define CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_2_ADDR (CONN_BCRM_ON_BASE + 0x044) // E044
#define CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_3_ADDR (CONN_BCRM_ON_BASE + 0x048) // E048

#endif // __CONN_BCRM_ON_REGS_H__
