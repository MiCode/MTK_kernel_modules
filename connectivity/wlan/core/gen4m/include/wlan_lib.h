/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "wlan_lib.h"
 *    \brief  The declaration of the functions of the wlanAdpater objects
 *
 *    Detail description.
 */

#ifndef _WLAN_LIB_H
#define _WLAN_LIB_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "CFG_Wifi_File.h"
#include "rlm_domain.h"
#include "nic_init_cmd_event.h"
#include "fw_dl.h"
#include "queue.h"
#include "cmd_buf.h"
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* These values must sync from Wifi HAL
 * /hardware/libhardware_legacy/include/hardware_legacy/wifi_hal.h
 */
/* Basic infrastructure mode */
#define WIFI_FEATURE_INFRA              (0x0001)
/* Support for 5 GHz Band */
#define WIFI_FEATURE_INFRA_5G           (0x0002)
/* Support for GAS/ANQP */
#define WIFI_FEATURE_HOTSPOT            (0x0004)
/* Wifi-Direct */
#define WIFI_FEATURE_P2P                (0x0008)
/* Soft AP */
#define WIFI_FEATURE_SOFT_AP            (0x0010)
/* Google-Scan APIs */
#define WIFI_FEATURE_GSCAN              (0x0020)
/* Neighbor Awareness Networking */
#define WIFI_FEATURE_NAN                (0x0040)
/* Device-to-device RTT */
#define WIFI_FEATURE_D2D_RTT            (0x0080)
/* Device-to-AP RTT */
#define WIFI_FEATURE_D2AP_RTT           (0x0100)
/* Batched Scan (legacy) */
#define WIFI_FEATURE_BATCH_SCAN         (0x0200)
/* Preferred network offload */
#define WIFI_FEATURE_PNO                (0x0400)
/* Support for two STAs */
#define WIFI_FEATURE_ADDITIONAL_STA     (0x0800)
/* Tunnel directed link setup */
#define WIFI_FEATURE_TDLS               (0x1000)
/* Support for TDLS off channel */
#define WIFI_FEATURE_TDLS_OFFCHANNEL    (0x2000)
/* Enhanced power reporting */
#define WIFI_FEATURE_EPR                (0x4000)
/* Support for AP STA Concurrency */
#define WIFI_FEATURE_AP_STA             (0x8000)
/* Link layer stats collection */
#define WIFI_FEATURE_LINK_LAYER_STATS   (0x10000)
/* WiFi Logger */
#define WIFI_FEATURE_LOGGER             (0x20000)
/* WiFi PNO enhanced */
#define WIFI_FEATURE_HAL_EPNO           (0x40000)
/* RSSI Monitor */
#define WIFI_FEATURE_RSSI_MONITOR       (0x80000)
/* WiFi mkeep_alive */
#define WIFI_FEATURE_MKEEP_ALIVE        (0x100000)
/* ND offload configure */
#define WIFI_FEATURE_CONFIG_NDO         (0x200000)
/* Capture Tx transmit power levels */
#define WIFI_FEATURE_TX_TRANSMIT_POWER  (0x400000)
/* Enable/Disable firmware roaming */
#define WIFI_FEATURE_CONTROL_ROAMING    (0x800000)
/* Support Probe IE white listing */
#define WIFI_FEATURE_IE_WHITELIST       (0x1000000)
/* Support MAC & Probe Sequence Number randomization */
#define WIFI_FEATURE_SCAN_RAND          (0x2000000)
/* Support Tx Power Limit setting */
#define WIFI_FEATURE_SET_TX_POWER_LIMIT (0x4000000)
/* Support Using Body/Head Proximity for SAR */
#define WIFI_FEATURE_USE_BODY_HEAD_SAR  (0x8000000)
/* Support Random P2P MAC */
#define WIFI_FEATURE_P2P_RAND_MAC  (0x80000000)

/* note: WIFI_FEATURE_GSCAN be enabled just for ACTS test item: scanner */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
#define WIFI_HAL_FEATURE_SET ((WIFI_FEATURE_P2P) |\
			      (WIFI_FEATURE_SOFT_AP) |\
			      (WIFI_FEATURE_PNO) |\
			      (WIFI_FEATURE_TDLS) |\
			      (WIFI_FEATURE_RSSI_MONITOR) |\
			      (WIFI_FEATURE_CONTROL_ROAMING) |\
			      (WIFI_FEATURE_SET_TX_POWER_LIMIT) |\
			      (WIFI_FEATURE_P2P_RAND_MAC)\
			      )
#else
#define WIFI_HAL_FEATURE_SET ((WIFI_FEATURE_P2P) |\
			      (WIFI_FEATURE_SOFT_AP) |\
			      (WIFI_FEATURE_PNO) |\
			      (WIFI_FEATURE_TDLS) |\
			      (WIFI_FEATURE_RSSI_MONITOR) |\
			      (WIFI_FEATURE_CONTROL_ROAMING) |\
			      (WIFI_FEATURE_P2P_RAND_MAC)\
			      )
#endif

#define MAX_NUM_GROUP_ADDR		32 /* max number of group addresses */
#define AUTO_RATE_NUM			8
#define AR_RATE_TABLE_ENTRY_MAX		25
#define AR_RATE_ENTRY_INDEX_NULL	0x80
#define MAX_TX_QUALITY_INDEX		4

#define TX_CS_TCP_UDP_GEN	BIT(1)
#define TX_CS_IP_GEN		BIT(0)

#define CSUM_OFFLOAD_EN_TX_TCP	BIT(0)
#define CSUM_OFFLOAD_EN_TX_UDP	BIT(1)
#define CSUM_OFFLOAD_EN_TX_IP	BIT(2)
#define CSUM_OFFLOAD_EN_RX_TCP	BIT(3)
#define CSUM_OFFLOAD_EN_RX_UDP	BIT(4)
#define CSUM_OFFLOAD_EN_RX_IPv4	BIT(5)
#define CSUM_OFFLOAD_EN_RX_IPv6	BIT(6)
#define CSUM_OFFLOAD_EN_TX_MASK	BITS(0, 2)
#define CSUM_OFFLOAD_EN_ALL	BITS(0, 6)

/* TCP, UDP, IP Checksum */
#define RX_CS_FLAG_UNKNOWN_NH	BIT(10)
#define RX_CS_FLAG_FRAGMENT	BIT(9)
#define RX_CS_FLAG_WRONG_IP_LEN	BIT(8)
#define RX_CS_FLAG_NOT_DONE	\
	(RX_CS_FLAG_UNKNOWN_NH | RX_CS_FLAG_FRAGMENT | RX_CS_FLAG_WRONG_IP_LEN)

#define RX_CS_TYPE_UDP		BIT(7)
#define RX_CS_TYPE_TCP		BIT(6)
#define RX_CS_TYPE_IPv6		BIT(5)
#define RX_CS_TYPE_IPv4		BIT(4)

#define RX_CS_STATUS_UDP	BIT(3)
#define RX_CS_STATUS_TCP	BIT(2)
#define RX_CS_STATUS_RESERVED	BIT(1)
#define RX_CS_STATUS_IP		BIT(0)

#define CSUM_NOT_SUPPORTED	0x0

#define TXPWR_USE_PDSLOPE	0

/* NVRAM error code definitions */
#define NVRAM_ERROR_VERSION_MISMATCH        BIT(1)
#define NVRAM_ERROR_INVALID_TXPWR           BIT(2)
#define NVRAM_ERROR_INVALID_DPD             BIT(3)
#define NVRAM_ERROR_INVALID_MAC_ADDR        BIT(4)
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
#define NVRAM_POWER_LIMIT_TABLE_INVALID     BIT(5)
#endif

#define NUM_TC_RESOURCE_TO_STATISTICS       4
#if CFG_SUPPORT_NCHO
#define WLAN_CFG_ARGV_MAX 64
#else
#define WLAN_CFG_ARGV_MAX 20
#endif
#define WLAN_CFG_ARGV_MAX_LONG	22	/* for WOW, 2+20 */
#define WLAN_CFG_ENTRY_NUM_MAX	550	/* max number of wifi.cfg */
#if CFG_SUPPORT_NCHO
#define WLAN_CFG_KEY_LEN_MAX	48	/* include \x00  EOL */
#else
#define WLAN_CFG_KEY_LEN_MAX	32	/* include \x00  EOL */
#endif
#define WLAN_CFG_VALUE_LEN_MAX	128	/* include \x00 EOL */
#define WLAN_CFG_FLAG_SKIP_CB	BIT(0)

#if (CFG_TC10_FEATURE == 1)
#define WLAN_CFG_REC_ENTRY_NUM_MAX 580
#else
#define WLAN_CFG_REC_ENTRY_NUM_MAX 550
#endif

#define WLAN_CFG_SET_CHIP_LEN_MAX 10
#define WLAN_CFG_SET_DEBUG_LEVEL_LEN_MAX 10
#define WLAN_CFG_SET_SW_CTRL_LEN_MAX 10

/* OID timeout (in ms) */
#define WLAN_OID_TIMEOUT_THRESHOLD			2000

/* OID timeout during chip-resetting  (in ms) */
#define WLAN_OID_TIMEOUT_THRESHOLD_IN_RESETTING		300

#define WLAN_OID_NO_ACK_THRESHOLD			3


/* If not setting the priority, 0 is the default */
#define WLAN_THREAD_TASK_PRIORITY			0

/* If not setting the nice, -10 is the default */
#define WLAN_THREAD_TASK_NICE				(-10)

#define WLAN_TX_STATS_LOG_TIMEOUT			30000
#define WLAN_TX_STATS_LOG_DURATION			1500

#define WLAN_TYPE_UNKNOWN 0
#define WLAN_TYPE_LEGACY 1
#define WLAN_TYPE_HE 2
#define WLAN_TYPE_EHT 3

#define WLAN_LEGACY_MAX_BA_SIZE 64
#define WLAN_HE_MAX_BA_SIZE 256
#define WLAN_EHT_MAX_BA_SIZE 1024
/* Add to the negotiated WinSize to cope with Ball Behind after Fall Ahead */
#define WLAN_RX_BA_EXT_SIZE 64
#define WLAN_RX_BA_EXT_MAX_SIZE 256
#define WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN 11454

/* Define for wifi path usage */
#define WLAN_FLAG_2G4_WF0		BIT(0)	/*1: support, 0: NOT support */
#define WLAN_FLAG_5G_WF0		BIT(1)	/*1: support, 0: NOT support */
#define WLAN_FLAG_2G4_WF1		BIT(2)	/*1: support, 0: NOT support */
#define WLAN_FLAG_5G_WF1		BIT(3)	/*1: support, 0: NOT support */
#define WLAN_FLAG_2G4_COANT_SUPPORT	BIT(4)	/*1: support, 0: NOT support */
#define WLAN_FLAG_2G4_COANT_PATH	BIT(5)	/*1: WF1, 0:WF0 */
#define WLAN_FLAG_5G_COANT_SUPPORT	BIT(6)	/*1: support, 0: NOT support */
#define WLAN_FLAG_5G_COANT_PATH		BIT(7)	/*1: WF1, 0:WF0 */

#define WLAN_FLAG_6G_WF0		BIT(0)	/*1: support, 0: NOT support */
#define WLAN_FLAG_6G_WF1		BIT(1)	/*1: support, 0: NOT support */

/* Define concurrent network channel number, using by CNM/CMD */
#define MAX_OP_CHNL_NUM			3

#if (CFG_SUPPORT_CONNAC2X == 1 || CFG_SUPPORT_CONNAC3X == 1)
#define AGG_RANGE_SEL_NUM		15
#else
#define AGG_RANGE_SEL_NUM		7
#endif

#define ACS_AP_RSSI_LEVEL_HIGH		-50
#define ACS_AP_RSSI_LEVEL_LOW		-80
#define ACS_DIRTINESS_LEVEL_HIGH	52
#define ACS_DIRTINESS_LEVEL_MID		40
#define ACS_DIRTINESS_LEVEL_LOW		32

#if CFG_SUPPORT_TPENHANCE_MODE
#define TPENHANCE_SESSION_MAP_LEN	20
#define TPENHANCE_PKT_LATCH_MIN	    10
#define TPENHANCE_PKT_KEEP_MAX	    256
struct TPENHANCE_PKT_MAP {
	uint16_t au2SPort;
	uint16_t au2DPort;
	uint32_t au4Ip;
	uint16_t au2Hit;
};

#endif /* CFG_SUPPORT_TPENHANCE_MODE */

#if CFG_WOW_SUPPORT
#define INVALID_WOW_WAKE_UP_REASON 255
/* HIF suspend should wait for cfg80211 suspend done.
 * by experience, 5ms is enough, and worst case ~= 250ms.
 * if > 250 ms --> treat as no cfg80211 suspend
 */
#define HIF_SUSPEND_MAX_WAIT_TIME 50 /* unit: 5ms */
#endif

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
/* Default ED threshold -65 dBm */
#define ED_CCA_BW20_2G_DEFAULT (-65)
#define ED_CCA_BW20_5G_DEFAULT (-65)
#endif

#define WLAN_DRV_READY_CHECK_WLAN_ON       BIT(0)
#define WLAN_DRV_READY_CHECK_HIF_SUSPEND   BIT(1)
#define WLAN_DRV_READY_CHECK_RESET         BIT(2)

