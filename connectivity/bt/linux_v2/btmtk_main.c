/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/input.h>
#include <linux/pm_wakeup.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/sched/clock.h>

#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_fw_log.h"
#include "btmtk_chip_if.h"
#include "btmtk_buffer_mode.h"
#include "btmtk_chip_common.h"
#if (USE_DEVICE_NODE == 1)
#include "btmtk_queue.h"
#include "btmtk_proj_sp.h"
#else
#if defined(CHIP_IF_UART_TTY)
#include "btmtk_proj_ce.h"
#endif
#endif

#define MTKBT_UNSLEEPABLE_LOCK(x, y)	spin_lock_irqsave(x, y)
#define MTKBT_UNSLEEPABLE_UNLOCK(x, y)	spin_unlock_irqsave(x, y)

/* TODO, need to modify the state mutex for each hci dev*/
static DEFINE_MUTEX(btmtk_chip_state_mutex);
#define CHIP_STATE_MUTEX_LOCK()	mutex_lock(&btmtk_chip_state_mutex)
#define CHIP_STATE_MUTEX_UNLOCK()	mutex_unlock(&btmtk_chip_state_mutex)
static DEFINE_MUTEX(btmtk_fops_state_mutex);
#define FOPS_MUTEX_LOCK()	mutex_lock(&btmtk_fops_state_mutex)
#define FOPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_fops_state_mutex)
static DEFINE_MUTEX(btmtk_handle_mutex);
#define HANDLE_MUTEX_LOCK()	mutex_lock(&btmtk_handle_mutex)
#define HANDLE_MUTEX_UNLOCK()	mutex_unlock(&btmtk_handle_mutex)
static DEFINE_MUTEX(btmtk_filter_cmd_num_mutex);
#define FILTER_CMD_NUM_LOCK()	mutex_lock(&btmtk_filter_cmd_num_mutex)
#define FILTER_CMD_NUM_UNLOCK()	mutex_unlock(&btmtk_filter_cmd_num_mutex)

#define SECTION_NUM_MAX 10

/**
 * Global parameters(mtkbt_)
 */
uint8_t btmtk_log_lvl = BTMTK_LOG_LVL_DEF;
uint8_t _raw_buf_[HCI_SNOOP_MAX_BUF_SIZE * 3 + 3];

/* To support dynamic mount of interface can be probed */
static int btmtk_intf_num = BT_MCU_MINIMUM_INTERFACE_NUM;
/* To allow g_bdev being sized from btmtk_intf_num setting */
static struct btmtk_dev **g_bdev;
struct btmtk_dev *g_sbdev;

/*btmtk main information*/
static struct btmtk_main_info main_info;

/* State machine table that clarify through each HIF events,
 * To specify HIF event on
 * Entering / End / Error
 */

#if (USE_DEVICE_NODE == 0)
static const struct btmtk_cif_state g_cif_state[] = {
	/* HIF_EVENT_PROBE */
	{BTMTK_STATE_PROBE, BTMTK_STATE_WORKING, BTMTK_STATE_DISCONNECT},
	/* HIF_EVENT_DISCONNECT */
	{BTMTK_STATE_DISCONNECT, BTMTK_STATE_DISCONNECT, BTMTK_STATE_DISCONNECT},
	/* HIF_EVENT_SUSPEND */
	{BTMTK_STATE_SUSPEND, BTMTK_STATE_SUSPEND, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_RESUME */
	{BTMTK_STATE_RESUME, BTMTK_STATE_WORKING, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_STANDBY */
	{BTMTK_STATE_STANDBY, BTMTK_STATE_STANDBY, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_SUBSYS_RESET */
	{BTMTK_STATE_SUBSYS_RESET, BTMTK_STATE_WORKING, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_WHOLE_CHIP_RESET */
	{BTMTK_STATE_FW_DUMP, BTMTK_STATE_DISCONNECT, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_FW_DUMP */
	{BTMTK_STATE_FW_DUMP, BTMTK_STATE_FW_DUMP, BTMTK_STATE_FW_DUMP},
};
#else //(USE_DEVICE_NODE == 1)
static const struct btmtk_cif_state g_cif_state[] = {
	/* HIF_EVENT_PROBE */
	{BTMTK_STATE_PROBE, BTMTK_STATE_WORKING, BTMTK_STATE_ERR},
	/* HIF_EVENT_DISCONNECT */
	{BTMTK_STATE_DISCONNECT, BTMTK_STATE_DISCONNECT, BTMTK_STATE_DISCONNECT},
	/* HIF_EVENT_SUSPEND */
	{BTMTK_STATE_SUSPEND, BTMTK_STATE_SUSPEND, BTMTK_STATE_ERR},
	/* HIF_EVENT_RESUME */
	{BTMTK_STATE_RESUME, BTMTK_STATE_WORKING, BTMTK_STATE_ERR},
	/* HIF_EVENT_STANDBY */
	{BTMTK_STATE_STANDBY, BTMTK_STATE_STANDBY, BTMTK_STATE_ERR},
	/* HIF_EVENT_SUBSYS_RESET */
	{BTMTK_STATE_SUBSYS_RESET, BTMTK_STATE_SUBSYS_RESET, BTMTK_STATE_ERR},
	/* HIF_EVENT_WHOLE_CHIP_RESET */
	{BTMTK_STATE_FW_DUMP, BTMTK_STATE_CLOSED, BTMTK_STATE_ERR},
	/* HIF_EVENT_FW_DUMP */
	{BTMTK_STATE_FW_DUMP, BTMTK_STATE_CLOSED, BTMTK_STATE_ERR},
};
#endif

void btmtk_handle_mutex_lock(struct btmtk_dev *bdev)
{
	HANDLE_MUTEX_LOCK();
}

void btmtk_handle_mutex_unlock(struct btmtk_dev *bdev)
{
	HANDLE_MUTEX_UNLOCK();
}

void btmtk_filter_cmd_num_lock(struct btmtk_dev *bdev)
{
	FILTER_CMD_NUM_LOCK();
}

void btmtk_filter_cmd_num_unlock(struct btmtk_dev *bdev)
{
	FILTER_CMD_NUM_UNLOCK();
}

void btmtk_reg_hif_chip_hook(struct hif_hook_chip_ptr *hook)
{
	memcpy(&main_info.hif_hook_chip, hook, sizeof(struct hif_hook_chip_ptr));
}
#if CFG_SUPPORT_LEAUDIO_CLK
void btmtk_le_audio_getrawtime(struct timeval *tv)
{
	struct timespec64 ts;

#if (KERNEL_VERSION(4, 18, 0) > LINUX_VERSION_CODE)
	getrawmonotonic64(&ts);
#else
	ktime_get_raw_ts64(&ts);
#endif
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec/1000;
}

irqreturn_t btmtk_le_audio_clk_irq_handler(int irq, void *dev)
{
	/* Get sys clk */
	struct timeval tv;
	struct btmtk_dev *bdev = (struct btmtk_dev *)dev;

	if (bdev->le_aud.sysclk == 0 || bdev->le_aud.irq_cnt == 0) {
		btmtk_le_audio_getrawtime(&tv);
		bdev->le_aud.sysclk = (u64)tv.tv_sec * 1000000L + tv.tv_usec;
	}
	bdev->le_aud.irq_cnt++;
#if CFG_SUPPORT_DEINT_IRQ
	mtk_deint_ack(irq);
#endif
	return IRQ_HANDLED;
}

int btmtk_le_audio_clk_enable(struct btmtk_dev *bdev)
{
#define LE_AUDIO_CLK_EN_CMD_LEN 8
#define LE_AUDIO_CLK_EN_EVT_LEN 7
#define LE_AUDIO_CLK_EN_OFFSET_METHOD 5
#define LE_AUDIO_CLK_EN_OFFSET_GPIO_NUM 6
#define LE_AUDIO_CLK_EN_OFFSET_TRIGGER_TYPE 7
	int ret = 0;
	u8 cmd[LE_AUDIO_CLK_EN_CMD_LEN] = { 0x01, 0x03, 0xFD, 0x04, 0x00, 0x00, 0x00, 0x00 };
	u8 event[LE_AUDIO_CLK_EN_EVT_LEN] = { 0x04, 0x0E, 0x04, 0x01, 0x03, 0xFD, 0x00 };

	if (bdev->bt_cfg.support_le_audio_clk == 0) {
		BTMTK_INFO("%s: not support le audio clk", __func__);
		return 0;
	}

	BTMTK_INFO("%s enter", __func__);

	if (bdev->le_aud.irq < 0) {
		cmd[LE_AUDIO_CLK_EN_OFFSET_METHOD] = 0x03; // Get clk if event received
	} else {
		cmd[LE_AUDIO_CLK_EN_OFFSET_METHOD] = 0x01; // Get clk if irq triggered

		if (bdev->bt_cfg.le_audio_clk_fw_gpio >= 0) {
			cmd[LE_AUDIO_CLK_EN_OFFSET_GPIO_NUM] = bdev->bt_cfg.le_audio_clk_fw_gpio;
		} else {
			cmd[LE_AUDIO_CLK_EN_OFFSET_GPIO_NUM] = 0x03;

			if (is_mt7902(bdev->chip_id)) {
				cmd[LE_AUDIO_CLK_EN_OFFSET_GPIO_NUM] = 0x2B;
			}
		}

		if (bdev->le_aud.irq_type == IRQF_TRIGGER_RISING || bdev->le_aud.irq_type == IRQF_TRIGGER_HIGH)
			cmd[LE_AUDIO_CLK_EN_OFFSET_TRIGGER_TYPE] = 0x01; /* default low, active high */
		else
			cmd[LE_AUDIO_CLK_EN_OFFSET_TRIGGER_TYPE] = 0x00; /* default high, active low */
	}
	BTMTK_INFO_RAW(cmd, LE_AUDIO_CLK_EN_CMD_LEN, "%s: enable clk, ", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd, LE_AUDIO_CLK_EN_CMD_LEN,
			event, LE_AUDIO_CLK_EN_EVT_LEN, 0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	BTMTK_INFO("%s exit", __func__);
	return ret;
}

static void btmtk_le_audio_clk_init(struct btmtk_dev *bdev)
{
	int ret = 0;
	struct device_node *node;
	int interrupts[2];
#if CFG_SUPPORT_DEINT_IRQ
	int deint[3];
#endif

	if (bdev->bt_cfg.support_le_audio_clk == 0) {
		BTMTK_INFO("%s: not support le audio clk", __func__);
		return;
	}

	/* Get irq info from dts */
	BTMTK_INFO("%s: enter, irq=%d", __func__, bdev->le_aud.irq);

	node = of_find_compatible_node(NULL, NULL, "mediatek,connectivity-combo");
	if (!node) {
		BTMTK_ERR("%s: get node failed", __func__);
		return;
	}

	ret = of_property_read_u32_array(node, "interrupts",
				interrupts, ARRAY_SIZE(interrupts));
	if (ret < 0) {
		BTMTK_ERR("%s: get interrupts fail(%d)", __func__, ret);
		return;
	}

	if (bdev->le_aud.irq <= 0) {
		bdev->le_aud.irq = irq_of_parse_and_map(node, 0);
		if (bdev->le_aud.irq <= 0) {
			BTMTK_INFO("%s: request irq failed", __func__);
			return;
		}
		BTMTK_INFO("%s: irq_of_parse_and_map, irq=%d", __func__, bdev->le_aud.irq);
	}

#if CFG_SUPPORT_DEINT_IRQ
	ret = request_irq(bdev->le_aud.irq, (irq_handler_t)btmtk_le_audio_clk_irq_handler,
					IRQF_TRIGGER_NONE, "BT_LE_AUDIO_CLK_DEINT_ISR_Handler", bdev);
	if (ret) {
		BTMTK_ERR("%s: request_irq DEINT fail(%d) irq_number=%d", __func__, ret, bdev->le_aud.irq);
		bdev->le_aud.irq = -1;
		return;
	}
//	ret = mt_secure_call(MTK_SIP_BT_FIQ_REG, interrupts[1], interrupts[2], 0, 0);
//	BTMTK_INFO("%s: SMC call ret(%d), num(%d), type(%d)", __func__, ret, interrupts[1], interrupts[2]);
	of_property_read_u32_array(node, "deint", deint, ARRAY_SIZE(deint));
	mtk_deint_enable(deint[0], deint[1], deint[2]);
	bdev->le_aud.irq_type = deint[2];
#else	/*generic EINT*/
	ret = request_irq(bdev->le_aud.irq, (irq_handler_t)btmtk_le_audio_clk_irq_handler,
					interrupts[1], "BT_LE_AUDIO_CLK_ISR_Handler", bdev);
	if (ret) {
		BTMTK_ERR("%s: request_irq EINT fail(%d) irq_number=%d", __func__, ret, bdev->le_aud.irq);
		bdev->le_aud.irq = -1;
		return;
	}
	bdev->le_aud.irq_type = interrupts[1];
#endif
	btmtk_le_audio_clk_enable(bdev);
}

static void btmtk_le_audio_clk_deinit(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s: enter, irq=%d", __func__, bdev->le_aud.irq);
	if (bdev->le_aud.irq > 0) {
		free_irq(bdev->le_aud.irq, bdev);
	}
}

static void btmtk_le_audio_clk_evt_handler(struct btmtk_dev *bdev, struct sk_buff *skb)
{
#define RXCLK_SYS_TIME_POS 14
#define RXPTS_SYS_TIME_POS 22
	int rxclk_sys_time_pos = 0;
	if (skb->data[1] == 0x0C) {
			rxclk_sys_time_pos = RXCLK_SYS_TIME_POS;
	} else if (skb->data[1] == 0x14) {
			rxclk_sys_time_pos = RXPTS_SYS_TIME_POS;
	} else {
			return;
	}

	if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
			skb->data[0] == 0x0E &&
			skb->data[3] == 0x03 &&
			skb->data[4] == 0xFD) {
		skb_put(skb, 5);
		skb->data[1] += 5;
		if (bdev->bt_cfg.le_audio_clk_fw_gpio >= 0 &&
			bdev->le_aud.irq > 0 &&
			bdev->le_aud.sysclk != 0 &&
			bdev->le_aud.irq_cnt == 1) {
			//main_info.le_audio_sysclk =
			//	mt_secure_call(MTK_SIP_BT_GET_CLK, 0, 0, 0, 0);
			BTMTK_DBG("%s: sysclk(%llu)", __func__, bdev->le_aud.sysclk);
			skb->data[rxclk_sys_time_pos + 4] = 0; //Add NO_IRQ_FLAG, 0:trigger by IRQ
		} else {
			struct timeval tv;
			btmtk_le_audio_getrawtime(&tv);
			bdev->le_aud.sysclk = (u64)tv.tv_sec * 1000000L + tv.tv_usec;
			skb->data[rxclk_sys_time_pos + 4] = 1; //Add NO_IRQ_FLAG, 1:not trigger by IRQ
		}
		/* Add SYS_CLOCK to tail
		 * OLD format: 0E LEN(1) 01 03 FD 00 HANDLE(2) BITCNT(2) BTCLK(4)
		 * NEW format: 0E LEN(1) 01 03 FD 00 HANDLE(2) BITCNT(2) BTCLK(4) SYSCLK(4)
		 */
		skb->data[rxclk_sys_time_pos] = bdev->le_aud.sysclk & 0xFF;
		skb->data[rxclk_sys_time_pos + 1] = (bdev->le_aud.sysclk >> 8) & 0xFF;
		skb->data[rxclk_sys_time_pos + 2] = (bdev->le_aud.sysclk >> 16) & 0xFF;
		skb->data[rxclk_sys_time_pos + 3] = (bdev->le_aud.sysclk >> 24) & 0xFF;
		bdev->le_aud.sysclk = 0;
		if (bdev->le_aud.irq_cnt != 1)
			BTMTK_ERR("%s: le_audio_irq_cnt(%d)", __func__, bdev->le_aud.irq_cnt);
		bdev->le_aud.irq_cnt = 0;
	}
}
#endif

__weak int btmtk_cif_register(void)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_cif_deregister(void)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_cif_send_calibration(struct btmtk_dev *bdev)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

__weak int btmtk_send_apcf_reserved(struct btmtk_dev *bdev)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

#if 0
void btmtk_do_gettimeofday(struct timeval *tv)
{
#if (KERNEL_VERSION(4, 19, 85) > LINUX_VERSION_CODE)
	do_gettimeofday(tv);
#else
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec/1000;
#endif
}
#endif
void btmtk_getUTCtime(struct bt_utc_struct *utc)
{
#if (KERNEL_VERSION(4, 19, 85) > LINUX_VERSION_CODE)
	struct timeval tv;

	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec, &utc->tm);
	utc->usec = tv.tv_usec;
	utc->sec = tv.tv_sec;
#else
	struct timespec64 ts;
	unsigned long long kerneltime;


	ktime_get_real_ts64(&ts);
	rtc_time64_to_tm(ts.tv_sec, &utc->tm);
	utc->usec = ts.tv_nsec/1000;
	utc->sec = ts.tv_sec;

	/* kernel time */
	/* ktime_get_ts64 is not accurate */
	kerneltime = sched_clock();
	utc->ksec = kerneltime/1000000000;
	utc->knsec = do_div(kerneltime, 1000000000)/1000;
#endif
	utc->tm.tm_year += 1900;
	utc->tm.tm_mon += 1;
}

int32_t btmtk_intcmd_set_fw_log(uint8_t flag)
{
#if (USE_DEVICE_NODE == 1)
	u8 fw_log_cmd[8] = { 0x01, 0x5D, 0xFC, 0x04, 0x00, 0x00, 0x02, 0x03 };
	u8 evt[] = {0x04, 0x0E, 0x08, 0x01, 0x5D, 0xFC, 0x00, 0x00, 0x00};
#else
	u8 fw_log_cmd[8] = { 0x01, 0x5D, 0xFC, 0x04, 0x02, 0x00, 0x02, 0x03 };
#endif
	int ret, retry = 20;

	BTMTK_INFO("%s log level[0x%02X]", __func__, flag);
	fw_log_cmd[7] = flag;

	while (g_sbdev->dynamic_fwdl_start && --retry) {
		BTMTK_INFO("%s: re-download on-goning, wait(%d)", __func__, retry);
		msleep(50);
	}

	if (retry <= 0) {
		BTMTK_WARN("%s re-download not done in 1s, skip", __func__);
		return 0;
	}

	ret = btmtk_main_send_cmd(g_sbdev,
#if (USE_DEVICE_NODE == 1)
			fw_log_cmd, sizeof(fw_log_cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_NO_ASSERT, CMD_NO_NEED_FILTER);
#else
			fw_log_cmd, 8, NULL, 0,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
#endif
	if (ret < 0)
		BTMTK_ERR("%s faill to send log level[0x%02X]", __func__, flag);

	return ret;
}

void btmtk_get_UTC_time_str(char *ts_str)
{
	struct bt_utc_struct utc;

	btmtk_getUTCtime(&utc);

	memset(ts_str, 0, HCI_SNOOP_TS_STR_LEN);
	(void)snprintf(ts_str, HCI_SNOOP_TS_STR_LEN,
			"%04d%02d%02d-%02d%02d%02d.%06u/Kernel:%6lu.%06lu",
			utc.tm.tm_year, utc.tm.tm_mon, utc.tm.tm_mday,
			utc.tm.tm_hour+8, utc.tm.tm_min, utc.tm.tm_sec, utc.usec,
			utc.ksec, utc.knsec);
}

/*get 1 byte only*/
int btmtk_efuse_read(struct btmtk_dev *bdev, u16 addr, u8 *value)
{
	int ret = 0;
	uint8_t sub_block_addr_in_event = 0;
	uint16_t sub_block = (addr / 16) * 4;
	uint8_t temp = 0;
	struct data_struct efuse_r = {0}, efuse_r_event = {0};

	BTMTK_INFO("%s enter", __func__);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_EFUSE_CMD, efuse_r);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, READ_EFUSE_EVT, efuse_r_event);

	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET] = sub_block & 0xFF;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 1] = (sub_block & 0xFF00) >> 8;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 2] = (sub_block + 1) & 0xFF;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 3] = ((sub_block + 1) & 0xFF00) >> 8;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 4] = (sub_block + 2) & 0xFF;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 5] = ((sub_block + 2) & 0xFF00) >> 8;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 6] = (sub_block + 3) & 0xFF;
	efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 7] = ((sub_block + 3) & 0xFF00) >> 8;

	ret = btmtk_main_send_cmd(bdev,
			efuse_r.content, efuse_r.len,
			efuse_r_event.content, efuse_r_event.len,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret) {
		BTMTK_WARN("btmtk_main_send_cmd error");
		return ret;
	}

	if (memcmp(bdev->io_buf, efuse_r_event.content, efuse_r_event.len) == 0) {
		/*compare rxbuf format ok, compare addr*/
		BTMTK_DBG("compare rxbuf format ok");
		if (efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET] == bdev->io_buf[9] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 1] == bdev->io_buf[10] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 2] == bdev->io_buf[15] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 3] == bdev->io_buf[16] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 4] == bdev->io_buf[21] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 5] == bdev->io_buf[22] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 6] == bdev->io_buf[27] &&
			efuse_r.content[READ_EFUSE_CMD_BLOCK_OFFSET + 7] == bdev->io_buf[28]) {

			BTMTK_DBG("address compare ok");
			/*Get value*/
			sub_block_addr_in_event = ((addr / 16) / 4);/*cal block num*/
			temp = addr % 16;
			BTMTK_DBG("address in block %d", temp);
			switch (temp) {
			case 0:
			case 1:
			case 2:
			case 3:
				*value = bdev->io_buf[11 + temp];
				break;
			case 4:
			case 5:
			case 6:
			case 7:
				*value = bdev->io_buf[17 + temp - 4];
				break;
			case 8:
			case 9:
			case 10:
			case 11:
				*value = bdev->io_buf[23 + temp - 8];
				break;

			case 12:
			case 13:
			case 14:
			case 15:
				*value = bdev->io_buf[29 + temp - 12];
				break;
			}
		} else {
			BTMTK_WARN("address compare fail");
			ret = -1;
		}
	} else {
		BTMTK_WARN("compare rxbuf format fail");
		ret = -1;
	}

	return ret;
}

void btmtk_free_fw_cfg_struct(struct fw_cfg_struct *fw_cfg, int count)
{
	int i = 0;

	for (i = 0; i < count; i++) {
		if (fw_cfg[i].content) {
			BTMTK_INFO("%s:kfree %d", __func__, i);
			kfree(fw_cfg[i].content);
			fw_cfg[i].content = NULL;
			fw_cfg[i].length = 0;
		} else
			fw_cfg[i].length = 0;
	}
}

void btmtk_free_setting_file(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s begin", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev == NULL", __func__);
		return;
	}

	btmtk_free_fw_cfg_struct(&bdev->bt_cfg.picus_filter, 1);
	btmtk_free_fw_cfg_struct(&bdev->bt_cfg.picus_enable, 1);
	btmtk_free_fw_cfg_struct(bdev->bt_cfg.phase1_wmt_cmd, PHASE1_WMT_CMD_COUNT);
	btmtk_free_fw_cfg_struct(bdev->bt_cfg.vendor_cmd, VENDOR_CMD_COUNT);
	btmtk_free_fw_cfg_struct(&bdev->bt_cfg.audio_cmd, 1);

	memset(&bdev->bt_cfg, 0, sizeof(bdev->bt_cfg));
	/* reset pin initial value need to be -1, used to judge after
	 * disconnected before probe, can't do chip reset
	 */
	bdev->bt_cfg.dongle_reset_gpio_pin = -1;
}

static void btmtk_initialize_cfg_items(struct btmtk_dev *bdev)
{
	BTMTK_DBG("%s begin", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}

	bdev->bt_cfg.dongle_reset_gpio_pin = 220;
	bdev->bt_cfg.support_dongle_subsys_reset = 0;
	bdev->bt_cfg.support_dongle_whole_reset = 0;
	bdev->bt_cfg.support_unify_woble = 1;
	bdev->bt_cfg.unify_woble_type = 0;
	bdev->bt_cfg.support_woble_by_eint = 0;
	bdev->bt_cfg.support_woble_for_bt_disable = 0;
	bdev->bt_cfg.support_woble_wakelock = 0;
	bdev->bt_cfg.reset_stack_after_woble = 0;
	bdev->bt_cfg.support_auto_picus = 0;
	bdev->bt_cfg.support_picus_to_host = 0;
	bdev->bt_cfg.support_bt_single_sku = 0;
#if BTMTK_ISOC_TEST
	bdev->bt_cfg.support_usb_sco_test = 0;
#endif
	bdev->bt_cfg.support_audio_setting = 0;
	bdev->bt_cfg.debug_sop_mode = 0;
	btmtk_free_fw_cfg_struct(&bdev->bt_cfg.picus_filter, 1);
	btmtk_free_fw_cfg_struct(&bdev->bt_cfg.picus_enable, 1);
	btmtk_free_fw_cfg_struct(bdev->bt_cfg.phase1_wmt_cmd, PHASE1_WMT_CMD_COUNT);
	btmtk_free_fw_cfg_struct(bdev->bt_cfg.vendor_cmd, VENDOR_CMD_COUNT);
	btmtk_free_fw_cfg_struct(&bdev->bt_cfg.audio_cmd, 1);
	bdev->bt_cfg.support_dts_subsys_rst = 0;
#if CFG_SUPPORT_LEAUDIO_CLK
	bdev->bt_cfg.support_le_audio_clk = 0;
	bdev->bt_cfg.le_audio_clk_fw_gpio = 0;
#endif

	BTMTK_DBG("%s end", __func__);
}

static void btmtk_init_memory(struct btmtk_dev *bdev)
{
	bdev->suspend_count = 0;
	bdev->tx_in_flight = 0;
	bdev->get_hci_reset = 0;
	bdev->iso_threshold = 0;
	bdev->sco_num = 0;
	bdev->isoc_altsetting = 0;
	bdev->tx_state = 0;
	bdev->need_compare_num = 0;

#if CFG_SUPPORT_LEAUDIO_CLK
	bdev->le_aud.irq = -1;
#endif
}

void btmtk_assert_wake_lock(void)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: enter", __func__);
	__pm_stay_awake(bmain_info->assert_ws);
	BTMTK_INFO("%s: exit", __func__);
}

