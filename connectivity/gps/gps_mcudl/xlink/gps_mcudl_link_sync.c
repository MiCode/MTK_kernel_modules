/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_log.h"
#include "gps_dl_name_list.h"
#include "gps_dl_hal.h"
#if GPS_DL_ON_LINUX
#include <asm/bitops.h>
#endif

void gps_mcudl_link_waitable_reset(enum gps_mcudl_xid x_id,
	enum gps_each_link_waitable_type type)
{
	struct gps_mcudl_each_link *p_link;

	p_link = gps_mcudl_link_get(x_id);
#if GPS_DL_ON_LINUX
	clear_bit(0, &p_link->waitables[type].fired);
#else
	p_link->waitables[type].fired = false;
#endif
}

enum GDL_RET_STATUS gps_mcudl_link_try_wait_on(enum gps_mcudl_xid x_id,
	enum gps_each_link_waitable_type type)
{
	struct gps_mcudl_each_link *p_link;
	struct gps_each_link_waitable *p;

	p_link = gps_mcudl_link_get(x_id);
	p = &p_link->waitables[type];
	return gps_dl_waitable_try_wait_on(p);
}

void gps_mcudl_link_open_wait(enum gps_mcudl_xid link_id, long *p_sigval)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	long sigval;

	gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE], &sigval);
	if (gdl_ret == GDL_FAIL_SIGNALED) {
		/*
		 * LINK_OPENING should be a temp state. if code arriving here,
		 * it means something block LINK_OPENING changing to LINK_OPENED
		 * use the api to dump and check whether it's blocked by conn infra driver operation
		 */
		/*gps_dl_hal_conn_infra_driver_debug_dump();*/
		if (p_sigval != NULL) {
			*p_sigval = sigval;
			return;
		}
	} else if (gdl_ret == GDL_FAIL_NOT_SUPPORT)
		; /* show warnning */
}

void gps_mcudl_link_open_ack(enum gps_mcudl_xid link_id, bool okay)
{
#if 0
	enum GDL_RET_STATUS gdl_ret;
	struct gdl_dma_buf_entry dma_buf_entry;
#endif
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	bool send_msg = false;

	MDL_LOGXD_ONF(link_id, "");

	gps_mcudl_each_link_take_big_lock(link_id, GDL_LOCK_FOR_OPEN_DONE);
	if (gps_mcudl_each_link_get_bool_flag(link_id, LINK_USER_OPEN) && okay) {
		MDL_LOGXD_ONF(link_id,
			"user still online, try to change to opened");

		/* Note: if pre_status not OPENING, it might be RESETTING, not handle it here */
		gps_mcudl_each_link_change_state_from(link_id, LINK_OPENING, LINK_OPENED);
		if (gps_mcudl_each_link_get_bool_flag(link_id, LINK_MISS_MNLBIN_ACK))
			gps_mcudl_each_link_set_bool_flag(link_id, LINK_MISS_MNLBIN_ACK, false);

		/* TODO: ack on DSP reset done */
#if 0
		/* if has pending data, can send it now */
		gdl_ret = gdl_dma_buf_get_data_entry(&p->tx_dma_buf, &dma_buf_entry);
		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_a2d_tx_dma_start(link_id, &dma_buf_entry);
#endif
	} else {
		MDL_LOGXW_ONF(link_id,
			"okay = %d or user already offline, try to change to closing", okay);

		/* Note: if pre_status not OPENING, it might be RESETTING, not handle it here */
		if (gps_mcudl_each_link_change_state_from(link_id, LINK_OPENING, LINK_CLOSING))
			send_msg = true;
	}

	gps_mcudl_each_link_set_bool_flag(link_id, LINK_OPEN_RESULT_OKAY, okay);
	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE]);

	gps_mcudl_each_link_give_big_lock(link_id);

	if (send_msg) {
		gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_CLOSE);
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, true);
	}
}

void gps_mcudl_link_close_wait(enum gps_mcudl_xid link_id, long *p_sigval)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	long sigval;

	gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE], &sigval);
	if (gdl_ret == GDL_FAIL_SIGNALED) {
		if (p_sigval != NULL) {
			*p_sigval = sigval;
			return;
		}
	} else if (gdl_ret == GDL_FAIL_NOT_SUPPORT)
		; /* show warnning */
}

void gps_mcudl_link_close_ack(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);

	MDL_LOGXD_ONF(link_id, "");

	gps_mcudl_each_link_take_big_lock(link_id, GDL_LOCK_FOR_CLOSE_DONE);
	gps_mcudl_each_link_change_state_from(link_id, LINK_CLOSING, LINK_CLOSED);

	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE]);
	gps_mcudl_each_link_give_big_lock(link_id);
}


