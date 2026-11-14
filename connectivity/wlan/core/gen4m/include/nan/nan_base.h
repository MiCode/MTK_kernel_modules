/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_BASE_H_
#define _NAN_BASE_H_

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if CFG_SUPPORT_NAN
/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/* NAN Frame - OUI */
#define VENDOR_OUI_LEN 3

#define NAN_OUI                                                                \
	{ 0x50, 0x6f, 0x9a }

/* NAN Frame - OUI Type */
#define VENDOR_OUI_TYPE_NAN_SDF 0x13
#define VENDOR_OUI_TYPE_NAN_NAF 0x18

/* NAF Header Length */
#define NAF_HEADER_LEN 6

/* NAN Attribute Header Length */
#define NAN_ATTR_HDR_LEN 3


#define NAN_ATTR_ID(fp)		(((struct _NAN_ATTR_HDR_T *)fp)->ucAttrId)
#define NAN_ATTR_LEN(fp)	(((struct _NAN_ATTR_HDR_T *)fp)->u2Length)
#define NAN_ATTR_SIZE(fp)	(NAN_ATTR_HDR_LEN + NAN_ATTR_LEN(fp))
#define NAN_ATTR_END(fp)	((void *)((uint8_t *)(fp) + NAN_ATTR_SIZE(fp)))

#define NAN_AVAIL_ENTRY_HDR_LEN 2
#define NAN_AVAIL_ENTRY_LEN(fp)		\
	(((struct _NAN_AVAILABILITY_ENTRY_T *)fp)->u2Length)
#define NAN_AVAIL_ENTRY_SIZE(fp)	\
	(NAN_AVAIL_ENTRY_HDR_LEN + NAN_AVAIL_ENTRY_LEN(fp))
#define NAN_AVAIL_ENTRY_END(fp)	\
		((void *)((uint8_t *)(fp) + NAN_AVAIL_ENTRY_SIZE(fp)))

/* NAN 4.0 Table 58. Service Protocol Types */
enum NAN_SERVICE_PROTOCOL_TYPES {
	NAN_SERVICE_PROTOCOL_TYPE_RESERVED,
	NAN_SERVICE_PROTOCOL_TYPE_BONJOUR,
	NAN_SERVICE_PROTOCOL_TYPE_GENERIC,
	NAN_SERVICE_PROTOCOL_TYPE_CSA_MATTER,
	/* 4~255: Reserved */
};

/* NAN Service Name Hash Length */
#define NAN_SERVICE_HASH_LENGTH 6

/* NAN Attribute ID Definitions */
/* NAN 4.0 Table 42. NAN attributes in NAN Beacon frame and NAN SDF */
#define NAN_ATTR_ID_MASTER_INDICATION 0x00
#define NAN_ATTR_ID_CLUSTER 0x01
#define NAN_ATTR_ID_SERVICE_ID_LIST 0x02
#define NAN_ATTR_ID_SERVICE_DESCRIPTOR 0x03
#define NAN_ATTR_ID_NAN_CONNECTION_CAPABILITY 0x04
#define NAN_ATTR_ID_WLAN_INFRASTRUCTURE 0x05
#define NAN_ATTR_ID_P2P_OPERATION 0x06
#define NAN_ATTR_ID_IBSS 0x07
#define NAN_ATTR_ID_MESH 0x08
#define NAN_ATTR_ID_FSD 0x09
#define NAN_ATTR_ID_FURTHER_AVAILABILITY_MAP 0x0A
#define NAN_ATTR_ID_COUNTRY_CODE 0x0B
#define NAN_ATTR_ID_RANGING 0x0C
#define NAN_ATTR_ID_CLUSTER_DISCOVERY 0x0D
#define NAN_ATTR_ID_SDEA 0x0E
#define NAN_ATTR_ID_DEVICE_CAPABILITY 0x0F
#define NAN_ATTR_ID_NDP 0x10
#define NAN_ATTR_ID_NMSG 0x11
#define NAN_ATTR_ID_NAN_AVAILABILITY 0x12
#define NAN_ATTR_ID_NDC 0x13
#define NAN_ATTR_ID_NDL 0x14
#define NAN_ATTR_ID_NDL_QOS 0x15
#define NAN_ATTR_ID_MULTICAST_SCHEDULE 0x16
#define NAN_ATTR_ID_UNALIGNED_SCHEDULE 0x17
#define NAN_ATTR_ID_PAGING_UNICAST 0x18
#define NAN_ATTR_ID_PAGING_MULTICAST 0x19
#define NAN_ATTR_ID_RANGING_INFORMATION 0x1A
#define NAN_ATTR_ID_RANGING_SETUP 0x1B
#define NAN_ATTR_ID_FTM_RANGING_REPORT 0x1C
#define NAN_ATTR_ID_ELEMENT_CONTAINER 0x1D
#define NAN_ATTR_ID_EXT_WLAN_INFRASTRUCTURE 0x1E
#define NAN_ATTR_ID_EXT_P2P_OPERATION 0x1F
#define NAN_ATTR_ID_EXT_IBSS 0x20
#define NAN_ATTR_ID_EXT_MESH 0x21
#define NAN_ATTR_ID_CIPHER_SUITE_INFO 0x22
#define NAN_ATTR_ID_SECURITY_CONTEXT_INFO 0x23
#define NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR 0x24
#define NAN_ATTR_ID_MULTICAST_SCHEDULE_CHANGE 0x25
#define NAN_ATTR_ID_MULTICAST_SCHEDULE_OWNER_CHANGE 0x26
#define NAN_ATTR_ID_PUBLIC_AVAILABILITY 0x27
#define NAN_ATTR_ID_SUBSCRIBE_SERVICE_ID_LIST 0x28
#define NAN_ATTR_ID_NDP_EXTENSION 0x29
#define NAN_ATTR_ID_DEVICE_CAPABILITY_EXT 0x2A
#define NAN_ATTR_ID_NAN_IDENTITY_RESOLUTION 0x2B
#define NAN_ATTR_ID_NAN_PAIRING_BOOTSTRAPPING 0x2C
#define NAN_ATTR_ID_S3 0x2D
#define NAN_ATTR_ID_TRANSMIT_POWER_ENVELOPE 0x2E
#define NAN_ATTR_ID_VENDOR_SPECIFIC 0xDD

