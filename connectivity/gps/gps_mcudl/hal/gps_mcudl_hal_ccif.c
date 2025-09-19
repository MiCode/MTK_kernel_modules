/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hw_ccif.h"
#include "gps_mcudl_hw_mcu.h"
#include "gps_dl_isr.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_time_tick.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcudl_data_pkt_slot.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#include "connsyslog.h"
#include "gps_dl_linux_plat_drv.h"
#endif

bool gps_mcudl_hal_ccif_tx_is_busy(enum gps_mcudl_ccif_ch ch)
{
	return gps_mcudl_hw_ccif_is_tch_busy(ch);
}

void gps_mcudl_hal_ccif_tx_prepare(enum gps_mcudl_ccif_ch ch)
{
	gps_mcudl_hw_ccif_set_tch_busy(ch);
}

void gps_mcudl_hal_ccif_tx_trigger(enum gps_mcudl_ccif_ch ch)
{
	gps_mcudl_hw_ccif_set_tch_start(ch);
}

bool gps_mcudl_hal_ccif_tx_is_ack_done(enum gps_mcudl_ccif_ch ch)
{
	return !!(gps_mcudl_hw_ccif_get_tch_start_bitmask() & (1UL << ch));
}

void gps_mcudl_hal_ccif_show_status(void)
{
	unsigned int busy_mask, start_mask, rch_mask;
	enum gpsmdl_ccif_misc_cr_group id;
	unsigned int mask, dummy, shadow;

	busy_mask = gps_mcudl_hw_ccif_get_tch_busy_bitmask();
	start_mask = gps_mcudl_hw_ccif_get_tch_start_bitmask();
	rch_mask = gps_mcudl_hw_ccif_get_rch_bitmask();
	MDL_LOGW("busy=0x%x, start=0x%x, rch=0x%x",
		busy_mask, start_mask, rch_mask);

	for (id = 0; id < GMDL_CCIF_MISC_NUM; id++) {
		mask = gps_mcudl_hw_ccif_get_irq_mask(id);
		dummy = gps_mcudl_hw_ccif_get_dummy(id);
		shadow = gps_mcudl_hw_ccif_get_shadow(id);
		MDL_LOGW("msic_id=%d, mask=0x%x, dummy=0x%x, shadow=0x%x",
			id, mask, dummy, shadow);
	}
}

unsigned long g_gps_ccif_irq_cnt;
unsigned int g_gps_fw_log_irq_cnt;
void gps_mcudl_hal_ccif_rx_isr(void)
{
	unsigned int rch_mask, last_rch_mask = 0;
	enum gps_mcudl_ccif_ch ch;
	unsigned long tick_us0, tick_us1, dt_us, curr_tick;
	static unsigned long last_tick;
	unsigned int recheck_cnt = 0;
	bool already_wakeup = false;

	gps_dl_irq_mask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_ISR);
	g_gps_ccif_irq_cnt++;
	tick_us0 = gps_dl_tick_get_us();

	already_wakeup = gps_mcudl_hal_user_add_if_fw_own_is_clear(GMDL_FW_OWN_CTRL_BY_CCIF);
	if (!already_wakeup) {
		GDL_LOGD("ntf to clr_fw_own, ccif_irq_cnt=%lu", g_gps_ccif_irq_cnt);
		gps_mcudl_hal_set_ccif_irq_en_flag(false);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_CLR_FW_OWN);
		return;
	}

