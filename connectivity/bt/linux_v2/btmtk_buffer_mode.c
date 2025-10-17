/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_buffer_mode.h"

static struct btmtk_buffer_mode_struct btmtk_buffer_mode;

static int btmtk_buffer_mode_check_auto_mode(struct btmtk_buffer_mode_struct *buffer_mode)
{
	u16 addr = 1;
	u8 value = 0;

	if (buffer_mode->efuse_mode != AUTO_MODE)
		return 0;

	if (btmtk_efuse_read(buffer_mode->bdev, addr, &value)) {
		BTMTK_WARN("read fail");
		BTMTK_WARN("Use EEPROM Bin file mode");
		buffer_mode->efuse_mode = BIN_FILE_MODE;
		return -EIO;
	}

	if (value == ((buffer_mode->bdev->chip_id & 0xFF00) >> 8)) {
		BTMTK_WARN("get efuse[1]: 0x%02x", value);
		BTMTK_WARN("use efuse mode");
		buffer_mode->efuse_mode = EFUSE_MODE;
	} else {
		BTMTK_WARN("get efuse[1]: 0x%02x", value);
		BTMTK_WARN("Use EEPROM Bin file mode");
		buffer_mode->efuse_mode = BIN_FILE_MODE;
	}

	return 0;
}

static int btmtk_buffer_mode_parse_mode(uint8_t *buf, size_t buf_size)
{
	int efuse_mode = EFUSE_MODE;
	char *p_buf = NULL;
	char *ptr = NULL, *p = NULL;

	if (!buf) {
		BTMTK_WARN("buf is null");
		return efuse_mode;
	} else if (buf_size < (strlen(BUFFER_MODE_SWITCH_FIELD) + 2)) {
		BTMTK_WARN("incorrect buf size(%d)", (int)buf_size);
		return efuse_mode;
	}

	p_buf = kmalloc(buf_size + 1, GFP_KERNEL);
	if (!p_buf)
		return efuse_mode;
	memcpy(p_buf, buf, buf_size);
	p_buf[buf_size] = '\0';

	/* find string */
	p = ptr = strstr(p_buf, BUFFER_MODE_SWITCH_FIELD);
	if (!ptr) {
		BTMTK_ERR("Can't find %s", BUFFER_MODE_SWITCH_FIELD);
		goto out;
	}

	if (p > p_buf) {
		p--;
		while ((*p == ' ') && (p != p_buf))
			p--;
		if (*p == '#') {
			BTMTK_ERR("It's not EEPROM - Bin file mode");
			goto out;
		}
	}

	/* check access mode */
	ptr += (strlen(BUFFER_MODE_SWITCH_FIELD) + 1);
	BTMTK_WARN("It's EEPROM bin mode: %c", *ptr);
	efuse_mode = *ptr - '0';
	if (efuse_mode > AUTO_MODE)
		efuse_mode = EFUSE_MODE;
out:
	kfree(p_buf);
	return efuse_mode;
}

