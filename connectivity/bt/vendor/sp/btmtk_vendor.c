/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/firmware.h>
#include "btmtk_vendor.h"

#define BTMTK_VND_VERSION     "6.0.2024101413"
#define LOG_TAG            "[btmtk-vendor]"
#define LOG_INFO(tag, fmt, args...) pr_emerg("%s: " fmt "\n", tag, ## args)
#define LOGD_ERR(tag, fmt, args...) pr_err("%s: " fmt "\n", tag, ## args)
#define LOG_DEBUG(tag, fmt, args...) pr_info("%s: " fmt "\n", tag, ## args)
#define LOGINFO(...)           LOG_INFO(LOG_TAG,__VA_ARGS__)
#define LOGERR(...)            LOGD_ERR(LOG_TAG,__VA_ARGS__)
#define LOGDBG(...)            LOG_DEBUG(LOG_TAG,__VA_ARGS__)
#define HCI_SNOOP_MAX_BUF_SIZE 66
#define BTMTK_INFO_RAW(p, l, fmt, ...)						\
	do {	\
			int cnt_ = 0;	\
			int len_ = (l <= HCI_SNOOP_MAX_BUF_SIZE ? l : HCI_SNOOP_MAX_BUF_SIZE);	\
			uint8_t raw_buf[HCI_SNOOP_MAX_BUF_SIZE * 5 + 10];	\
			const unsigned char *ptr = p;	\
			for (cnt_ = 0; cnt_ < len_; ++cnt_)	\
				(void)snprintf(raw_buf+5*cnt_, 6, "0x%02X ", ptr[cnt_]);	\
			raw_buf[5*cnt_] = '\0';	\
			if (l <= HCI_SNOOP_MAX_BUF_SIZE) {	\
				LOGINFO("%s: data:%s", __func__, raw_buf);	\
			} else {	\
				LOGINFO("%s: data:%s", __func__, raw_buf);	\
			}	\
	} while (0)

#define BTMTK_DBG_RAW(p, l, fmt, ...)						\
	do {	\
			int cnt_ = 0;	\
			int len_ = (l <= HCI_SNOOP_MAX_BUF_SIZE ? l : HCI_SNOOP_MAX_BUF_SIZE);	\
			uint8_t raw_buf[HCI_SNOOP_MAX_BUF_SIZE * 5 + 10];	\
			const unsigned char *ptr = p;	\
			for (cnt_ = 0; cnt_ < len_; ++cnt_)	\
				(void)snprintf(raw_buf+5*cnt_, 6, "0x%02X ", ptr[cnt_]);	\
			raw_buf[5*cnt_] = '\0';	\
			if (l <= HCI_SNOOP_MAX_BUF_SIZE) {	\
				LOGDBG("%s: data:%s", __func__, raw_buf);	\
			} else {	\
				LOGDBG("%s: data:%s", __func__, raw_buf);	\
			}	\
	} while (0)

static int major_device_id;
static int minor_device_id = 0;
static dev_t devno;
static struct cdev vendor_cdev;
static struct class *vendordevclass;
static struct device *vendordev;
struct sk_buff_head vendor_rx_q;
static wait_queue_head_t vendor_wait_q;
spinlock_t vendor_vendor_spinlock;
struct LE_CLOCK_INFO le_info;
bool le_irq_enable = 0;

#define READ_EFUSE_CMD_LEN 18
#define READ_EFUSE_CMD_BLOCK_OFFSET 10
#define READ_EFUSE_EVT_HDR_LEN 9
enum {
	CO_PIN_UNKNOWN,
	CO_PIN_DISABLE,
	CO_PIN_ENABLE
};
enum {
	PIN_UNKNOWN,
	PIN_DISABLE,
	PIN_ENABLE
};
static int co_pin_flag = CO_PIN_UNKNOWN;
static int pin_flag = PIN_UNKNOWN;
uint8_t efuse_event[0x1E + 3] = {0x4, 0xE4, 0x1E, 0x02, 0x0D, 0x1A, 0x00, 02, 04};
static int le_audio_clk_fw_gpio = -1;
static int le_audio_clk_fw_co_gpio = -1;
static unsigned int chip_id = 0;
#define BT_LE_AUDIO_CLK_FW_GPIO "LE_AUDIO_CLK_FW_GPIO"
#define BT_LE_AUDIO_CLK_FW_CO_GPIO "LE_AUDIO_CLK_FW_CO_GPIO"

