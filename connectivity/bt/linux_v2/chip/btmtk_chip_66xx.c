/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_66xx.h"
#include "btmtk_chip_common.h"

static int btmtk_load_fw_patch(struct btmtk_dev *bdev)
{
	int err = 0;

	BTMTK_DBG_DEV(bdev, "%s: enter", __func__);

	err = btmtk_load_rom_patch_connac3(bdev, BT_DOWNLOAD);
	if (err < 0) {
		BTMTK_ERR_DEV(bdev, "%s: btmtk_load_rom_patch_connac3 bt patch failed!", __func__);
		return err;
	}

	return err;
}

static int btmtk_cif_rx_packet_handler(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	BTMTK_DBG("%s start", __func__);
#if (USE_DEVICE_NODE == 1)
	return rx_skb_enqueue(bdev, skb);
#else
	return 0;
#endif
}

static void btmtk_recv_error_handler(struct hci_dev *hdev,
		const u8 *buf, u32 len, const u8 *dbg_buf, u32 dbg_len)
{
	struct btmtk_dev *bdev =  hci_get_drvdata(hdev);

	if (*buf == 0xFF || *buf == 0x00)
		BTMTK_INFO_DEV(bdev, "%s: skip 0x%02X pkt", __func__, *buf);
	btmtk_set_sleep(hdev, FALSE);
}

static int btmtk_cif_bt_setup(struct hci_dev *hdev)
{
	int ret = 0;
	struct btmtk_dev *bdev =  hci_get_drvdata(hdev);

	if (bdev == NULL) {
		BTMTK_ERR("Error: bdev is NULL\n");
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s", __func__);

	ret = bdev->main_info.hif_hook.open(hdev);
	if (ret)
		BTMTK_ERR_DEV(bdev, "%s: fail", __func__);
	return ret;
}

static int btmtk_cif_bt_flush(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev =  hci_get_drvdata(hdev);

	if (bdev == NULL) {
		BTMTK_ERR("Error: bdev is NULL\n");
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s", __func__);

	return bdev->main_info.hif_hook.flush(bdev);
}

static int btmtk_subsys_reset(struct btmtk_dev *bdev)
{
	int val = 0, retry = 10;
	u32 mcu_init_done = MCU_BT0_INIT_DONE;

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

int btmtk_cif_chip_66xx_register(struct btmtk_dev *bdev)
{
	int retval = 0;

	if (bdev == NULL) {
		BTMTK_ERR("Error: bdev is NULL\n");
		return -1;
	}

	BTMTK_INFO_DEV(bdev, "%s", __func__);

	bdev->main_info.hif_hook_chip.get_fw_info = NULL;
	bdev->main_info.hif_hook_chip.load_patch = btmtk_load_fw_patch;
	bdev->main_info.hif_hook_chip.err_handler = btmtk_recv_error_handler;
	bdev->main_info.hif_hook_chip.rx_handler = btmtk_cif_rx_packet_handler;
	bdev->main_info.hif_hook_chip.bt_setup_handler = btmtk_cif_bt_setup;
	bdev->main_info.hif_hook_chip.bt_flush_handler = btmtk_cif_bt_flush;
	bdev->main_info.hif_hook_chip.bt_check_power_status = NULL;
	bdev->main_info.hif_hook_chip.bt_set_pinmux = NULL;
	bdev->main_info.hif_hook_chip.dispatch_fwlog = btmtk_dispatch_fwlog;
	bdev->main_info.hif_hook_chip.support_woble = 0;
	bdev->main_info.hif_hook_chip.bt_subsys_reset = btmtk_subsys_reset;

	return retval;
}
