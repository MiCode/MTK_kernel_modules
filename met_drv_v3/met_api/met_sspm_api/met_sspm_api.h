/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#if !(IS_ENABLED(CONFIG_MTK_GMO_RAM_OPTIMIZE) || IS_ENABLED(CONFIG_MTK_MET_MEM_ALLOC))
EXTERNAL_SYMBOL_FUNC(phys_addr_t,sspm_reserve_mem_get_phys,unsigned int id)
EXTERNAL_SYMBOL_FUNC(phys_addr_t,sspm_reserve_mem_get_virt,unsigned int id)
EXTERNAL_SYMBOL_FUNC(phys_addr_t,sspm_reserve_mem_get_size,unsigned int id)
#endif
