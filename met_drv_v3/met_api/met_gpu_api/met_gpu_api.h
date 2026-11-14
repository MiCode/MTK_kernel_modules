/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#ifdef MET_GPU_LOAD_MONITOR
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_loading,unsigned int *pLoading)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_block,unsigned int *pBlock)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_idle,unsigned int *pIdle)
#endif

#ifdef MET_GPU_DVFS_MONITOR
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_cur_freq,unsigned int *puiFreq)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_cur_oppidx,int *piIndex)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_floor_index,int *piIndex)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_ceiling_index,int *piIndex)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_floor_limiter,unsigned int *puiLimiter)
EXTERNAL_SYMBOL_FUNC(bool,mtk_get_gpu_ceiling_limiter,unsigned int *puiLimiter)
#endif