void btmtk_assert_wake_unlock(void)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_INFO("%s: enter", __func__);
	__pm_relax(bmain_info->assert_ws);
	BTMTK_INFO("%s: exit", __func__);
}

bool btmtk_assert_wake_lock_check(void)
{
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	return bmain_info->assert_ws->active;
}

u8 btmtk_get_chip_state(struct btmtk_dev *bdev)
{
	u8 state = 0;

	CHIP_STATE_MUTEX_LOCK();
	if (bdev)
		state = bdev->interface_state;
	else
		BTMTK_ERR("%s: bdev is NULL", __func__);
	CHIP_STATE_MUTEX_UNLOCK();

	return state;
}

void btmtk_set_chip_state(struct btmtk_dev *bdev, u8 new_state)
{
	static const char * const state_msg[BTMTK_STATE_MSG_NUM] = {
		"UNKNOWN", "INIT", "DISCONNECT", "PROBE", "WORKING", "SUSPEND", "RESUME",
		"FW_DUMP", "STANDBY", "SUBSYS_RESET", "SEND_ASSERT", "CLOSED", "ERROR"
	};

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return;
	}

	if (new_state >= BTMTK_STATE_MSG_NUM) {
		BTMTK_INFO("%s: new_state invalid(%d)", __func__, new_state);
		return;
	}

	BTMTK_INFO("%s: %s(%d) -> %s(%d) dongle_index[%d]", __func__, state_msg[bdev->interface_state],
			bdev->interface_state, state_msg[new_state], new_state, bdev->dongle_index);

	CHIP_STATE_MUTEX_LOCK();
	bdev->interface_state = new_state;
	CHIP_STATE_MUTEX_UNLOCK();
}

u8 btmtk_fops_get_state(struct btmtk_dev *bdev)
{
	u8 state = 0;

	FOPS_MUTEX_LOCK();
	if (bdev)
		state = bdev->fops_state;
	else
		BTMTK_ERR("%s: bdev is NULL", __func__);
	FOPS_MUTEX_UNLOCK();

	return state;
}

static void btmtk_fops_set_state(struct btmtk_dev *bdev, u8 new_state)
{
	static const char * const fstate_msg[BTMTK_FOPS_STATE_MSG_NUM] = {
		"UNKNOWN", "INIT", "OPENING", "OPENED", "CLOSING", "CLOSED",
	};

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return;
	}

	if (new_state >= BTMTK_FOPS_STATE_MSG_NUM) {
		BTMTK_INFO("%s: new_state invalid(%d)", __func__, new_state);
		return;
	}

	BTMTK_INFO("%s: FOPS_%s(%d) -> FOPS_%s(%d)", __func__, fstate_msg[bdev->fops_state],
			bdev->fops_state, fstate_msg[new_state], new_state);
	FOPS_MUTEX_LOCK();
	bdev->fops_state = new_state;
	FOPS_MUTEX_UNLOCK();
#if (USE_DEVICE_NODE == 1)
	if (main_info.hif_hook.fw_log_state)
		main_info.hif_hook.fw_log_state(new_state);
#endif
}

#if (CFG_GKI_SUPPORT == 0)
void *btmtk_kallsyms_lookup_name(const char *name)
{
#if (USE_DEVICE_NODE == 0)
	void *addr = __symbol_get(name);

	if (addr) {
#ifdef CONFIG_ARM
#ifdef CONFIG_THUMB2_KERNEL
		/* set bit 0 in address for thumb mode */
		addr |= 1;
#endif
#endif
		__symbol_put(name);
	}
	return addr;
#else
	return NULL;
#endif
}
#endif

static void btmtk_main_info_initialize(void)
{
	u32 snoop_idx = 0;

	memset(&main_info, 0, sizeof(main_info));
	for (snoop_idx = 0; snoop_idx < HCI_SNOOP_TYPE_MAX; snoop_idx++)
		main_info.snoop[snoop_idx].index = HCI_SNOOP_ENTRY_NUM - 1;
	atomic_set(&main_info.chip_reset, BTMTK_RESET_DONE);
	atomic_set(&main_info.subsys_reset, BTMTK_RESET_DONE);

#if defined(CONFIG_MP_WAKEUP_SOURCE_SYSFS_STAT)
	main_info.assert_ws = wakeup_source_register(NULL, "btmtk_assert_wakelock");
	main_info.woble_ws = wakeup_source_register(NULL, "btmtk_woble_wakelock");
	main_info.eint_ws = wakeup_source_register(NULL, "btevent_eint");
	main_info.chip_reset_ws = wakeup_source_register(NULL, "btmtk_chip_reset_ws");
#if WAKEUP_BT_IRQ
	main_info.irq_ws = wakeup_source_register(NULL, "btevent_irq");
#endif
#elif (defined(LINUX_OS) && (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)) ||	\
	(defined(ANDROID_OS) && (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE))
	main_info.assert_ws = wakeup_source_register("btmtk_assert_wakelock");
	main_info.woble_ws = wakeup_source_register("btmtk_woble_wakelock");
	main_info.eint_ws = wakeup_source_register("btevent_eint");
	main_info.chip_reset_ws = wakeup_source_register("btmtk_chip_reset_ws");
#if WAKEUP_BT_IRQ
	main_info.irq_ws = wakeup_source_register("btevent_irq");
#endif
#else
	main_info.assert_ws = wakeup_source_register(NULL, "btmtk_assert_wakelock");
	main_info.woble_ws = wakeup_source_register(NULL, "btmtk_woble_wakelock");
	main_info.eint_ws = wakeup_source_register(NULL, "btevent_eint");
	main_info.chip_reset_ws = wakeup_source_register(NULL, "btmtk_chip_reset_ws");
#if WAKEUP_BT_IRQ
	main_info.irq_ws = wakeup_source_register(NULL, "btevent_irq");
#endif
#endif

	main_info.wmt_over_hci_header[0] = HCI_COMMAND_PKT;
	main_info.wmt_over_hci_header[1] = 0x6F;
	main_info.wmt_over_hci_header[2] = 0xFC;

	main_info.read_iso_packet_size_cmd[0] = HCI_COMMAND_PKT;
	main_info.read_iso_packet_size_cmd[1] = 0x98;
	main_info.read_iso_packet_size_cmd[2] = 0xFD;
	main_info.read_iso_packet_size_cmd[3] = 0x02;

}

struct btmtk_main_info *btmtk_get_main_info(void)
{
	return &main_info;
}

int btmtk_get_interface_num(void)
{
	return btmtk_intf_num;
}

struct btmtk_dev **btmtk_get_pp_bdev(void)
{
	return g_bdev;
}

void btmtk_free_debug_reg_struct(struct debug_reg_struct *debug_reg)
{
	int i = 0;
	int count = debug_reg->num;

	for (i = 0; i < count; i++) {
		if (debug_reg->reg[i].content) {
			BTMTK_DBG("%s:kfree %d", __func__, i);
			kfree(debug_reg->reg[i].content);
			debug_reg->reg[i].content = NULL;
			debug_reg->reg[i].length = 0;
		} else {
			debug_reg->reg[i].length = 0;
		}
	}

	kfree(debug_reg->reg);
	debug_reg->reg = NULL;
	debug_reg->num = 0;
}

static void btmtk_initialize_debug_reg_items(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s begin", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}
#if (USE_DEVICE_NODE == 0)
	btmtk_clean_debug_reg_file(bdev);
#endif
	bdev->debug_sop_reg_dump.num = 0;
	BTMTK_INFO("%s end", __func__);
}

void btmtk_clean_debug_reg_file(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s begin", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev == NULL", __func__);
		return;
	} else if (is_connac3(bdev->chip_id)) {
		BTMTK_WARN("%s: connac3 not supported", __func__);
		return;
	}

	btmtk_free_debug_reg_struct(&bdev->debug_sop_reg_dump);
	BTMTK_INFO("%s end", __func__);
}

int btmtk_load_register(char *block_name, struct debug_reg_struct *save_reg,
		u8 *searchcontent, enum debug_reg_index_len index_length)
{
	int ret = 0, i = 0;
	u16 temp_len = 0;
	u32 temp[DEBUG_REG_NUM]; /* save for total hex number */
	unsigned long parsing_result = 0;
	char *search_result = NULL;
	char *search_end = NULL;
	char search[SEARCH_LEN];
	char *next_block = NULL;
	char number[DEBUG_REG_SIZE + 1];	/* 1 is for '\0' */
	char *regnum = NULL;

	memset(search, 0, SEARCH_LEN);
	memset(temp, 0, DEBUG_REG_NUM * sizeof(u32));
	memset(number, 0, DEBUG_REG_SIZE + 1);

	if (searchcontent == NULL) {
		BTMTK_ERR("%s: Searchcontent is NULL", __func__);
		return -1;
	}

	/* search REG NUM */
	(void)snprintf(search, SEARCH_LEN, "%sNUM:", block_name);
	search_result = strstr((char *)searchcontent, search);
	if (search_result) {
		search_result = strstr(search_result, ":");
		if (search_result == NULL) {
			BTMTK_ERR("%s:regnum Incorrect format", __func__);
			return -1;
		}

		search_end = strstr(search_result, ",");
		if (search_end == NULL) {
			BTMTK_ERR("%s: regnum is NULL", __func__);
			return -1;
		}

		if (search_end - search_result < 0) {
			BTMTK_ERR("%s: Incorrect Format in %s", __func__, search);
			return -1;
		}
		regnum = kzalloc((search_end - search_result) * sizeof(char), GFP_KERNEL);
		if (regnum == NULL) {
			BTMTK_ERR("%s: Allocate memory fail", __func__);
			return -ENOMEM;
		}

		memset(regnum, 0, search_end - search_result);
		memcpy(regnum, search_result + 1, search_end - search_result - 1);
		regnum[search_end - search_result - 1] = '\0';
		ret = kstrtoul(regnum, 0, &parsing_result);
		kfree(regnum);
		if (ret != 0) {
			BTMTK_ERR("%s: kstrtoul fail: %d", __func__, ret);
			return -ENOMEM;
		}

		save_reg->reg = (struct debug_reg *)kzalloc(parsing_result * sizeof(struct debug_reg), GFP_KERNEL);
		if (save_reg->reg == NULL) {
			BTMTK_ERR("%s: Allocate memory fail", __func__);
			return -ENOMEM;
		}
		save_reg->num = (u32)parsing_result;
		BTMTK_INFO("%s: reg num is %d", __func__, save_reg->num);
	} else {
		BTMTK_ERR("%s: %s is not found", __func__, search);
		return ret;
	}

	/* search block name */
	for (i = 0; i < save_reg->num; i++) {
		temp_len = 0;
		if (index_length == DEBUG_REG_INX_LEN_2) /* EX: POWER_STATUS_01 */
			(void)snprintf(search, SEARCH_LEN, "%s%02d:", block_name, i);
		else if (index_length == DEBUG_REG_INX_LEN_3) /* EX: POWER_STATUS_001 */
			(void)snprintf(search, SEARCH_LEN, "%s%03d:", block_name, i);
		else
			(void)snprintf(search, SEARCH_LEN, "%s:", block_name);

		ret = 0;

		search_result = strstr((char *)searchcontent, search);
		if (search_result) {
			memset(temp, 0, DEBUG_REG_NUM * sizeof(u32));
			search_result = strstr(search_result, "0x");
			if (search_result == NULL) {
				BTMTK_ERR("%s: search_result is NULL", __func__);
				return -1;
			}

			/* find next line as end of this command line, if NULL means last line */
			next_block = strstr(search_result, ":");
			if (next_block == NULL)
				BTMTK_WARN("%s: if NULL means last line", __func__);

			do {
				search_end = strstr(search_result, ",");
				if (search_end == NULL) {
					BTMTK_ERR("%s: Search_end is NULL", __func__);
					break;
				}

				if (search_end - search_result != DEBUG_REG_SIZE) {
					BTMTK_ERR("%s: Incorrect Format in %s", __func__, search);
					break;
				}

				memset(number, 0, DEBUG_REG_SIZE + 1);
				memcpy(number, search_result, DEBUG_REG_SIZE);
				ret = kstrtoul(number, 0, &parsing_result);
				if (ret == 0) {
					if (temp_len >= DEBUG_REG_NUM) {
						BTMTK_ERR("%s: %s data over %d", __func__, search, DEBUG_REG_NUM);
						break;
					}
					temp[temp_len] = parsing_result;
					temp_len++;
				} else {
					BTMTK_WARN("%s: %s kstrtoul fail: %d", __func__, search, ret);
					break;
				}
				search_result = strstr(search_end, "0x");
				if (search_result == NULL) {
					BTMTK_ERR("%s: search_result is NULL", __func__);
					break;
				}
			} while (search_result < next_block || (search_result && next_block == NULL));
		} else
			BTMTK_DBG("%s: %s is not found in %d", __func__, search, i);

		if (temp_len && temp_len < DEBUG_REG_NUM) {
			BTMTK_DBG("%s: %s found & stored in %d", __func__, search, i);
			save_reg->reg[i].content = (u32 *)kzalloc(temp_len * sizeof(u32), GFP_KERNEL);
			if (save_reg->reg[i].content == NULL) {
				BTMTK_ERR("%s: Allocate memory fail(%d)", __func__, i);
				return -ENOMEM;
			}
			memcpy(save_reg->reg[i].content, temp, temp_len * sizeof(u32));
			save_reg->reg[i].length = temp_len;
			BTMTK_DBG("%s: %s has found %d stored , value0 is %08x",
					__func__, block_name, temp_len, temp[0]);
		}
	}

	return ret;
}

void btmtk_load_debug_sop_register(char *debug_sop_name, struct device *dev, struct btmtk_dev *bdev)
{
	int err;
	u32 code_len = 0;

	if (is_connac3(bdev->chip_id)) {
		BTMTK_WARN("%s: connac3 chip not supported", __func__);
		return;
	}

	btmtk_initialize_debug_reg_items(bdev);

	err = btmtk_load_code_from_setting_files(debug_sop_name, dev, &code_len, bdev);
	if (err) {
		BTMTK_WARN("btmtk_load_code_from_setting_files failed!!");
		goto LOAD_END;
	} else {
	/* load from file */
		err = btmtk_load_register("DEBUG_REG_",
			&bdev->debug_sop_reg_dump, bdev->setting_file, DEBUG_REG_INX_LEN_3);
		if (err)
			goto LOAD_END;
		BTMTK_INFO("btmtk_load_debug_sop_register from .bin!!");
	}

LOAD_END:
	/* release setting file memory */
	kfree(bdev->setting_file);
	bdev->setting_file = NULL;

	if (err)
		BTMTK_ERR("%s: error return %d", __func__, err);
}

void btmtk_hci_snoop_print_to_log(void)
{
	u8 counter, index, snoop_index;
	char *snoop_str[HCI_SNOOP_TYPE_MAX] = {
#if (USE_DEVICE_NODE == 0)
		"Command from stack",
		"Command to FW",
		"Event to stack",
		"Event From FW",
		"ADV Event to stack",
		"ADV Event From FW",
		"NOCP Event to stack",
		"NOCP Event From FW",
		"TX ACL from stack",
		"TX ACL to FW",
		"RX ACL to stack",
		"RX ACL From FW",
		"TX ISO from stack",
		"TX ISO to FW",
		"RX ISO to stack",
		"RX ISO From FW"
#else
		"Tx_CMD_from_Stack",
		"Tx_from_Drv",		/* for sp uart */
		"Rx_EVT_to_Stack",
		"Rx_from_TTY",		/* for sp uart */
		"ADV_EVT_to_Stack",
		"ADV_EVT_from_FW",
		"NOCP_EVT_to_Stack",
		"NOCP_EVT_from_FW",
		"Tx_ACL_from_Stack",
		"Tx_ACL_to_FW",
		"Rx_ACL_to_Stack",
		"Rx_ACL_from_FW",
		"Tx_ISO_from_Stack",
		"Tx_to_TTY",		/* for sp uart */
		"Rx_ISO_to_Stack",
		"Rx_ISO_from_FW"
#endif
		};


	for (snoop_index = 0; snoop_index < HCI_SNOOP_TYPE_MAX; snoop_index++) {
		BTMTK_INFO("*** Dump %s *** (Using A5 A5 to separator the head 32 bytes and the tail 32 bytes data)",
			snoop_str[snoop_index]);
		if (main_info.snoop[snoop_index].index >= (HCI_SNOOP_ENTRY_NUM - 1))
			index = 0;
		else
			index = main_info.snoop[snoop_index].index + 1;
		for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
			if (main_info.snoop[snoop_index].len[index] > 0)
				BTMTK_INFO_RAW(main_info.snoop[snoop_index].buf[index],
					main_info.snoop[snoop_index].len[index],
					"[%s] time(%s)-act_len(%d)-len(%d):", snoop_str[snoop_index],
					main_info.snoop[snoop_index].timestamp[index],
					main_info.snoop[snoop_index].actual_len[index],
					main_info.snoop[snoop_index].len[index]);
			index++;
			if (index >= HCI_SNOOP_ENTRY_NUM)
				index = 0;
		}
	}
}

void btmtk_hci_snoop_save(unsigned int type, const u8 *buf, u32 len)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;
	u32 copy_tail_len = HCI_SNOOP_BUF_SIZE;
	u8 separator_char[SEPARATOR_LEN] = {0xA5, 0xA5};
	const u8 *copy_tail_buf;

	if (!buf || len == 0 || type >= HCI_SNOOP_TYPE_MAX) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return;
	}

	if (main_info.snoop[type].index < HCI_SNOOP_ENTRY_NUM) {
		if (len < HCI_SNOOP_BUF_SIZE) {
			copy_len = len;
			copy_tail_len = 0;
		} else if (len > HCI_SNOOP_BUF_SIZE && len <= HCI_SNOOP_BUF_SIZE * 2)
			copy_tail_len = len - copy_len;

		main_info.snoop[type].len[main_info.snoop[type].index] = copy_len & 0xff;
		main_info.snoop[type].actual_len[main_info.snoop[type].index] = len & 0xffff;
		btmtk_get_UTC_time_str(main_info.snoop[type].timestamp[main_info.snoop[type].index]);
		memset(main_info.snoop[type].buf[main_info.snoop[type].index], 0, HCI_SNOOP_MAX_BUF_SIZE);
		memcpy(main_info.snoop[type].buf[main_info.snoop[type].index], buf, copy_len & 0xff);
		/* save less then 32 bytes data in the buffer tail, using A5 A5 to
		 * separator the head 32 bytes data and the tail 32 bytes data
		 */
		if (copy_tail_len > 0) {
			copy_tail_buf = buf + len - copy_tail_len;
			main_info.snoop[type].len[main_info.snoop[type].index] +=
				(copy_tail_len + SEPARATOR_LEN) & 0xff;
			memcpy(main_info.snoop[type].buf[main_info.snoop[type].index] + copy_len, separator_char,
				SEPARATOR_LEN);
			memcpy(main_info.snoop[type].buf[main_info.snoop[type].index] + copy_len + SEPARATOR_LEN,
				copy_tail_buf, copy_tail_len);
		}

		if (main_info.snoop[type].index == 0)
			main_info.snoop[type].index = HCI_SNOOP_ENTRY_NUM;
		main_info.snoop[type].index--;
	}
}

void btmtk_hci_snoop_print(const u8 *buf, u32 len)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;
	u32 copy_tail_len = HCI_SNOOP_BUF_SIZE;
	u8 separator_char[SEPARATOR_LEN] = {0xA5, 0xA5};
	const u8 *copy_tail_buf;
	u8 hci_snoop_buf[HCI_SNOOP_MAX_BUF_SIZE] = {0};
	u16 hci_snoop_len = 0;

	if (buf && len > 0) {
		if (len < HCI_SNOOP_BUF_SIZE) {
			copy_len = len;
			copy_tail_len = 0;
		} else if (len > HCI_SNOOP_BUF_SIZE && len <= HCI_SNOOP_BUF_SIZE * 2)
			copy_tail_len = len - copy_len;

		memcpy(hci_snoop_buf, buf, copy_len & 0xff);
		hci_snoop_len = copy_len & 0xff;

		/* save less then 32 bytes data in the buffer tail, using A5 A5 to
		 * separator the head 32 bytes data and the tail 32 bytes data
		 */
		if (copy_tail_len > 0) {
			copy_tail_buf = buf + len - copy_tail_len;
			hci_snoop_len += (copy_tail_len + SEPARATOR_LEN) & 0xff;
			memcpy(hci_snoop_buf + copy_len, separator_char, SEPARATOR_LEN);
			memcpy(hci_snoop_buf + copy_len + SEPARATOR_LEN,
				copy_tail_buf, copy_tail_len);
		}

		if (hci_snoop_len > 0)
			BTMTK_INFO_RAW(hci_snoop_buf, hci_snoop_len, "act_len(%d)-len(%d)-buf(%p):",
				len, hci_snoop_len, buf);
	}
}

/* HCI receive mechnism */
static inline struct sk_buff *btmtk_parse_buf(struct hci_dev *hdev,
					  struct sk_buff *skb,
					  const unsigned char *buf,
					  int size,
					  const struct h4_recv_pkt *pkts,
					  int pkts_count)
{
	struct btmtk_dev *bdev = NULL;
	/* used for print debug log*/
	const unsigned char *buffer_dbg = buf;
	int size_dbg = size;
	unsigned char *skb_tmp = NULL;

	if (hdev == NULL || buf == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return ERR_PTR(-EINVAL);
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is invalid", __func__);
		return ERR_PTR(-EINVAL);
	}
	/* Check for error from previous call */
	if (IS_ERR(skb))
		skb = NULL;
	/* BTMTK_DBG("%s, buffer[0]=0x%02X, count[%d], pkts_count[%d]", __func__, *buffer, count, pkts_count); */

	while (size) {
		int index, length;

		if (!skb) {
			/* ignore first byte as 0xFF (FW wakeup) & 0x00 (noise from uart) */
			while (size > 1 && (*buf == 0xFF || *buf == 0x00)) {
				BTMTK_INFO_LIMITTED("%s, receive 0x%02X in pkt head, pkt_len[%d]", __func__, buf[0], size);
				buf++;
				size--;
			}

			for (index = 0; index < pkts_count; index++) {
				if (buf[0] == (&pkts[index])->type) {
					skb = bt_skb_alloc(
						(&pkts[index])->maxlen,
						   GFP_KERNEL);
					if (!skb) {
						BTMTK_ERR("%s alloc failed!",
							__func__);
						return ERR_PTR(-ENOMEM);
					}

					hci_skb_pkt_type(skb) = (&pkts[index])->type;
					hci_skb_expect(skb) = (&pkts[index])->hlen;
					break;
				}
			}

			/* Check for invalid packet type */
			if (!skb) {
				if (*buf == 0xFF || *buf == 0x00) {
					BTMTK_INFO_LIMITTED("%s: skip 0x%02X pkt", __func__, *buf);
					return NULL;
				}

				BTMTK_ERR("%s,skb is invalid, buffer[0] = 0x%02X!", __func__,
					buf[0]);
				if (main_info.hif_hook_chip.err_handler)
					main_info.hif_hook_chip.err_handler(hdev,
							buf, size, buffer_dbg, size_dbg);

				return ERR_PTR(-EILSEQ);
			}

			size -= 1;
			buf += 1;
		}

		length = min_t(uint, hci_skb_expect(skb) - skb->len, size);
		skb_tmp = skb_put(skb, length);
		if (!skb_tmp) {
			BTMTK_ERR("%s, skb_put failed. Len = %d!", __func__, length);
			kfree_skb(skb);
			skb = NULL;
			return ERR_PTR(-ENOMEM);
		}
		memcpy(skb_tmp, buf, length);
		size -= length;
		buf += length;

		if (skb->len < hci_skb_expect(skb))
			continue;

		for (index = 0; index < pkts_count; index++) {
			if (hci_skb_pkt_type(skb) == (&pkts[index])->type)
				break;
		}

		if (index >= pkts_count) {
			BTMTK_ERR("%s, type is invalid!", __func__);
			if (main_info.hif_hook_chip.err_handler)
				main_info.hif_hook_chip.err_handler(hdev,
						buf, size, buffer_dbg, size_dbg);
			kfree_skb(skb);
			skb = NULL;
			return ERR_PTR(-EILSEQ);
		}

		if (skb->len == (&pkts[index])->hlen) {
			u16 dlen;
			/* BTMTK_DBG("%s begin, skb->len = %d, %d, %d", __func__, skb->len, */
			switch ((&pkts[index])->lsize) {
			case 0:
				/* No variable data length */
				dlen = 0;
				break;
			case 1:
				dlen = skb->data[(&pkts[index])->loff];
				hci_skb_expect(skb) += dlen;

				if (skb_tailroom(skb) < dlen) {
					BTMTK_ERR("%s, maxlen[%d] skb_tailroom[%d] is not enough, dlen:%d!",
						__func__, (&pkts[index])->maxlen, skb_tailroom(skb), dlen);
					BTMTK_INFO_RAW(skb->data, skb->len, "%s, drop pkt len = [%d]:", __func__, skb->len);
					if (main_info.hif_hook_chip.err_handler)
						main_info.hif_hook_chip.err_handler(hdev,
								buf, size, buffer_dbg, size_dbg);
					kfree_skb(skb);
					skb = NULL;
					return ERR_PTR(-EMSGSIZE);
				}
				break;
			case 2:
				/* Double octet variable length */
				dlen = get_unaligned_le16(skb->data +
							  (&pkts[index])->loff);
				/* parse ISO packet len*/
				if ((&pkts[index])->type == HCI_ISODATA_PKT) {
					unsigned char *cp = (unsigned char *)&dlen + 1;
					*cp = *cp & 0x3F;
				}
				hci_skb_expect(skb) += dlen;

				if (skb_tailroom(skb) < dlen) {
					BTMTK_ERR("%s, maxlen[%d] skb_tailroom[%d] is not enough, dlen:%d!",
						__func__, (&pkts[index])->maxlen, skb_tailroom(skb), dlen);
					BTMTK_INFO_RAW(skb->data, skb->len, "%s, drop pkt len[%d]:", __func__, skb->len);
					if (main_info.hif_hook_chip.err_handler)
						main_info.hif_hook_chip.err_handler(hdev,
								buf, size, buffer_dbg, size_dbg);
					kfree_skb(skb);
					skb = NULL;
					return ERR_PTR(-EMSGSIZE);
				}
				break;
			default:
				/* Unsupported variable length */
				BTMTK_ERR("%s, Unsupported variable length!", __func__);
				BTMTK_INFO_RAW(skb->data, skb->len, "%s, drop pkt len[%d]:", __func__, skb->len);
				if (main_info.hif_hook_chip.err_handler)
					main_info.hif_hook_chip.err_handler(hdev,
							buf, size, buffer_dbg, size_dbg);
				kfree_skb(skb);
				skb = NULL;
				return ERR_PTR(-EILSEQ);
			}

			if (!dlen) {
				/* No more data, complete frame */
				(&pkts[index])->recv(hdev, skb);
				if (is_mt66xx(bdev->chip_id))
					btmtk_set_sleep(hdev, FALSE);
				skb = NULL;
			}
		} else {
			/* Complete frame */
			(&pkts[index])->recv(hdev, skb);
			if (is_mt66xx(bdev->chip_id))
				btmtk_set_sleep(hdev, FALSE);
			skb = NULL;
		}
	}

	return skb;
}

