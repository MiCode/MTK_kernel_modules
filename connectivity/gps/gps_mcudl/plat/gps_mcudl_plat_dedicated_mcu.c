/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_config.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_plat_api.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcusys_fsm.h"
#if GPS_DL_ON_LINUX
#include "gps_dl_subsys_reset.h"
#include "gps_dl_linux_reserved_mem_v2.h"
#endif
#include "gps_dl_hal.h"
#include "gps_dl_time_tick.h"
#include "gps_mcu_hif_api.h"
#include "gps_mcu_hif_host.h"
#if GPS_DL_HAS_MCUDL_FW
#include "gps_mcudl_fw_code.h"
#endif
#include "gps_mcu_hif_mgmt_cmd_send.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_mcudl_hw_mcu.h"
#include "gps_mcusys_data_sync2target.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"

#include "gps_dl_hw_api.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_data_pkt_payload_struct.h"
#include "gps_mcudl_link_state.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_mcudl_hal_conn.h"

#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
#include "gps_mcudl_hal_stat.h"
#endif

struct gps_mcudl_ystate {
	bool open;
	gpsmdl_u32 xstate_bitmask;
};

struct gps_mcudl_ystate g_gps_mcudl_ystate_list[GPS_MDLY_CH_NUM];

int gps_mcudl_plat_do_mcu_ctrl(enum gps_mcudl_yid yid, bool open)
{
	int retval = 0;

	if (yid == GPS_MDLY_NORMAL) {
		if (open)
			retval = gps_mcudl_plat_mcu_open();
		else
			retval = gps_mcudl_plat_mcu_close();
	}

	MDL_LOGYI(yid, "open=%d, ret=%d", open, retval);
	return retval;
}

unsigned int gps_dl_util_get_u32(const unsigned char *p_buffer)
{
	return ((unsigned int)(*p_buffer) +
		((unsigned int)(*(p_buffer + 1)) << 8) +
		((unsigned int)(*(p_buffer + 2)) << 16) +
		((unsigned int)(*(p_buffer + 3)) << 24));
}

#define GPS_MCU_FW_VER_STR_MAX_LEN (100)
bool gps_mcudl_link_drv_on_recv_mgmt_data(const unsigned char *p_data, unsigned int data_len)
{
	unsigned char cmd;
	unsigned char status = 0xFF;
	unsigned int addr, bytes, value;
	unsigned char fw_ver_str[GPS_MCU_FW_VER_STR_MAX_LEN] = {'\x00'};
	int i, j;

	/*TODO:*/
	if (data_len >= 4) {
		MDL_LOGW("data_len=%d, data[0~3]=0x%x 0x%x 0x%x 0x%x",
			data_len, p_data[0], p_data[1], p_data[2], p_data[3]);
	} else if (data_len == 3) {
		MDL_LOGW("data_len=%d, data[0~2]=0x%x 0x%x 0x%x",
			data_len, p_data[0], p_data[1], p_data[2]);
	} else if (data_len == 2) {
		MDL_LOGW("data_len=%d, data[0~1]=0x%x 0x%x",
			data_len, p_data[0], p_data[1]);
	} else if (data_len == 1) {
		MDL_LOGW("data_len=%d, data[0]=0x%x",
			data_len, p_data[0]);
	} else {
		MDL_LOGW("data_len=%d", data_len);
		return true;
	}

	cmd = p_data[0];
	if (data_len >= 2)
		status = p_data[1];

	switch (cmd) {
	case 1:
		gps_mcudl_mgmt_cmd_on_ack(GPS_MCUDL_CMD_OFL_INIT);
		break;
	case 2:
		gps_mcudl_mgmt_cmd_on_ack(GPS_MCUDL_CMD_OFL_DEINIT);
		break;
	case 3:
		gps_mcudl_mgmt_cmd_on_ack(GPS_MCUDL_CMD_FW_LOG_CTRL);
		break;
	case 5:
		if (data_len >= 16) {
			addr = gps_dl_util_get_u32(&p_data[4]);
			bytes = gps_dl_util_get_u32(&p_data[8]);
			value = gps_dl_util_get_u32(&p_data[12]);
			MDL_LOGW("mcu reg read: stat=%d, addr=0x%08x, bytes=%d, value[0]=0x%08x",
				status, addr, bytes, value);
		}
		break;
	case 6:
		if (status != 0)
			break;
		for (i = 0, j = 2; i < GPS_MCU_FW_VER_STR_MAX_LEN - 1; i++, j++) {
			if (j >= data_len)
				break;
			if (p_data[j] == '\x00')
				break;
			fw_ver_str[i] = p_data[j];
		}
		fw_ver_str[i] = '\x00';
		MDL_LOGW("fw_ver=%s", &fw_ver_str[0]);
		break;
	default:
		break;
	}

	return true;
}

