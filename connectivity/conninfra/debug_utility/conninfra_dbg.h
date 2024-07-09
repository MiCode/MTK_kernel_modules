/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CONNINFRA_DBG_H_
#define _CONNINFRA_DBG_H_
#include "osal.h"

typedef int(*CONNINFRA_DEV_DBG_FUNC) (int par1, int par2, int par3);
int conninfra_dev_dbg_init(void);
int conninfra_dev_dbg_deinit(void);

ssize_t conninfra_dbg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t conninfra_dbg_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

#endif /* _CONNINFRA_DBG_H_ */
