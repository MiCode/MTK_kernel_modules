/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __TINYSYS_SSPM_H__
#define __TINYSYS_SSPM_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include "tinysys_common.h"

/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define SSPM_LOG_FILE           0
#define SSPM_LOG_SRAM           1
#define SSPM_LOG_DRAM           2

#define SSPM_RUN_NORMAL         0
#define SSPM_RUN_CONTINUOUS     1


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern int sspm_buf_available;

#endif /* __TINYSYS_SSPM_H__ */