bool gps_mcudl_link_drv_on_recv_urgent_data(const unsigned char *p_data, unsigned int data_len)
{
#if 1
	gps_mcudl_mcu2ap_ydata_recv(GPS_MDLY_URGENT, p_data, data_len);
#else
	gps_mcudl_plat_mcu_ch1_read_proc2(p_data, data_len);
#endif
	return true;
}


bool gps_mcudl_link_drv_on_recv_normal_data(const unsigned char *p_data, unsigned int data_len)
{
	MDL_LOGD("data_len=%d, data0=0x%x", data_len, p_data[0]);
#if 1
	gps_mcudl_mcu2ap_ydata_recv(GPS_MDLY_NORMAL, p_data, data_len);
#else
	gps_mcudl_plat_mcu_ch1_read_proc2(p_data, data_len);
#endif
	return true;
}

void gps_mcudl_hal_link_power_on_fail_handler(enum gps_mcudl_xid xid)
{
	int reset_check_cnt;

	gps_mcudl_hal_mcu_show_pc_log();
	gps_mcudl_hal_mcu_show_status();
	gps_mcudl_hal_ccif_show_status();
	gps_dl_hw_dump_host_csr_gps_info(false);
	reset_check_cnt = 0;
	while (!g_gps_mcudl_ever_do_coredump) {
		if (gps_mcudl_each_link_get_state(xid) == LINK_RESETTING) {
			MDL_LOGXE(xid, "fail to turn on - coredump");
			gps_mcudl_connsys_coredump_start_wrapper();
			g_gps_mcudl_ever_do_coredump = true;
			break;
		}

		/* ~15ms */
		if (reset_check_cnt >= 6)
			break;

		gps_dl_sleep_us(2200, 3200);
		gps_mcudl_hal_mcu_show_status();
		gps_mcudl_hal_ccif_show_status();
		reset_check_cnt++;
	}
}

unsigned int g_gps_mcu_open_index;