static const struct h4_recv_pkt mtk_recv_pkts[] = {
	{ H4_RECV_ACL,      .recv = btmtk_recv_acl },
#if (USE_DEVICE_NODE == 1)
	{ H4_RECV_SCO,      .recv = btmtk_recv_sco },
#else
	{ H4_RECV_SCO,      .recv = hci_recv_frame },
#endif
	{ H4_RECV_EVENT,    .recv = btmtk_recv_event },
	{ H4_RECV_ISO,      .recv = btmtk_recv_iso },
#if (USE_DEVICE_NODE == 1)
	{ H4_RECV_RHW_READ,	.recv = btmtk_recv_rhw},
	{ H4_RECV_RHW_WRITE,	.recv = btmtk_recv_rhw},
#endif
};

#if ENABLESTP
static inline struct sk_buff *btmtk_add_stp(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	struct btmtk_stp_hdr *stp_hdr;
	int dlen, err = 0, type = 0;
	u8 stp_crc[STP_CRC_LEN] = {0x00, 0x00};

	if (unlikely(skb_headroom(skb) < sizeof(*stp_hdr)) ||
		(skb_tailroom(skb) < MTK_STP_TLR_SIZE)) {
		BTMTK_DBG("%s, add pskb_expand_head, headroom = %d, tailroom = %d",
				__func__, skb_headroom(skb), skb_tailroom(skb));

		err = pskb_expand_head(skb, sizeof(*stp_hdr), MTK_STP_TLR_SIZE,
					   GFP_ATOMIC);
	}
	dlen = skb->len;
	stp_hdr = (void *) skb_push(skb, sizeof(*stp_hdr));
	stp_hdr->prefix = 0x80;
	stp_hdr->dlen = cpu_to_be16((dlen & 0x0fff) | (type << 12));
	stp_hdr->cs = 0;
/*	Add the STP trailer
 *	kernel version > 4.20
 *	skb_put_zero(skb, MTK_STP_TLR_SIZE);
 *	kernel version < 4.20
 */
	skb_put(skb, STP_CRC_LEN);

	return skb;
}

static const unsigned char *btmtk_split_stp(struct btmtk_dev *bdev,
		const unsigned char *data, int count, int *size)
{
	struct btmtk_stp_hdr *stp_hdr;

	/* The cursor is reset when all the data of STP is consumed out */
	if (!bdev->stp_info.dlen && bdev->stp_info.cursor >= STP_MAX_HDR_LEN) {
		bdev->stp_info.cursor = 0;
		BTMTK_ERR("reset cursor = %d", bdev->stp_info.cursor);
	}

	/* Filling pad until all STP info is obtained */
	while (bdev->stp_info.cursor < STP_MAX_HDR_LEN && count > 0) {
		bdev->stp_info.data[bdev->stp_info.cursor] = *data;
		BTMTK_ERR("fill stp format (%02x, %d, %d)",
		   bdev->stp_info.data[bdev->stp_info.cursor], bdev->stp_info.cursor, count);
		bdev->stp_info.cursor++;
		data++;
		count--;
	}

	/* Retrieve STP info and have a sanity check */
	if (!bdev->stp_info.dlen && bdev->stp_info.cursor >= STP_MAX_HDR_LEN) {
		stp_hdr = (struct btmtk_stp_hdr *)&bdev->stp_info.data[STP_HDR_OFFSET];
		bdev->stp_info.dlen = be16_to_cpu(stp_hdr->dlen) & 0x0fff;
		BTMTK_DBG("stp format (%02x, %02x)",
			   stp_hdr->prefix, bdev->stp_info.dlen);

		/* Resync STP when unexpected data is being read */
		if (stp_hdr->prefix != STP_HDR_PREFIX || bdev->stp_info.dlen > STP_MAX_PKT_LEN) {
			BTMTK_ERR("stp format unexpect (%02x, %02x)",
				   stp_hdr->prefix, bdev->stp_info.dlen);
			BTMTK_ERR("reset cursor = %d", bdev->stp_info.cursor);
			bdev->stp_info.cursor = STP_HDR_OFFSET;
			bdev->stp_info.dlen = 0;
		}
	}

	/* Directly quit when there's no data found for H4 can process */
	if (count <= 0)
		return NULL;

	/* Tranlate to how much the size of data H4 can handle so far */
	*size = min_t(int, count, bdev->stp_info.dlen);

	/* Update the remaining size of STP packet */
	bdev->stp_info.dlen -= *size;

	/* Data points to STP payload which can be handled by H4 */
	return data;
}
#endif

int btmtk_recv(struct hci_dev *hdev, const u8 *data, size_t count)
{
	struct btmtk_dev *bdev = NULL;
	const unsigned char *p_left = data;
	int sz_left = count;
	int err;
#if ENABLESTP
	const unsigned char **p_h4 = NULL;
	int sz_h4 = 0, adv = 0;
#endif

	if (hdev == NULL || data == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is NULL!", __func__);
		return -EINVAL;
	}

#ifndef CHIP_IF_UART_TTY
	if (btmtk_data_length_check((u8 *)&data[1], count, (u8)data[0])) {
		BTMTK_INFO("%s: length check fail,return", __func__);
		return -1;
	}
#endif

	while (sz_left > 0) {
		/*  The serial data received from MT7622 BT controller is
		 *  at all time padded around with the STP header and tailer.
		 *
		 *  A full STP packet is looking like
		 *   -----------------------------------
		 *  | STP header  |  H:4   | STP tailer |
		 *   -----------------------------------
		 *  but it doesn't guarantee to contain a full H:4 packet which
		 *  means that it's possible for multiple STP packets forms a
		 *  full H:4 packet that means extra STP header + length doesn't
		 *  indicate a full H:4 frame, things can fragment. Whose length
		 *  recorded in STP header just shows up the most length the
		 *  H:4 engine can handle currently.
		 */
#if ENABLESTP
		p_h4 = btmtk_split_stp(bdev, p_left, sz_left, &sz_h4);
		if (!p_h4)
			break;

		adv = p_h4 - p_left;
		sz_left -= adv;
		p_left += adv;
#endif

#if ENABLESTP
		bdev->rx_skb = btmtk_parse_buf(hdev, bdev->rx_skb, p_h4,
					   sz_h4, mtk_recv_pkts,
					   ARRAY_SIZE(mtk_recv_pkts));
#else
		bdev->rx_skb = btmtk_parse_buf(hdev, bdev->rx_skb, data,
					   count, mtk_recv_pkts,
					   ARRAY_SIZE(mtk_recv_pkts));
#endif

		if (IS_ERR(bdev->rx_skb)) {
			err = PTR_ERR(bdev->rx_skb);
			BTMTK_ERR("Frame reassembly failed (%d)", err);
			bdev->rx_skb = NULL;
			return err;
		}

#if ENABLESTP
		sz_left -= sz_h4;
		p_left += sz_h4;
#else
		sz_left -= count;
		p_left += count;
#endif
	}

	return 0;
}

#if (USE_DEVICE_NODE == 0)
static int btmtk_set_audio_slave(struct btmtk_dev *bdev)
{
	int ret = 0;
	struct data_struct cmd = {0}, event = {0};
	struct fw_cfg_struct *audio_setting = &bdev->bt_cfg.audio_cmd;

	BTMTK_INFO("%s enter", __func__);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, AUDIO_SLAVE_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, AUDIO_SLAVE_EVT, event);

	if (audio_setting->content && audio_setting->length) {
		BTMTK_INFO("%s load audio setting from bt.cfg", __func__);
		memcpy((cmd.content + 4), audio_setting->content, audio_setting->length);
	} else {
		BTMTK_INFO("%s load default audio cmd", __func__);
	}
	BTMTK_INFO_RAW(cmd.content, cmd.len, "%s: Send CMD:", __func__);
	ret = btmtk_main_send_cmd(bdev, cmd.content, cmd.len,
			event.content, event.len, 0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s: failed(%d)", __func__, ret);

	BTMTK_INFO("%s exit", __func__);
	return ret;
}

int btmtk_set_audio_setting(struct btmtk_dev *bdev)
{
	int ret = 0;

	if (bdev->bt_cfg.support_audio_setting == true) {
		ret = btmtk_set_audio_slave(bdev);
		if (ret) {
			BTMTK_ERR("%s, btmtk_sdio_set_audio_slave error(%d)", __func__, ret);
			return ret;
		}

		if (main_info.hif_hook_chip.bt_set_pinmux) {
			ret = main_info.hif_hook_chip.bt_set_pinmux(bdev);
			if (ret < 0) {
				BTMTK_ERR("%s, btmtk_sdio_set_audio_pin_mux error(%d)", __func__, ret);
				return ret;
			}
		}
	}

	return ret;
}
#endif // (USE_DEVICE_NODE == 0)

int btmtk_recv_acl(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = NULL;

	if (hdev == NULL || skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL || bdev->workqueue == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		return -EINVAL;
	}

	skb_queue_tail(&bdev->rx_q, skb);
	queue_work(bdev->workqueue, &bdev->rx_work);

	/* remove it, if workqueue can't be scheduled, you can reuse it */
#if 0
	skip_pkt = btmtk_dispatch_fwlog(bdev, skb);
	if (skip_pkt == 0)
		err = hci_recv_frame(hdev, skb);
#endif
	return 0;
}


int btmtk_recv_event(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = NULL;

	if (hdev == NULL || skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL || bdev->workqueue == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		kfree_skb(skb);
		skb = NULL;
		return -EINVAL;
	}

	/* Fix up the vendor event id with 0xff for vendor specific instead
	 * of 0xe4 so that event send via monitoring socket can be parsed
	 * properly.
	 */
	/* if (hdr->evt == 0xe4) {
	 * BTMTK_DBG("%s hdr->evt is %02x", __func__, hdr->evt);
	 * hdr->evt = HCI_EV_VENDOR;
	 * }
	 */

	//BTMTK_DBG_RAW(skb->data, skb->len, "%s, recv evt(hci_recv_frame)", __func__);

	skb_queue_tail(&bdev->rx_q, skb);
	queue_work(bdev->workqueue, &bdev->rx_work);

	/* remove it, if workqueue can't be scheduled, you can reuse it */
#if 0
	skip_pkt = btmtk_dispatch_event(hdev, skb);
	if (skip_pkt == 0)
		err = hci_recv_frame(hdev, skb);

	if (err < 0) {
		BTMTK_ERR("%s hci_recv_failed, err = %d", __func__, err);
		goto err_out;
	}
#endif
	return 0;
}

int btmtk_recv_iso(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = NULL;

	if (hdev == NULL || skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL || bdev->workqueue == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		kfree_skb(skb);
		skb = NULL;
		return -EINVAL;
	}

	skb_queue_tail(&bdev->rx_q, skb);
	queue_work(bdev->workqueue, &bdev->rx_work);

	return 0;
}

#if (USE_DEVICE_NODE == 1)
int btmtk_recv_sco(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = NULL;

	if (hdev == NULL || skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL || bdev->workqueue == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		kfree_skb(skb);
		skb = NULL;
		return -EINVAL;
	}

	BTMTK_DBG_RAW(skb->data, skb->len, "%s:", __func__);

	skb_queue_tail(&bdev->rx_q, skb);
	queue_work(bdev->workqueue, &bdev->rx_work);

	return 0;
}

int btmtk_recv_rhw(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btmtk_dev *bdev = NULL;

	if (hdev == NULL || skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL || bdev->workqueue == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		kfree_skb(skb);
		skb = NULL;
		return -EINVAL;
	}

	skb_queue_tail(&bdev->rx_q, skb);
	queue_work(bdev->workqueue, &bdev->rx_work);

	return 0;
}
#endif

int btmtk_main_send_cmd(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, const uint8_t *event, const int event_len, int delay,
		int retry, int pkt_type, bool flag)
{
	struct sk_buff *skb = NULL;
	int ret = 0;
	int state = 0;
	u8 drv_own_retry = 10;

	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is NULL", __func__);
		ret = -EINVAL;
		goto exit;
	}

	if (bdev == NULL || bdev->hdev == NULL ||
		cmd == NULL || cmd_len <= 0) {
		BTMTK_ERR("%s, invalid parameters! cmd_len[%d]", __func__, cmd_len);
		ret = -EINVAL;
		goto exit;
	}

	if (main_info.hif_hook_chip.bt_check_power_status) {
		ret = main_info.hif_hook_chip.bt_check_power_status(bdev, cmd, pkt_type);
		if (ret)
			goto exit;
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_FW_DUMP) {
		BTMTK_WARN("%s: FW dumping ongoing, don't send any cmd to FW!!!", __func__);
		ret = -1;
		goto exit;
	}

	skb = alloc_skb(cmd_len + BT_SKB_RESERVE, GFP_KERNEL);
	if (skb == NULL) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		goto exit;
	}
	/* Reserv for core and drivers use */
	skb_reserve(skb, 7);
	bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
	memcpy(skb->data, cmd, cmd_len);
	skb->len = cmd_len;

#if ENABLESTP
	skb = btmtk_add_stp(bdev, skb);
#endif
	/* wmt cmd and download fw patch using wmt cmd with USB interface, need use
	 * usb_control_msg to recv wmt event;
	 * other HIF don't use this method to recv wmt event
	 */

	do {
	ret = main_info.hif_hook.send_and_recv(bdev,
			skb,
			event, event_len,
			delay, retry, pkt_type, flag);
	} while (ret == -EAGAIN && drv_own_retry--);

	if (ret < 0) {
		BTMTK_ERR("%s send_and_recv failed!!", __func__);
		/* ERRNUM is used to handle when skb has been sent successful,
		 * but wait related event failed, in this case, we don't need to free skb here,
		 * otherwise, it will be double free.
		 */
		if (ret != -ERRNUM && skb) {
			kfree_skb(skb);
		}
	}

exit:
	BTMTK_DBG("%s end!!", __func__);
	return ret;
}

int btmtk_load_code_from_bin(u8 **image, char *bin_name, struct device *dev,
		u32 *code_len, u8 retry)
{
	const struct firmware *fw_entry = NULL;
	int err = 0;
	unsigned long start_time = 0, time_diff = 0;

	if (!bin_name) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -1;
	}
	BTMTK_DBG("%s: load %s", __func__, bin_name);

	start_time = jiffies;
	do {
		err = request_firmware(&fw_entry, bin_name, dev);
		if (err >= 0) {
			break;
		} else if (retry <= 0) {
			*image = NULL;
			BTMTK_INFO("%s: request_firmware %d times fail, maybe file not exist, err = %d",
				__func__, 10, err);
			return -1;
		}
		BTMTK_INFO("%s: request_firmware fail, maybe file not exist, err = %d, retry = %d",
			__func__, err, retry);
		msleep(100);
	} while (retry-- > 0);

#if (USE_DEVICE_NODE == 0)
	if (fw_entry->size > FW_PATCH_SIZE_MAX) {
		BTMTK_ERR("%s: fw_entry->size = %ld is too large!!!", __func__, fw_entry->size);
		*code_len = 0;
		release_firmware(fw_entry);
		return -1;
	}
#endif

	time_diff = jiffies_to_msecs(jiffies) - jiffies_to_msecs(start_time);
	if (time_diff > TIME_BOUND_OF_REQ_FW)
		BTMTK_WARN("%s: request_firmware takes %lu ms", __func__, time_diff);

	*image = vmalloc(ALIGN_4(fw_entry->size));
	if (*image == NULL) {
		*code_len = 0;
		BTMTK_ERR("%s: vmalloc failed!! error code = %d", __func__, err);
		release_firmware(fw_entry);
		return -1;
	}

	memcpy(*image, fw_entry->data, fw_entry->size);
	*code_len = fw_entry->size;

	release_firmware(fw_entry);
	return 0;
}

int btmtk_load_code_from_bin_check(u8 *fwbuf, u32 len, int patch_flag)
{
	u32 section_num = 0;
	int loop_count = 0;
	u32 secsize = 0;
	u32 datasize = 0;
	struct _Section_Map *sectionMap;
	struct _Global_Descr *globaDescr;

	globaDescr = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);

	if (patch_flag == WIFI_DOWNLOAD)
		section_num = be2cpu32(globaDescr->u4SectionNum);
	else
		section_num = globaDescr->u4SectionNum;

	if (section_num > SECTION_NUM_MAX) {
		BTMTK_ERR("%s: section_num 0x%08x is an error value", __func__, section_num);
		return -1;
	}

	do {
		sectionMap = (struct _Section_Map *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE + FW_ROM_PATCH_GD_SIZE +
						FW_ROM_PATCH_SEC_MAP_SIZE * loop_count);
		if (patch_flag == WIFI_DOWNLOAD) {
			secsize = be2cpu32(sectionMap->u4SecSize);
		} else {
			secsize = sectionMap->u4SecSize;
		}
		datasize += secsize;
	} while (++loop_count < section_num);

	datasize += FW_ROM_PATCH_HEADER_SIZE + FW_ROM_PATCH_GD_SIZE + FW_ROM_PATCH_SEC_MAP_SIZE * section_num;

	if (datasize != len) {
		BTMTK_ERR("%s: rom patch length check fail, data len = 0x%08x, load rom patch len = 0x%08x", __func__, datasize, len);
		return -1;
	}

	return 0;
}

static u8 *btmtk_memstr(u8 *buf, u32 buf_len, char *substr)
{
	u32 sublen = 0;
	int i = 0;
	u8 *cur = NULL;

	if (buf == NULL || buf_len == 0 || substr == NULL) {
		BTMTK_WARN("%s, parameter is invalid!", __func__);
		return NULL;
	}

	sublen = strlen(substr);
	cur = buf + buf_len - 1;
	for (i = buf_len - 1; i >= 0; i--) {
		if (buf_len - 1 - i < sublen) {
			cur--;
			continue;
		}
		if (*cur == *substr && memcmp(cur, substr, sublen) == 0)
			return cur;

		cur--;
	}

	return NULL;
}

static void btmtk_print_bt_patch_info(struct btmtk_dev *bdev, u8 *fwbuf, u32 fwbuf_len)
{
	struct _PATCH_HEADER *patchHdr = NULL;
	struct _Global_Descr *globalDesrc = NULL;
	u8 *fw_version = NULL;
	u32 fw_version_len = 0;
	u8 build_time[16] = {0};

	if (fwbuf == NULL) {
		BTMTK_WARN("%s, fwbuf is NULL!", __func__);
		return;
	}

	patchHdr = (struct _PATCH_HEADER *)fwbuf;
	fw_version = btmtk_memstr(fwbuf, fwbuf_len, FW_VERSION_KEY_WORDS);

	globalDesrc = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);

	BTMTK_DBG("=============== Patch Info ==============");

	/* if not found version still need to do memset */
	memset(main_info.fw_version_str, 0, FW_VERSION_BUF_SIZE);
	if (fw_version) {
		fw_version_len = MIN(((u32)(fwbuf + fwbuf_len - fw_version)), FW_VERSION_BUF_SIZE);
		memcpy(main_info.fw_version_str, fw_version, fw_version_len);
		BTMTK_INFO("fw_version = %s", main_info.fw_version_str);
	}

	if (patchHdr) {
		build_time[15] = '\0';
		memcpy(build_time, patchHdr->ucDateTime, 15);
		BTMTK_INFO("Built Time = %s, Platform = %c%c%c%c, Hw Ver = 0x%04x, Sw Ver = 0x%04x, Magic Number = 0x%08x",
			    build_time,
			    patchHdr->ucPlatform[0],
			    patchHdr->ucPlatform[1],
			    patchHdr->ucPlatform[2],
			    patchHdr->ucPlatform[3],
			    patchHdr->u2HwVer,
			    patchHdr->u2SwVer,
			    patchHdr->u4MagicNum);
	} else
		BTMTK_WARN("%s, patchHdr is NULL!", __func__);

	if (globalDesrc)
		BTMTK_INFO("Patch Ver = 0x%08x, Section num = 0x%08x",
			   globalDesrc->u4PatchVer,
			   globalDesrc->u4SectionNum);
	else
		BTMTK_WARN("%s, globalDesrc is NULL!", __func__);
	BTMTK_DBG("=========================================");
}

static void btmtk_print_wifi_patch_info(struct btmtk_dev *bdev, u8 *fwbuf)
{
	struct _PATCH_HEADER *patchHdr = NULL;
	struct _Global_Descr *globalDesrc = NULL;
	u8 build_time[16] = {0};

	if (fwbuf == NULL) {
		BTMTK_WARN("%s, fwbuf is NULL!", __func__);
		return;
	}

	patchHdr = (struct _PATCH_HEADER *)fwbuf;

	globalDesrc = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);

	BTMTK_INFO("=============== Wifi Patch Info ==============");
	if (patchHdr) {
		build_time[15] = '\0';
		memcpy(build_time, patchHdr->ucDateTime, 15);
		BTMTK_INFO("Built Time = %s", build_time);
		BTMTK_INFO("Hw Ver = 0x%04x",
			((patchHdr->u2HwVer & 0x00ff) << 8) | ((patchHdr->u2HwVer & 0xff00) >> 8));
		BTMTK_INFO("Sw Ver = 0x%04x",
			((patchHdr->u2SwVer & 0x00ff) << 8) | ((patchHdr->u2SwVer & 0xff00) >> 8));
		BTMTK_INFO("Magic Number = 0x%08x", be2cpu32(patchHdr->u4MagicNum));

		BTMTK_INFO("Platform = %c%c%c%c",
				patchHdr->ucPlatform[0],
				patchHdr->ucPlatform[1],
				patchHdr->ucPlatform[2],
				patchHdr->ucPlatform[3]);
	} else
		BTMTK_WARN("%s, patchHdr is NULL!", __func__);

	if (globalDesrc) {
		BTMTK_INFO("Patch Ver = 0x%08x",
			be2cpu32(globalDesrc->u4PatchVer));
		BTMTK_INFO("Section num = 0x%08x",
			be2cpu32(globalDesrc->u4SectionNum));
	} else
		BTMTK_WARN("%s, globalDesrc is NULL!", __func__);
	BTMTK_INFO("=========================================");
}

static int btmtk_send_wmt_download_cmd(struct btmtk_dev *bdev, u8 *cmd,
		int cmd_len, u8 *event, int event_len, struct _Section_Map *sectionMap,
		u8 fw_state, u8 dma_flag, int patch_flag)
{
	int payload_len = 0;
	int ret = -1;
	int i = 0;
	u32 revert_SecSpec = 0;

	if (bdev == NULL || cmd == NULL || event == NULL || sectionMap == NULL) {
		BTMTK_ERR("%s: invalid parameter!", __func__);
		return ret;
	}

	/* need refine this cmd to mtk_wmt_hdr struct*/
	/* prepare HCI header */
	cmd[0] = 0x01;
	cmd[1] = 0x6F;
	cmd[2] = 0xFC;

	/* prepare WMT header */
	cmd[4] = 0x01;
	if ((is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id)) && patch_flag == WIFI_DOWNLOAD) {
		cmd[5] = 0x54; /* opcode */
		event[4] = 0x54; /* should be same as cmd*/
	} else {
		cmd[5] = 0x01; /* opcode */
		event[4] = 0x01; /* should be same as cmd*/
	}
	if (fw_state == 0) {
		/* prepare WMT DL cmd */
		payload_len = SEC_MAP_NEED_SEND_SIZE + 2;

		cmd[3] = (payload_len + 4) & 0xFF; /* length*/
		cmd[6] = payload_len & 0xFF;
		cmd[7] = (payload_len >> 8) & 0xFF;
		cmd[8] = 0x00; /* which is the FW download state 0 */
		cmd[9] = dma_flag; /* 1:using DMA to download, 0:using legacy wmt cmd*/
		cmd_len = SEC_MAP_NEED_SEND_SIZE + PATCH_HEADER_SIZE;

		if (patch_flag == WIFI_DOWNLOAD) {
			for (i = 0; i < SECTION_SPEC_NUM; i++) {
				revert_SecSpec = be2cpu32(sectionMap->u4SecSpec[i]);
				memcpy(&cmd[PATCH_HEADER_SIZE] + i * sizeof(u32), (u8 *)&revert_SecSpec, sizeof(u32));
			}
		} else
			memcpy(&cmd[PATCH_HEADER_SIZE], (u8 *)(sectionMap->u4SecSpec), SEC_MAP_NEED_SEND_SIZE);

		BTMTK_DBG_RAW(cmd, cmd_len, "%s: CMD: len[%d]", __func__, cmd_len);

		ret = btmtk_main_send_cmd(bdev, cmd, cmd_len,
				event, event_len, DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
		if (ret < 0) {
			BTMTK_ERR("%s: send wmd dl cmd failed, terminate!", __func__);
			return PATCH_ERR;
		}

		if (bdev->recv_evt_len >= event_len)
			return bdev->io_buf[PATCH_STATUS];

		return PATCH_ERR;
	}

	BTMTK_ERR("%s: fw state is error!", __func__);
	return ret;
}

