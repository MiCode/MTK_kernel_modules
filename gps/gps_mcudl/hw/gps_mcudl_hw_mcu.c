/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_config.h"

#include "gps_mcudl_hw_mcu.h"
#include "gps_mcudl_hw_dep_macro.h"
#include "gps_mcudl_hw_priv_util.h"
#include "gps_dl_time_tick.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_dl_hal.h"

bool gps_mcudl_hw_conn_ver_and_wake_is_ok(void)
{
	bool poll_okay = false;
	unsigned int poll_ver;

	/* Poll conninfra hw version */
	GDL_HW_CHECK_CONN_INFRA_VER(&poll_okay, &poll_ver);
	if (!poll_okay) {
		GDL_LOGE("_fail_conn_hw_ver_not_okay, poll_ver = 0x%08x", poll_ver);
		goto _fail_conn_hw_ver_not_okay;
	}

#if GPS_DL_ON_LINUX
	gps_dl_hal_set_conn_infra_ver(poll_ver);
#endif

	/* Poll conninfra cmdbt restore done, 0.5ms * 10 */
	GDL_HW_POLL_CONN_INFRA_ENTRY(
		CONN_CFG_ON_CONN_INFRA_CFG_PWRCTRL1_CONN_INFRA_RDY, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_conn_cmdbt_restore_not_okay");
		goto _fail_conn_cmdbt_restore_not_okay;
	}
	return true;

_fail_conn_cmdbt_restore_not_okay:
_fail_conn_hw_ver_not_okay:
	return false;
}

bool gps_mcudl_hw_conn_force_wake(bool enable)
{
	bool poll_okay = false;

	if (enable) {
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS, 1);
		/* GDL_HW_MAY_WAIT_CONN_INFRA_SLP_PROT_DISABLE_ACK(&poll_okay);*/
		poll_okay = gps_mcudl_hw_conn_ver_and_wake_is_ok();
		if (!poll_okay) {
			GDL_LOGE("_fail_conn_ver_or_wake_not_okay");
			return false; /* not okay */
		}
	} else
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS, 0);

	return true;
}

static int gps_dl_hw_gps_sleep_prot_ctrl(int op)
{
	bool poll_okay = false;
	int ret;

	if (1 == op) {
		/* disable when on */
		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_DIS_VAL(1);
		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_DIS_VAL(0);
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

		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_DIS_VAL(1);
		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(0);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
			goto _fail_disable_gps_slp_prot;
		}

		/* AXI */
		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_DIS_VAL(0);
		GDL_HW_SET_GPS2CONN_AXI_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_GPS2CONN_AXI_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn_axi rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_AXI_SLP_PROT_TX_DIS_VAL(1);
		GDL_HW_SET_GPS2CONN_AXI_SLP_PROT_TX_VAL(0);
		GDL_HW_POLL_GPS2CONN_AXI_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn_axi tx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_AXI_SLP_PROT_TX_DIS_VAL(0);
		return 0;

_fail_disable_gps_slp_prot:
#if 0
		GDL_HW_WR_CONN_INFRA_REG(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_ADDR,
			CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN_MASK |
			CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN_MASK |
			CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN_MASK |
			CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN_MASK);
#endif
		gps_mcudl_hal_mcu_show_pc_log();
		gps_dl_slp_prot_fail_and_dump();
		return -1;
	} else if (0 == op) {
		ret = 0;
		/* enable when off */
		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn tx");
			ret = -1;
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn rx");
			ret = -1;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps tx");
			ret = -1;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps rx");
		}

		/* AXI */
		GDL_HW_SET_GPS2CONN_AXI_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_AXI_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn_axi tx");
			ret = -1;
			goto _fail_enable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_AXI_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_GPS2CONN_AXI_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn_axi rx");
			ret = -1;
			goto _fail_enable_gps_slp_prot;
		}

		return ret;

_fail_enable_gps_slp_prot:
		/* trigger reset on outer function */
#if 0
		gps_dl_trigger_connsys_reset();
