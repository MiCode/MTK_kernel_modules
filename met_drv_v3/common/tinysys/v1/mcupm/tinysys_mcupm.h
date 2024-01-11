/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __TINYSYS_MCUPM_H__
#define __TINYSYS_MCUPM_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include "tinysys_common.h"
#include "mtk_tinysys_ipi.h"  /* for mtk_ipi_device */
/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define MCUPM_LOG_FILE           0
#define MCUPM_LOG_SRAM           1
#define MCUPM_LOG_DRAM           2

#define MCUPM_RUN_NORMAL         0
#define MCUPM_RUN_CONTINUOUS     1


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern int mcupm_buf_available;

#endif /* __TINYSYS_MCUPM_H__ */
