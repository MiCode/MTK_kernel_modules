/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3
 *     /include/mgmt/cnm.h#1
 */

/*! \file   "cnm.h"
 *    \brief
 */


#ifndef _CNM_H
#define _CNM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "nic_cmd_event.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define DBDC_5G_WMM_INDEX	0
#define DBDC_2G_WMM_INDEX	1
#define HW_WMM_NUM		(prAdapter->ucWmmSetNum)
#define MAX_HW_WMM_INDEX	(HW_WMM_NUM - 1)
#define DEFAULT_HW_WMM_INDEX	MAX_HW_WMM_INDEX
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

#if (CFG_SUPPORT_IDC_CH_SWITCH == 1)
enum ENUM_CH_SWITCH_TYPE {
	CH_SWITCH_2G, /* Default */
	CH_SWITCH_5G,
	CH_SWITCH_NUM
};
#endif

struct MSG_CH_REQ {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
	uint8_t ucPrimaryChannel;
	enum ENUM_CHNL_EXT eRfSco;
	enum ENUM_BAND eRfBand;

	/* To support 80/160MHz bandwidth */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;

	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */

#if (CFG_SUPPORT_802_11AX == 1)
	/* record original 20 /40 /80/160MHz bandwidth form AP's IE */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidthFromAP;

	/* record original 80/160MHz bandwidth form AP's IE */
	uint8_t ucRfCenterFreqSeg1FromAP;
	uint8_t ucRfCenterFreqSeg2FromAP;
#endif	/* CFG_SUPPORT_802_11AX == 1 */

	enum ENUM_CH_REQ_TYPE eReqType;
	uint32_t u4MaxInterval;	/* In unit of ms */
	enum ENUM_DBDC_BN eDBDCBand;
};

struct MSG_CH_ABORT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
	enum ENUM_DBDC_BN eDBDCBand;
};

struct MSG_CH_GRANT {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
	uint8_t ucPrimaryChannel;
	enum ENUM_CHNL_EXT eRfSco;
	enum ENUM_BAND eRfBand;

	/* To support 80/160MHz bandwidth */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;

	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */
	enum ENUM_CH_REQ_TYPE eReqType;
	uint32_t u4GrantInterval;	/* In unit of ms */
	enum ENUM_DBDC_BN eDBDCBand;
};

struct MSG_CH_REOCVER {
	struct MSG_HDR rMsgHdr;	/* Must be the first member */
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
	uint8_t ucPrimaryChannel;
	enum ENUM_CHNL_EXT eRfSco;
	enum ENUM_BAND eRfBand;

	/*  To support 80/160MHz bandwidth */
	enum ENUM_CHANNEL_WIDTH eRfChannelWidth;

	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */
	enum ENUM_CH_REQ_TYPE eReqType;
};

struct CNM_INFO {
	u_int8_t fgChGranted;
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
};

#if CFG_ENABLE_WIFI_DIRECT
/* Moved from p2p_fsm.h */
struct DEVICE_TYPE {
	uint16_t u2CategoryId;		/* Category ID */
	uint8_t aucOui[4];		/* OUI */
	uint16_t u2SubCategoryId;	/* Sub Category ID */
} __KAL_ATTRIB_PACKED__;
#endif

enum ENUM_CNM_DBDC_MODE {
	/* A/G traffic separate by WMM, but both
	 * TRX on band 0, CANNOT enable DBDC
	 */
	ENUM_DBDC_MODE_DISABLED,

	/* A/G traffic separate by WMM, WMM0/1
	 * TRX on band 0/1, CANNOT disable DBDC
	 */
	ENUM_DBDC_MODE_STATIC,

	/* Automatically enable/disable DBDC,
	 * setting just like static/disable mode
	 */
	ENUM_DBDC_MODE_DYNAMIC,

	ENUM_DBDC_MODE_NUM
};

#if CFG_SUPPORT_DBDC
enum ENUM_CNM_DBDC_SWITCH_MECHANISM { /* When DBDC available in dynamic DBDC */
	/* Switch to DBDC when available (less latency) */
	ENUM_DBDC_SWITCH_MECHANISM_LATENCY_MODE,

	/* Switch to DBDC when DBDC T-put > MCC T-put */
	ENUM_DBDC_SWITCH_MECHANISM_THROUGHPUT_MODE,

	ENUM_DBDC_SWITCH_MECHANISM_NUM
};
#endif	/* CFG_SUPPORT_DBDC */

