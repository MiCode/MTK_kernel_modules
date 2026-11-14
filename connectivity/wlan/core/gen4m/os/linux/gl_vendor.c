// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 ** gl_vendor.c
 **
 **
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "precomp.h"
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include "gl_cfg80211.h"
#include "gl_vendor.h"
#include "wlan_oid.h"
#include "rlm_domain.h"
#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif


#if CFG_SUPPORT_MBRAIN
#include "gl_mbrain.h"
#endif
#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
const struct nla_policy mtk_scan_param_policy[
		WIFI_ATTR_SCAN_MAX + 1] = {
	[WIFI_ATTR_SCAN_IFACE_TYPE] = {.type = NLA_U8},
	[WIFI_ATTR_SCAN_ASSOC_TYPE] = {.type = NLA_U8},
	[WIFI_ATTR_SCAN_TYPE] = {.type = NLA_U8},
	[WIFI_ATTR_SCAN_PROBE_NUM] = {.type = NLA_U8},
	[WIFI_ATTR_SCAN_ACTIVE_TIME] = {.type = NLA_U32},
	[WIFI_ATTR_SCAN_PASSIVE_TIME] = {.type = NLA_U32},
	[WIFI_ATTR_SCAN_HOME_TIME] = {.type = NLA_U32},
	[WIFI_ATTR_SCAN_ACTIVE_N_CH_BACK] = {.type = NLA_U8},
	[WIFI_ATTR_SCAN_PASSIVE_N_CH_BACK] = {.type = NLA_U8},
};

const struct nla_policy qca_wlan_vendor_attr_policy[
	QCA_WLAN_VENDOR_ATTR_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_ROAMING_POLICY] = {.type = NLA_U32},
	[QCA_WLAN_VENDOR_ATTR_MAC_ADDR] = {.type = NLA_BINARY,
					   .len = MAC_ADDR_LEN},
	[QCA_WLAN_VENDOR_ATTR_SETBAND_VALUE] = {.type = NLA_U32},
	[QCA_WLAN_VENDOR_ATTR_SETBAND_MASK] = {.type = NLA_U32},
};

const struct nla_policy nla_parse_wifi_attribute[
	WIFI_ATTRIBUTE_MAX + 1] = {
	[WIFI_ATTRIBUTE_BAND] = {.type = NLA_U32},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI] = {.type = NLA_MIN_LEN, .len = 0 },
#else
	[WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI] = {.type = NLA_BINARY},
#endif
	[WIFI_ATTRIBUTE_COUNTRY_CODE] = {.type = NLA_STRING},
	[WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_NUM] = {.type = NLA_U32},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_BSSID] =
		NLA_POLICY_EXACT_LEN_WARN(MAC_ADDR_LEN),
#else
	[WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_BSSID] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN},
#endif
	[WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_NUM] = {.type = NLA_U32},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_SSID] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_SSID] = {
		.type = NLA_MIN_LEN, .len = 0 },
#else
	[WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_SSID] = {.type = NLA_BINARY},
#endif
	[WIFI_ATTRIBUTE_ROAMING_STATE] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_TX_POWER_SCENARIO] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_LATENCY_MODE] = {.type = NLA_U32},
};

const struct nla_policy nla_parse_wifi_multista[
		MULTISTA_ATTRIBUTE_MAX + 1] = {
	[MULTISTA_ATTRIBUTE_PRIMARY_IFACE] = {.type = NLA_U32},
	[MULTISTA_ATTRIBUTE_USE_CASE] = {.type = NLA_U32},
};

const struct nla_policy nla_parse_wifi_rssi_monitor[
		WIFI_ATTRIBUTE_RSSI_MONITOR_ATTRIBUTE_MAX + 1] = {
	[WIFI_ATTRIBUTE_RSSI_MONITOR_MAX_RSSI] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_RSSI_MONITOR_MIN_RSSI] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_RSSI_MONITOR_START]    = {.type = NLA_U32},
};

const struct nla_policy nla_get_version_policy[
		LOGGER_ATTRIBUTE_MAX + 1] = {
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[LOGGER_ATTRIBUTE_DRIVER_VER] = NLA_POLICY_MIN_LEN(0),
	[LOGGER_ATTRIBUTE_FW_VER] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[LOGGER_ATTRIBUTE_DRIVER_VER] = { .type = NLA_MIN_LEN, .len = 0 },
	[LOGGER_ATTRIBUTE_FW_VER] = { .type = NLA_MIN_LEN, .len = 0 },
#else
	[LOGGER_ATTRIBUTE_DRIVER_VER] = { .type = NLA_UNSPEC },
	[LOGGER_ATTRIBUTE_FW_VER] = { .type = NLA_UNSPEC },
#endif
#if CFG_SUPPORT_LOGGER
	[LOGGER_ATTRIBUTE_RING_ID] = { .type = NLA_U32 },
	[LOGGER_ATTRIBUTE_RING_NAME] = { .type = NLA_NUL_STRING },
	[LOGGER_ATTRIBUTE_RING_FLAGS] = { .type = NLA_U32 },
	[LOGGER_ATTRIBUTE_LOG_LEVEL] = { .type = NLA_U32 },
	[LOGGER_ATTRIBUTE_LOG_TIME_INTVAL] = { .type = NLA_U32 },
	[LOGGER_ATTRIBUTE_LOG_MIN_DATA_SIZE] = { .type = NLA_U32 },
	[LOGGER_ATTRIBUTE_RING_DATA] = { .type = NLA_BINARY },
	[LOGGER_ATTRIBUTE_RING_STATUS] = { .type = NLA_BINARY },
	[LOGGER_ATTRIBUTE_RING_NUM] = { .type = NLA_U32 },
#endif
};

const struct nla_policy nla_parse_offloading_policy[
		 MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC + 1] = {
	[MKEEP_ALIVE_ATTRIBUTE_ID] = {.type = NLA_U8},
	[MKEEP_ALIVE_ATTRIBUTE_IP_PKT] = {.type = NLA_BINARY},
	[MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN] = {.type = NLA_U16},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR] =
		NLA_POLICY_EXACT_LEN_WARN(MAC_ADDR_LEN),
	[MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR] =
		NLA_POLICY_EXACT_LEN_WARN(MAC_ADDR_LEN),
#else
	[MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN},
	[MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN},
#endif
	[MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC] = {.type = NLA_U32},
};

const struct nla_policy nla_get_preferred_freq_list_policy[
		WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_MAX + 1] = {
	[WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_IFACE_TYPE] = {.type = NLA_U32},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_GET] = NLA_POLICY_MIN_LEN(0),
#endif
};

const struct nla_policy
	nla_p2p_listen_offload_policy
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_CHANNEL] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_PERIOD] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_INTERVAL] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_COUNT] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_DEVICE_TYPES] = {
		.type = NLA_BINARY, .len = 80},
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_VENDOR_IE] = {
		.type = NLA_BINARY, .len = 512},
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_CTRL_FLAG] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_P2P_LO_STOP_REASON] = {
		.type = NLA_U8 },
};


const struct nla_policy nla_get_acs_policy[
		WIFI_VENDOR_ATTR_ACS_MAX + 1] = {
	[WIFI_VENDOR_ATTR_ACS_HW_MODE] = { .type = NLA_U8 },
	[WIFI_VENDOR_ATTR_ACS_HT_ENABLED] = { .type = NLA_FLAG },
	[WIFI_VENDOR_ATTR_ACS_HT40_ENABLED] = { .type = NLA_FLAG },
	[WIFI_VENDOR_ATTR_ACS_VHT_ENABLED] = { .type = NLA_FLAG },
	[WIFI_VENDOR_ATTR_ACS_CHWIDTH] = { .type = NLA_U16 },
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[WIFI_VENDOR_ATTR_ACS_CH_LIST] = NLA_POLICY_MIN_LEN(0),
	[WIFI_VENDOR_ATTR_ACS_FREQ_LIST] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[WIFI_VENDOR_ATTR_ACS_CH_LIST] = { .type = NLA_MIN_LEN, .len = 0 },
	[WIFI_VENDOR_ATTR_ACS_FREQ_LIST] = { .type = NLA_MIN_LEN, .len = 0 },
#else
	[WIFI_VENDOR_ATTR_ACS_CH_LIST] = { .type = NLA_UNSPEC },
	[WIFI_VENDOR_ATTR_ACS_FREQ_LIST] = { .type = NLA_UNSPEC },
#endif
	[WIFI_VENDOR_ATTR_ACS_EHT_ENABLED] = { .type = NLA_FLAG },
	[WIFI_VENDOR_ATTR_ACS_LINK_ID] = { .type = NLA_U8 },
};

const struct nla_policy nla_string_cmd_policy[
		STRING_ATTRIBUTE_MAX + 1] = {
	[STRING_ATTRIBUTE_DATA] = { .type = NLA_STRING },
};

const struct nla_policy nla_get_apf_policy[
		APF_ATTRIBUTE_MAX + 1] = {
	[APF_ATTRIBUTE_VERSION] = {.type = NLA_U32},
	[APF_ATTRIBUTE_MAX_LEN] = {.type = NLA_U32},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[APF_ATTRIBUTE_PROGRAM] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[APF_ATTRIBUTE_PROGRAM] = {.type = NLA_MIN_LEN, .len = 0},
#else
	[APF_ATTRIBUTE_PROGRAM] = {.type = NLA_UNSPEC},
#endif
	[APF_ATTRIBUTE_PROGRAM_LEN] = {.type = NLA_U32},
};

const struct nla_policy nla_set_rtt_config_policy[
		RTT_ATTRIBUTE_TARGET_BW + 1] = {
	[RTT_ATTRIBUTE_TARGET_CNT] = {.type = NLA_U8},
	[RTT_ATTRIBUTE_TARGET_INFO] = {.type = NLA_NESTED},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[RTT_ATTRIBUTE_TARGET_MAC] =
		NLA_POLICY_EXACT_LEN_WARN(MAC_ADDR_LEN),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[RTT_ATTRIBUTE_TARGET_MAC] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN},
#endif
	[RTT_ATTRIBUTE_TARGET_TYPE] = {.type = NLA_U8},
	[RTT_ATTRIBUTE_TARGET_PEER] = {.type = NLA_U8},
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[RTT_ATTRIBUTE_TARGET_CHAN] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[RTT_ATTRIBUTE_TARGET_CHAN] = { .type = NLA_MIN_LEN, .len = 0 },
#else
	[RTT_ATTRIBUTE_TARGET_CHAN] = {.type = NLA_UNSPEC},
#endif
	[RTT_ATTRIBUTE_TARGET_PERIOD] = {.type = NLA_U32},
	[RTT_ATTRIBUTE_TARGET_NUM_BURST] = {.type = NLA_U32},
	[RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST] = {.type = NLA_U32},
	[RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTM] = {.type = NLA_U32},
	[RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTMR] = {.type = NLA_U32},
	[RTT_ATTRIBUTE_TARGET_LCI] = {.type = NLA_U8},
	[RTT_ATTRIBUTE_TARGET_LCR] = {.type = NLA_U8},
	[RTT_ATTRIBUTE_TARGET_BURST_DURATION] = {.type = NLA_U32},
	[RTT_ATTRIBUTE_TARGET_PREAMBLE] = {.type = NLA_U8},
	[RTT_ATTRIBUTE_TARGET_BW] = {.type = NLA_U8},
};

/* PASN */
const struct nla_policy nla_pasn_policy[
		QCA_WLAN_VENDOR_ATTR_PASN_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_PASN_INVALID] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_PASN_ACTION] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_PASN_PEERS] = {
		.type = NLA_NESTED },
};

const struct nla_policy nla_pasn_peer_policy[
		QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_PASN_PEER_INVALID] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_PASN_PEER_SRC_ADDR] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN },
	[QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAC_ADDR] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN },
	[QCA_WLAN_VENDOR_ATTR_PASN_PEER_LTF_KEYSEED_REQUIRED] = {
		.type = NLA_FLAG },
	[QCA_WLAN_VENDOR_ATTR_PASN_PEER_STATUS_SUCCESS] = {
		.type = NLA_FLAG },
};

const struct nla_policy nla_secure_ranging_ctx_policy[
		QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_INVALID] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_ACTION] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SRC_ADDR] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN },
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_PEER_MAC_ADDR] = {
		.type = NLA_BINARY, .len = MAC_ADDR_LEN },
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SHA_TYPE] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_TK] = {
		.type = NLA_BINARY, .len = 512},
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_CIPHER] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_LTF_KEYSEED] = {
		.type = NLA_BINARY, .len = 512 },
};

#if CFG_SUPPORT_CSI
const struct nla_policy nla_get_csi_policy[
		WIFI_ATTRIBUTE_CSI_MAX + 1] = {
	[WIFI_ATTRIBUTE_CSI_CONTROL_MODE] = {.type = NLA_U8},
	[WIFI_ATTRIBUTE_CSI_CONFIG_ITEM] = {.type = NLA_U8},
	[WIFI_ATTRIBUTE_CSI_VALUE_1] = {.type = NLA_U8},
	[WIFI_ATTRIBUTE_CSI_VALUE_2] = {.type = NLA_U32},
};
#endif
const struct nla_policy nla_trx_stats_policy[
	WIFI_ATTRIBUTE_STATS_MAX + 1] = {
	[WIFI_ATTRIBUTE_STATS_TX_NUM] = {.type = NLA_U8},
	[WIFI_ATTRIBUTE_STATS_RX_NUM] = {.type = NLA_U8},
	[WIFI_ATTRIBUTE_STATS_CGS_NUM] = {.type = NLA_U8},

#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_STATS_TX_TAG_LIST] = NLA_POLICY_MIN_LEN(0),
	[WIFI_ATTRIBUTE_STATS_RX_TAG_LIST] = NLA_POLICY_MIN_LEN(0),
	[WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[WIFI_ATTRIBUTE_STATS_TX_TAG_LIST] = {.type = NLA_MIN_LEN, .len = 0 },
	[WIFI_ATTRIBUTE_STATS_RX_TAG_LIST] = {.type = NLA_MIN_LEN, .len = 0 },
	[WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST] = {.type = NLA_MIN_LEN, .len = 0 },
#else
	[WIFI_ATTRIBUTE_STATS_TX_TAG_LIST] = {.type = NLA_BINARY},
	[WIFI_ATTRIBUTE_STATS_RX_TAG_LIST] = {.type = NLA_BINARY},
	[WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST] = {.type = NLA_BINARY},
#endif
	[WIFI_ATTRIBUTE_STATS_VERSION] = {.type = NLA_U8},
};

const struct nla_policy mtk_tx_lat_montr_param_policy[
		WIFI_ATTR_TX_LAT_MONTR_MAX + 1] = {
	[WIFI_ATTR_TX_LAT_MONTR_EN] = {.type = NLA_U8},
	[WIFI_ATTR_TX_LAT_MONTR_INTVL] = {.type = NLA_U32},
	[WIFI_ATTR_TX_LAT_MONTR_DRIVER_CRIT] = {.type = NLA_U32},
	[WIFI_ATTR_TX_LAT_MONTR_MAC_CRIT] = {.type = NLA_U32},
	[WIFI_ATTR_TX_LAT_MONTR_IS_AVG] = {.type = NLA_U8},
};

const struct nla_policy mtk_wfd_tx_br_montr_policy[
		WIFI_ATTR_WFD_TX_BR_MONTR_MAX + 1] = {
	[WIFI_ATTR_WFD_TX_BR_MONTR_EN] = {.type = NLA_U8},
};

const struct nla_policy mtk_usable_channel_policy[
	WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX + 1] = {
	[WIFI_ATTRIBUTE_USABLE_CHANNEL_BAND] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_USABLE_CHANNEL_IFACE] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_USABLE_CHANNEL_FILTER] = {.type = NLA_U32},
	[WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX_SIZE] = {.type = NLA_U32},
};

const struct nla_policy mtk_enable_sta_channel_for_peer_network_policy[
	WIFI_ATTRIBUTE_ENABLE_STA_CHANNEL_FOR_P2P_MAX + 1] = {
	[WIFI_ATTRIBUTE_ENABLE_STA_CHANNEL_FOR_P2P_ENABLE_FLAG] = {
		.type = NLA_U32
	},
};

#if CFG_SUPPORT_WIFI_ADJUST_DTIM
const struct nla_policy mtk_set_dtim_param_policy[
		WIFI_ATTR_SET_DTIM_MAX + 1] = {
	[WIFI_ATTR_SET_DTIM_PARAMS] = {.type = NLA_U32},
};
#endif

const struct nla_policy nla_connect_ext_policy[
		QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_INVALID] = {
		.type = NLA_U32 },
	[QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_FEATURES] = {
	    .type = NLA_BINARY, .len = (NUM_QCA_CONNECT_EXT_FEATURES + 7) / 8 },
};

const struct nla_policy nla_parse_wifi_mlo_mode_policy[
		MTK_MLO_MODE_ATTRIBUTE_MAX + 1] = {
	[MTK_MLO_MODE_ATTRIBUTE_INVALID] = {.type = NLA_U32},
	[MTK_MLO_MODE_ATTRIBUTE_MLO_MODE] = {.type = NLA_U32},
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define GO_SAP_IFTYPE ((IFACE_MODE_P2P_GO) | IFACE_MODE_SOFTAP)
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
int mtk_cfg80211_NLA_PUT(struct sk_buff *skb, int attrtype,
			 int attrlen, const void *data)
{
	if (unlikely(nla_put(skb, attrtype, attrlen, data) < 0))
		return 0;
	return 1;
}

int mtk_cfg80211_nla_put_type(struct sk_buff *skb,
			      enum ENUM_NLA_PUT_DATE_TYPE type, int attrtype,
			      const void *value)
{
	u8 u8data = 0;
	u16 u16data = 0;
	u32 u32data = 0;
	u64 u64data = 0;

	switch (type) {
	case NLA_PUT_DATE_U8:
		u8data = *(u8 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u8),
					    &u8data);
	case NLA_PUT_DATE_U16:
		u16data = *(u16 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u16),
					    &u16data);
	case NLA_PUT_DATE_U32:
		u32data = *(u32 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u32),
					    &u32data);
	case NLA_PUT_DATE_U64:
		u64data = *(u64 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u64),
					    &u64data);
	default:
		break;
	}

	return 0;
}

int mtk_cfg80211_vendor_get_channel_list(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *attr;
	uint32_t band = 0;
	uint8_t ucNumOfChannel, i, j;
	struct RF_CHANNEL_INFO *aucChannelList;
	uint32_t num_channels;
	uint32_t channels[MAX_CHN_NUM];
	struct sk_buff *skb;
	uint16_t u2CountryCode;

	ASSERT(wiphy && wdev);
	if ((data == NULL) || !data_len)
		return -EINVAL;

	DBGLOG(REQ, TRACE, "data_len=%d, iftype=%d\n", data_len, wdev->iftype);

	attr = (struct nlattr *)data;
	if (attr->nla_type == WIFI_ATTRIBUTE_BAND)
		band = nla_get_u32(attr);

	DBGLOG(REQ, TRACE, "Get channel list for band: %d\n", band);

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	aucChannelList = (struct RF_CHANNEL_INFO *)
		kalMemAlloc(sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM,
			VIR_MEM_TYPE);
	if (!aucChannelList) {
		DBGLOG(REQ, ERROR,
			"Can not alloc memory for rf channel info\n");
		return -ENOMEM;
	}
	kalMemZero(aucChannelList,
		sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM);

	switch (band) {
	case 1: /* 2.4G band */
		rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_2G4, TRUE,
			MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);
		break;
	case 2: /* 5G band without DFS channels */
		rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_5G, TRUE,
			MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case 3: /* 6G band */
		rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_6G, TRUE,
			MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);
	break;
#endif
	case 4: /* 5G band DFS channels only */
		rlmDomainGetDfsChnls(prGlueInfo->prAdapter, MAX_CHN_NUM,
			&ucNumOfChannel, aucChannelList);
		break;
	default:
		ucNumOfChannel = 0;
		break;
	}

	kalMemZero(channels, sizeof(channels));
	u2CountryCode = prGlueInfo->prAdapter->rWifiVar.u2CountryCode;
	for (i = 0, j = 0; i < ucNumOfChannel; i++) {
		/* We need to report frequency list to HAL */
		channels[j] =
		    nicChannelNum2Freq(
				aucChannelList[i].ucChannelNum,
				aucChannelList[i].eBand) / 1000;
		if (channels[j] == 0)
			continue;
		else if ((u2CountryCode == COUNTRY_CODE_TW) &&
			 (channels[j] >= 5180 && channels[j] <= 5260)) {
			/* Taiwan NCC has resolution to follow FCC spec
			 * to support 5G Band 1/2/3/4
			 * (CH36~CH48, CH52~CH64, CH100~CH140, CH149~CH165)
			 * Filter CH36~CH52 for compatible with some old
			 * devices.
			 */
			DBGLOG(REQ, TRACE, "skip channels[%d]=%d, country=%d\n",
			       j, channels[j], u2CountryCode);
			continue;
		} else {
			DBGLOG(REQ, TRACE, "channels[%d] = %d\n", j,
			       channels[j]);
			j++;
		}
	}
	num_channels = j;
	DBGLOG(REQ, DEBUG, "Get channel list for band: %d, num_channels=%d\n",
	       band, num_channels);

	kalMemFree(aucChannelList, VIR_MEM_TYPE,
		sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(channels));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, WIFI_ATTRIBUTE_NUM_CHANNELS,
				 num_channels) < 0))
		goto nla_put_failure;

	if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_CHANNEL_LIST,
			     (sizeof(uint32_t) * num_channels), channels) < 0))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;
}

#if CFG_SUPPORT_LLW_SCAN
int mtk_cfg80211_vendor_set_latency_mode(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *prAttr;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SetInfoLen = 0, u4Mode = 0;
	struct AIS_FSM_INFO *ais;
	uint8_t ucBssIndex = 0;

	if (wiphy == NULL) {
		DBGLOG(REQ, ERROR, "wiphy is NULL\n");
		return -EFAULT;
	}

	if (wdev == NULL) {
		DBGLOG(REQ, ERROR, "wdev is NULL\n");
		return -EFAULT;
	}

	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAttr = (struct nlattr *)data;
	if (prAttr->nla_type == WIFI_ATTRIBUTE_LATENCY_MODE)
		u4Mode = nla_get_u32(prAttr);
	else {
		DBGLOG(REQ, INFO, "Unknown nla type:%d\n", prAttr->nla_type);
		return -EINVAL;
	}

	ais = aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex);

	DBGLOG(REQ, INFO,
		"Input Mode = %d, Current = %d\n",
		u4Mode, ais->ucLatencyMode);

	/* Do further scan handling for mode 2 and mode 3,
	 * reset if u4Mode == 0
	 */
	if (u4Mode >= 2 || u4Mode == 0) {
		if (u4Mode == 0)
			wlanChipConfigWithType(prGlueInfo->prAdapter,
				"SET_LATENCY_CRT_DATA 0",
				22,
				CHIP_CONFIG_TYPE_WO_RESPONSE);

		rStatus = kalIoctl(prGlueInfo,
			wlanoidSetLatency,
			&u4Mode, sizeof(uint32_t),
			&u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR,
				"SET_LATENCY fail 0x%x\n", rStatus);
		else
			DBGLOG(INIT, TRACE,
				"SET_LATENCY success\n");
	} else {
		if (ais->ucLatencyMode)
			goto exit;

		/* for mode 1 */
		wlanChipConfigWithType(prGlueInfo->prAdapter,
			"SET_LATENCY_CRT_DATA 1",
			22,
			CHIP_CONFIG_TYPE_WO_RESPONSE);


#if (CFG_EXT_FEATURE == 0)
		rStatus = kalIoctl(prGlueInfo,
			wlanoidSetLatency,
			&u4Mode, sizeof(uint32_t),
			&u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR,
				"SET_LATENCY fail 0x%x\n", rStatus);
		else
			DBGLOG(INIT, TRACE,
				"SET_LATENCY success\n");
#endif
	}

exit:

	return rStatus;
}
#endif

int mtk_cfg80211_vendor_set_country_code(struct wiphy
		*wiphy, struct wireless_dev *wdev, const void *data,
		int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	struct nlattr *attr;
	uint8_t country[2] = {0};

	ASSERT(wiphy && wdev);
	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	DBGLOG(REQ, TRACE,
	       "vendor command: data_len=%d, iftype=%d\n", data_len,
	       wdev->iftype);

	attr = (struct nlattr *)data;
	if (attr->nla_type == WIFI_ATTRIBUTE_COUNTRY_CODE &&
			nla_len(attr) >= 2) {
		country[0] = *((uint8_t *)nla_data(attr));
		country[1] = *((uint8_t *)nla_data(attr) + 1);
	}

	DBGLOG(REQ, DEBUG, "Set country code: %c%c\n", country[0],
	       country[1]);

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
	prAdapter = prGlueInfo->prAdapter;

	if (regd_is_single_sku_en(prAdapter)) {
		struct COUNTRY_CODE_SETTING prCountrySetting = {0};

#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		wiphy_unlock(wiphy);
#endif
		prCountrySetting.aucCountryCode[0] = country[0];
		prCountrySetting.aucCountryCode[1] = country[1];
		prCountrySetting.ucCountryLength = 2;
		prCountrySetting.fgNeedHoldRtnlLock = 1;
		rStatus = kalIoctl(prGlueInfo,
					wlanoidSetCountryCode,
					&prCountrySetting,
					sizeof(struct COUNTRY_CODE_SETTING),
					&u4BufLen);

#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		wiphy_lock(wiphy);
#endif
	} else {
		rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
			country, 2, &u4BufLen);
	}

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "Set country code error: %x\n", rStatus);
		return -EFAULT;
	}

	return 0;
}