static int btmtk_buffer_mode_set_addr(struct btmtk_buffer_mode_struct *buffer_mode)
{
	int ret = 0;
	struct data_struct cmd = {0}, event = {0};

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_ADDRESS_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_ADDRESS_EVT, event);

	if (buffer_mode->bt0_mac[0] == 0x00 && buffer_mode->bt0_mac[1] == 0x00
		&& buffer_mode->bt0_mac[2] == 0x00 && buffer_mode->bt0_mac[3] == 0x00
		&& buffer_mode->bt0_mac[4] == 0x00 && buffer_mode->bt0_mac[5] == 0x00) {
		BTMTK_WARN("BDAddr is Zero, not set");
	} else {
		cmd.content[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 5] = buffer_mode->bt0_mac[0];
		cmd.content[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 4] = buffer_mode->bt0_mac[1];
		cmd.content[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 3] = buffer_mode->bt0_mac[2];
		cmd.content[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 2] = buffer_mode->bt0_mac[3];
		cmd.content[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 1] = buffer_mode->bt0_mac[4];
		cmd.content[SET_ADDRESS_CMD_PAYLOAD_OFFSET] = buffer_mode->bt0_mac[5];

		BTMTK_INFO("%s: SEND BDADDR = "MACSTR, __func__, MAC2STR(buffer_mode->bt0_mac));
		ret = btmtk_main_send_cmd(buffer_mode->bdev,
				cmd.content, cmd.len,
				event.content, event.len,
				DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	}

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_buffer_mode_set_radio(struct btmtk_buffer_mode_struct *buffer_mode)
{
	struct data_struct cmd = {0}, event = {0};
	int ret = 0, i = 0;

	BTMTK_INFO("%s enter", __func__);

	if (is_connac3(buffer_mode->bdev->chip_id)) {
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_RADIO_CMD_6639, cmd);
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_RADIO_EVT_6639, event);

		for (i = 0; i < BUFFER_MODE_RADIO_LENGTH_6639; i++) {
			cmd.content[SET_RADIO_OFFSET_6639 + i] = buffer_mode->bt_radio_6639[i];
		}
	} else {
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_RADIO_CMD, cmd);
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_RADIO_EVT, event);

		cmd.content[SET_EDR_DEF_OFFSET] = buffer_mode->bt0_radio.radio_0 & 0x3F;		/* edr_init_pwr */
		cmd.content[SET_BLE_OFFSET] = buffer_mode->bt0_radio.radio_2 & 0x3F;		/* ble_default_pwr */
		cmd.content[SET_EDR_MAX_OFFSET] = buffer_mode->bt0_radio.radio_1 & 0x3F;		/* edr_max_pwr */
		cmd.content[SET_EDR_MODE_OFFSET] = (buffer_mode->bt0_radio.radio_0 & 0xC0) >> 6;	/* edr_pwr_mode */
	}

	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send", __func__);
	ret = btmtk_main_send_cmd(buffer_mode->bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_buffer_mode_set_group_boundary(struct btmtk_buffer_mode_struct *buffer_mode)
{
	struct data_struct cmd = {0}, event = {0};
	int ret = 0;

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_GRP_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_GRP_EVT, event);

	memcpy(&cmd.content[SET_GRP_CMD_PAYLOAD_OFFSET], buffer_mode->bt0_ant0_grp_boundary, BUFFER_MODE_GROUP_LENGTH);

	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send", __func__);
	ret = btmtk_main_send_cmd(buffer_mode->bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_buffer_mode_set_power_offset(struct btmtk_buffer_mode_struct *buffer_mode)
{
	struct data_struct cmd = {0}, event = {0};
	int ret = 0;

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_PWR_OFFSET_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_PWR_OFFSET_EVT, event);

	memcpy(&cmd.content[SET_PWR_OFFSET_CMD_PAYLOAD_OFFSET],
			buffer_mode->bt0_ant0_pwr_offset, BUFFER_MODE_CAL_LENGTH);

	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send", __func__);
	ret = btmtk_main_send_cmd(buffer_mode->bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_buffer_mode_set_compensation(struct btmtk_buffer_mode_struct *buffer_mode)
{
	struct data_struct cmd = {0}, event = {0};
	int ret = 0;

	BTMTK_INFO("%s enter", __func__);

	if (is_mt6639(buffer_mode->bdev->chip_id)) {
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_COMPENSATION_CMD_6639, cmd);
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_COMPENSATION_EVT_6639, event);

		memcpy(&cmd.content[SET_COMPENSATION_CMD_PAYLOAD_LV9_OFFSET_6639],
				buffer_mode->bt_compensation_lv9_6639, BUFFER_MODE_COMPENSATION_LENGTH);
		memcpy(&cmd.content[SET_COMPENSATION_CMD_PAYLOAD_LV8_OFFSET_6639],
				buffer_mode->bt_compensation_lv8_6639, BUFFER_MODE_COMPENSATION_LENGTH);
		memcpy(&cmd.content[SET_COMPENSATION_CMD_PAYLOAD_LV7_OFFSET_6639],
				buffer_mode->bt_compensation_lv7_6639, BUFFER_MODE_COMPENSATION_LENGTH);
	} else {
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_COMPENSATION_CMD, cmd);
		BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_COMPENSATION_EVT, event);

		memcpy(&cmd.content[SET_COMPENSATION_CMD_PAYLOAD_OFFSET],
				buffer_mode->bt0_ant0_compensation, BUFFER_MODE_COMPENSATION_LENGTH);
	}

	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send", __func__);
	ret = btmtk_main_send_cmd(buffer_mode->bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_buffer_mode_set_compensation_by_channel(struct btmtk_buffer_mode_struct *buffer_mode)
{
	struct data_struct cmd = {0}, event = {0};
	int ret = 0;

	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_COMPENSATION_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_COMPENSATION_EVT, event);

	memcpy(&cmd.content[SET_COMPENSATION_BY_CHANNEL_CMD_PAYLOAD_OFFSET],
		buffer_mode->bt_compensation_by_channel, BUFFER_MODE_COMPENSATION_BY_CHANNEL);

	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send", __func__);
	ret = btmtk_main_send_cmd(buffer_mode->bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);

	return ret;
}