static int btmtk_load_fw_patch_using_wmt_cmd(struct btmtk_dev *bdev,
		u8 *image, u8 *fwbuf, u8 *event, int event_len, u32 patch_len, int offset, int patch_flag)
{
	int ret = 0;
	u32 cur_len = 0;
	s32 sent_len;
	int first_block = 1;
	u8 phase;
	int delay = PATCH_DOWNLOAD_PHASE1_2_DELAY_TIME;
	int retry = PATCH_DOWNLOAD_PHASE1_2_RETRY;

	if (bdev == NULL || image == NULL || fwbuf == NULL) {
		BTMTK_WARN("%s, invalid parameters!", __func__);
		ret = -1;
		goto exit;
	}

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;

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
			image[0] = 0x02;
			image[1] = 0x6F;
			image[2] = 0xFC;
			image[3] = (sent_len + 5) & 0xFF;
			image[4] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			image[5] = 0x01;
			image[6] = 0x01;
			image[7] = (sent_len + 1) & 0xFF;
			image[8] = ((sent_len + 1) >> 8) & 0xFF;

			image[9] = phase;
			memcpy(&image[10], fwbuf + offset + cur_len, sent_len);
			if (phase == PATCH_PHASE3) {
				/* if secure boot enable, it need take 76ms at less
				 * for RSA check.
				 */
				delay = main_info.hif_hook_chip.dl_delay_time;
				retry = PATCH_DOWNLOAD_PHASE3_RETRY;
			}

			if ((is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id)) && patch_flag == WIFI_DOWNLOAD) {
				image[6] = 0x54; /* opcode */
				event[4] = 0x54; /* should be same as cmd*/
			} else {
				image[6] = 0x01; /* opcode */
				event[4] = 0x01; /* should be same as cmd*/
			}

			cur_len += sent_len;
			BTMTK_DBG("%s: sent_len = %d, cur_len = %d, phase = %d", __func__,
					sent_len, cur_len, phase);

			ret = btmtk_main_send_cmd(bdev, image, sent_len + PATCH_HEADER_SIZE,
					event, event_len, delay, retry, BTMTK_TX_ACL_FROM_DRV, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_INFO("%s: send patch failed, terminate", __func__);
				goto exit;
			}
		} else
			break;
	}

exit:
	return ret;
}

int btmtk_send_hw_err_to_host(struct btmtk_dev *bdev)
{
	struct sk_buff *skb = NULL;
	struct data_struct hwerr_event = {0};

	BTMTK_ERR("%s reset_stack_flag = %d!!", __func__, main_info.reset_stack_flag);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, HWERR_EVT, hwerr_event);

	if (main_info.reset_stack_flag) {
#if (USE_DEVICE_NODE == 0)
		skb = alloc_skb(hwerr_event.len + BT_SKB_RESERVE, GFP_KERNEL);
		if (skb == NULL) {
			BTMTK_ERR("%s allocate skb failed!!", __func__);
		} else {
			hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
			skb->data[0] = hwerr_event.content[1];
			skb->data[1] = hwerr_event.content[2];
			skb->data[2] = main_info.reset_stack_flag;
			skb->len = hwerr_event.len - 1;
			BTMTK_DBG_RAW(skb->data, skb->len, "%s: hw err event:", __func__);
			hci_recv_frame(bdev->hdev, skb);
		}
#else
		/* copy assert reason to hw error event payload */
		/* ASSERT_REASON_SIZE + 3 is payload 255 add event header 3 bytes */
		skb = alloc_skb(ASSERT_REASON_SIZE + 3 + BT_SKB_RESERVE, GFP_KERNEL);
		if (skb == NULL) {
			BTMTK_ERR("%s allocate skb failed!!", __func__);
		} else {
			/* send to RX buffer instead of hci driver */
			skb_reserve(skb, BT_SKB_RESERVE);
			hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
			skb->data[0] = hwerr_event.content[1];
			skb->data[1] = ASSERT_REASON_SIZE;
			skb->data[2] = HW_ERR_NONE;
			strncpy(&skb->data[3], bdev->assert_reason, ASSERT_REASON_SIZE - 1);
			/* +2 is for event header without pkt type
			 * and set the last byte to 0 for string end */
			skb->data[ASSERT_REASON_SIZE + 2 - 1] = '\0';
			skb->len = ASSERT_REASON_SIZE + 2;
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: hw err event: ", __func__);
			skb_queue_tail(&bdev->rx_q, skb);
			queue_work(bdev->workqueue, &bdev->rx_work);
		}
#endif
	}
	return 0;
}

static int btmtk_parsing_fw_rom_patch(struct btmtk_dev *bdev,
		u8 *fwbuf, int patch_flag)
{
	int loop_count = 0;
	int ret = 0;
	u32 section_num = 0;
	u32 section_offset = 0;
	u32 dl_size = 0;
	u32 bin_type = 0;
	u8 bin_index = 0;
	u8 dma_flag = PATCH_DOWNLOAD_USING_WMT;
	struct _Section_Map *sectionMap;
	struct _Global_Descr *globalDescr;

	if (fwbuf == NULL) {
		BTMTK_WARN("%s, fwbuf is NULL!", __func__);
		ret = -1;
		goto exit;
	}

	globalDescr = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);

	if (patch_flag == WIFI_DOWNLOAD)
		section_num = be2cpu32(globalDescr->u4SectionNum);
	else
		section_num = globalDescr->u4SectionNum;
	BTMTK_INFO("%s: section_num = 0x%08x", __func__, section_num);

	if (section_num > SECTION_NUM_MAX) {
		BTMTK_ERR("%s: section_num 0x%08x is an error value", __func__, section_num);
		ret = -1;
		goto exit;
	}

	if (bdev->sectionMap_table) {
		kfree(bdev->sectionMap_table);
		bdev->sectionMap_table = NULL;
	}

	bdev->sectionMap_table = kmalloc_array(section_num,
			sizeof(struct _Section_Map), GFP_KERNEL);
	if (!bdev->sectionMap_table) {
		BTMTK_ERR("%s: alloc memory fail (bdev->sectionMap_table)", __func__);
		ret = -1;
		goto exit;
	}

	do {
		sectionMap = (struct _Section_Map *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE +
				FW_ROM_PATCH_GD_SIZE + FW_ROM_PATCH_SEC_MAP_SIZE * loop_count);

		if (patch_flag == WIFI_DOWNLOAD) {
			/* wifi is big-endian */
			section_offset = be2cpu32(sectionMap->u4SecOffset);
			dl_size = be2cpu32(sectionMap->bin_info_spec.u4DLSize);
			if (main_info.hif_hook.dl_dma)
				dma_flag = be2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;
			bin_index = (be2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) >> 16) & 0xFF;
			bin_type = be2cpu32(sectionMap->bin_info_spec.u4SecType);
		} else {
			/* BT is little-endian */
			section_offset = sectionMap->u4SecOffset;
			dl_size = sectionMap->bin_info_spec.u4DLSize;
			/*
			 * loop_count = 0: BGF patch
			 *              1: BT ILM
			 * only BT ILM support DL DMA for Buzzard
			 */
			if (main_info.hif_hook.dl_dma)
				dma_flag = le2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;
			bin_index = (le2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) >> 16) & 0xFF;
			bin_type = sectionMap->bin_info_spec.u4SecType;
		}

		BTMTK_INFO("%s: loop_count = %d, section_offset = 0x%08x, download patch_len = 0x%08x, "
				"dl mode = %d, section bin_type = 0x%08x, bin_index =%d",
				__func__, loop_count, section_offset, dl_size, dma_flag, bin_type, bin_index);
		memcpy(&bdev->sectionMap_table[loop_count], sectionMap, sizeof(struct _Section_Map));
	} while (++loop_count < section_num);

exit:
	return ret;

}

int btmtk_load_fw_by_bin_info(struct btmtk_dev *bdev,
		u8 *fwbuf, u32 binInfo, int mode)
{
	u8 *pos;
	int loop_count = 0;
	int ret = 0;
	u32 section_num = 0;
	u32 section_offset = 0;
	u32 dl_size = 0;
	u32 temp_type = 0;
	u8 temp_index = 0;
	int patch_status = 0;
	int retry = 20;
	u8 dma_flag = PATCH_DOWNLOAD_USING_WMT;
	struct _Section_Map *sectionMap;
	struct _Global_Descr *globalDescr;
	u8 event[LD_PATCH_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; /* event[7] is status*/

	/*
	 * flag 0 for bin_index u8
	 * flag 1 for bin_type u32
	 */
	BTMTK_DBG("%s enter : mode %d expected bin_info = 0x%08x", __func__, mode, binInfo);

	if (fwbuf == NULL) {
		BTMTK_WARN("%s, fwbuf is NULL!", __func__);
		ret = -1;
		goto exit;
	}

	globalDescr = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);
	section_num = globalDescr->u4SectionNum;

	if (section_num > SECTION_NUM_MAX) {
		BTMTK_ERR("%s: section_num 0x%08x is an error value", __func__, section_num);
		ret = -1;
		goto exit;
	}

	do {
		sectionMap = &bdev->sectionMap_table[loop_count];
		temp_index = (le2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) >> 16) & 0xFF;
		temp_type = le2cpu32(sectionMap->bin_info_spec.u4SecType);
		if (mode == DOWNLOAD_BY_TYPE) {
			if (temp_type == binInfo) {
				BTMTK_DBG("%s, download by bin type", __func__);
				break;
			}
		} else {
			if (temp_index == (binInfo & 0xFF)) {
				BTMTK_DBG("%s, download by bin index", __func__);
				break;
			}
		}
	} while (++loop_count < section_num);

	if (loop_count == section_num) {
		BTMTK_ERR("%s: bin_info 0x%08x is not found", __func__, binInfo);
		ret = 0;
		goto exit;
	}

	pos = kmalloc(UPLOAD_PATCH_UNIT, GFP_KERNEL);
	if (!pos) {
		BTMTK_ERR("%s: alloc memory failed", __func__);
		ret = -1;
		goto exit;
	}

	/* BT is little-endian */
	section_offset = sectionMap->u4SecOffset;
	dl_size = sectionMap->bin_info_spec.u4DLSize;
	/*
	 * loop_count = 0: BGF patch
	 *              1: BT ILM
	 * only BT ILM support DL DMA for Buzzard
	 */
	if (main_info.hif_hook.dl_dma)
		dma_flag = le2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;

/* dynamic dl with uart hif is too slow, stack will timeout when startup */
#if !defined(CHIP_IF_UART_TTY) || (USE_DEVICE_NODE == 1)
	if (mode == DOWNLOAD_BY_INDEX) {
		/* dynamic download need downloaded by wmt cmd */
		dma_flag = PATCH_DOWNLOAD_USING_WMT;
	}
#endif
	BTMTK_INFO("%s:bin_type[0x%08x] mode[%d] bin_info[0x%08x] section_index[%d] bin_index[%d] dma_flag[%d]",
			__func__, temp_type, mode, binInfo, loop_count, temp_index, dma_flag);

	if (dl_size > 0) {
		retry = 20;
		do {
			patch_status = btmtk_send_wmt_download_cmd(bdev, pos, 0,
					event, LD_PATCH_EVT_LEN - 1, sectionMap, 0, dma_flag, BT_DOWNLOAD);
			BTMTK_DBG("%s: patch_status %d", __func__, patch_status);

			if (patch_status > PATCH_READY || patch_status == PATCH_ERR) {
				BTMTK_ERR("%s: error patch_status[%d]", __func__, patch_status);
				ret = -1;
				goto err;
			} else if (patch_status == PATCH_READY) {
				BTMTK_INFO("%s: no need to load rom patch section%d", __func__, loop_count);
				goto err;
			} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
				BTMTK_INFO("%s: PATCH_IS_DOWNLOAD_BY_OTHER", __func__);
				msleep(100);
				retry--;
			} else if (patch_status == PATCH_NEED_DOWNLOAD) {
				break;  /* Download ROM patch directly */
			}
		} while (retry > 0);

		if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
			BTMTK_WARN("%s: Hold by another fun more than 2 seconds", __func__);
			ret = -1;
			goto err;
		}

		if (dma_flag == PATCH_DOWNLOAD_USING_DMA && main_info.hif_hook.dl_dma) {
			BTMTK_DBG("%s: btmtk_load_fw_patch_using_dma!", __func__);
			/* using DMA to download fw patch*/
			ret = main_info.hif_hook.dl_dma(bdev,
				pos, fwbuf,
				dl_size, section_offset);
			if (ret < 0) {
				BTMTK_ERR("%s: btmtk_load_fw_patch_using_dma failed!", __func__);
				goto err;
			}
		} else {
			BTMTK_INFO("%s: btmtk_load_fw_patch_using_wmt_cmd!", __func__);
			/* using legacy wmt cmd to download fw patch */
			ret = btmtk_load_fw_patch_using_wmt_cmd(bdev, pos, fwbuf, event,
					LD_PATCH_EVT_LEN, dl_size, section_offset, BT_DOWNLOAD);
			if (ret < 0) {
				BTMTK_ERR("%s: btmtk_load_fw_patch_using_wmt_cmd failed!", __func__);
				goto err;
			}
		}
		BTMTK_INFO("%s end", __func__);
	}

err:
	kfree(pos);
	pos = NULL;

exit:
	return ret;
}

static int btmtk_send_fw_rom_patch_79xx(struct btmtk_dev *bdev,
		u8 *fwbuf, int patch_flag)
{
	u8 *pos;
	int loop_count = 0;
	int ret = 0;
	u32 section_num = 0;
	u32 section_offset = 0;
	u32 dl_size = 0;
	int patch_status = 0;
	int retry = 20;
	u8 dma_flag = PATCH_DOWNLOAD_USING_WMT;
	struct _Section_Map *sectionMap;
	struct _Global_Descr *globalDescr;
	struct data_struct event = {0};
#if DEBUG_LD_PATCH_TIME
	struct timeval tv_start, tv_bgf, tv_ilm;
	u32 dlt_dma = 0, dlt_all = 0;

	memset(&tv_start, 0, sizeof(tv_start));
	memset(&tv_bgf, 0, sizeof(tv_bgf));
	memset(&tv_ilm, 0, sizeof(tv_ilm));
//	btmtk_do_gettimeofday(&tv_start);
#endif
	if (fwbuf == NULL) {
		BTMTK_WARN("%s, fwbuf is NULL!", __func__);
		ret = -1;
		goto exit;
	}

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, LD_PATCH_EVT, event);

	globalDescr = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);

	if (patch_flag == WIFI_DOWNLOAD)
		section_num = be2cpu32(globalDescr->u4SectionNum);
	else
		section_num = globalDescr->u4SectionNum;
	BTMTK_INFO("%s: section_num = 0x%08x", __func__, section_num);

	if (section_num > SECTION_NUM_MAX) {
		BTMTK_ERR("%s: section_num 0x%08x is an error value", __func__, section_num);
		ret = -1;
		goto exit;
	}

	pos = kmalloc(UPLOAD_PATCH_UNIT, GFP_ATOMIC);
	if (!pos) {
		BTMTK_ERR("%s: alloc memory failed", __func__);
		ret = -1;
		goto exit;
	}

	do {
		sectionMap = (struct _Section_Map *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE +
				FW_ROM_PATCH_GD_SIZE + FW_ROM_PATCH_SEC_MAP_SIZE * loop_count);

		if (patch_flag == WIFI_DOWNLOAD) {
			/* wifi is big-endian */
			section_offset = be2cpu32(sectionMap->u4SecOffset);
			dl_size = be2cpu32(sectionMap->bin_info_spec.u4DLSize);
			if (main_info.hif_hook.dl_dma)
				dma_flag = be2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;
		} else {
			/* BT & ZB are little-endian */
			section_offset = sectionMap->u4SecOffset;
			dl_size = sectionMap->bin_info_spec.u4DLSize;
			/*
			 * loop_count = 0: BGF patch
			 *              1: BT ILM
			 * only BT ILM support DL DMA for Buzzard
			 */
			if (main_info.hif_hook.dl_dma)
				dma_flag = le2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;
		}
		BTMTK_INFO("%s: loop_count = %d, section_offset = 0x%08x, download patch_len = 0x%08x, dl mode = %d",
						__func__, loop_count, section_offset, dl_size, dma_flag);
		if (dl_size > 0) {
			retry = 20;
			do {
				patch_status = btmtk_send_wmt_download_cmd(bdev, pos, 0,
						event.content, event.len - 1, sectionMap, 0, dma_flag, patch_flag);
				BTMTK_INFO("%s: patch_status %d", __func__, patch_status);

				if (patch_status > PATCH_READY || patch_status == PATCH_ERR) {
					BTMTK_ERR("%s: patch_status error", __func__);
					ret = -1;
					goto err;
				} else if (patch_status == PATCH_READY) {
					BTMTK_INFO("%s: no need to load rom patch section%d", __func__, loop_count);
					goto next_section;
				} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
					msleep(100);
					retry--;
				} else if (patch_status == PATCH_NEED_DOWNLOAD) {
					break;  /* Download ROM patch directly */
				}
			} while (retry > 0);

			if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
				BTMTK_WARN("%s: Hold by another fun more than 2 seconds", __func__);
				ret = -1;
				goto err;
			}

			if (dma_flag == PATCH_DOWNLOAD_USING_DMA && main_info.hif_hook.dl_dma) {
				/* using DMA to download fw patch*/
				ret = main_info.hif_hook.dl_dma(bdev,
					pos, fwbuf,
					dl_size, section_offset);
				if (ret < 0) {
					BTMTK_ERR("%s: btmtk_load_fw_patch_using_dma failed!", __func__);
					goto err;
				}
			} else {
				/* using legacy wmt cmd to download fw patch */
				ret = btmtk_load_fw_patch_using_wmt_cmd(bdev, pos, fwbuf, event.content,
						event.len - 1, dl_size, section_offset, patch_flag);
				if (ret < 0) {
					BTMTK_ERR("%s: btmtk_load_fw_patch_using_wmt_cmd failed!", __func__);
					goto err;
				}
			}
		}
		/* FW Download finished */
		/* remove it, comment from fw dl owner
		 * if (patch_flag == WIFI_DOWNLOAD) {
		 * if (loop_count == section_num - 1) {
		 * mdelay(500);
		 * }
		 * }
		 */
#if DEBUG_LD_PATCH_TIME
		if (loop_count == 0) {
//				btmtk_do_gettimeofday(&tv_bgf);
		} else if (loop_count == 1) {
//				btmtk_do_gettimeofday(&tv_ilm);
			if (tv_bgf.tv_sec != 0 || tv_bgf.tv_usec != 0) {
				if (tv_ilm.tv_sec >= tv_bgf.tv_sec)
					dlt_dma = (tv_ilm.tv_sec - tv_bgf.tv_sec) * 1000;
				else
					dlt_dma = (~(tv_bgf.tv_sec - tv_ilm.tv_sec) + 1) * 1000;

				dlt_dma += (tv_ilm.tv_usec - tv_bgf.tv_usec) / 1000;
			}
			if (tv_ilm.tv_sec >= tv_start.tv_sec)
				dlt_all = (tv_ilm.tv_sec - tv_start.tv_sec) * 1000;
			else
				dlt_all = (~(tv_start.tv_sec - tv_ilm.tv_sec) + 1) * 1000;

			dlt_all += (tv_ilm.tv_usec - tv_start.tv_usec) / 1000;

			BTMTK_INFO("LD PATCH 1 tv_start: tv_sec:%zu, tv_usec:%zu.",
					tv_start.tv_sec, tv_start.tv_usec);
			BTMTK_INFO("LD PATCH 2 tv_bgf: tv_sec:%zu, tv_usec:%zu.",
					tv_bgf.tv_sec, tv_bgf.tv_usec);
			BTMTK_INFO("LD PATCH 3 tv_ilm: tv_sec:%zu, tv_usec:%zu.",
					tv_ilm.tv_sec, tv_ilm.tv_usec);

			if (dlt_dma != 0)
				BTMTK_INFO("LD PATCH time: ILM_DMA:%ums, ALL:%ums.",
						dlt_dma, dlt_all);
			else
				BTMTK_INFO("LD PATCH time: ILM_DMA:%ums.",
						dlt_all);
		}
#endif
next_section:
		continue;
	} while (++loop_count < section_num);

err:
	kfree(pos);
	pos = NULL;

exit:
	return ret;
}

int btmtk_dynamic_load_rom_patch(struct btmtk_dev *bdev, u32 binInfo)
{
	int ret = 0;
	u8 *rom_patch = NULL;
	unsigned int rom_patch_len = 0;

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	/* should  reload bt fw bin name , may be recover by co-dl wifi*/
#if 0
	if (bdev->flavor) {
		(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN, "BT_RAM_CODE_MT%04x_1a_%x_hdr.bin",
				bdev->chip_id & 0xffff, (bdev->fw_version & 0xff) + 1);
	} else {
		(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN, "BT_RAM_CODE_MT%04x_1_%x_hdr.bin",
				bdev->chip_id & 0xffff, (bdev->fw_version & 0xff) + 1);
	}
#endif
	if (is_mt6639(bdev->chip_id) || is_mt66xx(bdev->chip_id))
		(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
#if (USE_DEVICE_NODE == 0)
							"BT_RAM_CODE_MT6639_2_1_hdr.bin");
#else
							"BT_RAM_CODE_MT%x_%s_1_hdr.bin", bdev->chip_id, bdev->flavor_bin);
							//"BT_RAM_CODE_MT6639_1_1_nonenc_hdr.bin");
#endif
	else if (is_mt7925(bdev->chip_id))
		(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
							"BT_RAM_CODE_MT7925_1_1_hdr.bin");


	BTMTK_INFO("%s: rom patch file name is %s", __func__,
		bdev->rom_patch_bin_file_name);

	btmtk_load_code_from_bin(&rom_patch, bdev->rom_patch_bin_file_name, NULL,
							&rom_patch_len, 10);

	if (!rom_patch) {
		BTMTK_ERR("%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				__func__, bdev->rom_patch_bin_file_name, bdev->rom_patch_bin_file_name);
		ret = -1;
		goto err;
	}

	/* dynamic download should download section by bin index */
	ret = btmtk_load_fw_by_bin_info(bdev, rom_patch, binInfo, DOWNLOAD_BY_INDEX);
	if (ret < 0) {
		BTMTK_ERR("%s, btmtk_load_rom_patch_connac3 failed!, bin type is 0x%08x",
			__func__, binInfo);
		ret = -1;
	}

err:
	if (rom_patch)
		vfree(rom_patch);
	return ret;
}

int btmtk_load_rom_patch_connac3(struct btmtk_dev *bdev, int patch_flag)
{
	int ret = 0;
	u8 *rom_patch = NULL;
	unsigned int rom_patch_len = 0;
	int i = 0;
	u32 bt_bin_type[BT_BIN_TYP_NUM] = {0x00000000, 0x00000002, 0x00000003, 0x00000080,
			0x00000090, 0x00010000};
	//u32 wf_bin_type[] = {0x00000100};

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	if (patch_flag == WIFI_DOWNLOAD) {
		/* For 7902, we can't read flavor from controller successfully */
		if (bdev->flavor)
			/* if flavor equals 1, it represent 7920, else it represent 7921 */
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
					"WIFI_MT%04x_patch_mcu_1a_%x_hdr.bin",
					bdev->chip_id & 0xffff, (bdev->fw_version & 0xff) + 1);
		else
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
					"WIFI_MT%04x_patch_mcu_1_%x_hdr.bin",
					bdev->chip_id & 0xffff, (bdev->fw_version & 0xff) + 1);

		if (is_mt6639(bdev->chip_id) || is_mt7925(bdev->chip_id))
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
					"WIFI_MT%04x_PATCH_MCU_%x_%x_hdr.bin",
					bdev->chip_id & 0xffff, (bdev->proj & 0xff) + 1, (bdev->fw_version & 0xff) + 1);

	} else if (patch_flag == ZB_DOWNLOAD) {
		if (bdev->flavor)
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
					"ZB_RAM_CODE_MT%04x_1a_%x_hdr.bin",
					bdev->chip_id & 0xffff, (bdev->fw_version & 0xff) + 1);
		else
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
					"ZB_RAM_CODE_MT%04x_1_%x_hdr.bin",
					bdev->chip_id & 0xffff, (bdev->fw_version & 0xff) + 1);
	} else if (patch_flag == BT_DOWNLOAD) {
		BTMTK_DBG("%s: BT_DOWNLOAD %u", __func__, bdev->chip_id);
		if (is_mt6639(bdev->chip_id) || is_mt66xx(bdev->chip_id))
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
#if (USE_DEVICE_NODE == 0)
					"BT_RAM_CODE_MT6639_2_1_hdr.bin");
#else
					"BT_RAM_CODE_MT%x_%s_1_hdr.bin", bdev->chip_id, bdev->flavor_bin);
					//"BT_RAM_CODE_MT6639_1_1_nonenc_hdr.bin");
#endif
		else if (is_mt7925(bdev->chip_id))
			(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN,
					"BT_RAM_CODE_MT7925_1_1_hdr.bin");
	} else
		BTMTK_ERR("%s: unknow patch_flag", __func__);

	BTMTK_INFO("%s: rom patch[%s], bt_cfg_file_name[%s] patch_flag[%d]", __func__,
			bdev->rom_patch_bin_file_name, bdev->bt_cfg_file_name, patch_flag);

	btmtk_load_code_from_bin(&rom_patch, bdev->rom_patch_bin_file_name, NULL,
							&rom_patch_len, 10);

	if (!rom_patch) {
		BTMTK_ERR("%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				__func__, bdev->rom_patch_bin_file_name, bdev->rom_patch_bin_file_name);
		ret = -1;
		goto err;
	}

	if (btmtk_load_code_from_bin_check(rom_patch, rom_patch_len, patch_flag)) {
		BTMTK_ERR("%s: rom patch length check fail, retyrn", __func__);
		ret = -1;
		goto err;
	}

	if (patch_flag == WIFI_DOWNLOAD) {
		/*Display rom patch info*/
		btmtk_print_wifi_patch_info(bdev, rom_patch);
		ret = btmtk_send_fw_rom_patch_79xx(bdev, rom_patch, patch_flag);
		if (ret < 0) {
			BTMTK_ERR("%s, btmtk_send_fw_rom_patch_79xx failed!", __func__);
			goto err;
		}
	} else if (patch_flag == ZB_DOWNLOAD) {
		/* ZB header is the same to ZB little endian */
		btmtk_print_bt_patch_info(bdev, rom_patch, rom_patch_len);
		ret = btmtk_send_fw_rom_patch_79xx(bdev, rom_patch, patch_flag);
		if (ret < 0) {
			BTMTK_ERR("%s, btmtk_send_fw_rom_patch_79xx failed!", __func__);
			goto err;
		}
	} else {
		btmtk_print_bt_patch_info(bdev, rom_patch, rom_patch_len);
		btmtk_parsing_fw_rom_patch(bdev, rom_patch, patch_flag);

		for (i = 0; i < BT_BIN_TYP_NUM; i++) {
			ret = btmtk_load_fw_by_bin_info(bdev, rom_patch, bt_bin_type[i],
					DOWNLOAD_BY_TYPE);
			if (ret < 0) {
				BTMTK_ERR("%s failed!, bin type is 0x%08x",
					__func__, bt_bin_type[i]);
				goto err;
			}
#if (USE_DEVICE_NODE == 1)
			/* Send efem command before bt cal */
			if (bt_bin_type[i] == 0x00000003) {
				ret = btmtk_send_connfem_cmd(bdev);
				if (ret < 0) {
					BTMTK_ERR("%s send connfem fail", __func__);
					goto err;
				}
			}
#endif

			if (main_info.bk_rs_flag && bt_bin_type[i] == 0x00000003 &&
					bdev->cali_backup.num &&
					main_info.hif_hook_chip.restore) {
				ret = main_info.hif_hook_chip.restore(bdev);
				if (ret < 0) {
					BTMTK_ERR("%s, calibration restore failed!", __func__);
					if (main_info.hif_hook_chip.backup_free)
						main_info.hif_hook_chip.backup_free(bdev);
					continue;
				}
			}
		}
	}

	if (main_info.bk_rs_flag && main_info.hif_hook_chip.backup_send && bdev->cali_backup.num == 0) {
		ret = main_info.hif_hook_chip.backup_send(bdev);
	}
	bdev->power_state = BTMTK_DONGLE_STATE_POWER_OFF;
	BTMTK_INFO("%s end", __func__);

err:
	if (rom_patch)
		vfree(rom_patch);
	return ret;
}

