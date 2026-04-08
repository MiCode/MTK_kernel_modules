/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_config.h"

#include "gps_mcudl_hw_mcu.h"
#include "gps_mcudl_hw_dep_macro.h"
#include "gps_mcudl_hw_priv_util.h"
#include "gps_dl_hw_atf.h"
#include "gps_mcudl_log.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_dl_hal.h"

bool gps_mcudl_check_conn_infra_ver_is_ok(unsigned int *poll_ver)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_CHECK_CONN_INFRA_VER_IS_OK_OPID,
				0, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		*poll_ver = (unsigned int)res.a1;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_check_conn_infra_ver_is_ok, poll_ver = %d, cnt = %d",
			(*poll_ver), i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_check_conn_infra_ver_is_not_ok");

	return ret;
}

bool gps_mcudl_poll_conn_infra_cmbdt_restore_is_ok(void)
{
	struct arm_smccc_res res;
	bool ret = false;
	int i;

	/* 0x18001210[16] == 1b'1 conn_infra cmdbt restore done, */
	/*polling 10 times at most, interval = 0.5ms */
	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_POLL_CONN_INFRA_CMBDT_RESTORE_IS_OK_OPID,
				0, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_poll_conn_infra_cmdbt_restore_done, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_poll_conn_infra_cmdbt_restore_not_done");

	return ret;
}

bool gps_mcudl_hw_conn_ver_and_wake_is_ok(void)
{
	bool poll_okay = false;
	unsigned int poll_ver = 0;

	/* Poll conninfra hw version */
	poll_okay = gps_mcudl_check_conn_infra_ver_is_ok(&poll_ver);
	if (!poll_okay) {
		GDL_LOGE("_fail_check_conn_infra_ver_not_okay");
		goto _fail_check_conn_infra_ver_not_okay;
	}

#if GPS_DL_ON_LINUX
	gps_dl_hal_set_conn_infra_ver(poll_ver);
#endif

	/* Poll conninfra cmdbt restore done, 0.5ms * 10 */
	poll_okay = gps_mcudl_poll_conn_infra_cmbdt_restore_is_ok();
	if (!poll_okay) {
		GDL_LOGE("_fail_conn_cmdbt_restore_not_okay");
		goto _fail_conn_cmdbt_restore_not_okay;
	}
	return true;

_fail_conn_cmdbt_restore_not_okay:
_fail_check_conn_infra_ver_not_okay:
	return false;
}

bool gps_mcudl_hw_conn_force_wake_inner(bool enable)
{
	struct arm_smccc_res res;
	bool poll_okay = false;

	if (enable) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_WAKEUP_GPS_OPID,
				1, 0, 0, 0, 0, 0, &res);
		poll_okay = gps_mcudl_hw_conn_ver_and_wake_is_ok();
		if (!poll_okay) {
			GDL_LOGE("_fail_conn_ver_or_wake_not_okay");
			return false; /* not okay */
		}
	} else
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_WAKEUP_GPS_OPID,
				0, 0, 0, 0, 0, 0, &res);

	return true;
}

static bool gps_dl_hw_gps_sleep_prot_ctrl_conn2gps_rx(int op)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_GPS_SLEEP_PROT_CTRL_CONN2GPS_RX_OPID,
				op, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_gps_sleep_prot_ctrl_conn2gps_rx, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_gps_sleep_prot_ctrl_conn2gps_rx_not_okay");

	return ret;

}

static bool gps_dl_hw_gps_sleep_prot_ctrl_conn2gps_tx(int op)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_GPS_SLEEP_PROT_CTRL_CONN2GPS_TX_OPID,
				op, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_gps_sleep_prot_ctrl_conn2gps_tx, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_gps_sleep_prot_ctrl_conn2gps_tx_not_okay");

	return ret;
}

static bool gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_rx(int op)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_GPS_SLEEP_PROT_CTRL_GPS2CONN_RX_OPID,
				op, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_gps_sleep_prot_ctrl_gps2conn_rx, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_gps_sleep_prot_ctrl_gps2conn_rx_not_okay");

	return ret;
}