#define MAX_CMD_ITEM_MAX		4	/* Max item per cmd. */
#define MAX_CMD_NAME_MAX_LENGTH		32	/* Max name string length */
#define MAX_CMD_VALUE_MAX_LENGTH	32	/* Max value string length */
#define MAX_CMD_TYPE_LENGTH		1
#define MAX_CMD_STRING_LENGTH		1
#define MAX_CMD_VALUE_LENGTH		1
#define MAX_CMD_RESERVE_LENGTH		1

#define CMD_FORMAT_V1_LENGTH	\
	(MAX_CMD_NAME_MAX_LENGTH + MAX_CMD_VALUE_MAX_LENGTH + \
	MAX_CMD_TYPE_LENGTH + MAX_CMD_STRING_LENGTH + MAX_CMD_VALUE_LENGTH + \
	MAX_CMD_RESERVE_LENGTH)

#define MAX_CMD_BUFFER_LENGTH	(CMD_FORMAT_V1_LENGTH * MAX_CMD_ITEM_MAX)

#if 1
#define ED_STRING_SITE		0
#define ED_VALUE_SITE		1


#else
#define ED_ITEMTYPE_SITE	0
#define ED_STRING_SITE		1
#define ED_VALUE_SITE		2
#endif

enum CMD_VER {
	CMD_VER_1,	/* Type[2]+String[32]+Value[32] */
	CMD_VER_1_EXT
};


#if 0
enum ENUM_AIS_REQUEST_TYPE {
	AIS_REQUEST_SCAN,
	AIS_REQUEST_RECONNECT,
	AIS_REQUEST_ROAMING_SEARCH,
	AIS_REQUEST_ROAMING_CONNECT,
	AIS_REQUEST_REMAIN_ON_CHANNEL,
	AIS_REQUEST_NUM
};
#endif
enum CMD_TYPE {
	CMD_TYPE_QUERY,
	CMD_TYPE_SET
};

enum WLAN_CFG_TPYE {
	 WLAN_CFG_DEFAULT = 0x00,
	 WLAN_CFG_REC = 0x01,
	 WLAN_CFG_EM = 0x02,
	 WLAN_CFG_NUM
};

enum POWER_ACTION_CATEGORY {
	SKU_POWER_LIMIT_CTRL = 0x0,
	PERCENTAGE_CTRL = 0x1,
	PERCENTAGE_DROP_CTRL = 0x2,
	BACKOFF_POWER_LIMIT_CTRL = 0x3,
	POWER_LIMIT_TABLE_CTRL = 0x4,
	RF_TXANT_CTRL = 0x5,
	ATEMODE_CTRL = 0x6,
	TX_POWER_SHOW_INFO = 0x7,
	TPC_FEATURE_CTRL = 0x8,
	MU_TX_POWER_CTRL = 0x9,
	BF_NDPA_TXD_CTRL = 0xa,
	TSSI_WORKAROUND = 0xb,
	THERMAL_COMPENSATION_CTRL = 0xc,
	TX_RATE_POWER_CTRL = 0xd,
	TXPOWER_UP_TABLE_CTRL = 0xe,
	TX_POWER_SET_TARGET_POWER = 0xf,
	TX_POWER_GET_TARGET_POWER = 0x10,
	POWER_LIMIT_TX_PWR_ENV_CTRL = 0x11,
	POWER_ACTION_NUM
};

#define ITEM_TYPE_DEC	1
#define ITEM_TYPE_HEX	2
#define ITEM_TYPE_STR	3

enum CMD_DEFAULT_SETTING_VALUE {
	CMD_PNO_ENABLE,
	CMD_PNO_SCAN_PERIOD,
	CMD_SCN_CHANNEL_PLAN,
	CMD_SCN_DWELL_TIME,
	CMD_SCN_STOP_SCAN,
	CMD_MAX,
};

enum CMD_DEFAULT_STR_SETTING_VALUE {
	CMD_STR_TEST_STR,
	CMD_STR_MAX,
};

struct CMD_FORMAT_V1 {
	uint8_t itemType;
	uint8_t itemStringLength;
	uint8_t itemValueLength;
	uint8_t Reserved;
	uint8_t itemString[MAX_CMD_NAME_MAX_LENGTH];
	uint8_t itemValue[MAX_CMD_VALUE_MAX_LENGTH];
};

struct CMD_HEADER {
	enum CMD_VER	cmdVersion;
	enum CMD_TYPE	cmdType;
	uint8_t	itemNum;
	uint16_t	cmdBufferLen;
	uint8_t	buffer[MAX_CMD_BUFFER_LENGTH];
};

struct CFG_DEFAULT_SETTING_TABLE {
	uint32_t itemNum;
	const char *String;
	uint8_t itemType;
	uint32_t defaultValue;
	uint32_t minValue;
	uint32_t maxValue;
};

struct CFG_DEFAULT_SETTING_STR_TABLE {
	uint32_t itemNum;
	const char *String;
	uint8_t itemType;
	const char *DefString;
	uint16_t minLen;
	uint16_t maxLen;
};

struct CFG_QUERY_FORMAT {
	uint32_t Length;
	uint32_t Value;
	uint32_t Type;
	uint32_t *ptr;
};

/*Globol Configure define */
struct CFG_SETTING {
	uint8_t	PnoEnable;
	uint32_t PnoScanPeriod;
	uint8_t ScnChannelPlan;
	uint16_t ScnDwellTime;
	uint8_t ScnStopScan;
	uint8_t TestStr[80];
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
typedef uint32_t(*PFN_OID_HANDLER_FUNC) (struct ADAPTER *prAdapter,
					 void *pvBuf, uint32_t u4BufLen,
					 uint32_t *pu4OutInfoLen);

enum ENUM_CSUM_TYPE {
	CSUM_TYPE_IPV4,
	CSUM_TYPE_IPV6,
	CSUM_TYPE_TCP,
	CSUM_TYPE_UDP,
	CSUM_TYPE_NUM
};

enum ENUM_CSUM_RESULT {
	CSUM_RES_NONE,
	CSUM_RES_SUCCESS,
	CSUM_RES_FAILED,
	CSUM_RES_NUM
};

enum ENUM_PHY_MODE {
	ENUM_PHY_2G4_CCK,
	ENUM_PHY_2G4_OFDM_BPSK,
	ENUM_PHY_2G4_OFDM_QPSK,
	ENUM_PHY_2G4_OFDM_16QAM,
	ENUM_PHY_2G4_OFDM_48M,
	ENUM_PHY_2G4_OFDM_54M,
	ENUM_PHY_2G4_HT20_BPSK,
	ENUM_PHY_2G4_HT20_QPSK,
	ENUM_PHY_2G4_HT20_16QAM,
	ENUM_PHY_2G4_HT20_MCS5,
	ENUM_PHY_2G4_HT20_MCS6,
	ENUM_PHY_2G4_HT20_MCS7,
	ENUM_PHY_2G4_HT40_BPSK,
	ENUM_PHY_2G4_HT40_QPSK,
	ENUM_PHY_2G4_HT40_16QAM,
	ENUM_PHY_2G4_HT40_MCS5,
	ENUM_PHY_2G4_HT40_MCS6,
	ENUM_PHY_2G4_HT40_MCS7,
	ENUM_PHY_5G_OFDM_BPSK,
	ENUM_PHY_5G_OFDM_QPSK,
	ENUM_PHY_5G_OFDM_16QAM,
	ENUM_PHY_5G_OFDM_48M,
	ENUM_PHY_5G_OFDM_54M,
	ENUM_PHY_5G_HT20_BPSK,
	ENUM_PHY_5G_HT20_QPSK,
	ENUM_PHY_5G_HT20_16QAM,
	ENUM_PHY_5G_HT20_MCS5,
	ENUM_PHY_5G_HT20_MCS6,
	ENUM_PHY_5G_HT20_MCS7,
	ENUM_PHY_5G_HT40_BPSK,
	ENUM_PHY_5G_HT40_QPSK,
	ENUM_PHY_5G_HT40_16QAM,
	ENUM_PHY_5G_HT40_MCS5,
	ENUM_PHY_5G_HT40_MCS6,
	ENUM_PHY_5G_HT40_MCS7,
	ENUM_PHY_MODE_NUM
};

enum ENUM_POWER_SAVE_POLL_MODE {
	ENUM_POWER_SAVE_POLL_DISABLE,
	ENUM_POWER_SAVE_POLL_LEGACY_NULL,
	ENUM_POWER_SAVE_POLL_QOS_NULL,
	ENUM_POWER_SAVE_POLL_NUM
};

enum ENUM_AC_TYPE {
	ENUM_AC_TYPE_AC0,
	ENUM_AC_TYPE_AC1,
	ENUM_AC_TYPE_AC2,
	ENUM_AC_TYPE_AC3,
	ENUM_AC_TYPE_AC4,
	ENUM_AC_TYPE_AC5,
	ENUM_AC_TYPE_AC6,
	ENUM_AC_TYPE_BMC,
	ENUM_AC_TYPE_NUM
};

enum ENUM_ADV_AC_TYPE {
	ENUM_ADV_AC_TYPE_RX_NSW,
	ENUM_ADV_AC_TYPE_RX_PTA,
	ENUM_ADV_AC_TYPE_RX_SP,
	ENUM_ADV_AC_TYPE_TX_PTA,
	ENUM_ADV_AC_TYPE_TX_RSP,
	ENUM_ADV_AC_TYPE_NUM
};

enum ENUM_REG_CH_MAP {
	REG_CH_MAP_COUNTRY_CODE,
	REG_CH_MAP_TBL_IDX,
	REG_CH_MAP_CUSTOMIZED,
	REG_CH_MAP_BLOCK_INDOOR,
	REG_CH_MAP_NUM
};

enum ENUM_FEATURE_OPTION {
	FEATURE_DISABLED,
	FEATURE_ENABLED,
	FEATURE_FORCE_ENABLED
};

/* This enum indicates whether the config accessible in user load */
enum ENUM_FEATURE_SUPPORT_SCOPE {
	/* Accessible in user load */
	FEATURE_TO_CUSTOMER,
	/* Unaccessible in user load */
	FEATURE_DEBUG_ONLY
};

/* This enum is for later added feature options which use command reserved field
 * as option switch
 */
enum ENUM_FEATURE_OPTION_IN_CMD {
	FEATURE_OPT_CMD_AUTO,
	FEATURE_OPT_CMD_DISABLED,
	FEATURE_OPT_CMD_ENABLED,
	FEATURE_OPT_CMD_FORCE_ENABLED
};

enum ENUM_T2LM_NEGOTIATION_SUPPORT {
	T2LM_NO_SUPPORT,
	T2LM_ALL_TIDS_SAME_LINK,
	T2LM_RESERVED,
	T2LM_EACH_TIDS_DIFFERENT_LINK,
};

enum ENUM_FEATURE_OPTION_IN_SER {
	/* DISABLE means
	 * 1. When driver init, driver will send fw CMD to disable the
	 *    corresponding SER feature even if it was enabled in fw build time.
	 * 2. driver will bypass the corresponding SER reset procedure when it
	 *    receives the error notification from chip.
	 */
	FEATURE_OPT_SER_DISABLE = 0,
	/* ENABLE means
	 * 1. driver will execute the corresponding SER reset procedure when
	 *    it receives the error notification from chip.
	 */
	FEATURE_OPT_SER_ENABLE,
	/* Monitor means
	 * 1. driver will bypass the corresponding SER reset procedure when it
	 *    receives the error notification from chip.
	 */
	FEATURE_OPT_SER_MONITOR

