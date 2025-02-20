/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_define.h"
#include "btmtk_uart_serdev.h"
#include "btmtk_main.h"

static char event_need_compare[EVENT_COMPARE_SIZE] = {0};
static char event_need_compare_len;
static char event_compare_status;

static DEFINE_MUTEX(btmtk_uart_ops_mutex);
#define UART_OPS_MUTEX_LOCK()	mutex_lock(&btmtk_uart_ops_mutex)
#define UART_OPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_uart_ops_mutex)
static struct btmtk_uart_dev g_uart_dev;

static void btmtk_uart_write_wakeup(struct serdev_device *serdev)
{
	//struct btmtk_dev *bdev = serdev_device_get_drvdata(serdev);

	/* call
	schedule_work(&bdev->tx_work); or serdev_device_write_buf(serdev, skb->data, skb->len);
	*/
}


static int btmtk_uart_receive_buf(struct serdev_device *serdev, const u8 *data,
				 size_t count)
{
	struct btmtk_dev *bdev = serdev_device_get_drvdata(serdev);
	//int err;

	/* call btmtk_recv */
	bdev->hdev->stat.byte_rx += count;

	return count;
}

static void btmtk_uart_cif_mutex_lock(struct btmtk_dev *bdev)
{
	UART_OPS_MUTEX_LOCK();
}

static void btmtk_uart_cif_mutex_unlock(struct btmtk_dev *bdev)
{
	UART_OPS_MUTEX_UNLOCK();
}

static int btmtk_uart_read_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	int ret;
	u8 cmd[READ_REGISTER_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0C,
				0x01, 0x08, 0x08, 0x00,
				0x02, 0x01, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00};

	u8 event[READ_REGISTER_EVT_HDR_LEN] = {0x04, 0xE4, 0x10, 0x02,
			0x08, 0x0C, 0x00, 0x00,
			0x00, 0x00, 0x01};

	BTMTK_INFO("%s: read cr %x", __func__, reg);

	memcpy(&cmd[MCU_ADDRESS_OFFSET_CMD], &reg, sizeof(reg));

	ret = btmtk_main_send_cmd(bdev, cmd, READ_REGISTER_CMD_LEN, event, READ_REGISTER_EVT_HDR_LEN, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	memcpy(val, bdev->io_buf + MCU_ADDRESS_OFFSET_EVT - HCI_TYPE_SIZE, sizeof(u32));
	*val = le32_to_cpu(*val);

	BTMTK_INFO("%s: reg=%x, value=0x%08x", __func__, reg, *val);

	return 0;
}

int btmtk_cif_send_calibration(struct btmtk_dev *bdev)
{
	return 0;
}

static int btmtk_cif_allocate_memory(struct btmtk_uart_dev *cif_dev)
{
	if (cif_dev->transfer_buf == NULL) {
		cif_dev->transfer_buf = kzalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
		if (!cif_dev->transfer_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->transfer_buf)", __func__);
			return -1;
		}
	}

	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

static void btmtk_cif_free_memory(struct btmtk_uart_dev *cif_dev)
{
	kfree(cif_dev->transfer_buf);
	cif_dev->transfer_buf = NULL;

	BTMTK_INFO("%s: Success", __func__);
}

static int btmtk_uart_open(struct hci_dev *hdev)
{
	//struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	//struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_INFO("%s enter!", __func__);

	return 0;
}

static int btmtk_uart_close(struct hci_dev *hdev)
{
	//struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	BTMTK_INFO("%s enter!", __func__);
	return 0;
}

int btmtk_uart_send_cmd(struct btmtk_dev *bdev, struct sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag)
{

	return 0;
}

#if 0
static int btmtk_cif_recv_evt(struct btmtk_dev *bdev)
{
	return 0;
}
#endif