int mtk_cfg80211_vendor_set_scan_mac_oui(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct nlattr *attr;
	uint32_t i = 0;
	struct PARAM_BSS_MAC_OUI rParamMacOui;
	uint32_t u4BufLen = 0;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;

	ASSERT(wiphy);
	ASSERT(wdev);

	if (data == NULL || data_len <= 0) {
		log_dbg(REQ, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo) {
		log_dbg(REQ, ERROR, "Invalid glue info\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
	prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) netdev_priv(wdev->netdev);
	if (!prNetDevPrivate) {
		log_dbg(REQ, ERROR, "Invalid net device private\n");
		return -EFAULT;
	}
	rParamMacOui.ucBssIndex = prNetDevPrivate->ucBssIdx;

	attr = (struct nlattr *)data;
	kalMemZero(rParamMacOui.ucMacOui, MAC_OUI_LEN);
	if (nla_type(attr) != WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI) {
		log_dbg(REQ, ERROR, "Set MAC oui type error(%u)\n",
			nla_type(attr));
		return -EINVAL;
	}

	if (nla_len(attr) != MAC_OUI_LEN) {
		log_dbg(REQ, ERROR, "Set MAC oui length error(%u), %u needed\n",
			nla_len(attr), MAC_OUI_LEN);
		return -EINVAL;
	}

	for (i = 0; i < MAC_OUI_LEN; i++)
		rParamMacOui.ucMacOui[i] = *((uint8_t *)nla_data(attr) + i);

	log_dbg(REQ, DEBUG, "Set MAC oui: %02x-%02x-%02x\n",
		rParamMacOui.ucMacOui[0], rParamMacOui.ucMacOui[1],
		rParamMacOui.ucMacOui[2]);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetScanMacOui,
		&rParamMacOui, sizeof(rParamMacOui), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		log_dbg(REQ, ERROR, "Set MAC oui error: 0x%X\n", rStatus);
		return -EFAULT;
	}

	return 0;
}

int mtk_cfg80211_vendor_string_cmd(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct nlattr *attr;
	char cmd[1024] = {0};

	if ((data == NULL) || !data_len)
		return -EINVAL;

	attr = (struct nlattr *)data;
#if KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE
	nla_strscpy(cmd, attr, sizeof(cmd));
#else
	nla_strlcpy(cmd, attr, sizeof(cmd));
#endif
	return mtk_cfg80211_process_str_cmd(wiphy, wdev, cmd,
		(data_len > strlen(cmd)) ? strlen(cmd) : data_len);
}

/* Size (in bytes) of the various private data types */
static const char iw_priv_type_size[] = {
	0,				/* IW_PRIV_TYPE_NONE */
	1,				/* IW_PRIV_TYPE_BYTE */
	1,				/* IW_PRIV_TYPE_CHAR */
	0,				/* Not defined */
	sizeof(uint32_t),		/* IW_PRIV_TYPE_INT */
	sizeof(struct iw_freq),		/* IW_PRIV_TYPE_FLOAT */
	sizeof(struct sockaddr),	/* IW_PRIV_TYPE_ADDR */
	0,				/* Not defined */
};

static int32_t get_priv_size(uint16_t args)
{
	int32_t	num = args & IW_PRIV_SIZE_MASK;
	int32_t	type = (args & IW_PRIV_TYPE_MASK) >> 12;

	return num * iw_priv_type_size[type];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Port from iwpriv source code.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_iw_cmd(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	char cmd[1024] = {0};
	char *out = cmd;
	uint16_t cmd_size = sizeof(cmd);
	char *cmdname = NULL;
	uint32_t index;
	int32_t subcmd = 0;
	char *buffer = NULL;
	uint16_t buffer_size = 4096 * sizeof(char);
	int32_t i4Argc = 0, argsCount;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	char **args;
	const struct iw_handler_def *iw_handlers;
	const struct iw_priv_args *priv;
	uint32_t priv_num;
	struct iw_request_info info;
	struct iwreq wrq;
	int32_t i = 0, j, k;
	int32_t offset = 0;
	int32_t temp = 0;
	int32_t err = -1;
	int32_t i4BytesWritten = 0;

	if (data == NULL || !data_len || !wdev || !wdev->netdev) {
		i4BytesWritten = -EINVAL;
		goto error;
	}

	iw_handlers = wdev->netdev->wireless_handlers;
	if (!iw_handlers) {
		i4BytesWritten = -EFAULT;
		goto error;
	}

	DBGLOG(REQ, INFO, "cmd: %s", data);
	buffer = (char *) kalMemAlloc(buffer_size, VIR_MEM_TYPE);
	if (!buffer) {
		i4BytesWritten = -ENOMEM;
		goto error;
	}

	kalStrnCpy(cmd, data, sizeof(cmd) - 1);
	wlanCfgParseArgument(cmd, &i4Argc, apcArgv);
	if (i4Argc < 1) {
		i4BytesWritten = -EFAULT;
		goto error;
	}

	cmdname = apcArgv[0];
	args = (char **)&apcArgv[1];
	argsCount = i4Argc - 1;

	priv = iw_handlers->private_args;
	priv_num = iw_handlers->num_private_args;

	/* Search the correct ioctl */
	k = -1;
	while ((++k < priv_num) && kalStrCmp(priv[k].name, cmdname))
		;

	/* If not found... */
	if (k == priv_num) {
		DBGLOG(REQ, ERROR, "Invalid command : %s\n", cmdname);
		i4BytesWritten = -EINVAL;
		goto error;
	}

	/* Watch out for sub-ioctls ! */
	if (priv[k].cmd < SIOCDEVPRIVATE) {
		j = -1;

		/* Find the matching *real* ioctl */
		while (++j < priv_num &&
		       (priv[j].name[0] != '\0' ||
			priv[j].set_args != priv[k].set_args ||
			priv[j].get_args != priv[k].get_args))
			;

		/* If not found... */
		if (j == priv_num) {
			DBGLOG(REQ, ERROR,
			       "Invalid private ioctl definition for : %s\n",
			       cmdname);
			i4BytesWritten = -EINVAL;
			goto error;
		}

		/* Save sub-ioctl number */
		subcmd = priv[k].cmd;
		/* Reserve one int (simplify alignment issues) */
		offset = sizeof(uint32_t);
		/* Use real ioctl definition from now on */
		k = j;
	}

	/* If we have to set some data */
	if ((priv[k].set_args & IW_PRIV_TYPE_MASK) &&
	    (priv[k].set_args & IW_PRIV_SIZE_MASK)) {
		switch (priv[k].set_args & IW_PRIV_TYPE_MASK) {
		case IW_PRIV_TYPE_BYTE:
			/* Number of args to fetch */
			wrq.u.data.length = argsCount;
			if (wrq.u.data.length >
			    (priv[k].set_args & IW_PRIV_SIZE_MASK))
				wrq.u.data.length =
					priv[k].set_args & IW_PRIV_SIZE_MASK;

			/* Fetch args */
			for (; i < wrq.u.data.length; i++) {
				if (kstrtoint(args[i], 10, &temp)) {
					i4BytesWritten = -EINVAL;
					goto error;
				}
				buffer[i] = (char)temp;
			}
			break;
		case IW_PRIV_TYPE_INT:
			/* Number of args to fetch */
			wrq.u.data.length = argsCount;
			if (wrq.u.data.length >
			    (priv[k].set_args & IW_PRIV_SIZE_MASK))
				wrq.u.data.length =
					priv[k].set_args & IW_PRIV_SIZE_MASK;

			/* Fetch args */
			for (; i < wrq.u.data.length; i++) {
				if (kstrtoint(args[i], 10, &temp)) {
					i4BytesWritten = -EINVAL;
					goto error;
				}
				((int32_t *)buffer)[i] = (int32_t)temp;
			}
			break;
		case IW_PRIV_TYPE_CHAR:
			if (i < argsCount) {
				/* Size of the string to fetch */
				wrq.u.data.length = kalStrLen(args[i]) + 1;
				if (wrq.u.data.length >
				    (priv[k].set_args & IW_PRIV_SIZE_MASK))
					wrq.u.data.length =
					  priv[k].set_args & IW_PRIV_SIZE_MASK;

				/* Fetch string */
				kalMemCopy(buffer, args[i], wrq.u.data.length);
				buffer[sizeof(buffer) - 1] = '\0';
				i++;
			} else {
				wrq.u.data.length = 1;
				buffer[0] = '\0';
			}
			break;
		/* TODO: Not support in gen4m now */
		case IW_PRIV_TYPE_FLOAT:
		case IW_PRIV_TYPE_ADDR:
		default:
			DBGLOG(REQ, ERROR, "Not implemented...\n");
			i4BytesWritten = -EINVAL;
			goto error;
		}

		if ((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
		    (wrq.u.data.length !=
				(priv[k].set_args & IW_PRIV_SIZE_MASK))) {
			DBGLOG(REQ, WARN,
			       "The command %s needs exactly %d argument(s)...\n",
			       cmdname, priv[k].set_args & IW_PRIV_SIZE_MASK);
			i4BytesWritten = -EINVAL;
			goto error;
		}
	} else {
		wrq.u.data.length = 0L;
	}

	if (strscpy(wrq.ifr_name, wdev->netdev->name, IFNAMSIZ) < 0) {
		i4BytesWritten = -EINVAL;
		goto error;
	}

	/* Those two tests are important. They define how the driver
	 * will have to handle the data
	 */
	if ((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
	    (get_priv_size(priv[k].set_args) + offset <= IFNAMSIZ)) {
		/* First case : all SET args fit within wrq */
		if (offset)
			wrq.u.mode = subcmd;
		kalMemCopy(wrq.u.name + offset, buffer, IFNAMSIZ - offset);
	} else {
		if ((priv[k].set_args == 0) &&
		    (priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
		    (get_priv_size(priv[k].get_args) <= IFNAMSIZ)) {
			/* Second case : no SET args, GET args fit within wrq */
			if (offset)
				wrq.u.mode = subcmd;
		} else {
			/* Third case : args won't fit in wrq, or variable
			 * number of args
			 */
			wrq.u.data.pointer = (caddr_t)buffer;
			wrq.u.data.flags = subcmd;
		}
	}

	/* Perform the private ioctl */
	info.cmd = priv[k].cmd;
	index = priv[k].cmd - SIOCIWFIRSTPRIV;
	if (index < iw_handlers->num_private && iw_handlers->private[index])
		err = iw_handlers->private[index](wdev->netdev, &info, &wrq.u,
						  (char *)&wrq.u);
	if (err < 0) {
		DBGLOG(REQ, ERROR, "%s (%x) failed, err=%d\n",
		       cmdname, priv[k].cmd, err);
		i4BytesWritten = err;
		goto error;
	}
	DBGLOG(REQ, INFO, "cmd: %s success, err=%d\n", cmdname, err);

	/* If we have to get some data */
	if ((priv[k].get_args & IW_PRIV_TYPE_MASK) &&
	    (priv[k].get_args & IW_PRIV_SIZE_MASK)) {
		int32_t	n = 0;		/* number of args */

		DBGLOG(REQ, INFO, "get cmd, size_fixed=%u, priv_size=%d\n",
		       priv[k].get_args & IW_PRIV_SIZE_FIXED,
		       get_priv_size(priv[k].get_args));
		kalMemZero(out, cmd_size);
		LOGBUF(out, cmd_size, i4BytesWritten,
		       "%-8.16s  %s:", wdev->netdev->name, cmdname);

		/* Check where is the returned data */
		if ((priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
		    (get_priv_size(priv[k].get_args) <= IFNAMSIZ)) {
			kalMemCopy(buffer, wrq.u.name, IFNAMSIZ);
			n = priv[k].get_args & IW_PRIV_SIZE_MASK;
		} else {
			n = wrq.u.data.length;
		}
		DBGLOG_MEM8(REQ, INFO, buffer, n);

		switch (priv[k].get_args & IW_PRIV_TYPE_MASK) {
		case IW_PRIV_TYPE_BYTE:
			/* Display args */
			for (j = 0; j < n; j++)
				LOGBUF(out, cmd_size, i4BytesWritten,
				       "%d  ", buffer[j]);
			LOGBUF(out, cmd_size, i4BytesWritten, "\n");
			break;
		case IW_PRIV_TYPE_INT:
			/* Display args */
			for (j = 0; j < n; j++)
				LOGBUF(out, cmd_size, i4BytesWritten,
				       "%d  ", ((uint32_t *) buffer)[j]);
			LOGBUF(out, cmd_size, i4BytesWritten, "\n");
			break;
		case IW_PRIV_TYPE_CHAR:
			/* Display args */
			out[n] = '\0';
			LOGBUF(out, cmd_size, i4BytesWritten,
			       "%s\n", buffer);
			break;
		/* TODO: Not support in gen4m now */
		case IW_PRIV_TYPE_FLOAT:
		case IW_PRIV_TYPE_ADDR:
		default:
			DBGLOG(REQ, ERROR, "Not yet implemented...\n");
			i4BytesWritten = -EINVAL;
			goto error;
		}
	}	/* if args to set */

	if (i4BytesWritten > 0) {
		DBGLOG(REQ, LOUD, "done. out:\n");
		DBGLOG_MEM8(REQ, LOUD, out, i4BytesWritten);
		i4BytesWritten = mtk_cfg80211_process_str_cmd_reply(wiphy, out,
							  i4BytesWritten);
	}

error:
	if (buffer)
		kalMemFree(buffer, VIR_MEM_TYPE, sizeof(struct char));

	return i4BytesWritten;
}

int mtk_cfg80211_vendor_iw_cmd_driver(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct net_device *prNetDev = wdev->netdev;
	int8_t cmd[1024] = {0};
	int32_t i4BytesWritten = 0;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (data == NULL || !data_len || !prGlueInfo)
		return -EINVAL;

	kalStrnCpy(cmd, data, sizeof(cmd) - 1);
	i4BytesWritten =
		priv_driver_cmds(prGlueInfo, prNetDev, cmd, sizeof(cmd));

	if (i4BytesWritten > 0)
		return mtk_cfg80211_process_str_cmd_reply(wiphy,
							  cmd, i4BytesWritten);
	return i4BytesWritten;
}

#if CFG_SUPPORT_WIFI_ADJUST_DTIM
int mtk_cfg80211_vendor_set_dtim_param(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct ADAPTER *prAdapter;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	struct nlattr *attr[WIFI_ATTR_SET_DTIM_PARAMS + 1];
	char str[64] = {0};
	int8_t len;
	uint32_t u4SetDtimPeriod = 0, rStatus, u4BufLen;

	ASSERT(wiphy);
	ASSERT(wdev);
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter)
		return -EFAULT;

	if ((data == NULL) || (data_len == 0))
		goto fail;

	kalMemZero(attr, sizeof(struct nlattr *) *
			   (WIFI_ATTR_SET_DTIM_PARAMS + 1));

	if (NLA_PARSE_NESTED(attr,
			     WIFI_ATTR_SET_DTIM_PARAMS,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     mtk_set_dtim_param_policy) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		goto fail;
	}

	u4SetDtimPeriod = nla_get_u32(attr[WIFI_ATTR_SET_DTIM_PARAMS]);

	len = kalSnprintf(str, sizeof(str), "DtimPeriod 0 %d", u4SetDtimPeriod);

	if (len <= 0 || u4SetDtimPeriod > 10) {
		DBGLOG(REQ, ERROR,
			"set_dtim_param invalid parameters! %d\n",
			u4SetDtimPeriod);
		goto fail;
	}

	DBGLOG(REQ, DEBUG,
		"vendor_set_dtim_param: str=%s, len=%d\n", str, len);

	kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));
	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
	rChipConfigInfo.u2MsgSize = len;
	kalStrnCpy(rChipConfigInfo.aucCmd, str,
		   CHIP_CONFIG_RESP_SIZE - 1);
	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidSetChipConfig,
		&rChipConfigInfo, sizeof(rChipConfigInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "%s: kalIoctl ret=%d\n", __func__,
		       rStatus);
		return -1;
	}

	return WLAN_STATUS_SUCCESS;
fail:
	return -EINVAL;
}
#endif

int mtk_cfg80211_vendor_set_scan_param(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
#define STR_SIZE 64

	struct ADAPTER *prAdapter;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	struct nlattr *attr[WIFI_ATTR_SCAN_PASSIVE_N_CH_BACK + 1];
	char str[STR_SIZE] = {0};
	uint8_t i, len;
	uint8_t ucNetworkType = 0, ucAssocState = 0, ucScanType = 0;
	uint8_t ucProbeCount = 0, ucActiveScnChBack = 0, ucPassiveScnChBack = 0;
	uint32_t u4ActiveDwellTimeInMs = 0, u4PassiveDwellTimeInMs = 0;
	uint32_t u4OpChStayTimeInMs = 0, rStatus, u4BufLen;


	ASSERT(wiphy);
	ASSERT(wdev);
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter)
		return -EFAULT;

	if ((data == NULL) || (data_len == 0))
		goto fail;

	kalMemZero(attr, sizeof(struct nlattr *) *
			   (WIFI_ATTR_SCAN_PASSIVE_N_CH_BACK + 1));

	if (NLA_PARSE_NESTED(attr,
			     WIFI_ATTR_SCAN_PASSIVE_N_CH_BACK,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     mtk_scan_param_policy) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		goto fail;
	}

	for (i = WIFI_ATTR_SCAN_IFACE_TYPE; i < WIFI_ATTR_SCAN_MAX; i++) {
		if (attr[i]) {
			switch (i) {
			case WIFI_ATTR_SCAN_IFACE_TYPE:
				ucNetworkType =	nla_get_u8(attr[i]);
				break;
			case WIFI_ATTR_SCAN_ASSOC_TYPE:
				ucAssocState = nla_get_u8(attr[i]);
				break;
			case WIFI_ATTR_SCAN_TYPE:
				ucScanType = nla_get_u8(attr[i]);
				break;
			case WIFI_ATTR_SCAN_PROBE_NUM:
				ucProbeCount = nla_get_u8(attr[i]);
				break;
			case WIFI_ATTR_SCAN_ACTIVE_TIME:
				u4ActiveDwellTimeInMs = nla_get_u32(attr[i]);
				break;
			case WIFI_ATTR_SCAN_PASSIVE_TIME:
				u4PassiveDwellTimeInMs = nla_get_u32(attr[i]);
				break;
			case WIFI_ATTR_SCAN_HOME_TIME:
				u4OpChStayTimeInMs = nla_get_u32(attr[i]);
				break;
			case WIFI_ATTR_SCAN_ACTIVE_N_CH_BACK:
				ucActiveScnChBack = nla_get_u8(attr[i]);
				break;
			case WIFI_ATTR_SCAN_PASSIVE_N_CH_BACK:
				ucPassiveScnChBack = nla_get_u8(attr[i]);
				break;
			}
		}
	}

	len = kalSnprintf(str, sizeof(str),
			"scnSetParameter %d %d %d %d %d %d %d %d %d",
			ucNetworkType, ucAssocState, ucScanType,
			ucProbeCount, u4ActiveDwellTimeInMs,
			u4PassiveDwellTimeInMs, u4OpChStayTimeInMs,
			ucActiveScnChBack, ucPassiveScnChBack);

	if (len <= 0) {
		DBGLOG(REQ, ERROR, "set_scan_param invalid length!\n");
		goto fail;
	}

	DBGLOG(REQ, DEBUG,
		"vendor_set_scan_param: str=%s, len=%d\n", str, len);

	kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));
	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rChipConfigInfo.u2MsgSize = len;
	kalStrnCpy(rChipConfigInfo.aucCmd, str,
		(STR_SIZE < CHIP_CONFIG_RESP_SIZE) ?
		STR_SIZE : CHIP_CONFIG_RESP_SIZE);

	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

#undef STR_SIZE

	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidSetChipConfig,
		&rChipConfigInfo, sizeof(rChipConfigInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "%s: kalIoctl ret=%d\n", __func__,
		       rStatus);
		return -1;
	}

	return WLAN_STATUS_SUCCESS;
