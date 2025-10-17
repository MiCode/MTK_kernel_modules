/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Log: gl_vendor.h
 *
 * 10 14 2014
 * add vendor declaration
 *
 *
 */

#ifndef _GL_VENDOR_H
#define _GL_VENDOR_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include "wlan_lib.h"
#include "gl_wext.h"


/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define GOOGLE_OUI 0x001A11
#define OUI_QCA 0x001374
#define OUI_MTK 0x000CE7

/* QCA-OUI subcmds */
#define NL80211_VENDOR_SUBCMD_GET_PREFER_FREQ_LIST 103
#define NL80211_VENDOR_SUBCMD_ACS 54
#define NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_STARTED 56
#define NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_FINISHED 57
#define NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_ABORTED 58
#define NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_NOP_FINISHED 59
#define NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_RADAR_DETECTED 60
#define NL80211_VENDOR_SUBCMD_DFS_CAPABILITY 11
#define NL80211_VENDOR_SUBCMD_GET_FEATURES 55
#define QCA_NL80211_VENDOR_SUBCMD_ROAMING 9
#define QCA_NL80211_VENDOR_SUBCMD_ROAM 64
#define QCA_NL80211_VENDOR_SUBCMD_SETBAND 105
#define QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_START 122
#define QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_STOP 123
/* End of QCA-OUI subcmds */

#define NL80211_VENDOR_SUBCMD_GET_APF_CAPABILITIES 14
#define NL80211_VENDOR_SUBCMD_SET_PACKET_FILTER 15
#define NL80211_VENDOR_SUBCMD_READ_PACKET_FILTER 16
#define NL80211_VENDOR_SUBCMD_GET_TRX_STATS 48
#define MTK_NL80211_OP_MODE_CHANGE 14
#define MTK_NL80211_TRIGGER_RESET 15
#define COMB_MATRIX_LEN 6

#define NL80211_VENDOR_SUBCMD_SET_TX_LAT_MONTR_PARAM 49

#define NL80211_VENDOR_SUBCMD_GET_WFD_PRED_TX_BR 50
#define NL80211_VENDOR_SUBCMD_SET_WFD_TX_BR_MONTR 51

#define WIFI_VENDOR_ATTR_FEATURE_FLAGS 7
#define WIFI_VENDOR_DATA_OP_MODE_CHANGE(bssIdx, channelBw, TxNss, RxNss) \
	(uint32_t)((((bssIdx) << 24) + ((channelBw) << 16) + \
		((TxNss) << 8) + (RxNss)) & 0xffff)

#define LLS_RADIO_STAT_MAX_TX_LEVELS 256

enum NL80211_VENDOR_FEATURES {
	VENDOR_FEATURE_KEY_MGMT_OFFLOAD        = 0,
	VENDOR_FEATURE_SUPPORT_HW_MODE_ANY     = 1,
	VENDOR_FEATURE_OFFCHANNEL_SIMULTANEOUS = 2,
	VENDOR_FEATURE_P2P_LISTEN_OFFLOAD      = 3,
	VENDOR_FEATURE_OCE_STA                 = 4,
	VENDOR_FEATURE_OCE_AP                  = 5,
	VENDOR_FEATURE_OCE_STA_CFON            = 6,
	NUM_VENDOR_FEATURES /* keep last */
};

enum NL80211_VENDOR_CHIP_CAPABILITIES {
	MAX_MLO_ASSOCIATION_LINK_COUNT         = 1,
	MAX_MLO_STR_LINK_COUNT                 = 2,
	MAX_CONCURRENT_TDLS_SESSION_COUNT      = 3,

	NUM_CHIP_CAPABILITIES /* keep last */
};

enum ANDROID_VENDOR_SUB_COMMAND {
	/* Don't use 0 as a valid subcommand */
	ANDROID_NL80211_SUBCMD_UNSPECIFIED,

	/* Define all vendor startup commands between 0x0 and 0x0FFF */
	ANDROID_NL80211_SUBCMD_WIFI_RANGE_START = 0x0001,
	ANDROID_NL80211_SUBCMD_WIFI_RANGE_END	= 0x0FFF,

	/* Define all GScan related commands between 0x1000 and 0x10FF */
	ANDROID_NL80211_SUBCMD_GSCAN_RANGE_START = 0x1000,
	ANDROID_NL80211_SUBCMD_GSCAN_RANGE_END	 = 0x10FF,

	/* Define all RTT related commands between 0x1100 and 0x11FF */
	ANDROID_NL80211_SUBCMD_RTT_RANGE_START = 0x1100,
	ANDROID_NL80211_SUBCMD_RTT_RANGE_END   = 0x11FF,

	ANDROID_NL80211_SUBCMD_LSTATS_RANGE_START = 0x1200,
	ANDROID_NL80211_SUBCMD_LSTATS_RANGE_END   = 0x12FF,

	/* Define all Logger related commands between 0x1400 and 0x14FF */
	ANDROID_NL80211_SUBCMD_DEBUG_RANGE_START = 0x1400,
	ANDROID_NL80211_SUBCMD_DEBUG_RANGE_END	 = 0x14FF,

	/* Define all wifi offload related commands between 0x1600 and 0x16FF */
	ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START = 0x1600,
	ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_END	= 0x16FF,

	/* This is reserved for future usage */

};

enum WIFI_SUB_COMMAND {
	WIFI_SUBCMD_GET_CHANNEL_LIST = ANDROID_NL80211_SUBCMD_WIFI_RANGE_START,

	WIFI_SUBCMD_GET_FEATURE_SET,				/* 0x0002 */
	WIFI_SUBCMD_GET_FEATURE_SET_MATRIX,			/* 0x0003 */
	WIFI_SUBCMD_SET_PNO_RANDOM_MAC_OUI,			/* 0x0004 */
	WIFI_SUBCMD_NODFS_SET,					/* 0x0005 */
	WIFI_SUBCMD_SET_COUNTRY_CODE,				/* 0x0006 */
	WIFI_SUBCMD_SET_RSSI_MONITOR,				/* 0x0007 */

	/* Add more sub commands here */
	WIFI_SUBCMD_GET_ROAMING_CAPABILITIES,			/* 0x0008 */
	WIFI_SUBCMD_SET_ROAMING = 0x0009,			/* 0x0009 */
	WIFI_SUBCMD_CONFIG_ROAMING = 0x000a,			/* 0x000a */
	WIFI_SUBCMD_ENABLE_ROAMING,				/* 0x000b */
	WIFI_SUBCMD_SELECT_TX_POWER_SCENARIO,			/* 0x000c */
	WIFI_SUBCMD_SET_MULTISTA_PRIMARY_CONNECTION,		/* 0x000d */
	WIFI_SUBCMD_SET_MULTISTA_USE_CASE,			/* 0x000e */
	WIFI_SUBCMD_SET_SCAN_PARAM,				/* 0x000f */
	WIFI_SUBCMD_CHANNEL_POLICY,				/* 0x0010 */
};

enum RTT_SUB_COMMAND {
	RTT_SUBCMD_SET_CONFIG = ANDROID_NL80211_SUBCMD_RTT_RANGE_START,
	RTT_SUBCMD_CANCEL_CONFIG,
	RTT_SUBCMD_GETCAPABILITY,
};

enum LSTATS_SUB_COMMAND {
	LSTATS_SUBCMD_GET_INFO = ANDROID_NL80211_SUBCMD_LSTATS_RANGE_START,
};

/* moved from wifi_logger.cpp */
enum DEBUG_SUB_COMMAND {
	LOGGER_START_LOGGING = ANDROID_NL80211_SUBCMD_DEBUG_RANGE_START,
	LOGGER_GET_VER,
	LOGGER_DRIVER_MEM_DUMP,
};

enum WIFI_OFFLOAD_SUB_COMMAND {
	WIFI_OFFLOAD_START_MKEEP_ALIVE =
		ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START,
	WIFI_OFFLOAD_STOP_MKEEP_ALIVE,
};

/* MTK subcmds should be here */
enum MTK_WIFI_VENDOR_SUB_COMMAND {
	MTK_SUBCMD_TRIGGER_RESET = 1,
	MTK_SUBCMD_GET_RADIO_COMBO_MATRIX = 2,
	MTK_SUBCMD_NAN = 12,
	MTK_SUBCMD_CSI = 17,
	MTK_SUBCMD_NDP = 81,
	MTK_SUBCMD_GET_USABLE_CHANNEL = 82,
	MTK_SUBCMD_GET_CHIP_CAPABILITIES = 83,
	MTK_SUBCMD_GET_CHIP_CONCURRENCY_MATRIX = 84,
#if CFG_SUPPORT_WIFI_ADJUST_DTIM
	MTK_SUBCMD_SET_CHIP_DTIM_PERIOD = 85,
#endif

	MTK_SUBCMD_STRING_CMD = 0x2454,
};

