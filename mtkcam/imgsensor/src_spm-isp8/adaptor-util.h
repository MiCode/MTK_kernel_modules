/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 */

#ifndef _ADAPTOR_UTIL_H__
#define _ADAPTOR_UTIL_H__

#include "adaptor.h"
#include <aee.h>

#define ADAPTOR_LOG_BUF_SZ 256

struct adaptor_log_buf {
	char *buf;
	size_t buf_sz;
	size_t remind;
};

void adaptor_log_buf_init(struct adaptor_log_buf *buf, size_t size);
void adaptor_log_buf_deinit(struct adaptor_log_buf *buf);
void adaptor_log_buf_flush(struct adaptor_ctx *ctx, const char *caller,
			struct adaptor_log_buf *buf);
int adaptor_log_buf_gather(struct adaptor_ctx *ctx, const char *caller,
			struct adaptor_log_buf *buf,
			char *fmt, ...);

/* AEE */
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#define WRAP_AEE_EXCEPTION(module, msg)					\
	aee_kernel_exception_api(__FILE__, __LINE__,			\
				 DB_OPT_DEFAULT | DB_OPT_FTRACE,	\
				 module, msg)

#else
#define WRAP_AEE_EXCEPTION(module, msg)	\
	WARN_ON(1, "<%s:%d> %s: %s\n", __FILE__, __LINE__, module, msg)

#endif //IS_ENABLED(CONFIG_MTK_AEE_FEATURE)

#endif