fail:
	return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to answer FWK that we can support FW Roaming.
 *
 * \param[in] wiphy wiphy for AIS STA.
 *
 * \param[in] wdev (not used here).
 *
 * \param[in] data (not used here).
 *
 * \param[in] data_len (not used here).
 *
 * \retval TRUE Success.
 *
 * \note we use cfg80211_vendor_cmd_reply to send the max number of our
 *       blocklist and whiltlist directly without receiving any data
 *       from the upper layer.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_get_roaming_capabilities(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	uint32_t maxNumOfList[2] = { MAX_FW_ROAMING_BLOCKLIST_SIZE,
				     MAX_FW_ROAMING_ALLOWLIST_SIZE };
	struct sk_buff *skb;

	ASSERT(wiphy);

	DBGLOG(REQ, DEBUG,
		"Get roaming capabilities: max block/allowlist=%d/%d\n",
		maxNumOfList[0], maxNumOfList[1]);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(maxNumOfList));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_NUM,
				sizeof(uint32_t), &maxNumOfList[0]) < 0))
		goto nla_put_failure;
	if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_ROAMING_ALLOWLIST_NUM,
				sizeof(uint32_t), &maxNumOfList[1]) < 0))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to receive the block/allowlist. from FWK.
 *
 * \param[in] wiphy wiphy for AIS STA.
 *
 * \param[in] wdev (not used here).
 *
 * \param[in] data BSSIDs in the FWK block&allowlist.
 *
 * \param[in] data_len the byte-length of the FWK block&allowlist.
 *
 * \retval TRUE Success.
 *
 * \note we iterate each BSSID in 'data' and put it into driver blocklist.
 *       For now, whiltelist doesn't be implemented by the FWK currently.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_config_roaming(struct wiphy *wiphy,
	       struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct nlattr *attrlist;
	struct AIS_BLOCKLIST_ITEM *prBlockList;
	struct BSS_DESC *prBssDesc = NULL;
	uint32_t len_shift = 0;
	uint32_t numOfList[2] = { 0 };
	uint8_t *aucBSSID = NULL;
	int i;

	DBGLOG(REQ, DEBUG,
	       "Receives roaming blocklist & allowlist with data_len=%d\n",
	       data_len);
	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EINVAL;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4FWRoamingEnable == 0) {
		DBGLOG(REQ, DEBUG,
		       "FWRoaming is disabled (FWRoamingEnable=%d)\n",
		       prGlueInfo->u4FWRoamingEnable);
		return WLAN_STATUS_SUCCESS;
	}

	attrlist = (struct nlattr *)((uint8_t *) data);

	/* get the number of blocklist and copy those mac addresses from HAL */
	if (attrlist->nla_type ==
	    WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_NUM) {
		numOfList[0] = nla_get_u32(attrlist);
		len_shift += NLA_ALIGN(attrlist->nla_len);
	}
	DBGLOG(REQ, DEBUG, "Get the number of blocklist=%d\n",
	       numOfList[0]);

	if (numOfList[0] > MAX_FW_ROAMING_BLOCKLIST_SIZE)
		return -EINVAL;

	/*Refresh all the FWKBlocklist */
	aisRefreshFWKBlocklist(prGlueInfo->prAdapter);

	/* Start to receive blocklist mac addresses and set to FWK blocklist */
	attrlist = (struct nlattr *)((uint8_t *) data + len_shift);
	for (i = 0; i < numOfList[0]; i++) {
		if (attrlist->nla_type ==
		    WIFI_ATTRIBUTE_ROAMING_BLOCKLIST_BSSID) {
			aucBSSID = nla_data(attrlist);
			prBssDesc =
				scanSearchBssDescByBssid(prGlueInfo->prAdapter,
							aucBSSID);
			len_shift += NLA_ALIGN(attrlist->nla_len);
			attrlist =
				(struct nlattr *)((uint8_t *) data + len_shift);

			if (prBssDesc == NULL) {
				DBGLOG(REQ, ERROR, "No found blocklist BSS="
					MACSTR "\n",
					MAC2STR(aucBSSID));
				continue;
			}

			prBlockList = aisAddBlocklist(prGlueInfo->prAdapter,
						      prBssDesc);

			if (prBlockList) {
				prBlockList->fgIsInFWKBlocklist = TRUE;
				DBGLOG(REQ, DEBUG,
					"Gets roaming blocklist SSID=%s addr="
					MACSTR "\n",
					HIDE(prBssDesc->aucSSID),
					MAC2STR(prBssDesc->aucBSSID));
			} else {
				DBGLOG(REQ, ERROR,
					"prBlockList is NULL, return -EINVAL!");
				return -EINVAL;
			}
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to turn on/off FW Roaming.
 *
 * \param[in] wiphy wiphy for AIS STA.
 *
 * \param[in] wdev (not used here).
 *
 * \param[in] data 1 for ON / 0 for OFF.
 *
 * \param[in] data_len the byte-length of the data.
 *
 * \retval TRUE Success.
 *
 * \note we only receive the data and make the interface available to FWK.
 *       For now, this SUBCMD woundn't be sent from the FWK currently.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_enable_roaming(struct wiphy *wiphy,
	       struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct nlattr *attr;

	ASSERT(wiphy);	/* change to if (wiphy == NULL) then return? */
	ASSERT(wdev);	/* change to if (wiphy == NULL) then return? */
	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	attr = (struct nlattr *)data;
	if (attr->nla_type == WIFI_ATTRIBUTE_ROAMING_STATE)
		prGlueInfo->u4FWRoamingEnable = nla_get_u32(attr);

	DBGLOG(REQ, DEBUG, "FWK set FWRoamingEnable = %d\n",
	       prGlueInfo->u4FWRoamingEnable);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_RTT
int mtk_cfg80211_vendor_get_rtt_capabilities(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus, u4BufLen;
	struct RTT_CAPABILITIES rRttCapabilities;
	struct sk_buff *skb;

	DBGLOG(REQ, TRACE, "vendor command\r\n");

	ASSERT(wiphy);
	ASSERT(wdev);
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
			sizeof(rRttCapabilities));
	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed:\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(&rRttCapabilities, sizeof(rRttCapabilities));

	rStatus = kalIoctl(prGlueInfo, wlanoidGetRttCapabilities,
			   &rRttCapabilities,
			   sizeof(struct RTT_CAPABILITIES),
			   &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "RTT get capabilities error:%x\n", rStatus);
		goto failure;
	}

		DBGLOG(RTT, DEBUG,
			"one_sided=%hhu, ftm=%hhu, lci=%hhu, lcr=%hhu, preamble=%hhu, bw=%hhu, responder=%hhu, ver=%hhu",
			rRttCapabilities.fgRttOneSidedSupported,
			rRttCapabilities.fgRttFtmSupported,
			rRttCapabilities.fgLciSupported,
			rRttCapabilities.fgLcrSupported,
			rRttCapabilities.ucPreambleSupport,
			rRttCapabilities.ucBwSupport,
			rRttCapabilities.fgResponderSupported,
			rRttCapabilities.fgMcVersion);


	if (unlikely(nla_put_nohdr(skb,
		sizeof(rRttCapabilities), &rRttCapabilities) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		goto failure;
	}

	return cfg80211_vendor_cmd_reply(skb);
failure:
	kfree_skb(skb);
	return -EFAULT;
}

int mtk_cfg80211_vendor_set_rtt_config(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct nlattr *attrs[RTT_ATTRIBUTE_TARGET_INFO + 1];
	struct nlattr *tb[RTT_ATTRIBUTE_TARGET_BW + 1];
	struct nlattr *attr;
	uint32_t rStatus, u4BufLen;
	int tmp, err;
	uint8_t i = 0;
	struct PARAM_RTT_REQUEST *request;
	int ret = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;

	DBGLOG(REQ, DEBUG, "vendor command\r\n");

	ASSERT(wiphy);
	ASSERT(wdev);

	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (NLA_PARSE(attrs, RTT_ATTRIBUTE_TARGET_INFO,
			data, data_len, nla_set_rtt_config_policy)) {
		DBGLOG(RTT, ERROR, "Invalid ATTR.\n");
		return -EINVAL;
	}

	request = kalMemAlloc(sizeof(struct PARAM_RTT_REQUEST), VIR_MEM_TYPE);
	if (!request) {
		DBGLOG(RTT, ERROR, "fail to alloc memory for request.\n");
		return -EFAULT;
	}
	kalMemZero(request, sizeof(struct PARAM_RTT_REQUEST));
	request->fgEnable = true;

	if (attrs[RTT_ATTRIBUTE_TARGET_CNT]) {
		request->ucConfigNum =
			nla_get_u8(attrs[RTT_ATTRIBUTE_TARGET_CNT]);
		DBGLOG(RTT, DEBUG, "TARGET_CNT = %u\n", request->ucConfigNum);
	} else {
		DBGLOG(RTT, ERROR, "No RTT_ATTRIBUTE_TARGET_CNT\n");
		ret = -EINVAL;
		goto RETURN;
	}

	if (!attrs[RTT_ATTRIBUTE_TARGET_INFO]) {
		DBGLOG(RTT, ERROR, "No RTT_ATTRIBUTE_TARGET_INFO\n");
		ret = -EINVAL;
		goto RETURN;
	}

	nla_for_each_nested(attr, attrs[RTT_ATTRIBUTE_TARGET_INFO], tmp) {
		struct RTT_CONFIG *config = NULL;

		if (i >= request->ucConfigNum || i >= CFG_RTT_MAX_CANDIDATES) {
			DBGLOG(RTT, ERROR, "Wrong num %hhu/%hhu/%d\n",
			      i, request->ucConfigNum, CFG_RTT_MAX_CANDIDATES);
			ret = -EINVAL;
			goto RETURN;
		}

		err = NLA_PARSE(tb, RTT_ATTRIBUTE_TARGET_BW,
				nla_data(attr), nla_len(attr),
				nla_set_rtt_config_policy);
		if (err) {
			DBGLOG(RTT, WARN, "Wrong RTT ATTR, %d\n", err);
			ret = err;
			goto RETURN;
		}

		config = &request->arRttConfigs[i++];
		if (tb[RTT_ATTRIBUTE_TARGET_MAC]) {
			COPY_MAC_ADDR(config->aucAddr,
				nla_data(tb[RTT_ATTRIBUTE_TARGET_MAC]));
		}
		if (tb[RTT_ATTRIBUTE_TARGET_TYPE]) {
			config->eType =
				nla_get_u8(tb[RTT_ATTRIBUTE_TARGET_TYPE]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_PEER]) {
			config->ePeer =
				nla_get_u8(tb[RTT_ATTRIBUTE_TARGET_PEER]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_CHAN]) {
			kalMemCopy(&config->rChannel,
				nla_data(tb[RTT_ATTRIBUTE_TARGET_CHAN]),
				sizeof(config->rChannel));
		}
		if (tb[RTT_ATTRIBUTE_TARGET_PERIOD]) {
			config->u2BurstPeriod =
				nla_get_u32(tb[RTT_ATTRIBUTE_TARGET_PERIOD]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_NUM_BURST]) {
			config->u2NumBurstExponent =
				nla_get_u32(tb[RTT_ATTRIBUTE_TARGET_NUM_BURST]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST]) {
			config->ucNumFramesPerBurst =
			    nla_get_u32(tb[RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTM]) {
			config->ucNumRetriesPerRttFrame =
			    nla_get_u32(tb[RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTM]);

		}
		if (tb[RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTMR]) {
			config->ucNumRetriesPerFtmr =
			   nla_get_u32(tb[RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTMR]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_LCI]) {
			config->ucLciRequest =
				nla_get_u8(tb[RTT_ATTRIBUTE_TARGET_LCI]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_LCR]) {
			config->ucLcrRequest =
				nla_get_u8(tb[RTT_ATTRIBUTE_TARGET_LCR]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_BURST_DURATION]) {
			config->ucBurstDuration =
			   nla_get_u32(tb[RTT_ATTRIBUTE_TARGET_BURST_DURATION]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_PREAMBLE]) {
			config->ePreamble =
				nla_get_u8(tb[RTT_ATTRIBUTE_TARGET_PREAMBLE]);
		}
		if (tb[RTT_ATTRIBUTE_TARGET_BW])
			config->eBw = nla_get_u8(tb[RTT_ATTRIBUTE_TARGET_BW]);

		config->u2PreferencePartialTsfTimer = 0;
		config->eEventType = 0;
		config->ucASAP = 1;
		config->ucFtmMinDeltaTime = 40;

#if (CFG_SUPPORT_NAN == 1)
		if (nanIsOn(prGlueInfo->prAdapter) &&
			(config->ePeer == RTT_PEER_NAN_RSTA)) {
			struct ADAPTER *prAdapter =
				prGlueInfo->prAdapter;
			uint16_t u2Band =
				prAdapter->rWifiVar.u2NanRangingBand;
			enum ENUM_BAND eBand =
				cnmGetBandByFreq(
				config->rChannel.center_freq);

			/* Workaround for NanRangingBand */
			if ((u2Band > 2) &&
				(eBand == BAND_2G4)) {
				config->rChannel.center_freq =
					nicChannelNum2Freq
					(g_r5gDwChnl.u4PrimaryChnl,
					BAND_5G) / 1000;
				eBand =
				cnmGetBandByFreq(
				config->rChannel.center_freq);
				config->eBw = WIFI_RTT_BW_80;
				config->rChannel.width =
					WIFI_CHAN_WIDTH_80;
			}

			ucBssIndex = nanGetBssIdxbyBand(
				prGlueInfo->prAdapter,
				(eBand == BAND_2G4)
				? BAND_2G4
				: BAND_5G);

			/* Workaround for preamble */
			if (config->eType == RTT_TYPE_2_SIDED_11MC)
				config->ePreamble = WIFI_RTT_PREAMBLE_VHT;
			else if (config->eType == RTT_TYPE_2_SIDED_11AZ_NTB)
				config->ePreamble = WIFI_RTT_PREAMBLE_HE;
		}
#endif

		DBGLOG(RTT, DEBUG,
			"#%d: MAC=" MACSTR
			" TYPE=%hhu,PEER=%hhu, PRD=%hhu,CHL=(%d,%d),BRST=%hhu,NFTM=%hhu,RFTM=%hhu, RFTMR=%hhu,LCI=%hhu,LCR=%hhu,DUR=%hhu,PRB=%hhu,BW=%hhu\n",
			i - 1, MAC2STR(config->aucAddr), config->eType,
			config->ePeer, config->u2BurstPeriod,
			config->rChannel.width, config->rChannel.center_freq,
			config->u2NumBurstExponent, config->ucNumFramesPerBurst,
			config->ucNumRetriesPerRttFrame,
			config->ucNumRetriesPerFtmr, config->ucLciRequest,
			config->ucLcrRequest, config->ucBurstDuration,
			config->ePreamble, config->eBw);
	}

	if (i != request->ucConfigNum) {
		DBGLOG(RTT, ERROR, "Config Num not match %hhu/%hhu\n",
			i, request->ucConfigNum);
		ret = -EINVAL;
		goto RETURN;
	}

	rStatus = kalIoctlByBssIdx(prGlueInfo,
			   wlanoidHandleRttRequest,
			   request,
			   sizeof(struct PARAM_RTT_REQUEST),
			   &u4BufLen,
			   ucBssIndex);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(RTT, ERROR, "RTT request error:%x\n", rStatus);
		ret = -EFAULT;
	}

RETURN:
	kalMemFree(request, VIR_MEM_TYPE, sizeof(struct PARAM_RTT_REQUEST));
	return ret;
}

int mtk_cfg80211_vendor_cancel_rtt_config(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_RTT_REQUEST *request;
	struct nlattr *attr;
	struct nlattr *attrs = (struct nlattr *) data;
	int tmp;
	uint8_t i = 0;
	uint32_t rStatus, u4BufLen;
	int ret = WLAN_STATUS_SUCCESS;

	DBGLOG(REQ, DEBUG, "vendor command\r\n");

	ASSERT(wiphy);
	ASSERT(wdev);

	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EINVAL;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	request = kalMemAlloc(sizeof(struct PARAM_RTT_REQUEST), VIR_MEM_TYPE);
	if (!request) {
		DBGLOG(RTT, ERROR, "fail to alloc memory for request.\n");
		return -EFAULT;
	}
	kalMemZero(request, sizeof(struct PARAM_RTT_REQUEST));
	request->fgEnable = false;

	attr = nla_find(attrs, data_len, RTT_ATTRIBUTE_TARGET_CNT);
	if (attr) {
		request->ucConfigNum = nla_get_u8(attr);
		DBGLOG(RTT, DEBUG, "TARGET_CNT = %u\n", request->ucConfigNum);
	} else {
		DBGLOG(RTT, ERROR, "No RTT_ATTRIBUTE_TARGET_CNT\n");
		ret = -EINVAL;
		goto RETURN;
	}

	DBGLOG(RTT, DEBUG, "Cancel RTT=%u\n", request->ucConfigNum);

	nla_for_each_attr(attr, attrs, data_len, tmp) {
		if (attr->nla_type == RTT_ATTRIBUTE_TARGET_MAC) {
			struct RTT_CONFIG *config = NULL;

			if (i >= request->ucConfigNum ||
				i >= CFG_RTT_MAX_CANDIDATES) {
				ret = -EINVAL;
				goto RETURN;
			}

			config = &request->arRttConfigs[i++];
			COPY_MAC_ADDR(config->aucAddr, nla_data(attr));
			DBGLOG(RTT, TRACE, "MAC=" MACSTR "\n",
				MAC2STR(config->aucAddr));
		}
	}

	if (i != request->ucConfigNum) {
		ret = -EINVAL;
		goto RETURN;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidHandleRttRequest,
			   request,
			   sizeof(struct PARAM_RTT_REQUEST),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(RTT, WARN, "RTT request error:%x\n", rStatus);
		ret = -EFAULT;
	}

RETURN:
	kalMemFree(request, VIR_MEM_TYPE, sizeof(struct PARAM_RTT_REQUEST));
	return ret;
}
#endif /* CFG_SUPPORT_RTT */

#if CFG_SUPPORT_LLS
#if (defined AOSP_LLS_V1_SINGLE_INTERFACE && AOSP_LLS_V1_SINGLE_INTERFACE == 1)
static void dumpLinkStatsIface(struct STATS_LLS_WIFI_IFACE_STAT *iface)
{
	DBGLOG(REQ, DEBUG, "Dump iface");

	DBGLOG(REQ, DEBUG,
		"0x%p %u "MACSTR " %u %u %u '%s' " MACSTR " %c%c%c %c%c%c %u",
			iface->iface,
			iface->info.mode,
			MAC2STR(iface->info.mac_addr),
			iface->info.state,
			iface->info.roaming,
			iface->info.capabilities,
			iface->info.ssid,
			MAC2STR(iface->info.bssid),
			iface->info.ap_country_str[0],
			iface->info.ap_country_str[1],
			iface->info.ap_country_str[2],
			iface->info.country_str[0],
			iface->info.country_str[1],
			iface->info.country_str[2],
			iface->info.time_slicing_duty_cycle_percent);

	DBGLOG(REQ, DEBUG, "%u %llu %u %u %u %u %u %u %d %d %d AC[] [%u]",
			iface->beacon_rx,
			iface->average_tsf_offset,
			iface->leaky_ap_detected,
			iface->leaky_ap_avg_num_frames_leaked,
			iface->leaky_ap_guard_time,
			iface->mgmt_rx,
			iface->mgmt_action_rx,
			iface->mgmt_action_tx,
			iface->rssi_mgmt,
			iface->rssi_data,
			iface->rssi_ack,
			/* AC */
			iface->num_peers);
}
#endif

static void dumpLinkStatsMultiLinkIface(uint8_t bss_idx,
		struct STATS_LLS_WIFI_IFACE_ML_STAT *ml_iface)
{
	DBGLOG(REQ, DEBUG, "Dump ml_iface, bss_isx=%u", bss_idx);

	DBGLOG(REQ, DEBUG,
		"0x%p %u "MACSTR " %u %u %u '%s' " MACSTR " %c%c%c %c%c%c %u",
		ml_iface->iface,
		ml_iface->info.mode,
		MAC2STR(ml_iface->info.mac_addr),
		ml_iface->info.state,
		ml_iface->info.roaming,
		ml_iface->info.capabilities,
		ml_iface->info.ssid,
		MAC2STR(ml_iface->info.bssid),
		ml_iface->info.ap_country_str[0],
		ml_iface->info.ap_country_str[1],
		ml_iface->info.ap_country_str[2],
		ml_iface->info.country_str[0],
		ml_iface->info.country_str[1],
		ml_iface->info.country_str[2],
		ml_iface->info.time_slicing_duty_cycle_percent);
}

static void dumpLinkStatsLink(struct STATS_LLS_WIFI_LINK_STAT *link,
		int32_t link_id)
{
	DBGLOG(REQ, DEBUG, "Dump ml_link[%d]", link_id);

	DBGLOG(REQ, DEBUG,
		"%u %u %d %u  %u %llu %u %u %u %u %u %u %d %d %d AC[] %u [%u]",
		link->link_id,
		link->state,
		link->radio,
		link->frequency,

		link->beacon_rx,
		link->average_tsf_offset,
		link->leaky_ap_detected,
		link->leaky_ap_avg_num_frames_leaked,
		link->leaky_ap_guard_time,
		link->mgmt_rx,
		link->mgmt_action_rx,
		link->mgmt_action_tx,
		link->rssi_mgmt,
		link->rssi_data,
		link->rssi_ack,
		/* AC */
		link->time_slicing_duty_cycle_precent,
		link->num_peers);
}

static void dumpLinkStatsAc(struct STATS_LLS_WMM_AC_STAT *ac_stat,
		enum ENUM_STATS_LLS_AC ac)
{
	static const char * const s[STATS_LLS_WIFI_AC_MAX] = {
		"VO", "VI", "BE", "BK"};

	DBGLOG(REQ, DEBUG,
	       "AC[%s] %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
	       s[(uint32_t)ac],
	       ac_stat[ac].ac,
	       ac_stat[ac].tx_mpdu,
	       ac_stat[ac].rx_mpdu,
	       ac_stat[ac].tx_mcast,
	       ac_stat[ac].rx_mcast,
	       ac_stat[ac].rx_ampdu,
	       ac_stat[ac].tx_ampdu,
	       ac_stat[ac].mpdu_lost,
	       ac_stat[ac].retries,
	       ac_stat[ac].retries_short,
	       ac_stat[ac].retries_long,
	       ac_stat[ac].contention_time_min,
	       ac_stat[ac].contention_time_max,
	       ac_stat[ac].contention_time_avg,
	       ac_stat[ac].contention_num_samples);
}


static void dumpLinkStatsPeerInfo(struct STATS_LLS_PEER_INFO *peer,
				uint32_t idx)
{
	static const char * const type[STATS_LLS_WIFI_PEER_INVALID + 1] = {
		"STA", "AP", "P2P_GO", "P2P_CLIENT", "NAN", "TDLS", "INVALID"};

	DBGLOG(REQ, DEBUG, "Peer(%u) %u(%s) " MACSTR " %u %u %u [%u]",
			idx,
			peer->type, type[(uint32_t)peer->type],
			MAC2STR(peer->peer_mac_address),
			peer->capabilities,
			peer->bssload.sta_count,
			peer->bssload.chan_util,
			peer->num_rate);
}


static void dumpLinkStatsRate(struct STATS_LLS_RATE_STAT *rate, uint32_t idx)
{
	static const char * const preamble[] = {
		"OFDM", "CCK", "HT", "VHT", "HE", "EHT", "", ""};

	DBGLOG(REQ, DEBUG, "Rate(%u) %u(%s) %u %u %u %u %u %u %u %u %u %u",
			idx,
			rate->rate.preamble, rate->rate.preamble > 5 ? "" :
			preamble[rate->rate.preamble],
			rate->rate.nss,
			rate->rate.bw,
			rate->rate.rateMcsIdx,
			rate->rate.bitrate,
			rate->tx_mpdu,
			rate->rx_mpdu,
			rate->mpdu_lost,
			rate->retries,
			rate->retries_short,
			rate->retries_long);
}

static void dumpLinkStatsRadio(struct STATS_LLS_WIFI_RADIO_STAT *radio,
				uint32_t idx)
{
	DBGLOG(REQ, DEBUG, "Radio(%u) %d %u %u %u %p %u %u %u %u %u %u %u [%u]",
			idx,
			radio->radio,
			radio->on_time,
			radio->tx_time,
			radio->num_tx_levels,
			radio->tx_time_per_levels,
			radio->rx_time,
			radio->on_time_scan,
			radio->on_time_nbd,
			radio->on_time_gscan,
			radio->on_time_roam_scan,
			radio->on_time_pno_scan,
			radio->on_time_hs20,
			radio->num_channels);
}

static void dumpLinkStatsPowerLevels(uint8_t *ptr, uint8_t band, uint32_t size)
{
	DBGLOG(REQ, DEBUG, "PowerLevels: %p, #%u, size=%u\n",
			ptr, band, size);
	DBGLOG_HEX(REQ, DEBUG, ptr, size);
}

static void dumpLinkStatsChannel(struct STATS_LLS_CHANNEL_STAT *channel,
		uint32_t idx)
{
	DBGLOG(REQ, DEBUG, "Channel(%u) %u %d %d %d %u %u",
			idx,
			channel->channel.width,
			channel->channel.center_freq,
			channel->channel.center_freq0,
			channel->channel.center_freq1,
			channel->on_time,
			channel->cca_busy_time);
}

/**
 * find_peer_starec() - return a station record by matching peer's MAC address
 */
struct STA_RECORD *find_peer_starec(struct ADAPTER *prAdapter,
		struct STATS_LLS_PEER_INFO *peer_info)
{
	struct STA_RECORD *prStaRec;
	uint8_t pucIndex;
	uint8_t ucIdx;

	if (!wlanGetWlanIdxByAddress(prAdapter, peer_info->peer_mac_address,
				&pucIndex))
		return NULL;

	for (ucIdx = 0; ucIdx < CFG_STA_REC_NUM; ucIdx++) {
		prStaRec = cnmGetStaRecByIndex(prAdapter, ucIdx);
		if (prStaRec && prStaRec->ucWlanIndex == pucIndex)
			break;
	}

	if (ucIdx == CFG_STA_REC_NUM)
		return NULL;

	return prStaRec;
}

/**
 * Return the accumulated MPDU count of the given rate parameters.
 */
uint32_t receivedMpduCount(struct STA_RECORD *sta_rec,
		struct STATS_LLS_RATE_STAT *rate_stats,
		uint32_t ofdm_idx, uint32_t cck_idx)
{
	struct STATS_LLS_WIFI_RATE *rate = &rate_stats->rate;
	uint32_t n = 0;
	uint32_t mcsIdx;

	if (!sta_rec)
		return 0;

	mcsIdx = rate->rateMcsIdx;
	if (rate->preamble == LLS_MODE_OFDM)
		n = sta_rec->u4RxMpduOFDM[rate->nss][rate->bw][ofdm_idx];
	else if (rate->preamble == LLS_MODE_CCK)
		n = sta_rec->u4RxMpduCCK[rate->nss][rate->bw][cck_idx];
	else if (rate->preamble == LLS_MODE_HT)
		n = sta_rec->u4RxMpduHT[rate->nss][rate->bw][mcsIdx];
	else if (rate->preamble == LLS_MODE_VHT)
		n = sta_rec->u4RxMpduVHT[rate->nss][rate->bw][mcsIdx];
	else if (rate->preamble == LLS_MODE_HE)
		n = sta_rec->u4RxMpduHE[rate->nss][rate->bw][mcsIdx];
	else if (rate->preamble == LLS_MODE_EHT)
		n = sta_rec->u4RxMpduEHT[rate->nss][rate->bw][mcsIdx];
	return n;
}

uint8_t isValidRate(struct STATS_LLS_RATE_STAT *rate_stats,
	uint32_t ofdm_idx, uint32_t cck_idx)
{
	struct STATS_LLS_WIFI_RATE *rate = &rate_stats->rate;

	switch (rate->preamble) {
	case LLS_MODE_OFDM:
		if (rate->nss >= 1 ||
		    rate->bw >= STATS_LLS_MAX_OFDM_BW_NUM ||
		    ofdm_idx >= STATS_LLS_OFDM_NUM)
			goto invalid_rate;
		break;
	case LLS_MODE_CCK:
		if (rate->nss >= 1 ||
		    rate->bw >= STATS_LLS_MAX_CCK_BW_NUM ||
		    cck_idx >= STATS_LLS_CCK_NUM)
			goto invalid_rate;
		break;
	case LLS_MODE_HT:
		if (rate->nss >= 1 ||
		    rate->bw >= STATS_LLS_MAX_HT_BW_NUM ||
		    rate->rateMcsIdx >= STATS_LLS_HT_NUM)
			goto invalid_rate;
		break;
	case LLS_MODE_VHT:
		if (rate->nss >= STATS_LLS_MAX_NSS_NUM ||
		    rate->bw >= STATS_LLS_MAX_VHT_BW_NUM ||
		    rate->rateMcsIdx >= STATS_LLS_VHT_NUM)
			goto invalid_rate;
		break;
	case LLS_MODE_HE:
		if (rate->nss >= STATS_LLS_MAX_NSS_NUM ||
		    rate->bw >= STATS_LLS_MAX_HE_BW_NUM ||
		    rate->rateMcsIdx >= STATS_LLS_HE_NUM)
			goto invalid_rate;
		break;
	case LLS_MODE_EHT:
		if (rate->nss >= STATS_LLS_MAX_NSS_NUM ||
		    rate->bw >= STATS_LLS_MAX_EHT_BW_NUM ||
		    rate->rateMcsIdx >= STATS_LLS_EHT_NUM)
			goto invalid_rate;
		break;
	default:
		goto invalid_rate;
	}
	return TRUE;

invalid_rate:
	DBGLOG(REQ, ERROR, "BAD:preamble=%u nss=%u bw=%u mcs=%u ofdm=%u cck=%u",
			rate->preamble, rate->nss, rate->bw, rate->rateMcsIdx,
			ofdm_idx, cck_idx);
	return FALSE;
}

/* Given a BSS index,
 * return the band index bitmap of the MLD the BSS affiliated to.
 * LLS can collect the statistics data from [i] with the bitmap set BIT(i).
 */
static uint8_t bandMaskByBssIdx(struct ADAPTER *prAdapter, uint8_t bss_idx)
{
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, bss_idx);
	uint8_t ucHwBandIdxBitmap = 0;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo;
#endif

	if (!prBssInfo) {
		if (prWifiVar->fgLinkStatsDump)
			DBGLOG(REQ, WARN, "prBssInfo == NULL for bss_idx=%u\n",
					bss_idx);
		return 0;
	}

	if (prBssInfo->eHwBandIdx < ENUM_BAND_NUM)
		ucHwBandIdxBitmap = BIT(prBssInfo->eHwBandIdx);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (prMldBssInfo)
		ucHwBandIdxBitmap |= prMldBssInfo->ucHwBandBitmap;
#endif

	return ucHwBandIdxBitmap;
}

static void updateApRec(struct ADAPTER *prAdapter,
			struct STATS_LLS_PEER_INFO *dst_peer)
{
	struct STATS_LLS_PEER_AP_REC *prPeerApRec = NULL;
	uint8_t i;
	uint8_t j;

	for (i = 0; i < KAL_AIS_NUM; i++) {
		for (j = 0; j < MLD_LINK_MAX; j++) {
			prPeerApRec = &prAdapter->rPeerApRec[i][j];

			if (UNEQUAL_MAC_ADDR(dst_peer->peer_mac_address,
					     prPeerApRec->mac_addr))
				continue;
			dst_peer->bssload.sta_count = prPeerApRec->sta_count;
			dst_peer->bssload.chan_util = prPeerApRec->chan_util;
		}
	}
}

/**
 * fill_peer_info() - Collect the associated peer info in the given BSS
 *
 * @prAdapter: adapter pointer to look up required information
 * @dst: pointing to destination buffer to write STATS_LLS_PEER_INFO.
 * @src: pointer to EMI holding PEER_INFO_RATE_STAT[CFG_STA_REC_NUM]
 * @num_peers: num_peers in upper layer structure before STATS_LLS_PEER_INFO[],
 *	       indicating the number of STATS_LLS_PEER_INFO immediately
 *	       following at the structure.
 * @bss_idx: BSS index of the queried link
 *
 * Traverse all reported starec in EMI and collect by matching the BSS index.
 */
static uint32_t fill_peer_info(struct ADAPTER *prAdapter, uint8_t *dst,
			       uint32_t *num_peers, uint8_t bss_idx)
{
	struct STATS_LLS_PEER_INFO *dst_peer;
	struct STATS_LLS_PEER_INFO *src_peer;
	struct STATS_LLS_RATE_STAT *dst_rate;
	struct STATS_LLS_RATE_STAT *src_rate;
	struct STA_RECORD *sta_rec;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t i;
	uint32_t j;
	uint8_t *orig = dst;
	int32_t ofdm_idx;
	int32_t cck_idx;
	uint32_t rxMpduCount;
	struct STATS_LLS_PEER_INFO peer_info = {0};
	struct PEER_INFO_RATE_STAT *prPeer = prAdapter->prLinkStatsPeerInfo;

	*num_peers = 0;
	for (i = 0; i < CFG_STA_REC_NUM; i++, prPeer++) {
		src_peer = &prPeer->peer;
		if (src_peer->type >= STATS_LLS_WIFI_PEER_INVALID)
			continue;

		kalMemCopyFromIo(&peer_info, src_peer,
				sizeof(struct STATS_LLS_PEER_INFO));

		DBGLOG(REQ, TRACE, "Peer MAC: " MACSTR,
				MAC2STR(peer_info.peer_mac_address));
		sta_rec = find_peer_starec(prAdapter, &peer_info);
		if (!sta_rec) {
			if (prWifiVar->fgLinkStatsDump)
				DBGLOG(REQ, WARN, "MAC not found: " MACSTR,
					MAC2STR(peer_info.peer_mac_address));
			continue;
		}

		if (prAdapter->ucLinkStatsBssNum != 1 &&
		    sta_rec->ucBssIndex != bss_idx)
			continue; /* collect per BSS, not a collecting one */

		if (prWifiVar->fgLinkStatsDump)
			DBGLOG(REQ, DEBUG,
			       "Peer=%u type=%u", i, peer_info.type);

		(*num_peers)++;
		dst_peer = (struct STATS_LLS_PEER_INFO *)dst;
		*dst_peer = peer_info;

		if (dst_peer->type == STATS_LLS_WIFI_PEER_AP)
			updateApRec(prAdapter, dst_peer);

		if (prWifiVar->fgLinkStatsDump)
			dumpLinkStatsPeerInfo(dst_peer, i);
		dst += offsetof(struct STATS_LLS_PEER_INFO, rate_stats);

		dst_peer->num_rate = 0;
		dst_rate = (struct STATS_LLS_RATE_STAT *)dst;
		src_rate = prPeer->rate;
		ofdm_idx = -1;
		cck_idx = -1;
		for (j = 0; j < STATS_LLS_RATE_NUM; j++, src_rate++) {
			if (unlikely(src_rate->rate.preamble == LLS_MODE_OFDM))
				ofdm_idx++;
			if (unlikely(src_rate->rate.preamble == LLS_MODE_CCK))
				cck_idx++;

			if (!isValidRate(src_rate, ofdm_idx, cck_idx))
				continue;
			rxMpduCount = receivedMpduCount(sta_rec,
					src_rate, ofdm_idx >= 0 ? ofdm_idx : 0,
					cck_idx >= 0 ? cck_idx : 0);
			if (src_rate->tx_mpdu || src_rate->mpdu_lost ||
			    src_rate->retries || rxMpduCount) {
				dst_peer->num_rate++;
				if (prWifiVar->fgLinkStatsDump) {
					DBGLOG(REQ, TRACE, "valid rate %u", j);
					DBGLOG(REQ, TRACE,
						"memcpy(dst_rate=(%td), src_rate=(%td))",
						(uint8_t *)dst_rate -
						     (uint8_t *)dst,
						(uint8_t *)src_rate -
						     (uint8_t *)prPeer->rate);
				}
				kalMemCopyFromIo(dst_rate, src_rate,
					sizeof(struct STATS_LLS_RATE_STAT));

				dst_rate->rx_mpdu = rxMpduCount;
				if (prWifiVar->fgLinkStatsDump)
					dumpLinkStatsRate(dst_rate, j);
				dst_rate++;
			}
		}
		dst += sizeof(struct STATS_LLS_RATE_STAT) * dst_peer->num_rate;
		DBGLOG(REQ, TRACE, "peer[%u] contains %u rates",
				i, dst_peer->num_rate);
	}

	DBGLOG(REQ, TRACE, "advanced %td bytes", dst - orig);
	return dst - orig;
}

static void sum_ac_rx_mpdu(struct STATS_LLS_WMM_AC_STAT ac[],
		struct BSS_INFO *prBssInfo)
{
	uint32_t *pu4RxMpduAc;
	uint8_t i = 0;

	pu4RxMpduAc = prBssInfo->u4RxMpduAc;
	for (i = 0; i < STATS_LLS_WIFI_AC_MAX; i++)
		ac[i].rx_mpdu += pu4RxMpduAc[i];
}

#if AOSP_LLS_V1_SINGLE_INTERFACE /* For legacy Android S/T */
static void fill_iface_ac_mpdu(struct ADAPTER *prAdapter, uint8_t bss_idx,
	struct STATS_LLS_WIFI_IFACE_STAT *iface)
{
	struct BSS_INFO *prBssInfo;
	uint32_t i;

	if (prAdapter->ucLinkStatsBssNum == 1) { /* legacy */
		/* FW report only one record, all data are summed up into one */
		for (i = 0; i < ARRAY_SIZE(prAdapter->aprBssInfo); i++) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
			if (!prBssInfo)
				continue;

			sum_ac_rx_mpdu(iface->ac, prBssInfo);
		}
	} else { /* report per BSS stats */
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, bss_idx);

		if (prBssInfo)
			sum_ac_rx_mpdu(iface->ac, prBssInfo);
	}
}

/**
 * STATS_LLS_WIFI_IFACE_STAT
 *     ...
 *     STATS_LLS_WMM_AC_STAT ac[STATS_LLS_WIFI_AC_MAX] *rx_mpdu
 *     num_peers
 *     --------------------------
 *     STATS_LLS_PEER_INFO[] <- up to 27
 *          ...
 *          num_rate
 *          STATS_LLS_RATE_STAT[] <- up to 200 () *rx_mpdu
 */
static uint32_t fill_iface(struct ADAPTER *prAdapter, uint8_t *dst,
			   uint8_t bss_idx)
{
	struct STATS_LLS_WIFI_IFACE_STAT *iface;
	uint8_t *orig = dst;
	uint8_t ac = 0;
	uint32_t iface_offset = bss_idx;

	if (prAdapter->ucLinkStatsBssNum == 1)
		iface_offset = 0;
	kalMemCopyFromIo(dst, &prAdapter->prLinkStatsIface[iface_offset],
			sizeof(struct STATS_LLS_WIFI_IFACE_STAT));
	iface = (struct STATS_LLS_WIFI_IFACE_STAT *)dst;

	fill_iface_ac_mpdu(prAdapter, bss_idx, iface);

	if (prAdapter->rWifiVar.fgLinkStatsDump) {
		dumpLinkStatsIface(iface);
		for (ac = 0; ac < STATS_LLS_WIFI_AC_MAX; ac++)
			dumpLinkStatsAc(iface->ac, ac);
	}
	dst += offsetof(struct STATS_LLS_WIFI_IFACE_STAT, peer_info);

	dst += fill_peer_info(prAdapter, dst, &iface->num_peers, bss_idx);

	DBGLOG(REQ, TRACE, "advanced %td bytes, %u peers",
			dst - orig, iface->num_peers);

	return dst - orig;
}
#endif /* AOSP_LLS_V1_SINGLE_INTERFACE */

/**
 * Copy per-AC RX MPDU count stored in the BSS info with the given bss_idx.
 */
static void fill_ml_link_ac_mpdu(struct ADAPTER *prAdapter, uint8_t bss_idx,
	struct STATS_LLS_WIFI_LINK_STAT *link)
{
	struct BSS_INFO *prBssInfo;
	uint32_t i;

	if (prAdapter->ucLinkStatsBssNum == 1) {
		/* FW report only one record, all data are summed up into one */
		for (i = 0; i < ARRAY_SIZE(prAdapter->aprBssInfo); i++) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
			if (!prBssInfo)
				continue;

			sum_ac_rx_mpdu(link->ac, prBssInfo);
		}
	} else {
		/* ASSERT(prAdapter->ucLinkStatsBssNum == BSSID_NUM) */
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, bss_idx);
		if (!prBssInfo)
			return;

		sum_ac_rx_mpdu(link->ac, prBssInfo);
	}
}

static uint32_t find_bss_group(struct ADAPTER *prAdapter, uint8_t bss_idx)
{
	uint32_t bss_idx_bitmap = 0;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo;
	struct BSS_INFO *prBssInfo;
	struct BSS_INFO *bss;

	do {
		bss_idx_bitmap |= BIT(bss_idx);
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, bss_idx);
		if (!prBssInfo)
			break;

		prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
		if (prMldBssInfo) {
			LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
					rLinkEntryMld, struct BSS_INFO) {
				bss_idx_bitmap |= BIT(bss->ucBssIndex);
			}
		}
	} while (0);
#else
	bss_idx_bitmap |= BIT(bss_idx);
#endif
	return bss_idx_bitmap;
}

/**
 * fill_ml_link_stats() - Collect the associated link info with the bss_idx
 *
 * @prAdapter: adapter pointer to look up required information
 * @dst: pointing to destination buffer to write STATS_LLS_WIFI_LINK_STAT
 * @num_links: num_links in upper layer structure before
 *	       STATS_LLS_WIFI_LINK_STAT[], indicating the number of
 *	       STATS_LLS_WIFI_LINK_STAT immediately following at the structure.
 * @bbss_idx: the BSS index associated to the queried netdev
 */
static uint32_t fill_ml_link_stats(struct ADAPTER *prAdapter, uint8_t *dst,
				   int32_t *num_links, uint8_t bss_idx)
{
	int max_bss_idx = prAdapter->ucLinkStatsBssNum;
	struct BSS_INFO *prBssInfo;
	struct STATS_LLS_WIFI_LINK_STAT *link;
	uint8_t *orig = dst;
	int b;
	int bss_idx_bitmap;
	struct STA_RECORD *prStaRec;
	static const enum nl80211_band band[BAND_NUM] = {
		[BAND_2G4] = NL80211_BAND_2GHZ,
		[BAND_5G] = NL80211_BAND_5GHZ,
#if (CFG_SUPPORT_WIFI_6G == 1)
		[BAND_6G] = NL80211_BAND_6GHZ,
#endif
		};

	link = (struct STATS_LLS_WIFI_LINK_STAT *)dst;

	*num_links = 0;
	bss_idx_bitmap = find_bss_group(prAdapter, bss_idx);

	for (b = 0; b < max_bss_idx && b < prAdapter->ucLinkStatsBssNum; b++) {
		struct STATS_LLS_WIFI_IFACE_STAT *prLinkStatsIface;

		if (!(bss_idx_bitmap & BIT(b)))
			continue;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, b);
		if (!prBssInfo) {
			DBGLOG(REQ, WARN, "BSS index %u no BSS found\n", b);
			continue;
		}

		prLinkStatsIface = &prAdapter->prLinkStatsIface[b];
		(*num_links)++;

		link = (struct STATS_LLS_WIFI_LINK_STAT *)dst;

		prStaRec = prBssInfo->prStaRecOfAP;
		if (prStaRec) {
			link->link_id = prStaRec->ucLinkId;
			link->state = cnmStaRecIsActive(prAdapter, prStaRec) ?
					WIFI_LINK_STATE_IN_USE :
					WIFI_LINK_STATE_NOT_IN_USE;
		} else {
			link->link_id = 0;
			link->state = WIFI_LINK_STATE_IN_USE;
		}

		link->radio = prBssInfo->eHwBandIdx;
		link->frequency = ieee80211_channel_to_frequency(
				prBssInfo->ucPrimaryChannel,
				band[prBssInfo->eBand]);

		/* Copy link stats data for this BSS[b]
		 * src structure: STATS_LLS_WIFI_IFACE_STAT
		 * dst structure: STATS_LLS_WIFI_LINK_STAT
		 *  1. beacon_rx, special handling, since there's a hole between
		 *     beacon_rx and average_tsf_offset
		 *  2. average_tsf_offset, ..., rssi_ack, before ac[]
		 *  3. ac[] followed by num_peers,
		 */
		/* 1. beacon_rx, special handling */
		kalMemCopyFromIo(&link->beacon_rx, &prLinkStatsIface->beacon_rx,
			sizeof(uint32_t));
		if (prAdapter->rWifiVar.fgLinkStatsDump) {
			DBGLOG(REQ, DEBUG,
			       "Copy beacon_rx to ac of %zu bytes",
			       sizeof(uint32_t));
		}

		/*  2. average_tsf_offset, ..., rssi_ack, before ac[] */
		kalMemCopyFromIo(&link->average_tsf_offset,
			&prLinkStatsIface->average_tsf_offset,
			offsetof(struct STATS_LLS_WIFI_IFACE_STAT, ac) -
			offsetof(struct STATS_LLS_WIFI_IFACE_STAT,
				average_tsf_offset));

		if (prAdapter->rWifiVar.fgLinkStatsDump) {
			DBGLOG(REQ, DEBUG,
			       "Copy average_tsf_offset to ac of %zu bytes",
			       offsetof(struct STATS_LLS_WIFI_IFACE_STAT, ac) -
			       offsetof(struct STATS_LLS_WIFI_IFACE_STAT,
			       average_tsf_offset));
		}

		/*  3. ac[] followed by num_peers */
		kalMemCopyFromIo(&link->ac, &prLinkStatsIface->ac,
				sizeof(link->ac));
		if (prAdapter->rWifiVar.fgLinkStatsDump) {
			DBGLOG(REQ, DEBUG, "Copy ac of %zu bytes",
			       sizeof(link->ac));
		}

		fill_ml_link_ac_mpdu(prAdapter, b, link);
		link->time_slicing_duty_cycle_precent =
			prLinkStatsIface->info.time_slicing_duty_cycle_percent;

		if (prAdapter->rWifiVar.fgLinkStatsDump) {
			int ac;

			dumpLinkStatsLink(link, b);
			for (ac = 0; ac < STATS_LLS_WIFI_AC_MAX; ac++)
				dumpLinkStatsAc(link->ac, ac);
		}
		link->num_peers = 0;
		dst += offsetof(struct STATS_LLS_WIFI_LINK_STAT, peer_info);

		/* dst is pointing to STATS_LLS_WIFI_LINK_STAT.peer_info[0];
		 * increment link->num_peers when appending peer_info records.
		 */
		dst += fill_peer_info(prAdapter, dst, &link->num_peers,
				      bss_idx);
	}
	DBGLOG(REQ, TRACE, "advanced %td bytes, %u links",
			dst - orig, *num_links);
	return dst - orig;
}