/* need to remove after modify to using function pointer*/
__weak int32_t bgfsys_bt_patch_dl(void)
{
	BTMTK_ERR("No bgfsys_bt_patch_dl function");
	return -1;
}

/* need to remove after modify to using function pointer*/
__weak int32_t btmtk_set_sleep(struct hci_dev *hdev, u_int8_t need_wait)
{
	//BTMTK_ERR("No btmtk_set_sleep function");
	return -1;
}

int btmtk_load_rom_patch(struct btmtk_dev *bdev)
{
	int err = -1;

	if (!bdev || !bdev->hdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return err;
	}

	if (main_info.hif_hook_chip.load_patch) {
		err = main_info.hif_hook_chip.load_patch(bdev);
		if (err < 0) {
			BTMTK_ERR("%s: btmtk_load_rom_patch bt patch failed!", __func__);
			return err;
		}
	} else
		BTMTK_WARN("%s: unknown chip id (%d)", __func__, bdev->chip_id);
	BTMTK_DBG("%s: end, err[%d]", __func__, err);

	return err;
}

struct btmtk_dev *btmtk_get_dev(void)
{
	int i = 0;
	struct btmtk_dev *tmp_bdev = NULL;

	for (i = 0; i < btmtk_intf_num; i++) {
		/* Find empty slot for newly probe interface.
		 * Judged from load_rom_patch is done and
		 * Identified chip_id from cap_init.
		 */
		if (g_bdev[i]->hdev == NULL) {
#if (USE_DEVICE_NODE == 1)
			/* only use g_bdev[0] in SP project */
			i = 0;
			g_bdev[i]->dongle_index = i;
#else
			if (i == 0)
				g_bdev[i]->dongle_index = i;
			else
				g_bdev[i]->dongle_index = g_bdev[i - 1]->dongle_index + 1;
#endif
			/* reset pin initial value need to be -1, used to judge after
			 * disconnected before probe, can't do chip reset
			 */
			g_bdev[i]->bt_cfg.dongle_reset_gpio_pin = -1;
			tmp_bdev = g_bdev[i];

			/* Hook pre-defined table on state machine */
			g_bdev[i]->cif_state = (struct btmtk_cif_state *)g_cif_state;
			break;
		}
	}
	BTMTK_INFO("%s use g_bdev[%d]", __func__, i);

	return tmp_bdev;
}

void btmtk_release_dev(struct btmtk_dev *bdev)
{
	int i = 0;
	struct btmtk_dev *tmp_bdev = NULL;

	BTMTK_INFO("%s", __func__);

	tmp_bdev = bdev;
	if (tmp_bdev != NULL) {
		for (i = 0; i < btmtk_intf_num; i++) {
			/* Find slot on probed interface.
			 * Judged from load_rom_patch is done and
			 * Identified chip_id from cap_init.
			 */
			if (memcmp(tmp_bdev, g_bdev[i], sizeof(*tmp_bdev)) == 0) {
				memset(tmp_bdev, 0, sizeof(*tmp_bdev));
				/* reset pin initial value need to be -1, used to judge after
				 * disconnected before probe, can't do chip reset
				 */
				bdev->bt_cfg.dongle_reset_gpio_pin = -1;
				break;
			}
		}
	}

}

struct btmtk_dev *btmtk_allocate_dev_memory(struct device *dev)
{
	struct btmtk_dev *bdev = NULL;
	size_t len = sizeof(*bdev);

	BTMTK_DBG("%s", __func__);

	if (dev != NULL)
		bdev = devm_kzalloc(dev, len, GFP_KERNEL);
	else
		bdev = kzalloc(len, GFP_KERNEL);

	if (!bdev)
		return NULL;
#if (USE_DEVICE_NODE == 0)
	memset(bdev, 0, sizeof(struct btmtk_dev));
#endif
	btmtk_set_chip_state(bdev, BTMTK_STATE_INIT);

	return bdev;
}

void btmtk_free_dev_memory(struct device *dev, struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s", __func__);

	if (bdev != NULL) {
		if (dev != NULL)
			devm_kfree(dev, bdev);
		else
			kfree(bdev);
	}
}

static int btmtk_calibration_flow(struct btmtk_dev *bdev)
{
	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return -1;
	}

	btmtk_cif_send_calibration(bdev);
	BTMTK_INFO("%s done", __func__);
	return 0;
}

int btmtk_send_wmt_power_on_cmd(struct btmtk_dev *bdev)
{
	int ret = -1, retry = RETRY_TIMES;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return ret;
	}

retry_again:
	ret = btmtk_send_cmd_to_fw(bdev,
		WMT_POWER_ON_CMD, WMT_POWER_ON_EVT,
		WMT_DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		ret = -1;
	} else if (ret == 0 && bdev->recv_evt_len > 0) {
		switch (bdev->io_buf[WMT_POWER_ON_EVT_RESULT_OFFSET]) {
		case 0:			 /* successful */
			BTMTK_INFO("%s: OK", __func__);
			bdev->power_state = BTMTK_DONGLE_STATE_POWER_ON;
			break;
		case 2:			 /* TODO:retry */
			if (retry > 0) {
				/* comment from fw, we need to retry a sec until power on successfully. */
				retry--;
				BTMTK_INFO("%s: need to try again", __func__);
				msleep(50);
				goto retry_again;
			}
			break;
		default:
			BTMTK_WARN("%s: Unknown result: %02X", __func__, bdev->io_buf[WMT_POWER_ON_EVT_RESULT_OFFSET]);
			bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
			ret = -1;
			break;
		}
	}

	return ret;
}

int btmtk_send_wmt_power_off_cmd(struct btmtk_dev *bdev)
{
	int ret = 0;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return ret;
	}

	if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		BTMTK_WARN("%s: power_state already power off", __func__);
		return 0;
	}

	ret = btmtk_send_cmd_to_fw(bdev,
		WMT_POWER_OFF_CMD, WMT_POWER_OFF_EVT,
		DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		return ret;
	}

	bdev->power_state = BTMTK_DONGLE_STATE_POWER_OFF;
	BTMTK_INFO("%s done", __func__);
	return ret;
}

/* Check power status, if power is off, try to set power on */
int btmtk_reset_power_on(struct btmtk_dev *bdev)
{
	if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		if (is_mt6639(bdev->chip_id))
			btmtk_dynamic_load_rom_patch(bdev, 1);

		if (btmtk_send_wmt_power_on_cmd(bdev) < 0)
			return -1;
		bdev->power_state = BTMTK_DONGLE_STATE_POWER_ON;
	}

	if (bdev->power_state != BTMTK_DONGLE_STATE_POWER_ON) {
		BTMTK_WARN("%s: end of Incorrect state:%d", __func__, bdev->power_state);
		return -1;
	}
	BTMTK_INFO("%s: end success", __func__);

	return 0;
}
#if (STPBTFWLOG_ENABLE == 1)
int btmtk_picus_enable(struct btmtk_dev *bdev, int via_uart)
{
	int ret = 0;	/* if successful, 0 */
	struct data_struct cmd = {0}, event = {0};

	BTMTK_INFO("%s", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, PICUS_ENABLE_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, PICUS_ENABLE_EVT, event);

	if (via_uart) {
		cmd.content[cmd.len - 1] = 0x03;
		cmd.content[cmd.len - 2] = 0x03;
	} else {
		cmd.content[cmd.len - 1] = 0x02;
		cmd.content[cmd.len - 2] = 0x02;
	}

	BTMTK_INFO_RAW(cmd.content,
			cmd.len, "%s: ", __func__);

	ret = btmtk_main_send_cmd(bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s: ret %d", __func__, ret);
	return ret;
}

int btmtk_picus_disable(struct btmtk_dev *bdev)
{
	int ret = 0;	/* if successful, 0 */

	BTMTK_INFO("%s", __func__);

	ret = btmtk_send_cmd_to_fw(bdev,
			PICUS_DISABLE_CMD, PICUS_DISABLE_EVT,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s: ret %d", __func__, ret);
	return ret;
}
#endif

void btmtk_save_filter_vendor_cmd(struct sk_buff *skb, struct btmtk_dev *bdev, bool flag)
{
	int i = 0;
	u16 recv_opcode = 0;
	u16 filter_list_opcode = 0;

	if (skb->data[0] != 0x01 || flag != CMD_NEED_FILTER)
		return;

	recv_opcode = (skb->data[2] << 8) | skb->data[1];

	for (; i < FILTER_VENDOR_CMD_COUNT; i++) {
		if (bdev->bt_cfg.filter_vendor_cmd[i].content == NULL)
			return;

		filter_list_opcode =
			*(u16 *)&bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_LIST_OPCODE_1];
		if (recv_opcode == filter_list_opcode) {
			btmtk_filter_cmd_num_lock(bdev);
			bdev->need_compare_num +=
				(bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_LIST_COMPLETE_NUM] +
				bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_LIST_STATUS_NUM]);
			bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_OPCODE_NUM] =
				(bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_LIST_COMPLETE_NUM] +
				bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_LIST_STATUS_NUM]);

			BTMTK_INFO("%s: save opcode=0x%04X", __func__, recv_opcode);
			btmtk_filter_cmd_num_unlock(bdev);
		}
	}
}

int btmtk_vendor_cmd_filter(struct btmtk_dev *bdev, struct sk_buff *skb)
{
	int i = 0;
	u16 recv_opcode = 0;
	u16 filter_list_opcode = 0;

	if (!((skb->data[0] == 0x0E || skb->data[0] == 0x0F) && bdev->need_compare_num))
		return 0;

	if (skb->data[0] == 0x0E)
		recv_opcode = (skb->data[4] << 8) | skb->data[3];
	else
		recv_opcode = (skb->data[5] << 8) | skb->data[4];

	for (; i < FILTER_VENDOR_CMD_COUNT; i++) {
		if (bdev->bt_cfg.filter_vendor_cmd[i].content == NULL)
			return 0;

		if (bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_OPCODE_NUM]) {
			filter_list_opcode =
			*(u16 *)&bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_LIST_OPCODE_1];
			if (recv_opcode == filter_list_opcode) {
				btmtk_filter_cmd_num_lock(bdev);
				bdev->need_compare_num--;
				bdev->bt_cfg.filter_vendor_cmd[i].content[FILTER_OPCODE_NUM]--;
				btmtk_filter_cmd_num_unlock(bdev);
				return 1;
			}
		}
	}

	return 0;
}

int btmtk_load_fw_cfg_setting(char *block_name, struct fw_cfg_struct *save_content,
		int counter, u8 *searchcontent, enum fw_cfg_index_len index_length)
{
	int ret = 0, i = 0;
	u16 temp_len = 0;
	u8 temp[TEMP_LEN]; /* save for total hex number */
	unsigned long parsing_result = 0;
	char *search_result = NULL, *ptr = NULL;
	char *search_end = NULL;
	char search[SEARCH_LEN];
	char *next_block = NULL;
	char number[CHAR2HEX_SIZE + 1];	/* 1 is for '\0' */

	memset(search, 0, SEARCH_LEN);
	memset(temp, 0, TEMP_LEN);
	memset(number, 0, CHAR2HEX_SIZE + 1);

	if (searchcontent == NULL) {
		BTMTK_ERR("%s: Searchcontent is NULL", __func__);
		return -1;
	}

	/* search block name */
	for (i = 0; i < counter; i++) {
		temp_len = 0;
		if (index_length == FW_CFG_INX_LEN_2) /* EX: APCF01 */
			(void)snprintf(search, SEARCH_LEN, "%s%02d:", block_name, i);
		else if (index_length == FW_CFG_INX_LEN_3) /* EX: APCF001 */
			(void)snprintf(search, SEARCH_LEN, "%s%03d:", block_name, i);
		else
			(void)snprintf(search, SEARCH_LEN, "%s:", block_name);

		ret = 0;

		ptr = search_result = strstr((char *)searchcontent, search);
		if (search_result) {
			/* Add # for comment in bt.cfg */
			if (ptr > (char *)searchcontent) {
				ptr--;
				while ((*ptr == ' ') && (ptr != (char *)searchcontent))
					ptr--;
				if (*ptr == '#') {
					BTMTK_WARN("%s: %s has been ignored", __func__, search);
					return -1;
				}
			}

			memset(temp, 0, TEMP_LEN);
			search_result = strstr(search_result, "0x");
			if (search_result == NULL) {
				BTMTK_ERR("%s: search_result is NULL", __func__);
				return -1;
			}

			/* find next line as end of this command line, if NULL means last line */
			next_block = strstr(search_result, ":");
			if (next_block == NULL)
				BTMTK_WARN("%s: if NULL means last line", __func__);

			/* Add HCI packet type to front of each command/event */
			if (!memcmp(block_name, "APCF", sizeof("APCF")) ||
				!memcmp(block_name, "RADIOOFF", sizeof("RADIOOFF")) ||
				!memcmp(block_name, "RADIOON", sizeof("RADIOON")) ||
				!memcmp(block_name, "APCF_RESUME", sizeof("APCF_RESUME")) ||
				!memcmp(block_name, "VENDOR_CMD", sizeof("VENDOR_CMD")) ||
				!memcmp(block_name, "PHASE1_WMT_CMD", sizeof("PHASE1_WMT_CMD"))) {
				temp[0] = 0x01;
				temp_len++;
			} else if (!memcmp(block_name, "RADIOOFF_STATUS_EVENT", sizeof("RADIOOFF_STATUS_EVENT")) ||
				!memcmp(block_name, "RADIOOFF_COMPLETE_EVENT", sizeof("RADIOOFF_COMPLETE_EVENT")) ||
				!memcmp(block_name, "RADIOON_STATUS_EVENT", sizeof("RADIOON_STATUS_EVENT")) ||
				!memcmp(block_name, "RADIOON_COMPLETE_EVENT", sizeof("RADIOON_COMPLETE_EVENT"))) {
				temp[0] = 0x04;
				temp_len++;
			}

			do {
				search_end = strstr(search_result, ",");
				if (search_end == NULL) {
					BTMTK_ERR("%s: Search_end is NULL", __func__);
					break;
				}

				if (search_end - search_result != CHAR2HEX_SIZE) {
					BTMTK_ERR("%s: Incorrect Format in %s", __func__, search);
					break;
				}

				memset(number, 0, CHAR2HEX_SIZE + 1);
				memcpy(number, search_result, CHAR2HEX_SIZE);
				ret = kstrtoul(number, 0, &parsing_result);
				if (ret == 0) {
					if (temp_len >= TEMP_LEN) {
						BTMTK_ERR("%s: %s data over %d", __func__, search, TEMP_LEN);
						break;
					}
					temp[temp_len] = parsing_result;
					temp_len++;
				} else {
					BTMTK_WARN("%s: %s kstrtoul fail: %d", __func__, search, ret);
					break;
				}
				search_result = strstr(search_end, "0x");
				if (search_result == NULL || (search_result - search_end) != 2) {
					BTMTK_ERR("%s: search_result is NULL", __func__);
					break;
				}
			} while (search_result < next_block || (search_result && next_block == NULL));
		} else
			BTMTK_DBG("%s: %s is not found in %d", __func__, search, i);

		if (temp_len && temp_len < TEMP_LEN) {
			BTMTK_INFO("%s: %s found & stored in %d", __func__, search, i);
			save_content[i].content = kzalloc(temp_len, GFP_KERNEL);
			if (save_content[i].content == NULL) {
				BTMTK_ERR("%s: Allocate memory fail(%d)", __func__, i);
				return -ENOMEM;
			}
			memcpy(save_content[i].content, temp, temp_len);
			save_content[i].length = temp_len;
			BTMTK_DBG_RAW(save_content[i].content, save_content[i].length, "%s", search);
		}
	}

	return ret;
}

int btmtk_load_code_from_setting_files(char *setting_file_name,
			struct device *dev, u32 *code_len, struct btmtk_dev *bdev)
{
	int err = 0;
	const struct firmware *fw_entry = NULL;

	*code_len = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: g_data is NULL!!", __func__);
		err = -EINVAL;
		goto end;
	}

	BTMTK_INFO("%s: begin setting_file_name = %s", __func__, setting_file_name);
	err = request_firmware(&fw_entry, setting_file_name, dev);
	if (err != 0 || fw_entry == NULL) {
		BTMTK_INFO("%s: request_firmware fail, maybe file %s not exist, err = %d, fw_entry = %p",
				__func__, setting_file_name, err, fw_entry);
		if (fw_entry)
			release_firmware(fw_entry);
		err = -ENOENT;
		goto end;
	}

	BTMTK_INFO("%s: setting file request_firmware size %zu success", __func__, fw_entry->size);
	if (bdev->setting_file != NULL) {
		kfree(bdev->setting_file);
		bdev->setting_file = NULL;
	}
	bdev->setting_file = kzalloc(fw_entry->size + 1, GFP_KERNEL); /* alloc setting file memory */
	if (bdev->setting_file == NULL) {
		BTMTK_ERR("%s: kzalloc size %zu failed!!", __func__, fw_entry->size);
		release_firmware(fw_entry);
		err = -ENOMEM;
		goto end;
	}

	memcpy(bdev->setting_file, fw_entry->data, fw_entry->size);
	bdev->setting_file[fw_entry->size] = '\0';

	*code_len = fw_entry->size;
	release_firmware(fw_entry);

	BTMTK_INFO("%s: setting_file len (%d) assign done", __func__, *code_len);
end:
	return err;
}

#if (USE_DEVICE_NODE == 0)
static bool btmtk_parse_bt_cfg_file(char *item_name,
		char *text, u8 *searchcontent)
{
	bool ret = 0;
	int temp_len = 0;
	char search[SEARCH_LEN];
	char *ptr = NULL, *p = NULL;
	char *temp = text;

	if (text == NULL) {
		BTMTK_ERR("%s: text param is invalid!", __func__);
		ret = false;
		goto out;
	}

	memset(search, 0, SEARCH_LEN);
	(void)snprintf(search, SEARCH_LEN, "%s", item_name); /* EX: SUPPORT_UNIFY_WOBLE */
	p = ptr = strstr((char *)searchcontent, search);

	if (!ptr) {
		BTMTK_ERR("%s: Can't find %s", __func__, item_name);
		ret = false;
		goto out;
	}

	if (p > (char *)searchcontent) {
		p--;
		while ((*p == ' ') && (p != (char *)searchcontent))
			p--;
		if (*p == '#') {
			BTMTK_ERR("%s: It's invalid bt cfg item", __func__);
			ret = false;
			goto out;
		}
	}

	p = ptr + strlen(item_name) + 1;
	ptr = p;

	for (;;) {
		switch (*p) {
		case '\n':
			goto textdone;
		default:
			*temp++ = *p++;
			break;
		}
	}

textdone:
	temp_len = p - ptr;
	*temp = '\0';
	ret = true;

out:
	return ret;
}

static void btmtk_bt_cfg_item_value_to_bool(char *item_value, bool *value)
{
	unsigned long text_value = 0;

	if (item_value == NULL) {
		BTMTK_ERR("%s: item_value is NULL!", __func__);
		return;
	}

	if (kstrtoul(item_value, 10, &text_value) == 0) {
		if (text_value == 1)
			*value = true;
		else
			*value = false;
	} else {
		BTMTK_WARN("%s: kstrtoul failed!", __func__);
	}
}

static void btmtk_load_bt_cfg_item(struct bt_cfg_struct *bt_cfg_content,
		u8 *searchcontent, struct btmtk_dev *bdev)
{
	bool ret = 0;
	char text[TEXT_LEN]; /* save for search text */
	unsigned long text_value = 0;

	memset(text, 0, TEXT_LEN);
	ret = btmtk_parse_bt_cfg_file(BT_LOG_LVL, text, searchcontent);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			btmtk_log_lvl = text_value;
		else
			BTMTK_WARN("%s: kstrtoul failed %s!", __func__, BT_LOG_LVL);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_LOG_LVL);
	}

	BTMTK_INFO("%s: btmtk_log_lvl = %d", __func__,btmtk_log_lvl);

	ret = btmtk_parse_bt_cfg_file(BT_UNIFY_WOBLE, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_unify_woble);
		BTMTK_INFO("%s: bt_cfg_content->support_unify_woble = %d", __func__,
				bt_cfg_content->support_unify_woble);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_UNIFY_WOBLE);
	}

	ret = btmtk_parse_bt_cfg_file(BT_UNIFY_WOBLE_TYPE, text, searchcontent);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			bt_cfg_content->unify_woble_type = text_value;
		else
			BTMTK_WARN("%s: kstrtoul failed %s!", __func__, BT_UNIFY_WOBLE_TYPE);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_UNIFY_WOBLE_TYPE);
	}

	BTMTK_INFO("%s: bt_cfg_content->unify_woble_type = %d", __func__,
			bt_cfg_content->unify_woble_type);

	ret = btmtk_parse_bt_cfg_file(BT_WOBLE_BY_EINT, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_woble_by_eint);
		BTMTK_INFO("%s: bt_cfg_content->support_woble_by_eint = %d", __func__,
					bt_cfg_content->support_woble_by_eint);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_WOBLE_BY_EINT);
	}

	ret = btmtk_parse_bt_cfg_file(BT_DONGLE_RESET_PIN, text, searchcontent);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			bt_cfg_content->dongle_reset_gpio_pin = text_value;
		else
			BTMTK_WARN("%s: kstrtoul failed %s!", __func__, BT_DONGLE_RESET_PIN);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_DONGLE_RESET_PIN);
	}

	BTMTK_INFO("%s: bt_cfg_content->dongle_reset_gpio_pin = %d", __func__,
			bt_cfg_content->dongle_reset_gpio_pin);

	ret = btmtk_parse_bt_cfg_file(BT_SUBSYS_RESET_DONGLE, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_dongle_subsys_reset );
		BTMTK_INFO("%s: bt_cfg_content->support_dongle_subsys_reset  = %d", __func__,
				bt_cfg_content->support_dongle_subsys_reset );
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_SUBSYS_RESET_DONGLE);
	}

	ret = btmtk_parse_bt_cfg_file(BT_WHOLE_RESET_DONGLE, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_dongle_whole_reset);
		BTMTK_INFO("%s: bt_cfg_content->support_dongle_whole_reset = %d", __func__,
				bt_cfg_content->support_dongle_whole_reset);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_WHOLE_RESET_DONGLE);
	}

	ret = btmtk_parse_bt_cfg_file(BT_WOBLE_WAKELOCK, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_woble_wakelock);
		BTMTK_INFO("%s: bt_cfg_content->support_woble_wakelock = %d", __func__,
				bt_cfg_content->support_woble_wakelock);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_WOBLE_WAKELOCK);
	}

	ret = btmtk_parse_bt_cfg_file(BT_WOBLE_FOR_BT_DISABLE, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_woble_for_bt_disable);
		BTMTK_INFO("%s: bt_cfg_content->support_woble_for_bt_disable = %d", __func__,
				bt_cfg_content->support_woble_for_bt_disable);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_WOBLE_FOR_BT_DISABLE);
	}

	ret = btmtk_parse_bt_cfg_file(BT_RESET_STACK_AFTER_WOBLE, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->reset_stack_after_woble);
		BTMTK_INFO("%s: bt_cfg_content->reset_stack_after_woble = %d", __func__,
				bt_cfg_content->reset_stack_after_woble);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_RESET_STACK_AFTER_WOBLE);
	}

	ret = btmtk_parse_bt_cfg_file(BT_AUTO_PICUS, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_auto_picus);
		BTMTK_INFO("%s: bt_cfg_content->support_auto_picus = %d", __func__,
				bt_cfg_content->support_auto_picus);
		if (bt_cfg_content->support_auto_picus == true) {
			ret = btmtk_load_fw_cfg_setting(BT_AUTO_PICUS_FILTER,
					&bt_cfg_content->picus_filter, 1, searchcontent, FW_CFG_INX_LEN_NONE);
			if (ret)
				BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUTO_PICUS_FILTER);

			ret = btmtk_load_fw_cfg_setting(BT_AUTO_PICUS_ENABLE,
					&bt_cfg_content->picus_enable, 1, searchcontent, FW_CFG_INX_LEN_NONE);
			if (ret)
				BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUTO_PICUS_ENABLE);
		}
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUTO_PICUS);
	}

	ret = btmtk_parse_bt_cfg_file(BT_PICUS_TO_HOST, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_picus_to_host);
		BTMTK_INFO("%s: bt_cfg_content->support_picus_to_host = %d", __func__,
				bt_cfg_content->support_picus_to_host);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_PICUS_TO_HOST);
	}

	ret = btmtk_parse_bt_cfg_file(BT_SINGLE_SKU, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_bt_single_sku);
		BTMTK_INFO("%s: bt_cfg_content->support_bt_single_sku = %d", __func__,
				bt_cfg_content->support_bt_single_sku);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_SINGLE_SKU);
	}

