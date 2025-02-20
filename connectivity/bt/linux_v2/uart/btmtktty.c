/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/version.h>
#if (KERNEL_VERSION(4, 11, 0) > LINUX_VERSION_CODE)
#include <linux/sched.h>
#else
#include <uapi/linux/sched/types.h>
#endif

#include "btmtk_define.h"
#include "btmtk_uart_tty.h"
#include "btmtk_main.h"

#if (USE_DEVICE_NODE == 1)
#include "btmtk_proj_sp.h"
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include "btmtk_fw_log.h"
#include "btmtk_queue.h"
extern void bthost_debug_print(void);
#else
#include "btmtk_proj_ce.h"
#endif

#define LOG TRUE

/*============================================================================*/
/* Function Prototype */
/*============================================================================*/
int btmtk_cif_send_cmd(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, int retry, int delay);
static int btmtk_tx_thread_exit(struct btmtk_uart_dev *cif_dev);
static int btmtk_tx_thread_start(struct btmtk_dev *bdev);
static int btmtk_uart_tx_thread(void *data);
static int btmtk_uart_fw_own(struct btmtk_dev *bdev);
static int btmtk_uart_driver_own(struct btmtk_dev *bdev);


/*============================================================================*/
/* Global variable */
/*============================================================================*/
static DECLARE_WAIT_QUEUE_HEAD(tx_wait_q);
static DECLARE_WAIT_QUEUE_HEAD(drv_own_wait_q);
static DECLARE_WAIT_QUEUE_HEAD(fw_to_md_wait_q);
static DEFINE_MUTEX(btmtk_uart_ops_mutex);
#define UART_OPS_MUTEX_LOCK()	mutex_lock(&btmtk_uart_ops_mutex)
#define UART_OPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_uart_ops_mutex)
static DEFINE_MUTEX(btmtk_uart_own_mutex);
#define UART_OWN_MUTEX_LOCK()	mutex_lock(&btmtk_uart_own_mutex)
#define UART_OWN_MUTEX_UNLOCK()	mutex_unlock(&btmtk_uart_own_mutex)
static DEFINE_MUTEX(btmtk_tx_thread_mutex);
#define TX_THREAD_MUTEX_LOCK()	mutex_lock(&btmtk_tx_thread_mutex)
#define TX_THREAD_MUTEX_UNLOCK()	mutex_unlock(&btmtk_tx_thread_mutex)

static struct wakeup_source *bt_trx_wakelock;

static char event_need_compare[EVENT_COMPARE_SIZE] = {0};
static char event_need_compare_len;
static char event_compare_status;
static struct tty_struct *g_tty;
static struct tty_ldisc_ops btmtk_uart_ldisc;
extern struct btmtk_dev *g_sbdev;
static CONNXO_CFG_PARAM_STRUCT sConnxo_cfg;

#if (SLEEP_ENABLE == 1)

#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
static void btmtk_fw_own_timer(unsigned long arg)
{
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)arg;

	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_RUNNING);
		wake_up_interruptible(&tx_wait_q);
	} else
		BTMTK_DBG_LIMITTED("%s: not create yet", __func__);
}
#else
static void btmtk_fw_own_timer(struct timer_list *timer)
{
	struct btmtk_uart_dev *cif_dev = from_timer(cif_dev, timer, fw_own_timer);

	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_RUNNING);
		wake_up_interruptible(&tx_wait_q);
	} else
		BTMTK_DBG_LIMITTED("%s: not create yet", __func__);

}
#endif

static void btmtk_uart_update_fw_own_timer(struct btmtk_uart_dev *cif_dev)
{

	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		BTMTK_DBG_LIMITTED("update fw own timer");
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_INIT);
		mod_timer(&cif_dev->fw_own_timer, jiffies + msecs_to_jiffies(FW_OWN_TIMEOUT));
	} else
		BTMTK_DBG_LIMITTED("%s: not create yet", __func__);
}

static void btmtk_uart_create_fw_own_timer(struct btmtk_uart_dev *cif_dev)
{
	BTMTK_DBG("%s: create fw own timer", __func__);
#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
	init_timer(&cif_dev->fw_own_timer);
	cif_dev->fw_own_timer.function = btmtk_fw_own_timer;
	cif_dev->fw_own_timer.data = (unsigned long)cif_dev;
#else
	timer_setup(&cif_dev->fw_own_timer, btmtk_fw_own_timer, 0);
#endif
	atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_INIT);
}

static void btmtk_uart_delete_fw_own_timer(struct btmtk_uart_dev *cif_dev)
{
	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_UKNOWN);
		BTMTK_WARN("%s timer deleted", __func__);
		del_timer_sync(&cif_dev->fw_own_timer);
	} else
		BTMTK_DBG_LIMITTED("%s: not create yet", __func__);
}
#endif //(SLEEP_ENABLE == 1)

static int btmtk_uart_open(struct hci_dev *hdev)
{
	BTMTK_DBG("%s enter!", __func__);
	return 0;
}

static int btmtk_uart_close(struct hci_dev *hdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	BTMTK_DBG("%s enter!", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is NULL", __func__);
		return -EINVAL;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s, cif_dev is NULL", __func__);
		return -EINVAL;
	}

	/* stop tx thread before releasing uarthub trx request */
	btmtk_tx_thread_exit(bdev->cif_dev);

#if (SLEEP_ENABLE == 1)
	btmtk_uart_delete_fw_own_timer(cif_dev);
#endif

#if (USE_DEVICE_NODE == 1)
	btmtk_sp_close();

	__pm_relax(bt_trx_wakelock);

	if (!cif_dev->is_pre_cal) {
		int ret = 0;
		ret = connv3_pwr_off(CONNV3_DRV_TYPE_BT);
		if (ret)
			BTMTK_ERR("%s: ConnInfra power off failed, ret[%d]", __func__, ret);
	}
	btmtk_pwrctrl_post_off();
#endif
	BTMTK_INFO("%s end!", __func__);

	return 0;
}

static int btmtk_send_to_tx_queue(struct btmtk_uart_dev *cif_dev, struct sk_buff *skb)
{
	ulong flags = 0;

	/* error handle */
	if (!atomic_read(&cif_dev->thread_status)) {
		BTMTK_WARN("%s tx thread already stopped, don't send cmd anymore!!", __func__);
		/* Removed kfree_skb: leave free to btmtk_main_send_cmd */
		return -1;
	}

	spin_lock_irqsave(&cif_dev->tx_lock, flags);
	skb_queue_tail(&cif_dev->tx_queue, skb);
	spin_unlock_irqrestore(&cif_dev->tx_lock, flags);
	wake_up_interruptible(&tx_wait_q);

	return 0;
}

int btmtk_uart_send_cmd(struct btmtk_dev *bdev, struct sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;

	if (bdev == NULL || skb == NULL) {
		BTMTK_ERR("bdev or skb is NULL");
		ret = -1;
		return ret;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("cif_dev is NULL, bdev=%p", bdev);
		ret = -1;
		return ret;
	}

	/* send pkt direct or not */
	/* if want to use send_and_recv cmd in tx_thread would not be able to send the pkt */
	if (pkt_type == BTMTK_TX_PKT_SEND_DIRECT || pkt_type == BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT) {
		BTMTK_DBG("%s send pkt direct, not queue in tx queue ", __func__);
		ret = btmtk_cif_send_cmd(bdev, skb->data, skb->len, delay, retry);

		/* in normal case, cif_send success would kfree_skb in tx_thread */
		/* but in this case, would not pass by tx_thread, so need not kfree_skb */
		if (ret >= 0 && skb) {
			kfree_skb(skb);
			skb = NULL;
		}
	} else
		ret = btmtk_send_to_tx_queue(cif_dev, skb);

	return ret;
}

static int btmtk_uart_read_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	int ret = 0;
#if (USE_DEVICE_NODE == 0)
	struct data_struct cmd, event;

	btmtk_get_cmd_or_event(bdev, READ_REGISTER_CMD, &cmd);
	btmtk_get_cmd_or_event(bdev, READ_REGISTER_EVT, &event);

	BTMTK_INFO("%s: read cr %x", __func__, reg);

	memcpy(&cmd.content[MCU_ADDRESS_OFFSET_CMD], &reg, sizeof(reg));

	ret = btmtk_main_send_cmd(bdev,
			cmd.content, cmd.len, event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	memcpy(val, bdev->io_buf + MCU_ADDRESS_OFFSET_EVT - HCI_TYPE_SIZE, sizeof(u32));
	*val = le32_to_cpu(*val);

	BTMTK_INFO("%s: reg=%x, value=0x%08x", __func__, reg, *val);
#endif
	return ret;
}

#if (USE_DEVICE_NODE == 0)
static int btmtk_uart_write_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	int ret = 0;

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
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);

	return ret;
}
#endif

int btmtk_uart_event_filter(struct btmtk_dev *bdev, struct sk_buff *skb)
{
#if (USE_DEVICE_NODE == 0)
	const u8 read_address_event[READ_ADDRESS_EVT_HDR_LEN] = { 0x4, 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00 };
	const u8 get_baudrate_event[GETBAUD_EVT_LEN] = { 0x04, 0xE4, 0x0A, 0x02, 0x04, 0x06, 0x00, 0x00, 0x02 };
#else
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
#endif

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE &&
		skb->len >= event_need_compare_len) {
		memset(bdev->io_buf, 0, IO_BUF_SIZE);
#if (USE_DEVICE_NODE == 1)
		/* cmd from stack would pass 0x01, 0xXX, 0xXX as event_need_compare */
		if (event_need_compare_len == 2 && skb->len > 5 &&
				memcmp(&skb->data[3], event_need_compare, event_need_compare_len) == 0) {
			BTMTK_INFO("%s: compare opcode[0x%02X%02X] from stack success",
					__func__, skb->data[4], skb->data[3]);
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			/* return 0 not drop event by driver */
			return 0;
		}
		if (skb->data[2] == 0x02 && skb->data[3] == 0x18) {
			int ret = 0;
			BTMTK_INFO("match DFD event");
			if (skb->data[6] == 0x00) {
				event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
				memcpy(bdev->dfd_pstd, &skb->data[7], skb->len - 7);
				ret = connv3_coredump_send(bmain_info->hif_hook.coredump_handler, "PSTD", &g_sbdev->dfd_pstd[0], skb->len - 7);
				if (ret)
					BTMTK_ERR("%s: connv3_coredump_send fail, ret = %d", __func__, ret);
				return 1;
			} else {
				BTMTK_ERR("%s compare fail!!!", __func__);
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: skb->data:", __func__);
			}
		}
#else // (USE_DEVICE_NODE == 0)
		if ((skb->len == (GETBAUD_EVT_LEN - HCI_TYPE_SIZE + BAUD_SIZE)) &&
			memcmp(skb->data, &get_baudrate_event[1], GETBAUD_EVT_LEN - 1) == 0) {
			BTMTK_INFO("%s: GET BAUD = %02X %02X %02X, FC = %02X", __func__,
				skb->data[10], skb->data[9], skb->data[8], skb->data[11]);
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
		} else if ((skb->len == (READ_ADDRESS_EVT_HDR_LEN - HCI_TYPE_SIZE + BD_ADDRESS_SIZE)) &&
					memcmp(skb->data, &read_address_event[1], READ_ADDRESS_EVT_HDR_LEN - 1) == 0) {
			memcpy(bdev->bdaddr, &skb->data[READ_ADDRESS_EVT_PAYLOAD_OFFSET - 1], BD_ADDRESS_SIZE);
			BTMTK_DBG("%s: GET BDADDR = "MACSTR, __func__, MAC2STR(bdev->bdaddr));
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;

			/* SP project need to send to stack */
			//return 0;
		} else
#endif
		if (memcmp(skb->data, event_need_compare,
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
#if (USE_DEVICE_NODE == 0)
			msleep(IO_BUF_DELAY_TIME);
#endif
			bdev->recv_evt_len = skb->len;
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			BTMTK_DBG("%s, compare success", __func__);
		} else {
			BTMTK_INFO("%s compare fail", __func__);
			BTMTK_INFO_RAW(event_need_compare, event_need_compare_len,
				"%s: event_need_compare_len[%d]", __func__, event_need_compare_len);
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: skb->data:", __func__);
			return 0;
		}
		return 1;
	}

	return 0;
}

