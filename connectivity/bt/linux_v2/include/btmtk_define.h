/*
 *  Copyright (c) 2016,2017 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __BTMTK_DEFINE_H__
#define __BTMTK_DEFINE_H__


#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/device.h>
#include <asm/unaligned.h>

/* Define for proce node */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* Define for whole chip reset */
#include <linux/of.h>
#include <linux/of_gpio.h>

#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/vmalloc.h>
#include <linux/rtc.h>

/** Driver version */
#define VERSION "8.0.2022101901"
#define SUBVER ":connac3_dev"

#ifdef CFG_SUPPORT_WAKEUP_IRQ
#define WAKEUP_BT_IRQ 1
#else
#define WAKEUP_BT_IRQ 0
#endif

#define ENABLESTP FALSE
#define BTMTKUART_TX_STATE_ACTIVE	1
#define BTMTKUART_TX_STATE_WAKEUP	2
#define BTMTK_TX_WAIT_VND_EVT		3
#define BTMTKUART_REQUIRED_WAKEUP	4
#define BTMTKUART_REQUIRED_DOWNLOAD	5
#define BTMTK_TX_SKIP_VENDOR_EVT	6

#define BTMTKUART_RX_STATE_ACTIVE	1
#define BTMTKUART_RX_STATE_WAKEUP	2
#define BTMTKUART_RX_STATE_RESET	3

#define BTMTK_DRV_OWN			0	/* init state is driver own for fw dl */
#define BTMTK_DRV_OWNING		1
#define BTMTK_FW_OWN			2
#define BTMTK_FW_OWNING			3
#define BTMTK_OWN_FAIL			4

#define FW_OWN_TIMEOUT			30

#define FW_OWN_TIMER_UKNOWN		0
#define FW_OWN_TIMER_INIT		1
#define FW_OWN_TIMER_RUNNING		2
#define FW_OWN_TIMER_DONE		3

#define BTMTK_THREAD_STOP		(1 << 0)
#define BTMTK_THREAD_TX			(1 << 1)
#define BTMTK_THREAD_RX			(1 << 2)
#define BTMTK_THREAD_FW_OWN		(1 << 3)
#define BTMTK_THREAD_ASSERT		(1 << 4)

/**
 * Maximum rom patch file name length
 */
#define MAX_BIN_FILE_NAME_LEN 64

/**
 * Type definition
 */
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef UNUSED
	#define UNUSED(x) (void)(x)
#endif

#ifndef ALIGN_4
#define ALIGN_4(_value)             (((_value) + 3) & ~3u)
#endif /* ALIGN_4 */

#ifndef ALIGN_8
#define ALIGN_8(_value)             (((_value) + 7) & ~7u)
#endif /* ALIGN_4 */

/* This macro check the DW alignment of the input value.
 * _value - value of address need to check
 */
#ifndef IS_ALIGN_4
#define IS_ALIGN_4(_value)          (((_value) & 0x3) ? FALSE : TRUE)
#endif /* IS_ALIGN_4 */

#ifndef IS_NOT_ALIGN_4
#define IS_NOT_ALIGN_4(_value)      (((_value) & 0x3) ? TRUE : FALSE)
#endif /* IS_NOT_ALIGN_4 */

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


/**
 * Log and level definition
 */
#define BTMTK_LOG_LVL_ERR	1
#define BTMTK_LOG_LVL_WARN	2
#define BTMTK_LOG_LVL_INFO	3
#define BTMTK_LOG_LVL_DBG	4
#define BTMTK_LOG_LVL_MAX	BTMTK_LOG_LVL_DBG
#define BTMTK_LOG_LVL_DEF	BTMTK_LOG_LVL_INFO	/* default setting */

#define SEPARATOR_LEN 2
#define HCI_SNOOP_ENTRY_NUM	30
#define HCI_SNOOP_BUF_SIZE	32
#define HCI_SNOOP_MAX_BUF_SIZE	(HCI_SNOOP_BUF_SIZE * 2 + SEPARATOR_LEN)
#define HCI_SNOOP_TS_STR_LEN	50
#define WMT_OVER_HCI_HEADER_SIZE	3
#define READ_ISO_PACKET_CMD_SIZE	4

extern uint8_t btmtk_log_lvl;

