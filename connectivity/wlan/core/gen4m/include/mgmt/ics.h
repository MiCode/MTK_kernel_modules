/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_ICS_H_
#define _FW_LOG_ICS_H_

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
typedef void (*ics_fwlog_event_func_cb)(int, int);
extern ssize_t wifi_ics_fwlog_write(char *buf, size_t count);
extern void wifi_ics_event_func_register(ics_fwlog_event_func_cb pfFwlog);

int IcsInit(void);
int IcsDeInit(void);
#endif /* CFG_SUPPORT_ICS */

#endif /*_FW_LOG_ICS_H_*/
