// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "que_mgt.c"
 *    \brief  TX/RX queues management
 *
 *    The main tasks of queue management include TC-based HIF TX flow control,
 *    adaptive TC quota adjustment, HIF TX grant scheduling, Power-Save
 *    forwarding control, RX packet reordering, and RX BA agreement management.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "queue.h"
#include "mddp.h"
#include "nic_tx.h"

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
OS_SYSTIME g_arMissTimeout[CFG_STA_REC_NUM][CFG_RX_MAX_BA_TID_NUM];

const uint8_t aucTid2ACI[TX_DESC_TID_NUM] = {
	WMM_AC_BE_INDEX,	/* TID0 */
	WMM_AC_BK_INDEX,	/* TID1 */
	WMM_AC_BK_INDEX,	/* TID2 */
	WMM_AC_BE_INDEX,	/* TID3 */
	WMM_AC_VI_INDEX,	/* TID4 */
	WMM_AC_VI_INDEX,	/* TID5 */
	WMM_AC_VO_INDEX,	/* TID6 */
	WMM_AC_VO_INDEX		/* TID7 */
};

const uint8_t aucACI2TxQIdx[WMM_AC_INDEX_NUM] = {
	TX_QUEUE_INDEX_AC1,	/* WMM_AC_BE_INDEX */
	TX_QUEUE_INDEX_AC0,	/* WMM_AC_BK_INDEX */
	TX_QUEUE_INDEX_AC2,	/* WMM_AC_VI_INDEX */
	TX_QUEUE_INDEX_AC3	/* WMM_AC_VO_INDEX */
};

const uint8_t *apucACI2Str[WMM_AC_INDEX_NUM] = {
	"BE", "BK", "VI", "VO"
};

const uint8_t arNetwork2TcResource[MAX_BSSID_NUM + 1][NET_TC_NUM] = {
	/* HW Queue Set 1 */
	/* AC_BE, AC_BK, AC_VI, AC_VO, MGMT, BMC */
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	/* AIS */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
	/* P2P/BoW */
	{TC6_INDEX, TC5_INDEX, TC7_INDEX, TC8_INDEX, TC4_INDEX, TC6_INDEX},
	/* P2P/BoW */
	{TC10_INDEX, TC9_INDEX, TC11_INDEX, TC12_INDEX, TC4_INDEX, TC10_INDEX},
	/* P2P/BoW */
	{TC13_INDEX, TC13_INDEX, TC13_INDEX, TC13_INDEX, TC4_INDEX, TC13_INDEX},
	/* P2P_DEV */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
#else
	/* AIS */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
	/* P2P/BoW */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
	/* P2P/BoW */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
	/* P2P/BoW */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
	/* P2P_DEV */
	{TC1_INDEX, TC0_INDEX, TC2_INDEX, TC3_INDEX, TC4_INDEX, BMC_TC_INDEX},
#endif
};

const uint8_t aucWmmAC2TcResourceSet1[WMM_AC_INDEX_NUM] = {
	TC1_INDEX,
	TC0_INDEX,
	TC2_INDEX,
	TC3_INDEX
};

#if NIC_TX_ENABLE_SECOND_HW_QUEUE
const uint8_t aucWmmAC2TcResourceSet2[WMM_AC_INDEX_NUM] = {
	TC7_INDEX,
	TC6_INDEX,
	TC8_INDEX,
	TC9_INDEX
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
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
#define LINK_QUALITY_COUNT_DUP(prAdapter, prSwRfb) \
do { \
	struct BSS_INFO *prBssInfo = NULL; \
	prBssInfo = aisGetDefaultLinkBssInfo(prAdapter); \
	if (prBssInfo && prBssInfo->prStaRecOfAP) \
		if (prBssInfo->prStaRecOfAP->ucWlanIndex == \
		    prSwRfb->ucWlanIdx) \
			prAdapter->rLinkQualityInfo.u4RxDupCount++; \
} while (0)
#else
#define LINK_QUALITY_COUNT_DUP
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static void fallWithinVerboseLogging(struct ADAPTER *prAdapter,
		struct SW_RFB *prReorderedSwRfb,
		struct RX_BA_ENTRY *prReorderQueParm,
		u_int8_t fgDequeuHead,
		u_int8_t fgMissing,
		uint8_t fgIsAmsduSubframe,
		u_int8_t fgWinAdvanced);

static void resetRxRetryCount(struct ADAPTER *prAdapter,
			      struct RX_BA_ENTRY *prReorderQueParm)
{
#if (CFG_SUPPORT_CONNAC3X == 1)
	if (!prAdapter->chip_info->fgCheckRxDropThreshold)
		return;

	prReorderQueParm->u4RxRetryCount = 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Init Queue Management for TX
 *
 * \param[in] (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmInit(struct ADAPTER *prAdapter,
	u_int8_t isTxResrouceControlEn)
{
	uint32_t u4Idx;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	uint32_t u4TotalMinReservedTcResource = 0;
	uint32_t u4TotalTcResource = 0;
	uint32_t u4TotalGurantedTcResource = 0;
#endif

	struct QUE_MGT *prQM = &prAdapter->rQM;

	/* DbgPrint("QM: Enter qmInit()\n"); */

	/* 4 <2> Initialize other TX queues (queues not in STA_RECs) */
	for (u4Idx = 0; u4Idx < NUM_OF_PER_TYPE_TX_QUEUES; u4Idx++)
		QUEUE_INITIALIZE(&(prQM->arTxQueue[u4Idx]));

	/* 4 <3> Initialize the RX BA table and RX queues */
	/* Initialize the RX Reordering Parameters and Queues */
	for (u4Idx = 0; u4Idx < CFG_NUM_OF_RX_BA_AGREEMENTS; u4Idx++) {
		prQM->arRxBaTable[u4Idx].fgIsValid = FALSE;
		QUEUE_INITIALIZE(&(prQM->arRxBaTable[u4Idx].rReOrderQue));
		prQM->arRxBaTable[u4Idx].u2WinStart = 0xFFFF;
		prQM->arRxBaTable[u4Idx].u2WinEnd = 0xFFFF;
		prQM->arRxBaTable[u4Idx].u2BarSSN = 0;
		prQM->arRxBaTable[u4Idx].u2BarSSNIsValid = 0;
		prQM->arRxBaTable[u4Idx].u2LastRcvdSN = 0;

		prQM->arRxBaTable[u4Idx].fgIsWaitingForPktWithSsn = FALSE;
		prQM->arRxBaTable[u4Idx].fgHasBubble = FALSE;
#if CFG_SUPPORT_RX_AMSDU
		/* RX reorder for one MSDU in AMSDU issue */
		prQM->arRxBaTable[u4Idx].ucLastAmsduSubIdx =
			RX_PAYLOAD_FORMAT_MSDU;
		prQM->arRxBaTable[u4Idx].fgAmsduNeedLastFrame = FALSE;
		prQM->arRxBaTable[u4Idx].fgIsAmsduDuplicated = FALSE;
#endif
#if CFG_WOW_SUPPORT
		prQM->arRxBaTable[u4Idx].fgFirstSnToWinStart = FALSE;
#endif
		cnmTimerInitTimer(prAdapter,
			&(prQM->arRxBaTable[u4Idx].rReorderBubbleTimer),
			(PFN_MGMT_TIMEOUT_FUNC) qmHandleReorderBubbleTimeout,
			(uintptr_t) (&prQM->arRxBaTable[u4Idx]));

	}
	prQM->ucRxBaCount = 0;

	kalMemSet(&g_arMissTimeout, 0, sizeof(g_arMissTimeout));

	prQM->fgIsTxResrouceControlEn = isTxResrouceControlEn;

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 4 <4> Initialize TC resource control variables */
	for (u4Idx = 0; u4Idx < TC_NUM; u4Idx++)
		prQM->au4AverageQueLen[u4Idx] = 0;

	ASSERT(prQM->u4TimeToAdjustTcResource
	       && prQM->u4TimeToUpdateQueLen);

	for (u4Idx = 0; u4Idx < TC_NUM; u4Idx++) {
		prQM->au4CurrentTcResource[u4Idx] =
			prAdapter->rTxCtrl.rTc.au4MaxNumOfBuffer[u4Idx];

		if (u4Idx != TC4_INDEX) {
			u4TotalTcResource += prQM->au4CurrentTcResource[u4Idx];
			u4TotalGurantedTcResource +=
				prQM->au4GuaranteedTcResource[u4Idx];
			u4TotalMinReservedTcResource +=
				prQM->au4MinReservedTcResource[u4Idx];
		}
	}

	/* Sanity Check */
	if (u4TotalMinReservedTcResource > u4TotalTcResource)
		kalMemZero(prQM->au4MinReservedTcResource,
			   sizeof(prQM->au4MinReservedTcResource));

	if (u4TotalGurantedTcResource > u4TotalTcResource)
		kalMemZero(prQM->au4GuaranteedTcResource,
			   sizeof(prQM->au4GuaranteedTcResource));

	u4TotalGurantedTcResource = 0;

	/* Initialize Residual TC resource */
	for (u4Idx = 0; u4Idx < TC_NUM; u4Idx++) {
		if (prQM->au4GuaranteedTcResource[u4Idx] <
		    prQM->au4MinReservedTcResource[u4Idx])
			prQM->au4GuaranteedTcResource[u4Idx] =
				prQM->au4MinReservedTcResource[u4Idx];

		if (u4Idx != TC4_INDEX)
			u4TotalGurantedTcResource +=
				prQM->au4GuaranteedTcResource[u4Idx];
	}

	prQM->u4ResidualTcResource = u4TotalTcResource -
				     u4TotalGurantedTcResource;

	prQM->fgTcResourcePostAnnealing = FALSE;
	prQM->fgForceReassign = FALSE;
#if QM_FAST_TC_RESOURCE_CTRL
	prQM->fgTcResourceFastReaction = FALSE;
#endif
#endif

#if QM_TEST_MODE
	prQM->u4PktCount = 0;

#if QM_TEST_FAIR_FORWARDING

	prQM->u4CurrentStaRecIndexToEnqueue = 0;
	{
		uint8_t aucMacAddr[MAC_ADDR_LEN];
		struct STA_RECORD *prStaRec;

		/* Irrelevant in case this STA is an AIS AP
		 * (see qmDetermineStaRecIndex())
		 */
		aucMacAddr[0] = 0x11;
		aucMacAddr[1] = 0x22;
		aucMacAddr[2] = 0xAA;
		aucMacAddr[3] = 0xBB;
		aucMacAddr[4] = 0xCC;
		aucMacAddr[5] = 0xDD;

		prStaRec = &prAdapter->arStaRec[1];
		ASSERT(prStaRec);

		prStaRec->fgIsValid = TRUE;
		prStaRec->fgIsQoS = TRUE;
		qmSetStaPS(prAdapter, prStaRec, FALSE);
		prStaRec->ucNetTypeIndex = NETWORK_TYPE_AIS_INDEX;
		COPY_MAC_ADDR((prStaRec)->aucMacAddr, aucMacAddr);

	}

#endif

#endif

#if QM_FORWARDING_FAIRNESS
	for (u4Idx = 0; u4Idx < NUM_OF_PER_STA_TX_QUEUES; u4Idx++) {
		prQM->au4ResourceUsedCount[u4Idx] = 0;
		prQM->au4HeadStaRecIndex[u4Idx] = 0;
	}

	prQM->u4HeadBssInfoIndex = 0;
	prQM->u4GlobalResourceUsedCount = 0;
#endif

	prQM->u4TxAllowedStaCount = 0;

	prQM->rLastTxPktDumpTime = (OS_SYSTIME) kalGetTimeTick();

}

#if QM_TEST_MODE
void qmTestCases(struct ADAPTER *prAdapter)
{
	struct QUE_MGT *prQM = &prAdapter->rQM;

	DbgPrint("QM: ** TEST MODE **\n");

	if (QM_TEST_STA_REC_DETERMINATION) {
		if (prAdapter->arStaRec[0].fgIsValid) {
			prAdapter->arStaRec[0].fgIsValid = FALSE;
			DbgPrint("QM: (Test) Deactivate STA_REC[0]\n");
		} else {
			prAdapter->arStaRec[0].fgIsValid = TRUE;
			DbgPrint("QM: (Test) Activate STA_REC[0]\n");
		}
	}

	if (QM_TEST_STA_REC_DEACTIVATION) {
		/* Note that QM_STA_REC_HARD_CODING
		 * shall be set to 1 for this test
		 */

		if (prAdapter->arStaRec[0].fgIsValid) {

			DbgPrint("QM: (Test) Deactivate STA_REC[0]\n");
			qmDeactivateStaRec(prAdapter, &prAdapter->arStaRec[0]);
		} else {

			uint8_t aucMacAddr[MAC_ADDR_LEN];

			/* Irrelevant in case this STA is an AIS AP
			 * (see qmDetermineStaRecIndex())
			 */
			aucMacAddr[0] = 0x11;
			aucMacAddr[1] = 0x22;
			aucMacAddr[2] = 0xAA;
			aucMacAddr[3] = 0xBB;
			aucMacAddr[4] = 0xCC;
			aucMacAddr[5] = 0xDD;

			DbgPrint("QM: (Test) Activate STA_REC[0]\n");
			qmActivateStaRec(prAdapter,	/* Adapter pointer */
				0,	/* STA_REC index from FW */
				TRUE,	/* fgIsQoS */
				NETWORK_TYPE_AIS_INDEX, /* Network type */
				TRUE,	/* fgIsAp */
				aucMacAddr	/* MAC address */
			);
		}
	}

	if (QM_TEST_FAIR_FORWARDING) {
		if (prAdapter->arStaRec[1].fgIsValid) {
			prQM->u4CurrentStaRecIndexToEnqueue++;
			prQM->u4CurrentStaRecIndexToEnqueue %= 2;
			DbgPrint("QM: (Test) Switch to STA_REC[%ld]\n",
				 prQM->u4CurrentStaRecIndexToEnqueue);
		}
	}

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update a STA_REC
 *
 * \param[in] prAdapter  Pointer to the Adapter instance
 * \param[in] prStaRec The pointer of the STA_REC
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmUpdateStaRec(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo;
	u_int8_t fgIsTxAllowed = FALSE;

	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);

	/* 4 <1> Ensure STA is valid */
	if (prStaRec->fgIsValid) {
		/* 4 <2.1> STA/BSS is protected */
		if (secIsProtectedBss(prAdapter, prBssInfo)) {
			if (prStaRec->fgIsTxKeyReady ||
				secIsWepBss(prAdapter, prBssInfo))
				fgIsTxAllowed = TRUE;
			else
				fgIsTxAllowed = FALSE;
		}
		/* 4 <2.2> OPEN security */
		else
			fgIsTxAllowed = TRUE;
	}
	/* 4 <x> Update StaRec */
	qmSetStaRecTxAllowed(prAdapter, prStaRec, fgIsTxAllowed);

#if CFG_SUPPORT_BFER
#if (CFG_SUPPORT_802_11AX == 1)
	if ((IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtBfer) ||
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHtBfer) ||
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHeSuBfer)) &&
		fgIsTxAllowed && (prStaRec->ucStaState == STA_STATE_3)) {
#else
	if ((IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaVhtBfer) ||
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHtBfer)) &&
		fgIsTxAllowed && (prStaRec->ucStaState == STA_STATE_3)) {
#endif
		rlmETxBfTriggerPeriodicSounding(prAdapter);
		rlmBfStaRecPfmuUpdate(prAdapter, prStaRec);
	}
#endif

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Activate a STA_REC
 *
 * \param[in] prAdapter  Pointer to the Adapter instance
 * \param[in] prStaRec The pointer of the STA_REC
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmActivateStaRec(struct ADAPTER *prAdapter,
		      struct STA_RECORD *prStaRec)
{
	/* 4 <1> Deactivate first */
	if (!prStaRec)
		return;

	if (prStaRec->fgIsValid) {	/* The STA_REC has been activated */
		DBGLOG(QM, WARN,
			"QM: (WARNING) Activating a STA_REC which has been activated\n");
		DBGLOG(QM, WARN,
			"QM: (WARNING) Deactivating a STA_REC before re-activating\n");
		/* To flush TX/RX queues and del RX BA agreements */
		qmDeactivateStaRec(prAdapter, prStaRec);
	}
	/* 4 <2> Activate the STA_REC */
	/* Reset buffer count  */
	prStaRec->ucFreeQuota = 0;
	prStaRec->ucFreeQuotaForDelivery = 0;
	prStaRec->ucFreeQuotaForNonDelivery = 0;

	/* Init the STA_REC */
	prStaRec->fgIsValid = TRUE;
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	qmSetStaRecRxAllowed(prAdapter, prStaRec, TRUE);
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
	qmSetStaPS(prAdapter, prStaRec, FALSE);

	/* Default setting of TX/RX AMPDU */
	prStaRec->fgTxAmpduEn = IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucAmpduTx);
	prStaRec->fgRxAmpduEn = IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucAmpduRx);

	nicTxGenerateDescTemplate(prAdapter, prStaRec);

	qmUpdateStaRec(prAdapter, prStaRec);

	/* Done in qmInit() or qmDeactivateStaRec() */
#if 0
	/* At the beginning, no RX BA agreements have been established */
	for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++)
		(prStaRec->aprRxReorderParamRefTbl)[i] = NULL;
#endif

#if CFG_MTK_MDDP_SUPPORT
	if (mddpIsSupportMcifWifi())
		mddpNotifyDrvTxd(prAdapter, prStaRec, TRUE);
#endif

	LINK_INITIALIZE(&prStaRec->rMscsMonitorList);
	LINK_INITIALIZE(&prStaRec->rMscsTcpMonitorList);

	DBGLOG(QM, INFO, "QM: +STA[%d]\n", prStaRec->ucIndex);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Deactivate a STA_REC
 *
 * \param[in] prAdapter  Pointer to the Adapter instance
 * \param[in] u4StaRecIdx The index of the STA_REC
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmDeactivateStaRec(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	uint32_t i;

	if (!prStaRec)
		return;

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
	/* clear fragment cache when reconnect, reassoc, disconnect */
	nicRxClearFrag(prAdapter, prStaRec);
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

	/* 4 <1> Flush TX queues */
	if (HAL_IS_TX_DIRECT(prAdapter)) {
		nicTxDirectClearStaAcmQ(prAdapter, prStaRec->ucIndex);
		nicTxDirectClearStaPendQ(prAdapter, prStaRec->ucIndex);
		nicTxDirectClearStaPsQ(prAdapter, prStaRec->ucIndex);
	} else {
		struct MSDU_INFO *prFlushedTxPacketList = NULL;

		prFlushedTxPacketList = qmFlushStaTxQueues(prAdapter,
			prStaRec->ucIndex);

		if (prFlushedTxPacketList)
			wlanProcessQueuedMsduInfo(prAdapter,
				prFlushedTxPacketList);
	}

	/* 4 <2> Flush RX queues and delete RX BA agreements */
	for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++)
		qmDelRxBaEntry(prAdapter, prStaRec->ucIndex, i, FALSE);

	/* 4 <3> Deactivate the STA_REC */
	prStaRec->fgIsValid = FALSE;
#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	qmSetStaRecRxAllowed(prAdapter, prStaRec, FALSE);
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */
	qmSetStaPS(prAdapter, prStaRec, FALSE);
	prStaRec->fgIsTxKeyReady = FALSE;
	prStaRec->fgIsMscsSupported = FALSE;

	/* Reset buffer count  */
	prStaRec->ucFreeQuota = 0;
	prStaRec->ucFreeQuotaForDelivery = 0;
	prStaRec->ucFreeQuotaForNonDelivery = 0;

	nicTxFreeDescTemplate(prAdapter, prStaRec);

	qmUpdateStaRec(prAdapter, prStaRec);

#if CFG_MTK_MDDP_SUPPORT
	if (mddpIsSupportMcifWifi())
		mddpNotifyDrvTxd(prAdapter, prStaRec, FALSE);
#endif
#if CFG_FAST_PATH_SUPPORT
	mscsDeactivate(prAdapter, prStaRec);
#endif
	DBGLOG(QM, INFO, "QM: -STA[%u]\n", prStaRec->ucIndex);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Deactivate a STA_REC
 *
 * \param[in] prAdapter  Pointer to the Adapter instance
 * \param[in] ucBssIndex The index of the BSS
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmFreeAllByBssIdx(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{

	struct QUE_MGT *prQM = NULL;
	struct QUE *prQue = NULL;
	struct QUE rNeedToFreeQue;
	struct QUE rTempQue;
	struct QUE *prNeedToFreeQue = NULL;
	struct QUE *prTempQue = NULL;
	struct MSDU_INFO *prMsduInfo = NULL;

	prQM = &prAdapter->rQM;
	prQue = &prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST];

	QUEUE_INITIALIZE(&rNeedToFreeQue);
	QUEUE_INITIALIZE(&rTempQue);

	prNeedToFreeQue = &rNeedToFreeQue;
	prTempQue = &rTempQue;

	QUEUE_MOVE_ALL(prTempQue, prQue);

	QUEUE_REMOVE_HEAD(prTempQue, prMsduInfo,
		struct MSDU_INFO *);
	while (prMsduInfo) {

		if (prMsduInfo->ucBssIndex == ucBssIndex) {
			/* QUEUE_INSERT_TAIL */
			QUEUE_INSERT_TAIL(prNeedToFreeQue, prMsduInfo);
		} else {
			/* QUEUE_INSERT_TAIL */
			QUEUE_INSERT_TAIL(prQue, prMsduInfo);
		}

		QUEUE_REMOVE_HEAD(prTempQue, prMsduInfo, struct MSDU_INFO *);
	}
	if (QUEUE_IS_NOT_EMPTY(prNeedToFreeQue))
		wlanProcessQueuedMsduInfo(prAdapter,
				QUEUE_GET_HEAD(prNeedToFreeQue));

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Flush all TX queues
 *
 * \param[in] (none)
 *
 * \return The flushed packets (in a list of MSDU_INFOs)
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *qmFlushTxQueues(struct ADAPTER *prAdapter)
{
	uint8_t ucStaArrayIdx;
	uint8_t ucQueArrayIdx;

	struct QUE_MGT *prQM = &prAdapter->rQM;

	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;
	struct QUE *prQue;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushTxQueues()\n");

	QUEUE_INITIALIZE(prTempQue);

	/* Concatenate all MSDU_INFOs in per-STA queues */
	for (ucStaArrayIdx = 0; ucStaArrayIdx < CFG_STA_REC_NUM;
		ucStaArrayIdx++) {
		for (ucQueArrayIdx = 0;
			ucQueArrayIdx < NUM_OF_PER_STA_TX_QUEUES;
			ucQueArrayIdx++) {
			prQue = &(prAdapter->arStaRec[ucStaArrayIdx].
			arPendingTxQueue[ucQueArrayIdx]);
			QUEUE_CONCATENATE_QUEUES(prTempQue, prQue);
			prQue = &(prAdapter->arStaRec[ucStaArrayIdx].
			arTxQueue[ucQueArrayIdx]);
			QUEUE_CONCATENATE_QUEUES(prTempQue, prQue);
		}
	}

	/* Flush per-Type queues */
	for (ucQueArrayIdx = 0;
		ucQueArrayIdx < NUM_OF_PER_TYPE_TX_QUEUES;
		ucQueArrayIdx++) {
		prQue = &(prQM->arTxQueue[ucQueArrayIdx]);
		QUEUE_CONCATENATE_QUEUES(prTempQue, prQue);
	}

	return QUEUE_GET_HEAD(prTempQue);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Flush TX packets for a particular STA
 *
 * \param[in] u4StaRecIdx STA_REC index
 *
 * \return The flushed packets (in a list of MSDU_INFOs)
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *qmFlushStaTxQueues(struct ADAPTER *prAdapter,
	uint32_t u4StaRecIdx)
{
	uint8_t ucQueArrayIdx;
	struct STA_RECORD *prStaRec;
	struct QUE *prQue;
	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushStaTxQueues(%u)\n", u4StaRecIdx);

	if (u4StaRecIdx >= CFG_STA_REC_NUM)
		return NULL;

	prStaRec = &prAdapter->arStaRec[u4StaRecIdx];
	ASSERT(prStaRec);

	QUEUE_INITIALIZE(prTempQue);

	/* Concatenate all MSDU_INFOs in TX queues of this STA_REC */
	for (ucQueArrayIdx = 0;
		ucQueArrayIdx < NUM_OF_PER_STA_TX_QUEUES; ucQueArrayIdx++) {
		prQue = &(prStaRec->arPendingTxQueue[ucQueArrayIdx]);
		QUEUE_CONCATENATE_QUEUES(prTempQue, prQue);
		prQue = &(prStaRec->arTxQueue[ucQueArrayIdx]);
		QUEUE_CONCATENATE_QUEUES(prTempQue, prQue);
	}

	return QUEUE_GET_HEAD(prTempQue);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Flush RX packets
 *
 * \param[in] (none)
 *
 * \return The flushed packets (in a list of SW_RFBs)
 */
/*----------------------------------------------------------------------------*/
struct SW_RFB *qmFlushRxQueues(struct ADAPTER *prAdapter)
{
	uint32_t i;
	struct SW_RFB *prSwRfbListHead;
	struct SW_RFB *prSwRfbListTail;
	struct QUE_MGT *prQM = &prAdapter->rQM;

	prSwRfbListHead = prSwRfbListTail = NULL;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushRxQueues()\n");
	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_LOCK(prAdapter->prGlueInfo, 0);

	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		if (QUEUE_IS_NOT_EMPTY(&
			(prQM->arRxBaTable[i].rReOrderQue))) {
			if (!prSwRfbListHead) {

				/* The first MSDU_INFO is found */
				prSwRfbListHead = QUEUE_GET_HEAD(
						&(prQM->arRxBaTable[i].
						rReOrderQue));
				prSwRfbListTail = QUEUE_GET_TAIL(
						&(prQM->arRxBaTable[i].
						rReOrderQue));
			} else {
				/* Concatenate the MSDU_INFO list with
				 * the existing list
				 */
				QUEUE_ENTRY_SET_NEXT(prSwRfbListTail,
					QUEUE_GET_HEAD(&(prQM->arRxBaTable[i].
						rReOrderQue)));

				prSwRfbListTail = QUEUE_GET_TAIL(
						&(prQM->arRxBaTable[i].
						rReOrderQue));
			}

			QUEUE_INITIALIZE(&(prQM->arRxBaTable[i].rReOrderQue));
			if (QM_RX_GET_NEXT_SW_RFB(prSwRfbListTail)) {
				DBGLOG(QM, ERROR,
					"QM: non-null tail->next at arRxBaTable[%u]\n",
					i);
			}
		} else {
			continue;
		}
	}

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_UNLOCK(prAdapter->prGlueInfo, 0);

	/* Terminate the MSDU_INFO list with a NULL pointer */
	if (prSwRfbListTail)
		QUEUE_ENTRY_SET_NEXT(prSwRfbListTail, NULL);

	return prSwRfbListHead;

}

/**
 * qmFlushStaRxQueue() - Flush RX packets with given reordering BA
 *
 * @prReorderQueParm: Pointer to BA Rroeder context
 *
 * This procedure only processes packets based on prReorderQueParm,
 * STA record are isolated in the caller.
 *
 * Return: The flushed packets (in a list of SW_RFBs)
 */
static struct SW_RFB *qmFlushStaRxQueue(struct ADAPTER *prAdapter,
		struct RX_BA_ENTRY *prReorderQueParm)
{
	struct SW_RFB *prSwRfbListHead = NULL;
	struct SW_RFB *prSwRfbListTail = NULL;

	DBGLOG(QM, TRACE, "QM: Enter qmFlushStaRxQueues(%u:%u)\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

	/* Note: For each queued packet,
	 * prCurrSwRfb->eDst equals RX_PKT_DESTINATION_HOST
	 */
	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_LOCK(prAdapter->prGlueInfo, 0);

	if (QUEUE_IS_NOT_EMPTY(&prReorderQueParm->rReOrderQue)) {
		prSwRfbListHead =
			QUEUE_GET_HEAD(&prReorderQueParm->rReOrderQue);
		prSwRfbListTail =
			QUEUE_GET_TAIL(&prReorderQueParm->rReOrderQue);

		QUEUE_INITIALIZE(&prReorderQueParm->rReOrderQue);
	}

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_UNLOCK(prAdapter->prGlueInfo, 0);

	if (prSwRfbListTail) {
		if (QM_RX_GET_NEXT_SW_RFB(prSwRfbListTail)) {
			DBGLOG(QM, ERROR,
				"QM: non-empty tail->next at STA %u TID %u\n",
				prReorderQueParm->ucStaRecIdx,
				prReorderQueParm->ucTid);
		}

		/* Terminate the MSDU_INFO list with a NULL pointer */
		QUEUE_ENTRY_SET_NEXT(prSwRfbListTail, NULL);
	}
	return prSwRfbListHead;
}

struct QUE *qmDetermineStaTxQueue(struct ADAPTER *prAdapter,
				  struct MSDU_INFO *prMsduInfo,
				  uint8_t ucActiveTs, uint8_t *pucTC)
{
	struct QUE *prTxQue = NULL;
	struct STA_RECORD *prStaRec;
	enum ENUM_WMM_ACI eAci = WMM_AC_BE_INDEX;
	u_int8_t fgCheckACMAgain;
	uint8_t ucTC = TC0_INDEX;
	uint8_t ucQueIdx = TX_QUEUE_INDEX_AC0;
	struct BSS_INFO *prBssInfo;
	/* BEtoBK, na, VItoBE, VOtoVI */
	uint8_t aucNextUP[WMM_AC_INDEX_NUM] = {1, 1, 0, 4};

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsduInfo->ucBssIndex);
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter,
		prMsduInfo->ucStaRecIndex);
	if (prStaRec == NULL)
		return prTxQue;

	if (!prStaRec) {
		DBGLOG(QM, ERROR, "prStaRec is null.\n");
		return NULL;
	}
	if (prMsduInfo->ucUserPriority < 8) {
		QM_DBG_CNT_INC(&prAdapter->rQM,
			prMsduInfo->ucUserPriority + 15);
	}

	eAci = WMM_AC_BE_INDEX;
	do {
		fgCheckACMAgain = FALSE;
		if (prStaRec->fgIsQoS) {
			if (prMsduInfo->ucUserPriority < TX_DESC_TID_NUM) {
				eAci = aucTid2ACI[prMsduInfo->ucUserPriority];
				if (eAci < WMM_AC_INDEX_NUM) {
					ucQueIdx = aucACI2TxQIdx[eAci];
					ucTC = nicTxWmmTc2ResTc(prAdapter,
						prMsduInfo->ucBssIndex, eAci);
				}
			} else {
				ucQueIdx = TX_QUEUE_INDEX_AC1;
				ucTC = TC1_INDEX;
				eAci = WMM_AC_BE_INDEX;
				DBGLOG(QM, WARN,
					"Packet TID is not in [0~7]\n");
				ASSERT(0);
			}
			if (eAci < WMM_AC_INDEX_NUM &&
				(prBssInfo->arACQueParms[eAci].ucIsACMSet) &&
			    !(ucActiveTs & BIT(eAci)) &&
			    (eAci != WMM_AC_BK_INDEX)) {
				DBGLOG(WMM, TRACE,
					"ucUserPriority: %d, aucNextUP[eAci]: %d",
					prMsduInfo->ucUserPriority,
					aucNextUP[eAci]);
				prMsduInfo->ucUserPriority = aucNextUP[eAci];
				fgCheckACMAgain = TRUE;
			}
		} else {
#if CFG_NON_QOS_ARP_USE_QOS_TXQ_MAPPING
			if (GLUE_TEST_PKT_FLAG(prMsduInfo->prPacket,
				ENUM_PKT_ARP) && (prMsduInfo->ucUserPriority <
				TX_DESC_TID_NUM)) {
				eAci = aucTid2ACI[prMsduInfo->ucUserPriority];
				if (eAci < WMM_AC_INDEX_NUM) {
					ucQueIdx = aucACI2TxQIdx[eAci];
					ucTC = nicTxWmmTc2ResTc(prAdapter,
						prMsduInfo->ucBssIndex, eAci);
				}
			} else {
#endif
				ucQueIdx = TX_QUEUE_INDEX_NON_QOS;
				ucTC = nicTxWmmTc2ResTc(prAdapter,
					prMsduInfo->ucBssIndex,
					NET_TC_WMM_AC_BE_INDEX);
#if CFG_NON_QOS_ARP_USE_QOS_TXQ_MAPPING
			}
#endif
		}

		if (prAdapter->rWifiVar.ucTcRestrict < TC_NUM) {
			ucTC = prAdapter->rWifiVar.ucTcRestrict;
			ucQueIdx = ucTC;
		}

	} while (fgCheckACMAgain);

	if (ucQueIdx >= NUM_OF_PER_STA_TX_QUEUES) {
		DBGLOG(QM, ERROR,
			"ucQueIdx = %u, needs 0~3 to avoid out-of-bounds.\n",
			ucQueIdx);
		return NULL;
	}
	if (prStaRec->fgIsTxAllowed) {
		/* non protected BSS or protected BSS with key set */
		prTxQue = prStaRec->aprTargetQueue[ucQueIdx];
	} else if (secIsProtectedBss(prAdapter, prBssInfo) &&
		prMsduInfo->fgIs802_1x &&
		prMsduInfo->fgIs802_1x_NonProtected &&
		!prAdapter->fgIsPostponeTxEAPOLM3) {
		/* protected BSS without key set */
		/* Tx pairwise EAPOL 1x packet (non-protected frame) */
		prTxQue = &prStaRec->arTxQueue[ucQueIdx];
	} else {
		/* protected BSS without key set */
		/* Enqueue protected frame into pending queue */
		prTxQue = prStaRec->aprTargetQueue[ucQueIdx];
	}

	*pucTC = ucTC;
#if CFG_ENABLE_PKT_LIFETIME_PROFILE && CFG_ENABLE_PER_STA_STATISTICS
	/*
	 * Record how many packages enqueue this STA
	 * to TX during statistic intervals
	 */
	prStaRec->u4EnqueueCounter++;
#endif

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	DBGLOG(HIF_WMM_ENHANCE, TRACE,
		   "Tc%d, StaRec[%d], Sta_QIdx:%u BssIdx:%d\n",
		   ucTC, prMsduInfo->ucStaRecIndex,
		   ucQueIdx, prMsduInfo->ucBssIndex);
#endif

	return prTxQue;
}

void qmSetTxPacketDescTemplate(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(
		prAdapter, prMsduInfo->ucStaRecIndex);

	/* Check the Tx descriptor template is valid */
	if (prStaRec &&
		prStaRec->aprTxDescTemplate[prMsduInfo->ucUserPriority]) {
		prMsduInfo->fgIsTXDTemplateValid = TRUE;
	} else {
		if (prStaRec) {
			DBGLOG(QM, TRACE,
				"Cannot get TXD template for STA[%u] QoS[%u] MSDU UP[%u]\n",
				prStaRec->ucIndex, prStaRec->fgIsQoS,
				prMsduInfo->ucUserPriority);
		}
		prMsduInfo->fgIsTXDTemplateValid = FALSE;
	}
}

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
void qmSetStaRecRxAllowed(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, u_int8_t fgIsRxAllowed)
{
	prStaRec->fgIsRxAllowed = fgIsRxAllowed;

	/*
	 * If AP does not reply assoc resp,
	 * AIS will disconnect and indirectly call this function,
	 * so we also need dequeue when fgIsRxAllowed is FALSE to finish
	 * the SwRfb cleanup.
	 */
	if (kalScheduleNapiTask(prAdapter) == WLAN_STATUS_NOT_ACCEPTED) {
		/* Handle Non Rx-direct call path */
		nicRxDequeuePendingQueue(prAdapter);
	}
}
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

/*----------------------------------------------------------------------------*/
/*!
 * \brief : To StaRec, function to stop TX
 *
 * \param[in] :
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void qmSetStaRecTxAllowed(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, u_int8_t fgIsTxAllowed)
{
	uint8_t ucIdx;
	struct QUE *prSrcQ, *prDstQ;

	DBGLOG(QM, INFO, "Set Sta[%u] TxAllowed from [%u] to [%u] %s TxQ\n",
		prStaRec->ucIndex,
		prStaRec->fgIsTxAllowed,
		fgIsTxAllowed,
		fgIsTxAllowed ? "normal" : "pending");

	/* Update Tx queue when allowed state change*/
	if (prStaRec->fgIsTxAllowed != fgIsTxAllowed) {
		for (ucIdx = 0; ucIdx < NUM_OF_PER_STA_TX_QUEUES; ucIdx++) {
			if (fgIsTxAllowed) {
				prSrcQ = &prStaRec->arPendingTxQueue[ucIdx];
				prDstQ = &prStaRec->arTxQueue[ucIdx];
				QUEUE_CONCATENATE_QUEUES(prDstQ, prSrcQ);
			} else {
				prSrcQ = &prStaRec->arTxQueue[ucIdx];
				prDstQ = &prStaRec->arPendingTxQueue[ucIdx];
				QUEUE_CONCATENATE_QUEUES_HEAD(prDstQ, prSrcQ);
			}
			prStaRec->aprTargetQueue[ucIdx] = prDstQ;
		}

		if (fgIsTxAllowed)
			prAdapter->rQM.u4TxAllowedStaCount++;
		else
			prAdapter->rQM.u4TxAllowedStaCount--;
	}
	prStaRec->fgIsTxAllowed = fgIsTxAllowed;

	/* Start tx the pending frame for TX Direct path */
	if (prStaRec->fgIsTxAllowed &&
	    HAL_IS_TX_DIRECT(prGlueInfo->prAdapter)) {
		nicTxDirectStartCheckQTimer(prAdapter);
	}
}
void qmDetermineTxPacketRate(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo)
{
	/* Set Tx rate */
	switch (prAdapter->rWifiVar.ucDataTxRateMode) {
	case DATA_RATE_MODE_BSS_LOWEST:
		nicTxSetPktLowestFixedRate(prAdapter, prMsduInfo);
		break;

	case DATA_RATE_MODE_MANUAL:
		prMsduInfo->u4FixedRateOption =
			prAdapter->rWifiVar.u4DataTxRateCode;

		prMsduInfo->ucRateMode = MSDU_RATE_MODE_MANUAL_DESC;
		break;

	case DATA_RATE_MODE_AUTO:
	default:
		if (prMsduInfo->ucRateMode == MSDU_RATE_MODE_LOWEST_RATE)
			nicTxSetPktLowestFixedRate(prAdapter, prMsduInfo);
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Enqueue TX packets
 *
 * \param[in] prMsduInfoListHead Pointer to the list of TX packets
 *
 * \return The freed packets, which are not enqueued
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *qmEnqueueTxPackets(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfoListHead)
{
	struct MSDU_INFO *prMsduInfoReleaseList;
	struct MSDU_INFO *prCurrentMsduInfo;
	struct MSDU_INFO *prNextMsduInfo;

	struct QUE *prTxQue;
	struct QUE rNotEnqueuedQue;
	uint8_t ucTC;
	struct TX_CTRL *prTxCtrl = &prAdapter->rTxCtrl;
	struct QUE_MGT *prQM = &prAdapter->rQM;
	struct BSS_INFO *prBssInfo;
	u_int8_t fgDropPacket;
	uint8_t ucActivedTspec = 0;

	DBGLOG(QM, LOUD, "Enter qmEnqueueTxPackets\n");

	ASSERT(prMsduInfoListHead);

	prMsduInfoReleaseList = NULL;
	prCurrentMsduInfo = NULL;
	QUEUE_INITIALIZE(&rNotEnqueuedQue);
	prNextMsduInfo = prMsduInfoListHead;

	do {
		prCurrentMsduInfo = prNextMsduInfo;
		prNextMsduInfo = QM_TX_GET_NEXT_MSDU_INFO(
			prCurrentMsduInfo);
		ucTC = TC1_INDEX;

		/* 4 <0> Sanity check of BSS_INFO */
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prCurrentMsduInfo->ucBssIndex);

		if (!prBssInfo) {
			/* No BSS_INFO */
			fgDropPacket = TRUE;
		} else if (IS_BSS_ACTIVE(prBssInfo)) {
			/* BSS active */
			fgDropPacket = FALSE;
		} else {
			/* BSS inactive */
			fgDropPacket = TRUE;
		}

		if (!fgDropPacket) {
			/* 4 <1> Lookup the STA_REC index */
			/* The ucStaRecIndex will be set in this function */
			qmDetermineStaRecIndex(prAdapter, prCurrentMsduInfo);

			/*get per-AC Tx packets */
			wlanUpdateTxStatistics(prAdapter, prCurrentMsduInfo,
				FALSE);

			DBGLOG(QM, LOUD, "Enqueue MSDU by StaRec[%u]!\n",
			       prCurrentMsduInfo->ucStaRecIndex);

			switch (prCurrentMsduInfo->ucStaRecIndex) {
			case STA_REC_INDEX_BMCAST:
				prTxQue =
					&prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST];
				ucTC = nicTxWmmTc2ResTc(prAdapter,
					prCurrentMsduInfo->ucBssIndex,
					NET_TC_BMC_INDEX);

				/* Always set BMC packet retry limit
				 * to unlimited
				 */
				if (!(prCurrentMsduInfo->u4Option &
				      MSDU_OPT_MANUAL_RETRY_LIMIT))
					nicTxSetPktRetryLimit(prCurrentMsduInfo,
						TX_DESC_TX_COUNT_NO_LIMIT);

				QM_DBG_CNT_INC(prQM, QM_DBG_CNT_23);
				break;

			case STA_REC_INDEX_NOT_FOUND:
				/* Drop packet if no STA_REC is found */
				DBGLOG(QM, ERROR,
					"Drop the Packet for no STA_REC\n");

				prTxQue = &rNotEnqueuedQue;

				TX_INC_CNT(&prAdapter->rTxCtrl,
					TX_INACTIVE_STA_DROP);
				QM_DBG_CNT_INC(prQM, QM_DBG_CNT_24);
				break;

			default:
				ucActivedTspec = nicGetActiveTspec(prAdapter,
					prCurrentMsduInfo->ucBssIndex);

				prTxQue = qmDetermineStaTxQueue(
					prAdapter, prCurrentMsduInfo,
					ucActivedTspec, &ucTC);
				if (!prTxQue) {
					DBGLOG(QM, INFO,
						"Drop the Packet for TxQue is NULL\n");
					prTxQue = &rNotEnqueuedQue;
					TX_INC_CNT(&prAdapter->rTxCtrl,
						TX_INACTIVE_STA_DROP);
					QM_DBG_CNT_INC(prQM, QM_DBG_CNT_24);
				}
#if ARP_MONITER_ENABLE
				arpMonProcessTxPacket(prAdapter,
							prCurrentMsduInfo);
#endif /* ARP_MONITER_ENABLE */
				break;	/*default */
			}	/* switch (prCurrentMsduInfo->ucStaRecIndex) */

			if (prCurrentMsduInfo->eSrc == TX_PACKET_FORWARDING) {
				DBGLOG(QM, TRACE,
					"Forward Pkt to STA[%u] BSS[%u]\n",
					prCurrentMsduInfo->ucStaRecIndex,
					prCurrentMsduInfo->ucBssIndex);

				if (prTxQue->u4NumElem >=
					prQM->u4MaxForwardBufferCount) {
					DBGLOG(QM, INFO,
					       "Drop the Packet for full Tx queue (forwarding) Bss %u\n",
					       prCurrentMsduInfo->ucBssIndex);
					prTxQue = &rNotEnqueuedQue;
					TX_INC_CNT(&prAdapter->rTxCtrl,
						TX_FORWARD_OVERFLOW_DROP);
				}
			}

		} else {
			DBGLOG(QM, INFO,
				"Drop the Packet for inactive Bss %u\n",
				prCurrentMsduInfo->ucBssIndex);
			QM_DBG_CNT_INC(prQM, QM_DBG_CNT_31);
			prTxQue = &rNotEnqueuedQue;
			TX_INC_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_BSS_DROP);
		}

		/* 4 <3> Fill the MSDU_INFO for constructing HIF TX header */
		/* Note that the BSS Index and STA_REC index are determined in
		 *  qmDetermineStaRecIndex(prCurrentMsduInfo).
		 */
		prCurrentMsduInfo->ucTC = ucTC;

		/* Check the Tx descriptor template is valid */
		qmSetTxPacketDescTemplate(prAdapter, prCurrentMsduInfo);

		/* BMC pkt need limited rate according to coex report*/
		if (prCurrentMsduInfo->ucStaRecIndex == STA_REC_INDEX_BMCAST)
			nicTxSetPktLowestFixedRate(prAdapter,
							prCurrentMsduInfo);

		/* 4 <4> Enqueue the packet */
		QUEUE_INSERT_TAIL(prTxQue, prCurrentMsduInfo);
		wlanFillTimestamp(prAdapter, prCurrentMsduInfo->prPacket,
				  PHASE_ENQ_QM);
		/*
		 * Record how many packages enqueue
		 * to TX during statistic intervals
		 */
		if (prTxQue != &rNotEnqueuedQue) {
			prQM->u4EnqueueCounter++;
			/* how many page count this frame wanted */
			prQM->au4QmTcWantedPageCounter[ucTC] +=
				prCurrentMsduInfo->u4PageCount;
		}
#if QM_TC_RESOURCE_EMPTY_COUNTER
		if (prCurrentMsduInfo->u4PageCount >
			prTxCtrl->rTc.au4FreePageCount[ucTC]) {
			if (prCurrentMsduInfo->ucBssIndex < MAX_BSSID_NUM
				&& ucTC < TC_NUM)
				prQM->au4QmTcResourceEmptyCounter[
				prCurrentMsduInfo->ucBssIndex][ucTC]++;
		}
#endif

#if QM_FAST_TC_RESOURCE_CTRL && QM_ADAPTIVE_TC_RESOURCE_CTRL
		if (prTxQue != &rNotEnqueuedQue) {
			/* Check and trigger fast TC resource
			 * adjustment for queued packets
			 */
			qmCheckForFastTcResourceCtrl(prAdapter, ucTC);
		}
#endif

#if QM_TEST_MODE
		if (++prQM->u4PktCount == QM_TEST_TRIGGER_TX_COUNT) {
			prQM->u4PktCount = 0;
			qmTestCases(prAdapter);
		}
#endif
	} while (prNextMsduInfo);

	if (QUEUE_IS_NOT_EMPTY(&rNotEnqueuedQue)) {
		QUEUE_ENTRY_SET_NEXT(QUEUE_GET_TAIL(&rNotEnqueuedQue), NULL);
		prMsduInfoReleaseList = QUEUE_GET_HEAD(&rNotEnqueuedQue);
	}
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 4 <x> Update TC resource control related variables */
	/* Keep track of the queue length */
	qmDoAdaptiveTcResourceCtrl(prAdapter);
#endif

	return prMsduInfoReleaseList;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Determine the STA_REC index for a packet
 *
 * \param[in] prMsduInfo Pointer to the packet
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmDetermineStaRecIndex(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prTempStaRec;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss;
	struct MLD_STA_RECORD *prMldSta;
#endif

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	prTempStaRec = NULL;

	ASSERT(prMsduInfo);

	if (prBssInfo == NULL) {
		DBGLOG(QM, INFO, "TX with prBssInfo = NULL\n");
		return;
	}

	DBGLOG(QM, LOUD,
		"Msdu BSS Idx[%u] OpMode[%u] StaRecOfApExist[%u]\n",
		prMsduInfo->ucBssIndex, prBssInfo->eCurrentOPMode,
		prBssInfo->prStaRecOfAP ? TRUE : FALSE);

	switch (prBssInfo->eCurrentOPMode) {
	case OP_MODE_IBSS:
	case OP_MODE_ACCESS_POINT:
	case OP_MODE_NAN:
		/* 4 <1> DA = BMCAST */
		if (IS_BMCAST_MAC_ADDR(prMsduInfo->aucEthDestAddr)) {
			prMsduInfo->ucStaRecIndex = STA_REC_INDEX_BMCAST;
			DBGLOG(QM, LOUD, "TX with DA = BMCAST\n");
			return;
		}
		break;

	/* Infra Client/GC */
	case OP_MODE_INFRASTRUCTURE:
	case OP_MODE_BOW:
		if (prBssInfo->prStaRecOfAP) {
#if CFG_SUPPORT_TDLS
			if (prAdapter->u4TdlsLinkCount > 0) {
				prTempStaRec =
					cnmGetTdlsPeerByAddress(prAdapter,
						prBssInfo->ucBssIndex,
						prMsduInfo->aucEthDestAddr);
				if (IS_DLS_STA(prTempStaRec) &&
				    prTempStaRec->ucStaState == STA_STATE_3) {
					if (g_arTdlsLink[
						prTempStaRec->ucTdlsIndex]) {
						prMsduInfo->ucStaRecIndex =
							prTempStaRec->ucIndex;
						return;
					}
				}
			}
#endif
			/* 4 <2> Check if an AP STA is present */
			prTempStaRec = prBssInfo->prStaRecOfAP;

			if (prTempStaRec) {
				DBGLOG(QM, LOUD,
	"StaOfAp Idx[%u] WIDX[%u] Valid[%u] TxAllowed[%u] InUse[%u] Type[%u]\n",
				       prTempStaRec->ucIndex,
				       prTempStaRec->ucWlanIndex,
				       prTempStaRec->fgIsValid,
				       prTempStaRec->fgIsTxAllowed,
				       prTempStaRec->fgIsInUse,
				       prTempStaRec->eStaType);

				if (prTempStaRec->fgIsInUse) {
					prMsduInfo->ucStaRecIndex =
						prTempStaRec->ucIndex;
					DBGLOG(QM, LOUD, "TX with AP_STA[%u]\n",
					       prTempStaRec->ucIndex);
					return;
				}
			}
		}
		break;

	case OP_MODE_P2P_DEVICE:
		break;

	default:
		break;
	}

	/* 4 <3> Not BMCAST, No AP --> Compare DA
	 * (i.e., to see whether this is a unicast frame to a client)
	 */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prAdapter, prBssInfo);
	prMldSta = mldStarecGetByMldAddr(prAdapter, prMldBss,
		prMsduInfo->aucEthDestAddr);
	if (prMldSta) {
		prTempStaRec = cnmGetStaRecByIndex(prAdapter,
			secGetStaIdxByWlanIdx(prAdapter,
				prMldSta->u2PrimaryMldId));
	} else
#endif
	{
		prTempStaRec = cnmGetStaRecByAddress(prAdapter,
			prMsduInfo->ucBssIndex,
			prMsduInfo->aucEthDestAddr);
	}

	if (prTempStaRec) {
		prMsduInfo->ucStaRecIndex = prTempStaRec->ucIndex;
		DBGLOG(QM, LOUD, "TX with STA[%u]\n",
			prTempStaRec->ucIndex);
		return;
	}

	/* 4 <4> No STA found, Not BMCAST --> Indicate NOT_FOUND to FW */
	prMsduInfo->ucStaRecIndex = STA_REC_INDEX_NOT_FOUND;
	DBGLOG(QM, LOUD, "QM: TX with STA_REC_INDEX_NOT_FOUND\n");

#if (QM_TEST_MODE && QM_TEST_FAIR_FORWARDING)
	prMsduInfo->ucStaRecIndex =
		(uint8_t)	prQM->u4CurrentStaRecIndexToEnqueue;
#endif
}

struct STA_RECORD *qmDetermineStaToBeDequeued(
	struct ADAPTER *prAdapter,
	uint32_t u4StartStaRecIndex)
{

	return NULL;
}

struct QUE *qmDequeueStaTxPackets(struct ADAPTER *prAdapter)
{

	return NULL;
}
#if CFG_SUPPORT_NAN
void
qmUpdateFreeNANQouta(struct ADAPTER *prAdapter,
		     struct EVENT_UPDATE_NAN_TX_STATUS *prTxStatus) {
	struct EVENT_UPDATE_NAN_TX_STATUS *prUpdateTxStatus;
	uint8_t ucStaIndex = 0;
	struct STA_RECORD *prStaRec; /* The current focused STA */

	prUpdateTxStatus = prTxStatus;

	prStaRec = &prAdapter->arStaRec[ucStaIndex];

	kalSetEvent(prAdapter->prGlueInfo);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dequeue TX packets from a STA_REC for a particular TC
 *
 * \param[out] prQue The queue to put the dequeued packets
 * \param[in] ucTC The TC index (TC0_INDEX to TC5_INDEX)
 * \param[in] ucMaxNum The maximum amount of dequeued packets
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t
qmDequeueTxPacketsFromPerStaQueues(struct ADAPTER *prAdapter,
	struct QUE *prQue, uint8_t ucTC,
	uint32_t u4CurrentQuota,
	uint32_t *prPleCurrentQuota,
	uint32_t u4TotalQuota)
{
	uint32_t ucLoop;		/* Loop for */

	uint32_t u4CurStaIndex = 0;
	uint32_t u4CurStaUsedResource = 0;

	/* The current focused STA */
	struct STA_RECORD *prStaRec;
	/* The Bss for current focused STA */
	struct BSS_INFO	*prBssInfo;
	/* The current TX queue to dequeue */
	struct QUE *prCurrQueue;
	/* The dequeued packet */
	struct MSDU_INFO *prDequeuedPkt;

	/* To remember the total forwarded packets for a STA */
	uint32_t u4CurStaForwardFrameCount;
	/* The maximum number of packets a STA can forward */
	uint32_t u4MaxForwardFrameCountLimit;
	uint32_t u4AvaliableResource;	/* The TX resource amount */
	uint32_t u4MaxResourceLimit;

	u_int8_t fgEndThisRound;
	struct QUE_MGT *prQM = &prAdapter->rQM;

	uint8_t *pucPsStaFreeQuota;
#if CFG_SUPPORT_NAN
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
	unsigned char fgIsNanStaRec;
#endif
#endif
#if CFG_SUPPORT_SOFT_ACM
	uint8_t ucAc = ACI_BE;
	u_int8_t fgAcmFlowCtrl = FALSE;
	static const uint8_t aucTc2Ac[] = {ACI_BK, ACI_BE, ACI_VI, ACI_VO};
#endif
	uint8_t ucAcIdx = ucTC;

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	const uint8_t TC_LIMIT = TC_NUM;
#else
	const uint8_t TC_LIMIT = NUM_OF_PER_STA_TX_QUEUES;
#endif

	/* Sanity Check */
	if (!u4CurrentQuota) {
		DBGLOG(TX, LOUD,
			"(Fairness) Skip TC = %u u4CurrentQuota = %u\n", ucTC,
			u4CurrentQuota);
		prQM->au4DequeueNoTcResourceCounter[ucTC]++;
		return u4CurrentQuota;
	}
	/* Check PLE resource */
	if (!(*prPleCurrentQuota))
		return u4CurrentQuota;
	/* 4 <1> Assign init value */
	u4AvaliableResource = u4CurrentQuota;
	u4MaxResourceLimit = u4TotalQuota;

#if QM_FORWARDING_FAIRNESS
	u4CurStaIndex = prQM->au4HeadStaRecIndex[ucTC];
	u4CurStaUsedResource = prQM->au4ResourceUsedCount[ucTC];
#endif

	fgEndThisRound = FALSE;
	ucLoop = 0;
	u4CurStaForwardFrameCount = 0;

	DBGLOG(QM, TEMP,
		"(Fairness) TC[%u] Init Head STA[%u] Resource[%u]\n",
		ucTC, u4CurStaIndex, u4AvaliableResource);

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	if (u4CurStaIndex >= CFG_STA_REC_NUM) {
		DBGLOG(QM, ERROR, "Invalid StaRecIdx.\n");
		return u4CurrentQuota;
	}

	/* TODO: special handler for WMM3 case? */
	/* TODO: check WmmIdx here? */
	ucAcIdx = nicTxGetAcIdxByTc(ucTC);

	DBGLOG(HIF_WMM_ENHANCE, TRACE,
		"TC%d Ac[%d], PSE:%d, PLE:%d\n",
		ucTC, ucAcIdx, u4CurrentQuota,
		(*prPleCurrentQuota));


	if (ucAcIdx >= NUM_OF_PER_STA_TX_QUEUES) {
		DBGLOG(HIF_WMM_ENHANCE, ERROR,
			"Invalid STA AC index. (%d)\n",
			ucTC);

		return u4CurrentQuota;
	}
#endif

	/* 4 <2> Traverse STA array from Head STA */
	/* From STA[x] to STA[x+1] to STA[x+2] to ... to STA[x] */
	while (ucLoop < CFG_STA_REC_NUM) {
		prStaRec = &prAdapter->arStaRec[u4CurStaIndex];
		prCurrQueue = &prStaRec->arTxQueue[ucAcIdx];

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		if (prStaRec->fgIsInUse == FALSE)
			goto NEXT;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				prStaRec->ucBssIndex);

		if (!prBssInfo)
			goto NEXT;

		if (!QUEUE_IS_EMPTY(prCurrQueue)) {
			prDequeuedPkt = QUEUE_GET_HEAD(prCurrQueue);

			/* Ignore pkts without my TC idx */
			if (prDequeuedPkt->ucTC != ucTC) {
				DBGLOG(QM, TRACE,
					"TC mismatch [%d] [%d]\n",
					prDequeuedPkt->ucTC, ucTC);
				goto NEXT;
			}
		}
#endif

		/* 4 <2.1> Find a Tx allowed STA */
		/* Only Data frame will be queued in */
		/* if (prStaRec->fgIsTxAllowed) */
		if (QUEUE_IS_NOT_EMPTY(prCurrQueue)) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				prStaRec->ucBssIndex);

			if (!prBssInfo)
				goto NEXT;

			/* prCurrQueue = &prStaRec->aprTxQueue[ucTC]; */
			prDequeuedPkt = NULL;
			pucPsStaFreeQuota = NULL;
			/* Set default forward count limit to unlimited */
			u4MaxForwardFrameCountLimit =
				QM_STA_FORWARD_COUNT_UNLIMITED;

			/* 4 <2.2> Update forward frame/page count
			 * limit for this STA
			 */
			/* AP mode: STA in PS buffer handling */
			if (qmIsStaInPS(prAdapter, prStaRec)) {
				if (prStaRec->fgIsQoS &&
					prStaRec->fgIsUapsdSupported &&
					(prStaRec->ucBmpTriggerAC &
						BIT(ucAcIdx))) {
					u4MaxForwardFrameCountLimit =
						prStaRec->
						ucFreeQuotaForDelivery;
					pucPsStaFreeQuota =
						&prStaRec->
						ucFreeQuotaForDelivery;
				} else {
					u4MaxForwardFrameCountLimit =
						prStaRec->
						ucFreeQuotaForNonDelivery;
					pucPsStaFreeQuota =
						&prStaRec->
						ucFreeQuotaForNonDelivery;
				}
			}

#if CFG_SUPPORT_NAN
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
			fgIsNanStaRec = FALSE;
			if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN) {
				fgIsNanStaRec = TRUE;
				DBGLOG(NAN, TEMP, "NAN STA:%d, TC:%d\n",
				       prStaRec->ucIndex, ucTC);
			}
#endif
#endif

			/* fgIsInPS */
			/* Absent BSS handling */
			if (isNetAbsent(prAdapter, prBssInfo)) {
				if (u4MaxForwardFrameCountLimit >
					prBssInfo->ucBssFreeQuota)
					u4MaxForwardFrameCountLimit =
						prBssInfo->ucBssFreeQuota;
			}
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
			/*remove DBDC quota hard rule restriction*/
#elif CFG_SUPPORT_DBDC
			if (prAdapter->rWifiVar.fgDbDcModeEn)
				u4MaxResourceLimit =
					gmGetDequeueQuota(prAdapter,
						prStaRec, prBssInfo,
						u4TotalQuota);
#endif
#if CFG_SUPPORT_SOFT_ACM
			if (ucTC <= TC3_INDEX &&
			    prStaRec->afgAcmRequired[aucTc2Ac[ucTC]]) {
				ucAc = aucTc2Ac[ucTC];
				DBGLOG(QM, TRACE, "AC %d Pending Pkts %u\n",
				       ucAc, prCurrQueue->u4NumElem);
				/* Quick check remain medium time and pending
				** packets
				*/
				if (QUEUE_IS_EMPTY(prCurrQueue) ||
				    !wmmAcmCanDequeue(prAdapter, ucAc,
				    0, prBssInfo->ucBssIndex))
					goto skip_dequeue;
				fgAcmFlowCtrl = TRUE;
			} else
				fgAcmFlowCtrl = FALSE;
#endif
			/* 4 <2.3> Dequeue packet */
			/* Three cases to break: (1) No resource
			 * (2) No packets (3) Fairness
			 */
			while (!QUEUE_IS_EMPTY(prCurrQueue)) {
				prDequeuedPkt = QUEUE_GET_HEAD(prCurrQueue);

				if ((u4CurStaForwardFrameCount >=
				     u4MaxForwardFrameCountLimit) ||
				    (u4CurStaUsedResource >=
				    u4MaxResourceLimit)) {
					/* Exceeds Limit */
					prQM->
					au4DequeueNoTcResourceCounter[ucTC]++;
					break;
				} else if (prDequeuedPkt->u4PageCount >
					   u4AvaliableResource) {
					/* Available Resource is not enough */
					prQM->
					au4DequeueNoTcResourceCounter[ucTC]++;
					if (!(prAdapter->rWifiVar.
						ucAlwaysResetUsedRes & BIT(0)))
						fgEndThisRound = TRUE;
					break;
				} else if ((*prPleCurrentQuota) <
					   NIX_TX_PLE_PAGE_CNT_PER_FRAME) {
					if (!(prAdapter->rWifiVar.
						ucAlwaysResetUsedRes & BIT(0)))
						fgEndThisRound = TRUE;
					break;
				} else if (!prStaRec->fgIsValid) {
					/* In roaming, if the sta_rec doesn't
					 * active by event 0x0C, it can't
					 * dequeue data.
					 */
					DBGLOG_LIMITED(QM, WARN,
						"sta_rec is not valid\n");
					break;
				}

#if CFG_SUPPORT_NAN
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
				if (fgIsNanStaRec == TRUE) {
					OS_SYSTIME rCurrentTime;
					unsigned char fgExpired;

					rCurrentTime = kalGetTimeTick();
					fgExpired = CHECK_FOR_EXPIRATION(
						rCurrentTime,
						prStaRec->rNanExpiredSendTime);

					/* avoid to flood the kernel log,
					 * only the 1st expiry event logged
					 */
					if (fgExpired &&
					    !prStaRec->fgNanSendTimeExpired)
						DBGLOG(NAN, TEMP,
						       "[NAN Pkt Tx Expired] Sta:%u, Exp:%u, Now:%u\n",
						       prStaRec->ucIndex,
						       prStaRec->
							rNanExpiredSendTime,
						       rCurrentTime);

					if (fgExpired) {
						prStaRec->fgNanSendTimeExpired =
							TRUE;
						break;
					}

					prStaRec->fgNanSendTimeExpired = FALSE;
				}
#endif
#endif

#if CFG_SUPPORT_SOFT_ACM
				if (fgAcmFlowCtrl) {
					uint32_t u4PktTxTime = 0;

					u4PktTxTime = wmmCalculatePktUsedTime(
						prBssInfo, prStaRec,
						prDequeuedPkt->u2FrameLength -
							ETH_HLEN);
					if (!wmmAcmCanDequeue(prAdapter, ucAc,
						u4PktTxTime,
						prBssInfo->ucBssIndex))
						break;
				}
#endif
				/* Available to be Tx */

				QUEUE_REMOVE_HEAD(prCurrQueue, prDequeuedPkt,
						  struct MSDU_INFO *);

				if (!QUEUE_IS_EMPTY(prCurrQueue)) {
					/* XXX: check all queues for STA */
					prDequeuedPkt->ucPsForwardingType =
						PS_FORWARDING_MORE_DATA_ENABLED;
				}

				if (unlikely(prStaRec->ucBssIndex !=
					prDequeuedPkt->ucBssIndex)) {
					DBGLOG(QM, INFO,
						"BssIdx mismatch [%d,%d]",
						prStaRec->ucBssIndex,
						prDequeuedPkt->ucBssIndex);
				}

				/* to record WMM Set */
				prDequeuedPkt->ucWmmQueSet =
					prBssInfo->ucWmmQueSet;
				QUEUE_INSERT_TAIL(prQue, prDequeuedPkt);
#if CFG_ENABLE_PER_STA_STATISTICS && CFG_ENABLE_PKT_LIFETIME_PROFILE
				prStaRec->u4DeqeueuCounter++;
#endif
				prQM->u4DequeueCounter++;

				u4AvaliableResource -=
					prDequeuedPkt->u4PageCount;
				u4CurStaUsedResource +=
					prDequeuedPkt->u4PageCount;
				u4CurStaForwardFrameCount++;
				(*prPleCurrentQuota) -=
					NIX_TX_PLE_PAGE_CNT_PER_FRAME;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
				DBGLOG(HIF_WMM_ENHANCE, TRACE,
					"Real DeQ: Available:%d TC%d AC%d\n",
					u4AvaliableResource,
					ucTC, ucAcIdx);
#endif
			}
#if CFG_SUPPORT_SOFT_ACM
skip_dequeue:
#endif
			/* AP mode: Update STA in PS Free quota */
			if (qmIsStaInPS(prAdapter, prStaRec)
					&& pucPsStaFreeQuota) {
				if ((*pucPsStaFreeQuota) >=
					u4CurStaForwardFrameCount)
					(*pucPsStaFreeQuota) -=
						u4CurStaForwardFrameCount;
				else
					(*pucPsStaFreeQuota) = 0;
			}

			if (isNetAbsent(prAdapter, prBssInfo)) {
				if (prBssInfo->ucBssFreeQuota >=
					u4CurStaForwardFrameCount)
					prBssInfo->ucBssFreeQuota -=
						u4CurStaForwardFrameCount;
				else
					prBssInfo->ucBssFreeQuota = 0;
			}
		}

		if (fgEndThisRound) {
			/* End this round */
			break;
		}

NEXT:
		/* Prepare for next STA */
		ucLoop++;
		u4CurStaIndex++;
		u4CurStaIndex %= CFG_STA_REC_NUM;
		u4CurStaUsedResource = 0;
		u4CurStaForwardFrameCount = 0;
	}

	/* 4 <3> Store Head Sta information to QM */
	/* No need to count used resource if thers is only one STA */
	if ((prQM->u4TxAllowedStaCount == 1) ||
		(prAdapter->rWifiVar.ucAlwaysResetUsedRes & BIT(1)))
		u4CurStaUsedResource = 0;

#if QM_FORWARDING_FAIRNESS
	if (ucTC < TC_LIMIT) {
		prQM->au4HeadStaRecIndex[ucTC] = u4CurStaIndex;
		prQM->au4ResourceUsedCount[ucTC] =
			u4CurStaUsedResource;
	}
#endif

	DBGLOG(QM, TEMP,
		"(Fairness) TC[%u] Scheduled Head STA[%u] Left Resource[%u]\n",
		ucTC, u4CurStaIndex, u4AvaliableResource);

	return u4AvaliableResource;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dequeue TX packets from a per-Type-based Queue for a particular TC
 *
 * \param[out] prQue The queue to put the dequeued packets
 * \param[in] ucTC The TC index(Shall always be BMC_TC_INDEX)
 * \param[in] ucMaxNum The maximum amount of available resource
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void
qmDequeueTxPacketsFromPerTypeQueues(struct ADAPTER *prAdapter,
	struct QUE *prQue, uint8_t ucTC,
	uint32_t u4CurrentQuota,
	uint32_t *prPleCurrentQuota,
	uint32_t u4TotalQuota)
{
	uint32_t u4AvaliableResource, u4LeftResource;
	uint32_t u4MaxResourceLimit;
	uint32_t u4TotalUsedResource = 0;
	struct QUE_MGT *prQM;
	PFN_DEQUEUE_FUNCTION pfnDeQFunc[2];
	u_int8_t fgChangeDeQFunc = TRUE;
	u_int8_t fgGlobalQueFirst;
	uint32_t u4MaxPageCntPerFrame;

	DBGLOG(QM, TEMP, "Enter %s (TC = %d, quota = %u)\n",
		__func__, ucTC, u4CurrentQuota);

	/* Broadcast/Multicast data packets */
	if (u4CurrentQuota == 0)
		return;
	/* Check PLE resource */
	if (!(*prPleCurrentQuota))
		return;

	prQM = &prAdapter->rQM;

	u4AvaliableResource = u4CurrentQuota;
	u4MaxResourceLimit = u4TotalQuota;
#if QM_FORWARDING_FAIRNESS
	u4TotalUsedResource = prQM->u4GlobalResourceUsedCount;
	fgGlobalQueFirst = prQM->fgGlobalQFirst;
#else
	fgGlobalQueFirst = TRUE;
#endif

	/* Dequeue function selection */
	if (fgGlobalQueFirst) {
		pfnDeQFunc[0] = qmDequeueTxPacketsFromGlobalQueue;
		pfnDeQFunc[1] = qmDequeueTxPacketsFromPerStaQueues;
	} else {
		pfnDeQFunc[0] = qmDequeueTxPacketsFromPerStaQueues;
		pfnDeQFunc[1] = qmDequeueTxPacketsFromGlobalQueue;
	}

	/* 1st dequeue function */
	u4LeftResource = pfnDeQFunc[0](prAdapter, prQue, ucTC,
		u4AvaliableResource,
		prPleCurrentQuota,
		(u4MaxResourceLimit - u4TotalUsedResource));

	/* dequeue function comsumes no resource, change */
	if (ucTC == TC4_INDEX)
		u4MaxPageCntPerFrame =
			prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame;
	else
		u4MaxPageCntPerFrame =
			prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame;

	if ((u4LeftResource >= u4AvaliableResource) &&
		(u4AvaliableResource >= u4MaxPageCntPerFrame)) {
		fgChangeDeQFunc = TRUE;
	} else {
		u4TotalUsedResource +=
			(u4AvaliableResource - u4LeftResource);
		/* Used resource exceeds limit, change */
		if (u4TotalUsedResource >= u4MaxResourceLimit)
			fgChangeDeQFunc = TRUE;
	}

	if (fgChangeDeQFunc) {
		fgGlobalQueFirst = !fgGlobalQueFirst;
		u4TotalUsedResource = 0;
	}

	/* 2nd dequeue function */
	u4LeftResource = pfnDeQFunc[1](prAdapter, prQue, ucTC,
		u4LeftResource, prPleCurrentQuota, u4MaxResourceLimit);

#if QM_FORWARDING_FAIRNESS
	prQM->fgGlobalQFirst = fgGlobalQueFirst;
	prQM->u4GlobalResourceUsedCount = u4TotalUsedResource;
#endif

}				/* qmDequeueTxPacketsFromPerTypeQueues */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dequeue TX packets from a QM global Queue for a particular TC
 *
 * \param[out] prQue The queue to put the dequeued packets
 * \param[in] ucTC The TC index(Shall always be BMC_TC_INDEX)
 * \param[in] ucMaxNum The maximum amount of available resource
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t
qmDequeueTxPacketsFromGlobalQueue(struct ADAPTER *prAdapter,
	struct QUE *prQue,
	uint8_t ucTC, uint32_t u4CurrentQuota,
	uint32_t *prPleCurrentQuota,
	uint32_t u4TotalQuota)
{
	struct BSS_INFO *prBssInfo;
	struct QUE *prCurrQueue;
	uint32_t u4AvaliableResource;
	struct MSDU_INFO *prDequeuedPkt;
	struct MSDU_INFO *prBurstEndPkt;
	struct QUE rMergeQue;
	struct QUE *prMergeQue;
	struct QUE_MGT *prQM;

	DBGLOG(QM, TEMP, "Enter %s (TC = %d, quota = %u)\n",
		__func__, ucTC, u4CurrentQuota);

	/* Broadcast/Multicast data packets */
	if (u4CurrentQuota == 0)
		return u4CurrentQuota;
	/* Check PLE resource */
	if (!(*prPleCurrentQuota))
		return u4CurrentQuota;

	prQM = &prAdapter->rQM;

	/* 4 <1> Determine the queue */
	prCurrQueue = &prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST];
	u4AvaliableResource = u4CurrentQuota;
	prDequeuedPkt = NULL;
	prBurstEndPkt = NULL;

	QUEUE_INITIALIZE(&rMergeQue);
	prMergeQue = &rMergeQue;

	/* 4 <2> Dequeue packets */
	while (!QUEUE_IS_EMPTY(prCurrQueue)) {
		prDequeuedPkt = QUEUE_GET_HEAD(prCurrQueue);
		if (prDequeuedPkt->u4PageCount > u4AvaliableResource)
			break;
		if ((*prPleCurrentQuota) < NIX_TX_PLE_PAGE_CNT_PER_FRAME)
			break;

		QUEUE_REMOVE_HEAD(prCurrQueue, prDequeuedPkt,
			struct MSDU_INFO *);

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prDequeuedPkt->ucBssIndex);

		if (prBssInfo && IS_BSS_ACTIVE(prBssInfo)) {
			if (!isNetAbsent(prAdapter, prBssInfo)
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
				&& ucTC == prDequeuedPkt->ucTC
#endif
				){
				/* to record WMM Set */
				prDequeuedPkt->ucWmmQueSet =
					prBssInfo->ucWmmQueSet;
				QUEUE_INSERT_TAIL(prQue, prDequeuedPkt);
				prBurstEndPkt = prDequeuedPkt;
				prQM->u4DequeueCounter++;
				u4AvaliableResource -=
					prDequeuedPkt->u4PageCount;
				(*prPleCurrentQuota) -=
					NIX_TX_PLE_PAGE_CNT_PER_FRAME;
				QM_DBG_CNT_INC(prQM, QM_DBG_CNT_26);
			} else {
				QUEUE_INSERT_TAIL(prMergeQue, prDequeuedPkt);
			}
		} else {
			QUEUE_ENTRY_SET_NEXT(prDequeuedPkt, NULL);
			wlanProcessQueuedMsduInfo(prAdapter, prDequeuedPkt);
		}
	}

	if (QUEUE_IS_NOT_EMPTY(prMergeQue)) {
		QUEUE_CONCATENATE_QUEUES(prMergeQue, prCurrQueue);
		QUEUE_MOVE_ALL(prCurrQueue, prMergeQue);
		QUEUE_ENTRY_SET_NEXT(QUEUE_GET_TAIL(prCurrQueue), NULL);
	}

	return u4AvaliableResource;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dequeue TX packets to send to HIF TX
 *
 * \param[in] prTcqStatus Info about the maximum amount of dequeued packets
 *
 * \return The list of dequeued TX packets
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *qmDequeueTxPackets(struct ADAPTER *prAdapter,
	struct TX_TCQ_STATUS *prTcqStatus)
{
	int32_t i;
	struct MSDU_INFO *prReturnedPacketListHead;
	struct QUE rReturnedQue;
	uint32_t u4MaxQuotaLimit;
	uint32_t u4AvailableResourcePLE, u4AvailableResourcePSE;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	uint8_t ucWmmIdx;
#endif

	DBGLOG(QM, TEMP, "Enter qmDequeueTxPackets\n");

	QUEUE_INITIALIZE(&rReturnedQue);

	prReturnedPacketListHead = NULL;

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	for (i = TC_NUM-1; i >= TC0_INDEX; i--) {
		if (i == TC4_INDEX) {
			/* ignore TC4: command */
			continue;
		}
		if (!NIC_TX_RES_IS_ACTIVE(prAdapter, i))
			continue;
#else
	/* TC0 to TC3: AC0~AC3 (commands packets are not handled by QM) */
	for (i = TC3_INDEX; i >= TC0_INDEX; i--) {
#endif
		DBGLOG(QM, TEMP, "Dequeue packets from Per-STA queue[%u]\n", i);

		u4AvailableResourcePLE = nicTxResourceGetPleFreeCount(
			prAdapter, i);
		u4AvailableResourcePSE = nicTxResourceGetPseFreeCount(
			prAdapter, i);

		/* If only one STA is Tx allowed,
		 * no need to restrict Max quota
		 */
		if (prAdapter->rWifiVar.u4MaxTxDeQLimit)
			u4MaxQuotaLimit = prAdapter->rWifiVar.u4MaxTxDeQLimit;
		else if (prAdapter->rQM.u4TxAllowedStaCount == 1)
			u4MaxQuotaLimit = QM_STA_FORWARD_COUNT_UNLIMITED;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		else if (u4AvailableResourcePSE ==
				QM_STA_FORWARD_COUNT_UNLIMITED)
			u4MaxQuotaLimit = QM_STA_FORWARD_COUNT_UNLIMITED;
#endif
		else
			u4MaxQuotaLimit =
				(uint32_t) prTcqStatus->au4MaxNumOfPage[i];

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		ucWmmIdx = nicTxGetWmmIdxByTc(i);
		if (i == nicTxWmmTc2ResTc(prAdapter,
			ucWmmIdx, NET_TC_BMC_INDEX))
#else
		if (i == BMC_TC_INDEX)
#endif
			qmDequeueTxPacketsFromPerTypeQueues(prAdapter,
				&rReturnedQue, (uint8_t)i,
				u4AvailableResourcePSE,
				&u4AvailableResourcePLE,
				u4MaxQuotaLimit);
		else
			qmDequeueTxPacketsFromPerStaQueues(prAdapter,
				&rReturnedQue,
				(uint8_t)i,
				u4AvailableResourcePSE,
				&u4AvailableResourcePLE,
				u4MaxQuotaLimit);

		/* The aggregate number of dequeued packets */
		DBGLOG(QM, TEMP, "DQA)[%u](%u)\n", i,
			rReturnedQue.u4NumElem);
	}

	if (QUEUE_IS_NOT_EMPTY(&rReturnedQue)) {
		prReturnedPacketListHead = QUEUE_GET_HEAD(&rReturnedQue);
		QUEUE_ENTRY_SET_NEXT(QUEUE_GET_TAIL(&rReturnedQue), NULL);
	}

	return prReturnedPacketListHead;
}

#if CFG_SUPPORT_MULTITHREAD
/*----------------------------------------------------------------------------*/
/*!
 * \brief Dequeue TX packets to send to HIF TX
 *
 * \param[in] prTcqStatus Info about the maximum amount of dequeued packets
 *
 * \return The list of dequeued TX packets
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *qmDequeueTxPacketsMthread(
	struct ADAPTER *prAdapter,
	struct TX_TCQ_STATUS *prTcqStatus)
{

	/* INT_32 i; */
	struct MSDU_INFO *prReturnedPacketListHead;
	/* QUE_T rReturnedQue; */
	/* UINT_32 u4MaxQuotaLimit; */
	struct MSDU_INFO *prMsduInfo, *prNextMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	if (prAdapter->rQM.fgForceReassign)
		qmDoAdaptiveTcResourceCtrl(prAdapter);
#endif

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	prReturnedPacketListHead = qmDequeueTxPackets(prAdapter,
		prTcqStatus);

	/* require the resource first to prevent from unsync */
	prMsduInfo = prReturnedPacketListHead;
	while (prMsduInfo) {
		prNextMsduInfo = QUEUE_GET_NEXT_ENTRY(prMsduInfo);
#if (CFG_SUPPORT_TRACE_TC4 == 1)
	if (prMsduInfo->ucTC == TC4_INDEX)
		DBGLOG(TX, ERROR, "[ERROR] sdio tx resource!! ucTC=%d\n",
		  prMsduInfo->ucTC);
#endif
		nicTxAcquireResource(prAdapter, prMsduInfo->ucTC,
			nicTxGetDataPageCount(prAdapter,
			    prMsduInfo->u2FrameLength,
			    FALSE), FALSE);
		prMsduInfo = prNextMsduInfo;
	}

	if (prReturnedPacketListHead)
		wlanTxProfilingTagMsdu(prAdapter, prReturnedPacketListHead,
			TX_PROF_TAG_DRV_DEQUE);

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	return prReturnedPacketListHead;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Adjust the PLE-TC quotas according to traffic demands
 *
 * \param[out] prTcqAdjust The resulting adjustment
 * \param[in] prTcqStatus Info about the current TC quotas and counters
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t
_qmAdjustTcQuotasMthread_PLE(struct ADAPTER *prAdapter,
	struct TX_TCQ_ADJUST *prTcqAdjust,
	struct TX_TCQ_STATUS *prTcqStatus)
{
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	uint32_t i;
	struct QUE_MGT *prQM = &prAdapter->rQM;
	uint8_t ucPleVariation = FALSE;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	int8_t j;
	uint8_t startTc;
	struct TX_CTRL *prTxCtrl;
#endif

	/* This API works if TC with PSE-disabled+PLE-enabled is found */
	for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
		if (nicTxResourceIsPleCtrlNeeded(prAdapter, i) &&
			!nicTxResourceIsPseCtrlNeeded(prAdapter, i)) {
			/* PLE only case found */
			ucPleVariation = TRUE;
			break;
		}
	}
	if (!ucPleVariation)
		return FALSE;

	/* 4 <2> Adjust TcqStatus according to
	 * the updated prQM->au4CurrentTcResource
	 */
	else {
		int32_t i4TotalExtraQuota = 0;
		int32_t ai4ExtraQuota[QM_ACTIVE_TC_NUM];
		u_int8_t fgResourceRedistributed = TRUE;

		/* Must initialize */
		for (i = 0; i < TC_NUM; i++)
			prTcqAdjust->ai4Variation[i] = 0;

		/* Obtain the free-to-distribute resource */
		for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
			if (!nicTxResourceIsPleCtrlNeeded(prAdapter, i))
				continue;
			ai4ExtraQuota[i] =
			(int32_t) prTcqStatus->au4MaxNumOfBuffer_PLE[i] -
			(int32_t) prQM->au4CurrentTcResource[i];

			if (ai4ExtraQuota[i] > 0) {
				/* The resource shall be reallocated
				 * to other TCs
				 */
				if (ai4ExtraQuota[i] >
					prTcqStatus->au4FreeBufferCount_PLE[
					i]) {
					ai4ExtraQuota[i] =
						prTcqStatus->
						au4FreeBufferCount_PLE[i];
					fgResourceRedistributed = FALSE;
				}

				i4TotalExtraQuota += ai4ExtraQuota[i];
				prTcqAdjust->ai4Variation[i] =
					(-ai4ExtraQuota[i]);
			}
		}

		/* Distribute quotas to TCs which need extra resource
		 * according to prQM->au4CurrentTcResource
		 */
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		ASSERT(prAdapter);

		prTxCtrl = &prAdapter->rTxCtrl;
		ASSERT(prTxCtrl);

		/*start from VO*/
		startTc = nicTxWmmTc2ResTc(prAdapter,
			prTxCtrl->rTc.ucNextHifWmmIdx,
			NET_TC_WMM_AC_VO_INDEX);

		for (j = QM_ACTIVE_TC_NUM; j > 0; j--) {
			i = (startTc + j) % QM_ACTIVE_TC_NUM;
#else
		for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
#endif
			if (!nicTxResourceIsPleCtrlNeeded(prAdapter, i))
				continue;
			if (ai4ExtraQuota[i] < 0) {
				if ((-ai4ExtraQuota[i]) > i4TotalExtraQuota) {
					ai4ExtraQuota[i] = (-i4TotalExtraQuota);
					fgResourceRedistributed = FALSE;
				}

				i4TotalExtraQuota += ai4ExtraQuota[i];
				prTcqAdjust->ai4Variation[i] =
					(-ai4ExtraQuota[i]);
			}
		}
		/* In case some TC is waiting for TX Done,
		 * continue to adjust TC quotas upon TX Done
		 */
		prQM->fgTcResourcePostAnnealing = (!fgResourceRedistributed);

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		/* Fix TC9 AVQ=9x but qmDoAdaptiveTcResourceCtrl()
		 * is not called,
		 * resulting in TC9 has no resource to transmit
		 */
		if (prQM->fgTcResourceFastReaction) {
			prQM->fgTcResourcePostAnnealing = FALSE;
			prQM->u4TimeToAdjustTcResource = 1;
		}
#endif
	}

	return TRUE;
#else
	return FALSE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Adjust the TC quotas according to traffic demands
 *
 * \param[out] prTcqAdjust The resulting adjustment
 * \param[in] prTcqStatus Info about the current TC quotas and counters
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t
qmAdjustTcQuotasMthread(struct ADAPTER *prAdapter,
	struct TX_TCQ_ADJUST *prTcqAdjust,
	struct TX_TCQ_STATUS *prTcqStatus)
{
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	uint32_t i;
	struct QUE_MGT *prQM = &prAdapter->rQM;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	int8_t j;
	uint8_t startTc;
	struct TX_CTRL *prTxCtrl;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	/* Must initialize */
	for (i = 0; i < QM_ACTIVE_TC_NUM; i++)
		prTcqAdjust->ai4Variation[i] = 0;

	/* 4 <1> If TC resource is not just adjusted, exit directly */
	if (!prQM->fgTcResourcePostAnnealing)
		return FALSE;

	/* 4 <2> Adjust TcqStatus according to
	 * the updated prQM->au4CurrentTcResource
	 */
	else {
		int32_t i4TotalExtraQuota = 0;
		int32_t ai4ExtraQuota[QM_ACTIVE_TC_NUM];
		u_int8_t fgResourceRedistributed = TRUE;

		/* Must initialize */
		for (i = 0; i < TC_NUM; i++)
			prTcqAdjust->ai4Variation[i] = 0;

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

		/* Obtain the free-to-distribute resource */
		for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
			if (!nicTxResourceIsPseCtrlNeeded(prAdapter, i))
				continue;

			ai4ExtraQuota[i] =
				(int32_t) prTcqStatus->au4MaxNumOfBuffer[i] -
				(int32_t) prQM->au4CurrentTcResource[i];

			if (ai4ExtraQuota[i] > 0) {
				/* The resource shall be reallocated
				 * to other TCs
				 */
				if (ai4ExtraQuota[i] >
					prTcqStatus->au4FreeBufferCount[
					i]) {
					ai4ExtraQuota[i] =
						prTcqStatus->
						au4FreeBufferCount[i];
					fgResourceRedistributed = FALSE;
				}

				i4TotalExtraQuota += ai4ExtraQuota[i];
				prTcqAdjust->ai4Variation[i] =
					(-ai4ExtraQuota[i]);
			}
		}

		/* Distribute quotas to TCs which need extra resource
		 * according to prQM->au4CurrentTcResource
		 */
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		ASSERT(prAdapter);

		prTxCtrl = &prAdapter->rTxCtrl;
		ASSERT(prTxCtrl);

		/*start from VO*/
		startTc = nicTxWmmTc2ResTc(prAdapter,
			prTxCtrl->rTc.ucNextHifWmmIdx,
			NET_TC_WMM_AC_VO_INDEX);

		/*update next index*/
		prTxCtrl->rTc.ucNextHifWmmIdx++;
		prTxCtrl->rTc.ucNextHifWmmIdx %= HIF_WMM_SET_NUM;

		for (j = QM_ACTIVE_TC_NUM; j > 0; j--) {
			i = (startTc + j) % QM_ACTIVE_TC_NUM;
#else
		for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
#endif
			if (!nicTxResourceIsPseCtrlNeeded(prAdapter, i))
				continue;

			if (ai4ExtraQuota[i] < 0) {
				if ((-ai4ExtraQuota[i]) > i4TotalExtraQuota) {
					ai4ExtraQuota[i] = (-i4TotalExtraQuota);
					fgResourceRedistributed = FALSE;
				}

				i4TotalExtraQuota += ai4ExtraQuota[i];
				prTcqAdjust->ai4Variation[i] =
					(-ai4ExtraQuota[i]);
			}
		}

		/* In case some TC is waiting for TX Done,
		 * continue to adjust TC quotas upon TX Done
		 */
		prQM->fgTcResourcePostAnnealing = (!fgResourceRedistributed);

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		/* Fix TC9 AVQ=9x but qmDoAdaptiveTcResourceCtrl()
		 * is not called,
		 * resulting in TC9 has no resource to transmit
		 */
		if (prQM->fgTcResourceFastReaction) {
			prQM->fgTcResourcePostAnnealing = FALSE;
			prQM->u4TimeToAdjustTcResource = 1;
		}
#endif

		for (i = 0; i < TC_NUM; i++) {
			if (!nicTxResourceIsPseCtrlNeeded(prAdapter, i))
				continue;

			if (i == TC4_INDEX) {
				prTcqStatus->au4FreePageCount[i] +=
					(prTcqAdjust->ai4Variation[i] *
				prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame);
				prTcqStatus->au4MaxNumOfPage[i] +=
					(prTcqAdjust->ai4Variation[i] *
				prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame);

			} else {
				prTcqStatus->au4FreePageCount[i] +=
					(prTcqAdjust->ai4Variation[i] *
				prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame);
				prTcqStatus->au4MaxNumOfPage[i] +=
					(prTcqAdjust->ai4Variation[i] *
				prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame);

			}

			prTcqStatus->au4FreeBufferCount[i] +=
				prTcqAdjust->ai4Variation[i];
			prTcqStatus->au4MaxNumOfBuffer[i] +=
				prTcqAdjust->ai4Variation[i];
		}


		/* PLE */
		qmAdjustTcQuotaPle(prAdapter, prTcqAdjust, prTcqStatus);


#if QM_FAST_TC_RESOURCE_CTRL
		prQM->fgTcResourceFastReaction = FALSE;
#endif
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
	}

	return TRUE;
#else
	return FALSE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Adjust the TC PLE quotas according to traffic demands
 *
 * \param[out] prTcqAdjust The resulting adjustment
 * \param[in] prTcqStatus Info about the current TC quotas and counters
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmAdjustTcQuotaPle(struct ADAPTER *prAdapter,
	struct TX_TCQ_ADJUST *prTcqAdjust,
	struct TX_TCQ_STATUS *prTcqStatus)
{
	uint8_t i;
	int32_t i4pages;
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;
	int32_t i4TotalExtraQuota = 0;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

	/* no PLE resource control */
	if (!prTc->fgNeedPleCtrl)
		return;

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	_qmAdjustTcQuotasMthread_PLE(prAdapter,
			prTcqAdjust,
			prTcqStatus);
#endif

	/* collect free PLE resource */
	for (i = TC0_INDEX; i < TC_NUM; i++) {

		if (!nicTxResourceIsPleCtrlNeeded(prAdapter, i))
			continue;

		/* adjust ple resource */
		i4pages = prTcqAdjust->ai4Variation[i] *
				NIX_TX_PLE_PAGE_CNT_PER_FRAME;

		if (i4pages < 0) {
			/* donate resource to other TC */
			if (prTcqStatus->au4FreePageCount_PLE[i] < (-i4pages)) {
				/* not enough to give */
				i4pages =
				(-1) * (prTcqStatus->au4FreePageCount_PLE[i]);
			}
			i4TotalExtraQuota += -i4pages;

			prTcqStatus->au4FreePageCount_PLE[i] += i4pages;
			prTcqStatus->au4MaxNumOfPage_PLE[i] += i4pages;

			prTcqStatus->au4FreeBufferCount_PLE[i] +=
				(i4pages / NIX_TX_PLE_PAGE_CNT_PER_FRAME);
			prTcqStatus->au4MaxNumOfBuffer_PLE[i] +=
				(i4pages / NIX_TX_PLE_PAGE_CNT_PER_FRAME);
		}
	}

	/* distribute PLE resource */
	for (i = TC0_INDEX; i < TC_NUM; i++) {
		if (!nicTxResourceIsPleCtrlNeeded(prAdapter, i))
			continue;

		/* adjust ple resource */
		i4pages = prTcqAdjust->ai4Variation[i] *
				NIX_TX_PLE_PAGE_CNT_PER_FRAME;

		if (i4pages > 0) {
			if (i4TotalExtraQuota >= i4pages) {
				i4TotalExtraQuota -= i4pages;
			} else {
				i4pages = i4TotalExtraQuota;
				i4TotalExtraQuota = 0;
			}
			prTcqStatus->au4FreePageCount_PLE[i] += i4pages;
			prTcqStatus->au4MaxNumOfPage_PLE[i] += i4pages;

			prTcqStatus->au4FreeBufferCount_PLE[i] +=
				(i4pages / NIX_TX_PLE_PAGE_CNT_PER_FRAME);
			prTcqStatus->au4MaxNumOfBuffer_PLE[i] +=
				(i4pages / NIX_TX_PLE_PAGE_CNT_PER_FRAME);
		}
	}

	/* distribute remaining PLE resource */
	while (i4TotalExtraQuota != 0) {
		DBGLOG(QM, INFO,
				"distribute remaining PLE resource[%u]\n",
				i4TotalExtraQuota);
		for (i = TC0_INDEX; i < TC_NUM; i++) {
			if (!nicTxResourceIsPleCtrlNeeded(prAdapter, i))
				continue;

			if (i4TotalExtraQuota >=
				NIX_TX_PLE_PAGE_CNT_PER_FRAME) {
				prTcqStatus->au4FreePageCount_PLE[i] +=
					NIX_TX_PLE_PAGE_CNT_PER_FRAME;
				prTcqStatus->au4MaxNumOfPage_PLE[i] +=
					NIX_TX_PLE_PAGE_CNT_PER_FRAME;

				prTcqStatus->au4FreeBufferCount_PLE[i] += 1;
				prTcqStatus->au4MaxNumOfBuffer_PLE[i] += 1;

				i4TotalExtraQuota -=
					NIX_TX_PLE_PAGE_CNT_PER_FRAME;
			} else {
				/* remaining PLE pages are
				 * not enough for a package
				 */
				prTcqStatus->au4FreePageCount_PLE[i] +=
					i4TotalExtraQuota;
				prTcqStatus->au4MaxNumOfPage_PLE[i] +=
					i4TotalExtraQuota;

				prTcqStatus->au4FreeBufferCount_PLE[i] +=
					(i4TotalExtraQuota /
					NIX_TX_PLE_PAGE_CNT_PER_FRAME);
				prTcqStatus->au4MaxNumOfBuffer_PLE[i] +=
					(i4TotalExtraQuota /
					NIX_TX_PLE_PAGE_CNT_PER_FRAME);

				i4TotalExtraQuota = 0;
			}
		}
	}

}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Adjust the TC quotas according to traffic demands
 *
 * \param[out] prTcqAdjust The resulting adjustment
 * \param[in] prTcqStatus Info about the current TC quotas and counters
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t qmAdjustTcQuotas(struct ADAPTER *prAdapter,
	struct TX_TCQ_ADJUST *prTcqAdjust,
	struct TX_TCQ_STATUS *prTcqStatus)
{
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	uint32_t i;
	struct QUE_MGT *prQM = &prAdapter->rQM;

	/* Must initialize */
	for (i = 0; i < QM_ACTIVE_TC_NUM; i++)
		prTcqAdjust->ai4Variation[i] = 0;

	/* 4 <1> If TC resource is not just adjusted, exit directly */
	if (!prQM->fgTcResourcePostAnnealing)
		return FALSE;
	/* 4 <2> Adjust TcqStatus according to
	 * the updated prQM->au4CurrentTcResource
	 */
	else {
		int32_t i4TotalExtraQuota = 0;
		int32_t ai4ExtraQuota[QM_ACTIVE_TC_NUM];
		u_int8_t fgResourceRedistributed = TRUE;

		/* Obtain the free-to-distribute resource */
		for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
			ai4ExtraQuota[i] =
				(int32_t) prTcqStatus->au4MaxNumOfBuffer[i] -
				(int32_t) prQM->au4CurrentTcResource[i];

			if (ai4ExtraQuota[i] > 0) {
				/* The resource shall be
				 * reallocated to other TCs
				 */
				if (ai4ExtraQuota[i] >
					prTcqStatus->au4FreeBufferCount[i]) {
					ai4ExtraQuota[i] =
						prTcqStatus->
						au4FreeBufferCount[i];
					fgResourceRedistributed = FALSE;
				}

				i4TotalExtraQuota += ai4ExtraQuota[i];
				prTcqAdjust->ai4Variation[i] =
					(-ai4ExtraQuota[i]);
			}
		}

		/* Distribute quotas to TCs which need extra resource
		 * according to prQM->au4CurrentTcResource
		 */
		for (i = 0; i < QM_ACTIVE_TC_NUM; i++) {
			if (ai4ExtraQuota[i] < 0) {
				if ((-ai4ExtraQuota[i]) > i4TotalExtraQuota) {
					ai4ExtraQuota[i] = (-i4TotalExtraQuota);
					fgResourceRedistributed = FALSE;
				}

				i4TotalExtraQuota += ai4ExtraQuota[i];
				prTcqAdjust->ai4Variation[i] =
					(-ai4ExtraQuota[i]);
			}
		}

		/* In case some TC is waiting for TX Done,
		 * continue to adjust TC quotas upon TX Done
		 */
		prQM->fgTcResourcePostAnnealing = (!fgResourceRedistributed);

#if QM_FAST_TC_RESOURCE_CTRL
		prQM->fgTcResourceFastReaction = FALSE;
#endif

#if QM_PRINT_TC_RESOURCE_CTRL
		DBGLOG(QM, LOUD,
			"QM: Curr Quota [0]=%u [1]=%u [2]=%u [3]=%u [4]=%u [5]=%u\n",
			prTcqStatus->au4FreeBufferCount[0],
			prTcqStatus->au4FreeBufferCount[1],
			prTcqStatus->au4FreeBufferCount[2],
			prTcqStatus->au4FreeBufferCount[3],
			prTcqStatus->au4FreeBufferCount[4],
			prTcqStatus->au4FreeBufferCount[5]);
#endif
	}

	return TRUE;
#else
	return FALSE;
#endif
}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
void qmCalAveQLen(struct QUE_MGT *prQM, uint32_t u4Tc, uint32_t u4CurrQueLen)
{
	if (prQM->au4AverageQueLen[u4Tc] == 0) {
		prQM->au4AverageQueLen[u4Tc] =
		(u4CurrQueLen << prQM->u4QueLenMovingAverage);
	} else {
		prQM->au4AverageQueLen[u4Tc] -=
		(prQM->au4AverageQueLen[u4Tc] >> prQM->u4QueLenMovingAverage);

		prQM->au4AverageQueLen[u4Tc] += (u4CurrQueLen);
	}

	DBGLOG(HIF_WMM_ENHANCE, LOUD,
		"TC[%u], u4CurrQueLen = %u, avQLen = %u\n",
		u4Tc, u4CurrQueLen, prQM->au4AverageQueLen[u4Tc]);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update the average TX queue length for the TC resource control
 *        mechanism
 *
 * \param (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmUpdateAverageTxQueLen(struct ADAPTER *prAdapter)
{
	uint32_t u4Tc, u4StaRecIdx;
	struct STA_RECORD *prStaRec;
	struct QUE_MGT *prQM = &prAdapter->rQM;
	struct BSS_INFO *prBssInfo;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	uint32_t arStaQNum[TC_NUM] = {0}, ucBcmCnt;
	uint8_t ucWmmSet = 0, ucAc;

	/* 1. Collect stations' each queue length*/
	/*based on AC q index to search all STA_TxQ*/
	for (ucAc = 0; ucAc < NUM_OF_PER_STA_TX_QUEUES; ucAc++) {

		/*Per STA search for a specific AC*/
		for (u4StaRecIdx = 0; u4StaRecIdx < CFG_STA_REC_NUM;
			u4StaRecIdx++) {
			uint8_t ucTcQ;

			prStaRec = cnmGetStaRecByIndex(prAdapter, u4StaRecIdx);
			if (!prStaRec || !prStaRec->fgIsValid)
				continue;

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					prStaRec->ucBssIndex);
			if (!prBssInfo || isNetAbsent(prAdapter, prBssInfo))
				continue;

			/* If the STA is activated,
			 * get the queue length
			 */
			ucWmmSet = prStaRec->ucBssIndex;
			u4Tc = nicTxWmmTc2ResTc(prAdapter, ucWmmSet, ucAc);
			ucTcQ = aucACI2TxQIdx[ucAc];

			arStaQNum[u4Tc] +=
				prStaRec->arTxQueue[ucTcQ].u4NumElem;
		}
	}

	/* 2. Add BCM_IDX queue length for each wmm set
	 *	 and Update total queue length
	 */
	for (u4Tc = 0; u4Tc < TC_NUM; u4Tc++) {
		ucBcmCnt = 0;
		ucWmmSet = nicTxGetWmmIdxByTc(u4Tc);

		/* For BMC_IDX */
		if (u4Tc == nicTxWmmTc2ResTc(prAdapter,
				ucWmmSet, NET_TC_BMC_INDEX))
			ucBcmCnt = prQM->arTxQueue[ucWmmSet].u4NumElem;

		/*Update total queue length for each TC*/
		qmCalAveQLen(prQM, u4Tc, ucBcmCnt + arStaQNum[u4Tc]);
	}
#else
	int32_t u4CurrQueLen;


	/* 4 <1> Update the queue lengths for TC0 to TC3 (skip TC4) and TC5 */
	for (u4Tc = 0; u4Tc < QM_ACTIVE_TC_NUM; u4Tc++) {
		u4CurrQueLen = 0;

		/* Calculate per-STA queue length */
		if (u4Tc < NUM_OF_PER_STA_TX_QUEUES) {
			for (u4StaRecIdx = 0; u4StaRecIdx < CFG_STA_REC_NUM;
			     u4StaRecIdx++) {
				prStaRec = cnmGetStaRecByIndex(prAdapter,
					u4StaRecIdx);
				if (prStaRec) {
					prBssInfo = GET_BSS_INFO_BY_INDEX(
						prAdapter,
						prStaRec->ucBssIndex);

					/* If the STA is activated,
					 * get the queue length
					 */
					if (prStaRec->fgIsValid && prBssInfo &&
					    !isNetAbsent(prAdapter, prBssInfo))
						u4CurrQueLen +=
							(prStaRec->
							arTxQueue[u4Tc].
							u4NumElem);
				}
			}
		}

		if (u4Tc == BMC_TC_INDEX) {
			/* Update the queue length for (BMCAST) */
			u4CurrQueLen += prQM->arTxQueue[
			TX_QUEUE_INDEX_BMCAST].u4NumElem;
		}

		if (prQM->au4AverageQueLen[u4Tc] == 0) {
			prQM->au4AverageQueLen[u4Tc] = (u4CurrQueLen <<
				prQM->u4QueLenMovingAverage);
		} else {
			prQM->au4AverageQueLen[u4Tc] -=
				(prQM->au4AverageQueLen[u4Tc] >>
				 prQM->u4QueLenMovingAverage);
			prQM->au4AverageQueLen[u4Tc] += (u4CurrQueLen);
		}
	}
#endif
#if 0
	/* Update the queue length for TC5 (BMCAST) */
	u4CurrQueLen =
		prQM->arTxQueue[TX_QUEUE_INDEX_BMCAST].u4NumElem;

	if (prQM->au4AverageQueLen[TC5_INDEX] == 0) {
		prQM->au4AverageQueLen[TC5_INDEX] = (u4CurrQueLen <<
			QM_QUE_LEN_MOVING_AVE_FACTOR);
	} else {
		prQM->au4AverageQueLen[TC5_INDEX] -=
			(prQM->au4AverageQueLen[TC5_INDEX] >>
			QM_QUE_LEN_MOVING_AVE_FACTOR);
		prQM->au4AverageQueLen[TC5_INDEX] += (u4CurrQueLen);
	}
#endif
}

void qmAllocateResidualTcResource(struct ADAPTER *prAdapter,
	int32_t *ai4TcResDemand,
	uint32_t *pu4ResidualResource,
	uint32_t *pu4ShareCount)
{
	struct QUE_MGT *prQM = &prAdapter->rQM;
	uint32_t u4Share = 0;
	uint32_t u4TcIdx;
	uint8_t ucIdx;
	uint32_t au4AdjTc[] = { TC3_INDEX, TC2_INDEX, TC1_INDEX, TC0_INDEX };
	uint32_t u4AdjTcSize = ARRAY_SIZE(au4AdjTc);
	uint32_t u4ResidualResource = *pu4ResidualResource;
	uint32_t u4ShareCount = *pu4ShareCount;
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	uint8_t ucHifWmmSet;
#endif

	/* If there is no resource left, exit directly */
	if (u4ResidualResource == 0)
		return;

	/* This shall not happen */
	if (u4ShareCount == 0) {
		prQM->au4CurrentTcResource[TC1_INDEX] += u4ResidualResource;
		DBGLOG(QM, ERROR, "QM: (Error) u4ShareCount = 0\n");
		return;
	}

	/* Share the residual resource evenly */
	u4Share = (u4ResidualResource / u4ShareCount);
	if (u4Share) {
		for (u4TcIdx = 0; u4TcIdx < QM_ACTIVE_TC_NUM; u4TcIdx++) {
			/* Skip TC4 (not adjustable) */
			if (u4TcIdx == TC4_INDEX)
				continue;
			if (!NIC_TX_RES_IS_ACTIVE(prAdapter, u4TcIdx))
				continue;

			if (ai4TcResDemand[u4TcIdx] > 0) {
				if (ai4TcResDemand[u4TcIdx] > u4Share) {
					prQM->au4CurrentTcResource[u4TcIdx] +=
						u4Share;
					u4ResidualResource -= u4Share;
					ai4TcResDemand[u4TcIdx] -= u4Share;
				} else {
					prQM->au4CurrentTcResource[u4TcIdx] +=
						ai4TcResDemand[u4TcIdx];
					u4ResidualResource -=
						ai4TcResDemand[u4TcIdx];
					ai4TcResDemand[u4TcIdx] = 0;
					u4ShareCount--;
				}
			}
		}
	}

	/* By priority, allocate the left resource
	 * that is not divisible by u4Share
	 */
	ucIdx = 0;
	while (u4ResidualResource) {

		if (u4ShareCount == 0) {
			/*break from while loop*/
			break;
		}

		u4TcIdx = au4AdjTc[ucIdx];

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		/* For each AcIdx, loop all BSS as well */
		for (ucHifWmmSet = 0;
			ucHifWmmSet < HIF_WMM_SET_NUM;
			ucHifWmmSet++) {
			u4TcIdx = nicTxWmmTc2ResTc(prAdapter,
				ucHifWmmSet, au4AdjTc[ucIdx]);
#endif
		if (ai4TcResDemand[u4TcIdx]) {
			prQM->au4CurrentTcResource[u4TcIdx]++;
			u4ResidualResource--;
			ai4TcResDemand[u4TcIdx]--;

			if (ai4TcResDemand[u4TcIdx] == 0)
				u4ShareCount--;
		}

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		/* Add contorls for addtional WMM-loop */
		}
#endif
		ucIdx++;
		ucIdx %= u4AdjTcSize;
	}

	/* Allocate the left resource */
	prQM->au4CurrentTcResource[TC3_INDEX] += u4ResidualResource;

	*pu4ResidualResource = u4ResidualResource;
	*pu4ShareCount = u4ShareCount;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Assign TX resource for each TC according to TX queue length and
 *        current assignment
 *
 * \param (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmReassignTcResource(struct ADAPTER *prAdapter)
{
	int32_t i4TotalResourceDemand = 0;
	uint32_t u4ResidualResource = 0;
	uint32_t u4TcIdx;
	int32_t ai4TcResDemand[QM_ACTIVE_TC_NUM];
	uint32_t u4ShareCount = 0;
	uint32_t u4Share = 0;
	struct QUE_MGT *prQM = &prAdapter->rQM;
	uint32_t u4ActiveTcCount = 0;
	uint32_t u4LastActiveTcIdx = TC3_INDEX;

	/* Note: After the new assignment is obtained,
	 * set prQM->fgTcResourcePostAnnealing to TRUE to
	 * start the TC-quota adjusting procedure,
	 * which will be invoked upon every TX Done
	 */

	/* 4 <1> Determine the demands */
	/* Determine the amount of extra resource
	 * to fulfill all of the demands
	 */
	for (u4TcIdx = 0; u4TcIdx < QM_ACTIVE_TC_NUM; u4TcIdx++) {
		/* Skip TC4, which is not adjustable */
		if (u4TcIdx == TC4_INDEX)
			continue;
		if (!NIC_TX_RES_IS_ACTIVE(prAdapter, u4TcIdx))
			continue;

		/* Define: extra_demand = que_length +
		 * min_reserved_quota - current_quota
		 */
		ai4TcResDemand[u4TcIdx] = ((int32_t)(QM_GET_TX_QUEUE_LEN(
			prAdapter, u4TcIdx) +
			prQM->au4MinReservedTcResource[u4TcIdx]) -
			(int32_t)prQM->au4CurrentTcResource[u4TcIdx]);

		/* If there are queued packets, allocate extra resource
		 * for the TC (for TCP consideration)
		 */
		if (QM_GET_TX_QUEUE_LEN(prAdapter, u4TcIdx)) {
			ai4TcResDemand[u4TcIdx] +=
				prQM->u4ExtraReservedTcResource;
			u4ActiveTcCount++;
		}

		i4TotalResourceDemand += ai4TcResDemand[u4TcIdx];
	}

	/* 4 <2> Case 1: Demand <= Total Resource */
	if (i4TotalResourceDemand <= 0) {

		/* 4 <2.1> Calculate the residual resource evenly */
		/* excluding TC4 */
		if (u4ActiveTcCount == 0)
			u4ShareCount = (QM_ACTIVE_TC_NUM - 1);
		else
			u4ShareCount = u4ActiveTcCount;

		u4ResidualResource = (uint32_t) (-i4TotalResourceDemand);
		u4Share = (u4ResidualResource / u4ShareCount);

		/* 4 <2.2> Satisfy every TC and share
		 * the residual resource evenly
		 */
		for (u4TcIdx = 0; u4TcIdx < QM_ACTIVE_TC_NUM; u4TcIdx++) {
			/* Skip TC4 (not adjustable) */
			if (u4TcIdx == TC4_INDEX)
				continue;
			if (!NIC_TX_RES_IS_ACTIVE(prAdapter, u4TcIdx))
				continue;

			prQM->au4CurrentTcResource[u4TcIdx] +=
				ai4TcResDemand[u4TcIdx];

			/* Every TC is fully satisfied */
			ai4TcResDemand[u4TcIdx] = 0;

			/* The left resource will be allocated */
			if (QM_GET_TX_QUEUE_LEN(prAdapter, u4TcIdx) ||
				(u4ActiveTcCount == 0)) {
				prQM->au4CurrentTcResource[u4TcIdx] += u4Share;
				u4ResidualResource -= u4Share;
				u4LastActiveTcIdx = u4TcIdx;
			}
		}

		/* 4 <2.3> Allocate the left resource to last active TC */
		prQM->au4CurrentTcResource[u4LastActiveTcIdx] +=
			(u4ResidualResource);

	}
	/* 4 <3> Case 2: Demand > Total Resource --> Guarantee
	 * a minimum amount of resource for each TC
	 */
	else {
		u4ShareCount = 0;
		u4ResidualResource = prQM->u4ResidualTcResource;

		/* 4 <3.1> Allocated resouce amount  = minimum
		 * of (guaranteed, total demand)
		 */
		for (u4TcIdx = 0; u4TcIdx < QM_ACTIVE_TC_NUM; u4TcIdx++) {
			/* Skip TC4 (not adjustable) */
			if (u4TcIdx == TC4_INDEX)
				continue;
			if (!NIC_TX_RES_IS_ACTIVE(prAdapter, u4TcIdx))
				continue;

			/* The demand can be fulfilled with
			 * the guaranteed resource amount
			 */
			if ((prQM->au4CurrentTcResource[u4TcIdx] +
			     ai4TcResDemand[u4TcIdx]) <=
			    prQM->au4GuaranteedTcResource[u4TcIdx]) {

				prQM->au4CurrentTcResource[u4TcIdx] +=
					ai4TcResDemand[u4TcIdx];
				u4ResidualResource +=
					(prQM->au4GuaranteedTcResource[u4TcIdx]
					- prQM->au4CurrentTcResource[u4TcIdx]);
				ai4TcResDemand[u4TcIdx] = 0;
			}

			/* The demand can not be fulfilled with
			 * the guaranteed resource amount
			 */
			else {
				ai4TcResDemand[u4TcIdx] -=
					(prQM->au4GuaranteedTcResource[u4TcIdx]
					- prQM->au4CurrentTcResource[u4TcIdx]);

				prQM->au4CurrentTcResource[u4TcIdx] =
					prQM->au4GuaranteedTcResource[u4TcIdx];
				u4ShareCount++;
			}
		}

		/* 4 <3.2> Allocate the residual resource */
		qmAllocateResidualTcResource(prAdapter, ai4TcResDemand,
			&u4ResidualResource, &u4ShareCount);
	}

	prQM->fgTcResourcePostAnnealing = TRUE;

#if QM_PRINT_TC_RESOURCE_CTRL
	/* Debug print */
	DBGLOG(QM, INFO,
		"QM: TC Rsc adjust to [%03u:%03u:%03u:%03u:%03u:%03u]\n",
		prQM->au4CurrentTcResource[0],
		prQM->au4CurrentTcResource[1],
		prQM->au4CurrentTcResource[2],
		prQM->au4CurrentTcResource[3],
		prQM->au4CurrentTcResource[4],
		prQM->au4CurrentTcResource[5]);
#endif
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	DBGLOG(QM, INFO,
	"QM: TC[6-10] Rsc adjust to [%03u:%03u:%03u:%03u:%03u]\n",
	prQM->au4CurrentTcResource[6], prQM->au4CurrentTcResource[7],
	prQM->au4CurrentTcResource[8], prQM->au4CurrentTcResource[9],
	prQM->au4CurrentTcResource[10]);

	DBGLOG(QM, INFO,
	"QM: TC[11-13] Rsc adjust to [%03u:%03u:%03u]\n",
	prQM->au4CurrentTcResource[11], prQM->au4CurrentTcResource[12],
	prQM->au4CurrentTcResource[13]);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Adjust TX resource for each TC according to TX queue length and
 *        current assignment
 *
 * \param (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmDoAdaptiveTcResourceCtrl(struct ADAPTER	*prAdapter)
{
	struct QUE_MGT *prQM = &prAdapter->rQM;

	if (prQM->fgForceReassign) {
		prQM->u4TimeToUpdateQueLen = 1;
		prQM->u4TimeToAdjustTcResource = 1;
		prQM->fgTcResourceFastReaction = TRUE;

		prQM->fgForceReassign = FALSE;
	}

	/* 4 <0> Check to update queue length or not */
	if (--prQM->u4TimeToUpdateQueLen)
		return;
	/* 4 <1> Update TC queue length */
	prQM->u4TimeToUpdateQueLen = QM_INIT_TIME_TO_UPDATE_QUE_LEN;
	qmUpdateAverageTxQueLen(prAdapter);

	/* 4 <2> Adjust TC resource assignment */
	/* Check whether it is time to adjust the TC resource assignment */
	if (--prQM->u4TimeToAdjustTcResource == 0) {
		/* The last assignment has not been completely applied */
		if (prQM->fgTcResourcePostAnnealing) {
			/* Upon the next qmUpdateAverageTxQueLen function call,
			 * do this check again
			 */
			prQM->u4TimeToAdjustTcResource = 1;
		}

		/* The last assignment has been applied */
		else {
			prQM->u4TimeToAdjustTcResource =
				QM_INIT_TIME_TO_ADJUST_TC_RSC;
			qmReassignTcResource(prAdapter);
#if QM_FAST_TC_RESOURCE_CTRL
			if (prQM->fgTcResourceFastReaction) {
				prQM->fgTcResourceFastReaction = FALSE;
				nicTxAdjustTcq(prAdapter);
			}
#endif
		}
	}

	/* Debug */
#if QM_PRINT_TC_RESOURCE_CTRL
	do {
		uint32_t u4Tc;

		for (u4Tc = 0; u4Tc < QM_ACTIVE_TC_NUM; u4Tc++) {
			if (QM_GET_TX_QUEUE_LEN(prAdapter, u4Tc) >= 100) {
				log_dbg(QM, LOUD, "QM: QueLen [%ld %ld %ld %ld %ld %ld]\n",
					QM_GET_TX_QUEUE_LEN(prAdapter, 0),
					QM_GET_TX_QUEUE_LEN(prAdapter, 1),
					QM_GET_TX_QUEUE_LEN(prAdapter, 2),
					QM_GET_TX_QUEUE_LEN(prAdapter, 3),
					QM_GET_TX_QUEUE_LEN(prAdapter, 4),
					QM_GET_TX_QUEUE_LEN(prAdapter, 5));
				break;
			}
		}
	} while (FALSE);
#endif

}

#if QM_FAST_TC_RESOURCE_CTRL
void qmCheckForFastTcResourceCtrl(struct ADAPTER *prAdapter,
	uint8_t ucTc)
{
	struct QUE_MGT *prQM = &prAdapter->rQM;
	u_int8_t fgTrigger = FALSE;

	if (!prAdapter->rTxCtrl.rTc.au4FreeBufferCount[ucTc]
	|| ((prAdapter->rTxCtrl.rTc.fgNeedPleCtrl) &&
	(!prAdapter->rTxCtrl.rTc.au4FreeBufferCount_PLE[ucTc]))) {
		if (!prQM->au4CurrentTcResource[ucTc] ||
			nicTxGetAdjustableResourceCnt(prAdapter))
			fgTrigger = TRUE;
	}

	/* Trigger TC resource adjustment
	 * if there is a requirement coming for a empty TC
	 */
	if (fgTrigger) {
		prQM->fgForceReassign = TRUE;

		DBGLOG(QM, LOUD,
			"Trigger TC Resource adjustment for TC[%u]\n", ucTc);
	}
}
#endif

#endif

uint32_t gmGetDequeueQuota(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prBssInfo,
	uint32_t			u4TotalQuota
)
{
	uint32_t	u4Weight = 100;
	uint32_t	u4Quota;

	struct QUE_MGT *prQM = &prAdapter->rQM;

	/* Only apply to SDIO, PCIE/AXI use HW DMA */
	if ((prAdapter->rWifiVar.uDeQuePercentEnable == FALSE) ||
		(prQM->fgIsTxResrouceControlEn == FALSE))
		return u4TotalQuota;

	if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_VHT) {
		if (prBssInfo->ucVhtChannelWidth >
			VHT_OP_CHANNEL_WIDTH_20_40) {
			/* BW80 NSS1 rate: MCS9 433 Mbps */
			u4Weight = prAdapter->rWifiVar.u4DeQuePercentVHT80Nss1;
		} else if (prBssInfo->eBssSCO != CHNL_EXT_SCN) {
			/* BW40 NSS1 Max rate: 200 Mbps */
			u4Weight = prAdapter->rWifiVar.u4DeQuePercentVHT40Nss1;
		} else {
			/* BW20 NSS1 Max rate: 72.2Mbps (MCS8 86.7Mbps) */
			u4Weight = prAdapter->rWifiVar.u4DeQuePercentVHT20Nss1;
		}
	} else if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_HT) {
		if (prBssInfo->ucHtOpInfo1 & HT_OP_INFO1_STA_CHNL_WIDTH) {
			/* BW40 NSS1 Max rate: 150 Mbps (MCS9 200Mbps)*/
			u4Weight = prAdapter->rWifiVar.u4DeQuePercentHT40Nss1;
		} else {
			/* BW20 NSS1 Max rate: 72.2Mbps (MCS8 86.7Mbps)*/
			u4Weight = prAdapter->rWifiVar.u4DeQuePercentHT20Nss1;
		}
	}
#if (CFG_SUPPORT_802_11AX == 1)
	else if (fgEfuseCtrlAxOn == 1) {
		if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_HE)
		;/* TBD */
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	else if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_EHT)
		;/* TBD */
#endif

	u4Quota = u4TotalQuota * u4Weight / 100;

	if (u4Quota > u4TotalQuota || u4Quota <= 0)
		return u4TotalQuota;

	return u4Quota;
}

/*----------------------------------------------------------------------------*/
/* RX-Related Queue Management                                                */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*!
 * \brief Init Queue Management for RX
 *
 * \param[in] (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmInitRxQueues(struct ADAPTER *prAdapter)
{
	/* DbgPrint("QM: Enter qmInitRxQueues()\n"); */
	/* TODO */
}

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
u_int8_t qmHandleRroPkt(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	u_int8_t fgDrop = FALSE;
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;

	if (unlikely(prSwRfb->u4IndReason >= RRO_COUNTER_NUM ||
		prSwRfb->u4IndReason == RRO_REPEAT ||
		prSwRfb->u4IndReason == RRO_OLDPKT)) {
		/* no drop for independent pkt or low latency mode */
		if (!qmIsNoDropPacket(prAdapter, prSwRfb))
			fgDrop = TRUE;
	}

	if (prSwRfb->u4IndReason < RRO_COUNTER_NUM) {
		/* increase rro counter for normal reason */
		RX_RRO_INC_CNT(prRxCtrl, prSwRfb->u4IndReason);
	} else {
		/* RRO_COUNTER_NUM used for abnormal reason */
		RX_RRO_INC_CNT(prRxCtrl, RRO_COUNTER_NUM);
		DBGLOG(QM, WARN, "Invalid RRO reason: %d\n",
			prSwRfb->u4IndReason);
	}

	return fgDrop;
}
#endif /* CFG_SUPPORT_HOST_OFFLOAD */

static void processNanBmcRx(struct ADAPTER *prAdapter,
		struct SW_RFB *prCurrSwRfb,
		struct WLAN_MAC_HEADER *prWlanHeader, uint16_t u2FrameCtrl)
{
#if (CFG_SUPPORT_NAN == 1)
	struct BSS_INFO *prBssInfo;
	uint16_t u2MACLen = 0;
	uint8_t ucBssIndex;

	if (!prCurrSwRfb->prStaRec)
		prCurrSwRfb->prStaRec =
			nanGetStaRecByNDI(prAdapter, prWlanHeader->aucAddr2);

	if (!prCurrSwRfb->prStaRec)
		return;

	ucBssIndex = prCurrSwRfb->prStaRec->ucBssIndex;
	if (ucBssIndex >= ARRAY_SIZE(prAdapter->aprBssInfo))
		return;

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	if (prBssInfo->eNetworkType != NETWORK_TYPE_NAN)
		return;

	DBGLOG(QM, INFO, "NAN special case for BMC packet\n");
	if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl))
		u2MACLen = sizeof(struct WLAN_MAC_HEADER_QOS);
	else
		u2MACLen = sizeof(struct WLAN_MAC_HEADER);

	u2MACLen += ETH_LLC_LEN + ETH_SNAP_OUI_LEN;
	u2MACLen -= ETHER_TYPE_LEN_OFFSET;
	prCurrSwRfb->pvHeader += u2MACLen;
	kalMemCopy(prCurrSwRfb->pvHeader, prWlanHeader->aucAddr1, MAC_ADDR_LEN);
	kalMemCopy(prCurrSwRfb->pvHeader + MAC_ADDR_LEN, prWlanHeader->aucAddr2,
					MAC_ADDR_LEN);
	prCurrSwRfb->u2PacketLen -= u2MACLen;
	/* record StaRec related info */
	prCurrSwRfb->ucStaRecIdx = prCurrSwRfb->prStaRec->ucIndex;
	prCurrSwRfb->ucWlanIdx = prCurrSwRfb->prStaRec->ucWlanIndex;
	GLUE_SET_PKT_BSS_IDX(prCurrSwRfb->pvPacket,
		secGetBssIdxByWlanIdx(prAdapter, prCurrSwRfb->ucWlanIdx));
#endif
}

/**
 * When prCurrSwRfb->prStaRec == NULL, prBssInfo exist and start is connected,
 * fill the information from prBssInfo if address matched.
 * Update these information:
 *   prCurrSwRfb->pvHeader and the MAC address at where it points to
 *   prCurrSwRfb->u2PacketLen
 *   prCurrSwRfb->prStaRec
 *   prCurrSwRfb->ucStaRecIdx
 *   prCurrSwRfb->ucWlanIdx
 */
static void fillRxNullStaRec(struct ADAPTER *prAdapter,
		struct SW_RFB *prCurrSwRfb, uint16_t u2FrameCtrl,
		struct BSS_INFO *prBssInfo,
		struct WLAN_MAC_HEADER *prWlanHeader)
{
	if (!RXM_IS_DATA_FRAME(u2FrameCtrl))
		return;

	if (!prBssInfo || prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
		return;

	/* rx header translation */
	DBGLOG(QM, WARN,
		"RXD Trans: FrameCtrl=0x%02x GVLD=0x%x, StaRecIdx=%u, WlanIdx=%u PktLen=%u\n",
		u2FrameCtrl, prCurrSwRfb->ucGroupVLD, prCurrSwRfb->ucStaRecIdx,
		prCurrSwRfb->ucWlanIdx, prCurrSwRfb->u2PacketLen);

	DBGLOG_MEM8(QM, LOUD, prCurrSwRfb->pvHeader,
			prCurrSwRfb->u2PacketLen < 32 ?
			prCurrSwRfb->u2PacketLen : 32);

	if (!prBssInfo->prStaRecOfAP)
		return;

	if (EQUAL_MAC_ADDR(prWlanHeader->aucAddr1, prBssInfo->aucOwnMacAddr) &&
	    EQUAL_MAC_ADDR(prWlanHeader->aucAddr2, prBssInfo->aucBSSID)) {
		uint16_t u2MACLen;

		/* QoS data, VHT */
		if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl))
			u2MACLen = sizeof(struct WLAN_MAC_HEADER_QOS);
		else
			u2MACLen = sizeof(struct WLAN_MAC_HEADER);
		u2MACLen += ETH_LLC_LEN + ETH_SNAP_OUI_LEN;
		u2MACLen -= ETHER_TYPE_LEN_OFFSET;
		prCurrSwRfb->pvHeader =
			(uint8_t *)prCurrSwRfb->pvHeader + u2MACLen;
		prCurrSwRfb->u2PacketLen -= u2MACLen;
		COPY_MAC_ADDR(prCurrSwRfb->pvHeader, prWlanHeader->aucAddr1);
		COPY_MAC_ADDR((uint8_t *)prCurrSwRfb->pvHeader + MAC_ADDR_LEN,
				prWlanHeader->aucAddr2);

		/* record StaRec related info */
		prCurrSwRfb->prStaRec = prBssInfo->prStaRecOfAP;
		prCurrSwRfb->ucStaRecIdx = prCurrSwRfb->prStaRec->ucIndex;
		prCurrSwRfb->ucWlanIdx = prCurrSwRfb->prStaRec->ucWlanIndex;
		GLUE_SET_PKT_BSS_IDX(prCurrSwRfb->pvPacket,
				secGetBssIdxByWlanIdx(prAdapter,
					prCurrSwRfb->ucWlanIdx));
		DBGLOG_MEM8(QM, WARN, prCurrSwRfb->pvHeader,
				prCurrSwRfb->u2PacketLen < 64 ?
				prCurrSwRfb->u2PacketLen : 64);
	}
}

static void processAPPktDst(struct ADAPTER *prAdapter,
	uint8_t *pucEthDestAddr, struct SW_RFB *prCurrSwRfb)
{
	uint8_t ucStaRecIdx = STA_REC_INDEX_NOT_FOUND;
	struct STA_RECORD *prStaRec = NULL;

	if (IS_BMCAST_MAC_ADDR(pucEthDestAddr)) {
		prCurrSwRfb->eDst = RX_PKT_DESTINATION_HOST_WITH_FORWARD;
		return;
	}

	ucStaRecIdx = secLookupStaRecIndexFromTA(prAdapter, pucEthDestAddr);
	if (ucStaRecIdx == STA_REC_INDEX_NOT_FOUND)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);

	if ((prStaRec != NULL) &&
	    (prStaRec->ucBssIndex == prCurrSwRfb->prStaRec->ucBssIndex))
		prCurrSwRfb->eDst = RX_PKT_DESTINATION_FORWARD;
}

#if CFG_RX_REORDERING_ENABLED
u_int8_t qmTkipWoMicValidation(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	u_int8_t fgMicErr = TRUE;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;
	uint8_t *pucMicKey;

	/* bypass tkip frag */
	if (prSwRfb->fgFragFrame ||
		prSwRfb->ucSecMode != CIPHER_SUITE_TKIP_WO_MIC) {
		fgMicErr = FALSE;
		goto end;
	}

	if (!prSwRfb->prStaRec)
		goto end;

	ucBssIndex = prSwRfb->prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		goto end;

	if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE
		|| !IS_BSS_AIS(prBssInfo)) {
		DBGLOG_LIMITED(QM, ERROR,
			"BSS[%u] OPMode[%u] Type[%u] not support TKIP WO MIC",
			ucBssIndex, prBssInfo->eCurrentOPMode,
			prBssInfo->eNetworkType);
		goto end;
	}

	pucMicKey = &(aisGetAisSpecBssInfo(prAdapter,
				ucBssIndex)->aucRxMicKey[0]);
	/* SW TKIP MIC verify */
	if (tkipMicDecapsulateInRxHdrTransMode(prAdapter,
		prSwRfb, pucMicKey) == TRUE)
		fgMicErr = FALSE;

end:
	return fgMicErr;
}

void qmProcessPktWithoutReordering(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct QUE *prReturnedQue)
{
	void *prRxStatus;
	u_int8_t fgIsBMC = (prSwRfb->fgIsBC | prSwRfb->fgIsMC);

	if (!fgIsBMC && nicRxIsDuplicateFrame(prSwRfb) == TRUE) {
		DBGLOG(RX, TEMP, "Duplicated packet is detected\n");
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_DUPICATE_DROP_COUNT);
		LINK_QUALITY_COUNT_DUP(prAdapter, prSwRfb);
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
	}

	if (prSwRfb->fgFragFrame) {
		prSwRfb = nicRxDefragMPDU(prAdapter, prSwRfb, prReturnedQue);
		if (prSwRfb) {
			prRxStatus = prSwRfb->prRxStatus;
			DBGLOG(RX, TEMP,
				"defragmentation RxStatus=%p\n", prRxStatus);
		}
	}

	if (!prSwRfb)
		return;

	if (qmTkipWoMicValidation(prAdapter, prSwRfb)) {
		DBGLOG(RX, ERROR, "Mark NULL for TKIP Mic Error\n");
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_MIC_ERROR_DROP_COUNT);
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
	}

	QUEUE_INSERT_TAIL(prReturnedQue, prSwRfb);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle RX packets (buffer reordering)
 *
 * \param[in] prSwRfbListHead The list of RX packets
 *
 * \return The list of packets which are not buffered for reordering
 */
/*----------------------------------------------------------------------------*/
struct SW_RFB *qmHandleRxPackets(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfbListHead)
{

#if CFG_RX_REORDERING_ENABLED
	struct SW_RFB *prCurrSwRfb;
	struct SW_RFB *prNextSwRfb;
	void *prRxStatus;
	struct QUE rReturnedQue;
	struct QUE *prReturnedQue;
	uint8_t *pucEthDestAddr;
	u_int8_t fgIsBMC, fgIsHTran;
#if CFG_SUPPORT_REPLAY_DETECTION
	u_int8_t ucBssIndexRly = 0;
	struct BSS_INFO *prBssInfoRly = NULL;
#endif
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	u_int8_t fgIsHwRROSupport =
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgEnableRro);
	/* SwRRO must be disabled when HwRRO is enabled */
	u_int8_t fgSwRxReordering =
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgEnableRro) ? FALSE :
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgSwRxReordering);
#else /* CFG_SUPPORT_HOST_OFFLOAD == 1 */
	u_int8_t fgSwRxReordering =
		IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgSwRxReordering);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

#if CFG_SUPPORT_WED_PROXY
	/* SwRRO must be disabled when wed attached (wed RRO is enabled) */
	if (IsWedAttached())
		fgSwRxReordering = FALSE;
#endif

	ASSERT(prSwRfbListHead);

	prReturnedQue = &rReturnedQue;

	QUEUE_INITIALIZE(prReturnedQue);
	prNextSwRfb = prSwRfbListHead;

	do {
		prCurrSwRfb = prNextSwRfb;
		prNextSwRfb = QM_RX_GET_NEXT_SW_RFB(prCurrSwRfb);

		prRxStatus = prCurrSwRfb->prRxStatus;
		if (prCurrSwRfb->u2RxByteCount > CFG_RX_MAX_PKT_SIZE) {
			prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
			QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
			DBGLOG(QM, ERROR,
				"Drop packet when packet length is larger than CFG_RX_MAX_PKT_SIZE. Packet length=%d\n",
				prCurrSwRfb->u2RxByteCount);
			continue;
		}
		/* TODO: (Tehuang) Check if relaying */
		prCurrSwRfb->eDst = RX_PKT_DESTINATION_HOST;

		fgIsBMC = (prCurrSwRfb->fgIsBC | prCurrSwRfb->fgIsMC);
		fgIsHTran = FALSE;

		if (prAdapter->rWifiVar.u4BaVerboseLogging && fgIsBMC)
			DBGLOG(QM, INFO,
				"Rx BMC [Sta,Tid,SN,len]:%u,%u,%u,%u\n",
				prCurrSwRfb->ucStaRecIdx,
				prCurrSwRfb->ucTid,
				HAL_RX_STATUS_GET_SEQFrag_NUM(
					prCurrSwRfb->prRxStatusGroup4)
						>> RX_STATUS_SEQ_NUM_OFFSET,
				prCurrSwRfb->u2PacketLen);

		if (prCurrSwRfb->fgHdrTran) {
			uint8_t ucBssIndex;
			struct BSS_INFO *prBssInfo;
			uint8_t aucTaAddr[MAC_ADDR_LEN];

			fgIsHTran = TRUE;
			pucEthDestAddr = prCurrSwRfb->pvHeader;
			if (prCurrSwRfb->prRxStatusGroup4 == NULL) {
				DBGLOG(QM, ERROR,
					"H/W did Header Trans but prRxStatusGroup4 is NULL !!!\n");
				DBGLOG_MEM8(QM, ERROR, prCurrSwRfb->pucRecvBuff,
					prCurrSwRfb->u2RxByteCount);
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				DBGLOG(RX, WARN,
				       "rxStatusGroup4 for data packet is NULL, drop this packet, and dump RXD and Packet\n");
				/* prRxStatus is claimed as void *
				 * *prRxStatus deference is void
				 */
				DBGLOG_MEM8(RX, WARN, (uint8_t *) prRxStatus,
					prAdapter->chip_info->rxd_size);
				if (prCurrSwRfb->pvHeader)
					DBGLOG_MEM8(RX, WARN,
						prCurrSwRfb->pvHeader,
						prCurrSwRfb->u2PacketLen >
						 32 ? 32 :
						prCurrSwRfb->u2PacketLen);
#if 0
				glSetRstReason(RST_GROUP4_NULL);
				GL_RESET_TRIGGER(prAdapter,
					RST_FLAG_DO_CORE_DUMP);
#endif
				continue;
			}

			if (prCurrSwRfb->prStaRec == NULL) {
				/* Workaround WTBL Issue */
				HAL_RX_STATUS_GET_TA(
					prCurrSwRfb->prRxStatusGroup4,
					aucTaAddr);
				prCurrSwRfb->ucStaRecIdx =
					secLookupStaRecIndexFromTA(
					prAdapter, aucTaAddr);
				if (prCurrSwRfb->ucStaRecIdx <
					CFG_STA_REC_NUM) {
					prCurrSwRfb->prStaRec =
						cnmGetStaRecByIndex(prAdapter,
							prCurrSwRfb->
							ucStaRecIdx);
#define __STR_FMT__ \
	"Re-search the staRec = %d, mac = " MACSTR ", byteCnt= %d\n"
					log_dbg(QM, TRACE,
						__STR_FMT__,
						prCurrSwRfb->ucStaRecIdx,
						MAC2STR(aucTaAddr),
						prCurrSwRfb->u2RxByteCount);
#undef __STR_FMT__
				}

				if (prCurrSwRfb->prStaRec == NULL) {
					DBGLOG(RX, WARN,
						"Mark NULL the Packet for no STA_REC, wlanIdx=%d\n",
						prCurrSwRfb->ucWlanIdx);
					RX_INC_CNT(&prAdapter->rRxCtrl,
						RX_NO_STA_DROP_COUNT);
					prCurrSwRfb->eDst =
						RX_PKT_DESTINATION_NULL;
					QUEUE_INSERT_TAIL(prReturnedQue,
						prCurrSwRfb);
					NIC_DUMP_ICV_RXD(prAdapter, prRxStatus);
					NIC_DUMP_ICV_RXP(prCurrSwRfb->pvHeader,
						prCurrSwRfb->u2PacketLen);

					continue;
				}

				prCurrSwRfb->ucWlanIdx =
					prCurrSwRfb->prStaRec->ucWlanIndex;
				GLUE_SET_PKT_BSS_IDX(prCurrSwRfb->pvPacket,
					prCurrSwRfb->prStaRec->ucBssIndex);
			}

			ucBssIndex = prCurrSwRfb->prStaRec->ucBssIndex;
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
				ucBssIndex);

			if (!prBssInfo || !IS_BSS_ACTIVE(prBssInfo)) {
				log_dbg(RX, TEMP, "Mark NULL the Packet for inactive Bss %u\n",
					ucBssIndex);
				RX_INC_CNT(&prAdapter->rRxCtrl,
					RX_INACTIVE_BSS_DROP_COUNT);
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				continue;
			}

			if (prBssInfo->eCurrentOPMode ==
				OP_MODE_ACCESS_POINT) {
				processAPPktDst(prAdapter,
					pucEthDestAddr, prCurrSwRfb);
			}
#if CFG_SUPPORT_PASSPOINT
			else if (IS_BSS_AIS(prBssInfo) &&
				hs20IsFrameFilterEnabled(prAdapter,
				prBssInfo) &&
				hs20IsUnsecuredFrame(prAdapter,
				prBssInfo,
				prCurrSwRfb)) {
				DBGLOG(QM, WARN,
					"Mark NULL the Packet for Dropped Packet %u\n",
					ucBssIndex);
				RX_INC_CNT(&prAdapter->rRxCtrl,
						RX_HS20_DROP_COUNT);
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				continue;
			}
#endif /* CFG_SUPPORT_PASSPOINT */
		} else {
			struct BSS_INFO *prBssInfo;
			struct WLAN_MAC_HEADER *prWlanHeader;
			uint16_t u2FrameCtrl;
			uint8_t ucBssIndex;

			ucBssIndex = secGetBssIdxByWlanIdx(prAdapter,
					prCurrSwRfb->ucWlanIdx);
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					ucBssIndex);
			prWlanHeader = prCurrSwRfb->pvHeader;
			u2FrameCtrl = prWlanHeader->u2FrameCtrl;
			prCurrSwRfb->u2SequenceControl =
				prWlanHeader->u2SeqCtrl;

			if (fgIsBMC)
				processNanBmcRx(prAdapter, prCurrSwRfb,
					prWlanHeader, u2FrameCtrl);

			if (!prCurrSwRfb->prStaRec)
				fillRxNullStaRec(prAdapter, prCurrSwRfb,
						u2FrameCtrl, prBssInfo,
						prWlanHeader);
		}

		/*
		 * Independent pkt is marked in stats,
		 * so it should be placed before rx reordering
		 */
		StatsRxPktInfoDisplay(prCurrSwRfb);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
		if (likely(fgIsHwRROSupport)) {
			if (qmHandleRroPkt(prAdapter, prCurrSwRfb)) {
				prCurrSwRfb->eDst =
					RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				continue;
			}
		}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
		if (prCurrSwRfb->fgDataFrame && prCurrSwRfb->prStaRec &&
			qmAmsduValidation(prAdapter, prCurrSwRfb)) {
			prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
			DBGLOG(QM, INFO, "drop Abnormal AMSDU packet\n");
		}

		if (prCurrSwRfb->fgDataFrame && prCurrSwRfb->prStaRec &&
			qmDetectRxInvalidEAPOL(prAdapter, prCurrSwRfb)) {
			prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
			DBGLOG(QM, INFO,
				"drop EAPOL packet not in sec mode\n");
		}
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

#if CFG_SUPPORT_WAPI
		if (prCurrSwRfb->u2PacketLen > ETHER_HEADER_LEN) {
			uint8_t *pc = (uint8_t *) prCurrSwRfb->pvHeader;
			uint16_t u2Etype = 0;

			u2Etype = (pc[ETHER_TYPE_LEN_OFFSET] << 8) |
				  (pc[ETHER_TYPE_LEN_OFFSET + 1]);
			/* for wapi integrity test. WPI_1x packet should be
			 * always in non-encrypted mode. if we received any
			 * WPI(0x88b4) packet that is encrypted, drop here.
			 */
			if (u2Etype == ETH_WPI_1X &&
			    prCurrSwRfb->ucSecMode != 0 &&
			    prCurrSwRfb->fgIsCipherMS == 0) {
				DBGLOG(QM, INFO,
					"drop wpi packet with sec mode\n");
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				continue;
			}
		}
#endif

		/* Todo:: Move the data class error check here */

#if CFG_SUPPORT_REPLAY_DETECTION
		if (prCurrSwRfb->prStaRec) {
			ucBssIndexRly = prCurrSwRfb->prStaRec->ucBssIndex;
			prBssInfoRly = GET_BSS_INFO_BY_INDEX(prAdapter,
				ucBssIndexRly);
			if (prBssInfoRly && !IS_BSS_ACTIVE(prBssInfoRly)) {
				DBGLOG(QM, INFO,
					"Mark NULL the Packet for inactive Bss %u\n",
					ucBssIndexRly);
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				continue;
			}
		}
		if (fgIsBMC && prBssInfoRly && IS_BSS_AIS(prBssInfoRly) &&
			qmHandleRxReplay(prAdapter, prCurrSwRfb)) {
			prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
			QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
			DBGLOG(RX, TEMP, "BMC replay\n");
			continue;
		}
#endif
		if (prAdapter->rWifiVar.u4BaVerboseLogging && fgIsBMC)
			DBGLOG(QM, INFO,
				"Checked BMC [Sta,Tid,SN,len,fgDataFrame,StaRec,sec]:%u,%u,%u,%u,%u,%p,%u\n",
				prCurrSwRfb->ucStaRecIdx,
				prCurrSwRfb->ucTid,
				HAL_RX_STATUS_GET_SEQFrag_NUM(
					prCurrSwRfb->prRxStatusGroup4)
						>> RX_STATUS_SEQ_NUM_OFFSET,
				prCurrSwRfb->u2PacketLen,
				prCurrSwRfb->fgDataFrame,
				prCurrSwRfb->prStaRec,
				prCurrSwRfb->prStaRec ?
				secCheckClassError(prAdapter, prCurrSwRfb,
					prCurrSwRfb->prStaRec) : 0);

		if (fgSwRxReordering &&
		    prCurrSwRfb->fgReorderBuffer && !fgIsBMC && fgIsHTran) {
			/* If this packet should dropped or indicated to the
			 * host immediately, it should be enqueued into the
			 * rReturnedQue with specific flags. If this packet
			 * should be buffered for reordering, it should be
			 * enqueued into the reordering queue in the STA_REC
			 * rather than into the rReturnedQue.
			 */
			if (prCurrSwRfb->ucTid >= CFG_RX_MAX_BA_TID_NUM) {
				log_dbg(QM, ERROR,
					"TID from RXD = %d, out of range !!!\n",
					prCurrSwRfb->ucTid);
				DBGLOG_MEM8(QM, ERROR,
					prCurrSwRfb->pucRecvBuff,
					prCurrSwRfb->u2RxByteCount);
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
			} else
				qmProcessPktWithReordering(prAdapter,
					prCurrSwRfb, prReturnedQue);

		} else if (prCurrSwRfb->fgDataFrame) {
			/* Check Class Error */
			if (prCurrSwRfb->prStaRec &&
				(secCheckClassError(prAdapter, prCurrSwRfb,
				prCurrSwRfb->prStaRec) == TRUE)) {
				struct RX_BA_ENTRY *prReorderQueParm = NULL;

				if ((prCurrSwRfb->ucTid < CFG_RX_MAX_BA_TID_NUM)
					&& !fgIsBMC && fgIsHTran &&
					(HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
					prCurrSwRfb->prRxStatusGroup4) &
					MASK_FRAME_TYPE) != MAC_FRAME_DATA) {
					prReorderQueParm =
						((prCurrSwRfb->prStaRec->
						aprRxReorderParamRefTbl)[
						prCurrSwRfb->ucTid]);
				}

				if (fgSwRxReordering && prReorderQueParm &&
					prReorderQueParm->fgIsValid) {
					/* Only QoS Data frame with BA aggrement
					 * shall enter reordering buffer
					 */
					qmProcessPktWithReordering(prAdapter,
						prCurrSwRfb,
						prReturnedQue);
				} else
					qmProcessPktWithoutReordering(prAdapter,
						prCurrSwRfb,
						prReturnedQue);
			} else {
				DBGLOG(RX, TEMP,
					"Mark NULL the Packet for class error\n");
				RX_INC_CNT(&prAdapter->rRxCtrl,
					RX_CLASS_ERR_DROP_COUNT);
				prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
			}
		} else {
			struct WLAN_MAC_HEADER *prWlanMacHeader;

			ASSERT(prCurrSwRfb->pvHeader);

			prWlanMacHeader = (struct WLAN_MAC_HEADER *)
				prCurrSwRfb->pvHeader;
			prCurrSwRfb->eDst = RX_PKT_DESTINATION_NULL;

			switch (prWlanMacHeader->u2FrameCtrl &
				MASK_FRAME_TYPE) {
			/* BAR frame */
			case MAC_FRAME_BLOCK_ACK_REQ:
				qmProcessBarFrame(prAdapter,
					prCurrSwRfb, prReturnedQue);
				RX_INC_CNT(&prAdapter->rRxCtrl,
					RX_BAR_DROP_COUNT);
				break;
			default:
				DBGLOG(RX, WARN,
					"Mark NULL the Packet for non-interesting type\n");
				RX_INC_CNT(&prAdapter->rRxCtrl,
					RX_NO_INTEREST_DROP_COUNT);
				QUEUE_INSERT_TAIL(prReturnedQue, prCurrSwRfb);
				NIC_DUMP_ICV_RXD(prAdapter, prRxStatus);
				NIC_DUMP_ICV_RXP(prCurrSwRfb->pvHeader,
						prCurrSwRfb->u2PacketLen);
				break;
			}
		}

	} while (prNextSwRfb);

	/* The returned list of SW_RFBs must end with a NULL pointer */
	if (QUEUE_IS_NOT_EMPTY(prReturnedQue))
		QUEUE_ENTRY_SET_NEXT(QUEUE_GET_TAIL(prReturnedQue), NULL);

	return QUEUE_GET_HEAD(prReturnedQue);

#else
	StatsRxPktInfoDisplay(prSwRfbListHead);

	/* DbgPrint("QM: Enter qmHandleRxPackets()\n"); */
	return prSwRfbListHead;

#endif

}

#if CFG_SUPPORT_FRAG_AGG_VALIDATION
/*----------------------------------------------------------------------------*/
/*!
 * \brief qmDetectRxInvalidEAPOL() is used for fake EAPOL checking.
 *
 * \param[in] prSwRfb        The RFB which is being processed.
 *
 * \return TRUE when we need to drop it
 */
/*----------------------------------------------------------------------------*/
u_int8_t qmDetectRxInvalidEAPOL(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	uint8_t *pucPkt = NULL;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EtherType = 0;
	u_int8_t fgDrop = FALSE;
	uint16_t u2SeqCtrl, u2FrameCtrl;
	uint8_t ucFragNo;

	ASSERT(prSwRfb);
	ASSERT(prSwRfb->prStaRec);

	/* return FALSE if no Header Translation*/
	if (prSwRfb->fgHdrTran == FALSE)
		return FALSE;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return FALSE;

	pucPkt = prSwRfb->pvHeader;
	if (!pucPkt)
		return FALSE;

	ucBssIndex = prSwRfb->prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	/* return FALSE if OP_MODE is not SAP */
	if (!prBssInfo || !IS_BSS_ACTIVE(prBssInfo)
		|| prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT)
		return FALSE;

	/* return FALSE for this frame is a mid/last fragment*/
	u2FrameCtrl = HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
			prSwRfb->prRxStatusGroup4);
	u2SeqCtrl = HAL_RX_STATUS_GET_SEQFrag_NUM(prSwRfb->prRxStatusGroup4);
	ucFragNo = (uint8_t) (u2SeqCtrl & MASK_SC_FRAG_NUM);
	if (prSwRfb->fgFragFrame && ucFragNo != 0)
		return FALSE;

	u2EtherType = (pucPkt[ETH_TYPE_LEN_OFFSET] << 8)
			| (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);

	/* return FALSE if EtherType is not EAPOL */
	if (u2EtherType != ETH_P_1X)
		return FALSE;

	if ((prSwRfb->eDst
			== RX_PKT_DESTINATION_HOST_WITH_FORWARD
		    || prSwRfb->eDst == RX_PKT_DESTINATION_FORWARD)) {
		/* fgIsTxKeyReady is set by nicEventAddPkeyDone */
		if (prSwRfb->prStaRec->fgIsTxKeyReady != TRUE)
			fgDrop = TRUE;
	}

	DBGLOG(QM, TRACE,
		"QM: eDst:%d TxKeyReady:%d fgDrop:%d",
		prSwRfb->eDst, prSwRfb->prStaRec->fgIsTxKeyReady,
		fgDrop);

	return fgDrop;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief AMSDU Validation
 *
 * \param[in] prSwRfb The RX packet to process
 *
 * \return TRUE when we find an abnormal amsdu
 */
/*----------------------------------------------------------------------------*/
u_int8_t qmAmsduValidation(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	u_int8_t fgDrop = FALSE;
	uint8_t aucTaAddr[MAC_ADDR_LEN];
	uint8_t *pucTaAddr = NULL, *pucRaAddr = NULL;
	uint8_t *pucSaAddr = NULL, *pucDaAddr = NULL;
	uint8_t *pucAmsduAddr = NULL, *pucCmpAddr = NULL;
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint16_t u2FrameCtrl, u2SSN;
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	uint8_t ucTid;
	uint8_t *pucPaylod = NULL;

	ASSERT(prSwRfb);

	prStaRec = prSwRfb->prStaRec;
	ASSERT(prStaRec);

	/* fill in nicRxFillSSN */
	u2SSN = prSwRfb->u2SSN;

	/* 802.11 header TA */
	if (prSwRfb->fgHdrTran) {
		u2FrameCtrl = HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
				prSwRfb->prRxStatusGroup4);
		HAL_RX_STATUS_GET_TA(prSwRfb->prRxStatusGroup4, aucTaAddr);
		pucTaAddr = &aucTaAddr[0];
		pucPaylod = prSwRfb->pvHeader;
	} else {
		prWlanHeader = (struct WLAN_MAC_HEADER *) prSwRfb->pvHeader;
		u2FrameCtrl = prWlanHeader->u2FrameCtrl;
		pucTaAddr = prWlanHeader->aucAddr2;
		pucPaylod = (uint8_t *)(
			(uintptr_t)prSwRfb->pvHeader + prSwRfb->u2HeaderLen);
	}

	/* 802.11 header RA */
	ucBssIndex = prSwRfb->prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(QM, ERROR, "prBssInfo NULL for BssIndex:%u\n",
				ucBssIndex);
		return FALSE;
	}

	pucRaAddr = &prBssInfo->aucOwnMacAddr[0];

	/* DA and SA */
	pucDaAddr = pucPaylod;
	pucSaAddr = pucPaylod + MAC_ADDR_LEN;

	if (RXM_IS_QOS_DATA_FRAME(u2FrameCtrl)) {
		ucTid = prSwRfb->ucTid;
		if (ucTid >= TID_NUM) {
			DBGLOG(QM, ERROR, "Invalid Tid:%u\n", ucTid);
			return FALSE;
		}
	} else {
		/* for non-qos data, use TID_NUM as tid */
		ucTid = TID_NUM;
	}

	if (prSwRfb->ucPayloadFormat == RX_PAYLOAD_FORMAT_MSDU) {
		return FALSE;
	} else if (prSwRfb->ucPayloadFormat
			== RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU) {
		if (RXM_IS_FROM_DS(u2FrameCtrl)) {
			/*
			 * FromDS frames:
			 * A-MSDU DA must match 802.11 header RA
			 */
			pucCmpAddr = pucDaAddr;
			pucAmsduAddr = pucRaAddr;
		} else if (RXM_IS_TO_DS(u2FrameCtrl)) {
			/*
			 * ToDS frames:
			 * A-MSDU SA must match 802.11 header TA
			 */
			pucCmpAddr = pucSaAddr;
			pucAmsduAddr = pucTaAddr;
		}

		/* mark to drop amsdu with same SeqNo */
		if (prSwRfb->fgIsFirstSubAMSDULLCMS) {
			fgDrop = TRUE;
			DBGLOG(QM, TRACE,
				"QM: Abnormal AMSDU LLC Mismatch.");
		} else {
			if (prSwRfb->fgHdrTran) {
				if (prSwRfb->u2PacketLen <= ETH_HLEN)
					fgDrop = TRUE;
			} else {
				if (prSwRfb->u2PacketLen
					<= prSwRfb->u2HeaderLen + ETH_HLEN)
					fgDrop = TRUE;
			}

			if (fgDrop == TRUE)
				DBGLOG(QM, TRACE,
					"QM: Abnormal AMSDU Unexpected HLen.");
		}

		if (fgDrop == FALSE &&
			pucCmpAddr != NULL && pucAmsduAddr != NULL) {
			if (UNEQUAL_MAC_ADDR(pucCmpAddr, pucAmsduAddr))
				fgDrop = TRUE;

#define __STR_FMT__ \
	"QM: FromDS:%d ToDS:%d TID:%u SN:%u PF:%u" \
	" TA:" MACSTR " RA:" MACSTR " DA:" MACSTR " SA:" MACSTR " Drop:%d"
			if (fgDrop)
				DBGLOG(QM, INFO,
					__STR_FMT__,
					RXM_IS_FROM_DS(u2FrameCtrl),
					RXM_IS_TO_DS(u2FrameCtrl),
					ucTid, prSwRfb->u2SSN,
					prSwRfb->ucPayloadFormat,
					MAC2STR(pucTaAddr),
					MAC2STR(pucRaAddr),
					MAC2STR(pucDaAddr),
					MAC2STR(pucSaAddr),
					fgDrop
					);
#undef __STR_FMT__
		}

		prStaRec->afgIsAmsduInvalid[ucTid] = fgDrop;
		prStaRec->au2AmsduInvalidSN[ucTid] = u2SSN;
	} else {
		/* drop it if find an abnormal asmdu in station record */
		if (prStaRec->afgIsAmsduInvalid[ucTid] == TRUE &&
		    prStaRec->au2AmsduInvalidSN[ucTid] == u2SSN) {
			fgDrop = TRUE;
			DBGLOG(QM, TRACE,
				"QM: Abnormal AMSDU TID:%u SN:%u PF:%u",
				ucTid, prSwRfb->u2SSN,
				prSwRfb->ucPayloadFormat);
		}

		/* reset flag when find last subframe */
		if (prSwRfb->ucPayloadFormat ==
		    RX_PAYLOAD_FORMAT_LAST_SUB_AMSDU) {
			prStaRec->afgIsAmsduInvalid[ucTid] = FALSE;
			prStaRec->au2AmsduInvalidSN[ucTid] = 0XFFFF;
		}
	}

	return fgDrop;
}
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Reorder the received packet
 *
 * \param[in] prSwRfb The RX packet to process
 * \param[out] prReturnedQue The queue for indicating packets
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmProcessPktWithReordering(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct QUE *prReturnedQue)
{

	struct STA_RECORD *prStaRec;
	struct RX_BA_ENTRY *prReorderQueParm;
	struct RX_CTRL *prRxCtrl;

#if CFG_SUPPORT_RX_AMSDU
	uint8_t ucAmsduSubframeIdx;
	uint32_t u2SeqNo;
#endif
	ASSERT(prSwRfb);
	ASSERT(prReturnedQue);

	/* We should have STA_REC here */
	prStaRec = prSwRfb->prStaRec;
	ASSERT(prStaRec);
	ASSERT(prSwRfb->ucTid < CFG_RX_MAX_BA_TID_NUM);

	if (prSwRfb->ucTid >= CFG_RX_MAX_BA_TID_NUM) {
		DBGLOG(QM, WARN, "TID from RXD = %d, out of range!!\n",
			prSwRfb->ucTid);
		DBGLOG_MEM8(QM, ERROR, prSwRfb->pucRecvBuff,
			prSwRfb->u2RxByteCount);
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
		QUEUE_INSERT_TAIL(prReturnedQue, prSwRfb);
		return;
	}

	/* Check whether the BA agreement exists */
	prReorderQueParm = prStaRec->aprRxReorderParamRefTbl[prSwRfb->ucTid];
	if (!prReorderQueParm || !(prReorderQueParm->fgIsValid)) {
		DBGLOG(RX, TEMP,
			"Reordering but no BA agreement for STA[%d] TID[%d]\n",
			prStaRec->ucIndex, prSwRfb->ucTid);
		QUEUE_INSERT_TAIL(prReturnedQue, prSwRfb);
		return;
	}

	prRxCtrl = &prAdapter->rRxCtrl;
	RX_INC_CNT(prRxCtrl, RX_DATA_REORDER_TOTAL_COUNT);

	ucAmsduSubframeIdx = prSwRfb->ucPayloadFormat;
#if CFG_SUPPORT_RX_AMSDU
	u2SeqNo = prSwRfb->u2SSN;

	switch (ucAmsduSubframeIdx) {
	case RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU:
		if (prReorderQueParm->fgAmsduNeedLastFrame) {
			RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT);
			prReorderQueParm->fgAmsduNeedLastFrame = FALSE;
		}
		RX_INC_CNT(prRxCtrl, RX_DATA_MSDU_IN_AMSDU_COUNT);
		RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT);
		break;

	case RX_PAYLOAD_FORMAT_MIDDLE_SUB_AMSDU:
		prReorderQueParm->fgAmsduNeedLastFrame = TRUE;
		RX_INC_CNT(prRxCtrl, RX_DATA_MSDU_IN_AMSDU_COUNT);
		if (prReorderQueParm->u2SeqNo != u2SeqNo) {
			RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT);
			RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT);
		}
		break;
	case RX_PAYLOAD_FORMAT_LAST_SUB_AMSDU:
		prReorderQueParm->fgAmsduNeedLastFrame = FALSE;
		RX_INC_CNT(prRxCtrl, RX_DATA_MSDU_IN_AMSDU_COUNT);
		if (prReorderQueParm->u2SeqNo != u2SeqNo) {
			RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT);
			RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT);
		}
		break;

	case RX_PAYLOAD_FORMAT_MSDU:
		if (prReorderQueParm->fgAmsduNeedLastFrame) {
			RX_INC_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT);
			prReorderQueParm->fgAmsduNeedLastFrame = FALSE;
		}
		break;
	default:
		break;
	}

	prReorderQueParm->u2SeqNo = u2SeqNo;
#endif

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_LOCK(prAdapter->prGlueInfo, 0);

#if CFG_WOW_SUPPORT
	/* After resuming, WinStart and WinEnd are obsolete and unsync
	 * with AP's SN. So assign the SN of first packet to WinStart
	 * as "Fall Within" case.
	 */
	if (prReorderQueParm->fgFirstSnToWinStart) {
		DBGLOG(QM, INFO,
		       "[%u] First resumed SN(%u) reset Window{%u,%u}\n",
		       prSwRfb->ucTid, prSwRfb->u2SSN,
		       prReorderQueParm->u2WinStart,
		       prReorderQueParm->u2WinEnd);

		prReorderQueParm->u2WinStart = prSwRfb->u2SSN;
		prReorderQueParm->u2WinEnd =
			SEQ_ADD(prReorderQueParm->u2WinStart,
				prReorderQueParm->u2WinSize - 1);
		prReorderQueParm->fgFirstSnToWinStart = FALSE;
	}
#endif

	if (prAdapter->chip_info->fgCheckRxDropThreshold &&
	    prSwRfb->ucRxMode != TX_RATE_MODE_EHT_ER &&
	    prSwRfb->ucRxMode != TX_RATE_MODE_EHT_TRIG &&
	    prSwRfb->ucRxMode != TX_RATE_MODE_EHT_MU)
		resetRxRetryCount(prAdapter, prReorderQueParm);

	/* Insert reorder packet */
	qmInsertReorderPkt(prAdapter, prSwRfb, prReorderQueParm, prReturnedQue);

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_UNLOCK(prAdapter->prGlueInfo, 0);
}

void qmProcessBarFrame(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, struct QUE *prReturnedQue)
{
	struct CTRL_BAR_FRAME *prBarCtrlFrame;

	ASSERT(prSwRfb);
	ASSERT(prReturnedQue);
	ASSERT(prSwRfb->pvHeader);

	prBarCtrlFrame = (struct CTRL_BAR_FRAME *) prSwRfb->pvHeader;

	prSwRfb->ucTid =
		(*((uint16_t *) ((uint8_t *) prBarCtrlFrame +
		CTRL_BAR_BAR_CONTROL_OFFSET))) >>
		BAR_CONTROL_TID_INFO_OFFSET;
	prSwRfb->u2SSN =
		(*((uint16_t *) ((uint8_t *) prBarCtrlFrame +
		CTRL_BAR_BAR_INFORMATION_OFFSET))) >>
		OFFSET_BAR_SSC_SN;

	prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
	QUEUE_INSERT_TAIL(prReturnedQue, prSwRfb);

	/* Incorrect STA_REC index */
	prSwRfb->ucStaRecIdx = secLookupStaRecIndexFromTA(prAdapter,
		prBarCtrlFrame->aucSrcAddr);
	if (prSwRfb->ucStaRecIdx >= CFG_STA_REC_NUM) {
		DBGLOG(QM, WARN,
			"QM: (Warning) BAR for a NULL STA_REC, ucStaRecIdx = %d\n",
			prSwRfb->ucStaRecIdx);
		/* ASSERT(0); */
		return;
	}

	DBGLOG(QM, TRACE, "BAR: STA[%u] TID[%u] SSN[%u]\n",
		prSwRfb->ucStaRecIdx, prSwRfb->ucTid, prSwRfb->u2SSN);

	qmHandleRxReorderWinShift(prAdapter, prSwRfb->ucStaRecIdx,
		prSwRfb->ucTid, prSwRfb->u2SSN, prReturnedQue);
}

/**
 * Log when RX stick on the same SN (count only MPDU begin) for a long time,
 * in the case the sender has only one MPDU (AMSDU) and keep retrying.
 */
static void checkRxDuplicateSsn(struct ADAPTER *prAdapter,
				struct RX_BA_ENTRY *prReorderQueParm,
				struct SW_RFB *prSwRfb)
{
#if (CFG_SUPPORT_CONNAC3X == 1)
	uint32_t u4RxDropResetThreshold;

	if (!IS_RX_MPDU_BEGIN(prSwRfb->ucPayloadFormat))
		return;

	if (prSwRfb->u2SSN == prReorderQueParm->rDupDrop.u2SSN) {
		prReorderQueParm->rDupDrop.u4Count++;
	} else {
		prReorderQueParm->rDupDrop.u4Count = 1;
		prReorderQueParm->rDupDrop.u2SSN = prSwRfb->u2SSN;
	}

	u4RxDropResetThreshold = prAdapter->rWifiVar.u4RxDropResetThreshold;
	if (prReorderQueParm->rDupDrop.u4Count % u4RxDropResetThreshold == 0) {
		DBGLOG_LIMITED(QM, TRACE,
			       "QM: sta %u TID %u duplicate %u drop SSN:%u\n",
			       prReorderQueParm->ucStaRecIdx,
			       prReorderQueParm->ucTid,
			       prReorderQueParm->rDupDrop.u4Count,
			       prReorderQueParm->rDupDrop.u2SSN);
	}
#endif
}

/* Increment drop counter and trigger TX reset if over threshold with
 * all the contitions matched:
 *  1. RX mode == EHT
 *  2. Peer retry over defined threshold
 * The counter is maintained in struct RX_BA_ENTRY, and will be reset
 * in the conditions which break rule 1.
 *  if (RX mode != EHT) in qmProcessPktWithReordering()
 */
static void checkRxDuplicateRetry(struct ADAPTER *prAdapter,
		struct RX_BA_ENTRY *prReorderQueParm, struct SW_RFB *prSwRfb)
{
#if (CFG_SUPPORT_CONNAC3X == 1)
	uint8_t ucRxMode = prSwRfb->ucRxMode;
	uint8_t ucCurrentMcs = prSwRfb->ucRxMcs;
	uint32_t u4RxDropResetThreshold;
	uint16_t u2SeqNo = prSwRfb->u2SSN;

	if (!prAdapter->chip_info->fgCheckRxDropThreshold)
		return;

	if (ucRxMode != TX_RATE_MODE_EHT_ER &&
	    ucRxMode != TX_RATE_MODE_EHT_TRIG &&
	    ucRxMode != TX_RATE_MODE_EHT_MU)
		return;

	/**
	 * If Last received == Last drop:
	 *	SWRFB is the 1st SN of 2nd retry.
	 * When retrying, if the first SN == Logged first SN:
	 *	A new retry round
	 */
	if (prReorderQueParm->u4RxRetryCount == 0) {
		if (prReorderQueParm->u2LastRcvdSN ==
		    prReorderQueParm->u2LastFallBehindDropSN)
			prReorderQueParm->u4RxRetryCount = 2;
	} else {
		if (prReorderQueParm->u2LoggedDropHeadSN == u2SeqNo)
			prReorderQueParm->u4RxRetryCount++;
		else
			resetRxRetryCount(prAdapter, prReorderQueParm);
	}
	prReorderQueParm->u2LoggedDropHeadSN = u2SeqNo; /* compared next time */

	if (prReorderQueParm->u4RxRetryCount) {
		DBGLOG_LIMITED(QM, TRACE,
			       "QM: duplicate RX: %u, mode:%u, MCS:%u, bn=%u, SSN:%u\n",
			       prReorderQueParm->u4RxRetryCount, ucRxMode,
			       ucCurrentMcs, prSwRfb->ucHwBandIdx, u2SeqNo);
	}

	u4RxDropResetThreshold = prAdapter->rWifiVar.u4RxDropResetThreshold;
	if (prReorderQueParm->u4RxRetryCount >= u4RxDropResetThreshold) {
		DBGLOG(QM, WARN,
		       "QM: duplicate RX over threshold: %u, MCS:%u, bn=%u, SSN:%u\n",
		       prReorderQueParm->u4RxRetryCount, ucCurrentMcs,
		       prSwRfb->ucHwBandIdx, u2SeqNo);
		/* Send TX reset command */
		wlanResetTxScrambleSeed(prAdapter, prSwRfb->ucHwBandIdx);
		prReorderQueParm->u4ScrambleReset++;
		resetRxRetryCount(prAdapter, prReorderQueParm);
	}
#endif
}

/**
 * To avoid printing every fall behind drop msdu overwhelming the output buffer,
 * log only start and end SN by checking whether there is gap between current
 * dropping SN and the last dropped SN.
 */
static void qmLogDropFallBehind(struct ADAPTER *prAdapter,
		struct RX_BA_ENTRY *prReorderQueParm, struct SW_RFB *prSwRfb,
		uint8_t ucTid, uint16_t u2BarSSN,
		uint16_t u2SeqNo, uint16_t u2WinStart, uint16_t u2WinEnd,
		uint16_t u2IpId)
{
	uint16_t u2LastDrop = prReorderQueParm->u2LastFallBehindDropSN;
	uint16_t u2DropGap = SEQ_DIFF(u2LastDrop, u2SeqNo);
	uint64_t u8Count;

	prReorderQueParm->u2LastFallBehindDropSN = u2SeqNo;

	u8Count = RX_GET_CNT(&prAdapter->rRxCtrl, RX_REORDER_BEHIND_DROP_COUNT);
	if (prAdapter->rWifiVar.u4BaVerboseLogging) {
		DBGLOG(RX, INFO,
		       "QM:(D)[%u:%u]L:%u(~%u)(%u~){%u,%u} ipid:%u BAR SSN:%u/%u total:%lu",
		       prReorderQueParm->ucStaRecIdx, ucTid,
		       prReorderQueParm->u2LastRcvdSN, u2LastDrop, u2SeqNo,
		       u2WinStart, u2WinEnd, u2IpId,
		       IS_BAR_SSN_VALID(prReorderQueParm), u2BarSSN, u8Count);
		return;
	}

	checkRxDuplicateSsn(prAdapter, prReorderQueParm, prSwRfb);

	if (u2DropGap <= 1)
		return;

	checkRxDuplicateRetry(prAdapter, prReorderQueParm, prSwRfb);

	if (IS_BAR_SSN_VALID(prReorderQueParm))
		DBGLOG(RX, INFO,
		       "QM:(D)[%u:%u]L:%u(~%u)(%u~){%u,%u} ipid:%u BAR SSN:%u/%u total:%lu reset:%u",
		       prReorderQueParm->ucStaRecIdx, ucTid,
		       prReorderQueParm->u2LastRcvdSN, u2LastDrop, u2SeqNo,
		       u2WinStart, u2WinEnd, u2IpId, 1, u2BarSSN, u8Count,
		       prReorderQueParm->u4ScrambleReset);
	else
		DBGLOG(RX, TRACE,
		       "QM:(D)[%u:%u]L:%u(~%u)(%u~){%u,%u} ipid:%u BAR SSN:%u/%u total:%lu reset:%u",
		       prReorderQueParm->ucStaRecIdx, ucTid,
		       prReorderQueParm->u2LastRcvdSN, u2LastDrop, u2SeqNo,
		       u2WinStart, u2WinEnd, u2IpId, 0, u2BarSSN, u8Count,
		       prReorderQueParm->u4ScrambleReset);
}

static struct UDP_HEADER *qmGetUdpPkt(uint8_t *pucData, uint16_t u2PacketLen,
		uint16_t *pUdpLen)
{
	uint16_t u2EtherType = 0;
	uint8_t *pucEthBody = NULL;
	struct UDP_HEADER *pUdp = NULL;
	uint32_t ipHLen = 0;
	uint16_t u2UdpLen = 0;

	/* check if pkt at least have eth/ip/udp header to read */
	if (u2PacketLen < (ETHER_HEADER_LEN + IP_HEADER_LEN + UDP_HDR_LEN) ||
		u2PacketLen > ETHER_MAX_PKT_SZ)
		goto end;

	u2EtherType = (pucData[ETH_TYPE_LEN_OFFSET] << 8) |
		(pucData[ETH_TYPE_LEN_OFFSET + 1]);
	if (u2EtherType != ETH_P_IPV4)
		goto end;

	/* check ip version and ip proto */
	pucEthBody = &pucData[ETHER_HEADER_LEN];
	if (((pucEthBody[0] & IPVH_VERSION_MASK) >>
		IPVH_VERSION_OFFSET) != IPVERSION)
		goto end;
	if (pucEthBody[IP_PROTO_HLEN] != IP_PRO_UDP)
		goto end;

	/* get actual ip header len and check if udp header safe to read */
	ipHLen = (pucEthBody[0] & 0x0F) << 2;
	if (unlikely(u2PacketLen < ETHER_HEADER_LEN + ipHLen + UDP_HDR_LEN))
		goto end;

	/* check if udp payload safe to read */
	pUdp = (struct UDP_HEADER *)&pucEthBody[ipHLen];
	u2UdpLen = NTOHS(pUdp->u2Length);
	if (unlikely(u2PacketLen < ETHER_HEADER_LEN + ipHLen + u2UdpLen)) {
		pUdp = NULL;
		u2UdpLen = 0;
		goto end;
	}
end:
	if (pUdpLen)
		*pUdpLen = u2UdpLen;
	return pUdp;
}

struct DHCP_PROTOCOL *qmGetDhcpPkt(uint8_t *pucData, uint16_t u2PacketLen,
	u_int8_t fgFromServer, uint16_t *pDhcpLen)
{
	struct UDP_HEADER *pucUdpPkt = NULL;
	uint16_t udpLen = 0;
	uint16_t dhcpLen = 0;
	uint16_t u2UdpDstPort;
	uint16_t u2UdpSrcPort;
	struct DHCP_PROTOCOL *prDhcp = NULL;
	uint32_t u4DhcpMagicCode = 0;

	pucUdpPkt = qmGetUdpPkt(pucData, u2PacketLen, &udpLen);
	if (!pucUdpPkt)
		goto end;

	/* check udp port is dhcp */
	u2UdpDstPort = NTOHS(pucUdpPkt->u2DstPort);
	u2UdpSrcPort = NTOHS(pucUdpPkt->u2SrcPort);
	if (fgFromServer &&
	    (u2UdpSrcPort != UDP_PORT_DHCPS || u2UdpDstPort != UDP_PORT_DHCPC))
		goto end;

	if (!fgFromServer &&
	    (u2UdpSrcPort != UDP_PORT_DHCPC || u2UdpDstPort != UDP_PORT_DHCPS))
		goto end;

	if (udpLen < UDP_HDR_LEN + sizeof(struct DHCP_PROTOCOL))
		goto end;

	prDhcp = (struct DHCP_PROTOCOL *)pucUdpPkt->aucData;
	u4DhcpMagicCode = NTOHL(prDhcp->u4MagicCookie);
	if (u4DhcpMagicCode != DHCP_MAGIC_NUMBER) {
		DBGLOG(INIT, WARN, "dhcp wrong magic number, magic code: %d\n",
			u4DhcpMagicCode);
		prDhcp = NULL;
		goto end;
	}

	dhcpLen = udpLen - UDP_HDR_LEN;

	DBGLOG(QM, LOUD, "Len:%u dhcpLen:%u\n", u2PacketLen, dhcpLen);
end:
	if (pDhcpLen)
		*pDhcpLen = dhcpLen;

	return prDhcp;
}


#if CFG_SUPPORT_DHCP_RESET_BA_WINDOW
u_int8_t qmIsBaNeedReset(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	u_int8_t fgRet = FALSE;
	uint8_t *pucData;
	struct DHCP_PROTOCOL *prDhcp;

	pucData = (uint8_t *)prSwRfb->pvHeader;
	if (!pucData)
		goto end;

	/* check if pkt is dhcp from server */
	prDhcp = (struct DHCP_PROTOCOL *) qmGetDhcpPkt(pucData,
			prSwRfb->u2PacketLen, TRUE, NULL);
	if (prDhcp && prSwRfb->u2SSN == 0)
		fgRet = TRUE;

end:
	return fgRet;
}

void qmBaResetCheck(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct QUE *prReturnedQue)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint16_t u2WinStart;
	uint16_t u2WinEnd;

	if (IS_FEATURE_DISABLED(prWifiVar->fgDhcpResetBaWindow))
		return;

	if (!qmIsBaNeedReset(prAdapter, prSwRfb))
		return;

	/* Stop rReorderBubbleTimer and dequeue all pkt */
	if (prReorderQueParm->fgHasBubble) {
		prReorderQueParm->fgHasBubble = FALSE;
		cnmTimerStopTimer(prAdapter,
			&prReorderQueParm->rReorderBubbleTimer);

		QUEUE_CONCATENATE_QUEUES(prReturnedQue,
			&(prReorderQueParm->rReOrderQue));
	}

	/* Reset BA Windows */
	u2WinStart = prReorderQueParm->u2WinStart;
	u2WinEnd = prReorderQueParm->u2WinEnd;
	prReorderQueParm->u2WinStart = SEQ_ADD(prSwRfb->u2SSN, 1);
	prReorderQueParm->u2WinEnd =
		SEQ_ADD(prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinSize - 1);
#if CFG_SUPPORT_RX_AMSDU
	prReorderQueParm->ucLastAmsduSubIdx = RX_PAYLOAD_FORMAT_MSDU;
#endif

	DBGLOG(QM, INFO, "BA Win Shift STA[%u] TID[%u] {%u,%u} => {%u,%u}\n",
		prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid,
		u2WinStart, u2WinEnd, prReorderQueParm->u2WinStart,
		prReorderQueParm->u2WinEnd);
}
#endif /* CFG_SUPPORT_DHCP_RESET_BA_WINDOW */

void qmInsertReorderPkt(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct QUE *prReturnedQue)
{
	uint16_t u2SeqNo;
	uint16_t u2WinStart;
	uint16_t u2WinEnd;
	uint16_t u2BarSSN;
	uint8_t ucBssIndex;

	/* Start to reorder packets */
	u2SeqNo = prSwRfb->u2SSN;
	u2WinStart = prReorderQueParm->u2WinStart;
	u2WinEnd = prReorderQueParm->u2WinEnd;
	u2BarSSN = prReorderQueParm->u2BarSSN;

	/* Debug */
	DBGLOG(RX, TEMP, "QM:ipid=%d(R)[%u](%u){%u,%u}\n",
		GLUE_GET_PKT_IP_ID(prSwRfb->pvPacket), prSwRfb->ucTid,
		u2SeqNo, u2WinStart, u2WinEnd);

	ucBssIndex = secGetBssIdxByWlanIdx(prAdapter, prSwRfb->ucWlanIdx);
	REORDERING_INC_BSS_CNT(&prAdapter->rRxCtrl, ucBssIndex);
#if CFG_RFB_TRACK
	RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
		RFB_TRACK_REORDERING_IN);
#endif /* CFG_RFB_TRACK */

	if (prReorderQueParm->fgNoDrop) {
		if (!SEQ_SMALLER(u2SeqNo, prReorderQueParm->u2WinStart)) {
			/* Fall behind SSN pkts started to Fall within
			 * StartWin
			 */
			prReorderQueParm->u4SNOverlapCount++;
			if (prReorderQueParm->u4SNOverlapCount > 10) {
				/*  Set threshold as 10 to make sure
				 *  the SSN is really falling within StartWin
				 */
				prReorderQueParm->u4SNOverlapCount = 0;
				prReorderQueParm->fgNoDrop = FALSE;
				DBGLOG(QM, INFO, "NO drop = FALSE, [%d][%d]\n",
					u2SeqNo, prReorderQueParm->u2WinStart);
			}
		}
	}
	/* Case 1: Fall within */
	if ((SEQ_SMALLER(u2WinStart, u2SeqNo) || u2SeqNo == u2WinStart) &&
	    (SEQ_SMALLER(u2SeqNo, u2WinEnd) || u2SeqNo == u2WinEnd)) {
		DBGLOG(RX, TEMP, "RxPkt=%p seq=%d fall within\n",
			prSwRfb, u2SeqNo);

#if CFG_SUPPORT_RX_OOR_BAR
		if (IS_BAR_SSN_VALID(prReorderQueParm) &&
		    (u2SeqNo == u2BarSSN || SEQ_SMALLER(u2BarSSN, u2SeqNo))) {
			prReorderQueParm->u2BarSSN = 0;
			CLR_BAR_SSN_VALID(prReorderQueParm);
			prReorderQueParm->u2LastRcvdSN = 0;
			DBGLOG(RX, INFO,
				"Clear %u:%u BAR SSN, SN %d >= u2BarSSN %d\n",
				prReorderQueParm->ucStaRecIdx,
				prReorderQueParm->ucTid,
				u2SeqNo, u2BarSSN);
		}
		prReorderQueParm->u2LastRcvdSN = u2SeqNo;
#endif /* CFG_SUPPORT_RX_OOR_BAR */

		qmInsertFallWithinReorderPkt(prAdapter, prSwRfb,
					     prReorderQueParm, prReturnedQue);

#if QM_RX_WIN_SSN_AUTO_ADVANCING
		if (prReorderQueParm->fgIsWaitingForPktWithSsn) {
			/* Let the first received packet
			 * pass the reorder check
			 */
			DBGLOG(RX, TEMP, "QM:(A)[%d](%u){%u,%u}\n",
				prSwRfb->ucTid, u2SeqNo, u2WinStart, u2WinEnd);

			prReorderQueParm->u2WinStart = u2SeqNo;
			prReorderQueParm->u2WinEnd =
				SEQ_ADD(prReorderQueParm->u2WinStart,
					prReorderQueParm->u2WinSize - 1);
			prReorderQueParm->fgIsWaitingForPktWithSsn = FALSE;
#if CFG_SUPPORT_RX_AMSDU
			/* RX reorder for one MSDU in AMSDU issue */
			prReorderQueParm->ucLastAmsduSubIdx =
				RX_PAYLOAD_FORMAT_MSDU;
#endif
		}
#endif

		qmPopOutDueToFallWithin(prAdapter, prReorderQueParm,
			prReturnedQue);
	} else if (SEQ_SMALLER(u2WinEnd, u2SeqNo) &&
		   SEQ_SMALLER(u2WinStart, u2SeqNo)) { /* Case 2: Fall ahead */
		uint16_t u2Delta, u2BeforeWinEnd;
		uint32_t u4BeforeCount, u4MissingCount;

		DBGLOG(RX, TEMP, "RxPkt=%p seq=%d fall ahead\n",
			prSwRfb, u2SeqNo);

#if QM_RX_WIN_SSN_AUTO_ADVANCING
		if (prReorderQueParm->fgIsWaitingForPktWithSsn)
			prReorderQueParm->fgIsWaitingForPktWithSsn = FALSE;
#endif

		qmInsertFallAheadReorderPkt(prAdapter, prSwRfb,
			prReorderQueParm, prReturnedQue);

		u2BeforeWinEnd = prReorderQueParm->u2WinEnd;

		/* Advance the window after inserting a new tail */
		prReorderQueParm->u2WinEnd = u2SeqNo;
		prReorderQueParm->u2WinStart =
			SEQ_ADD(prReorderQueParm->u2WinEnd,
				-(prReorderQueParm->u2WinSize - 1));
#if CFG_SUPPORT_RX_AMSDU
		/* RX reorder for one MSDU in AMSDU issue */
		prReorderQueParm->ucLastAmsduSubIdx =
			RX_PAYLOAD_FORMAT_MSDU;
#endif
		u4BeforeCount = prReorderQueParm->rReOrderQue.u4NumElem;
		qmPopOutDueToFallAhead(prAdapter, prReorderQueParm,
			prReturnedQue);

		u2Delta = SEQ_DIFF(u2BeforeWinEnd, prReorderQueParm->u2WinEnd);
		u4MissingCount = u2Delta - (u4BeforeCount -
			prReorderQueParm->rReOrderQue.u4NumElem);

		RX_ADD_CNT(&prAdapter->rRxCtrl, RX_DATA_REORDER_MISS_COUNT,
			u4MissingCount);
		/* If the SSN jump over 1024,
		  *  we consider it as an abnormal jump and
		  *  temporally reserve the fall behind packets until
		  *  the pkt SSN is really falling within StartWin
		  */
		if (u2Delta > QUARTER_SEQ_NO_COUNT) {
			prReorderQueParm->fgNoDrop = TRUE;
			prReorderQueParm->u4SNOverlapCount = 0;
			DBGLOG_LIMITED(QM, INFO,
				"QM: SSN jump over 1024:[%d]\n", u2Delta);
		}
		DBGLOG(RX, TEMP, "QM: Miss Count:[%lu]\n",
			RX_GET_CNT(&prAdapter->rRxCtrl,
			RX_DATA_REORDER_MISS_COUNT));

		prReorderQueParm->u2LastRcvdSN = u2SeqNo;
	} else { /* Case 3: Fall behind */
#if CFG_SUPPORT_RX_OOR_BAR
		if (IS_BAR_SSN_VALID(prReorderQueParm) &&
		    SEQ_SMALLER(u2SeqNo, u2BarSSN) &&
		    (u2SeqNo == prReorderQueParm->u2LastRcvdSN || /* AMSDU */
		     SEQ_SMALLER(prReorderQueParm->u2LastRcvdSN, u2SeqNo))) {
			qmPopOutReorderPkt(prAdapter, prReorderQueParm, prSwRfb,
				prReturnedQue, RX_DATA_REORDER_BEHIND_COUNT);
			DBGLOG(RX, LOUD,
				"QM: Data after BAR:[%d](%d){%d,%d} total:%lu",
				prSwRfb->ucTid, u2SeqNo, u2WinStart, u2WinEnd,
				RX_GET_CNT(&prAdapter->rRxCtrl,
				RX_DATA_REORDER_BEHIND_COUNT));
			return;
		}
#endif /* CFG_SUPPORT_RX_OOR_BAR */

#if CFG_SUPPORT_LOWLATENCY_MODE || CFG_SUPPORT_OSHARE
		if (qmIsNoDropPacket(prAdapter, prSwRfb) ||
			prReorderQueParm->fgNoDrop) {
			qmPopOutReorderPkt(prAdapter, prReorderQueParm, prSwRfb,
				prReturnedQue, RX_DATA_REORDER_BEHIND_COUNT);
			DBGLOG(RX, TEMP,
				"QM: No drop packet:[%d](%d){%d,%d} total:%lu\n",
				prSwRfb->ucTid, u2SeqNo, u2WinStart, u2WinEnd,
				RX_GET_CNT(&prAdapter->rRxCtrl,
					RX_DATA_REORDER_BEHIND_COUNT));
#if CFG_SUPPORT_DHCP_RESET_BA_WINDOW
			qmBaResetCheck(prAdapter, prSwRfb, prReorderQueParm,
				prReturnedQue);
#endif /* CFG_SUPPORT_DHCP_RESET_BA_WINDOW */
			return;
		}
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if QM_RX_WIN_SSN_AUTO_ADVANCING && QM_RX_INIT_FALL_BEHIND_PASS
		if (prReorderQueParm->fgIsWaitingForPktWithSsn) {
			qmPopOutReorderPkt(prAdapter, prReorderQueParm, prSwRfb,
				prReturnedQue, RX_DATA_REORDER_BEHIND_COUNT);
			DBGLOG(RX, TEMP, "QM:(P)[%u](%u){%u,%u} total:%lu\n",
				prSwRfb->ucTid, u2SeqNo, u2WinStart, u2WinEnd,
				RX_GET_CNT(&prAdapter->rRxCtrl,
					RX_DATA_REORDER_BEHIND_COUNT));
			return;
		}
#endif

		/* An erroneous packet */
		prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
		qmPopOutReorderPkt(prAdapter, prReorderQueParm, prSwRfb,
				prReturnedQue, RX_REORDER_BEHIND_DROP_COUNT);

		qmLogDropFallBehind(prAdapter, prReorderQueParm, prSwRfb,
		       prSwRfb->ucTid, u2BarSSN, u2SeqNo, u2WinStart, u2WinEnd,
		       GLUE_GET_PKT_IP_ID(prSwRfb->pvPacket));
		return;
	}
}

static void clearReorderingIndexCache(struct RX_BA_ENTRY *prReorderQueParm,
				const struct SW_RFB *prSwRfb)
{
#if CFG_SUPPORT_RX_CACHE_INDEX
	uint16_t u2SSN = prSwRfb->u2SSN & HALF_SEQ_MASK;

	if (prReorderQueParm->prCacheIndex[u2SSN])
		prReorderQueParm->u2CacheIndexCount--;
	prReorderQueParm->prCacheIndex[u2SSN] = NULL;
#endif
}

static void setReorderingIndexCache(struct RX_BA_ENTRY *prReorderQueParm,
				struct SW_RFB *prSwRfb)
{
#if CFG_SUPPORT_RX_CACHE_INDEX
	uint16_t u2SSN = prSwRfb->u2SSN & HALF_SEQ_MASK;

	if (prReorderQueParm->prCacheIndex[u2SSN] == NULL)
		prReorderQueParm->u2CacheIndexCount++;
	prReorderQueParm->prCacheIndex[u2SSN] = prSwRfb;
#endif
}

/**
 * getReorderingIndexCache() - Get a proper pointer as starting point
 * Returing the element by SN in cached index pinters backward,
 * if not found, return the head of the list as a fallback solution.
 */
static struct SW_RFB *getReorderingIndexCache(
				struct RX_BA_ENTRY *prReorderQueParm,
				const struct SW_RFB *prSwRfb)
{
#if CFG_SUPPORT_RX_CACHE_INDEX
	uint16_t i;
	const struct QUE *prReorderQue;
	struct SW_RFB **prCacheIndex = prReorderQueParm->prCacheIndex;
	uint16_t u2WinStart = prReorderQueParm->u2WinStart;

	for (i = prSwRfb->u2SSN;
	     SEQ_SMALLER(u2WinStart, i) || u2WinStart == i; SEQ_DEC(i)) {
		if (prCacheIndex[i & HALF_SEQ_MASK])
			return prCacheIndex[i & HALF_SEQ_MASK];
	}
#endif
	/* Not found, fallback */
	prReorderQue = &(prReorderQueParm->rReOrderQue);
	return QUEUE_GET_HEAD(prReorderQue);
}

void qmInsertFallWithinReorderPkt(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct QUE *prReturnedQue)
{
	struct SW_RFB *prExaminedQueuedSwRfb;
	struct QUE *prReorderQue;
	uint8_t ucAmsduSubframeIdx; /* RX reorder for one MSDU in AMSDU issue */

	ASSERT(prSwRfb);
	ASSERT(prReorderQueParm);
	ASSERT(prReturnedQue);

	prReorderQue = &(prReorderQueParm->rReOrderQue);
	/* There are no packets queued in the Reorder Queue */
	if (QUEUE_GET_HEAD(prReorderQue) == NULL) {
		QUEUE_INSERT_HEAD(prReorderQue, prSwRfb);
		setReorderingIndexCache(prReorderQueParm, prSwRfb);
	} else {
		ucAmsduSubframeIdx = prSwRfb->ucPayloadFormat;

		/* Determine the insert position */
		prExaminedQueuedSwRfb =
			getReorderingIndexCache(prReorderQueParm, prSwRfb);
		do {
			/* Case 1: Terminate. A duplicate packet */
			if (prExaminedQueuedSwRfb->u2SSN == prSwRfb->u2SSN) {
#if CFG_SUPPORT_RX_AMSDU
				/* RX reorder for one MSDU in AMSDU issue */
				/* if middle or last and first is not
				 * duplicated, not a duplicat packet
				 */
				if (!prReorderQueParm->fgIsAmsduDuplicated &&
				    !IS_RX_MPDU_BEGIN(ucAmsduSubframeIdx)) {
					prExaminedQueuedSwRfb =
						(struct SW_RFB *)((
						(struct QUE_ENTRY *)
						prExaminedQueuedSwRfb)->prNext);
					while (prExaminedQueuedSwRfb &&
						((prExaminedQueuedSwRfb->
						u2SSN) == (prSwRfb->u2SSN)))
						prExaminedQueuedSwRfb =
							(struct SW_RFB *)((
							(struct QUE_ENTRY *)
							prExaminedQueuedSwRfb)->
							prNext);

					break;
				}
				/* if first is duplicated,
				 * drop subsequent middle and last frames
				 */
				if (ucAmsduSubframeIdx ==
					RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU)
					prReorderQueParm->fgIsAmsduDuplicated =
						TRUE;
#endif
				prSwRfb->eDst = RX_PKT_DESTINATION_NULL;
				qmPopOutReorderPkt(prAdapter, prReorderQueParm,
					prSwRfb, prReturnedQue,
					RX_DUPICATE_DROP_COUNT);
				DBGLOG(RX, TEMP,
					"seq=%d dup drop total:%lu\n",
					prSwRfb->u2SSN,
					RX_GET_CNT(&prAdapter->rRxCtrl,
						RX_DUPICATE_DROP_COUNT));
				LINK_QUALITY_COUNT_DUP(prAdapter, prSwRfb);
				return;
			}

			/* Case 2: Terminate. The insert point is found */
			else if (SEQ_SMALLER(prSwRfb->u2SSN,
					     prExaminedQueuedSwRfb->u2SSN))
				break;

			/* Case 3: Insert point not found.
			 * Check the next SW_RFB in the Reorder Queue
			 */
			else
				prExaminedQueuedSwRfb =
					(struct SW_RFB *) (((struct QUE_ENTRY *)
						prExaminedQueuedSwRfb)->prNext);
		} while (prExaminedQueuedSwRfb);
#if CFG_SUPPORT_RX_AMSDU
		prReorderQueParm->fgIsAmsduDuplicated = FALSE;
#endif
		/* Update the Reorder Queue Parameters according to
		 * the found insert position
		 */
		if (prExaminedQueuedSwRfb == NULL) /* add RX packet to tail */
			QUEUE_INSERT_TAIL(prReorderQue, prSwRfb);
		else
			QUEUE_INSERT_BEFORE(prReorderQue, prExaminedQueuedSwRfb,
					prSwRfb);

		setReorderingIndexCache(prReorderQueParm, prSwRfb);
	}
}

void qmInsertFallAheadReorderPkt(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct QUE *prReturnedQue)
{
	struct QUE *prReorderQue;

	ASSERT(prSwRfb);
	ASSERT(prReorderQueParm);
	ASSERT(prReturnedQue);
#if CFG_SUPPORT_RX_AMSDU
	/* RX reorder for one MSDU in AMSDU issue */
	prReorderQueParm->fgIsAmsduDuplicated = FALSE;
#endif
	prReorderQue = &(prReorderQueParm->rReOrderQue);
	/* There are no packets queued in the Reorder Queue */
	QUEUE_INSERT_TAIL(prReorderQue, prSwRfb);
	setReorderingIndexCache(prReorderQueParm, prSwRfb);
}

void qmPopOutReorderPkt(struct ADAPTER *prAdapter,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct SW_RFB *prSwRfb, struct QUE *prReturnedQue,
	enum ENUM_RX_STATISTIC_COUNTER eRxCounter)
{
	uint32_t u4PktCnt = 0;
	uint8_t ucBssIndex;

	u4PktCnt++;
	QUEUE_INSERT_TAIL(prReturnedQue, prSwRfb);
	clearReorderingIndexCache(prReorderQueParm, prSwRfb);

	RX_ADD_CNT(&prAdapter->rRxCtrl, eRxCounter, u4PktCnt);

	ucBssIndex = secGetBssIdxByWlanIdx(prAdapter, prSwRfb->ucWlanIdx);
	REORDERING_DEC_BSS_CNT(&prAdapter->rRxCtrl, ucBssIndex);
#if CFG_RFB_TRACK
	RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
		RFB_TRACK_REORDERING_OUT);
#endif /* CFG_RFB_TRACK */
}


void fallWithinVerboseLogging(struct ADAPTER *prAdapter,
		struct SW_RFB *prReorderedSwRfb,
		struct RX_BA_ENTRY *prReorderQueParm,
		u_int8_t fgDequeuHead,
		u_int8_t fgMissing,
		uint8_t fgIsAmsduSubframe,
		u_int8_t fgWinAdvanced)
{
	static const char * const fmt[] = {
		"MSDU", "Last", "Middle", "First"};

	if  (!prAdapter->rWifiVar.u4BaVerboseLogging)
		return;

	DBGLOG(RX, INFO,
		"[class,type,miss,plfmt,isSub,Sta,Tid,WinStart,cSN,rSN,Lsub,WinAdv,deq,fin,inc1,inc2]:%u,%u,%u,%u,%u(%s),%u,%u,%u,%u,%u,%u(%s),%u,%u,%u,%u.%u,%u\n",
		prReorderedSwRfb->ucRxClassify, /* class */
		prReorderedSwRfb->ucPacketType, /* type */
		fgMissing, /* miss */
		prReorderedSwRfb->ucPayloadFormat, /* plfmt */
		fgIsAmsduSubframe, fmt[fgIsAmsduSubframe], /* isSub */
		prReorderQueParm->ucStaRecIdx, /* Sta */
		prReorderQueParm->ucTid, /* Tid */
		prReorderQueParm->u2WinStart, /* WinStart */
		prReorderedSwRfb->u2SSN, /* cSN */
		prReorderQueParm->u2SeqNo, /* rSN */
		prReorderQueParm->ucLastAmsduSubIdx, /* Lsub */
		fmt[prReorderQueParm->ucLastAmsduSubIdx],
		fgWinAdvanced, /* WinAdv */
		fgDequeuHead, /* deq */
		/* fin */
		IS_RX_MPDU_FINAL(prReorderQueParm->ucLastAmsduSubIdx),
		/* inc1 */
		SEQ_SMALLER(prReorderQueParm->u2WinStart,
					prReorderedSwRfb->u2SSN),
		prReorderQueParm->u2SeqNo != prReorderQueParm->u2WinStart,
		/* inc2 */
		prReorderedSwRfb->u2SSN == prReorderQueParm->u2WinStart);
}

void qmPopOutDueToFallWithin(struct ADAPTER *prAdapter,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct QUE *prReturnedQue)
{
	u_int8_t fgMoveWinOnMissingLast;
	struct SW_RFB *prReorderedSwRfb;
	struct QUE *prReorderQue;
	u_int8_t fgDequeuHead, fgMissing;
	OS_SYSTIME rCurrentTime, *prMissTimeout;
	/* RX reorder for one MSDU in AMSDU issue */
	uint8_t fgIsAmsduSubframe;
	u_int8_t fgWinAdvanced = FALSE;

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	fgMissing = FALSE;
	rCurrentTime = 0;
	prMissTimeout =
		&g_arMissTimeout[prReorderQueParm->ucStaRecIdx][
		prReorderQueParm->ucTid];
	if (*prMissTimeout) {
		fgMissing = TRUE;
		GET_CURRENT_SYSTIME(&rCurrentTime);
	}

	/* Tradeoff of spending time waiting for a never arrived LAST;
	 * or for MLO, wait for LAST to cope with MSDU interleaving.
	 */
	fgMoveWinOnMissingLast = prAdapter->rWifiVar.fgMoveWinOnMissingLast;
	/* Check whether any packet can be indicated to the higher layer */
	while (TRUE) {
		if (QUEUE_IS_EMPTY(prReorderQue))
			break;

		/* Always examine the head packet */
		prReorderedSwRfb = QUEUE_GET_HEAD(prReorderQue);
		fgDequeuHead = FALSE;

		/* RX reorder for one MSDU in AMSDU issue */
		/* frameType = curr.frameType */
		fgIsAmsduSubframe = prReorderedSwRfb->ucPayloadFormat;

		fallWithinVerboseLogging(prAdapter, prReorderedSwRfb,
				prReorderQueParm, fgDequeuHead,
				fgMissing, fgIsAmsduSubframe, fgWinAdvanced);

#if CFG_SUPPORT_RX_AMSDU
		/* If SN + 1 come and last frame is first or middle,
		 * update winstart
		 *
		 * if (WinStart < curr.SN && rcvd.SN != WinStart &&
		 * (Ba.LastType == 1st || BA.LastType == middle)
		 */
		if (fgMoveWinOnMissingLast &&
				SEQ_SMALLER(prReorderQueParm->u2WinStart,
				prReorderedSwRfb->u2SSN) &&
		    prReorderQueParm->u2SeqNo != prReorderQueParm->u2WinStart) {
			uint8_t ucLastAmsduSubIdx =
					prReorderQueParm->ucLastAmsduSubIdx;

			if (!IS_RX_MPDU_FINAL(ucLastAmsduSubIdx)) {
				SEQ_INC(prReorderQueParm->u2WinStart);
				prReorderQueParm->ucLastAmsduSubIdx =
					RX_PAYLOAD_FORMAT_MSDU;
			}
		}
#endif
		/* SN == WinStart, so the head packet shall be indicated
		 * (advance the window)
		 */
		/* if (curr.SN == WinStart) */
		if (prReorderedSwRfb->u2SSN == prReorderQueParm->u2WinStart) {
			fgDequeuHead = TRUE;

			/* Processing MSDU and window advanced in while loop,
			 * move onto an exising buffered block,
			 * indicates a bubble was filled. Stop timer.
			 */
			if (fgWinAdvanced && prReorderQueParm->fgHasBubble) {
				prReorderQueParm->fgHasBubble = FALSE;
				cnmTimerStopTimer(prAdapter,
					&prReorderQueParm->rReorderBubbleTimer);
			}

			/* RX reorder for one MSDU in AMSDU issue */
			/* if last frame, winstart++.
			 * Otherwise, keep winstart
			 *
			 * if (curr.frameType == Last || curr.frameType == MSDU)
			 *     WinStart++
			 */
			if (IS_RX_MPDU_FINAL(fgIsAmsduSubframe)) {
				SEQ_INC(prReorderQueParm->u2WinStart);
				fgWinAdvanced = TRUE;
			}
#if CFG_SUPPORT_RX_AMSDU
			/* BA.LastType = curr.frameType */
			if (fgMoveWinOnMissingLast)
				prReorderQueParm->ucLastAmsduSubIdx =
					fgIsAmsduSubframe;
#endif
		} else { /* SN > WinStart, break to update WinEnd */
			if (!prReorderQueParm->fgHasBubble) {
				cnmTimerStartTimer(prAdapter,
					&prReorderQueParm->rReorderBubbleTimer,
					prAdapter->u4QmRxBaMissTimeout);
				prReorderQueParm->fgHasBubble = TRUE;
				prReorderQueParm->u2FirstBubbleSn =
					prReorderQueParm->u2WinStart;

				DBGLOG(RX, TEMP,
					"QM:(Bub Timer) STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
					prReorderQueParm->ucStaRecIdx,
					prReorderedSwRfb->ucTid,
					prReorderQueParm->u2FirstBubbleSn,
					prReorderQueParm->u2WinStart,
					prReorderQueParm->u2WinEnd);
			}

			if (fgMissing &&
				CHECK_FOR_TIMEOUT(rCurrentTime, *prMissTimeout,
				MSEC_TO_SYSTIME(
				prAdapter->u4QmRxBaMissTimeout
				))) {

				DBGLOG(RX, TRACE,
					"QM:RX BA Timeout Next Tid %u SSN %u, WinStart:%u->%u\n",
					prReorderQueParm->ucTid,
					prReorderedSwRfb->u2SSN,
					prReorderQueParm->u2WinStart,
					prReorderedSwRfb->u2SSN+1 & MAX_SEQ_NO);
				fgDequeuHead = TRUE;
				/* WinStart = curr.SN + 1 */
				prReorderQueParm->u2WinStart =
					SEQ_ADD(prReorderedSwRfb->u2SSN, 1);
#if CFG_SUPPORT_RX_AMSDU
				/* RX reorder for one MSDU in AMSDU issue */
				/* BA.LastType = MSDU */
				if (fgMoveWinOnMissingLast)
					prReorderQueParm->ucLastAmsduSubIdx =
						RX_PAYLOAD_FORMAT_MSDU;
#endif
				fgMissing = FALSE;
			} else
				break;
		}

		/* Dequeue the head packet */
		if (fgDequeuHead) {
			QUEUE_REMOVE_HEAD(prReorderQue, prReorderedSwRfb,
					struct SW_RFB *);

			qmPopOutReorderPkt(prAdapter, prReorderQueParm,
				prReorderedSwRfb, prReturnedQue,
				RX_DATA_REORDER_WITHIN_COUNT);
			DBGLOG(RX, TEMP, "QM: [%d] %d (%d) within total:%lu\n",
				prReorderQueParm->ucTid,
				prReorderedSwRfb->u2PacketLen,
				prReorderedSwRfb->u2SSN,
				RX_GET_CNT(&prAdapter->rRxCtrl,
					RX_DATA_REORDER_WITHIN_COUNT));
		}
	}

	if (QUEUE_IS_EMPTY(prReorderQue))
		*prMissTimeout = 0;
	else {
		if (fgMissing == FALSE)
			GET_CURRENT_SYSTIME(prMissTimeout);
	}

	/* After WinStart has been determined, update the WinEnd */
	prReorderQueParm->u2WinEnd =
		SEQ_ADD(prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinSize - 1);
}

void qmPopOutDueToFallAhead(struct ADAPTER *prAdapter,
	struct RX_BA_ENTRY *prReorderQueParm,
	struct QUE *prReturnedQue)
{
	struct SW_RFB *prReorderedSwRfb;
	struct QUE *prReorderQue;
	u_int8_t fgDequeuHead;
	uint8_t fgIsAmsduSubframe; /* RX reorder for one MSDU in AMSDU issue */
	u_int8_t fgWinAdvanced = FALSE;
	OS_SYSTIME *prMissTimeout;

	prReorderQue = &(prReorderQueParm->rReOrderQue);
	prMissTimeout = &g_arMissTimeout[
		prReorderQueParm->ucStaRecIdx][prReorderQueParm->ucTid];

	/* Check whether any packet can be indicated to the higher layer */
	while (TRUE) {
		if (QUEUE_IS_EMPTY(prReorderQue))
			break;

		/* Always examine the head packet */
		prReorderedSwRfb = QUEUE_GET_HEAD(prReorderQue);
		fgDequeuHead = FALSE;

		/* RX reorder for one MSDU in AMSDU issue */
		fgIsAmsduSubframe = prReorderedSwRfb->ucPayloadFormat;
#if CFG_SUPPORT_RX_AMSDU
		/* If SN + 1 come and last frame is first or middle,
		 * update winstart
		 */
		if (SEQ_SMALLER(prReorderQueParm->u2WinStart,
				prReorderedSwRfb->u2SSN) &&
		    prReorderQueParm->u2SeqNo != prReorderQueParm->u2WinStart) {
			uint8_t ucLastAmsduSubIdx =
					prReorderQueParm->ucLastAmsduSubIdx;

			if (!IS_RX_MPDU_FINAL(ucLastAmsduSubIdx)) {
				SEQ_INC(prReorderQueParm->u2WinStart);
				prReorderQueParm->ucLastAmsduSubIdx =
					RX_PAYLOAD_FORMAT_MSDU;
			}
		}
#endif
		/* SN == WinStart, so the head packet shall be indicated
		 * (advance the window)
		 */
		if (prReorderedSwRfb->u2SSN == prReorderQueParm->u2WinStart) {
			fgDequeuHead = TRUE;

			/* Processing MSDU and window advanced in while loop,
			 * move onto an exising buffered block,
			 * indicates a bubble was filled. Stop timer.
			 */
			if (fgWinAdvanced && prReorderQueParm->fgHasBubble) {
				prReorderQueParm->fgHasBubble = FALSE;
				cnmTimerStopTimer(prAdapter,
					&prReorderQueParm->rReorderBubbleTimer);
			}

			/* RX reorder for one MSDU in AMSDU issue */
			/* if last frame, winstart++.
			 * Otherwise, keep winstart
			 */
			if (IS_RX_MPDU_FINAL(fgIsAmsduSubframe)) {
				prReorderQueParm->u2WinStart =
					SEQ_ADD(prReorderedSwRfb->u2SSN, 1);
				fgWinAdvanced = TRUE;
			}
#if CFG_SUPPORT_RX_AMSDU
			prReorderQueParm->ucLastAmsduSubIdx = fgIsAmsduSubframe;
#endif
		} else if (SEQ_SMALLER(prReorderedSwRfb->u2SSN,
					prReorderQueParm->u2WinStart)) {
			/* SN < WinStart, so the head packet shall be
			 * indicated (do not advance the window)
			 */
			fgDequeuHead = TRUE;
		} else { /* SN > WinStart, break to update WinEnd */
			if (!prReorderQueParm->fgHasBubble) {
				cnmTimerStartTimer(prAdapter,
					&prReorderQueParm->rReorderBubbleTimer,
					prAdapter->u4QmRxBaMissTimeout);
				prReorderQueParm->fgHasBubble = TRUE;
				prReorderQueParm->u2FirstBubbleSn =
					prReorderQueParm->u2WinStart;

				DBGLOG(RX, TEMP,
					"QM:(Bub Timer) STA[%u] TID[%u] BubSN[%u] Win{%d, %d}\n",
					prReorderQueParm->ucStaRecIdx,
					prReorderedSwRfb->ucTid,
					prReorderQueParm->u2FirstBubbleSn,
					prReorderQueParm->u2WinStart,
					prReorderQueParm->u2WinEnd);
			}
			break;
		}

		/* Dequeue the head packet */
		if (fgDequeuHead) {
			QUEUE_REMOVE_HEAD(prReorderQue, prReorderedSwRfb,
					struct SW_RFB *);

			qmPopOutReorderPkt(prAdapter, prReorderQueParm,
				prReorderedSwRfb, prReturnedQue,
				RX_DATA_REORDER_AHEAD_COUNT);
			DBGLOG(RX, TEMP, "QM: [%u] %u (%u) ahead total:%lu\n",
				prReorderQueParm->ucTid,
				prReorderedSwRfb->u2PacketLen,
				prReorderedSwRfb->u2SSN,
				RX_GET_CNT(&prAdapter->rRxCtrl,
					RX_DATA_REORDER_WITHIN_COUNT));
		}
	}

	/* After WinStart has been determined, update the WinEnd */
	prReorderQueParm->u2WinEnd =
		SEQ_ADD(prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinSize - 1);

	/* stop rReorderBubbleTimer if queue empty*/
	if (QUEUE_IS_EMPTY(prReorderQue)) {
		*prMissTimeout = 0;
		if (prReorderQueParm->fgHasBubble) {
			prReorderQueParm->fgHasBubble = FALSE;
			cnmTimerStopTimer(prAdapter,
				&prReorderQueParm->rReorderBubbleTimer);
		}
	}
}

/**
 * Form a RX_BA_QUE_ENTRY with given prReorderQueParm and append it to
 * prRxBaEntry.
 */
void addReorderQueParm(struct QUE *prRxBaEntry,
		struct RX_BA_ENTRY *prReorderQueParm,
		struct ADAPTER *prAdapter,
		enum ENUM_SPIN_LOCK_CATEGORY_E lock)
{
	struct RX_BA_QUE_ENTRY *prRxBaQueEntry = NULL;

	prRxBaQueEntry = kalMemZAlloc(sizeof(struct RX_BA_QUE_ENTRY),
					VIR_MEM_TYPE);
	if (unlikely(!prRxBaQueEntry))
		return;

	prRxBaQueEntry->prReorderQueParm = prReorderQueParm;

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, lock);
	QUEUE_INSERT_TAIL(prRxBaEntry, &prRxBaQueEntry->rQueEntry);
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, lock);
}

/**
 * Get a RX_BA_ENTRY, embedded in RX_BA_QUE_ENTRY, from prRxBaEntry.
 */
struct RX_BA_ENTRY *getReorderQueParm(struct QUE *prRxBaEntry,
		struct ADAPTER *prAdapter,
		enum ENUM_SPIN_LOCK_CATEGORY_E lock)
{
	struct RX_BA_QUE_ENTRY *prRxBaQueEntry = NULL;
	struct RX_BA_ENTRY *prReorderQueParm = NULL;

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, lock);
	QUEUE_REMOVE_HEAD(prRxBaEntry,
			prRxBaQueEntry, struct RX_BA_QUE_ENTRY *);
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, lock);
	if (prRxBaQueEntry) {
		prReorderQueParm = prRxBaQueEntry->prReorderQueParm;
		kalMemFree(prRxBaQueEntry, VIR_MEM_TYPE,
				sizeof(struct RX_BA_QUE_ENTRY));
	}

	return prReorderQueParm;
}

/**
 * Run in main_thread context triggered by reordering timeout,
 * call the bottom-half handler in main_thread or add a RX_BA_ENTRY
 * then schedule the handler to be invoked by NAPI.
 */
void qmHandleReorderBubbleTimeout(struct ADAPTER *prAdapter,
				uintptr_t ulParamPtr)
{
	struct RX_BA_ENTRY *prReorderQueParm = (struct RX_BA_ENTRY *)ulParamPtr;
	uint32_t rc;

	addReorderQueParm(&prAdapter->rTimeoutRxBaEntry, prReorderQueParm,
			prAdapter, SPIN_LOCK_RX_FLUSH_TIMEOUT);
	DBGLOG(QM, TRACE, "QM:(Bub Timeout Flush scheduled) STA[%u] TID[%u]\n",
		prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

	rc = kalScheduleNapiTask(prAdapter);

	if (rc == WLAN_STATUS_NOT_ACCEPTED) { /* Not NAPI, flush in main */
		prReorderQueParm =
			getReorderQueParm(&prAdapter->rTimeoutRxBaEntry,
				prAdapter, SPIN_LOCK_RX_FLUSH_TIMEOUT);
		if (prReorderQueParm)
			qmFlushTimeoutReorderBubble(prAdapter,
						prReorderQueParm);
	}
}

/**
 * The bottom-half handler to flush reordering frames on RX timeout.
 */
void qmFlushTimeoutReorderBubble(struct ADAPTER *prAdapter,
				 struct RX_BA_ENTRY *prReorderQueParm)
{
	struct GLUE_INFO *prGlueInfo;
	uint16_t u2FirstBubbleSn;
	uint16_t u2WinStart;
	uint16_t u2WinEnd;
	u_int8_t fgHasBubble;

	ASSERT(prAdapter);

	if (!prReorderQueParm->fgIsValid) {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check Cancel) STA[%u] TID[%u], No Rx BA entry\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		return;
	}

	if (!prReorderQueParm->fgHasBubble) {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check Cancel) STA[%u] TID[%u], Bubble has been filled\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		return;
	}

	DBGLOG(QM, TRACE,
		"QM:(Bub Timeout) STA[%u] TID[%u] BubSN[%u]\n",
		prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid,
		prReorderQueParm->u2FirstBubbleSn);

	fgHasBubble = prReorderQueParm->fgHasBubble;
	u2FirstBubbleSn = prReorderQueParm->u2FirstBubbleSn;
	u2WinStart = prReorderQueParm->u2WinStart;
	u2WinEnd = prReorderQueParm->u2WinEnd;

	prGlueInfo = prAdapter->prGlueInfo;
	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_RX_DIRECT);
	qmHandleEventCheckReorderBubble(prAdapter, prReorderQueParm);
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_RX_DIRECT);

	DBGLOG(QM, INFO,
		"QM:(Bub Timeout) %u:%u Bub(%u)[%u]{%u,%u} -> Bub(%u)[%u]{%u,%u}\n",
		prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid,
		fgHasBubble, u2FirstBubbleSn, u2WinStart, u2WinEnd,
		prReorderQueParm->fgHasBubble,
		prReorderQueParm->u2FirstBubbleSn,
		prReorderQueParm->u2WinStart, prReorderQueParm->u2WinEnd);
}

void qmHandleEventCheckReorderBubble(struct ADAPTER *prAdapter,
				     struct RX_BA_ENTRY *prReorderQueParm)
{
	struct QUE *prReorderQue;
	struct QUE rReturnedQue;
	struct QUE *prReturnedQue = &rReturnedQue;
	struct SW_RFB *prReorderedSwRfb, *prSwRfb;
	OS_SYSTIME *prMissTimeout;

	QUEUE_INITIALIZE(prReturnedQue);

	/* Sanity Check */
	if (!prReorderQueParm) {
		DBGLOG(QM, TRACE, "QM:(Bub Check Cancel) No Rx BA entry\n");
		return;
	}

	if (!prReorderQueParm->fgIsValid) {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check Cancel) STA[%u] TID[%u], No Rx BA entry\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		return;
	}

	if (!prReorderQueParm->fgHasBubble) {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check Cancel) STA[%u] TID[%u], Bubble has been filled\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);
		return;
	}

	prReorderQue = &(prReorderQueParm->rReOrderQue);

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_LOCK(prAdapter->prGlueInfo, 0);

	if (QUEUE_IS_EMPTY(prReorderQue)) {
		prReorderQueParm->fgHasBubble = FALSE;

		DBGLOG(QM, TRACE,
			"QM:(Bub Check Cancel) STA[%u] TID[%u], Bubble has been filled\n",
			prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

		if (HAL_IS_RX_DIRECT(prAdapter))
			RX_DIRECT_REORDER_UNLOCK(prAdapter->prGlueInfo, 0);

		return;
	}

	DBGLOG(QM, TRACE,
		"QM:(Bub Check Event Got) STA[%u] TID[%u]\n",
		prReorderQueParm->ucStaRecIdx, prReorderQueParm->ucTid);

	/* Expected bubble timeout => pop out packets before win_end */
	prReorderedSwRfb = QUEUE_GET_TAIL(prReorderQue);

	prReorderQueParm->u2WinStart = SEQ_ADD(prReorderedSwRfb->u2SSN, 1);
	prReorderQueParm->u2WinEnd = SEQ_ADD(prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinSize - 1);
#if CFG_SUPPORT_RX_AMSDU
	prReorderQueParm->ucLastAmsduSubIdx = RX_PAYLOAD_FORMAT_MSDU;
#endif
	qmPopOutDueToFallAhead(prAdapter, prReorderQueParm, prReturnedQue);

	DBGLOG(QM, TRACE,
		"QM:(Bub Flush) STA[%u] TID[%u] BubSN[%u] Win{%u, %u}\n",
		prReorderQueParm->ucStaRecIdx,
		prReorderQueParm->ucTid,
		prReorderQueParm->u2FirstBubbleSn,
		prReorderQueParm->u2WinStart,
		prReorderQueParm->u2WinEnd);

	prReorderQueParm->fgHasBubble = FALSE;

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_UNLOCK(prAdapter->prGlueInfo, 0);

	/* process prReturnedQue after unlock prReturnedQue */
	if (QUEUE_IS_NOT_EMPTY(prReturnedQue)) {
		QUEUE_ENTRY_SET_NEXT(QUEUE_GET_TAIL(prReturnedQue), NULL);

		prSwRfb = QUEUE_GET_HEAD(prReturnedQue);
		while (prSwRfb) {
			DBGLOG(QM, TRACE,
				"QM:(Bub Flush) STA[%u] TID[%u] Pop Out SN[%u]\n",
			  prReorderQueParm->ucStaRecIdx,
			  prReorderQueParm->ucTid,
			  prSwRfb->u2SSN);

			prSwRfb = QUEUE_GET_NEXT_ENTRY(prSwRfb);
		}

		nicRxIndicatePackets(prAdapter, QUEUE_GET_HEAD(prReturnedQue));
	} else {
		DBGLOG(QM, TRACE,
			"QM:(Bub Flush) STA[%u] TID[%u] Pop Out 0 packet\n",
			prReorderQueParm->ucStaRecIdx,
			prReorderQueParm->ucTid);
	}

	prMissTimeout = &g_arMissTimeout[
		prReorderQueParm->ucStaRecIdx][prReorderQueParm->ucTid];
	if (QUEUE_IS_EMPTY(prReorderQue)) {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check) Reset prMissTimeout to zero\n");
		*prMissTimeout = 0;
	} else {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check) Reset prMissTimeout to current time\n");
		GET_CURRENT_SYSTIME(prMissTimeout);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle Mailbox RX messages
 *
 * \param[in] prMailboxRxMsg The received Mailbox message from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleMailboxRxMessage(struct MAILBOX_MSG prMailboxRxMsg)
{
	/* DbgPrint("QM: Enter qmHandleMailboxRxMessage()\n"); */
	/* TODO */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle ADD TX BA Event from the FW
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prEvent The event packet from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleEventTxAddBa(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct mt66xx_chip_info *prChipInfo;
	struct EVENT_TX_ADDBA *prEventTxAddBa;
	struct STA_RECORD *prStaRec;
	uint8_t ucStaRecIdx;
	uint8_t ucTid;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DBGLOG(QM, INFO, "QM:Event +TxBa\n");

	if (!prChipInfo->is_support_hw_amsdu &&
	    prChipInfo->ucMaxSwAmsduNum <= 1) {
		DBGLOG(QM, INFO, "QM:Event +TxBa but chip is not support\n");
		return;
	}

	prEventTxAddBa = (struct EVENT_TX_ADDBA *) (prEvent->aucBuffer);
	ucStaRecIdx = prEventTxAddBa->ucStaRecIdx;
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, ucStaRecIdx);
	if (!prStaRec) {
		/* Invalid STA_REC index, discard the event packet */
		/* ASSERT(0); */
		DBGLOG(QM, INFO,
		       "QM: (Warning) TX ADDBA Event for a NULL STA_REC\n");
		return;
	}

	for (ucTid = 0; ucTid < TX_DESC_TID_NUM; ucTid++) {
		uint8_t ucStaEn = prStaRec->ucAmsduEnBitmap & BIT(ucTid);
		uint8_t ucEvtEn = prEventTxAddBa->ucAmsduEnBitmap & BIT(ucTid);

		if (prChipInfo->is_support_hw_amsdu && ucStaEn != ucEvtEn)
			nicTxSetHwAmsduDescTemplate(prAdapter, prStaRec, ucTid,
						    ucEvtEn >> ucTid);
	}

	prStaRec->ucAmsduEnBitmap = prEventTxAddBa->ucAmsduEnBitmap;
	prStaRec->ucMaxMpduCount = prEventTxAddBa->ucMaxMpduCount;
	prStaRec->u4MaxMpduLen = prEventTxAddBa->u4MaxMpduLen;
	prStaRec->u4MinMpduLen = prEventTxAddBa->u4MinMpduLen;

#if CFG_MTK_MDDP_SUPPORT
	if (mddpIsSupportMcifWifi())
		mddpNotifyDrvTxd(prAdapter, prStaRec, TRUE);
#endif

	DBGLOG(QM, INFO,
	       "QM:Event +TxBa bitmap[0x%x] count[%u] MaxLen[%u] MinLen[%u]\n",
	       prStaRec->ucAmsduEnBitmap, prStaRec->ucMaxMpduCount,
	       prStaRec->u4MaxMpduLen, prStaRec->u4MinMpduLen);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle ADD RX BA Event from the FW
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prEvent The event packet from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleEventRxAddBa(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_RX_ADDBA *prEventRxAddBa;
	struct STA_RECORD *prStaRec;
	uint8_t ucTid;
	uint16_t u2WinSize;
	uint16_t u2WinStart;

	DBGLOG(QM, INFO, "QM:Event +RxBa\n");

	prEventRxAddBa = (struct EVENT_RX_ADDBA *)prEvent->aucBuffer;
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter,
			prEventRxAddBa->ucStaRecIdx);

	ucTid = (prEventRxAddBa->u2BAParameterSet & BA_PARAM_SET_TID_MASK) >>
		BA_PARAM_SET_TID_MASK_OFFSET;

	u2WinSize = (prEventRxAddBa->u2BAParameterSet &
			BA_PARAM_SET_BUFFER_SIZE_MASK) >>
				BA_PARAM_SET_BUFFER_SIZE_MASK_OFFSET;

	u2WinStart = prEventRxAddBa->u2BAStartSeqCtrl >> OFFSET_BAR_SSC_SN;

	nicEventHandleAddBa(prAdapter, prStaRec, ucTid, u2WinSize, u2WinStart);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle DEL RX BA Event from the FW
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prEvent The event packet from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleEventRxDelBa(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_RX_DELBA *prEventRxDelBa;
	struct STA_RECORD *prStaRec;

	/* DbgPrint("QM:Event -RxBa\n"); */

	prEventRxDelBa = (struct EVENT_RX_DELBA *) (
		prEvent->aucBuffer);
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter,
		prEventRxDelBa->ucStaRecIdx);

	if (!prStaRec)
		/* Invalid STA_REC index, discard the event packet */
		/* ASSERT(0); */
		return;
#if 0
	if (!(prStaRec->fgIsValid))
		/* TODO: (Tehuang) Handle the Host-FW synchronization issue */
		/* ASSERT(0); */
		return;
#endif

	qmDelRxBaEntry(prAdapter, prStaRec->ucIndex,
		prEventRxDelBa->ucTid, TRUE);

}

struct RX_BA_ENTRY *qmLookupRxBaEntry(struct ADAPTER *prAdapter,
	uint8_t ucStaRecIdx, uint8_t ucTid)
{
	int i;
	struct QUE_MGT *prQM = &prAdapter->rQM;

	/* DbgPrint("QM: Enter qmLookupRxBaEntry()\n"); */

	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		if (prQM->arRxBaTable[i].fgIsValid) {
			if ((prQM->arRxBaTable[i].ucStaRecIdx == ucStaRecIdx)
				&& (prQM->arRxBaTable[i].ucTid == ucTid))
				return &prQM->arRxBaTable[i];
		}
	}
	return NULL;
}

u_int8_t qmAddRxBaEntry(struct ADAPTER *prAdapter,
	uint8_t ucStaRecIdx, uint8_t ucTid,
	uint16_t u2WinStart, uint16_t u2WinSize)
{
	int i;
	struct RX_BA_ENTRY *prRxBaEntry = NULL;
	struct STA_RECORD *prStaRec;
	struct QUE_MGT *prQM = &prAdapter->rQM;

	ASSERT(ucStaRecIdx < CFG_STA_REC_NUM);

	if (ucStaRecIdx >= CFG_STA_REC_NUM || ucTid >= CFG_RX_MAX_BA_TID_NUM) {
		/* Invalid STA_REC index, discard the event packet */
		DBGLOG(QM, WARN,
			"QM: (WARNING) RX ADDBA Event for a invalid ucStaRecIdx = %d, ucTID=%d\n",
			ucStaRecIdx, ucTid);
		return FALSE;
	}

	prStaRec = &prAdapter->arStaRec[ucStaRecIdx];
	ASSERT(prStaRec);

	/* 4 <1> Delete before adding */
	/* Remove the BA entry for the same (STA, TID) tuple if it exists */
	/* prQM->ucRxBaCount-- */
	if (qmLookupRxBaEntry(prAdapter, ucStaRecIdx, ucTid))
		qmDelRxBaEntry(prAdapter, ucStaRecIdx, ucTid, TRUE);
	/* 4 <2> Add a new BA entry */
	/* No available entry to store the BA agreement info. Retrun FALSE. */
	if (prQM->ucRxBaCount >= CFG_NUM_OF_RX_BA_AGREEMENTS) {
		DBGLOG(QM, ERROR,
			"QM: **failure** (limited resource, ucRxBaCount=%d)\n",
			prQM->ucRxBaCount);
		return FALSE;
	}
	/* Find the free-to-use BA entry */
	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		if (!prQM->arRxBaTable[i].fgIsValid) {
			prRxBaEntry = &(prQM->arRxBaTable[i]);
			prQM->ucRxBaCount++;
			DBGLOG(QM, LOUD,
				"QM: ucRxBaCount=%d\n", prQM->ucRxBaCount);
			break;
		}
	}

	/* If a free-to-use entry is found,
	 * configure it and associate it with the STA_REC
	 */
	u2WinSize += prAdapter->rWifiVar.u2BaExtSize;
	if (prRxBaEntry) {
		prRxBaEntry->ucStaRecIdx = ucStaRecIdx;
		prRxBaEntry->ucTid = ucTid;
		prRxBaEntry->u2WinStart = u2WinStart;
		prRxBaEntry->u2WinSize = u2WinSize;
		prRxBaEntry->u2WinEnd = SEQ_ADD(u2WinStart, u2WinSize - 1);
		prRxBaEntry->u4ScrambleReset = 0;
		prRxBaEntry->rDupDrop.u4Count = 0;
		prRxBaEntry->rDupDrop.u2SSN = MAX_SEQ_NO_COUNT;
		resetRxRetryCount(prAdapter, prRxBaEntry);
#if CFG_SUPPORT_RX_AMSDU
		/* RX reorder for one MSDU in AMSDU issue */
		prRxBaEntry->ucLastAmsduSubIdx = RX_PAYLOAD_FORMAT_MSDU;
		prRxBaEntry->fgAmsduNeedLastFrame = FALSE;
		prRxBaEntry->fgIsAmsduDuplicated = FALSE;
#endif
		prRxBaEntry->fgIsValid = TRUE;
		prRxBaEntry->fgIsWaitingForPktWithSsn = TRUE;
		prRxBaEntry->fgHasBubble = FALSE;
#if CFG_SUPPORT_RX_CACHE_INDEX
		kalMemZero(prRxBaEntry->prCacheIndex,
				sizeof(prRxBaEntry->prCacheIndex));
		prRxBaEntry->u2CacheIndexCount = 0;
#endif

		g_arMissTimeout[ucStaRecIdx][ucTid] = 0;

		DBGLOG(QM, INFO,
			"QM: +RxBA(STA=%u TID=%u WinStart=%u WinEnd=%u WinSize=%u)\n",
			ucStaRecIdx, ucTid, prRxBaEntry->u2WinStart,
			prRxBaEntry->u2WinEnd,
			prRxBaEntry->u2WinSize);

		/* Update the BA entry reference table for per-packet lookup */
		prStaRec->aprRxReorderParamRefTbl[ucTid] = prRxBaEntry;
	} else {
		/* This shall not happen because
		 * FW should keep track of the usage of RX BA entries
		 */
		DBGLOG(QM, ERROR, "QM: **AddBA Error** (ucRxBaCount=%d)\n",
			prQM->ucRxBaCount);
		return FALSE;
	}


	return TRUE;
}

/**
 * The bottom-half handler to flush frames on RX BA deleted.
 */
void qmFlushDeletedBaReorder(struct ADAPTER *prAdapter,
		struct RX_BA_ENTRY *prRxBaEntry)
{
	uint8_t ucStaRecIdx = prRxBaEntry->ucStaRecIdx;
	uint8_t ucTid = prRxBaEntry->ucTid;
	struct QUE_MGT *prQM = &prAdapter->rQM;
	struct SW_RFB *prFlushedPacketList = NULL;

	prFlushedPacketList = qmFlushStaRxQueue(prAdapter, prRxBaEntry);

	if (prFlushedPacketList) {
		if (prRxBaEntry->fgFlushToHost) {
			nicRxIndicatePackets(prAdapter, prFlushedPacketList);
		} else {
			struct SW_RFB *prSwRfb;
			struct SW_RFB *prNextSwRfb;

			prSwRfb = prFlushedPacketList;

			do {
				prNextSwRfb = QUEUE_GET_NEXT_ENTRY(prSwRfb);
				nicRxReturnRFB(prAdapter, prSwRfb);
				prSwRfb = prNextSwRfb;
			} while (prSwRfb);
		}
	}

	if (prRxBaEntry->fgHasBubble) {
		DBGLOG(QM, TRACE,
			"QM:(Bub Check Cancel) STA[%u] TID[%u], DELBA\n",
			ucStaRecIdx, ucTid);

		cnmTimerStopTimer(prAdapter, &prRxBaEntry->rReorderBubbleTimer);
		prRxBaEntry->fgHasBubble = FALSE;
	}

#if ((QM_TEST_MODE == 0) && (QM_TEST_STA_REC_DEACTIVATION == 0))
	/* Update RX BA entry state.
	 * Note that RX queue flush is not done here
	 */
	prRxBaEntry->fgIsValid = FALSE;
	prQM->ucRxBaCount--;
#endif
}

/**
 * Run in main_thread context triggered by deleting reordering BA.
 * If a RX_BA_ENTRY valid exists, call the bottom-half handler in
 * main_thread or add a RX_BA_ENTRY then schedule the handler to
 * be invoked by NAPI.
 */
void qmDelRxBaEntry(struct ADAPTER *prAdapter, uint8_t ucStaRecIdx,
			uint8_t ucTid, u_int8_t fgFlushToHost)
{
	struct RX_BA_ENTRY *prRxBaEntry = NULL;
	struct STA_RECORD *prStaRec;
	uint32_t rc;

	if (ucStaRecIdx >= CFG_STA_REC_NUM) {
		DBGLOG(QM, ERROR, "%u > %u(CFG_STA_REC_NUM)\n",
			ucStaRecIdx, CFG_STA_REC_NUM);
		return;
	}

	prStaRec = &prAdapter->arStaRec[ucStaRecIdx];
	if (!prStaRec) {
		DBGLOG(QM, ERROR, "NULL prStaRec\n");
		return;
	}

	/* Remove the BA entry for the same (STA, TID) tuple if it exists */
	if (ucTid < CFG_RX_MAX_BA_TID_NUM) {
		prRxBaEntry = prStaRec->aprRxReorderParamRefTbl[ucTid];
		if (prRxBaEntry) {
#if ((QM_TEST_MODE == 0) && (QM_TEST_STA_REC_DEACTIVATION == 0))
			prStaRec->aprRxReorderParamRefTbl[ucTid] = NULL;
#endif
			DBGLOG(QM, INFO, "QM: -RxBA(STA=%d,TID=%d)\n",
				ucStaRecIdx, ucTid);
		}
	}

	if (!prRxBaEntry)
		return;

	prRxBaEntry->fgFlushToHost = fgFlushToHost; /* save to be used later */
	addReorderQueParm(&prAdapter->rFlushRxBaEntry, prRxBaEntry,
			prAdapter, SPIN_LOCK_RX_FLUSH_BA);
	DBGLOG(QM, TRACE, "QM:(BA Delete Flush scheduled) STA[%u] TID[%u]\n",
		prRxBaEntry->ucStaRecIdx, prRxBaEntry->ucTid);

	rc = kalScheduleNapiTask(prAdapter);

	if (rc == WLAN_STATUS_NOT_ACCEPTED) { /* Not NAPI, flush in main */
		prRxBaEntry =
			getReorderQueParm(&prAdapter->rFlushRxBaEntry,
				prAdapter, SPIN_LOCK_RX_FLUSH_BA);
		if (prRxBaEntry)
			qmFlushDeletedBaReorder(prAdapter, prRxBaEntry);
	}
}

u_int8_t qmIsIndependentPkt(struct SW_RFB *prSwRfb)
{

	void *pvPacket = NULL;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return FALSE;

	pvPacket = prSwRfb->pvPacket;

	if (!pvPacket)
		return FALSE;

	if (GLUE_GET_INDEPENDENT_PKT(pvPacket))
		return TRUE;

	return FALSE;
}

void mqmParseAssocReqWmmIe(struct ADAPTER *prAdapter,
	uint8_t *pucIE, struct STA_RECORD *prStaRec)
{
	struct IE_WMM_INFO *prIeWmmInfo;
	uint8_t ucQosInfo;
	uint8_t ucQosInfoAC;
	uint8_t ucBmpAC;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;

	if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM)
		&& (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {

		switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
		case VENDOR_OUI_SUBTYPE_WMM_INFO:
			if (IE_LEN(pucIE) != 7)
				break;	/* WMM Info IE with a wrong length */

			prStaRec->fgIsQoS = TRUE;
			prStaRec->fgIsWmmSupported = TRUE;

			prIeWmmInfo = (struct IE_WMM_INFO *) pucIE;
			ucQosInfo = prIeWmmInfo->ucQosInfo;
			ucQosInfoAC = ucQosInfo & BITS(0, 3);

			if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucUapsd))
				prStaRec->fgIsUapsdSupported =
					(ucQosInfoAC) ? TRUE : FALSE;
			else
				prStaRec->fgIsUapsdSupported = FALSE;

			ucBmpAC = 0;

			if (ucQosInfoAC & WMM_QOS_INFO_VO_UAPSD)
				ucBmpAC |= BIT(ACI_VO);

			if (ucQosInfoAC & WMM_QOS_INFO_VI_UAPSD)
				ucBmpAC |= BIT(ACI_VI);

			if (ucQosInfoAC & WMM_QOS_INFO_BE_UAPSD)
				ucBmpAC |= BIT(ACI_BE);

			if (ucQosInfoAC & WMM_QOS_INFO_BK_UAPSD)
				ucBmpAC |= BIT(ACI_BK);
			prStaRec->ucBmpTriggerAC = prStaRec->ucBmpDeliveryAC =
				ucBmpAC;
			prStaRec->ucUapsdSp = (ucQosInfo &
				WMM_QOS_INFO_MAX_SP_LEN_MASK) >> 5;
			break;

		default:
			/* Other WMM QoS IEs. Ignore any */
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To process WMM related IEs in ASSOC_RSP
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prSwRfb            The received frame
 * \param[in] pucIE              The pointer to the first IE in the frame
 * \param[in] u2IELength         The total length of IEs in the frame
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mqmProcessAssocReq(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint8_t *pucIE,
	uint16_t u2IELength)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2Offset;
	uint8_t *pucIEStart;

	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	prStaRec->fgIsQoS = FALSE;
	prStaRec->fgIsWmmSupported = prStaRec->fgIsUapsdSupported = FALSE;

	pucIEStart = pucIE;

	/* If the device does not support QoS or if
	 * WMM is not supported by the peer, exit.
	 */
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return;

	/* Determine whether QoS is enabled with the association */
	else {
		prStaRec->u4Flags = 0;
		IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
			switch (IE_ID(pucIE)) {
			case ELEM_ID_VENDOR:
				mqmParseAssocReqWmmIe(prAdapter,
					pucIE, prStaRec);

#if CFG_SUPPORT_MTK_SYNERGY
				rlmParseCheckMTKOuiIE(prAdapter,
					pucIE, prStaRec);
#endif
				break;

			case ELEM_ID_HT_CAP:
				/* Some client won't put the WMM IE
				 * if client is 802.11n
				 */
				if (IE_LEN(pucIE) ==
					(sizeof(struct IE_HT_CAP) - 2))
					prStaRec->fgIsQoS = TRUE;
				break;
			default:
				break;
			}
		}

		DBGLOG(QM, TRACE,
			"MQM: Assoc_Req Parsing (QoS Enabled=%d)\n",
			prStaRec->fgIsQoS);

	}
}

void mqmParseAssocRspWmmIe(const uint8_t *pucIE,
	struct STA_RECORD *prStaRec)
{
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;

	if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM)
		&& (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {
		struct IE_WMM_PARAM *prWmmParam = (struct IE_WMM_PARAM *) pucIE;
		enum ENUM_ACI eAci;

		switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
		case VENDOR_OUI_SUBTYPE_WMM_PARAM:
			if (IE_LEN(pucIE) != 24)
				break;	/* WMM Info IE with a wrong length */
			prStaRec->fgIsQoS = TRUE;
			prStaRec->fgIsWmmSupported = TRUE;
			prStaRec->fgIsUapsdSupported =
				!!(prWmmParam->ucQosInfo & WMM_QOS_INFO_UAPSD);
			for (eAci = ACI_BE; eAci < ACI_NUM; eAci++)
				prStaRec->afgAcmRequired[eAci] = !!(
					prWmmParam->arAcParam[eAci].ucAciAifsn &
					WMM_ACIAIFSN_ACM);
			DBGLOG(WMM, INFO,
			       "WMM: " MACSTR "ACM BK=%d BE=%d VI=%d VO=%d\n",
			       MAC2STR(prStaRec->aucMacAddr),
			       prStaRec->afgAcmRequired[ACI_BK],
			       prStaRec->afgAcmRequired[ACI_BE],
			       prStaRec->afgAcmRequired[ACI_VI],
			       prStaRec->afgAcmRequired[ACI_VO]);
			break;

		case VENDOR_OUI_SUBTYPE_WMM_INFO:
			if (IE_LEN(pucIE) != 7)
				break;	/* WMM Info IE with a wrong length */
			prStaRec->fgIsQoS = TRUE;
			break;

		default:
			/* Other WMM QoS IEs. Ignore any */
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To process WMM related IEs in ASSOC_RSP
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prSwRfb            The received frame
 * \param[in] pucIE              The pointer to the first IE in the frame
 * \param[in] u2IELength         The total length of IEs in the frame
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mqmProcessAssocRsp(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, const uint8_t *pucIE,
	uint16_t u2IELength)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2Offset;
	const uint8_t *pucIEStart;
#if CFG_SUPPORT_RXSMM_ALLOWLIST
	uint8_t  fgRxsmmEnable = FALSE;
#endif

	ASSERT(prSwRfb);
	ASSERT(pucIE);

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		prSwRfb->ucStaRecIdx);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	prStaRec->fgIsQoS = FALSE;

	pucIEStart = pucIE;

	DBGLOG(QM, TRACE,
		"QM: (fgIsWmmSupported=%d, fgSupportQoS=%d)\n",
		prStaRec->fgIsWmmSupported, prAdapter->rWifiVar.ucQoS);

	/* If the device does not support QoS
	 * or if WMM is not supported by the peer, exit.
	 */
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return;

	/* Determine whether QoS is enabled with the association */
	else {
		prStaRec->u4Flags = 0;
		IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
			switch (IE_ID(pucIE)) {
			case ELEM_ID_VENDOR:
				/* Process WMM related IE */
				mqmParseAssocRspWmmIe(pucIE, prStaRec);

#if CFG_SUPPORT_MTK_SYNERGY
				rlmParseCheckMTKOuiIE(prAdapter,
					pucIE, prStaRec);
#endif

#if CFG_SUPPORT_RXSMM_ALLOWLIST
				if (rlmParseCheckRxsmmOuiIE(prAdapter,
					pucIE, &fgRxsmmEnable))
					prStaRec->fgRxsmmEnable =
						(fgRxsmmEnable) ?
						(fgRxsmmEnable) :
						(prStaRec->fgRxsmmEnable);

				DBGLOG(QM, TRACE, "RxSMM: STAREC enable = %d\n",
					prStaRec->fgRxsmmEnable);
#endif

				break;

			case ELEM_ID_HT_CAP:
				/* Some AP won't put the WMM IE
				 * if client is 802.11n
				 */
				if (IE_LEN(pucIE) ==
					(sizeof(struct IE_HT_CAP) - 2))
					prStaRec->fgIsQoS = TRUE;
				break;

			case ELEM_ID_QOS_MAP_SET:
				DBGLOG(QM, WARN,
					"QM: received assoc resp qosmapset ie\n");
				qosParseQosMapSet(prAdapter, prStaRec, pucIE);
				break;

			default:
				break;
			}
		}
		/* Parse AC parameters and write to HW CRs */
		if (prStaRec->fgIsQoS) {
			mqmIsBssEdcaParamsUpdated(prAdapter, prSwRfb,
					pucIEStart, u2IELength, TRUE);
#if (CFG_SUPPORT_802_11AX == 1)
			if (fgEfuseCtrlAxOn == 1) {
				mqmParseMUEdcaParams(prAdapter, prSwRfb,
				pucIEStart, u2IELength, TRUE);
			}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			/*TODO */
#endif

		}
		DBGLOG(QM, TRACE,
			"MQM: Assoc_Rsp Parsing (QoS Enabled=%d)\n",
			prStaRec->fgIsQoS);

#if (CFG_SUPPORT_802_11BE == 1)
		/*TODO */
#endif
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mqmProcessBcn(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, const uint8_t *pucIE,
	uint16_t u2IELength)
{
	struct BSS_INFO *prBssInfo;
	u_int8_t fgNewParameter;
#if (CFG_SUPPORT_802_11AX == 1)
	u_int8_t fgNewMUEdca = FALSE;
#endif
	uint8_t i;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	ASSERT(pucIE);

	DBGLOG(QM, LOUD, "Enter %s\n", __func__);
	DBGLOG(QM, LOUD, "[BCN] Frame content:");
	DBGLOG_MEM8(QM, LOUD, prSwRfb->pvHeader, prSwRfb->u2PacketLen);
	DBGLOG(QM, LOUD, "[BCN] !=======================================!\n");

	fgNewParameter = FALSE;

	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);

		if (!prBssInfo || !IS_BSS_ACTIVE(prBssInfo))
			continue;

		if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		    prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
			/* P2P client or AIS infra STA */
			if (EQUAL_MAC_ADDR(prBssInfo->aucBSSID,
				((struct WLAN_MAC_MGMT_HEADER *)
				(prSwRfb->pvHeader))->aucBSSID)) {

				fgNewParameter = mqmIsBssEdcaParamsUpdated(
						prAdapter, prSwRfb, pucIE,
						u2IELength, FALSE);
#if (CFG_SUPPORT_802_11AX == 1)
			if (fgEfuseCtrlAxOn == 1)
				fgNewMUEdca = mqmParseMUEdcaParams(prAdapter,
						prSwRfb, pucIE, u2IELength,
						FALSE);
#endif
#if (CFG_SUPPORT_802_11BE == 1)
			/*TODO */
#endif
			}
		}

		/* Appy new parameters if necessary */
		if (fgNewParameter) {
			nicQmUpdateWmmParms(prAdapter, prBssInfo->ucBssIndex);
			fgNewParameter = FALSE;
		}
#if (CFG_SUPPORT_802_11AX == 1)
		if (fgEfuseCtrlAxOn == 1) {
			if (fgNewMUEdca) {
				nicQmUpdateMUEdcaParams(prAdapter,
						prBssInfo->ucBssIndex);
				fgNewMUEdca = FALSE;
			}
		}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
		/*TODO */
#endif
	}
}

/* Return TRUE if WMM IE has been updated to BSS_INFO */
u_int8_t mqmHandleWMMEdcaParams(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
		const uint8_t *pucIE, u_int8_t fgForceOverwrite)
{
	struct IE_WMM_PARAM *prIeWmmParam;
	u_int8_t fgUpdated = FALSE;

	if (IE_LEN(pucIE) != sizeof(struct IE_WMM_PARAM) - ELEM_HDR_LEN)
		return fgUpdated;	/* WMM Param IE with a wrong length */

	prIeWmmParam = (struct IE_WMM_PARAM *) pucIE;
	/* Check the Parameter Set Count to determine
	 * whether EDCA parameters have been changed
	 */
	if (!fgForceOverwrite)
		fgUpdated = mqmIsEdcaParamsChanged(prAdapter, prBssInfo,
				prStaRec, prIeWmmParam->ucQosInfo,
				prIeWmmParam->arAcParam);
	if (fgUpdated || fgForceOverwrite) {
		mqmUpdateEdcaParams(prAdapter, prBssInfo, prStaRec,
				prIeWmmParam->ucQosInfo,
				prIeWmmParam->arAcParam);
		fgUpdated = TRUE;
	}

	return fgUpdated;
}

/* Return TRUE if EDCA Param Set IE has been updated to BSS_INFO */
u_int8_t mqmHandle80211EdcaParamSet(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
		const uint8_t *pucIE, u_int8_t fgForceOverwrite)
{
	struct IE_EDCA_PARAM_SET *pr80211Edca;
	u_int8_t fgUpdated = FALSE;

	if (IE_LEN(pucIE) != sizeof(struct IE_EDCA_PARAM_SET) - ELEM_HDR_LEN)
		return fgUpdated;
	pr80211Edca = (struct IE_EDCA_PARAM_SET *)pucIE;

	if (!fgForceOverwrite)
		fgUpdated = mqmIsEdcaParamsChanged(prAdapter, prBssInfo,
				prStaRec, pr80211Edca->ucQosInfo,
				pr80211Edca->arAcParam);
	if (fgUpdated || fgForceOverwrite) {
		mqmUpdateEdcaParams(prAdapter, prBssInfo, prStaRec,
				pr80211Edca->ucQosInfo, pr80211Edca->arAcParam);
		fgUpdated = TRUE;
	}
	return fgUpdated;
}

void mqmUpdateEdcaParams(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
		struct STA_RECORD *prStaRec, uint8_t ucQosInfo,
		struct WMM_AC_PARAM *arAcParam)
{
	struct WMM_AC_PARAM *prAcParam;
	struct AC_QUE_PARMS *prAcQueParams, *arAcQueParams;
	enum ENUM_WMM_ACI eAci;
	uint8_t *pucWmmParamSetCount;
	uint8_t arBuf[256];
	uint8_t *pos = arBuf;
	uint8_t *end = arBuf + sizeof(arBuf);
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	struct MLD_STA_RECORD *prMldStaRec;
#endif

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	pucWmmParamSetCount = (prMldStaRec && prMldStaRec->fgEPCS) ?
		&prBssInfo->ucBackupWmmParamSetCount :
		&prBssInfo->ucWmmParamSetCount;
	arAcQueParams = (prMldStaRec && prMldStaRec->fgEPCS) ?
		prBssInfo->arBackupACQueParms : prBssInfo->arACQueParms;
#else
	pucWmmParamSetCount = &prBssInfo->ucWmmParamSetCount;
	arAcQueParams = prBssInfo->arACQueParms;
#endif /* CFG_SUPPORT_802_11BE_EPCS */

	/* Update Parameter Set Count */
	*pucWmmParamSetCount = (ucQosInfo & WMM_QOS_INFO_PARAM_SET_CNT);
	/* Update EDCA parameters */
	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prAcQueParams = &arAcQueParams[eAci];
		prAcParam = &arAcParam[eAci];
		mqmFillAcQueParam(prAcParam, prAcQueParams);
		pos += kalSnprintf(pos, end - pos,
			"[%d/%d/%d/%d/%d/%d] ",
			eAci, prAcQueParams->ucIsACMSet,
			prAcQueParams->u2Aifsn, prAcQueParams->u2CWmin,
			prAcQueParams->u2CWmax,
			prAcQueParams->u2TxopLimit);
	}
	DBGLOG(QM, INFO,
	       "BSS[%u] [AC/ACM/Aifsn/CWmin/CWmax/TxopLimit] %sCnt[%d]\n",
	       prBssInfo->ucBssIndex, arBuf, *pucWmmParamSetCount);
}

#if (CFG_SUPPORT_802_11AX == 1)
u_int8_t mqmCompareMUEdcaParameters(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
	struct _IE_MU_EDCA_PARAM_T *prIeMUEdcaParam)
{
	struct _CMD_MU_EDCA_PARAMS_T *prBSSMUEdca;
	struct _MU_AC_PARAM_RECORD_T *prMUAcParamInIE;
	enum ENUM_WMM_ACI eAci;
	uint8_t *pucMUEdcaUpdateCnt;
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	struct MLD_STA_RECORD *prMldStaRec;
#endif

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	pucMUEdcaUpdateCnt = (prMldStaRec && prMldStaRec->fgEPCS) ?
		&prBssInfo->ucBackupMUEdcaUpdateCnt :
		&prBssInfo->ucMUEdcaUpdateCnt;
#else
	pucMUEdcaUpdateCnt = &prBssInfo->ucMUEdcaUpdateCnt;
#endif /* CFG_SUPPORT_802_11BE_EPCS */


	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
		prBSSMUEdca = (prMldStaRec && prMldStaRec->fgEPCS) ?
			&prBssInfo->arBackupMUEdcaParams[eAci] :
			&prBssInfo->arMUEdcaParams[eAci];
#else
		prBSSMUEdca = &prBssInfo->arMUEdcaParams[eAci];
#endif /* CFG_SUPPORT_802_11BE_EPCS */
		prMUAcParamInIE = &prIeMUEdcaParam->arMUAcParam[eAci];

		/* ACM */
		if (prBSSMUEdca->ucIsACMSet != !!(prMUAcParamInIE->ucAciAifsn &
			WMM_ACIAIFSN_ACM))
			return FALSE;

		/* AIFSN */
		if (prBSSMUEdca->ucAifsn != (prMUAcParamInIE->ucAciAifsn &
			WMM_ACIAIFSN_AIFSN))
			return FALSE;

		/* CW Max */
		if (prBSSMUEdca->ucECWmax !=
			((prMUAcParamInIE->ucEcw & WMM_ECW_WMAX_MASK)
				>> WMM_ECW_WMAX_OFFSET))
			return FALSE;

		/* CW Min */
		if (prBSSMUEdca->ucECWmin !=
			(prMUAcParamInIE->ucEcw & WMM_ECW_WMIN_MASK))
			return FALSE;

		/* MU EDCA timer */
		if (prBSSMUEdca->ucMUEdcaTimer !=
			prMUAcParamInIE->ucMUEdcaTimer) {
			DBGLOG(QM, INFO, "timer changed, %d -> %d\n",
			       prBSSMUEdca->ucMUEdcaTimer,
			       prMUAcParamInIE->ucMUEdcaTimer);
			return FALSE;
		}
	}
	/* Check Set Count */
	if (*pucMUEdcaUpdateCnt != (prIeMUEdcaParam->ucMUQosInfo &
		WMM_QOS_INFO_PARAM_SET_CNT)) {
		DBGLOG(QM, INFO, "cnt changed, %d -> %d\n", *pucMUEdcaUpdateCnt,
		       prIeMUEdcaParam->ucMUQosInfo &
		       WMM_QOS_INFO_PARAM_SET_CNT);
		*pucMUEdcaUpdateCnt = (prIeMUEdcaParam->ucMUQosInfo &
			WMM_QOS_INFO_PARAM_SET_CNT);
	}

	return TRUE;
}

u_int8_t mqmUpdateMUEdcaParams(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
	const uint8_t *pucIE, uint8_t fgForceOverride)
{
	struct _CMD_MU_EDCA_PARAMS_T *prBSSMUEdca;
	struct _IE_MU_EDCA_PARAM_T *prIeMUEdcaParam;
	struct _MU_AC_PARAM_RECORD_T *prMUAcParamInIE;
	enum ENUM_WMM_ACI eAci;
	uint8_t fgNewParameter = FALSE;
	uint8_t *pucMUEdcaUpdateCnt;
	uint8_t arBuf[256];
	uint8_t *pos = arBuf;
	uint8_t *end = arBuf + sizeof(arBuf);
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	struct MLD_STA_RECORD *prMldStaRec;
#endif

	do {
		if (IE_LEN(pucIE) != 14)
			break;	/* MU EDCA Param IE with a wrong length */

		prIeMUEdcaParam = (struct _IE_MU_EDCA_PARAM_T *) pucIE;

		/*
		 * Check the Parameter Set Count to determine whether
		 * MU EDCA parameters have been changed
		 */
		if (!fgForceOverride) {
			if (mqmCompareMUEdcaParameters(prAdapter, prBssInfo,
					prStaRec, prIeMUEdcaParam)) {
				fgNewParameter = FALSE;
				break;
			}
		}

		fgNewParameter = TRUE;
		/* Update Parameter Set Count */
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
		prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
		pucMUEdcaUpdateCnt = (prMldStaRec && prMldStaRec->fgEPCS) ?
			&prBssInfo->ucBackupMUEdcaUpdateCnt :
			&prBssInfo->ucMUEdcaUpdateCnt;
#else
		pucMUEdcaUpdateCnt = &prBssInfo->ucMUEdcaUpdateCnt;
#endif /* CFG_SUPPORT_802_11BE_EPCS */
		*pucMUEdcaUpdateCnt = (prIeMUEdcaParam->ucMUQosInfo &
			WMM_QOS_INFO_PARAM_SET_CNT);
		/* Update MU EDCA parameters to BSS structure */
		for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
			prMUAcParamInIE = &(prIeMUEdcaParam->arMUAcParam[eAci]);
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
			prBSSMUEdca = (prMldStaRec && prMldStaRec->fgEPCS) ?
				&prBssInfo->arBackupMUEdcaParams[eAci] :
				&prBssInfo->arMUEdcaParams[eAci];
#else
			prBSSMUEdca = &prBssInfo->arMUEdcaParams[eAci];
#endif /* CFG_SUPPORT_802_11BE_EPCS */

			prBSSMUEdca->ucECWmin = prMUAcParamInIE->ucEcw &
							WMM_ECW_WMIN_MASK;
			prBSSMUEdca->ucECWmax = (prMUAcParamInIE->ucEcw &
							WMM_ECW_WMAX_MASK)
							>> WMM_ECW_WMAX_OFFSET;
			prBSSMUEdca->ucAifsn = (prMUAcParamInIE->ucAciAifsn &
				WMM_ACIAIFSN_AIFSN);
			prBSSMUEdca->ucIsACMSet = (prMUAcParamInIE->ucAciAifsn &
				WMM_ACIAIFSN_ACM) ? TRUE : FALSE;
			prBSSMUEdca->ucMUEdcaTimer =
				prMUAcParamInIE->ucMUEdcaTimer;

			pos += kalSnprintf(pos, end - pos,
				"[%d/%d/%d/%d/%d/%d] ",
				eAci, prBSSMUEdca->ucIsACMSet,
				prBSSMUEdca->ucAifsn, prBSSMUEdca->ucECWmin,
				prBSSMUEdca->ucECWmax,
				prBSSMUEdca->ucMUEdcaTimer);
		}
		DBGLOG(QM, INFO,
		       "BSS[%u] [AC/ACM/Aifsn/ECWmin/ECWmax/Timer] %sForceOverride[%d] NewParameter[%d] Cnt[%d]\n",
		       prBssInfo->ucBssIndex, arBuf, fgForceOverride,
		       fgNewParameter, *pucMUEdcaUpdateCnt);
	} while (FALSE);


	return fgNewParameter;
}

u_int8_t mqmParseMUEdcaParams(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		const uint8_t *pucIE, uint16_t u2IELength,
		uint8_t fgForceOverride)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2Offset;
	struct BSS_INFO *prBssInfo;
	uint8_t fgNewParameter = FALSE;

	if (!prSwRfb)
		return FALSE;

	if (!pucIE)
		return FALSE;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (prStaRec == NULL)
		return FALSE;

	DBGLOG(QM, TRACE, "QM: (fgIsQoS=%d)\n", prStaRec->fgIsQoS);

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS) ||
		(!prStaRec->fgIsQoS))
		return FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return FALSE;

	/* Goal: Obtain the MU EDCA parameters */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_RESERVED:
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_MU_EDCA_PARAM) {
				prStaRec->fgIsMuEdcaSupported = TRUE;
				fgNewParameter = mqmUpdateMUEdcaParams(
					prAdapter, prBssInfo, prStaRec, pucIE,
					fgForceOverride);
			}
			break;
		default:
			break;
		}
	}

	return fgNewParameter;
}
#endif


/*----------------------------------------------------------------------------*/
/*!
 * \brief To parse EDCA Parameter IE (in BCN, Assoc_Rsp or EPCS req/rsp from AP)
 *
 * \param[in] prAdapter          Adapter pointer
 * \param[in] prSwRfb            The received frame
 * \param[in] pucIE              The pointer to the first IE in the frame
 * \param[in] u2IELength         The total length of IEs in the frame
 * \param[in] fgForceOverwrite   TRUE: If EDCA parameters are found, always set
 *                               to BSS INFO.
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
u_int8_t mqmIsBssEdcaParamsUpdated(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, const uint8_t *pucIE,
		uint16_t u2IELength, u_int8_t fgForceOverwrite)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2Offset;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;
	struct BSS_INFO *prBssInfo;
	u_int8_t fgNewParameter = FALSE;
	const uint8_t *pucIEEdca = NULL;
	const uint8_t *pucIEWmm = NULL;

	if (!prSwRfb || !pucIE)
		return FALSE;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return FALSE;

	DBGLOG(QM, TRACE, "QM: (fgIsWmmSupported=%d, fgIsQoS=%d)\n",
		prStaRec->fgIsWmmSupported, prStaRec->fgIsQoS);

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS)
	    || (!prStaRec->fgIsWmmSupported)
	    || (!prStaRec->fgIsQoS))
		return FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	if (!prBssInfo)
		return FALSE;
	/* Goal: Obtain the EDCA parameters */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_EDCA_PARAM_SET:
			DBGLOG(QM, TRACE, "Exist Ieee, %p", pucIE);
			pucIEEdca = pucIE;
			break;
		case ELEM_ID_WMM:
			if (!((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM) &&
			      (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))))
				break;
			if (WMM_IE_OUI_SUBTYPE(pucIE) !=
					VENDOR_OUI_SUBTYPE_WMM_PARAM)
				break;
			DBGLOG(QM, TRACE, "Exist Wmm, %p", pucIE);
			pucIEWmm = pucIE;
			break;
		default:
			break;
		}
	}

	if (pucIEWmm) {
		fgNewParameter = mqmHandleWMMEdcaParams(prAdapter,
				prBssInfo, prStaRec, pucIEWmm,
				fgForceOverwrite);
		DBGLOG(QM, TRACE, "Wmm, %d", fgNewParameter);
		return fgNewParameter;
	}
	if (pucIEEdca) {
		fgNewParameter = mqmHandle80211EdcaParamSet(prAdapter,
				prBssInfo, prStaRec, pucIEEdca,
				fgForceOverwrite);
		DBGLOG(QM, TRACE, "Ieee, %d", fgNewParameter);
		return fgNewParameter;
	}

	return fgNewParameter;
}

/* Return TRUE if EDCA param set in IE is different from the set in BssInfo */
u_int8_t mqmIsEdcaParamsChanged(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
		uint8_t ucQosInfo, struct WMM_AC_PARAM *arAcParam)
{
	struct AC_QUE_PARMS *arAcQueParams, *prAcQueParams;
	struct WMM_AC_PARAM *prWmmAcParams;
	enum ENUM_WMM_ACI eAci;
	uint8_t *pucWmmParamSetCount;
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	struct MLD_STA_RECORD *prMldStaRec;
#endif

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	pucWmmParamSetCount = (prMldStaRec && prMldStaRec->fgEPCS) ?
		&prBssInfo->ucBackupWmmParamSetCount :
		&prBssInfo->ucWmmParamSetCount;
	arAcQueParams = (prMldStaRec && prMldStaRec->fgEPCS) ?
		prBssInfo->arBackupACQueParms : prBssInfo->arACQueParms;
#else
	pucWmmParamSetCount = &prBssInfo->ucWmmParamSetCount;
	arAcQueParams = prBssInfo->arACQueParms;
#endif /* CFG_SUPPORT_802_11BE_EPCS */


	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prAcQueParams = &arAcQueParams[eAci];
		prWmmAcParams = &arAcParam[eAci];

		/* ACM */
		if (prAcQueParams->ucIsACMSet != !!(prWmmAcParams->ucAciAifsn &
			WMM_ACIAIFSN_ACM))
			return TRUE;

		/* AIFSN */
		if (prAcQueParams->u2Aifsn != (prWmmAcParams->ucAciAifsn &
			WMM_ACIAIFSN_AIFSN))
			return TRUE;

		/* CW Max */
		if (prAcQueParams->u2CWmax !=
			(BIT((prWmmAcParams->ucEcw & WMM_ECW_WMAX_MASK) >>
			WMM_ECW_WMAX_OFFSET) - 1))
			return TRUE;

		/* CW Min */
		if (prAcQueParams->u2CWmin != (BIT(prWmmAcParams->ucEcw &
			WMM_ECW_WMIN_MASK) - 1))
			return TRUE;

		if (prAcQueParams->u2TxopLimit !=
			prWmmAcParams->u2TxopLimit)
			return TRUE;
	}
	/* Check Set Count */
	if (*pucWmmParamSetCount != (ucQosInfo &
			WMM_QOS_INFO_PARAM_SET_CNT)) {
		DBGLOG(QM, INFO,
		       "IE count changed (%d -> %d) but Param unchanged\n",
		       *pucWmmParamSetCount,
		       ucQosInfo & WMM_QOS_INFO_PARAM_SET_CNT);
		*pucWmmParamSetCount = ucQosInfo & WMM_QOS_INFO_PARAM_SET_CNT;
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used for filling EDCA IE (including 80211 Edca Set
 * and WFA WMM Edca) into AC_QUE_PARMS in BssInfo
 * \param prAcParam           The pointer to WMM_AC_PARAM in IE
 * \param prAcQueParams       The parameter structure used to store EDCA params
 *                                in BssInfo
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mqmFillAcQueParam(struct WMM_AC_PARAM *prAcParam,
		struct AC_QUE_PARMS *prAcQueParams)
{

	prAcQueParams->ucIsACMSet = !!(prAcParam->ucAciAifsn &
		WMM_ACIAIFSN_ACM);

	prAcQueParams->u2Aifsn = (prAcParam->ucAciAifsn &
		WMM_ACIAIFSN_AIFSN);

	prAcQueParams->u2CWmax = BIT((prAcParam->ucEcw &
		WMM_ECW_WMAX_MASK) >> WMM_ECW_WMAX_OFFSET) - 1;

	prAcQueParams->u2CWmin = BIT(prAcParam->ucEcw &
		WMM_ECW_WMIN_MASK) - 1;

	WLAN_GET_FIELD_16(&prAcParam->u2TxopLimit,
		&prAcQueParams->u2TxopLimit);

	prAcQueParams->ucGuradTime =
		TXM_DEFAULT_FLUSH_QUEUE_GUARD_TIME;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To parse WMM/11n related IEs in scan results (only for AP peers)
 *
 * \param[in] prAdapter       Adapter pointer
 * \param[in]  prScanResult   The scan result which shall be parsed to
 *                            obtain needed info
 * \param[out] prStaRec       The obtained info is stored in the STA_REC
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void mqmProcessScanResult(struct ADAPTER *prAdapter,
	struct BSS_DESC *prScanResult,
	struct STA_RECORD *prStaRec)
{
	uint8_t *pucIE;
	uint16_t u2IELength;
	uint16_t u2Offset;
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;
	u_int8_t fgIsHtVht;

	ASSERT(prScanResult);
	ASSERT(prStaRec);

	/* Reset the flag before parsing */
	prStaRec->fgIsWmmSupported = FALSE;
	prStaRec->fgIsUapsdSupported = FALSE;
	prStaRec->fgIsQoS = FALSE;
	prStaRec->fgIsMscsSupported = FALSE;
#if (CFG_SUPPORT_802_11AX == 1)
	prStaRec->fgIsMuEdcaSupported = FALSE;
#endif
#if CFG_SUPPORT_MLR
	prStaRec->ucMlrSupportBitmap = prScanResult->ucMlrSupportBitmap;
	prStaRec->ucRCPI = prScanResult->ucRCPI;
	MLR_DBGLOG(prAdapter, WMM, INFO,
		"MLR beacon - BSSID:" MACSTR
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		" ,MLIE Valid:%d|LinkId:%d|Mld Addr:" MACSTR
#endif
		" ,MlrSB=0x%02x ucRCPI=%d(RSSI=%d)\n",
		MAC2STR(prScanResult->aucBSSID),
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		prScanResult->rMlInfo.fgValid,
		prScanResult->rMlInfo.ucLinkIndex,
		MAC2STR(prScanResult->rMlInfo.aucMldAddr),
#endif
		prStaRec->ucMlrSupportBitmap,
		prStaRec->ucRCPI,
		RCPI_TO_dBm(prStaRec->ucRCPI));
#endif

	fgIsHtVht = FALSE;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return;

	u2IELength = prScanResult->u2IELength;
	pucIE = prScanResult->pucIeBuf;

	/* <1> Determine whether the peer supports WMM/QoS and UAPSDU */
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {

		case ELEM_ID_EXTENDED_CAP:
#if CFG_SUPPORT_TDLS
			TdlsBssExtCapParse(prStaRec, pucIE);
#endif /* CFG_SUPPORT_TDLS */

#if CFG_STAINFO_FEATURE
			prStaRec->fgSupportProxyARP =
				!!((*(uint32_t *)(pucIE + 2)) &
			BIT(ELEM_EXT_CAP_PROXY_ARP_BIT));

			prStaRec->fgSupportTFS =
				!!((*(uint32_t *)(pucIE + 2)) &
			BIT(ELEM_EXT_CAP_TFS_BIT));

			prStaRec->fgSupportWNMSleep =
				!!((*(uint32_t *)(pucIE + 2)) &
			BIT(ELEM_EXT_CAP_WNM_SLEEP_BIT));

			prStaRec->fgSupportTIMBcast =
				!!((*(uint32_t *)(pucIE + 2)) &
			BIT(ELEM_EXT_CAP_TIM_BCAST_BIT));

			prStaRec->fgSupportDMS =
				!!((*(uint32_t *)(pucIE + 2)) &
			BIT(ELEM_EXT_CAP_DMS_BIT));
#endif

			prStaRec->fgIsMscsSupported = wlanCheckExtCapBit(
				prStaRec, pucIE, ELEM_EXT_CAP_MSCS_BIT);
			if (IS_FEATURE_DISABLED(
				prAdapter->rWifiVar.ucCheckBeacon))
				prStaRec->fgIsMscsSupported = TRUE;
			break;
		case ELEM_ID_WMM:
			if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM) &&
			    (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {
				struct IE_WMM_PARAM *prWmmParam =
					(struct IE_WMM_PARAM *)pucIE;
				enum ENUM_ACI eAci;

				switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
				case VENDOR_OUI_SUBTYPE_WMM_PARAM:
					/* WMM Param IE with a wrong length */
					if (IE_LEN(pucIE) != 24)
						break;
					prStaRec->fgIsWmmSupported = TRUE;
					prStaRec->fgIsUapsdSupported =
						!!(prWmmParam->ucQosInfo &
						   WMM_QOS_INFO_UAPSD);
					for (eAci = ACI_BE; eAci < ACI_NUM;
					     eAci++)
						prStaRec->afgAcmRequired
							[eAci] = !!(
							prWmmParam
								->arAcParam
									[eAci]
								.ucAciAifsn &
							WMM_ACIAIFSN_ACM);
					DBGLOG(WMM, INFO,
					       "WMM: " MACSTR
					       "ACM BK=%d BE=%d VI=%d VO=%d\n",
					       MAC2STR(prStaRec->aucMacAddr),
					       prStaRec->afgAcmRequired[ACI_BK],
					       prStaRec->afgAcmRequired[ACI_BE],
					       prStaRec->afgAcmRequired[ACI_VI],
					       prStaRec->afgAcmRequired
						       [ACI_VO]);
					break;

				case VENDOR_OUI_SUBTYPE_WMM_INFO:
					/* WMM Info IE with a wrong length */
					if (IE_LEN(pucIE) != 7)
						break;
					prStaRec->fgIsWmmSupported = TRUE;
					prStaRec->fgIsUapsdSupported =
						((((
							(struct IE_WMM_INFO *)
							pucIE)->ucQosInfo)
							& WMM_QOS_INFO_UAPSD)
							? TRUE : FALSE);
					break;

				default:
					/* A WMM QoS IE that doesn't matter.
					 * Ignore it.
					 */
					break;
				}
			}
			break;
		default:
			/* A WMM IE that doesn't matter. Ignore it. */
			break;
		}
	}

	/* <1> Determine QoS */
	if (prStaRec->ucDesiredPhyTypeSet & (PHY_TYPE_SET_802_11N |
		PHY_TYPE_SET_802_11AC))
		fgIsHtVht = TRUE;

	if (fgIsHtVht || prStaRec->fgIsWmmSupported)
		prStaRec->fgIsQoS = TRUE;

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AX)
			prStaRec->fgIsQoS = TRUE;
	}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
	if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11BE)
		prStaRec->fgIsQoS = TRUE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Generate the WMM Info IE by Param
 *
 * \param[in] prAdapter  Adapter pointer
 * @param prMsduInfo The TX MMPDU
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t
mqmFillWmmInfoIE(uint8_t *pucOutBuf,
	u_int8_t fgSupportUAPSD, uint8_t ucBmpDeliveryAC,
	uint8_t ucBmpTriggerAC, uint8_t ucUapsdSp)
{
	struct IE_WMM_INFO *prIeWmmInfo;
	uint32_t ucUapsd[] = {
		WMM_QOS_INFO_BE_UAPSD,
		WMM_QOS_INFO_BK_UAPSD,
		WMM_QOS_INFO_VI_UAPSD,
		WMM_QOS_INFO_VO_UAPSD
	};
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;

	ASSERT(pucOutBuf);

	prIeWmmInfo = (struct IE_WMM_INFO *) pucOutBuf;

	prIeWmmInfo->ucId = ELEM_ID_WMM;
	prIeWmmInfo->ucLength = ELEM_MAX_LEN_WMM_INFO;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmInfo->aucOui[0] = aucWfaOui[0];
	prIeWmmInfo->aucOui[1] = aucWfaOui[1];
	prIeWmmInfo->aucOui[2] = aucWfaOui[2];
	prIeWmmInfo->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmInfo->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_INFO;

	prIeWmmInfo->ucVersion = VERSION_WMM;
	prIeWmmInfo->ucQosInfo = 0;

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	if (fgSupportUAPSD) {
		uint8_t ucQosInfo = 0;
		uint8_t i;

		/* Static U-APSD setting */
		for (i = ACI_BE; i <= ACI_VO; i++) {
			if (ucBmpDeliveryAC & ucBmpTriggerAC & BIT(i))
				ucQosInfo |= (uint8_t) ucUapsd[i];
		}

		if (ucBmpDeliveryAC & ucBmpTriggerAC) {
			switch (ucUapsdSp) {
			case WMM_MAX_SP_LENGTH_ALL:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_ALL;
				break;

			case WMM_MAX_SP_LENGTH_2:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_2;
				break;

			case WMM_MAX_SP_LENGTH_4:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_4;
				break;

			case WMM_MAX_SP_LENGTH_6:
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_6;
				break;

			default:
				DBGLOG(QM, INFO, "MQM: Incorrect SP length\n");
				ucQosInfo |= WMM_QOS_INFO_MAX_SP_2;
				break;
			}
		}
		prIeWmmInfo->ucQosInfo = ucQosInfo;

	}

	/* Increment the total IE length
	 * for the Element ID and Length fields.
	 */
	return IE_SIZE(prIeWmmInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Generate the WMM Info IE
 *
 * \param[in] prAdapter  Adapter pointer
 * @param prMsduInfo The TX MMPDU
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t
mqmGenerateWmmInfoIEByStaRec(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo, struct STA_RECORD *prStaRec,
	uint8_t *pucOutBuf)
{
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	u_int8_t fgSupportUapsd;

	ASSERT(pucOutBuf);

	/* In case QoS is not turned off, exit directly */
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return 0;

	if (prStaRec == NULL)
		return 0;

	if (!prStaRec->fgIsQoS)
		return 0;

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;

	fgSupportUapsd = (IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.ucUapsd) &&
		prStaRec->fgIsUapsdSupported);

	return mqmFillWmmInfoIE(pucOutBuf,
		fgSupportUapsd,
		prPmProfSetupInfo->ucBmpDeliveryAC,
		prPmProfSetupInfo->ucBmpTriggerAC,
		prPmProfSetupInfo->ucUapsdSp);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Generate the WMM Info IE
 *
 * \param[in] prAdapter  Adapter pointer
 * @param prMsduInfo The TX MMPDU
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void mqmGenerateWmmInfoIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint32_t u4Length;

	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		prMsduInfo->ucStaRecIndex);
	ASSERT(prStaRec);

	if (prStaRec == NULL)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);

	u4Length = mqmGenerateWmmInfoIEByStaRec(prAdapter,
		prBssInfo, prStaRec,
		((uint8_t *) prMsduInfo->prPacket +
		prMsduInfo->u2FrameLength));

	prMsduInfo->u2FrameLength += u4Length;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Generate the WMM Param IE
 *
 * \param[in] prAdapter  Adapter pointer
 * @param prMsduInfo The TX MMPDU
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void mqmGenerateWmmParamIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct IE_WMM_PARAM *prIeWmmParam;

	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;

	uint8_t aucACI[] = {
		WMM_ACI_AC_BE,
		WMM_ACI_AC_BK,
		WMM_ACI_AC_VI,
		WMM_ACI_AC_VO
	};

	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	enum ENUM_WMM_ACI eAci;
	struct WMM_AC_PARAM *prAcParam;

	DBGLOG(QM, LOUD, "\n");

	ASSERT(prMsduInfo);

	/* In case QoS is not turned off, exit directly */
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		prMsduInfo->ucStaRecIndex);

	if (prStaRec) {
		if (!prStaRec->fgIsQoS)
			return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsduInfo->ucBssIndex);

	if (!prBssInfo || !prBssInfo->fgIsQBSS)
		return;

	prIeWmmParam = (struct IE_WMM_PARAM *)
		((uint8_t *) prMsduInfo->prPacket +
		prMsduInfo->u2FrameLength);

	prIeWmmParam->ucId = ELEM_ID_WMM;
	prIeWmmParam->ucLength = ELEM_MAX_LEN_WMM_PARAM;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmParam->aucOui[0] = aucWfaOui[0];
	prIeWmmParam->aucOui[1] = aucWfaOui[1];
	prIeWmmParam->aucOui[2] = aucWfaOui[2];
	prIeWmmParam->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmParam->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_PARAM;

	prIeWmmParam->ucVersion = VERSION_WMM;
	prIeWmmParam->ucQosInfo = (prBssInfo->ucWmmParamSetCount &
		WMM_QOS_INFO_PARAM_SET_CNT);

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucUapsd))
		prIeWmmParam->ucQosInfo |= WMM_QOS_INFO_UAPSD;

	/* EDCA parameter */

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prAcParam = &prIeWmmParam->arAcParam[eAci];

		/* ACI */
		prAcParam->ucAciAifsn = aucACI[eAci];
		/* ACM */
		if (prBssInfo->arACQueParmsForBcast[eAci].ucIsACMSet)
			prAcParam->ucAciAifsn |= WMM_ACIAIFSN_ACM;
		/* AIFSN */
		prAcParam->ucAciAifsn |=
			(prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn &
			WMM_ACIAIFSN_AIFSN);

		/* ECW Min */
		prAcParam->ucEcw = (prBssInfo->aucCWminLog2ForBcast[eAci] &
			WMM_ECW_WMIN_MASK);
		/* ECW Max */
		prAcParam->ucEcw |=
			((prBssInfo->aucCWmaxLog2ForBcast[eAci] <<
			WMM_ECW_WMAX_OFFSET) & WMM_ECW_WMAX_MASK);

		/* Txop limit */
		WLAN_SET_FIELD_16(&prAcParam->u2TxopLimit,
			prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit);

	}

	/* Increment the total IE length
	 * for the Element ID and Length fields.
	 */
	prMsduInfo->u2FrameLength += IE_SIZE(prIeWmmParam);

}

#if CFG_SUPPORT_TDLS
/*----------------------------------------------------------------------------*/
/*!
 * @brief Generate the WMM Param IE
 *
 * \param[in] prAdapter  Adapter pointer
 * @param prMsduInfo The TX MMPDU
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t mqmGenerateWmmParamIEByParam(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo, uint8_t *pOutBuf)
{
	struct IE_WMM_PARAM *prIeWmmParam;

	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;

	uint8_t aucACI[] = {
		WMM_ACI_AC_BE,
		WMM_ACI_AC_BK,
		WMM_ACI_AC_VI,
		WMM_ACI_AC_VO
	};

	struct AC_QUE_PARMS *prACQueParms;
	/* BE, BK, VO, VI */
	uint8_t auCWminLog2ForBcast[WMM_AC_INDEX_NUM] = {4, 4, 3, 2};
	uint8_t auCWmaxLog2ForBcast[WMM_AC_INDEX_NUM] = {10, 10, 4, 3};
	uint8_t auAifsForBcast[WMM_AC_INDEX_NUM] = {3, 7, 2, 2};
	/* If the AP is OFDM */
	uint8_t auTxopForBcast[WMM_AC_INDEX_NUM] = {0, 0, 94, 47};

	enum ENUM_WMM_ACI eAci;
	struct WMM_AC_PARAM *prAcParam;

	DBGLOG(QM, LOUD, "\n");

	ASSERT(pOutBuf);

	/* In case QoS is not turned off, exit directly */
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return WLAN_STATUS_SUCCESS;

	if (!prBssInfo->fgIsQBSS)
		return WLAN_STATUS_SUCCESS;

	if (IS_FEATURE_ENABLED(
		prAdapter->rWifiVar.fgTdlsBufferSTASleep)) {
		prACQueParms = prBssInfo->arACQueParmsForBcast;

		for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
			prACQueParms[eAci].ucIsACMSet = FALSE;
			prACQueParms[eAci].u2Aifsn = auAifsForBcast[eAci];
			prACQueParms[eAci].u2CWmin =
				BIT(auCWminLog2ForBcast[eAci]) - 1;
			prACQueParms[eAci].u2CWmax =
				BIT(auCWmaxLog2ForBcast[eAci]) - 1;
			prACQueParms[eAci].u2TxopLimit = auTxopForBcast[eAci];

			/* used to send WMM IE */
			prBssInfo->aucCWminLog2ForBcast[eAci] =
				auCWminLog2ForBcast[eAci];
			prBssInfo->aucCWmaxLog2ForBcast[eAci] =
				auCWmaxLog2ForBcast[eAci];
		}
	}

	prIeWmmParam = (struct IE_WMM_PARAM *) pOutBuf;

	prIeWmmParam->ucId = ELEM_ID_WMM;
	prIeWmmParam->ucLength = ELEM_MAX_LEN_WMM_PARAM;

	/* WMM-2.2.1 WMM Information Element Field Values */
	prIeWmmParam->aucOui[0] = aucWfaOui[0];
	prIeWmmParam->aucOui[1] = aucWfaOui[1];
	prIeWmmParam->aucOui[2] = aucWfaOui[2];
	prIeWmmParam->ucOuiType = VENDOR_OUI_TYPE_WMM;
	prIeWmmParam->ucOuiSubtype = VENDOR_OUI_SUBTYPE_WMM_PARAM;

	prIeWmmParam->ucVersion = VERSION_WMM;
	/* STAUT Buffer STA, also sleeps (optional)
	 * The STAUT sends a TDLS Setup/Response/Confirm Frame,
	 * with all four AC flags set to 1 in QoS Info Field
	 * to STA 2, via the AP,
	 */
	if (IS_FEATURE_ENABLED(
		    prAdapter->rWifiVar.fgTdlsBufferSTASleep))
		prIeWmmParam->ucQosInfo = (0x0F &
			WMM_QOS_INFO_PARAM_SET_CNT);
	else
		prIeWmmParam->ucQosInfo = (prBssInfo->ucWmmParamSetCount &
			WMM_QOS_INFO_PARAM_SET_CNT);

	/* UAPSD initial queue configurations (delivery and trigger enabled) */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucUapsd))
		prIeWmmParam->ucQosInfo |= WMM_QOS_INFO_UAPSD;

	/* EDCA parameter */

	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prAcParam = &prIeWmmParam->arAcParam[eAci];
		/* ACI */
		prAcParam->ucAciAifsn = aucACI[eAci];
		/* ACM */
		if (prBssInfo->arACQueParmsForBcast[eAci].ucIsACMSet)
			prAcParam->ucAciAifsn |= WMM_ACIAIFSN_ACM;
		/* AIFSN */
		prAcParam->ucAciAifsn |=
			(prBssInfo->arACQueParmsForBcast[eAci].u2Aifsn &
			WMM_ACIAIFSN_AIFSN);

		/* ECW Min */
		prAcParam->ucEcw = (prBssInfo->aucCWminLog2ForBcast[eAci] &
			WMM_ECW_WMIN_MASK);
		/* ECW Max */
		prAcParam->ucEcw |=
			((prBssInfo->aucCWmaxLog2ForBcast[eAci] <<
			WMM_ECW_WMAX_OFFSET) & WMM_ECW_WMAX_MASK);

		/* Txop limit */
		WLAN_SET_FIELD_16(&prAcParam->u2TxopLimit,
			prBssInfo->arACQueParmsForBcast[eAci].u2TxopLimit);

	}

	/* Increment the total IE length
	 * for the Element ID and Length fields.
	 */
	return IE_SIZE(prIeWmmParam);

}

#endif

u_int8_t isProbeResponse(struct MSDU_INFO *prMgmtTxMsdu)
{
	struct WLAN_MAC_HEADER *prWlanHdr = NULL;

	prWlanHdr =
		(struct WLAN_MAC_HEADER *) ((uintptr_t)
		prMgmtTxMsdu->prPacket + MAC_TX_RESERVED_FIELD);

	return (prWlanHdr->u2FrameCtrl & MASK_FRAME_TYPE) ==
		MAC_FRAME_PROBE_RSP ? TRUE : FALSE;
}


enum ENUM_FRAME_ACTION qmGetFrameAction(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucStaRecIdx, struct MSDU_INFO *prMsduInfo,
	enum ENUM_FRAME_TYPE_IN_CMD_Q eFrameType,
	uint16_t u2FrameLength)
{
	enum ENUM_FRAME_ACTION eFrameAction = FRAME_ACTION_TX_PKT;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucTC = nicTxGetFrameResourceType(eFrameType, prMsduInfo);
	uint16_t u2FreeResource = nicTxGetResource(prAdapter, ucTC);
	uint8_t ucReqResource;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prStaRec = cnmGetStaRecByIndexWithoutInUseCheck(prAdapter,
			ucStaRecIdx);

	do {
		/* 4 <1> Tx, if FORCE_TX is set */
		if (prMsduInfo) {
			if (prMsduInfo->ucControlFlag &
				MSDU_CONTROL_FLAG_FORCE_TX) {
				eFrameAction = FRAME_ACTION_TX_PKT;
				break;
			}

#if CFG_SUPPORT_NAN
			if (prMsduInfo->ucTxToNafQueFlag == TRUE) {
				eFrameAction = FRAME_ACTION_TX_PKT;
				break;
			}
#endif
		}
		/* 4 <2> Drop, if BSS is inactive */
		if (!prBssInfo) {
			DBGLOG(QM, INFO,
				"Drop packets (BSS is NULL)\n");
			eFrameAction = FRAME_ACTION_DROP_PKT;
			break;
		}

		if (!IS_BSS_ACTIVE(prBssInfo)) {
			DBGLOG(QM, TRACE,
				"Drop packets (BSS[%u] is INACTIVE)\n",
				prBssInfo->ucBssIndex);
			eFrameAction = FRAME_ACTION_DROP_PKT;
			break;
		}

		/* 4 <3> Queue, if BSS is absent, drop probe response */
		if (prBssInfo && isNetAbsent(prAdapter, prBssInfo)) {
			if (prMsduInfo && isProbeResponse(prMsduInfo)) {
				DBGLOG(TX, TRACE,
					"Drop probe response (BSS[%u] Absent)\n",
					prBssInfo->ucBssIndex);

				eFrameAction = FRAME_ACTION_DROP_PKT;
			} else {
				DBGLOG(TX, TRACE,
					"Queue packets (BSS[%u] Absent)\n",
					prBssInfo->ucBssIndex);
				eFrameAction = FRAME_ACTION_QUEUE_PKT;
			}
			break;
		}

		/* 4 <4> Check based on StaRec */
		if (prStaRec) {
			/* 4 <4.1> Drop, if StaRec is not in use */
			if (!prStaRec->fgIsInUse) {
				DBGLOG(QM, TRACE,
					"Drop packets (Sta[%u] not in USE)\n",
					prStaRec->ucIndex);
				eFrameAction = FRAME_ACTION_DROP_PKT;
				break;
			}
			/* 4 <4.2> Sta in PS */
			if (qmIsStaInPS(prAdapter, prStaRec)) {
				ucReqResource = halTxGetCmdPageCount(prAdapter,
					u2FrameLength, FALSE) +
					prWifiVar->ucCmdRsvResource +
					QM_MGMT_QUEUED_THRESHOLD;

				/* 4 <4.2.1> Tx, if resource is enough */
				if (u2FreeResource > ucReqResource) {
					eFrameAction = FRAME_ACTION_TX_PKT;
					break;
				}
				/* 4 <4.2.2> Queue, if resource is not enough */
				else {
					DBGLOG(QM, INFO,
						"Queue packets (Sta[%u] in PS)\n",
						prStaRec->ucIndex);
					eFrameAction = FRAME_ACTION_QUEUE_PKT;
					break;
				}
			}
		}
	} while (FALSE);

	/* <5> Resource CHECK! */
	/* <5.1> Reserve resource for CMD & 1X */
	if (eFrameType == FRAME_TYPE_MMPDU) {

		ucReqResource = halTxGetCmdPageCount(prAdapter, u2FrameLength,
			FALSE) + prWifiVar->ucCmdRsvResource;

		if (u2FreeResource < ucReqResource) {
			eFrameAction = FRAME_ACTION_QUEUE_PKT;
			DBGLOG(QM, INFO,
				"Queue MGMT (MSDU[0x%p] Req/Rsv/Free[%u/%u/%u])\n",
				prMsduInfo,
				halTxGetCmdPageCount(prAdapter,
					u2FrameLength, FALSE),
				prWifiVar->ucCmdRsvResource, u2FreeResource);
		}

		/* <6> Timeout check! */
#if CFG_ENABLE_PKT_LIFETIME_PROFILE
		if ((eFrameAction == FRAME_ACTION_QUEUE_PKT) && prMsduInfo) {
			OS_SYSTIME rCurrentTime, rEnqTime;

			GET_CURRENT_SYSTIME(&rCurrentTime);
			rEnqTime = prMsduInfo->rPktProfile.rEnqueueTimestamp;

			if (CHECK_FOR_TIMEOUT(rCurrentTime, rEnqTime,
				MSEC_TO_SYSTIME(
					prWifiVar->u4MgmtQueueDelayTimeout))) {
				eFrameAction = FRAME_ACTION_DROP_PKT;
				log_dbg(QM, INFO, "Drop MGMT (MSDU[0x%p] timeout[%ums])\n",
					prMsduInfo,
					prWifiVar->u4MgmtQueueDelayTimeout);
			}
		}
#endif
	}

	return eFrameAction;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle BSS change operation Event from the FW
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prEvent The event packet from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleEventBssAbsencePresence(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_BSS_ABSENCE_PRESENCE *prEventBssStatus;
	struct BSS_INFO *prBssInfo;
	u_int8_t fgIsNetAbsentOld;

	prEventBssStatus = (struct EVENT_BSS_ABSENCE_PRESENCE *) (
		prEvent->aucBuffer);

	if (!IS_BSS_INDEX_VALID(prEventBssStatus->ucBssIndex)) {
		DBGLOG(QM, WARN, "NAF:BSS IDX is invalid: %u\n",
			prEventBssStatus->ucBssIndex);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prEventBssStatus->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(QM, INFO, "NAF:prBssInfo is NULL\n");
		return;
	}

	fgIsNetAbsentOld = prBssInfo->fgIsNetAbsent;
	prBssInfo->fgIsNetAbsent = prEventBssStatus->ucIsAbsent;
	prBssInfo->ucBssFreeQuota = prEventBssStatus->ucBssFreeQuota;

	if (!prBssInfo->fgIsNetAbsent) {
		if (!prBssInfo->tmLastPresent)
			prBssInfo->tmLastPresent = kalGetTimeTick();
		/* ToDo:: QM_DBG_CNT_INC */
		QM_DBG_CNT_INC(&(prAdapter->rQM), QM_DBG_CNT_27);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		prAdapter->ucBssAbsentBitmap &= ~BIT(prBssInfo->ucBssIndex);
#endif
	} else {
		if (prBssInfo->tmLastPresent) {
			prBssInfo->u4PresentTime = kalGetTimeTick() -
				prBssInfo->tmLastPresent;
			prBssInfo->tmLastPresent = 0;
		}
		/* ToDo:: QM_DBG_CNT_INC */
		QM_DBG_CNT_INC(&(prAdapter->rQM), QM_DBG_CNT_28);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		prAdapter->ucBssAbsentBitmap |= BIT(prBssInfo->ucBssIndex);
#endif
	}

	DBGLOG(QM, INFO, "NAF:B=%d,A=%d,F=%d,P=%u\n",
		prEventBssStatus->ucBssIndex, prBssInfo->fgIsNetAbsent,
		prBssInfo->ucBssFreeQuota, prBssInfo->u4PresentTime);

	/* From Absent to Present */
	if (fgIsNetAbsentOld && !prBssInfo->fgIsNetAbsent) {
		if (HAL_IS_TX_DIRECT(prAdapter)) {
			nicTxDirectStartCheckQTimer(prAdapter);
			/* To process the Mgmt frame which is queued
			 * in tx cmd Q during absent period.
			 */
			if (GLUE_GET_REF_CNT(
				prAdapter->rTxCtrl.i4TxMgmtPendingNum))
				kalSetEvent(prAdapter->prGlueInfo);
		} else {
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
			prAdapter->rQM.fgForceReassign = TRUE;
#endif
			kalSetEvent(prAdapter->prGlueInfo);
			kalSetTxEvent2Hif(prAdapter->prGlueInfo);
		}
	}
}

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle STA change PS mode Event from the FW
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prEvent The event packet from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleEventStaChangePsMode(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_STA_CHANGE_PS_MODE *prEventStaChangePsMode;
	struct STA_RECORD *prStaRec;
	u_int8_t fgIsInPSOld;

	/* DbgPrint("QM:Event -RxBa\n"); */

	prEventStaChangePsMode = (struct EVENT_STA_CHANGE_PS_MODE *)
		(prEvent->aucBuffer);
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter,
		prEventStaChangePsMode->ucStaRecIdx);
	/* ASSERT(prStaRec); */

	if (prStaRec) {
		fgIsInPSOld = qmIsStaInPS(prAdapter, prStaRec);
		qmSetStaPS(prAdapter, prStaRec,
				prEventStaChangePsMode->ucIsInPs);

		qmUpdateFreeQuota(prAdapter,
			prStaRec,
			prEventStaChangePsMode->ucUpdateMode,
			prEventStaChangePsMode->ucFreeQuota);

		DBGLOG(QM, INFO, "PS=%d,%d M:%d Q:%d\n",
			prEventStaChangePsMode->ucStaRecIdx,
			prStaRec->fgIsInPS,
			prEventStaChangePsMode->ucUpdateMode,
			prEventStaChangePsMode->ucFreeQuota);

		/* From PS to Awake */
		if ((fgIsInPSOld) && (!qmIsStaInPS(prAdapter, prStaRec))) {
			if (HAL_IS_TX_DIRECT(prAdapter))
				nicTxDirectStartCheckQTimer(prAdapter);
			else {
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
				prAdapter->rQM.fgForceReassign = TRUE;
#endif
				kalSetEvent(prAdapter->prGlueInfo);
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update STA free quota Event from FW
 *
 * \param[in] prAdapter Adapter pointer
 * \param[in] prEvent The event packet from the FW
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void qmHandleEventStaUpdateFreeQuota(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_STA_UPDATE_FREE_QUOTA
		*prEventStaUpdateFreeQuota;
	struct STA_RECORD *prStaRec;

	prEventStaUpdateFreeQuota = (struct
		EVENT_STA_UPDATE_FREE_QUOTA *) (prEvent->aucBuffer);
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter,
			prEventStaUpdateFreeQuota->ucStaRecIdx);
	/* 2013/08/30
	 * Station Record possible been freed.
	 */
	/* ASSERT(prStaRec); */

	if (prStaRec) {
		if (prStaRec->fgIsInPS) {
			qmUpdateFreeQuota(prAdapter,
				prStaRec,
				prEventStaUpdateFreeQuota->ucUpdateMode,
				prEventStaUpdateFreeQuota->ucFreeQuota);

			if (HAL_IS_TX_DIRECT(prAdapter))
				nicTxDirectStartCheckQTimer(prAdapter);
			else
				kalSetEvent(prAdapter->prGlueInfo);
		}
		DBGLOG(QM, TRACE, "UFQ=%d,%d,%d\n",
			prEventStaUpdateFreeQuota->ucStaRecIdx,
			prEventStaUpdateFreeQuota->ucUpdateMode,
			prEventStaUpdateFreeQuota->ucFreeQuota);

	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update STA free quota
 *
 * \param[in] prStaRec the STA
 * \param[in] ucUpdateMode the method to update free quota
 * \param[in] ucFreeQuota  the value for update
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void
qmUpdateFreeQuota(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint8_t ucUpdateMode,
	uint8_t ucFreeQuota)
{

	uint8_t ucFreeQuotaForNonDelivery;
	uint8_t ucFreeQuotaForDelivery;

	ASSERT(prStaRec);
	DBGLOG(QM, LOUD,
		"qmUpdateFreeQuota orig ucFreeQuota=%d Mode %u New %u\n",
		prStaRec->ucFreeQuota, ucUpdateMode, ucFreeQuota);

	if (!prStaRec->fgIsInPS)
		return;

	switch (ucUpdateMode) {
	case FREE_QUOTA_UPDATE_MODE_INIT:
	case FREE_QUOTA_UPDATE_MODE_OVERWRITE:
		prStaRec->ucFreeQuota = ucFreeQuota;
		break;
	case FREE_QUOTA_UPDATE_MODE_INCREASE:
		prStaRec->ucFreeQuota += ucFreeQuota;
		break;
	case FREE_QUOTA_UPDATE_MODE_DECREASE:
		prStaRec->ucFreeQuota -= ucFreeQuota;
		break;
	default:
		ASSERT(0);
	}

	DBGLOG(QM, LOUD, "qmUpdateFreeQuota new ucFreeQuota=%d)\n",
		prStaRec->ucFreeQuota);

	ucFreeQuota = prStaRec->ucFreeQuota;

	ucFreeQuotaForNonDelivery = 0;
	ucFreeQuotaForDelivery = 0;

	if (ucFreeQuota > 0) {
		if (prStaRec->fgIsQoS && prStaRec->fgIsUapsdSupported
		    /* && prAdapter->rWifiVar.fgSupportQoS */
		    /* && prAdapter->rWifiVar.fgSupportUAPSD */) {
			/* XXX We should assign quota to
			 * aucFreeQuotaPerQueue[NUM_OF_PER_STA_TX_QUEUES]
			 */

			if (prStaRec->ucFreeQuotaForNonDelivery > 0
			    && prStaRec->ucFreeQuotaForDelivery > 0) {
				ucFreeQuotaForNonDelivery = ucFreeQuota >> 1;
				ucFreeQuotaForDelivery = ucFreeQuota -
					ucFreeQuotaForNonDelivery;
			} else if (prStaRec->ucFreeQuotaForNonDelivery == 0
				   && prStaRec->ucFreeQuotaForDelivery == 0) {
				ucFreeQuotaForNonDelivery = ucFreeQuota >> 1;
				ucFreeQuotaForDelivery = ucFreeQuota -
					ucFreeQuotaForNonDelivery;
			} else if (prStaRec->ucFreeQuotaForNonDelivery > 0) {
				/* NonDelivery is not busy */
				if (ucFreeQuota >= 3) {
					ucFreeQuotaForNonDelivery = 2;
					ucFreeQuotaForDelivery = ucFreeQuota -
						ucFreeQuotaForNonDelivery;
				} else {
					ucFreeQuotaForDelivery = ucFreeQuota;
					ucFreeQuotaForNonDelivery = 0;
				}
			} else if (prStaRec->ucFreeQuotaForDelivery > 0) {
				/* Delivery is not busy */
				if (ucFreeQuota >= 3) {
					ucFreeQuotaForDelivery = 2;
					ucFreeQuotaForNonDelivery =
						ucFreeQuota -
						ucFreeQuotaForDelivery;
				} else {
					ucFreeQuotaForNonDelivery = ucFreeQuota;
					ucFreeQuotaForDelivery = 0;
				}
			}

		} else {
			/* !prStaRec->fgIsUapsdSupported */
			ucFreeQuotaForNonDelivery = ucFreeQuota;
			ucFreeQuotaForDelivery = 0;
		}
	}
	/* ucFreeQuota > 0 */
	prStaRec->ucFreeQuotaForDelivery = ucFreeQuotaForDelivery;
	prStaRec->ucFreeQuotaForNonDelivery =
		ucFreeQuotaForNonDelivery;

	DBGLOG(QM, LOUD,
		"new QuotaForDelivery = %d  QuotaForNonDelivery = %d\n",
		prStaRec->ucFreeQuotaForDelivery,
		prStaRec->ucFreeQuotaForNonDelivery);

}
#endif /* CFG_ENABLE_WIFI_DIRECT */
/*----------------------------------------------------------------------------*/
/*!
 * \brief Return the reorder queued RX packets
 *
 * \param[in] (none)
 *
 * \return The number of queued RX packets
 */
/*----------------------------------------------------------------------------*/
uint32_t qmGetRxReorderQueuedBufferCount(struct ADAPTER *prAdapter)
{
	uint32_t i, u4Total;
	struct QUE_MGT *prQM = &prAdapter->rQM;

	u4Total = 0;
	/* XXX The summation may impact the performance */
	for (i = 0; i < CFG_NUM_OF_RX_BA_AGREEMENTS; i++) {
		u4Total += prQM->arRxBaTable[i].rReOrderQue.u4NumElem;
#if DBG && 0
		if (QUEUE_IS_EMPTY(&(prQM->arRxBaTable[i].rReOrderQue)))
			ASSERT(prQM->arRxBaTable[i].rReOrderQue == 0);
#endif
	}
	ASSERT(u4Total <= (CFG_NUM_OF_QM_RX_PKT_NUM * 2));
	return u4Total;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dump current queue status
 *
 * \param[in] (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t qmDumpQueueStatus(struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max)
{
	struct TX_CTRL *prTxCtrl;
	struct QUE_MGT *prQM;
	struct GLUE_INFO *prGlueInfo;
	uint32_t i, u4TotalBufferCount, u4TotalPageCount;
	uint32_t u4CurBufferCount, u4CurPageCount;
	uint32_t u4Len = 0;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prQM = &prAdapter->rQM;
	prGlueInfo = prAdapter->prGlueInfo;
	u4TotalBufferCount = 0;
	u4TotalPageCount = 0;
	u4CurBufferCount = 0;
	u4CurPageCount = 0;

	LOGBUF(pucBuf, u4Max, u4Len, "\n");
	LOGBUF(pucBuf, u4Max, u4Len,
		"=<Dump QUEUE Status>=\n");
	/* For corresponding abbreviation */
	LOGBUF(pucBuf, u4Max, u4Len,
	  "M:Max F:Free PU:PreUsed AQL:AvgQLen mR:minRsv CTR:CurTcRes GTR:GuaranteedTcRes\n");
	LOGBUF(pucBuf, u4Max, u4Len,
		"R:Residual ER:ExtraReserved P:Pending\n");

	for (i = TC0_INDEX; i < TC_NUM; i++) {
		if (!nicTxResourceIsPseCtrlNeeded(prAdapter, i))
			continue;

		LOGBUF(pucBuf, u4Max, u4Len,
			"TC%u ResCnt: M[%02u/%03u] F[%02u/%03u] PU[%03u]\n",
			i, prTxCtrl->rTc.au4MaxNumOfBuffer[i],
			prTxCtrl->rTc.au4MaxNumOfPage[i],
			prTxCtrl->rTc.au4FreeBufferCount[i],
			prTxCtrl->rTc.au4FreePageCount[i],
			prTxCtrl->rTc.au4PreUsedPageCount[i]);

		u4TotalBufferCount += prTxCtrl->rTc.au4MaxNumOfBuffer[i];
		u4TotalPageCount += prTxCtrl->rTc.au4MaxNumOfPage[i];
		u4CurBufferCount += prTxCtrl->rTc.au4FreeBufferCount[i];
		u4CurPageCount += prTxCtrl->rTc.au4FreePageCount[i];
	}

	LOGBUF(pucBuf, u4Max, u4Len,
		"ToT ResCnt: M[%02u/%03u] F[%02u/%03u]\n",
		u4TotalBufferCount, u4TotalPageCount, u4CurBufferCount,
		u4CurPageCount);

	u4TotalBufferCount = 0;
	u4TotalPageCount = 0;
	u4CurBufferCount = 0;
	u4CurPageCount = 0;
	LOGBUF(pucBuf, u4Max, u4Len,
		"=<Dump PLE QUEUE Status>=\n");

	for (i = TC0_INDEX; i < TC_NUM; i++) {
		if (!nicTxResourceIsPseCtrlNeeded(prAdapter, i))
			continue;

		LOGBUF(pucBuf, u4Max, u4Len,
			"TC%u ResCnt: M[%02u/%03u] F[%02u/%03u] PU[%03u]\n",
			i, prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[i],
			prTxCtrl->rTc.au4MaxNumOfPage_PLE[i],
			prTxCtrl->rTc.au4FreeBufferCount_PLE[i],
			prTxCtrl->rTc.au4FreePageCount_PLE[i],
			prTxCtrl->rTc.au4PreUsedPageCount[i]);

		u4TotalBufferCount += prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[i];
		u4TotalPageCount += prTxCtrl->rTc.au4MaxNumOfPage_PLE[i];
		u4CurBufferCount += prTxCtrl->rTc.au4FreeBufferCount_PLE[i];
		u4CurPageCount += prTxCtrl->rTc.au4FreePageCount_PLE[i];
	}

	LOGBUF(pucBuf, u4Max, u4Len,
		"ToT ResCnt: M[%02u/%03u] F[%02u/%03u]\n",
		u4TotalBufferCount, u4TotalPageCount, u4CurBufferCount,
		u4CurPageCount);

	LOGBUF(pucBuf, u4Max, u4Len,
		"===\n");

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	for (i = TC0_INDEX; i < TC_NUM; i++) {
		if (!NIC_TX_RES_IS_ACTIVE(prAdapter, i))
			continue;

		LOGBUF(pucBuf, u4Max, u4Len,
			"TC%u AQL[%04u] mR[%02u] CTR[%02u] GTR[%02u]\n",
			i, QM_GET_TX_QUEUE_LEN(prAdapter, i),
			prQM->au4MinReservedTcResource[i],
			prQM->au4CurrentTcResource[i],
			prQM->au4GuaranteedTcResource[i]);
	}

	LOGBUF(pucBuf, u4Max, u4Len,
		"Res R[%u] ER[%u]\n",
		prQM->u4ResidualTcResource,
		prQM->u4ExtraReservedTcResource);
	LOGBUF(pucBuf, u4Max, u4Len,
		"QueLenMovingAvg[%u] Time2AdjResource[%u] Time2UpdateQLen[%u]\n",
		prQM->u4QueLenMovingAverage, prQM->u4TimeToAdjustTcResource,
		prQM->u4TimeToUpdateQueLen);
#endif

	DBGLOG(SW4, INFO, "===\n");

#if QM_FORWARDING_FAIRNESS
	for (i = 0; i < NUM_OF_PER_STA_TX_QUEUES; i++) {
		LOGBUF(pucBuf, u4Max, u4Len,
			"TC%u HeadSta[%u] ResUsedCnt[%u]\n",
			i, prQM->au4HeadStaRecIndex[i],
			prQM->au4ResourceUsedCount[i]);
	}
#endif

	LOGBUF(pucBuf, u4Max, u4Len,
		"BMC or unknown TxQueue Len[%u]\n",
		prQM->arTxQueue[0].u4NumElem);
	LOGBUF(pucBuf, u4Max, u4Len,
		"P QLen Normal[%u] Cmd[%u]\n",
		GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
		GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingCmdNum));

#if QM_TC_RESOURCE_EMPTY_COUNTER
	for (i = TC0_INDEX; i < TC_NUM; i++) {
		if (!NIC_TX_RES_IS_ACTIVE(prAdapter, i)
			|| !prQM->au4DequeueNoTcResourceCounter[i])
			break;
		LOGBUF(pucBuf, u4Max, u4Len,
			"TC%u TcNoResCnt: [%02u]\n",
			i, prQM->au4DequeueNoTcResourceCounter[i]);
	}
#endif

#if defined(LINUX)
	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		LOGBUF(pucBuf, u4Max, u4Len,
			"P BSS[%u] QLen[%u:%u:%u:%u]\n", i,
			prGlueInfo->ai4TxPendingFrameNumPerQueue[i][0],
			prGlueInfo->ai4TxPendingFrameNumPerQueue[i][1],
			prGlueInfo->ai4TxPendingFrameNumPerQueue[i][2],
			prGlueInfo->ai4TxPendingFrameNumPerQueue[i][3]);
	}
#endif
	LOGBUF(pucBuf, u4Max, u4Len, "P FWD CNT[%d]\n",
		prTxCtrl->i4PendingFwdFrameCount);
	LOGBUF(pucBuf, u4Max, u4Len, "P MGMT CNT[%d]\n",
		prTxCtrl->i4TxMgmtPendingNum);

	LOGBUF(pucBuf, u4Max, u4Len,
		"===\n");

	LOGBUF(pucBuf, u4Max, u4Len, "Total RFB[%u]\n",
		CFG_RX_MAX_PKT_NUM);
	LOGBUF(pucBuf, u4Max, u4Len, "FSwRfbList[%u]\n",
		RX_GET_FREE_RFB_CNT(&prAdapter->rRxCtrl));
	LOGBUF(pucBuf, u4Max, u4Len, "RecRfbList[%u]\n",
		RX_GET_RECEIVED_RFB_CNT(&prAdapter->rRxCtrl));
	LOGBUF(pucBuf, u4Max, u4Len, "IndicatedRfbList[%u]\n",
		RX_GET_INDICATED_RFB_CNT(&prAdapter->rRxCtrl));
	LOGBUF(pucBuf, u4Max, u4Len, "NumIndPacket[%u]\n",
		prAdapter->rRxCtrl.ucNumIndPacket);
	LOGBUF(pucBuf, u4Max, u4Len, "NumRetainedPacket[%u]\n",
		prAdapter->rRxCtrl.ucNumRetainedPacket);
#if CFG_SUPPORT_MULTITHREAD
	LOGBUF(pucBuf, u4Max, u4Len,
		"===\n");
	LOGBUF(pucBuf, u4Max, u4Len,
		"CMD: F[%u/%u] PQ[%u] CQ[%u] TCQ[%u] TCDQ[%u]\n",
		prAdapter->rFreeCmdList.u4NumElem,
		CFG_TX_MAX_CMD_PKT_NUM,
		prAdapter->rPendingCmdQueue.u4NumElem,
		prGlueInfo->rCmdQueue.u4NumElem,
		prAdapter->rTxCmdQueue.u4NumElem,
		prAdapter->rTxCmdDoneQueue.u4NumElem);
	LOGBUF(pucBuf, u4Max, u4Len,
		"MSDU: F[%u/%u] P[%u] Done[%u]\n",
		prAdapter->rTxCtrl.rFreeMsduInfoList.u4NumElem,
		CFG_TX_MAX_PKT_NUM,
		prAdapter->rTxCtrl.rTxMgmtTxingQueue.u4NumElem,
		prAdapter->rTxDataDoneQueue.u4NumElem);

	LOGBUF(pucBuf, u4Max, u4Len,
		"===\n");

	if (HAL_IS_TX_DIRECT(prAdapter)) {
		LOGBUF(pucBuf, u4Max, u4Len,
			"TxDirect : SkbQ[%d] HifQ",
			kalGetTxDirectQueueLength(prAdapter->prGlueInfo));
		LOGBUF(pucBuf, u4Max, u4Len,
			"[%u:%u:%u:%u]\n",
			prAdapter->rTxDirectHifQueue[0].u4NumElem,
			prAdapter->rTxDirectHifQueue[1].u4NumElem,
			prAdapter->rTxDirectHifQueue[2].u4NumElem,
			prAdapter->rTxDirectHifQueue[3].u4NumElem);
		LOGBUF(pucBuf, u4Max, u4Len,
			"===\n");
	}

	LOGBUF(pucBuf, u4Max, u4Len,
		"===\n");
	if (prGlueInfo->rCmdQueue.u4NumElem > 0)
		cmdBufDumpCmdQueue(&prGlueInfo->rCmdQueue,
				"waiting Tx CMD queue");
#endif
	return u4Len;
}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
void qmResetTcControlResource(struct ADAPTER *prAdapter)
{
	uint32_t u4Idx;
	uint32_t u4TotalMinReservedTcResource = 0;
	uint32_t u4TotalTcResource = 0;
	uint32_t u4TotalGurantedTcResource = 0;
	struct QUE_MGT *prQM = &prAdapter->rQM;
	struct TX_TCQ_STATUS *prTc =
		&prAdapter->rTxCtrl.rTc;

	/* Initialize TC resource control variables */
	for (u4Idx = 0; u4Idx < TC_NUM; u4Idx++)
		prQM->au4AverageQueLen[u4Idx] = 0;

	ASSERT(prQM->u4TimeToAdjustTcResource
	       && prQM->u4TimeToUpdateQueLen);

	for (u4Idx = 0; u4Idx < TC_NUM; u4Idx++) {
		if (!NIC_TX_RES_IS_ACTIVE(prAdapter, u4Idx))
			continue;

		prQM->au4CurrentTcResource[u4Idx] =
			prTc->au4MaxNumOfBuffer[u4Idx];

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		if (!nicTxResourceIsPseCtrlNeeded(prAdapter, u4Idx)
			&& nicTxResourceIsPleCtrlNeeded(prAdapter, u4Idx))
			/* Overwrite PLE-cnt only if PSE-disable+PLE-enable */
			prQM->au4CurrentTcResource[u4Idx] =
				prTc->au4MaxNumOfBuffer_PLE[u4Idx];
#endif

		if (u4Idx != TC4_INDEX) {
			u4TotalTcResource += prQM->au4CurrentTcResource[u4Idx];
			u4TotalGurantedTcResource +=
				prQM->au4GuaranteedTcResource[u4Idx];
			u4TotalMinReservedTcResource +=
				prQM->au4MinReservedTcResource[u4Idx];
		}
	}

	/* Sanity Check */
	if (u4TotalMinReservedTcResource > u4TotalTcResource)
		kalMemZero(prQM->au4MinReservedTcResource,
			   sizeof(prQM->au4MinReservedTcResource));

	if (u4TotalGurantedTcResource > u4TotalTcResource)
		kalMemZero(prQM->au4GuaranteedTcResource,
			   sizeof(prQM->au4GuaranteedTcResource));

	u4TotalGurantedTcResource = 0;

	/* Initialize Residual TC resource */
	for (u4Idx = 0; u4Idx < TC_NUM; u4Idx++) {
	/* Add for prevent if TC num is not match will result
	*  u4TotalGurantedTcResource > u4TotalTcResource and
	*  u4ResidualTcResource value is negative (will result
	*  resource control error).
	*/
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
		if (!nicTxResourceIsPleCtrlNeeded(prAdapter, u4Idx))
			continue;
#endif
		if (prQM->au4GuaranteedTcResource[u4Idx] <
		    prQM->au4MinReservedTcResource[u4Idx])
			prQM->au4GuaranteedTcResource[u4Idx] =
				prQM->au4MinReservedTcResource[u4Idx];

		if (u4Idx != TC4_INDEX)
			u4TotalGurantedTcResource +=
				prQM->au4GuaranteedTcResource[u4Idx];
	}

	prQM->u4ResidualTcResource = u4TotalTcResource -
		u4TotalGurantedTcResource;

}
#endif

/* To change PN number to UINT64 */
u_int8_t qmRxPNtoU64(uint8_t *pucPN, uint8_t uPNNum,
	uint64_t *pu64Rets)
{
	uint8_t ucCount = 0;
	uint64_t u64Data = 0;

	if (!pu64Rets) {
		DBGLOG(QM, ERROR, "Please input valid pu8Rets\n");
		return FALSE;
	}

	if (uPNNum > CCMPTSCPNNUM) {
		DBGLOG(QM, ERROR, "Please input valid uPNNum:%d\n", uPNNum);
		return FALSE;
	}

	*pu64Rets = 0;
	for (; ucCount < uPNNum; ucCount++) {
		u64Data = ((uint64_t) pucPN[ucCount]) << (8 * ucCount);
		*pu64Rets +=  u64Data;
	}
	return TRUE;
}

#ifdef CFG_SUPPORT_REPLAY_DETECTION
/* To check PN/TSC between RxStatus and local record.
 * return TRUE if PNS is not bigger than PNT
 */
u_int8_t qmRxDetectReplay(uint8_t *pucPNS, uint8_t *pucPNT)
{
	uint64_t u8RxNum = 0;
	uint64_t u8LocalRec = 0;

	if (!pucPNS || !pucPNT) {
		DBGLOG(QM, ERROR, "Please input valid PNS:%p and PNT:%p\n",
			pucPNS, pucPNT);
		return TRUE;
	}

	if (!qmRxPNtoU64(pucPNS, CCMPTSCPNNUM, &u8RxNum)
	    || !qmRxPNtoU64(pucPNT, CCMPTSCPNNUM, &u8LocalRec)) {
		DBGLOG(QM, ERROR, "PN2U64 failed\n");
		return TRUE;
	}
	/* PN overflow ? */
	return !(u8RxNum > u8LocalRec);
}

/* TO filter broadcast and multicast data packet replay issue. */
u_int8_t qmHandleRxReplay(struct ADAPTER *prAdapter,
			  struct SW_RFB *prSwRfb)
{
	uint8_t *pucPN = NULL;
	uint8_t ucKeyID = 0;				/* 0~4 */
	/* CIPHER_SUITE_NONE~CIPHER_SUITE_GCMP */
	uint8_t ucSecMode = CIPHER_SUITE_NONE;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_WPA_INFO *prWpaInfo = NULL;
	struct GL_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;

	if (!prAdapter)
		return TRUE;
	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return TRUE;

	if (!(prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_1))) {
		DBGLOG_LIMITED(QM, TRACE, "Group 1 invalid\n");
		return FALSE;
	}

	/* BMC only need check CCMP and TKIP Cipher suite */
	ucSecMode = prSwRfb->ucSecMode;

	prGlueInfo = prAdapter->prGlueInfo;

	prWpaInfo = aisGetWpaInfo(prAdapter,
		secGetBssIdxByRfb(prAdapter, prSwRfb));

	DBGLOG_LIMITED(QM, TRACE, "ucSecMode = [%u], ChiperGroup = [%u]\n",
			ucSecMode, prWpaInfo->u4CipherGroup);

	if (ucSecMode != CIPHER_SUITE_CCMP
	    && ucSecMode != CIPHER_SUITE_TKIP) {
		DBGLOG_LIMITED(QM, TRACE,
			"SecMode: %d and CipherGroup: %d, no need check replay\n",
			ucSecMode, prWpaInfo->u4CipherGroup);
		return FALSE;
	}

	if (!(prWpaInfo->u4CipherGroup &
		(IW_AUTH_CIPHER_TKIP | IW_AUTH_CIPHER_CCMP))) {
		DBGLOG(QM, ERROR,
			"RX status Chipher mode doens't match AP's setting\n");
		return FALSE;
	}

	ucKeyID = prSwRfb->ucKeyID;
	if (ucKeyID >= MAX_KEY_NUM) {
		DBGLOG(QM, ERROR, "KeyID: %d error\n", ucKeyID);
		return TRUE;
	}

	prDetRplyInfo = aisGetDetRplyInfo(prAdapter,
		secGetBssIdxByRfb(prAdapter, prSwRfb));
	/* TODO : Need check fw rekey while fw rekey event. */
	if (ucKeyID != prDetRplyInfo->ucCurKeyId) {
		DBGLOG(QM, TRACE,
			"use last keyID while detect replay information.(0x%x->0x%x)\n",
			prDetRplyInfo->ucCurKeyId, ucKeyID);
		ucKeyID = prDetRplyInfo->ucCurKeyId;
	}

	if (prDetRplyInfo->arReplayPNInfo[ucKeyID].fgFirstPkt) {
		prDetRplyInfo->arReplayPNInfo[ucKeyID].fgFirstPkt = FALSE;
		HAL_RX_STATUS_GET_PN(prSwRfb->prRxStatusGroup1,
			prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN);
		DBGLOG(QM, INFO,
			"First check packet. Key ID:0x%x\n", ucKeyID);
		return FALSE;
	}

	pucPN = prSwRfb->prRxStatusGroup1->aucPN;
	DBGLOG_LIMITED(QM, TRACE,
		"[KID=%d] BC packet 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x--0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
		ucKeyID,
		pucPN[0], pucPN[1], pucPN[2], pucPN[3], pucPN[4], pucPN[5],
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN[0],
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN[1],
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN[2],
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN[3],
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN[4],
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN[5]);
	if (qmRxDetectReplay(pucPN,
			prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN)) {
		DBGLOG_LIMITED(QM, WARN, "Drop BC replay packet!\n");
		return TRUE;
	}

	HAL_RX_STATUS_GET_PN(prSwRfb->prRxStatusGroup1,
		prDetRplyInfo->arReplayPNInfo[ucKeyID].auPN);
	return FALSE;
}
#endif

u_int8_t
qmIsIPLayerPacket(uint8_t *pucPkt)
{
	uint16_t u2EtherType =
		(pucPkt[ETH_TYPE_LEN_OFFSET] << 8)
			| (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);

	if (u2EtherType == ETH_P_IPV4 || u2EtherType == ETH_P_IPV6) {
		uint8_t *pucEthBody = &pucPkt[ETH_HLEN];
		uint8_t ucIpProto =
			(u2EtherType == ETH_P_IPV4 ?
				pucEthBody[IP_PROTO_HLEN] :
				pucEthBody[IPV6_HDR_PROTOCOL_OFFSET]);

		if (ucIpProto == IP_PRO_UDP || ucIpProto == IP_PRO_TCP)
			return TRUE;
	}

	return FALSE;
}

u_int8_t
qmIsNoDropPacket(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	uint8_t *pucData = (uint8_t *) prSwRfb->pvHeader;
	uint8_t ucBssIndex
		= secGetBssIdxByWlanIdx(prAdapter, prSwRfb->ucWlanIdx);
#if ((CFG_SUPPORT_LOWLATENCY_MODE == 1) || (CFG_SUPPORT_OSHARE == 1) || \
	(CFG_MTK_MDDP_SUPPORT == 1))
	u_int8_t fgCheckDrop = FALSE;
#endif
	struct BSS_INFO *prBssInfo = NULL;

	if (!pucData)
		return FALSE;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return FALSE;

	if (ucBssIndex <= MAX_BSSID_NUM)
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

#if CFG_SUPPORT_LOWLATENCY_MODE
	if (prAdapter->fgEnLowLatencyMode)
		fgCheckDrop = TRUE;
#endif

#if CFG_SUPPORT_OSHARE
	if (!fgCheckDrop &&
		(prAdapter->fgEnOshareMode) &&
		prBssInfo &&
		prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
		fgCheckDrop = TRUE;
#endif

#if CFG_MTK_MDDP_SUPPORT
	if (!fgCheckDrop && mddpIsSupportMddpWh() &&
	    prAdapter->fgMddpActivated &&
	    prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
		struct WIFI_VAR *prWifiVar = NULL;
		struct P2P_CONNECTION_SETTINGS *prP2PConnSettings = NULL;

		prWifiVar = &prAdapter->rWifiVar;
		if (prWifiVar && prBssInfo->u4PrivateData < BSS_P2P_NUM) {
			prP2PConnSettings = prWifiVar->prP2PConnSettings[
				prBssInfo->u4PrivateData];
			fgCheckDrop = prP2PConnSettings &&
				p2pFuncIsAPMode(prP2PConnSettings);
		}
	}
#endif

#if ((CFG_SUPPORT_LOWLATENCY_MODE == 1) || (CFG_SUPPORT_OSHARE == 1) || \
	(CFG_MTK_MDDP_SUPPORT == 1))
	if (fgCheckDrop && qmIsIPLayerPacket(pucData))
		return TRUE;
#endif

	/* For some special packet, like DNS, DHCP,
	 * do not drop evan fall behind.
	 */
	if (qmIsIndependentPkt(prSwRfb))
		return TRUE;

	return FALSE;
}

void qmMoveStaTxQueue(struct ADAPTER *prAdapter,
	struct STA_RECORD *prSrcStaRec, struct STA_RECORD *prDstStaRec)
{
	uint8_t ucQueArrayIdx;
	struct QUE *prSrcQue = NULL;
	struct QUE *prDstQue = NULL;
	struct MSDU_INFO *prMsduInfo = NULL;
	uint8_t ucDstStaIndex = 0;

	if (!prSrcStaRec || !prDstStaRec)
		return;

	prSrcQue = &prSrcStaRec->arTxQueue[0];
	prDstQue = &prDstStaRec->arPendingTxQueue[0];
	ucDstStaIndex = prDstStaRec->ucIndex;

	DBGLOG(QM, INFO, "Move Pending MSDUs STA[%u->%u] TC[%u %u %u %u]\n",
		prSrcStaRec->ucIndex, ucDstStaIndex,
		prSrcQue[TC0_INDEX].u4NumElem, prSrcQue[TC1_INDEX].u4NumElem,
		prSrcQue[TC2_INDEX].u4NumElem, prSrcQue[TC3_INDEX].u4NumElem);
	/* Concatenate all MSDU_INFOs in TX queues of this STA_REC */
	for (ucQueArrayIdx = 0; ucQueArrayIdx < TC4_INDEX; ucQueArrayIdx++) {
		prMsduInfo = QUEUE_GET_HEAD(&prSrcQue[ucQueArrayIdx]);
		while (prMsduInfo) {
			prMsduInfo->ucStaRecIndex = ucDstStaIndex;
			prMsduInfo = QUEUE_GET_NEXT_ENTRY(
					&prMsduInfo->rQueEntry);
		}
		QUEUE_CONCATENATE_QUEUES((&prDstQue[ucQueArrayIdx]),
					 (&prSrcQue[ucQueArrayIdx]));
	}

	if (HAL_IS_TX_DIRECT(prAdapter)) {
		nicTxDirectMoveStaAcmQ(
			prAdapter, ucDstStaIndex, prSrcStaRec->ucIndex);
		nicTxDirectMoveStaPendQ(
			prAdapter, ucDstStaIndex, prSrcStaRec->ucIndex);
		nicTxDirectMoveStaPsQ(
			prAdapter, ucDstStaIndex, prSrcStaRec->ucIndex);
		nicTxDirectMoveBssAbsentQ(
			prAdapter, prSrcStaRec->ucBssIndex,
			ucDstStaIndex, prSrcStaRec->ucIndex);
	}
}

void qmHandleDelTspec(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
		      enum ENUM_ACI eAci)
{
	uint8_t aucNextUP[ACI_NUM] = {1 /* BEtoBK */, 1 /*na */, 0 /*VItoBE */,
				      4 /*VOtoVI */};
	enum ENUM_ACI aeNextAci[ACI_NUM] = {ACI_BK, ACI_BK, ACI_BE, ACI_VI};
	uint8_t ucActivedTspec = 0;
	uint8_t ucNewUp = 0;
	struct QUE *prSrcQue = NULL;
	struct QUE *prDstQue = NULL;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct AC_QUE_PARMS *prAcQueParam = NULL;
	uint8_t ucTc = 0;
	struct BSS_INFO *prAisBssInfo = NULL;

	if (eAci >= ACI_NUM)
		return;

	if (!prStaRec || eAci == ACI_NUM || eAci == ACI_BK || !prAdapter) {
		DBGLOG(QM, ERROR, "prSta NULL %d, eAci %d, prAdapter NULL %d\n",
		       !prStaRec, eAci, !prAdapter);
		return;
	}
	prAisBssInfo = aisGetAisBssInfo(prAdapter,
		prStaRec->ucBssIndex);
	if (!prAisBssInfo) {
		DBGLOG(QM, ERROR, "prAisBssInfo NULL\n");
		return;
	}

	prSrcQue = &prStaRec->arTxQueue[aucWmmAC2TcResourceSet1[eAci]];
	prAcQueParam = &(prAisBssInfo->arACQueParms[0]);
	ucActivedTspec = nicGetActiveTspec(prAdapter, prAisBssInfo->ucBssIndex);

	while (prAcQueParam[eAci].ucIsACMSet &&
			!(ucActivedTspec & BIT(eAci)) && eAci != ACI_BK
			&& eAci < ACI_NUM) {
		eAci = aeNextAci[eAci];
		if (eAci < ACI_NUM)
			ucNewUp = aucNextUP[eAci];
	}
	if (eAci >= ACI_NUM)
		return;
	DBGLOG(QM, INFO, "new ACI %d, ACM %d, HasTs %d\n", eAci,
	       prAcQueParam[eAci].ucIsACMSet, !!(ucActivedTspec & BIT(eAci)));
	ucTc = aucWmmAC2TcResourceSet1[eAci];
	prDstQue = &prStaRec->arTxQueue[ucTc];
	prMsduInfo = QUEUE_GET_HEAD(prSrcQue);
	while (prMsduInfo) {
		prMsduInfo->ucUserPriority = ucNewUp;
		prMsduInfo->ucTC = ucTc;
		prMsduInfo = QUEUE_GET_NEXT_ENTRY(&prMsduInfo->rQueEntry);
	}
	QUEUE_CONCATENATE_QUEUES(prDstQue, prSrcQue);
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	qmUpdateAverageTxQueLen(prAdapter);
	qmReassignTcResource(prAdapter);
#endif
	nicTxAdjustTcq(prAdapter);
	kalSetEvent(prAdapter->prGlueInfo);
}

void qmReleaseCHAtFinishedDhcp(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo = (struct AIS_FSM_INFO *) NULL;

	if (prAdapter == NULL)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (prBssInfo == NULL)
		return;

	if (IS_BSS_AIS(prBssInfo) && ucBssIndex < MAX_BSSID_NUM) { /* STA */
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

		if (!timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer)) {
			DBGLOG(QM, ERROR, "No channel occupation\n");
		} else {
			DBGLOG(QM, INFO, "Dhcp done, stop join timer.\n");
			cnmTimerStopTimer(prAdapter,
				&prAisFsmInfo->rJoinTimeoutTimer);
			aisFsmRunEventJoinTimeout(prAdapter, ucBssIndex);
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (IS_BSS_P2P(prBssInfo)) { /* GC */
		DBGLOG(QM, INFO, "Dhcp done, stop GC join timer\n");
		p2pRoleFsmNotifyDhcpDone(prAdapter, ucBssIndex);
	}
#endif
}

void qmHandleRxReorderWinShift(struct ADAPTER *prAdapter,
	uint8_t ucStaRecIdx, uint8_t ucTid, uint16_t u2SSN,
	struct QUE *prReturnedQue)
{
	struct STA_RECORD *prStaRec;
	struct RX_BA_ENTRY *prReorderQueParm;
	uint16_t u2WinStart;
	uint16_t u2WinEnd;

	/* Check whether the STA_REC is activated */
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);
	if (prStaRec == NULL) {
		/* ASSERT(prStaRec); */
		return;
	}

	/* Check whether the BA agreement exists */
	prReorderQueParm = prStaRec->aprRxReorderParamRefTbl[ucTid];
	if (!prReorderQueParm) {
		/* TODO: (Tehuang) Handle the Host-FW sync issue. */
		DBGLOG(QM, WARN,
			"QM: (Warning) BAR for a NULL ReorderQueParm\n");
		/* ASSERT(0); */
		return;
	}

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_LOCK(prAdapter->prGlueInfo, 0);

	u2WinStart = prReorderQueParm->u2WinStart;
	u2WinEnd = prReorderQueParm->u2WinEnd;

	if (SEQ_SMALLER(u2WinStart, u2SSN)) {
#if CFG_SUPPORT_RX_OOR_BAR
		prReorderQueParm->u2BarSSN = u2SSN;
		DBGLOG(RX, INFO,
			"BAR: update %u:%u WinStart %u -> %u, LRcvdSN=%u, B=%u",
			prReorderQueParm->ucStaRecIdx,
			prReorderQueParm->ucTid,
			prReorderQueParm->u2WinStart,
			prReorderQueParm->u2BarSSN,
			prReorderQueParm->u2LastRcvdSN,
			prReorderQueParm->u2FirstBubbleSn);
		SET_BAR_SSN_VALID(prReorderQueParm);
#endif /* CFG_SUPPORT_RX_OOR_BAR */

		prReorderQueParm->u2WinStart = u2SSN;
		prReorderQueParm->u2WinEnd =
			SEQ_ADD(prReorderQueParm->u2WinStart,
				prReorderQueParm->u2WinSize - 1);

#if CFG_SUPPORT_RX_AMSDU
		/* RX reorder for one MSDU in AMSDU issue */
		prReorderQueParm->ucLastAmsduSubIdx = RX_PAYLOAD_FORMAT_MSDU;
#endif

		DBGLOG(RX, TEMP,
			"QM:(BAR U)[%d](%u){%hu,%hu}\n",
			ucTid, u2SSN,
			prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinEnd);
		qmPopOutDueToFallAhead(prAdapter, prReorderQueParm,
			prReturnedQue);
	} else {
		DBGLOG(RX, TEMP, "QM:(BAR I)(%d)(%u){%u,%u}\n",
			ucTid, u2SSN, u2WinStart, u2WinEnd);
	}

	if (HAL_IS_RX_DIRECT(prAdapter))
		RX_DIRECT_REORDER_UNLOCK(prAdapter->prGlueInfo, 0);

}

void qmCheckRxEAPOLM3(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb, uint8_t ucBssIndex)
{
	uint8_t *pPkt = NULL;
	uint16_t u2EtherType;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return;

	pPkt = prSwRfb->pvHeader;
	if (!pPkt)
		return;

	if (!prSwRfb->pvPacket)
		return;

	/* get ethernet protocol */
	u2EtherType = (pPkt[ETH_TYPE_LEN_OFFSET] << 8)
			| (pPkt[ETH_TYPE_LEN_OFFSET + 1]);

	prAdapter->fgIsPostponeTxEAPOLM3 = FALSE;

	if (u2EtherType == ETH_P_1X) {
		uint8_t *pucEthBody = &pPkt[ETH_HLEN];
		uint8_t *pucEapol = pucEthBody;
		uint8_t ucEapolType = pucEapol[1];
		uint16_t u2KeyInfo = 0;
		uint8_t m;

		if (ucEapolType == ETH_EAPOL_KEY) {
			WLAN_GET_FIELD_BE16(&pucEapol[5], &u2KeyInfo);
			m = ((u2KeyInfo & 0x1100) == 0x0000 ||
				(u2KeyInfo & 0x0008) == 0x0000) ? 1 : 3;

			if (prAdapter->rWifiVar.u4SwTestMode ==
					ENUM_SW_TEST_MODE_SIGMA_HS20_R2 &&
					m == 3 &&
					!prSwRfb->prStaRec->fgIsTxKeyReady) {
				prAdapter->fgIsPostponeTxEAPOLM3 = TRUE;
				DBGLOG(QM, INFO,
					"[Passpoint] Postpone sending EAPOL M4 until PTK installed!");
			}
		}
	}
}

u_int8_t qmIsStaInPS(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec)
{
	u_int8_t fgIsInPS = prStaRec->fgIsInPS;
#if CFG_SUPPORT_802_11BE_MLO
	struct MLD_STA_RECORD *prMldStarec;

	if (!fgIsInPS)
		return FALSE;

	prMldStarec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStarec)
		return fgIsInPS;

	if ((prMldStarec->u4StaBitmap & prAdapter->u4StaInPSBitmap) ==
		prMldStarec->u4StaBitmap)
		return TRUE;

	return FALSE;
#else /* CFG_SUPPORT_802_11BE_MLO */
	return fgIsInPS;
#endif /* CFG_SUPPORT_802_11BE_MLO */
}

void qmSetStaPS(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	u_int8_t fgIsInPS)
{
	prStaRec->fgIsInPS = fgIsInPS;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (fgIsInPS)
		prAdapter->u4StaInPSBitmap |= BIT(prStaRec->ucIndex);
	else
		prAdapter->u4StaInPSBitmap &= ~BIT(prStaRec->ucIndex);
#endif
}