static bool gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_tx(int op)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_GPS_SLEEP_PROT_CTRL_GPS2CONN_TX_OPID,
				op, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_gps_sleep_prot_ctrl_gps2conn_tx, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_gps_sleep_prot_ctrl_gps2conn_tx_not_okay");

	return ret;
}

static bool gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_axi_rx(int op)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_GPS_SLEEP_PROT_CTRL_GPS2CONN_AXI_RX_OPID,
				op, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_gps_sleep_prot_ctrl_gps2conn_axi_rx, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_gps_sleep_prot_ctrl_gps2conn_axi_rx_not_okay");

	return ret;
}

static bool gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_axi_tx(int op)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_GPS_SLEEP_PROT_CTRL_GPS2CONN_AXI_TX_OPID,
				op, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_gps_sleep_prot_ctrl_gps2conn_axi_tx, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_gps_sleep_prot_ctrl_gps2conn_axi_tx_not_okay");

	return ret;
}

static int gps_dl_hw_gps_sleep_prot_ctrl(int op)
{
	bool poll_okay = false;
	int ret;

	if (op == 1) {
		/* disable when on */
		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_conn2gps_rx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps rx");
			goto _fail_disable_gps_slp_prot;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_conn2gps_tx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps tx");
			goto _fail_disable_gps_slp_prot;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_rx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn rx");
			goto _fail_disable_gps_slp_prot;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_tx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
			goto _fail_disable_gps_slp_prot;
		}

		/* AXI */
		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_axi_rx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn_axi rx");
			goto _fail_disable_gps_slp_prot;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_axi_tx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn_axi tx");
			goto _fail_disable_gps_slp_prot;
		}
		return 0;

_fail_disable_gps_slp_prot:
		gps_mcudl_hal_mcu_show_pc_log();
		gps_dl_slp_prot_fail_and_dump();
		return -1;
	} else if (op == 0) {
		ret = 0;
		/* enable when off */
		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_tx(op);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn tx");
			ret = -1;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_rx(op);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn rx");
			ret = -1;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_conn2gps_tx(op);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps tx");
			ret = -1;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_conn2gps_rx(op);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps rx");
		}

		/* AXI */
		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_axi_tx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn_axi tx");
			ret = -1;
			goto _fail_enable_gps_slp_prot;
		}

		poll_okay = gps_dl_hw_gps_sleep_prot_ctrl_gps2conn_axi_rx(op);
		if (!poll_okay) {
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn_axi rx");
			ret = -1;
			goto _fail_enable_gps_slp_prot;
		}

		return ret;

_fail_enable_gps_slp_prot:
		/* trigger reset on outer function */
		gps_mcudl_hal_mcu_show_pc_log();
		gps_dl_slp_prot_fail_and_dump();
		return -1;
	}
	return 0;
}

bool gps_mcudl_hw_mcu_do_on_with_rst_held_1(void)
{
	struct arm_smccc_res res;
	bool ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_DO_ON_WITH_RST_HELD_1_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (bool)res.a0;
	return ret;
}

bool gps_mcudl_hw_mcu_do_on_with_rst_held_2(void)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_DO_ON_WITH_RST_HELD_2_OPID,
				0, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_hw_mcu_do_on_with_rst_held_2, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_hw_mcu_do_on_with_rst_held_2_not_okay");

	return ret;
}

bool gps_mcudl_hw_mcu_do_on_with_rst_held_3(void)
{
	struct arm_smccc_res res;
	bool ret;
	int i;

	for (i = 0; i < 40; i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_DO_ON_WITH_RST_HELD_3_OPID,
				0, 0, 0, 0, 0, 0, &res);
		ret = (bool)res.a0;
		if (ret)
			return ret;
		if (i > 0)
			GDL_LOGD("_poll_mcu_do_on_with_rst_held_3, cnt = %d", i + 1);
	}
	if (!ret)
		GDL_LOGE("_poll_mcu_do_on_with_rst_held_3_not_okay");

	return ret;
}

void gps_mcudl_hw_mcu_do_on_with_rst_held_fail_handler_1(void)
{
	struct arm_smccc_res res;
	bool ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_DO_ON_WITH_RST_HELD_FAIL_HANDLER_1_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (bool)res.a0;
}

