/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_time_tick.h"

#include "gps_each_link.h"
#include "gps_dl_hal.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hal_util.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_dl_lib_misc.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_osal.h"
#include "gps_dl_name_list.h"
#include "gps_dl_context.h"
#include "gps_dl_subsys_reset.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_log.h"
#include "linux/errno.h"


void gps_mcudl_each_link_init(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);

	p->session_id = 0;
	p->epoll_flag = false;
	gps_mcudl_each_link_mutexes_init(p);
	gps_mcudl_each_link_spin_locks_init(p);
	gps_mcudl_each_link_set_active(link_id, false);
	gps_mcudl_each_link_set_ready_to_write(link_id, false);
	gps_mcudl_each_link_context_init(link_id);
	gps_mcudl_each_link_set_state(link_id, LINK_CLOSED);
}

void gps_mcudl_each_link_deinit(enum gps_mcudl_xid link_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);

	gps_mcudl_each_link_set_state(link_id, LINK_UNINIT);
	gps_mcudl_each_link_mutexes_deinit(p);
	gps_mcudl_each_link_spin_locks_deinit(p);
}

void gps_mcudl_each_link_context_init(enum gps_mcudl_xid link_id)
{
	enum gps_each_link_waitable_type j;

	for (j = 0; j < GPS_DL_WAIT_NUM; j++)
		gps_mcudl_each_link_waitable_reset(link_id, j);
}

void gps_mcudl_each_link_context_clear(enum gps_mcudl_xid link_id)
{
	gps_mcudl_each_link_waitable_reset(link_id, GPS_DL_WAIT_WRITE);
	gps_mcudl_each_link_waitable_reset(link_id, GPS_DL_WAIT_READ);
}

