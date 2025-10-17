/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __BTMTK_MAIN_H__
#define __BTMTK_MAIN_H__
#include "btmtk_define.h"

#define DEFAULT_COUNTRY_TABLE_NAME "btPowerTable.dat"

#ifdef CHIP_IF_USB
#define DEFAULT_DEBUG_SOP_NAME "usb_debug"
#elif defined(CHIP_IF_SDIO)
#define DEFAULT_DEBUG_SOP_NAME "sdio_debug"
#elif defined(CHIP_IF_UART_TTY)
#define DEFAULT_DEBUG_SOP_NAME "uart_debug"
#endif

//static inline struct sk_buff *mtk_add_stp(struct btmtk_dev *bdev, struct sk_buff *skb);

#define hci_dev_test_and_clear_flag(hdev, nr)  test_and_clear_bit((nr), (hdev)->dev_flags)

/* h4_recv */
#define hci_skb_pkt_type(skb) bt_cb((skb))->pkt_type
#define hci_skb_expect(skb) bt_cb((skb))->expect
#define hci_skb_opcode(skb) bt_cb((skb))->hci.opcode

/* HCI bus types */
#define HCI_VIRTUAL	0
#define HCI_USB		1
#define HCI_PCCARD	2
#define HCI_UART	3
#define HCI_RS232	4
#define HCI_PCI		5
#define HCI_SDIO	6
#define HCI_SPI		7
#define HCI_I2C		8
#define HCI_SMD		9

#define HCI_TYPE_SIZE	1

/* this for 7663 need download patch staus
 * 0:
 * patch download is not complete/BT get patch semaphore fail (WiFi get semaphore success)
 * 1:
 * patch download is complete
 * 2:
 * patch download is not complete/BT get patch semaphore success
 */
#define MT766X_PATCH_IS_DOWNLOAD_BY_OTHER 0
#define MT766X_PATCH_READY 1
#define MT766X_PATCH_NEED_DOWNLOAD 2

/* this for 79XX need download patch staus
 * 0:
 * patch download is not complete, BT driver need to download patch
 * 1:
 * patch is downloading by Wifi,BT driver need to retry until status = PATCH_READY
 * 2:
 * patch download is complete, BT driver no need to download patch
 */
#define PATCH_ERR -1
#define PATCH_NEED_DOWNLOAD 0
#define PATCH_IS_DOWNLOAD_BY_OTHER 1
#define PATCH_READY 2

/* 0:
 * using legacy wmt cmd/evt to download fw patch, usb/sdio just support 0 now
 * 1:
 * using DMA to download fw patch
 */
#define PATCH_DOWNLOAD_USING_WMT 0
#define PATCH_DOWNLOAD_USING_DMA 1

#define PATCH_DOWNLOAD_PHASE1_2_DELAY_TIME 1
#define PATCH_DOWNLOAD_PHASE1_2_RETRY 5
#define PATCH_DOWNLOAD_PHASE3_DELAY_TIME 20
#if (USE_DEVICE_NODE == 0)
#define PATCH_DOWNLOAD_PHASE3_RETRY 20
#else
#define PATCH_DOWNLOAD_PHASE3_RETRY 4
#endif
#define PATCH_DOWNLOAD_PHASE3_SECURE_BOOT_DELAY_TIME 200
#define TIME_MULTIPL 1000
#define TIME_US_OFFSET_RANGE 2000
#define TIME_BOUND_OF_REQ_FW	500

/* * delay and retrey for main_send_cmd */
#define WMT_DELAY_TIMES 100
#define DELAY_TIMES 20
#if (USE_DEVICE_NODE == 0)
#define RETRY_TIMES 20
#else
#define RETRY_TIMES 4
#endif
#define SEND_RETRY_ONE_TIMES_500MS 1

#define TX_THREAD_RETRY	100
#define BT_OPEN_MAX_RETRY	160

/* Expected minimum supported interface */
#define BT_MCU_MINIMUM_INTERFACE_NUM	4

/* Bus event */
#define HIF_EVENT_PROBE		0
#define HIF_EVENT_DISCONNECT	1
#define HIF_EVENT_SUSPEND	2
#define HIF_EVENT_RESUME	3
#define HIF_EVENT_STANDBY	4
#define HIF_EVENT_SUBSYS_RESET	5
#define HIF_EVENT_WHOLE_CHIP_RESET	6
#define HIF_EVENT_FW_DUMP	7


#define CHAR2HEX_SIZE	4

/*probe reset flag*/
#define BT_PROBE_FAIL_FOR_SUBSYS_RESET	8
#define BT_PROBE_FAIL_FOR_WHOLE_RESET	9
#define BT_PROBE_RESET_DONE	10
#define BT_PROBE_DO_WHOLE_CHIP_RESET 11

/* woble reset flag */
#define BT_WOBLE_FAIL_FOR_SUBSYS_RESET 12
#define BT_WOBLE_RESET_DONE 13
#define BT_WOBLE_SKIP_WHOLE_CHIP_RESET 14

/**
 * For chip reset pin
 */
#define RESET_PIN_SET_LOW_TIME		100

/* stpbtfwlog setting */
#define FWLOG_QUEUE_COUNT			(400 * BT_MCU_MINIMUM_INTERFACE_NUM)
#define FWLOG_ASSERT_QUEUE_COUNT		45000
#define FWLOG_BLUETOOTH_KPI_QUEUE_COUNT		400
#define HCI_MAX_COMMAND_SIZE			255
#define HCI_CMD_HEADER_SIZE			4
#define HCI_MAX_COMMAND_BUF_SIZE		(HCI_MAX_COMMAND_SIZE * 3)
#ifndef HCI_MAX_ISO_SIZE
#define HCI_MAX_ISO_SIZE	340
#endif