int btmtk_uart_send_and_recv(struct btmtk_dev *bdev,
		struct sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag)
{
	unsigned long comp_event_timo = 0, start_time = 0;
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	u8 opcode[2] = {0};

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT) {
		BTMTK_ERR("%s: BTMTK_STATE_DISCONNECT", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

#if (SLEEP_ENABLE == 1)
	/* update fw own timer or wait fw own done then do drv own */
	/* this case is not called by tx_thread(fw own/drv own) */
	if (pkt_type != BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT) {
		/* normal case for keep drv own
		 * rhw dump (wifi, connv3) would use this to do drv own
		 */
		ret = btmtk_uart_driver_own(bdev);
		if (ret < 0) {
			BTMTK_ERR("%s: driver own failed, return", __func__);
			bmain_info->hif_hook.trigger_assert(bdev);
			return -1;
		}
		/* for cancel fw own if fw own timer just complete */
		atomic_set(&cif_dev->need_drv_own, 1);
	}
#endif
	btmtk_hci_snoop_save(HCI_SNOOP_TYPE_CMD_HIF, skb->data, skb->len);
	BTMTK_DBG_RAW(skb->data, skb->len, "%s: len[%d]", __func__, skb->len);

	/* if just protect event, another cmd would reinit event_compare_status */
	down(&cif_dev->evt_comp_sem);

	if (event) {
		if (event_len > EVENT_COMPARE_SIZE || event_len <= 1) {
			BTMTK_ERR("%s, invalid event_len (%d)", __func__, event_len);
			up(&cif_dev->evt_comp_sem);
			return -1;
		}

		event_compare_status = BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE;
		memcpy(event_need_compare, event + 1, event_len - 1);
		event_need_compare_len = event_len - 1;

		/* if send cmd without drv own, not direct send cmd incase of tx_thread cant not do drv own with send_and_recv */
		if (pkt_type != BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT &&
			(cif_dev->own_state != BTMTK_DRV_OWN ||
			atomic_read(&cif_dev->fw_own_timer_flag) == FW_OWN_TIMER_RUNNING ||
			atomic_read(&cif_dev->fw_own_timer_flag) == FW_OWN_TIMER_DONE)) {

			BTMTK_WARN("%s: wait driver own retry, own_state[%d]", __func__, cif_dev->own_state);
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
			up(&cif_dev->evt_comp_sem);
			return -EAGAIN;
		}

		start_time = jiffies;
		/* check hci event /wmt event for uart/UART interface, check hci
		 * event for USB interface
		 */
#if (USE_DEVICE_NODE == 0)
		comp_event_timo = jiffies + msecs_to_jiffies(WOBLE_COMP_EVENT_TIMO);
#else
		comp_event_timo = jiffies + msecs_to_jiffies(WOBLE_EVENT_INTERVAL_TIMO);
#endif
		BTMTK_DBG("event_need_compare_len %d, event_compare_status %d",
			event_need_compare_len, event_compare_status);
	} else {
		event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
	}

#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
	if (btmtk_get_chip_state(bdev) != BTMTK_STATE_DISCONNECT)
		mtk8250_uart_start_record(cif_dev->tty);
#endif
	if (skb->len > 2) {
		opcode[0] = skb->data[1];
		opcode[1] = skb->data[2];
	}
	ret = btmtk_uart_send_cmd(bdev, skb, delay, retry, pkt_type, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_cmd failed!!", __func__);
		goto exit;
	}

#if (USE_DEVICE_NODE == 1)
	/* 4 round and dump cif status each round (500ms), total 2 secs */
	do {
#endif
		do {
			ret = -1;

			/* check if event_compare_success */
			if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
				ret = 0;
				break;
			}

			/* if during re-download, not wait hci reset cmd/blank cmd.
			 * Cuz in test mode, tool would use hci reset cmd to trigger re-download */
			if (bdev->dynamic_fwdl_start &&
				((opcode[0] == 0x03 && opcode[1] == 0x0C) || (opcode[0] == 0x5D && opcode[1] == 0xFC))) {
				BTMTK_WARN("%s: hci reset cmd trigger re-download, don't wait evt, opecode[0x%02x%02x]",
							__func__, opcode[1], opcode[0]);
				ret = 0;
				break;
			}

			/* error handle*/
			if (btmtk_get_chip_state(bdev) == BTMTK_STATE_FW_DUMP || !atomic_read(&cif_dev->thread_status)) {
				BTMTK_WARN("%s thread stopped or fw dumping, don't wait evt anymore!!", __func__);
				ret = -2;
				break;
			}
#if (USE_DEVICE_NODE == 1)
			usleep_range(1000, 1100);
#else
			usleep_range(10, 100);
#endif
	} while (time_before(jiffies, comp_event_timo));
#if (USE_DEVICE_NODE == 1)
		if (ret != -1)	/* successfully received event or coredump case */
			break;
#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
		if (btmtk_get_chip_state(bdev) != BTMTK_STATE_DISCONNECT)
			mtk8250_uart_end_record(cif_dev->tty);
#endif
		comp_event_timo = jiffies + msecs_to_jiffies(WOBLE_EVENT_INTERVAL_TIMO);
	} while (--retry > 0);
#endif

	if (ret < 0) {
		BTMTK_ERR("%s wait event timeout [0x%02x%02x], ret[%d]",
				__func__,opcode[1], opcode[0], ret);
		bdev->recv_evt_len = 0;
		ret = -ERRNUM;
	}


exit:
	event_compare_status = BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
	up(&cif_dev->evt_comp_sem);
	/* control not trigger assert */
	if (ret < 0 && pkt_type != BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT
			&& pkt_type != BTMTK_TX_PKT_SEND_NO_ASSERT) {
		if(bmain_info->hif_hook.trigger_assert) {
			if (bdev->assert_reason[0] == '\0') {
				if (snprintf(bdev->assert_reason, ASSERT_REASON_SIZE , "[BT_DRV assert] cmd timeout 0x%02x%02x"
						,opcode[1], opcode[0]) < 0)
					strncpy(bdev->assert_reason, "[BT_DRV assert] cmd timeout",
						strlen("[BT_DRV assert] cmd timeout") + 1);
				BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
			}
			bmain_info->hif_hook.trigger_assert(bdev);
		} else
			btmtk_send_assert_cmd(bdev);
	}

	return ret;
}

int btmtk_uart_send_enable_ctxrtx(struct hci_dev *hdev)
{
	u8 cmd[] = {0x01, 0x6F, 0xFC, 0x2C, 0x01, 0x08, 0x28, 0x00,
				0x01, 0x01, 0x00, 0x03, 0x10, 0x00, 0x09, 0x80,
				0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
				0x34, 0x53, 0x00, 0x70, 0x00, 0x00, 0x02, 0x00,
				0xFF, 0xFF, 0xFF, 0xFF, 0x44, 0x53, 0x00, 0x70,
				0x00, 0x00, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	u8 evt[] = {0x04, 0xE4, 0x08, 0x02, 0x08, 0x04, 0x00, 0x00,
				0x00, 0x00, 0x03};
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;

	ret = btmtk_main_send_cmd(bdev, cmd, sizeof(cmd), evt, sizeof(evt), DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV,CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

int btmtk_uart_send_set_uart_cmd(struct hci_dev *hdev, struct UART_CONFIG *uart_cfg)
{
	u8 baud_115200[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0x01, 0xC2, 0x01, 0x00, 0x03 };
	u8 baud_921600[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0x00, 0x10, 0x0E, 0x00, 0x03 };
	u8 baud_3M[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0xC0, 0xC6, 0x2D, 0x00, 0x03 };
	u8 baud_4M[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0x00, 0x09, 0x3D, 0x00, 0x03 };
	u8 baud_8M[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0x00, 0x12, 0x7A, 0x00, 0x03 };
	u8 baud_10M[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0x80, 0x96, 0x98, 0x00, 0x03 };
	u8 baud_12M[] = { 0x01, 0x6F, 0xFC, 0x0A, 0x01, 0x04,
				 0x06, 0x00, 0x01, 0x00, 0x1B, 0xB7, 0x00, 0x03 };
	u8 baud_24M[] = { 0x01, 0x6F, 0xFC, 0x0B, 0x01, 0x04,
				 0x07, 0x00, 0x01, 0x00, 0x36, 0x6E, 0x01, 0x00, 0x03 };
	u8 event[] = {0x04, 0xE4, 0x06, 0x02, 0x04, 0x02, 0x00, 0x00, 0x01};
	u8 *cmd = NULL;
        u8 cmd_len = SETBAUD_CMD_LEN;
	u8 fc_offset = BT_FLOWCTRL_OFFSET;
	u8 hub_crc_rhw_offset = BT_HUB_CRC_RHW_OFFSET;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;

	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is NULL", __func__);
		return -EINVAL;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	switch (uart_cfg->iBaudrate) {
	case 921600:
		cmd = baud_921600;
		break;
	case 3000000:
		cmd = baud_3M;
		break;
	case 4000000:
		cmd = baud_4M;
		break;
	case 8000000:
		cmd = baud_8M;
		break;
	case 10000000:
		cmd = baud_10M;
		break;
	case 12000000:
		cmd = baud_12M;
		break;
	case 24000000:
		cmd = baud_24M;
		fc_offset++;
		hub_crc_rhw_offset++;
		cmd_len++;
		break;
	default:
		/* default chip baud is 115200 */
		cmd = baud_115200;
		//return 0;
		break;
	}

	switch (uart_cfg->fc) {
	case UART_HW_FC:
		cmd[fc_offset] = BT_HW_FC;
		break;
	case UART_MTK_SW_FC:
	case UART_LINUX_FC:
		cmd[fc_offset] = BT_SW_FC;
		break;
	default:
		/* default disable flow control */
		cmd[fc_offset] = BT_NONE_FC;
	}

#if (USE_DEVICE_NODE == 1)
	/* uarthub setting
	 * ex: thes last byte means hub enable, rhw disable, crc disable
	 * DX-4: need to tell connsys using new/old sleep flow
	 */
	if (!cif_dev->sleep_flow_hw_mech_en) {
		BTMTK_INFO("%s sleep_flow_hw_mech_en: %d, old sleep flow", __func__, cif_dev->sleep_flow_hw_mech_en);
		cif_dev->fw_hub_mode = 3;
	}
#endif

	cmd[hub_crc_rhw_offset] = (cif_dev->fw_hub_mode << 4 | !cif_dev->rhw_en << 1 | !cif_dev->crc_en << 0);

	ret = btmtk_main_send_cmd(bdev,
			cmd, cmd_len, event, SETBAUD_EVT_LEN, 0,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		return ret;
	}

	cif_dev->uart_baudrate_set = 1;
	BTMTK_INFO("%s done, fc_offset[%d], hub_crc_rhw_offset[%d]", __func__, fc_offset, hub_crc_rhw_offset);

	return 0;
}

static int btmtk_uart_send_query_uart_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x04, 0x01, 0x00, 0x02};
	u8 event[] = { 0x04, 0xE4, 0x0a, 0x02, 0x04, 0x06, 0x00, 0x00, 0x02};
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;

	ret = btmtk_main_send_cmd(bdev, cmd, GETBAUD_CMD_LEN, event, GETBAUD_EVT_LEN, DELAY_TIMES,
#if (USE_DEVICE_NODE == 0)
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
#else
			RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
#endif
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_query_uart_cmd failed!!", __func__);
		return ret;
	}

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_uart_read_chip_id_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x04, 0x01, 0x00, 0x09};
	u8 event[] = {0x04, 0xE4};
	// u8 event[] = {0x04, 0xE4, 0x0A, 0x02, 0x04, 0x06, 0x00, 0x00, 0x09, 0x39, 0x66, 0x00, 0x00};
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;

	ret = btmtk_main_send_cmd(bdev, cmd, sizeof(cmd), event, sizeof(event), DELAY_TIMES,
#if (USE_DEVICE_NODE == 0)
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
#else
			RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
#endif
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_read_chip_id_cmd failed, set bdev->chip_id to 0x6653", __func__);
		/* temp set to 0x6653 wheh get chip id failed, this command may failed on FPGA */
		bdev->chip_id = 0x6653;
		return ret;
	}

	bdev->chip_id = (bdev->io_buf[9] == 0x39) ? 0x6639 : 0x6653;
	BTMTK_INFO("%s done, bdev->chip_id = 0x%04x", __func__, bdev->chip_id);
	return ret;
}

int btmtk_uart_send_wakeup_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6f, 0xfc, 0x01, 0xFF };
	/* event before fw dl */
	u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x03};
#if (USE_DEVICE_NODE == 0)
	/* For ce, can't judge load patch or not now */
	u8 event_partial[] = { 0x04, 0xE4 };
#else
	/* event after fw dl */
	u8 event2[] = { 0x04, 0xE4, 0x07, 0x02, 0x03, 0x03, 0x00, 0x00, 0x03, 0x01 };
