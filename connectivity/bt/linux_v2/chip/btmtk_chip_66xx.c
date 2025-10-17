/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_66xx.h"
#include "btmtk_chip_common.h"

static int btmtk_load_fw_patch(struct btmtk_dev *bdev)
{
	int err = 0;

	BTMTK_INFO("%s: enter", __func__);

	err = btmtk_load_rom_patch_connac3(bdev, BT_DOWNLOAD);
	if (err < 0) {
		BTMTK_ERR("%s: btmtk_load_rom_patch_connac3 bt patch failed!", __func__);
		return err;
	}

	return err;
}

static int btmtk_cif_rx_packet_handler(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	BTMTK_DBG("%s start", __func__);
#if (USE_DEVICE_NODE == 1)
	return rx_skb_enqueue(skb);
#else
	return 0;
#endif
}

static void btmtk_recv_error_handler(struct hci_dev *hdev,
		const u8 *buf, u32 len, const u8 *dbg_buf, u32 dbg_len)
{
	if (*buf == 0xFF || *buf == 0x00)
		BTMTK_INFO("%s: skip 0x%02X pkt", __func__, *buf);
	btmtk_set_sleep(hdev, FALSE);
}

static int btmtk_cif_bt_setup(struct hci_dev *hdev)
{
	int ret = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s", __func__);

	ret = bmain_info->hif_hook.open(hdev);
	if (ret)
		BTMTK_ERR("%s: fail", __func__);
	return ret;
}

static int btmtk_cif_bt_flush(struct hci_dev *hdev)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct btmtk_dev *bdev =  hci_get_drvdata(hdev);

	BTMTK_INFO("%s", __func__);

	return bmain_info->hif_hook.flush(bdev);
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

int btmtk_cif_chip_66xx_register(void)
{
	int retval = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s", __func__);

	bmain_info->hif_hook_chip.get_fw_info = NULL;
	bmain_info->hif_hook_chip.load_patch = btmtk_load_fw_patch;
	bmain_info->hif_hook_chip.err_handler = btmtk_recv_error_handler;
	bmain_info->hif_hook_chip.rx_handler = btmtk_cif_rx_packet_handler;
	bmain_info->hif_hook_chip.bt_setup_handler = btmtk_cif_bt_setup;
	bmain_info->hif_hook_chip.bt_flush_handler = btmtk_cif_bt_flush;
	bmain_info->hif_hook_chip.bt_check_power_status = NULL;
	bmain_info->hif_hook_chip.bt_set_pinmux = NULL;
	bmain_info->hif_hook_chip.dispatch_fwlog = btmtk_dispatch_fwlog;
	bmain_info->hif_hook_chip.support_woble = 0;
	bmain_info->hif_hook_chip.bt_subsys_reset = btmtk_subsys_reset;

	return retval;
}
