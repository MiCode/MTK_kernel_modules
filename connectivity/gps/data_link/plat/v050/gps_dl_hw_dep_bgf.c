/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_context.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"
#include "gps_dl_hal.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
#include "conninfra.h"
#elif GPS_DL_ON_CTP
#include "conninfra_ext.h"
#endif
#endif
#include "../gps_dl_hw_priv_util.h"
#include "conn_infra/conn_infra_bus_cr.h"
#include "conn_infra/conn_infra_cfg_on.h"
#include "conn_infra/conn_infra_cfg.h"

void gps_dl_hw_dep_gps_sw_request_peri_usage(bool request)
{
	/* Do nothing for MT6983 */
}

bool gps_dl_hw_dep_en_gps_func_and_poll_bgf_ack(void)
{
	bool poll_okay = false;

	/* bit24: BGFSYS_ON_TOP primary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_on_1st_pwr_ack_not_okay");
		goto _fail_bgf_top_1st_pwr_ack_not_okay;
	}

	/* bit25: BGFSYS_ON_TOP secondary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_AN_BGFSYS_ON_TOP_PWR_ACK_S, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_on_2nd_pwr_ack_not_okay");
		goto _fail_bgf_top_2nd_pwr_ack_not_okay;
	}

	/* Enable GPS function */
	GDL_HW_SET_GPS_FUNC_EN(1);

	/* bit24: BGFSYS_OFF_TOP primary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_off_1st_pwr_ack_not_okay");
		goto _fail_bgf_top_1st_pwr_ack_not_okay;
	}

	/* bit25: BGFSYS_OFF_TOP secondary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_S, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_off_2nd_pwr_ack_not_okay");
		goto _fail_bgf_top_2nd_pwr_ack_not_okay;
	}
	return true; /* okay */

_fail_bgf_top_1st_pwr_ack_not_okay:
_fail_bgf_top_2nd_pwr_ack_not_okay:
	return false; /* fail */
}


bool gps_dl_hw_dep_poll_bgf_bus_and_gps_top_ack(void)
{
	bool poll_okay = false;
	unsigned int poll_ver_v051;
	int i;

	/* 0x18c21010[31:0] bgf ip version */
	GDL_HW_CHECK_BGF_IP_VER(&poll_okay, &poll_ver_v051);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_ip_ver_not_okay");
		goto _fail_bgf_ip_ver_not_okay;
	}

	/* 0x18c21014[7:0] bgf ip cfg */
	GDL_HW_POLL_GPS_ENTRY(BGF_GPS_CFG_BGF_IP_CONFIG_BGFSYS_CONFIG, 0,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_ip_cfg_not_okay");
		goto _fail_bgf_ip_cfg_not_okay;
	}

	/* 0x18c0102c[3:0] == 4h'2 gps_top_off is GPS_ACTIVE state */
	for (i = 0; i < 3; i++) {
		GDL_HW_POLL_GPS_ENTRY(BGF_GPS_CFG_ON_GPS_TOP_OFF_PWR_CTL_GPS_TOP_OFF_PWR_CTL_CS, 2,
			POLL_DEFAULT, &poll_okay);
		if (poll_okay)
			break;
		/*
		 * TODO:
		 * if (!gps_dl_reset_level_is_none()) break;
		 */
		if (i > 0)
			GDL_LOGW("_poll_gps_top_off_active, cnt = %d", i + 1);
	}
	if (!poll_okay) {
		GDL_LOGE("_fail_gps_top_off_active_not_okay");
		goto _fail_gps_top_off_active_not_okay;
	}
	return true;

_fail_gps_top_off_active_not_okay:
_fail_bgf_ip_ver_not_okay:
_fail_bgf_ip_cfg_not_okay:
	return false;
}

bool gps_dl_hw_dep_may_enable_bpll(void)
{
	/*MT6983 do nothing*/
	/*avoid return check, return true*/
	return true;
}


void gps_dl_hw_dep_may_disable_bpll(void)
{
	/*MT6983 do nothing*/
}