/**
 * STATS_LLS_WIFI_IFACE_ML_STAT
 *     iface
 *     info: mode, mac_addr, ..., country_str, time_slicing_duty_cycle_percent
 *     ...
 *     num_links
 *         link_id, radio, frequency
 *         beacon_rx, average_tsf_ofset, ...,
 *         STATS_LLS_WMM_AC_STAT ac[STATS_LLS_WIFI_AC_MAX] *rx_mpdu
 *         num_peers
 *         --------------------------
 *         STATS_LLS_PEER_INFO[] <- up to 27
 *             ...
 *                 num_rate
 *                 STATS_LLS_RATE_STAT[] <- up to 200 () *rx_mpdu
 *
 * TODO: Sum up time_slicing_duty_cycle_percent from all BSSes?
 */
uint32_t fill_ml_iface(struct ADAPTER *prAdapter, uint8_t *dst, uint8_t bss_idx)
{
	struct STATS_LLS_WIFI_IFACE_ML_STAT *ml_iface;
	uint8_t *orig = dst;

	_Static_assert(offsetof(struct STATS_LLS_WIFI_IFACE_ML_STAT, num_links)
			==
			offsetof(struct STATS_LLS_WIFI_IFACE_STAT, beacon_rx),
			"iface + info size not matched");

	if (bss_idx >= prAdapter->ucLinkStatsBssNum) {
		DBGLOG(REQ, WARN, "bss_idx out of range, %u >= %u",
				bss_idx, prAdapter->ucLinkStatsBssNum);
		return 0;
	}

	/* copy the netdev level common part from iface to
	 * info->time_slicing_duty_cycle_percent, right before num_links
	 */
	ml_iface = (struct STATS_LLS_WIFI_IFACE_ML_STAT *)dst;
	kalMemCopyFromIo(dst, &prAdapter->prLinkStatsIface[bss_idx],
		offsetof(struct STATS_LLS_WIFI_IFACE_ML_STAT, num_links));
	dst += offsetof(struct STATS_LLS_WIFI_IFACE_ML_STAT, num_links);
	DBGLOG(REQ, TRACE,
		"Copy STATS_LLS_WIFI_IFACE_ML_STAT [%u] up to num_links of %zu bytes",
		bss_idx,
		offsetof(struct STATS_LLS_WIFI_IFACE_ML_STAT, num_links));

	if (prAdapter->rWifiVar.fgLinkStatsDump)
		dumpLinkStatsMultiLinkIface(bss_idx, ml_iface);

	/* dst is pointing to STATS_LLS_WIFI_IFACE_ML_STAT.links[0];
	 * increment ml_iface->num_links when appending link records.
	 */
	ml_iface->num_links = 0;

	dst = (uint8_t *)&ml_iface->links;
	dst += fill_ml_link_stats(prAdapter, dst, &ml_iface->num_links,
				  bss_idx);

	return dst - orig;
}

/**
 * STATS_LLS_WIFI_RADIO_STAT[] <-- check by eBand considering MLO as well
 *     ...
 *     num_channels
 *     STATS_LLS_CHANNEL_STAT[] <-- up to 46 (2.4 + 5G; 6G will be more)
 */
static uint32_t fill_radio(struct ADAPTER *prAdapter, uint8_t *dst,
			   uint32_t num_radio, uint8_t band_map)
{
	struct STATS_LLS_WIFI_RADIO_STAT *radio;
	struct STATS_LLS_CHANNEL_STAT *src_ch;
	struct STATS_LLS_CHANNEL_STAT *dst_ch;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t *orig = dst;
	uint32_t i, j;
	struct WIFI_RADIO_CHANNEL_STAT *prRadio;

	*(uint32_t *)orig = 0;
	dst += sizeof(uint32_t);

	prRadio = prAdapter->prLinkStatsRadioInfo;
	for (i = 0; i < num_radio; i++, prRadio++) {
		if ((band_map & BIT(i)) == 0)
			continue;

		(*(uint32_t *)orig)++;

		kalMemCopyFromIo(dst, prRadio,
				sizeof(struct STATS_LLS_WIFI_RADIO_STAT));
		radio = (struct STATS_LLS_WIFI_RADIO_STAT *)dst;
		if (prAdapter->pu4TxTimePerLevels)
			radio->num_tx_levels = TX_POWER_LEVELS;
		dst += offsetof(struct STATS_LLS_WIFI_RADIO_STAT, channels);

		if (prWifiVar->fgLinkStatsDump)
			dumpLinkStatsRadio(radio, i);
		radio->num_channels = 0;

		src_ch = prRadio->channel;
		dst_ch = (struct STATS_LLS_CHANNEL_STAT *)dst;
		for (j = 0; j < STATS_LLS_CH_NUM; j++, src_ch++) {
			if (!src_ch->channel.center_freq ||
				(!src_ch->on_time && !src_ch->cca_busy_time))
				continue;
			radio->num_channels++;
			kalMemCopyFromIo(dst_ch, src_ch, sizeof(*dst_ch));
			if (prWifiVar->fgLinkStatsDump)
				dumpLinkStatsChannel(dst_ch, j);
			dst_ch++;
		}

		dst += sizeof(struct STATS_LLS_CHANNEL_STAT) *
			radio->num_channels;

		DBGLOG(REQ, TRACE, "radio[%u] contains %u channels",
				i, radio->num_channels);
	}

	DBGLOG(REQ, TRACE, "advanced %td bytes", dst - orig);
	return dst - orig;
}

/**
 * Stored in pu4TxTimePerLevels in (uint32_t * size).
 */
static uint32_t fill_power_levels(struct ADAPTER *prAdapter, uint8_t *dst,
		uint32_t num_radio, uint8_t band_map,
		uint32_t u4TxTimePerLevelsSize)
{
	uint8_t *orig = dst;
	uint8_t i = 0;
	uint32_t *pu4TxTimePerLevels = prAdapter->pu4TxTimePerLevels;

	if (!pu4TxTimePerLevels)
		return 0;

	for (i = 0; i < num_radio; i++) {
		if ((band_map & BIT(i)) == 0)
			continue;

		kalMemCopyFromIo(dst, pu4TxTimePerLevels + TX_POWER_LEVELS * i,
				sizeof(uint32_t) * TX_POWER_LEVELS);

		if (prAdapter->rWifiVar.fgLinkStatsDump)
			dumpLinkStatsPowerLevels(dst, i,
					sizeof(uint32_t) * TX_POWER_LEVELS);
		dst += sizeof(uint32_t) * TX_POWER_LEVELS;
	}

	DBGLOG(REQ, TRACE, "Copy power level %td bytes", dst - orig);

	return dst - orig;
}

/* Copy and dump source buffer data for debugging */
static void dumpSourceBufferData(struct ADAPTER *prAdapter, uint8_t ucBssIdx)
{
	int i;
	struct PEER_INFO_RATE_STAT *peer = (struct PEER_INFO_RATE_STAT *)
					   &prAdapter->rLinkStatsDestBuffer;

	if (!prAdapter->rWifiVar.fgLinkStatsDump)
		return;

	DBGLOG(REQ, DEBUG, "LLS iface[bssidx=%u]\n", ucBssIdx);

	kalMemCopyFromIo(&prAdapter->rLinkStatsDestBuffer,
			&prAdapter->prLinkStatsIface[ucBssIdx],
			sizeof(struct STATS_LLS_WIFI_IFACE_STAT));
	DBGLOG_HEX(REQ, DEBUG, &prAdapter->rLinkStatsDestBuffer,
			sizeof(struct STATS_LLS_WIFI_IFACE_STAT));

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		kalMemCopyFromIo(&prAdapter->rLinkStatsDestBuffer,
				&prAdapter->prLinkStatsPeerInfo[i],
				sizeof(struct PEER_INFO_RATE_STAT));
		if (peer->peer.type >= STATS_LLS_WIFI_PEER_INVALID) {
			DBGLOG(REQ, DEBUG, "Peer[%u].type = %u\n",
					i, peer->peer.type);
			continue;
		}

		DBGLOG(REQ, DEBUG, "Dump peer_info[%u]\n", i);
		DBGLOG_HEX(REQ, DEBUG, &prAdapter->rLinkStatsDestBuffer,
				sizeof(struct PEER_INFO_RATE_STAT));
	}

	for (i = 0; i < ENUM_BAND_NUM; i++) {
		DBGLOG(REQ, DEBUG, "Dump radio[%u]\n", i);
		kalMemCopyFromIo(&prAdapter->rLinkStatsDestBuffer,
			&prAdapter->prLinkStatsRadioInfo[i],
			sizeof(struct WIFI_RADIO_CHANNEL_STAT));
		DBGLOG_HEX(REQ, DEBUG, &prAdapter->rLinkStatsDestBuffer,
				sizeof(struct WIFI_RADIO_CHANNEL_STAT));
	}
}
#endif /* CFG_SUPPORT_LLS */

int mtk_cfg80211_vendor_llstats_get_info(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	int32_t rStatus = -EOPNOTSUPP;
#if CFG_SUPPORT_LLS
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	uint32_t u4QueryInfoLen;

#if (CFG_SUPPORT_STATS_ONE_CMD == 0)
	union {
		struct CMD_GET_STATS_LLS cmd;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		struct UNI_EVENT_LINK_LAYER_STATS rtlv;
#else
		struct EVENT_STATS_LLS_DATA data;
#endif
	} query = {0};

	uint32_t u4QueryBufLen = sizeof(query);
#else
	struct PARAM_GET_STATS_ONE_CMD rParam;
#endif

	uint8_t *buf = NULL;
	struct sk_buff *skb = NULL;

	uint8_t *ptr = NULL;
	uint8_t ucBssIdx;
	uint8_t band_hint = 0xFF; /* report all band/radio info */
	uint8_t band_map = 0;

	_Static_assert(sizeof(struct HAL_LLS_FULL_REPORT) >=
		       sizeof(struct STATS_LLS_WIFI_IFACE_STAT) +
		       sizeof(struct PEER_INFO_RATE_STAT[CFG_STA_REC_NUM]) +
		       sizeof(struct WIFI_RADIO_CHANNEL_STAT[ENUM_BAND_NUM]),
		       "HAL_LLS_FULL_REPORT too small");
	_Static_assert(sizeof(struct HAL_LLS_FULL_REPORT_V2) >=
		       sizeof(struct STATS_LLS_WIFI_IFACE_STAT) +
		       sizeof(struct PEER_INFO_RATE_STAT[CFG_STA_REC_NUM]) +
		       sizeof(struct WIFI_RADIO_CHANNEL_STAT[ENUM_BAND_NUM]),
		       "HAL_LLS_FULL_REPORT_V2 too small");

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;

#if (CFG_SUPPORT_CONNAC3X == 1)
	ucBssIdx = wlanGetBssIdx(wdev->netdev);
	if (ucBssIdx >= prAdapter->ucLinkStatsBssNum) {
		DBGLOG(REQ, ERROR, "Invalid BSS Index ucBssIdx=%u (%u)\n",
		       ucBssIdx, prAdapter->ucLinkStatsBssNum);
		return -EFAULT;
	}
#else
	ucBssIdx = 0; /* legacy FW only report 1 iface structure */
#endif

	if (!prAdapter->pucLinkStatsSrcBufAddr) {
		DBGLOG(REQ, ERROR, "EMI mapping not done");
		return -EFAULT;
	}

	dumpSourceBufferData(prAdapter, ucBssIdx);

	do {
		buf = (uint8_t *)&prAdapter->rLinkStatsDestBuffer;
		kalMemZero(buf, sizeof(prAdapter->rLinkStatsDestBuffer));

#if (CFG_SUPPORT_STATS_ONE_CMD == 0)
		u4QueryInfoLen = sizeof(query.cmd);
		query.cmd.u4Tag = STATS_LLS_TAG_LLS_DATA;
		rStatus = kalIoctl(prGlueInfo,
			   wlanQueryLinkStats, /* pfnOidHandler */
			   &query, /* pvInfoBuf */
			   u4QueryBufLen, /* u4InfoBufLen */
			   &u4QueryInfoLen); /* pu4QryInfoLen */

		if (rStatus != WLAN_STATUS_SUCCESS ||
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			u4QueryInfoLen !=
				sizeof(struct UNI_EVENT_LINK_LAYER_STATS) ||
			query.rtlv.u2Tag !=
				UNI_EVENT_STATISTICS_TAG_LINK_LAYER_STATS ||
			query.rtlv.u2Length !=
				sizeof(struct UNI_EVENT_LINK_LAYER_STATS) ||
			query.rtlv.data.eUpdateStatus !=
				STATS_LLS_UPDATE_STATUS_SUCCESS
#else
			u4QueryInfoLen !=
				sizeof(struct EVENT_STATS_LLS_DATA) ||
			query.data.eUpdateStatus !=
				STATS_LLS_UPDATE_STATUS_SUCCESS
#endif
			) {
			enum ENUM_STATS_LLS_UPDATE_STATUS eUpdateStatus;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			eUpdateStatus = query.rtlv.data.eUpdateStatus;
#else
			eUpdateStatus = query.data.eUpdateStatus;
#endif
			DBGLOG(REQ, WARN, "kalIoctl=%x, %u bytes, status=%u",
					rStatus, u4QueryInfoLen, eUpdateStatus);
			rStatus = -EFAULT;
			break;
		}

		if (data) {
			DBGLOG(REQ, WARN, "kalIoctl=%x, %u bytes",
					rStatus, u4QueryInfoLen);
			rStatus = -EFAULT;
			break;
		}
#else
		rParam.u4Period = prAdapter->rWifiVar.u4LlsStatsCmdPeriod;
		rStatus = kalIoctlByBssIdx(prGlueInfo,
			   wlanoidQueryStatsOneCmd, &rParam,
			   sizeof(rParam), &u4QueryInfoLen, ucBssIdx);
		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, WARN, "kalIoctl=%x",
				rStatus);
#endif

		DBGLOG(REQ, TRACE, "llstats_get_info(bss=%u)", ucBssIdx);
		/* Fill returning buffer from shared EMI address */
		ptr = buf;

#if AOSP_LLS_V1_SINGLE_INTERFACE /* leave legecy code here, need to verify */
		ptr += fill_iface(prAdapter, ptr, ucBssIdx);
#else /* multiple links, Android U goes here, support connac2/3 */
		ptr += fill_ml_iface(prAdapter, ptr, ucBssIdx);
#endif

		/* Set band_hint = 0x00 here to collect band/radio
		 * by Bss HW band index.
		 * However, before associated to specific AP,
		 * there will be no band associated to the queried BSS,
		 * which does not match what AOSP HAL expects.
		 */
		band_map = bandMaskByBssIdx(prAdapter, ucBssIdx);
		band_map |= band_hint;
		if (!band_map || prAdapter->rWifiVar.fgLinkStatsDump) {
			DBGLOG(REQ, DEBUG,
				"%s bss_idx=%u, hint=0x%02x, band_map=0x%02x\n",
				band_map ? "" : "No band associated",
				ucBssIdx, band_hint, band_map);
			if (band_map == 0) {
				rStatus = -ENOTCONN;
				break;
			}
		}

		ptr += fill_radio(prAdapter, ptr, ENUM_BAND_NUM, band_map);

		ptr += fill_power_levels(prAdapter, ptr, ENUM_BAND_NUM,
				band_map, prAdapter->u4TxTimePerLevelsSize);
		DBGLOG(REQ, TRACE, "Collected %td bytes for LLS", ptr - buf);

		skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, ptr - buf);
		if (!skb) {
			DBGLOG(REQ, WARN, "allocate skb %td bytes failed",
			       ptr - buf);
			rStatus = -ENOMEM;
			break;
		}

		if (unlikely(nla_put_nohdr(skb, ptr - buf, buf) < 0)) {
			rStatus = -EIO;
			break;
		}

		rStatus = cfg80211_vendor_cmd_reply(skb);
		return rStatus;
	} while (0);

	if (skb != NULL)
		kfree_skb(skb);
#endif
	return rStatus;
}

int mtk_cfg80211_vendor_set_wfd_tx_br_montr(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	int32_t rStatus = -EOPNOTSUPP;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct nlattr *attr[WIFI_ATTR_WFD_TX_BR_MONTR_MAX];
#if CFG_SUPPORT_LLS
	uint8_t uEnabled;
	union {
		struct CMD_GET_STATS_LLS cmd;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		struct UNI_EVENT_LINK_LAYER_STATS rtlv;
#else
		struct EVENT_STATS_LLS_DATA data;
#endif
	} query = {0};
	uint32_t u4QueryBufLen = sizeof(query);
	uint32_t u4QueryInfoLen = sizeof(query.cmd);
#endif

	DBGLOG(REQ, DEBUG, "%s data_len=%d\n", __func__, data_len);

	if ((wiphy == NULL) || (wdev == NULL))
		return -EINVAL;
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return -EFAULT;

	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	kalMemZero(attr, sizeof(struct nlattr *) *
			   (WIFI_ATTR_WFD_TX_BR_MONTR_MAX));
	if (NLA_PARSE_NESTED(attr,
			     WIFI_ATTR_WFD_TX_BR_MONTR_EN,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     mtk_wfd_tx_br_montr_policy) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		return -EINVAL;
	}

	if (!attr[WIFI_ATTR_WFD_TX_BR_MONTR_EN]) {
		DBGLOG(REQ, ERROR, "missing param enabled.");
		return -EINVAL;
	}

#if CFG_SUPPORT_LLS
	uEnabled = nla_get_u8(attr[WIFI_ATTR_WFD_TX_BR_MONTR_EN]);
	DBGLOG(REQ, DEBUG, "enabled:%u\n", uEnabled);
	if (unlikely(uEnabled > 1)) {
		DBGLOG(REQ, ERROR, "invalid param: enabled=%u", uEnabled);
		return -EINVAL;
	}

	query.cmd.u4Tag = STATS_LLS_TAG_SET_WFD_TX_BITRATE_MONTR;
	query.cmd.ucArg0 = uEnabled;
	rStatus = kalIoctl(prGlueInfo,
			wlanQueryLinkStats,
			&query,
			u4QueryBufLen,
			&u4QueryInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS ||
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		u4QueryInfoLen !=
			sizeof(struct UNI_EVENT_LINK_LAYER_STATS) ||
		query.rtlv.u2Tag !=
			UNI_EVENT_STATISTICS_TAG_SET_WFD_TX_BITRATE_MONTR
			||
		query.rtlv.u2Length !=
			sizeof(struct UNI_EVENT_LINK_LAYER_STATS) ||
		query.rtlv.data.eUpdateStatus !=
			STATS_LLS_UPDATE_STATUS_SUCCESS
#else
		u4QueryInfoLen !=
			sizeof(struct EVENT_STATS_LLS_DATA) ||
		query.data.eUpdateStatus !=
			STATS_LLS_UPDATE_STATUS_SUCCESS
#endif
		) {
		DBGLOG(REQ, WARN, "kalIoctl=%x, %u bytes, status=%u",
			rStatus, u4QueryBufLen,
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			query.rtlv.data.eUpdateStatus
#else
			query.data.eUpdateStatus
#endif
			);
		rStatus = -EFAULT;
	}
#endif
	return rStatus;
}

#if CFG_SUPPORT_LLS
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void forcePredBrIdx(
	struct ADAPTER *prAdapter,
	struct UNI_EVENT_BSS_PRED_TX_BR *src,
	struct EVENT_STATS_LLS_TX_BIT_RATE *dest,
	uint8_t *pucAisIdx
)
{
#define P2P_IDX 2
#define SAP_IDX 3
	struct BSS_INFO *prBssInfo;

	if (!prAdapter)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, src->ucBssIdx);

	if (!prBssInfo)
		return;

	if (IS_BSS_AIS(prBssInfo)) {
		dest->au4CurrentBitrate[*pucAisIdx] =
			src->rBitRate.u4CurrentBitrate;
		dest->au4PredictBitrate[*pucAisIdx] =
			src->rBitRate.u4PredictBitrate;
		(*pucAisIdx)++;
	} else if (IS_BSS_P2P(prBssInfo)) {
		if (IS_BSS_AP(prAdapter, prBssInfo)) {
			dest->au4CurrentBitrate[SAP_IDX] =
				src->rBitRate.u4CurrentBitrate;
			dest->au4PredictBitrate[SAP_IDX] =
				src->rBitRate.u4PredictBitrate;
		} else {
			/* gc/go */
			dest->au4CurrentBitrate[P2P_IDX] =
				src->rBitRate.u4CurrentBitrate;
			dest->au4PredictBitrate[P2P_IDX] =
				src->rBitRate.u4PredictBitrate;
		}
	}
}
#else
void forcePredBrIdx(
	struct ADAPTER *prAdapter,
	struct EVENT_STATS_LLS_TX_BIT_RATE *src,
	struct EVENT_STATS_LLS_TX_BIT_RATE *dest
)
{
#define P2P_IDX 2
#define SAP_IDX 3
	uint8_t i, aisIdx = 0;
	struct BSS_INFO *prBssInfo;

	for (i = 0; i < MAX_BSSID_NUM &&
			i < ARRAY_SIZE(src->au4CurrentBitrate); i++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
		if (!prBssInfo)
			continue;
		if (IS_BSS_AIS(prBssInfo)) {
			kalMemCopy(&dest->au4CurrentBitrate[aisIdx],
				&src->au4CurrentBitrate[i],
				sizeof(src->au4CurrentBitrate[i]));
			kalMemCopy(&dest->au4PredictBitrate[aisIdx],
				&src->au4PredictBitrate[i],
				sizeof(src->au4CurrentBitrate[i]));
			aisIdx++;
		} else if (IS_BSS_P2P(prBssInfo)) {
			if (IS_BSS_AP(prAdapter, prBssInfo)) {
				/* sap */
				kalMemCopy(&dest->au4CurrentBitrate[SAP_IDX],
					&src->au4CurrentBitrate[i],
					sizeof(src->au4CurrentBitrate[i]));
				kalMemCopy(&dest->au4PredictBitrate[SAP_IDX],
					&src->au4PredictBitrate[i],
					sizeof(src->au4CurrentBitrate[i]));
			} else {
				/* gc/go */
				kalMemCopy(&dest->au4CurrentBitrate[P2P_IDX],
					&src->au4CurrentBitrate[i],
					sizeof(src->au4CurrentBitrate[i]));
				kalMemCopy(&dest->au4PredictBitrate[P2P_IDX],
					&src->au4PredictBitrate[i],
					sizeof(src->au4CurrentBitrate[i]));
			}
		}
	}
}
#endif /* CFG_SUPPORT_UNIFIED_COMMAND */
#endif /* CFG_SUPPORT_LLS */

int mtk_cfg80211_vendor_get_wfd_pred_tx_br(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	int32_t rStatus = -EOPNOTSUPP;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
#if CFG_SUPPORT_LLS
	struct sk_buff *skb;

	union {
		struct CMD_GET_STATS_LLS cmd;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		struct UNI_EVENT_BSS_PRED_TX_BR arTlv[MAX_BSSID_NUM];
#else
		struct EVENT_STATS_LLS_TX_BIT_RATE bitrate;
#endif /* CFG_SUPPORT_UNIFIED_COMMAND */
	} query = {0};
	uint32_t u4QueryBufLen = sizeof(query);
	uint32_t u4QueryInfoLen = sizeof(query.cmd);
	struct EVENT_STATS_LLS_TX_BIT_RATE res = {0};
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint16_t offset = 0;
	uint8_t *tag = NULL;
	uint8_t ucAisIdx = 0;
#endif
#endif

	if ((wiphy == NULL) || (wdev == NULL))
		return -EINVAL;
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return -EFAULT;

#if CFG_SUPPORT_LLS
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	query.cmd.u4Tag = STATS_LLS_TAG_GET_WFD_BSS_PRED_TX_BITRATE;
#else
	query.cmd.u4Tag = STATS_LLS_TAG_GET_WFD_PRED_TX_BITRATE;
#endif
	rStatus = kalIoctl(prGlueInfo,
			wlanQueryLinkStats,
			&query,
			u4QueryBufLen,
			&u4QueryInfoLen);
	DBGLOG(REQ, TRACE, "kalIoctl=%x, %u bytes", rStatus, u4QueryInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS ||
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		u4QueryInfoLen > (sizeof(struct UNI_EVENT_BSS_PRED_TX_BR)
			* MAX_BSSID_NUM)
#else
		u4QueryInfoLen != sizeof(struct EVENT_STATS_LLS_TX_BIT_RATE)
#endif
		) {
		DBGLOG(REQ, WARN, "kalIoctl=%x, %u bytes",
				rStatus, u4QueryBufLen);
		return -EFAULT;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	tag = (uint8_t *) query.arTlv;
	TAG_FOR_EACH(tag, u4QueryInfoLen, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_STATISTICS_TAG_GET_BSS_PRED_TX_BITRATE: {
			uint8_t ucBssIdx;
			struct UNI_EVENT_BSS_PRED_TX_BR *tlv =
				(struct UNI_EVENT_BSS_PRED_TX_BR *)tag;

			ucBssIdx = tlv->ucBssIdx;
			if (unlikely(ucBssIdx >= prAdapter->ucSwBssIdNum))
				break;

			forcePredBrIdx(prAdapter, tlv, &res, &ucAisIdx);
			break;
		}
		default:
			DBGLOG(REQ, WARN, "invalid tag:%u", TAG_ID(tag));
			break;
		}
	}
#else
	forcePredBrIdx(prAdapter, &query.bitrate, &res);
#endif

	DBGLOG(REQ, TRACE, "CurBitRate=%u/%u/%u/%u PredBitRate=%u/%u/%u/%u",
		res.au4CurrentBitrate[0], res.au4CurrentBitrate[1],
		res.au4CurrentBitrate[2], res.au4CurrentBitrate[3],
		res.au4PredictBitrate[0], res.au4PredictBitrate[1],
		res.au4PredictBitrate[2], res.au4PredictBitrate[3]);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
			sizeof(struct EVENT_STATS_LLS_TX_BIT_RATE));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, WIFI_ATTR_WFD_CUR_TX_BR,
			     sizeof(res.au4CurrentBitrate),
			     res.au4CurrentBitrate)))
		goto nla_put_failure;
	if (unlikely(nla_put(skb, WIFI_ATTR_WFD_PRED_TX_BR,
			     sizeof(res.au4PredictBitrate),
			     res.au4PredictBitrate)))
		goto nla_put_failure;
	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;

#else
	return rStatus;
#endif
}

int mtk_cfg80211_vendor_set_tx_lat_montr_param(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	int32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct nlattr *attr[WIFI_ATTR_TX_LAT_MONTR_MAX];
	struct TX_LAT_MONTR_PARAM_STRUCT rParam;
	uint8_t i, uEnabled = 0, uIsAvg = 0;
	uint32_t u4Intvl = 0, u4DriverCrit = 0;
	uint32_t u4MacCrit = 0, u4BufLen = 0;

	DBGLOG(REQ, DEBUG, "%s data_len=%d\n", __func__, data_len);
	ASSERT(wiphy);
	ASSERT(wdev);
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return -EFAULT;

	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	kalMemZero(attr, sizeof(struct nlattr *) *
			   (WIFI_ATTR_TX_LAT_MONTR_MAX));
	if (NLA_PARSE_NESTED(attr,
			     WIFI_ATTR_TX_LAT_MONTR_IS_AVG,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     mtk_tx_lat_montr_param_policy) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		return -EINVAL;
	}

	for (i = WIFI_ATTR_TX_LAT_MONTR_EN;
	     i < WIFI_ATTR_TX_LAT_MONTR_MAX; i++) {
		if (attr[i]) {
			switch (i) {
			case WIFI_ATTR_TX_LAT_MONTR_EN:
				uEnabled = nla_get_u8(attr[i]);
				break;
			case WIFI_ATTR_TX_LAT_MONTR_INTVL:
				u4Intvl = nla_get_u32(attr[i]);
				break;
			case WIFI_ATTR_TX_LAT_MONTR_DRIVER_CRIT:
				u4DriverCrit = nla_get_u32(attr[i]);
				break;
			case WIFI_ATTR_TX_LAT_MONTR_MAC_CRIT:
				u4MacCrit = nla_get_u32(attr[i]);
				break;
			case WIFI_ATTR_TX_LAT_MONTR_IS_AVG:
				uIsAvg = nla_get_u8(attr[i]);
				break;
			}
		} else {
			return -EINVAL;
		}
	}
	DBGLOG(REQ, ERROR, "enabled=%u freq=%u driCri=%u macCri=%u isAvg=%u\n",
			uEnabled, u4Intvl, u4DriverCrit, u4MacCrit, uIsAvg);

	if (uEnabled != 0 && unlikely(uEnabled > 1 || uIsAvg > 1 ||
			u4Intvl < TX_LAT_MONTR_INTVL_MIN ||
			u4Intvl > TX_LAT_MONTR_INTVL_MAX ||
			u4DriverCrit < TX_LAT_MONTR_CRIT_MIN ||
			u4DriverCrit > TX_LAT_MONTR_CRIT_MAX ||
			u4MacCrit < TX_LAT_MONTR_CRIT_MIN ||
			u4MacCrit > TX_LAT_MONTR_CRIT_MAX)) {
		DBGLOG(REQ, ERROR, "invalid param\n");
		return -EINVAL;
	}
	kalMemZero(&rParam, sizeof(struct TX_LAT_MONTR_PARAM_STRUCT));
	rParam.fgEnabled = uEnabled ? TRUE : FALSE;
	rParam.u4Intvl = u4Intvl;
	rParam.u4DriverCrit = u4DriverCrit;
	rParam.u4MacCrit = u4MacCrit;
	rParam.fgIsAvg = uIsAvg ? TRUE : FALSE;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetTxLatMontrParam,
		&rParam, sizeof(struct TX_LAT_MONTR_PARAM_STRUCT),
		&u4BufLen);
	return rStatus;
}

