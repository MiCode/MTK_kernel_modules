/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_context.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_link_state.h"
#include "gps_dl_name_list.h"
#include "gps_mcudl_log.h"


void gps_mcudl_each_link_set_bool_flag(enum gps_mcudl_xid link_id,
	enum gps_each_link_bool_state name, bool value)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);

	if (!p)
		return;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	switch (name) {
	case LINK_TO_BE_CLOSED:
		p->sub_states.to_be_closed = value;
		break;
	case LINK_USER_OPEN:
		p->sub_states.user_open = value;
		break;
	case LINK_OPEN_RESULT_OKAY:
		p->sub_states.open_result_okay = value;
		break;
	case LINK_NEED_A2Z_DUMP:
		p->sub_states.need_a2z_dump = value;
		break;
	case LINK_SUSPEND_TO_CLK_EXT:
		p->sub_states.suspend_to_clk_ext = value;
		break;
	case LINK_MISS_MNLBIN_ACK:
		p->sub_states.miss_mnlbin_ack = value;
		break;
	default:
		break; /* do nothing */
	}
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

bool gps_mcudl_each_link_get_bool_flag(enum gps_mcudl_xid link_id,
	enum gps_each_link_bool_state name)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	bool value = false;

	if (!p)
		return false;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	switch (name) {
	case LINK_TO_BE_CLOSED:
		value = p->sub_states.to_be_closed;
		break;
	case LINK_USER_OPEN:
		value = p->sub_states.user_open;
		break;
	case LINK_OPEN_RESULT_OKAY:
		value = p->sub_states.open_result_okay;
		break;
	case LINK_NEED_A2Z_DUMP:
		value = p->sub_states.need_a2z_dump;
		break;
	case LINK_SUSPEND_TO_CLK_EXT:
		value = p->sub_states.suspend_to_clk_ext;
		break;
	case LINK_MISS_MNLBIN_ACK:
		value = p->sub_states.miss_mnlbin_ack;
		break;
	default:
		break; /* TODO: warning it */
	}
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return value;
}

void gps_mcudl_each_link_set_ready_to_write(enum gps_mcudl_xid link_id, bool is_ready)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		p->sub_states.is_ready_to_write = is_ready;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

bool gps_mcudl_each_link_is_ready_to_write(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	bool ready = false;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		ready = p->sub_states.is_ready_to_write;
	else
		ready = false;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return ready;
}

void gps_mcudl_each_link_set_active(enum gps_mcudl_xid link_id, bool is_active)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		p->sub_states.is_active = is_active;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

bool gps_mcudl_each_link_is_active(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	bool ready = false;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		ready = p->sub_states.is_active;
	else
		ready = false;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return ready;
}

void gps_mcudl_each_link_inc_session_id(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	int sid;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p->session_id >= GPS_EACH_LINK_SID_MAX)
		p->session_id = 1;
	else
		p->session_id++;
	sid = p->session_id;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	MDL_LOGXD(link_id, "new sid = %d", sid);
}

int gps_mcudl_each_link_get_session_id(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	int sid;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	sid = p->session_id;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return sid;
}

enum gps_each_link_state_enum gps_mcudl_each_link_get_state(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum gps_each_link_state_enum state;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	state = p->state_for_user;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return state;
}

void gps_mcudl_each_link_set_state(enum gps_mcudl_xid link_id, enum gps_each_link_state_enum state)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum gps_each_link_state_enum pre_state;


	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	pre_state = p->state_for_user;
	p->state_for_user = state;
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	MDL_LOGXD_STA(link_id, "state change: %s -> %s",
		gps_dl_link_state_name(pre_state), gps_dl_link_state_name(state));
}

bool gps_mcudl_each_link_change_state_from(enum gps_mcudl_xid link_id,
	enum gps_each_link_state_enum from, enum gps_each_link_state_enum to)
{
	bool is_okay = false;
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum gps_each_link_state_enum pre_state;
	enum gps_each_link_reset_level old_level, new_level;

	gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	pre_state = p->state_for_user;
	if (from == pre_state) {
		p->state_for_user = to;
		is_okay = true;

		if (to == LINK_RESETTING) {
			old_level = p->reset_level;
			if (old_level < GPS_DL_RESET_LEVEL_GPS_SINGLE_LINK)
				p->reset_level = GPS_DL_RESET_LEVEL_GPS_SINGLE_LINK;
			new_level = p->reset_level;
		}
	}
	gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	if (is_okay && (to == LINK_RESETTING)) {
		MDL_LOGXI_STA(link_id, "state change: %s -> %s, okay, level: %d -> %d",
			gps_dl_link_state_name(from), gps_dl_link_state_name(to),
			old_level, new_level);
	} else if (is_okay) {
		MDL_LOGXI_STA(link_id, "state change: %s -> %s, okay",
			gps_dl_link_state_name(from), gps_dl_link_state_name(to));
	} else {
		MDL_LOGXW_STA(link_id, "state change: %s -> %s, fail on pre_state = %s",
			gps_dl_link_state_name(from), gps_dl_link_state_name(to),
			gps_dl_link_state_name(pre_state));
	}

	return is_okay;
}


#if GPS_DL_ON_LINUX
wait_queue_head_t *gps_mcudl_each_link_poll_get_wq_ptr(enum gps_mcudl_xid x_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	return &(p->waitables[GPS_DL_WAIT_READ].wq);
}
#endif

bool gps_mcudl_each_link_poll_is_in_data_ready(enum gps_mcudl_xid x_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);
	enum GDL_RET_STATUS gdl_ret;
	bool is_ready = false;
#if 0
	unsigned int pkt_cnt;
#endif
	do {
		/* The case should be happening only on EPOLLET used.
		 * It's not the case, so comment it
		 */
#if 0
		/* In normal case, pkt_cnt should be 1.
		 *
		 * Here is a suspected case:
		 * If larger than 1, it means new pkt comes in around
		 *   ep_wait's `ep_send_events` handling.
		 * Due to `gps_mcudl_link_try_wait_on` is not being called yet,
		 *   gps_kctrld will not do `wake_up` in `gps_dl_link_wake_up2`.
		 * We need to make a `wake_up` here in case of
		 *   mnld has 1EPOLL_IN-1Read implementation.
		 */
		pkt_cnt = gps_dma_buf_count_data_entry(&p->rx_dma_buf);
		if (pkt_cnt > 1) {
#if GPS_DL_ON_LINUX
			wake_up(&p->waitables[GPS_DL_WAIT_READ].wq);
#endif
			MDL_LOGXW(x_id, "pkt_cnt=%u, fired=%d, wait=%d", pkt_cnt,
				p->waitables[GPS_DL_WAIT_READ].fired,
				p->waitables[GPS_DL_WAIT_READ].waiting);
			is_ready = true;
			break;
		}
#endif
		gdl_ret = gps_mcudl_link_try_wait_on(x_id, GPS_DL_WAIT_READ);
		if (gdl_ret == GDL_OKAY)
			is_ready = true;
		else
			is_ready = !gps_dma_buf_is_empty(&p->rx_dma_buf);
	} while (0);

	if (is_ready) {
		p->waitables[GPS_DL_WAIT_READ].waiting = false;
		p->epoll_flag = true;
	}
	return is_ready;
}

