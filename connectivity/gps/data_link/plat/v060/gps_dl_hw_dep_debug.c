/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_osal.h"
#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"
#include "../gps_dl_hw_priv_util.h"
#include "mcudl/mcu_sys/conn_mcu_confg_on.h"

void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len)
{
	unsigned int flag, out;
	#define HOST_CSR_PRINT_LINE_MAX (8)
	unsigned int print_list[HOST_CSR_PRINT_LINE_MAX];
	unsigned int print_flag;
	unsigned int non_print_cnt;

	non_print_cnt = 0;
	gps_dl_osal_memset(&print_list[0], 0, sizeof(print_list));

	for (flag = flag_start; flag < (flag_start + len); flag++) {
		if (non_print_cnt >= HOST_CSR_PRINT_LINE_MAX) {
			GDL_LOGW("flag=0x%x,cnt=%d,out=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x",
				print_flag, non_print_cnt,
				print_list[0], print_list[1], print_list[2], print_list[3],
				print_list[4], print_list[5], print_list[6], print_list[7]);
			non_print_cnt = 0;
			gps_dl_osal_memset(&print_list[0], 0, sizeof(print_list));
		}

		gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, flag, 0);
		out = gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR, 0);

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
#if 0
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
#else
	int i;
	const struct gps_dl_hw_host_csr_dump_range *p_range;

	for (i = 0; i < g_gps_v06x_host_csr_dump_range_num; i++) {
		p_range = &g_gps_v06x_host_csr_dump_range_ptr[i];
		gps_dl_hw_dep_dump_host_csr_range(p_range->flag_start, p_range->len);
	}
#endif
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

void gps_dl_hw_dep_gps_dump_power_state(struct gps_dl_power_raw_state *p_raw)
{
#define BGF_LP_DBG_DUMP_LEN (5)
	unsigned int is_fw_own = 0;
	unsigned int conn_wake_by_top = 0, conn_wake_by_gps = 0;
	unsigned int clock_det = 0;
	unsigned int conn_pwr_st = 0;

	unsigned int bgf_dummy = 0, bgf_dummy2 = 0;
	unsigned int bgf_dbg_30004a = 0, bgf_dbg_30004b = 0;
	unsigned int bgf_dbg_300040[BGF_LP_DBG_DUMP_LEN] = {0};
	unsigned int i;

	unsigned int pc1, pc2, pc3, pc4, not_rst;
	unsigned int lp_status, lp_status2;

	is_fw_own = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR);
	conn_wake_by_top = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR);
	conn_wake_by_gps = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_ADDR);

	clock_det = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_CLOCK_DETECT_ADDR);
	conn_pwr_st = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR);

	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x200c00);
	bgf_dummy = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	GDL_HW_WR_CONN_INFRA_REG(
		CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, 0x80040000);
	bgf_dummy2 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);

	for (i = 0; i < BGF_LP_DBG_DUMP_LEN; i++) {
		GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, (0x300040 + i));
		bgf_dbg_300040[i] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	}
	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x30004a);
	bgf_dbg_30004a = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x30004b);
	bgf_dbg_30004b = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);

	GDL_LOGI(
		"fo=%u,wk:t/g=%d/%d,clk/pwr=0x%x,0x%x,dmy=0x%x,0x%x,dbg[0-4;a-b]=0x%x,0x%x,0x%x,0x%x,0x%x;0x%x,0x%x",
		is_fw_own, conn_wake_by_top, conn_wake_by_gps, clock_det, conn_pwr_st,
		bgf_dummy, bgf_dummy2, bgf_dbg_300040[0], bgf_dbg_300040[1], bgf_dbg_300040[2], bgf_dbg_300040[3],
		bgf_dbg_300040[4], bgf_dbg_30004a, bgf_dbg_30004b);

	not_rst = GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_RGU_ON_GPSSYS_CPU_SW_RST_B_GPSSYS_CPU_SW_RST_B);
	GDL_HW_WR_CONN_INFRA_REG(
		CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, 0xC0040103);
	pc1 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	pc2 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	pc3 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	pc4 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);

	/* Direct reading it from register gets 0xDEADFEED when MCU sleep,
	 * reading from host_csr can avoid this.
	 * lp_status = GDL_HW_RD_GPS_REG(CONN_MCU_CONFG_ON_HOST_MAILBOX_MCU_ADDR);
	 */
	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x300d43);
	lp_status = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	lp_status2 = GDL_HW_RD_GPS_REG(CONN_MCU_CONFG_ON_HOST_MAILBOX_MCU_ADDR);

	GDL_LOGW("nr=%d, pc=0x%08X,0x%08X,0x%08X,0x%08X, lp=0x%08X,0x%08X",
		not_rst, pc1, pc2, pc3, pc4, lp_status, lp_status2);
	if (p_raw != NULL) {
		p_raw->mcu_pc = pc1;
		/* bit4(+16) and bit2(+16) are L5/L1 osc_en */
		p_raw->is_hw_clk_ext = ((bgf_dbg_30004a & 0x0014) != 0);
		p_raw->sw_gps_ctrl = (bgf_dummy & 0xFFFF);
	}
}
