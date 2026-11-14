/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_define.h"
#include "btmtk_uart_tty.h"
#include "btmtk_main.h"

#define LOG TRUE

/*============================================================================*/
/* Function Prototype */
/*============================================================================*/
static DECLARE_WAIT_QUEUE_HEAD(tx_wait_q);
static DECLARE_WAIT_QUEUE_HEAD(fw_to_md_wait_q);
static DEFINE_MUTEX(btmtk_uart_ops_mutex);
#define UART_OPS_MUTEX_LOCK()	mutex_lock(&btmtk_uart_ops_mutex)
#define UART_OPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_uart_ops_mutex)

static char event_need_compare[EVENT_COMPARE_SIZE] = {0};
static char event_need_compare_len;
static char event_compare_status;
static struct tty_struct *g_tty;
static struct tty_ldisc_ops btmtk_uart_ldisc;

static int btmtk_uart_open(struct hci_dev *hdev)
{
	BTMTK_INFO("%s enter!", __func__);
	return 0;
}

static int btmtk_uart_close(struct hci_dev *hdev)
{
	BTMTK_INFO("%s enter!", __func__);
	return 0;
}

static int btmtk_send_to_tx_queue(struct btmtk_uart_dev *cif_dev, struct sk_buff *skb)
{
	ulong flags = 0;

	BTMTK_DBG("%s: start", __func__);
	spin_lock_irqsave(&cif_dev->tx_lock, flags);
	skb_queue_tail(&cif_dev->tx_queue, skb);
	spin_unlock_irqrestore(&cif_dev->tx_lock, flags);
	wake_up_interruptible(&tx_wait_q);
	BTMTK_DBG("%s: end", __func__);
	return 0;
}

int btmtk_uart_send_cmd(struct btmtk_dev *bdev, struct sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;

	BTMTK_DBG("%s: start", __func__);

	if (bdev == NULL) {
		BTMTK_ERR("bdev is NULL");
		ret = -1;
		kfree_skb(skb);
		skb = NULL;
		goto exit;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("cif_dev is NULL, bdev=%p", bdev);
		ret = -1;
		kfree_skb(skb);
		skb = NULL;
		goto exit;
	}

	ret = btmtk_send_to_tx_queue(cif_dev, skb);

exit:
	return ret;

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

	return ret;
}

static int btmtk_uart_write_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	int ret;
	u8 cmd[WRITE_REGISTER_CMD_LEN] = { 0x01, 0x6F, 0xFC, 0x14,
			0x01, 0x08, 0x10, 0x00,
			0x01, 0x01, 0x00, 0x01,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0xFF, 0xFF, 0xFF, 0xFF };
	u8 event[WRITE_REGISTER_EVT_HDR_LEN] = { 0x04, 0xE4, 0x08,
			0x02, 0x08, 0x04, 0x00,
			0x00, 0x00, 0x00, 0x01 };

	BTMTK_INFO("%s: write reg=%x, value=0x%08x", __func__, reg, *val);
	memcpy(&cmd[MCU_ADDRESS_OFFSET_CMD], &reg, BT_REG_LEN);
	memcpy(&cmd[MCU_VALUE_OFFSET_CMD], val, BT_REG_VALUE_LEN);

	ret = btmtk_main_send_cmd(bdev, cmd, WRITE_REGISTER_CMD_LEN, event, WRITE_REGISTER_EVT_HDR_LEN, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	return ret;
}

int btmtk_uart_event_filter(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	const u8 read_address_event[READ_ADDRESS_EVT_HDR_LEN] = { 0x4, 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00 };
	const u8 get_baudrate_event[GETBAUD_EVT_LEN] = { 0x04, 0xE4, 0x0A, 0x02, 0x04, 0x06, 0x00, 0x00, 0x02 };

	/* event may be fragment in uart */
	bdev->recv_evt_len = skb->len;
	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE &&
		skb->len >= event_need_compare_len) {
		memset(bdev->io_buf, 0, IO_BUF_SIZE);
		if (memcmp(skb->data, &get_baudrate_event[1], GETBAUD_EVT_LEN - 1) == 0
			&& (skb->len == (GETBAUD_EVT_LEN - HCI_TYPE_SIZE + BAUD_SIZE))) {
			BTMTK_INFO("%s: GET BAUD = %02X %02X %02X, FC = %02X", __func__,
				skb->data[10], skb->data[9], skb->data[8], skb->data[11]);
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
		} else if (memcmp(skb->data, &read_address_event[1], READ_ADDRESS_EVT_HDR_LEN - 1) == 0
			&& (skb->len == (READ_ADDRESS_EVT_HDR_LEN - HCI_TYPE_SIZE + BD_ADDRESS_SIZE))) {
			u8 buffer[32] = {0};

			memcpy(bdev->bdaddr, &skb->data[READ_ADDRESS_EVT_PAYLOAD_OFFSET - 1], BD_ADDRESS_SIZE);
			BTMTK_INFO("%s: GET BDADDR = %s", __func__, MAC2STR(bdev->bdaddr, buffer, sizeof(buffer)));
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
			bdev->io_buf[0] = bt_cb(skb)->pkt_type;
			memmove(&bdev->io_buf[1], skb->data, skb->len);
			/* if io_buf is not update timely, it will write wrong number to register
			 * it might make uart pinmux been changed, add delay or print log can avoid this
			 * or mstar member said we can also use dsb(ISHST);
			 */
			msleep(IO_BUF_DELAY_TIME);
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
	}

	if (btmtk_vendor_cmd_filter(bdev, skb))
		return 1;
	return 0;
}

int btmtk_uart_send_and_recv(struct btmtk_dev *bdev,
		struct sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag)
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

		ret = -1;
		usleep_range(10, 100);
	} while (time_before(jiffies, comp_event_timo));

	if (ret == -1) {
		BTMTK_ERR("%s wait event timeout!!", __func__);
		bdev->recv_evt_len = 0;
		ret = -ERRNUM;
		goto fw_assert;
	}

	event_compare_status = BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
	goto exit;
