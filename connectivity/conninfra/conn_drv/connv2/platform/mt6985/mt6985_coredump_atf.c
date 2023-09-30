// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../../debug_utility/coredump/coredump_mng.h"
#include "../include/connsys_smc.h"
#include "../include/consys_hw.h"
#include "include/mt6985_atf.h"
#include "include/mt6985_coredump.h"

struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6985_atf = {
	.consys_coredump_get_platform_config = consys_plt_coredump_get_platform_config_mt6985,
	.consys_coredump_get_platform_chipid = consys_plt_coredump_get_platform_chipid_mt6985,
	.consys_coredump_get_task_string = consys_plt_coredump_get_task_string_mt6985,
	.consys_coredump_get_sys_name = consys_plt_coredump_get_sys_name_mt6985,
	.consys_coredump_is_host_view_cr = consys_plt_coredump_is_host_view_cr_mt6985,
	.consys_coredump_is_host_csr_readable = consys_plt_coredump_is_host_csr_readable_mt6985,
	.consys_coredump_get_cr_category = consys_plt_coredump_get_cr_category_mt6985,
	.consys_coredump_get_emi_offset = consys_plt_coredump_get_emi_offset_mt6985,
	.consys_coredump_get_emi_phy_addr = consys_plt_coredump_get_emi_phy_addr_mt6985,
	.consys_coredump_get_mcif_emi_phy_addr = consys_plt_coredump_get_mcif_emi_phy_addr_mt6985,
	.consys_coredump_setup_dump_region = consys_plt_coredump_setup_dump_region_mt6985_atf,
	.consys_coredump_setup_dynamic_remap = consys_plt_coredump_setup_dynamic_remap_mt6985_atf,
	.consys_coredump_remap = consys_plt_coredump_remap_mt6985,
	.consys_coredump_unmap = consys_plt_coredump_unmap_mt6985,
	.consys_coredump_get_tag_name = consys_plt_coredump_get_tag_name_mt6985,
};

int consys_plt_coredump_setup_dump_region_mt6985_atf(int conn_type)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SETUP_DUMP_REGION_OPID,
			     conn_type, 0, 0, 0, 0, 0, ret);
	return ret;
}
unsigned int consys_plt_coredump_setup_dynamic_remap_mt6985_atf(int conn_type, unsigned int idx,
	unsigned int base, unsigned int length)
{
	int ret;
	/*
	 * Cannot send address to ATF
	 * Send index of dump_regions instead
	 */
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SETUP_DYNAMIC_REMAP_OPID,
			     conn_type, idx, 0, 0, 0, 0, ret);
	return ret;
}
