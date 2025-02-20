/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#if IS_ENABLED(CONFIG_MTK_DRAMC)
EXTERNAL_SYMBOL_FUNC(unsigned int,mtk_dramc_get_data_rate,void)
EXTERNAL_SYMBOL_FUNC(unsigned int,mtk_dramc_get_ddr_type,void)
#elif IS_ENABLED(CONFIG_MTK_DRAMC_LEGACY)
EXTERNAL_SYMBOL_FUNC(unsigned int,get_dram_data_rate,void)
#endif/* CONFIG_MTK_DRAMC */

#if IS_ENABLED(CONFIG_MTK_DVFSRC_MET) && IS_ENABLED(CONFIG_MTK_DVFSRC_HELPER)
EXTERNAL_SYMBOL_FUNC(int,get_cur_ddr_ratio,void)
#endif
