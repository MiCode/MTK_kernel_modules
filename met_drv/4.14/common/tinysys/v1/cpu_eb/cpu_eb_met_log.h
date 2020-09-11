/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __CPU_EB_MET_LOG_H__
#define __CPU_EB_MET_LOG_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/device.h>
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
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
int cpu_eb_log_init(struct device *dev);
int cpu_eb_log_uninit(struct device *dev);
int cpu_eb_log_start(void);
int cpu_eb_log_stop(void);

int cpu_eb_log_req_enq(
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param);
int cpu_eb_parse_num(const char *str, unsigned int *value, int len);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern void *cpu_eb_log_virt_addr;
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
extern dma_addr_t cpu_eb_log_phy_addr;
#else
extern unsigned int cpu_eb_log_phy_addr;
#endif
extern unsigned int cpu_eb_buffer_size;

#endif	/* __CPU_EB_MET_LOG_H__ */