int gps_mcudl_each_link_open(enum gps_mcudl_xid link_id)
{
	enum gps_each_link_state_enum state, state2;
	enum GDL_RET_STATUS gdl_ret;
	long sigval = 0;
	bool okay = false;
	bool is_twice_check = false;
	int retval;
#if GPS_DL_ON_CTP
	/* Todo: is it need on LINUX? */
	struct gps_mcudl_each_link *p_link = gps_mcudl_link_get(link_id);

	gps_dma_buf_align_as_byte_mode(&p_link->tx_dma_buf);
	gps_dma_buf_align_as_byte_mode(&p_link->rx_dma_buf);
#endif

#if 0
#if (GPS_DL_ON_LINUX && !GPS_DL_NO_USE_IRQ && !GPS_DL_HW_IS_MOCK)
	if (!p_link->sub_states.irq_is_init_done) {
		gps_dl_irq_init();
		p_link->sub_states.irq_is_init_done = true;
	}
#endif
#endif

	state = gps_mcudl_each_link_get_state(link_id);

	switch (state) {
	case LINK_RESUMING:
	case LINK_SUSPENDED:
	case LINK_SUSPENDING:
		retval = -EAGAIN;
		break;

	case LINK_CLOSING:
	case LINK_RESETTING:
	case LINK_DISABLED:
		retval = -EAGAIN;
		break;

	case LINK_RESET_DONE:
		/* RESET_DONE stands for user space not close me */
		retval = -EBUSY; /* twice open not allowed */
		break;

	case LINK_OPENED:
	case LINK_OPENING:
		retval = -EBUSY;; /* twice open not allowed */
		break;

	case LINK_CLOSED:
		okay = gps_mcudl_each_link_change_state_from(link_id, LINK_CLOSED, LINK_OPENING);
		if (!okay) {
			retval = -EBUSY;
			break;
		}

		/* TODO: simplify the flags */
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, false);
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_USER_OPEN, true);

		gps_mcudl_each_link_waitable_reset(link_id, GPS_DL_WAIT_OPEN_CLOSE);
		gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_OPEN);
		gps_mcudl_link_open_wait(link_id, &sigval);

		/* TODO: Check this mutex can be removed?
		 * the possible purpose is make it's atomic from LINK_USER_OPEN and LINK_OPEN_RESULT_OKAY.
		 */
		gps_mcudl_each_link_take_big_lock(link_id, GDL_LOCK_FOR_OPEN);
		if (sigval == 0) {
			/* Not signalled */
			okay = gps_mcudl_each_link_get_bool_flag(link_id, LINK_OPEN_RESULT_OKAY);
			gps_mcudl_each_link_give_big_lock(link_id);

			if (okay)
				retval = 0;
			else {
				gps_mcudl_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
				retval = -EBUSY;
			}
			break;
		}

		/* It is signalled */
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		is_twice_check = false;
_check_again:
		gdl_ret = gps_mcudl_link_try_wait_on(link_id, GPS_DL_WAIT_OPEN_CLOSE);
		if (gdl_ret == GDL_OKAY) {
			okay = gps_mcudl_each_link_change_state_from(link_id, LINK_OPENED, LINK_CLOSING);

			/* Change okay, need to send event to trigger close */
			if (okay) {
				gps_mcudl_each_link_give_big_lock(link_id);
				gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_CLOSE);
				MDL_LOGXW_ONF(link_id,
					"is_twice=%d, sigval=%ld, corner case 1",
					is_twice_check, sigval);
				retval = -EBUSY;
				break;
			}

			/* Still not change okay, state maybe RESETTING or RESET_DONE */
			state2 = gps_mcudl_each_link_get_state(link_id);
			if (state2 == LINK_RESET_DONE)
				gps_mcudl_each_link_set_state(link_id, LINK_CLOSED);

			gps_mcudl_each_link_give_big_lock(link_id);
			MDL_LOGXW_ONF(link_id, "is_twice=%d, sigval=%ld, corner case 2, st=%s",
				is_twice_check, sigval, gps_dl_link_state_name(state2));
			retval = -EBUSY;
			break;
		}

		if (!is_twice_check)
			okay = gps_mcudl_each_link_change_state_from(link_id, LINK_OPENING, LINK_CLOSING);
		else {
			/* Currrently, due to LINK_OPENING should not be long,
			 * no need to handle it by reset
			 */
#if 0
			/* gps_dl_trigger_connsys_reset(); */
			gps_mcudl_trigger_gps_subsys_reset(false, "GNSS opening fail");
			okay = false;
#else
			okay = gps_mcudl_each_link_change_state_from(link_id, LINK_OPENING, LINK_CLOSING);
#endif
		}
		if (!okay) {
			/* Not change okay, try again */
			if (!is_twice_check) {
				is_twice_check = true;
				goto _check_again;
			}

			/* Not change okay, state should be RESETTING */
			state2 = gps_mcudl_each_link_get_state(link_id);
			if (state2 == LINK_RESET_DONE)
				gps_mcudl_each_link_set_state(link_id, LINK_CLOSED);

			gps_mcudl_each_link_give_big_lock(link_id);
			MDL_LOGXW_ONF(link_id, "is_twice=%d, sigval=%ld, corner case 3: st=%s",
				is_twice_check, sigval, gps_dl_link_state_name(state2));
			retval = -EBUSY;
			break;
		}

		/* Change okay from OPENING to CLOSING,*/
		/* need to send event to trigger close */
		gps_mcudl_each_link_give_big_lock(link_id);
		gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_CLOSE);
		MDL_LOGXW_ONF(link_id, "is_twice=%d, sigval=%ld, normal case",
			is_twice_check, sigval);

		/* sigval should be -ERESTARTSYS */
		retval = sigval;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	if (retval == 0) {
		MDL_LOGXD_ONF(link_id, "prev_state = %s, retval = %d",
			gps_dl_link_state_name(state), retval);
	} else {
		MDL_LOGXW_ONF(link_id, "prev_state = %s, retval = %d",
			gps_dl_link_state_name(state), retval);
	}

	return retval;
}

int gps_mcudl_each_link_send_reset_evt(enum gps_mcudl_xid link_id)
{
	gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_RESET2);
	return 0;
}

