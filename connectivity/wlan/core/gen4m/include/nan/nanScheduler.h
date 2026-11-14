/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_SCHEDULER_H_
#define _NAN_SCHEDULER_H_

#if CFG_SUPPORT_NAN

#define NAN_2G_DW_INDEX 0
#define NAN_5G_DW_INDEX 8

#define NAN_2G_DEFAULT_NDC_INDEX (NAN_2G_DW_INDEX + 1)
#define NAN_5G_DEFAULT_NDC_INDEX (NAN_5G_DW_INDEX + 1)

#define NAN_5G_HIGH_DISC_CH_OP_CLASS 124
#define NAN_5G_HIGH_DISC_CHANNEL 149
#define NAN_5G_LOW_DISC_CH_OP_CLASS 115
#define NAN_5G_LOW_DISC_CHANNEL 44
#define NAN_2P4G_DISC_CH_OP_CLASS 81
#define NAN_2P4G_DISC_CHANNEL 6


#if (CFG_SUPPORT_NAN_6G == 1)
/* 6G chnl info */
#define NAN_6G_BW20_DEFAULT_CHANNEL	5
#define NAN_6G_BW40_DEFAULT_CHANNEL	3
#define NAN_6G_BW80_DEFAULT_CHANNEL	7
#define NAN_6G_BW160_DEFAULT_CHANNEL	15
#define NAN_6G_BW320_DEFAULT_CHANNEL	31

#define NAN_6G_BW20_CERT_DEFAULT_CHANNEL	37
#endif

#define NAN_5G_HIGH_BW40_DISC_CH_OP_CLASS 126
#define NAN_5G_LOW_BW40_DISC_CH_OP_CLASS 116
#define NAN_2P4G_BW40_DISC_CH_OP_CLASS 83

#define NAN_5G_HIGH_BW80_DISC_CH_OP_CLASS 128
#define NAN_5G_LOW_BW80_DISC_CH_OP_CLASS 128

#define NAN_5G_HIGH_BW160_DISC_CH_OP_CLASS 129
#define NAN_5G_LOW_BW160_DISC_CH_OP_CLASS 129
#define NAN_5G_BW160_DEF_CHANNEL 36

#if (CFG_SUPPORT_NAN_DBDC == 1)
#define NAN_FAW_OFFSET 1 /* slot 0 dedicate for 2.4G DW with single timeline */
#else
#define NAN_FAW_OFFSET 2 /* 1 slot for 2.4G/5G switch with single timeline */
#endif

#define NAN_SEND_PKT_TIME_SLOT 16
#define NAN_SEND_PKT_TIME_GUARD_TIME 2

#define NAN_MAX_NDC_RECORD (NAN_MAX_CONN_CFG + 1) /* one for default NDC */

#define NAN_SUPPORTED_2G_FAW_CH_NUM		4
#define NAN_SUPPORTED_5G_FAW_CH_NUM		4

/* the number of availability attribute per NAN station */
#define NAN_NUM_AVAIL_DB 2
/* the number of availability entry per NAN availability attribute */
#define NAN_NUM_AVAIL_TIMELINE 10
/* the number of BAND_CHNL entry per NAN availability entry attribute */
#define NAN_NUM_BAND_CHNL_ENTRY 5

#define NAN_NUM_PEER_SCH_DESC 50

#if (CFG_SUPPORT_NAN_DBDC == 1)
#define NAN_TIMELINE_MGMT_SIZE          2  /* need to align with FW */
#else
#define NAN_TIMELINE_MGMT_SIZE          1  /* need to align with FW */
#endif
#define NAN_TIMELINE_MGMT_CHNL_LIST_NUM \
	(NAN_SUPPORTED_2G_FAW_CH_NUM+NAN_SUPPORTED_5G_FAW_CH_NUM)
	/* need to align with FW */

#define NAN_MAX_POTENTIAL_CHNL_LIST 7 /* 5G 4 chnl entry + 6G 3 chnl entry */

#define NAN_CRB_NEGO_MAX_TRANSACTION 20

#define NAN_DW_INTERVAL 512
#define NAN_SLOT_INTERVAL 16

#define NAN_SLOTS_PER_DW_INTERVAL (NAN_DW_INTERVAL / NAN_SLOT_INTERVAL) /* 32 */

#define NAN_TOTAL_DW 16
#define NAN_TOTAL_SLOT_WINDOWS (NAN_TOTAL_DW * NAN_SLOTS_PER_DW_INTERVAL)

#define NAN_TIME_BITMAP_CONTROL_SIZE 2
#define NAN_TIME_BITMAP_LENGTH_SIZE 1
#define NAN_TIME_BITMAP_MAX_SIZE 64
#define NAN_TIME_BITMAP_FIELD_MAX_LENGTH                                       \
	(NAN_TIME_BITMAP_CONTROL_SIZE + NAN_TIME_BITMAP_LENGTH_SIZE +          \
	 NAN_TIME_BITMAP_MAX_SIZE)

#define NAN_MAX_CONN_CFG 8 /* the max supported concurrent NAN data path */

#define NAN_INVALID_QOS_MAX_LATENCY 65535
#define NAN_INVALID_QOS_MIN_SLOTS 0
#define NAN_QOS_MAX_LATENCY_LOW_BOUND 1
#define NAN_QOS_MAX_LATENCY_UP_BOUND 30
#define NAN_QOS_MIN_SLOTS_LOW_BOUND 1
#define NAN_QOS_MIN_SLOTS_UP_BOUND 30
#define NAN_DEFAULT_NDL_QUOTA_LOW_BOUND 3
#define NAN_DEFAULT_NDL_QUOTA_UP_BOUND 30
#define NAN_DEFAULT_RANG_QUOTA_LOW_BOUND 1
#define NAN_DEFAULT_RANG_QUOTA_UP_BOUND 3

/* Config bits for ucNanUseR4AvailAttr */
#define NAN_AVAIL_BIT 0
#define NAN_POTEN_BIT 1
#define NAN_R4_6G_CAP_BIT 2
#define NAN_REF_PEER_CAP_BIT 3
#define NAN_REF_PEER_AVAIL_BIT 4

/**
 * Merge potential to committed if the conditions holds:
 * 1. The bitmap length must be 4 (to be modified)
 * 2. The committed slots is less than ucNanMergePotentialThreshold
 * 3. The potential slots is equal or more than SUFFICIENT_POTENTIAL_SLOTS
 */
#define TYPICAL_BITMAP_LENGTH 4
#define NAN_FEW_COMMITTED_SLOTS 4
#define INSUFFICIENT_COMMITTED_SLOTS 16
#define SUFFICIENT_POTENTIAL_SLOTS 12

#define NAN_INVALID_MAP_ID 0xFF

#define NAN_PREFER_BAND_MASK_CUST_CH	0x1
#define NAN_PREFER_BAND_MASK_2G_DW_CH	0x2
#define NAN_PREFER_BAND_MASK_5G_DW_CH	0x3
#define NAN_PREFER_BAND_MASK_2G_BAND	0x4
#define NAN_PREFER_BAND_MASK_5G_BAND	0x5
#define NAN_PREFER_BAND_MASK_6G_BAND	0x6
#define NAN_PREFER_BAND_MASK_DFT		0xF