#define BTMTK_ERR(fmt, ...)	 \
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_ERR) pr_info("[btmtk_err] ***"fmt"***\n", ##__VA_ARGS__); } while (0)
#define BTMTK_WARN(fmt, ...)	\
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_WARN) pr_info("[btmtk_warn] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTMTK_INFO(fmt, ...)	\
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_INFO) pr_info("[btmtk_info] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTMTK_DBG(fmt, ...)	 \
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG) pr_info("[btmtk_dbg] "fmt"\n", ##__VA_ARGS__); } while (0)

#define BTMTK_ERR_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_ERR)	\
			printk_ratelimited(KERN_WARNING "[btmtk_err_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

#define BTMTK_WARN_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_WARN)	\
			printk_ratelimited(KERN_WARNING "[btmtk_warn_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

#define BTMTK_DBG_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG)	\
			printk_ratelimited(KERN_WARNING "[btmtk_dbg_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

#define BTMTK_INFO_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_INFO)	\
			printk_ratelimited(KERN_WARNING "[btmtk_info_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

#define BTMTK_INFO_RAW(p, l, fmt, ...)						\
	do {	\
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_INFO) {	\
			int cnt_ = 0;	\
			int len_ = (l <= HCI_SNOOP_MAX_BUF_SIZE ? l : HCI_SNOOP_MAX_BUF_SIZE);	\
			uint8_t raw_buf[HCI_SNOOP_MAX_BUF_SIZE * 3 + 10];	\
			const unsigned char *ptr = p;	\
			for (cnt_ = 0; cnt_ < len_; ++cnt_)	\
				(void)snprintf(raw_buf+3*cnt_, 4, "%02X ", ptr[cnt_]);	\
			raw_buf[3*cnt_] = '\0';	\
			if (l <= HCI_SNOOP_MAX_BUF_SIZE) {	\
				pr_info("[btmtk_info] "fmt" %s\n", ##__VA_ARGS__, raw_buf);	\
			} else {	\
				pr_info("[btmtk_info] "fmt" %s (partial)\n", ##__VA_ARGS__, raw_buf);	\
			}	\
		}	\
	} while (0)

// dbg level print partial
#define BTMTK_DBG_RAW(p, l, fmt, ...)						\
	do {	\
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG) {	\
			int cnt_ = 0;	\
			int len_ = (l <= HCI_SNOOP_MAX_BUF_SIZE ? l : HCI_SNOOP_MAX_BUF_SIZE);	\
			uint8_t raw_buf[HCI_SNOOP_MAX_BUF_SIZE * 3 + 10];	\
			const unsigned char *ptr = p;	\
			for (cnt_ = 0; cnt_ < len_; ++cnt_) \
				(void)snprintf(raw_buf+3*cnt_, 4, "%02X ", ptr[cnt_]);	\
			raw_buf[3*cnt_] = '\0'; \
			if (l <= HCI_SNOOP_MAX_BUF_SIZE) {	\
				pr_info("[btmtk_debug] "fmt" %s\n", ##__VA_ARGS__, raw_buf); \
			} else {	\
				pr_info("[btmtk_debug] "fmt" %s (partial)\n", ##__VA_ARGS__, raw_buf);	\
			}	\
		}	\
	} while (0)

#if 0 // dbg level print all data
#define BTMTK_DBG_RAW(p, l, fmt, ...)						\
	do {	\
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG) {	\
			int cnt_ = 0;	\
			int pos = 0;	\
			int len_ = (l <= HCI_SNOOP_MAX_BUF_SIZE ? l : HCI_SNOOP_MAX_BUF_SIZE);	\
			uint8_t raw_buf[HCI_SNOOP_MAX_BUF_SIZE * 3 + 10];	\
			const unsigned char *ptr = p;	\
			for (pos = 0; pos < l; ) {	\
				for (cnt_ = 0; cnt_ < len_ && pos < l; ++cnt_, ++pos) {	\
					(void)snprintf(raw_buf+3*cnt_, 4, "%02X ", ptr[pos]);	\
				}	\
				raw_buf[3*cnt_] = '\0';	\
				pr_warn("[btmtk_debug] "fmt" %s\n", ##__VA_ARGS__, raw_buf);	\
			}	\
		}	\
	} while (0)
#endif

#define BTMTK_CIF_IS_NULL(bdev, cif_event) \
	(!bdev || !(&bdev->cif_state[cif_event]))

/**
 *
 * HCI packet type
 */
#define MTK_HCI_COMMAND_PKT		0x01
#define MTK_HCI_ACLDATA_PKT		0x02
#define MTK_HCI_SCODATA_PKT		0x03
#define MTK_HCI_EVENT_PKT		0x04
#define HCI_ISO_PKT			0x05
#define HCI_ISO_PKT_HEADER_SIZE	4
#define HCI_ISO_PKT_WITH_ACL_HEADER_SIZE	5

/**
 * ROM patch related
 */
#define PATCH_HCI_HEADER_SIZE	4
#define PATCH_WMT_HEADER_SIZE	5
/*
 * Enable STP
 * HCI+WMT+STP = 4 + 5 + 1(phase) +(4=STP_HEADER + 2=CRC)
#define PATCH_HEADER_SIZE	16
 */
/*#ifdef ENABLESTP
 * #define PATCH_HEADER_SIZE	(PATCH_HCI_HEADER_SIZE + PATCH_WMT_HEADER_SIZE + 1 + 6)
 * #define UPLOAD_PATCH_UNIT	916
 * #define PATCH_INFO_SIZE		30
 *#else
 */
#define PATCH_HEADER_SIZE	(PATCH_HCI_HEADER_SIZE + PATCH_WMT_HEADER_SIZE + 1)
/* TODO, If usb use 901 patch unit size, download patch will timeout
 * because the timeout has been set to 1s
 */
#define UPLOAD_PATCH_UNIT	1988
#define PATCH_INFO_SIZE		30
/*#endif*/
#define PATCH_PHASE1		1
#define PATCH_PHASE2		2
#define PATCH_PHASE3		3

/* It is for mt7961 download rom patch*/
#define FW_ROM_PATCH_HEADER_SIZE	32
#define FW_ROM_PATCH_GD_SIZE	64
#define FW_ROM_PATCH_SEC_MAP_SIZE	64
#define SEC_MAP_NEED_SEND_SIZE	52
#define PATCH_STATUS	7

#define DOWNLOAD_BY_INDEX	0
#define DOWNLOAD_BY_TYPE	1
#define BT_BIN_TYP_NUM	6

#define IO_BUF_SIZE		(HCI_MAX_EVENT_SIZE > 256 ? HCI_MAX_EVENT_SIZE : 256)
#define EVENT_COMPARE_SIZE	64

#define SECTION_SPEC_NUM	13

#define ASSERT_REASON_SIZE 255

#define BD_ADDRESS_SIZE 6
#define PHASE1_WMT_CMD_COUNT 255
#define VENDOR_CMD_COUNT 255

#define BT_CFG_NAME "bt.cfg"
#define BT_CFG_NAME_PREFIX "bt_mt"
#define BT_CFG_NAME_SUFFIX "cfg"
#define WOBLE_CFG_NAME_PREFIX "woble_setting"
#define WOBLE_CFG_NAME_SUFFIX "bin"
#define BT_UNIFY_WOBLE "SUPPORT_UNIFY_WOBLE"
#define BT_UNIFY_WOBLE_TYPE "UNIFY_WOBLE_TYPE"
#define BT_WOBLE_BY_EINT "SUPPORT_WOBLE_BY_EINT"
#define BT_DONGLE_RESET_PIN "BT_DONGLE_RESET_GPIO_PIN"
#define BT_RESET_DONGLE "SUPPORT_DONGLE_RESET"
#define BT_FULL_FW_DUMP "SUPPORT_FULL_FW_DUMP"
#define BT_WOBLE_WAKELOCK "SUPPORT_WOBLE_WAKELOCK"
#define BT_WOBLE_FOR_BT_DISABLE "SUPPORT_WOBLE_FOR_BT_DISABLE"
#define BT_RESET_STACK_AFTER_WOBLE "RESET_STACK_AFTER_WOBLE"
#define BT_AUTO_PICUS "SUPPORT_AUTO_PICUS"
#define BT_AUTO_PICUS_FILTER "PICUS_FILTER_COMMAND"
#define BT_AUTO_PICUS_ENABLE "PICUS_ENABLE_COMMAND"
#define BT_PICUS_TO_HOST "SUPPORT_PICUS_TO_HOST"
#define BT_PHASE1_WMT_CMD "PHASE1_WMT_CMD"
#define BT_VENDOR_CMD "VENDOR_CMD"
#define BT_SINGLE_SKU "SUPPORT_BT_SINGLE_SKU"
#define BT_AUDIO_SET "SUPPORT_BT_AUDIO_SETTING"
#define BT_AUDIO_ENABLE_CMD "AUDIO_ENABLE_CMD"
#define BT_AUDIO_PINMUX_NUM "AUDIO_PINMUX_NUM"
#define BT_AUDIO_PINMUX_MODE "AUDIO_PINMUX_MODE"

#define PM_KEY_BTW (0x0015) /* Notify PM the unify woble type */

#define BTMTK_RESET_DOING 1
#define BTMTK_RESET_DONE 0
#define BTMTK_MAX_SUBSYS_RESET_COUNT 3

/**
 * Disable RESUME_RESUME
 */
#ifndef BT_DISABLE_RESET_RESUME
#define BT_DISABLE_RESET_RESUME 0
#endif

enum fw_cfg_index_len {
	FW_CFG_INX_LEN_NONE = 0,
	FW_CFG_INX_LEN_2 = 2,
	FW_CFG_INX_LEN_3 = 3,
};

struct fw_cfg_struct {
	char	*content;	/* APCF content or radio off content */
	u32	length;		/* APCF content or radio off content of length */
};

struct bt_cfg_struct {
	bool	support_unify_woble;	/* support unify woble or not */
	bool	support_woble_by_eint;		/* support woble by eint or not */
	bool	support_dongle_reset;		/* support chip reset or not */
	bool	support_full_fw_dump;		/* dump full fw coredump or not */
	bool	support_woble_wakelock;		/* support when woble error, do wakelock or not */
	bool	support_woble_for_bt_disable;		/* when bt disable, support enter susend or not */
	bool	reset_stack_after_woble;	/* support reset stack to re-connect IOT after resume */
	bool	support_auto_picus;			/* support enable PICUS automatically */
	struct fw_cfg_struct picus_filter;	/* support on PICUS filter command customization */
	struct fw_cfg_struct picus_enable;	/* support on PICUS enable command customization */
	bool	support_picus_to_host;			/* support picus log to host (boots/bluedroid) */
	int	dongle_reset_gpio_pin;		/* BT_DONGLE_RESET_GPIO_PIN number */
	unsigned int	unify_woble_type;	/* 0: legacy. 1: waveform. 2: IR */
	struct fw_cfg_struct phase1_wmt_cmd[PHASE1_WMT_CMD_COUNT];
	struct fw_cfg_struct vendor_cmd[VENDOR_CMD_COUNT];
	bool	support_bt_single_sku;
	bool	support_audio_setting;			/* support audio set pinmux */
	struct fw_cfg_struct audio_cmd;	/* support on audio enable command customization */
	struct fw_cfg_struct audio_pinmux_num;	/* support on set audio pinmux num command customization */
	struct fw_cfg_struct audio_pinmux_mode;	/* support on set audio pinmux mode command customization */
};

struct bt_utc_struct {
	struct rtc_time tm;
	u32 usec;
	u32 sec;
	unsigned long ksec;
	unsigned long knsec;
};

#define BT_DOWNLOAD	1
#define WIFI_DOWNLOAD	2
#define ZB_DOWNLOAD	3

enum debug_reg_index_len {
	DEBUG_REG_INX_LEN_NONE = 0,
	DEBUG_REG_INX_LEN_2 = 2,
	DEBUG_REG_INX_LEN_3 = 3,
};

#define DEBUG_REG_SIZE	10
#define DEBUG_REG_NUM	10

struct debug_reg {
	u32	*content;
	u32	length;
};

struct debug_reg_struct {
	struct debug_reg	*reg;
	u32	num;
};

#define SWAP32(x) \
	((u32) (\
	(((u32) (x) & (u32) 0x000000ffUL) << 24) | \
	(((u32) (x) & (u32) 0x0000ff00UL) << 8) | \
	(((u32) (x) & (u32) 0x00ff0000UL) >> 8) | \
	(((u32) (x) & (u32) 0xff000000UL) >> 24)))

/* Endian byte swapping codes */
#ifdef __LITTLE_ENDIAN
#define cpu2le32(x) ((uint32_t)(x))
#define le2cpu32(x) ((uint32_t)(x))
#define cpu2be32(x) SWAP32((x))
#define be2cpu32(x) SWAP32((x))
#else
#define cpu2le32(x) SWAP32((x))
#define le2cpu32(x) SWAP32((x))
#define cpu2be32(x) ((uint32_t)(x))
#define be2cpu32(x) ((uint32_t)(x))
#endif

#define FW_VERSION	0x80021004
#define CHIP_ID	0x70010200
#define FLAVOR	0x70010020


#ifndef DEBUG_LD_PATCH_TIME
#define DEBUG_LD_PATCH_TIME 0
#endif

#ifndef DEBUG_DUMP_TIME
#define DEBUG_DUMP_TIME 0
#endif

#define ERRNUM 0xFF

void btmtk_getUTCtime(struct bt_utc_struct *utc);

#if DEBUG_DUMP_TIME
#define DUMP_TIME_STAMP(__str) \
	do { \
		struct bt_utc_struct utc; \
		btmtk_getUTCtime(&utc); \
		BTMTK_INFO("%s:%d, %s - DUMP_TIME_STAMP UTC: %d-%02d-%02d %02d:%02d:%02d.%06u", \
			__func__, __LINE__, __str, \
			utc.tm.tm_year, utc.tm.tm_mon, utc.tm.tm_mday, \
			utc.tm.tm_hour, utc.tm.tm_min, utc.tm.tm_sec, utc.usec); \
	} while (0)
#else
#define DUMP_TIME_STAMP(__str)
#endif

#endif /* __BTMTK_DEFINE_H__ */
