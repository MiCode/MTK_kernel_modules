/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_UART_H_
#define _BTMTK_UART_H_
#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_woble.h"
#include "btmtk_chip_reset.h"
#include "btmtk_chip_common.h"

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/serial.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/suspend.h>
#if (USE_DEVICE_NODE == 1)
#include "btmtk_proj_sp.h"
#endif

#define HCI_HEADER_LEN	4

struct btmtk_stp_hdr {
	u8	prefix;
	__be16	dlen;
	u8	cs;
} __packed;
#define MTK_STP_TLR_SIZE	2
#define STP_HEADER_LEN	4
#define STP_HEADER_CRC_LEN	2

#define BTMTKUART_FLAG_STANDALONE_HW	 BIT(0)

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
#define BT_HUB_CRC_RHW_OFFSET 13
#define BT_FLOWCTRL_OFFSET 12
#define BT_NONE_FC 0x00
#define BT_HW_FC 0x40
#define BT_SW_FC 0x80

/**
 * Send cmd dispatch evt
 */
#define HCI_EV_VENDOR			0xff

#define READ_ADDRESS_EVT_PAYLOAD_OFFSET 7
#define WOBLE_DEBUG_EVT_TYPE 0xE8

#define LD_PATCH_CMD_LEN 10
#define LD_PATCH_EVT_LEN 8

#define SETBAUD_CMD_LEN 14
#define SETBAUD_EVT_LEN 9

#define GETBAUD_CMD_LEN 9
#define GETBAUD_EVT_LEN 9
#define BAUD_SIZE 4

#define WAKEUP_CMD_LEN 5
#define WAKEUP_EVT_LEN 9

#if (USE_DEVICE_NODE == 0)
#define FWOWN_CMD_LEN 9
#define DRVOWN_CMD_LEN 1
#define OWNTYPE_EVT_LEN 9
#else
#define FWOWN_CMD_LEN 10
#define DRVOWN_CMD_LEN 1
#define OWNTYPE_EVT_LEN 10
#endif

#define BTMTK_MAX_SEND_RETRY 10000
#if (USE_DEVICE_NODE == 1)
#define BTMTK_MAX_WAIT_RETRY 400
#define BTMTK_MAX_WAKEUP_RETRY 4
#else
#define BTMTK_MAX_WAIT_RETRY 30000
#define BTMTK_MAX_WAKEUP_RETRY 2
#endif
#define BTMTK_MAX_RECV_ERR_CNT 3
#define BTMTK_MAX_WAIT_UART_RESUME_CNT 1500

#define BT_UART_DEFAULT_BAUD 115200

/* Delay time between subsys reset GPIO pull low/high */
#define SUBSYS_RESET_GPIO_DELAY_TIME 50

/* Delay time after write data to io_buf */
#define IO_BUF_DELAY_TIME 50

/* Time bound for flush tty: 100ms */
#define TIMT_BOUND_OF_CHARS_WAIT 40
#define TIME_BOUND_OF_TTY_FLUSH 100
#define TIME_BOUND_OF_FW_PKG_DL 2000
#define TIME_DUMP_OF_FW_PKG_DL 50

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
	atomic_t		thread_status;
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

	/* config form dts*/
	u32			baudrate;
	u32			hub_en;
	u32			sleep_en;
	u32			flush_en;
	u8			hub_bypass_only;
	u32			assert_state;

	/* For uarthub setting */
	u8			fw_hub_mode;
	u8			crc_en;
	u8			rhw_en;
	u8			fw_dl_ready;

	/* driver,fw own */
	bool			no_fw_own;
	u8			own_state;
	struct timer_list	fw_own_timer;
	atomic_t		fw_own_timer_flag;
	atomic_t		need_drv_own;
	atomic_t		fw_wake;

	atomic_t		need_assert;

	u32			rhw_fail_cnt;

	/* sempaphore to compare event */
	struct semaphore	evt_comp_sem;
	/* sempaphore to tty flush & write operation */
	struct semaphore	tty_flush_sem;

#if (USE_DEVICE_NODE == 1)
	/* dynamic tx power control */
	struct btmtk_dypwr_st dy_pwr;

	/* pre-cal flag */
	bool 		is_pre_cal;

	bool		is_pre_on_done;

	/* identify bt sleep flow hw mech */
	int		sleep_flow_hw_mech_en;

#endif
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
#define HCIUARTDEINIT _IOW('U', 207, int)
#define HCIUARTXOPARAM _IOW('U', 208, CONNXO_CFG_PARAM_STRUCT)
/**
 * parameter settings
 */
#define BTMTK_HUB_EN		(1 << 0)
#define BTMTK_SLEEP_EN		(1 << 1)
#define BTMTK_UARTHUB_BYPASS_ONLY	(1 << 2)


//int btmtk_cif_send_calibration(struct hci_dev *hdev);
#endif

