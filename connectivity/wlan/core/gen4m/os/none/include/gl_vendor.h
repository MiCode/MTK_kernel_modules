/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Log: gl_vendor.h
 *
 * 10 14 2014
 * add vendor declaration
 * some structure is needed by common part in this file
 * 1) struct PARAM_PACKET_KEEPALIVE_T needed by nic_cmd_event.h
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
#include "wlan_lib.h"


/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define GOOGLE_OUI 0x001A11
#define OUI_QCA 0x001374

#define NL80211_VENDOR_SUBCMD_GET_PREFER_FREQ_LIST 103
#define NL80211_VENDOR_SUBCMD_DFS_CAPABILITY 11
#define QCA_NL80211_VENDOR_SUBCMD_ROAMING 9
#define QCA_NL80211_VENDOR_SUBCMD_ROAM 64
#define QCA_NL80211_VENDOR_SUBCMD_SETBAND 105
#define QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_START 122
#define QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_STOP 123
#define QCA_NL80211_VENDOR_SUBCMD_PASN 215
#define QCA_NL80211_VENDOR_SUBCMD_SECURE_RANGING_CONTEXT 216
#define QCA_NL80211_VENDOR_SUBCMD_CONNECT_EXT 250

#define COMB_MATRIX_LEN 6

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

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
	WIFI_SUBCMD_ENABLE_ROAMING				/* 0x000b */
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
	LOGGER_GET_VER
};

enum WIFI_OFFLOAD_SUB_COMMAND {
	WIFI_OFFLOAD_START_MKEEP_ALIVE =
		ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START,
	WIFI_OFFLOAD_STOP_MKEEP_ALIVE,
};

enum WIFI_VENDOR_EVENT {
	GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS,
	GSCAN_EVENT_HOTLIST_RESULTS_FOUND,
	GSCAN_EVENT_SCAN_RESULTS_AVAILABLE,
	GSCAN_EVENT_FULL_SCAN_RESULTS,
	RTT_EVENT_COMPLETE,
	GSCAN_EVENT_COMPLETE_SCAN,
	GSCAN_EVENT_HOTLIST_RESULTS_LOST,
	WIFI_EVENT_RSSI_MONITOR
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
	WIFI_ATTRIBUTE_LATENCY_MODE,
	WIFI_ATTRIBUTE_MAX
};

enum WIFI_RSSI_MONITOR_ATTRIBUTE {
	WIFI_ATTRIBUTE_RSSI_MONITOR_INVALID	  = 0,
	WIFI_ATTRIBUTE_RSSI_MONITOR_MAX_RSSI      = 1,
	WIFI_ATTRIBUTE_RSSI_MONITOR_MIN_RSSI      = 2,
	WIFI_ATTRIBUTE_RSSI_MONITOR_START	  = 3,
	WIFI_ATTRIBUTE_RSSI_MONITOR_ATTRIBUTE_MAX
};

enum LOGGER_ATTRIBUTE {
	LOGGER_ATTRIBUTE_INVALID    = 0,
	LOGGER_ATTRIBUTE_DRIVER_VER = 1,
	LOGGER_ATTRIBUTE_FW_VER     = 2,
	LOGGER_ATTRIBUTE_MAX	    = 3
};

enum RTT_ATTRIBUTE {
	RTT_ATTRIBUTE_CAPABILITIES = 1,