enum WIFI_VENDOR_EVENT {
	GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS = 0,
	GSCAN_EVENT_HOTLIST_RESULTS_FOUND = 1,
	GSCAN_EVENT_SCAN_RESULTS_AVAILABLE = 2,
	GSCAN_EVENT_FULL_SCAN_RESULTS = 3,
	RTT_EVENT_COMPLETE = 4,
	GSCAN_EVENT_COMPLETE_SCAN = 5,
	GSCAN_EVENT_HOTLIST_RESULTS_LOST = 6,
	WIFI_EVENT_RSSI_MONITOR = 7,
	WIFI_EVENT_DRIVER_ERROR = 8,
	WIFI_EVENT_ACS = 9,
	WIFI_EVENT_GENERIC_RESPONSE = 10,
	WIFI_EVENT_BIGDATA_PIP = 11,
	WIFI_EVENT_OP_MODE_CHANGE = 12,
	WIFI_EVENT_DFS_OFFLOAD_CAC_STARTED = 13,
	WIFI_EVENT_DFS_OFFLOAD_CAC_FINISHED = 14,
	WIFI_EVENT_DFS_OFFLOAD_CAC_ABORTED = 15,
	WIFI_EVENT_DFS_OFFLOAD_CAC_NOP_FINISHED = 16,
	WIFI_EVENT_DFS_OFFLOAD_RADAR_DETECTED = 17,
	WIFI_EVENT_RESET_TRIGGERED = 18,
	WIFI_EVENT_SUBCMD_NAN = 19,
	WIFI_EVENT_SUBCMD_NDP = 20,
	WIFI_EVENT_SUBCMD_CSI = 21,
	WIFI_EVENT_P2P_LISTEN_OFFLOAD = 22,
	WIFI_EVENT_SUBCMD_GET_CHIP_CAPABILITIES = 23,
	/* Always add at the end.*/
};

enum WIFI_ATTRIBUTE {
	WIFI_ATTRIBUTE_BAND = 1,
	WIFI_ATTRIBUTE_NUM_CHANNELS,
	WIFI_ATTRIBUTE_CHANNEL_LIST,

	WIFI_ATTRIBUTE_NUM_FEATURE_SET,
	WIFI_ATTRIBUTE_FEATURE_SET,
	WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI,
	WIFI_ATTRIBUTE_NODFS_VALUE,
	WIFI_ATTRIBUTE_COUNTRY_CODE,

	WIFI_ATTRIBUTE_ROAMING_CAPABILITIES,
	WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_NUM,
	WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_BSSID,
	WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_NUM,
	WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_SSID,
	WIFI_ATTRIBUTE_ROAMING_STATE,
	WIFI_ATTRIBUTE_TX_POWER_SCENARIO,
	WIFI_ATTRIBUTE_CONCURRENCY_MATRIX,
	WIFI_ATTRIBUTE_MAX,
};

enum WIFI_STATS_ATTRIBUTE {
	WIFI_ATTRIBUTE_STATS_TX  = 0,
	WIFI_ATTRIBUTE_STATS_RX,
	WIFI_ATTRIBUTE_STATS_CGS,
	WIFI_ATTRIBUTE_STATS_TX_NUM,
	WIFI_ATTRIBUTE_STATS_TX_TAG_LIST,
	WIFI_ATTRIBUTE_STATS_RX_NUM,
	WIFI_ATTRIBUTE_STATS_RX_TAG_LIST,
	WIFI_ATTRIBUTE_STATS_CGS_NUM,
	WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST,
	WIFI_ATTRIBUTE_STATS_VERSION,
	WIFI_ATTRIBUTE_STATS_MAX,
};

enum WIFI_STA_CHANNEL_FOR_P2P_ATTRIBUTE {
	WIFI_ATTRIBUTE_STA_CHANNEL_FOR_P2P_INVALID,
	WIFI_ATTRIBUTE_ENABLE_STA_CHANNEL_FOR_P2P_ENABLE_FLAG,
	/* Add more attributes here */
	WIFI_ATTRIBUTE_ENABLE_STA_CHANNEL_FOR_P2P_MAX
};

enum CHANNEL_CATEROGY_MASK {
	INDOOR_CHANNEL = 1 << 0,
	DFS_CHANNEL = 1 << 1
};

#define TX_LAT_MONTR_INTVL_MIN		10
#define TX_LAT_MONTR_INTVL_MAX		5000
#define TX_LAT_MONTR_CRIT_MIN		1
#define TX_LAT_MONTR_CRIT_MAX		1000

enum WIFI_TX_LAT_MONTR_PARAMS_ATTRIBUTE {
	WIFI_ATTR_TX_LAT_MONTR_INVALID = 0,
	WIFI_ATTR_TX_LAT_MONTR_EN,
	WIFI_ATTR_TX_LAT_MONTR_INTVL,
	WIFI_ATTR_TX_LAT_MONTR_DRIVER_CRIT,
	WIFI_ATTR_TX_LAT_MONTR_MAC_CRIT,
	WIFI_ATTR_TX_LAT_MONTR_IS_AVG,
	WIFI_ATTR_TX_LAT_MONTR_MAX,
};

enum WIFI_WFD_TX_BR_MONTR_ATTRIBUTE {
	WIFI_ATTR_WFD_TX_BR_MONTR_INVALID = 0,
	WIFI_ATTR_WFD_TX_BR_MONTR_EN,
	WIFI_ATTR_WFD_TX_BR_MONTR_MAX,
};

enum WIFI_WFD_ATTRIBUTE {
	WIFI_ATTR_WFD_CUR_TX_BR    = 0,
	WIFI_ATTR_WFD_PRED_TX_BR   = 1,
	WIFI_ATTR_WFD_MAX
};

enum WIFI_RSSI_MONITOR_ATTRIBUTE {
	WIFI_ATTRIBUTE_RSSI_MONITOR_INVALID	  = 0,
	WIFI_ATTRIBUTE_RSSI_MONITOR_MAX_RSSI      = 1,
	WIFI_ATTRIBUTE_RSSI_MONITOR_MIN_RSSI      = 2,
	WIFI_ATTRIBUTE_RSSI_MONITOR_START	  = 3,
	WIFI_ATTRIBUTE_RSSI_MONITOR_ATTRIBUTE_MAX
};

enum MULTISTA_ATTRIBUTE {
	MULTISTA_ATTRIBUTE_INVALID,
	MULTISTA_ATTRIBUTE_PRIMARY_IFACE,
	MULTISTA_ATTRIBUTE_USE_CASE,
	/* Add more attributes here */
	MULTISTA_ATTRIBUTE_MAX
};

enum LOGGER_ATTRIBUTE {
	LOGGER_ATTRIBUTE_INVALID    = 0,
	LOGGER_ATTRIBUTE_DRIVER_VER = 1,
	LOGGER_ATTRIBUTE_FW_VER     = 2,
	LOGGER_ATTRIBUTE_MAX	    = 3
};

enum STRING_ATTRIBUTE {
	STRING_ATTRIBUTE_INVALID = 0,
	STRING_ATTRIBUTE_DATA    = 1,
	STRING_ATTRIBUTE_MAX     = 2
};

enum RTT_ATTRIBUTE {
	RTT_ATTRIBUTE_TARGET_CNT = 1,
	RTT_ATTRIBUTE_TARGET_INFO,
	RTT_ATTRIBUTE_TARGET_MAC,
	RTT_ATTRIBUTE_TARGET_TYPE,
	RTT_ATTRIBUTE_TARGET_PEER,
	RTT_ATTRIBUTE_TARGET_CHAN,
	RTT_ATTRIBUTE_TARGET_PERIOD,
	RTT_ATTRIBUTE_TARGET_NUM_BURST,
	RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST,
	RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTM,
	RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTMR,
	RTT_ATTRIBUTE_TARGET_LCI,
	RTT_ATTRIBUTE_TARGET_LCR,
	RTT_ATTRIBUTE_TARGET_BURST_DURATION,
	RTT_ATTRIBUTE_TARGET_PREAMBLE,
	RTT_ATTRIBUTE_TARGET_BW,
	RTT_ATTRIBUTE_RESULTS_COMPLETE			= 30,
	RTT_ATTRIBUTE_RESULTS_PER_TARGET		= 31,
	RTT_ATTRIBUTE_RESULT_CNT				= 32,
	RTT_ATTRIBUTE_RESULT					= 33,
	RTT_ATTRIBUTE_RESUTL_DETAIL				= 34,
	/* Add any new RTT_ATTRIBUTE prior to RTT_ATTRIBUTE_MAX */
	RTT_ATTRIBUTE_MAX
};

enum LSTATS_ATTRIBUTE {
	LSTATS_ATTRIBUTE_STATS = 2,
};