/* fwlog information define */
#define FWLOG_TYPE		0xF0
#define FWLOG_LEN_SIZE		2
#define FWLOG_TL_SIZE		(HCI_TYPE_SIZE + FWLOG_LEN_SIZE)
#define FWLOG_ATTR_TYPE_LEN	1
#define FWLOG_ATTR_LEN_LEN	1
#define FWLOG_ATTR_RX_LEN_LEN	2
#define FWLOG_ATTR_TL_SIZE	(FWLOG_ATTR_TYPE_LEN + FWLOG_ATTR_LEN_LEN)

#define FWLOG_HCI_IDX		0x00
#define FWLOG_DONGLE_IDX	0x01
#define FWLOG_TX		0x10
#define FWLOG_RX		0x11

/* total fwlog info len */
#define FWLOG_PRSV_LEN		32

#define COUNTRY_CODE_LEN	2


#define EDR_MIN		-32
#define EDR_MAX		20
#define EDR_MIN_LV9	13
#define BLE_MIN		-29
#define BLE_MAX		20
#define EDR_MIN_R1	-64
#define EDR_MAX_R1	40
#define EDR_MIN_LV9_R1	26
#define BLE_MIN_R1	-58
#define BLE_MAX_R1	40
#define EDR_MIN_R2	-128
#define EDR_MAX_R2	80
#define EDR_MIN_LV9_R2	52
#define BLE_MIN_R2	-116
#define BLE_MAX_R2	80

#define ERR_PWR		-9999

#define WAIT_POWERKEY_TIMEOUT 5000
#define WAIT_FW_DUMP_TIMEOUT 15000
#define WAIT_DRV_OWN_TIMEOUT 3000

#define STP_CRC_LEN 2
#define TEMP_LEN 260
#define SEARCH_LEN 32
#define TEXT_LEN 128

#define DUAL_BT_FLAG (0x1 << 5)

#define FW_VERSION_BUF_SIZE 256
#define FW_VERSION_KEY_WORDS "t-neptune"

#define OPCODE_LEN 2
#define CMD_NEED_FILTER 1
#define CMD_NO_NEED_FILTER 0
#define FILTER_LIST_OPCODE_1 0
#define FILTER_LIST_OPCODE_2 1
#define FILTER_LIST_COMPLETE_NUM 2
#define FILTER_LIST_STATUS_NUM 3
#define FILTER_OPCODE_NUM 4

#define FW_PATCH_SIZE_MAX (2*1024*1024)

#define WMT_POWER_ON_EVT_RESULT_OFFSET 7
#define READ_EFUSE_CMD_BLOCK_OFFSET 10
#define READ_PINMUX_EVT_REAL_LEN 11
#define FW_COREDUMP_CMD_LEN 4
#define HCI_RESET_CMD_LEN 4
#define READ_ISO_PACKET_SIZE_CMD_HDR_LEN 4

#if BUILD_QA_DBG
#define CFG_SHOW_FULL_MACADDR 1
#define CFG_ENABLE_DEBUG_WRITE 1
#else
#define CFG_SHOW_FULL_MACADDR 0
#define CFG_ENABLE_DEBUG_WRITE 0
#endif

#if CFG_SHOW_FULL_MACADDR
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC2STR(a) ((unsigned char *)a)[0], ((unsigned char *)a)[1], ((unsigned char *)a)[2],\
				((unsigned char *)a)[3], ((unsigned char *)a)[4], ((unsigned char *)a)[5]
#else
#define MACSTR "%02X:%02X:**:**:**:%02X"
#define MAC2STR(a) ((unsigned char *)a)[0], ((unsigned char *)a)[1], ((unsigned char *)a)[5]
#endif

#define DFD_EMI_DUMP_SIZE 0x9000

enum {
	RES_1 = 0,
	RES_DOT_5,
	RES_DOT_25
};

enum {
	CHECK_SINGLE_SKU_PWR_MODE	= 0,
	CHECK_SINGLE_SKU_EDR_MAX,
	CHECK_SINGLE_SKU_BLE,
	CHECK_SINGLE_SKU_BLE_2M,
	CHECK_SINGLE_SKU_BLE_LR_S2,
	CHECK_SINGLE_SKU_BLE_LR_S8,
	CHECK_SINGLE_SKU_ALL
};

enum {
	DISABLE_LV9 = 0,
	ENABLE_LV9
};

enum {
	DIFF_MODE_3DB = 0,
	DIFF_MODE_0DB
};

struct btmtk_cif_state {
	unsigned char ops_enter;
	unsigned char ops_end;
	unsigned char ops_error;
};

enum TX_TYPE {
	BTMTK_TX_CMD_FROM_DRV = 0,		/* send hci cmd and wmt cmd by driver */
	BTMTK_TX_ACL_FROM_DRV,			/* send acl pkt with load rompatch by driver */
	BTMTK_TX_PKT_FROM_HOST,			/* send pkt from host, include acl and hci */
	BTMTK_TX_PKT_SEND_DIRECT,		/* send tx not through tx_thread */
	BTMTK_TX_PKT_SEND_NO_ASSERT,		/* send tx not through tx_thread */
	BTMTK_TX_PKT_SEND_DIRECT_NO_ASSERT,	/* send tx not through tx_thread and not trigger assert */
};

