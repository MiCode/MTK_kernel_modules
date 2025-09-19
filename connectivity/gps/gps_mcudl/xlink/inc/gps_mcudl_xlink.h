/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_XLINK_H
#define _GPS_MCUDL_XLINK_H

#include "gps_mcudl_data_intf_type.h"

enum gps_mcudl_xid {
	GPS_MDLX_MCUSYS,
	GPS_MDLX_MCUFN,
	GPS_MDLX_MNL,
	GPS_MDLX_AGENT,
	GPS_MDLX_NMEA,
	GPS_MDLX_GDLOG,
	GPS_MDLX_PMTK,
	GPS_MDLX_MEAS,
	GPS_MDLX_CORR,
	GPS_MDLX_DSP0,
	GPS_MDLX_DSP1,
	GPS_MDLX_GDLOG2,   /* for ko debug purpose */

	GPS_MDLX_AOL_TEST, /* for test purpose, send test cmd to GPS_MCU */
	GPS_MDLX_MPE_TEST, /* for test purpose, send test cmd to GPS_MCU */
	GPS_MDLX_SCP_TEST, /* for test purpose, recv test cmd from GPS_MCU */

	GPS_MDLX_LPPM,     /* for gps low power positioning mode */
	GPS_MDLX_MPELOG,   /* for MPE log */
	GPS_MDLX_MPELOG2,  /* for MPE log */

	GPS_MDLX_CH_NUM
};

enum gps_mcudl_xlink_event_id {
	GPS_MCUDL_EVT_LINK_OPEN,
	GPS_MCUDL_EVT_LINK_CLOSE,
	GPS_MCUDL_EVT_LINK_WRITE,
	GPS_MCUDL_EVT_LINK_READ,
	GPS_MCUDL_EVT_LINK_RESET,
	GPS_MCUDL_EVT_LINK_PRE_CONN_RESET,
	GPS_MCUDL_EVT_LINK_POST_CONN_RESET,
	GPS_MCUDL_EVT_LINK_UPDATE_SETTING,
	GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS,
	GPS_MCUDL_EVT_LINK_PRINT_DATA_STATUS,
	GPS_MCUDL_EVT_LINK_FW_LOG_ON,
	GPS_MCUDL_EVT_LINK_FW_LOG_OFF,
	GPS_MCUDL_EVT_LINK_RESET2,
	GPS_MCUDL_LINK_EVT_NUM,
};

const char *gpsmdl_xid_name(enum gps_mcudl_xid xid);
const char *gps_mcudl_xlink_event_name(enum gps_mcudl_xlink_event_id event);

/* if it's true,
 *    - opening the link will trigger or keep MCU on
 *    - MCU assert will cause the link to be resetting also
 */
bool gps_mcudl_xlink_is_connected_to_mcu_lifecycle(enum gps_mcudl_xid x_id);

bool gps_mcudl_xlink_is_connected_mnlbin(enum gps_mcudl_xid x_id);

/* if it's true, wake_lock will be taken during the link is opened
 */
bool gps_mcudl_xlink_is_requiring_wake_lock(enum gps_mcudl_xid x_id);
unsigned int gps_mcudl_xlink_get_bitmask_requiring_wake_lock(void);

void gps_mcudl_xlink_event_send(enum gps_mcudl_xid link_id,
	enum gps_mcudl_xlink_event_id evt);
void gps_mcudl_xlink_event_proc(enum gps_mcudl_xid link_id,
	enum gps_mcudl_xlink_event_id evt);
extern bool g_gps_fw_log_is_on;
extern unsigned int g_gps_fw_log_irq_cnt;

void gps_mcudl_xlink_trigger_print_hw_status(void);
void gps_mcudl_xlink_test_fw_own_ctrl(bool to_set);
void gps_mcudl_xlink_test_toggle_ccif(unsigned int ch);
bool gps_mcudl_xlink_test_toggle_reset_by_gps_hif(unsigned int type);
void gps_mcudl_xlink_test_read_mcu_reg(unsigned int addr, unsigned int bytes);
void gps_mcudl_xlink_test_query_ver(void);
void gps_mcudl_xlink_test_wakeup_ap_later(unsigned int data);
void gps_mcudl_xlink_test_send_4byte_mgmt_data(unsigned int data_4byte);
void gps_mcudl_xlink_test_bypass_mcu2ap_data(bool bypass);

void gps_mcudl_xlink_fw_log_ctrl(bool on);

extern int gps_mcudl_hal_link_power_ctrl(enum gps_mcudl_xid xid, int op);
void gps_mcudl_hal_may_set_link_power_flag(enum gps_mcudl_xid xid,
	bool power_ctrl);
extern int gps_mcudl_hal_conn_power_ctrl(enum gps_mcudl_xid xid, int op);
extern bool g_gps_mcudl_ever_do_coredump;

#endif /* _GPS_MCUDL_XLINK_H */

