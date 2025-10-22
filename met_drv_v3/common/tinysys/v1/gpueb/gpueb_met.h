/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __GPUEB_MET_H__
#define __GPUEB_MET_H__
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
void register_gpueb_process_argument(void (*fn)(const char *arg));
void register_gpueb_print_header(int (*fn)(char *buf, int len, int cnt));
void deregister_gpueb_process_argument(void);
void deregister_gpueb_print_header(void);

/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern struct metdevice met_gpueb;


#endif /* __GPUEB_MET_H__ */