static DEFINE_MUTEX(btmtk_le_clk_mutex);
#define LE_CLK_MUTEX_LOCK()	mutex_lock(&btmtk_le_clk_mutex)
#define LE_CLK_MUTEX_UNLOCK()	mutex_unlock(&btmtk_le_clk_mutex)

void btmtk_le_getrawtime(struct timespec64 *tv)
{
	struct timespec64 ts;

#if (KERNEL_VERSION(4, 18, 0) > LINUX_VERSION_CODE)
	getrawmonotonic64(&ts);
#else
	ktime_get_raw_ts64(&ts);
#endif
	*tv = ts;
}

static int btmtk_le_rxclk_check_copin(void) {
#define EFUSE_COPIN_ENABLE (0x1 << 7)
#define READ_EFUSE_CMD_LEN 18

	u16 addr = 0x49;
	uint8_t sub_block_addr_in_event = 0;
	uint16_t sub_block = 0;
	uint8_t temp = 0;
	uint8_t value = 0;
	uint8_t efuse_cmd[READ_EFUSE_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0E,
				0x01, 0x0D, 0x0A, 0x00, 0x02, 0x04,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00};/*4 sub block number(sub block 0~3)*/
	struct sk_buff *skb = NULL;
	int ret = 0;
	int retry = 200;

	if (co_pin_flag != CO_PIN_UNKNOWN)
		return co_pin_flag;

	skb = alloc_skb(READ_EFUSE_CMD_LEN + BT_SKB_RESERVE, GFP_ATOMIC);
	if (!skb) {
		LOGERR("alloc skb buffer fail, no memory");
		return false;
	}
	if (chip_id == 0x7925)
		addr = 0x3E;

	sub_block = (addr / 16) * 4;

	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET] = sub_block & 0xFF;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 1] = (sub_block & 0xFF00) >> 8;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 2] = (sub_block + 1) & 0xFF;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 3] = ((sub_block + 1) & 0xFF00) >> 8;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 4] = (sub_block + 2) & 0xFF;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 5] = ((sub_block + 2) & 0xFF00) >> 8;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 6] = (sub_block + 3) & 0xFF;
	efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 7] = ((sub_block + 3) & 0xFF00) >> 8;

	memcpy(skb->data, efuse_cmd, READ_EFUSE_CMD_LEN);
	skb->len = READ_EFUSE_CMD_LEN;

	ret = btmtk_send_vendor_cmd(skb);
	if (ret < 0) {
		kfree_skb(skb);
		return CO_PIN_DISABLE;
	}

	while (co_pin_flag == CO_PIN_UNKNOWN && retry-- > 0)
		msleep(5);

	if (retry > 0 &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET] == efuse_event[9] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 1] == efuse_event[10] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 2] == efuse_event[15] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 3] == efuse_event[16] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 4] == efuse_event[21] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 5] == efuse_event[22] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 6] == efuse_event[27] &&
		efuse_cmd[READ_EFUSE_CMD_BLOCK_OFFSET + 7] == efuse_event[28]) {
		/*Get value*/
		/*check event
		 *04 E4 LEN(1B) 02 0D LEN(2Byte) 02 04 ADDR(2Byte) VALUE(4B) ADDR(2Byte) VALUE(4Byte)
		 *ADDR(2Byte) VALUE(4B)  ADDR(2Byte) VALUE(4Byte)
		 */
		sub_block_addr_in_event = ((addr / 16) / 4);/*cal block num*/
		temp = addr % 16;
		switch (temp) {
		case 0:
		case 1:
		case 2:
		case 3:
			value = efuse_event[11 + temp];
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			value = efuse_event[17 + temp - 4];
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			value = efuse_event[23 + temp - 8];
			break;
		case 12:
		case 13:
		case 14:
		case 15:
			value = efuse_event[29 + temp - 12];
			break;
		}

		LOGINFO("%s: copin is 0x%02X", __func__, value);
		if ((value & EFUSE_COPIN_ENABLE) != EFUSE_COPIN_ENABLE)
			co_pin_flag = CO_PIN_DISABLE;
		else
			co_pin_flag = CO_PIN_ENABLE;
	} else {
		LOGERR("%s: get event failed(%d)", __func__, retry);
		co_pin_flag = CO_PIN_DISABLE;
	}
	kfree_skb(skb);
	return co_pin_flag;
}

