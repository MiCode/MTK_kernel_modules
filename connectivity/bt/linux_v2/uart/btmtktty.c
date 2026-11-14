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
#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
#include "btmtk_tasar.h"
#endif // CFG_SUPPORT_TAS_HOST_CONTROL
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
static atomic_t event_compare_status;
static struct tty_struct *g_tty;
static struct tty_ldisc_ops btmtk_uart_ldisc;
extern struct btmtk_dev *g_sbdev;
static struct platform_device *btmtk_uart_device;

#if (USE_DEVICE_NODE == 1)
static CONNXO_CFG_PARAM_STRUCT sConnxo_cfg;
#endif

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
	if (cif_dev->no_fw_own == 1) {
		BTMTK_DBG_LIMITTED("not update fw own timer");
		return;
	}

	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		BTMTK_DBG_LIMITTED("update fw own timer");
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_INIT);
		/* check only rx case or have host tx */
		if (atomic_read(&g_sbdev->receive_sleep_event) && !atomic_read(&g_sbdev->have_host_tx))
			mod_timer(&cif_dev->fw_own_timer, jiffies + msecs_to_jiffies(0));
		else
			mod_timer(&cif_dev->fw_own_timer, jiffies + msecs_to_jiffies(FW_OWN_TIMEOUT));
	} else
		BTMTK_DBG_LIMITTED("%s: not create yet", __func__);
}

static void btmtk_uart_create_fw_own_timer(struct btmtk_uart_dev *cif_dev)
{
	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		BTMTK_DBG_LIMITTED("%s: created yet", __func__);
	} else {
		BTMTK_WARN("%s: create fw own timer", __func__);
#if (KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE)
		init_timer(&cif_dev->fw_own_timer);
		cif_dev->fw_own_timer.function = btmtk_fw_own_timer;
		cif_dev->fw_own_timer.data = (unsigned long)cif_dev;
#else
		timer_setup(&cif_dev->fw_own_timer, btmtk_fw_own_timer, 0);
#endif
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_INIT);
	}
}

static void btmtk_uart_delete_fw_own_timer(struct btmtk_uart_dev *cif_dev)
{
	if (atomic_read(&cif_dev->fw_own_timer_flag)) {
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_UKNOWN);
		BTMTK_WARN("%s timer deleted", __func__);
#if (KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE)
		del_timer_sync(&cif_dev->fw_own_timer);
#else
		timer_delete_sync(&cif_dev->fw_own_timer);
#endif
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
#if (USE_DEVICE_NODE == 1)
	/* stop tx thread before releasing uarthub trx request */
	btmtk_tx_thread_exit(bdev->cif_dev);
#endif
#if (SLEEP_ENABLE == 1)
	btmtk_uart_delete_fw_own_timer(cif_dev);
#endif

#if (USE_DEVICE_NODE == 0)
	btmtk_uart_fw_own(bdev);
#else
	btmtk_sp_close();

#if (USE_DEVICE_NODE == 0)
	if (bdev->bt_cfg.drv_own_wakelock_enable)
#endif
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
	struct data_struct cmd = {0}, event = {0};

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_REGISTER_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_REGISTER_EVT, event);

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
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	return ret;
}
#endif

static int btmtk_uart_raw_filter(struct btmtk_dev *bdev, const unsigned char *buf, int size)
{
	if (*buf == 0xFF || *buf == 0x00
#if (USE_DEVICE_NODE == 0)
		|| *buf == 0xF0
#endif
	) {
		return 1;
	}
	return 0;
}

int btmtk_uart_event_filter(struct btmtk_dev *bdev, struct sk_buff *skb)
{
#if (USE_DEVICE_NODE == 0)
	const u8 read_address_event[READ_ADDRESS_EVT_HDR_LEN] = { 0x4, 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00 };
	const u8 get_baudrate_event[GETBAUD_EVT_LEN] = { 0x04, 0xE4, 0x0A, 0x02, 0x04, 0x06, 0x00, 0x00, 0x02 };
#endif

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if (atomic_read(&event_compare_status) == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE &&
		skb->len >= event_need_compare_len) {
		memset(bdev->io_buf, 0, IO_BUF_SIZE);
#if (USE_DEVICE_NODE == 1)
		/* cmd from stack would pass 0x01, 0xXX, 0xXX as event_need_compare */
		if (event_need_compare_len == 2 && skb->len > 5 &&
				memcmp(&skb->data[3], event_need_compare, event_need_compare_len) == 0) {
			BTMTK_INFO("%s: compare opcode[0x%02X%02X] from stack success",
					__func__, skb->data[4], skb->data[3]);
			atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS);
			/* return 0 not drop event by driver */
			return 0;
		}
		if (skb->data[2] == 0x02 && skb->data[3] == 0x18) {
			int ret = 0;
			BTMTK_INFO("match DFD event");
			if (skb->data[6] == 0x00) {
				atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS);
				memcpy(bdev->dfd_pstd, &skb->data[7], skb->len - 7);
				ret = connv3_coredump_send(bdev->main_info.hif_hook.coredump_handler, "PSTD", &g_sbdev->dfd_pstd[0], skb->len - 7);
				if (ret)
					BTMTK_ERR("%s: connv3_coredump_send fail, ret = %d", __func__, ret);
				return 1;
			} else {
				BTMTK_ERR("%s compare fail!!!", __func__);
				BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: skb->data:", __func__);
			}
		}
#else // (USE_DEVICE_NODE == 0)
		if ((skb->len == (GETBAUD_EVT_LEN - HCI_TYPE_SIZE + BAUD_SIZE)) &&
			memcmp(skb->data, &get_baudrate_event[1], GETBAUD_EVT_LEN - 1) == 0) {
			BTMTK_INFO("%s: GET BAUD = %02X %02X %02X, FC = %02X", __func__,
				skb->data[10], skb->data[9], skb->data[8], skb->data[11]);
			atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS);
		} else if ((skb->len == (READ_ADDRESS_EVT_HDR_LEN - HCI_TYPE_SIZE + BD_ADDRESS_SIZE)) &&
					memcmp(skb->data, &read_address_event[1], READ_ADDRESS_EVT_HDR_LEN - 1) == 0) {
			memcpy(bdev->bdaddr, &skb->data[READ_ADDRESS_EVT_PAYLOAD_OFFSET - 1], BD_ADDRESS_SIZE);
			BTMTK_DBG("%s: GET BDADDR = "MACSTR, __func__, MAC2STR(bdev->bdaddr));
			atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS);

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
				BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: wobx debug log:", __func__);

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
			atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS);
			BTMTK_DBG("%s, compare success", __func__);
		} else {
			BTMTK_DBG("%s compare fail", __func__);
			BTMTK_INFO_RAW(bdev, event_need_compare, event_need_compare_len,
				"%s: event_need_compare_len[%d]", __func__, event_need_compare_len);
			BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: skb->data:", __func__);
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
			if (bdev->main_info.hif_hook.trigger_assert)
				bdev->main_info.hif_hook.trigger_assert(bdev);
			return -1;
		}
	}