recheck_rch:
	if (recheck_cnt == 0) {
		if (!gps_dl_conninfra_is_readable()) {
			GDL_LOGE("readable check fail, ccif_irq_cnt=%lu", g_gps_ccif_irq_cnt);
			gps_mcudl_hal_set_ccif_irq_en_flag(false);
			gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_ABNORMAL);
			return;
		}
	}
	rch_mask = gps_mcudl_hw_ccif_get_rch_bitmask();

	/* handling read 0xdeadfeed case */
	if ((rch_mask & 0xFFFFFF00) != 0) {
		GDL_LOGW("cnt=%lu, rch_mask=0x%x, abnormal", g_gps_ccif_irq_cnt, rch_mask);
		gps_mcudl_hal_set_ccif_irq_en_flag(false);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_ABNORMAL);
		return;
	}

	if (rch_mask == 0) {
		gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_ISR);
		gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_CCIF);
		return;
	}

	for (ch = 0; ch < GPS_MCUDL_CCIF_CH_NUM; ch++) {
		if (rch_mask & (1UL << ch)) {
			gps_mcudl_hw_ccif_set_rch_ack(ch);
			if (ch == GPS_MCUDL_CCIF_CH4)
				gps_mcu_hif_host_ccif_irq_handler_in_isr();
#if GPS_DL_HAS_CONNINFRA_DRV
			else if (ch == GPS_MCUDL_CCIF_CH1) {
				connsys_log_irq_handler(CONN_DEBUG_TYPE_GPS);
				curr_tick = gps_dl_tick_get_us();
				g_gps_fw_log_irq_cnt++;
				if ((curr_tick - last_tick) >= 1000000) {
					MDL_LOGW("gps_fw_log_irq_cnt=%d", g_gps_fw_log_irq_cnt);
					last_tick = curr_tick;
				}
			} else if (ch == GPS_MCUDL_CCIF_CH2) {
#if 0
				/* SUBSYS reset
				 * Currently, treat it as whole chip reset
				 */
				conninfra_trigger_whole_chip_rst(
					CONNDRV_TYPE_GPS, "GNSS FW trigger subsys reset");
#else
				/* mark irq is not enabled and return for this case */
				gps_mcudl_hal_set_ccif_irq_en_flag(false);

				/* SUBSYS reset */
				gps_mcudl_trigger_gps_subsys_reset(false, "GNSS FW trigger subsys reset");
				return;
#endif
			} else if (ch == GPS_MCUDL_CCIF_CH3) {
				/* mark irq is not enabled and return for this case */
				gps_mcudl_hal_set_ccif_irq_en_flag(false);

				/* WHOLE_CHIP reset */
				conninfra_trigger_whole_chip_rst(
					CONNDRV_TYPE_GPS, "GNSS FW trigger whole chip reset");
				return;
			}
#endif
		}
	}

	tick_us1 = gps_dl_tick_get_us();
	dt_us = tick_us1 - tick_us0;
	if ((dt_us >= 10 * 1000) || (rch_mask & (1UL << GPS_MCUDL_CCIF_CH2)) ||
		(rch_mask & (1UL << GPS_MCUDL_CCIF_CH3))) {
		GDL_LOGW("cnt=%lu, rch_mask=0x%x, dt_us=%lu, recheck=%u, last_mask=0x%x",
			g_gps_ccif_irq_cnt, rch_mask, dt_us, recheck_cnt, last_rch_mask);
	} else {
		GDL_LOGD("cnt=%lu, rch_mask=0x%x, dt_us=%lu, recheck=%u, last_mask=0x%x",
			g_gps_ccif_irq_cnt, rch_mask, dt_us, recheck_cnt, last_rch_mask);
	}
	recheck_cnt++;
	last_rch_mask = rch_mask;
	goto recheck_rch;
}

bool g_gps_ccif_irq_en;

bool gps_mcudl_hal_get_ccif_irq_en_flag(void)
{
	bool enable;

	gps_mcudl_slot_protect();
	enable = g_gps_ccif_irq_en;
	gps_mcudl_slot_unprotect();
	return enable;
}

void gps_mcudl_hal_set_ccif_irq_en_flag(bool enable)
{
	gps_mcudl_slot_protect();
	g_gps_ccif_irq_en = enable;
	gps_mcudl_slot_unprotect();
}