static irqreturn_t btmtk_le_rxclk_irq_handler(int irq, void *dev)
{
	/* Get sys clk */
	struct timespec64 tv;

	btmtk_le_getrawtime(&tv);
	le_info.sysclk = (u64)tv.tv_sec * 1000000L + tv.tv_nsec/1000;
	le_info.irq_cnt++;
	//LOGERR("%s:pin 0 sysclk(%llu), irq_cnt=%d", __func__,
	//						le_info.sysclk,le_info.irq_cnt);
#if CFG_SUPPORT_DEINT_IRQ
	mtk_deint_ack(irq);
#endif
	return IRQ_HANDLED;
}

int btmtk_le_rxclk_enable(struct sk_buff *skb)
{
	if (le_info.irq < 0) {
		skb->data[LE_CLK_EN_OFFSET_METHOD] = 0x03; // Get clk if event received
	} else {
		skb->data[LE_CLK_EN_OFFSET_METHOD] = 0x01; // Get clk if irq triggered
		switch (chip_id) {
		case 0x7961:
			if (btmtk_le_rxclk_check_copin() == CO_PIN_ENABLE) {
				if (le_audio_clk_fw_co_gpio != -1)
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = le_audio_clk_fw_co_gpio;
				else
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = 0x0D;
			} else {
				if (le_audio_clk_fw_gpio != -1)
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = le_audio_clk_fw_gpio;
				else
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = 0x03;
			}
			break;
		case 0x7902:
			if (le_audio_clk_fw_gpio != -1)
				skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = le_audio_clk_fw_gpio;
			else
				skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = 0x2B;
			break;
		case 0x7925:
			if (btmtk_le_rxclk_check_copin() == CO_PIN_ENABLE) {
				if (le_audio_clk_fw_co_gpio != -1)
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = le_audio_clk_fw_co_gpio;
				else
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = 0x05;
			} else {
				if (le_audio_clk_fw_gpio != -1)
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = le_audio_clk_fw_gpio;
				else
					skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = 0x05;
			};
			break;
		default:
			skb->data[LE_CLK_EN_OFFSET_GPIO_NUM] = 0x03;
		}

		if (le_info.irq_type == IRQF_TRIGGER_RISING ||
			le_info.irq_type == IRQF_TRIGGER_HIGH)
			skb->data[LE_CLK_EN_OFFSET_TRIGGER_TYPE] = 0x01; /* default low, active high */
		else
			skb->data[LE_CLK_EN_OFFSET_TRIGGER_TYPE] = 0x00; /* default high, active low */
	}
	return 0;
}

static int btmtk_irq_count(struct device_node *dev) {
	struct of_phandle_args irq;
	int nr = 0;

	while (of_irq_parse_one(dev, nr, &irq) == 0)
		nr++;

	return nr;
}