/* Predefined Ranging slots */
#define NAN_2G_DEFAULT_RANGING 0x000000FE /* slot 1~7 */
#define NAN_5G_DEFAULT_RANGING 0x0000FE00 /* slot 9~15 */

/* Predefined AIS slots for NAN-AIS concurrent */
#define NAN_DEFAULT_2G_AIS 0xFF00FF00
#define NAN_DEFAULT_5G_AIS 0x00FF00FF
#define NAN_DEFAULT_6G_AIS 0x00FF00FF

#define NAN_NON_DBDC_2G_AIS 0x00FF00FC
#define NAN_NON_DBDC_5G_AIS 0x00FF00FC
#define NAN_NON_DBDC_6G_AIS 0x00FF00FC
#define NAN_NON_DBDC_AIS 0x00FF00FC

#define NAN_T0_SLOT_MASK_TYPE_AIS 0xFF00FF00 /* 8~15, 23~31 */
#define NAN_SLOT_MASK_TYPE_AIS 0x00FF00FF /* 0~7, 16~23 */

#define NAN_T0_SLOT_MASK_TYPE_DEFAULT_NDL 0x00FF00FF /* 0~7, 16~23 */
#define NAN_SLOT_MASK_TYPE_DEFAULT_NDL 0xFF00FF00 /* 9~15, 24~31 */

#define NAN_SLOT_MASK_TYPE_DEFAULT 0xFFFFFFFF /* For NDP setup */

/* For P2P SCC concurrent, slot #0 for DW. #1, #2 for NDC and channel switch */
#define NAN_T0_SLOT_MASK_CONCURRENT_FULL 0xFFFFFFF8

/* For P2P SCC concurrent, slot #8, #9 are special for NDC and channel switch */
#define NAN_T1_SLOT_MASK_CONCURRENT_FULL 0xFFFFF8FF

#define NAN_DW_INDEX(__szSlotIdx) ((__szSlotIdx) / NAN_SLOTS_PER_DW_INTERVAL)
#define NAN_SLOT_INDEX(__szSlotIdx) ((__szSlotIdx) % NAN_SLOTS_PER_DW_INTERVAL)
#define NAN_FULL_SLOT_INDEX(__szDwIdx, __szSlotIdx) \
	((__szDwIdx) * NAN_SLOTS_PER_DW_INTERVAL + (__szSlotIdx))

#define NAN_INVALID_SCH_RECORD 0xFFFFFFFF
#define NAN_INVALID_AVAIL_DB_INDEX 0xFF
#define NAN_INVALID_AVAIL_ENTRY_INDEX 0xFF
#define NAN_INVALID_TIMELINE_EVENT_TAG 0xFF
#define NAN_INVALID_SLOT_INDEX 0xFFFF
#define NAN_INVALID_CHNL_ID 0xFF

#define NAN_IS_2G_TIMELINE(_prAdapter, _szTimeLineIdx)                  \
({                                                                      \
	const size_t sz2gTimeLineIdx =                                  \
		nanGetTimelineMgmtIndexByBand(_prAdapter, BAND_2G4);    \
	_szTimeLineIdx == sz2gTimeLineIdx;                              \
})

#define NAN_IS_5G_TIMELINE(_prAdapter, _szTimeLineIdx)                  \
({                                                                      \
	const size_t sz5gTimeLineIdx =                                  \
		nanGetTimelineMgmtIndexByBand(_prAdapter, BAND_5G);     \
	_szTimeLineIdx == sz5gTimeLineIdx;                              \
})

#define NAN_IS_6G_TIMELINE(_prAdapter, _szTimeLineIdx)                  \
({                                                                      \
	const size_t sz6gTimeLineIdx =                                  \
		nanGetTimelineMgmtIndexByBand(_prAdapter, BAND_6G);     \
	_szTimeLineIdx == sz6gTimeLineIdx;                              \
})

/* For single map, do not restrict slot_1 for CERT */
#define NAN_SLOT_IS_AIS(_prAdapter, _szTimeLineIdx, _szSlotIdx)		\
({									\
	uint32_t u4Bitmap = NAN_SLOT_MASK_TYPE_AIS;			\
	if (nanGetTimelineNum() == 1 &&					\
		!nanGetFeatureIsSigma(_prAdapter))			\
		u4Bitmap = NAN_NON_DBDC_AIS;				\
	if (nanGetTimelineNum() >= 2 &&					\
		NAN_IS_2G_TIMELINE(_prAdapter, _szTimeLineIdx))		\
		u4Bitmap = NAN_T0_SLOT_MASK_TYPE_AIS;			\
	(BIT(NAN_SLOT_INDEX(_szSlotIdx)) & u4Bitmap);			\
})

#define nanGetNdlSlots(_adapter)			\
	(NAN_SLOT_MASK_TYPE_DEFAULT_NDL & ~nanGetFcSlots(_adapter))	\

#define NAN_SLOT_IS_NDL(_adapter, _szTimeLineIdx, _szSlotIdx)		\
({									\
	uint32_t u4Bitmap = NAN_SLOT_MASK_TYPE_DEFAULT_NDL;		\
	uint32_t u4FcSlots = 0;				\
	if (nanGetTimelineNum() >= 2 &&					\
		NAN_IS_2G_TIMELINE(_adapter, _szTimeLineIdx))		\
		u4Bitmap = NAN_T0_SLOT_MASK_TYPE_DEFAULT_NDL;		\
	else								\
		u4FcSlots = nanGetFcSlots(_adapter);			\
	BIT(NAN_SLOT_INDEX(_szSlotIdx)) & (u4Bitmap & ~u4FcSlots);	\
})

#define NAN_SLOT_IS_FC(_adapter, _szSlotIdx)				\
	(BIT(NAN_SLOT_INDEX(_szSlotIdx)) & nanGetFcSlots(_adapter))
#define NAN_SLOT_TIMELINE_IS_FC(_adapter, _szTimelineIdx, _szSlotIdx)	\
	(BIT(NAN_SLOT_INDEX(_szSlotIdx)) &				\
	 nanGetTimelineFcSlots(_adapter, _szTimelineIdx, _szSlotIdx))
#define NAN_SLOT_IS_NDC(_szTimeLineIdx, _szSlotIdx)			\
	(_szTimeLineIdx ==						\
		 nanGetTimelineMgmtIndexByBand(prAdapter, BAND_5G) &&	\
	  NAN_SLOT_INDEX(_szSlotIdx) == NAN_5G_DEFAULT_NDC_INDEX ||	\
	 _szTimeLineIdx ==						\
		 nanGetTimelineMgmtIndexByBand(prAdapter, BAND_2G4) &&	\
	 NAN_SLOT_INDEX(_szSlotIdx) == NAN_2G_DEFAULT_NDC_INDEX)

#define NAN_SLOT_IS_NDC_BY_BAND(_szTimeLineIdx, _szSlotIdx, _band)	\
	(_szTimeLineIdx ==						\
		 nanGetTimelineMgmtIndexByBand(prAdapter, _band) &&	\
	 NAN_SLOT_INDEX(_szSlotIdx) ==					\
		 (_band == BAND_2G4 ? NAN_2G_DEFAULT_NDC_INDEX :	\
				      NAN_5G_DEFAULT_NDC_INDEX))