#endif
		gps_mcudl_hal_mcu_show_pc_log();
		gps_dl_slp_prot_fail_and_dump();
		return -1;
	}
	return 0;
}

bool gps_mcudl_hw_mcu_do_on_with_rst_held(void)
{
	bool poll_okay = false;
	/*unsigned int poll_ver, adie_ver = 0;*/

	/* Set MCU mode */
	/* Field bit16 ~ bit19,
	 *   bit17 = 0 is for MCU mode
	 */
	/*GDL_HW_SET_CONN_INFRA_ENTRY( \*/
	/*	CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_GPS_MANUAL_CTRL, 0);*/
	#define AP_MODE_EN_ADDR CONN_CFG_ON_CONN_INFRA_CFG_GPS_MANUAL_CTRL_ADDR
	#define AP_MODE_EN_MASK 0x00020000
	#define AP_MODE_EN_SHFT 17
	GDL_HW_SET_CONN_INFRA_ENTRY(
		AP_MODE_EN, 0);

	/* Hold MCU reset */
	GDL_HW_SET_CONN_INFRA_ENTRY(
		CONN_RGU_ON_GPSSYS_CPU_SW_RST_B_GPSSYS_CPU_SW_RST_B, 0);

	/* Enable Conninfra BGF_ON */
	GDL_HW_SET_CONN_INFRA_BGF_EN(1);

/* Poll BGF_ON_STATUS */
	#define BGF_ON_STATUS_ADDR CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR
	#define BGF_ON_STATUS_MASK 0x000C0000
	#define BGF_ON_STATUS_SHFT 18
	GDL_HW_POLL_CONN_INFRA_ENTRY(
		BGF_ON_STATUS, 0x1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_on_status_not_okay");
		goto _fail_bgf_on_status_not_okay;
	}

	/* Enable GPS function */
	GDL_HW_SET_GPS_FUNC_EN(1);

	/* Disable sleep prot */
	if (gps_dl_hw_gps_sleep_prot_ctrl(1) != 0) {
		GDL_LOGE("_fail_disable_gps_slp_prot_not_okay");
		goto _fail_disable_gps_slp_prot_not_okay;
	}

	/* Poll 0x18c12010[31:0] bgf ip version */
	GDL_HW_POLL_GPS_ENTRY(
		BG_GPS_CFG_BGF_IP_VERSION_BGFSYS_VERSION, GDL_HW_BGF_VER_MT6985,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_ip_ver_not_okay");
		goto _fail_bgf_ip_ver_not_okay;
	}

	/* Set GALS_SAMPLE_SEL */
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MCU_BUS_CR_BG_MCU_EIF_GALS_CTL1_BG2CONN_AXI_GALS_SAMPLE_SEL, 1);

	/* Clr MCCR_DIS */
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MCU_CONFG_MCCR_AHB_AUTO_DIS, 0);

	/* Set MCCR_SET */
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MCU_CONFG_MCCR_SET_AHB, 1);

	/* Force GPS_OSC_CTL on */
	GDL_HW_FORCE_OSC_CTL(1);

	/* Poll OSC_CTL */
	GDL_HW_POLL_GPS_ENTRY(
		BG_GPS_CFG_ON_GPS_OSC_CTL_ST_GPS_OSC_CTL_CS, 0x1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_gps_osc_ctl");
		goto _fail_gps_osc_ctl;
	}

	/* Poll TOP_OFF_PWR_CTL */
	GDL_HW_POLL_GPS_ENTRY(
		BG_GPS_CFG_ON_GPS_TOP_OFF_PWR_CTL_GPS_TOP_OFF_PWR_CTL_CS, 0x2,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_gps_top_off_pwr_ctl");
		goto _fail_gps_top_off_pwr_ctl;
	}
	return true;

_fail_gps_top_off_pwr_ctl:
_fail_gps_osc_ctl:
_fail_bgf_ip_ver_not_okay:
_fail_disable_gps_slp_prot_not_okay:
	gps_dl_hw_gps_sleep_prot_ctrl(0);
	GDL_HW_SET_GPS_FUNC_EN(0);