enum WIFI_MKEEP_ALIVE_ATTRIBUTE {
	MKEEP_ALIVE_ATTRIBUTE_INVALID = 0,
	MKEEP_ALIVE_ATTRIBUTE_ID,
	MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN,
	MKEEP_ALIVE_ATTRIBUTE_IP_PKT,
	MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR,
	MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR,
	MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC,
	MKEEP_ALIVE_ATTRIBUTE_MAX
};

#if (CFG_SUPPORT_APF == 1)
#define APF_VERSION		4
#define APF_MAX_PROGRAM_LEN	2048
#else
#define APF_VERSION		0
#define APF_MAX_PROGRAM_LEN	0
#endif

enum WIFI_APF_ATTRIBUTE {
	APF_ATTRIBUTE_INVALID = 0,
	APF_ATTRIBUTE_VERSION,
	APF_ATTRIBUTE_MAX_LEN,
	APF_ATTRIBUTE_PROGRAM,
	APF_ATTRIBUTE_PROGRAM_LEN,
	APF_ATTRIBUTE_MAX
};

/* QCA Vender CMD */
enum QCA_SET_BAND {
	QCA_SETBAND_AUTO,
	QCA_SETBAND_5G,
	QCA_SETBAND_2G,
	QCA_SETBAND_6G,
};

enum QCA_ATTR_ROAM_SUBCMD {
	QCA_ATTR_ROAM_SUBCMD_INVALID = 0,
	QCA_ATTR_ROAM_SUBCMD_SET_BLOCKLIST_BSSID = 6,

	/* keep last */
	QCA_ATTR_ROAM_SUBCMD_AFTER_LAST,
	QCA_ATTR_ROAM_SUBCMD_MAX =
	QCA_ATTR_ROAM_SUBCMD_AFTER_LAST - 1,
};

enum QCA_ATTR_ROAMING_PARAMS {
	QCA_ATTR_ROAMING_PARAM_INVALID = 0,

	QCA_ATTR_ROAMING_SUBCMD = 1,
	QCA_ATTR_ROAMING_REQ_ID = 2,

	/* Attribute for set_blocklist bssid params */
	QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS = 18,
	QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID = 19,
	QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID = 20,

	/* keep last */
	QCA_ATTR_ROAMING_PARAM_AFTER_LAST,
	QCA_ATTR_ROAMING_PARAM_MAX =
	QCA_ATTR_ROAMING_PARAM_AFTER_LAST - 1,
};

enum QCA_WLAN_ATTR {
	/* used by NL80211_VENDOR_SUBCMD_DFS_CAPABILITY */
	QCA_ATTR_DFS_CAPAB = 1,
	/* used by QCA_NL80211_VENDOR_SUBCMD_ROAMING, u32 with values defined
	 * by enum qca_roaming_policy.
	 */
	QCA_WLAN_VENDOR_ATTR_ROAMING_POLICY = 5,
	QCA_WLAN_VENDOR_ATTR_MAC_ADDR = 6,
	/* Unsigned 32-bit value from enum qca_set_band. The allowed values for
	 * this attribute are limited to QCA_SETBAND_AUTO, QCA_SETBAND_5G, and
	 * QCA_SETBAND_2G. This attribute is deprecated. Recommendation is to
	 * use QCA_WLAN_VENDOR_ATTR_SETBAND_MASK instead.
	 */
	QCA_WLAN_VENDOR_ATTR_SETBAND_VALUE = 12,
	/* Unsigned 32-bitmask value from enum qca_set_band. Substitutes the
	 * attribute QCA_WLAN_VENDOR_ATTR_SETBAND_VALUE for which only a subset
	 * of single values from enum qca_set_band are valid. This attribute
	 * uses bitmask combinations to define the respective allowed band
	 * combinations and this attributes takes precedence over
	 * QCA_WLAN_VENDOR_ATTR_SETBAND_VALUE if both attributes are included.
	 */
	QCA_WLAN_VENDOR_ATTR_SETBAND_MASK = 43,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_MAX = QCA_WLAN_VENDOR_ATTR_AFTER_LAST - 1,
};

enum WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST {
	WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_INVALID,
	WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_IFACE_TYPE,
	WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_GET,
	WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_LAST,
	WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_MAX =
		WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_LAST - 1
};

enum QCA_WLAN_VENDOR_ATTR_P2P_LISTEN_OFFLOAD {
	QCA_WLAN_VENDOR_ATTR_P2P_LO_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_CHANNEL,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_PERIOD,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_INTERVAL,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_COUNT,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_DEVICE_TYPES,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_VENDOR_IE,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_CTRL_FLAG,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_STOP_REASON,
	/* keep last */
	QCA_WLAN_VENDOR_ATTR_P2P_LO_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX =
	QCA_WLAN_VENDOR_ATTR_P2P_LO_AFTER_LAST - 1
};

enum WIFI_VENDOR_ATTR_ACS {
	WIFI_VENDOR_ATTR_ACS_CHANNEL_INVALID = 0,
	WIFI_VENDOR_ATTR_ACS_PRIMARY_CHANNEL = 1,
	WIFI_VENDOR_ATTR_ACS_SECONDARY_CHANNEL = 2,
	WIFI_VENDOR_ATTR_ACS_HW_MODE = 3,
	WIFI_VENDOR_ATTR_ACS_HT_ENABLED = 4,
	WIFI_VENDOR_ATTR_ACS_HT40_ENABLED = 5,
	WIFI_VENDOR_ATTR_ACS_VHT_ENABLED = 6,
	WIFI_VENDOR_ATTR_ACS_CHWIDTH = 7,
	WIFI_VENDOR_ATTR_ACS_CH_LIST = 8,
	WIFI_VENDOR_ATTR_ACS_VHT_SEG0_CENTER_CHANNEL = 9,
	WIFI_VENDOR_ATTR_ACS_VHT_SEG1_CENTER_CHANNEL = 10,
	WIFI_VENDOR_ATTR_ACS_FREQ_LIST = 11,
	WIFI_VENDOR_ATTR_ACS_PRIMARY_FREQUENCY = 12,
	WIFI_VENDOR_ATTR_ACS_SECONDARY_FREQUENCY = 13,
	WIFI_VENDOR_ATTR_ACS_VHT_SEG0_CENTER_FREQUENCY = 14,
	WIFI_VENDOR_ATTR_ACS_VHT_SEG1_CENTER_FREQUENCY = 15,
	WIFI_VENDOR_ATTR_ACS_ACS_EDMG_ENABLED = 16,
	WIFI_VENDOR_ATTR_ACS_ACS_EDMG_CHANNEL = 17,
	WIFI_VENDOR_ATTR_ACS_ACS_PUNCTURE_BITMAP = 18,
	WIFI_VENDOR_ATTR_ACS_ACS_EHT_ENABLED = 19,
	WIFI_VENDOR_ATTR_ACS_ACS_LAST_SCAN_AGEOUT_TIME = 20,

	/* keep last */
	WIFI_VENDOR_ATTR_ACS_AFTER_LAST,
	WIFI_VENDOR_ATTR_ACS_MAX =
		WIFI_VENDOR_ATTR_ACS_AFTER_LAST - 1
};
#if CFG_SUPPORT_NAN
enum WIFI_VENDOR_NAN {
	MTK_WLAN_VENDOR_ATTR_NAN = 2,
};
#endif

#define MAX_FW_ROAMING_BLOCKLIST_SIZE	16
#define MAX_FW_ROAMING_ALLOWLIST_SIZE	8

#if CFG_SUPPORT_DATA_STALL
enum WIFI_DATA_STALL_ATTRIBUTE {
	WIFI_ATTRIBUTE_ERROR_REASON = 0,
};
#endif

#if CFG_SUPPORT_BIGDATA_PIP
enum WIFI_BIGDATA_PIP_ATTRIBUTE {
	WIFI_ATTRIBUTE_PIP_PAYLOAD = 0,
};
#endif

enum WIFI_SCAN_PARAMS_ATTRIBUTE {
	WIFI_ATTR_SCAN_IFACE_TYPE = 0,
	WIFI_ATTR_SCAN_ASSOC_TYPE,
	WIFI_ATTR_SCAN_TYPE,
	WIFI_ATTR_SCAN_PROBE_NUM,
	WIFI_ATTR_SCAN_ACTIVE_TIME,
	WIFI_ATTR_SCAN_PASSIVE_TIME,
	WIFI_ATTR_SCAN_HOME_TIME,
	WIFI_ATTR_SCAN_ACTIVE_N_CH_BACK,
	WIFI_ATTR_SCAN_PASSIVE_N_CH_BACK,
	WIFI_ATTR_SCAN_MAX
};

#if CFG_SUPPORT_WIFI_ADJUST_DTIM
enum WIFI_SET_DTIM_PARAMS_ATTRIBUTE {
	WIFI_ATTR_SET_DTIM_PARAMS = 1,
	WIFI_ATTR_SET_DTIM_MAX
};
#endif