static bool gps_mcudl_link_try_to_clear_all_resetting_status(void)
{
	enum gps_mcudl_xid link_id;
	struct gps_mcudl_each_link *p = NULL;

	for (link_id = 0; link_id < GPS_MDLX_CH_NUM; link_id++) {
		p = gps_mcudl_link_get(link_id);
		if (p->reset_level != GPS_DL_RESET_LEVEL_NONE) {
			MDL_LOGXD(link_id, "reset_level=%d, return false", p->reset_level);
			return false;
		}
	}

	for (link_id = 0; link_id < GPS_MDLX_CH_NUM; link_id++) {
		p = gps_mcudl_link_get(link_id);

		if (p->sub_states.user_open)
			p->state_for_user = LINK_RESET_DONE;
		else
			p->state_for_user = LINK_CLOSED;
	}

	return true;
}

static void gps_mcudl_link_reset_ack_inner(enum gps_mcudl_xid link_id, bool post_conn_reset)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum gps_each_link_state_enum old_state, new_state;
	enum gps_each_link_reset_level old_level, new_level;
	bool user_still_open = false;
	bool all_clear_done = false;
	bool try_conn_infra_off = false;

	gps_mcudl_each_link_take_big_lock(link_id, GDL_LOCK_FOR_RESET_DONE);
	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	old_state = p->state_for_user;
	old_level = p->reset_level;
	user_still_open = p->sub_states.user_open;

	switch (old_level) {
	case GPS_DL_RESET_LEVEL_GPS_SINGLE_LINK:
		p->reset_level = GPS_DL_RESET_LEVEL_NONE;
		if (p->sub_states.user_open)
			p->state_for_user = LINK_RESET_DONE;
		else
			p->state_for_user = LINK_CLOSED;
		break;

	case GPS_DL_RESET_LEVEL_CONNSYS:
		if (!post_conn_reset)
			break;
		p->reset_level = GPS_DL_RESET_LEVEL_NONE;
		all_clear_done = gps_mcudl_link_try_to_clear_all_resetting_status();
		try_conn_infra_off = true;
		break;

	case GPS_DL_RESET_LEVEL_GPS_SUBSYS:
		p->reset_level = GPS_DL_RESET_LEVEL_NONE;
		all_clear_done = gps_mcudl_link_try_to_clear_all_resetting_status();
		break;

	default:
		break;
	}

	new_state = p->state_for_user;
	new_level = p->reset_level;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	if (try_conn_infra_off) {
		/* During connsys resetting, conninfra_pwr_off may fail,
		 * it need to be called again when connsys reset done.
		 */
		gps_dl_hal_conn_infra_driver_off();
	}

	/* TODO: if both clear, show another link's log */
	MDL_LOGXE_STA(link_id,
		"state change: %s -> %s, level: %d -> %d, user = %d, post_reset = %d, all_clear = %d",
		gps_dl_link_state_name(old_state), gps_dl_link_state_name(new_state),
		old_level, new_level,
		user_still_open, post_conn_reset, all_clear_done);

	gps_mcudl_each_link_give_big_lock(link_id);

	/* Note: for CONNSYS or GPS_SUBSYS RESET, here might be still RESETTING,
	 * if any other link not reset done (see all_clear_done print).
	 */
	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_RESET]);
}

void gps_mcudl_link_reset_ack(enum gps_mcudl_xid link_id)
{
	gps_mcudl_link_reset_ack_inner(link_id, false);
}

void gps_mcudl_link_on_post_conn_reset(enum gps_mcudl_xid link_id)
{
	gps_mcudl_link_reset_ack_inner(link_id, true);
}

int gps_mcudl_link_wait_state_ntf(enum gps_mcudl_xid x_id, long *p_sigval)
{
	struct gps_mcudl_each_link *p_link;

	enum GDL_RET_STATUS gdl_ret;
	long sigval;

	p_link = gps_mcudl_link_get(x_id);
	gdl_ret = gps_dl_link_wait_on(&p_link->waitables[GPS_DL_WAIT_STATE_NTF], &sigval);
	if (gdl_ret == GDL_FAIL_SIGNALED) {
		if (p_sigval != NULL) {
			*p_sigval = sigval;
			return -ERESTARTSYS;
		}
	} else if (gdl_ret == GDL_FAIL_NOT_SUPPORT) {
		MDL_LOGXW_ONF(x_id, "not support to wait");
		return -EINVAL;
	}

	return 0;
}

void gps_mcudl_link_trigger_state_ntf(enum gps_mcudl_xid x_id)
{
	struct gps_mcudl_each_link *p_link;

	p_link = gps_mcudl_link_get(x_id);

	gps_dl_link_wake_up(&p_link->waitables[GPS_DL_WAIT_STATE_NTF]);
}

void gps_mcudl_link_trigger_state_ntf_all(void)
{
	enum gps_mcudl_xid x_id;

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++)
		gps_mcudl_link_trigger_state_ntf(x_id);
}
