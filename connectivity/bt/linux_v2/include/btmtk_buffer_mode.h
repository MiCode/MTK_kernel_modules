/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __BTMTK_BUFFER_MODE_H__
#define __BTMTK_BUFFER_MODE_H__

#include "btmtk_main.h"
#include "btmtk_chip_common.h"

#define BUFFER_MODE_SWITCH_FILE	"wifi.cfg"
#define WIFI_CFG_NAME_PREFIX		"wifi_mt"
#define WIFI_CFG_NAME_SUFFIX		"cfg"
#define BUFFER_MODE_SWITCH_FIELD	"EfuseBufferModeCal"
#define BUFFER_MODE_CFG_FILE		"EEPROM_MT%X_1.bin"
#define EFUSE_MODE			0
#define BIN_FILE_MODE			1
#define AUTO_MODE			2

#define SET_ADDRESS_CMD_PAYLOAD_OFFSET 4

#define SET_EDR_DEF_OFFSET 4
#define SET_BLE_OFFSET 8
#define SET_EDR_MAX_OFFSET 9
#define SET_EDR_MODE_OFFSET 11

#define SET_GRP_CMD_PAYLOAD_OFFSET 8

#define SET_PWR_OFFSET_CMD_PAYLOAD_OFFSET 8

#define SET_COMPENSATION_CMD_PAYLOAD_OFFSET 4
#define BUFFER_MODE_COMPENSATION_LENGTH	16
#define BT0_COMPENSATION_OFFSET		0x376

#define BUFFER_MODE_MAC_LENGTH	6
#define BT0_MAC_OFFSET			0x139
#define BT1_MAC_OFFSET			0x13F

#define BUFFER_MODE_RADIO_LENGTH	4
#define BT0_RADIO_OFFSET		0x145
#define BT1_RADIO_OFFSET		0x149

#define BUFFER_MODE_GROUP_LENGTH	5
#define BT0_GROUP_ANT0_OFFSET		0x984
#define BT0_GROUP_ANT1_OFFSET		0x9BE
#define BT1_GROUP_ANT0_OFFSET		0x9A1
#define BT1_GROUP_ANT1_OFFSET		0x9DB

#define BUFFER_MODE_CAL_LENGTH	6
#define BT0_CAL_ANT0_OFFSET		0x96C
#define BT0_CAL_ANT1_OFFSET		0x9A6
#define BT1_CAL_ANT0_OFFSET		0x989
#define BT1_CAL_ANT1_OFFSET		0x9C3

/* For 6639 (Falcon) */
#define BT_MAC_OFFSET_6639		0x681

#define BUFFER_MODE_RADIO_LENGTH_6639	21
#define BT_RADIO_OFFSET_6639	0xC2E
#define SET_RADIO_OFFSET_6639	5

#define BT_POWER_COMPENSATION_LV9_OFFSET_6639	0x770
#define BT_POWER_COMPENSATION_LV8_OFFSET_6639	0x790
#define BT_POWER_COMPENSATION_LV7_OFFSET_6639	0xCF0
#define SET_COMPENSATION_CMD_PAYLOAD_LV9_OFFSET_6639 5
#define SET_COMPENSATION_CMD_PAYLOAD_LV8_OFFSET_6639 21
#define SET_COMPENSATION_CMD_PAYLOAD_LV7_OFFSET_6639 37

#define BUFFER_MODE_BT_LOSS_LENGTH	1
#define BT_LOSS_OFFSET_6639		0x6A5
#define SET_BT_LOSS_CMD_PAYLOAD_OFFSET		7

/* For 7925 (Owl) */
#define BT_MAC_OFFSET_7925			0x3B2
#define BT_RADIO_OFFSET_7925			0x843
#define BT0_GROUP_ANT0_ENABLE_OFFSET_7925	0x706
#define BT0_GROUP_ANT0_OFFSET_7925		0x729
#define BT0_CAL_ANT0_ENABLE_OFFSET_7925		0x708
#define BT0_CAL_ANT0_OFFSET_7925		0x711
#define BT_LOSS_ENABLE_OFFSET_7925		0x705
#define BT_LOSS_OFFSET_7925			0x709
#define BUFFER_MODE_COMPENSATION_BY_CHANNEL	6
#define SET_COMPENSATION_BY_CHANNEL_CMD_PAYLOAD_OFFSET 8

struct btmtk_buffer_mode_radio_struct {
	u8 radio_0;	/* bit 0-5:edr_init_pwr, 6-7:edr_pwr_mode */
	u8 radio_1;	/* bit 0-5:edr_max_pwr, 6-7:reserved */
	u8 radio_2;	/* bit 0-5:ble_default_pwr, 6-7:reserved */
	u8 radio_3;	/* reserved */
};

struct btmtk_buffer_mode_struct {
	struct btmtk_dev *bdev;

	unsigned char		file_name[MAX_BIN_FILE_NAME_LEN];
	int			efuse_mode;

	u8 bt0_mac[BUFFER_MODE_MAC_LENGTH];
	u8 bt1_mac[BUFFER_MODE_MAC_LENGTH];
	struct btmtk_buffer_mode_radio_struct bt0_radio;
	struct btmtk_buffer_mode_radio_struct bt1_radio;
	u8 bt0_ant0_grp_boundary[BUFFER_MODE_GROUP_LENGTH];
	u8 bt0_ant1_grp_boundary[BUFFER_MODE_GROUP_LENGTH];
	u8 bt1_ant0_grp_boundary[BUFFER_MODE_GROUP_LENGTH];
	u8 bt1_ant1_grp_boundary[BUFFER_MODE_GROUP_LENGTH];
	u8 bt0_ant0_pwr_offset[BUFFER_MODE_CAL_LENGTH];
	u8 bt0_ant1_pwr_offset[BUFFER_MODE_CAL_LENGTH];
	u8 bt1_ant0_pwr_offset[BUFFER_MODE_CAL_LENGTH];
	u8 bt1_ant1_pwr_offset[BUFFER_MODE_CAL_LENGTH];
	u8 bt0_ant0_compensation[BUFFER_MODE_COMPENSATION_LENGTH];
	u8 bt_radio_6639[BUFFER_MODE_RADIO_LENGTH_6639];
	u8 bt_compensation_lv9_6639[BUFFER_MODE_COMPENSATION_LENGTH];
	u8 bt_compensation_lv8_6639[BUFFER_MODE_COMPENSATION_LENGTH];
	u8 bt_compensation_lv7_6639[BUFFER_MODE_COMPENSATION_LENGTH];
	u8 bt_compensation_by_channel[BUFFER_MODE_COMPENSATION_BY_CHANNEL];
	u8 bt_loss;
};

int btmtk_buffer_mode_send(struct btmtk_dev *bdev);
void btmtk_buffer_mode_initialize(struct btmtk_dev *bdev);
#endif /* __BTMTK_BUFFER_MODE_H__ */

