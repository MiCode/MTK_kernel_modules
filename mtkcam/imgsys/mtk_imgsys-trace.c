// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Johnson-CH Chiu <Johnson-CH.chiu@mediatek.com>
 *
 */

#define CREATE_TRACE_POINTS
#include "mtk_imgsys-trace.h"

int imgsys_ftrace_en;
module_param(imgsys_ftrace_en, int, 0644);


void __imgsys_systrace(const char *fmt, ...)
{
	char buf[IMGSYS_TRACE_LEN];
	va_list args;
	int len;

	memset(buf, ' ', sizeof(buf));
	va_start(args, fmt);
	len = vsnprintf(buf, (IMGSYS_TRACE_LEN -1), fmt, args);
	va_end(args);

	if (len >= IMGSYS_TRACE_LEN) {
        	pr_info("%s trace size(%d) over limit", __func__, len);
		len = IMGSYS_TRACE_LEN - 1;
		return;
	}
	trace_tracing_mark_write(buf);
}

bool imgsys_core_ftrace_enabled(void)
{
	return imgsys_ftrace_en;
}