fw_assert:
	btmtk_send_assert_cmd(bdev);
exit:
	return ret;
}

int btmtk_uart_send_set_uart_cmd(struct hci_dev *hdev, struct UART_CONFIG *uart_cfg)
{
	u8 cmd_115200[] = { 0x01, 0x6F, 0xFC, 0x09,
		0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0xC2, 0x01, 0x00};
	u8 cmd_230400[] = { 0x01, 0x6F, 0xFC, 0x09,
		0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x84, 0x03, 0x00};
	u8 cmd_921600[] = { 0x01, 0x6F, 0xFC, 0x09,
		0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x10, 0x0E, 0x00};
	u8 cmd_2M[] = { 0x01, 0x6F, 0xFC, 0x09,
		0x01, 0x04, 0x05, 0x00, 0x01, 0x80, 0x84, 0x1E, 0x00};
	u8 cmd_3M[] = { 0x01, 0x6F, 0xFC, 0x09,
		0x01, 0x04, 0x05, 0x00, 0x01, 0xC0, 0xC6, 0x2D, 0x00};
	u8 cmd_4M[] = { 0x01, 0x6F, 0xFC, 0x09,
		0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x09, 0x3D, 0x00};

	u8 event[] = {0x04, 0xE4, 0x06, 0x02, 0x04, 0x02, 0x00, 0x00, 0x01};
	u8 *cmd = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	switch (uart_cfg->iBaudrate) {
	case 230400:
		cmd = cmd_230400;
		break;
	case 921600:
		cmd = cmd_921600;
		break;
	case 2000000:
		cmd = cmd_2M;
		break;
	case 3000000:
		cmd = cmd_3M;
		break;
	case 4000000:
		cmd = cmd_4M;
		break;
	default:
		/* default chip baud is 115200 */
		cmd = cmd_115200;
	}

	switch (uart_cfg->fc) {
	case UART_HW_FC:
		cmd[BT_FLOWCTRL_OFFSET] = BT_HW_FC;
		break;
	case UART_LINUX_FC:
		cmd[BT_FLOWCTRL_OFFSET] = BT_SW_FC;
		break;
	case UART_MTK_SW_FC:
		cmd[BT_FLOWCTRL_OFFSET] = BT_MTK_SW_FC;
		break;
	default:
		/* default disable flow control */
		cmd[BT_FLOWCTRL_OFFSET] = BT_NONE_FC;
	}

	ret = btmtk_main_send_cmd(bdev,
			cmd, SETBAUD_CMD_LEN, event, SETBAUD_EVT_LEN,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_set_uart_cmd failed!!", __func__);
		return ret;
	}

	cif_dev->uart_baudrate_set = 1;
	BTMTK_INFO("%s done", __func__);
	return 0;
}

static int btmtk_uart_send_query_uart_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x04, 0x01, 0x00, 0x02};
	u8 event[] = { 0x04, 0xE4, 0x0a, 0x02, 0x04, 0x06, 0x00, 0x00, 0x02};
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;

	ret = btmtk_main_send_cmd(bdev, cmd, GETBAUD_CMD_LEN, event, GETBAUD_EVT_LEN, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_query_uart_cmd failed!!", __func__);
		return ret;
	}

	BTMTK_INFO("%s done", __func__);
	return ret;
}

int btmtk_uart_send_wakeup_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6f, 0xfc, 0x01, 0xFF };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x03};
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev->uart_baudrate_set == 0) {
		BTMTK_INFO("%s uart baudrate is 115200, no need", __func__);
		return 0;
	}

	ret = btmtk_main_send_cmd(bdev, cmd, WAKEUP_CMD_LEN, event, WAKEUP_EVT_LEN,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_query_uart_cmd failed!!", __func__);
		return ret;
	}

	BTMTK_INFO("%s done", __func__);
	return ret;
}


#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>

#define BT_OF_NODE ("mediatek,bt_7xxx")
#define RST_ON_PINCTRL_NAME		("bt_rst_on_7xxx")                       /* BT RST: gpio mode,  output high */
#define RST_OFF_PINCTRL_NAME		("bt_rst_off_7xxx")                  /* BT RST: gpio mode,  output low */

static struct pinctrl *pinctrl_ptr;


static int btmtk_ce_gpio_init(struct btmtk_uart_dev *cif_dev)
{
	BTMTK_INFO("%s start", __func__);

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (pinctrl_ptr == NULL) {
		BTMTK_INFO("%s init pinctrl", __func__);
		cif_dev->tty->dev->of_node = of_find_compatible_node(NULL, NULL, BT_OF_NODE);
		if (!cif_dev->tty->dev->of_node) {
			BTMTK_INFO("%s: %s of_node not found", __func__, BT_OF_NODE);
		} else {
			pinctrl_ptr = devm_pinctrl_get(cif_dev->tty->dev);
			if (IS_ERR(pinctrl_ptr))
				BTMTK_INFO("%s: fail to get bt pinctrl", __func__);
		}
	}

	return 0;
}

static int btmtk_pinctrl_exec(const char *name)
{
	struct pinctrl_state *pinctrl;
	int ret = 0;

	BTMTK_INFO("%s start %s", __func__, name);
	if (IS_ERR(pinctrl_ptr)) {
		BTMTK_INFO("%s: fail to get bt pinctrl", __func__);
		return 0;
	}
	pinctrl = pinctrl_lookup_state(pinctrl_ptr, name);
	if (!IS_ERR(pinctrl)) {
		ret = pinctrl_select_state(pinctrl_ptr, pinctrl);
		if (ret) {
			BTMTK_ERR("%s: pinctrl %s fail [%d]", __func__, name, ret);
			return 0;
		}
	} else {
		BTMTK_INFO("%s: pinctrl %s is not exist", __func__, name);
		return 0;
	}
	BTMTK_INFO("%s end %s", __func__, name);
	return 0;
}