enum bt_state {
	FUNC_OFF = 0,
	TURNING_ON = 1,
	PRE_ON_AFTER_CAL = 2,
	FUNC_ON = 3,
	RESET_START = 4,
	RESET_END = 5
};

struct bt_power_setting {
	int8_t EDR_Max;
	int8_t LV9;
	int8_t DM;
	int8_t IR;
	int8_t BLE_1M;
	int8_t BLE_2M;
	int8_t BLE_LR_S2;
	int8_t BLE_LR_S8;
	char country_code[COUNTRY_CODE_LEN + 1];
};

enum {
	BTMTK_DONGLE_STATE_UNKNOWN,
	BTMTK_DONGLE_STATE_POWER_ON,
	BTMTK_DONGLE_STATE_POWER_OFF,
	BTMTK_DONGLE_STATE_ERROR,
};

enum {
	HW_ERR_NONE = 0x00,
	HW_ERR_CODE_CHIP_RESET = 0xF0,
	HW_ERR_CODE_USB_DISC = 0xF1,
	HW_ERR_CODE_CORE_DUMP = 0xF2,
	HW_ERR_CODE_POWER_ON = 0xF3,
	HW_ERR_CODE_POWER_OFF = 0xF4,
	HW_ERR_CODE_SET_SLEEP_CMD = 0xF5,
	HW_ERR_CODE_RESET_STACK_AFTER_WOBLE = 0xF6,
};

/* Please keep sync with btmtk_set_state function */
enum {
	BTMTK_STATE_UNKNOWN = 0,
	BTMTK_STATE_INIT = 1,
	BTMTK_STATE_DISCONNECT,
	BTMTK_STATE_PROBE,
	BTMTK_STATE_WORKING,
	BTMTK_STATE_SUSPEND,
	BTMTK_STATE_RESUME,
	BTMTK_STATE_FW_DUMP,
	BTMTK_STATE_STANDBY,
	BTMTK_STATE_SUBSYS_RESET,
	BTMTK_STATE_SEND_ASSERT,
	BTMTK_STATE_CLOSED,
	BTMTK_STATE_ERR,

	BTMTK_STATE_MSG_NUM
};

/* Please keep sync with btmtk_fops_set_state function */
enum {
	BTMTK_FOPS_STATE_UNKNOWN = 0,
	BTMTK_FOPS_STATE_INIT = 1,
	BTMTK_FOPS_STATE_OPENING,	/* during opening */
	BTMTK_FOPS_STATE_OPENED,	/* open in fops_open */
	BTMTK_FOPS_STATE_CLOSING,	/* during closing */
	BTMTK_FOPS_STATE_CLOSED,	/* closed */

	BTMTK_FOPS_STATE_MSG_NUM
};

enum {
	BTMTK_ASSERT_NONE = 0,
	BTMTK_ASSERT_START,
	BTMTK_ASSERT_END,
};

enum {
	BTMTK_EVENT_COMPARE_STATE_UNKNOWN,
	BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE,
	BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE,
	BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS,
};

enum {
	HCI_SNOOP_TYPE_CMD_STACK = 0,
	HCI_SNOOP_TYPE_CMD_HIF,
	HCI_SNOOP_TYPE_EVT_STACK,
	HCI_SNOOP_TYPE_EVT_HIF,
	HCI_SNOOP_TYPE_ADV_EVT_STACK,
	HCI_SNOOP_TYPE_ADV_EVT_HIF,
	HCI_SNOOP_TYPE_NOCP_EVT_STACK,
	HCI_SNOOP_TYPE_NOCP_EVT_HIF,
	HCI_SNOOP_TYPE_TX_ACL_STACK,
	HCI_SNOOP_TYPE_TX_ACL_HIF,
	HCI_SNOOP_TYPE_RX_ACL_STACK,
	HCI_SNOOP_TYPE_RX_ACL_HIF,
	HCI_SNOOP_TYPE_TX_ISO_STACK,
	HCI_SNOOP_TYPE_TX_ISO_HIF,
	HCI_SNOOP_TYPE_RX_ISO_STACK,
	HCI_SNOOP_TYPE_RX_ISO_HIF,
	HCI_SNOOP_TYPE_MAX
};

enum {
	DEBUG_SOP_SLEEP,
	DEBUG_SOP_WAKEUP,
	DEBUG_SOP_NO_RESPONSE,

	DEBUG_SOP_NONE
};

struct dump_debug_cr {
	u32 addr_w;
	u32 value_w;
	u32 addr_r;
};

#define CAL_DATA_PACKET_NUM 10
struct cali_data_s {
	struct fw_cfg_struct cal_data[CAL_DATA_PACKET_NUM];
	u32 num;
};

struct h4_recv_pkt {
	u8  type;	/* Packet type */
	u8  hlen;	/* Header length (not include pkt type)*/
	u8  loff;	/* Data length offset in header (not include pkt type)*/
	u8  lsize;	/* Data length field size */
	u16 maxlen;	/* Max overall packet length */
	int (*recv)(struct hci_dev *hdev, struct sk_buff *skb);
};

#pragma pack(1)
struct _PATCH_HEADER {
	u8 ucDateTime[16];
	u8 ucPlatform[4];
	u16 u2HwVer;
	u16 u2SwVer;
	u32 u4MagicNum;
};