int gps_mcudl_each_link_reset(enum gps_mcudl_xid link_id)
{
	/*
	 * - set each link resetting flag
	 */
	enum gps_each_link_state_enum state, state2;
	bool okay = false;
	int retval;

	state = gps_mcudl_each_link_get_state(link_id);

	switch (state) {
	case LINK_OPENING:
	case LINK_CLOSING:
	case LINK_CLOSED:
	case LINK_DISABLED:
		retval = -EBUSY;
		break;

	case LINK_RESETTING:
	case LINK_RESET_DONE:
		retval = 0;
		break;

	case LINK_RESUMING:
	case LINK_SUSPENDING:
	case LINK_SUSPENDED:
	case LINK_OPENED:
_try_change_to_reset_again:
		okay = gps_mcudl_each_link_change_state_from(link_id, state, LINK_RESETTING);
		if (!okay) {
			state2 = gps_mcudl_each_link_get_state(link_id);

			/* Already reset or close, not trigger reseeting again */
			MDL_LOGXW_ONF(link_id, "state flip to %s - corner case",
				gps_dl_link_state_name(state2));

			/* -ing state may become -ed state, try change to reset again */
			if ((state == LINK_SUSPENDING && state2 == LINK_SUSPENDED) ||
				(state == LINK_RESUMING && state2 == LINK_OPENED)) {
				state = state2;
				goto _try_change_to_reset_again;
			}

			if (state2 == LINK_RESETTING || state2 == LINK_RESET_DONE)
				retval = 0;
			else
				retval = -EBUSY;
			break;
		}

		gps_mcudl_each_link_set_bool_flag(link_id, LINK_IS_RESETTING, true);

		/* no need to wait reset ack
		 * TODO: make sure message send okay
		 */
		gps_mcudl_link_waitable_reset(link_id, GPS_DL_WAIT_RESET);
		gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_RESET);
		retval = 0;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	/* wait until cttld thread ack the status */
	if (retval == 0) {
		MDL_LOGXD_ONF(link_id, "prev_state = %s, retval = %d",
			gps_dl_link_state_name(state), retval);
	} else {
		MDL_LOGXW_ONF(link_id, "prev_state = %s, retval = %d",
			gps_dl_link_state_name(state), retval);
	}

	return retval;
}

int gps_mcudl_each_link_close_or_suspend_inner(enum gps_mcudl_xid link_id,
	enum gps_each_link_state_enum state,
	enum gps_each_link_close_or_suspend_op close_or_suspend_op)
{
	enum gps_each_link_state_enum state2;
	bool okay = false;
	int retval = 0;
	bool hw_suspend;

	hw_suspend = !!(close_or_suspend_op == GDL_DPSTOP || close_or_suspend_op == GDL_CLKEXT);
	gps_mcudl_each_link_take_big_lock(link_id, GDL_LOCK_FOR_CLOSE);
	do {
		if (hw_suspend) {
			okay = gps_mcudl_each_link_change_state_from(link_id, LINK_OPENED, LINK_SUSPENDING);
			if (!okay) {
				state2 = gps_mcudl_each_link_get_state(link_id);
				gps_mcudl_each_link_give_big_lock(link_id);
				MDL_LOGXW_ONF(link_id, "state check: %s, return hw suspend fail",
					gps_dl_link_state_name(state2));
				retval = -EINVAL;
				break;
			}

			gps_mcudl_each_link_set_bool_flag(link_id,
				LINK_SUSPEND_TO_CLK_EXT, close_or_suspend_op == GDL_CLKEXT);
		} else {
			if (state == LINK_SUSPENDED) {
				okay = gps_mcudl_each_link_change_state_from(
					link_id, LINK_SUSPENDED, LINK_CLOSING);
			} else {
				okay = gps_mcudl_each_link_change_state_from(
					link_id, LINK_OPENED, LINK_CLOSING);
			}
			gps_mcudl_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
			if (!okay) {
				state2 = gps_mcudl_each_link_get_state(link_id);
				if (state2 == LINK_RESET_DONE)
					gps_mcudl_each_link_set_state(link_id, LINK_CLOSED);
				else {
					MDL_LOGXW_ONF(link_id, "state check: %s, return close ok",
						gps_dl_link_state_name(state2));
				}
				gps_mcudl_each_link_give_big_lock(link_id);
				retval = 0;
				break;
			}
		}
		gps_mcudl_each_link_give_big_lock(link_id);
	} while (0);
	return retval;
}

int gps_mcudl_each_link_close_or_suspend(enum gps_mcudl_xid link_id,
	enum gps_each_link_close_or_suspend_op close_or_suspend_op)
{
	enum gps_each_link_state_enum state;
	long sigval = 0;
	int retval;
	bool hw_suspend;

	state = gps_mcudl_each_link_get_state(link_id);
	hw_suspend = !!(close_or_suspend_op == GDL_DPSTOP || close_or_suspend_op == GDL_CLKEXT);