static void btmtk_le_rxclk_init(void)
{
#define NUM_INTR 2
#define SIZE_INTR 2
	int ret = 0;
	struct device_node *node;
	int interrupts[SIZE_INTR * NUM_INTR];
	int intr_idx = 0;
	int intr_num = 0;
#if CFG_SUPPORT_DEINT_IRQ
	int deint[3];
#endif

	/* Get irq info from dts */
	LOGINFO("%s: enter, irq=%d", __func__, le_info.irq);

	node = of_find_compatible_node(NULL, NULL, "mediatek,connectivity-combo");
	if (!node) {
		LOGERR("%s: get node failed", __func__);
		return;
	}

	intr_num = btmtk_irq_count(node);
	LOGINFO("%s: get intr_num(%d)", __func__, intr_num);
	if (intr_num > NUM_INTR || intr_num == 0)
		return;

	ret = of_property_read_u32_array(node, "interrupts",
				interrupts, SIZE_INTR * intr_num);
	if (ret < 0) {
		LOGERR("%s: get interrupts fail(%d)", __func__, ret);
		return;
	}

	intr_idx = ((btmtk_le_rxclk_check_copin() == CO_PIN_ENABLE) && (intr_num == NUM_INTR)) ? 1 : 0;
	if (le_info.irq <= 0) {
		le_info.irq = irq_of_parse_and_map(node, intr_idx);
		if (le_info.irq <= 0) {
			LOGINFO("%s: request irq failed", __func__);
			return;
		}
		LOGINFO("%s: irq_of_parse_and_map, irq=%d", __func__, le_info.irq);
	}

#if CFG_SUPPORT_DEINT_IRQ
	ret = request_irq(le_info.irq, btmtk_le_rxclk_irq_handler,
					IRQF_TRIGGER_NONE, "BT_LE_AUDIO_CLK_DEINT_ISR_Handler", NULL);
	if (ret) {
		LOGERR("%s: request_irq DEINT fail(%d) irq_number=%d", __func__, ret,
			le_info.irq);
		le_info.irq = -1;
		return;
	}
//	ret = mt_secure_call(MTK_SIP_BT_FIQ_REG, interrupts[1], interrupts[2], 0, 0);
	of_property_read_u32_array(node, "deint", deint, ARRAY_SIZE(deint));
	mtk_deint_enable(deint[0], deint[1], deint[2]);
	le_info.irq_type = deint[2];
	LOGDBG("%s: irq_type=%d ", __func__, le_info.irq_type);
#else	/*generic EINT*/
	ret = request_irq(le_info.irq, btmtk_le_rxclk_irq_handler,
					interrupts[1 + (intr_num - 1) * 2], "BT_LE_AUDIO_CLK_ISR_Handler", NULL);
	if (ret) {
		LOGERR("%s: request_irq EINT fail(%d) interrupts[1]=%d", __func__, ret,
				 interrupts[1 + (intr_num - 1) * 2]);
		le_info.irq = -1;
		return;
	}
	le_info.irq_type = interrupts[1 + (intr_num - 1) * 2];
#endif
	le_irq_enable = 1;
}

static int btmtk_check_le_sync_pin(struct sk_buff *skb)
{
	u8 event[LE_CLK_EN_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0xFD, 0x00};

	if (skb == NULL || skb->len < LE_CLK_EN_EVT_LEN -1)
		return -ENOMEM;
	if (memcmp(&event[1], skb->data, LE_CLK_EN_EVT_LEN -1) == 0) {
		skb_put(skb, 1);
		skb->data[1] = 0x05;
		if (le_info.irq_cnt > 0) {
			LOGINFO("%s: Le audio sync pin set", __func__);
			skb->data[6] = 0x01; /*pin connect*/
			pin_flag = PIN_ENABLE;
		} else {
			LOGINFO("%s: No le audio sync pin", __func__);
			skb->data[6] = 0x00; /*no pin or pin disconnect*/
			pin_flag = PIN_DISABLE;
		}
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: enable clk event:", __func__);
	}
	return pin_flag;
}