#if CFG_SUPPORT_DBDC
enum WIFI_OP_MODE_CHANGE_ATTRIBUTE {
	WIFI_ATTRIBUTE_OP_MODE_CHANGE = 0,
};
#endif

enum WIFI_RESET_TRIGGERED_ATTRIBUTE {
	WIFI_ATTRIBUTE_RESET_REASON = 1,
};

#if CFG_SUPPORT_CSI
enum WIFI_VENDOR_CSI {
	MTK_WLAN_VENDOR_ATTR_CSI = 5,
};

enum WIFI_CSI_ATTRIBUTE {
	WIFI_ATTRIBUTE_CSI_INVALID,
	WIFI_ATTRIBUTE_CSI_CONTROL_MODE,
	WIFI_ATTRIBUTE_CSI_CONFIG_ITEM,
	WIFI_ATTRIBUTE_CSI_VALUE_1,
	WIFI_ATTRIBUTE_CSI_VALUE_2,

	/* keep last */
	WIFI_ATTRIBUTE_CSI_AFTER_LAST,
	WIFI_ATTRIBUTE_CSI_MAX =
		WIFI_ATTRIBUTE_CSI_AFTER_LAST - 1
};
#endif

enum wifi_radio_combinations_matrix_attributes {
	WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_INVALID    = 0,
	WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_MATRIX     = 1,
		/* Add more attribute here */
	WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_MAX
};

enum WIFI_USABLE_CHANNEL_RESP_ATTRIBUTE {
	WIFI_ATTRIBUTE_USABLE_CHANNEL_RESP_INVALID,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_ARRAY,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_RESP_MAX
};


enum WIFI_USABLE_CHANNEL_REQ_ATTRIBUTE {
	WIFI_ATTRIBUTE_USABLE_CHANNEL_INVALID,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_BAND,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_IFACE,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_FILTER,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX_SIZE,
	WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX
};

#define MAX_IFACE_COMBINATIONS 16
#define MAX_IFACE_LIMITS 8

struct wifi_iface_limit {
	/* Max number of interfaces of same type */
	uint32_t max_limit;

	/* BIT mask of interfaces from wifi_interface_type */
	uint32_t iface_mask;
};

struct wifi_iface_combination {
	/* Maximum number of concurrent interfaces allowed in this
	 * combination
	 */
	uint32_t max_ifaces;

	/* Total number of interface limits in a combination */
	uint32_t num_iface_limits;

	/* Interface limits */
	struct wifi_iface_limit iface_limits[MAX_IFACE_LIMITS];
};

struct wifi_iface_concurrency_matrix {
	/* Total count of possible iface combinations */
	uint32_t num_iface_combinations;

	/* Interface combinations */
	struct wifi_iface_combination iface_combinations[
		MAX_IFACE_COMBINATIONS];
};

struct mtk_wifi_iface_combination {
	/* Maximum number of concurrent interfaces allowed in this
	 * combination
	 */
	uint32_t max_ifaces;

	/* Total number of interface limits in a combination */
	uint32_t num_iface_limits;

	/* Interface limits */
	struct wifi_iface_limit *iface_limits;
};

struct mtk_wifi_iface_concurrency_matrix {
	/* Total count of possible iface combinations */
	uint32_t num_iface_combinations;

	/* Interface combinations */
	struct mtk_wifi_iface_combination *iface_combinations;
};

enum wifi_interface_type {
	WIFI_INTERFACE_TYPE_STA        = 0,
	WIFI_INTERFACE_TYPE_AP         = 1,
	WIFI_INTERFACE_TYPE_P2P        = 2,
	WIFI_INTERFACE_TYPE_NAN        = 3,
	WIFI_INTERFACE_TYPE_AP_BRIDGED = 4,
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
extern const struct nla_policy mtk_scan_param_policy[
		WIFI_ATTR_SCAN_MAX + 1];
#if CFG_SUPPORT_WIFI_ADJUST_DTIM
extern const struct nla_policy mtk_set_dtim_param_policy[
		WIFI_ATTR_SET_DTIM_MAX + 1];
#endif
extern const struct nla_policy nla_parse_wifi_multista[
		MULTISTA_ATTRIBUTE_MAX + 1];
extern const struct nla_policy nla_parse_wifi_rssi_monitor[
		WIFI_ATTRIBUTE_RSSI_MONITOR_ATTRIBUTE_MAX + 1];
extern const struct nla_policy nla_parse_wifi_attribute[
		WIFI_ATTRIBUTE_MAX + 1];
extern const struct nla_policy qca_wlan_vendor_attr_policy[
		QCA_WLAN_VENDOR_ATTR_MAX + 1];
extern const struct nla_policy nla_get_version_policy[
		LOGGER_ATTRIBUTE_MAX + 1];
extern const struct nla_policy nla_parse_offloading_policy[
		MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC + 1];
extern const struct nla_policy nla_get_preferred_freq_list_policy[
		WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_MAX + 1];
extern const struct nla_policy nla_get_acs_policy[
		WIFI_VENDOR_ATTR_ACS_MAX + 1];

extern const struct nla_policy nla_string_cmd_policy[
		STRING_ATTRIBUTE_MAX + 1];

#if CFG_SUPPORT_MBO
extern const struct nla_policy qca_roaming_param_policy[
	QCA_ATTR_ROAMING_PARAM_MAX + 1];
#endif

extern const struct nla_policy nla_p2p_listen_offload_policy[
	QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX + 1];

extern const struct nla_policy nla_get_apf_policy[
		APF_ATTRIBUTE_MAX + 1];

extern const struct nla_policy nla_set_rtt_config_policy[
		RTT_ATTRIBUTE_TARGET_BW + 1];

#if CFG_SUPPORT_CSI
extern const struct nla_policy nla_get_csi_policy[
		WIFI_ATTRIBUTE_CSI_MAX + 1];
#endif

extern const struct nla_policy mtk_tx_lat_montr_param_policy[
		WIFI_ATTR_TX_LAT_MONTR_MAX + 1];

extern const struct nla_policy mtk_wfd_tx_br_montr_policy[
		WIFI_ATTR_WFD_TX_BR_MONTR_MAX + 1];

extern const struct nla_policy nla_trx_stats_policy[
	WIFI_ATTRIBUTE_STATS_MAX + 1];

extern const struct nla_policy mtk_usable_channel_policy[
	WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX + 1];

extern const struct nla_policy mtk_enable_sta_channel_for_peer_network_policy[
	WIFI_ATTRIBUTE_ENABLE_STA_CHANNEL_FOR_P2P_MAX + 1];
enum WIFIBAND {
	WIFIBAND_BAND_24GHZ = 1 << 0,
	WIFIBAND_BAND_5GHZ  = 1 << 1,
	WIFIBAND_BAND_6GHZ  = 1 << 2,
};

enum WIFIIFACE {
	/**
	 * Interface operation mode is client.
	 */
	IFACE_MODE_STA = 1 << 0,
	/**
	 * Interface operation mode is Hotspot.
	 */
	IFACE_MODE_SOFTAP = 1 << 1,
	/**
	 * Interface operation mode is Ad-Hoc network.
	 */
	IFACE_MODE_IBSS = 1 << 2,
	/**
	 * Interface operation mode is Wifi Direct Client.
	 */
	IFACE_MODE_P2P_CLIENT = 1 << 3,
	/**
	 * Interface operation mode is Wifi Direct Group Owner.
	 */
	IFACE_MODE_P2P_GO = 1 << 4,
	/**
	 * Interface operation mode is Aware.
	 */
	IFACE_MODE_NAN = 1 << 5,
	/**
	 * Interface operation mode is Mesh network.
	 */
	IFACE_MODE_MESH = 1 << 6,
	/**
	 * Interface operation mode is Tunneled Direct Link Setup.
	 */
	IFACE_MODE_TDLS = 1 << 7,
};

enum UsableChannelFilter {
	CELLULAR_COEXISTENCE = (1 << 0) /* 1 */,
	CONCURRENCY = (1 << 1) /* 2 */,
	NAN_INSTANT_MODE = (1 << 2) /* 4 */,
};

/*******************************************************************************
 *                           MACROS
 *******************************************************************************
 */

#if KERNEL_VERSION(3, 5, 0) <= LINUX_VERSION_CODE
/*
 * #define NLA_PUT(skb, attrtype, attrlen, data) \
 *	do { \
 *		if (unlikely(nla_put(skb, attrtype, attrlen, data) < 0)) \
 *			goto nla_put_failure; \
 *	} while (0)
 *
 *#define NLA_PUT_TYPE(skb, type, attrtype, value) \
 *	do { \
 *		type __tmp = value; \
 *		NLA_PUT(skb, attrtype, sizeof(type), &__tmp); \
 *	} while (0)
 */
#define NLA_PUT(skb, attrtype, attrlen, data) \
	mtk_cfg80211_NLA_PUT(skb, attrtype, attrlen, data)

#define NLA_PUT_TYPE(skb, type, attrtype, value) \
	mtk_cfg80211_nla_put_type(skb, type, attrtype, value)

#define NLA_PUT_U8(skb, attrtype, value) \
	NLA_PUT_TYPE(skb, NLA_PUT_DATE_U8, attrtype, value)

#define NLA_PUT_U16(skb, attrtype, value) \
	NLA_PUT_TYPE(skb, NLA_PUT_DATE_U16, attrtype, value)

#define NLA_PUT_U32(skb, attrtype, value) \
	NLA_PUT_TYPE(skb, NLA_PUT_DATE_U32, attrtype, value)

#define NLA_PUT_U64(skb, attrtype, value) \
	NLA_PUT_TYPE(skb, NLA_PUT_DATE_U64, attrtype, value)

#endif

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
#define NLA_PARSE_NESTED(nlattr, maxtype, nla, policy)	\
	nla_parse_nested(nlattr, maxtype, nla, policy, NULL)
#define NLA_PARSE(tb, maxtype, head, len, policy) \
	nla_parse(tb, maxtype, head, len, policy, NULL)
#else
#define NLA_PARSE_NESTED(nlattr, maxtype, nla, policy)	\
	nla_parse_nested(nlattr, maxtype, nla, policy)
#define NLA_PARSE(tb, maxtype, head, len, policy) \
	nla_parse(tb, maxtype, head, len, policy)
#endif

/*******************************************************************************
 *				P R I V A T E   D A T A
 *
 *******************************************************************************
 */
struct PARAM_WIFI_CHANGE_RESULT {
	uint16_t flags;
	uint16_t channel;
	uint8_t bssid[6];	/* BSSID */
	int8_t rssi[8];	/* RSSI history in db */
};

struct PARAM_AP_THRESHOLD {
	uint8_t bssid[6];	/* AP BSSID */
	int32_t low;	/* low threshold */
	int32_t high;	/* high threshold */
	uint32_t channel;	/* channel hint */
};

enum WIFI_MULTI_STA_USE_CASE {
	WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY = 0,
	WIFI_DUAL_STA_NON_TRANSIENT_UNBIASED = 1,
	WIFI_DUAL_STA_MTK_LEGACY = 15,
};

#if CFG_SUPPORT_LLS
#define STATS_LLS_MAX_NSS_NUM    2

#define STATS_LLS_CCK_NUM        4   /* 1M/2M/5.5M/11M */
#define STATS_LLS_OFDM_NUM       8   /* 6M/9M/12M/18M/24M/36M/48M/54M */
#define STATS_LLS_HT_NUM         16  /* MCS0~15 */
#define STATS_LLS_VHT_NUM        10  /* MCS0~9 */
#define STATS_LLS_HE_NUM         12  /* MCS0~11 */
#define STATS_LLS_EHT_NUM        16  /* MCS0~15 */

#define STATS_LLS_MAX_CCK_BW_NUM  1  /* BW20 */
#define STATS_LLS_MAX_OFDM_BW_NUM 1  /* BW20 */
#define STATS_LLS_MAX_HT_BW_NUM   2  /* BW20/40 */
#define STATS_LLS_MAX_VHT_BW_NUM  3  /* BW20/40/80 */
#define STATS_LLS_MAX_HE_BW_NUM   4  /* BW20/40/80/160 */
#define STATS_LLS_MAX_EHT_BW_NUM  5  /* BW20/40/80/160/320 */

#define STATS_LLS_MAX_CCK_NUM  (STATS_LLS_CCK_NUM)
#define STATS_LLS_MAX_OFDM_NUM (STATS_LLS_OFDM_NUM)
#define STATS_LLS_MAX_HT_NUM   \
	(STATS_LLS_HT_NUM * STATS_LLS_MAX_HT_BW_NUM)
#define STATS_LLS_MAX_VHT_NUM  \
	(STATS_LLS_VHT_NUM * STATS_LLS_MAX_VHT_BW_NUM * STATS_LLS_MAX_NSS_NUM)
#define STATS_LLS_MAX_HE_NUM   \
	(STATS_LLS_HE_NUM * STATS_LLS_MAX_HE_BW_NUM * STATS_LLS_MAX_NSS_NUM)
#if (CFG_SUPPORT_802_11BE == 1)
#define STATS_LLS_MAX_EHT_NUM   \
	(STATS_LLS_EHT_NUM * STATS_LLS_MAX_EHT_BW_NUM * STATS_LLS_MAX_NSS_NUM)
#else
#define STATS_LLS_MAX_EHT_NUM   0
#endif

#define TX_POWER_LEVELS 256

#define STATS_LLS_CH_NUM_2G4 14
#define STATS_LLS_CH_NUM_5G 32
#if CFG_SUPPORT_WIFI_6G
#define STATS_LLS_CH_NUM_6G 64
#else
#define STATS_LLS_CH_NUM_6G 0
#endif

#define STATS_LLS_CH_NUM (STATS_LLS_CH_NUM_2G4 +	\
			  STATS_LLS_CH_NUM_5G +		\
			  STATS_LLS_CH_NUM_6G)