#if BTMTK_ISOC_TEST
	ret = btmtk_parse_bt_cfg_file(BT_USB_SCO_TEST, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_usb_sco_test);
		BTMTK_INFO("%s: bt_cfg_content->support_usb_sco_test = %d", __func__,
				bt_cfg_content->support_usb_sco_test);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_USB_SCO_TEST);
	}
#endif

	ret = btmtk_parse_bt_cfg_file(BT_AUDIO_SET, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_audio_setting);
		BTMTK_INFO("%s: bt_cfg_content->support_audio_setting = %d", __func__,
				bt_cfg_content->support_audio_setting);
		if (bt_cfg_content->support_audio_setting == true) {
			ret = btmtk_load_fw_cfg_setting(BT_AUDIO_ENABLE_CMD,
					&bt_cfg_content->audio_cmd, 1, searchcontent, FW_CFG_INX_LEN_NONE);
			if (ret)
				BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUDIO_ENABLE_CMD);

			ret = btmtk_load_fw_cfg_setting(BT_AUDIO_PINMUX_NUM,
					&bt_cfg_content->audio_pinmux_num, 1, searchcontent, FW_CFG_INX_LEN_NONE);
			if (ret)
				BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUDIO_PINMUX_NUM);

			ret = btmtk_load_fw_cfg_setting(BT_AUDIO_PINMUX_MODE,
					&bt_cfg_content->audio_pinmux_mode, 1, searchcontent, FW_CFG_INX_LEN_NONE);
			if (ret)
				BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUDIO_PINMUX_MODE);
		}
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_AUDIO_SET);
		bt_cfg_content->support_audio_setting = true; /* default to turn on, for others not update */
		BTMTK_WARN("%s: %s default turn on %d!", __func__, BT_AUDIO_SET,
				bt_cfg_content->support_audio_setting);
	}

	ret = btmtk_load_fw_cfg_setting(BT_PHASE1_WMT_CMD, bt_cfg_content->phase1_wmt_cmd,
				PHASE1_WMT_CMD_COUNT, searchcontent, FW_CFG_INX_LEN_3);
	if (ret)
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_PHASE1_WMT_CMD);

	ret = btmtk_load_fw_cfg_setting(BT_VENDOR_CMD, bt_cfg_content->vendor_cmd,
				VENDOR_CMD_COUNT, searchcontent, FW_CFG_INX_LEN_3);
	if (ret)
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_VENDOR_CMD);

	ret = btmtk_load_fw_cfg_setting(BT_FILTER_VENDOR_CMD, bt_cfg_content->filter_vendor_cmd,
			FILTER_VENDOR_CMD_COUNT, searchcontent, FW_CFG_INX_LEN_3);
	if (ret)
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_FILTER_VENDOR_CMD);

	ret = btmtk_parse_bt_cfg_file(BT_DEBUG_SOP_MODE, text, searchcontent);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			bt_cfg_content->debug_sop_mode = text_value;
		else
			BTMTK_WARN("%s: kstrtoul failed %s!", __func__, BT_DEBUG_SOP_MODE);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_DEBUG_SOP_MODE);
	}

	BTMTK_INFO("%s: bt_cfg_content->debug_sop_mode = %d", __func__,
			bt_cfg_content->debug_sop_mode);

	ret = btmtk_parse_bt_cfg_file(DTS_SUBSYS_RST, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_dts_subsys_rst);
		BTMTK_INFO("%s: bt_cfg_content->support_dts_subsys_rst = %d",
			__func__,
			bt_cfg_content->support_dts_subsys_rst);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, DTS_SUBSYS_RST);	}

#if CFG_SUPPORT_LEAUDIO_CLK
	ret = btmtk_parse_bt_cfg_file(BT_LE_AUDIO_CLK, text, searchcontent);
	if (ret) {
		btmtk_bt_cfg_item_value_to_bool(text, &bt_cfg_content->support_le_audio_clk);
		BTMTK_INFO("%s: bt_cfg_content->support_le_audio_clk = %d", __func__,
				bt_cfg_content->support_le_audio_clk);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_LE_AUDIO_CLK);
	}
	ret = btmtk_parse_bt_cfg_file(BT_LE_AUDIO_CLK_FW_GPIO, text, searchcontent);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			bt_cfg_content->le_audio_clk_fw_gpio = text_value;
		else
			BTMTK_WARN("%s: kstrtoul failed %s!", __func__, BT_DONGLE_RESET_PIN);
	} else {
		BTMTK_WARN("%s: search item %s is invalid!", __func__, BT_LE_AUDIO_CLK_FW_GPIO);
	}
#endif

	/* release setting file memory */
	if (bdev) {
		kfree(bdev->setting_file);
		bdev->setting_file = NULL;
	}
}

static void btmtk_load_bt_cfg(char *cfg_name, struct device *dev, struct btmtk_dev *bdev)
{
	int ret = 0;
	u32 code_len = 0;

	ret = btmtk_load_code_from_setting_files(cfg_name, dev, &code_len, bdev);
	if (ret != 0) {
		BTMTK_ERR("btmtk_usb_load_code_from_setting_files %s failed!!", cfg_name);
		if (ret != -ENOENT)
			return;

		if (btmtk_load_code_from_setting_files(BT_CFG_NAME, dev, &code_len, bdev) != 0) {
			BTMTK_ERR("btmtk_usb_load_code_from_setting_files %s failed!!", BT_CFG_NAME);
			return;
		}
		(void)snprintf(bdev->bt_cfg_file_name, MAX_BIN_FILE_NAME_LEN, "%s", BT_CFG_NAME);
	}

	btmtk_load_bt_cfg_item(&bdev->bt_cfg, bdev->setting_file, bdev);
}
#endif // (USE_DEVICE_NODE == 0)

#if ENABLESTP
static int btmtk_send_set_stp_cmd(struct btmtk_dev *bdev)
{
	int ret = 0;

	ret = btmtk_send_cmd_to_fw(bdev,
		STP0_CMD, STP0_EVT,
		0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}

static int btmtk_send_set_stp1_cmd(struct btmtk_dev *bdev)
{
	int ret = 0;

	ret = btmtk_send_cmd_to_fw(bdev,
		STP1_CMD, STP1_EVT,
		0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	BTMTK_INFO("%s done", __func__);
	return ret;
}
#endif

int btmtk_cap_init(struct btmtk_dev *bdev)
{
	int ret = 0;

	BTMTK_DBG("%s start", __func__);
	if (!bdev) {
		BTMTK_ERR("%s, bdev is NULL!", __func__);
		ret = -1;
		goto exit;
	}

	ret = btmtk_common_data_initialize(bdev); /* load default data */
	if (ret) {
		BTMTK_ERR("%s, load default data failed!", __func__);
		ret = -1;
		goto exit;
	}

	btmtk_cif_chip_common_register(); /* load default hook func */

#if (USE_DEVICE_NODE == 1)
	BTMTK_INFO("%s bdev->chip_id = 0x%04x", __func__, bdev->chip_id);
	if (!bdev->chip_id)
		bdev->chip_id = 0x6653;

	ret = btmtk_cif_chip_register(bdev);
	if (ret == -ENOENT) {
		BTMTK_ERR("btmtk_cif_chip_register failed: no such file");
		goto exit;
	} else	if (ret < 0) {
		BTMTK_ERR("btmtk_cif_chip_register failed");
		ret = -EIO;
		goto exit;
	}
#else
	/* Todo read wifi fw version
	 * int wifi_fw_ver;

	 * btmtk_cif_write_register(bdev, 0x7C4001C4, 0x00008800);
	 * btmtk_cif_read_register(bdev, 0x7c4f0004, &wifi_fw_ver);
	 * BTMTK_ERR("wifi fw_ver = %04X", wifi_fw_ver);
	 */

	ret = main_info.hif_hook.reg_read(bdev, CHIP_ID, &bdev->chip_id);
	if (ret < 0) {
		BTMTK_ERR("read chip id failed");
		ret = -EIO;
		goto exit;
	}

	if (is_connac3(bdev->chip_id))
		btmtk_cif_chip_commonv3_register(); /* load connac3 default hook func */

	ret = btmtk_cif_chip_register(bdev);
	if (ret == -ENOENT) {
		BTMTK_ERR("btmtk_cif_chip_register failed: no such file");
		goto exit;
	} else	if (ret < 0) {
		BTMTK_ERR("btmtk_cif_chip_register failed");
		ret = -EIO;
		goto exit;
	}

	if (main_info.hif_hook_chip.get_fw_info) {
		ret = main_info.hif_hook_chip.get_fw_info(bdev);
		if (ret < 0) {
			BTMTK_ERR("%s, get_fw_info error(%d)", __func__, ret);
			goto exit;
		}
	}

	/* if flavor equals 1, it represent 7920, else it represent 7921 */
	if (bdev->flavor) {
		(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN, "BT_RAM_CODE_MT%04x_%xa_%x_hdr.bin",
				bdev->chip_id & 0xffff, (bdev->proj & 0xff) + 1, (bdev->fw_version & 0xff) + 1);
		(void)snprintf(bdev->bt_cfg_file_name, MAX_BIN_FILE_NAME_LEN, "%s%x_%xa_%x.%s",
				BT_CFG_NAME_PREFIX, bdev->chip_id & 0xffff,
				(bdev->proj & 0xff) + 1, (bdev->fw_version & 0xff) + 1, BT_CFG_NAME_SUFFIX);
	} else {
		(void)snprintf(bdev->rom_patch_bin_file_name, MAX_BIN_FILE_NAME_LEN, "BT_RAM_CODE_MT%04x_%x_%x_hdr.bin",
				bdev->chip_id & 0xffff, (bdev->proj & 0xff) + 1, (bdev->fw_version & 0xff) + 1);
		(void)snprintf(bdev->bt_cfg_file_name, MAX_BIN_FILE_NAME_LEN, "%s%x_%x_%x.%s",
				BT_CFG_NAME_PREFIX, bdev->chip_id & 0xffff,
				(bdev->proj & 0xff) + 1, (bdev->fw_version & 0xff) + 1, BT_CFG_NAME_SUFFIX);
	}

	if (is_mt7925(bdev->chip_id)) {
		(void)snprintf(bdev->bt_cfg_file_name, MAX_BIN_FILE_NAME_LEN, "%s%x_%x_%x.%s",
				BT_CFG_NAME_PREFIX, bdev->chip_id & 0xffff,
				(bdev->proj & 0xff) + 1, (bdev->fw_version & 0xff) + 1, BT_CFG_NAME_SUFFIX);
	}

	BTMTK_INFO("%s: rom patch file name is %s, bt_cfg_file_name is %s", __func__,
		bdev->rom_patch_bin_file_name, bdev->bt_cfg_file_name);

	memset(bdev->bdaddr, 0, BD_ADDRESS_SIZE);
#endif
exit:
	return ret;
}

#if CFG_SUPPORT_BLUEZ
#else
__attribute__((unused)) static int btmtk_send_vendor_cfg(struct btmtk_dev *bdev)
{
	int ret = 0;
	u16 index = 0;
	struct data_struct event = {0};

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, VENDOR_EVT, event);

	for (index = 0; index < VENDOR_CMD_COUNT; index++) {
		if (bdev->bt_cfg.vendor_cmd[index].content &&
			bdev->bt_cfg.vendor_cmd[index].length) {
			BTMTK_INFO_RAW(bdev->bt_cfg.vendor_cmd[index].content,
				bdev->bt_cfg.vendor_cmd[index].length, "send vendor cmd");

			ret = btmtk_main_send_cmd(bdev,
				bdev->bt_cfg.vendor_cmd[index].content,
				bdev->bt_cfg.vendor_cmd[index].length,
				event.content, event.len,
				0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_ERR("%s: Send vendor cmd failed(%d)! Index: %d",
					__func__, ret, index);
				goto exit;
			}
		}
	}

exit:
	BTMTK_INFO("%s exit", __func__);
	return ret;
}
#endif

static int btmtk_send_phase1_wmt_cfg(struct btmtk_dev *bdev)
{
	int ret = 0;
	u16 index = 0;
	struct data_struct event = {0};

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, PHASE1_WMT_EVT, event);

	for (index = 0; index < PHASE1_WMT_CMD_COUNT; index++) {
		if (bdev->bt_cfg.phase1_wmt_cmd[index].content &&
			bdev->bt_cfg.phase1_wmt_cmd[index].length) {
			ret = btmtk_main_send_cmd(bdev,
					bdev->bt_cfg.phase1_wmt_cmd[index].content,
					bdev->bt_cfg.phase1_wmt_cmd[index].length,
					event.content, event.len,
					DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_ERR("%s: Send phase1 wmt cmd failed(%d)! Index: %d",
					__func__, ret, index);
				goto exit;
			}

			BTMTK_INFO_RAW(bdev->bt_cfg.phase1_wmt_cmd[index].content,
				bdev->bt_cfg.phase1_wmt_cmd[index].length, "send wmt cmd");
		}
	}

exit:
	BTMTK_INFO("%s exit", __func__);
	return ret;
}

int btmtk_send_init_cmds(struct btmtk_dev *bdev)
{
	int ret = -1;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		goto exit;
	}

	BTMTK_INFO("%s", __func__);

#if ENABLESTP
	btmtk_send_set_stp_cmd(bdev);
	btmtk_send_set_stp1_cmd(bdev);
#endif
	ret = btmtk_calibration_flow(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s, btmtk_calibration_flow failed!", __func__);
		goto exit;
	}
	ret = btmtk_send_wmt_power_on_cmd(bdev);
	if (ret < 0) {
		if (bdev->power_state != BTMTK_DONGLE_STATE_POWER_ON) {
			BTMTK_ERR("%s, btmtk_send_wmt_power_on_cmd failed!", __func__);
			if (main_info.reset_stack_flag == HW_ERR_NONE)
				main_info.reset_stack_flag = HW_ERR_CODE_POWER_ON;
		}
		goto exit;
	}

	ret = btmtk_send_phase1_wmt_cfg(bdev);
	if (ret < 0) {
		BTMTK_ERR("btmtk_send_wmt_cfg failed");
		goto exit;
	}

	if (bdev->bt_cfg.support_auto_picus == true &&
		(bdev->bt_cfg.support_picus_to_host == true || atomic_read(&bmain_info->fwlog_ref_cnt) != 0)) {
#if (STPBTFWLOG_ENABLE == 1)
		if (btmtk_picus_enable(bdev, 0) < 0) {
			BTMTK_ERR("send picus filter param failed");
			ret = -1;
			goto exit;
		}
#endif
	}

#if CFG_SUPPORT_BLUEZ
#else
	ret = btmtk_send_vendor_cfg(bdev);
	if (ret < 0) {
		BTMTK_ERR("btmtk_send_vendor_cfg failed");
		goto exit;
	}
#endif

exit:
	return ret;
}


int btmtk_send_deinit_cmds(struct btmtk_dev *bdev)
{
	int ret = -1;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return ret;
	}

	BTMTK_DBG("%s", __func__);

	if (bdev->bt_cfg.support_auto_picus == true &&
		(bdev->bt_cfg.support_picus_to_host == true || atomic_read(&bmain_info->fwlog_ref_cnt) != 0)) {
#if (STPBTFWLOG_ENABLE == 1)
		if (btmtk_picus_disable(bdev) < 0) {
			BTMTK_ERR("send picus filter param failed");
			return -1;
		}
#endif
		btmtk_update_bt_status(0);
	}

#if (USE_DEVICE_NODE == 1)
	if (bmain_info->find_my_phone_mode) {
		btmtk_find_my_phone_cmd(bmain_info->find_my_phone_mode);
	}

	BTMTK_INFO("%s: poweroff_finder_mode = %d, bdev = %p", __func__, atomic_read(&bdev->poweroff_finder_mode), bdev);
        if (atomic_read(&bdev->poweroff_finder_mode)) {
		u8 retry = 20;
		u8 count = 0;
		ret = connv3_enter_fmd_mode();
		if (ret != 0)
			BTMTK_ERR("%s: connv3_enter_fmd_mode failed", __func__);

		/* connv3 will callback in pre_fmd_cb => btmtk_set_powered_off_finder_mode */
		BTMTK_WARN("%s: FMD enable, do not sent power off", __func__);

		/* wait poweroff_finder_mode set to 0 in post_fmd_cb and for up to 10s */
		while(atomic_read(&bdev->poweroff_finder_mode) && ++count <= retry) {
			BTMTK_WARN("%s: poweroff_finder_mode = %d", __func__, atomic_read(&bdev->poweroff_finder_mode));
			mdelay(500);
		}

		return 0;
        }
#endif

	ret = btmtk_send_wmt_power_off_cmd(bdev);
	if (bdev->power_state != BTMTK_DONGLE_STATE_POWER_OFF) {
		BTMTK_WARN("Power off failed, reset it");
		if (main_info.reset_stack_flag == HW_ERR_NONE)
			main_info.reset_stack_flag = HW_ERR_CODE_POWER_OFF;
	}

	return ret;
}

int btmtk_send_assert_cmd(struct btmtk_dev *bdev)
{
	int ret = 0;
	int state;
	struct sk_buff *skb = NULL;
	unsigned char fstate = BTMTK_FOPS_STATE_INIT;
	struct data_struct cmd = {0};

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		ret = -EINVAL;
		goto exit;
	}

	fstate = btmtk_fops_get_state(bdev);
	state = btmtk_get_chip_state(bdev);
	/* use fop to judge bt off, not use chip state because chip state keep closed until wmt power on cmd success */
	if (state == BTMTK_STATE_FW_DUMP || state == BTMTK_STATE_SUSPEND ||
		state == BTMTK_STATE_SEND_ASSERT || fstate == BTMTK_FOPS_STATE_CLOSED) {
		BTMTK_WARN("%s: chip_state(%d) fop_state(%d) don't send assert",
			__func__, state, fstate);
		return ret;
	}

	BTMTK_INFO("%s: send assert cmd", __func__);
#if (USE_DEVICE_NODE == 0)
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, FD5B_ASSERT_CMD, cmd);
#else
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, RHW_ASSERT_CMD, cmd);
	if (bdev->chip_id == 0x6653) {
		cmd.content[9] = 0x01;
	}
#endif

	skb = alloc_skb(cmd.len + BT_SKB_RESERVE, GFP_KERNEL);
	if (!skb) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		goto exit;
	}
	btmtk_assert_wake_lock();
	bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
	memcpy(skb->data, cmd.content, cmd.len);
	skb->len = cmd.len;

#if (SLEEP_ENABLE == 0)
	ret = main_info.hif_hook.send_cmd(bdev, skb, WMT_DELAY_TIMES, RETRY_TIMES, (int)BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
#else
	/* use btmtk_main_send_cmd make sure drv own for BTMTK_TX_PKT_SEND_DIRECT
	 * evt would be same with cmd
	 */
	ret = btmtk_main_send_cmd(g_sbdev, cmd.content, cmd.len, cmd.content, cmd.len, DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT, CMD_NO_NEED_FILTER);
#endif

	if (ret < 0 && skb) {
		BTMTK_ERR("%s failed!!", __func__);
		kfree_skb(skb);
		skb = NULL;
#if (USE_DEVICE_NODE == 0)
		btmtk_reset_trigger(bdev);
#else
		btmtk_assert_wake_unlock();
		return ret;
#endif
	} else {
		btmtk_reset_timer_update(bdev);
		BTMTK_INFO("%s: OK", __func__);
		btmtk_set_chip_state(bdev, BTMTK_STATE_SEND_ASSERT);
	}

exit:
	return ret;
}

static int btmtk_send_txpower_cmd(struct btmtk_dev *bdev)
{
	/**
	 *  TCI Set TX Power Command
	 *  01 2C FC 0C QQ 00 00 00 XX YY ZZ GG AA BB CC DD
	 *  QQ: EDR init TX power dbm // the value is equal to EDR MAX
	 *  XX: BLE TX power dbm
	 *  YY: EDR MAX TX power dbm
	 *  ZZ: Enable LV9
	 *  GG: 3db diff mode
	 *  AA: [5:4] Indicator // [5] 1: command send to BT1, [4] 1: command send to BT0
	 *      [3:0] Resolution // 0: 1dBm, 1: 0.5dBm, 2: 0.25dBm
	 *  BB: BLE 2M
	 *  CC: BLE S2
	 *  DD: BLE S8
	 */

	int ret = 0;
	struct data_struct cmd = {0}, event = {0};

	BTMTK_INFO("%s: enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, TXPOWER_CMD, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, TXPOWER_EVT, event);

	cmd.content[4] = (u8)main_info.PWS.EDR_Max;
	cmd.content[8] = (u8)main_info.PWS.BLE_1M;
	cmd.content[9] = (u8)main_info.PWS.EDR_Max;
	cmd.content[10] = (u8)main_info.PWS.LV9;
	cmd.content[11] = (u8)main_info.PWS.DM;
	cmd.content[12] = (u8)main_info.PWS.IR;
	cmd.content[13] = (u8)main_info.PWS.BLE_2M;
	cmd.content[14] = (u8)main_info.PWS.BLE_LR_S2;
	cmd.content[15] = (u8)main_info.PWS.BLE_LR_S8;

	ret = btmtk_main_send_cmd(bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	if (ret < 0)
		BTMTK_ERR("%s failed!!", __func__);
	else
		BTMTK_INFO("%s: OK", __func__);

	return ret;
}

static int btmtk_set_power_value(char *str, int resolution, int is_edr)
{
	int power = ERR_PWR, integer = 0, decimal = 0;
	char *ptr = NULL;

	if (resolution == RES_DOT_25) {
		/* XX.YY => XX.YY/0.25 = XX*4 + YY/25 */
		if (strstr(str, ".")) {
			ptr = strsep(&str, ".");
			if (ptr == NULL)
				return -1;
			if (kstrtoint(ptr, 0, &integer) != 0) {
				BTMTK_ERR("Read integer Fail");
				return -1;
			}

			if (kstrtoint(str, 0, &decimal) != 0) {
				BTMTK_ERR("Read decimal Fail");
				return -1;
			}

			if (decimal != 25 && decimal != 75 && decimal != 5 && decimal != 50)
				return ERR_PWR;
			if (decimal == 5)
				decimal = 50;
			if (integer >= 0)
				power = integer * 4 + decimal / 25;
			else
				power = integer * 4 - decimal / 25;
		} else {
			if (kstrtoint(str, 0, &integer) != 0) {
				BTMTK_ERR("Read integer Fail");
				return -1;
			}
			power = integer * 4;
		}

		BTMTK_DBG("%s: power = %d", __func__, power);

		if (is_edr) {
			if (power > EDR_MAX_R2 || power < EDR_MIN_R2)
				return ERR_PWR;
			if (power >= EDR_MIN_LV9_R2)
				main_info.PWS.LV9 = 1;
		} else if (!is_edr && (power > BLE_MAX_R2 || power < BLE_MIN_R2))
			return ERR_PWR;
	} else if (resolution == RES_DOT_5) {
		/* XX.YY => XX.YY/0.5 = XX*2 + YY/5 */
		if (strstr(str, ".")) {
			ptr = strsep(&str, ".");
			if (ptr == NULL)
				return -1;
			if (kstrtoint(ptr, 0, &integer) != 0) {
				BTMTK_ERR("Read integer Fail");
				return -1;
			}

			if (kstrtoint(str, 0, &decimal) != 0) {
				BTMTK_ERR("Read decimal Fail");
				return -1;
			}

			if (decimal != 5)
				return ERR_PWR;
			if (integer >= 0)
				power = integer * 2 + decimal / 5;
			if (integer < 0)
				power = integer * 2 - decimal / 5;
		} else {
			if (kstrtoint(str, 0, &integer) != 0) {
				BTMTK_ERR("Read integer Fail");
				return -1;
			}
			power = integer * 2;
		}

		BTMTK_DBG("%s: power = %d", __func__, power);

		if (is_edr) {
			if (power > EDR_MAX_R1 || power < EDR_MIN_R1)
				return ERR_PWR;
			if (power >= EDR_MIN_LV9_R1)
				main_info.PWS.LV9 = 1;
		} else if (!is_edr && (power > BLE_MAX_R1 || power < BLE_MIN_R1))
			return ERR_PWR;
	} else if (resolution == RES_1) {
		if (kstrtoint(str, 0, &power) != 0) {
			BTMTK_ERR("Read power Fail");
			return -1;
		}

		BTMTK_DBG("%s: power = %d", __func__, power);

		if (is_edr) {
			if (power > EDR_MAX || power < EDR_MIN)
				return ERR_PWR;
			if (power >= EDR_MIN_LV9)
				main_info.PWS.LV9 = 1;
		} else if (!is_edr && (power > BLE_MAX || power < BLE_MIN))
			return ERR_PWR;
	}

	return power;
}

static int btmtk_check_power_resolution(char *str)
{
	if (str == NULL)
		return -1;
	if (strstr(str, ".25") || strstr(str, ".75"))
		return RES_DOT_25;
	if (strstr(str, ".5"))
		return RES_DOT_5;
	if (!strstr(str, ".") || strstr(str, ".0"))
		return RES_1;
	return -1;
}

static void btmtk_init_power_setting_struct(void)
{
	main_info.PWS.BLE_1M = 0;
	main_info.PWS.EDR_Max = 0;
	main_info.PWS.LV9 = 0;
	main_info.PWS.DM = 0;
	main_info.PWS.IR = 0;
	main_info.PWS.BLE_2M = 0;
	main_info.PWS.BLE_LR_S2 = 0;
	main_info.PWS.BLE_LR_S8 = 0;
}

static int btmtk_parse_power_table(char *context)
{
	char *ptr = NULL;
	int step = 0, temp;
	int resolution;
	int power;

	if (context == NULL) {
		BTMTK_ERR("%s context is NULL", __func__);
		return -1;
	}

	BTMTK_INFO("%s", __func__);
	btmtk_init_power_setting_struct();

	/* Send to BT0? BT1? */
	if (strstr(context, "BT0")) {
		BTMTK_INFO("Parse power for BT0");
		main_info.PWS.IR |= 0x10;
		context += strlen("[BT0]");
	} else if (strstr(context, "BT1")) {
		BTMTK_INFO("Parse power for BT1");
		main_info.PWS.IR |= 0x20;
		context += strlen("[BT1]");
	} else {
		BTMTK_ERR("%s BT indicator error", __func__);
		return -1;
	}

	resolution = btmtk_check_power_resolution(context);
	if (resolution == -1) {
		BTMTK_ERR("Check resolution fail");
		return -1;
	}

	main_info.PWS.IR |= resolution;
	BTMTK_INFO("%s: resolution = %d", __func__, resolution);

	while ((ptr = strsep(&context, ",")) != NULL) {
		while (*ptr == '\t' || *ptr == ' ')
			ptr++;

		switch (step) {
		/* BR_EDR_PWR_MODE */
		case CHECK_SINGLE_SKU_PWR_MODE:
			if (kstrtoint(ptr, 0, &temp) == 0) {
				if (temp == 0 || temp == 1) {
					main_info.PWS.DM = temp;
					step++;
					continue;
				} else {
					BTMTK_ERR("PWR MODE value wrong");
					return -1;
				}
			} else {
				BTMTK_ERR("Read PWR MODE Fail");
				return -1;
			}
			break;
		/* Parse EDR MAX */
		case CHECK_SINGLE_SKU_EDR_MAX:
			power = btmtk_set_power_value(ptr, resolution, 1);
			if (power == ERR_PWR) {
				BTMTK_ERR("EDR MAX value wrong");
				return -1;
			}
			main_info.PWS.EDR_Max = power;
			step++;
			break;
		/* Parse BLE Default */
		case CHECK_SINGLE_SKU_BLE:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTMTK_ERR("BLE value wrong");
				return -1;
			}
			main_info.PWS.BLE_1M = power;
			step++;
			break;
		/* Parse BLE 2M */
		case CHECK_SINGLE_SKU_BLE_2M:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTMTK_ERR("BLE 2M value wrong");
				return -1;
			}
			main_info.PWS.BLE_2M = power;
			step++;
			break;
		/* Parse BLE long range S2 */
		case CHECK_SINGLE_SKU_BLE_LR_S2:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTMTK_ERR("BLE LR S2 value wrong");
				return -1;
			}
			main_info.PWS.BLE_LR_S2 = power;
			step++;
			break;
		/* Parse BLE long range S8 */
		case CHECK_SINGLE_SKU_BLE_LR_S8:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTMTK_ERR("BLE LR S8 value wrong");
				return -1;
			}
			main_info.PWS.BLE_LR_S8 = power;
			step++;
			break;
		default:
			BTMTK_ERR("%s step is wrong: %d", __func__, step);
			break;
		}
		continue;
	}

	return step;
}