#define NAN_SLOT_IS_RANGING_BY_BAND(_szTimeLineIdx, _szSlotIdx, _band)	\
	(_szTimeLineIdx ==						\
		 nanGetTimelineMgmtIndexByBand(prAdapter, _band) &&	\
	 BIT(NAN_SLOT_INDEX(_szSlotIdx)) &				\
		 (_band == BAND_2G4 ? NAN_2G_DEFAULT_RANGING :	\
				      NAN_5G_DEFAULT_RANGING))

/**
 * NAN 4.0 Figure 72. Availability Schedule for Instant Communication
 * slot 0: 2.4G DW (Ch 6)
 * slot 1: 2.4G NDC (Ch 6)
 * slot 2~23: Instant Communication Channel (>= 352 TU) (NAN Discovery Channel)
 * slot 24~31: Other Channel (<= 128 TU)
 */
#define NAN_ICM_DEFAULT_BITMAP 0x00FFFFFC

/* Limited log */
#define NAN_DW_DBGLOG(Mod, Clz, Print, Index, Fmt, ...)			\
	do {								\
		if (Print && Index < NAN_SLOTS_PER_DW_INTERVAL)		\
			DBGLOG(Mod, Clz, Fmt, __VA_ARGS__);		\
	} while (0)

#define NAN_IS_CHANNEL_6G(_rChnlInfo) \
	(IS_6G_OP_CLASS((_rChnlInfo).u4OperatingClass))

#define NAN_IS_CHANNEL_5G_HIGH(_rChnlInfo) \
	(IS_5G_OP_CLASS((_rChnlInfo).u4OperatingClass) && \
	 (_rChnlInfo).u4PrimaryChnl >= 149 && \
	 (_rChnlInfo).u4PrimaryChnl <= 165)

#define NAN_IS_CHANNEL_5G_LOW(_rChnlInfo) \
	(IS_5G_OP_CLASS((_rChnlInfo).u4OperatingClass) && \
	 (_rChnlInfo).u4PrimaryChnl >= 36 && \
	 (_rChnlInfo).u4PrimaryChnl <= 48)

#define NAN_IS_CHANNEL_5G(_rChnlInfo) \
	(NAN_IS_CHANNEL_5G_LOW(_rChnlInfo) || \
	 NAN_IS_CHANNEL_5G_HIGH(_rChnlInfo))

#define NAN_IS_CHANNEL_2G(_rChnlInfo) \
	(IS_2G_OP_CLASS((_rChnlInfo).u4OperatingClass))

#define NAN_GET_CHNL_BAND_INFO(_rChnlInfo) \
({ \
	enum _NAN_SUPPORTED_BAND_BIT eBand = ENUM_SUPPORTED_BN_NUM; \
	if (NAN_IS_CHANNEL_6G(_rChnlInfo)) \
		eBand = ENUM_SUPPORTED_BN_6G; \
	else if (NAN_IS_CHANNEL_5G_HIGH(_rChnlInfo)) \
		eBand = ENUM_SUPPORTED_BN_5G_HIGH; \
	else if (NAN_IS_CHANNEL_5G_LOW(_rChnlInfo)) \
		eBand = ENUM_SUPPORTED_BN_5G_LOW; \
	else if (NAN_IS_CHANNEL_2G(_rChnlInfo)) \
		eBand = ENUM_SUPPORTED_BN_2G; \
	eBand; \
})

#define NAN_CHNL_IN_COMMON_BAND(_prAdapter, _prNegoCtrl, _rChnl) \
({ \
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord = NULL; \
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL; \
	uint32_t u4CommonBand = 0; \
	if (_prNegoCtrl) \
		prPeerSchRecord = nanSchedGetPeerSchRecord(_prAdapter, \
					     (_prNegoCtrl)->u4SchIdx); \
	if (prPeerSchRecord) \
		prPeerSchDesc = prPeerSchRecord->prPeerSchDesc; \
	if (prPeerSchDesc)	\
		u4CommonBand = prPeerSchDesc->u4CommonSupportedBand; \
	(u4CommonBand & BIT(ENUM_SUPPORTED_BN_6G) && \
	 NAN_IS_CHANNEL_6G(_rChnl) || \
	 u4CommonBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH) && \
	 NAN_IS_CHANNEL_5G_HIGH(_rChnl) || \
	 u4CommonBand & BIT(ENUM_SUPPORTED_BN_5G_LOW) && \
	 NAN_IS_CHANNEL_5G_LOW(_rChnl) || \
	 u4CommonBand & BIT(ENUM_SUPPORTED_BN_2G) && \
	 NAN_IS_CHANNEL_2G(_rChnl)); \
})


#define NAN_IS_TIMELINE_MATCH_BAND(_prAdapter, _szTimeline, _eBand)     \
	(nanGetTimelineMgmtIndexByBand(_prAdapter, _eBand) == _szTimeline)


/* Define 0-index NAN band index for addressing array based on enum ENUM_BAND */
enum NAN_BAND_IDX {
	NAN_2G_IDX, /* 0 */
	NAN_5G_IDX, /* 1 */
#if CFG_SUPPORT_WIFI_6G  == 1
	NAN_6G_IDX, /* 2 */
#endif
	/* NOTE: NAN_BAND_NUM is used for au4NanAisSlots storing AIS bitmaps,
	 * mind the inconsistency of CFG_SUPPORT_WIFI_6G and CFG_SUPPORT_NAN_6G
	 */
	NAN_BAND_NUM, /* 2 or 3 depends on if CFG_SUPPORT_WIFI_6G is enabled */
};

struct _NAN_AIS_BITMAP {
	u_int8_t fgCustSet;
	uint32_t u4Bitmap;
};

enum _NAN_CHNL_BW_MAP {
	NAN_CHNL_BW_20 = 0,
	NAN_CHNL_BW_40,
	NAN_CHNL_BW_80,
	NAN_CHNL_BW_160,
	NAN_CHNL_BW_320,
	NAN_CHNL_BW_NUM
};

enum _NAN_SUPPORTED_BAND_BIT {
	ENUM_SUPPORTED_BN_2G = 0,
	ENUM_SUPPORTED_BN_5G_LOW,
	ENUM_SUPPORTED_BN_5G_HIGH,
	ENUM_SUPPORTED_BN_6G,
	ENUM_SUPPORTED_BN_NUM
};

/* Porting from FW, TODO: move to correct file */
enum _ENUM_CNM_CH_CONCURR_T {
	CNM_CH_CONCURR_MCC,
	CNM_CH_CONCURR_SCC_NEW,
	CNM_CH_CONCURR_SCC_CURR,
	CNM_CH_CONCURR_NUM
};

struct NAN_EVT_NDL_FLOW_CTRL {
	uint16_t au2FlowCtrl[NAN_MAX_CONN_CFG];
};

struct NAN_FLOW_CTRL {
	uint32_t u4ExpiryTime;
};

