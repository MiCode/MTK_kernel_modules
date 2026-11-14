/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "bss.h"
 *    \brief  In this file we define the function prototype used in BSS/IBSS.
 *
 *    The file contains the function declarations and defines
 *						for used in BSS/IBSS.
 */


#ifndef _BSS_H
#define _BSS_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "wlan_def.h"
extern const uint8_t *apucNetworkType[NETWORK_TYPE_NUM];

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* Define how many concurrent operation networks. */
#define BSS_P2P_NUM             KAL_P2P_NUM

#if (BSS_P2P_NUM > MAX_BSSID_NUM)
#error Exceed HW capability (KAL_BSS_NUM or KAL_P2P_NUM)!!
#endif

/* NOTE(Kevin): change define for george */
/* #define MAX_LEN_TIM_PARTIAL_BMP     (((MAX_ASSOC_ID + 1) + 7) / 8) */
/* Required bits = (MAX_ASSOC_ID + 1) */
#define MAX_LEN_TIM_PARTIAL_BMP                     ((CFG_STA_REC_NUM + 7) / 8)
/* reserve length greater than maximum size of STA_REC */
/* obsoleted: Assume we only use AID:1~15 */

/* CTRL FLAGS for Probe Response */
#define BSS_PROBE_RESP_USE_P2P_DEV_ADDR             BIT(0)
#define BSS_PROBE_RESP_INCLUDE_P2P_IE               BIT(1)

#define IS_BSS_ALIVE(_prAdapter, _prBssInfo) \
	(_prBssInfo && \
	_prBssInfo->fgIsInUse && \
	_prBssInfo->fgIsNetActive && \
	(_prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED || \
	(_prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT && \
	IS_NET_PWR_STATE_ACTIVE(_prAdapter, \
	_prBssInfo->ucBssIndex))))

#define IS_BSS_NOT_ALIVE(_prAdapter, _prBssInfo) \
	(!IS_BSS_ALIVE(_prAdapter, _prBssInfo))

#define IS_BSS_P2P_DEV(_prAdapter, _prBssInfo) \
	(_prAdapter && _prBssInfo && \
	 _prAdapter->ucP2PDevBssIdx == _prBssInfo->ucBssIndex)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

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
#define GET_BSS_INFO_BY_INDEX(_prAdapter, _ucBssIndex) \
	(IS_BSS_INDEX_VALID(_ucBssIndex) ? \
		(_prAdapter)->aprBssInfo[(_ucBssIndex)] : NULL)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/* Routines for all Operation Modes                                           */
/*----------------------------------------------------------------------------*/
u_int8_t IS_BSS_INDEX_VALID(uint8_t ucBssIndex);

uint32_t bssInfoConnType(struct ADAPTER *ad, struct BSS_INFO *bssinfo);

struct STA_RECORD *
bssCreateStaRecFromBssDesc(struct ADAPTER *prAdapter,
			   enum ENUM_STA_TYPE eStaType, uint8_t uBssIndex,
			   struct BSS_DESC *prBssDesc);

void bssComposeNullFrame(struct ADAPTER *prAdapter,
			 uint8_t *pucBuffer, struct STA_RECORD *prStaRec);

void
bssComposeQoSNullFrame(struct ADAPTER *prAdapter,
		       uint8_t *pucBuffer, struct STA_RECORD *prStaRec,
		       uint8_t ucUP, u_int8_t fgSetEOSP);

