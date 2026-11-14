// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mtk_gpufreq.h>

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include "mtk_platform_logbuffer.h"
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

static char *infra_status_logbuf;

void mtk_debug_dump_infra_status_init(void)
{
    infra_status_logbuf = kmalloc(GPUFREQ_DUMP_INFRA_SIZE, GFP_KERNEL);
}

void mtk_debug_dump_infra_status_term(void)
{
    kfree(infra_status_logbuf);
}

void mtk_debug_dump_infra_status(struct kbase_device *kbdev)
{
	char *sep_logbuf = NULL, *token = NULL;
	const char *delim = "\n";
	int len = 0;
	int i = 0;

	if (kbase_io_is_gpu_powered(kbdev)) {
#if defined(CONFIG_MTK_GPUFREQ_V2)
		if (infra_status_logbuf) {
			memset((void *)infra_status_logbuf, 0, GPUFREQ_DUMP_INFRA_SIZE);
			gpufreq_dump_infra_status_logbuffer(infra_status_logbuf, &len, GPUFREQ_DUMP_INFRA_SIZE);

			sep_logbuf = infra_status_logbuf;
			token = strsep(&sep_logbuf, delim);
			while (token != NULL) {
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev,
					MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "%s\n", token);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
				token = strsep(&sep_logbuf, delim);
			}
		} else
			gpufreq_dump_infra_status();
#else
		mt_gpufreq_dump_infra_status();
#endif /* CONFIG_MTK_GPUFREQ_V2 */
	}
}
