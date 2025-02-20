/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_context.h"
#include "gps_mcudl_reset.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_log.h"
#include "gps_dl_name_list.h"
#include "gps_dl_hw_api.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#include "connsys_coredump.h"
#endif
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hal_conn.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
#include "gps_mcudl_hal_stat.h"
#endif

#if 1
enum GDL_RET_STATUS gps_mcudl_reset_level_set_and_trigger(
	enum gps_each_link_reset_level level, bool wait_reset_done)
{
	enum gps_mcudl_xid x_id;
	struct gps_mcudl_each_link *p = NULL;
	enum gps_each_link_state_enum old_state, new_state;
	enum gps_each_link_reset_level old_level, new_level;
	bool need_wait[GPS_MDLX_CH_NUM] = {false};
	bool send_reset[GPS_MDLX_CH_NUM] = {false};
	bool to_send_reset_event = false;
	long sigval;
	enum GDL_RET_STATUS wait_status;

	if (level != GPS_DL_RESET_LEVEL_GPS_SUBSYS && level !=  GPS_DL_RESET_LEVEL_CONNSYS) {
		GDL_LOGW("level = %d, do nothing and return", level);
		return GDL_FAIL_INVAL;
	}

	if (wait_reset_done)
		; /* TODO: take mutex to allow pending more waiter */

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		p = gps_mcudl_link_get(x_id);
		to_send_reset_event = false;

		gps_mcudl_each_link_spin_lock_take(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
		old_state = p->state_for_user;
		old_level = p->reset_level;

		switch (old_state) {
		case LINK_CLOSED:
			need_wait[x_id] = false;
			p->state_for_user = LINK_DISABLED;
			p->reset_level = level;

			/* Send reset event to ctld:
			 *
			 * for GPS_DL_RESET_LEVEL_GPS_SUBSYS ctrld do nothing but
			 *   just change state from DISABLED back to CLOSED
			 *
			 * for GPS_DL_RESET_LEVEL_CONNSYS ctrld do nothing but
			 *   just change state from DISABLED state back to CLOSED
			 */
			to_send_reset_event = true;
			break;

		case LINK_OPENING:
		case LINK_OPENED:
		case LINK_CLOSING:
		case LINK_RESET_DONE:
		case LINK_RESUMING:
		case LINK_SUSPENDING:
		case LINK_SUSPENDED:
			need_wait[x_id] = true;
			p->state_for_user = LINK_RESETTING;
			p->reset_level = level;
			to_send_reset_event = true;
			break;

		case LINK_RESETTING:
			need_wait[x_id] = true;
			if (old_level < level)
				p->reset_level = level;
			break;

		case LINK_DISABLED:
		case LINK_UNINIT:
			need_wait[x_id] = false;
			break;

		default:
			need_wait[x_id] = false;
			break;
		}

		new_state = p->state_for_user;
		new_level = p->reset_level;
		gps_mcudl_each_link_spin_lock_give(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

		send_reset[x_id] = to_send_reset_event;

		/* reduce log */
		if (!need_wait[x_id])
			continue;

		MDL_LOGXE_STA(x_id,
			"state change: %s -> %s, level = %d (%d -> %d), is_sent = %d, to_wait = %d",
			gps_dl_link_state_name(old_state), gps_dl_link_state_name(new_state),
			level, old_level, new_level,
			to_send_reset_event, need_wait[x_id]);
	}

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		to_send_reset_event = send_reset[x_id];
		if (to_send_reset_event) {
			gps_mcudl_each_link_waitable_reset(x_id, GPS_DL_WAIT_RESET);
			if (level == GPS_DL_RESET_LEVEL_CONNSYS)
				gps_mcudl_xlink_event_send(x_id, GPS_MCUDL_EVT_LINK_PRE_CONN_RESET);
			else
				gps_mcudl_xlink_event_send(x_id, GPS_MCUDL_EVT_LINK_RESET);
		}
	}

	if (!wait_reset_done) {
		GDL_LOGE("force no wait");
		return GDL_OKAY;
	}

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		if (!need_wait[x_id])
			continue;

