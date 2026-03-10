/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_ylink.h"
#include "gps_dl_context.h"
#include "gps_dl_osal.h"
#include "gps_dl_name_list.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_plat_api.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcusys_fsm.h"
#include "gps_dl_time_tick.h"
#include "gps_dl_subsys_reset.h"


void gps_mcudl_ylink_event_send(enum gps_mcudl_yid y_id, enum gps_mcudl_ylink_event_id evt)
{
#if GPS_DL_HAS_CTRLD
	struct gps_dl_osal_lxop *pOp = NULL;
	struct gps_dl_osal_signal *pSignal = NULL;
	int iRet;

	pOp = gps_dl_get_free_op();
	if (!pOp)
		return;

	pSignal = &pOp->signal;
	pSignal->timeoutValue = 0;
	if (y_id < GPS_MDLY_CH_NUM) {
		pOp->op.opId = GPS_DL_OPID_MCUDL_YLINK_EVENT_PROC;
		pOp->op.au4OpData[0] = y_id;
		pOp->op.au4OpData[1] = evt;
		pOp->op.op_enq = gps_dl_tick_get_ms();
		iRet = gps_dl_put_act_op(pOp);
	} else {
		gps_dl_put_op_to_free_queue(pOp);
		/*printf error msg*/
		return;
	}
#else
	gps_mcudl_ylink_event_proc(y_id, evt);
#endif
}

void gps_mcudl_ylink_event_proc(enum gps_mcudl_yid y_id, enum gps_mcudl_ylink_event_id evt)
{
	unsigned long tick_us0, tick_us1, dt_us;
	bool is_okay = false;

	tick_us0 = gps_dl_tick_get_us();
	MDL_LOGYD_EVT(y_id, "evt=%d", evt);
	switch (evt) {
	case GPS_MCUDL_YLINK_EVT_ID_RX_DATA_READY:
#if 1
		gps_mcudl_mcu2ap_ydata_proc(y_id);
#else
		if (y_id == GPS_MDLY_NORMAL)
			gps_mcudl_plat_mcu_ch1_read_proc();
#endif
		break;

	case GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA:
		gps_mcudl_ap2mcu_data_slot_flush_on_recv_sta(y_id);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_START:
		gps_mcusys_mnlbin_fsm(GPS_MCUSYS_MNLBIN_SYS_RESET_START);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_END:
		gps_mcusys_mnlbin_fsm(GPS_MCUSYS_MNLBIN_SYS_RESET_END);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_ABNORMAL: {
		bool conninfra_okay, ccif_irq_en;

		conninfra_okay = gps_mcudl_conninfra_is_okay_or_handle_it();
		ccif_irq_en = gps_mcudl_hal_get_ccif_irq_en_flag();

		MDL_LOGE("conninfra_okay = %d, ccif_irq_en = %d", conninfra_okay, ccif_irq_en);
		if (conninfra_okay && !ccif_irq_en) {
			gps_mcudl_hal_set_ccif_irq_en_flag(true);
			gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_HAL);
		}
		break;
	}
	case GPS_MCUDL_YLINK_EVT_ID_MCU_SET_FW_OWN:
		gps_mcudl_hal_user_set_fw_own_if_no_recent_clr();
		break;
	case GPS_MCUDL_YLINK_EVT_ID_CCIF_CLR_FW_OWN:
		is_okay = gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_CCIF);
		if (!is_okay) {
			MDL_LOGE("ccif msg clr_fw_own fail");
			break;
		}
		gps_mcudl_hal_set_ccif_irq_en_flag(true);
		gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_HAL);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_TEST_CLR_FW_OWN:
		(void)gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_TEST);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_MCU_WDT_DUMP:
		if (gps_mcusys_gpsbin_state_is(GPS_MCUSYS_GPSBIN_POST_ON)) {
			MDL_LOGW("do gps_mcudl_hal_wdt_dump");
			gps_mcudl_hal_wdt_dump();
		} else
			MDL_LOGW("bypass gps_mcudl_hal_wdt_dump");
		break;
	default:
		break;
	}
	tick_us1 = gps_dl_tick_get_us();
	dt_us = tick_us1 - tick_us0;
	MDL_LOGYI_EVT(y_id, "evt=%d, dt_us=%lu", evt, dt_us);
}

