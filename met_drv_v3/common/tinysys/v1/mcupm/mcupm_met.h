/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MCUPM_MET_H__
#define __MCUPM_MET_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include "met_drv.h"  /* for metdevice */


/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
void notify_mcupm_cpu_pmu(int flag);
void register_mcupm_process_argument(void (*fn)(const char *arg));
void register_mcupm_print_header(int (*fn)(char *buf, int len, int cnt));
void deregister_mcupm_process_argument(void);
void deregister_mcupm_print_header(void);

/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern struct metdevice met_mcupm;


#endif /* __MCUPM_MET_H__ */