#define STATS_LLS_RATE_NUM (STATS_LLS_MAX_CCK_NUM +	\
			    STATS_LLS_MAX_OFDM_NUM +	\
			    STATS_LLS_MAX_HT_NUM +	\
			    STATS_LLS_MAX_VHT_NUM +	\
			    STATS_LLS_MAX_HE_NUM +	\
			    STATS_LLS_MAX_EHT_NUM)

/**
 * channel operating width
 */
enum ENUM_STATS_LLS_CHANNEL_WIDTH {
	STATS_LLS_WIFI_CHAN_WIDTH_20    = 0,
	STATS_LLS_WIFI_CHAN_WIDTH_40    = 1,
	STATS_LLS_WIFI_CHAN_WIDTH_80    = 2,
	STATS_LLS_WIFI_CHAN_WIDTH_160   = 3,
	STATS_LLS_WIFI_CHAN_WIDTH_80P80 = 4,
	STATS_LLS_WIFI_CHAN_WIDTH_5     = 5,
	STATS_LLS_WIFI_CHAN_WIDTH_10    = 6,
	STATS_LLS_WIFI_CHAN_WIDTH_INVALID = -1
};

/**
 * @width: channel width (20, 40, 80, 80+80, 160)
 * @center_freq: primary 20 MHz channel
 * @center_freq0: center frequency (MHz) first segment
 * @center_freq1: center frequency (MHz) second segment
 */
struct STATS_LLS_CHANNEL_INFO {
	enum ENUM_STATS_LLS_CHANNEL_WIDTH width;
	int32_t center_freq;
	int32_t center_freq0;
	int32_t center_freq1;
};

/**
 * @channel: channel
 * @on_time: msecs the radio is awake
 *           (32 bits number accruing over time)
 * @cca_busy_time: msecs the CCA register is busy
 *                 (32 bits number accruing over time)
 */
struct STATS_LLS_CHANNEL_STAT {
	struct STATS_LLS_CHANNEL_INFO channel;
	uint32_t on_time;
	uint32_t cca_busy_time;
};

/**
 * Statistics data reported for STATS_LLS_TAG_RADIO_STAT.
 *
 * @radio: wifi radio (if multiple radio supported)
 * @on_time: msecs the radio is awake (32 bits number accruing over time)
 * @tx_time: msecs the radio is transmitting (32 bits number accruing over time)
 * @num_tx_levels: number of radio transmit power levels (unavailable)
 * @tx_time_per_levels: pointer to an array of radio transmit per power levels
 *                      in msecs accured over time (unavailable)
 * @rx_time: msecs the radio is in active receive
 *           (32 bits number accruing over time)
 * @on_time_scan: msecs the radio is awake due to all scan
 *                (32 bits number accruing over time)
 * @on_time_nbd: msecs the radio is awake due to NAN
 *               (32 bits number accruing over time)
 * @on_time_gscan: msecs the radio is awake due to G?scan
 *                 (32 bits number accruing over time)
 * @on_time_roam_scan: msecs the radio is awake due to roam?scan
 *                     (32 bits number accruing over time)
 * @on_time_pno_scan: msecs the radio is awake due to PNO scan
 *                    (32 bits number accruing over time)
 * @on_time_hs20: msecs the radio is awake due to HS2.0 scans and GAS exchange
 *                (32 bits number accruing over time)
 * @num_channels: number of channels
 * @RESERVED: for padding since sizeof this structure returns 56 bytes.
 * @channels: channel statistics
 */
struct STATS_LLS_WIFI_RADIO_STAT {
	int32_t radio;
	uint32_t on_time;
	uint32_t tx_time;
	uint32_t num_tx_levels; /* TX_POWER_LEVELS */
	uint32_t *tx_time_per_levels; /* NULL */
	uint32_t rx_time;
	uint32_t on_time_scan;
	uint32_t on_time_nbd;
	uint32_t on_time_gscan;
	uint32_t on_time_roam_scan;
	uint32_t on_time_pno_scan;
	uint32_t on_time_hs20;
	uint32_t num_channels;
	struct STATS_LLS_CHANNEL_STAT channels[];
};


