/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_context.h"
#include "gps_dl_hw_ver.h"
#include "gps_dl_hal.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
#include "conninfra.h"
#elif GPS_DL_ON_CTP
#include "conninfra_ext.h"
#endif
#endif
#include "conn_infra/conn_bus_cr.h"

#include "../gps_dl_hw_priv_util.h"
#include "conn_infra/conn_cfg_on.h"
#include "conn_infra/conn_cfg.h"
#include "gps_dl_hw_atf.h"
#include "gps_dl_subsys_reset.h"
#if	GPS_DL_GET_INFO_FROM_NODE
#include "gps_dl_info_node.h"
#endif

void gps_dl_hw_dep_gps_sw_request_peri_usage(bool request)
{
	/* Do nothing for MT6980 */
}

bool gps_dl_hw_dep_en_gps_func_and_poll_bgf_ack(void)
{
	bool poll_okay = false;

	/* bit18: BGFSYS_ON ISO_EN */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_CONNSYS_PWR_STATES_S, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_sys_on_iso_en_not_okay");
		goto _fail_bgf_sys_on_iso_en_not_okay;
	}

	/* bit19: BGFSYS_ON RESET */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_CONNSYS_PWR_STATES, 0,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_sys_on_reset_not_okay");
		goto _fail_bgf_sys_on_reset_not_okay;
	}

	/* Enable GPS function */
	GDL_HW_SET_GPS_FUNC_EN(1);

	return true; /* okay */

_fail_bgf_sys_on_reset_not_okay:
_fail_bgf_sys_on_iso_en_not_okay:

	return false; /* fail */
}


bool gps_dl_hw_dep_poll_bgf_bus_and_gps_top_ack(void)
{
	bool poll_okay = false;
	unsigned int poll_ver_v061;
	/*int i;*/

	/* 0x18c21010[31:0] bgf ip version */
	GDL_HW_CHECK_BGF_IP_VER(&poll_okay, &poll_ver_v061);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_ip_ver_not_okay");
		goto _fail_bgf_ip_ver_not_okay;
	}

	return true;

_fail_bgf_ip_ver_not_okay:
	return false;
}

bool gps_dl_hw_dep_may_enable_bpll(void)
{
	/*mt6980 do nothing*/
	/*avoid return check, return true*/
	return true;
}

void gps_dl_hw_dep_may_disable_bpll(void)
{
	/*mt6980 do nothing*/
}

void gps_dl_hw_dep_may_set_bus_debug_flag(void)
{
	/* Do nothing for MT6899 */
}

void gps_dl_hw_dep_may_remap_conn2ap_gps_peri(void)
{
	/* Bollinger CONN_GPS need to access 0x1200_xxxx and 0x1201_xxxx*/
	/* Conninfra set remap register already, no need*/
	/*gps_dl_hw_set_gps_peri_remapping(0x01c00);*/
}

bool gps_dl_hw_dep_may_check_conn_infra_restore_done(void)
{
	bool poll_okay = false;
	int i;

	/* 0x18001210[16] == 1b'1 conn_infra cmdbt restore done, */
	/*polling 10 times at most, interval = 0.5ms */
	for (i = 0; i < 10; i++) {
		GDL_HW_POLL_CONN_INFRA_ENTRY(
			CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY, 1,
			POLL_DEFAULT2, &poll_okay);
		if (poll_okay)
			return true;

		if (i > 0)
			GDL_LOGW("_poll_poll_conn_infra_cmdbt_restore_done, cnt = %d", i + 1);
	}
	if (!poll_okay)
		GDL_LOGE("_poll_poll_conn_infra_cmdbt_restore_not_done");

	return false;
}

void gps_dl_hw_dep_may_set_conn_infra_l1_request(bool request)
{
	if (request)
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1, 1);
	else
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_CFG_EMI_CTL_GPS_L1_INFRA_REQ_GPS_L1, 0);
}

bool gps_dl_hw_dep_may_set_gps_axi_sleep_prot_ctrl(unsigned int val)
{
	bool poll_okay = false;

	GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_RX_VAL(val);
	GDL_HW_POLL_GPS2CONN_AXi_SLP_PROT_RX_UNTIL_VAL(val, POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn rx");
		return false;
	}

	GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_TX_VAL(val);
	GDL_HW_POLL_GPS2CONN_AXi_SLP_PROT_TX_UNTIL_VAL(val, POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
		return false;
	}
	return true;
}

void gps_dl_hw_dep_gps_sw_request_emi_usage(bool request)
{
	bool reg_rw_log = false;
	struct arm_smccc_res res;
	int ret;

#if GPS_DL_ON_LINUX
	reg_rw_log = gps_dl_log_reg_rw_is_on(GPS_DL_REG_RW_EMI_SW_REQ_CTRL);
#endif

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_SW_REQUEST_EMI_USAGE_OPID,
			request, reg_rw_log, 0, 0, 0, 0, &res);
	ret = res.a0;
}

