/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//[File]            : conn_semaphore.h
//[Revision time]   : Mon Nov  9 19:46:03 2020

#ifndef __CONN_SEMAPHORE_REGS_H__
#define __CONN_SEMAPHORE_REGS_H__

//****************************************************************************
//
//                     CONN_SEMAPHORE CR Definitions
//
//****************************************************************************

#define CONN_SEMAPHORE_BASE                                    (CONN_REG_CONN_SEMAPHORE_ADDR)

#define CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M0_STA_REP_1_ADDR      (CONN_SEMAPHORE_BASE + 0x0400) // 0400
#define CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M1_STA_REP_1_ADDR      (CONN_SEMAPHORE_BASE + 0x1400) // 1400
#define CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_STA_ADDR             (CONN_SEMAPHORE_BASE + 0x2000) // 2000
#define CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_REL_ADDR             (CONN_SEMAPHORE_BASE + 0x2200) // 2200
#define CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M2_STA_REP_1_ADDR      (CONN_SEMAPHORE_BASE + 0x2400) // 2400
#define CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M3_STA_REP_1_ADDR      (CONN_SEMAPHORE_BASE + 0x3400) // 3400


#endif // __CONN_SEMAPHORE_REGS_H__
