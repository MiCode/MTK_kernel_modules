/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_BT_MCU_H_
#define _FW_LOG_BT_MCU_H_

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
int fw_log_bt_mcu_init(void);
void fw_log_bt_mcu_deinit(void);
int fw_log_bt_mcu_register_event_cb(void);
#endif

#endif /*_FW_LOG_BT_MCU_H_*/