#endif
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev->uart_baudrate_set == 0) {
		BTMTK_INFO("%s uart baudrate is 115200, no need", __func__);
		return 0;
	}
	if (is_mt6639(bdev->chip_id) || is_mt66xx(bdev->chip_id)) {
#if (USE_DEVICE_NODE == 0)
		ret = btmtk_main_send_cmd(bdev, cmd + 4, 1, event_partial, 2,
					0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
#else
		if (!cif_dev->fw_dl_ready && bdev->chip_id == 0x6639)
			ret = btmtk_main_send_cmd(bdev, cmd + 4, 1, event, WAKEUP_EVT_LEN,
					0, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
		else
			ret = btmtk_main_send_cmd(bdev, cmd + 4, 1, event2, WAKEUP_EVT_LEN + 1,
					0, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
#endif
	} else
		ret = btmtk_main_send_cmd(bdev, cmd, WAKEUP_CMD_LEN, event, WAKEUP_EVT_LEN,
				0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		return ret;
	}

	BTMTK_INFO("%s done, chip_id = 0x%x", __func__, bdev->chip_id);
	return ret;
}
#if (USE_DEVICE_NODE == 1)
static int btmtk_uart_send_xo_param_cmd(struct hci_dev *hdev, CONNXO_CFG_PARAM_STRUCT *connxo_cfg_param)
{
	uint8_t *cmd = NULL;
	// 0x47 = OP + Length + flag + + Offset + data(64 Bytes)
	uint8_t cmd_header[] = {0x01, 0x6F, 0xFC, 0x47};
	uint16_t offset = 0;
	//u8 cmd[] = { 0x01, 0x6F, 0xFC, 0xXX, 0x01, 0x19, 0xXX, 0x00, 0x00, 0x00, 0x00};
	/* To-do: event payload */
	u8 event[] = { 0x04, 0xE4 };
	u8 cmd_len = cmd_header[3] + sizeof(cmd_header);
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = -1;
	uint8_t *data = NULL;
	uint32_t size = 0;

	cmd = vmalloc(HCI_MAX_ACL_SIZE);
	if (cmd == NULL) {
		BTMTK_ERR("%s: vmalloc failed!!", __func__);
		return -1;
	}

	memcpy(cmd, cmd_header, sizeof(cmd_header));
	offset = sizeof(cmd_header);

	// OP code
	cmd[offset++] = 0x01;
	cmd[offset++] = 0x19;

	// Length: 2 bytes, cmd_header[3] - 4
	cmd[offset++] = 0x43;
	cmd[offset++] = 0x00;

	// flag
	cmd[offset++] = 0x01;

	// Offset
	cmd[offset++] = 0x00;
	cmd[offset++] = 0x00;

	memcpy(&cmd[offset], &connxo_cfg_param->ucFreqC1Axm52m, 25);

	data = connv3_get_plat_config(&size);
	if (data == NULL) {
		BTMTK_ERR("%s: data is NULL", __func__);
		ret = -1;
		goto exit;
	}

	BTMTK_INFO("%s: 0x%02x, 0x%02x, 0x%02x, 0x%02x", __func__, data[0], data[1], data[2], data[3]);
	// platform customized config, total 64 bytes, 60~63 bytes
	memcpy(&cmd[offset + 60], data, 4);

	ret = btmtk_main_send_cmd(bdev, cmd, cmd_len, event, sizeof(event), DELAY_TIMES,
#if (USE_DEVICE_NODE == 0)
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
#else
			RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		vfree(cmd);
		return ret;
	}
#endif
	BTMTK_INFO("%s done", __func__);
exit:
	vfree(cmd);
	return ret;
}
#endif

#if (USE_DEVICE_NODE == 0)
static int btmtk_uart_subsys_reset(struct btmtk_dev *bdev)
{
	struct ktermios new_termios;
	struct tty_struct *tty;
	struct UART_CONFIG uart_cfg;
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	uart_cfg = cif_dev->uart_cfg;
	tty = cif_dev->tty;
	new_termios = tty->termios;

	/* RST pin must be defined in dts for connac3 */
	/* Maybe can be moved into bt.cfg ? */
	if (is_connac3(bdev->chip_id)) {
		btmtk_ce_subsys_reset(bdev);
	} else {
		BTMTK_INFO("%s tigger reset pin: %d", __func__, bdev->bt_cfg.dongle_reset_gpio_pin);
		gpio_set_value(bdev->bt_cfg.dongle_reset_gpio_pin, 0);
		msleep(SUBSYS_RESET_GPIO_DELAY_TIME);
		gpio_set_value(bdev->bt_cfg.dongle_reset_gpio_pin, 1);
		/* Basically, we need to polling the cr (BT_MISC) untill the subsys reset is completed
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
	new_termios.c_iflag &= ~(NOFLSH|CRTSCTS);
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
		new_termios.c_iflag |= CRTSCTS;
		new_termios.c_cflag &= ~(NOFLSH);
		break;
	/* default disable flow control */
	default:
		new_termios.c_cflag &= ~(CRTSCTS);
		new_termios.c_iflag &= ~(NOFLSH|CRTSCTS);
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


#else // (USE_DEVICE_NODE == 1)
static int btmtk_uart_subsys_reset(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s trigger connv3_conninfra_bus_dump", __func__);
	/* cant not put at rx thread, would deadlock to get event */
	connv3_conninfra_bus_dump(CONNV3_DRV_TYPE_BT);
	return 0;
}

void btmtk_uart_trigger_assert_by_tx_thread(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s: start", __func__);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}

	atomic_set(&cif_dev->need_assert, 1);
	wake_up_interruptible(&tx_wait_q);
}

static void btmtk_uart_trigger_assert(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int state = BTMTK_STATE_INIT, ret = 0;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	state = btmtk_get_chip_state(bdev);
	fstate = btmtk_fops_get_state(bdev);

	if (state == BTMTK_STATE_DISCONNECT) {
		BTMTK_WARN("%s: uart disconnected, complete dump_comp", __func__);
		/* if uart disconnected during coredump, no need to wait */
		complete_all(&bdev->dump_comp);
		return;
	}

	BTMTK_INFO("%s tty_port[%p]", __func__, cif_dev->tty->port);

#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
	mtk8250_uart_dump(cif_dev->tty);
#endif

	/* driver dump */
	btmtk_hci_snoop_print_to_log();
	btmtk_dump_gpio_state();

#if (SLEEP_ENABLE == 1)
	/* incase do fw own in debug sop flow */
	btmtk_uart_delete_fw_own_timer(cif_dev);
#endif

	if (state == BTMTK_STATE_INIT || fstate == BTMTK_FOPS_STATE_CLOSED
		|| fstate == BTMTK_FOPS_STATE_CLOSING || atomic_read(&bdev->assert_state)) {
		BTMTK_WARN("%s: state[%d] fstate[%d] bt assert_state[%d], not trigger coredump",
				__func__, state, fstate, atomic_read(&bdev->assert_state));
		return;
	}

	/* set this bt on is already asserted, not trigger assert anymore */
	BTMTK_INFO("%s: set bt assert_state[1]", __func__);
	atomic_set(&bdev->assert_state, BTMTK_ASSERT_START);

	if (cif_dev->rhw_en == 0) {
		/* not enable rhw yet, do hif dump */
		if (bmain_info->hif_hook.dump_hif_debug_sop)
			bmain_info->hif_hook.dump_hif_debug_sop(bdev);
		return;
	}

	/* dump debug sop before coredump */
	if (bmain_info->hif_hook.dump_debug_sop)
		bmain_info->hif_hook.dump_debug_sop(bdev);

	/* incase of fw dump happened during rhw debug sop
	 * then would trigger hif debug sop
	 */

	/* rhw already do driver own
	 * not through tx_thread for block before set is_whole_chip_reset
	 */
	BTMTK_WARN("%s: trigger assert", __func__);
	ret = btmtk_send_assert_cmd(bdev);

	if (ret < 0) {
		BTMTK_WARN("%s: trigger assert failed, ret[%d]", __func__, ret);
		/* hif dump */
		if (bmain_info->hif_hook.dump_hif_debug_sop)
			bmain_info->hif_hook.dump_hif_debug_sop(bdev);

		/* if during btmtk_pre_chip_rst_handler (BTMTK_RESET_DOING)
		 * leave hw_err to btmtk_post_chip_rst_handler
		 */
		if (atomic_read(&bmain_info->chip_reset) == BTMTK_RESET_DONE) {
			/* direct send hw_err event notify host to close bt */
			bmain_info->reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
			btmtk_send_hw_err_to_host(bdev);
		}
		return;
	}

}

static int btmtk_uart_driver_own_cmd(struct btmtk_dev *bdev)
{
	u8 fw_own_clr_cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x03, 0x02, 0x00, 0x03, 0x01 };
	u8 evt[] = { 0x04, 0xE4, 0x07, 0x02, 0x03, 0x03, 0x00, 0x00, 0x03, 0x01 };
	int ret = 0;

	ret = btmtk_main_send_cmd(bdev, fw_own_clr_cmd, 10, evt, OWNTYPE_EVT_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed, ret[%d]", __func__, ret);
	return ret;
}

static int btmtk_sp_pre_open(struct btmtk_dev *bdev)
{
	struct ktermios new_termios;
	struct tty_struct *tty;
	struct UART_CONFIG uart_cfg;
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	int query_retry = 3;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	uart_cfg = cif_dev->uart_cfg;
	tty = cif_dev->tty;
	new_termios = tty->termios;

	btmtk_pwrctrl_pre_on(bdev);

	BTMTK_INFO("%s: bdev->is_dfd_done: %d, bdev->reset_type = %d", __func__, bdev->is_dfd_done, bdev->reset_type);
	if (!cif_dev->is_pre_cal && (bdev->is_dfd_done || bdev->reset_type != CONNV3_CHIP_RST_TYPE_DFD_DUMP)) {
		ret = connv3_pwr_on(CONNV3_DRV_TYPE_BT);
		if(ret) {
			BTMTK_ERR("%s: ConnInfra power on failed, ret[%d]", __func__, ret);
			if(ret == CONNV3_ERR_RST_ONGOING) {
				bdev->on_fail_count = 0;
				return CONNV3_ERR_RST_ONGOING;
			} else
				return -EFAULT;
		}
	}

	/* start tx_thread */
	if (btmtk_tx_thread_start(bdev))
		return -EFAULT;

	/* temp solution wait pmic enable */
	msleep(100);

	btmtk_set_uart_rx_aux();
	btmtk_dump_gpio_state();

	if (connv3_ext_32k_on()) {
		BTMTK_ERR("connv3_ext_32k_on failed!");
		return -EFAULT;
	}

	if (btmtk_get_chip_state(g_sbdev) == BTMTK_STATE_DISCONNECT) {
		BTMTK_WARN("%s: uart disconnected", __func__);
		return -1;
	}

	/* reinit state */
	BTMTK_INFO("%s: init bt assert_state[0], dump_comp", __func__);
	atomic_set(&bmain_info->chip_reset, BTMTK_RESET_DONE);
	atomic_set(&bmain_info->subsys_reset, BTMTK_RESET_DONE);
	bmain_info->chip_reset_flag = 0;
	atomic_set(&bdev->assert_state, BTMTK_ASSERT_NONE);
	cif_dev->rhw_fail_cnt = 0;
	reinit_completion(&bdev->dump_comp);
	reinit_completion(&bdev->dump_dfd_comp);
	bdev->is_whole_chip_reset = FALSE;
	bmain_info->dbg_send = 0;

	/* set tty host baud and flowcontrol to default value */
	BTMTK_INFO("Set default baud: %d, disable flowcontrol", BT_UART_DEFAULT_BAUD);
	tty_termios_encode_baud_rate(&new_termios, BT_UART_DEFAULT_BAUD, BT_UART_DEFAULT_BAUD);
	new_termios.c_cflag &= ~(CRTSCTS);
	new_termios.c_iflag &= ~(NOFLSH|CRTSCTS);
	tty_set_termios(tty, &new_termios);

	/* update baurdrate from dts */
	if (cif_dev->baudrate)
		uart_cfg.iBaudrate = cif_dev->baudrate;

	/* uarhub setting */
	cif_dev->fw_hub_mode = 0;
	cif_dev->rhw_en = 0;
	cif_dev->crc_en = 0;
	cif_dev->fw_dl_ready = 0;
	cif_dev->flush_en = 1;
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	cif_dev->sleep_flow_hw_mech_en = (mtk8250_uart_hub_get_bt_sleep_flow_hw_mech_en() <= 0) ? 0 : mtk8250_uart_hub_get_bt_sleep_flow_hw_mech_en();
	BTMTK_INFO("sleep_flow_hw_mech_en: %d", cif_dev->sleep_flow_hw_mech_en);
#endif
	/* Flush any pending characters in the driver and discipline. */
	tty_ldisc_flush(tty);

	/* If DFD, return */
	if (bdev->reset_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP && !bdev->is_dfd_done) {
		/* set is_dfd_done */
		bdev->is_dfd_done = TRUE;
		BTMTK_INFO("%s: DFD done, return", __func__);
		return 0;
	}

	/* for dfd, clear assert reason here */
	memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);

	/* query cmd, make sure can communicate with fw */
	do {
		/* clean rx queues */
		skb_queue_purge(&bdev->rx_q);
		if (!IS_ERR_OR_NULL(bdev->rx_skb))
			kfree_skb(bdev->rx_skb);
		bdev->rx_skb = NULL;

		ret = btmtk_uart_send_query_uart_cmd(bdev->hdev);
	} while (ret < 0 && query_retry --);
	if (ret < 0) {
		btmtk_uart_trigger_assert(bdev);
		goto exit;
	}

	if (uart_cfg.fc == UART_HW_FC) {
		BTMTK_INFO("support HW flow control");
		ret = btmtk_uart_send_enable_ctxrtx(bdev->hdev);
		if (ret < 0)
			goto exit;
	}

	/* read chip id */
	ret = btmtk_uart_read_chip_id_cmd(bdev->hdev);
	if (ret < 0)
		BTMTK_ERR("%s: btmtk_uart_read_chip_id_cmd failed", __func__);

	/* set chip baud and flowcontrol to config setting */
	ret = btmtk_uart_send_set_uart_cmd(bdev->hdev, &uart_cfg);
	if (ret < 0)
		goto exit;

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
		new_termios.c_iflag |= CRTSCTS;
		new_termios.c_cflag &= ~(NOFLSH);
		break;
	/* default disable flow control */
	default:
		new_termios.c_cflag &= ~(CRTSCTS);
		new_termios.c_iflag &= ~(NOFLSH|CRTSCTS);
	}

	tty_set_termios(tty, &new_termios);

	ret = btmtk_uart_send_wakeup_cmd(bdev->hdev);
	if (ret < 0)
		goto exit;

	/* send xo cal cmd */
	ret = btmtk_uart_send_xo_param_cmd(bdev->hdev, &sConnxo_cfg);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_uart_send_xo_param_cmd fail", __func__);
	}

	ret = btmtk_load_rom_patch(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_load_rom_patch fail", __func__);
		goto exit;
	}

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en && !cif_dev->hub_bypass_only) {
		/* uarhub setting */
		cif_dev->fw_hub_mode = 1;
		cif_dev->crc_en = 1;
	} else
		BTMTK_INFO("%s hub_bypass_only, not send hub_en to fw", __func__);
#endif

	cif_dev->fw_dl_ready = 1;
	cif_dev->rhw_en = 1;

	/* set chip baud and flowcontrol to config setting */
	ret = btmtk_uart_send_set_uart_cmd(bdev->hdev, &uart_cfg);
	if (ret < 0) {
		BTMTK_ERR("%s after fwdl, send uarhub setting cmd fail", __func__);
		goto exit;
	}

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en && !cif_dev->hub_bypass_only) {
		/* after fw dl, use uarthub multi-host mode */
		ret = mtk8250_uart_hub_enable_bypass_mode(0);
		BTMTK_INFO("%s after fw dl, mtk8250_uart_hub_enable_bypass_mode(0) ret[%d]", __func__, ret);
	} else
		BTMTK_INFO("%s: hub_bypass_only, not set mtk8250_uart_hub_enable_bypass_mode(0)", __func__);