	switch (state) {
	case LINK_OPENING:
	case LINK_CLOSING:
	case LINK_CLOSED:
	case LINK_DISABLED:
		/* twice close */
		/* TODO: show user open flag */
		retval = -EINVAL;
		break;

	case LINK_SUSPENDING:
	case LINK_RESUMING:
	case LINK_RESETTING:
		if (hw_suspend) {
			if (state == LINK_SUSPENDING)
				retval = 0;
			else if (state == LINK_RESUMING)
				retval = -EBUSY;
			else
				retval = -EINVAL;
			break;
		}

		/* close on xxx-ing states: just recording user is not online
		 * ctrld will handle it on the end of xxx-ing handling
		 */
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		MDL_LOGXE_ONF(link_id, "state check: %s, return close ok", gps_dl_link_state_name(state));
		/* return okay to avoid twice close */
		retval = 0;
		break;

	case LINK_RESET_DONE:
		if (hw_suspend) {
			retval = -EINVAL;
			break;
		}
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		gps_mcudl_each_link_set_state(link_id, LINK_CLOSED);
		retval = 0;
		break;

	case LINK_SUSPENDED:
	case LINK_OPENED:
		retval = gps_mcudl_each_link_close_or_suspend_inner(link_id, state, close_or_suspend_op);
		if (retval != 0)
			break;

		/* clean the done(fired) flag before send and wait */
		gps_mcudl_each_link_waitable_reset(link_id, GPS_DL_WAIT_OPEN_CLOSE);
		gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_CLOSE);

		/* set this status, hal proc will by pass the message from the link
		 * it can make LINK_CLOSE be processed faster
		 */
		gps_mcudl_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, true);
		gps_mcudl_link_close_wait(link_id, &sigval);

		if (sigval) {
			retval = -EINVAL;
			break;
		}

		retval = 0;
		break;
	default:
		retval = -EINVAL;
		break;
	}

	if (retval == 0) {
		MDL_LOGXD_ONF(link_id, "prev_state = %s, retval = %d, op = %d",
			gps_dl_link_state_name(state), retval, close_or_suspend_op);
	} else {
		MDL_LOGXW_ONF(link_id, "prev_state = %s, retval = %d, op = %d",
			gps_dl_link_state_name(state), retval, close_or_suspend_op);
	}

	return retval;
}

int gps_mcudl_each_link_close(enum gps_mcudl_xid link_id)
{
	return gps_mcudl_each_link_close_or_suspend(link_id, GDL_CLOSE);
}

int gps_mcudl_each_link_check(enum gps_mcudl_xid link_id, int reason)
{
	enum gps_each_link_state_enum state;
	enum gps_mcudl_xlink_event_id event;
	int retval = 0;

	state = gps_mcudl_each_link_get_state(link_id);

	switch (state) {
	case LINK_OPENING:
	case LINK_CLOSING:
	case LINK_CLOSED:
	case LINK_DISABLED:
		break;

	case LINK_RESETTING:
#if 0
		if (rstflag == 1) {
			/* chip resetting */
			retval = -888;
		} else if (rstflag == 2) {
			/* chip reset end */
			retval = -889;
		} else {
			/* normal */
			retval = 0;
		}
#endif
		retval = -888;
		break;

	case LINK_RESET_DONE:
		retval = -889;
		break;

	case LINK_RESUMING:
	case LINK_SUSPENDING:
	case LINK_SUSPENDED:
	case LINK_OPENED:
		if (reason == 2)
			event = GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS;
		else if (reason == 4)
			event = GPS_MCUDL_EVT_LINK_PRINT_DATA_STATUS;
		else
			break;
#if 0
		/* if L1 trigger it, also print L5 status
		 * for this case, dump L5 firstly.
		 */
		if (link_id == GPS_DATA_LINK_ID0)
			gps_mcudl_xlink_event_send(event, GPS_DATA_LINK_ID1);
#endif
		gps_mcudl_xlink_event_send(link_id, event);
		break;

	default:
		break;
	}

	MDL_LOGXW_ONF(link_id, "prev_state = %s, reason = %d, retval = %d",
		gps_dl_link_state_name(state), reason, retval);

	return retval;
}