void gps_dl_hw_dep_may_set_bus_debug_flag(void)
{
	/* Do nothing for MT6983 */
}

void gps_dl_hw_dep_may_remap_conn2ap_gps_peri(void)
{
	/* MT6983 CONN-GPS need to access 0x1c00_xxxx and 0x1c01_xxxx */
	gps_dl_hw_set_gps_peri_remapping(0x01c00);
}

bool gps_dl_hw_dep_may_check_conn_infra_restore_done(void)
{
	bool poll_okay = false;
	int i;

	/* 0x18001210[16] == 1b'1 conn_infra cmdbt restore done, polling 10 times at most, interval = 0.5ms */
	for (i = 0; i < 10; i++) {
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY, 1,
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
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS, 1);
	else
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_SW_CONN_SRCCLKENA_GPS, 0);
}

void gps_dl_hw_dep_gps_sw_request_emi_usage(bool request)
{
	bool show_log = false;
	bool reg_rw_log = false;

#if GPS_DL_ON_LINUX
	reg_rw_log = gps_dl_log_reg_rw_is_on(GPS_DL_REG_RW_EMI_SW_REQ_CTRL);
#endif
	if (reg_rw_log) {
		show_log = gps_dl_set_show_reg_rw_log(true);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_TOP_ADDR);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_WF_ADDR);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_BT_ADDR);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_GPS_ADDR);
	}
#if (GPS_DL_USE_TIA && GPS_DL_USE_TOP_EMI_REQ_FOR_TIA)
	/* If use TIA, CONN_INFRA_CFG_EMI_CTL_GPS used by DSP, driver use TOP's. */
	if (request)
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_TOP_EMI_REQ_TOP, 1);
	else {
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_TOP_EMI_REQ_TOP, 1);
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_TOP_EMI_REQ_TOP, 0);
	}
#else
	if (request)
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_EMI_REQ_GPS, 1);
	else {
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_EMI_REQ_GPS, 1);
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_EMI_REQ_GPS, 0);
	}
#endif
	if (reg_rw_log)
		gps_dl_set_show_reg_rw_log(show_log);

}

int gps_dl_hw_gps_sleep_prot_ctrl(int op)
{
	bool poll_okay = false;

	if (1 == op) {
		/* disable when on */
		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps rx");
			goto _fail_disable_gps_slp_prot;
		}

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
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
			goto _fail_disable_gps_slp_prot;
		}
		return 0;

_fail_disable_gps_slp_prot:
#if 0
		GDL_HW_WR_CONN_INFRA_REG(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_ADDR,
			CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN_MASK |
			CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN_MASK |
			CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN_MASK |
			CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN_MASK);
#endif
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

		return 0;

_fail_enable_gps_slp_prot:
		/* trigger reset on outer function */
#if 0
		gps_dl_trigger_connsys_reset();
#endif
		return -1;
	}

	return 0;

}

