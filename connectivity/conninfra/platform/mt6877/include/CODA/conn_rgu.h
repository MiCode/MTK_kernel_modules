/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_rgu.h
//[Revision time]   : Mon Nov  9 19:11:20 2020

#ifndef __CONN_RGU_REGS_H__
#define __CONN_RGU_REGS_H__

//****************************************************************************
//
//                     CONN_RGU CR Definitions
//
//****************************************************************************

#define CONN_RGU_BASE                                          (CONN_REG_CONN_INFRA_RGU_ADDR)

#define CONN_RGU_WFSYS_ON_TOP_PWR_CTL_ADDR                     (CONN_RGU_BASE + 0x010) // 0010
#define CONN_RGU_BGFYS_ON_TOP_PWR_CTL_ADDR                     (CONN_RGU_BASE + 0x020) // 0020
#define CONN_RGU_SYSRAM_HWCTL_PDN_ADDR                         (CONN_RGU_BASE + 0x050) // 0050
#define CONN_RGU_SYSRAM_HWCTL_SLP_ADDR                         (CONN_RGU_BASE + 0x054) // 0054
#define CONN_RGU_CO_EXT_MEM_HWCTL_PDN_ADDR                     (CONN_RGU_BASE + 0x070) // 0070
#define CONN_RGU_CO_EXT_MEM_HWCTL_SLP_ADDR                     (CONN_RGU_BASE + 0x074) // 0074
#define CONN_RGU_WFSYS_WA_WDT_EN_ADDR                          (CONN_RGU_BASE + 0x104) // 0104
#define CONN_RGU_WFSYS_ON_TOP_PWR_ST_ADDR                      (CONN_RGU_BASE + 0x400) // 0400


#endif // __CONN_RGU_REGS_H__