union _NAN_BAND_CHNL_CTRL {
	struct {
		uint32_t u4Type : 1;
		uint32_t u4Rsvd : 31;
	}; /* u4Type to distinguish band or channel */

	struct /* _NanBandCtrl */ { /* NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL:0 */
		uint32_t u4BandType : 1;
		uint32_t u4BandRsvd : 23;
		/* Table 99, same to enum NAN_SUPPORTED_BANDS */
		uint32_t u4BandIdMask : 8;
	} /* rBand */;

	struct /* _NanChannelCtrl */ { /* NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL:1 */
		uint32_t u4ChnlType : 1;
		uint32_t u4ChnlRsvd : 7;
		uint32_t u4OperatingClass : 8;
		uint32_t u4PrimaryChnl : 8;
		uint32_t u4AuxCenterChnl : 8;
	} /* rChannel */;

	uint32_t u4RawData;
};

struct NAN_EVT_NDL_FLOW_CTRL_V2 {
	uint16_t au2RemainingTime[NAN_MAX_CONN_CFG];
	uint16_t u2SeqNum;
	uint16_t u2Reserved[7];
	union _NAN_BAND_CHNL_CTRL arBandChnlInfo[NAN_MAX_CONN_CFG];
};

enum _ENUM_NAN_NEGO_TYPE_T {
	ENUM_NAN_NEGO_DATA_LINK,
	ENUM_NAN_NEGO_RANGING,
	ENUM_NAN_NEGO_NUM
};

enum _ENUM_NAN_NEGO_ROLE_T {
	ENUM_NAN_NEGO_ROLE_INITIATOR,
	ENUM_NAN_NEGO_ROLE_RESPONDER,
	ENUM_NAN_NEGO_ROLE_NUM
};

struct _NAN_SCHEDULE_TIMELINE_T {
	uint8_t ucMapId; /* peer MapId or local generated MapId */
	uint8_t ucLocalMapId; /* TX NDC attribute, peer MapId ?!= local MapId */
	uint8_t aucRsvd[2];

	union {
		uint32_t au4AvailMap[NAN_TOTAL_DW];
		uint8_t aucAvailBlock[NAN_TOTAL_DW * 4];
	};
};

enum NDP_PS_STATE {
	NDP_PS_NONE,
	NDP_ENTER_PS, /* in power aving state */
	NDP_RESUME_FROM_PS, /* active from power saving */
};

struct _NAN_NDL_CUSTOMIZED_T {
	u_int8_t fgBitmapSet;
	enum NDP_PS_STATE ePowerSaving;
	uint8_t ucOpChannel;
	uint32_t u4Bitmap;
};

enum _ENUM_NAN_DEVCAP_FIELD_T {
	ENUM_NAN_DEVCAP_CAPABILITY_SET,
	ENUM_NAN_DEVCAP_TX_ANT_NUM,
	ENUM_NAN_DEVCAP_RX_ANT_NUM
};

struct _NAN_PHY_SETTING_T {
	uint8_t ucPhyTypeSet;
	uint8_t ucNonHTBasicPhyType;
	unsigned char fgUseShortPreamble;
	unsigned char fgUseShortSlotTime;

	uint16_t u2OperationalRateSet;
	uint16_t u2BSSBasicRateSet;

	uint16_t u2VhtBasicMcsSet;
	unsigned char fgErpProtectMode;
	uint8_t ucHtOpInfo1;

	uint16_t u2HtOpInfo2;
	uint16_t u2HtOpInfo3;

	enum ENUM_HT_PROTECT_MODE eHtProtectMode;
	enum ENUM_GF_MODE eGfOperationMode;
	enum ENUM_RIFS_MODE eRifsOperationMode;

	/* ENUM_CHNL_EXT_T eBssSCO; */
};

struct _NAN_SCHED_EVENT_NAN_ATTR_T {
	uint8_t aucNmiAddr[MAC_ADDR_LEN];
	uint8_t aucRsvd[2];
	uint8_t aucNanAttr[1000];
};

enum _ENUM_NAN_SCHED_NDL_DISCONNECT_REASON_T {
	ENUM_NAN_NDL_DISCONNECT_BY_KEEP_ALIVE = 0,
	ENUM_NAN_NDL_DISCONNECT_BY_AGING = 1,
	ENUM_NAN_NDL_DISCONNECT_REASON_NUM
};

struct _NAN_DEVICE_CAPABILITY_T {
	unsigned char fgValid;
	uint8_t ucMapId;
	uint8_t ucSupportedBand;
	union { /* as defined in struct _NAN_ATTR_DEVICE_CAPABILITY_T */
		uint8_t ucOperationMode;
		struct {
			uint8_t ucOperPhyModeVht :1, /* 0: HT only */
				ucOperPhyModeBw_80_80 :1,
				ucOperPhyModeBw_160 :1,
				ucOperPhyModeReserved :1,
				ucOperPhyModeHe :1,
				ucOperPhyModeReserved2 :3;
		};
	};
	uint8_t ucDw24g;
	uint8_t ucDw5g;
	uint8_t ucOvrDw24gMapId;
	uint8_t ucOvrDw5gMapId;
	uint8_t ucNumTxAnt;
	uint8_t ucNumRxAnt;
	uint8_t ucCapabilitySet;

	uint16_t u2MaxChnlSwitchTime;
};

struct _NAN_NDC_CTRL_T {
	unsigned char fgValid;
	uint8_t aucNdcId[NAN_NDC_ATTRIBUTE_ID_LENGTH];
	uint8_t aucRsvd[1];

	/* The ucMapId shall set according to prNanTimelineMgmt->ucMapId
	 * Ref: nanSchedNegoAllocNdcCtrl
	 */
	struct _NAN_SCHEDULE_TIMELINE_T arTimeline[NAN_NUM_AVAIL_DB];
};

struct _NAN_FAW_NDC_TIMELINE_T {
	uint32_t au4AvailMap[NAN_TOTAL_DW];
};

/* Table 96 */
union _NAN_AVAIL_ENTRY_CTRL {
	struct {
		uint16_t b3Type :3,
			 b16Reserved : 13;
	};
	struct {
		uint16_t b1Committed :1,
			 b1Potential :1,
			 b1Conditional :1,
			 b2Preference :2,
			 b3Util :3,
			 b4RxNss : 4,
			 b1TimeMapAvail :1,
			 b3Rsvd : 3;
	} /* rField */;

	uint16_t u2RawData;
};

struct _NAN_AVAILABILITY_TIMELINE_T {
	unsigned char fgActive;

	union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl;

	uint8_t ucNumBandChnlCtrl;
	union _NAN_BAND_CHNL_CTRL arBandChnlCtrl[NAN_NUM_BAND_CHNL_ENTRY];

	uint32_t au4AvailMap[NAN_TOTAL_DW];
};

struct _NAN_AVAILABILITY_DB_T {
	uint8_t ucMapId;
	uint32_t u4AvailAttrToken;
	struct _NAN_AVAILABILITY_TIMELINE_T
		arAvailEntryList[NAN_NUM_AVAIL_TIMELINE];
};

