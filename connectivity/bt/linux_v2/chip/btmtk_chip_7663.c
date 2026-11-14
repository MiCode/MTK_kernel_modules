/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_7663.h"
#include "btmtk_chip_common.h"

static int btmtk_get_fw_info(struct btmtk_dev *bdev)
{
	int ret = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	ret = bdev->main_info.hif_hook.reg_read(bdev, FLAVOR, &bdev->flavor);
	if (ret < 0) {
		BTMTK_ERR_DEV(bdev, "read flavor id failed");
		ret = -EIO;
		return ret;
	}
	ret = bdev->main_info.hif_hook.reg_read(bdev, FW_VERSION, &bdev->fw_version);
	if (ret < 0) {
		BTMTK_ERR_DEV(bdev, "read fw version failed");
		ret = -EIO;
		return ret;
	}

	BTMTK_INFO_DEV(bdev, "%s: Chip ID = 0x%x", __func__, bdev->chip_id);
	BTMTK_INFO_DEV(bdev, "%s: flavor = 0x%x", __func__, bdev->flavor);
	BTMTK_INFO_DEV(bdev, "%s: FW Ver = 0x%x", __func__, bdev->fw_version);

	memset(bdev->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
	if ((bdev->fw_version & 0xff) == 0xff) {
		BTMTK_ERR_DEV(bdev, "%s: failed, wrong FW version : 0x%x !", __func__, bdev->fw_version);
		ret = -1;
		return ret;
	}

	return ret;
}

static int btmtk_usb_check_need_load_rom_patch(struct btmtk_dev *bdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x17, 0x01, 0x00, 0x01 };
	u8 event[] = { 0x04, 0xE4, 0x05, 0x02, 0x17, 0x01, 0x00, /* 0x02 */ };	/* event[7] is key */
	int ret = -1;

	ret = btmtk_main_send_cmd(bdev, cmd, 9,
			event, 7, DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR_DEV(bdev, "%s: send wmd dl cmd failed, terminate!", __func__);
		return PATCH_ERR;
	}

	if (bdev->recv_evt_len >= 8) {
		return bdev->io_buf[7];
	}

	return PATCH_ERR;
}

