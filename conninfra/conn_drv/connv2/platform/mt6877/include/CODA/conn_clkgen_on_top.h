/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_clkgen_on_top.h
//[Revision time]   : Mon Nov  9 19:44:15 2020

#ifndef __CONN_CLKGEN_ON_TOP_REGS_H__
#define __CONN_CLKGEN_ON_TOP_REGS_H__

//****************************************************************************
//
//                     CONN_CLKGEN_ON_TOP CR Definitions
//
//****************************************************************************

#define CONN_CLKGEN_ON_TOP_BASE                                (CONN_REG_CONN_INFRA_CLKGEN_ON_TOP_ADDR) // 0x1800_9000

#define CONN_CLKGEN_ON_TOP_CKGEN_BUS_BPLL_DIV_1_ADDR           (CONN_CLKGEN_ON_TOP_BASE + 0x000) // 9000
#define CONN_CLKGEN_ON_TOP_CKGEN_BUS_BPLL_DIV_2_ADDR           (CONN_CLKGEN_ON_TOP_BASE + 0x004) // 9004
#define CONN_CLKGEN_ON_TOP_CKGEN_BUS_WPLL_DIV_1_ADDR           (CONN_CLKGEN_ON_TOP_BASE + 0x008) // 9008
#define CONN_CLKGEN_ON_TOP_CKGEN_BUS_WPLL_DIV_2_ADDR           (CONN_CLKGEN_ON_TOP_BASE + 0x00C) // 900C
#define CONN_CLKGEN_ON_TOP_CKGEN_RFSPI_WPLL_DIV_ADDR           (CONN_CLKGEN_ON_TOP_BASE + 0x040) // 9040
#define CONN_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR           (CONN_CLKGEN_ON_TOP_BASE + 0x048) // 9048
#define CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR                      (CONN_CLKGEN_ON_TOP_BASE + 0xA00) // 9A00

#define CONN_CLKGEN_ON_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_SEL_ADDR CONN_CLKGEN_ON_TOP_CKGEN_RFSPI_WPLL_DIV_ADDR
#define CONN_CLKGEN_ON_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_SEL_MASK 0x000000FC                // WPLL_DIV_SEL[7..2]
#define CONN_CLKGEN_ON_TOP_CKGEN_RFSPI_WPLL_DIV_WPLL_DIV_SEL_SHFT 2

#endif // __CONN_CLKGEN_ON_TOP_REGS_H__