/* TODO: determine return value type */
int gps_mcudl_each_link_write(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len)
{
	return gps_mcudl_each_link_write_with_opt(link_id, buf, len, true);
}

#define GPS_DL_READ_SHOW_BUF_MAX_LEN (32)
int gps_mcudl_each_link_write_with_opt_legacy(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len, bool wait_tx_done)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	long sigval = 0;

	if (NULL == p)
		return -EINVAL;

	if (len > p->tx_dma_buf.len)
		return -EINVAL;

	if (gps_mcudl_each_link_get_state(link_id) != LINK_OPENED) {
		MDL_LOGXW_DRW(link_id, "not opened, drop the write data len = %d", len);
		return -EBUSY;
	}

	if (len <= GPS_DL_READ_SHOW_BUF_MAX_LEN)
		gps_dl_hal_show_buf("wr_buf", buf, len);
	else
		MDL_LOGXD_DRW(link_id, "wr_buf, len = %d", len);

	while (1) {
		gdl_ret = gdl_dma_buf_put(&p->tx_dma_buf, buf, len);

		if (gdl_ret == GDL_OKAY) {
			gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_WRITE);
#if (GPS_DL_USE_POLLING)
			if (wait_tx_done) {
				do {
					gps_dl_hal_a2d_tx_dma_wait_until_done_and_stop_it(
						link_id, GPS_DL_RW_NO_TIMEOUT, false);
					gps_dl_hal_event_send(GPS_DL_HAL_EVT_A2D_TX_DMA_DONE, link_id);
					/* for case tx transfer_max > 0, GPS_DL_HAL_EVT_A2D_TX_DMA_DONE may */
					/* start anthor dma session again, need to loop again until all data done */
				} while (!gps_dma_buf_is_empty(&p->tx_dma_buf));
			}
#endif
			return 0;
		} else if (gdl_ret == GDL_FAIL_NOSPACE || gdl_ret == GDL_FAIL_BUSY ||
			gdl_ret == GDL_FAIL_NOENTRY) {
			/* TODO: */
			/* 1. note: BUSY stands for others thread is do write, it should be impossible */
			/* - If wait on BUSY, should wake up the waitings or return eno_again? */
			/* 2. note: NOSPACE stands for need wait for tx dma working done */
			gps_dma_buf_show(&p->tx_dma_buf, false);
			MDL_LOGXD(link_id,
				"wait due to gdl_dma_buf_put ret = %s", gdl_ret_to_name(gdl_ret));
			gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_WRITE], &sigval);
			if (gdl_ret == GDL_FAIL_SIGNALED)
				return -ERESTARTSYS;
		} else {
			gps_dma_buf_show(&p->tx_dma_buf, true);
			MDL_LOGXW(link_id,
				"fail due to gdl_dma_buf_put ret = %s", gdl_ret_to_name(gdl_ret));
			break;
		}
	}

	return -EFAULT;
}

int gps_mcudl_each_link_write_with_opt(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len, bool wait_tx_done)
{
	struct gps_mcudl_each_link *p_xlink = gps_mcudl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	bool send_is_okay = false;
	long sigval = 0;

	if (NULL == p_xlink)
		return -EINVAL;

	if (len > GPSMDL_PKT_PAYLOAD_MAX)
		return -EINVAL;

	if (gps_mcudl_each_link_get_state(link_id) != LINK_OPENED) {
		MDL_LOGXW_DRW(link_id, "not opened, drop the write data len = %d", len);
		return -EBUSY;
	}

	if (len <= GPS_DL_READ_SHOW_BUF_MAX_LEN)
		gps_dl_hal_show_buf("wr_buf", buf, len);
	else
		MDL_LOGXI_DRW(link_id, "wr_buf, len = %d", len);

	while (1) {
		bool to_notify = true;

		send_is_okay = gps_mcudl_ap2mcu_xdata_send_v2(link_id, buf, len, &to_notify);
		if (send_is_okay) {
			if (to_notify)
				gps_mcudl_xlink_event_send(link_id, GPS_MCUDL_EVT_LINK_WRITE);
			return 0;
		}

		MDL_LOGXI(link_id, "gps_dl_link_wait_on: start");
		gdl_ret = gps_dl_link_wait_on(&p_xlink->waitables[GPS_DL_WAIT_WRITE], &sigval);
		MDL_LOGXI(link_id, "gps_dl_link_wait_on: ret=%s", gdl_ret_to_name(gdl_ret));
		if (gdl_ret == GDL_FAIL_SIGNALED)
			return -ERESTARTSYS;
	}

	return -EFAULT;
}