_fail_bgf_on_status_not_okay:
	GDL_HW_SET_CONN_INFRA_BGF_EN(0);
	return false;
}

void gps_mcudl_hw_mcu_speed_up_clock(void)
{
	int i;
	unsigned int flag_out;
	bool pll_okay = false;

	/* Enable PLL */
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_CTL_CR_GPS_MON_FLAG_EN_OFF, 0x1);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_SYS_SEL0_CR_GPS_MON_SYS_SEL0_OFF, 0x0000);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_SEL0_CR_GPS_MON_FLAG_SEL0_OFF, 0x000C);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_GPS_CLKGEN_CTL1_CR_GPS_MCU_WBG_EN_PLL, 0x1); /* Enable */

	/* Wait PLL ready */
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_SEL0_CR_GPS_MON_FLAG_SEL0_OFF, 0x03020100);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_SEL1_CR_GPS_MON_FLAG_SEL1_OFF, 0x07060504);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_SEL2_CR_GPS_MON_FLAG_SEL2_OFF, 0x0b0a0908);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_SEL3_CR_GPS_MON_FLAG_SEL3_OFF, 0x0f0e0d0c);
	for (i = 0; i < 30; i++) {
		flag_out = GDL_HW_GET_GPS_ENTRY(BG_GPS_CFG_BGSYS_OFF_MONFLAG_RD_MON_FLAG_OUT);
		GDL_LOGW("flag_out=0x%x, i=%d", flag_out, i);
		if ((flag_out & (1UL << 28)) || (flag_out & (1UL << 12))) {
			pll_okay = true;
			break;
		}
		gps_dl_sleep_us(100, 300);
	}

	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_CTL_CR_GPS_MON_FLAG_EN_OFF, 0x0);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_SYS_SEL0_CR_GPS_MON_SYS_SEL0_OFF, 0x0000);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_BG_GPS_OFF_MON_SEL0_CR_GPS_MON_FLAG_SEL0_OFF, 0x0000);

	if (!pll_okay) {
		GDL_LOGW("flag_out=0x%x, i=%d, pll not okay!", flag_out, i);
		return;
	}

	/* Set pll div switch to pll */
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_GPS_CLKGEN_CTL1_CR_GPS_BUS_PLL_DIV_EN, 0x1);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_GPS_CLKGEN_CTL1_CR_GPS_BUS_PLL_DIV_SEL, 0x5);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_CFG_GPS_CLKGEN_CTL0_GPS_HCLK_CK_SEL_CR, 0x2);
}

void gps_mcudl_hw_mcu_release_rst(void)
{
	/* Set it's 1st time power on */
	/* TODO:
	 * GDL_HW_SET_CONN_INFRA_ENTRY( \
	 *	BG_GPS_CFG_ON_GPS_ON_PWRCTL0_NON_1ST_TIME_PWR_ON_CLR, 1);
	 */

	/* Release MCU reset */
	GDL_HW_SET_CONN_INFRA_ENTRY(
		CONN_RGU_ON_GPSSYS_CPU_SW_RST_B_GPSSYS_CPU_SW_RST_B, 1);
}

bool gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us(unsigned int timeout_us)
{
	unsigned int val1, val2;
	unsigned int pc1;
	unsigned int wait_us = 0;

	do {
		val1 = GDL_HW_RD_GPS_REG(BG_GPS_MCU_CONFG_SW_DBG_CTL_ADDR);
		val2 = GDL_HW_RD_GPS_REG(BG_GPS_MCU_CONFG_SW_DBG_CTL_ADDR);
		GDL_HW_WR_CONN_INFRA_REG(
			CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, 0xC0040103);
		pc1 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
		if (val1 == 0x1D1E || val2 == 0x1D1E) {
			GDL_LOGW("idle_val=0x%08X, 0x%08X, pc=0x%08X", val1, val2, pc1);
			return true;
		}
		if (wait_us % 20000 == 0)
			GDL_LOGW("idle_val=0x%08X, 0x%08X, pc=0x%08X", val1, val2, pc1);

		gps_dl_sleep_us(900, 1100);
		wait_us += 1000;
	} while (wait_us <= timeout_us);

	return false;
}