int btmtk_ce_subsys_reset(struct btmtk_dev *bdev)
{
	int ret = 0;

	BTMTK_INFO("%s start", __func__);

	ret = btmtk_pinctrl_exec(RST_ON_PINCTRL_NAME);
	if (ret < 0)
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_ON_PINCTRL_NAME);

	msleep(100);

	ret = btmtk_pinctrl_exec(RST_OFF_PINCTRL_NAME);
	if (ret < 0)
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_OFF_PINCTRL_NAME);

	msleep(100);

	ret = btmtk_pinctrl_exec(RST_ON_PINCTRL_NAME);
	if (ret < 0)
		BTMTK_INFO("%s: pinctrl %s fail", __func__, RST_ON_PINCTRL_NAME);

	msleep(100);

	return ret;
}

static int btmtk_uart_subsys_reset(struct btmtk_dev *bdev)
{
	struct ktermios new_termios;
	struct tty_struct *tty;
	struct UART_CONFIG uart_cfg;
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	uart_cfg = cif_dev->uart_cfg;
	tty = cif_dev->tty;
	new_termios = tty->termios;

	if (bdev->bt_cfg.dongle_reset_gpio_pin == -1) {
		btmtk_ce_subsys_reset(bdev);
	} else {
		BTMTK_INFO("%s tigger reset pin: %d", __func__, bdev->bt_cfg.dongle_reset_gpio_pin);
		gpio_set_value(bdev->bt_cfg.dongle_reset_gpio_pin, 0);
		msleep(SUBSYS_RESET_GPIO_DELAY_TIME);
		gpio_set_value(bdev->bt_cfg.dongle_reset_gpio_pin, 1);
		/* Basically, we need to polling the cr (BT_MISC) until the subsys reset is completed
		 * However, there is no uart_hw mechnism in buzzard, we can't read the info from controller now
		 * use msleep instead currently
		 */
		msleep(SUBSYS_RESET_GPIO_DELAY_TIME);
	}

	/* Flush any pending characters in the driver and discipline. */
	tty_ldisc_flush(tty);
	tty_driver_flush_buffer(tty);

	/* set tty host baud and flowcontrol to default value */
	BTMTK_INFO("Set default baud: %d, disable flowcontrol", BT_UART_DEFAULT_BAUD);
	tty_termios_encode_baud_rate(&new_termios, BT_UART_DEFAULT_BAUD, BT_UART_DEFAULT_BAUD);
	new_termios.c_cflag &= ~(CRTSCTS);
	new_termios.c_iflag &= ~(NOFLSH);
	tty_set_termios(tty, &new_termios);

	/* set chip baud and flowcontrol to config setting */
	ret = btmtk_uart_send_set_uart_cmd(bdev->hdev, &uart_cfg);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_set_uart_cmd failed!!", __func__);
		goto exit;
	}

	/* set tty host baud and flowcontrol to config setting */
	BTMTK_INFO("Set config baud: %d, flowcontrol: %d", uart_cfg.iBaudrate, uart_cfg.fc);
	tty_termios_encode_baud_rate(&new_termios, uart_cfg.iBaudrate, uart_cfg.iBaudrate);

	switch (uart_cfg.fc) {
	/* HW FC Enable */
	case UART_HW_FC:
		new_termios.c_cflag |= CRTSCTS;
		new_termios.c_iflag &= ~(NOFLSH);
		break;
	/* Linux Software FC */
	case UART_LINUX_FC:
		new_termios.c_iflag |= (IXON | IXOFF | IXANY);
		new_termios.c_cflag &= ~(CRTSCTS);
		new_termios.c_iflag &= ~(NOFLSH);
		break;
	/* MTK Software FC */
	case UART_MTK_SW_FC:
		new_termios.c_cflag &= ~(CRTSCTS);
		new_termios.c_iflag |= NOFLSH;
		break;
	/* default disable flow control */
	default:
		new_termios.c_cflag &= ~(CRTSCTS);
		new_termios.c_iflag &= ~(NOFLSH);
	}

	tty_set_termios(tty, &new_termios);
	ret = btmtk_uart_send_wakeup_cmd(bdev->hdev);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_set_uart_cmd failed!!", __func__);
		goto exit;
	}

	BTMTK_INFO("%s done", __func__);

exit:
	return ret;
}

static void btmtk_uart_waker_notify(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s enter!", __func__);
	schedule_work(&bdev->reset_waker);
}

static void btmtk_uart_cif_mutex_lock(struct btmtk_dev *bdev)
{
	UART_OPS_MUTEX_LOCK();
}

static void btmtk_uart_cif_mutex_unlock(struct btmtk_dev *bdev)
{
	UART_OPS_MUTEX_UNLOCK();
}

static void btmtk_uart_chip_reset_notify(struct btmtk_dev *bdev)
{
	//struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
}