	RTT_ATTRIBUTE_TARGET_CNT = 10,
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
	RTT_ATTRIBUTE_RESULTS_COMPLETE = 30,
	RTT_ATTRIBUTE_RESULTS_PER_TARGET,
	RTT_ATTRIBUTE_RESULT_CNT,
	RTT_ATTRIBUTE_RESULT
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

/* QCA Vender CMD */
enum QCA_SET_BAND {
	QCA_SETBAND_AUTO,
	QCA_SETBAND_5G,
	QCA_SETBAND_2G,
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

/* PASN */
enum QCA_WLAN_VENDOR_PASN_ACTION {
	QCA_WLAN_VENDOR_PASN_ACTION_AUTH,
	QCA_WLAN_VENDOR_PASN_ACTION_DELETE_SECURE_RANGING_CONTEXT,
};

enum QCA_WLAN_VENDOR_ATTR_PASN_PEER {
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_SRC_ADDR = 1,
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAC_ADDR = 2,
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_LTF_KEYSEED_REQUIRED = 3,
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_STATUS_SUCCESS = 4,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAX =
	QCA_WLAN_VENDOR_ATTR_PASN_PEER_AFTER_LAST - 1,
};

enum QCA_WLAN_VENDOR_ATTR_PASN {
	QCA_WLAN_VENDOR_ATTR_PASN_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_PASN_ACTION = 1,
	QCA_WLAN_VENDOR_ATTR_PASN_PEERS = 2,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_PASN_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_PASN_MAX =
	QCA_WLAN_VENDOR_ATTR_PASN_AFTER_LAST - 1,
};

enum QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION {
	QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION_ADD,
	QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION_DELETE,
};

enum QCA_WLAN_VENDOR_SHA_TYPE {
	QCA_WLAN_VENDOR_SHA_256,
	QCA_WLAN_VENDOR_SHA_384,
};

enum QCA_WLAN_VENDOR_ATTR_SECYRE_RANGING_CTX {
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_ACTION = 1,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SRC_ADDR = 2,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_PEER_MAC_ADDR = 3,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SHA_TYPE = 4,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_TK = 5,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_CIPHER = 6,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_LTF_KEYSEED = 7,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_MAX =
	QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_AFTER_LAST - 1,
};

enum QCA_WLAN_CONNECT_EXT_FEATURES {
	QCA_CONNECT_EXT_FEATURE_RSNO	= 0,
	NUM_QCA_CONNECT_EXT_FEATURES /* keep last */
};

enum QCA_WLAN_VENDOR_ATTR_CONNECT_EXT {
	QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_FEATURES = 1,

	QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_MAX =
	QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_AFTER_LAST - 1,
};

enum wifi_radio_combinations_matrix_attributes {
	WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_INVALID    = 0,
	WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_MATRIX     = 1,
		/* Add more attribute here */
	WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_MAX
};

enum WIFI_MLO_MODE_ATTRIBUTE {
	MTK_MLO_MODE_ATTRIBUTE_INVALID,
	MTK_MLO_MODE_ATTRIBUTE_MLO_MODE,
	MTK_MLO_MODE_ATTRIBUTE_MAX
};

#define MAX_FW_ROAMING_BLOCKLIST_SIZE	16
#define MAX_FW_ROAMING_ALLOWLIST_SIZE	8

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           MACROS
 *******************************************************************************
 */
/*
 * TODO: function is os-related, while may depend on the purpose of function
 * to implement on other os
 */
#if 0
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
	bool enable;	/* 1=Start, 0=Stop*/
	uint8_t index;
	int16_t u2IpPktLen;
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

#if CFG_SUPPORT_LLS
/* channel statistics */
struct WIFI_CHANNEL_STAT {
	struct WIFI_CHANNEL_INFO channel;
	uint32_t on_time;
	uint32_t cca_busy_time;
};

/* radio statistics */
struct WIFI_RADIO_STAT {
	uint32_t radio;
	uint32_t on_time;
	uint32_t tx_time;
	uint32_t rx_time;
	uint32_t on_time_scan;
	uint32_t on_time_nbd;
	uint32_t on_time_gscan;
	uint32_t on_time_roam_scan;
	uint32_t on_time_pno_scan;
	uint32_t on_time_hs20;
	uint32_t num_channels;
	struct WIFI_CHANNEL_STAT channels[];
};

/* per rate statistics */
struct WIFI_RATE_STAT {
	struct WIFI_RATE rate;
	uint32_t tx_mpdu;
	uint32_t rx_mpdu;
	uint32_t mpdu_lost;
	uint32_t retries;
	uint32_t retries_short;
	uint32_t retries_long;
};

/*wifi_interface_link_layer_info*/
enum WIFI_CONNECTION_STATE {
	WIFI_DISCONNECTED = 0,
	WIFI_AUTHENTICATING = 1,
	WIFI_ASSOCIATING = 2,
	WIFI_ASSOCIATED = 3,
	WIFI_EAPOL_STARTED = 4,
	WIFI_EAPOL_COMPLETED = 5,
};

enum WIFI_ROAM_STATE {
	WIFI_ROAMING_IDLE = 0,
	WIFI_ROAMING_ACTIVE = 1,
};

enum WIFI_INTERFACE_MODE {
	WIFI_INTERFACE_STA = 0,
	WIFI_INTERFACE_SOFTAP = 1,
	WIFI_INTERFACE_IBSS = 2,
	WIFI_INTERFACE_P2P_CLIENT = 3,
	WIFI_INTERFACE_P2P_GO = 4,
	WIFI_INTERFACE_NAN = 5,
	WIFI_INTERFACE_MESH = 6,
	WIFI_INTERFACE_UNKNOWN = -1
};

struct WIFI_INTERFACE_LINK_LAYER_INFO {
	enum WIFI_INTERFACE_MODE mode;
	u8 mac_addr[6];
	enum WIFI_CONNECTION_STATE state;
	enum WIFI_ROAM_STATE roaming;
	u32 capabilities;
	u8 ssid[33];
	u8 bssid[6];
	u8 ap_country_str[3];
	u8 country_str[3];
};

/* wifi peer type */
enum WIFI_PEER_TYPE {
	WIFI_PEER_STA,
	WIFI_PEER_AP,
	WIFI_PEER_P2P_GO,
	WIFI_PEER_P2P_CLIENT,
	WIFI_PEER_NAN,
	WIFI_PEER_TDLS,
	WIFI_PEER_INVALID,
};

/* per peer statistics */
struct WIFI_PEER_INFO {
	enum WIFI_PEER_TYPE type;
	uint8_t peer_mac_address[6];
	uint32_t capabilities;
	uint32_t num_rate;
	struct WIFI_RATE_STAT rate_stats[];
};

/* interface statistics */
struct WIFI_IFACE_STAT {
	struct WIFI_INTERFACE_LINK_LAYER_INFO info;
	uint32_t beacon_rx;
	uint32_t mgmt_rx;
	uint32_t mgmt_action_rx;
	uint32_t mgmt_action_tx;
	int32_t rssi_mgmt;
	int32_t rssi_data;
	int32_t rssi_ack;
	struct WIFI_WMM_AC_STAT_ ac[WIFI_AC_MAX];
	uint32_t num_peers;
	struct WIFI_PEER_INFO peer_info[];
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
 * access categories
 */
enum ENUM_STATS_LLS_AC {
	STATS_LLS_WIFI_AC_VO  = 0,
	STATS_LLS_WIFI_AC_VI  = 1,
	STATS_LLS_WIFI_AC_BE  = 2,
	STATS_LLS_WIFI_AC_BK  = 3,
	STATS_LLS_WIFI_AC_MAX = 4,
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

/**
 * interface statistics
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
 */
struct STATS_LLS_WIFI_IFACE_STAT {
	void *iface;
	struct WIFI_INTERFACE_LINK_LAYER_INFO info;
	uint32_t beacon_rx;
	uint64_t average_tsf_offset;
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

enum PARAM_MLO_MODE {
	PARAM_MLO_MODE_DEFAULT = 0,
	PARAM_MLO_MODE_LOW_LATENCY,
	PARAM_MLO_MODE_HIGH_THROUGHPUT,
	PARAM_MLO_MODE_LOW_POWER,
	PARAM_MLO_MODE_MAX,
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */



/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/*
 * TODO: function is os-related, while may depend on the purpose of function
 * to implement on other os
 */
#if 0
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

int mtk_cfg80211_vendor_llstats_get_info(struct wiphy
		*wiphy, struct wireless_dev *wdev,
		const void *data, int data_len);

int mtk_cfg80211_vendor_set_band(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

int mtk_cfg80211_vendor_set_scan_mac_oui(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

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

int mtk_cfg80211_vendor_get_preferred_freq_list(struct wiphy
		*wiphy, struct wireless_dev *wdev, const void *data,
		int data_len);
#endif
#endif /* _GL_VENDOR_H */