int btmtk_uart_event_filter(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	const u8 read_address_event[READ_ADDRESS_EVT_HDR_LEN] = { 0x4, 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00 };

	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE &&
		skb->len >= event_need_compare_len) {
		if (memcmp(skb->data, &read_address_event[1], READ_ADDRESS_EVT_HDR_LEN - 1) == 0
			&& (skb->len == (READ_ADDRESS_EVT_HDR_LEN - 1 + BD_ADDRESS_SIZE))) {
			memcpy(bdev->bdaddr, &skb->data[READ_ADDRESS_EVT_PAYLOAD_OFFSET - 1], BD_ADDRESS_SIZE);
			BTMTK_INFO("%s: GET BDADDR = "MACSTR, __func__, MAC2STR(bdev->bdaddr));
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
		} else if (memcmp(skb->data, event_need_compare,
					event_need_compare_len) == 0) {
			/* if it is wobx debug event, just print in kernel log, drop it
			 * by driver, don't send to stack
			 */
			if (skb->data[0] == WOBLE_DEBUG_EVT_TYPE)
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: wobx debug log:", __func__);

			/* If driver need to check result from skb, it can get from io_buf */
			/* Such as chip_id, fw_version, etc. */
			memcpy(skb_push(skb, 1), &bt_cb(skb)->pkt_type, 1);
			memcpy(bdev->io_buf, skb->data, skb->len);
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			BTMTK_DBG("%s, compare success", __func__);
		} else {
			BTMTK_INFO("%s compare fail", __func__);
			BTMTK_INFO_RAW(event_need_compare, event_need_compare_len,
				"%s: event_need_compare:", __func__);
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: skb->data:", __func__);

			if (btmtk_vendor_cmd_filter(bdev, skb))
				return 1;

			return 0;
		}

		return 1;
	} else {
		if (btmtk_vendor_cmd_filter(bdev, skb))
			return 1;
	}

	return 0;
}

int btmtk_uart_send_and_recv(struct btmtk_dev *bdev,
		struct sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type , bool flag)
{
	unsigned long comp_event_timo = 0, start_time = 0;
	int ret = -1;

	if (event) {
		if (event_len > EVENT_COMPARE_SIZE) {
			BTMTK_ERR("%s, event_len (%d) > EVENT_COMPARE_SIZE(%d), error",
				__func__, event_len, EVENT_COMPARE_SIZE);
			ret = -1;
			goto exit;
		}
		event_compare_status = BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE;
		memcpy(event_need_compare, event + 1, event_len - 1);
		event_need_compare_len = event_len - 1;

		start_time = jiffies;
		/* check hci event /wmt event for uart/UART interface, check hci
		 * event for USB interface
		 */
		comp_event_timo = jiffies + msecs_to_jiffies(WOBLE_COMP_EVENT_TIMO);
		BTMTK_DBG("event_need_compare_len %d, event_compare_status %d",
			event_need_compare_len, event_compare_status);
	} else {
		event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
	}

	BTMTK_DBG_RAW(skb->data, skb->len, "%s, send, len = %d", __func__, skb->len);

	ret = btmtk_uart_send_cmd(bdev, skb, delay, retry, pkt_type, flag);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_cmd failed!!", __func__);
		goto fw_assert;
	}

	do {
		/* check if event_compare_success */
		if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
			ret = 0;
			break;
		}
		usleep_range(10, 100);
	} while (time_before(jiffies, comp_event_timo));

	event_compare_status = BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
	goto exit;
fw_assert:
	btmtk_send_assert_cmd(bdev);
exit:
	return ret;
}

static int btmtk_uart_load_fw_patch_using_dma(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset, int patch_flag)
{
	return 0;
}

static int btmtk_uart_subsys_reset(struct btmtk_dev *bdev)
{
	//struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	/* Process GPIO for subsys reset pin */
	return 0;
}

static int btmtk_uart_whole_reset(struct btmtk_dev *bdev)
{
	int ret = -1;
	/* Process GPIO for whole chip reset pin */
	/* if wifi use pcie interface, it will cause system reboot. */
	return ret;
}

