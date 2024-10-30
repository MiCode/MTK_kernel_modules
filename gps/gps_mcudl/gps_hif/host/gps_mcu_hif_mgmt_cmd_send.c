/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_time_tick.h"
#include "gps_mcudl_log.h"
#include "gps_mcu_hif_mgmt_cmd_send.h"
#include "gps_mcudl_data_pkt_slot.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcu_hif_host.h"
#if GPS_DL_ON_LINUX
#include <linux/jiffies.h>
#include <linux/completion.h>
#endif

enum gps_mcudl_mgmt_cmd_state_enum {
	GMDL_CMD_IDLE,
	GMDL_CMD_PRE_SEND,
	GMDL_CMD_WAIT_ACK,
	GMDL_CMD_ACK_BEFORE_WAIT,
	GMDL_CMD_ACK_AFTER_WAIT,
};

struct gps_mcudl_mgmt_cmd_state_wrapper {
	enum gps_mcudl_mgmt_cmd_state_enum state;
	bool is_result_okay;
#if GPS_DL_ON_LINUX
	bool ever_init;
	struct completion ack_done;
#endif
};

struct gps_mcudl_mgmt_cmd_state_wrapper g_gps_mcudl_mgmt_cmd_states[GPS_MCUDL_CMD_NUM];

struct gps_mcudl_mgmt_cmd_state_wrapper *gps_mcudl_mgmt_cmd_get_state_ptr(enum gps_mcudl_mgmt_cmd_id cmd_id)
{
	if ((unsigned int)cmd_id >= (unsigned int)GPS_MCUDL_CMD_NUM)
		return NULL;

	return &g_gps_mcudl_mgmt_cmd_states[cmd_id];
}

void gps_mcudl_mgmt_cmd_state_init_all(void)
{
	enum gps_mcudl_mgmt_cmd_id cmd_id;
	struct gps_mcudl_mgmt_cmd_state_wrapper *p_wrapper;
	enum gps_mcudl_mgmt_cmd_state_enum old_state;
	bool ever_init;

	for (cmd_id = 0; cmd_id < GPS_MCUDL_CMD_NUM; cmd_id++) {
		p_wrapper = &g_gps_mcudl_mgmt_cmd_states[cmd_id];
		gps_mcudl_slot_protect();
		old_state = p_wrapper->state;
		p_wrapper->state = GMDL_CMD_IDLE;
		p_wrapper->is_result_okay = false;
#if GPS_DL_ON_LINUX
		ever_init = p_wrapper->ever_init;
		if (!p_wrapper->ever_init) {
			init_completion(&p_wrapper->ack_done);
			p_wrapper->ever_init = true;
		} else {
			if (old_state != GMDL_CMD_IDLE)
				complete_all(&p_wrapper->ack_done);
			reinit_completion(&p_wrapper->ack_done);
		}
#endif
		gps_mcudl_slot_unprotect();
		if (old_state != GMDL_CMD_IDLE)
			MDL_LOGW("cmd_id=%d, ever_init=%d, old_state=%d not idle!", cmd_id, ever_init, old_state);
	}
}

bool gps_mcudl_mgmt_cmd_pre_send(enum gps_mcudl_mgmt_cmd_id cmd_id)
{
	struct gps_mcudl_mgmt_cmd_state_wrapper *p_wrapper;
	enum gps_mcudl_mgmt_cmd_state_enum old_state;

	p_wrapper = gps_mcudl_mgmt_cmd_get_state_ptr(cmd_id);
	if (!p_wrapper) {
		MDL_LOGW("cmd_id=%d, out of range", cmd_id);
		return false;
	}

	gps_mcudl_slot_protect();
	old_state = p_wrapper->state;
	if (old_state == GMDL_CMD_IDLE)
		p_wrapper->state = GMDL_CMD_PRE_SEND;
	gps_mcudl_slot_unprotect();
	if (old_state != GMDL_CMD_IDLE) {
		MDL_LOGW("cmd_id=%d, old_state=%d not idle!", cmd_id, old_state);
		return false;
	}
	return true;
}

