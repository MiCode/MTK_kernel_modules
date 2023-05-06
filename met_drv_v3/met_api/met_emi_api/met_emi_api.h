/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#if IS_ENABLED(CONFIG_MTK_DRAMC)
EXTERNAL_SYMBOL_FUNC(unsigned int,mtk_dramc_get_data_rate,void)
EXTERNAL_SYMBOL_FUNC(unsigned int,mtk_dramc_get_ddr_type,void)
#endif

#if IS_ENABLED(CONFIG_MTK_DVFSRC_MET)
EXTERNAL_SYMBOL_FUNC(int,get_cur_ddr_ratio,void)
#endif
