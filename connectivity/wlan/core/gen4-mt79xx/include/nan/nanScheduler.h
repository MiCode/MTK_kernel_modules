/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_SCHEDULER_H_
#define _NAN_SCHEDULER_H_

#if CFG_SUPPORT_NAN

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

#define NAN_TIMELINE_MGMT_SIZE          2  /* need to align with FW */
#define NAN_TIMELINE_MGMT_CHNL_LIST_NUM 4 /* need to align with FW */

#define NAN_MAX_POTENTIAL_CHNL_LIST 5

#define NAN_CRB_NEGO_MAX_TRANSACTION 20

#define NAN_DW_INTERVAL 512
#define NAN_SLOT_INTERVAL 16

#define NAN_SLOTS_PER_DW_INTERVAL (NAN_DW_INTERVAL / NAN_SLOT_INTERVAL)

#define NAN_TOTAL_DW 16
#define NAN_TOTAL_SLOT_WINDOWS (NAN_TOTAL_DW * NAN_SLOTS_PER_DW_INTERVAL)

#define NAN_TIME_BITMAP_CONTROL_SIZE 2
#define NAN_TIME_BITMAP_LENGTH_SIZE 1
#define NAN_TIME_BITMAP_MAX_SIZE 64
#define NAN_TIME_BITMAP_FIELD_MAX_LENGTH                                       \
	(NAN_TIME_BITMAP_CONTROL_SIZE + NAN_TIME_BITMAP_LENGTH_SIZE +          \
	 NAN_TIME_BITMAP_MAX_SIZE)

#define NAN_DEFAULT_MAP_ID 0

#define NAN_MAX_CONN_CFG 8 /* the max supported concurrent NAN data path */

#define NAN_INVALID_QOS_MAX_LATENCY 65535
#define NAN_INVALID_QOS_MIN_SLOTS 0
#define NAN_QOS_MAX_LATENCY_LOW_BOUND 1
#define NAN_QOS_MAX_LATENCY_UP_BOUND 30
#define NAN_QOS_MIN_SLOTS_LOW_BOUND 1
#define NAN_QOS_MIN_SLOTS_UP_BOUND 30
#define NAN_DEFAULT_NDL_QUOTA_LOW_BOUND 3
#define NAN_BITMAP_LOG "%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x"
#if (CFG_SUPPORT_NAN_CUST_DW_CHNL == 0)
#define NAN_DEFAULT_NDL_QUOTA_UP_BOUND 31
#else
#define NAN_DEFAULT_NDL_QUOTA_UP_BOUND 32
#endif
#define NAN_DEFAULT_RANG_QUOTA_LOW_BOUND 1
#define NAN_DEFAULT_RANG_QUOTA_UP_BOUND 3

#define NAN_INVALID_MAP_ID 0xFF

#define NAN_MAX_DEFAULT_TIMELINE_NUM		2
	/* Default timeline channel number */
#define NAN_MAX_NONNAN_TIMELINE_NUM		1
	/* Non-Nan timeline number */

enum _NAN_CHNL_BW_MAP {
	NAN_CHNL_BW_20 = 0,
	NAN_CHNL_BW_40,
	NAN_CHNL_BW_80,
	NAN_CHNL_BW_160,
	NAN_CHNL_BW_NUM
};

struct NAN_EVT_NDL_FLOW_CTRL {
	uint16_t au2FlowCtrl[NAN_MAX_CONN_CFG];
};

union _NAN_BAND_CHNL_CTRL {
	struct {
		uint32_t u4Type : 1;
		uint32_t u4Rsvd : 31;
	} rInfo;

	struct {
		uint32_t u4Type : 1;
		uint32_t u4Rsvd : 23;
		uint32_t u4BandIdMask : 8;
	} rBand;

	struct {
		uint32_t u4Type : 1;
		uint32_t u4Rsvd : 7;
		uint32_t u4OperatingClass : 8;
		uint32_t u4PrimaryChnl : 8;
		uint32_t u4AuxCenterChnl : 8;
	} rChannel;

	uint32_t u4RawData;
};

