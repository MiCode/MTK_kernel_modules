/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Log: gl_vendor_nan.h
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_wext.h"
#include "wlan_lib.h"
#include <linux/can/netlink.h>
#include <linux/ieee80211.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>
#include <net/cfg80211.h>
#include <net/netlink.h>

#if CFG_SUPPORT_NAN

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

/*******************************************************************************
 *				P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define PACKED __packed
/* 8-byte control message header used by NAN */
struct _NanMsgHeader {
	u16 msgVersion : 4;
	u16 msgId : 12;
	u16 msgLen;
	u16 handle;
	u16 transactionId;
} PACKED;

struct _NanTlv  {
	u16 type;
	u16 length;
	u8 *value;
} PACKED;

/* NAN Enable Rsp */
struct NanEnableRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

/* NAN Disable Rsp */
struct NanDisableRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 reserved;
} PACKED;

/* Get Country Code Rsp */
struct NanGetCountryCodeRspMsg {
	struct _NanMsgHeader fwHeader;
	u8 countryCode[2];
} PACKED;

/* INFRA BSSID Rsp */
struct NanGetInfraBssidRspMsg {
	struct _NanMsgHeader fwHeader;
	/* BSSID of infra */
	u8 MacAddr[6];
} PACKED;

struct NanGetInfraChannelRspMsg {
	struct _NanMsgHeader fwHeader;
	/* BSSID of infra */
	u32 Channel;
	u32 Flag;
} PACKED;

struct NanDriverCapabilitiesRspMsg {
	struct _NanMsgHeader fwHeader;
	u16 capabilities; /* wfpal_wifi_driver_capability_flags_t */
} PACKED;

struct NanInfraAssocReadyIndMsg {
	struct _NanMsgHeader fwHeader;
	u16 status;
	u16 is_ipv6;
	u8 addressv4[4];
	u8 addressv6[16];
} PACKED;

/* NAN Configuration Rsp */
struct NanConfigRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

/* NAN Publish Response */
struct NanPublishServiceRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

/* NAN Publish Cancel Rsp */
struct NanPublishServiceCancelRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

/* NAN Subscribe Service Rsp */
struct NanSubscribeServiceRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

struct NanDfspConfigReqMsg {
	struct _NanMsgHeader fwHeader;
} PACKED;

struct NanSubscribeServiceCancelRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

/* NAN Transmit Followup Rsp */
struct NanTransmitFollowupRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 value;
} PACKED;

#if CFG_SUPPORT_NAN_EXT
#define NAN_MAX_EXT_DATA_SIZE 512

struct NanExtCmdMsg {
	struct _NanMsgHeader fwHeader;
	u8 data[NAN_MAX_EXT_DATA_SIZE];
} PACKED;

struct NanExtResponseMsg {
	struct _NanMsgHeader fwHeader;
	u8 data[NAN_MAX_EXT_DATA_SIZE];
} PACKED;

struct NanExtIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 data[NAN_MAX_EXT_DATA_SIZE];
} PACKED;
#endif /* CFG_SUPPORT_NAN_EXT */

/* NAN Capabilities Rsp */
struct NanCapabilitiesRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u32 status;
	u32 value;
	u32 max_concurrent_nan_clusters;
	u32 max_publishes;
	u32 max_subscribes;
	u32 max_service_name_len;
	u32 max_match_filter_len;
	u32 max_total_match_filter_len;
	u32 max_service_specific_info_len;
	u32 max_vsa_data_len;
	u32 max_mesh_data_len;
	u32 max_ndi_interfaces;
	u32 max_ndp_sessions;
	u32 max_app_info_len;
	u32 max_queued_transmit_followup_msgs;
	u32 ndp_supported_bands;
	u32 cipher_suites_supported;
	u32 max_scid_len;
	u32 is_ndp_security_supported : 1;
	u32 max_sdea_service_specific_info_len : 16;
	u32 is_instant_mode_supported : 1;
	u32 is_6g_supported : 1;
	u32 is_he_supported : 1;
	u32 is_pairing_supported : 1;
	u32 is_set_cluster_id_supported : 1;
	u32 is_suspension_supported : 1;
	u32 reserved1 : 4;
	u32 ndpe_attr_supported : 1;
	u32 reserved : 4;
	u32 max_subscribe_address;
} PACKED;

/* NAN Beacon Sdf Payload Rsp */
struct NanBeaconSdfPayloadRspMsg {
	struct _NanMsgHeader fwHeader;
	/* status of the request */
	u16 status;
	u16 reserved;
} PACKED;

/* Indication Part */
/* Params for NAN Publish Replied Ind */
struct _NanPublishRepliedIndParams  {u32 matchHandle; } PACKED;

/* NAN Publish Replied Ind */
struct NanPublishRepliedIndMsg {
	struct _NanMsgHeader fwHeader;
	struct _NanPublishRepliedIndParams publishRepliedIndParams;
	/*
     * Excludes TLVs
     *
	 * Required: MAC Address
	 * Optional: Received RSSI Value
	 *
	 */
	u8 ptlv[];
} PACKED;

/* NAN Publish Terminated Ind */
struct NanPublishTerminatedIndMsg {
	struct _NanMsgHeader fwHeader;
	/* reason for the termination */
	u16 reason;
	u16 reserved;
} PACKED;

/* NAN Subscribe Terminated Ind */
struct NanSubscribeTerminatedIndMsg {
	struct _NanMsgHeader fwHeader;
	/* reason for the termination */
	u16 reason;
	u16 reserved;
} PACKED;

/* NAN Publish Followup Ind */
struct _NanFollowupIndParams {
	u32 matchHandle;
	u32 window : 1;
	u32 reserved : 31;
	/*
	 * Excludes TLVs
	 *
	 * Required: Service Specific Info or
	 * Extended Service Specific Info
	 */
} PACKED;

struct NanFollowupIndMsg {
	struct _NanMsgHeader fwHeader;
	struct _NanFollowupIndParams followupIndParams;
	u8 ptlv[];
} PACKED;

struct NanSelfFollowupIndMsg {
	struct _NanMsgHeader fwHeader;
	u32 reason;
} PACKED;

/* Event Ind */
struct NanEventIndParams {
	u32 eventId : 8;
	u32 reserved : 24;
} PACKED;

struct NanEventIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 ptlv[];
} PACKED;

/* Disable Ind */
struct NanDisableIndMsg {
	struct _NanMsgHeader fwHeader;
	u16 reason;
	u16 reserved;
} PACKED;

/* NAN Subscribe Match Ind */
struct _NanMatchIndParams  {
	u32 matchHandle;
	u32 matchOccuredFlag : 1;
	u32 outOfResourceFlag : 1;
	u32 reserved : 30;
} PACKED;

struct NanMatchIndMsg {
	struct _NanMsgHeader fwHeader;
	struct _NanMatchIndParams matchIndParams;
	u8 ptlv[];
} PACKED;

/* NAN Match Expired Ind */
struct _NanmatchExpiredIndParams  {
	u32 matchHandle;
} PACKED;

struct NanMatchExpiredIndMsg {
	struct _NanMsgHeader fwHeader;
	struct _NanmatchExpiredIndParams matchExpiredIndParams;
} PACKED;


/* NAN Ranging Configuration params */
struct _NanFWGeoFenceDescriptor {
	u32 inner_threshold;
	u32 outer_threshold;
} PACKED;