enum NAN_RX_PEER_SPECIAL_AVAIL {
	NAN_PEER_AVAIL_FORCE_NONE = 0,
	NAN_PEER_AVAIL_FORCE_R3 = 1,
	NAN_PEER_AVAIL_FORCE_R4 = 2,
};

struct _NAN_PEER_SCH_DESC_T {
	struct LINK_ENTRY rLinkEntry;

	uint8_t aucNmiAddr[MAC_ADDR_LEN];

	OS_SYSTIME rUpdateTime;

	unsigned char fgUsed;   /* indicate the SCH DESC is used by a SCH REC */
	uint32_t u4SchIdx; /* valid only when fgUsed is TRUE */

	struct _NAN_AVAILABILITY_DB_T arAvailAttr[NAN_NUM_AVAIL_DB];

	uint32_t u4DevCapAttrToken;
	struct _NAN_DEVICE_CAPABILITY_T arDevCapability[NAN_NUM_AVAIL_DB + 1];

	uint32_t u4QosMinSlots;
	uint32_t u4QosMaxLatency;

	/* for peer proposal cache during shedule negotiation */
	struct _NAN_NDC_CTRL_T rSelectedNdcCtrl;

	unsigned char fgImmuNdlTimelineValid;
	struct _NAN_SCHEDULE_TIMELINE_T arImmuNdlTimeline[NAN_NUM_AVAIL_DB];

	unsigned char fgRangingTimelineValid;
	struct _NAN_SCHEDULE_TIMELINE_T arRangingTimeline[NAN_NUM_AVAIL_DB];

	/* MERGE_POTENTIAL */
	uint32_t u4MergedCommittedChannel;
	uint8_t aucPotMergedBitmap[TYPICAL_BITMAP_LENGTH];

#if (CFG_SUPPORT_NAN_11BE == 1)
	u_int8_t fgEht;
#endif
	/* Intersection of supported band between self and this peer */
	uint32_t u4CommonSupportedBand;
	uint32_t u4PeerSupportedBand;

	/* Record band with sufficient committed slots */
	uint32_t u4CommittedBand;

	enum NAN_RX_PEER_SPECIAL_AVAIL ePeerForceAvailAttr;

	uint8_t ucLastAvailabilitySequence;
};

/** NAN Peer Schedule Record */
struct _NAN_PEER_SCHEDULE_RECORD_T {
	uint8_t aucNmiAddr[MAC_ADDR_LEN];
	unsigned char fgActive;
	unsigned char fgUseRanging;
	unsigned char fgUseDataPath;

	uint32_t u4DefNdlNumSlots;
	uint32_t u4DefRangingNumSlots;

