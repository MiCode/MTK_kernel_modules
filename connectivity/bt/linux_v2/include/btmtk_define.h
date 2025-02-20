/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
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


#if (KERNEL_VERSION(4, 11, 0) < LINUX_VERSION_CODE)
#include <linux/sched/clock.h>
#endif

#ifdef CFG_SUPPORT_CHIP_RESET_KO
#include "reset.h"
#include "reset_fsm.h"
#include "reset_fsm_def.h"
#endif

/** Driver version */

#define VERSION "8.0.2023122701"
#define SUBVER ":connac3_dev"

#if CFG_SUPPORT_WAKEUP_IRQ
#define WAKEUP_BT_IRQ 1
#else
#define WAKEUP_BT_IRQ 0
#endif

#ifndef BTMTK_RUNTIME_ENABLE
#define BTMTK_RUNTIME_ENABLE 0
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

/* Extend to 60ms to prohibit set fw own while iso streaming.
 * Or iso TX/RX may pass anchor interval as set FW own takes long time.
 * And also FW may assert due to queue full since driver cannot read data in time*/
#define FW_OWN_TIMEOUT			60

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
#if BTMTK_LOG_LVL
#define PRINTF_INFO pr_emerg
#define PRINTK_LIMITE_LVL KERN_EMERG
#define PR_CONT(fmt, ...) printk(KERN_CONT KERN_EMERG fmt, ##__VA_ARGS__)
#else
#define PRINTF_INFO pr_warn
#define PRINTK_LIMITE_LVL KERN_WARNING
#define PR_CONT(fmt, ...) printk(KERN_CONT KERN_WARNING fmt, ##__VA_ARGS__)
#endif