struct NanFWRangeConfigParams {
	u32 range_resolution;
	u32 range_interval;
	u32 ranging_indication_event;
	struct _NanFWGeoFenceDescriptor geo_fence_threshold;
} PACKED;

/* 2 word representation of MAC addr */
struct _fw_mac_addr {
	/* upper 4 bytes of  MAC address */
	u32 mac_addr31to0;
	/* lower 2 bytes of  MAC address */
	u32 mac_addr47to32;
};

struct NanFWRangeReqMsg {
	struct _fw_mac_addr range_mac_addr;
	u32 range_id; /* Match handle in match_ind, publish_id in result ind */
	u32 ranging_accept : 1;
	u32 ranging_reject : 1;
	u32 ranging_cancel : 1;
	u32 reserved : 29;
} PACKED;

struct NanDebugParams {
    /* To indicate the debug command type. */
	u32 cmd;
    /* To hold the data for the above command
     * type.
     */
	u8 debug_cmd_data[NAN_MAX_DEBUG_MESSAGE_DATA_LEN];
} PACKED;

struct _NanAvailabilityTimeBitmap {
	u16 offset;
	u8 period;
	u8 bitDuration;
	u8 time_bitmap_length;
	u8 time_bitmap[NAN_MAX_AVAILABILITY_BITMAP_LENGTH];
} PACKED;

struct _NanChannelAvailabilityEntry {
	u8 usage_preference;
	u8 utilization;
	u8 rx_nss;
	struct _NanAvailabilityTimeBitmap time_bitmap;
	u8 op_class;
	u8 primary_channel_bitmap;
	u16 op_class_bitmap;
	u16 auxiliary_channel_bitmap;
} PACKED;

struct _NanCommittedChannelSchedule {
	u8 max_period;
	u8 num_entries;
	u8 map_id;
	struct _NanChannelAvailabilityEntry
		channel_entries[NAN_MAX_AVAILABILITY_CHANNEL_ENTRIES];
} PACKED;

struct _NanCommittedAvailability {
	u8 num_maps_ids;
	struct _NanCommittedChannelSchedule schedule[NAN_MAX_MAP_IDS];
} PACKED;

struct _NanPotentialChannelSchedule {
	u8 map_id;
	u8 num_band_entries;
	u8 band_ids[NAN_MAX_BAND_IDS];
	u8 num_entries;
	struct _NanChannelAvailabilityEntry
		channel_entries[NAN_MAX_AVAILABILITY_CHANNEL_ENTRIES];
} PACKED;

struct _NanPotentialAvailability {
	u8 num_maps_ids;
	struct _NanPotentialChannelSchedule potential[NAN_MAX_MAP_IDS];
} PACKED;

struct _NanDataClusterID {
	u8 octet[NAN_MAC_ADDR_LEN];
} PACKED;

struct _NanDataClusterAvailabilityParams {
	struct _NanDataClusterID ndc_id;
	u8 map_id;
	u8 selected;
	struct _NanAvailabilityTimeBitmap time_bitmap;
} PACKED;

struct _NanDataClusterAvailability {
	u8 num_maps_ids;
	struct _NanDataClusterAvailabilityParams ndc[NAN_MAX_MAP_IDS];
} PACKED;

struct _NanForcedDiscBeaconTxAvailabilityParams {
	u8 map_id;
	struct _NanAvailabilityTimeBitmap time_bitmap;
} PACKED;

struct _NanForcedDiscBeaconTxAvailability {
	u8 num_maps_ids;
	struct _NanForcedDiscBeaconTxAvailabilityParams slots[NAN_MAX_MAP_IDS];
} PACKED;

struct _NanForcedDiscBeaconTransmission {
	u8 enable;
	u32 beacon_interval;
	struct _NanForcedDiscBeaconTxAvailability availability;
	u8 reason;
} PACKED;

struct _NanDfspConfig {
	u16 version;
	u16 length;
	u16 flags; /* bit 0 = enable;no other defined */
	u16 max_bcn_miss_duration; /* duration of no beacon for suspension */
	u8 mcsp_ttl;
	u8 bcsa_cnt;
	u8 max_empty_aw;
	u16 mon_chan; /* passive monitor channel */
	u8 mon_bssid[NAN_MAC_ADDR_LEN]; /* bssid of the AP */
	u16 max_bcn_miss_af_duration;
};

struct DFSP_EVENT_CSA_T {
	uint16_t flags;
	uint8_t length;
	uint8_t dfs_tlv_data[];
} PACKED;

struct IE_DFS_EXT_CSA {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucChannelSwitchMode;
	uint8_t ucNewChannelNum;
	uint8_t ucChannelSwitchCount;
} PACKED;

struct IE_DFS_MESH_CSP {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucTimetoLive;
	uint8_t ucFlags;
	uint8_t u2ReasonCodes;
	uint8_t u2ProcedenceValue;
} PACKED;

struct IE_WIDE_BW_CS {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucNewChannelWidth;
	uint8_t ucChannelS1;
	uint8_t ucChannelS2;
} PACKED;

struct DFSP_COMBINED_EVENT_DATA_T {
	struct IE_DFS_EXT_CSA csa_data;
	struct IE_DFS_MESH_CSP mesh_csp;
	struct IE_WIDE_BW_CS wide_bw_cs;
} PACKED;

struct DFSP_EVENT_CSA_COMPLETE_T {
	uint8_t ucNewChannelNum;
} PACKED;

struct DFSP_SUSPEND_RESUME_T {
	uint16_t flags;
	uint8_t suspended;
	uint8_t resumed;
} PACKED;

struct NanDfspCsaIndMsg {
	struct _NanMsgHeader fwHeader;
	struct DFSP_EVENT_CSA_T csaInd;
} PACKED;

struct NanDfspCsaCompleteIndMsg {
	struct _NanMsgHeader fwHeader;
	struct DFSP_EVENT_CSA_COMPLETE_T csaCompleteInd;
} PACKED;

struct NanDfspSusResIndMsg {
	struct _NanMsgHeader fwHeader;
	struct DFSP_SUSPEND_RESUME_T susResInd;
} PACKED;

struct wfpal_channel {
	u32 channel;
	u32 flags;
} PACKED;

#define WFPAL_MAX_PMK_LENGTH 64
#define WFPAL_PMKID_LENGTH 16

struct NanInfraAuthRxIndMsg {
	struct _NanMsgHeader fwHeader;
	u32 status;
	u8 reason;
	u8 peer_mac[6];
	u32 pmk_len;
	u8 pmk[WFPAL_MAX_PMK_LENGTH];
	u32 pmkid_len;
	u8 pmkid[WFPAL_PMKID_LENGTH];
} PACKED;

struct NanInfraAssocStartIndMsg {
	struct _NanMsgHeader fwHeader;
	struct wfpal_channel channel;
} PACKED;

struct _NanWfpalJoinSubstateInfo {
	u8 bssid[6];
	u8 flags;
	u32 auth_status;
	u8 auth_reason;
	u32 assoc_status;
	u8 assoc_reason;
	u32 set_ssid_status;
	u8 set_ssid_reason;
	u32 supplicant_status;
	u8 supplicant_reason;
} PACKED;