static int btmtk_uart_probe(struct serdev_device *serdev)
{
	int err = -1;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;

	bdev = serdev_device_get_drvdata(serdev);
	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		return -ENOMEM;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	cif_dev->serdev = serdev;

	/* if (btmtk_uart_register_dev(cif_dev) < 0) {
		BTMTK_ERR("Failed to register BT device!");
		return -ENODEV;
	} */

	err = btmtk_cif_allocate_memory(cif_dev);
	if (err < 0) {
		BTMTK_ERR("[ERR] btmtk_cif_allocate_memory failed!");
		goto end;
	}

	err = btmtk_main_cif_initialize(bdev, HCI_UART);
	if (err < 0) {
		BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed!");
		goto free_mem;
	}

	err = btmtk_load_rom_patch(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk load rom patch failed!");
		goto deinit;
	}

	err = btmtk_woble_initialize(bdev, &cif_dev->bt_woble);
	if (err < 0) {
		BTMTK_ERR("btmtk_main_woble_initialize failed, do chip reset!!!");
		goto free_setting;
	}

	err = btmtk_register_hci_device(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk_register_hci_device failed!");
		goto free_setting;
	}

	goto end;

free_setting:
	btmtk_woble_uninitialize(&cif_dev->bt_woble);
	btmtk_free_setting_file(bdev);
deinit:
	btmtk_main_cif_uninitialize(bdev, HCI_USB);
free_mem:
	btmtk_cif_free_memory(cif_dev);
//unreg_uart:
	//btmtk_uart_unregister_dev(cif_dev);
end:
	BTMTK_INFO("%s normal end, ret = %d", __func__, err);

	return 0;
}

static void btmtk_uart_disconnect(struct serdev_device *serdev)
{
	struct btmtk_dev *bdev = serdev_device_get_drvdata(serdev);
	struct btmtk_uart_dev *cif_dev = NULL;

	if (!bdev)
		return;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	btmtk_woble_uninitialize(&cif_dev->bt_woble);
	btmtk_cif_free_memory(cif_dev);
	//btmtk_uart_unregister_dev(bdev->cif_dev);

	btmtk_main_cif_disconnect_notify(bdev, HCI_UART);
}

static int btmtk_cif_probe(struct serdev_device *serdev)
{
	int ret = -1;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;

	/* Mediatek Driver Version */
	BTMTK_INFO("%s: MTK BT Driver Version : %s", __func__, VERSION);

	/* Retrieve priv data and set to interface structure */
	bdev = btmtk_get_dev();
	bdev->cif_dev = &g_uart_dev;
	serdev_device_set_drvdata(serdev, bdev);

	/* Retrieve current HIF event state */
	cif_event = HIF_EVENT_PROBE;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s priv setting is NULL", __func__);
		return -ENODEV;
	}

	cif_state = &bdev->cif_state[cif_event];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	/* Do HIF events */
	ret = btmtk_uart_probe(serdev);

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);

	return ret;
}

static void btmtk_cif_disconnect(struct serdev_device *serdev)
{
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;

	bdev = serdev_device_get_drvdata(serdev);

	/* Retrieve current HIF event state */
	cif_event = HIF_EVENT_DISCONNECT;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s priv setting is NULL", __func__);
		return;
	}

	cif_state = &bdev->cif_state[cif_event];

	btmtk_uart_cif_mutex_lock(bdev);
	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	/* Do HIF events */
	btmtk_uart_disconnect(serdev);

	/* Set End/Error state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	btmtk_uart_cif_mutex_unlock(bdev);
}

#ifdef CONFIG_PM
static int btmtk_cif_suspend(struct device *dev)
{
	int ret = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	int state = BTMTK_STATE_UNKNOWN;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_woble *bt_woble = NULL;
	struct serdev_device *serdev = NULL;

	BTMTK_INFO("%s, enter", __func__);

	if (!dev)
		return 0;
	serdev = to_serdev_device(dev);
	if (!serdev)
		return 0;
	bdev = serdev_device_get_drvdata(serdev);
	if (!bdev)
		return 0;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	bt_woble = &cif_dev->bt_woble;

	state = btmtk_get_chip_state(bdev);
	/* Retrieve current HIF event state */
	if (state == BTMTK_STATE_FW_DUMP) {
		BTMTK_WARN("%s: FW dumping ongoing, don't dos suspend flow!!!", __func__);
		cif_event = HIF_EVENT_FW_DUMP;
	} else {
		cif_event = HIF_EVENT_SUSPEND;
	}

	cif_state = &bdev->cif_state[cif_event];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

