/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "gps_dl_config.h"
#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "../gps_dl_hw_priv_util.h"
#include "gps_dl_hw_dep_macro.h"

void gps_dl_hw_dep_dump_host_csr_gps_info(void)
{
	int i;

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_ADDR,
		BMASK_RW_FORCE_PRINT);

	for (i = 0xA2; i <= 0xB7; i++) {
		gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_ADDR, i,
			BMASK_RW_FORCE_PRINT);
		gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_ADDR,
			BMASK_RW_FORCE_PRINT);
	}
}

static void gps_dl_hw_dump_host_csr_conninfra_info_inner(unsigned int selection, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		/* Due to RW_DO_CHECK might be enabled, not use
		 * GDL_HW_SET_CONN_INFRA_ENTRY and GDL_HW_GET_CONN_INFRA_ENTRY to avoid redundant print.
		 */
		gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_AO_DEBUGSYS_ADDR, selection,
			BMASK_RW_FORCE_PRINT);

		gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_CTRL_AO2SYS_OUT_ADDR,
			BMASK_RW_FORCE_PRINT);
		selection -= 0x10000;
	}
}

void gps_dl_hw_dep_dump_host_csr_conninfra_info(void)
{
	gps_dl_hw_dump_host_csr_conninfra_info_inner(0x000F0001, 15);
	gps_dl_hw_dump_host_csr_conninfra_info_inner(0x00030002, 3);
	gps_dl_hw_dump_host_csr_conninfra_info_inner(0x00040003, 4);
}

void gps_dl_hw_dep_may_do_bus_check_and_print(unsigned int host_addr)
{
	/* not do rw check because here is the checking */
	GDL_LOGI("for addr = 0x%08x", host_addr);

	gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_AO_DEBUGSYS_ADDR, 0x000D0001,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_CTRL_AO2SYS_OUT_ADDR,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_AO_DEBUGSYS_ADDR, 0x000B0001,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_CTRL_AO2SYS_OUT_ADDR,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_AO_DEBUGSYS_ADDR, 0x000A0001,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_CONN_INFRA_DEBUG_CTRL_AO2SYS_OUT_ADDR,
		BMASK_RW_FORCE_PRINT);
}

