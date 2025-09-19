/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_log.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcu_hif_api.h"


bool gps_mcudl_xlink_is_connected_to_mcu_lifecycle(enum gps_mcudl_xid x_id)
{
	bool not_connected_to;

	not_connected_to = (
		x_id == GPS_MDLX_GDLOG2 ||
		x_id == GPS_MDLX_MPELOG2 ||
		false
	);

	return !not_connected_to;
}

bool gps_mcudl_xlink_is_connected_mnlbin(enum gps_mcudl_xid x_id)
{
	bool connected_to;

	connected_to = (
		x_id == GPS_MDLX_MNL ||
		x_id == GPS_MDLX_AGENT ||
		x_id == GPS_MDLX_NMEA ||
		x_id == GPS_MDLX_PMTK ||
		x_id == GPS_MDLX_MEAS ||
		x_id == GPS_MDLX_CORR ||
		/*
		 * Bind LPPM node with MNL.bin load done event.
		 *
		 * Once it becomes LINK_OPENED, mnld will ack LPP mode init done to SCP,
		 * then SCP will communicate with MNL.bin.
		 *
		 * More than this, we bind it to SCIF ready event also for same reason,
		 * due to they communicate via SCIF.
		 */
		x_id == GPS_MDLX_LPPM ||
		false
	);

	return !!connected_to;
}

bool gps_mcudl_xlink_is_requiring_wake_lock(enum gps_mcudl_xid x_id)
{
	return !!(1UL << x_id &
		gps_mcudl_xlink_get_bitmask_requiring_wake_lock());
}

unsigned int gps_mcudl_xlink_get_bitmask_requiring_wake_lock(void)
{
	unsigned int bitmask;

	bitmask = (
		(1UL << GPS_MDLX_GDLOG) &
		(1UL << GPS_MDLX_GDLOG2) &
		(1UL << GPS_MDLX_MPELOG) &
		(1UL << GPS_MDLX_MPELOG2) &
		(1UL << GPS_MDLX_LPPM) &
		0xFFFFFFFF
	);

	return bitmask;
}

void gps_mcudl_xlink_trigger_print_hw_status(void)
{
	gps_mcudl_xlink_event_send(GPS_MDLX_MCUSYS, GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS);
}

void gps_mcudl_xlink_fw_log_ctrl(bool on)
{
	if (on)
		gps_mcudl_xlink_event_send(GPS_MDLX_MCUSYS, GPS_MCUDL_EVT_LINK_FW_LOG_ON);
	else
		gps_mcudl_xlink_event_send(GPS_MDLX_MCUSYS, GPS_MCUDL_EVT_LINK_FW_LOG_OFF);
}

void gps_mcudl_xlink_test_fw_own_ctrl(bool to_set)
{
	bool ntf = false;

	if (to_set) {
		ntf = gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_TEST);
		MDL_LOGW("to_set=%d, ntf=%d", to_set, ntf);
		return;
	}

	ntf = !gps_mcudl_hal_user_add_if_fw_own_is_clear(GMDL_FW_OWN_CTRL_BY_TEST);
	if (ntf)
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_TEST_CLR_FW_OWN);
	MDL_LOGW("to_set=%d, ntf=%d", to_set, ntf);
}

void gps_mcudl_xlink_test_toggle_ccif(unsigned int ch)
{
	if (ch >= 8)
		return;
	gps_mcudl_hal_ccif_tx_prepare(ch);
	gps_mcudl_hal_ccif_tx_trigger(ch);
}

bool gps_mcudl_xlink_test_toggle_reset_by_gps_hif(unsigned int type)
{
	bool is_okay;
	unsigned char buf[2];

	if (gps_mcudl_hal_get_open_flag() != 0) {
		buf[0] = '\x04';
		buf[1] = (unsigned char)(type & 0xFF);
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, &buf[0], 2);
		MDL_LOGW("write cmd4, type=%u, is_ok=%d", type, is_okay);
		return is_okay;
	}

	MDL_LOGW("GPS is not active, FalseAlarm");
	return false;
}

void gps_mcudl_xlink_test_read_mcu_reg(unsigned int addr, unsigned int bytes)
{
	bool is_okay;
	unsigned char buf[12];

	memset(&buf[0], 0, sizeof(buf));
	buf[0] = 5;
	buf[4] = (addr >>  0) & 0xFF;
	buf[5] = (addr >>  8) & 0xFF;
	buf[6] = (addr >> 16) & 0xFF;
	buf[7] = (addr >> 24) & 0xFF;
	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, &buf[0], 12);
	MDL_LOGW("write cmd5, is_ok=%d", is_okay);
}

void gps_mcudl_xlink_test_query_ver(void)
{
	bool is_okay = false;

	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, "\x06", 1);
	MDL_LOGW("write cmd6, is_ok=%d", is_okay);
}

void gps_mcudl_xlink_test_wakeup_ap_later(unsigned int data)
{
	bool is_okay = false;
	unsigned char u8_data;
	unsigned char send_data[2];

	u8_data = data & 0xff;
	send_data[0] = '\x09';
	send_data[1] = u8_data;
	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, &send_data[0], 2);
	MDL_LOGW("write cmd9: data=0x%02x, is_ok=%d", u8_data, is_okay);
}

void gps_mcudl_xlink_test_send_4byte_mgmt_data(unsigned int data_4byte)
{
	bool is_okay = false;

	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT,
		(const unsigned char *)&data_4byte, 4);
	MDL_LOGW("write 0x%08x, is_ok=%d", data_4byte, is_okay);
}

void gps_mcudl_xlink_test_bypass_mcu2ap_data(bool bypass)
{
	MDL_LOGW("bypass=%d", bypass);
	gps_mcudl_mcu2ap_test_bypass_set(bypass);
}