void gps_mcudl_hw_mcu_do_off(void)
{
	/* Enable sleep prot */
	if (gps_dl_hw_gps_sleep_prot_ctrl(0) != 0)
		GDL_LOGE("_fail_enable_gps_slp_prot_not_okay");

	GDL_HW_SET_GPS_FUNC_EN(0);

	GDL_HW_SET_CONN_INFRA_BGF_EN(0);

	/* Hold MCU reset */
	GDL_HW_SET_CONN_INFRA_ENTRY(
		CONN_RGU_ON_GPSSYS_CPU_SW_RST_B_GPSSYS_CPU_SW_RST_B, 0);
}

void gps_mcudl_hw_mcu_show_status(void)
{
	unsigned int conn_ver, bg_ver;
	unsigned int pc1, pc2, pc3, pc4, not_rst;
	unsigned int val1, val2;
	unsigned int lp_status;
	unsigned int fw_own[7];

	conn_ver = GDL_HW_RD_CONN_INFRA_REG(CONN_CFG_IP_VERSION_ADDR);
	bg_ver = GDL_HW_RD_GPS_REG(BG_GPS_CFG_BGF_IP_VERSION_ADDR);
	GDL_LOGW("conn_ver=0x%08X, bg_ver=0x%08X", conn_ver, bg_ver);

	not_rst = GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_RGU_ON_GPSSYS_CPU_SW_RST_B_GPSSYS_CPU_SW_RST_B);
	GDL_HW_WR_CONN_INFRA_REG(
		CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, 0xC0040103);
	pc1 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	pc2 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	pc3 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	pc4 = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR);
	GDL_LOGW("not_rst=%d, pc=0x%08X, 0x%08X, 0x%08X, 0x%08X",
		not_rst, pc1, pc2, pc3, pc4);

	fw_own[0] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR);
	fw_own[1] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_IRQ_STAT_ADDR);
	fw_own[2] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_IRQ_ENA_ADDR);
	fw_own[3] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_FW_OWN_IRQ_ADDR);
	fw_own[4] = GDL_HW_RD_CONN_INFRA_REG(CONN_CFG_ON_CSR_BGF_ON_IRQ_STATUS_ADDR);
	fw_own[5] = GDL_HW_RD_CONN_INFRA_REG(CONN_CFG_ON_CSR_BGF_ON_HOST_CSR_MISC_ADDR);
	fw_own[6] = GDL_HW_RD_CONN_INFRA_REG(CONN_CFG_ON_CSR_BGF_ON_FW_OWN_IRQ_ADDR);
	GDL_LOGW("fw_own_sta=0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X",
		fw_own[0], fw_own[1], fw_own[2], fw_own[3], fw_own[4], fw_own[5], fw_own[6]);

	val1 = GDL_HW_RD_GPS_REG(BG_GPS_MCU_CONFG_SW_DBG_CTL_ADDR);
	val2 = GDL_HW_RD_GPS_REG(BG_GPS_MCU_CONFG_SW_DBG_CTL_ADDR);
	GDL_LOGW("idle_val=0x%08X, 0x%08X", val1, val2);

	lp_status = GDL_HW_RD_GPS_REG(CONN_MCU_CONFG_ON_HOST_MAILBOX_MCU_ADDR);
	GDL_LOGW("lp_status=0x%08X", lp_status);
}

void gps_mcudl_hw_mcu_show_pc_log(void)
{
	unsigned int flag;

	for (flag = 0xC0040104; flag <= 0xC0040113; flag++) {
		gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, flag,
			BMASK_RW_FORCE_PRINT);
		gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR,
			BMASK_RW_FORCE_PRINT);
	}
	for (flag = 0xC0040D00; flag <= 0xC0040D31; flag++) {
		gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, flag,
			BMASK_RW_FORCE_PRINT);
		gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR,
			BMASK_RW_FORCE_PRINT);
	}
}

