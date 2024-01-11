/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __TINYSYS_GPUEB_H__
#define __TINYSYS_GPUEB_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include "tinysys_common.h"
#include "mtk_tinysys_ipi.h"  /* for mtk_ipi_device */
/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define GPUEB_LOG_FILE           0
#define GPUEB_LOG_SRAM           1
#define GPUEB_LOG_DRAM           2

#define GPUEB_RUN_NORMAL         0
#define GPUEB_RUN_CONTINUOUS     1


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern int gpueb_buf_available;

#endif /* __TINYSYS_GPUEB_H__ */