union _NAN_AVAIL_ENTRY_CTRL {
	struct {
		uint16_t u2Type : 3;
		uint16_t u2Preference : 2;
		uint16_t u2Util : 3;
		uint16_t u2RxNss : 4;
		uint16_t u2TimeMapAvail : 1;
		uint16_t u2Rsvd : 3;
	} rField;

	uint16_t u2RawData;
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
	uint8_t ucMapId;
	uint8_t aucRsvd[3];

	uint32_t au4AvailMap[NAN_TOTAL_DW];
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

enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T {
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE = 0,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_PREPARE,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_CHECK,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_RUN,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_DONE, /* 4 */

	ENUM_NAN_SYNC_SCH_UPDATE_STATE_NUM
};

enum _ENUM_NAN_WINDOW_T {
	ENUM_NAN_DW,
	ENUM_NAN_FAW,
	ENUM_NAN_IDLE_WINDOW,
};

struct _NAN_DEVICE_CAPABILITY_T {
	uint8_t fgValid;
	uint8_t ucMapId;
	uint8_t ucSupportedBand;
	uint8_t ucOperationMode;
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
	uint8_t fgValid;
	uint8_t aucNdcId[NAN_NDC_ATTRIBUTE_ID_LENGTH];
	uint8_t aucRsvd[1];
	struct _NAN_SCHEDULE_TIMELINE_T arTimeline[NAN_TIMELINE_MGMT_SIZE];
};

struct _NAN_PEER_NDC_CTRL_T {
	uint8_t fgValid;
	uint8_t aucNdcId[NAN_NDC_ATTRIBUTE_ID_LENGTH];
	uint8_t aucRsvd[1];
	struct _NAN_SCHEDULE_TIMELINE_T arTimeline[NAN_NUM_AVAIL_DB];
};

struct _NAN_AVAILABILITY_TIMELINE_T {
	uint8_t fgActive;

	union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl;

	uint8_t ucNumBandChnlCtrl;
	union _NAN_BAND_CHNL_CTRL arBandChnlCtrl[NAN_NUM_BAND_CHNL_ENTRY];

	uint32_t au4AvailMap[NAN_TOTAL_DW];
};

struct _NAN_AVAILABILITY_DB_T {
	uint8_t ucMapId;

	struct _NAN_AVAILABILITY_TIMELINE_T
		arAvailEntryList[NAN_NUM_AVAIL_TIMELINE];
};

struct _NAN_PEER_SCH_DESC_T {
	struct LINK_ENTRY rLinkEntry;

	uint8_t aucNmiAddr[MAC_ADDR_LEN];

	OS_SYSTIME rUpdateTime;

	uint8_t fgUsed;   /* indicate the SCH DESC is used by a SCH REC */
	uint32_t u4SchIdx; /* valid only when fgUsed is TRUE */

	uint32_t u4AvailAttrToken;
	struct _NAN_AVAILABILITY_DB_T arAvailAttr[NAN_NUM_AVAIL_DB];

	uint32_t u4DevCapAttrToken;
	struct _NAN_DEVICE_CAPABILITY_T arDevCapability[NAN_NUM_AVAIL_DB + 1];

	uint32_t u4QosMinSlots;
	uint32_t u4QosMaxLatency;

	/* for peer proposal cache during shedule negotiation */
	struct _NAN_PEER_NDC_CTRL_T rSelectedNdcCtrl;

	uint8_t fgImmuNdlTimelineValid;
	struct _NAN_SCHEDULE_TIMELINE_T arImmuNdlTimeline[NAN_NUM_AVAIL_DB];

	uint8_t fgRangingTimelineValid;
	struct _NAN_SCHEDULE_TIMELINE_T arRangingTimeline[NAN_NUM_AVAIL_DB];
};

struct _NAN_CHANNEL_TIMELINE_T {
	uint8_t fgValid;
	uint8_t aucRsvd[1];
	union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl;
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
	uint8_t fgChkCondAvailability;
	struct _NAN_CHANNEL_TIMELINE_T
		arCondChnlList[NAN_TIMELINE_MGMT_CHNL_LIST_NUM];

	/* for custom committed FAW */
	struct _NAN_CHANNEL_TIMELINE_T
		arCustChnlList[NAN_TIMELINE_MGMT_CHNL_LIST_NUM];
};

/* NAN Scheduler Control Block */
struct _NAN_SCHEDULER_T {
	uint8_t fgInit;