void gps_mcudl_hw_mcu_do_on_with_rst_held_fail_handler_2(void)
{
	struct arm_smccc_res res;
	bool ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_DO_ON_WITH_RST_HELD_FAIL_HANDLER_2_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (bool)res.a0;
}

bool gps_mcudl_hw_mcu_do_on_with_rst_held(void)
{
	bool poll_okay = false;

	poll_okay = gps_mcudl_hw_mcu_do_on_with_rst_held_1();
	if (!poll_okay) {
		GDL_LOGE("_fail_mcudl_hw_mcu_do_on_with_rst_held_1_not_okay");
		goto _fail_mcudl_hw_mcu_do_on_with_rst_held_1_not_okay;
	}

	/* Disable sleep prot */
	if (gps_dl_hw_gps_sleep_prot_ctrl(1) != 0) {
		GDL_LOGE("_fail_disable_gps_slp_prot_not_okay");
		goto _fail_disable_gps_slp_prot_not_okay;
	}

	poll_okay = gps_mcudl_hw_mcu_do_on_with_rst_held_2();
	if (!poll_okay) {
		GDL_LOGE("_fail_mcudl_hw_mcu_do_on_with_rst_held_2_not_okay");
		goto _fail_mcudl_hw_mcu_do_on_with_rst_held_2_not_okay;
	}

	poll_okay = gps_mcudl_hw_mcu_do_on_with_rst_held_3();
	if (!poll_okay) {
		GDL_LOGE("_fail_mcudl_hw_mcu_do_on_with_rst_held_3_not_okay");
		goto _fail_mcudl_hw_mcu_do_on_with_rst_held_3_not_okay;
	}

	return true;

_fail_mcudl_hw_mcu_do_on_with_rst_held_3_not_okay:
_fail_mcudl_hw_mcu_do_on_with_rst_held_2_not_okay:
_fail_disable_gps_slp_prot_not_okay:
	gps_dl_hw_gps_sleep_prot_ctrl(0);
	gps_mcudl_hw_mcu_do_on_with_rst_held_fail_handler_2();

_fail_mcudl_hw_mcu_do_on_with_rst_held_1_not_okay:
	gps_mcudl_hw_mcu_do_on_with_rst_held_fail_handler_1();
	return false;
}

void gps_mcudl_hw_mcu_enable_clock(void)
{
	struct arm_smccc_res res;
	bool ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_ENABLE_CLOCK_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (bool)res.a0;
}

