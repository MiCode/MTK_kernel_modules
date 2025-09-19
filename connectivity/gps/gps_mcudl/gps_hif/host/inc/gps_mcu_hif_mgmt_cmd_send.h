/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCU_HIF_MGMT_CMD_SEND_H
#define _GPS_MCU_HIF_MGMT_CMD_SEND_H

#include "gps_mcu_hif_api.h"

bool gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(bool enable);

enum gps_mcudl_mgmt_cmd_id {
	GPS_MCUDL_CMD_LPBK,
	GPS_MCUDL_CMD_OFL_INIT,
	GPS_MCUDL_CMD_OFL_DEINIT,
	GPS_MCUDL_CMD_FW_LOG_CTRL,
	GPS_MCUDL_CMD_NUM
};

void gps_mcudl_mgmt_cmd_state_init_all(void);
bool gps_mcudl_mgmt_cmd_pre_send(enum gps_mcudl_mgmt_cmd_id cmd_id);
void gps_mcudl_mgmt_cmd_on_ack(enum gps_mcudl_mgmt_cmd_id cmd_id);
bool gps_mcudl_mgmt_cmd_wait_ack(enum gps_mcudl_mgmt_cmd_id cmd_id, int timeout_ms);

#endif /* _GPS_MCU_HIF_MGMT_CMD_SEND_H */