	/* Thus, the difference between disable and monitor is
	 *   - In L0.5 reset, chip wfsys watchdog is disabled and enabled
	 *     when feature options is disable and monitor respectively.
	 *     So, in the former, driver won't receive watchdog timeout
	 *     event even when mcu hangs and in the latter, driver will.
	 *   - In L1 reset, chip MAC watchdog is disabled and enabled
	 *     when feature option is disable and monitor respectively.
	 *     So, in the former, driver won't receive hw error notification
	 *     even when hw hangs and in the latter, driver will.
	 */
};

#define DEBUG_MSG_SIZE_MAX 1200
enum {
	DEBUG_MSG_ID_UNKNOWN = 0x00,
	DEBUG_MSG_ID_PRINT = 0x01,
	DEBUG_MSG_ID_FWLOG = 0x02,
	DEBUG_MSG_ID_END
};

enum {
	DEBUG_MSG_TYPE_UNKNOWN = 0x00,
	DEBUG_MSG_TYPE_MEM8 = 0x01,
	DEBUG_MSG_TYPE_MEM32 = 0x02,
	DEBUG_MSG_TYPE_ASCII = 0x03,
	DEBUG_MSG_TYPE_BINARY = 0x04,
	DEBUG_MSG_TYPE_DRIVER = 0x05,
	DEBUG_MSG_TYPE_END
};


#if (CFG_SUPPORT_PKT_OFLD == 1)

#define PKT_OFLD_BUF_SIZE 1488
enum {
	PKT_OFLD_TYPE_APF = 0,
	PKT_OFLD_TYPE_IGMP,
	PKT_OFLD_TYPE_MDNS,
	PKT_OFLD_TYPE_RA,
	PKT_OFLD_TYPE_CUSTOM,
	PKT_OFLD_TYPE_END
};

enum {
	PKT_OFLD_OP_DISABLE = 0,
	PKT_OFLD_OP_ENABLE,
	PKT_OFLD_OP_INSTALL,
	PKT_OFLD_OP_QUERY,
	PKT_OFLD_OP_ENABLE_W_TPUT_DETECT,
	PKT_OFLD_OP_ADD,
	PKT_OFLD_OP_REMOVE,
	PKT_OFLD_OP_UPDATE,
	PKT_OFLD_OP_REPORT,
	PKT_OFLD_OP_END
};
#endif /* CFG_SUPPORT_PKT_OFLD */
#define CHIP_CONFIG_RESP_SIZE 320
enum {
	CHIP_CONFIG_TYPE_WO_RESPONSE = 0x00,
	CHIP_CONFIG_TYPE_MEM8 = 0x01,
	CHIP_CONFIG_TYPE_MEM32 = 0x02,
	CHIP_CONFIG_TYPE_ASCII = 0x03,
	CHIP_CONFIG_TYPE_BINARY = 0x04,
	CHIP_CONFIG_TYPE_DRV_PASSTHROUGH = 0x05,
	CHIP_CONFIG_TYPE_END
};

struct SET_TXPWR_CTRL {
	int8_t c2GLegacyStaPwrOffset;	/* Unit: 0.5dBm, default: 0 */
	int8_t c2GHotspotPwrOffset;
	int8_t c2GP2pPwrOffset;
	int8_t c2GBowPwrOffset;
	int8_t c5GLegacyStaPwrOffset;	/* Unit: 0.5dBm, default: 0 */
	int8_t c5GHotspotPwrOffset;
	int8_t c5GP2pPwrOffset;
	int8_t c5GBowPwrOffset;
	uint8_t ucConcurrencePolicy;	/* TX power policy when concurrence
					 *  in the same channel
					 *  0: Highest power has priority
					 *  1: Lowest power has priority
					 */

	int8_t acReserved1[3];		/* Must be zero */

	/* Power limit by channel for all data rates */
	int8_t acTxPwrLimit2G[14];	/* Channel 1~14, Unit: 0.5dBm */
	int8_t acTxPwrLimit5G[4];	/* UNII 1~4 */
	int8_t acReserved2[2];		/* Must be zero */
};

#if CFG_WOW_SUPPORT

struct WOW_WAKE_HIF {
	/* use in-band signal to wakeup system, ENUM_HIF_TYPE */
	uint8_t		ucWakeupHif;

	/* GPIO Pin */
	uint8_t		ucGpioPin;

	/* refer to PF_WAKEUP_CMD_BIT0_OUTPUT_MODE_EN */
	uint8_t		ucTriggerLvl;

	/* non-zero means output reverse wakeup signal after delay time */
	uint32_t	u4GpioInterval;

	uint8_t		aucResv[5];
};

struct WOW_CTRL {
	uint8_t fgWowEnable;	/* 0: disable, 1: wow enable */
	uint8_t ucScenarioId;	/* just a profile ID */
	uint8_t ucBlockCount;
	uint8_t aucReserved1[1];
	struct WOW_WAKE_HIF astWakeHif[2];
	struct WOW_PORT stWowPort;
	uint8_t ucReason;
	uint8_t aucReserved2[3];
};

#if CFG_DC_USB_WOW_CALLBACK
enum ENUM_WOW_SCENARIO {
	WOW_NORMAL = 0,
	WOW_HOST_STANDBY = 1
};
#endif

#if CFG_SUPPORT_MDNS_OFFLOAD

/* Maximum size of the data array is defined as 4KB */
#define MAX_MDNS_USE_SIZE 4096
/* Maximum size of the transfer size is  as 1KB */
#define MAX_MDNS_TRANSFER_SIZE 1024

/* fail of mdns data struct oversize */
#define FAIL_MDNS_OVERSIZE 65535

/* mdns record max number */
#define MAX_MDNS_CACHE_NUM	10
/* mdns passthrough max number */
#define MAX_MDNS_PASSTHTOUGH_NUM 20

/*
 * DataBlock structure to hold the actual data.
 * The data in 'data' array is organized in a way
 * that it holds a two-byte length and then the data,
 * following this pattern: LENGTH HIGH_BYTE LENGTH LOW_BYTE DATA[LENGTH].
 *
 DataBlock Design
+----------------------------------------------------------------------------+
|                                   DataBlock                                |
| +------------------------------------------------------------------------+ |
| | length1 H-byte | length1 L-byte | data1[1] | data1[2] |  | data1[length1]|
| | length2 H-byte | length2 L-byte | data2[1] | data2[2] |  | data2[length2]|
| |                             ............................               | |
| | lengthN H-byte | lengthN L-byte | dataN[1] | dataN[2] |  | dataN[lengthN]|
| +------------------------------------------------------------------------+ |
| |                                     index                                |
+----------------------------------------------------------------------------+
 */
struct MDNS_DATABLOCK_T  {
    /* An array to mdns record and passthrough payload */
	uint8_t data[MAX_MDNS_USE_SIZE];
	/*the used size in data ,max 4096 */
	uint16_t index;
};

/*
 * Index structure to hold the indices pointing to locations of specific data
 * (response and name) in the data array within a DataBlock structure.
 *
 * Index Design
 *    +-------------------+     +-------------------+     +-------------------+
 *     |       Index       |     |       Index       |     |       Index       |
 *     | +---------------+ |     | +---------------+ |     | +---------------+ |
 *     | |      type     | |     | |      type     | |     | |      type     | |
 *     | +---------------+ |     | +---------------+ |     | +---------------+ |
 *     | | responseIndex | |     | | responseIndex | |     | | responseIndex | |
 *     | +---------------+ |     | +---------------+ |     | +---------------+ |
 *     | |   nameIndex   | |     | |  nameIndex    | |     | |  nameIndex    | |
 *     | +---------------+ |     | +---------------+ |     | +---------------+ |
 *     +-------------------+     +-------------------+     +-------------------+
 */

struct MDNS_RECORD_T {
/*
 * Type variable structure:
	ucquerynumber: 1-4
	u2querytype: 1 - A, 12 - PTR, 16 - TXT, 33 – SRV and others
 */
	uint8_t ucquerynumber;
	uint16_t u2querytype[4];
/* index of the 'response' data in the data array */
/* The first two are the length, and the rest is the valid data */
/*|length1 H-byte | length1 L-byte | data1[1] | data1[2] || data1[length1]*/
	uint16_t u2responseIndex;
/* index of the 'name' data in the data array */
/* The first two are the length, and the rest is the valid data */
/*|length1 H-byte | length1 L-byte | data1[1] | data1[2] || data1[length1]*/
	uint16_t u2nameIndex[4];
};


/*
 * Name Index Array structure to hold indices of specific
 * names in the data array
 * It can store up to MAX_MDNS_PASSTHTOUGH_NUM indices
 * For passrthrough
 * Passrthrough
 * +------------------------------------+
 * |            Passrthrough            |
 * | +-------------------------------+  |
 * | |nameIndices[MAX_MDNS_PASSTHTOUGH_NUM]
 * | +-------------------------------+  |
 * | |             count             |  |
 * | +-------------------------------+  |
 * +------------------------------------+
 */
struct MDNS_PASSTHROUGH_T {
/* Current number of passthrough stored name indices */
/*  passthrough number max to MAX_MDNS_PASSTHTOUGH_NUM */
	uint8_t count;
/* index of the 'passthroughname' data in the data array */
/* The first two are the length, and the rest is the valid data */
/*|length1 H-byte | length1 L-byte | data1[1] | data1[2] || data1[length1]*/
	uint16_t nameIndices[MAX_MDNS_PASSTHTOUGH_NUM];
};

/* mdns record max response length */
#define MDNS_RESPONSE_RECORD_MAX_LEN	1024
/* mdns record max name length */
#define MDNS_QUESTION_NAME_MAX_LEN	256
#define MDNS_QURTRY_NUMBER	4

/* mdns and mdns record cmd */
#define MDNS_CMD_ENABLE		1
#define MDNS_CMD_DISABLE	2
#define MDNS_CMD_ADD_RECORD	3
#define MDNS_CMD_CLEAR_RECORD	4
#define MDNS_CMD_DEL_RECORD	5

/* mdns and mdns passthrough cmd */
#define MDNS_CMD_SET_PASSTHTOUGH	6
#define MDNS_CMD_ADD_PASSTHTOUGH	7
#define MDNS_CMD_DEL_PASSTHTOUGH	8
#define MDNS_CMD_GET_HITCOUNTER		9
#define MDNS_CMD_GET_MISSCOUNTER	10
#define MDNS_CMD_RESETALL	11
#define MDNS_CMD_CLEAR_PASSTHTOUGH	12
/* IPV6 wake up host*/
#define MDNS_CMD_SET_IPV6_WAKEUP_FLAG	13
#define MDNS_CMD_SET_WAKEUP_FLAG	14

/* ucCmd passthrouth Behavior  */
enum MDNS_PassthroughBehavior {
	MDNS_PASSTHROUGH_FORWARD_ALL = 1,
	MDNS_PASSTHROUGH_DROP_ALL = 2,
	MDNS_PASSTHROUGH_LIST = 3
};

#define MDNS_PAYLOAD_TYPE_LEN				2
#define MDNS_PAYLOAD_CLASS_LEN				2
#define MDNS_PAYLOAD_TTL_LEN				4
#define MDNS_PAYLOAD_DATALEN_LEN			2

#define MDNS_ELEM_TYPE_PTR		12
#define MDNS_ELEM_TYPE_SRV		33
#define MDNS_ELEM_TYPE_TXT		16
#define MDNS_ELEM_TYPE_A		1


#define MDNS_WAKEUP_BY_NO_MATCH_RECORD BIT(0)
#define MDNS_WAKEUP_BY_SUB_REQ	BIT(1)

struct WLAN_MAC_HEADER_QoS_T {
	uint16_t u2FrameCtrl;
	uint16_t DurationID;
	uint8_t aucAddr1[MAC_ADDR_LEN];
	uint8_t aucAddr2[MAC_ADDR_LEN];
	uint8_t aucAddr3[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;
	uint16_t u2QosCtrl;
};

struct WLAN_MDNS_HDR_T {
	uint16_t usMdnsId;
	uint16_t usMdnsFlags;
	uint16_t usQuestionCnt;
	uint16_t usAnswerCnt;
	uint16_t usAuthCnt;
	uint16_t usAddtionCnt;
};

#define UDP_HEADER_LENGTH 8
#define IPV4_HEADER_LENGTH 20

struct MDNS_TEMPLATE_T {
	uint8_t name[MDNS_QUESTION_NAME_MAX_LEN];
	uint8_t name_length;
	uint8_t ucPadding0; /*padding*/
	uint16_t class;
	uint16_t type;
};

struct MDNS_PARAM_T {
	struct MDNS_TEMPLATE_T query[MDNS_QURTRY_NUMBER];
	uint16_t response_len;
	uint8_t ucPadding0[2]; /*padding*/
	uint8_t response[MDNS_RESPONSE_RECORD_MAX_LEN];
};

struct MDNS_PASSTHROUGHLIST_T {
	uint8_t name[MDNS_QUESTION_NAME_MAX_LEN];
	uint16_t u2PassthroghLength;
};

struct MDNS_INFO_UPLAYER_T {
	uint8_t ucCmd;
	struct MDNS_PARAM_T mdns_param;
	uint8_t recordKey;
	uint8_t name[MDNS_QUESTION_NAME_MAX_LEN];
	uint8_t passthroughBehavior;
	uint8_t ucIPV6WakeupFlag;
};

struct MDNS_PARAM_ENTRY_T {
	struct LINK_ENTRY rLinkEntry;
	struct MDNS_PARAM_T mdns_param;
	uint8_t recordKey;
};

struct MDNS_PASSTHROUGH_ENTRY_T {
	struct LINK_ENTRY rLinkEntry;
	struct MDNS_PASSTHROUGHLIST_T mdns_passthrough;
};

struct CMD_MDNS_PARAM_T {
    /* 1 Byte fields, total 8 bytes */
	uint8_t ucCmd;
	uint8_t ucRecordId;
	uint8_t ucWakeFlag;
	uint8_t ucPassthrouthId;

	uint8_t ucPassthroughBehavior;
	uint8_t ucIPV6WakeupFlag;
	/* mdns total transfer length 0 - 4096 */
	uint8_t ucPayloadOrder;
	uint8_t ucPadding;

	/* 26 bytes */
	struct WLAN_MAC_HEADER_QoS_T aucMdnsMacHdr;

	/* 2 bytes */
	/* mdns total transfer length 0 - 4096 */
	uint16_t u2PayloadTotallength;

	/* 20 bytes */
	uint8_t aucMdnsIPHdr[IPV4_HEADER_LENGTH];

	/* 8 bytes */
	uint8_t aucMdnsUdpHdr[UDP_HEADER_LENGTH];