#define WFPAL_MAX_ASSOC_BSSID 3
struct NanInfraAssocDoneIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 join_address[6];
	u32 return_val;
	u32 ieee_status;
	struct _NanWfpalJoinSubstateInfo substate_info[WFPAL_MAX_ASSOC_BSSID];
	u32 extension_return_val;
} PACKED;

struct NanInfraAssocReceivedIndMsg {
	struct _NanMsgHeader fwHeader;
	u32 status;
	u32 reason;
} PACKED;

#define WFPAL_MAX_CHANNELS 128
struct NanInfraScanStartIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 num_of_24G_channels;
	u8 num_of_5G_channels;
	u8 channel_list[WFPAL_MAX_CHANNELS];
} PACKED;

struct NanInfraScanCompleteIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 status;
} PACKED;

struct NanDwStartIndMsg {
	struct _NanMsgHeader fwHeader;
	struct wfpal_channel channel;
	u32 expected_tsf_l;
	u32 expected_tsf_h;
	u32 actual_tsf_l;
	u32 actual_tsf_h;
	u8 dw_num;
} PACKED;

struct NanRoleChangedIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 role;
	u8 hopcount;
} PACKED;

#define WFPAL_NAN_OUT_OF_BOUND_ACTION_FRAME_MAX_SIZE 960
struct NanOobAfRxIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 dst_addr[MAC_ADDR_LEN];
	u8 src_addr[MAC_ADDR_LEN];
	u8 bssid[MAC_ADDR_LEN];
	struct wfpal_channel channel;
	u8 rx_rssi;
	u16 length;
	u8 data[WFPAL_NAN_OUT_OF_BOUND_ACTION_FRAME_MAX_SIZE];
} PACKED;

struct NanChipRstIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 state;
} PACKED;

#define NAN_PRIV_CMD_MAX_SIZE 32
struct NanDrvPrivCmdWork {
	struct work_struct work;
	struct GLUE_INFO *prGlueInfo;
	u8 cmd[NAN_PRIV_CMD_MAX_SIZE];
} PACKED;

struct NanDrvPrivCmd {
	u8 cmd[NAN_PRIV_CMD_MAX_SIZE];
} PACKED;

struct NanBcnRxIndMsg {
	struct _NanMsgHeader fwHeader;
	s32 rssi;
	struct wfpal_channel channel;
	u8 rate;
	u32 local_tsf_offset_h;
	u32 local_tsf_offset_l;
	u16 length;
	u8 frame[];
} PACKED;

#define NAN_CHANNEL_REPORT_MAX_SIZE 128
struct NanCountryCodeChangedIndMsg {
	struct _NanMsgHeader fwHeader;
	u8 countryCode[4];
	u32 channel_num;
	struct wfpal_channel channel[NAN_CHANNEL_REPORT_MAX_SIZE];
} PACKED;

enum nan_driver_capability_flags {
	WFPAL_WIFI_DRIVER_SUPPORTS_NAN = (1 << 0),
	WFPAL_WIFI_DRIVER_SUPPORTS_DUAL_BAND = (1 << 2),
	WFPAL_WIFI_DRIVER_SUPPORTS_SIMULTANEOUS_DUAL_BAND = (1 << 3),
	WFPAL_WIFI_DRIVER_IS_CLM_RESTRICTED = (1 << 4),
	WFPAL_WIFI_DRIVER_PEER_COUNTRY_CODE_LEARNING_CAPABLE = (1 << 5),
} PACKED;

extern const struct nla_policy mtk_wlan_vendor_nan_policy[NL80211_ATTR_MAX + 1];

/* Service Discovery Extended Attribute params Format to HAL */
struct NanFWSdeaCtrlParams {
	u32 fsd_required : 1;
	u32 fsd_with_gas : 1;
	u32 data_path_required : 1;
	u32 data_path_type : 1;
	u32 multicast_type : 1;
	u32 qos_required : 1;
	u32 security_required : 1;
	u32 ranging_required : 1;
	u32 range_limit_present : 1;
	u32 service_update_ind_present : 1;
	u32 reserved1 : 6;
	u32 range_report : 1;
	u32 reserved2 : 15;
} PACKED;

enum NanMsgId {
	NAN_MSG_ID_ERROR_RSP = 0,
	NAN_MSG_ID_CONFIGURATION_REQ = 1,
	NAN_MSG_ID_CONFIGURATION_RSP = 2,
	NAN_MSG_ID_PUBLISH_SERVICE_REQ = 3,
	NAN_MSG_ID_PUBLISH_SERVICE_RSP = 4,
	NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_REQ = 5,
	NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_RSP = 6,
	NAN_MSG_ID_PUBLISH_REPLIED_IND = 7,
	NAN_MSG_ID_PUBLISH_TERMINATED_IND = 8,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_REQ = 9,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_RSP = 10,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_REQ = 11,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_RSP = 12,
	NAN_MSG_ID_MATCH_IND = 13,
	NAN_MSG_ID_MATCH_EXPIRED_IND = 14,
	NAN_MSG_ID_SUBSCRIBE_TERMINATED_IND = 15,
	NAN_MSG_ID_DE_EVENT_IND = 16,
	NAN_MSG_ID_TRANSMIT_FOLLOWUP_REQ = 17,
	NAN_MSG_ID_TRANSMIT_FOLLOWUP_RSP = 18,
	NAN_MSG_ID_FOLLOWUP_IND = 19,
	NAN_MSG_ID_STATS_REQ = 20,
	NAN_MSG_ID_STATS_RSP = 21,
	NAN_MSG_ID_ENABLE_REQ = 22,
	NAN_MSG_ID_ENABLE_RSP = 23,
	NAN_MSG_ID_DISABLE_REQ = 24,
	NAN_MSG_ID_DISABLE_RSP = 25,
	NAN_MSG_ID_DISABLE_IND = 26,
	NAN_MSG_ID_TCA_REQ = 27,
	NAN_MSG_ID_TCA_RSP = 28,
	NAN_MSG_ID_TCA_IND = 29,
	NAN_MSG_ID_BEACON_SDF_REQ = 30,
	NAN_MSG_ID_BEACON_SDF_RSP = 31,
	NAN_MSG_ID_BEACON_SDF_IND = 32,
	NAN_MSG_ID_CAPABILITIES_REQ = 33,
	NAN_MSG_ID_CAPABILITIES_RSP = 34,
	NAN_MSG_ID_SELF_TRANSMIT_FOLLOWUP_IND = 35,
	NAN_MSG_ID_RANGING_REQUEST_RECEVD_IND = 36,
	NAN_MSG_ID_RANGING_RESULT_IND = 37,
	NAN_MSG_ID_GET_INFRA_BSSID = 38,
	NAN_MSG_ID_GET_INFRA_CHANNEL = 39,
	NAN_MSG_ID_INFRA_CHANGED_IND = 40,
	NAN_MSG_ID_SET_COMMITTED_AVAILABILITY = 41,
	NAN_MSG_ID_SET_POTENTIAL_AVAILABILITY = 42,
	NAN_MSG_ID_SET_DATA_CLUSTER_AVAILABILITY = 43,
	NAN_MSG_ID_UPDATE_DFSP_CONFIG = 44,
	NAN_MSG_ID_UPDATE_DFSP_CSA_IND = 45,
	NAN_MSG_ID_UPDATE_DFSP_CSA_COMPLETE_IND = 46,
	NAN_MSG_ID_UPDATE_DFSP_SUSPEND_RESUME_IND = 47,
	NAN_MSG_ID_INFRA_ASSOC_START_IND = 48,
	NAN_MSG_ID_INFRA_ASSOC_DONE_IND = 49,
	NAN_MSG_ID_INFRA_ASSOC_RECEIVED_IND = 50,
	NAN_MSG_ID_INFRA_SCAN_START_IND = 51,
	NAN_MSG_ID_INFRA_SCAN_COMPLETE_IND = 52,
	NAN_MSG_ID_DW_START_IND = 53,
	NAN_MSG_ID_INFRA_ASSOC_READY_IND = 54,
	NAN_MSG_ID_FORCED_BEACON_TRANSMISSION = 55,
	NAN_MSG_ID_ROLE_CHANGED_IND = 56,
	NAN_MSG_ID_OOB_AF_RX_IND = 57,
	NAN_MSG_ID_UPDATE_CUSTOM_ATTRIBUTE = 58,
	NAN_MSG_ID_BEACON_RX_IND = 59,
	NAN_MSG_ID_INFRA_AUTH_RX_IND = 60,
	NAN_MSG_ID_CHIP_RST_IND = 61,
	NAN_MSG_ID_COUNTRY_CODE_CHANGED = 62,
	NAN_MSG_ID_GET_COUNTRY_CODE = 63,
	NAN_MSG_ID_GET_DRIVER_CAPABILITIES = 64,
	NAN_MSG_ID_EXT_CMD = 92,
	NAN_MSG_ID_EXT_IND = 93,
	NAN_MSG_ID_PRIV_CMD = 1024,
	NAN_MSG_ID_TESTMODE_REQ = 1025,
	NAN_MSG_ID_TESTMODE_RSP = 1026
};