struct _Global_Descr {
	u32 u4PatchVer;
	u32 u4SubSys;
	u32 u4FeatureOpt;
	u32 u4SectionNum;
};

struct _Section_Map {
	u32 u4SecType;
	u32 u4SecOffset;
	u32 u4SecSize;
	union {
		u32 u4SecSpec[SECTION_SPEC_NUM];
		struct {
			u32 u4DLAddr;
			u32 u4DLSize;
			u32 u4SecKeyIdx;
			u32 u4AlignLen;
			u32 u4SecType;
			u32 u4DLModeCrcType;
			u32 u4Crc;
			u32 reserved[6];
		} bin_info_spec;
	};
};
#pragma pack()

#if (USE_DEVICE_NODE == 1)
#define BT_HCI_MAX_FRAME_SIZE	4096


#define RHW_WRITE_TYPE		0x40
#define RHW_READ_TYPE		0x41

#define RHW_VAL_LEN		4
#define RHW_ADDR_LEN		4
#define RHW_PKT_HDR_LEN		4
#define RHW_PKT_LEN			13
#define RHW_PKT_COMP_LEN	9
#define RHW_ADDR_OFFSET_CMD	5
#define RHW_VAL_OFFSET_CMD	9

#define H4_RECV_RHW_WRITE \
	.type = RHW_WRITE_TYPE, \
	.hlen = RHW_PKT_HDR_LEN, \
	.loff = 2, \
	.lsize = 2, \
	.maxlen = RHW_PKT_LEN

#define H4_RECV_RHW_READ \
	.type = RHW_READ_TYPE, \
	.hlen = RHW_PKT_HDR_LEN, \
	.loff = 2, \
	.lsize = 2, \
	.maxlen = RHW_PKT_LEN

#define H4_RECV_ACL \
	.type = HCI_ACLDATA_PKT, \
	.hlen = HCI_ACL_HDR_SIZE, \
	.loff = 2, \
	.lsize = 2, \
	.maxlen = BT_HCI_MAX_FRAME_SIZE

#else // (USE_DEVICE_NODE == 0)
#define H4_RECV_ACL \
	.type = HCI_ACLDATA_PKT, \
	.hlen = HCI_ACL_HDR_SIZE, \
	.loff = 2, \
	.lsize = 2, \
	.maxlen = HCI_MAX_FRAME_SIZE
#endif

#define H4_RECV_SCO \
	.type = HCI_SCODATA_PKT, \
	.hlen = HCI_SCO_HDR_SIZE, \
	.loff = 2, \
	.lsize = 1, \
	.maxlen = HCI_MAX_SCO_SIZE

#define H4_RECV_EVENT \
	.type = HCI_EVENT_PKT, \
	.hlen = HCI_EVENT_HDR_SIZE, \
	.loff = 1, \
	.lsize = 1, \
	.maxlen = HCI_MAX_EVENT_SIZE

/* yumin todo */
// TODO: replace by kernel constant if kernel support new spec
#define HCI_ISODATA_PKT		0x05
#define HCI_ISO_HDR_SIZE	4
#define H4_RECV_ISO \
	.type = HCI_ISODATA_PKT, \
	.hlen = HCI_ISO_HDR_SIZE, \
	.loff = 2, \
	.lsize = 2, \
	.maxlen = HCI_MAX_FRAME_SIZE

#define STP_MAX_HDR_LEN 6
#define STP_MAX_PKT_LEN 2048
#define STP_HDR_PREFIX 0x80
#define STP_HDR_OFFSET 2
struct btmtk_stp_info {
	int dlen;
	int cursor;
	unsigned char data[STP_MAX_HDR_LEN];
};

#if CFG_SUPPORT_LEAUDIO_CLK
struct le_audio {
	int irq;
	u32 irq_type;
	u32 irq_cnt;
	u64 sysclk;
};
#endif

struct btmtk_dev {
	struct hci_dev	*hdev;
	unsigned long	hdev_flags;
	unsigned long	flags;
	void *intf_dev;
	void *cif_dev;
	void *buffer_mode;

	struct work_struct	work;
	struct work_struct	waker;
	struct work_struct	reset_waker;
	struct work_struct	hif_dump_work;

	struct timer_list chip_reset_timer;

	int	recv_evt_len;
	int	tx_in_flight;
	spinlock_t	txlock;
	spinlock_t	rxlock;

	struct sk_buff	*evt_skb;
	struct sk_buff	*sco_skb;

	/* For ble iso packet size */
	int iso_threshold;

	unsigned int	sco_num;
	int	isoc_altsetting;

	int	suspend_count;
	bool	suspend_state;
	int	need_compare_num;

	/* pre-cal flag */
	bool	is_pre_cal_done;
	bool	is_whole_chip_reset;

	/* For tx queue */
	unsigned long	tx_state;

	/* For rx queue */
	struct workqueue_struct	*workqueue;
	struct sk_buff_head	rx_q;
	struct work_struct	rx_work;
	struct sk_buff		*rx_skb;

	wait_queue_head_t	p_wait_event_q;
	wait_queue_head_t	p_woble_fail_q;
	wait_queue_head_t	probe_fail_wq;
	wait_queue_head_t	compare_event_wq;

	/* assert */
	char	assert_reason[ASSERT_REASON_SIZE];
	unsigned int	subsys_reset;
	unsigned int	chip_reset;
	atomic_t	assert_state;