static void btmtk_send_txpower_cmd_to_all_interface(void)
{
	int i, ret;
	struct btmtk_dev *bdev = NULL;

	for (i = 0; i < btmtk_intf_num; i++) {
		if (g_bdev[i]->hdev != NULL) {
			bdev = g_bdev[i];
			BTMTK_INFO("send to %d", i);
			ret = btmtk_send_txpower_cmd(bdev);
			if (ret < 0)
				BTMTK_ERR("Device %d send txpower cmd fail", i);
		}
	}
}

static void btmtk_requset_country_cb(const struct firmware *fw, void *context)
{
	char *ptr, *data, *p_data = NULL;
	char *country = NULL;
	int ret = 0;
	bool find_country = false;
	bool read_next = false;

	if (fw == NULL) {
		BTMTK_ERR("fw is NULL");
		return;
	}

	BTMTK_INFO("%s request %s success", __func__, DEFAULT_COUNTRY_TABLE_NAME);
	p_data = data = kzalloc(fw->size, GFP_KERNEL);
	if (data == NULL) {
		BTMTK_WARN("%s allocate memory fail (data)", __func__);
		goto exit;
	}

	memcpy(data, fw->data, fw->size);
	while ((ptr = strsep(&p_data, "\n")) != NULL) {
		/* If the '#' in front of the line, ignore this line */
		if (*ptr == '#')
			continue;

		/* Set power for BT1 */
		if (read_next) {
			if (strncmp(ptr, "[BT1]", 5) == 0) {
				ret = btmtk_parse_power_table(ptr);
				if (ret != CHECK_SINGLE_SKU_ALL) {
					BTMTK_ERR("Parse power fail, ret = %d", ret);
					break;
				}

				btmtk_send_txpower_cmd_to_all_interface();
			} else {
				BTMTK_INFO("No power data for BT1");
			}
			break;
		}

		if (find_country) {
			ret = btmtk_parse_power_table(ptr);
			/* Check if the next line has power value for BT1 */
			read_next = true;
			if (ret != CHECK_SINGLE_SKU_ALL) {
				BTMTK_ERR("Parse power fail, ret = %d", ret);
				continue;
			}

			btmtk_send_txpower_cmd_to_all_interface();
			continue;
		}

		while ((country = strsep(&ptr, ",[]")) != NULL) {
			if (strlen(country) != COUNTRY_CODE_LEN)
				continue;
			if (strcmp(country, main_info.PWS.country_code) == 0) {
				find_country = true;
				break;
			}
		}
	}
	kfree(data);

	if (find_country == false)
		BTMTK_ERR("Can't find country in the table");

exit:
	release_firmware(fw);
}

static int btmtk_load_country_table(struct btmtk_dev *bdev)
{
	int err = 0;

	if (bdev->country_file_name)
		err = request_firmware_nowait(THIS_MODULE, true,
			bdev->country_file_name, NULL, GFP_KERNEL, NULL,
			btmtk_requset_country_cb);
	else
		BTMTK_WARN("%s country_file_name is null", __func__);

	return err;
}

void btmtk_set_country_code_from_wifi(char *code)
{
	int i;
	struct btmtk_dev *bdev = NULL;

	if (!code)
		return;

	if (strlen(code) == COUNTRY_CODE_LEN) {
		BTMTK_INFO("%s country code is %s", __func__, code);
		memcpy(main_info.PWS.country_code, code, sizeof(main_info.PWS.country_code));
		for (i = 0; i < btmtk_intf_num; i++) {
			if (g_bdev[i]->hdev != NULL) {
				bdev = g_bdev[i];
				if (bdev->bt_cfg.support_bt_single_sku) {
					btmtk_load_country_table(bdev);
					break;
				}
			}
		}
	} else {
		BTMTK_INFO("%s country code is not valid", __func__);
	}
}
/* Pin-Hao: remove export symbol to build 2 same BT driver for SLT */
#if (USE_DEVICE_NODE == 0)
EXPORT_SYMBOL_GPL(btmtk_set_country_code_from_wifi);
#endif

/**
 * Kernel HCI Interface Registeration
 */
static int bt_flush(struct hci_dev *hdev)
{
	if (main_info.hif_hook_chip.bt_flush_handler)
		return main_info.hif_hook_chip.bt_flush_handler(hdev);
	else
		return 0;
}

#if (USE_DEVICE_NODE == 0)
static int bt_close(struct hci_dev *hdev)
#else
int bt_close(struct hci_dev *hdev)
#endif
{
	int ret = -1;
	int state = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;
#if BTMTK_RUNTIME_ENABLE
	struct btmtk_usb_dev *cif_dev = NULL;
#endif

	if (!hdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return ret;
	}

	bdev = hci_get_drvdata(hdev);
	if (!bdev) {
		BTMTK_ERR("%s: bdev is invalid!", __func__);
		return ret;
	}

#if BTMTK_RUNTIME_ENABLE
	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_INFO("%s: autopm get interface fail.", __func__);
	}
#endif

	if(bdev->is_whole_chip_reset) {
		BTMTK_WARN("%s: is_whole_chip_reset, fstate(%d)", __func__, fstate);
		if (main_info.hif_hook.cif_mutex_lock)
			main_info.hif_hook.cif_mutex_lock(bdev);
		goto unlock;
	}

	fstate = btmtk_fops_get_state(bdev);
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not allow close(%d)", __func__, fstate);
		goto err;
	}
	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_CLOSING);

	state = btmtk_get_chip_state(bdev);

#if (USE_DEVICE_NODE == 1)
	if (state == BTMTK_STATE_FW_DUMP || state == BTMTK_STATE_SEND_ASSERT
			|| state == BTMTK_STATE_SUBSYS_RESET) {
		if (main_info.hif_hook.cif_mutex_lock)
			main_info.hif_hook.cif_mutex_lock(bdev);
		goto unlock;
	}
#endif

	if (state != BTMTK_STATE_WORKING && state != BTMTK_STATE_STANDBY) {
		/* If hif disconnect occurs,
		 * it will call cif_mutex_lock and release hci device.
		 * Release hci device will call bt_close.
		 * It must return with this,
		 * otherwise the below cif_mutex_lock will cause deadlock
		 */

		BTMTK_WARN("%s: not in working state and standby state(%d).", __func__, state);
		goto exit;
	}

	if (main_info.hif_hook.cif_mutex_lock)
		main_info.hif_hook.cif_mutex_lock(bdev);

	state = btmtk_get_chip_state(bdev);
	BTMTK_INFO("%s, enter, state[%d]", __func__, state);

	if (state != BTMTK_STATE_WORKING && state != BTMTK_STATE_STANDBY) {
		/* It's for the case that
		 * bt_close and hif_disconnect occur at the same time
		 */
		BTMTK_WARN("%s: not in working state and standby state(%d).", __func__, state);
		goto unlock;
	}

#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
	/* Don't send init cmd for DVT
	 * Such as Lowpower DVT
	 */
	bdev->power_state = BTMTK_DONGLE_STATE_POWER_OFF;
	BTMTK_INFO("%s, SKIP btmtk_send_deinit_cmds", __func__);
#else
	if (state != BTMTK_STATE_STANDBY && main_info.reset_stack_flag != HW_ERR_CODE_CORE_DUMP
		&& main_info.reset_stack_flag != HW_ERR_CODE_CHIP_RESET) {
		ret = btmtk_send_deinit_cmds(bdev);
		if (ret < 0) {
			BTMTK_ERR("%s, btmtk_send_deinit_cmds failed", __func__);
			goto unlock;
		}
	} else
		BTMTK_WARN("%s, SKIP by state[%d], reset_stack_flag[%d]", __func__, state, main_info.reset_stack_flag);
#endif /* CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT */

	/* Flush RX works */
	flush_work(&bdev->rx_work);
	flush_work(&bdev->dynamic_fwdl_work);

unlock:
	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_CLOSED)
		BTMTK_WARN("%s: fops is already close", __func__);
	else
		main_info.hif_hook.close(hdev);

	if (main_info.hif_hook.cif_mutex_unlock)
		main_info.hif_hook.cif_mutex_unlock(bdev);
exit:
#if (USE_DEVICE_NODE == 1)
	/* avoid reset start at new bt on */
	btmtk_reset_timer_del(bdev);
	state = btmtk_get_chip_state(bdev);

	if (state != BTMTK_STATE_DISCONNECT)
		btmtk_set_chip_state(bdev, BTMTK_STATE_CLOSED);

#endif
	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_CLOSED);

err:
	main_info.reset_stack_flag = HW_ERR_NONE;
	bdev->get_hci_reset = 0;

#if CFG_SUPPORT_LEAUDIO_CLK
	btmtk_le_audio_clk_deinit(bdev);
#endif

	/* clean rx queues */
	skb_queue_purge(&bdev->rx_q);
	if (!IS_ERR_OR_NULL(bdev->rx_skb))
		kfree_skb(bdev->rx_skb);
	bdev->rx_skb = NULL;

#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	BTMTK_INFO("%s end state[%d], reset_stack_flag[%d]", __func__, state, main_info.reset_stack_flag);
	return 0;
}

#if (USE_DEVICE_NODE == 0)
static int bt_open(struct hci_dev *hdev)
#else
int bt_open(struct hci_dev *hdev)
#endif
{
	int ret = 0;
	int state = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;
#if (CFG_GKI_SUPPORT == 0)
	void (*rlm_get_alpha2)(char *);
	const char *wifi_func_name = "rlm_get_alpha2";
	char alpha2[5];
#endif
#if BTMTK_RUNTIME_ENABLE
	struct btmtk_usb_dev *cif_dev = NULL;
#endif

	BTMTK_INFO("%s: MTK BT Driver Version : %s", __func__, VERSION);
	DUMP_TIME_STAMP("open_start");

	if (!hdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return -EFAULT;
	}

	bdev = hci_get_drvdata(hdev);
	if (!bdev) {
		BTMTK_ERR("%s: bdev is invalid", __func__);
		return -EFAULT;
	}

#if BTMTK_RUNTIME_ENABLE
	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;

	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_INFO("%s: usb get interface fail", __func__);
	}
#endif

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops opened!", __func__);
		return -EIO;
	}

	if ((fstate == BTMTK_FOPS_STATE_CLOSING) ||
		(fstate == BTMTK_FOPS_STATE_OPENING)) {
		BTMTK_WARN("%s: fops open/close is on-going !", __func__);
		return -EAGAIN;
	}

	/* mutex with bt close and disconnect */
	if (main_info.hif_hook.cif_mutex_lock)
		main_info.hif_hook.cif_mutex_lock(bdev);

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops opened!", __func__);
		ret = -EIO;
		goto unlock;
	}

	if ((fstate == BTMTK_FOPS_STATE_CLOSING) ||
		(fstate == BTMTK_FOPS_STATE_OPENING)) {
		BTMTK_WARN("%s: fops open/close is on-going !", __func__);
		ret = -EAGAIN;
		goto unlock;
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_INIT || state == BTMTK_STATE_DISCONNECT
#if (USE_DEVICE_NODE == 0)
		|| state == BTMTK_STATE_PROBE) {
#else
	) {
#endif
		BTMTK_WARN("%s: chip_state[%d] is init or disconnect!", __func__, state);
		ret = -EAGAIN;
		goto unlock;
	}

	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_OPENING);

	if (main_info.hif_hook.pre_open) {
		ret = main_info.hif_hook.pre_open(bdev);
		if (ret < 0) {
			BTMTK_ERR("%s: pre_open fail", __func__);
			goto failed;
		}
	}

	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_WORKING && state != BTMTK_STATE_STANDBY) {
		BTMTK_WARN("%s: not in working state and standby state(%d).", __func__, state);
		ret = -ENODEV;
		goto failed;
	}

	BTMTK_DBG("%s state[%d], fstate[%d]", __func__, state, fstate);

	ret = main_info.hif_hook.open(hdev);
	if (ret < 0 && ret != -EBUSY) {
		BTMTK_ERR("%s, cif_open failed", __func__);
		goto failed;
	}

#if (USE_DEVICE_NODE == 0)
	if (main_info.hif_hook_chip.bt_open_handler) {
		ret = main_info.hif_hook_chip.bt_open_handler(bdev);
		if (ret < 0) {
			BTMTK_ERR("bt_open_handler fail");
			goto failed;
		}
	}
#endif

#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
	/* Don't send init cmd for DVT
	 * Such as Lowpower DVT
	 */
	bdev->power_state = BTMTK_DONGLE_STATE_POWER_ON;
	BTMTK_INFO("%s, SKIP btmtk_send_init_cmds", __func__);
	/* We don't need to enable buffer mode during bring-up stage. */
	BTMTK_INFO("SKIP buffer mode");
#else
	ret = btmtk_send_init_cmds(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s, btmtk_send_init_cmds failed", __func__);
		goto failed;
	}

#if (USE_DEVICE_NODE == 0)
	ret = btmtk_send_apcf_reserved(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s, btmtk_send_apcf_reserved failed", __func__);
		goto failed;
	}

	(void)btmtk_buffer_mode_send(bdev);
#endif
#endif /* CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT */

	if (main_info.hif_hook.open_done)
		main_info.hif_hook.open_done(bdev);

	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_OPENED);
	main_info.reset_stack_flag = HW_ERR_NONE;
	bdev->on_fail_count = 0;

#if (CFG_GKI_SUPPORT == 0)
	if (bdev->bt_cfg.support_bt_single_sku) {
		rlm_get_alpha2 = (void *)btmtk_kallsyms_lookup_name(wifi_func_name);

		if (rlm_get_alpha2) {
			rlm_get_alpha2(alpha2);
			if (strlen(alpha2) == COUNTRY_CODE_LEN) {
				BTMTK_INFO("Wifi set country code %s", alpha2);
				memcpy(main_info.PWS.country_code, alpha2, sizeof(main_info.PWS.country_code));
			} else {
				BTMTK_ERR("Country code length is wrong");
			}
		} else {
			BTMTK_INFO("Wifi didn't set country code");
		}

		main_info.PWS.country_code[COUNTRY_CODE_LEN] = '\0';
		if (strcmp(main_info.PWS.country_code, "") != 0)
			btmtk_load_country_table(bdev);
	}
#endif

#if CFG_SUPPORT_LEAUDIO_CLK
	btmtk_le_audio_clk_init(bdev);
#endif
#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	DUMP_TIME_STAMP("open_end");
	ret = 0;
	goto unlock;

failed:
#if (USE_DEVICE_NODE == 1)
	if (ret != CONNV3_ERR_RST_ONGOING)
		main_info.hif_hook.close(hdev);
	else {
		BTMTK_WARN("%s: whole chip reset going, not do close when bt on fail", __func__);
		ret = -EAGAIN;
	}
	state = btmtk_get_chip_state(bdev);
	/* if state is disconnect means uart_launcher is disconnected, not set to close state */
	if (state != BTMTK_STATE_DISCONNECT)
		btmtk_set_chip_state(bdev, BTMTK_STATE_CLOSED);

	if (++bdev->on_fail_count > BTMTK_MAX_SUBSYS_RESET_COUNT && main_info.hif_hook.whole_reset) {
		memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
		strncpy(bdev->assert_reason, "[BT_DRV assert] on fail more than 3 times",
							 strlen("[BT_DRV assert] on fail more than 3 times") + 1);
		BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
		bdev->on_fail_count = 0;
		main_info.hif_hook.whole_reset(bdev);
	}
#endif

#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_CLOSED);
	main_info.reset_stack_flag = HW_ERR_NONE;

unlock:
	if (main_info.hif_hook.cif_mutex_unlock)
		main_info.hif_hook.cif_mutex_unlock(bdev);

	return ret;
}

static int bt_setup(struct hci_dev *hdev)
{
	int ret = 0;

	BTMTK_INFO("%s", __func__);

	if (main_info.hif_hook_chip.bt_setup_handler) {
		ret = main_info.hif_hook_chip.bt_setup_handler(hdev);
		if (ret)
			BTMTK_ERR("%s: fail", __func__);
		return ret;
	}
	return 0;
}

#if (USE_DEVICE_NODE == 0)
static int bt_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
#else
int bt_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
#endif
{
	int ret = 0;
	int state = 0;
	unsigned char fstate = 0;
	u8 fw_coredump_flag = 0, retry = 20;
#if BTMTK_RUNTIME_ENABLE
	struct btmtk_usb_dev *cif_dev = NULL;
#endif
	struct btmtk_dev *bdev = NULL;
	unsigned char *skb_tmp = NULL;

	if (hdev == NULL || skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -ENODEV;
	}

	bdev = hci_get_drvdata(hdev);
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is invalid!", __func__);
		return -ENODEV;
	}

#if BTMTK_RUNTIME_ENABLE
	cif_dev = (struct btmtk_usb_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_INFO("%s: cif_dev is NULL", __func__);
	}

	ret = usb_autopm_get_interface(cif_dev->intf);
	if (ret < 0) {
		BTMTK_INFO("%s: usb autopm get interface fail", __func__);
	}
#endif

	if (btmtk_data_length_check((u8 *)skb->data, skb->len + 1, hci_skb_pkt_type(skb))) {
		BTMTK_INFO("%s: length check fail,return", __func__);
		return -1;
	}

	if (main_info.hif_hook.cif_mutex_lock)
		main_info.hif_hook.cif_mutex_lock(bdev);

	fstate = btmtk_fops_get_state(bdev);
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		ret = -ENODEV;
		goto exit;
	}

	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_WORKING) {
		BTMTK_WARN_LIMITTED("%s: chip state is not working state[%d]", __func__, state);
		if (state == BTMTK_STATE_DISCONNECT)
			ret = -ENODEV;
		else
			ret = -EAGAIN;
		goto exit;
	}

	if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		BTMTK_WARN("%s: dongle state already power off, do not write", __func__);
		ret = -EFAULT;
		goto exit;
	}

	if (main_info.reset_stack_flag) {
		BTMTK_WARN("%s: reset_stack_flag (%d)!", __func__, main_info.reset_stack_flag);
		ret = -EFAULT;
		goto exit;
	}

	skb_tmp = skb_push(skb, 1);
	if (!skb_tmp) {
		BTMTK_ERR("%s, skb_put failed!", __func__);
		ret = -ENOMEM;
		goto exit;
	}
	memcpy(skb_tmp, &hci_skb_pkt_type(skb), 1);
#if ENABLESTP
	skb = btmtk_add_stp(bdev, skb);
#endif

	/* avoid host send data during re-download */
	while (bdev->dynamic_fwdl_start && retry--) {
		BTMTK_INFO("%s: re-download on-goning, wait(%d)", __func__, retry);
		msleep(50);
	}
	/* avoid dynamic_fwdl_start flag keep blocking every cmd if something wrong. */
	if (bdev->dynamic_fwdl_start && retry == 0) {
		BTMTK_INFO("%s: wait re-download more than 1s, reset dynamic_fwdl_start flag", __func__);
		bdev->dynamic_fwdl_start = false;
	}

	if (main_info.hif_hook_chip.bt_tx_frame_handler) {
		/* ret = 0, means data not handle in bt_tx_frame_handler, need to send fw
		 * ret = -1, means something wrong in bt_tx_frame_handler, may need kree_skb
		 * ret = 1, means data is already handled in bt_tx_frame_handler
		 */
		ret = main_info.hif_hook_chip.bt_tx_frame_handler(bdev, skb, &fw_coredump_flag);
		if (ret < 0) {
			BTMTK_INFO("%s bt_tx_frame_handler return failed, ret[%d]", __func__, ret);
			goto exit;
		}
		if (ret == 1) {
			ret = 0;
			BTMTK_INFO("%s bt_tx_frame_handler already handle data, skip", __func__);
			goto exit;
		}
	} else
		BTMTK_INFO("%s not register bt_tx_frame_handler", __func__);

	BTMTK_DBG_RAW(skb->data, skb->len, "%s, send, len = %d ", __func__, skb->len);
	ret = main_info.hif_hook.send_cmd(bdev, skb, 0, 0, (int)BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		goto exit;
	}

exit:
	if (main_info.hif_hook.cif_mutex_unlock)
		main_info.hif_hook.cif_mutex_unlock(bdev);

	if (ret >= 0 && fw_coredump_flag == 1)
		btmtk_set_chip_state(bdev, BTMTK_STATE_SEND_ASSERT);

#if BTMTK_RUNTIME_ENABLE
	usb_autopm_put_interface(cif_dev->intf);
#endif

	return ret;
}

void btmtk_reg_hif_hook(struct hif_hook_ptr *hook)
{
	memcpy(&main_info.hif_hook, hook, sizeof(struct hif_hook_ptr));
}

int btmtk_data_length_check(u8 *data, u16 length, u8 type)
{
	int data_len = 0;

	if (type == HCI_COMMAND_PKT) {
		data_len = data[2] + MTK_HCI_CMD_HEAD_SIZE;
		if (data_len != length) {
			BTMTK_ERR("%s CMD length check fail, recv len = 0x%04x, data len = 0x%04x", __func__, length, data_len);
			BTMTK_INFO_RAW(data, length, "%s: - ", __func__);
			return -1;
		}
	} else if (type == HCI_ACLDATA_PKT) {
		data_len = data[2] + ((data[3] << 8) & 0xff00) + MTK_HCI_ACL_HEAD_SIZE;
		if (data_len != length) {
			BTMTK_ERR("%s ACL length check fail, recv len = 0x%04x, data len = 0x%04x", __func__, length, data_len);
			BTMTK_INFO_RAW(data, length, "%s: - ", __func__);
			return -1;
		}
	} else if (type == HCI_SCODATA_PKT) {
#if CFG_SUPPORT_DVT || CFG_SUPPORT_HW_DVT
		/* The SCO loopback test need to ignore the pkt length check */
#else
		data_len = data[2] + MTK_HCI_SCO_HEAD_SIZE;
		if (data_len != length) {
			BTMTK_ERR("%s SCO length check fail, recv len = 0x%04x, data len = 0x%04x", __func__, length, data_len);
			BTMTK_INFO_RAW(data, length, "%s: - ", __func__);
			return -1;
		}
#endif
	} else if (type == HCI_EVENT_PKT) {
		data_len = data[1] + MTK_HCI_EVENT_HEAD_SIZE;
		if (data_len != length) {
			BTMTK_ERR("%s EVENT length check fail, recv len = 0x%04x, data len = 0x%04x", __func__, length, data_len);
			BTMTK_INFO_RAW(data, length, "%s: - ", __func__);
			return -1;
		}
	} else if (type == HCI_ISO_PKT) {
		data_len = data[2] + (((data[3] & 0x3f) << 8) & 0x3f00) + HCI_ISO_PKT_HEADER_SIZE + 1;
		if (data_len != length) {
			BTMTK_ERR("%s ISO length check fail, recv len = 0x%04x, data len = 0x%04x", __func__, length, data_len);
			BTMTK_INFO_RAW(data, length, "%s: - ", __func__);
			return -1;
		}
	} else {
		BTMTK_ERR("%s invalid parameters, recv len = 0x%04x", __func__, length);
		BTMTK_INFO_RAW(data, length, "%s: - ", __func__);
		return -1;
	}
	return 0;
}

static void btmtk_dynamic_fwdl_work(struct work_struct *work)
{
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, dynamic_fwdl_work);

	BTMTK_INFO("%s enter", __func__);
	btmtk_dynamic_load_rom_patch(bdev, bdev->fw_bin_info);
	bdev->dynamic_fwdl_start = false;
}

