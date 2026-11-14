/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#if !(defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC))
EXTERNAL_SYMBOL_FUNC(phys_addr_t,mcupm_reserve_mem_get_phys,unsigned int id)
EXTERNAL_SYMBOL_FUNC(phys_addr_t,mcupm_reserve_mem_get_virt,unsigned int id)
EXTERNAL_SYMBOL_FUNC(phys_addr_t,mcupm_reserve_mem_get_size,unsigned int id)
#endif

EXTERNAL_SYMBOL_FUNC(void *,get_mcupm_ipidev,void)