int gps_mcudl_hal_link_power_ctrl(enum gps_mcudl_xid xid, int op)
{
	enum gps_mcudl_yid yid;
	struct gps_mcudl_ystate *p_ystate;
	gpsmdl_u32 old_xbitmask, new_xbitmask;
	bool do_mcu_ctrl = false;
	bool non_lppm_sleep = false;
	int mcu_ctrl_ret = 0;
	unsigned long ts;
	unsigned long ts2;
	unsigned long ts_start, ts_end;
	unsigned int d_ms;

	/* only need to power on normal channel */
	yid = GPS_MDLY_NORMAL;
	p_ystate = &g_gps_mcudl_ystate_list[yid];
	old_xbitmask = p_ystate->xstate_bitmask;
	new_xbitmask = old_xbitmask;
	if (op)
		new_xbitmask |= (1UL << xid);
	else
		new_xbitmask &= ~(1UL << xid);

	if (xid == GPS_MDLX_LPPM && op == 0) {
		non_lppm_sleep = gps_mcudl_hal_get_non_lppm_sleep_flag();
		MDL_LOGYI(yid, "out_lpp_mode_notify_mcu, non_lppm_slp=%d", non_lppm_sleep);
		if (!non_lppm_sleep) {
			/* disallow set_fw_own due to lpp mode is disabled
			 * if non_lppm_sleep, no need this ctrl
			 */
			(void)gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_NON_LPPM);
		}
		gps_mcusys_data_sync2target_lpp_mode_status_cmd(op);
	}

	if (new_xbitmask == old_xbitmask) {
		/* do nothing */
		;
	} else if (op && old_xbitmask == 0) {
		ts = gps_dl_tick_get_ms();
		ts_start = gps_dl_tick_get_no_hop_ktime_ms();
		ts2 = gps_dl_tick_get_ktime_ms();
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
		gps_mcudl_stat_set_mcu_sid(g_gps_mcu_open_index);
#endif
		g_gps_mcu_open_index++;

		/* turn on */
		do_mcu_ctrl = true;

		MDL_LOGYI(yid, "gps_mcu_hif_init");
		gps_mcu_hif_init();
		gps_mcudl_mcu2ap_ydata_sta_init();
		MDL_LOGYI(yid, "gps_mcudl_ap2mcu_context_init");
		gps_mcudl_ap2mcu_context_init(GPS_MDLY_NORMAL);
		gps_mcudl_ap2mcu_context_init(GPS_MDLY_URGENT);
		gps_mcusys_mnlbin_fsm(GPS_MCUSYS_MNLBIN_SYS_ON);
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_PRE_ON);
		MDL_LOGYI(yid, "gps_mcudl_may_do_fw_loading, before");
		gps_mcudl_may_do_fw_loading();
		MDL_LOGYI(yid, "gps_mcudl_may_do_fw_loading, after");
		mcu_ctrl_ret = gps_mcudl_plat_do_mcu_ctrl(yid, true);
		if (mcu_ctrl_ret == 0) {
			gps_mcudl_hal_sync_non_flag_lppm_sleep_flag();
			non_lppm_sleep = gps_mcudl_hal_get_non_lppm_sleep_flag();
			if (!non_lppm_sleep) {
				/* disallow set_fw_own due to lpp mode has not been enabled yet */
				MDL_LOGYI(yid, "disallow set_fw_own for non_lppm_slp=%d", non_lppm_sleep);
				(void)gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_NON_LPPM);
			} else
				MDL_LOGYI(yid, "allow set_fw_own for non_lppm_slp=%d", non_lppm_sleep);
			gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_ON);
		} else {
			MDL_LOGXE(xid, "fail to turn on");
			gps_mcudl_hal_link_power_on_fail_handler(xid);
			gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_PRE_OFF);
			(void)gps_mcudl_plat_do_mcu_ctrl(yid, false);
			gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_OFF);
			gps_mcudl_clear_fw_loading_done_flag();
		}
		ts_end = gps_dl_tick_get_no_hop_ktime_ms();
		d_ms = (unsigned int)(ts_end - ts_start);
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
		gps_mcudl_stat_set_mcu_open_info(ts, ts2, d_ms);
#endif
	} else if (!op && new_xbitmask == 0) {
		ts = gps_dl_tick_get_ms();
		ts_start = gps_dl_tick_get_no_hop_ktime_ms();
		ts2 = gps_dl_tick_get_ktime_ms();
		/* turn off */
		do_mcu_ctrl = true;
		gps_mcudl_set_opp_vote_phase(GPS_MCU_CLOSING, true);
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_PRE_OFF);
		mcu_ctrl_ret = gps_mcudl_plat_do_mcu_ctrl(yid, false);
		MDL_LOGYD(yid, "gps_mcudl_clear_fw_loading_done_flag");
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_OFF);
		gps_mcudl_clear_fw_loading_done_flag();
		ts_end = gps_dl_tick_get_no_hop_ktime_ms();
		d_ms = (unsigned int)(ts_end - ts_start);
		gps_mcudl_hal_dump_reset_pwr_sw_flag_rec();
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
		gps_mcudl_stat_set_mcu_close_info(ts, ts2, d_ms);