#endif

	ret = btmtk_uart_send_wakeup_cmd(bdev->hdev);
	if (ret < 0) {
		/* err handle for fw get dirty data trigger EINT */
		ret = btmtk_uart_driver_own_cmd(bdev);
		if (ret < 0)
			goto exit;
	}

	/* bt on success, reset subsys count */
	atomic_set(&bmain_info->subsys_reset_conti_count, 0);
	bdev->reset_type = 0;

	BTMTK_DBG("%s done", __func__);

exit:
	if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT
		|| btmtk_get_chip_state(bdev) == BTMTK_STATE_FW_DUMP)
		return ret;

	cif_event = HIF_EVENT_PROBE;
	cif_state = &bdev->cif_state[cif_event];
	/* Set End/Error state */
	if (ret == 0) {
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	} else {
		BTMTK_ERR("%s: btmtk_load_rom_patch failed (%d)", __func__, ret);
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
	}

	return ret;
}

#endif //(USE_DEVICE_NODE)

static int btmtk_uart_pre_open(struct btmtk_dev *bdev)
{
	int ret = 0;
#if (SLEEP_ENABLE == 1)
	struct btmtk_uart_dev *cif_dev = NULL;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	BTMTK_DBG("%s init to driver own state", __func__);
	/* not start fw_own_timer until bt open done */
	atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_UKNOWN);
	__pm_stay_awake(bt_trx_wakelock);
	cif_dev->own_state = BTMTK_DRV_OWN;
#endif

#if (USE_DEVICE_NODE == 1)
	ret = btmtk_sp_pre_open(bdev);
#endif
	return ret;
}

static void btmtk_uart_open_done(struct btmtk_dev *bdev)
{

	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s start", __func__);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

#if (USE_DEVICE_NODE == 1)
	if (!cif_dev->is_pre_cal) {
		int ret = 0;
		ret = connv3_pwr_on_done(CONNV3_DRV_TYPE_BT);
		if (ret)
			BTMTK_ERR("%s: ConnInfra power done failed, ret[%d]", __func__, ret);
	}
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en) {
		if (!cif_dev->hub_bypass_only) {
			int ret = 0;
			/* enable ADSP,MD when fw dl done*/
			ret = mtk8250_uart_hub_fifo_ctrl(0);
			BTMTK_DBG("%s: Set mtk8250_uart_hub_fifo_ctrl(0) ret[%d]", __func__, ret);

			/* 27. Set DEV0_HOST_AWAKE to 1 */
			if (cif_dev->sleep_flow_hw_mech_en) {
				int ret = 0;
				ret = mtk8250_uart_hub_set_host_awake_sta(0);
				BTMTK_DBG("%s: Set DEV0_HOST_AWAKE ret[%d]", __func__, ret);

				/* 28. If DEV0_BT_AWAKE is 0, send 0xFF */
				if (mtk8250_uart_hub_get_host_bt_awake_sta(0) == 0) {
					u8 wakeup_cmd[] = { 0xFF };
					ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, NULL, 0, DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
					if (ret < 0)
						BTMTK_ERR("%s btmtk_main_send_cmd 0xFF fail when DEV0_BT_AWAKE is 0, ret[%d]", __func__, ret);
				}
			}
		} else
			BTMTK_INFO("%s: hub_bypass_only, not set mtk8250_uart_hub_fifo_ctrl(0)", __func__);
		mtk8250_uart_hub_assert_bit_ctrl(0);
		BTMTK_INFO("%s mtk8250_uart_hub_assert_bit_ctrl(0)", __func__);
	}
#endif
	btmtk_read_pmic_state(bdev);
#endif

#if (SLEEP_ENABLE == 1)
	/* start fw own timer */
	btmtk_uart_create_fw_own_timer(cif_dev);
#endif

}


static void btmtk_uart_waker_notify(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s enter!", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}
	schedule_work(&bdev->reset_waker);
}

static int btmtk_uart_set_para(struct btmtk_dev *bdev, int val)
{
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s start val[%d]", __func__, val);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_INFO("%s:[old] hub_en[%d] sleep_en[%d] hub_bypass_only[%d]", __func__
				, cif_dev->hub_en, cif_dev->sleep_en, cif_dev->hub_bypass_only);

	cif_dev->hub_en = !!(val & BTMTK_HUB_EN);
	cif_dev->sleep_en = !!(val & BTMTK_SLEEP_EN);
	cif_dev->hub_bypass_only = !!(val & BTMTK_UARTHUB_BYPASS_ONLY);

	BTMTK_INFO("%s:[new] hub_en[%d] sleep_en[%d] hub_bypass_only[%d]", __func__
				, cif_dev->hub_en, cif_dev->sleep_en, cif_dev->hub_bypass_only);
	return 0;
}

#if (USE_DEVICE_NODE == 1)
static int btmtk_uart_set_xonv(struct btmtk_dev *bdev, u8 *connxo, int nvsz)
{
	struct btmtk_uart_dev *cif_dev = NULL;

	BTMTK_INFO("%s start nvsz[%d]", __func__, nvsz);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_INFO("%s:sConnxo_cfg.ucDataLenLSB[%02x] sConnxo_cfg.ucDataLenMSB[%02x], sConnxo_cfg.ucFreqC1Axm52m[%02x]", __func__
				, sConnxo_cfg.ucDataLenLSB, sConnxo_cfg.ucDataLenMSB, sConnxo_cfg.ucFreqC1Axm52m);

	// update sConnxo_cfg
	memcpy(&sConnxo_cfg, connxo, nvsz);

	BTMTK_INFO("%s:[new] sConnxo_cfg.ucDataLenLSB[%02x] sConnxo_cfg.ucDataLenMSB[%02x], sConnxo_cfg.ucFreqC1Axm52m[%02x]", __func__
				, sConnxo_cfg.ucDataLenLSB, sConnxo_cfg.ucDataLenMSB, sConnxo_cfg.ucFreqC1Axm52m);
	return 0;
}
#endif

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


static int btmtk_uart_wait_tty_buffer_clean(struct btmtk_dev *bdev, bool do_flush)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	int count = 0, flush_retry = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (cif_dev->flush_en) {
		unsigned long start_time = jiffies, time_diff = 0;

		do {
			if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT) {
				BTMTK_ERR("%s: BTMTK_STATE_DISCONNECT", __func__);
				return -1;
			}
			count = tty_chars_in_buffer(cif_dev->tty);
			if (count == 0)
				break;
			/* only wait 200ms for tty buffer clean */
			/* use udelay instead of usleep_range incase of sleep too long */
			usleep_range(500, 550);
		} while (flush_retry++ < BTMTK_MAX_WAIT_RETRY);
		time_diff = jiffies_to_msecs(jiffies) - jiffies_to_msecs(start_time);
		if (time_diff > TIMT_BOUND_OF_CHARS_WAIT)
			BTMTK_WARN("%s: chars in buffer takes %lu ms to clear, remain count[%d]",
				__func__, time_diff, count);

		if (flush_retry < BTMTK_MAX_WAIT_RETRY && do_flush) {
			/* stop uart auto send next pkt to avoid flush conflict with send pkt */
#if (KERNEL_VERSION(5, 14, 0) < LINUX_VERSION_CODE)
			cif_dev->tty->flow.stopped = true;
			tty_driver_flush_buffer(cif_dev->tty);
			cif_dev->tty->flow.stopped = false;
#else
			cif_dev->tty->stopped = true;
			tty_driver_flush_buffer(cif_dev->tty);
			cif_dev->tty->stopped = false;
#endif
		}
		time_diff = jiffies_to_msecs(jiffies) - jiffies_to_msecs(start_time);
		if (time_diff >= TIME_BOUND_OF_TTY_FLUSH) {
			BTMTK_ERR("%s: flush time takes %lu ms", __func__, time_diff);
#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
			mtk8250_uart_dump(cif_dev->tty);
#endif
		}
	}

	return flush_retry;

}