uint32_t
bssSendNullFrame(struct ADAPTER *prAdapter,
		 struct STA_RECORD *prStaRec,
		 PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint32_t
bssSendQoSNullFrame(struct ADAPTER *prAdapter,
		    struct STA_RECORD *prStaRec, uint8_t ucUP,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler);

void bssDumpBssInfo(struct ADAPTER *prAdapter,
		    uint8_t ucBssIndex);

void bssDetermineApBssInfoPhyTypeSet(struct ADAPTER
				     *prAdapter, u_int8_t fgIsPureAp,
				     struct BSS_INFO *prBssInfo);

void bssUpdateStaRecFromBssDesc(struct ADAPTER *prAdapter,
				struct BSS_DESC *prBssDesc,
				struct STA_RECORD *prStaRec);

int8_t bssGetHtRxNss(struct BSS_DESC *prBssDesc);
int8_t bssGetVhtRxNss(struct BSS_DESC *prBssDesc);
int8_t bssGetHeRxNss(struct BSS_DESC *prBssDesc);
int8_t bssGetEhtRxNss(struct BSS_DESC *prBssDesc);
int8_t bssGetRxNss(struct BSS_DESC *prBssDesc);

#if CFG_SUPPORT_IOT_AP_BLOCKLIST
uint32_t bssGetIotApAction(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc);
bool bssIsIotAp(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, enum ENUM_WLAN_IOT_ACTION eAction);
#endif

const char *bssOpBw2Str(struct BSS_INFO *prBssInfo);

uint32_t bssGetAliveBssByBand(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, struct BSS_INFO **prBssList);

const char *bssGetRoleTypeString(struct ADAPTER *prAdapter,
				 struct BSS_INFO *bss);

#if CFG_ENABLE_WIFI_DIRECT
void bssGetAliveBssHwBitmap(struct ADAPTER *prAdapter, uint32_t *pau4Bitmap);

/*----------------------------------------------------------------------------*/
/* Routines for both IBSS(AdHoc) and BSS(AP)                                  */
/*----------------------------------------------------------------------------*/
void bssGenerateExtSuppRate_IE(struct ADAPTER *prAdapter,
			       struct MSDU_INFO *prMsduInfo);

void
bssBuildBeaconProbeRespFrameCommonIEs(struct MSDU_INFO
				      *prMsduInfo,
				      struct BSS_INFO *prBssInfo,
				      uint8_t *pucDestAddr);

void
bssComposeBeaconProbeRespFrameHeaderAndFF(
	uint8_t *pucBuffer,
	uint8_t *pucDestAddr,
	uint8_t *pucOwnMACAddress,
	uint8_t *pucBSSID, uint16_t u2BeaconInterval,
	uint16_t u2CapInfo);

uint32_t
bssSendBeaconProbeResponse(struct ADAPTER *prAdapter,
				uint8_t uBssIndex, uint8_t *pucDestAddr,
				uint32_t u4ControlFlags);

uint32_t bssProcessProbeRequest(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb);

void bssInitializeClientList(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo);

void bssAddClient(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo,
				struct STA_RECORD *prStaRec);

u_int8_t bssRemoveClient(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo,
				struct STA_RECORD *prStaRec);

struct STA_RECORD *bssRemoveClientByMac(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo,
				uint8_t *pucMac);

struct STA_RECORD *bssGetClientByMac(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo,
				uint8_t *pucMac);

struct STA_RECORD *bssRemoveHeadClient(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo);

uint32_t bssGetClientCount(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo);

void bssDumpClientList(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo);

void bssCheckClientList(struct ADAPTER *prAdapter,
				struct BSS_INFO *prBssInfo);

uint32_t bssUpdateBeaconContent(struct ADAPTER
				*prAdapter, uint8_t uBssIndex);

uint32_t bssUpdateBeaconContentEx(struct ADAPTER
				*prAdapter, uint8_t uBssIndex,
				enum ENUM_IE_UPD_METHOD eMethod);
/*----------------------------------------------------------------------------*/
/* Routines for BSS(AP) only                                                  */
/*----------------------------------------------------------------------------*/
void bssInitForAP(struct ADAPTER *prAdapter,
		  struct BSS_INFO *prBssInfo, u_int8_t fgIsRateUpdate);
#endif /* CFG_ENABLE_WIFI_DIRECT */

void bssUpdateDTIMCount(struct ADAPTER *prAdapter,
			uint8_t uBssIndex);

void bssSetTIMBitmap(struct ADAPTER *prAdapter,
		     struct BSS_INFO *prBssInfo, uint16_t u2AssocId);

/*link function to p2p module for txBcnIETable*/

/* WMM-2.2.2 WMM ACI to AC coding */
enum ENUM_ACI {
	ACI_BE = 0,
	ACI_BK = 1,
	ACI_VI = 2,
	ACI_VO = 3,
	ACI_NUM
};

enum ENUM_AC_PRIORITY {
	AC_BK_PRIORITY = 0,
	AC_BE_PRIORITY,
	AC_VI_PRIORITY,
	AC_VO_PRIORITY
};

#if (CFG_SUPPORT_HE_ER == 1)
struct EVENT_ER_TX_MODE {
	uint8_t ucBssInfoIdx;
	uint8_t ucErMode;
};

void bssProcessErTxModeEvent(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif

#endif /* _BSS_H */