	/* 1024 bytes */
	/* mdns of 1024 per transmission*/
	uint8_t ucPayload[MAX_MDNS_TRANSFER_SIZE];
};

struct EVENT_ID_MDNS_RECORD_T {
/* DWORD_0 */
	uint8_t ucVersion;
	uint8_t ucType; /* 0: invalid, 1: Hit 2: Miss */
	uint16_t u2ControlFlag;
/* DWORD_1 */
	uint32_t u4MdnsHitMiss;
/* DWORD_2 */
	uint8_t aucReserved2[64];
};

struct MDNS_SETTING_FLAGS_T {
/* DWORD_0 */
	uint8_t ucSetPortFlag;
	uint8_t ucPassthroughBehavior;
	uint8_t ucIPV6WakeupFlag;
	uint8_t ucPadding1[1]; /*padding*/
};

struct MDNS_INFO_T {
	struct LINK rMdnsRecordList;
	struct LINK rMdnsRecordFreeList;
	struct MDNS_PARAM_ENTRY_T rMdnsEntry[MAX_MDNS_CACHE_NUM];
	int rMdnsRecordCout;
	int rMdnsPassthroughCout;

	struct LINK rMdnsPassthroughList;
	struct LINK rMdnsPassthroughFreeList;
	struct MDNS_PASSTHROUGH_ENTRY_T
		rMdnsPassthroughEntry[MAX_MDNS_PASSTHTOUGH_NUM];
	struct EVENT_ID_MDNS_RECORD_T rMdnsRecordEvent;
	struct MDNS_SETTING_FLAGS_T rMdnsSaveFlags;

	struct MDNS_RECORD_T rMdnsRecordIndices[MAX_MDNS_CACHE_NUM];
	uint16_t currentIndex;

	struct MDNS_PASSTHROUGH_T passrthrough;
	struct MDNS_DATABLOCK_T  dataBlock;

};

#endif /* #if CFG_SUPPORT_MDNS_OFFLOAD */

#endif /* #if CFG_WOW_SUPPORT */

#if (CFG_SUPPORT_TWT == 1)
enum _ENUM_TWT_TYPE_T {
	ENUM_TWT_TYPE_DEFAULT = 0, /* for local emu */
	ENUM_TWT_TYPE_ITWT,
	ENUM_TWT_TYPE_BTWT,
	ENUM_TWT_TYPE_MLTWT,
	ENUM_TWT_TYPE_RTWT,
	ENUM_TWT_TYPE_NUM
};

enum _TWT_GET_TSF_REASON {
	TWT_GET_TSF_FOR_ADD_AGRT_BYPASS = 1,
	TWT_GET_TSF_FOR_ADD_AGRT = 2,
	TWT_GET_TSF_FOR_RESUME_AGRT = 3,
	TWT_GET_TSF_FOR_ADD_AGRT_BTWT = 4,
	TWT_GET_TSF_FOR_ADD_AGRT_ML_TWT_ALL_LINKS = 5,
	TWT_GET_TSF_FOR_ADD_AGRT_ML_TWT_ONE_BY_ONE = 6,
	TWT_GET_TSF_FOR_END_AGRT_ML_TWT_ONE_BY_ONE = 7,
	TWT_GET_TSF_FOR_CNM_TEARDOWN_GRANTED = 8,
	TWT_GET_TSF_FOR_ADD_AGRT_RTWT = 9,
	TWT_GET_TSF_FOR_JOIN_AGRT_RTWT = 10,
	TWT_GET_TSF_REASON_MAX
};

enum _TWT_STA_CNM_REASON {
	TWT_STA_CNM_DEFAULT = 0,
	TWT_STA_CNM_SETUP = 1,
	TWT_STA_CNM_TEARDOWN = 2,
	TWT_STA_CNM_REASON_MAX
};

struct _TWT_PARAMS_T {
	uint8_t fgReq;
	uint8_t fgTrigger;
	uint8_t fgProtect;
	uint8_t fgUnannounced;
	uint8_t ucSetupCmd;
	uint8_t ucMinWakeDur;
	uint8_t ucWakeIntvalExponent;
	uint8_t fgByPassNego;
	uint16_t u2WakeIntvalMantiss;
	/* TWT target wake time from iwpriv command parameter */
	uint16_t u2TWT;
	/*
	 * Final target wake time calculation
	 * u8twt_interval = (u_int64_t)(u2WakeIntvalMantiss
	 *                              < ucWakeIntvalExponent)
	 * u8Temp = u8CurTsf + u8twt_interval
	 * u8Mod = kal_mod64(u8Temp, u8twt_interval)
	 * u8TWT = u8CurTsf + u8twt_interval - u8Mod
	 * the u8TWT thus obtained is the final target wakeup time
	 * on which the underlying F/W + H/W operates
	 */
	uint64_t u8TWT;
#if (CFG_SUPPORT_RTWT == 1)
	uint8_t ucTrafficInfoPresent;
	uint8_t ucDlUlBmpValid;
	uint8_t ucDlBmp;
	uint8_t ucUlBmp;
#endif
#ifdef CFG_SUPPORT_TWT_EXT
	uint32_t u4DesiredWakeTime;
	uint32_t u4WakeIntvalMin;
	uint32_t u4WakeIntvalMax;
	uint32_t ucWakeDurMin;
	uint32_t ucWakeDurMax;
#endif
};

struct _NEXT_TWT_INFO_T {
	uint64_t u8NextTWT;
	uint8_t ucNextTWTSize;
};

struct _TWT_CTRL_T {
	uint8_t ucBssIdx;
	uint8_t ucCtrlAction;
	uint8_t ucTWTFlowId;
	uint8_t ucMLTWT_Param_Last;
	struct _TWT_PARAMS_T rTWTParams;
	struct _NEXT_TWT_INFO_T rNextTWT;
	u_int8_t fgTeardownAll;
};

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
struct _TWT_HOTSPOT_CTRL_T {
	uint8_t ucBssIdx;
	uint8_t ucCtrlAction;
	uint8_t ucTWTFlowId;
	struct _TWT_PARAMS_T rTWTParams;
	uint8_t ucDialogToken;
	uint8_t ucIsReject;
	struct STA_RECORD *prStaRec;
};

struct _TWT_HOTSPOT_STA_NODE {
	/* twt link node control */
	struct LINK_ENTRY list_entry;
	/* twt entry */
	uint8_t	used;
	uint8_t	agrt_tbl_idx;
	uint8_t	own_mac_idx;
	uint8_t	flow_id;
	uint16_t	peer_id_grp_id;
	uint8_t	agrt_sp_duration;
	uint8_t	bss_idx;
	/* used by scheduler and will be within TWT_SP_SPAN_TIME */
	uint64_t	schedule_sp_start_tsf;
	uint64_t  twt_assigned_tsf;  /* TWT real timestamp */
	uint64_t	agrt_sp_start_tsf;  /* tsf from requester */
	uint16_t	agrt_sp_wake_intvl_mantissa;
	uint8_t	agrt_sp_wake_intvl_exponent;
	struct STA_RECORD *prStaRec;
};
#endif

struct _TWT_GET_TSF_CONTEXT_T {
	enum _TWT_GET_TSF_REASON ucReason;
	uint8_t ucBssIdx;
	uint8_t ucTWTFlowId;
	uint8_t fgIsOid;
	struct _TWT_PARAMS_T rTWTParams;
	struct _NEXT_TWT_INFO_T rNextTWT;
	enum _TWT_STA_CNM_REASON ucTwtStaCnmReason;
};

#endif

#if (CFG_SUPPORT_802_11AX == 1)
struct _SMPS_PARAMS_T {
	uint8_t ucHtHeCap;
};

struct _SMPS_CTRL_T {
	uint8_t ucBssIdx;
	uint8_t ucCtrlAction;
	struct _SMPS_PARAMS_T rSMPSParams;
};
#endif

struct REAL_TIME {
	int32_t i4TmSec;
	int32_t i4TmMin;
	int32_t i4TmHour;
	int32_t i4TmDay;
	int32_t i4TmMon;
	int32_t i4TmYear;
	int32_t i4TmWday;
	int32_t i4TmYday;
	int32_t i4TmIsDst;
	uint32_t u4TvValUsec;
	uint32_t u4TvValSec;
};

enum ENUM_FT_DS_STATE {
	FT_DS_STATE_IDLE,
	FT_DS_STATE_ONGOING,
	FT_DS_STATE_AUTHORIZED,
	FT_DS_STATE_FAIL,
};

struct FT_EVENT_PARAMS {
	enum ENUM_FT_DS_STATE eFtDsState;
	uint16_t u2FtDsStatusCode; /* Status of FT response */

	const uint8_t *pcIe;
	uint16_t u2IeLen;
	struct STA_RECORD *prTargetAp;
	const uint8_t *pcRicIes;
	uint16_t u2RicIesLen;
};

enum ENUM_NVRAM_MTK_FEATURE {
	MTK_FEATURE_2G_256QAM_DISABLED = 0,
	MTK_FEATURE_NUM
};

/* For storing driver initialization value from glue layer */
struct REG_INFO {
	uint32_t u4SdBlockSize;	/* SDIO block size */
	uint32_t u4SdBusWidth;	/* SDIO bus width. 1 or 4 */
	uint32_t u4SdClockRate;	/* SDIO clock rate. (in unit of HZ) */

	/* Start Frequency for Ad-Hoc network : in unit of KHz */
	uint32_t u4StartFreq;

	/* Default mode for Ad-Hoc network : ENUM_PARAM_AD_HOC_MODE_T */
	uint32_t u4AdhocMode;

	uint32_t u4RddStartFreq;
	uint32_t u4RddStopFreq;
	uint32_t u4RddTestMode;
	uint32_t u4RddShutFreq;
	uint32_t u4RddDfs;
	int32_t i4HighRssiThreshold;
	int32_t i4MediumRssiThreshold;
	int32_t i4LowRssiThreshold;
	int32_t au4TxPriorityTag[ENUM_AC_TYPE_NUM];
	int32_t au4RxPriorityTag[ENUM_AC_TYPE_NUM];
	int32_t au4AdvPriorityTag[ENUM_ADV_AC_TYPE_NUM];
	uint32_t u4FastPSPoll;
	uint32_t u4PTA;		/* 0: disable, 1: enable */
	uint32_t u4TXLimit;	/* 0: disable, 1: enable */
	uint32_t u4SilenceWindow;	/* range: 100 - 625, unit: us */
	uint32_t u4TXLimitThreshold;	/* range: 250 - 1250, unit: us */
	uint32_t u4PowerMode;
	uint32_t fgEnArpFilter;
	uint32_t u4PsCurrentMeasureEn;
	uint32_t u4UapsdAcBmp;
	uint32_t u4MaxSpLen;

	/* 0: enable online scan, non-zero: disable online scan */
	uint32_t fgDisOnlineScan;

	/* 0: enable online scan, non-zero: disable online scan */
	uint32_t fgDisBcnLostDetection;

	/* 0: automatic, non-zero: fixed rate */
	uint32_t u4FixedRate;

	uint32_t u4ArSysParam0;
	uint32_t u4ArSysParam1;
	uint32_t u4ArSysParam2;
	uint32_t u4ArSysParam3;

	/* 0:enable roaming 1:disable */
	uint32_t fgDisRoaming;

	/* NVRAM - MP Data -START- */
#if 1
	uint16_t u2Part1OwnVersion;
	uint16_t u2Part1PeerVersion;
#endif
	uint8_t aucMacAddr[6];
	/* Country code (in ISO 3166-1 expression, ex: "US", "TW")  */
	uint16_t au2CountryCode[4];
	uint8_t ucSupport5GBand;

	enum ENUM_REG_CH_MAP eRegChannelListMap;
	uint8_t ucRegChannelListIndex;
	struct DOMAIN_INFO_ENTRY rDomainInfo;
	struct RSSI_PATH_COMPASATION rRssiPathCompasation;
	uint8_t ucRssiPathCompasationUsed;
	/* NVRAM - MP Data -END- */

	/* NVRAM - Functional Data -START- */
	uint8_t ucEnable5GBand;
	/* NVRAM - Functional Data -END- */

	struct NEW_EFUSE_MAPPING2NVRAM *prOldEfuseMapping;

	uint8_t aucNvram[512];
	struct WIFI_CFG_PARAM_STRUCT *prNvramSettings;
#if CFG_SUPPORT_XONVRAM
	struct XO_CFG_PARAM_STRUCT *prXonvCfg;
#endif
};

/* for divided firmware loading */
struct FWDL_SECTION_INFO {
#if 0
	uint32_t u4Offset;
	uint32_t u4Reserved;
	uint32_t u4Length;
	uint32_t u4DestAddr;
#endif
	uint32_t u4DestAddr;
	uint8_t ucChipInfo;
	uint8_t ucFeatureSet;
	uint8_t ucEcoCode;
	uint8_t aucReserved[9];
	uint8_t aucBuildDate[16];
	uint32_t u4Length;
};

struct FIRMWARE_DIVIDED_DOWNLOAD {
#if 0
	uint32_t u4Signature;

	/* CRC calculated without first 8 bytes included */
	uint32_t u4CRC;