int gps_mcudl_each_link_read(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len)
{
	return gps_mcudl_each_link_read_with_timeout(link_id,
		buf, len, GPS_DL_RW_NO_TIMEOUT, NULL);
}

int gps_mcudl_each_link_read_with_timeout(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len, int timeout_usec, bool *p_is_nodata)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
#if (!GPS_DL_USE_POLLING)
	long sigval = 0;
#endif
	unsigned int data_len;

	if (NULL == p)
		return -EINVAL;

	while (1) {
		gdl_ret = gdl_dma_buf_get(&p->rx_dma_buf, buf, len, &data_len, p_is_nodata);

		if (gdl_ret == GDL_OKAY) {
			if (data_len <= GPS_DL_READ_SHOW_BUF_MAX_LEN) {
				/* comment it to reduce log
				 * gps_dl_hal_show_buf("rd_buf", buf, data_len);
				 */
			} else
				MDL_LOGXD_DRW(link_id, "rd_buf, len = %d", data_len);

			gps_mcudl_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);
			if (p->rx_dma_buf.has_pending_rx) {
				p->rx_dma_buf.has_pending_rx = false;
				gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);

				MDL_LOGXI_DRW(link_id, "has pending rx, trigger again");
				/*gps_dl_hal_event_send(GPS_DL_HAL_EVT_D2A_RX_HAS_DATA, link_id);*/
			} else
				gps_mcudl_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);

			return data_len;
		} else if (gdl_ret == GDL_FAIL_NODATA) {
			MDL_LOGXD_DRW(link_id, "gdl_dma_buf_get no data and wait");
#if (GPS_DL_USE_POLLING)
			gdl_ret = gps_dl_hal_wait_and_handle_until_usrt_has_data(
				link_id, timeout_usec);
			if (gdl_ret == GDL_FAIL_TIMEOUT)
				return -EAGAIN;

			gdl_ret = gps_dl_hal_wait_and_handle_until_usrt_has_nodata_or_rx_dma_done(
				link_id, timeout_usec, true);
			if (gdl_ret == GDL_FAIL_TIMEOUT)
				return -EAGAIN;
			continue;
#else
			MDL_LOGXW_DRW(link_id, "gdl_dma_buf_get no data, poll_flag=%d", p->epoll_flag);
			if (p->epoll_flag) {
				gdl_ret = gps_mcudl_link_try_wait_on(link_id, GPS_DL_WAIT_READ);
				if (gdl_ret == GDL_OKAY) {
					MDL_LOGXW_DRW(link_id, "gdl_dma_buf_get no data, poll continue");
					continue;
				}
				p->epoll_flag = false;
				return 0;
			}
			gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_READ], &sigval);
			MDL_LOGXW_DRW(link_id, "gdl_dma_buf_get wait success, gdl_ret=%d", gdl_ret);
			if (gdl_ret == GDL_FAIL_SIGNALED)
				return -ERESTARTSYS;
			else if (gdl_ret == GDL_FAIL_NOT_SUPPORT)
				return -EFAULT;
#endif
		} else {
			MDL_LOGXW_DRW(link_id, "gdl_dma_buf_get fail %s", gdl_ret_to_name(gdl_ret));
			return -EFAULT;
		}
	}

	return 0;
}


void gps_mcudl_each_link_rec_poll(enum gps_mcudl_xid x_id, int pid, unsigned int mask)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	if (mask == 0)
		p->rec.poll_zero_us = gps_dl_tick_get_us();
	else
		p->rec.poll_non_zero_us = gps_dl_tick_get_us();
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
}

void gps_mcudl_each_link_rec_read_start(enum gps_mcudl_xid x_id, int pid, int len)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	p->rec.is_reading = true;
	p->rec.reading_us = gps_dl_tick_get_us();
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
}

void gps_mcudl_each_link_rec_read_end(enum gps_mcudl_xid x_id, int pid, int len)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	p->rec.is_reading = false;
	p->rec.last2_read_retlen = p->rec.last_read_retlen;
	p->rec.last_read_retlen = len;
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
}

