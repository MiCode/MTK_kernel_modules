/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __TINYSYS_MGR_H__
#define __TINYSYS_MGR_H__
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/device.h>


/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
enum {
    ONDIEMET_SSPM,
    ONDIEMET_MCUPM,
    ONDIEMET_TINYSYS_NUM,
};


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
int ondiemet_attr_init(struct device *dev);
int ondiemet_attr_uninit(struct device *dev);

int ondiemet_log_manager_init(struct device *dev);
int ondiemet_log_manager_uninit(struct device *dev);
void ondiemet_log_manager_start(void);
void ondiemet_log_manager_stop(void);

void ondiemet_start(void);
void ondiemet_stop(void);
void ondiemet_extract(void);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern unsigned int ondiemet_module[ONDIEMET_TINYSYS_NUM];
extern unsigned int ondiemet_record_check[ONDIEMET_TINYSYS_NUM];
extern unsigned int ondiemet_recording[ONDIEMET_TINYSYS_NUM];


#endif /* __TINYSYS_MGR_H__ */