	unsigned char	*rom_patch_bin_file_name;
	unsigned int	chip_id;
	unsigned int	flavor;
	const char	*flavor_bin;
	bool		is_eap;
	unsigned int	dualBT;
	unsigned int	proj;
	unsigned int	fw_version;
	unsigned char	dongle_index;
	unsigned char	power_state;
	unsigned char	fops_state;
	unsigned char	interface_state;
	unsigned char	blank_state;
	struct btmtk_cif_state *cif_state;

	/* io buffer for usb control transfer */
	unsigned char	*io_buf;

	unsigned char	*setting_file;
	unsigned char	bdaddr[BD_ADDRESS_SIZE];

	unsigned char		*bt_cfg_file_name;
	struct bt_cfg_struct	bt_cfg;

	/* single sku */
	unsigned char		*country_file_name;

	int get_hci_reset;

	bool	collect_fwdump;

	/* debug sop */
	struct debug_reg_struct debug_sop_reg_dump;
	unsigned char		debug_sop_file_name[MAX_BIN_FILE_NAME_LEN];

	struct _Section_Map	*sectionMap_table;

	/* raw data cmd & event */
	unsigned char		chip_data_file_name[MAX_BIN_FILE_NAME_LEN];
	void *bt_raw_data;

	/* dynamic fw download */
	struct work_struct  dynamic_fwdl_work;
	unsigned int		fw_bin_info;
        bool dynamic_fwdl_start;
	struct cali_data_s cali_backup;

#if (USE_DEVICE_NODE == 1)
	/* asynchronize tx/rx */
	struct work_struct  async_trx_work;

	/* UDS work for only wifi on*/
	struct work_struct  pwr_on_uds_work;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	struct gpio_desc *gpio_cs;
#else
	/* fw wakeup host irq */
	int	wakeup_irq;
#endif

	/* DFD reset_type */
	unsigned int reset_type;
	void __iomem *dfd_value_addr_remap_base;
	bool is_dfd_done;
	u64	dfd_value_addr;
	u32	dfd_value_size;
	char temp_buf[DFD_EMI_DUMP_SIZE];
	u8 dfd_pstd[DFD_EMI_DUMP_SIZE];

	/* BLE Finding function */
	atomic_t poweroff_finder_mode;
#endif
	/* completion */
	struct completion	dump_comp;
	struct completion	dump_dfd_comp;

	unsigned int on_fail_count;

#ifdef CHIP_IF_USB
	unsigned char	chip_reset_signal;
#endif

	struct btmtk_stp_info stp_info;
	int dump_cnt;

#if CFG_SUPPORT_LEAUDIO_CLK
	struct le_audio le_aud;
#endif
};

#if (USE_DEVICE_NODE == 1)
typedef void (*cif_chrdev_fw_log_state_ptr)(uint8_t state);
#endif

typedef int (*cif_bt_init_ptr)(void);
typedef void (*cif_bt_exit_ptr)(void);
typedef int (*cif_pre_open_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_open_ptr)(struct hci_dev *hdev);
typedef void (*cif_open_done_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_close_ptr)(struct hci_dev *hdev);
typedef int (*cif_reg_read_ptr)(struct btmtk_dev *bdev, u32 reg, u32 *val);
typedef int (*cif_reg_write_ptr)(struct btmtk_dev *bdev, u32 reg, u32 val);
typedef int (*cif_probe_hdl_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_disc_hdl_ptr)(void);
typedef int (*cif_send_cmd_ptr)(struct btmtk_dev *bdev, struct sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag);
typedef int (*cif_send_and_recv_ptr)(struct btmtk_dev *bdev,
		struct sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag);
typedef int (*cif_event_filter_ptr)(struct btmtk_dev *bdev, struct sk_buff *skb);
typedef int (*cif_subsys_reset_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_whole_reset_ptr)(struct btmtk_dev *bdev);
typedef void (*cif_chip_reset_notify_ptr)(struct btmtk_dev *bdev);
typedef void (*cif_mutex_lock_ptr)(struct btmtk_dev *bdev);
typedef void (*cif_mutex_unlock_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_flush_ptr)(struct btmtk_dev *bdev);
typedef void (*cif_log_init_ptr)(void (*log_event_cb)(void));
typedef void (*cif_log_register_cb_ptr)(void (*func)(void));
typedef ssize_t (*cif_log_read_to_user_ptr)(char __user *buf, size_t count);
typedef unsigned int (*cif_log_get_buf_size_ptr)(void);
typedef void (*cif_log_deinit_ptr)(void);
typedef int (*cif_log_handler_ptr)(u8 *buf, u32 size);
typedef int (*cif_met_log_handler_ptr)(struct btmtk_dev *bdev, u8 *buf, u32 size);
typedef int (*cif_dl_dma_ptr)(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset);
typedef void (*cif_dump_debug_sop_ptr)(struct btmtk_dev *bdev);
typedef void (*cif_dump_hif_debug_sop_ptr)(struct btmtk_dev *bdev);
typedef void (*cif_waker_notify_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_enter_standby_ptr)(void);
typedef int (*cif_set_para_ptr)(struct btmtk_dev *bdev, int val);
typedef void (*cif_trigger_assert_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_write_uhw_register)(struct btmtk_dev *bdev, u32 reg, u32 val);
typedef int (*cif_read_uhw_register)(struct btmtk_dev *bdev, u32 reg, u32 *val);
typedef void (*cif_wakeup_host_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_set_xonv_ptr)(struct btmtk_dev *bdev, u8 *connxo, int nvsz);