#endif
	btmtk_hci_snoop_save(bdev, HCI_SNOOP_TYPE_CMD_HIF, skb->data, skb->len);
	BTMTK_DBG_RAW(bdev, skb->data, skb->len, "%s: len[%d]", __func__, skb->len);

	/* if just protect event, another cmd would reinit event_compare_status */
	down(&cif_dev->evt_comp_sem);

	if (event) {
		if (event_len > EVENT_COMPARE_SIZE) {
			BTMTK_ERR("%s, event_len (%d) > EVENT_COMPARE_SIZE(%d), error",
				__func__, event_len, EVENT_COMPARE_SIZE);
			up(&cif_dev->evt_comp_sem);
			return -1;
		}

		atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE);
		memcpy(event_need_compare, event + 1, event_len - 1);
		event_need_compare_len = event_len - 1;

		/* if send cmd without drv own, not direct send cmd incase of tx_thread cant not do drv own with send_and_recv */
		if (pkt_type != BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT &&
			(atomic_read(&cif_dev->own_state) != BTMTK_DRV_OWN ||
			atomic_read(&cif_dev->fw_own_timer_flag) == FW_OWN_TIMER_RUNNING ||
			atomic_read(&cif_dev->fw_own_timer_flag) == FW_OWN_TIMER_DONE)) {

			BTMTK_WARN("%s: wait driver own retry, own_state[%d], fw_own_timer_flag[%d]", __func__,
				atomic_read(&cif_dev->own_state), atomic_read(&cif_dev->fw_own_timer_flag));
			atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE);
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
			event_need_compare_len, atomic_read(&event_compare_status));
	} else {
		atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS);
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
			if (atomic_read(&event_compare_status) == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
				ret = 0;
				break;
			}

			/* if during re-download, not wait hci reset cmd/blank cmd.
			 * Cuz in test mode, tool would use hci reset cmd to trigger re-download */
			if (atomic_read(&bdev->dynamic_fwdl_start) &&
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

			/* if uart disconnected, do not wait event anymore */
			if (btmtk_get_chip_state(bdev) == BTMTK_STATE_DISCONNECT) {
				BTMTK_WARN("%s chip state disconnect, break", __func__);
				ret = 0;
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
	atomic_set(&event_compare_status, BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE);
	up(&cif_dev->evt_comp_sem);
	/* control not trigger assert */
	if (ret < 0 && pkt_type != BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT
			&& pkt_type != BTMTK_TX_PKT_SEND_NO_ASSERT) {
		if (bdev->main_info.hif_hook.trigger_assert) {
			if (bdev->assert_reason[0] == '\0') {
				if (snprintf(bdev->assert_reason, ASSERT_REASON_SIZE , "[BT_DRV assert] cmd timeout 0x%02x%02x"
						,opcode[1], opcode[0]) < 0)
					strncpy(bdev->assert_reason, "[BT_DRV assert] cmd timeout",
						strlen("[BT_DRV assert] cmd timeout") + 1);
				BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
			}
			bdev->main_info.hif_hook.trigger_assert(bdev);
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

#if (USE_DEVICE_NODE == 0)
	/* When FW own, FW will wait DRV own before send data to host
	 * If disable uarthub, fw will trigger EINT to notify host
	 * If enable uarthub, fw will send 0xFF and 0xF0 to host
	 */
	cif_dev->ap_cfg.detail.hub_mode_en = 1;
#endif

	/* setting config */
	{
		/* inband enable & fw inband picus enable*/
		cif_dev->ap_cfg.detail.inband_picus_fc_en =
			(cif_dev->ap_cfg.detail.inband_en & cif_dev->fw_cfg.detail.inband_picus_fc_en);
		/* host register uart callback & fw feature enable */
		cif_dev->ap_cfg.detail.wakeup_enh_en =
			(cif_dev->uart_wakeup_cb_en & cif_dev->fw_cfg.detail.wakeup_enh_en);
		/* control by fw */
		cif_dev->ap_cfg.detail.picus_fc_en = cif_dev->fw_cfg.detail.picus_fc_en;
	}

	cmd[hub_crc_rhw_offset] = cif_dev->ap_cfg.setting_byte;

	ret = btmtk_main_send_cmd(bdev,
			cmd, cmd_len, event, SETBAUD_EVT_LEN, 0,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		return ret;
	}

	cif_dev->uart_baudrate_set = 1;
	BTMTK_INFO("%s done, fc_offset[%d], hub_crc_rhw_offset[%d], setting_byte[0x%x]",
			__func__, fc_offset, hub_crc_rhw_offset, cmd[hub_crc_rhw_offset]);

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

#if (USE_DEVICE_NODE == 1)
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
#endif

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
		BTMTK_INFO("%s: chip_id = 0x%x", __func__, bdev->chip_id);
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

	BTMTK_DBG("%s done", __func__);
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
	BTMTK_DBG("%s done", __func__);
exit:
	vfree(cmd);
	return ret;
}
#endif

int btmtk_uart_set_tty_termios(struct btmtk_dev *bdev, int baudrate, enum UART_FC fc) {

	struct tty_struct *tty;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct ktermios new_termios;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	tty = cif_dev->tty;

	if (tty == NULL) {
		BTMTK_ERR("%s: tty  is NULL", __func__);
		return -1;
	}

	new_termios = tty->termios;

	/* set tty host baud and flowcontrol to config setting */
	BTMTK_INFO("set uart cfg: baud[%d]/fc[%d]", baudrate, fc);
	tty_termios_encode_baud_rate(&new_termios, baudrate, baudrate);

	switch (fc) {
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

	return 0;
}

int btmtk_uart_set_ap_fw_uart_cfg(struct btmtk_dev *bdev, struct UART_CONFIG *uart_cfg) {
	int ret = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	/* set chip baud and flowcontrol to config setting */
	ret = btmtk_uart_send_set_uart_cmd(bdev->hdev, uart_cfg);
	if (ret < 0) {
		BTMTK_ERR("%s: btmtk_uart_send_set_uart_cmd fail", __func__);
		return ret;
	}

	/* set tty host baud and flowcontrol to config setting */
	btmtk_uart_set_tty_termios(bdev, uart_cfg->iBaudrate, uart_cfg->fc);

	/* notify fw change baudrate and related setting */
	ret = btmtk_uart_send_wakeup_cmd(bdev->hdev);
	if (ret < 0) {
		BTMTK_ERR("%s: btmtk_uart_send_wakeup_cmd fail", __func__);
		return ret;
	}

	return ret;
}

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
	btmtk_hci_snoop_print_to_log(bdev);
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

	if (cif_dev->ap_cfg.detail.rhw_bypass) {
		/* not enable rhw yet, do hif dump */
		if (bdev->main_info.hif_hook.dump_hif_debug_sop)
			bdev->main_info.hif_hook.dump_hif_debug_sop(bdev);
		return;
	}

	/* dump debug sop before coredump */
	if (bdev->main_info.hif_hook.dump_debug_sop)
		bdev->main_info.hif_hook.dump_debug_sop(bdev);

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
		if (bdev->main_info.hif_hook.dump_hif_debug_sop)
			bdev->main_info.hif_hook.dump_hif_debug_sop(bdev);

		/* if during btmtk_pre_chip_rst_handler (BTMTK_RESET_DOING)
		 * leave hw_err to btmtk_post_chip_rst_handler
		 */
		if (atomic_read(&bdev->main_info.chip_reset) == BTMTK_RESET_DONE) {
			/* direct send hw_err event notify host to close bt */
			bdev->main_info.reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
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

static int btmtk_query_fw_uart_settings_cmd(struct btmtk_dev *bdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x04, 0x01, 0x00, 0x22 };
	u8 evt[] = { 0x04, 0xE4, 0x0B, 0x02, 0x04, 0x07, 0x00};
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	memset(bdev->io_buf, 0, IO_BUF_SIZE);

	ret = btmtk_main_send_cmd(bdev, cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s: failed, ret[%d]", __func__, ret);
		return ret;
	}

	/*	record fw setting
	 *	evt format: {0x04, 0xE4, 0x0B, 0x02, 0x04, 0x07, 0x00, 0xDD, 0x22, 0xAA, 0xAA, 0xAA, 0xAA, 0xCC}
	 *		DD:
	 *			Status (0 for success)
	 *		AA AA AA AA:
	 *			Max supported baudrate
	 *		CC:
	 *			[0] CRC supported
	 *			[1] RHW supported
	 *			[2] inband supported
	 *			[3] wakeup enhance supported
	 *			[4] hub mode supported
	 *			[5] RSV (hw mode bypass)
	 *			[6] picus flow control supported
	 *			[7] inband picus flow control supported
	 */
	memcpy(&cif_dev->fw_cfg.max_baudrate, &bdev->io_buf[9],
			sizeof(cif_dev->fw_cfg.max_baudrate));
	cif_dev->fw_cfg.query_status = bdev->io_buf[7];
	cif_dev->fw_cfg.setting_byte = bdev->io_buf[13];

	BTMTK_INFO("%s: max_baudrate[%u], setting_byte[0x%x], query_status[%d], "
				"inband_en[%d], wakeup_enh_en[%d], hub_mode_en[%d]",
			__func__, cif_dev->fw_cfg.max_baudrate,
			cif_dev->fw_cfg.setting_byte,
			cif_dev->fw_cfg.query_status,
			cif_dev->fw_cfg.detail.inband_en,
			cif_dev->fw_cfg.detail.wakeup_enh_en,
			cif_dev->fw_cfg.detail.hub_mode_en);

	return ret;
}

static int btmtk_sp_pre_open(struct btmtk_dev *bdev)
{
	struct tty_struct *tty;
	struct UART_CONFIG uart_cfg;
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = -1;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	int query_retry = 3;
	unsigned int rst_out = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	/* copy config for set ap/fw uart setting */
	uart_cfg = cif_dev->uart_cfg;
	tty = cif_dev->tty;

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
	//msleep(100);
	for (int i = 0; i < 10; i++) {
		usleep_range(10000, 11000);
		rst_out = (rst_out | (btmtk_get_rst_pin_out_state() << i));
	}
	BTMTK_INFO("dump rst_gpio 10 times [0x%x]", rst_out);

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
	atomic_set(&bdev->main_info.chip_reset, BTMTK_RESET_DONE);
	atomic_set(&bdev->main_info.subsys_reset, BTMTK_RESET_DONE);
	bdev->main_info.chip_reset_flag = 0;
	atomic_set(&bdev->assert_state, BTMTK_ASSERT_NONE);
	atomic_set(&bdev->receive_sleep_event, 0);
	cif_dev->rhw_fail_cnt = 0;
	reinit_completion(&bdev->dump_comp);
	reinit_completion(&bdev->dump_dfd_comp);
	bdev->is_whole_chip_reset = FALSE;
	bdev->main_info.dbg_send = 0;
	cif_dev->ap_cfg.setting_byte = 0;
	cif_dev->fw_cfg.setting_byte = 0;
#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
	btmtk_tasar_init();
#endif // CFG_SUPPORT_TAS_HOST_CONTROL

	/* set tty host baud and flowcontrol to default value */
	btmtk_uart_set_tty_termios(bdev, BT_UART_DEFAULT_BAUD, UART_DISABLE_FC);

	/* update baurdrate from dts */
	if (cif_dev->baudrate)
		uart_cfg.iBaudrate = cif_dev->baudrate;

	/* uarhub setting */
	cif_dev->ap_cfg.detail.hub_mode_en = 0;
	cif_dev->ap_cfg.detail.rhw_bypass = 1;
	cif_dev->ap_cfg.detail.crc_bypass = 1;
	cif_dev->fw_dl_ready = 0;
	cif_dev->flush_en = 1;
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	cif_dev->sleep_flow_hw_mech_en = (mtk8250_uart_hub_get_bt_sleep_flow_hw_mech_en() <= 0) ? 0 : mtk8250_uart_hub_get_bt_sleep_flow_hw_mech_en();
	BTMTK_DBG("sleep_flow_hw_mech_en: %d", cif_dev->sleep_flow_hw_mech_en);

	/* for set uart cmd, old wakeup flow use 1 */
	if (!cif_dev->sleep_flow_hw_mech_en)
		cif_dev->ap_cfg.detail.hw_mode_bypass = 1;
#endif
	/* Flush any pending characters in the driver and discipline. */
	tty_ldisc_flush(tty);

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

	/* set AP/FW chip baud and flowcontrol to config setting */
	ret = btmtk_uart_set_ap_fw_uart_cfg(bdev, &uart_cfg);
	if (ret < 0)
		goto exit;

	/* If DFD, return */
	if (bdev->reset_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP && !bdev->is_dfd_done) {
		/* set is_dfd_done */
		bdev->is_dfd_done = TRUE;
		BTMTK_INFO("%s: DFD done, return", __func__);
		return 0;
	}

	/* avoid another reset start before BT open done */
	bdev->reset_type = 0;

	/* for dfd, clear assert reason here */
	memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);

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

	/* after fw download */
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (cif_dev->hub_en && !cif_dev->hub_bypass_only) {
		/* uarhub setting */
		cif_dev->ap_cfg.detail.hub_mode_en = 1;
		cif_dev->ap_cfg.detail.crc_bypass = 0;

#if defined(CFG_INBAND_SUPPORT)
		/* check UARTHUB support inband cmd or not */
		if (mtk8250_uart_hub_inband_is_support() == 1)
			cif_dev->ap_cfg.detail.inband_en = 1;
		BTMTK_INFO("%s mtk8250_uart_hub_inband_is_support, ret[%d]",
				__func__, cif_dev->ap_cfg.detail.inband_en);
#endif // CFG_INBAND_SUPPORT

	} else
		BTMTK_INFO("%s hub_bypass_only, not send hub_en to fw", __func__);
#endif

	/* setting after fw download ready */
	cif_dev->fw_dl_ready = 1;
	cif_dev->ap_cfg.detail.rhw_bypass = 0;

	/* query fw feature configs */
	btmtk_query_fw_uart_settings_cmd(bdev);

	/* check support inband or not */
	if (!cif_dev->fw_cfg.detail.inband_en) {
		BTMTK_INFO("%s fw not support inband", __func__);
		cif_dev->ap_cfg.detail.inband_en = 0;
	}

#if !defined(CFG_INBAND_SUPPORT)
	/* disable inband config */
	BTMTK_INFO("%s inband config not enable", __func__);
	cif_dev->ap_cfg.detail.inband_en = 0;
#endif // ! CFG_INBAND_SUPPORT

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
#if defined(CFG_INBAND_SUPPORT)
		if (cif_dev->ap_cfg.detail.inband_en) {
			ret = mtk8250_uart_hub_inband_enable_ctrl(1);
			BTMTK_INFO("%s mtk8250_uart_hub_inband_enable_ctrl(1) ret[%d]", __func__, ret);
		}
#endif // CFG_INBAND_SUPPORT
	} else
		BTMTK_INFO("%s: hub_bypass_only, not set mtk8250_uart_hub_enable_bypass_mode(0)", __func__);
#endif // CONFIG_MTK_UARTHUB

	/* notify fw change baudrate and related setting */
	ret = btmtk_uart_send_wakeup_cmd(bdev->hdev);
	if (ret < 0) {
		/* err handle for fw get dirty data trigger EINT */
		ret = btmtk_uart_driver_own_cmd(bdev);
		if (ret < 0)
			goto exit;
	}

	/* bt on success, reset subsys count */
	atomic_set(&bdev->main_info.subsys_reset_conti_count, 0);

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
	struct btmtk_uart_dev *cif_dev = NULL;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

#if (USE_DEVICE_NODE == 0)
	/* FW patch will be downloaded before BT open for CE,
	 *   the own state must be set to FW own for ON/OFF SNS.
	 * Because these is a timeing issue that
	 *   the own state may be set to DRV own when BT off
	 * For example:
	 *   BT OFF -> enqueue power off cmd -> set DRV own -> send power off cmd
	 *   -> delete FW own timer -> BT OFF done
	 */
	atomic_set(&cif_dev->own_state, BTMTK_FW_OWN);
	atomic_set(&cif_dev->fw_wake, 0);
	UART_OWN_MUTEX_LOCK();
	cif_dev->no_fw_own = 1;
	UART_OWN_MUTEX_UNLOCK();
	btmtk_uart_create_fw_own_timer(cif_dev);
#else
#if (SLEEP_ENABLE == 1)
	BTMTK_DBG("%s init to driver own state", __func__);
	/* not start fw_own_timer until bt open done */
	atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_UKNOWN);
#if (USE_DEVICE_NODE == 0)
	if (bdev->bt_cfg.drv_own_wakelock_enable)
#endif
		__pm_stay_awake(bt_trx_wakelock);
	atomic_set(&cif_dev->own_state, BTMTK_DRV_OWN);
#endif

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

#if (USE_DEVICE_NODE == 0)
	UART_OWN_MUTEX_LOCK();
	cif_dev->no_fw_own = 0;
	UART_OWN_MUTEX_UNLOCK();
#else
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
				BTMTK_INFO("%s: Set DEV0_HOST_AWAKE ret[%d]", __func__, ret);

				/* 28. If DEV0_BT_AWAKE is 0, send 0xFF */
				if (mtk8250_uart_hub_get_bt_awake_sta() == 0) {
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

#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
	/* for SQC test, let fw test feature in every scenario
	 * use countrycode=TW, eci=0
	 */
	//btmtk_set_tasar_from_host(bdev, 0x5457, 0);
#endif // CFG_SUPPORT_TAS_HOST_CONTROL
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
#if (USE_DEVICE_NODE == 0)
	struct btmtk_uart_dev *cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	cif_dev->subsys_reset = 1;
	btmtk_deregister_hci_device(bdev);
#endif
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
			mtk8250_set_flush_flag(cif_dev->tty, true);
			tty_driver_flush_buffer(cif_dev->tty);
			mtk8250_set_flush_flag(cif_dev->tty, false);
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
		u8 *fwbuf, int section_dl_size, int section_offset, int patch_flag)
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
				end_time = jiffies + msecs_to_jiffies(TIME_BOUND_OF_FW_PKG_DL);
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
	int ret = -1, flush_retry = 0;
	u32 len = 0, count = 0;
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
#if IS_ENABLED(CONFIG_SUPPORT_UARTDBG)
		/* set tty runtime_status = RPM_ACTIVE, avoid the problem from tty kworker scheduling */
		mtk8250_set_runtime_active_status(cif_dev->tty);
#endif
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
	btmtk_hci_snoop_save(bdev, HCI_SNOOP_TYPE_TX_ISO_HIF, cmd, cmd_len);
#endif
	if (cif_dev->tty)
		BTMTK_DBG_RAW(bdev, cmd, cmd_len,
					"%s, len[%d] write_retry[%d] room[%d] flush_retry[%d] CMD :",
					__func__, cmd_len,
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

	if (!skb_queue_empty(&cif_dev->tx_queue)) {
		ret |= BTMTK_THREAD_TX;
		BTMTK_DBG("%s tx_queue have data", __func__);
	}
#if (SLEEP_ENABLE == 1)
	if (atomic_read(&cif_dev->need_drv_own)) {
		//BTMTK_DBG("%s: set drv own", __func__);
		atomic_set(&cif_dev->need_drv_own, 0);
		ret |= BTMTK_THREAD_RX;
		BTMTK_DBG("%s need_drv_own", __func__);
	}

	if (atomic_read(&cif_dev->fw_own_timer_flag) == FW_OWN_TIMER_RUNNING) {
		//BTMTK_DBG("%s: set fw own", __func__);
		/* incase of tx_thread keep running for FW_OWN_TIMER_RUNNING */
		atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_DONE);
		ret |= BTMTK_THREAD_FW_OWN;
		BTMTK_DBG("%s fw_own_timer_flag need fw own", __func__);
	}
#endif

	if (atomic_read(&cif_dev->need_assert)) {
		BTMTK_DBG("%s: need_assert", __func__);
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
		int ret = wait_event_interruptible(tx_wait_q,
				  (thread_flag = btmtk_thread_wait_for_msg(bdev)));
		if (ret) {
			BTMTK_WARN("%s: wait_event_interruptible was interrupted, ret = %d", __func__, ret);
			break;
		}

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
			if (atomic_read(&cif_dev->own_state) != BTMTK_DRV_OWN) {
				BTMTK_WARN_LIMITTED("%s not in dirver_own state[%d] can not send cmd",
					__func__, atomic_read(&cif_dev->own_state));
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

void btmtk_fwlog_flow_ctrl_work(struct work_struct *work)
{
	int ret = 0;
	struct btmtk_main_info *main_info = container_of(work, struct btmtk_main_info, fwlog_flow_ctrl_work);
	struct btmtk_dev *bdev = container_of(main_info, struct btmtk_dev, main_info);
	struct btmtk_uart_dev *cif_dev = NULL;
	u8 *cmd = NULL;
	int cmd_len = 0;

	if (!bdev) {
		BTMTK_WARN_DEV(bdev, "%s: bdev is NULL", __func__);
		return;
	}

	if (btmtk_fops_get_state(bdev) != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_INFO_DEV(bdev, "%s: bt already closed", __func__);
		return;
	}

	if (atomic_read(&bdev->dynamic_fwdl_start)) {
		BTMTK_INFO_DEV(bdev, "%s: fw doing re-download, skip", __func__);
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (!cif_dev) {
		BTMTK_WARN_DEV(bdev, "%s: cif_dev is NULL", __func__);
		return;
	}

	/* if fw own or fw owning, skip, avoid trigger driver own */
	if (atomic_read(&cif_dev->own_state) != BTMTK_DRV_OWN && atomic_read(&cif_dev->own_state) != BTMTK_DRV_OWNING) {
		BTMTK_INFO_DEV(bdev, "%s: not in driver own[%d], skip", __func__, atomic_read(&cif_dev->own_state));
		return;
	}

	mutex_lock(&bdev->main_info.fwlog_ctrl_lock);

	/* payload + cmd header */
	cmd_len = bdev->main_info.fwlog_flow_ctrl_data_len + PATCH_HCI_HEADER_SIZE;

	/* allocate and format anchor cmd */
	cmd = kmalloc(cmd_len, GFP_KERNEL);
	if (!cmd) {
		BTMTK_ERR_DEV(bdev, "%s: alloc memory size(%d) failed", __func__, cmd_len);
		mutex_unlock(&bdev->main_info.fwlog_ctrl_lock);
		return;
	}

	/* cmd header */
	cmd[0] = 0x01;
	cmd[1] = 0x6C;
	cmd[2] = 0xFC;
	cmd[3] = bdev->main_info.fwlog_flow_ctrl_data_len;

	/* copy payload */
	memcpy(&cmd[PATCH_HCI_HEADER_SIZE], &bdev->main_info.fwlog_flow_ctrl_data[0],
			bdev->main_info.fwlog_flow_ctrl_data_len);

	mutex_unlock(&bdev->main_info.fwlog_ctrl_lock);

	BTMTK_DBG_RAW(bdev, cmd, cmd_len, "%s: len[%d] cmd:", __func__, cmd_len);

	ret = btmtk_main_send_cmd(bdev,
			cmd, cmd_len, NULL, 0,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret < 0)
		BTMTK_WARN_DEV(bdev, "%s fail", __func__);

	kfree(cmd);
	cmd = NULL;
}

#if (USE_DEVICE_NODE == 0)
static int btmtk_uart_set_pinmux(struct btmtk_dev *bdev)
{
	int err = 0;
	u32 val = 0;

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

	INIT_WORK(&bdev->reset_waker, btmtk_reset_waker);

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
#endif

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

	bdev = g_sbdev;
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

	spin_lock_init(&cif_dev->tx_lock);
	skb_queue_head_init(&cif_dev->tx_queue);

	/* start tx_thread */
	if (btmtk_tx_thread_start(bdev))
		return -EFAULT;

	cif_dev->stp_cursor = 2;
	cif_dev->stp_dlen = 0;

	mtk8250_set_flush_flag(tty, true);
	/* definition changed!! */
	if (tty->ldisc->ops->flush_buffer)
		tty->ldisc->ops->flush_buffer(tty);
	tty_driver_flush_buffer(tty);
	mtk8250_set_flush_flag(tty, false);

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
	btmtk_woble_uninitialize(bdev);
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
		cif_dev->subsys_reset = 0;
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
		memcpy(&cif_dev->uart_cfg, &uart_cfg, sizeof(struct UART_CONFIG));
		BTMTK_INFO("%s: <!!> Set BAUDRATE, fc = %u iBaudrate = %d <!!>",
				__func__, uart_cfg.fc, uart_cfg.iBaudrate);
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

#if (SLEEP_ENABLE == 1)
		cif_dev->sleep_en = 1;
		BTMTK_INFO("enable drv/fw own");
		btmtk_uart_fw_own(bdev);
#endif

		err = btmtk_woble_initialize(bdev);
		if (err < 0)
			BTMTK_ERR("btmtk_woble_initialize failed!");
		else
			BTMTK_ERR("btmtk_woble_initialize");
		err = btmtk_register_hci_device(bdev);
		if (err < 0) {
			BTMTK_ERR("btmtk_register_hci_device failed!");
			return err;
		}
		atomic_set(&bdev->main_info.subsys_reset_conti_count, 0);
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
#if (USE_DEVICE_NODE == 1)
	case HCIUARTXOPARAM:
		if (copy_from_user(&sConnxo_cfg, (CONNXO_CFG_PARAM_STRUCT __user *)arg, sizeof(CONNXO_CFG_PARAM_STRUCT)))
			return -ENOMEM;
		BTMTK_INFO("%s: <!!> Set HCIUARTXOPARAM <!!>", __func__);
		BTMTK_INFO("%s: ucDataLenLSB = %02x, ucDataLenMSB = %02x", __func__, sConnxo_cfg.ucDataLenLSB, sConnxo_cfg.ucDataLenMSB);
		break;
#endif
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
		memcpy(&cif_dev->uart_cfg, &uart_cfg, sizeof(struct UART_CONFIG));
		BTMTK_INFO("%s: <!!> Set BAUDRATE, fc = %u iBaudrate = %d <!!>",
				__func__, uart_cfg.fc, uart_cfg.iBaudrate);
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

static int match_dirty_data(const u8 *data_ptr, size_t count)
{
	const u8 expected_tail[10] = { 0x04, 0xE4, 0x07,
		0x02, 0x03, 0x03, 0x00, 0x00, 0x03, 0x01};
	int i = 0;

	if (data_ptr == NULL || count != 73)
		return 0;

	for (i = 1; i < 63; i++) {
		if (data_ptr[i] != data_ptr[0])
			return 0;
	}

	if (memcmp(data_ptr + 63, expected_tail, sizeof(expected_tail)) != 0)
		return 0;

	return 1;
}

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

	/* record data from tty */
	btmtk_hci_snoop_save(bdev, HCI_SNOOP_TYPE_EVT_HIF, data, count);

	fstate = btmtk_fops_get_state(bdev);
	state = btmtk_get_chip_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_CLOSED
#if (USE_DEVICE_NODE == 0)
	 && state != BTMTK_STATE_FW_DUMP && state != BTMTK_STATE_SUBSYS_RESET
#endif
	) {
#if (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
		BTMTK_INFO_RAW(bdev, data, count, "[SKIP] %s: count[%d]", __func__, count);
#else
		BTMTK_INFO_RAW(bdev, data, count, "[SKIP] %s: count[%zu]", __func__, count);
#endif
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;

#if (KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE)
	BTMTK_DBG_RAW_ALL(bdev, data, count, "%s: len[%d]", __func__, count);
#else
	BTMTK_DBG_RAW_ALL(bdev, data, count, "%s: len[%zu]", __func__, count);
#endif

#if (SLEEP_ENABLE == 1)
	//BTMTK_INFO_RAW(bdev, data, count, "%s: count[%d]", __func__, count);

	/* if flag is BTMTK_FW_OWNING not set driver own , because data is fw own event */
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
	if (atomic_read(&cif_dev->own_state) == BTMTK_FW_OWN &&
	    !atomic_read(&cif_dev->fw_wake) &&
	    data != NULL && count > 0 && data[0] != 0xFF && cif_dev->sleep_en) {
		unsigned int index = 0;
		size_t _count = count;
		u8 *buf = (u8 *)data;

		BTMTK_INFO_RAW(bdev, data, count, "%s: recv none 0xFF as first byte in FW own state", __func__);
		while (_count && *(buf + index) != 0xFF) {
			index++;
			_count--;
		}

		if (_count > 0) {
			count = _count;
			data = buf + index;
		} else
			return;
		BTMTK_INFO_RAW(bdev, data, count, "%s: data after trim", __func__);
	}
	/* Filter 0x 0x 0x ... */
	if (atomic_read(&cif_dev->own_state) == BTMTK_DRV_OWNING && count == 73 &&
		match_dirty_data(data, count) == 1 && cif_dev->sleep_en) {
		BTMTK_INFO_RAW(bdev, data, count, "%s: recv 0x 0x 0x ...", __func__);
		data = data + 63;
		count = 10;

		BTMTK_INFO_RAW(bdev, data, count, "%s: data after trim", __func__);
	}
#endif
	if (data != NULL && (count > 1 || data[0] != 0x00) && atomic_read(&cif_dev->own_state) != BTMTK_FW_OWNING) {
		if (
#if (USE_DEVICE_NODE == 0)
			/* Drop driver own request from FW when enter suspend */
			btmtk_get_chip_state(bdev) != BTMTK_STATE_SUSPEND
#else
			/* Drop driver own request from FW when during suspend */
			!atomic_read(&bdev->main_info.suspend_entry)
#endif
			) {
			atomic_set(&cif_dev->need_drv_own, 1);
			atomic_set(&cif_dev->fw_wake, 1);
			wake_up_interruptible(&tx_wait_q);
		} else {
			BTMTK_INFO_RAW(bdev, data, count, "%s: drop driver own request when suspend,", __func__);
		}
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

	/* record data from tty */
	btmtk_hci_snoop_save(bdev, HCI_SNOOP_TYPE_EVT_HIF_DONE, data, count);
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

	/* ack fw log flow control before fw own */
	flush_work(&bdev->main_info.fwlog_flow_ctrl_work);

	UART_OWN_MUTEX_LOCK();
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	/* no need to compare BTMTK_FW_OWNING because the state must be fw_own/fail before leaving mutex */
	if (atomic_read(&cif_dev->own_state) == BTMTK_FW_OWN || atomic_read(&cif_dev->own_state) == BTMTK_OWN_FAIL) {
		BTMTK_WARN("Already at fw own state or error state[%d], skip", atomic_read(&cif_dev->own_state));
		goto unlock;
	}

	if (cif_dev->no_fw_own == 1) {
		BTMTK_WARN("no_fw_own is set");
		goto unlock;
	}

	BTMTK_DBG("%s: fw owning", __func__);
	atomic_set(&cif_dev->own_state, BTMTK_FW_OWNING);
	if (atomic_read(&event_compare_status) == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE ||
			!skb_queue_empty(&cif_dev->tx_queue) || atomic_read(&bdev->dynamic_fwdl_start)) {
		BTMTK_WARN_DEV(bdev, "%s: during send_and_recv[%d] or tx queue have data[%d] "
				"or dynamic_fwdl_start[%d], keep drv own",
				__func__, atomic_read(&event_compare_status), !skb_queue_empty(&cif_dev->tx_queue),
				atomic_read(&bdev->dynamic_fwdl_start));
		atomic_set(&cif_dev->own_state, BTMTK_DRV_OWN);
		atomic_set(&bdev->receive_sleep_event, 0);
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
		/* evt[7] = 1 for no sleep, evt[8] = 3 represent drv own evt */
		if (ret == 0 && (bdev->io_buf[7] || bdev->io_buf[8] == 0x03)) {
			/* re-set tx request */
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
			if (cif_dev->hub_en)
				btmtk_wakeup_uarthub();
#endif
			BTMTK_WARN("%s fw not allow sleep or receive drv own evt, keep drv own, cmd[9] = 0x%02x, evt[7] = %d, evt[8] = %d",
						__func__, cmd[9], bdev->io_buf[7], bdev->io_buf[8]);
			atomic_set(&cif_dev->own_state, BTMTK_DRV_OWN);
			atomic_set(&bdev->receive_sleep_event, 0);
			btmtk_uart_update_fw_own_timer(cif_dev);
			goto unlock;
		}
	} else
		ret = 0;

	if (ret < 0) {
		atomic_set(&cif_dev->own_state, BTMTK_DRV_OWN);
		BTMTK_ERR("%s: set fw own return fail, ret[%d]", __func__, ret);
		if (bdev->assert_reason[0] == '\0') {
			strncpy(bdev->assert_reason, "[BT_DRV assert] fw own failed",
					strlen("[BT_DRV assert] fw own failed") + 1);
			BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
		}
		goto unlock;
	} else {
		atomic_set(&cif_dev->own_state, BTMTK_FW_OWN);
		atomic_set(&cif_dev->fw_wake, 0);

		if (atomic_read(&cif_dev->fw_own_timer_flag)) {
			BTMTK_DBG("%s fw_own_timer deleted", __func__);
#if (KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE)
			del_timer(&cif_dev->fw_own_timer);
#else
			timer_delete(&cif_dev->fw_own_timer);
#endif
		} else
			BTMTK_DBG_LIMITTED("%s: not create yet", __func__);

#if IS_ENABLED(CONFIG_MTK_UARTHUB)
		if (cif_dev->sleep_flow_hw_mech_en && cif_dev->sleep_en) {
			/*
			 *	12 Clear DEVx_HOST_AWAKE = 0
			 *	14 Clear TX/RX REQ
			 */
			BTMTK_DBG_LIMITTED("%s For new sleep wakeup", __func__);
			btmtk_release_uarthub(true);
		} else {
			btmtk_release_uarthub(false);
		}
#endif
		/* no need to judge suspend_entry here
		 * wakelock itself has a detection count mechanism
		 */
#if (USE_DEVICE_NODE == 0)
		if (bdev->bt_cfg.drv_own_wakelock_enable)
#endif
			__pm_relax(bt_trx_wakelock);

		BTMTK_DBG("%s: success, cmd[9] = 0x%02x, receive_sleep_event = %d", __func__, cmd[9], atomic_read(&bdev->receive_sleep_event));
	}
unlock:
	/* use fw own mutex to protect flag
	 * fw log may appear during suspend,
	 * which may also cause driver own at the same time
	 */
	atomic_set(&bdev->main_info.suspend_entry, 0);
	atomic_set(&bdev->receive_sleep_event, 0);
	atomic_set(&bdev->have_host_tx, 0);
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
	unsigned long start_time = jiffies;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	UART_OWN_MUTEX_LOCK();
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (atomic_read(&cif_dev->own_state) == BTMTK_DRV_OWN || atomic_read(&cif_dev->own_state) == BTMTK_OWN_FAIL) {
		//BTMTK_WARN("Already at driver own state or error state[%d], skip", cif_dev->own_state);
		btmtk_uart_update_fw_own_timer(cif_dev);
		goto unlock;
	}

	atomic_set(&cif_dev->own_state, BTMTK_DRV_OWNING);

	/* get wakelock */
#if (USE_DEVICE_NODE == 0)
	if (bdev->bt_cfg.drv_own_wakelock_enable)
#else
	/* notify fw suspend state feature */
	/* if during suspend, skip get wakelock */
	if (!atomic_read(&bdev->main_info.suspend_entry))
#endif
		__pm_stay_awake(bt_trx_wakelock);
	else
		BTMTK_INFO_DEV(bdev, "%s: skip get wakelock", __func__);

	/* wait uart resume done */
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
			atomic_set(&cif_dev->own_state, BTMTK_OWN_FAIL);
			goto unlock;
		}
	}
#endif

	retry = BTMTK_MAX_WAKEUP_RETRY;
	/* if fw already coredump, no need to send drv own cmd */
	if (cif_dev->sleep_en && btmtk_get_chip_state(bdev) != BTMTK_STATE_FW_DUMP) {

		/* if fw already wake, no need to send 0xFF and wait 6ms before clr fw own */
		if (atomic_read(&cif_dev->fw_wake)
#if (USE_DEVICE_NODE == 1)
			&& !cif_dev->sleep_flow_hw_mech_en
#endif
		) {
			/* wait a while for avoid rx pkt error */
			usleep_range(4000, 4100);
		} else {
			u8 wakeup_cmd[] = { 0xFF };
#if (USE_DEVICE_NODE == 1)
			if (bdev->is_eap) {
				if (!cif_dev->sleep_flow_hw_mech_en)
					ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, evt, OWNTYPE_EVT_LEN, DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
			} else {
#if IS_ENABLED(CONFIG_MTK_UARTHUB)
				if (cif_dev->sleep_flow_hw_mech_en) {
					int retry_cnt = 1000;
					struct bt_utc_struct record_set_host_awake_time, record_ff_time, record_start_query_time;

					btmtk_getUTCtime(&record_set_host_awake_time);
					/* new wakeup flow */
					/* 3. Set DEVx_HOST_AWAKE = 1 */
					/* 4. Tx 0xFF, if DEVx_BT_AWAKE = 0 */
					/* 4~8 Polling CMM_BT_AWAKE = 1 or DEVx_BT_AWAKE = 1 */
					ret = mtk8250_uart_hub_set_host_awake_sta(0);
					BTMTK_DBG("%s: Set DEVx_HOST_AWAKE = 1, ret = %d ", __func__, ret);

					if (mtk8250_uart_hub_get_host_bt_awake_sta(0) == 0) {
						btmtk_getUTCtime(&record_ff_time);
						BTMTK_DBG("%s: host_bt_awake_sta = 0, send 0xFF ", __func__);
						ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, NULL, 0,
										DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
						/* make sure 0xff is sent */
						btmtk_uart_wait_tty_buffer_clean(bdev, FALSE);
					}
					btmtk_getUTCtime(&record_start_query_time);
					while (mtk8250_uart_hub_get_cmm_bt_awake_sta() == 0 && mtk8250_uart_hub_get_host_bt_awake_sta(0) == 0 && retry_cnt) {
						/* If CMM_BT_AWAKE = 1 or DEVx_BT_AWAKE = 1 leave while */
						retry_cnt--;
						usleep_range(1000, 1100);
					}
					BTMTK_INFO("%s: CMM_BT_AWAKE[%d], DEV0_BT_AWAKE[%d], retry_cnt[%d], set_host_awake_time[%6lu.%06lu], "
							"send_ff_time[%6lu.%06lu], start_query_time[%6lu.%06lu]",
							__func__, mtk8250_uart_hub_get_cmm_bt_awake_sta(), mtk8250_uart_hub_get_host_bt_awake_sta(0),
							retry_cnt, record_set_host_awake_time.ksec, record_set_host_awake_time.knsec,
							record_ff_time.ksec, record_ff_time.knsec,
							record_start_query_time.ksec, record_start_query_time.knsec);

					if (mtk8250_uart_hub_get_cmm_bt_awake_sta() == 0 && mtk8250_uart_hub_get_host_bt_awake_sta(0) == 0 && !retry_cnt) {
						BTMTK_ERR("%s: Query BT_AWAKE failed, fw_awake = %d", __func__, atomic_read(&cif_dev->fw_wake));
						mtk8250_uart_hub_dump_with_tag("BT driver query BT_AWAKE failed");
						mtk8250_uart_dump(cif_dev->tty);
					}
#endif
				} else {
#endif
					int i = 0;

					for (i = 0; i < 3; i++) {
						/* no need to wait event */
						ret = btmtk_main_send_cmd(bdev, wakeup_cmd, DRVOWN_CMD_LEN, NULL, 0,
							DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
						/* wait a while for fw wakeup */
						usleep_range(6000, 6100);
					}
#if (USE_DEVICE_NODE == 1)
				}
			}
#endif
			if (ret < 0)
				BTMTK_ERR("%s btmtk_main_send_cmd 0xFF fail, ret[%d]", __func__, ret);
		}
		if (cif_dev->sleep_flow_hw_mech_en) {
			ret = btmtk_main_send_cmd(bdev, fw_own_clr_cmd, 10, evt, OWNTYPE_EVT_LEN,
					DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
			if (ret < 0)
				BTMTK_ERR("%s fw_own_clr_cmd fail retry[%d]", __func__, retry);
		} else {
			do {
				/* fw own clr cmd for notice is wakeup by bt driver */
				/* let retry = 0 for only wait for event 500ms */
				ret = btmtk_main_send_cmd(bdev, fw_own_clr_cmd, 10, evt, OWNTYPE_EVT_LEN,
						DELAY_TIMES, SEND_RETRY_ONE_TIMES_500MS, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
				if (ret < 0)
					BTMTK_ERR("%s fw_own_clr_cmd fail retry[%d]", __func__, retry);
			} while (ret < 0 && --retry);
		}
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
		atomic_set(&cif_dev->own_state, BTMTK_DRV_OWN);
		BTMTK_ERR("%s: set driver own return fail, ret[%d]", __func__, ret);
		if (bdev->assert_reason[0] == '\0') {
			strncpy(bdev->assert_reason, "[BT_DRV assert] drv own failed",
					strlen("[BT_DRV assert] drv own failed") + 1);
			BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
		}
		goto unlock;
	} else {
		atomic_set(&cif_dev->own_state, BTMTK_DRV_OWN);
		btmtk_uart_update_fw_own_timer(cif_dev);
#if (USE_DEVICE_NODE == 1)
		if (time_after(jiffies, start_time + msecs_to_jiffies(40)))
			BTMTK_INFO("%s success, cost %u ms", __func__, jiffies_to_msecs(jiffies - start_time));
#else
		BTMTK_DBG("%s success", __func__);
#endif
	}

unlock:
	UART_OWN_MUTEX_UNLOCK();
	return ret;
}
#endif

#if (USE_DEVICE_NODE == 0)
static int btmtk_cif_suspend(void)
{
	int ret = 0;
#if 0
	int cif_event = 0, state;
	struct btmtk_cif_state *cif_state = NULL;
#endif
	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = NULL;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct st_woble *bt_woble = NULL;

	BTMTK_INFO("%s", __func__);

	if (tty == NULL) {
		BTMTK_ERR("%s: tty is NULL, maybe not run btmtk_cif_probe yet", __func__);
		return -EAGAIN;
	}

	bdev = g_sbdev;
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -ENODEV;
	}

	if (bdev->get_hci_reset) {
		BTMTK_WARN("open flow not ready(%d), retry", bdev->get_hci_reset);
		return -EAGAIN;
	}

	if (bdev->main_info.reset_stack_flag) {
		BTMTK_WARN("reset stack flag(%d), retry", bdev->main_info.reset_stack_flag);
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
#if 0
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
#endif
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev) {
		bt_woble = &cif_dev->bt_woble;
		ret = btmtk_woble_suspend(bt_woble);
		if (ret < 0)
			BTMTK_ERR("%s: btmtk_woble_suspend return fail %d", __func__, ret);
	}

	ret = btmtk_uart_fw_own(bdev);
	BTMTK_WARN("%s: set fw own %d", __func__, ret);
#if 0
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

	struct tty_struct *tty = g_tty;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_cif_state *cif_state = NULL;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	int ret = 0;
	struct btmtk_uart_dev *cif_dev = NULL;
	struct st_woble *bt_woble = NULL;

	BTMTK_INFO("%s start", __func__);

	if (tty == NULL) {
		BTMTK_ERR("%s: tty is NULL, maybe not run btmtk_cif_probe yet", __func__);
		return -EAGAIN;
	}

	bdev = g_sbdev;
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

	BTMTK_DBG("%s end", __func__);
	return 0;
}
#endif

static int btmtk_pm_notification(struct notifier_block *this, unsigned long event, void *ptr)
{
	int ret = NOTIFY_DONE;
#if (USE_DEVICE_NODE == 0)
	int cif_event = 0, state;
	struct btmtk_cif_state *cif_state = NULL;
#endif

	BTMTK_DBG("event = %ld", event);

	/* if get into suspend flow while doing audio pinmux setting
	 * it may have chance mischange uart pinmux we want to write
	 * retry and wait audio setting done then do suspend flow
	 */
	switch (event) {
	case PM_SUSPEND_PREPARE:
#if (USE_DEVICE_NODE == 0)
		state = btmtk_get_chip_state(g_sbdev);
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

		cif_state = &g_sbdev->cif_state[cif_event];
		/* Set Entering state */
		btmtk_set_chip_state((void *)g_sbdev, cif_state->ops_enter);

		if (g_sbdev->bt_cfg.support_auto_picus == true &&
				(g_sbdev->bt_cfg.support_picus_to_host == true
				|| atomic_read(&g_sbdev->main_info.fwlog_ref_cnt) != 0)) {
			btmtk_picus_disable(g_sbdev);
		}

		ret = btmtk_uart_fw_own(g_sbdev);
		BTMTK_WARN("%s: set fw own %d", __func__, ret);
#else
#if (CFG_SUPPORT_FWLOG_OFF_WHEN_SUSPEND == 1)
		if (btmtk_fops_get_state(g_sbdev) == BTMTK_FOPS_STATE_OPENED) {
			/* Do not take wakelock to avoid interruption the suspend flow */
			atomic_set(&g_sbdev->main_info.suspend_entry, 1);
			/* notify fw suspend mode */
			btmtk_intcmd_system_status(WMT_PARA_SUSPEND);
			/* do fw own immediately */
			btmtk_uart_fw_own(g_sbdev);
		} else
			BTMTK_INFO("%s: fops[%d], not send suspend cmd", __func__, btmtk_fops_get_state(g_sbdev));
#endif // CFG_SUPPORT_FWLOG_OFF_WHEN_SUSPEND
#endif
		break;
	case PM_POST_SUSPEND:
#if (USE_DEVICE_NODE == 0)
		cif_state = &g_sbdev->cif_state[HIF_EVENT_RESUME];
		/* Set Entering state */
		btmtk_set_chip_state((void *)g_sbdev, cif_state->ops_end);
		if (g_sbdev->bt_cfg.support_auto_picus == true &&
				(g_sbdev->bt_cfg.support_picus_to_host == true
				|| atomic_read(&g_sbdev->main_info.fwlog_ref_cnt) != 0)) {
			btmtk_picus_enable(g_sbdev, 0);
		}
#else
		if (btmtk_fops_get_state(g_sbdev) == BTMTK_FOPS_STATE_OPENED) {
			ret = schedule_work(&g_sbdev->system_state_work);
			BTMTK_INFO("%s: ret[%d]", __func__, ret);
			ret = NOTIFY_DONE;
		} else
			BTMTK_INFO("%s: fops[%d], not send resume cmd", __func__, btmtk_fops_get_state(g_sbdev));
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

static void btmtk_cif_hook_register(struct btmtk_dev *bdev)
{
	struct hif_hook_ptr hook;

	BTMTK_INFO("%s", __func__);

	memset(&hook, 0, sizeof(hook));
#if (USE_DEVICE_NODE == 1)
	rx_queue_initialize();
	hook.fw_log_state = fw_log_bt_state_cb;
	hook.met_log_handler = btmtk_met_log_handler;
	hook.fw_sleep_handler = btmtk_fw_sleep_handler;
	hook.init = btmtk_connv3_sub_drv_init;
	hook.exit = btmtk_connv3_sub_drv_deinit;
	hook.dump_debug_sop = btmtk_uart_sp_dump_debug_sop;
	hook.dump_hif_debug_sop = btmtk_hif_sp_dump_debug_sop;
	hook.whole_reset = btmtk_sp_whole_chip_reset;
	hook.trigger_assert = btmtk_uart_trigger_assert;
	hook.wakeup_host = btmtk_wakeup_host;
	hook.set_xonv = btmtk_uart_set_xonv;
#else
	hook.init = btmtk_ce_init;
	//hook.enter_standby = btmtk_cif_suspend;
#endif
	hook.open = btmtk_uart_open;
	hook.close = btmtk_uart_close;
	hook.pre_open = btmtk_uart_pre_open;
	hook.open_done = btmtk_uart_open_done;
	hook.reg_read = btmtk_uart_read_register;
	hook.send_cmd = btmtk_uart_send_cmd;
	hook.send_and_recv = btmtk_uart_send_and_recv;
	hook.raw_filter = btmtk_uart_raw_filter;
	hook.event_filter = btmtk_uart_event_filter;
	hook.subsys_reset = btmtk_uart_subsys_reset;
	hook.chip_reset_notify = btmtk_uart_chip_reset_notify;
	hook.cif_mutex_lock = btmtk_uart_cif_mutex_lock;
	hook.cif_mutex_unlock = btmtk_uart_cif_mutex_unlock;
	hook.dl_dma = btmtk_uart_load_fw_patch_using_dma;
	hook.waker_notify = btmtk_uart_waker_notify;
	hook.set_para = btmtk_uart_set_para;
	btmtk_reg_hif_hook(bdev, &hook);
}

static int btmtk_cif_probe(struct tty_struct *tty)
{
	int err = 0;
	int ret = 0;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_uart_dev *cif_dev = NULL;

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
		bdev->main_info.reset_stack_flag = HW_ERR_CODE_CHIP_RESET;
		btmtk_send_hw_err_to_host(bdev);
	} else {
		btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);
	}
	/* Init completion */
	init_completion(&bdev->dump_comp);
	init_completion(&bdev->dump_dfd_comp);

	/* Init semaphore */
	sema_init(&cif_dev->evt_comp_sem, 1);
	sema_init(&cif_dev->tty_flush_sem, 1);

	/* Do HIF events */
	btmtk_cif_hook_register(bdev);
	ret = btmtk_uart_tty_probe(tty);
#if (defined(LINUX_OS) && (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)) ||	\
		(defined(ANDROID_OS) && (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE))
	bt_trx_wakelock = wakeup_source_register("bt_drv_trx");
#else
	bt_trx_wakelock = wakeup_source_register(NULL, "bt_drv_trx");
#endif


	if (bdev->main_info.hif_hook.init)
		bdev->main_info.hif_hook.init();

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

	BTMTK_INFO("%s: start", __func__);
	bdev = g_sbdev;
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
	if (bdev->main_info.hif_hook.fw_log_state)
		bdev->main_info.hif_hook.fw_log_state(bdev, BTMTK_FOPS_STATE_CLOSED);
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

	if (bdev->main_info.hif_hook.exit)
		bdev->main_info.hif_hook.exit();

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

static int __maybe_unused btmtk_uart_suspend(struct device *dev)
{
	BTMTK_DBG("%s", __func__);
#if (USE_DEVICE_NODE == 0)
	btmtk_cif_suspend();
#else
	g_sbdev->suspend_state = TRUE;
#endif
	return 0;
}

static int __maybe_unused btmtk_uart_resume(struct device *dev)
{
	BTMTK_DBG("%s", __func__);
#if (USE_DEVICE_NODE == 0)
	btmtk_cif_resume();
#else
	g_sbdev->suspend_state = FALSE;
#endif
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


static int btmtk_uart_driver_probe(struct platform_device *pdev) {
	BTMTK_DBG("%s", __func__);
	return 0;
}

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
static int btmtk_uart_driver_remove(struct platform_device *pdev)
{
	BTMTK_DBG("%s", __func__);
	return 0;
}
#else
static void btmtk_uart_driver_remove(struct platform_device *pdev)
{
	BTMTK_DBG("%s", __func__);
}
#endif

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

void btmtk_platform_driver_init(void) {
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

void btmtk_platform_driver_deinit(void) {
	BTMTK_INFO("%s", __func__);
	platform_device_unregister(btmtk_uart_device);
	platform_driver_unregister(&btmtk_uart_driver);
}

struct platform_device *btmtk_get_uart_device(void)
{
	return btmtk_uart_device;
}

int btmtk_cif_register(void)
{
	int ret = -1;
	ret = uart_register();
	if (ret < 0) {
		BTMTK_ERR("*** UART registration fail(%d)! ***", ret);
		return ret;
	}

	btmtk_platform_driver_init();

	BTMTK_DBG("%s: Done", __func__);
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
#if (USE_DEVICE_NODE == 1)
	rx_queue_destroy();
	btmtk_dump_gpio_state_unmap();
#endif
	BTMTK_INFO("%s: Done", __func__);
	return 0;
}

#if (USE_DEVICE_NODE == 1)
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

	if (!buf) {
		BTMTK_ERR("%s: buf is NULL", __func__);
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
			len = snprintf(raw_buf + strlen(raw_buf), MET_LOG_MAX_BUF_SIZE - strlen(raw_buf),
				          "%02X%02X%02X%02X,", buf[i+3], buf[i+2], buf[i+1], buf[i]);
		else if (single_data_len == 8)
			len = snprintf(raw_buf + strlen(raw_buf), MET_LOG_MAX_BUF_SIZE - strlen(raw_buf),
						  "%02X%02X%02X%02X%02X%02X%02X%02X,",
						     buf[i+7], buf[i+6], buf[i+5], buf[i+4],
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

void btmtk_fw_sleep_handler(struct btmtk_dev *bdev)
{
	struct btmtk_uart_dev *cif_dev = NULL;
	if (bdev == NULL) {
		BTMTK_ERR("bdev is NULL");
		return;
	}

	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("cif_dev is NULL, bdev=%p", bdev);
		return;
	}

	if (atomic_read(&bdev->dynamic_fwdl_start)) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return;
	}

	BTMTK_DBG("%s: Receive sleep evt and need to do fw own flow now!", __func__);
	atomic_set(&bdev->receive_sleep_event, 1);
	btmtk_uart_update_fw_own_timer(cif_dev);
	return;
}
#endif