#endif
	}

	if (mcu_ctrl_ret == 0 || !op)
		p_ystate->xstate_bitmask = new_xbitmask;

	if (xid == GPS_MDLX_LPPM && op == 1) {
		non_lppm_sleep = gps_mcudl_hal_get_non_lppm_sleep_flag();
		MDL_LOGYI(yid, "in_lpp_mode_notify_mcu, non_lppm_slp=%d", non_lppm_sleep);
		gps_mcusys_data_sync2target_lpp_mode_status_cmd(op);
		if (!non_lppm_sleep) {
			/* allow set_fw_own due to lpp mode is enabled
			 * if non_lppm_sleep, it's already allowed by default
			 */
			(void)gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_NON_LPPM);
		}
	}

	MDL_LOGYI(yid, "xid=%d, op=%d, xbitmask: 0x%08x -> 0x%08x, mcu_ctrl: do=%d, ret=%d",
		xid, op, old_xbitmask, new_xbitmask, do_mcu_ctrl, mcu_ctrl_ret);
	return mcu_ctrl_ret;
}

void gps_mcudl_hal_may_set_link_power_flag(enum gps_mcudl_xid xid,
	bool power_ctrl)
{
	gps_mcudl_hw_may_set_link_power_flag(xid, power_ctrl);
}

unsigned int g_conn_xuser;
bool g_gps_mcudl_ever_do_coredump;

int gps_mcudl_hal_conn_power_ctrl(enum gps_mcudl_xid xid, int op)
{

	if (1 == op) {
		if (g_conn_xuser == 0) {
			MDL_LOGXI_ONF(xid,
				"sid = %d, op = %d, user = 0x%x,%d, tia_on = %d",
				gps_mcudl_each_link_get_session_id(xid),
				op, g_conn_xuser, g_gps_conninfa_on, g_gps_tia_on);

			g_gps_mcudl_ever_do_coredump = false;
			gps_dl_log_info_show();
			gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(true);
			if (!gps_dl_hal_conn_infra_driver_on()) {
				gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(false);
				return -1;
			}
			gps_mcudl_set_opp_vote_phase(GPS_MCU_OPENING, true);
			gps_mcudl_set_opp_vote_phase(GPS_DSP_NOT_WORKING, true);

			gps_dl_hal_load_clock_flag();
#if GPS_DL_HAS_PLAT_DRV
			/*gps_dl_wake_lock_hold(true);*/
#if GPS_DL_USE_TIA
			/*gps_dl_tia_gps_ctrl(true);*/
			/*g_gps_tia_on = true;*/
#endif
#endif
		}
		g_conn_xuser |= (1UL << xid);
	} else if (0 == op) {
		g_conn_xuser &= ~(1UL << xid);
		if (g_conn_xuser == 0) {
			MDL_LOGXI_ONF(xid,
				"sid = %d, op = %d, user = 0x%x,%d, tia_on = %d",
				gps_mcudl_each_link_get_session_id(xid),
				op, g_conn_xuser, g_gps_conninfa_on, g_gps_tia_on);
#if GPS_DL_HAS_PLAT_DRV
#if GPS_DL_USE_TIA
			/*if (g_gps_tia_on) {*/
			/*	gps_dl_tia_gps_ctrl(false);*/
			/*	g_gps_tia_on = false;*/
			/*}*/
#endif
			/*gps_dl_wake_lock_hold(false);*/
#endif
			gps_mcudl_set_opp_vote_phase(GPS_MCU_CLOSING, false);
			gps_mcudl_set_opp_vote_phase(GPS_DSP_NOT_WORKING, false);
			gps_mcudl_set_opp_vote_phase(GPS_MNLD_LPPM_CLOSING, false);
			gps_mcudl_end_all_opp_vote_phase();

			/* double trial in case that gps_mcudl_hw_conn_force_wake is not called */
			gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(false);

			/* no need to vote opp for gps_dl_hal_conn_infra_driver_off */
			gps_dl_hal_conn_infra_driver_off();
		}
	}

	return 0;
}