enum ENUM_CNM_NETWORK_TYPE_T {
	ENUM_CNM_NETWORK_TYPE_OTHER,
	ENUM_CNM_NETWORK_TYPE_AIS,
	ENUM_CNM_NETWORK_TYPE_P2P_GC,
	ENUM_CNM_NETWORK_TYPE_P2P_GO,
	ENUM_CNM_NETWORK_TYPE_NUM
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
#define CNM_CH_GRANTED_FOR_BSS(_prAdapter, _ucBssIndex) \
	((_prAdapter)->rCnmInfo.fgChGranted && \
	 (_prAdapter)->rCnmInfo.ucBssIndex == (_ucBssIndex))

/* True if our TxNss > 1 && peer support 2ss rate && peer no Rx limit. */
#if (CFG_SUPPORT_WIFI_6G == 1)
#define IS_CONNECTION_NSS2(prBssInfo, prStaRec) \
	((((prBssInfo)->ucOpTxNss > 1) && \
	((prStaRec)->aucRxMcsBitmask[1] != 0x00) \
	&& (((prStaRec)->u2HtCapInfo & HT_CAP_INFO_SM_POWER_SAVE) != 0)) || \
	(((prBssInfo)->ucOpTxNss > 1) && ((((prStaRec)->u2VhtRxMcsMap \
	& BITS(2, 3)) >> 2) != BITS(0, 1)) && ((((prStaRec)->ucVhtOpMode \
	& VHT_OP_MODE_RX_NSS) >> VHT_OP_MODE_RX_NSS_OFFSET) > 0)) || \
	(((prBssInfo)->ucOpTxNss > 1) \
	&& ((prBssInfo)->eBand == BAND_6G) \
	&& ((((prStaRec)->u2HeRxMcsMapBW80 & BITS(2, 3)) >> 2) != BITS(0, 1)) \
	&& (((prStaRec)->u2He6gBandCapInfo \
	& HE_6G_CAP_INFO_SM_POWER_SAVE) != 0)))

#else
#define IS_CONNECTION_NSS2(prBssInfo, prStaRec) \
	((((prBssInfo)->ucOpTxNss > 1) && \
	((prStaRec)->aucRxMcsBitmask[1] != 0x00) \
	&& (((prStaRec)->u2HtCapInfo & HT_CAP_INFO_SM_POWER_SAVE) != 0)) || \
	(((prBssInfo)->ucOpTxNss > 1) && ((((prStaRec)->u2VhtRxMcsMap \
	& BITS(2, 3)) >> 2) != BITS(0, 1)) && ((((prStaRec)->ucVhtOpMode \
	& VHT_OP_MODE_RX_NSS) >> VHT_OP_MODE_RX_NSS_OFFSET) > 0)))
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void cnmInit(struct ADAPTER *prAdapter);

void cnmUninit(struct ADAPTER *prAdapter);

void cnmChMngrRequestPrivilege(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void cnmChMngrAbortPrivilege(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void cnmChMngrHandleChEvent(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);

#if (CFG_SUPPORT_IDC_CH_SWITCH == 1)
uint8_t cnmIdcCsaReq(IN struct ADAPTER *prAdapter,
       IN uint8_t ch_num, IN uint8_t ucRoleIdx);

void cnmIdcDetectHandler(IN struct ADAPTER *prAdapter,
       IN struct WIFI_EVENT *prEvent);

#if (CFG_SUPPORT_AUTO_SCC == 1)
void cnmSCCAutoSwitchMode(IN struct ADAPTER *prAdapter,
	IN uint8_t ucAISChannel);

u_int8_t cnmIsSCCAutoSwitch(IN struct ADAPTER *prAdapter,
	IN uint8_t ucAISChannel);
#endif /* CFG_SUPPORT_AUTO_SCC */

#if (CFG_SUPPORT_DFS_MASTER == 1)
void cnmRadarDetectEvent(struct ADAPTER *prAdapter,
       struct WIFI_EVENT *prEvent);

void cnmCsaDoneEvent(struct ADAPTER *prAdapter,
       struct WIFI_EVENT *prEvent);

uint8_t cnmSapChannelSwitchReq(IN struct ADAPTER *prAdapter,
       IN struct RF_CHANNEL_INFO *prRfChannelInfo,
       IN uint8_t ucRoleIdx);
#endif
#endif

u_int8_t cnmPreferredChannel(struct ADAPTER *prAdapter, enum ENUM_BAND *prBand,
	uint8_t *pucPrimaryChannel, enum ENUM_CHNL_EXT *prBssSCO);

u_int8_t cnmAisInfraChannelFixed(struct ADAPTER *prAdapter,
	enum ENUM_BAND *prBand, uint8_t *pucPrimaryChannel);

void cnmAisInfraConnectNotify(struct ADAPTER *prAdapter);

u_int8_t cnmAisIbssIsPermitted(struct ADAPTER *prAdapter);

u_int8_t cnmP2PIsPermitted(struct ADAPTER *prAdapter);

u_int8_t cnmBowIsPermitted(struct ADAPTER *prAdapter);

u_int8_t cnmBss40mBwPermitted(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

u_int8_t cnmBss80mBwPermitted(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t cnmGetBssMaxBw(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

uint8_t cnmGetBssMaxBwToChnlBW(struct ADAPTER *prAdapter, uint8_t ucBssIndex);

#if CFG_SUPPORT_DBDC
u_int8_t cnmDBDCIsReqPeivilegeLock(void);
#endif

struct BSS_INFO *cnmGetBssInfoAndInit(struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType, u_int8_t fgIsP2pDevice);

void cnmFreeBssInfo(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo);
#if CFG_SUPPORT_CHNL_CONFLICT_REVISE
u_int8_t cnmAisDetectP2PChannel(struct ADAPTER *prAdapter,
	enum ENUM_BAND *prBand, uint8_t *pucPrimaryChannel);
#endif

u_int8_t cnmWmmIndexDecision(IN struct ADAPTER *prAdapter,
	IN struct BSS_INFO *prBssInfo);
void cnmFreeWmmIndex(IN struct ADAPTER *prAdapter,
	IN struct BSS_INFO *prBssInfo);

#if CFG_SUPPORT_DBDC
void cnmInitDbdcSetting(IN struct ADAPTER *prAdapter);

void cnmUpdateDbdcSetting(IN struct ADAPTER *prAdapter, IN u_int8_t fgDbdcEn);

uint8_t cnmGetDbdcBwCapability(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex
);

void cnmDbdcPreConnectionEnableDecision(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucChangedBssIndex,
	IN enum ENUM_BAND eRfBand,
	IN uint8_t ucPrimaryChannel,
	IN uint8_t ucWmmQueIdx
);

void cnmDbdcRuntimeCheckDecision(IN struct ADAPTER *prAdapter,
	IN uint8_t ucChangedBssIndex,
	IN u_int8_t ucForceLeaveEnGuard);

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
u_int8_t cnmDbdcIsP2pListenDbdcEn(void);
#endif

void cnmDbdcGuardTimerCallback(IN struct ADAPTER *prAdapter,
	IN unsigned long plParamPtr);
void cnmDbdcEventHwSwitchDone(IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent);
u_int8_t cnmDbdcIsWaitHwDisable(IN struct ADAPTER *prAdapter);
#endif /*CFG_SUPPORT_DBDC*/

enum ENUM_CNM_NETWORK_TYPE_T cnmGetBssNetworkType(struct BSS_INFO *prBssInfo);

u_int8_t cnmSapIsActive(IN struct ADAPTER *prAdapter);

u_int8_t cnmSapIsConcurrent(IN struct ADAPTER *prAdapter);

struct BSS_INFO *cnmGetSapBssInfo(IN struct ADAPTER *prAdapter);

void cnmOpModeGetTRxNss(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucBssIndex,
	OUT uint8_t *pucOpRxNss,
	OUT uint8_t *pucOpTxNss
);

void cnmOpmodeEventHandler(
	IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent
);

struct BSS_INFO *cnmGetP2pBssInfo(struct ADAPTER *prAdapter);

#if (CFG_SUPPORT_P2P_CSA_ACS == 1)
uint8_t cnmCheckStateForCsa(IN struct ADAPTER *prAdapter);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#ifndef _lint
/* We don't have to call following function to inspect the data structure.
 * It will check automatically while at compile time.
 * We'll need this to guarantee the same member order in different structures
 * to simply handling effort in some functions.
 */
static __KAL_INLINE__ void cnmMsgDataTypeCheck(void)
{
	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, rMsgHdr)
			== 0);

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, rMsgHdr)
			== OFFSET_OF(struct MSG_CH_REOCVER, rMsgHdr));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, ucBssIndex)
			== OFFSET_OF(struct MSG_CH_REOCVER, ucBssIndex));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, ucTokenID)
			== OFFSET_OF(struct MSG_CH_REOCVER, ucTokenID));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, ucPrimaryChannel)
			== OFFSET_OF(struct MSG_CH_REOCVER, ucPrimaryChannel));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, eRfSco)
			== OFFSET_OF(struct MSG_CH_REOCVER, eRfSco));

	DATA_STRUCT_INSPECTING_ASSERT(OFFSET_OF(
		struct MSG_CH_GRANT, eRfBand)
			== OFFSET_OF(struct MSG_CH_REOCVER, eRfBand));

	DATA_STRUCT_INSPECTING_ASSERT(
		OFFSET_OF(struct MSG_CH_GRANT, eReqType)
			== OFFSET_OF(struct MSG_CH_REOCVER, eReqType));
}
#endif /* _lint */

#endif /* _CNM_H */
