/* SPDX-License-Identifier: GPL-2.0 */  
/*
 * Copyright (c) 2016,2017 MediaTek Inc.
 */

#ifndef _BTMTK_USB_H_
#define _BTMTK_USB_H_
#include <linux/usb.h>
#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_woble.h"
#include "btmtk_chip_reset.h"

#define HCI_MAX_COMMAND_SIZE	255
#define URB_MAX_BUFFER_SIZE	(4*1024)

#define BT0_MCU_INTERFACE_NUM 0
#define BT1_MCU_INTERFACE_NUM 3
#define BT_MCU_INTERFACE_NUM_MAX 4
#define BT_MCU_NUM_MAX 2

typedef int (*pdwnc_func) (u8 fgReset);
typedef int (*reset_func_ptr2) (unsigned int gpio, int init_value);
typedef int (*set_gpio_low)(u8 gpio);
typedef int (*set_gpio_high)(u8 gpio);

/**
 * Send cmd dispatch evt
 */
#define HCI_EV_VENDOR			0xff
#define HCI_USB_IO_BUF_SIZE		256


/* UHW CR mapping */
#define BT_MISC 0x70002510
#define MCU_BT0_INIT_DONE (0x1 << 8)
#define MCU_BT1_INIT_DONE (0x1 << 9)
#define BT_SUBSYS_RST 0x70002610
#define BT_SUBSYS_RST_6639 0x70028610
#define UDMA_INT_STA_BT 0x74000024
#define UDMA_INT_STA_BT1 0x74000308
#define BT_WDT_STATUS 0x740003A0
#define EP_RST_OPT 0x74011890
#define EP_RST_IN_OUT_OPT 0x00010001

#define BT_GDMA_DONE_ADDR_W 0x74000A0C
#define BT_GDMA_DONE_7921_VALUE_W 0x00403FA9
#define BT_GDMA_DONE_7922_VALUE_W 0x00403EA9
#define BT_GDMA_DONE_7902_VALUE_W 0x00403EA9
#define BT_GDMA_DONE_ADDR_R 0x74000A08
#define BT_GDMA_DONE_VALUE_R 0xFFFFFFFB /* bit2: 0 - dma done, 1 - dma doing */

/* CMD&Event sent by driver */
#define NOTIFY_ALT_EVT_LEN 7

#define LD_PATCH_CMD_LEN 9
#define LD_PATCH_EVT_LEN 8

#define READ_ADDRESS_EVT_HDR_LEN 7
#define READ_ADDRESS_EVT_PAYLOAD_OFFSET 7
#define WOBLE_DEBUG_EVT_TYPE 0xE8
#define BLE_EVT_TYPE 0x3E

#define WMT_TRIGGER_ASSERT_LEN 9

struct btmtk_cif_chip_reset {
	/* For Whole chip reset */
	pdwnc_func pf_pdwndFunc;
	reset_func_ptr2 pf_resetFunc2;
	set_gpio_low pf_lowFunc;
	set_gpio_high pf_highFunc;
};

struct btmtk_usb_dev {
	struct usb_endpoint_descriptor	*intr_ep;
	/* EP10 OUT */
	struct usb_endpoint_descriptor	*intr_iso_tx_ep;
	/* EP10 IN */
	struct usb_endpoint_descriptor	*intr_iso_rx_ep;
	/* BULK CMD EP1 OUT or EP 11 OUT */
	struct usb_endpoint_descriptor	*bulk_cmd_tx_ep;
	/* EP15 in for reset */
	struct usb_endpoint_descriptor	*reset_intr_ep;
	struct usb_endpoint_descriptor	*bulk_tx_ep;
	struct usb_endpoint_descriptor	*bulk_rx_ep;
	struct usb_endpoint_descriptor	*isoc_tx_ep;
	struct usb_endpoint_descriptor	*isoc_rx_ep;

	struct usb_device	*udev;
	struct usb_interface	*intf;
	struct usb_interface	*isoc;
	struct usb_interface	*iso_channel;


	struct usb_anchor	tx_anchor;
	struct usb_anchor	intr_anchor;
	struct usb_anchor	bulk_anchor;
	struct usb_anchor	isoc_anchor;
	struct usb_anchor	ctrl_anchor;
	struct usb_anchor	ble_isoc_anchor;

	__u8	cmdreq_type;
	__u8	cmdreq;

	int new_isoc_altsetting;
	int new_isoc_altsetting_interface;

	unsigned char	*o_usb_buf;

	unsigned char	*urb_intr_buf;
	unsigned char	*urb_bulk_buf;
	unsigned char	*urb_ble_isoc_buf;
	struct btmtk_woble	bt_woble;
};

#endif
