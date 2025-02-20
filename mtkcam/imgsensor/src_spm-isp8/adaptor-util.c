// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#include "adaptor-util.h"

void adaptor_log_buf_init(struct adaptor_log_buf *buf, size_t sz)
{
	if (buf) {
		buf->buf_sz = sz;
		buf->remind = buf->buf_sz;
		buf->buf = kzalloc(sz + 1, GFP_KERNEL);
	}
}

void adaptor_log_buf_deinit(struct adaptor_log_buf *buf)
{
	if (buf) {
		buf->buf_sz = 0;
		buf->remind = 0;
		kfree(buf->buf);
	}
}

void adaptor_log_buf_flush(struct adaptor_ctx *ctx, const char *caller,
			struct adaptor_log_buf *buf)
{
	if (ctx && buf) {
		if (!caller)
			caller = "unknown";

		adaptor_logi(ctx, "%s: %s\n", caller, buf->buf);
		memset(buf->buf, 0, buf->buf_sz);
		buf->remind = buf->buf_sz;
	}
}

int adaptor_log_buf_gather(struct adaptor_ctx *ctx, const char *caller,
			struct adaptor_log_buf *buf,
			char *fmt, ...)
{
	char *strptr = NULL;
	int num = 0;
	va_list args;

	if (!buf || !(buf->buf))
		return -1;

	if (!caller)
		caller = "unknown";

	va_start(args, fmt);

	strptr = buf->buf + (buf->buf_sz - buf->remind);
	num = vsnprintf(strptr, buf->remind, fmt, args);
	if (num <= 0) {
		adaptor_logi(ctx, "%s: snprintf return negative\n", caller);
		va_end(args);
		return -1;
	}

	buf->remind -= num;

	va_end(args);

	return 0;
}