int mtk_cfg80211_vendor_set_band(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct nlattr *attr;
	uint8_t setBand = 0;
	enum ENUM_BAND band;

	ASSERT(wiphy);
	ASSERT(wdev);

	TRACE_FUNC(REQ, DEBUG, "%s()\n");

	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	attr = (struct nlattr *)data;
	if (attr->nla_type == QCA_WLAN_VENDOR_ATTR_SETBAND_VALUE)
		setBand = nla_get_u32(attr);
	else
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	DBGLOG(REQ, DEBUG,
	       "vendor command: data_len=%d, data=0x%x 0x%x, set band value=%d\r\n",
	       data_len, *((uint32_t *) data), *((uint32_t *) data + 1),
	       setBand);

	if (setBand == QCA_SETBAND_5G)
		band = BAND_5G;
	else if (setBand == QCA_SETBAND_2G)
		band = BAND_2G4;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (setBand == QCA_SETBAND_6G)
		band = BAND_6G;
#endif
	else
		band = BAND_NULL;

	prGlueInfo->prAdapter->aePreferBand[NETWORK_TYPE_AIS] =
		band;
	return 0;

nla_put_failure:
	return -1;
}

#if CFG_SUPPORT_MBO
const struct nla_policy
qca_roaming_param_policy[QCA_ATTR_ROAMING_PARAM_MAX + 1] = {
	[QCA_ATTR_ROAMING_SUBCMD] = {.type = NLA_U32},
	[QCA_ATTR_ROAMING_REQ_ID] = {.type = NLA_U32},
	[QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID] = {.type = NLA_U32},
	[QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS] = {.type = NLA_NESTED},
	[QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID] = {
					.type = NLA_BINARY,
					.len = MAC_ADDR_LEN},
};

#define SET_BSSID_PARAMS_NUM_BSSID \
		QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID
#define SET_BSSID_PARAMS \
		QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS
#define SET_BSSID_PARAMS_BSSID \
		QCA_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID

int mtk_cfg80211_vendor_set_roaming_param(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct nlattr *tb[QCA_ATTR_ROAMING_PARAM_MAX + 1] = {};
	struct nlattr *tb2[QCA_ATTR_ROAMING_PARAM_MAX + 1] = {};
	struct nlattr *attr;
	uint32_t rStatus, u4BufLen, cmd_type, count, index;
	int tmp;
	uint8_t i = 0;

	ASSERT(wiphy);
	ASSERT(wdev);

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if ((data == NULL) || (data_len == 0))
		goto fail;

	if (NLA_PARSE(tb, QCA_ATTR_ROAMING_PARAM_MAX,
			data, data_len, qca_roaming_param_policy)) {
		DBGLOG(REQ, ERROR, "Wrong ROAM ATTR.\n");
		goto fail;
	}

	/* Parse and fetch Command Type*/
	if (!tb[QCA_ATTR_ROAMING_SUBCMD]) {
		DBGLOG(REQ, ERROR, "Invalid roam cmd type\n");
		goto fail;
	}

	cmd_type = nla_get_u32(tb[QCA_ATTR_ROAMING_SUBCMD]);
	if (cmd_type == QCA_ATTR_ROAM_SUBCMD_SET_BLOCKLIST_BSSID) {
		struct PARAM_BSS_DISALLOWED_LIST request = {};

		/* Parse and fetch number of blocklist BSSID */
		if (!tb[SET_BSSID_PARAMS_NUM_BSSID]) {
			DBGLOG(REQ, ERROR, "Invlaid num of blocklist bssid\n");
			goto fail;
		}
		count = nla_get_u32(tb[SET_BSSID_PARAMS_NUM_BSSID]);
		if (count > MAX_FW_ROAMING_BLOCKLIST_SIZE) {
			DBGLOG(REQ, ERROR, "Count %u exceeds\n", count);
			goto fail;
		}
		request.u4NumBssDisallowed = count;
		i = 0;
		if (count && tb[SET_BSSID_PARAMS]) {
			nla_for_each_nested(attr, tb[SET_BSSID_PARAMS], tmp) {
				char *bssid = NULL;

				if (i == count) {
					DBGLOG(REQ, ERROR, "Excess num\n");
					break;
				}
				if (NLA_PARSE(tb2,
					QCA_ATTR_ROAMING_PARAM_MAX,
					nla_data(attr), nla_len(attr),
					qca_roaming_param_policy)) {
					DBGLOG(REQ, ERROR, "Wrong ROAM ATTR\n");
					goto fail;
				}
				/* Parse and fetch MAC address */
				if (!tb2[SET_BSSID_PARAMS_BSSID]) {
					DBGLOG(REQ, ERROR, "addr failed\n");
					goto fail;
				}
				bssid = nla_data(tb2[SET_BSSID_PARAMS_BSSID]);
				index = i * MAC_ADDR_LEN;
				COPY_MAC_ADDR(&request.aucList[index], bssid);
				DBGLOG(REQ, DEBUG, "disallow #%d " MACSTR "\n",
					i, MAC2STR(bssid));
				i++;
			}
		}
		if (i != count)
			DBGLOG(REQ, ERROR, "Count %u, expected %u\n", i, count);

		rStatus = kalIoctl(prGlueInfo, wlanoidBssDisallowedList,
				   &request,
				   sizeof(struct PARAM_BSS_DISALLOWED_LIST),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "disallowed error:%x\n", rStatus);
			return -EFAULT;
		}
	} else {
		DBGLOG(REQ, DEBUG, "unhandled cmd_type %d\n", cmd_type);
		goto fail;
	}

	return WLAN_STATUS_SUCCESS;
fail:
	return -EINVAL;
}

#undef SET_BSSID_PARAMS_NUM_BSSID
#undef SET_BSSID_PARAMS
#undef SET_BSSID_PARAMS_BSSID

#endif

#if CFG_SUPPORT_ROAMING
int mtk_cfg80211_vendor_set_roaming_policy(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct nlattr *attr;
	uint32_t setRoaming = 0;
	uint32_t u4BufLen = 0;
	int32_t i4Status = -EINVAL;
	uint8_t ucBssIndex = 0;

	ASSERT(wiphy);
	ASSERT(wdev);

	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	attr = (struct nlattr *)data;
	if (attr->nla_type == QCA_WLAN_VENDOR_ATTR_ROAMING_POLICY)
		setRoaming = nla_get_u32(attr);
	else
		return -EINVAL;
	WIPHY_PRIV(wiphy, prGlueInfo);
	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	ASSERT(prGlueInfo);

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	DBGLOG(REQ, DEBUG,
	       "vendor command: data_len=%d, data=0x%x 0x%x, roaming policy=%d\r\n",
	       data_len, *((uint32_t *) data), *((uint32_t *) data + 1),
	       setRoaming);
	rStatus = kalIoctlByBssIdx(prGlueInfo,
			   wlanoidSetDrvRoamingPolicy,
			   &setRoaming, sizeof(uint32_t),
			   &u4BufLen,
			   ucBssIndex);

	return rStatus;

nla_put_failure:
	return i4Status;
}
#endif

int mtk_cfg80211_vendor_set_rssi_monitoring(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	int32_t i4Status = -EINVAL;
	struct PARAM_RSSI_MONITOR_T rRSSIMonitor;
	struct nlattr *attr[WIFI_ATTRIBUTE_RSSI_MONITOR_START + 1];
	uint32_t i = 0;

	ASSERT(wiphy);
	ASSERT(wdev);

	DBGLOG(REQ, TRACE, "vendor command: data_len=%d\r\n",
	       data_len);
	kalMemZero(&rRSSIMonitor, sizeof(struct PARAM_RSSI_MONITOR_T));
	if ((data == NULL) || !data_len)
		goto nla_put_failure;
	kalMemZero(attr, sizeof(struct nlattr *) *
		   (WIFI_ATTRIBUTE_RSSI_MONITOR_START + 1));
	if (NLA_PARSE_NESTED(attr,
			     WIFI_ATTRIBUTE_RSSI_MONITOR_START,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     nla_parse_wifi_rssi_monitor) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		goto nla_put_failure;
	}

	for (i = WIFI_ATTRIBUTE_RSSI_MONITOR_INVALID + 1;
	     i < WIFI_ATTRIBUTE_RSSI_MONITOR_ATTRIBUTE_MAX; i++) {
		if (attr[i]) {
			switch (i) {
			case WIFI_ATTRIBUTE_RSSI_MONITOR_MAX_RSSI:
				rRSSIMonitor.max_rssi_value =
					nla_get_u32(attr[i]);
				break;
			case WIFI_ATTRIBUTE_RSSI_MONITOR_MIN_RSSI:
				rRSSIMonitor.min_rssi_value
					= nla_get_u32(attr[i]);
				break;
			case WIFI_ATTRIBUTE_RSSI_MONITOR_START:
				rRSSIMonitor.enable = nla_get_u32(attr[i]);
				break;
			}
		}
	}

	DBGLOG(REQ, TRACE,
	       "mMax_rssi=%d, mMin_rssi=%d enable=%d\r\n",
	       rRSSIMonitor.max_rssi_value, rRSSIMonitor.min_rssi_value,
	       rRSSIMonitor.enable);

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidRssiMonitor,
			   &rRSSIMonitor, sizeof(struct PARAM_RSSI_MONITOR_T),
			   &u4BufLen);
	return rStatus;

nla_put_failure:
	return i4Status;
}

int mtk_cfg80211_vendor_packet_keep_alive_start(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_PACKET_KEEPALIVE_T *prPkt = NULL;
	struct nlattr *attr[MKEEP_ALIVE_ATTRIBUTE_MAX];
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint16_t u2IpPktLen = 0;
	uint32_t u4BufLen = 0;
	uint8_t ucBssIndex = 0, ucIdx = 0;
	int32_t i4Status = -EINVAL;

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	DBGLOG(REQ, TRACE, "vendor command: data_len=%d\r\n", data_len);
	prPkt = (struct PARAM_PACKET_KEEPALIVE_T *) kalMemAlloc(
		sizeof(struct PARAM_PACKET_KEEPALIVE_T), VIR_MEM_TYPE);
	if (!prPkt) {
		DBGLOG(REQ, ERROR,
		       "Can not alloc memory for struct PARAM_PACKET_KEEPALIVE_T\n");
		return -ENOMEM;
	}
	kalMemZero(prPkt, sizeof(struct PARAM_PACKET_KEEPALIVE_T));
	kalMemZero(attr, sizeof(struct nlattr *) * (MKEEP_ALIVE_ATTRIBUTE_MAX));

	prPkt->enable = TRUE; /*start packet keep alive*/
	prPkt->reserved[0] = ucBssIndex;
	if (NLA_PARSE_NESTED(attr,
			     MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     nla_parse_offloading_policy) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		i4Status = -EFAULT;
		goto nla_put_failure;
	}

	for (ucIdx = MKEEP_ALIVE_ATTRIBUTE_ID;
	     ucIdx <= MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC; ucIdx++) {
		if (attr[ucIdx]) {
			switch (ucIdx) {
			case MKEEP_ALIVE_ATTRIBUTE_ID:
				prPkt->index = nla_get_u8(attr[ucIdx]);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
				prPkt->u2IpPktLen = nla_get_u16(attr[ucIdx]);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
				u2IpPktLen = prPkt->u2IpPktLen <= 256
					? prPkt->u2IpPktLen : 256;
				kalMemCopy(prPkt->pIpPkt, nla_data(attr[ucIdx]),
					u2IpPktLen);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
				kalMemCopy(prPkt->ucSrcMacAddr,
				   nla_data(attr[ucIdx]), sizeof(uint8_t) * 6);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
				kalMemCopy(prPkt->ucDstMacAddr,
				   nla_data(attr[ucIdx]), sizeof(uint8_t) * 6);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
				prPkt->u4PeriodMsec = nla_get_u32(attr[ucIdx]);
				break;
			}
		}
	}

	DBGLOG(REQ, DEBUG,
		"BssIdx=%d enable=%d, index=%d, u2IpPktLen=%d u4PeriodMsec=%d\n",
		prPkt->reserved[0], prPkt->enable, prPkt->index,
		prPkt->u2IpPktLen, prPkt->u4PeriodMsec);
	DBGLOG(REQ, TRACE, "prPkt->pIpPkt=0x%02x%02x%02x%02x\n",
		prPkt->pIpPkt[0], prPkt->pIpPkt[1],
		prPkt->pIpPkt[2], prPkt->pIpPkt[3]);
	DBGLOG(REQ, TRACE, "%02x%02x%02x%02x, %02x%02x%02x%02x\n",
		prPkt->pIpPkt[4], prPkt->pIpPkt[5],
		prPkt->pIpPkt[6], prPkt->pIpPkt[7],
		prPkt->pIpPkt[8], prPkt->pIpPkt[9],
		prPkt->pIpPkt[10], prPkt->pIpPkt[11]);
	DBGLOG(REQ, TRACE, "%02x%02x%02x%02x\n",
		prPkt->pIpPkt[12], prPkt->pIpPkt[13],
		prPkt->pIpPkt[14], prPkt->pIpPkt[15]);
	DBGLOG(REQ, TRACE,
		"prPkt->srcMAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
		prPkt->ucSrcMacAddr[0], prPkt->ucSrcMacAddr[1],
		prPkt->ucSrcMacAddr[2], prPkt->ucSrcMacAddr[3],
		prPkt->ucSrcMacAddr[4], prPkt->ucSrcMacAddr[5]);
	DBGLOG(REQ, TRACE, "dstMAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
		prPkt->ucDstMacAddr[0], prPkt->ucDstMacAddr[1],
		prPkt->ucDstMacAddr[2], prPkt->ucDstMacAddr[3],
		prPkt->ucDstMacAddr[4], prPkt->ucDstMacAddr[5]);
	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		i4Status = -EFAULT;
		goto nla_put_failure;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidPacketKeepAlive,
			   prPkt, sizeof(struct PARAM_PACKET_KEEPALIVE_T),
			   &u4BufLen);
	kalMemFree(prPkt, VIR_MEM_TYPE,
		   sizeof(struct PARAM_PACKET_KEEPALIVE_T));
	return rStatus;

nla_put_failure:
	if (prPkt != NULL)
		kalMemFree(prPkt, VIR_MEM_TYPE,
			   sizeof(struct PARAM_PACKET_KEEPALIVE_T));
	return i4Status;
}

int mtk_cfg80211_vendor_packet_keep_alive_stop(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	int32_t i4Status = -EINVAL;
	struct PARAM_PACKET_KEEPALIVE_T *prPkt = NULL;
	struct nlattr *attr;

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(REQ, TRACE, "vendor command: data_len=%d\r\n",
	       data_len);
	prPkt = (struct PARAM_PACKET_KEEPALIVE_T *)
		kalMemAlloc(sizeof(struct PARAM_PACKET_KEEPALIVE_T),
			    VIR_MEM_TYPE);
	if (!prPkt) {
		DBGLOG(REQ, ERROR,
		       "Can not alloc memory for PARAM_PACKET_KEEPALIVE_T\n");
		return -ENOMEM;
	}
	kalMemZero(prPkt, sizeof(struct PARAM_PACKET_KEEPALIVE_T));

	prPkt->enable = FALSE;  /*stop packet keep alive*/
	attr = (struct nlattr *)data;
	if (attr->nla_type == MKEEP_ALIVE_ATTRIBUTE_ID)
		prPkt->index = nla_get_u8(attr);

	DBGLOG(REQ, DEBUG, "enable=%d, index=%d\r\n",
	       prPkt->enable, prPkt->index);

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		kalMemFree(prPkt, VIR_MEM_TYPE,
		   sizeof(struct PARAM_PACKET_KEEPALIVE_T));
		return -EFAULT;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidPacketKeepAlive,
			   prPkt, sizeof(struct PARAM_PACKET_KEEPALIVE_T),
			   &u4BufLen);
	kalMemFree(prPkt, VIR_MEM_TYPE,
		   sizeof(struct PARAM_PACKET_KEEPALIVE_T));
	return rStatus;

nla_put_failure:
	if (prPkt != NULL)
		kalMemFree(prPkt, VIR_MEM_TYPE,
			   sizeof(struct PARAM_PACKET_KEEPALIVE_T));
	return i4Status;
}

int mtk_cfg80211_vendor_get_version(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct sk_buff *skb = NULL;
	struct nlattr *attrlist = NULL;
	char aucVersionBuf[256];
	uint16_t u2CopySize = 0;
	uint16_t u2Len = 0;

	ASSERT(wiphy);
	ASSERT(wdev);

	if ((data == NULL) || !data_len)
		return -ENOMEM;

	kalMemZero(aucVersionBuf, 256);
	attrlist = (struct nlattr *)((uint8_t *) data);
	if (attrlist->nla_type == LOGGER_ATTRIBUTE_DRIVER_VER) {
		char aucDriverVersionStr[] = STR(NIC_DRIVER_MAJOR_VERSION) "_"
					     STR(NIC_DRIVER_MINOR_VERSION) "_"
					     STR(NIC_DRIVER_SERIAL_VERSION);

		u2Len = kalStrLen(aucDriverVersionStr);
		DBGLOG(REQ, TRACE, "Get driver version len: %d\n", u2Len);
		u2CopySize = (u2Len >= 256) ? 255 : u2Len;
		if (u2CopySize > 0)
			kalMemCopy(aucVersionBuf, &aucDriverVersionStr[0],
				u2CopySize);
	} else if (attrlist->nla_type == LOGGER_ATTRIBUTE_FW_VER) {
		struct ADAPTER *prAdapter;

		WIPHY_PRIV(wiphy, prGlueInfo);
		ASSERT(prGlueInfo);
		if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON
			| WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
			DBGLOG(REQ, WARN, "driver is not ready\n");
			return -EFAULT;
		}

		prAdapter = prGlueInfo->prAdapter;
		if (prAdapter) {
			u2Len = kalStrLen(
					prAdapter->rVerInfo.aucReleaseManifest);
			DBGLOG(REQ, TRACE,
				"Get FW manifest version len: %d\n", u2Len);
			u2CopySize = (u2Len >= 256) ? 255 : u2Len;
			if (u2CopySize > 0)
				kalMemCopy(aucVersionBuf,
					prAdapter->rVerInfo.aucReleaseManifest,
					u2CopySize);
		}
	}

	DBGLOG(REQ, DEBUG, "Get version(%d)=[%s]\n", u2CopySize, aucVersionBuf);

	if (u2CopySize == 0)
		return -EFAULT;

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, u2CopySize);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_nohdr(skb, u2CopySize, &aucVersionBuf[0]) < 0))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;
}

int mtk_cfg80211_vendor_event_generic_response(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	uint32_t len, uint8_t *data)
{
	struct sk_buff *skb;

	if (!wiphy || !wdev || !data || len <= 0) {
		DBGLOG(REQ, ERROR, "%s wrong input parameters\n", __func__);
		return -EINVAL;
	}

	skb = cfg80211_vendor_event_alloc(wiphy,
#if KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE
			wdev,
#endif
			len, WIFI_EVENT_GENERIC_RESPONSE, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	/* Do not use nla_put_nohdr because it aligns buffer
	 *
	 * if (unlikely(nla_put_nohdr(skb, len, data) < 0))
	 *	goto nla_put_failure;
	 */
	kalMemCopy(skb_put(skb, len), data, len);

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return 0;
}

int mtk_cfg80211_vendor_get_supported_feature_set(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	uint64_t u8FeatureSet;
	struct GLUE_INFO *prGlueInfo;
	struct sk_buff *skb;

	ASSERT(wiphy);
	ASSERT(wdev);

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	u8FeatureSet = wlanGetSupportedFeatureSet(prGlueInfo);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(u8FeatureSet));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(
	    nla_put_nohdr(skb, sizeof(u8FeatureSet), &u8FeatureSet) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		goto nla_put_failure;
	}

	DBGLOG(REQ, TRACE, "supported feature set=0x%llx\n", u8FeatureSet);

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;
}

int mtk_cfg80211_vendor_event_rssi_beyond_range(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIdx, int rssi)
{
	struct wiphy *wiphy = GLUE_GET_WIPHY(prGlueInfo);
	struct net_device *dev = wlanGetNetDev(prGlueInfo,
			ucBssIdx);
	struct sk_buff *skb;
	struct PARAM_RSSI_MONITOR_EVENT rRSSIEvt;
	struct BSS_INFO *prAisBssInfo;

	ASSERT(wiphy);

	if (dev == NULL || dev->ieee80211_ptr == NULL)
		return -EINVAL;

	DBGLOG(REQ, TRACE, "vendor command rssi=%d\r\n", rssi);
	kalMemZero(&rRSSIEvt,
		   sizeof(struct PARAM_RSSI_MONITOR_EVENT));

#if KERNEL_VERSION(4, 4, 0) <= LINUX_VERSION_CODE
	skb = cfg80211_vendor_event_alloc(wiphy, dev->ieee80211_ptr,
				  sizeof(struct PARAM_RSSI_MONITOR_EVENT),
				  WIFI_EVENT_RSSI_MONITOR, GFP_KERNEL);
#else
	skb = cfg80211_vendor_event_alloc(wiphy,
				  sizeof(struct PARAM_RSSI_MONITOR_EVENT),
				  WIFI_EVENT_RSSI_MONITOR, GFP_KERNEL);
#endif /* KERNEL_VERSION(4, 4, 0) <= LINUX_VERSION_CODE */

	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	prAisBssInfo = aisGetDefaultLinkBssInfo(prGlueInfo->prAdapter);

	kalMemCopy(rRSSIEvt.BSSID, prAisBssInfo->aucBSSID,
		   sizeof(uint8_t) * MAC_ADDR_LEN);

	rRSSIEvt.version = 1; /* RSSI_MONITOR_EVT_VERSION = 1 */
	if (rssi > PARAM_WHQL_RSSI_MAX_DBM)
		rssi = PARAM_WHQL_RSSI_MAX_DBM;
	else if (rssi < -120)
		rssi = -120;
	rRSSIEvt.rssi = (int8_t)rssi;
	DBGLOG(REQ, DEBUG,
	       "RSSI Event: version=%d, rssi=%d, BSSID=" MACSTR "\r\n",
	       rRSSIEvt.version, rRSSIEvt.rssi, MAC2STR(rRSSIEvt.BSSID));

	/*NLA_PUT_U32(skb, GOOGLE_RSSI_MONITOR_EVENT, rssi);*/
	{
		/* unsigned int __tmp = rssi; */

		if (unlikely(nla_put(skb, WIFI_EVENT_RSSI_MONITOR,
				     sizeof(struct PARAM_RSSI_MONITOR_EVENT),
				     &rRSSIEvt) < 0))
			goto nla_put_failure;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return 0;

nla_put_failure:
	kfree_skb(skb);
	return -ENOMEM;
}

int mtk_cfg80211_vendor_set_tx_power_scenario(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	struct PARAM_TX_PWR_CTRL_IOCTL rPwrCtrlParam = { 0 };
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *attr;
	struct sk_buff *skb;
	uint32_t u4Scenario;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SetInfoLen = 0;
	uint8_t index = 0;
	char name[] = { "_G_Scenario" };

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	attr = (struct nlattr *)data;
	if (attr->nla_type == WIFI_ATTRIBUTE_TX_POWER_SCENARIO)
		u4Scenario = nla_get_u32(attr);
	else
		return -EINVAL;

	if (u4Scenario == UINT_MAX) {
		index = 0;
	} else if (u4Scenario <= 4) {
		index = u4Scenario + 1;
	} else {
		DBGLOG(REQ, ERROR, "invalid scenario index: %u\n", u4Scenario);
		return -EINVAL;
	}

	rPwrCtrlParam.fgApplied = (index == 0) ? FALSE : TRUE;
	rPwrCtrlParam.name = name;
	rPwrCtrlParam.index = index;

	DBGLOG(REQ, DEBUG,
	       "applied=[%d], name=[%s], index=[%u], setting=[%s], UINT_MAX=[%u], iftype=[%d]\n",
	       rPwrCtrlParam.fgApplied,
	       rPwrCtrlParam.name,
	       rPwrCtrlParam.index,
	       rPwrCtrlParam.newSetting,
	       UINT_MAX,
	       wdev->iftype);

	rStatus = kalIoctl(prGlueInfo, wlanoidTxPowerControl,
		 (void *)&rPwrCtrlParam, sizeof(struct PARAM_TX_PWR_CTRL_IOCTL),
		 &u4SetInfoLen);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(rStatus));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(
	    nla_put_nohdr(skb, sizeof(rStatus), &rStatus) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		goto errHandleLabel;
	}

	DBGLOG(REQ, DEBUG, "rStatus=0x%x\n", rStatus);

	return cfg80211_vendor_cmd_reply(skb);

errHandleLabel:
	kfree_skb(skb);
#endif
	return -EOPNOTSUPP;
}

int mtk_cfg80211_vendor_set_mlo_mode(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
#if CFG_SUPPORT_MLC
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	struct nlattr *attr;
	struct sk_buff *skb;
	uint32_t u4Mode;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SetInfoLen = 0;
	uint8_t ucBssIdx = 0;
	union PARAM_MLC rMlcParam = {0};

	if (!wiphy || !wdev || !data || !data_len)
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return -EFAULT;

	if (prAdapter->rWifiVar.ucDisableFwkMlc)
		return -EOPNOTSUPP;

	ucBssIdx = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIdx))
		return -EINVAL;

	attr = (struct nlattr *)data;
	if (attr->nla_type == MTK_MLO_MODE_ATTRIBUTE_MLO_MODE)
		u4Mode = nla_get_u32(attr);
	else {
		DBGLOG(REQ, ERROR, "Unknown nla type: %u\n", attr->nla_type);
		return -EINVAL;
	}

	if (u4Mode < PARAM_MLO_MODE_MAX) {
		switch (u4Mode) {
		case PARAM_MLO_MODE_DEFAULT:
			rMlcParam.rReq.eMlcMode = MLC_MODE_DEFAULT;
			break;
		case PARAM_MLO_MODE_LOW_LATENCY:
			rMlcParam.rReq.eMlcMode = MLC_MODE_LOW_LATENCY;
			break;
		case PARAM_MLO_MODE_HIGH_THROUGHPUT:
			rMlcParam.rReq.eMlcMode = MLC_MODE_HIGH_TPUT;
			break;
		case PARAM_MLO_MODE_LOW_POWER:
			rMlcParam.rReq.eMlcMode = MLC_MODE_LOW_POWER;
			break;
		default:
			return -EINVAL;
		}
	} else {
		DBGLOG(REQ, ERROR, "invalid mlo mode: %u\n", u4Mode);
		return -EINVAL;
	}

	DBGLOG(REQ, DEBUG, "mlcReq=%d, data1=0x%x data2=0x%x\n",
		rMlcParam.rReq.eMlcMode, rMlcParam.rReq.u4Data1,
		rMlcParam.rReq.u4Data2);

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetMlcMode,
		 (void *)&rMlcParam, sizeof(struct PARAM_MLC_REQ),
		 &u4SetInfoLen, ucBssIdx);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(rStatus));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(
	    nla_put_nohdr(skb, sizeof(rStatus), &rStatus) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		goto errHandleLabel;
	}

	DBGLOG(REQ, DEBUG, "rStatus=0x%x\n", rStatus);

	return cfg80211_vendor_cmd_reply(skb);

errHandleLabel:
	kfree_skb(skb);
#endif /* CFG_SUPPORT_MLC */
	return -EOPNOTSUPP;
}


int mtk_cfg80211_vendor_set_multista_primary_connection(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *prAttr;
	uint32_t u4InterfaceIdx, u4BufLen = 0;
	uint32_t u4AisIndex = AIS_DEFAULT_INDEX;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct wireless_dev **pprWdev = NULL;

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAttr = (struct nlattr *)data;
	if (prAttr->nla_type == MULTISTA_ATTRIBUTE_PRIMARY_IFACE)
		u4InterfaceIdx = nla_get_u32(prAttr);
	else {
		DBGLOG(REQ, DEBUG, "Unknown nla type:%d\n", prAttr->nla_type);
		return -EINVAL;
	}

	DBGLOG(REQ, DEBUG, "primary interface index=%d\n", u4InterfaceIdx);
	pprWdev = wlanGetWirelessDevice(prGlueInfo);

	if (!pprWdev) {
		DBGLOG(REQ, ERROR, "pprWdev is NULL\n");
		return -EINVAL;
	}

	if (pprWdev[AIS_DEFAULT_INDEX] && u4InterfaceIdx ==
		pprWdev[AIS_DEFAULT_INDEX]->netdev->ifindex)
		u4AisIndex = AIS_DEFAULT_INDEX;
#if KAL_AIS_NUM > 1
	else if (pprWdev[AIS_SECONDARY_INDEX] && u4InterfaceIdx ==
		pprWdev[AIS_SECONDARY_INDEX]->netdev->ifindex)
		u4AisIndex = AIS_SECONDARY_INDEX;
#endif
	else {
		DBGLOG(REQ, DEBUG, "No match with pprWdev\n");
		return -EINVAL;
	}

	u4Status = kalIoctl(prGlueInfo, wlanoidSetMultiStaPrimaryInterface,
			&u4AisIndex, sizeof(uint32_t), &u4BufLen);

	return u4Status;

}

