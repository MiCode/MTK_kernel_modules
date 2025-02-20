/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_7961.h"
#include "btmtk_chip_common.h"

static int btmtk_get_fw_info_7961(struct btmtk_dev *bdev)
{
	int ret = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	ret = bmain_info->hif_hook.reg_read(bdev, FLAVOR, &bdev->flavor);
	if (ret < 0) {
		BTMTK_ERR("read flavor id failed");
		ret = -EIO;
		return ret;
	}
	ret = bmain_info->hif_hook.reg_read(bdev, FW_VERSION, &bdev->fw_version);
	if (ret < 0) {
		BTMTK_ERR("read fw version failed");
		ret = -EIO;
		return ret;
	}

	BTMTK_INFO("%s: Chip ID = 0x%x", __func__, bdev->chip_id);
	BTMTK_INFO("%s: flavor = 0x%x", __func__, bdev->flavor);
	BTMTK_INFO("%s: FW Ver = 0x%x", __func__, bdev->fw_version);

	memset(bdev->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
	if ((bdev->fw_version & 0xff) == 0xff) {
		BTMTK_ERR("%s: failed, wrong FW version : 0x%x !", __func__, bdev->fw_version);
		ret = -1;
		return ret;
	}

	if (bdev->flavor & DUAL_BT_FLAG)
		bdev->dualBT = 1;
	else
		bdev->dualBT = 0;


	/* Bin filename format : "BT_RAM_CODE_MT%04x_%x_%x_hdr.bin"
	 *	$$$$ : chip id
	 *	% : fw version & 0xFF + 1 (in HEX)
	 */
	bdev->flavor = (bdev->flavor & 0x00000080) >> 7;
	bdev->proj = 0;

	BTMTK_INFO("%s: flavor1 = 0x%x", __func__, bdev->flavor);
	return ret;
}

static int btmtk_read_pin_mux_setting_7961(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, const uint8_t *event, const int event_len, u32 *value)
{
	int ret = 0;

	BTMTK_INFO("%s enter", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, cmd_len,
			event, event_len, 0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	*value = (bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 1] << 24) +
			(bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 2] << 16) +
			(bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 3] << 8) +
			bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 4];
	BTMTK_INFO("%s, value=0x%08x", __func__, *value);
	return ret;
}

static int btmtk_write_pin_mux_setting_7961(struct btmtk_dev *bdev, uint8_t *cmd,
		int cmd_len, const uint8_t *event, const int event_len, u32 value)
{
	int ret = 0;

	BTMTK_INFO("%s begin, value = 0x%08x", __func__, value);

	cmd[cmd_len - 4] = (value & 0x000000FF);
	cmd[cmd_len - 3] = ((value & 0x0000FF00) >> 8);
	cmd[cmd_len - 2] = ((value & 0x00FF0000) >> 16);
	cmd[cmd_len - 1] = ((value & 0xFF000000) >> 24);

	ret = btmtk_main_send_cmd(bdev, cmd, cmd_len,
			event, event_len, 0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	BTMTK_INFO("%s exit", __func__);
	return ret;
}


static int btmtk_set_audio_pin_mux_7961(struct btmtk_dev *bdev)
{
	int ret = 0;
	unsigned int i = 0;
	u32 pinmux = 0;
	struct data_struct read_pinmux_cmd = {0}, read_pinmux_event = {0};
	struct data_struct write_pinmux_cmd = {0}, write_pinmux_event = {0};
	char pin_addr[PINMUX_REG_NUM] = {0x50, 0x54};

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_PINMUX_CMD, read_pinmux_cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_PINMUX_EVT, read_pinmux_event);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, WRITE_PINMUX_CMD, write_pinmux_cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, WRITE_PINMUX_EVT, write_pinmux_event);

	for (i = 0; i < PINMUX_REG_NUM; i++) {
		pinmux = 0;
		read_pinmux_cmd.content[AUDIO_PINMUX_SETTING_OFFSET] = pin_addr[i];
		write_pinmux_cmd.content[AUDIO_PINMUX_SETTING_OFFSET] = pin_addr[i];
		ret = btmtk_read_pin_mux_setting_7961(bdev, read_pinmux_cmd.content, read_pinmux_cmd.len,
			read_pinmux_event.content, read_pinmux_event.len, &pinmux);
		if (ret) {
			BTMTK_ERR("%s, btmtk_read_pin_mux_setting error(%d)", __func__, ret);
			goto exit;
		}
		if (i == 0) {
			pinmux &= 0x00FFFFFF;
			pinmux |= 0x11000000;
		} else {
			pinmux &= 0xFFFFF0F0;
			pinmux |= 0x00000101;
		}

		ret = btmtk_write_pin_mux_setting_7961(bdev, write_pinmux_cmd.content, write_pinmux_cmd.len,
			write_pinmux_event.content, write_pinmux_event.len, pinmux);

		if (ret) {
			BTMTK_ERR("%s, btmtk_write_pin_mux_setting error(%d)", __func__, ret);
			goto exit;
		}

		pinmux = 0;
		ret = btmtk_read_pin_mux_setting_7961(bdev, read_pinmux_cmd.content, read_pinmux_cmd.len,
			read_pinmux_event.content, read_pinmux_event.len, &pinmux);
		if (ret) {
			BTMTK_ERR("%s, btmtk_read_pin_mux_setting error(%d)", __func__, ret);
			goto exit;
		}
		BTMTK_INFO("%s, confirm pinmux register 0x%02x pinmux 0x%08x", __func__,
				write_pinmux_cmd.content[4], pinmux);
	}