enum STATS_LLS_WIFI_RATE_PREAMBLE {
	LLS_MODE_OFDM,
	LLS_MODE_CCK,
	LLS_MODE_HT,
	LLS_MODE_VHT,
	LLS_MODE_HE,
	LLS_MODE_EHT,
	LLS_MODE_RESERVED,
};

/**
 * wifi rate
 *
 * @preamble: 0: OFDM, 1:CCK, 2:HT 3:VHT 4:HE 5..7 reserved
 * @nss: 0:1x1, 1:2x2, 3:3x3, 4:4x4
 * @bw: 0:20MHz, 1:40Mhz, 2:80Mhz, 3:160Mhz
 * @rateMcsIdx: OFDM/CCK rate code would be as per ieee std in the units of
 *              0.5mbps
 *              HT/VHT/HE it would be mcs index
 * @reserved: reserved
 * @bitrate: units of 100 Kbps
 */
struct STATS_LLS_WIFI_RATE {
	uint32_t preamble   :3;
	uint32_t nss        :2;
	uint32_t bw         :3;
	uint32_t rateMcsIdx :8;

	uint32_t reserved   :16;
	uint32_t bitrate;
};

/**
 * per rate statistics
 *
 * @rate: rate information
 * @tx_mpdu: number of successfully transmitted data pkts (ACK rcvd)
 * @rx_mpdu: number of received data pkts
 * @mpdu_lost: number of data packet losses (no ACK)
 * @retries: total number of data pkt retries
 * @retries_short: number of short data pkt retries
 * @retries_long: number of long data pkt retries
 */
struct STATS_LLS_RATE_STAT {
	struct STATS_LLS_WIFI_RATE rate;
	uint32_t tx_mpdu;
	uint32_t rx_mpdu;
	uint32_t mpdu_lost;
	uint32_t retries;
	uint32_t retries_short;
	uint32_t retries_long;
};

enum ENUM_WIFI_CONNECTION_STATE {
	WIFI_DISCONNECTED = 0,
	WIFI_AUTHENTICATING = 1,
	WIFI_ASSOCIATING = 2,
	WIFI_ASSOCIATED = 3,
	WIFI_EAPOL_STARTED = 4,   /* if done by firmware/driver */
	WIFI_EAPOL_COMPLETED = 5, /* if done by firmware/driver */
};

enum ENUM_WIFI_ROAM_STATE {
	WIFI_ROAMING_IDLE = 0,
	WIFI_ROAMING_ACTIVE = 1,
};

enum ENUM_WIFI_INTERFACE_MODE {
	WIFI_INTERFACE_STA = 0,
	WIFI_INTERFACE_SOFTAP = 1,
	WIFI_INTERFACE_IBSS = 2,
	WIFI_INTERFACE_P2P_CLIENT = 3,
	WIFI_INTERFACE_P2P_GO = 4,
	WIFI_INTERFACE_NAN = 5,
	WIFI_INTERFACE_MESH = 6,
	WIFI_INTERFACE_TDLS = 7,
	WIFI_INTERFACE_UNKNOWN = -1
};

/**
 * Link layer statistics interface information
 * @mode: interface mode
 * @mac_addr[6]: interface mac address (self)
 * @state: connection state (valid for STA, CLI only)
 * @roaming: roaming state
 * @capabilities: WIFI_CAPABILITY_XXX (self)
 * @ssid[33]: null terminated SSID
 * @bssid[6]: bssid
 * @ap_country_str[3]: country string advertised by AP
 * @country_str[3]: country string for this association
 * @time_slicing_duty_cycle_percent: if this iface is being served using time
 *                     slicing on a radio with one or more ifaces (i.e MCC),
 *                     then the duty cycle assigned to this iface in %.
 *                     If not using time slicing (i.e SCC or DBS), set to 100.
 *
 * This structure is common part of legacy STATS_LLS_WIFI_IFACE_STAT and
 * multi-link STATS_LLS_WIFI_IFACE_ML_STAT.
 *
 * Total size = 4 + 6 (+2) + 4 + 4 + 4 + 33 + 6 + 3 + 3 + 1 = 70 bytes
 */
struct WIFI_INTERFACE_LINK_LAYER_INFO {
	enum ENUM_WIFI_INTERFACE_MODE mode;
	uint8_t mac_addr[6];
	enum ENUM_WIFI_CONNECTION_STATE state;
	enum ENUM_WIFI_ROAM_STATE roaming;
	uint32_t capabilities;
	uint8_t ssid[33];
	uint8_t bssid[6];
	uint8_t ap_country_str[3];
	uint8_t country_str[3];
	uint8_t time_slicing_duty_cycle_percent;
};

/**
 * access categories
 */
enum ENUM_STATS_LLS_AC {
	STATS_LLS_WIFI_AC_VO  = 0,
	STATS_LLS_WIFI_AC_VI  = 1,
	STATS_LLS_WIFI_AC_BE  = 2,
	STATS_LLS_WIFI_AC_BK  = 3,
	STATS_LLS_WIFI_AC_MAX = 4,
};

/* wifi peer type */
enum ENUM_STATS_PEER_TYPE {
	STATS_LLS_WIFI_PEER_STA,
	STATS_LLS_WIFI_PEER_AP,
	STATS_LLS_WIFI_PEER_P2P_GO,
	STATS_LLS_WIFI_PEER_P2P_CLIENT,
	STATS_LLS_WIFI_PEER_NAN,
	STATS_LLS_WIFI_PEER_TDLS,
	STATS_LLS_WIFI_PEER_INVALID,
};

/**
 * per peer statistics
 *
 * @sta_count: station count
 * @chan_util: channel utilization
 * @reserved: reserved
 */
struct STATS_LLS_BSSLOAD_INFO {
	uint16_t sta_count;
	uint16_t chan_util;
	uint8_t reserved[4];
};

/**
 * Statistics data reported for STATS_LLS_TAG_PEER_STAT.
 *
 * @type: peer type (AP, TDLS, GO etc.)
 * @peer_mac_address[6]: mac address (unavailable)
 * @capabilities: peer WIFI_CAPABILITY_XXX (unavailable)
 * @bssload: STA count and CU
 * @num_rate: number of rates
 * @rate_stats: per rate statistics, number of entries  = num_rate
 */
struct STATS_LLS_PEER_INFO {
	enum ENUM_STATS_PEER_TYPE type;
	uint8_t peer_mac_address[6];
	uint32_t capabilities;
	struct STATS_LLS_BSSLOAD_INFO bssload;
	uint32_t num_rate;
	struct STATS_LLS_RATE_STAT rate_stats[]; /* structure revised */
};

/**
 * Statistics data reported for STATS_LLS_TAG_WMM_AC_STAT.
 *
 * @ac: access category (VI, VO, BE, BK)
 * @tx_mpdu: number of successfully transmitted unicast data pkts (ACK rcvd)
 * @rx_mpdu: number of received unicast data packets
 * @tx_mcast: number of successfully transmitted multicast data packets
 *            STA case: implies ACK received from AP for the unicast packet
 *            in which mcast pkt was sent
 * @rx_mcast: number of received multicast data packets
 * @rx_ampdu: number of received unicast a-mpdus;
 *              support of this counter is optional
 * @tx_ampdu: number of transmitted unicast a-mpdus;
 *              support of this counter is optional
 * @mpdu_lost: number of data pkt losses (no ACK)
 * @retries: total number of data pkt retries
 * @retries_short: number of short data pkt retries
 * @retries_long: number of long data pkt retries
 * @contention_time_min: data pkt min contention time (usecs)
 * @contention_time_max: data pkt max contention time (usecs)
 * @contention_time_avg: data pkt avg contention time (usecs)
 * @contention_num_samples: num of data pkts used for contention statistics
 */
struct STATS_LLS_WMM_AC_STAT {
	enum ENUM_STATS_LLS_AC ac;
	uint32_t tx_mpdu;
	uint32_t rx_mpdu;
	uint32_t tx_mcast;
	uint32_t rx_mcast;
	uint32_t rx_ampdu;
	uint32_t tx_ampdu;
	uint32_t mpdu_lost;
	uint32_t retries;
	uint32_t retries_short;
	uint32_t retries_long;
	uint32_t contention_time_min;
	uint32_t contention_time_max;
	uint32_t contention_time_avg;
	uint32_t contention_num_samples;
};

