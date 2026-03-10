/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
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

#define MLR_BIT_V1_V2_SUPPORT(u4MlrBitmap) \
	(((u4MlrBitmap & (MLR_MODE_MLR_V1 | MLR_MODE_MLR_V2)) \
	== (MLR_MODE_MLR_V1 | MLR_MODE_MLR_V1)) ? TRUE : FALSE)

#define MLR_BIT_INTERSECTION(u4MlrBitmapA, ucMlrBitmapB) \
	(u4MlrBitmapA & ucMlrBitmapB)

#define MLR_STATE_IN_START(ucMlrState) \
	(ucMlrState == MLR_STATE_START)

#define MLR_IS_SUPPORT(prAdapter) \
	(prAdapter->ucMlrIsSupport \
	&& (prAdapter->u4MlrSupportBitmap != MLR_MODE_NOT_SUPPORT)) \

#define MLR_IS_PEER_SUPPORT(prStaRec) \
	(prStaRec->fgIsMlrSupported \
	&& (prStaRec->ucMlrSupportBitmap != MLR_MODE_NOT_SUPPORT))

#define MLR_IS_BOTH_SUPPORT(prAdapter, prStaRec) \
	(prAdapter->ucMlrIsSupport \
	&& (prAdapter->u4MlrSupportBitmap != MLR_MODE_NOT_SUPPORT) \
	&& prStaRec->fgIsMlrSupported \
	&& (prStaRec->ucMlrSupportBitmap != MLR_MODE_NOT_SUPPORT))

#define MLR_CHECK_IF_BAND_IS_SUPPORT(eBand) \
	(eBand == BAND_5G)

#define MLR_CHECK_IF_RCPI_IS_LOW(prAdapter, ucRCPI) \
	(ucRCPI < prAdapter->rWifiVar.ucTxMlrRateRcpiThr)

#define MLR_CHECK_IF_MGMT_USE_MLR_RATE(u2FrameCtrl) \
	(u2FrameCtrl == MAC_FRAME_AUTH \
	|| u2FrameCtrl == MAC_FRAME_ASSOC_REQ \
	|| u2FrameCtrl == MAC_FRAME_REASSOC_REQ)

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

#define MLR_DBGLOG(prAdapter, _Mod, _Clz, _Fmt, ...) \
	do { \
		if (!MLR_CHECK_IF_ENABLE_DEBUG(prAdapter)) \
			break; \
		if ((aucDebugModule[DBG_##_Mod##_IDX] & \
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

u_int8_t mlrDecideIfUseMlrRate(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct STA_RECORD *prStaRec,
		struct MSDU_INFO *prMsduInfo,
		uint16_t *pu2RateCode);

void mlrGenerateMTKOuiIEforMlr(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo);

void mlrEventMlrFsmUpdateHandler(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);

#endif
#endif /* _MLR_H */
