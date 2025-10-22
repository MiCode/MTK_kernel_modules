/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONNSYSLOG_MACRO_H_
#define _CONNSYSLOG_MACRO_H_

#define DECLARE_FW_LOG_SYS(name) \
int fw_log_##name##_mcu_init(int conn_type, struct connlog_to_user_cb *cb); \
void fw_log_##name##_mcu_deinit(void); \
void fw_log_##name##_mcu_event_cb(void);


#define FUNC_IMPL_FW_LOG_SYS_DECLARE(name) \
struct fw_log_mcu_info g_##name##_mcu_info;

#define FUNC_IMPL_FW_LOG_SYS_BODY(name) \
static ssize_t fw_log_##name##_mcu_read(struct file *filp, char __user *buf, \
	size_t count, loff_t *f_pos) \
{ \
	if (g_##name##_mcu_info.callback.read_to_user_cb == NULL) \
		return 0; \
	return (*(g_##name##_mcu_info.callback.read_to_user_cb)) (g_##name##_mcu_info.callback.conn_type_id, buf, count); \
} \
\
static unsigned int fw_log_##name##_mcu_poll(struct file *filp, poll_table *wait) \
{ \
	poll_wait(filp, &g_##name##_mcu_info.wq, wait); \
	if (g_##name##_mcu_info.callback.get_buf_size == NULL) \
		return 0; \
	if ((*(g_##name##_mcu_info.callback.get_buf_size))(g_##name##_mcu_info.callback.conn_type_id) > 0) \
		return POLLIN | POLLRDNORM; \
	return 0; \
} \
\
const struct file_operations g_log_##name##_mcu_ops = { \
	.open = fw_log_mcu_open, \
	.release = fw_log_mcu_close, \
	.read = fw_log_##name##_mcu_read, \
	.poll = fw_log_##name##_mcu_poll, \
}; \
\
void fw_log_##name##_mcu_event_cb(void) \
{ \
	wake_up_interruptible(&g_##name##_mcu_info.wq); \
} \
\
int fw_log_##name##_mcu_init(int conn_type, struct connlog_to_user_cb *cb) \
{ \
	int ret = 0; \
\
	g_##name##_mcu_info.conn_type = conn_type; \
	g_##name##_mcu_info.driver_name = "fw_log_" #name "mcu"; \
	memcpy(&g_##name##_mcu_info.callback, cb, sizeof(*cb)); \
	ret = fw_log_mcu_init(&g_##name##_mcu_info, &g_log_##name##_mcu_ops); \
	return ret; \
} \
\
void fw_log_##name##_mcu_deinit(void) \
{ \
	fw_log_mcu_deinit(&g_##name##_mcu_info); \
}



#endif /* _CONNSYSLOG_MACRO_H_ */
