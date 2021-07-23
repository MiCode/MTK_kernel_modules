/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _ONDIEMET_LOG_H_
#define _ONDIEMET_LOG_H_

#include <linux/device.h>

int ondiemet_log_manager_init(struct device *dev);
int ondiemet_log_manager_uninit(struct device *dev);
int ondiemet_log_manager_start(void);
/* Log manager can be reactivated by inserting new requests, i.e., calling ondiemet_log_req_enq() */
int ondiemet_log_manager_stop(void);
int ondiemet_log_req_enq(const char *src, size_t num, void (*on_fini_cb) (const void *p),
			 const void *param);

#endif				/* _ONDIEMET_LOG_H_ */