static int btmtk_uart_load_fw_patch_using_dma(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset)
{
	int cur_len = 0;
	u32 max_pkt_cnt = 0, zero_pkt_cnt = 0, current_cpu = 0;
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;
	s32 sent_len;
	u8 cmd[LD_PATCH_CMD_LEN] = {0x02, 0x6F, 0xFC, 0x05, 0x00, 0x01, 0x01, 0x01, 0x00, PATCH_PHASE3};
	u8 event[LD_PATCH_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; /* event[7] is status*/
	unsigned long end_time = 0, dump_time = 0, record_time = 0, log_time = 0;

	if (bdev == NULL || image == NULL || fwbuf == NULL) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return -1;
	}
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	BTMTK_DBG("%s: loading rom patch... start", __func__);

	down(&cif_dev->tty_flush_sem);
	log_time = jiffies;
	record_time = jiffies;
	end_time = jiffies + msecs_to_jiffies(TIME_BOUND_OF_FW_PKG_DL);
	dump_time = jiffies + msecs_to_jiffies(WOBLE_EVENT_INTERVAL_TIMO);
	while (1) {
		sent_len = (section_dl_size - cur_len) >= (UPLOAD_PATCH_UNIT) ?
				(UPLOAD_PATCH_UNIT) : (section_dl_size - cur_len);
		if (bdev->is_whole_chip_reset) {
			BTMTK_WARN("%s: whole chip reset happened, don't send cmd", __func__);
			ret = -1;
			goto exit;
		}

		if (sent_len > 0) {
			// memcpy(image, fwbuf + section_offset + cur_len, sent_len);

			/* get interface state without mutex */
			if (bdev && bdev->interface_state != BTMTK_STATE_DISCONNECT) {
				/* avoid uart_launcher get signal 9 close uart, and not notify driver */
				if(cif_dev->tty == NULL || cif_dev->tty->port == NULL || cif_dev->tty->port->count == 0) {
					BTMTK_WARN("%s: tty port count is 0", __func__);
					goto exit;
				}
				log_time = jiffies_to_msecs(jiffies) - jiffies_to_msecs(log_time);
				if (log_time > TIME_DUMP_OF_FW_PKG_DL)
					BTMTK_WARN("%s: while(1) loop more than %d ms, [%lu] ms, cpu[%u]",
							__func__, TIME_DUMP_OF_FW_PKG_DL, log_time,  current_thread_info()->cpu);
				log_time = jiffies;
				current_cpu =  current_thread_info()->cpu;
				ret = cif_dev->tty->ops->write(cif_dev->tty, fwbuf + section_offset + cur_len, sent_len);
				log_time = jiffies_to_msecs(jiffies) - jiffies_to_msecs(log_time);
				if (log_time > TIME_DUMP_OF_FW_PKG_DL)
					BTMTK_WARN("%s: tty write more than %d ms, [%lu] ms, tty_chars_in_buffer[%d], cur_len[%d], write_cnt[%d]",
							__func__, TIME_DUMP_OF_FW_PKG_DL, log_time, tty_chars_in_buffer(cif_dev->tty), cur_len, ret);
				if(current_cpu !=  current_thread_info()->cpu)
					BTMTK_DBG("%s: cpu changed in tty write [%u]->[%u]", __func__, current_cpu,  current_thread_info()->cpu);
				log_time = jiffies;
			} else {
				BTMTK_WARN("%s: tty is closing, skip download", __func__);
				ret = -1;
				goto exit;
			}

			/* every 500ms dump uart state */
			if (time_after(jiffies, dump_time)) {
				BTMTK_WARN("%s: download 1 patch more than %d ms, tty_chars[%d], cur_len[%d], zero_pkt[%d], cpu[%u]",
						__func__, WOBLE_EVENT_INTERVAL_TIMO, tty_chars_in_buffer(cif_dev->tty),
						cur_len, zero_pkt_cnt, current_thread_info()->cpu);
#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
				mtk8250_uart_dump(cif_dev->tty);
#endif
				BTMTK_INFO("%s: mtk8250_uart_dump end", __func__);
				dump_time = jiffies + msecs_to_jiffies(WOBLE_EVENT_INTERVAL_TIMO);
			}

			if (time_after(jiffies, end_time)) {
				BTMTK_ERR("%s: Download 1 patch more than %d ms, tty_chars[%d], cur_len[%d], zero_pkt[%d], cpu[%u]",
						__func__, TIME_BOUND_OF_FW_PKG_DL, tty_chars_in_buffer(cif_dev->tty),
						cur_len, zero_pkt_cnt, current_thread_info()->cpu);
			}

			if (ret == UPLOAD_PATCH_UNIT)
				max_pkt_cnt++;
			else if (ret == 0)
				zero_pkt_cnt++;
			else
				BTMTK_DBG("%s, sent_len[%d] tty_write[%d] max_pkt_cnt[%d]",
						__func__, sent_len, ret, max_pkt_cnt);
			cur_len += ret;
		} else
			break;
	}
	up(&cif_dev->tty_flush_sem);
	record_time = jiffies_to_msecs(jiffies) - jiffies_to_msecs(record_time);
	BTMTK_INFO("%s: patch done, cost %lu ms, max_pkt_cnt[%d], zero_pkt_cnt[%d], cur_len[%d], send dl phase3 cmd ",
			__func__, record_time, max_pkt_cnt, zero_pkt_cnt, cur_len);

	/* seperate phase 3 cmd with dma mode content */
	usleep_range(1000, 1100);
	ret = btmtk_main_send_cmd(bdev,
			cmd, LD_PATCH_CMD_LEN,
			event, LD_PATCH_EVT_LEN,
			PATCH_DOWNLOAD_PHASE3_DELAY_TIME,
			PATCH_DOWNLOAD_PHASE3_RETRY,
			BTMTK_TX_ACL_FROM_DRV,
			CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: send wmd dl cmd failed, terminate!", __func__);
		return ret;
	}
	BTMTK_DBG("%s: loading rom patch... Done", __func__);

	return ret;
exit:
	up(&cif_dev->tty_flush_sem);
	return ret;
}

int btmtk_cif_send_cmd(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, int retry, int delay)
{
	int ret = -1;
	u32 len = 0, count = 0, flush_retry = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (bdev->is_whole_chip_reset) {
		BTMTK_WARN("%s: whole chip reset happened, don't send cmd", __func__);
		return -1;
	}

	down(&cif_dev->tty_flush_sem);
	/* wait tty buffer clean */

	flush_retry = btmtk_uart_wait_tty_buffer_clean(bdev, TRUE);
	if (flush_retry < 0) {
		up(&cif_dev->tty_flush_sem);
		return -1;
	}

	while (len != cmd_len && count < BTMTK_MAX_SEND_RETRY
			&& btmtk_get_chip_state(bdev) != BTMTK_STATE_DISCONNECT) {
		/* avoid uart_launcher get signal 9 close uart, and not notify driver */
		if(cif_dev->tty == NULL || cif_dev->tty->port == NULL || cif_dev->tty->port->count == 0) {
			BTMTK_WARN("%s: tty port count is 0", __func__);
			ret = -1;
			break;
		}
		ret = cif_dev->tty->ops->write(cif_dev->tty, cmd + len, cmd_len - len);
		if (ret == 0)
			udelay(500);
		len += ret;
		count++;
	}
	up(&cif_dev->tty_flush_sem);

	if (count == BTMTK_MAX_SEND_RETRY) {
		BTMTK_ERR("%s: retry[%d] fail", __func__, count);
		ret = -1;
	}
#if (USE_DEVICE_NODE == 1)
	/* use HCI_SNOOP_TYPE_TX_ISO_HIF to record data sended to tty */
	btmtk_hci_snoop_save(HCI_SNOOP_TYPE_TX_ISO_HIF, cmd, cmd_len);
#endif
	BTMTK_DBG_RAW(cmd, cmd_len, "%s, len[%d] write_retry[%d] room[%d] flush_retry[%d] CMD :", __func__, cmd_len,
						count, tty_write_room(cif_dev->tty), flush_retry);

	return ret;
}

/* bt_tx_wait_for_msg
 *
 *    Check needing action of current bt status to wake up bt thread
 *
 * Arguments:
 *    [IN] bdev     - bt driver control strcuture
 *
 * Return Value:
 *    return check  - 1 for waking up bt thread, 0 otherwise
 *
 */
static u32 btmtk_thread_wait_for_msg(struct btmtk_dev *bdev)
{
	u32 ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (!skb_queue_empty(&cif_dev->tx_queue))
		ret |= BTMTK_THREAD_TX;

#if (SLEEP_ENABLE == 1)
	if (atomic_read(&cif_dev->need_drv_own)) {
		//BTMTK_DBG("%s: set drv own", __func__);
		atomic_set(&cif_dev->need_drv_own, 0);
		ret |= BTMTK_THREAD_RX;
	}

	if (atomic_read(&cif_dev->fw_own_timer_flag) == FW_OWN_TIMER_RUNNING) {
		//BTMTK_DBG("%s: set fw own", __func__);
		/* incase of tx_thread keep running for FW_OWN_TIMER_RUNNING */
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_DONE);
		ret |= BTMTK_THREAD_FW_OWN;
	}
#endif

	if (atomic_read(&cif_dev->need_assert)) {
		BTMTK_INFO("%s: need_assert", __func__);
		atomic_set(&cif_dev->need_assert, 0);
		ret |= BTMTK_THREAD_ASSERT;
	}

	if (kthread_should_stop()) {
		BTMTK_DBG("%s: kthread_should_stop", __func__);
		ret |= BTMTK_THREAD_STOP;
	}

	return ret;
}

static int btmtk_uart_tx_thread(void *data)
{
	struct btmtk_dev *bdev = data;
	struct btmtk_uart_dev *cif_dev = NULL;
	int state = BTMTK_STATE_INIT;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	struct sk_buff *skb;
	u32 thread_flag = 0;
	int ret = 0;

	BTMTK_INFO("%s start", __func__);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}
	/* avoid unused var for USE_DEVICE_NODE == 0 */
	ret = 0;

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	atomic_set(&cif_dev->thread_status, 1);

	while (1) {
		wait_event_interruptible(tx_wait_q,
			(thread_flag = btmtk_thread_wait_for_msg(bdev)));

		if (thread_flag & BTMTK_THREAD_STOP) {
			BTMTK_WARN("%s: thread is stopped, break", __func__);
			break;
		}

		state = btmtk_get_chip_state(bdev);
		fstate = btmtk_fops_get_state(bdev);

#if (SLEEP_ENABLE == 1)
		if (state == BTMTK_STATE_FW_DUMP || state == BTMTK_STATE_SEND_ASSERT
			|| state == BTMTK_STATE_SUBSYS_RESET || fstate == BTMTK_FOPS_STATE_CLOSING) {
			//BTMTK_DBG("%s: no fw/driver own, no tx when dumping", __func__);
			/* if disable tx would not send rhw debug sop */
			thread_flag &= ~(BTMTK_THREAD_FW_OWN);
			/* incase of aftrer fw coredump, send fw own fail and trigger assert again */
			btmtk_uart_delete_fw_own_timer(cif_dev);
		}

		if (thread_flag & (BTMTK_THREAD_TX | BTMTK_THREAD_RX)) {
			ret = btmtk_uart_driver_own(bdev);
			if (ret < 0)
				thread_flag |= BTMTK_THREAD_ASSERT;
		} else if (thread_flag & BTMTK_THREAD_FW_OWN) {
			ret = btmtk_uart_fw_own(bdev);
			if (ret < 0)
				thread_flag |= BTMTK_THREAD_ASSERT;
		}
#endif
#if (USE_DEVICE_NODE == 1)
		if (thread_flag & BTMTK_THREAD_ASSERT)
			btmtk_uart_trigger_assert(bdev);
#endif

		if (thread_flag & BTMTK_THREAD_TX) {
			if (cif_dev->own_state != BTMTK_DRV_OWN) {
				BTMTK_WARN_LIMITTED("%s not in dirver_own state[%d] can not send cmd", __func__, cif_dev->own_state);
				skb_queue_purge(&cif_dev->tx_queue);
			}
			while (skb_queue_len(&cif_dev->tx_queue)) {
				/* skb_dequeue already have lock protection */
				skb = skb_dequeue(&cif_dev->tx_queue);
				if (skb) {
					btmtk_cif_send_cmd(bdev,
						skb->data, skb->len,
						5, 0);
					kfree_skb(skb);
					skb = NULL;
				}
			}
		}
	}
	atomic_set(&cif_dev->thread_status, 0);
	BTMTK_INFO("%s end", __func__);
	return 0;
}

static int btmtk_tx_thread_start(struct btmtk_dev *bdev)
{
	int i = 0;
	struct btmtk_uart_dev *cif_dev = NULL;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_DBG("%s start", __func__);

	if (!atomic_read(&cif_dev->thread_status)) {
		cif_dev->tx_task = kthread_run(btmtk_uart_tx_thread,
						bdev, "btmtk_uart_tx_thread");
		if (IS_ERR(cif_dev->tx_task)) {
			BTMTK_ERR("%s create tx thread FAILED", __func__);
			return -1;
		}

		while (!atomic_read(&cif_dev->thread_status) && i < TX_THREAD_RETRY) {
			BTMTK_INFO("%s: wait btmtk_uart_tx_thread start, retry[%d]", __func__, i);
			usleep_range(2000, 2100);
			i++;
			if (i == TX_THREAD_RETRY - 1) {
				BTMTK_INFO("%s: wait btmtk_uart_tx_thread start failed", __func__);
				return -1;
			}
		}

		BTMTK_INFO("%s started", __func__);
	} else {
		BTMTK_INFO("%s already running", __func__);
	}
	skb_queue_purge(&cif_dev->tx_queue);


	return 0;
}


static int btmtk_tx_thread_exit(struct btmtk_uart_dev *cif_dev)
{
	int i = 0;

	BTMTK_DBG("%s start", __func__);

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}
	TX_THREAD_MUTEX_LOCK();
	if (!IS_ERR(cif_dev->tx_task) && atomic_read(&cif_dev->thread_status)) {
		kthread_stop(cif_dev->tx_task);

		while (atomic_read(&cif_dev->thread_status) && i < TX_THREAD_RETRY) {
			BTMTK_INFO("%s: wait btmtk_uart_tx_thread stop, retry[%d]", __func__, i);
			usleep_range(2000, 2100);
			i++;
			if (i == TX_THREAD_RETRY - 1) {
				BTMTK_INFO("%s: wait btmtk_uart_tx_thread stop failed", __func__);
				break;
			}
		}
	}
	TX_THREAD_MUTEX_UNLOCK();
	skb_queue_purge(&cif_dev->tx_queue);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