int mtk_cfg80211_vendor_set_multista_use_case(
		struct wiphy *wiphy, struct wireless_dev *wdev,
		const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *prAttr;
	uint32_t u4UseCase;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || (data_len == 0))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAttr = (struct nlattr *)data;
	if (prAttr->nla_type == MULTISTA_ATTRIBUTE_USE_CASE)
		u4UseCase = nla_get_u32(prAttr);
	else {
		DBGLOG(REQ, DEBUG, "Unknown nla type:%d\n", prAttr->nla_type);
		return -EINVAL;
	}

	DBGLOG(REQ, DEBUG, "Multiple station use case=%d\n", u4UseCase);

#if 0
	u4Status = kalIoctl(prGlueInfo, wlanoidSetMultiStaUseCase,
		&u4UseCase, sizeof(uint32_t), &u4BufLen);
#endif

	return u4Status;
}

#if CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST

int mtk_cfg80211_vendor_get_preferred_freq_list(struct wiphy
		*wiphy, struct wireless_dev *wdev, const void *data,
		int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct sk_buff *skb;
	struct nlattr *tb[WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_LAST] = {};
	uint32_t freq_list[MAX_CHN_NUM] = {};
	uint32_t num_freq_list = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	enum CONN_MODE_IFACE_TYPE type;
	enum ENUM_IFTYPE eIftype;
	uint32_t i;
	uint32_t au4FreqAllowList[MAX_CHN_NUM] = { 0 };
	uint8_t ucAllowFreqNum;
	struct WIFI_VAR *prWifiVar;

	ASSERT(wiphy);
	ASSERT(wdev);

	if ((data == NULL) || !data_len)
		return -EINVAL;

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (NLA_PARSE(tb, WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_MAX,
			data, data_len, nla_get_preferred_freq_list_policy)) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		return -EINVAL;
	}

	if (!tb[WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_IFACE_TYPE]) {
		DBGLOG(REQ, ERROR, "Invalid type.\n");
		return -EINVAL;
	}
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	type = nla_get_u32(tb[WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_IFACE_TYPE]);

	DBGLOG(REQ, DEBUG, "type: %d\n", type);

	switch (type) {
	case CONN_MODE_IFACE_TYPE_STA:
		eIftype = IFTYPE_STATION;
		break;
	case CONN_MODE_IFACE_TYPE_SAP:
		eIftype = IFTYPE_AP;
		break;
	case CONN_MODE_IFACE_TYPE_P2P_GC:
		eIftype = IFTYPE_P2P_CLIENT;
		break;
	case CONN_MODE_IFACE_TYPE_P2P_GO:
		eIftype = IFTYPE_P2P_GO;
		break;
	default:
		eIftype = IFTYPE_NUM;
		break;
	}

	if (eIftype != IFTYPE_P2P_CLIENT && eIftype != IFTYPE_P2P_GO) {
		DBGLOG(REQ, ERROR, "Only support p2p gc/go type.\n");
		return -EINVAL;
	}

	ucAllowFreqNum = p2pFuncGetFreqAllowList(prGlueInfo->prAdapter,
					       au4FreqAllowList);
	rStatus = p2pFunGetPreferredFreqList(prGlueInfo->prAdapter, eIftype,
			freq_list, &num_freq_list, au4FreqAllowList,
			ucAllowFreqNum, prWifiVar->fgP2pPrefSkipDfs);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "get preferred freq list failed.\n");
		return -EINVAL;
	}

	DBGLOG(P2P, TRACE, "num. of preferred freq list = %d\n", num_freq_list);
	for (i = 0; i < num_freq_list; i++) {
		DBGLOG(P2P, TRACE, "dump preferred freq list[%u]: %u, ch: %u\n",
			i, freq_list[i],
			nicFreq2ChannelNum(freq_list[i] * 1000));
	}

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(u32) +
			sizeof(uint32_t) * num_freq_list + NLMSG_HDRLEN);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed.\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb,
			WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_IFACE_TYPE,
			type) < 0)) {
		DBGLOG(REQ, ERROR, "put iface into skb failed.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put(skb, WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_GET,
			sizeof(uint32_t) * num_freq_list, freq_list) < 0)) {
		DBGLOG(REQ, ERROR, "put freq list into skb failed.\n");
		goto nla_put_failure;
	}

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);
	return -EFAULT;
}
#endif

#if CFG_AUTO_CHANNEL_SEL_SUPPORT

int mtk_cfg80211_vendor_acs(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *tb[WIFI_VENDOR_ATTR_ACS_MAX + 1] = {};
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	bool ht_enabled, ht40_enabled, vht_enabled, eht_enabled = false;
	uint16_t ch_width = 0;
	enum P2P_VENDOR_ACS_HW_MODE hw_mode;
	uint32_t *freq_list = NULL;
	uint8_t ch_list_count = 0;
	uint8_t i;
	uint32_t msg_size;
	struct MSG_P2P_ACS_REQUEST *prMsgAcsRequest;
	struct RF_CHANNEL_INFO *prRfChannelInfo;
	struct sk_buff *reply_skb;
	uint8_t role_idx;
	int8_t link_id = -1;
	struct PARAM_GET_CHN_INFO *prLteSafeChn = NULL;
#if CFG_SUPPORT_GET_LTE_SAFE_CHANNEL
	uint32_t u4BufLen;
#endif

	if (!wiphy || !wdev || !data || !data_len) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (mtk_Netdev_To_RoleIdx(prGlueInfo, wdev->netdev, &role_idx) < 0) {
		DBGLOG(REQ, ERROR, "get role index fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (NLA_PARSE(tb, WIFI_VENDOR_ATTR_ACS_MAX, data, data_len,
			nla_get_acs_policy)) {
		DBGLOG(REQ, ERROR, "parse acs attr fail.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	if (!tb[WIFI_VENDOR_ATTR_ACS_HW_MODE]) {
		DBGLOG(REQ, ERROR, "attr hw_mode failed.\n");
		rStatus = -EINVAL;
		goto exit;
	}
	hw_mode = nla_get_u8(tb[WIFI_VENDOR_ATTR_ACS_HW_MODE]);

	if (tb[WIFI_VENDOR_ATTR_ACS_HT_ENABLED])
		ht_enabled =
			nla_get_flag(tb[WIFI_VENDOR_ATTR_ACS_HT_ENABLED]);
	else
		ht_enabled = 0;

	if (tb[WIFI_VENDOR_ATTR_ACS_HT40_ENABLED])
		ht40_enabled =
			nla_get_flag(tb[WIFI_VENDOR_ATTR_ACS_HT40_ENABLED]);
	else
		ht40_enabled = 0;

	if (tb[WIFI_VENDOR_ATTR_ACS_VHT_ENABLED])
		vht_enabled =
			nla_get_flag(tb[WIFI_VENDOR_ATTR_ACS_VHT_ENABLED]);
	else
		vht_enabled = 0;

#if (CFG_SUPPORT_802_11BE == 1)
	if (tb[WIFI_VENDOR_ATTR_ACS_EHT_ENABLED])
		eht_enabled =
			nla_get_flag(tb[WIFI_VENDOR_ATTR_ACS_EHT_ENABLED]);
	else
		eht_enabled = 0;
#endif

	if (tb[WIFI_VENDOR_ATTR_ACS_CHWIDTH])
		ch_width = nla_get_u16(tb[WIFI_VENDOR_ATTR_ACS_CHWIDTH]);

	if (tb[WIFI_VENDOR_ATTR_ACS_FREQ_LIST]) {
		uint32_t *freq =
			nla_data(tb[WIFI_VENDOR_ATTR_ACS_FREQ_LIST]);

		ch_list_count = nla_len(tb[WIFI_VENDOR_ATTR_ACS_FREQ_LIST]) /
				sizeof(uint32_t);
		if (ch_list_count) {
			if (ch_list_count > MAX_CHN_NUM) {
				DBGLOG(REQ, ERROR, "Invalid freq count.\n");
				rStatus = -EINVAL;
				goto exit;
			}
			freq_list = kalMemAlloc(
				sizeof(uint32_t) * ch_list_count,
				VIR_MEM_TYPE);
			if (freq_list == NULL) {
				DBGLOG(REQ, ERROR, "allocate freq_list fail.");
				rStatus = -ENOMEM;
				goto exit;
			}

			for (i = 0; i < ch_list_count; i++)
				freq_list[i] = freq[i];
		}
	}

	if (tb[WIFI_VENDOR_ATTR_ACS_LINK_ID]) {
		uint8_t ucRoleIdx;

		link_id = nla_get_u8(tb[WIFI_VENDOR_ATTR_ACS_LINK_ID]);
		if (__mtk_Netdev_To_RoleIdx(prGlueInfo, wdev->netdev, link_id,
					    &ucRoleIdx) == 0) {
			role_idx = ucRoleIdx;
		} else {
			DBGLOG(REQ, ERROR,
				"__mtk_Netdev_To_RoleIdx failed, link_id=%d\n",
				link_id);
			rStatus = -ENOMEM;
			goto exit;
		}
	}

	if (!ch_list_count) {
		DBGLOG(REQ, ERROR, "channel list count can NOT be 0\n");
		rStatus = -EINVAL;
		goto exit;
	}

	msg_size = sizeof(struct MSG_P2P_ACS_REQUEST) +
			(ch_list_count * sizeof(struct RF_CHANNEL_INFO));

	prMsgAcsRequest = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, msg_size);

	if (prMsgAcsRequest == NULL) {
		DBGLOG(REQ, ERROR, "allocate msg acs req. fail.\n");
		rStatus = -ENOMEM;
		goto exit;
	}

	kalMemSet(prMsgAcsRequest, 0, msg_size);
	prMsgAcsRequest->rMsgHdr.eMsgId = MID_MNY_P2P_ACS;
	prMsgAcsRequest->ucRoleIdx = role_idx;
	prMsgAcsRequest->icLinkId = link_id;
	prMsgAcsRequest->fgIsHtEnable = ht_enabled;
	prMsgAcsRequest->fgIsHt40Enable = ht40_enabled;
	prMsgAcsRequest->fgIsVhtEnable = vht_enabled;
	prMsgAcsRequest->fgIsEhtEnable = eht_enabled;
	switch (ch_width) {
	case 20:
		prMsgAcsRequest->eChnlBw = MAX_BW_20MHZ;
		break;
	case 40:
		prMsgAcsRequest->eChnlBw = MAX_BW_40MHZ;
		break;
	case 80:
		prMsgAcsRequest->eChnlBw = MAX_BW_80MHZ;
		break;
	case 160:
		prMsgAcsRequest->eChnlBw = MAX_BW_160MHZ;
		break;
	case 320:
		prMsgAcsRequest->eChnlBw = MAX_BW_320_1MHZ;
		break;
	default:
		DBGLOG(REQ, ERROR, "unsupport width: %d.\n", ch_width);
		prMsgAcsRequest->eChnlBw = MAX_BW_UNKNOWN;
		break;
	}
	prMsgAcsRequest->eHwMode = hw_mode;
	prMsgAcsRequest->u4NumChannel = ch_list_count;

	for (i = 0; i < ch_list_count; i++) {
		/* Translate Freq from MHz to channel number. */
		prRfChannelInfo =
			&(prMsgAcsRequest->arChannelListInfo[i]);

		prRfChannelInfo->ucChannelNum =
			nicFreq2ChannelNum(freq_list[i] * 1000);

		if ((freq_list[i] >= 2412) && (freq_list[i] <= 2484))
			prRfChannelInfo->eBand = BAND_2G4;
		else if ((freq_list[i] >= 5180) && (freq_list[i] <= 5900))
			prRfChannelInfo->eBand = BAND_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if ((freq_list[i] >= 5955) && (freq_list[i] <= 7115))
			prRfChannelInfo->eBand = BAND_6G;
#endif

		DBGLOG(REQ, TRACE, "acs channel, band[%d] ch[%d] freq[%d]\n",
			prRfChannelInfo->eBand,
			prRfChannelInfo->ucChannelNum,
			freq_list[i]);

		/* Iteration. */
		prRfChannelInfo++;
	}

	/* Get Lte Safe Chnl, free in p2pRoleFsmRunEventAcs */
	prLteSafeChn = kalMemZAlloc(sizeof(struct PARAM_GET_CHN_INFO),
			VIR_MEM_TYPE);
	if (!prLteSafeChn)
		goto exit;

#if CFG_SUPPORT_GET_LTE_SAFE_CHANNEL
	rStatus = kalIoctl(prGlueInfo, wlanoidQueryLteSafeChannel,
		   prLteSafeChn, sizeof(struct PARAM_GET_CHN_INFO), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(P2P, ERROR, "get safe chnl failed");
#else
	for (i = 0; i < ENUM_SAFE_CH_MASK_MAX_NUM; ++i) {
		prLteSafeChn->rLteSafeChnList.au4SafeChannelBitmask[i] =
			BITS(0, 31);
	}
#endif

	kalMemCopy(&prMsgAcsRequest->au4SafeChnl,
		   prLteSafeChn->rLteSafeChnList.au4SafeChannelBitmask,
		   ENUM_SAFE_CH_MASK_MAX_NUM * sizeof(uint32_t));

	mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgAcsRequest,
			MSG_SEND_METHOD_BUF);

exit:
	if (freq_list)
		kalMemFree(freq_list, VIR_MEM_TYPE,
				sizeof(uint32_t) * ch_list_count);
	if (prLteSafeChn)
		kalMemFree(prLteSafeChn, VIR_MEM_TYPE,
				sizeof(struct PARAM_GET_CHN_INFO));
	if (rStatus == WLAN_STATUS_SUCCESS) {
		reply_skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
				NLMSG_HDRLEN);
		if (reply_skb != NULL)
			return cfg80211_vendor_cmd_reply(reply_skb);
	}
	return rStatus;
}
#endif

int mtk_cfg80211_vendor_dfs_capability(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	uint32_t dfs_capability = 0;
	struct sk_buff *reply_skb;

	ASSERT(wiphy);
	ASSERT(wdev);

	dfs_capability = 1;

	reply_skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
			sizeof(uint32_t) + NLMSG_HDRLEN);

	if (!reply_skb)
		goto nla_put_failure;

	if (nla_put_u32(reply_skb, QCA_ATTR_DFS_CAPAB, dfs_capability))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(reply_skb);

nla_put_failure:
	kfree_skb(reply_skb);
	return -EINVAL;
}

int mtk_cfg80211_vendor_get_features(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct sk_buff *reply_skb;
	uint8_t feature_flags[(NUM_VENDOR_FEATURES + 7) / 8] = {0};
	uint8_t i;

	if (!wiphy || !wdev) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		return -EINVAL;
	}

#if CFG_AUTO_CHANNEL_SEL_SUPPORT
	feature_flags[(VENDOR_FEATURE_SUPPORT_HW_MODE_ANY / 8)] |=
			(1 << (VENDOR_FEATURE_SUPPORT_HW_MODE_ANY % 8));
#endif
#if CFG_SUPPORT_P2P_LISTEN_OFFLOAD
	feature_flags[(VENDOR_FEATURE_P2P_LISTEN_OFFLOAD / 8)] |=
			(1 << (VENDOR_FEATURE_P2P_LISTEN_OFFLOAD % 8));
#endif

#if (CFG_SUPPORT_RSNO == 1)
	feature_flags[(VENDOR_FEATURE_RSN_OVERRIDE_STA / 8)] |=
			(1 << (VENDOR_FEATURE_RSN_OVERRIDE_STA % 8));
#endif /* CFG_SUPPORT_RSNO */

	for (i = 0; i < ((NUM_VENDOR_FEATURES + 7) / 8); i++) {
		DBGLOG(REQ, TRACE, "Dump feature flags[%d]=0x%x.\n", i,
				feature_flags[i]);
	}

	reply_skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
			sizeof(feature_flags) + NLMSG_HDRLEN);

	if (!reply_skb)
		goto nla_put_failure;

	if (nla_put(reply_skb, WIFI_VENDOR_ATTR_FEATURE_FLAGS,
			sizeof(feature_flags), feature_flags))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(reply_skb);

nla_put_failure:
	kfree_skb(reply_skb);
	return -EINVAL;
}

int mtk_cfg80211_vendor_get_chip_capabilities(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct sk_buff *reply_skb;
	int32_t chip_capabilities[NUM_CHIP_CAPABILITIES] = {0};

	if (!wiphy || !wdev)
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		return -EINVAL;
	}

	reply_skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
				sizeof(chip_capabilities) + NLMSG_HDRLEN);

	if (!reply_skb)
		goto nla_put_failure;

	/* return -1 if driver doesn't support the capabilities */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	chip_capabilities[MAX_MLO_ASSOCIATION_LINK_COUNT] = MLD_LINK_MAX;
	chip_capabilities[MAX_MLO_STR_LINK_COUNT] =
		prGlueInfo->prAdapter->rWifiVar.ucMaxSimuLinksCap + 1;
#else
	chip_capabilities[MAX_MLO_ASSOCIATION_LINK_COUNT] = -1;
	chip_capabilities[MAX_MLO_STR_LINK_COUNT] = -1;
#endif /* (CFG_SUPPORT_802_11BE_MLO == 1) */

	chip_capabilities[MAX_CONCURRENT_TDLS_SESSION_COUNT] = MAXNUM_TDLS_PEER;

	if (unlikely(nla_put_s32(reply_skb,
		MAX_MLO_ASSOCIATION_LINK_COUNT,
		chip_capabilities[MAX_MLO_ASSOCIATION_LINK_COUNT]) < 0))
		goto nla_put_failure;
	if (unlikely(nla_put_s32(reply_skb,
		MAX_MLO_STR_LINK_COUNT,
		chip_capabilities[MAX_MLO_STR_LINK_COUNT]) < 0))
		goto nla_put_failure;
	if (unlikely(nla_put_s32(reply_skb,
		MAX_CONCURRENT_TDLS_SESSION_COUNT,
		chip_capabilities[MAX_CONCURRENT_TDLS_SESSION_COUNT]) < 0))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(reply_skb);

nla_put_failure:
	if (reply_skb)
		kfree_skb(reply_skb);
	return -EINVAL;
}

int mtk_cfg80211_vendor_get_apf_capabilities(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	uint32_t aucCapablilities[2] = {APF_VERSION, APF_MAX_PROGRAM_LEN};
	struct sk_buff *skb;
	uint8_t ucBssIdx = 0;
	uint8_t ucAisIdx = 0;
#if (CFG_SUPPORT_APF == 1)
	struct GLUE_INFO *prGlueInfo = NULL;
#if (CFG_SUPPORT_APF_GET_CAPABILITY == 1)
	uint32_t u4SetInfoLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_APF_INFO *prInfo = NULL;
#endif
#endif
	ASSERT(wiphy);
	ASSERT(wdev);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		sizeof(aucCapablilities));

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

#if (CFG_SUPPORT_APF == 1)

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);

	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		goto nla_put_failure;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		goto nla_put_failure;
	}

	if (!prGlueInfo->prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL.\n");
		goto nla_put_failure;
	}

	if (prGlueInfo->prAdapter->rWifiVar.ucApfEnable == 0)
		kalMemZero(&aucCapablilities[0], sizeof(aucCapablilities));
	else {

		ucBssIdx = wlanGetBssIdx(wdev->netdev);
		ucAisIdx = AIS_INDEX(prGlueInfo->prAdapter, ucBssIdx);

#if (CFG_SUPPORT_APF_GET_CAPABILITY == 1)

		prInfo = kalMemZAlloc(sizeof(struct PARAM_APF_INFO),
					VIR_MEM_TYPE);
		if (prInfo == NULL) {
			DBGLOG(REQ, ERROR, "Failed to allocate prInfo\n");
			goto nla_put_failure;
		}

		prInfo->ucBssIdx = ucBssIdx;
		prInfo->ucAisIdx = ucAisIdx;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryApfInfo, prInfo,
				sizeof(struct PARAM_APF_INFO), &u4SetInfoLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			aucCapablilities[0] = prInfo->u4ApfVer;
			aucCapablilities[1] = prInfo->u4ApfBufSize;
		} else {
			DBGLOG(REQ, ERROR, "APF query fail:0x%x\n", rStatus);
		}

#endif
#if (CFG_SUPPORT_MULTI_APF == 0)

		if (ucAisIdx != AIS_DEFAULT_INDEX) {
			DBGLOG(REQ, ERROR,
				"Not supporting APF for secondary STA.\n");
			kalMemZero(&aucCapablilities[0],
				sizeof(aucCapablilities));
		}

#endif
	}
#endif

	if (unlikely(nla_put(skb, APF_ATTRIBUTE_VERSION,
				sizeof(uint32_t), &aucCapablilities[0]) < 0))
		goto nla_put_failure;
	if (unlikely(nla_put(skb, APF_ATTRIBUTE_MAX_LEN,
				sizeof(uint32_t), &aucCapablilities[1]) < 0))
		goto nla_put_failure;

	DBGLOG(REQ, DEBUG, "BSS[%d] Ais[%d] capability - ver:%d, max len: %d\n",
		ucBssIdx, ucAisIdx, aucCapablilities[0], aucCapablilities[1]);

#if (CFG_SUPPORT_APF_GET_CAPABILITY == 1)

	if (prInfo != NULL)
		kalMemFree(prInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_APF_INFO));
#endif

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	kfree_skb(skb);

#if (CFG_SUPPORT_APF_GET_CAPABILITY == 1)

	if (prInfo != NULL)
		kalMemFree(prInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_APF_INFO));
#endif
	return -EFAULT;
}

#if (CFG_SUPPORT_APF == 1)
int mtk_cfg80211_vendor_set_packet_filter(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIdx = 0;
	uint8_t ucAisIdx = 0;
	uint8_t ucApfStart = 0;
	uint16_t u2ApfBufSize = PKT_OFLD_BUF_SIZE;
	uint16_t u2CopySize = 0;

	struct nlattr *attr;
	struct PARAM_OFLD_INFO *prInfo = NULL;

	uint8_t *prProg = NULL;
	uint32_t u4ProgLen = 0, u4SentLen = 0, u4RemainLen = 0;
	uint32_t u4SetInfoLen = 0;

	uint8_t ucFragNum = 0, ucFragSeq = 0, ret = 0;

	ASSERT(wiphy);
	ASSERT(wdev);

	if (data == NULL || data_len <= 0) {
		DBGLOG(REQ, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "Invalid glue info\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	attr = (struct nlattr *)data;

	if (nla_type(attr) != APF_ATTRIBUTE_PROGRAM) {
		DBGLOG(REQ, ERROR, "Get program fail. (%u)\n",
			nla_type(attr));
		return -EINVAL;
	}

	ucBssIdx = wlanGetBssIdx(wdev->netdev);
	ucAisIdx = AIS_INDEX(prGlueInfo->prAdapter, ucBssIdx);

#if (CFG_SUPPORT_MULTI_APF == 0)
	if (ucAisIdx != AIS_DEFAULT_INDEX) {
		DBGLOG(REQ, ERROR, "Not supporting APF for secondary STA.\n");
		return -EFAULT;
	}
#else
	/* First two bytes will used to store BSS index and AIS index */
	ucApfStart = 2;
#endif

	u2ApfBufSize = PKT_OFLD_BUF_SIZE - ucApfStart;

	u4ProgLen = nla_len(attr);
	ucFragNum = u4ProgLen / u2ApfBufSize;
	if (u4ProgLen > u2ApfBufSize && u4ProgLen % u2ApfBufSize > 0)
		ucFragNum++;

	prProg = (uint8_t *) nla_data(attr);
	prInfo = kalMemZAlloc(sizeof(struct PARAM_OFLD_INFO), VIR_MEM_TYPE);
	if (prInfo == NULL) {
		DBGLOG(AIS, WARN, "alloc PARAM_OFLD_INFO failed\n");
		ret = -ENOMEM;
		goto exit;
	}

	DBGLOG_MEM8(REQ, LOUD, prProg, u4ProgLen);

	/* Init OFLD description */
	prInfo->ucType = PKT_OFLD_TYPE_APF;
	prInfo->ucOp = PKT_OFLD_OP_INSTALL;
	prInfo->u4TotalLen = u4ProgLen;
	prInfo->ucFragNum = ucFragNum;

	u4RemainLen = u4ProgLen;
	do {
		prInfo->ucFragSeq = ucFragSeq;
		u2CopySize = u4RemainLen > u2ApfBufSize ?
					u2ApfBufSize : u4RemainLen;

		DBGLOG(REQ, TRACE, "Bss[%d] Ais[%d] Remain[%d]  BufLen[%d]\n",
				ucBssIdx, ucAisIdx, u4RemainLen, u2CopySize);

#if (CFG_SUPPORT_MULTI_APF == 1)
		prInfo->aucBuf[0] = ucBssIdx;
		prInfo->aucBuf[1] = ucAisIdx;
#endif

		kalMemCopy(&prInfo->aucBuf[ucApfStart], (prProg + u4SentLen),
				u2CopySize);

		u4SentLen += u2CopySize;

		if (u4SentLen == u4ProgLen) {
			prInfo->ucOp = PKT_OFLD_OP_ENABLE_W_TPUT_DETECT;
		}

		prInfo->u4BufLen = u2CopySize + ucApfStart;

		DBGLOG(REQ, TRACE, "Set APF size(%d, %d) frag(%d, %d).\n",
				u4ProgLen, u4SentLen,
				ucFragNum, ucFragSeq);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetOffloadInfo, prInfo,
				sizeof(struct PARAM_OFLD_INFO), &u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "APF install fail:0x%x\n", rStatus);
			ret = -EFAULT;
			goto exit;
		}
		ucFragSeq++;
		u4RemainLen -= u2CopySize;
	} while (ucFragSeq < ucFragNum);
exit:
	if (prInfo)
		kalMemFree(prInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_OFLD_INFO));
	return ret;
}


int mtk_cfg80211_vendor_read_packet_filter(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIdx = 0;
	uint8_t ucAisIdx = 0;
	struct PARAM_OFLD_INFO *prInfo = NULL;
	uint32_t u4SetInfoLen = 0;
	struct sk_buff *skb = NULL;

	uint8_t *prProg = NULL;
	uint32_t u4ProgLen = 0, u4RecvLen = 0;
	uint8_t ucFragNum = 0, ucCurrSeq = 0;
	uint8_t ucApfStart = 0;

	ASSERT(wiphy);
	ASSERT(wdev);

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "Invalid glue info\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	ucBssIdx = wlanGetBssIdx(wdev->netdev);
	ucAisIdx = AIS_INDEX(prGlueInfo->prAdapter, ucBssIdx);

#if (CFG_SUPPORT_MULTI_APF == 0)
	if (ucAisIdx != AIS_DEFAULT_INDEX) {
		DBGLOG(REQ, ERROR, "Not supporting APF for secondary STA.\n");
		return -EFAULT;
	}
#endif

	prProg = kalMemZAlloc(APF_MAX_PROGRAM_LEN, VIR_MEM_TYPE);
	prInfo = kalMemZAlloc(sizeof(struct PARAM_OFLD_INFO), VIR_MEM_TYPE);
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		APF_MAX_PROGRAM_LEN);
	if (prProg == NULL || prInfo == NULL || skb == NULL) {
		DBGLOG(REQ, ERROR, "Can not allocate memory.\n");
		goto query_apf_failure;
	}

	/* Init OFLD description */
	prInfo->ucType = PKT_OFLD_TYPE_APF;
	prInfo->ucOp = PKT_OFLD_OP_QUERY;
	prInfo->u4BufLen = PKT_OFLD_BUF_SIZE;

#if (CFG_SUPPORT_MULTI_APF == 1)
	prInfo->aucBuf[0] = ucBssIdx;
	prInfo->aucBuf[1] = ucAisIdx;
	ucApfStart = 2;
#endif

	do {
		rStatus = kalIoctl(prGlueInfo, wlanoidQueryOffloadInfo, prInfo,
				sizeof(struct PARAM_OFLD_INFO), &u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "APF query fail:0x%x\n", rStatus);
			goto query_apf_failure;
		}

		if (ucCurrSeq == 0) {
			ucFragNum = prInfo->ucFragNum;
			u4ProgLen = prInfo->u4TotalLen;
			if (u4ProgLen == 0) {
				DBGLOG(REQ, ERROR,
					"Failed to query APF from firmware.\n");
				 goto query_apf_failure;
			}
		} else if (prInfo->ucFragSeq != ucCurrSeq) {
			DBGLOG(REQ, ERROR, "Wrong frag seq (%d, %d)\n",
				ucCurrSeq, prInfo->ucFragSeq);
			goto query_apf_failure;
		} else if ((prInfo->u4BufLen - ucApfStart) > PKT_OFLD_BUF_SIZE
				|| (u4RecvLen +
				(prInfo->u4BufLen - ucApfStart)) > u4ProgLen
				|| prInfo->u4TotalLen > APF_MAX_PROGRAM_LEN) {

			DBGLOG(REQ, ERROR,
				"Buffer overflow, got wrong size %d\n",
				(u4RecvLen + (prInfo->u4BufLen - ucApfStart)));
			goto query_apf_failure;
		}

		kalMemCopy((prProg + u4RecvLen), &prInfo->aucBuf[ucApfStart],
					(prInfo->u4BufLen - ucApfStart));
		u4RecvLen += (prInfo->u4BufLen - ucApfStart);
		DBGLOG(REQ, DEBUG, "Get APF size(%d, %d) frag(%d, %d).\n",
					u4ProgLen, u4RecvLen,
					ucFragNum, prInfo->ucFragSeq);
		ucCurrSeq++;
		prInfo->ucFragSeq = ucCurrSeq;
	} while (prInfo->ucFragSeq < ucFragNum);

	if (unlikely(nla_put(skb, APF_ATTRIBUTE_PROGRAM,
				u4ProgLen, prProg) < 0))
		goto query_apf_failure;


	if (prProg != NULL)
		kalMemFree(prProg, VIR_MEM_TYPE, APF_MAX_PROGRAM_LEN);
	if (prInfo != NULL)
		kalMemFree(prInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_OFLD_INFO));

	return cfg80211_vendor_cmd_reply(skb);

query_apf_failure:
	if (skb != NULL)
		kfree_skb(skb);

	if (prProg != NULL)
		kalMemFree(prProg, VIR_MEM_TYPE, APF_MAX_PROGRAM_LEN);
	if (prInfo != NULL)
		kalMemFree(prInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_OFLD_INFO));

	return -EFAULT;
}
#endif /* CFG_SUPPORT_APF */