static void btmtk_rx_work(struct work_struct *work)
{
	int err = 0;
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, rx_work);
	struct sk_buff *skb;
	unsigned char fstate = 0;
	unsigned char state = 0;

	while ((skb = skb_dequeue(&bdev->rx_q))) {
		if (main_info.hif_hook_chip.dispatch_fwlog) {
			err = main_info.hif_hook_chip.dispatch_fwlog(bdev, skb);
			if (err != 0) {
				/* kfree_skb should be moved to btmtk_dispach_pkt */
				kfree_skb(skb);
				continue;
			}
#if CFG_SUPPORT_LEAUDIO_CLK
			btmtk_le_audio_clk_evt_handler(bdev, skb);
#endif
		}
		BTMTK_DBG_RAW(skb->data, skb->len, "%s: len[%d] %02x", __func__,
							skb->len + 1, hci_skb_pkt_type(skb));
		if (hci_skb_pkt_type(skb) == HCI_EVENT_PKT) {
			/* save hci evt pkt for debug */
			if (skb->data[0] == 0x3E)
				btmtk_hci_snoop_save(HCI_SNOOP_TYPE_ADV_EVT_STACK, skb->data, skb->len);
			else if (skb->data[0] == 0x13)
				btmtk_hci_snoop_save(HCI_SNOOP_TYPE_NOCP_EVT_STACK, skb->data, skb->len);
			else
				btmtk_hci_snoop_save(HCI_SNOOP_TYPE_EVT_STACK, skb->data, skb->len);

			/* dynamic download for connac3 */
			if (skb->data[0] == 0x0E && skb->data[1] == 0x04 &&
					skb->data[2] == 0x01 && skb->data[3] == 0x01 &&
					skb->data[4] == 0xFE) {
				bdev->fw_bin_info = skb->data[5];
				/*
				 * Create a thread to do dynamic fwdl
				 * dynamic fwdl will block thread to wait for specific event,
				 * blocking rx_work thread means waited event won't be handled in
				 * rx_work thread, so here we create a new thread for dynamic fwdl.
				 */
				bdev->dynamic_fwdl_start = true;
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: Get dynamic DL EVENT- ", __func__);
				schedule_work(&bdev->dynamic_fwdl_work);

				/* Drop by driver, don't send to stack */
				kfree_skb(skb);
				skb = NULL;
				continue;
			}

			if (main_info.hif_hook.event_filter(bdev, skb)) {
				BTMTK_DBG("%s Drop by driver, don't send to stack", __func__);
				/* Drop by driver, don't send to stack */
				kfree_skb(skb);
				skb = NULL;
				continue;
			}
#if (USE_DEVICE_NODE == 1)
			/* intercept picus event and wmt utc sync event */
			if (skb->data[0] == 0x0E && skb->data[2] == 0x01
				&& skb->data[3] == 0x5D && skb->data[4] == 0xFC) {
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: intercept picus event and wmt utc sync event", __func__);

				/* Drop by driver, don't send to stack */
				kfree_skb(skb);
				skb = NULL;
				continue;
			}
#endif
		} else if (hci_skb_pkt_type(skb) == HCI_ACLDATA_PKT) {
			/* calibration backup */
			if (skb->data[0] == 0xff && skb->data[1] <= 0x03) {
				if (main_info.hif_hook_chip.backup_save) {
					BTMTK_INFO_RAW(skb->data, skb->len, "%s: Get calibration DATA- ", __func__);
					main_info.hif_hook_chip.backup_save(bdev, skb);
				}
				continue;
			}

			/* save hci acl pkt for debug, not include picus log and coredump*/
			if (!(skb->data[0] == 0xFF && skb->data[1] == 0xF0))
				btmtk_hci_snoop_save(HCI_SNOOP_TYPE_RX_ACL_STACK, skb->data, skb->len);

		} else if (hci_skb_pkt_type(skb) == HCI_ISO_PKT) {
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_RX_ISO_STACK, skb->data, skb->len);

#if (USE_DEVICE_NODE == 1)
		} else if (hci_skb_pkt_type(skb) == RHW_WRITE_TYPE
					|| hci_skb_pkt_type(skb) == RHW_READ_TYPE) {
			main_info.hif_hook.event_filter(bdev, skb);
			BTMTK_DBG("%s Drop by driver, don't send to stack", __func__);
			/* Drop by driver, don't send to stack */
			kfree_skb(skb);
			skb = NULL;
			continue;
		} else if (hci_skb_pkt_type(skb) == MTK_HCI_SCODATA_PKT) {
			/* for VTS SOC data loopback test workaround
			 * Uarthub and BT uart SOC format not sync
			 *
			 * rearrange: 03 AA LL+2 LL+1 BB PP PP PP
			 * original:  03 AA BB   LL 	  PP PP PP
			 */
			u8 original_soc_hd[] = { 0x00, 0x00, 0x00 };
			original_soc_hd[0] = skb->data[0];
			original_soc_hd[1] = skb->data[3];
			original_soc_hd[2] = skb->data[1] - 2;

			BTMTK_INFO_RAW(skb->data, skb->len, "%s: VTS RX before rearrange[%d]: %02x",
					__func__, skb->len + 1, hci_skb_pkt_type(skb));

			/* remove 1 byte */
			skb_pull(skb, 1);
			memcpy(skb->data, original_soc_hd, sizeof(original_soc_hd));

			BTMTK_INFO_RAW(skb->data, skb->len, "%s: VTS RX after rearrange[%d]: %02x",
					__func__, skb->len + 1, hci_skb_pkt_type(skb));

#endif
		}

		fstate = btmtk_fops_get_state(bdev);
		state = btmtk_get_chip_state(bdev);

#if (USE_DEVICE_NODE == 1)
		/* is_whole_chip_reset need to send hw err event after bt close */
		if (fstate != BTMTK_FOPS_STATE_OPENED && !bdev->is_whole_chip_reset && bdev->reset_type != CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
#else
		if (fstate != BTMTK_FOPS_STATE_OPENED && state != BTMTK_STATE_FW_DUMP) {
#endif
			/* BT close case, drop by driver, don't send to stack */
			BTMTK_WARN("%s Drop by driver, fops(%d) chip(%d) state is not opened", __func__, fstate, state);
			kfree_skb(skb);
			skb = NULL;
			continue;
		}

		if (main_info.hif_hook_chip.rx_handler) {
			err = main_info.hif_hook_chip.rx_handler(bdev, skb);
			if (err == CONTINUE_RET)
				continue;
		} else
			BTMTK_ERR("%s rx_handler is NULL!!!", __func__);

		if (err < 0) {
			if (err != -ENXIO)
				BTMTK_ERR("%s btmtk_rx_work failed, err = %d", __func__, err);
			continue;
		}
	}
}

void btmtk_free_hci_device(struct btmtk_dev *bdev, int hci_bus_type)
{
	unsigned char fstate = 0;

	if (!bdev)
		return;

	BTMTK_INFO("%s Begin", __func__);
	/* Flush RX works */
	flush_work(&bdev->rx_work);

	if (skb_queue_len(&bdev->rx_q) != 0) {
		/* Drop queues */
		skb_queue_purge(&bdev->rx_q);
	}
#if (USE_DEVICE_NODE == 0)
	if (bdev->workqueue) {
		destroy_workqueue(bdev->workqueue);
		bdev->workqueue = NULL;
	}

	if (bdev->hdev) {
		hci_free_dev(bdev->hdev);
		bdev->hdev = NULL;
	}
#endif

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_OPENED || fstate == BTMTK_FOPS_STATE_CLOSING) {
		BTMTK_WARN("%s: fstate = %d , set reset_stack_flag", __func__, fstate);
		if (main_info.reset_stack_flag == HW_ERR_NONE)
			main_info.reset_stack_flag = HW_ERR_CODE_USB_DISC;
	}

	bdev->get_hci_reset = 0;
	BTMTK_INFO("%s End", __func__);
}

int btmtk_allocate_hci_device(struct btmtk_dev *bdev, int hci_bus_type)
{
	struct hci_dev *hdev;
	int err = 0;

	if (!bdev) {
		BTMTK_ERR("%s, bdev is NULL!", __func__);
		err = -EINVAL;
		goto end;
	}

#if (USE_DEVICE_NODE == 1)
	/*
	 * delay free to here instead of unitialize for phone in case
	 * of uart_launcher crash or force terminate, and driver function
	 * is still running in
	 * hdev pointer
	 */
	if (bdev->hdev)
		hci_free_dev(bdev->hdev);
#endif

	BTMTK_INFO("%s", __func__);
	/* Add hci device */
	hdev = hci_alloc_dev();
	if (!hdev) {
		BTMTK_ERR("%s, hdev is NULL!", __func__);
		err = -ENOMEM;
		goto end;
	}

	hdev->bus = hci_bus_type;
	hci_set_drvdata(hdev, bdev);

#if (KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE)
	/* HCI_PRIMARY = 0x00 */
	hdev->dev_type = 0x00;
#endif

	bdev->hdev = hdev;

	/* register hci callback */
	hdev->open	   = bt_open;
	hdev->close    = bt_close;
	hdev->flush    = bt_flush;
	hdev->send	   = bt_send_frame;
	hdev->setup    = bt_setup;

	init_waitqueue_head(&bdev->p_wait_event_q);

	/* rx_work init */
	INIT_WORK(&bdev->rx_work, btmtk_rx_work);
	INIT_WORK(&bdev->dynamic_fwdl_work, btmtk_dynamic_fwdl_work);
#if (USE_DEVICE_NODE == 1)
	INIT_WORK(&bdev->async_trx_work, btmtk_async_trx_work);
	INIT_WORK(&bdev->pwr_on_uds_work, btmtk_pwr_on_uds_work);
#endif
	skb_queue_head_init(&bdev->rx_q);
	if (bdev->workqueue == NULL) {
		bdev->workqueue = alloc_workqueue("BTMTK_RX_WQ", WQ_HIGHPRI | WQ_UNBOUND |
							WQ_MEM_RECLAIM, 1);
	} else {
		BTMTK_INFO("%s: bdev->workqueue exist", __func__);
	}

	if (!bdev->workqueue) {
		BTMTK_ERR("%s, bdev->workqueue is NULL!", __func__);
		err = -ENOMEM;
		goto err0;
	}

	bdev->get_hci_reset = 0;
	bdev->on_fail_count = 0;

	BTMTK_DBG("%s done", __func__);
	return 0;

err0:
	hci_free_dev(hdev);
	hdev = NULL;
end:
	return err;
}

int btmtk_register_hci_device(struct btmtk_dev *bdev)
{
	struct hci_dev *hdev;
	int err = 0;
	int ret = 0;

	hdev = bdev->hdev;

#if (KERNEL_VERSION(5, 10, 20) < LINUX_VERSION_CODE)
	set_bit(HCI_QUIRK_NO_SUSPEND_NOTIFIER, &hdev->quirks);
#endif

	err = hci_register_dev(hdev);
	/* After hci_register_dev completed
	 * It will set dev_flags to HCI_SETUP
	 * That cause vendor_lib create socket failed
	 */
	if (err < 0) {
		BTMTK_INFO("%s can't register", __func__);
		goto exit;
	}

#if CFG_SUPPORT_BLUEZ
	UNUSED(ret);
#else
	/* why need to clear flag HCI_SETUP? */
	/* reason: if don't clearflag HCI_SETUP, bluedroid do open will return at
	 * the case HCI_CHANNEL_USER of hci_sock_bind API, because hci_dev_test_flag(hdev, HCI_SETUP)
	 * is true, it will goto done, just skip the hci_dev_open
	 */

#if (KERNEL_VERSION(4, 4, 0) > LINUX_VERSION_CODE)
	ret = test_and_clear_bit(HCI_SETUP, &hdev->dev_flags);
#else
	ret = hci_dev_test_and_clear_flag(hdev, HCI_SETUP);
#endif
	BTMTK_INFO("%s, the bit value returned is %d", __func__, ret);
#endif /* CFG_SUPPORT_BLUEZ */

exit:
	return err;
}

int btmtk_deregister_hci_device(struct btmtk_dev *bdev)
{
	int err = 0;

	/* when not do hci_register_dev action, we do hci_unregister_dev will crash,
	 * so we add test_flag to decide whether hci_register_dev has been
	 * successful or failed, if  hci_register_dev success, it will set flag to
	 * HCI_BREDR_ENABLED, After this flag has been set to HCI_BREDR_ENABLED, we
	 * can be able to do hci_unregister_dev.
	 */
#if (KERNEL_VERSION(4, 1, 0) > LINUX_VERSION_CODE)
	if (bdev && bdev->hdev && test_bit(HCI_BREDR_ENABLED, &bdev->hdev->dev_flags)) {
#else
	if (bdev && bdev->hdev && hci_dev_test_flag(bdev->hdev, HCI_BREDR_ENABLED)) {
#endif
		hci_unregister_dev(bdev->hdev);
		BTMTK_INFO("%s end", __func__);
	}

	return err;
}

static int btmtk_main_allocate_memory(struct btmtk_dev *bdev)
{
	int err = -1;

	BTMTK_INFO("%s Begin", __func__);

	if (bdev->rom_patch_bin_file_name == NULL) {
		bdev->rom_patch_bin_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!bdev->rom_patch_bin_file_name) {
			BTMTK_ERR("%s: alloc memory fail (bdev->rom_patch_bin_file_name)", __func__);
			goto end;
		}
	}

	if (bdev->io_buf == NULL) {
		bdev->io_buf = kzalloc(IO_BUF_SIZE, GFP_KERNEL);
		if (!bdev->io_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->io_buf)", __func__);
			goto err2;
		}
	}

	if (bdev->bt_cfg_file_name == NULL) {
		bdev->bt_cfg_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!bdev->bt_cfg_file_name) {
			BTMTK_ERR("%s: alloc memory fail (bdev->bt_cfg_file_name)", __func__);
			goto err1;
		}
	}

	if (bdev->country_file_name == NULL) {
		bdev->country_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!bdev->country_file_name) {
			BTMTK_ERR("%s: alloc memory fail (bdev->country_file_name)", __func__);
			goto err0;
		}
	}
	BTMTK_DBG("%s Done", __func__);
	return 0;

err0:
	kfree(bdev->bt_cfg_file_name);
	bdev->bt_cfg_file_name = NULL;
err1:
	kfree(bdev->io_buf);
	bdev->io_buf = NULL;
err2:
	kfree(bdev->rom_patch_bin_file_name);
	bdev->rom_patch_bin_file_name = NULL;
end:
	return err;
}

static void btmtk_main_free_memory(struct btmtk_dev *bdev)
{
	kfree(bdev->rom_patch_bin_file_name);
	bdev->rom_patch_bin_file_name = NULL;

	kfree(bdev->bt_cfg_file_name);
	bdev->bt_cfg_file_name = NULL;

	kfree(bdev->country_file_name);
	bdev->country_file_name = NULL;

	kfree(bdev->io_buf);
	bdev->io_buf = NULL;

	BTMTK_INFO("%s: Success", __func__);
}

int btmtk_main_cif_initialize(struct btmtk_dev *bdev, int hci_bus)
{
	int err = 0;

#if (USE_DEVICE_NODE == 0)
	btmtk_init_node();
	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_INIT);
#endif
	btmtk_reset_timer_add(bdev);

	err = btmtk_main_allocate_memory(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk_main_allocate_memory failed!");
		goto end;
	}

	btmtk_initialize_cfg_items(bdev);
	btmtk_init_memory(bdev);

	err = btmtk_allocate_hci_device(bdev, hci_bus);
	if (err < 0) {
		BTMTK_ERR("btmtk_allocate_hci_device failed!");
		goto free_mem;
	}

	err = btmtk_cap_init(bdev);
	if (err < 0) {
		if (err == -EIO) {
			BTMTK_ERR("btmtk_cap_init failed, do chip reset!");
			goto end;
		} else {
			BTMTK_ERR("btmtk_cap_init failed!");
			goto free_hci_dev;
		}
	}

#if (USE_DEVICE_NODE == 0)
	btmtk_load_bt_cfg(bdev->bt_cfg_file_name, bdev->intf_dev, bdev);

	(void)snprintf(bdev->country_file_name, MAX_BIN_FILE_NAME_LEN,
			DEFAULT_COUNTRY_TABLE_NAME);

	btmtk_buffer_mode_initialize(bdev);
#endif

#ifdef BTMTK_DEBUG_SOP
#ifdef DEFAULT_DEBUG_SOP_NAME
	/* debug sop */
	(void)snprintf(bdev->debug_sop_file_name, MAX_BIN_FILE_NAME_LEN,
		"%s_%x.bin", DEFAULT_DEBUG_SOP_NAME, bdev->chip_id & 0xffff);
	BTMTK_INFO("%s: debug sop file name is %s", __func__,
		bdev->debug_sop_file_name);

#if (USE_DEVICE_NODE == 0)
	btmtk_load_debug_sop_register(bdev->debug_sop_file_name, bdev->intf_dev, bdev);
#endif
#endif
#endif

	return 0;

free_hci_dev:
	btmtk_free_hci_device(bdev, hci_bus);
free_mem:
	btmtk_main_free_memory(bdev);
end:
	return err;
}

void btmtk_main_cif_uninitialize(struct btmtk_dev *bdev, int hci_bus)
{
	BTMTK_DBG("%s start", __func__);
	btmtk_init_memory(bdev);
	btmtk_free_setting_file(bdev);
	btmtk_free_hci_device(bdev, hci_bus);
	btmtk_main_free_memory(bdev);
	btmtk_reset_timer_del(bdev);
#ifdef BTMTK_DEBUG_SOP
	btmtk_clean_debug_reg_file(bdev);
#endif
	btmtk_cif_chip_deregister(bdev);
	if (main_info.hif_hook_chip.backup_free)
		main_info.hif_hook_chip.backup_free(bdev);
}

#ifdef CFG_SUPPORT_CHIP_RESET_KO
void btmtk_resetko_notify(unsigned int event, void *data)
{
	BTMTK_DBG("%s: event: %d", __func__, event);
	if (event == MODULE_NOTIFY_PRE_RESET)
		send_reset_event(RESET_MODULE_TYPE_BT, RFSM_EVENT_L0_RESET_READY);
}

void btmtk_resetko_reset(void)
{
	int i;
	struct btmtk_dev *bdev = NULL;

	for (i = 0; i < btmtk_intf_num; i++) {
		if (g_bdev[i]->hdev != NULL) {
			bdev = g_bdev[i];
			BTMTK_INFO("send to %d", i);
			main_info.hif_hook.whole_reset(bdev);
		}
	}
}
#endif

int btmtk_main_cif_disconnect_notify(struct btmtk_dev *bdev, int hci_bus)
{
	/* need to rewirte when add txqueue, because usb need to add more clear action
	 * when do whole chip reset, usb need to do clear action in usb_close when disconnect,
	 * because usb_close will not execute when do chip reset
	 */
	BTMTK_DBG("%s: start", __func__);
	cancel_work_sync(&bdev->rx_work);
	cancel_work_sync(&bdev->dynamic_fwdl_work);
#if (USE_DEVICE_NODE == 0)
	btmtk_deregister_hci_device(bdev);
#endif
	btmtk_main_cif_uninitialize(bdev, hci_bus);

#if (USE_DEVICE_NODE == 1)
	if (main_info.hif_hook.coredump_handler) {
		BTMTK_INFO("%s: deinit coredump handle", __func__);
		connv3_coredump_deinit(main_info.hif_hook.coredump_handler);
	}
#endif

	bdev->power_state = BTMTK_DONGLE_STATE_POWER_OFF;
	/* btmtk_release_dev(bdev); */

	return 0;
}

static int btmtk_reboot_notify(struct notifier_block *nb,
			unsigned long event, void *unused)
{
	int ret = 0;
	int i = 0;
	int cif_event = 0;
	unsigned char fstate = 0;
	int state = 0;
	struct btmtk_cif_state *cif_state = NULL;
	struct btmtk_dev *bdev = NULL;

	BTMTK_INFO("%s: btmtk_reboot_notify(%d)", __func__, (int)event);

	if (event == SYS_POWER_OFF && main_info.hif_hook.enter_standby != NULL) {
		BTMTK_DBG("%s: set woble for standby", __func__);
		main_info.hif_hook.enter_standby();
	}

	if (event == SYS_RESTART) {
		BTMTK_INFO("%s: enter", __func__);
		for (i = 0; i < btmtk_intf_num; i++) {
			/* Find valid dev for already probe interface. */
			if (g_bdev[i]->hdev != NULL) {
				bdev = g_bdev[i];

				fstate = btmtk_fops_get_state(bdev);
				if (fstate != BTMTK_FOPS_STATE_OPENED) {
					BTMTK_WARN("%s: fops is not opened(%d)", __func__, fstate);
					continue;
				}

				state = btmtk_get_chip_state(bdev);
				if (state != BTMTK_STATE_WORKING) {
					BTMTK_WARN("%s: not in working(%d).", __func__, state);
					continue;
				}

				cif_event = HIF_EVENT_DISCONNECT;
				if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
					/* Error */
					BTMTK_WARN("%s parameter is NULL", __func__);
					continue;
				}

				cif_state = &bdev->cif_state[cif_event];
				/* Set Entering state */
				btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

				btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_CLOSING);

				if (main_info.hif_hook.cif_mutex_lock)
					main_info.hif_hook.cif_mutex_lock(bdev);

				ret = btmtk_send_deinit_cmds(bdev);
				if (ret < 0)
					BTMTK_ERR("%s, btmtk_send_deinit_cmds failed", __func__);

				main_info.hif_hook.close(bdev->hdev);

				if (main_info.hif_hook.cif_mutex_unlock)
					main_info.hif_hook.cif_mutex_unlock(bdev);

				btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_CLOSED);

				/* Set End/Error state */
				if (ret == 0)
					btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
				else
					btmtk_set_chip_state((void *)bdev, cif_state->ops_error);
			}
		}
	}

	return 0;
}

static struct notifier_block btmtk_reboot_notifier = {
	.notifier_call = btmtk_reboot_notify,
	.next = NULL,
	.priority = 0,
};

static int main_init(void)
{
	int i = 0;

	BTMTK_DBG("%s", __func__);

	/* Check if user changes default minimum supported intf count */
	if (btmtk_intf_num < BT_MCU_MINIMUM_INTERFACE_NUM) {
		btmtk_intf_num = BT_MCU_MINIMUM_INTERFACE_NUM;
		BTMTK_WARN("%s minimum interface is %d", __func__, btmtk_intf_num);
	}

	BTMTK_DBG("%s supported intf count <%d>", __func__, btmtk_intf_num);

	BTMTK_DBG("%s: Register reboot_notifier callback success.", __func__);

	/* reboot callback */
	register_reboot_notifier(&btmtk_reboot_notifier);

	g_bdev = kzalloc((sizeof(*g_bdev) * btmtk_intf_num), GFP_KERNEL);
	if (!g_bdev) {
		BTMTK_WARN("%s insufficient memory", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < btmtk_intf_num; i++) {
		g_bdev[i] = btmtk_allocate_dev_memory(NULL);
		if (g_bdev[i]) {
			/* BTMTK_STATE_UNKNOWN instead? */
			/* btmtk_set_chip_state(g_bdev[i], BTMTK_STATE_INIT); */

			/* BTMTK_FOPS_STATE_UNKNOWN instead? */
			btmtk_fops_set_state(g_bdev[i], BTMTK_FOPS_STATE_INIT);
		} else {
			return -ENOMEM;
		}
	}

	// Set global variable for btif interface
	g_sbdev = g_bdev[0];

	btmtk_main_info_initialize();

	return 0;
}

static int main_exit(void)
{
	int i = 0;

	BTMTK_INFO("%s releasing intf count <%d>", __func__, btmtk_intf_num);

	if (g_bdev == NULL) {
		BTMTK_WARN("%s g_data is NULL", __func__);
		return 0;
	}

	BTMTK_INFO("%s: Unregister reboot_notifier callback success.", __func__);
	/* Is it necessary? bt_close will be called by reboot. */
	unregister_reboot_notifier(&btmtk_reboot_notifier);

	wakeup_source_unregister(main_info.assert_ws);
	wakeup_source_unregister(main_info.woble_ws);
	wakeup_source_unregister(main_info.eint_ws);
	wakeup_source_unregister(main_info.chip_reset_ws);
#if WAKEUP_BT_IRQ
	wakeup_source_unregister(main_info.irq_ws);
#endif

	for (i = 0; i < btmtk_intf_num; i++) {
		if (g_bdev[i] != NULL)
			btmtk_free_dev_memory(NULL, g_bdev[i]);
	}

	kfree(g_bdev);
	return 0;
}

/**
 * Kernel Module init/exit Functions
 */

int __init main_driver_init(void)
{
	int ret = 0;
	int i;

	/* Mediatek Driver Version */
	BTMTK_INFO("%s: MTK BT Driver Version : %s", __func__, VERSION);

#ifdef CFG_SUPPORT_CHIP_RESET_KO
	/* Register reset function to reset ko */
	resetko_register_module(RESET_MODULE_TYPE_BT,
				"bt",
				TRIGGER_RESET_TYPE_GPIO_API,
				btmtk_resetko_reset,
				btmtk_resetko_notify);
#endif

	ret = main_init();
	if (ret < 0)
		return ret;

	for (i = 0; i < btmtk_intf_num; i++)
		btmtk_set_chip_state(g_bdev[i], BTMTK_STATE_DISCONNECT);

	ret = btmtk_cif_register();
	if (ret < 0) {
		BTMTK_ERR("*** STPBTFWLOG registration failed(%d)! ***", ret);
		main_exit();
		return ret;
	}

	if (main_info.hif_hook.init)
		ret = main_info.hif_hook.init();

#if (USE_DEVICE_NODE == 1)
	btmtk_init_node();
#endif

	BTMTK_DBG("%s: Done", __func__);
	return ret;
}

void __exit main_driver_exit(void)
{
	BTMTK_INFO("%s", __func__);

#ifdef CFG_SUPPORT_CHIP_RESET_KO
	/* notify reset ko module BT rmmod */
	resetko_unregister_module(RESET_MODULE_TYPE_BT);
#ifdef CHIP_IF_SDIO
	unregister_bt_notify_callback();
#elif defined(CHIP_IF_USB)

#endif
#endif
	btmtk_deinit_node();

	if (main_info.hif_hook.exit)
		main_info.hif_hook.exit();

	btmtk_cif_deregister();

	main_exit();
}

module_init(main_driver_init);
module_exit(main_driver_exit);

/**
 * Module Common Information
 */
MODULE_DESCRIPTION("Mediatek Bluetooth Driver");
MODULE_VERSION(VERSION SUBVER);
MODULE_LICENSE("Dual BSD/GPL");
module_param(btmtk_intf_num, int, 0444);