/* NAN Reason Code Field */
/* NAN 4.0 Table 43. Reason Code field */
#define NAN_REASON_CODE_RESERVED 0
#define NAN_REASON_CODE_UNSPECIFIED 1
#define NAN_REASON_CODE_RESOURCE_LIMITATION 2
#define NAN_REASON_CODE_INVALID_PARAMS 3
#define NAN_REASON_CODE_FTM_PARAMS_INCAPABLE 4
#define NAN_REASON_CODE_NO_MOVEMENT 5
#define NAN_REASON_CODE_INVALID_AVAILABILITY 6
#define NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE 7
#define NAN_REASON_CODE_SECURITY_POLICY 8
#define NAN_REASON_CODE_QOS_UNACCEPTABLE 9
#define NAN_REASON_CODE_NDP_REJECTED 10
#define NAN_REASON_CODE_NDL_UNACCEPTABLE 11
#define NAN_REASON_CODE_RANGING_SCHEDULE_UNACCEPTABLE 12
#define NAN_REASON_CODE_PAIRING_BOOTSTRAPPING_REJECTED 13

/* NAN NDP Attribute - Type and Status */
#define NAN_ATTR_NDP_TYPE_MASK BITS(0, 3)
#define NAN_ATTR_NDP_STATUS_MASK BITS(4, 7)
#define NAN_ATTR_NDP_STATUS_OFFSET (4)

/* NAN NDP Attribute - Type */
#define NAN_ATTR_NDP_TYPE_REQUEST 0
#define NAN_ATTR_NDP_TYPE_RESPONSE 1
#define NAN_ATTR_NDP_TYPE_CONFIRM 2
#define NAN_ATTR_NDP_TYPE_SEC_INSTALL 3
#define NAN_ATTR_NDP_TYPE_TERMINATE 4

/* NAN NDP Attribute - Status */
#define NAN_ATTR_NDP_STATUS_CONTINUED 0
#define NAN_ATTR_NDP_STATUS_ACCEPTED 1
#define NAN_ATTR_NDP_STATUS_REJECTED 2

/* NAN NDP Attribute - NDP Control Field */
#define NAN_ATTR_NDP_CTRL_CONFIRM_REQUIRED BIT(0)
#define NAN_ATTR_NDP_CTRL_SECURITY_PRESENT BIT(2)
#define NAN_ATTR_NDP_CTRL_PUBLISHID_PRESENT BIT(3)
#define NAN_ATTR_NDP_CTRL_RESP_NDI_PRESENT BIT(4)
#define NAN_ATTR_NDP_CTRL_SPECIFIC_INFO_PRESENT BIT(5)

/* NAN NDPE Attribute - Type and Status */
#define NAN_ATTR_NDPE_TYPE_MASK BITS(0, 3)
#define NAN_ATTR_NDPE_STATUS_MASK BITS(4, 7)
#define NAN_ATTR_NDPE_STATUS_OFFSET (4)

/* NAN NDPE Attribute - Type */
#define NAN_ATTR_NDPE_TYPE_REQUEST 0
#define NAN_ATTR_NDPE_TYPE_RESPONSE 1
#define NAN_ATTR_NDPE_TYPE_CONFIRM 2
#define NAN_ATTR_NDPE_TYPE_SEC_INSTALL 3
#define NAN_ATTR_NDPE_TYPE_TERMINATE 4

/* NAN NDPE Attribute - Status */
#define NAN_ATTR_NDPE_STATUS_CONTINUED 0
#define NAN_ATTR_NDPE_STATUS_ACCEPTED 1
#define NAN_ATTR_NDPE_STATUS_REJECTED 2

/* NAN NDPE Attribute - NDPE Control Field */
#define NAN_ATTR_NDPE_CTRL_CONFIRM_REQUIRED BIT(0)
#define NAN_ATTR_NDPE_CTRL_SECURITY_PRESENT BIT(2)
#define NAN_ATTR_NDPE_CTRL_PUBLISHID_PRESENT BIT(3)
#define NAN_ATTR_NDPE_CTRL_RESP_NDI_PRESENT BIT(4)

/* NAN NDPE Attribute - TLV Type */
/* NAN 4.0 Table 89. List of TLV Types for the NDPE attribute */
enum NAN_ATTR_NDPE_TLV_TYPES {
	NAN_ATTR_NDPE_TLV_TYPE_IPV6_LINK_LOCAL,
	NAN_ATTR_NDPE_TLV_TYPE_SERVICE_INFO,
};

#define NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_TRANSPORT_PORT 0x00
#define NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_PROTOCOL 0x01
#define NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_SPECINFO 0xDD

/*NAN NDC attribute control field*/
#define NAN_ATTR_NDC_CTRL_SELECTED_FOR_NDL BIT(0)

#define NAN_NDC_ATTRIBUTE_ID_LENGTH 6

/* NAN NDL Attribute - Type and Status */
#define NAN_ATTR_NDL_TYPE_MASK BITS(0, 3)
#define NAN_ATTR_NDL_STATUS_MASK BITS(4, 7)
#define NAN_ATTR_NDL_STATUS_OFFSET (4)

/* NAN NDL Attribute - Type */
#define NAN_ATTR_NDL_TYPE_REQUEST 0
#define NAN_ATTR_NDL_TYPE_RESPONSE 1
#define NAN_ATTR_NDL_TYPE_CONFIRM 2

/* NAN NDL Attribute - Status */
#define NAN_ATTR_NDL_STATUS_CONTINUED 0
#define NAN_ATTR_NDL_STATUS_ACCEPTED 1
#define NAN_ATTR_NDL_STATUS_REJECTED 2

/* NAN NDL Attribute - NDL Control Field */
/* b2NdlSetupReason */
#define NAN_ATTR_NDL_CTRL_NDL_SETUP_NDP 0
#define NAN_ATTR_NDL_CTRL_NDL_SETUP_FSD_USING_GAS 1

/* NAN Device capability extension */
#define NAN_DEVICE_CAPABILITY_EXT_REG_INFO_EN BIT(0)
#define NAN_DEVICE_CAPABILITY_EXT_REG_INFO BITS(1, 3)
#define NAN_DEVICE_CAPABILITY_EXT_PARING_SETUP_EN BIT(8)
#define NAN_DEVICE_CAPABILITY_EXT_NPK_NIK_CACHE_EN BIT(9)
#define NAN_DEVICE_CAPABILITY_EXT_REG_INFO_OFFSET 1
#define NAN_DEVICE_CAPABILITY_EXT_SETTING_OFFSET 8