/* Various TLV Type ID sent as part of NAN Stats Response
 * or NAN TCA Indication
 */
enum NanTlvType {
	NAN_TLV_TYPE_FIRST = 0,

	/* Service Discovery Frame types */
	NAN_TLV_TYPE_SDF_FIRST = NAN_TLV_TYPE_FIRST,
	NAN_TLV_TYPE_SERVICE_NAME = NAN_TLV_TYPE_SDF_FIRST,
	NAN_TLV_TYPE_SDF_MATCH_FILTER,
	NAN_TLV_TYPE_TX_MATCH_FILTER,
	NAN_TLV_TYPE_RX_MATCH_FILTER,
	NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO,
	NAN_TLV_TYPE_EXT_SERVICE_SPECIFIC_INFO = 5,
	NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_TRANSMIT = 6,
	NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_RECEIVE = 7,
	NAN_TLV_TYPE_POST_NAN_CONNECTIVITY_CAPABILITIES_RECEIVE = 8,
	NAN_TLV_TYPE_POST_NAN_DISCOVERY_ATTRIBUTE_RECEIVE = 9,
	NAN_TLV_TYPE_BEACON_SDF_PAYLOAD_RECEIVE = 10,
	NAN_TLV_TYPE_NAN_DATA_PATH_PARAMS = 11,
	NAN_TLV_TYPE_NAN_DATA_SUPPORTED_BAND = 12,
	NAN_TLV_TYPE_2G_COMMITTED_DW = 13,
	NAN_TLV_TYPE_5G_COMMITTED_DW = 14,
	NAN_TLV_TYPE_NAN_DATA_RESPONDER_MODE = 15,
	NAN_TLV_TYPE_NAN_DATA_ENABLED_IN_MATCH = 16,
	NAN_TLV_TYPE_NAN_SERVICE_ACCEPT_POLICY = 17,
	NAN_TLV_TYPE_NAN_CSID = 18,
	NAN_TLV_TYPE_NAN_SCID = 19,
	NAN_TLV_TYPE_NAN_PMK = 20,
	NAN_TLV_TYPE_SDEA_CTRL_PARAMS = 21,
	NAN_TLV_TYPE_NAN_RANGING_CFG = 22,
	NAN_TLV_TYPE_CONFIG_DISCOVERY_INDICATIONS = 23,
	NAN_TLV_TYPE_NAN20_RANGING_REQUEST = 24,
	NAN_TLV_TYPE_NAN20_RANGING_RESULT = 25,
	NAN_TLV_TYPE_NAN20_RANGING_REQUEST_RECEIVED = 26,
	NAN_TLV_TYPE_NAN_PASSPHRASE = 27,
	NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO = 28,
	NAN_TLV_TYPE_DEV_CAP_ATTR_CAPABILITY = 29,
	NAN_TLV_TYPE_EXT_CMD = 92,
	NAN_TLV_TYPE_SDF_LAST = 4095,

	/* Configuration types */
	NAN_TLV_TYPE_CONFIG_FIRST = 4096,
	NAN_TLV_TYPE_24G_SUPPORT = NAN_TLV_TYPE_CONFIG_FIRST,
	NAN_TLV_TYPE_24G_BEACON,
	NAN_TLV_TYPE_24G_SDF,
	NAN_TLV_TYPE_24G_RSSI_CLOSE,
	NAN_TLV_TYPE_24G_RSSI_MIDDLE = 4100,
	NAN_TLV_TYPE_24G_RSSI_CLOSE_PROXIMITY,
	NAN_TLV_TYPE_5G_SUPPORT,
	NAN_TLV_TYPE_5G_BEACON,
	NAN_TLV_TYPE_5G_SDF,
	NAN_TLV_TYPE_5G_RSSI_CLOSE,
	NAN_TLV_TYPE_5G_RSSI_MIDDLE,
	NAN_TLV_TYPE_5G_RSSI_CLOSE_PROXIMITY,
	NAN_TLV_TYPE_SID_BEACON,
	NAN_TLV_TYPE_HOP_COUNT_LIMIT,
	NAN_TLV_TYPE_MASTER_PREFERENCE = 4110,
	NAN_TLV_TYPE_CLUSTER_ID_LOW,
	NAN_TLV_TYPE_CLUSTER_ID_HIGH,
	NAN_TLV_TYPE_RSSI_AVERAGING_WINDOW_SIZE,
	NAN_TLV_TYPE_CLUSTER_OUI_NETWORK_ID,
	NAN_TLV_TYPE_SOURCE_MAC_ADDRESS,
	NAN_TLV_TYPE_CLUSTER_ATTRIBUTE_IN_SDF,
	NAN_TLV_TYPE_SOCIAL_CHANNEL_SCAN_PARAMS,
	NAN_TLV_TYPE_DEBUGGING_FLAGS,
	NAN_TLV_TYPE_POST_NAN_CONNECTIVITY_CAPABILITIES_TRANSMIT,
	NAN_TLV_TYPE_POST_NAN_DISCOVERY_ATTRIBUTE_TRANSMIT = 4120,
	NAN_TLV_TYPE_FURTHER_AVAILABILITY_MAP,
	NAN_TLV_TYPE_HOP_COUNT_FORCE,
	NAN_TLV_TYPE_RANDOM_FACTOR_FORCE,
	NAN_TLV_TYPE_RANDOM_UPDATE_TIME = 4124,
	NAN_TLV_TYPE_EARLY_WAKEUP,
	NAN_TLV_TYPE_PERIODIC_SCAN_INTERVAL,
	NAN_TLV_TYPE_DW_INTERVAL = 4128,
	NAN_TLV_TYPE_DB_INTERVAL,
	NAN_TLV_TYPE_FURTHER_AVAILABILITY,
	NAN_TLV_TYPE_24G_CHANNEL,
	NAN_TLV_TYPE_5G_CHANNEL,
	NAN_TLV_TYPE_DISC_MAC_ADDR_RANDOM_INTERVAL,
	NAN_TLV_TYPE_RANGING_AUTO_RESPONSE_CFG = 4134,
	NAN_TLV_TYPE_SUBSCRIBE_SID_BEACON = 4135,
	NAN_TLV_TYPE_DW_EARLY_TERMINATION = 4136,
	NAN_TLV_TYPE_TX_RX_CHAINS = 4137,
	NAN_TLV_TYPE_ENABLE_DEVICE_RANGING = 4138,
	NAN_TLV_TYPE_ENABLE_INSTANT_MODE = 4140,
	NAN_TLV_TYPE_ENABLE_INSTANT_MODE_CHANNEL = 4141,
	NAN_TLV_TYPE_CONFIG_LAST = 8191,