int gps_dl_hw_gps_sleep_prot_ctrl(int op)
{
	bool poll_okay = false;

	if (1 == op) {
		/* disable when on */
		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(0);
		GDL_HW_SET_CONN2GPS_SLP_PROT_DIS_RX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_DIS_RX_VAL(0);
		GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(0);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps tx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(0);
		GDL_HW_SET_GPS2CONN_SLP_PROT_DIS_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_DIS_TX_VAL(0);
		GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_GPS2CONN_AXi_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_TX_VAL(0);
		GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_DIS_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_AXi_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
			goto _fail_disable_gps_slp_prot;
		}
		GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_DIS_TX_VAL(0);

		return 0;

_fail_disable_gps_slp_prot:
		gps_dl_slp_prot_fail_and_dump();
		return -1;
	} else if (0 == op) {
		/* enable when off */
		GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps tx");
			goto _fail_enable_gps_slp_prot;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps rx");
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn tx");
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn rx");
			goto _fail_enable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_AXi_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
			goto _fail_enable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_AXi_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_GPS2CONN_AXi_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn rx");
			goto _fail_enable_gps_slp_prot;
		}

		return 0;

_fail_enable_gps_slp_prot:
		/* trigger reset on outer function */
		gps_dl_slp_prot_fail_and_dump();
		return -1;
	}

	return 0;
}

void gps_dl_hw_gps_set_adie_chipid_to_atf(unsigned int chipid)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_SET_ADIE_CHIPID_TO_ATF_OPID,
			chipid, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

int gps_dl_hw_gps_common_on_part3(int i)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_PART3_OPID,
			i, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_on_part4(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_PART4_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_on_inner_fail_handle(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_INNER_FAIL_HANDLER_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

bool gps_dl_hw_gps_common_on_inner(void)
{
	int poll_okay = -1;
	unsigned int adie_ver = 0;

	gps_dl_hw_gps_common_on_part3(1);

#if GPS_DL_HAS_CONNINFRA_DRV
	adie_ver = conninfra_get_ic_info(CONNSYS_ADIE_CHIPID);
	if (!(adie_ver == 0x6637 || adie_ver == 0x6635 || adie_ver == 0x6686)) {
		GDL_LOGE("_fail_adie_ver_not_okay, adie_ver = 0x%08x", adie_ver);
		goto _fail_adie_ver_not_okay;
	}
	gps_dl_hal_set_adie_ver(adie_ver);

	gps_dl_hw_gps_set_adie_chipid_to_atf(adie_ver);
#endif

	GDL_LOGW("adie_ver = 0x%08x is ok", adie_ver);

	poll_okay = gps_dl_hw_gps_common_on_part4();
	if (poll_okay == -1) {
		GDL_LOGE("_fail_gps_hw_common_on_part4_fail");
		goto _fail_gps_hw_common_on_part4_not_okay;
	}

	/*set gps emi remap here for Bollinger*/
	gps_dl_emi_remap_calc_and_set();

	poll_okay = (int)gps_dl_hw_dep_gps_control_adie_on();
	if (poll_okay == 0) {
		GDL_LOGE("_fail_gps_dl_hw_dep_gps_control_adie_on_not_okay");
		goto _fail_gps_dl_hw_dep_gps_control_adie_on_not_okay;
	}

#if GPS_DL_ON_CTP
	/* Request EMI anyway */
	gps_dl_hw_gps_sw_request_emi_usage(true);
#elif GPS_DL_ON_LINUX
	/* Will not request EMI until data routing */
	gps_dl_hal_emi_usage_init();
#endif
	return true;

_fail_gps_dl_hw_dep_gps_control_adie_on_not_okay:
_fail_gps_hw_common_on_part4_not_okay:
	gps_dl_hw_gps_common_on_inner_fail_handle();

#if GPS_DL_HAS_CONNINFRA_DRV
_fail_adie_ver_not_okay:
#endif
	return false;
}

void gps_dl_hw_dep_gps_control_adie_on_inner_1(bool if_on)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_CONTROL_ADIE_ON_1_OPID,
			if_on, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_gps_control_adie_on_inner_2(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_CONTROL_ADIE_ON_2_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

bool gps_dl_hw_dep_gps_control_adie_on(void)
{
	unsigned int conn_ver = 0;

	conn_ver = gps_dl_hal_get_conn_infra_ver();
	if (GDL_HW_CONN_INFRA_VER_MT6985 == conn_ver)
		return gps_dl_hw_dep_gps_control_adie_on_6985();
	else if (GDL_HW_CONN_INFRA_VER_MT6989 == conn_ver)
		return gps_dl_hw_dep_gps_control_adie_on_6989();

	GDL_LOGW("unknown conn ver 0x%08x, do not open adie", conn_ver);
	return false;
}

void gps_dl_hw_dep_adie_mt6686_dump_status(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int adie_03C = 0, adie_B18 = 0, adie_750 = 0;
	int rd_status;

	/* READ TOP: 0x03C */
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0x03C, &adie_03C);
	ASSERT_ZERO(rd_status, GDL_VOIDF());

	/* READ TOP: 0xB18 */
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xB18, &adie_B18);
	ASSERT_ZERO(rd_status, GDL_VOIDF());

	/* READ TOP: 0x750 */
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0x750, &adie_750);
	ASSERT_ZERO(rd_status, GDL_VOIDF());

	GDL_LOGW("adie_dump, 03C=0x%x, B18=0x%x, 750=0x%x",
		adie_03C, adie_B18, adie_750);
#endif
}

