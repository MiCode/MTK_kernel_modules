/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_USER_FW_OWN_CTRL_H
#define _GPS_MCUDL_HAL_USER_FW_OWN_CTRL_H

#include "gps_dl_osal.h"

enum gps_mcudl_fw_own_ctrl_user {
	GMDL_FW_OWN_CTRL_BY_POS,
	GMDL_FW_OWN_CTRL_BY_HIF_SEND,
	GMDL_FW_OWN_CTRL_BY_MGMT_CMD,
	GMDL_FW_OWN_CTRL_BY_CCIF,
	GMDL_FW_OWN_CTRL_BY_NON_LPPM,
	GMDL_FW_OWN_CTRL_BY_PRINT_STATUS,
	GMDL_FW_OWN_CTRL_BY_TEST,
	GMDL_FW_OWN_CTRL_USER_NUM
};


void gps_mcudl_hal_user_fw_own_init(enum gps_mcudl_fw_own_ctrl_user user);

/* return true for clear okay, false for failure.
 * this func is for who must rely on the result of clr_fw_own immediately,
 * so consider to trigger assert for failure.
 */
bool gps_mcudl_hal_user_clr_fw_own(enum gps_mcudl_fw_own_ctrl_user user);

/* return true for alreay clear okay, false for failure and you may notify other task to do this.
 * this func is for who cannot wait too long on clr_fw_own but can notify other task to do it.
 */
bool gps_mcudl_hal_user_add_if_fw_own_is_clear(enum gps_mcudl_fw_own_ctrl_user user);

/* return true for notification is sent to gps_kctrld task to set fw own,
 * false for someone had already sent notification and no need to send again here
 */
bool gps_mcudl_hal_user_set_fw_own_may_notify(enum gps_mcudl_fw_own_ctrl_user user);
void gps_mcudl_hal_user_set_fw_own_arrange_notify(void);
void gps_mcudl_hal_user_set_fw_own_do_notify(void);
#if GPS_DL_ON_LINUX
void gps_mcudl_hal_user_timeout_and_ntf_set_fw_own(struct timer_list *p_timer);
#else
void gps_mcudl_hal_user_timeout_and_ntf_set_fw_own(unsigned long data);
#endif

/* this function is for gps_kctrld to set fw own if conditions meet.
 * we always do set fw own in one task(gps_kctrld), and a mechanism is applied to avoid
 * too frequently set/clr by check whether there is recent_clr.
 */
void gps_mcudl_hal_user_set_fw_own_if_no_recent_clr(void);

void gps_mcudl_hal_user_fw_own_deinit(enum gps_mcudl_fw_own_ctrl_user user);
void gps_mcudl_hal_user_fw_own_status_dump(void);

void gps_mcudl_hal_set_non_lppm_sleep_flag(bool enable);
void gps_mcudl_hal_sync_non_flag_lppm_sleep_flag(void);
bool gps_mcudl_hal_get_non_lppm_sleep_flag(void);

/* return true when query okay, otherwise return false when arriving max query times */
bool gps_mcudl_hal_clr_fw_own_fail_handler(void);
bool gps_mcudl_hal_is_fw_own(void);
bool gps_mcudl_hal_force_conn_wake_if_fw_own_is_clear(void);

/* call them once when sys bootup/shutdown or module init/deinit */
void gps_mcudl_hal_user_fw_own_timer_setup(void);
void gps_mcudl_hal_user_fw_own_timer_destroy(void);
void gps_mcudl_hal_user_set_timeout_ms_to_set_fw_own(unsigned int timeout_ms);
void gps_mcudl_hal_user_set_fw_own_op_duration_us_to_warn(unsigned int duration_us);
unsigned int gps_mcudl_hal_user_get_timeout_ms_to_set_fw_own(void);
unsigned int gps_mcudl_hal_user_get_fw_own_op_duration_us_to_warn(void);


#endif /* _GPS_MCUDL_HAL_USER_FW_OWN_CTRL_H */