static int btmtk_uart_load_fw_patch_using_dma(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset)
{
	int cur_len = 0;
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;
	s32 sent_len;
	u8 cmd[LD_PATCH_CMD_LEN] = {0x02, 0x6F, 0xFC, 0x05, 0x00, 0x01, 0x01, 0x01, 0x00, PATCH_PHASE3};
	u8 event[LD_PATCH_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; /* event[7] is status*/

	if (bdev == NULL || image == NULL || fwbuf == NULL) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		ret = -1;
		goto exit;
	}

	BTMTK_INFO("%s: loading rom patch... start", __func__);
	while (1) {
		sent_len = (section_dl_size - cur_len) >= (UPLOAD_PATCH_UNIT) ?
				(UPLOAD_PATCH_UNIT) : (section_dl_size - cur_len);

		if (sent_len > 0) {
			memcpy(image, fwbuf + section_offset + cur_len, sent_len);
		       BTMTK_DBG("%s: sent_len = %d, cur_len = %d", __func__,
					     sent_len, cur_len);
		       cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
		       ret = cif_dev->tty->ops->write(cif_dev->tty, image, sent_len);
		       BTMTK_DBG("%s, send length: ret= %d", __func__, ret);

			if (ret < 0) {
				BTMTK_ERR("%s: send patch failed, terminate", __func__);
				goto exit;
			}
			cur_len += ret;
		} else
			break;
	}

	BTMTK_INFO("%s: send dl cmd", __func__);
	ret = btmtk_main_send_cmd(bdev,
			cmd, LD_PATCH_CMD_LEN,
			event, LD_PATCH_EVT_LEN,
			PATCH_DOWNLOAD_PHASE3_DELAY_TIME,
			PATCH_DOWNLOAD_PHASE3_RETRY,
			BTMTK_TX_ACL_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: send wmd dl cmd failed, terminate!", __func__);
		return ret;
	}
	BTMTK_INFO("%s: loading rom patch... Done", __func__);

exit:
	return ret;
}

int btmtk_cif_send_cmd(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, int retry, int delay)
{
	int ret = -1, len = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	BTMTK_DBG_RAW(cmd, cmd_len, "%s, len = %d Send CMD : ", __func__, cmd_len);
	/* BTMTK_INFO("%s: tty %p\n", __func__, bdev->tty); */
	while (len != cmd_len) {
		ret = cif_dev->tty->ops->write(cif_dev->tty, cmd + len, cmd_len - len);
		len += ret;
		BTMTK_DBG("%s, len = %d", __func__, len);
	}

	return ret;
}

static int btmtk_uart_tx_thread(void *data)
{
	struct btmtk_dev *bdev = data;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct sk_buff *skb;
	ulong flags = 0;

	BTMTK_INFO("%s start", __func__);

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	while (1) {
		wait_event_interruptible(tx_wait_q,
				(skb_queue_len(&cif_dev->tx_queue) > 0
				|| kthread_should_stop()));
		if (kthread_should_stop()) {
			BTMTK_WARN("thread is stopped, break");
			break;
		}

		while (skb_queue_len(&cif_dev->tx_queue)) {
			spin_lock_irqsave(&cif_dev->tx_lock, flags);
			skb = skb_dequeue(&cif_dev->tx_queue);
			spin_unlock_irqrestore(&cif_dev->tx_lock, flags);
			if (skb) {
				btmtk_cif_send_cmd(bdev,
					skb->data, skb->len,
					5, 0);
				kfree_skb(skb);
			}
		}
	}

	BTMTK_INFO("%s end", __func__);
	return 0;
}

static int btmtk_tx_thread_exit(struct btmtk_uart_dev *cif_dev)
{
	BTMTK_INFO("%s", __func__);

	if (!IS_ERR(cif_dev->tx_task))
		kthread_stop(cif_dev->tx_task);

	skb_queue_purge(&cif_dev->tx_queue);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

/* Allocate Uart-Related memory */
static int btmtk_uart_allocate_memory(void)
{
	return 0;
}

int btmtk_cif_send_calibration(struct btmtk_dev *bdev)
{
	return 0;
}

static int btmtk_uart_set_pinmux(struct btmtk_dev *bdev)
{
	int err = 0;
	u32 val;

	/* BT_PINMUX_CTRL_REG setup  */
	btmtk_uart_read_register(bdev, BT_PINMUX_CTRL_REG, &val);
	val |= BT_PINMUX_CTRL_ENABLE;
	err = btmtk_uart_write_register(bdev, BT_PINMUX_CTRL_REG, &val);
	if (err < 0) {
		BTMTK_ERR("btmtk_uart_write_register failed!");
		return -1;
	}
	btmtk_uart_read_register(bdev, BT_PINMUX_CTRL_REG, &val);

	/* BT_PINMUX_CTRL_REG setup  */
	btmtk_uart_read_register(bdev, BT_SUBSYS_RST_REG, &val);
	val |= BT_SUBSYS_RST_ENABLE;
	err = btmtk_uart_write_register(bdev, BT_SUBSYS_RST_REG, &val);
	if (err < 0) {
		BTMTK_ERR("btmtk_uart_write_register failed!");
		return -1;
	}
	btmtk_uart_read_register(bdev, BT_SUBSYS_RST_REG, &val);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

static int btmtk_uart_init(struct btmtk_dev *bdev)
{
	int err = 0;

	err = btmtk_main_cif_initialize(bdev, HCI_UART);
	if (err < 0) {
		BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed!");
		goto end;
	}

	err = btmtk_uart_set_pinmux(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk_uart_set_pinmux failed!");
		goto deinit;
	}

	INIT_WORK(&bdev->reset_waker, btmtk_reset_waker);
	goto end;

deinit:
	btmtk_main_cif_uninitialize(bdev, HCI_UART);
end:
	BTMTK_INFO("%s done", __func__);
	return err;
}

/* ------ LDISC part ------ */
/* btmtk_uart_tty_probe
 *
 *     Called when line discipline changed to HCI_UART.
 *
 * Arguments:
 *     tty    pointer to tty info structure
 * Return Value:
 *     0 if success, otherwise error code
 */
static int btmtk_uart_tty_probe(struct tty_struct *tty)
{
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s: tty %p", __func__, tty);

	bdev = dev_get_drvdata(tty->dev);
	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		return -ENOMEM;
	}

	/* Init tty-related operation */
	tty->receive_room = 65536;
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 12, 0) > LINUX_VERSION_CODE))
	tty->port->low_latency = 1;
