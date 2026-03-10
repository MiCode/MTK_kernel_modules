/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"
#include "gps_dl_hw_atf.h"

#include "../gps_dl_hw_priv_util.h"

void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len)
{
	struct arm_smccc_res res;
	unsigned int flag, atf_ret, flag2, out;
	#define HOST_CSR_PRINT_LINE_MAX (8)
	unsigned int print_list[HOST_CSR_PRINT_LINE_MAX];
	unsigned int print_flag;
	unsigned int non_print_cnt;

	non_print_cnt = 0;
	memset(&print_list[0], 0, sizeof(print_list));

	for (flag = flag_start; flag < (flag_start + len); flag++) {
		if (non_print_cnt >= HOST_CSR_PRINT_LINE_MAX) {
			GDL_LOGW("flag=0x%x,cnt=%d,out=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x",
				print_flag, non_print_cnt,
				print_list[0], print_list[1], print_list[2], print_list[3],
				print_list[4], print_list[5], print_list[6], print_list[7]);
			non_print_cnt = 0;
			memset(&print_list[0], 0, sizeof(print_list));
		}

		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_DUMP_HOST_CSR_GPS_INFO_OPID,
			flag, 0, 0, 0, 0, 0, &res);
		atf_ret = res.a0;
		flag2 = res.a1;
		out = res.a2;
		if (flag != flag2 || atf_ret != 0)
			GDL_LOGW("atf_ret=%d, flag=0x%08x,0x%08x, out=0x%08x", atf_ret, flag, flag2, out);

		if (non_print_cnt == 0)
			print_flag = flag;
		print_list[non_print_cnt++] = out;
	}

	if (non_print_cnt != 0) {
		GDL_LOGW("flag=0x%x,cnt=%d,out=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x",
			print_flag, non_print_cnt,
			print_list[0], print_list[1], print_list[2], print_list[3],
			print_list[4], print_list[5], print_list[6], print_list[7]);
	}
}

void gps_dl_hw_dep_dump_host_csr_gps_info(void)
{
	int i;
	const struct gps_dl_hw_host_csr_dump_range *p_range;

	for (i = 0; i < g_gps_v06x_host_csr_dump_range_num; i++) {
		p_range = &g_gps_v06x_host_csr_dump_range_ptr[i];
		gps_dl_hw_dep_dump_host_csr_range(p_range->flag_start, p_range->len);
	}
}

void gps_dl_hw_dep_dump_host_csr_conninfra_info(void)
{
	/* TODO */
}

void gps_dl_hw_dep_may_do_bus_check_and_print(unsigned int host_addr)
{
	/* TODO */
}

void gps_dl_hw_gps_dump_gps_rf_temp_cr(void)
{
	/* TODO */
}

