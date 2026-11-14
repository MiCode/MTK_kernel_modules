/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __CONN_MCU_CONFG_REGS_H__
#define __CONN_MCU_CONFG_REGS_H__

#define CONN_MCU_CONFG_BASE                                    0x80000000

#define CONN_MCU_CONFG_MCCR_SET_ADDR                           (CONN_MCU_CONFG_BASE + 0x0104)
#define CONN_MCU_CONFG_MCCR_CLEAR_ADDR                         (CONN_MCU_CONFG_BASE + 0x0108)

#define CONN_MCU_CONFG_MCCR_SET_GDMA_CH_ADDR                   CONN_MCU_CONFG_MCCR_SET_ADDR
#define CONN_MCU_CONFG_MCCR_SET_GDMA_CH_MASK                   0x0FFF0000
#define CONN_MCU_CONFG_MCCR_SET_GDMA_CH_SHFT                   16

#define CONN_MCU_CONFG_MCCR_CLEAR_GDMA_CH_ADDR                 CONN_MCU_CONFG_MCCR_CLEAR_ADDR
#define CONN_MCU_CONFG_MCCR_CLEAR_GDMA_CH_MASK                 0x0FFF0000
#define CONN_MCU_CONFG_MCCR_CLEAR_GDMA_CH_SHFT                 16

#endif /* __CONN_MCU_CONFG_REGS_H__*/