#endif

	btmtk_uart_allocate_memory();

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	tty->disc_data = bdev;
	cif_dev->tty = tty;
	dev_set_drvdata(tty->dev, bdev);

	spin_lock_init(&cif_dev->tx_lock);
	skb_queue_head_init(&cif_dev->tx_queue);
	cif_dev->tx_task = kthread_run((void *)btmtk_uart_tx_thread,
					bdev, "btmtk_uart_tx_thread");
	if (IS_ERR(cif_dev->tx_task)) {
		BTMTK_ERR("%s create tx thread FAILED", __func__);
		return -1;
	}

	cif_dev->stp_cursor = 2;
	cif_dev->stp_dlen = 0;

	/* definition changed!! */
	if (tty->ldisc->ops->flush_buffer)
		tty->ldisc->ops->flush_buffer(tty);

	tty_driver_flush_buffer(tty);

	btmtk_ce_gpio_init(cif_dev);
	btmtk_ce_subsys_reset(bdev);

	BTMTK_INFO("%s: tty done %p", __func__, tty);

	return 0;
}

/* btmtk_uart_tty_disconnect
 *
 *    Called when the line discipline is changed to something
 *    else, the tty is closed, or the tty detects a hangup.
 */
static void btmtk_uart_tty_disconnect(struct tty_struct *tty)
{
	struct btmtk_dev *bdev = tty->disc_data;

	BTMTK_INFO("%s: tty %p", __func__, tty);

	btmtk_woble_uninitialize(bdev->bt_woble);
	cancel_work_sync(&bdev->reset_waker);
	btmtk_tx_thread_exit(bdev->cif_dev);
	btmtk_main_cif_disconnect_notify(bdev, HCI_UART);
}

/*
 * We don't provide read/write/poll interface for user space.
 */
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 10, 20) > LINUX_VERSION_CODE))
static ssize_t btmtk_uart_tty_read(struct tty_struct *tty, struct file *file,
				 unsigned char *buf, size_t count)
#else
static ssize_t btmtk_uart_tty_read(struct tty_struct *tty, struct file *file,
				 unsigned char *buf, size_t nr,
				 void **cookie, unsigned long offset)
#endif
{
	BTMTK_INFO("%s: tty %p", __func__, tty);
	return 0;
}

static ssize_t btmtk_uart_tty_write(struct tty_struct *tty, struct file *file,
				 const unsigned char *data, size_t count)
{
	BTMTK_INFO("%s: tty %p", __func__, tty);
	return 0;
}

static unsigned int btmtk_uart_tty_poll(struct tty_struct *tty, struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct btmtk_dev *bdev = tty->disc_data;
	struct btmtk_uart_dev *cif_dev = bdev->cif_dev;

	if (cif_dev->subsys_reset == 1) {
		mask |= POLLIN | POLLRDNORM;                    /* readable */
		BTMTK_INFO("%s: tty %p", __func__, tty);
	}
	return mask;
}

/* btmtk_uart_tty_ioctl()
 *
 *    Process IOCTL system call for the tty device.
 *
 * Arguments:
 *
 *    tty        pointer to tty instance data
 *    file       pointer to open file object for device
 *    cmd        IOCTL command code
 *    arg        argument for IOCTL call (cmd dependent)
 *
 * Return Value:    Command dependent
 */
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 17, 0) <= LINUX_VERSION_CODE))
static int btmtk_uart_tty_ioctl(struct tty_struct *tty,
			      unsigned int cmd, unsigned long arg)
#else
static int btmtk_uart_tty_ioctl(struct tty_struct *tty, struct file *file,
			      unsigned int cmd, unsigned long arg)
#endif
{
	int err = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct UART_CONFIG uart_cfg;
	struct btmtk_dev *bdev = tty->disc_data;
	struct btmtk_uart_dev *cif_dev = NULL;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_INFO("%s: tty %p cmd = %u", __func__, tty, cmd);

	switch (cmd) {
	case HCIUARTSETPROTO:
		BTMTK_INFO("%s: <!!> Set low_latency to TRUE <!!>", __func__);
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 12, 0) > LINUX_VERSION_CODE))
		tty->port->low_latency = 1;
#endif
		break;
	case HCIUARTSETBAUD:
		if (copy_from_user(&uart_cfg, (struct UART_CONFIG __user *)arg,
					sizeof(struct UART_CONFIG)))
			return -ENOMEM;
		cif_dev->uart_cfg = uart_cfg;
		BTMTK_INFO("%s: <!!> Set BAUDRATE, fc = %d iBaudrate = %d <!!>",
				__func__, (int)uart_cfg.fc, uart_cfg.iBaudrate);
		err = btmtk_uart_send_set_uart_cmd(bdev->hdev, &uart_cfg);
		break;
	case HCIUARTSETWAKEUP:
		BTMTK_INFO("%s: <!!> Send Wakeup <!!>", __func__);
		err = btmtk_uart_send_wakeup_cmd(bdev->hdev);
		break;
	case HCIUARTGETBAUD:
		BTMTK_INFO("%s: <!!> Get BAUDRATE <!!>", __func__);
		err = btmtk_uart_send_query_uart_cmd(bdev->hdev);
		break;
	case HCIUARTSETSTP:
		BTMTK_INFO("%s: <!!> Set STP mandatory command <!!>", __func__);
		break;
	case HCIUARTLOADPATCH:
		BTMTK_INFO("%s: <!!> Set HCIUARTLOADPATCH command <!!>", __func__);
		err = btmtk_load_rom_patch(bdev);
		cif_event = HIF_EVENT_PROBE;
		cif_state = &bdev->cif_state[cif_event];
		/* Set End/Error state */
		if (err == 0)
			btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
		else {
			BTMTK_ERR("%s: Set HCIUARTLOADPATCH command failed (%d)", __func__, err);
			btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
		}

		err = btmtk_woble_initialize(bdev);
		if (err < 0)
			BTMTK_ERR("btmtk_woble_initialize failed!");
		else
			BTMTK_ERR("btmtk_woble_initialize");

		err = btmtk_register_hci_device(bdev);
		if (err < 0)
			BTMTK_ERR("btmtk_register_hci_device failed!");
		break;
	case HCIUARTINIT:
		BTMTK_INFO("%s: <!!> Set HCIUARTINIT <!!>", __func__);
		err = btmtk_uart_init(bdev);
		break;
	default:
		/* BTMTK_INFO("<!!> n_tty_ioctl_helper <!!>\n"); */
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		err = n_tty_ioctl_helper(tty, cmd, arg);
#else
		err = n_tty_ioctl_helper(tty, file, cmd, arg);