bool gps_mcudl_hw_bg_is_readable(void)
{
	unsigned int slp_prot;
	unsigned int conn_ver = 0, bg_ver = 0;
	bool is_okay = true, conn_ver_okay = false;

	slp_prot = GDL_HW_RD_CONN_INFRA_REG(CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_ADDR);
	GDL_HW_CHECK_CONN_INFRA_VER(&conn_ver_okay, &conn_ver);

	if (slp_prot & (
		CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_RX_RDY_MASK |
		CONN_CFG_ON_GALS_CONN2GPS_SLP_STATUS_CONN2GPS_GALS_CTRL_PROT_TX_RDY_MASK))
		is_okay = false;

	if (is_okay && conn_ver_okay) {
		is_okay = false;
		GDL_HW_CHECK_BGF_IP_VER(&is_okay, &bg_ver);
	}

	GDL_LOGW("slp_prot=0x%08X, conn_ver=0x%08X, bg_ver=0x%08X, ok=%d",
		slp_prot, conn_ver, bg_ver, is_okay);
	return is_okay;
}

bool gps_mcudl_hw_mcu_set_or_clr_fw_own(bool to_set)
{
	bool is_okay = false;
	unsigned int fw_own;
	unsigned long d_us;
	unsigned long us0;

	fw_own = GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC);

	if ((!!fw_own) == to_set) {
		is_okay = true;
		GDL_LOGW("fw_own=%d, to_set=%d, is_okay=%d, bypass", fw_own, to_set, is_okay);
		return is_okay;
	}

	if (to_set)
		GDL_HW_SET_CONN_INFRA_ENTRY(
			CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_SET_FW_OWN_HS_PULSE, 1);
	else
		GDL_HW_SET_CONN_INFRA_ENTRY(
			CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_CLR_FW_OWN_HS_PULSE, 1);

	us0 = gps_dl_tick_get_us();
	GDL_HW_POLL_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC, to_set,
		POLL_DEFAULT, &is_okay);
	d_us = gps_dl_tick_get_us() - us0;

	if (!is_okay)
		GDL_LOGE("fw_own=%d, to_set=%d, is_okay=%d, d_us=%lu", fw_own, to_set, is_okay, d_us);
	else if (d_us > gps_mcudl_hal_user_get_fw_own_op_duration_us_to_warn())
		GDL_LOGW("fw_own=%d, to_set=%d, is_okay=%d, d_us=%lu", fw_own, to_set, is_okay, d_us);
	else
		GDL_LOGD("fw_own=%d, to_set=%d, is_okay=%d, d_us=%lu", fw_own, to_set, is_okay, d_us);
	return is_okay;
}

bool gps_mcudl_hw_mcu_set_or_clr_fw_own_is_okay(bool check_set)
{
	bool is_okay = false;

	GDL_HW_POLL_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC, check_set,
		POLL_1_TIME, &is_okay);
	return is_okay;
}

/* tmp*/
#if 1
void gps_dl_hw_set_mcu_emi_remapping_tmp(unsigned int _20msb_of_36bit_phy_addr)
{
	GDL_HW_SET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_CONN2AP_REMAP_MCU_EMI_BASE_ADDR,
		_20msb_of_36bit_phy_addr);
}

unsigned int gps_dl_hw_get_mcu_emi_remapping_tmp(void)
{
	return GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_CONN2AP_REMAP_MCU_EMI_BASE_ADDR);
}

void gps_dl_hw_set_gps_dyn_remapping_tmp(unsigned int val)
{
	GDL_HW_WR_GPS_REG(BG_GPS_MCU_BUS_CR_CONN2BGF_REMAP_1_ADDR, val);
}
#endif
/* tmp*/

void gps_mcudl_hw_may_set_link_power_flag(enum gps_mcudl_xid xid, bool power_ctrl)
{
/*No Need in Non-ATF*/
}