void gps_dl_hw_dep_gps_control_adie_off(void)
{
	unsigned int conn_ver = 0;

	conn_ver = gps_dl_hal_get_conn_infra_ver();

	if (GDL_HW_CONN_INFRA_VER_MT6985 == conn_ver)
		gps_dl_hw_dep_gps_control_adie_off_6985();
	else if (GDL_HW_CONN_INFRA_VER_MT6989 == conn_ver)
		gps_dl_hw_dep_gps_control_adie_off_6989();
	else
		GDL_LOGW("unknown conn ver 0x%08x, do not close adie", conn_ver);
}

bool gps_dl_hw_dep_gps_get_ecid_info(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int ecid_lsb = 0, ecid_msb = 0;
	unsigned long ecid_data = 0, ecid_bak = 0;
	unsigned int macro_sel;
	bool print_ecid_1st = true;
#endif

#if GPS_DL_HAS_CONNINFRA_DRV
twice_get_ecid:
	macro_sel = (print_ecid_1st)?(0x0000030D):(0x0000050D);
	if (conninfra_spi_write(SYS_SPI_TOP, 0x144, macro_sel) != 0) {
		GDL_LOGI_RRW("conninfra_spi_write_ecid_macro_sel not okay");
		goto _fail_conninfra_spi_write_ecid_macro_sel_not_okay;
	}
	if (conninfra_spi_write(SYS_SPI_TOP, 0x108, 0x40000040) != 0) {
		GDL_LOGI_RRW("conninfra_spi_write_ecid_mode_trig not okay");
		goto _fail_conninfra_spi_write_ecid_mode_trig_not_okay;
	}

	gps_dl_wait_us(20);

	if (conninfra_spi_read(SYS_SPI_TOP, 0x130, &ecid_lsb) != 0) {
		GDL_LOGI_RRW("conninfra_spi_read_ecid_lsb_data not okay");
		goto _fail_conninfra_spi_read_ecid_lsb_data_not_okay;
	}
	if (conninfra_spi_read(SYS_SPI_TOP, 0x134, &ecid_msb) != 0) {
		GDL_LOGI_RRW("conninfra_spi_read_ecid_hsb_data not okay");
		goto _fail_conninfra_spi_read_ecid_hsb_data_not_okay;
	}
	ecid_data = ecid_msb;
	ecid_data = ecid_data << 32 | ecid_lsb;
	GDL_LOGI("[MT6686P_ECID] : 0x%lx", ecid_data);
	if (print_ecid_1st) {
		print_ecid_1st = false;
		ecid_bak = ecid_data;
		goto twice_get_ecid;
	}
#endif
#if	GPS_DL_GET_INFO_FROM_NODE
	gps_dl_info_node_set_ecid_info(ecid_bak, ecid_data);
#endif
	return true;

#if GPS_DL_HAS_CONNINFRA_DRV
_fail_conninfra_spi_read_ecid_hsb_data_not_okay:
_fail_conninfra_spi_read_ecid_lsb_data_not_okay:
_fail_conninfra_spi_write_ecid_mode_trig_not_okay:
_fail_conninfra_spi_write_ecid_macro_sel_not_okay:
#endif
	return false;
}

unsigned int gps_dl_hw_gps_get_adie_id_from_conninfra(void)
{
	unsigned int adie_ver = 0xFF;

#if GPS_DL_HAS_CONNINFRA_DRV
	adie_ver = conninfra_get_ic_info(CONNSYS_ADIE_CHIPID);
	if (!(adie_ver == 0x6637 || adie_ver == 0x6635 || adie_ver == 0x6686)) {
		GDL_LOGE("_fail_adie_ver_not_okay, adie_ver = 0x%08x", adie_ver);
		return 0xFF;
	}
#endif

	return adie_ver;
}

