// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if (CFG_SUPPORT_NAN == 1)

#include "precomp.h"
#include "typedef.h"
#include "nanReg.h"
#include "nanRescheduler.h"
#include "rlm_domain.h"

#define NDC_NEXT_SLOT_CHANNEL 149

#define REMOVE_6G_COMMIT_BY_PEER_CAP 1

#define MERGE_POTENTIAL 1 /* MERGE POTENTIAL for INSUFFICIENT COMMITTED */
#define CFG_NAN_SIGMA_TEST 1
#define CFG_NAN_AVAIL_CTRL_RESET_TIMEOUT 100

#define NAN_MAX_DEFAULT_TIMELINE_NUM 2

#if (CFG_SUPPORT_NAN_6G == 1)
#define NAN_6G_BW20_OP_CLASS	131
#define NAN_6G_BW40_OP_CLASS	132
#define NAN_6G_BW80_OP_CLASS	133
#define NAN_6G_BW160_OP_CLASS	134
#define NAN_6G_BW320_OP_CLASS	137

#define NAN_6G_BW20_START_CHNL	1
#define NAN_6G_BW40_START_CHNL	3
#define NAN_6G_BW80_START_CHNL	7
#define NAN_6G_BW160_START_CHNL	15
#define NAN_6G_BW320_START_CHNL	31

/* NOTE each project might have its preference,
 * determine the result in g_ar6gPotentialChnlMap
 */
#define NAN_6G_BW20_TOTAL_CHNL_NUM     24 /* 59 */
#define NAN_6G_BW40_TOTAL_CHNL_NUM     12 /* 29 */
#define NAN_6G_BW80_TOTAL_CHNL_NUM     6 /* 14 */
#define NAN_6G_BW160_TOTAL_CHNL_NUM    3 /* 7 */
#define NAN_6G_BW320_TOTAL_CHNL_NUM    1 /* 3 */
#endif

#define NAN_MAX_PREFER_CHNL_SEL			4

#define NAN_IS_AVAIL_MAP_SET(pu4AvailMap, u2SlotIdx)		\
	((pu4AvailMap[NAN_DW_INDEX(u2SlotIdx)] &		\
	  BIT(NAN_SLOT_INDEX(u2SlotIdx))) != 0)

#define NAN_TIMELINE_SET(pu4AvailMap, u2SlotIdx)		\
do {								\
	pu4AvailMap[NAN_DW_INDEX(u2SlotIdx)] |=			\
		BIT(NAN_SLOT_INDEX(u2SlotIdx));			\
	DBGLOG(NAN, TEMP, "SET in %s, %p, set %u, 0x%08x\n",	\
	       __func__, pu4AvailMap, (uint16_t)u2SlotIdx,	\
	       pu4AvailMap[NAN_DW_INDEX(u2SlotIdx)]);		\
} while (0)

#define NAN_TIMELINE_UNSET(pu4AvailMap, u2SlotIdx)		\
	(pu4AvailMap[NAN_DW_INDEX(u2SlotIdx)] &=		\
	 (~BIT(NAN_SLOT_INDEX(u2SlotIdx))))


#define NAN_IS_U8_BIT_SET(pucBitmap, u2SlotIdx)		\
	((pucBitmap[(u2SlotIdx) / CHAR_BIT] & \
	  BIT((u2SlotIdx) % CHAR_BIT)) != 0)

#define NAN_U8_BIT_SET(pucBitmap, u2SlotIdx)		\
do {								\
	pucBitmap[(u2SlotIdx) / CHAR_BIT] |= BIT((u2SlotIdx) % CHAR_BIT); \
	DBGLOG(NAN, TEMP, "SET in %s, %p, set %u, 0x%08x\n",	\
	       __func__, pucBitmap, u2SlotIdx,		\
	       pucBitmap[(u2SlotIdx) / CHAR_BIT]);		\
} while (0)

#define NAN_U8_BIT_UNSET(pucBitmap, u2SlotIdx)		\
	(pucBitmap[(u2SlotIdx) / CHAR_BIT] &= (~BIT((u2SlotIdx) % CHAR_BIT)))


#define NAN_MAX_NONNAN_TIMELINE_NUM		NAN_TIMELINE_MGMT_SIZE
	/* Non-Nan timeline number */

#ifndef sizeof_field
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#endif

enum _ENUM_NAN_WINDOW_T {
	ENUM_NAN_DW,
	ENUM_NAN_FAW,
	ENUM_NAN_IDLE_WINDOW,
};

enum _ENUM_CHNL_CHECK_T {
	ENUM_CHNL_CHECK_PASS,
	ENUM_CHNL_CHECK_CONFLICT,
	ENUM_CHNL_CHECK_NOT_FOUND,
};

enum _ENUM_NEGO_CTRL_RST_REASON_T {
	ENUM_NEGO_CTRL_RST_BY_WAIT_RSP_STATE = 1,
	ENUM_NEGO_CTRL_RST_PREPARE_WAIT_RSP_STATE,
	ENUM_NEGO_CTRL_RST_PREPARE_CONFIRM_STATE,
};

enum _ENUM_TIME_BITMAP_CTRL_PERIOD_T {
	ENUM_TIME_BITMAP_CTRL_PERIOD_128 = 1, /* 8 NAN slots */
	ENUM_TIME_BITMAP_CTRL_PERIOD_256,     /* 16 NAN slots */
	ENUM_TIME_BITMAP_CTRL_PERIOD_512,     /* 32 NAN slots */
	ENUM_TIME_BITMAP_CTRL_PERIOD_1024,    /* 64 NAN slots */
	ENUM_TIME_BITMAP_CTRL_PERIOD_2048,    /* 128 NAN slots */
	ENUM_TIME_BITMAP_CTRL_PERIOD_4096,    /* 256 NAN slots */
	ENUM_TIME_BITMAP_CTRL_PERIOD_8192,    /* 512 NAN slots */
};

enum _ENUM_TIME_BITMAP_CTRL_DURATION {
	ENUM_TIME_BITMAP_CTRL_DURATION_16 = 0, /* 1 NAN slot */
	ENUM_TIME_BITMAP_CTRL_DURATION_32,     /* 2 NAN slots */
	ENUM_TIME_BITMAP_CTRL_DURATION_64,     /* 4 NAN slots */
	ENUM_TIME_BITMAP_CTRL_DURATION_128,    /* 8 NAN slots */
};

enum _ENUM_NAN_CRB_NEGO_STATE_T {
	ENUM_NAN_CRB_NEGO_STATE_IDLE,
	ENUM_NAN_CRB_NEGO_STATE_INITIATOR,
	ENUM_NAN_CRB_NEGO_STATE_RESPONDER,
	ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP,
	ENUM_NAN_CRB_NEGO_STATE_CONFIRM,
};

struct _NAN_CRB_NEGO_TRANSACTION_T {
	uint8_t aucNmiAddr[MAC_ADDR_LEN];
	enum _ENUM_NAN_NEGO_TYPE_T eType;
	enum _ENUM_NAN_NEGO_ROLE_T eRole;

	void(*pfCallback)
	(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	 enum _ENUM_NAN_NEGO_TYPE_T eType, enum _ENUM_NAN_NEGO_ROLE_T eRole,
	 void *pvToken);
	void *pvToken;

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
	enum RESCHEDULE_SOURCE eReschedSrc;
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1 && CFG_SUPPORT_NAN_11BE == 1)
	uint8_t fgIsEhtReschedule;
	uint8_t fgIsEhtRescheduleNewNDL;
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	uint8_t fgTriggerReschedNewNDL;
	uint8_t fgIs3rd6GNewNDL;
#endif
	uint32_t u4NotChoose6GCnt;
	u_int8_t fgCounterCountry;
	uint8_t fgPropose2gConditional;
};

struct _NAN_POTENTIAL_CHNL_MAP_T {
	uint8_t ucPrimaryChnl;
	uint8_t aucOperatingClass[NAN_CHNL_BW_NUM];
	uint16_t au2ChnlMapIdx[NAN_CHNL_BW_NUM];
	/* the index in the operating class group */
	uint8_t aucPriChnlMapIdx[NAN_CHNL_BW_NUM];
};

struct _NAN_POTENTIAL_CHNL_T {
	uint8_t ucOpClass;
	uint8_t ucPriChnlBitmap;
	uint16_t u2ChnlBitmap;
};

enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T {
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE = 0,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_PREPARE,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_CHECK,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_RUN,
	ENUM_NAN_SYNC_SCH_UPDATE_STATE_DONE, /* 4 */

	ENUM_NAN_SYNC_SCH_UPDATE_STATE_NUM
};

/* NAN CRB Negotiation Control Block */
struct _NAN_CRB_NEGO_CTRL_T {
	uint32_t u4SchIdx;

	uint32_t u4DefNdlNumSlots;
	uint32_t u4DefRangingNumSlots;

	enum _ENUM_NAN_CRB_NEGO_STATE_T eState;
	enum _ENUM_NAN_NEGO_TYPE_T eType;
	enum _ENUM_NAN_NEGO_ROLE_T eRole;

	/* for local proposal or final proposal cache
	 * during schedule negotiation
	 */
	struct _NAN_NDC_CTRL_T rSelectedNdcCtrl;
	struct _NAN_SCHEDULE_TIMELINE_T
			arImmuNdlTimeline[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_SCHEDULE_TIMELINE_T
			arRangingTimeline[NAN_TIMELINE_MGMT_SIZE];

	uint32_t u4QosMinSlots;
	uint32_t u4QosMaxLatency;
	uint32_t u4NegoQosMinSlots;
	uint32_t u4NegoQosMaxLatency;

	/* for QoS negotiation */
	uint32_t aau4AvailSlots[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW];
	uint32_t aau4UnavailSlots[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW];
	uint32_t aau4FreeSlots[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW];
	uint32_t aau4CondSlots[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW];
	uint32_t aau4FawSlots[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW];

	uint8_t ucNegoCurTransIdx;
	uint8_t ucNegoTransNum;
	uint8_t ucNegoPendingTransNum;
	uint8_t ucNegoTransHeadPos;
	struct _NAN_CRB_NEGO_TRANSACTION_T
		rNegoTrans[NAN_CRB_NEGO_MAX_TRANSACTION];

	struct TIMER rCrbNegoDispatchTimer;

	enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T eSyncSchUpdateLastState;
	enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T eSyncSchUpdateCurrentState;
	uint32_t u4SyncPeerSchRecIdx;

	int32_t i4InNegoContext;

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	uint8_t fgInsistNDLSlot5GMode;
#endif
};

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_UPDATE_CRB_T {
	uint32_t u4SchIdx;
	uint8_t fgUseDataPath :1,
		b2Avail6GFormat :2,
		b5Reserved :5;
	uint8_t fgUseRanging;
	uint8_t aucRsvd[2];
	struct _NAN_SCHEDULE_TIMELINE_T
			arCommRangingTimeline[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_SCHEDULE_TIMELINE_T
			arCommFawTimeline[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_NDC_CTRL_T rCommNdcCtrl;
	struct _NAN_FAW_NDC_TIMELINE_T
			arFawNdcTimeline[NAN_TIMELINE_MGMT_SIZE];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T {
	uint32_t u4SchIdx;
	uint8_t fgActivate;
	uint8_t aucNmiAddr[MAC_ADDR_LEN];
	uint8_t aucRsvd[1];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_UPDATE_PEER_CAPABILITY_T {
	uint32_t u4SchIdx;
	uint8_t ucSupportedBands;
	uint16_t u2MaxChnlSwitchTime;
	uint8_t ucPeerSupportedBands;
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_MAP_STA_REC_T {
	uint8_t aucNmiAddr[MAC_ADDR_LEN];
	uint8_t ucStaRecIdx;
	uint8_t ucNdpCxtId;

	enum NAN_BSS_ROLE_INDEX eRoleIdx;
	uint8_t aucNdiAddr[MAC_ADDR_LEN];
	uint8_t aucRsvd[2];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T {
	uint8_t ucMapId;
	uint8_t fgChkCondAvailability;
	uint8_t ucTimelineIdx;
	uint8_t fgMultipleMap;

	struct _NAN_CHANNEL_TIMELINE_T
		arChnlList[NAN_TIMELINE_MGMT_CHNL_LIST_NUM];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_CTRL_T {
	uint16_t u2AvailAttrControlField;
	uint8_t ucAvailSeqID;
	uint8_t aucRsvd[1];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_UPDATE_PHY_PARAM_T {
	struct _NAN_PHY_SETTING_T r2P4GPhySettings;
	struct _NAN_PHY_SETTING_T r5GPhySettings;
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_UPDATE_PONTENTIAL_CHNL_LIST_T {
	uint32_t au4Num[NAN_TIMELINE_MGMT_SIZE];
	struct _NAN_POTENTIAL_CHNL_T aarChnlList[NAN_TIMELINE_MGMT_SIZE]
		[NAN_MAX_POTENTIAL_CHNL_LIST];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_SCHED_CMD_SET_SCHED_VER_T {
	uint8_t ucNdlFlowCtrlVer;
	uint8_t aucRsvd[3];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

struct _NAN_SCHED_EVENT_SCHEDULE_CONFIG_T {
	uint8_t fgEn2g;
	uint8_t fgEn5gH;
	uint8_t fgEn5gL;
	uint8_t fgEn6g;
};

struct _NAN_SCHED_EVENT_DW_INTERVAL_T {
	uint8_t ucDwInterval;
};

struct _NAN_SCHED_EVENT_DEV_CAP_T {
	uint16_t u2MaxChnlSwitchTimeUs;
	uint8_t aucRsvd[2];
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

static const char *pcConcurrentTag = "nanConcurrent";

uint8_t g_aucNanIEBuffer[NAN_IE_BUF_MAX_SIZE];

uint32_t g_u4MaxChnlSwitchTimeUs = 8000; /* TODO: remove it, use FW reported */
#define CHANNEL_SWITCH_THRESHOLD 4096 /* a quarter of a slot */

struct _NAN_CRB_NEGO_CTRL_T g_rNanSchNegoCtrl = {0};

struct _NAN_PEER_SCHEDULE_RECORD_T g_arNanPeerSchedRecord[NAN_MAX_CONN_CFG];
struct _NAN_TIMELINE_MGMT_T g_arNanTimelineMgmt[NAN_TIMELINE_MGMT_SIZE] = {0};
struct _NAN_SCHEDULER_T g_rNanScheduler = {0};

struct _NAN_FAW_NDC_TIMELINE_T g_arNanFawNdcTimeline[NAN_TIMELINE_MGMT_SIZE];

const union _NAN_BAND_CHNL_CTRL g_rNullChnl = {.u4RawData = 0 };

union _NAN_BAND_CHNL_CTRL g_r2gDwChnl = {
	.u4ChnlType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
	.u4OperatingClass = NAN_2P4G_DISC_CH_OP_CLASS,
	.u4PrimaryChnl = NAN_2P4G_DISC_CHANNEL,
	.u4AuxCenterChnl = 0
};

union _NAN_BAND_CHNL_CTRL g_r5gDwChnl = {
	.u4ChnlType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
	.u4OperatingClass = NAN_5G_HIGH_DISC_CH_OP_CLASS,
	.u4PrimaryChnl = NAN_5G_HIGH_DISC_CHANNEL,
	.u4AuxCenterChnl = 0
};

union _NAN_BAND_CHNL_CTRL g_r5g160Chnl = {
	.u4ChnlType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
	.u4OperatingClass = NAN_5G_LOW_BW160_DISC_CH_OP_CLASS,
	.u4PrimaryChnl = NAN_5G_BW160_DEF_CHANNEL,
	.u4AuxCenterChnl = 0
};

#if (CFG_SUPPORT_NAN_6G == 1)
union _NAN_BAND_CHNL_CTRL g_r6gDefChnl = {
	.u4ChnlType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
	.u4OperatingClass = NAN_6G_BW20_OP_CLASS,
	.u4PrimaryChnl = NAN_6G_BW20_DEFAULT_CHANNEL,
	.u4AuxCenterChnl = 0
};
#endif

/* Set in nanSchedConfigAllowedBand */
union _NAN_BAND_CHNL_CTRL g_rPreferredChnl[NAN_TIMELINE_MGMT_SIZE] = {
	{
		.u4ChnlType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
		.u4OperatingClass = NAN_2P4G_DISC_CH_OP_CLASS,
		.u4PrimaryChnl = NAN_2P4G_DISC_CHANNEL,
		.u4AuxCenterChnl = 0
	}
};

struct _NAN_POTENTIAL_CHNL_MAP_T g_arPotentialChnlMap[] = {
	{ 149,
	  { 124, 126, 128, 0 },
	  { BIT(0), BIT(0), BIT(5), 0 },
	  { 0, 0, BIT(0), 0 } },
	{ 153,
	  { 124, 127, 128, 0 },
	  { BIT(1), BIT(0), BIT(5), 0 },
	  { 0, 0, BIT(1), 0 } },
	{ 157,
	  { 124, 126, 128, 0 },
	  { BIT(2), BIT(1), BIT(5), 0 },
	  { 0, 0, BIT(2), 0 } },
	{ 161,
	  { 124, 127, 128, 0 },
	  { BIT(3), BIT(1), BIT(5), 0 },
	  { 0, 0, BIT(3), 0 } },
	{ 36,
	  { 115, 116, 128, 129 },
	  { BIT(0), BIT(0), BIT(0), BIT(0) },
	  { 0, 0, BIT(0), BIT(0) } },
	{ 40,
	  { 115, 117, 128, 129 },
	  { BIT(1), BIT(0), BIT(0), BIT(0) },
	  { 0, 0, BIT(1), BIT(1) } },
	{ 44,
	  { 115, 116, 128, 129 },
	  { BIT(2), BIT(1), BIT(0), BIT(0) },
	  { 0, 0, BIT(2), BIT(2) } },
	{ 48,
	  { 115, 117, 128, 129 },
	  { BIT(3), BIT(1), BIT(0), BIT(0) },
	  { 0, 0, BIT(3), BIT(3) } },
	{ 1, { 81, 0, 0, 0 }, { BIT(0), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 2, { 81, 0, 0, 0 }, { BIT(1), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 3, { 81, 0, 0, 0 }, { BIT(2), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 4, { 81, 0, 0, 0 }, { BIT(3), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 5, { 81, 0, 0, 0 }, { BIT(4), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 6, { 81, 0, 0, 0 }, { BIT(5), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 7, { 81, 0, 0, 0 }, { BIT(6), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 8, { 81, 0, 0, 0 }, { BIT(7), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 9, { 81, 0, 0, 0 }, { BIT(8), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 10, { 81, 0, 0, 0 }, { BIT(9), 0, 0, 0 }, { 0, 0, 0, 0 } },
	{ 11, { 81, 0, 0, 0 }, { BIT(10), 0, 0, 0 }, { 0, 0, 0, 0 } },

	{ 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }
};

#if (CFG_SUPPORT_NAN_6G == 1)
/**
 * Update ucPriChnlBitmap in nanSchedConfigAllowedBand() according to
 * the primary channel in wifi.cfg.
 */
struct _NAN_POTENTIAL_CHNL_T g_ar6gPotentialChnlMap[NAN_CHNL_BW_NUM] = {
	{NAN_6G_BW20_OP_CLASS, 0,
		((NAN_6G_BW20_START_CHNL & 0xFF) |
		(NAN_6G_BW20_TOTAL_CHNL_NUM << 8))},
	{NAN_6G_BW40_OP_CLASS,
		BIT((NAN_6G_BW20_DEFAULT_CHANNEL - 1) / 4),
		((NAN_6G_BW40_START_CHNL & 0xFF) |
		(NAN_6G_BW40_TOTAL_CHNL_NUM << 8))},
	{NAN_6G_BW80_OP_CLASS,
		BIT((NAN_6G_BW20_DEFAULT_CHANNEL - 1) / 4),
		((NAN_6G_BW80_START_CHNL & 0xFF) |
		(NAN_6G_BW80_TOTAL_CHNL_NUM << 8))},
	{NAN_6G_BW160_OP_CLASS,
		BIT((NAN_6G_BW20_DEFAULT_CHANNEL - 1) / 4),
		((NAN_6G_BW160_START_CHNL & 0xFF) |
		(NAN_6G_BW160_TOTAL_CHNL_NUM << 8))},
	{NAN_6G_BW320_OP_CLASS,
		BIT((NAN_6G_BW20_DEFAULT_CHANNEL - 1) / 4),
		((NAN_6G_BW320_START_CHNL & 0xFF) |
		(NAN_6G_BW320_TOTAL_CHNL_NUM << 8))},
};
#endif

#if (CFG_NAN_SCHEDULER_VERSION == 1)
/* Record default channel bitmap when NO NDP */
struct _NAN_CUST_TIMELINE_PROFILE_T
	g_arDefChnlMap[NAN_MAX_DEFAULT_TIMELINE_NUM] = {0};

/* Record Non-NAN network channel bitmap */
struct _NAN_NONNAN_NETWORK_TIMELINE_T
	g_arNonNanTimeline[NAN_MAX_NONNAN_TIMELINE_NUM] = {0};
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
struct _NAN_RESCHEDULE_TOKEN_T*
(*nanSchedGetRescheduleToken)(struct ADAPTER *prAdapter) = NULL;
#endif

uint8_t *nanGetNanIEBuffer(void)
{
	return g_aucNanIEBuffer;
}

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
static union _NAN_BAND_CHNL_CTRL
nanSchedNegoFindSlotCrb(struct ADAPTER *prAdapter,
			struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl,
			enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand,
			unsigned char fgPrintLog,
			size_t szTimeLineIdx,
			size_t szSlotIdx,
			unsigned char fgReschedForce5G,
			unsigned char *pfgNotChoose6G);
#endif

static u_int8_t nanIsP2pAisMCC(struct ADAPTER *prAdapter, size_t szTimeLineIdx,
			       union _NAN_BAND_CHNL_CTRL *prP2pChnlInfo,
			       union _NAN_BAND_CHNL_CTRL *prAisChnlInfo);
#define NAN_IS_P2P_AIS_MCC(_prAdapter, _eBand)  ({      \
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo; \
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo; \
	size_t szTimeLine = nanGetTimelineMgmtIndexByBand(_prAdapter, _eBand); \
	nanIsP2pAisMCC(_prAdapter, szTimeLine,  \
		       &rP2pChnlInfo, &rAisChnlInfo);   \
})

static u_int8_t updateAvailability(struct ADAPTER *prAdapter,
		    enum _NAN_ACTION_T eNanAction,
		    struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
		    struct _NAN_ATTR_NAN_AVAILABILITY_T *prAttrNanAvailibility,
		    struct _NAN_AVAILABILITY_DB_T *prNanAvailDB,
		    u_int8_t fgFillByPotential);


static u_int8_t NAN_IS_CHANNEL_MATCH_TIMELINE(struct ADAPTER *prAdapter,
					union _NAN_BAND_CHNL_CTRL rChnlInfo,
					size_t szTimeLineIdx)
{
	const size_t sz2gTimeLineIdx =
		nanGetTimelineMgmtIndexByBand(prAdapter, BAND_2G4);
	const size_t sz5gTimeLineIdx =
		nanGetTimelineMgmtIndexByBand(prAdapter, BAND_5G);
#if (CFG_SUPPORT_NAN_6G == 1)
	const size_t sz6gTimeLineIdx =
		nanGetTimelineMgmtIndexByBand(prAdapter, BAND_6G);
#endif

	if (szTimeLineIdx == sz2gTimeLineIdx && NAN_IS_CHANNEL_2G(rChnlInfo))
		return TRUE;

	if (szTimeLineIdx == sz5gTimeLineIdx && NAN_IS_CHANNEL_5G(rChnlInfo))
		return TRUE;

#if (CFG_SUPPORT_NAN_6G == 1)
	if (szTimeLineIdx == sz6gTimeLineIdx && NAN_IS_CHANNEL_6G(rChnlInfo) ||
	    szTimeLineIdx == sz5gTimeLineIdx && NAN_IS_CHANNEL_6G(rChnlInfo) ||
	    szTimeLineIdx == sz5gTimeLineIdx && NAN_IS_CHANNEL_6G(rChnlInfo))
		return TRUE;
#endif

	return FALSE;
}

/**
 * nanGetChBound() - Get Channel high/low bounds with given parameters
 * @ucPrimaryCh: Primary channel
 * @eChannelWidth: Bandwidth
 * @eSCO: Secondary Channel Offset. Value of no secondary channel (SCN),
 *	  secondary channel above (SCA), or secondary channel below (SCB).
 * @ucChannelS1: segment 1 channel
 * @ucChannelS2: segment 2 channel, only for non-contiguous (80+80)
 * @pucChLowBound1: return low bound of segment 1 channel
 * @pucChHighBound1: return high bound of segment 1 channel
 * @pucChLowBound2: return low bound of segment 2 channel, only for 80+80
 * @pucChHighBound2: return high bound of segment 2 channel, only for 80+80
 *
 * Return: return channel parameters in the passed-in pointer arguments.
 */
void nanGetChBound(uint8_t ucPrimaryCh, enum ENUM_CHANNEL_WIDTH eChannelWidth,
	      enum ENUM_CHNL_EXT eSCO, uint8_t ucChannelS1, uint8_t ucChannelS2,
	      uint8_t *pucChLowBound1, uint8_t *pucChHighBound1,
	      uint8_t *pucChLowBound2, uint8_t *pucChHighBound2)
{
	*pucChLowBound1 = ucPrimaryCh;
	*pucChHighBound1 = ucPrimaryCh;

	switch (eChannelWidth) {
	case CW_20_40MHZ:
		if (eSCO == CHNL_EXT_SCB)
			*pucChLowBound1 = ucPrimaryCh - 4;
		else if (eSCO == CHNL_EXT_SCA)
			*pucChHighBound1 = ucPrimaryCh + 4;
		break;

	case CW_80MHZ:
		*pucChLowBound1 = ucChannelS1 - 6;
		*pucChHighBound1 = ucChannelS1 + 6;
		break;

	case CW_160MHZ:
		*pucChLowBound1 = ucChannelS1 - 14;
		*pucChHighBound1 = ucChannelS1 + 14;
		break;

	case CW_320_1MHZ:
		*pucChLowBound1 = ucChannelS1 - 30;
		*pucChHighBound1 = ucChannelS1 + 30;
		break;

	case CW_80P80MHZ:
		*pucChLowBound1 = ucChannelS1 - 6;
		*pucChHighBound1 = ucChannelS1 + 6;
		*pucChLowBound2 = ucChannelS2 * 2 - ucChannelS1 - 6;
				/* S1 + (S2-S1)*2 = S2*2 - S1 */
		*pucChHighBound2 = ucChannelS2 * 2 - ucChannelS1 + 6;
				/* S1 + (S2-S1)*2 = S2*2 - S1 */
		break;

	default: /* 320MHz */
		/* TODO */
		break;
	}
}

enum _ENUM_CNM_CH_CONCURR_T
nanChConCurrType(uint8_t ucPrimaryChNew,
		 enum ENUM_CHANNEL_WIDTH eChannelWidthNew,
		 enum ENUM_CHNL_EXT eSCONew,
		 uint8_t ucChannelS1New, uint8_t ucChannelS2New,
		 uint8_t ucPrimaryChCurr,
		 enum ENUM_CHANNEL_WIDTH eChannelWidthCurr,
		 enum ENUM_CHNL_EXT eSCOCurr,
		 uint8_t ucChannelS1Curr, uint8_t ucChannelS2Curr)
{
	uint8_t ucChLowBound_New1 = 0;
	uint8_t ucChHighBound_New1 = 0;
	uint8_t ucChLowBound_New2 = 0;
	uint8_t ucChHighBound_New2 = 0;

	uint8_t ucChLowBound_Curr1 = 0;
	uint8_t ucChHighBound_Curr1 = 0;
	uint8_t ucChLowBound_Curr2 = 0;
	uint8_t ucChHighBound_Curr2 = 0;

	if (ucPrimaryChNew != ucPrimaryChCurr)
		return CNM_CH_CONCURR_MCC;

	nanGetChBound(ucPrimaryChNew, eChannelWidthNew, eSCONew, ucChannelS1New,
		      ucChannelS2New, &ucChLowBound_New1, &ucChHighBound_New1,
		      &ucChLowBound_New2, &ucChHighBound_New2);

	nanGetChBound(ucPrimaryChCurr, eChannelWidthCurr, eSCOCurr,
		      ucChannelS1Curr, ucChannelS2Curr, &ucChLowBound_Curr1,
		      &ucChHighBound_Curr1, &ucChLowBound_Curr2,
		      &ucChHighBound_Curr2);

	if (eChannelWidthNew != CW_80P80MHZ &&
	    eChannelWidthCurr != CW_80P80MHZ) {
		if ((ucChLowBound_New1 >= ucChLowBound_Curr1) &&
		    (ucChHighBound_New1 <= ucChHighBound_Curr1)) {
			return CNM_CH_CONCURR_SCC_CURR;
		} else if ((ucChLowBound_Curr1 >= ucChLowBound_New1) &&
			 (ucChHighBound_Curr1 <= ucChHighBound_New1)) {
			return CNM_CH_CONCURR_SCC_NEW;
		}
	} else {

		if (eChannelWidthNew == CW_80P80MHZ &&
		    eChannelWidthCurr == CW_80P80MHZ) {

			if (ucChannelS2Curr ==
				    ucChannelS2New && /* Mirror == Mirror */
			    (ucChannelS1Curr == ucChannelS1New ||
			     ucChannelS1Curr ==
				     (ucChannelS2New * 2 - ucChannelS1New))) {
				/* Same BW */
				return CNM_CH_CONCURR_SCC_CURR;
			}

		} else if (eChannelWidthNew == CW_80P80MHZ) {

			switch (eChannelWidthCurr) {
			case CW_20_40MHZ:
				if (((ucChLowBound_Curr1 >=
				      ucChLowBound_New1) &&
				     (ucChHighBound_Curr1 <=
				      ucChHighBound_New1)) ||
				    ((ucChLowBound_Curr1 >=
				      ucChLowBound_New2) &&
				     (ucChHighBound_Curr1 <=
				      ucChHighBound_New2))) {

					/* curr is subset of new */
					return CNM_CH_CONCURR_SCC_NEW;
				}
				break;

			case CW_80MHZ:
				if (ucChannelS1Curr == ucChannelS1New ||
				    ucChannelS1Curr == (ucChannelS2New * 2 -
							ucChannelS1New)) {
					/* curr is subset of new */
					return CNM_CH_CONCURR_SCC_NEW;
				}
				break;

			case CW_160MHZ:
			case CW_320_1MHZ:
				if (ucChLowBound_New1 >= ucChLowBound_Curr1 &&
				    ucChLowBound_New2 >= ucChLowBound_Curr1 &&
				    ucChHighBound_New1 <= ucChHighBound_Curr1 &&
				    ucChHighBound_New2 <= ucChHighBound_Curr1) {

					return CNM_CH_CONCURR_SCC_CURR;
				}
				break;
			default:
				break;
			}

		} else if (eChannelWidthCurr == CW_80P80MHZ) {
			switch (eChannelWidthNew) {
			case CW_20_40MHZ:
				if (((ucChLowBound_New1 >=
				      ucChLowBound_Curr1) &&
				     (ucChHighBound_New1 <=
				      ucChHighBound_Curr1)) ||
				    ((ucChLowBound_New1 >=
				      ucChLowBound_Curr2) &&
				     (ucChHighBound_New1 <=
				      ucChHighBound_Curr2))) {
					/* curr is subset of new */
					return CNM_CH_CONCURR_SCC_CURR;
				}
				break;

			case CW_80MHZ:
				if (ucChannelS1New == ucChannelS1Curr ||
				    ucChannelS1New == (ucChannelS2Curr * 2 -
						       ucChannelS1Curr)) {
					/* curr is subset of new */
					return CNM_CH_CONCURR_SCC_CURR;
				}
				break;

			case CW_160MHZ:
			case CW_320_1MHZ:
				if (ucChLowBound_Curr1 >= ucChLowBound_New1 &&
				    ucChLowBound_Curr2 >= ucChLowBound_New1 &&
				    ucChHighBound_Curr1 <= ucChHighBound_New1 &&
				    ucChHighBound_Curr2 <= ucChHighBound_New1) {

					return CNM_CH_CONCURR_SCC_NEW;
				}
				break;
			default:
				break;
			}
		}
	}

	return CNM_CH_CONCURR_MCC;
}

struct _NAN_SCHEDULER_T *
nanGetScheduler(struct ADAPTER *prAdapter)
{
	return &g_rNanScheduler;
}

struct _NAN_TIMELINE_MGMT_T *
nanGetTimelineMgmt(struct ADAPTER *prAdapter, uint8_t ucIdx)
{
	if (ucIdx < NAN_TIMELINE_MGMT_SIZE)
		return &g_arNanTimelineMgmt[ucIdx];
	else
		return &g_arNanTimelineMgmt[0];
}

size_t
nanGetTimelineMgmtIndexByBand(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand)
{
#if (CFG_SUPPORT_NAN_DBDC == 1)
	if (!prAdapter->rWifiVar.fgDbDcModeEn)
		return 0;
#endif

	if (!prAdapter->fgNanMultipleMapTimeline)
		return 0;

	if (eBand == BAND_2G4)
		return 0;
	else
		return (NAN_TIMELINE_MGMT_SIZE - 1);
}

struct _NAN_TIMELINE_MGMT_T *
nanGetTimelineMgmtByBand(struct ADAPTER *prAdapter, enum ENUM_BAND eBand)
{
	size_t szIdx = nanGetTimelineMgmtIndexByBand(prAdapter, eBand);

	return &g_arNanTimelineMgmt[szIdx];
}


static size_t
nanGetActiveTimelineMgmtNum(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_NAN_DBDC == 1)

	if (prAdapter->rWifiVar.fgDbDcModeEn &&
		prAdapter->fgNanMultipleMapTimeline)
		return NAN_TIMELINE_MGMT_SIZE;

#endif

	return 1;
}

static uint32_t
nanGetTimelineSupportedBand(struct ADAPTER *prAdapter, size_t szIdx)
{
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);
	uint32_t u4SuppBandIdMask = 0;

	if (prNanScheduler->fgEn2g && NAN_IS_2G_TIMELINE(prAdapter, szIdx))
		u4SuppBandIdMask |= BIT(BAND_2G4);

	if ((prNanScheduler->fgEn5gH || prNanScheduler->fgEn5gL) &&
	    NAN_IS_5G_TIMELINE(prAdapter, szIdx))
		u4SuppBandIdMask |= BIT(BAND_5G);

#if (CFG_SUPPORT_NAN_6G == 1)
	if (prNanScheduler->fgEn6g && NAN_IS_6G_TIMELINE(prAdapter, szIdx))
		u4SuppBandIdMask |= BIT(BAND_6G);
#endif

	return u4SuppBandIdMask;
}

uint8_t nanGetTimelineNum(void)
{
	return sizeof(g_arNanTimelineMgmt)/sizeof(struct _NAN_TIMELINE_MGMT_T);
}

struct _NAN_CRB_NEGO_CTRL_T *
nanGetNegoControlBlock(struct ADAPTER *prAdapter)
{
	return &g_rNanSchNegoCtrl;
}

static u_int8_t nanChnlInfoEqual(union _NAN_BAND_CHNL_CTRL a,
				 union _NAN_BAND_CHNL_CTRL b)
{
	return NAN_GET_CHNL_BAND_INFO(a) == NAN_GET_CHNL_BAND_INFO(b) &&
		a.u4PrimaryChnl == b.u4PrimaryChnl;
}

u_int8_t nanOverConcurrentChannelLimit(struct ADAPTER *prAdapter,
				       struct _NAN_SCHEDULER_T *prNanScheduler,
				       uint8_t ucMaxConcurrentLimit,
				       union _NAN_BAND_CHNL_CTRL rNanChnlInfo)
{
	uint8_t i;
	uint8_t ucChannelNum = 1; /* the passed-in rNanChnlInfo */
	struct NAN_P2P_AIS_MCC_RECORD *prP2pAisMcc;
	union _NAN_BAND_CHNL_CTRL rSocialChnlInfo;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo[NAN_TIMELINE_MGMT_SIZE] = {0};
	uint8_t fgP2pAisSameChannel = FALSE;

	for (i = 0; i < ARRAY_SIZE(prNanScheduler->arP2pAisMcc); i++) {
		prP2pAisMcc = &prNanScheduler->arP2pAisMcc[i];
		ucChannelNum += prP2pAisMcc->ucNumOfChannel;
		if (NAN_IS_2G_TIMELINE(prAdapter, i))
			rSocialChnlInfo = g_r2gDwChnl;
		else
			rSocialChnlInfo = g_r5gDwChnl;

		rAisChnlInfo[i] = prP2pAisMcc->rAisChnlInfo;
		/* AIS == new || P2P == new || new == NAN social */
		if (prP2pAisMcc->rAisChnlInfo.u4PrimaryChnl &&
		    nanChnlInfoEqual(prP2pAisMcc->rAisChnlInfo, rNanChnlInfo) ||
		    prP2pAisMcc->rP2pChnlInfo.u4PrimaryChnl &&
		    nanChnlInfoEqual(prP2pAisMcc->rP2pChnlInfo, rNanChnlInfo) ||
		    nanChnlInfoEqual(rSocialChnlInfo, rNanChnlInfo)) {
			ucChannelNum--;
		}

		if (prP2pAisMcc->rAisChnlInfo.u4PrimaryChnl &&
		    prP2pAisMcc->rP2pChnlInfo.u4PrimaryChnl &&
		    nanChnlInfoEqual(prP2pAisMcc->rAisChnlInfo, rNanChnlInfo))
			fgP2pAisSameChannel = TRUE;
	}
#if CFG_SUPPORT_NAN_DBDC
	/* MLO use only single link, if STA counts two channels in two bands,
	 * both channels are different from NAN and P2P, decrease one.
	 */
	if (rAisChnlInfo[0].u4PrimaryChnl && rAisChnlInfo[1].u4PrimaryChnl &&
	    !nanChnlInfoEqual(rAisChnlInfo[0], g_r2gDwChnl) &&
	    !nanChnlInfoEqual(rAisChnlInfo[1], g_r5gDwChnl) &&
	    !fgP2pAisSameChannel) {
		DBGLOG(NAN, INFO, "MLO channels %u and %u, channel--",
		       rAisChnlInfo[0].u4PrimaryChnl,
		       rAisChnlInfo[1].u4PrimaryChnl);
		ucChannelNum--;
	}
#endif /* CFG_SUPPORT_NAN_DBDC */
	if (ucChannelNum > ucMaxConcurrentLimit)
		DBGLOG(NAN, DEBUG,
		       "Check OC=%u, ch=%u, ucChannelNum=%u, Limit=%u",
		       rNanChnlInfo.u4OperatingClass,
		       rNanChnlInfo.u4PrimaryChnl,
		       ucChannelNum, ucMaxConcurrentLimit);
	else
		DBGLOG(NAN, LOUD,
		       "Check OC=%u, ch=%u, ucChannelNum=%u, Limit=%u",
		       rNanChnlInfo.u4OperatingClass,
		       rNanChnlInfo.u4PrimaryChnl,
		       ucChannelNum, ucMaxConcurrentLimit);

	return ucChannelNum > ucMaxConcurrentLimit;
}

u_int8_t nanIsAllowedChannel(struct ADAPTER *prAdapter,
			     union _NAN_BAND_CHNL_CTRL rNanChnlInfo)
{
	enum ENUM_BAND eBand;
	struct _NAN_SCHEDULER_T *prNanScheduler;

	prNanScheduler = nanGetScheduler(prAdapter);

	eBand = nanRegGetNanChnlBand(rNanChnlInfo);

	if (!rlmDomainIsLegalChannel(prAdapter, eBand,
		rNanChnlInfo.u4PrimaryChnl))
		return FALSE;

	if (eBand == BAND_2G4)
		return prNanScheduler->fgEn2g;

	if (eBand == BAND_5G)
		return (prNanScheduler->fgEn5gL || prNanScheduler->fgEn5gH) &&
			!NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G); /* Use 2.4G */

#if (CFG_SUPPORT_NAN_6G == 1)
	if (eBand == BAND_6G) {
		/**
		 * limited to 4 channels:
		 * 1. band0: NAN(6), P2P(x), AIS(y)
		 *    band1: NAN(149), NAN (6G)
		 *    Total: = 5, over limit
		 * 2. band0: NAN(6), P2P(x)
		 *    band1: NAN(149), AIS(y), NAN(6G)
		 *    Total = 5, over limit
		 * 3. band0: NAN(6),AIS (x)
		 *    band1: NAN(149),P2P(y), NAN(y)
		 *    Total = 4
		 * 4. band0: NAN(6), NAN(6)
		 *    band1: NAN(149), P2P(x), AIS(y)
		 *    Total = 4
		 */
		if (prAdapter->chip_info &&
		    prAdapter->chip_info->ucMaxConcurrentLimit &&
		    prAdapter->chip_info->ucMaxConcurrentLimit == 4 &&
		    nanOverConcurrentChannelLimit(prAdapter, prNanScheduler,
			prAdapter->chip_info->ucMaxConcurrentLimit,
			rNanChnlInfo))
			return FALSE;

		return prNanScheduler->fgEn6g;
	}
#endif

	return FALSE;
}

static unsigned char
nanIsDiscWindow(struct ADAPTER *prAdapter, size_t szSlotIdx,
				size_t szTimeLineIdx)
{
	struct _NAN_SCHEDULER_T *prScheduler = nanGetScheduler(prAdapter);

	if (prScheduler->fgEn2g &&
	    NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
	    NAN_SLOT_INDEX(szSlotIdx) == NAN_2G_DW_INDEX)
		return TRUE;

	if ((prScheduler->fgEn5gH || prScheduler->fgEn5gL) &&
	    NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx) &&
	    NAN_SLOT_INDEX(szSlotIdx) == NAN_5G_DW_INDEX)
		return TRUE;

	return FALSE;
}

enum _ENUM_NAN_WINDOW_T
nanWindowType(struct ADAPTER *prAdapter, size_t szSlotIdx, size_t szTimeLineIdx)
{
	if (nanIsDiscWindow(prAdapter, szSlotIdx, szTimeLineIdx))
		return ENUM_NAN_DW;

	if (nanQueryPrimaryChnlBySlot(prAdapter, szSlotIdx, TRUE,
		szTimeLineIdx) == 0)
		return ENUM_NAN_IDLE_WINDOW;

	return ENUM_NAN_FAW;
}

void
nanSchedAvailAttrCtrlTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam)
{
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);

	prScheduler->u2NanCurrAvailAttrControlField = 0;

	nanSchedCmdUpdateAvailabilityCtrl(prAdapter);
}

struct _NAN_AVAILABILITY_DB_T *
nanSchedPeerAcquireAvailabilityDB(struct ADAPTER *prAdapter,
				  struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				  uint8_t ucMapId)
{
	struct _NAN_AVAILABILITY_DB_T *prNanAvailDB;
	uint32_t u4Idx;
	uint32_t u4InvalidIdx = NAN_NUM_AVAIL_DB;
	uint32_t u4EntryListPos;

	if (!prPeerSchDesc) {
		DBGLOG(NAN, ERROR, "prPeerSchDesc error!\n");
		return NULL;
	}

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prNanAvailDB = &prPeerSchDesc->arAvailAttr[u4Idx];
		if (prNanAvailDB->ucMapId == NAN_INVALID_MAP_ID) {
			if (u4InvalidIdx == NAN_NUM_AVAIL_DB)
				u4InvalidIdx = u4Idx;
		} else if (prNanAvailDB->ucMapId == ucMapId)
			return prNanAvailDB;
	}

	if (u4InvalidIdx != NAN_NUM_AVAIL_DB) {
		/* initialize new availability attribute */
		DBGLOG(NAN, DEBUG,
		       "alloc new availability for station idx:%d, mapID:%d\n",
		       u4InvalidIdx, ucMapId);
		prNanAvailDB = &prPeerSchDesc->arAvailAttr[u4InvalidIdx];
		prNanAvailDB->ucMapId = ucMapId;
		for (u4EntryListPos = 0;
		     u4EntryListPos < NAN_NUM_AVAIL_TIMELINE;
		     u4EntryListPos++) {

			prNanAvailDB->arAvailEntryList[u4EntryListPos]
				.fgActive = FALSE;
		}

		return prNanAvailDB;
	}

	return NULL;
}

void
nanSchedResetPeerSchDesc(struct ADAPTER *prAdapter,
			 struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	uint32_t u4Idx;

	kalMemZero(prPeerSchDesc, sizeof(struct _NAN_PEER_SCH_DESC_T));

	prPeerSchDesc->fgImmuNdlTimelineValid = FALSE;
	prPeerSchDesc->fgRangingTimelineValid = FALSE;

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prPeerSchDesc->arDevCapability[u4Idx].fgValid = FALSE;
		prPeerSchDesc->arDevCapability[u4Idx].ucMapId =
			NAN_INVALID_MAP_ID;
		prPeerSchDesc->arAvailAttr[u4Idx].ucMapId = NAN_INVALID_MAP_ID;
		prPeerSchDesc->arAvailAttr[u4Idx].u4AvailAttrToken = 0;
		prPeerSchDesc->arImmuNdlTimeline[u4Idx].ucMapId =
			NAN_INVALID_MAP_ID;
		prPeerSchDesc->arRangingTimeline[u4Idx].ucMapId =
			NAN_INVALID_MAP_ID;
	}
	prPeerSchDesc->arDevCapability[NAN_NUM_AVAIL_DB].fgValid = FALSE;
	prPeerSchDesc->arDevCapability[NAN_NUM_AVAIL_DB].ucMapId =
		NAN_INVALID_MAP_ID;

	prPeerSchDesc->u4QosMaxLatency = NAN_INVALID_QOS_MAX_LATENCY;
	prPeerSchDesc->u4QosMinSlots = NAN_INVALID_QOS_MIN_SLOTS;

	prPeerSchDesc->u4DevCapAttrToken = 0;
}

struct _NAN_PEER_SCH_DESC_T *
nanSchedAcquirePeerSchDescByNmi(struct ADAPTER *prAdapter,
		uint8_t *pucNmiAddr)
{
	struct LINK *prPeerSchDescList;
	struct LINK *prFreePeerSchDescList;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDescNext;
	struct _NAN_PEER_SCH_DESC_T *prAgingPeerSchDesc;
	struct _NAN_SCHEDULER_T *prScheduler;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return NULL;
	}

	prScheduler = nanGetScheduler(prAdapter);

	prPeerSchDescList = &prScheduler->rPeerSchDescList;
	prFreePeerSchDescList = &prScheduler->rFreePeerSchDescList;
	prAgingPeerSchDesc = NULL;

	LINK_FOR_EACH_ENTRY_SAFE(prPeerSchDesc, prPeerSchDescNext,
				 prPeerSchDescList, rLinkEntry,
				 struct _NAN_PEER_SCH_DESC_T) {
		if (EQUAL_MAC_ADDR(prPeerSchDesc->aucNmiAddr, pucNmiAddr)) {
			LINK_REMOVE_KNOWN_ENTRY(prPeerSchDescList,
						prPeerSchDesc);

			LINK_INSERT_TAIL(prPeerSchDescList,
					 &prPeerSchDesc->rLinkEntry);
			DBGLOG(NAN, LOUD, "Find peer schedule desc %p [%d]\n",
			       prPeerSchDesc, __LINE__);
			return prPeerSchDesc;
		} else if ((prAgingPeerSchDesc == NULL) &&
			   (prPeerSchDesc->fgUsed == FALSE))
			prAgingPeerSchDesc = prPeerSchDesc;
	}

	LINK_REMOVE_HEAD(prFreePeerSchDescList, prPeerSchDesc,
			 struct _NAN_PEER_SCH_DESC_T *);
	if (prPeerSchDesc) {
		nanSchedResetPeerSchDesc(prAdapter, prPeerSchDesc);
		kalMemCopy(prPeerSchDesc->aucNmiAddr, pucNmiAddr, MAC_ADDR_LEN);

		LINK_INSERT_TAIL(prPeerSchDescList, &prPeerSchDesc->rLinkEntry);
	} else if (prAgingPeerSchDesc) {
		LINK_REMOVE_KNOWN_ENTRY(prPeerSchDescList, prAgingPeerSchDesc);

		nanSchedResetPeerSchDesc(prAdapter, prAgingPeerSchDesc);
		kalMemCopy(prAgingPeerSchDesc->aucNmiAddr, pucNmiAddr,
			   MAC_ADDR_LEN);

		LINK_INSERT_TAIL(prPeerSchDescList,
				 &prAgingPeerSchDesc->rLinkEntry);
		DBGLOG(NAN, LOUD, "Find peer schedule desc %p [%d]\n",
		       prPeerSchDesc, __LINE__);
		return prAgingPeerSchDesc;
	}

	DBGLOG(NAN, LOUD, "Find peer schedule desc %p [%d]\n", prPeerSchDesc,
	       __LINE__);
	return prPeerSchDesc;
}

void
nanSchedReleaseAllPeerSchDesc(struct ADAPTER *prAdapter)
{
	struct LINK *prPeerSchDescList;
	struct LINK *prFreePeerSchDescList;
	struct _NAN_SCHEDULER_T *prNanScheduler;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return;
	}

	prNanScheduler = nanGetScheduler(prAdapter);
	prPeerSchDescList = &prNanScheduler->rPeerSchDescList;
	prFreePeerSchDescList = &prNanScheduler->rFreePeerSchDescList;

	while (!LINK_IS_EMPTY(prPeerSchDescList)) {
		LINK_REMOVE_HEAD(prPeerSchDescList, prPeerSchDesc,
				 struct _NAN_PEER_SCH_DESC_T *);
		if (prPeerSchDesc) {
			kalMemZero(prPeerSchDesc, sizeof(*prPeerSchDesc));
			LINK_INSERT_TAIL(prFreePeerSchDescList,
					 &prPeerSchDesc->rLinkEntry);
		} else {
			/* should not deliver to this function */
			DBGLOG(NAN, ERROR, "prPeerSchDesc error!\n");
			return;
		}
	}
}

struct _NAN_PEER_SCH_DESC_T *
nanSchedSearchPeerSchDescByNmi(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	struct LINK *prPeerSchDescList;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_SCHEDULER_T *prNanScheduler;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return NULL;
	}
	prNanScheduler = nanGetScheduler(prAdapter);

	prPeerSchDescList = &prNanScheduler->rPeerSchDescList;

	LINK_FOR_EACH_ENTRY(prPeerSchDesc, prPeerSchDescList, rLinkEntry,
			    struct _NAN_PEER_SCH_DESC_T) {
		if (EQUAL_MAC_ADDR(prPeerSchDesc->aucNmiAddr, pucNmiAddr))
			return prPeerSchDesc;
	}

	return NULL;
}

struct _NAN_PEER_SCHEDULE_RECORD_T *
nanSchedGetPeerSchRecord(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	if (u4SchIdx < NAN_MAX_CONN_CFG)
		return &g_arNanPeerSchedRecord[u4SchIdx];

	return NULL;
}

void
nanSchedResetFawNdcTimeline(void)
{
	uint8_t ucIdx = 0;

	for (ucIdx = 0; ucIdx < NAN_TIMELINE_MGMT_SIZE; ucIdx++)
		kalMemZero(&g_arNanFawNdcTimeline[ucIdx],
			sizeof(struct _NAN_FAW_NDC_TIMELINE_T));
}

struct _NAN_FAW_NDC_TIMELINE_T*
nanSchedGetFawNdcTimeline(uint8_t ucTimelineMgmtIndex)
{
	return &g_arNanFawNdcTimeline[ucTimelineMgmtIndex];
}


struct _NAN_PEER_SCH_DESC_T *
nanSchedGetPeerSchDesc(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRec)
		return prPeerSchRec->prPeerSchDesc;

	return NULL;
}

unsigned char
nanSchedPeerSchRecordIsValid(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRec)
		return prPeerSchRec->fgActive;

	return FALSE;
}

/* For printing the channgl of NDC specified by Map ID and Availability */
static union _NAN_BAND_CHNL_CTRL
nanGetChnlInfoByMapIdAndBitmap(struct ADAPTER *prAdapter,
			       uint8_t ucMapId, uint32_t u4Avail,
			       uint8_t *fgIsCommitted)
{
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prTimeline;
	size_t i;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	DBGLOG(NAN, INFO, "Find Map=%u, Avail=%08x\n", ucMapId, u4Avail);

	for (szTimeLineIdx = szNanActiveTimelineNum; szTimeLineIdx--; ) {
		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, szTimeLineIdx);
		if (prNanTimelineMgmt->ucMapId != ucMapId)
			continue;

		for (i = ARRAY_SIZE(prNanTimelineMgmt->arChnlList); i--; ) {
			prTimeline = &prNanTimelineMgmt->arChnlList[i];

			if ((prTimeline->au4AvailMap[0] & u4Avail) == 0)
				continue;

			*fgIsCommitted = TRUE;
			return  prTimeline->rChnlInfo;
		}

		for (i = ARRAY_SIZE(prNanTimelineMgmt->arChnlList); i--; ) {
			prTimeline = &prNanTimelineMgmt->arCondChnlList[i];

			if ((prTimeline->au4AvailMap[0] & u4Avail) == 0)
				continue;

			return  prTimeline->rChnlInfo;
		}

	}

	return g_rNullChnl;
}

void nanSchedDumpPeerSchDesc(struct ADAPTER *prAdapter,
			     struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	uint32_t u4Idx;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	u_int8_t fgPrinted = FALSE;

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, DEBUG, "null peer sch desc\n");
		return;
	}

	nanSchedDbgDumpPeerAvailability(prAdapter, prPeerSchDesc->aucNmiAddr);

	if (prPeerSchDesc->rSelectedNdcCtrl.fgValid == TRUE) {
		union _NAN_BAND_CHNL_CTRL rChnl;
		u_int8_t fgIsCommitted = FALSE;

		for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
			prTimeline = &prPeerSchDesc->rSelectedNdcCtrl
					.arTimeline[u4Idx];
			if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			rChnl = nanGetChnlInfoByMapIdAndBitmap(prAdapter,
						prTimeline->ucMapId,
						prTimeline[0].au4AvailMap[0],
						&fgIsCommitted);
			DBGLOG(NAN, DEBUG,
			       "NDC ID: %02x:%02x:%02x:%02x:%02x:%02x MapID:%d, DbIdx:%u, Avail:%02x-%02x-%02x-%02x, ch=%u(%c)",
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[0],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[1],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[2],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[3],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[4],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[5],
			       prTimeline->ucMapId, u4Idx,
			       ((uint8_t *)prTimeline[0].au4AvailMap)[0],
			       ((uint8_t *)prTimeline[0].au4AvailMap)[1],
			       ((uint8_t *)prTimeline[0].au4AvailMap)[2],
			       ((uint8_t *)prTimeline[0].au4AvailMap)[3],
			       rChnl.u4PrimaryChnl,
			       rChnl.u4PrimaryChnl ?
				       (fgIsCommitted ? 'C' : 'c') : ' ');
			fgPrinted = TRUE;
		}

		if  (!fgPrinted)
			DBGLOG(NAN, INFO,
			       "NDC ID: %02x:%02x:%02x:%02x:%02x:%02x",
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[0],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[1],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[2],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[3],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[4],
			       prPeerSchDesc->rSelectedNdcCtrl.aucNdcId[5]);
	}

	if (prPeerSchDesc->fgImmuNdlTimelineValid) {
		for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
			prTimeline = &prPeerSchDesc->arImmuNdlTimeline[u4Idx];

			if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			nanUtilDump(prAdapter, "ImmuNDL",
					(uint8_t *)prTimeline->au4AvailMap,
					sizeof(prTimeline->au4AvailMap));
		}
	}

	if (prPeerSchDesc->fgRangingTimelineValid) {
		for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
			prTimeline = &prPeerSchDesc->arRangingTimeline[u4Idx];

			if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			nanUtilDump(prAdapter, "Rang Map",
					(uint8_t *)prTimeline->au4AvailMap,
					sizeof(prTimeline->au4AvailMap));
		}
	}

	DBGLOG(NAN, DEBUG, "[QoS] MinSlot:%d, MaxLatency:%d\n",
		prPeerSchDesc->u4QosMinSlots, prPeerSchDesc->u4QosMaxLatency);
}

uint32_t
nanSchedLookupPeerSchRecordIdx(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	uint32_t u4Idx;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	for (u4Idx = 0; u4Idx < NAN_MAX_CONN_CFG; u4Idx++) {
		prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4Idx);

		if (prPeerSchRec && prPeerSchRec->fgActive &&
		    EQUAL_MAC_ADDR(prPeerSchRec->aucNmiAddr, pucNmiAddr)) {
			DBGLOG(NAN, DEBUG, "Find peer schedule record %d\n",
			       u4Idx);
			return u4Idx;
		}
	}

	return NAN_INVALID_SCH_RECORD;
}

struct _NAN_PEER_SCHEDULE_RECORD_T *
nanSchedLookupPeerSchRecord(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	uint32_t u4Idx;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	for (u4Idx = 0; u4Idx < NAN_MAX_CONN_CFG; u4Idx++) {
		prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4Idx);

		if (prPeerSchRec && prPeerSchRec->fgActive &&
		    EQUAL_MAC_ADDR(prPeerSchRec->aucNmiAddr, pucNmiAddr)) {

			DBGLOG(NAN, DEBUG, "Find peer schedule record %d\n",
			       u4Idx);
			return prPeerSchRec;
		}
	}

	return NULL;
}

uint32_t
nanSchedQueryStaRecIdx(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
		       uint32_t u4Idx, uint8_t ucLinkIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRec == NULL) {
		DBGLOG(NAN, ERROR, "Get Peer Sch Record %d error\n", u4SchIdx);
		return STA_REC_INDEX_NOT_FOUND;
	}

	if (ucLinkIdx >= NAN_LINK_NUM)
		return prPeerSchRec->aucStaRecIdx[NAN_MAIN_LINK_INDEX][u4Idx];

	return prPeerSchRec->aucStaRecIdx[ucLinkIdx][u4Idx];
}

uint32_t
nanSchedResetPeerSchedRecord(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	uint32_t u4Idx;
	uint32_t i;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (!prPeerSchRecord) {
		DBGLOG(NAN, ERROR, "prPeerSchRecord error!\n");
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prPeerSchRecord, sizeof(*prPeerSchRecord));

	/* Set non-zero initialized fields */
	for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDP_CXT_NUM; u4Idx++)
		for (i = 0; i < NAN_LINK_NUM; i++)
			prPeerSchRecord->aucStaRecIdx[i][u4Idx] =
				STA_REC_INDEX_NOT_FOUND;

	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_SIZE; u4Idx++) {
		prPeerSchRecord->arCommImmuNdlTimeline[u4Idx].ucMapId =
			NAN_INVALID_MAP_ID;
		prPeerSchRecord->arCommRangingTimeline[u4Idx].ucMapId =
			NAN_INVALID_MAP_ID;
		prPeerSchRecord->arCommFawTimeline[u4Idx].ucMapId =
			NAN_INVALID_MAP_ID;
	}

	prPeerSchRecord->u4FinalQosMaxLatency = NAN_INVALID_QOS_MAX_LATENCY;
	prPeerSchRecord->u4FinalQosMinSlots = NAN_INVALID_QOS_MIN_SLOTS;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedAcquirePeerSchedRecord(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	uint32_t u4SchIdx;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	DBGLOG(NAN, TRACE, "IN\n");

	for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
		if (prPeerSchRecord->fgActive == FALSE)
			break;
	}

	if (u4SchIdx >= NAN_MAX_CONN_CFG)
		return NAN_INVALID_SCH_RECORD;

	prPeerSchDesc = nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);

	if (!prPeerSchDesc) {
		DBGLOG(NAN, ERROR, "prPeerSchDesc error!\n");
		return NAN_INVALID_SCH_RECORD;
	}
	DBGLOG(NAN, LOUD, "Find peer schedule desc %p\n", prPeerSchDesc);
	prPeerSchDesc->fgUsed = TRUE;
	prPeerSchDesc->u4SchIdx = u4SchIdx;

	nanSchedResetPeerSchedRecord(prAdapter, u4SchIdx);
	prPeerSchRecord->fgActive = TRUE;
	kalMemCopy(prPeerSchRecord->aucNmiAddr, pucNmiAddr, MAC_ADDR_LEN);
	prPeerSchRecord->prPeerSchDesc = prPeerSchDesc;

	nanSchedCmdManagePeerSchRecord(prAdapter, u4SchIdx, TRUE);
	nanSchedCmdUpdatePeerCapability(prAdapter, u4SchIdx);

	return u4SchIdx;
}

uint32_t
nanSchedReleasePeerSchedRecord(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;

	DBGLOG(NAN, TRACE, "IN\n");

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

	if (prPeerSchRecord == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchRecord\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prPeerSchRecord->prPeerSchDesc)
		prPeerSchRecord->prPeerSchDesc->fgUsed = FALSE;

	nanSchedResetPeerSchedRecord(prAdapter, u4SchIdx);

	nanExtClearCustomNdpFaw(u4SchIdx);

	nanSchedCmdManagePeerSchRecord(prAdapter, u4SchIdx, FALSE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedPeerGetAvailabilityDbIdx(struct ADAPTER *prAdapter,
				 struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				 uint32_t u4MapId)
{
	uint32_t u4Idx;

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		if (prPeerSchDesc->arAvailAttr[u4Idx].ucMapId == u4MapId)
			return u4Idx;
	}

	return NAN_INVALID_AVAIL_DB_INDEX;
}

/* Collect peer supported band by peer Availability attribute info
 * return by bitmask, with bits set by NAN_SUPPORTED_BANDS
 * The argument prAvailability is associated with a specific timeline
 * Caller can use this function to check the band information of this timeline
 * 0x04: 2G
 * 0x10: 5G
 * 0x40, 0x80: 6G
 */
static enum ENUM_BAND
nanGetAvailabilityBand(struct _NAN_AVAILABILITY_DB_T *prAvailability)
{
	struct _NAN_AVAILABILITY_TIMELINE_T *prAvailabilityTimeline;
	union _NAN_BAND_CHNL_CTRL *prBandChannel;
	uint32_t u4SupportedBands = 0;
	uint32_t u4OpClass;
	size_t i;
	size_t j;

	if (!prAvailability)
		return 0;

	if (prAvailability->ucMapId == NAN_INVALID_MAP_ID)
		return 0;

	for (i = 0; i < ARRAY_SIZE(prAvailability->arAvailEntryList); i++) {
		prAvailabilityTimeline = &prAvailability->arAvailEntryList[i];
		if (!prAvailabilityTimeline->fgActive)
			continue;

		DBGLOG(NAN, INFO, "ucNumBandChnlCtrl=%u",
		       prAvailabilityTimeline->ucNumBandChnlCtrl);
		for (j = 0; j < prAvailabilityTimeline->ucNumBandChnlCtrl;
		     j++) {
			prBandChannel =
				&prAvailabilityTimeline->arBandChnlCtrl[j];

			DBGLOG(NAN, INFO, "prBandChannel[%zu]=%04x, type=%u",
			       j, prBandChannel->u4RawData,
			       prBandChannel->u4Type);

			if (prBandChannel->u4Type ==
			    NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {
				u4SupportedBands |=
					prBandChannel->u4BandIdMask;
				DBGLOG(NAN, INFO, "[%zu] band=%02x",
				       j, prBandChannel->u4BandIdMask);
			} else if (prBandChannel->u4Type ==
				   NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL) {
				u4OpClass = prBandChannel->u4OperatingClass;

				DBGLOG(NAN, INFO, "[%zu] OC=%u", j, u4OpClass);

				if (IS_2G_OP_CLASS(u4OpClass))
					u4SupportedBands |=
						BIT(NAN_SUPPORTED_BAND_ID_2P4G);
				else if (IS_5G_OP_CLASS(u4OpClass))
					u4SupportedBands |=
						BIT(NAN_SUPPORTED_BAND_ID_5G);
				else if (IS_6G_OP_CLASS(u4OpClass))
					u4SupportedBands |=
						BIT(NAN_SUPPORTED_BAND_ID_6G);
			}
		}
	}

	return u4SupportedBands;
}

static struct _NAN_AVAILABILITY_DB_T *
nanSchedPeerAvailabilityDbByDesc(struct ADAPTER *prAdapter,
				 struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				 uint32_t u4AvailDbIdx)
{
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;

	prAvailabilityDB = &prPeerSchDesc->arAvailAttr[u4AvailDbIdx];
	if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
		return NULL;

	return prAvailabilityDB;
}

static struct _NAN_AVAILABILITY_DB_T *
nanSchedPeerAvailabilityDbByID(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx, uint32_t u4AvailDbIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

	if (prPeerSchRec == NULL)
		return NULL;

	if (prPeerSchRec->prPeerSchDesc == NULL)
		return NULL;

	return nanSchedPeerAvailabilityDbByDesc(prAdapter,
				prPeerSchRec->prPeerSchDesc, u4AvailDbIdx);
}

unsigned char
nanSchedPeerAvailabilityDbValidByDesc(struct ADAPTER *prAdapter,
				     struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				     uint32_t u4AvailDbIdx)
{
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;

	prAvailabilityDB = &prPeerSchDesc->arAvailAttr[u4AvailDbIdx];
	if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
		return FALSE;

	return TRUE;
}

unsigned char
nanSchedPeerAvailabilityDbValidByID(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx, uint32_t u4AvailDbIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

	if (prPeerSchRec == NULL)
		return FALSE;

	if (prPeerSchRec->prPeerSchDesc == NULL)
		return FALSE;

	return nanSchedPeerAvailabilityDbValidByDesc(
		prAdapter, prPeerSchRec->prPeerSchDesc, u4AvailDbIdx);
}

u_int8_t nanSchedPeerAvailabilityDbValid(struct ADAPTER *prAdapter,
					 uint32_t u4SchIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;
	uint32_t u4AvailDbIdx;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

	if (prPeerSchRec == NULL)
		return FALSE;

	if (prPeerSchRec->prPeerSchDesc == NULL)
		return FALSE;

	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB; u4AvailDbIdx++)
		if (nanSchedPeerAvailabilityDbValidByDesc(
			    prAdapter, prPeerSchRec->prPeerSchDesc,
			    u4AvailDbIdx))
			return TRUE;

	return FALSE;
}

uint32_t nanSchedInit(struct ADAPTER *prAdapter)
{
	uint32_t u4Idx = 0;
	uint8_t ucTimeLineIdx = 0;
	struct _NAN_SCHEDULER_T *prNanScheduler = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	size_t szNanActiveTimelineNum;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prNanScheduler = nanGetScheduler(prAdapter);

	DBGLOG(NAN, DEBUG, "Init:%d\n", prNanScheduler->fgInit);
	DBGLOG(NAN, DEBUG, "Supported Timeline number:%d\n",
		NAN_TIMELINE_MGMT_SIZE);
	DBGLOG(NAN, DEBUG, "Supported chnl list number:%d\n",
		NAN_TIMELINE_MGMT_CHNL_LIST_NUM);

#if (CFG_SUPPORT_NAN_DBDC == 1)
	/* Set default multiple map flag in NAN init stage */
	if (prAdapter->rWifiVar.ucNanMapMask < NAN_TIMELINE_MGMT_SIZE) {
		prAdapter->fgNanMultipleMapTimeline = FALSE;
	} else {
		if (prAdapter->rWifiVar.fgDbDcModeEn)
			prAdapter->fgNanMultipleMapTimeline = TRUE;
		else
			prAdapter->fgNanMultipleMapTimeline = FALSE;
	}
#endif

	if (prNanScheduler->fgInit == FALSE) {
		/* Clear peer schedule record and descriptor */
		for (u4Idx = 0; u4Idx < NAN_MAX_CONN_CFG; u4Idx++) {
			prPeerSchRecord =
				nanSchedGetPeerSchRecord(prAdapter, u4Idx);
			kalMemZero(prPeerSchRecord,
				   sizeof(struct _NAN_PEER_SCHEDULE_RECORD_T));
		}

		kalMemZero((uint8_t *)prNegoCtrl, sizeof(*prNegoCtrl));
		kalMemZero((uint8_t *)prNanScheduler, sizeof(*prNanScheduler));

		LINK_INITIALIZE(&prNanScheduler->rPeerSchDescList);
		LINK_INITIALIZE(&prNanScheduler->rFreePeerSchDescList);
		for (u4Idx = 0; u4Idx < NAN_NUM_PEER_SCH_DESC; u4Idx++) {
			kalMemZero(&prNanScheduler->arPeerSchDescArray[u4Idx],
				   sizeof(struct _NAN_PEER_SCH_DESC_T));
			LINK_INSERT_TAIL(
				&prNanScheduler->rFreePeerSchDescList,
				&prNanScheduler->arPeerSchDescArray[u4Idx]
					 .rLinkEntry);
		}

		/* Initialize timer */
		cnmTimerInitTimer(prAdapter, &prNegoCtrl->rCrbNegoDispatchTimer,
				  nanSchedNegoDispatchTimeout, 0);
		cnmTimerInitTimer(prAdapter,
				  &prNanScheduler->rAvailAttrCtrlResetTimer,
				  nanSchedAvailAttrCtrlTimeout, 0);
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
		cnmTimerInitTimer(prAdapter,
				  &prNanScheduler->rResumeRescheduleTimer,
				  nanResumeRescheduleTimeout, 0);
#endif
	}

	/* Reset scheduler parameters */
	prNanScheduler->fgInit = TRUE;
	prNanScheduler->fgEn2g = 0;
	prNanScheduler->fgEn5gH = 0;
	prNanScheduler->fgEn5gL = 0;
	kalMemZero((uint8_t *)prNanScheduler->arNdcCtrl,
		sizeof_field(struct _NAN_SCHEDULER_T, arNdcCtrl));
	prNanScheduler->fgAttrDevCapValid = FALSE;
	kalMemZero((uint8_t *)&prNanScheduler->rAttrDevCap,
		sizeof_field(struct _NAN_SCHEDULER_T, rAttrDevCap));

	/* Clear availability related info */
	prNanScheduler->ucNanAvailAttrSeqId = 0;
	prNanScheduler->u2NanAvailAttrControlField = 0;
	prNanScheduler->u2NanCurrAvailAttrControlField = 0;
	kalMemZero((uint8_t *)prNanScheduler->au4NumOfPotentialChnlList,
		sizeof_field(struct _NAN_SCHEDULER_T,
		au4NumOfPotentialChnlList));
	kalMemZero((uint8_t *)prNanScheduler->aarPotentialChnlList,
		sizeof_field(struct _NAN_SCHEDULER_T, aarPotentialChnlList));
	prNanScheduler->ucCommitDwInterval =
		prAdapter->rWifiVar.ucNanCommittedDw;
	kalMemZero(prNanScheduler->arCustFawEntry,
		   sizeof(prNanScheduler->arCustFawEntry));
	prNanScheduler->eLocalCommittedBand = ENUM_SUPPORTED_BN_2G;

	for (u4Idx = 0; u4Idx < NAN_MAX_NDC_RECORD; u4Idx++)
		prNanScheduler->arNdcCtrl[u4Idx].fgValid = FALSE;

	szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	for (ucTimeLineIdx = 0; ucTimeLineIdx < NAN_TIMELINE_MGMT_SIZE;
		ucTimeLineIdx++) {
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
					ucTimeLineIdx);
		kalMemZero((uint8_t *)prNanTimelineMgmt,
				sizeof(struct _NAN_TIMELINE_MGMT_T));
		prNanTimelineMgmt->ucMapId =
			(uint8_t)(szNanActiveTimelineNum - ucTimeLineIdx);
		prNanTimelineMgmt->fgChkCondAvailability = FALSE;
		for (u4Idx = 0;
			u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
			prNanTimelineMgmt->arChnlList[u4Idx].fgValid = FALSE;
			prNanTimelineMgmt->arCondChnlList[u4Idx].fgValid
						= FALSE;
			prNanTimelineMgmt->arCustChnlList[u4Idx].fgValid
						= FALSE;
		}
	}

	prNegoCtrl->eSyncSchUpdateCurrentState =
		ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE;
	prNegoCtrl->eSyncSchUpdateLastState =
		ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE;

	for (u4Idx = 0; u4Idx < NAN_MAX_CONN_CFG; u4Idx++)
		nanSchedReleasePeerSchedRecord(prAdapter, u4Idx);

	nanSchedResetFawNdcTimeline();

	nanSchedReleaseAllPeerSchDesc(prAdapter);

	nanSchedConfigPhyParams(prAdapter);
	nanSchedCmdUpdateSchedVer(prAdapter);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedUninit(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prNanScheduler;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prNanScheduler = nanGetScheduler(prAdapter);
	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	prNanScheduler->fgInit = FALSE;

	cnmTimerStopTimer(prAdapter,
		&prNegoCtrl->rCrbNegoDispatchTimer);
	cnmTimerStopTimer(prAdapter,
		&prNanScheduler->rAvailAttrCtrlResetTimer);
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	cnmTimerStopTimer(prAdapter,
		&prNanScheduler->rResumeRescheduleTimer);
#endif

	kalMemZero(prNanScheduler->arConcurrentCust,
		   sizeof(prNanScheduler->arConcurrentCust));
	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanUtilCalAttributeToken(struct _NAN_ATTR_HDR_T *prNanAttr)
{
	uint8_t *pucPayload;
	uint32_t u4Token, u4Len;
	uint32_t u4Tail;

	pucPayload = prNanAttr->aucAttrBody;
	u4Len = prNanAttr->u2Length;
	u4Token = pucPayload[0];
	pucPayload++;
	u4Len--;

	while (u4Len > 4) {
		u4Token ^= *(uint32_t *)pucPayload;
		pucPayload += 4;
		u4Len -= 4;
	}

	u4Tail = 0;
	while (u4Len) {
		u4Tail <<= 8;
		u4Tail |= pucPayload[0];
		pucPayload++;
		u4Len--;
	}
	u4Token ^= u4Tail;

	return u4Token;
}

uint32_t nanUtilCheckBitOneCnt(void *pBuffer, uint32_t u4Size)
{
	uint32_t u4Num;
	uint32_t u4Idx;
	uint8_t *pucBitMask = pBuffer;

	u4Num = 0;
	for (u4Idx = 0; u4Idx < u4Size * 8; u4Idx++) {
		if (pucBitMask[u4Idx / 8] & BIT(u4Idx % 8))
			u4Num++;
	}

	return u4Num;
}

void nanUtilDump(struct ADAPTER *prAdapter, uint8_t *pucMsg,
		 uint8_t *pucContent, uint32_t u4Length)
{
	uint8_t aucBuf[16];

	while (u4Length >= 16) {
		DBGLOG(NAN, DEBUG,
		       "%p: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       pucContent, pucContent[0], pucContent[1], pucContent[2],
		       pucContent[3], pucContent[4], pucContent[5],
		       pucContent[6], pucContent[7], pucContent[8],
		       pucContent[9], pucContent[10], pucContent[11],
		       pucContent[12], pucContent[13], pucContent[14],
		       pucContent[15]);

		u4Length -= 16;
		pucContent += 16;
	}

	if (u4Length > 0) {
		kalMemZero(aucBuf, 16);
		kalMemCopy(aucBuf, pucContent, u4Length);

		DBGLOG(NAN, DEBUG,
		       "%p: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       pucContent, aucBuf[0], aucBuf[1], aucBuf[2], aucBuf[3],
		       aucBuf[4], aucBuf[5], aucBuf[6], aucBuf[7], aucBuf[8],
		       aucBuf[9], aucBuf[10], aucBuf[11], aucBuf[12],
		       aucBuf[13], aucBuf[14], aucBuf[15]);
	}
}

/* pucTimeBitmapField:
 *       return Time Bitmap Field (must provide at least 67 bytes)
 *
 *   pu4TimeBitmapFieldLength:
 *       return real Time Bitmap Field total length
 */
uint32_t
nanParserGenTimeBitmapField(struct ADAPTER *prAdapter, uint32_t *pu4AvailMap,
			    uint8_t *pucTimeBitmapField,
			    uint32_t *pu4TimeBitmapFieldLength)
{
	uint32_t u4RepeatInterval;
	uint32_t u4StartOffset;
	uint32_t u4BitDuration;
	uint32_t u4BitDurationChecked;
	uint32_t u4BitmapLength;
	uint32_t u4BitmapPos1, u4BitmapPos2;
	uint32_t u4CheckPos;
	unsigned char fgCheckDone;
	unsigned char fgBitSet, fgBitSetCheck;
	uint8_t aucTimeBitmapField[NAN_TIME_BITMAP_FIELD_MAX_LENGTH] = {0};
	uint16_t *pu2TimeBitmapControl;
	uint8_t *pucTimeBitmapLength;
	uint8_t *pucTimeBitmapValue;
	uint8_t *pucBitmap;
	uint16_t u2TimeBitmapControl;

	pucBitmap = (uint8_t *)&pu4AvailMap[0];

	for (u4StartOffset = 0;
	     u4StartOffset < NAN_TIME_BITMAP_MAX_SIZE * CHAR_BIT;
	     u4StartOffset++) {
		if (NAN_IS_U8_BIT_SET(pucBitmap, u4StartOffset))
			break;
	}
#if CFG_NAN_SIGMA_TEST
	u4StartOffset = 0;
#endif

	/**
	 * Find proper u4RepeatInterval
	 * 8192TU =  512 bits (16 TU / bit) => 64 bytes
	 * e.g., 8192TU: 0, check 2nd part from 32 byte, length = 32 bytes
	 */
	u4RepeatInterval = ENUM_TIME_BITMAP_CTRL_PERIOD_8192;
	u4BitmapLength = NAN_TIME_BITMAP_MAX_SIZE;
	fgCheckDone = FALSE;
	do {
		u4BitmapLength >>= 1;
		u4BitmapPos1 = 0;
		u4BitmapPos2 = u4BitmapLength;

		for (u4CheckPos = 0; u4CheckPos < u4BitmapLength;
		     u4CheckPos++) {
			if (pucBitmap[u4BitmapPos1] !=
			    pucBitmap[u4BitmapPos2]) {
				fgCheckDone = TRUE;
				break;
			}

			u4BitmapPos1++;
			u4BitmapPos2++;
		}

		if (!fgCheckDone)
			u4RepeatInterval--;

#if CFG_NAN_SIGMA_TEST /* To prevent IOT issue for bitmap length less than 4 */
		if (u4RepeatInterval == ENUM_TIME_BITMAP_CTRL_PERIOD_512)
			fgCheckDone = 1;
#endif

	} while (u4RepeatInterval > ENUM_TIME_BITMAP_CTRL_PERIOD_128 &&
		 !fgCheckDone);


	/* Find proper u4BitDuration */
	u4BitDuration = ENUM_TIME_BITMAP_CTRL_DURATION_128;
	u4CheckPos = u4StartOffset;
	while ((u4CheckPos < (1 << (u4RepeatInterval + 2))) &&
	       (u4BitDuration > ENUM_TIME_BITMAP_CTRL_DURATION_16)) {
		fgBitSet = NAN_IS_U8_BIT_SET(pucBitmap, u4CheckPos);

		u4BitmapPos1 = u4CheckPos + 1;
		for (u4BitDurationChecked = (1 << u4BitDuration) - 1;
		     u4BitDurationChecked > 0; u4BitDurationChecked--) {
			if (u4BitmapPos1 >= (1 << (u4RepeatInterval + 2))) {
				u4BitDurationChecked = 0;
				break;
			}

			fgBitSetCheck =
				NAN_IS_U8_BIT_SET(pucBitmap, u4BitmapPos1);
			if (fgBitSet != fgBitSetCheck)
				break;

			u4BitmapPos1++;
		}

		if (u4BitDurationChecked == 0)
			u4CheckPos += (1 << u4BitDuration);
		else
			u4BitDuration--;
	}
#if CFG_NAN_SIGMA_TEST
	u4BitDuration = ENUM_TIME_BITMAP_CTRL_DURATION_16;
#endif

	pu2TimeBitmapControl = (uint16_t *)&aucTimeBitmapField[0];
	pucTimeBitmapLength = &aucTimeBitmapField[2];
	pucTimeBitmapValue = &aucTimeBitmapField[3];

	u2TimeBitmapControl =
		(u4BitDuration << NAN_TIME_BITMAP_CTRL_DURATION_OFFSET) |
		(u4RepeatInterval << NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET) |
		(u4StartOffset << NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET);
	kalMemCopy(pu2TimeBitmapControl, &u2TimeBitmapControl, 2);

	u4CheckPos = u4StartOffset;
	u4BitmapPos1 = 0;
	while (u4CheckPos < (1 << (u4RepeatInterval + 2))) {
		/* NAN_IS_AVAIL_MAP_SET */
		if (NAN_IS_U8_BIT_SET(pucBitmap, u4CheckPos)) {
			/* NAN_TIMELINE_SET  */
			NAN_U8_BIT_SET(pucTimeBitmapValue, u4BitmapPos1);
			u4BitmapPos2 = u4BitmapPos1;
		}

		u4BitmapPos1++;
		u4CheckPos += (1 << u4BitDuration);
	}
	*pucTimeBitmapLength = 1 + u4BitmapPos2 / 8;
#if CFG_NAN_SIGMA_TEST
	if (*pucTimeBitmapLength < 4)
		*pucTimeBitmapLength = 4;
#endif

	*pu4TimeBitmapFieldLength = *pucTimeBitmapLength + 3;
	kalMemMove(pucTimeBitmapField, aucTimeBitmapField,
		   *pu4TimeBitmapFieldLength);

	DBGLOG(NAN, LOUD,
	       "BitDur:%d, RepeatPeriod:%d, StartOffset:%d, Bitmap:%02x-%02x-%02x-%02x, len:%d\n",
	       u4BitDuration, u4RepeatInterval, u4StartOffset,
	       pucTimeBitmapValue[0], pucTimeBitmapValue[1],
	       pucTimeBitmapValue[2], pucTimeBitmapValue[3],
	       *pucTimeBitmapLength);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanParserGenChnlEntryField(struct ADAPTER *prAdapter,
			   union _NAN_BAND_CHNL_CTRL *prChnlCtrl,
			   struct _NAN_CHNL_ENTRY_T *prChnlEntry)
{
	uint32_t u4Bw = 0;
	uint8_t ucPrimaryChnl;
	uint8_t ucOperatingClass;
	uint8_t ucCenterChnl = 0;
	uint8_t ucChnlLowerBound = 0;

	ucOperatingClass = prChnlCtrl->u4OperatingClass;
	ucPrimaryChnl = prChnlCtrl->u4PrimaryChnl;

	u4Bw = nanRegGetBw(ucOperatingClass);
	prChnlEntry->ucOperatingClass = ucOperatingClass;

#if (CFG_SUPPORT_NAN_6G == 1)
	if (IS_6G_OP_CLASS(ucOperatingClass)) {
		if (u4Bw == 20) {
			prChnlEntry->ucPrimaryChnlBitmap = 0;
			prChnlEntry->u2AuxChannelBitmap = 0;
			nanRegGetChannelBitmap(ucOperatingClass, ucPrimaryChnl,
				       &prChnlEntry->u2ChannelBitmap);
		} else if ((u4Bw == 40) || (u4Bw == 80)) {
			ucCenterChnl = nanRegGetCenterChnlByPriChnl(
				ucOperatingClass, ucPrimaryChnl);
			nanRegGetChannelBitmap(ucOperatingClass, ucCenterChnl,
				       &prChnlEntry->u2ChannelBitmap);
			ucChnlLowerBound = (u4Bw == 40) ?
				(ucCenterChnl - 2) : (ucCenterChnl - 6);
			prChnlEntry->ucPrimaryChnlBitmap =
				(1 << ((ucPrimaryChnl - ucChnlLowerBound) / 4));
		} else if (u4Bw == 160) {
			ucCenterChnl = nanRegGetCenterChnlByPriChnl(
				ucOperatingClass, ucPrimaryChnl);
			nanRegGetChannelBitmap(ucOperatingClass, ucCenterChnl,
				       &prChnlEntry->u2ChannelBitmap);
			ucChnlLowerBound = ucCenterChnl - 14;
			prChnlEntry->ucPrimaryChnlBitmap =
			(1 << ((ucPrimaryChnl - ucChnlLowerBound) / 4));
		} else if (u4Bw == 320) {
			ucCenterChnl = nanRegGetCenterChnlByPriChnl(
				ucOperatingClass, ucPrimaryChnl);
			nanRegGetChannelBitmap(ucOperatingClass, ucCenterChnl,
				       &prChnlEntry->u2ChannelBitmap);
			ucChnlLowerBound = ucCenterChnl - 30;
			prChnlEntry->ucPrimaryChnlBitmap =
			(1 << ((ucPrimaryChnl - ucChnlLowerBound) / 4));
		} else {
			/* to be add */
		}
		return WLAN_STATUS_SUCCESS;
	}
#endif

	if (u4Bw == 20 || u4Bw == 40) {
		prChnlEntry->ucPrimaryChnlBitmap = 0;
		prChnlEntry->u2AuxChannelBitmap = 0;
		nanRegGetChannelBitmap(ucOperatingClass, ucPrimaryChnl,
				       &prChnlEntry->u2ChannelBitmap);
	} else if (u4Bw == 80) {
		if (ucPrimaryChnl >= 36 && ucPrimaryChnl <= 48) {
			ucChnlLowerBound = 36;
			ucCenterChnl = 42;
		} else if (ucPrimaryChnl >= 52 && ucPrimaryChnl <= 64) {
			ucChnlLowerBound = 52;
			ucCenterChnl = 58;
		} else if (ucPrimaryChnl >= 100 && ucPrimaryChnl <= 112) {
			ucChnlLowerBound = 100;
			ucCenterChnl = 106;
		} else if (ucPrimaryChnl >= 116 && ucPrimaryChnl <= 128) {
			ucChnlLowerBound = 116;
			ucCenterChnl = 122;
		} else if (ucPrimaryChnl >= 132 && ucPrimaryChnl <= 144) {
			ucChnlLowerBound = 132;
			ucCenterChnl = 138;
		} else if (ucPrimaryChnl >= 149 && ucPrimaryChnl <= 161) {
			ucChnlLowerBound = 149;
			ucCenterChnl = 155;
		}

		if (ucChnlLowerBound == 0) {
			DBGLOG(NAN, ERROR, "Illegal channel number:%d\n",
				ucPrimaryChnl);
			return WLAN_STATUS_FAILURE;
		}

		prChnlEntry->ucPrimaryChnlBitmap =
			(1 << ((ucPrimaryChnl - ucChnlLowerBound) / 4));
		nanRegGetChannelBitmap(ucOperatingClass, ucCenterChnl,
				       &prChnlEntry->u2ChannelBitmap);

		if (prChnlCtrl->u4AuxCenterChnl == 0) {
			prChnlEntry->u2AuxChannelBitmap = 0;
		} else {
			nanRegGetChannelBitmap(ucOperatingClass,
				prChnlCtrl->u4AuxCenterChnl,
				&prChnlEntry->u2AuxChannelBitmap);
		}
	} else if (u4Bw == 160) {
		if (ucPrimaryChnl >= 36 && ucPrimaryChnl <= 64) {
			ucChnlLowerBound = 36;
			ucCenterChnl = 50;
		} else if (ucPrimaryChnl >= 100 && ucPrimaryChnl <= 128) {
			ucChnlLowerBound = 100;
			ucCenterChnl = 114;
		}

		if (ucChnlLowerBound == 0) {
			DBGLOG(NAN, ERROR, "Illegal channel number:%d\n",
				ucPrimaryChnl);
			return WLAN_STATUS_FAILURE;
		}

		prChnlEntry->ucPrimaryChnlBitmap =
			(1 << ((ucPrimaryChnl - ucChnlLowerBound) / 4));
		prChnlEntry->u2AuxChannelBitmap = 0;
		nanRegGetChannelBitmap(ucOperatingClass, ucCenterChnl,
				       &prChnlEntry->u2ChannelBitmap);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t nanParserGenBandChnlEntryListField(struct ADAPTER *prAdapter,
			union _NAN_BAND_CHNL_CTRL *prBandChnlListCtrl,
			uint32_t u4NumOfList,
			uint8_t *pucBandChnlEntryListField,
			uint32_t *pu4BandChnlEntryListFieldLength)
{
	struct _NAN_BAND_CHNL_LIST_T *prBandChnlList;
	struct _NAN_CHNL_ENTRY_T *prChnlEntry;
	uint8_t *pucPos;
	uint32_t u4BandIdMask;

	pucPos = pucBandChnlEntryListField;

	if (prBandChnlListCtrl->u4Type == NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {

		if (u4NumOfList != 1) {
			DBGLOG(NAN, ERROR, "u4NumOfList doesn't equal 1!\n");
			return WLAN_STATUS_FAILURE;
		}

		prBandChnlList = (struct _NAN_BAND_CHNL_LIST_T *)pucPos;
		prBandChnlList->ucNonContiguous = 0;
		prBandChnlList->ucType = NAN_BAND_CH_ENTRY_LIST_TYPE_BAND;
		prBandChnlList->ucRsvd = 0;
		prBandChnlList->ucNumberOfEntry = 0;
		pucPos += 1;

		u4BandIdMask = prBandChnlListCtrl->u4BandIdMask;
		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_2P4G)) {
			*pucPos = NAN_SUPPORTED_BAND_ID_2P4G;
			prBandChnlList->ucNumberOfEntry++;
			pucPos++;
		}
		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_5G)) {
			*pucPos = NAN_SUPPORTED_BAND_ID_5G;
			prBandChnlList->ucNumberOfEntry++;
			pucPos++;
		}
#if (CFG_SUPPORT_NAN_6G == 1)
		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_6G)) {
			*pucPos = NAN_SUPPORTED_BAND_ID_6G;
			prBandChnlList->ucNumberOfEntry++;
			pucPos++;
		}
#endif
		*pu4BandChnlEntryListFieldLength =
			(pucPos - pucBandChnlEntryListField);
	} else {
		prBandChnlList = (struct _NAN_BAND_CHNL_LIST_T *)pucPos;
		prBandChnlList->ucNonContiguous = 0; /* Fixme */
		prBandChnlList->ucType =
			NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
		prBandChnlList->ucRsvd = 0;
		prBandChnlList->ucNumberOfEntry = 0;
		pucPos += 1;

		for (; u4NumOfList > 0;
		     u4NumOfList--, prBandChnlListCtrl += 1) {
			if (prBandChnlListCtrl->u4Type !=
			    NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL)
				continue;

			prChnlEntry = (struct _NAN_CHNL_ENTRY_T *)pucPos;
			nanParserGenChnlEntryField(
				prAdapter, prBandChnlListCtrl, prChnlEntry);

			prBandChnlList->ucNumberOfEntry++;

			if (prBandChnlList->ucNonContiguous == 0) {
				pucPos += (sizeof(struct _NAN_CHNL_ENTRY_T) -
					   2 /* Aux channel bitmap */);
				DBGLOG(NAN, LOUD,
				"Local generated Channel Bitmap 0x%04x , Primary Channel bitmap 0x%02x ,\n",
				prChnlEntry->u2ChannelBitmap,
				prChnlEntry->ucPrimaryChnlBitmap);
			} else {
				pucPos += sizeof(struct _NAN_CHNL_ENTRY_T);
			}
		}

		*pu4BandChnlEntryListFieldLength =
			(pucPos - pucBandChnlEntryListField);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanParserInterpretTimeBitmapField(struct ADAPTER *prAdapter,
		  uint16_t u2TimeBitmapCtrl,
		  uint8_t ucTimeBitmapLength,
		  uint8_t *pucTimeBitmap, uint32_t *pu4AvailMap)
{
	uint32_t u4Duration;
	uint32_t u4Period;
	uint32_t u4StartOffset;
	uint32_t u4Repeat, u4Bit, u4Num;
	uint32_t u4BitPos;
	uint8_t ucLength;
	uint8_t *pucBitmap;
	uint32_t u4Run;

	u4Duration =
		NAN_GET_TIME_BITMAP_CTRL_DURATION_IN_SLOT(u2TimeBitmapCtrl);
	u4Period = NAN_GET_TIME_BITMAP_CTRL_PERIOD_IN_SLOT(u2TimeBitmapCtrl);
	u4StartOffset =
		NAN_GET_TIME_BITMAP_CTRL_STARTOFFSET_IN_SLOT(u2TimeBitmapCtrl);

	DBGLOG(NAN, LOUD,
	       "BitDur:%d, RepeatPeriod:%d, StartOffset:%d, len:%d\n",
	       u4Duration, u4Period, u4StartOffset, ucTimeBitmapLength);

	if (u4Period)
		u4Repeat = NAN_TOTAL_SLOT_WINDOWS / u4Period;
	else {
		DBGLOG(NAN, WARN,
		       "BitDur:%d, RepeatPeriod:%d, StartOffset:%d, len:%d\n",
		       u4Duration, u4Period, u4StartOffset, ucTimeBitmapLength);
		u4Repeat = 1;
	}
	u4Run = 0;

	do {
		u4BitPos = u4Run * u4Period + u4StartOffset;
		pucBitmap = pucTimeBitmap;
		ucLength = ucTimeBitmapLength;
		if (ucLength > (u4Period / CHAR_BIT))
			ucLength = u4Period / CHAR_BIT;

		do {
			for (u4Bit = 0; u4Bit < CHAR_BIT; u4Bit++) {
				if ((*pucBitmap & BIT(u4Bit)) == 0) {
					u4BitPos += u4Duration;
					continue;
				}

				/* (*pucBitmap & BIT(u4Bit) */
				for (u4Num = 0;
				     u4Num < u4Duration &&
				     u4BitPos < NAN_TOTAL_SLOT_WINDOWS;
				     u4Num++) {
					NAN_TIMELINE_SET(pu4AvailMap, u4BitPos);
					u4BitPos++;
				}
			}

			pucBitmap++;
			ucLength--;
		} while (ucLength);

		u4Run++;
	} while ((u4BitPos < NAN_TOTAL_SLOT_WINDOWS) && (u4Run < u4Repeat));

	return WLAN_STATUS_SUCCESS;
}

union _NAN_BAND_CHNL_CTRL
nanQueryPeerPotentialChnlInfoBySlot(
		struct ADAPTER *prAdapter, uint32_t u4SchIdx,
		uint32_t u4AvailDbIdx, uint16_t u2SlotIdx,
		uint32_t u4WantedBandMask,
		uint8_t ucWantedPriChannel, uint32_t u4PrefBandMask)
{
	uint32_t u4Idx;
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4ChnlIdx;
	union _NAN_BAND_CHNL_CTRL rSelChnl = g_rNullChnl;
	union _NAN_BAND_CHNL_CTRL *prCurrChnl;
	const size_t sz2gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_2G4);
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
#if (CFG_SUPPORT_NAN_6G == 1)
	uint32_t u4PrefChnlIdx;
#endif
	enum ENUM_BAND eBand, ePrefBand;
	union _NAN_BAND_CHNL_CTRL rPrefChnl = g_rNullChnl;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	if (prPeerSchDesc == NULL)
		return g_rNullChnl;

	prAvailabilityDB = &prPeerSchDesc->arAvailAttr[u4AvailDbIdx];
	NAN_DW_DBGLOG(NAN, LOUD, TRUE, u2SlotIdx,
		      "ucAvailabilityDbIdx:%d, MapID:%d\n",
		      u4AvailDbIdx, prAvailabilityDB->ucMapId);

	if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
		return g_rNullChnl;

	for (u4Idx = 0; (u4Idx < NAN_NUM_AVAIL_TIMELINE); u4Idx++) {
		prNanAvailEntry = &prAvailabilityDB->arAvailEntryList[u4Idx];

		if (prNanAvailEntry->fgActive == FALSE)
			continue;

		if (!NAN_IS_AVAIL_MAP_SET(prNanAvailEntry->au4AvailMap,
					  u2SlotIdx))
			continue;

		if (prNanAvailEntry->arBandChnlCtrl[0].u4Type ==
		    NAN_BAND_CH_ENTRY_LIST_TYPE_BAND)
			continue;

		if (!prNanAvailEntry->rEntryCtrl.b1Potential) {
			DBGLOG(NAN, LOUD,
			       "Entry:%d, Slot:%d, Type:%d not equal\n", u4Idx,
			       u2SlotIdx,
			       prNanAvailEntry->rEntryCtrl.b3Type);
			continue;
		}

		if (u4PrefBandMask == 0)
			u4PrefBandMask = NAN_PREFER_BAND_MASK_DFT;
		while (u4PrefBandMask) {
			switch ((u4PrefBandMask & 0xF)) {
			case NAN_PREFER_BAND_MASK_CUST_CH:
				/* reference preferred ch */
				if (u4WantedBandMask & BIT(BAND_5G))
					rPrefChnl =
					      g_rPreferredChnl[sz5gTimeLineIdx];
				else
					rPrefChnl =
					      g_rPreferredChnl[sz2gTimeLineIdx];
				ePrefBand =
					nanRegGetNanChnlBand(rPrefChnl);
				break;

			case NAN_PREFER_BAND_MASK_2G_DW_CH:
				/* reference 2G DW ch */
				rPrefChnl = g_r2gDwChnl;
				ePrefBand =
					nanRegGetNanChnlBand(rPrefChnl);
				break;

			case NAN_PREFER_BAND_MASK_5G_DW_CH:
				/* reference 5G DW ch */
				rPrefChnl = g_r5gDwChnl;
				ePrefBand =
					nanRegGetNanChnlBand(rPrefChnl);
				break;

			case NAN_PREFER_BAND_MASK_2G_BAND:
				/* reference 2G BAND */
				rPrefChnl = g_rNullChnl;
				ePrefBand = BAND_2G4;
				break;

			case NAN_PREFER_BAND_MASK_5G_BAND:
				/* reference 5G BAND */
				rPrefChnl = g_rNullChnl;
				ePrefBand = BAND_5G;
				break;

#if (CFG_SUPPORT_WIFI_6G == 1)
			case NAN_PREFER_BAND_MASK_6G_BAND:
				/* reference 6G BAND */
				rPrefChnl = g_rNullChnl;
				ePrefBand = BAND_6G;
				break;
#endif

			case NAN_PREFER_BAND_MASK_DFT:
			default:
				rPrefChnl = g_rNullChnl;
				ePrefBand = BAND_NULL;
				break;
			}
			u4PrefBandMask >>= 4;

			for (u4ChnlIdx = 0;
			     u4ChnlIdx < prNanAvailEntry->ucNumBandChnlCtrl;
			     u4ChnlIdx++) {
				prCurrChnl =
				&prNanAvailEntry->arBandChnlCtrl[u4ChnlIdx];

				eBand = nanRegGetNanChnlBand(*prCurrChnl);
				if ((u4WantedBandMask != 0) &&
					((u4WantedBandMask & BIT(eBand)) == 0))
					continue;
				if ((ucWantedPriChannel != 0) &&
					(prCurrChnl->u4PrimaryChnl !=
					ucWantedPriChannel))
					continue;

				if (!nanIsAllowedChannel(prAdapter,
					*prCurrChnl))
					continue;

				if (rSelChnl.u4PrimaryChnl == 0)
					rSelChnl = *prCurrChnl;

				if (ePrefBand == BAND_NULL)
					return *prCurrChnl;
				else if (eBand == ePrefBand) {
					if (rPrefChnl.u4PrimaryChnl == 0)
						return *prCurrChnl;
					if (prCurrChnl->u4PrimaryChnl ==
					    rPrefChnl.u4PrimaryChnl)
						return *prCurrChnl;

					rSelChnl = *prCurrChnl;
				}
			}
		}
#if (CFG_SUPPORT_NAN_6G == 1)
		/* Select potential channel by local prefrence priority */
		for (u4PrefChnlIdx = 0;
			u4PrefChnlIdx < NAN_MAX_PREFER_CHNL_SEL;
			u4PrefChnlIdx++) {
			switch (u4PrefChnlIdx) {
			case 0:
				rPrefChnl = g_rPreferredChnl[0];
				break;
			case 1:
				rPrefChnl = g_r6gDefChnl;
				break;
			case 2:
				rPrefChnl = g_r5gDwChnl;
				break;
			case 3:
				rPrefChnl = g_r2gDwChnl;
				break;
			default:
				break;
			}
			ePrefBand =
				nanRegGetNanChnlBand(rPrefChnl);

			prCurrChnl = &prNanAvailEntry->arBandChnlCtrl[0];
			for (u4ChnlIdx = 0;
			     u4ChnlIdx < prNanAvailEntry->ucNumBandChnlCtrl;
			     u4ChnlIdx++, prCurrChnl++) {
				if (nanIsAllowedChannel(
					    prAdapter,
					    *prCurrChnl)) {
					eBand =
					    nanRegGetNanChnlBand(*prCurrChnl);

					if (!rSelChnl.u4PrimaryChnl)
						rSelChnl = *prCurrChnl;

					if (eBand == ePrefBand &&
					    prCurrChnl->u4PrimaryChnl ==
					    rPrefChnl.u4PrimaryChnl)
						return *prCurrChnl;
				}
			}
		}
#else
		prCurrChnl = &prNanAvailEntry->arBandChnlCtrl[0];
		for (u4ChnlIdx = 0;
			 u4ChnlIdx < prNanAvailEntry->ucNumBandChnlCtrl;
			 u4ChnlIdx++) {
			if (nanIsAllowedChannel(
					prAdapter,
					*prCurrChnl)) {
				if (rSelChnl.u4PrimaryChnl == 0)
					rSelChnl = *prCurrChnl;

				if (prCurrChnl->u4PrimaryChnl < 36) {
					if (prCurrChnl->u4PrimaryChnl ==
					    g_r2gDwChnl.u4PrimaryChnl)
						return *prCurrChnl;
				} else {
					if (prCurrChnl->u4PrimaryChnl ==
					    g_r5gDwChnl.u4PrimaryChnl)
						return *prCurrChnl;
				}
			}
		}
#endif

	}

	return rSelChnl;
}

#ifdef NAN_UNUSED
union _NAN_BAND_CHNL_CTRL
nanGetPeerPotentialChnlInfoBySlot(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx, uint16_t u2SlotIdx)
{
	uint32_t u4AvailDbIdx;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;

	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
	     u4AvailDbIdx++) {
		rSelChnlInfo = nanQueryPeerPotentialChnlInfoBySlot(
			prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx);
		if (rSelChnlInfo.u4PrimaryChnl != 0)
			return rSelChnlInfo;
	}

	return g_rNullChnl;
}
#endif

union _NAN_BAND_CHNL_CTRL
nanQueryPeerPotentialBandInfoBySlot(struct ADAPTER *prAdapter,
		uint32_t u4SchIdx, uint32_t u4AvailDbIdx,
		uint16_t u2SlotIdx,
		uint32_t u4WantedBandMask)
{
	uint32_t u4Idx;
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4NanBandIdMask;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	if (prPeerSchDesc == NULL)
		return g_rNullChnl;

	prAvailabilityDB = &prPeerSchDesc->arAvailAttr[u4AvailDbIdx];
	DBGLOG(NAN, LOUD, "ucAvailabilityDbIdx:%d, MapID:%d\n", u4AvailDbIdx,
	       prAvailabilityDB->ucMapId);

	if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
		return g_rNullChnl;

	for (u4Idx = 0; (u4Idx < NAN_NUM_AVAIL_TIMELINE); u4Idx++) {
		prNanAvailEntry = &prAvailabilityDB->arAvailEntryList[u4Idx];

		if (prNanAvailEntry->fgActive == FALSE)
			continue;

		if (!NAN_IS_AVAIL_MAP_SET(prNanAvailEntry->au4AvailMap,
					  u2SlotIdx))
			continue;

		if (prNanAvailEntry->arBandChnlCtrl[0].u4Type ==
		    NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL)
			continue;

		u4NanBandIdMask =
			prNanAvailEntry->arBandChnlCtrl[0].u4BandIdMask;
		if (((u4WantedBandMask & BIT(BAND_2G4)) &&
			(u4NanBandIdMask & BIT(NAN_SUPPORTED_BAND_ID_2P4G))) ||
			((u4WantedBandMask & BIT(BAND_5G)) &&
			(u4NanBandIdMask & BIT(NAN_SUPPORTED_BAND_ID_5G))))
			return prNanAvailEntry->arBandChnlCtrl[0];
	}

	return g_rNullChnl;
}

#ifdef NAN_UNUSED
union _NAN_BAND_CHNL_CTRL
nanGetPeerPotentialBandInfoBySlot(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
				  uint16_t u2SlotIdx)
{
	uint32_t u4AvailDbIdx;
	union _NAN_BAND_CHNL_CTRL rSelBandInfo;

	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
	     u4AvailDbIdx++) {
		rSelBandInfo = nanQueryPeerPotentialBandInfoBySlot(
			prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx);
		if (rSelBandInfo.u4BandIdMask != 0)
			return rSelBandInfo;
	}

	return g_rNullChnl;
}
#endif

/**
 * nanQueryChnlInfoBySlot() - Get channel information by given time slot
 * @prAdapter: pointer to adapter
 * @u2SlotIdx: time slot to query the channel information
 * @ppau4AvailMap: optional return pointer to point to the time bitmap
 * @fgCommitOrCond: committed(1) or conditional(0) to determine the
 *		  ChnlTimelineList (arChnlList or arCondChnlList) to examine
 *
 * Return: If the slot is set, return the channel information associated to
 *	 the queried time slot; if the slot is not set, return empty record.
 *	 If a non-NULL ppau4AvailMap is passed in, the time bitmap of the
 *	 associated ChnlTimeline will be returned from the parameter.
 */
union _NAN_BAND_CHNL_CTRL
nanQueryChnlInfoBySlot(struct ADAPTER *prAdapter, uint16_t u2SlotIdx,
		       uint32_t **ppau4AvailMap, unsigned char fgCommitOrCond,
			   uint8_t ucTimeLineIdx)
{
	uint32_t u4Idx = 0;
	uint32_t u4Valid = 0;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList = NULL;
	struct _NAN_SCHEDULER_T *prScheduler = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;

	if (ppau4AvailMap != NULL)
		*ppau4AvailMap = NULL;

	prScheduler = nanGetScheduler(prAdapter);
	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, ucTimeLineIdx);

	if (fgCommitOrCond) {

		if (nanIsDiscWindow(prAdapter, u2SlotIdx, ucTimeLineIdx)) {
			if (NAN_IS_2G_TIMELINE(prAdapter, ucTimeLineIdx))
				return g_r2gDwChnl;
			if (NAN_IS_5G_TIMELINE(prAdapter, ucTimeLineIdx))
				return g_r5gDwChnl;
		}

		prChnlTimelineList = prNanTimelineMgmt->arChnlList;
	} else {
		if (prNanTimelineMgmt->fgChkCondAvailability == FALSE)
			return g_rNullChnl;

		prChnlTimelineList = prNanTimelineMgmt->arCondChnlList;
	}

	/* FAW channel list */
	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
		if (prChnlTimelineList[u4Idx].fgValid == FALSE)
			continue;

		u4Valid = NAN_IS_AVAIL_MAP_SET(
			prChnlTimelineList[u4Idx].au4AvailMap, u2SlotIdx);
		if (u4Valid) {
			if (ppau4AvailMap != NULL)
				*ppau4AvailMap = &prChnlTimelineList[u4Idx]
							  .au4AvailMap[0];

			return prChnlTimelineList[u4Idx].rChnlInfo;
		}
	}

	return g_rNullChnl;
}

uint8_t
nanQueryPrimaryChnlBySlot(struct ADAPTER *prAdapter, uint16_t u2SlotIdx,
			unsigned char fgCommitOrCond, uint8_t ucTimeLineIdx)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};

	rChnlInfo = nanQueryChnlInfoBySlot(prAdapter, u2SlotIdx, NULL,
					   fgCommitOrCond, ucTimeLineIdx);
	return rChnlInfo.u4PrimaryChnl;
}

union _NAN_BAND_CHNL_CTRL
nanGetChnlInfoBySlot(struct ADAPTER *prAdapter, uint16_t u2SlotIdx,
				uint8_t ucTimeLineIdx)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};

	rChnlInfo = nanQueryChnlInfoBySlot(prAdapter, u2SlotIdx, NULL,
		TRUE, ucTimeLineIdx);
	if (rChnlInfo.u4PrimaryChnl == 0)
		rChnlInfo = nanQueryChnlInfoBySlot(prAdapter, u2SlotIdx, NULL,
						   FALSE, ucTimeLineIdx);

	return rChnlInfo;
}

/* Get the committed channel info in any timeline of the given slot */
union _NAN_BAND_CHNL_CTRL
nanGetCommittedChnlInfoBySlot(struct ADAPTER *prAdapter, uint16_t u2SlotIdx)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};
	uint8_t ucTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	/* check 5G/6G timeline first */
	for (ucTimeLineIdx = szNanActiveTimelineNum; ucTimeLineIdx--; ) {
		rChnlInfo = nanQueryChnlInfoBySlot(prAdapter, u2SlotIdx, NULL,
						   TRUE, ucTimeLineIdx);
		if (rChnlInfo.u4PrimaryChnl)
			break;
	}

	return rChnlInfo;
}

uint32_t
nanGetPrimaryChnlBySlot(struct ADAPTER *prAdapter, uint16_t u2SlotIdx,
	size_t szTimeLineIdx)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo;

	rChnlInfo = nanGetChnlInfoBySlot(prAdapter, u2SlotIdx, szTimeLineIdx);

	return rChnlInfo.u4PrimaryChnl;
}

union _NAN_BAND_CHNL_CTRL
nanQueryPeerChnlInfoBySlot(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			   uint32_t u4AvailDbIdx, uint16_t u2SlotIdx,
			   unsigned char fgCommitOrCond)
{
	uint32_t u4Idx;
	uint32_t u4AvailType;
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	if (prPeerSchDesc == NULL)
		return g_rNullChnl;

	prAvailabilityDB = &prPeerSchDesc->arAvailAttr[u4AvailDbIdx];
	NAN_DW_DBGLOG(NAN, LOUD, TRUE, u2SlotIdx,
		      "ucAvailabilityDbIdx:%d, MapID:%d\n",
		      u4AvailDbIdx, prAvailabilityDB->ucMapId);

	if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
		return g_rNullChnl;

	for (u4Idx = 0; (u4Idx < NAN_NUM_AVAIL_TIMELINE); u4Idx++) {
		prNanAvailEntry = &prAvailabilityDB->arAvailEntryList[u4Idx];

		if (prNanAvailEntry->fgActive == FALSE)
			continue;

		if (!NAN_IS_AVAIL_MAP_SET(prNanAvailEntry->au4AvailMap,
					  u2SlotIdx))
			continue;

		if (fgCommitOrCond)
			u4AvailType = NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT;
		else
			u4AvailType = NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND;

		if ((prNanAvailEntry->rEntryCtrl.b3Type & u4AvailType) == 0) {
			NAN_DW_DBGLOG(NAN, LOUD, TRUE, u2SlotIdx,
				     "Entry:%d, Slot:%d, Type:%d not equal\n",
				     u4Idx, u2SlotIdx,
				     prNanAvailEntry->rEntryCtrl.b3Type);
			continue;
		}

		return prNanAvailEntry->arBandChnlCtrl[0];
	}

	return g_rNullChnl;
}

uint8_t
nanQueryPeerPrimaryChnlBySlot(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			      uint32_t u4AvailDbIdx, uint16_t u2SlotIdx,
			      unsigned char fgCommitOrCond)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo;

	rChnlInfo = nanQueryPeerChnlInfoBySlot(
		prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx, fgCommitOrCond);
	return rChnlInfo.u4PrimaryChnl;
}

union _NAN_BAND_CHNL_CTRL
nanGetPeerChnlInfoBySlot(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			 uint32_t u4AvailDbIdx, uint16_t u2SlotIdx,
			 unsigned char fgChkRmtCondSlot)
{
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	if (prPeerSchDesc == NULL)
		return g_rNullChnl;

	rRmtChnlInfo = g_rNullChnl;
	if (u4AvailDbIdx == NAN_NUM_AVAIL_DB) {
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByDesc(
				    prAdapter, prPeerSchDesc, u4AvailDbIdx) ==
			    FALSE)
				continue;

			rRmtChnlInfo = nanQueryPeerChnlInfoBySlot(
				prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx,
				TRUE);
			if ((rRmtChnlInfo.u4PrimaryChnl == 0) &&
			    fgChkRmtCondSlot) {
				rRmtChnlInfo = nanQueryPeerChnlInfoBySlot(
					prAdapter, u4SchIdx, u4AvailDbIdx,
					u2SlotIdx, FALSE);
			}
			if (rRmtChnlInfo.u4PrimaryChnl != 0)
				break;
		}
	} else {
		rRmtChnlInfo = nanQueryPeerChnlInfoBySlot(
			prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx, TRUE);
		if ((rRmtChnlInfo.u4PrimaryChnl == 0) &&
		    fgChkRmtCondSlot) {
			rRmtChnlInfo = nanQueryPeerChnlInfoBySlot(
				prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx,
				FALSE);
		}
	}

	return rRmtChnlInfo;
}

union _NAN_BAND_CHNL_CTRL
nanGetPeerTimelineChnlInfoBySlot(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			 uint32_t szTimeLineIdx, uint16_t u2SlotIdx,
			 unsigned char fgChkRmtCondSlot)
{
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = g_rNullChnl;
	uint32_t u4AvailDbIdx;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	if (prPeerSchDesc == NULL)
		return g_rNullChnl;

	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
	     u4AvailDbIdx++) {
		if (nanSchedPeerAvailabilityDbValidByDesc(prAdapter,
				prPeerSchDesc, u4AvailDbIdx) == FALSE)
			continue;

		rRmtChnlInfo = nanQueryPeerChnlInfoBySlot(prAdapter, u4SchIdx,
					u4AvailDbIdx, u2SlotIdx, TRUE);
		if (rRmtChnlInfo.u4PrimaryChnl == 0 && fgChkRmtCondSlot) {
			rRmtChnlInfo = nanQueryPeerChnlInfoBySlot(prAdapter,
						u4SchIdx, u4AvailDbIdx,
						u2SlotIdx, FALSE);
		}
		if (!NAN_IS_CHANNEL_MATCH_TIMELINE(prAdapter, rRmtChnlInfo,
						   szTimeLineIdx))
			rRmtChnlInfo = g_rNullChnl;

		if (rRmtChnlInfo.u4PrimaryChnl != 0)
			break;
	}

	return rRmtChnlInfo;
}

uint32_t
nanGetPeerPrimaryChnlBySlot(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			    uint32_t u4AvailDbIdx, uint16_t u2SlotIdx,
			    unsigned char fgChkRmtCondSlot)
{
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;

	rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
		prAdapter, u4SchIdx, u4AvailDbIdx, u2SlotIdx, fgChkRmtCondSlot);

	return rRmtChnlInfo.u4PrimaryChnl;
}

uint8_t
nanGetPeerMaxBw(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	uint8_t ucBw = 20;
	uint32_t u4Idx;
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4AvailDbIdx;
	uint32_t u4SlotIdx;
	uint32_t u4AvailType;
	union _NAN_BAND_CHNL_CTRL *prBandChnlCtrl;
	uint32_t u4OperatingClass;
	const uint32_t u4COMMIT_COND = NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT |
				       NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND;

	do {
		prPeerSchDesc =
			nanSchedSearchPeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			DBGLOG(NAN, DEBUG, "Cannot find peer schedule desc\n");
			break;
		}

		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			prAvailabilityDB =
				&prPeerSchDesc->arAvailAttr[u4AvailDbIdx];

			if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				for (u4Idx = 0;
				     (u4Idx < NAN_NUM_AVAIL_TIMELINE);
				     u4Idx++) {
					prNanAvailEntry =
						&prAvailabilityDB
							 ->arAvailEntryList
								 [u4Idx];

					if (prNanAvailEntry->fgActive == FALSE)
						continue;

					if (!NAN_IS_AVAIL_MAP_SET(
						    prNanAvailEntry
							    ->au4AvailMap,
						    u4SlotIdx))
						continue;

					prBandChnlCtrl =
					    &prNanAvailEntry->arBandChnlCtrl[0];

					u4OperatingClass =
					    prBandChnlCtrl->u4OperatingClass;
					u4AvailType = u4COMMIT_COND;
					if ((prNanAvailEntry->rEntryCtrl.b3Type
					     & u4AvailType) == 0)
						continue;

					if (nanRegGetBw(u4OperatingClass) >
					    ucBw) {
						ucBw = nanRegGetBw(
							u4OperatingClass);
						break;
					}
				}
			}
		}
	} while (FALSE);

	return ucBw;
}

uint8_t
nanGetPeerMinBw(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
		enum ENUM_BAND eBand)
{
	uint8_t ucBw =
		nanSchedConfigGetAllowedBw(
		prAdapter, eBand);
	uint32_t u4Idx;
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4AvailDbIdx;
	uint32_t u4SlotIdx;
	uint32_t u4AvailType;

	do {
		prPeerSchDesc =
			nanSchedSearchPeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			DBGLOG(NAN, DEBUG, "Cannot find peer schedule desc\n");
			break;
		}

		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			prAvailabilityDB =
				&prPeerSchDesc->arAvailAttr[u4AvailDbIdx];

			if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				for (u4Idx = 0;
				     (u4Idx < NAN_NUM_AVAIL_TIMELINE);
				     u4Idx++) {
					prNanAvailEntry =
					&prAvailabilityDB->arAvailEntryList
					    [u4Idx];

					if (prNanAvailEntry->fgActive == FALSE)
						continue;

					if (!NAN_IS_AVAIL_MAP_SET(
					    prNanAvailEntry->au4AvailMap,
					    u4SlotIdx))
						continue;

					u4AvailType =
				    (NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT |
				     NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND);
					if ((prNanAvailEntry->rEntryCtrl.b3Type
					     & u4AvailType) == 0)
						continue;

					if (nanRegGetNanChnlBand(
						prNanAvailEntry
						->arBandChnlCtrl[0]) !=
						eBand) {
						continue;
					}

					if (nanRegGetBw(
					    prNanAvailEntry->arBandChnlCtrl[0]
					    .u4OperatingClass) <
						ucBw) {
						ucBw = nanRegGetBw(
						prNanAvailEntry
						->arBandChnlCtrl[0]
						.u4OperatingClass);
						break;
					}
				}
			}
		}
	} while (FALSE);

	return ucBw;
}

uint32_t
nanGetPeerDevCapability(struct ADAPTER *prAdapter,
			enum _ENUM_NAN_DEVCAP_FIELD_T eField,
			uint8_t *pucNmiAddr, uint8_t ucMapID,
			uint32_t *pu4RetValue)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4AvailabilityDbIdx;
	struct _NAN_DEVICE_CAPABILITY_T *prNanDevCapability;

	do {
		prPeerSchDesc =
			nanSchedSearchPeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			DBGLOG(NAN, DEBUG, "Cannot find peer schedule desc\n");
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		if (ucMapID != NAN_INVALID_MAP_ID) {
			u4AvailabilityDbIdx = nanSchedPeerGetAvailabilityDbIdx(
				prAdapter, prPeerSchDesc, ucMapID);
			if (u4AvailabilityDbIdx == NAN_INVALID_AVAIL_DB_INDEX) {
				rRetStatus = WLAN_STATUS_FAILURE;
				break;
			}
		} else {
			u4AvailabilityDbIdx = NAN_NUM_AVAIL_DB;
		}

		prNanDevCapability =
			&prPeerSchDesc->arDevCapability[u4AvailabilityDbIdx];
		if (prNanDevCapability->fgValid == FALSE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		switch (eField) {
		case ENUM_NAN_DEVCAP_CAPABILITY_SET:
			if (pu4RetValue)
				*pu4RetValue =
					prNanDevCapability->ucCapabilitySet;
			break;

		case ENUM_NAN_DEVCAP_TX_ANT_NUM:
			if (pu4RetValue)
				*pu4RetValue = prNanDevCapability->ucNumTxAnt;
			break;

		case ENUM_NAN_DEVCAP_RX_ANT_NUM:
			if (pu4RetValue)
				*pu4RetValue = prNanDevCapability->ucNumRxAnt;
			break;

		default:
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}
	} while (FALSE);

	return rRetStatus;
}

unsigned char
nanGetFeaturePeerNDPE(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_DEVICE_CAPABILITY_T *prNanDevCapability;
	uint32_t u4Idx;

	do {
		prPeerSchDesc =
			nanSchedSearchPeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			DBGLOG(NAN, DEBUG, "Cannot find peer schedule desc\n");
			break;
		}

		for (u4Idx = 0; u4Idx < (NAN_NUM_AVAIL_DB + 1); u4Idx++) {
			prNanDevCapability =
				&prPeerSchDesc->arDevCapability[u4Idx];
			if (prNanDevCapability->fgValid == FALSE)
				continue;
			else if (
				(prNanDevCapability->ucCapabilitySet &
				 NAN_ATTR_DEVICE_CAPABILITY_CAP_SUPPORT_NDPE) !=
				0)
				return TRUE;
		}
	} while (FALSE);

	return FALSE;
}

unsigned char
nanGetFeatureNDPE(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.fgEnableNDPE;
}

uint32_t
nanSchedDbgDumpTimelineDb(struct ADAPTER *prAdapter, const char *pucFunction,
			  uint32_t u4Line)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx = 0;
	uint8_t ucTimeLineIdx = 0;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline = NULL;
	union _NAN_BAND_CHNL_CTRL *prChnlInfo;

	DBGLOG(NAN, DEBUG, "Dump timeline DB [%s:%d]\n", pucFunction, u4Line);
	for (ucTimeLineIdx = 0;
		ucTimeLineIdx < NAN_TIMELINE_MGMT_SIZE; ucTimeLineIdx++) {
		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, ucTimeLineIdx);

		for (u4Idx = 0;
			u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
			prChnlTimeline = &prNanTimelineMgmt->arChnlList[u4Idx];
			if (prChnlTimeline->fgValid == FALSE)
				continue;

			prChnlInfo = &prChnlTimeline->rChnlInfo;
			DBGLOG(NAN, INFO,
			    "[%u][%u] MapId: %u, Raw:0x%x, Commit Chnl:%d, Class:%d, Bw:%d Bitmap:%02x-%02x-%02x-%02x\n",
			    ucTimeLineIdx, u4Idx, prNanTimelineMgmt->ucMapId,
			    prChnlInfo->u4RawData,
			    prChnlInfo->u4PrimaryChnl,
			    prChnlInfo->u4OperatingClass,
			    nanRegGetBw(prChnlInfo->u4OperatingClass),
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[0],
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[1],
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[2],
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[3]);
			nanUtilDump(prAdapter, "Commit AvailMap",
				    (uint8_t *)prChnlTimeline->au4AvailMap,
				    sizeof(prChnlTimeline->au4AvailMap));
		}

		for (u4Idx = 0;
			u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
			if (prNanTimelineMgmt->fgChkCondAvailability == FALSE)
				break;

			prChnlTimeline =
				&prNanTimelineMgmt->arCondChnlList[u4Idx];
			if (prChnlTimeline->fgValid == FALSE)
				continue;

			prChnlInfo = &prChnlTimeline->rChnlInfo;
			DBGLOG(NAN, DEBUG,
			    "[%u][%u] MapId: %u, Raw:0x%x, Cond Chnl:%d, Class:%d, Bw:%d Bitmap:%02x-%02x-%02x-%02x\n",
			    ucTimeLineIdx, u4Idx, prNanTimelineMgmt->ucMapId,
			    prChnlInfo->u4RawData,
			    prChnlInfo->u4PrimaryChnl,
			    prChnlInfo->u4OperatingClass,
			    nanRegGetBw(prChnlInfo->u4OperatingClass),
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[0],
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[1],
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[2],
			    ((uint8_t *)prChnlTimeline->au4AvailMap)[3]);
			nanUtilDump(prAdapter, "Cond AvailMap",
				    (uint8_t *)prChnlTimeline->au4AvailMap,
				    sizeof(prChnlTimeline->au4AvailMap));
		}
	}

	return rRetStatus;
}

uint32_t nanSchedDbgDumpPeerAvailability(struct ADAPTER *prAdapter,
					 uint8_t *pucNmiAddr)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx, u4Idx1, u4Idx2;
	struct _NAN_AVAILABILITY_DB_T *prNanAvailAttr;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint16_t u2EntryControl;

	prPeerSchDesc = nanSchedSearchPeerSchDescByNmi(prAdapter, pucNmiAddr);
	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, DEBUG, "Cannot find peer schedule desc\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(NAN, DEBUG, "\n");
	DBGLOG(NAN, INFO, "Dump %02x:%02x:%02x:%02x:%02x:%02x Availability\n",
	       prPeerSchDesc->aucNmiAddr[0], prPeerSchDesc->aucNmiAddr[1],
	       prPeerSchDesc->aucNmiAddr[2], prPeerSchDesc->aucNmiAddr[3],
	       prPeerSchDesc->aucNmiAddr[4], prPeerSchDesc->aucNmiAddr[5]);

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prNanAvailAttr = &prPeerSchDesc->arAvailAttr[u4Idx];
		if (prNanAvailAttr->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		for (u4Idx1 = 0; u4Idx1 < NAN_NUM_AVAIL_TIMELINE; u4Idx1++) {
			prNanAvailEntry =
				&prNanAvailAttr->arAvailEntryList[u4Idx1];
			if (prNanAvailEntry->fgActive == FALSE)
				continue;

			if (prNanAvailEntry->arBandChnlCtrl[0].u4Type ==
			    NAN_BAND_CH_ENTRY_LIST_TYPE_BAND)
				continue;

			u2EntryControl = prNanAvailEntry->rEntryCtrl.u2RawData;
			DBGLOG(NAN, INFO,
			       "[%u][%u] MapID:%u, Ctrl:0x%x (Type:%u C:%u/p:%u/c:%u, Pref=%u, Util=%u, NSS=%u, TBITMAP=%u), ChnlRaw:0x%x, Class:%u, Bw:%u\n",
			       u4Idx, u4Idx1, prNanAvailAttr->ucMapId,
			       u2EntryControl,
			       NAN_AVAIL_ENTRY_CTRL_TYPE(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_POTENTIAL(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_P(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_U(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_NSS(u2EntryControl),
			       NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(u2EntryControl),
			       prNanAvailEntry->arBandChnlCtrl[0].u4RawData,
			       prNanAvailEntry->arBandChnlCtrl[0]
			       .u4OperatingClass,
			       nanRegGetBw(prNanAvailEntry->arBandChnlCtrl[0]
					   .u4OperatingClass));
			for (u4Idx2 = 0;
			     u4Idx2 < prNanAvailEntry->ucNumBandChnlCtrl;
			     u4Idx2++)
				DBGLOG(NAN, INFO,
				  "   [%d] PriChnl:%u Bitmap:%02x-%02x-%02x-%02x\n",
				  u4Idx2,
				  prNanAvailEntry->arBandChnlCtrl[u4Idx2]
				  .u4PrimaryChnl,
				  ((uint8_t *)prNanAvailEntry->au4AvailMap)[0],
				  ((uint8_t *)prNanAvailEntry->au4AvailMap)[1],
				  ((uint8_t *)prNanAvailEntry->au4AvailMap)[2],
				  ((uint8_t *)prNanAvailEntry->au4AvailMap)[3]);
			nanUtilDump(prAdapter, "[Map]",
				    (uint8_t *)prNanAvailEntry->au4AvailMap,
				    sizeof(prNanAvailEntry->au4AvailMap));
		}
	}

	return rRetStatus;
}

struct _NAN_NDC_CTRL_T *
nanSchedAcquireNdcCtrl(struct ADAPTER *prAdapter)
{
	uint32_t u4Idx;
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);

	for (u4Idx = 0; u4Idx < NAN_MAX_NDC_RECORD; u4Idx++) {
		if (prScheduler->arNdcCtrl[u4Idx].fgValid == FALSE)
			return &prScheduler->arNdcCtrl[u4Idx];
	}

	return NULL;
}

struct _NAN_NDC_CTRL_T *
nanSchedGetNdcCtrl(struct ADAPTER *prAdapter, uint8_t *pucNdcId)
{
	uint32_t u4Idx;
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);

	for (u4Idx = 0; u4Idx < NAN_MAX_NDC_RECORD; u4Idx++) {
		if (prScheduler->arNdcCtrl[u4Idx].fgValid == FALSE)
			continue;

		if ((pucNdcId == NULL) ||
		    (kalMemCmp(prScheduler->arNdcCtrl[u4Idx].aucNdcId, pucNdcId,
			       NAN_NDC_ATTRIBUTE_ID_LENGTH) == 0))
			return &prScheduler->arNdcCtrl[u4Idx];
	}

	return NULL;
}

enum _ENUM_CNM_CH_CONCURR_T
nanSchedChkConcurrOp(union _NAN_BAND_CHNL_CTRL rCurrChnlInfo,
		     union _NAN_BAND_CHNL_CTRL rNewChnlInfo)
{
	uint8_t ucPrimaryChNew;
	enum ENUM_CHANNEL_WIDTH eChannelWidthNew;
	enum ENUM_CHNL_EXT eSCONew;
	uint8_t ucChannelS1New;
	uint8_t ucChannelS2New;
	uint8_t ucPrimaryChCurr;
	enum ENUM_CHANNEL_WIDTH eChannelWidthCurr;
	enum ENUM_CHNL_EXT eSCOCurr;
	uint8_t ucChannelS1Curr;
	uint8_t ucChannelS2Curr;

#if (CFG_SUPPORT_NAN_6G == 1)
	if (nanRegGetNanChnlBand(rCurrChnlInfo) !=
		nanRegGetNanChnlBand(rNewChnlInfo))
		return CNM_CH_CONCURR_MCC;
#endif

	if (nanRegConvertNanChnlInfo(rCurrChnlInfo, &ucPrimaryChCurr,
				     &eChannelWidthCurr, &eSCOCurr,
				     &ucChannelS1Curr,
				     &ucChannelS2Curr) != WLAN_STATUS_SUCCESS)
		return CNM_CH_CONCURR_MCC;

	if (nanRegConvertNanChnlInfo(rNewChnlInfo, &ucPrimaryChNew,
				     &eChannelWidthNew, &eSCONew,
		    &ucChannelS1New, &ucChannelS2New) != WLAN_STATUS_SUCCESS)
		return CNM_CH_CONCURR_MCC;

	return nanChConCurrType(ucPrimaryChNew, eChannelWidthNew, eSCONew,
				ucChannelS1New, ucChannelS2New,
				ucPrimaryChCurr, eChannelWidthCurr, eSCOCurr,
				ucChannelS1Curr, ucChannelS2Curr);
}

/**
 * nanSchedAcquireChnlTimeline() - Find a valid Channel timeline compabile with
 *				 given prChnlInfo
 * @prAdapter: pointer to adapter
 * @prChnlTimelineList: channel timeline, could be committed or conditional
 * @prChnlInfo: expected channel info to match a timeline
 *
 * Return: Channel timeline matched by prChnlInfo, or NULL if no available found
 */
struct _NAN_CHANNEL_TIMELINE_T *
nanSchedAcquireChnlTimeline(struct ADAPTER *prAdapter,
			    struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList,
			    union _NAN_BAND_CHNL_CTRL *prChnlInfo)
{
	uint32_t u4Idx;
	uint32_t u4InvalidIdx = NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
	enum _ENUM_CNM_CH_CONCURR_T eConcurr;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline;

	prChnlTimeline = &prChnlTimelineList[0];
	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
	     u4Idx++, prChnlTimeline++) {
		if (prChnlTimeline->fgValid == FALSE) {
			if (u4InvalidIdx == NAN_TIMELINE_MGMT_CHNL_LIST_NUM)
				u4InvalidIdx = u4Idx;

			continue;
		}

		eConcurr = nanSchedChkConcurrOp(prChnlTimeline->rChnlInfo,
						*prChnlInfo);
		if (eConcurr == CNM_CH_CONCURR_MCC)
			continue;
		else if (eConcurr == CNM_CH_CONCURR_SCC_NEW)
			prChnlTimeline->rChnlInfo.u4RawData =
				prChnlInfo->u4RawData;

		return prChnlTimeline;
	}

	if (u4InvalidIdx != NAN_TIMELINE_MGMT_CHNL_LIST_NUM) {
		prChnlTimelineList[u4InvalidIdx].fgValid = TRUE;
		prChnlTimelineList[u4InvalidIdx].rChnlInfo.u4RawData =
			prChnlInfo->u4RawData;
		prChnlTimelineList[u4InvalidIdx].i4Num = 0;
		kalMemZero(
			prChnlTimelineList[u4InvalidIdx].au4AvailMap,
			sizeof(prChnlTimelineList[u4InvalidIdx].au4AvailMap));

		return &prChnlTimelineList[u4InvalidIdx];
	}

	return NULL;
}

union _NAN_BAND_CHNL_CTRL
nanSchedConvergeChnlInfo(struct ADAPTER *prAdapter,
			 union _NAN_BAND_CHNL_CTRL rChnlInfo)
{
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;
	uint32_t u4MaxAllowedBw;
	enum ENUM_BAND eBand;
	uint32_t u4ChnlBw = NAN_CHNL_BW_20;
	uint32_t u4MinBw = NAN_CHNL_BW_20;

	eBand = nanRegGetNanChnlBand(rChnlInfo);

	u4MaxAllowedBw = nanSchedConfigGetAllowedBw(
		prAdapter, eBand);
	u4ChnlBw = nanRegGetBw(rChnlInfo.u4OperatingClass);
	u4MinBw = kal_min_t(uint32_t, u4MaxAllowedBw, u4ChnlBw);

	if (u4ChnlBw != u4MaxAllowedBw) {
		rSelChnlInfo = nanRegGenNanChnlInfoByPriChannel(
			rChnlInfo.u4PrimaryChnl, u4MinBw,
			eBand);
	} else {
		rSelChnlInfo = rChnlInfo;
	}

	return rSelChnlInfo;
}

uint8_t nanSchedChooseBestFromChnlBitmap(struct ADAPTER *prAdapter,
					 uint8_t ucOperatingClass,
					 uint16_t *pu2ChnlBitmap,
					 unsigned char fgNonContBw,
					 uint8_t ucPriChnlBitmap,
					 uint8_t *pucTimeBitmap)
{
	uint8_t ucChnl;
	uint8_t ucFirstChnl;
	uint8_t ucPriChnl;
	union _NAN_BAND_CHNL_CTRL rNanChnlInfo;
	enum ENUM_BAND eBand;
	const uint16_t u2ChnlBitmap = *pu2ChnlBitmap;

	ucPriChnl = ucFirstChnl = 0;

	do {
		/* NOTE: data in pu2ChnlBitmap will be updated */
		ucChnl = nanRegGetPrimaryChannelByOrder(ucOperatingClass,
						pu2ChnlBitmap, fgNonContBw,
						ucPriChnlBitmap);
		if (ucChnl == REG_INVALID_INFO)
			break;

		DBGLOG(NAN, LOUD, "Return priChnl=%u, ChnlBitmap=0x%04x\n",
		       ucChnl, *pu2ChnlBitmap);

		if (ucFirstChnl == 0)
			ucFirstChnl = ucChnl;

		rNanChnlInfo.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
		rNanChnlInfo.u4OperatingClass = ucOperatingClass;
		rNanChnlInfo.u4PrimaryChnl = ucChnl;
		rNanChnlInfo.u4AuxCenterChnl = 0;

		if (!nanIsAllowedChannel(prAdapter, rNanChnlInfo))
			continue;

		eBand = nanRegGetNanChnlBand(rNanChnlInfo);

		if (ucPriChnl == 0)
			ucPriChnl = ucChnl;

		if (eBand == BAND_2G4) {
			if (ucChnl == g_r2gDwChnl.u4PrimaryChnl) {
				ucPriChnl = ucChnl;
				break;
			}
		} else if (eBand == BAND_5G) {
			if (ucChnl == g_r5gDwChnl.u4PrimaryChnl) {
				ucPriChnl = ucChnl;
				break;
			}
		} else {
#if (CFG_SUPPORT_NAN_6G == 1)
			if (ucChnl == g_r6gDefChnl.u4PrimaryChnl) {
				ucPriChnl = ucChnl;
				break;
			}
#endif
		}
	} while (TRUE);

	if (ucPriChnl == 0)
		ucPriChnl = ucFirstChnl;

	DBGLOG(NAN, DEBUG,
	       "OC=%u, ChnlBitmap=0x%04x, PriChnlBitmap=%u, fgNonContBw=%u, ucPriChnl=%u, bitmap=%02x-%02x-%02x-%02x\n",
	       ucOperatingClass, u2ChnlBitmap, ucPriChnlBitmap, fgNonContBw,
	       ucPriChnl,
	       pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);
	return ucPriChnl;
}

union _NAN_BAND_CHNL_CTRL nanSchedGetFixedChnlInfo(struct ADAPTER *prAdapter)
{
	uint32_t u4Bw;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = g_rNullChnl;

	if (prAdapter->rWifiVar.ucNanFixChnl != 0) {
		u4Bw = nanSchedConfigGetAllowedBw(prAdapter,
					prAdapter->rWifiVar.ucNanFixBand);
		rSelChnlInfo = nanRegGenNanChnlInfoByPriChannel(
					prAdapter->rWifiVar.ucNanFixChnl, u4Bw,
					prAdapter->rWifiVar.ucNanFixBand);
	}

	return rSelChnlInfo;
}

/**
 * nanGetChannelTimelineList() - Get the committed or conditional timeline list
 * @prAdapter: pointer to adapter
 * @fgCommitOrCond: committed(1) or conditional(0) to determine the
 *		  ChnlTimelineList (arChnlList or arCondChnlList) to return
 *
 * Return: The channel timeline list in the global Timeline management structure
 */
static struct _NAN_CHANNEL_TIMELINE_T *nanGetChannelTimelineList(
			struct ADAPTER *prAdapter, u_int8_t fgCommitOrCond,
			size_t szTimeLineIdx)
{
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList = NULL;

	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, szTimeLineIdx);
	if (fgCommitOrCond) {
		prChnlTimelineList = prNanTimelineMgmt->arChnlList;
	} else {
		if (prNanTimelineMgmt->fgChkCondAvailability)
			prChnlTimelineList = prNanTimelineMgmt->arCondChnlList;
	}

	return prChnlTimelineList;
}

uint32_t nanSchedMergeAvailabileChnlList(struct ADAPTER *prAdapter,
				unsigned char fgCommitOrCond)
{
	uint32_t u4Idx1 = 0, u4Idx2 = 0;
	uint32_t u4DwIdx = 0;
	uint8_t ucTimeLineIdx = 0;
	enum _ENUM_CNM_CH_CONCURR_T eConcurr = CNM_CH_CONCURR_MCC;
	union _NAN_BAND_CHNL_CTRL *prChnl1 = NULL, *prChnl2 = NULL;
	union _NAN_BAND_CHNL_CTRL rTargetChnl = {.u4RawData = 0};
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (ucTimeLineIdx = 0;
		ucTimeLineIdx < szNanActiveTimelineNum; ucTimeLineIdx++) {

		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, ucTimeLineIdx);

		if (fgCommitOrCond) {
			prChnlTimelineList = prNanTimelineMgmt->arChnlList;
		} else {
			if (prNanTimelineMgmt->fgChkCondAvailability == FALSE)
				return WLAN_STATUS_FAILURE;

			prChnlTimelineList = prNanTimelineMgmt->arCondChnlList;
		}

		for (u4Idx1 = 0;
			u4Idx1 < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx1++) {
			if (prChnlTimelineList[u4Idx1].fgValid == FALSE)
				continue;

			prChnl1 = &prChnlTimelineList[u4Idx1].rChnlInfo;

			for (u4Idx2 = u4Idx1 + 1;
			     u4Idx2 < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
			     u4Idx2++) {
				if (prChnlTimelineList[u4Idx2].fgValid == FALSE)
					continue;

				prChnl2 = &prChnlTimelineList[u4Idx2].rChnlInfo;

				eConcurr = nanSchedChkConcurrOp(*prChnl1,
							*prChnl2);
				if (eConcurr == CNM_CH_CONCURR_MCC)
					continue;
				else if (eConcurr == CNM_CH_CONCURR_SCC_NEW)
					rTargetChnl.u4RawData =
						prChnl2->u4RawData;
				else /* CNM_CH_CONCURR_SCC_CURR */
					rTargetChnl.u4RawData =
						prChnl1->u4RawData;

				prChnl1->u4RawData = rTargetChnl.u4RawData;
				for (u4DwIdx = 0;
					u4DwIdx < NAN_TOTAL_DW; u4DwIdx++)
					prChnlTimelineList[u4Idx1]
						.au4AvailMap[u4DwIdx] |=
						prChnlTimelineList[u4Idx2]
							.au4AvailMap[u4DwIdx];
				prChnlTimelineList[u4Idx1].i4Num =
				    nanUtilCheckBitOneCnt(
					prChnlTimelineList[u4Idx1].au4AvailMap,
					sizeof(prChnlTimelineList[u4Idx1]
						.au4AvailMap));
				prChnlTimelineList[u4Idx2].fgValid = FALSE;
			}
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/**
 * nanSchedAddCrbToChnlList() - Find a suitable timeline according to prChnlInfo
 *			      and fgCommitOrCond, with the timeline by
 *			      eRepeatPeriod, u4StartOffset, and u4NumSlots.
 * @prAdapter: pointer to adapter
 * @prChnlInfo: to locate a timeline with this given channel info, create new if
 *	      no existing match found
 * @u4StartOffset: start offset of CRB time slots
 * @u4NumSlots: consecutive slots to set
 * @eRepeatPeriod: the set bitmap is repeat for every 128~8192 TU, in enum
 * @fgCommitOrCond: committed(1) or conditional(0) to determine the
 *		  ChnlTimelineList (arChnlList or arCondChnlList) to be used
 * @au4RetMap: an optional return bitmap to hold 8192 TUs (16 TUs per slot),
 *	     if a buffer provided, a copy of timeline availability map will be
 *	     set accordingly.
 *
 * Return: WLAN_STATUS_FAILURE if no timeline available
 *	 WLAN_STATUS_SUCCESS with the timeline with requested time slots set
 */
uint32_t nanSchedAddCrbToChnlList(struct ADAPTER *prAdapter,
			 union _NAN_BAND_CHNL_CTRL *prChnlInfo,
			 uint32_t u4StartOffset, uint32_t u4NumSlots,
			 enum _ENUM_TIME_BITMAP_CTRL_PERIOD_T eRepeatPeriod,
			 unsigned char fgCommitOrCond,
			 uint32_t au4RetMap[NAN_TOTAL_DW])
{
	uint32_t u4Run = 0, u4Idx = 0, u4SlotIdx = 0;
	unsigned char fgChanged = FALSE;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_SCHEDULER_T *prNanScheduler = NULL;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList = NULL;
	struct _NAN_CHANNEL_TIMELINE_T *prTargetChnlTimeline = NULL;
	enum ENUM_BAND eBand = BAND_NULL;

	eBand = nanRegGetNanChnlBand(*prChnlInfo);
	prNanTimelineMgmt = nanGetTimelineMgmtByBand(prAdapter, eBand);

	if (fgCommitOrCond) {
		prChnlTimelineList = prNanTimelineMgmt->arChnlList;
	} else {
		if (prNanTimelineMgmt->fgChkCondAvailability == FALSE)
			return WLAN_STATUS_FAILURE;

		prChnlTimelineList = prNanTimelineMgmt->arCondChnlList;
	}

	prTargetChnlTimeline = nanSchedAcquireChnlTimeline(prAdapter,
					prChnlTimelineList, prChnlInfo);
	if (prTargetChnlTimeline == NULL)
		return WLAN_STATUS_FAILURE;

	for (u4Run = 0; u4Run < (128 >> eRepeatPeriod); u4Run++) {
		for (u4Idx = 0; u4Idx < u4NumSlots; u4Idx++) {
			u4SlotIdx = (u4Run << (eRepeatPeriod + 2)) +
				    u4StartOffset + u4Idx;

			if (NAN_IS_AVAIL_MAP_SET(
				    prTargetChnlTimeline->au4AvailMap,
				    u4SlotIdx) == FALSE) {
				prTargetChnlTimeline->i4Num++;
				NAN_TIMELINE_SET(
					prTargetChnlTimeline->au4AvailMap,
					u4SlotIdx);
				fgChanged = TRUE;
			}

			if (au4RetMap)
				NAN_TIMELINE_SET(au4RetMap, u4SlotIdx);
		}
	}

	prNanScheduler = nanGetScheduler(prAdapter);
	if (fgChanged || !fgCommitOrCond) {
		prNanScheduler->u2NanAvailAttrControlField |=
			NAN_AVAIL_CTRL_COMMIT_CHANGED;
	}

	return WLAN_STATUS_SUCCESS;
}

/**
 * Filling Channel info for Ranging by
 * nanSchedNegoGenDefCrbV2()->nanSchedNegoFindSlotCrb()
 */
uint32_t nanSchedAddCrbToRangingChnlList(struct ADAPTER *prAdapter,
			 union _NAN_BAND_CHNL_CTRL *prChnlInfo,
			 uint32_t u4StartOffset, uint32_t u4NumSlots,
			 enum _ENUM_TIME_BITMAP_CTRL_PERIOD_T eRepeatPeriod,
			 unsigned char fgCommitOrCond,
			 uint32_t au4RetMap[NAN_TOTAL_DW])
{
	struct _NAN_CRB_NEGO_CTRL_T * const prNegoCtrl =
			nanGetNegoControlBlock(prAdapter);
	uint32_t u4Run = 0, u4Idx = 0, u4SlotIdx = 0;
	unsigned char fgChanged = FALSE;
	size_t szTimeLineIdx =
			nanGetTimelineMgmtIndexByBand(prAdapter, BAND_5G);
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_SCHEDULE_TIMELINE_T *prSchedTimeline;

	if (IS_2G_OP_CLASS(prChnlInfo->u4OperatingClass)) {
		szTimeLineIdx =
			nanGetTimelineMgmtIndexByBand(prAdapter, BAND_2G4);
	}

	prSchedTimeline = &prNegoCtrl->arRangingTimeline[szTimeLineIdx];
	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

	if (prSchedTimeline->ucMapId == NAN_INVALID_MAP_ID) {
		prSchedTimeline->ucMapId = prNanTimelineMgmt->ucMapId;
		kalMemZero(prSchedTimeline->au4AvailMap,
			   sizeof(prSchedTimeline->au4AvailMap));
	}

	for (u4Run = 0; u4Run < (128 >> eRepeatPeriod); u4Run++) {
		for (u4Idx = 0; u4Idx < u4NumSlots; u4Idx++) {
			u4SlotIdx = (u4Run << (eRepeatPeriod + 2)) +
				    u4StartOffset + u4Idx;

			if (NAN_IS_AVAIL_MAP_SET(prSchedTimeline->au4AvailMap,
						 u4SlotIdx) == FALSE) {
				NAN_TIMELINE_SET(prSchedTimeline->au4AvailMap,
						 u4SlotIdx);
				fgChanged = TRUE;
			}

			if (au4RetMap)
				NAN_TIMELINE_SET(au4RetMap, u4SlotIdx);
		}
	}

	if (fgChanged || !fgCommitOrCond) {
		prNanScheduler->u2NanAvailAttrControlField |=
			NAN_AVAIL_CTRL_COMMIT_CHANGED;
	}

	return WLAN_STATUS_SUCCESS;
}

/**
 * nanSchedDeleteCrbFromChnlList() - Remove specified time slots in all valid
 *				   channel timelines
 * @prAdapter: pointer to adapter
 * @u4StartOffset: describe the start offset of the time slots to be cleared
 * @u4NumSlots: consecutive slots to clear
 * @eRepeatPeriod: the cleared bitmap is repeat for every 128~8192 TU, in enum
 * @fgCommitOrCond: committed(1) or conditional(0) to determine the
 *		  ChnlTimelineList (arChnlList or arCondChnlList) to be used
 *
 * Return: WLAN_STATUS_FAILURE if no timeline available
 *         WLAN_STATUS_SUCCESS if the timeline with requested time slots set
 */
uint32_t nanSchedDeleteCrbFromChnlList(struct ADAPTER *prAdapter,
		uint32_t u4StartOffset, uint32_t u4NumSlots,
		enum _ENUM_TIME_BITMAP_CTRL_PERIOD_T eRepeatPeriod,
		unsigned char fgCommitOrCond,
		uint8_t ucTimeLineIdx)
{
	uint32_t u4ChnlIdx = 0, u4Run = 0, u4Idx = 0, u4SlotIdx = 0;
	unsigned char fgChanged = FALSE;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_SCHEDULER_T *prNanScheduler = NULL;

	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, ucTimeLineIdx);
	prNanScheduler = nanGetScheduler(prAdapter);

	if (fgCommitOrCond) {
		prChnlTimelineList = prNanTimelineMgmt->arChnlList;
	} else {
		if (prNanTimelineMgmt->fgChkCondAvailability == FALSE)
			return WLAN_STATUS_FAILURE;

		prChnlTimelineList = prNanTimelineMgmt->arCondChnlList;
	}

	for (u4ChnlIdx = 0; u4ChnlIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
	     u4ChnlIdx++) {
		if (prChnlTimelineList[u4ChnlIdx].fgValid == FALSE)
			continue;

		for (u4Run = 0; u4Run < (128 >> eRepeatPeriod); u4Run++) {
			for (u4Idx = 0; u4Idx < u4NumSlots; u4Idx++) {
				u4SlotIdx = (u4Run << (eRepeatPeriod + 2)) +
					    u4StartOffset;

				if (NAN_IS_AVAIL_MAP_SET(
					    prChnlTimelineList[u4ChnlIdx]
						    .au4AvailMap,
					    u4SlotIdx) == TRUE) {
					prChnlTimelineList[u4ChnlIdx].i4Num--;
					if (prChnlTimelineList[u4ChnlIdx]
						    .i4Num <= 0)
						prChnlTimelineList[u4ChnlIdx]
							.fgValid = FALSE;

					NAN_TIMELINE_UNSET(
						prChnlTimelineList[u4ChnlIdx]
							.au4AvailMap,
						u4SlotIdx);
					fgChanged = TRUE;
				}
			}
		}

		if (fgChanged && !fgCommitOrCond)
			prNanScheduler->u2NanAvailAttrControlField |=
				NAN_AVAIL_CTRL_COMMIT_CHANGED;
	}

	return WLAN_STATUS_SUCCESS;
}

void
nanSchedPeerPurgeOldCrb(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord = NULL;
	uint32_t u4Idx = 0;
	uint8_t ucTimeLineIdx = 0;
	struct _NAN_SCHEDULE_TIMELINE_T *prCommonFawTimeline;
	struct _NAN_SCHEDULE_TIMELINE_T *prCommonImmuNdlTimeline;
	struct _NAN_SCHEDULE_TIMELINE_T *prCommRangingTimeline;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_SCHEDULE_TIMELINE_T *prFawTimeline = NULL;
	struct _NAN_SCHEDULE_TIMELINE_T *prImmuTimeline = NULL;
	struct _NAN_SCHEDULE_TIMELINE_T *prRangingTimeline = NULL;

	do {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
		if (prPeerSchRecord == NULL)
			break;

		/* reserve necessary CRB */
		prCommonFawTimeline = prPeerSchRecord->arCommFawTimeline;
		prCommonImmuNdlTimeline =
			prPeerSchRecord->arCommImmuNdlTimeline;
		prCommRangingTimeline = prPeerSchRecord->arCommRangingTimeline;
		for (ucTimeLineIdx = 0; ucTimeLineIdx < szNanActiveTimelineNum;
		     ucTimeLineIdx++) {
			prFawTimeline = &prCommonFawTimeline[ucTimeLineIdx];
			prImmuTimeline =
				&prCommonImmuNdlTimeline[ucTimeLineIdx];
			prRangingTimeline =
				&prCommRangingTimeline[ucTimeLineIdx];

			kalMemZero((uint8_t *)prFawTimeline->au4AvailMap,
				   sizeof(prFawTimeline->au4AvailMap));
			if (prPeerSchRecord->fgUseDataPath == TRUE) {
				for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++)
					prFawTimeline->au4AvailMap[u4Idx] |=
					prImmuTimeline->au4AvailMap[u4Idx];
				prPeerSchRecord->prCommNdcCtrl = NULL;
			}
			if (prPeerSchRecord->fgUseRanging == TRUE) {
				for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++)
					prFawTimeline->au4AvailMap[u4Idx] |=
					prRangingTimeline->au4AvailMap[u4Idx];
			}
		}
	} while (FALSE);

	nanSchedReleaseUnusedCommitSlot(prAdapter);
}

void
nanSchedPeerDataPathConflict(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{}

void
nanSchedPeerRangingConflict(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	nanRangingScheduleViolation(prAdapter, pucNmiAddr);
}

uint8_t nanPeerToLocalMapId(struct ADAPTER *prAdapter,
			    struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
			    struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntry)
{
	size_t szTimeLineIdx;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_AVAILABILITY_DB_T *prAvailabilityDB;
	uint8_t ucMapID = prScheduleEntry->ucMapID;
	uint8_t *pucTimeBitmap = prScheduleEntry->aucTimeBitmap;
	uint32_t ucTimeBitmapLength = prScheduleEntry->ucTimeBitmapLength;
	union _NAN_BAND_CHNL_CTRL rChnlInfo = g_rNullChnl;
	enum ENUM_BAND eBand;
	struct _NAN_AVAILABILITY_TIMELINE_T *prAvailEntry;
	size_t i;
	size_t j;

	if (ucTimeBitmapLength < sizeof(uint32_t)) {
		DBGLOG(NAN, ERROR, "ucTimeBitmapLength=%u too small, return %u",
		       ucTimeBitmapLength, ucMapID);
		return ucMapID;
	}

	for (i = 0; i < ARRAY_SIZE(prPeerSchDesc->arAvailAttr); i++) {
		prAvailabilityDB = &prPeerSchDesc->arAvailAttr[i];
		if (prAvailabilityDB->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		if (prAvailabilityDB->ucMapId != ucMapID)
			continue;
		for (j = 0; j < ARRAY_SIZE(prAvailabilityDB->arAvailEntryList);
		     j++) {
			prAvailEntry = &prAvailabilityDB->arAvailEntryList[j];
			if (!prAvailEntry->fgActive)
				continue;

			if (!prAvailEntry->rEntryCtrl.b1Committed &&
			    !prAvailEntry->rEntryCtrl.b1Conditional)
				continue;

			if ((prAvailEntry->au4AvailMap[0] &
			     *(uint32_t *)pucTimeBitmap) == 0)
				continue;

			rChnlInfo = prAvailEntry->arBandChnlCtrl[0];
			goto found;
		}
	}

found:
	if (rChnlInfo.u4PrimaryChnl == 0) {
		DBGLOG(NAN, INFO,
		       "No peer availability found for map ID %u 0x%08x",
		       ucMapID, *(uint32_t *)pucTimeBitmap);
		return ucMapID;
	}

	eBand = nanRegGetNanChnlBand(rChnlInfo);
	szTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter, eBand);
	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

	DBGLOG(NAN, INFO,
	       "Peer Avail Map ID %u = local ID %u, bitmap=0x%08x ch=%u",
	       ucMapID, prNanTimelineMgmt->ucMapId,
	       *(uint32_t *)pucTimeBitmap,
	       rChnlInfo.u4PrimaryChnl);

	return prNanTimelineMgmt->ucMapId;
}

/**
 * dst: arTimeline
 * src: prScheduleEntryList
 *
 * Callers: Immutable, NDC, and Ranging
 */
uint32_t
nanSchedPeerInterpretScheduleEntryList(struct ADAPTER *prAdapter,
		struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
		struct _NAN_SCHEDULE_TIMELINE_T arTimeline[NAN_NUM_AVAIL_DB],
		struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntryList,
		uint32_t u4ScheduleEntryListLength)
{
	uint32_t rRetStatus = WLAN_STATUS_FAILURE;
	uint8_t *pucScheduleEntry;
	uint8_t *pucScheduleEntryListEnd;
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntry;
	uint32_t szTimeLineIdx;
	uint32_t u4TargetIdx;

	for (szTimeLineIdx = 0; szTimeLineIdx < NAN_NUM_AVAIL_DB;
	     szTimeLineIdx++) {
		arTimeline[szTimeLineIdx].ucMapId = NAN_INVALID_MAP_ID;
		arTimeline[szTimeLineIdx].ucLocalMapId = NAN_INVALID_MAP_ID;
		kalMemZero(arTimeline[szTimeLineIdx].au4AvailMap,
			   sizeof(arTimeline[szTimeLineIdx].au4AvailMap));
	}

	pucScheduleEntry = (uint8_t *)prScheduleEntryList;
	pucScheduleEntryListEnd = pucScheduleEntry + u4ScheduleEntryListLength;
	for (prScheduleEntry =
		      (struct _NAN_SCHEDULE_ENTRY_T *)pucScheduleEntry;
	     pucScheduleEntry < pucScheduleEntryListEnd;
	     pucScheduleEntry += (prScheduleEntry->ucTimeBitmapLength + 4
	       /* MapID(1)+TimeBitmapControl(2)+TimeBitmapLength(1) */)) {

		prScheduleEntry =
			(struct _NAN_SCHEDULE_ENTRY_T *)pucScheduleEntry;

		/* TODO: nanSchedNegoAllocNdcCtrl */

		u4TargetIdx = NAN_NUM_AVAIL_DB;
		for (szTimeLineIdx = 0; szTimeLineIdx < NAN_NUM_AVAIL_DB;
		     szTimeLineIdx++) {
			if (arTimeline[szTimeLineIdx].ucMapId ==
			    prScheduleEntry->ucMapID) { /* matched entry */
				u4TargetIdx = szTimeLineIdx;
				break;
			}
			if (u4TargetIdx == NAN_NUM_AVAIL_DB &&
			    arTimeline[szTimeLineIdx].ucMapId ==
					    NAN_INVALID_MAP_ID)
				u4TargetIdx = szTimeLineIdx; /* first empty */
		}

		if (u4TargetIdx >= NAN_NUM_AVAIL_DB)
			break;

		/* ucMapId will be used later in nanSchedNegoIsRmtCrbConflict */
		arTimeline[u4TargetIdx].ucMapId = prScheduleEntry->ucMapID;
		arTimeline[u4TargetIdx].ucLocalMapId =
				nanPeerToLocalMapId(prAdapter, prPeerSchDesc,
						    prScheduleEntry);

		nanParserInterpretTimeBitmapField(prAdapter,
					prScheduleEntry->u2TimeBitmapControl,
					prScheduleEntry->ucTimeBitmapLength,
					prScheduleEntry->aucTimeBitmap,
					arTimeline[u4TargetIdx].au4AvailMap);

		DBGLOG(NAN, INFO,
		       "Set timeline[%u] ucMapId=%u, LocalMapId=%u, %02x-%02x-%02x-%02x",
		       u4TargetIdx,
		       prScheduleEntry->ucMapID,
		       arTimeline[u4TargetIdx].ucLocalMapId,
		       arTimeline[u4TargetIdx].aucAvailBlock[0],
		       arTimeline[u4TargetIdx].aucAvailBlock[1],
		       arTimeline[u4TargetIdx].aucAvailBlock[2],
		       arTimeline[u4TargetIdx].aucAvailBlock[3]);

		rRetStatus = WLAN_STATUS_SUCCESS;
	}

	return rRetStatus;
}

uint32_t
nanSchedPeerChkQos(struct ADAPTER *prAdapter,
		   struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec = NULL;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4DwIdx = 0;
	uint32_t u4SchIdx = 0;
	uint32_t u4QosMinSlots = 0;
	uint32_t u4QosMaxLatency = 0;
	uint32_t u4EmptySlots = 0;
	uint32_t u4BitCount;
	uint32_t i4Latency = 0;
	uint32_t u4Idx1 = 0;
	uint8_t ucTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	if (!prPeerSchDesc->fgUsed)
		return WLAN_STATUS_FAILURE;

	u4SchIdx = prPeerSchDesc->u4SchIdx;
	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRec == NULL) {
		DBGLOG(NAN, ERROR, "No peer sch record\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prPeerSchRec->fgUseDataPath == FALSE)
		return WLAN_STATUS_FAILURE;

	for (ucTimeLineIdx = 0;
		ucTimeLineIdx < szNanActiveTimelineNum; ucTimeLineIdx++) {

		prTimeline = &prPeerSchRec->arCommFawTimeline[ucTimeLineIdx];

		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
			u4QosMinSlots = prPeerSchRec->u4FinalQosMinSlots;
			if (u4QosMinSlots > NAN_INVALID_QOS_MIN_SLOTS) {
				u4BitCount = nanUtilCheckBitOneCnt(
					&prTimeline->au4AvailMap[u4DwIdx],
					sizeof(uint32_t));
				if (u4BitCount < u4QosMinSlots) {
					/* Qos min slot validation fail */
					return WLAN_STATUS_FAILURE;
				}
			}

			u4QosMaxLatency = prPeerSchRec->u4FinalQosMaxLatency;
			if (u4QosMaxLatency < NAN_INVALID_QOS_MAX_LATENCY) {
				if (u4QosMaxLatency == 0)
					u4QosMaxLatency =
					1; /* reserve 1 slot for DW window */

				u4EmptySlots =
					~(prTimeline->au4AvailMap[u4DwIdx]);
				i4Latency = 0;

				for (u4Idx1 = 0; u4Idx1 < 32; u4Idx1++) {
					if (u4EmptySlots & BIT(u4Idx1))
						i4Latency++;
					else
						i4Latency = 0;

					/* Qos max latency
					 * validation fail
					 */
					if (i4Latency > u4QosMaxLatency)
						return WLAN_STATUS_FAILURE;
				}
			}
		}
	}

	return rRetStatus;
}

uint32_t
nanSchedPeerChkTimeline(struct ADAPTER *prAdapter,
			struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
			struct _NAN_SCHEDULE_TIMELINE_T
				arTimeline[NAN_TIMELINE_MGMT_SIZE])
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec = NULL;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {0};
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {0};
	uint32_t u4SlotIdx = 0;
	uint32_t u4AvailDbIdx = 0;
	uint32_t u4SchIdx = 0;
	unsigned char fgCheckOk = 0;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline = NULL;
	uint8_t ucTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);


	if (!prPeerSchDesc->fgUsed)
		return WLAN_STATUS_FAILURE;

	u4SchIdx = prPeerSchDesc->u4SchIdx;
	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRec == NULL) {
		DBGLOG(NAN, ERROR, "No peer sch record\n");
		return WLAN_STATUS_FAILURE;
	}

	for (ucTimeLineIdx = 0;
		ucTimeLineIdx < szNanActiveTimelineNum; ucTimeLineIdx++) {

		prTimeline = &arTimeline[ucTimeLineIdx];

		for (u4SlotIdx = 0;
			u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
			if (!NAN_IS_AVAIL_MAP_SET(prTimeline->au4AvailMap,
				u4SlotIdx))
				continue;

			/* check local availability window */
			rLocalChnlInfo =
				nanQueryChnlInfoBySlot(prAdapter, u4SlotIdx,
					NULL, TRUE, ucTimeLineIdx);
			if (rLocalChnlInfo.u4PrimaryChnl == 0)
				return WLAN_STATUS_FAILURE;

			/* check peer's availability window */
			fgCheckOk = FALSE;

			for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
			     u4AvailDbIdx++) {
				if (nanSchedPeerAvailabilityDbValidByID(
					prAdapter, u4SchIdx, u4AvailDbIdx)
					== FALSE)
					continue;

				rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
					prAdapter, u4SchIdx, u4AvailDbIdx,
					u4SlotIdx, FALSE);
				if (rRmtChnlInfo.u4PrimaryChnl == 0)
					continue;

				if (nanSchedChkConcurrOp(rLocalChnlInfo,
							 rRmtChnlInfo) !=
				    CNM_CH_CONCURR_MCC) {

					fgCheckOk = TRUE;
					break;
				}
			}

			if (!fgCheckOk)
				return WLAN_STATUS_FAILURE;
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedPeerChkDataPath(struct ADAPTER *prAdapter,
			struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec)
{
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	/* UINT_32 u4DwIdx; */
	uint32_t rRetStatus;

	do {
		rRetStatus = WLAN_STATUS_SUCCESS;
		if (!prPeerSchRec->fgUseDataPath)
			break;

#ifdef NAN_UNUSED
		prTimeline = &prPeerSchRec->arCommFawTimeline[0];
		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
			uint32_t u4BitCount;

			u4BitCount = nanUtilCheckBitOneCnt(
					&prTimeline->au4AvailMap[u4DwIdx],
					sizeof(uint32_t));
			if (u4BitCount < prPeerSchRec->u4DefNdlNumSlots) {
				rRetStatus = WLAN_STATUS_FAILURE;
				break;
			}
		}
#endif

		if (prPeerSchRec->prCommNdcCtrl) {
			prTimeline =
				&prPeerSchRec->prCommNdcCtrl->arTimeline[0];
			rRetStatus = nanSchedPeerChkTimeline(
				prAdapter, prPeerSchRec->prPeerSchDesc,
				prTimeline);
			if (rRetStatus != WLAN_STATUS_SUCCESS)
				break;
		}

		prTimeline = &prPeerSchRec->arCommImmuNdlTimeline[0];
		rRetStatus = nanSchedPeerChkTimeline(
			prAdapter, prPeerSchRec->prPeerSchDesc, prTimeline);
		if (rRetStatus != WLAN_STATUS_SUCCESS)
			break;

		rRetStatus = nanSchedPeerChkQos(prAdapter,
						prPeerSchRec->prPeerSchDesc);
		if (rRetStatus != WLAN_STATUS_SUCCESS)
			break;
	} while (FALSE);

	return rRetStatus;
}

uint32_t
nanSchedPeerChkRanging(struct ADAPTER *prAdapter,
		       struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec)
{
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	uint32_t rRetStatus;

	do {
		rRetStatus = WLAN_STATUS_SUCCESS;
		if (!prPeerSchRec->fgUseRanging)
			break;

		prTimeline = &prPeerSchRec->arCommRangingTimeline[0];
		rRetStatus = nanSchedPeerChkTimeline(
			prAdapter, prPeerSchRec->prPeerSchDesc, prTimeline);
		if (rRetStatus != WLAN_STATUS_SUCCESS)
			break;
	} while (FALSE);

	return rRetStatus;
}

void
nanSchedPeerChkAvailability(struct ADAPTER *prAdapter,
			    struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;
	uint32_t u4SchIdx;
	uint32_t rRetStatus;

	if (!prPeerSchDesc->fgUsed)
		return;

	u4SchIdx = prPeerSchDesc->u4SchIdx;
	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRec == NULL) {
		DBGLOG(NAN, ERROR, "No peer sch record\n");
		return;
	}

	/* Check Data Path CRB Violation */
	rRetStatus = nanSchedPeerChkDataPath(prAdapter, prPeerSchRec);
	if (rRetStatus != WLAN_STATUS_SUCCESS)
		nanSchedPeerDataPathConflict(prAdapter,
					     prPeerSchDesc->aucNmiAddr);

	/* Check Ranging CRB Violation */
	rRetStatus = nanSchedPeerChkRanging(prAdapter, prPeerSchRec);
	if (rRetStatus != WLAN_STATUS_SUCCESS)
		nanSchedPeerRangingConflict(prAdapter,
					    prPeerSchDesc->aucNmiAddr);
}

void
nanSchedPeerPrepareNegoState(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prPeerSchRecord = nanSchedLookupPeerSchRecord(prAdapter, pucNmiAddr);
	if (prPeerSchRecord == NULL)
		return;
	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	prPeerSchRecord->i4InNegoContext++;
	prNegoCtrl->i4InNegoContext++;

	DBGLOG(NAN, DEBUG,
	       "[%02x:%02x:%02x:%02x:%02x:%02x] i4InNegoContext:%d (%d) state(%d)\n",
	       pucNmiAddr[0], pucNmiAddr[1], pucNmiAddr[2], pucNmiAddr[3],
	       pucNmiAddr[4], pucNmiAddr[5], prPeerSchRecord->i4InNegoContext,
	       prNegoCtrl->i4InNegoContext,
	       prNegoCtrl->eState);
}

void
nanSchedPeerCompleteNegoState(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prPeerSchRecord = nanSchedLookupPeerSchRecord(prAdapter, pucNmiAddr);
	if (prPeerSchRecord == NULL)
		return;
	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	prPeerSchRecord->i4InNegoContext--;
	prNegoCtrl->i4InNegoContext--;

	DBGLOG(NAN, DEBUG,
	       "[%02x:%02x:%02x:%02x:%02x:%02x] i4InNegoContext:%d (%d) state(%d)\n",
	       pucNmiAddr[0], pucNmiAddr[1], pucNmiAddr[2], pucNmiAddr[3],
	       pucNmiAddr[4], pucNmiAddr[5], prPeerSchRecord->i4InNegoContext,
	       prNegoCtrl->i4InNegoContext,
	       prNegoCtrl->eState);
}

unsigned char
nanSchedPeerInNegoState(struct ADAPTER *prAdapter,
			struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;

	if (!prPeerSchDesc->fgUsed)
		return FALSE;

	prPeerSchRecord =
		nanSchedGetPeerSchRecord(prAdapter, prPeerSchDesc->u4SchIdx);

	if (!prPeerSchRecord) {
		DBGLOG(NAN, ERROR, "prPeerSchRecord error!\n");
		return FALSE;
	}

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if (prNegoCtrl->eSyncSchUpdateCurrentState !=
	    ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE)
		return TRUE;

#if 1
	if (prPeerSchRecord->i4InNegoContext > 0) {
		DBGLOG(NAN, DEBUG,
		       "PeeSchRecord:%d in NEGO state, i4InNegoContext:%d (%d)\n",
		       prPeerSchDesc->u4SchIdx,
		       prPeerSchRecord->i4InNegoContext,
		       prNegoCtrl->i4InNegoContext);
		return TRUE;
	}
#else
	if ((prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_IDLE) &&
	    (prNegoCtrl->u4SchIdx == prPeerSchDesc->u4SchIdx))
		return TRUE;
#endif

	return FALSE;
}

static void setBandChnlByPref(union _NAN_BAND_CHNL_CTRL dw2gChnl,
			      union _NAN_BAND_CHNL_CTRL dw5gChnl,
			      union _NAN_BAND_CHNL_CTRL miscChnl[],
			      union _NAN_BAND_CHNL_CTRL arBandChnlCtrl[],
			      uint8_t ucNumBandChnlCtrl)
{
	uint32_t i = 0;
	uint32_t j = 0;

	if (dw5gChnl.u4RawData) {
		arBandChnlCtrl[i++] = dw5gChnl;
		ucNumBandChnlCtrl--;
	}

	if (dw2gChnl.u4RawData) {
		arBandChnlCtrl[i++] = dw2gChnl;
		ucNumBandChnlCtrl--;
	}

	for  (j = 0 ; j < ucNumBandChnlCtrl && miscChnl[j].u4RawData != 0; j++)
		arBandChnlCtrl[i++] = miscChnl[j++];
}

#if MERGE_POTENTIAL
/* Matching a specific Availability Entry Pattern with limited Time Bitmap */
static u_int8_t isCommittedInsufficient(struct ADAPTER *prAdapter,
			uint16_t u2EntryControl,
			uint8_t *pucTimeBitmap,
			uint8_t ucTimeBitmapLength,
			struct _NAN_BAND_CHNL_LIST_T *prAttrBandChnlList)
{
	uint32_t u4BitCount;
	struct _NAN_CHNL_ENTRY_T *prAttrChnlEntry;

	DBGLOG(NAN, TRACE,
	       "u2EntryControl=0x%04x, C=%u, len=%u %02x-%02x-%02x-%02x ",
	       u2EntryControl,
	       NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl),
	       ucTimeBitmapLength,
	       pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);

	if (!NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl))
		return FALSE;

	if (prAttrBandChnlList->ucType != NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL ||
	    prAttrBandChnlList->ucNumberOfEntry != 1)
		return FALSE;

	prAttrChnlEntry =
		(struct _NAN_CHNL_ENTRY_T *)prAttrBandChnlList->aucEntry;
	if (IS_2G_OP_CLASS(prAttrChnlEntry->ucOperatingClass))
		return FALSE;

	if (ucTimeBitmapLength != TYPICAL_BITMAP_LENGTH)
		return FALSE;

	u4BitCount = nanUtilCheckBitOneCnt(pucTimeBitmap, ucTimeBitmapLength);
	if (u4BitCount >= prAdapter->rWifiVar.ucNanMergePotentialThreshold)
		return FALSE;

	DBGLOG(NAN, INFO,
	       "OC=%u, ChBitmap=0x%04x, PriBitmap=0x%02x, Bits=%u, C:%02x-%02x-%02x-%02x",
	       prAttrChnlEntry->ucOperatingClass,
	       prAttrChnlEntry->u2ChannelBitmap,
	       prAttrChnlEntry->ucPrimaryChnlBitmap,
	       u4BitCount, pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);
	return TRUE;
}

/* If a conditional availability is better than currently select one */
static u_int8_t isConditionalHigher(uint16_t u2EntryControl,
			uint8_t *pucTimeBitmap,
			uint8_t ucTimeBitmapLength,
			struct _NAN_BAND_CHNL_LIST_T *prAttrBandChnlList)
{
	uint32_t u4BitCount;
	struct _NAN_CHNL_ENTRY_T *prAttrChnlEntry;

	DBGLOG(NAN, TRACE,
	       "u2EntryControl=0x%04x, c=%u, len=%u %02x-%02x-%02x-%02x ",
	       u2EntryControl,
	       NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl),
	       ucTimeBitmapLength,
	       pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);

	if (!NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl))
		return FALSE;

	if (prAttrBandChnlList->ucType != NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL)
		return FALSE;

	prAttrChnlEntry =
		(struct _NAN_CHNL_ENTRY_T *)prAttrBandChnlList->aucEntry;
	if (!IS_6G_OP_CLASS(prAttrChnlEntry->ucOperatingClass))
		return FALSE;

	if (ucTimeBitmapLength != TYPICAL_BITMAP_LENGTH)
		return FALSE;

	/* 6G conditional always fill regardless of the number of set slots */
	u4BitCount = nanUtilCheckBitOneCnt(pucTimeBitmap, ucTimeBitmapLength);

	DBGLOG(NAN, INFO,
	       "OC=%u, ChBitmap=0x%04x, PriBitmap=0x%02x, Bits=%u, C:%02x-%02x-%02x-%02x",
	       prAttrChnlEntry->ucOperatingClass,
	       prAttrChnlEntry->u2ChannelBitmap,
	       prAttrChnlEntry->ucPrimaryChnlBitmap,
	       u4BitCount, pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);

	return TRUE;
}

static u_int8_t isPotentialCandidate(uint16_t u2EntryControl,
				     uint8_t *pucTimeBitmap,
				     uint8_t ucTimeBitmapLength)
{
	uint32_t u4BitCount;

	DBGLOG(NAN, TRACE,
	       "u2EntryControl=0x%04x, p=%u, len=%u %02x-%02x-%02x-%02x ",
	       u2EntryControl,
	       NAN_AVAIL_ENTRY_CTRL_POTENTIAL(u2EntryControl),
	       ucTimeBitmapLength,
	       pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);

	if (!NAN_AVAIL_ENTRY_CTRL_POTENTIAL(u2EntryControl))
		return FALSE;

	if (ucTimeBitmapLength != TYPICAL_BITMAP_LENGTH)
		return FALSE;

	u4BitCount = nanUtilCheckBitOneCnt(pucTimeBitmap, ucTimeBitmapLength);
	if (u4BitCount < SUFFICIENT_POTENTIAL_SLOTS)
		return FALSE;

	DBGLOG(NAN, INFO, "Bits=%u, P:%02x-%02x-%02x-%02x\n",
	       u4BitCount, pucTimeBitmap[0], pucTimeBitmap[1],
	       pucTimeBitmap[2], pucTimeBitmap[3]);
	return TRUE;
}

static uint8_t mergeCommittedPotentialTimeBitmap(uint8_t ucPotentialPriChnl,
					      uint8_t *comm_cond,
					      uint8_t *potential,
					      uint8_t *committed,
					      uint8_t *merged)
{
	int i;
	uint8_t modified = 0;

	for (i = 0; i < TYPICAL_BITMAP_LENGTH; i++) {
		merged[i] |= comm_cond[i] | (potential[i] & ~committed[i]);
		modified |= merged[i] - comm_cond[i];
	}

	DBGLOG(NAN, DEBUG,
	       "Ch: %u, C:%02x-%02x-%02x-%02x P:%02x-%02x-%02x-%02x => %02x-%02x-%02x-%02x, modified=0x%02x\n",
	       ucPotentialPriChnl,
	       comm_cond[0], comm_cond[1], comm_cond[2], comm_cond[3],
	       potential[0], potential[1], potential[2], potential[3],
	       merged[0], merged[1], merged[2], merged[3],
	       modified);

	return modified;
}
#endif

static u_int8_t nanIsR4Avail(uint16_t u2EntryControl, uint8_t ucOperatingClass,
			     void *pChannelBitmap)
{
	uint8_t ucStart = ((uint8_t *)pChannelBitmap)[0];
	uint8_t ucNum = ((uint8_t *)pChannelBitmap)[1];
	uint16_t u2Bitmap = *(uint16_t *)pChannelBitmap;

	DBGLOG(NAN, TRACE,
	       "Type:%u C:%u/p:%u/c:%u, 6G availability OC=%u, start=%u, num=%u, bitmap=0x%04x",
	       NAN_AVAIL_ENTRY_CTRL_TYPE(u2EntryControl),
	       NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl),
	       NAN_AVAIL_ENTRY_CTRL_POTENTIAL(u2EntryControl),
	       NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl),
	       ucOperatingClass, ucStart, ucNum, u2Bitmap);

	if (ucNum == 0)
		return FALSE;

	/* NAN R4 Table 9 */
	if ((NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl) ||
	     NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl)) && ucNum != 1)
		return FALSE;

	if (ucOperatingClass == 137 && ucStart % 64 == 31 ||
	    ucOperatingClass == 134 && ucStart % 32 == 15 ||
	    ucOperatingClass == 133 && ucStart % 16 == 7 ||
	    ucOperatingClass == 132 && ucStart % 8 == 3 ||
	    ucOperatingClass == 131 && ucStart % 4 == 1)
		return TRUE;

	return FALSE;
}

static u_int8_t nanOpClassBandMatch(uint8_t ucCheckOpClass, uint8_t ucOC)
{
	return IS_2G_OP_CLASS(ucCheckOpClass) && IS_2G_OP_CLASS(ucOC) ||
	       IS_5G_OP_CLASS(ucCheckOpClass) && IS_5G_OP_CLASS(ucOC) ||
	       IS_6G_OP_CLASS(ucCheckOpClass) && IS_6G_OP_CLASS(ucOC);
}

static u_int8_t nanIsChosenInPotential(struct _NAN_SIMPLE_CHNL_ENTRY_T *pChosen,
				struct _NAN_SIMPLE_CHNL_ENTRY_T *prPotential,
				struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry)
{
	if (!pChosen)
		return FALSE;

	if (pChosen->ucOperatingClass != prPotential->ucOperatingClass)
		return FALSE;

	if (nanIsR4Avail(prAvailEntry->u2EntryControl,
			 prPotential->ucOperatingClass,
			 &prPotential->u2ChannelBitmap)) {

		if (pChosen->ucChannelStart < prPotential->ucChannelStart)
			return FALSE;

		if (pChosen->ucChannelStart + pChosen->ucChannelNum >
		    prPotential->ucChannelStart + prPotential->ucChannelNum)
			return FALSE;

	} else {
		if (!(pChosen->u2ChannelBitmap & prPotential->u2ChannelBitmap))
			return FALSE;
	}

	if (!(pChosen->ucPrimaryChnlBitmap & prPotential->ucPrimaryChnlBitmap))
		return FALSE;

	return TRUE;
}

static struct _NAN_AVAILABILITY_ENTRY_T *
scanAvailabilityAttr(struct _NAN_ATTR_NAN_AVAILABILITY_T *prAttrNanAvailibility,
		     struct _NAN_AVAILABILITY_ENTRY_SIMPLE_T *prConditional)
{
	struct _NAN_SIMPLE_CHNL_ENTRY_T *prChnlEntry;
	uint8_t *pTimeBitmapTmp;
	struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry;
	struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T *prTimeBitmap;
	struct _NAN_BAND_CHNL_LIST_T *prChnlList;
	struct _NAN_SIMPLE_CHNL_ENTRY_T *pChosen = NULL;
	struct _NAN_SIMPLE_CHNL_ENTRY_T rPrefer2gChannel = {
		.ucOperatingClass = g_r2gDwChnl.u4OperatingClass,
		.u2ChannelBitmap = BITS(0, 10), /* ff 07 (little endian dump) */
		.ucPrimaryChnlBitmap = 0,
	};
#if (CFG_SUPPORT_NAN_6G == 1)
	struct _NAN_SIMPLE_CHNL_ENTRY_T rPrefer6gChannel = {
		.ucOperatingClass = g_r6gDefChnl.u4OperatingClass,
		.u2ChannelBitmap = BIT(0), /* 01 00 (little endian dump) */
		.ucPrimaryChnlBitmap =
			BIT((g_r6gDefChnl.u4PrimaryChnl - 1) / 4),
	};
#endif
	uint8_t *pucTimeBitmap = NULL;
	struct _NAN_SIMPLE_CHNL_ENTRY_T *prBandChnlList;
	uint8_t *pNextAvailEntry;
	uint8_t *pucAvailAttrEnd;
	uint32_t i;
	uint32_t u4CommittedBitmap = 0;
	u_int8_t fgConditional = FALSE;
	u_int8_t fgCommitted2G = FALSE;
	u_int8_t fgCommitted6G = FALSE;
	void *pConditionalPtr = NULL;
	uint32_t ucCheckOpClass;
	uint8_t ucTimeBitmapLength;
	uint8_t *pucBand;
	uint8_t *pucChnl;
	uint8_t aucMergedTimeBitmap[TYPICAL_BITMAP_LENGTH] = {0};

	prChnlEntry = &prConditional->channelEntry.rChnlEntry;
	ucCheckOpClass = prChnlEntry->ucOperatingClass;

	pNextAvailEntry = prAttrNanAvailibility->aucAvailabilityEntryList;
	pucAvailAttrEnd = NAN_ATTR_END(prAttrNanAvailibility);

	do {
		prAvailEntry =
			(struct _NAN_AVAILABILITY_ENTRY_T *)pNextAvailEntry;

		pNextAvailEntry = NAN_AVAIL_ENTRY_END(prAvailEntry);

		if (prAvailEntry->rCtrl.u2TypeConditional)
			fgConditional = TRUE;

		if (prAvailEntry->rCtrl.u2TypePotential && !pConditionalPtr)
			pConditionalPtr = prAvailEntry;

		if (!prAvailEntry->rCtrl.u2TimeBitmapPresent)
			continue;

		prTimeBitmap = (struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T *)
			prAvailEntry;
		ucTimeBitmapLength = prTimeBitmap->ucTimeBitmapLength;
		if (ucTimeBitmapLength >
		    pNextAvailEntry - prTimeBitmap->aucTimeBitmapAndBandChnl) {
			DBGLOG(NAN, WARN, "TimeBitmapLength=%u too large",
			       ucTimeBitmapLength);
			continue;
		}

		prChnlList = (struct _NAN_BAND_CHNL_LIST_T *)
			(prTimeBitmap->aucTimeBitmapAndBandChnl +
			 ucTimeBitmapLength);

		pTimeBitmapTmp = prTimeBitmap->aucTimeBitmapAndBandChnl;

		if (prChnlList->ucNonContiguous)
			continue;

		if (prChnlList->ucType == NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {
			pucBand = prChnlList->aucEntry;

			if (prChnlList->ucNumberOfEntry >
			    pNextAvailEntry - pucBand) {
				DBGLOG(NAN, WARN,
				       "ucNumberOfEntry=%u too large (> %td)",
				       prChnlList->ucNumberOfEntry,
				       pNextAvailEntry - pucBand);
				continue;
			}

			for (i = 0; i < prChnlList->ucNumberOfEntry; i++) {
				DBGLOG(NAN, INFO, "potential band: %u",
				       pucBand[i]);
				if (IS_2G_OP_CLASS(ucCheckOpClass) &&
				    pucBand[i] == NAN_SUPPORTED_BAND_ID_2P4G &&
				    !pChosen &&
				    !fgCommitted2G && !fgConditional) {
					pChosen = &rPrefer2gChannel;
					pucTimeBitmap = pTimeBitmapTmp;
					kalMemCopy(aucMergedTimeBitmap,
						   pucTimeBitmap,
						   TYPICAL_BITMAP_LENGTH);
					continue;
				}
#if (CFG_SUPPORT_NAN_6G == 1)
				if (IS_6G_OP_CLASS(ucCheckOpClass) &&
				    (pucBand[i] == NAN_PROPRIETARY_BAND_ID_6G ||
				     pucBand[i] == NAN_SUPPORTED_BAND_ID_6G) &&
				    !pChosen &&
				    !fgCommitted6G && !fgConditional) {
					pChosen = &rPrefer6gChannel;
					pucTimeBitmap = pTimeBitmapTmp;
					kalMemCopy(aucMergedTimeBitmap,
						   pucTimeBitmap,
						   TYPICAL_BITMAP_LENGTH);
					continue;
				}
#endif
			}
			continue;
		}

		/* NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL */
		pucChnl = prChnlList->aucEntry;
		if (prChnlList->ucNumberOfEntry *
		    sizeof(struct _NAN_CHNL_ENTRY_NO_AUX_T) >
		    pNextAvailEntry - pucChnl) {
			DBGLOG(NAN, WARN,
			       "Channel ucNumberOfEntry=%u too large (> %td)",
			       prChnlList->ucNumberOfEntry,
			       pNextAvailEntry - pucChnl);
			continue;
		}

		prBandChnlList = (struct _NAN_SIMPLE_CHNL_ENTRY_T *)
			prChnlList->aucEntry;
		for (i = 0; i < prChnlList->ucNumberOfEntry; i++) {
			uint8_t ucOC;
			uint8_t ucPrimaryChnlBitmap;
			uint16_t u2ChannelBitmap;

			ucOC = prBandChnlList[i].ucOperatingClass;
			u2ChannelBitmap = prBandChnlList[i].u2ChannelBitmap;
			ucPrimaryChnlBitmap =
				prBandChnlList[i].ucPrimaryChnlBitmap;
			if (prAvailEntry->rCtrl.u2TypeCommitted) {
				u4CommittedBitmap |=
					*(uint32_t *)pTimeBitmapTmp;
				if (IS_6G_OP_CLASS(ucOC))
					fgCommitted6G = TRUE;
				if (IS_2G_OP_CLASS(ucOC))
					fgCommitted2G = TRUE;
			}

			if (!prAvailEntry->rCtrl.u2TypePotential)
				continue;

			if (!nanOpClassBandMatch(ucCheckOpClass, ucOC))
				continue;

			DBGLOG(NAN, TRACE,
			       "Check potential channel[%u], OC=%u, u2ChannelBitmap=0x%04x, ucPrimaryChnlBitmap=0x%02x, %02x-%02x-%02x-%02x",
			       i, ucOC, u2ChannelBitmap, ucPrimaryChnlBitmap,
			       pTimeBitmapTmp[0],
			       pTimeBitmapTmp[1],
			       pTimeBitmapTmp[2],
			       pTimeBitmapTmp[3]);

			if (IS_6G_OP_CLASS(ucOC) &&
			    IS_6G_OP_CLASS(ucCheckOpClass)) {
				/* Reach here: 6G && Potential */

				/* merge multiple potential of same channel */
				if (nanIsChosenInPotential(pChosen,
							   &prBandChnlList[i],
							   prAvailEntry)) {
					DBGLOG(NAN, TRACE,
					      "Same channel, OC=%u %02x-%02x-%02x-%02x",
					      ucOC,
					      aucMergedTimeBitmap[0],
					      aucMergedTimeBitmap[1],
					      aucMergedTimeBitmap[2],
					      aucMergedTimeBitmap[3]);
					for (i = 0; i < TYPICAL_BITMAP_LENGTH;
					     i++) {
						aucMergedTimeBitmap[i] |=
							pTimeBitmapTmp[i];
					}
					DBGLOG(NAN, TRACE,
					      "Merged channel, %02x-%02x-%02x-%02x",
					      aucMergedTimeBitmap[0],
					      aucMergedTimeBitmap[1],
					      aucMergedTimeBitmap[2],
					      aucMergedTimeBitmap[3]);
				}

				if (!pChosen &&
				    !fgCommitted6G && !fgConditional) {
					pChosen = &prBandChnlList[i];
					pucTimeBitmap = pTimeBitmapTmp;
					kalMemCopy(aucMergedTimeBitmap,
						   pucTimeBitmap,
						   TYPICAL_BITMAP_LENGTH);
				}

				if (!pChosen)
					continue;

				/* pChosen */
				if (pChosen->ucOperatingClass < ucOC) {
					/* better chosen */
					pChosen = &prBandChnlList[i];
					pucTimeBitmap = pTimeBitmapTmp;
					kalMemCopy(aucMergedTimeBitmap,
						   pucTimeBitmap,
						   TYPICAL_BITMAP_LENGTH);
				}
			}

			if (IS_2G_OP_CLASS(ucOC) &&
			    IS_2G_OP_CLASS(ucCheckOpClass)) {
				/* Reach here: 2G && Potential.
				 * Skip checking fgCommitted2G, since in the
				 * case FC enabled it is always set.
				 */
				if (!pChosen && !fgConditional) {
					pChosen = &prBandChnlList[i];
					pucTimeBitmap = pTimeBitmapTmp;
					kalMemCopy(aucMergedTimeBitmap,
						   pucTimeBitmap,
						   TYPICAL_BITMAP_LENGTH);
				}

				if (!pChosen)
					continue;

				/* pChosen */
				if (pChosen->ucOperatingClass < ucOC) {
					/* better chosen */
					pChosen = &prBandChnlList[i];
					pucTimeBitmap = pTimeBitmapTmp;
					kalMemCopy(aucMergedTimeBitmap,
						   pucTimeBitmap,
						   TYPICAL_BITMAP_LENGTH);
				}
			}
		}

		if (pChosen) {
			DBGLOG(NAN, TRACE,
			       "pChosen, OC=%u, u2ChannelBitmap=0x%04x, ucPrimaryChnlBitmap=0x%02x, %02x-%02x-%02x-%02x",
			       pChosen->ucOperatingClass,
			       pChosen->u2ChannelBitmap,
			       pChosen->ucPrimaryChnlBitmap,
			       aucMergedTimeBitmap[0],
			       aucMergedTimeBitmap[1],
			       aucMergedTimeBitmap[2],
			       aucMergedTimeBitmap[3]);
		}
	} while (pNextAvailEntry < pucAvailAttrEnd);

	if (pChosen) {
		prChnlEntry->ucOperatingClass = pChosen->ucOperatingClass;
		prChnlEntry->u2ChannelBitmap = pChosen->u2ChannelBitmap;
		prChnlEntry->ucPrimaryChnlBitmap = pChosen->ucPrimaryChnlBitmap;
		kalMemCopy(prConditional->aucTimeBitmap, aucMergedTimeBitmap,
			   TYPICAL_BITMAP_LENGTH);
		DBGLOG(NAN, DEBUG,
		       "Committed=0x%08x, potential=0x%08x => conditional=0x%08x\n",
		       u4CommittedBitmap, prConditional->u4TimeBitmap,
		       prConditional->u4TimeBitmap & ~u4CommittedBitmap);
		prConditional->u4TimeBitmap &= ~u4CommittedBitmap;
		if (prConditional->u4TimeBitmap == 0)
			return NULL;
		return pConditionalPtr;
	} else {
		return NULL;
	}
}

static uint8_t nanGetPeerBands(struct ADAPTER *prAdapter,
			       union _NAN_BAND_CHNL_CTRL *prBandCtrl)
{
	struct _NAN_SCHEDULER_T *prNanScheduler;
	uint8_t ucPeerSupportedBand = 0;

	prNanScheduler = nanGetScheduler(prAdapter);

	if (prBandCtrl->u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_2P4G))
		ucPeerSupportedBand |= BIT(ENUM_SUPPORTED_BN_2G);

	if (prBandCtrl->u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_5G)) {
		if (prNanScheduler) {
			if (prNanScheduler->fgEn5gL)
				ucPeerSupportedBand |=
					BIT(ENUM_SUPPORTED_BN_5G_LOW);
			if (prNanScheduler->fgEn5gH)
				ucPeerSupportedBand |=
					BIT(ENUM_SUPPORTED_BN_5G_HIGH);
		} else {
			ucPeerSupportedBand |=
				(BIT(ENUM_SUPPORTED_BN_5G_LOW) |
				 BIT(ENUM_SUPPORTED_BN_5G_HIGH));
		}
	}

	if (prBandCtrl->u4BandIdMask &
	    (BIT(NAN_PROPRIETARY_BAND_ID_6G) | BIT(NAN_SUPPORTED_BAND_ID_6G)))
		ucPeerSupportedBand |= BIT(ENUM_SUPPORTED_BN_6G);

	return ucPeerSupportedBand;
}

uint32_t
nanSchedChkPeerCommonBand(struct ADAPTER *prAdapter,
			  uint8_t *pucNmiAddr)
{
	uint32_t u4Idx, u4Idx1;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_AVAILABILITY_DB_T *prNanAvailAttr;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	uint32_t j;
	uint32_t u4OperatingClass;
	uint32_t u4PrimaryChnl;
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);
	uint8_t ucPeerSupportedBand = 0; /* bitmap of NAN_SUPPORTED_BANDS */
	union _NAN_BAND_CHNL_CTRL *prBandChnlCtrl;
	union _NAN_BAND_CHNL_CTRL *prNanChannelCtrl;

	prPeerSchDesc = nanSchedSearchPeerSchDescByNmi(prAdapter, pucNmiAddr);

	if (prPeerSchDesc == NULL)
		return WLAN_STATUS_FAILURE;

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prNanAvailAttr = &prPeerSchDesc->arAvailAttr[u4Idx];
		if (prNanAvailAttr->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		for (u4Idx1 = 0; u4Idx1 < NAN_NUM_AVAIL_TIMELINE; u4Idx1++) {
			prNanAvailEntry =
				&prNanAvailAttr->arAvailEntryList[u4Idx1];
			if (prNanAvailEntry->fgActive == FALSE)
				continue;

			for (j = 0; j < NAN_NUM_BAND_CHNL_ENTRY; j++) {
				prBandChnlCtrl =
					&prNanAvailEntry->arBandChnlCtrl[j];
				if (prBandChnlCtrl->u4Type ==
				    NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {

					ucPeerSupportedBand |=
						nanGetPeerBands(prAdapter,
								prBandChnlCtrl);
					continue;
				}

				/* NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL */
				prNanChannelCtrl = prBandChnlCtrl;
				if (prNanChannelCtrl->u4PrimaryChnl == 0)
					continue;

				u4OperatingClass =
					prNanChannelCtrl->u4OperatingClass;
				u4PrimaryChnl = prNanChannelCtrl->u4PrimaryChnl;
				if (IS_2G_OP_CLASS(u4OperatingClass))
					ucPeerSupportedBand |=
						BIT(ENUM_SUPPORTED_BN_2G);

				if (IS_5G_OP_CLASS(u4OperatingClass) &&
				    u4PrimaryChnl >= UNII1_LOWER_BOUND &&
				    u4PrimaryChnl <= UNII1_UPPER_BOUND)
					ucPeerSupportedBand |=
						BIT(ENUM_SUPPORTED_BN_5G_LOW);

				if (IS_5G_OP_CLASS(u4OperatingClass) &&
				    u4PrimaryChnl >= UNII3_LOWER_BOUND &&
				    u4PrimaryChnl <= UNII3_UPPER_BOUND)
					ucPeerSupportedBand |=
						BIT(ENUM_SUPPORTED_BN_5G_HIGH);

				if (IS_6G_OP_CLASS(u4OperatingClass))
					ucPeerSupportedBand |=
						BIT(ENUM_SUPPORTED_BN_6G);
			}
		}
	}

	prPeerSchDesc->u4CommonSupportedBand =
		(prNanScheduler->ucLocalSupportedBand & ucPeerSupportedBand);
	prPeerSchDesc->u4PeerSupportedBand = ucPeerSupportedBand;

	DBGLOG(NAN, INFO,
	       "Peer %02x:%02x:%02x:%02x:%02x:%02x, CommonBn[6G/5GH/5GL/2G]:[%u/%u/%u/%u], SupportBn[6G/5GH/5GL/2G]:[%u/%u/%u/%u]\n",
	       prPeerSchDesc->aucNmiAddr[0], prPeerSchDesc->aucNmiAddr[1],
	       prPeerSchDesc->aucNmiAddr[2], prPeerSchDesc->aucNmiAddr[3],
	       prPeerSchDesc->aucNmiAddr[4], prPeerSchDesc->aucNmiAddr[5],
	       !!(prPeerSchDesc->u4CommonSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_6G)),
	       !!(prPeerSchDesc->u4CommonSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_5G_HIGH)),
	       !!(prPeerSchDesc->u4CommonSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_5G_LOW)),
	       !!(prPeerSchDesc->u4CommonSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_2G)),
	       !!(prPeerSchDesc->u4PeerSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_6G)),
	       !!(prPeerSchDesc->u4PeerSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_5G_HIGH)),
	       !!(prPeerSchDesc->u4PeerSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_5G_LOW)),
	       !!(prPeerSchDesc->u4PeerSupportedBand &
	       BIT(ENUM_SUPPORTED_BN_2G)));

	return WLAN_STATUS_SUCCESS;
}

uint32_t nanGetCommonBandWithNdl(struct ADAPTER *prAdapter,
					struct _NAN_NDL_INSTANCE_T *prNDL)
{
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	prPeerSchDesc = nanSchedSearchPeerSchDescByNmi(prAdapter,
						       prNDL->aucPeerMacAddr);
	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR,
			"Cannot get PeerSchDesc by "MACSTR"\n",
			prNDL->aucPeerMacAddr);
			return 0;
	}
	DBGLOG(NAN, INFO, "Peer "MACSTR", CommonSupported=%u\n",
			prNDL->aucPeerMacAddr,
			prPeerSchDesc->u4CommonSupportedBand);
	return prPeerSchDesc->u4CommonSupportedBand;
}

uint32_t nanGetCommonBandWithConcurrent(struct ADAPTER *prAdapter,
					u_int8_t fgPrint,
					uint32_t u4CommonSupportedBand)
{
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo = {0};
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo = {0};
	u_int8_t fgMcc;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4Idx;

	fgMcc = nanIsP2pAisMCC(prAdapter, sz5gTimeLineIdx,
			       &rP2pChnlInfo, &rAisChnlInfo);

	if (fgMcc) {
		NAN_DW_DBGLOG(NAN, TRACE, fgPrint, 0,
			       "MCC in 5G/6G band ais=%u, p2p=%u, NAN use 2G",
			       rAisChnlInfo.u4PrimaryChnl,
			       rP2pChnlInfo.u4PrimaryChnl);
		u4CommonSupportedBand = BIT(ENUM_SUPPORTED_BN_2G);
	} else if (rP2pChnlInfo.u4PrimaryChnl) {
		switch (NAN_GET_CHNL_BAND_INFO(rP2pChnlInfo)) {
		case ENUM_SUPPORTED_BN_6G:
			NAN_DW_DBGLOG(NAN, TRACE, fgPrint, 0,
			       "P2P in 6G ch %u, common 0x%x->0x%x",
			       rP2pChnlInfo.u4PrimaryChnl,
			       u4CommonSupportedBand,
			       (uint32_t)(u4CommonSupportedBand &
				~(BIT(ENUM_SUPPORTED_BN_5G_HIGH) |
				  BIT(ENUM_SUPPORTED_BN_5G_LOW))));
			u4CommonSupportedBand &=
				~(BIT(ENUM_SUPPORTED_BN_5G_HIGH) |
				  BIT(ENUM_SUPPORTED_BN_5G_LOW));
			break;

		case ENUM_SUPPORTED_BN_5G_HIGH:
			NAN_DW_DBGLOG(NAN, TRACE, fgPrint, 0,
			       "P2P in 5G ch %u, common 0x%x->0x%x",
			       rP2pChnlInfo.u4PrimaryChnl,
			       u4CommonSupportedBand,
			       u4CommonSupportedBand &
			       (uint32_t)(~(BIT(ENUM_SUPPORTED_BN_6G) |
					    BIT(ENUM_SUPPORTED_BN_5G_LOW))));
			u4CommonSupportedBand &=
				~(BIT(ENUM_SUPPORTED_BN_6G) |
				  BIT(ENUM_SUPPORTED_BN_5G_LOW));
			break;

		case ENUM_SUPPORTED_BN_5G_LOW:
			NAN_DW_DBGLOG(NAN, TRACE, fgPrint, 0,
			       "P2P in 5G ch %u, common 0x%x->0x%x",
			       rP2pChnlInfo.u4PrimaryChnl,
			       u4CommonSupportedBand,
			       (uint32_t)(u4CommonSupportedBand &
				~(BIT(ENUM_SUPPORTED_BN_6G) |
				  BIT(ENUM_SUPPORTED_BN_5G_HIGH))));
			u4CommonSupportedBand &=
				~(BIT(ENUM_SUPPORTED_BN_6G) |
				  BIT(ENUM_SUPPORTED_BN_5G_HIGH));
			break;

		case ENUM_SUPPORTED_BN_2G:
		case ENUM_SUPPORTED_BN_NUM:
			break;
		}
	}

	/* single map, considering current established NDL */
	if (nanGetEstablishedNdlNum(prAdapter) >= 1 &&
	    nanGetActiveTimelineMgmtNum(prAdapter) == 1) {
		prDataPathInfo = &prAdapter->rDataPathInfo;
		for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDL_NUM; u4Idx++) {
			prNDL = &prDataPathInfo->arNDL[u4Idx];
			if (prNDL->fgNDLValid &&
			    prNDL->eCurrentNDLMgmtState ==
			    NDL_SCHEDULE_ESTABLISHED) {
				u4CommonSupportedBand &=
				nanGetCommonBandWithNdl(prAdapter, prNDL);
			}
		}
	}
	DBGLOG(NAN, INFO, "CommonSupportBand = %u\n", u4CommonSupportedBand);

	return u4CommonSupportedBand;
}

static
uint32_t nanGetConcurrentCommonBand(struct ADAPTER *prAdapter,
				    struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				    u_int8_t fgPrint)
{
	return nanGetCommonBandWithConcurrent(prAdapter, fgPrint,
					prPeerSchDesc->u4CommonSupportedBand);
}

enum _NAN_SUPPORTED_BAND_BIT
nanGetNafHighestCommonBand(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prScheduler = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_PEER_SCH_DESC_T *prSchPeerDesc = NULL;
	uint32_t u4PeerSupportedBand = 0;

	prScheduler = nanGetScheduler(prAdapter);
	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prSchPeerDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prSchPeerDesc == NULL) {
		DBGLOG(NAN, DEBUG, "SchPeerDesc[%u] is NULL\n",
			prNegoCtrl->u4SchIdx);
		return ENUM_SUPPORTED_BN_2G;
	}

	u4PeerSupportedBand = prSchPeerDesc->u4PeerSupportedBand;

	/* Peer and Local both support 5G_H */
	if ((u4PeerSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH)) &&
	    prScheduler->fgEn5gH == TRUE)
		return ENUM_SUPPORTED_BN_5G_HIGH;

	/* Peer and Local both support 5G_L only */
	if (prScheduler->fgEn5gH == FALSE && prScheduler->fgEn5gL == TRUE &&
	    (u4PeerSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH)) == 0 &&
	    u4PeerSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_LOW))
		return ENUM_SUPPORTED_BN_5G_LOW;

	return ENUM_SUPPORTED_BN_2G;
}

enum _NAN_SUPPORTED_BAND_BIT
nanGetHighestCommonBand(struct ADAPTER *prAdapter,
			struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
			u_int8_t fgPrint)
{
	uint32_t u4CommonSupportedBand;
	enum _NAN_SUPPORTED_BAND_BIT eReturnBand = ENUM_SUPPORTED_BN_2G;
	static const char * const pcaBandString[] = {
		[ENUM_SUPPORTED_BN_2G] = "2G",
		[ENUM_SUPPORTED_BN_5G_LOW] = "5GL",
		[ENUM_SUPPORTED_BN_5G_HIGH] = "5GH",
		[ENUM_SUPPORTED_BN_6G] = "6G",
		[ENUM_SUPPORTED_BN_NUM] = "Unknown",
	};

	u4CommonSupportedBand =
		nanGetConcurrentCommonBand(prAdapter, prPeerSchDesc,
					    fgPrint);

	if (u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_6G))
		eReturnBand = ENUM_SUPPORTED_BN_6G;
	else if (u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH))
		eReturnBand = ENUM_SUPPORTED_BN_5G_HIGH;
	else if (u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_LOW))
		eReturnBand = ENUM_SUPPORTED_BN_5G_LOW;
	else
		eReturnBand = ENUM_SUPPORTED_BN_2G;


	NAN_DW_DBGLOG(NAN, INFO, fgPrint, 0, "Highest band=%u(%s)\n",
		      eReturnBand, pcaBandString[eReturnBand]);

	return eReturnBand;
}

enum _NAN_SUPPORTED_BAND_BIT
nanSchedGetHighestCommonBand(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			     u_int8_t fgPrint)
{
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	enum _NAN_SUPPORTED_BAND_BIT eCommittedBand = 0;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);

	if (prPeerSchDesc == NULL) {
		NAN_DW_DBGLOG(NAN, WARN, fgPrint, 0,
			      "peer schedule desc not found %u", u4SchIdx);
		return ENUM_SUPPORTED_BN_NUM;
	}

	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
	    prPeerSchDesc->u4CommittedBand) {
		if (prPeerSchDesc->u4CommittedBand &
		    BIT(ENUM_SUPPORTED_BN_2G))
			eCommittedBand = ENUM_SUPPORTED_BN_2G;
		if (prPeerSchDesc->u4CommittedBand &
		    BIT(ENUM_SUPPORTED_BN_5G_LOW))
			eCommittedBand = ENUM_SUPPORTED_BN_5G_LOW;
		if (prPeerSchDesc->u4CommittedBand &
		    BIT(ENUM_SUPPORTED_BN_5G_HIGH))
			eCommittedBand = ENUM_SUPPORTED_BN_5G_HIGH;
		if (prPeerSchDesc->u4CommittedBand &
		    BIT(ENUM_SUPPORTED_BN_6G))
			eCommittedBand = ENUM_SUPPORTED_BN_6G;

		return eCommittedBand;
	}

	return nanGetHighestCommonBand(prAdapter, prPeerSchDesc, fgPrint);
}

struct _NAN_ATTR_NAN_AVAILABILITY_T*
nanInsertConditionalAvailability(uint8_t *pucAvailabilityAttr,
			struct _NAN_AVAILABILITY_ENTRY_SIMPLE_T *prConditional,
			struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry)
{
	uint8_t *p = (uint8_t *)prAvailEntry;
	uint8_t *end;
	size_t orig_size;
	size_t new_size;
	struct _NAN_ATTR_NAN_AVAILABILITY_T *prAttrNanAvailibility;
	uint8_t *pucAvailAttrCond;

	prAttrNanAvailibility = (struct _NAN_ATTR_NAN_AVAILABILITY_T *)
		pucAvailabilityAttr;

	orig_size = NAN_ATTR_SIZE(prAttrNanAvailibility);
	new_size = orig_size + sizeof(*prConditional);

	DBGDUMP_HEX(NAN, INFO, "Before add conditional",
		    prAttrNanAvailibility, orig_size);
	pucAvailAttrCond = kalMemAlloc(new_size, VIR_MEM_TYPE);
	if (!pucAvailAttrCond)
		return NULL;

	end = pucAvailabilityAttr + orig_size;
	kalMemCopy(pucAvailAttrCond, pucAvailabilityAttr,
		   p - pucAvailabilityAttr);
	kalMemCopy(pucAvailAttrCond + (p - pucAvailabilityAttr),
		   prConditional, sizeof(*prConditional));
	kalMemCopy(pucAvailAttrCond + (p - pucAvailabilityAttr) +
		   sizeof(*prConditional), p, end - p);

	prAttrNanAvailibility =
		(struct _NAN_ATTR_NAN_AVAILABILITY_T *)pucAvailAttrCond;
	prAttrNanAvailibility->u2Length += sizeof(*prConditional);

	DBGDUMP_HEX(NAN, INFO, "After add conditional",
		    prAttrNanAvailibility, new_size);

	return prAttrNanAvailibility;
}

static char nanChnlTypeStr(struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry)
{
	if (prAvailEntry->rCtrl.u2TypeCommitted)
		return 'C';
	if (prAvailEntry->rCtrl.u2TypePotential)
		return 'p';
	if (prAvailEntry->rCtrl.u2TypeConditional)
		return 'c';

	return ' ';
}

static uint8_t
nanGetSubBandByChannelEntry(struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry,
			    struct _NAN_BAND_CHNL_LIST_T *prChnlList,
			    void *pEntry)
{
	uint8_t ucSupportedBands = 0;
	uint8_t *pucBand;
	struct _NAN_SIMPLE_CHNL_ENTRY_T *prChnlEntry;

	if (prChnlList->ucType == NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {
		pucBand = pEntry;
		if (*pucBand == NAN_SUPPORTED_BAND_ID_2P4G)
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_2G);
		if (*pucBand == NAN_SUPPORTED_BAND_ID_5G) {
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_5G_LOW) |
					    BIT(ENUM_SUPPORTED_BN_5G_HIGH);
		}
#if (CFG_SUPPORT_NAN_6G == 1)
		if (*pucBand == NAN_SUPPORTED_BAND_ID_6G)
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_6G);
#endif
	} else { /* prChnlList->ucType == NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL */
		prChnlEntry = pEntry;
#if (CFG_SUPPORT_NAN_6G == 1)
		if (IS_6G_OP_CLASS(prChnlEntry->ucOperatingClass))
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_6G);
#endif

		if (IS_2G_OP_CLASS(prChnlEntry->ucOperatingClass))
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_2G);

		if (IS_5G_HIGH_ONLY_OP_CLASS(prChnlEntry->ucOperatingClass))
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_5G_HIGH);
		else if (IS_5G_LOW_ONLY_OP_CLASS(prChnlEntry->ucOperatingClass))
			ucSupportedBands |= BIT(ENUM_SUPPORTED_BN_5G_LOW);
		else if (IS_5G_OP_CLASS(prChnlEntry->ucOperatingClass)) {
			if (BIT(0) & prChnlEntry->u2ChannelBitmap)
				ucSupportedBands |=
					BIT(ENUM_SUPPORTED_BN_5G_LOW);

			if (BIT(5) & prChnlEntry->u2ChannelBitmap)
				ucSupportedBands |=
					BIT(ENUM_SUPPORTED_BN_5G_HIGH);
		}
	}

	DBGLOG(NAN, TRACE,
	       "%c availability %s support 6G/5GH/5GL/2G=%u/%u/%u/%u",
	       nanChnlTypeStr(prAvailEntry),
	       prChnlList->ucType == NAN_BAND_CH_ENTRY_LIST_TYPE_BAND ?
	       "Band" : "Channel",
	       !!(ucSupportedBands & BIT(ENUM_SUPPORTED_BN_6G)),
	       !!(ucSupportedBands & BIT(ENUM_SUPPORTED_BN_5G_HIGH)),
	       !!(ucSupportedBands & BIT(ENUM_SUPPORTED_BN_5G_LOW)),
	       !!(ucSupportedBands & BIT(ENUM_SUPPORTED_BN_2G)));

	return ucSupportedBands;
}

/* Given an availability attribute to get the supported band information.
 * parse pAvailabilityAttr and collect the common band information.
 * This is a simple version parse only as scanAvailabilityAttr
 */
u_int8_t nanCommonBandFromAvailabilityAttr(struct ADAPTER *prAdapter,
					   void *pAvailabilityAttr)
{
	struct _NAN_ATTR_NAN_AVAILABILITY_T *prAvailabilityAttr =
					pAvailabilityAttr;
	struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry;
	struct _NAN_SIMPLE_CHNL_ENTRY_T *prBandChnlList;
	struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T *prTimeBitmap;
	struct _NAN_BAND_CHNL_LIST_T *prChnlList;
	uint8_t *pTimeBitmapTmp;
	uint8_t *pucAvailEntry;
	uint8_t *pucAvailAttrEnd;
	uint32_t i;
	uint8_t ucSupportedBands = BIT(ENUM_SUPPORTED_BN_2G);
	uint8_t ucTimeBitmapLength;
	uint8_t *pucBand;
	uint8_t *pucChnl;

	/* TODO: check length */
	if (prAvailabilityAttr->ucAttrId != NAN_ATTR_ID_NAN_AVAILABILITY)
		return ucSupportedBands;

	pucAvailEntry = prAvailabilityAttr->aucAvailabilityEntryList;
	pucAvailAttrEnd = NAN_ATTR_END(prAvailabilityAttr);

	do {
		prAvailEntry =
			(struct _NAN_AVAILABILITY_ENTRY_T *)pucAvailEntry;
		pucAvailEntry = NAN_AVAIL_ENTRY_END(prAvailEntry);

		if (!prAvailEntry->rCtrl.u2TimeBitmapPresent)
			continue;

		prTimeBitmap = (struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T *)
			prAvailEntry;
		ucTimeBitmapLength = prTimeBitmap->ucTimeBitmapLength;

		prChnlList = (struct _NAN_BAND_CHNL_LIST_T *)
			(prTimeBitmap->aucTimeBitmapAndBandChnl +
			 ucTimeBitmapLength);

		pTimeBitmapTmp = prTimeBitmap->aucTimeBitmapAndBandChnl;

		if (prChnlList->ucNonContiguous)
			continue;

		if (prChnlList->ucType == NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {
			pucBand = prChnlList->aucEntry;

			if (prChnlList->ucNumberOfEntry >
			    pucAvailEntry - pucBand) {
				DBGLOG(NAN, WARN,
				       "ucNumberOfEntry=%u too large (> %td)",
				       prChnlList->ucNumberOfEntry,
				       pucAvailEntry - pucBand);
				continue;
			}

			for (i = 0; i < prChnlList->ucNumberOfEntry; i++) {
				ucSupportedBands |= nanGetSubBandByChannelEntry(
							prAvailEntry,
							prChnlList,
							&pucBand[i]);
			}
			continue;
		}

		/* NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL */
		pucChnl = prChnlList->aucEntry;
		if (prChnlList->ucNumberOfEntry *
		    sizeof(struct _NAN_CHNL_ENTRY_NO_AUX_T) >
		    pucAvailEntry - pucChnl) {
			DBGLOG(NAN, WARN,
			       "Channel ucNumberOfEntry=%u too large (> %td)",
			       prChnlList->ucNumberOfEntry,
			       pucAvailEntry - pucChnl);
			continue;
		}

		prBandChnlList = (struct _NAN_SIMPLE_CHNL_ENTRY_T *)
			prChnlList->aucEntry;
		for (i = 0; i < prChnlList->ucNumberOfEntry; i++) {
			ucSupportedBands |= nanGetSubBandByChannelEntry(
						prAvailEntry,
						prChnlList,
						&prBandChnlList[i]);
		}
	} while (pucAvailEntry < pucAvailAttrEnd);

	return ucSupportedBands;
}

static u_int8_t
nanIsValidConditional(struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry,
		  struct _NAN_ATTR_NAN_AVAILABILITY_T *prAttrNanAvailibility)
{
	return (void *)prAvailEntry > (void *)prAttrNanAvailibility &&
	       (void *)prAvailEntry < NAN_ATTR_END(prAttrNanAvailibility) &&
	       *(uint16_t *)prAvailEntry < NAN_ATTR_SIZE(prAttrNanAvailibility);
}

static void nanParseNdcFromNextAttribute(struct ADAPTER *prAdapter,
				struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				enum _NAN_ACTION_T eNanAction,
				void *pNextAttr,
				struct _NAN_NDL_INSTANCE_T *prNDL)
{
	struct _NAN_ATTR_NDC_T *prNdcAttr = pNextAttr;
	struct _NAN_NDC_CTRL_T *prNdcCtrl;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	if (prNdcAttr->ucAttrId != NAN_ATTR_ID_NDC)
		return;

	if (!prNDL || prNDL->eNdcParseAction != NDC_NEED_PARSE_WITH_AVAIL)
		return;

	DBGLOG(NAN, TRACE, "Peek NDC attribute, action=%u", eNanAction);
	if (eNanAction != NAN_ACTION_SCHEDULE_CONFIRM)
		return;

	nanNDCAttrHandler(prAdapter, eNanAction, prNdcAttr, prNDL);

	prNDL->eNdcParseAction = NDC_PARSED_WITH_AVAIL;

	/**
	 * To set associated prPeerSchRecord->prCommNdcCtrl as
	 * done in nanSchedNegoChkRmtCrbProposal()
	 * to be copy and send to firmware in nanSchedCmdUpdateCRB().
	 *
	 * nanSchedPeerUpdateNdcAttr() has set
	 * prPeerSchDesc->rSelectedNdcCtrl.fgValid = TRUE;
	 * prNdcCtrl = nanSchedGetNdcCtrl(prAdapter, *prNdcAttr->aucNDCID);
	 *                    prNegoCtrl->rSelectedNdcCtrl.aucNdcId);
	 * prPeerSchRec->prCommNdcCtrl = prNdcCtrl;
	 */
	prNdcCtrl = nanSchedGetNdcCtrl(prAdapter, prNdcAttr->aucNDCID);
	if (!prNdcCtrl) {
		prNdcCtrl = nanSchedAcquireNdcCtrl(prAdapter);
		if (!prNdcCtrl) {
			DBGLOG(NAN, WARN, "Allocate NDC ctrl failed");
			return;
		}

		kalMemCopy(prNdcCtrl, &prPeerSchDesc->rSelectedNdcCtrl,
			   sizeof(struct _NAN_NDC_CTRL_T));
	}

	prPeerSchRec =
		nanSchedGetPeerSchRecord(prAdapter, prPeerSchDesc->u4SchIdx);
	if (!prPeerSchRec) {
		DBGLOG(NAN, WARN,
		       "Peer SchRec not found for %02x-%02x-%02x-%02x-%02x-%02x",
		       prNDL->aucPeerMacAddr[0], prNDL->aucPeerMacAddr[1],
		       prNDL->aucPeerMacAddr[2], prNDL->aucPeerMacAddr[3],
		       prNDL->aucPeerMacAddr[4], prNDL->aucPeerMacAddr[5]);
		return;
	}
	prPeerSchRec->prCommNdcCtrl = prNdcCtrl;

	DBGLOG(NAN, TRACE,
	       "Processed NDC with Availability, set sch %td, NDC=%02x-%02x-%02x-%02x-%02x-%02x",
	       prPeerSchRec - g_arNanPeerSchedRecord,
	       prNdcAttr->aucNDCID[0], prNdcAttr->aucNDCID[1],
	       prNdcAttr->aucNDCID[2], prNdcAttr->aucNDCID[3],
	       prNdcAttr->aucNDCID[4], prNdcAttr->aucNDCID[5]);
}

static void
nanInvalidateAvailabilityMaps(struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	struct _NAN_AVAILABILITY_DB_T *prNanAvailDB;
	uint32_t u4Idx;

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prNanAvailDB = &prPeerSchDesc->arAvailAttr[u4Idx];
		if (prNanAvailDB->ucMapId == NAN_NUM_AVAIL_DB)
			continue;

		prNanAvailDB->ucMapId = NAN_INVALID_MAP_ID;
		prNanAvailDB->u4AvailAttrToken = 0;
	}
}

uint32_t
nanSchedPeerUpdateAvailabilityAttr(struct ADAPTER *prAdapter,
				   enum _NAN_ACTION_T eNanAction,
				   uint8_t *pucNmiAddr,
				   uint8_t *pucAvailabilityAttr,
				   struct _NAN_NDP_INSTANCE_T *prNDP)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct _NAN_ATTR_NAN_AVAILABILITY_T *prAttrNanAvailibility;
	struct _NAN_ATTR_NAN_AVAILABILITY_T *prCondAttrNanAvailibility = NULL;
	struct _NAN_AVAILABILITY_DB_T *prNanAvailDB;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint16_t u2AttributeControl;
	uint8_t ucMapId;
	uint32_t u4EntryListPos;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4Token;
	u_int8_t fgFillByPotential = FALSE;
	uint8_t ucBackupNanMergePotentialThreshold = 0xFF;
	struct _NAN_AVAILABILITY_ENTRY_T *p2 = NULL;
	struct _NAN_AVAILABILITY_ENTRY_T *p6 = NULL;
	size_t new_size;
	struct _NAN_AVAILABILITY_ENTRY_SIMPLE_T r6gConditional = {
		.u2Length = 14,
		.u2EntryControl = 0x1204, /* Conditional, Time Bitmap Present */
		.u2TimeBitmapControl = 0x0018, /* Duration: 16TU, Period: 512 */
		.ucTimeBitmapLength = 4,
		/* To be filled by mergeCommittedPotentialTimeBitmap()
		 * in updateAvailability
		 */
		.aucTimeBitmap = {0, 0, 0, 0},

		/* OC and channel entries */
		.channelEntry.ucType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
		.channelEntry.ucNonContiguous = 0,
		.channelEntry.ucNumberOfEntry = 1,

		/* To be filled by scan result, ucOperatingClass as parameter */
		.channelEntry.rChnlEntry.ucOperatingClass = 134,
		.channelEntry.rChnlEntry.u2ChannelBitmap = 0,
		.channelEntry.rChnlEntry.ucPrimaryChnlBitmap = 0,
	};
	struct _NAN_AVAILABILITY_ENTRY_SIMPLE_T r2gConditional = {
		.u2Length = 14,
		.u2EntryControl = 0x1204, /* Conditional, Time Bitmap Present */
		.u2TimeBitmapControl = 0x0018, /* Duration: 16TU, Period: 512 */
		.ucTimeBitmapLength = 4,
		/* To be filled by mergeCommittedPotentialTimeBitmap()
		 * in updateAvailability
		 */
		.aucTimeBitmap = {0xfe, 0xff, 0xff, 0xff},

		/* OC and channel entries */
		.channelEntry.ucType = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
		.channelEntry.ucNonContiguous = 0,
		.channelEntry.ucNumberOfEntry = 1,

		/* To be filled by scan result, ucOperatingClass as parameter */
		.channelEntry.rChnlEntry.ucOperatingClass = 81,
		.channelEntry.rChnlEntry.u2ChannelBitmap = 0x2000,
		.channelEntry.rChnlEntry.ucPrimaryChnlBitmap = 0,
	};

	if (!pucAvailabilityAttr) {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto done;
	}

	prAttrNanAvailibility = (struct _NAN_ATTR_NAN_AVAILABILITY_T *)
		pucAvailabilityAttr;

	prPeerSchDesc = nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);
	if (!prPeerSchDesc) {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto done;
	}

	/* Invalidate all peer availability for new update */
	if (prPeerSchDesc->ucLastAvailabilitySequence !=
	    prAttrNanAvailibility->ucSeqID) {
		DBGLOG(NAN, TRACE, "Update Availability from %u to %u",
		       prPeerSchDesc->ucLastAvailabilitySequence,
		       prAttrNanAvailibility->ucSeqID);

		nanInvalidateAvailabilityMaps(prPeerSchDesc);

		prPeerSchDesc->ucLastAvailabilitySequence =
			prAttrNanAvailibility->ucSeqID;
	}

	u2AttributeControl = prAttrNanAvailibility->u2AttributeControl;
	ucMapId = u2AttributeControl & NAN_AVAIL_CTRL_MAPID;

	prNanAvailDB = nanSchedPeerAcquireAvailabilityDB(prAdapter,
					prPeerSchDesc, ucMapId);
	if (!prNanAvailDB) {
		DBGLOG(NAN, ERROR, "No Availability attribute record\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		goto done;
	}

	u4Token = nanUtilCalAttributeToken(
		(struct _NAN_ATTR_HDR_T *)pucAvailabilityAttr);
	if (prNanAvailDB->u4AvailAttrToken != 0 &&
	    prNanAvailDB->u4AvailAttrToken == u4Token)
		goto done;
	prNanAvailDB->u4AvailAttrToken = u4Token;

	DBGLOG(NAN, DEBUG, "\n");
	DBGLOG(NAN, DEBUG, "------>\n");
	nanUtilDump(prAdapter, "[Peer Avail]", pucAvailabilityAttr,
		    NAN_ATTR_SIZE(prAttrNanAvailibility));

	/* release old availability entries */
	for (u4EntryListPos = 0; u4EntryListPos < NAN_NUM_AVAIL_TIMELINE;
	     u4EntryListPos++) {
		prNanAvailEntry =
			&prNanAvailDB->arAvailEntryList[u4EntryListPos];
		prNanAvailEntry->fgActive = FALSE;
	}

	/* Check and optionally add conditional to 5G/6G availability */
#if (CFG_SUPPORT_NAN_11BE == 1)
	prPeerSchDesc->fgEht = FALSE;
#endif
	if (!nanGetFeatureIsSigma(prAdapter) &&
	    prNDP && prNDP->eCurrentNDPProtocolState == NDP_IDLE) {
		fgFillByPotential = TRUE;
	}

	prDataPathInfo = &prAdapter->rDataPathInfo;
	/* In Rescheduling, call from nanNdlParseAttributes w/o NDP */
	if (!prNDP && prDataPathInfo->ucNDLNum == 1 &&
	    eNanAction != NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION) {
		DBGLOG(NAN, INFO, "Force fgFillByPotential = TRUE");
		fgFillByPotential = TRUE;

		ucBackupNanMergePotentialThreshold =
			prWifiVar->ucNanMergePotentialThreshold;
		prWifiVar->ucNanMergePotentialThreshold =
			nanGetActiveTimelineMgmtNum(prAdapter) == 1 ? 14 : 17;
		DBGLOG(NAN, INFO, "Set Merge Potential Threshold = %u",
		       prWifiVar->ucNanMergePotentialThreshold);
	}

	/* Check and optionally add conditional to 2.4G availability */
	if (fgFillByPotential &&
	    (NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G) ||
	     nanGetHighestCommonBand(prAdapter, prPeerSchDesc, TRUE) ==
		    ENUM_SUPPORTED_BN_2G)) {
		prNDL = nanDataUtilSearchNdlByMac(prAdapter, pucNmiAddr);
		if (prNDL)
			DBGLOG(NAN, STATE,
			       "Attempt to add 2G conditional, established=%u, reschedule=%u\n",
			       prNDL->fgScheduleEstablished,
			       prNDL->fgTriggerReschedNewNDL);
		else
			DBGLOG(NAN, STATE, "Attempt to add 2G conditional");
		if (nanGetActiveTimelineMgmtNum(prAdapter) > 1 ||
		    (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
		     prNDL && prNDL->fgScheduleEstablished == FALSE))
			p2 = scanAvailabilityAttr(
				prAttrNanAvailibility, &r2gConditional);
	}
	if (p2 && nanIsValidConditional(p2, prAttrNanAvailibility)) {
		prCondAttrNanAvailibility =
			nanInsertConditionalAvailability(pucAvailabilityAttr,
						 &r2gConditional, p2);
		new_size = NAN_ATTR_SIZE(prAttrNanAvailibility) +
			sizeof(r2gConditional);
		if (prCondAttrNanAvailibility)
			prAttrNanAvailibility = prCondAttrNanAvailibility;
	}

	if (fgFillByPotential &&
	    !(NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G) ||
	      nanGetHighestCommonBand(prAdapter, prPeerSchDesc, TRUE) ==
		    ENUM_SUPPORTED_BN_2G)) {
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_NAN_6G == 1)
		if (getPeerSchDescMaxCap(prPeerSchDesc) == BAND_6G) {
			DBGLOG(NAN, STATE, "Attempt to add 6G conditional");
			p6 = scanAvailabilityAttr(prAttrNanAvailibility,
				&r6gConditional);
		}
#endif
	}
	if (p6 && nanIsValidConditional(p6, prAttrNanAvailibility)) {
		if (p2)
			DBGLOG(NAN, WARN,
			       "p2 and p6 exists in one Availability entry");
		prCondAttrNanAvailibility =
			nanInsertConditionalAvailability(pucAvailabilityAttr,
						 &r6gConditional, p6);
		new_size = NAN_ATTR_SIZE(prAttrNanAvailibility) +
			sizeof(r6gConditional);
		if (prCondAttrNanAvailibility)
			prAttrNanAvailibility = prCondAttrNanAvailibility;
	}

	if (updateAvailability(prAdapter, eNanAction, prPeerSchDesc,
			       prAttrNanAvailibility,
			       prNanAvailDB, fgFillByPotential)) {
		rRetStatus = WLAN_STATUS_PENDING;
	}
	if (prCondAttrNanAvailibility)
		kalMemFree(prCondAttrNanAvailibility, VIR_MEM_TYPE, new_size);


	/* Parse next atribute here for NDC schedule confirm to collect NDC for
	 * before calling nanSchedCmdUpdateCRB
	 */
	if (eNanAction == NAN_ACTION_SCHEDULE_CONFIRM) {
		prNDL = nanDataUtilSearchNdlByMac(prAdapter, pucNmiAddr);
		nanParseNdcFromNextAttribute(prAdapter, prPeerSchDesc,
					     eNanAction,
					     NAN_ATTR_END(pucAvailabilityAttr),
					     prNDL);
	}

	if (prPeerSchDesc->fgUsed) {
		nanSchedPeerUpdateCommonFAW(prAdapter, prPeerSchDesc->u4SchIdx);
		if (!nanSchedNegoInProgress(prAdapter))
			nanSchedPeerChkAvailability(prAdapter, prPeerSchDesc);
	}

	nanSchedChkPeerCommonBand(prAdapter, pucNmiAddr);
	nanSchedDbgDumpPeerAvailability(prAdapter, pucNmiAddr);
	DBGLOG(NAN, DEBUG, "<------\n");

	/* NAN_CHK_PNT log message */
	nanSchedDbgDumpPeerCommittedSlotAndChannel(prAdapter,
						   pucNmiAddr,
						   "peer_availability_attr_changed");

	if (ucBackupNanMergePotentialThreshold != 0xFF) {
		prWifiVar->ucNanMergePotentialThreshold =
			ucBackupNanMergePotentialThreshold;
		DBGLOG(NAN, INFO, "Restore Merge Potential Threshold = %u",
		       prWifiVar->ucNanMergePotentialThreshold);
	}
done:
	return rRetStatus;
}

/* Reflect to send Availability by peer availability if peer format is different
 * from local setting
 * See also: nanSetPeerAvailByCap when nanSchedPeerUpdateDevCapabilityAttr
 */
static u_int8_t nanReflectPeerAvailFormat(struct WIFI_VAR *prWifiVar,
				 enum _NAN_ACTION_T eNanAction,
				struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				uint16_t u2EntryControl,
				u_int8_t fgNanUseR4AvailAttr)
{
	/* Schedule update is RX only, don't follow it */
	if (eNanAction == NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION)
		return FALSE;

	/* Only follow Committed or Conditional */
	if (!(NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl) ||
	      NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl)))
		return FALSE;

	if (!prWifiVar->b1NanReflectPeerAvailabilityByAvail)
		return FALSE;

	/* Only set when the peer use different from our default format */
	if (fgNanUseR4AvailAttr && !prWifiVar->b1NanUseR4AvailAttr ||
	    !fgNanUseR4AvailAttr && prWifiVar->b1NanUseR4AvailAttr) {

		DBGLOG(NAN, INFO,
		       "Peer %02x:%02x:%02x:%02x:%02x:%02x use special Availability, R4=%u",
		       prPeerSchDesc->aucNmiAddr[0],
		       prPeerSchDesc->aucNmiAddr[1],
		       prPeerSchDesc->aucNmiAddr[2],
		       prPeerSchDesc->aucNmiAddr[3],
		       prPeerSchDesc->aucNmiAddr[4],
		       prPeerSchDesc->aucNmiAddr[5],
		       fgNanUseR4AvailAttr);
		return TRUE;
	}

	return FALSE;
}

void nanSchedChkPeerCommittedBand(struct ADAPTER *prAdapter,
			struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
			struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry,
			union _NAN_BAND_CHNL_CTRL rChnlInfo)
{
	uint32_t u4CommittedSlots = 0;
	uint32_t u4Bitmap = 0;
	uint32_t u4CommittedBand = 0;

	if (!prPeerSchDesc || !prNanAvailEntry) {
		DBGLOG(NAN, DEBUG, "PeerSchDesc or Attr NULL\n");
		return;
	}

	u4Bitmap = prNanAvailEntry->au4AvailMap[0];
	u4Bitmap &= NAN_SLOT_MASK_TYPE_DEFAULT_NDL;
	u4CommittedSlots = nanUtilCheckBitOneCnt(&u4Bitmap, sizeof(uint32_t));

	if (u4CommittedSlots <= 8) {
		DBGLOG(NAN, DEBUG,
			"AvailMap=0x%8x, Bitmap=0x%8x, CommittedSlots=%u\n",
			prNanAvailEntry->au4AvailMap[0],
			u4Bitmap,
			u4CommittedSlots);
		return;
	}

	if (NAN_IS_CHANNEL_2G(rChnlInfo))
		u4CommittedBand |= BIT(ENUM_SUPPORTED_BN_2G);
	else if (NAN_IS_CHANNEL_5G_LOW(rChnlInfo))
		u4CommittedBand |= BIT(ENUM_SUPPORTED_BN_5G_LOW);
	else if (NAN_IS_CHANNEL_5G_HIGH(rChnlInfo))
		u4CommittedBand |= BIT(ENUM_SUPPORTED_BN_5G_HIGH);
	else if (NAN_IS_CHANNEL_6G(rChnlInfo))
		u4CommittedBand |= BIT(ENUM_SUPPORTED_BN_6G);

	prPeerSchDesc->u4CommittedBand |= u4CommittedBand;
	DBGLOG(NAN, INFO,
		"AvailMmap=0x%8x, Slots=%u, CommittedBn=0x%x, PeerCommittedBn=0x%x\n",
		prNanAvailEntry->au4AvailMap[0],
		u4CommittedSlots,
		u4CommittedBand,
		prPeerSchDesc->u4CommittedBand);
}

u_int8_t updateAvailability(struct ADAPTER *prAdapter,
		    enum _NAN_ACTION_T eNanAction,
		    struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
		    struct _NAN_ATTR_NAN_AVAILABILITY_T *prAttrNanAvailibility,
		    struct _NAN_AVAILABILITY_DB_T *prNanAvailDB,
		    u_int8_t fgFillByPotential)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t ucNeedCounter = 0;
	uint8_t *pNextAvailEntry;
	uint8_t *pucAvailEntryEndPos;
	struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry;
	struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T *prTimeBitmap;
	uint16_t u2EntryControl;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	uint32_t u4EntryListPos;
	uint16_t u2TimeBitmapControl;
	uint8_t ucTimeBitmapLength = 0;
	struct _NAN_BAND_CHNL_LIST_T *prAttrBandChnlList;
	unsigned char fgChnlType;
	uint8_t *pucBandChnlEntryList;
	unsigned char fgNonContinuousBw;
	uint32_t u4NumBandChnlEntries;
	uint8_t ucNumBandChnlCtrl;
	struct _NAN_DEVICE_CAPABILITY_T *prDevCap;
#if MERGE_POTENTIAL
	uint32_t *prDstAvailEntry = NULL;
	uint16_t u2DstTimeBitmapControl = 0;
	u_int8_t fgIsPoetntialCandidate = FALSE;
	uint8_t *pucCommitTimeBitmap;
	uint8_t aucCommitTimeBitmap[TYPICAL_BITMAP_LENGTH] = {0}; /* exclude */

	uint8_t aucZeroTimeBitmap[TYPICAL_BITMAP_LENGTH] = {0};
	uint8_t *pucBitmap = aucZeroTimeBitmap;
	uint8_t aucCommitConditionalBitmap[TYPICAL_BITMAP_LENGTH] = {0};

	/* selected target bitmap, C or c */
	uint8_t *pucCommitCondTimeBitmap = NULL;
	/* selected target channel, C or c */
	uint8_t ucCommCondPriChnl = 0;
	uint8_t ucPotentPriChnl = 0;
	uint8_t aucMergedBitmap[TYPICAL_BITMAP_LENGTH] = {0};
	u_int8_t fgFillPotentialToConditional = FALSE;
	/* To compare with conditional */
	uint8_t ucCommittedOpClass = 0;
#endif
	uint8_t fgNanUseR4AvailAttrBackup = g_fgNanUseR4AvailAttr;
	enum NAN_RX_PEER_SPECIAL_AVAIL *pePeerForceAvailAttr =
		&prPeerSchDesc->ePeerForceAvailAttr;

	u4EntryListPos = 0;

	pNextAvailEntry = prAttrNanAvailibility->aucAvailabilityEntryList;
	pucAvailEntryEndPos = NAN_ATTR_END(prAttrNanAvailibility);

	do {
		prAvailEntry =
			(struct _NAN_AVAILABILITY_ENTRY_T *)pNextAvailEntry;

		pNextAvailEntry = NAN_AVAIL_ENTRY_END(prAvailEntry);

		u2EntryControl = prAvailEntry->u2EntryControl;
#if MERGE_POTENTIAL
		fgIsPoetntialCandidate = FALSE;
#endif

		prNanAvailEntry =
			&prNanAvailDB->arAvailEntryList[u4EntryListPos];
		prNanAvailEntry->fgActive = TRUE;
		prNanAvailEntry->ucNumBandChnlCtrl = 0;
		prNanAvailEntry->rEntryCtrl.u2RawData = 0;
		kalMemZero(prNanAvailEntry->au4AvailMap,
			   sizeof(prNanAvailEntry->au4AvailMap));

		DBGLOG(NAN, DEBUG,
		       "[%d] Entry Control:0x%04x (Type:%u C:%u/p:%u/c:%u, Pref=%u, Util=%u, NSS=%u, TBITMAP=%u)\n",
		       u4EntryListPos, u2EntryControl,
		       NAN_AVAIL_ENTRY_CTRL_TYPE(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_POTENTIAL(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_P(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_U(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_NSS(u2EntryControl),
		       NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(u2EntryControl));

		prNanAvailEntry->rEntryCtrl.u2RawData = u2EntryControl;

		/**
		 * prAvailEntry->aucTimeBitmapAndBandChnlEntry:
		 * Time Bitmap Control (2):
		 * Time Bitmap Length (1): ucTimeBitmapLength
		 * Time Bitmap (*):
		 * Band/Chanlel Entries (*): prAttrBandChnlList
		 *
		 * pucBitmap:
		 *     (following Time Bitmap Control and Time Bitmap Length))
		 * Time Bitmap (*):
		 * Band/Chanlel Entries (*): prAttrBandChnlList
		 */

		if (NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(u2EntryControl)) {
			prTimeBitmap =
				(struct _NAN_AVAILABILITY_TIMEBITMAP_ENTRY_T *)
				prAvailEntry;
			u2TimeBitmapControl = prTimeBitmap->u2TimeBitmapControl;
			ucTimeBitmapLength = prTimeBitmap->ucTimeBitmapLength;
			if (ucTimeBitmapLength > pNextAvailEntry -
			    prTimeBitmap->aucTimeBitmapAndBandChnl) {
				DBGLOG(NAN, WARN,
				       "TimeBitmapLength=%u too large",
				       ucTimeBitmapLength);
				continue;
			}

			prAttrBandChnlList = (struct _NAN_BAND_CHNL_LIST_T *)
				(prTimeBitmap->aucTimeBitmapAndBandChnl +
				 ucTimeBitmapLength);
			nanParserInterpretTimeBitmapField(prAdapter,
				u2TimeBitmapControl, ucTimeBitmapLength,
				prTimeBitmap->aucTimeBitmapAndBandChnl,
				prNanAvailEntry->au4AvailMap);

			pucBitmap = prTimeBitmap->aucTimeBitmapAndBandChnl;
#if MERGE_POTENTIAL
			u2DstTimeBitmapControl = u2TimeBitmapControl;

			/* To count Committed/conditional slots */
			if (ucTimeBitmapLength == TYPICAL_BITMAP_LENGTH &&
			   (NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl) ||
			    NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl))) {
				uint32_t i;

				for (i = 0; i < TYPICAL_BITMAP_LENGTH; i++) {
					aucCommitConditionalBitmap[i] |=
						pucBitmap[i];
				}
			}

			/* Committed, too few bits, mark prDstAvailEntry */
			if (fgFillByPotential &&
			    isCommittedInsufficient(prAdapter,
					u2EntryControl,
					pucBitmap,
					ucTimeBitmapLength,
					prAttrBandChnlList)) {
				u2DstTimeBitmapControl = u2TimeBitmapControl;
				prDstAvailEntry = prNanAvailEntry->au4AvailMap;
				pucCommitCondTimeBitmap = pucBitmap;

				/* reset for 2nd Committed */
				DBGLOG(NAN, DEBUG,
				       "reset ucCommCondPriChnl %u to 0\n",
				       ucCommCondPriChnl);
				ucCommCondPriChnl = 0;
			}

			/* All Committed shall not be filled from potential */
			if (NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl) &&
			    ucTimeBitmapLength == TYPICAL_BITMAP_LENGTH) {
				uint32_t i;

				pucCommitTimeBitmap = pucBitmap;

				for (i = 0; i < TYPICAL_BITMAP_LENGTH; i++) {
					aucCommitTimeBitmap[i] |=
						pucCommitTimeBitmap[i];
				}
			}

			/* Conditional is better (i.e., C=5G, c=6G) */
			if (!fgFillPotentialToConditional && prDstAvailEntry &&
			    IS_5G_OP_CLASS(ucCommittedOpClass) &&
			    isConditionalHigher(u2EntryControl, pucBitmap,
					ucTimeBitmapLength,
					prAttrBandChnlList)) {
				u2DstTimeBitmapControl = u2TimeBitmapControl;
				prDstAvailEntry = prNanAvailEntry->au4AvailMap;
				pucCommitCondTimeBitmap = pucBitmap;
				fgFillPotentialToConditional = TRUE;
				ucCommCondPriChnl = 0; /* reset, set later */
			}

			/* Assume Committed-Conditional-Potential */
			if (prDstAvailEntry) {
				fgIsPoetntialCandidate =
					isPotentialCandidate(u2EntryControl,
						pucBitmap,
						ucTimeBitmapLength);
			}
#endif
		} else { /* !NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(u2EntryControl) */
			/* all slots are available when timebitmap is not set */
			prAttrBandChnlList =
			    (struct _NAN_BAND_CHNL_LIST_T *)
			    (prAvailEntry->aucTimeBitmapAndBandChnlEntry);
			kalMemSet(prNanAvailEntry->au4AvailMap, 0xFF,
				  sizeof(prNanAvailEntry->au4AvailMap));
		}

		pucBandChnlEntryList = prAttrBandChnlList->aucEntry;
		fgChnlType = prAttrBandChnlList->ucType;
		fgNonContinuousBw = prAttrBandChnlList->ucNonContiguous;
		u4NumBandChnlEntries = prAttrBandChnlList->ucNumberOfEntry;

		if (fgChnlType == NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL) {
			const uint32_t u4Default2gChnl =
				g_r2gDwChnl.u4PrimaryChnl;
			const uint32_t u4Default5gChnl =
				g_r5gDwChnl.u4PrimaryChnl;
			union _NAN_BAND_CHNL_CTRL dw2gChnl = {0};
			union _NAN_BAND_CHNL_CTRL dw5gChnl = {0};
			union _NAN_BAND_CHNL_CTRL
				miscChnl[NAN_NUM_BAND_CHNL_ENTRY] = {0};
			union _NAN_BAND_CHNL_CTRL *prTmpChnl;
			uint32_t i = 0;

			u4NumBandChnlEntries = kal_min_t(uint32_t,
				u4NumBandChnlEntries, NAN_NUM_BAND_CHNL_ENTRY);

			ucNumBandChnlCtrl = 0;
			while (u4NumBandChnlEntries) {
				struct _NAN_CHNL_ENTRY_T *prAttrChnlEntry;
				uint8_t ucPriChnl;
				uint16_t *pu2ChannelBitmap;
				uint8_t ucOperatingClass;
				uint8_t ucPrimaryChnlBitmap;
				uint16_t *pu2AuxChannelBitmap;

				prAttrChnlEntry = (struct _NAN_CHNL_ENTRY_T *)
					pucBandChnlEntryList;

				ucOperatingClass =
					prAttrChnlEntry->ucOperatingClass;
				pu2ChannelBitmap =
					&prAttrChnlEntry->u2ChannelBitmap;
				ucPrimaryChnlBitmap =
					prAttrChnlEntry->ucPrimaryChnlBitmap;
				pu2AuxChannelBitmap =
					&prAttrChnlEntry->u2AuxChannelBitmap;

				if (IS_6G_OP_CLASS(ucOperatingClass)) {
					uint32_t i;

					prDevCap =
						prPeerSchDesc->arDevCapability;
					for (i = 0; i < NAN_NUM_AVAIL_DB + 1;
					     i++) {
						/* only consider valid ones */
						prDevCap[i].ucSupportedBand |=
						      NAN_SUPPORTED_6G_BIT *
						      prDevCap[i].fgValid;
					}
					if (nanIsR4Avail(u2EntryControl,
							 ucOperatingClass,
							 pu2ChannelBitmap))
						g_fgNanUseR4AvailAttr = TRUE;
					else
						g_fgNanUseR4AvailAttr = FALSE;

					/* Peer uses different Avail format */
					if (nanReflectPeerAvailFormat(prWifiVar,
						eNanAction,
						prPeerSchDesc,
						u2EntryControl,
						g_fgNanUseR4AvailAttr)) {
						/* Update ePeerForceAvailAttr */
						*pePeerForceAvailAttr =
						      NAN_PEER_AVAIL_FORCE_R3 +
						      g_fgNanUseR4AvailAttr;
					}
				}

				/* only select one channel from Channel
				 * Bitmap in the Channel Entry
				 */
				ucPriChnl = nanSchedChooseBestFromChnlBitmap(
					      prAdapter, ucOperatingClass,
					      /* NOTE: data will be updated */
					      &prAttrChnlEntry->u2ChannelBitmap,
					      fgNonContinuousBw,
					      ucPrimaryChnlBitmap,
					      pucBitmap);
				prTmpChnl = &prNanAvailEntry->arBandChnlCtrl
					[ucNumBandChnlCtrl];
				prTmpChnl->u4Type =
					NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
				prTmpChnl->u4OperatingClass =
					ucOperatingClass;
				prTmpChnl->u4PrimaryChnl = ucPriChnl;

				if (fgNonContinuousBw &&
				    (nanRegGetBw(ucOperatingClass) == 160 ||
				     nanRegGetBw(ucOperatingClass) == 320)) {
					prTmpChnl->u4AuxCenterChnl =
						nanRegGetChannelByOrder(
							ucOperatingClass,
							pu2AuxChannelBitmap);
					pucBandChnlEntryList +=
					       sizeof(struct _NAN_CHNL_ENTRY_T);
				} else {
					prTmpChnl->u4AuxCenterChnl = 0;
					pucBandChnlEntryList +=
					       sizeof(struct _NAN_CHNL_ENTRY_T)
							- 2;
				}

#if (CFG_SUPPORT_NAN_11BE == 1)
				if (nanRegGetBw(ucOperatingClass) == 320) {
					prPeerSchDesc->fgEht = TRUE;
					DBGLOG(NAN, TRACE,
					       "Update Peer EHT: %u\n",
					       prPeerSchDesc->fgEht);
				}
#endif

				if (ucPriChnl == 0) {
					u4NumBandChnlEntries--;
					continue;
				}

				if (ucPriChnl == u4Default2gChnl)
					dw2gChnl = *prTmpChnl;
				else if (ucPriChnl == u4Default5gChnl)
					dw5gChnl = *prTmpChnl;
				else
					miscChnl[i++] = *prTmpChnl;

#if MERGE_POTENTIAL
				/**
				 * Store destination channel in
				 * ucCommCondPriChnl at the first time
				 * the prDstAvailEntry was set
				 */
				if (NAN_AVAIL_ENTRY_CTRL_COMMITTED(
							u2EntryControl) &&
				    prDstAvailEntry && !ucCommCondPriChnl) {
					ucCommCondPriChnl = ucPriChnl;
					ucCommittedOpClass = ucOperatingClass;
				}

				/* Conditional overwrite Committed, only once */
				if (NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(
							u2EntryControl) &&
				    fgFillPotentialToConditional &&
				    !ucCommCondPriChnl)
					ucCommCondPriChnl = ucPriChnl;

				/* While traversing potential channels */
				if (ucCommCondPriChnl &&
				    fgIsPoetntialCandidate &&
				    ucPriChnl == ucCommCondPriChnl) {
					ucPotentPriChnl = ucPriChnl;
				}
#endif
				DBGLOG(NAN, DEBUG,
				       "Ch:%d, Bw:%d, OpClass:%d\n", ucPriChnl,
				       nanRegGetBw(ucOperatingClass),
				       ucOperatingClass);

				if (NAN_AVAIL_ENTRY_CTRL_COMMITTED(
				    u2EntryControl))
					nanSchedChkPeerCommittedBand(prAdapter,
						prPeerSchDesc,
						prNanAvailEntry,
						prNanAvailEntry->arBandChnlCtrl
						[ucNumBandChnlCtrl]);

				u4NumBandChnlEntries--;
				ucNumBandChnlCtrl++;
			}
			prNanAvailEntry->ucNumBandChnlCtrl = ucNumBandChnlCtrl;
			setBandChnlByPref(dw2gChnl, dw5gChnl, miscChnl,
				prNanAvailEntry->arBandChnlCtrl,
				ucNumBandChnlCtrl);

#if MERGE_POTENTIAL

			if (nanUtilCheckBitOneCnt(aucCommitConditionalBitmap,
						  TYPICAL_BITMAP_LENGTH) <
				prWifiVar->ucNanMergePotentialThreshold &&
			    prDstAvailEntry && ucPotentPriChnl) {
				/* multiple potential availability attributes */
				if (pucCommitCondTimeBitmap)
					ucNeedCounter |=
					mergeCommittedPotentialTimeBitmap(
					       ucPotentPriChnl,
					       pucCommitCondTimeBitmap,
					       pucBitmap,
					       /* exlclude */
					       aucCommitTimeBitmap,
					       aucMergedBitmap);
				if (NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(
				    u2EntryControl))
					nanParserInterpretTimeBitmapField(
						prAdapter,
						u2DstTimeBitmapControl,
						ucTimeBitmapLength,
						aucMergedBitmap,
						prDstAvailEntry);
				ucPotentPriChnl = 0;
				fgIsPoetntialCandidate = FALSE;
			}
#endif

		} else { /* NAN_BAND_CH_ENTRY_LIST_TYPE_BAND */
			union _NAN_BAND_CHNL_CTRL *prBandCtrl =
				&prNanAvailEntry->arBandChnlCtrl[0];

			prNanAvailEntry->ucNumBandChnlCtrl = 1;
			prBandCtrl->u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_BAND;
			prBandCtrl->u4BandIdMask = 0;

			while (u4NumBandChnlEntries) {
				prBandCtrl->u4BandIdMask |=
					BIT(*pucBandChnlEntryList);

				pucBandChnlEntryList++;
				u4NumBandChnlEntries--;
			}
#if MERGE_POTENTIAL
			if (nanUtilCheckBitOneCnt(aucCommitConditionalBitmap,
						  TYPICAL_BITMAP_LENGTH) <
				prWifiVar->ucNanMergePotentialThreshold &&
			    prDstAvailEntry &&
			    prBandCtrl->u4BandIdMask &
				    BIT(NAN_SUPPORTED_BAND_ID_5G) &&
			    IS_5G_OP_CLASS(ucCommittedOpClass)) {
				DBGLOG(NAN, DEBUG,
				       "Merge by band ch=%u, b=0x%02x",
				       ucCommCondPriChnl,
				       prBandCtrl->u4BandIdMask);

				if (pucCommitCondTimeBitmap)
					ucNeedCounter |=
					mergeCommittedPotentialTimeBitmap(
					       ucCommCondPriChnl,
					       pucCommitCondTimeBitmap,
					       pucBitmap,
					       /* exlclude */
					       aucCommitTimeBitmap,
					       aucMergedBitmap);
				if (NAN_AVAIL_ENTRY_CTRL_TBITMAP_P(
				    u2EntryControl))
					nanParserInterpretTimeBitmapField(
						prAdapter,
						u2DstTimeBitmapControl,
						ucTimeBitmapLength,
						aucMergedBitmap,
						prDstAvailEntry);
				ucPotentPriChnl = 0;
				fgIsPoetntialCandidate = FALSE;
			}
#endif

		}

		u4EntryListPos++;
	} while (pNextAvailEntry < pucAvailEntryEndPos &&
		 u4EntryListPos < NAN_NUM_AVAIL_TIMELINE);

#if MERGE_POTENTIAL
	if (ucNeedCounter && !fgFillPotentialToConditional) {
		prPeerSchDesc->u4MergedCommittedChannel = ucCommCondPriChnl;
		kalMemCopy(prPeerSchDesc->aucPotMergedBitmap,
			   aucMergedBitmap, TYPICAL_BITMAP_LENGTH);
	}
#endif

	DBGLOG(NAN, TRACE, "Restore backed up Use R4=%u",
	       fgNanUseR4AvailAttrBackup);
	g_fgNanUseR4AvailAttr = fgNanUseR4AvailAttrBackup;
	return !!ucNeedCounter;
}

/* Set to send Availability according to the position of peer 6G capability bit,
 * if peer format is different from local setting
 * See also: nanReflectPeerAvailFormat
 */
static u_int8_t nanSetPeerAvailByCap(struct ADAPTER *prAdapter,
		struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
		struct _NAN_ATTR_DEVICE_CAPABILITY_T *prAttrDevCapability)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (!prWifiVar->b1NanReflectPeerAvailabilityByCap)
		return FALSE;

	if (!(prAttrDevCapability->ucSupportedBands &
	      (NAN_PROPRIETARY_6G_BIT | NAN_SUPPORTED_6G_BIT)))
		return FALSE;

	/* Only set when the peer use different bit from our default format */
	if (prAttrDevCapability->ucSupportedBands & NAN_PROPRIETARY_6G_BIT &&
	    !prWifiVar->b1NanUseR4AvailAttr ||
	    prAttrDevCapability->ucSupportedBands & NAN_SUPPORTED_6G_BIT &&
	    prWifiVar->b1NanUseR4AvailAttr)
		return FALSE;

	DBGLOG(NAN, INFO,
	       "Peer %02x:%02x:%02x:%02x:%02x:%02x use special 6G Capability=%02x",
	       prPeerSchDesc->aucNmiAddr[0],
	       prPeerSchDesc->aucNmiAddr[1],
	       prPeerSchDesc->aucNmiAddr[2],
	       prPeerSchDesc->aucNmiAddr[3],
	       prPeerSchDesc->aucNmiAddr[4],
	       prPeerSchDesc->aucNmiAddr[5],
	       prAttrDevCapability->ucSupportedBands);

	return TRUE;
}

uint32_t
nanSchedPeerUpdateDevCapabilityAttr(struct ADAPTER *prAdapter,
				    uint8_t *pucNmiAddr,
				    uint8_t *pucDevCapabilityAttr) {
	struct _NAN_ATTR_DEVICE_CAPABILITY_T *prAttrDevCapability;
	struct _NAN_DEVICE_CAPABILITY_T *prNanDevCapability;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4AvailabilityDbIdx;
	uint8_t ucMapID;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4Token;

	do {
		prAttrDevCapability = (struct _NAN_ATTR_DEVICE_CAPABILITY_T *)
			pucDevCapabilityAttr;

		prPeerSchDesc =
			nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		u4Token = nanUtilCalAttributeToken(
			(struct _NAN_ATTR_HDR_T *)pucDevCapabilityAttr);
		if ((prPeerSchDesc->u4DevCapAttrToken != 0) &&
		    (prPeerSchDesc->u4DevCapAttrToken == u4Token))
			break;
		prPeerSchDesc->u4DevCapAttrToken = u4Token;

		DBGLOG(NAN, DEBUG, "\n\n");
		DBGLOG(NAN, DEBUG, "------>\n");
		nanUtilDump(prAdapter, "[Peer DevCap]", pucDevCapabilityAttr,
			    NAN_ATTR_SIZE(prAttrDevCapability));

		if (prAttrDevCapability->ucMapID & BIT(0)) {
			ucMapID = (prAttrDevCapability->ucMapID & BITS(1, 4)) >>
				  1;

			u4AvailabilityDbIdx = nanSchedPeerGetAvailabilityDbIdx(
				prAdapter, prPeerSchDesc, ucMapID);
			if (u4AvailabilityDbIdx == NAN_INVALID_AVAIL_DB_INDEX) {
				rRetStatus = WLAN_STATUS_FAILURE;
				break;
			}
		} else {
			ucMapID = NAN_INVALID_MAP_ID;
			/* for global device capability */
			u4AvailabilityDbIdx = NAN_NUM_AVAIL_DB;
		}

		prNanDevCapability =
			&prPeerSchDesc->arDevCapability[u4AvailabilityDbIdx];
		prNanDevCapability->fgValid = TRUE;
		prNanDevCapability->ucMapId = ucMapID;

		prNanDevCapability->ucCapabilitySet =
			prAttrDevCapability->ucCapabilities;
		prNanDevCapability->u2MaxChnlSwitchTime =
			prAttrDevCapability->u2MaxChannelSwitchTime;
		prNanDevCapability->ucSupportedBand |=
			prAttrDevCapability->ucSupportedBands;
		if (prAttrDevCapability->ucSupportedBands &
		    NAN_PROPRIETARY_6G_BIT) {
			prNanDevCapability->ucSupportedBand |=
				NAN_SUPPORTED_6G_BIT;
		}

		if (nanSetPeerAvailByCap(prAdapter, prPeerSchDesc,
					 prAttrDevCapability)) {
			enum NAN_RX_PEER_SPECIAL_AVAIL *pePeerForceAvailAttr;

			pePeerForceAvailAttr =
				&prPeerSchDesc->ePeerForceAvailAttr;

			if (prAttrDevCapability->ucSupportedBands &
			    NAN_PROPRIETARY_6G_BIT)
				*pePeerForceAvailAttr = NAN_PEER_AVAIL_FORCE_R3;
			else if (prAttrDevCapability->ucSupportedBands &
			    NAN_SUPPORTED_6G_BIT)
				*pePeerForceAvailAttr = NAN_PEER_AVAIL_FORCE_R4;
		}

		prNanDevCapability->ucOperationMode =
			prAttrDevCapability->ucOperationMode;
		prNanDevCapability->ucNumRxAnt =
			prAttrDevCapability->ucNumOfAntennasRx;
		prNanDevCapability->ucNumTxAnt =
			prAttrDevCapability->ucNumOfAntennasTx;
		prNanDevCapability->ucDw24g =
			prAttrDevCapability->u2CommittedDw2g;
		prNanDevCapability->ucDw5g =
			prAttrDevCapability->u2CommittedDw5g;
		prNanDevCapability->ucOvrDw24gMapId =
			prAttrDevCapability->u2CommittedDw2gOverwrite;
		prNanDevCapability->ucOvrDw5gMapId =
			prAttrDevCapability->u2CommittedDw5gOverwrite;

		DBGLOG(NAN, DEBUG,
		       "[Dev Capability] NMI=>%02x:%02x:%02x:%02x:%02x:%02x\n",
		       prPeerSchDesc->aucNmiAddr[0],
		       prPeerSchDesc->aucNmiAddr[1],
		       prPeerSchDesc->aucNmiAddr[2],
		       prPeerSchDesc->aucNmiAddr[3],
		       prPeerSchDesc->aucNmiAddr[4],
		       prPeerSchDesc->aucNmiAddr[5]);
		DBGLOG(NAN, DEBUG, "MapID:%d\n", prNanDevCapability->ucMapId);
		DBGLOG(NAN, DEBUG, "Capability Set:0x%02x\n",
		       prNanDevCapability->ucCapabilitySet);
		DBGLOG(NAN, DEBUG, "Max Chnl Switch Time:%d\n",
		       prNanDevCapability->u2MaxChnlSwitchTime);
		DBGLOG(NAN, DEBUG,
		       "Supported Band:0x%02x (2G:%u, 5G:%u, 6G:%u,%u)",
		       prNanDevCapability->ucSupportedBand,
		       !!(prNanDevCapability->ucSupportedBand &
				NAN_SUPPORTED_2G_BIT),
		       !!(prNanDevCapability->ucSupportedBand &
				NAN_SUPPORTED_5G_BIT),
		       !!(prNanDevCapability->ucSupportedBand &
				NAN_PROPRIETARY_6G_BIT),
		       !!(prNanDevCapability->ucSupportedBand &
				NAN_SUPPORTED_6G_BIT));
		DBGLOG(NAN, DEBUG,
		       "Operation Mode:0x%02x (VHT=%u, HE=%u, 80+80=%u, 160=%u)\n",
		       prNanDevCapability->ucOperationMode,
		       prNanDevCapability->ucOperPhyModeVht,
		       prNanDevCapability->ucOperPhyModeHe,
		       prNanDevCapability->ucOperPhyModeBw_80_80,
		       prNanDevCapability->ucOperPhyModeBw_160);
		DBGLOG(NAN, DEBUG, "Rx Ant:%d\n",
		       prNanDevCapability->ucNumRxAnt);
		DBGLOG(NAN, DEBUG, "Tx Ant:%d\n",
		       prNanDevCapability->ucNumTxAnt);
		DBGLOG(NAN, DEBUG, "DW 2.4G:%d\n", prNanDevCapability->ucDw24g);
		DBGLOG(NAN, DEBUG, "DW 5G:%d\n", prNanDevCapability->ucDw5g);

		if (prPeerSchDesc->fgUsed)
			nanSchedCmdUpdatePeerCapability(
				prAdapter, prPeerSchDesc->u4SchIdx);
		DBGLOG(NAN, DEBUG, "<------\n");
	} while (FALSE);

	return rRetStatus;
}

uint32_t
nanSchedPeerUpdateQosAttr(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			  uint8_t *pucQosAttr)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_ATTR_NDL_QOS_T *prNdlQosAttr;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	DBGLOG(NAN, DEBUG, "\n\n");
	DBGLOG(NAN, DEBUG, "------>\n");

	do {
		prPeerSchDesc =
			nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		prNdlQosAttr = (struct _NAN_ATTR_NDL_QOS_T *)pucQosAttr;

		if (prNdlQosAttr->u2MaxLatency < prPeerSchDesc->u4QosMaxLatency)
			prPeerSchDesc->u4QosMaxLatency =
				prNdlQosAttr->u2MaxLatency;

		if (prNdlQosAttr->ucMinTimeSlot > prPeerSchDesc->u4QosMinSlots)
			prPeerSchDesc->u4QosMinSlots =
				prNdlQosAttr->ucMinTimeSlot;

		DBGLOG(NAN, DEBUG, "MinSlot:%d, MaxLatency:%d\n",
		       prPeerSchDesc->u4QosMinSlots,
		       prPeerSchDesc->u4QosMaxLatency);
	} while (FALSE);

	DBGLOG(NAN, DEBUG, "<------\n");

	return rRetStatus;
}

uint32_t
nanSchedPeerUpdateNdcAttr(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			  uint8_t *pucNdcAttr)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4ScheduleEntryListLength;
	struct _NAN_ATTR_NDC_T *prAttrNdc;
	struct _NAN_NDC_CTRL_T *prNanNdcCtrl;
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntryList;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntry;
	struct _NAN_ATTR_TIME_BITMAP_CONTROL_T *prTimeBitmapCtrl;

	do {
		prPeerSchDesc =
			nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		prAttrNdc = (struct _NAN_ATTR_NDC_T *)pucNdcAttr;
		prScheduleEntryList = (struct _NAN_SCHEDULE_ENTRY_T *)
					      prAttrNdc->aucScheduleEntryList;
		u4ScheduleEntryListLength =
			prAttrNdc->u2Length -
			7 /* NdcID(6)+AttributeControl(1) */;

		/** Only handle selected NDC for CRB negotiation */
		if (!(prAttrNdc->ucAttributeControl &
		      NAN_ATTR_NDC_CTRL_SELECTED_FOR_NDL)) {
			DBGLOG(NAN, DEBUG, "Not Selected NDC\n");
			break;
		}

		prScheduleEntry = (struct _NAN_SCHEDULE_ENTRY_T *)
					prAttrNdc->aucScheduleEntryList;
		prTimeBitmapCtrl = (struct _NAN_ATTR_TIME_BITMAP_CONTROL_T *)
					&prScheduleEntry->u2TimeBitmapControl;
		if (NAN_ATTR_SIZE(prAttrNdc) >=
		    sizeof(*prAttrNdc) + sizeof(*prScheduleEntry) +
		    sizeof(uint8_t) * 4) {
			DBGLOG(NAN, INFO,
			       "NDCID=%02x:%02x:%02x:%02x:%02x:%02x, Selected=%u, MapId=%u, Duration=%u, Period=%u, Offset=%u, TimeBitmapLen=%u, TimeBitmap=%02x-%02x-%02x-%02x",
			       prAttrNdc->aucNDCID[0], prAttrNdc->aucNDCID[1],
			       prAttrNdc->aucNDCID[2], prAttrNdc->aucNDCID[3],
			       prAttrNdc->aucNDCID[4], prAttrNdc->aucNDCID[5],
			       prAttrNdc->selected_ndc,
			       prScheduleEntry->ucMapID,
			       prTimeBitmapCtrl->u2BitDuration,
			       prTimeBitmapCtrl->u2Period,
			       prTimeBitmapCtrl->u2StartOffset,
			       prScheduleEntry->ucTimeBitmapLength,
			       prScheduleEntry->aucTimeBitmap[0],
			       prScheduleEntry->aucTimeBitmap[1],
			       prScheduleEntry->aucTimeBitmap[2],
			       prScheduleEntry->aucTimeBitmap[3]);
		} else {
			DBGLOG(NAN, INFO,
			       "NDCID=%02x:%02x:%02x:%02x:%02x:%02x, Selected=%u",
			       prAttrNdc->aucNDCID[0], prAttrNdc->aucNDCID[1],
			       prAttrNdc->aucNDCID[2], prAttrNdc->aucNDCID[3],
			       prAttrNdc->aucNDCID[4], prAttrNdc->aucNDCID[5],
			       prAttrNdc->selected_ndc);
		}

		prNanNdcCtrl = &prPeerSchDesc->rSelectedNdcCtrl;
		kalMemZero(prNanNdcCtrl, sizeof(struct _NAN_NDC_CTRL_T));

		if (nanSchedPeerInterpretScheduleEntryList(prAdapter,
				prPeerSchDesc,
				prNanNdcCtrl->arTimeline,
				prScheduleEntryList,
				u4ScheduleEntryListLength) ==
		    WLAN_STATUS_SUCCESS) {
			prNanNdcCtrl->fgValid = TRUE;
			kalMemCopy(prNanNdcCtrl->aucNdcId, prAttrNdc->aucNDCID,
				   NAN_NDC_ATTRIBUTE_ID_LENGTH);
		} else {
			prNanNdcCtrl->fgValid = FALSE;
		}
		DBGLOG(NAN, INFO, "Set sch %u NDC prNanNdcCtrl->fgValid=%u",
		       prPeerSchDesc->u4SchIdx, prNanNdcCtrl->fgValid);

	} while (FALSE);

	return rRetStatus;
}

uint32_t
nanSchedPeerUpdateUawAttr(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			  uint8_t *pucUawAttr)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_ATTR_UNALIGNED_SCHEDULE_T *prAttrUaw;

	void *prCmdBuffer = NULL;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint8_t *pucUawBuf;
	uint32_t u4UlwAttrSize;

	do {
		prPeerSchRecord =
			nanSchedLookupPeerSchRecord(prAdapter, pucNmiAddr);
		if (prPeerSchRecord == NULL)
			break;

		DBGLOG(NAN, DEBUG, "\n\n");
		DBGLOG(NAN, DEBUG, "------>\n");

		prAttrUaw = (struct _NAN_ATTR_UNALIGNED_SCHEDULE_T *)pucUawAttr;
		u4UlwAttrSize = OFFSET_OF(struct _NAN_ATTR_UNALIGNED_SCHEDULE_T,
					  u2AttributeControl) +
				prAttrUaw->u2Length;
		nanUtilDump(prAdapter, "[Peer UAW]", (uint8_t *)prAttrUaw,
			    u4UlwAttrSize);
		DBGLOG(NAN, DEBUG, "ULW: " MACSTR
		       " id=%u, len=%u, SchedId=%u SeqId=%u, start=%u, dur=%u(us), period=%u(us), count=%u, OWall=%u, OWmap=%u\n",
		       MAC2STR(pucNmiAddr),
		       prAttrUaw->ucAttrId, prAttrUaw->u2Length,
		       prAttrUaw->b4ScheduleId,
		       prAttrUaw->b8SequenceId,
		       prAttrUaw->u4StartingTime,
		       prAttrUaw->u4Duration,
		       prAttrUaw->u4Period,
		       prAttrUaw->ucCountDown,
		       prAttrUaw->b1OverwriteAll,
		       prAttrUaw->b4OverwriteMapId);

		u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
				 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
				 ALIGN_4(MAC_ADDR_LEN + u4UlwAttrSize);
		prCmdBuffer =
			cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
		if (!prCmdBuffer) {
			rStatus = WLAN_STATUS_FAILURE;
			break;
		}

		prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

		prTlvCommon->u2TotalElementNum = 0;

		rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_PEER_UAW,
					ALIGN_4(MAC_ADDR_LEN + u4UlwAttrSize),
					u4CmdBufferLen, prCmdBuffer);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
			rStatus = WLAN_STATUS_FAILURE;
			break;
		}

		prTlvElement = nicNanGetTargetTlvElement(
					prTlvCommon->u2TotalElementNum,
					prCmdBuffer);
		if (prTlvElement == NULL) {
			DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
			rStatus = WLAN_STATUS_FAILURE;
			break;
		}

		pucUawBuf = prTlvElement->aucbody;
		kalMemCopy(pucUawBuf, pucNmiAddr, MAC_ADDR_LEN);
		kalMemCopy(pucUawBuf + MAC_ADDR_LEN, pucUawAttr, u4UlwAttrSize);

		rStatus = wlanSendSetQueryCmd(
			prAdapter, CMD_ID_NAN_EXT_CMD, TRUE, FALSE, FALSE, NULL,
			nicCmdTimeoutCommon, u4CmdBufferLen,
			prCmdBuffer, NULL, 0);
		if (rStatus == WLAN_STATUS_PENDING)
			rStatus = WLAN_STATUS_SUCCESS;

		DBGLOG(NAN, DEBUG, "<------\n");
	} while (FALSE);

	if (prCmdBuffer)
		cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

/* [nanSchedPeerUpdateImmuNdlScheduleList]
 *
 *   Only allowed when it acquires the token to nego CRB.
 */
uint32_t
nanSchedPeerUpdateImmuNdlScheduleList(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntryList,
	uint32_t u4ScheduleEntryListLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	do {
		prPeerSchDesc =
			nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		nanUtilDump(prAdapter, "[Peer Immu NDL]",
			    (uint8_t *)prScheduleEntryList,
			    u4ScheduleEntryListLength);

		prPeerSchDesc->fgImmuNdlTimelineValid = FALSE;
		rRetStatus = nanSchedPeerInterpretScheduleEntryList(
			prAdapter, prPeerSchDesc,
			prPeerSchDesc->arImmuNdlTimeline,
			prScheduleEntryList,
			u4ScheduleEntryListLength);
		if (rRetStatus == WLAN_STATUS_SUCCESS) {
			prPeerSchDesc->fgImmuNdlTimelineValid = TRUE;

#ifdef NAN_UNUSED
{
			UINT_32 u4Idx = 0;

			for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
				nanUtilDump(prAdapter, "[Peer Immu NDL Bitmap]",
				(PUINT_8)prPeerSchDesc
				->arImmuNdlTimeline[u4Idx].au4AvailMap,
				sizeof(prPeerSchDesc->arImmuNdlTimeline[u4Idx].
				au4AvailMap));
			}
}
#endif
		}
	} while (FALSE);

	return rRetStatus;
}

/* [nanSchedPeerUpdateRangingScheduleList]
 *
 *   Only allowed when it acquires the token to nego CRB.
 */
uint32_t
nanSchedPeerUpdateRangingScheduleList(struct ADAPTER *prAdapter,
			uint8_t *pucNmiAddr,
			struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntryList,
			uint32_t u4ScheduleEntryListLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	do {
		prPeerSchDesc =
			nanSchedAcquirePeerSchDescByNmi(prAdapter, pucNmiAddr);
		if (prPeerSchDesc == NULL) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		/* nanUtilDump(prAdapter, "[Peer Ranging]",
		 *	    (uint8_t *)prScheduleEntryList,
		 *	    u4ScheduleEntryListLength);
		 */
		prPeerSchDesc->fgRangingTimelineValid = FALSE;
		rRetStatus = nanSchedPeerInterpretScheduleEntryList(prAdapter,
				prPeerSchDesc,
				prPeerSchDesc->arRangingTimeline,
				prScheduleEntryList,
				u4ScheduleEntryListLength);
		if (rRetStatus == WLAN_STATUS_SUCCESS) {
			prPeerSchDesc->fgRangingTimelineValid = TRUE;
		} else {
			DBGLOG(NAN, WARN,
			       "nanSchedPeerInterpretScheduleEntryList=0x%08x",
			       rRetStatus);
		}
	} while (FALSE);

	return rRetStatus;
}

static u_int8_t
nanCustomizedPeerNdp(struct _NAN_SCHEDULE_TIMELINE_T *prTimeline,
		     struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord,
		     enum NAN_BAND_IDX eBand, uint8_t ucOpChannel,
		     uint32_t u4SlotIdx)
{
	struct _NAN_NDL_CUSTOMIZED_T *prNdlCustomized;

	prNdlCustomized = &prPeerSchRecord->arCustomized[eBand];

	if (!prNdlCustomized->fgBitmapSet && !prNdlCustomized->ePowerSaving)
		return FALSE;

	NAN_DW_DBGLOG(NAN, LOUD, TRUE, u4SlotIdx,
		      "Peer check eBand=%u ucOpChannel=%u u4SlotIdx=%u, fgBitmapSet=%u, ucOpChannel=%u, u4Bitmap=%08x, SET=%u, PS=%u\n",
		      eBand, ucOpChannel, u4SlotIdx,
		      prNdlCustomized->fgBitmapSet,
		      prNdlCustomized->ucOpChannel, prNdlCustomized->u4Bitmap,
		      !!(prNdlCustomized->u4Bitmap & BIT(u4SlotIdx % 32)),
		      prNdlCustomized->ePowerSaving);

	if (prNdlCustomized->ePowerSaving == NDP_RESUME_FROM_PS)
		return FALSE;

	if (prNdlCustomized->ePowerSaving) /* Return TRUE to set 0 */
		return TRUE;

	if (prNdlCustomized->ucOpChannel != ucOpChannel)
		return FALSE;

	if (prNdlCustomized->u4Bitmap &
	    BIT(u4SlotIdx % NAN_SLOTS_PER_DW_INTERVAL)) {
		NAN_TIMELINE_SET(prTimeline->au4AvailMap, u4SlotIdx);
		NAN_DW_DBGLOG(NAN, TRACE, TRUE, u4SlotIdx,
			      "Set schidx=%u band=%u customized slot:%u\n",
			      prPeerSchRecord->prPeerSchDesc->u4SchIdx,
			      eBand, u4SlotIdx);
	}
	return TRUE;
}

static u_int8_t
nanCustomizedGlobalNdp(struct ADAPTER *prAdapter,
		       struct _NAN_SCHEDULE_TIMELINE_T *prTimeline,
		       enum NAN_BAND_IDX eBand, uint8_t ucOpChannel,
		       uint32_t u4SlotIdx)
{
	struct _NAN_NDL_CUSTOMIZED_T *prNdlCustomized;
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);
	if (!prScheduler)
		return FALSE;

	prNdlCustomized = &prScheduler->arGlobalCustomized[eBand];

	if (!prNdlCustomized->fgBitmapSet && !prNdlCustomized->ePowerSaving)
		return FALSE;

	NAN_DW_DBGLOG(NAN, LOUD, TRUE, u4SlotIdx,
		      "Global check eBand=%u ucOpChannel=%u u4SlotIdx=%u, fgBitmapSet=%u, ucOpChannel=%u, u4Bitmap=%08x, SET=%u, PS=%u\n",
		      eBand, ucOpChannel, u4SlotIdx,
		      prNdlCustomized->fgBitmapSet,
		      prNdlCustomized->ucOpChannel, prNdlCustomized->u4Bitmap,
		      !!(prNdlCustomized->u4Bitmap & BIT(u4SlotIdx % 32)),
		      prNdlCustomized->ePowerSaving);

	if (prNdlCustomized->ePowerSaving == NDP_RESUME_FROM_PS)
		return FALSE;

	if (prNdlCustomized->ePowerSaving) /* Return TRUE to set 0 */
		return TRUE;

	if (prNdlCustomized->ucOpChannel != ucOpChannel)
		return FALSE;

	if (prNdlCustomized->u4Bitmap & BIT(u4SlotIdx % 32)) {
		NAN_TIMELINE_SET(prTimeline->au4AvailMap, u4SlotIdx);
		NAN_DW_DBGLOG(NAN, TRACE, TRUE, u4SlotIdx,
			      "Global set band=%u, customized slot:%u\n",
			      eBand, u4SlotIdx);
	}
	return TRUE;
}

static u_int8_t
nanCustomizedNdp(struct ADAPTER *prAdapter,
		 struct _NAN_SCHEDULE_TIMELINE_T *prTimeline,
		 struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord,
		 u_int8_t *pfgCustomizedGlobal,
		 enum NAN_BAND_IDX eBand, uint8_t ucOpChannel,
		 uint32_t u4SlotIdx)
{
	if (nanCustomizedPeerNdp(prTimeline, prPeerSchRecord, eBand,
				 ucOpChannel, u4SlotIdx))
		return TRUE;

	if (nanCustomizedGlobalNdp(prAdapter, prTimeline, eBand,
				   ucOpChannel, u4SlotIdx)) {
		if (pfgCustomizedGlobal)
			*pfgCustomizedGlobal = TRUE;
		return TRUE;
	}

	return FALSE;
}

void nanUpdateStaRecIdx(struct ADAPTER *prAdapter,
			struct _NAN_NDL_INSTANCE_T *prNDL)
{
#if (CFG_SUPPORT_NAN_DBDC == 1)
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	enum NAN_BSS_ROLE_INDEX eRoleIdx = NAN_BSS_INDEX_BAND0;
	enum ENUM_BAND eBand;
	uint8_t (*pucStaRecIdx)[NAN_MAX_SUPPORT_NDP_CXT_NUM];
	uint8_t ucLink;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	struct _NAN_NDP_CONTEXT_T *prNdpCxt;
	struct STA_RECORD *prNanStaRec;
	size_t szNdpCxtIdx;
	size_t j;

	if (!prNDL)
		return;

	prPeerSchRecord = nanSchedLookupPeerSchRecord(prAdapter,
						      prNDL->aucPeerMacAddr);
	if (!prPeerSchRecord)
		return;

	eBand = prPeerSchRecord->eBand;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G)
		eRoleIdx = NAN_BSS_INDEX_BAND1;
#endif
	if (eBand == BAND_5G)
		eRoleIdx = NAN_BSS_INDEX_BAND1;

	ucLink = nanGetLinkIndexbyRole(eRoleIdx);

	pucStaRecIdx = prPeerSchRecord->aucStaRecIdx;


	for (szNdpCxtIdx = 0; szNdpCxtIdx < ARRAY_SIZE(prNDL->arNdpCxt);
	     szNdpCxtIdx++) {
		prNdpCxt = &prNDL->arNdpCxt[szNdpCxtIdx];
		if (!prNdpCxt->fgValid)
			return;

		for (j = 0; j < ARRAY_SIZE(prNdpCxt->aprEnrollNdp); j++) {
			prNDP = prNdpCxt->aprEnrollNdp[j];
			if (!prNDP)
				continue;

			if (!prNDP->fgNDPValid)
				continue;

			prNanStaRec = nanGetLinkStaRec(prNdpCxt, ucLink);
			nanSchedCmdMapStaRecord(prAdapter,
						prPeerSchRecord->aucNmiAddr,
						eRoleIdx,
						prNanStaRec->ucIndex,
						prNdpCxt->ucId,
						prNanStaRec->ucWlanIndex,
						prNDP->aucPeerNDIAddr);

			nanSetPreferLinkStaRec(prAdapter, prNdpCxt,
					nanGetBssIdxbyBand(prAdapter, eBand));
		}
	}
#endif
}

/**
 * Set prPeerSchRecord->arCommFawTimeline[ucTimeLineIdx].au4AvailMap timeslots
 * if ((nanSchedChkConcurrOp(rLocalChnlInfo, rRmtChnlInfo) != CNM_CH_CONCURR_MCC
 * where rLocalChnlInfo returns committed CRB from nanQueryChnlInfoBySlot()
 * with given hu4SlotIdx, ucTimeLineIdx.
 * rRmtChnlInfo returns committed CRB from  nanGetPeerChnlInfoBySlot()
 * with given u4SchIdx, u4AvailDbIdx, u4SlotIdx.
 */
void nanSchedPeerUpdateCommonFAW(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	uint32_t u4SlotIdx = 0;
	uint32_t u4AvailDbIdx = 0;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord = NULL;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline = NULL;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {0};
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {0};
	enum ENUM_BAND eBand = BAND_NULL;
	int32_t i4SlotNum[BAND_NUM - 1] = {0};
	uint8_t ucTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_NDC_CTRL_T *prCommNdcCtrl;
	size_t i;
	uint32_t u4FcSlots;

	DBGLOG(NAN, DEBUG, "Update common FAW idx=%u\n", u4SchIdx);

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRecord == NULL)
		return;

	if ((prPeerSchRecord->fgActive == FALSE) ||
	    ((prPeerSchRecord->fgUseDataPath == FALSE) &&
	     (prPeerSchRecord->fgUseRanging == FALSE)))
		return;

	/* have a preference for 5G band */
	for (ucTimeLineIdx = szNanActiveTimelineNum; ucTimeLineIdx--; ) {
		u4FcSlots = nanGetTimelineFcSlots(prAdapter, ucTimeLineIdx, 0);

		prTimeline = &prPeerSchRecord->arCommFawTimeline[ucTimeLineIdx];
		kalMemZero(prTimeline->au4AvailMap,
				sizeof(prTimeline->au4AvailMap));

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				ucTimeLineIdx);
		prTimeline->ucMapId = prNanTimelineMgmt->ucMapId;
		prTimeline->ucLocalMapId = prNanTimelineMgmt->ucMapId;

		for (u4SlotIdx = 0;
			u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
			if (nanIsDiscWindow(prAdapter, u4SlotIdx,
				ucTimeLineIdx))
				continue;

			rLocalChnlInfo =
				nanQueryChnlInfoBySlot(prAdapter, u4SlotIdx,
					NULL, TRUE, ucTimeLineIdx);

			if (rLocalChnlInfo.u4PrimaryChnl == 0)
				continue;

			eBand = nanRegGetNanChnlBand(rLocalChnlInfo);
			if (eBand == BAND_NULL)
				continue;

			NAN_DW_DBGLOG(NAN, LOUD, TRUE, u4SlotIdx,
				      "[LOCAL] Slot:%d -> Chnl:%d\n",
				      u4SlotIdx,
				      rLocalChnlInfo.u4PrimaryChnl);

			for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
			     u4AvailDbIdx++) {
				u_int8_t fgCustomizedGlobal = FALSE;

				if (nanSchedPeerAvailabilityDbValidByDesc(
					prAdapter,
					prPeerSchRecord->prPeerSchDesc,
					u4AvailDbIdx) == FALSE)
					continue;

#define USE_CUSTOMIZED_BITMAP 1
#if (USE_CUSTOMIZED_BITMAP == 1)
				/* Set according customized bitmap */
				if (nanCustomizedNdp(prAdapter, prTimeline,
					prPeerSchRecord,
					&fgCustomizedGlobal,
					(enum NAN_BAND_IDX)(eBand - 1),
					rLocalChnlInfo.u4PrimaryChnl,
					u4SlotIdx)) {

					NAN_DW_DBGLOG(NAN, TRACE,
						TRUE, u4SlotIdx,
						"Customized (%s) slot=%u ch=%u, set=%u",
						fgCustomizedGlobal ?
						"global" : "peer",
						u4SlotIdx,
						rLocalChnlInfo.u4PrimaryChnl,
						NAN_IS_AVAIL_MAP_SET(
							prTimeline->au4AvailMap,
							u4SlotIdx)
						);
					break; /* customized bitmap applied */
				}
#endif

				rRmtChnlInfo =
					nanGetPeerChnlInfoBySlot(prAdapter,
						u4SchIdx, u4AvailDbIdx,
						u4SlotIdx, TRUE);
				if (rRmtChnlInfo.u4PrimaryChnl == 0)
					continue;

				NAN_DW_DBGLOG(NAN, LOUD, TRUE, u4SlotIdx,
					   "[RMT][%d] Slot:%d -> Chnl:%d\n",
					   u4AvailDbIdx, u4SlotIdx,
					   rRmtChnlInfo.u4PrimaryChnl);

				if ((nanSchedChkConcurrOp(rLocalChnlInfo,
							 rRmtChnlInfo) !=
					CNM_CH_CONCURR_MCC)) {
					NAN_DW_DBGLOG(NAN, LOUD, TRUE,
						      u4SlotIdx,
						      "%u L:%08x R:%08x co-channel check pass\n",
						      u4SlotIdx,
						      rLocalChnlInfo.u4RawData,
						      rRmtChnlInfo.u4RawData);

					NAN_TIMELINE_SET(
							prTimeline->au4AvailMap,
							u4SlotIdx);

					/* Update used band to peer
					 * schedule record
					 */
					i4SlotNum[eBand - 1]++;
					break;
				}
			}
		}

		if (NAN_IS_2G_TIMELINE(prAdapter, ucTimeLineIdx) &&
		    (prTimeline->au4AvailMap[0] & ~u4FcSlots) == 0) {
			DBGLOG(NAN, INFO,
			       "Erase 2G from timeline %02x-%02x-%02x-%02x equals FC, Skip 2G FAW",
			       ((uint8_t *)prTimeline->au4AvailMap)[0],
			       ((uint8_t *)prTimeline->au4AvailMap)[1],
			       ((uint8_t *)prTimeline->au4AvailMap)[2],
			       ((uint8_t *)prTimeline->au4AvailMap)[3]);
			kalMemZero(prTimeline->au4AvailMap,
				   sizeof(prTimeline->au4AvailMap));
		}

		DBGLOG(NAN, INFO,
		       "Update FAW idx=%u, timeline=%u map=%02x-%02x-%02x-%02x\n",
		       u4SchIdx, ucTimeLineIdx,
		       ((uint8_t *)prTimeline->au4AvailMap)[0],
		       ((uint8_t *)prTimeline->au4AvailMap)[1],
		       ((uint8_t *)prTimeline->au4AvailMap)[2],
		       ((uint8_t *)prTimeline->au4AvailMap)[3]);
	}

	if (prPeerSchRecord->prCommNdcCtrl) {
		union _NAN_BAND_CHNL_CTRL rChnl;
		u_int8_t fgIsCommitted = FALSE;

		prCommNdcCtrl = prPeerSchRecord->prCommNdcCtrl;

		prTimeline = prCommNdcCtrl->arTimeline;
		for (i = 0; i < ARRAY_SIZE(prCommNdcCtrl->arTimeline); i++) {
			if (prTimeline[i].ucMapId == NAN_INVALID_MAP_ID)
				continue;

			prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
					ucTimeLineIdx);
			rChnl = nanGetChnlInfoByMapIdAndBitmap(prAdapter,
						prTimeline[i].ucMapId,
						prTimeline[i].au4AvailMap[0],
						&fgIsCommitted);
			DBGLOG(NAN, INFO,
			       "sch idx=%u, timeline=%zu, NDC=%02x:%02x:%02x:%02x:%02x:%02x MapID=%u, Avail=%02x-%02x-%02x-%02x, ch=%u(%c)",
			       u4SchIdx, i,
			       prCommNdcCtrl->aucNdcId[0],
			       prCommNdcCtrl->aucNdcId[1],
			       prCommNdcCtrl->aucNdcId[2],
			       prCommNdcCtrl->aucNdcId[3],
			       prCommNdcCtrl->aucNdcId[4],
			       prCommNdcCtrl->aucNdcId[5],
			       prTimeline[i].ucMapId,
			       prTimeline[i].aucAvailBlock[0],
			       prTimeline[i].aucAvailBlock[1],
			       prTimeline[i].aucAvailBlock[2],
			       prTimeline[i].aucAvailBlock[3],
			       rChnl.u4PrimaryChnl,
			       rChnl.u4PrimaryChnl ?
				       (fgIsCommitted ? 'C' : 'c') : ' ');
		}
	}

	/* Set eBand by collected slot number with higher band preferred */
	eBand = BAND_2G4;
	if (i4SlotNum[BAND_5G - 1])
		eBand = BAND_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (i4SlotNum[BAND_6G - 1])
		eBand = BAND_6G;
#endif

	if (prPeerSchRecord->eBand == BAND_NULL) {
		prPeerSchRecord->eBand = eBand;
		DBGLOG(NAN, DEBUG, "Peer %u use band %d(%d)\n",
		       u4SchIdx, prPeerSchRecord->eBand, i4SlotNum[eBand - 1]);
	} else if (prPeerSchRecord->eBand != eBand) {
		DBGLOG(NAN, ERROR, "Peer %u Band conflict %d != %d(%d)\n",
		       u4SchIdx, prPeerSchRecord->eBand, eBand,
		       i4SlotNum[eBand - 1]);

		/* Switch from 2G to 6G.
		 * Log: Peer 1 Band conflict 1 != 3
		 * update later in nanNdlMgmtFsmStep() NDL_SCHEDULE_ESTABLISHED,
		 * otherwise Schedule Request Response will be sent in new band
		 */
		prPeerSchRecord->eBand = eBand;
	}

	nanSchedCmdUpdateCRB(prAdapter, u4SchIdx);
}

enum _NAN_CHNL_BW_MAP nanSchedGet2gNanBw(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.ucNan2gBandwidth;
}

enum _NAN_CHNL_BW_MAP nanSchedGet5gNanBw(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.ucNan5gBandwidth;
}

enum _NAN_CHNL_BW_MAP nanSchedGet6gNanBw(struct ADAPTER *prAdapter)
{
	if (prAdapter->rWifiVar.ucNan6gBandwidth >= NAN_CHNL_BW_160 &&
		nanIsEhtEnable(prAdapter))
		return NAN_CHNL_BW_320;
	else
		return prAdapter->rWifiVar.ucNan6gBandwidth;
}

enum ENUM_CHANNEL_WIDTH
nanGetChannelBwByBand(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand)
{
	uint32_t u4Bw =
	nanSchedConfigGetAllowedBw(
			prAdapter,
			eBand);

	switch (u4Bw) {
	case 20:
		return CW_20_40MHZ;
	case 40:
		return CW_20_40MHZ;
	case 80:
		return CW_80MHZ;
	case 160:
		return CW_160MHZ;
	case 320:
		return CW_320_1MHZ;
	default:
		return CW_20_40MHZ;
	}
}

uint32_t nanSchedConfigGetAllowedBw(struct ADAPTER *prAdapter,
				    enum ENUM_BAND eBand)
{
	enum _NAN_CHNL_BW_MAP eBwMap;
	uint32_t u4SupportedBw;

	if (eBand == BAND_5G)
		eBwMap = nanSchedGet5gNanBw(prAdapter);
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_NAN_6G == 1)
	else if (eBand == BAND_6G)
		eBwMap = nanSchedGet6gNanBw(prAdapter);
#endif
	else
		eBwMap = nanSchedGet2gNanBw(prAdapter);

	if (((eBand == BAND_2G4) || (!prAdapter->rWifiVar.fgEnNanVHT)) &&
	    (eBwMap > NAN_CHNL_BW_40))
		eBwMap = NAN_CHNL_BW_40;

	switch (eBwMap) {
	default:
	case NAN_CHNL_BW_20:
		u4SupportedBw = 20;
		break;

	case NAN_CHNL_BW_40:
		u4SupportedBw = 40;
		break;

	case NAN_CHNL_BW_80:
		u4SupportedBw = 80;
		break;

	case NAN_CHNL_BW_160:
		u4SupportedBw = 160;
		break;

	case NAN_CHNL_BW_320:
		u4SupportedBw = 320;
		break;
	}

	return u4SupportedBw;
}

uint32_t
nanSchedConfigDefNdlNumSlots(struct ADAPTER *prAdapter, uint32_t u4NumSlots)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	if (u4NumSlots < NAN_DEFAULT_NDL_QUOTA_LOW_BOUND)
		u4NumSlots = NAN_DEFAULT_NDL_QUOTA_LOW_BOUND;
	else if (u4NumSlots > NAN_DEFAULT_NDL_QUOTA_UP_BOUND)
		u4NumSlots = NAN_DEFAULT_NDL_QUOTA_UP_BOUND;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prNegoCtrl->u4DefNdlNumSlots = u4NumSlots;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedConfigDefRangingNumSlots(struct ADAPTER *prAdapter,
				 uint32_t u4NumSlots)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	if (u4NumSlots < NAN_DEFAULT_RANG_QUOTA_LOW_BOUND)
		u4NumSlots = NAN_DEFAULT_RANG_QUOTA_LOW_BOUND;
	else if (u4NumSlots > NAN_DEFAULT_RANG_QUOTA_UP_BOUND)
		u4NumSlots = NAN_DEFAULT_RANG_QUOTA_UP_BOUND;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prNegoCtrl->u4DefRangingNumSlots = u4NumSlots;

	return WLAN_STATUS_SUCCESS;
}

void nanSet6GModeCtrl(struct ADAPTER *prAdapter, uint8_t mode)
{
#if (CFG_SUPPORT_NAN_6G == 1)
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	uint8_t cmd[30] = {0};
	uint8_t strLen = 0;
	uint32_t strOutLen = 0;

	strLen = kalSnprintf(cmd, sizeof(cmd),
			"NanSched set %d %d",
			0x0c0c,
			mode);
	DBGLOG(NAN, DEBUG,
		"6G Notify FW %s, strlen=%d\n",
		cmd, strLen);

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
	rChipConfigInfo.u2MsgSize = strLen;
	kalStrnCpy(rChipConfigInfo.aucCmd, cmd, strLen);
	wlanSetChipConfig(prAdapter, &rChipConfigInfo,
			sizeof(rChipConfigInfo), &strOutLen, FALSE);
#endif
}

#if (CFG_SUPPORT_NAN_6G == 1)
/* Update the primary channel bitmap (default set by) */
static void nanUpdate6gPotentialPrimary(union _NAN_BAND_CHNL_CTRL g_r6gDefChnl)
{
	size_t i;
	struct _NAN_POTENTIAL_CHNL_T *pr6gPotentialChnlMap;
	uint8_t ucPriChnlBitmap;

	ucPriChnlBitmap = BIT((g_r6gDefChnl.u4PrimaryChnl - 1) / 4);

	for (i = 0; i < ARRAY_SIZE(g_ar6gPotentialChnlMap); i++) {
		pr6gPotentialChnlMap = &g_ar6gPotentialChnlMap[i];

		if (pr6gPotentialChnlMap->ucPriChnlBitmap)
			pr6gPotentialChnlMap->ucPriChnlBitmap = ucPriChnlBitmap;
	}
}
#endif

enum ENUM_MAX_BANDWIDTH_SETTING nanBwTransmitMaxBw(
	enum _NAN_CHNL_BW_MAP eNanBw)
{
	switch (eNanBw) {
	case NAN_CHNL_BW_20:
		return MAX_BW_20MHZ;
	case NAN_CHNL_BW_40:
		return MAX_BW_40MHZ;
	case NAN_CHNL_BW_80:
		return MAX_BW_80MHZ;
	case NAN_CHNL_BW_160:
		return MAX_BW_160MHZ;
	case NAN_CHNL_BW_320:
		return MAX_BW_320_1MHZ;
	default:
		return MAX_BW_UNKNOWN;
	}

	return MAX_BW_UNKNOWN;
}

uint8_t nanGetMaxBw(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand)
{
	enum _NAN_CHNL_BW_MAP eBwMap = NAN_CHNL_BW_20;

	if (eBand == BAND_2G4)
		eBwMap = nanSchedGet2gNanBw(prAdapter);
	else if (eBand == BAND_5G)
		eBwMap = nanSchedGet5gNanBw(prAdapter);
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eBand == BAND_6G)
		eBwMap = nanSchedGet6gNanBw(prAdapter);
#endif
	else {
		DBGLOG(NAN, WARN,
			"Invalid band(%d).\n", eBand);
		return MAX_BW_20MHZ;
	}

	return (uint8_t)nanBwTransmitMaxBw(eBwMap);

}

uint32_t
nanSchedConfigAllowedBand(struct ADAPTER *prAdapter, unsigned char fgEn2g,
			  unsigned char fgEn5gH, unsigned char fgEn5gL,
			  unsigned char fgEn6g)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct _NAN_SCHEDULER_T *prNanScheduler;
	/* whsu */
	/* UINT_8 ucDiscChnlBw = BW_20; */
	uint8_t ucDisc2GChnlBw = NAN_CHNL_BW_20;
	uint8_t ucDisc5GChnlBw = NAN_CHNL_BW_20;
#if (CFG_SUPPORT_NAN_6G == 1)
	uint8_t ucDisc6GChnlBw = NAN_CHNL_BW_20;
#endif
	size_t szTimeLineIdx = 0;
#if (CFG_SUPPORT_NAN_6G == 1)
	enum ENUM_MAX_BANDWIDTH_SETTING eBw =
		nanBwTransmitMaxBw(nanSchedGet6gNanBw(prAdapter));
	uint8_t fgIsNAN6GChnlAllowed =
			rlmDomainIsLegalChlByNetType(prAdapter, BAND_6G,
						NAN_6G_BW20_DEFAULT_CHANNEL,
						eBw,
						NETWORK_TYPE_NAN);

	/* If NAN 6G chnl not legal, close NAN 6G to prevent nego issue. */
	if (!fgIsNAN6GChnlAllowed) {
		prWifiVar->ucNanEnable6g = 0;
		wlanCfgSetUint32(prAdapter, "NanEnable6g", 0);
	}
#endif

	prNanScheduler = nanGetScheduler(prAdapter);

	prNanScheduler->fgEn2g = fgEn2g;
	prNanScheduler->fgEn5gL = fgEn5gL;
	prNanScheduler->fgEn5gH = fgEn5gH;
#if (CFG_SUPPORT_NAN_6G == 1)
	prNanScheduler->fgEn6g = fgEn6g &&
				 prWifiVar->ucNanEnable6g &&
				 fgIsNAN6GChnlAllowed;
	nanSetNanUseR4AvailAttr(prWifiVar->ucNanUseR4AvailAttr);

	nanSet6GModeCtrl(prAdapter, prNanScheduler->fgEn6g);
#endif

	DBGLOG(NAN, DEBUG,
	       "Country=%c%c (%c %c%c%c %c), Allowed Band: %d, %d, %d, %d, %d\n",
	       kalIsPrint(prAdapter->rWifiVar.CountryCode[1]) ?
		       prAdapter->rWifiVar.CountryCode[1] : '-',
	       kalIsPrint(prAdapter->rWifiVar.CountryCode[0]) ?
		       prAdapter->rWifiVar.CountryCode[0] : '-',
	       prNanScheduler->fgEn6g ? '6' : ' ',
	       prNanScheduler->fgEn5gH || prNanScheduler->fgEn5gL ? '5' : ' ',
	       prNanScheduler->fgEn5gH ? 'H' : ' ',
	       prNanScheduler->fgEn5gL ? 'L' : ' ',
	       prNanScheduler->fgEn2g ? '2' : ' ',
	       fgEn2g, fgEn5gH, fgEn5gL, fgEn6g, prNanScheduler->fgEn6g);

	ucDisc2GChnlBw = nanSchedGet2gNanBw(prAdapter);
	ucDisc5GChnlBw = nanSchedGet5gNanBw(prAdapter);
#if (CFG_SUPPORT_NAN_6G == 1)
	ucDisc6GChnlBw = nanSchedGet6gNanBw(prAdapter);
#endif

	/* NAN 2G BW check */
	if ((!prWifiVar->fgEnNanVHT) &&
	    (ucDisc2GChnlBw > NAN_CHNL_BW_40))
		ucDisc2GChnlBw = NAN_CHNL_BW_40;

	g_r2gDwChnl.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
	g_r2gDwChnl.u4AuxCenterChnl = 0;
	g_r2gDwChnl.u4PrimaryChnl = NAN_2P4G_DISC_CHANNEL;
	if (ucDisc2GChnlBw == NAN_CHNL_BW_20)
		g_r2gDwChnl.u4OperatingClass =
			NAN_2P4G_DISC_CH_OP_CLASS;
	else
		g_r2gDwChnl.u4OperatingClass =
			NAN_2P4G_BW40_DISC_CH_OP_CLASS;

	g_r5gDwChnl.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
	g_r5gDwChnl.u4AuxCenterChnl = 0;
	if (fgEn5gH) {
		g_r5gDwChnl.u4PrimaryChnl = NAN_5G_HIGH_DISC_CHANNEL;

		if (ucDisc5GChnlBw == NAN_CHNL_BW_20)
			g_r5gDwChnl.u4OperatingClass =
				NAN_5G_HIGH_DISC_CH_OP_CLASS;
		else if (ucDisc5GChnlBw == NAN_CHNL_BW_40)
			g_r5gDwChnl.u4OperatingClass =
				NAN_5G_HIGH_BW40_DISC_CH_OP_CLASS;
		else
			g_r5gDwChnl.u4OperatingClass =
				NAN_5G_HIGH_BW80_DISC_CH_OP_CLASS;
	} else if (fgEn5gL) {
		g_r5gDwChnl.u4PrimaryChnl = NAN_5G_LOW_DISC_CHANNEL;

		if (ucDisc5GChnlBw == NAN_CHNL_BW_20)
			g_r5gDwChnl.u4OperatingClass =
				NAN_5G_LOW_DISC_CH_OP_CLASS;
		else if (ucDisc5GChnlBw == NAN_CHNL_BW_40)
			g_r5gDwChnl.u4OperatingClass =
				NAN_5G_LOW_BW40_DISC_CH_OP_CLASS;
		else
			g_r5gDwChnl.u4OperatingClass =
				NAN_5G_LOW_BW80_DISC_CH_OP_CLASS;
	}

	if (ucDisc5GChnlBw == NAN_CHNL_BW_160) {
		g_r5g160Chnl.u4Type =
			NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
		g_r5g160Chnl.u4OperatingClass =
			NAN_5G_LOW_BW160_DISC_CH_OP_CLASS;
		g_r5g160Chnl.u4PrimaryChnl =
			NAN_5G_BW160_DEF_CHANNEL;
	}

#if (CFG_SUPPORT_NAN_6G == 1)
	g_r6gDefChnl.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
	g_r6gDefChnl.u4AuxCenterChnl = 0;
	if (prNanScheduler->fgEn6g) {

		if (nanGetFeatureIsSigma(prAdapter))
			g_r6gDefChnl.u4PrimaryChnl =
				NAN_6G_BW20_CERT_DEFAULT_CHANNEL;
		else
			g_r6gDefChnl.u4PrimaryChnl =
				prWifiVar->ucNan6gDefaultChannel;

		nanUpdate6gPotentialPrimary(g_r6gDefChnl);

		if (ucDisc6GChnlBw == NAN_CHNL_BW_20)
			g_r6gDefChnl.u4OperatingClass =
				NAN_6G_BW20_OP_CLASS;
		else if (ucDisc6GChnlBw == NAN_CHNL_BW_40)
			g_r6gDefChnl.u4OperatingClass =
				NAN_6G_BW40_OP_CLASS;
		else if (ucDisc6GChnlBw == NAN_CHNL_BW_80)
			g_r6gDefChnl.u4OperatingClass =
				NAN_6G_BW80_OP_CLASS;
		else if (ucDisc6GChnlBw == NAN_CHNL_BW_320)
			g_r6gDefChnl.u4OperatingClass =
				NAN_6G_BW320_OP_CLASS;
		else/* NAN_CHNL_BW160 */
			g_r6gDefChnl.u4OperatingClass =
				NAN_6G_BW160_OP_CLASS;
	}
#endif

	szTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter, BAND_2G4);

	if (fgEn2g)
		g_rPreferredChnl[szTimeLineIdx] = g_r2gDwChnl;

	szTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter, BAND_5G);

	if (fgEn5gH)
		g_rPreferredChnl[szTimeLineIdx] = g_r5gDwChnl;
	else if (fgEn5gL)
		g_rPreferredChnl[szTimeLineIdx] = g_r5gDwChnl;

#if (CFG_SUPPORT_NAN_DBDC == 1)
	nanUpdateMbmcIdx(prAdapter,
		nanGetBssIdxbyBand(prAdapter, BAND_2G4),
		(uint8_t)NAN_BSS_INDEX_BAND0);
	nanUpdateMbmcIdx(prAdapter,
		nanGetBssIdxbyBand(prAdapter, BAND_5G),
		(uint8_t)NAN_BSS_INDEX_BAND1);
#endif

#if (CFG_SUPPORT_NAN_6G == 1)
	if (prNanScheduler->fgEn6g)
		g_rPreferredChnl[szTimeLineIdx] = g_r6gDefChnl;
#endif

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedConfigPhyParams(struct ADAPTER *prAdapter)
{
#define VHT_CAP_INFO_NSS_MAX 8

	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	struct _NAN_PHY_SETTING_T r2P4GPhySettings;
	struct _NAN_PHY_SETTING_T r5GPhySettings;
	struct _NAN_PHY_SETTING_T *prPhySettings;

	enum ENUM_PARAM_NAN_MODE_T eNanMode;
	const struct NON_HT_PHY_ATTRIBUTE *prLegacyPhyAttr;
	const struct NON_HT_ADHOC_MODE_ATTRIBUTE *prLegacyModeAttr;
	uint8_t ucLegacyPhyTp;
	int32_t i;
	uint8_t ucOffset;
	uint8_t ucNss;
	uint8_t ucMcsMap;
	uint8_t ucVhtCapInfoNssOfst[VHT_CAP_INFO_NSS_MAX] = {
		VHT_CAP_INFO_MCS_1SS_OFFSET, VHT_CAP_INFO_MCS_2SS_OFFSET,
		VHT_CAP_INFO_MCS_3SS_OFFSET, VHT_CAP_INFO_MCS_4SS_OFFSET,
		VHT_CAP_INFO_MCS_5SS_OFFSET, VHT_CAP_INFO_MCS_6SS_OFFSET,
		VHT_CAP_INFO_MCS_7SS_OFFSET, VHT_CAP_INFO_MCS_8SS_OFFSET
	};

	/* 2.4G */
	prPhySettings = &r2P4GPhySettings;

	eNanMode = NAN_MODE_MIXED_11BG;
	prLegacyModeAttr = &rNonHTNanModeAttr[eNanMode];
	ucLegacyPhyTp = (uint8_t)prLegacyModeAttr->ePhyTypeIndex;
	prLegacyPhyAttr = &rNonHTPhyAttributes[ucLegacyPhyTp];

	prPhySettings->ucPhyTypeSet =
		prWifiVar->ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11BGN;
	prPhySettings->ucNonHTBasicPhyType = ucLegacyPhyTp;
	if (prLegacyPhyAttr->fgIsShortPreambleOptionImplemented &&
	    (prWifiVar->ePreambleType == PREAMBLE_TYPE_SHORT ||
	     prWifiVar->ePreambleType == PREAMBLE_TYPE_AUTO))
		prPhySettings->fgUseShortPreamble = TRUE;
	else
		prPhySettings->fgUseShortPreamble = FALSE;
	prPhySettings->fgUseShortSlotTime =
		prLegacyPhyAttr->fgIsShortSlotTimeOptionImplemented;

	prPhySettings->u2OperationalRateSet =
		prLegacyPhyAttr->u2SupportedRateSet;
	prPhySettings->u2BSSBasicRateSet = prLegacyModeAttr->u2BSSBasicRateSet;
	/* Mask CCK 1M For Sco scenario except FDD mode */
	if (prAdapter->u4FddMode == FALSE)
		prPhySettings->u2BSSBasicRateSet &= ~RATE_SET_BIT_1M;
	prPhySettings->u2VhtBasicMcsSet = 0;

	prPhySettings->ucHtOpInfo1 = 0; /* FW configues SCN, SCA or SCB */
	prPhySettings->u2HtOpInfo2 = 0;
	prPhySettings->u2HtOpInfo3 = 0;
	prPhySettings->fgErpProtectMode = FALSE;
	prPhySettings->eHtProtectMode = HT_PROTECT_MODE_NONE;
	prPhySettings->eGfOperationMode = GF_MODE_DISALLOWED;
	prPhySettings->eRifsOperationMode = RIFS_MODE_DISALLOWED;

	/* 5G */
	prPhySettings = &r5GPhySettings;

	eNanMode = NAN_MODE_11A;
	prLegacyModeAttr = &rNonHTNanModeAttr[eNanMode];
	ucLegacyPhyTp = (uint8_t)prLegacyModeAttr->ePhyTypeIndex;
	prLegacyPhyAttr = &rNonHTPhyAttributes[ucLegacyPhyTp];

	prPhySettings->ucPhyTypeSet =
		prWifiVar->ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11ANAC;
	prPhySettings->ucNonHTBasicPhyType = ucLegacyPhyTp;
	if (prLegacyPhyAttr->fgIsShortPreambleOptionImplemented &&
	    (prWifiVar->ePreambleType == PREAMBLE_TYPE_SHORT ||
	     prWifiVar->ePreambleType == PREAMBLE_TYPE_AUTO))
		prPhySettings->fgUseShortPreamble = TRUE;
	else
		prPhySettings->fgUseShortPreamble = FALSE;
	prPhySettings->fgUseShortSlotTime =
		prLegacyPhyAttr->fgIsShortSlotTimeOptionImplemented;

	prPhySettings->u2OperationalRateSet =
		prLegacyPhyAttr->u2SupportedRateSet;
	prPhySettings->u2BSSBasicRateSet = prLegacyModeAttr->u2BSSBasicRateSet;
	/* Mask CCK 1M For Sco scenario except FDD mode */
	if (prAdapter->u4FddMode == FALSE)
		prPhySettings->u2BSSBasicRateSet &= ~RATE_SET_BIT_1M;

	/* Filled the VHT BW/S1/S2 and MCS rate set */
	ucNss = prAdapter->rWifiVar.ucNSS;
#if CFG_SISO_SW_DEVELOP
	if (IS_WIFI_5G_SISO(prAdapter))
		ucNss = 1;
#endif
	if (prPhySettings->ucPhyTypeSet & PHY_TYPE_BIT_VHT) {
		prPhySettings->u2VhtBasicMcsSet = 0;
		for (i = 0; i < VHT_CAP_INFO_NSS_MAX; i++) {
			ucOffset = ucVhtCapInfoNssOfst[i];

			if (i < ucNss)
				ucMcsMap = VHT_CAP_INFO_MCS_MAP_MCS9;
			else
				ucMcsMap = VHT_CAP_INFO_MCS_NOT_SUPPORTED;

			prPhySettings->u2VhtBasicMcsSet |=
				(ucMcsMap << ucOffset);
		}
	} else {
		prPhySettings->u2VhtBasicMcsSet = 0;
	}

	prPhySettings->ucHtOpInfo1 = 0; /* FW configues SCN, SCA or SCB */
	prPhySettings->u2HtOpInfo2 = 0;
	prPhySettings->u2HtOpInfo3 = 0;
	prPhySettings->fgErpProtectMode = FALSE;
	prPhySettings->eHtProtectMode = HT_PROTECT_MODE_NONE;
	prPhySettings->eGfOperationMode = GF_MODE_DISALLOWED;
	prPhySettings->eRifsOperationMode = RIFS_MODE_DISALLOWED;

	nanSchedCmdUpdatePhySettigns(prAdapter, &r2P4GPhySettings,
				     &r5GPhySettings);

	return WLAN_STATUS_SUCCESS;
}

/**
 * if (timeline->mapid && ndc->mapid) && (timeline->bitmap & ndc->bitmap)
 */
static void nanSchedUpdateActiveNdcBands(struct ADAPTER *prAdapter)
{
	size_t szTimeLineIdx = 0;
	size_t szIdx = 0;
	size_t i, j;
	struct _NAN_SCHEDULER_T *prScheduler;
	struct _NAN_NDC_CTRL_T *prNdcCtrl;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlList;
	struct _NAN_SCHEDULE_TIMELINE_T *prNdcTimeline;
	enum ENUM_BAND eBand;
	uint8_t ucNdcBandBitmap = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	prScheduler = nanGetScheduler(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < NAN_TIMELINE_MGMT_SIZE;
	     szTimeLineIdx++) {
		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

		for (szIdx = 0; szIdx < NAN_MAX_NDC_RECORD; szIdx++) {
			prNdcCtrl = &prScheduler->arNdcCtrl[szIdx];
			if (!prNdcCtrl->fgValid)
				continue;

			for (i = 0; i < NAN_TIMELINE_MGMT_SIZE; i++) {
				prNdcTimeline = &prNdcCtrl->arTimeline[i];

				if (prNdcCtrl->arTimeline[i].ucMapId !=
				    prNanTimelineMgmt->ucMapId)
					continue;

				/* ucMap matched */
				if (szNanActiveTimelineNum > 1) {
					ucNdcBandBitmap |=
						BIT(szTimeLineIdx);
					continue;
				}

				/* Single timeline */
				prChnlList = prNanTimelineMgmt->arChnlList;
				for (j = 0; j < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
				     j++, prChnlList++) {
					if (!prChnlList->fgValid)
						continue;

					if (!(prChnlList->au4AvailMap[0] &
					      prNdcTimeline->au4AvailMap[0]))
						continue;

					eBand = nanRegGetNanChnlBand(
							prChnlList->rChnlInfo);

					/* enum NAN_BAND_IDX */
					if (eBand != BAND_NULL)
						ucNdcBandBitmap |=
							BIT(eBand - 1);
				}
			}
		}
	}

	prScheduler->ucNdcBand = ucNdcBandBitmap;
	DBGLOG(NAN, DEBUG, "NDC band = 0x%02x\n", prScheduler->ucNdcBand);

#if (CFG_SUPPORT_NAN_DBDC == 1)
	if (prScheduler->ucNdcBand ==
	    (BIT(NAN_BSS_INDEX_BAND0) | BIT(NAN_BSS_INDEX_BAND1))) {
		DBGLOG(NAN, WARN, "NDC in both bands\n");
	}
#endif
}

static void
nanSchedReleaseUnusedNdcCtrl(struct ADAPTER *prAdapter)
{
	size_t szIdx = 0, szSchIdx = 0, szTimeLineIdx = 0;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord = NULL;
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	struct _NAN_SCHEDULER_T *prScheduler = NULL;

	prScheduler = nanGetScheduler(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < NAN_TIMELINE_MGMT_SIZE;
	     szTimeLineIdx++) {

		for (szIdx = 0; szIdx < NAN_MAX_NDC_RECORD; szIdx++) {
			prNdcCtrl = &prScheduler->arNdcCtrl[szIdx];
			if (prNdcCtrl->fgValid == FALSE)
				continue;

			/* release the NDC if there's
			 * no any peer occupying it
			 */
			for (szSchIdx = 0;
				szSchIdx < NAN_MAX_CONN_CFG; szSchIdx++) {
				prPeerSchRecord =
					nanSchedGetPeerSchRecord(prAdapter,
						szSchIdx);
				if (prPeerSchRecord == NULL)
					continue;

				if (prPeerSchRecord->fgActive == FALSE)
					continue;

				if (prPeerSchRecord->prCommNdcCtrl == prNdcCtrl)
					break;
			}
			if (szSchIdx >= NAN_MAX_CONN_CFG)
				prNdcCtrl->fgValid = FALSE;
		}
	}
}

void nanSchedReleaseUnusedCommitSlot(struct ADAPTER *prAdapter)
{
	uint32_t au4UsedAvailMap[NAN_TOTAL_DW];
	struct _NAN_SCHEDULE_TIMELINE_T *prCommFawTimeline;
	struct _NAN_SCHEDULE_TIMELINE_T *prRangingTimeline;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	uint32_t u4Idx, u4Idx1;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline;
	uint32_t u4SlotIdx;
	uint32_t u4SchIdx;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_NDC_CTRL_T *prNdcCtrl;
	struct _NAN_SCHEDULER_T *prScheduler;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	union {
		uint32_t u4TimelineSlots;
		uint8_t ucSlot[4];
	} uTimeline[3];

	prScheduler = nanGetScheduler(prAdapter);

	nanSchedReleaseUnusedNdcCtrl(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

		kalMemZero(au4UsedAvailMap, sizeof(au4UsedAvailMap));
		kalMemZero(uTimeline, sizeof(uTimeline));

		for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
			prPeerSchRecord =
				nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
			if (prPeerSchRecord == NULL)
				continue;

			if (prPeerSchRecord->fgActive == FALSE)
				continue;

			prCommFawTimeline = prPeerSchRecord->arCommFawTimeline;
			prRangingTimeline =
				prPeerSchRecord->arCommRangingTimeline;
			if (prPeerSchRecord->fgUseDataPath)
				prTimeline = &prCommFawTimeline[szTimeLineIdx];
			else if (prPeerSchRecord->fgUseRanging)
				prTimeline = &prRangingTimeline[szTimeLineIdx];
			else
				continue;

			for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++) {
				au4UsedAvailMap[u4Idx] |=
					prTimeline->au4AvailMap[u4Idx];

				uTimeline[0].u4TimelineSlots |=
					prTimeline->au4AvailMap[u4Idx];
			}
		}

		for (u4Idx = 0; u4Idx < NAN_MAX_NDC_RECORD; u4Idx++) {
			prNdcCtrl = &prScheduler->arNdcCtrl[u4Idx];
			if (prNdcCtrl->fgValid == FALSE)
				continue;

			/* release the NDC if there's
			 * no any peer occupying it
			 */
			for (u4SchIdx = 0;
				u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
				prPeerSchRecord =
					nanSchedGetPeerSchRecord(prAdapter,
						u4SchIdx);
				if (prPeerSchRecord == NULL)
					continue;

				if (prPeerSchRecord->fgActive == FALSE)
					continue;

				if (prPeerSchRecord->prCommNdcCtrl == prNdcCtrl)
					break;
			}
			if (u4SchIdx >= NAN_MAX_CONN_CFG) {
				prNdcCtrl->fgValid = FALSE;
				continue;
			}

			for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++) {
				struct _NAN_SCHEDULE_TIMELINE_T *pNdcTimeline;

				pNdcTimeline =
					&prNdcCtrl->arTimeline[szTimeLineIdx];

				au4UsedAvailMap[u4Idx] |=
					pNdcTimeline->au4AvailMap[u4Idx];
				uTimeline[1].u4TimelineSlots |=
					pNdcTimeline->au4AvailMap[u4Idx];
			}
		}

		/* consider custom FAW */
		for (u4Idx = 0;
			u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
			prChnlTimeline =
				&prNanTimelineMgmt->arCustChnlList[u4Idx];
			if (prChnlTimeline->fgValid == FALSE)
				continue;

			for (u4Idx1 = 0; u4Idx1 < NAN_TOTAL_DW; u4Idx1++) {
				au4UsedAvailMap[u4Idx1] |=
					prChnlTimeline->au4AvailMap[u4Idx1];
				uTimeline[2].u4TimelineSlots |=
					prChnlTimeline->au4AvailMap[u4Idx1];
			}
		}
		DBGLOG(NAN, INFO,
		       "timeline=%zu held by peer=%02x-%02x-%02x-%02x, NDC=%02x-%02x-%02x-%02x, custom=%02x-%02x-%02x-%02x",
		       szTimeLineIdx,
		       uTimeline[0].ucSlot[0], uTimeline[0].ucSlot[1],
		       uTimeline[0].ucSlot[2], uTimeline[0].ucSlot[3],
		       uTimeline[1].ucSlot[0], uTimeline[1].ucSlot[1],
		       uTimeline[1].ucSlot[2], uTimeline[1].ucSlot[3],
		       uTimeline[2].ucSlot[0], uTimeline[2].ucSlot[1],
		       uTimeline[2].ucSlot[2], uTimeline[2].ucSlot[3]);

		for (u4SlotIdx = 0;
			u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
			if (NAN_IS_AVAIL_MAP_SET(au4UsedAvailMap, u4SlotIdx))
				continue;

			nanSchedDeleteCrbFromChnlList(prAdapter,
				u4SlotIdx, 1,
				ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
				TRUE, szTimeLineIdx);
		}
	}
	/* nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__); */
}

void
nanSchedDropResources(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
		      enum _ENUM_NAN_NEGO_TYPE_T eType)
{
	uint32_t u4SchIdx;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4Idx;

	u4SchIdx = nanSchedLookupPeerSchRecordIdx(prAdapter, pucNmiAddr);
	if (u4SchIdx == NAN_INVALID_SCH_RECORD)
		return;

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

	if (prPeerSchRecord == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchRecord\n");
		return;
	}

	prPeerSchDesc = prPeerSchRecord->prPeerSchDesc;

	DBGLOG(NAN, DEBUG, "\n\n");
	DBGLOG(NAN, DEBUG, "------>\n");
	DBGLOG(NAN, DEBUG, "Drop %02x:%02x:%02x:%02x:%02x:%02x Type:%d\n",
	       pucNmiAddr[0], pucNmiAddr[1], pucNmiAddr[2], pucNmiAddr[3],
	       pucNmiAddr[4], pucNmiAddr[5], eType);

	switch (eType) {
	case ENUM_NAN_NEGO_DATA_LINK:
		prPeerSchRecord->fgUseDataPath = FALSE;
		prPeerSchRecord->u4FinalQosMaxLatency =
			NAN_INVALID_QOS_MAX_LATENCY;
		prPeerSchRecord->u4FinalQosMinSlots = NAN_INVALID_QOS_MIN_SLOTS;

		prPeerSchDesc->fgImmuNdlTimelineValid = FALSE;
		for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
			prPeerSchDesc->arImmuNdlTimeline[u4Idx].ucMapId =
				NAN_INVALID_MAP_ID;
			prPeerSchDesc->arImmuNdlTimeline[u4Idx].ucLocalMapId =
				NAN_INVALID_MAP_ID;
		}
		prPeerSchDesc->rSelectedNdcCtrl.fgValid = FALSE;
		prPeerSchDesc->u4QosMaxLatency = NAN_INVALID_QOS_MAX_LATENCY;
		prPeerSchDesc->u4QosMinSlots = NAN_INVALID_QOS_MIN_SLOTS;
		break;

	case ENUM_NAN_NEGO_RANGING:
		prPeerSchRecord->fgUseRanging = FALSE;

		prPeerSchDesc->fgRangingTimelineValid = FALSE;
		for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
			prPeerSchDesc->arRangingTimeline[u4Idx].ucMapId =
				NAN_INVALID_MAP_ID;
			prPeerSchDesc->arRangingTimeline[u4Idx].ucLocalMapId =
				NAN_INVALID_MAP_ID;
		}
		break;

	default:
		break;
	}

	if (prPeerSchRecord->fgActive && !prPeerSchRecord->fgUseDataPath &&
	    !prPeerSchRecord->fgUseRanging) {
		nanSchedReleasePeerSchedRecord(prAdapter, u4SchIdx);

		/* release unused commit slot */
		if (!nanSchedNegoInProgress(prAdapter)) {
			nanSchedReleaseUnusedCommitSlot(prAdapter);
			nanSchedCmdUpdateAvailability(prAdapter);
		}
	}

	nanSchedDumpPeerSchDesc(prAdapter, prPeerSchDesc);
	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
	DBGLOG(NAN, DEBUG, "<------\n");
}

void
nanSchedNegoDumpState(struct ADAPTER *prAdapter, uint8_t *pucFunc,
		      uint32_t u4Line)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	DBGLOG(NAN, DEBUG, "\n\n");
	DBGLOG(NAN, DEBUG, "------>\n");
	DBGLOG(NAN, DEBUG, "#%s@%d\n", pucFunc, u4Line);
	DBGLOG(NAN, DEBUG, "Role:%d, Type:%d, State:%d\n", prNegoCtrl->eRole,
	       prNegoCtrl->eType, prNegoCtrl->eState);

	do {
		prPeerSchRecord = nanSchedGetPeerSchRecord(
			prAdapter, prNegoCtrl->u4SchIdx);
		if (!prPeerSchRecord)
			break;

		prPeerSchDesc = prPeerSchRecord->prPeerSchDesc;
		if (!prPeerSchDesc)
			break;

		nanSchedDumpPeerSchDesc(prAdapter, prPeerSchDesc);
	} while (FALSE);

	nanSchedDbgDumpTimelineDb(prAdapter, pucFunc, u4Line);
	DBGLOG(NAN, DEBUG, "<------\n\n");
}

/**
 * nanSchedNegoSelectChnlInfo() - select channel for the slot index
 * @prAdapter: pointer to adapter
 * @u4SlotIdx: git given slot index to select channel
 *
 * The channel selection is determined in the oeder:
 * 1. peer committed
 * 2. conditional
 * 3. potential channel
 * 4. potential band
 * 5. local preference (g_rPreferredChnl)
 *
 * Return: The selected channel info of the band
 */
union _NAN_BAND_CHNL_CTRL
nanSchedNegoSelectChnlInfo(struct ADAPTER *prAdapter, uint32_t u4SlotIdx,
	size_t szTimeLineIdx, uint32_t u4PrefBandMask)
{
	uint32_t u4SchIdx = 0;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rTargetChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rSelBandInfo = {.u4RawData = 0};
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	struct _NAN_SCHEDULER_T *prNanScheduler = NULL;
	uint32_t u4AvailDbIdx = 0;
	uint32_t u4BandIdMask = 0;
	enum ENUM_BAND eBand = BAND_2G4, eSelChnlBand = BAND_2G4;
	uint32_t u4SuppBandIdMask = 0;
	const size_t sz2gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_2G4);
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	enum ENUM_BAND ePrefBand[BAND_NUM] = {
#if (CFG_SUPPORT_WIFI_6G == 1)
			BAND_6G,
#endif
			BAND_5G,
			BAND_2G4,
			BAND_NULL
		};
	uint32_t u4PrefBandMaskTmp = 0;
	size_t szIdx = 0;
	size_t szPrefBandNum = BAND_NUM;
	union _NAN_BAND_CHNL_CTRL rPrefChnl = {0};
	uint8_t fgSkipPotentialCh = FALSE;
	union _NAN_BAND_CHNL_CTRL rPeerValidChnl = g_rNullChnl;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	rTargetChnlInfo.u4RawData = 0;
	u4SuppBandIdMask = nanGetTimelineSupportedBand(prAdapter,
			szTimeLineIdx);


	if (u4PrefBandMask)
		szPrefBandNum = 0;
	u4PrefBandMaskTmp = u4PrefBandMask;
	while (u4PrefBandMaskTmp) {
		switch ((u4PrefBandMaskTmp & 0xF)) {
		case NAN_PREFER_BAND_MASK_CUST_CH: /* reference preferred ch */
			if (u4SuppBandIdMask & BIT(BAND_5G))
				rPrefChnl = g_rPreferredChnl[sz5gTimeLineIdx];
			else
				rPrefChnl = g_rPreferredChnl[sz2gTimeLineIdx];
			eBand = nanRegGetNanChnlBand(rPrefChnl);
			break;

		case NAN_PREFER_BAND_MASK_2G_DW_CH: /* reference 2G DW ch */
		case NAN_PREFER_BAND_MASK_2G_BAND: /* reference 2G BAND */
			eBand = BAND_2G4;
			break;

		case NAN_PREFER_BAND_MASK_5G_DW_CH: /* reference 5G DW ch */
		case NAN_PREFER_BAND_MASK_5G_BAND: /* reference 5G BAND */
			eBand = BAND_5G;
			break;

#if (CFG_SUPPORT_WIFI_6G == 1)
		case NAN_PREFER_BAND_MASK_6G_BAND: /* reference 6G BAND */
			eBand = BAND_6G;
			break;
#endif

		default:
			eBand = BAND_NULL;
			break;
		}
		u4PrefBandMaskTmp >>= 4;

		if (eBand == BAND_NULL)
			continue;

		for (szIdx = 0; szIdx < szPrefBandNum; szIdx++) {
			if (ePrefBand[szIdx] == eBand)
				break;
		}

		if (szIdx >= szPrefBandNum) {
			ePrefBand[szIdx] = eBand;
			szPrefBandNum++;
		}
	}

	for (szIdx = 0; szIdx <  szPrefBandNum; szIdx++) {
		eBand = ePrefBand[szIdx];
		if (eBand == BAND_NULL)
			break;

		if ((u4SuppBandIdMask & BIT(eBand)) == 0)
			continue;

		if (!prPeerSchDesc) {
			DBGLOG(NAN, ERROR, "prPeerSchDesc error!\n");
			return g_rPreferredChnl[szTimeLineIdx];
		}

		if (prNegoCtrl->eState == ENUM_NAN_CRB_NEGO_STATE_INITIATOR) {
			rSelChnlInfo = nanSchedGetFixedChnlInfo(prAdapter);
			eSelChnlBand = nanRegGetNanChnlBand(rSelChnlInfo);
			if ((rSelChnlInfo.u4PrimaryChnl != 0)
				&& (eSelChnlBand == eBand))
				return rSelChnlInfo;
		}

		/* commit channel */
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByDesc(prAdapter,
				prPeerSchDesc, u4AvailDbIdx) == FALSE)
				continue;

			rSelChnlInfo = nanQueryPeerChnlInfoBySlot(prAdapter,
				u4SchIdx, u4AvailDbIdx, u4SlotIdx, TRUE);
			if (rSelChnlInfo.u4PrimaryChnl == 0)
				continue;

			if (!nanIsAllowedChannel(prAdapter, rSelChnlInfo))
				continue;

			rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
								rSelChnlInfo);
			eSelChnlBand = nanRegGetNanChnlBand(rSelChnlInfo);
			if (rSelChnlInfo.u4PrimaryChnl != 0) {
#ifdef NAN_UNUSED
				if (eSelChnlBand == eBand)
					return rSelChnlInfo;
#else
				if (nanRegGetNanChnlBand(rSelChnlInfo) >
				    nanRegGetNanChnlBand(rTargetChnlInfo))
					rTargetChnlInfo = rSelChnlInfo;
#endif

				if ((u4SuppBandIdMask & BIT(eSelChnlBand))
					!= 0) {
					fgSkipPotentialCh = TRUE;
					if (rPeerValidChnl.u4PrimaryChnl == 0)
						rPeerValidChnl = rSelChnlInfo;
				}
			}
		}
		if (rTargetChnlInfo.u4PrimaryChnl != 0)
			/* found committed */
			return rTargetChnlInfo;

		/* conditional channel */
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByDesc(prAdapter,
				prPeerSchDesc, u4AvailDbIdx) == FALSE)
				continue;

			rSelChnlInfo =
				nanQueryPeerChnlInfoBySlot(prAdapter,
				u4SchIdx, u4AvailDbIdx, u4SlotIdx, FALSE);
			if (rSelChnlInfo.u4PrimaryChnl == 0)
				continue;

			if (!nanIsAllowedChannel(prAdapter, rSelChnlInfo))
				continue;

			rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
								rSelChnlInfo);
			eSelChnlBand = nanRegGetNanChnlBand(rSelChnlInfo);
			if (rSelChnlInfo.u4PrimaryChnl != 0) {
#ifdef NAN_UNUSED
				if (eSelChnlBand == eBand)
					return rSelChnlInfo;
#else
				if (nanRegGetNanChnlBand(rSelChnlInfo) >
					nanRegGetNanChnlBand(rTargetChnlInfo))
					rTargetChnlInfo = rSelChnlInfo;
#endif

				if ((u4SuppBandIdMask & BIT(eSelChnlBand))
					!= 0) {
					fgSkipPotentialCh = TRUE;
					if (rPeerValidChnl.u4PrimaryChnl == 0)
						rPeerValidChnl = rSelChnlInfo;
				}
			}
		}
		if (rTargetChnlInfo.u4PrimaryChnl != 0)
			/* found conditional */
			return rTargetChnlInfo;

		/*
		 * If commit/conditional channel is available on the peer,
		 * it has the highest priority.
		 * So even it doesn't satisfy local preference, still need
		 * to honor it.
		 */
		if (fgSkipPotentialCh)
			continue;

		/* potential channel */
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
			u4AvailDbIdx++) {
			rSelChnlInfo =
				nanQueryPeerPotentialChnlInfoBySlot(prAdapter,
					u4SchIdx, u4AvailDbIdx, u4SlotIdx,
					BIT(eBand), 0, u4PrefBandMask);
			if (rSelChnlInfo.u4PrimaryChnl == 0)
				continue;

			if (!nanIsAllowedChannel(prAdapter, rSelChnlInfo))
				continue;

			rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
								rSelChnlInfo);
			if (rSelChnlInfo.u4PrimaryChnl != 0) {
#ifdef NAN_UNUSED
				return rSelChnlInfo;
#else
				if (nanRegGetNanChnlBand(rSelChnlInfo) >
				    nanRegGetNanChnlBand(rTargetChnlInfo))
					rTargetChnlInfo = rSelChnlInfo;
#endif
			}
		}
		if (rTargetChnlInfo.u4PrimaryChnl != 0)
			/* found potential */
			return rTargetChnlInfo;

		/* potential band */
		u4BandIdMask = 0;
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			rSelBandInfo =
				nanQueryPeerPotentialBandInfoBySlot(prAdapter,
				u4SchIdx, u4AvailDbIdx, u4SlotIdx, BIT(eBand));
			u4BandIdMask |= rSelBandInfo.u4BandIdMask;
		}

		prNanScheduler = nanGetScheduler(prAdapter);

#if (CFG_SUPPORT_NAN_6G == 1)
		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_6G) &&
		    prNanScheduler->fgEn6g)
			return g_r6gDefChnl;
#endif
		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_5G) &&
		    (prNanScheduler->fgEn5gH || prNanScheduler->fgEn5gL))
			return g_r5gDwChnl;

		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_2P4G) &&
		    prNanScheduler->fgEn2g)
			return g_r2gDwChnl;
	}

	if (rPeerValidChnl.u4PrimaryChnl != 0)
		return rPeerValidChnl;

	return g_rPreferredChnl[szTimeLineIdx];
}

uint32_t
nanSchedNegoDecideChnlInfoForEmptySlot(struct ADAPTER *prAdapter,
	uint32_t aau4EmptyMap[NAN_TIME_BITMAP_MAX_SIZE][NAN_TOTAL_DW])
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SlotIdx = 0;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {.u4RawData = 0};
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		for (u4SlotIdx = 0;
			u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
			if (NAN_IS_AVAIL_MAP_SET(aau4EmptyMap[szTimeLineIdx],
				u4SlotIdx)) {
				rSelChnlInfo =
					nanSchedNegoSelectChnlInfo(prAdapter,
					u4SlotIdx, szTimeLineIdx,
					prAdapter->rWifiVar
						.u4NanPreferBandMask);
				if (rSelChnlInfo.u4PrimaryChnl == 0) {
					DBGLOG(NAN, ERROR,
						"u4PrimaryChnl equals 0!\n");
					return WLAN_STATUS_FAILURE;
				}
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);
			}
		}
	}

	return rRetStatus;
}

struct _NAN_PEER_SCHEDULE_RECORD_T *
nanSchedNegoSyncSchFindNextPeerSchRec(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;
	uint32_t u4SchIdx;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	for (u4SchIdx = prNegoCtrl->u4SyncPeerSchRecIdx;
	     u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
		prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
		if ((prPeerSchRec == NULL) ||
		    (prPeerSchRec->fgActive == FALSE) ||
		    (prPeerSchRec->prPeerSchDesc == NULL))
			continue;

		prNegoCtrl->u4SyncPeerSchRecIdx =
			u4SchIdx + 1; /* point to next record */
		return prPeerSchRec;
	}

	prNegoCtrl->u4SyncPeerSchRecIdx = u4SchIdx;
	return NULL;
}

static enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T
nanSchedNegoSyncSchUpdateFsmStep(
	struct ADAPTER *prAdapter,
	enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T eNextState)
{
	enum _ENUM_NAN_SYNC_SCH_UPDATE_STATE_T eLastState;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;
	struct _NAN_PARAMETER_NDL_SCH rNanUpdateSchParam;
	uint32_t rStatus;
	uint8_t *pucNmiAddr;
	uint32_t u4SchIdx;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	do {
		DBGLOG(NAN, DEBUG, "SCH UPDATE STATE: [%d] -> [%d]\n",
		       prNegoCtrl->eSyncSchUpdateCurrentState, eNextState);

		prNegoCtrl->eSyncSchUpdateLastState =
			prNegoCtrl->eSyncSchUpdateCurrentState;
		prNegoCtrl->eSyncSchUpdateCurrentState = eNextState;

		eLastState = eNextState;

		switch (eNextState) {
		case ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE:
			/* stable state */

			/* check all peers to see if any one conflicts
			 * the new schedule
			 */
			nanSchedReleaseUnusedCommitSlot(prAdapter);
			nanSchedCmdUpdateAvailability(prAdapter);

			for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG;
			     u4SchIdx++) {
				prPeerSchRec = nanSchedGetPeerSchRecord(
					prAdapter, u4SchIdx);
				if (!prPeerSchRec->fgActive ||
				    (prPeerSchRec->prPeerSchDesc == NULL))
					continue;

				nanSchedPeerChkAvailability(
					prAdapter, prPeerSchRec->prPeerSchDesc);
			}
			break;

		case ENUM_NAN_SYNC_SCH_UPDATE_STATE_PREPARE:
			if (prNegoCtrl->ucNegoTransNum == 0)
				eNextState =
					ENUM_NAN_SYNC_SCH_UPDATE_STATE_CHECK;
			break;

		case ENUM_NAN_SYNC_SCH_UPDATE_STATE_CHECK:
			if (prNegoCtrl->eSyncSchUpdateLastState !=
			    ENUM_NAN_SYNC_SCH_UPDATE_STATE_RUN) {
				nanSchedNegoApplyCustChnlList(prAdapter);
				prNegoCtrl->u4SyncPeerSchRecIdx =
					0; /* reset to first record */

				/* update availability schedule to FW */
				nanSchedCmdUpdateAvailability(prAdapter);
			}

			do {
				rStatus = WLAN_STATUS_SUCCESS;

				prPeerSchRec =
					nanSchedNegoSyncSchFindNextPeerSchRec(
						prAdapter);
				if (prPeerSchRec == NULL)
					eNextState =
					ENUM_NAN_SYNC_SCH_UPDATE_STATE_DONE;
				else {
#ifdef NAN_UNUSED
					/* check if schedule conflict */
					if ((nanSchedPeerChkDataPath(
					  prAdapter, prPeerSchRec)
					== WLAN_STATUS_SUCCESS) &&
					(nanSchedPeerChkRanging(
					prAdapter, prPeerSchRec)
					== WLAN_STATUS_SUCCESS)) {
						continue;
				    }
#endif

				    /* start schedule update */
				    pucNmiAddr = prPeerSchRec->prPeerSchDesc
							     ->aucNmiAddr;
				    DBGLOG(NAN, DEBUG,
					"Update Sch for %02x:%02x:%02x:%02x:%02x:%02x\n",
					pucNmiAddr[0], pucNmiAddr[1],
					pucNmiAddr[2], pucNmiAddr[3],
					pucNmiAddr[4], pucNmiAddr[5]);
				    kalMemZero(&rNanUpdateSchParam,
					sizeof(rNanUpdateSchParam));
				    kalMemCopy(rNanUpdateSchParam
					.aucPeerDataAddress,
					pucNmiAddr, MAC_ADDR_LEN);
				    rStatus = nanUpdateNdlSchedule(
					prAdapter, &rNanUpdateSchParam);
				    if (rStatus == WLAN_STATUS_SUCCESS)
					eNextState =
					    ENUM_NAN_SYNC_SCH_UPDATE_STATE_RUN;
				}
			} while (rStatus != WLAN_STATUS_SUCCESS);
			break;

		case ENUM_NAN_SYNC_SCH_UPDATE_STATE_RUN:
			if (prNegoCtrl->ucNegoTransNum == 0)
				eNextState =
					ENUM_NAN_SYNC_SCH_UPDATE_STATE_CHECK;
			break;

		case ENUM_NAN_SYNC_SCH_UPDATE_STATE_DONE:

			/* Fixme: send schedule update notification frames
			 * to all peers
			 */

			nanSchedDbgDumpTimelineDb(prAdapter, __func__,
						  __LINE__);

			eNextState = ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE;
			break;

		default:
			break;
		}
	} while (eLastState != eNextState);

	return eNextState;
}

/* nanSchedNegoApplyCustChnlList() merged arCustChnlList into arChnlList.
 * This function removes the slots marked by arCustChnlList to apply new
 * Customize bitmaps.
 */
static uint32_t nanSchedNegoRemoveCustChnlList(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx;
	uint32_t u4SlotIdx;
	union _NAN_BAND_CHNL_CTRL rCustChnl, rCommitChnl;
	struct _NAN_CHANNEL_TIMELINE_T *prCustChnlTimeline;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

		prCustChnlTimeline = prNanTimelineMgmt->arCustChnlList;
		for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
		     u4Idx++, prCustChnlTimeline++) {
			if (prCustChnlTimeline->fgValid == FALSE)
				continue;

			rCustChnl = prCustChnlTimeline->rChnlInfo;

			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				if (!NAN_IS_AVAIL_MAP_SET(
					prCustChnlTimeline->au4AvailMap,
					u4SlotIdx))
					continue;

				rCommitChnl = nanQueryChnlInfoBySlot(prAdapter,
					u4SlotIdx, NULL, TRUE, szTimeLineIdx);
				if (rCommitChnl.u4PrimaryChnl == 0) {
					rRetStatus = nanSchedAddCrbToChnlList(
					      prAdapter, &rCustChnl,
					      u4SlotIdx, 1,
					      ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					      TRUE, NULL);
					if (rRetStatus != WLAN_STATUS_SUCCESS)
						DBGLOG(NAN, DEBUG,
						"nanSchedAddCrbToChnlList fail@%d\n",
						__LINE__);
					continue;
				}

				if (rCustChnl.u4RawData ==
					rCommitChnl.u4RawData) {
					nanSchedDeleteCrbFromChnlList(prAdapter,
						u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
						TRUE, szTimeLineIdx);
					NAN_DW_DBGLOG(NAN, TRACE,
						      TRUE, u4SlotIdx,
						      "Delete u4Idx=%u, u4SlotIdx=%u, rCustChnl.u4RawData=0x%08x\n",
						      u4Idx, u4SlotIdx,
						      rCustChnl.u4RawData);
				}
			}
		}
	}

	return rRetStatus;
}

/**
 * Original call flow the later caller will reset the previously configured FAW.
 * nanSchedNegoCustFawResetCmd()
 * nanSchedNegoCustFawConfigCmd()
 * nanSchedNegoCustFawApplyCmd()
 *
 * New call flow:
 * nanSchedNegoCustFawRemoveEntry(): remove entries to the custom FAW DB
 * nanSchedNegoCustFawAddEntry(): add entries to the custom FAW DB
 * nanSchedNegoCustFawReconfigure(): invokes nanSchedNegoCustFawResetCmd(),
 * nanSchedNegoCustFawConfigCmd(), and nanSchedNegoCustFawApplyCmd() internally
 * by Config with entries in the custom FAW DB.
 */

uint32_t nanSchedNegoCustFawAddEntry(struct ADAPTER *prAdapter,
				     struct _NAN_CUST_FAW_ENTRY *prNewEntry)
{
	struct _NAN_SCHEDULER_T *prScheduler = nanGetScheduler(prAdapter);
	size_t n = ARRAY_SIZE(prScheduler->arCustFawEntry);
	struct _NAN_CUST_FAW_ENTRY *prCustFawEntry;
	size_t i;
	u_int8_t fgUpdated = FALSE;

	prCustFawEntry = prScheduler->arCustFawEntry;
	for (i = 0; i < n; i++) {
		if (prCustFawEntry[i].pcTag &&
		    prCustFawEntry[i].pcTag == prNewEntry->pcTag &&
		    prCustFawEntry[i].ucOpChannel == prNewEntry->ucOpChannel &&
		    prCustFawEntry[i].eBand == prNewEntry->eBand &&
		    prCustFawEntry[i].u4Bitmap == prNewEntry->u4Bitmap) {
			break;
		}

		if (prCustFawEntry[i].pcTag)
			continue;

		prCustFawEntry[i] = *prNewEntry;
		fgUpdated = TRUE;
		break;
	}

	DBGLOG(NAN, DEBUG,
	       "%s to %zu, %s, ch=%u, band=%u, bitmap=%02x-%02x-%02x-%02x\n",
	       fgUpdated ? "Add" : "Duplicate",
	       i, prNewEntry->pcTag, prNewEntry->ucOpChannel, prNewEntry->eBand,
	       ((uint8_t *)&prNewEntry->u4Bitmap)[0],
	       ((uint8_t *)&prNewEntry->u4Bitmap)[1],
	       ((uint8_t *)&prNewEntry->u4Bitmap)[2],
	       ((uint8_t *)&prNewEntry->u4Bitmap)[3]);

	if (i == n || !fgUpdated)
		return WLAN_STATUS_FAILURE;

	return WLAN_STATUS_SUCCESS;
}

/* Remove all entries in prScheduler->arCustFawEntry by matching pcTag */
uint32_t nanSchedNegoCustFawRemoveEntry(struct ADAPTER *prAdapter,
					const char *pcTag)
{
	struct _NAN_SCHEDULER_T *prScheduler = nanGetScheduler(prAdapter);
	size_t n = ARRAY_SIZE(prScheduler->arCustFawEntry);
	struct _NAN_CUST_FAW_ENTRY *prCustFawEntry;
	size_t src;
	size_t dst;

	prCustFawEntry = prScheduler->arCustFawEntry;
	for (src = 0, dst = 0; src < n; src++) {
		if (!prCustFawEntry[src].pcTag)
			break;

		if (prCustFawEntry[src].pcTag != pcTag) {
			prCustFawEntry[dst] = prCustFawEntry[src];
			dst++;
		}
	}
	kalMemZero(&prCustFawEntry[dst],
		   sizeof(struct _NAN_CUST_FAW_ENTRY) * (n - dst));

	DBGLOG(NAN, DEBUG, "remove all entries of %s, remain=%zu\n",
	       pcTag, dst);

	return WLAN_STATUS_SUCCESS;
}

void nanSchedNegoCustFawReconfigure(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prScheduler = nanGetScheduler(prAdapter);
	struct _NAN_CUST_FAW_ENTRY *prCustFawEntry;
	size_t i;

	nanSchedNegoCustFawResetCmd(prAdapter);

	prCustFawEntry = prScheduler->arCustFawEntry;
	for (i = 0; i < ARRAY_SIZE(prScheduler->arCustFawEntry); i++) {
		if (!prCustFawEntry[i].pcTag)
			break;

		nanSchedNegoCustFawConfigCmd(prAdapter,
				     prCustFawEntry[i].ucOpChannel,
				     prCustFawEntry[i].eBand,
				     prCustFawEntry[i].u4Bitmap);
	}

	nanSchedNegoCustFawApplyCmd(prAdapter);
}

uint32_t nanSchedNegoCustFawResetCmd(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	DBGLOG(NAN, DEBUG, "Enter\n");

	/* Clear arChnlList set by arCustChnlList */
	nanSchedNegoRemoveCustChnlList(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);
		kalMemZero(prNanTimelineMgmt->arCustChnlList,
			   sizeof(prNanTimelineMgmt->arCustChnlList));
	}

	return rRetStatus;
}


static void nanRemoveFawForDw(
	struct ADAPTER *prAdapter,
	struct _NAN_SCHEDULER_T *prScheduler,
	enum ENUM_BAND eBand,
	uint32_t *u4SlotBitmap)
{
	uint32_t u4ClearBits = 0;
	size_t szNanActiveTimelineNum =
		nanGetActiveTimelineMgmtNum(prAdapter);

#if (NAN_TIMELINE_MGMT_SIZE > 1)
	if (eBand == BAND_2G4)
#endif
		if (prScheduler->fgEn2g)
			u4ClearBits |= BIT(NAN_2G_DW_INDEX);

#if (NAN_TIMELINE_MGMT_SIZE > 1)
	if (eBand != BAND_2G4)
#endif
		if (prScheduler->fgEn5gH || prScheduler->fgEn5gL)
			u4ClearBits |= BIT(NAN_5G_DW_INDEX);

	if (szNanActiveTimelineNum < NAN_TIMELINE_MGMT_SIZE) {
		if (prScheduler->fgEn2g)
			u4ClearBits |= BIT(NAN_2G_DW_INDEX);
		if (prScheduler->fgEn5gH || prScheduler->fgEn5gL)
			u4ClearBits |= BIT(NAN_5G_DW_INDEX);
	}

	/* single map */
	if (szNanActiveTimelineNum < 2) {
		if (prScheduler->fgEn2g)
			u4ClearBits |= BIT(NAN_2G_DEFAULT_NDC_INDEX);
		if (prScheduler->fgEn5gH || prScheduler->fgEn5gL)
			u4ClearBits |= BIT(NAN_5G_DEFAULT_NDC_INDEX);
	}

	*u4SlotBitmap &= ~u4ClearBits;
}

/**
 * nanSchedNegoCustFawConfigCmd() - Find a channel matched custom timeline
 *				  to set bitmap
 * @prAdapter: pointer to adapter
 * @eBand: band requested
 * @ucChnl: channel <36: 2.4GHz; >= 36: 5GHz
 * @u4SlotBitmap: bitmap to set
 *
 * Return: WLAN_STATUS_SUCCESS
 */
uint32_t nanSchedNegoCustFawConfigCmd(struct ADAPTER *prAdapter,
					uint8_t ucChnl, enum ENUM_BAND eBand,
				    uint32_t u4SlotBitmap)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo;
	uint32_t u4Bw;
	uint32_t u4Idx, u4DwIdx;
	uint32_t u4TargetIdx = NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlMajorTimeline;
	struct _NAN_CHANNEL_TIMELINE_T temp = { 0 };
	struct _NAN_SCHEDULER_T *prScheduler;
	uint8_t ucPrimaryChnl = g_r5gDwChnl.u4PrimaryChnl;

	prScheduler = nanGetScheduler(prAdapter);
	prNanTimelineMgmt = nanGetTimelineMgmtByBand(prAdapter, eBand);
	u4Bw = nanSchedConfigGetAllowedBw(prAdapter, eBand);
	rChnlInfo = nanRegGenNanChnlInfoByPriChannel(ucChnl, u4Bw, eBand);

	if (nanGetActiveTimelineMgmtNum(prAdapter) > 1 && eBand == BAND_2G4)
		ucPrimaryChnl = g_r2gDwChnl.u4PrimaryChnl;

	DBGLOG(NAN, DEBUG, "B:%u, Chnl:%u, bitmap=%02x-%02x-%02x-%02x\n",
	       eBand, ucChnl,
	       ((uint8_t *)&u4SlotBitmap)[0],
	       ((uint8_t *)&u4SlotBitmap)[1],
	       ((uint8_t *)&u4SlotBitmap)[2],
	       ((uint8_t *)&u4SlotBitmap)[3]);

	/* DW slots are reserved */
	nanRemoveFawForDw(prAdapter, prScheduler, eBand, &u4SlotBitmap);

	prChnlMajorTimeline = &prNanTimelineMgmt->arCustChnlList[0];

	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
		prChnlTimeline = &prNanTimelineMgmt->arCustChnlList[u4Idx];
		if (prChnlTimeline->fgValid == FALSE) {
			if (u4TargetIdx == NAN_TIMELINE_MGMT_CHNL_LIST_NUM)
				u4TargetIdx = u4Idx; /* the first invalid */

			continue;
		}

		if (prChnlTimeline->rChnlInfo.u4PrimaryChnl == ucChnl) {
			u4TargetIdx = u4Idx; /* the first channel match */
			continue;
		}

		/* For not matched timeline, clear the bits requested to set */
		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++)
			prChnlTimeline->au4AvailMap[u4DwIdx] &= (~u4SlotBitmap);
		prChnlTimeline->i4Num = nanUtilCheckBitOneCnt(
					prChnlTimeline->au4AvailMap,
					sizeof(prChnlTimeline->au4AvailMap));
		if (prChnlTimeline->i4Num == 0)
			prChnlTimeline->fgValid = FALSE;
	}

	DBGLOG(NAN, DEBUG, "Found TargetIdx=%u", u4TargetIdx);
	/* channel matched Timeline, or invalid one as new entry to set */
	if (u4TargetIdx != NAN_TIMELINE_MGMT_CHNL_LIST_NUM) {
		prChnlTimeline =
			&prNanTimelineMgmt->arCustChnlList[u4TargetIdx];
		prChnlTimeline->rChnlInfo = rChnlInfo;
		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++)
			prChnlTimeline->au4AvailMap[u4DwIdx] |= u4SlotBitmap;
		prChnlTimeline->i4Num = nanUtilCheckBitOneCnt(
					prChnlTimeline->au4AvailMap,
					sizeof(prChnlTimeline->au4AvailMap));
		if (prChnlTimeline->i4Num)
			prChnlTimeline->fgValid = TRUE;
	}

	/* Swap with major timeline
	 * if the new added channel is NAN social channel and
	 * the Major timeline is not social channel
	 */
	if (u4TargetIdx != 0 &&
	    prChnlTimeline->rChnlInfo.u4PrimaryChnl == ucPrimaryChnl &&
	    prChnlMajorTimeline->rChnlInfo.u4PrimaryChnl != ucPrimaryChnl) {
		DBGLOG(NAN, TRACE,
		       "swap [%u]ch=%u with [0]ch=%u",
		       u4TargetIdx, prChnlTimeline->rChnlInfo.u4PrimaryChnl,
		       prChnlMajorTimeline->rChnlInfo.u4PrimaryChnl);
		temp = *prChnlMajorTimeline;
		*prChnlMajorTimeline = *prChnlTimeline;
		*prChnlTimeline = temp;
	}

	/* Dump valid timelines */
	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
		prChnlTimeline = &prNanTimelineMgmt->arCustChnlList[u4Idx];
		if (prChnlTimeline->fgValid == FALSE)
			continue;

		DBGLOG(NAN, DEBUG,
		       "[%d] Raw:0x%x, Cust Chnl:%d, Class:%d, Bw:%d Bitmap:%02x-%02x-%02x-%02x\n",
		       u4Idx,
		       prChnlTimeline->rChnlInfo.u4RawData,
		       prChnlTimeline->rChnlInfo.u4PrimaryChnl,
		       prChnlTimeline->rChnlInfo.u4OperatingClass,
		       nanRegGetBw(prChnlTimeline->rChnlInfo.u4OperatingClass),
		       ((uint8_t *)prChnlTimeline->au4AvailMap)[0],
		       ((uint8_t *)prChnlTimeline->au4AvailMap)[1],
		       ((uint8_t *)prChnlTimeline->au4AvailMap)[2],
		       ((uint8_t *)prChnlTimeline->au4AvailMap)[3]);
		nanUtilDump(prAdapter, "AvailMap",
			    (uint8_t *)prChnlTimeline->au4AvailMap,
			    sizeof(prChnlTimeline->au4AvailMap));
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t nanSchedNegoCustFawApplyCmd(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;

	nanSchedNegoSyncSchUpdateFsmStep(
		prAdapter, ENUM_NAN_SYNC_SCH_UPDATE_STATE_PREPARE);

	return rRetStatus;
}

static
uint32_t nanSchedCheckBandNDLSlotCommitNum(struct ADAPTER *prAdapter,
					   enum ENUM_BAND eBand)
{
	uint32_t u4BitCount = 0;
	size_t szTimeLineIdx = 0;
	size_t szIdx = 0;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline;
	uint32_t u4NDLSlotAvailMap = 0;

	for (szTimeLineIdx = 0; szTimeLineIdx < NAN_TIMELINE_MGMT_SIZE;
	     szTimeLineIdx++) {
		if (szTimeLineIdx !=
		    nanGetTimelineMgmtIndexByBand(prAdapter, eBand))
			continue;

		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

		for (szIdx = 0;
		     szIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; szIdx++) {
			prChnlTimeline = &prNanTimelineMgmt->arChnlList[szIdx];

			if (prChnlTimeline->fgValid == FALSE ||
			    prChnlTimeline->i4Num <= 0)
				continue;

			if (nanRegGetNanChnlBand(prChnlTimeline->rChnlInfo) !=
			    eBand)
				continue;

			/* Only check input band commit slot num */
			u4NDLSlotAvailMap |=
				(prChnlTimeline->au4AvailMap[0] &
				 nanGetNdlSlots(prAdapter));
		}
	}

	u4BitCount = nanUtilCheckBitOneCnt(&u4NDLSlotAvailMap,
					   sizeof(uint32_t));
	DBGLOG(NAN, DEBUG, "BitCount=%u, %02x-%02x-%02x-%02x\n",
	       u4BitCount,
	       ((uint8_t *)&u4NDLSlotAvailMap)[0],
	       ((uint8_t *)&u4NDLSlotAvailMap)[1],
	       ((uint8_t *)&u4NDLSlotAvailMap)[2],
	       ((uint8_t *)&u4NDLSlotAvailMap)[3]);
	return u4BitCount;
}

void
nanSchedNegoDispatchTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam)
{
	uint32_t u4Idx;
	uint32_t u4SchIdx;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_DATA_ENGINE_SCHEDULE_TOKEN_T *prDataEngineToken = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
#if (CFG_SUPPORT_NAN_6G == 1) && (CFG_SUPPORT_NAN_11BE == 1)
	u_int8_t fgIsEhtRescheduleNewNDL = FALSE;
#endif

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_IDLE)
		return;

	DBGLOG(NAN, DEBUG, "Num:%d\n", prNegoCtrl->ucNegoTransNum);

	while (prNegoCtrl->ucNegoTransNum > 0) {
		u4Idx = prNegoCtrl->ucNegoTransHeadPos;
		prNegoCtrl->ucNegoTransHeadPos =
			(prNegoCtrl->ucNegoTransHeadPos + 1) %
			NAN_CRB_NEGO_MAX_TRANSACTION;
		prNegoCtrl->ucNegoTransNum--;

		u4SchIdx = nanSchedLookupPeerSchRecordIdx(
			prAdapter, prNegoCtrl->rNegoTrans[u4Idx].aucNmiAddr);
		if (u4SchIdx == NAN_INVALID_SCH_RECORD) {
			DBGLOG(NAN, ERROR,
				"u4SchIdx equals NAN_INVALID_SCH_RECORD!\n");
			DBGLOG(NAN, ERROR, "invalid NegoTranIdx:%u\n", u4Idx);
			continue;
		}

		DBGLOG(NAN, ERROR, "NegoTranIdx:%u\n", u4Idx);
		/* Release slot for first NDL in reschedule token */
		prDataEngineToken =
			(struct _NAN_DATA_ENGINE_SCHEDULE_TOKEN_T *)
			prNegoCtrl->rNegoTrans[u4Idx].pvToken;
		if (prDataEngineToken)
			prNDL = prDataEngineToken->prNDL;
		if (prNDL == NULL)
			DBGLOG(NAN, ERROR, "prNDL is NULL\n");

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1) && \
	(CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
#if (CFG_SUPPORT_NAN_6G == 1) && (CFG_SUPPORT_NAN_11BE == 1)
		fgIsEhtRescheduleNewNDL =
			prNegoCtrl->rNegoTrans[u4Idx].fgIsEhtRescheduleNewNDL;
#endif

		if (prNegoCtrl->rNegoTrans[u4Idx].fgTriggerReschedNewNDL) {
#if (CFG_SUPPORT_NAN_6G == 1) && (CFG_SUPPORT_NAN_11BE == 1)
			if (nanSchedCheckBandNDLSlotCommitNum(prAdapter,
							      BAND_6G) > 0)
				ReleaseNanSlotsForSchedulePrep(prAdapter,
						       NEW_NDL,
						       fgIsEhtRescheduleNewNDL);
#endif

#if (CFG_SUPPORT_NAN_EXT == 1)
#if (CFG_SUPPORT_NAN_6G == 1) && (CFG_SUPPORT_NAN_11BE == 1)
			if (fgIsEhtRescheduleNewNDL)
				nanEnableEht(prAdapter, FALSE);
#endif /* CFG_SUPPORT_NAN_6G && CFG_SUPPORT_NAN_11BE */
#endif /* CFG_SUPPORT_NAN_EXT */
		}
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
		if (prNegoCtrl->rNegoTrans[u4Idx].fgIs3rd6GNewNDL) {
			prNegoCtrl->fgInsistNDLSlot5GMode = TRUE;
			DBGLOG(NAN, INFO, "Enter NDL slot 5G Mode\n");
		}
#endif

		nanSchedNegoInitDb(prAdapter, u4SchIdx,
				   prNegoCtrl->rNegoTrans[u4Idx].eType,
				   prNegoCtrl->rNegoTrans[u4Idx].eRole);

		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {
			prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
						szTimeLineIdx);
			prNanTimelineMgmt->fgChkCondAvailability = TRUE;
		}

		nanSchedNegoDumpState(prAdapter,
			(uint8_t *) __func__, __LINE__);

		prNegoCtrl->rNegoTrans[u4Idx].pfCallback(
			prAdapter, prNegoCtrl->rNegoTrans[u4Idx].aucNmiAddr,
			prNegoCtrl->rNegoTrans[u4Idx].eType,
			prNegoCtrl->rNegoTrans[u4Idx].eRole,
			prNegoCtrl->rNegoTrans[u4Idx].pvToken);
		break;
	}
}

void nanSchedNegoInitDb(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
		   enum _ENUM_NAN_NEGO_TYPE_T eType,
		   enum _ENUM_NAN_NEGO_ROLE_T eRole)
{
	uint32_t u4Idx;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	uint32_t u4DftNdlQosLatencyVal = NAN_INVALID_QOS_MAX_LATENCY;
	uint32_t u4DftNdlQosQuotaVal = NAN_INVALID_QOS_MIN_SLOTS;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	prNegoCtrl->u4SchIdx = u4SchIdx;
	prNegoCtrl->eRole = eRole;
	prNegoCtrl->eType = eType;
	if (eRole == ENUM_NAN_NEGO_ROLE_INITIATOR)
		prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_INITIATOR;
	else {
		nanSchedPeerPurgeOldCrb(prAdapter, u4SchIdx);
		prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_RESPONDER;
	}

	prNegoCtrl->rSelectedNdcCtrl.fgValid = FALSE;
	kalMemZero(&prNegoCtrl->rSelectedNdcCtrl,
		   sizeof(prNegoCtrl->rSelectedNdcCtrl));
	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx].ucMapId =
			NAN_INVALID_MAP_ID;
		prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx].ucLocalMapId =
			NAN_INVALID_MAP_ID;
		prNegoCtrl->arRangingTimeline[szTimeLineIdx].ucMapId =
			NAN_INVALID_MAP_ID;
		prNegoCtrl->arRangingTimeline[szTimeLineIdx].ucLocalMapId =
			NAN_INVALID_MAP_ID;

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);

		prNanTimelineMgmt->fgChkCondAvailability = FALSE;

		for (u4Idx = 0;
			u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++)
			prNanTimelineMgmt->arCondChnlList[u4Idx]
					.fgValid = FALSE;
	}

	prNegoCtrl->u4QosMaxLatency = NAN_INVALID_QOS_MAX_LATENCY;
	prNegoCtrl->u4QosMinSlots = NAN_INVALID_QOS_MIN_SLOTS;
	prNegoCtrl->u4NegoQosMaxLatency = NAN_INVALID_QOS_MAX_LATENCY;
	prNegoCtrl->u4NegoQosMinSlots = NAN_INVALID_QOS_MIN_SLOTS;

	kalMemZero(prNegoCtrl->aau4AvailSlots,
		   sizeof(prNegoCtrl->aau4AvailSlots));
	kalMemZero(prNegoCtrl->aau4UnavailSlots,
		   sizeof(prNegoCtrl->aau4UnavailSlots));
	kalMemZero(prNegoCtrl->aau4CondSlots,
		sizeof(prNegoCtrl->aau4CondSlots));

	if (prAdapter->rWifiVar.u2DftNdlQosLatencyVal)
		u4DftNdlQosLatencyVal =
			prAdapter->rWifiVar.u2DftNdlQosLatencyVal;

	if (prAdapter->rWifiVar.ucDftNdlQosQuotaVal)
		u4DftNdlQosQuotaVal = prAdapter->rWifiVar.ucDftNdlQosQuotaVal;

	nanSchedNegoAddQos(prAdapter, u4DftNdlQosQuotaVal,
			   u4DftNdlQosLatencyVal);
}

uint32_t
nanSchedNegoStart(
	struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
	enum _ENUM_NAN_NEGO_TYPE_T eType, enum _ENUM_NAN_NEGO_ROLE_T eRole,
	void (*pfCallback)(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			   enum _ENUM_NAN_NEGO_TYPE_T eType,
			   enum _ENUM_NAN_NEGO_ROLE_T eRole, void *pvToken),
	void *pvToken)
{
	uint32_t u4Idx;
	uint32_t u4SchIdx;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken;
#endif

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	DBGLOG(NAN, TRACE, "IN\n");

	u4SchIdx = nanSchedLookupPeerSchRecordIdx(prAdapter, pucNmiAddr);
	if (u4SchIdx == NAN_INVALID_SCH_RECORD)
		u4SchIdx =
			nanSchedAcquirePeerSchedRecord(prAdapter, pucNmiAddr);
	if (u4SchIdx == NAN_INVALID_SCH_RECORD)
		return WLAN_STATUS_FAILURE;

	if (prNegoCtrl->ucNegoTransNum >= NAN_CRB_NEGO_MAX_TRANSACTION)
		return WLAN_STATUS_FAILURE;

	u4Idx = (prNegoCtrl->ucNegoTransHeadPos + prNegoCtrl->ucNegoTransNum) %
		NAN_CRB_NEGO_MAX_TRANSACTION;
	prNegoCtrl->ucNegoTransNum++;

	kalMemCopy(prNegoCtrl->rNegoTrans[u4Idx].aucNmiAddr, pucNmiAddr,
		   MAC_ADDR_LEN);
	prNegoCtrl->rNegoTrans[u4Idx].eRole = eRole;
	prNegoCtrl->rNegoTrans[u4Idx].eType = eType;
	prNegoCtrl->rNegoTrans[u4Idx].pfCallback = pfCallback;
	prNegoCtrl->rNegoTrans[u4Idx].pvToken = pvToken;

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
	/* Default value for responder and NDP setup
	 * Only get reschedule source when reschedule happens
	 */
	prNegoCtrl->rNegoTrans[u4Idx].eReschedSrc = RESCHEDULE_NULL;
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1 && CFG_SUPPORT_NAN_11BE == 1)
	prNegoCtrl->rNegoTrans[u4Idx].fgIsEhtReschedule = FALSE;
	prNegoCtrl->rNegoTrans[u4Idx].fgIsEhtRescheduleNewNDL = FALSE;
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	prNegoCtrl->rNegoTrans[u4Idx].fgTriggerReschedNewNDL = FALSE;
	prNegoCtrl->rNegoTrans[u4Idx].fgIs3rd6GNewNDL = FALSE;
#endif

	prNegoCtrl->rNegoTrans[u4Idx].u4NotChoose6GCnt = 0;
	prNegoCtrl->rNegoTrans[u4Idx].fgCounterCountry = FALSE;
	prNegoCtrl->rNegoTrans[u4Idx].fgPropose2gConditional = FALSE;

	prNDL = nanDataUtilSearchNdlByMac(prAdapter, pucNmiAddr);
	if (prNDL == NULL)
		DBGLOG(NAN, ERROR, "NDL is NULL\n");
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	else {
		prNegoCtrl->rNegoTrans[u4Idx].fgTriggerReschedNewNDL =
			prNDL->fgTriggerReschedNewNDL;

		if (prNDL->fgTriggerReschedNewNDL) {
			prNDL->fgTriggerReschedNewNDL = FALSE;
#if (CFG_SUPPORT_NAN_11BE == 1)
			prNegoCtrl->rNegoTrans[u4Idx].fgIsEhtRescheduleNewNDL =
				prNDL->fgIsEhtReschedule;
#endif
		}

		prNegoCtrl->rNegoTrans[u4Idx].fgIs3rd6GNewNDL =
			prNDL->fgIs3rd6GNewNDL;
		prNDL->fgIs3rd6GNewNDL = FALSE;
	}
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	if (eRole == ENUM_NAN_NEGO_ROLE_INITIATOR && prNDL &&
	    prNDL->eCurrentNDLMgmtState == NDL_REQUEST_SCHEDULE_NDL) {
		prReScheduleToken = NULL;
		if (nanSchedGetRescheduleToken != NULL) {
			prReScheduleToken =
				nanSchedGetRescheduleToken(prAdapter);
		} else {
			DBGLOG(NAN, ERROR, "Reschedule func ptr is NULL\n");
		}

		if (prReScheduleToken != NULL) {
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
			prNegoCtrl->rNegoTrans[u4Idx].eReschedSrc =
				prReScheduleToken->ucEvent;
#endif
#if (CFG_SUPPORT_NAN_11BE == 1)
			prNegoCtrl->rNegoTrans[u4Idx].fgIsEhtReschedule =
				prReScheduleToken->fgIsEhtReschedule;
#endif
		} else {
			DBGLOG(NAN, ERROR, "Reschedule token is NULL\n");
		}
	}
#endif

	nanSchedPeerPrepareNegoState(prAdapter, pucNmiAddr);

	if (prNegoCtrl->eState == ENUM_NAN_CRB_NEGO_STATE_IDLE) {
		cnmTimerStopTimer(prAdapter,
				  &(prNegoCtrl->rCrbNegoDispatchTimer));
		cnmTimerStartTimer(prAdapter,
				   &(prNegoCtrl->rCrbNegoDispatchTimer), 1);
	}

	return WLAN_STATUS_PENDING;
}


void
nanSchedNegoStop(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	size_t szIdx;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	DBGLOG(NAN, TRACE, "IN\n");

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	prPeerSchRecord =
		nanSchedGetPeerSchRecord(prAdapter, prNegoCtrl->u4SchIdx);

	prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_IDLE;

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);
		prNanTimelineMgmt->fgChkCondAvailability = FALSE;

		for (szIdx = 0;
			szIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; szIdx++)
			prNanTimelineMgmt->arCondChnlList[szIdx]
					.fgValid = FALSE;
	}

	nanSchedNegoDumpState(prAdapter, (uint8_t *) __func__, __LINE__);

	if (prPeerSchRecord != NULL) {
		nanSchedPeerCompleteNegoState(
			prAdapter, prPeerSchRecord->aucNmiAddr);

		if (prPeerSchRecord->fgActive &&
			!prPeerSchRecord->fgUseDataPath &&
		    !prPeerSchRecord->fgUseRanging) {
			nanSchedReleasePeerSchedRecord(
				prAdapter, prNegoCtrl->u4SchIdx);
		}
	} else
		DBGLOG(NAN, ERROR, "NULL prPeerSchRecord\n");

	nanSchedUpdateActiveNdcBands(prAdapter);

	cnmTimerStopTimer(prAdapter, &(prNegoCtrl->rCrbNegoDispatchTimer));
	if ((prNegoCtrl->ucNegoTransNum > 0) &&
	    nanGetScheduler(prAdapter)->fgInit)
		cnmTimerStartTimer(prAdapter,
				   &(prNegoCtrl->rCrbNegoDispatchTimer), 1);
	else
		nanSchedNegoSyncSchUpdateFsmStep(
			prAdapter, prNegoCtrl->eSyncSchUpdateCurrentState);
}

unsigned char
nanSchedNegoInProgress(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_IDLE)
		return TRUE;

	if (prNegoCtrl->i4InNegoContext) {
		DBGLOG(NAN, DEBUG, "In NEGO state, i4InNegoContext:%d\n",
		       prNegoCtrl->i4InNegoContext);
		return TRUE;
	}

	return FALSE;
}

uint32_t nanSchedNegoApplyCustChnlList(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx1;
	uint32_t u4SlotIdx;
	union _NAN_BAND_CHNL_CTRL rCustChnl, rCommitChnl;
	struct _NAN_CHANNEL_TIMELINE_T *prCustChnlTimeline;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	enum _ENUM_CNM_CH_CONCURR_T eConcurr;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);

		prCustChnlTimeline = &prNanTimelineMgmt->arCustChnlList[0];
		for (u4Idx1 = 0; u4Idx1 < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
		     u4Idx1++, prCustChnlTimeline++) {
			if (prCustChnlTimeline->fgValid == FALSE)
				continue;
			rCustChnl = prCustChnlTimeline->rChnlInfo;

			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				if (!NAN_IS_AVAIL_MAP_SET(
					prCustChnlTimeline->au4AvailMap,
					u4SlotIdx))
					continue;

				rCommitChnl = nanQueryChnlInfoBySlot(prAdapter,
					u4SlotIdx, NULL, TRUE, szTimeLineIdx);
				if (rCommitChnl.u4PrimaryChnl == 0) {
					NAN_DW_DBGLOG(NAN, TRACE,
						TRUE, u4SlotIdx,
						"Add u4Idx=%u, u4SlotIdx=%u, rCustChnl.u4RawData=0x%08x\n",
						u4Idx1, u4SlotIdx,
						rCustChnl.u4RawData);
					rRetStatus = nanSchedAddCrbToChnlList(
					      prAdapter, &rCustChnl,
					      u4SlotIdx, 1,
					      ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					      TRUE, NULL);
					if (rRetStatus != WLAN_STATUS_SUCCESS)
						DBGLOG(NAN, DEBUG,
						       "nanSchedAddCrbToChnlList fail@%d\n",
						       __LINE__);
					continue;
				}

				eConcurr = nanSchedChkConcurrOp(rCustChnl,
						rCommitChnl);
				if (eConcurr == CNM_CH_CONCURR_MCC) {
					NAN_DW_DBGLOG(NAN, TRACE,
						TRUE, u4SlotIdx,
						"Clear u4Idx=%u, u4SlotIdx=%u, rCustChnl.u4RawData=0x%08x\n",
						u4Idx1, u4SlotIdx,
						rCustChnl.u4RawData);
					nanSchedDeleteCrbFromChnlList(prAdapter,
						u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
						TRUE, szTimeLineIdx);
				}

				NAN_DW_DBGLOG(NAN, TRACE, TRUE, u4SlotIdx,
				       "Set u4Idx=%u, u4SlotIdx=%u, rCustChnl.u4RawData=0x%08x\n",
				       u4Idx1, u4SlotIdx, rCustChnl.u4RawData);
				rRetStatus = nanSchedAddCrbToChnlList(prAdapter,
					&rCustChnl, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				if (rRetStatus != WLAN_STATUS_SUCCESS)
					DBGLOG(NAN, DEBUG,
					       "nanSchedAddCrbToChnlList fail@%d\n",
					       __LINE__);
			}
		}
	}
	return rRetStatus;
}

uint32_t
nanSchedNegoValidateCondChnlList(struct ADAPTER *prAdapter,
	size_t szTimeLineIdx)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SlotIdx;
	uint32_t u4LocalChnl;
	uint32_t u4RmtChnl;
	uint32_t u4AvailDbIdx;
	uint32_t u4SchIdx;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;

	/* check if local & remote sides both in the same channel during
	 * the conditional window
	 */
	for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
		u4LocalChnl =
			nanQueryPrimaryChnlBySlot(prAdapter, u4SlotIdx, FALSE,
				szTimeLineIdx);
		if (u4LocalChnl == 0)
			continue;

		/* Fixme */
		u4RmtChnl = 0;
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByID(
				    prAdapter, u4SchIdx, u4AvailDbIdx) == FALSE)
				continue;

			u4RmtChnl = nanQueryPeerPrimaryChnlBySlot(
				prAdapter, u4SchIdx, u4AvailDbIdx, u4SlotIdx,
				TRUE);
			if (u4RmtChnl == 0)
				u4RmtChnl = nanQueryPeerPrimaryChnlBySlot(
					prAdapter, u4SchIdx, u4AvailDbIdx,
					u4SlotIdx, FALSE);

			if (u4RmtChnl == u4LocalChnl)
				break;
		}

		if (u4RmtChnl != u4LocalChnl)
			nanSchedDeleteCrbFromChnlList(prAdapter, u4SlotIdx, 1,
				ENUM_TIME_BITMAP_CTRL_PERIOD_8192, FALSE,
				szTimeLineIdx);
	}

	return rRetStatus;
}

uint32_t
nanSchedNegoCommitCondChnlList(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx1;
	uint32_t u4SlotIdx;
	union _NAN_BAND_CHNL_CTRL rCondChnl;
	struct _NAN_CHANNEL_TIMELINE_T *prCondChnlTimeline;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
					szTimeLineIdx);

		/* check local & remote availability to remove conflict
		 * conditional windows
		 */
		nanSchedNegoValidateCondChnlList(prAdapter, szTimeLineIdx);

		/* merge remain conditional windows into committed windows */
		prCondChnlTimeline = &prNanTimelineMgmt->arCondChnlList[0];
		for (u4Idx1 = 0; u4Idx1 < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
		     u4Idx1++, prCondChnlTimeline++) {
			if (prCondChnlTimeline->fgValid == FALSE)
				continue;

			rCondChnl = prCondChnlTimeline->rChnlInfo;
			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				if (!NAN_IS_AVAIL_MAP_SET(
					prCondChnlTimeline->au4AvailMap,
					u4SlotIdx))
					continue;

				/* remove CRB from conditional window list */
				nanSchedDeleteCrbFromChnlList(prAdapter,
					u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, szTimeLineIdx);

				/* add CRB to committed window list */
				rRetStatus = nanSchedAddCrbToChnlList(prAdapter,
					&rCondChnl, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				if (rRetStatus != WLAN_STATUS_SUCCESS)
					DBGLOG(NAN, DEBUG,
					       "nanSchedAddCrbToChnlList fail@%d\n",
					       __LINE__);
			}
		}
	}
	return rRetStatus;
}

enum _ENUM_CHNL_CHECK_T
nanSchedNegoChkChnlConflict(struct ADAPTER *prAdapter, uint32_t u4SlotIdx,
			    unsigned char fgChkRmtCondSlot,
			    size_t szTimeLineIdx)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	uint32_t u4AvailDbIdx;
	uint32_t u4SchIdx;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;
	unsigned char fgValidDB = FALSE;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;

	rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter, u4SlotIdx,
		szTimeLineIdx);

	DBGLOG(NAN, LOUD, "slot: %d, local CH = %d\n",
		u4SlotIdx, rLocalChnlInfo.u4PrimaryChnl);

	/* Case1: No local CH */
	if (rLocalChnlInfo.u4PrimaryChnl == 0)
		return ENUM_CHNL_CHECK_NOT_FOUND;

	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
	     u4AvailDbIdx++) {
		if (nanSchedPeerAvailabilityDbValidByID(prAdapter, u4SchIdx,
							u4AvailDbIdx) == FALSE)
			continue;

		fgValidDB = TRUE;

		rRmtChnlInfo = nanGetPeerChnlInfoBySlot(prAdapter, u4SchIdx,
							u4AvailDbIdx, u4SlotIdx,
							fgChkRmtCondSlot);

		DBGLOG(NAN, LOUD, "Rmt CH = %d\n",
				rRmtChnlInfo.u4PrimaryChnl);

		/* Case2: Has local CH, No peer CH */
		if (rRmtChnlInfo.u4PrimaryChnl == 0)
			return ENUM_CHNL_CHECK_PASS;

		/* Case3: Local & Peer CH are SCC */
		if (nanSchedChkConcurrOp(rLocalChnlInfo, rRmtChnlInfo) !=
		    CNM_CH_CONCURR_MCC)
			return ENUM_CHNL_CHECK_PASS;
	}

	/* Case4: No peer valid Avail DB and
	 * No need to check peer Conditional Avail
	 */
	if (!fgValidDB && !fgChkRmtCondSlot)
		return ENUM_CHNL_CHECK_PASS;

	/* Case5: Others */
	return ENUM_CHNL_CHECK_CONFLICT;
}

uint32_t
nanSchedNegoUpdateDatabase(struct ADAPTER *prAdapter,
			   unsigned char fgChkRmtCondSlot)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	uint32_t i;
	uint32_t u4Slot;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	enum _ENUM_CHNL_CHECK_T eChkChnlResult;
	uint32_t u4RmtCondChnl;
	uint32_t u4SchIdx;
	size_t szTimeLine;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	uint32_t (*pau4FawSlots)[NAN_TOTAL_DW];
	uint32_t (*pau4AvailSlots)[NAN_TOTAL_DW];
	uint32_t (*pau4FreeSlots)[NAN_TOTAL_DW];
	uint32_t (*pau4UnavailSlots)[NAN_TOTAL_DW];

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;

	kalMemZero(prNegoCtrl->aau4AvailSlots,
		   sizeof(prNegoCtrl->aau4AvailSlots));
	kalMemZero(prNegoCtrl->aau4UnavailSlots,
		   sizeof(prNegoCtrl->aau4UnavailSlots));

	pau4FawSlots = prNegoCtrl->aau4FawSlots;
	pau4FreeSlots = prNegoCtrl->aau4FreeSlots;
	pau4AvailSlots = prNegoCtrl->aau4AvailSlots;
	pau4UnavailSlots = prNegoCtrl->aau4UnavailSlots;

	for (szTimeLine = 0; szTimeLine < szNanActiveTimelineNum;
	     szTimeLine++) {
		for (u4Slot = 0; u4Slot < NAN_TOTAL_SLOT_WINDOWS; u4Slot++) {
			if (nanWindowType(prAdapter, u4Slot, szTimeLine)
				== ENUM_NAN_DW) {
				NAN_TIMELINE_SET(pau4UnavailSlots[szTimeLine],
					u4Slot);
				continue;
			}

			eChkChnlResult = nanSchedNegoChkChnlConflict(prAdapter,
						u4Slot, fgChkRmtCondSlot,
						szTimeLine);

			if (eChkChnlResult == ENUM_CHNL_CHECK_NOT_FOUND)
				continue;
			else if (eChkChnlResult == ENUM_CHNL_CHECK_PASS)
				NAN_TIMELINE_SET(pau4AvailSlots[szTimeLine],
					u4Slot);
			else
				NAN_TIMELINE_SET(pau4UnavailSlots[szTimeLine],
					u4Slot);
		}

		for (i = 0; i < NAN_TOTAL_DW; i++) {
			pau4FreeSlots[szTimeLine][i] =
				~(pau4AvailSlots[szTimeLine][i] |
				  pau4UnavailSlots[szTimeLine][i]);
			pau4FawSlots[szTimeLine][i] =
				pau4AvailSlots[szTimeLine][i];
		}

		DBGLOG(NAN, DEBUG, "fgChkRmtCondSlot:%d\n", fgChkRmtCondSlot);
		nanUtilDump(prAdapter, "aau4AvailSlots",
			(uint8_t *)prNegoCtrl->aau4AvailSlots[szTimeLine],
			sizeof(prNegoCtrl->aau4AvailSlots[szTimeLine]));
		nanUtilDump(prAdapter, "aau4UnavailSlots",
			(uint8_t *)prNegoCtrl->aau4UnavailSlots[szTimeLine],
			sizeof(prNegoCtrl->aau4UnavailSlots[szTimeLine]));
		nanUtilDump(prAdapter, "aau4FawSlots",
			(uint8_t *)prNegoCtrl->aau4FawSlots[szTimeLine],
			sizeof(prNegoCtrl->aau4FawSlots[szTimeLine]));
		nanUtilDump(prAdapter, "aau4FreeSlots",
			(uint8_t *)prNegoCtrl->aau4FreeSlots[szTimeLine],
			sizeof(prNegoCtrl->aau4FreeSlots[szTimeLine]));

		if (!fgChkRmtCondSlot)
			continue;

		/* fgChkRmtCondSlot */
		for (u4Slot = 0; u4Slot < NAN_TOTAL_SLOT_WINDOWS; u4Slot++) {
			/* TODO: consider DW slot */
			u4RmtCondChnl = nanGetPeerPrimaryChnlBySlot(prAdapter,
					u4SchIdx, NAN_NUM_AVAIL_DB,
					u4Slot, TRUE);
			if (u4RmtCondChnl != 0)
				continue;

			/* u4RmtCondChnl == 0 */
			if (NAN_IS_AVAIL_MAP_SET(pau4FawSlots[szTimeLine],
						 u4Slot))
				NAN_TIMELINE_UNSET(pau4FawSlots[szTimeLine],
						   u4Slot);
			else if (NAN_IS_AVAIL_MAP_SET(pau4FreeSlots[szTimeLine],
						      u4Slot))
				NAN_TIMELINE_UNSET(pau4FreeSlots[szTimeLine],
						   u4Slot);
		}

		nanUtilDump(prAdapter, "final aau4FawSlots",
			    (uint8_t *)prNegoCtrl->aau4FawSlots,
			    sizeof(prNegoCtrl->aau4FawSlots));
		nanUtilDump(prAdapter, "final aau4FreeSlots",
			    (uint8_t *)prNegoCtrl->aau4FreeSlots,
			    sizeof(prNegoCtrl->aau4FreeSlots));
	}
	return rRetStatus;
}

uint32_t nanSchedNegoGenQosCriteria(struct ADAPTER *prAdapter)
{
	struct WIFI_VAR  *prWifiVar = &prAdapter->rWifiVar;
	uint32_t i, j;
	uint32_t u4SlotIdx;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Lack;
	uint32_t u4EmptySlots;
	uint32_t au4CondSlots[NAN_TIMELINE_MGMT_SIZE];
	int32_t i4Latency, i4LatencyStart, i4LatencyEnd;
	uint32_t u4QosMinSlots;
	uint32_t u4QosMaxLatency;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;
	uint32_t u4UnavailSlotsAll, u4FreeSlotsAll, u4FawSlotsAll;
	size_t szTimeLine;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	uint32_t (*pau4FawSlots)[NAN_TOTAL_DW];
	uint32_t (*pau4FreeSlots)[NAN_TOTAL_DW];
	uint32_t (*pau4UnavailSlots)[NAN_TOTAL_DW];

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		goto CHK_QOS_DONE;
	}

	DBGLOG(NAN, DEBUG, "------>\n");
	DBGLOG(NAN, DEBUG, "Peer Qos spec MinSlots:%d, MaxLatency:%d\n",
	       prPeerSchDesc->u4QosMinSlots, prPeerSchDesc->u4QosMaxLatency);
	DBGLOG(NAN, DEBUG, "My Qos spec MinSlots:%d, MaxLatency:%d\n",
	       prNegoCtrl->u4QosMinSlots, prNegoCtrl->u4QosMaxLatency);

	/* negotiate min slots */
	u4QosMinSlots = kal_min_t(uint32_t, prNegoCtrl->u4QosMinSlots,
					    prPeerSchDesc->u4QosMinSlots);
	if (u4QosMinSlots > NAN_INVALID_QOS_MIN_SLOTS) {
		u4QosMinSlots = kal_clamp_t(uint32_t, u4QosMinSlots,
					    NAN_QOS_MIN_SLOTS_LOW_BOUND,
					    NAN_QOS_MIN_SLOTS_UP_BOUND);
		prNegoCtrl->u4NegoQosMinSlots = u4QosMinSlots;
	}

	/* negotiate max latency */
	u4QosMaxLatency = kal_min_t(uint32_t, prNegoCtrl->u4QosMaxLatency,
					      prPeerSchDesc->u4QosMaxLatency);
	if (u4QosMaxLatency < NAN_INVALID_QOS_MAX_LATENCY) {
		u4QosMaxLatency = kal_clamp_t(uint32_t, u4QosMaxLatency,
					      NAN_QOS_MAX_LATENCY_LOW_BOUND,
					      NAN_QOS_MAX_LATENCY_UP_BOUND);
		prNegoCtrl->u4NegoQosMaxLatency = u4QosMaxLatency;
	}

	pau4FawSlots = prNegoCtrl->aau4FawSlots;
	pau4FreeSlots = prNegoCtrl->aau4FreeSlots;
	pau4UnavailSlots = prNegoCtrl->aau4UnavailSlots;

	for (i = 0; i < NAN_TOTAL_DW; i++) { /* check 32 slots in DW interval */
		kalMemZero(au4CondSlots, sizeof(au4CondSlots));
		kalMemSet(&u4UnavailSlotsAll, 0xFF, sizeof(u4UnavailSlotsAll));
		u4FreeSlotsAll = 0;
		u4FawSlotsAll = 0;

		for (szTimeLine = 0; szTimeLine < szNanActiveTimelineNum;
		     szTimeLine++) {
			/* [0][0], [1][0]
			 * [0][1], [1][1]
			 * [0][2], [1][2]
			 * ...
			 * [0][15], [1][15]
			 */
			u4FawSlotsAll |= pau4FawSlots[szTimeLine][i];
			u4FreeSlotsAll |= pau4FreeSlots[szTimeLine][i];

			/* set unvail slot when all timelines are unavailable */
			u4UnavailSlotsAll &= pau4UnavailSlots[szTimeLine][i];
		}

		/* step1. check QoS min slots */
		i4Lack = 0;
		if (u4QosMinSlots <= NAN_INVALID_QOS_MIN_SLOTS ||
		    nanUtilCheckBitOneCnt(&u4FawSlotsAll, sizeof(uint32_t)) >=
		    u4QosMinSlots)
			goto check_max_latency;

		/* check min slots */
		i4Lack = u4QosMinSlots -
			nanUtilCheckBitOneCnt(&u4FawSlotsAll, sizeof(uint32_t));
		if (i4Lack > 0 &&
		    nanUtilCheckBitOneCnt(&u4FreeSlotsAll, sizeof(uint32_t)) <
		    i4Lack) {
			DBGLOG(NAN, INFO, "MinSlots:%d, Lack:%d, free=%u\n",
			       u4QosMinSlots, i4Lack,
			       nanUtilCheckBitOneCnt(&u4FawSlotsAll,
						     sizeof(uint32_t)));
			rRetStatus = WLAN_STATUS_FAILURE;
			goto CHK_QOS_DONE;
		}

check_max_latency:
		/* step2. check & guarantee QoS max latency */
		if (u4QosMaxLatency >= NAN_INVALID_QOS_MAX_LATENCY)
			goto CHK_QOS_LATENCY_DONE;

		u4EmptySlots = ~u4FawSlotsAll;
		i4Latency = 0;
		i4LatencyStart = i4LatencyEnd = -1;

		/* Examine by counting contiguous empty slots */
		for (j = 0; j < NAN_SLOTS_PER_DW_INTERVAL; j++) {
			if (!(u4EmptySlots & BIT(j))) { /* not empty, reset */
				i4Latency = 0;
				i4LatencyStart = i4LatencyEnd = -1;
				continue;
			}

			if (i4Latency == 0) /* start to count the latency */
				i4LatencyStart = i4LatencyEnd = j;
			else /* track the end slot */
				i4LatencyEnd = j;
			i4Latency++;

			/* Once the accumulated i4Latency > u4QosMaxLatency,
			 * set a slot to make it meet the requirement
			 */
			for (;
			     i4Latency > u4QosMaxLatency &&
			     i4LatencyStart <= i4LatencyEnd;
			     i4LatencyEnd--) {
				if (u4UnavailSlotsAll & BIT(i4LatencyEnd))
					continue;

				/* Have a preference for 5G band */
				for (szTimeLine = szNanActiveTimelineNum;
				     szTimeLine--;) {
					if (!(pau4FreeSlots[szTimeLine][i] &
					     BIT(i4LatencyEnd)))
					    /* Unavail slots checked earlier */
						continue;

					pau4FawSlots[szTimeLine][i] |=
						BIT(i4LatencyEnd);
					au4CondSlots[szTimeLine] |=
						BIT(i4LatencyEnd);

					/* Set a slot, reset latency counting */
					i4Lack--;
					j = i4LatencyEnd;
					i4Latency = 0;
					i4LatencyStart = i4LatencyEnd = -1;
				}
			}

			if (i4Latency > u4QosMaxLatency) {
				rRetStatus = WLAN_STATUS_FAILURE;
				goto CHK_QOS_DONE;
			}
		}

CHK_QOS_LATENCY_DONE:
		/* step3. guarantee QoS min slots */
		for (j = 0; i4Lack > 0 && j < NAN_SLOTS_PER_DW_INTERVAL; j++) {
			/* Have a preference for 5G band */
			for (szTimeLine = szNanActiveTimelineNum;
			     szTimeLine--;) {
				if (pau4FawSlots[szTimeLine][i] & BIT(j) ||
				    !(pau4FreeSlots[szTimeLine][i] & BIT(j)) ||
				    pau4UnavailSlots[szTimeLine][i] & BIT(j))
					continue;

				/* slot j: !FAW && Free && !Unavail, set it */
				i4Lack--;

				pau4FawSlots[szTimeLine][i] |= BIT(j);
				au4CondSlots[szTimeLine] |= BIT(j);
				break;
			}
		}

		if (i4Lack > 0) {
			rRetStatus = WLAN_STATUS_FAILURE;
			goto CHK_QOS_DONE;
		}

		/* step4. assign channel to conditional slots */
		for (j = 0; j < NAN_SLOTS_PER_DW_INTERVAL; j++) {
			/* Have a preference for 5G band */
			for (szTimeLine = szNanActiveTimelineNum;
			     szTimeLine--;) {
				if (!(au4CondSlots[szTimeLine] & BIT(j)))
					continue;

				u4SlotIdx = NAN_FULL_SLOT_INDEX(i, j);
				rSelChnlInfo = nanSchedNegoSelectChnlInfo(
						prAdapter, u4SlotIdx,
						szTimeLine,
						prWifiVar->u4NanPreferBandMask);
				rRetStatus = nanSchedAddCrbToChnlList(prAdapter,
					      &rSelChnlInfo,
					      u4SlotIdx, 1,
					      ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					      FALSE, NULL);

				if (rRetStatus != WLAN_STATUS_SUCCESS)
					break;
			}
		}

		/* step5. merge availability entry for the same primary channel
		 */
		rRetStatus = nanSchedMergeAvailabileChnlList(prAdapter, FALSE);
	}

CHK_QOS_DONE:
	if (rRetStatus == WLAN_STATUS_SUCCESS) {
		nanUtilDump(prAdapter, "QoS au4FawSlots",
			    (uint8_t *)prNegoCtrl->aau4FawSlots,
			    sizeof(prNegoCtrl->aau4FawSlots));
	} else {
		DBGLOG(NAN, ERROR, "can't satisfy QoS spec\n");
		nanUtilDump(prAdapter, "QoS au4FawSlots",
			    (uint8_t *)prNegoCtrl->aau4FawSlots,
			    sizeof(prNegoCtrl->aau4FawSlots));
	}
	DBGLOG(NAN, DEBUG, "<------\n");

	return rRetStatus;
}

unsigned char
nanSchedIsRmtHasCommit5gCh(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	uint32_t u4SlotIdx;
	uint32_t u4AvailDbIdx;
	uint32_t u4SchIdx;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;

	for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
			u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByID(
				prAdapter, u4SchIdx,
				u4AvailDbIdx) == FALSE)
				continue;

			/* Get remote committed ch info */
			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, u4SchIdx,
				u4AvailDbIdx, u4SlotIdx,
				FALSE);

			/* TODO: need better check for 5G channel */
			if (rRmtChnlInfo.u4PrimaryChnl >= 36) {
				DBGLOG(NAN, DEBUG, "Peer has 5G commit\n");
				return TRUE;
			}
		}
	}

	return FALSE;
}


uint32_t
nanSchedNegoChkQosSpecForRspState(
	struct ADAPTER *prAdapter,
	unsigned char fgChkRmtCondSlot)
{
#define QOS_MIN_SLOTS_FOR_RESP 16 /* Half slots */
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4TpQosMinSlots = 0, rRetStatus = WLAN_STATUS_SUCCESS;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		return WLAN_STATUS_FAILURE;
	}

	/* peer has no preference */
	if (prPeerSchDesc->u4QosMinSlots == NAN_INVALID_QOS_MIN_SLOTS &&
	    prPeerSchDesc->u4QosMaxLatency == NAN_INVALID_QOS_MAX_LATENCY) {
		DBGLOG(NAN, DEBUG, "skip since peer has no QoS preference\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* Use QOS_MIN_SLOTS_FOR_RESP only
	 * when peer didn't bring commit 5G CH
	 * (Not 5G concurrent case)
	 */
	if (prAdapter->rWifiVar.fgDbDcModeEn) {
		if (!nanSchedIsRmtHasCommit5gCh(prAdapter)) {
			/* Backup prNegoCtrl->u4QosMinSlots
			 * since it's for common use,
			 * not only for Resp state
			 */
			u4TpQosMinSlots = prNegoCtrl->u4QosMinSlots;

			DBGLOG(NAN, DEBUG,
				"Nego QosMinSlots:%d->%d\n",
				prNegoCtrl->u4QosMinSlots,
				QOS_MIN_SLOTS_FOR_RESP);
			prNegoCtrl->u4QosMinSlots = QOS_MIN_SLOTS_FOR_RESP;
		}
	}

	if (prNegoCtrl->u4QosMinSlots == NAN_INVALID_QOS_MIN_SLOTS &&
	    prNegoCtrl->u4QosMaxLatency == NAN_INVALID_QOS_MAX_LATENCY)
		return WLAN_STATUS_SUCCESS;

	/* Update current slot status */
	nanSchedNegoUpdateDatabase(prAdapter, fgChkRmtCondSlot);

	/* Verify QoS requirement */
	rRetStatus = nanSchedNegoGenQosCriteria(prAdapter);

	if (prNegoCtrl->u4QosMinSlots != u4TpQosMinSlots) {
		DBGLOG(NAN, DEBUG,
			"Restore Nego QosMinSlots:%d->%d\n",
			prNegoCtrl->u4QosMinSlots, u4TpQosMinSlots);
		prNegoCtrl->u4QosMinSlots = u4TpQosMinSlots;
	}

	return rRetStatus;
}

uint32_t
nanSchedNegoChkQosSpec(struct ADAPTER *prAdapter,
		unsigned char fgChkRmtCondSlot)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		return WLAN_STATUS_FAILURE;
	}

	if ((prNegoCtrl->u4QosMinSlots == NAN_INVALID_QOS_MIN_SLOTS) &&
	    (prNegoCtrl->u4QosMaxLatency == NAN_INVALID_QOS_MAX_LATENCY) &&
	    (prPeerSchDesc->u4QosMinSlots == NAN_INVALID_QOS_MIN_SLOTS) &&
	    (prPeerSchDesc->u4QosMaxLatency == NAN_INVALID_QOS_MAX_LATENCY))
		return WLAN_STATUS_SUCCESS;

	/* Update current slot status */
	nanSchedNegoUpdateDatabase(prAdapter, fgChkRmtCondSlot);

	/* Verify QoS requirement */
	return nanSchedNegoGenQosCriteria(prAdapter);
}

uint32_t
nanSchedNegoIsRmtAvailabilityConflict(struct ADAPTER *prAdapter)
{
	uint32_t u4RetCode = 0;
	uint32_t u4Idx;
	uint32_t u4SlotIdx;
	uint32_t u4AvailDbIdx;
	uint32_t u4AvailEntryIdx;
	uint16_t u2Ctrl;
	struct _NAN_CRB_NEGO_CTRL_T * const prNegoCtrl =
		nanGetNegoControlBlock(prAdapter);
	const uint32_t u4SchIdx = prNegoCtrl->u4SchIdx;
	struct _NAN_PEER_SCH_DESC_T * const prPeerSchDesc =
		nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	struct _NAN_AVAILABILITY_DB_T *prAvailDb;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	union _NAN_BAND_CHNL_CTRL *prChnlCur;
	union _NAN_BAND_CHNL_CTRL *prChnlTmp;

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		u4RetCode = -1;
		goto CHK_RMT_AVAIL_DONE;
	}

	/* Check if peer Committed / conditional availability self conflict */
	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
	     u4AvailDbIdx++) {
		prAvailDb = &prPeerSchDesc->arAvailAttr[u4AvailDbIdx];
		if (prAvailDb->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
		     u4SlotIdx++) {
			prChnlTmp = NULL;

			for (u4AvailEntryIdx = 0;
			     u4AvailEntryIdx < NAN_NUM_AVAIL_TIMELINE;
			     u4AvailEntryIdx++) {
				prNanAvailEntry = &prAvailDb->arAvailEntryList
							   [u4AvailEntryIdx];
				if (prNanAvailEntry->fgActive == FALSE)
					continue;

				if (!NAN_IS_AVAIL_MAP_SET(
					    prNanAvailEntry->au4AvailMap,
					    u4SlotIdx))
					continue;

				u2Ctrl = prNanAvailEntry->rEntryCtrl.u2RawData;

				if (!NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2Ctrl) &&
				    !NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2Ctrl))
					continue;

				prChnlCur = prNanAvailEntry->arBandChnlCtrl;
				if (prChnlTmp == NULL) {
					prChnlTmp = prChnlCur;
					continue;
				}

				if (prChnlTmp->u4PrimaryChnl ==
				    prChnlCur->u4PrimaryChnl)
					continue;

				u4RetCode = -5;
				DBGLOG(NAN, WARN, "slot=%u tmp=%u cur=%u",
				       u4SlotIdx,
				       prChnlTmp->u4PrimaryChnl,
				       prChnlCur->u4PrimaryChnl);
				goto CHK_RMT_AVAIL_DONE;
			}
		}
	}

	/* Check if NDC selected slot not in committed or conditional */
	if (prPeerSchDesc->rSelectedNdcCtrl.fgValid == TRUE) {
		for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
			prTimeline = &prPeerSchDesc->rSelectedNdcCtrl
					      .arTimeline[u4Idx];
			if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			u4AvailDbIdx = nanSchedPeerGetAvailabilityDbIdx(
				prAdapter, prPeerSchDesc, prTimeline->ucMapId);
			if (u4AvailDbIdx == NAN_INVALID_AVAIL_DB_INDEX) {
				DBGLOG(NAN, WARN, "u4Idx=%u, get map %u failed",
				       u4Idx, prTimeline->ucMapId);
				u4RetCode = -10;
				goto CHK_RMT_AVAIL_DONE;
			}

			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				if (!NAN_IS_AVAIL_MAP_SET(
					    prTimeline->au4AvailMap, u4SlotIdx))
					continue;

				if (nanQueryPeerPrimaryChnlBySlot(prAdapter,
						u4SchIdx, u4AvailDbIdx,
						u4SlotIdx, TRUE) != 0)
					continue;

				if (nanQueryPeerPrimaryChnlBySlot(prAdapter,
						u4SchIdx, u4AvailDbIdx,
						u4SlotIdx, FALSE) != 0)
					continue;

				u4RetCode = -15;
				DBGLOG(NAN, WARN,
				       "u4Idx=%u, MapId=%u, u4SlotIdx=%u, Comm=%u, cond=%u",
				       u4Idx, prTimeline->ucMapId,
				       u4SlotIdx,
				       nanQueryPeerPrimaryChnlBySlot(prAdapter,
							u4SchIdx, u4AvailDbIdx,
							u4SlotIdx, TRUE),
				       nanQueryPeerPrimaryChnlBySlot(prAdapter,
							u4SchIdx, u4AvailDbIdx,
							u4SlotIdx, FALSE));
				goto CHK_RMT_AVAIL_DONE;
			}
		}
	}

	/* Check Immutable or Ranging timeline */
	if (!(prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK &&
	      prPeerSchDesc->fgImmuNdlTimelineValid == TRUE ||
	      prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING &&
	      prPeerSchDesc->fgRangingTimelineValid == TRUE)) {
		goto CHK_RMT_AVAIL_DONE;
	}

	/* Immutable or Ranging timeline is set */
	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prTimeline = NULL;
		if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK)
			prTimeline = &prPeerSchDesc->arImmuNdlTimeline[u4Idx];
		else
			prTimeline = &prPeerSchDesc->arRangingTimeline[u4Idx];

		if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		u4AvailDbIdx = nanSchedPeerGetAvailabilityDbIdx(prAdapter,
					prPeerSchDesc, prTimeline->ucMapId);

		if (u4AvailDbIdx == NAN_INVALID_AVAIL_DB_INDEX) {
			u4RetCode = -20;
			goto CHK_RMT_AVAIL_DONE;
		}

		for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
		     u4SlotIdx++) {
			if (!NAN_IS_AVAIL_MAP_SET(prTimeline->au4AvailMap,
						  u4SlotIdx))
				continue;

			if (nanQueryPeerPrimaryChnlBySlot(prAdapter,
						u4SchIdx, u4AvailDbIdx,
						u4SlotIdx, TRUE) != 0)
				continue;

			if (nanQueryPeerPrimaryChnlBySlot(prAdapter,
						u4SchIdx, u4AvailDbIdx,
						u4SlotIdx, FALSE) != 0)
				continue;

			DBGLOG(NAN, INFO,
			       "db=%u map=%u, slot=%u, timeline=%02x-%02x-%02x-%02x",
			       u4Idx, prTimeline->ucMapId, u4SlotIdx,
			       ((uint8_t *)(prTimeline->au4AvailMap))[0],
			       ((uint8_t *)(prTimeline->au4AvailMap))[1],
			       ((uint8_t *)(prTimeline->au4AvailMap))[2],
			       ((uint8_t *)(prTimeline->au4AvailMap))[3]);
			u4RetCode = -25;
			goto CHK_RMT_AVAIL_DONE;
		}
	}

CHK_RMT_AVAIL_DONE:
	DBGLOG(NAN, DEBUG, "Return:%d\n", u4RetCode);

	return u4RetCode;
}

uint32_t nanGetCommittedSlotsByBand(struct ADAPTER *prAdapter,
				   uint32_t *u46gCommitted,
				   uint32_t *u45gHighCommitted,
				   uint32_t *u45gLowCommitted)
{
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;
	uint8_t i;

	if (!u46gCommitted || !u45gHighCommitted || !u45gLowCommitted)
		return WLAN_STATUS_FAILURE;

	*u46gCommitted = 0;
	*u45gHighCommitted = 0;
	*u45gLowCommitted = 0;

	for (i = 0; i < NAN_SLOTS_PER_DW_INTERVAL; i++) {
		rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter, i,
						      sz5gTimeLineIdx);
		if (NAN_IS_CHANNEL_6G(rLocalChnlInfo))
			++*u46gCommitted;

		if (NAN_IS_CHANNEL_5G_HIGH(rLocalChnlInfo))
			++*u45gHighCommitted;

		if (NAN_IS_CHANNEL_5G_LOW(rLocalChnlInfo))
			++*u45gLowCommitted;
	}

	return WLAN_STATUS_SUCCESS;
}

/**
 * Check the condition the peer does not support a specific 5G band,
 * but fully committed to the band
 */
static
u_int8_t nanIsBandLocalOccupiedPeerNotSupported(struct ADAPTER *prAdapter,
				struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl)
{
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4SupportedBand;
	uint32_t u45gLowCommitted = 0;
	uint32_t u45gHighCommitted = 0;
	uint32_t u46gCommitted = 0;

	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);
	if (!prPeerSchDesc)
		return FALSE;

	u4SupportedBand = prPeerSchDesc->u4CommonSupportedBand;

	nanGetCommittedSlotsByBand(prAdapter,
				   &u46gCommitted,
				   &u45gHighCommitted,
				   &u45gLowCommitted);

	DBGLOG(NAN, INFO,
	       "u4CommonSupportedBand=(%c %c%c%c %c), local committed=%u/%u/%u",
	       u4SupportedBand & BIT(ENUM_SUPPORTED_BN_6G) ? '6' : ' ',
	       (u4SupportedBand &
		(BIT(ENUM_SUPPORTED_BN_5G_HIGH) |
		 BIT(ENUM_SUPPORTED_BN_5G_LOW))) ? '5' : ' ',
	       u4SupportedBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH) ? 'H' : ' ',
	       u4SupportedBand & BIT(ENUM_SUPPORTED_BN_5G_LOW) ? 'L' : ' ',
	       u4SupportedBand & BIT(ENUM_SUPPORTED_BN_2G) ? '2' : ' ',
	       u46gCommitted, u45gHighCommitted, u45gLowCommitted);

	if (!(u4SupportedBand & BIT(ENUM_SUPPORTED_BN_6G)) &&
	    u46gCommitted > 16)
		return TRUE;

	if (!(u4SupportedBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH)) &&
	    u45gHighCommitted > 16)
		return TRUE;

	if (!(u4SupportedBand & BIT(ENUM_SUPPORTED_BN_5G_LOW)) &&
	    u45gLowCommitted > 16)
		return TRUE;

	return FALSE;
}

static u_int8_t NAN_IS_A_BAND(enum ENUM_BAND eBand)
{
	if (eBand == BAND_5G)
		return TRUE;

#if (CFG_SUPPORT_NAN_6G == 1)
	if (eBand == BAND_6G)
		return TRUE;
#endif

	return FALSE;
}

/* check if remote CRB conflicts with local CRB */
static unsigned char
nanSchedNegoIsRmtCrbConflict(struct ADAPTER *prAdapter,
		struct _NAN_SCHEDULE_TIMELINE_T arTimeline[NAN_NUM_AVAIL_DB],
		unsigned char *pfgEmptyMapSet, /* found local empty slot */
		uint32_t aau4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW])
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	uint32_t u4AvailDbIdx;
	uint32_t u4SlotIdx;
	uint32_t u4Idx;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	size_t szTimeLineIdx;
	enum ENUM_BAND eRmtBand = BAND_NULL;
	uint8_t fgIsPeerNDC2G = FALSE, fgIsPeerNDC5GOr6G = FALSE;
	union _NAN_BAND_CHNL_CTRL rRmtSlot9ChnlInfo;
	union _NAN_BAND_CHNL_CTRL rLocalSlot9ChnlInfo;
	uint8_t fgNotNormalNDCTimeline = FALSE;
	uint32_t u4NotNormalSlotIdx = 0;
	uint32_t u4NegoTransIdx = nanSchedGetCurrentNegoTransIdx(prAdapter);
	struct _NAN_CRB_NEGO_TRANSACTION_T *prNegoTrans;
	struct _NAN_SCHEDULER_T *prScheduler;
	enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand;
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prNegoTrans = &prNegoCtrl->rNegoTrans[u4NegoTransIdx];
	prScheduler = nanGetScheduler(prAdapter);

	if (nanIsBandLocalOccupiedPeerNotSupported(prAdapter, prNegoCtrl))
		prNegoTrans->fgPropose2gConditional = TRUE;

	prPeerSchRecord =
		nanSchedGetPeerSchRecord(prAdapter, prNegoCtrl->u4SchIdx);
	eHighestCommonBand =
		nanSchedGetHighestCommonBand(prAdapter, prNegoCtrl->u4SchIdx,
					     TRUE);

	if (prPeerSchRecord == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchRecord\n");
		return TRUE;
	}

	if (pfgEmptyMapSet && aau4EmptyMap) {
		*pfgEmptyMapSet = FALSE;
		kalMemZero(aau4EmptyMap,
			(sizeof(uint32_t) *
			NAN_TIMELINE_MGMT_SIZE * NAN_TOTAL_DW));
	}

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		prTimeline = &arTimeline[u4Idx];
		if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		u4AvailDbIdx = nanSchedPeerGetAvailabilityDbIdx(
			prAdapter, prPeerSchRecord->prPeerSchDesc,
			prTimeline->ucMapId);
		if (u4AvailDbIdx == NAN_INVALID_AVAIL_DB_INDEX)
			return TRUE;

		/* Check whether 5G NDC default slot conflict */
		rRmtSlot9ChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, prNegoCtrl->u4SchIdx, u4AvailDbIdx,
				NAN_5G_DEFAULT_NDC_INDEX, TRUE);

		eRmtBand = nanRegGetNanChnlBand(rRmtSlot9ChnlInfo);

		szTimeLineIdx = sz5gTimeLineIdx;
		rLocalSlot9ChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
					NAN_5G_DEFAULT_NDC_INDEX, NULL, TRUE,
					szTimeLineIdx);

		if (rRmtSlot9ChnlInfo.u4PrimaryChnl != 0 &&
		    NAN_IS_A_BAND(eRmtBand) &&
		    rRmtSlot9ChnlInfo.u4PrimaryChnl !=
		    rLocalSlot9ChnlInfo.u4PrimaryChnl) {
			DBGLOG(NAN, WARN,
			       "Def 5G NDC slot conflict, use another slot\n");
			prPeerSchRecord->fgDef5GNDCConflict = TRUE;
		}
		DBGLOG(NAN, INFO,
		       "Remote#9=%u, eRmtBand=%u, Local#9=%u, highest_common=%u, NDC_5G_conflict=%u",
		       rRmtSlot9ChnlInfo.u4PrimaryChnl, eRmtBand,
		       rLocalSlot9ChnlInfo.u4PrimaryChnl, eHighestCommonBand,
		       prPeerSchRecord->fgDef5GNDCConflict);

		for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
		     u4SlotIdx++) {
			if (!NAN_IS_AVAIL_MAP_SET(prTimeline->au4AvailMap,
						  u4SlotIdx))
				continue;

			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(prAdapter,
					prNegoCtrl->u4SchIdx, u4AvailDbIdx,
					u4SlotIdx, TRUE);
			if (rRmtChnlInfo.u4PrimaryChnl == 0) {
				DBGLOG(NAN, ERROR, "rmt channel invalid\n");
				return TRUE;
			}
			if (!nanIsAllowedChannel(prAdapter, rRmtChnlInfo)) {
				DBGLOG(NAN, WARN,
				       "rmt channel (%d) not allowed\n",
				       rRmtChnlInfo.u4PrimaryChnl);
				return TRUE;
			}

			/* Highest is 2G */
			if (eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
			    !NAN_IS_CHANNEL_2G(rRmtChnlInfo)) {
				DBGLOG(NAN, WARN,
				       "rmt channel (%d) not allowed for concurrent\n",
				       rRmtChnlInfo.u4PrimaryChnl);
				return TRUE;
			}

			/* Default NDC slot not match */
			if (!nanGetFeatureIsSigma(prAdapter) &&
				NAN_SLOT_INDEX(u4SlotIdx) ==
					    NAN_5G_DEFAULT_NDC_INDEX &&
			    rRmtChnlInfo.u4PrimaryChnl !=
				    g_r5gDwChnl.u4PrimaryChnl) {
				DBGLOG(NAN, WARN,
				       "NDC slot %u rmt channel %d != local %u",
				       u4SlotIdx,
				       rRmtChnlInfo.u4PrimaryChnl,
				       g_r5gDwChnl.u4PrimaryChnl);
				/**
				 * Except the case ranging set to 6G channel 5
				 * in #9 in nanSchedNegoGenDefCrb:
				 * NDC slot 9 rmt channel 5 != local 149
				 * Force to use 5G social chanenl 149 there,
				 * or skip this conflict check.
				 * But need to consider required channel switch
				 * time cutting the end of DW if setting #9 to
				 * channel != 149
				 */
				if (prNegoCtrl->eType != ENUM_NAN_NEGO_RANGING)
					return TRUE;
			}

			eRmtBand = nanRegGetNanChnlBand(rRmtChnlInfo);

			szTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								      eRmtBand);
			rLocalChnlInfo =
				nanGetChnlInfoBySlot(prAdapter, u4SlotIdx,
						szTimeLineIdx);

			/* Skip 2G NDC to avoid 2G NDC bitmask unequal to local
			 * and NDP response reject peer.
			 * Only apply this rule to NDP negotiation.
			 */
			if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK &&
			    !nanGetFeatureIsSigma(prAdapter) &&
			    eRmtBand == BAND_2G4 &&
			    prNegoCtrl->eState ==
				ENUM_NAN_CRB_NEGO_STATE_RESPONDER &&
			    !prNegoTrans->fgPropose2gConditional) {
				DBGLOG(NAN, DEBUG, "Skip peer NDC 2G ch:%u\n",
				       rRmtChnlInfo.u4PrimaryChnl);
				fgIsPeerNDC2G = TRUE;
				break;
			} else if (NAN_IS_A_BAND(eRmtBand)) {
				fgIsPeerNDC5GOr6G = TRUE;

				if (NAN_SLOT_INDEX(u4SlotIdx) !=
					    NAN_5G_DEFAULT_NDC_INDEX &&
				    !prPeerSchRecord->fgDef5GNDCConflict &&
				    prNegoCtrl->eType !=
					    ENUM_NAN_NEGO_RANGING &&
				    rRmtChnlInfo.u4PrimaryChnl !=
				    rLocalChnlInfo.u4PrimaryChnl) {
					DBGLOG(NAN, DEBUG,
						"Check NDC abnormal on slot%u, rmt=%u, local=%u\n",
						u4SlotIdx,
						rRmtChnlInfo.u4PrimaryChnl,
						rLocalChnlInfo.u4PrimaryChnl);
					fgNotNormalNDCTimeline = TRUE;
					u4NotNormalSlotIdx = u4SlotIdx;
					break;
				}
			}

#if (CFG_NAN_SCHEDULER_VERSION == 1)
			if (rLocalChnlInfo.u4PrimaryChnl == 0 &&
			    nanRegGetNanChnlBand(rRmtChnlInfo) == BAND_2G4)
				rLocalChnlInfo = nanQueryNonNanChnlInfoBySlot(
				prAdapter, u4SlotIdx, BAND_2G4);
#endif

			if (rLocalChnlInfo.u4PrimaryChnl == 0) {
				/* Skip adding 5G conditional slot for FC slot,
				 * because it'll commit later
				 */
				uint32_t u4SlotOffset =
					NAN_SLOT_INDEX(u4SlotIdx);
				if (NAN_IS_5G_TIMELINE(prAdapter,
						       szTimeLineIdx) &&
				    NAN_SLOT_TIMELINE_IS_FC(prAdapter,
							    szTimeLineIdx,
							    u4SlotOffset))
					continue;

				/* Skip 2G NDC slot if peer channel != ch6 */
				/* And do not restrict for CERT, ex 5.5.3
				 */
				if (nanGetActiveTimelineMgmtNum(prAdapter) == 1
				    && !nanGetFeatureIsSigma(prAdapter)
				    && (eHighestCommonBand >
				       ENUM_SUPPORTED_BN_2G
				    || prScheduler->eLocalCommittedBand >
				       ENUM_SUPPORTED_BN_2G)
				    && NAN_SLOT_INDEX(u4SlotIdx) ==
				       NAN_2G_DEFAULT_NDC_INDEX
				    && NAN_IS_CHANNEL_2G(rRmtChnlInfo)) {
					fgNotNormalNDCTimeline = TRUE;
					break;
				}

				if (pfgEmptyMapSet && aau4EmptyMap) {
					*pfgEmptyMapSet = TRUE;
					NAN_TIMELINE_SET(
						aau4EmptyMap[szTimeLineIdx],
						u4SlotIdx);
					NAN_DW_DBGLOG(NAN, DEBUG,
						TRUE, u4SlotIdx,
						"Local empty! TIdx,%zu,Slot,%u,rmtChnl,%u,AvailDb,%u,MapId,%d\n",
						szTimeLineIdx, u4SlotIdx,
						rRmtChnlInfo.u4PrimaryChnl,
						u4AvailDbIdx,
						prTimeline->ucMapId);
				}
				continue;
			}

			if (nanSchedChkConcurrOp(rLocalChnlInfo,
						 rRmtChnlInfo) ==
			    CNM_CH_CONCURR_MCC)
				return TRUE;
		}
	}

	/* Skip 2G timeline nego for now, so counter 2G NDC */
	if (fgIsPeerNDC2G && !fgIsPeerNDC5GOr6G) {
		DBGLOG(NAN, WARN, "Counter peer 2G NDC\n");
		return TRUE;
	} else if (fgNotNormalNDCTimeline) {
		DBGLOG(NAN, WARN, "Counter peer 5/6G strange NDC (Slot %u)\n",
		       u4NotNormalSlotIdx);
		return TRUE;
	}

	return FALSE;
}

/* check if local CRB conflicts with remote CRB */
unsigned char
nanSchedNegoIsLocalCrbConflict(
	struct ADAPTER *prAdapter, struct _NAN_SCHEDULE_TIMELINE_T *prTimeline,
	unsigned char fgChkRmtCondSlot,
	unsigned char *pfgEmptyMapSet, /* found remote empty slot */
	uint32_t aau4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW],
	size_t szTimeLineIdx)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	uint32_t u4SlotIdx;
	uint32_t u4AvailDbIdx;
	unsigned char fgConflict;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if (prTimeline->ucMapId == NAN_INVALID_MAP_ID) {
		DBGLOG(NAN, ERROR, "ucMapId equals NAN_INVALID_MAP_ID!\n");
		return TRUE;
	}

	if (!nanSchedPeerAvailabilityDbValid(prAdapter, prNegoCtrl->u4SchIdx))
		return TRUE;

	if (pfgEmptyMapSet && aau4EmptyMap) {
		*pfgEmptyMapSet = FALSE;
		kalMemZero(aau4EmptyMap,
			(sizeof(uint32_t) *
			NAN_TIMELINE_MGMT_SIZE * NAN_TOTAL_DW));
	}

	for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
		if (!NAN_IS_AVAIL_MAP_SET(prTimeline->au4AvailMap, u4SlotIdx))
			continue;

		rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter, u4SlotIdx,
			szTimeLineIdx);
		if (rLocalChnlInfo.u4PrimaryChnl == 0) {
			DBGLOG(NAN, ERROR, "u4PrimaryChnl equals 0!\n");
			return TRUE;
		}

		fgConflict = TRUE;

		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByID(
				    prAdapter, prNegoCtrl->u4SchIdx,
				    u4AvailDbIdx) == FALSE)
				continue;

			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, prNegoCtrl->u4SchIdx, u4AvailDbIdx,
				u4SlotIdx, fgChkRmtCondSlot);
			if (rRmtChnlInfo.u4PrimaryChnl == 0)
				fgConflict = FALSE;
			else if (!nanIsAllowedChannel(
					 prAdapter,
					 rRmtChnlInfo))
				continue;

			if (nanSchedChkConcurrOp(rLocalChnlInfo,
						 rRmtChnlInfo) !=
			    CNM_CH_CONCURR_MCC) {
				fgConflict = FALSE;
				break;
			}
		}

		if (fgConflict == TRUE)
			return TRUE;

		if (u4AvailDbIdx >= NAN_NUM_AVAIL_DB) {
			/* cannot find a coherent rmt channel */
			if (pfgEmptyMapSet && aau4EmptyMap) {
				*pfgEmptyMapSet = TRUE;
				NAN_TIMELINE_SET(aau4EmptyMap[szTimeLineIdx],
				u4SlotIdx);
			}
		}
	}

	return FALSE;
}

unsigned char
nanSchedNegoIsCrbEqual(
	struct ADAPTER *prAdapter,
	struct _NAN_SCHEDULE_TIMELINE_T arLocalTimeline[NAN_TIMELINE_MGMT_SIZE],
	struct _NAN_SCHEDULE_TIMELINE_T arRmtTimeline[NAN_NUM_AVAIL_DB])
{
	uint32_t u4Idx, u4Idx1;
	struct _NAN_SCHEDULE_TIMELINE_T *prRmtTimeline;
	struct _NAN_SCHEDULE_TIMELINE_T *prLocalTimeline;
	uint32_t u4Bitmask = 0, u4LocalBitmask = 0;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++) {
		u4Bitmask = 0;

		for (u4Idx1 = 0; u4Idx1 < NAN_NUM_AVAIL_DB; u4Idx1++) {
			prRmtTimeline = &arRmtTimeline[u4Idx1];
			if (prRmtTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			u4Bitmask |= prRmtTimeline->au4AvailMap[u4Idx];
		}

		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {
			prLocalTimeline = &arLocalTimeline[szTimeLineIdx];
			if (prLocalTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			u4LocalBitmask |= prLocalTimeline->au4AvailMap[u4Idx];
		}

		/* Todo: problem might occur if same bit set on
		 * different timeline
		 */
		if (u4LocalBitmask != u4Bitmask) {
			DBGLOG(NAN, ERROR,
				"bitmask unequal local(0x%x)!=rmt(0x%x)\n",
				u4LocalBitmask, u4Bitmask);
			return FALSE;
		}
	}

	return !nanSchedNegoIsRmtCrbConflict(prAdapter, arRmtTimeline, NULL,
					     NULL);
}

/* [nanSchedNegoMergeCrb]
 *
 *   It just merges the CRBs and doesn't check if CRB channel conflict.
 */
uint32_t
nanSchedNegoMergeCrb(
	struct ADAPTER *prAdapter,
	struct _NAN_SCHEDULE_TIMELINE_T *prLocalTimeline,
	struct _NAN_SCHEDULE_TIMELINE_T arRmtTimeline[NAN_NUM_AVAIL_DB])
{
	uint32_t u4Idx, u4Idx1;

	for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++) {
		for (u4Idx1 = 0; u4Idx1 < NAN_NUM_AVAIL_DB; u4Idx1++) {
			if (arRmtTimeline[u4Idx1].ucMapId !=
				NAN_INVALID_MAP_ID)
				prLocalTimeline->au4AvailMap[u4Idx] |=
					arRmtTimeline[u4Idx1]
						.au4AvailMap[u4Idx];
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedNegoMergeCrbByBand(
	struct ADAPTER *prAdapter,
	struct _NAN_SCHEDULE_TIMELINE_T arLocalTimeline[NAN_TIMELINE_MGMT_SIZE],
	struct _NAN_SCHEDULE_TIMELINE_T arRmtTimeline[NAN_NUM_AVAIL_DB])
{
	size_t szIdx = 0, szIdx1 = 0, szIdx2 = 0;
	size_t szSlotIdx = 0, szAvailDbIdx = 0;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {0};
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (!prPeerSchDesc)
		return WLAN_STATUS_INVALID_DATA;

	for (szIdx1 = 0; szIdx1 < NAN_TOTAL_DW; szIdx1++) {
		for (szIdx2 = 0; szIdx2 < NAN_NUM_AVAIL_DB; szIdx2++) {
			if (arRmtTimeline[szIdx2].ucMapId == NAN_INVALID_MAP_ID)
				continue;

			szAvailDbIdx = nanSchedPeerGetAvailabilityDbIdx(
				prAdapter, prPeerSchDesc,
				arRmtTimeline[szIdx2].ucMapId);
			if (szAvailDbIdx > NAN_NUM_AVAIL_DB)
				break;
			for (szSlotIdx = 0; szSlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     szSlotIdx++) {
				if (!NAN_IS_AVAIL_MAP_SET(
					arRmtTimeline[szIdx2].au4AvailMap,
					szSlotIdx))
					continue;

				rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
					prAdapter, prNegoCtrl->u4SchIdx,
					szAvailDbIdx, szSlotIdx, TRUE);
				szIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
					nanRegGetNanChnlBand(rRmtChnlInfo));
				arLocalTimeline[szIdx].au4AvailMap[szIdx1] |=
					arRmtTimeline[szIdx2]
						.au4AvailMap[szIdx1];
			}
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedNegoAddQos(struct ADAPTER *prAdapter, uint32_t u4MinSlots,
		   uint32_t u4MaxLatency)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if ((prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR) &&
	    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER)) {
		return WLAN_STATUS_FAILURE;
	}

	if (u4MaxLatency < NAN_INVALID_QOS_MAX_LATENCY) {
		if (u4MaxLatency < prNegoCtrl->u4QosMaxLatency)
			prNegoCtrl->u4QosMaxLatency = u4MaxLatency;

		if (prNegoCtrl->u4QosMaxLatency < NAN_QOS_MAX_LATENCY_LOW_BOUND)
			prNegoCtrl->u4QosMaxLatency =
				NAN_QOS_MAX_LATENCY_LOW_BOUND;
		else if (prNegoCtrl->u4QosMaxLatency >
			 NAN_QOS_MAX_LATENCY_UP_BOUND)
			prNegoCtrl->u4QosMaxLatency =
				NAN_QOS_MAX_LATENCY_UP_BOUND;
	}

	if (u4MinSlots > NAN_INVALID_QOS_MIN_SLOTS) {
		if (u4MinSlots > prNegoCtrl->u4QosMinSlots)
			prNegoCtrl->u4QosMinSlots = u4MinSlots;

		if (prNegoCtrl->u4QosMinSlots < NAN_QOS_MIN_SLOTS_LOW_BOUND)
			prNegoCtrl->u4QosMinSlots = NAN_QOS_MIN_SLOTS_LOW_BOUND;
		else if (prNegoCtrl->u4QosMinSlots > NAN_QOS_MIN_SLOTS_UP_BOUND)
			prNegoCtrl->u4QosMinSlots = NAN_QOS_MIN_SLOTS_UP_BOUND;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSchedNegoAddNdcCrb(struct ADAPTER *prAdapter,
		      union _NAN_BAND_CHNL_CTRL *prChnlInfo,
		      uint32_t u4StartOffset, uint32_t u4NumSlots,
		      enum _ENUM_TIME_BITMAP_CTRL_PERIOD_T eRepeatPeriod)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Idx;
	uint32_t u4Run;
	uint32_t u4SlotIdx;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	struct _NAN_NDC_CTRL_T *prNdcCtrl;
	uint8_t rRandMacAddr[MAC_ADDR_LEN] = {0};
	uint8_t rRandMacMask[MAC_ADDR_LEN] = {0};
	uint32_t au4AvailMap[NAN_TOTAL_DW];
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	size_t szTimeLineIdx;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	do {
		if ((prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR) &&
		    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER)) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		if ((prChnlInfo == NULL) ||
		    (prChnlInfo->u4PrimaryChnl == 0)) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		if ((u4StartOffset + u4NumSlots) > (1 << (eRepeatPeriod + 2))) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}
		/* check if the CRB conflicts with existed status */
		szTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
					nanRegGetNanChnlBand(*prChnlInfo));
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
					szTimeLineIdx);

		for (u4Run = 0; (u4Run < (128 >> eRepeatPeriod)) &&
				(rRetStatus == WLAN_STATUS_SUCCESS);
		     u4Run++) {
			for (u4Idx = 0; u4Idx < u4NumSlots; u4Idx++) {
				u4SlotIdx = u4StartOffset +
					    (u4Run << (eRepeatPeriod + 2)) +
					    u4Idx;

				if ((nanQueryPrimaryChnlBySlot(prAdapter,
					u4SlotIdx, TRUE, szTimeLineIdx) != 0)
					&& (nanQueryPrimaryChnlBySlot(
					prAdapter, u4SlotIdx, TRUE,
					szTimeLineIdx) !=
					prChnlInfo->u4PrimaryChnl)) {
					rRetStatus = WLAN_STATUS_FAILURE;
					break;
				}

				if ((nanQueryPrimaryChnlBySlot(prAdapter,
					u4SlotIdx, FALSE, szTimeLineIdx) != 0)
					&& (nanQueryPrimaryChnlBySlot(
					prAdapter, u4SlotIdx, FALSE,
					szTimeLineIdx) !=
					prChnlInfo->u4PrimaryChnl)) {
					rRetStatus = WLAN_STATUS_FAILURE;
					break;
				}
			}
		}

		if (rRetStatus != WLAN_STATUS_SUCCESS)
			break;

		kalMemZero(au4AvailMap, sizeof(au4AvailMap));
		rRetStatus = nanSchedAddCrbToChnlList(prAdapter, prChnlInfo,
						      u4StartOffset, u4NumSlots,
						      eRepeatPeriod, FALSE,
						      au4AvailMap);
		if (rRetStatus != WLAN_STATUS_SUCCESS)
			break;

		prNdcCtrl = &prNegoCtrl->rSelectedNdcCtrl;
		if (prNdcCtrl->fgValid == FALSE) {
			prNdcCtrl->fgValid = TRUE;
			get_random_mask_addr(rRandMacAddr,
					     rRandMacAddr, rRandMacMask);
			kalMemCopy(prNdcCtrl->aucNdcId, rRandMacAddr,
				   NAN_NDC_ATTRIBUTE_ID_LENGTH);
			kalMemCopy(
			       prNdcCtrl->arTimeline[szTimeLineIdx].au4AvailMap,
			       (uint8_t *)au4AvailMap, sizeof(au4AvailMap));
			prNdcCtrl->arTimeline[szTimeLineIdx].ucMapId =
				prNanTimelineMgmt->ucMapId;
			prNdcCtrl->arTimeline[szTimeLineIdx].ucLocalMapId =
				prNanTimelineMgmt->ucMapId;
		} else {
			for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++)
				prNdcCtrl->arTimeline[szTimeLineIdx]
				.au4AvailMap[u4Idx] |= au4AvailMap[u4Idx];
		}
	} while (FALSE);

	return rRetStatus;
}

static void updateDbTimeline(struct ADAPTER *prAdapter,
			uint8_t fgChkRmtCondSlot,
			const struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl,
			union _NAN_BAND_CHNL_CTRL *prLocalChnlInfo,
			struct _NAN_SCHEDULE_TIMELINE_T *prSchedTimeline,
			uint32_t *pu4CrbNum,
			uint32_t u4SlotIdx,
			size_t szTimeLineIdx)
{
	unsigned char fgFound;
	uint32_t u4AvailDbIdx;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;

	fgFound = FALSE;

	for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
	     u4AvailDbIdx++) {
		if (!nanSchedPeerAvailabilityDbValidByID(prAdapter,
				prNegoCtrl->u4SchIdx, u4AvailDbIdx))
			continue;

		rRmtChnlInfo = nanGetPeerChnlInfoBySlot(prAdapter,
				prNegoCtrl->u4SchIdx, u4AvailDbIdx,
				u4SlotIdx, fgChkRmtCondSlot);
		NAN_DW_DBGLOG(NAN, LOUD, TRUE, u4SlotIdx,
			      "Slot:%d, localChnl:%d, rmtChnl:%d\n",
			      u4SlotIdx,
			      prLocalChnlInfo->u4PrimaryChnl,
			      rRmtChnlInfo.u4PrimaryChnl);
		if (rRmtChnlInfo.u4PrimaryChnl == 0 && !fgChkRmtCondSlot) {
			if (prLocalChnlInfo->u4PrimaryChnl == 0) {
				rSelChnlInfo =
					nanSchedNegoSelectChnlInfo(prAdapter,
					u4SlotIdx, szTimeLineIdx,
					prAdapter->rWifiVar
					.u4NanPreferBandMask);
				/* Clear this slot before adding channel to it
				 * to avoid adding multiple channels to the
				 * same slot
				 * Note: currently only add
				 * fgCommitOrCond=FALSE (cond)
				 * in this function,
				 * so also clear this type here
				 */
				nanSchedDeleteCrbFromChnlList(prAdapter,
					u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, szTimeLineIdx);
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);
			}

			fgFound = TRUE;
			(*pu4CrbNum)--;
			break;
		} else if (rRmtChnlInfo.u4PrimaryChnl != 0) {
			if (!nanIsAllowedChannel(prAdapter,
				rRmtChnlInfo))
				continue;

			if (prLocalChnlInfo->u4PrimaryChnl != 0) {
				if (nanSchedChkConcurrOp(*prLocalChnlInfo,
							 rRmtChnlInfo) !=
				    CNM_CH_CONCURR_MCC) {
					fgFound = TRUE;

					if (u4AvailDbIdx == 0)
						(*pu4CrbNum)--;
				}
				continue;
			}

			/* no local channel allocation case */
			rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
								rRmtChnlInfo);
			if (rSelChnlInfo.u4PrimaryChnl != 0) {
				/* Clear this slot before adding channel to it
				 * to avoid adding multiple channels to the
				 * same slot
				 * Note: currently only add
				 * fgCommitOrCond=FALSE (cond)
				 * in this function,
				 * so also clear this type here
				 */
				nanSchedDeleteCrbFromChnlList(prAdapter,
					u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, szTimeLineIdx);
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);

				fgFound = TRUE;
				if (u4AvailDbIdx == 0)
					(*pu4CrbNum)--;
				continue;
			}
		}
	}

	if (fgFound && prSchedTimeline)
		NAN_TIMELINE_SET(prSchedTimeline->au4AvailMap, u4SlotIdx);
}

/**
 * updateDWTimeline() - update Timeline of a Discovery Window interval
 * @prAdapter: pointer to adapter
 * @u4DefCrbNum: requested number of CRBs
 * @fgChkRmtCondSlot: need to check remote conditional slots
 * @fgPeerAvailMapValid: is peer availability map valid
 * @prSchedTimeline: schedule timeline to be updated
 * @prNegoCtrl: negotiation control context
 * @u4DwIdx: iteration index 0..15 to fill the DW interval index.
 *	A DW interval is 512 TU, represented by 32-bit as 16 TU for each.
 */
static uint32_t updateDWIntervalTimeline(struct ADAPTER *prAdapter,
			uint32_t u4DefCrbNum,
			uint8_t fgChkRmtCondSlot,
			uint8_t fgPeerAvailMapValid,
			struct _NAN_SCHEDULE_TIMELINE_T *prSchedTimeline,
			const struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl,
			uint32_t u4DwIdx,
			size_t szTimeLineIdx)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SlotOffset;
	uint32_t u4NanQuota;
	uint32_t u4CrbNum;
	uint32_t u4SlotIdx;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;

	u4NanQuota = NAN_SLOTS_PER_DW_INTERVAL; /* 32 */
#if (CFG_NAN_SCHEDULER_VERSION == 0)
	if (!fgChkRmtCondSlot)
		u4NanQuota -= prAdapter->rWifiVar.ucAisQuotaVal;
#endif

	if (fgChkRmtCondSlot)
		u4SlotOffset = 1;
	else
		u4SlotOffset = prAdapter->rWifiVar.ucDftQuotaStartOffset;


	for (u4CrbNum = u4DefCrbNum;
	     u4CrbNum && u4SlotOffset < u4NanQuota;
	     u4SlotOffset++) {
		/* u4CrbNum: u4DefCrbNum..1, decrement
		 * u4SlotOffset: 0..31
		 * => u4SlotIdx: u4SlotOffset..31 or by u4CrbNum
		 */

		u4SlotIdx = NAN_FULL_SLOT_INDEX(u4DwIdx, u4SlotOffset);

		if (nanWindowType(prAdapter, u4SlotIdx, szTimeLineIdx)
			== ENUM_NAN_DW)
			continue;

		rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter, u4SlotIdx,
			szTimeLineIdx);

		if (!fgPeerAvailMapValid) {
			if (rLocalChnlInfo.u4PrimaryChnl == 0) {
				rSelChnlInfo =
					nanSchedNegoSelectChnlInfo(prAdapter,
						u4SlotIdx, szTimeLineIdx,
						prAdapter->rWifiVar
						.u4NanPreferBandMask);
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);
			}

			u4CrbNum--;

			if (prSchedTimeline)
				NAN_TIMELINE_SET(prSchedTimeline->au4AvailMap,
						 u4SlotIdx);

			continue;
		}

		updateDbTimeline(prAdapter, fgChkRmtCondSlot, prNegoCtrl,
				 &rLocalChnlInfo, prSchedTimeline, &u4CrbNum,
				 u4SlotIdx, szTimeLineIdx);
	}

	if (u4CrbNum)
		rRetStatus = WLAN_STATUS_RESOURCES;

	return rRetStatus;
}

static union _NAN_BAND_CHNL_CTRL nanGetRangingChannel(uint16_t u2NanRangingBand)
{
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = g_rNullChnl;

	if (u2NanRangingBand == 2)
		rSelChnlInfo = g_r2gDwChnl;
	else if (u2NanRangingBand == 5)
		rSelChnlInfo = g_r5gDwChnl;

#if (CFG_SUPPORT_NAN_6G == 1)
	if (u2NanRangingBand == 6)
		rSelChnlInfo = g_r6gDefChnl;
#endif

	return rSelChnlInfo;
}


unsigned char nanSchedNegoTryToFindCrb(struct ADAPTER *prAdapter,
		struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl,
		uint32_t u4SuppBandIdMask,
		size_t szSlotIdx,
		uint8_t fgChkRmtCondSlot,
		union _NAN_BAND_CHNL_CTRL rSelChnlInfo,
		union _NAN_BAND_CHNL_CTRL rLocalChnlInfo,
		uint32_t *u4DefCrbNum,
		size_t szTimeLineIdx)
{
	struct WIFI_VAR  *prWifiVar = &prAdapter->rWifiVar;
	const uint16_t u2NanRangingBand = prWifiVar->u2NanRangingBand;
	uint32_t u4PrefBandMaskTmp;
	uint32_t u4NanPreferBandMask;
	enum ENUM_BAND eBand = BAND_2G4;
	enum ENUM_BAND eRmtBand = BAND_NULL;
	union _NAN_BAND_CHNL_CTRL rPrefChnl = {0};
	size_t szAvailDbIdx = 0;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};
	unsigned char fgFound = FALSE;
	const size_t sz2gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_2G4);
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);

	u4PrefBandMaskTmp = prAdapter->rWifiVar.u4NanPreferBandMask;
	u4NanPreferBandMask = u4PrefBandMaskTmp;

	while (u4PrefBandMaskTmp && !fgFound) {
		switch ((u4PrefBandMaskTmp & 0xF)) {
		case NAN_PREFER_BAND_MASK_CUST_CH: /* reference preferred ch */
			if (u4SuppBandIdMask & BIT(BAND_5G))
				rPrefChnl = g_rPreferredChnl[sz5gTimeLineIdx];
			else
				rPrefChnl = g_rPreferredChnl[sz2gTimeLineIdx];
			eBand = nanRegGetNanChnlBand(rPrefChnl);
			break;

		case NAN_PREFER_BAND_MASK_2G_DW_CH: /* reference 2G DW ch */
		case NAN_PREFER_BAND_MASK_2G_BAND: /* reference 2G BAND */
			eBand = BAND_2G4;
			break;

		case NAN_PREFER_BAND_MASK_5G_DW_CH: /* reference 5G DW ch */
		case NAN_PREFER_BAND_MASK_5G_BAND: /* reference 5G BAND */
			eBand = BAND_5G;
			break;

#if (CFG_SUPPORT_WIFI_6G == 1)
		case NAN_PREFER_BAND_MASK_6G_BAND: /* reference 6G BAND */
			eBand = BAND_6G;
			break;
#endif

		default:
			eBand = BAND_NULL;
			break;
		}

		u4PrefBandMaskTmp >>= 4;

		if ((eBand == BAND_NULL) ||
			((u4SuppBandIdMask & BIT(eBand)) == 0))
			continue;

		for (szAvailDbIdx = 0;
			szAvailDbIdx < NAN_NUM_AVAIL_DB; szAvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
				prNegoCtrl->u4SchIdx, szAvailDbIdx) == FALSE)
				continue;

			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, prNegoCtrl->u4SchIdx,
				szAvailDbIdx, szSlotIdx,
				fgChkRmtCondSlot);
			DBGLOG(NAN, LOUD,
				"Slot:%zu, localChnl:%u, rmtChnl:%u\n",
				szSlotIdx,
				rLocalChnlInfo.u4PrimaryChnl,
				rRmtChnlInfo.u4PrimaryChnl);

			eRmtBand = nanRegGetNanChnlBand(rRmtChnlInfo);
			if ((rRmtChnlInfo.u4PrimaryChnl != 0) &&
				(eRmtBand != eBand)) {
				DBGLOG(NAN, LOUD,
					"Skip diff band! TIdx,%zu,Slot,%zu,rmtChnl,%u,B=%u,SupB=0x%x\n",
					szTimeLineIdx, szSlotIdx,
					rRmtChnlInfo.u4PrimaryChnl,
					eRmtBand, eBand);
				continue;
			}

			if (rRmtChnlInfo.u4PrimaryChnl == 0 &&
			    !fgChkRmtCondSlot) {

				if (rLocalChnlInfo.u4PrimaryChnl == 0) {
					rSelChnlInfo =
						nanSchedNegoSelectChnlInfo(
							prAdapter, szSlotIdx,
							szTimeLineIdx,
							u4NanPreferBandMask);

					/* Prefer 5G default channel at #9
					 * if not force 5G, follow 6G setting
					 */
					if (prNegoCtrl->eType ==
					    ENUM_NAN_NEGO_RANGING) {
						rSelChnlInfo =
							nanGetRangingChannel(
							     u2NanRangingBand);
					}

					nanSchedAddCrbToChnlList(prAdapter,
					      &rSelChnlInfo, szSlotIdx, 1,
					      ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					      FALSE, NULL);
				}

				fgFound = TRUE;
				(*u4DefCrbNum)--;
				break;
			}

			if (rRmtChnlInfo.u4PrimaryChnl != 0) {
				if (!nanIsAllowedChannel(prAdapter,
					rRmtChnlInfo))
					continue;

				if (rLocalChnlInfo.u4PrimaryChnl != 0 &&
				    nanSchedChkConcurrOp(rLocalChnlInfo,
							 rRmtChnlInfo) !=
						CNM_CH_CONCURR_MCC) {
					fgFound = TRUE;
					(*u4DefCrbNum)--;
					break;
				} else if (rLocalChnlInfo.u4PrimaryChnl != 0) {
					continue;
				}

				/* no local channel allocation case */
				rSelChnlInfo =
					nanSchedConvergeChnlInfo(
					prAdapter, rRmtChnlInfo);
				if (rSelChnlInfo.u4PrimaryChnl != 0) {
					nanSchedAddCrbToChnlList(
						prAdapter, &rSelChnlInfo,
						szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
						FALSE, NULL);

					fgFound = TRUE;
					(*u4DefCrbNum)--;
					break;
				}
			}
		}
	}

	return fgFound;
}

uint32_t nanSchedNegoGenDefCrb(struct ADAPTER *prAdapter,
			       uint8_t fgChkRmtCondSlot)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t u4DwIdx = 0;
	uint32_t u4DefCrbNum = 0;
	unsigned char fgPeerAvailMapValid = FALSE;
	struct _NAN_SCHEDULE_TIMELINE_T *prSchedTimeline = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	size_t szSlotIdx = 0, szSlotOffset = 0;
	uint32_t u4DefCrbNumBackup = 0, u4IterationNum = 0;
	unsigned char fgFound = FALSE;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {.u4RawData = 0};
	uint32_t u4NanQuota = 0;
	unsigned char fgGenCrbSuccess = FALSE;
	uint32_t u4SuppBandIdMask = 0;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR &&
	    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER &&
	    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP) {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto GEN_DFT_CRB_DONE;
	}

	if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK) {
		u4DefCrbNum = prNegoCtrl->u4DefNdlNumSlots;
	} else if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING) {
		u4DefCrbNum = prNegoCtrl->u4DefRangingNumSlots;
	} else {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto GEN_DFT_CRB_DONE;
	}

	fgPeerAvailMapValid = nanSchedPeerAvailabilityDbValid(prAdapter,
					prNegoCtrl->u4SchIdx);
	DBGLOG(NAN, DEBUG,
	       "SchIdx:%d, fgChkRmtCondSlot:%d, CrbNum:%d, PeerAvailValid:%d\n",
	       prNegoCtrl->u4SchIdx, fgChkRmtCondSlot, u4DefCrbNum,
	       fgPeerAvailMapValid);

	if (fgChkRmtCondSlot && !fgPeerAvailMapValid) {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto GEN_DFT_CRB_DONE;
	}

	u4NanQuota = NAN_SLOTS_PER_DW_INTERVAL;
#if (CFG_NAN_SCHEDULER_VERSION == 0)
	if (!fgChkRmtCondSlot)
		u4NanQuota -= prAdapter->rWifiVar.ucAisQuotaVal;
#endif

	u4DefCrbNumBackup = u4DefCrbNum;

	/* have a preference for 5G band */
	for (szTimeLineIdx = szNanActiveTimelineNum; szTimeLineIdx--; ) {

		if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING &&
		    NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx)) /* 5G only */
			continue;

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);
		u4SuppBandIdMask = nanGetTimelineSupportedBand(prAdapter,
			szTimeLineIdx);

		if (prNanTimelineMgmt->ucMapId == NAN_INVALID_MAP_ID) {
			DBGLOG(NAN, DEBUG, "Skip invalid map, Tidx=%zu\n",
				szTimeLineIdx);
			continue;
		}

		if (u4SuppBandIdMask == 0)
			continue;

		if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING)
			prSchedTimeline =
				&prNegoCtrl->arRangingTimeline[szTimeLineIdx];
		if (prSchedTimeline &&
		    prSchedTimeline->ucMapId == NAN_INVALID_MAP_ID) {
			prSchedTimeline->ucMapId = prNanTimelineMgmt->ucMapId;
			prSchedTimeline->ucLocalMapId =
				prNanTimelineMgmt->ucMapId;
			kalMemZero(prSchedTimeline->au4AvailMap,
				sizeof(prSchedTimeline->au4AvailMap));

			DBGLOG(NAN, TRACE, "Set Ranging MapId=%u",
			       prSchedTimeline->ucMapId);
		}

		/* try to allocate basic CRBs for every DW interval */
		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
			if (fgChkRmtCondSlot)
				szSlotOffset = 1;
			else {
				szSlotOffset =
					prAdapter->rWifiVar
					.ucDftQuotaStartOffset;
			}

			for ((u4DefCrbNum = u4DefCrbNumBackup),
				(u4IterationNum = 0);
				(u4DefCrbNum && (u4IterationNum < u4NanQuota));
				(szSlotOffset =
				(szSlotOffset + 1) % u4NanQuota),
				(u4IterationNum++)) {

				szSlotIdx = NAN_FULL_SLOT_INDEX(u4DwIdx,
								szSlotOffset);

				if (nanWindowType(prAdapter, szSlotIdx,
					szTimeLineIdx) == ENUM_NAN_DW)
					continue;

				rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter,
						      szSlotIdx, szTimeLineIdx);

				if (!fgPeerAvailMapValid) {
					u4DefCrbNum--;

					if (prSchedTimeline)
						NAN_TIMELINE_SET(
						prSchedTimeline->au4AvailMap,
						szSlotIdx);

					if (rLocalChnlInfo.u4PrimaryChnl)
						continue;

					/* rLocalChnlInfo.u4PrimaryChnl == 0 */
					rSelChnlInfo =
						nanSchedNegoSelectChnlInfo(
							prAdapter, szSlotIdx,
							szTimeLineIdx,
							prAdapter->rWifiVar
							.u4NanPreferBandMask);
					nanSchedAddCrbToChnlList(prAdapter,
					      &rSelChnlInfo, szSlotIdx, 1,
					      ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					      FALSE, NULL);

					continue;
				}

				fgFound = nanSchedNegoTryToFindCrb(prAdapter,
						prNegoCtrl,
						u4SuppBandIdMask,
						szSlotIdx,
						fgChkRmtCondSlot,
						rSelChnlInfo,
						rLocalChnlInfo,
						&u4DefCrbNum,
						szTimeLineIdx);

				if (fgFound && prSchedTimeline)
					NAN_TIMELINE_SET(
						prSchedTimeline->au4AvailMap,
						szSlotIdx);
			}

			if (u4DefCrbNum)
				rRetStatus = WLAN_STATUS_RESOURCES;
		}

		if (rRetStatus == WLAN_STATUS_SUCCESS || fgGenCrbSuccess) {
			fgGenCrbSuccess = TRUE;
			rRetStatus = WLAN_STATUS_SUCCESS;
		}
	}


#if (CFG_SUPPORT_NAN_NDP_DUAL_BAND == 0)
	nanSchedRemoveDiffBandChnlList(prAdapter);
#endif

GEN_DFT_CRB_DONE:

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
	return rRetStatus;
}

unsigned char
nanSchedNegoCheckNdcCrbConflict(struct ADAPTER *prAdapter,
		struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl,
		uint32_t u4SuppBandIdMask,
		enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand,
		uint32_t u4SlotIdx,
		union _NAN_BAND_CHNL_CTRL rLocalChnlInfo)
{
	uint32_t u4AvailDbIdx = 0;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};
	unsigned char fgConflict = TRUE;
	struct _NAN_AVAILABILITY_DB_T *prAvailability;
	uint32_t u4SupportedBand;

	for (u4AvailDbIdx = 0;
		u4AvailDbIdx < NAN_NUM_AVAIL_DB; u4AvailDbIdx++) {


		if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
			prNegoCtrl->u4SchIdx, u4AvailDbIdx) == FALSE)
			continue;

		/* Skip handling 5G NDC if peer only support 2G */
		prAvailability = nanSchedPeerAvailabilityDbByID(prAdapter,
							prNegoCtrl->u4SchIdx,
							u4AvailDbIdx);
		u4SupportedBand = nanGetAvailabilityBand(prAvailability);
		if (eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
		    (u4SupportedBand & BIT(NAN_SUPPORTED_BAND_ID_2P4G)) == 0) {
			DBGLOG(NAN, INFO,
			       "Skip u4AvailDbIdx=%u, peer=2G, availability=0x%02x",
			       u4AvailDbIdx, u4SupportedBand);
			continue;
		}
		DBGLOG(NAN, INFO,
		       "Check u4AvailDbIdx=%u, availability=0x%02x",
		       u4AvailDbIdx, u4SupportedBand);

		/**
		 * if (szNanActiveTimelineNum >= NAN_TIMELINE_MGMT_SIZE)
		 * Remove the could be redundant check, keep monitoring
		 */
		fgConflict = TRUE;

		/* Not check conditional for JP to KR country code issue
		 * If peer FC slot and other conditional all not allowed,
		 * NDC choosing will all fail and reject
		 * Thus, skip checking conditional here,
		 * counter later with other committed chnl
		 *
		 * If local P2P/SAP channel causes 5G not available, skip check
		 * conditional too.
		 * Example: JP AIS=1/0/0; SAP=1/36/0 => NG
		 */
		if (eHighestCommonBand == ENUM_SUPPORTED_BN_5G_LOW ||
		    eHighestCommonBand == ENUM_SUPPORTED_BN_2G)
			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(prAdapter,
						prNegoCtrl->u4SchIdx,
						u4AvailDbIdx, u4SlotIdx, FALSE);
		else
			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(prAdapter,
						prNegoCtrl->u4SchIdx,
						u4AvailDbIdx, u4SlotIdx, TRUE);

		DBGLOG(NAN, INFO,
		       "u4AvailDbIdx=%u, eHighestCommonBand=%u, u4SlotIdx=%u local=%u, rmt=%u",
		       u4AvailDbIdx, eHighestCommonBand, u4SlotIdx,
		       rLocalChnlInfo.u4PrimaryChnl,
		       rRmtChnlInfo.u4PrimaryChnl);

#ifdef NAN_UNUSED /* Disable potential check with same reason as conditional */
		if (rRmtChnlInfo.u4PrimaryChnl == 0) {
			/* Check whether peer has potential channel info */
			if (rLocalChnlInfo.u4PrimaryChnl != 0) {
				u4ChkBandIdMask = BIT(nanRegGetNanChnlBand(
					rLocalChnlInfo));
			} else {
				u4ChkBandIdMask = u4SuppBandIdMask;
			}
			rRmtChnlInfo = nanQueryPeerPotentialChnlInfoBySlot(
				prAdapter,
				prNegoCtrl->u4SchIdx,
				u4AvailDbIdx,
				u4SlotIdx,
				u4ChkBandIdMask,
				rLocalChnlInfo.u4PrimaryChnl,
				prAdapter->rWifiVar.u4NanNdcPreferBandMask);
		}
#endif
		if (rRmtChnlInfo.u4PrimaryChnl != 0) {
			if (rLocalChnlInfo.u4PrimaryChnl == 0) {
				fgConflict = FALSE;
				break;
			}

			if (nanSchedChkConcurrOp(rLocalChnlInfo, rRmtChnlInfo)
				!= CNM_CH_CONCURR_MCC) {
				fgConflict = FALSE;
				DBGLOG(NAN, INFO,
				       "rmt=%u, local=%u, Concurrent=%u, return fgConflict=0",
				       rRmtChnlInfo.u4PrimaryChnl,
				       rLocalChnlInfo.u4PrimaryChnl,
				       nanSchedChkConcurrOp(rLocalChnlInfo,
							    rRmtChnlInfo));
				/* Alternative solution of skip 5G NDC,
				 * channel matched, early return fgConflict here
				 */
			} else if (prAdapter->rWifiVar.fgDbDcModeEn &&
				   (nanRegGetNanChnlBand(rLocalChnlInfo) ==
						    BAND_2G4 ||
				    nanRegGetNanChnlBand(rRmtChnlInfo) ==
						    BAND_2G4) &&
				   nanRegGetNanChnlBand(rLocalChnlInfo) !=
				   nanRegGetNanChnlBand(rRmtChnlInfo)) {
				fgConflict = FALSE;
				DBGLOG(NAN, INFO,
				       "rmt=%u, local=%u, DBDC=%u, chnlband(l)=%u, chnlband(r)=%u, set fgConflict=0",
				       rRmtChnlInfo.u4PrimaryChnl,
				       rLocalChnlInfo.u4PrimaryChnl,
				       prAdapter->rWifiVar.fgDbDcModeEn,
				       nanRegGetNanChnlBand(rLocalChnlInfo),
				       nanRegGetNanChnlBand(rRmtChnlInfo));
			} else if (eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
				   nanRegGetNanChnlBand(rLocalChnlInfo) ==
						   BAND_2G4 &&
				   nanRegGetNanChnlBand(rRmtChnlInfo) ==
						   BAND_2G4) {
				DBGLOG(NAN, INFO,
				       "rmt=%u, local=%u, eHighestCommonBand=%u, chnlband(l)=%u, chnlband(r)=%u, return fgConflict=%u",
				       rRmtChnlInfo.u4PrimaryChnl,
				       rLocalChnlInfo.u4PrimaryChnl,
				       eHighestCommonBand,
				       nanRegGetNanChnlBand(rLocalChnlInfo),
				       nanRegGetNanChnlBand(rRmtChnlInfo),
				       fgConflict);
				return fgConflict;
			}
		} else {
#ifdef NAN_UNUSED /* Disable potential check with same reason as conditional */
			/* Check whether peer has potential band info */
			if (rLocalChnlInfo.u4PrimaryChnl != 0) {
				u4ChkBandIdMask = BIT(nanRegGetNanChnlBand(
					rLocalChnlInfo));
			} else {
				u4ChkBandIdMask = u4SuppBandIdMask;
			}
			rRmtChnlInfo = nanQueryPeerPotentialBandInfoBySlot(
				prAdapter,
				prNegoCtrl->u4SchIdx,
				u4AvailDbIdx,
				u4SlotIdx,
				u4ChkBandIdMask);
			u4BandIdMask = rRmtChnlInfo.u4BandIdMask;
			if (u4BandIdMask != 0) {
				fgConflict = FALSE;
				break;
			}
#else
			fgConflict = FALSE;
#endif
		}
	}

	return fgConflict;
}

u_int8_t nanIsNdcTimelineConflict(struct ADAPTER *prAdapter,
				size_t szTimeLineIdx,
				struct _NAN_SCHEDULE_TIMELINE_T *prNdcTimeline)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	u_int8_t fgPeerAvailMapValid;
	uint32_t u4SlotIdx = 0;
	uint32_t u4AvailDbIdx = 0;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;
	union _NAN_BAND_CHNL_CTRL rLocalCondChnlInfo;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};


	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	fgPeerAvailMapValid = nanSchedPeerAvailabilityDbValid(prAdapter,
							prNegoCtrl->u4SchIdx);
	for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
		if (!NAN_IS_AVAIL_MAP_SET(prNdcTimeline->au4AvailMap,
					  u4SlotIdx))
			continue;

		rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
						u4SlotIdx, NULL,
						TRUE, szTimeLineIdx);
		rLocalCondChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
						u4SlotIdx, NULL,
						FALSE, szTimeLineIdx);
		if (rLocalChnlInfo.u4PrimaryChnl == 0 &&
		    rLocalCondChnlInfo.u4PrimaryChnl == 0) {
			DBGLOG(NAN, ERROR,
			       "timeline %zu no committed & conditional slot for NDC slot=%u map=%02x-%02x-%02x-%02x",
			       szTimeLineIdx, u4SlotIdx,
			       prNdcTimeline->aucAvailBlock[0],
			       prNdcTimeline->aucAvailBlock[1],
			       prNdcTimeline->aucAvailBlock[2],
			       prNdcTimeline->aucAvailBlock[3]);
			return TRUE;
		}

		if (!fgPeerAvailMapValid)
			continue;

		for (u4AvailDbIdx = 0; u4AvailDbIdx < NAN_NUM_AVAIL_DB;
		     u4AvailDbIdx++) {
			if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
							prNegoCtrl->u4SchIdx,
							u4AvailDbIdx) == FALSE)
				continue;

			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(prAdapter,
				prNegoCtrl->u4SchIdx,
				u4AvailDbIdx, u4SlotIdx, TRUE);
			if (rRmtChnlInfo.u4PrimaryChnl == 0)
				continue;

			if (nanSchedChkConcurrOp(rLocalChnlInfo,
						 rRmtChnlInfo) !=
			    CNM_CH_CONCURR_MCC)
				break;
		}
		/* For this slot, no SCC matched, conflict */
		if (u4AvailDbIdx == NAN_NUM_AVAIL_DB)
			return TRUE;
	}

	return FALSE;
}

static  u_int8_t nanIsLocalNdcCrbFound(struct ADAPTER *prAdapter,
			      struct _NAN_NDC_CTRL_T *prNdcCtrl)
{
	size_t szTimeLineIdx;
	struct _NAN_SCHEDULE_TIMELINE_T *prNdcTimeline;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		prNdcTimeline = &prNdcCtrl->arTimeline[szTimeLineIdx];

		if (prNdcTimeline->ucMapId != NAN_INVALID_MAP_ID &&
		    !nanIsNdcTimelineConflict(prAdapter, szTimeLineIdx,
					      prNdcTimeline)) {
			DBGLOG(NAN, INFO,
			       "timeline %zu MapID=%u map=%02x-%02x-%02x-%02x found",
			       szTimeLineIdx, prNdcTimeline->ucMapId,
			       prNdcTimeline->aucAvailBlock[0],
			       prNdcTimeline->aucAvailBlock[1],
			       prNdcTimeline->aucAvailBlock[2],
			       prNdcTimeline->aucAvailBlock[3]);

			return TRUE;
		}
	}
	return FALSE;
}

static struct _NAN_NDC_CTRL_T *nanFindUsableNdc(struct ADAPTER *prAdapter)
{
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	uint32_t u4Idx = 0;
	struct _NAN_SCHEDULER_T *prScheduler = nanGetScheduler(prAdapter);

	for (u4Idx = 0; u4Idx < NAN_MAX_NDC_RECORD; u4Idx++) {
		prNdcCtrl = &prScheduler->arNdcCtrl[u4Idx];

		if (!prNdcCtrl->fgValid)
			continue;

		/* NOTE: FIXED previously unintentionally wrong condition
		 * returning prNdcCtrl on !nanIsLocalNdcCrbFound() here.
		 */
		if (nanIsLocalNdcCrbFound(prAdapter, prNdcCtrl)) {
			DBGLOG(NAN, INFO,
			       "Found NDC %02x-%02x-%02x-%02x-%02x-%02x",
			       prNdcCtrl->aucNdcId[0],
			       prNdcCtrl->aucNdcId[1],
			       prNdcCtrl->aucNdcId[2],
			       prNdcCtrl->aucNdcId[3],
			       prNdcCtrl->aucNdcId[4],
			       prNdcCtrl->aucNdcId[5]);
			return prNdcCtrl;
		}
	}

	return NULL;
}

uint32_t nanGetNdcSlotIdxFromAvailMap(uint32_t u4AvailMap)
{
	uint32_t u4SlotIdx;

	for (u4SlotIdx = 0;
		u4SlotIdx < NAN_SLOTS_PER_DW_INTERVAL;
		u4SlotIdx++) {
		if (u4AvailMap & BIT(u4SlotIdx))
			return u4SlotIdx;
	}

	return NAN_INVALID_SLOT_INDEX;
}

uint32_t nanGetCurrentNdcSlotIdx(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_SCHEDULE_TIMELINE_T arNdcTimeline;
	uint32_t u4Idx;
	uint32_t u4TimelineIdx;
	uint32_t u4SlotIdx;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDL_NUM; u4Idx++) {
		prNDL = &prDataPathInfo->arNDL[u4Idx];
		if (prNDL->fgNDLValid != TRUE ||
		    prNDL->eCurrentNDLMgmtState != NDL_SCHEDULE_ESTABLISHED)
			continue;

		prPeerSchDesc = nanSchedSearchPeerSchDescByNmi(prAdapter,
				prNDL->aucPeerMacAddr);

		if (prPeerSchDesc == NULL) {
			DBGLOG(NAN, WARN,
				"PeerSchDesc "MACSTR" Null\n",
				prNDL->aucPeerMacAddr);
			return NAN_INVALID_SLOT_INDEX;
		}

		if (nanGetActiveTimelineMgmtNum(prAdapter) == 1)
			u4TimelineIdx = 0;
		else
			u4TimelineIdx = eBand == BAND_2G4 ? 0 : 1;

		arNdcTimeline = prPeerSchDesc->rSelectedNdcCtrl
				.arTimeline[u4TimelineIdx];
		u4SlotIdx = nanGetNdcSlotIdxFromAvailMap(
				arNdcTimeline.au4AvailMap[0]);
		DBGLOG(NAN, INFO, "Current NDC SlotIdx=%u\n", u4SlotIdx);

		if (u4SlotIdx != NAN_INVALID_SLOT_INDEX)
			return u4SlotIdx;
	}

	return NAN_INVALID_SLOT_INDEX;
}

static uint32_t nanGetNdcSlotOffset(struct ADAPTER *prAdapter,
		uint32_t u4SuppBandIdMask,
		enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand)
{
	struct _NAN_SCHEDULER_T *prScheduler;
	uint32_t u4SlotOffset = NAN_INVALID_SLOT_INDEX;
	uint32_t u4CurrentNdcSlot = NAN_INVALID_SLOT_INDEX;

	prScheduler = nanGetScheduler(prAdapter);

	if ((prScheduler->fgEn5gH || prScheduler->fgEn5gL) &&
	    u4SuppBandIdMask & BIT(BAND_5G))
		u4SlotOffset = NAN_5G_DEFAULT_NDC_INDEX;
	else
		u4SlotOffset = NAN_2G_DEFAULT_NDC_INDEX;

	/* single map */
	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
	    eHighestCommonBand == ENUM_SUPPORTED_BN_2G)
		u4SlotOffset = NAN_2G_DEFAULT_NDC_INDEX;

	if (prAdapter->rWifiVar.ucDftNdcStartOffset != 0)
		u4SlotOffset = prAdapter->rWifiVar.ucDftNdcStartOffset;

	/* single map, prefer current NDC slot with established NDL */
	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1) {
		if (nanGetEstablishedNdlNum(prAdapter) >= 1)
			u4CurrentNdcSlot = nanGetCurrentNdcSlotIdx(
					   prAdapter, BAND_2G4);
		else if (nanReschedIsOngoing(prAdapter))
			u4CurrentNdcSlot = nanReschedGetNdlNdc(prAdapter);

		if (u4CurrentNdcSlot != NAN_INVALID_SLOT_INDEX)
			u4SlotOffset = u4CurrentNdcSlot;
	}

	return u4SlotOffset;
}

uint32_t nanGetNdcSlotIndex(struct ADAPTER *prAdapter, size_t szTimeLineIdx)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	u_int8_t fgPeerAvailMapValid;
	uint32_t u4Num;
	uint32_t u4SlotOffset;
	uint32_t u4SlotIdx;
	uint32_t u4DwIdx = 0;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;
	u_int8_t fgConflict = FALSE;
	uint32_t u4SuppBandIdMask;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;
	enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand;
	u_int8_t fgIsP2pAisMCC;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	fgPeerAvailMapValid = nanSchedPeerAvailabilityDbValid(prAdapter,
							prNegoCtrl->u4SchIdx);

	fgIsP2pAisMCC = NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G);
	eHighestCommonBand = nanSchedGetHighestCommonBand(prAdapter,
						     prNegoCtrl->u4SchIdx,
						     TRUE);
	u4SuppBandIdMask =
		nanGetTimelineSupportedBand(prAdapter, szTimeLineIdx);
	u4SlotOffset = nanGetNdcSlotOffset(prAdapter, u4SuppBandIdMask,
							eHighestCommonBand);

	NAN_DW_DBGLOG(NAN, INFO, TRUE, 0,
		      "fgIsP2pAisMCC=%u, eHighestCommonBand=%u",
		      fgIsP2pAisMCC, eHighestCommonBand);
	for (u4Num = 0; u4Num < NAN_SLOTS_PER_DW_INTERVAL;
	     u4Num++, u4SlotOffset = NAN_SLOT_INDEX(u4SlotOffset + 1)) {

		if (nanIsDiscWindow(prAdapter, u4SlotOffset, szTimeLineIdx))
			continue;

		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
			uint32_t u4SelPrimaryChnl = 0;
#endif

			u4SlotIdx = NAN_FULL_SLOT_INDEX(u4DwIdx, u4SlotOffset);

			rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter,
					      u4SlotIdx, szTimeLineIdx);

			if (rLocalChnlInfo.u4PrimaryChnl == 0) {
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
				/* When no NDC CRB conflict, pre-select
				 * channel to avoid after choosing slot,
				 * new channel selection algorithm
				 * return channel 0
				 */
				rSelChnlInfo = nanSchedNegoFindSlotCrb(
							prAdapter,
							prNegoCtrl,
							eHighestCommonBand,
							FALSE,
							szTimeLineIdx,
							u4SlotIdx,
							FALSE,
							NULL);
				u4SelPrimaryChnl = rSelChnlInfo.u4PrimaryChnl;
				if (u4SelPrimaryChnl == 0)
					break;

				rLocalChnlInfo = rSelChnlInfo;
#else
				continue; /* pass */
#endif
			}

			if (!fgPeerAvailMapValid)
				continue; /* pass */


			fgConflict = nanSchedNegoCheckNdcCrbConflict(prAdapter,
					prNegoCtrl, u4SuppBandIdMask,
					eHighestCommonBand,
					u4SlotIdx, rLocalChnlInfo);
			NAN_DW_DBGLOG(NAN, TRACE, TRUE, u4SlotIdx,
			       "timeline=%zu, u4SuppBandIdMask=0x%02x, u4SlotIdx=%u, rLocalChnlInfo=%u, fgConflict=%u\n",
			       szTimeLineIdx,
			       u4SuppBandIdMask, u4SlotIdx,
			       rLocalChnlInfo.u4PrimaryChnl,
			       fgConflict);

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
			if (fgConflict) {
				if (u4SelPrimaryChnl != 0)
					nanSchedDeleteCrbFromChnlList(prAdapter,
					      u4SlotIdx, 1,
					      ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					      TRUE, szTimeLineIdx);

				break;
			}

			NAN_DW_DBGLOG(NAN, INFO, TRUE,
				      u4SlotIdx,
				      "Tidx(%zu) NDC slot(%u): Pre-select channel = ch:%u\n",
				      szTimeLineIdx, u4SlotIdx,
				      u4SelPrimaryChnl);
#else
			if (fgConflict)
				break;
#endif
		}

		if (u4DwIdx == NAN_TOTAL_DW)
			break;
	}
	DBGLOG(NAN, TRACE,
	       "Checked NDC szTimeLineIdx=%zu u4Num=%u, u4DwIdx=%u, u4SlotOffset=%u, fgConflict=%u",
	       szTimeLineIdx, u4Num, u4DwIdx, u4SlotOffset, fgConflict);


	if (u4Num == NAN_SLOTS_PER_DW_INTERVAL)
		return WLAN_STATUS_FAILURE;

	return u4SlotOffset;
}

uint32_t nanSetNdcSlotIndex(struct ADAPTER *prAdapter,
			    size_t szTimeLineIdx, uint32_t u4SlotIndex)
{
	struct _NAN_CRB_NEGO_CTRL_T * const prNegoCtrl =
				nanGetNegoControlBlock(prAdapter);
	struct _NAN_NDC_CTRL_T * const prNdcCtrl =
				&prNegoCtrl->rSelectedNdcCtrl;
	struct _NAN_TIMELINE_MGMT_T * const prNanTimelineMgmt =
				nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION != 1)
	uint32_t rRetStatus;
#endif
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	uint32_t u4DwIdx;
	uint32_t u4Idx;
	uint8_t rRandMacAddr[MAC_ADDR_LEN] = {0};
	uint8_t rRandMacMask[MAC_ADDR_LEN] = {0};
	uint32_t u4SlotIdx;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {0};

	kalMemZero(prNdcCtrl, sizeof(struct _NAN_NDC_CTRL_T));
	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_SIZE;
		u4Idx++) {
		prTimeline = &prNdcCtrl->arTimeline[u4Idx];
		prTimeline->ucMapId = NAN_INVALID_MAP_ID;
		prTimeline->ucLocalMapId = NAN_INVALID_MAP_ID;
		kalMemZero(prTimeline->au4AvailMap,
			   sizeof(prTimeline->au4AvailMap));
	}
	prNdcCtrl->fgValid = TRUE;
	get_random_mask_addr(rRandMacAddr, rRandMacAddr, rRandMacMask);
	kalMemCopy(prNdcCtrl->aucNdcId, rRandMacAddr,
		   NAN_NDC_ATTRIBUTE_ID_LENGTH);

	prTimeline = &prNdcCtrl->arTimeline[szTimeLineIdx];
	prTimeline->ucMapId = prNanTimelineMgmt->ucMapId;
	prTimeline->ucLocalMapId = prNanTimelineMgmt->ucMapId;

	for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
		u4SlotIdx = NAN_FULL_SLOT_INDEX(u4DwIdx, u4SlotIndex);
		NAN_TIMELINE_SET(prTimeline->au4AvailMap, u4SlotIdx);

		rSelChnlInfo = nanGetChnlInfoBySlot(prAdapter,
						      u4SlotIdx, szTimeLineIdx);
		if (rSelChnlInfo.u4PrimaryChnl != 0)
			continue;

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
		/* Pre-select channel will get commit/cond,
		 * got removed at somewhere?
		 */
		NAN_DW_DBGLOG(NAN, ERROR, TRUE, u4SlotIdx,
			      "Tidx(%zu) NDC slot(%u): Pre-select NDC channel = 0!\n",
			      szTimeLineIdx, u4SlotIdx);
		return WLAN_STATUS_FAILURE;
#else
		/* Assign channel for new NDC */
		rSelChnlInfo = nanSchedNegoSelectChnlInfo(prAdapter,
					u4SlotIdx, szTimeLineIdx,
					prWifiVar->u4NanNdcPreferBandMask);

		rRetStatus = nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);
		if (rRetStatus != WLAN_STATUS_SUCCESS)
			return rRetStatus;
#endif
	}

	nanUtilDump(prAdapter, "New NDC Map",
		    (uint8_t *)prTimeline->au4AvailMap,
		    sizeof(prTimeline->au4AvailMap));
	nanUtilDump(prAdapter, "NDC ID", prNdcCtrl->aucNdcId,
		NAN_NDC_ATTRIBUTE_ID_LENGTH);
	DBGLOG(NAN, INFO, "NDC MapID:%d, TIdx:%zu, slot=%u, ch=%u\n",
	       prTimeline->ucMapId, szTimeLineIdx,
	       u4SlotIndex, rSelChnlInfo.u4PrimaryChnl);

	/* just need to create one NDC from any timeline success */
	return WLAN_STATUS_SUCCESS;
}

static u_int8_t nanIsNegoCtrl2gOnly(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;
	enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	eHighestCommonBand = nanSchedGetHighestCommonBand(prAdapter,
							  prNegoCtrl->u4SchIdx,
							  TRUE);

	return (eHighestCommonBand == ENUM_SUPPORTED_BN_2G);
}

static uint32_t nanAllocateNewNdc(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	uint32_t u4SlotIndex;
	u_int32_t fgNegoCtrl2gOnly;

	fgNegoCtrl2gOnly = nanIsNegoCtrl2gOnly(prAdapter);
	for (szTimeLineIdx = szNanActiveTimelineNum; szTimeLineIdx--; ) {
		DBGLOG(NAN, INFO, "Search NDC in szTimeLineIdx=%zu",
		       szTimeLineIdx);

		rRetStatus = WLAN_STATUS_SUCCESS;
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);
		if (prNanTimelineMgmt->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		if (nanGetTimelineSupportedBand(prAdapter, szTimeLineIdx) == 0)
			continue;

		/* Sigma 5.3.1 must choose 2G NDC for 2G only peer,
		 * skip 5G timeline
		 * TODO: limit only nanGetFeatureIsSigma(prAdapter)?
		 */
		/* Do not skip 2G only but single map */
		if (fgNegoCtrl2gOnly &&
		    NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx) &&
		    nanGetTimelineNum() > 1) {
			DBGLOG(NAN, INFO,
			       "Sigma 5.3.1 must choose 2G NDC for 2G only peer, skip 5G timeline. 2G only=%u",
			       fgNegoCtrl2gOnly);
			continue;
		}

		u4SlotIndex = nanGetNdcSlotIndex(prAdapter, szTimeLineIdx);

		if (u4SlotIndex == WLAN_STATUS_FAILURE) {
			if (szTimeLineIdx == 0) /* all traversed */
				return WLAN_STATUS_FAILURE;
			continue; /* try next timeline */
		}

		return nanSetNdcSlotIndex(prAdapter, szTimeLineIdx,
					  u4SlotIndex);
	}

	return WLAN_STATUS_SUCCESS;
}

static uint32_t nanSchedNegoGenNdcCrb(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T * const prNegoCtrl =
				nanGetNegoControlBlock(prAdapter);
	struct _NAN_NDC_CTRL_T *prNdcCtrl;

	if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR &&
	    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER) {
		return WLAN_STATUS_FAILURE;
	}

	/* TODO: select NDC in timeline considering the concurrent condition */

	/* Step1 check existed NDC CRB */
	prNdcCtrl = nanFindUsableNdc(prAdapter);
	if (prNdcCtrl) {
		kalMemCopy(&prNegoCtrl->rSelectedNdcCtrl,
			   prNdcCtrl, sizeof(prNegoCtrl->rSelectedNdcCtrl));
		nanUtilDump(prAdapter, "Reuse NDC ID", prNdcCtrl->aucNdcId,
			    NAN_NDC_ATTRIBUTE_ID_LENGTH);
		return WLAN_STATUS_SUCCESS;
	}

	/* Step2 allocate new NDC CRB */
	return nanAllocateNewNdc(prAdapter);
}

uint32_t nanSchedNegoAllocNdcCtrl(struct ADAPTER *prAdapter,
				  struct _NAN_NDC_CTRL_T *prSelectedNdcCtrl)
{
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szTimeLineIdx = 0, szNum = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	do {
		prNdcCtrl = nanSchedGetNdcCtrl(prAdapter,
					       prSelectedNdcCtrl->aucNdcId);
		if ((prNdcCtrl != NULL) &&
		    (nanSchedNegoIsCrbEqual(
			     prAdapter, &prNdcCtrl->arTimeline[0],
			     prSelectedNdcCtrl->arTimeline) == FALSE)) {

			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		prNdcCtrl = &prNegoCtrl->rSelectedNdcCtrl;
		prNdcCtrl->fgValid = TRUE;
		kalMemZero(prNdcCtrl->arTimeline,
			   sizeof(prNdcCtrl->arTimeline));
		kalMemCopy(prNdcCtrl->aucNdcId, prSelectedNdcCtrl->aucNdcId,
			   NAN_NDC_ATTRIBUTE_ID_LENGTH);

		/* Initialize NDC map id by local timeline MAP ID */
		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {
			prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
						szTimeLineIdx);
			prNdcCtrl->arTimeline[szTimeLineIdx].ucMapId =
					prNanTimelineMgmt->ucMapId;
			prNdcCtrl->arTimeline[szTimeLineIdx].ucLocalMapId =
					prNanTimelineMgmt->ucMapId;
		}
		nanSchedNegoMergeCrbByBand(prAdapter, prNdcCtrl->arTimeline,
					   prSelectedNdcCtrl->arTimeline);

		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {
			struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;

			prTimeline = &prNdcCtrl->arTimeline[szTimeLineIdx];
			szNum = nanUtilCheckBitOneCnt(prTimeline->au4AvailMap,
					       sizeof(prTimeline->au4AvailMap));

			DBGLOG(NAN, DEBUG,
				"NDC MapID:%u, TIdx:%zu, SlotCnt:%zu\n",
				prNdcCtrl->arTimeline[szTimeLineIdx].ucMapId,
				szTimeLineIdx, szNum);

			if (szNum > 0)
				continue;

			/* szNum == 0, set Map ID invalid */
			DBGLOG(NAN, INFO,
			       "Invalidate timeline %zu NDC MapID:%u",
			       szTimeLineIdx,
			       prNdcCtrl->arTimeline[szTimeLineIdx].ucMapId);

			prNdcCtrl->arTimeline[szTimeLineIdx].ucMapId =
				NAN_INVALID_MAP_ID;
			prNdcCtrl->arTimeline[szTimeLineIdx].ucLocalMapId =
				NAN_INVALID_MAP_ID;
		}
		DBGLOG(NAN, INFO, "Join NDC ID: %02x:%02x:%02x:%02x:%02x:%02x",
		       prSelectedNdcCtrl->aucNdcId[0],
		       prSelectedNdcCtrl->aucNdcId[1],
		       prSelectedNdcCtrl->aucNdcId[2],
		       prSelectedNdcCtrl->aucNdcId[3],
		       prSelectedNdcCtrl->aucNdcId[4],
		       prSelectedNdcCtrl->aucNdcId[5]);
	} while (FALSE);

	return rRetStatus;
}

uint32_t
nanSchedNegoResetControlInfo(struct ADAPTER *prAdapter,
			     enum _ENUM_NEGO_CTRL_RST_REASON_T eResetReason)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SlotIdx = 0;
	uint32_t u4LocalChnl = 0;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_SCHEDULE_TIMELINE_T *prImmuTimeline = NULL;
	struct _NAN_SCHEDULE_TIMELINE_T *prRangingTimeline = NULL;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		return WLAN_STATUS_FAILURE;
	}

	switch (eResetReason) {
	case ENUM_NEGO_CTRL_RST_BY_WAIT_RSP_STATE:
		/* honor peer's NDC proposal in WAIT-RSP-STATE */
		prNegoCtrl->rSelectedNdcCtrl.fgValid = FALSE;

		prImmuTimeline = prNegoCtrl->arImmuNdlTimeline;
		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++, prImmuTimeline++) {

			for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
			     u4SlotIdx++) {
				u4LocalChnl = nanQueryPrimaryChnlBySlot(
					prAdapter, u4SlotIdx, FALSE,
					szTimeLineIdx);
				if (u4LocalChnl == 0)
					continue;

				if (prImmuTimeline->ucMapId !=
					    NAN_INVALID_MAP_ID &&
				    NAN_IS_AVAIL_MAP_SET(
					    prImmuTimeline->au4AvailMap,
					    u4SlotIdx))
					continue;

				nanSchedDeleteCrbFromChnlList(prAdapter,
					u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, szTimeLineIdx);
			}
		}
		break;

	case ENUM_NEGO_CTRL_RST_PREPARE_CONFIRM_STATE:
		prPeerSchDesc->rSelectedNdcCtrl.fgValid = FALSE;

		if ((prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK) &&
		    (prPeerSchDesc->fgImmuNdlTimelineValid == TRUE)) {

			prImmuTimeline = prNegoCtrl->arImmuNdlTimeline;
			for (szTimeLineIdx = 0;
			     szTimeLineIdx < szNanActiveTimelineNum;
			     szTimeLineIdx++, prImmuTimeline++) {
				prNanTimelineMgmt =
					nanGetTimelineMgmt(prAdapter,
					szTimeLineIdx);

				if (prImmuTimeline->ucMapId !=
				    NAN_INVALID_MAP_ID)
					continue;

				kalMemZero(prImmuTimeline->au4AvailMap,
					sizeof(prImmuTimeline->au4AvailMap));
				prImmuTimeline->ucMapId =
						prNanTimelineMgmt->ucMapId;
				prImmuTimeline->ucLocalMapId =
						prNanTimelineMgmt->ucMapId;
			}
			nanSchedNegoMergeCrbByBand(prAdapter,
					     prNegoCtrl->arImmuNdlTimeline,
					     prPeerSchDesc->arImmuNdlTimeline);

			prPeerSchDesc->fgImmuNdlTimelineValid = FALSE;
		}

		if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING &&
		    prPeerSchDesc->fgRangingTimelineValid) {

			prRangingTimeline = prNegoCtrl->arRangingTimeline;
			for (szTimeLineIdx = 0;
			     szTimeLineIdx < szNanActiveTimelineNum;
			     szTimeLineIdx++, prRangingTimeline++) {

				prNanTimelineMgmt =
					nanGetTimelineMgmt(prAdapter,
					szTimeLineIdx);

				if (prRangingTimeline->ucMapId !=
				    NAN_INVALID_MAP_ID)
					continue;

				kalMemZero(&prRangingTimeline->au4AvailMap,
					sizeof(prRangingTimeline->au4AvailMap));
				prRangingTimeline->ucMapId =
					prNanTimelineMgmt->ucMapId;
				prRangingTimeline->ucLocalMapId =
					prNanTimelineMgmt->ucMapId;
			}
			nanSchedNegoMergeCrbByBand(prAdapter,
					     prNegoCtrl->arRangingTimeline,
					     prPeerSchDesc->arRangingTimeline);

			prPeerSchDesc->fgRangingTimelineValid = FALSE;
		}

		break;

	case ENUM_NEGO_CTRL_RST_PREPARE_WAIT_RSP_STATE:
		/* peer should propose the final NDC CRB in the last state
		 * (WAIT-RSP-STATE)
		 */
		prPeerSchDesc->rSelectedNdcCtrl.fgValid = FALSE;

		break;

	default:
		break;
	}

	return rRetStatus;
}

#define USE_GEN_DEF_CRB_V2 1 /* Use nanSchedNegoGenDefCrbV2() for Ranging */
uint32_t
nanSchedNegoGenLocalCrbProposal(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
#if (USE_GEN_DEF_CRB_V2 != 1)
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
#endif

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	DBGLOG(NAN, DEBUG, "\n\n");
	DBGLOG(NAN, DEBUG, "------>\n");

	nanSchedNegoDumpState(prAdapter, (uint8_t *) __func__, __LINE__);

	switch (prNegoCtrl->eState) {
	case ENUM_NAN_CRB_NEGO_STATE_INITIATOR:
		if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK) {
			/* Generate initial proposal */

			/* Determine local NDC schedule */
			if (prNegoCtrl->rSelectedNdcCtrl.fgValid == FALSE) {
				if (nanSchedNegoGenNdcCrb(prAdapter) ==
				    WLAN_STATUS_FAILURE)
					DBGLOG(NAN, WARN,
					       "gen local NDC fail\n");
			}

			/* check default CRB quota */
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
			nanSchedNegoGenDefCrbV2(prAdapter, FALSE);
#else
			nanSchedNegoGenDefCrb(prAdapter, FALSE);
#endif

			/* check QoS requirement */
			rRetStatus = nanSchedNegoChkQosSpec(prAdapter, FALSE);
			if (rRetStatus != WLAN_STATUS_SUCCESS)
				break;
		} else if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING) {
#if (USE_GEN_DEF_CRB_V2 == 1)
			nanSchedNegoGenDefCrbV2(prAdapter, FALSE);
#else
			/* check default CRB quota */
			prTimeline = prNegoCtrl->arRangingTimeline;
			for (szTimeLineIdx = 0;
			     szTimeLineIdx < szNanActiveTimelineNum;
			     szTimeLineIdx++, prTimeline++) {
				if (prTimeline->ucMapId != NAN_INVALID_MAP_ID)
					continue;

				rRetStatus = nanSchedNegoGenDefCrb(prAdapter,
								   FALSE);
			}
#endif
		}
		break;

	default:
		/* wrong state */
		rRetStatus = WLAN_STATUS_FAILURE;
		break;
	}

	if (rRetStatus == WLAN_STATUS_SUCCESS) {
		nanSchedCmdUpdateAvailability(prAdapter);
		prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP;

		/* NAN_CHK_PNT log message */
		nanSchedDbgDumpCommittedSlotAndChannel(prAdapter,
						       "ndl_gen_local_crb");
	} else {
		prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_CONFIRM;
	}

	DBGLOG(NAN, DEBUG, "<------\n");

	return rRetStatus;
}

uint32_t
nanSchedNegoDataPathChkRmtCrbProposalForRspState(struct ADAPTER *prAdapter,
						 uint32_t *pu4RejectCode)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SchIdx = 0;
	unsigned char fgCounterProposal = FALSE;
	uint32_t u4ReasonCode = NAN_REASON_CODE_RESERVED;
	unsigned char fgEmptyMapSet = FALSE;
	uint32_t aau4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW] = {0};
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint8_t fgDelayUpdateAvailability = FALSE;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
		goto DATA_RESPONDER_STATE_DONE;
	} else {
		prNDL = nanDataUtilSearchNdlByMac(prAdapter,
						  prPeerSchDesc->aucNmiAddr);
		if (prNDL &&
		    prNDL->prOperatingNDP &&
		    prNDL->prOperatingNDP->eCurrentNDPProtocolState ==
				NDP_RESPONDER_WAIT_DATA_RSP) {
			fgDelayUpdateAvailability =
				prNDL->prOperatingNDP->fgConfirmRequired ||
				prNDL->prOperatingNDP->fgSecurityRequired;
			DBGLOG(NAN, DEBUG, "NDP delay update availability: %u",
			       fgDelayUpdateAvailability);
		} else if (prNDL &&
			   prNDL->eCurrentNDLMgmtState ==
				NDL_RESPONDER_RX_SCHEDULE_REQUEST) {
			fgDelayUpdateAvailability =
				prNDL->fgNeedRespondCounter;
			DBGLOG(NAN, DEBUG,
			       "Reschedule delay update availability: %u\n",
			       fgDelayUpdateAvailability);
		}
	}

	/* Generate compliant/counter proposal */

	/* check if remote immutable NDL CRB conflict */
	if (prPeerSchDesc->fgImmuNdlTimelineValid == TRUE) {
		if (nanSchedNegoIsRmtCrbConflict(prAdapter,
				prPeerSchDesc->arImmuNdlTimeline,
				&fgEmptyMapSet, aau4EmptyMap) == TRUE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode = NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE;
			goto DATA_RESPONDER_STATE_DONE;
		} else {
			if (fgEmptyMapSet) {
				/* honor remote's channel sequence */
				nanSchedNegoDecideChnlInfoForEmptySlot(
					prAdapter, aau4EmptyMap);
			}
		}
	}

	/* check if local immutable NDL CRB conflict */
	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		if (prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx].ucMapId ==
		    NAN_INVALID_MAP_ID)
			continue;

		if (nanSchedNegoIsLocalCrbConflict(prAdapter,
			&prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx],
			FALSE, &fgEmptyMapSet, aau4EmptyMap,
			szTimeLineIdx) == TRUE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode = NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE;
			goto DATA_RESPONDER_STATE_DONE;
		}

		if (nanSchedNegoIsLocalCrbConflict(prAdapter,
			&prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx],
			TRUE, &fgEmptyMapSet, aau4EmptyMap,
			szTimeLineIdx) == TRUE) {
			/* expect the initiator can change conditional
			 * window to comply with immutable NDL
			 */
			fgCounterProposal = TRUE;
		} else {
			if (fgEmptyMapSet)
				fgCounterProposal = TRUE;
		}
	}

	/* Step1. Determine NDC CRB */
	if (prNegoCtrl->rSelectedNdcCtrl.fgValid == TRUE) {
		struct _NAN_NDC_CTRL_T *prPeerSelectedNdcCtrl;
		struct _NAN_NDC_CTRL_T *prNegoSelectedNdcCtrl;
		struct _NAN_SCHEDULE_TIMELINE_T *prNegoTimeline;

		prPeerSelectedNdcCtrl = &prPeerSchDesc->rSelectedNdcCtrl;
		prNegoSelectedNdcCtrl = &prNegoCtrl->rSelectedNdcCtrl;
		prNegoTimeline = prNegoSelectedNdcCtrl->arTimeline;

		if (prPeerSelectedNdcCtrl->fgValid == TRUE &&
		    !nanSchedNegoIsCrbEqual(prAdapter,
					   prNegoSelectedNdcCtrl->arTimeline,
					   prPeerSelectedNdcCtrl->arTimeline)) {
			fgCounterProposal = TRUE;
		}

		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {

			if (prNegoTimeline[szTimeLineIdx].ucMapId ==
			    NAN_INVALID_MAP_ID)
				continue;

			if (nanSchedNegoIsLocalCrbConflict(prAdapter,
						&prNegoTimeline[szTimeLineIdx],
						FALSE,
						&fgEmptyMapSet, aau4EmptyMap,
						szTimeLineIdx) == TRUE) {
				rRetStatus = WLAN_STATUS_FAILURE;
				u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
				goto DATA_RESPONDER_STATE_DONE;
			}

			if (nanSchedNegoIsLocalCrbConflict(prAdapter,
						&prNegoTimeline[szTimeLineIdx],
						TRUE,
						&fgEmptyMapSet, aau4EmptyMap,
						szTimeLineIdx) == TRUE) {
				fgCounterProposal = TRUE;
			} else {
				if (fgEmptyMapSet)
					fgCounterProposal = TRUE;
			}
		}
	} else if (prPeerSchDesc->rSelectedNdcCtrl.fgValid == TRUE) {
		/* TODO: counter strange NDC */
		if (nanSchedNegoIsRmtCrbConflict(prAdapter,
				    prPeerSchDesc->rSelectedNdcCtrl.arTimeline,
				    &fgEmptyMapSet, aau4EmptyMap) == TRUE) {

			rRetStatus = nanSchedNegoGenNdcCrb(prAdapter);
			if (rRetStatus != WLAN_STATUS_SUCCESS) {
				u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
				goto DATA_RESPONDER_STATE_DONE;
			}

			fgCounterProposal = TRUE;
		} else {
			if (fgEmptyMapSet) {
				/* honor remote's channel sequence */
				nanSchedNegoDecideChnlInfoForEmptySlot(
					prAdapter, aau4EmptyMap);
			}

			if (nanSchedNegoAllocNdcCtrl(prAdapter,
				    &prPeerSchDesc->rSelectedNdcCtrl) !=
			    WLAN_STATUS_SUCCESS) {
				rRetStatus = WLAN_STATUS_FAILURE;
				u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
				goto DATA_RESPONDER_STATE_DONE;
			}
		}
	} else {
		rRetStatus = nanSchedNegoGenNdcCrb(prAdapter);
		if (rRetStatus != WLAN_STATUS_SUCCESS) {
			u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
			goto DATA_RESPONDER_STATE_DONE;
		}
	}

	if (fgCounterProposal) {
		/* generate default CRB quota */
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
		nanSchedNegoGenDefCrbV2(prAdapter, FALSE);
#else
		nanSchedNegoGenDefCrb(prAdapter, FALSE);
#endif

		/* check QoS requirement */
		rRetStatus =
			nanSchedNegoChkQosSpecForRspState(prAdapter, FALSE);
		if (rRetStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR,
				"%d: Rsp check Qos fail status = 0x%x\n",
				__LINE__, rRetStatus);
			u4ReasonCode = NAN_REASON_CODE_QOS_UNACCEPTABLE;
			goto DATA_RESPONDER_STATE_DONE;
		}
	} else {
		/* check QoS requirement */
		rRetStatus = nanSchedNegoChkQosSpecForRspState(prAdapter, TRUE);
		if (rRetStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR,
				"%d: Rsp check Qos fail status = 0x%x\n",
				__LINE__, rRetStatus);
			nanSchedDbgDumpTimelineDb(prAdapter, __func__,
						  __LINE__);

			fgCounterProposal = TRUE;

			/* generate default CRB quota */
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
			nanSchedNegoGenDefCrbV2(prAdapter, FALSE);
#else
			nanSchedNegoGenDefCrb(prAdapter, FALSE);
#endif

			/* check QoS requirement */
			rRetStatus = nanSchedNegoChkQosSpecForRspState(
				prAdapter, FALSE);
			if (rRetStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, ERROR,
				"%d: Rsp check Qos fail status = 0x%x\n",
					__LINE__, rRetStatus);
				u4ReasonCode = NAN_REASON_CODE_QOS_UNACCEPTABLE;
				goto DATA_RESPONDER_STATE_DONE;
			}

		} else {
			uint32_t u4NegoTransIdx =
				nanSchedGetCurrentNegoTransIdx(prAdapter);
			struct _NAN_CRB_NEGO_TRANSACTION_T *prCurrNegoTrans;

			/* check default CRB quota */
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
			nanSchedNegoGenDefCrbV2(prAdapter, TRUE);
#else
			nanSchedNegoGenDefCrb(prAdapter, TRUE);
#endif

			/* Counter if peer reschedule req NDL slot bring 6G,
			 * but we already committed in 5G in previous nego
			 */
			prCurrNegoTrans =
				&prNegoCtrl->rNegoTrans[u4NegoTransIdx];
			if (prCurrNegoTrans &&
			    (prCurrNegoTrans->u4NotChoose6GCnt ||
			     prCurrNegoTrans->fgCounterCountry)) {
				fgCounterProposal = TRUE;
				DBGLOG(NAN, DEBUG,
				       "Counter peer because not choose 6G (%u), counter=%u\n",
				       prCurrNegoTrans->u4NotChoose6GCnt,
				       prCurrNegoTrans->fgCounterCountry);
			}
		}
	}

DATA_RESPONDER_STATE_DONE:
	if ((rRetStatus == WLAN_STATUS_SUCCESS) &&
	     (fgCounterProposal == TRUE || fgDelayUpdateAvailability == TRUE))
		rRetStatus = WLAN_STATUS_PENDING;

	if (pu4RejectCode != NULL)
		*pu4RejectCode = u4ReasonCode;

	return rRetStatus;
}

uint32_t
nanSchedNegoRangingChkRmtCrbProposalForRspState(struct ADAPTER *prAdapter,
						      uint32_t *pu4RejectCode)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4ReasonCode = NAN_REASON_CODE_RESERVED;
	unsigned char fgEmptyMapSet = FALSE;
	uint32_t aau4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW] = {0};
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	size_t i;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
		goto RANG_RESPONDER_STATE_DONE;
	}

	/* Step1. check initial ranging proposal */
	if (prPeerSchDesc->fgRangingTimelineValid == TRUE) {
		for (i = 0; i < ARRAY_SIZE(prPeerSchDesc->arRangingTimeline);
		     i++) {
			struct _NAN_SCHEDULE_TIMELINE_T *prTimeline =
				&prPeerSchDesc->arRangingTimeline[i];

			nanUtilDump(prAdapter, "Ranging Map",
				    (uint8_t *)prTimeline->au4AvailMap,
				    sizeof(prTimeline->au4AvailMap));
		}
		if (nanSchedNegoIsRmtCrbConflict(prAdapter,
				prPeerSchDesc->arRangingTimeline,
				&fgEmptyMapSet, aau4EmptyMap) == TRUE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode =
				NAN_REASON_CODE_RANGING_SCHEDULE_UNACCEPTABLE;
			goto RANG_RESPONDER_STATE_DONE;
		} else {
			if (fgEmptyMapSet) {
				nanSchedNegoDecideChnlInfoForEmptySlot(
					prAdapter, aau4EmptyMap);
			}
		}
	} else {
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_RANGING_SCHEDULE_UNACCEPTABLE;
		goto RANG_RESPONDER_STATE_DONE;
	}

RANG_RESPONDER_STATE_DONE:
	if (pu4RejectCode != NULL)
		*pu4RejectCode = u4ReasonCode;

	return rRetStatus;
}

const char *nanGetNanReasonString(uint32_t u4ReasonCode)
{
	static const char * const reason_code_str[] = {
		[NAN_REASON_CODE_RESERVED] = "Reserved",
		[NAN_REASON_CODE_UNSPECIFIED] = "Unspecified",
		[NAN_REASON_CODE_RESOURCE_LIMITATION] = "Resource_Limitation",
		[NAN_REASON_CODE_INVALID_PARAMS] = "Invalid_Params",
		[NAN_REASON_CODE_FTM_PARAMS_INCAPABLE] = "Ftm_Params_Incapable",
		[NAN_REASON_CODE_NO_MOVEMENT] = "No_Movement",
		[NAN_REASON_CODE_INVALID_AVAILABILITY] = "Invalid_Availability",
		[NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE] =
			"Immutable_Unacceptable",
		[NAN_REASON_CODE_SECURITY_POLICY] = "Security_Policy",
		[NAN_REASON_CODE_QOS_UNACCEPTABLE] = "Qos_Unacceptable",
		[NAN_REASON_CODE_NDP_REJECTED] = "NDP_Rejected",
		[NAN_REASON_CODE_NDL_UNACCEPTABLE] = "NDL_Unacceptable",
		[NAN_REASON_CODE_RANGING_SCHEDULE_UNACCEPTABLE] =
			"Ranging_Schedule_Unacceptable",
		[NAN_REASON_CODE_PAIRING_BOOTSTRAPPING_REJECTED] =
			"Pairing_Bootstrapping_Rejected",
	};


	if (u4ReasonCode < ARRAY_SIZE(reason_code_str))
		return reason_code_str[u4ReasonCode];

	return "";
}

uint32_t
nanSchedNegoDataPathChkRmtCrbProposalForWaitRspState(struct ADAPTER *prAdapter,
						     uint32_t *pu4RejectCode)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4ReasonCode = NAN_REASON_CODE_RESERVED;
	unsigned char fgEmptyMapSet = 0;
	uint32_t aau4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW] = {0};
	uint32_t u4SchIdx = 0;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	u4SchIdx = prNegoCtrl->u4SchIdx;
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, u4SchIdx);

	nanSchedNegoResetControlInfo(prAdapter,
				     ENUM_NEGO_CTRL_RST_BY_WAIT_RSP_STATE);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
		goto WAIT_RSP_STATE_DONE;
	}

	/* Generate confirm proposal */

	/* check if remote immutable NDL CRB conflict */
	if (prPeerSchDesc->fgImmuNdlTimelineValid == TRUE) {
		if (nanSchedNegoIsRmtCrbConflict(prAdapter,
				prPeerSchDesc->arImmuNdlTimeline,
				&fgEmptyMapSet, aau4EmptyMap) == TRUE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode = NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE;
			goto WAIT_RSP_STATE_DONE;
		} else {
			if (fgEmptyMapSet) {
				nanSchedNegoDecideChnlInfoForEmptySlot(
					prAdapter, aau4EmptyMap);
			}
		}
	}
	/* check if local immutable NDL CRB conflict */
	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		if (prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx].ucMapId
			!= NAN_INVALID_MAP_ID) {
			if (nanSchedNegoIsLocalCrbConflict(
				prAdapter,
				&prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx],
				TRUE, &fgEmptyMapSet, aau4EmptyMap,
				szTimeLineIdx) == TRUE) {
				rRetStatus = WLAN_STATUS_FAILURE;
				u4ReasonCode =
				NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE;
				goto WAIT_RSP_STATE_DONE;
			} else {
				if (fgEmptyMapSet) {
					rRetStatus = WLAN_STATUS_FAILURE;
					u4ReasonCode =
					NAN_REASON_CODE_IMMUTABLE_UNACCEPTABLE;
					goto WAIT_RSP_STATE_DONE;
				}
			}
		}
	}

	/* determine NDC CRB */
	if (prPeerSchDesc->rSelectedNdcCtrl.fgValid == TRUE) {
		if (nanSchedNegoIsRmtCrbConflict(prAdapter,
				 prPeerSchDesc->rSelectedNdcCtrl.arTimeline,
				 &fgEmptyMapSet, aau4EmptyMap) == TRUE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
			goto WAIT_RSP_STATE_DONE;
		} else {
			if (fgEmptyMapSet) {
				nanSchedNegoDecideChnlInfoForEmptySlot(
					prAdapter, aau4EmptyMap);
			}

			if (nanSchedNegoAllocNdcCtrl(prAdapter,
				    &prPeerSchDesc->rSelectedNdcCtrl) !=
			    WLAN_STATUS_SUCCESS) {
				rRetStatus = WLAN_STATUS_FAILURE;
				u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
				goto WAIT_RSP_STATE_DONE;
			}
		}
	} else {
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_NDL_UNACCEPTABLE;
		goto WAIT_RSP_STATE_DONE;
	}

	/* check QoS requirement */
	rRetStatus = nanSchedNegoChkQosSpec(prAdapter, TRUE);
	if (rRetStatus != WLAN_STATUS_SUCCESS) {
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_QOS_UNACCEPTABLE;
		goto WAIT_RSP_STATE_DONE;
	}

	/* check default CRB quota */
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
	nanSchedNegoGenDefCrbV2(prAdapter, TRUE);
#else
	nanSchedNegoGenDefCrb(prAdapter, TRUE);
#endif

WAIT_RSP_STATE_DONE:

	if (pu4RejectCode != NULL)
		*pu4RejectCode = u4ReasonCode;

	DBGLOG(NAN, DEBUG, "*pu4RejectCode = %u(%s)\n",
	       u4ReasonCode, nanGetNanReasonString(u4ReasonCode));

	return rRetStatus;
}

uint32_t
nanSchedNegoRangingChkRmtCrbProposalForWaitRspState(struct ADAPTER *prAdapter,
						    uint32_t *pu4RejectCode)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4ReasonCode = NAN_REASON_CODE_RESERVED;
	unsigned char fgEmptyMapSet = FALSE;
	uint32_t aau4EmptyMap[NAN_TIMELINE_MGMT_SIZE][NAN_TOTAL_DW] = {0};
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	size_t i;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);

	if (prPeerSchDesc == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchDesc\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
		goto WAIT_RSP_STATE_DONE;
	}

	if (prPeerSchDesc->fgRangingTimelineValid == TRUE) {
		for (i = 0; i < ARRAY_SIZE(prPeerSchDesc->arRangingTimeline);
		     i++) {
			struct _NAN_SCHEDULE_TIMELINE_T *prTimeline =
				&prPeerSchDesc->arRangingTimeline[i];

			nanUtilDump(prAdapter, "Ranging Map",
				    (uint8_t *)prTimeline->au4AvailMap,
				    sizeof(prTimeline->au4AvailMap));
		}
		if (nanSchedNegoIsRmtCrbConflict(prAdapter,
				 prPeerSchDesc->arRangingTimeline,
				 &fgEmptyMapSet, aau4EmptyMap) == TRUE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode =
				NAN_REASON_CODE_RANGING_SCHEDULE_UNACCEPTABLE;
			goto WAIT_RSP_STATE_DONE;
		} else {
			if (fgEmptyMapSet) {
				nanSchedNegoDecideChnlInfoForEmptySlot(
					prAdapter, aau4EmptyMap);
			}
		}
	} else {
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_RANGING_SCHEDULE_UNACCEPTABLE;
		goto WAIT_RSP_STATE_DONE;
	}

WAIT_RSP_STATE_DONE:

	if (pu4RejectCode != NULL)
		*pu4RejectCode = u4ReasonCode;

	return rRetStatus;
}

uint32_t nanSchedNegoChkRmtCrbProposal(struct ADAPTER *prAdapter,
				       uint32_t *pu4RejectCode)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4ReasonCode = NAN_REASON_CODE_RESERVED;
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prPeerSchRec =
		nanSchedGetPeerSchRecord(prAdapter, prNegoCtrl->u4SchIdx);

	nanSchedNegoDumpState(prAdapter, (uint8_t *) __func__, __LINE__);

	if (nanSchedNegoIsRmtAvailabilityConflict(prAdapter) != 0) {
		u4ReasonCode = NAN_REASON_CODE_INVALID_AVAILABILITY;
		rRetStatus = WLAN_STATUS_FAILURE;
		goto RMT_PROPOSAL_DONE;
	}

	DBGLOG(NAN, DEBUG, "state = %u\n", prNegoCtrl->eState);
	switch (prNegoCtrl->eState) {
	case ENUM_NAN_CRB_NEGO_STATE_RESPONDER:
		if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK) {
			rRetStatus =
			nanSchedNegoDataPathChkRmtCrbProposalForRspState(
			    prAdapter, &u4ReasonCode);
		} else if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING) {
			rRetStatus =
			nanSchedNegoRangingChkRmtCrbProposalForRspState(
			    prAdapter, &u4ReasonCode);
		} else {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
			goto RMT_PROPOSAL_DONE;
		}
		break;

	case ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP:
		if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK) {
			rRetStatus =
			nanSchedNegoDataPathChkRmtCrbProposalForWaitRspState(
			    prAdapter, &u4ReasonCode);
		} else if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING) {
			rRetStatus =
			nanSchedNegoRangingChkRmtCrbProposalForWaitRspState(
			    prAdapter, &u4ReasonCode);
		} else {
			rRetStatus = WLAN_STATUS_FAILURE;
			u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
			goto RMT_PROPOSAL_DONE;
		}
		break;

	case ENUM_NAN_CRB_NEGO_STATE_CONFIRM:
		rRetStatus = WLAN_STATUS_SUCCESS;
		u4ReasonCode = NAN_REASON_CODE_RESERVED;
		break;

	default:
		/* wrong state */
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
		break;
	}

RMT_PROPOSAL_DONE:
	if (prPeerSchRec == NULL) {
		DBGLOG(NAN, ERROR, "NULL prPeerSchRec\n");
		rRetStatus = WLAN_STATUS_FAILURE;
		u4ReasonCode = NAN_REASON_CODE_UNSPECIFIED;
	}

	if (rRetStatus == WLAN_STATUS_PENDING) {
		rRetStatus = WLAN_STATUS_NOT_ACCEPTED;
		prNDL = nanDataUtilSearchNdlByMac(
			prAdapter, prPeerSchRec->aucNmiAddr);
		if (prNDL &&
		    prNDL->eCurrentNDLMgmtState ==
		    NDL_RESPONDER_RX_SCHEDULE_REQUEST) {
			DBGLOG(NAN, INFO, "Set NDL state as CONFIRM\n");
			prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_CONFIRM;
			nanSchedNegoResetControlInfo(prAdapter,
				ENUM_NEGO_CTRL_RST_PREPARE_CONFIRM_STATE);
		} else {
			prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP;
			nanSchedNegoResetControlInfo(prAdapter,
				ENUM_NEGO_CTRL_RST_PREPARE_WAIT_RSP_STATE);
		}
		nanSchedCmdUpdateAvailability(prAdapter);
	} else if (rRetStatus == WLAN_STATUS_SUCCESS) {
		prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_CONFIRM;
		nanSchedNegoResetControlInfo(
			prAdapter, ENUM_NEGO_CTRL_RST_PREPARE_CONFIRM_STATE);

		/* save negotiation result to peer sch record */
		if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK) {
			prNdcCtrl = nanSchedGetNdcCtrl(prAdapter,
				prNegoCtrl->rSelectedNdcCtrl.aucNdcId);
			if (prNdcCtrl == NULL) {
				prNdcCtrl = nanSchedAcquireNdcCtrl(prAdapter);
				if (prNdcCtrl == NULL) {
					rRetStatus = WLAN_STATUS_FAILURE;
					u4ReasonCode =
						NAN_REASON_CODE_UNSPECIFIED;
					goto RMT_PROPOSAL_DONE;
				}

				kalMemCopy(prNdcCtrl,
					   &prNegoCtrl->rSelectedNdcCtrl,
					   sizeof(struct _NAN_NDC_CTRL_T));
			}
			prPeerSchRec->prCommNdcCtrl = prNdcCtrl;
			DBGLOG(NAN, INFO,
			       "Set NDC %02x-%02x-%02x-%02x-%02x-%02x",
			       prNdcCtrl->aucNdcId[0], prNdcCtrl->aucNdcId[1],
			       prNdcCtrl->aucNdcId[2], prNdcCtrl->aucNdcId[3],
			       prNdcCtrl->aucNdcId[4], prNdcCtrl->aucNdcId[5]);

			prPeerSchRec->u4DefNdlNumSlots =
				prNegoCtrl->u4DefNdlNumSlots;

			kalMemCopy(&prPeerSchRec->arCommImmuNdlTimeline,
				   &prNegoCtrl->arImmuNdlTimeline,
				   sizeof(prNegoCtrl->arImmuNdlTimeline));

			prPeerSchRec->u4FinalQosMinSlots =
				prNegoCtrl->u4NegoQosMinSlots;
			prPeerSchRec->u4FinalQosMaxLatency =
				prNegoCtrl->u4NegoQosMaxLatency;

			prPeerSchRec->fgUseDataPath = TRUE;
		} else if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING) {
			prPeerSchRec->u4DefRangingNumSlots =
				prNegoCtrl->u4DefRangingNumSlots;

			kalMemCopy(&prPeerSchRec->arCommRangingTimeline,
				   &prNegoCtrl->arRangingTimeline,
				   sizeof(prNegoCtrl->arRangingTimeline));

			prPeerSchRec->fgUseRanging = TRUE;
		}

		nanSchedNegoUpdateNegoResult(prAdapter);

		/* NAN_CHK_PNT log message */
		nanSchedDbgDumpCommittedSlotAndChannel(prAdapter,
						       "ndl_chk_rmt_crb");
	} else {
		prNegoCtrl->eState = ENUM_NAN_CRB_NEGO_STATE_CONFIRM;
	}

	if (pu4RejectCode != NULL)
		*pu4RejectCode = u4ReasonCode;

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
	DBGLOG(NAN, DEBUG, "Reason:%u(%s)\n",
	       u4ReasonCode, nanGetNanReasonString(u4ReasonCode));

	return rRetStatus;
}

uint32_t
nanSchedNegoGetImmuNdlScheduleList(struct ADAPTER *prAdapter,
					uint8_t **ppucScheduleList,
					uint32_t *pu4ScheduleListLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = g_aucNanIEBuffer;
	uint32_t u4RetLength = 0;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntry = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_SCHEDULE_TIMELINE_T *prNdlTL = NULL;


	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	kalMemZero(g_aucNanIEBuffer, sizeof(g_aucNanIEBuffer));
	do {
		if ((prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR) &&
		    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER) &&
		    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP) &&
		    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_CONFIRM)) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		if (prNegoCtrl->eType != ENUM_NAN_NEGO_DATA_LINK) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {

			prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);

			prNdlTL = &prNegoCtrl->arImmuNdlTimeline[szTimeLineIdx];
			if (prNdlTL->ucMapId != NAN_INVALID_MAP_ID) {
				prScheduleEntry =
					(struct _NAN_SCHEDULE_ENTRY_T *)pucPos;
				prScheduleEntry->ucMapID =
					prNanTimelineMgmt->ucMapId;
				/* TODO: use ucLocalMapId? */
				pucPos += 1 /* MapID(1) */;
				nanParserGenTimeBitmapField(
					prAdapter,
					prNdlTL->au4AvailMap,
					pucPos, &u4RetLength);
				pucPos += u4RetLength;
			}
		}
	} while (FALSE);

	if (ppucScheduleList)
		*ppucScheduleList = g_aucNanIEBuffer;
	if (pu4ScheduleListLength)
		*pu4ScheduleListLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

uint32_t
nanSchedNegoGetRangingScheduleList(struct ADAPTER *prAdapter,
					uint8_t **ppucScheduleList,
					uint32_t *pu4ScheduleListLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = g_aucNanIEBuffer;
	uint32_t u4RetLength = 0;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_SCHEDULE_ENTRY_T *prScheduleEntry = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_SCHEDULE_TIMELINE_T *prRangngTimeline = NULL;


	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	kalMemZero(g_aucNanIEBuffer, sizeof(g_aucNanIEBuffer));
	do {
		if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR &&
		    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER &&
		    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP &&
		    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_CONFIRM) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		if (prNegoCtrl->eType != ENUM_NAN_NEGO_RANGING) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}


		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {

			prRangngTimeline =
				&prNegoCtrl->arRangingTimeline[szTimeLineIdx];
			DBGLOG(NAN, INFO,
			       "timeline=%zu, MapId=%u, avail=%02x-%02x-%02x-%02x",
			       szTimeLineIdx, prRangngTimeline->ucMapId,
			       ((uint8_t *)(prRangngTimeline->au4AvailMap))[0],
			       ((uint8_t *)(prRangngTimeline->au4AvailMap))[1],
			       ((uint8_t *)(prRangngTimeline->au4AvailMap))[2],
			       ((uint8_t *)(prRangngTimeline->au4AvailMap))[3]);

			if (prRangngTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			prScheduleEntry =
				(struct _NAN_SCHEDULE_ENTRY_T *)pucPos;
			prScheduleEntry->ucMapID = prRangngTimeline->ucMapId;
			/* TODO: use ucLocalMapId? */
			pucPos += 1 /* MapID(1) */;
			nanParserGenTimeBitmapField(prAdapter,
						prRangngTimeline->au4AvailMap,
						pucPos, &u4RetLength);
			pucPos += u4RetLength;
		}
	} while (FALSE);

	if (ppucScheduleList)
		*ppucScheduleList = g_aucNanIEBuffer;
	if (pu4ScheduleListLength)
		*pu4ScheduleListLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

uint32_t
nanSchedNegoGetQosAttr(struct ADAPTER *prAdapter, uint8_t **ppucQosAttr,
				uint32_t *pu4QosLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = g_aucNanIEBuffer;
	struct _NAN_ATTR_NDL_QOS_T *prNdlQosAttr = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl;


	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	kalMemZero(g_aucNanIEBuffer, sizeof(g_aucNanIEBuffer));
	do {
		if ((prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR) &&
		    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER) &&
		    (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP)) {
			rRetStatus = WLAN_STATUS_FAILURE;
			break;
		}

		if ((prNegoCtrl->u4QosMaxLatency <
		     NAN_INVALID_QOS_MAX_LATENCY) ||
		    (prNegoCtrl->u4QosMinSlots > NAN_INVALID_QOS_MIN_SLOTS)) {
			prNdlQosAttr = (struct _NAN_ATTR_NDL_QOS_T *)pucPos;
			pucPos += sizeof(struct _NAN_ATTR_NDL_QOS_T);

			prNdlQosAttr->ucAttrId = NAN_ATTR_ID_NDL_QOS;
			prNdlQosAttr->u2Length = 3;
			prNdlQosAttr->u2MaxLatency =
				prNegoCtrl->u4QosMaxLatency;
			prNdlQosAttr->ucMinTimeSlot = prNegoCtrl->u4QosMinSlots;
		}
	} while (FALSE);

	if (ppucQosAttr)
		*ppucQosAttr = g_aucNanIEBuffer;
	if (pu4QosLength)
		*pu4QosLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

uint32_t
nanSchedNegoGetSelectedNdcAttr(struct ADAPTER *prAdapter, uint8_t **ppucNdcAttr,
					uint32_t *pu4NdcAttrLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = NULL;
	uint32_t u4RetLength = 0;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_ATTR_NDC_T *prNdcAttr = NULL;
	struct _NAN_SCHEDULE_ENTRY_T *prNanScheduleEntry = NULL;
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_SCHEDULE_TIMELINE_T *prNdcTimeline;

	do {
		pucPos = g_aucNanIEBuffer;

		prNegoCtrl = nanGetNegoControlBlock(prAdapter);

		prNdcCtrl = &prNegoCtrl->rSelectedNdcCtrl;
		if (prNdcCtrl->fgValid == FALSE) {
			rRetStatus = WLAN_STATUS_FAILURE;
			DBGLOG(NAN, ERROR,
			       "Can't find valid NDC control block\n");
			break;
		}

		kalMemZero(g_aucNanIEBuffer, sizeof(g_aucNanIEBuffer));

		prNdcAttr = (struct _NAN_ATTR_NDC_T *)pucPos;
		prNdcAttr->ucAttrId = NAN_ATTR_ID_NDC;
		kalMemCopy(prNdcAttr->aucNDCID, prNdcCtrl->aucNdcId,
			   NAN_NDC_ATTRIBUTE_ID_LENGTH);
		prNdcAttr->ucAttributeControl |=
			NAN_ATTR_NDC_CTRL_SELECTED_FOR_NDL;
		prNdcAttr->u2Length = 7; /* ID(6)+Attribute Control(1) */

		pucPos = prNdcAttr->aucScheduleEntryList;
		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {
			prNanScheduleEntry =
				(struct _NAN_SCHEDULE_ENTRY_T *)pucPos;

			prNdcTimeline = &prNdcCtrl->arTimeline[szTimeLineIdx];
			if (prNdcTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			prNanScheduleEntry->ucMapID =
				prNdcTimeline->ucLocalMapId;

			pucPos++; /* Map ID(1) */
			nanParserGenTimeBitmapField(prAdapter,
				prNdcTimeline->au4AvailMap,
				pucPos,
				&u4RetLength);
			prNdcAttr->u2Length += (1 /*MapID*/ + u4RetLength);
			pucPos += u4RetLength;
		}
	} while (FALSE);

	*ppucNdcAttr = g_aucNanIEBuffer;
	*pu4NdcAttrLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

/**
 * setEntryControl() - Compose Entry Control field in Availability attribute
 * @ucAvailType: NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT,
 *	         NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN,
 *	         NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND
 * @ucPref: range 0 ~ 3, larger value has higher preference
 * @ucUtil: range 0 ~ 5, already utilized for other purpose, multiply by 20%
 * @ucRxNss: the max number of spatial streams can receive
 * @fgTbitmapPresent: indicate whether Time Bitmap Control, Time Bitmap Length,
 *                    and Time Bitmap fields are present
 *
 * Return: composed Entry Control integer
 */
static uint16_t setEntryControl(uint8_t ucAvailType, uint8_t ucPref,
				uint8_t ucUtil, uint8_t ucRxNss,
				uint8_t fgTbitmapPresent)
{
	uint16_t u2EntryControl = 0;

	NAN_AVAIL_ENTRY_CTRL_SET_TYPE(u2EntryControl, ucAvailType);
	NAN_AVAIL_ENTRY_CTRL_SET_PREF(u2EntryControl, ucPref);
	NAN_AVAIL_ENTRY_CTRL_SET_UTIL(u2EntryControl, ucUtil);
	NAN_AVAIL_ENTRY_CTRL_SET_NSS(u2EntryControl, ucRxNss);
	NAN_AVAIL_ENTRY_CTRL_SET_TBITMAP_P(u2EntryControl, fgTbitmapPresent);

	DBGLOG(NAN, DEBUG,
	       "u2EntryControl=0x%02x, (Type=%u C:%u/p:%u/c:%u, pref=%u, util=%u, nss=%u, bit=%u)\n",
	       u2EntryControl, ucAvailType,
	       NAN_AVAIL_ENTRY_CTRL_COMMITTED(u2EntryControl),
	       NAN_AVAIL_ENTRY_CTRL_POTENTIAL(u2EntryControl),
	       NAN_AVAIL_ENTRY_CTRL_CONDITIONAL(u2EntryControl),
	       ucPref, ucUtil,
	       ucRxNss, fgTbitmapPresent);
	return u2EntryControl;
}

uint32_t
nanSchedAddPotentialWindows(struct ADAPTER *prAdapter, uint8_t *pucBuf,
	size_t szTimeLineIdx)
{
#if (CFG_SUPPORT_NAN_6G == 1)
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	u_int8_t fgNanPotentialBand =
			prWifiVar->ucNanBandChnlType ==
			NAN_BAND_CH_ENTRY_LIST_TYPE_BAND;
	u_int8_t fgNanPotentialChannel =
			prWifiVar->ucNanBandChnlType ==
			NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
#else /* CFG_SUPPORT_NAN_6G != 1 */
	u_int8_t fgNanPotentialBand = FALSE;
	u_int8_t fgNanPotentialChannel = TRUE;
#endif
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	uint8_t *pucPos;
	uint8_t *pucTmp;
	uint32_t u4EntryIdx;
	struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry;
	uint32_t u4RetLength;
	/* uint32_t u2EntryControl; */
	uint32_t au4PotentialAvailMap[NAN_TOTAL_DW];
	struct _NAN_SCHEDULER_T *prScheduler;
	uint32_t u4Idx;
	union _NAN_BAND_CHNL_CTRL rPotentialBandInfo;

	uint8_t *pucPotentialChnls;
	uint32_t u4PotentialChnlSize;

	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo;
	struct BSS_INFO *prBssInfo;
	uint8_t ucOpRxNss = 1;

	union _NAN_BAND_CHNL_CTRL rAisChnlInfo = g_rNullChnl;
	uint32_t u4SlotBitmap = 0;
	uint8_t ucPhyTypeSet = 0;
	uint8_t ucAisPrimaryChnl = 0;
	size_t szSlotIdx = 0;


	prScheduler = nanGetScheduler(prAdapter);
	prNanSpecificBssInfo =
		nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prNanSpecificBssInfo->ucBssIndex);

	if (prBssInfo == NULL)
		DBGLOG(NAN, ERROR, "NULL prBssInfo, idx=%d\n",
		       prNanSpecificBssInfo->ucBssIndex);
	else
		ucOpRxNss = prBssInfo->ucOpRxNss;

	pucPos = pucBuf;

	/**
	 * FIXME:
	 * Expected:
	 *  single map:
	 *	Map 1 (2G/5G+6G): fe-fe-ff-ff
	 *  dual map:
	 *	Map 1 (2G):       fe-ff-ff-ff
	 *	Map 2 (5G+6G):    ff-fe-ff-ff
	 */
	kalMemSet(au4PotentialAvailMap, 0xFF, sizeof(au4PotentialAvailMap));
	if (prScheduler->fgEn2g &&
	    NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
	    (fgNanPotentialBand ||
	     !fgNanPotentialBand && szNanActiveTimelineNum > 1)) {
		for (u4EntryIdx = 0; u4EntryIdx < NAN_TOTAL_DW; u4EntryIdx++)
			NAN_TIMELINE_UNSET(au4PotentialAvailMap,
				NAN_FULL_SLOT_INDEX(u4EntryIdx,
						    NAN_2G_DW_INDEX));
	}
	if ((prScheduler->fgEn5gH || prScheduler->fgEn5gL) &&
	    NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx) &&
	    (fgNanPotentialBand ||
	     !fgNanPotentialBand && szNanActiveTimelineNum > 1)) {
		for (u4EntryIdx = 0; u4EntryIdx < NAN_TOTAL_DW; u4EntryIdx++)
			NAN_TIMELINE_UNSET(au4PotentialAvailMap,
				NAN_FULL_SLOT_INDEX(u4EntryIdx,
						    NAN_5G_DW_INDEX));
	}

	/* Remove AIS slot in potential when infra channel is DFS(5G) */
	nanSchedGetConnChnlUsage(prAdapter, NETWORK_TYPE_AIS, BAND_5G,
				 &rAisChnlInfo, &u4SlotBitmap, &ucPhyTypeSet);

	ucAisPrimaryChnl = (uint8_t) rAisChnlInfo.u4PrimaryChnl;

	if (ucAisPrimaryChnl != 0 &&
	    rlmDomainIsDfsChnls(prAdapter, ucAisPrimaryChnl)) {
		for (szSlotIdx = 0;
		     szSlotIdx < NAN_TOTAL_SLOT_WINDOWS;
		     szSlotIdx++) {
			if (!NAN_SLOT_IS_AIS(prAdapter, szTimeLineIdx,
					    NAN_SLOT_INDEX(szSlotIdx)))
				continue;

			NAN_TIMELINE_UNSET(au4PotentialAvailMap, szSlotIdx);
		}
	}

	/* process_potential_channel */
	if (!fgNanPotentialChannel)
		goto process_potential_band;

	pucTmp = pucPos;

	prAvailEntry = (struct _NAN_AVAILABILITY_ENTRY_T *)pucTmp;

	prAvailEntry->u2EntryControl =
		setEntryControl(NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN, 3, 0,
				ucOpRxNss, 1);
	pucPos += 4 /* length(2)+entry control(2) */;

	nanParserGenTimeBitmapField(prAdapter, au4PotentialAvailMap, pucPos,
				    &u4RetLength);
	pucPos += u4RetLength;

	*pucPos = ((prScheduler->au4NumOfPotentialChnlList[szTimeLineIdx] <<
			    NAN_BAND_CH_ENTRY_LIST_NUM_ENTRY_OFFSET) |
		   (NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL));
	pucPos++;
	for (u4Idx = 0;
	     u4Idx < prScheduler->au4NumOfPotentialChnlList[szTimeLineIdx];
	     u4Idx++) {
		pucPotentialChnls = (uint8_t *)
		       &prScheduler->aarPotentialChnlList[szTimeLineIdx][u4Idx];
		u4PotentialChnlSize = 4;
		/* struct _NAN_CHNL_ENTRY_T excluding u2AuxChannelBitmap */

		kalMemCopy(pucPos, pucPotentialChnls, u4PotentialChnlSize);
#if (CFG_SUPPORT_NAN_6G == 1)
		/* prAdapter->rWifiVar.ucNanUseR4AvailAttr: static configuration
		 * g_fgNanUseR4AvailAttr: a runtime dynamic flag
		 */
		if (!prAdapter->rWifiVar.b1NanPotentialUseR4AvailAttr)
			nanChannelBitmapR4ToR3(pucPos);
#endif
		pucPos += u4PotentialChnlSize;
	}

	prAvailEntry->u2Length = (pucPos - pucTmp) - 2 /* length(2) */;

process_potential_band:
	if (!fgNanPotentialBand)
		goto process_potential_finished;

	rPotentialBandInfo.u4BandIdMask = 0;
	rPotentialBandInfo.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_BAND;
	if (prScheduler->fgEn2g &&
	    NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx))
		rPotentialBandInfo.u4BandIdMask |=
			BIT(NAN_SUPPORTED_BAND_ID_2P4G);
	if ((prScheduler->fgEn5gH || prScheduler->fgEn5gL) &&
	    NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx))
		rPotentialBandInfo.u4BandIdMask |=
			BIT(NAN_SUPPORTED_BAND_ID_5G);
#if (CFG_SUPPORT_NAN_6G == 1)
	if (prScheduler->fgEn6g &&
	    NAN_IS_6G_TIMELINE(prAdapter, szTimeLineIdx))
		rPotentialBandInfo.u4BandIdMask |=
			BIT(NAN_SUPPORTED_BAND_ID_6G);
#endif

	if (rPotentialBandInfo.u4BandIdMask != 0) {
		pucTmp = pucPos;
		prAvailEntry = (struct _NAN_AVAILABILITY_ENTRY_T *)pucTmp;

		prAvailEntry->u2EntryControl =
			setEntryControl(NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN,
					3, 0, ucOpRxNss, 1);
		pucPos += 4 /* length(2)+entry control(2) */;

		nanParserGenTimeBitmapField(prAdapter, au4PotentialAvailMap,
					    pucPos, &u4RetLength);
		pucPos += u4RetLength;

		nanParserGenBandChnlEntryListField(prAdapter,
						   &rPotentialBandInfo, 1,
						   pucPos, &u4RetLength);
		pucPos += u4RetLength;

		prAvailEntry->u2Length = (pucPos - pucTmp) - 2 /* length(2) */;
	}

process_potential_finished:

	nanUtilDump(prAdapter, "Potential Windows", pucBuf, (pucPos - pucBuf));
	return (pucPos - pucBuf);
}

/* Set max band to eMaxBand according to bitmap ucMaxSupportedBands */
static enum ENUM_BAND getMaxBand(uint8_t ucMaxSupportedBands)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (ucMaxSupportedBands & BIT(NAN_SUPPORTED_BAND_ID_6G))
		return BAND_6G;
#endif
	if (ucMaxSupportedBands & BIT(NAN_SUPPORTED_BAND_ID_5G))
		return BAND_5G;

	if (ucMaxSupportedBands & BIT(NAN_SUPPORTED_BAND_ID_2P4G))
		return BAND_2G4;

	return BAND_2G4;
}

enum ENUM_BAND
getPeerSchDescMaxCap(struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc)
{
	struct _NAN_DEVICE_CAPABILITY_T *prDevCapList;
	enum ENUM_BAND eAllMaxBand;
	uint32_t j;
	uint8_t ucAllMaxSupportedBands = BIT(NAN_SUPPORTED_BAND_ID_2P4G) |
		BIT(NAN_SUPPORTED_BAND_ID_5G) |
		BIT(NAN_SUPPORTED_BAND_ID_6G);

	prDevCapList = prPeerSchDesc->arDevCapability;
	for (j = 0; j < NAN_NUM_AVAIL_DB + 1; j++, prDevCapList++) {
		if (!prDevCapList->fgValid)
			continue;

		ucAllMaxSupportedBands &= prDevCapList->ucSupportedBand;
	}

	eAllMaxBand = getMaxBand(ucAllMaxSupportedBands);

	return eAllMaxBand;
}

uint32_t
nanSchedGetAvailabilityAttr(struct ADAPTER *prAdapter,
			    struct _NAN_NDL_INSTANCE_T *prNDL,
			    uint8_t **ppucAvailabilityAttr,
			    uint32_t *pu4AvailabilityAttrLength)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = NULL;
	struct _NAN_ATTR_NAN_AVAILABILITY_T *prAvailAttr = NULL;
	struct _NAN_AVAILABILITY_ENTRY_T *prAvailEntry = NULL;
	uint32_t u4EntryIdx = 0;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline = NULL;
	uint32_t u4RetLength = 0;
	uint8_t ucType;
	uint32_t u2EntryLength;
	struct _NAN_SCHEDULER_T *prScheduler = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucOpRxNss = 1;
	union _NAN_BAND_CHNL_CTRL *prChnlInfo;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	uint8_t *pucPosEnd = NULL;
	size_t szNanIERemainLen = NAN_IE_BUF_MAX_SIZE;
	size_t szAvailAttrHeaderSize = 0, szAvailEntryHeaderSize = 0;
	size_t szMaxTimeBitmapFieldSize = 0, szMaxChnlEntryListSize = 0;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand;
	uint8_t fgNanUseR4AvailAttrBackup = g_fgNanUseR4AvailAttr;

	prScheduler = nanGetScheduler(prAdapter);
	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	if (ppucAvailabilityAttr)
		*ppucAvailabilityAttr = NULL;

	prAvailAttr = (struct _NAN_ATTR_NAN_AVAILABILITY_T *)g_aucNanIEBuffer;
	kalMemZero(g_aucNanIEBuffer, sizeof(g_aucNanIEBuffer));
	pucPos = (uint8_t *)prAvailAttr;
	pucPosEnd = pucPos + sizeof(g_aucNanIEBuffer);

	szAvailAttrHeaderSize =
		OFFSET_OF(struct _NAN_ATTR_NAN_AVAILABILITY_T,
		aucAvailabilityEntryList);
	szAvailEntryHeaderSize =
		OFFSET_OF(struct _NAN_AVAILABILITY_ENTRY_T,
		aucTimeBitmapAndBandChnlEntry);
	szMaxTimeBitmapFieldSize = NAN_TIME_BITMAP_FIELD_MAX_LENGTH;
	szMaxChnlEntryListSize = OFFSET_OF(struct _NAN_BAND_CHNL_LIST_T,
				aucEntry)
		+ 15 /* max number of chnl entries */
		* sizeof(struct _NAN_CHNL_ENTRY_T);


	if (prNDL) {
		prPeerSchDesc = nanSchedAcquirePeerSchDescByNmi(prAdapter,
							prNDL->aucPeerMacAddr);
	}


	if (prPeerSchDesc &&
	    prPeerSchDesc->ePeerForceAvailAttr != NAN_PEER_AVAIL_FORCE_NONE) {
		g_fgNanUseR4AvailAttr = prPeerSchDesc->ePeerForceAvailAttr -
					NAN_PEER_AVAIL_FORCE_R3;
		DBGLOG(NAN, INFO, "Force Use R4 Availability=%u",
		       g_fgNanUseR4AvailAttr);
	}

	eHighestCommonBand = nanSchedGetHighestCommonBand(prAdapter,
							  prNegoCtrl->u4SchIdx,
							  TRUE);
	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK &&
			NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx)) {
			union _NAN_BAND_CHNL_CTRL rP2pChnlInfo;

			if (szNanActiveTimelineNum >= 2 &&
			    !(prScheduler->fgEn5gL || prScheduler->fgEn5gH)) {
				DBGLOG(NAN, INFO,
				       "Skip 5G/6G availability by local capability");
				continue;
			}

			if (getPeerSchDescMaxCap(prPeerSchDesc) == BAND_2G4 &&
				szNanActiveTimelineNum >= 2) {
				/* do not skip when single map*/
				DBGLOG(NAN, INFO,
				       "Skip 5G/6G availability by peer capability");
				continue;
			}

			if (NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G)) {
				DBGLOG(NAN, INFO,
				       "Skip 5G/6G availability for local concurrent MCC");
				continue;
			}

			if (prNDL &&
				prNDL->eNDLRole == NAN_PROTOCOL_RESPONDER &&
				eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
				szNanActiveTimelineNum >= 2) {
				/* do no skip when single map */
				DBGLOG(NAN, INFO,
				       "Skip 5G/6G availability to use 2G");
				continue;
			}

			rP2pChnlInfo = nanGetActiveChnl(prAdapter,
							NETWORK_TYPE_P2P,
							BAND_5G);
			if (rP2pChnlInfo.u4PrimaryChnl &&
			    !NAN_CHNL_IN_COMMON_BAND(prAdapter, prNegoCtrl,
						     rP2pChnlInfo)) {
				DBGLOG(NAN, INFO,
				       "Skip 5G/6G availability since P2P channel %u not in common band",
				       rP2pChnlInfo.u4PrimaryChnl);
				continue;
			}
		}

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
					szTimeLineIdx);

		if (prNanTimelineMgmt->ucMapId == NAN_INVALID_MAP_ID) {
			DBGLOG(NAN, DEBUG, "Skip invalid map, Tidx=%zu\n",
				szTimeLineIdx);
			continue;
		}

		if (!(prAdapter->rWifiVar.ucNanMapMask & BIT(szTimeLineIdx))) {
			DBGLOG(NAN, DEBUG,
				"Skip Tidx=%zu, NanMapMask=%u\n",
				szTimeLineIdx,
				prAdapter->rWifiVar.ucNanMapMask);
			continue;
		}

		prNanSpecificBssInfo =
			nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				prNanSpecificBssInfo->ucBssIndex);

		if (prBssInfo == NULL)
			DBGLOG(NAN, ERROR, "NULL prBssInfo, idx=%d\n",
				prNanSpecificBssInfo->ucBssIndex);
		else
			ucOpRxNss = prBssInfo->ucOpRxNss;

		if (szAvailAttrHeaderSize > szNanIERemainLen) {
			DBGLOG(NAN, ERROR, "[%s] fail reason: 0x%08x\n",
				__func__, WLAN_STATUS_BUFFER_TOO_SHORT);
			*pu4AvailabilityAttrLength = 0U;
			u4Status = WLAN_STATUS_BUFFER_TOO_SHORT;
			goto end;
		}
		prAvailAttr->ucAttrId = NAN_ATTR_ID_NAN_AVAILABILITY;
		prAvailAttr->u2AttributeControl =
			prScheduler->u2NanCurrAvailAttrControlField |
			(prNanTimelineMgmt->ucMapId & NAN_AVAIL_CTRL_MAPID);
		prAvailAttr->ucSeqID = prScheduler->ucNanAvailAttrSeqId;
		prAvailAttr->u2Length = 3; /*Sequence ID + Attriburte Control*/
		szNanIERemainLen -= (size_t)(prAvailAttr->u2Length +
			3 /* attr ID(1) + attr length(2) */);

		pucPos = prAvailAttr->aucAvailabilityEntryList;

		/* Commit availability type */
		for (u4EntryIdx = 0,
			prChnlTimeline = &prNanTimelineMgmt->arChnlList[0];
			u4EntryIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
			u4EntryIdx++, prChnlTimeline++) {

			if (prChnlTimeline->fgValid == FALSE)
				continue;

			if ((szAvailEntryHeaderSize +
				szMaxTimeBitmapFieldSize +
				szMaxChnlEntryListSize) > szNanIERemainLen) {
				DBGLOG(NAN, ERROR,
					"[%s][%d] fail reason: 0x%08x\n",
					__func__, __LINE__,
					WLAN_STATUS_BUFFER_TOO_SHORT);
				*pu4AvailabilityAttrLength = 0U;
				u4Status = WLAN_STATUS_BUFFER_TOO_SHORT;
				goto end;
			}

			prChnlInfo = &prChnlTimeline->rChnlInfo;
			/* Do not add slots for AIS in 6G if NAN 6G disabled */
			if (IS_6G_OP_CLASS(prChnlInfo->u4OperatingClass)) {
#if (CFG_SUPPORT_NAN_6G == 0) && (CFG_SUPPORT_WIFI_6G == 1)
				DBGLOG(NAN, DEBUG,
				       "Skip adding committed oc=%u, ch=%u since NAN 6G not supported\n",
				       prChnlInfo->u4OperatingClass,
				       prChnlInfo->u4PrimaryChnl);
				continue;

				if (prPeerSchDesc &&
				    getPeerSchDescMaxCap(prPeerSchDesc) !=
				    BAND_6G) {
					DBGLOG(NAN, DEBUG,
					       "Skip sch=%u adding committed oc=%u, ch=%u since peer does not support 6G\n",
					       prPeerSchDesc->u4SchIdx,
					       prChnlInfo->u4OperatingClass,
					       prChnlInfo->u4PrimaryChnl);
					continue;
				}
#endif
			}

			prAvailEntry =
				(struct _NAN_AVAILABILITY_ENTRY_T *)pucPos;

			prAvailEntry->u2EntryControl =
			setEntryControl(NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT,
					0, 0, ucOpRxNss, 1);
			u2EntryLength = 2;

			pucPos += 4 /* length(2)+entry control(2) */;

			nanParserGenTimeBitmapField(prAdapter,
				prChnlTimeline->au4AvailMap, pucPos,
				&u4RetLength);

#if MERGE_POTENTIAL
			if (prPeerSchDesc &&
			    NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx) &&
			    pucPos[2] == TYPICAL_BITMAP_LENGTH &&
			    prPeerSchDesc->u4MergedCommittedChannel ==
				    prChnlInfo->u4PrimaryChnl &&
			    *(uint32_t *)prPeerSchDesc->aucPotMergedBitmap) {
				int i;

				DBGLOG(NAN, INFO,
				       "Masking ch=%u %02x-%02x-%02x-%02x => %02x-%02x-%02x-%02x\n",
				       prChnlInfo->u4PrimaryChnl,
				       pucPos[3], pucPos[4],
				       pucPos[5], pucPos[6],
				       pucPos[3] &
				       prPeerSchDesc->aucPotMergedBitmap[0],
				       pucPos[4] &
				       prPeerSchDesc->aucPotMergedBitmap[1],
				       pucPos[5] &
				       prPeerSchDesc->aucPotMergedBitmap[2],
				       pucPos[6] &
				       prPeerSchDesc->aucPotMergedBitmap[3]);
				for (i = 0; i < TYPICAL_BITMAP_LENGTH; i++)
					pucPos[3 + i] &=
					   prPeerSchDesc->aucPotMergedBitmap[i];
			}
#endif
			u2EntryLength += u4RetLength;
			pucPos += u4RetLength;

			nanParserGenBandChnlEntryListField(prAdapter,
				prChnlInfo, 1, pucPos, &u4RetLength);
			u2EntryLength += u4RetLength;
			pucPos += u4RetLength;

			prAvailEntry->u2Length = u2EntryLength;
			prAvailAttr->u2Length +=
				(u2EntryLength + 2 /* length(2) */);
			szNanIERemainLen -=
				(u2EntryLength + 2 /* length(2) */);
		}

		/* Conditional availability type */
		if (prNanTimelineMgmt->fgChkCondAvailability == TRUE) {
			for (u4EntryIdx = 0,
				prChnlTimeline =
				&prNanTimelineMgmt->arCondChnlList[0];
				u4EntryIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
				u4EntryIdx++, prChnlTimeline++) {

				if (prChnlTimeline->fgValid == FALSE)
					continue;

				if ((szAvailEntryHeaderSize +
					szMaxTimeBitmapFieldSize +
					szMaxChnlEntryListSize) >
					szNanIERemainLen) {
					DBGLOG(NAN, ERROR,
						"[%s][%d] fail reason: 0x%08x\n",
						__func__, __LINE__,
						WLAN_STATUS_BUFFER_TOO_SHORT);
					*pu4AvailabilityAttrLength = 0U;
					u4Status = WLAN_STATUS_BUFFER_TOO_SHORT;
					goto end;
				}

				prChnlInfo = &prChnlTimeline->rChnlInfo;

#if (CFG_SUPPORT_NAN_6G == 0)/* && (CFG_SUPPORT_WIFI_6G == 1)*/
				/* Do not add slots for AIS in 6G if
				 * NAN 6G disabled
				 */
				if (IS_6G_OP_CLASS(
				    prChnlInfo->u4OperatingClass)) {
					DBGLOG(NAN, DEBUG,
					       "Skip adding conditional oc=%u, ch=%u since NAN 6G not supported\n",
					       prChnlInfo->u4OperatingClass,
					       prChnlInfo->u4PrimaryChnl);
					continue;
				}
#endif

				prAvailEntry =
				(struct _NAN_AVAILABILITY_ENTRY_T *)pucPos;

#if CFG_NAN_SIGMA_TEST
				if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING)
					ucType =
					NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT;
				else
					ucType =
					NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND;

#else
				ucType = NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COND;
#endif
				prAvailEntry->u2EntryControl =
					setEntryControl(ucType, 0, 0,
							ucOpRxNss, 1);
				u2EntryLength = 2;

				pucPos += 4 /* length(2)+entry control(2) */;

				nanParserGenTimeBitmapField(prAdapter,
					prChnlTimeline->au4AvailMap,
					pucPos, &u4RetLength);
				u2EntryLength += u4RetLength;
				pucPos += u4RetLength;

				nanParserGenBandChnlEntryListField(prAdapter,
						prChnlInfo, 1, pucPos,
						&u4RetLength);
				u2EntryLength += u4RetLength;
				pucPos += u4RetLength;
				prAvailEntry->u2Length = u2EntryLength;
				prAvailAttr->u2Length +=
					(u2EntryLength + 2 /* length(2) */);
				szNanIERemainLen -=
					(u2EntryLength + 2 /* length(2) */);

				/* set committed change
				 * only when conditional
				 * slot is included
				 */
				prAvailAttr->u2AttributeControl |=
					NAN_AVAIL_CTRL_COMMIT_CHANGED;
			}
		}

		/* add potential availability */
		u4RetLength = nanSchedAddPotentialWindows(prAdapter, pucPos,
			szTimeLineIdx);
		pucPos += u4RetLength;
		prAvailAttr->u2Length += u4RetLength;
		prAvailAttr = (struct _NAN_ATTR_NAN_AVAILABILITY_T *)pucPos;
	}

	if (ppucAvailabilityAttr)
		*ppucAvailabilityAttr = g_aucNanIEBuffer;
	if (pu4AvailabilityAttrLength)
		*pu4AvailabilityAttrLength = (pucPos - g_aucNanIEBuffer);

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
	/* nanUtilDump(prAdapter, "Avail Attr",
	 *		g_aucNanIEBuffer, (pucPos-g_aucNanIEBuffer));
	 */

end:
	g_fgNanUseR4AvailAttr = fgNanUseR4AvailAttrBackup;
	return u4Status;
}

uint32_t
nanSchedGetDevCapabilityAttr(struct ADAPTER *prAdapter,
		uint8_t **ppucDevCapAttr,
		uint32_t *pu4DevCapAttrLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_ATTR_DEVICE_CAPABILITY_T *prAttrDevCap;
	struct _NAN_SCHEDULER_T *prScheduler;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo;
	struct BSS_INFO *prBssInfo;
	uint8_t ucOpRxNss = 1;

	prScheduler = nanGetScheduler(prAdapter);
	prAttrDevCap = &prScheduler->rAttrDevCap;

	if (prScheduler->fgAttrDevCapValid) {
		if (ppucDevCapAttr)
			*ppucDevCapAttr = (uint8_t *)prAttrDevCap;
		if (pu4DevCapAttrLength)
			*pu4DevCapAttrLength =
				sizeof(struct _NAN_ATTR_DEVICE_CAPABILITY_T);

		return WLAN_STATUS_SUCCESS;
	}

	prAttrDevCap->ucAttrId = NAN_ATTR_ID_DEVICE_CAPABILITY;
	prAttrDevCap->u2Length = sizeof(struct _NAN_ATTR_DEVICE_CAPABILITY_T) -
				 3 /* ID(1)+Length(2) */;
	prAttrDevCap->ucMapID = 0; /* applied to all MAP */

	prAttrDevCap->u2CommittedDWInfo = 0;
	prAttrDevCap->u2CommittedDw2g = prScheduler->ucCommitDwInterval;
	if (prScheduler->fgEn5gL || prScheduler->fgEn5gH)
		prAttrDevCap->u2CommittedDw5g = prScheduler->ucCommitDwInterval;

	prAttrDevCap->ucSupportedBands = 0;
	if (prScheduler->fgEn2g)
		prAttrDevCap->ucSupportedBands |=
			BIT(NAN_SUPPORTED_BAND_ID_2P4G);
	if (prScheduler->fgEn5gL || prScheduler->fgEn5gH)
		prAttrDevCap->ucSupportedBands |= BIT(NAN_SUPPORTED_BAND_ID_5G);
#if (CFG_SUPPORT_NAN_6G == 1)
	if (prScheduler->fgEn6g) {
		DBGLOG(NAN, TRACE, "UseR4Cap=%u\n",
		       prAdapter->rWifiVar.b1NanUseR4Cap6GBit);

		if (prAdapter->rWifiVar.b1NanUseR4Cap6GBit)
			prAttrDevCap->ucSupportedBands |= NAN_SUPPORTED_6G_BIT;
		else
			prAttrDevCap->ucSupportedBands |=
				NAN_PROPRIETARY_6G_BIT;
	}
#endif

	/* Support VHT Mode */
	prAttrDevCap->ucOperPhyModeVht = 1;
#if (CFG_SUPPORT_802_11AX == 1)
	prAttrDevCap->ucOperPhyModeHe = 1;
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prScheduler->fgEn6g)
		prAttrDevCap->ucOperPhyModeBw_160 = 1;
#else
	if (prAdapter->rWifiVar.ucNan5gBandwidth == MAX_BW_160MHZ)
		/* only support HE/VHT 160 when dbdc off */
		if (prAdapter->rWifiVar.fgDbDcModeEn == FALSE)
			prAttrDevCap->ucOperPhyModeBw_160 = 1;
#endif
#endif

	prNanSpecificBssInfo =
		nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prNanSpecificBssInfo->ucBssIndex);

	if (prBssInfo == NULL)
		DBGLOG(NAN, ERROR, "NULL prBssInfo, idx=%d\n",
			prNanSpecificBssInfo->ucBssIndex);
	else {
		DBGLOG(NAN, DEBUG, "Bss idx:%d, Nss:%d\n",
			prBssInfo->ucBssIndex, prBssInfo->ucOpRxNss);
		ucOpRxNss = prBssInfo->ucOpRxNss;
	}

	prAttrDevCap->ucNumOfAntennasTx = ucOpRxNss;
	prAttrDevCap->ucNumOfAntennasRx = ucOpRxNss;

	/* TODO: use value reported from firmware */
	prAttrDevCap->u2MaxChannelSwitchTime = g_u4MaxChnlSwitchTimeUs;

	prAttrDevCap->ucCapabilities = 0;
	if (prAdapter->rWifiVar.fgEnableNDPE)
		prAttrDevCap->ucCapabilities |=
			NAN_ATTR_DEVICE_CAPABILITY_CAP_SUPPORT_NDPE;

	if (ppucDevCapAttr)
		*ppucDevCapAttr = (uint8_t *)prAttrDevCap;
	if (pu4DevCapAttrLength)
		*pu4DevCapAttrLength =
			sizeof(struct _NAN_ATTR_DEVICE_CAPABILITY_T);

	prScheduler->fgAttrDevCapValid = TRUE;

	return rRetStatus;
}

#if (CFG_SUPPORT_NAN_6G == 1)
uint32_t
nanSchedGetDevCapabilityExtAttr(struct ADAPTER *prAdapter,
		uint8_t **ppucDevCapExtAttr,
		uint32_t *pu4DevCapExtAttrLength)
{
	uint8_t *pucPos;
	struct _NAN_ATTR_DEVICE_CAPABILITY_EXT_T *prAttrDevCapExt;
	struct _NAN_ATTR_EXT_CAPABILITIES_T *prExtCap;
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);

	prAttrDevCapExt =
		(struct _NAN_ATTR_DEVICE_CAPABILITY_EXT_T *)
		g_aucNanIEBuffer;
	kalMemZero(g_aucNanIEBuffer, sizeof(g_aucNanIEBuffer));

	prAttrDevCapExt->ucAttrId = NAN_ATTR_ID_DEVICE_CAPABILITY_EXT;
	prAttrDevCapExt->u2Length = 0;

	pucPos = prAttrDevCapExt->aucExtCapabilities;

	prExtCap = (struct _NAN_ATTR_EXT_CAPABILITIES_T *)pucPos;

	if (prScheduler->fgEn6g) {
		prExtCap->ucRegulatoryInfo |=
			NAN_DEVICE_CAPABILITY_EXT_REG_INFO_EN;
		prExtCap->ucRegulatoryInfo |=
			(2 << NAN_DEVICE_CAPABILITY_EXT_REG_INFO_OFFSET);
	}
	prExtCap->ucRegulatoryInfo =
		(prExtCap->ucRegulatoryInfo &
		NAN_DEVICE_CAPABILITY_EXT_REG_INFO_EN)
		| (prExtCap->ucRegulatoryInfo &
		NAN_DEVICE_CAPABILITY_EXT_REG_INFO);

	prExtCap->ucSettings =
		(prExtCap->ucSettings &
		(NAN_DEVICE_CAPABILITY_EXT_PARING_SETUP_EN >>
		NAN_DEVICE_CAPABILITY_EXT_SETTING_OFFSET))
		|
		(prExtCap->ucSettings &
		(NAN_DEVICE_CAPABILITY_EXT_NPK_NIK_CACHE_EN >>
		NAN_DEVICE_CAPABILITY_EXT_SETTING_OFFSET));

	pucPos += 2; /* RegulatoryInfo(1) + Settings(1) */
	prAttrDevCapExt->u2Length += 2;

	if (ppucDevCapExtAttr)
		*ppucDevCapExtAttr = g_aucNanIEBuffer;
	if (pu4DevCapExtAttrLength)
		*pu4DevCapExtAttrLength = (pucPos - g_aucNanIEBuffer);

	/* nanUtilDump(prAdapter, "Avail Attr",
	 *		g_aucNanIEBuffer, (pucPos-g_aucNanIEBuffer));
	 */

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t
nanSchedGetUnalignedScheduleAttr(struct ADAPTER *prAdapter,
				 uint8_t **ppucUnalignedScheduleAttr,
				 uint32_t *pu4UnalignedScheduleLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = g_aucNanIEBuffer;

	if (ppucUnalignedScheduleAttr)
		*ppucUnalignedScheduleAttr = g_aucNanIEBuffer;
	if (pu4UnalignedScheduleLength)
		*pu4UnalignedScheduleLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

uint32_t
nanSchedGetExtWlanInfraAttr(struct ADAPTER *prAdapter,
			    uint8_t **ppucExtWlanInfraAttr,
			    uint32_t *pu4ExtWlanInfraLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = g_aucNanIEBuffer;

	if (ppucExtWlanInfraAttr)
		*ppucExtWlanInfraAttr = g_aucNanIEBuffer;
	if (pu4ExtWlanInfraLength)
		*pu4ExtWlanInfraLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

uint32_t
nanSchedGetPublicAvailAttr(struct ADAPTER *prAdapter,
			   uint8_t **ppucPublicAvailAttr,
			   uint32_t *pu4PublicAvailLength)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint8_t *pucPos = g_aucNanIEBuffer;

	if (ppucPublicAvailAttr)
		*ppucPublicAvailAttr = g_aucNanIEBuffer;
	if (pu4PublicAvailLength)
		*pu4PublicAvailLength = (pucPos - g_aucNanIEBuffer);

	return rRetStatus;
}

uint32_t nanSchedCmdUpdatePotentialChnlList(struct ADAPTER *prAdapter)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_UPDATE_PONTENTIAL_CHNL_LIST_T
		*prCmdUpdatePontentialChnlList = NULL;
	struct _NAN_POTENTIAL_CHNL_MAP_T *prPotentialChnl;
	struct _NAN_POTENTIAL_CHNL_T *prPotentialChnlList;
	uint32_t u4Idx, u4Num;
	enum ENUM_BAND eBand;
	enum _NAN_CHNL_BW_MAP eBw;
	struct _NAN_SCHEDULER_T *prNanScheduler;
	union _NAN_BAND_CHNL_CTRL rFixChnl;
	struct _NAN_CHNL_ENTRY_T rChnlEntry;
	size_t szTimeLineIdx;
	uint32_t u4SuppBandIdMask = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	uint8_t ucPrimaryChnl;
	struct _NAN_CHNL_ENTRY_T *prSchedulerPotentialChannel;

	prNanScheduler = nanGetScheduler(prAdapter);

	u4CmdBufferLen =
		sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
		sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
		sizeof(struct _NAN_SCHED_CMD_UPDATE_PONTENTIAL_CHNL_LIST_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_POTENTIAL_CHNL_LIST,
		sizeof(struct _NAN_SCHED_CMD_UPDATE_PONTENTIAL_CHNL_LIST_T),
		u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						 prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdUpdatePontentialChnlList =
		(struct _NAN_SCHED_CMD_UPDATE_PONTENTIAL_CHNL_LIST_T *)
			prTlvElement->aucbody;
	kalMemZero(prCmdUpdatePontentialChnlList,
		   sizeof(*prCmdUpdatePontentialChnlList));

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		u4Num = 0;
		prPotentialChnlList =
		      prCmdUpdatePontentialChnlList->aarChnlList[szTimeLineIdx];
		u4SuppBandIdMask =
		      nanGetTimelineSupportedBand(prAdapter, szTimeLineIdx);

		for (prPotentialChnl = g_arPotentialChnlMap;
		     prPotentialChnl->ucPrimaryChnl != 0;
		     prPotentialChnl++) {
			ucPrimaryChnl = prPotentialChnl->ucPrimaryChnl;

			eBw = ucPrimaryChnl < 36 ?
				nanSchedGet2gNanBw(prAdapter) :
				nanSchedGet5gNanBw(prAdapter);
			/* NAN 2G BW check*/
			if ((ucPrimaryChnl < 36 ||
			     !prAdapter->rWifiVar.fgEnNanVHT) &&
			    eBw > NAN_CHNL_BW_40)
				eBw = NAN_CHNL_BW_40;

			if (ucPrimaryChnl < 36) {
				if (!prNanScheduler->fgEn2g)
					continue;

				eBand = BAND_2G4;
			} else if (ucPrimaryChnl < 100) {
				if (!prNanScheduler->fgEn5gL)
					continue;

				eBand = BAND_5G;
			} else {
				if (!prNanScheduler->fgEn5gH)
					continue;

				eBand = BAND_5G;
			}

			if ((u4SuppBandIdMask & BIT(eBand)) == 0)
				continue;

			while (eBw > NAN_CHNL_BW_20 &&
			       prPotentialChnl->aucOperatingClass[eBw] == 0)
				eBw--;

			if (!rlmDomainIsLegalChannel(prAdapter, eBand,
						     ucPrimaryChnl))
				continue;

			/* search the current potential channel list for the
			 * same group
			 */
			for (u4Idx = 0; u4Idx < u4Num; u4Idx++) {
				if (prPotentialChnl->aucOperatingClass[eBw] ==
				    prPotentialChnlList[u4Idx].ucOpClass &&
				    prPotentialChnl->aucPriChnlMapIdx[eBw] ==
				    prPotentialChnlList[u4Idx].ucPriChnlBitmap)
					break;
			}

			if (u4Idx == u4Num &&
			    u4Num < NAN_MAX_POTENTIAL_CHNL_LIST) {
				u4Num++;

				prPotentialChnlList[u4Idx].ucOpClass =
					prPotentialChnl->aucOperatingClass[eBw];
				prPotentialChnlList[u4Idx].ucPriChnlBitmap =
					prPotentialChnl->aucPriChnlMapIdx[eBw];
			}

			if (u4Idx < u4Num) {
				prPotentialChnlList[u4Idx].u2ChnlBitmap |=
					prPotentialChnl->au2ChnlMapIdx[eBw];
			}
		}

#if (CFG_SUPPORT_NAN_6G == 1)
		if (prNanScheduler->fgEn6g &&
		    NAN_IS_6G_TIMELINE(prAdapter, szTimeLineIdx) &&
		    !(nanSchedGetConnBands(prAdapter, NETWORK_TYPE_P2P) &
		      BIT(BAND_5G))) {
			/* Generate 6G potential channel list */
			eBw = nanSchedGet6gNanBw(prAdapter);
			/* To limit channel entry num, only bring 6G BW > 40 */
			u4Idx = NAN_CHNL_BW_80;
			for (u4Idx = eBw; u4Idx >= NAN_CHNL_BW_80; u4Idx--) {
				if (u4Num < NAN_MAX_POTENTIAL_CHNL_LIST) {
					prPotentialChnlList[u4Num] =
						g_ar6gPotentialChnlMap[u4Idx];
					u4Num++;
				}
			}
		}
#endif

		prCmdUpdatePontentialChnlList->au4Num[szTimeLineIdx] = u4Num;

		rFixChnl = nanSchedGetFixedChnlInfo(prAdapter);

		if (rFixChnl.u4PrimaryChnl != 0) {
			nanParserGenChnlEntryField(prAdapter, &rFixChnl,
					&rChnlEntry);
			u4Num = 1;
			prCmdUpdatePontentialChnlList->au4Num[szTimeLineIdx]
				= u4Num;

			prPotentialChnlList->ucOpClass =
				rFixChnl.u4OperatingClass;
			prPotentialChnlList->ucPriChnlBitmap =
				rChnlEntry.ucPrimaryChnlBitmap;
			prPotentialChnlList->u2ChnlBitmap =
				rChnlEntry.u2ChannelBitmap;
		}

		for (u4Idx = 0; u4Idx < u4Num; u4Idx++) {
			DBGLOG(NAN, DEBUG,
			     "[%zu][%d] OpClass:%d, PriChnlBitmap:0x%x, ChnlBitmap:0x%04x, Bw:%d\n",
			     szTimeLineIdx, u4Idx,
			     prPotentialChnlList[u4Idx].ucOpClass,
			     prPotentialChnlList[u4Idx].ucPriChnlBitmap,
			     prPotentialChnlList[u4Idx].u2ChnlBitmap,
			     nanRegGetBw(prPotentialChnlList[u4Idx].ucOpClass));
		}

		prNanScheduler->au4NumOfPotentialChnlList[szTimeLineIdx] =
			u4Num;

		prSchedulerPotentialChannel =
			prNanScheduler->aarPotentialChnlList[szTimeLineIdx];
		for (u4Idx = 0; u4Idx < u4Num; u4Idx++) {
			prSchedulerPotentialChannel[u4Idx].ucOperatingClass =
				prPotentialChnlList[u4Idx].ucOpClass;
			prSchedulerPotentialChannel[u4Idx].u2ChannelBitmap =
				prPotentialChnlList[u4Idx].u2ChnlBitmap;
			prSchedulerPotentialChannel[u4Idx].ucPrimaryChnlBitmap =
				prPotentialChnlList[u4Idx].ucPriChnlBitmap;
		}
	}
	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t nanSchedTransferFawAvailMap(uint32_t bitmap)
{
	uint32_t u4Result = 0;
	uint8_t ucInSequence = 0;
	uint8_t ucFirstOne = 32; // Initialize to an invalid position
	uint8_t ucLastOne = 32;  // Initialize to an invalid position
	uint8_t ucBitmapIdx = 0;

	for (ucBitmapIdx = 0; ucBitmapIdx < 32; ucBitmapIdx++) {
		if ((bitmap & (1 << ucBitmapIdx)) != 0) {
			if (!ucInSequence) {
				ucInSequence = 1;
				ucFirstOne = ucBitmapIdx;
			}
			ucLastOne = ucBitmapIdx;
		} else {
			if (ucInSequence) {
				ucInSequence = 0;
				if (ucFirstOne != 32) {
					u4Result |= (1 << ucFirstOne);

					if (ucLastOne + 1 < 32)
						u4Result |=
							(1 << (ucLastOne + 1));
				}
				ucFirstOne = 32;
				ucLastOne = 32;
			}
		}
	}

	if (ucInSequence && ucFirstOne != 32) {
		u4Result |= (1 << ucFirstOne);

		if (ucLastOne + 1 < 32)
			u4Result |= (1 << (ucLastOne + 1));
	}
	if ((bitmap & 0x80000000) != 0)
		u4Result |= 0x80000000;

	return u4Result;
}

uint32_t nanSchedTransferNdcAvailMap(uint32_t u4Bitmap)
{
	uint32_t u4Result = 0;
	uint8_t ucShift = 0;
	uint8_t ucBitmapIdx = 0;

	for (ucBitmapIdx = 0; ucBitmapIdx < 32; ucBitmapIdx++) {
		if (u4Bitmap & (1 << ucBitmapIdx)) {

			u4Result |= (1 << (ucBitmapIdx + ucShift));

			if (ucBitmapIdx + ucShift + 1 < 32)
				u4Result |= (1 << (ucBitmapIdx + ucShift + 1));

			ucShift++;
		} else {
			u4Result |= (u4Bitmap & (1 << ucBitmapIdx)) << ucShift;
		}
	}

	return u4Result;
}


uint32_t
nanSchedGetTimelineByBand(struct ADAPTER *prAdapter, enum ENUM_BAND eBand)
{
	uint8_t ucTimelineIdx = 0;
	struct _NAN_SCHEDULER_T *prScheduler;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec = NULL;
	uint32_t u4SchIdx = 0;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline = NULL;
	struct _NAN_FAW_NDC_TIMELINE_T *prNanFawNdcTimeline = NULL;
	uint32_t u4EntryIdx = 0;
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;

	ucTimelineIdx = nanGetTimelineMgmtIndexByBand(prAdapter, eBand);
	prScheduler = nanGetScheduler(prAdapter);
	prNanFawNdcTimeline = nanSchedGetFawNdcTimeline(ucTimelineIdx);
	kalMemZero(prNanFawNdcTimeline->au4AvailMap,
		sizeof(prNanFawNdcTimeline));

	if (((eBand == BAND_2G4) && prScheduler->fgEn2g) ||
		((eBand == BAND_5G) && (prScheduler->fgEn5gH ||
		prScheduler->fgEn5gL))) {
		for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
			prPeerSchRec =
				nanSchedGetPeerSchRecord(
					prAdapter, u4SchIdx);

			if ((prPeerSchRec == NULL) ||
				(prPeerSchRec->fgUseDataPath == FALSE))
				continue;

			prTimeline =
				&prPeerSchRec->arCommFawTimeline[ucTimelineIdx];

			/*
			 * Combine all FAW bitmap
			 * Ex. 1111111100000000 -> 1000000010000000
			 */
			for (u4EntryIdx = 0;
				u4EntryIdx < NAN_TOTAL_DW;
				u4EntryIdx++) {
				if (prTimeline->au4AvailMap[u4EntryIdx] == 0)
					continue;

				prNanFawNdcTimeline->au4AvailMap[u4EntryIdx] |=
					nanSchedTransferFawAvailMap(
					prTimeline->au4AvailMap[u4EntryIdx]);
			}

			prNdcCtrl = prPeerSchRec->prCommNdcCtrl;

			if (!prNdcCtrl || !prNdcCtrl->fgValid)
				continue;

			prTimeline =
				&prNdcCtrl->arTimeline[ucTimelineIdx];

			/*
			 * Combine all NDC bitmap
			 * Ex. 0000100000000000  -> 0000110000000000
			 */
			for (u4EntryIdx = 0;
				u4EntryIdx < NAN_TOTAL_DW;
				u4EntryIdx++) {
				if (prTimeline->au4AvailMap[u4EntryIdx] == 0)
					continue;

				prNanFawNdcTimeline->au4AvailMap[u4EntryIdx] |=
					nanSchedTransferNdcAvailMap(
					prTimeline->au4AvailMap[u4EntryIdx]);
			}
		}

		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}


uint32_t
nanSchedCmdUpdateCRB(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_UPDATE_CRB_T *prCmdUpdateCRB = NULL;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_FAW_NDC_TIMELINE_T *prNanFawNdcTimeline = NULL;
	uint8_t ucIdx = 0;
	size_t i;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (!prPeerSchRecord || prPeerSchRecord->fgActive == FALSE)
		return WLAN_STATUS_FAILURE;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_UPDATE_CRB_T);
	DBGLOG(NAN, TRACE, "allocate %u bytes for %zu\n",
	       u4CmdBufferLen, sizeof(struct _NAN_SCHED_CMD_UPDATE_CRB_T));
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_CRB,
				sizeof(struct _NAN_SCHED_CMD_UPDATE_CRB_T),
				u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						 prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prPeerSchDesc = prPeerSchRecord->prPeerSchDesc;
	for (i = 0; i < ARRAY_SIZE(prPeerSchRecord->arCommFawTimeline); i++) {
		struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;

		prTimeline = &prPeerSchRecord->arCommFawTimeline[i];
		if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		DBGLOG(NAN, INFO,
		       "sch:%u, %zu, MapId=%u, avail=%02x-%02x-%02x-%02x\n",
		       u4SchIdx, i, prTimeline->ucMapId,
		       ((uint8_t *)(prTimeline->au4AvailMap))[0],
		       ((uint8_t *)(prTimeline->au4AvailMap))[1],
		       ((uint8_t *)(prTimeline->au4AvailMap))[2],
		       ((uint8_t *)(prTimeline->au4AvailMap))[3]);
	}
	DBGLOG(NAN, TRACE, "ePeerForceAvailAttr=%u",
	       prPeerSchDesc->ePeerForceAvailAttr);
	DBGLOG(NAN, TRACE, "element tag=%u, body_len=%u, copy %zu, sch=%u\n",
	       prTlvElement->tag_type, prTlvElement->body_len,
	       sizeof(struct _NAN_SCHED_CMD_UPDATE_CRB_T), u4SchIdx);
	DBGDUMP_HEX(NAN, TEMP, "prPeerSchRecord->arCommFawTimeline\n",
		    prPeerSchRecord->arCommFawTimeline,
		    sizeof(prPeerSchRecord->arCommFawTimeline));

	prCmdUpdateCRB =
		(struct _NAN_SCHED_CMD_UPDATE_CRB_T *)prTlvElement->aucbody;
	kalMemZero(prCmdUpdateCRB, sizeof(struct _NAN_SCHED_CMD_UPDATE_CRB_T));

	prCmdUpdateCRB->u4SchIdx = u4SchIdx;

	kalMemCopy(&prCmdUpdateCRB->arCommFawTimeline,
		   &prPeerSchRecord->arCommFawTimeline,
		   sizeof(prCmdUpdateCRB->arCommFawTimeline));

	if (prPeerSchRecord->fgUseDataPath) {
		prCmdUpdateCRB->fgUseDataPath = TRUE;
		if (prPeerSchDesc->ePeerForceAvailAttr !=
		    NAN_PEER_AVAIL_FORCE_NONE)
			prCmdUpdateCRB->b2Avail6GFormat =
				prPeerSchDesc->ePeerForceAvailAttr & BITS(0, 1);

		if (prPeerSchRecord->prCommNdcCtrl) {
			struct _NAN_NDC_CTRL_T *prCommNdcCtrl;
			struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;

			prCmdUpdateCRB->rCommNdcCtrl =
				*prPeerSchRecord->prCommNdcCtrl;

			prCommNdcCtrl = prPeerSchRecord->prCommNdcCtrl;
			prTimeline = prCommNdcCtrl->arTimeline;
			for (i = 0; i < ARRAY_SIZE(prCommNdcCtrl->arTimeline);
			     i++) {
				if (prTimeline[i].ucMapId == NAN_INVALID_MAP_ID)
					continue;

				DBGLOG(NAN, INFO,
				       "sch idx=%u, timeline=%zu, NDC=%02x:%02x:%02x:%02x:%02x:%02x MapID=%u, Avail=%02x-%02x-%02x-%02x",
				       u4SchIdx, i,
				       prCommNdcCtrl->aucNdcId[0],
				       prCommNdcCtrl->aucNdcId[1],
				       prCommNdcCtrl->aucNdcId[2],
				       prCommNdcCtrl->aucNdcId[3],
				       prCommNdcCtrl->aucNdcId[4],
				       prCommNdcCtrl->aucNdcId[5],
				       prTimeline[i].ucMapId,
				       prTimeline[i].aucAvailBlock[0],
				       prTimeline[i].aucAvailBlock[1],
				       prTimeline[i].aucAvailBlock[2],
				       prTimeline[i].aucAvailBlock[3]);
			}
		} else {
			prNdcCtrl = nanSchedGetNdcCtrl(prAdapter,
				prNegoCtrl->rSelectedNdcCtrl.aucNdcId);
			DBGLOG(NAN, WARN,
			       "prCommNdcCtrl is NULL, prNdcCtrl exist:%u\n",
			       prNdcCtrl != NULL);
		}
	}

	if (prPeerSchRecord->fgUseRanging) {
		prCmdUpdateCRB->fgUseRanging = TRUE;
		kalMemCopy(&prCmdUpdateCRB->arCommRangingTimeline,
			   &prPeerSchRecord->arCommRangingTimeline,
			   sizeof(prCmdUpdateCRB->arCommRangingTimeline));
	}

	nanSchedGetTimelineByBand(prAdapter, BAND_2G4);
	nanSchedGetTimelineByBand(prAdapter, BAND_5G);
	for (ucIdx = 0; ucIdx < NAN_TIMELINE_MGMT_SIZE; ucIdx++) {
		prNanFawNdcTimeline = nanSchedGetFawNdcTimeline(ucIdx);
		kalMemCopy(&prCmdUpdateCRB->arFawNdcTimeline[ucIdx],
			prNanFawNdcTimeline,
			sizeof(struct _NAN_FAW_NDC_TIMELINE_T));
		DBGLOG(NAN, DEBUG,
			"MapIdx=%u, Revised bitmap=%02x-%02x-%02x-%02x\n",
			ucIdx,
			((uint8_t *)prNanFawNdcTimeline->au4AvailMap)[0],
			((uint8_t *)prNanFawNdcTimeline->au4AvailMap)[1],
			((uint8_t *)prNanFawNdcTimeline->au4AvailMap)[2],
			((uint8_t *)prNanFawNdcTimeline->au4AvailMap)[3]);
	}
	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t
nanSchedCmdMapStaRecord(
	struct ADAPTER *prAdapter,
	uint8_t *pucNmiAddr,
	enum NAN_BSS_ROLE_INDEX eRoleIdx,
	uint8_t ucStaRecIdx,
	uint8_t ucNdpCxtId,
	uint8_t ucWlanIndex,
	uint8_t *pucNdiAddr)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_MAP_STA_REC_T *prCmdMapStaRec = NULL;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	uint8_t i = nanGetLinkIndexbyRole(eRoleIdx);

	prPeerSchRecord = nanSchedLookupPeerSchRecord(prAdapter, pucNmiAddr);
	if (!prPeerSchRecord)
		return WLAN_STATUS_FAILURE;

	if (i >= NAN_LINK_NUM)
		i = 0;

	prPeerSchRecord->aucStaRecIdx[i][ucNdpCxtId] = ucStaRecIdx;

	/* Both 2G and 5G/6G band are called in nanDataEngineEnrollNMIContext
	 * and nanDataEngineEnrollNDPContext, check the consistency here
	 */
#if (CFG_SUPPORT_NAN_DBDC == 1)
	if (ucStaRecIdx != STA_REC_INDEX_NOT_FOUND &&
	    (prPeerSchRecord->eBand == BAND_2G4 &&
		     eRoleIdx == NAN_BSS_INDEX_BAND1 ||
	     prPeerSchRecord->eBand != BAND_2G4 &&
		     eRoleIdx == NAN_BSS_INDEX_BAND0)) {
		DBGLOG(NAN, INFO,
		       "Skip set StaRec %u to firmware, prefer=%u, set band=%u NMI:%02x:%02x:%02x:%02x:%02x:%02x, eRoleIdx=%u, link=%u ucStaRecIdx=%u, ucNdpCxtId=%u, ucWlanIndex=%u, pucNdiAddr:%02x:%02x:%02x:%02x:%02x:%02x",
		       ucStaRecIdx, prPeerSchRecord->eBand, eRoleIdx,
		       pucNmiAddr[0], pucNmiAddr[1], pucNmiAddr[2],
		       pucNmiAddr[3], pucNmiAddr[4], pucNmiAddr[5],
		       eRoleIdx, i, ucStaRecIdx, ucNdpCxtId, ucWlanIndex,
		       pucNdiAddr[0], pucNdiAddr[1], pucNdiAddr[2],
		       pucNdiAddr[3], pucNdiAddr[4], pucNdiAddr[5]);
		return WLAN_STATUS_SUCCESS;
	}

	DBGLOG(NAN, INFO,
	       "Set StaRec %u to firmware, prefer=%u, set band=%u NMI:%02x:%02x:%02x:%02x:%02x:%02x, eRoleIdx=%u, link=%u ucStaRecIdx=%u, ucNdpCxtId=%u, ucWlanIndex=%u, pucNdiAddr:%02x:%02x:%02x:%02x:%02x:%02x",
	       ucStaRecIdx, prPeerSchRecord->eBand, eRoleIdx,
	       pucNmiAddr[0], pucNmiAddr[1], pucNmiAddr[2],
	       pucNmiAddr[3], pucNmiAddr[4], pucNmiAddr[5],
	       eRoleIdx, i, ucStaRecIdx, ucNdpCxtId, ucWlanIndex,
	       pucNdiAddr[0], pucNdiAddr[1], pucNdiAddr[2],
	       pucNdiAddr[3], pucNdiAddr[4], pucNdiAddr[5]);
#endif

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_MAP_STA_REC_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_MAP_STA_RECORD,
				    sizeof(struct _NAN_SCHED_CMD_MAP_STA_REC_T),
				    u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						 prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdMapStaRec =
		(struct _NAN_SCHED_CMD_MAP_STA_REC_T *)prTlvElement->aucbody;
	COPY_MAC_ADDR(prCmdMapStaRec->aucNmiAddr, pucNmiAddr);
	COPY_MAC_ADDR(prCmdMapStaRec->aucNdiAddr, pucNdiAddr);
	prCmdMapStaRec->eRoleIdx = eRoleIdx;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	prCmdMapStaRec->ucStaRecIdx = ucWlanIndex;
#else
	prCmdMapStaRec->ucStaRecIdx = ucStaRecIdx;
#endif
	prCmdMapStaRec->ucNdpCxtId = ucNdpCxtId;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t
nanSchedCmdUpdatePeerCapability(struct ADAPTER *prAdapter, uint32_t u4SchIdx)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_UPDATE_PEER_CAPABILITY_T *prCmdUpdatePeerCap =
		NULL;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint8_t ucSupportedBands, ucPeerSupportedBands;
	uint32_t u4Idx;
	struct _NAN_DEVICE_CAPABILITY_T *prDevCapList;
	uint16_t u2MaxChnlSwitchTime = 0;

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (!prPeerSchRecord || prPeerSchRecord->fgActive != TRUE ||
	    !prPeerSchRecord->prPeerSchDesc)
		return WLAN_STATUS_FAILURE;
	prPeerSchDesc = prPeerSchRecord->prPeerSchDesc;
	if (!prPeerSchDesc) {
		DBGLOG(NAN, ERROR, "prPeerSchDesc Null\n");
		return WLAN_STATUS_FAILURE;
	}

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_UPDATE_PEER_CAPABILITY_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_PEER_CAPABILITY,
			sizeof(struct _NAN_SCHED_CMD_UPDATE_PEER_CAPABILITY_T),
			u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						 prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdUpdatePeerCap = (struct _NAN_SCHED_CMD_UPDATE_PEER_CAPABILITY_T *)
				     prTlvElement->aucbody;
	ucSupportedBands = BIT(NAN_SUPPORTED_BAND_ID_2P4G);
	prDevCapList = prPeerSchRecord->prPeerSchDesc->arDevCapability;
	for (u4Idx = 0; u4Idx < (NAN_NUM_AVAIL_DB + 1);
		u4Idx++, prDevCapList++) {
		if (prDevCapList->fgValid) {
			ucSupportedBands |= prDevCapList->ucSupportedBand;
			u2MaxChnlSwitchTime =
				(prDevCapList->u2MaxChnlSwitchTime
				>= u2MaxChnlSwitchTime) ?
				prDevCapList->u2MaxChnlSwitchTime :
				u2MaxChnlSwitchTime;
		}
	}

	ucPeerSupportedBands = prPeerSchDesc->u4PeerSupportedBand;
	prCmdUpdatePeerCap->u4SchIdx = u4SchIdx;
	prCmdUpdatePeerCap->ucSupportedBands = ucSupportedBands;
	prCmdUpdatePeerCap->ucPeerSupportedBands = ucPeerSupportedBands;
	prCmdUpdatePeerCap->u2MaxChnlSwitchTime = u2MaxChnlSwitchTime;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t
nanSchedCmdManagePeerSchRecord(struct ADAPTER *prAdapter, uint32_t u4SchIdx,
			       unsigned char fgActivate)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T *prCmdManagePeerSchRec =
		NULL;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (!prPeerSchRecord || prPeerSchRecord->fgActive != fgActivate)
		return WLAN_STATUS_FAILURE;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T);
	DBGLOG(NAN, TRACE, "allocate %u bytes for %zu\n",
	       u4CmdBufferLen,
	       sizeof(struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T));
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_MANAGE_PEER_SCH_RECORD,
			sizeof(struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T),
			u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdManagePeerSchRec = (struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T *)
					prTlvElement->aucbody;
	kalMemZero(prCmdManagePeerSchRec,
		   sizeof(struct _NAN_SCHED_CMD_MANAGE_PEER_SCH_REC_T));
	prCmdManagePeerSchRec->u4SchIdx = u4SchIdx;
	prCmdManagePeerSchRec->fgActivate = fgActivate;
	kalMemCopy(prCmdManagePeerSchRec->aucNmiAddr,
		   prPeerSchRecord->aucNmiAddr, MAC_ADDR_LEN);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

static void nanDumpTimelineUpdate(const char *type,
	struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T *prCmdUpdateAvailability)
{
	struct _NAN_CHANNEL_TIMELINE_T *pTime;
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(prCmdUpdateAvailability->arChnlList); i++) {
		pTime = &prCmdUpdateAvailability->arChnlList[i];

		if (!pTime->fgValid)
			continue;

		DBGLOG(NAN, INFO,
		       "Update availability: %s map=%u, timeline=%u, multiple=%u, op=%3u, ch=%3u, bitmap=%02x-%02x-%02x-%02x",
		       type,
		       prCmdUpdateAvailability->ucMapId,
		       prCmdUpdateAvailability->ucTimelineIdx,
		       prCmdUpdateAvailability->fgMultipleMap,
		       pTime->rChnlInfo.u4OperatingClass,
		       pTime->rChnlInfo.u4PrimaryChnl,
		       ((uint8_t *)(pTime->au4AvailMap))[0],
		       ((uint8_t *)(pTime->au4AvailMap))[1],
		       ((uint8_t *)(pTime->au4AvailMap))[2],
		       ((uint8_t *)(pTime->au4AvailMap))[3]);
	}
}

uint32_t
nanSchedCmdUpdateAvailabilityDb(struct ADAPTER *prAdapter,
				size_t szTimeLineIdx,
				unsigned char fgChkCondAvailability)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T *prCmdUpdateAvailability =
		NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T);
	DBGLOG(NAN, TRACE, "allocate %u bytes for %zu\n",
	       u4CmdBufferLen,
	       sizeof(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T));
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_AVAILABILITY,
			sizeof(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T),
			u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
						prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdUpdateAvailability =
		(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_T *)
			prTlvElement->aucbody;

	prCmdUpdateAvailability->ucMapId = prNanTimelineMgmt->ucMapId;
	prCmdUpdateAvailability->fgChkCondAvailability = fgChkCondAvailability;
	prCmdUpdateAvailability->ucTimelineIdx = (uint8_t)szTimeLineIdx;
	if (szNanActiveTimelineNum > 1)
		prCmdUpdateAvailability->fgMultipleMap = TRUE;
	else
		prCmdUpdateAvailability->fgMultipleMap = FALSE;

	DBGLOG(NAN, TRACE,
	       "element tag=%u, body_len=%u, copy %zu, mapId=%u, timeline=%zu\n",
	       prTlvElement->tag_type, prTlvElement->body_len,
	       sizeof(prCmdUpdateAvailability->arChnlList),
	       prNanTimelineMgmt->ucMapId, szTimeLineIdx);
	if (!fgChkCondAvailability) {
		DBGDUMP_HEX(NAN, TEMP, "arChnlList\n",
			   prNanTimelineMgmt->arChnlList,
			   sizeof(prCmdUpdateAvailability->arChnlList));
		kalMemCopy(prCmdUpdateAvailability->arChnlList,
			   prNanTimelineMgmt->arChnlList,
			   sizeof(prCmdUpdateAvailability->arChnlList));

		nanDumpTimelineUpdate("Comm", prCmdUpdateAvailability);
	} else {
		DBGDUMP_HEX(NAN, TEMP, "arCondChnlList\n",
			    prNanTimelineMgmt->arCondChnlList,
			    sizeof(prCmdUpdateAvailability->arChnlList));
		kalMemCopy(prCmdUpdateAvailability->arChnlList,
			   prNanTimelineMgmt->arCondChnlList,
			   sizeof(prCmdUpdateAvailability->arChnlList));

		nanDumpTimelineUpdate("Cond", prCmdUpdateAvailability);
	}

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t
nanSchedCmdUpdateAvailabilityCtrl(struct ADAPTER *prAdapter)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_CTRL_T *prCmdUpdateAvailCtrl =
		NULL;
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);

	u4CmdBufferLen =
		sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
		sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
		sizeof(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_CTRL_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_AVAILABILITY_CTRL,
		       sizeof(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_CTRL_T),
		       u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdUpdateAvailCtrl =
		(struct _NAN_SCHED_CMD_UPDATE_AVAILABILITY_CTRL_T *)
			prTlvElement->aucbody;

	prCmdUpdateAvailCtrl->u2AvailAttrControlField =
		prScheduler->u2NanCurrAvailAttrControlField;
	prCmdUpdateAvailCtrl->ucAvailSeqID = prScheduler->ucNanAvailAttrSeqId;
	DBGLOG(NAN, DEBUG, "AvailAttr SeqID:%d, Ctrl:%x\n",
	       prCmdUpdateAvailCtrl->ucAvailSeqID,
	       prCmdUpdateAvailCtrl->u2AvailAttrControlField);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

/* Public function to query whether NAN is using 6G channels */
u_int8_t nanIs6gInUse(struct ADAPTER *prAdapter,
		      enum _NAN_CHNL_BW_MAP *e6gBandwidth)
{
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);
	if (prScheduler->fgIs6gInUse && e6gBandwidth)
		*e6gBandwidth = prScheduler->e6gBandwidth;

	return prScheduler->fgIs6gInUse;
}

#if (CFG_SUPPORT_NAN_6G == 1)
/* Update flag reflecting whether NAN is using 6G channels */
static void nanUpdate6gUsage(struct ADAPTER *prAdapter,
			     struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt)
{
	struct _NAN_SCHEDULER_T *prScheduler;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline;
	union _NAN_BAND_CHNL_CTRL *prChnlInfo;
	uint32_t u4Idx;

	prScheduler = nanGetScheduler(prAdapter);
	prScheduler->fgIs6gInUse = FALSE;

	for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; u4Idx++) {
		prChnlTimeline = &prNanTimelineMgmt->arChnlList[u4Idx];
		if (prChnlTimeline->fgValid == FALSE)
			continue;

		prChnlInfo = &prChnlTimeline->rChnlInfo;
		if (IS_6G_OP_CLASS(prChnlInfo->u4OperatingClass)) {
			prScheduler->fgIs6gInUse = TRUE;
			DBGLOG(NAN, TRACE, "NAN scheduler 6G %u in use",
			       prChnlInfo->u4PrimaryChnl);

			switch (prChnlInfo->u4OperatingClass) {
			case NAN_6G_BW20_OP_CLASS:
				prScheduler->e6gBandwidth = NAN_CHNL_BW_20;
				break;
			case NAN_6G_BW40_OP_CLASS:
				prScheduler->e6gBandwidth = NAN_CHNL_BW_40;
				break;
			case NAN_6G_BW80_OP_CLASS:
				prScheduler->e6gBandwidth = NAN_CHNL_BW_80;
				break;
			case NAN_6G_BW160_OP_CLASS:
				prScheduler->e6gBandwidth = NAN_CHNL_BW_160;
				break;
			case NAN_6G_BW320_OP_CLASS:
				prScheduler->e6gBandwidth = NAN_CHNL_BW_320;
				break;
			}
		}
	}

	if (!prScheduler->fgIs6gInUse)
		DBGLOG(NAN, TRACE, "NAN scheduler no 6G channels in use");
}
#endif

uint32_t
nanSchedCmdUpdateAvailability(struct ADAPTER *prAdapter)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_SCHEDULER_T *prScheduler = NULL;
	unsigned char fgChange = FALSE;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	prScheduler = nanGetScheduler(prAdapter);

	if (!(prScheduler->fgEn2g |
	      prScheduler->fgEn5gL | prScheduler->fgEn5gH |
	      prScheduler->fgEn6g)) {
		/* Updated in nanSchedConfigAllowedBand */
		DBGLOG(NAN, WARN, "NAN scheduler not ready\n");
		return WLAN_STATUS_FAILURE;
	}

	do {
		if (prScheduler->u2NanAvailAttrControlField &
		    NAN_AVAIL_CTRL_CHECK_FOR_CHANGED) {
			prScheduler->ucNanAvailAttrSeqId++;
			fgChange = TRUE;
		}

		if (prScheduler->u2NanCurrAvailAttrControlField !=
		    prScheduler->u2NanAvailAttrControlField) {
			prScheduler->u2NanCurrAvailAttrControlField =
				prScheduler->u2NanAvailAttrControlField;
			fgChange = TRUE;
		}
		prScheduler->u2NanAvailAttrControlField = 0;

		if (fgChange) {
			cnmTimerStopTimer(
				prAdapter,
				&(prScheduler->rAvailAttrCtrlResetTimer));
			cnmTimerStartTimer(
				prAdapter,
				&(prScheduler->rAvailAttrCtrlResetTimer),
				CFG_NAN_AVAIL_CTRL_RESET_TIMEOUT);

			nanSchedCmdUpdateAvailabilityCtrl(prAdapter);
		}

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
		rlmDomainConnectionNotifiey(prAdapter, NAN_TIMELINE_UPDATE);
#endif
		/* Todo: Update by single command */
		for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
		     szTimeLineIdx++) {
			prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
					szTimeLineIdx);

			rStatus = nanSchedCmdUpdateAvailabilityDb(prAdapter,
				szTimeLineIdx, FALSE);

			if (prNanTimelineMgmt->fgChkCondAvailability)
				rStatus =
				nanSchedCmdUpdateAvailabilityDb(prAdapter,
					szTimeLineIdx, TRUE);

#if (CFG_SUPPORT_NAN_6G == 1)
			if (NAN_IS_6G_TIMELINE(prAdapter, szTimeLineIdx))
				nanUpdate6gUsage(prAdapter, prNanTimelineMgmt);
#endif
		}
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
		uint8_t ucEvtAction =
			TX_PWR_EMI_SCENARIO_TYPE_NAN_TIMELINE_UPDATE;
		rlmDomainPowerLimitEmiEvent(prAdapter, &ucEvtAction);
#endif
	} while (FALSE);

	rlmDomainNanTimeLineUpdateNotify(prAdapter);

	return rStatus;
}

uint32_t
nanSchedCmdUpdatePhySettigns(struct ADAPTER *prAdapter,
			     struct _NAN_PHY_SETTING_T *pr2P4GPhySettings,
			     struct _NAN_PHY_SETTING_T *pr5GPhySettings)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_CMD_UPDATE_PHY_PARAM_T *prNanPhyParam;

	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_UPDATE_PHY_PARAM_T);
	DBGLOG(NAN, TRACE, "allocate %u bytes for %zu\n",
	       u4CmdBufferLen,
	       sizeof(struct _NAN_SCHED_CMD_UPDATE_PHY_PARAM_T));
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_UPDATE_PHY_SETTING,
			sizeof(struct _NAN_SCHED_CMD_UPDATE_PHY_PARAM_T),
			u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prNanPhyParam = (struct _NAN_SCHED_CMD_UPDATE_PHY_PARAM_T *)
				prTlvElement->aucbody;
	prNanPhyParam->r2P4GPhySettings = *pr2P4GPhySettings;
	prNanPhyParam->r5GPhySettings = *pr5GPhySettings;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t
nanSchedCmdUpdateSchedVer(struct ADAPTER *prAdapter)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_CMD_SET_SCHED_VER_T *prNanSchedVer;

	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_SCHED_CMD_SET_SCHED_VER_T);
	DBGLOG(NAN, TRACE, "allocate %u bytes for %zu\n",
	       u4CmdBufferLen, sizeof(struct _NAN_SCHED_CMD_SET_SCHED_VER_T));
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_SET_SCHED_VERSION,
				sizeof(struct _NAN_SCHED_CMD_SET_SCHED_VER_T),
				u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prNanSchedVer = (struct _NAN_SCHED_CMD_SET_SCHED_VER_T *)
				prTlvElement->aucbody;
	prNanSchedVer->ucNdlFlowCtrlVer = prAdapter->rWifiVar.ucNdlFlowCtrlVer;
	DBGLOG(NAN, DEBUG, "Set NDL version:%u\n",
		prNanSchedVer->ucNdlFlowCtrlVer);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}

uint32_t
nanSchedEventScheduleConfig(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
			    uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_EVENT_SCHEDULE_CONFIG_T *prEventScheduleConfig;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	prEventScheduleConfig =
		(struct _NAN_SCHED_EVENT_SCHEDULE_CONFIG_T *)pucBuf;

	nanSchedConfigAllowedBand(prAdapter, prEventScheduleConfig->fgEn2g,
				  prEventScheduleConfig->fgEn5gH,
				  prEventScheduleConfig->fgEn5gL,
				  prEventScheduleConfig->fgEn6g);
	nanSchedConfigDefNdlNumSlots(prAdapter, prWifiVar->ucDftNdlQuotaVal);
	nanSchedConfigDefRangingNumSlots(prAdapter,
					 prWifiVar->ucDftRangQuotaVal);

	nanSchedCmdUpdatePotentialChnlList(prAdapter);

	return rRetStatus;
}

uint32_t
nanSchedEventDevCapability(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
			uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_EVENT_DEV_CAP_T *prEventDevCap;

	prEventDevCap = (struct _NAN_SCHED_EVENT_DEV_CAP_T *)pucBuf;
	g_u4MaxChnlSwitchTimeUs = prEventDevCap->u2MaxChnlSwitchTimeUs;
	DBGLOG(NAN, DEBUG,
	       "MaxChnlSwitchTime:%d us\n", g_u4MaxChnlSwitchTimeUs);

	return rRetStatus;
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
uint32_t
nanSchedUniEventNanAttr(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
		     uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_EVENT_NAN_ATTR_T *prEventNanAttr;
	struct _NAN_ATTR_HDR_T *prAttrHdr;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	uint32_t u4Idx = 0;

	prEventNanAttr = (struct _NAN_SCHED_EVENT_NAN_ATTR_T *)pucBuf;
	prAttrHdr = (struct _NAN_ATTR_HDR_T *)prEventNanAttr->aucNanAttr;

	DBGLOG(NAN, DEBUG, "Nmi> %02x:%02x:%02x:%02x:%02x:%02x, SubEvent:%d\n",
		prEventNanAttr->aucNmiAddr[0], prEventNanAttr->aucNmiAddr[1],
		prEventNanAttr->aucNmiAddr[2], prEventNanAttr->aucNmiAddr[3],
		prEventNanAttr->aucNmiAddr[4], prEventNanAttr->aucNmiAddr[5],
		u4SubEvent);
#ifdef NAN_UNUSED
	nanUtilDump(prAdapter, "NAN Attribute",
		(PUINT_8)prAttrHdr, NAN_ATTR_SIZE(prAttrHdr));
#endif

	switch (u4SubEvent) {
	case UNI_EVENT_NAN_TAG_ID_PEER_CAPABILITY:
		nanSchedPeerUpdateDevCapabilityAttr(prAdapter,
						    prEventNanAttr->aucNmiAddr,
						    prEventNanAttr->aucNanAttr);
		break;
	case UNI_EVENT_NAN_TAG_ID_PEER_AVAILABILITY:
		/* Skip update availability by event
		* if NDP negotiation is ongoing
		*/
		prNDL = nanDataUtilSearchNdlByMac(
			prAdapter, prEventNanAttr->aucNmiAddr);
		if (prNDL) {
			if (prNDL->prOperatingNDP)
				DBGLOG(NAN, DEBUG, "operating NDP %d\n",
				prNDL->prOperatingNDP->ucNDPID);

			if (prNDL->ucNDPNum) {
				for (u4Idx = 0; u4Idx < prNDL->ucNDPNum;
				     u4Idx++) {
					prNDP = &(prNDL->arNDP[u4Idx]);
					DBGLOG(NAN, DEBUG,
					       "NDP idx[%d] NDPID[%d] state[%d]\n",
					       u4Idx, prNDP->ucNDPID,
					       prNDP->eCurrentNDPProtocolState);

					if (prNDP->eCurrentNDPProtocolState !=
					    NDP_IDLE &&
					    prNDP->eCurrentNDPProtocolState !=
					    NDP_NORMAL_TR) {
						DBGLOG(NAN, DEBUG,
						       "Skip due to peer under negotiation\n");
						return WLAN_STATUS_FAILURE;
					}
				}
			} else {
				DBGLOG(NAN, DEBUG,
					"No NDP found %d\n", prNDL->ucNDPNum);
			}
		} else {
			DBGLOG(NAN, DEBUG, "No NDL found\n");
		}

		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, /* from SDF */
						   prEventNanAttr->aucNmiAddr,
						   prEventNanAttr->aucNanAttr,
						   prNDP);
		break;

	default:
		break;
	}

	return rRetStatus;
}
#else
uint32_t
nanSchedEventNanAttr(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
		     uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_EVENT_NAN_ATTR_T *prEventNanAttr;
	struct _NAN_ATTR_HDR_T *prAttrHdr;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	uint32_t u4Idx = 0;

	prEventNanAttr = (struct _NAN_SCHED_EVENT_NAN_ATTR_T *)pucBuf;
	prAttrHdr = (struct _NAN_ATTR_HDR_T *)prEventNanAttr->aucNanAttr;

	DBGLOG(NAN, DEBUG, "Nmi> %02x:%02x:%02x:%02x:%02x:%02x, SubEvent:%d\n",
		prEventNanAttr->aucNmiAddr[0], prEventNanAttr->aucNmiAddr[1],
		prEventNanAttr->aucNmiAddr[2], prEventNanAttr->aucNmiAddr[3],
		prEventNanAttr->aucNmiAddr[4], prEventNanAttr->aucNmiAddr[5],
		u4SubEvent);
#ifdef NAN_UNUSED
	nanUtilDump(prAdapter, "NAN Attribute",
		(PUINT_8)prAttrHdr, NAN_ATTR_SIZE(prAttrHdr));
#endif

	switch (u4SubEvent) {
	case NAN_EVENT_ID_PEER_CAPABILITY:
		nanSchedPeerUpdateDevCapabilityAttr(prAdapter,
						    prEventNanAttr->aucNmiAddr,
						    prEventNanAttr->aucNanAttr);
		break;
	case NAN_EVENT_ID_PEER_AVAILABILITY:
		/* Skip update availability by event
		* if NDP negotiation is ongoing
		*/
		prNDL = nanDataUtilSearchNdlByMac(
			prAdapter, prEventNanAttr->aucNmiAddr);
		if (prNDL) {
			if (prNDL->prOperatingNDP)
				DBGLOG(NAN, DEBUG, "operating NDP %d\n",
				prNDL->prOperatingNDP->ucNDPID);

			if (prNDL->ucNDPNum) {
				for (u4Idx = 0; u4Idx < prNDL->ucNDPNum;
				     u4Idx++) {
					prNDP = &(prNDL->arNDP[u4Idx]);
					DBGLOG(NAN, DEBUG,
					       "NDP idx[%d] NDPID[%d] state[%d]\n",
					       u4Idx, prNDP->ucNDPID,
					       prNDP->eCurrentNDPProtocolState);

					if (prNDP->eCurrentNDPProtocolState !=
					    NDP_IDLE &&
					    prNDP->eCurrentNDPProtocolState !=
					    NDP_NORMAL_TR) {
						DBGLOG(NAN, DEBUG,
						       "Skip due to peer under negotiation\n");
						return WLAN_STATUS_FAILURE;
					}
				}
			} else {
				DBGLOG(NAN, DEBUG,
					"No NDP found %d\n", prNDL->ucNDPNum);
			}
		} else {
			DBGLOG(NAN, DEBUG, "No NDL found\n");
		}

		nanSchedPeerUpdateAvailabilityAttr(prAdapter,
						   0,
						   prEventNanAttr->aucNmiAddr,
						   prEventNanAttr->aucNanAttr,
						   prNDP);
		break;

	default:
		break;
	}

	return rRetStatus;
}
#endif

uint32_t
nanSchedEventDwInterval(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
			uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SCHED_EVENT_DW_INTERVAL_T *prEventDwInterval;
	struct _NAN_SCHEDULER_T *prScheduler;

	prEventDwInterval = (struct _NAN_SCHED_EVENT_DW_INTERVAL_T *)pucBuf;
	prScheduler = nanGetScheduler(prAdapter);
	prScheduler->ucCommitDwInterval = prEventDwInterval->ucDwInterval;

	return rRetStatus;
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
uint32_t
nanSchedulerUniEventDispatch(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
			  uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(NAN, DEBUG, "Evt:%d\n", u4SubEvent);

	switch (u4SubEvent) {
	case UNI_EVENT_NAN_TAG_ID_SCHEDULE_CONFIG:
		nanSchedEventScheduleConfig(prAdapter, u4SubEvent, pucBuf);
		break;

	case UNI_EVENT_NAN_TAG_ID_PEER_CAPABILITY:
	case UNI_EVENT_NAN_TAG_ID_PEER_AVAILABILITY:
		nanSchedUniEventNanAttr(prAdapter, u4SubEvent, pucBuf);
		break;
	case UNI_EVENT_NAN_TAG_ID_CRB_HANDSHAKE_TOKEN:
		break;
	case UNI_EVENT_NAN_TAG_DW_INTERVAL:
		nanSchedEventDwInterval(prAdapter, u4SubEvent, pucBuf);
		break;
	case UNI_EVENT_NAN_TAG_ID_DEVICE_CAPABILITY:
		nanSchedEventDevCapability(prAdapter, u4SubEvent, pucBuf);
		break;
	default:
		break;
	}

	return rRetStatus;
}

#else
uint32_t
nanSchedulerEventDispatch(struct ADAPTER *prAdapter, uint32_t u4SubEvent,
			  uint8_t *pucBuf)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(NAN, LOUD, "Evt:%d\n", u4SubEvent);

	switch (u4SubEvent) {
	case NAN_EVENT_ID_SCHEDULE_CONFIG:
		nanSchedEventScheduleConfig(prAdapter, u4SubEvent, pucBuf);
		break;

	case NAN_EVENT_ID_PEER_CAPABILITY:
	case NAN_EVENT_ID_PEER_AVAILABILITY:
		nanSchedEventNanAttr(prAdapter, u4SubEvent, pucBuf);
		break;
	case NAN_EVENT_ID_CRB_HANDSHAKE_TOKEN:
		break;
	case NAN_EVENT_DW_INTERVAL:
		nanSchedEventDwInterval(prAdapter, u4SubEvent, pucBuf);
		break;
	case NAN_EVENT_ID_DEVICE_CAPABILITY:
		nanSchedEventDevCapability(prAdapter, u4SubEvent, pucBuf);
		break;
	default:
		break;
	}

	return rRetStatus;
}
#endif

#define NAN_STATION_TEST_ADDRESS                                               \
	{ 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 }

/* [0] ChnlRaw:0x247301, PriChnl:36
 * [Map], len:64
 * 0x02057668: 00 0e 00 00 00 0e 00 00 00 0e 00 00 00 0e 00 00
 * 0x02057678: 00 0e 00 00 00 0e 00 00 00 0e 00 00 00 0e 00 00
 * 0x02057688: 00 0e 00 00 00 0e 00 00 00 0e 00 00 00 0e 00 00
 * 0x02057698: 00 0e 00 00 00 0e 00 00 00 0e 00 00 00 0e 00 00
 *
 * [1] ChnlRaw:0xa17c01, PriChnl:161
 * [Map], len:64
 * 0x020576c4: 00 20 00 00 00 20 00 00 00 20 00 00 00 20 00 00
 * 0x020576d4: 00 20 00 00 00 20 00 00 00 20 00 00 00 20 00 00
 * 0x020576e4: 00 20 00 00 00 20 00 00 00 20 00 00 00 20 00 00
 * 0x020576f4: 00 20 00 00 00 20 00 00 00 20 00 00 00 20 00 00
 */
uint8_t g_aucPeerAvailabilityAttr[] = { 0x12, 0x1D, 0x0,  0x0,  0x1, 0x0, 0xb,
				       0x0,  0x1,  0x12, 0x58, 0x2, 0x1, 0x7,
				       0x11, 0x73, 0x1,  0x0,  0x0, 0xb, 0x0,
				       0x1,  0x12, 0x58, 0x3,  0x1, 0x1, 0x11,
				       0x7c, 0x8,  0x0,  0x0 };
uint8_t g_aucPeerAvailabilityAttr2[] = { 0x12, 0xc,  0x0,  0x1,  0x21,
					0x0,  0x7,  0x0,  0x1a, 0x0,
					0x11, 0x51, 0xff, 0x7,  0x0 };

/* commit Chnl:6
 * AvailMap, len:64
 * ffffffffc0c9158c: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * ffffffffc0c9159c: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * ffffffffc0c915ac: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * ffffffffc0c915bc: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * Commit Chnl:149
 * AvailMap, len:64
 * ffffffffc0c915d8: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * ffffffffc0c915e8: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * ffffffffc0c915f8: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * ffffffffc0c91608: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 */
uint8_t g_aucPeerAvailabilityAttr3[] = {
	0x12, 0x2b, 0x0,  0x01, 0x11, 0x0,  0xb,  0x0,  0x1,  0x12, 0x18, 0x0,
	0x1,  0x1,  0x11, 0x51, 0x20, 0x0,  0x0,  0xb,  0x0,  0x1,  0x12, 0x18,
	0x2,  0x1,  0x1,  0x11, 0x7c, 0x01, 0x0,  0x0,  0xc,  0x0,  0x2,  0x12,
	0x58, 0x0,  0x4,  0x7f, 0xff, 0xff, 0x7f, 0x20, 0x02, 0x04
};

/* [0], Commit Chnl:6
 * AvailMap, len:64
 * ffffffffc0c915cc: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * ffffffffc0c915dc: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * ffffffffc0c915ec: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * ffffffffc0c915fc: 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00
 * [1], Commit Chnl:149
 * AvailMap, len:64
 * ffffffffc0c91618: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * ffffffffc0c91628: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * ffffffffc0c91638: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * ffffffffc0c91648: 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00
 * [0], Cond Chnl:149
 * AvailMap, len:64
 * ffffffffc0c918c8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 * ffffffffc0c918d8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 * ffffffffc0c918e8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 * ffffffffc0c918f8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 */
uint8_t g_aucPeerAvailabilityAttr4[] = {
	0x12, 0x38, 0x00, 0x01, 0x11, 0x00, 0x0b, 0x00, 0x01, 0x12, 0x18, 0x00,
	0x01, 0x01, 0x11, 0x51, 0x20, 0x00, 0x00, 0x0b, 0x00, 0x01, 0x12, 0x18,
	0x02, 0x01, 0x01, 0x11, 0x7c, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x04, 0x12,
	0x58, 0x00, 0x01, 0x01, 0x11, 0x7c, 0x01, 0x00, 0x00, 0x0c, 0x00, 0x02,
	0x12, 0x58, 0x00, 0x04, 0x7f, 0xff, 0xff, 0x7f, 0x20, 0x02, 0x04
};

uint8_t g_aucPeerAvailabilityAttr5[] = {
	0x12, 0x2f, 0x00, 0x01, 0x11, 0x00,

	0x0e, 0x00, 0x01, 0x12, 0x18, 0x00, 0x04, 0x01, 0x00, 0x00,
	0x00, 0x11, 0x7c, 0x01, 0x00, 0x00,

	0x1a, 0x00, 0x1a, 0x12, 0x18, 0x00, 0x04, 0xff, 0xfe, 0xff,
	0xff, 0x41, 0x7e, 0x03, 0x00, 0x00, 0x7f, 0x03, 0x00, 0x00,
	0x73, 0x0f, 0x00, 0x00, 0x51, 0xff, 0x07, 0x00
};

uint8_t g_aucRangSchEntryList1[] = { 0x1, 0x18, 0x0, 0x4, 0x1, 0x0, 0x0, 0x0 };

uint8_t g_aucPeerSelectedNdcAttr[] = { 0x13, 0xc, 0x0, 0x1,  0x2, 0x3, 0x4, 0x5,
				      0x6,  0x1, 0x1, 0x58, 0x3, 0x1, 0x1 };

/* ffffffffc0c918c8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 * ffffffffc0c918d8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 * ffffffffc0c918e8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 * ffffffffc0c918f8: 02 00 00 00 02 00 00 00 02 00 00 00 02 00 00 00
 */
uint8_t g_aucRangSchEntryList[] = { 0x1, 0x58, 0x0, 0x1, 0x1 };
uint8_t g_aucTestData[100];

#ifdef NAN_UNUSED
UINT_8 g_aucCase_5_3_3_DataReq_AvailAttr[] = {
	0x12, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x0e, 0x00, 0x0c, 0x11, 0x18, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x11, 0x51,
	0x20, 0x00, 0x00, 0x0b, 0x00, 0x0a, 0x11,
	0x18, 0x00, 0x04, 0x7e, 0xfe, 0xff,
	0x7f, 0x10, 0x02};
#else
uint8_t g_aucCase_5_3_3_DataReq_AvailAttr[] = {
	0x12, 0x20, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0c, 0x11, 0x18, 0x00,
	0x04, 0x7e, 0xfe, 0xff, 0x0f, 0x11, 0x51, 0x20, 0x00, 0x00, 0x0b, 0x00,
	0x0a, 0x11, 0x18, 0x00, 0x04, 0x7e, 0xfe, 0xff, 0x7f, 0x10, 0x02
};
#endif
uint8_t g_aucCase_5_3_3_DataReq_NdcAttr[] = {
	0x13, 0x0f, 0x00, 0x50, 0x6f, 0x9a, 0x01, 0x00, 0x00,
	0x01, 0x00, 0x18, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00
};

uint8_t g_aucCase_5_3_3_DataReq_ImmNdl[] = { 0x00, 0x18, 0x00, 0x04,
					    0x00, 0x00, 0x01, 0x00 };

uint8_t g_aucCase_5_3_1_Publish_AvailAttr[] = { 0x12, 0x0c, 0x00, 0x01, 0x20,
					       0x00, 0x07, 0x00, 0x1a, 0x00,
					       0x11, 0x51, 0xff, 0x07, 0x00 };

uint8_t g_aucCase_5_3_1_Publish_AvailAttr2[] = {
	0x12, 0x23, 0x00, 0x07, 0x00, 0x00, 0x0e, 0x00, 0x1a, 0x10,
	0x18, 0x00, 0x04, 0x00, 0x00, 0x80, 0xff, 0x11, 0x51, 0xff,
	0x07, 0x00, 0x0e, 0x00, 0x02, 0x10, 0x18, 0x00, 0x04, 0xfe,
	0xff, 0x7f, 0x00, 0x11, 0x51, 0x20, 0x00, 0x00
};

uint8_t g_aucCase_5_3_1_DataRsp_AvailAttr[] = {
	0x12, 0x23, 0x00, 0x08, 0xb0, 0x00, 0x0e, 0x00, 0x1a, 0x10,
	0x18, 0x00, 0x04, 0x00, 0x00, 0x80, 0xff, 0x11, 0x51, 0xff,
	0x07, 0x00, 0x0e, 0x00, 0x04, 0x10, 0x18, 0x00, 0x04, 0xfe,
	0xff, 0x7f, 0x00, 0x11, 0x51, 0x20, 0x00, 0x00
};

uint8_t g_aucCase_5_3_11_DataReq_Avail1Attr[] = {
	0x12, 0x31, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0c, 0x11, 0x18,
	0x00, 0x04, 0x7e, 0x00, 0x00, 0x00, 0x11, 0x51, 0x20, 0x00, 0x00,
	0x0e, 0x00, 0x0c, 0x11, 0x18, 0x00, 0x04, 0x00, 0xfe, 0xff, 0x7f,
	0x11, 0x7d, 0x01, 0x00, 0x00, 0x0c, 0x00, 0x0a, 0x11, 0x18, 0x00,
	0x04, 0x7e, 0xfe, 0xff, 0x7f, 0x20, 0x02, 0x04
};

uint8_t g_aucCase_5_3_11_DataReq_Avail2Attr[] = {
	0x12, 0x31, 0x00, 0x00, 0x01, 0x00, 0x0e, 0x00, 0x0c, 0x11, 0x18,
	0x00, 0x04, 0x7e, 0x00, 0x00, 0x00, 0x11, 0x7d, 0x01, 0x00, 0x00,
	0x0e, 0x00, 0x0c, 0x11, 0x18, 0x00, 0x04, 0x00, 0xfe, 0xff, 0x7f,
	0x11, 0x51, 0x20, 0x00, 0x00, 0x0c, 0x00, 0x0a, 0x11, 0x18, 0x00,
	0x04, 0x7e, 0xfe, 0xff, 0x7f, 0x20, 0x02, 0x04
};

uint8_t g_aucCase_5_3_11_DataReq_NdcAttr[] = { 0x13, 0x0f, 0x00, 0x50, 0x6f,
					      0x9a, 0x01, 0x00, 0x00, 0x01,
					      0x00, 0x18, 0x00, 0x04, 0x00,
					      0x02, 0x00, 0x00 };

void
nanScheduleNegoTestFunc(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr,
			enum _ENUM_NAN_NEGO_TYPE_T eType,
			enum _ENUM_NAN_NEGO_ROLE_T eRole, void *pvToken)
{
	uint8_t *pucBuf;
	uint32_t u4Length;
	uint32_t rRetStatus;
	uint32_t u4RejectCode;
	uint8_t aucNmiAddr[] = NAN_STATION_TEST_ADDRESS;
	uint8_t aucTestData[100];

	DBGLOG(NAN, TRACE, "IN\n");

	if (pvToken == (void *)5) {
		nanSchedNegoAddNdcCrb(prAdapter, &g_r5gDwChnl, 16, 1,
				      ENUM_TIME_BITMAP_CTRL_PERIOD_512);
		nanSchedNegoGetSelectedNdcAttr(prAdapter, &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[NDC Attr]", pucBuf, u4Length);

		nanSchedNegoStop(prAdapter);
	} else if (pvToken == (void *)7) {
		nanSchedNegoGenLocalCrbProposal(prAdapter);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStop(prAdapter);
	} else if (pvToken == (void *)8) {
		nanSchedNegoAddQos(prAdapter, 10, 3);
		nanSchedNegoGenLocalCrbProposal(prAdapter);
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);

		nanSchedNegoStop(prAdapter);
	} else if (pvToken == (void *)9) {
		nanSchedNegoAddNdcCrb(prAdapter, &g_r2gDwChnl, 14, 1,
				      ENUM_TIME_BITMAP_CTRL_PERIOD_256);
		nanSchedNegoGenLocalCrbProposal(prAdapter);
		nanSchedNegoGetSelectedNdcAttr(prAdapter, &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Selected NDC]", pucBuf, u4Length);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)10) {
		/* nanSchedNegoAddQos(prAdapter, 10, 3); */
		rRetStatus =
			nanSchedNegoChkRmtCrbProposal(prAdapter, &u4RejectCode);
		DBGLOG(NAN, DEBUG, "nanSchedNegoChkRmtCrbProposal: %x, %u\n",
		       rRetStatus, u4RejectCode);
		nanSchedNegoGetSelectedNdcAttr(prAdapter, &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[NDC Attr]", pucBuf, u4Length);
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStop(prAdapter);
	} else if (pvToken == (void *)11) {
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)14) {

		rRetStatus = nanSchedNegoGenLocalCrbProposal(prAdapter);
		DBGLOG(NAN, DEBUG, "nanSchedNegoGenLocalCrbProposal: %x\n",
		       rRetStatus);
		nanSchedNegoGetRangingScheduleList(prAdapter, &pucBuf,
						   &u4Length);
		nanUtilDump(prAdapter, "[Ranging]", pucBuf, u4Length);

		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)15) {
		rRetStatus =
			nanSchedNegoChkRmtCrbProposal(prAdapter, &u4RejectCode);
		DBGLOG(NAN, DEBUG, "nanSchedNegoChkRmtCrbProposal: %x, %u\n",
		       rRetStatus, u4RejectCode);
		nanSchedNegoGetRangingScheduleList(prAdapter, &pucBuf,
						   &u4Length);
		nanUtilDump(prAdapter, "[Ranging]", pucBuf, u4Length);
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)18) {

		rRetStatus =
			nanSchedNegoChkRmtCrbProposal(prAdapter, &u4RejectCode);
		DBGLOG(NAN, DEBUG, "nanSchedNegoChkRmtCrbProposal: %x, %u\n",
		       rRetStatus, u4RejectCode);

		DBGLOG(NAN, DEBUG, "DUMP#5\n");
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);

		nanSchedNegoGetSelectedNdcAttr(prAdapter, &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[NDC Attr]", pucBuf, u4Length);

		nanSchedNegoGetImmuNdlScheduleList(prAdapter, &pucBuf,
						   &u4Length);
		nanUtilDump(prAdapter, "[Imm NDL Attr]", pucBuf, u4Length);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)19) {
		rRetStatus = nanSchedNegoGenLocalCrbProposal(prAdapter);
		DBGLOG(NAN, DEBUG, "nanSchedNegoGenLocalCrbProposal: %x\n",
		       rRetStatus);

		DBGLOG(NAN, DEBUG, "DUMP#2\n");
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)20) {

		rRetStatus = nanSchedNegoGenLocalCrbProposal(prAdapter);
		DBGLOG(NAN, DEBUG, "nanSchedNegoGenLocalCrbProposal: %x\n",
		       rRetStatus);

		DBGLOG(NAN, DEBUG, "DUMP#4\n");
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);

		nanSchedNegoGetSelectedNdcAttr(prAdapter, &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[NDC Attr]", pucBuf, u4Length);

		nanSchedNegoStop(prAdapter);

	} else if (pvToken == (void *)23) {
		rRetStatus = nanSchedNegoGenLocalCrbProposal(prAdapter);
		DBGLOG(NAN, DEBUG, "nanSchedNegoGenLocalCrbProposal: %x\n",
		       rRetStatus);

		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);

		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr5,
			   sizeof(g_aucPeerAvailabilityAttr5));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		kalMemCopy(aucTestData, g_aucRangSchEntryList1,
			   sizeof(g_aucRangSchEntryList1));
		nanSchedPeerUpdateRangingScheduleList(
			prAdapter, aucNmiAddr,
			(struct _NAN_SCHEDULE_ENTRY_T *)aucTestData,
			sizeof(g_aucRangSchEntryList1));

		rRetStatus =
			nanSchedNegoChkRmtCrbProposal(prAdapter, &u4RejectCode);
		DBGLOG(NAN, DEBUG, "nanSchedNegoChkRmtCrbProposal: %x, %u\n",
		       rRetStatus, u4RejectCode);

		nanSchedNegoStop(prAdapter);
	} else if (pvToken == (void *)24) {
		rRetStatus =
			nanSchedNegoChkRmtCrbProposal(prAdapter, &u4RejectCode);
		DBGLOG(NAN, DEBUG, "nanSchedNegoChkRmtCrbProposal: %x, %u\n",
		       rRetStatus, u4RejectCode);

		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[AVAIL ATTR]", pucBuf, u4Length);

		nanSchedNegoGetSelectedNdcAttr(prAdapter, &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[NDC ATTR]", pucBuf, u4Length);

		nanSchedNegoStop(prAdapter);
	}
}

uint32_t
nanSchedSwDbg4(struct ADAPTER *prAdapter, uint32_t u4Data) /* 0x7426000d */
{
	uint32_t u4Ret = 0;
	uint8_t *pucBuf;
	uint32_t u4Length;
	uint8_t aucNmiAddr[] = NAN_STATION_TEST_ADDRESS;
	uint32_t au4Map[NAN_TOTAL_DW];
	uint32_t u4Idx;
	uint8_t aucTestData[100];
	uint32_t rRetStatus;
	struct _NAN_SCHEDULER_T *prNanScheduler;
	uint32_t u4TestDataLen;
	union _NAN_BAND_CHNL_CTRL rChnlInfo = {
		.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
		.u4OperatingClass = NAN_5G_LOW_DISC_CH_OP_CLASS,
		.u4PrimaryChnl = 36,
		.u4AuxCenterChnl = 0
	};
	uint8_t aucBitmap[4];

	prNanScheduler = nanGetScheduler(prAdapter);

	if (prNanScheduler->fgInit == FALSE)
		nanSchedInit(prAdapter);

	switch (u4Data) {
	case 0:
#ifdef NAN_UNUSED
		nanSchedConfigAllowedBand(prAdapter, TRUE, TRUE, TRUE, TRUE);
#else
		nanSchedConfigAllowedBand(prAdapter, TRUE, FALSE, FALSE, FALSE);
#endif
		nanSchedConfigDefNdlNumSlots(prAdapter, 3);
		nanSchedConfigDefRangingNumSlots(prAdapter, 1);
		break;

	case 1:
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
		break;

	case 2:
		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);
		break;

	case 3:
		kalMemZero(au4Map, sizeof(au4Map));
		for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++) {
			au4Map[u4Idx] |= (BIT(NAN_5G_DW_INDEX + 1));
			au4Map[u4Idx] |= (BIT(NAN_5G_DW_INDEX + 2));
			au4Map[u4Idx] |= (BIT(NAN_5G_DW_INDEX + 3));
			au4Map[u4Idx] |= (BIT(NAN_5G_DW_INDEX + 4));
		}

		nanUtilDump(prAdapter, "[Map]", (uint8_t *)au4Map,
			    sizeof(au4Map));
		nanParserGenTimeBitmapField(prAdapter, au4Map, g_aucNanIEBuffer,
					    &u4Length);
		nanUtilDump(prAdapter, "[TimeBitmap]", g_aucNanIEBuffer,
			    u4Length);
		break;

	case 4:
		kalMemZero(au4Map, sizeof(au4Map));
		for (u4Idx = 0; u4Idx < NAN_TOTAL_DW; u4Idx++)
			au4Map[u4Idx] |= (BIT(NAN_5G_DW_INDEX + 5));

		nanUtilDump(prAdapter, "[Map]", (uint8_t *)au4Map,
			    sizeof(au4Map));
		nanParserGenTimeBitmapField(prAdapter, au4Map, g_aucNanIEBuffer,
					    &u4Length);
		nanUtilDump(prAdapter, "[TimeBitmap]", g_aucNanIEBuffer,
			    u4Length);
		break;

	case 5:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr,
			   sizeof(g_aucPeerAvailabilityAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)5);
		break;

	case 6:
#if 1
		rChnlInfo.u4OperatingClass = NAN_5G_LOW_DISC_CH_OP_CLASS;
		rChnlInfo.u4PrimaryChnl = 36;
		rRetStatus = nanSchedAddCrbToChnlList(prAdapter, &rChnlInfo,
				9, 2, ENUM_TIME_BITMAP_CTRL_PERIOD_512,
				TRUE, NULL);
#endif

#if 1
		rChnlInfo.u4OperatingClass = NAN_5G_HIGH_DISC_CH_OP_CLASS;
		rChnlInfo.u4PrimaryChnl = 161;
		rRetStatus = nanSchedAddCrbToChnlList(prAdapter, &rChnlInfo,
				13, 1, ENUM_TIME_BITMAP_CTRL_PERIOD_512,
				TRUE, NULL);
#endif
		break;

	case 7:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr,
			   sizeof(g_aucPeerAvailabilityAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)7);
		break;

	case 8:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr,
			   sizeof(g_aucPeerAvailabilityAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)8);

		break;

	case 9:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr,
			   sizeof(g_aucPeerAvailabilityAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)9);
		break;

	case 10:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr,
			   sizeof(g_aucPeerAvailabilityAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_RESPONDER,
				  nanScheduleNegoTestFunc, (void *)10);
		break;

	case 11:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr2,
			   sizeof(g_aucPeerAvailabilityAttr2));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_RESPONDER,
				  nanScheduleNegoTestFunc, (void *)11);

		break;

	case 12:
		break;

	case 13:
		prNanScheduler->fgInit = FALSE;
		break;

	case 14:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr3,
			   sizeof(g_aucPeerAvailabilityAttr3));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		nanSchedNegoStart(prAdapter, aucNmiAddr, ENUM_NAN_NEGO_RANGING,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)14);
		break;

	case 15:
		kalMemCopy(aucTestData, g_aucRangSchEntryList,
			   sizeof(g_aucRangSchEntryList));
		u4TestDataLen = sizeof(g_aucRangSchEntryList);
		nanSchedPeerUpdateRangingScheduleList(
			prAdapter, aucNmiAddr,
			(struct _NAN_SCHEDULE_ENTRY_T *)aucTestData,
			u4TestDataLen);

		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr4,
			   sizeof(g_aucPeerAvailabilityAttr4));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
		nanSchedNegoStart(prAdapter, aucNmiAddr, ENUM_NAN_NEGO_RANGING,
				  ENUM_NAN_NEGO_ROLE_RESPONDER,
				  nanScheduleNegoTestFunc, (void *)15);

		break;

	case 16:
		nanSchedInit(prAdapter);
		break;

	case 17:
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);
		break;

	case 18:
		DBGLOG(NAN, DEBUG, "DUMP#1\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		kalMemCopy(aucTestData, g_aucCase_5_3_3_DataReq_AvailAttr,
			   sizeof(g_aucCase_5_3_3_DataReq_AvailAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		DBGLOG(NAN, DEBUG, "DUMP#2\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		kalMemCopy(aucTestData, g_aucCase_5_3_3_DataReq_NdcAttr,
			   sizeof(g_aucCase_5_3_3_DataReq_NdcAttr));
		nanSchedPeerUpdateNdcAttr(prAdapter, aucNmiAddr, aucTestData);

#if 1
		nanSchedPeerUpdateImmuNdlScheduleList(
			prAdapter, aucNmiAddr,
			(struct _NAN_SCHEDULE_ENTRY_T *)
				g_aucCase_5_3_3_DataReq_ImmNdl,
			sizeof(g_aucCase_5_3_3_DataReq_ImmNdl));
#endif

		DBGLOG(NAN, DEBUG, "DUMP#3\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		DBGLOG(NAN, DEBUG, "DUMP#4\n");
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_RESPONDER,
				  nanScheduleNegoTestFunc, (void *)18);
		break;

	case 19:
		DBGLOG(NAN, DEBUG, "DUMP#1\n");
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)19);

		break;

	case 20:
		DBGLOG(NAN, DEBUG, "DUMP#1\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		kalMemCopy(aucTestData, g_aucCase_5_3_1_Publish_AvailAttr,
			   sizeof(g_aucCase_5_3_1_Publish_AvailAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		DBGLOG(NAN, DEBUG, "DUMP#2\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		DBGLOG(NAN, DEBUG, "DUMP#3\n");
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)20);
		break;

	case 21:
		DBGLOG(NAN, DEBUG, "DUMP#1\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		kalMemCopy(aucTestData, g_aucCase_5_3_1_DataRsp_AvailAttr,
			   sizeof(g_aucCase_5_3_1_DataRsp_AvailAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		DBGLOG(NAN, DEBUG, "DUMP#2\n");
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);
		break;

	case 22:
		nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
		nanSchedGetAvailabilityAttr(prAdapter, NULL,
					    &pucBuf, &u4Length);
		nanUtilDump(prAdapter, "[Availability Attr]", pucBuf, u4Length);
		break;

	case 23:
		kalMemCopy(aucTestData, g_aucPeerAvailabilityAttr,
			   sizeof(g_aucPeerAvailabilityAttr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		nanSchedNegoStart(prAdapter, aucNmiAddr, ENUM_NAN_NEGO_RANGING,
				  ENUM_NAN_NEGO_ROLE_INITIATOR,
				  nanScheduleNegoTestFunc, (void *)23);
		break;

	case 24:
		kalMemCopy(aucTestData, g_aucCase_5_3_11_DataReq_Avail1Attr,
			   sizeof(g_aucCase_5_3_11_DataReq_Avail1Attr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);

		kalMemCopy(aucTestData, g_aucCase_5_3_11_DataReq_Avail2Attr,
			   sizeof(g_aucCase_5_3_11_DataReq_Avail2Attr));
		nanSchedPeerUpdateAvailabilityAttr(prAdapter, 0, aucNmiAddr,
						   aucTestData, NULL);
		nanSchedDbgDumpPeerAvailability(prAdapter, aucNmiAddr);

		kalMemCopy(aucTestData, g_aucCase_5_3_11_DataReq_NdcAttr,
			   sizeof(g_aucCase_5_3_11_DataReq_NdcAttr));
		nanSchedPeerUpdateNdcAttr(prAdapter, aucNmiAddr, aucTestData);

		nanSchedNegoStart(prAdapter, aucNmiAddr,
				  ENUM_NAN_NEGO_DATA_LINK,
				  ENUM_NAN_NEGO_ROLE_RESPONDER,
				  nanScheduleNegoTestFunc, (void *)24);
		break;

	case 25:
		halPrintHifDbgInfo(prAdapter);
		break;

	case 26:
		aucBitmap[0] = 0xaa;
		aucBitmap[1] = aucBitmap[2] = aucBitmap[3] = 0;
		kalMemZero(aucTestData, sizeof(aucTestData));
		nanParserInterpretTimeBitmapField(
		    prAdapter, 0x0008, 4, aucBitmap, (uint32_t *)aucTestData);
		nanUtilDump(prAdapter, "time map", aucTestData, 64);
		break;

	default:
		break;
	}

	return u4Ret;
}

enum ENUM_BAND
nanSchedGetSchRecBandByMac(struct ADAPTER *prAdapter, uint8_t *pucNmiAddr)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;

	prPeerSchRecord = nanSchedLookupPeerSchRecord(prAdapter, pucNmiAddr);
	if (prPeerSchRecord)
		return prPeerSchRecord->eBand;
	else
		return BAND_NULL;
}

#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
struct NAN_FLOW_CTRL *nanSchedGetPeerSchRecFlowCtrl(struct ADAPTER *prAdapter,
						    uint32_t u4SchId)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;

	prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, u4SchId);
	return prPeerSchRec->rNanFlowCtrlRecord;
}
#endif


#if (CFG_NAN_SCHEDULER_VERSION == 1)
struct _NAN_NONNAN_NETWORK_TIMELINE_T *
nanGetNonNanTimeline(struct ADAPTER *prAdapter, uint8_t ucNetIdx)
{
	if (ucNetIdx < ARRAY_SIZE(g_arNonNanTimeline))
		return &g_arNonNanTimeline[ucNetIdx];
	else
		return &g_arNonNanTimeline[0];
}

union _NAN_BAND_CHNL_CTRL
nanQueryNonNanChnlInfoBySlot(struct ADAPTER *prAdapter,
	uint16_t u2SlotIdx, enum ENUM_BAND eBand)
{
	uint32_t u4Idx;
	struct _NAN_NONNAN_NETWORK_TIMELINE_T *prNonNanTimeline;

	for (u4Idx = 0; u4Idx < NAN_MAX_NONNAN_TIMELINE_NUM; u4Idx++) {
		prNonNanTimeline = nanGetNonNanTimeline(prAdapter, u4Idx);
		if (prNonNanTimeline->u4SlotBitmap &
		    BIT(NAN_SLOT_INDEX(u2SlotIdx))) {
			if (nanRegGetNanChnlBand(prNonNanTimeline->rChnlInfo)
				== eBand)
				return prNonNanTimeline->rChnlInfo;
		}
	}

	return g_rNullChnl;
}

void nanUpdateAisBitmap(struct ADAPTER *prAdapter, u_int8_t fgSet)
{
	struct _NAN_AIS_BITMAP *prAisSlots = prAdapter->arNanAisSlots;

	if (!fgSet) {
		kalMemZero(prAisSlots, sizeof(prAdapter->arNanAisSlots));
		return;
	}

	/* SET */
	if (prAdapter->rWifiVar.fgDbDcModeEn) {
		if (!prAisSlots[NAN_2G_IDX].fgCustSet)
			prAisSlots[NAN_2G_IDX].u4Bitmap = NAN_DEFAULT_2G_AIS;
		if (!prAisSlots[NAN_5G_IDX].fgCustSet)
			prAisSlots[NAN_5G_IDX].u4Bitmap = NAN_DEFAULT_5G_AIS;
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (!prAisSlots[NAN_6G_IDX].fgCustSet)
			prAisSlots[NAN_6G_IDX].u4Bitmap = NAN_DEFAULT_6G_AIS;
#endif
	} else {
		if (!prAisSlots[NAN_2G_IDX].fgCustSet)
			prAisSlots[NAN_2G_IDX].u4Bitmap = NAN_NON_DBDC_2G_AIS;
		if (!prAisSlots[NAN_5G_IDX].fgCustSet)
			prAisSlots[NAN_5G_IDX].u4Bitmap = NAN_NON_DBDC_5G_AIS;
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (!prAisSlots[NAN_6G_IDX].fgCustSet)
			prAisSlots[NAN_6G_IDX].u4Bitmap = NAN_NON_DBDC_6G_AIS;
#endif
	}
}

static uint8_t
nanGetMockedConcurrentChannelByBand(struct ADAPTER *prAdapter,
				    enum ENUM_NETWORK_TYPE eNetworkType,
				    enum ENUM_BAND eBand,
				    union _NAN_BAND_CHNL_CTRL *prChnl,
				    uint8_t *ucPhyTypeSet)
{
	struct WIFI_VAR *prWifiVar;
	uint16_t u4Bw;
	uint8_t ucChannel;

	prWifiVar = &prAdapter->rWifiVar;
	ucChannel = prWifiVar->aucNanMockedChannel[eNetworkType][eBand];

	if (ucChannel) {
		u4Bw = nanSchedConfigGetAllowedBw(prAdapter, eBand);
		*prChnl = nanRegGenNanChnlInfoByPriChannel(ucChannel,
							   u4Bw, eBand);
		*ucPhyTypeSet = PHY_TYPE_SET_802_11ABGNACAX;
	}

	return ucChannel;
}

static uint8_t nanGetMockedConcurrentChannel(struct ADAPTER *prAdapter,
					enum ENUM_NETWORK_TYPE eNetworkType,
					size_t szTimeline,
					enum ENUM_BAND *peBand,
					union _NAN_BAND_CHNL_CTRL *prChnl,
					uint8_t *ucPhyTypeSet)
{
	const size_t sz5gTimeLineIdx =
			nanGetTimelineMgmtIndexByBand(prAdapter, BAND_5G);
	enum ENUM_BAND eBand;
	uint8_t ucChannel;

	if (szTimeline == sz5gTimeLineIdx) {
		for (eBand = BAND_5G; eBand < BAND_NUM; eBand++) {
			ucChannel = nanGetMockedConcurrentChannelByBand(
						prAdapter,
						eNetworkType, eBand,
						prChnl, ucPhyTypeSet);
			if (ucChannel) {
				*peBand = eBand;
				break;
			}
		}
	} else {
		eBand = BAND_2G4;
		*peBand = eBand;
		ucChannel = nanGetMockedConcurrentChannelByBand(prAdapter,
						eNetworkType, eBand,
						prChnl, ucPhyTypeSet);
	}

	return ucChannel;
}

/**
 * nanSchedGetConnChnlUsage() - Get connected network channel and time slots
 * @prAdapter: pointer to adapter
 * @eNetworkType: network type to query, NETWORK_TYPE_AIS or NETWORK_TYPE_P2P
 * @eBand: specify the band to query
 * @prChnl: return channel info of connected AIS BSS
 * @pu4SlotBitmap: return bitmap slots of connected AIS BSS
 *
 * Context: Check current AIS status, used to determine the non-NAN timeline.
 *
 * Return: WLAN_STATUS_SUCCESS.
 *	   WLAN_STATUS_FAILURE if no network type found or invalid arguments.
 */
uint32_t nanSchedGetConnChnlUsage(struct ADAPTER *prAdapter,
				  enum ENUM_NETWORK_TYPE eNetworkType,
				  enum ENUM_BAND eBand,
				  union _NAN_BAND_CHNL_CTRL *prChnl,
				  uint32_t *pu4SlotBitmap,
				  uint8_t *ucPhyTypeSet)
{
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t u4Bw = 0;
	uint8_t i;
	struct _NAN_AIS_BITMAP *prAisSlots;
	uint8_t band_idx;
	uint8_t ucBssCount;
	uint8_t ucChannel;

	if (!prAdapter || !prChnl || !pu4SlotBitmap ||
	    (eNetworkType != NETWORK_TYPE_AIS &&
	     eNetworkType != NETWORK_TYPE_P2P))
		return WLAN_STATUS_FAILURE;

	prChnl->u4RawData = 0;
	*pu4SlotBitmap = 0;

	ucChannel = nanGetMockedConcurrentChannelByBand(prAdapter,
							eNetworkType, eBand,
							prChnl, ucPhyTypeSet);
	if (ucChannel) {
		DBGLOG(NAN, INFO, "Use mocked test channel=%u", ucChannel);
		if (eNetworkType == NETWORK_TYPE_AIS) {
			if (eBand == BAND_2G4)
				*pu4SlotBitmap = NAN_DEFAULT_2G_AIS;
			else
				*pu4SlotBitmap = NAN_DEFAULT_5G_AIS;
		}
		goto channel_determined;
	}

	ucBssCount = prAdapter->ucSwBssIdNum;

	for (i = 0; i < ucBssCount; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if ((eNetworkType == NETWORK_TYPE_AIS &&
		     IS_BSS_ACTIVE_LINK(prAdapter, prBssInfo) &&
		     prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED ||
		     eNetworkType == NETWORK_TYPE_P2P &&
		     IS_BSS_P2P(prBssInfo)) &&
		    prBssInfo->fgIsInUse && prBssInfo->fgIsNetActive &&
		    prBssInfo->eBand == eBand) {

			/* Use NAN BW instead of max(AIS,NAN) */
			u4Bw = nanSchedConfigGetAllowedBw(prAdapter, eBand);

			*prChnl = nanRegGenNanChnlInfoByPriChannel(
				prBssInfo->ucPrimaryChannel, u4Bw, eBand);
			*ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
			break;
		}

	}
	if (i == ucBssCount) {
		DBGLOG(NAN, DEBUG, "%s band %u not in use\n",
		       apucNetworkType[eNetworkType], eBand);
		return WLAN_STATUS_FAILURE;
	}

channel_determined:
	if (eNetworkType == NETWORK_TYPE_AIS) {
		prAisSlots = prAdapter->arNanAisSlots;
		if (prChnl->u4RawData) {
			band_idx = (enum NAN_BAND_IDX)(eBand - 1);
			if (band_idx < NAN_BAND_NUM)
				*pu4SlotBitmap = prAisSlots[band_idx].u4Bitmap;
		}
	}

	if (eNetworkType == NETWORK_TYPE_P2P)
		*pu4SlotBitmap = 0xFFFFFFFF;

	DBGLOG(NAN, DEBUG,
	       "network=%c, eBand=%u, bw=%u, type=%c, BandIdMask=%u, OC=%u, primaryChnl=%u, auxChnl=%u, bitmap=0x%08x\n",
	       eNetworkType == NETWORK_TYPE_AIS ? 'A' :
	       eNetworkType == NETWORK_TYPE_P2P ? 'P' :
	       '0' + eNetworkType,
	       eBand, u4Bw,
	       prChnl->u4Type ==
		       NAN_BAND_CH_ENTRY_LIST_TYPE_BAND ?
		       'B' : 'C',
	       prChnl->u4BandIdMask,
	       prChnl->u4OperatingClass,
	       prChnl->u4PrimaryChnl,
	       prChnl->u4AuxCenterChnl,
	       *pu4SlotBitmap);

	return WLAN_STATUS_SUCCESS;
}

/**
 * nanSchedGetConnChnlUsageByTimeline() - Get connected channel by timeline
 * @prAdapter: pointer to adapter
 * @eNetworkType: network type to query, NETWORK_TYPE_AIS or NETWORK_TYPE_P2P
 * @szTimeline: specify the timeline to query
 * @prChnl: return channel info of connected AIS BSS
 * @pu4SlotBitmap: return bitmap slots of connected AIS BSS
 *
 * Context: Check current AIS status, used to determine the non-NAN timeline.
 *
 * Return: WLAN_STATUS_SUCCESS.
 *	   WLAN_STATUS_FAILURE if no network type found or invalid arguments.
 */
uint32_t nanSchedGetConnChnlUsageByTimeline(struct ADAPTER *prAdapter,
					    enum ENUM_NETWORK_TYPE eNetworkType,
					    size_t szTimeline,
					    union _NAN_BAND_CHNL_CTRL *prChnl,
					    uint32_t *pu4SlotBitmap,
					    uint8_t *ucPhyTypeSet)
{
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t u4Bw = 0;
	uint8_t i;
	struct _NAN_AIS_BITMAP *prAisSlots;
	uint8_t band_idx;
	uint8_t ucBssCount;
	uint8_t ucChannel;
	enum ENUM_BAND eBand = BAND_NULL;

	if (!prAdapter || !prChnl || !pu4SlotBitmap ||
	    (eNetworkType != NETWORK_TYPE_AIS &&
	     eNetworkType != NETWORK_TYPE_P2P))
		return WLAN_STATUS_FAILURE;

	prChnl->u4RawData = 0;
	*pu4SlotBitmap = 0;

	ucChannel = nanGetMockedConcurrentChannel(prAdapter, eNetworkType,
						  szTimeline, &eBand,
						  prChnl, ucPhyTypeSet);
	if (ucChannel) {
		DBGLOG(NAN, INFO, "Use mocked test chanenl %u", ucChannel);
		if (eNetworkType == NETWORK_TYPE_AIS) {
			if (NAN_IS_2G_TIMELINE(prAdapter, szTimeline))
				*pu4SlotBitmap = NAN_DEFAULT_2G_AIS;
			else
				*pu4SlotBitmap = NAN_DEFAULT_5G_AIS;
		}
		goto channel_determined;
	}

	ucBssCount = prAdapter->ucSwBssIdNum;

	for (i = 0; i < ucBssCount; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if ((eNetworkType == NETWORK_TYPE_AIS &&
		     IS_BSS_ACTIVE_LINK(prAdapter, prBssInfo) &&
		     kalGetMediaStateIndicated(prAdapter->prGlueInfo,
					       prBssInfo->ucBssIndex) ==
			     MEDIA_STATE_CONNECTED ||
		     eNetworkType == NETWORK_TYPE_P2P &&
		     IS_BSS_P2P(prBssInfo)) &&
		    prBssInfo->fgIsInUse && prBssInfo->fgIsNetActive &&
		    NAN_IS_TIMELINE_MATCH_BAND(prAdapter, szTimeline,
					       prBssInfo->eBand)) {
			eBand = prBssInfo->eBand;
			/* Use NAN BW instead of max(AIS,NAN) */
			u4Bw = nanSchedConfigGetAllowedBw(prAdapter, eBand);

			*prChnl = nanRegGenNanChnlInfoByPriChannel(
				prBssInfo->ucPrimaryChannel, u4Bw, eBand);
			*ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
			break;
		}

	}
	if (i == ucBssCount) {
		DBGLOG(NAN, DEBUG, "Timeline=%zu, network %u(%s) not in use\n",
		       szTimeline, eNetworkType, apucNetworkType[eNetworkType]);
		return WLAN_STATUS_FAILURE;
	}

channel_determined:
	if (eNetworkType == NETWORK_TYPE_AIS) {
		prAisSlots = prAdapter->arNanAisSlots;
		if (prChnl->u4RawData) {
			band_idx = (enum NAN_BAND_IDX)(eBand - 1);
			if (band_idx < NAN_BAND_NUM)
				*pu4SlotBitmap = prAisSlots[band_idx].u4Bitmap;
		}
	}

	if (eNetworkType == NETWORK_TYPE_P2P)
		*pu4SlotBitmap = 0xFFFFFFFF;

	DBGLOG(NAN, DEBUG,
	       "Timeline=%zu, network=%c, eBand=%u, bw=%u, type=%c, BandIdMask=%u, OC=%u, primaryChnl=%u, auxChnl=%u, bitmap=0x%08x\n",
	       szTimeline,
	       eNetworkType == NETWORK_TYPE_AIS ? 'A' :
	       eNetworkType == NETWORK_TYPE_P2P ? 'P' :
	       '0' + eNetworkType,
	       eBand, u4Bw,
	       prChnl->u4Type ==
		       NAN_BAND_CH_ENTRY_LIST_TYPE_BAND ?
		       'B' : 'C',
	       prChnl->u4BandIdMask,
	       prChnl->u4OperatingClass,
	       prChnl->u4PrimaryChnl,
	       prChnl->u4AuxCenterChnl,
	       *pu4SlotBitmap);

	return WLAN_STATUS_SUCCESS;
}

uint8_t nanSchedGetConnBands(struct ADAPTER *prAdapter,
			     enum ENUM_NETWORK_TYPE eNetworkType)
{
	enum ENUM_BAND eBand;
	uint8_t ucBandBitmap = 0;

	union _NAN_BAND_CHNL_CTRL prChnl;
	uint32_t pu4SlotBitmap;
	uint8_t ucPhyTypeSet;

	for (eBand = BAND_2G4; eBand < BAND_NUM; eBand++) {
		if (nanSchedGetConnChnlUsage(prAdapter, eNetworkType, eBand,
					     &prChnl, &pu4SlotBitmap,
					     &ucPhyTypeSet) ==
						WLAN_STATUS_SUCCESS)
			ucBandBitmap |= BIT(eBand);
	}

	return ucBandBitmap;
}

uint32_t nanSchedUpdateNonNanTimelineByAis(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	enum ENUM_BAND eBand;
	union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};
	uint32_t u4SlotBitmap = 0;
	uint8_t ucPhyTypeSet;
	struct _NAN_NONNAN_NETWORK_TIMELINE_T *prNonNanTimeline;
	struct _NAN_SCHEDULER_T *prNanScheduler;

	ASSERT(prAdapter);
	prNanScheduler = nanGetScheduler(prAdapter);

	if (prNanScheduler->fgInit == FALSE) {
		DBGLOG(NAN, WARN, "NAN not init\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	for (eBand = BAND_2G4; eBand < BAND_NUM; eBand++) {
		if (eBand - 1 < NAN_MAX_NONNAN_TIMELINE_NUM) {
			prNonNanTimeline = nanGetNonNanTimeline(prAdapter,
								eBand - 1);
		} else { /* 6G overwrites 5G? */
			prNonNanTimeline = nanGetNonNanTimeline(prAdapter,
					  NAN_MAX_NONNAN_TIMELINE_NUM - 1);
		}
		nanSchedGetConnChnlUsage(prAdapter, NETWORK_TYPE_AIS, eBand,
					 &rChnlInfo, &u4SlotBitmap,
					 &ucPhyTypeSet);

		DBGLOG(NAN, DEBUG,
			"AIS chnlRaw:0x%08x, PrimCh:%d, bitmap:%08x\n",
			rChnlInfo.u4RawData, rChnlInfo.u4PrimaryChnl,
			u4SlotBitmap);

		/* Update Non-NAN timeline */
		if (rChnlInfo.u4PrimaryChnl == 0) {
			prNonNanTimeline->rChnlInfo.u4RawData = 0;
			prNonNanTimeline->u4SlotBitmap = 0;
		} else {
			prNonNanTimeline->rChnlInfo = rChnlInfo;
			prNonNanTimeline->u4SlotBitmap = u4SlotBitmap;
		}
	}

	return rRetStatus;
}

u_int8_t nanIsDbdcAllowed(enum ENUM_BAND eNanBand, enum ENUM_BAND eNonNanBand)
{
	/* If AIS & NAN are under different (2.4G vs. 5G) band
	 * won't affect current timeline
	 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eNanBand == BAND_5G && eNonNanBand == BAND_6G)
		return FALSE;
#endif

	if (eNonNanBand != eNanBand)
		return TRUE;

	return FALSE;
}

uint32_t nanSchedCommitNonNanChnlList(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SlotIdx, u4SlotOffset, u4DwIdx;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_NONNAN_NETWORK_TIMELINE_T *prNonNanTimeline;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList;
	union _NAN_BAND_CHNL_CTRL rNonNanChnlInfo, rCommitChnlInfo;
	enum ENUM_BAND eNonNanBand, eNanBand = BAND_NULL;
	uint32_t u4ChnlIdx;
	size_t szNonNanTimeLineIdx;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	uint32_t u4CommitPrimaryChnl;

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);

		for (szNonNanTimeLineIdx = 0;
			szNonNanTimeLineIdx < NAN_MAX_NONNAN_TIMELINE_NUM;
			szNonNanTimeLineIdx++) {

			prNonNanTimeline = nanGetNonNanTimeline(prAdapter,
				szNonNanTimeLineIdx);
			rNonNanChnlInfo = prNonNanTimeline->rChnlInfo;
			eNonNanBand = nanRegGetNanChnlBand(rNonNanChnlInfo);

			DBGLOG(NAN, DEBUG, "Non-NAN band=%u, chnl=%u\n",
			       eNonNanBand,
			       rNonNanChnlInfo.u4PrimaryChnl);

			if (rNonNanChnlInfo.u4PrimaryChnl == 0) {
				DBGLOG(NAN, DEBUG, "Non-NAN channel = 0\n");
				return WLAN_STATUS_NOT_ACCEPTED;
			}

			prChnlTimelineList = prNanTimelineMgmt->arChnlList;

			for (u4ChnlIdx = 0;
				u4ChnlIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
				u4ChnlIdx++) {
				union _NAN_BAND_CHNL_CTRL *prChnlInfo;

				if (prChnlTimelineList[u4ChnlIdx].fgValid
					== FALSE)
					continue;

				prChnlInfo = &prChnlTimelineList[u4ChnlIdx]
						.rChnlInfo;
				eNanBand = nanRegGetNanChnlBand(*prChnlInfo);
				DBGLOG(NAN, DEBUG,
					"NAN chnlIdx=%u, band=%u, chnl=%u\n",
					u4ChnlIdx, eNanBand,
					prChnlInfo->u4PrimaryChnl);
			}

			DBGLOG(NAN, DEBUG, "NAN band=%u, Non-NAN band=%u\n",
			       eNanBand,  eNonNanBand);
			if (prAdapter->rWifiVar.fgDbDcModeEn) {
				/* If AIS & NAN are under different band
				 * won't affect current timeline
				 */
				if (eNanBand != BAND_NULL &&
				    nanIsDbdcAllowed(eNanBand, eNonNanBand)) {
					DBGLOG(NAN, DEBUG,
						"Skip. NDP B%d != AIS B%d\n",
						eNanBand, eNonNanBand);
					return WLAN_STATUS_NOT_ACCEPTED;
				}

				/* Skip if NDP fix channel on 2.4G, and
				 * AIS operated under 5G
				 */
				if (nanRegGetNanChnlBand
					(nanSchedGetFixedChnlInfo(prAdapter))
					== BAND_2G4) {
					if (eNonNanBand != BAND_2G4) {
						DBGLOG(NAN, DEBUG,
						"Skip. NDP fixed on 2.4G, AIS use 5G\n");
						return WLAN_STATUS_NOT_ACCEPTED;
					}
				} else if (eNonNanBand == BAND_2G4) {
					/* Skip if NDP not fix channel at 2.4G
					 * but AIS operated under 2.4G
					 */
					DBGLOG(NAN, DEBUG,
						"Skip. NDP Null, AIS in 2G\n");
					return WLAN_STATUS_NOT_ACCEPTED;
				}
			}

			for (u4SlotOffset = 0;
				u4SlotOffset < NAN_SLOTS_PER_DW_INTERVAL;
				u4SlotOffset++) {

				/* Skip if the slot is not used by
				 * non-NAN network
				 */
				if ((prNonNanTimeline->u4SlotBitmap &
					BIT(u4SlotOffset)) == 0)
					continue;

				for (u4DwIdx = 0;
					u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
					u4SlotIdx = NAN_FULL_SLOT_INDEX(u4DwIdx,
								u4SlotOffset);

					/* Get committed channel */
					rCommitChnlInfo =
						nanQueryChnlInfoBySlot(
							prAdapter, u4SlotIdx,
							NULL, TRUE,
							szTimeLineIdx);
					u4CommitPrimaryChnl =
						rCommitChnlInfo.u4PrimaryChnl;

					if (u4CommitPrimaryChnl != 0 &&
					    nanSchedChkConcurrOp(
						rCommitChnlInfo,
						rNonNanChnlInfo)
						== CNM_CH_CONCURR_MCC) {
						/* If concurrent check fail,
						 * remove original channel,
						 * and add new channel
						 * If check pass, use original
						 * channel
						 */
						nanSchedDeleteCrbFromChnlList(
							prAdapter,
							u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
							TRUE, szTimeLineIdx);
							/* AIS slots marked
							 * here
							 */
						nanSchedAddCrbToChnlList(
							prAdapter,
							&rNonNanChnlInfo,
							u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
							TRUE, NULL);
					} else if (u4CommitPrimaryChnl == 0)
						nanSchedAddCrbToChnlList(
							prAdapter,
							&rNonNanChnlInfo,
							u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
							TRUE, NULL);
				}
			}
		}
	}

	return rRetStatus;
}

uint32_t nanSchedSyncNonNanChnlToNan(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SchIdx;

	ASSERT(prAdapter);

	/* step1: Add AIS channel to committed slots */
	rRetStatus = nanSchedCommitNonNanChnlList(prAdapter);
	if (rRetStatus != WLAN_STATUS_SUCCESS)
		return rRetStatus;

	/* step2: determine common FAW CRB */
	for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++)
		nanSchedPeerUpdateCommonFAW(prAdapter, u4SchIdx);

	/* step3: release unused slots */
	nanSchedReleaseUnusedCommitSlot(prAdapter);

	/* Update local timeline to firmware */
	nanSchedCmdUpdateAvailability(prAdapter);

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

	return rRetStatus;
}
#endif /* (CFG_NAN_SCHEDULER_VERSION == 1) */

#if (CFG_SUPPORT_NAN_NDP_DUAL_BAND == 0)
uint32_t nanSchedRemoveDiffBandChnlList(struct ADAPTER *prAdapter)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4SlotIdx;
	uint32_t u4ChnlIdx;
	enum ENUM_BAND eBand, eSelBand = BAND_NULL;
	unsigned char fgNeedRemove = FALSE;
	int32_t i4SlotNum[BAND_NUM - 1] = {0};
	union _NAN_BAND_CHNL_CTRL rChnlInfo;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList;
	unsigned char fgCommitOrCond;
	size_t szTimeLineIdx;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	/* Predefined priority: (6G >) 5G > 2G */
#if (CFG_SUPPORT_NAN_6G == 1)
	uint32_t u4BandStart = BAND_6G;
#else
	uint32_t u4BandStart = BAND_5G;
#endif

	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
			szTimeLineIdx);

		for (fgCommitOrCond = 0;
			fgCommitOrCond <= 1; fgCommitOrCond++) {
			if (fgCommitOrCond)
				prChnlTimelineList =
					prNanTimelineMgmt->arChnlList;
			else
				prChnlTimelineList =
					prNanTimelineMgmt->arCondChnlList;

			for (u4ChnlIdx = 0;
				u4ChnlIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
				u4ChnlIdx++) {
				if (prChnlTimelineList[u4ChnlIdx].fgValid
					== FALSE)
					continue;

				eBand = nanRegGetNanChnlBand(
					prChnlTimelineList[u4ChnlIdx]
					.rChnlInfo);
				if (eBand != BAND_NULL)
					i4SlotNum[eBand - 1] +=
					prChnlTimelineList[u4ChnlIdx].i4Num;
			}
		}

		/* Predefined priority: (6G >) 5G > 2G */
		for (eBand = u4BandStart; eBand > 0; eBand--) {
			if (i4SlotNum[eBand - 1] == 0)
				continue;

			if (eSelBand == BAND_NULL)
				eSelBand = eBand;

			if (eBand != eSelBand)
				fgNeedRemove = TRUE;

			DBGLOG(NAN, DEBUG,
				"eSelBand:%d, Band:%d, SlotNum:%d, NeedRemove:%d, fgCommitOrCond:%d\n",
				eSelBand, eBand, i4SlotNum[eBand-1],
				fgNeedRemove, fgCommitOrCond);
		}

		if (fgNeedRemove == FALSE)
			return rRetStatus;

		for (fgCommitOrCond = 0;
			fgCommitOrCond <= 1; fgCommitOrCond++) {
			for (u4SlotIdx = 0;
				u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS;
				u4SlotIdx++) {
				rChnlInfo =
					nanQueryChnlInfoBySlot(
					prAdapter, u4SlotIdx, NULL,
					fgCommitOrCond, szTimeLineIdx);
				if (rChnlInfo.u4PrimaryChnl == 0)
					continue;
				eBand = nanRegGetNanChnlBand(rChnlInfo);

				if (eBand != eSelBand)
					nanSchedDeleteCrbFromChnlList(prAdapter,
						u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
						fgCommitOrCond, szTimeLineIdx);
			}
		}
	}
	return rRetStatus;
}
#endif /* (CFG_SUPPORT_NAN_NDP_DUAL_BAND == 0) */

uint32_t nanSchedUpdateCustomCommands(struct ADAPTER *prAdapter,
				      const uint8_t *buf, uint32_t len)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement;
	uint8_t *prCmdUpdateAscc;
	uint32_t u4BufSize = ALIGN_4(len);


	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 /* A whole Custom command in aucbody */
			 u4BufSize;

	DBGLOG(NAN, TRACE, "allocate %u bytes for %u, align4=%u\n",
	       u4CmdBufferLen, len, u4BufSize);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicNanAddNewTlvElement(NAN_CMD_EXT_CUSTOM_CMD,
				      u4BufSize, u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicNanGetTargetTlvElement(prTlvCommon->u2TotalElementNum,
					      prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(NAN, TRACE, "element tag=%u, body_len=%u\n",
	       prTlvElement->tag_type, prTlvElement->body_len);
	prCmdUpdateAscc = (uint8_t *)prTlvElement->aucbody;
	kalMemZero(prCmdUpdateAscc, u4BufSize);


	/* TODO: get from saved command string */
	memcpy(prCmdUpdateAscc, buf, len);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;
}


/* BIT maps containing the affected timeslots to be examined */
#define SLOT_MASK_TYPE_AIS 0x00FF00FF /* 0~7, 16~23 */
#define SLOT_MASK_TYPE_NDL 0xFF000F00 /* 12~15, 24~31 */

enum TIMELINE_INDEX {
	TIMELINE_BAND_2G4,
	TIMELINE_BAND_5G6G,
	TIMELINE_NUM,
};

/* Return the number of active (not in IDLE) NDL. */
uint32_t nanGetActiveNdlNum(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4ActiveNdlNum = 0;
	uint32_t i;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		prNDL = &prDataPathInfo->arNDL[i];
		if (prNDL->fgNDLValid &&
		    prNDL->eCurrentNDLMgmtState != NDL_IDLE)
			u4ActiveNdlNum++;
	}

	return u4ActiveNdlNum;
}

/* Return the number of established NDL. */
uint32_t nanGetEstablishedNdlNum(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4ActiveNdlNum = 0;
	uint32_t i;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		prNDL = &prDataPathInfo->arNDL[i];
		if (prNDL->fgNDLValid &&
		    prNDL->eCurrentNDLMgmtState == NDL_SCHEDULE_ESTABLISHED)
			u4ActiveNdlNum++;
	}

	return u4ActiveNdlNum;
}

uint32_t nanGetValidNdlNum(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	return prDataPathInfo->ucNDLNum;
}

/**
 * Return whether rChnlInfo is in DFS.
 * DFS channels: 52~144
 */
static u_int8_t isDfs(struct ADAPTER *prAdapter,
		      union _NAN_BAND_CHNL_CTRL *prChnlInfo)
{
	uint32_t u4PrimaryChnl = prChnlInfo->u4PrimaryChnl;

	return rlmDomainIsDfsChnls(prAdapter, u4PrimaryChnl);
}

/* Return TRUE if rLocalChnlInfo != rAisChnlInfo */
static u_int8_t isAisConflictNan(union _NAN_BAND_CHNL_CTRL rLocalChnlInfo,
				 union _NAN_BAND_CHNL_CTRL rAisChnlInfo)
{
	return (rLocalChnlInfo.u4OperatingClass !=
		rAisChnlInfo.u4OperatingClass ||
		rLocalChnlInfo.u4PrimaryChnl !=
		rAisChnlInfo.u4PrimaryChnl);
}

/* Return TRUE if local channel not in used */
static u_int8_t isLocalNotInUse(union _NAN_BAND_CHNL_CTRL rLocalChnlInfo,
				union _NAN_BAND_CHNL_CTRL rChnlInfo)
{
	UNUSED(rChnlInfo);

	return (rLocalChnlInfo.u4RawData == 0);
}

/**
 * Find out the maximum capability of all peers
 * szSlotIdx: control log output
 * *eAllMaxBand = min(max(peer capability, i))
 * *eAnyMaxBand = max(max(peer capability, i))
 * *eAllMaxABand = min(max(peer capability, i where i is not 2G only device))
 *
 * *ucAllPeerMaxPhy = bitmap 'AND' result of all NAN peer phy set
 */
static void nanGetMaxCapabilityAllPeers(struct ADAPTER *prAdapter,
					size_t szSlotIdx,
					enum ENUM_BAND *eAllMaxBand,
					enum ENUM_BAND *eAnyMaxBand,
					enum ENUM_BAND *eAllMaxABand,
					uint8_t *ucAllPeerMaxPhy)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_DEVICE_CAPABILITY_T *prDevCapList;
	const uint8_t uc2Gonly = BIT(NAN_SUPPORTED_BAND_ID_2P4G);
	uint32_t i;
	uint32_t j;
	uint32_t ucIdx;
	/**
	 * ucAllMaxSupportedBands: intersection of all device/bands
	 * ucAnyMaxSupportedBands: union of all device/bands
	 * Example:
	 *   if ucAllMaxSupportedBands & 6G: all devices support 6G
	 *   if ucAnyMaxSupportedBands & 6G: at least one device support 6G
	 */
	uint8_t ucAllMaxSupportedBands = BIT(NAN_SUPPORTED_BAND_ID_2P4G) |
					 BIT(NAN_SUPPORTED_BAND_ID_5G) |
					 BIT(NAN_SUPPORTED_BAND_ID_6G);
	uint8_t ucAnyMaxSupportedBands = BIT(NAN_SUPPORTED_BAND_ID_2P4G);
	uint8_t ucAllMaxSupportedABands = BIT(NAN_SUPPORTED_BAND_ID_5G) |
					  BIT(NAN_SUPPORTED_BAND_ID_6G);

	*ucAllPeerMaxPhy = PHY_TYPE_SET_802_11ABGNACAXBE;

	for (i = 0; i < NAN_MAX_CONN_CFG; i++) {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, i);
		if (!prPeerSchRecord || !prPeerSchRecord->fgActive ||
		    !prPeerSchRecord->prPeerSchDesc)
			continue;

		prDevCapList = prPeerSchRecord->prPeerSchDesc->arDevCapability;
		for (j = 0; j < NAN_NUM_AVAIL_DB + 1; j++, prDevCapList++) {
			if (!prDevCapList->fgValid)
				continue;

			ucAllMaxSupportedBands &= prDevCapList->ucSupportedBand;
			ucAnyMaxSupportedBands |= prDevCapList->ucSupportedBand;

			if (prDevCapList->ucSupportedBand == uc2Gonly)
				continue;

			/* Do not consider 2G only device for A-band */
			ucAllMaxSupportedABands &=
				prDevCapList->ucSupportedBand;
		}
	}

	for (ucIdx = 0; ucIdx < CFG_STA_REC_NUM; ucIdx++) {
		struct STA_RECORD *prStaRec;

		prStaRec = cnmGetStaRecByIndex(prAdapter, ucIdx);
		if (!prStaRec || !prStaRec->fgIsInUse)
			continue;

		if (!IS_STA_NAN_TYPE(prStaRec))
			continue;

		*ucAllPeerMaxPhy &= prStaRec->ucPhyTypeSet;
	}

	*eAllMaxBand = getMaxBand(ucAllMaxSupportedBands);
	*eAnyMaxBand = getMaxBand(ucAnyMaxSupportedBands);
	*eAllMaxABand = getMaxBand(ucAllMaxSupportedABands);

	NAN_DW_DBGLOG(NAN, DEBUG, TRUE, szSlotIdx,
		      "slot=%zu, eAllMaxBand=%u || eAnyMaxBand=%u, eAllMaxABand=%u, ucAllPeerMaxPhy=%u\n",
		      szSlotIdx, *eAllMaxBand, *eAnyMaxBand,
		      *eAllMaxABand, *ucAllPeerMaxPhy);
}

/* Check whether any peer device supports only 2.4G band exists */
u_int8_t nanCheck2gOnlyPeerExists(struct ADAPTER *prAdapter)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_DEVICE_CAPABILITY_T *prDevCapList;
	uint32_t i;
	uint32_t j;
	const uint8_t uc2Gonly = BIT(NAN_SUPPORTED_BAND_ID_2P4G);

	for (i = 0; i < NAN_MAX_CONN_CFG; i++) {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, i);
		if (!prPeerSchRecord || !prPeerSchRecord->fgActive ||
		    !prPeerSchRecord->prPeerSchDesc)
			continue;

		prDevCapList = prPeerSchRecord->prPeerSchDesc->arDevCapability;
		for (j = 0; j < NAN_NUM_AVAIL_DB + 1; j++, prDevCapList++) {
			if (!prDevCapList->fgValid)
				continue;

			if (prDevCapList->ucSupportedBand == uc2Gonly)
				return TRUE;
		}
	}
	return FALSE;
}

static
u_int8_t isLocalInUseAndAllPeerHigher(struct ADAPTER *prAdapter,
				      union _NAN_BAND_CHNL_CTRL rLocalChnlInfo,
				      enum ENUM_BAND ePeerCap)
{
	enum ENUM_BAND eLocalBand;

	if (rLocalChnlInfo.u4RawData != 0) {
		eLocalBand = nanRegGetNanChnlBand(rLocalChnlInfo);

		return (ePeerCap > eLocalBand);
	}

	return FALSE;
}

static u_int8_t isLocalInUseNotEhtBw320(struct ADAPTER *prAdapter,
				   union _NAN_BAND_CHNL_CTRL rLocalChnlInfo,
				   enum ENUM_BAND ePeerCap)
{
	UNUSED(ePeerCap);

	if (rLocalChnlInfo.u4RawData != 0)
		return !nanRegNanChnlBandIsEht(rLocalChnlInfo);

	return FALSE;
}

/* eNewPeerCap < current channel band */
static
u_int8_t isNewCapabilityLower(struct ADAPTER *prAdapter,
			      union _NAN_BAND_CHNL_CTRL rLocalChnlInfo,
			      enum ENUM_BAND eNewPeerCap)
{
	enum ENUM_BAND eLocalBand;

	eLocalBand = nanRegGetNanChnlBand(rLocalChnlInfo);

	return (eNewPeerCap < eLocalBand);
}

static
u_int8_t nanNeedRescheduleByChannel(struct ADAPTER *prAdapter,
			      uint32_t slot_mask,
			      union _NAN_BAND_CHNL_CTRL rAisChnlInfo,
			      enum TIMELINE_INDEX eTimelineIdx,
			      u_int8_t (*matchFunc)(union _NAN_BAND_CHNL_CTRL,
						    union _NAN_BAND_CHNL_CTRL))
{
	uint32_t i;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;

	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1)
		eTimelineIdx = 0;

	for (i = 0; i < NAN_TOTAL_SLOT_WINDOWS; i++) {
		if ((BIT(NAN_SLOT_INDEX(i)) & slot_mask) == 0)
			continue;

		rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
							/* TODO: eTimelineIdx */
							i, NULL, TRUE,
							eTimelineIdx);
		if (matchFunc(rLocalChnlInfo, rAisChnlInfo))
			return TRUE;
	}

	return FALSE;
}

/**
 * for each slots, check the channel usage, and check peer's minimum capability.
 */
static u_int8_t nanNeedRescheduleByCapability(struct ADAPTER *prAdapter,
				uint32_t slot_mask,
				enum TIMELINE_INDEX eTimelineIdx,
				enum ENUM_BAND ePeerCap,
				u_int8_t (*checkCap)(struct ADAPTER*,
						     union _NAN_BAND_CHNL_CTRL,
						     enum ENUM_BAND))
{
	uint32_t i;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo;

	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1)
		eTimelineIdx = 0;

	for (i = 0; i < NAN_TOTAL_SLOT_WINDOWS; i++) {
		if ((BIT(NAN_SLOT_INDEX(i)) & slot_mask) == 0)
			continue;

		rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
							i, NULL, TRUE,
							eTimelineIdx);
		NAN_DW_DBGLOG(NAN, LOUD, TRUE, i,
			      "[%u] local=%u, peer=%u, result=%u\n",
			      i, nanRegGetNanChnlBand(rLocalChnlInfo),
			      ePeerCap,
			      checkCap(prAdapter, rLocalChnlInfo, ePeerCap));

		if (checkCap(prAdapter, rLocalChnlInfo, ePeerCap)) {
			DBGLOG(NAN, TRACE, "return TRUE\n");
			return TRUE;
		}
	}

	DBGLOG(NAN, TRACE, "return FALSE\n");
	return FALSE;
}

/**
 * for each slots, check the channel usage, and check timeline channel info
 */
static u_int8_t nanNeedRescheduleByChannelDiff(struct ADAPTER *prAdapter,
			    uint32_t slot_mask,
			    enum TIMELINE_INDEX eTimelineIdx,
			    const union _NAN_BAND_CHNL_CTRL *prTargetChnlInfo)
{
	uint32_t i;
	union _NAN_BAND_CHNL_CTRL rCurrentChnlInfo;

	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1)
		eTimelineIdx = 0;

	for (i = 0; i < NAN_TOTAL_SLOT_WINDOWS; i++) {
		if ((BIT(NAN_SLOT_INDEX(i)) & slot_mask) == 0)
			continue;

		rCurrentChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
							i, NULL, TRUE,
							eTimelineIdx);
		NAN_DW_DBGLOG(NAN, LOUD, TRUE, i,
			      "[%u] current=%u, target=%u\n",
			      i, rCurrentChnlInfo.u4PrimaryChnl,
			      prTargetChnlInfo->u4PrimaryChnl);

		if (rCurrentChnlInfo.u4PrimaryChnl !=
		    prTargetChnlInfo->u4PrimaryChnl ||
		    rCurrentChnlInfo.u4OperatingClass !=
		    prTargetChnlInfo->u4OperatingClass ||
		    rCurrentChnlInfo.u4Type !=
		    prTargetChnlInfo->u4Type) {
			DBGLOG(NAN, DEBUG, "return TRUE at slot %u %u!=%u\n",
			       i, rCurrentChnlInfo.u4PrimaryChnl,
			       prTargetChnlInfo->u4PrimaryChnl);
			return TRUE;
		}
	}

	DBGLOG(NAN, DEBUG, "return FALSE\n");
	return FALSE;
}

static void nanDumpNDL(struct ADAPTER *prAdapter)
{

	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t i;

	static const char * const ndl_state[] = {
		[NDL_IDLE] = "NDL_IDLE",
		[NDL_REQUEST_SCHEDULE_NDP] = "NDL_REQUEST_SCHEDULE_NDP",
		[NDL_REQUEST_SCHEDULE_NDL] = "NDL_REQUEST_SCHEDULE_NDL",
		[NDL_SCHEDULE_SETUP] = "NDL_SCHEDULE_SETUP",
		[NDL_INITIATOR_TX_SCHEDULE_REQUEST] =
			"NDL_INITIATOR_TX_SCHEDULE_REQUEST",
		[NDL_RESPONDER_RX_SCHEDULE_REQUEST] =
			"NDL_RESPONDER_RX_SCHEDULE_REQUEST",
		[NDL_RESPONDER_TX_SCHEDULE_RESPONSE] =
			"NDL_RESPONDER_TX_SCHEDULE_RESPONSE",
		[NDL_INITIATOR_RX_SCHEDULE_RESPONSE] =
			"NDL_INITIATOR_RX_SCHEDULE_RESPONSE",
		[NDL_INITIATOR_TX_SCHEDULE_CONFIRM] =
			"NDL_INITIATOR_TX_SCHEDULE_CONFIRM",
		[NDL_RESPONDER_RX_SCHEDULE_CONFIRM] =
			"NDL_RESPONDER_RX_SCHEDULE_CONFIRM",
		[NDL_SCHEDULE_ESTABLISHED] = "NDL_SCHEDULE_ESTABLISHED",
		[NDL_TEARDOWN_BY_NDP_TERMINATION] =
			"NDL_TEARDOWN_BY_NDP_TERMINATION",
		[NDL_TEARDOWN] = "NDL_TEARDOWN",
		[NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE] =
			"NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE",
	};

	prDataPathInfo = &prAdapter->rDataPathInfo;

	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		prNDL = &prDataPathInfo->arNDL[i];
		if (!prNDL->fgNDLValid)
			continue;

		DBGLOG(NAN, DEBUG, "NDL[%u]: valid=%u state=%u(%s)\n",
		       i, prNDL->fgNDLValid, prNDL->eCurrentNDLMgmtState,
		       prNDL->eCurrentNDLMgmtState < ARRAY_SIZE(ndl_state) ?
		       ndl_state[prNDL->eCurrentNDLMgmtState] : "");
	}
}

/* Used when only one NDL active */
static u_int8_t nanGetPeerCommitted(struct ADAPTER *prAdapter,
				    uint8_t ucTimeLineIdx,
				    uint32_t *committed)
{
	uint32_t u4SchIdx;
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	uint32_t u4BitCount = 0;

	for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
		if (!prPeerSchRecord->fgActive)
			continue;

		prTimeline = &prPeerSchRecord->arCommFawTimeline[ucTimeLineIdx];
		if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		u4BitCount = nanUtilCheckBitOneCnt(prTimeline->au4AvailMap,
						   sizeof(uint32_t));

		DBGLOG(NAN, DEBUG,
		       "u4SchIdx=%u, u4BitCount=%u, bitmap=%02x-%02x-%02x-%02x\n",
		       u4SchIdx, u4BitCount,
		       ((uint8_t *)prTimeline->au4AvailMap)[0],
		       ((uint8_t *)prTimeline->au4AvailMap)[1],
		       ((uint8_t *)prTimeline->au4AvailMap)[2],
		       ((uint8_t *)prTimeline->au4AvailMap)[3]);

		/* only one NDL active */
		if (committed)
			*committed = prTimeline->au4AvailMap[0];
		return TRUE;
	}

	return FALSE;
}

static u_int8_t nanCountryCodeConflict(struct ADAPTER *prAdapter)
{
	return FALSE; /* TODO */
}

/* Save and to be retrieved by nanIsP2pAisMCC() */
void nanSchedUpdateP2pAisMcc(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prScheduler;
	union _NAN_BAND_CHNL_CTRL rSocialChnlInfo;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo = {0};
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo = {0};
	size_t szTimeline;
	uint32_t u4SlotBitmap;
	uint8_t ucPhyTypeSet;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct NAN_P2P_AIS_MCC_RECORD *prP2pAisMcc;

	prScheduler = nanGetScheduler(prAdapter);

	for (szTimeline = 0; szTimeline < szNanActiveTimelineNum;
	     szTimeline++) {
		prP2pAisMcc = &prScheduler->arP2pAisMcc[szTimeline];
		prP2pAisMcc->ucNumOfChannel = 1; /* NAN social channel */

		nanSchedGetConnChnlUsageByTimeline(prAdapter, NETWORK_TYPE_AIS,
						  szTimeline, &rAisChnlInfo,
						  &u4SlotBitmap, &ucPhyTypeSet);
		nanSchedGetConnChnlUsageByTimeline(prAdapter, NETWORK_TYPE_P2P,
						  szTimeline, &rP2pChnlInfo,
						  &u4SlotBitmap, &ucPhyTypeSet);

		if (rP2pChnlInfo.u4PrimaryChnl && rAisChnlInfo.u4PrimaryChnl) {
			prP2pAisMcc->fgIsP2pAisMCC =
				nanSchedChkConcurrOp(rP2pChnlInfo,
						     rAisChnlInfo) ==
						CNM_CH_CONCURR_MCC;
		} else {
			prP2pAisMcc->fgIsP2pAisMCC = FALSE;
		}

		prP2pAisMcc->rP2pChnlInfo = rP2pChnlInfo;
		prP2pAisMcc->rAisChnlInfo = rAisChnlInfo;

		if (NAN_IS_5G_TIMELINE(prAdapter, szTimeline)) {
			if (rP2pChnlInfo.u4PrimaryChnl)
				nanSetFlashCommunication(prAdapter, FALSE);
			else
				nanSetFlashCommunication(prAdapter, TRUE);
		}

		if (NAN_IS_2G_TIMELINE(prAdapter, szTimeline))
			rSocialChnlInfo = g_r2gDwChnl;
		else
			rSocialChnlInfo = g_r5gDwChnl;

		if (rP2pChnlInfo.u4PrimaryChnl &&
		    !nanChnlInfoEqual(rP2pChnlInfo, rSocialChnlInfo))
			prP2pAisMcc->ucNumOfChannel++;
		if (rAisChnlInfo.u4PrimaryChnl &&
		    !nanChnlInfoEqual(rAisChnlInfo, rSocialChnlInfo))
			prP2pAisMcc->ucNumOfChannel++;
		if (rP2pChnlInfo.u4PrimaryChnl &&
		    rAisChnlInfo.u4PrimaryChnl &&
		    nanChnlInfoEqual(rP2pChnlInfo, rAisChnlInfo))
			prP2pAisMcc->ucNumOfChannel--;

		DBGLOG(NAN, INFO,
		       "Country=%c%c, Tidx=%zu p2p=%u, ais=%u, MCC=%u, Num=%u\n",
		       kalIsPrint(prAdapter->rWifiVar.CountryCode[1]) ?
			       prAdapter->rWifiVar.CountryCode[1] : '-',
		       kalIsPrint(prAdapter->rWifiVar.CountryCode[0]) ?
			       prAdapter->rWifiVar.CountryCode[0] : '-',
		       szTimeline,
		       rP2pChnlInfo.u4PrimaryChnl, rAisChnlInfo.u4PrimaryChnl,
		       prP2pAisMcc->fgIsP2pAisMCC, prP2pAisMcc->ucNumOfChannel);
	}
	nanSetConcurrentCustomFAW(prAdapter);
	nanSchedCmdUpdatePotentialChnlList(prAdapter);
}

static u_int8_t nanIsP2pAisMCC(struct ADAPTER *prAdapter, size_t szTimeLineIdx,
			       union _NAN_BAND_CHNL_CTRL *prP2pChnlInfo,
			       union _NAN_BAND_CHNL_CTRL *prAisChnlInfo)
{
	struct _NAN_SCHEDULER_T *prScheduler;

	if (!prAdapter || !prAisChnlInfo || !prP2pChnlInfo) {
		DBGLOG(NAN, ERROR, "NULL pointer %p, %p, %p!\n",
		       prAdapter, prP2pChnlInfo, prAisChnlInfo);
		return FALSE;
	}

	prScheduler = nanGetScheduler(prAdapter);
	if (!prScheduler) {
		*prP2pChnlInfo = g_rNullChnl;
		*prAisChnlInfo = g_rNullChnl;
		return FALSE;
	}

	*prP2pChnlInfo = prScheduler->arP2pAisMcc[szTimeLineIdx].rP2pChnlInfo;
	*prAisChnlInfo = prScheduler->arP2pAisMcc[szTimeLineIdx].rAisChnlInfo;
	return prScheduler->arP2pAisMcc[szTimeLineIdx].fgIsP2pAisMCC;
}

/**
 * P2P=N, AIS=N: pure NAN, do nothing
 * P2P=Y, AIS=N: commit P2P band 1 slots
 * P2P=N, AIS=Y: commit AIS band 1 slots
 * P2P=Y, AIS=Y: If MCC: use 2.4G channel 6;
 *		 if SCC: use the operating channel in P2P slots
 */
void nanSetConcurrentCustomFAW(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prScheduler = NULL;
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo;
	/* Now only one customized items */
	struct _NAN_CUST_FAW_ENTRY *prCurrent0;
	struct _NAN_CUST_FAW_ENTRY *prCurrent1;
	struct _NAN_CUST_FAW_ENTRY rNew0 = { 0 }; /* 2.4G */
	struct _NAN_CUST_FAW_ENTRY rNew1 = { 0 }; /* 5G/6G */
	enum ENUM_BAND eBand = BAND_5G;
	uint8_t fgChanged = FALSE;
	union {
		uint32_t u4Bitmap;
		uint8_t ucBlock[4];
	} bitmap;

	prScheduler = nanGetScheduler(prAdapter);
	if (!prScheduler)
		return;
	prCurrent0 = &prScheduler->arConcurrentCust[0];
	prCurrent1 = &prScheduler->arConcurrentCust[1];

	/* For single map, only concurrent with AIS, do not set custom FAW */
	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1)
		return;

	/* As responder with MCC, need to customize in 2.4G P2P channel */
	/* 2.4G SCC */
	rP2pChnlInfo = nanGetActiveChnl(prAdapter, NETWORK_TYPE_P2P, BAND_2G4);
	if (!NAN_IS_P2P_AIS_MCC(prAdapter, BAND_2G4) &&
	    rP2pChnlInfo.u4PrimaryChnl) {
		bitmap.u4Bitmap = NAN_T0_SLOT_MASK_CONCURRENT_FULL;
		rNew0 = (struct _NAN_CUST_FAW_ENTRY){
			.pcTag = pcConcurrentTag,
			.ucOpChannel = rP2pChnlInfo.u4PrimaryChnl,
			.eBand = BAND_2G4,
			.u4Bitmap = bitmap.u4Bitmap,
		};
		DBGLOG(NAN, INFO,
		       "SCC or P2P only, set 2.4G ch=%u, %02x-%02x-%02x-%02x",
		       rP2pChnlInfo.u4PrimaryChnl,
		       bitmap.ucBlock[0], bitmap.ucBlock[1],
		       bitmap.ucBlock[2], bitmap.ucBlock[3]);
	} else {
		rNew0 = (const struct _NAN_CUST_FAW_ENTRY){0};
		DBGLOG(NAN, INFO, "Clear 2.4G concurrent customization");
	}
	if (prCurrent0->eBand != rNew0.eBand ||
	    prCurrent0->ucOpChannel != rNew0.ucOpChannel ||
	    prCurrent0->u4Bitmap != rNew0.u4Bitmap)
		fgChanged = TRUE;

	rP2pChnlInfo = nanGetActiveChnl(prAdapter, NETWORK_TYPE_P2P, BAND_5G);
	rAisChnlInfo = nanGetActiveChnl(prAdapter, NETWORK_TYPE_AIS, BAND_5G);
	if (NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G)) {
		rNew1 = (const struct _NAN_CUST_FAW_ENTRY){0};
		DBGLOG(NAN, INFO, "5G MCC");
	} else if (rP2pChnlInfo.u4PrimaryChnl) {
		/* 5G/6G SCC or P2P only */
#if (CFG_SUPPORT_NAN_6G == 1)
		if (IS_6G_OP_CLASS(rP2pChnlInfo.u4OperatingClass))
			eBand = BAND_6G;
#endif
		bitmap.u4Bitmap = NAN_T1_SLOT_MASK_CONCURRENT_FULL;
		rNew1 = (struct _NAN_CUST_FAW_ENTRY){
			.pcTag = pcConcurrentTag,
				.ucOpChannel = rP2pChnlInfo.u4PrimaryChnl,
				.eBand = eBand,
				.u4Bitmap = bitmap.u4Bitmap,
		};
		DBGLOG(NAN, INFO,
		       "SCC or P2P only, set B=%uG, ch=%u, %02x-%02x-%02x-%02x",
		       eBand == BAND_5G ? 5 : 6, rP2pChnlInfo.u4PrimaryChnl,
		       bitmap.ucBlock[0], bitmap.ucBlock[1],
		       bitmap.ucBlock[2], bitmap.ucBlock[3]);
	} else if (rAisChnlInfo.u4PrimaryChnl) {
		/* 5G/6G AIS only */
#if (CFG_SUPPORT_NAN_6G == 1)
		if (IS_6G_OP_CLASS(rAisChnlInfo.u4OperatingClass))
			eBand = BAND_6G;
#endif
		bitmap.u4Bitmap = NAN_SLOT_MASK_TYPE_AIS;
		rNew1 = (struct _NAN_CUST_FAW_ENTRY){
			.pcTag = pcConcurrentTag,
				.ucOpChannel = rAisChnlInfo.u4PrimaryChnl,
				.eBand = eBand,
				.u4Bitmap = bitmap.u4Bitmap,
		};
		DBGLOG(NAN, INFO,
		       "AIS only, set B=%uG ch=%u, %02x-%02x-%02x-%02x",
		       eBand == BAND_5G ? 5 : 6, rAisChnlInfo.u4PrimaryChnl,
		       bitmap.ucBlock[0], bitmap.ucBlock[1],
		       bitmap.ucBlock[2], bitmap.ucBlock[3]);
	}

	if (prCurrent1->eBand != rNew1.eBand ||
	    prCurrent1->ucOpChannel != rNew1.ucOpChannel ||
	    prCurrent1->u4Bitmap != rNew1.u4Bitmap)
		fgChanged = TRUE;

	if (!fgChanged) {
		DBGLOG(NAN, INFO, "Same custom channel info");
		return;
	}

	nanSchedNegoCustFawRemoveEntry(prAdapter, pcConcurrentTag);
	if (rNew0.pcTag)
		nanSchedNegoCustFawAddEntry(prAdapter, &rNew0);
	if (rNew1.pcTag)
		nanSchedNegoCustFawAddEntry(prAdapter, &rNew1);

	nanSchedNegoCustFawReconfigure(prAdapter);
	*prCurrent0 = rNew0;
	*prCurrent1 = rNew1;
}

uint8_t select6gChannel(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_WIFI_6G == 1) && (CFG_SUPPORT_NAN_6G == 1)

#if (CFG_SUPPORT_NAN_6G == 1)
	/* P2P in 5G && NDP in 6G results to MCC */
	if (nanSchedGetConnBands(prAdapter, NETWORK_TYPE_P2P) & BIT(BAND_5G))
		return FALSE;

	if (nanCountryCodeConflict(prAdapter))
		return FALSE; /* Go to use 2G */

	return g_r6gDefChnl.u4PrimaryChnl;
#else /* (CFG_SUPPORT_NAN_6G == 1) */
	return FALSE;
#endif

#else
	return FALSE;
#endif
}

static
union _NAN_BAND_CHNL_CTRL nanGetChnlInfoByBandChnl(struct ADAPTER *prAdapter,
						   enum ENUM_BAND eBand,
						   uint8_t ucPrimaryChannel)
{
	uint32_t u4Bw;
	union _NAN_BAND_CHNL_CTRL rChnl;

	u4Bw = nanSchedConfigGetAllowedBw(prAdapter, eBand);

	rChnl = nanRegGenNanChnlInfoByPriChannel(ucPrimaryChannel, u4Bw, eBand);

	return rChnl;
}

uint8_t select5gChannel(struct ADAPTER *prAdapter)
{
	union _NAN_BAND_CHNL_CTRL rP2p5gChnlInfo = g_rNullChnl;

	/* P2P in 5G && MCC with AIS, slot base -X- 50-50, don't join them */
	if (NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G))
		return FALSE;

	if (nanCountryCodeConflict(prAdapter))
		return FALSE; /* Go to use 2G */

	rP2p5gChnlInfo = nanGetActiveChnl(prAdapter, NETWORK_TYPE_P2P, BAND_5G);
	/* 5G acceptale, returning a channel number */
	if (rP2p5gChnlInfo.u4PrimaryChnl)
		return rP2p5gChnlInfo.u4PrimaryChnl;
	else /* P2P in 2G, NAN can use 5G, even MCC with AIS */
		return g_r5gDwChnl.u4PrimaryChnl;
}

uint8_t select2gChannel(struct ADAPTER *prAdapter)
{
	union _NAN_BAND_CHNL_CTRL rP2p2gChnlInfo = g_rNullChnl;
	uint32_t u4SlotBitmap = 0;
	uint8_t ucPhyTypeSet;

	nanSchedGetConnChnlUsage(prAdapter, NETWORK_TYPE_P2P, BAND_2G4,
				 &rP2p2gChnlInfo, &u4SlotBitmap,
				 &ucPhyTypeSet);

	if (rP2p2gChnlInfo.u4PrimaryChnl)
		return rP2p2gChnlInfo.u4PrimaryChnl;
	else
		return g_r2gDwChnl.u4PrimaryChnl;
}

static uint8_t sameBandUser(struct ADAPTER *prAdapter, enum ENUM_BAND eBand)
{
	uint8_t ucNetworkTypes = 0;

	if (nanSchedGetConnBands(prAdapter, NETWORK_TYPE_AIS) & BIT(eBand))
		ucNetworkTypes |= BIT(NETWORK_TYPE_AIS);

	if (nanSchedGetConnBands(prAdapter, NETWORK_TYPE_P2P) & BIT(eBand))
		ucNetworkTypes |= BIT(NETWORK_TYPE_P2P);

	return ucNetworkTypes;
}

uint32_t determineBitmap(struct ADAPTER *prAdapter, enum ENUM_BAND eBand)
{
	if (sameBandUser(prAdapter, eBand) == 0)
		return 0xffffffff;

	/* TODO: only SAP full, P2P could be half by block? */
	if (sameBandUser(prAdapter, eBand) & BIT(NETWORK_TYPE_P2P))
		return 0xffffffff; /* 0xf0f0f0f0? */
	/* TODO: P2P could be half by block?
	 * return 0xff00ff00;
	 */

	/* Only AIS+NAN in this timeline, by block */
	if (sameBandUser(prAdapter, eBand) & BIT(NETWORK_TYPE_AIS)) {
		if (eBand == BAND_2G4)
			return ~NAN_DEFAULT_2G_AIS;
		else
			return ~NAN_DEFAULT_5G_AIS;
	}

	return 0xffffffff;
}

u_int8_t nanDetermineChannelInfoAndBitmap(struct ADAPTER *prAdapter,
					  enum ENUM_BAND *eNanBand,
					  union _NAN_BAND_CHNL_CTRL *rChnlInfo,
					  uint32_t *u4Bitmap)
{
	uint8_t ucChannel;

#if (CFG_SUPPORT_NAN_6G == 1)
	ucChannel = select6gChannel(prAdapter);
	if (ucChannel) {
		*eNanBand = BAND_6G;
		*rChnlInfo = nanGetChnlInfoByBandChnl(prAdapter,
						      BAND_6G, ucChannel);
		*u4Bitmap = determineBitmap(prAdapter, BAND_6G);
		return TRUE;
	}
#endif

	ucChannel = select5gChannel(prAdapter);
	if (ucChannel) {
		*eNanBand = BAND_5G;
		*rChnlInfo = nanGetChnlInfoByBandChnl(prAdapter,
						      BAND_5G, ucChannel);
		*u4Bitmap = determineBitmap(prAdapter, BAND_5G);
		return TRUE;
	}

	ucChannel = select2gChannel(prAdapter);
	if (ucChannel) {
		*eNanBand = BAND_2G4;
		*rChnlInfo = nanGetChnlInfoByBandChnl(prAdapter,
						      BAND_2G4, ucChannel);
		*u4Bitmap = determineBitmap(prAdapter, BAND_2G4);
		return TRUE;
	}

	DBGLOG(NAN, WARN, "No proper band available, NAN=0x%02x, AIS=0x%02x\n",
	       nanSchedGetConnBands(prAdapter, NETWORK_TYPE_P2P),
	       nanSchedGetConnBands(prAdapter, NETWORK_TYPE_AIS));
	return FALSE;
}

/**
 * 1. Find the target band/channel/bitmap according to current combination
 * 2. Check if the current timeline of the bitmap/channel info match next state
 * 3. If matched, no need to reschedule, return FALSE;
 *    otherwise, return TRUE to trigger reschedule.
 */
u_int8_t nanCheckIsNeedRescheduleWithP2p(struct ADAPTER *prAdapter,
					 enum RESCHEDULE_SOURCE event,
					 uint8_t *pucPeerNmi)
{
	enum ENUM_BAND eNanBand;
	uint32_t u4Bitmap;
	union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};
	enum TIMELINE_INDEX eTimeline;

	if (!nanDetermineChannelInfoAndBitmap(prAdapter, &eNanBand, &rChnlInfo,
					      &u4Bitmap)) {
		return FALSE;
	}

	if (eNanBand == BAND_2G4)
		eTimeline = TIMELINE_BAND_2G4;
	else
		eTimeline = TIMELINE_BAND_5G6G;

	return nanNeedRescheduleByChannelDiff(prAdapter, u4Bitmap, eTimeline,
					      &rChnlInfo);

	return FALSE;
}

static u_int8_t nanIsEhtNeedReschedule(struct ADAPTER *prAdapter,
				struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc,
				uint8_t ucAllPeerMaxPhy)
{
	if ((ucAllPeerMaxPhy & PHY_TYPE_BIT_EHT) == 0)
		return FALSE;

	if (!nanIsEhtSupport(prAdapter))
		return FALSE;

	if (!prPeerSchDesc)
		return FALSE;

#if (CFG_SUPPORT_NAN_11BE == 1)
	if (!prPeerSchDesc->fgEht)
		return TRUE;
#endif

#if (CFG_SUPPORT_NAN_6G == 1)
	if (getPeerSchDescMaxCap(prPeerSchDesc) != BAND_6G)
		return TRUE;
#endif

	return FALSE;
}

static u_int8_t nanIsAisUse5G6G(struct ADAPTER *prAdapter,
				uint8_t eAisBandBitmap,
				union _NAN_BAND_CHNL_CTRL *prAisChnlInfo)
{
	if (eAisBandBitmap & BIT(BAND_5G) && !isDfs(prAdapter, prAisChnlInfo))
		return TRUE;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eAisBandBitmap & BIT(BAND_6G))
		return TRUE;
#endif

	return FALSE;
}

static u_int8_t nanIsAisAndAllPeers6G(uint8_t eAisBandBitmap,
				      enum ENUM_BAND eAllPeerMaxCap,
				      uint8_t ucAllPeerMaxPhy)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if ((eAisBandBitmap & BIT(BAND_6G)) == 0)
		return FALSE;
#endif

#if (CFG_SUPPORT_NAN_6G == 1)
	if (eAllPeerMaxCap < BAND_6G)
		return FALSE;
#endif

	if ((ucAllPeerMaxPhy & PHY_TYPE_BIT_EHT) == 0)
		return FALSE;

	return TRUE;
}

uint32_t nanGetCurrentNdlState(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_PEER_SCH_DESC_T *prSchDesc = NULL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	uint32_t u4Idx = 0;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prSchDesc = nanSchedGetPeerSchDesc(prAdapter, prNegoCtrl->u4SchIdx);
	if (prSchDesc == NULL)
		return NDL_MGMT_STATE_NUM;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDL_NUM; u4Idx++) {
		prNDL = &prDataPathInfo->arNDL[u4Idx];
		if (prNDL->fgNDLValid &&
		    EQUAL_MAC_ADDR(prNDL->aucPeerMacAddr,
				   prSchDesc->aucNmiAddr))
			return prNDL->eCurrentNDLMgmtState;
	}

	return NDL_MGMT_STATE_NUM;
}

static uint8_t nanBandCommittedFawSlotCount(struct ADAPTER *prAdapter,
			    uint32_t u4Bands,
			    uint32_t u4CheckBitmap,
			    uint8_t aucCommittedCount[ENUM_SUPPORTED_BN_NUM])
{
	uint8_t ucSlotCount = 0;
	size_t szTimeLineIdx;
	uint32_t u4Idx;
	uint32_t u4SlotIndex;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline;
	union _NAN_BAND_CHNL_CTRL rChnlInfo;

	kalMemZero(aucCommittedCount, sizeof(uint8_t[ENUM_SUPPORTED_BN_NUM]));

	for (szTimeLineIdx = szNanActiveTimelineNum; szTimeLineIdx--; ) {
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
						       szTimeLineIdx);

		for (u4Idx = 0;
		     u4Idx < ARRAY_SIZE(prNanTimelineMgmt->arChnlList);
		     u4Idx++) {
			prChnlTimeline = &prNanTimelineMgmt->arChnlList[u4Idx];

			if (!prChnlTimeline->fgValid)
				continue;

			rChnlInfo = prChnlTimeline->rChnlInfo;

#if (CFG_SUPPORT_NAN_6G == 1)
			if (NAN_IS_CHANNEL_6G(rChnlInfo) &&
			    (u4Bands & BIT(ENUM_SUPPORTED_BN_6G)) == 0)
				continue;
#endif

			if (NAN_IS_CHANNEL_5G_LOW(rChnlInfo) &&
			    (u4Bands & BIT(ENUM_SUPPORTED_BN_5G_LOW)) == 0)
				continue;

			if (NAN_IS_CHANNEL_5G_HIGH(rChnlInfo) &&
			    (u4Bands & BIT(ENUM_SUPPORTED_BN_5G_HIGH)) == 0)
				continue;

			if (NAN_IS_CHANNEL_2G(rChnlInfo) &&
			    (u4Bands & BIT(ENUM_SUPPORTED_BN_2G)) == 0)
				continue;

			for (u4SlotIndex = 0;
			     u4SlotIndex < NAN_SLOTS_PER_DW_INTERVAL;
			     u4SlotIndex++) {
				if ((BIT(u4SlotIndex) &
				     prChnlTimeline->au4AvailMap[0]) == 0)
					continue;

				if ((BIT(u4SlotIndex) & u4CheckBitmap) == 0)
					continue;

				if (NAN_SLOT_TIMELINE_IS_FC(prAdapter,
							    szTimeLineIdx,
							    u4SlotIndex))
					continue;

				aucCommittedCount[ENUM_SUPPORTED_BN_6G] +=
					NAN_IS_CHANNEL_6G(rChnlInfo);

				aucCommittedCount[ENUM_SUPPORTED_BN_5G_HIGH] +=
					NAN_IS_CHANNEL_5G_HIGH(rChnlInfo);

				aucCommittedCount[ENUM_SUPPORTED_BN_5G_LOW] +=
					NAN_IS_CHANNEL_5G_LOW(rChnlInfo);

				aucCommittedCount[ENUM_SUPPORTED_BN_2G] +=
					NAN_IS_CHANNEL_2G(rChnlInfo);

				ucSlotCount++;
			}
		}
	}

	DBGLOG(NAN, INFO, "%u slots committed in band 0x%08x, %u/%u/%u/%u",
	       ucSlotCount, u4Bands,
	       aucCommittedCount[ENUM_SUPPORTED_BN_6G],
	       aucCommittedCount[ENUM_SUPPORTED_BN_5G_HIGH],
	       aucCommittedCount[ENUM_SUPPORTED_BN_5G_LOW],
	       aucCommittedCount[ENUM_SUPPORTED_BN_2G]);
	return ucSlotCount;
}

static uint8_t nanCommonBandPeerCount(struct ADAPTER *prAdapter,
				      uint32_t u4SuportedBands)
{
	uint8_t ucPeerCount = 0;
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);
	struct LINK *prPeerSchDescList;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;

	prPeerSchDescList = &prNanScheduler->rPeerSchDescList;
	LINK_FOR_EACH_ENTRY(prPeerSchDesc, prPeerSchDescList, rLinkEntry,
			    struct _NAN_PEER_SCH_DESC_T) {
		if (prPeerSchDesc == NULL)
			break;

		if (!prPeerSchDesc->fgUsed)
			continue;

		if (prPeerSchDesc->u4PeerSupportedBand & u4SuportedBands)
			ucPeerCount++;
	}

	DBGLOG(NAN, INFO, "%u peer supports band 0x%08x",
	       ucPeerCount, u4SuportedBands);

	return ucPeerCount;
}

static u_int8_t
nanNdcTimelineInBand(struct ADAPTER *prAdapter,
		     struct _NAN_SCHEDULE_TIMELINE_T *prTimeline,
		     enum ENUM_BAND eBand)
{
	uint32_t i;
	union _NAN_BAND_CHNL_CTRL rChnlInfo;

	for (i = 0; i < NAN_SLOTS_PER_DW_INTERVAL; i++) {

		if ((prTimeline->au4AvailMap[0] & BIT(i)) == 0)
			continue;

		rChnlInfo = nanGetCommittedChnlInfoBySlot(prAdapter, i);

		if (rChnlInfo.u4PrimaryChnl == 0)
			continue;

		if (eBand == BAND_5G && NAN_IS_CHANNEL_5G(rChnlInfo) ||
		    eBand == BAND_2G4 && NAN_IS_CHANNEL_2G(rChnlInfo))
			break;

#if (CFG_SUPPORT_NAN_6G == 1)
		if (eBand == BAND_6G && NAN_IS_CHANNEL_6G(rChnlInfo))
			break;
#endif
	}

	if (i == NAN_SLOTS_PER_DW_INTERVAL)
		return FALSE;

	return TRUE;
}

/* Release the NDC control by band,
 * and deassociate the peer with the NDC control block
 */
void nanReleaseNdcByBand(struct ADAPTER *prAdapter, enum ENUM_BAND eBand)
{
	struct _NAN_NDC_CTRL_T *prNdcCtrl;
	uint32_t i;
	uint32_t j;
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);

	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRec;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_SCHEDULE_TIMELINE_T *prTimeline;
	struct LINK *prPeerSchDescList;

	for (i = 0; i < NAN_MAX_NDC_RECORD; i++) {
		prNdcCtrl = &prNanScheduler->arNdcCtrl[i];

		if (!prNdcCtrl->fgValid)
			continue;

		/* Find any NDC control committed in current band */
		for (j = 0; j < ARRAY_SIZE(prNdcCtrl->arTimeline); j++) {
			prTimeline = &prNdcCtrl->arTimeline[j];

			if (prTimeline->ucMapId == NAN_INVALID_MAP_ID)
				continue;

			if (nanNdcTimelineInBand(prAdapter, prTimeline, eBand))
				break;
		}

		if (j == ARRAY_SIZE(prNdcCtrl->arTimeline))
			continue;

		DBGLOG(NAN, INFO,
		       "Found NDC[%u] use band %u %02x-%02x-%02x-%02x-%02x-%02x MapId=%u avail %02x-%02x-%02x-%02x",
		       i, eBand,
		       prNdcCtrl->aucNdcId[0], prNdcCtrl->aucNdcId[1],
		       prNdcCtrl->aucNdcId[2], prNdcCtrl->aucNdcId[3],
		       prNdcCtrl->aucNdcId[4], prNdcCtrl->aucNdcId[5],
		       prNdcCtrl->arTimeline[j].ucLocalMapId,
		       prNdcCtrl->arTimeline[j].aucAvailBlock[0],
		       prNdcCtrl->arTimeline[j].aucAvailBlock[1],
		       prNdcCtrl->arTimeline[j].aucAvailBlock[2],
		       prNdcCtrl->arTimeline[j].aucAvailBlock[3]);

		/* unlink all users, reset this NDC record */
		for (j = 0; j < NAN_MAX_CONN_CFG; j++) {
			prPeerSchRec = nanSchedGetPeerSchRecord(prAdapter, j);

			if (!prPeerSchRec || !prPeerSchRec->fgActive)
				continue;

			DBGLOG(NAN, INFO, "Unlink prPeerSchRec[%u] NDC", j);
			if (prPeerSchRec->prCommNdcCtrl == prNdcCtrl)
				prPeerSchRec->prCommNdcCtrl = NULL;
		}

		prPeerSchDescList = &prNanScheduler->rPeerSchDescList;
		LINK_FOR_EACH_ENTRY(prPeerSchDesc, prPeerSchDescList,
				    rLinkEntry, struct _NAN_PEER_SCH_DESC_T) {
			if (prPeerSchDesc == NULL)
				break;

			if (!prPeerSchDesc->fgUsed)
				continue;

			if (kalMemCmp(prNdcCtrl->aucNdcId,
				      prPeerSchDesc->rSelectedNdcCtrl.aucNdcId,
				      NAN_NDC_ATTRIBUTE_ID_LENGTH) != 0)
				continue;

			DBGLOG(NAN, INFO, "Reset prPeerSchDesc[%u] NDC",
			       prPeerSchDesc->u4SchIdx);
			kalMemZero(&prPeerSchDesc->rSelectedNdcCtrl,
				   sizeof(prPeerSchDesc->rSelectedNdcCtrl));
		}

		DBGLOG(NAN, INFO,
		       "Reset prNdcCtrl[%u] %02x-%02x-%02x-%02x-%02x-%02x",
		       i,
		       prNdcCtrl->aucNdcId[0], prNdcCtrl->aucNdcId[1],
		       prNdcCtrl->aucNdcId[2], prNdcCtrl->aucNdcId[3],
		       prNdcCtrl->aucNdcId[4], prNdcCtrl->aucNdcId[5]);

		kalMemZero(prNdcCtrl, sizeof(*prNdcCtrl));
	}
}

u_int8_t nanCheckIsNeedReschedule(struct ADAPTER *prAdapter,
				  enum RESCHEDULE_SOURCE event,
				  uint8_t *pucPeerNmi)
{
	static uint32_t u4PrevNewNdlActiveNdl;

	uint32_t slot_mask = 0;
	enum ENUM_BAND eAllPeerMaxCap;
	enum ENUM_BAND eAnyPeerMaxCap;
	enum ENUM_BAND eAllPeerAbandMaxCap; /* Not considering 2G only devces */
	enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand;
	uint8_t ucAllPeerMaxPhy;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint8_t aucCommittedCount[ENUM_SUPPORTED_BN_NUM] = {0};

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	eHighestCommonBand = nanSchedGetHighestCommonBand(
					prAdapter, prNegoCtrl->u4SchIdx, TRUE);

	DBGLOG(NAN, DEBUG,
	       "event=%u, NDL valid=%u, active=%u, established=%u\n",
	       event,
	       nanGetValidNdlNum(prAdapter),
	       nanGetActiveNdlNum(prAdapter),
	       nanGetEstablishedNdlNum(prAdapter));

	nanDumpNDL(prAdapter);

	if (event == AIS_CONNECTED)
		slot_mask = NAN_SLOT_MASK_TYPE_AIS;
	else if (event == AIS_DISCONNECTED)
		slot_mask = NAN_SLOT_MASK_TYPE_AIS;
	else if (event == NEW_NDL)
		slot_mask = nanGetNdlSlots(prAdapter);
	else if (event == REMOVE_NDL)
		slot_mask = nanGetNdlSlots(prAdapter);
	else if (event == P2P_CONNECTED)
		slot_mask = NAN_T1_SLOT_MASK_CONCURRENT_FULL;
	else if (event == P2P_DISCONNECTED)
		slot_mask = NAN_T1_SLOT_MASK_CONCURRENT_FULL;

	/* P2P */
	if (event == P2P_CONNECTED || event == P2P_DISCONNECTED ||
	    nanSchedGetConnBands(prAdapter, NETWORK_TYPE_P2P)) {
		return nanCheckIsNeedRescheduleWithP2p(prAdapter, event,
						       pucPeerNmi);
	}

	nanGetMaxCapabilityAllPeers(prAdapter, 0,
				    &eAllPeerMaxCap, &eAnyPeerMaxCap,
				    &eAllPeerAbandMaxCap, &ucAllPeerMaxPhy);

	if (event == AIS_CONNECTED) {
		/* Legacy 1: (NAN) +AIS = NAN+AIS, MLO add link? */
		union _NAN_BAND_CHNL_CTRL rAisChnlInfo = g_rNullChnl;
		union _NAN_BAND_CHNL_CTRL rAis2gChnlInfo = g_rNullChnl;
		uint32_t u4SlotBitmap;
		enum ENUM_BAND eBand;
		uint8_t eAisBandBitmap = 0;
		uint8_t ucAisPhyTypeSet;

		for (eBand = BAND_2G4; eBand < BAND_NUM; eBand++) {
			if (nanSchedGetConnChnlUsage(prAdapter,
					NETWORK_TYPE_AIS, eBand,
					&rAisChnlInfo, &u4SlotBitmap,
					&ucAisPhyTypeSet) !=
			    WLAN_STATUS_SUCCESS)
				continue;

			eAisBandBitmap |= BIT(eBand);
		}
		if (eAisBandBitmap == 0)
			return FALSE;

		if (nanIsAisUse5G6G(prAdapter, eAisBandBitmap, &rAisChnlInfo)) {
			/* If AP use 5G non-DFS or 6G && local channel conflict,
			 * need reschedule.
			 */
			u_int8_t fgIsAisAndAllPeers6G = FALSE;

			fgIsAisAndAllPeers6G =
				nanIsAisAndAllPeers6G(eAisBandBitmap,
						      eAllPeerMaxCap,
						      ucAllPeerMaxPhy);

			return eAnyPeerMaxCap >= BAND_5G &&
			       nanNeedRescheduleByChannel(prAdapter, slot_mask,
					       rAisChnlInfo, TIMELINE_BAND_5G6G,
					       isAisConflictNan) ||
			       /* AIS == NAN == 6G, All == EHT but AIS != EHT */
			       fgIsAisAndAllPeers6G;
		}

		if (eAisBandBitmap & BIT(BAND_2G4)) {
			nanSchedGetConnChnlUsage(prAdapter,
					NETWORK_TYPE_AIS, BAND_2G4,
					&rAis2gChnlInfo, &u4SlotBitmap,
					&ucAisPhyTypeSet);
			/* single map, NAN_2G + STA_2G and STA != ch6 */
			if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
			    eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
			    rAis2gChnlInfo.u4PrimaryChnl !=
			    NAN_2P4G_DISC_CHANNEL)
				return TRUE;

			/* If AP use 2.4G && local 5G/6G channel == NULL,
			 * use it
			 */
			return eAnyPeerMaxCap >= BAND_5G &&
			       nanNeedRescheduleByChannel(prAdapter, slot_mask,
					       g_rNullChnl, TIMELINE_BAND_5G6G,
					       isLocalNotInUse);
		}

		if (eAisBandBitmap & BIT(BAND_5G) &&
			   isDfs(prAdapter, &rAisChnlInfo)) {
			/* If AP use DFS && NAN 2.4G band slot == NULL, then
			 * NAN use 2G
			 */
			/* TODO:
			 * HOW if (G_band.slot[i].ch != NULL &&
			 *         A_band.slot[i].ch != AP.ch)?
			 */
			return nanNeedRescheduleByChannel(prAdapter, slot_mask,
					       g_rNullChnl, TIMELINE_BAND_2G4,
					       isLocalNotInUse);
		}
		return FALSE;
	}

	if (event == AIS_DISCONNECTED || event == REMOVE_NDL) {
		/* Legacy 2: (NAN+AIS) -AIS = NAN, MLO del link? */
		/* Legacy 4a: (NAN) -NAN */
		/* Legacy 4b: (AIS+NAN) -NAN = (AIS+NAN) */
		/* Legacy 4c: (AIS+NAN) -NAN = AIS */

		/* Upgrade: if min(max_cap) > current band, OR
		 *          if channel not in used
		 *
		 * Considering 2G only device.
		 *
		 * Can upgrade:
		 * current=2G  2G  2G  5G  5G  5G  6G  6G  6G (get A-band chnl)
		 * new_min=2G  5G  6G  2G  5G  6G  2G  5G  6G
		 * 2-map:  -   T   T   F   -   F           -  (get A-band chnl)
		 * 1-map:  -   T   T   F   -   F           -
		 */

		/* Remove below EHT condition, recover EHT at nanDataFreeNdl()
		 * All peers support EHT, but local in-use is NOT EHT
		 */
#ifdef NAN_UNUSED /* Dont' consider each slots */
		return (eAnyPeerMaxCap >= BAND_5G &&
			nanNeedRescheduleByChannel(prAdapter, slot_mask,
						g_rNullChnl, TIMELINE_BAND_5G6G,
						isLocalNotInUse)) ||
		       (nanCheck2gOnlyPeerExists(prAdapter) &&
			nanNeedRescheduleByChannel(prAdapter, slot_mask,
						g_rNullChnl, TIMELINE_BAND_2G4,
						isLocalNotInUse)) ||
#endif

#if (CFG_SUPPORT_NAN_6G == 1)
		/* Reschedule if all peers are higher than current channel info
		 */
		if (eAllPeerMaxCap == BAND_6G &&
		    nanNeedRescheduleByCapability(prAdapter, slot_mask,
					TIMELINE_BAND_5G6G,
					eAllPeerAbandMaxCap,
					isLocalInUseAndAllPeerHigher)) {
			DBGLOG(NAN, INFO,
			       "Trigger rescheule, all peers higher");
			return TRUE;
		}
#endif

		/* In the condition to migrate peers from 2G to higher */
		if (nanBandCommittedFawSlotCount(prAdapter,
					BIT(ENUM_SUPPORTED_BN_5G_LOW) |
					BIT(ENUM_SUPPORTED_BN_5G_HIGH) |
					BIT(ENUM_SUPPORTED_BN_6G),
					NAN_SLOT_MASK_TYPE_DEFAULT,
					aucCommittedCount) == 0 &&
		    nanCommonBandPeerCount(prAdapter,
					BIT(ENUM_SUPPORTED_BN_5G_LOW) |
					BIT(ENUM_SUPPORTED_BN_5G_HIGH) |
					BIT(ENUM_SUPPORTED_BN_6G)) != 0) {
			const size_t sz2gTimeLineIdx =
				nanGetTimelineMgmtIndexByBand(prAdapter,
							      BAND_2G4);

			nanSchedReleaseReschedCommitSlot(prAdapter,
						NAN_SLOT_MASK_TYPE_DEFAULT,
						sz2gTimeLineIdx);

			nanReleaseNdcByBand(prAdapter, BAND_2G4);

			return TRUE;
		}

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
		/* the only one, leave insist NDL slot 5G mode */
		if (nanGetValidNdlNum(prAdapter) == 1 &&
		    prNegoCtrl->fgInsistNDLSlot5GMode) {
			prNegoCtrl->fgInsistNDLSlot5GMode = FALSE;
			DBGLOG(NAN, INFO, "Leave NDL slot 5G Mode\n");
		}
#endif

		/* the only one, might be upgradable */
		if (nanGetValidNdlNum(prAdapter) == 1 ||
		    nanGetValidNdlNum(prAdapter) <=
		    prAdapter->rWifiVar.u4NanRescheduleInit &&
		    eAnyPeerMaxCap == BAND_5G) {
			uint32_t committed;
			uint32_t ndl_slots;
			uint32_t ais_slots;

			if (nanGetPeerCommitted(prAdapter, TIMELINE_BAND_5G6G,
							&committed)) {
				ndl_slots = committed &
					nanGetNdlSlots(prAdapter);
				ais_slots = committed & NAN_SLOT_MASK_TYPE_AIS;

				DBGLOG(NAN, DEBUG,
				       "committed=%02x-%02x-%02x-%02x, ndlslots=%02x-%02x-%02x-%02x, aisslots=%02x-%02x-%02x-%02x\n",
				       ((uint8_t *)&committed)[0],
				       ((uint8_t *)&committed)[1],
				       ((uint8_t *)&committed)[2],
				       ((uint8_t *)&committed)[3],
				       ((uint8_t *)&ndl_slots)[0],
				       ((uint8_t *)&ndl_slots)[1],
				       ((uint8_t *)&ndl_slots)[2],
				       ((uint8_t *)&ndl_slots)[3],
				       ((uint8_t *)&ais_slots)[0],
				       ((uint8_t *)&ais_slots)[1],
				       ((uint8_t *)&ais_slots)[2],
				       ((uint8_t *)&ais_slots)[3]);

				if (nanUtilCheckBitOneCnt(&ndl_slots,
							  sizeof(uint32_t)) <
					    NAN_FEW_COMMITTED_SLOTS ||
				    nanUtilCheckBitOneCnt(&ais_slots,
							  sizeof(uint32_t)) ==
					    0)
					return TRUE;
			}
		}

		return FALSE;
	}

	if (event == NEW_NDL) {
		/* Legacy 3a: (NAN) +NAN */
		/* Legacy 3b: (AIS+NAN) +NAN = (AIS+NAN) */
		/* Legacy 3c: (AIS) +NAN = AIS */

		enum ENUM_BAND ePeerMaxCap = BAND_5G;
		u_int8_t fgIsNewNdlLower;
#if (CFG_SUPPORT_NAN_11BE)
		u_int8_t fgIsEhtReschedule;
#endif

		/* Downgrade: new_cap < current band,
		 * 2-map: if new==2G && current>=5G, no need to reschedule
		 * 1-map: if new==2G && current>=5G, need to reschedule
		 *
		 * Need downgrade:
		 * current=2G  2G  2G  5G  5G  5G  6G  6G  6G (get A-band chnl)
		 * new NDL=2G  5G  6G  2G  5G  6G  2G  5G  6G
		 * 2-map:  x   x   x   F   F   F   ?   T   F
		 * 1-map:  F   T   T   F   T   F
		 */
		/* TODO: explicit specify New NDL band in the parameter.
		 * e.g., eAllPeerMaxCap = BAND_5G;
		 *
		 * However, if new peer has added to peer sched record,
		 * use eAllPeerMaxCap might be sufficient, since new Peer cap
		 * lowers the lowerbound
		 */

		if (pucPeerNmi) {
			prNDL = nanDataUtilSearchNdlByMac(prAdapter,
							  pucPeerNmi);
			prPeerSchDesc =
				nanSchedAcquirePeerSchDescByNmi(prAdapter,
								pucPeerNmi);
		}
		if (prNDL)
			DBGLOG(NAN, TRACE, "PhyTypeSet=%u\n",
			       prNDL->ucPhyTypeSet);
		if (prPeerSchDesc) {
			ePeerMaxCap = getPeerSchDescMaxCap(prPeerSchDesc);
			DBGLOG(NAN, DEBUG, "ePeerMaxCap=%u\n", ePeerMaxCap);
#if (CFG_SUPPORT_NAN_11BE == 1)
			DBGLOG(NAN, DEBUG, "EHT=%u\n", prPeerSchDesc->fgEht);
#endif
		}

		fgIsNewNdlLower = nanNeedRescheduleByCapability(prAdapter,
					slot_mask,
					TIMELINE_BAND_5G6G,
					ePeerMaxCap,
					isNewCapabilityLower);

		/* nanGetMaxCapabilityAllPeers() does not count the new NDL */
		if (nanGetActiveNdlNum(prAdapter) ==
		    prAdapter->rWifiVar.u4NanRescheduleInit) {
			DBGLOG(NAN, DEBUG,
			       "Active device=%u, AllMax=%u, AnyMax=%u, ePeerMaxCap=%u, u4PrevNewNdlActiveNdl=%u\n",
			       nanGetActiveNdlNum(prAdapter),
			       eAllPeerMaxCap, eAnyPeerMaxCap, ePeerMaxCap,
			       u4PrevNewNdlActiveNdl);
			u4PrevNewNdlActiveNdl = nanGetActiveNdlNum(prAdapter);
		} else if (nanGetActiveNdlNum(prAdapter) ==
			   prAdapter->rWifiVar.u4NanRescheduleInit + 1) {

			DBGLOG(NAN, DEBUG,
			       "AllMax=%u(6G=0x%X), AnyMax=%u, ePeerMaxCap=%u, at %u active device, u4PrevNewNdlActiveNdl=%u\n",
			       eAllPeerMaxCap,
#if (CFG_SUPPORT_NAN_6G == 1)
			       eAllPeerMaxCap == BAND_6G,
#else
			       0xF, /* hint as "FALSE" for special config */
#endif
			       eAnyPeerMaxCap, ePeerMaxCap,
			       nanGetActiveNdlNum(prAdapter),
			       u4PrevNewNdlActiveNdl);

#if (CFG_SUPPORT_NAN_6G == 1)
			if (eAllPeerMaxCap == BAND_6G || fgIsNewNdlLower) {
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
				/* If peer seq is 6 6 6,
				 * set NDL flag for later to
				 * enable insist NDL slot 5G mode
				 */
				if (prNDL && ePeerMaxCap == BAND_6G &&
				    !prNegoCtrl->fgInsistNDLSlot5GMode)
					prNDL->fgIs3rd6GNewNDL = TRUE;
#endif

				if (nanGetActiveNdlNum(prAdapter) !=
				    u4PrevNewNdlActiveNdl) {
					u4PrevNewNdlActiveNdl =
						nanGetActiveNdlNum(prAdapter);
					return TRUE;
				}

				/* equals u4PrevNewNdlActiveNdl */
				if (prNDL)
					prNDL->fgTriggerReschedNewNDL = TRUE;
			}
#endif

			u4PrevNewNdlActiveNdl = nanGetActiveNdlNum(prAdapter);
			return FALSE;
		}

		if (nanGetActiveNdlNum(prAdapter) >
			   prAdapter->rWifiVar.u4NanRescheduleInit + 1) {
			DBGLOG(NAN, DEBUG,
			       "No Reschedule for %u active device, AllMax=%u, AnyMax=%u, ePeerMaxCap=%u, u4PrevNewNdlActiveNdl=%u\n",
			       nanGetActiveNdlNum(prAdapter),
			       eAllPeerMaxCap, eAnyPeerMaxCap, ePeerMaxCap,
			       u4PrevNewNdlActiveNdl);

			u4PrevNewNdlActiveNdl = nanGetActiveNdlNum(prAdapter);
			return FALSE;
		}

		/* NDP 1 to 1 case no need to reschedule,
		 * check whether need to downgrade EHT mode
		 */
		if (nanGetActiveNdlNum(prAdapter) <= 1) {
#if (CFG_SUPPORT_NAN_EXT == 1)
#if (CFG_SUPPORT_NAN_11BE == 1)
			if (nanIsEhtSupport(prAdapter) &&
			    nanIsEhtEnable(prAdapter) &&
			    prPeerSchDesc &&
			    !prPeerSchDesc->fgEht) {
				DBGLOG(NAN, DEBUG,
				       "Downgrade EHT mode in NEW_NDL\n");
				nanEnableEht(prAdapter, FALSE);
			}
#endif /* CFG_SUPPORT_NAN_11BE */
#endif /* CFG_SUPPORT_NAN_EXT */

			return FALSE;
		}

#if (CFG_SUPPORT_NAN_11BE == 1)
		/* All peers support EHT, but new != EHT */
		fgIsEhtReschedule = nanIsEhtNeedReschedule(prAdapter,
							   prPeerSchDesc,
							   ucAllPeerMaxPhy);

		DBGLOG(NAN, DEBUG, "fgIsEhtReschedule=%u\n",
			fgIsEhtReschedule);

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
		if (prNDL)
			prNDL->fgIsEhtReschedule = fgIsEhtReschedule;
#endif
#endif

#if (CFG_SUPPORT_NAN_11BE == 1)
		return fgIsNewNdlLower || fgIsEhtReschedule;
#else
		return fgIsNewNdlLower;
#endif
	}

	return FALSE;
}

void
nanSchedSetLocalCommittedBand(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prScheduler;
	uint8_t aucCommittedSlots[ENUM_SUPPORTED_BN_NUM] = {0};
	uint32_t u4Bands = 0;
	uint32_t u4BnIdx = 0;

	prScheduler = nanGetScheduler(prAdapter);
	u4Bands = BIT(ENUM_SUPPORTED_BN_2G) |
		  BIT(ENUM_SUPPORTED_BN_5G_LOW) |
		  BIT(ENUM_SUPPORTED_BN_5G_HIGH) |
		  BIT(ENUM_SUPPORTED_BN_6G);

	nanBandCommittedFawSlotCount(prAdapter,
				     u4Bands,
				     NAN_SLOT_MASK_TYPE_DEFAULT_NDL,
				     aucCommittedSlots);
	for (u4BnIdx = 0; u4BnIdx < ENUM_SUPPORTED_BN_NUM; u4BnIdx++) {
		if (aucCommittedSlots[u4BnIdx] > 8) {
			prScheduler->eLocalCommittedBand = u4BnIdx;
			break;
		}
	}

	DBGLOG(NAN, INFO, "Local Committed = %u\n",
					prScheduler->eLocalCommittedBand);
}

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
/**
 * Brief: Release the slots not locked noted in u4ReschedSlot,
 * e.g., all slots except slot#9.
 */
void nanSchedReleaseReschedCommitSlot(struct ADAPTER *prAdapter,
					     uint32_t u4ReschedSlot,
					     size_t szTimeLineIdx)
{
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimelineList = NULL;
	uint32_t u4SlotIdx = 0;
	uint32_t u4Idx = 0;
	uint32_t u4Valid = 0;
	uint32_t u4SchIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	if (szTimeLineIdx >= szNanActiveTimelineNum) {
		DBGLOG(NAN, ERROR, "szTimeLineIdx invalid\n");
		return;
	}

	/* Release timeline committed slot according to szTimeLineIdx*/
	prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter, szTimeLineIdx);
	prChnlTimelineList = prNanTimelineMgmt->arChnlList;

	DBGLOG(NAN, DEBUG,
	       "Released u4ReschedSlot: 0x%08x, szTimeLineIdx: %zu\n",
	       u4ReschedSlot, szTimeLineIdx);

	for (u4SlotIdx = 0; u4SlotIdx < NAN_TOTAL_SLOT_WINDOWS; u4SlotIdx++) {
		if ((u4ReschedSlot & BIT(NAN_SLOT_INDEX(u4SlotIdx))) == 0)
			continue;

		for (u4Idx = 0; u4Idx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM;
		     u4Idx++) {
			if (prChnlTimelineList[u4Idx].fgValid == FALSE)
				continue;

			u4Valid = NAN_IS_AVAIL_MAP_SET(
				prChnlTimelineList[u4Idx].au4AvailMap,
				u4SlotIdx);
			if (u4Valid) {
				nanSchedDeleteCrbFromChnlList(prAdapter,
					u4SlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, szTimeLineIdx);
				break;
			}
		}
	}

	/* Release all */
	for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++)
		nanSchedPeerUpdateCommonFAW(prAdapter, u4SchIdx);

	nanSchedReleaseUnusedCommitSlot(prAdapter);

	nanSchedCmdUpdateAvailability(prAdapter);

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
}

void nanSchedRegisterReschedInf(
	struct _NAN_RESCHEDULE_TOKEN_T *
		(*fnGetRescheduleToken)(struct ADAPTER *prAdapter))
{
	if (fnGetRescheduleToken != NULL)
		nanSchedGetRescheduleToken = fnGetRescheduleToken;
}

void nanSchedUnRegisterReschedInf(void)
{
	nanSchedGetRescheduleToken = NULL;
}
#endif

/* Check local 5/6G chnl and peer 5/6G chnl have intersection in this slot
 * Condition:
 *     (1) local 5/6g committed or conditional chnl != 0
 *     (2) Peer 5/6G committed or conditional chnl != 0
 *     (3) local 5/6G chnl == peer 5/6G chnl
 */
uint8_t nanSchedNegoChk56GIntersectBySlot(struct ADAPTER *prAdapter,
					  uint32_t u4SchIdx,
					  size_t szSlotIdx,
					  uint8_t *local, uint8_t *remote)
{
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};
	uint32_t u4LocalPrimaryChnl = 0;
	size_t szAvailDbIdx = 0;
	enum ENUM_BAND eLocalBand = BAND_NULL;
	enum ENUM_BAND eRmtBand = BAND_NULL;

	rLocalChnlInfo = nanGetChnlInfoBySlot(prAdapter, szSlotIdx,
						sz5gTimeLineIdx);

	u4LocalPrimaryChnl = rLocalChnlInfo.u4PrimaryChnl;
	if (u4LocalPrimaryChnl == 0)
		return FALSE;

	eLocalBand = nanRegGetNanChnlBand(rLocalChnlInfo);

	for (szAvailDbIdx = 0;
		szAvailDbIdx < NAN_NUM_AVAIL_DB; szAvailDbIdx++) {
		uint32_t u4RmtPrimaryChnl;

		if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
				u4SchIdx, szAvailDbIdx) == FALSE)
			continue;

		rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, u4SchIdx,
				szAvailDbIdx, szSlotIdx,
				TRUE);

		u4RmtPrimaryChnl = rRmtChnlInfo.u4PrimaryChnl;
		if (u4RmtPrimaryChnl == 0)
			continue;

		eRmtBand = nanRegGetNanChnlBand(rRmtChnlInfo);

		if (eRmtBand == BAND_2G4)
			continue;

		*local = u4LocalPrimaryChnl;
		*remote = u4RmtPrimaryChnl;
		if (eLocalBand == eRmtBand &&
		    u4LocalPrimaryChnl == u4RmtPrimaryChnl)
			return TRUE;
	}

	return FALSE;
}

#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
static uint32_t nanSchedSelectNegoSlot(struct ADAPTER *prAdapter,
				enum RESCHEDULE_SOURCE eReschedSrc)
{
	if (eReschedSrc == AIS_CONNECTED || eReschedSrc == AIS_DISCONNECTED)
		return NAN_SLOT_MASK_TYPE_AIS;

	if (eReschedSrc == NEW_NDL)
		return nanGetNdlSlots(prAdapter);

	/* Use AIS + NDL slot to recover full slot for last peer */
	if (eReschedSrc == REMOVE_NDL)
		return NAN_SLOT_MASK_TYPE_AIS | nanGetNdlSlots(prAdapter);

	if (eReschedSrc == P2P_CONNECTED || eReschedSrc == P2P_DISCONNECTED)
		return NAN_T1_SLOT_MASK_CONCURRENT_FULL;

	return NAN_SLOT_MASK_TYPE_DEFAULT;
}

enum ENUM_BAND nanSchedGetMaxCap(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prScheduler = NULL;

	prScheduler = nanGetScheduler(prAdapter);

#if (CFG_SUPPORT_NAN_6G == 1)
	if (prScheduler->fgEn6g)
		return BAND_6G;
#endif
	if (prScheduler->fgEn5gL || prScheduler->fgEn5gH)
		return BAND_5G;

	return BAND_2G4;
}

/**
 * nanSchedNegoFindAisSlotCrb() - Determine ChnlInfo of at szSlotIdx
 *				  in szTimeLineIdx
 * @prAdapter:
 * @eHighestCommonBand: highest common band
 * @fgPrintLog:
 * @szTimeLineIdx: loop down from 5GHz at caller
 * @szSlotIdx: slot index in range [0,8192)
 *
 * Longer description
 * Select the AIS slots (block #1, #3) according to the current concurrent
 * connections.
 *
 * Context:
 * Called from nanSchedNegoGenDefCrbV2() (via nanSchedNegoFindSlotCrb) by
 * looping szTimeLineIdx downward with each szSlotIdx to set the channel info
 * by nanSchedAddCrbToChnlList().
 *
 * Return:
 */
static union _NAN_BAND_CHNL_CTRL
nanSchedNegoFindAisSlotCrb(struct ADAPTER *prAdapter,
			   enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand,
			   unsigned char fgPrintLog,
			   size_t szTimeLineIdx,
			   size_t szSlotIdx)
{
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo = g_rNullChnl;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo = g_rNullChnl;
	enum ENUM_BAND eAisBand = BAND_NULL;
	enum ENUM_BAND eRmtBand = BAND_NULL;
	uint8_t ucAisPrimaryChnl = 0;
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rRmtInfraChnlInfo = {.u4RawData = 0};
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	enum ENUM_BAND eAllPeerMaxCap = BAND_NULL;
	enum ENUM_BAND eAnyPeerMaxCap = BAND_NULL;
	enum ENUM_BAND eAllPeerAbandMaxCap = BAND_NULL;
	uint8_t ucAllPeerMaxPhy = 0;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};
	size_t szAvailDbIdx = 0;
	struct _NAN_SCHEDULER_T *prScheduler = NULL;
	enum ENUM_BAND eMaxBand = BAND_NULL;
	enum ENUM_BAND eMinBand = BAND_NULL;
	uint8_t fgPeerChnlExist = FALSE;
	uint32_t u4NegoTransIdx = nanSchedGetCurrentNegoTransIdx(prAdapter);
	struct _NAN_CRB_NEGO_TRANSACTION_T *prNegoTrans;
	uint8_t local;
	uint8_t remote;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prScheduler = nanGetScheduler(prAdapter);

	/* P2P + AIS MCC, do not select channel in this timeline */
	if (nanIsP2pAisMCC(prAdapter, szTimeLineIdx,
			   &rP2pChnlInfo, &rAisChnlInfo)) {

		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): MCC p2p=%u, ais=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rP2pChnlInfo.u4PrimaryChnl,
			      rAisChnlInfo.u4PrimaryChnl);

		return g_rNullChnl;
	}

	/* single map concurrent case */
	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1) {
		/* NAN only and highest 2G, do not skip timeline and set 2G */
		if (rAisChnlInfo.u4PrimaryChnl == 0 &&
		    eHighestCommonBand == ENUM_SUPPORTED_BN_2G)
			goto channel_select;

		/* Concurrent with STA */
		if (rAisChnlInfo.u4PrimaryChnl != 0) {
			/**
			 * if STA_2G + NAN_5/6G or STA_5/6G + NAN_2G
			 * config FAW only 5/6G or 2G
			 * therefore, remove AIS slots here
			 */
			if ((eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
			     !NAN_IS_CHANNEL_2G(rAisChnlInfo)) ||
			    (eHighestCommonBand > ENUM_SUPPORTED_BN_2G &&
			     NAN_IS_CHANNEL_2G(rAisChnlInfo))) {
				NAN_DW_DBGLOG(NAN, INFO, fgPrintLog, szSlotIdx,
						"Tidx(%zu) AIS slot(%zu): ais=%u, highest=%u\n",
						szTimeLineIdx, szSlotIdx,
						rAisChnlInfo.u4PrimaryChnl,
						eHighestCommonBand);
				nanSchedDeleteCrbFromChnlList(prAdapter,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, szTimeLineIdx);
				return g_rNullChnl;
			}
		}
	}

	/* Only P2P, or P2P+AIS SCC, use this channel, MCC returned earlier */
	if (rP2pChnlInfo.u4PrimaryChnl) {
		if (!NAN_CHNL_IN_COMMON_BAND(prAdapter,
					     prNegoCtrl, rP2pChnlInfo)) {
			NAN_DW_DBGLOG(NAN, INFO, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) AIS slot(%zu): p2p=%u, ais=%u, skip use %u since peer not support\n",
				      szTimeLineIdx, szSlotIdx,
				      rP2pChnlInfo.u4PrimaryChnl,
				      rAisChnlInfo.u4PrimaryChnl,
				      rP2pChnlInfo.u4PrimaryChnl);
			return g_rNullChnl;
		}

		nanSchedAddCrbToChnlList(prAdapter, &rP2pChnlInfo, szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 TRUE, NULL);
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): Use P2p channel p2p=%u, ais=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rP2pChnlInfo.u4PrimaryChnl,
			      rAisChnlInfo.u4PrimaryChnl);
		return rP2pChnlInfo;
	}

	/* Only AIS */

	/* Check infra channel != DFS(5G) channel */

	/* Use the channel AIS is in use */
	ucAisPrimaryChnl = (uint8_t) rAisChnlInfo.u4PrimaryChnl;
	eAisBand = nanRegGetNanChnlBand(rAisChnlInfo);

	if (ucAisPrimaryChnl != 0 &&
	    NAN_IS_TIMELINE_MATCH_BAND(prAdapter, szTimeLineIdx, eAisBand)) {
		/* Channel remain 0 for DFS channel, or
		 * not supported channel (e.g. NAN 5G only connect 6G infra)
		 */
		if (rlmDomainIsDfsChnls(prAdapter, ucAisPrimaryChnl)) {
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			       "Tidx(%zu) AIS slot(%zu): Infra DFS ch:%u!\n",
			       szTimeLineIdx, szSlotIdx, ucAisPrimaryChnl);
			return g_rNullChnl;
		}

		if (!nanIsAllowedChannel(prAdapter, rAisChnlInfo)) {
			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
			       "Tidx(%zu) AIS slot(%zu): Not allowed infra ch:%u!\n",
			       szTimeLineIdx, szSlotIdx, ucAisPrimaryChnl);
			return g_rNullChnl;
		}

		if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
			eAisBand == BAND_2G4 &&
			eHighestCommonBand > ENUM_SUPPORTED_BN_2G) {
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				"Tidx(%zu) AIS slot(%zu): Single map NAN_5G but AIS_2G_ch%u\n",
				szTimeLineIdx, szSlotIdx, ucAisPrimaryChnl);
			return g_rNullChnl;
		}

		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): Infra ch:%u\n",
			      szTimeLineIdx, szSlotIdx, ucAisPrimaryChnl);

		nanSchedAddCrbToChnlList(prAdapter, &rAisChnlInfo, szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 TRUE, NULL);

		return rAisChnlInfo;
	}

	/* Check local committed channel != 0 */
	rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter, szSlotIdx, NULL,
						TRUE, szTimeLineIdx);

	if (rLocalChnlInfo.u4PrimaryChnl != 0) {
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): Committed ch:%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rLocalChnlInfo.u4PrimaryChnl);
		return rLocalChnlInfo;
	}

	/* Check if it is initiator (NDP/Schedule request) */
	if (prNegoCtrl->eState == ENUM_NAN_CRB_NEGO_STATE_INITIATOR) {
		uint32_t u4RmtPrimaryChnl;

		eRmtBand = nanRegGetNanChnlBand(rRmtInfraChnlInfo);
		u4RmtPrimaryChnl = rRmtInfraChnlInfo.u4PrimaryChnl;

		/* Check peer has single potential channel */
		/* Not ready yet, skip first */
		if (u4RmtPrimaryChnl != 0 &&
		    NAN_IS_TIMELINE_MATCH_BAND(prAdapter, szTimeLineIdx,
					       eRmtBand)) {
			if (!nanIsAllowedChannel(prAdapter,
						 rRmtInfraChnlInfo)) {
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) AIS slot(%zu): Peer not allowed infra ch:%u!\n",
					      szTimeLineIdx, szSlotIdx,
					      u4RmtPrimaryChnl);
				return g_rNullChnl;
			}

			rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
							rRmtInfraChnlInfo);

			if (rSelChnlInfo.u4PrimaryChnl != 0) {
				NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) AIS slot(%zu): Peer infra ch:%u\n",
					      szTimeLineIdx, szSlotIdx,
					      u4RmtPrimaryChnl);

				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);

				return rSelChnlInfo;
			}
			/* rSelChnlInfo u4PrimaryChnl == 0 */
			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) AIS slot(%zu): Peer infra CH = 0!\n",
				      szTimeLineIdx, szSlotIdx);
			return g_rNullChnl;
		}

		/* u4RmtPrimaryChnl == 0 ||
		 * szTimeLineIdx != nanGetTimelineMgmtIndexByBand()
		 * Check min of peer max cap and our max cap
		 */
		if (NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx)) {

			rSelChnlInfo = g_r5gDwChnl;

			nanGetMaxCapabilityAllPeers(prAdapter,
						    szSlotIdx,
						    &eAllPeerMaxCap,
						    &eAnyPeerMaxCap,
						    &eAllPeerAbandMaxCap,
						    &ucAllPeerMaxPhy);
			eMaxBand = nanSchedGetMaxCap(prAdapter);
			eMinBand = (eMaxBand < eAllPeerAbandMaxCap) ?
				       eMaxBand :
				       eAllPeerAbandMaxCap;

			if (prAdapter->rWifiVar.ucNan5gBandwidth
				== NAN_CHNL_BW_160)
				rSelChnlInfo = g_r5g160Chnl;

#if (CFG_SUPPORT_NAN_6G == 1)
			if (eHighestCommonBand == ENUM_SUPPORTED_BN_6G &&
			    eMinBand == BAND_6G &&
			    nanIsAllowedChannel(prAdapter, g_r6gDefChnl))
				rSelChnlInfo = g_r6gDefChnl;
			else
#endif
				/* non-DBDC 2G only case */
				if (eMinBand == BAND_2G4)
					rSelChnlInfo = g_r2gDwChnl;
		} else {
			rSelChnlInfo = g_r2gDwChnl;

			prNegoTrans = &prNegoCtrl->rNegoTrans[u4NegoTransIdx];
			if (!prNegoTrans->fgPropose2gConditional) {
				DBGLOG(NAN, DEBUG, "Propose 2G conditional\n");
				prNegoTrans->fgPropose2gConditional = TRUE;
			}
		}

		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): Min cap ch:%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rSelChnlInfo.u4PrimaryChnl);

		nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					 szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 FALSE, NULL);

		return rSelChnlInfo;
	}

	/* prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR */
	/* Responder (NDP/Schedule response/confirm) */
	/* If 5/6G already had intersection with peer, skip 2G chnl selection */
	if (NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
	    nanSchedNegoChk56GIntersectBySlot(prAdapter,
					      prNegoCtrl->u4SchIdx,
					      szSlotIdx,
					      &local, &remote)) {
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): 5/6G has intersection (l=%u, r=%u), skip 2G\n",
			      szTimeLineIdx, szSlotIdx, local, remote);
		return g_rNullChnl;

	}

channel_select:
	/* Check peer committed/conditional != 0 */
	for (szAvailDbIdx = 0;
		szAvailDbIdx < NAN_NUM_AVAIL_DB; szAvailDbIdx++) {
		uint32_t u4RmtPrimaryChnl;
		uint32_t u4SelPrimaryChnl;

		if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
			prNegoCtrl->u4SchIdx, szAvailDbIdx) == FALSE)
			continue;

		rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, prNegoCtrl->u4SchIdx,
				szAvailDbIdx, szSlotIdx,
				TRUE);

		u4RmtPrimaryChnl = rRmtChnlInfo.u4PrimaryChnl;
		if (u4RmtPrimaryChnl == 0)
			continue;

		eRmtBand = nanRegGetNanChnlBand(rRmtChnlInfo);
		if (!NAN_IS_TIMELINE_MATCH_BAND(prAdapter, szTimeLineIdx,
						eRmtBand))
			continue;

		/* for single map, skip 2G channel if highest > 2G */
		if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
		    eRmtBand == BAND_2G4 &&
		    eHighestCommonBand > ENUM_SUPPORTED_BN_2G)
			continue;

		fgPeerChnlExist = TRUE;

		if (!nanIsAllowedChannel(prAdapter, rRmtChnlInfo)) {
			struct _NAN_CRB_NEGO_TRANSACTION_T *prCurrNegoTrans;
			uint32_t u4NegoTransIdx =
				nanSchedGetCurrentNegoTransIdx(prAdapter);

			if (rRmtChnlInfo.u4PrimaryChnl ==
			    NAN_5G_HIGH_DISC_CHANNEL &&
			    (eHighestCommonBand != ENUM_SUPPORTED_BN_2G &&
			     eHighestCommonBand != ENUM_SUPPORTED_BN_NUM)) {
				prCurrNegoTrans =
					&prNegoCtrl->rNegoTrans[u4NegoTransIdx];

				/* JP allows 44 but not peer's proposal 149 */
				rSelChnlInfo = g_r5gDwChnl;
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					    "Tidx(%zu) AIS slot(%zu): Peer ch=%u not allowed, counter with %u!\n",
					    szTimeLineIdx, szSlotIdx,
					    u4RmtPrimaryChnl,
					    g_r5gDwChnl.u4PrimaryChnl);
				prCurrNegoTrans->fgCounterCountry = TRUE;
				return rSelChnlInfo;
			}

			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) AIS slot(%zu): Peer commit/cond ch:%u not allowed!\n",
				      szTimeLineIdx, szSlotIdx,
				      u4RmtPrimaryChnl);
			continue;
		}

		rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
							rRmtChnlInfo);
		u4SelPrimaryChnl = rSelChnlInfo.u4PrimaryChnl;

		if (u4SelPrimaryChnl != 0) {
			if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
				eRmtBand == BAND_2G4 &&
				eHighestCommonBand > ENUM_SUPPORTED_BN_2G) {
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					"Tidx(%zu) AIS slot(%zu):Single map, peer ch:%u not allowed!\n",
					szTimeLineIdx, szSlotIdx,
					u4SelPrimaryChnl);
				return g_rNullChnl;
			}
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) AIS slot(%zu): Peer commit/cond ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      u4SelPrimaryChnl);

			nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);

			return rSelChnlInfo;
		} else {
			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) AIS slot(%zu): Peer commit/cond ch:%u converge fail!\n",
				      szTimeLineIdx, szSlotIdx,
				      u4RmtPrimaryChnl);
		}

		NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
			      "Tidx(%u) AIS slot(%zu): Peer commit/cond CH%u converge fail!\n",
			      szTimeLineIdx, szSlotIdx,
			      u4RmtPrimaryChnl);
	}

	if (!fgPeerChnlExist)
		NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) AIS slot(%zu): Peer commit/cond not found!\n",
			      szTimeLineIdx, szSlotIdx);

	return g_rNullChnl;
}

/**
 * nanSchedNegoFindNdlSlotCrb() - Determine ChnlInfo of at szSlotIdx
 *				  in szTimeLineIdx
 * @prAdapter:
 * @eHighestCommonBand: highest common band
 * @fgPrintLog:
 * @szTimeLineIdx: loop down from 5GHz at caller
 * @szSlotIdx: slot index in range [0,8192)
 * @fgReschedForce5G:
 * @pfgNotChoose6G:
 *
 * Longer description
 * Select the NDL slots (block #2, #4) according to the current concurrent
 * connections.
 *
 * Context:
 * Called from nanSchedNegoGenDefCrbV2() (via nanSchedNegoFindSlotCrb) by
 * looping szTimeLineIdx downward with each szSlotIdx to set the channel info
 * by nanSchedAddCrbToChnlList().
 *
 * Return:
 */
static union _NAN_BAND_CHNL_CTRL
nanSchedNegoFindNdlSlotCrb(struct ADAPTER *prAdapter,
			   enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand,
			   unsigned char fgPrintLog,
			   size_t szTimeLineIdx,
			   size_t szSlotIdx,
			   unsigned char fgReschedForce5G,
			   unsigned char *pfgNotChoose6G)
{
	union _NAN_BAND_CHNL_CTRL rP2pChnlInfo = g_rNullChnl;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo = g_rNullChnl;

	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {.u4RawData = 0};
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {.u4RawData = 0};
	enum ENUM_BAND eAllPeerMaxCap = BAND_NULL;
	enum ENUM_BAND eAnyPeerMaxCap = BAND_NULL;
	enum ENUM_BAND eAllPeerAbandMaxCap = BAND_NULL;
	uint8_t ucAllPeerMaxPhy = 0;
	size_t szAvailDbIdx = 0;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};
	enum ENUM_BAND eRmtBand = BAND_NULL;
	enum ENUM_BAND eMaxBand = BAND_NULL;
	enum ENUM_BAND eMinBand = BAND_NULL;
	uint8_t fgPeerChnlExist = FALSE;
#if (CFG_SUPPORT_NAN_6G == 1)
	uint8_t fgIs6GDefChnlAllowed = FALSE;
	uint32_t u4RmtOperatingClass = 0;
#endif
	uint32_t u4RmtPrimaryChnl = 0;
	uint32_t u4NegoTransIdx = nanSchedGetCurrentNegoTransIdx(prAdapter);
	struct _NAN_CRB_NEGO_TRANSACTION_T *prNegoTrans;
	uint8_t local;
	uint8_t remote;
	struct _NAN_SCHEDULER_T *prScheduler;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);
	prScheduler = nanGetScheduler(prAdapter);

	/* Check committed channel != 0 */
	rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter, szSlotIdx, NULL,
						TRUE, szTimeLineIdx);

	if (nanIsP2pAisMCC(prAdapter, szTimeLineIdx,
			   &rP2pChnlInfo, &rAisChnlInfo)) {
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) NDL slot(%zu): MCC p2p=%u, ais=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rP2pChnlInfo.u4PrimaryChnl,
			      rAisChnlInfo.u4PrimaryChnl);
		return g_rNullChnl;
	}

	/* single map */
	if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
	    eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
	    prScheduler->eLocalCommittedBand == ENUM_SUPPORTED_BN_2G) {

		/* reserve slot 9 to channel switch to protect 5G DW */
		if ((prScheduler->fgEn5gH || prScheduler->fgEn5gL) &&
		    NAN_SLOT_IS_NDC_BY_BAND(szTimeLineIdx, szSlotIdx, BAND_5G))
			return g_rNullChnl;

		rSelChnlInfo = g_r2gDwChnl;
		nanSchedAddCrbToChnlList(prAdapter,
					 &rSelChnlInfo, szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 TRUE, NULL);
		return rSelChnlInfo;
	}

#if (NDC_NEXT_SLOT_CHANNEL == 149) /* slot #10 (NDC+1) = 149 or 44 */
	if (g_u4MaxChnlSwitchTimeUs >= CHANNEL_SWITCH_THRESHOLD &&
	    NAN_SLOT_IS_NDC_BY_BAND(szTimeLineIdx, szSlotIdx - 1, BAND_5G) &&
	    eHighestCommonBand != ENUM_SUPPORTED_BN_2G) {

		rSelChnlInfo = g_r5gDwChnl;

#if (CFG_SUPPORT_NAN_6G == 1)
		if (nanGetFeatureIsSigma(prAdapter) &&
		    eHighestCommonBand == ENUM_SUPPORTED_BN_6G) /* follow #9 */
			rSelChnlInfo = g_r6gDefChnl;
#endif

		/* single map, reschedule with diff country code */
		if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
		    rLocalChnlInfo.u4PrimaryChnl == 0 &&
		    nanGetNafHighestCommonBand(prAdapter) ==
		    ENUM_SUPPORTED_BN_2G &&
		    nanGetCurrentNdlState(prAdapter) ==
		    NDL_RESPONDER_RX_SCHEDULE_REQUEST) {
			NAN_DW_DBGLOG(NAN, DEBUG, TRUE, szSlotIdx,
			      "Local=%u, Skip slot %u due to country code\n",
			      rLocalChnlInfo.u4PrimaryChnl, szSlotIdx);
			return g_rNullChnl;
		}

		nanSchedAddCrbToChnlList(prAdapter,
					 &rSelChnlInfo, szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 TRUE, NULL);
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Force timeline %zu slot %zu (following NDC) to use channel %u\n",
			      szTimeLineIdx, szSlotIdx,
			      rSelChnlInfo.u4PrimaryChnl);

		return rSelChnlInfo;
	}
#endif

	/* Only P2P, or P2P+AIS SCC, use this channel, MCC returned earlier */
	if (rP2pChnlInfo.u4PrimaryChnl) {
		if (!NAN_CHNL_IN_COMMON_BAND(prAdapter,
					     prNegoCtrl, rP2pChnlInfo)) {
			NAN_DW_DBGLOG(NAN, INFO, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): p2p=%u, ais=%u, skip use %u since peer not support\n",
				      szTimeLineIdx, szSlotIdx,
				      rP2pChnlInfo.u4PrimaryChnl,
				      rAisChnlInfo.u4PrimaryChnl,
				      rP2pChnlInfo.u4PrimaryChnl);
			return g_rNullChnl;
		}

		nanSchedAddCrbToChnlList(prAdapter, &rP2pChnlInfo, szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 TRUE, NULL);

		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) NDL slot(%zu): Use P2p channel p2p=%u, ais=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rP2pChnlInfo.u4PrimaryChnl,
			      rAisChnlInfo.u4PrimaryChnl);
		return rP2pChnlInfo;
	}

	/* Only AIS */
#if (CFG_SUPPORT_NAN_6G == 1)
	for (szAvailDbIdx = 0;
		szAvailDbIdx < NAN_NUM_AVAIL_DB; szAvailDbIdx++) {
		if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
			prNegoCtrl->u4SchIdx, szAvailDbIdx) == FALSE)
			continue;

		rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
			prAdapter, prNegoCtrl->u4SchIdx,
			szAvailDbIdx, szSlotIdx,
			TRUE);

		u4RmtPrimaryChnl = rRmtChnlInfo.u4PrimaryChnl;
		u4RmtOperatingClass = rRmtChnlInfo.u4OperatingClass;

		/* Update this value to check that
		 * it's the case peer counter 6G and we already commit in 5G
		 * Set TRUE only for response/confirm.
		 */
		if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR &&
		    u4RmtPrimaryChnl != 0 &&
		    IS_6G_OP_CLASS(u4RmtOperatingClass) &&
		    rLocalChnlInfo.u4PrimaryChnl ==
			g_r5gDwChnl.u4PrimaryChnl) {
			if (pfgNotChoose6G)
				*pfgNotChoose6G = TRUE;
		}
	}
#endif

	if (rLocalChnlInfo.u4PrimaryChnl != 0) {
		if (pfgNotChoose6G && *pfgNotChoose6G)
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Committed ch:%u, not choose 6G\n",
				      szTimeLineIdx, szSlotIdx,
				      rLocalChnlInfo.u4PrimaryChnl);
		else
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Committed ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      rLocalChnlInfo.u4PrimaryChnl);
		return rLocalChnlInfo;
	}

	/* Check if it is initiator (NDP/Schedule request) */
	if (prNegoCtrl->eState == ENUM_NAN_CRB_NEGO_STATE_INITIATOR) {
		/* Check min of peer max cap and our max cap */
		if (NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx)) {

			rSelChnlInfo = g_r5gDwChnl;

			/* When the highest common is 5G Low, select channel
			 * NAN_5G_LOW_DISC_CHANNEL (44) to avoid counter
			 */
			if (eHighestCommonBand == ENUM_SUPPORTED_BN_5G_LOW)
				rSelChnlInfo.u4PrimaryChnl =
					NAN_5G_LOW_DISC_CHANNEL;

			nanGetMaxCapabilityAllPeers(prAdapter,
						    szSlotIdx,
						    &eAllPeerMaxCap,
						    &eAnyPeerMaxCap,
						    &eAllPeerAbandMaxCap,
						    &ucAllPeerMaxPhy);
			eMaxBand = nanSchedGetMaxCap(prAdapter);
			eMinBand = eMaxBand < eAllPeerAbandMaxCap ?
				       eMaxBand :
				       eAllPeerAbandMaxCap;

			if (prAdapter->rWifiVar.ucNan5gBandwidth
				== NAN_CHNL_BW_160)
				rSelChnlInfo = g_r5g160Chnl;

#if (CFG_SUPPORT_NAN_6G == 1)
			fgIs6GDefChnlAllowed =
				nanIsAllowedChannel(prAdapter, g_r6gDefChnl);
			if (eHighestCommonBand == ENUM_SUPPORTED_BN_6G &&
			    eMinBand == BAND_6G &&
			    fgIs6GDefChnlAllowed &&
			    !fgReschedForce5G)
				rSelChnlInfo = g_r6gDefChnl;
			else
#endif
				/* non-DBDC 2G only case */
				if (eMinBand == BAND_2G4)
					rSelChnlInfo = g_r2gDwChnl;
		} else {
			rSelChnlInfo = g_r2gDwChnl;

			prNegoTrans = &prNegoCtrl->rNegoTrans[u4NegoTransIdx];
			if (!prNegoTrans->fgPropose2gConditional) {
				DBGLOG(NAN, DEBUG, "Propose 2G conditional\n");
				prNegoTrans->fgPropose2gConditional = TRUE;
			}
		}

#if (CFG_SUPPORT_NAN_6G == 1)
		if (eMinBand == BAND_6G &&
		    kalMemCmp(&rSelChnlInfo, &g_r6gDefChnl,
			      sizeof(union _NAN_BAND_CHNL_CTRL)) != 0)
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Force ch:%u, ChnlAllowed:%u, ReschedForce5G:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      rSelChnlInfo.u4PrimaryChnl,
				      fgIs6GDefChnlAllowed,
				      fgReschedForce5G);
		else
#endif
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Min cap ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      rSelChnlInfo.u4PrimaryChnl);

		nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo, szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 FALSE, NULL);

		return rSelChnlInfo;
	}
	/* prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR */
	/* Responder (NDP/Schedule response/confirm) */
	/* If 5/6G already had intersection with peer, skip 2G chnl selection */
	if (nanGetActiveNdlNum(prAdapter) > 1 &&
	    NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
	    !NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G) && /* MCC case: all 2.4G */
	    nanSchedNegoChk56GIntersectBySlot(prAdapter,
					      prNegoCtrl->u4SchIdx,
					      szSlotIdx,
					      &local, &remote)) {
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) NDL slot(%zu): 5/6G has intersection (l=%u, r=%u), skip 2G\n",
			      szTimeLineIdx, szSlotIdx,
			      local, remote);
		return g_rNullChnl;
	}

	/* Check peer committed/conditional != 0 */
	for (szAvailDbIdx = 0;
		szAvailDbIdx < NAN_NUM_AVAIL_DB; szAvailDbIdx++) {
		uint32_t u4SelPrimaryChnl;
		uint32_t u4SelOperatingClass;

		if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
			prNegoCtrl->u4SchIdx, szAvailDbIdx) == FALSE)
			continue;

		rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
				prAdapter, prNegoCtrl->u4SchIdx,
				szAvailDbIdx, szSlotIdx,
				TRUE);

		u4RmtPrimaryChnl = rRmtChnlInfo.u4PrimaryChnl;
		if (u4RmtPrimaryChnl == 0)
			continue;

		eRmtBand = nanRegGetNanChnlBand(rRmtChnlInfo);
		if (!NAN_IS_TIMELINE_MATCH_BAND(prAdapter, szTimeLineIdx,
						eRmtBand))
			continue;

		/* for single map, skip 2G channel if highest > 2G */
		if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
		    eRmtBand == BAND_2G4 &&
		    eHighestCommonBand > ENUM_SUPPORTED_BN_2G)
			continue;

		fgPeerChnlExist = TRUE;

		if (!nanIsAllowedChannel(prAdapter, rRmtChnlInfo)) {
			struct _NAN_CRB_NEGO_TRANSACTION_T *prCurrNegoTrans;
			uint32_t u4NegoTransIdx =
				nanSchedGetCurrentNegoTransIdx(prAdapter);

			if (rRmtChnlInfo.u4PrimaryChnl ==
			    NAN_5G_HIGH_DISC_CHANNEL &&
			    (eHighestCommonBand != ENUM_SUPPORTED_BN_2G &&
			     eHighestCommonBand != ENUM_SUPPORTED_BN_NUM)) {
				prCurrNegoTrans =
					&prNegoCtrl->rNegoTrans[u4NegoTransIdx];

				/* JP allows 44 but not peer's proposal 149 */
				rSelChnlInfo = g_r5gDwChnl;
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					    "Tidx(%zu) NDL slot(%zu): Peer ch=%u not allowed, counter with %u!\n",
					    szTimeLineIdx, szSlotIdx,
					    u4RmtPrimaryChnl,
					    g_r5gDwChnl.u4PrimaryChnl);
				prCurrNegoTrans->fgCounterCountry = TRUE;
				return rSelChnlInfo;
			} else if (NAN_IS_CHANNEL_6G(rRmtChnlInfo) &&
				   eHighestCommonBand == ENUM_SUPPORTED_BN_6G) {
				/* 6G hit max channel limit, fallback to 5G */
				struct mt66xx_chip_info *prChipInfo;

				prChipInfo = prAdapter->chip_info;
				prCurrNegoTrans =
					&prNegoCtrl->rNegoTrans[u4NegoTransIdx];

				rSelChnlInfo = g_r5gDwChnl;
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					    "Tidx(%zu) NDL slot(%zu): Peer 6G ch=%u not allowed (hit limit %u?), counter with %u!\n",
					    szTimeLineIdx, szSlotIdx,
					    u4RmtPrimaryChnl,
					    prChipInfo == NULL ? 0 :
					    prChipInfo->ucMaxConcurrentLimit,
					    g_r5gDwChnl.u4PrimaryChnl);
				prCurrNegoTrans->fgCounterCountry = TRUE;
				return rSelChnlInfo;
			}

			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Peer commit/cond ch:%u not allowed!\n",
				      szTimeLineIdx, szSlotIdx,
				      u4RmtPrimaryChnl);
			continue;
		}

		rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
							rRmtChnlInfo);
		u4SelPrimaryChnl = rSelChnlInfo.u4PrimaryChnl;

		u4SelOperatingClass = rSelChnlInfo.u4OperatingClass;

		if (u4SelPrimaryChnl != 0) {
#if (CFG_SUPPORT_NAN_6G == 1)
			if (IS_6G_OP_CLASS(u4SelOperatingClass) &&
			    fgReschedForce5G) {
				/* Update this value to check that
				 * it's the case we can choose 6G but choose 5G.
				 * Set TRUE only for response/confirm.
				 */
				if (pfgNotChoose6G)
					*pfgNotChoose6G = TRUE;

				NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) NDL slot(%zu): Peer commit/cond ch:%u, but force 5G\n",
					      szTimeLineIdx, szSlotIdx,
					      u4SelPrimaryChnl);

				rSelChnlInfo = g_r5gDwChnl;
			} else
				NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) NDL slot(%zu): Peer commit/cond ch:%u\n",
					      szTimeLineIdx, szSlotIdx,
					      u4SelPrimaryChnl);
#else
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Peer commit/cond ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      u4SelPrimaryChnl);
#endif

			nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);

			return rSelChnlInfo;
		} else {
			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) NDL slot(%zu): Peer commit/cond ch:%u converge fail!\n",
				      szTimeLineIdx, szSlotIdx,
				      u4RmtPrimaryChnl);
		}

		NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
			      "Tidx(%u) NDL slot(%zu): Peer commit/cond CH%u converge fail!\n",
			      szTimeLineIdx, szSlotIdx,
			      u4RmtPrimaryChnl);
	}

	if (!fgPeerChnlExist)
		NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
			      "Tidx(%zu) NDL slot(%zu): Peer commit/cond not found!\n",
			      szTimeLineIdx, szSlotIdx);

	return g_rNullChnl;
}

/**
 * nanSchedNegoFindFCSlotCrb() - Determine ChnlInfo of at szSlotIdx
 *				  in szTimeLineIdx
 * @prAdapter:
 * @fgPrintLog:
 * @szTimeLineIdx: loop down from 5GHz at caller
 * @szSlotIdx: slot index in range [0,8192)
 *
 * Longer description
 * Select the FC slots according to the current concurrent connections.
 *
 * Context:
 * Called from nanSchedNegoGenDefCrbV2() (via nanSchedNegoFindSlotCrb) by
 * looping szTimeLineIdx downward with each szSlotIdx to set the channel info
 * by nanSchedAddCrbToChnlList().
 *
 * Return:
 */
static union _NAN_BAND_CHNL_CTRL
nanSchedNegoFindFCSlotCrb(struct ADAPTER *prAdapter,
			  unsigned char fgPrintLog,
			  size_t szTimeLineIdx,
			  size_t szSlotIdx)
{
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {.u4RawData = 0};
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = {.u4RawData = 0};
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	size_t szAvailDbIdx = 0;
	union _NAN_BAND_CHNL_CTRL rRmtChnlInfo = {.u4RawData = 0};
	enum ENUM_BAND eRmtBand = BAND_NULL;
	uint8_t fgPeerChnlExist = FALSE;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if (NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) ||
	    (NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx) &&
	     nanGetFeatureIsSigma(prAdapter))) {
		/* Sigma 5.3.2 must pass with 2G only,
		 * other cases skip 2G timeline
		 */
		if (NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
		    !(nanGetFeatureIsSigma(prAdapter) &&
		      nanCheck2gOnlyPeerExists(prAdapter)))
			return g_rNullChnl;

		/* Check committed channel != 0 */
		rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
							szSlotIdx,
							NULL,
							TRUE,
							szTimeLineIdx);
		if (rLocalChnlInfo.u4PrimaryChnl != 0) {
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) FC slot(%zu): Committed ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      rLocalChnlInfo.u4PrimaryChnl);
			return rLocalChnlInfo;
		}

		if (prNegoCtrl->eState == ENUM_NAN_CRB_NEGO_STATE_INITIATOR) {
			if (NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx))
				rSelChnlInfo = g_r2gDwChnl;
			else if (NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx))
				rSelChnlInfo = g_r5gDwChnl;

			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) FC slot(%zu): Conditional ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      rSelChnlInfo.u4PrimaryChnl);

			nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					FALSE, NULL);

			return rSelChnlInfo;
		}

		/* prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR */
		/* Responder (NDP/Schedule response/confirm) */
		/* Check peer committed/conditional != 0 */
		for (szAvailDbIdx = 0;
			szAvailDbIdx < NAN_NUM_AVAIL_DB; szAvailDbIdx++) {
			uint32_t u4RmtPrimaryChnl;
			uint32_t u4SelPrimaryChnl;

			if (nanSchedPeerAvailabilityDbValidByID(prAdapter,
				prNegoCtrl->u4SchIdx, szAvailDbIdx) == FALSE)
				continue;

			rRmtChnlInfo = nanGetPeerChnlInfoBySlot(
					prAdapter, prNegoCtrl->u4SchIdx,
					szAvailDbIdx, szSlotIdx,
					TRUE);

			u4RmtPrimaryChnl = rRmtChnlInfo.u4PrimaryChnl;
			if (u4RmtPrimaryChnl == 0)
				continue;

			eRmtBand = nanRegGetNanChnlBand(rRmtChnlInfo);
			if (!NAN_IS_TIMELINE_MATCH_BAND(prAdapter,
						szTimeLineIdx, eRmtBand))
				continue;

			fgPeerChnlExist = TRUE;

			if (!nanIsAllowedChannel(prAdapter,
					rRmtChnlInfo)) {
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) FC slot(%zu): Peer commit/cond ch:%u not allowed!\n",
					      szTimeLineIdx, szSlotIdx,
					      u4RmtPrimaryChnl);
				continue;
			}

			rSelChnlInfo = nanSchedConvergeChnlInfo(prAdapter,
								rRmtChnlInfo);
			u4SelPrimaryChnl = rSelChnlInfo.u4PrimaryChnl;

			if (u4SelPrimaryChnl != 0) {
				NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) FC slot(%zu): Peer commit/cond ch:%u\n",
					      szTimeLineIdx, szSlotIdx,
					      u4SelPrimaryChnl);

				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);

				return rSelChnlInfo;
			} else {
				NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
					      "Tidx(%zu) FC slot(%zu): Peer commit/cond ch:%u converge fail!\n",
					      szTimeLineIdx, szSlotIdx,
					      u4RmtPrimaryChnl);
			}

			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%u) FC slot(%zu): Peer commit/cond CH%u converge fail!\n",
				      szTimeLineIdx, szSlotIdx,
				      u4RmtPrimaryChnl);
		}

		if (!fgPeerChnlExist)
			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) FC slot(%zu): Peer commit/cond not found!\n",
				      szTimeLineIdx, szSlotIdx);

		return g_rNullChnl;
	}

	/* szTimeLineIdx == 5G */
	/* Check committed channel == CH149 */
	rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter, szSlotIdx, NULL,
						TRUE, szTimeLineIdx);
	if (rLocalChnlInfo.u4PrimaryChnl != 0) {
		if (rLocalChnlInfo.u4PrimaryChnl ==
				g_r5gDwChnl.u4PrimaryChnl) {
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) FC slot(%zu): Committed ch:%u\n",
				      szTimeLineIdx, szSlotIdx,
				      rLocalChnlInfo.u4PrimaryChnl);
			return rLocalChnlInfo;
		}

		/* rLocalChnlInfo u4PrimaryChnl != g_r5gDwChnl */
		rSelChnlInfo = g_r5gDwChnl;

			NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
				      "Tidx(%zu) FC slot(%zu): Change commit ch:%u to ch:%u!\n",
				      szTimeLineIdx, szSlotIdx,
				      rLocalChnlInfo.u4PrimaryChnl,
				      rSelChnlInfo.u4PrimaryChnl);

		/* Change committed to CH149 */
		nanSchedDeleteCrbFromChnlList(prAdapter,
			szSlotIdx, 1,
			ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
			TRUE, szTimeLineIdx);

		nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					 szSlotIdx, 1,
					 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					 TRUE, NULL);

		return rSelChnlInfo;
	}

	/* rLocalChnlInfo.u4PrimaryChnl == 0
	 * Set committed channel to CH149
	 */
	rSelChnlInfo = g_r5gDwChnl;

	NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
		      "Tidx(%zu) FC slot(%zu): Choose ch:%u\n",
		      szTimeLineIdx, szSlotIdx,
		      rSelChnlInfo.u4PrimaryChnl);

	nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo, szSlotIdx, 1,
				 ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
				 TRUE, NULL);

	return rSelChnlInfo;
}

/**
 *                         1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * 2G: DW     NDL     |      AIS      |      NDL      |      AIS
 * 5G:     AIS        |DW    NDL      |      AIS      |      NDL
 *                    |  FCFCFC       |                                #case 1
 *                    | NDC--         |                                #case 2
 *                    |  --NDC        |                                #case 3
 *                    | NDC(NDL==149) |                                #case 4
 * case 1: (no P2P || P2P in social channel) && FC configured
 *	   NDL=6G ch5 or 149. Last FC slot can be used for channel switch
 * case 2: (P2P active in non-social channel || max common == 6G) && !FC
 *	   NDL=P2P channel or 6G ch5. NDC=149, reserve a channel switch slot
 *	   FC not configured, set NDC=social channel, reserve ChnlSwitchSlot
 * case 3: Skip this case currently
 * case 4: (no P2P && max common != 6G || P2P in social channel)
 *	   NDL=149, no channel switch required.
 *
 * Summary:
 * #9=149(44) always, TODO: peer commit on other slots, lead to bad performance
 * #10=149 if FC else fill by NDL,
 * Reset #10 if !FC && #11 != 149(44) for channel switch
 *
 * @szSlotIdx: slot index [0..512) representing the range in 0~8192 TU
 */
static union _NAN_BAND_CHNL_CTRL
nanSchedNegoFindSlotCrb(struct ADAPTER *prAdapter,
			struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl,
			enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand,
			unsigned char fgPrintLog,
			size_t szTimeLineIdx,
			size_t szSlotIdx,
			unsigned char fgReschedForce5G,
			unsigned char *pfgNotChoose6G)
{
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo = g_rNullChnl;
	struct _NAN_SCHEDULER_T *prScheduler;

	prScheduler = nanGetScheduler(prAdapter);

	if (nanIsChnlSwitchSlot(prAdapter, fgPrintLog,
				szTimeLineIdx, szSlotIdx))
		return g_rNullChnl;

	if (prNegoCtrl->eType == ENUM_NAN_NEGO_RANGING) {
		const uint16_t u2NanRangingBand =
					prAdapter->rWifiVar.u2NanRangingBand;

		if (NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx)) {
			if (u2NanRangingBand == 2 &&
			    NAN_SLOT_IS_RANGING_BY_BAND(szTimeLineIdx,
							szSlotIdx,
							BAND_2G4)) {
				rSelChnlInfo = g_r2gDwChnl;
				nanSchedAddCrbToRangingChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
				return rSelChnlInfo;
			} else {
				return g_rNullChnl;
			}
		}

		/* 5G/6G timeline */
		if (NAN_SLOT_IS_RANGING_BY_BAND(szTimeLineIdx, szSlotIdx,
						BAND_5G)) {
			if (u2NanRangingBand == 2)
				return g_rNullChnl;

			rSelChnlInfo = g_r5gDwChnl;
#if (CFG_SUPPORT_NAN_6G == 1)
			if (u2NanRangingBand == 6)
				rSelChnlInfo = g_r6gDefChnl;
#endif

			nanSchedAddCrbToRangingChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
			nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
			return rSelChnlInfo;
		}

		return g_rNullChnl;
	}

	/* Special case for 2G NDC, all channel 6 or 6-6-6 followed by P2P */
	if ((NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
		nanGetActiveTimelineMgmtNum(prAdapter) > 1) ||
		(nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
		 eHighestCommonBand == ENUM_SUPPORTED_BN_2G &&
		 prScheduler->eLocalCommittedBand == ENUM_SUPPORTED_BN_2G)) {
		union _NAN_BAND_CHNL_CTRL rP2pChnlInfo = {0};

		if (NAN_SLOT_IS_NDC_BY_BAND(szTimeLineIdx, szSlotIdx,
					    BAND_2G4)) {
			rSelChnlInfo = g_r2gDwChnl;

			nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
			NAN_DW_DBGLOG(NAN, INFO, fgPrintLog, szSlotIdx,
				      "Force 2G NDC slot SchIdx=%u, timeline:%zu, slot=%zu, ch=%u\n",
				      prNegoCtrl->u4SchIdx, szTimeLineIdx,
				      szSlotIdx, rSelChnlInfo.u4PrimaryChnl);

			return rSelChnlInfo;
		}

		rP2pChnlInfo = nanGetActiveChnl(prAdapter, NETWORK_TYPE_P2P,
						BAND_2G4);
		/* need channel switch from 6 to P2P channel */
		if (!NAN_IS_P2P_AIS_MCC(prAdapter, BAND_2G4) &&
		    rP2pChnlInfo.u4PrimaryChnl &&
		    rP2pChnlInfo.u4PrimaryChnl != g_r2gDwChnl.u4PrimaryChnl &&
		    NAN_SLOT_IS_NDC_BY_BAND(szTimeLineIdx, szSlotIdx - 1,
					    BAND_2G4) &&
		    g_u4MaxChnlSwitchTimeUs >= CHANNEL_SWITCH_THRESHOLD) {
#if (NDC_NEXT_SLOT_CHANNEL == 0) /* slot #1 (NDC+1) = 0 */
			return g_rNullChnl;
#else /* slot #2 (NDC+1) = 6 */
			rSelChnlInfo = g_r2gDwChnl;
			nanSchedAddCrbToChnlList(prAdapter,
					&rSelChnlInfo, szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
			NAN_DW_DBGLOG(NAN, INFO, fgPrintLog, szSlotIdx,
				      "Force timeline %zu slot %zu (following NDC) to use channel %u\n",
				      szTimeLineIdx, szSlotIdx,
				      rSelChnlInfo.u4PrimaryChnl);

			return rSelChnlInfo;
#endif
		}
	}

	if (NAN_SLOT_IS_AIS(prAdapter, szTimeLineIdx, szSlotIdx)) {
		rSelChnlInfo = nanSchedNegoFindAisSlotCrb(prAdapter,
						eHighestCommonBand, fgPrintLog,
						szTimeLineIdx, szSlotIdx);
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Find AIS slot timeline:%zu, slot=%zu, ch=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rSelChnlInfo.u4PrimaryChnl);
		return rSelChnlInfo;
	}

	/* #9: always 5G NDC */
	if (NAN_SLOT_IS_NDC_BY_BAND(szTimeLineIdx, szSlotIdx, BAND_5G)) {

		union _NAN_BAND_CHNL_CTRL rLocalChnlInfo = {0};

		rLocalChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
							szSlotIdx,
							NULL, TRUE,
							szTimeLineIdx);
		/* Single map reschedule with diff country code */
		if (nanGetActiveTimelineMgmtNum(prAdapter) == 1 &&
		    rLocalChnlInfo.u4PrimaryChnl == 0 &&
		    nanGetNafHighestCommonBand(prAdapter) ==
		    ENUM_SUPPORTED_BN_2G &&
		    nanGetCurrentNdlState(prAdapter) ==
		    NDL_RESPONDER_RX_SCHEDULE_REQUEST) {
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				"Skip slot %u due to country code\n",
				szSlotIdx);
			return g_rNullChnl;
		}

		/* Use 5G social channel for NDC */
		if (likely(eHighestCommonBand != ENUM_SUPPORTED_BN_2G &&
			   !NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G))) {
			rSelChnlInfo = g_r5gDwChnl;

#if (CFG_SUPPORT_NAN_6G == 1)
			if (nanGetFeatureIsSigma(prAdapter) &&
			    eHighestCommonBand == ENUM_SUPPORTED_BN_6G) /* #9 */
				rSelChnlInfo = g_r6gDefChnl;
#endif
			nanSchedAddCrbToChnlList(prAdapter, &rSelChnlInfo,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, NULL);
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Force NDC slot SchIdx=%u, timeline:%zu, slot=%zu, ch=%u",
				      prNegoCtrl->u4SchIdx, szTimeLineIdx,
				      szSlotIdx, rSelChnlInfo.u4PrimaryChnl);

			return rSelChnlInfo;
		}

		NAN_DW_DBGLOG(NAN, WARN, fgPrintLog, szSlotIdx,
			      "%u, slot=%zu Highest Common Band=%u MCC=%u, decide NDC by NDL",
			      prNegoCtrl->u4SchIdx, szSlotIdx,
			      eHighestCommonBand,
			      NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G));
	}

	if (NAN_SLOT_IS_NDL(prAdapter, szTimeLineIdx, szSlotIdx)) {
		rSelChnlInfo = nanSchedNegoFindNdlSlotCrb(prAdapter,
						eHighestCommonBand, fgPrintLog,
						szTimeLineIdx, szSlotIdx,
						fgReschedForce5G,
						pfgNotChoose6G);

#if (NDC_NEXT_SLOT_CHANNEL == 0) /* slot #10 (NDC+1) = 0 */
		/**
		 * Checking the slot after 5G NDC slot, a slot might be reserved
		 * for channel switch from NDC (social) to NDL non-social
		 */
		if (g_u4MaxChnlSwitchTimeUs >= CHANNEL_SWITCH_THRESHOLD &&
		    NAN_SLOT_IS_NDC_BY_BAND(szTimeLineIdx, szSlotIdx - 1,
					    BAND_5G)) {
			union _NAN_BAND_CHNL_CTRL rNdcChnlInfo = {0};

			rNdcChnlInfo = nanQueryChnlInfoBySlot(prAdapter,
							szSlotIdx - 1,
							NULL, TRUE,
							szTimeLineIdx);
			NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
				      "Check timeline:%zu, slot=%zu, NDC ch=%u, NDC+1 ch=%u for channel switch(%u)\n",
				      szTimeLineIdx, szSlotIdx,
				      rNdcChnlInfo.u4PrimaryChnl,
				      rSelChnlInfo.u4PrimaryChnl,
				      g_u4MaxChnlSwitchTimeUs);

			if (rNdcChnlInfo.u4PrimaryChnl != 0 &&
			    nanSchedChkConcurrOp(rNdcChnlInfo, rSelChnlInfo) ==
					    CNM_CH_CONCURR_MCC) {
				nanSchedDeleteCrbFromChnlList(prAdapter,
					szSlotIdx, 1,
					ENUM_TIME_BITMAP_CTRL_PERIOD_8192,
					TRUE, szTimeLineIdx);
				rSelChnlInfo = g_rNullChnl;
			}
		}
#endif

		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Find NDL slot timeline:%zu, slot=%zu, ch=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rSelChnlInfo.u4PrimaryChnl);

		return rSelChnlInfo;
	}

	if (NAN_SLOT_TIMELINE_IS_FC(prAdapter, szTimeLineIdx, szSlotIdx)) {
		rSelChnlInfo = nanSchedNegoFindFCSlotCrb(prAdapter, fgPrintLog,
						szTimeLineIdx, szSlotIdx);
		NAN_DW_DBGLOG(NAN, DEBUG, fgPrintLog, szSlotIdx,
			      "Find FC slot timeline:%zu, slot=%zu, ch=%u\n",
			      szTimeLineIdx, szSlotIdx,
			      rSelChnlInfo.u4PrimaryChnl);

		return rSelChnlInfo;
	}

	return g_rNullChnl;
}

static u_int8_t nanPeerSupportOpClass(struct ADAPTER *prAdapter,
				      uint32_t u4SchIdx,
				      union _NAN_BAND_CHNL_CTRL rChnlInfo)
{
	u_int8_t fgSupported = FALSE;
	/* default return TRUE, only when peer schecule descriptor exist */
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	uint32_t u4CommonSupportedBand = 0;
	u_int8_t fgPeerSupport6g = FALSE;
	u_int8_t fgPeerSupport5gHigh = FALSE;
	u_int8_t fgPeerSupport5gLow = FALSE;
	u_int8_t fgPeerSupport2g = FALSE;

	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRecord == NULL)
		goto done;

	prPeerSchDesc = prPeerSchRecord->prPeerSchDesc;
	if (!prPeerSchDesc)
		goto done;

	u4CommonSupportedBand =
		nanGetConcurrentCommonBand(prAdapter, prPeerSchDesc,
					    FALSE);
	fgPeerSupport6g =
		!!(u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_6G));
	fgPeerSupport5gHigh =
		!!(u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_HIGH));
	fgPeerSupport5gLow =
		!!(u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_5G_LOW));
	fgPeerSupport2g =
		!!(u4CommonSupportedBand & BIT(ENUM_SUPPORTED_BN_2G));


	if (rChnlInfo.u4PrimaryChnl == 0)
		goto done;

	if (NAN_IS_CHANNEL_6G(rChnlInfo) && !fgPeerSupport6g)
		goto done;

	if (NAN_IS_CHANNEL_5G_HIGH(rChnlInfo) && !fgPeerSupport5gHigh)
		goto done;

	if (NAN_IS_CHANNEL_5G_LOW(rChnlInfo) && !fgPeerSupport5gLow)
		goto done;

	if (NAN_IS_CHANNEL_2G(rChnlInfo) && !fgPeerSupport2g)
		goto done;

	fgSupported = TRUE;

done:
	if (!fgSupported)
		DBGLOG(NAN, TRACE,
		       "Local(%u) oc=%u, ch=%u, peer support=%u/%u/%u/%u",
		       u4SchIdx,
		       rChnlInfo.u4OperatingClass, rChnlInfo.u4PrimaryChnl,
		       fgPeerSupport6g, fgPeerSupport5gHigh,
		       fgPeerSupport5gLow, fgPeerSupport2g);

	return fgSupported;
}

static u_int8_t nanPeerHas5G6GAvailability(struct ADAPTER *prAdapter,
					   uint32_t u4SchIdx)
{
	/* default return TRUE, only when peer schecule descriptor exist */
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc;
	struct _NAN_AVAILABILITY_DB_T *prNanAvailAttr;
	struct _NAN_AVAILABILITY_TIMELINE_T *prNanAvailEntry;
	union _NAN_BAND_CHNL_CTRL *prBandCtrl;
	union _NAN_BAND_CHNL_CTRL *prChnlCtrl;
	size_t i;
	size_t j;
	uint8_t fgSupport2G = FALSE;
	uint8_t fgSupport5G = FALSE;
	uint8_t fgSupport6G = FALSE;


	prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);
	if (prPeerSchRecord == NULL)
		goto done;

	prPeerSchDesc = prPeerSchRecord->prPeerSchDesc;
	if (!prPeerSchDesc)
		goto done;

	for (i = 0; i < NAN_NUM_AVAIL_DB; i++) {
		prNanAvailAttr = &prPeerSchDesc->arAvailAttr[i];
		if (prNanAvailAttr->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		for (j = 0; j < NAN_NUM_AVAIL_TIMELINE; j++) {
			prNanAvailEntry = &prNanAvailAttr->arAvailEntryList[j];
			if (prNanAvailEntry->fgActive == FALSE)
				continue;

			prChnlCtrl = prNanAvailEntry->arBandChnlCtrl;
			if (prChnlCtrl->u4Type ==
			    NAN_BAND_CH_ENTRY_LIST_TYPE_BAND) {
				prBandCtrl = prChnlCtrl;

				if (prBandCtrl->u4BandIdMask &
				    BIT(NAN_SUPPORTED_BAND_ID_2P4G))
					fgSupport2G = TRUE;

				if (prBandCtrl->u4BandIdMask &
				    BIT(NAN_SUPPORTED_BAND_ID_5G))
					fgSupport5G = TRUE;

				if (prBandCtrl->u4BandIdMask &
				    (BIT(NAN_PROPRIETARY_BAND_ID_6G) |
				     BIT(NAN_SUPPORTED_BAND_ID_6G)))
					fgSupport6G = TRUE;

				continue;
			}

			/* NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL */
			if (!prNanAvailEntry->rEntryCtrl.b1Committed &&
			    !prNanAvailEntry->rEntryCtrl.b1Conditional)
				continue;

			if (IS_2G_OP_CLASS(prChnlCtrl->u4OperatingClass))
				fgSupport2G = TRUE;
			if (IS_5G_OP_CLASS(prChnlCtrl->u4OperatingClass))
				fgSupport5G = TRUE;
			if (IS_6G_OP_CLASS(prChnlCtrl->u4OperatingClass))
				fgSupport6G = TRUE;
		}
	}


done:
	DBGLOG(NAN, INFO, "Check peer %u 2G=%u 5G=%u 6G=%u",
	       u4SchIdx, fgSupport2G, fgSupport5G, fgSupport6G);

	if (fgSupport2G)
		return fgSupport5G || fgSupport6G;

	return /* assume 5G/6G is supported if unknown */
		TRUE;
}

uint8_t nanCountConflictSlots(struct ADAPTER *prAdapter)
{
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	uint8_t committed[NAN_SLOTS_PER_DW_INTERVAL];
	uint8_t local[NAN_SLOTS_PER_DW_INTERVAL];

	union _NAN_BAND_CHNL_CTRL rPeerCommChnlInfo[NAN_SLOTS_PER_DW_INTERVAL];
	union _NAN_BAND_CHNL_CTRL rLocalChnlInfo[NAN_SLOTS_PER_DW_INTERVAL];

	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint8_t i;
	uint8_t n = 0;
	uint32_t u4ConflictMap = 0;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	for (i = 0; i < NAN_SLOTS_PER_DW_INTERVAL; i++) {
		rPeerCommChnlInfo[i] = nanGetPeerTimelineChnlInfoBySlot(
						prAdapter, prNegoCtrl->u4SchIdx,
						sz5gTimeLineIdx, i, FALSE);

		committed[i] = rPeerCommChnlInfo[i].u4PrimaryChnl;

		rLocalChnlInfo[i] = nanGetChnlInfoBySlot(prAdapter, i,
							 sz5gTimeLineIdx);
		local[i] = rLocalChnlInfo[i].u4PrimaryChnl;

		if ((rPeerCommChnlInfo[i].u4PrimaryChnl &&
		     rLocalChnlInfo[i].u4PrimaryChnl &&
		     rPeerCommChnlInfo[i].u4PrimaryChnl !=
		     rLocalChnlInfo[i].u4PrimaryChnl) ||
		    (rLocalChnlInfo[i].u4PrimaryChnl &&
		     !nanPeerSupportOpClass(prAdapter, prNegoCtrl->u4SchIdx,
					    rLocalChnlInfo[i]))) {
			n++;
			u4ConflictMap |= BIT(i);
		}
	}

	DBGLOG(NAN, DEBUG,
	       "Remote committed: %u-%u-%u-%u-%u-%u-%u-%u %u-%u-%u-%u-%u-%u-%u-%u %u-%u-%u-%u-%u-%u-%u-%u %u-%u-%u-%u-%u-%u-%u-%u",
	       committed[0], committed[1], committed[2], committed[3],
	       committed[4], committed[5], committed[6], committed[7],
	       committed[8], committed[9], committed[10], committed[11],
	       committed[12], committed[13], committed[14], committed[15],
	       committed[16], committed[17], committed[18], committed[19],
	       committed[20], committed[21], committed[22], committed[23],
	       committed[24], committed[25], committed[26], committed[27],
	       committed[28], committed[29], committed[30], committed[31]);

	DBGLOG(NAN, DEBUG,
	       "Local: %u-%u-%u-%u-%u-%u-%u-%u %u-%u-%u-%u-%u-%u-%u-%u %u-%u-%u-%u-%u-%u-%u-%u %u-%u-%u-%u-%u-%u-%u-%u, conflict=0x%08x",
	       local[0], local[1], local[2], local[3],
	       local[4], local[5], local[6], local[7],
	       local[8], local[9], local[10], local[11],
	       local[12], local[13], local[14], local[15],
	       local[16], local[17], local[18], local[19],
	       local[20], local[21], local[22], local[23],
	       local[24], local[25], local[26], local[27],
	       local[28], local[29], local[30], local[31],
	       u4ConflictMap);

	return n;
}

/* shall only skip 2G when:
 * 0. Normal NDP operation &&
 * 1. 5G/6G timeline suffiicent
 */
static u_int8_t nanSkipGen2GTimeline(struct ADAPTER *prAdapter,
				     u_int8_t fgChkRmtCondSlot,
				     uint8_t uc5gTimelineCommittedSlot,
				     uint8_t *ucConflictSlots)
{
	/* 5.5.2 requires DUT to send 2G availability in DP request (counter) */
	if (nanGetFeatureIsSigma(prAdapter))
		return FALSE;


	*ucConflictSlots = nanCountConflictSlots(prAdapter);

	/* 5G/6G slots is not sufficient need to propose 2G availability further
	 * for instance, half-2G-half-5G or P2p cuased custom committed in 5G/6G
	 */
	if (uc5gTimelineCommittedSlot - *ucConflictSlots < 16)
		return FALSE;

	return TRUE;
}

uint32_t nanSchedNegoGenDefCrbV2(struct ADAPTER *prAdapter,
					uint8_t fgChkRmtCondSlot)
{
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	union _NAN_BAND_CHNL_CTRL rSelChnlInfo;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t u4DwIdx = 0;
	unsigned char fgPeerAvailMapValid = FALSE;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);
	size_t szSlotIdx = 0, szSlotOffset = 0;
	uint32_t u4IterationNum = 0;
	uint32_t u4NanQuota = 0;
	unsigned char fgGenCrbSuccess = FALSE;
	uint32_t u4SuppBandIdMask = 0;
	uint32_t u4NegoTransIdx = 0;
	enum RESCHEDULE_SOURCE eReschedSrc = RESCHEDULE_NULL;
	uint32_t u4NegoSlot = 0xFFFFFFFF;
	unsigned char fgReschedForce5G = FALSE;
	unsigned char fgNotChoose6G = FALSE;
	uint32_t u4NotChoose6GTmpCnt = 0;
	enum _NAN_SUPPORTED_BAND_BIT eHighestCommonBand = ENUM_SUPPORTED_BN_NUM;
	uint32_t ucSlotChannel[NAN_SLOTS_PER_DW_INTERVAL];
	uint8_t ucSlotCommitted[NAN_TIMELINE_MGMT_SIZE] = {0};
	uint8_t ucDefaultChnl;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	if (prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_INITIATOR &&
	    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_RESPONDER &&
	    prNegoCtrl->eState != ENUM_NAN_CRB_NEGO_STATE_WAIT_RESP) {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto GEN_DFT_CRB_DONE;
	}

	fgPeerAvailMapValid = nanSchedPeerAvailabilityDbValid(prAdapter,
					prNegoCtrl->u4SchIdx);
	DBGLOG(NAN, DEBUG,
	       "SchIdx:%d, fgChkRmtCondSlot:%d, PeerAvailValid:%d\n",
	       prNegoCtrl->u4SchIdx, fgChkRmtCondSlot,
	       fgPeerAvailMapValid);

	if (fgChkRmtCondSlot && !fgPeerAvailMapValid) {
		rRetStatus = WLAN_STATUS_FAILURE;
		goto GEN_DFT_CRB_DONE;
	}

	u4NanQuota = NAN_SLOTS_PER_DW_INTERVAL;
#if (CFG_NAN_SCHEDULER_VERSION == 0)
	if (!fgChkRmtCondSlot)
		u4NanQuota -= prAdapter->rWifiVar.ucAisQuotaVal;
#endif

	/* Select nego slot for different reschedule source */
#if (CFG_SUPPORT_NAN_RESCHEDULE_CHANNEL_SELECTION == 1)
	u4NegoTransIdx = nanSchedGetCurrentNegoTransIdx(prAdapter);
	eReschedSrc = prNegoCtrl->rNegoTrans[u4NegoTransIdx].eReschedSrc;
#endif
	u4NegoSlot = nanSchedSelectNegoSlot(prAdapter, eReschedSrc);

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1 && CFG_SUPPORT_NAN_11BE == 1)
	if (prNegoCtrl->rNegoTrans[u4NegoTransIdx].fgIsEhtReschedule)
		u4NegoSlot = NAN_SLOT_MASK_TYPE_AIS |
			nanGetNdlSlots(prAdapter);
#endif

	/* For NEW_NDL, use half 6G and half 5G for reschedule performance.
	 * Peer sequence {N 6Gs,5G} need to trigger N reschedule,
	 * but if reschedule at X 6G peer to half 6G and half 5G,
	 * no need to reschedule for remain N-X 6G peer.
	 */
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	fgReschedForce5G = prNegoCtrl->fgInsistNDLSlot5GMode;
#endif
	DBGLOG(NAN, DEBUG, "NDL slot force 5G = %u\n", fgReschedForce5G);

	prNegoCtrl->rNegoTrans[u4NegoTransIdx].u4NotChoose6GCnt = 0;
	prNegoCtrl->rNegoTrans[u4NegoTransIdx].fgCounterCountry = FALSE;

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);

	DBGLOG(NAN, DEBUG, "Select u4NegoSlot = 0x%08x, eReschedSrc=%d\n",
	       u4NegoSlot, eReschedSrc);

	eHighestCommonBand =
		nanSchedGetHighestCommonBand(prAdapter, prNegoCtrl->u4SchIdx,
					     TRUE);

	/* have a preference for 5G band */
	for (szTimeLineIdx = szNanActiveTimelineNum; szTimeLineIdx--; ) {
		if (szTimeLineIdx == sz5gTimeLineIdx)
			ucDefaultChnl = g_r5gDwChnl.u4PrimaryChnl;
		else
			ucDefaultChnl = g_r2gDwChnl.u4PrimaryChnl;

		/* Sigma 5.3.2 must pass with 2G only, skip 5G timeline */
		if (nanGetFeatureIsSigma(prAdapter) &&
		    nanCheck2gOnlyPeerExists(prAdapter)) {
			if (!NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx))
				continue;
		}
		/* If highest common band is not 2G,
		 * skip 2G timeline nego to prevent side effect
		 * TODO: always nego 2G timeline if 2/5G TX to same peer
		 * is ready
		 *
		 * Cases to skip 2G channel:
		 * 0. In NDP negotiation
		 * 1. highest common band is not 2G, and
		 * 2. P2P & AIS is NOT MCC in 5G (New)
		 */
		else if (prNegoCtrl->eType == ENUM_NAN_NEGO_DATA_LINK &&
			 (eHighestCommonBand != ENUM_SUPPORTED_BN_2G &&
			  nanPeerHas5G6GAvailability(prAdapter,
						     prNegoCtrl->u4SchIdx)) &&
			 NAN_IS_2G_TIMELINE(prAdapter, szTimeLineIdx) &&
			 szNanActiveTimelineNum > 1 &&
			 /* !nanLinkNeedMlo(prAdapter) && */ /* FIXME */
			 !NAN_IS_P2P_AIS_MCC(prAdapter, BAND_5G)) {
			uint8_t ucConflictSlots = 0;

			if (nanSkipGen2GTimeline(prAdapter, fgChkRmtCondSlot,
					ucSlotCommitted[sz5gTimeLineIdx],
					&ucConflictSlots)) {
				DBGLOG(NAN, INFO,
				       "Skip 2G timeline, 5G/6G slots=%u, [effective=%u]",
				       ucSlotCommitted[sz5gTimeLineIdx],
				       ucSlotCommitted[sz5gTimeLineIdx] -
				       ucConflictSlots);
				continue;
			}

			DBGLOG(NAN, INFO,
			       "Check 2G timeline, fgChkRmtCondSlot=%u, 5G/6G slots=%u [effective=%u]",
			       fgChkRmtCondSlot,
			       ucSlotCommitted[sz5gTimeLineIdx],
			       ucSlotCommitted[sz5gTimeLineIdx] -
			       ucConflictSlots);
		}

		/* Skip 5G/6G timeline if P2P is active in 5G/6G but not in
		 * common band with peer
		 */
		if (NAN_IS_5G_TIMELINE(prAdapter, szTimeLineIdx)) {
			union _NAN_BAND_CHNL_CTRL rP2pChnlInfo = {0};
			union _NAN_BAND_CHNL_CTRL rAisChnlInfo = {0};

			/* TODO: macro NAN_NEED_SKIP_5G,
			 * or check in nanSchedGetAvailabilityAttr()?
			 */
			if (nanIsP2pAisMCC(prAdapter, szTimeLineIdx,
					   &rP2pChnlInfo, &rAisChnlInfo)) {
				DBGLOG(NAN, WARN,
				       "MCC P2P=%u AIS=%u, skip 5G/6G timeline",
				       rP2pChnlInfo.u4PrimaryChnl,
				       rAisChnlInfo.u4PrimaryChnl);
				continue;
			}

			if (rP2pChnlInfo.u4PrimaryChnl &&
			    !NAN_CHNL_IN_COMMON_BAND(prAdapter, prNegoCtrl,
						     rP2pChnlInfo)) {
				DBGLOG(NAN, WARN,
				       "P2P in %u, but peer does not support, skip 5G/6G timeline",
				       rP2pChnlInfo.u4PrimaryChnl);
				continue;
			}
		}

		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
				szTimeLineIdx);
		u4SuppBandIdMask = nanGetTimelineSupportedBand(prAdapter,
			szTimeLineIdx);

		if (prNanTimelineMgmt->ucMapId == NAN_INVALID_MAP_ID) {
			DBGLOG(NAN, DEBUG, "Skip invalid map, Tidx=%zu\n",
				szTimeLineIdx);
			continue;
		}

		if (u4SuppBandIdMask == 0)
			continue;

		/* try to allocate basic CRBs for every DW interval */
		kalMemZero(ucSlotChannel, sizeof(ucSlotChannel));
		for (u4DwIdx = 0; u4DwIdx < NAN_TOTAL_DW; u4DwIdx++) {
			szSlotOffset = fgChkRmtCondSlot ? 1 :
				prAdapter->rWifiVar.ucDftQuotaStartOffset;

			for (u4IterationNum = 0;
			     u4IterationNum < u4NanQuota;
			     szSlotOffset = (szSlotOffset + 1) % u4NanQuota,
			     u4IterationNum++) {

				szSlotIdx = NAN_FULL_SLOT_INDEX(u4DwIdx,
								szSlotOffset);

				if (nanWindowType(prAdapter, szSlotIdx,
					szTimeLineIdx) == ENUM_NAN_DW) {
					/* add 1 if u4DwIdx == 0 */
					ucSlotCommitted[szTimeLineIdx] +=
						u4DwIdx == 0;
					ucSlotChannel[szSlotOffset] =
						ucDefaultChnl;
					continue;
				}

				if ((BIT(szSlotOffset) & u4NegoSlot) == 0) {
					/* log not rescheduled channel info */
					ucSlotChannel[szSlotOffset] =
						nanQueryPrimaryChnlBySlot(
								prAdapter,
								szSlotIdx,
								TRUE,
								szTimeLineIdx);
					continue;
				}

				fgNotChoose6G = FALSE;

				rSelChnlInfo =
					nanSchedNegoFindSlotCrb(prAdapter,
							prNegoCtrl,
							eHighestCommonBand,
							TRUE,
							szTimeLineIdx,
							szSlotIdx,
							fgReschedForce5G,
							&fgNotChoose6G);
				ucSlotChannel[szSlotOffset] =
					rSelChnlInfo.u4PrimaryChnl;
				if (ucSlotChannel[szSlotOffset] && u4DwIdx == 0)
					ucSlotCommitted[szTimeLineIdx]++;

				if (fgNotChoose6G)
					u4NotChoose6GTmpCnt++;
			}
		}
		DBGLOG(NAN, INFO,
		       "sch=%u, timeline[%zu] slot channel: %u-%u-%u-%u-%u-%u-%u-%u  %u-%u-%u-%u-%u-%u-%u-%u  %u-%u-%u-%u-%u-%u-%u-%u  %u-%u-%u-%u-%u-%u-%u-%u",
		       prNegoCtrl->u4SchIdx, szTimeLineIdx,
		       ucSlotChannel[0], ucSlotChannel[1], ucSlotChannel[2],
		       ucSlotChannel[3], ucSlotChannel[4], ucSlotChannel[5],
		       ucSlotChannel[6], ucSlotChannel[7],
		       ucSlotChannel[8], ucSlotChannel[9], ucSlotChannel[10],
		       ucSlotChannel[11], ucSlotChannel[12], ucSlotChannel[13],
		       ucSlotChannel[14], ucSlotChannel[15],
		       ucSlotChannel[16], ucSlotChannel[17], ucSlotChannel[18],
		       ucSlotChannel[19], ucSlotChannel[20], ucSlotChannel[21],
		       ucSlotChannel[22], ucSlotChannel[23],
		       ucSlotChannel[24], ucSlotChannel[25], ucSlotChannel[26],
		       ucSlotChannel[27], ucSlotChannel[28], ucSlotChannel[29],
		       ucSlotChannel[30], ucSlotChannel[31]);

		prNegoCtrl->rNegoTrans[u4NegoTransIdx].u4NotChoose6GCnt =
			u4NotChoose6GTmpCnt;

		if ((rRetStatus == WLAN_STATUS_SUCCESS) ||
			(fgGenCrbSuccess == TRUE)) {
			fgGenCrbSuccess = TRUE;
			rRetStatus = WLAN_STATUS_SUCCESS;
		}
	}

#if (CFG_SUPPORT_NAN_NDP_DUAL_BAND == 0)
	nanSchedRemoveDiffBandChnlList(prAdapter);
#endif

GEN_DFT_CRB_DONE:

	nanSchedDbgDumpTimelineDb(prAdapter, __func__, __LINE__);
	return rRetStatus;
}
#endif

#if (CFG_SUPPORT_NAN_11BE == 1)
uint8_t nanSchedCheckEHTSlotExist(struct ADAPTER *prAdapter)
{
	size_t szIdx = 0;
	size_t szTimeLineIdx = 0;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	struct _NAN_CHANNEL_TIMELINE_T *prChnlTimeline = NULL;

	for (szTimeLineIdx = 0; szTimeLineIdx < NAN_TIMELINE_MGMT_SIZE;
	     szTimeLineIdx++) {
		prNanTimelineMgmt =
			nanGetTimelineMgmt(prAdapter, szTimeLineIdx);

		for (szIdx = 0;
			szIdx < NAN_TIMELINE_MGMT_CHNL_LIST_NUM; szIdx++) {
			prChnlTimeline = &prNanTimelineMgmt->arChnlList[szIdx];

			if (prChnlTimeline->fgValid == FALSE ||
			    prChnlTimeline->i4Num <= 0)
				continue;

			if (nanRegNanChnlBandIsEht(prChnlTimeline->rChnlInfo))
				return TRUE;
		}
	}

	return FALSE;
}
#endif

/* Check if NAN can follow P2P channel, with szTimelineIdx */
u_int8_t nanIsFollowP2pInNonSocialChannel(struct ADAPTER *prAdapter,
					  size_t szTimelineIdx,
					  union _NAN_BAND_CHNL_CTRL *prP2pChnl)
{
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	u_int8_t fgIsP2pAisMCC;
	union _NAN_BAND_CHNL_CTRL rP2pChnl = {0};
	union _NAN_BAND_CHNL_CTRL rAisChnl = {0};

	/**
	 * change 9452466
	 * Reason:
	 *   8: DW
	 *   9: NDC
	 *   10: channel switch
	 */
	fgIsP2pAisMCC = nanIsP2pAisMCC(prAdapter, szTimelineIdx,
				       &rP2pChnl, &rAisChnl);
	DBGLOG(NAN, DEBUG,
	       "Timeline:%zu(5g=%zu) p2p=%u, ais=%u mcc=%u, returning=%u\n",
	       szTimelineIdx, sz5gTimeLineIdx,
	       rP2pChnl.u4PrimaryChnl, rAisChnl.u4PrimaryChnl,
	       fgIsP2pAisMCC,
	       NAN_IS_5G_TIMELINE(prAdapter, szTimelineIdx) &&
	       rP2pChnl.u4PrimaryChnl != 0 &&
	       rP2pChnl.u4PrimaryChnl != g_r5gDwChnl.u4PrimaryChnl &&
	       (rAisChnl.u4PrimaryChnl == 0 ||
		rAisChnl.u4PrimaryChnl != 0 && !fgIsP2pAisMCC));

	/* Checking 5G timeline, P2P active but not 149 and not MCC with AIS,
	 * NAN can use the same channel.
	 * In this case, NAN use the same non-149 channel, but need a channel
	 * switch slot following NDC slot to switch from 149 to the P2P channel.
	 */
	if (NAN_IS_5G_TIMELINE(prAdapter, szTimelineIdx) &&
	    rP2pChnl.u4PrimaryChnl != 0 &&
	    rP2pChnl.u4PrimaryChnl != g_r5gDwChnl.u4PrimaryChnl &&
	    (rAisChnl.u4PrimaryChnl == 0 ||
	     rAisChnl.u4PrimaryChnl != 0 && !fgIsP2pAisMCC)) {
		if (prP2pChnl)
			*prP2pChnl = rP2pChnl;
		return TRUE;

	}
	if (prP2pChnl)
		*prP2pChnl = rP2pChnl;
	return FALSE;
}

/* Get Active Channel by Net and Band, based on nanSchedUpdateP2pAisMcc() */
union _NAN_BAND_CHNL_CTRL
nanGetActiveChnl(struct ADAPTER *prAdapter,
		 enum ENUM_NETWORK_TYPE eNetworkType, enum ENUM_BAND eBand)
{
	struct _NAN_SCHEDULER_T *prScheduler;
	size_t szTimeline;
	union _NAN_BAND_CHNL_CTRL rChnlInfo = g_rNullChnl;

	if (!prAdapter)
		return rChnlInfo;

	prScheduler = nanGetScheduler(prAdapter);
	if (!prScheduler)
		return rChnlInfo;

	szTimeline = nanGetTimelineMgmtIndexByBand(prAdapter, eBand);

	if (eNetworkType == NETWORK_TYPE_P2P)
		rChnlInfo = prScheduler->arP2pAisMcc[szTimeline].rP2pChnlInfo;
	else if (eNetworkType == NETWORK_TYPE_AIS)
		rChnlInfo = prScheduler->arP2pAisMcc[szTimeline].rAisChnlInfo;

	return rChnlInfo;
}

void nanSchedNegoUpdateNegoResult(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	struct _NAN_TIMELINE_MGMT_T *prNanTimelineMgmt = NULL;
	size_t szTimeLineIdx = 0;
	size_t szNanActiveTimelineNum = nanGetActiveTimelineMgmtNum(prAdapter);

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	/* step1: commit conditional windows */
	nanSchedNegoCommitCondChnlList(prAdapter);
	for (szTimeLineIdx = 0; szTimeLineIdx < szNanActiveTimelineNum;
	     szTimeLineIdx++) {
		prNanTimelineMgmt = nanGetTimelineMgmt(prAdapter,
			szTimeLineIdx);
		prNanTimelineMgmt->fgChkCondAvailability = FALSE;
	}
	/* step2: determine common FAW CRB */
	nanSchedPeerUpdateCommonFAW(prAdapter, prNegoCtrl->u4SchIdx);
	/* step3: release unused slots */
	nanSchedReleaseUnusedCommitSlot(prAdapter);

	nanSchedCmdUpdateAvailability(prAdapter);
}

uint32_t nanSchedGetCurrentNegoTransIdx(struct ADAPTER *prAdapter)
{
	struct _NAN_CRB_NEGO_CTRL_T *prNegoCtrl = NULL;
	uint32_t u4NegoTransIdx = 0;

	prNegoCtrl = nanGetNegoControlBlock(prAdapter);

	/* ucNegoTransHeadPos already +1 in nanSchedNegoDispatchTimeout
	 * Get origin ucNegoTransHeadPos by -1
	 */
	u4NegoTransIdx = prNegoCtrl->ucNegoTransHeadPos == 0 ?
			 NAN_CRB_NEGO_MAX_TRANSACTION - 1 :
			 prNegoCtrl->ucNegoTransHeadPos - 1;

	return u4NegoTransIdx;
}

void nanUpdateMbmcIdx(struct ADAPTER *ad,
	uint8_t ucBssIdx,
	uint8_t ucBandIdx)
{
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(ad,
		ucBssIdx);

	if (prBssInfo) {
#if (CFG_SUPPORT_NAN_DBDC == 1)
		if (prBssInfo->eBand == BAND_5G) {
			if (rlmDomainIsLegalChannel(ad,
			    BAND_5G,
			    NAN_5G_LOW_DISC_CHANNEL))
				prBssInfo->ucPrimaryChannel =
				NAN_5G_LOW_DISC_CHANNEL;
			if (rlmDomainIsLegalChannel(ad,
			    BAND_5G,
			    NAN_5G_HIGH_DISC_CHANNEL))
				prBssInfo->ucPrimaryChannel =
				NAN_5G_HIGH_DISC_CHANNEL;
		}
		if (prBssInfo->eBand == BAND_2G4)
			prBssInfo->ucPrimaryChannel =
				g_r2gDwChnl.u4PrimaryChnl;
#else
		prBssInfo->ucPrimaryChannel = 149;
#endif

		DBGLOG(CNM, INFO,
			"bss[%d]=%d, eHwBandIdx=%d, ucBandIdx=%d\n",
			ucBssIdx,
			prBssInfo->ucPrimaryChannel,
			prBssInfo->eHwBandIdx,
			ucBandIdx);

#ifdef NAN_UNUSED
		if (prBssInfo->eHwBandIdx != ucBandIdx &&
		    prBssInfo->eHwBandIdx != ENUM_BAND_AUTO)
			nicUniUpdateStaRecFastAll(ad, prBssInfo);
#endif
		/* TODO */
		prBssInfo->eBackupHwBandIdx = prBssInfo->eHwBandIdx;
		prBssInfo->eHwBandIdx = (enum ENUM_MBMC_BN)ucBandIdx;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		mldBssUpdateBandIdxBitmap(ad, prBssInfo);
#endif
	} else
		DBGLOG(CNM, ERROR, "ucBssIdx=%d, ucBandIdx=%d\n",
			ucBssIdx, ucBandIdx);
}

/**
 * Get the number of active peers which are not in power saving status with idle
 * timer paused
 */
uint32_t nanGetActivePeerNum(struct ADAPTER *prAdapter)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	uint32_t u4ActivePeerNum = 0;
	uint32_t u4SchIdx;

	/* count valid peer schedule record with idle timer not paused */
	for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

		if (!prPeerSchRecord->fgActive)
			continue;

		if (!prPeerSchRecord->fgInPowerSaving)
			u4ActivePeerNum++;
	}

	DBGLOG(NAN, INFO, "NAN current active peer num = %u\n",
	       u4ActivePeerNum);

	return u4ActivePeerNum;
}

uint32_t nanGetPowerSavingPeerNum(struct ADAPTER *prAdapter)
{
	struct _NAN_PEER_SCHEDULE_RECORD_T *prPeerSchRecord;
	uint32_t u4PowerSavingPeerNum = 0;
	uint32_t u4SchIdx;

	/* count valid peer schedule record with idle timer not paused */
	for (u4SchIdx = 0; u4SchIdx < NAN_MAX_CONN_CFG; u4SchIdx++) {
		prPeerSchRecord = nanSchedGetPeerSchRecord(prAdapter, u4SchIdx);

		if (!prPeerSchRecord->fgActive)
			continue;

		if (prPeerSchRecord->fgInPowerSaving)
			u4PowerSavingPeerNum++;
	}

	DBGLOG(NAN, INFO, "NAN current PS peer num = %u\n",
	       u4PowerSavingPeerNum);

	return u4PowerSavingPeerNum;
}
#endif /* CFG_SUPPORT_NAN */
