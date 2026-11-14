/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef _CONNV3_DBG_H_
#define _CONNV3_DBG_H_

typedef int(*CONNV3_DEV_DBG_FUNC) (int par1, int par2, int par3);
int connv3_dev_dbg_init(void);
int connv3_dev_dbg_deinit(void);


#endif /* _CONNV3_DBG_H_ */