static int btmtk_usb_send_wmt_cfg(struct btmtk_dev *bdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x03, 0x01};
	int ret = 0;
	int index = 0;

	for (index = 0; index < WMT_CMD_COUNT; index++) {
		if (bdev->bt_cfg.wmt_cmd[index].content && bdev->bt_cfg.wmt_cmd[index].length) {
			ret = btmtk_main_send_cmd(bdev,
				bdev->bt_cfg.wmt_cmd[index].content,
				bdev->bt_cfg.wmt_cmd[index].length,
				NULL, 0, 100, 10, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_ERR_DEV(bdev, "%s: Send wmt cmd failed(%d)! Index: %d", __func__, ret, index);
				return ret;
			}
		}
	}

	if (!bdev->bt_cfg.wmt_cmd[0].content) {
		BTMTK_INFO_DEV(bdev, "%s: use default wmt cmd", __func__);
		ret = btmtk_main_send_cmd(bdev, cmd, sizeof(cmd),
			NULL, 0, 100, 10, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

		if (ret < 0) {
			BTMTK_ERR_DEV(bdev, "%s: Send default wmt cmd failed(%d)!", __func__, ret);
			return ret;
		}
	}

	return ret;
}
static int btmtk_usb_send_wmt_reset_cmd(struct btmtk_dev *bdev)
{
	u8 cmd[] = {  0x01, 0x6F, 0xFC, 0x05, 0x01, 0x07, 0x01, 0x00, 0x04 };
	u8 event[] = {  0x04, 0xE4, 0x05, 0x02, 0x07, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	BTMTK_INFO_DEV(bdev, "%s", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, sizeof(cmd), event, sizeof(event), 20, 5, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR_DEV(bdev, "%s: Check reset wmt result: NG", __func__);
	} else {
		BTMTK_INFO_DEV(bdev, "%s: Check reset wmt result: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_load_partial_rom_patch(struct btmtk_dev *bdev, u8 *rom_patch, u32 patch_len, int offset)
{
	s32 sent_len;
	int ret = 0;
	u32 cur_len = 0;
	int first_block = 1;
	u8 phase;
	u8 *pos;
	u8 event[] = { 0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00 };

	BTMTK_INFO_DEV(bdev, "%s: begin", __func__);

	pos = kmalloc(UPLOAD_PATCH_UNIT, GFP_ATOMIC);
	if (!pos) {
		BTMTK_ERR_DEV(bdev, "%s: alloc memory failed", __func__);
		ret = -1;
		goto exit;
	}

	BTMTK_INFO_DEV(bdev, "%s: patch_len = %d", __func__, patch_len);

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - 10;
		//int status = -1;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x02;
			pos[1] = 0x6F;
			pos[2] = 0xFC;
			pos[3] = (sent_len + 5) & 0xFF;
			pos[4] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[5] = 0x01;
			pos[6] = 0x01;
			pos[7] = (sent_len + 1) & 0xFF;
			pos[8] = ((sent_len + 1) >> 8) & 0xFF;

			pos[9] = phase;

			memcpy(&pos[10], rom_patch + offset + cur_len, sent_len);

			BTMTK_DBG_DEV(bdev, "%s: sent_len = %d, cur_len = %d, phase = %d", __func__, sent_len,
					cur_len, phase);
			ret = btmtk_main_send_cmd(bdev, pos, sent_len + PATCH_HEADER_SIZE,
					event, 8, 1, 5, BTMTK_TX_ACL_FROM_DRV, CMD_NO_NEED_FILTER);

			cur_len += sent_len;
		} else {
			break;
		}
	}
	kfree(pos);
exit:
	BTMTK_INFO_DEV(bdev, "%s end", __func__);
	return ret;
}

static int btmtk_load_fw_patch(struct btmtk_dev *bdev)
{
	u8 *pos = NULL;
	u8 *rom_patch = NULL;
	u32 patch_len = 0;
	int ret = 0;
	int patch_status = 0;
	int retry = 20;
	unsigned int rom_patch_len = 0;
	char *tmp_str;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s: enter", __func__);

	(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
			"mt7663_patch_e2_hdr.bin");

	btmtk_load_code_from_bin(bdev, &rom_patch, bdev->rom_patch_bin_file_name, NULL,
							&rom_patch_len, 10);
	if (!rom_patch) {
		BTMTK_ERR_DEV(bdev, "%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				__func__, bdev->rom_patch_bin_file_name, bdev->rom_patch_bin_file_name);
		ret = -1;
		goto err;
	}

	memcpy(bdev->main_info.fw_version_str, rom_patch, 14);
	pos = kmalloc(UPLOAD_PATCH_UNIT, GFP_ATOMIC);
	if (!pos) {
		BTMTK_ERR_DEV(bdev, "%s: alloc memory failed", __func__);
		ret = -1;
		goto err;
	}

	do {
		patch_status = btmtk_usb_check_need_load_rom_patch(bdev);
		BTMTK_INFO_DEV(bdev, "%s: patch_status %d", __func__, patch_status);

		if ((patch_status > PATCH_NEED_DOWNLOAD_7663) || (patch_status == PATCH_ERR_7663)) {
			BTMTK_ERR_DEV(bdev, "%s: patch_status error", __func__);
			ret = -1;
			goto err;
		} else if (patch_status == PATCH_READY_7663) {
			BTMTK_INFO_DEV(bdev, "%s: no need to load rom patch", __func__);
			goto patch_end;
		} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER_7663) {
			msleep(100);
			retry--;
		} else if (patch_status == PATCH_NEED_DOWNLOAD_7663) {
			ret = btmtk_usb_send_wmt_cfg(bdev);
			if (ret < 0) {
				BTMTK_ERR_DEV(bdev, "%s: send wmt cmd failed(%d)", __func__, ret);
				goto err;
			}

			break;  /* Download ROM patch directly */
		}
	} while (retry > 0);

	if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER_7663) {
		BTMTK_WARN_DEV(bdev, "%s: Hold by another fun more than 2 seconds", __func__);
		ret = -1;
		goto err;
	}

	tmp_str = rom_patch + 16;
	BTMTK_INFO_DEV(bdev, "%s: platform = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = rom_patch + 20;
	BTMTK_INFO_DEV(bdev, "%s: HW/SW version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = rom_patch + 24;

	BTMTK_INFO_DEV(bdev, "%s: loading ILM rom patch...", __func__);
	patch_len = rom_patch_len - PATCH_INFO_SIZE;
	ret = btmtk_usb_load_partial_rom_patch(bdev, rom_patch, patch_len, PATCH_INFO_SIZE);
	if (ret < 0)
		goto patch_end;
	BTMTK_INFO_DEV(bdev, "%s: loading ILM rom patch... Done", __func__);

	/* CHIP_RESET, ROM patch would be reactivated.
	 * Currently, wmt reset is only for ILM rom patch, and there are also
	 * some preparations need to be done in FW for loading sysram3 patch...
	 */
	ret = btmtk_usb_send_wmt_reset_cmd(bdev);

patch_end:
	BTMTK_INFO_DEV(bdev, "btmtk_usb_load_rom_patch end");

err:
	if (rom_patch)
		vfree(rom_patch);
	if (pos)
		kfree(pos);

	return ret;
}


int btmtk_cif_chip_7663_register(struct btmtk_dev *bdev)
{
	int retval = 0;

	if (bdev == NULL) {
		BTMTK_ERR("Error: bdev is NULL\n");
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s", __func__);

	bdev->main_info.hif_hook_chip.get_fw_info  = btmtk_get_fw_info;
	bdev->main_info.hif_hook_chip.bt_subsys_reset = NULL;
	bdev->main_info.hif_hook_chip.load_patch = btmtk_load_fw_patch;
	bdev->main_info.hif_hook_chip.bt_set_pinmux = NULL;

	return retval;
}