struct hif_hook_ptr {
#if (USE_DEVICE_NODE == 1)
	cif_chrdev_fw_log_state_ptr fw_log_state;
	cif_met_log_handler_ptr 	met_log_handler;
#endif
	cif_bt_init_ptr			init;
	cif_bt_exit_ptr			exit;
	cif_pre_open_ptr		pre_open;
	cif_open_ptr			open;
	cif_open_done_ptr		open_done;
	cif_close_ptr			close;
	cif_reg_read_ptr		reg_read;
	cif_reg_write_ptr		reg_write;
	cif_probe_hdl_ptr		probe_handler;
	cif_disc_hdl_ptr		disc_handler;
	cif_send_cmd_ptr		send_cmd;
	cif_send_and_recv_ptr		send_and_recv;
	cif_event_filter_ptr		event_filter;
	cif_subsys_reset_ptr		subsys_reset;
	cif_whole_reset_ptr		whole_reset;
	cif_chip_reset_notify_ptr	chip_reset_notify;
	cif_mutex_lock_ptr		cif_mutex_lock;
	cif_mutex_unlock_ptr		cif_mutex_unlock;
	cif_flush_ptr				flush;
	cif_log_init_ptr			log_init;
	cif_log_register_cb_ptr		log_register_cb;
	cif_log_read_to_user_ptr	log_read_to_user;
	cif_log_get_buf_size_ptr	log_get_buf_size;
	cif_log_deinit_ptr			log_deinit;
	cif_log_handler_ptr			log_handler;
	cif_dl_dma_ptr			dl_dma;
	cif_dump_debug_sop_ptr		dump_debug_sop;
	cif_dump_hif_debug_sop_ptr		dump_hif_debug_sop;
	cif_waker_notify_ptr		waker_notify;
	cif_enter_standby_ptr		enter_standby;
	cif_set_para_ptr		set_para;
	cif_trigger_assert_ptr		trigger_assert;
	cif_wakeup_host_ptr		wakeup_host;
	void				*coredump_handler;
	cif_write_uhw_register		write_uhw_register;
	cif_read_uhw_register		read_uhw_register;
	cif_set_xonv_ptr		set_xonv;
};

/* for calibration */
typedef int (*cif_bt_cal_restore_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_bt_cal_backup_send_ptr)(struct btmtk_dev *bdev);
typedef int (*cif_bt_cal_backup_save_ptr)(struct btmtk_dev *bdev, struct sk_buff *skb);
typedef void (*cif_bt_cal_backup_free_ptr)(struct btmtk_dev *bdev);
/* ******* */
typedef int (*cif_bt_probe_handler)(struct btmtk_dev *bdev);
typedef int (*cif_bt_disc_handler)(struct btmtk_dev *bdev);
typedef int (*cif_bt_load_rom_patch)(struct btmtk_dev *bdev);
typedef void (*cif_bt_recv_error_handler)(struct hci_dev *hdev, const u8 *buf, u32 len, const u8 *dbg_buf, u32 dbg_len);
typedef int (*cif_bt_rx_packet_handler)(struct btmtk_dev *bdev,  struct sk_buff *skb);
typedef int (*cif_bt_dispatch_fwlog)(struct btmtk_dev *bdev, struct sk_buff *skb);
typedef int (*cif_bt_setup_handler)(struct hci_dev *hdev);
typedef int (*cif_bt_flush_handler)(struct hci_dev *hdev);
typedef int (*cif_bt_tx_frame_handler)(struct btmtk_dev *bdev,  struct sk_buff *skb,  u8 *fw_dump);
typedef int (*cif_bt_check_power_status)(struct btmtk_dev *bdev, const uint8_t *cmd, int pkt_type);
typedef int (*cif_bt_open_handler)(struct btmtk_dev *bdev);
typedef int (*cif_bt_close_handler)(struct btmtk_dev *bdev);
typedef int (*cif_bt_set_audio_pinmux)(struct btmtk_dev *bdev);
typedef int (*cif_bt_dl_delay_time)(struct btmtk_dev *bdev);
typedef int (*cif_bt_get_fw_info)(struct btmtk_dev *bdev);
typedef int (*cif_bt_subsys_reset)(struct btmtk_dev *bdev);

#ifdef CHIP_IF_SDIO
typedef int (*cif_bt_read_infra_pc)(void *func, u32 *val);
#endif

struct hif_hook_chip_ptr {
	cif_bt_probe_handler			probe_handler;
	cif_bt_disc_handler				disc_handler;
	cif_bt_load_rom_patch			load_patch;
	cif_bt_recv_error_handler		err_handler;
	cif_bt_rx_packet_handler		rx_handler;
	cif_bt_dispatch_fwlog			dispatch_fwlog;
	cif_bt_setup_handler			bt_setup_handler;
	cif_bt_flush_handler			bt_flush_handler;
	cif_bt_tx_frame_handler			bt_tx_frame_handler;
	cif_bt_check_power_status		bt_check_power_status;
	cif_bt_open_handler				bt_open_handler;
	cif_bt_close_handler			bt_close_handler;
	cif_bt_set_audio_pinmux			bt_set_pinmux;
	cif_bt_get_fw_info				get_fw_info;
	cif_bt_subsys_reset				bt_subsys_reset;

/* for calibration */
	cif_bt_cal_restore_ptr				restore;
	cif_bt_cal_backup_send_ptr			backup_send;
	cif_bt_cal_backup_save_ptr			backup_save;
	cif_bt_cal_backup_free_ptr			backup_free;
/* ******* */