bool gps_dl_hw_gps_common_on_inner(void)
{
	bool poll_okay = false;
	unsigned int adie_ver;

	gps_dl_hw_dep_may_remap_conn2ap_gps_peri();

	/*set gps emi remap here*/
	gps_dl_emi_remap_calc_and_set();

	/* Enable Conninfra BGF */
	GDL_HW_SET_CONN_INFRA_BGF_EN(1);

#if GPS_DL_HAS_CONNINFRA_DRV
	adie_ver = conninfra_get_ic_info(CONNSYS_ADIE_CHIPID);
	if (!(adie_ver == 0x6637 || adie_ver == 0x6635 || adie_ver == 0x6631 ||
		adie_ver == 0x6686)) {
		GDL_LOGE("_fail_adie_ver_not_okay, adie_ver = 0x%08x", adie_ver);
		goto _fail_adie_ver_not_okay;
	}

#if GPS_DL_DO_ADIE2_ACTION
	/*
	* mt6878 has 2 adie, check adie
	* adie_ver==0x6631 from conninfra_get_ic_info(CONNSYS_ADIE_CHIPID)
	* has 2 cases:
	* MT6631 for BT/WIFI/GNSS
	* MT6631 for BT/WIFI and MT6686 for GNSS
	* so we need to check conninfra_get_ic_info(CONNSYS_GPS_ADIE_CHIPID) again for GNSS part
	*/
	if (gps_dl_hal_get_conn_infra_ver() == GDL_HW_CONN_INFRA_VER_MT6878) {
		if (adie_ver == 0x6631) {
			adie_ver = conninfra_get_ic_info(CONNSYS_GPS_ADIE_CHIPID);
			if (adie_ver == 0x6686)
				adie_ver = 0x6686;
		}
	}
#endif

	gps_dl_hal_set_adie_ver(adie_ver);
#endif

	GDL_LOGW("adie_ver = 0x%08x is ok", adie_ver);

#if GPS_DL_ON_CTP
	/* Request EMI anyway */
	gps_dl_hw_gps_sw_request_emi_usage(true);
#elif GPS_DL_ON_LINUX
	/* Will not request EMI until data routing */
	gps_dl_hal_emi_usage_init();
#endif
	gps_dl_hw_gps_sw_request_peri_usage(true);

	/* L1 infra request, only for mt6983\6879\... */
	gps_dl_hw_dep_may_set_conn_infra_l1_request(true);

	poll_okay = gps_dl_hw_dep_en_gps_func_and_poll_bgf_ack();
	if (!poll_okay)
		goto _fail_bgf_top_pwr_ack_not_okay;

	GDL_WAIT_US(200);

	/* sleep prot */
	if (gps_dl_hw_gps_sleep_prot_ctrl(1) != 0) {
		GDL_LOGE("_fail_disable_gps_slp_prot_not_okay");
		goto _fail_disable_gps_slp_prot_not_okay;
	}

	/* polling status and version */
	poll_okay = gps_dl_hw_dep_poll_bgf_bus_and_gps_top_ack();
	if (!poll_okay)
		goto _fail_bgf_bus_or_gps_top_pwr_ack_not_okay;

#if GPS_DL_DO_ADIE2_ACTION
	if (adie_ver == 0x6686)
		gps_dl_hw_dep_gps_control_adie_on_6878();
#endif

	return true;
_fail_bgf_bus_or_gps_top_pwr_ack_not_okay:
_fail_disable_gps_slp_prot_not_okay:
_fail_bgf_top_pwr_ack_not_okay:
	GDL_HW_SET_GPS_FUNC_EN(0);

#if GPS_DL_HAS_CONNINFRA_DRV
_fail_adie_ver_not_okay:
#endif
	return false;
}

unsigned int gps_dl_hw_gps_get_adie_id_from_conninfra(void)
{
	unsigned int adie_ver  = 0xFF;
	unsigned int chip_ver  = 0xFF;

#if GPS_DL_HAS_CONNINFRA_DRV
	adie_ver = conninfra_get_ic_info(CONNSYS_ADIE_CHIPID);
	if (!(adie_ver == 0x6637 || adie_ver == 0x6635 || adie_ver == 0x6631 ||
		adie_ver == 0x6686)) {
		GDL_LOGE("_fail_adie_ver_not_okay, adie_ver = 0x%08x", adie_ver);
		return -1;
	}

	chip_ver = conninfra_get_ic_info(CONNSYS_SOC_CHIPID);

#if GPS_DL_DO_ADIE2_ACTION
	/*
	* mt6878 has 2 adie, check adie
	* adie_ver==0x6631 from conninfra_get_ic_info(CONNSYS_ADIE_CHIPID)
	* has 2 cases:
	* MT6631 for BT/WIFI/GNSS
	* MT6631 for BT/WIFI and MT6686 for GNSS
	* so we need to check conninfra_get_ic_info(CONNSYS_GPS_ADIE_CHIPID) again for GNSS part
	*/
	if (chip_ver == 0x6878) {
		if (adie_ver == 0x6631) {
			adie_ver = conninfra_get_ic_info(CONNSYS_GPS_ADIE_CHIPID);
			if (adie_ver == 0x6686)
				adie_ver = 0x6686;
		}
	}
#endif
#endif
	return adie_ver;
}