	uint32_t u4NumOfEntries;
	uint32_t u4Reserved;
	struct FWDL_SECTION_INFO arSection[];
#endif
	struct FWDL_SECTION_INFO arSection[2];
};

struct PARAM_MCR_RW_STRUCT {
	uint32_t u4McrOffset;
	uint32_t u4McrData;
};

/* per access category statistics */
struct WIFI_WMM_AC_STAT {
	uint32_t u4TxMsdu;
	uint32_t u4RxMsdu;
	uint32_t u4TxDropMsdu;
	uint32_t u4TxFailMsdu;
	uint32_t u4TxRetryMsdu;
};

struct TX_VECTOR_BBP_LATCH {
	uint32_t u4TxV[3];
};

struct MIB_INFO_STAT {
	uint32_t u4RxMpduCnt;
	uint32_t u4FcsError;
	uint32_t u4RxFifoFull;
	uint32_t u4AmpduTxSfCnt;
	uint32_t u4AmpduTxAckSfCnt;
#if (CFG_SUPPORT_CONNAC3X == 1)
	uint32_t au4TxRangeAmpduCnt[AGG_RANGE_SEL_NUM + 1];
#else
	uint16_t au2TxRangeAmpduCnt[AGG_RANGE_SEL_NUM + 1];
#endif
};

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
struct PARAM_GET_STATS_ONE_CMD {
	/* unit: ms */
	uint32_t u4Period;
};
#endif

struct PARAM_GET_STA_STATISTICS {
	/* Per-STA statistic */
	uint8_t ucInvalid;
	uint8_t ucVersion;
	uint8_t aucMacAddr[MAC_ADDR_LEN];

	uint32_t u4LinkScore;
	uint32_t u4Flag;

	uint8_t ucReadClear;
	uint8_t ucLlsReadClear;

	/* From driver */
	uint32_t u4TxTotalCount;
	uint32_t u4TxExceedThresholdCount;

	uint32_t u4TxMaxTime;
	uint32_t u4TxAverageProcessTime;

	uint32_t u4TxMaxHifTime;
	uint32_t u4TxAverageHifTime;

	uint32_t u4RxTotalCount;

	/*
	 * How many packages Enqueue/Deqeue during statistics interval
	 */
	uint32_t u4EnqueueCounter;
	uint32_t u4DequeueCounter;

	uint32_t u4EnqueueStaCounter;
	uint32_t u4DequeueStaCounter;

	uint32_t IsrCnt;
	uint32_t IsrPassCnt;
	uint32_t TaskIsrCnt;

	uint32_t IsrAbnormalCnt;
	uint32_t IsrSoftWareCnt;
	uint32_t IsrRxCnt;
	uint32_t IsrTxCnt;