	/* Attributes types */
	NAN_TLV_TYPE_ATTRS_FIRST = 8192,
	NAN_TLV_TYPE_AVAILABILITY_INTERVALS_MAP = NAN_TLV_TYPE_ATTRS_FIRST,
	NAN_TLV_TYPE_WLAN_MESH_ID,
	NAN_TLV_TYPE_MAC_ADDRESS,
	NAN_TLV_TYPE_RECEIVED_RSSI_VALUE,
	NAN_TLV_TYPE_CLUSTER_ATTRIBUTE,
	NAN_TLV_TYPE_WLAN_INFRA_SSID,
	NAN_TLV_TYPE_ATTRS_LAST = 12287,

	/* Events Type */
	NAN_TLV_TYPE_EVENTS_FIRST = 12288,
	NAN_TLV_TYPE_EVENT_SELF_STATION_MAC_ADDRESS = NAN_TLV_TYPE_EVENTS_FIRST,
	NAN_TLV_TYPE_EVENT_STARTED_CLUSTER,
	NAN_TLV_TYPE_EVENT_JOINED_CLUSTER,
	NAN_TLV_TYPE_EVENT_CLUSTER_SCAN_RESULTS,
	NAN_TLV_TYPE_FAW_MEM_AVAIL,
	NAN_TLV_TYPE_EVENTS_LAST = 16383,

	/* TCA types */
	NAN_TLV_TYPE_TCA_FIRST = 16384,
	NAN_TLV_TYPE_CLUSTER_SIZE_REQ = NAN_TLV_TYPE_TCA_FIRST,
	NAN_TLV_TYPE_CLUSTER_SIZE_RSP,
	NAN_TLV_TYPE_TCA_LAST = 32767,

	/* Statistics types */
	NAN_TLV_TYPE_STATS_FIRST = 32768,
	NAN_TLV_TYPE_DE_PUBLISH_STATS = NAN_TLV_TYPE_STATS_FIRST,
	NAN_TLV_TYPE_DE_SUBSCRIBE_STATS,
	NAN_TLV_TYPE_DE_MAC_STATS,
	NAN_TLV_TYPE_DE_TIMING_SYNC_STATS,
	NAN_TLV_TYPE_DE_DW_STATS,
	NAN_TLV_TYPE_DE_STATS,
	NAN_TLV_TYPE_STATS_LAST = 36863,

	/* Testmode types */
	NAN_TLV_TYPE_TESTMODE_FIRST = 36864,
	NAN_TLV_TYPE_TESTMODE_GENERIC_CMD = NAN_TLV_TYPE_TESTMODE_FIRST,
	NAN_TLV_TYPE_TESTMODE_LAST = 37000,

	NAN_TLV_TYPE_LAST = 65535
};

/* Definitions of debug subcommand type for the
 * generic debug command.
 * This debug command carries any one command type
 * followed by corresponding command data content
 * as indicated below.
 *
 * command: NAN_TEST_MODE_CMD_NAN_AVAILABILITY
 * content: NAN Avaiability attribute blob
 *
 * command: NAN_TEST_MODE_CMD_NDP_INCLUDE_IMMUTABLE
 * content: u32 value (0 - Ignore 1 - Include immuatable,
 *                     2 - Don't include immutable)
 *
 * command: NAN_TEST_MODE_CMD_NDP_AVOID_CHANNEL
 * content: u32 channel_frequency; (0 - Ignore)
 *
 * command: NAN_TEST_MODE_CMD_NAN_SUPPORTED_BANDS
 * content: u32 supported_bands; (0 . Ignore, 1 . 2g,
 *                                2 . 5g, 3 . 2g & 5g)
 *
 * command: NAN_TEST_MODE_CMD_AUTO_RESPONDER_MODE
 * content: u32 auto_resp_mode; (0 . Auto, 1 . Accept,
 *                               2 . Reject, 3 . Counter)
 *
 * command: NAN_TEST_MODE_CMD_M4_RESPONSE_TYPE
 * content: u32 m4_response_type; (0.Ignore, 1.Accept,
 *                                 2.Reject, 3.BadMic)
 *
 * command: NAN_TEST_MODE_CMD_NAN_SCHED_TYPE
 * content: u32 invalid_nan_schedule; (0. Valid sched,
 *                                     1.Invalid Sched bad FA,
 *                                     2.Invalid schedbad NDC,
 *                                     3.Invalid sched bad Immutable)
 *
 * command: NAN_TEST_MODE_CMD_NAN_NMF_CLEAR_CONFIG
 * content: u32 nmf_security_config_val;(0:NAN_NMF_CLEAR_DISABLE,
 *                                       1:NAN_NMF_CLEAR_ENABLE)
 *
 * command: NAN_TEST_MODE_CMD_NAN_SCHED_UPDATE_ULW_NOTIFY
 * content: u32 channel_availability;(0/1)
 *
 * command: NAN_TEST_MODE_CMD_NAN_SCHED_UPDATE_NDL_NEGOTIATE
 * content: responder_nmi_mac (Responder NMI Mac Address)
 *
 * command: NAN_TEST_MODE_CMD_NAN_SCHED_UPDATE_NDL_NOTIFY
 * content: NONE
 *
 * command: NAN_TEST_MODE_CMD_NAN_AVAILABILITY_MAP_ORDER
 * content: u32 map_order_val; (0/1)
 *
 */
enum NanDebugModeCmd {
	NAN_TEST_MODE_CMD_NAN_AVAILABILITY = 1,
	NAN_TEST_MODE_CMD_NDP_INCLUDE_IMMUTABLE = 2,
	NAN_TEST_MODE_CMD_NDP_AVOID_CHANNEL = 3,
	NAN_TEST_MODE_CMD_NAN_SUPPORTED_BANDS = 4,
	NAN_TEST_MODE_CMD_AUTO_RESPONDER_MODE = 5,
	NAN_TEST_MODE_CMD_M4_RESPONSE_TYPE = 6,
	NAN_TEST_MODE_CMD_NAN_SCHED_TYPE = 7,
	NAN_TEST_MODE_CMD_NAN_NMF_CLEAR_CONFIG = 8,
	NAN_TEST_MODE_CMD_NAN_SCHED_UPDATE_ULW_NOTIFY = 9,
	NAN_TEST_MODE_CMD_NAN_SCHED_UPDATE_NDL_NEGOTIATE = 10,
	NAN_TEST_MODE_CMD_NAN_SCHED_UPDATE_NDL_NOTIFY = 11,
	NAN_TEST_MODE_CMD_NAN_AVAILABILITY_MAP_ORDER = 12,
	NAN_TEST_MODE_CMD_CONFIG_QOS = 13,
	NAN_TEST_MODE_CMD_DEVICE_TYPE = 14,
	NAN_TEST_MODE_CMD_DISABLE_NDPE = 15,
	NAN_TEST_MODE_CMD_ENABLE_NDP = 16,
};

