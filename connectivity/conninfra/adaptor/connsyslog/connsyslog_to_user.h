/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONNSYSLOG_TO_USER_H_
#define _CONNSYSLOG_TO_USER_H_

#include "conn_drv.h"

struct connlog_to_user_cb {
	int conn_type_id;
	int (*register_evt_cb)(int conn_type_id, void (*evt_cb)(void));
	ssize_t (*read_to_user_cb) (int conn_type_id, char __user *buf, size_t count);
	unsigned int (*get_buf_size) (int conn_type_id);
};

int connlog_to_user_register(int conn_type,
		struct connlog_to_user_cb *data);
int connlog_to_user_unregister(int conn_type);


#endif /* _CONN_DRV_H_ */
