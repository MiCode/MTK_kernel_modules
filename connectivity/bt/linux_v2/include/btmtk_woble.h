/* SPDX-License-Identifier: GPL-2.0 */  
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#ifndef __BTMTK_WOBLE_H__
#define __BTMTK_WOBLE_H__
#include "btmtk_define.h"
#include "btmtk_main.h"

/* Define for WoBLE */
#define WOBLE_SETTING_COUNT 10
#define WOBLE_EVENT_INTERVAL_TIMO	500
#define WOBLE_COMP_EVENT_TIMO		5000

/* WOBX attribute type */
#define WOBX_TRIGGER_INFO_ADDR_TYPE		1
#define WOBX_TRIGGER_INFO_ADV_DATA_TYPE		2
#define WOBX_TRIGGER_INFO_TRACE_LOG_TYPE	3
#define WOBX_TRIGGER_INFO_SCAN_LOG_TYPE		4
#define WOBX_TRIGGER_INFO_TRIGGER_CNT_TYPE	5

struct btmtk_woble {
	unsigned char	*woble_setting_file_name;
	unsigned int	woble_setting_len;

	struct fw_cfg_struct	woble_setting_apcf[WOBLE_SETTING_COUNT];
	struct fw_cfg_struct	woble_setting_apcf_fill_mac[WOBLE_SETTING_COUNT];
	struct fw_cfg_struct	woble_setting_apcf_fill_mac_location[WOBLE_SETTING_COUNT];

	struct fw_cfg_struct	woble_setting_radio_off;
	struct fw_cfg_struct	woble_setting_wakeup_type;
	struct fw_cfg_struct	woble_setting_radio_off_status_event;
	/* complete event */
	struct fw_cfg_struct	woble_setting_radio_off_comp_event;

	struct fw_cfg_struct	woble_setting_radio_on;
	struct fw_cfg_struct	woble_setting_radio_on_status_event;
	struct fw_cfg_struct	woble_setting_radio_on_comp_event;

	/* set apcf after resume(radio on) */
	struct fw_cfg_struct	woble_setting_apcf_resume[WOBLE_SETTING_COUNT];

	/* Foe Woble eint */
	unsigned int wobt_irq;
	int wobt_irqlevel;
	atomic_t irq_enable_count;
	struct input_dev *WoBLEInputDev;
	void *bdev;
};

int btmtk_woble_suspend(struct btmtk_woble *bt_woble);
int btmtk_woble_resume(struct btmtk_woble *bt_woble);
int btmtk_woble_initialize(struct btmtk_dev *bdev, struct btmtk_woble *bt_woble);
void btmtk_woble_uninitialize(struct btmtk_woble *bt_woble);
void btmtk_woble_wake_unlock(struct btmtk_dev *bdev);
#if WAKEUP_BT_IRQ
void btmtk_sdio_irq_wake_lock_timeout(struct btmtk_dev *bdev);
#endif
int btmtk_send_apcf_reserved(struct btmtk_dev *bdev);

#endif /* __BTMTK_WOBLE_H__ */