/* NAN Resp status type */
enum NanInternalStatusType {
	/* NAN Protocol Response Codes */
	NAN_I_STATUS_SUCCESS = 0,
	NAN_I_STATUS_TIMEOUT = 1,
	NAN_I_STATUS_DE_FAILURE = 2,
	NAN_I_STATUS_INVALID_MSG_VERSION = 3,
	NAN_I_STATUS_INVALID_MSG_LEN = 4,
	NAN_I_STATUS_INVALID_MSG_ID = 5,
	NAN_I_STATUS_INVALID_HANDLE = 6,
	NAN_I_STATUS_NO_SPACE_AVAILABLE = 7,
	NAN_I_STATUS_INVALID_PUBLISH_TYPE = 8,
	NAN_I_STATUS_INVALID_TX_TYPE = 9,
	NAN_I_STATUS_INVALID_MATCH_ALGORITHM = 10,
	NAN_I_STATUS_DISABLE_IN_PROGRESS = 11,
	NAN_I_STATUS_INVALID_TLV_LEN = 12,
	NAN_I_STATUS_INVALID_TLV_TYPE = 13,
	NAN_I_STATUS_MISSING_TLV_TYPE = 14,
	NAN_I_STATUS_INVALID_TOTAL_TLVS_LEN = 15,
	NAN_I_STATUS_INVALID_REQUESTER_INSTANCE_ID = 16,
	NAN_I_STATUS_INVALID_TLV_VALUE = 17,
	NAN_I_STATUS_INVALID_TX_PRIORITY = 18,
	NAN_I_STATUS_INVALID_CONNECTION_MAP = 19,
	NAN_I_STATUS_INVALID_THRESHOLD_CROSSING_ALERT_ID = 20,
	NAN_I_STATUS_INVALID_STATS_ID = 21,
	NAN_I_STATUS_NAN_NOT_ALLOWED = 22,
	NAN_I_STATUS_NO_OTA_ACK = 23,
	NAN_I_STATUS_TX_FAIL = 24,
	NAN_I_STATUS_NAN_ALREADY_ENABLED = 25,
	NAN_I_STATUS_FOLLOWUP_QUEUE_FULL = 26,
	/* 27-4095 Reserved */
	/* NAN Configuration Response codes */
	NAN_I_STATUS_INVALID_RSSI_CLOSE_VALUE = 4096,
	NAN_I_STATUS_INVALID_RSSI_MIDDLE_VALUE = 4097,
	NAN_I_STATUS_INVALID_HOP_COUNT_LIMIT = 4098,
	NAN_I_STATUS_INVALID_MASTER_PREFERENCE_VALUE = 4099,
	NAN_I_STATUS_INVALID_LOW_CLUSTER_ID_VALUE = 4100,
	NAN_I_STATUS_INVALID_HIGH_CLUSTER_ID_VALUE = 4101,
	NAN_I_STATUS_INVALID_BACKGROUND_SCAN_PERIOD = 4102,
	NAN_I_STATUS_INVALID_RSSI_PROXIMITY_VALUE = 4103,
	NAN_I_STATUS_INVALID_SCAN_CHANNEL = 4104,
	NAN_I_STATUS_INVALID_POST_NAN_CONNECTIVITY_CAPABILITIES_BITMAP = 4105,
	NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_NUMCHAN_VALUE = 4106,
	NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_DURATION_VALUE = 4107,
	NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_CLASS_VALUE = 4108,
	NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_CHANNEL_VALUE = 4109,
	NAN_I_STATUS_INVALID_FURTHER_AVAIL_MAP_AVAIL_INTERVAL_BITMAP_VALUE =
		4110,
	NAN_I_STATUS_INVALID_FURTHER_AVAILABILITY_MAP_MAP_ID = 4111,
	NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_CONN_TYPE_VALUE = 4112,
	NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_DEVICE_ROLE_VALUE = 4113,
	NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_DURATION_VALUE = 4114,
	NAN_I_STATUS_INVALID_POST_NAN_DISCOVERY_BITMAP_VALUE = 4115,
	NAN_I_STATUS_MISSING_FUTHER_AVAILABILITY_MAP = 4116,
	NAN_I_STATUS_INVALID_BAND_CONFIG_FLAGS = 4117,
	NAN_I_STATUS_INVALID_RANDOM_FACTOR_UPDATE_TIME_VALUE = 4118,
	NAN_I_STATUS_INVALID_ONGOING_SCAN_PERIOD = 4119,
	NAN_I_STATUS_INVALID_DW_INTERVAL_VALUE = 4120,
	NAN_I_STATUS_INVALID_DB_INTERVAL_VALUE = 4121,
	/* 4122-8191 RESERVED */
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_INVALID = 8192,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_TIMEOUT = 8193,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_USER_REQUEST = 8194,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_FAILURE = 8195,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_COUNT_REACHED = 8196,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_DE_SHUTDOWN = 8197,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_DISABLE_IN_PROGRESS = 8198,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_POST_DISC_ATTR_EXPIRED = 8199,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_POST_DISC_LEN_EXCEEDED = 8200,
	NAN_I_PUBLISH_SUBSCRIBE_TERMINATED_REASON_FURTHER_AVAIL_MAP_EMPTY =
		8201,
	/* 9000-9500 NDP Status type */
	NDP_I_UNSUPPORTED_CONCURRENCY = 9000,
	NDP_I_NAN_DATA_IFACE_CREATE_FAILED = 9001,
	NDP_I_NAN_DATA_IFACE_DELETE_FAILED = 9002,
	NDP_I_DATA_INITIATOR_REQUEST_FAILED = 9003,
	NDP_I_DATA_RESPONDER_REQUEST_FAILED = 9004,
	NDP_I_INVALID_SERVICE_INSTANCE_ID = 9005,
	NDP_I_INVALID_NDP_INSTANCE_ID = 9006,
	NDP_I_INVALID_RESPONSE_CODE = 9007,
	NDP_I_INVALID_APP_INFO_LEN = 9008,
	/* OTA failures and timeouts during negotiation */
	NDP_I_MGMT_FRAME_REQUEST_FAILED = 9009,
	NDP_I_MGMT_FRAME_RESPONSE_FAILED = 9010,
	NDP_I_MGMT_FRAME_CONFIRM_FAILED = 9011,
	NDP_I_END_FAILED = 9012,
	NDP_I_MGMT_FRAME_END_REQUEST_FAILED = 9013,
	NDP_I_MGMT_FRAME_SECURITY_INSTALL_FAILED = 9014,

	/* 9500 onwards vendor specific error codes */
	NDP_I_VENDOR_SPECIFIC_ERROR = 9500
};

