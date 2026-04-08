/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_7902.h"
#include "btmtk_chip_common.h"

static int btmtk_load_fw_patch(struct btmtk_dev *bdev)
{
	int err = 0;
#if CFG_SUPPORT_BT_DL_ZB_PATCH
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	unsigned int zb_enbale = 0;
#endif

	BTMTK_INFO("%s: enter", __func__);

	err = btmtk_load_rom_patch_connac3(bdev, BT_DOWNLOAD);
	if (err < 0) {
		BTMTK_ERR("%s: btmtk_load_rom_patch_connac3 bt patch failed!", __func__);
		return err;
	}

#if CFG_SUPPORT_BT_DL_ZB_PATCH
		if (is_mt7902(bdev->chip_id)) {
			err = bmain_info->hif_hook.reg_read(bdev, ZB_ENABLE, &zb_enbale);
			BTMTK_INFO("%s: ZB_ENABLE is %d", __func__, zb_enbale);
			if (err < 0) {
				BTMTK_ERR("read ZB enbale failed");
			} else if ((zb_enbale & (1 << 0)) == 0) {
				err = btmtk_load_rom_patch_connac3(bdev, ZB_DOWNLOAD);
				if (err < 0) {
					BTMTK_WARN("%s: btmtk_load_rom_patch_79xx ZB patch failed!", __func__);
				}
			}
		}
#endif
#if CFG_SUPPORT_BT_DL_WIFI_PATCH
	err = btmtk_load_rom_patch_connac3(bdev, WIFI_DOWNLOAD);
	if (err < 0) {
		BTMTK_WARN("%s: btmtk_load_rom_patch_connac3 wifi patch failed!", __func__);
	}
#endif

	/* Do not care about the failure of ZB and wifi, directly return 0 */
	return 0;
}

static int btmtk_get_fw_info(struct btmtk_dev *bdev)
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

	bdev->dualBT = 0;

	/* Bin filename format : "BT_RAM_CODE_MT%04x_%x_%x_hdr.bin"
	 *	$$$$ : chip id
	 *	% : fw version & 0xFF + 1 (in HEX)
	 */

	/* 7902 can't use the same rule to recognize */
	bdev->flavor = 0;
	bdev->proj = 0;

	BTMTK_INFO("%s: flavor1 = 0x%x", __func__, bdev->flavor);
	return ret;
}

#ifdef CHIP_IF_SDIO
#if 0
int btmtk_sdio_read_conn_infra_pc_7902(struct sdio_func *func, u32 *val)
{
	btmtk_sdio_writel(0x34, 0x04, func);
	btmtk_sdio_writel(0x3C, 0x9F1E0000, func);
	btmtk_sdio_readl(0x38, val, func);

	return 0;
}
#endif
#endif

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


	bmain_info->hif_hook.read_uhw_register(bdev, BT_BACKDOOR_RET_7902, &val);
	BTMTK_INFO("%s: read 1 BT_BACKDOOR_RET_7902 CR : 0x%08x", __func__, val);
	val |= 0x30000;
	BTMTK_INFO("%s: write 1 BT_BACKDOOR_RET_7902 CR : 0x%08x", __func__, val);
	bmain_info->hif_hook.write_uhw_register(bdev, BT_BACKDOOR_RET_7902,val);

	msleep(20);
	bmain_info->hif_hook.read_uhw_register(bdev, BT_BACKDOOR_RET_7902, &val);
	BTMTK_INFO("%s: read 2 BT_BACKDOOR_RET_7902 CR : 0x%08x", __func__, val);
	val &= 0xFFFCFFFF;
	BTMTK_INFO("%s: write 2 BT_BACKDOOR_RET_7902 CR : 0x%08x", __func__, val);
	bmain_info->hif_hook.write_uhw_register(bdev, BT_BACKDOOR_RET_7902,val);

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

int btmtk_cif_chip_7902_register(void)
{
	int retval = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s", __func__);

	bmain_info->hif_hook_chip.load_patch = btmtk_load_fw_patch;
	bmain_info->hif_hook_chip.get_fw_info = btmtk_get_fw_info;
#ifdef CHIP_IF_SDIO
	//bmain_info->hif_hook_chip.bt_conn_infra_pc = btmtk_sdio_read_conn_infra_pc_7902;
#endif

	bmain_info->hif_hook_chip.dl_delay_time = PATCH_DOWNLOAD_PHASE3_SECURE_BOOT_DELAY_TIME;
	bmain_info->hif_hook_chip.bt_subsys_reset = btmtk_subsys_reset;

	return retval;
}