/**
 * Varioud sttes for the link
 * @WIFI_LINK_STATE_UNKNOWN: Chip does not support reporting the state of the
 *	link
 * @WIFI_LINK_STATE_NOT_IN_USE: Link has not been in use since laste report.
 *	It is placed in power save.
 *	All management, control and data frames for the MLO connection are
 *	carried over links. In this state the link will not listen to beacons
 *	even in DTIM period and does not perform any GTK/IGTK/BIGTK updates but
 *	remains associated.
 * @WIFI_LINK_STATE_IN_USE: Link is in use. In presence of traffic,it is set to
 *	be power active.
 *	When the traffic stops, the link will go into power save mode and will
 *	listen for beacons every DTIM period.
 */
enum wifi_link_state {
	WIFI_LINK_STATE_UNKNOWN = 0,
	WIFI_LINK_STATE_NOT_IN_USE = 1,
	WIFI_LINK_STATE_IN_USE = 2,
};

/**
 * Link statistics
 * @link_id: (NEW) Identifier for the link.
 * @state: (NEW) state for the link
 * @radio: (NEW) Radio on which link stats are sampled.
 * @frequency: (NEW) Frequency on which link is operating.
 *
 * @beacon_rx: access point beacon received count from connected AP
 * @average_tsf_offset: average beacon offset encountered (beacon_TSF - TBTT)
 *               The average_tsf_offset field is used so as to calculate the
 *               typical beacon contention time on the channel as well may be
 *               used to debug beacon synchronization and related power
 *               consumption issue
 * @leaky_ap_detected: indicate that this AP typically leaks packets
 *                     beyond the driver guard time.
 * @leaky_ap_avg_num_frames_leaked: average number of frame leaked by AP
 *                                  after frame with PM bit set was ACK'ed by AP
 * @leaky_ap_guard_time: guard time currently in force
 *                       (when implementing IEEE power management based on
 *                       frame control PM bit), How long driver waits before
 *                       shutting down the radio and after receiving an ACK
 *                       for a data frame with PM bit set)
 * @mgmt_rx: access point mgmt frames received count from connected AP
 *           (including Beacon)
 * @mgmt_action_rx: action frames received count
 * @mgmt_action_tx: action frames transmit count
 * @rssi_mgmt: access Point Beacon and Management frames RSSI (averaged)
 * @rssi_data: access Point Data Frames RSSI (averaged) from connected AP
 * @rssi_ack: access Point ACK RSSI (averaged) from connected AP
 * @ac[WIFI_AC_MAX]: per ac data packet statistics
 * @time_slicing_duty_cycle_percent: (NEW) If this link is being served using
 *	time slicing on a radio with one or more links, then the duty cycle
 *	assigned to this link in %.
 * @num_peers: number of peers
 * @peer_info[]: per peer statistics
 */
struct STATS_LLS_WIFI_LINK_STAT {
	uint8_t link_id; /* NEW, 1 byte (+ 3 bytes) */
	enum wifi_link_state state; /* NEW, 4 bytes, 2023/03/29 */
	int32_t radio; /* NEW, 4 bytes */
	uint32_t frequency; /* NEW, 4 bytes */
	uint32_t beacon_rx; /* 4 bytes */
	/* 4 bytes padding */
	uint64_t average_tsf_offset; /* 8 bytes */
	uint32_t leaky_ap_detected;
	uint32_t leaky_ap_avg_num_frames_leaked;
	uint32_t leaky_ap_guard_time;
	uint32_t mgmt_rx;
	uint32_t mgmt_action_rx;
	uint32_t mgmt_action_tx;
	int32_t rssi_mgmt;
	int32_t rssi_data;
	int32_t rssi_ack;
	struct STATS_LLS_WMM_AC_STAT ac[STATS_LLS_WIFI_AC_MAX];
	uint8_t time_slicing_duty_cycle_precent; /* NEW */
	uint32_t num_peers;
	struct STATS_LLS_PEER_INFO peer_info[];
};

/**
 * interface statistics, used by single link (Android T)
 *
 * @iface: wifi interface
 * @info: current state of the interface
 * @beacon_rx: access point beacon received count from connected AP
 * @average_tsf_offset: average beacon offset encountered (beacon_TSF - TBTT)
 *               The average_tsf_offset field is used so as to calculate the
 *               typical beacon contention time on the channel as well may be
 *               used to debug beacon synchronization and related power
 *               consumption issue
 * @leaky_ap_detected: indicate that this AP typically leaks packets
 *                     beyond the driver guard time.
 * @leaky_ap_avg_num_frames_leaked: average number of frame leaked by AP
 *                                  after frame with PM bit set was ACK'ed by AP
 * @leaky_ap_guard_time: guard time currently in force
 *                       (when implementing IEEE power management based on
 *                       frame control PM bit), How long driver waits before
 *                       shutting down the radio and after receiving an ACK
 *                       for a data frame with PM bit set)
 * @mgmt_rx: access point mgmt frames received count from connected AP
 *           (including Beacon)
 * @mgmt_action_rx: action frames received count
 * @mgmt_action_tx: action frames transmit count
 * @rssi_mgmt: access Point Beacon and Management frames RSSI (averaged)
 * @rssi_data: access Point Data Frames RSSI (averaged) from connected AP
 * @rssi_ack: access Point ACK RSSI (averaged) from connected AP
 * @ac[WIFI_AC_MAX]: per ac data packet statistics
 * @num_peers: number of peers
 * @peer_info[]: per peer statistics
 *
 * The bottom part from beacon_rx have been moved to STATS_LLS_WIFI_LINK_STAT
 * in the new multi-link netdev level structure STATS_LLS_WIFI_IFACE_ML_STAT.
 */
struct STATS_LLS_WIFI_IFACE_STAT {
	void *iface;
	struct WIFI_INTERFACE_LINK_LAYER_INFO info; /* 70 bytes + 2 bytes */
	uint32_t beacon_rx; /* + a hole of 4 bytes */
	uint64_t average_tsf_offset; /* 8-byte aligned */
	uint32_t leaky_ap_detected;
	uint32_t leaky_ap_avg_num_frames_leaked;
	uint32_t leaky_ap_guard_time;
	uint32_t mgmt_rx;
	uint32_t mgmt_action_rx;
	uint32_t mgmt_action_tx;
	int32_t rssi_mgmt;
	int32_t rssi_data;
	int32_t rssi_ack;
	struct STATS_LLS_WMM_AC_STAT ac[STATS_LLS_WIFI_AC_MAX];
	uint32_t num_peers;
	struct STATS_LLS_PEER_INFO peer_info[];
};

/* Multi link stats for interface, used by Android U
 *
 * @iface: wifi interface
 * @info: current state of the interface
 * @num_links: Number of links
 * @links: Stats per link
 */
struct STATS_LLS_WIFI_IFACE_ML_STAT {
	void *iface;
	struct WIFI_INTERFACE_LINK_LAYER_INFO info; /* 70 bytes + 2 bytes */
	int32_t num_links;
	struct STATS_LLS_WIFI_LINK_STAT links[MLD_LINK_MAX];
};


/* Shared EMI Memory layout */
struct PEER_INFO_RATE_STAT {
	struct STATS_LLS_PEER_INFO peer;
	struct STATS_LLS_RATE_STAT rate[STATS_LLS_RATE_NUM];
};

struct WIFI_RADIO_CHANNEL_STAT {
	struct STATS_LLS_WIFI_RADIO_STAT radio;
	struct STATS_LLS_CHANNEL_STAT channel[STATS_LLS_CH_NUM];
};

/* Buffer to hold collected data from FW reported EMI address */
struct HAL_LLS_FULL_REPORT {
	struct STATS_LLS_WIFI_IFACE_STAT iface;
	struct PEER_INFO_RATE_STAT peer_info[CFG_STA_REC_NUM];
	struct WIFI_RADIO_CHANNEL_STAT radio[ENUM_BAND_NUM];
	uint32_t tx_levels[ENUM_BAND_NUM][LLS_RADIO_STAT_MAX_TX_LEVELS];
};

/* Buffer to hold collected data from FW reported EMI address */
struct HAL_LLS_FULL_REPORT_V2 {
	struct STATS_LLS_WIFI_IFACE_ML_STAT iface;
	struct PEER_INFO_RATE_STAT peer_info[CFG_STA_REC_NUM];
	struct WIFI_RADIO_CHANNEL_STAT radio[ENUM_BAND_NUM];
	uint32_t tx_levels[ENUM_BAND_NUM][LLS_RADIO_STAT_MAX_TX_LEVELS];
};

struct STATS_LLS_PEER_AP_REC {
	uint16_t sta_count;
	uint16_t chan_util;
	uint8_t mac_addr[ETH_ALEN];
};
#endif /* CFG_SUPPORT_LLS */

struct ANDROID_T_COMB_UNIT {
	uint8_t band_0;
	uint8_t ant_0;
	uint8_t band_1;
	uint8_t ant_1;
};

struct ANDROID_T_COMB_MATRIX {
	struct ANDROID_T_COMB_UNIT comb_mtx[COMB_MATRIX_LEN];
};