int mtk_cfg80211_vendor_driver_memory_dump(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct LINK_QUALITY_INFO_OUTPUT_DATA {
		uint16_t u2Tag01; /* cur tx rate */
		uint16_t u2Len01;
		uint32_t u4CurTxRate;
		uint16_t u2Tag02; /* tx total count */
		uint16_t u2Len02;
		uint64_t u8TxTotalCount;
		uint16_t u2Tag03; /* tx retry count */
		uint16_t u2Len03;
		uint64_t u8TxRetryCount;
		uint16_t u2Tag04; /* tx fail Count */
		uint16_t u2Len04;
		uint64_t u8TxFailCount;
		uint16_t u2Tag05; /* Rts fail count */
		uint16_t u2Len05;
		uint64_t u8TxRtsFailCount;
		uint16_t u2Tag06; /* Ack fail count */
		uint16_t u2Len06;
		uint64_t u8TxAckFailCount;
		uint16_t u2Tag07; /* cur rx rate */
		uint16_t u2Len07;
		uint32_t u4CurRxRate;
		uint16_t u2Tag08; /* Rx total count */
		uint16_t u2Len08;
		uint64_t u8RxTotalCount;
		uint16_t u2Tag09; /* Rx dup count */
		uint16_t u2Len09;
		uint32_t u4RxDupCount;
		uint16_t u2Tag10; /* Rx err count */
		uint16_t u2Len10;
		uint64_t u8RxErrCount;
		uint16_t u2Tag11; /* Idle slot count */
		uint16_t u2Len11;
		uint64_t u8IdleSlotCount;
		uint16_t u2Tag12; /* Awake duration */
		uint16_t u2Len12;
		uint64_t u8HwMacAwakeDuration;
		uint16_t u2Tag13; /* Scan Flag */
		uint16_t u2Len13;
		uint16_t u2FlagScanning;
	} __packed outputData = {
		.u2Tag01 = 1,  /* tag: 1, cur tx rate */
		.u2Len01 = 4,  /* len: 4, bytes */
		.u2Tag02 = 2,  /* tag: 2, tx total count */
		.u2Len02 = 8,  /* len: 8, bytes */
		.u2Tag03 = 3,  /* tag: 3, tx retry count */
		.u2Len03 = 8,  /* len: 8, bytes */
		.u2Tag04 = 4,  /* tag: 4, tx fail count */
		.u2Len04 = 8,  /* len: 8, bytes */
		.u2Tag05 = 5,  /* tag: 5, tx rts fail count */
		.u2Len05 = 8,  /* len: 8, bytes */
		.u2Tag06 = 6,  /* tag: 6, tx ack fail count */
		.u2Len06 = 8,  /* len: 8, bytes */
		.u2Tag07 = 7,  /* tag: 7, cur rx rate */
		.u2Len07 = 4,  /* len: 4, bytes */
		.u2Tag08 = 8,  /* tag: 8, rx total count */
		.u2Len08 = 8,  /* len: 8, bytes */
		.u2Tag09 = 9,  /* tag: 9, rx dup count */
		.u2Len09 = 4,  /* len: 4, bytes */
		.u2Tag10 = 10, /* tag: 10, rx err count */
		.u2Len10 = 8,  /* len: 8, bytes */
		.u2Tag11 = 11,
		.u2Len11 = 8,
		.u2Tag12 = 12, /* tag: 12, Hw Mac Awake Duration */
		.u2Len12 = 8,  /* len: 8, bytes */
		.u2Tag13 = 13, /* tag: 13, Scanning Flag */
		.u2Len13 = 2,  /* len: 2, bytes */
	};
	struct PARAM_GET_LINK_QUALITY_INFO rParam;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct WIFI_LINK_QUALITY_INFO rLinkQualityInfo;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4BufLen;
#endif
	struct sk_buff *skb = NULL;
	uint32_t *puBuffer = NULL;
	int32_t i4Status;
	uint16_t u2CopySize = 0;

	ASSERT(wiphy);
	ASSERT(wdev);
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) netdev_priv(wdev->netdev);
	if (!prNetDevPrivate) {
		DBGLOG(REQ, ERROR, "Invalid net device private\n");
		return -EFAULT;
	}
	rParam.ucBssIdx = 0; /* prNetDevPrivate->ucBssIdx; */
	rParam.prLinkQualityInfo = &rLinkQualityInfo;
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	i4Status = kalIoctl(prGlueInfo, wlanoidGetLinkQualityInfo, &rParam,
			sizeof(struct PARAM_GET_LINK_QUALITY_INFO), &u4BufLen);
	if (i4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "wlanoidGetLinkQualityInfo error\n");
		goto err_handle_label;
	}

	outputData.u4CurTxRate = rLinkQualityInfo.u4CurTxRate;
	outputData.u8TxTotalCount = rLinkQualityInfo.u8TxTotalCount;
	outputData.u8TxRetryCount = rLinkQualityInfo.u8TxRetryCount;
	outputData.u8TxFailCount = rLinkQualityInfo.u8TxFailCount;
	outputData.u8TxRtsFailCount = rLinkQualityInfo.u8TxRtsFailCount;
	outputData.u8TxAckFailCount = rLinkQualityInfo.u8TxAckFailCount;
	outputData.u4CurRxRate = rLinkQualityInfo.u4CurRxRate;
	outputData.u8RxTotalCount = rLinkQualityInfo.u8RxTotalCount;
	outputData.u4RxDupCount = rLinkQualityInfo.u4RxDupCount;
	outputData.u8RxErrCount = rLinkQualityInfo.u8RxErrCount;
	outputData.u8IdleSlotCount = rLinkQualityInfo.u8IdleSlotCount;
	outputData.u8HwMacAwakeDuration = rLinkQualityInfo.u4HwMacAwakeDuration;
	outputData.u2FlagScanning = rLinkQualityInfo.u2FlagScanning;

	DBGLOG(REQ, DEBUG,
	       "LQ: Tx(rate:%u, total:%llu, Rty:%lu, fail:%lu, RTSF:%lu, ACKF:%lu), Rx(rate:%u, total:%llu, dup:%u, error:%lu), Idle:%lu AwakeDur:%lu\n",
	       outputData.u4CurTxRate, /* tx rate, current tx link speed */
	       outputData.u8TxTotalCount, /* tx total packages */
	       outputData.u8TxRetryCount, /* tx retry count */
	       outputData.u8TxFailCount, /* tx fail count */
	       outputData.u8TxRtsFailCount, /* tx RTS fail count */
	       outputData.u8TxAckFailCount, /* tx ACK fail count */
	       outputData.u4CurRxRate, /* current rx rate */
	       outputData.u8RxTotalCount, /* rx total packages */
	       outputData.u4RxDupCount, /* rx duplicate package count */
	       outputData.u8RxErrCount, /* rx error count */
	       outputData.u8IdleSlotCount,
	       outputData.u8HwMacAwakeDuration
	);

	u2CopySize = sizeof(struct LINK_QUALITY_INFO_OUTPUT_DATA);
	puBuffer = (uint32_t *)&outputData;
#endif

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, u2CopySize);
	if (!skb) {
		DBGLOG(REQ, ERROR, "allocate skb failed\n");
		i4Status = -ENOMEM;
		goto err_handle_label;
	}

	if (unlikely(nla_put_nohdr(skb, u2CopySize, puBuffer) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed: len=%u, ptr=%p\n",
		       u2CopySize, puBuffer);
		i4Status = -EINVAL;
		goto err_handle_label;
	}

	return cfg80211_vendor_cmd_reply(skb);

err_handle_label:
	kfree_skb(skb);
	return i4Status;
}
#if (CFG_SUPPORT_STATISTICS == 1)
int mtk_cfg80211_vendor_get_trx_stats(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct sk_buff *skb = NULL;
	int32_t i4Status = -EFAULT;
	uint8_t ucTxNum = 0, ucRxNum = 0, ucCgsNum = 0;
	uint32_t u4TxTlvSize, u4RxTlvSize, u4CgsTlvSize;
	uint32_t u4MaxTlvSize;
	struct STATS_TRX_TLV_T *aucTlvList = NULL;
	uint8_t version = 3;
	uint8_t ucBssIdx;

	uint32_t *arIndTx = NULL, *arIndRx = NULL, *arIndCgs = NULL;
	struct nlattr *attr[WIFI_ATTRIBUTE_STATS_MAX];
	uint32_t i = 0;

	ASSERT(wiphy && wdev);
	DBGLOG(REQ, TRACE, "data_len=%d, iftype=%d\n", data_len, wdev->iftype);
	if (data == NULL || data_len <= 0) {
		log_dbg(REQ, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return i4Status;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (!prGlueInfo->prAdapter)
		return -EFAULT;

	/* parsing required info */
	if (NLA_PARSE_NESTED(attr,
			     WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST,
			     (struct nlattr *)(data - NLA_HDRLEN),
			     nla_trx_stats_policy) < 0) {
		DBGLOG(REQ, ERROR, "%s nla_parse_nested failed\n",
		       __func__);
		return i4Status;
	}

	ucBssIdx = wlanGetBssIdx(wdev->netdev);

	DBGLOG(REQ, DEBUG, "bssIdx:%u\n", ucBssIdx);
#define MAX_TAG_NUM 16
	for (i = WIFI_ATTRIBUTE_STATS_TX;
	     i <= WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST; i++) {
		if (attr[i]) {
			switch (i) {
			case WIFI_ATTRIBUTE_STATS_TX_NUM:
				ucTxNum = nla_get_u8(attr[i]);
				if (ucTxNum > MAX_TAG_NUM)
					goto err_handle_label;
				break;
			case WIFI_ATTRIBUTE_STATS_TX_TAG_LIST:
				if (ucTxNum != 0 && !arIndTx) {
					arIndTx = (uint32_t *)kalMemAlloc(
						ucTxNum * sizeof(uint32_t),
						VIR_MEM_TYPE);
					if (!arIndTx) {
						DBGLOG(REQ, ERROR,
							"Can not alloc memory for ind tx info\n");
						i4Status = -ENOMEM;
						goto err_handle_label;
					}
					kalMemCopy(arIndTx, nla_data(attr[i]),
						ucTxNum * sizeof(uint32_t));
				}
				break;
			case WIFI_ATTRIBUTE_STATS_RX_NUM:
				ucRxNum = nla_get_u8(attr[i]);
				if (ucRxNum > MAX_TAG_NUM)
					goto err_handle_label;
				break;
			case WIFI_ATTRIBUTE_STATS_RX_TAG_LIST:
				if (ucRxNum != 0 && !arIndRx) {
					arIndRx = (uint32_t *)kalMemAlloc(
						ucRxNum * sizeof(uint32_t),
						VIR_MEM_TYPE);
					if (!arIndRx) {
						DBGLOG(REQ, ERROR,
							"Can not alloc memory for ind tx info\n");
						i4Status = -ENOMEM;
						goto err_handle_label;
					}
					kalMemCopy(arIndRx, nla_data(attr[i]),
						ucRxNum * sizeof(uint32_t));
				}
				break;
			case WIFI_ATTRIBUTE_STATS_CGS_NUM:
				ucCgsNum = nla_get_u8(attr[i]);
				if (ucCgsNum > MAX_TAG_NUM)
					goto err_handle_label;
				break;
			case WIFI_ATTRIBUTE_STATS_CGS_TAG_LIST:
				if (ucCgsNum != 0 && !arIndCgs) {
					arIndCgs = (uint32_t *)kalMemAlloc(
						ucCgsNum * sizeof(uint32_t),
						VIR_MEM_TYPE);
					if (!arIndCgs) {
						DBGLOG(REQ, ERROR,
							"Can not alloc memory for ind tx info\n");
						i4Status = -ENOMEM;
						goto err_handle_label;
					}
					kalMemCopy(arIndCgs, nla_data(attr[i]),
						ucCgsNum * sizeof(uint32_t));
				}
				break;
			}
		}
	}

	DBGLOG(REQ, TRACE, "%s TxNum:%d RxNum:%d CgsNum:%d\n",
		       __func__, ucTxNum, ucRxNum, ucCgsNum);

	u4TxTlvSize = statsGetTlvStatTotalLen(prGlueInfo,
		STATS_TX_TAG, ucTxNum, arIndTx);
	u4RxTlvSize = statsGetTlvStatTotalLen(prGlueInfo,
		STATS_RX_TAG, ucRxNum, arIndRx);
	u4CgsTlvSize = statsGetTlvStatTotalLen(prGlueInfo,
		STATS_CGS_TAG, ucCgsNum, arIndCgs);

	u4MaxTlvSize = max(max(u4TxTlvSize, u4RxTlvSize),
				     u4CgsTlvSize);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		u4TxTlvSize + u4RxTlvSize + u4CgsTlvSize);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		i4Status = -ENOMEM;
		goto err_handle_label;
	}

	if (unlikely(nla_put_u8(skb, WIFI_ATTRIBUTE_STATS_VERSION,
				 version) < 0))
		goto err_handle_label;

	aucTlvList = (struct STATS_TRX_TLV_T *) kalMemAlloc(u4MaxTlvSize,
		VIR_MEM_TYPE);

	if (!aucTlvList) {
		DBGLOG(REQ, ERROR,
			"Can not alloc memory for stats info\n");
		i4Status = -ENOMEM;
		goto err_handle_label;
	}

	kalMemZero(aucTlvList, u4MaxTlvSize);
	if (ucTxNum != 0) {
		statsGetInfoHdlr(ucBssIdx, prGlueInfo, aucTlvList,
			STATS_TX_TAG, ucTxNum, arIndTx);
		if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_STATS_TX,
			     u4TxTlvSize, aucTlvList) < 0))
			goto err_handle_label;
	}

	/* rx tlv */
	kalMemZero(aucTlvList, u4MaxTlvSize);
	if (ucRxNum != 0) {
		statsGetInfoHdlr(ucBssIdx, prGlueInfo, aucTlvList,
			STATS_RX_TAG, ucRxNum, arIndRx);
		if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_STATS_RX,
				     u4RxTlvSize, aucTlvList) < 0))
			goto err_handle_label;
	}

	/* cgstn tlv */
	kalMemZero(aucTlvList, u4MaxTlvSize);
	if (ucCgsNum != 0) {
		statsGetInfoHdlr(ucBssIdx, prGlueInfo, aucTlvList,
			STATS_CGS_TAG, ucCgsNum, arIndCgs);
		if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_STATS_CGS,
				     u4CgsTlvSize, aucTlvList) < 0))
			goto err_handle_label;
	}
	if (aucTlvList != NULL)
		kalMemFree(aucTlvList, u4MaxTlvSize, VIR_MEM_TYPE);
	if (arIndTx != NULL)
		kalMemFree(arIndTx, ucTxNum * sizeof(uint32_t), VIR_MEM_TYPE);
	if (arIndRx != NULL)
		kalMemFree(arIndRx, ucRxNum * sizeof(uint32_t), VIR_MEM_TYPE);
	if (arIndCgs != NULL)
		kalMemFree(arIndCgs, ucCgsNum * sizeof(uint32_t), VIR_MEM_TYPE);
	return cfg80211_vendor_cmd_reply(skb);

err_handle_label:
	if (aucTlvList != NULL)
		kalMemFree(aucTlvList, u4MaxTlvSize, VIR_MEM_TYPE);
	if (arIndTx != NULL)
		kalMemFree(arIndTx, ucTxNum * sizeof(uint32_t), VIR_MEM_TYPE);
	if (arIndRx != NULL)
		kalMemFree(arIndRx, ucRxNum * sizeof(uint32_t), VIR_MEM_TYPE);
	if (arIndCgs != NULL)
		kalMemFree(arIndCgs, ucCgsNum * sizeof(uint32_t), VIR_MEM_TYPE);
	if (skb != NULL)
		kfree_skb(skb);
	return i4Status;
}
#endif
#endif /* KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to handle a reset CMD from FWK.
 *
 * \param[in] wiphy wiphy
 * \param[in] wdev wireless_dev
 * \param[in] data (not used here)
 * \param[in] data_len (not used here)
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_trigger_reset(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);

	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "Invalid glue info\n");
		return -EFAULT;
	}
	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
	DBGLOG(REQ, DEBUG, "Framework trigger reset\n");

	GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter, RST_FWK_TRIGGER);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is for P2P GO to add DFS channel that
 * being used by connected AP to perfer channel list.
 *
 * \param[in] wiphy
 * \param[in] wdev wireless_dev
 * \param[in] data (not used here)
 * \param[in] data_len (not used here)
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_enable_sta_channel_for_peer_network(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct nlattr *attr;
	uint32_t  sta_channel_for_peer_network_enable;

	if (!wiphy || !wdev) {
		log_dbg(REQ, ERROR, "Not find wiphy %d, wdev %d\n",
			(wiphy == NULL), (wdev == NULL));
		return -EINVAL;
	}

	if (!data || data_len <= 0) {
		log_dbg(REQ, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "Invalid glue info\n");
		return -EFAULT;
	}
	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter) {
		DBGLOG(REQ, WARN, "Invalid adapter\n");
		return -EFAULT;
	}

	attr = (struct nlattr *)data;

	sta_channel_for_peer_network_enable = nla_get_u32(attr);

	prAdapter->fgEnableStaDfsChannel =
		sta_channel_for_peer_network_enable & INDOOR_CHANNEL;
	prAdapter->fgEnableStaIndoorChannel =
		sta_channel_for_peer_network_enable & DFS_CHANNEL;

	DBGLOG(REQ, INFO, "STA_CHANNEL_FOR_P2P. (0x%x)\n",
			sta_channel_for_peer_network_enable);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to send FWK the number of band/ant combination.
 *
 * \param[in] wiphy wiphy
 * \param[in] wdev wireless_dev
 * \param[in] data (not used here)
 * \param[in] data_len (not used here)
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_comb_matrix(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	struct sk_buff *skb = NULL;
	struct ANDROID_T_COMB_MATRIX *pr_comb_matrix = NULL;
	int32_t i4Status = 0;

	pr_comb_matrix = (struct ANDROID_T_COMB_MATRIX *)
		kalMemAlloc(sizeof(struct ANDROID_T_COMB_MATRIX),
		VIR_MEM_TYPE);

	if (!pr_comb_matrix) {
		DBGLOG(REQ, ERROR,
			"Can not alloc memory for stats info\n");
		i4Status = -ENOMEM;
		goto end;
	}
	kalMemZero(pr_comb_matrix,
		   sizeof(struct ANDROID_T_COMB_MATRIX));

	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "Invalid glue info\n");
		i4Status = -EFAULT;
		goto end;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		i4Status = -EFAULT;
		goto end;
	}

	pr_comb_matrix->comb_mtx[0].band_0 = 2;
	pr_comb_matrix->comb_mtx[0].ant_0 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
	pr_comb_matrix->comb_mtx[1].band_0 = 5;
	pr_comb_matrix->comb_mtx[1].ant_0 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
#if (CFG_SUPPORT_WIFI_6G == 1)
	pr_comb_matrix->comb_mtx[2].band_0 = 6;
	pr_comb_matrix->comb_mtx[2].ant_0 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
#endif
	pr_comb_matrix->comb_mtx[3].band_0 = 2;
	pr_comb_matrix->comb_mtx[3].band_1 = 5;
#if (CFG_SUPPORT_WIFI_6G == 1)
	pr_comb_matrix->comb_mtx[4].band_0 = 2;
	pr_comb_matrix->comb_mtx[4].band_1 = 6;
	pr_comb_matrix->comb_mtx[5].band_0 = 5;
	pr_comb_matrix->comb_mtx[5].band_1 = 6;
#endif
#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1)
	pr_comb_matrix->comb_mtx[3].ant_0 = 1;
	pr_comb_matrix->comb_mtx[3].ant_1 = 1;
	pr_comb_matrix->comb_mtx[4].ant_0 = 1;
	pr_comb_matrix->comb_mtx[4].ant_1 = 1;
	pr_comb_matrix->comb_mtx[5].ant_0 = 1;
	pr_comb_matrix->comb_mtx[5].ant_1 = 1;
#else
	pr_comb_matrix->comb_mtx[3].ant_0 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
	pr_comb_matrix->comb_mtx[3].ant_1 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
	pr_comb_matrix->comb_mtx[4].ant_0 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
	pr_comb_matrix->comb_mtx[4].ant_1 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
	pr_comb_matrix->comb_mtx[5].ant_0 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
	pr_comb_matrix->comb_mtx[5].ant_1 =
		prGlueInfo->prAdapter->rWifiVar.ucNSS;
#endif

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		sizeof(struct ANDROID_T_COMB_MATRIX));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		i4Status = -ENOMEM;
		goto end;
	}

	if (unlikely(nla_put(skb,
		WIFI_ATTRIBUTE_RADIO_COMBINATIONS_MATRIX_MATRIX,
		sizeof(struct ANDROID_T_COMB_MATRIX),
		pr_comb_matrix) < 0)) {
		i4Status = -EINVAL;
		goto end;
	}

	kalMemFree(pr_comb_matrix,
		sizeof(struct ANDROID_T_COMB_MATRIX),
		VIR_MEM_TYPE);
	return cfg80211_vendor_cmd_reply(skb);
end:
	if (pr_comb_matrix)
		kalMemFree(pr_comb_matrix,
			sizeof(struct ANDROID_T_COMB_MATRIX),
			VIR_MEM_TYPE);
	kfree_skb(skb);
	return i4Status;
}

static u_int8_t isIfaceTypeSupportChannel(struct ADAPTER *prAdapter,
	uint32_t iface, uint32_t filter, enum ENUM_BAND eBand,
	uint8_t ucChannelNum, uint32_t u4channel_freq, uint8_t fgDFS,
	uint32_t *pau4SafeChnl)
{
	uint16_t u2CountryCode = prAdapter->rWifiVar.u2CountryCode;
	u_int8_t result = TRUE;

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	u2CountryCode = rlmDomainReverseAlpha2(u2CountryCode);
#endif /* CFG_SUPPORT_CE_6G_PWR_REGULATIONS */

	if (iface & (IFACE_MODE_STA | IFACE_MODE_P2P_CLIENT | IFACE_MODE_TDLS |
		IFACE_MODE_NAN)){
		/* do nothing */
	} else if (iface & GO_SAP_IFTYPE) {
		if (eBand == BAND_5G) {
			if (rlmDomainIsIndoorChannel(
				prAdapter,
				BAND_5G,
				ucChannelNum)) {
				DBGLOG(REQ, TRACE,
					"skip freq %d, indoor channel\n",
					u4channel_freq);
				result = FALSE;
			} else if (fgDFS) {
				DBGLOG(REQ, TRACE,
					"skip freq %d, dfs channel\n",
					u4channel_freq);
				result = FALSE;
			} else if ((u2CountryCode == COUNTRY_CODE_TW) &&
				(u4channel_freq >= 5180 &&
					u4channel_freq <= 5260)) {
				DBGLOG(REQ, TRACE,
					"skip freq %d, country=%d\n",
					u4channel_freq, u2CountryCode);
				result = FALSE;
			}
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eBand == BAND_6G) {
			if (!IS_6G_PSC_CHANNEL(ucChannelNum)) {
				DBGLOG(REQ, TRACE,
					"skip freq %d, not psc channel\n",
					u4channel_freq);
				result = FALSE;
			}
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
			else if (!rlmDomainIsLegalChlByNetType(
					prAdapter,
					eBand,
					ucChannelNum,
					MAX_BW_20MHZ, /* NOT SUER */
					NETWORK_TYPE_P2P)) {
				DBGLOG(REQ, TRACE,
					"skip freq %d, not vlp\n",
					u4channel_freq);
				result = FALSE;
			}
#else
			else if (u4channel_freq >= 6435) {
				DBGLOG(REQ, TRACE,
					"skip freq %d >= 6435\n",
					u4channel_freq);
				result = FALSE;
			}
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE */
		}
#endif /* CFG_SUPPORT_WIFI_6G */

		if ((filter & CELLULAR_COEXISTENCE) && pau4SafeChnl) {
			if (!p2pFuncIsLteSafeChnl(
				eBand,
				ucChannelNum,
				pau4SafeChnl)) {
				DBGLOG(REQ, TRACE,
					"skip freq %d, lte safe chnl\n",
					u4channel_freq);
				result = FALSE;
			}
		}
	}
	return result;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to send FWK the usable channel for each source.
 *
 * \param[in] wiphy wiphy
 * \param[in] wdev wireless_dev
 * \param[in] data (not used here)
 * \param[in] data_len (not used here)
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_get_usable_channel(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	struct ADAPTER *prAdapter;
	struct sk_buff *skb = NULL;
	struct ANDROID_USABLE_CHANNEL_ARRAY *pr_channel_array = NULL;
	struct ANDROID_USABLE_CHANNEL_UNIT *pr_channel_unit = NULL;
	int32_t i4Status = 0;
	struct nlattr *attr;
	uint32_t band_mask = 0;
	uint32_t iface;
	uint32_t iface_type = 0;
	uint32_t iface_type_copy = 0;
	uint32_t supported_iface_mask;
	uint32_t filter = 0;
	uint32_t max_size = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct nlattr *tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX + 1] = {};
	struct RF_CHANNEL_INFO *aucChannelList = NULL;
	uint8_t i, usable_channel_nums;
	uint32_t u4channel_freq;
	uint8_t ucNumOfChannel = 0;
	struct PARAM_GET_CHN_INFO *prLteSafeChn = NULL;
	struct LTE_SAFE_CHN_INFO *prLteSafeChnList = NULL;
	uint32_t *pau4SafeChnl = NULL;
	uint32_t u4SafeChnlInfo_2g;
	uint32_t u4SafeChnlInfo_5g_0;
	uint32_t u4SafeChnlInfo_5g_1;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint32_t u4SafeChnlInfo_6g;
#endif /* CFG_SUPPORT_WIFI_6G */
	uint32_t u4BufLen;
	uint32_t idx;
	u_int8_t result;


	if (NLA_PARSE(tb, WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX, data, data_len,
			mtk_usable_channel_policy)) {
		DBGLOG(REQ, ERROR, "parse acs attr fail.\n");
		rStatus = -EINVAL;
		goto end;
	}

	if (!tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_BAND]) {
		DBGLOG(REQ, ERROR, "attr channel band failed.\n");
		rStatus = -EINVAL;
		goto end;
	}
	band_mask = nla_get_u32(tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_BAND]);

	if (!tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_IFACE]) {
		DBGLOG(REQ, ERROR, "attr channel iface failed.\n");
		rStatus = -EINVAL;
		goto end;
	}
	iface_type = nla_get_u32(tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_IFACE]);

	if (!tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX_SIZE]) {
		DBGLOG(REQ, ERROR, "attr channel max size failed.\n");
		rStatus = -EINVAL;
		goto end;
	}
	max_size = nla_get_u32(tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX_SIZE]);

	if (!tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_FILTER]) {
		DBGLOG(REQ, ERROR, "attr channel filter failed.\n");
		rStatus = -EINVAL;
		goto end;
	}
	filter = nla_get_u32(tb[WIFI_ATTRIBUTE_USABLE_CHANNEL_FILTER]);

	DBGLOG(REQ, TRACE,
		"band_mask = 0x%x, iface_type = 0x%x, max_size = 0x%x, filter = 0x%x\n"
		, band_mask, iface_type, max_size, filter);

	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "Invalid glue info\n");
		i4Status = -EFAULT;
		goto end;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;

	attr = (struct nlattr *)data;

	aucChannelList = (struct RF_CHANNEL_INFO *)
		kalMemAlloc(sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM,
			VIR_MEM_TYPE);
	if (!aucChannelList) {
		DBGLOG(REQ, ERROR,
			"Can not alloc memory for rf channel info\n");
		return -ENOMEM;
	}
	kalMemZero(aucChannelList,
		sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM);
	pr_channel_array = (struct ANDROID_USABLE_CHANNEL_ARRAY *)
		kalMemAlloc(
		(sizeof(struct ANDROID_USABLE_CHANNEL_UNIT)*
		MAX_CHN_NUM +
		sizeof(struct ANDROID_USABLE_CHANNEL_ARRAY)),
		VIR_MEM_TYPE);

	if (!pr_channel_array) {
		DBGLOG(REQ, ERROR,
			"Can not alloc memory for stats info\n");
		i4Status = -ENOMEM;
		goto end;
	}

	if (filter & CELLULAR_COEXISTENCE) {
		prLteSafeChn = kalMemZAlloc(sizeof(struct PARAM_GET_CHN_INFO),
			VIR_MEM_TYPE);
		if (!prLteSafeChn) {
			DBGLOG(REQ, TRACE, "allocate prLteSafeChn failed\n");
		} else {
			rStatus = kalIoctl(prGlueInfo,
				wlanoidQueryLteSafeChannel, prLteSafeChn,
				sizeof(struct PARAM_GET_CHN_INFO), &u4BufLen);
			prLteSafeChnList = &(prLteSafeChn->rLteSafeChnList);
			pau4SafeChnl = prLteSafeChnList->au4SafeChannelBitmask;

			u4SafeChnlInfo_2g =
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_2G4];
			u4SafeChnlInfo_5g_0 =
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_0];
			u4SafeChnlInfo_5g_1 =
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_1];
#if (CFG_SUPPORT_WIFI_6G == 1)
			u4SafeChnlInfo_6g =
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_6G];
#endif /* CFG_SUPPORT_WIFI_6G */

			if (!u4SafeChnlInfo_2g && !u4SafeChnlInfo_5g_0 &&
				!u4SafeChnlInfo_5g_1
#if (CFG_SUPPORT_WIFI_6G == 1)
				&& !u4SafeChnlInfo_6g
#endif /* CFG_SUPPORT_WIFI_6G */
			) {
				DBGLOG(P2P, WARN,
					"No safe chnl, reset bitmap\n");
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_2G4] =
					BITS(0, 31);
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_0] =
					BITS(0, 31);
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_5G_1] =
					BITS(0, 31);
#if (CFG_SUPPORT_WIFI_6G == 1)
				pau4SafeChnl[ENUM_SAFE_CH_MASK_BAND_6G] =
					BITS(0, 31);