	uint32_t au4TcResourceEmptyCount[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4DequeueNoTcResource[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4TcResourceBackCount[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4TcResourceUsedPageCount[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4TcResourceWantedPageCount[NUM_TC_RESOURCE_TO_STATISTICS];

	uint32_t au4TcQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

	/* From FW */
	uint8_t ucPer;		/* base: 128 */
	uint8_t ucRcpi;
	uint32_t u4PhyMode;
	uint16_t u2LinkSpeed;	/* unit is 0.5 Mbits */

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	uint32_t u4TxDataCount;
#endif
	uint32_t u4TxFailCount;
	uint32_t u4TxLifeTimeoutCount;

	uint32_t u4TxAverageAirTime;

	/* Transmit in the air (wtbl) */
	uint32_t u4TransmitCount;

	/* Transmit without ack/ba in the air (wtbl) */
	uint32_t u4TransmitFailCount;

	/*link layer statistics */
	struct WIFI_WMM_AC_STAT arLinkStatistics[AC_NUM];

	/* Global queue management statistic */
	uint32_t au4TcAverageQueLen[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4TcCurrentQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

	uint8_t ucTemperature;
	uint8_t ucSkipAr;
	uint8_t ucArTableIdx;
	uint8_t ucRateEntryIdx;
	uint8_t ucRateEntryIdxPrev;
	uint8_t ucTxSgiDetectPassCnt;
	uint8_t ucAvePer;
#if (CFG_SUPPORT_RA_GEN == 0)
	uint8_t aucArRatePer[AR_RATE_TABLE_ENTRY_MAX];
	uint8_t aucRateEntryIndex[AUTO_RATE_NUM];
#else
	uint32_t u4AggRangeCtrl_0;
	uint32_t u4AggRangeCtrl_1;
	uint8_t ucRangeType;
#if (CFG_SUPPORT_CONNAC2X == 0 && CFG_SUPPORT_CONNAC3X == 0)
	uint8_t aucReserved5[24];
#else
	uint32_t u4AggRangeCtrl_2;
	uint32_t u4AggRangeCtrl_3;
#if (CFG_SUPPORT_CONNAC3X == 1)
	uint32_t u4AggRangeCtrl_4;
	uint32_t u4AggRangeCtrl_5;
	uint32_t u4AggRangeCtrl_6;
	uint32_t u4AggRangeCtrl_7;
#else
	uint8_t aucReserved5[16];
#endif
#endif
#endif
	uint8_t ucArStateCurr;
	uint8_t ucArStatePrev;
	uint8_t ucArActionType;
	uint8_t ucHighestRateCnt;
	uint8_t ucLowestRateCnt;
	uint16_t u2TrainUp;
	uint16_t u2TrainDown;
	uint32_t u4Rate1TxCnt;
	uint32_t u4Rate1FailCnt;
	struct TX_VECTOR_BBP_LATCH rTxVector[ENUM_BAND_NUM];
	struct MIB_INFO_STAT rMibInfo[ENUM_BAND_NUM];
	uint8_t ucResetCounter;
	u_int8_t fgIsForceTxStream;
	u_int8_t fgIsForceSeOff;
#if (CFG_SUPPORT_RA_GEN == 0)
	uint8_t aucReserved6[17];
#else
	uint16_t u2RaRunningCnt;
	uint8_t ucRaStatus;
	uint8_t ucFlag;
	uint8_t aucTxQuality[MAX_TX_QUALITY_INDEX];
	uint8_t ucTxRateUpPenalty;
	uint8_t ucLowTrafficMode;
	uint8_t ucLowTrafficCount;
	uint8_t ucLowTrafficDashBoard;
	uint8_t ucDynamicSGIState;
	uint8_t ucDynamicSGIScore;
	uint8_t ucDynamicBWState;
	uint8_t ucDynamicGband256QAMState;
	uint8_t ucVhtNonSpRateState;
#endif
#if CFG_SUPPORT_STA_INFO
	uint32_t u4RxBmcCnt;
#endif
	/* Reserved fields */
	uint8_t au4Reserved[3];
};

struct PARAM_GET_BSS_STATISTICS {
	/* Per-STA statistic */
	uint8_t aucMacAddr[MAC_ADDR_LEN];

	uint32_t u4Flag;

	uint8_t ucReadClear;

	uint8_t ucLlsReadClear;

	uint8_t ucBssIndex;

	/* From driver */
	uint32_t u4TxTotalCount;
	uint32_t u4TxExceedThresholdCount;

	uint32_t u4TxMaxTime;
	uint32_t u4TxAverageProcessTime;

	uint32_t u4RxTotalCount;

	uint32_t au4TcResourceEmptyCount[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4TcQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

	/* From FW */
	uint8_t ucPer;		/* base: 128 */
	uint8_t ucRcpi;
	uint32_t u4PhyMode;
	uint16_t u2LinkSpeed;	/* unit is 0.5 Mbits */

	uint32_t u4TxFailCount;
	uint32_t u4TxLifeTimeoutCount;

	uint32_t u4TxAverageAirTime;

	/* Transmit in the air (wtbl) */
	uint32_t u4TransmitCount;

	/* Transmit without ack/ba in the air (wtbl) */
	uint32_t u4TransmitFailCount;

	/*link layer statistics */
	struct WIFI_WMM_AC_STAT arLinkStatistics[AC_NUM];

	/* Global queue management statistic */
	uint32_t au4TcAverageQueLen[NUM_TC_RESOURCE_TO_STATISTICS];
	uint32_t au4TcCurrentQueLen[NUM_TC_RESOURCE_TO_STATISTICS];

	/* Reserved fields */
	uint8_t au4Reserved[32];	/* insufficient for LLS?? */
};

struct NET_INTERFACE_INFO {
	uint8_t ucBssIndex;
	void *pvNetInterface;
};

enum ENUM_TX_RESULT_CODE {
	TX_RESULT_SUCCESS = 0,
	TX_RESULT_LIFE_TIMEOUT,
	TX_RESULT_RTS_ERROR,
	TX_RESULT_MPDU_ERROR,
	TX_RESULT_AGING_TIMEOUT,
	TX_RESULT_FLUSHED,
	TX_RESULT_BIP_ERROR,
	TX_RESULT_UNSPECIFIED_ERROR,
	TX_RESULT_DROPPED_IN_DRIVER = 32,
	TX_RESULT_DROPPED_IN_FW,
	TX_RESULT_QUEUE_CLEARANCE,
	TX_RESULT_INACTIVE_BSS,
	TX_RESULT_FLUSH_PENDING,
	TX_RESULT_NUM
};

struct WLAN_CFG_ENTRY {
	uint8_t aucKey[WLAN_CFG_KEY_LEN_MAX];
	uint8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];
	void *pPrivate;
	uint32_t u4Flags;
};

struct WLAN_CFG {
	uint32_t u4WlanCfgEntryNumMax;
	uint32_t u4WlanCfgKeyLenMax;
	uint32_t u4WlanCfgValueLenMax;
	struct WLAN_CFG_ENTRY arWlanCfgBuf[WLAN_CFG_ENTRY_NUM_MAX];
};

struct WLAN_CFG_REC {
	uint32_t u4WlanCfgEntryNumMax;
	uint32_t u4WlanCfgKeyLenMax;
	uint32_t u4WlanCfgValueLenMax;
	struct WLAN_CFG_ENTRY arWlanCfgBuf[WLAN_CFG_REC_ENTRY_NUM_MAX];
};

enum ENUM_MAX_BANDWIDTH_SETTING {
	MAX_BW_20MHZ = 0,
	MAX_BW_40MHZ,
	MAX_BW_80MHZ,
	MAX_BW_160MHZ,
	MAX_BW_80_80_MHZ,
	MAX_BW_320_1MHZ,
	MAX_BW_320_2MHZ,
	MAX_BW_UNKNOWN,
	MAX_BW_NUM = MAX_BW_UNKNOWN
};

struct TX_PACKET_INFO {
	uint8_t ucPriorityParam;
	uint32_t u4PacketLen;
	uint8_t aucEthDestAddr[MAC_ADDR_LEN];
	uint16_t u2Flag;

#if 0
	u_int8_t fgIs1X;
	u_int8_t fgIsPAL;
	u_int8_t fgIs802_3;
	u_int8_t fgIsVlanExists;
	u_int8_t fgIsDhcp;
	u_int8_t fgIsArp;
#endif
};

enum ENUM_TX_PROFILING_TAG {
	TX_PROF_TAG_OS_TO_DRV = 0,
	TX_PROF_TAG_DRV_ENQUE,
	TX_PROF_TAG_DRV_DEQUE,
	TX_PROF_TAG_DRV_TX_DONE,
	TX_PROF_TAG_ACQR_MSDU_TOK,
	TX_PROF_TAG_DRV_FREE
};

#if (CFG_SISO_SW_DEVELOP == 1) || (CFG_SUPPORT_SPE_IDX_CONTROL == 1)
enum ENUM_WF_PATH_FAVOR_T {
	ENUM_WF_NON_FAVOR = 0xff,
	ENUM_WF_0_ONE_STREAM_PATH_FAVOR = 0,
	ENUM_WF_1_ONE_STREAM_PATH_FAVOR = 1,
	ENUM_WF_0_1_TWO_STREAM_PATH_FAVOR = 2,
	ENUM_WF_0_1_DUP_STREAM_PATH_FAVOR = 3,
};

enum ENUM_SPE_SEL_BY_T {
	ENUM_SPE_SEL_BY_TXD = 0,
	ENUM_SPE_SEL_BY_WTBL = 1,
};
#endif

struct PARAM_GET_CNM_T {
	uint8_t	fgIsDbdcEnable;

	uint8_t	ucOpChNum[ENUM_BAND_NUM];
	uint8_t	ucChList[ENUM_BAND_NUM][MAX_OP_CHNL_NUM];
	uint8_t	ucChBw[ENUM_BAND_NUM][MAX_OP_CHNL_NUM];
	uint8_t	ucChSco[ENUM_BAND_NUM][MAX_OP_CHNL_NUM];
	uint8_t	ucChNetNum[ENUM_BAND_NUM][MAX_OP_CHNL_NUM];
	uint8_t	ucChBssList[ENUM_BAND_NUM][MAX_OP_CHNL_NUM][MAX_BSSID_NUM];

	uint8_t	ucBssInuse[MAX_BSSID_NUM + 1];
	uint8_t	ucBssActive[MAX_BSSID_NUM + 1];
	uint8_t	ucBssConnectState[MAX_BSSID_NUM + 1];

	uint8_t	ucBssCh[MAX_BSSID_NUM + 1];
	uint8_t	ucBssDBDCBand[MAX_BSSID_NUM + 1];
	uint8_t	ucBssWmmSet[MAX_BSSID_NUM + 1];
	uint8_t	ucBssWmmDBDCBand[MAX_BSSID_NUM + 1];
	uint8_t	ucBssOMACSet[MAX_BSSID_NUM + 1];
	uint8_t	ucBssOMACDBDCBand[MAX_BSSID_NUM + 1];
	uint8_t	ucBssOpTxNss[MAX_BSSID_NUM + 1];
	uint8_t	ucBssOpRxNss[MAX_BSSID_NUM + 1];
	uint8_t	ucBssLinkIdx[MAX_BSSID_NUM + 1];

	/* Reserved fields */
	uint8_t	au4Reserved[54]; /*Total 160 byte*/
};

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
struct WIFI_LINK_QUALITY_INFO {
	uint32_t u4CurTxRate;		/* current tx link speed */
	uint64_t u8TxTotalCount;	/* tx total accumulated count */
	uint64_t u8TxRetryCount;	/* tx retry count */
	uint64_t u8TxFailCount;		/* tx fail count */
	uint64_t u8TxRtsFailCount;	/* tx RTS fail count */
	uint64_t u8TxAckFailCount;	/* tx ACK fail count */

	uint32_t u4CurRxRate;		/* current rx link speed */
	uint64_t u8RxTotalCount;	/* rx total packages */
	uint32_t u4RxDupCount;		/* rx duplicate package count */
	uint64_t u8RxErrCount;		/* rx fcs fail count */

	uint32_t u4CurTxPer;		/* current Tx PER */
	uint64_t u8MdrdyCount;
	uint64_t u8IdleSlotCount;	/* congestion stats: idle slot */
	uint64_t u8DiffIdleSlotCount;
	uint32_t u4HwMacAwakeDuration;
	uint16_t u2FlagScanning;

	uint32_t u4PhyMode;
	uint16_t u2LinkSpeed;

	uint64_t u8LastTxTotalCount;
	uint64_t u8LastTxFailCount;
	uint64_t u8LastIdleSlotCount;
	uint64_t u8LastRxTotalCount;
};
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if CFG_SUPPORT_IOT_AP_BLOCKLIST
enum ENUM_WLAN_IOT_AP_FLAG_T {
	WLAN_IOT_AP_FG_VERSION = 0,
	WLAN_IOT_AP_FG_OUI,
	WLAN_IOT_AP_FG_DATA,
	WLAN_IOT_AP_FG_DATA_MASK,
	WLAN_IOT_AP_FG_BSSID,
	WLAN_IOT_AP_FG_BSSID_MASK,
	WLAN_IOT_AP_FG_NSS,
	WLAN_IOT_AP_FG_HT,
	WLAN_IOT_AP_FG_BAND,
	WLAN_IOT_AP_FG_ACTION,
	WLAN_IOT_AP_FG_MAX
};

enum ENUM_WLAN_IOT_ACTION {
	WLAN_IOT_AP_VOID = 0,
	WLAN_IOT_AP_DBDC_1SS,
	WLAN_IOT_AP_DIS_SG,
	WLAN_IOT_AP_COEX_CTS2SELF,
	WLAN_IOT_AP_FIX_MODE,
	WLAN_IOT_AP_DIS_2GHT40,
	WLAN_IOT_AP_KEEP_EDCA_PARAM = 6,
	WLAN_IOT_AP_COEX_DIS_RX_AMPDU,
	WLAN_IOT_AP_ADAPTIVE_BEACON_TIME = 12,
	WLAN_IOT_AP_OWE_PMK_REMOVE = 13,
	WLAN_IOT_AP_DIS_TX_AMSDU = 18,
	WLAN_IOT_AP_COEX_FORCE_NULL = 23,
	WLAN_IOT_AP_ACT_MAX
};

struct WLAN_IOT_AP_RULE_T {
	uint16_t u2MatchFlag;
	uint8_t  ucDataLen;
	uint8_t  ucDataMaskLen;
	uint8_t  ucVersion;
	uint8_t  aVendorOui[MAC_OUI_LEN];
	uint8_t  aVendorData[CFG_IOT_AP_DATA_MAX_LEN];
	uint8_t  aVendorDataMask[CFG_IOT_AP_DATA_MAX_LEN];
	uint8_t  aBssid[MAC_ADDR_LEN];
	uint8_t  aBssidMask[MAC_ADDR_LEN];
	uint8_t  ucNss;
	uint8_t  ucHtType;
	uint8_t  ucBand;
	/* Byte Alignment*/
	uint8_t  aReserved[1];
	uint64_t u8Action;
};
#endif

struct TRX_INFO {
	uint32_t u4TxFail[2];		/* By Band */
	uint32_t u4RxFail[2];		/* By Band */
	uint32_t u4TxHwRetry[2];	/* By Band */
	uint32_t u4TxOk[MAX_BSSID_NUM];	/* By BSSIDX */
	uint32_t u4RxOk[MAX_BSSID_NUM];	/* By BSSIDX */
};

struct RxRateInfo {
	uint32_t u4Mode;
	uint32_t u4Nss;
	uint32_t u4Bw;
	uint32_t u4Gi;
	uint32_t u4Rate;
};

enum THERMAL_TEMP_TYPE {
	THERMAL_TEMP_TYPE_ADIE,
	THERMAL_TEMP_TYPE_DDIE,
	THERMAL_TEMP_TYPE_NUM,
};

struct THERMAL_TEMP_DATA {
	enum THERMAL_TEMP_TYPE eType;
	uint8_t ucIdx;
	uint32_t u4Temperature;
};

struct THERMAL_TEMP_DATA_V2 {
	uint8_t ucType;
	uint8_t ucIdx;
	uint8_t *pu1SensorResult;
};

/* channel operating width */
enum WIFI_CHANNEL_WIDTH {
	WIFI_CHAN_WIDTH_20 = 0,
	WIFI_CHAN_WIDTH_40 = 1,
	WIFI_CHAN_WIDTH_80 = 2,
	WIFI_CHAN_WIDTH_160 = 3,
	WIFI_CHAN_WIDTH_80P80 = 4,
	WIFI_CHAN_WIDTH_5 = 5,
	WIFI_CHAN_WIDTH_10 = 6,
	WIFI_CHAN_WIDTH_INVALID = -1
};

/* channel information */
struct WIFI_CHANNEL_INFO {
	enum WIFI_CHANNEL_WIDTH width;
	uint32_t center_freq;
	uint32_t center_freq0;
	uint32_t center_freq1;
};

/* wifi rate */
struct WIFI_RATE {
	uint32_t preamble: 3;
	uint32_t nss: 2;
	uint32_t bw: 3;
	uint32_t rateMcsIdx: 8;
	uint32_t reserved: 16;
	uint32_t bitrate;
};

/* RTT Capabilities */
struct RTT_CAPABILITIES {
	/* if 1-sided rtt data collection is supported */
	uint8_t fgRttOneSidedSupported;
	/* if ftm rtt data collection is supported */
	uint8_t fgRttFtmSupported;
	/* if initiator supports LCI request. Applies to 2-sided RTT */
	uint8_t fgLciSupported;
	/* if initiator supports LCR request. Applies to 2-sided RTT */
	uint8_t fgLcrSupported;
	/* bit mask indicates what preamble is supported by initiator */
	uint8_t ucPreambleSupport;
	/* bit mask indicates what BW is supported by initiator */
	uint8_t ucBwSupport;
	/* if 11mc responder mode is supported */
	uint8_t fgResponderSupported;
	/* draft 11mc spec version supported by chip. For instance,
	 * version 4.0 should be 40 and version 4.3 should be 43 etc.
	 */
	uint8_t fgMcVersion;
};

/* RTT configuration */
struct RTT_CONFIG {
	uint8_t aucAddr[MAC_ADDR_LEN]; /* peer device mac address */
	uint8_t eType; /* enum ENUM_RTT_TYPE */
	uint8_t ePeer; /* enum ENUM_RTT_PEER_TYPE */
	struct WIFI_CHANNEL_INFO rChannel;
	uint16_t u2BurstPeriod;
	uint16_t u2NumBurstExponent;
	uint16_t u2PreferencePartialTsfTimer;
	uint8_t ucNumFramesPerBurst;
	uint8_t ucNumRetriesPerRttFrame;
	uint8_t ucNumRetriesPerFtmr;
	uint8_t ucLciRequest;
	uint8_t ucLcrRequest;
	uint8_t ucBurstDuration;
	uint8_t ePreamble; /* enum ENUM_WIFI_RTT_PREAMBLE */
	uint8_t eBw; /* enum ENUM_WIFI_RTT_BW */

	/* bellow are for internal useages */
	uint8_t eBand; /* enum ENUM_BAND */
	uint8_t ucPrimaryChannel;
	uint8_t ucS1;
	uint8_t ucS2;
	uint8_t eChannelWidth; /* enum ENUM_CHANNEL_WIDTH */
	uint8_t ucBssIndex;
	uint8_t eEventType;  /* enum ENUM_LOC_EVENT_TYPE_T*/
	uint8_t ucASAP;
	uint8_t ucFtmMinDeltaTime; //mc: UNIT:100us
	uint8_t ucReserved; // 4 byte align
};

struct RTT_RESULT {
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint32_t u4BurstNum;
	uint32_t  u4MeasurementNumber;
	uint32_t  u4SuccessNumber;
	uint8_t  ucNumPerBurstPeer;
	uint32_t eStatus; /* enum ENUM_RTT_STATUS */
	uint8_t  ucRetryAfterDuration;
	uint32_t eType; /* enum ENUM_RTT_TYPE */
	int32_t i4Rssi;
	int32_t i4RssiSpread;
	struct WIFI_RATE rTxRate;
	struct WIFI_RATE rRxRate;
	int64_t i8Rtt;
	int64_t i8RttSd;
	int64_t i8RttSpread;
	int32_t i4DistanceMM;
	int32_t i4DistanceSdMM;
	int32_t i4DistanceSpreadMM;
	int64_t i8Ts;
	int32_t i4BurstDuration;
	int32_t i4NegotiatedBustNum;
};

enum ENUM_TX_OVER_LIMIT_STATS_TYPE {
	REPORT_IMMEDIATE,
	REPORT_AVERAGE,
};

enum ENUM_TX_OVER_LIMIT_DELAY_TYPE {
	DRIVER_DELAY,
	MAC_DELAY,
	MAX_TX_OVER_LIMIT_TYPE,
};

enum CHAN_FLAGS {
	CHAN_RADAR,
	CHAN_NO_HT40PLUS,
	CHAN_NO_HT40MINUS,
	CHAN_NO_HT40,
	CHAN_NO_OFDM,
	CHAN_NO_80MHZ,
	CHAN_NO_160MHZ,
	CHAN_INDOOR_ONLY,
	CHAN_IR_CONCURRENT,
	CHAN_NO_20MHZ,
	CHAN_NO_10MHZ,
};

/* Consistent order with delayTypeChar */
enum ENUM_AVERAGE_TX_DELAY_TYPE {
	DRIVER_TX_DELAY,
	DRIVER_HIF_TX_DELAY,
	CONNSYS_TX_DELAY,
	MAC_TX_DELAY,
	AIR_TX_DELAY,
	FAIL_CONNSYS_TX_DELAY,
	MAX_AVERAGE_TX_DELAY_TYPE,
};

struct TEST_MODE_XO_CAL {
	uint32_t u4CalType;
	uint32_t u4ClkSrc;
	uint32_t u4Mode;
	uint32_t u4TargetReq;

	uint32_t u4AxmFreq;
	uint32_t u4AxmC1Freq;
	uint32_t u4AxmC2Freq;
	uint32_t u4AxmC1Comp;
	uint32_t u4AxmC2Comp;

	uint32_t u4BtmFreq;
	uint32_t u4BtmC1Freq;
	uint32_t u4BtmC2Freq;
	uint32_t u4BtmC1Comp;
	uint32_t u4BtmC2Comp;
};

/* Align multiate tool */
#define PLCAL_MAX_CNT	100
struct TEST_MODE_PL_CAL {
	uint32_t u4BandIdx;
	uint32_t u4PLCalId;
	uint32_t u4Action;
	uint32_t u4Flags;
	uint32_t u4InCnt;
	uint32_t u4InData[PLCAL_MAX_CNT];
	uint32_t u4OutCnt;
	uint32_t u4OutData[PLCAL_MAX_CNT];
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */


/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define BUILD_SIGN(ch0, ch1, ch2, ch3) \
	((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
	((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))

#define MTK_WIFI_SIGNATURE BUILD_SIGN('M', 'T', 'K', 'W')

#define IS_FEATURE_ENABLED(_ucFeature) \
	(((_ucFeature) == FEATURE_ENABLED) || \
	((_ucFeature) == FEATURE_FORCE_ENABLED))
#define IS_FEATURE_FORCE_ENABLED(_ucFeature) \
	((_ucFeature) == FEATURE_FORCE_ENABLED)
#define IS_FEATURE_DISABLED(_ucFeature) ((_ucFeature) == FEATURE_DISABLED)

/* This macro is for later added feature options which use command reserved
 * field as option switch
 */
/* 0: AUTO
 * 1: Disabled
 * 2: Enabled
 * 3: Force disabled
 */
#define FEATURE_OPT_IN_COMMAND(_ucFeature) ((_ucFeature) + 1)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

struct ADAPTER *wlanAdapterCreate(struct GLUE_INFO *prGlueInfo);

void wlanAdapterDestroy(struct ADAPTER *prAdapter);

void wlanOnPreAllocAdapterMem(struct ADAPTER *prAdapter,
			  const u_int8_t bAtResetFlow);

void wlanOnPostNicInitAdapter(struct ADAPTER *prAdapter,
	struct REG_INFO *prRegInfo,
	const u_int8_t bAtResetFlow);

void wlanCardEjected(struct ADAPTER *prAdapter);

void wlanIST(struct ADAPTER *prAdapter, bool fgEnInt);

u_int8_t wlanISR(struct ADAPTER *prAdapter, u_int8_t fgGlobalIntrCtrl);

uint32_t wlanProcessCommandQueue(struct ADAPTER *prAdapter,
				 struct QUE *prCmdQue);

uint32_t wlanSendCommand(struct ADAPTER *prAdapter,
			 struct CMD_INFO *prCmdInfo);

#if CFG_SUPPORT_MULTITHREAD
uint32_t wlanSendCommandMthread(struct ADAPTER *prAdapter,
				struct CMD_INFO *prCmdInfo);

uint32_t wlanTxCmdMthread(struct ADAPTER *prAdapter);

u_int8_t wlanIfCmdDbgEn(struct ADAPTER *prAdapter);

uint32_t wlanTxCmdDoneMthread(struct ADAPTER *prAdapter);

void wlanClearTxCommandQueue(struct ADAPTER *prAdapter);

void wlanClearTxCommandDoneQueue(struct ADAPTER *prAdapter);

void wlanClearDataQueue(struct ADAPTER *prAdapter);

void wlanClearRxToOsQueue(struct ADAPTER *prAdapter);
#endif

void wlanClearPendingCommandQueue(struct ADAPTER *prAdapter);

void wlanReleaseCommand(struct ADAPTER *prAdapter,
			struct CMD_INFO *prCmdInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void wlanReleaseCommandEx(struct ADAPTER *prAdapter,
			struct CMD_INFO *prCmdInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus,
			u_int8_t fgIsNeedHandler);

void wlanReleasePendingOid(struct ADAPTER *prAdapter,
			   uintptr_t ulParamPtr);

void wlanReleasePendingCMDbyBssIdx(struct ADAPTER *prAdapter,
				   uint8_t ucBssIndex);

void wlanReturnPacketDelaySetup(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_RETURN_TASK == 1)
void wlanReturnPacketDelaySetupTasklet(uintptr_t data);
#endif

void wlanReturnPacketDelaySetupTimeout(struct ADAPTER *prAdapter,
				       uintptr_t ulParamPtr);

void wlanReturnPacket(struct ADAPTER *prAdapter, void *pvPacket);

uint32_t
wlanQueryInformation(struct ADAPTER *prAdapter,
		     PFN_OID_HANDLER_FUNC pfOidQryHandler,
		     void *pvInfoBuf, uint32_t u4InfoBufLen,
		     uint32_t *pu4QryInfoLen);

uint32_t
wlanSetInformation(struct ADAPTER *prAdapter,
		   PFN_OID_HANDLER_FUNC pfOidSetHandler,
		   void *pvInfoBuf, uint32_t u4InfoBufLen,
		   uint32_t *pu4SetInfoLen);

uint32_t wlanAdapterStart(struct ADAPTER *prAdapter,
			  struct REG_INFO *prRegInfo,
			  const u_int8_t bAtResetFlow);

uint32_t wlanAdapterStop(struct ADAPTER *prAdapter,
		const u_int8_t bAtResetFlow);

void wlanCheckAsicCap(struct ADAPTER *prAdapter);

uint32_t wlanCheckWifiFunc(struct ADAPTER *prAdapter,
			   u_int8_t fgRdyChk);

void wlanReturnRxPacket(void *pvAdapter, void *pvPacket);

void wlanRxSetBroadcast(struct ADAPTER *prAdapter,
			u_int8_t fgEnableBroadcast);

u_int8_t wlanIsHandlerNeedHwAccess(PFN_OID_HANDLER_FUNC pfnOidHandler,
				   u_int8_t fgSetInfo);

void wlanSetPromiscuousMode(struct ADAPTER *prAdapter,
			    u_int8_t fgEnablePromiscuousMode);

uint32_t wlanSendDummyCmd(struct ADAPTER *prAdapter,
			  u_int8_t fgIsReqTxRsrc);

uint32_t wlanSendNicPowerCtrlCmd(struct ADAPTER *prAdapter,
				 uint8_t ucPowerMode);

u_int8_t wlanIsHandlerAllowedInRFTest(PFN_OID_HANDLER_FUNC pfnOidHandler,
				      u_int8_t fgSetInfo);

uint32_t wlanProcessQueuedMsduInfo(struct ADAPTER *prAdapter,
				   struct MSDU_INFO *prMsduInfoListHead);

u_int8_t wlanoidTimeoutCheck(struct ADAPTER *prAdapter,
			     PFN_OID_HANDLER_FUNC pfnOidHandler);

void wlanoidClearTimeoutCheck(struct ADAPTER *prAdapter);

uint32_t wlanUpdateNetworkAddress(struct ADAPTER *prAdapter);

uint32_t wlanUpdateBasicConfig(struct ADAPTER *prAdapter);

u_int8_t wlanQueryTestMode(struct ADAPTER *prAdapter);

u_int8_t wlanProcessTxFrame(struct ADAPTER *prAdapter,
			    void *prPacket);

uint32_t wlanGetThreadWakeUp(struct ADAPTER *prAdapter);

uint32_t wlanGetTxdAppendSize(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* OID/IOCTL Handling                                                         */
/*----------------------------------------------------------------------------*/
void wlanClearScanningResult(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void wlanClearBssInScanningResult(struct ADAPTER *prAdapter,
				  uint8_t *arBSSID);

#if CFG_TEST_WIFI_DIRECT_GO
void wlanEnableP2pFunction(struct ADAPTER *prAdapter);

void wlanEnableATGO(struct ADAPTER *prAdapter);
#endif

/*----------------------------------------------------------------------------*/
/* NIC Capability Retrieve by Polling                                         */
/*----------------------------------------------------------------------------*/
uint32_t wlanQueryNicCapability(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* PD MCR Retrieve by Polling                                                 */
/*----------------------------------------------------------------------------*/
uint32_t wlanQueryPdMcr(struct ADAPTER *prAdapter,
			struct PARAM_MCR_RW_STRUCT *prMcrRdInfo);

/*----------------------------------------------------------------------------*/
/* Get minimal tx power setting from NVRAM */
/*----------------------------------------------------------------------------*/
uint32_t wlanGetMiniTxPower(struct ADAPTER *prAdapter,
				  enum ENUM_BAND eBand,
				  enum ENUM_PHY_MODE_TYPE ePhyMode,
				  int8_t *pTxPwr);
/*----------------------------------------------------------------------------*/
/* Loading Manufacture Data                                                   */
/*----------------------------------------------------------------------------*/
uint32_t wlanLoadManufactureData(struct ADAPTER *prAdapter,
				 struct REG_INFO *prRegInfo);

/*----------------------------------------------------------------------------*/
/* Timer Timeout Check (for Glue Layer)                                       */
/*----------------------------------------------------------------------------*/
uint32_t wlanTimerTimeoutCheck(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* Mailbox Message Check (for Glue Layer)                                     */
/*----------------------------------------------------------------------------*/
uint32_t wlanProcessMboxMessage(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* TX Pending Packets Handling (for Glue Layer)                               */
/*----------------------------------------------------------------------------*/
uint32_t wlanEnqueueTxPacket(struct ADAPTER *prAdapter,
			     void *prNativePacket);

uint32_t wlanFlushTxPendingPackets(struct ADAPTER *prAdapter);

uint32_t wlanTxPendingPackets(struct ADAPTER *prAdapter,
			      u_int8_t *pfgHwAccess);

/*----------------------------------------------------------------------------*/
/* Low Power Acquire/Release (for Glue Layer)                                 */
/*----------------------------------------------------------------------------*/
uint32_t wlanAcquirePowerControl(struct ADAPTER *prAdapter);

uint32_t wlanReleasePowerControl(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* Pending Packets Number Reporting (for Glue Layer)                          */
/*----------------------------------------------------------------------------*/
uint32_t wlanGetTxPendingFrameCount(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* ACPI state inquiry (for Glue Layer)                                        */
/*----------------------------------------------------------------------------*/
enum ENUM_ACPI_STATE wlanGetAcpiState(struct ADAPTER *prAdapter);

void wlanSetAcpiState(struct ADAPTER *prAdapter,
		      enum ENUM_ACPI_STATE ePowerState);


/*----------------------------------------------------------------------------*/
/* get ECO version from Revision ID register (for Win32)                      */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetEcoVersion(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* get Rom version                                                            */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetRomVersion(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* set preferred band configuration corresponding to network type             */
/*----------------------------------------------------------------------------*/
void wlanSetPreferBandByNetwork(struct ADAPTER *prAdapter,
				enum ENUM_BAND eBand, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* get currently operating channel information                                */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetChannelNumberByNetwork(struct ADAPTER *prAdapter,
				      uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* get currently operating band information                                */
/*----------------------------------------------------------------------------*/
uint32_t wlanGetBandIndexByNetwork(struct ADAPTER *prAdapter,
				uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* check for system configuration to generate message on scan list            */
/*----------------------------------------------------------------------------*/
uint32_t wlanCheckSystemConfiguration(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* query bss statistics information from driver and firmware                  */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryBssStatistics(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);

/*----------------------------------------------------------------------------*/
/* dump per-BSS statistics            */
/*----------------------------------------------------------------------------*/
void wlanDumpBssStatistics(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* query sta statistics information from driver and firmware                  */
/*----------------------------------------------------------------------------*/

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
uint32_t
wlanQueryStatsOneCmd(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen, uint8_t fgIsOid,
			   uint8_t ucBssIndex);
#endif

void wlanDumpAllBssStatistics(struct ADAPTER *prAdapter);

uint32_t
wlanoidQueryStaStatistics(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanQueryStaStatistics(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen,
		       u_int8_t fgIsOid);

uint32_t
updateStaStats(struct ADAPTER *prAdapter,
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics);

uint32_t
wlanQueryStatistics(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen, uint8_t fgIsOid);

/*----------------------------------------------------------------------------*/
/* query NIC resource information from chip and reset Tx resource for normal  */
/* operation                                                                  */
/*----------------------------------------------------------------------------*/
void wlanQueryNicResourceInformation(struct ADAPTER *prAdapter);

uint32_t wlanQueryNicCapabilityV2(struct ADAPTER *prAdapter);

void wlanUpdateNicResourceInformation(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* GET/SET BSS index mapping for network interfaces                           */
/*----------------------------------------------------------------------------*/
void wlanBindNetInterface(struct GLUE_INFO *prGlueInfo,
			  uint8_t ucNetInterfaceIndex,
			  void *pvNetInterface);

void wlanBindBssIdxToNetInterface(struct GLUE_INFO *prGlueInfo,
				  uint8_t ucBssIndex,
				  void *pvNetInterface);

uint8_t wlanGetBssIdxByNetInterface(struct GLUE_INFO *prGlueInfo,
				    void *pvNetInterface);

void *wlanGetNetInterfaceByBssIdx(struct GLUE_INFO *prGlueInfo,
				  uint8_t ucBssIndex);

/* for windows as windows glue cannot see through P_ADAPTER_T */

void wlanInitFeatureOption(struct ADAPTER *prAdapter);
void wlanInitFeatureOptionImpl(struct ADAPTER *prAdapter, uint8_t *pucKey);

void wlanCfgSetSwCtrl(struct ADAPTER *prAdapter);

void wlanCfgSetChip(struct ADAPTER *prAdapter);

void wlanCfgSetDebugLevel(struct ADAPTER *prAdapter);

void wlanCfgSetCountryCode(struct ADAPTER *prAdapter);

struct WLAN_CFG_ENTRY *wlanCfgGetEntry(struct ADAPTER *prAdapter,
				       const int8_t *pucKey,
				       uint32_t u4Flags);

uint32_t
wlanCfgGet(struct ADAPTER *prAdapter, const int8_t *pucKey, int8_t *pucValue,
	   int8_t *pucValueDef, uint32_t u4Flags,
	   enum ENUM_FEATURE_SUPPORT_SCOPE fgIsDebugUsed);

void wlanCfgRecordValue(struct ADAPTER *prAdapter,
			const int8_t *pucKey, uint32_t u4Value);

uint32_t wlanCfgGetUint32(struct ADAPTER *prAdapter, const int8_t *pucKey,
			  uint32_t u4ValueDef,
			  enum ENUM_FEATURE_SUPPORT_SCOPE fgIsDebugUsed);

int32_t wlanCfgGetInt32(struct ADAPTER *prAdapter, const int8_t *pucKey,
			int32_t i4ValueDef,
			enum ENUM_FEATURE_SUPPORT_SCOPE fgIsDebugUsed);

uint32_t wlanCfgSetUint32(struct ADAPTER *prAdapter, const int8_t *pucKey,
			  uint32_t u4Value);

uint32_t wlanCfgSet(struct ADAPTER *prAdapter, const int8_t *pucKey,
		    int8_t *pucValue, uint32_t u4Flags);

#if CFG_SUPPORT_EASY_DEBUG
uint32_t wlanCfgParse(struct ADAPTER *prAdapter, uint8_t *pucConfigBuf,
		      uint32_t u4ConfigBufLen, u_int8_t isFwConfig);
void wlanFeatureToFw(struct ADAPTER *prAdapter, uint32_t u4Flag,
	uint8_t *pucKey);
#endif

void wlanLoadDefaultCustomerSetting(struct ADAPTER *prAdapter);

uint32_t wlanCfgInit(struct ADAPTER *prAdapter, uint8_t *pucConfigBuf,
		     uint32_t u4ConfigBufLen, uint32_t u4Flags);

void wlanCfgParseArgument(int8_t *cmdLine, int32_t *argc, int8_t *argv[]);

#if CFG_WOW_SUPPORT
uint32_t wlanCfgParseArgumentLong(int8_t *cmdLine, int32_t *argc,
				  int8_t *argv[]);
#endif

#if CFG_SUPPORT_IOT_AP_BLOCKLIST
void wlanCfgLoadIotApRule(struct ADAPTER *prAdapter);
void wlanCfgDumpIotApRule(struct ADAPTER *prAdapter);
#endif

int32_t wlanHexToNum(int8_t c);

int32_t wlanHexToByte(int8_t *hex);

int32_t wlanHexToArray(int8_t *hexString, int8_t *hexArray, uint8_t arrayLen);
int32_t wlanHexToArrayR(int8_t *hexString, int8_t *hexArray, uint8_t arrayLen);

int32_t wlanHwAddrToBin(int8_t *txt, uint8_t *addr);

int32_t wlanNumBitSet(uint32_t val);

u_int8_t wlanIsChipNoAck(struct ADAPTER *prAdapter);

u_int8_t wlanIsChipRstRecEnabled(struct ADAPTER *prAdapter);

u_int8_t wlanIsChipAssert(struct ADAPTER *prAdapter);

void wlanChipRstPreAct(struct ADAPTER *prAdapter);

#if CFG_SUPPORT_TX_LATENCY_STATS
void wlanCountTxDelayOverLimit(struct ADAPTER *prAdapter,
		enum ENUM_TX_OVER_LIMIT_DELAY_TYPE type,
		uint32_t u4Latency);
#endif

void wlanReportTxDelayOverLimit(struct ADAPTER *prAdapter,
	enum ENUM_TX_OVER_LIMIT_DELAY_TYPE type, uint32_t delay);

int wlanSetTxDelayOverLimitReport(struct ADAPTER *prAdapter,
		bool enable, bool isAverage,
		uint32_t interval, uint32_t driver_limit, uint32_t mac_limit);

void wlanTxProfilingTagPacket(struct ADAPTER *prAdapter, void *prPacket,
			      enum ENUM_TX_PROFILING_TAG eTag);

void wlanTxProfilingTagMsdu(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo,
			    enum ENUM_TX_PROFILING_TAG eTag);

#if CFG_ENABLE_PKT_LIFETIME_PROFILE
void wlanTxLifetimeTagPacket(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfoListHead,
			     enum ENUM_TX_PROFILING_TAG eTag);
#endif

#if (CFG_CE_ASSERT_DUMP == 1)
void wlanCorDumpTimerInit(struct ADAPTER *prAdapter);

void wlanCorDumpTimerReset(struct ADAPTER *prAdapter);

void wlanN9CorDumpTimeOut(struct ADAPTER *prAdapter,
			  uintptr_t ulParamPtr);

#endif

u_int8_t wlanGetWlanIdxByAddress(struct ADAPTER *prAdapter,
				 uint8_t *pucAddr, uint8_t *pucIndex);

uint8_t *wlanGetStaAddrByWlanIdx(struct ADAPTER *prAdapter,
				 uint8_t ucIndex);

struct WLAN_CFG_ENTRY *wlanCfgGetEntryByIndex(struct ADAPTER *prAdapter,
					      const uint8_t ucIdx,
					      uint32_t flag);

uint32_t wlanCfgGetTotalCfgNum(
	struct ADAPTER *prAdapter, uint32_t flag);


uint32_t wlanGetStaIdxByWlanIdx(struct ADAPTER *prAdapter,
				uint8_t ucIndex, uint8_t *pucStaIdx);
/*----------------------------------------------------------------------------*/
/* update per-AC statistics for LLS                */
/*----------------------------------------------------------------------------*/
void wlanUpdateTxStatistics(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo, u_int8_t fgTxDrop);

void wlanUpdateRxStatistics(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb);

uint32_t wlanPktTxDone(struct ADAPTER *prAdapter,
		       struct MSDU_INFO *prMsduInfo,
		       enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t wlanPowerOffWifi(struct ADAPTER *prAdapter);

void wlanPrintVersion(struct ADAPTER *prAdapter);

uint32_t wlanAccessRegister(struct ADAPTER *prAdapter,
	uint32_t u4Addr, uint32_t *pru4Result,
	uint32_t u4Data, uint8_t ucSetQuery);

uint32_t wlanSetChipEcoInfo(struct ADAPTER *prAdapter);
void wlanClearPendingInterrupt(struct ADAPTER *prAdapter);

#if CFG_WMT_WIFI_PATH_SUPPORT
extern int32_t mtk_wcn_wmt_wifi_fem_cfg_report(void *pvInfoBuf);
#endif

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
void set_wifi_test_mode_fwdl(const int mode);
uint8_t get_wifi_test_mode_fwdl(void);
void set_wifi_in_switch_mode(const int enabled);
uint8_t get_wifi_in_switch_mode(void);
#endif

#if ((CFG_SISO_SW_DEVELOP == 1) || (CFG_SUPPORT_SPE_IDX_CONTROL == 1))
uint8_t wlanGetAntPathType(struct ADAPTER *prAdapter,
			   enum ENUM_WF_PATH_FAVOR_T eWfPathFavor);
#endif

uint8_t wlanGetSpeIdx(struct ADAPTER *prAdapter,
		      uint8_t ucBssIndex,
		      enum ENUM_WF_PATH_FAVOR_T eWfPathFavor);

uint8_t wlanGetSupportNss(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t wlanGetSupportRxNss(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

#if CFG_SUPPORT_LOWLATENCY_MODE
uint32_t wlanAdapterStartForLowLatency(struct ADAPTER *prAdapter);
uint32_t wlanConnectedForLowLatency(struct ADAPTER *prAdapter,
				    uint8_t ucBssIndex);
uint32_t wlanSetLowLatencyMode(struct ADAPTER *prAdapter,
				uint32_t u4Events, uint8_t ucBssIndex);
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if CFG_SUPPORT_EASY_DEBUG
uint32_t wlanFwCfgParse(struct ADAPTER *prAdapter, uint8_t *pucConfigBuf);
#endif /* CFG_SUPPORT_EASY_DEBUG */

void wlanReleasePendingCmdById(struct ADAPTER *prAdapter, uint8_t ucCid);

uint32_t wlanDecimalStr2Hexadecimals(uint8_t *pucDecimalStr, uint16_t *pu2Out);

uint64_t wlanGetSupportedFeatureSet(struct GLUE_INFO *prGlueInfo);

uint32_t
wlanCalculateAllChannelDirtiness(struct ADAPTER *prAdapter);

void
wlanInitChnLoadInfoChannelList(struct ADAPTER *prAdapter);

uint8_t
wlanGetChannelIndex(enum ENUM_BAND band, uint8_t channel);

uint8_t
wlanGetChannelNumFromIndex(uint8_t ucIdx);

enum ENUM_BAND
wlanGetChannelBandFromIndex(uint8_t ucIdx);

void
wlanSortChannel(struct ADAPTER *prAdapter,
		enum ENUM_CHNL_SORT_POLICY ucSortType);

void wlanSuspendPmHandle(struct GLUE_INFO *prGlueInfo);
void wlanResumePmHandle(struct GLUE_INFO *prGlueInfo);
uint32_t wlanWakeUpWiFi(struct ADAPTER *prAdapter);

#if CFG_REPORT_MAX_TX_RATE
int wlanGetMaxTxRate(struct ADAPTER *prAdapter,
		 void *prBssPtr, struct STA_RECORD *prStaRec,
		 uint32_t *pu4CurRate, uint32_t *pu4MaxRate);
#endif /* CFG_REPORT_MAX_TX_RATE */

int wlanGetRxRateByBssid(struct GLUE_INFO *prGlueInfo, uint8_t ucBssIdx,
		uint32_t *pu4CurRate, uint32_t *pu4MaxRate,
		struct RxRateInfo *prRxRateInfo);
#ifdef CFG_SUPPORT_LINK_QUALITY_MONITOR
uint32_t wlanLinkQualityMonitor(struct GLUE_INFO *prGlueInfo, bool bFgIsOid);
void wlanFinishCollectingLinkQuality(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */
u_int8_t wlanIsDriverReady(struct GLUE_INFO *prGlueInfo, uint32_t u4Check);
void wlanOffUninitNicModule(struct ADAPTER *prAdapter,
				const u_int8_t bAtResetFlow);
void wlanOffClearAllQueues(struct ADAPTER *prAdapter);
int wlanQueryRateByTable(uint32_t txmode, uint32_t rate,
			uint32_t frmode, uint32_t sgi, uint32_t nsts,
			uint32_t *pu4CurRate, uint32_t *pu4MaxRate);

#if (CFG_SUPPORT_DATA_STALL && CFG_SUPPORT_LINK_QUALITY_MONITOR)
void wlanCustomMonitorFunction(struct ADAPTER *prAdapter,
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo, uint8_t ucBssIdx);
#endif /* CFG_SUPPORT_DATA_STALL */

uint8_t wlanCheckExtCapBit(struct STA_RECORD *prStaRec, uint8_t *pucIE,
	uint8_t ucTargetBit);

uint32_t wlanSetForceRTS(struct ADAPTER *prAdapter,
	u_int8_t fgEnForceRTS);

void
wlanBackupEmCfgSetting(struct ADAPTER *prAdapter);

void
wlanResoreEmCfgSetting(struct ADAPTER *prAdapter);

void
wlanCleanAllEmCfgSetting(struct ADAPTER *prAdapter);

u_int8_t wlanWfdEnabled(struct ADAPTER *prAdapter);

int wlanChipConfig(struct ADAPTER *prAdapter,
	char *pcCommand, int i4TotalLen);
int wlanChipCommand(struct ADAPTER *prAdapter,
	char *pcCommand, int i4TotalLen);

uint32_t wlanSetRxBaSize(struct GLUE_INFO *prGlueInfo,
	int8_t i4Type, uint16_t u2BaSize);
uint32_t wlanSetTxBaSize(struct GLUE_INFO *prGlueInfo,
	int8_t i4Type, uint16_t u2BaSize);

uint32_t wlanSetTxAggLimit(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen, uint8_t fgIsOid);

void
wlanGetTRXInfo(struct ADAPTER *prAdapter,
	struct TRX_INFO *prTRxInfo);

#if (CFG_WOW_SUPPORT == 1)
void wlanReleaseAllTxCmdQueue(struct ADAPTER *prAdapter);
void wlanWaitCfg80211SuspendDone(struct GLUE_INFO *prGlueInfo);
#endif

void wlanSetConnsysFwLog(struct ADAPTER *prAdapter);
uint32_t wlanSendFwLogControlCmd(struct ADAPTER *prAdapter,
				uint8_t ucCID,
				PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
				PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
				uint32_t u4SetQueryInfoLen,
				int8_t *pucInfoBuffer);

void wlanGetChipDbgOps(struct ADAPTER *prAdapter, uint32_t **pu4Handle);

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
uint32_t wlanSetEd(struct ADAPTER *prAdapter, int32_t i4EdVal2G,
					int32_t i4EdVal5G, uint32_t u4Sel);
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
void wlanRxMcsInfoMonitor(struct ADAPTER *prAdapter,
					uintptr_t ulParamPtr);
#endif

uint32_t wlanQueryThermalTemp(struct ADAPTER *ad,
	struct THERMAL_TEMP_DATA *data);

uint32_t wlanQueryThermalTempV2(struct ADAPTER *ad,
	struct THERMAL_TEMP_DATA_V2 *data);

uint32_t wlanSetRFTestModeCMD(struct GLUE_INFO *prGlueInfo, bool fgEn);

int8_t hexDigitToInt(uint8_t ch);

int wlanChipConfigWithType(struct ADAPTER *prAdapter,
	char *pcCommand, int i4TotalLen, uint8_t type);

#if CFG_SUPPORT_XONVRAM
uint32_t wlanTestModeXoCal(struct ADAPTER *ad,
	struct TEST_MODE_XO_CAL *data);
#endif /* CFG_SUPPORT_XONVRAM */

#if CFG_SUPPORT_PLCAL
uint32_t wlanTestModePlCal(struct ADAPTER *ad,
	struct TEST_MODE_PL_CAL *data);
#endif /* CFG_SUPPORT_PLCAL */

#endif /* _WLAN_LIB_H */