/* Allocate Uart-Related memory */
static int btmtk_uart_allocate_memory(struct btmtk_dev *bdev)
{
	if (!IS_ERR_OR_NULL(bdev->rx_skb))
		kfree_skb(bdev->rx_skb);
	bdev->rx_skb = NULL;
	return 0;
}

int btmtk_cif_send_calibration(struct btmtk_dev *bdev)
{
	return 0;
}

#if (USE_DEVICE_NODE == 0)
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
#endif

static int btmtk_uart_deinit(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s", __func__);
	return 0;
}

static int btmtk_uart_init(struct btmtk_dev *bdev)
{
	int err = 0;

#if (USE_DEVICE_NODE == 0)
	err = btmtk_ce_gpio_init(bdev->cif_dev);
	if (err < 0) {
		BTMTK_ERR("%s: ce gpio init failed!", __func__);
		return err;
	}
	err = btmtk_pre_power_on_handler(bdev->cif_dev);
	if (err < 0) {
		BTMTK_ERR("%s: pre power on handler failed!", __func__);
		return err;
	}
#endif

	err = btmtk_main_cif_initialize(bdev, HCI_UART);
	if (err < 0) {
		BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed!");
		goto end;
	}

#if (USE_DEVICE_NODE == 0)
	err = btmtk_uart_set_pinmux(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk_uart_set_pinmux failed!");
		goto deinit;
	}
#else
	INIT_WORK(&bdev->hif_dump_work, btmtk_hif_dump_work);
#endif

	INIT_WORK(&bdev->reset_waker, btmtk_reset_waker);
	goto end;

#if (USE_DEVICE_NODE == 0)
deinit:
	btmtk_main_cif_uninitialize(bdev, HCI_UART);
#endif
end:
	BTMTK_DBG("%s done", __func__);
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
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
	tty->port->low_latency = 1;
#endif

	btmtk_uart_allocate_memory(bdev);

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	tty->disc_data = bdev;
	cif_dev->tty = tty;
	dev_set_drvdata(tty->dev, bdev);

	spin_lock_init(&cif_dev->tx_lock);
	skb_queue_head_init(&cif_dev->tx_queue);

	/* start tx_thread */
	if (btmtk_tx_thread_start(bdev))
		return -EFAULT;

	cif_dev->stp_cursor = 2;
	cif_dev->stp_dlen = 0;

	/* definition changed!! */
	if (tty->ldisc->ops->flush_buffer)
		tty->ldisc->ops->flush_buffer(tty);

	tty_driver_flush_buffer(tty);

	BTMTK_DBG("%s: tty done %p", __func__, tty);

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
#if (USE_DEVICE_NODE == 0)
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	btmtk_woble_uninitialize(&cif_dev->bt_woble);
#endif
	BTMTK_INFO("%s: tty %p", __func__, tty);
	cancel_work_sync(&bdev->reset_waker);
	btmtk_tx_thread_exit(bdev->cif_dev);
	btmtk_main_cif_disconnect_notify(bdev, HCI_UART);
}

/*
 * We don't provide read/write/poll interface for user space.
 */
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
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
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
static int btmtk_uart_tty_ioctl(struct tty_struct *tty,
			      unsigned int cmd, unsigned long arg)
#else
static int btmtk_uart_tty_ioctl(struct tty_struct *tty, struct file *file,
			      unsigned int cmd, unsigned long arg)
#endif
{
	int err = 0;
#if (USE_DEVICE_NODE == 0)
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
#endif
	struct UART_CONFIG uart_cfg;
	struct btmtk_dev *bdev = tty->disc_data;
	struct btmtk_uart_dev *cif_dev = NULL;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_DBG("%s: tty %p cmd = %u", __func__, tty, cmd);

	switch (cmd) {
	case HCIUARTSETPROTO:
		BTMTK_INFO("%s: <!!> Set low_latency to TRUE <!!>", __func__);
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
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
#if (USE_DEVICE_NODE == 0)
		err = btmtk_uart_send_set_uart_cmd(bdev->hdev, &uart_cfg);
#endif
		break;
	case HCIUARTSETWAKEUP:
#if (USE_DEVICE_NODE == 0)
		BTMTK_INFO("%s: <!!> Send Wakeup <!!>", __func__);
		err = btmtk_uart_send_wakeup_cmd(bdev->hdev);
#endif
		break;
	case HCIUARTGETBAUD:
#if (USE_DEVICE_NODE == 0)
		BTMTK_INFO("%s: <!!> Get BAUDRATE <!!>", __func__);
		err = btmtk_uart_send_query_uart_cmd(bdev->hdev);
#endif
		break;
	case HCIUARTSETSTP:
		BTMTK_INFO("%s: <!!> Set STP mandatory command <!!>", __func__);
		break;
	case HCIUARTLOADPATCH:
#if (USE_DEVICE_NODE == 0)
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
			return err;
		}
		cif_dev->fw_dl_ready = 1;

		err = btmtk_woble_initialize(bdev, &cif_dev->bt_woble);
		if (err < 0)
			BTMTK_ERR("btmtk_woble_initialize failed!");
		else
			BTMTK_ERR("btmtk_woble_initialize");
		err = btmtk_register_hci_device(bdev);

		if (err < 0)
			BTMTK_ERR("btmtk_register_hci_device failed!");
#endif
		break;
	case HCIUARTINIT:
		BTMTK_INFO("%s: <!!> Set HCIUARTINIT <!!>", __func__);
		cif_dev->fw_dl_ready = 0;
		err = btmtk_uart_init(bdev);
		break;
	case HCIUARTDEINIT:
		BTMTK_INFO("%s: <!!> Set HCIUARTDEINIT <!!>", __func__);
		err = btmtk_uart_deinit(bdev);
		break;
        case HCIUARTXOPARAM:
		if (copy_from_user(&sConnxo_cfg, (CONNXO_CFG_PARAM_STRUCT __user *)arg, sizeof(CONNXO_CFG_PARAM_STRUCT)))
			return -ENOMEM;
		BTMTK_INFO("%s: <!!> Set HCIUARTXOPARAM <!!>", __func__);
		BTMTK_INFO("%s: ucDataLenLSB = %02x, ucDataLenMSB = %02x", __func__, sConnxo_cfg.ucDataLenLSB, sConnxo_cfg.ucDataLenMSB);
		break;
	default:
		/* pr_info("<!!> n_tty_ioctl_helper <!!>\n"); */
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		err = n_tty_ioctl_helper(tty, cmd, arg);
#else
		err = n_tty_ioctl_helper(tty, file, cmd, arg);
#endif
		break;
	};

	return err;
}

#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
static long btmtk_uart_tty_compat_ioctl(struct tty_struct *tty, struct file *file,
			      unsigned int cmd, unsigned long arg)
#elif (KERNEL_VERSION(5, 16, 0) > LINUX_VERSION_CODE)
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

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	BTMTK_INFO("%s: tty %p cmd = %u", __func__, tty, cmd);

	switch (cmd) {
	case HCIUARTSETPROTO:
		BTMTK_INFO("%s: <!!> Set low_latency to TRUE <!!>", __func__);
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
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

		err = btmtk_woble_initialize(bdev, &cif_dev->bt_woble);
		if (err < 0)
			BTMTK_ERR("btmtk_woble_initialize failed!");
		else
			BTMTK_ERR("btmtk_woble_initialize");

		err = btmtk_register_hci_device(bdev);
		if (err < 0) {
			BTMTK_ERR("btmtk_register_hci_device failed!");
		}
		break;
	case HCIUARTINIT:
		BTMTK_INFO("%s: <!!> Set HCIUARTINIT <!!>", __func__);
		err = btmtk_uart_init(bdev);
		break;
	case HCIUARTDEINIT:
		BTMTK_INFO("%s: <!!> Set HCIUARTDEINIT <!!>", __func__);
		btmtk_set_chip_state(bdev, BTMTK_STATE_DISCONNECT);
		break;
	default:
		/* pr_info("<!!> n_tty_ioctl_helper <!!>\n"); */
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		err = n_tty_ioctl_helper(tty, cmd, arg);
#else
		err = n_tty_ioctl_helper(tty, file, cmd, arg);
#endif
		break;
	};

	return err;
}

void btmtk_wakeup_host(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	BTMTK_DBG("%s start", __func__);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	if (bdev->fops_state != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_ERR("%s: fops is not opended (%d)", __func__, bdev->fops_state);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}
	atomic_set(&cif_dev->need_drv_own, 1);
	atomic_set(&cif_dev->fw_wake, 1);
	wake_up_interruptible(&tx_wait_q);
}

#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
static void btmtk_uart_tty_receive(struct tty_struct *tty, const u8 *data, char *flags, int count)
#elif (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
static void btmtk_uart_tty_receive(struct tty_struct *tty, const u8 *data, const char *flags, int count)
#else
static void btmtk_uart_tty_receive(struct tty_struct *tty, const u8 *data, const u8 *flags, size_t count)
#endif
{
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	unsigned char state = 0;
	struct btmtk_dev *bdev = tty->disc_data;
	static u32 recv_fail_cnt;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	fstate = btmtk_fops_get_state(bdev);
	state = btmtk_get_chip_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_CLOSED
#if (USE_DEVICE_NODE == 0)
	 && state != BTMTK_STATE_FW_DUMP && state != BTMTK_STATE_SUBSYS_RESET
#endif
	) {
#if (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
		BTMTK_INFO_RAW(data, count, "[SKIP] %s: count[%d]", __func__, count);
#else
		BTMTK_INFO_RAW(data, count, "[SKIP] %s: count[%zu]", __func__, count);
#endif
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	/* record data from tty */
	btmtk_hci_snoop_save(HCI_SNOOP_TYPE_EVT_HIF, data, count);
#if (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
	BTMTK_DBG_RAW(data, count, "%s: len[%d]", __func__, count);
#else
	BTMTK_DBG_RAW(data, count, "%s: len[%zu]", __func__, count);
#endif

#if (SLEEP_ENABLE == 1)
	//BTMTK_INFO_RAW(data, count, "%s: count[%d]", __func__, count);

	/* if flag is BTMTK_FW_OWNING not set driver own , because data is fw own event */
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->own_state == BTMTK_FW_OWN &&
	    !atomic_read(&cif_dev->fw_wake) &&
	    data != NULL && count > 0 && data[0] != 0xFF && cif_dev->sleep_en) {
		unsigned int index = 0, _count = count;
		u8 *buf = (u8 *)data;

		BTMTK_INFO_RAW(data, count, "%s: recv none 0xFF as first byte in FW own state", __func__);
		while (_count && *(buf + index) != 0xFF) {
			index++;
			_count--;
		}

		if (_count > 0) {
			count = _count;
			data = buf + index;
		} else
			return;
		BTMTK_INFO_RAW(data, count, "%s: data after trim", __func__);
	}
#endif
	if (data != NULL && (count > 1 || data[0] != 0x00) && cif_dev->own_state != BTMTK_FW_OWNING) {
		atomic_set(&cif_dev->need_drv_own, 1);
		atomic_set(&cif_dev->fw_wake, 1);
		wake_up_interruptible(&tx_wait_q);
	}
#endif

	/* add hci device part */
	ret = btmtk_recv(bdev->hdev, data, count);

	/* debug for invalid buffer */
	if ((ret == -EILSEQ || ret == -EMSGSIZE) && count > 1
			&& btmtk_get_chip_state(bdev) != BTMTK_STATE_DISCONNECT) {
		if (!atomic_read(&bdev->assert_state) && recv_fail_cnt == BTMTK_MAX_RECV_ERR_CNT) {
			if (bdev->assert_reason[0] == '\0') {
				strncpy(bdev->assert_reason, "[BT_DRV assert] recv unknown data",
						strlen("[BT_DRV assert] recv unknown data") + 1);
				BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
			}
#if (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
			BTMTK_WARN("%s: trigger assert, recv_fail_cnt[%d] count[%d]",
					__func__, ++recv_fail_cnt, count);
#else
			BTMTK_WARN("%s: trigger assert, recv_fail_cnt[%d] count[%zu]",
					__func__, ++recv_fail_cnt, count);
#endif
			/* can not trigger assert in this thread, would block event of debug sop */
			atomic_set(&cif_dev->need_assert, 1);
			wake_up_interruptible(&tx_wait_q);
		}
		else if (recv_fail_cnt < BTMTK_MAX_RECV_ERR_CNT) {
#if (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
			BTMTK_WARN("%s: recv error data, recv_fail_cnt[%d] count[%d]",
					__func__, ++recv_fail_cnt, count);
#else
			BTMTK_WARN("%s: recv error data, recv_fail_cnt[%d] count[%zu]",
					__func__, ++recv_fail_cnt, count);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
			mtk8250_uart_dump(cif_dev->tty);
#endif
		}
	} else
		recv_fail_cnt = 0;
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