/* NAN Availability Attribute - Attribute Control Field */
#define NAN_AVAIL_CTRL_MAPID BITS(0, 3)
#define NAN_AVAIL_CTRL_COMMIT_CHANGED BIT(4)
#define NAN_AVAIL_CTRL_POTN_CHANGED BIT(5)
#define NAN_AVAIL_CTRL_PUBLIC_AVAIL_CHANGED BIT(6)
#define NAN_AVAIL_CTRL_NDC_CHANGED BIT(7)
#define NAN_AVAIL_CTRL_CHECK_FOR_CHANGED BITS(4, 7)

/* NAN Availability Attribute - Availability Entry Control Field */
#define NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT BIT(0)
#define NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN BIT(1)
#define NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND BIT(2)

#define NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE BITS(0, 2)
#define NAN_AVAIL_ENTRY_CTRL_USAGE_PREF BITS(3, 4)
#define NAN_AVAIL_ENTRY_CTRL_UTIL BITS(5, 7)
#define NAN_AVAIL_ENTRY_CTRL_RX_NSS BITS(8, 11)
#define NAN_AVAIL_ENTRY_CTRL_TBITMAP_PRESENT BIT(12)

#define NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_OFFSET 0
#define NAN_AVAIL_ENTRY_CTRL_USAGE_PREF_OFFSET 3
#define NAN_AVAIL_ENTRY_CTRL_UTIL_OFFSET 5
#define NAN_AVAIL_ENTRY_CTRL_RX_NSS_OFFSET 8
#define NAN_AVAIL_ENTRY_CTRL_TBITMAP_PRESENT_OFFSET 12

#define NAN_AVAIL_ENTRY_CTRL_COMMITTED(_ctrl) \
	(!!((_ctrl) & NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT))
#define NAN_AVAIL_ENTRY_CTRL_POTENTIAL(_ctrl) \
	(!!((_ctrl) & NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN))
#define NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(_ctrl) \
	(!!((_ctrl) & NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND))

#define NAN_AVAIL_ENTRY_CTRL_TYPE(_ctrl) \
	((uint8_t)(((_ctrl) & NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE) >> \
		NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_OFFSET))
#define NAN_AVAIL_ENTRY_CTRL_P(_ctrl) \
	((uint8_t)(((_ctrl) & NAN_AVAIL_ENTRY_CTRL_USAGE_PREF) >> \
		NAN_AVAIL_ENTRY_CTRL_USAGE_PREF_OFFSET))
#define NAN_AVAIL_ENTRY_CTRL_U(_ctrl) \
	((uint8_t)(((_ctrl) & NAN_AVAIL_ENTRY_CTRL_UTIL) >> \
		NAN_AVAIL_ENTRY_CTRL_UTIL_OFFSET))
#define NAN_AVAIL_ENTRY_CTRL_NSS(_ctrl) \
	((uint8_t)(((_ctrl) & NAN_AVAIL_ENTRY_CTRL_RX_NSS) >> \
		NAN_AVAIL_ENTRY_CTRL_RX_NSS_OFFSET))
#define NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(_ctrl) \
	((uint8_t)(((_ctrl) & NAN_AVAIL_ENTRY_CTRL_TBITMAP_PRESENT) >> \
		NAN_AVAIL_ENTRY_CTRL_TBITMAP_PRESENT_OFFSET))

#define NAN_AVAIL_ENTRY_CTRL_SET_TYPE(_ctrl, _val)			       \
	do {								       \
		(_ctrl) &= ~NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE;		       \
		(_ctrl) |= (_val) << NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_OFFSET & \
				NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE;	       \
	} while (0)
#define NAN_AVAIL_ENTRY_CTRL_SET_PREF(_ctrl, _val)			\
	((_ctrl) |= (_val) << NAN_AVAIL_ENTRY_CTRL_USAGE_PREF_OFFSET &	\
				NAN_AVAIL_ENTRY_CTRL_USAGE_PREF)
#define NAN_AVAIL_ENTRY_CTRL_SET_UTIL(_ctrl, _val)			\
	((_ctrl) |= (_val) << NAN_AVAIL_ENTRY_CTRL_UTIL_OFFSET &	\
				NAN_AVAIL_ENTRY_CTRL_UTIL)
#define NAN_AVAIL_ENTRY_CTRL_SET_NSS(_ctrl, _val)			\
	((_ctrl) |= (_val) << NAN_AVAIL_ENTRY_CTRL_RX_NSS_OFFSET &	\
				NAN_AVAIL_ENTRY_CTRL_RX_NSS)
#define NAN_AVAIL_ENTRY_CTRL_SET_TBITMAP_P(_ctrl, _val)			    \
	((_ctrl) |= (_val) << NAN_AVAIL_ENTRY_CTRL_TBITMAP_PRESENT_OFFSET & \
				NAN_AVAIL_ENTRY_CTRL_TBITMAP_PRESENT)

/* NAN Band/Channel Entry List Field */
#define NAN_BAND_CH_ENTRY_LIST_TYPE_BAND 0
#define NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL 1

#define NAN_BAND_CH_ENTRY_LIST_TYPE_OFFSET 0
#define NAN_BAND_CH_ENTRY_LIST_TYPE BIT(0)
#define NAN_BAND_CH_ENTRY_LIST_NON_CONTINUOUS_BW_OFFSET 1
#define NAN_BAND_CH_ENTRY_LIST_NON_CONTINUOUS_BW BIT(1)
#define NAN_BAND_CH_ENTRY_LIST_NUM_ENTRY_OFFSET 4
#define NAN_BAND_CH_ENTRY_LIST_NUM_ENTRY BITS(4, 7)

/* NAN 4.0 Table 79. Device Capability attribute format, Supported Bands */
enum NAN_SUPPORTED_BANDS {
	/* RESERVED for TV whitespace = 0 */
	/* Sub-1 GHz (excluding TV whitespace) = 1 */
	NAN_SUPPORTED_BAND_ID_2P4G = 2,
	/* Reserved (for 3.6 GHz) = 3 */
	NAN_SUPPORTED_BAND_ID_5G = 4,
	/* Reserved (for 60 GHz) = 5 */
	/* Reserved (for 45 GHz) = 6 */
	NAN_PROPRIETARY_BAND_ID_6G = 6, /* from IOT devices */
	NAN_SUPPORTED_BAND_ID_6G = 7,
};

