/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __GPUEB_MET_LOG_H__
#define __GPUEB_MET_LOG_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <tinysys_met_log.h>

/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
int gpueb_log_init(struct device *dev);
int gpueb_log_uninit(struct device *dev);
int gpueb_log_start(void);
int gpueb_log_stop(void);

int gpueb_log_req_enq(
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param);
int gpueb_parse_num(const char *str, unsigned int *value, int len);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern void *gpueb_log_virt_addr;
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
extern dma_addr_t gpueb_log_phy_addr;
#else
extern phys_addr_t gpueb_log_phy_addr;
#endif
extern unsigned int gpueb_buffer_size;

#endif	/* __GPUEB_MET_LOG_H__ */