static void btmtk_le_rxclk_evt_handler(struct sk_buff *skb)
{
	int rxclk_sys_time_pos = 0;

	/*check pin exit or not*/
	if (pin_flag == PIN_UNKNOWN) {
		if (btmtk_check_le_sync_pin(skb) != PIN_UNKNOWN)
			return;
	}

	/*check co-pin exit or not*/
	if (co_pin_flag == CO_PIN_UNKNOWN && memcmp(&efuse_event[1], skb->data, READ_EFUSE_EVT_HDR_LEN - 1) == 0) {
		LOGINFO("%s: get efuse event", __func__);
		memcpy(&efuse_event[1], skb->data, sizeof(efuse_event) - 1);
		co_pin_flag = CO_PIN_DISABLE;
		return;
	}

	if (skb->data[1] == 0x0C)
		rxclk_sys_time_pos = RXCLK_SYS_TIME_POS;
	else if (skb->data[1] == 0x14)
		rxclk_sys_time_pos = RXPTS_SYS_TIME_POS;
	else
		return;

	if ((bt_cb(skb)->pkt_type == HCI_EVENT_PKT) &&
			skb->data[0] == 0x0E &&
			skb->data[3] == 0x03 &&
			skb->data[4] == 0xFD) {
		//BTMTK_INFO_RAW(skb->data, skb->len, "%s: vendor_ko_recv_event: ", __func__);
		skb_put(skb, 5);
		skb->data[1] += 5;
		if (le_info.irq > 0 && le_info.sysclk != 0 && le_info.irq_cnt > 0) {
			//main_info.le_audio_sysclk =
			//	mt_secure_call(MTK_SIP_BT_GET_CLK, 0, 0, 0, 0);
			//LOGINFO("%s:pin 0 sysclk(%llu), irq_cnt=%d", __func__,
			//				le_info.sysclk,le_info.irq_cnt);
			skb->data[rxclk_sys_time_pos + 4] = 0; //Add NO_IRQ_FLAG, 0:trigger by IRQ
		} else {
			struct timespec64 tv;

			btmtk_le_getrawtime(&tv);
			le_info.sysclk = (u64)tv.tv_sec * 1000000L + tv.tv_nsec/1000;
			skb->data[rxclk_sys_time_pos + 4] = 1; //Add NO_IRQ_FLAG, 1:not trigger by IRQ
			//LOGINFO("%s: no pin 1, irq_cnt=%d", __func__, le_info.irq_cnt);
		}
		/* Add SYS_CLOCK to tail
		 * OLD format: 0E LEN(1) 01 03 FD 00 HANDLE(2) BITCNT(2) BTCLK(4)
		 * NEW format: 0E LEN(1) 01 03 FD 00 HANDLE(2) BITCNT(2) BTCLK(4) SYSCLK(4)
		 */
		skb->data[rxclk_sys_time_pos] = le_info.sysclk & 0xFF;
		skb->data[rxclk_sys_time_pos + 1] = (le_info.sysclk >> 8) & 0xFF;
		skb->data[rxclk_sys_time_pos + 2] = (le_info.sysclk >> 16) & 0xFF;
		skb->data[rxclk_sys_time_pos + 3] = (le_info.sysclk >> 24) & 0xFF;
		le_info.sysclk = 0;
		if (le_info.irq_cnt != 1)
			LOGERR("%s: le_audio_irq_cnt(%d)", __func__, le_info.irq_cnt);
		le_info.irq_cnt = 0;
	}
}

static void btmtk_le_rxclk_deinit(void *bdev)
{
	LOGINFO("%s: enter", __func__);
	if (le_info.irq > 0) {
		free_irq(le_info.irq, NULL);
		le_irq_enable = 0;
	}
	pin_flag = PIN_UNKNOWN;
}

static ssize_t btmtk_vendor_user_recv(struct file *file, char __user *buf,
					size_t count, loff_t *f_pos)
{
	unsigned long readlen = 0;
	unsigned long ret = 0;
	ulong flags = 0;
	struct sk_buff *skb = NULL;

	spin_lock_irqsave(&vendor_vendor_spinlock, flags);
	if (skb_queue_len(&vendor_rx_q))
		skb = skb_dequeue(&vendor_rx_q);
	spin_unlock_irqrestore(&vendor_vendor_spinlock, flags);
	if (skb == NULL)
		return 0;
	readlen = (count < skb->len) ? count : skb->len;
	ret = copy_to_user(buf, skb->data, readlen);
	if (ret)
		LOGERR("copy to user fail");

	if (skb)
		kfree_skb(skb);
	return readlen;
}