		sigval = 0;
		p = gps_mcudl_link_get(x_id);
		wait_status = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_RESET], &sigval);
		if (wait_status == GDL_FAIL_SIGNALED) {
			GDL_LOGXE(x_id, "sigval = %ld", sigval);
			return GDL_FAIL_SIGNALED;
		}

		MDL_LOGXE(x_id, "wait ret = %s", gdl_ret_to_name(wait_status));
	}

	if (wait_reset_done)
		; /* TODO: take mutex to allow pending more waiter */

	return GDL_OKAY;
}

#define MDL_RST_REASON_MAX (32)
char g_gps_mcudl_subsys_reset_reason[MDL_RST_REASON_MAX];
int gps_mcudl_trigger_gps_subsys_reset(bool wait_reset_done, const char *p_reason)
{
	enum GDL_RET_STATUS ret_status;
	int i;

	if (p_reason != NULL) {
		for (i = 0; i < MDL_RST_REASON_MAX - 1; i++) {
			if (p_reason[i] == '\0')
				break;
			g_gps_mcudl_subsys_reset_reason[i] = p_reason[i];
		}
		g_gps_mcudl_subsys_reset_reason[i] = '\0';
		MDL_LOGE("reason=%s", &g_gps_mcudl_subsys_reset_reason[0]);
	}
	ret_status = gps_mcudl_reset_level_set_and_trigger(
		GPS_DL_RESET_LEVEL_GPS_SUBSYS, wait_reset_done);
	if (ret_status != GDL_OKAY) {
		MDL_LOGE("status %s is not okay, return -1", gdl_ret_to_name(ret_status));
		return -1;
	}
	return 0;
}

void gps_mcudl_handle_connsys_reset_done(void)
{
	enum gps_mcudl_xid x_id;
	struct gps_mcudl_each_link *p = NULL;
	enum gps_each_link_state_enum state;
	enum gps_each_link_reset_level level;
	bool to_send_reset_event = false;

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		p = gps_mcudl_link_get(x_id);
		to_send_reset_event = false;

		gps_mcudl_each_link_spin_lock_take(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
		state = p->state_for_user;
		level = p->reset_level;

		if (level == GPS_DL_RESET_LEVEL_CONNSYS) {
			if (state == LINK_DISABLED || state == LINK_RESETTING)
				to_send_reset_event = true;
		}
		gps_mcudl_each_link_spin_lock_give(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

		if (to_send_reset_event)
			gps_mcudl_xlink_event_send(x_id, GPS_MCUDL_EVT_LINK_POST_CONN_RESET);

		MDL_LOGXE_STA(x_id, "state check: %s, level = %d, is_sent = %d",
			gps_dl_link_state_name(state), level, to_send_reset_event);
	}
}
#endif



#if 1 /* coredump*/
int gps_mcudl_coredump_conninfra_is_readable_by_mask(unsigned int mask)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	int readable1, readable2;
	int hung_value;

	readable1 = conninfra_reg_readable();
	hung_value = conninfra_is_bus_hang();

	/* Try our best to do DLM/ILM/SRAM coredump by return readable2=1 even readable1 == 0.
	 * It returns readable2=0 only if SLP_PROT_ERR.
	 * Similar api is gps_dl_conninfra_is_readable_by_hung_value
	 */
	if ((hung_value < 0) || (hung_value & mask))
		readable2 = 0;
	else
		readable2 = 1;

	MDL_LOGW("readable1=%d, hung_value=0x%x(0x%x), readable2=%d",
		readable1, hung_value, mask, readable2);
	return readable2;
#else
	return 1;
#endif
}

int gps_mcudl_coredump_conninfra_on_is_readable(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	return gps_mcudl_coredump_conninfra_is_readable_by_mask(
		CONNINFRA_AP2CONN_RX_SLP_PROT_ERR |
		CONNINFRA_AP2CONN_TX_SLP_PROT_ERR);
#else
	return true;
#endif
}