exit:
	return ret;
}

static int btmtk_subsys_reset(struct btmtk_dev *bdev)
{
	int val = 0, retry = 10;
	u32 mcu_init_done = MCU_BT0_INIT_DONE;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	/* For reset */
	bmain_info->hif_hook.write_uhw_register(bdev, BT_EP_RST_OPT, EP_RST_IN_OUT_OPT);

	/* Write Reset CR to 1 */
	bmain_info->hif_hook.read_uhw_register(bdev, BT_SUBSYS_RST, &val);
	BTMTK_INFO("%s: read Reset CR : 0x%08x", __func__, val);
	val |= (1 << 0);
	BTMTK_INFO("%s: write 1 to Reset CR : 0x%08x", __func__, val);
	bmain_info->hif_hook.write_uhw_register(bdev, BT_SUBSYS_RST, val);

	bmain_info->hif_hook.write_uhw_register(bdev, UDMA_INT_STA_BT, 0x000000FF);
	bmain_info->hif_hook.read_uhw_register(bdev, UDMA_INT_STA_BT, &val);
	bmain_info->hif_hook.write_uhw_register(bdev, UDMA_INT_STA_BT1, 0x000000FF);
	bmain_info->hif_hook.read_uhw_register(bdev, UDMA_INT_STA_BT1, &val);

	bmain_info->hif_hook.read_uhw_register(bdev, BT_BACKDOOR_RET_7961, &val);
	BTMTK_INFO("%s: read 1 BT_BACKDOOR_RET_7961 CR : 0x%08x", __func__, val);
	val |= 0x30000;
	BTMTK_INFO("%s: write 1 BT_BACKDOOR_RET_7961 CR : 0x%08x", __func__, val);
	bmain_info->hif_hook.write_uhw_register(bdev, BT_BACKDOOR_RET_7961,val);

	msleep(20);
	bmain_info->hif_hook.read_uhw_register(bdev, BT_BACKDOOR_RET_7961, &val);
	BTMTK_INFO("%s: read 2 BT_BACKDOOR_RET_7961 CR : 0x%08x", __func__, val);
	val &= 0xFFFCFFFF;
	BTMTK_INFO("%s: write 2 BT_BACKDOOR_RET_7961 CR : 0x%08x", __func__, val);
	bmain_info->hif_hook.write_uhw_register(bdev, BT_BACKDOOR_RET_7961,val);

	/* Write Reset CR to 0 */
	bmain_info->hif_hook.read_uhw_register(bdev, BT_SUBSYS_RST, &val);
	BTMTK_INFO("%s: read Reset CR : 0x%08x", __func__, val);
	val &= ~(1 << 0);
	BTMTK_INFO("%s: write 1 to Reset CR : 0x%08x", __func__, val);
	bmain_info->hif_hook.write_uhw_register(bdev, BT_SUBSYS_RST, val);

	/* Read Reset CR */
	bmain_info->hif_hook.read_uhw_register(bdev, BT_SUBSYS_RST, &val);

	if (bdev->dualBT)
		mcu_init_done |= MCU_BT1_INIT_DONE;

	do {
		/* polling re-init CR */
		bmain_info->hif_hook.read_uhw_register(bdev, BT_MISC, &val);
		BTMTK_INFO("%s: reg=%x, value=0x%08x", __func__, BT_MISC, val);
		if (val == 0xffffffff) {
			/* read init CR failed */
			BTMTK_INFO("%s: read init CR failed, retry = %d", __func__, retry);
		} else if ((val & mcu_init_done) == mcu_init_done) {
			/* L0.5 reset done */
			BTMTK_INFO("%s: Do L0.5 reset successfully.", __func__);
			goto Finish;
		} else {
			BTMTK_INFO("%s: polling MCU-init done CR", __func__);
		}
		msleep(100);
	} while (retry-- > 0);

	/* L0.5 reset failed, do whole chip reset */
	return -1;

Finish:
	return 0;
}


int btmtk_cif_chip_7961_register(void)
{
	int retval = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s", __func__);

	bmain_info->hif_hook_chip.bt_set_pinmux = btmtk_set_audio_pin_mux_7961;
	bmain_info->hif_hook_chip.get_fw_info = btmtk_get_fw_info_7961;
	bmain_info->hif_hook_chip.patched = 1;
	bmain_info->hif_hook_chip.bt_subsys_reset = btmtk_subsys_reset;

	return retval;
}