static void btmtk_vendor_event_recv(struct sk_buff *skb)
{
	ulong flags = 0;
	struct sk_buff *skb_temp = NULL;

	if (skb) {
		skb_temp= skb_copy(skb, GFP_ATOMIC);
		if (!skb_temp) {
			LOGERR("%s, skb_copy event is fail!", __func__);
			return;
		}
		//BTMTK_INFO_RAW(skb->data, skb->len, "%s: vendor_ko_recv_event: ", __func__);
		btmtk_le_rxclk_evt_handler(skb_temp);
		spin_lock_irqsave(&vendor_vendor_spinlock, flags);
		skb_queue_tail(&vendor_rx_q, skb_temp);
		spin_unlock_irqrestore(&vendor_vendor_spinlock, flags);
		wake_up_interruptible(&vendor_wait_q);
	}
}

static ssize_t btmtk_vendor_send_cmd(struct file *file, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	size_t writelen = 0;
	int ret = 0;
	struct sk_buff *skb = NULL;
	u8 cmd[LE_CLK_EN_CMD_LEN] = { 0x01, 0x03, 0xFD, 0x04, 0x00, 0x00, 0x00, 0x00 };
	u8 cmd2[LE_CLK_DISABLE_CMD_LEN] = { 0x01, 0x03, 0xFD, 0x01, 0x04};

	writelen = (count < HCI_PKT_SIZE) ? count : HCI_PKT_SIZE;
	skb = alloc_skb(writelen + BT_SKB_RESERVE, GFP_ATOMIC);
	if (!skb) {
		LOGERR("alloc skb buffer fail, no memory");
		return -ENOMEM;
	}
	if (buf == NULL) {
		kfree_skb(skb);
		return -EINVAL;
	}
	ret = copy_from_user(skb->data, buf, writelen);
	if (ret < 0) {
		LOGERR("copy cmd from user fail");
		kfree_skb(skb);
		return -EFAULT;
	}

	skb->len = writelen;
	/*filter le audio clk cmd , add clk gpio info data to cmd*/
	if (skb->len == LE_CLK_EN_CMD_LEN &&
		memcmp(cmd, skb->data, LE_CLK_EN_CMD_LEN) == 0) {
		if (!le_irq_enable) {
			memset(&le_info, 0, sizeof(le_info));
			LE_CLK_MUTEX_LOCK();
			btmtk_le_rxclk_init();
			LE_CLK_MUTEX_UNLOCK();
		}
		btmtk_le_rxclk_enable(skb);
	}
	/*filter le audio clk disable cmd, don't send to fw*/
	if (skb->len == LE_CLK_DISABLE_CMD_LEN &&
		memcmp(cmd2, skb->data, LE_CLK_DISABLE_CMD_LEN) == 0) {
		LE_CLK_MUTEX_LOCK();
		btmtk_le_rxclk_deinit(NULL);
		LE_CLK_MUTEX_UNLOCK();
		memset(&le_info, 0, sizeof(le_info));
		kfree_skb(skb);
		return ret;
	}
	//BTMTK_INFO_RAW(skb->data, skb->len, "%s: vendor_ko_send_cmd: ", __func__);
	ret = btmtk_send_vendor_cmd(skb);
	if (ret < 0) {
		LOGERR("send cmd to bt driver fail");
		kfree_skb(skb);
		return ret;
	}
	kfree_skb(skb);
	return writelen;
}