unsigned long g_gps_wdt_irq_cnt;
unsigned long g_gps_wdt_irq_curr_session_cnt;
bool wdt_irq_trigger_reset_done;
void gps_mcudl_hal_wdt_init(void)
{
	g_gps_wdt_irq_curr_session_cnt = 0;
	if (wdt_irq_trigger_reset_done) {
		gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_WDT), GPS_DL_IRQ_CTRL_FROM_THREAD);
		MDL_LOGW("check wdt_irq_trigger_reset_done = %d, unmask for already wdt reset",
			wdt_irq_trigger_reset_done);
	}
	wdt_irq_trigger_reset_done = false;
}

void gps_mcudl_hal_wdt_isr(void)
{
	int readable = 0;
	int hung_value = 0;
	bool is_fw_own;

	g_gps_wdt_irq_cnt++;
	g_gps_wdt_irq_curr_session_cnt++;
	MDL_LOGW("cnt=%ld/%ld", g_gps_wdt_irq_cnt, g_gps_wdt_irq_curr_session_cnt);

	if (g_gps_wdt_irq_curr_session_cnt > 3) {
		if (!wdt_irq_trigger_reset_done) {
			gps_dl_irq_mask(gps_dl_irq_index_to_id(GPS_DL_IRQ_WDT), GPS_DL_IRQ_CTRL_FROM_ISR);
			wdt_irq_trigger_reset_done = true;
			gps_mcudl_trigger_gps_subsys_reset(false, "WDT trigger subsys reset");
		}
		return;
	}
	is_fw_own = gps_mcudl_hal_is_fw_own();
	readable = conninfra_reg_readable();
	hung_value = conninfra_is_bus_hang();
	MDL_LOGW("check: readable=%d, hung_value=%d, fw_own=%d", readable, hung_value, is_fw_own);
	gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_MCU_WDT_DUMP);
}

void gps_mcudl_hal_wdt_dump(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	int readable;
	int hung_value = 0;
	bool is_fw_own;

	is_fw_own = gps_mcudl_hal_force_conn_wake_if_fw_own_is_clear();
	readable = conninfra_reg_readable();
	hung_value = conninfra_is_bus_hang();
	MDL_LOGW("check1: readable=%d, hung_value=%d, fw_own=%d", readable, hung_value, is_fw_own);
	gps_mcudl_hal_user_fw_own_status_dump();

#if GPS_DL_HAS_PLAT_DRV
	gps_dl_tia_gps_ctrl(false);
#endif
	if (gps_dl_conninfra_is_readable_by_hung_value(hung_value)) {
		gps_mcudl_hal_mcu_show_pc_log();
		gps_mcudl_hal_mcu_show_status();
		gps_mcudl_hal_ccif_show_status();
		gps_dl_hw_dump_host_csr_gps_info(false);
	}
	/* gps_dl_sleep_us(2200, 3200); */

	readable = conninfra_reg_readable();
	hung_value = conninfra_is_bus_hang();
	MDL_LOGW("check2: readable=%d, hung_value=%d, fw_own=%d", readable, hung_value, is_fw_own);

#if GPS_DL_HAS_PLAT_DRV
	gps_dl_tia_gps_ctrl(false);
#endif
	if (gps_dl_conninfra_is_readable_by_hung_value(hung_value)) {
		gps_mcudl_hal_mcu_show_status();
		gps_mcudl_hal_ccif_show_status();
		if (gps_mcudl_hal_bg_is_readable(true))
			gps_mcudl_hal_vdnr_dump();
		gps_dl_hw_dump_host_csr_gps_info(false);
	}
	/* gps_dl_sleep_us(2200, 3200); */

	readable = conninfra_reg_readable();
	hung_value = conninfra_is_bus_hang();
	MDL_LOGW("check3: readable=%d, hung_value=%d, fw_own=%d", readable, hung_value, is_fw_own);
	if (is_fw_own)
		gps_mcudl_hw_conn_force_wake(false);
#endif
}