#if (SLEEP_ENABLE == 0)
static int btmtk_uart_fw_own(struct btmtk_dev *bdev)
{
	int ret;
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x01 };
	u8 evt[] = { 0x04, 0xE4, 0x07, 0x02, 0x03, 0x03, 0x00, 0x00, 0x01, 0x01 };


	BTMTK_INFO("%s", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, FWOWN_CMD_LEN, evt, OWNTYPE_EVT_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	return ret;
}

static int btmtk_uart_driver_own(struct btmtk_dev *bdev)
{
	int ret;
	u8 cmd[] = { 0xFF };
	u8 evt[] = { 0x04, 0xE4, 0x07, 0x02, 0x03, 0x03, 0x00, 0x00, 0x03, 0x01 };

	BTMTK_INFO("%s", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, DRVOWN_CMD_LEN, evt, OWNTYPE_EVT_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	return ret;
}

#else //(SLEEP_ENABLE == 1)
static int btmtk_uart_fw_own(struct btmtk_dev *bdev)
{
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x03, 0x02, 0x00, 0x01, 0x01 };
	u8 evt[] = { 0x04, 0xE4, 0x07, 0x02, 0x03, 0x03, 0x00, 0x00, 0x01, 0x01 };

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	UART_OWN_MUTEX_LOCK();
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	/* no need to compare BTMTK_FW_OWNING because the state must be fw_own/fail before leaving mutex */
	if (cif_dev->own_state == BTMTK_FW_OWN || cif_dev->own_state == BTMTK_OWN_FAIL) {
		BTMTK_WARN("Already at fw own state or error state[%d], skip", cif_dev->own_state);
		goto unlock;
	}

	BTMTK_DBG("%s: fw owning", __func__);
	cif_dev->own_state = BTMTK_FW_OWNING;

	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE) {
		BTMTK_WARN("%s: during send_and_recv, keep drv own", __func__);
		cif_dev->own_state = BTMTK_DRV_OWN;
		btmtk_uart_update_fw_own_timer(cif_dev);
		goto unlock;
	}

	if (cif_dev->sleep_en) {
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
		if (cif_dev->hub_en && !cif_dev->sleep_flow_hw_mech_en) {
			ret = mtk8250_uart_hub_dev0_clear_tx_request();
			BTMTK_DBG("%s mtk8250_uart_hub_dev0_clear_tx_request, ret[%d]", __func__, ret);
			/* record host wakeup info to fw, b[4] = AP, b[5] = MD, b[6] = ADSP */
			cmd[9] = cmd[9] | ((mtk8250_uart_hub_get_host_wakeup_status() & 0xf) << 4);
		}
#endif

		/* two different event for fw allow sleep or not */
		ret = btmtk_main_send_cmd(bdev, cmd, FWOWN_CMD_LEN, evt, OWNTYPE_EVT_LEN - 3,
				DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
		/* evt[7] = 1 for no sleep */
		if (bdev->io_buf[7]) {
			/* re-set tx request */
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
			if (cif_dev->hub_en)
				btmtk_wakeup_uarthub();
#endif
			BTMTK_WARN("%s fw not allow sleep, keep drv own, cmd[9] = 0x%02x", __func__, cmd[9]);
			cif_dev->own_state = BTMTK_DRV_OWN;
			goto unlock;
		}
	} else
		ret = 0;

	if (ret < 0) {
		cif_dev->own_state = BTMTK_DRV_OWN;
		BTMTK_ERR("%s: set fw own return fail, ret[%d]", __func__, ret);
		if (bdev->assert_reason[0] == '\0') {
			strncpy(bdev->assert_reason, "[BT_DRV assert] fw own failed",
					strlen("[BT_DRV assert] fw own failed") + 1);
			BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
		}
		goto unlock;
	} else {
		cif_dev->own_state = BTMTK_FW_OWN;
		atomic_set(&cif_dev->fw_wake, 0);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
		if (cif_dev->sleep_flow_hw_mech_en && cif_dev->sleep_en) {
			/*
				12 Clear DEVx_HOST_AWAKE = 0
				14 Clear TX/RX REQ
			*/
			BTMTK_DBG_LIMITTED("%s For new sleep wakeup", __func__);
			btmtk_release_uarthub(true);
		} else {
			btmtk_release_uarthub(false);
		}
#endif
		__pm_relax(bt_trx_wakelock);
		BTMTK_INFO("%s: success, cmd[9] = 0x%02x", __func__, cmd[9]);
	}
unlock:
	UART_OWN_MUTEX_UNLOCK();
	return ret;
}

static int btmtk_uart_driver_own(struct btmtk_dev *bdev)
{
	int ret = 0, retry = BTMTK_MAX_WAKEUP_RETRY;
	int wait_uart_resume_cnt = BTMTK_MAX_WAIT_UART_RESUME_CNT;
	struct btmtk_uart_dev *cif_dev = NULL;
	u8 fw_own_clr_cmd[] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x03, 0x02, 0x00, 0x03, 0x01 };
	u8 evt[] = { 0x04, 0xE4, 0x07, 0x02, 0x03, 0x03, 0x00, 0x00, 0x03, 0x01 };

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	UART_OWN_MUTEX_LOCK();
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev->own_state == BTMTK_DRV_OWN || cif_dev->own_state == BTMTK_OWN_FAIL) {
		//BTMTK_WARN("Already at driver own state or error state[%d], skip", cif_dev->own_state);
		btmtk_uart_update_fw_own_timer(cif_dev);
		goto unlock;
	}

	cif_dev->own_state = BTMTK_DRV_OWNING;
	__pm_stay_awake(bt_trx_wakelock);
	while (bdev->suspend_state && --wait_uart_resume_cnt) {
		usleep_range(1000, 1100);
		BTMTK_DBG("%s wait uart resume", __func__);
	}
	if (!wait_uart_resume_cnt)
		BTMTK_ERR("%s wait uart resume timeout", __func__);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en && cif_dev->sleep_en) {
		do {
			ret = btmtk_wakeup_uarthub();
			if (ret < 0)
				mtk8250_uart_hub_reset();
			else
				break;
		} while (retry--);

		if (ret < 0) {
			BTMTK_ERR("%s wakeup uart_hub fail", __func__);
			cif_dev->own_state = BTMTK_OWN_FAIL;
			goto unlock;
		}
	}
#endif

	retry = BTMTK_MAX_WAKEUP_RETRY;
	/* if fw already coredump, no need to send drv own cmd */
	if (cif_dev->sleep_en && btmtk_get_chip_state(bdev) != BTMTK_STATE_FW_DUMP) {
		/* if fw already wake, no need to send 0xFF and wait 6ms before clr fw own */
		if (atomic_read(&cif_dev->fw_wake) && !cif_dev->sleep_flow_hw_mech_en) {
			/* wait a while for avoid rx pkt error */
			usleep_range(4000, 4100);
		} else {
			u8 wakeup_cmd[] = { 0xFF };
			 if (bdev->is_eap) {
				if (!cif_dev->sleep_flow_hw_mech_en)
					ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, evt, OWNTYPE_EVT_LEN, DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
			} else {
				if (cif_dev->sleep_flow_hw_mech_en) {
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
					u8 retry_cnt = 50;

					/* new wakeup flow */
					/* 3. Set DEVx_HOST_AWAKE = 1 */
					/* 4. Tx 0xFF, if DEVx_BT_AWAKE = 0 */
					/* 4~8 Polling CMM_BT_AWAKE = 1 or DEVx_BT_AWAKE = 1 */
					ret = mtk8250_uart_hub_set_host_awake_sta(0);
					BTMTK_DBG("%s: Set DEVx_HOST_AWAKE = 1, ret = %d ", __func__, ret);

					if (mtk8250_uart_hub_get_host_bt_awake_sta(0)== 0) {
						BTMTK_DBG("%s: mtk8250_uart_hub_get_host_bt_awake_sta = 0, send 0xFF ", __func__);
						ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, NULL, 0, DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
					}

					while (mtk8250_uart_hub_get_cmm_bt_awake_sta() == 0 && mtk8250_uart_hub_get_host_bt_awake_sta(0) == 0 && retry_cnt) {
						/* If CMM_BT_AWAKE = 1 or DEVx_BT_AWAKE = 1 leave while */
						BTMTK_INFO("%s: CMM_BT_AWAKE[%d], DEV0_BT_AWAKE[%d], retry[%d]", __func__,
								mtk8250_uart_hub_get_cmm_bt_awake_sta(), mtk8250_uart_hub_get_host_bt_awake_sta(0), retry_cnt);
						retry_cnt--;
						usleep_range(1000, 1100);
					}

					if (mtk8250_uart_hub_get_cmm_bt_awake_sta() == 0 && mtk8250_uart_hub_get_host_bt_awake_sta(0) == 0 && !retry_cnt) {
						BTMTK_ERR("%s: Query BT_AWAKE failed, fw_awake = %d", __func__, atomic_read(&cif_dev->fw_wake));
						mtk8250_uart_hub_dump_with_tag("BT driver query BT_AWAKE failed");
						mtk8250_uart_dump(cif_dev->tty);
					}
#endif
				} else {
						int i = 0;
						for (i = 0; i < 3; i++) {
							/* no need to wait event */
							ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, NULL, 0,
								DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
							/* wait a while for fw wakeup */
							usleep_range(6000, 6100);
						}
				}

			}
			if (ret < 0)
				BTMTK_ERR("%s btmtk_main_send_cmd 0xFF fail, ret[%d]", __func__, ret);
		}

		do {
			/* fw own clr cmd for notice is wakeup by bt driver */
			/* let retry = 0 for only wait for event 500ms */
			ret = btmtk_main_send_cmd(bdev, fw_own_clr_cmd, 10, evt, OWNTYPE_EVT_LEN,
					DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
			if (ret < 0)
				BTMTK_ERR("%s fw_own_clr_cmd fail retry[%d]", __func__, retry);
		} while (ret < 0 && --retry);
	} else
		ret = 0;

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	/* no mattter success or not, all need to set rx request */
	if (cif_dev->hub_en) {
		mtk8250_uart_hub_dev0_set_rx_request();
		BTMTK_DBG("%s mtk8250_uart_hub_dev0_set_rx_request", __func__);
	}
#endif

	if (ret < 0) {
		/* set driver own state and hub request for trigger rhw debug sop */
		cif_dev->own_state = BTMTK_DRV_OWN;
		BTMTK_ERR("%s: set driver own return fail, ret[%d]", __func__, ret);
		if (bdev->assert_reason[0] == '\0') {
			strncpy(bdev->assert_reason, "[BT_DRV assert] drv own failed",
					strlen("[BT_DRV assert] drv own failed") + 1);
			BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
		}
		goto unlock;
	} else if (cif_dev->no_fw_own == 0) {
		cif_dev->own_state = BTMTK_DRV_OWN;
		btmtk_uart_update_fw_own_timer(cif_dev);
		BTMTK_INFO("%s success, fw_awake = %d", __func__, atomic_read(&cif_dev->fw_wake));
	}

unlock:
	UART_OWN_MUTEX_UNLOCK();
	return ret;
}
#endif

static int btmtk_cif_suspend(void)
{
#if (USE_DEVICE_NODE == 0)
	int ret = 0;
	int cif_event = 0, state;
	struct btmtk_cif_state *cif_state = NULL;
	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	struct btmtk_woble *bt_woble = &cif_dev->bt_woble;
#endif

	BTMTK_INFO("%s", __func__);

#if (USE_DEVICE_NODE == 0)
	if (tty == NULL) {
		BTMTK_ERR("%s: tty is NULL, maybe not run btmtk_cif_probe yet", __func__);
		return -EAGAIN;
	}

	bdev = dev_get_drvdata(tty->dev);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -ENODEV;
	}

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

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev) {
		bt_woble = &cif_dev->bt_woble;
		ret = btmtk_woble_suspend(bt_woble);
		if (ret < 0)
			BTMTK_ERR("%s: btmtk_woble_suspend return fail %d", __func__, ret);
	}

	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		ret = btmtk_uart_fw_own(bdev);
		if (ret < 0)
			BTMTK_ERR("%s: set fw own return fail %d", __func__, ret);
	}

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
#endif

	return 0;
}

