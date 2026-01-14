/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __SSPM_LOG_H__
#define __SSPM_LOG_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <tinysys_met_log.h>

/*****************************************************************************
 * define declaration
 *****************************************************************************/
//#define MET_PRINTF_D(...)         pr_debug(__VA_ARGS__)
#define MET_PRINTF_D(...)


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
int sspm_log_init(struct device *dev);
int sspm_log_uninit(struct device *dev);
int sspm_log_start(void);
int sspm_log_stop(void);

int sspm_log_req_enq(
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param);
int sspm_parse_num(const char *str, unsigned int *value, int len);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern void *sspm_log_virt_addr;
#if IS_ENABLED(CONFIG_MTK_GMO_RAM_OPTIMIZE) || IS_ENABLED(CONFIG_MTK_MET_MEM_ALLOC)
extern dma_addr_t sspm_log_phy_addr;
#else
extern phys_addr_t sspm_log_phy_addr;
#endif
extern unsigned int sspm_buffer_size;

#endif				/* __SSPM_LOG_H__ */