static int btmtk_buffer_mode_set_bt_loss(struct btmtk_buffer_mode_struct *buffer_mode)
{
	struct data_struct cmd = {0}, event = {0};
	int ret = 0;

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_BT_LOSS_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(buffer_mode->bdev, SET_BT_LOSS_EVT, event);

	cmd.content[SET_BT_LOSS_CMD_PAYLOAD_OFFSET] = buffer_mode->bt_loss;

	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send", __func__);
	ret = btmtk_main_send_cmd(buffer_mode->bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

int btmtk_buffer_mode_send(struct btmtk_dev *bdev)
{
	int ret = 0;
	struct btmtk_buffer_mode_struct *buffer_mode = NULL;

	buffer_mode = (struct btmtk_buffer_mode_struct *)bdev->buffer_mode;

	if (buffer_mode == NULL) {
		BTMTK_INFO("buffer_mode is NULL, not support");
		return -EIO;
	}

	if (btmtk_buffer_mode_check_auto_mode(buffer_mode)) {
		BTMTK_ERR("check auto mode failed");
		return -EIO;
	}

	if (buffer_mode->efuse_mode == BIN_FILE_MODE) {
		ret = btmtk_buffer_mode_set_addr(buffer_mode);
		if (ret < 0)
			BTMTK_ERR("set addr failed");

		ret = btmtk_buffer_mode_set_radio(buffer_mode);
		if (ret < 0)
			BTMTK_ERR("set radio failed");

		if ((is_mt7961(buffer_mode->bdev->chip_id) && buffer_mode->bdev->flavor) || is_mt6639(buffer_mode->bdev->chip_id)) {
			ret = btmtk_buffer_mode_set_compensation(buffer_mode);
			if (ret < 0)
				BTMTK_ERR("set compensation failed");

			if (is_mt6639(buffer_mode->bdev->chip_id)) {
				ret = btmtk_buffer_mode_set_bt_loss(buffer_mode);
				if (ret < 0)
					BTMTK_ERR("set bt loss failed");
			}
		} else if (is_mt7925(buffer_mode->bdev->chip_id)) {
			ret = btmtk_buffer_mode_set_compensation_by_channel(buffer_mode);
			if (ret < 0)
				BTMTK_ERR("set compensation by channel failed");

			ret = btmtk_buffer_mode_set_group_boundary(buffer_mode);
			if (ret < 0)
				BTMTK_ERR("set group boundary failed");

			ret = btmtk_buffer_mode_set_bt_loss(buffer_mode);
			if (ret < 0)
				BTMTK_ERR("set bt loss failed");
		} else {
			ret = btmtk_buffer_mode_set_group_boundary(buffer_mode);
			if (ret < 0)
				BTMTK_ERR("set group_boundary failed");

			ret = btmtk_buffer_mode_set_power_offset(buffer_mode);
			if (ret < 0)
				BTMTK_ERR("set power_offset failed");
		}
	}
	return 0;
}

void btmtk_buffer_mode_initialize(struct btmtk_dev *bdev)
{
	int ret = 0;
	u32 code_len = 0;
	char buffer_mode_switch_file_name[MAX_BIN_FILE_NAME_LEN] = {0};
	unsigned int chip_id = 0;

	btmtk_buffer_mode.bdev = bdev;

	if (bdev->chip_id == 0x7920) {
		chip_id = 0x7961;
	} else {
		chip_id = bdev->chip_id;
	}

	(void)snprintf(buffer_mode_switch_file_name, MAX_BIN_FILE_NAME_LEN, "%s%x.%s",
		WIFI_CFG_NAME_PREFIX, chip_id & 0xffff, WIFI_CFG_NAME_SUFFIX);
	BTMTK_INFO("%s: buffer mode file name is %s", __func__, buffer_mode_switch_file_name);

	ret = btmtk_load_code_from_setting_files(buffer_mode_switch_file_name, bdev->intf_dev, &code_len, bdev);
	if (ret < 0) {
		BTMTK_ERR("%s: btmtk_load_code_from_setting_files %s failed!", __func__, buffer_mode_switch_file_name);
		if (ret != -ENOENT) {
			return;
		}

		if (btmtk_load_code_from_setting_files(BUFFER_MODE_SWITCH_FILE, bdev->intf_dev, &code_len, bdev) != 0) {
			BTMTK_ERR("%s: btmtk_load_code_from_setting_files %s failed!", __func__, BUFFER_MODE_SWITCH_FILE);
			return;
		}
	}

	btmtk_buffer_mode.efuse_mode = btmtk_buffer_mode_parse_mode(bdev->setting_file, code_len);
	if (btmtk_buffer_mode.efuse_mode == EFUSE_MODE)
		return;

	if (is_connac3(bdev->chip_id)) {
		(void)snprintf(btmtk_buffer_mode.file_name, MAX_BIN_FILE_NAME_LEN, "EEPROM_MT%04x.bin",
					bdev->chip_id & 0xffff);
	} else {
		if (bdev->flavor)
			(void)snprintf(btmtk_buffer_mode.file_name, MAX_BIN_FILE_NAME_LEN, "EEPROM_MT%04x_1a.bin",
					bdev->chip_id & 0xffff);
		else
			(void)snprintf(btmtk_buffer_mode.file_name, MAX_BIN_FILE_NAME_LEN, "EEPROM_MT%04x_1.bin",
					bdev->chip_id & 0xffff);
	}

	ret = btmtk_load_code_from_setting_files(btmtk_buffer_mode.file_name, bdev->intf_dev, &code_len, bdev);
	if (ret < 0) {
		BTMTK_ERR("set load %s failed", btmtk_buffer_mode.file_name);
		return;
	}

	if (is_mt6639(bdev->chip_id)) {
		if (BT_MAC_OFFSET_6639 + BUFFER_MODE_MAC_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_mac, &bdev->setting_file[BT_MAC_OFFSET_6639],
					BUFFER_MODE_MAC_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_MAC_OFFSET_6639);

		if (BT_RADIO_OFFSET_6639 + BUFFER_MODE_RADIO_LENGTH_6639 <= code_len)
			memcpy(btmtk_buffer_mode.bt_radio_6639, &bdev->setting_file[BT_RADIO_OFFSET_6639],
					BUFFER_MODE_RADIO_LENGTH_6639);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_RADIO_OFFSET_6639);

		if (BT_POWER_COMPENSATION_LV9_OFFSET_6639 + BUFFER_MODE_COMPENSATION_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt_compensation_lv9_6639, &bdev->setting_file[BT_POWER_COMPENSATION_LV9_OFFSET_6639],
					BUFFER_MODE_COMPENSATION_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_POWER_COMPENSATION_LV9_OFFSET_6639);

		if (BT_POWER_COMPENSATION_LV8_OFFSET_6639 + BUFFER_MODE_COMPENSATION_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt_compensation_lv8_6639, &bdev->setting_file[BT_POWER_COMPENSATION_LV8_OFFSET_6639],
					BUFFER_MODE_COMPENSATION_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_POWER_COMPENSATION_LV8_OFFSET_6639);

		if (BT_POWER_COMPENSATION_LV7_OFFSET_6639 + BUFFER_MODE_COMPENSATION_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt_compensation_lv7_6639, &bdev->setting_file[BT_POWER_COMPENSATION_LV7_OFFSET_6639],
					BUFFER_MODE_COMPENSATION_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_POWER_COMPENSATION_LV7_OFFSET_6639);

		if (BT_LOSS_OFFSET_6639 + BUFFER_MODE_BT_LOSS_LENGTH <= code_len)
			btmtk_buffer_mode.bt_loss = bdev->setting_file[BT_LOSS_OFFSET_6639];
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_LOSS_OFFSET_6639);
	} else if (is_mt7925(bdev->chip_id)) {
		if (BT_MAC_OFFSET_7925 + BUFFER_MODE_MAC_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_mac, &bdev->setting_file[BT_MAC_OFFSET_7925],
					BUFFER_MODE_MAC_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_MAC_OFFSET_7925);

		if (BT_RADIO_OFFSET_7925 + BUFFER_MODE_RADIO_LENGTH_6639 <= code_len)
			memcpy(&btmtk_buffer_mode.bt_radio_6639, &bdev->setting_file[BT_RADIO_OFFSET_7925],
					BUFFER_MODE_RADIO_LENGTH_6639);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT_RADIO_OFFSET_7925);

		if (BT0_GROUP_ANT0_ENABLE_OFFSET_7925 + 1 <= code_len) {
			if (bdev->setting_file[BT0_GROUP_ANT0_ENABLE_OFFSET_7925] == 1) {
				if (BT0_GROUP_ANT0_OFFSET_7925 + BUFFER_MODE_GROUP_LENGTH <= code_len)
					memcpy(btmtk_buffer_mode.bt0_ant0_grp_boundary, &bdev->setting_file[BT0_GROUP_ANT0_OFFSET_7925],
						BUFFER_MODE_GROUP_LENGTH);
				else
					BTMTK_ERR("%s: error address, %x", __func__, BT0_GROUP_ANT0_OFFSET_7925);
			} else {
				BTMTK_INFO("%s: channel boundary disable", __func__);
			}
		} else {
			BTMTK_ERR("%s: error address, %x", __func__, BT0_GROUP_ANT0_ENABLE_OFFSET_7925);
		}

		if (BT0_CAL_ANT0_ENABLE_OFFSET_7925 + 1 <= code_len) {
			if (bdev->setting_file[BT0_CAL_ANT0_ENABLE_OFFSET_7925] == 1) {
				if (BT0_CAL_ANT0_OFFSET_7925 + BUFFER_MODE_COMPENSATION_BY_CHANNEL <= code_len)
					memcpy(btmtk_buffer_mode.bt_compensation_by_channel, &bdev->setting_file[BT0_CAL_ANT0_OFFSET_7925],
						BUFFER_MODE_COMPENSATION_BY_CHANNEL);
				else
					BTMTK_ERR("%s: error address, %x", __func__, BT0_CAL_ANT0_OFFSET_7925);
			} else {
				BTMTK_INFO("%s: compensation disable", __func__);
			}
		} else {
			BTMTK_ERR("%s: error address, %x", __func__, BT0_CAL_ANT0_ENABLE_OFFSET_7925);
		}

		if (BT_LOSS_ENABLE_OFFSET_7925 + 1 <= code_len) {
			if (bdev->setting_file[BT_LOSS_ENABLE_OFFSET_7925] == 1) {
				if (BT_LOSS_OFFSET_7925 + BUFFER_MODE_BT_LOSS_LENGTH <= code_len)
					btmtk_buffer_mode.bt_loss = bdev->setting_file[BT_LOSS_OFFSET_7925];
				else
					BTMTK_ERR("%s: error address, %x", __func__, BT_LOSS_OFFSET_7925);
			} else {
				BTMTK_INFO("%s: compensation disable", __func__);
			}
		} else {
			BTMTK_ERR("%s: error address, %x", __func__, BT_LOSS_ENABLE_OFFSET_7925);
		}
	} else {
		if (BT0_MAC_OFFSET + BUFFER_MODE_MAC_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_mac, &bdev->setting_file[BT0_MAC_OFFSET],
					BUFFER_MODE_MAC_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT0_MAC_OFFSET);

		if (BT1_MAC_OFFSET + BUFFER_MODE_MAC_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt1_mac, &bdev->setting_file[BT1_MAC_OFFSET],
					BUFFER_MODE_MAC_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT1_MAC_OFFSET);

		if (BT0_RADIO_OFFSET + BUFFER_MODE_RADIO_LENGTH <= code_len)
			memcpy(&btmtk_buffer_mode.bt0_radio, &bdev->setting_file[BT0_RADIO_OFFSET],
					BUFFER_MODE_RADIO_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT0_RADIO_OFFSET);

		if (BT1_RADIO_OFFSET + BUFFER_MODE_RADIO_LENGTH <= code_len)
			memcpy(&btmtk_buffer_mode.bt1_radio, &bdev->setting_file[BT1_RADIO_OFFSET],
					BUFFER_MODE_RADIO_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT1_RADIO_OFFSET);

		if (BT0_GROUP_ANT0_OFFSET + BUFFER_MODE_GROUP_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_ant0_grp_boundary, &bdev->setting_file[BT0_GROUP_ANT0_OFFSET],
					BUFFER_MODE_GROUP_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT0_GROUP_ANT0_OFFSET);

		if (BT0_GROUP_ANT1_OFFSET + BUFFER_MODE_GROUP_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_ant1_grp_boundary, &bdev->setting_file[BT0_GROUP_ANT1_OFFSET],
					BUFFER_MODE_GROUP_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT0_GROUP_ANT1_OFFSET);

		if (BT1_GROUP_ANT0_OFFSET + BUFFER_MODE_GROUP_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt1_ant0_grp_boundary, &bdev->setting_file[BT1_GROUP_ANT0_OFFSET],
					BUFFER_MODE_GROUP_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT1_GROUP_ANT0_OFFSET);

		if (BT1_GROUP_ANT1_OFFSET + BUFFER_MODE_GROUP_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt1_ant1_grp_boundary, &bdev->setting_file[BT1_GROUP_ANT1_OFFSET],
					BUFFER_MODE_GROUP_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT1_GROUP_ANT1_OFFSET);

		if (BT0_CAL_ANT0_OFFSET + BUFFER_MODE_CAL_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_ant0_pwr_offset, &bdev->setting_file[BT0_CAL_ANT0_OFFSET],
					BUFFER_MODE_CAL_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT0_CAL_ANT0_OFFSET);

		if (BT0_CAL_ANT1_OFFSET + BUFFER_MODE_CAL_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt0_ant1_pwr_offset, &bdev->setting_file[BT0_CAL_ANT1_OFFSET],
					BUFFER_MODE_CAL_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT0_CAL_ANT1_OFFSET);

		if (BT1_CAL_ANT0_OFFSET + BUFFER_MODE_CAL_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt1_ant0_pwr_offset, &bdev->setting_file[BT1_CAL_ANT0_OFFSET],
					BUFFER_MODE_CAL_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT1_CAL_ANT0_OFFSET);

		if (BT1_CAL_ANT1_OFFSET + BUFFER_MODE_CAL_LENGTH <= code_len)
			memcpy(btmtk_buffer_mode.bt1_ant1_pwr_offset, &bdev->setting_file[BT1_CAL_ANT1_OFFSET],
					BUFFER_MODE_CAL_LENGTH);
		else
			BTMTK_ERR("%s: error address, %x", __func__, BT1_CAL_ANT1_OFFSET);

		if (is_mt7961(bdev->chip_id) && bdev->flavor) {
			if (BT0_COMPENSATION_OFFSET + BUFFER_MODE_COMPENSATION_LENGTH <= code_len)
				memcpy(btmtk_buffer_mode.bt0_ant0_compensation, &bdev->setting_file[BT0_COMPENSATION_OFFSET],
						BUFFER_MODE_COMPENSATION_LENGTH);
			else
				BTMTK_ERR("%s: error address, %x", __func__, BT0_COMPENSATION_OFFSET);
		}
	}
	bdev->buffer_mode = &btmtk_buffer_mode;
}