#if 0
void gps_mcudl_plat_mcu_ch1_event_cb(void)
{
	gps_mcudl_mcu2ap_ydata_notify(GPS_MDLY_NORMAL);
}

void gps_mcudl_plat_mcu_ch1_read_proc(void)
{
	enum gps_mcudl_yid y_id;
	gpsmdl_u8 tmp_buf[2048];
	int ret_len;

	y_id = GPS_MDLY_NORMAL;
	MDL_LOGYD(y_id, "");

	gps_mcudl_mcu2ap_set_wait_read_flag(y_id, false);
	do {
		ret_len = gps_mcudl_plat_mcu_ch1_read_nonblock(&tmp_buf[0], 2048);
		MDL_LOGYD(y_id, "read: len=%d", ret_len);
		if (ret_len > 0)
			gps_mcudl_mcu2ap_ydata_recv(y_id, &tmp_buf[0], ret_len);
	} while (ret_len > 0);
}

void gps_mcudl_plat_mcu_ch1_read_proc2(const unsigned char *p_data, unsigned int data_len)
{
	enum gps_mcudl_yid y_id;

	y_id = GPS_MDLY_NORMAL;
	gps_mcudl_mcu2ap_set_wait_read_flag(y_id, false);
	MDL_LOGYD(y_id, "read: len=%u", data_len);
	if (data_len > 0)
		gps_mcudl_mcu2ap_ydata_recv(y_id, p_data, data_len);
}
#endif

void gps_mcudl_plat_mcu_ch1_reset_start_cb(void)
{
	enum gps_mcudl_yid y_id = GPS_MDLY_NORMAL;

	MDL_LOGI("");
	gps_mcudl_clear_fw_loading_done_flag();
	/*gps_dl_on_pre_connsys_reset();*/
	gps_mcudl_ylink_event_send(y_id, GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_START);
}

void gps_mcudl_plat_mcu_ch1_reset_end_cb(void)
{
	enum gps_mcudl_yid y_id = GPS_MDLY_NORMAL;

	MDL_LOGI("");
	/*gps_dl_on_post_connsys_reset();*/
	gps_mcudl_ylink_event_send(y_id, GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_END);
}

enum gps_mcudl_plat_mcu_ctrl_status {
	GDL_MCU_CLOSED,
	GDL_MCU_OPEN_OKAY,
	GDL_MCU_OPEN_FAIL_ON_POS,
	GDL_MCU_OPEN_FAIL_ON_CMD,
	GDL_MCU_OPEN_STATUS_NUM
};

enum gps_mcudl_plat_mcu_ctrl_status g_gps_mcudl_mcu_ctrl_status;

int gps_mcudl_plat_mcu_open(void)
{
	bool is_okay;

#if (GPS_DL_HAS_MCUDL_FW && GPS_DL_HAS_MCUDL_HAL)
	is_okay = gps_mcudl_xlink_on(&c_gps_mcudl_rom_only_fw_list);
	if (!is_okay) {
		MDL_LOGI("gps_mcudl_xlink_on failed");
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_POS;
		return -1;
	}
#endif
	gps_mcudl_mcu2ap_set_wait_read_flag(GPS_MDLY_NORMAL, false);
	gps_mcudl_mcu2ap_set_wait_read_flag(GPS_MDLY_URGENT, false);
	gps_mcudl_mgmt_cmd_state_init_all();
	gps_mcu_hif_host_trans_hist_init();
	gps_mcu_host_trans_hist_init();
	gps_mcudl_host_sta_hist_init();
	gps_mcudl_mcu2ap_rec_init();
	gps_mcudl_mcu2ap_put_to_xlink_fail_rec_init();
	gps_mcudl_mcu2ap_clear_ap_resume_pkt_dump_flag();
	gps_mcudl_mcu2ap_test_bypass_set(false);

	gps_mcu_hif_recv_listen_start(GPS_MCU_HIF_CH_DMALESS_MGMT,
		&gps_mcudl_link_drv_on_recv_mgmt_data);
	gps_mcu_hif_recv_listen_start(GPS_MCU_HIF_CH_DMA_NORMAL,
		&gps_mcudl_link_drv_on_recv_normal_data);
	gps_mcu_hif_recv_listen_start(GPS_MCU_HIF_CH_DMA_URGENT,
		&gps_mcudl_link_drv_on_recv_urgent_data);
	MDL_LOGI("add listeners, done");

	g_gps_ccif_irq_cnt = 0;
	g_gps_fw_log_irq_cnt = 0;
	if (g_gps_fw_log_is_on) {
		is_okay = gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(true);
		if (!is_okay) {
			MDL_LOGW("log_ctrl, is_ok=%d", is_okay);
			g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_CMD;
			return -1;
		}
	}

	gps_mcudl_mgmt_cmd_pre_send(GPS_MCUDL_CMD_OFL_INIT);
	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, "\x01", 1);
	MDL_LOGW("write cmd1, is_ok=%d", is_okay);
	if (!is_okay) {
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_CMD;
		return -1;
	}

	is_okay = gps_mcudl_mgmt_cmd_wait_ack(GPS_MCUDL_CMD_OFL_INIT, 100);
	MDL_LOGW("write cmd1, wait_ok=%d", is_okay);
	if (!is_okay)
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_CMD;
	else
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_OKAY;

	return is_okay ? 0 : -1;
}

