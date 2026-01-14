/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#ifdef MET_GPU_MEM_MONITOR
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_memory_usage,unsigned int *pMemUsage)
#endif

#ifdef MET_GPU_PMU_MONITOR
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_pmu_init,struct GPU_PMU *pmus, int pmu_size, int *ret_size)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_pmu_swapnreset,struct GPU_PMU *pmus, int pmu_size)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_pmu_deinit,void)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_pmu_swapnreset_stop,void)
#endif

#ifdef MET_GPU_PWR_MONITOR
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_power_loading,unsigned int)
#endif
