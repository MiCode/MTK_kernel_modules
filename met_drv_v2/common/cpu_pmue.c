/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "met_drv.h"
#include "interface.h"
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "sspm_met_ipi_handle.h"
#endif
#endif

static const char cpu_pmue_header[] = "met-info [000] 0.0: pmu_sampler: mcupm\n";

static int cpu_pmu_print_header(char *buf, int len)
{
	if (met_cpu_pmue.ondiemet_mode == 1)
		return snprintf(buf, len, cpu_pmue_header);
	return 0;
}

static void sspm_pmu_unique_start(void) {
	if (met_cpu_pmue.ondiemet_mode == 1)
		ondiemet_module[ONDIEMET_SSPM] |= ID_CPU_PMUE;
}

struct metdevice met_cpu_pmue = {
	.name = "cpu_pmue",
	.type = MET_TYPE_PMU,
	.cpu_related = 1,
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
	.ondiemet_mode = 1,
	.ondiemet_print_header = cpu_pmu_print_header,
	.uniq_ondiemet_start = sspm_pmu_unique_start,
#endif
#endif
};