#endif
		break;
	};

	return err;
}

#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE))
static long btmtk_uart_tty_compat_ioctl(struct tty_struct *tty, struct file *file,
			      unsigned int cmd, unsigned long arg)
#elif (defined(ANDROID_OS) && (KERNEL_VERSION(5, 16, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 17, 0) > LINUX_VERSION_CODE))
static int btmtk_uart_tty_compat_ioctl(struct tty_struct *tty, struct file *file,
			      unsigned int cmd, unsigned long arg)
#else
static int btmtk_uart_tty_compat_ioctl(struct tty_struct *tty,
			      unsigned int cmd, unsigned long arg)
#endif
{
	int err = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct UART_CONFIG uart_cfg;
	struct btmtk_dev *bdev = tty->disc_data;
	struct btmtk_uart_dev *cif_dev = NULL;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_INFO("%s: tty %p cmd = %u", __func__, tty, cmd);

	switch (cmd) {
	case HCIUARTSETPROTO:
		BTMTK_INFO("%s: <!!> Set low_latency to TRUE <!!>", __func__);
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 12, 0) > LINUX_VERSION_CODE))
		tty->port->low_latency = 1;
#endif
		break;
	case HCIUARTSETBAUD:
		if (copy_from_user(&uart_cfg, (struct UART_CONFIG __user *)arg,
					sizeof(struct UART_CONFIG)))
			return -ENOMEM;
		cif_dev->uart_cfg = uart_cfg;
		BTMTK_INFO("%s: <!!> Set BAUDRATE, fc = %d iBaudrate = %d <!!>",
				__func__, (int)uart_cfg.fc, uart_cfg.iBaudrate);
		err = btmtk_uart_send_set_uart_cmd(bdev->hdev, &uart_cfg);
		break;
	case HCIUARTSETWAKEUP:
		BTMTK_INFO("%s: <!!> Send Wakeup <!!>", __func__);
		err = btmtk_uart_send_wakeup_cmd(bdev->hdev);
		break;
	case HCIUARTGETBAUD:
		BTMTK_INFO("%s: <!!> Get BAUDRATE <!!>", __func__);
		err = btmtk_uart_send_query_uart_cmd(bdev->hdev);
		break;
	case HCIUARTSETSTP:
		BTMTK_INFO("%s: <!!> Set STP mandatory command <!!>", __func__);
		break;
	case HCIUARTLOADPATCH:
		BTMTK_INFO("%s: <!!> Set HCIUARTLOADPATCH command <!!>", __func__);
		err = btmtk_load_rom_patch(bdev);
		cif_event = HIF_EVENT_PROBE;
		cif_state = &bdev->cif_state[cif_event];
		/* Set End/Error state */
		if (err == 0)
			btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
		else {
			BTMTK_ERR("%s: Set HCIUARTLOADPATCH command failed (%d)", __func__, err);
			btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
		}

		err = btmtk_woble_initialize(bdev);
		if (err < 0)
			BTMTK_ERR("btmtk_woble_initialize failed!");
		else
			BTMTK_ERR("btmtk_woble_initialize");

		err = btmtk_register_hci_device(bdev);
		if (err < 0)
			BTMTK_ERR("btmtk_register_hci_device failed!");
		break;
	case HCIUARTINIT:
		BTMTK_INFO("%s: <!!> Set HCIUARTINIT <!!>", __func__);
		err = btmtk_uart_init(bdev);
		break;
	default:
		/* BTMTK_INFO("<!!> n_tty_ioctl_helper <!!>\n"); */
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		err = n_tty_ioctl_helper(tty, cmd, arg);
#else
		err = n_tty_ioctl_helper(tty, file, cmd, arg);
#endif
		break;
	};

	return err;
}

#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 14, 0) > LINUX_VERSION_CODE))
static void btmtk_uart_tty_receive(struct tty_struct *tty, const u8 *data, char *flags, int count)
#elif (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
static void btmtk_uart_tty_receive(struct tty_struct *tty, const u8 *data, const char *flags, int count)
#else
static void btmtk_uart_tty_receive(struct tty_struct *tty, const u8 *data, const u8 *flags, size_t count)
#endif
{
	int ret = -1;
	struct btmtk_dev *bdev = tty->disc_data;

	BTMTK_DBG_RAW(data, count, "Receive");

	/* add hci device part */
	bdev->recv_evt_len = count;
	BTMTK_DBG("%s: count= %d", __func__, count);
	ret = btmtk_recv(bdev->hdev, data, count);
	if (ret < 0)
		BTMTK_ERR("%s, ret = %d", __func__, ret);
}

/* btmtk_uart_tty_wakeup()
 *
 *    Callback for transmit wakeup. Called when low level
 *    device driver can accept more send data.
 *
 * Arguments:        tty    pointer to associated tty instance data
 * Return Value:    None
 */
static void btmtk_uart_tty_wakeup(struct tty_struct *tty)
{
	BTMTK_INFO("%s: tty %p", __func__, tty);
}

static int btmtk_uart_fw_own(struct btmtk_dev *bdev)
{
	int ret;
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x01 };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x01 };

	BTMTK_INFO("%s", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, FWOWN_CMD_LEN, event, OWNTYPE_EVT_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	return ret;
}

static int btmtk_uart_driver_own(struct btmtk_dev *bdev)
{
	int ret;
	u8 cmd[] = { 0xFF };
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x03 };

	BTMTK_INFO("%s", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, DRVOWN_CMD_LEN, event, OWNTYPE_EVT_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	return ret;
}