	uint8_t aucStaRecIdx[NAN_LINK_NUM][NAN_MAX_SUPPORT_NDP_CXT_NUM];

	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	/* [Common timeline information]
	 * 1. not support simultaneous availability schedule on local side
	 * 2. the Map ID refers to the local availability schedule
	 *    (the Map ID between rmt & local doesn't need to be same)
	 */
	struct _NAN_NDC_CTRL_T *prCommNdcCtrl;
	struct _NAN_SCHEDULE_TIMELINE_T
		arCommImmuNdlTimeline[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_SCHEDULE_TIMELINE_T
		arCommRangingTimeline[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_SCHEDULE_TIMELINE_T
		arCommFawTimeline[NAN_TIMELINE_MGMT_SIZE];

	uint32_t u4FinalQosMinSlots;
	uint32_t u4FinalQosMaxLatency;

	int32_t i4InNegoContext;
	enum ENUM_BAND eBand; /* peer use band */

#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
	/* Logging Flow Control V2 state and duration on handling event */
	struct NAN_FLOW_CTRL rNanFlowCtrlRecord[NAN_MAX_CONN_CFG];
#endif

	struct _NAN_NDL_CUSTOMIZED_T arCustomized[NAN_BAND_NUM];
	u_int8_t fgDef5GNDCConflict;
	u_int8_t fgInPowerSaving; /* intended PS with data stopped */
};

struct _NAN_CHANNEL_TIMELINE_T {
	unsigned char fgValid;
	uint8_t aucRsvd[3];

	union _NAN_BAND_CHNL_CTRL rChnlInfo;

	int32_t i4Num; /* track the number of slot in availability map */
	uint32_t au4AvailMap[NAN_TOTAL_DW];
};

struct _NAN_TIMELINE_MGMT_T {
	uint8_t ucMapId;

	/* for committed availability type */
	struct _NAN_CHANNEL_TIMELINE_T
		arChnlList[NAN_TIMELINE_MGMT_CHNL_LIST_NUM];

	/* for conditional availability type */
	unsigned char fgChkCondAvailability;
	struct _NAN_CHANNEL_TIMELINE_T
		arCondChnlList[NAN_TIMELINE_MGMT_CHNL_LIST_NUM];

	/* for custom committed FAW */
	struct _NAN_CHANNEL_TIMELINE_T
		arCustChnlList[NAN_TIMELINE_MGMT_CHNL_LIST_NUM];
};

struct _NAN_CUST_FAW_ENTRY {
	const char *pcTag;
	uint8_t ucOpChannel;
	enum ENUM_BAND eBand;
	uint32_t u4Bitmap;
};

struct NAN_P2P_AIS_MCC_RECORD {
	u_int8_t fgIsP2pAisMCC;
	u_int8_t ucNumOfChannel;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo;
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo;
};

/* NAN Scheduler Control Block */
struct _NAN_SCHEDULER_T {
	unsigned char fgInit;

	/**
	 * ENUM_SUPPORTED_BN_2G = 0,
	 * ENUM_SUPPORTED_BN_5G_LOW,
	 * ENUM_SUPPORTED_BN_5G_HIGH,
	 * ENUM_SUPPORTED_BN_6G,
	 */
	union {
		struct {
			uint8_t fgEn2g :1,
				fgEn5gL :1,
				fgEn5gH :1,
				fgEn6g :1,
				b4Reserved :4;
		};
		uint8_t ucLocalSupportedBand;
	};
	unsigned char fgIs6gInUse;
	enum _NAN_CHNL_BW_MAP e6gBandwidth;

	uint8_t ucNanAvailAttrSeqId; /* shared by all availability attr */
	uint16_t u2NanAvailAttrControlField;     /* tracking changed flags */
	uint16_t u2NanCurrAvailAttrControlField; /* tracking changed flags */

	struct _NAN_NDC_CTRL_T arNdcCtrl[NAN_MAX_NDC_RECORD];

	struct LINK rPeerSchDescList;
	struct LINK rFreePeerSchDescList;
	struct _NAN_PEER_SCH_DESC_T arPeerSchDescArray[NAN_NUM_PEER_SCH_DESC];

	unsigned char fgAttrDevCapValid;
	struct _NAN_ATTR_DEVICE_CAPABILITY_T rAttrDevCap;

	uint32_t au4NumOfPotentialChnlList[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_CHNL_ENTRY_T aarPotentialChnlList[NAN_TIMELINE_MGMT_SIZE]
		[NAN_MAX_POTENTIAL_CHNL_LIST];

	struct TIMER rAvailAttrCtrlResetTimer;
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	struct TIMER rResumeRescheduleTimer;
#endif

	uint8_t ucCommitDwInterval;

	/* Customized NDP scheduling to all peers */
	struct _NAN_NDL_CUSTOMIZED_T arGlobalCustomized[NAN_BAND_NUM];

	uint8_t ucNdcBand; /* band bitmap of NDC, enum NAN_BSS_ROLE_INDEX */

	struct _NAN_CUST_FAW_ENTRY arCustFawEntry[20];

	/* Store P2P/AIS channel info by timeline index */
	struct NAN_P2P_AIS_MCC_RECORD arP2pAisMcc[NAN_TIMELINE_MGMT_SIZE];

	/* Last customized channel for concurrency to avoid redundant set */
	struct _NAN_CUST_FAW_ENTRY arConcurrentCust[2]; /* 2 & 5/6 G bands */

	/* Instant Communication */
	u_int8_t fgNanInstantMode;
	uint32_t u4NanInstantModeChannel; /* frequency, e.g., 2437 for ch 6 */
	uint32_t u4NanInstantModeBitmap;

	/* Local committed band */
	enum _NAN_SUPPORTED_BAND_BIT eLocalCommittedBand;

	/* store for reschedule after PS peers resumed */
	enum RESCHEDULE_SOURCE eRescheduleEventInPS;
	struct _NAN_NDL_INSTANCE_T *prRescheduleNdlInPS;
};

uint8_t *nanGetNanIEBuffer(void);

struct _NAN_PEER_SCH_DESC_T *
nanSchedAcquirePeerSchDescByNmi(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);

uint32_t nanSchedNegoApplyCustChnlList(struct ADAPTER *prAdapter);

uint32_t nanSchedInit(struct ADAPTER *prAdapter);
uint32_t nanSchedUninit(struct ADAPTER *prAdapter);

void nanSchedDropResources(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			   enum _ENUM_NAN_NEGO_TYPE_T eType);

void nanSchedNegoInitDb(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			enum _ENUM_NAN_NEGO_TYPE_T eType,
			enum _ENUM_NAN_NEGO_ROLE_T eRole);
void nanSchedNegoDispatchTimeout(struct ADAPTER *prAdapter,
		uintptr_t ulParam);
uint32_t nanSchedNegoGenLocalCrbProposal(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoChkRmtCrbProposal(struct ADAPTER *prAdapter,
			uint32_t *pu4RejectCode);

unsigned char nanSchedNegoInProgress(struct ADAPTER *prAdapter);
void nanSchedNegoStop(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoStart(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	enum _ENUM_NAN_NEGO_TYPE_T eType, enum _ENUM_NAN_NEGO_ROLE_T eRole,
	void (*pfCallback)(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			   enum _ENUM_NAN_NEGO_TYPE_T eType,
			   enum _ENUM_NAN_NEGO_ROLE_T eRole, void *pvToken),
	void *pvToken);

struct _NAN_PEER_SCHEDULE_RECORD_T *
nanSchedGetPeerSchRecord(struct ADAPTER *prAdapter, uint32_t u4SchIdx);

struct _NAN_PEER_SCHEDULE_RECORD_T *
nanSchedLookupPeerSchRecord(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);

uint32_t nanSchedNegoGetImmuNdlScheduleList(struct ADAPTER *prAdapter,
					    uint8_t **ppucScheduleList,
					    uint32_t *pu4ScheduleListLength);
uint32_t nanSchedNegoGetRangingScheduleList(struct ADAPTER *prAdapter,
					    uint8_t **ppucScheduleList,
					    uint32_t *pu4ScheduleListLength);
uint32_t nanSchedNegoGetQosAttr(struct ADAPTER *prAdapter,
				uint8_t **ppucQosAttr, uint32_t *pu4QosLength);
uint32_t nanSchedNegoGetSelectedNdcAttr(struct ADAPTER *prAdapter,
					uint8_t **ppucNdcAttr,
					uint32_t *pu4NdcAttrLength);
uint32_t nanSchedNegoAddQos(struct ADAPTER *prAdapter, uint32_t u4MinSlots,
			    uint32_t u4MaxLatency);

enum ENUM_BAND
getPeerSchDescMaxCap(struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc);

uint32_t nanSchedGetAvailabilityAttr(struct ADAPTER *prAdapter,
				     struct _NAN_NDL_INSTANCE_T *prNDL,
				     uint8_t **ppucAvailabilityAttr,
				     uint32_t *pu4AvailabilityAttrLength);
uint32_t nanSchedGetNdcAttr(struct ADAPTER *prAdapter, uint8_t **ppucNdcAttr,
			    uint32_t *pu4NdcAttrLength);

uint32_t nanSchedGetDevCapabilityAttr(struct ADAPTER *prAdapter,
				      uint8_t **ppucDevCapAttr,
				      uint32_t *pu4DevCapAttrLength);

uint8_t nanSchedChooseBestFromChnlBitmap(struct ADAPTER *prAdapter,
					 uint8_t ucOperatingClass,
					 uint16_t *pu2ChnlBitmap,
					 unsigned char fgNonContBw,
					 uint8_t ucPriChnlBitmap,
					 uint8_t *pucTimeBitmap);

u_int8_t nanCommonBandFromAvailabilityAttr(struct ADAPTER *prAdapter,
					   void *pAvailabilityAttr);

uint32_t nanGetCommonBandWithConcurrent(struct ADAPTER *prAdapter,
					u_int8_t fgPrint,
					uint32_t u4CommonSupportedBand);

#if (CFG_SUPPORT_NAN_6G == 1)
uint32_t nanSchedGetDevCapabilityExtAttr(struct ADAPTER *prAdapter,
				      uint8_t **ppucDevCapExtAttr,
				      uint32_t *pu4DevCapExtAttrLength);
#endif

uint32_t nanSchedGetUnalignedScheduleAttr(struct ADAPTER *prAdapter,
					  uint8_t **ppucUnalignedScheduleAttr,
					  uint32_t *pu4UnalignedScheduleLength);

uint32_t nanSchedGetExtWlanInfraAttr(struct ADAPTER *prAdapter,
				     uint8_t **ppucExtWlanInfraAttr,
				     uint32_t *pu4ExtWlanInfraLength);

uint32_t nanSchedGetPublicAvailAttr(struct ADAPTER *prAdapter,
				    uint8_t **ppucPublicAvailAttr,
				    uint32_t *pu4PublicAvailLength);

uint32_t nanSchedPeerUpdateUawAttr(struct ADAPTER *prAdapter,
				   uint8_t *pucNmiAddr, uint8_t *pucUawAttr);
uint32_t nanSchedPeerUpdateQosAttr(struct ADAPTER *prAdapter,
				   uint8_t *pucNmiAddr, uint8_t *pucQosAttr);
uint32_t nanSchedPeerUpdateNdcAttr(struct ADAPTER *prAdapter,
				   uint8_t *pucNmiAddr, uint8_t *pucNdcAttr);
uint32_t nanSchedPeerUpdateAvailabilityAttr(struct ADAPTER *prAdapter,
					    enum _NAN_ACTION_T eNanAction,
					    uint8_t *pucNmiAddr,
					    uint8_t *pucAvailabilityAttr,
					    struct _NAN_NDP_INSTANCE_T *prNDP);
uint32_t nanSchedPeerUpdateImmuNdlScheduleList(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntryList,
	uint32_t u4ScheduleEntryListLength);
uint32_t nanSchedPeerUpdateRangingScheduleList(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntryList,
	uint32_t u4ScheduleEntryListLength);
uint32_t nanSchedPeerUpdateDevCapabilityAttr(struct ADAPTER *prAdapter,
					     uint8_t *pucNmiAddr,
					     uint8_t *pucDevCapabilityAttr);

void nanUpdateStaRecIdx(struct ADAPTER *prAdapter,
			struct _NAN_NDL_INSTANCE_T *prNDL);

void nanSchedPeerUpdateCommonFAW(struct ADAPTER *prAdapter, uint32_t u4SchIdx);

uint32_t nanGetPeerDevCapability(struct ADAPTER *prAdapter,
				 enum _ENUM_NAN_DEVCAP_FIELD_T eField,
				 uint8_t *pucNmiAddr, uint8_t ucMapID,
				 uint32_t *pu4RetValue);

uint8_t nanGetPeerMinBw(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
				enum ENUM_BAND eBand);

uint8_t nanGetPeerMaxBw(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);

uint32_t nanSchedDbgDumpPeerAvailability(struct ADAPTER *prAdapter,
					 uint8_t *pucNmiAddr);

uint32_t nanSchedChkPeerCommonBand(struct ADAPTER *prAdapter,
					 uint8_t *pucNmiAddr);

enum _NAN_SUPPORTED_BAND_BIT
nanSchedGetHighestCommonBand(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			     u_int8_t fgPrint);

enum _ENUM_NAN_WINDOW_T nanWindowType(struct ADAPTER *prAdapter,
				      size_t szSlotIdx, size_t szTimeLineIdx);

uint32_t nanUtilCheckBitOneCnt(void *pBuffer, uint32_t u4Size);

void nanUtilDump(struct ADAPTER *prAdapter,
		uint8_t *pucMsg, uint8_t *pucContent, uint32_t u4Length);
uint32_t nanUtilCalAttributeToken(struct _NAN_ATTR_HDR_T *prNanAttr);

unsigned char nanGetFeatureNDPE(struct ADAPTER *prAdapter);
unsigned char nanGetFeaturePeerNDPE(struct ADAPTER *prAdapter,
		uint8_t *pucNmiAddr);

union _NAN_BAND_CHNL_CTRL nanQueryChnlInfoBySlot(struct ADAPTER *prAdapter,
		uint16_t u2SlotIdx,
		uint32_t **ppau4AvailMap,
		unsigned char fgCommitOrCond,
		uint8_t ucTimeLineIdx);
uint8_t nanQueryPrimaryChnlBySlot(struct ADAPTER *prAdapter,
		uint16_t u2SlotIdx, unsigned char fgCommitOrCond,
		uint8_t ucTimeLineIdx);
union _NAN_BAND_CHNL_CTRL nanQueryPeerChnlInfoBySlot(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx,
		uint32_t u4AvailDatabaseIdx,
		uint16_t u2SlotIdx,
		unsigned char fgCommitOrCond);
uint8_t nanQueryPeerPrimaryChnlBySlot(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx,
		uint32_t u4AvailDatabaseIdx,
		uint16_t u2SlotIdx, unsigned char fgCommitOrCond);
uint32_t nanGetPeerPrimaryChnlBySlot(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx, uint32_t u4AvailDbIdx,
		uint16_t u2SlotIdx, unsigned char fgChkRmtCondSlot);

uint32_t nanSchedConfigPhyParams(struct ADAPTER *prAdapter);
uint32_t nanSchedCmdUpdateSchedVer(struct ADAPTER *prAdapter);
uint32_t nanSchedConfigGetAllowedBw(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand);
enum _NAN_CHNL_BW_MAP nanSchedGet2gNanBw(struct ADAPTER *prAdapter);
enum _NAN_CHNL_BW_MAP nanSchedGet5gNanBw(struct ADAPTER *prAdapter);
enum _NAN_CHNL_BW_MAP nanSchedGet6gNanBw(struct ADAPTER *prAdapter);
uint8_t nanGetMaxBw(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand);
enum ENUM_CHANNEL_WIDTH
nanGetChannelBwByBand(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand);

uint32_t
nanSchedCmdUpdatePhySettigns(struct ADAPTER *prAdapter,
			     struct _NAN_PHY_SETTING_T *pr2P4GPhySettings,
			     struct _NAN_PHY_SETTING_T *pr5GPhySettings);
uint32_t nanSchedCmdManagePeerSchRecord(struct ADAPTER *prAdapter,
			uint32_t u4SchIdx, unsigned char fgActivate);
uint32_t nanSchedCmdUpdatePeerCapability(struct ADAPTER *prAdapter,
					 uint32_t u4SchIdx);
uint32_t nanSchedCmdUpdateCRB(struct ADAPTER *prAdapter, uint32_t u4SchIdx);
uint32_t nanSchedCmdUpdateAvailability(struct ADAPTER *prAdapter);
uint32_t nanSchedCmdUpdatePotentialChnlList(struct ADAPTER *prAdapter);
uint32_t nanSchedCmdUpdateAvailabilityCtrl(struct ADAPTER *prAdapter);

u_int8_t nanIs6gInUse(struct ADAPTER *prAdapter,
		      enum _NAN_CHNL_BW_MAP *e6gBandwidth);

uint32_t
nanSchedConfigAllowedBand(struct ADAPTER *prAdapter,
			  unsigned char fgEn2g,
			  unsigned char fgEn5gH,
			  unsigned char fgEn5gL,
			  unsigned char fgEn6g);
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
uint32_t nanSchedulerUniEventDispatch(struct ADAPTER *prAdapter,
				   uint32_t u4SubEvent, uint8_t *pucBuf);
#else
uint32_t nanSchedulerEventDispatch(struct ADAPTER *prAdapter,
				   uint32_t u4SubEvent, uint8_t *pucBuf);
#endif
uint32_t nanSchedSwDbg4(struct ADAPTER *prAdapter, uint32_t u4Data);

uint32_t nanSchedCmdMapStaRecord(struct ADAPTER *prAdapter,
				 uint8_t *pucNmiAddr,
				 enum NAN_BSS_ROLE_INDEX eRoleIdx,
				 uint8_t ucStaRecIdx,
				 uint8_t ucNdpCxtId,
				 uint8_t ucWlanIndex,
				 uint8_t *pucNdiAddr);

unsigned char
nanSchedPeerSchRecordIsValid(struct ADAPTER *prAdapter, uint32_t u4SchIdx);
uint32_t
nanSchedQueryStaRecIdx(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
		       uint32_t u4Idx, uint8_t ucLinkIdx);

void nanSchedPeerPrepareNegoState(struct ADAPTER *prAdapter,
				  uint8_t *pucNmiAddr);
void nanSchedPeerCompleteNegoState(struct ADAPTER *prAdapter,
				   uint8_t *pucNmiAddr);

const char *nanGetNanReasonString(uint32_t u4ReasonCode);

uint32_t nanSchedNegoCustFawAddEntry(struct ADAPTER *prAdapter,
				     struct _NAN_CUST_FAW_ENTRY *prEntry);
uint32_t nanSchedNegoCustFawRemoveEntry(struct ADAPTER *prAdapter,
					const char *pcTag);
void nanSchedNegoCustFawReconfigure(struct ADAPTER *prAdapter);

uint32_t nanSchedNegoCustFawResetCmd(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoCustFawApplyCmd(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoCustFawConfigCmd(struct ADAPTER *prAdapter,
				      uint8_t ucChnl, enum ENUM_BAND eBand,
				      uint32_t u4SlotBitmap);

void nanSchedReleaseUnusedCommitSlot(struct ADAPTER *prAdapter);
enum ENUM_BAND
nanSchedGetSchRecBandByMac(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);

#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
struct NAN_FLOW_CTRL *nanSchedGetPeerSchRecFlowCtrl(struct ADAPTER *prAdapter,
						    uint32_t u2SchId);
#endif

struct _NAN_TIMELINE_MGMT_T *
nanGetTimelineMgmt(struct ADAPTER *prAdapter, uint8_t ucIdx);
uint8_t nanGetTimelineNum(void);

size_t
nanGetTimelineMgmtIndexByBand(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand);

struct _NAN_PEER_SCH_DESC_T *
nanSchedSearchPeerSchDescByNmi(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);
struct _NAN_SCHEDULER_T *
nanGetScheduler(struct ADAPTER *prAdapter);

extern const union _NAN_BAND_CHNL_CTRL g_rNullChnl;
extern union _NAN_BAND_CHNL_CTRL g_r2gDwChnl;
extern union _NAN_BAND_CHNL_CTRL g_r5gDwChnl;

void nanUpdateAisBitmap(struct ADAPTER *prAdapter, u_int8_t fgSet);

uint32_t nanSchedGetConnChnlUsage(struct ADAPTER *prAdapter,
				  enum ENUM_NETWORK_TYPE eNetworkType,
				  enum ENUM_BAND eBand,
				  union _NAN_BAND_CHNL_CTRL *prChnl,
				  uint32_t *pu4SlotBitmap,
				  uint8_t *ucPhyTypeSet);

uint32_t nanSchedGetConnChnlUsageByTimeline(struct ADAPTER *prAdapter,
					    enum ENUM_NETWORK_TYPE eNetworkType,
					    size_t szTimeline,
					    union _NAN_BAND_CHNL_CTRL *prChnl,
					    uint32_t *pu4SlotBitmap,
					    uint8_t *ucPhyTypeSet);

uint8_t nanSchedGetConnBands(struct ADAPTER *prAdapter,
			     enum ENUM_NETWORK_TYPE eNetworkType);

#if (CFG_NAN_SCHEDULER_VERSION == 1)
union _NAN_BAND_CHNL_CTRL
nanQueryNonNanChnlInfoBySlot(struct ADAPTER *prAdapter,
	uint16_t u2SlotIdx, enum ENUM_BAND eBand);

uint32_t
nanSchedUpdateNonNanTimelineByAis(struct ADAPTER *prAdapter);

uint32_t
nanSchedSyncNonNanChnlToNan(struct ADAPTER *prAdapter);

uint32_t nanSchedCommitNonNanChnlList(struct ADAPTER *prAdapter);

#endif/* (CFG_NAN_SCHEDULER_VERSION == 1) */

#if (CFG_SUPPORT_NAN_NDP_DUAL_BAND == 0)
uint32_t
nanSchedRemoveDiffBandChnlList(
	struct ADAPTER *prAdapter);
#endif /* (CFG_SUPPORT_NAN_NDP_DUAL_BAND == 0) */

#endif

uint32_t nanSchedUpdateCustomCommands(struct ADAPTER *prAdapter,
				      const uint8_t *buf, uint32_t len);

uint32_t nanGetActiveNdlNum(struct ADAPTER *prAdapter);
uint32_t nanGetEstablishedNdlNum(struct ADAPTER *prAdapter);

u_int8_t nanCheckIsNeedReschedule(struct ADAPTER *prAdapter,
				  enum RESCHEDULE_SOURCE event,
				  uint8_t *pucPeerNmi);

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
uint32_t nanSchedNegoGenDefCrbV2(struct ADAPTER *prAdapter,
					uint8_t fgChkRmtCondSlot);
#endif

enum _ENUM_CNM_CH_CONCURR_T
nanSchedChkConcurrOp(union _NAN_BAND_CHNL_CTRL rCurrChnlInfo,
		     union _NAN_BAND_CHNL_CTRL rNewChnlInfo);

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
void nanSchedReleaseReschedCommitSlot(struct ADAPTER *prAdapter,
				      uint32_t u4ReschedSlot,
				      size_t szTimeLineIdx);

void nanSchedRegisterReschedInf(
	struct _NAN_RESCHEDULE_TOKEN_T *
		(*fnGetRescheduleToken)(struct ADAPTER *prAdapter)
);

void nanSchedUnRegisterReschedInf(void);
#endif

#if (CFG_SUPPORT_NAN_11BE == 1)
uint8_t nanSchedCheckEHTSlotExist(struct ADAPTER *prAdapter);
#endif

void nanSchedUpdateP2pAisMcc(struct ADAPTER *prAdapter);

u_int8_t nanIsFollowP2pInNonSocialChannel(struct ADAPTER *prAdapter,
					  size_t szTimelineIdx,
					  union _NAN_BAND_CHNL_CTRL *prP2pChnl);

union _NAN_BAND_CHNL_CTRL
nanGetActiveChnl(struct ADAPTER *prAdapter,
		 enum ENUM_NETWORK_TYPE eNetworkType, enum ENUM_BAND eBand);

uint32_t
nanSchedDbgDumpTimelineDb(struct ADAPTER *prAdapter, const char *pucFunction,
			  uint32_t u4Line);

u_int8_t nanCheck2gOnlyPeerExists(struct ADAPTER *prAdapter);

void nanSchedNegoUpdateNegoResult(struct ADAPTER *prAdapter);

uint32_t nanSchedGetCurrentNegoTransIdx(struct ADAPTER *prAdapter);

void nanSetConcurrentCustomFAW(struct ADAPTER *prAdapter);

void nanUpdateMbmcIdx(struct ADAPTER *ad,
	uint8_t ucBssIdx,
	uint8_t ucBandIdx);

uint32_t
nanParserInterpretTimeBitmapField(struct ADAPTER *prAdapter,
		  uint16_t u2TimeBitmapCtrl,
		  uint8_t ucTimeBitmapLength,
		  uint8_t *pucTimeBitmap, uint32_t *pu4AvailMap);

uint32_t nanGetNdcSlotIdxFromAvailMap(uint32_t u4AvailMap);

void nanSchedSetLocalCommittedBand(struct ADAPTER *prAdapter);

uint32_t nanGetActivePeerNum(struct ADAPTER *prAdapter);
uint32_t nanGetPowerSavingPeerNum(struct ADAPTER *prAdapter);

#endif /* _NAN_SCHEDULER_H_ */
