// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuan-Jung Kuo <yuan-jung.kuo@mediatek.com>
 *
 */

#include "mtk_aie-trace.h"

int aie_sys_ftrace_en = 0;
module_param(aie_sys_ftrace_en, int, 0644);

static noinline int tracing_mark_write(const char *buf)
{
	trace_puts(buf);
	return 0;
}

void __aie_systrace(const char *fmt, ...)
{
	char buf[AIE_TRACE_LEN];
	va_list args;
	int len;

	memset(buf, ' ', sizeof(buf));
	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (unlikely(len < 0))
		return;
	else if (unlikely(len == AIE_TRACE_LEN))
		buf[AIE_TRACE_LEN - 1] = '\0';

	tracing_mark_write(buf);
}

bool aie_core_ftrace_enabled(void)
{
	return aie_sys_ftrace_en;
}