enum NanChannelFlag {
	/* no flags */
	NAN_C_FLAG_NONE = 0x0,
	/* 10 MHz wide */
	NAN_C_FLAG_10MHZ = 0x1,
	/* 20 MHz wide */
	NAN_C_FLAG_20MHZ = 0x2,
	/* 40 MHz wide */
	NAN_C_FLAG_40MHZ = 0x4,
	/* 2.4 GHz */
	NAN_C_FLAG_2GHZ = 0x8,
	/* 5 GHz */
	NAN_C_FLAG_5GHZ = 0x10,
	/* IBSS supported */
	NAN_C_FLAG_IBSS = 0x20,
	/* HOST AP mode supported */
	NAN_C_FLAG_HOST_AP = 0x40,
	/* active scanning supported */
	NAN_C_FLAG_ACTIVE = 0x80,
	/* DFS required */
	NAN_C_FLAG_DFS = 0x100,
	/* If 40 Mhz, extension channel above.
	 * If this flag is not set, then the extension channel is below.
	 */
	NAN_C_FLAG_EXTENSION_ABOVE = 0x200,
	/* 80 MHz 11ac 5GHz */
	NAN_C_FLAG_80MHZ = 0x400,
	/* 160 MHz 11ac 5GHz */
	NAN_C_FLAG_160MHZ = 0x800,
	/* Reported RSSI is inaccurate */
	NAN_C_FLAG_RSSI_INVALID = 0x1000,
	/* Channel is in passive mode */
	NAN_C_PASSIVE_MODE = 0x2000,
	/* Restricted use channel */
	NAN_C_INDOOR_RESTRICTED = 0x4000,
	/* Channel restricted in CLM (i.e. by default) */
	NAN_C_CLM_RESTRICTED = 0x8000,
	/* 6 GHz */
	NAN_C_FLAG_6GHZ = 0x10000,
	NAN_C_FLAG_ALL = UINT_MAX,
};

/* #define SIZEOF_TLV_HDR (sizeof(NanTlv.type) + sizeof(NanTlv.length)) */
#define SIZEOF_TLV_HDR                                                  \
	(sizeof(((struct _NanTlv *)0)->type) + \
	sizeof(((struct _NanTlv *)0)->length))

/* NAN ranging indication condition MASKS */
#define NAN_RANGING_INDICATE_CONTINUOUS_MASK 0x01
#define NAN_RANGING_INDICATE_INGRESS_MET_MASK 0x02
#define NAN_RANGING_INDICATE_EGRESS_MET_MASK 0x04

/* SDEA CTRL Parameters BIT Mapping*/
#define SDEA_CTRL_PARMS_FSD_REQUIRED BIT(0)
#define SDEA_CTRL_PARMS_FSD_WITH_GAS BIT(1)
#define SDEA_CTRL_PARMS_DATA_PATH_REQUIRED BIT(2)
#define SDEA_CTRL_PARMS_DATA_PATH_TYPE BIT(3)
#define SDEA_CTRL_PARMS_MULTICAST_TYPE BIT(4)
#define SDEA_CTRL_PARMS_QOS_REQUIRED BIT(5)
#define SDEA_CTRL_PARMS_SECURITY_REQUIRED BIT(6)
#define SDEA_CTRL_PARMS_RANGING_REQUIRED BIT(7)
#define SDEA_CTRL_PARMS_RANGE_LIMIT_PRESENT BIT(8)
#define SDEA_CTRL_PARMS_SERVICE_UPDATE_IND_PRESENT BIT(9)
#define SDEA_CTRL_PARMS_RESERVED_1 BITS(10, 15)
#define SDEA_CTRL_PARMS_RANGE_REPORT BIT(16)
#define SDEA_CTRL_PARMS_RESERVED_2 BIT(17, 31)

/* Publish Service Req parameters bit map */
#define PUB_RESERVED BITS(27, 31)
#define PUB_FOLLOWUP_RX_IND_DISABLE_FLAG BIT(26)
#define PUB_MATCH_EXPIRED_IND_DISABLE_FLAG BIT(25)
#define PUB_TERMINATED_IND_DISABLE_FLAG BIT(24)
#define PUB_CONNMAP BITS(16, 23)
#define PUB_COUNT BITS(8, 15)
#define PUB_MATCH_ALG BITS(6, 7)
#define PUB_OTA_FLAG BIT(5)
#define PUB_RSSI_THRESHOLD_FLAG BIT(4)
#define PUB_TX_TYPE BIT(3)
#define PUB_PUBLISH_TYPE BITS(1, 2)
#define PUB_REPLY_IND_FLAG BIT(0)

/* Subscribe Service Req parameters bit map */
#define SUB_CONNMAP BITS(24, 31)
#define SUB_RESERVED BITS(21, 23)
#define SUB_FOLLOWUP_RX_IND_DISABLE_FLAG BIT(20)
#define SUB_MATCH_EXPIRED_IND_DISABLE_FLAG BIT(19)
#define SUB_TERMINATED_IND_DISABLE_FLAG BIT(18)
#define SUB_OTA_FLAG BIT(17)
#define SUB_RSSI_THRESHOLD_FLAG BIT(16)
#define SUB_COUNT BITS(8, 15)
#define SUB_XBIT BIT(7)
#define SUB_MATCH_ALG BITS(5, 6)
#define SUB_SSI_REQUIRED BIT(4)
#define SUB_SRF_SEND BIT(3)
#define SUB_SRF_INCLUDE BIT(2)
#define SUB_SRF_ATTR BIT(1)
#define SUB_SUBSCRIBE_TYPE BIT(0)

/* Transmit Follow up Request parameters bit map */
#define FLWUP_RESERVED BITS(6, 31)
#define FLWUP_TX_RSP_DISABLE_FLAG BIT(5)
#define FLWUP_WINDOW BIT(4)
#define FLWUP_PRIORITY BITS(0, 3)

/* Get SDEA Ctrl parameters */
#define GET_SDEA_DATA_PATH_REQUIRED(flags)                                     \
	((flags & SDEA_CTRL_PARMS_DATA_PATH_REQUIRED) >>                       \
	 2) /* config_nan_data_path */
#define GET_SDEA_DATA_PATH_TYPE(flags)                                         \
	((flags & SDEA_CTRL_PARMS_DATA_PATH_TYPE) >> 3) /* ndp_type */
#define GET_SDEA_SECURITY_REQUIRED(flags)                                      \
	((flags & SDEA_CTRL_PARMS_SECURITY_REQUIRED) >> 6) /* security_cfg */
#define GET_SDEA_RANGING_REQUIRED(flags)                                       \
	((flags & SDEA_CTRL_PARMS_RANGING_REQUIRED) >> 7) /* ranging_state */
#define GET_SDEA_RANGE_REPORT(flags)                                           \
	((flags & SDEA_CTRL_PARMS_RANGE_REPORT) >> 16) /* range_report */
#define GET_SDEA_QOS_REQUIRED(flags)                                           \
	((flags & SDEA_CTRL_PARMS_QOS_REQUIRED) >> 5) /* fgQoS */
/* Below define is not used now */
#define GET_SDEA_FSD_REQUIRED(flags)                                           \
	(flags & SDEA_CTRL_PARMS_FSD_REQUIRED) /* fgFSDRequire */