static bool btmtk_vendor_parse_bt_cfg_file(char *item_name,
		char *text, u8 *searchcontent)
{
#define SEARCH_LEN 32
	bool ret = 0;
	int temp_len = 0;
	char search[SEARCH_LEN];
	char *ptr = NULL, *p = NULL;
	char *temp = text;

	if (text == NULL) {
		LOGERR("%s: text param is invalid!", __func__);
		ret = false;
		goto out;
	}

	memset(search, 0, SEARCH_LEN);
	(void)snprintf(search, SEARCH_LEN, "%s", item_name); /* EX: SUPPORT_UNIFY_WOBLE */
	p = ptr = strstr((char *)searchcontent, search);

	if (!ptr) {
		LOGERR("%s: Can't find %s\n", __func__, item_name);
		ret = false;
		goto out;
	}

	if (p > (char *)searchcontent) {
		p--;
		while ((*p == ' ') && (p != (char *)searchcontent))
			p--;
		if (*p == '#') {
			LOGERR("%s: It's invalid bt cfg item\n", __func__);
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


static int btmtk_vendor_load_bt_cfg(unsigned char *cfg_file)
{
#define TEXT_LEN 128
	int err = 0;
	bool ret;
	const struct firmware *fw_entry = NULL;
	unsigned char	*setting_file;
	int code_len;
	char text[TEXT_LEN]; /* save for search text */
	unsigned long text_value = 0;

	LOGINFO("%s: begin setting_file_name = %s", __func__, cfg_file);
	err = request_firmware(&fw_entry, cfg_file, NULL);
	if (err != 0 || fw_entry == NULL) {
		LOGERR("%s: request_firmware fail, maybe file %s not exist, err = %d, fw_entry = %p",
				__func__, cfg_file, err, fw_entry);
		if (fw_entry)
			release_firmware(fw_entry);
		return -ENOENT;
	}

	LOGINFO("%s: setting file request_firmware size %zu success", __func__, fw_entry->size);
	setting_file = kzalloc(fw_entry->size + 1, GFP_KERNEL); /* alloc setting file memory */
	if (setting_file == NULL) {
		LOGERR("%s: kzalloc size %zu failed!!", __func__, fw_entry->size);
		release_firmware(fw_entry);
		return -ENOMEM;
	}

	memcpy(setting_file, fw_entry->data, fw_entry->size);
	setting_file[fw_entry->size] = '\0';

	code_len = fw_entry->size;
	release_firmware(fw_entry);

	LOGINFO("%s: setting_file len (%d) assign done", __func__, code_len);
	ret = btmtk_vendor_parse_bt_cfg_file(BT_LE_AUDIO_CLK_FW_GPIO, text, setting_file);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			le_audio_clk_fw_gpio = text_value;
		else
			LOGERR("%s: kstrtoul failed %s!", __func__, BT_LE_AUDIO_CLK_FW_GPIO);
	} else {
		LOGERR("%s: search item %s is invalid!", __func__, BT_LE_AUDIO_CLK_FW_GPIO);
	}
	ret = btmtk_vendor_parse_bt_cfg_file(BT_LE_AUDIO_CLK_FW_CO_GPIO, text, setting_file);
	if (ret) {
		if (kstrtoul(text, 10, &text_value) == 0)
			le_audio_clk_fw_co_gpio = text_value;
		else
			LOGERR("%s: kstrtoul failed %s!", __func__, BT_LE_AUDIO_CLK_FW_CO_GPIO);
	} else {
		LOGERR("%s: search item %s is invalid!", __func__, BT_LE_AUDIO_CLK_FW_CO_GPIO);
	}

	kfree(setting_file);
	return 0;
}

static int btmtk_vendor_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	unsigned char cfg_file[32] = {0};
	event_callback cb = btmtk_vendor_event_recv;

	LOGINFO("%s enter", __func__);
	ret = btmtk_vendor_module_register(cb, cfg_file, 32);
	if (ret == 0) {
		LOGINFO("%s btmtk_vendor_open success", __func__);
		if (strlen(cfg_file) != 0) {
			btmtk_vendor_load_bt_cfg(cfg_file);
			if (strstr(cfg_file, "7961") != NULL)
				chip_id = 0x7961;
			else if (strstr(cfg_file, "7902") != NULL)
				chip_id = 0x7902;
			else if (strstr(cfg_file, "7925") != NULL)
				chip_id = 0x7925;
		}
	}
	return ret;
}

static int btmtk_vendor_close(struct inode *inode, struct file *file)
{
	int ret = 0;

	LOGINFO("%s enter", __func__);
	btmtk_le_rxclk_deinit(NULL);  //when bt crash, need to call deinit
	ret = btmtk_vendor_module_unregister();
	if (ret == 0)
		LOGINFO("%s btmtk_vendor_close success", __func__);
	le_audio_clk_fw_gpio = -1;
	le_audio_clk_fw_co_gpio = -1;
	chip_id = 0;
	return ret;
}

unsigned int btmtk_vendor_poll_packet(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &vendor_wait_q, wait);
	if (skb_queue_len(&vendor_rx_q) > 0)
		mask |= POLLIN | POLLRDNORM;   /* readable */

	return mask;
}