enum ANDROID_USABLE_CHANNEL_WIDTH {
	ANDROID_WIFI_CHAN_WIDTH_20    = 0,
	ANDROID_WIFI_CHAN_WIDTH_40    = 1,
	ANDROID_WIFI_CHAN_WIDTH_80    = 2,
	ANDROID_WIFI_CHAN_WIDTH_160   = 3,
	ANDROID_WIFI_CHAN_WIDTH_80P80 = 4,
	ANDROID_WIFI_CHAN_WIDTH_5     = 5,
	ANDROID_WIFI_CHAN_WIDTH_10    = 6,
	ANDROID_WIFI_CHAN_WIDTH_320   = 7,
	ANDROID_WIFI_CHAN_WIDTH_INVALID = -1
};

struct ANDROID_USABLE_CHANNEL_UNIT {
	uint16_t channel_freq;
	enum ANDROID_USABLE_CHANNEL_WIDTH channel_width;
	uint32_t iface_mode_mask;
};

struct ANDROID_USABLE_CHANNEL_ARRAY {
	uint16_t array_size;
	struct ANDROID_USABLE_CHANNEL_UNIT channel_array[];
};

/* RTT Capabilities */
struct PARAM_WIFI_RTT_CAPABILITIES {
	/* if 1-sided rtt data collection is supported */
	uint8_t rtt_one_sided_supported;
	/* if ftm rtt data collection is supported */
	uint8_t rtt_ftm_supported;
	/* if initiator supports LCI request. Applies to 2-sided RTT */
	uint8_t lci_support;
	/* if initiator supports LCR request. Applies to 2-sided RTT */
	uint8_t lcr_support;
	/* bit mask indicates what preamble is supported by initiator */
	uint8_t preamble_support;
	/* bit mask indicates what BW is supported by initiator */
	uint8_t bw_support;
};

enum ENUM_NLA_PUT_DATE_TYPE {
	NLA_PUT_DATE_U8 = 0,
	NLA_PUT_DATE_U16,
	NLA_PUT_DATE_U32,
	NLA_PUT_DATE_U64,
};

/* RSSI Monitoring */
struct PARAM_RSSI_MONITOR_T {
	bool enable;	/* 1=Start, 0=Stop*/
	int8_t max_rssi_value;
	int8_t min_rssi_value;
	uint8_t reserved[1];
	uint8_t reserved2[4]; /* reserved for MT6632 */
};

struct PARAM_RSSI_MONITOR_EVENT {
	uint8_t version;
	int8_t rssi;
	uint8_t BSSID[PARAM_MAC_ADDR_LEN];
};

/* Packet Keep Alive */
struct PARAM_PACKET_KEEPALIVE_T {
	uint8_t enable;	/* 1=Start, 0=Stop*/
	uint8_t index;
	uint16_t u2IpPktLen;
	uint8_t pIpPkt[256];
	uint8_t ucSrcMacAddr[PARAM_MAC_ADDR_LEN];
	uint8_t ucDstMacAddr[PARAM_MAC_ADDR_LEN];
	uint32_t u4PeriodMsec;
	uint8_t reserved[8]; /* reserved for MT6632 */
};

struct PARAM_BSS_MAC_OUI {
	uint8_t ucBssIndex;
	uint8_t ucMacOui[MAC_OUI_LEN];
};

enum PARAM_GENERIC_RESPONSE_ID {
	GRID_MANAGE_CHANNEL_LIST = 0,
	GRID_HANG_INFO = 1,
	GRID_SWPIS_BCN_INFO = 2,
	GRID_SWPIS_BCN_INFO_ABORT = 3,
	GRID_EXTERNAL_AUTH = 4,
	GRID_SWPIS_CONNECTIVITY_LOG = 5,
	GRID_MANAGE_FREQ_LIST = 6,
	GRID_RESET_FT_PROCESS = 7,
};

struct PARAM_EXTERNAL_AUTH_INFO {
	uint8_t id;
	uint8_t len;
	uint8_t ssid[ELEM_MAX_LEN_SSID + 1];
	uint8_t ssid_len;
	uint8_t bssid[PARAM_MAC_ADDR_LEN];
	uint32_t key_mgmt_suite;
	uint32_t action;
	uint8_t dot11MultiLinkActivated;
	uint8_t own_ml_addr[PARAM_MAC_ADDR_LEN];
	uint8_t peer_ml_addr[PARAM_MAC_ADDR_LEN];
} __KAL_ATTRIB_PACKED__;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */



/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
int mtk_cfg80211_NLA_PUT(struct sk_buff *skb, int attrtype,
			 int attrlen, const void *data);

int mtk_cfg80211_nla_put_type(struct sk_buff *skb,
			      enum ENUM_NLA_PUT_DATE_TYPE type, int attrtype,
			      const void *value);

int mtk_cfg80211_vendor_get_capabilities(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_set_config(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   const void *data, int data_len);

int mtk_cfg80211_vendor_set_scan_config(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int data_len);

int mtk_cfg80211_vendor_set_significant_change(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_set_hotlist(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    const void *data, int data_len);

int mtk_cfg80211_vendor_enable_scan(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    const void *data, int data_len);

int mtk_cfg80211_vendor_enable_full_scan_results(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_get_scan_results(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_get_channel_list(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_set_country_code(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_get_rtt_capabilities(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_set_rtt_config(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_cancel_rtt_config(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

#if CFG_SUPPORT_LLS
uint8_t isValidRate(struct STATS_LLS_RATE_STAT *rate_stats,
	uint32_t ofdm_idx, uint32_t cck_idx);

uint32_t receivedMpduCount(struct STA_RECORD *sta_rec,
		struct STATS_LLS_RATE_STAT *rate_stats,
		uint32_t ofdm_idx, uint32_t cck_idx);

struct STA_RECORD *find_peer_starec(struct ADAPTER *prAdapter,
		struct STATS_LLS_PEER_INFO *peer_info);
#endif

int mtk_cfg80211_vendor_llstats_get_info(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_set_tx_lat_montr_param(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_set_band(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

int mtk_cfg80211_vendor_set_scan_mac_oui(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

#if CFG_SUPPORT_MBO
int mtk_cfg80211_vendor_set_roaming_param(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);
#endif

int mtk_cfg80211_vendor_set_roaming_policy(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_get_roaming_capabilities(
	struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_config_roaming(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       const void *data,
				       int data_len);

int mtk_cfg80211_vendor_enable_roaming(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       const void *data,
				       int data_len);

int mtk_cfg80211_vendor_set_rssi_monitoring(
	struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_packet_keep_alive_start(
	struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_packet_keep_alive_stop(
	struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_get_version(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    const void *data, int data_len);

int mtk_cfg80211_vendor_event_generic_response(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	uint32_t len, uint8_t *data);

int mtk_cfg80211_vendor_get_supported_feature_set(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_event_rssi_beyond_range(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIdx, int rssi);

int mtk_cfg80211_vendor_set_tx_power_scenario(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_set_multista_primary_connection(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_set_multista_use_case(
		struct wiphy *wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

#if CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST
int mtk_cfg80211_vendor_get_preferred_freq_list(struct wiphy
		*wiphy, struct wireless_dev *wdev, const void *data,
		int data_len);
#endif

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
int mtk_cfg80211_vendor_acs(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);
#endif

int mtk_cfg80211_vendor_p2p_listen_offload_start(
		struct wiphy *wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_p2p_listen_offload_stop(
		struct wiphy *wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_dfs_capability(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_get_features(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_get_chip_capabilities(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_get_chip_concurrency_matrix(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_get_apf_capabilities(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

#if (CFG_SUPPORT_APF == 1)
int mtk_cfg80211_vendor_set_packet_filter(
	struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_read_packet_filter(
	struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);
#endif /* CFG_SUPPORT_APF */

int mtk_cfg80211_vendor_driver_memory_dump(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data,
					   int data_len);

int mtk_cfg80211_vendor_set_scan_param(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);

#if CFG_SUPPORT_WIFI_ADJUST_DTIM
int mtk_cfg80211_vendor_set_dtim_param(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len);
#endif

int mtk_cfg80211_vendor_string_cmd(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len);

int mtk_cfg80211_vendor_get_trx_stats(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data,
					   int data_len);

int mtk_cfg80211_vendor_set_wfd_tx_br_montr(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data,
					   int data_len);
int mtk_cfg80211_vendor_get_wfd_pred_tx_br(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data,
					   int data_len);

int mtk_cfg80211_vendor_trigger_reset(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_enable_sta_channel_for_peer_network(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_comb_matrix(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_event_reset_triggered(
	uint32_t data);

#if CFG_SUPPORT_CSI
int mtk_cfg80211_vendor_csi_control(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);

int mtk_cfg80211_vendor_event_csi_raw_data(
	struct ADAPTER *prAdapter);
#endif

int mtk_cfg80211_vendor_get_usable_channel(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len);
#endif /* _GL_VENDOR_H */