#define GET_SDEA_FSD_WITH_GAS(flags)                                           \
	((flags & SDEA_CTRL_PARMS_FSD_WITH_GAS) >> 1) /* fgGAS */
#define GET_SDEA_RANGE_LIMIT_PRESENT(flags)                                    \
	((flags & SDEA_CTRL_PARMS_RANGE_LIMIT_PRESENT) >> 8) /* fgRangeLimit */
#define GET_SDEA_SERVICE_UPDATE_IND_PRESENT(flags)                             \
	((flags & SDEA_CTRL_PARMS_SERVICE_UPDATE_IND_PRESENT) >>               \
	 9) /* eServUpdateInd */

/* Get flags
 * BIT0 - Disable publish termination indication.
 * BIT1 - Disable match expired indication.
 * BIT2 - Disable followUp indication received (OTA).
 * BIT3 - Disable publishReplied indication.
 */
#define GET_PUB_REPLY_IND_FLAG(flags) (!(flags & PUB_REPLY_IND_FLAG) << 3)
#define GET_PUB_FOLLOWUP_RX_IND_DISABLE_FLAG(flags)                            \
	(((flags & PUB_FOLLOWUP_RX_IND_DISABLE_FLAG) >> 26) << 2)
#define GET_PUB_MATCH_EXPIRED_IND_DISABLE_FLAG(flags)                          \
	(((flags & PUB_MATCH_EXPIRED_IND_DISABLE_FLAG) >> 25) << 1)
#define GET_PUB_TERMINATED_IND_DISABLE_FLAG(flags)                             \
	((flags & PUB_TERMINATED_IND_DISABLE_FLAG) >> 24)
#define GET_PUB_CONNMAP(flags) ((flags & PUB_CONNMAP) >> 16)
#define GET_PUB_COUNT(flags) ((flags & PUB_COUNT) >> 8)
#define GET_PUB_MATCH_ALG(flags) ((flags & PUB_MATCH_ALG) >> 6)
#define GET_PUB_OTA_FLAG(flags) ((flags & PUB_OTA_FLAG) >> 5)
#define GET_PUB_RSSI_THRESHOLD_FLAG(flags)                                     \
	((flags & PUB_RSSI_THRESHOLD_FLAG) >> 4)
#define GET_PUB_TX_TYPE(flags) ((flags & PUB_TX_TYPE) >> 3)
#define GET_PUB_PUBLISH_TYPE(flags) ((flags & PUB_PUBLISH_TYPE) >> 1)

/* Get Subscribe Request Parameters */
#define GET_SUB_CONNMAP(flags) ((flags & SUB_CONNMAP) >> 24)

#define GET_SUB_FOLLOWUP_RX_IND_DISABLE_FLAG(flags)                            \
	(((flags & SUB_FOLLOWUP_RX_IND_DISABLE_FLAG) >> 20) << 2)
#define GET_SUB_MATCH_EXPIRED_IND_DISABLE_FLAG(flags)                          \
	(((flags & SUB_MATCH_EXPIRED_IND_DISABLE_FLAG) >> 19) << 1)
#define GET_SUB_TERMINATED_IND_DISABLE_FLAG(flags)                             \
	(((flags & SUB_TERMINATED_IND_DISABLE_FLAG) >> 18))
#define GET_SUB_OTA_FLAG(flags) ((flags & SUB_OTA_FLAG) >> 17)
#define GET_SUB_RSSI_THRESHOLD_FLAG(flags)                                     \
	((flags & SUB_RSSI_THRESHOLD_FLAG) >> 16)
#define GET_SUB_COUNT(flags) ((flags & SUB_COUNT) >> 8)
#define GET_SUB_XBIT(flags) ((flags & SUB_XBIT) >> 7)
#define GET_SUB_MATCH_ALG(flags) ((flags & SUB_MATCH_ALG) >> 5)
#define GET_SUB_SSI_REQUIRED(flags) ((flags & SUB_SSI_REQUIRED) >> 4)
#define GET_SUB_SRF_SEND(flags) ((flags & SUB_SRF_SEND) >> 3)
#define GET_SUB_SRF_INCLUDE(flags) ((flags & SUB_SRF_INCLUDE) >> 2)
#define GET_SUB_SRF_ATTR(flags) ((flags & SUB_SRF_ATTR) >> 1)
#define GET_SUB_SUBSCRIBE_TYPE(flags) (flags & SUB_SUBSCRIBE_TYPE)

/* Get Transmit Follow up Request Parameters */
#define GET_FLWUP_PRIORITY(flags) (flags & FLWUP_PRIORITY)
#define GET_FLWUP_WINDOW(flags) ((flags & FLWUP_WINDOW) >> 4)
#define GET_FLWUP_TX_RSP_DISABLE_FLAG(flags)                                   \
	((flags & FLWUP_TX_RSP_DISABLE_FLAG) >> 5)

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
u16 nan_read_tlv(u8 *pInTlv, struct _NanTlv *pOutTlv);
void nanSetPublishPmkid(struct ADAPTER *prAdapter,
				struct NanPublishRequest *msg);
u16 nanMapPublishReqParams(u16 *pIndata, struct NanPublishRequest *pOutparams);
u16 nanMapSubscribeReqParams(u16 *pIndata,
			     struct NanSubscribeRequest *pOutparams);
u16 nanMapFollowupReqParams(u32 *pIndata,
			    struct NanTransmitFollowupRequest *pOutparams);
void nanMapSdeaCtrlParams(u32 *pIndata,
			  struct NanSdeaCtrlParams *prNanSdeaCtrlParms);
void nanMapRangingConfigParams(u32 *pIndata,
			       struct NanRangingCfg *prNanRangingCfg);
void
nanMapNan20RangingReqParams(struct ADAPTER *prAdapter, u32 *pIndata,
			    struct NanRangeResponseCfg *prNanRangeRspCfgParms);
int mtk_cfg80211_vendor_nan(struct wiphy *wiphy, struct wireless_dev *wdev,
			    const void *data, int data_len);
#if CFG_SUPPORT_NAN_EXT
int mtk_cfg80211_vendor_nan_ext(struct wiphy *wiphy, struct wireless_dev *wdev,
			    const void *data, int data_len);
int mtk_cfg80211_vendor_nan_ext_indication(struct ADAPTER *prAdapter,
					   u8 *data, uint16_t u2Size);
#endif /* CFG_SUPPORT_NAN_EXT */
int mtk_cfg80211_vendor_event_nan_event_indication(struct ADAPTER *prAdapter,
						   uint8_t *pcuEvtBuf);
int mtk_cfg80211_vendor_event_nan_schedule_config(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_lowpower_ctrl(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_report_beacon(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_replied_indication(struct ADAPTER *prAdapter,
						 uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_publish_terminate(struct ADAPTER *prAdapter,
						uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_subscribe_terminate(struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_followup_indication(struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_selfflwup_indication(struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf);
int mtk_cfg80211_vendor_event_nan_match_indication(struct ADAPTER *prAdapter,
						   uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_match_expire(struct ADAPTER *prAdapter,
					       uint8_t *pcuEvtBuf);
int
mtk_cfg80211_vendor_event_nan_disable_indication(struct ADAPTER *prAdapter,
						uint8_t *pcuEvtBuf);
void
nanNdpDissolve(struct ADAPTER *prAdapter,
	uint32_t u4Timeout);

#endif