#define NAN_SUPPORTED_2G_BIT    BIT(NAN_SUPPORTED_BAND_ID_2P4G)
#define NAN_SUPPORTED_5G_BIT    BIT(NAN_SUPPORTED_BAND_ID_5G)
#define NAN_PROPRIETARY_6G_BIT  BIT(NAN_PROPRIETARY_BAND_ID_6G)
#define NAN_SUPPORTED_6G_BIT    BIT(NAN_SUPPORTED_BAND_ID_6G)

/* NAN 4.0 Table 81. Operation Mode field format
 * PHY mode:
 * b0: 0 HT only; 1 VHT
 * b4: 0 HE not supported; 1 HE
 *
 * b1: HE/VHT 80+80 support
 * b2: HE/VHT 160 support
 * b3: Reserved (Paging NDL support)
 */
#define NAN_DEV_CAP_OP_MODE_PHY_MODE (0) /* NAN 3.2, 0: HT only; 1: VHT */

/* NAN SDA Service Control Field */
#define NAN_SDA_SERVICE_CONTROL_TYPE BITS(0, 1)
#define NAN_SDA_SERVICE_CONTROL_MATCH_FILTER_PRESENT BIT(2)
#define NAN_SDA_SERVICE_CONTROL_SRF_PRESENT BIT(3)
#define NAN_SDA_SERVICE_CONTROL_SERV_INFO_PRESENT BIT(4)
#define NAN_SDA_SERVICE_CONTROL_RANG_LIMIT_PRESENT BIT(5)
#define NAN_SDA_SERVICE_CONTROL_BINDING_BITMAP_PRESENT BIT(6)

#define NAN_SDA_SERVICE_CONTROL_TYPE_PUBLISH 0
#define NAN_SDA_SERVICE_CONTROL_TYPE_SUBSCRIBE BIT(0)
#define NAN_SDA_SERVICE_CONTROL_TYPE_FOLLOWUP BIT(1)

/* NAN SDEA Control Field */
#define NAN_SDEA_CTRL_FSD_REQUIRED BIT(0)
#define NAN_SDEA_CTRL_GAS_FSD BIT(0)
#define NAN_SDEA_CTRL_DATA_PATH_REQUIRED BIT(2)
#define NAN_SDEA_CTRL_DATA_PATH_TYPE BIT(3)
#define NAN_SDEA_CTRL_QOS_REQUIRED BIT(5)
#define NAN_SDEA_CTRL_SECURITY_REQUIRED BIT(6)
#define NAN_SDEA_CTRL_RANGING_REQUIRED BIT(7)
#define NAN_SDEA_CTRL_RANGING_LIMIT_PRESENT BIT(8)
#define NAN_SDEA_CTRL_SERV_UPDATE_INDICATOR BIT(9)

/* Table 97 NAN Time Bitmap Control Field */
#define NAN_TIME_BITMAP_CTRL_DURATION_OFFSET 0
#define NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET 3
#define NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET 6

#define NAN_TIME_BITMAP_CTRL_DURATION BITS(0, 2)
#define NAN_TIME_BITMAP_CTRL_PERIOD BITS(3, 5)
#define NAN_TIME_BITMAP_CTRL_STARTOFFSET BITS(6, 14)

/**
 * u2BitDuration BITS(0,2), 16 << n TU, n < 4
 * u2Period BITS(3,5), 128 << (n-1) TU, n > 0
 * u2StartOffset BITS(6,14), 16 * n TU
 */
#define NAN_GET_TIME_BITMAP_CTRL_DURATION_IN_TU(_ctrl)			\
	(16 << ((((_ctrl) & NAN_TIME_BITMAP_CTRL_DURATION) >>		\
		NAN_TIME_BITMAP_CTRL_DURATION_OFFSET)))
#define NAN_GET_TIME_BITMAP_CTRL_PERIOD_IN_TU(_ctrl)			\
	({	uint8_t x;						\
		x = (((_ctrl) & NAN_TIME_BITMAP_CTRL_PERIOD) >>		\
		     NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET);		\
		x ? 64 << x : 0;					\
	 })
#define NAN_GET_TIME_BITMAP_CTRL_STARTOFFSET_IN_TU(_ctrl)		\
	(16 * (((_ctrl) & NAN_TIME_BITMAP_CTRL_STARTOFFSET) >>		\
		NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET))

#define NAN_GET_TIME_BITMAP_CTRL_DURATION_IN_SLOT(_ctrl)		\
	(NAN_GET_TIME_BITMAP_CTRL_DURATION_IN_TU(_ctrl) / NAN_SLOT_INTERVAL)
#define NAN_GET_TIME_BITMAP_CTRL_PERIOD_IN_SLOT(_ctrl)			\
	(NAN_GET_TIME_BITMAP_CTRL_PERIOD_IN_TU(_ctrl) / NAN_SLOT_INTERVAL)
#define NAN_GET_TIME_BITMAP_CTRL_STARTOFFSET_IN_SLOT(_ctrl)		\
	(NAN_GET_TIME_BITMAP_CTRL_STARTOFFSET_IN_TU(_ctrl) / NAN_SLOT_INTERVAL)

/* NAN Cipher Suite ID */
#define NAN_CIPHER_SUITE_ID_NONE 0
#define NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128 1
#define NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256 2

/* NAN Security Context */
#define NAN_SCID_DEFAULT_LEN 16

/* Anchor Master rank length */
#define ANCHOR_MASTER_RANK_NUM 8

/* Ranging Info Attribute - Location Info Availability */
#define NAN_RANGING_LOCAL_COORDINATE_PRESENT BIT(0)
#define NAN_RANGING_GEO_LOCATION_PRESENT BIT(1)
#define NAN_RANGING_CIVIC_LOCATION_PRESENT BIT(2)
#define NAN_RANGING_LAST_MOVEMENT_PRESENT BIT(3)

/* Ranging Setup Attribute - Type and Status */
#define NAN_RANGING_TYPE_REQUEST 0
#define NAN_RANGING_TYPE_RESPONSE 1
#define NAN_RANGING_TYPE_TERMINATION 2
#define NAN_RANGING_TYPE_MASK BITS(0, 3)
#define NAN_RANGING_STATUS_ACCEPTED 0
#define NAN_RANGING_STATUS_REJECTED 1
#define NAN_RANGING_STATUS_MASK BITS(4, 7)
#define NAN_RANGING_STATUS_OFFSET 4