	int dl_delay_time;
	u8 support_woble;
	bool patched;

#ifdef CHIP_IF_SDIO
	cif_bt_read_infra_pc			bt_conn_infra_pc;
#endif

};

struct hci_snoop {
	u8 buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_MAX_BUF_SIZE];
	u8 len[HCI_SNOOP_ENTRY_NUM];
	u16 actual_len[HCI_SNOOP_ENTRY_NUM];
	char timestamp[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_TS_STR_LEN];
	u8 index;
};

struct btmtk_main_info {
	int chip_reset_flag;
	atomic_t subsys_reset;
	atomic_t chip_reset;
	atomic_t subsys_reset_count;
	atomic_t whole_reset_count;
	atomic_t subsys_reset_conti_count;

	u8 reset_stack_flag;
	struct wakeup_source *assert_ws;
	struct wakeup_source *woble_ws;
	struct wakeup_source *eint_ws;
	struct wakeup_source *chip_reset_ws;
#if WAKEUP_BT_IRQ
	struct wakeup_source *irq_ws;
#endif
	struct hif_hook_chip_ptr hif_hook_chip;
	struct hif_hook_ptr hif_hook;
	struct bt_power_setting PWS;
	/* save Hci Snoop for debug*/
	struct hci_snoop snoop[HCI_SNOOP_TYPE_MAX];

	u8 wmt_over_hci_header[WMT_OVER_HCI_HEADER_SIZE];
	u8 read_iso_packet_size_cmd[READ_ISO_PACKET_CMD_SIZE];

	/* record firmware version */
	struct proc_dir_entry *proc_dir;
	char fw_version_str[FW_VERSION_BUF_SIZE];

	/* fw log */
	u8	fw_log_on;
	atomic_t fwlog_ref_cnt;

	u32 find_my_phone_mode;
	u32 find_my_phone_mode_extend;
	int bk_rs_flag;
	u8	dbg_send;
	u8	dbg_send_opcode[2];
};

static inline int is_mt7925(u32 chip_id)
{
	chip_id &= 0xFFFF;
	if (chip_id == 0x7925)
		return 1;
	return 0;
}

static inline int is_mt6639(u32 chip_id)
{
	chip_id &= 0xFFFF;
	if (chip_id == 0x6639)
		return 1;
	return 0;
}

static inline int is_mt7902(u32 chip_id)
{
	chip_id &= 0xFFFF;
	if (chip_id == 0x7902)
		return 1;
	return 0;
}

static inline int is_mt7922(u32 chip_id)
{
	chip_id &= 0xFFFF;
	if (chip_id == 0x7922)
		return 1;
	return 0;
}

static inline int is_mt7961(u32 chip_id)
{
	chip_id &= 0xFFFF;
	if (chip_id == 0x7961)
		return 1;
	return 0;
}

static inline int is_mt66xx(u32 chip_id)
{
	chip_id &= 0xFFFF;
	if (chip_id == 0x6631 || chip_id == 0x6635 || chip_id == 0x6653)
		return 1;
	return 0;
}

static inline int is_connac2(u32 chip_id)
{
	if (is_mt7961(chip_id) || is_mt7922(chip_id) || is_mt7902(chip_id))
		return 1;
	return 0;
}

static inline int is_connac3(u32 chip_id)
{
	if (is_mt6639(chip_id) || is_mt7925(chip_id))
		return 1;
	return 0;
}


/* Get BT whole packet length except hci type */
static inline unsigned int get_pkt_len(unsigned char type, unsigned char *buf)
{
	unsigned int len = 0;

	switch (type) {
	/* Please reference hci header format
	 * AA = len
	 * xx = buf[0]
	 * cmd : 01 xx yy AA + payload
	 * acl : 02 xx yy AA AA + payload
	 * sco : 03 xx yy AA + payload
	 * evt : 04 xx AA + payload
	 * ISO : 05 xx yy AA AA + payload
	 */
	case HCI_COMMAND_PKT:
		len = buf[2] + 3;
		break;
	case HCI_ACLDATA_PKT:
		len = buf[2] + ((buf[3] << 8) & 0xff00) + 4;
		break;
	case HCI_SCODATA_PKT:
		len = buf[2] + 3;
		break;
	case HCI_EVENT_PKT:
		len = buf[1] + 2;
		break;
	case HCI_ISO_PKT:
		len = buf[2] + (((buf[3] & 0x3F) << 8) & 0xff00) + HCI_ISO_PKT_HEADER_SIZE;
		break;
	default:
		len = 0;
	}

	return len;
}