	uint8_t fgEn2g;
	uint8_t fgEn5gH;
	uint8_t fgEn5gL;

	uint8_t ucNanAvailAttrSeqId; /* shared by all availability attr */
	uint16_t u2NanAvailAttrControlField;     /* tracking changed flags */
	uint16_t u2NanCurrAvailAttrControlField; /* tracking changed flags */

	struct _NAN_NDC_CTRL_T arNdcCtrl[NAN_MAX_NDC_RECORD];

	struct LINK rPeerSchDescList;
	struct LINK rFreePeerSchDescList;
	struct _NAN_PEER_SCH_DESC_T arPeerSchDescArray[NAN_NUM_PEER_SCH_DESC];

	uint8_t fgAttrDevCapValid;
	struct _NAN_ATTR_DEVICE_CAPABILITY_T rAttrDevCap;

	uint32_t u4NumOfPotentialChnlList[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_CHNL_ENTRY_T
		arPotentialChnlList[NAN_TIMELINE_MGMT_SIZE]
		[NAN_MAX_POTENTIAL_CHNL_LIST];

	struct TIMER rAvailAttrCtrlResetTimer;

	uint8_t uc2gCommitDwInterval;
	uint8_t uc5gCommitDwInterval;
};

struct _NAN_CUST_TIMELINE_PROFILE_T {
	uint8_t ucChnl;
	enum ENUM_BAND eBand;
	uint32_t u4SlotBitmap;
};

struct _NAN_NONNAN_NETWORK_TIMELINE_T {
	enum ENUM_NETWORK_TYPE eNetworkType;
	union _NAN_BAND_CHNL_CTRL rChnlInfo;
	uint32_t u4SlotBitmap;
};

struct _NAN_SCHED_EVENT_DEV_CAP_T {
	uint16_t u2MaxChnlSwitchTimeUs;
	uint8_t aucRsvd[2];
};

struct _NAN_POTENTIAL_CHNL_LIST_T {
	uint8_t ucMapId;
	uint8_t ucNumOfPotChnlList;
	uint8_t ucNumOfBandList;
	uint8_t ucTimeLineIdx;
	union _NAN_BAND_CHNL_CTRL rPotentialBand;
	struct _NAN_CHANNEL_TIMELINE_T
		arChnlList[NAN_MAX_POTENTIAL_CHNL_LIST];
};

struct _NAN_SCHEDULER_T *nanGetScheduler(struct ADAPTER *prAdapter);
struct _NAN_TIMELINE_MGMT_T *
nanGetTimelineMgmt(struct ADAPTER *prAdapter, uint8_t ucIdx);

uint32_t nanSchedNegoApplyCustChnlList(struct ADAPTER *prAdapter);

uint32_t nanSchedInit(struct ADAPTER *prAdapter);
uint32_t nanSchedUninit(struct ADAPTER *prAdapter);

void nanSchedDropResources(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			   enum _ENUM_NAN_NEGO_TYPE_T eType);

void nanSchedNegoInitDb(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			enum _ENUM_NAN_NEGO_TYPE_T eType,
			enum _ENUM_NAN_NEGO_ROLE_T eRole);
void nanSchedNegoDispatchTimeout(struct ADAPTER *prAdapter,
		unsigned long ulParam);
uint32_t nanSchedNegoGenLocalCrbProposal(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoChkRmtCrbProposal(struct ADAPTER *prAdapter,
				       uint32_t *pu4RejectCode);

uint8_t nanSchedNegoIsRmtCrbConflict(
	struct ADAPTER *prAdapter,
	struct _NAN_SCHEDULE_TIMELINE_T arTimeline[NAN_NUM_AVAIL_DB],
	unsigned char *pfgEmptyMapSet,
	uint32_t au4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW]);

uint8_t nanSchedNegoInProgress(struct ADAPTER *prAdapter);
void nanSchedNegoStop(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoStart(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	enum _ENUM_NAN_NEGO_TYPE_T eType, enum _ENUM_NAN_NEGO_ROLE_T eRole,
	void (*pfCallback)(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			   enum _ENUM_NAN_NEGO_TYPE_T eType,
			   enum _ENUM_NAN_NEGO_ROLE_T eRole, void *pvToken),
	void *pvToken);

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

uint32_t nanSchedGetAvailabilityAttr(struct ADAPTER *prAdapter,
				     uint8_t **ppucAvailabilityAttr,
				     uint32_t *pu4AvailabilityAttrLength);
uint32_t nanSchedGetNdcAttr(struct ADAPTER *prAdapter, uint8_t **ppucNdcAttr,
			    uint32_t *pu4NdcAttrLength);

uint32_t nanSchedGetDevCapabilityAttr(struct ADAPTER *prAdapter,
				      uint8_t **ppucDevCapAttr,
				      uint32_t *pu4DevCapAttrLength);

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
					    uint8_t *pucNmiAddr,
					    uint8_t *pucAvailabilityAttr);
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

void nanSchedPeerUpdateCommonFAW(struct ADAPTER *prAdapter, uint32_t u4SchIdx);

uint32_t nanGetPeerDevCapability(struct ADAPTER *prAdapter,
				 enum _ENUM_NAN_DEVCAP_FIELD_T eField,
				 uint8_t *pucNmiAddr, uint8_t ucMapID,
				 uint32_t *pu4RetValue);

uint8_t nanGetPeerMinBw(struct ADAPTER *prAdapter,
			 uint8_t *pucNmiAddr,
			 enum ENUM_BAND eBand);

uint8_t nanGetPeerMaxBw(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);

uint32_t nanSchedDbgDumpPeerAvailability(struct ADAPTER *prAdapter,
					 uint8_t *pucNmiAddr);

enum _ENUM_NAN_WINDOW_T nanWindowType(struct ADAPTER *prAdapter,
			      uint16_t u2SlotIdx, uint8_t ucTimeLineIdx);
uint32_t nanUtilCheckBitOneCnt(struct ADAPTER *prAdapter, uint8_t *pucBitMask,
			      uint32_t u4Size);
void nanUtilDump(struct ADAPTER *prAdapter,
		uint8_t *pucMsg, uint8_t *pucContent, uint32_t u4Length);
uint32_t nanUtilCalAttributeToken(struct _NAN_ATTR_HDR_T *prNanAttr);

uint8_t nanGetFeatureNDPE(struct ADAPTER *prAdapter);
uint8_t nanGetFeaturePeerNDPE(struct ADAPTER *prAdapter,
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
uint32_t nanSchedConfigGetAllowedBw(struct ADAPTER *prAdapter,
		uint8_t ucChannel);

uint32_t
nanSchedCmdUpdatePhySettigns(IN struct ADAPTER *prAdapter,
			     struct _NAN_PHY_SETTING_T *pr2P4GPhySettings,
			     struct _NAN_PHY_SETTING_T *pr5GPhySettings);
uint32_t nanSchedCmdManagePeerSchRecord(IN struct ADAPTER *prAdapter,
			uint32_t u4SchIdx, unsigned char fgActivate);
uint32_t nanSchedCmdUpdatePeerCapability(struct ADAPTER *prAdapter,
					 uint32_t u4SchIdx);
uint32_t nanSchedCmdUpdateCRB(IN struct ADAPTER *prAdapter, uint32_t u4SchIdx);
uint32_t nanSchedCmdUpdateAvailability(IN struct ADAPTER *prAdapter);
uint32_t nanSchedCmdUpdatePotentialChnlList(IN struct ADAPTER *prAdapter);
uint32_t nanSchedCmdUpdateAvailabilityCtrl(struct ADAPTER *prAdapter);

uint32_t nanSchedulerEventDispatch(struct ADAPTER *prAdapter,
				   uint32_t u4SubEvent, uint8_t *pucBuf);

uint32_t nanSchedSwDbg4(struct ADAPTER *prAdapter, IN uint32_t u4Data);

uint32_t nanSchedCmdMapStaRecord(IN struct ADAPTER *prAdapter,
				 uint8_t *pucNmiAddr,
				 enum NAN_BSS_ROLE_INDEX eRoleIdx,
				 uint8_t ucStaRecIdx, uint8_t ucNdpCxtId);

uint8_t
nanSchedPeerSchRecordIsValid(struct ADAPTER *prAdapter, uint32_t u4SchIdx);
uint32_t
nanSchedQueryStaRecIdx(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
		       uint32_t u4Idx);

void nanSchedPeerPrepareNegoState(struct ADAPTER *prAdapter,
				  uint8_t *pucNmiAddr);
void nanSchedPeerCompleteNegoState(struct ADAPTER *prAdapter,
				   uint8_t *pucNmiAddr);

uint32_t nanSchedNegoCustFawResetCmd(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoCustFawApplyCmd(struct ADAPTER *prAdapter);
uint32_t nanSchedNegoCustFawConfigCmd(struct ADAPTER *prAdapter, uint8_t ucChnl,
				      uint32_t u4SlotBitmap);

void nanSchedReleaseUnusedCommitSlot(struct ADAPTER *prAdapter);
enum ENUM_BAND
nanSchedGetSchRecBandByMac(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr);

extern union _NAN_BAND_CHNL_CTRL g_rNullChnl;

enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T
nanSchedNegoSyncSchUpdateFsmStep(
	IN struct ADAPTER *prAdapter,
	IN enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T eNextState);

enum NanStatusType
nanDisableHandler(struct ADAPTER *prAdapter);
void
nanPreSuspendFlow(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_NAN_CUSTOMIZATION_VERSION == 1)
uint32_t
nanSchedInitDefChnlMap(struct ADAPTER *prAdapter);

uint32_t
nanSchedUpdateDefChnlInfo(struct ADAPTER *prAdapter);

uint32_t
nanSchedUpdateChnlInfoByAis(struct ADAPTER *prAdapter);

uint32_t
nanSchedFawDefConfigCmd(struct ADAPTER *prAdapter, uint8_t ucChnl,
	enum ENUM_BAND eBand, uint32_t u4SlotBitmap, uint8_t ucApply);
#endif /* (CFG_SUPPORT_NAN_CUSTOMIZATION_VERSION == 1) */

#if (CFG_SUPPORT_NAN_AVAILABILITY_CONTROL_BY_UPPER_LAYER == 1)
uint32_t
nanSchedResetCommitedAvailability(struct ADAPTER *prAdapter);
uint32_t
nanSchedResetPotentialAvailability(struct ADAPTER *prAdapter);
uint32_t
nanSchedConfigCommitedAvailability(struct ADAPTER *prAdapter,
	uint8_t ucMapIdx,
	union _NAN_BAND_CHNL_CTRL rChnlInfo,
	union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl,
	uint32_t *pu4AvailMap);
uint32_t
nanSchedConfigPotentialAvailability(struct ADAPTER *prAdapter,
	uint8_t ucMapIdx,
	union _NAN_BAND_CHNL_CTRL rChnlInfo,
	union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl,
	uint32_t *pu4AvailMap);
uint32_t
nanSchedConfigNdcAvailability(struct ADAPTER *prAdapter,
	uint8_t ucMapIdx,
	uint32_t *pu4AvailMap,
	uint8_t *pucNdcId);

uint32_t
nanSchedAddPotentialAvailability(struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint8_t ucTimeLineIdx);

uint32_t
nanSchedCmdUpdatePotentialChnlAvail(IN struct ADAPTER *prAdapter);
uint32_t
nanParserInterpretTimeBitmapField(struct ADAPTER *prAdapter,
		  uint16_t u2TimeBitmapCtrl,
		  uint8_t ucTimeBitmapLength,
		  uint8_t *pucTimeBitmap, uint32_t *pu4AvailMap);
void
nanSchedReleaseUnusedNdcCtrl(IN struct ADAPTER *prAdapter);

#endif /* (CFG_SUPPORT_NAN_AVAILABILITY_CONTROL_BY_UPPER_LAYER == 1) */

#if (CFG_SUPPORT_NAN_CUST_DW_CHNL == 1)
uint32_t
nanSchedSetDwTimeline(struct ADAPTER *prAdapter);
uint32_t
nanSchedUpdateDwTimeline(struct ADAPTER *prAdapter,
	uint32_t u45gChnlVal);
#endif /* (CFG_SUPPORT_NAN_CUST_DW_CHNL == 1) */

#endif
#endif /* _NAN_SCHEDULER_H_ */
