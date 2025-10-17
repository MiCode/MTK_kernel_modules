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
EXPORT_SYMBOL(__imgsys_systrace);

bool imgsys_core_ftrace_enabled(void)
{
	return imgsys_ftrace_en;
}
EXPORT_SYMBOL(imgsys_core_ftrace_enabled);

#define FTRACE_IMGSYS_QOF(name) \
	void ftrace_imgsys_qof_##name(const char *fmt, ...) \
	{ \
		struct va_format vaf; \
		va_list args; \
		va_start(args, fmt); \
		vaf.fmt = fmt; \
		vaf.va = &args; \
		trace_imgsys__qof_##name(&vaf); \
		va_end(args); \
	}

FTRACE_IMGSYS_QOF(mod0);
EXPORT_SYMBOL(ftrace_imgsys_qof_mod0);
FTRACE_IMGSYS_QOF(mod1);
EXPORT_SYMBOL(ftrace_imgsys_qof_mod1);
FTRACE_IMGSYS_QOF(mod2);
EXPORT_SYMBOL(ftrace_imgsys_qof_mod2);
FTRACE_IMGSYS_QOF(mod3);
EXPORT_SYMBOL(ftrace_imgsys_qof_mod3);
FTRACE_IMGSYS_QOF(mod4);
EXPORT_SYMBOL(ftrace_imgsys_qof_mod4);

#define FTRACE_IMGSYS_HWQOS(name) \
	void ftrace_imgsys_hwqos_##name(const char *fmt, ...) \
	{ \
		struct va_format vaf; \
		va_list args; \
		va_start(args, fmt); \
		vaf.fmt = fmt; \
		vaf.va = &args; \
		trace_imgsys__hwqos_##name(&vaf); \
		va_end(args); \
	}

FTRACE_IMGSYS_HWQOS(bwr);
EXPORT_SYMBOL(ftrace_imgsys_hwqos_bwr);

FTRACE_IMGSYS_HWQOS(bls);
EXPORT_SYMBOL(ftrace_imgsys_hwqos_bls);

FTRACE_IMGSYS_HWQOS(ostdl);
EXPORT_SYMBOL(ftrace_imgsys_hwqos_ostdl);

void ftrace_imgsys_hwqos_dbg_reg_read(u32 pa, u32 value)
{
	trace_imgsys__hwqos_dbg_reg_read(pa, value);
}
EXPORT_SYMBOL(ftrace_imgsys_hwqos_dbg_reg_read);
