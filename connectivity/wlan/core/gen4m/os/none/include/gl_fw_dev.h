/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_FW_DEV_H_
#define _GL_FW_DEV_H_

extern ssize_t wifi_index_fwlog_write(char *buf, size_t count);
int FwLogDevInit(void);
int FwLogDevUninit(void);

#endif
