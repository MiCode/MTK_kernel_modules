/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_name_list.h"
#include "gps_dl_time_tick.h"
#include "gps_dl_hal.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hal_util.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_dl_lib_misc.h"
#include "gps_dl_osal.h"
#include "gps_dl_context.h"
#include "gps_dl_subsys_reset.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcusys_fsm.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcu_hif_mgmt_cmd_send.h"
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_data_pkt_payload_struct.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"


bool g_gps_fw_log_is_on;

void gps_mcudl_xlink_event_send(enum gps_mcudl_xid link_id,
	enum gps_mcudl_xlink_event_id evt)
{
#if GPS_DL_HAS_CTRLD
	struct gps_dl_osal_lxop *pOp = NULL;
	struct gps_dl_osal_signal *pSignal = NULL;
	int iRet;

	pOp = gps_dl_get_free_op();
	if (!pOp) {
		MDL_LOGXE(link_id, "gps_dl_get_free_op failed!");
		return;
	}

	pSignal = &pOp->signal;
	pSignal->timeoutValue = 0;/* send data need to wait ?ms */
	if (link_id < GPS_MDLX_CH_NUM) {
		pOp->op.opId = GPS_DL_OPID_MCUDL_XLINK_EVENT_PROC;
		pOp->op.au4OpData[0] = link_id;
		pOp->op.au4OpData[1] = evt;
		pOp->op.op_enq = gps_dl_tick_get_ms();
		iRet = gps_dl_put_act_op(pOp);
		MDL_LOGXD(link_id, "iRet=%d", iRet);
	} else {
		gps_dl_put_op_to_free_queue(pOp);
		MDL_LOGXE(link_id, "invalid x_id=%d!", link_id);
	}
#else
	gps_mcudl_xlink_event_proc(link_id, evt);
#endif
}

void gps_mcudl_xlink_event_proc(enum gps_mcudl_xid link_id,
	enum gps_mcudl_xlink_event_id evt)
{
	struct gps_mcudl_each_link *p_link = gps_mcudl_link_get(link_id);
	bool show_log = false;
	bool is_okay = false;
	unsigned long tick_us0, tick_us1;
	int ret;
	enum gps_each_link_state_enum old_state, new_state;
	bool meet_state;
	int cnt;

	tick_us0 = gps_dl_tick_get_us();
	MDL_LOGXD_EVT(link_id, "evt=%s", gps_mcudl_xlink_event_name(evt));

