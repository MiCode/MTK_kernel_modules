/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef _ONDIEMET_LOG_H_
#define _ONDIEMET_LOG_H_
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/device.h>
#if IS_ENABLED(CONFIG_MTK_GMO_RAM_OPTIMIZE) || IS_ENABLED(CONFIG_MTK_MET_MEM_ALLOC)
#include <linux/dma-mapping.h>
#endif


/*****************************************************************************
 * define declaration
 *****************************************************************************/


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
extern unsigned int sspm_log_phy_addr;
#endif
extern unsigned int sspm_buffer_size;

#endif				/* _ONDIEMET_LOG_H_ */
