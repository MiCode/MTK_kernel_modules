/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

#if !(defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC))
EXTERNAL_SYMBOL_FUNC(phys_addr_t,gpueb_get_reserve_mem_phys,unsigned int id)
EXTERNAL_SYMBOL_FUNC(phys_addr_t,gpueb_get_reserve_mem_virt,unsigned int id)
EXTERNAL_SYMBOL_FUNC(phys_addr_t,gpueb_get_reserve_mem_size,unsigned int id)
#endif

EXTERNAL_SYMBOL_FUNC(void *,get_gpueb_ipidev,void)
EXTERNAL_SYMBOL_FUNC(int,gpueb_get_send_PIN_ID_by_name,char *)
EXTERNAL_SYMBOL_FUNC(int,gpueb_get_recv_PIN_ID_by_name,char *)
EXTERNAL_SYMBOL_FUNC(int,mtk_ipi_send_compl_to_gpueb, int ipi_id, int opt, void *data, int len, unsigned long timeout)