void gps_mcudl_mgmt_cmd_on_ack(enum gps_mcudl_mgmt_cmd_id cmd_id)
{
	struct gps_mcudl_mgmt_cmd_state_wrapper *p_wrapper;
	enum gps_mcudl_mgmt_cmd_state_enum old_state;

	p_wrapper = gps_mcudl_mgmt_cmd_get_state_ptr(cmd_id);
	if (!p_wrapper) {
		MDL_LOGW("cmd_id=%d, out of range", cmd_id);
		return;
	}

	gps_mcudl_slot_protect();
	old_state = p_wrapper->state;
	if (p_wrapper->state == GMDL_CMD_WAIT_ACK) {
		p_wrapper->is_result_okay = true;
		p_wrapper->state = GMDL_CMD_ACK_AFTER_WAIT;
#if GPS_DL_ON_LINUX
		complete(&p_wrapper->ack_done);
#endif
	} else if (p_wrapper->state == GMDL_CMD_PRE_SEND) {
		p_wrapper->is_result_okay = true;
		p_wrapper->state = GMDL_CMD_ACK_BEFORE_WAIT;
	}
	gps_mcudl_slot_unprotect();
	if (old_state != GMDL_CMD_WAIT_ACK &&
		old_state != GMDL_CMD_PRE_SEND && old_state != GMDL_CMD_IDLE)
		MDL_LOGW("cmd_id=%d, old_state=%d abnormal", cmd_id, old_state);
}

bool gps_mcudl_mgmt_cmd_wait_ack(enum gps_mcudl_mgmt_cmd_id cmd_id, int timeout_ms)
{
	struct gps_mcudl_mgmt_cmd_state_wrapper *p_wrapper;
	enum gps_mcudl_mgmt_cmd_state_enum old_state;
	bool already_ack = false;
	bool wait_for_comp = false;
	bool is_result_okay;
	bool is_timeout;
#if GPS_DL_ON_LINUX
	unsigned long remaining_jiffies;
#endif

	p_wrapper = gps_mcudl_mgmt_cmd_get_state_ptr(cmd_id);
	if (!p_wrapper) {
		MDL_LOGW("cmd_id=%d, out of range", cmd_id);
		return false;
	}

	gps_mcudl_slot_protect();
	old_state = p_wrapper->state;
	if (p_wrapper->state == GMDL_CMD_ACK_BEFORE_WAIT) {
		already_ack = true;
		is_result_okay = p_wrapper->is_result_okay;
		p_wrapper->state = GMDL_CMD_IDLE;
	} else if (p_wrapper->state == GMDL_CMD_PRE_SEND) {
		p_wrapper->state = GMDL_CMD_WAIT_ACK;
		wait_for_comp = true;
	}
	gps_mcudl_slot_unprotect();

	if (already_ack) {
		GDL_LOGW("cmd_id=%d, old_state=%d, ack before wait, res=%d",
			cmd_id, old_state, is_result_okay);
		return is_result_okay;
	}

	if (!wait_for_comp) {
		GDL_LOGW("cmd_id=%d, old_state=%d, not do wait",
			cmd_id, old_state);
		return false;
	}

	gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_MGMT_CMD);
	is_timeout = false;
#if GPS_DL_ON_LINUX
	remaining_jiffies = wait_for_completion_timeout(
		&p_wrapper->ack_done, msecs_to_jiffies(timeout_ms));
	if (remaining_jiffies == 0)
		is_timeout = true;
#else
	/* may do busy polling until ack_done or timeout */
#endif
	gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_MGMT_CMD);

	gps_mcudl_slot_protect();
	old_state = p_wrapper->state;
	if (p_wrapper->state == GMDL_CMD_ACK_AFTER_WAIT)
		is_result_okay = p_wrapper->is_result_okay;
	else
		is_result_okay = false;
	p_wrapper->state = GMDL_CMD_IDLE;
	gps_mcudl_slot_unprotect();
	if (is_timeout || old_state != GMDL_CMD_ACK_AFTER_WAIT || !is_result_okay) {
		GDL_LOGW("cmd_id=%d, is_timeout=%d, old_state=%d, is_okay=%d",
			cmd_id, is_timeout, old_state, is_result_okay);
		gps_mcu_hif_host_dump_ch(GPS_MCU_HIF_CH_DMALESS_MGMT);
	}
	return is_result_okay;
}


bool gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(bool enable)
{
	bool is_okay = false;
	bool wait_okay = false;

	gps_mcudl_mgmt_cmd_pre_send(GPS_MCUDL_CMD_FW_LOG_CTRL);
	if (enable) {
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT,
			"\x03\x01", 2);
	} else {
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT,
			"\x03\x00", 2);
	}
	MDL_LOGW("write cmd3, is_ok=%d", is_okay);

	if (is_okay)
		wait_okay = gps_mcudl_mgmt_cmd_wait_ack(GPS_MCUDL_CMD_FW_LOG_CTRL, 100);
	MDL_LOGI("write cmd3, is_ok=%d, wait_ok=%d", is_okay, wait_okay);
	return is_okay;
}

