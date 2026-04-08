/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
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

/**
 * For DTS sub system reset
 */
#define CHIP_RESET_DTS_NODE_NAME "mediatek,mtk-bt-reset"
#define CHIP_RESET_GPIO_PROPERTY_NAME "btreset-gpios"
#define BT_PINMUX_CTRL_REG 0x70005054
#define BT_SUBSYS_RST_REG 0x70002610
#define BT_SUBSYS_RST_PINMUX 0x00000020
#define BT_SUBSYS_RST_ENABLE 0x00000080

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
#define BT_GDMA_DONE_6639_VALUE_W 0xC0040900
#define BT_GDMA_DONE_6639_ADDR_W 0x18023A0C
#define BT_GDMA_DONE_6639_ADDR_R 0x18023A10
#define BT_GDMA_DONE_6639_VALUE_R 0xBFFFFFFF /* bit30: 0 - dma done, 1 - dma doing */

#define READ_ADDRESS_EVT_HDR_LEN 7
#define READ_ADDRESS_EVT_PAYLOAD_OFFSET 7
#define WOBLE_DEBUG_EVT_TYPE 0xE8
#define BLE_EVT_TYPE 0x3E

/**
 * Chip debug info dump position - USB operation related CRs
 */
#define POWER_MANAGEMENT	0x74013404
#define USB20_OPSTATE_SYS	0x74013460
#define SSUSB_IP_DEV_PDN	0x74013E08
#define SSUSB_U2_PORT_PDN	0x74013E50
#define MISC_CTRL	0x74011C84
#define SSUSB_IP_SLEEP_STS	0x74013E10

/**
 * Chip debug info dump position - USB EP0 Status CRs
 */
#define EPISR	0x74011080
#define EPIER	0x74011084
#define EPISR_MD	0x740110A0
#define EPIER_MD	0x740110A4
#define EPISR_UHW	0x740110B0
#define EPIER_UHW	0x740110B4
#define EP0CSR	0x74011100
#define EP0DISPATCH	0x7401110C
#define SSUSB_IP_SPARE0	0x74013EC8
#define SSUSB_IP_SPARE1	0x74013ECC

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

#if BTMTK_ISOC_TEST
/**
 * ISOC support
 */
#define BT_CHR_DEV_SCO "BT_chrdev_sco"
#define BT_DEV_NODE_SCO        "stpbt_sco"

#define ISOC_IF_ALT_MSBC               4
#define ISOC_IF_ALT_CVSD               2
#define ISOC_IF_ALT_DEFAULT            ISOC_IF_ALT_CVSD
#define ISOC_HCI_PKT_SIZE_MSBC         (33 * 3)
#define ISOC_HCI_PKT_SIZE_CVSD         (17 * 3)
#define ISOC_HCI_PKT_SIZE_DEFAULT      ISOC_HCI_PKT_SIZE_CVSD

#define HCE_DIS_CONN_COMPLETE  0x05
#define HCE_SYNC_CONN_COMPLETE 0x2C
#define HCE_SYNC_CONN_COMPLETE_LEN 19
#define HCE_SYNC_CONN_COMPLETE_AIR_MODE_OFFSET 18
#define HCE_SYNC_CONN_COMPLETE_AIR_MODE_CVSD 0x02
#define HCE_SYNC_CONN_COMPLETE_AIR_MODE_TRANSPARENT 0x03
#define SCO_BUFFER_SIZE                (1024 * 4)      /* Size of RX Queue */


struct btmtk_fops_sco {
	struct btmtk_dev *bdev;
	dev_t g_devIDsco;
	struct cdev BT_cdevsco;
	wait_queue_head_t inq_isoc;
	struct sk_buff_head     isoc_in_queue;
	struct class *pBTClass;
	struct device *pBTDevsco;
	spinlock_t isoc_lock;
	atomic_t isoc_out_count;
	struct semaphore isoc_wr_mtx;
	struct semaphore isoc_rd_mtx;
	unsigned char *o_sco_buf;
	unsigned char isoc_alt_setting;
	unsigned char isoc_urb_submitted;
};
#endif

/**
 * For debug SOP
 */
#define RETRY_CR_BOUNDARY 3
#define POLLING_CR_BOUNDARY 5
#define POLLING_CURRENT_PC 10
#define PSOP_STRING_LEN 10 // EX. PSOP_1_1_A (len = 10)
#define TX3CSR2_TXFIFOADDR 0x74011138
#define RX3CSR2_RXFIFOADDR 0x74011238

enum {
	BTMTK_DBG_DEFAULT_STATE,
	BTMTK_CHECK_DBG_STATUS,
	BTMTK_PDBG_SLAVE_NO_RESPONSE_CONDITIONAL,   // CONDITIONAL: Dump CR with DE flow
	BTMTK_PDBG_SLAVE_NO_RESPONSE_ALL,           // ALL: Dump all CR when DE flow is wrong
	BTMTK_LOW_POWER_CONDITIONAL,
	BTMTK_LOW_POWER_ALL,
	BTMTK_DBG_INFO
};

#endif
