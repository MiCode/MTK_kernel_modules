/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "mlr.h"
 *    \brief  The declaration of MLR functions
 *
 *    Interfaces for MLR related handling functions
 */


#ifndef _MLR_H
#define _MLR_H

#if CFG_SUPPORT_MLR
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define ELEM_MLR_MTK_OUI_LEN                        10

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

enum ENUM_MLR_MODE {
	MLR_MODE_NOT_SUPPORT = 0,
	MLR_MODE_MLR_V1 = BIT(0),
	MLR_MODE_MLR_V2 = BIT(1),
	MLR_MODE_MLR_PLUS = BIT(2),
	MLR_MODE_ALR = BIT(3),
	MLR_MODE_DUAL_CTS = BIT(4)
};

enum ENUM_MLR_STATE {
	/* MLR not start */
	MLR_STATE_IDLE = 0,
	/* MLR start */
	MLR_STATE_START = 1,
	MLR_STATE_NUM /* 2 */
};

#if (CFG_SUPPORT_BALANCE_MLRV2 == 1)
enum ENUM_MLR_BALANCE_MODE {
	MLR_BALANCE_MODE_NOT_SUPPORT = 0,
	MLR_BALANCE_MODE_SAP = BIT(0),
	MLR_BALANCE_MODE_P2P = BIT(1)
};
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define MLR_BIT_SUPPORT(u4MlrBitmap) \
	((u4MlrBitmap & (MLR_MODE_MLR_V1 \
	| MLR_MODE_MLR_V2 \
	| MLR_MODE_MLR_PLUS \
	| MLR_MODE_ALR \
	| MLR_MODE_DUAL_CTS)) ? TRUE : FALSE)

#define MLR_BIT_V1_SUPPORT(u4MlrBitmap) \
	((u4MlrBitmap & MLR_MODE_MLR_V1) ? TRUE : FALSE)

#define MLR_BIT_V2_SUPPORT(u4MlrBitmap) \
	((u4MlrBitmap & MLR_MODE_MLR_V2) ? TRUE : FALSE)

#define MLR_V2_OR_ABOVE_SUPPORT(u4MlrBitmap) \
	((u4MlrBitmap & (MLR_MODE_MLR_V2 \
	| MLR_MODE_MLR_PLUS \
	| MLR_MODE_ALR \
	| MLR_MODE_DUAL_CTS)) ? TRUE : FALSE)

#define MLR_BIT_V1_V2_SUPPORT(u4MlrBitmap) \
	(((u4MlrBitmap & (MLR_MODE_MLR_V1 | MLR_MODE_MLR_V2)) \
	== (MLR_MODE_MLR_V1 | MLR_MODE_MLR_V2)) ? TRUE : FALSE)

#define MLR_BIT_INTERSECT(u4MlrBitmapA, ucMlrBitmapB) \
	(u4MlrBitmapA & ucMlrBitmapB)

#define MLR_STATE_IN_START(prStaRec) \
	(prStaRec->ucMlrState == MLR_STATE_START)

/* Check if DUT supports at least one MLR */
#define MLR_IS_SUPPORT(prAdapter) \
	(prAdapter->u4MlrSupportBitmap != MLR_MODE_NOT_SUPPORT)

/* Check if Peer supports at least one MLR */
#define MLR_IS_PEER_SUPPORT(prStaRec) \
	(prStaRec->ucMlrSupportBitmap != MLR_MODE_NOT_SUPPORT)

/* Check if both DUT and Peer support at least one MLR */
#define MLR_IS_BOTH_SUPPORT(prAdapter, prStaRec) \
	((prAdapter->u4MlrSupportBitmap != MLR_MODE_NOT_SUPPORT) \
	&& (prStaRec->ucMlrSupportBitmap != MLR_MODE_NOT_SUPPORT))

#define MLR_IS_V1_AFTER_INTERSECT(prAdapter, prStaRec) \
	((prAdapter->u4MlrSupportBitmap \
	& prStaRec->ucMlrSupportBitmap) == MLR_MODE_MLR_V1)

#define MLR_IS_V2_AFTER_INTERSECT(prAdapter, prStaRec) \
	((prAdapter->u4MlrSupportBitmap \
	& prStaRec->ucMlrSupportBitmap) == MLR_MODE_MLR_V2)

#define MLR_IS_V1V2_AFTER_INTERSECT(prAdapter, prStaRec) \
	((prAdapter->u4MlrSupportBitmap \
	& prStaRec->ucMlrSupportBitmap) \
	== (MLR_MODE_MLR_V1 | MLR_MODE_MLR_V2))

#define MLR_IS_MLRP_AFTER_INTERSECT(prAdapter, prStaRec) \
	((prAdapter->u4MlrSupportBitmap \
	& prStaRec->ucMlrSupportBitmap) == MLR_MODE_MLR_PLUS)

#define MLR_IS_ALR_AFTER_INTERSECT(prAdapter, prStaRec) \
	((prAdapter->u4MlrSupportBitmap \
	& prStaRec->ucMlrSupportBitmap) == MLR_MODE_ALR)

/* Check if intersection of both support MLRv1 or above */
#define MLR_IS_V1_OR_ABOVE_AFTER_INTERSECT(prAdapter, prStaRec) \
	(MLR_BIT_SUPPORT(MLR_BIT_INTERSECT( \
	prAdapter->u4MlrSupportBitmap, prStaRec->ucMlrSupportBitmap)))