int gps_mcudl_coredump_conninfra_off_is_readable(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	return gps_mcudl_coredump_conninfra_is_readable_by_mask(
		CONNINFRA_AP2CONN_RX_SLP_PROT_ERR |
		CONNINFRA_AP2CONN_TX_SLP_PROT_ERR |
		CONNINFRA_AP2CONN_CLK_ERR);
#else
	return true;
#endif
}

int gps_mcudl_coredump_is_readable(void)
{
	int conn_readable, readable;
	bool bg_readable = false;

	conn_readable = gps_mcudl_coredump_conninfra_off_is_readable();
	if (conn_readable) {
		bg_readable = gps_mcudl_hal_bg_is_readable(false);
		readable = bg_readable ? 1 : 0;
	} else
		readable = 0;

	MDL_LOGW("readable=%d, conn=%d, bg=%d",
		readable, conn_readable, bg_readable);
	return readable;
}

#if GPS_DL_HAS_CONNINFRA_DRV
struct coredump_event_cb g_gps_coredump_cb = {
	.reg_readable = gps_mcudl_coredump_is_readable,
	.poll_cpupcr = NULL,
};

void *g_gps_coredump_handler;
#endif

void gps_mcudl_connsys_coredump_init(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	g_gps_coredump_handler = connsys_coredump_init(CONNDRV_TYPE_GPS, &g_gps_coredump_cb);
	if (g_gps_coredump_handler == NULL)
		GDL_LOGW("gps_mcudl_connsys_coredump_init fail");
#endif
}

void gps_mcudl_connsys_coredump_deinit(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_coredump_deinit(g_gps_coredump_handler);
#endif
}

void gps_mcudl_connsys_coredump_start(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	GDL_LOGI("gps_mcudl_connsys_coredump_start, reason=%s",
		&g_gps_mcudl_subsys_reset_reason[0]);
	connsys_coredump_start(g_gps_coredump_handler, 0, CONNDRV_TYPE_GPS,
		&g_gps_mcudl_subsys_reset_reason[0]);
	connsys_coredump_clean(g_gps_coredump_handler);
#endif
}

void gps_mcudl_connsys_coredump_start_wrapper(void)
{
	bool show_log = false;

	show_log = gps_dl_set_show_reg_rw_log(false);
#if GPS_DL_HAS_PLAT_DRV
	/*dump tia status*/
	gps_dl_tia_gps_ctrl(false);
#endif
	/* dump ydata status */
	gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_HOST_WR);
	gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_MCU_ACK);
	gps_mcu_hif_host_trans_hist_dump();
	gps_mcudl_mcu2ap_rec_dump();
	gps_mcudl_xlink_dump_all_rec();
	gps_mcudl_mcu2ap_ydata_sta_may_do_dump(GPS_MDLY_NORMAL, true);
	gps_mcudl_flowctrl_dump_host_sta(GPS_MDLY_NORMAL);
	gps_mcudl_host_sta_hist_dump(GPS_MDLY_NORMAL);
	gps_mcudl_host_sta_hist_dump(GPS_MDLY_URGENT);
	gps_mcudl_mcu2ap_ydata_sta_may_do_dump(GPS_MDLY_URGENT, true);
	gps_mcudl_flowctrl_dump_host_sta(GPS_MDLY_URGENT);
	gps_mcudl_hal_user_fw_own_status_dump();
	(void)gps_mcudl_hal_dump_power_state();
	gps_mcudl_mcu2ap_put_to_xlink_fail_rec_dump();

	if (gps_mcudl_coredump_conninfra_on_is_readable()) {
		gps_mcudl_hal_mcu_show_status();
		gps_mcudl_hal_ccif_show_status();
		gps_dl_hw_dump_host_csr_gps_info(false);
		if (gps_mcudl_hal_bg_is_readable(true))
			gps_mcudl_hal_vdnr_dump();
		gps_dl_hw_dump_host_csr_gps_info(false);
	} else
		MDL_LOGE("readable=0");

	gps_mcudl_connsys_coredump_start();
	gps_dl_set_show_reg_rw_log(show_log);
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
	gps_mcudl_stat_set_mcu_exception();
#endif
}
#endif