	switch (evt) {
	case GPS_MCUDL_EVT_LINK_OPEN:
		/* show_log = gps_dl_set_show_reg_rw_log(true); */
		/* set flag to atf */
		if (gps_dl_hal_get_open_flag() != 0) {
			GDL_LOGXE(link_id, "offload/ap mismatch mode");
			gps_mcudl_link_open_ack(link_id, false);
			break;
		}
		gps_mcudl_hal_may_set_link_power_flag(link_id, true);
		gps_mcudl_each_link_inc_session_id(link_id);
		gps_mcudl_each_link_set_active(link_id, true);

		if (!gps_mcudl_xlink_is_connected_to_mcu_lifecycle(link_id)) {
			/* connot write for this type of links */
			gps_mcudl_each_link_set_ready_to_write(link_id, false);
			gps_mcudl_link_open_ack(link_id, true);
			break;
		}

		ret = gps_mcudl_hal_conn_power_ctrl(link_id, 1);
		if (ret != 0) {
			gps_mcudl_link_open_ack(link_id, false);
			break;
		}

		ret = gps_mcudl_hal_link_power_ctrl(link_id, 1);
		if (ret != 0) {
			gps_mcudl_hal_link_power_ctrl(link_id, 0);
			gps_mcudl_link_open_ack(link_id, false);
			break;
		}
#if 0
		gps_dl_link_irq_set(link_id, true);
#if GPS_DL_NO_USE_IRQ
		gps_dl_wait_us(1000); /* wait 1ms */
#endif
#endif
		/* TODO: move to mcu state change to later like gps_dsp_state_change_to */
		gps_mcudl_each_link_set_ready_to_write(link_id, true);

		if (!gps_mcudl_xlink_is_connected_mnlbin(link_id)) {
			gps_mcudl_link_open_ack(link_id, true);
			break;
		}

		if (link_id == GPS_MDLX_LPPM) {
			bool mnlbin_ready = gps_mcusys_mnlbin_state_is(GPS_MCUSYS_MNLBIN_ST_CTLR_CREATED);
			bool scif_ready = gps_mcusys_scif_is_ready();

			if (mnlbin_ready && scif_ready) {
				gps_mcudl_link_open_ack(link_id, true);
				gps_mcusys_scif_set_lppm_open_ack_done(true);
				break;
			}

			if (!mnlbin_ready)
				gps_mcudl_each_link_set_bool_flag(link_id, LINK_MISS_MNLBIN_ACK, true);

			gps_mcusys_scif_set_lppm_open_ack_done(false);
			MDL_LOGXI(link_id,
				"bypass gps_mcudl_link_open_ack now, mnlbin=%d, scif=%d",
				mnlbin_ready, scif_ready);
			break;
		}

		if (gps_mcusys_mnlbin_state_is(GPS_MCUSYS_MNLBIN_ST_CTLR_CREATED)) {
			/* GPS_MDLX_LPPM has already been handled in previous if-break */
			gps_mcudl_link_open_ack(link_id, true);
		} else {
			gps_mcudl_each_link_set_bool_flag(link_id, LINK_MISS_MNLBIN_ACK, true);
			/* wait gps_mcusys_mnlbin_fsm to ack it */
			MDL_LOGXI(link_id, "bypass gps_mcudl_link_open_ack now");
		}

		/* gps_dl_set_show_reg_rw_log(show_log); */
		break;

	case GPS_MCUDL_EVT_LINK_RESET2:
		if (!gps_mcudl_conninfra_is_okay_or_handle_it())
			break;

		/* no more dump here, it should be dumped by
		 * GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS or
		 * GPS_MCUDL_EVT_LINK_PRINT_DATA_STATUS
		 */
		is_okay = gps_mcudl_xlink_test_toggle_reset_by_gps_hif(0);
		MDL_LOGXE(link_id, "toggle_reset_by_gps_hif, ok=%d", is_okay);
		if (!is_okay) {
			/* gps_dl_trigger_connsys_reset(); */
			/* use subsys reset here */
			gps_mcudl_trigger_gps_subsys_reset(false, "GNSS hif trigger fail-1");
			break;
		}

		/* wait MCU reset if reset cmd sent okay */
		old_state = gps_mcudl_each_link_get_state(link_id);
		cnt = 0;
		meet_state = false;
		while (1) {
			new_state = gps_mcudl_each_link_get_state(link_id);
			MDL_LOGXE(link_id, "cnt=%d, state=%s,%s", cnt,
				gps_dl_link_state_name(old_state),
				gps_dl_link_state_name(new_state));
			if (new_state == LINK_RESETTING || new_state == LINK_RESET_DONE ||
				new_state == LINK_CLOSING || new_state == LINK_DISABLED) {
				meet_state = true;
				break;
			}

			/* ~15ms */
			if (cnt >= 6)
				break;

			if (!gps_mcudl_conninfra_is_okay_or_handle_it())
				break;

			gps_mcudl_hal_mcu_show_status();
			gps_mcudl_hal_ccif_show_status();
			gps_dl_sleep_us(2200, 3200);
			cnt++;
		}

		/* state have changed to reset-like */
		if (meet_state)
			break;

		/* wait MCU reset timeout */
		gps_mcudl_trigger_gps_subsys_reset(false, "GNSS hif trigger fail-2");
		break;

	case GPS_MCUDL_EVT_LINK_CLOSE:
	case GPS_MCUDL_EVT_LINK_RESET:
	case GPS_MCUDL_EVT_LINK_PRE_CONN_RESET:
		if (gps_mcudl_each_link_get_bool_flag(link_id, LINK_MISS_MNLBIN_ACK))
			gps_mcudl_each_link_set_bool_flag(link_id, LINK_MISS_MNLBIN_ACK, false);

		gps_mcudl_link_trigger_state_ntf(link_id);
		if (evt != GPS_MCUDL_EVT_LINK_CLOSE)
			show_log = gps_dl_set_show_reg_rw_log(true);

		if (!gps_mcudl_xlink_is_connected_to_mcu_lifecycle(link_id)) {
			/* bypass all mcu related operations for this type of links */
			goto _close_non_mcu_link;
		}

		/* handle open fail case */
		if (!gps_mcudl_each_link_get_bool_flag(link_id, LINK_OPEN_RESULT_OKAY)) {
			MDL_LOGXW(link_id, "not open okay, just power off for %s",
				gps_mcudl_xlink_event_name(evt));

			gps_mcudl_each_link_set_active(link_id, false);
			gps_mcudl_hal_link_power_ctrl(link_id, 0);
			gps_mcudl_hal_conn_power_ctrl(link_id, 0);

			/* set flag to atf */
			gps_mcudl_hal_may_set_link_power_flag(link_id, false);
			goto _close_or_reset_ack;
		}

		/* if gpsbin status == post off, mcu is not opened.
		 * shouldn't gothrough following steps.
		 * From mmap check, ap/offload mode will be tested .
		 * enable slp prot fail in ap mode would call connsys reset and trigger
		 * offload mode dump either.
		 * we don't expect it to happen, so bypass.
		 */
		if (gps_mcusys_gpsbin_state_is(GPS_MCUSYS_GPSBIN_POST_OFF)) {
			MDL_LOGXW(link_id, "gps_bin not open, just bypass for %s",
				gps_mcudl_xlink_event_name(evt));
			goto _close_or_reset_ack;
		}

#if 0
		/* to avoid twice enter */
		if (GPS_DSP_ST_OFF == gps_dsp_state_get(link_id)) {
			MDL_LOGXD(link_id, "dsp state is off, do nothing for %s",
				gps_mcudl_xlink_event_name(evt));

			if (evt != GPS_DL_EVT_LINK_CLOSE)
				gps_dl_set_show_reg_rw_log(show_log);

			goto _close_or_reset_ack;
		} else if (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(link_id)) {
			/* exit deep stop mode and turn off it
			 * before exit deep stop, need clear pwr stat to make sure dsp is in hold-on state
			 * after exit deep stop mode.
			 */
			gps_dl_link_pre_leave_dpstop_setting(link_id);
			gps_dl_hal_link_clear_hw_pwr_stat(link_id);
			gps_dl_hal_link_power_ctrl(link_id, GPS_DL_HAL_LEAVE_DPSTOP);
		} else {
			/* make sure current link's DMAs are stopped and mask the IRQs */
			gps_dl_link_pre_off_setting(link_id);
		}
#else
		gps_mcudl_each_link_set_ready_to_write(link_id, false);
		/*gps_dl_link_irq_set(link_id, false);*/
		gps_mcudl_each_link_set_active(link_id, false);
#endif
		if (evt != GPS_MCUDL_EVT_LINK_CLOSE && !g_gps_mcudl_ever_do_coredump) {
			gps_mcudl_connsys_coredump_start_wrapper();
			g_gps_mcudl_ever_do_coredump = true;
		}

		gps_mcudl_hal_link_power_ctrl(link_id, 0);
		gps_mcudl_hal_conn_power_ctrl(link_id, 0);
_close_non_mcu_link:
		gps_mcudl_each_link_context_clear(link_id);
#if GPS_DL_ON_LINUX
		gps_dma_buf_reset(&p_link->tx_dma_buf);
		gps_dma_buf_reset(&p_link->rx_dma_buf);
		p_link->epoll_flag = false;
#endif

		/* set flag to atf */
		gps_mcudl_hal_may_set_link_power_flag(link_id, false);

_close_or_reset_ack:
		if (evt != GPS_MCUDL_EVT_LINK_CLOSE)
			gps_dl_set_show_reg_rw_log(show_log);

		if (GPS_MCUDL_EVT_LINK_CLOSE == evt)
			gps_mcudl_link_close_ack(link_id); /* TODO: check fired race */
		else
			gps_mcudl_link_reset_ack(link_id);
		break;

	case GPS_MCUDL_EVT_LINK_POST_CONN_RESET:
		gps_mcudl_link_on_post_conn_reset(link_id);
		gps_mcudl_link_trigger_state_ntf(link_id);
		break;

	case GPS_MCUDL_EVT_LINK_WRITE:
		/* gps_dl_hw_print_usrt_status(link_id); */
		if (gps_mcudl_each_link_is_ready_to_write(link_id)) {
#if 0
			gps_dl_link_start_tx_dma_if_has_data(link_id);
#else
			/*
			 * to_flush: w/ gps_mcudl_ap2mcu_xdata_send_v2
			 */
			gps_mcudl_ap2mcu_data_slot_flush_on_xwrite(link_id);

			/* TODO: wake up the threads waiting on xlink */
#endif
		} else
			MDL_LOGXW(link_id, "too early writing");
		break;

	case GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS:
	case GPS_MCUDL_EVT_LINK_PRINT_DATA_STATUS:
		if (!gps_mcudl_each_link_is_active(link_id)) {
			MDL_LOGXW(link_id, "inactive, do not dump hw status");
			break;
		}

		/*dump tia status*/
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_tia_gps_ctrl(false);
#endif
		/* dump ydata status */
		gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_HOST_WR);
		gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_MCU_ACK);
		gps_mcu_hif_host_trans_hist_dump();
		gps_mcudl_mcu2ap_rec_dump();
		gps_mcudl_xlink_dump_all_rec();
		gps_mcudl_host_sta_hist_dump(GPS_MDLY_NORMAL);
		gps_mcudl_mcu2ap_ydata_sta_may_do_dump(GPS_MDLY_NORMAL, true);
		gps_mcudl_flowctrl_dump_host_sta(GPS_MDLY_NORMAL);
		gps_mcudl_host_sta_hist_dump(GPS_MDLY_URGENT);
		gps_mcudl_mcu2ap_ydata_sta_may_do_dump(GPS_MDLY_URGENT, true);
		gps_mcudl_flowctrl_dump_host_sta(GPS_MDLY_URGENT);
		gps_mcudl_mcu2ap_put_to_xlink_fail_rec_dump();

		if (!gps_mcudl_conninfra_is_okay_or_handle_it())
			break;

		gps_mcudl_hal_mcu_show_pc_log();
		gps_mcudl_hal_mcu_show_status();
		gps_mcudl_hal_ccif_show_status();
		gps_dl_hw_dump_host_csr_gps_info(false);
		if (gps_mcudl_hal_bg_is_readable(true))
			gps_mcudl_hal_vdnr_dump();
		gps_dl_hw_dump_host_csr_gps_info(false);
		break;

	case GPS_MCUDL_EVT_LINK_FW_LOG_ON:
		if (g_gps_fw_log_is_on) {
			MDL_LOGW("fw log ready on");
			break;
		}

		g_gps_fw_log_is_on = true;
		if (!gps_mcusys_gpsbin_state_is(GPS_MCUSYS_GPSBIN_POST_ON)) {
			MDL_LOGW("fw log just keep on flag");
			break;
		}

		gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(true);
		break;

	case GPS_MCUDL_EVT_LINK_FW_LOG_OFF:
		if (!g_gps_fw_log_is_on) {
			MDL_LOGW("fw log ready off");
			break;
		}

		g_gps_fw_log_is_on = false;
		if (!gps_mcusys_gpsbin_state_is(GPS_MCUSYS_GPSBIN_POST_ON)) {
			MDL_LOGW("fw log just keep off flag");
			break;
		}

		gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(false);
		break;

	default:
		break;
	}

	tick_us1 =  gps_dl_tick_get_us();
	MDL_LOGXI_EVT(link_id, "evt=%s, dt_us=%lu",
		gps_mcudl_xlink_event_name(evt), tick_us1 - tick_us0);
}