#if CFG_SUPPORT_DVT
	BTMTK_INFO("%s: SKIP Driver woble_suspend flow", __func__);
#else
	ret = btmtk_woble_suspend(bt_woble);
	if (ret < 0)
		BTMTK_ERR("%s: btmtk_woble_suspend return fail %d", __func__, ret);
#endif

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);

	BTMTK_INFO("%s, end. ret = %d", __func__, ret);
	return ret;
}

static int btmtk_cif_resume(struct device *dev)
{
	u8 ret = 0;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_woble *bt_woble = NULL;
	struct serdev_device *serdev = NULL;

	BTMTK_INFO("%s, enter", __func__);

	if (!dev)
		return 0;
	serdev = to_serdev_device(dev);
	if (!serdev)
		return 0;
	bdev = serdev_device_get_drvdata(serdev);
	if (!bdev)
		return 0;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	bdev->suspend_count--;
	if (bdev->suspend_count) {
		BTMTK_INFO("data->suspend_count %d, return 0", bdev->suspend_count);
		return 0;
	}

	cif_state = &bdev->cif_state[HIF_EVENT_RESUME];
	bt_woble = &cif_dev->bt_woble;

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

#if CFG_SUPPORT_DVT
	BTMTK_INFO("%s: SKIP Driver woble_resume flow", __func__);
#else
	ret = btmtk_woble_resume(bt_woble);
	if (ret < 0)
		BTMTK_ERR("%s: btmtk_woble_resume return fail %d", __func__, ret);
#endif
	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);

	BTMTK_INFO("end");
	return 0;
}
#endif	/* CONFIG_PM */


static const struct serdev_device_ops btmtkuart_client_ops = {
	.receive_buf = btmtk_uart_receive_buf,
	.write_wakeup = btmtk_uart_write_wakeup,
};

#ifdef CONFIG_PM
static const struct dev_pm_ops btmtk_uart_pm_ops = {
	.suspend = btmtk_cif_suspend,
	.resume = btmtk_cif_resume,
};
#endif

static const struct of_device_id mtk_of_match_table[] = {
	{ }
};

MODULE_DEVICE_TABLE(of, mtk_of_match_table);

static struct serdev_device_driver btmtk_uart_driver = {
	.probe = btmtk_cif_probe,
	.remove = btmtk_cif_disconnect,
	.driver = {
		.owner = THIS_MODULE,
		.name = "btuart",
		.pm = &btmtk_uart_pm_ops,
		.of_match_table = of_match_ptr(mtk_of_match_table),
	}
};

//module_serdev_device_driver(btmtk_uart_driver);

static int uart_register(void)
{
	BTMTK_INFO("%s", __func__);
	serdev_device_driver_register(&btmtk_uart_driver);
	return 0;
}

static int uart_deregister(void)
{
	BTMTK_INFO("%s", __func__);
	serdev_device_driver_unregister(&btmtk_uart_driver);
	return 0;
}

static void btmtk_uart_chip_reset_notify(struct btmtk_dev *bdev)
{
	//struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
}

int btmtk_cif_register(void)
{
	struct hif_hook_ptr hook;

	BTMTK_INFO("%s", __func__);

	memset(&hook, 0, sizeof(hook));
	hook.open = btmtk_uart_open;
	hook.close = btmtk_uart_close;
	hook.reg_read = btmtk_uart_read_register;
	hook.send_cmd = btmtk_uart_send_cmd;
	hook.send_and_recv = btmtk_uart_send_and_recv;
	hook.event_filter = btmtk_uart_event_filter;
	hook.subsys_reset = btmtk_uart_subsys_reset;
	hook.whole_reset = btmtk_uart_whole_reset;
	hook.chip_reset_notify = btmtk_uart_chip_reset_notify;
	hook.cif_mutex_lock = btmtk_uart_cif_mutex_lock;
	hook.cif_mutex_unlock = btmtk_uart_cif_mutex_unlock;
	hook.dl_dma = btmtk_uart_load_fw_patch_using_dma;
	btmtk_reg_hif_hook(&hook);

	uart_register();
	return 0;
}

int btmtk_cif_deregister(void)
{
	BTMTK_INFO("%s", __func__);
	uart_deregister();
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