static const struct file_operations btmtk_vendor_fops = {
    .owner = THIS_MODULE,
    .open = btmtk_vendor_open,
    .read = btmtk_vendor_user_recv,
    .write = btmtk_vendor_send_cmd,
    .poll = btmtk_vendor_poll_packet,
    .release = btmtk_vendor_close,
};

static int btmtk_vendor_init(void)
{
	int ret = 0;

	devno = MKDEV(major_device_id, minor_device_id);
	ret = alloc_chrdev_region(&devno, 0, 1, "mtk_bt_vendor_chrdev");
	if (ret) {
		LOGERR("alloc device id fail");
		return ret;
	}
	cdev_init(&vendor_cdev, &btmtk_vendor_fops);
	vendor_cdev.owner = THIS_MODULE;
	ret = cdev_add(&vendor_cdev, devno, 1);
	if (ret) {
		LOGERR("cdev add fail");
		goto err1;
	}
	vendordevclass = class_create(THIS_MODULE, "mtk_bt_vendor_chrdev");
	if (IS_ERR(vendordevclass))
	{
		LOGERR("cdev class create fail");
		goto err1;
	}
	vendordev = device_create(vendordevclass, NULL, devno, NULL, "%s","mtk_bt_vendor");
	if (IS_ERR(vendordev))
	{
		LOGERR("vendordev create fail");
		goto err2;
	}

	LOGINFO("%s btmtk_vendor_driver_init success", __func__);
	return ret;
err2:
	if (vendordevclass) {
		class_destroy(vendordevclass);
		vendordevclass = NULL;
	}
err1:
	if (ret < 0)
		cdev_del(&vendor_cdev);
	if (ret == 0)
		unregister_chrdev_region(devno, 1);
	LOGERR("%s: error", __func__);
	return -1;
}

int __init btmtk_vendor_driver_init(void)
{
	int ret = 0;

	LOGINFO("%s, btmtk_vendor driver verison:%s!", __func__, BTMTK_VND_VERSION);

	/* rx_queue and vendor_wait_q init */
	skb_queue_head_init(&vendor_rx_q);
	init_waitqueue_head(&(vendor_wait_q));
	spin_lock_init(&vendor_vendor_spinlock);

	ret = btmtk_vendor_init();
	LOGINFO("%s, btmtk_vendor driver init success!", __func__);
	return ret;
}

static void btmtk_vendor_exit(void)
{
	LOGINFO("%s btmtk_vendor_exit enter", __func__);
	if (vendordev) {
		device_destroy(vendordevclass, devno);
		vendordev = NULL;
	}

	if (vendordevclass) {
		class_destroy(vendordevclass);
		vendordevclass = NULL;
	}

	cdev_del(&vendor_cdev);
	unregister_chrdev_region(devno, 1);

	LOGINFO("%s btmtk_vendor_driver_exit success", __func__);
}

void __exit btmtk_vendor_driver_exit(void)
{
	btmtk_vendor_exit();
	LOGINFO("%s btmtk_vendor driver exit", __func__);
}

module_init(btmtk_vendor_driver_init);
module_exit(btmtk_vendor_driver_exit);
MODULE_LICENSE("GPL");
