/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
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