int gps_mcudl_plat_mcu_close(void)
{
	bool is_okay;
	bool do_adie_off_in_driver = false;

	if (g_gps_mcudl_mcu_ctrl_status != GDL_MCU_OPEN_OKAY)
		MDL_LOGW("mcu_status = %d", g_gps_mcudl_mcu_ctrl_status);

	if (g_gps_mcudl_mcu_ctrl_status == GDL_MCU_OPEN_OKAY) {
		gps_mcudl_mgmt_cmd_pre_send(GPS_MCUDL_CMD_OFL_DEINIT);
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, "\x02", 1);
		MDL_LOGW("write cmd2, is_ok=%d", is_okay);
		if (is_okay) {
			is_okay = gps_mcudl_mgmt_cmd_wait_ack(GPS_MCUDL_CMD_OFL_DEINIT, 100);
			MDL_LOGW("write cmd2, wait_ok=%d", is_okay);
		}

		if (!is_okay)
			do_adie_off_in_driver = true;
	}

	if (g_gps_mcudl_mcu_ctrl_status != GDL_MCU_CLOSED &&
		g_gps_mcudl_mcu_ctrl_status != GDL_MCU_OPEN_FAIL_ON_POS) {
		gps_mcu_hif_recv_listen_stop(GPS_MCU_HIF_CH_DMA_NORMAL);
		gps_mcu_hif_recv_listen_stop(GPS_MCU_HIF_CH_DMALESS_MGMT);
		MDL_LOGI("remove listeners, done");
	}

	if (g_gps_mcudl_mcu_ctrl_status != GDL_MCU_CLOSED) {
#if GPS_DL_HAS_MCUDL_HAL
		if (do_adie_off_in_driver) {
			gps_dl_hw_gps_dump_top_rf_cr();
			gps_dl_hw_gps_dump_gps_rf_cr_new();
			gps_dl_hw_dep_gps_control_adie_off();
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
			gps_mcudl_stat_set_mcu_force_close();
#endif
		}
		gps_mcudl_xlink_off();
#endif
	}

#if GPS_DL_HAS_PLAT_DRV
	gps_dl_update_status_for_md_blanking(false);
#endif
	gps_mcudl_mcu2ap_put_to_xlink_fail_rec_dump();

	g_gps_mcudl_mcu_ctrl_status = GDL_MCU_CLOSED;
	return 0;
}

