/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"

#include "../gps_dl_hw_priv_util.h"
#include "conn_infra/conn_dbg_ctl.h"
#include "gps/gps_aon_top.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
#include "conninfra.h"
#elif GPS_DL_ON_CTP
#include "conninfra_ext.h"
#endif
#endif
#include "gps_dl_hw_atf.h"

const struct gps_dl_hw_host_csr_dump_range g_gps_v05x_host_csr_dump_ranges[] = {
	{0xC0020000,  2}, /* gps_cfg_off_debug, 0x0000_470b */
	{0xC0020100,  9}, /* gps_rgu_off_debug */
	{0xC0020200,  1}, /* gps_clkgen_off_debug */
	{0xC0020400,  6}, /* gps_fabric_debug */
};
const struct gps_dl_hw_host_csr_dump_range * const g_gps_v05x_host_csr_dump_range_ptr =
	&g_gps_v05x_host_csr_dump_ranges[0];
const int g_gps_v05x_host_csr_dump_range_num = ARRAY_SIZE(g_gps_v05x_host_csr_dump_ranges);

const struct gps_dl_hw_host_csr_dump_range g_gps_v05x_bgf_on_dump_ranges[] = {
	{0x00300800,  13}, /* gps_cfg_on_debug */
	{0x00300840,  9}, /* gps_rgu_on_debug */
	{0x00300880,  4}, /* mtmcmos_debug */
};
const struct gps_dl_hw_host_csr_dump_range * const g_gps_v05x_bgf_on_dump_range_ptr =
	&g_gps_v05x_bgf_on_dump_ranges[0];
const int g_gps_v05x_bgf_on_dump_range_num = ARRAY_SIZE(g_gps_v05x_bgf_on_dump_ranges);


void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id)
{
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		BGF_GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_ON_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		BGF_GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_SOFT_RST_B_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		BGF_GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_GPS_L1_SLP_PWR_CTL_CS_ADDR,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		BGF_GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_ON_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		BGF_GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_SOFT_RST_B_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		BGF_GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_GPS_L5_SLP_PWR_CTL_CS_ADDR,
		BMASK_RW_FORCE_PRINT);

	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		GPS_AON_TOP_TCXO_MS_H_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		GPS_AON_TOP_TCXO_MS_L_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_GPS_BUS,
		GPS_AON_TOP_DSLEEP_CTL_ADDR,
		BMASK_RW_FORCE_PRINT);

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

		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_HOST_CSR_GPS_DBG_SEL_OPID,
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

static void gps_dl_hw_dep_set_bgf_on_dbg_sel(unsigned int flag_start, unsigned int len)
{
	struct arm_smccc_res res;
	unsigned int flag, atf_ret, flag2, out;
#define BGF_ON_PRINT_LINE_MAX (8)
	unsigned int print_list[BGF_ON_PRINT_LINE_MAX];
	unsigned int print_flag;
	unsigned int non_print_cnt;

	non_print_cnt = 0;
	memset(&print_list[0], 0, sizeof(print_list));

	for (flag = flag_start; flag < (flag_start + len); flag++) {
		if (non_print_cnt >= BGF_ON_PRINT_LINE_MAX) {
			GDL_LOGW("flag=0x%x,cnt=%d,out=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x",
				print_flag, non_print_cnt,
				print_list[0], print_list[1], print_list[2], print_list[3],
				print_list[4], print_list[5], print_list[6], print_list[7]);
			non_print_cnt = 0;
			memset(&print_list[0], 0, sizeof(print_list));
		}

		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_HOST_CSR2BGF_DBG_SEL_OPID,
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

	for (i = 0; i < g_gps_v05x_host_csr_dump_range_num; i++) {
		p_range = &g_gps_v05x_host_csr_dump_range_ptr[i];
		gps_dl_hw_dep_dump_host_csr_range(p_range->flag_start, p_range->len);
	}

	for (i = 0; i < g_gps_v05x_bgf_on_dump_range_num; i++) {
		p_range = &g_gps_v05x_bgf_on_dump_range_ptr[i];
		gps_dl_hw_dep_set_bgf_on_dbg_sel(p_range->flag_start, p_range->len);
	}
}

#if 0
static void gps_dl_hw_dump_host_csr_conninfra_info_inner(unsigned int selection, int n)
{
	/* TODO */
}
#endif

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
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int spi_data;
	int rd_status;

	/*GPS: read 0x500*/
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x500, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("normally dump spi_data(GPS:0x500) = 0x%x", spi_data);

	/*GPS: read 0x518*/
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x518, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("normally dump spi_data(GPS:0x518) = 0x%x", spi_data);
#else
	GDL_LOGE("no conninfra driver");
#endif
}

