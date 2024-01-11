/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#include "fw_log_gps_lib.h"

#if USE_FW_LOG_GPS_LIB


struct gps_fw_log_all_ctx {
	struct gps_fw_log_each_ctx users[GPS_FW_LOG_USER_MAX];
	struct mutex lock;
};

static struct gps_fw_log_all_ctx g_gps_fw_log_all_ctx;

void gps_fw_log_lock(void)
{
	mutex_lock(&g_gps_fw_log_all_ctx.lock);
}

void gps_fw_log_unlock(void)
{
	mutex_unlock(&g_gps_fw_log_all_ctx.lock);
}


int gps_fw_log_all_ctx_init(void)
{
	int i;
	struct gps_fw_log_each_ctx *p_ctx;

	for (i = 0; i < GPS_FW_LOG_USER_MAX; i++) {
		p_ctx = &g_gps_fw_log_all_ctx.users[i];
		p_ctx->len = 0;
		p_ctx->is_allocated = false;
		p_ctx->read_after_poll = false;
		init_waitqueue_head(&p_ctx->wq);
	}
	mutex_init(&g_gps_fw_log_all_ctx.lock);
	return 0;
}

void gps_fw_log_all_ctx_exit(void)
{
	mutex_destroy(&g_gps_fw_log_all_ctx.lock);
}


struct gps_fw_log_each_ctx *gps_fw_log_each_ctx_alloc(void)
{
	int i;
	struct gps_fw_log_each_ctx *p_ctx;

	gps_fw_log_lock();
	for (i = 0; i < GPS_FW_LOG_USER_MAX; i++) {
		p_ctx = &g_gps_fw_log_all_ctx.users[i];
		if (!p_ctx->is_allocated) {
			p_ctx->len = 0;
			p_ctx->is_allocated = true;
			p_ctx->read_after_poll = false;
			break;
		}
	}
	gps_fw_log_unlock();
	if (i < GPS_FW_LOG_USER_MAX)
		return p_ctx;
	else
		return NULL;
}

void gps_fw_log_each_ctx_free(struct gps_fw_log_each_ctx *p_ctx)
{
	int i;

	gps_fw_log_lock();
	for (i = 0; i < GPS_FW_LOG_USER_MAX; i++) {
		if (p_ctx != &g_gps_fw_log_all_ctx.users[i])
			continue;

		if (p_ctx->is_allocated) {
			p_ctx->len = 0;
			p_ctx->is_allocated = false;
			p_ctx->read_after_poll = false;
		}
		break;
	}
	gps_fw_log_unlock();
}

void gps_fw_log_data_submit_to_all(const unsigned char *buf, int len)
{
	int i;
	struct gps_fw_log_each_ctx *p_ctx;

	for (i = 0; i < GPS_FW_LOG_USER_MAX; i++) {
		p_ctx = &g_gps_fw_log_all_ctx.users[i];
		gps_fw_log_data_submit_to_each(p_ctx, buf, len);
	}
}

void gps_fw_log_data_submit_to_each(struct gps_fw_log_each_ctx *p_ctx,
	const unsigned char *buf, int len)
{
	int old_len, new_len;
	bool fail = false;

	if (len <= 0)
		return;

	gps_fw_log_lock();
	if (!p_ctx->is_allocated) {
		gps_fw_log_unlock();
		return;
	}

	old_len = p_ctx->len;
	new_len = old_len + len;
	if (new_len <= GPS_FW_LOG_EACH_BUF_SIZE) {
		memcpy(&p_ctx->buf[old_len], buf, len);
		p_ctx->len = new_len;
	} else
		fail = true;
	new_len = p_ctx->len;
	gps_fw_log_unlock();

	wake_up_interruptible(&p_ctx->wq);

	if (!fail)
		return;

	pr_info("%s: p_ctx=0x%p, in_len=%d, fail=%d, ctx_len=%d -> %d",
		__func__, p_ctx, len, fail, old_len, new_len);
}

int gps_fw_log_data_get_size(struct gps_fw_log_each_ctx *p_ctx)
{
	int len;

	gps_fw_log_lock();
	len = p_ctx->len;
	p_ctx->read_after_poll = true;
	gps_fw_log_unlock();

	return len;
}

int gps_fw_log_data_copy_to_user(struct gps_fw_log_each_ctx *p_ctx, char __user *buf, size_t count)
{
	int copy_len;
	int retval;
	bool ret_zero = false;

_try_again:
	gps_fw_log_lock();
	if (count >= p_ctx->len)
		copy_len = p_ctx->len;
	else
		copy_len = count;

	if (p_ctx->read_after_poll) {
		p_ctx->read_after_poll = false;
		ret_zero = true;
	}

	if (copy_len <= 0) {
		gps_fw_log_unlock();

		if (ret_zero)
			return 0;

		retval = wait_event_interruptible(p_ctx->wq, (p_ctx->len > 0));
		if (retval != 0)
			return -ERESTARTSYS;
		goto _try_again;
	}

	retval = copy_to_user(buf, &p_ctx->buf[0], copy_len);
	if (retval == 0) {
		p_ctx->len -= copy_len;
		if (p_ctx->len > 0)
			memcpy(&p_ctx->buf[copy_len], &p_ctx->buf[0], p_ctx->len);
		retval = copy_len;
	} else
		retval = -EFAULT;
	gps_fw_log_unlock();

	return retval;
}

#endif /* USE_FW_LOG_GPS_LIB */