#define BTMTK_ERR(fmt, ...)	 \
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_ERR) PRINTF_INFO("[btmtk_err] ***"fmt"***\n", ##__VA_ARGS__); } while (0)
#define BTMTK_WARN(fmt, ...)	\
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_WARN) PRINTF_INFO("[btmtk_warn] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTMTK_INFO(fmt, ...)	\
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_INFO) PRINTF_INFO("[btmtk_info] "fmt"\n", ##__VA_ARGS__); } while (0)
#define BTMTK_DBG(fmt, ...)	 \
	do { if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG) PRINTF_INFO("[btmtk_dbg] "fmt"\n", ##__VA_ARGS__); } while (0)

#define BTMTK_ERR_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_ERR)	\
			printk_ratelimited(PRINTK_LIMITE_LVL "[btmtk_err_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

#define BTMTK_WARN_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_WARN)	\
			printk_ratelimited(PRINTK_LIMITE_LVL "[btmtk_warn_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

#define BTMTK_DBG_LIMITTED(fmt, ...)	\
	do { \
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG)	\
			printk_ratelimited(PRINTK_LIMITE_LVL "[btmtk_dbg_limit] "fmt"\n", ##__VA_ARGS__);	\
	} while (0)

extern uint8_t _raw_buf_[];
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
			const unsigned char *ptr = p;	\
			for (cnt_ = 0; cnt_ < len_; ++cnt_)	\
				(void)snprintf(_raw_buf_+3*cnt_, 4, "%02X ", ptr[cnt_]);	\
			_raw_buf_[3*cnt_] = '\0';	\
			if (l <= HCI_SNOOP_MAX_BUF_SIZE) {	\
				PR_CONT("[btmtk_info] "fmt" %s\n", ##__VA_ARGS__, _raw_buf_);	\
			} else {	\
				PR_CONT("[btmtk_info] "fmt" %s (partial)\n", ##__VA_ARGS__, _raw_buf_);	\
			}	\
		}	\
	} while (0)

#define BTMTK_DBG_RAW(p, l, fmt, ...)						\
	do {	\
		if (btmtk_log_lvl >= BTMTK_LOG_LVL_DBG) {	\
			int cnt_ = 0;	\
			int len_ = (l <= HCI_SNOOP_MAX_BUF_SIZE ? l : HCI_SNOOP_MAX_BUF_SIZE);	\
			const unsigned char *ptr = p;	\
			for (cnt_ = 0; cnt_ < len_; ++cnt_) \
				(void)snprintf(_raw_buf_+3*cnt_, 4, "%02X ", ptr[cnt_]);	\
			_raw_buf_[3*cnt_] = '\0'; \
			if (l <= HCI_SNOOP_MAX_BUF_SIZE) {	\
				PR_CONT("[btmtk_debug] "fmt" %s\n", ##__VA_ARGS__, _raw_buf_); \
			} else {	\
				PR_CONT("[btmtk_debug] "fmt" %s (partial)\n", ##__VA_ARGS__, _raw_buf_);	\
			}	\
		}	\
	} while (0)

#define BTMTK_CIF_IS_NULL(bdev, cif_event) \
	(!bdev || !(&bdev->cif_state[cif_event]))

/**
 *
 * HCI packet type
 */
#define MTK_HCI_COMMAND_PKT		0x01
#define MTK_HCI_CMD_HEAD_SIZE			4
#define MTK_HCI_ACLDATA_PKT		0x02
#define MTK_HCI_ACL_HEAD_SIZE			5
#define MTK_HCI_SCODATA_PKT		0x03
#define MTK_HCI_SCO_HEAD_SIZE			4
#define MTK_HCI_EVENT_PKT		0x04
#define MTK_HCI_EVENT_HEAD_SIZE			3
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
/* Current BT SDIO config GDMA with 4 bytes, 4 burst. In AHB BUS, burst transfer can no cross 0x400 (1K) boundary
 * In BT driver, FW download using 2048 (original UPLOAD_PATCH_UNIT) = 4 (header) + 2044 (payload),
 * 2044 * 2 = 4088 = 0xFF8, 0xFF8 + 0x10 (4 byte, 4 burst) = 0x1008, in this case,
 * burst transfer accross the 0x400 boundary.
 * If change to 1988 = 4 (header) + 1984 (payload), there are no burst transfer accross the 0x400 boundary,
 * since each burst transfer step is 0x10 and start address always 16 byte aligned.
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
#define FILTER_VENDOR_CMD_COUNT 255

#define BT_CFG_NAME "bt.cfg"
#define BT_CFG_NAME_PREFIX "bt_mt"
#define BT_CFG_NAME_SUFFIX "cfg"
#define WOBLE_CFG_NAME_PREFIX "woble_setting"
#define WOBLE_CFG_NAME_SUFFIX "bin"
#define BT_UNIFY_WOBLE "SUPPORT_UNIFY_WOBLE"
#define BT_UNIFY_WOBLE_TYPE "UNIFY_WOBLE_TYPE"
#define BT_WOBLE_BY_EINT "SUPPORT_WOBLE_BY_EINT"
#define BT_DONGLE_RESET_PIN "BT_DONGLE_RESET_GPIO_PIN"
#define BT_SUBSYS_RESET_DONGLE "SUPPORT_DONGLE_SUBSYS_RESET"
#define BT_WHOLE_RESET_DONGLE "SUPPORT_DONGLE_WHOLE_RESET"
#define BT_FILTER_VENDOR_CMD "FILTER_CMD"
#define BT_WOBLE_WAKELOCK "SUPPORT_WOBLE_WAKELOCK"
#define BT_WOBLE_FOR_BT_DISABLE "SUPPORT_WOBLE_FOR_BT_DISABLE"
#define BT_RESET_STACK_AFTER_WOBLE "RESET_STACK_AFTER_WOBLE"
#define BT_AUTO_PICUS "SUPPORT_AUTO_PICUS"
#define BT_AUTO_PICUS_FILTER "PICUS_FILTER_COMMAND"
#define BT_AUTO_PICUS_ENABLE "PICUS_ENABLE_COMMAND"
#define BT_PICUS_TO_HOST "SUPPORT_PICUS_TO_HOST"
#if BTMTK_ISOC_TEST
#define BT_USB_SCO_TEST "SUPPORT_USB_SCO_TEST"
#endif
#define BT_PHASE1_WMT_CMD "PHASE1_WMT_CMD"
#define BT_VENDOR_CMD "VENDOR_CMD"
#define BT_SINGLE_SKU "SUPPORT_BT_SINGLE_SKU"
#define BT_AUDIO_SET "SUPPORT_BT_AUDIO_SETTING"
#define BT_AUDIO_ENABLE_CMD "AUDIO_ENABLE_CMD"
#define BT_AUDIO_PINMUX_NUM "AUDIO_PINMUX_NUM"
#define BT_AUDIO_PINMUX_MODE "AUDIO_PINMUX_MODE"
#define BT_DEBUG_SOP_MODE "DEBUG_SOP_MODE"
#define DTS_SUBSYS_RST "SUPPORT_DTS_SUBSYS_RST"
#define BT_LOG_LVL "LOG_LVL"
#if CFG_SUPPORT_LEAUDIO_CLK
#define BT_LE_AUDIO_CLK "SUPPORT_LE_AUDIO_CLK"
#define BT_LE_AUDIO_CLK_FW_GPIO "LE_AUDIO_CLK_FW_GPIO"
#endif

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
	bool	support_dongle_subsys_reset;	/*support subsys reset or not*/
	bool	support_dongle_whole_reset;	/*support whole reset or not*/
	bool	support_woble_wakelock;		/* support when woble error, do wakelock or not */
	bool	support_woble_for_bt_disable;		/* when bt disable, support enter susend or not */
	bool	reset_stack_after_woble;	/* support reset stack to re-connect IOT after resume */
	bool	support_auto_picus;			/* support enable PICUS automatically */
	struct fw_cfg_struct picus_filter;	/* support on PICUS filter command customization */
	struct fw_cfg_struct picus_enable;	/* support on PICUS enable command customization */
	bool	support_picus_to_host;			/* support picus log to host (boots/bluedroid) */
#if BTMTK_ISOC_TEST
	bool	support_usb_sco_test;			/* support usb sco test for bluedroid */
#endif
	int	dongle_reset_gpio_pin;		/* BT_DONGLE_RESET_GPIO_PIN number */
	unsigned int	unify_woble_type;	/* 0: legacy. 1: waveform. 2: IR */
	struct fw_cfg_struct phase1_wmt_cmd[PHASE1_WMT_CMD_COUNT];
	struct fw_cfg_struct vendor_cmd[VENDOR_CMD_COUNT];
	bool	support_bt_single_sku;
	struct fw_cfg_struct filter_vendor_cmd[FILTER_VENDOR_CMD_COUNT];
	bool	support_audio_setting;			/* support audio set pinmux */
	struct fw_cfg_struct audio_cmd;	/* support on audio enable command customization */
	struct fw_cfg_struct audio_pinmux_num;	/* support on set audio pinmux num command customization */
	struct fw_cfg_struct audio_pinmux_mode;	/* support on set audio pinmux mode command customization */
	int debug_sop_mode; /* debug sop mode */
	bool	support_dts_subsys_rst;
#if CFG_SUPPORT_LEAUDIO_CLK
	bool	support_le_audio_clk;
	int	le_audio_clk_fw_gpio;
#endif
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

#define ZB_ENABLE	0x7C00114C

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
		BTMTK_INFO("%s: %d, %s - DUMP_TIME_STAMP UTC: %d-%02d-%02d %02d:%02d:%02d.%06u", \
			__func__, __LINE__, __str, \
			utc.tm.tm_year, utc.tm.tm_mon, utc.tm.tm_mday, \
			utc.tm.tm_hour, utc.tm.tm_min, utc.tm.tm_sec, utc.usec); \
	} while (0)
#else
#define DUMP_TIME_STAMP(__str)
#endif

#if CFG_SUPPORT_LEAUDIO_CLK
#if CFG_SUPPORT_DEINT_IRQ
extern int mtk_deint_enable(unsigned int eint_num, unsigned int spi_num, unsigned long type);
extern int mtk_deint_ack(unsigned int irq);
#endif
//#include <mt-plat/mtk_secure_api.h>
//#define MTK_SIP_BT_FIQ_REG (0x82000523 | MTK_SIP_SMC_AARCH_BIT)
//#define MTK_SIP_BT_GET_CLK (0x82000524 | MTK_SIP_SMC_AARCH_BIT)
#endif

#ifdef CFG_SUPPORT_CHIP_RESET_KO
void btmtk_resetko_reset(void);
void btmtk_resetko_notify(unsigned int event, void *data);
extern enum ReturnStatus resetko_register_module(enum ModuleType module,
						char *name,
						enum TriggerResetApiType resetApiType,
						void *resetFunc,
						void *notifyFunc);
extern enum ReturnStatus resetko_unregister_module(enum ModuleType module);
#endif

#endif /* __BTMTK_DEFINE_H__ */
