// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#define CREATE_TRACE_POINTS
#include "adaptor-trace.h"

uint adaptor_trace_en;
module_param(adaptor_trace_en, uint, 0644);
MODULE_PARM_DESC(adaptor_trace_en, "adaptor_trace_en");


void __adaptor_systrace(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	trace_tracing_mark_write(fmt, &args);
	va_end(args);
}

bool inline adaptor_trace_enabled(void)
{
	return adaptor_trace_en;
}


