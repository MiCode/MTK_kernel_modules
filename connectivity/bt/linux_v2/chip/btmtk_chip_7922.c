/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_7922.h"
#include "btmtk_chip_common.h"


static int btmtk_subsys_reset(struct btmtk_dev *bdev)
{
	int val = 0, retry = 10;
	u32 mcu_init_done = MCU_BT0_INIT_DONE;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	/* For reset */
	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_EP_RST_OPT, EP_RST_IN_OUT_OPT);

	/* Write Reset CR to 1 */
	bdev->main_info.hif_hook.read_uhw_register(bdev, BT_SUBSYS_RST, &val);
	BTMTK_INFO_DEV(bdev, "%s: read Reset CR : 0x%08x", __func__, val);
	val |= (1 << 0);
	BTMTK_INFO_DEV(bdev, "%s: write 1 to Reset CR : 0x%08x", __func__, val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_SUBSYS_RST, val);

	bdev->main_info.hif_hook.write_uhw_register(bdev, UDMA_INT_STA_BT, 0x000000FF);
	bdev->main_info.hif_hook.read_uhw_register(bdev, UDMA_INT_STA_BT, &val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, UDMA_INT_STA_BT1, 0x000000FF);
	bdev->main_info.hif_hook.read_uhw_register(bdev, UDMA_INT_STA_BT1, &val);

	/* Write Reset CR to 0 */
	bdev->main_info.hif_hook.read_uhw_register(bdev, BT_SUBSYS_RST, &val);
	BTMTK_INFO_DEV(bdev, "%s: read Reset CR : 0x%08x", __func__, val);
	val &= ~(1 << 0);
	BTMTK_INFO_DEV(bdev, "%s: write 1 to Reset CR : 0x%08x", __func__, val);
	bdev->main_info.hif_hook.write_uhw_register(bdev, BT_SUBSYS_RST, val);

	/* Read Reset CR */
	bdev->main_info.hif_hook.read_uhw_register(bdev, BT_SUBSYS_RST, &val);

	if (bdev->dualBT)
		mcu_init_done |= MCU_BT1_INIT_DONE;

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

int btmtk_cif_chip_7922_register(struct btmtk_dev *bdev)
{
	int retval = 0;

	if (bdev == NULL) {
		BTMTK_ERR("Error: bdev is NULL\n");
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s", __func__);

	bdev->main_info.hif_hook_chip.dl_delay_time = PATCH_DOWNLOAD_PHASE3_SECURE_BOOT_DELAY_TIME;
	bdev->main_info.hif_hook_chip.bt_subsys_reset = btmtk_subsys_reset;

	return retval;
}