#define MLR_BAND_IS_SUPPORT(eBand) \
	(eBand == BAND_5G)

#define MLR_GET_BAND(prAdapter, prStaRec) \
	(((prStaRec->ucBssIndex < ARRAY_SIZE(prAdapter->aprBssInfo)) && \
	(prAdapter->aprBssInfo[prStaRec->ucBssIndex] != NULL)) ? \
	prAdapter->aprBssInfo[prStaRec->ucBssIndex]->eBand : BAND_NULL)

#define MLR_CHECK_IF_RCPI_IS_LOW(prAdapter, ucRCPI) \
	(ucRCPI < prAdapter->rWifiVar.ucTxMlrRateRcpiThr)

#if ((CFG_SUPPORT_BALANCE_MLRV2 == 1) || (CFG_SUPPORT_BALANCE_MLRP_ALR == 1))
/* Consider SAP and GO TX MGMT may use MLR rate */
#define MLR_CHECK_IF_MGMT_USE_MLR_RATE(u2FrameCtrl) \
	(u2FrameCtrl == MAC_FRAME_AUTH \
	|| u2FrameCtrl == MAC_FRAME_ASSOC_REQ \
	|| u2FrameCtrl == MAC_FRAME_ASSOC_RSP \
	|| u2FrameCtrl == MAC_FRAME_REASSOC_REQ \
	|| u2FrameCtrl == MAC_FRAME_REASSOC_RSP)
#else
#define MLR_CHECK_IF_MGMT_USE_MLR_RATE(u2FrameCtrl) \
	(u2FrameCtrl == MAC_FRAME_AUTH \
	|| u2FrameCtrl == MAC_FRAME_ASSOC_REQ \
	|| u2FrameCtrl == MAC_FRAME_REASSOC_REQ)
#endif

#define MLR_CHECK_IF_PKT_LEN_DO_FRAG(prAdapter, prNativePacket) \
	(kalQueryPacketLength(prNativePacket) \
	> prAdapter->rWifiVar.u2TxFragThr)

#define MLR_CHECK_IF_MSDU_IS_FRAG(prMsduInfo) \
	(prMsduInfo->eFragPos != MSDU_FRAG_POS_NONE)

#define MLR_ENABLE_TX_FRAG(prStaRec) \
	(prStaRec->fgEnableTxFrag = TRUE)

#define MLR_DISABLE_TX_FRAG(prStaRec) \
	(prStaRec->fgEnableTxFrag = FALSE)

#define MLR_CHECK_IF_ENABLE_TX_FRAG(prStaRec) \
	prStaRec->fgEnableTxFrag

#define MLR_CHECK_IF_ENABLE_DEBUG(prAdapter) \
	prAdapter->rWifiVar.fgEnTxFragDebug

#if (CFG_SUPPORT_BALANCE_MLRV2 == 1)
#define MLR_CHECK_IF_ENABLE_SAP(prAdapter) \
	((prAdapter->rWifiVar.u4MlrCfgSapP2pEn \
	& MLR_BALANCE_MODE_SAP) == MLR_BALANCE_MODE_SAP)

#define MLR_CHECK_IF_ENABLE_P2P(prAdapter) \
	((prAdapter->rWifiVar.u4MlrCfgSapP2pEn \
	& MLR_BALANCE_MODE_P2P) == MLR_BALANCE_MODE_P2P)
#endif

#define MLR_CHECK_IF_MLRIE_EXIST(prAdapter) \
	prAdapter->rWifiVar.ucMlrBcnMlrIe

#define MLR_DBGLOG(prAdapter, _Mod, _Clz, _Fmt, ...) \
	do { \
		if (!MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) \
			break; \
		if ((au2DebugModule[DBG_##_Mod##_IDX] & \
			 DBG_CLASS_##_Clz) == 0) \
			break; \
		LOG_FUNC("[%u]%s:(" #_Mod " " #_Clz ") " _Fmt, \
			 KAL_GET_CURRENT_THREAD_ID(), \
			 __func__, ##__VA_ARGS__); \
	} while (0)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

u_int8_t mlrDoFragPacket(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		uint16_t u2SplitSize,
		uint16_t u2SplitThreshold,
		void *prNativePacket,
		struct QUE *prFragmentedQue);

u_int8_t mlrCheckIfDoFrag(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		void *prNativePacket);

void mlrDetermineRateCode(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		uint16_t *pu2RateCode);

u_int8_t mlrCanEnterMlrStart(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		enum ENUM_BAND eBand);

u_int8_t mlrCanUseMlrRate(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		enum ENUM_BAND eBand,
		struct MSDU_INFO *prMsduInfo);

u_int8_t mlrDecideIfUseMlrRate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct STA_RECORD *prStaRec,
		struct MSDU_INFO *prMsduInfo,
		uint16_t *pu2RateCode);

uint16_t mlrGenerateMlrIEforMTKOuiIE(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo, uint8_t *pucBuf);

void mlrEventMlrFsmUpdateHandler(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);

void mlrGetTxFragParameter(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo,
		uint16_t *prTxFragSplitSize, uint16_t *prTxFragThr);

void mlrUpdateBySetCfg(struct ADAPTER *prAdapter,
		struct net_device *prNetDev,
		struct PARAM_CUSTOM_KEY_CFG_STRUCT *prKeyCfgInfo);
#endif
#endif /* _MLR_H */
