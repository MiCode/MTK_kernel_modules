/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef _BTMTK_VENDOR_HCI_H_
#define _BTMTK_VENDOR_HCI_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/skbuff.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/mutex.h>
#include <net/bluetooth/bluetooth.h>

#include "btmtk_extern.h"

#define HCI_PKT_SIZE        255
#define HCI_EVENT_PKT       0x04
#define BT_SKB_RESERVE      8
#define LE_CLK_EN_CMD_LEN  8
#define LE_CLK_EN_EVT_LEN  7
#define LE_CLK_EN_OFFSET_METHOD 5
#define LE_CLK_EN_OFFSET_GPIO_NUM 6
#define LE_CLK_EN_OFFSET_TRIGGER_TYPE 7
#define LE_CLK_DISABLE_CMD_LEN 5
#define RXCLK_SYS_TIME_POS 14
#define RXPTS_SYS_TIME_POS 22

struct LE_CLOCK_INFO {
	int irq;
	u32 irq_type;
	u32 irq_cnt;
	u32 no_irq_cnt;
	u64 sysclk;
};

//typedef void (*event_callback)(struct sk_buff *skb);
#if CFG_SUPPORT_DEINT_IRQ
extern int mtk_deint_enable(unsigned int eint_num, unsigned int spi_num,
					unsigned long type);
extern int mtk_deint_ack(unsigned int irq);
#endif

#endif