unsigned int gps_mcudl_hw_mcu_wait_clock_ready(void)
{
	struct arm_smccc_res res;
	unsigned int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_WAIT_CLOCK_READY_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

void gps_mcudl_hw_mcu_set_pll(bool pll_okay)
{
	struct arm_smccc_res res;
	bool ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_SET_PLL_OPID,
			pll_okay, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
}

void gps_mcudl_hw_mcu_speed_up_clock(void)
{
	int i;
	unsigned int flag_out;
	bool pll_okay = false;

	gps_mcudl_hw_mcu_enable_clock();

	for (i = 0; i < 30; i++) {
		flag_out = gps_mcudl_hw_mcu_wait_clock_ready();
		GDL_LOGW("flag_out=0x%x, i=%d", flag_out, i);
		if ((flag_out & (1UL << 28)) || (flag_out & (1UL << 12))) {
			pll_okay = true;
			break;
		}
		gps_dl_sleep_us(100, 300);
	}

	gps_mcudl_hw_mcu_set_pll(pll_okay);
}

void gps_mcudl_hw_mcu_release_rst(void)
{
	/* Set it's 1st time power on */
	/* TODO:
	 * GDL_HW_SET_CONN_INFRA_ENTRY( \
	 *	BG_GPS_CFG_ON_GPS_ON_PWRCTL0_NON_1ST_TIME_PWR_ON_CLR, 1);
	 */
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_RELEASE_RST_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us_atf(unsigned int *val1,
	unsigned int *val2, unsigned int *pc1)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_WAIT_IDLE_LOOP_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	*val1 = (unsigned int)res.a1;
	*val2 = (unsigned int)res.a2;
	*pc1 = (unsigned int)res.a3;
}

bool gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us(unsigned int timeout_us)
{
	unsigned int val1, val2;
	unsigned int pc1;
	unsigned int wait_us = 0;

	do {
		gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us_atf(&val1, &val2, &pc1);
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

void gps_mcudl_hw_mcu_do_off_1(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_DO_OFF_1_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_mcudl_hw_mcu_do_off(void)
{
	/* Enable sleep prot */
	if (gps_dl_hw_gps_sleep_prot_ctrl(0) != 0)
		GDL_LOGE("_fail_enable_gps_slp_prot_not_okay");

	gps_mcudl_hw_mcu_do_on_with_rst_held_fail_handler_2();
	gps_mcudl_hw_mcu_do_on_with_rst_held_fail_handler_1();

	/* Hold MCU reset */
	gps_mcudl_hw_mcu_do_off_1();
}

void gps_mcudl_hw_mcu_show_status(void)
{
	struct arm_smccc_res res;
	int ret;
	unsigned int conn_ver, bg_ver;
	unsigned int pc1, pc2, pc3, pc4, not_rst;
	unsigned int val1, val2;
	unsigned int lp_status;
	unsigned int fw_own[7];

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_SHOW_STATUS_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;

	conn_ver = GDL_HW_RD_CONN_INFRA_REG(CONN_CFG_IP_VERSION_ADDR);
	bg_ver = GDL_HW_RD_GPS_REG(BG_GPS_CFG_BGF_IP_VERSION_ADDR);
	GDL_LOGW("conn_ver=0x%08X, bg_ver=0x%08X", conn_ver, bg_ver);

	not_rst = GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_RGU_ON_GPSSYS_CPU_SW_RST_B_GPSSYS_CPU_SW_RST_B);
	/*
	 * GDL_HW_WR_CONN_INFRA_REG(
	 *	CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, 0xC0040103);
	 */
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
	gps_dl_hw_dep_dump_host_csr_range(0xC0040104, 0x10);
	gps_dl_hw_dep_dump_host_csr_range(0xC0040D00, 0x32);
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
	struct arm_smccc_res res;
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

	us0 = gps_dl_tick_get_us();
	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_MCU_SET_OR_CLR_FW_OWN_OPID,
			to_set, 0, 0, 0, 0, 0, &res);
	is_okay = (bool)res.a0;
	if (!is_okay) {
		GDL_HW_POLL_CONN_INFRA_ENTRY(
			CONN_HOST_CSR_TOP_BGF_LPCTL_BGF_AP_HOST_OWNER_STATE_SYNC, to_set,
			POLL_DEFAULT, &is_okay);
	}
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
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_SET_MCU_EMI_REMAPPING_TMP_OPID,
			_20msb_of_36bit_phy_addr, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_dl_hw_get_mcu_emi_remapping_tmp(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_GET_MCU_EMI_REMAPPING_TMP_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;

	return ret;
}

void gps_dl_hw_set_gps_dyn_remapping_tmp(unsigned int val)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_SET_GPS_DYNC_REMAPPING_TMP_OPID,
			val, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}
#endif
/* tmp*/

void gps_mcudl_hw_may_set_link_power_flag(enum gps_mcudl_xid xid, bool power_ctrl)
{
	struct arm_smccc_res res;
	static unsigned int gps_mcu_common_on_flag;
	int ret;

	memset(&res, 0, sizeof(res));

	if (power_ctrl) {
		if (gps_mcu_common_on_flag == 0)
			arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_SET_FLAG_OPID,
					0xdb9db9, power_ctrl, 0, 0, 0, 0, &res);
		gps_mcu_common_on_flag |= (1UL << xid);
	} else {
		gps_mcu_common_on_flag &= ~(1UL << xid);
		if (gps_mcu_common_on_flag == 0)
			arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_SET_FLAG_OPID,
					0xdb9db9, power_ctrl, 0, 0, 0, 0, &res);
	}
	ret = res.a0;
}