static int btmtk_cif_probe(struct tty_struct *tty)
{
	int ret = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev;

	/* Mediatek Driver Version */
	BTMTK_INFO("%s: MTK BT Driver Version: %s", __func__, VERSION);

	/* Retrieve priv data and set to interface structure */
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_INFO("%s: bdev is NULL", __func__);
		return -ENODEV;
	}

	cif_dev = devm_kzalloc(tty->dev, sizeof(*cif_dev), GFP_KERNEL);
	if (!cif_dev)
		return -ENOMEM;

	bdev->intf_dev = tty->dev;
	bdev->cif_dev = cif_dev;
	dev_set_drvdata(tty->dev, bdev);
	g_tty = tty;

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
	ret = btmtk_uart_tty_probe(tty);

	return ret;
}

static int btmtk_cif_suspend(void)
{
	int ret = 0;
	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = dev_get_drvdata(tty->dev);
	struct btmtk_woble *bt_woble = bdev->bt_woble;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;

	BTMTK_INFO("%s", __func__);

	if (bdev->get_hci_reset) {
		BTMTK_WARN("open flow not ready(%d), retry", bdev->get_hci_reset);
		return -EAGAIN;
	}

	if (bmain_info->reset_stack_flag) {
		BTMTK_WARN("reset stack flag(%d), retry", bmain_info->reset_stack_flag);
		return -EAGAIN;
	}

	fstate = btmtk_fops_get_state(bdev);
	if ((fstate == BTMTK_FOPS_STATE_CLOSING) ||
		(fstate == BTMTK_FOPS_STATE_OPENING)) {
		BTMTK_WARN("%s: fops open/close is on-going, retry", __func__);
		return -EAGAIN;
	}

	if (bdev->suspend_count++) {
		BTMTK_WARN("Has suspended. suspend_count: %d, end", bdev->suspend_count);
		return 0;
	}

	ret = btmtk_woble_suspend(bt_woble);
	if (ret < 0)
		BTMTK_ERR("%s: btmtk_woble_suspend return fail %d", __func__, ret);

	ret = btmtk_uart_fw_own(bdev);
	if (ret < 0)
		BTMTK_ERR("%s: set fw own return fail %d", __func__, ret);

	return 0;
}

static int btmtk_cif_resume(void)
{
	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = dev_get_drvdata(tty->dev);
	struct btmtk_woble *bt_woble = bdev->bt_woble;
	struct btmtk_cif_state *cif_state = NULL;
	int ret;

	BTMTK_INFO("%s", __func__);
	bdev->suspend_count--;

	if (bdev->suspend_count) {
		BTMTK_INFO("data->suspend_count %d, return 0", bdev->suspend_count);
		return 0;
	}

	cif_state = &bdev->cif_state[HIF_EVENT_RESUME];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	ret = btmtk_uart_driver_own(bdev);
	if (ret < 0)
		BTMTK_ERR("%s: set driver own return fail %d", __func__, ret);

	ret = btmtk_woble_resume(bt_woble);
	if (ret < 0)
		BTMTK_ERR("%s: btmtk_woble_resume return fail %d", __func__, ret);

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
	return 0;
}

static int btmtk_pm_notification(struct notifier_block *this, unsigned long event, void *ptr)
{
	int ret = NOTIFY_DONE;
	int cif_event = 0, state;
	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = dev_get_drvdata(tty->dev);
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_DBG("event = %ld", event);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		state = btmtk_get_chip_state(bdev);
		/* Retrieve current HIF event state */
		if (state == BTMTK_STATE_FW_DUMP) {
			BTMTK_WARN("%s: FW dumping ongoing, don't do suspend flow!!!", __func__);
			cif_event = HIF_EVENT_FW_DUMP;
			return -EAGAIN;
		} else if (state == BTMTK_STATE_DISCONNECT) {
			BTMTK_ERR("%s: BTMTK_STATE_DISCONNECT", __func__);
			return 0;
		} else
			cif_event = HIF_EVENT_SUSPEND;

		cif_state = &bdev->cif_state[cif_event];
		/* Set Entering state */
		btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

		if (bdev->bt_cfg.support_auto_picus == true &&
			(bdev->bt_cfg.support_picus_to_host == true || atomic_read(&bmain_info->fwlog_ref_cnt) != 0)) {
			btmtk_picus_disable(bdev);
		}
#if 0
		ret = btmtk_uart_fw_own(bdev);
		BTMTK_WARN("%s: set fw own %d", __func__, ret);
#endif
		break;
	case PM_POST_SUSPEND:
		cif_state = &bdev->cif_state[HIF_EVENT_RESUME];
		/* Set Entering state */
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
		if (bdev->bt_cfg.support_auto_picus == true &&
			(bdev->bt_cfg.support_picus_to_host == true || atomic_read(&bmain_info->fwlog_ref_cnt) != 0)) {
			btmtk_picus_enable(bdev);
		}
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block btmtk_pm_notifier = {
	.notifier_call = btmtk_pm_notification,
};

static void btmtk_cif_disconnect(struct tty_struct *tty)
{
	int err = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev;

	bdev = dev_get_drvdata(tty->dev);
	cif_dev = bdev->cif_dev;

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
	btmtk_uart_tty_disconnect(tty);

	/* Set End/Error state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	btmtk_uart_cif_mutex_unlock(bdev);
	devm_kfree(tty->dev, cif_dev);

	err = unregister_pm_notifier(&btmtk_pm_notifier);
	if (err) {
		BTMTK_ERR("Unregister pm notifier failed. (%d)", err);
		return;
	}

	BTMTK_INFO("%s end", __func__);
}

static int uart_register(void)
{
	u32 err = 0;

	BTMTK_INFO("%s", __func__);

	/* Register the tty discipline */
	memset(&btmtk_uart_ldisc, 0, sizeof(btmtk_uart_ldisc));
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 13, 0) > LINUX_VERSION_CODE))
	btmtk_uart_ldisc.magic = TTY_LDISC_MAGIC;