/* Ranging Setup Attribute - Ranging Control */
#define NAN_RANGING_CTL_REPORT_REQUIRED BIT(0)
#define NAN_RANGING_CTL_FTM_PARAMETERS_PRESENT BIT(1)
#define NAN_RANGING_CTL_SCHEDULE_ENTRY_PRESENT BIT(2)

/* Device Capability - Capabilityies */
#define NAN_ATTR_DEVICE_CAPABILITY_CAP_DFS_MASTER BIT(0)
#define NAN_ATTR_DEVICE_CAPABILITY_CAP_EXTENDED_KEY_ID BIT(1)
#define NAN_ATTR_DEVICE_CAPABILITY_CAP_SIMULTANEOUS_NDP BIT(2)
#define NAN_ATTR_DEVICE_CAPABILITY_CAP_SUPPORT_NDPE BIT(3)

#define NAN_ACTION_TO_MSG(_ACT) (_ACT - 4)

/* The macro to check if it is WFA Specific OUI */
#define IS_WFA_SPECIFIC_OUI(_pucDestAddr)                   \
	((*(uint8_t *)(_pucDestAddr) == 0x50) &&           \
	 (*(uint8_t *)(_pucDestAddr + 1) == 0x6F) &&       \
	 (*(uint8_t *)(_pucDestAddr + 2) == 0x9A))

/* NAN Action frame subtypes */
enum _NAN_ACTION_T {
	NAN_ACTION_NULL = 0,
	NAN_ACTION_RANGING_REQUEST = 1,
	NAN_ACTION_RANGING_RESPONSE = 2,
	NAN_ACTION_RANGING_TERMINATION = 3,
	NAN_ACTION_RANGING_REPORT = 4,
	NAN_ACTION_DATA_PATH_REQUEST = 5,
	NAN_ACTION_DATA_PATH_RESPONSE = 6,
	NAN_ACTION_DATA_PATH_CONFIRM = 7,
	NAN_ACTION_DATA_PATH_KEY_INSTALLMENT = 8,
	NAN_ACTION_DATA_PATH_TERMINATION = 9,
	NAN_ACTION_SCHEDULE_REQUEST = 10,
	NAN_ACTION_SCHEDULE_RESPONSE = 11,
	NAN_ACTION_SCHEDULE_CONFIRM = 12,
	NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION = 13,
	NAN_ACTION_RESERVED = 221, /* 0xDD */
	NAN_ACTION_NUM
};

enum NAN_SEC_MSG {
	NAN_SEC_M1 = 1,
	NAN_SEC_M2,
	NAN_SEC_M3,
	NAN_SEC_M4,
	NAN_SEC_END
};

enum NAN_NDP_ROLE {
	NAN_NDP_INITIATOR = 0,
	NAN_NDP_RESPONDER,
	NAN_NDP_ROLE_NUM
};

enum RESCHEDULE_SOURCE {
	RESCHEDULE_NULL,
	AIS_CONNECTED,
	AIS_DISCONNECTED,
	NEW_NDL,
	REMOVE_NDL,
	P2P_CONNECTED,
	P2P_DISCONNECTED,
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* NAN Information Header */
/* NAN 4.0 Table 29. NAN IE format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_IE_T {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t aucNanOui[VENDOR_OUI_LEN];
	uint8_t ucNanOuiType;
	uint8_t ucNanDetails[]; /* one or more NAN attribute */
} __KAL_ATTRIB_PACKED__;

/* NAN Action Frame */
/* NAN 4.0 Table 34. General format of NAN Action frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ACTION_FRAME_T {
	/* action MAC header */
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucDestAddr[MAC_ADDR_LEN];
	uint8_t aucSrcAddr[MAC_ADDR_LEN];
	uint8_t aucClusterID[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;

	/* action frame body */
	uint8_t ucCategory;
	uint8_t ucAction;
	uint8_t aucOUI[VENDOR_OUI_LEN];
	uint8_t ucOUItype;
	uint8_t ucOUISubtype;
	uint8_t aucInfoContent[];
} __KAL_ATTRIB_PACKED__;

/* NAN SDF Action Frame */
/* NAN 4.0 Table 32. NAN Service Discovery frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_SDF_FRAME_T {
	/* action MAC header */
	uint16_t u2FrameCtrl;
	uint16_t u2DurationID;
	uint8_t aucDestAddr[MAC_ADDR_LEN];
	uint8_t aucSrcAddr[MAC_ADDR_LEN];
	uint8_t aucClusterID[MAC_ADDR_LEN];
	uint16_t u2SeqCtrl;

	/* action frame body */
	uint8_t ucCategory;
	uint8_t ucAction;
	uint8_t aucOUI[VENDOR_OUI_LEN];
	uint8_t ucOUItype;
	uint8_t aucInfoContent[];
} __KAL_ATTRIB_PACKED__;

/* NAN attribute general format */
/* NAN 4.0 Table 41. List of NAN attributes */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_HDR_T {
	uint8_t ucAttrId;
	uint16_t u2Length;
	uint8_t aucAttrBody[]; /* NAN attribute specific information field */
} __KAL_ATTRIB_PACKED__;