unsigned char btmtk_get_chip_state(struct btmtk_dev *bdev);
void btmtk_set_chip_state(struct btmtk_dev *bdev, unsigned char new_state);
int btmtk_allocate_hci_device(struct btmtk_dev *bdev, int hci_bus_type);
void btmtk_free_hci_device(struct btmtk_dev *bdev, int hci_bus_type);
int btmtk_register_hci_device(struct btmtk_dev *bdev);
int btmtk_deregister_hci_device(struct btmtk_dev *bdev);
int btmtk_recv(struct hci_dev *hdev, const u8 *data, size_t count);
int btmtk_recv_event(struct hci_dev *hdev, struct sk_buff *skb);
int btmtk_recv_acl(struct hci_dev *hdev, struct sk_buff *skb);
int btmtk_recv_iso(struct hci_dev *hdev, struct sk_buff *skb);
#if (USE_DEVICE_NODE == 1)
int btmtk_recv_sco(struct hci_dev *hdev, struct sk_buff *skb);
int btmtk_recv_rhw(struct hci_dev *hdev, struct sk_buff *skb);
#endif
int btmtk_send_init_cmds(struct btmtk_dev *hdev);
int btmtk_send_deinit_cmds(struct btmtk_dev *hdev);
int btmtk_load_rom_patch(struct btmtk_dev *bdev);
struct btmtk_dev *btmtk_get_dev(void);
int btmtk_cap_init(struct btmtk_dev *bdev);
struct btmtk_main_info *btmtk_get_main_info(void);
int btmtk_get_interface_num(void);
int btmtk_reset_power_on(struct btmtk_dev *bdev);

int btmtk_send_hw_err_to_host(struct btmtk_dev *bdev);
void btmtk_free_setting_file(struct btmtk_dev *bdev);

unsigned char btmtk_fops_get_state(struct btmtk_dev *bdev);

void btmtk_save_filter_vendor_cmd(struct sk_buff *skb,
		struct btmtk_dev *bdev, bool flag);
int btmtk_vendor_cmd_filter(struct btmtk_dev *bdev, struct sk_buff *skb);

void btmtk_hci_snoop_save(unsigned int type, const u8 *buf, u32 len);
void btmtk_hci_snoop_print(const u8 *buf, u32 len);
void btmtk_hci_snoop_print_to_log(void);
#if (CFG_GKI_SUPPORT == 0)
void *btmtk_kallsyms_lookup_name(const char *name);
#endif
void btmtk_get_UTC_time_str(char *ts_str);
void btmtk_reg_hif_hook(struct hif_hook_ptr *hook);
int btmtk_main_cif_initialize(struct btmtk_dev *bdev, int hci_bus);
void btmtk_main_cif_uninitialize(struct btmtk_dev *bdev, int hci_bus);
int btmtk_main_cif_disconnect_notify(struct btmtk_dev *bdev, int hci_bus);
int btmtk_load_code_from_bin(u8 **image, char *bin_name,
					 struct device *dev, u32 *code_len, u8 retry);
int btmtk_data_length_check(u8 *data, u16 length, u8 type);
int btmtk_main_send_cmd(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, const uint8_t *event, const int event_len, int delay,
		int retry, int pkt_type, bool flag);
int btmtk_load_code_from_setting_files(char *setting_file_name,
			struct device *dev, u32 *code_len, struct btmtk_dev *bdev);
int btmtk_load_fw_cfg_setting(char *block_name, struct fw_cfg_struct *save_content,
		int counter, u8 *searchcontent, enum fw_cfg_index_len index_length);
int btmtk_send_assert_cmd(struct btmtk_dev *bdev);
void btmtk_free_fw_cfg_struct(struct fw_cfg_struct *fw_cfg, int count);
void btmtk_handle_mutex_lock(struct btmtk_dev *bdev);
void btmtk_handle_mutex_unlock(struct btmtk_dev *bdev);
struct btmtk_dev **btmtk_get_pp_bdev(void);
void btmtk_load_debug_sop_register(char *debug_sop_name, struct device *dev, struct btmtk_dev *bdev);
void btmtk_clean_debug_reg_file(struct btmtk_dev *bdev);
int btmtk_dynamic_load_rom_patch(struct btmtk_dev *bdev, u32 binInfo);
void btmtk_reg_hif_chip_hook(struct hif_hook_chip_ptr *hook);

int btmtk_load_rom_patch_connac3(struct btmtk_dev *bdev, int  patch_flag);
int btmtk_picus_enable(struct btmtk_dev *bdev, int via_uart);
int btmtk_picus_disable(struct btmtk_dev *bdev);
int btmtk_set_audio_setting(struct btmtk_dev *bdev);

int32_t btmtk_set_sleep(struct hci_dev *hdev, u_int8_t need_wait);
int32_t bgfsys_bt_patch_dl(void);
int btmtk_efuse_read(struct btmtk_dev *bdev, u16 addr, u8 *value);
void btmtk_set_country_code_from_wifi(char *code);
void btmtk_assert_wake_lock(void);
void btmtk_assert_wake_unlock(void);
bool btmtk_assert_wake_lock_check(void);
#if (USE_DEVICE_NODE == 1)
int rx_skb_enqueue(struct sk_buff *skb);
int btmtk_chardev_init(void);
void btmtk_connsys_log_init(void (*log_event_cb)(void));
void btmtk_connsys_log_deinit(void);
int btmtk_connsys_log_handler(u8 *buf, u32 size);
ssize_t btmtk_connsys_log_read_to_user(char __user *buf, size_t count);
unsigned int btmtk_connsys_log_get_buf_size(void);
int btmtk_met_log_handler(struct btmtk_dev *bdev, u8 *buf, u32 size);
int32_t btmtk_intcmd_set_fw_log(uint8_t flag);
int btmtk_send_connfem_cmd(struct btmtk_dev *bdev);
int bt_send_frame(struct hci_dev *hdev, struct sk_buff *skb);
int bt_open(struct hci_dev *hdev);
int bt_close(struct hci_dev *hdev);
#endif

#endif /* __BTMTK_MAIN_H__ */
