/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_connac3.h"
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

	if (is_mt7925(bdev->chip_id))
		bdev->dualBT = 1;
	else
		bdev->dualBT = 0;

	/* Bin filename format : "BT_RAM_CODE_MT%04x_%x_%x_hdr.bin"
	 *	$$$$ : chip id
	 *	% : fw version & 0xFF + 1 (in HEX)
	 */

	bdev->proj = (bdev->flavor & 0x00000001);
	bdev->flavor = (bdev->flavor & 0x00000080) >> 7;

	BTMTK_INFO_DEV(bdev, "%s: proj = 0x%x", __func__, bdev->proj);
	BTMTK_INFO_DEV(bdev, "%s: flavor1 = 0x%x", __func__, bdev->flavor);
	return ret;
}

static int btmtk_load_fw_patch(struct btmtk_dev *bdev)
{
	int err = 0;
	int retry = 20;
	int patch_status = 0;

	BTMTK_INFO_DEV(bdev, "%s: enter", __func__);

	err = btmtk_load_rom_patch_connac3(bdev, BT_DOWNLOAD);
	if (err < 0) {
		BTMTK_ERR_DEV(bdev, "%s: btmtk_load_rom_patch_connac3 bt patch failed!", __func__);
		return err;
	}

	UNUSED(retry);
	UNUSED(patch_status);
#if CFG_SUPPORT_BT_DL_WIFI_PATCH
	retry = 20;
	do {
		err = btmtk_send_cmd_to_fw(bdev,
				WIFI_PATCH_QUERY_CMD, WIFI_PATCH_QUERY_EVT,
				DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
		if (err < 0) {
			BTMTK_ERR_DEV(bdev, "%s: send wifi patch query failed!", __func__);
			return PATCH_ERR;
		}

		if (bdev->recv_evt_len >= WIFI_PATCH_QUERY_EVT_LEN) {
			patch_status = bdev->io_buf[WIFI_PATCH_QUERY_EVT_LEN];
			BTMTK_INFO_RAW(bdev, bdev->io_buf, bdev->recv_evt_len, "%s WIFI patch OK: EVT:", __func__);
		}

		BTMTK_INFO_DEV(bdev, "%s: patch_status %d", __func__, patch_status);

		if (patch_status > PATCH_READY || patch_status == PATCH_ERR) {
			BTMTK_ERR_DEV(bdev, "%s: patch_status error", __func__);
			err = 0;
			return err;
		} else if (patch_status == PATCH_READY) {
			BTMTK_INFO_DEV(bdev, "%s: no need to load rom patch", __func__);
			err = 0;
			return err;
		} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
			msleep(100);
			retry--;
		} else if (patch_status == PATCH_NEED_DOWNLOAD) {
			break;	/* Download ROM patch directly */
		}
	} while (retry > 0);

	if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
		BTMTK_WARN_DEV(bdev, "%s: Hold by another fun more than 2 seconds", __func__);
		err = 0;
		return err;
	}

	err = btmtk_load_rom_patch_connac3(bdev, WIFI_DOWNLOAD);
	if (err < 0) {
		BTMTK_WARN_DEV(bdev, "%s: btmtk_load_rom_patch_connac3 wifi patch failed!", __func__);
		err = 0;
		return err;
	}

	err = btmtk_send_cmd_to_fw(bdev,
			WIFI_PATCH_ENABLE_CMD, WIFI_PATCH_ENABLE_EVT,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (err < 0) {
		BTMTK_ERR_DEV(bdev, "%s: send wifi patch enable failed!", __func__);
		return PATCH_ERR;
	}
	BTMTK_INFO_DEV(bdev, "%s: wifi patch download end", __func__);
#endif
	return err;
}

static int btmtk_subsys_reset(struct btmtk_dev *bdev)
{
	int val = 0, retry = 10;
	u32 mcu_init_done = MCU_BT0_INIT_DONE;

	bdev->main_info.hif_hook.read_uhw_register(bdev, BT_RESET_REG_CONNAC3, &val);
	BTMTK_INFO_DEV(bdev, "%s:read Reset CR : 0x%08x", __func__, val);
	val |= (1 << 5);
	BTMTK_INFO_DEV(bdev, "%s:write to Reset CR : 0x%08x", __func__, val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_RESET_REG_CONNAC3, val);
	bdev->main_info.hif_hook.read_uhw_register(bdev, BT_RESET_REG_CONNAC3, &val);
	BTMTK_INFO_DEV(bdev, "%s:read Reset CR : 0x%08x", __func__, val);
	val &= 0xFFFF00FF;
	val |= (1 << 13);
	BTMTK_INFO_DEV(bdev, "%s:write to Reset CR : 0x%08x", __func__, val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_RESET_REG_CONNAC3, val);

	/* For reset */
	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_EP_RST_OPT, EP_RST_IN_OUT_OPT);
	/* Write Reset CR to 1 */
	bdev->main_info.hif_hook.read_uhw_register(bdev, BT_RESET_REG_CONNAC3, &val);
	BTMTK_INFO_DEV(bdev, "%s: read Reset CR : 0x%08x", __func__, val);
	val |= (1 << 0);
	BTMTK_INFO_DEV(bdev, "%s: write 1 to Reset CR : 0x%08x", __func__, val);

	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_RESET_REG_CONNAC3, val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, UDMA_INT_STA_BT, 0x000000FF);
	bdev->main_info.hif_hook.read_uhw_register(bdev, UDMA_INT_STA_BT, &val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, UDMA_INT_STA_BT1, 0x000000FF);
	bdev->main_info.hif_hook.read_uhw_register(bdev, UDMA_INT_STA_BT1, &val);
	msleep(100);

	//if (bdev->dualBT)
	//	mcu_init_done |= MCU_BT1_INIT_DONE;
	do {
		/* polling re-init CR */
		bdev->main_info.hif_hook.read_uhw_register(bdev, BT_MISC, &val);
		BTMTK_INFO_DEV(bdev, "%s: reg=%x, value=0x%08x", __func__, BT_MISC, val);
		if (val == 0xffffffff) {
			/* read init CR failed */
			BTMTK_INFO_DEV(bdev, "%s: read init CR failed, retry = %d", __func__, retry);
		} else if ((val & mcu_init_done) == mcu_init_done) {
			/* L0.5 reset done */
			BTMTK_INFO_DEV(bdev, "%s: Do L0.5 reset successfully.", __func__);
			goto Finish;
		} else {
			BTMTK_INFO_DEV(bdev, "%s: polling MCU-init done CR", __func__);
		}
		msleep(100);
	} while (retry-- > 0);
	/* L0.5 reset failed, do whole chip reset */
	return -1;
Finish:
	return 0;
}

int btmtk_cif_chip_commonv3_register(struct btmtk_dev *bdev)
{
	int retval = 0;

	if (bdev == NULL) {
		BTMTK_ERR("Error: bdev is NULL\n");
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s", __func__);

	bdev->main_info.hif_hook_chip.get_fw_info = btmtk_get_fw_info;
	bdev->main_info.hif_hook_chip.load_patch = btmtk_load_fw_patch;
	bdev->main_info.hif_hook_chip.bt_set_pinmux = NULL;
	bdev->main_info.hif_hook_chip.bt_subsys_reset = btmtk_subsys_reset;

	return retval;
}