/* NAN attribute definitions */
/* NAN 4.0 Table 79. Device Capability attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_DEVICE_CAPABILITY_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_DEVICE_CAPABILITY */
	uint16_t u2Length;
	uint8_t ucMapID;
	union { /* NAN 4.0 Table 80. Committed DW Information field format */
		uint16_t u2CommittedDWInfo;

		struct {
			uint16_t u2CommittedDw2g :3,
				 u2CommittedDw5g :3,
				 u2CommittedDw2gOverwrite :4,
				 u2CommittedDw5gOverwrite :4,
				 u2CommittedDWReserved :2;
		};
	};

	uint8_t ucSupportedBands; /* enum NAN_SUPPORTED_BANDS */
	union { /* NAN 4.0 Table 81. Operation Mode field format */
		uint8_t ucOperationMode; /* NAN_DEV_CAP_OP_MODE_PHY_MODE */
		struct {
			uint8_t ucOperPhyModeVht :1, /* 0: HT only */
				ucOperPhyModeBw_80_80 :1,
				ucOperPhyModeBw_160 :1,
				ucOperPhyModeReserved :1,
				ucOperPhyModeHe :1,
				ucOperPhyModeReserved2 :3;
		};
	};

	struct { /* Number of Antennas */
		uint8_t ucNumOfAntennasTx :4,
			ucNumOfAntennasRx :4;
	};
	uint16_t u2MaxChannelSwitchTime; /* micro-seconds, 0: not available */
	uint8_t ucCapabilities;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 141. Capability Info field */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_EXT_CAPABILITIES_T {
	uint8_t ucRegulatoryInfo; /* NAN_ATTR_ID_DEVICE_CAPABILITY_EXT */
	uint8_t ucSettings;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 140. Device Capability Extension attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_DEVICE_CAPABILITY_EXT_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_DEVICE_CAPABILITY_EXT */
	uint16_t u2Length;
	uint8_t aucExtCapabilities[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 82. NDP attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDP_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_NDP */
	uint16_t u2Length;
	uint8_t ucDialogToken;
	union {
		uint8_t ucTypeStatus;
		struct {
			uint8_t ucType :4, /* NAN_ATTR_NDP_TYPE_* */
				ucStatus :4; /* NAN_ATTR_NDP_STATUS_* */
		};
	};
	uint8_t ucReasonCode;
	uint8_t aucInitiatorNDI[6];
	uint8_t ucNDPID;
	uint8_t ucNDPControl;
	uint8_t ucPublishID;	   /* optional */
	uint8_t aucResponderNDI[6];    /*optional */
	uint8_t aucNDPSpecificInfo[]; /*to be defined*/
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 88. General TLV format for the NDPE attribute */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDPE_GENERAL_TLV_T {
	uint8_t ucType;
	uint16_t u2Length;
	uint8_t aucValue[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 90. IPv6 Link Local TLV format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDPE_IPV6_LINK_LOCAL_TLV_T {
	uint8_t ucType; /* NAN_ATTR_NDPE_TLV_TYPE_IPV6_LINK_LOCAL */
	uint16_t u2Length;
	uint8_t aucInterfaceId[8];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 91. Service Info TLV format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDPE_SVC_INFO_TLV_T {
	uint8_t ucType; /* NAN_ATTR_NDPE_TLV_TYPE_SERVICE_INFO */
	uint16_t u2Length;
	uint8_t aucOui[VENDOR_OUI_LEN]; /* Vendor OUI */
	uint8_t aucBody[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 92. Wi-Fi Alliance Service Info TLV format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDPE_WFA_SVC_INFO_TLV_T {
	uint8_t ucType; /* NAN_ATTR_NDPE_TLV_TYPE_SERVICE_INFO */
	uint16_t u2Length;
	uint8_t aucOui[VENDOR_OUI_LEN]; /* WFA specific OUI, 0x50-6F-9A */
	uint8_t ucServiceProtocolType;  /* enum NAN_SERVICE_PROTOCOL_TYPES */
	uint8_t aucBody[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 86. NAN Data Path Extension attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDPE_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_NDP_EXTENSION */
	uint16_t u2Length;
	uint8_t ucDialogToken;
	union {
		uint8_t ucTypeStatus;
		struct {
			uint8_t ucType :4, /* NAN_ATTR_NDP_TYPE_* */
				ucStatus :4; /* NAN_ATTR_NDP_STATUS_* */
		};
	};
	uint8_t ucReasonCode;
	uint8_t aucInitiatorNDI[6];
	uint8_t ucNDPID;
	uint8_t ucNDPEControl;
	uint8_t ucPublishID;	/* optional */
	uint8_t aucResponderNDI[6]; /*optional */
	uint8_t aucTLVList[];      /* TLV list as specified in struct
				    * _NAN_ATTR_NDPE_IPV6_LINK_LOCAL_TLV_T, or
				    * _NAN_ATTR_NDPE_SVC_INFO_TLV_T
				    */
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 139. Vendor Specific attribute format
 * TODO (different from standard defined structure)
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_VENDOR_SPECIFIC_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_VENDOR_SPECIFIC */
	uint16_t u2Length;
	uint8_t aucOui[VENDOR_OUI_LEN]; /* NAN_OUI */
	uint8_t ucVendorSpecificOuiType;
	uint16_t u2SubAttrLength;
	uint8_t aucVendorSpecificOuiData[];      /*to be defined*/
} __KAL_ATTRIB_PACKED__;


/* NAN Cluster attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_CLUSTER_T {
	uint8_t ucId;
	uint16_t u2Length;
	uint8_t aucAnchorMasterRank[ANCHOR_MASTER_RANK_NUM];
	uint8_t ucHopCount;
	uint32_t u4AMBTT;
} __KAL_ATTRIB_PACKED__;

/* NAN Master Indication Attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_MASTER_INDICATION_T {
	uint8_t ucId;
	uint16_t u2Length;
	uint8_t ucMasterPreference;
	uint8_t ucRandomFactor;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 100. Channel Entry format for the NAN Availability attribute */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_SIMPLE_CHNL_ENTRY_T {
	uint8_t ucOperatingClass;
	union {
		uint16_t u2ChannelBitmap;
		struct {
			uint8_t ucChannelStart;
			uint8_t ucChannelNum;
		};
	};
	uint8_t ucPrimaryChnlBitmap;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 98. Band/Channel Entries List field format for the NAN
 * Availability attribute
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_SIMPLE_BAND_CHNL_LIST_T {
	uint8_t ucType : 1;
	uint8_t ucNonContiguous : 1;
	uint8_t ucRsvd : 2;
	uint8_t ucNumberOfEntry : 4;
	struct _NAN_SIMPLE_CHNL_ENTRY_T rChnlEntry;
} __KAL_ATTRIB_PACKED__;

/**
 * R4 Table 96. Entry Control field format for the NAN Availability attribute
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_AVAILABILITY_ENTRY_CONTROL_T {
	uint16_t u2TypeCommitted : 1,
		 u2TypePotential : 1,
		 u2TypeConditional : 1,
		 u2UsagePreference : 2,
		 u2Utilization : 3,
		 u2RxNss : 4,
		 u2TimeBitmapPresent : 1,
		 u2Reserved : 3;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 97. Time Bitmap Control field format for NAN Availability */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_TIME_BITMAP_CONTROL_T {
	uint16_t u2BitDuration :3,	/* BITS(0,2), 16 << n TU, n < 4 */
		 u2Period :3,		/* BITS(3,5), 128 << (n-1) TU, n > 0 */
		 u2StartOffset :9,	/* BITS(6,14), 16 * n TU */
		 u2Reserved :1;		/* BIT(15) */
} __KAL_ATTRIB_PACKED__;

/**
 * R4 Table 95. Availability Entry field format for NAN Availability attribute
 * if _NAN_ATTR_NAN_AVAILABILITY_T.u2AttributeControl.TimeBitmapPresent == 1
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_AVAILABILITY_ENTRY_SIMPLE_T {
	uint16_t u2Length;
	union {
		uint16_t u2EntryControl;
		struct _NAN_AVAILABILITY_ENTRY_CONTROL_T rCtrl;
	};
	union {
		uint16_t u2TimeBitmapControl;
		struct _NAN_ATTR_TIME_BITMAP_CONTROL_T rTimeBitmapCtrl;
	};
	uint8_t ucTimeBitmapLength; /* 4 */
	union {
		uint8_t aucTimeBitmap[4];
		uint32_t u4TimeBitmap;
	};
	struct _NAN_SIMPLE_BAND_CHNL_LIST_T channelEntry;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 98. Band/Channel Entries List field format for the NAN
 * Availability attribute
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_BAND_CHNL_LIST_T {
	uint8_t ucType : 1;
	uint8_t ucNonContiguous : 1;
	uint8_t ucRsvd : 2;
	uint8_t ucNumberOfEntry : 4;
	uint8_t aucEntry[]; /* if ucType==0, Band entries;
			     * if ucType==1, Channel entries as
			     * struct _NAN_CHNL_ENTRY_T
			     */
} __KAL_ATTRIB_PACKED__;

/**
 * NAN 4.0 Table 95
 * A more specific structure of struct _NAN_AVAILABILITY_ENTRY_T, with
 * Time Bitmap Present == 1 in u2EntryControl (Table 96)
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T {
	uint16_t u2Length;
	union {
		uint16_t u2EntryControl;
		struct _NAN_AVAILABILITY_ENTRY_CONTROL_T rCtrl;
	};
	union {
		uint16_t u2TimeBitmapControl;
		struct _NAN_ATTR_TIME_BITMAP_CONTROL_T rTimeBitmapCtrl;
	};
	uint8_t ucTimeBitmapLength;
	uint8_t aucTimeBitmapAndBandChnl[];
} __KAL_ATTRIB_PACKED__;

/**
 * NAN 4.0 Table 95
 * Generic structure
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_AVAILABILITY_ENTRY_T {
	uint16_t u2Length;
	union {
		uint16_t u2EntryControl;
		struct _NAN_AVAILABILITY_ENTRY_CONTROL_T rCtrl;
	};
	uint8_t aucTimeBitmapAndBandChnlEntry[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 94. Attribute Control field format for the NAN Availability
 * attribute
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NAN_ATTRIBUTE_CONTROL_T {
	uint16_t u2MapID : 4,
		 u2CommittedChanged : 1,
		 u2PotentialChanged : 1,
		 u2PublicAvailabilityChanged : 1,
		 u2NDCChanged : 1,
		 u2McastScheduleChanged : 1,
		 u2McastScheduleChangeChanged : 1,
		 u2Reserved : 6;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NAN_AVAILABILITY_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_NAN_AVAILABILITY */
	uint16_t u2Length;
	uint8_t ucSeqID;
	union {
		uint16_t u2AttributeControl;
		struct _NAN_ATTR_NAN_ATTRIBUTE_CONTROL_T rAttrCtrl;
	};
	uint8_t aucAvailabilityEntryList[]; /* _NAN_AVAILABILITY_ENTRY_T */
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 104. Schedule Entry format of the NDC attribute */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_SCHEDULE_ENTRY_T {
	uint8_t ucMapID : 4,
		reserved : 4;
	uint16_t u2TimeBitmapControl;
	uint8_t ucTimeBitmapLength;
	uint8_t aucTimeBitmap[];
} __KAL_ATTRIB_PACKED__;


/* NAN 4.0 Table 102. NDC attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDC_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_NDC */
	uint16_t u2Length;
	uint8_t aucNDCID[6];
	union { /* Table 103. Attribute Control field format for NDC */
		uint8_t ucAttributeControl;
		struct {
			uint8_t selected_ndc :1,
				reserved :7;
		};
	};
	uint8_t aucScheduleEntryList[]; /* structure of _NAN_SCHEDULE_ENTRY_T */
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 105. NDL attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDL_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_NDL */
	uint16_t u2Length;
	uint8_t ucDialogToken;
	union {
		uint8_t ucTypeStatus;
		struct {
			uint8_t ucType :4,	/* NAN_ATTR_NDL_TYPE_* */
				ucStatus :4;	/* NAN_ATTR_NDL_STATUS_* */
		};
	};
	uint8_t ucReasonCode;
	/* NAN 4.0 Table 107. NDL Control field format */
	union {
		uint8_t ucNDLControl;
		struct {
			uint8_t b1PeerIdPresent :1;
			uint8_t b1ImmutableSchedPresent :1;
			uint8_t b1NdcAttrPresent :1;
			uint8_t b1NdlQosAttrPresent :1;
			uint8_t b1MaxIdlePeriodPresent :1;
			uint8_t b1NdlType :1;
			uint8_t b2NdlSetupReason :2;
		};
	};
	uint8_t ucNDLPeerID;		/* optional */
	uint16_t u2MaxIdlePeriod;	/* optional */
	uint8_t aucImmutableSchedule[]; /* optional: NAN_SCHEDULE_ENTRY_T */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_NDL_QOS_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_NDL_QOS */
	uint16_t u2Length;
	uint8_t ucMinTimeSlot;
	uint16_t u2MaxLatency;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_UNALIGNED_SCHEDULE_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_UNALIGNED_SCHEDULE */
	uint16_t u2Length;
	union {
		uint16_t u2AttributeControl;
		struct {
			uint16_t b4ScheduleId :4;
			uint16_t b4AttrCtrlReserved :4;
			uint16_t b8SequenceId :8;
		};
	};
	uint32_t u4StartingTime;
	uint32_t u4Duration;
	uint32_t u4Period;
	uint8_t ucCountDown;
	union {
		uint8_t ucULWOverwrite;
		struct {
			uint8_t b1OverwriteAll :1;
			uint8_t b4OverwriteMapId :4;
			uint8_t b3OverwriteReserved :3;
		};
	};
	uint8_t aucULWControlBandIDChannelEntry[];
	/* ULW Control (O) + BandID/ChannelEntry*/
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_ELEMENT_CONTAINER_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_ELEMENT_CONTAINER */
	uint16_t u2Length;
	uint8_t ucMapID;
	uint8_t aucElements[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 121. Cipher Sutie attribute field format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_CIPHER_SUITE_ATTRIBUTE_T {
	uint8_t ucCipherSuiteID;
	uint8_t ucPublishID;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 122. Cipher Suite Information attribute (CSIA) field format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_CIPHER_SUITE_INFO_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_CIPHER_SUITE_INFO */
	uint16_t u2Length;
	uint8_t ucCapabilities;
	uint8_t aucCipherSuiteList[]; /* struct _NAN_CIPHER_SUITE_ATTRIBUTE_T */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_SECURITY_CONTEXT_ID_T {
	uint16_t u2SecurityContextIDTypeLength;
	uint8_t ucSecurityContextIDType;
	uint8_t ucPublishID;
	uint8_t aucSecurityContextID[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_SECURITY_CONTEXT_INFO_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_SECURITY_CONTEXT_INFO */
	uint16_t u2Length;
	uint8_t aucSecurityContextIDList[]; /* NAN_SECURITY_CONTEXT_ID_T */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_SHARED_KEY_DESCRIPTOR_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR */
	uint16_t u2Length;
	uint8_t ucPublishID;
	uint8_t aucRSNAKeyDescriptor[];
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 100. Channel Entry format for the NAN Availability attribute */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_CHNL_ENTRY_T {
	uint8_t ucOperatingClass;
	union {
		uint16_t u2ChannelBitmap;
		struct {
			uint8_t ucChannelStart;
			uint8_t ucChannelNum;
		};
	};
	uint8_t ucPrimaryChnlBitmap;
	uint16_t u2AuxChannelBitmap; /* optional, present if ucNonContiguous */
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 100. Channel Entry format for the NAN Availability attribute
 * without Non-contiguous bandwidth bit set in Table 98.
 */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_CHNL_ENTRY_NO_AUX_T {
	uint8_t ucOperatingClass;
	union {
		uint16_t u2ChannelBitmap;
		struct {
			uint8_t ucChannelStart;
			uint8_t ucChannelNum;
		};
	};
	uint8_t ucPrimaryChnlBitmap;
	/* uint16_t u2AuxChannelBitmap; optional, present if ucNonContiguous */
} __KAL_ATTRIB_PACKED__;


__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_FTM_PARAMETERS_T {
	uint32_t max_burst_duration : 4;
	uint32_t min_delata_ftm : 6;
	uint32_t max_ftms_per_brrst : 5;
	uint32_t ftm_format_and_bandwidth : 6;
	uint32_t reserved : 3;
} __KAL_ATTRIB_PACKED__;

/* NAN Ranging Info attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_RANGING_INFO_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_RANGING_INFORMATION */
	uint16_t u2Length;
	uint8_t ucLocationInfo;
	uint16_t u2LastMovement;
} __KAL_ATTRIB_PACKED__;

/* NAN 4.0 Table 118. Ranging Setup attribute format */
/* NAN Ranging Setup attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_RANGING_SETUP_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_RANGING_SETUP */
	uint16_t u2Length;
	uint8_t ucDialogToken;
	union {
		uint8_t ucTypeStatus;
		struct {
			uint8_t ucType :4, /* NAN_RANGING_TYPE_* */
				ucStatus :4; /* NAN_RANGING_STATUS_*  */
		};
	};
	uint8_t ucReasonCode;
	uint8_t ucRangingCtl;
	struct _NAN_ATTR_FTM_PARAMETERS_T rFtmParameter;
	uint8_t aucScheduleEntryList[];
	/* in structure of NAN_SCHEDULE_ENTRY_T */
} __KAL_ATTRIB_PACKED__;

/* NAN FTM Range Report attribute format */
__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_FTM_RANGE_REPORT_T {
	uint8_t ucAttrId; /* NAN_ATTR_ID_FTM_RANGING_REPORT */
	uint16_t u2Length;
	uint8_t aucFtmRangeReport[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_SDA_T {
	uint8_t ucAttribID;
	uint16_t u2Len;
	uint8_t aucServiceID[NAN_SERVICE_HASH_LENGTH];
	uint8_t ucInstanceID;
	uint8_t ucRequesterID;
	uint8_t ucServiceControl;
	uint8_t ucANASDFdetail[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct _NAN_ATTR_SDEA_T {
	uint8_t ucAttribID;
	uint16_t u2Len;
	uint8_t ucInstanceID;
	uint16_t u2Control;
	uint8_t ucSDEAdetail[];
} __KAL_ATTRIB_PACKED__;

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
#define NAN_GET_U8(var) (*((uint8_t *)var))
#define NAN_GET_U16(var) ((((uint16_t)(var)[1]) << 8) | ((uint16_t)(var)[0]))
#define NAN_GET_U32(var)                                                  \
	((((uint32_t)(var)[3]) << 24) | (((uint32_t)(var)[2]) << 16) |  \
	 (((uint32_t)(var)[1]) << 8) | ((uint32_t)(var)[0]))

#define NAN_ATTR_ID(fp) (((struct _NAN_ATTR_HDR_T *)fp)->ucAttrId)
#define NAN_ATTR_LEN(fp) (((struct _NAN_ATTR_HDR_T *)fp)->u2Length)
#define NAN_ATTR_SIZE(fp) (NAN_ATTR_HDR_LEN + NAN_ATTR_LEN(fp))

#define NAN_ATTR_FOR_EACH(_pucBuf, _u2BufLen, _u2Offset)                  \
	for ((_u2Offset) = 0; ((_u2Offset) < (_u2BufLen));               \
	     (_u2Offset) += NAN_ATTR_SIZE(_pucBuf),                      \
	    ((_pucBuf) += NAN_ATTR_SIZE(_pucBuf)))

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#endif
#endif /* _NAN_BASE_H_ */
