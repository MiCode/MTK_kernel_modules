/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"

#include "../gps_dl_hw_priv_util.h"

void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_gps_info(void)
{
	unsigned int flag;
	const struct gps_dl_hw_host_csr_dump_range *p_range;
	int i, j;

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR,
		BMASK_RW_FORCE_PRINT);

	for (i = 0; i < g_gps_v06x_host_csr_dump_range_num; i++) {
		p_range = &g_gps_v06x_host_csr_dump_range_ptr[i];
		for (j = 0; j < p_range->len; j++) {
			flag = j + p_range->flag_start;
			gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
				CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, flag,
				BMASK_RW_FORCE_PRINT);
			gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
				CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR,
				BMASK_RW_FORCE_PRINT);
		}
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