void gps_mcudl_each_link_rec_write_start(enum gps_mcudl_xid x_id, int pid, int len)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	p->rec.is_writing = true;
	p->rec.writing_us = gps_dl_tick_get_us();
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
}

void gps_mcudl_each_link_rec_write_end(enum gps_mcudl_xid x_id, int pid, int len)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	p->rec.is_writing = false;
	p->rec.last2_write_retlen = p->rec.last_write_retlen;
	p->rec.last_write_retlen = len;
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
}

void gps_mcudl_each_link_rec_dump(enum gps_mcudl_xid x_id)
{
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);
	enum gps_each_link_state_enum state;
	unsigned int pending_r_count, pending_w_count;

	state = gps_mcudl_each_link_get_state(x_id);
	if (state == LINK_CLOSED || state == LINK_DISABLED || state == LINK_UNINIT)
		return;

	pending_r_count = gps_dma_buf_count_data_entry(&p->rx_dma_buf);
	pending_w_count = gps_dma_buf_count_data_entry(&p->tx_dma_buf);

	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	MDL_LOGXW(x_id, "p0/1_us=%lu,%lu, r=%d,%lu,%d,%d, w=%d,%lu,%d,%d, pkt_r/w=%u,%u",
		p->rec.poll_zero_us, p->rec.poll_non_zero_us,
		p->rec.is_reading, p->rec.reading_us, p->rec.last_read_retlen,
		p->rec.last2_read_retlen,
		p->rec.is_writing, p->rec.writing_us, p->rec.last_write_retlen,
		p->rec.last2_write_retlen,
		pending_r_count, pending_w_count);
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
}

void gps_mcudl_xlink_dump_all_rec(void)
{
	enum gps_mcudl_xid x_id;

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++)
		gps_mcudl_each_link_rec_dump(x_id);
}

bool gps_mcudl_xlink_is_in_state_to_listen(enum gps_each_link_state_enum state)
{
	return (state == LINK_RESETTING || state == LINK_RESET_DONE ||
		state == LINK_CLOSING || state == LINK_CLOSED ||
		state == LINK_DISABLED || state == LINK_UNINIT);
}

int gps_mcudl_each_link_listen_state_ntf(enum gps_mcudl_xid x_id)
{
	int wait_ret;
	int retval;
	long sigval = 0;
	enum gps_each_link_state_enum state, pre_state;
	struct gps_dl_gps_awake_status gps_awake_status;
	unsigned long tick0, tick1;

	pre_state = gps_mcudl_each_link_get_state(x_id);
	if (gps_mcudl_xlink_is_in_state_to_listen(pre_state)) {
		MDL_LOGXW(x_id, "state=%s, not wait",
			gps_dl_link_state_name(pre_state));
		return 0;
	}

	MDL_LOGXD(x_id, "state=%s, go waiting", gps_dl_link_state_name(pre_state));
	tick0 = gps_dl_tick_get_us();
	gps_mcudl_each_link_waitable_reset(x_id, GPS_DL_WAIT_STATE_NTF);
	wait_ret = gps_mcudl_link_wait_state_ntf(x_id, &sigval);
	state = gps_mcudl_each_link_get_state(x_id);
	memset(&gps_awake_status, 0, sizeof(gps_awake_status));
	gps_dl_hal_get_gps_awake_status(&gps_awake_status);
	retval = wait_ret;
	if (wait_ret == 0) {
		if (gps_mcudl_xlink_is_in_state_to_listen(state)) {
			/* keeps 0 to indicate link is reset or not in proper state */
			retval = 0;
		} else {
			if (gps_awake_status.is_awake) {
				/* indicates ap just resumed and may need to be reset for LP */
				retval = 1;
			} else {
				/* indicates ap just resumed and may not need to be reset for LP */
				retval = 2;
			}
		}
	}
	tick1 = gps_dl_tick_get_us();
	MDL_LOGXW(x_id, "state=%s,%s, awake=%d,%lums, sigval=%ld, retval=%d,%d, delta_us = %lu",
		gps_dl_link_state_name(pre_state), gps_dl_link_state_name(state),
		gps_awake_status.is_awake, gps_awake_status.updated_ms,
		sigval, wait_ret, retval, (tick1 - tick0));
	return retval;
}