#else
	btmtk_uart_ldisc.num = N_MTK;
#endif
	btmtk_uart_ldisc.name = "n_mtk";
	btmtk_uart_ldisc.open = btmtk_cif_probe;
	btmtk_uart_ldisc.close = btmtk_cif_disconnect;
	btmtk_uart_ldisc.read = btmtk_uart_tty_read;
	btmtk_uart_ldisc.write = btmtk_uart_tty_write;
	btmtk_uart_ldisc.ioctl = btmtk_uart_tty_ioctl;
	btmtk_uart_ldisc.compat_ioctl = btmtk_uart_tty_compat_ioctl;
	btmtk_uart_ldisc.poll = btmtk_uart_tty_poll;
	btmtk_uart_ldisc.receive_buf = btmtk_uart_tty_receive;
	btmtk_uart_ldisc.write_wakeup = btmtk_uart_tty_wakeup;
	btmtk_uart_ldisc.owner = THIS_MODULE;

#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 14, 0) > LINUX_VERSION_CODE))
	err = tty_register_ldisc(N_MTK, &btmtk_uart_ldisc);
#else
	err = tty_register_ldisc(&btmtk_uart_ldisc);
#endif
	if (err) {
		BTMTK_ERR("MTK line discipline registration failed. (%d)", err);
		return err;
	}
	err = register_pm_notifier(&btmtk_pm_notifier);
	if (err) {
		BTMTK_ERR("Register pm notifier failed. (%d)", err);
		return err;
	}

	BTMTK_INFO("%s done", __func__);
	return err;
}
static int uart_deregister(void)
{
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(LINUX_OS) && (KERNEL_VERSION(5, 14, 0) > LINUX_VERSION_CODE))
	u32 err = 0;

	err = tty_unregister_ldisc(N_MTK);
	if (err) {
		BTMTK_ERR("line discipline registration failed. (%d)", err);
		return err;
	}
#else
	tty_unregister_ldisc(&btmtk_uart_ldisc);
#endif

	return 0;
}

static int __maybe_unused btmtk_uart_suspend(struct device *dev)
{
	BTMTK_DBG("%s", __func__);
	btmtk_cif_suspend();
	return 0;
}

static int __maybe_unused btmtk_uart_resume(struct device *dev)
{
	BTMTK_DBG("%s", __func__);
	btmtk_cif_resume();
	return 0;
}

static int __maybe_unused btmtk_uart_runtime_suspend(struct device *dev)
{
	BTMTK_DBG("%s", __func__);
	return 0;
}

static int __maybe_unused btmtk_uart_runtime_resume(struct device *dev)
{
	BTMTK_DBG("%s", __func__);
	return 0;
}

static const struct dev_pm_ops btmtk_uart_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(btmtk_uart_suspend, btmtk_uart_resume)
	SET_RUNTIME_PM_OPS(btmtk_uart_runtime_suspend, btmtk_uart_runtime_resume, NULL)
};

static struct platform_device *btmtk_uart_device;

static int btmtk_uart_driver_probe(struct platform_device *pdev)
{
	BTMTK_DBG("%s", __func__);
	return 0;
}

static int btmtk_uart_driver_remove(struct platform_device *pdev)
{
	BTMTK_DBG("%s", __func__);
	return 0;
}

#define BTMTK_UART_NAME			("btmtk_uart")
static struct platform_driver btmtk_uart_driver = {
	.driver = {
		.name	= BTMTK_UART_NAME,
		.owner	= THIS_MODULE,
		.pm		= &btmtk_uart_pm_ops,
	},
	.probe = btmtk_uart_driver_probe,
	.remove = btmtk_uart_driver_remove,
};

void btmtk_platform_driver_init(void)
{
	int ret = 0;

	BTMTK_DBG("%s", __func__);
	ret = platform_driver_register(&btmtk_uart_driver);
	BTMTK_INFO("%s: platform_driver_register ret = %d", __func__, ret);
	btmtk_uart_device = platform_device_alloc(BTMTK_UART_NAME, 0);
	if (btmtk_uart_device == NULL) {
		platform_driver_unregister(&btmtk_uart_driver);
		BTMTK_ERR("%s: platform_device_alloc device fail", __func__);
		return;
	}
	ret = platform_device_add(btmtk_uart_device);
	if (ret) {
		platform_driver_unregister(&btmtk_uart_driver);
		BTMTK_ERR("%s: platform_device_add fail", __func__);
		return;
	}

}

void btmtk_platform_driver_deinit(void)
{
	BTMTK_INFO("%s", __func__);
	platform_device_unregister(btmtk_uart_device);
	platform_driver_unregister(&btmtk_uart_driver);
}

int btmtk_cif_register(void)
{
	int ret = -1;
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
	hook.chip_reset_notify = btmtk_uart_chip_reset_notify;
	hook.cif_mutex_lock = btmtk_uart_cif_mutex_lock;
	hook.cif_mutex_unlock = btmtk_uart_cif_mutex_unlock;
	hook.dl_dma = btmtk_uart_load_fw_patch_using_dma;
	hook.waker_notify = btmtk_uart_waker_notify;
	hook.enter_standby = btmtk_cif_suspend;
	btmtk_reg_hif_hook(&hook);

	ret = uart_register();
	if (ret < 0) {
		BTMTK_ERR("*** UART registration fail(%d)! ***", ret);
		return ret;
	}

	btmtk_platform_driver_init();

	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

int btmtk_cif_deregister(void)
{
	int ret = -1;

	BTMTK_INFO("%s", __func__);

	btmtk_platform_driver_deinit();

	ret = uart_deregister();
	if (ret < 0) {
		BTMTK_ERR("*** UART deregistration fail(%d)! ***", ret);
		return ret;
	}
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}