#endif /* CFG_SUPPORT_WIFI_6G */
			}
		}
	}

	rlmDomainGetChnlList(prAdapter, BAND_NULL, FALSE,
		MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);

	for (i = 0; i < ucNumOfChannel; i++) {
		u4channel_freq = nicChannelNum2Freq(
				aucChannelList[i].ucChannelNum,
				aucChannelList[i].eBand) / 1000;
		if (u4channel_freq == 0)
			continue;
		pr_channel_array->channel_array[i].channel_freq =
			u4channel_freq;
		/* skip 2.4G if not included in band mask */
		if (aucChannelList[i].eBand == BAND_2G4 &&
			!(band_mask & WIFIBAND_BAND_24GHZ)) {
			DBGLOG(REQ, TRACE,
				"skip 2G freq %d due to  band_mask=%d\n"
				, u4channel_freq, band_mask);
			continue;
		}
		/* skip 5G non DFS if not included in band mask */
		if (aucChannelList[i].eBand == BAND_5G &&
			!(band_mask & WIFIBAND_BAND_5GHZ)) {
			DBGLOG(REQ, TRACE,
				"skip 5G freq %d due to band_mask=%d\n"
				, u4channel_freq, band_mask);
			continue;
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (aucChannelList[i].eBand == BAND_6G &&
			!(band_mask & WIFIBAND_BAND_6GHZ)) {
			DBGLOG(REQ, TRACE,
				"skip 6G freq %d due to band_mask=%d\n"
				, u4channel_freq, band_mask);
			continue;
		}
#endif /*CFG_SUPPORT_WIFI_6G*/

		if (aucChannelList[i].eBand == BAND_2G4)
			pr_channel_array->channel_array[i].channel_width =
				ANDROID_WIFI_CHAN_WIDTH_20;
		else if (aucChannelList[i].eBand == BAND_5G) {
			pr_channel_unit = &(pr_channel_array->channel_array[i]);
			if (aucChannelList[i].fgDFS)
				pr_channel_unit->channel_width =
					ANDROID_WIFI_CHAN_WIDTH_160;
			else
				pr_channel_unit->channel_width =
						ANDROID_WIFI_CHAN_WIDTH_80;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (aucChannelList[i].eBand == BAND_6G)
			pr_channel_array->channel_array[i].channel_width =
				ANDROID_WIFI_CHAN_WIDTH_160;
#endif /*CFG_SUPPORT_WIFI_6G*/
		iface_type_copy = iface_type;
		idx = 0;
		supported_iface_mask = 0;
		while (iface_type_copy) {
			iface = iface_type_copy & BIT(idx);
			result = isIfaceTypeSupportChannel(prAdapter,
					iface,
					filter,
					aucChannelList[i].eBand,
					aucChannelList[i].ucChannelNum,
					u4channel_freq,
					aucChannelList[i].fgDFS,
					pau4SafeChnl);
			if (result == TRUE)
				supported_iface_mask |= iface;
			iface_type_copy &= ~(iface);
			idx++;
		}
		pr_channel_array->channel_array[i].iface_mode_mask =
			supported_iface_mask;
		if (supported_iface_mask != 0)
			DBGLOG(REQ, TRACE,
				"channels[%d], freq %d\n", i, u4channel_freq);
	}
	usable_channel_nums = 0;
	for (i = 0; i < ucNumOfChannel; i++) {
		if (pr_channel_array->channel_array[i].iface_mode_mask != 0) {
			pr_channel_array->channel_array[usable_channel_nums] =
				pr_channel_array->channel_array[i];
			usable_channel_nums++;
		}
	}

	usable_channel_nums = usable_channel_nums > max_size ?
		max_size : usable_channel_nums;

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		(sizeof(struct ANDROID_USABLE_CHANNEL_UNIT)*
		usable_channel_nums +
		sizeof(u16)));
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		i4Status = -ENOMEM;
		goto end;
	}
	pr_channel_array->array_size = usable_channel_nums;

	if (unlikely(nla_put(skb,
		WIFI_ATTRIBUTE_USABLE_CHANNEL_ARRAY,
		sizeof(struct ANDROID_USABLE_CHANNEL_UNIT)*
		usable_channel_nums +
		sizeof(struct ANDROID_USABLE_CHANNEL_ARRAY),
		pr_channel_array) < 0)) {
		i4Status = -EINVAL;
		goto end;
	}

	kalMemFree(aucChannelList, VIR_MEM_TYPE,
		sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM);
	kalMemFree(pr_channel_array,
		sizeof(struct ANDROID_USABLE_CHANNEL_ARRAY),
		VIR_MEM_TYPE);
	kalMemFree(prLteSafeChn, VIR_MEM_TYPE,
			sizeof(struct PARAM_GET_CHN_INFO));

	return cfg80211_vendor_cmd_reply(skb);
end:
	if (aucChannelList)
		kalMemFree(aucChannelList, VIR_MEM_TYPE,
			sizeof(struct RF_CHANNEL_INFO)*MAX_CHN_NUM);
	if (pr_channel_array)
		kalMemFree(pr_channel_array,
			sizeof(struct ANDROID_USABLE_CHANNEL_ARRAY),
			VIR_MEM_TYPE);
	if (prLteSafeChn)
		kalMemFree(prLteSafeChn, VIR_MEM_TYPE,
				sizeof(struct PARAM_GET_CHN_INFO));
	kfree_skb(skb);
	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to send FWK a event that the reset happened.
 *
 * \param[in] data reset reason, eResetReason.
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_event_reset_triggered(
	struct GLUE_INFO *prGlueInfo, uint32_t data)
{
	struct wiphy *wiphy = NULL;
	struct wireless_dev *wdev = NULL;
	struct wireless_dev **pprWdev = NULL;
	struct sk_buff *skb;

	pprWdev = wlanGetWirelessDevice(prGlueInfo);

	if (!pprWdev) {
		DBGLOG(REQ, ERROR, "pprWdev is NULL\n");
		return -EINVAL;
	}

	wdev = pprWdev[0];
	if (!wdev || !wdev->netdev || !wdev->wiphy) {
		DBGLOG(REQ, ERROR, "%s wrong input parameters\n", __func__);
		return -EINVAL;
	}
	wiphy = wdev->wiphy;

	DBGLOG(REQ, DEBUG, "Reset event report through %s. Reason=[%u]\n",
			wdev->netdev->name, data);

	skb = cfg80211_vendor_event_alloc(wiphy,
#if KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE
			wdev,
#endif
			sizeof(uint32_t),
			WIFI_EVENT_RESET_TRIGGERED,
			GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, WIFI_ATTRIBUTE_RESET_REASON,
				data) < 0))
		goto nla_put_failure;

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return 0;

nla_put_failure:
	kfree_skb(skb);
	return -ENOMEM;
}

#if CFG_SUPPORT_CSI
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to set CSI CMD from FWK.
 *
 * \param[in] wiphy wiphy
 * \param[in] wdev wireless_dev
 * \param[in] data
 * \param[in] data_len
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_csi_control(
	struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *tb[WIFI_ATTRIBUTE_CSI_MAX + 1] = {};
	uint32_t rStatus;
	uint32_t u4BufLen;
	int32_t i4Status = 0;
	struct CMD_CSI_CONTROL_T *prCSICtrl = NULL;
	struct CSI_INFO_T *prCSIInfo = NULL;
	struct BSS_INFO *prAisBssInfo;

	if (!wiphy || !wdev || data == NULL || data_len == 0)
		return -EINVAL;

	prGlueInfo = wlanGetGlueInfoByWiphy(wiphy);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	DBGLOG(REQ, DEBUG,
	       "[CSI] vendor command: data_len=%d, iftype=%d\n", data_len,
	       wdev->iftype);

	prCSIInfo = glCsiGetCSIInfo(prGlueInfo);
	prCSICtrl = (struct CMD_CSI_CONTROL_T *) kalMemAlloc(
			sizeof(struct CMD_CSI_CONTROL_T), VIR_MEM_TYPE);
	if (!prCSICtrl) {
		DBGLOG(REQ, LOUD,
			"[CSI] allocate memory for prCSICtrl failed\n");
		i4Status = -ENOMEM;
		goto out;
	}

	kalMemZero(tb, sizeof(struct nlattr *) *
			   (WIFI_ATTRIBUTE_CSI_MAX + 1));

	if (NLA_PARSE(tb, WIFI_ATTRIBUTE_CSI_MAX, (struct nlattr *)data,
			data_len, nla_get_csi_policy)) {
		DBGLOG(REQ, ERROR, "[CSI] parse csi attr fail.\n");
		i4Status = -EINVAL;
		goto out;
	}

	if (!tb[WIFI_ATTRIBUTE_CSI_CONTROL_MODE]) {
		DBGLOG(REQ, LOUD, "[CSI] parse csi mode error.\n");
		i4Status = -EINVAL;
		goto out;
	}
	prCSICtrl->ucMode = nla_get_u8(tb[WIFI_ATTRIBUTE_CSI_CONTROL_MODE]);
	if (prCSICtrl->ucMode >= CSI_CONTROL_MODE_NUM) {
		DBGLOG(REQ, ERROR, "[CSI] Invalid csi mode %d\n",
			prCSICtrl->ucMode);
		i4Status = -EINVAL;
		goto out;
	}
	prCSIInfo->ucMode = prCSICtrl->ucMode;

	prAisBssInfo = aisGetAisBssInfo(
			prGlueInfo->prAdapter, wlanGetBssIdx(wdev->netdev));
	if (prAisBssInfo->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			|| prAisBssInfo->eBand == BAND_6G
#endif
		)
		prCSICtrl->ucBandIdx = ENUM_BAND_1;
	else
		prCSICtrl->ucBandIdx = ENUM_BAND_0;

	if (prCSICtrl->ucMode == CSI_CONTROL_MODE_STOP ||
		prCSICtrl->ucMode == CSI_CONTROL_MODE_START) {
		glCsiSetEnable(prGlueInfo,
			       prCSIInfo,
			       prCSICtrl->ucMode == CSI_CONTROL_MODE_START);
		goto send_cmd;
	}

	if (!tb[WIFI_ATTRIBUTE_CSI_CONFIG_ITEM]) {
		DBGLOG(REQ, LOUD, "[CSI] parse cfg item error.\n");
		i4Status = -EINVAL;
		goto out;
	}
	prCSICtrl->ucCfgItem = nla_get_u8(tb[WIFI_ATTRIBUTE_CSI_CONFIG_ITEM]);

	if (prCSICtrl->ucCfgItem >= CSI_CONFIG_ITEM_NUM) {
		DBGLOG(REQ, ERROR, "[CSI] Invalid csi cfg_item %u\n",
			prCSICtrl->ucCfgItem);
		i4Status = -EINVAL;
		goto out;
	}

	if (!tb[WIFI_ATTRIBUTE_CSI_VALUE_1]) {
		DBGLOG(REQ, LOUD, "[CSI] parse csi cfg value1 error.\n");
		i4Status = -EINVAL;
		goto out;
	}
	prCSICtrl->ucValue1 = nla_get_u8(tb[WIFI_ATTRIBUTE_CSI_VALUE_1]);
	prCSIInfo->ucValue1[prCSICtrl->ucCfgItem] = prCSICtrl->ucValue1;

	if (prCSICtrl->ucCfgItem == CSI_CONFIG_OUTPUT_METHOD) {
		if (prCSICtrl->ucValue1 == CSI_PROC_FILE_COMMAND) {
			prCSIInfo->eCSIOutput = CSI_OUTPUT_PROC_FILE;
			DBGLOG(REQ, DEBUG,
				"[CSI] Set CSI data output to proc file\n");
		} else if (prCSICtrl->ucValue1 == CSI_VENDOR_EVENT_COMMAND) {
			prCSIInfo->eCSIOutput = CSI_OUTPUT_VENDOR_EVENT;
			DBGLOG(REQ, DEBUG,
				"[CSI] Set CSI data output to vendor event\n");
		} else
			DBGLOG(REQ, ERROR,
				"[CSI] Invalid csi output method %d\n",
				prCSICtrl->ucValue1);
	}

	if (tb[WIFI_ATTRIBUTE_CSI_VALUE_2]) {
		prCSICtrl->u4Value2 =
			nla_get_u32(tb[WIFI_ATTRIBUTE_CSI_VALUE_2]);
		prCSIInfo->u4Value2[prCSICtrl->ucCfgItem] = prCSICtrl->u4Value2;
	}

send_cmd:
	DBGLOG(REQ, STATE,
	   "[CSI] Set band idx %d, mode %d, csi cfg item %d, value1 %d, value2 %d",
		prCSICtrl->ucBandIdx,
		prCSICtrl->ucMode, prCSICtrl->ucCfgItem,
		prCSICtrl->ucValue1, prCSICtrl->u4Value2);

	rStatus = kalIoctl(prGlueInfo, wlanoidSetCSIControl, prCSICtrl,
				sizeof(struct CMD_CSI_CONTROL_T), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR,
			"[CSI] send CSI control cmd failed, rStatus %u\n",
			rStatus);
		i4Status = -EFAULT;
	}

out:
	if (prCSICtrl)
		kalMemFree(prCSICtrl, VIR_MEM_TYPE,
				sizeof(struct CMD_CSI_CONTROL_T));
	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is to send FWK a event of CSI raw data.
 *
 * \retval 0 Success.
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_event_csi_raw_data(
	struct ADAPTER *prAdapter)
{
	struct wiphy *wiphy = NULL;
	struct wireless_dev *wdev = NULL;
	struct wireless_dev **pprWdev = NULL;
	struct CSI_INFO_T *prCSIInfo = NULL;
	struct CSI_DATA_T *prTempCSIData = NULL;
	struct sk_buff *skb = NULL;
	uint8_t *temp;
	int32_t i4Pos = 0;
	u_int8_t bStatus;

	if (!prAdapter || !prAdapter->prGlueInfo)
		return -EINVAL;

	pprWdev = wlanGetWirelessDevice(prAdapter->prGlueInfo);

	if (!pprWdev) {
		DBGLOG(REQ, ERROR, "pprWdev is NULL\n");
		return -EINVAL;
	}

	wdev = pprWdev[0];
	if (!wdev || !wdev->netdev || !wdev->wiphy) {
		DBGLOG(REQ, ERROR,
			"[CSI] %s wrong input parameters\n", __func__);
		return -EINVAL;
	}
	wiphy = wdev->wiphy;

	prCSIInfo = glCsiGetCSIInfo(prAdapter->prGlueInfo);
	prTempCSIData = glCsiGetCSIData(prAdapter->prGlueInfo);
	temp = glCsiGetCSIBuf(prAdapter->prGlueInfo);

	if (!prTempCSIData) {
		DBGLOG(REQ, ERROR, "[CSI] NULL CSI data.\n");
		return -EINVAL;
	}

	bStatus = wlanPopCSIData(prAdapter, prTempCSIData);
	if (!bStatus)
		return 0;
	i4Pos = wlanCSIDataPrepare(temp,  prCSIInfo, prTempCSIData);

	skb = cfg80211_vendor_event_alloc(wiphy,
#if KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE
			wdev,
#endif
			i4Pos,
			WIFI_EVENT_SUBCMD_CSI,
			GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "[CSI] %s allocate skb failed\n", __func__);
		goto err_handle;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_CSI,
				i4Pos, temp) < 0)) {
		DBGLOG(REQ, ERROR, "[CSI] nla_put failure: len=%u, ptr=%p\n",
		       i4Pos, temp);
		goto err_handle;
	}

#if CFG_CSI_DEBUG
	DBGLOG(REQ, DEBUG,
		"[CSI] copy size = %d, [used|head idx|tail idx] = [%d|%d|%d]\n",
		i4Pos, prCSIInfo->u4CSIBufferUsed,
		prCSIInfo->u4CSIBufferHead, prCSIInfo->u4CSIBufferTail);
#endif

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return 0;

err_handle:
	if (skb)
		kfree_skb(skb);

	return -ENOMEM;
}
#endif

int mtk_cfg80211_vendor_p2p_listen_offload_start(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	const void *data,
	int data_len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX + 1] = {};
	struct MSG_P2P_LISTEN_OFFLOAD *prMsg;
	uint32_t msg_size;
	uint8_t *buf;

	if (!wiphy || !wdev || !data || !data_len) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (NLA_PARSE(tb, QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX,
		data,
		data_len,
		nla_p2p_listen_offload_policy)) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	if (!tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_CHANNEL] ||
		!tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_PERIOD] ||
		!tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_INTERVAL] ||
		!tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_COUNT] ||
		!tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_DEVICE_TYPES] ||
		!tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_VENDOR_IE]) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	msg_size = sizeof(struct MSG_P2P_LISTEN_OFFLOAD);
	prMsg = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, msg_size);
	if (prMsg == NULL) {
		DBGLOG(REQ, ERROR, "allocate msg req. fail.\n");
		rStatus = -ENOMEM;
		goto exit;
	}

	kalMemSet(prMsg, 0, msg_size);

	prMsg->rMsgHdr.eMsgId = MID_MNY_P2P_LISTEN_OFFLOAD_START;
	prMsg->rInfo.ucBssIndex =
		prGlueInfo->prAdapter->ucP2PDevBssIdx;
	prMsg->rInfo.u4DevId = 0;
	prMsg->rInfo.u4flags = 1;

	if (tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_CTRL_FLAG])
		prMsg->rInfo.u4flags =
		nla_get_u32(tb[QCA_WLAN_VENDOR_ATTR_P2P_LO_CTRL_FLAG]);

	prMsg->rInfo.u4Freq = nla_get_u32(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_CHANNEL]);
	if ((prMsg->rInfo.u4Freq != 2412) &&
		(prMsg->rInfo.u4Freq != 2437) &&
		(prMsg->rInfo.u4Freq != 2462)) {
		DBGLOG(REQ, ERROR,
			"Invalid listening channel: %d",
			prMsg->rInfo.u4Freq);
		rStatus = -EINVAL;
		goto exit;
	}

	prMsg->rInfo.u4Period = nla_get_u32(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_PERIOD]);
	if (!((prMsg->rInfo.u4Period > 0) &&
		(prMsg->rInfo.u4Period < UINT_MAX))) {
		rStatus = -EINVAL;
		goto exit;
	}

	prMsg->rInfo.u4Interval = nla_get_u32(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_INTERVAL]);
	if (!((prMsg->rInfo.u4Interval > 0) &&
		(prMsg->rInfo.u4Interval < UINT_MAX))) {
		rStatus = -EINVAL;
		goto exit;
	}

	prMsg->rInfo.u4Count = nla_get_u32(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_COUNT]);
	if (!(prMsg->rInfo.u4Count < UINT_MAX)) {
		rStatus = -EINVAL;
		goto exit;
	}

	prMsg->rInfo.u2DevLen = nla_len(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_DEVICE_TYPES]);
	if (!(prMsg->rInfo.u2DevLen < MAX_UEVENT_LEN)) {
		DBGLOG(REQ, ERROR, "Invalid u2DevLen");
		rStatus = -EINVAL;
		goto exit;
	}

	buf = nla_data(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_DEVICE_TYPES]);
	if (!buf) {
		DBGLOG(REQ, ERROR, "Invalid aucDevice");
		rStatus = -EINVAL;
		goto exit;
	} else
		kalMemCopy(&prMsg->rInfo.aucDevice,
			buf, prMsg->rInfo.u2DevLen);

	prMsg->rInfo.u2IELen = nla_len(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_VENDOR_IE]);
	if (prMsg->rInfo.u2IELen > MAX_IE_LENGTH) {
		DBGLOG(REQ, ERROR, "Invalid u2IELen");
		rStatus = -EINVAL;
		goto exit;
	}

	buf = nla_data(tb
		[QCA_WLAN_VENDOR_ATTR_P2P_LO_VENDOR_IE]);
	if (!buf) {
		DBGLOG(REQ, ERROR, "Invalid aucIE");
		rStatus = -EINVAL;
		goto exit;
	} else
		kalMemCopy(&prMsg->rInfo.aucIE,
			buf, prMsg->rInfo.u2IELen);

	DBGLOG(REQ, DEBUG,
		"p2p_lo, f: %d, period: %d, interval: %d, count: %d",
		prMsg->rInfo.u4Freq,
		prMsg->rInfo.u4Period,
		prMsg->rInfo.u4Interval,
		prMsg->rInfo.u4Count);

	mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsg,
			MSG_SEND_METHOD_BUF);

exit:

	return rStatus;
}

int mtk_cfg80211_vendor_p2p_listen_offload_stop(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	const void *data,
	int data_len)
{
	struct GLUE_INFO *prGlueInfo;
	struct MSG_P2P_LISTEN_OFFLOAD *prMsg;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t msg_size;

	if (!wiphy || !wdev) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		rStatus = -EFAULT;
		goto exit;
	}

	msg_size = sizeof(struct MSG_P2P_LISTEN_OFFLOAD);
	prMsg = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, msg_size);
	if (prMsg == NULL) {
		DBGLOG(REQ, ERROR, "allocate msg req. fail.\n");
		rStatus = -ENOMEM;
		goto exit;
	}

	kalMemSet(prMsg, 0, msg_size);
	prMsg->rMsgHdr.eMsgId = MID_MNY_P2P_LISTEN_OFFLOAD_STOP;
	prMsg->rInfo.ucBssIndex =
		prGlueInfo->prAdapter->ucP2PDevBssIdx;

	DBGLOG(REQ, DEBUG, "p2p_lo stop");

	mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsg,
			MSG_SEND_METHOD_BUF);

exit:

	return rStatus;
}

#if CFG_SUPPORT_PASN
int mtk_cfg80211_vendor_pasn(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	const void *data,
	int data_len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *attr;
	struct nlattr *tb
		[QCA_WLAN_VENDOR_ATTR_PASN_MAX + 1] = {};
	struct nlattr *cfg
		[QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAX + 1] = {};
	unsigned int n_peers = 0, idx = 0;
	int rem;
	struct MSG_PASN_RESP *prMsg;
	uint32_t msg_size;
	uint8_t *buf;

	if (!wiphy || !wdev || !data || !data_len) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (NLA_PARSE(tb, QCA_WLAN_VENDOR_ATTR_PASN_MAX,
		data,
		data_len,
		nla_pasn_policy)) {
		DBGLOG(REQ, ERROR, "NLP_PARSE failed\n");
		rStatus = -EINVAL;
		goto exit;
	}

	if (!tb[QCA_WLAN_VENDOR_ATTR_PASN_ACTION] &&
		!tb[QCA_WLAN_VENDOR_ATTR_PASN_PEERS]) {
		DBGLOG(REQ, ERROR, "Invalid ATTR\n");
		rStatus = -EINVAL;
		goto exit;
	}

	msg_size = sizeof(struct MSG_PASN_RESP);
	prMsg = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, msg_size);
	if (prMsg == NULL) {
		DBGLOG(REQ, ERROR, "allocate msg req. fail.\n");
		rStatus = -ENOMEM;
		goto exit;
	}

	kalMemSet(prMsg, 0, msg_size);

	prMsg->rMsgHdr.eMsgId = MID_PASN_RESP;

	nla_for_each_nested(attr, tb[QCA_WLAN_VENDOR_ATTR_PASN_PEERS], rem)
		n_peers++;

	if (n_peers > PASN_MAX_PEERS) {
		DBGLOG(REQ, ERROR, "Too many peers: %d", n_peers);
		rStatus = -EINVAL;
		goto exit;
	}

	nla_for_each_nested(attr, tb[QCA_WLAN_VENDOR_ATTR_PASN_PEERS], rem) {
		struct nlattr *nl_src, *nl_peer, *nl_success;

		if (NLA_PARSE_NESTED(cfg, QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAX,
			attr, nla_pasn_peer_policy)) {
			DBGLOG(REQ, ERROR, "Invalid peer ATTR.\n");
			rStatus = -EINVAL;
			goto exit;
		}

		nl_src = cfg[QCA_WLAN_VENDOR_ATTR_PASN_PEER_SRC_ADDR];
		nl_peer = cfg[QCA_WLAN_VENDOR_ATTR_PASN_PEER_MAC_ADDR];
		nl_success = cfg[QCA_WLAN_VENDOR_ATTR_PASN_PEER_STATUS_SUCCESS];

		if (nl_src) {
			buf = nla_data(nl_src);
			COPY_MAC_ADDR(
				prMsg->rPasnRespEvt.arPeer[idx].aucOwnAddr,
				buf);
		}

		if (nl_peer) {
			buf = nla_data(nl_peer);
			COPY_MAC_ADDR(
				prMsg->rPasnRespEvt.arPeer[idx].aucPeerAddr,
				buf);
		}

		prMsg->rPasnRespEvt.arPeer[idx].eStatus = nl_success ?
			PASN_STATUS_SUCCESS : PASN_STATUS_FAILURE;

		DBGLOG(REQ, DEBUG,
			"src:"MACSTR", peer:"MACSTR", success=%d\n",
			MAC2STR(prMsg->rPasnRespEvt.arPeer[idx].aucOwnAddr),
			MAC2STR(prMsg->rPasnRespEvt.arPeer[idx].aucPeerAddr),
			prMsg->rPasnRespEvt.arPeer[idx].eStatus);

		idx++;
	}
	prMsg->rPasnRespEvt.ucNumPeers = n_peers;

	DBGLOG(REQ, DEBUG, "pasn_resp, peers: %d\n", n_peers);

	mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsg,
			MSG_SEND_METHOD_BUF);

exit:

	return rStatus;
}

int mtk_cfg80211_vendor_secure_ranging_ctx(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	const void *data,
	int data_len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *tb
		[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_MAX + 1] = {};
	struct MSG_PASN_SECURE_RANGING_CTX *prMsg;
	uint32_t msg_size;
	uint8_t ucTkLen, ucLtfKeyseedLen;
	uint8_t *buf;

	if (!wiphy || !wdev || !data || !data_len) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (NLA_PARSE(tb, QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_MAX,
		data,
		data_len,
		nla_secure_ranging_ctx_policy)) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	if (!tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_ACTION] ||
		!tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SRC_ADDR] ||
		!tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_PEER_MAC_ADDR]) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	msg_size = sizeof(struct MSG_PASN_SECURE_RANGING_CTX);

	prMsg = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, msg_size);
	if (prMsg == NULL) {
		DBGLOG(REQ, ERROR, "allocate msg req. fail.\n");
		rStatus = -ENOMEM;
		goto exit;
	}

	kalMemSet(prMsg, 0, msg_size);

	prMsg->rMsgHdr.eMsgId = MID_PASN_SECURE_RANGING_CTX;

	prMsg->rRangingCtx.u4Action = nla_get_u32(
		tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_ACTION]);

	buf = nla_data(
		tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SRC_ADDR]);
	COPY_MAC_ADDR(prMsg->rRangingCtx.aucOwnAddr, buf);

	buf = nla_data(
		tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_PEER_MAC_ADDR]);
	COPY_MAC_ADDR(prMsg->rRangingCtx.aucPeerAddr, buf);

	if (tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_CIPHER]) {
		prMsg->rRangingCtx.u4Cipher = nla_get_u32(
			tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_CIPHER]);
	}

	if (tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SHA_TYPE]) {
		prMsg->rRangingCtx.u4ShaType = nla_get_u32(
			tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_SHA_TYPE]);
	}

	if (tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_TK]) {
		buf = nla_data(tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_TK]);
		ucTkLen = nla_len(
			tb[QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_TK]);

		if (ucTkLen <= 32) {
			prMsg->rRangingCtx.ucTkLen = ucTkLen;
			kalMemCopy(prMsg->rRangingCtx.aucTk, buf,
				prMsg->rRangingCtx.ucTkLen);
		} else {
			DBGLOG(REQ, ERROR,
				"Exceed max TK length = %d\n",
				ucTkLen);
		}
	}

#define CTX_LTF_KEYSEED QCA_WLAN_VENDOR_ATTR_SECURE_RANGING_CTX_LTF_KEYSEED
	if (tb[CTX_LTF_KEYSEED]) {
		buf = nla_data(tb[CTX_LTF_KEYSEED]);
		ucLtfKeyseedLen = nla_len(tb[CTX_LTF_KEYSEED]);

		if (ucLtfKeyseedLen <= 48) {
			prMsg->rRangingCtx.ucLtfKeyseedLen = ucLtfKeyseedLen;
			kalMemCopy(prMsg->rRangingCtx.aucLtfKeyseed, buf,
				prMsg->rRangingCtx.ucLtfKeyseedLen);
		} else {
			DBGLOG(REQ, ERROR,
				"Exceed max LTF keyseed length = %d\n",
				ucLtfKeyseedLen);
		}
	}

	DBGLOG(REQ, DEBUG,
		"Ranging ctx, action=%d, src=" MACSTR ", peer=" MACSTR "\n",
		prMsg->rRangingCtx.u4Action,
		MAC2STR(prMsg->rRangingCtx.aucOwnAddr),
		MAC2STR(prMsg->rRangingCtx.aucPeerAddr));

	DBGLOG(REQ, DEBUG,
		"Ranging ctx, TK len=%d, LTF keyseed len=%d, Cipher=0x%x\n",
		prMsg->rRangingCtx.ucTkLen,
		prMsg->rRangingCtx.ucLtfKeyseedLen,
		prMsg->rRangingCtx.u4Cipher);

	mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsg,
			MSG_SEND_METHOD_BUF);
exit:
	return rStatus;
}

#endif /* CFG_SUPPORT_PASN */

int mtk_cfg80211_vendor_connect_ext(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	const void *data,
	int data_len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_MAX + 1] = {};
	uint8_t *buf;
	uint16_t len;

	if (!wiphy || !wdev || !data || !data_len) {
		DBGLOG(REQ, ERROR, "input data null.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "get glue structure fail.\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		rStatus = -EFAULT;
		goto exit;
	}

	if (NLA_PARSE(tb, QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_MAX,
		data,
		data_len,
		nla_connect_ext_policy)) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	if (!tb[QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_FEATURES]) {
		DBGLOG(REQ, ERROR, "Invalid ATTR.\n");
		rStatus = -EINVAL;
		goto exit;
	}

	buf = nla_data(tb[QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_FEATURES]);
	len = nla_len(tb[QCA_WLAN_VENDOR_ATTR_CONNECT_EXT_FEATURES]);

	DBGDUMP_MEM8(INIT, INFO, "connect_ext features", buf, len);

#if (CFG_SUPPORT_RSNO == 1)
	if (len > 0)
		prGlueInfo->prAdapter->rWifiVar.fgEnStaRSNO =
			!!(buf[0] & BIT(QCA_CONNECT_EXT_FEATURE_RSNO));
#endif /* CFG_SUPPORT_RSNO */

exit:
	return rStatus;
}