static int btmtk_cif_resume(void)
{
#if (USE_DEVICE_NODE == 0)
	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_cif_state *cif_state = NULL;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	struct btmtk_woble *bt_woble = &cif_dev->bt_woble;
#endif

	BTMTK_INFO("%s start", __func__);

#if (USE_DEVICE_NODE == 0)
	if (tty == NULL) {
		BTMTK_ERR("%s: tty is NULL, maybe not run btmtk_cif_probe yet", __func__);
		return -EAGAIN;
	}

	bdev = dev_get_drvdata(tty->dev);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	bdev->suspend_count--;
	if (bdev->suspend_count) {
		BTMTK_INFO("data->suspend_count %d, return 0", bdev->suspend_count);
		return 0;
	}

	if (btmtk_get_chip_state(bdev) != BTMTK_STATE_SUSPEND) {
		BTMTK_ERR("%s: not BTMTK_STATE_SUSPEND", __func__);
		return 0;
	}

	cif_state = &bdev->cif_state[HIF_EVENT_RESUME];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		ret = btmtk_uart_driver_own(bdev);
		if (ret < 0)
			BTMTK_ERR("%s: set driver own return fail %d", __func__, ret);
	}
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev) {
		bt_woble = &cif_dev->bt_woble;
		ret = btmtk_woble_resume(bt_woble);
		if (ret < 0)
			BTMTK_ERR("%s: btmtk_woble_resume return fail %d", __func__, ret);
	}

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
#endif

	BTMTK_DBG("%s end", __func__);
	return 0;
}

static int btmtk_pm_notification(struct notifier_block *this, unsigned long event, void *ptr)
{
	int retry = 40;
	int ret = NOTIFY_DONE;

	BTMTK_DBG("event = %ld", event);

	/* if get into suspend flow while doing audio pinmux setting
	 * it may have chance mischange uart pinmux we want to write
	 * retry and wait audio setting done then do suspend flow
	 */
	switch (event) {
	case PM_SUSPEND_PREPARE:
		do {
			ret = btmtk_cif_suspend();
			if (ret == 0) {
				break;
			} else if (retry <= 0) {
				BTMTK_ERR("not ready to suspend");
#if (USE_DEVICE_NODE == 0)
				return NOTIFY_STOP;
#else
				return NOTIFY_BAD;
#endif
			}
			msleep(50);
		} while (retry-- > 0);
		break;
	case PM_POST_SUSPEND:
		btmtk_cif_resume();
#if (USE_DEVICE_NODE == 1)
		if (btmtk_get_chip_state(g_sbdev) == BTMTK_STATE_WORKING) {
			bthost_debug_print();
			btmtk_query_fw_schedule_info(g_sbdev);
		}
#endif
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block btmtk_pm_notifier = {
	.notifier_call = btmtk_pm_notification,
};

static int btmtk_cif_probe(struct tty_struct *tty)
{
	int err = 0;
	int ret = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	/* Mediatek Driver Version */
	BTMTK_INFO("%s: MTK BT Driver Version: %s", __func__, VERSION);

	/* Retrieve priv data and set to interface structure */
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_INFO("%s: bdev is NULL", __func__);
		return -ENODEV;
	}

#if (USE_DEVICE_NODE == 1)
	if (!bdev->cif_dev)
		kfree(bdev->cif_dev);

	cif_dev = kzalloc(sizeof(*cif_dev), GFP_KERNEL);
#else
	cif_dev = devm_kzalloc(tty->dev, sizeof(*cif_dev), GFP_KERNEL);
#endif
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

	/* set working state for uart_launcher restart when driver already open */
	if (btmtk_fops_get_state(bdev) == BTMTK_FOPS_STATE_OPENED) {
		BTMTK_ERR("%s uart_launcher restart when BT already opened, send HW error event", __func__);
		btmtk_set_chip_state((void *)bdev, BTMTK_STATE_WORKING);
		/* notify stack to restart BT */
		bmain_info->reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
		btmtk_send_hw_err_to_host(bdev);
	}else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	/* Init completion */
	init_completion(&bdev->dump_comp);
	init_completion(&bdev->dump_dfd_comp);

	/* Init semaphore */
	sema_init(&cif_dev->evt_comp_sem, 1);
	sema_init(&cif_dev->tty_flush_sem, 1);

	/* Do HIF events */
	ret = btmtk_uart_tty_probe(tty);
#if (defined(LINUX_OS) && (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(ANDROID_OS) && (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE))
	bt_trx_wakelock = wakeup_source_register("bt_drv_trx");
#else
	bt_trx_wakelock = wakeup_source_register(NULL, "bt_drv_trx");
#endif

#if (USE_DEVICE_NODE == 1)
	btmtk_connv3_sub_drv_init(bdev);
	btmtk_pwrctrl_register_evt();

	/* Init coredump */
	bmain_info->hif_hook.coredump_handler = connv3_coredump_init(CONNV3_DEBUG_TYPE_BT, NULL);
#endif
	err = register_pm_notifier(&btmtk_pm_notifier);
	if (err) {
		BTMTK_ERR("Register pm notifier failed. (%d)", err);
		if (err == -EEXIST)
			return ret;
		else
			return err;
	}
	return ret;
}

static void btmtk_cif_disconnect(struct tty_struct *tty)
{
	int err = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
#if (USE_DEVICE_NODE == 1)
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
#endif

	BTMTK_INFO("%s: start", __func__);
	bdev = dev_get_drvdata(tty->dev);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	cif_dev = bdev->cif_dev;

	/* Retrieve current HIF event state */
	cif_event = HIF_EVENT_DISCONNECT;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_WARN("%s priv setting is NULL", __func__);
		return;
	}

	cif_state = &bdev->cif_state[cif_event];
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

#if (SLEEP_ENABLE == 1)
	btmtk_uart_delete_fw_own_timer(cif_dev);
#endif

#if (USE_DEVICE_NODE == 1)
	/* update fw log bt state */
	if (bmain_info->hif_hook.fw_log_state)
		bmain_info->hif_hook.fw_log_state(BTMTK_FOPS_STATE_CLOSED);
	cancel_work_sync(&bdev->pwr_on_uds_work);
#endif

	btmtk_uart_cif_mutex_lock(bdev);
	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);
#if (USE_DEVICE_NODE == 0)
	/* temp solution for disconnect at random time would KE */
	BTMTK_INFO("%s wait", __func__);
	msleep(3000);
#endif
	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_CLOSING || fstate == BTMTK_FOPS_STATE_OPENING) {
		/* temp solution for disconnect at random time would KE */
		BTMTK_WARN("%s bt opening/closing, skip free in disconnect", __func__);
	} else {
		/* Do HIF events */
#if (USE_DEVICE_NODE == 1)
		btmtk_connv3_sub_drv_deinit();
#endif

		btmtk_set_gpio_default();
		btmtk_uart_tty_disconnect(tty);
#if (USE_DEVICE_NODE == 0)
		devm_kfree(tty->dev, cif_dev);
#endif
	}
	wakeup_source_unregister(bt_trx_wakelock);
	/* Set End/Error state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	btmtk_uart_cif_mutex_unlock(bdev);

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
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
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

#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
	err = tty_register_ldisc(N_MTK, &btmtk_uart_ldisc);
#else
	err = tty_register_ldisc(&btmtk_uart_ldisc);
#endif
	if (err) {
		BTMTK_ERR("MTK line discipline registration failed. (%d)", err);
		return err;
	}

	BTMTK_DBG("%s done", __func__);
	return err;
}
static int uart_deregister(void)
{
#if (defined(ANDROID_OS) && (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)) || defined(LINUX_OS)
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

int btmtk_cif_register(void)
{
	int ret = -1;
	struct hif_hook_ptr hook;

	BTMTK_INFO("%s", __func__);

	memset(&hook, 0, sizeof(hook));
#if (USE_DEVICE_NODE == 1)
	rx_queue_initialize();
	hook.fw_log_state = fw_log_bt_state_cb;
	hook.log_init = btmtk_connsys_log_init;
	hook.log_read_to_user = btmtk_connsys_log_read_to_user;
	hook.log_get_buf_size = btmtk_connsys_log_get_buf_size;
	hook.log_deinit = btmtk_connsys_log_deinit;
	hook.log_handler = btmtk_connsys_log_handler;
	hook.met_log_handler = btmtk_met_log_handler;
	hook.init = btmtk_chardev_init;
	hook.dump_debug_sop = btmtk_uart_sp_dump_debug_sop;
	hook.dump_hif_debug_sop = btmtk_hif_sp_dump_debug_sop;
	hook.whole_reset = btmtk_sp_whole_chip_reset;
	hook.trigger_assert = btmtk_uart_trigger_assert;
	hook.wakeup_host = btmtk_wakeup_host;
	hook.set_xonv = btmtk_uart_set_xonv;
#else
	hook.init = btmtk_ce_init;
#endif
	hook.open = btmtk_uart_open;
	hook.close = btmtk_uart_close;
	hook.pre_open = btmtk_uart_pre_open;
	hook.open_done = btmtk_uart_open_done;
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
	hook.set_para = btmtk_uart_set_para;
	btmtk_reg_hif_hook(&hook);

	ret = uart_register();
	if (ret < 0) {
		BTMTK_ERR("*** UART registration fail(%d)! ***", ret);
		return ret;
	}

#if (USE_DEVICE_NODE == 1)
	btmtk_platform_driver_init();
#endif
	BTMTK_DBG("%s: Done", __func__);
	return 0;
}

int btmtk_cif_deregister(void)
{
	int ret = -1;

	BTMTK_INFO("%s", __func__);
#if (USE_DEVICE_NODE == 1)
	btmtk_platform_driver_deinit();
#endif
	ret = uart_deregister();
	if (ret < 0) {
		BTMTK_ERR("*** UART deregistration fail(%d)! ***", ret);
		return ret;
	}
#if (USE_DEVICE_NODE == 1)
	rx_queue_destroy();
	btmtk_dump_gpio_state_unmap();
#endif
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

#if (USE_DEVICE_NODE == 1)
void btmtk_connsys_log_init(void (*log_event_cb)(void))
{
	if (connv3_log_init(CONNV3_DEBUG_TYPE_BT, 2048, 2048, log_event_cb))
		BTMTK_ERR("*** %s fail! ***", __func__);
}

void btmtk_connsys_log_deinit(void)
{
	connv3_log_deinit(CONNV3_DEBUG_TYPE_BT);
}

int btmtk_connsys_log_handler(u8 *buf, u32 size)
{
	return connv3_log_handler(CONNV3_DEBUG_TYPE_BT, CONNV3_LOG_TYPE_PRIMARY, buf, size);
}

ssize_t btmtk_connsys_log_read_to_user(char __user *buf, size_t count)
{
	return connv3_log_read_to_user(CONNV3_DEBUG_TYPE_BT, buf, count);
}

unsigned int btmtk_connsys_log_get_buf_size(void)
{
	return connv3_log_get_buf_size(CONNV3_DEBUG_TYPE_BT);
}

#define MET_LOG_MAX_BUF_SIZE (512 * 2 + 64)
int btmtk_met_log_handler(struct btmtk_dev *bdev, u8 *buf, u32 size)
{
	const static int module_id = 0x00;
	static int project_id = 0x6985;
	static int adie_id = 0x6639;
	const static int adie_ver = 0x0000;
	unsigned int i = 0;
	int len = 0;
	uint8_t raw_buf[MET_LOG_MAX_BUF_SIZE] = {0};
	int single_data_len = 4;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if (bdev->chip_id == 0x6639) {
		project_id = 0x6985;
		adie_id = 0x6639;
		single_data_len = 4;
	} else if (bdev->chip_id == 0x6653) {
		project_id = 0x6991;
		adie_id = 0x6653;
		single_data_len = 8;
	} else {
		BTMTK_ERR("%s: Unsupport connsys chip for MET", __func__);
		return -1;
	}

	if (((size & (single_data_len - 1)) != 0) ||  size == 0) {
		BTMTK_ERR("%s: packet size should be multiple of %d", __func__, single_data_len);
		return -1;
	}

	for (i = 0; i <= size - single_data_len; i += single_data_len) {
		if (single_data_len == 4)
			len = snprintf(raw_buf, MET_LOG_MAX_BUF_SIZE, "%s%02X%02X%02X%02X,",
					raw_buf, buf[i+3], buf[i+2], buf[i+1], buf[i]);
		else if (single_data_len == 8)
			len = snprintf(raw_buf, MET_LOG_MAX_BUF_SIZE, "%s%02X%02X%02X%02X%02X%02X%02X%02X,",
					raw_buf, buf[i+7], buf[i+6], buf[i+5], buf[i+4],
					         buf[i+3], buf[i+2], buf[i+1], buf[i]);
		if (len <= 0) {
			BTMTK_ERR("%s: snprintf error len = %d", __func__, len);
			return -1;
		}

		if (((i + single_data_len) & 0xFF) == 0 || i == size - single_data_len) {
			raw_buf[len - 1] = '\0';
			BTMTK_INFO("MCU_MET_DATA:%02X%04X%04X%04X,%s",
						module_id, project_id,
						adie_id, adie_ver,
						raw_buf);
			memset(raw_buf, 0, MET_LOG_MAX_BUF_SIZE);
		}
	}

	return 0;
}
#endif
