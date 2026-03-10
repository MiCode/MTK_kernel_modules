/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _SA_LOG_H_
#define _SA_LOG_H_

#if (CFG_SUPPORT_SA_LOG == 1)
typedef void (*salog_event_func_cb)(int, int);
extern ssize_t wifi_salog_write(char *buf, size_t count);
extern void wifi_salog_event_func_register(salog_event_func_cb pflog);
int SalogInit(void);
int SalogDeInit(void);
#endif /* CFG_SUPPORT_SA_LOG */

#endif /*_SA_LOG_H_*/