#if 0
void gps_dl_link_pre_off_setting(enum gps_mcudl_xid link_id)
{
	/*
	 * The order is important:
	 * 1. disallow write, avoiding to start dma
	 * 2. stop tx/rx dma and mask dma irq if it is last link
	 * 3. mask link's irqs
	 * 4. set inactive after all irq mask done
	 * (at this time isr can check inactive and unmask irq safely due to step 3 already mask irqs)
	 */
	gps_dl_link_set_ready_to_write(link_id, false);
	gps_dl_hal_link_confirm_dma_stop(link_id);
	gps_dl_link_irq_set(link_id, false);
	gps_mcudl_each_link_set_active(link_id, false);
}

bool g_gps_dl_dpstop_release_wakelock_fg;
void gps_dl_link_post_enter_dpstop_setting(enum gps_mcudl_xid link_id)
{
	bool cond = true;

#if 0
	cond = (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(GPS_DATA_LINK_ID0)
			&& (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(GPS_DATA_LINK_ID1)
			|| GPS_DSP_ST_OFF == gps_dsp_state_get(GPS_DATA_LINK_ID1)))
#endif
	if (cond) {
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_wake_lock_hold(false);
#endif
		g_gps_dl_dpstop_release_wakelock_fg = true;
		MDL_LOGXW(link_id, "enter dpstop with wake_lock relased");
	}
}

void gps_dl_link_pre_leave_dpstop_setting(enum gps_mcudl_xid link_id)
{
	if (g_gps_dl_dpstop_release_wakelock_fg) {
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_wake_lock_hold(true);
#endif
		g_gps_dl_dpstop_release_wakelock_fg = false;
		MDL_LOGXW(link_id, "exit dpstop with wake_lock holded");
	}
}
#endif

