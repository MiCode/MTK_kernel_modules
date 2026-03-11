/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONN_KERN_ADAPTOR_H_
#define _CONN_KERN_ADAPTOR_H_

#include <linux/types.h>


int conn_kern_adaptor_init(int (*dbg_handler)(int x, int y, int z, char *buf, int buf_sz));
int conn_kern_adaptor_deinit(void);


#endif /* _CONN_ADAPTOR_H_ */