int gps_mcudl_plat_mcu_ch1_write(const unsigned char *kbuf, unsigned int count)
{
	bool is_okay = false, conn_okay = false;
	static bool is_okay_last = true, is_okay_new;
	unsigned long curr_us;
	bool not_too_freq = false;
	static unsigned int skip_print_cnt;
	static unsigned long print_us;
	static bool pair_print;
	struct gps_mcudl_data_pkt_rec_item rec_item;
	enum gps_mcu_hif_send_status send_status = GPS_MCU_HIF_SEND_STATUS_NUM;

	if (!gps_mcusys_gpsbin_state_is(GPS_MCUSYS_GPSBIN_POST_ON)) {
		MDL_LOGW("write count=%d, fail due to MCU not post_on", count);
		return 0;
	}

	is_okay = gps_mcu_hif_send_v2(GPS_MCU_HIF_CH_DMA_NORMAL, kbuf, count, &send_status);

	rec_item.host_wr.len = count;
	rec_item.host_wr.is_okay = is_okay;
	rec_item.host_wr.host_us = gps_dl_tick_get_us();
	gps_mcu_host_trans_hist_rec(&rec_item, GPS_MCUDL_HIST_REC_HOST_WR);

	is_okay_new = is_okay;

	if (gps_mcu_host_trans_get_if_need_dump())
		MDL_LOGW("write count=%d, is_ok=%d", count, is_okay);
	if (!is_okay) {
		conn_okay = gps_mcudl_conninfra_is_okay_or_handle_it();
		curr_us = gps_dl_tick_get_us();
		not_too_freq = (curr_us - print_us > 1000*1000); /* > 1sec */
		if (((is_okay_new != is_okay_last) && not_too_freq) || !conn_okay) {
			MDL_LOGW("write count=%d, is_ok=%d, conn_okay=%d, send_status = %d, skip = %u",
				count, is_okay, conn_okay, send_status, skip_print_cnt);
			/* above is a writing-failure log,
			 * set true to tigger a printing when writing becomes okay
			 */
			pair_print = true;
			skip_print_cnt = 0;
			print_us = curr_us;
		} else
			skip_print_cnt++;
		is_okay_last = is_okay;
		return 0;
	}

	if (is_okay_new != is_okay_last && pair_print) {
		MDL_LOGW("write count=%d, is_ok=%d, send_status = %d",
			count, is_okay, send_status);
		pair_print = false;
	}
	is_okay_last = is_okay;
	return count;
}


int gps_mcudl_plat_mcu_ch2_write(const unsigned char *kbuf, unsigned int count)
{
	bool is_okay;
	gps_mcudl_ap2mcu_set_write_fail_flag(GPS_MDLY_URGENT, true);

	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMA_URGENT, kbuf, count);
	if (!is_okay)
		return 0;
	gps_mcudl_ap2mcu_set_write_fail_flag(GPS_MDLY_URGENT, false);
	return count;
}


int gps_mcudl_plat_mcu_ch1_read_nonblock(unsigned char *kbuf, unsigned int count)
{
	return 0;
}

void gps_mcudl_plat_nv_emi_clear(void)
{
	struct gps_mcudl_emi_region_item region;
	void *region_virt_addr;

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_NVEMI, &region);
	if (region_virt_addr == NULL) {
		MDL_LOGW("region: nv_emi is null");
		return;
	}

	memset_io(region_virt_addr, 0, region.length);
}

void *gps_mcudl_plat_nv_emi_get_start_ptr(void)
{
	void *region_virt_addr;

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_NVEMI, NULL);
	return (void *)region_virt_addr;
}

void *gps_mcudl_plat_nv_emi_get_end_ptr(void)
{
	struct gps_mcudl_emi_region_item region;
	void *region_virt_addr;

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_NVEMI, &region);
	if (region_virt_addr == NULL)
		return NULL;

	return (void *)(region.offset + (unsigned char *)region_virt_addr);
}

gpsmdl_u32 gps_mcudl_ylink_get_xbitmask(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_ystate *p_ystate;
	gpsmdl_u32 xbitmask;

	p_ystate = &g_gps_mcudl_ystate_list[y_id];
	xbitmask = p_ystate->xstate_bitmask;
	return xbitmask;
}

unsigned int gps_mcudl_hal_get_open_flag(void)
{
	return g_conn_xuser;
}

