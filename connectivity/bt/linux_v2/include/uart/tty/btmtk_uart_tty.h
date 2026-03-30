/* SPDX-License-Identifier: GPL-2.0 */  
/*
 * Copyright (c) 2016,2017 MediaTek Inc.
 */

#ifndef _BTMTK_UART_H_
#define _BTMTK_UART_H_
#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_buffer_mode.h"
#include "btmtk_woble.h"
#include "btmtk_chip_reset.h"

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/serial.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <linux/suspend.h>

#define HCI_HEADER_LEN	4

struct mtk_stp_hdr {
	u8	prefix;
	__be16	dlen;
	u8	cs;
} __packed;
#define MTK_STP_TLR_SIZE	2
#define STP_HEADER_LEN	4
#define STP_HEADER_CRC_LEN	2

#define BTMTKUART_FLAG_STANDALONE_HW	 BIT(0)

/* CMD&Event sent by driver */
#define READ_REGISTER_CMD_LEN		16
#define READ_REGISTER_EVT_HDR_LEN		11

#define WRITE_REGISTER_CMD_LEN		24
#define WRITE_REGISTER_EVT_HDR_LEN		11

/* MCU address offset */
#define MCU_ADDRESS_OFFSET_CMD 12
#define MCU_ADDRESS_OFFSET_EVT 16

/* MCU value offset */
#define MCU_VALUE_OFFSET_CMD 16

/* Pimux Address and Value */
#define BT_PINMUX_CTRL_REG 0x70005054
#define BT_SUBSYS_RST_PINMUX 0x00000020
#define BT_CTSRTS_PINMUX 0x00044000
#define BT_PINMUX_CTRL_ENABLE (BT_SUBSYS_RST_PINMUX | BT_CTSRTS_PINMUX)

#define BT_SUBSYS_RST_REG 0x70002610
#define BT_SUBSYS_RST_ENABLE 0x00000080

#define BT_REG_LEN 4
#define BT_REG_VALUE_LEN 4

/* MCU baud define */
#define BT_FLOWCTRL_OFFSET 12
#define BT_NONE_FC 0x00
#define BT_HW_FC 0x40
#define BT_SW_FC 0x80
#define BT_MTK_SW_FC 0xC0

/**
 * Send cmd dispatch evt
 */
#define HCI_EV_VENDOR			0xff

#define READ_ADDRESS_EVT_HDR_LEN 7
#define READ_ADDRESS_EVT_PAYLOAD_OFFSET 7
#define WOBLE_DEBUG_EVT_TYPE 0xE8

#define LD_PATCH_CMD_LEN 10
#define LD_PATCH_EVT_LEN 8

#define SETBAUD_CMD_LEN 13
#define SETBAUD_EVT_LEN 9

#define GETBAUD_CMD_LEN 9
#define GETBAUD_EVT_LEN 9
#define BAUD_SIZE 4

#define WAKEUP_CMD_LEN 5
#define WAKEUP_EVT_LEN 9

#define FWOWN_CMD_LEN 9
#define DRVOWN_CMD_LEN 9
#define OWNTYPE_EVT_LEN 9

#define BT_UART_DEFAULT_BAUD 115200

/* Delay time between subsys reset GPIO pull low/high */
#define SUBSYS_RESET_GPIO_DELAY_TIME 50

/* Delay time after write data to io_buf */
#define IO_BUF_DELAY_TIME 50

typedef int (*pdwnc_func) (u8 fgReset);
typedef int (*reset_func_ptr2) (unsigned int gpio, int init_value);
typedef int (*set_gpio_low)(u8 gpio);
typedef int (*set_gpio_high)(u8 gpio);

enum UART_FC {
	UART_DISABLE_FC = 0, /*NO flow control*/
	/*MTK SW Flow Control, differs from Linux Flow Control*/
	UART_MTK_SW_FC = 1,
	UART_LINUX_FC = 2,   /*Linux SW Flow Control*/
	UART_HW_FC = 3,	  /*HW Flow Control*/
};

struct UART_CONFIG {
	enum UART_FC fc;
	int parity;
	int stop_bit;
	int iBaudrate;
};

struct btmtk_uart_data {
	unsigned int flags;
	const char *fwname;
};

struct btmtk_uart_dev {
	struct hci_dev	   *hdev;
	struct tty_struct *tty;
	unsigned long	hdev_flags;

	/* For tx queue */
	struct sk_buff_head	tx_queue;
	spinlock_t		tx_lock;
	struct task_struct	*tx_task;
	unsigned long		tx_state;

	/* For rx queue */
	struct sk_buff		*rx_skb;
	unsigned long		rx_state;

	struct sk_buff		*evt_skb;
	wait_queue_head_t	p_wait_event_q;

	unsigned int		subsys_reset;
	unsigned int		dongle_state;
	unsigned int		uart_baudrate_set;

	u8	stp_pad[6];
	u8	stp_cursor;
	u16	stp_dlen;

	struct UART_CONFIG	uart_cfg;
	struct btmtk_woble	bt_woble;
};

#define btmtk_uart_is_standalone(bdev)	\
	((bdev)->data->flags & BTMTKUART_FLAG_STANDALONE_HW)
#define btmtk_uart_is_builtin_soc(bdev)	\
	!((bdev)->data->flags & BTMTKUART_FLAG_STANDALONE_HW)


/**
 * Maximum rom patch file name length
 */

#define N_MTK        (15+1)
/**
 * Upper layeard IOCTL
 */
#define HCIUARTSETPROTO _IOW('U', 200, int)
#define HCIUARTSETBAUD _IOW('U', 201, int)
#define HCIUARTGETBAUD _IOW('U', 202, int)
#define HCIUARTSETSTP _IOW('U', 203, int)
#define HCIUARTLOADPATCH _IOW('U', 204, int)
#define HCIUARTSETWAKEUP _IOW('U', 205, int)
#define HCIUARTINIT _IOW('U', 206, int)

//int btmtk_cif_send_calibration(struct hci_dev *hdev);
#endif

