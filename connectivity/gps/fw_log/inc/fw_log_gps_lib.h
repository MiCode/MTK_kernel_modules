/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_GPS_LIB_H_
#define _FW_LOG_GPS_LIB_H_

#define USE_FW_LOG_GPS_LIB (1)
#if USE_FW_LOG_GPS_LIB

#include <linux/uaccess.h>
#include <linux/wait.h>


#define GPS_FW_LOG_USER_MAX (2)
#define GPS_FW_LOG_EACH_BUF_SIZE (100 * 1024)
struct gps_fw_log_each_ctx {
	unsigned char buf[GPS_FW_LOG_EACH_BUF_SIZE];
	unsigned int len;
	wait_queue_head_t wq;
	bool is_allocated;
	bool read_after_poll;
};

int  gps_fw_log_all_ctx_init(void);
void gps_fw_log_all_ctx_exit(void);

struct gps_fw_log_each_ctx *gps_fw_log_each_ctx_alloc(void);
void gps_fw_log_each_ctx_free(struct gps_fw_log_each_ctx *p_ctx);

void gps_fw_log_data_submit_to_all(const unsigned char *buf, int len);
void gps_fw_log_data_submit_to_each(struct gps_fw_log_each_ctx *p_ctx,
	const unsigned char *buf, int len);

int gps_fw_log_data_get_size(struct gps_fw_log_each_ctx *p_ctx);
int gps_fw_log_data_copy_to_user(struct gps_fw_log_each_ctx *p_ctx, char __user *buf, size_t count);
#endif /* USE_FW_LOG_GPS_LIB */

#endif /* _FW_LOG_GPS_LIB_H_ */

