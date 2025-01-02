/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/nic/nic_tx.c#2
 */

/*! \file   nic_tx.c
 *    \brief  Functions that provide TX operation in NIC Layer.
 *
 *    This file provides TX functions which are responsible for both Hardware
 *    and Software Resource Management and keep their Synchronization.
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
#include "que_mgt.h"
#if CFG_SUPPORT_NAN
#include "nan_txm.h"
#endif

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

PFN_TX_DATA_DONE_CB g_pfTxDataDoneCb = nicTxMsduDoneCb;

static const struct TX_RESOURCE_CONTROL
	arTcResourceControl[TC_NUM] = {
	/* dest port index, dest queue index,   HIF TX queue index */
	/* First HW queue */
	{PORT_INDEX_LMAC, MAC_TXQ_AC0_INDEX, HIF_TX_AC0_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC1_INDEX, HIF_TX_AC1_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC2_INDEX, HIF_TX_AC2_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC3_INDEX, HIF_TX_AC3_INDEX},
	{PORT_INDEX_MCU, MCU_Q0_INDEX, HIF_TX_CPU_INDEX},

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	{PORT_INDEX_LMAC, MAC_TXQ_AC10_INDEX, HIF_TX_AC10_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC11_INDEX, HIF_TX_AC11_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC12_INDEX, HIF_TX_AC12_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC13_INDEX, HIF_TX_AC13_INDEX},

	{PORT_INDEX_LMAC, MAC_TXQ_AC20_INDEX, HIF_TX_AC20_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC21_INDEX, HIF_TX_AC21_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC22_INDEX, HIF_TX_AC22_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC23_INDEX, HIF_TX_AC23_INDEX},

	{PORT_INDEX_LMAC, MAC_TXQ_AC30_INDEX, HIF_TX_AC3X_INDEX},
#endif

	/* Second HW queue */
#if NIC_TX_ENABLE_SECOND_HW_QUEUE
	{PORT_INDEX_LMAC, MAC_TXQ_AC10_INDEX, HIF_TX_AC10_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC11_INDEX, HIF_TX_AC11_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC12_INDEX, HIF_TX_AC12_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC13_INDEX, HIF_TX_AC13_INDEX},
	{PORT_INDEX_LMAC, MAC_TXQ_AC11_INDEX, HIF_TX_AC11_INDEX},
#endif
};

/* Traffic settings per TC */
static const struct TX_TC_TRAFFIC_SETTING
	arTcTrafficSettings[NET_TC_NUM] = {
	/* Tx desc template format, Remaining Tx time,
	 * Retry count
	 */
	/* For Data frame with StaRec,
	 * set Long Format to enable the following settings
	 */
	{
		NIC_TX_DESC_LONG_FORMAT_LENGTH, NIC_TX_AC_BE_REMAINING_TX_TIME,
		NIC_TX_DATA_DEFAULT_RETRY_COUNT_LIMIT
	},
	{
		NIC_TX_DESC_LONG_FORMAT_LENGTH, NIC_TX_AC_BK_REMAINING_TX_TIME,
		NIC_TX_DATA_DEFAULT_RETRY_COUNT_LIMIT
	},
	{
		NIC_TX_DESC_LONG_FORMAT_LENGTH, NIC_TX_AC_VI_REMAINING_TX_TIME,
		NIC_TX_DATA_DEFAULT_RETRY_COUNT_LIMIT
	},
	{
		NIC_TX_DESC_LONG_FORMAT_LENGTH, NIC_TX_AC_VO_REMAINING_TX_TIME,
		NIC_TX_DATA_DEFAULT_RETRY_COUNT_LIMIT
	},

	/* MGMT frame */
	{
		NIC_TX_DESC_LONG_FORMAT_LENGTH, NIC_TX_MGMT_REMAINING_TX_TIME,
		NIC_TX_MGMT_DEFAULT_RETRY_COUNT_LIMIT
	},

	/* non-StaRec frame (BMC, etc...) */
	{
		NIC_TX_DESC_LONG_FORMAT_LENGTH, NIC_TX_BMC_REMAINING_TX_TIME,
		NIC_TX_DATA_DEFAULT_RETRY_COUNT_LIMIT
	},
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

static uint8_t *apucTxResultStr[TX_RESULT_NUM] = {
	(uint8_t *) DISP_STRING("SUCCESS"),		/* success */
	(uint8_t *) DISP_STRING("LIFE_TO"),		/* life timeout */
	(uint8_t *) DISP_STRING("RTS_ER"),		/* RTS error */
	(uint8_t *) DISP_STRING("MPDU_ER"),		/* MPDU error */
	(uint8_t *) DISP_STRING("AGE_TO"),		/* aging timeout */
	(uint8_t *) DISP_STRING("FLUSHED"),		/* flushed */
	(uint8_t *) DISP_STRING("BIP_ER"),		/* BIP error */
	(uint8_t *) DISP_STRING("UNSPEC_ER"),		/* unspecified error */
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) NULL,
	(uint8_t *) DISP_STRING("DP_IN_DRV"),		/* drop in driver */
	(uint8_t *) DISP_STRING("DP_IN_FW"),		/* drop in FW */
	(uint8_t *) DISP_STRING("QUE_CLR"),		/* queue clearance */
	(uint8_t *) DISP_STRING("INACT_BSS")		/* inactive BSS */
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will initial all variables in regard to SW TX Queues and
 *        all free lists of MSDU_INFO_T and SW_TFCB_T.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxInitialize(IN struct ADAPTER *prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t *pucMemHandle;
	struct MSDU_INFO *prMsduInfo;
	uint32_t i;

	KAL_SPIN_LOCK_DECLARATION();

	DEBUGFUNC("nicTxInitialize");

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

	/* 4 <1> Initialization of Traffic Class Queue Parameters */
	nicTxResetResource(prAdapter);

	prTxCtrl->pucTxCoalescingBufPtr =
		prAdapter->pucCoalescingBufCached;

	prTxCtrl->u4WrIdx = 0;

	/* allocate MSDU_INFO_T and link it into rFreeMsduInfoList */
	QUEUE_INITIALIZE(&prTxCtrl->rFreeMsduInfoList);

	pucMemHandle = prTxCtrl->pucTxCached;
	for (i = 0; i < CFG_TX_MAX_PKT_NUM; i++) {
		prMsduInfo = (struct MSDU_INFO *) pucMemHandle;
		kalMemZero(prMsduInfo, sizeof(struct MSDU_INFO));

		KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				      SPIN_LOCK_TX_MSDU_INFO_LIST);
		QUEUE_INSERT_TAIL(&prTxCtrl->rFreeMsduInfoList,
				  (struct QUE_ENTRY *) prMsduInfo);
		KAL_RELEASE_SPIN_LOCK(prAdapter,
				      SPIN_LOCK_TX_MSDU_INFO_LIST);

		pucMemHandle += ALIGN_4(sizeof(struct MSDU_INFO));
	}

	ASSERT(prTxCtrl->rFreeMsduInfoList.u4NumElem ==
	       CFG_TX_MAX_PKT_NUM);
	/* Check if the memory allocation consist
	 * with this initialization function
	 */
	ASSERT((uint32_t) (pucMemHandle - prTxCtrl->pucTxCached) ==
	       prTxCtrl->u4TxCachedSize);

	QUEUE_INITIALIZE(&prTxCtrl->rTxMgmtTxingQueue);
	prTxCtrl->i4TxMgmtPendingNum = 0;

#if CFG_HIF_STATISTICS
	prTxCtrl->u4TotalTxAccessNum = 0;
	prTxCtrl->u4TotalTxPacketNum = 0;
#endif

	prTxCtrl->i4PendingFwdFrameCount = 0;

	/* Assign init value */
	/* Tx sequence number */
	prAdapter->ucTxSeqNum = 0;
	/* PID pool */
	for (i = 0; i < WTBL_SIZE; i++)
		prAdapter->aucPidPool[i] = NIC_TX_DESC_DRIVER_PID_MIN;

	/* enable/disable TX resource control */
	prTxCtrl->fgIsTxResourceCtrl = NIC_TX_RESOURCE_CTRL;

	prAdapter->cArpNoResponseIdx = -1;

	qmInit(prAdapter, halIsTxResourceControlEn(prAdapter));

	TX_RESET_ALL_CNTS(prTxCtrl);

}				/* end of nicTxInitialize() */

u_int8_t nicTxSanityCheckResource(IN struct ADAPTER
				  *prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t ucTC;
	uint32_t ucTotalMaxResource = 0;
	uint32_t ucTotalFreeResource = 0;
	u_int8_t fgError = FALSE;

	if (prAdapter->rWifiVar.ucTxDbg & BIT(0)) {
		prTxCtrl = &prAdapter->rTxCtrl;

		for (ucTC = TC0_INDEX; ucTC < TC_NUM; ucTC++) {
			ucTotalMaxResource +=
				prTxCtrl->rTc.au4MaxNumOfPage[ucTC];
			ucTotalFreeResource +=
				prTxCtrl->rTc.au4FreePageCount[ucTC];

			if (prTxCtrl->rTc.au4FreePageCount[ucTC] >
			    prTxCtrl->u4TotalPageNum) {
				DBGLOG(TX, ERROR,
					"%s:%u\n error\n", __func__, __LINE__);
				fgError = TRUE;
			}

			if (prTxCtrl->rTc.au4MaxNumOfPage[ucTC] >
			    prTxCtrl->u4TotalPageNum) {
				DBGLOG(TX, ERROR,
					"%s:%u\n error\n", __func__, __LINE__);
				fgError = TRUE;
			}

			if (prTxCtrl->rTc.au4FreePageCount[ucTC] >
			    prTxCtrl->rTc.au4MaxNumOfPage[ucTC]) {
				DBGLOG(TX, ERROR,
					"%s:%u\n error\n", __func__, __LINE__);
				fgError = TRUE;
			}
		}

		if (ucTotalMaxResource != prTxCtrl->u4TotalPageNum) {
			DBGLOG(TX, ERROR,
				"%s:%u\n error\n", __func__, __LINE__);
			fgError = TRUE;
		}

		if (ucTotalMaxResource < ucTotalFreeResource) {
			DBGLOG(TX, ERROR,
				"%s:%u\n error\n", __func__, __LINE__);
			fgError = TRUE;
		}

		if (ucTotalFreeResource > prTxCtrl->u4TotalPageNum) {
			DBGLOG(TX, ERROR,
				"%s:%u\n error\n", __func__, __LINE__);
			fgError = TRUE;
		}

		if (fgError) {
			DBGLOG(TX, ERROR, "Total resource[%u]\n",
			       prTxCtrl->u4TotalPageNum);
			qmDumpQueueStatus(prAdapter, NULL, 0);
		}
	}

	return !fgError;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Condition check if the PLE resource control is needed or not
 *
 * \param[in] prAdapter              Pointer to the Adapter structure.
 * \param[in] ucTC                   Specify the resource of TC
 *
 * \retval FALSE   Resource control is not needed.
 * \retval TRUE    Resource is not needed.
 */
/*----------------------------------------------------------------------------*/

u_int8_t nicTxResourceIsPleCtrlNeeded(IN struct ADAPTER
				      *prAdapter, IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

	/* no PLE resource control */
	if (!prTc->fgNeedPleCtrl)
		return FALSE;

	/* CMD doesn't have PLE */
	if (ucTC == 4)
		return FALSE;

	/* rom stage inbabd command use TC0. need refine a good method */
	if ((ucTC == 0) && (prAdapter->fgIsFwDownloaded == FALSE))
		return FALSE;

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	return !!(prTc->au4PleCtrlEnMap & (1<<ucTC));
#else
	return TRUE;
#endif
}

u_int8_t nicTxResourceIsPseCtrlNeeded(IN struct ADAPTER
				      *prAdapter, IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	return !!(prTc->au4PseCtrlEnMap & (1<<ucTC));
#else
	return TRUE;
#endif
}

uint32_t nicTxResourceGetPleFreeCount(IN struct ADAPTER
				      *prAdapter, IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

	if (!nicTxResourceIsPleCtrlNeeded(prAdapter, ucTC)) {
		/* unlimited value*/
		return 0xFFFFFFFF;
	}

	return prTc->au4FreePageCount_PLE[ucTC];
}



uint32_t nicTxResourceGetPseFreeCount(IN struct ADAPTER
				      *prAdapter, IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

	if (!nicTxResourceIsPseCtrlNeeded(prAdapter, ucTC)) {
		/* unlimited value*/
		return 0xFFFFFFFF;
	}

	return prTc->au4FreePageCount[ucTC];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver maintain a variable that is synchronous with the usage of
 *        individual TC Buffer Count. This function will check if has enough
 *        TC Buffer for incoming packet and then update the value after
 *        promise to provide the resources.
 *
 * \param[in] prAdapter              Pointer to the Adapter structure.
 * \param[in] ucTC                   Specify the resource of TC
 *
 * \retval WLAN_STATUS_SUCCESS   Resource is available and been assigned.
 * \retval WLAN_STATUS_RESOURCES Resource is not available.
 */
/*----------------------------------------------------------------------------*/

uint32_t nicTxAcquireResourcePLE(IN struct ADAPTER
				 *prAdapter, IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

	if (!nicTxResourceIsPleCtrlNeeded(prAdapter, ucTC))
		return WLAN_STATUS_SUCCESS;

	/* PLE Acquire */
	if (prTc->au4FreePageCount_PLE[ucTC] >=
	    NIX_TX_PLE_PAGE_CNT_PER_FRAME) {
		prTc->au4FreePageCount_PLE[ucTC] -=
			NIX_TX_PLE_PAGE_CNT_PER_FRAME;

		return WLAN_STATUS_SUCCESS;
	}

	DBGLOG(INIT, ERROR,
	       "Acquire PLE FAILURE. TC%d AcquirePageCnt[%u] FreeBufferCnt[%u] FreePageCnt[%u]\n",
	       ucTC, NIX_TX_PLE_PAGE_CNT_PER_FRAME,
	       prTc->au4FreeBufferCount_PLE[ucTC],
	       prTc->au4FreePageCount_PLE[ucTC]);


	return WLAN_STATUS_RESOURCES;
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver maintain a variable that is synchronous with the usage of
 *        individual TC Buffer Count. This function will check if has enough
 *        TC Buffer for incoming packet and then update the value after
 *        promise to provide the resources.
 *
 * \param[in] prAdapter              Pointer to the Adapter structure.
 * \param[in] ucTC                   Specify the resource of TC
 *
 * \retval WLAN_STATUS_SUCCESS   Resource is available and been assigned.
 * \retval WLAN_STATUS_RESOURCES Resource is not available.
 */
/*----------------------------------------------------------------------------*/
uint32_t u4CurrTick;
uint32_t nicTxAcquireResource(IN struct ADAPTER *prAdapter,
			      IN uint8_t ucTC, IN uint32_t u4PageCount,
			      IN u_int8_t fgReqLock)
{
#define TC4_NO_RESOURCE_DELAY_MS      5    /* exponential of 5s */

	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTc;
	uint32_t u4Status = WLAN_STATUS_RESOURCES;
	uint32_t u4MaxDataPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame;
	uint32_t u4MaxCmdPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame;
	struct QUE_MGT *prQM;

	KAL_SPIN_LOCK_DECLARATION();

	/* enable/disable TX resource control */
	if (!prAdapter->rTxCtrl.fgIsTxResourceCtrl)
		return WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;
	prTc = &prTxCtrl->rTc;

	if (fgReqLock)
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
#if 1
	prQM = &prAdapter->rQM;

	/* Force skip PSE page acuire */
	if (!nicTxResourceIsPseCtrlNeeded(prAdapter, ucTC))
		u4PageCount = 0;

	if (prTc->au4FreePageCount[ucTC] >= u4PageCount) {

		if (nicTxAcquireResourcePLE(prAdapter,
					    ucTC) != WLAN_STATUS_SUCCESS) {
			if (fgReqLock)
				KAL_RELEASE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_TX_RESOURCE);

			return WLAN_STATUS_RESOURCES;
		}

		/* This update must be AFTER the PLE-resource-check */
		prTc->au4FreePageCount[ucTC] -= u4PageCount;
		if (ucTC == TC4_INDEX) {
			u4CurrTick = 0;
			prTc->au4FreeBufferCount[ucTC] =
			(prTc->au4FreePageCount[ucTC] / u4MaxCmdPageCntPerFrame);
		} else
			prTc->au4FreeBufferCount[ucTC] =
			(prTc->au4FreePageCount[ucTC] / u4MaxDataPageCntPerFrame);

		prQM->au4QmTcUsedPageCounter[ucTC] += u4PageCount;

		DBGLOG(TX, TEMP,
		       "Acquire: TC%d AcquirePageCnt[%u] FreeBufferCnt[%u] FreePageCnt[%u]\n",
		       ucTC, u4PageCount, prTc->au4FreeBufferCount[ucTC],
		       prTc->au4FreePageCount[ucTC]);

		u4Status = WLAN_STATUS_SUCCESS;
	}

#else
	if (prTxCtrl->rTc.au4FreePageCount[ucTC] > 0) {

		prTxCtrl->rTc.au4FreePageCount[ucTC] -= 1;

		u4Status = WLAN_STATUS_SUCCESS;
	}
#endif
	if (fgReqLock)
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	if (ucTC == TC4_INDEX) {
		if (u4CurrTick == 0)
			u4CurrTick = kalGetTimeTick();
		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4CurrTick,
			SEC_TO_SYSTIME(TC4_NO_RESOURCE_DELAY_MS))) {
#if (CFG_SUPPORT_TRACE_TC4 == 1)
			wlanDumpTcResAndTxedCmd(NULL, 0);
#endif
			cmdBufDumpCmdQueue(&prAdapter->rPendingCmdQueue,
					   "waiting response CMD queue");
		}
	}

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Driver maintain a variable that is synchronous with the usage of
 *        individual TC Buffer Count. This function will do polling if FW has
 *        return the resource.
 *        Used when driver start up before enable interrupt.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param ucTC           Specify the resource of TC
 *
 * @retval WLAN_STATUS_SUCCESS   Resource is available.
 * @retval WLAN_STATUS_FAILURE   Resource is not available.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxPollingResource(IN struct ADAPTER *prAdapter,
			      IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	int32_t i = NIC_TX_RESOURCE_POLLING_TIMEOUT;
	/*UINT_32 au4WTSR[8];*/

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

	if (ucTC >= TC_NUM)
		return WLAN_STATUS_FAILURE;

	if (!nicTxResourceIsPseCtrlNeeded(prAdapter, ucTC))
		return WLAN_STATUS_SUCCESS;

	if (prTxCtrl->rTc.au4FreeBufferCount[ucTC] > 0)
		return WLAN_STATUS_SUCCESS;

	while (i-- > 0) {
#if 1
		u4Status = halTxPollingResource(prAdapter, ucTC);
		if (u4Status == WLAN_STATUS_RESOURCES)
			kalMsleep(NIC_TX_RESOURCE_POLLING_DELAY_MSEC);
		else
			break;
#else
		HAL_READ_TX_RELEASED_COUNT(prAdapter, au4WTSR);

		if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
		    || fgIsBusAccessFailed == TRUE) {
			u4Status = WLAN_STATUS_FAILURE;
			break;
		} else if (halTxReleaseResource(prAdapter,
						(uint16_t *) au4WTSR)) {
			if (prTxCtrl->rTc.au4FreeBufferCount[ucTC] > 0) {
				u4Status = WLAN_STATUS_SUCCESS;
				break;
			}
			kalMsleep(NIC_TX_RESOURCE_POLLING_DELAY_MSEC);
		} else {
			kalMsleep(NIC_TX_RESOURCE_POLLING_DELAY_MSEC);
		}
#endif
	}

#if DBG
	{
		int32_t i4Times = NIC_TX_RESOURCE_POLLING_TIMEOUT - (i + 1);

		if (i4Times) {
			DBGLOG(TX, TRACE,
				"Polling MCR_WTSR delay %ld times, %ld msec\n",
				i4Times,
				(i4Times * NIC_TX_RESOURCE_POLLING_DELAY_MSEC));
		}
	}
#endif /* DBG */

	return u4Status;

}				/* end of nicTxPollingResource() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver maintain a variable that is synchronous with the usage of
 *        individual TC Buffer Count. This function will release TC Buffer
 *        count according to the given TX_STATUS COUNTER after TX Done.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t nicTxReleaseResource(IN struct ADAPTER *prAdapter,
			      IN uint8_t ucTc, IN uint32_t u4PageCount,
			      IN u_int8_t fgReqLock, IN u_int8_t fgPLE)
{
	struct TX_TCQ_STATUS *prTcqStatus;
	u_int8_t bStatus = FALSE;
	uint32_t u4MaxDataPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame;
	uint32_t u4MaxCmdPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame;
	struct QUE_MGT *prQM = NULL;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	/* enable/disable TX resource control */
	if (!prAdapter->rTxCtrl.fgIsTxResourceCtrl)
		return TRUE;

	/* No need to do PLE resource control */
	if (fgPLE && !nicTxResourceIsPleCtrlNeeded(prAdapter, ucTc))
		return TRUE;
	/* No need to do PSE resource control */
	else if (!fgPLE && !nicTxResourceIsPseCtrlNeeded(prAdapter, ucTc))
		return TRUE;

	prTcqStatus = &prAdapter->rTxCtrl.rTc;
	prQM = &prAdapter->rQM;

	/* Return free Tc page count */
	if (fgReqLock)
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	if (fgPLE) {
		prTcqStatus->au4FreePageCount_PLE[ucTc] += u4PageCount;
		prTcqStatus->au4FreeBufferCount_PLE[ucTc] =
			(prTcqStatus->au4FreePageCount_PLE[ucTc] /
			 NIX_TX_PLE_PAGE_CNT_PER_FRAME);
	} else {
		prTcqStatus->au4FreePageCount[ucTc] += u4PageCount;
		if (ucTc == TC4_INDEX)
			prTcqStatus->au4FreeBufferCount[ucTc] =
				(prTcqStatus->au4FreePageCount[ucTc] /
						 u4MaxCmdPageCntPerFrame);
		else
			prTcqStatus->au4FreeBufferCount[ucTc] =
				(prTcqStatus->au4FreePageCount[ucTc] /
						 u4MaxDataPageCntPerFrame);

	}
	prQM->au4QmTcResourceBackCounter[ucTc] += u4PageCount;

	if (fgReqLock)
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

#if (CFG_SUPPORT_TRACE_TC4 == 1)
	if (ucTc == TC4_INDEX)
		wlanTraceReleaseTcRes(prAdapter, u4PageCount,
				      prTcqStatus->au4FreePageCount[ucTc]);
#endif
	bStatus = TRUE;

	return bStatus;
}				/* end of nicTxReleaseResource() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver maintain a variable that is synchronous with the usage of
 *        individual TC Buffer Count. This function will release TC Buffer
 *        count for resource allocated but un-Tx MSDU_INFO
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxReleaseMsduResource(IN struct ADAPTER *prAdapter,
			      IN struct MSDU_INFO *prMsduInfoListHead)
{
	struct MSDU_INFO *prMsduInfo = prMsduInfoListHead,
				  *prNextMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	while (prMsduInfo) {
		prNextMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					 struct QUE_ENTRY *) prMsduInfo);

#if (CFG_SUPPORT_TRACE_TC4 == 1)
	if (prMsduInfo->ucTC != TC4_INDEX)
		DBGLOG(TX, ERROR, "[ERROR] sdio tx resource!!\n");
#endif
		nicTxReleaseResource_PSE(prAdapter, prMsduInfo->ucTC,
			nicTxGetDataPageCount(
				prAdapter, prMsduInfo->u2FrameLength,
				FALSE), FALSE);

		nicTxReleaseResource_PLE(prAdapter, prMsduInfo->ucTC,
					 NIX_TX_PLE_PAGE_CNT_PER_FRAME, FALSE);

		prMsduInfo = prNextMsduInfo;
	};

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Reset TC Buffer Count to initialized value
 *
 * \param[in] prAdapter              Pointer to the Adapter structure.
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxResetResource(IN struct ADAPTER *prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	uint32_t u4MaxDataPageCntPerFrame = 0;
	uint32_t u4MaxCmdPageCntPerFrame = 0;
	uint8_t ucIdx;

	KAL_SPIN_LOCK_DECLARATION();

	DEBUGFUNC("nicTxResetResource");

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

	/* Following two lines MUST be in order. */
	prTxCtrl->u4DataPageSize = halGetHifTxDataPageSize(prAdapter);
	prTxCtrl->u4MaxDataPageCntPerFrame = nicTxGetMaxDataPageCntPerFrame(
			prAdapter);
	prTxCtrl->u4MaxCmdPageCntPerFrame = nicTxGetMaxCmdPageCntPerFrame(
			prAdapter);

	u4MaxDataPageCntPerFrame = prTxCtrl->u4MaxDataPageCntPerFrame;
	u4MaxCmdPageCntPerFrame = prTxCtrl->u4MaxCmdPageCntPerFrame;

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	/* Delta page count */
	kalMemZero(prTxCtrl->rTc.au4TxDonePageCount,
		   sizeof(prTxCtrl->rTc.au4TxDonePageCount));
	kalMemZero(prTxCtrl->rTc.au4PreUsedPageCount,
		   sizeof(prTxCtrl->rTc.au4PreUsedPageCount));

	prTxCtrl->rTc.ucNextTcIdx = TC0_INDEX;
	prTxCtrl->rTc.u4AvaliablePageCount = 0;

	DBGLOG(TX, TRACE,
	       "Default TCQ free resource [%u %u %u %u %u]\n",
	       prAdapter->rWifiVar.au4TcPageCount[TC0_INDEX],
	       prAdapter->rWifiVar.au4TcPageCount[TC1_INDEX],
	       prAdapter->rWifiVar.au4TcPageCount[TC2_INDEX],
	       prAdapter->rWifiVar.au4TcPageCount[TC3_INDEX],
	       prAdapter->rWifiVar.au4TcPageCount[TC4_INDEX]);

	/* Reset counter: PSE */
	prAdapter->rTxCtrl.u4TotalPageNum = 0;
	prAdapter->rTxCtrl.u4TotalTxRsvPageNum = 0;
	/* Reset counter: PLE */
	prAdapter->rTxCtrl.u4TotalPageNumPle = 0;

	/* Assign resource for each TC according to prAdapter->rWifiVar */
	for (ucIdx = TC0_INDEX; ucIdx < TC_NUM; ucIdx++) {
		/*
		 * PSE
		 */

		/* Page Count */
		prTxCtrl->rTc.au4MaxNumOfPage[ucIdx] =
			prAdapter->rWifiVar.au4TcPageCount[ucIdx];
		prTxCtrl->rTc.au4FreePageCount[ucIdx] =
			prAdapter->rWifiVar.au4TcPageCount[ucIdx];

		DBGLOG(TX, TRACE, "Set TC%u Default[%u] Max[%u] Free[%u]\n",
		       ucIdx,
		       prAdapter->rWifiVar.au4TcPageCount[ucIdx],
		       prTxCtrl->rTc.au4MaxNumOfPage[ucIdx],
		       prTxCtrl->rTc.au4FreePageCount[ucIdx]);

		/* Buffer count */
		if (ucIdx == TC4_INDEX) {
			prTxCtrl->rTc.au4MaxNumOfBuffer[ucIdx] =
				(prTxCtrl->rTc.au4MaxNumOfPage[ucIdx] /
				 (u4MaxCmdPageCntPerFrame));

			prTxCtrl->rTc.au4FreeBufferCount[ucIdx] =
				(prTxCtrl->rTc.au4FreePageCount[ucIdx] /
				 (u4MaxCmdPageCntPerFrame));
		} else {
			prTxCtrl->rTc.au4MaxNumOfBuffer[ucIdx] =
				(prTxCtrl->rTc.au4MaxNumOfPage[ucIdx] /
				 (u4MaxDataPageCntPerFrame));

			prTxCtrl->rTc.au4FreeBufferCount[ucIdx] =
				(prTxCtrl->rTc.au4FreePageCount[ucIdx] /
				 (u4MaxDataPageCntPerFrame));
		}

		DBGLOG(TX, TRACE,
		       "Set TC%u Default[%u] Buffer Max[%u] Free[%u]\n",
		       ucIdx,
		       prAdapter->rWifiVar.au4TcPageCount[ucIdx],
		       prTxCtrl->rTc.au4MaxNumOfBuffer[ucIdx],
		       prTxCtrl->rTc.au4FreeBufferCount[ucIdx]);

		prAdapter->rTxCtrl.u4TotalPageNum +=
			prTxCtrl->rTc.au4MaxNumOfPage[ucIdx];


		/*
		 * PLE
		 */
		if (prAdapter->rTxCtrl.rTc.fgNeedPleCtrl) {
			/* Page Count */
			prTxCtrl->rTc.au4MaxNumOfPage_PLE[ucIdx] =
				prAdapter->rWifiVar.au4TcPageCountPle[ucIdx];
			prTxCtrl->rTc.au4FreePageCount_PLE[ucIdx] =
				prAdapter->rWifiVar.au4TcPageCountPle[ucIdx];

			DBGLOG(TX, TRACE,
			       "[PLE]Set TC%u Default[%u] Max[%u] Free[%u]\n",
			       ucIdx,
			       prAdapter->rWifiVar.au4TcPageCountPle[ucIdx],
			       prTxCtrl->rTc.au4MaxNumOfPage_PLE[ucIdx],
			       prTxCtrl->rTc.au4FreePageCount_PLE[ucIdx]);

			/* Buffer count */
			prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[ucIdx] =
				(prTxCtrl->rTc.au4MaxNumOfPage_PLE[ucIdx] /
				 NIX_TX_PLE_PAGE_CNT_PER_FRAME);

			prTxCtrl->rTc.au4FreeBufferCount_PLE[ucIdx] =
				(prTxCtrl->rTc.au4FreePageCount_PLE[ucIdx] /
				 NIX_TX_PLE_PAGE_CNT_PER_FRAME);


			DBGLOG(TX, TRACE,
			       "[PLE]Set TC%u Default[%u] Buffer Max[%u] Free[%u]\n",
			       ucIdx,
			       prAdapter->rWifiVar.au4TcPageCountPle[ucIdx],
			       prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[ucIdx],
			       prTxCtrl->rTc.au4FreeBufferCount_PLE[ucIdx]);

			prAdapter->rTxCtrl.u4TotalPageNumPle +=
				prTxCtrl->rTc.au4MaxNumOfPage_PLE[ucIdx];
		}
	}

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

	if (!prAdapter->fgIsNicTxReousrceValid)/* use default value */
		prAdapter->nicTxReousrce.ucPpTxAddCnt =
			NIC_TX_LEN_ADDING_LENGTH;
	DBGLOG(TX, TRACE,
	       "Reset TCQ free resource to Page <<PSE>>:Buf [%u:%u %u:%u %u:%u %u:%u %u:%u]\n",
	       prTxCtrl->rTc.au4FreePageCount[TC0_INDEX],
	       prTxCtrl->rTc.au4FreeBufferCount[TC0_INDEX],
	       prTxCtrl->rTc.au4FreePageCount[TC1_INDEX],
	       prTxCtrl->rTc.au4FreeBufferCount[TC1_INDEX],
	       prTxCtrl->rTc.au4FreePageCount[TC2_INDEX],
	       prTxCtrl->rTc.au4FreeBufferCount[TC2_INDEX],
	       prTxCtrl->rTc.au4FreePageCount[TC3_INDEX],
	       prTxCtrl->rTc.au4FreeBufferCount[TC3_INDEX],
	       prTxCtrl->rTc.au4FreePageCount[TC4_INDEX],
	       prTxCtrl->rTc.au4FreeBufferCount[TC4_INDEX]);

	if (prAdapter->rTxCtrl.rTc.fgNeedPleCtrl)
		DBGLOG(TX, TRACE,
		       "Reset TCQ free resource to Page <<PLE>>:Buf [%u:%u %u:%u %u:%u %u:%u %u:%u]\n",
		       prTxCtrl->rTc.au4FreePageCount_PLE[TC0_INDEX],
		       prTxCtrl->rTc.au4FreeBufferCount_PLE[TC0_INDEX],
		       prTxCtrl->rTc.au4FreePageCount_PLE[TC1_INDEX],
		       prTxCtrl->rTc.au4FreeBufferCount_PLE[TC1_INDEX],
		       prTxCtrl->rTc.au4FreePageCount_PLE[TC2_INDEX],
		       prTxCtrl->rTc.au4FreeBufferCount_PLE[TC2_INDEX],
		       prTxCtrl->rTc.au4FreePageCount_PLE[TC3_INDEX],
		       prTxCtrl->rTc.au4FreeBufferCount_PLE[TC3_INDEX],
		       prTxCtrl->rTc.au4FreePageCount_PLE[TC4_INDEX],
		       prTxCtrl->rTc.au4FreeBufferCount_PLE[TC4_INDEX]);


	DBGLOG(TX, TRACE,
	       "Reset TCQ MAX resource to Page <<PSE>>:Buf [%u:%u %u:%u %u:%u %u:%u %u:%u]\n",
	       prTxCtrl->rTc.au4MaxNumOfPage[TC0_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfBuffer[TC0_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfPage[TC1_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfBuffer[TC1_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfPage[TC2_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfBuffer[TC2_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfPage[TC3_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfBuffer[TC3_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfPage[TC4_INDEX],
	       prTxCtrl->rTc.au4MaxNumOfBuffer[TC4_INDEX]);

	if (prAdapter->rTxCtrl.rTc.fgNeedPleCtrl)
		DBGLOG(TX, TRACE,
		       "Reset TCQ MAX resource to Page <<PLE>>:Buf [%u:%u %u:%u %u:%u %u:%u %u:%u]\n",
		       prTxCtrl->rTc.au4MaxNumOfPage_PLE[TC0_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[TC0_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfPage_PLE[TC1_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[TC1_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfPage_PLE[TC2_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[TC2_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfPage_PLE[TC3_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[TC3_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfPage_PLE[TC4_INDEX],
		       prTxCtrl->rTc.au4MaxNumOfBuffer_PLE[TC4_INDEX]);

	return WLAN_STATUS_SUCCESS;
}

#if QM_FAST_TC_RESOURCE_CTRL
uint32_t
nicTxGetAdjustableResourceCnt(IN struct ADAPTER *prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t ucIdx;
	uint32_t u4TotAdjCnt = 0;
	uint32_t u4AdjCnt;
	struct QUE_MGT *prQM = NULL;

	prQM = &prAdapter->rQM;
	prTxCtrl = &prAdapter->rTxCtrl;

	for (ucIdx = TC0_INDEX; ucIdx < TC_NUM; ucIdx++) {
		if (ucIdx == TC4_INDEX)
			continue;

		if (prTxCtrl->rTc.au4FreeBufferCount[ucIdx] >
		    prQM->au4MinReservedTcResource[ucIdx])
			u4AdjCnt = prTxCtrl->rTc.au4FreeBufferCount[ucIdx] -
				   prQM->au4MinReservedTcResource[ucIdx];
		else
			u4AdjCnt = 0;

		u4TotAdjCnt += u4AdjCnt;
	}

	/* no PLE resource control */
	if (!prAdapter->rTxCtrl.rTc.fgNeedPleCtrl)
		return u4TotAdjCnt;

	/* PLE part */
	for (ucIdx = TC0_INDEX; ucIdx < TC_NUM; ucIdx++) {
		if (ucIdx == TC4_INDEX)
			continue;

		if (prTxCtrl->rTc.au4FreeBufferCount_PLE[ucIdx] >
		    prQM->au4MinReservedTcResource[ucIdx])
			u4AdjCnt = prTxCtrl->rTc.au4FreeBufferCount_PLE[ucIdx] -
				   prQM->au4MinReservedTcResource[ucIdx];
		else
			u4AdjCnt = 0;

		u4TotAdjCnt += u4AdjCnt;
	}

	return u4TotAdjCnt;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief Driver maintain a variable that is synchronous with the usage of
 *        individual TC Buffer Count. This function will return the value for
 *        other component which needs this information for making decisions
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param ucTC           Specify the resource of TC
 *
 * @retval UINT_8        The number of corresponding TC number
 */
/*----------------------------------------------------------------------------*/
uint16_t nicTxGetResource(IN struct ADAPTER *prAdapter,
			  IN uint8_t ucTC)
{
	struct TX_CTRL *prTxCtrl;

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

	ASSERT(prTxCtrl);

	if (ucTC >= TC_NUM)
		return 0;
	else
		return prTxCtrl->rTc.au4FreePageCount[ucTC];
}

uint8_t nicTxGetFrameResourceType(IN uint8_t eFrameType,
				  IN struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucTC;

	switch (eFrameType) {
	case FRAME_TYPE_802_1X:
		ucTC = TC4_INDEX;
		break;

	case FRAME_TYPE_MMPDU:
		if (prMsduInfo)
			ucTC = prMsduInfo->ucTC;
		else
			ucTC = TC4_INDEX;
		break;

	default:
		DBGLOG(INIT, WARN, "Undefined Frame Type(%u)\n",
		       eFrameType);
		ucTC = TC4_INDEX;
		break;
	}

	return ucTC;
}

uint8_t nicTxGetCmdResourceType(IN struct CMD_INFO
				*prCmdInfo)
{
	uint8_t ucTC;

	switch (prCmdInfo->eCmdType) {
	case COMMAND_TYPE_NETWORK_IOCTL:
	case COMMAND_TYPE_GENERAL_IOCTL:
		ucTC = TC4_INDEX;
		break;

	case COMMAND_TYPE_SECURITY_FRAME:
		ucTC = nicTxGetFrameResourceType(FRAME_TYPE_802_1X, NULL);
		break;

	case COMMAND_TYPE_MANAGEMENT_FRAME:
		ucTC = nicTxGetFrameResourceType(FRAME_TYPE_MMPDU,
						 prCmdInfo->prMsduInfo);
		break;

	default:
		DBGLOG(INIT, WARN, "Undefined CMD Type(%u)\n",
		       prCmdInfo->eCmdType);
		ucTC = TC4_INDEX;
		break;
	}

	return ucTC;
}

uint8_t nicTxGetTxQByTc(IN struct ADAPTER *prAdapter,
			IN uint8_t ucTc)
{
	return arTcResourceControl[ucTc].ucHifTxQIndex;
}

uint8_t nicTxGetTxDestPortIdxByTc(IN uint8_t ucTc)
{
	return arTcResourceControl[ucTc].ucDestPortIndex;
}

uint8_t nicTxGetTxDestQIdxByTc(IN uint8_t ucTc)
{
	return arTcResourceControl[ucTc].ucDestQueueIndex;
}

uint32_t nicTxGetRemainingTxTimeByTc(IN uint8_t ucTc)
{
	const uint8_t ucMaxLen = ARRAY_SIZE(arTcTrafficSettings);
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	ucTc = nicTxResTc2WmmTc(ucTc);
#endif
	if (ucTc >= ucMaxLen) {
		DBGLOG(TX, WARN,
			"Invalid TC%d, fallback to TC%d\n",
			ucTc, ucTc % ucMaxLen);
		ucTc %= ucMaxLen;
	}
	return arTcTrafficSettings[ucTc].u4RemainingTxTime;
}

uint8_t nicTxGetTxCountLimitByTc(IN uint8_t ucTc)
{
	const uint8_t ucMaxLen = ARRAY_SIZE(arTcTrafficSettings);
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	ucTc = nicTxResTc2WmmTc(ucTc);
#endif
	if (ucTc >= ucMaxLen) {
		DBGLOG(TX, WARN,
			"Invalid TC%d, fallback to TC%d\n",
			ucTc, ucTc % ucMaxLen);
		ucTc %= ucMaxLen;
	}
	return arTcTrafficSettings[ucTc].ucTxCountLimit;
}
uint8_t nicTxDescLengthByTc(IN uint8_t ucTc)
{
    const uint8_t ucMaxLen = ARRAY_SIZE(arTcTrafficSettings);
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
    ucTc = nicTxResTc2WmmTc(ucTc);
#endif
    if (ucTc >= ucMaxLen)
        {
            DBGLOG(TX, WARN,"Invalid TC%d, fallback to TC%d\n",
                     ucTc, ucTc % ucMaxLen);
            ucTc %= ucMaxLen;
        }
    return arTcTrafficSettings[ucTc].u4TxDescLength;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll aggregate frame(PACKET_INFO_T)
 * corresponding to HIF TX port
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfoListHead     a link list of P_MSDU_INFO_T
 *
 * @retval WLAN_STATUS_SUCCESS   Bus access ok.
 * @retval WLAN_STATUS_FAILURE   Bus access fail.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxMsduInfoList(IN struct ADAPTER *prAdapter,
			   IN struct MSDU_INFO *prMsduInfoListHead)
{
	struct MSDU_INFO *prMsduInfo, *prNextMsduInfo;
	struct QUE qDataPort0, qDataPort1;
	struct QUE *prDataPort0, *prDataPort1;
	uint32_t status;

	ASSERT(prAdapter);
	ASSERT(prMsduInfoListHead);

	prMsduInfo = prMsduInfoListHead;

	prDataPort0 = &qDataPort0;
	prDataPort1 = &qDataPort1;

	QUEUE_INITIALIZE(prDataPort0);
	QUEUE_INITIALIZE(prDataPort1);

	/* Separate MSDU_INFO_T lists into 2 categories: for Port#0 & Port#1 */
	while (prMsduInfo) {
		prNextMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					 struct QUE_ENTRY *) prMsduInfo);

		switch (prMsduInfo->ucTC) {
		case TC0_INDEX:
		case TC1_INDEX:
		case TC2_INDEX:
		case TC3_INDEX:
			QUEUE_GET_NEXT_ENTRY((struct QUE_ENTRY *) prMsduInfo) =
				NULL;
			QUEUE_INSERT_TAIL(prDataPort0,
					  (struct QUE_ENTRY *) prMsduInfo);
			status = nicTxAcquireResource(
				prAdapter, prMsduInfo->ucTC,
				nicTxGetDataPageCount(
					prAdapter, prMsduInfo->u2FrameLength,
					FALSE), TRUE);
			ASSERT(status == WLAN_STATUS_SUCCESS);

			break;

		case TC4_INDEX:	/* Management packets */
			QUEUE_GET_NEXT_ENTRY((struct QUE_ENTRY *) prMsduInfo) =
				NULL;
			QUEUE_INSERT_TAIL(prDataPort1,
					  (struct QUE_ENTRY *) prMsduInfo);

			status = nicTxAcquireResource(
				prAdapter, prMsduInfo->ucTC,
				halTxGetCmdPageCount(prAdapter,
					prMsduInfo->u2FrameLength,
					FALSE), TRUE);
			ASSERT(status == WLAN_STATUS_SUCCESS);

			break;

		default:
			ASSERT(0);
			break;
		}

		prMsduInfo = prNextMsduInfo;
	}

	if (prDataPort0->u4NumElem > 0)
		nicTxMsduQueue(prAdapter, 0, prDataPort0);

	if (prDataPort1->u4NumElem > 0)
		nicTxMsduQueue(prAdapter, 1, prDataPort1);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_MULTITHREAD
/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll aggregate frame(PACKET_INFO_T)
 * corresponding to HIF TX port
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfoListHead     a link list of P_MSDU_INFO_T
 *
 * @retval WLAN_STATUS_SUCCESS   Bus access ok.
 * @retval WLAN_STATUS_FAILURE   Bus access fail.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxMsduInfoListMthread(IN struct ADAPTER
	*prAdapter, IN struct MSDU_INFO *prMsduInfoListHead)
{
#if CFG_FIX_2_TX_PORT
	struct MSDU_INFO *prMsduInfo, *prNextMsduInfo;
	struct QUE qDataPort0, qDataPort1;
	struct QUE *prDataPort0, *prDataPort1;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prMsduInfoListHead);

	prMsduInfo = prMsduInfoListHead;

	prDataPort0 = &qDataPort0;
	prDataPort1 = &qDataPort1;

	QUEUE_INITIALIZE(prDataPort0);
	QUEUE_INITIALIZE(prDataPort1);

	/* Separate MSDU_INFO_T lists into 2 categories: for Port#0 & Port#1 */
	while (prMsduInfo) {
		prNextMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					 struct QUE_ENTRY *) prMsduInfo);

		switch (prMsduInfo->ucTC) {
		case TC0_INDEX:
		case TC1_INDEX:
		case TC2_INDEX:
		case TC3_INDEX:
			QUEUE_GET_NEXT_ENTRY((struct QUE_ENTRY *) prMsduInfo) =
				NULL;
			QUEUE_INSERT_TAIL(prDataPort0,
					  (struct QUE_ENTRY *) prMsduInfo);
			break;

		case TC4_INDEX:	/* Management packets */
			QUEUE_GET_NEXT_ENTRY((struct QUE_ENTRY *) prMsduInfo) =
				NULL;
			QUEUE_INSERT_TAIL(prDataPort1,
					  (struct QUE_ENTRY *) prMsduInfo);
			break;

		default:
			ASSERT(0);
			break;
		}

		nicTxFillDataDesc(prAdapter, prMsduInfo);

		prMsduInfo = prNextMsduInfo;
	}

	if (prDataPort0->u4NumElem > 0
	    || prDataPort1->u4NumElem > 0) {

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
		QUEUE_CONCATENATE_QUEUES((&(prAdapter->rTxP0Queue)),
					 (prDataPort0));
		QUEUE_CONCATENATE_QUEUES((&(prAdapter->rTxP1Queue)),
					 (prDataPort1));
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);

		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
#else

	struct MSDU_INFO *prMsduInfo, *prNextMsduInfo;
	struct QUE qDataPort[TX_PORT_NUM];
	struct QUE *prDataPort[TX_PORT_NUM];
	int32_t i;
	u_int8_t fgSetTx2Hif = FALSE;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prMsduInfoListHead);

	prMsduInfo = prMsduInfoListHead;

	for (i = 0; i < TX_PORT_NUM; i++) {
		prDataPort[i] = &qDataPort[i];
		QUEUE_INITIALIZE(prDataPort[i]);
	}

	/* Separate MSDU_INFO_T lists into 2 categories: for Port#0 & Port#1 */
	while (prMsduInfo) {

		fgSetTx2Hif = TRUE;
		prNextMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					 struct QUE_ENTRY *) prMsduInfo);

		if (prMsduInfo->ucWmmQueSet == DBDC_2G_WMM_INDEX) {
			QUEUE_GET_NEXT_ENTRY((struct QUE_ENTRY *) prMsduInfo) =
				NULL;
			QUEUE_INSERT_TAIL(prDataPort[TX_2G_WMM_PORT_NUM],
					  (struct QUE_ENTRY *) prMsduInfo);
		} else {
			if (prMsduInfo->ucTC < TC_NUM) {
				QUEUE_GET_NEXT_ENTRY(
					(struct QUE_ENTRY *) prMsduInfo) =
					NULL;
				QUEUE_INSERT_TAIL(prDataPort[prMsduInfo->ucTC],
					(struct QUE_ENTRY *) prMsduInfo);
			} else
				ASSERT(0);
		}
		nicTxFillDataDesc(prAdapter, prMsduInfo);

		prMsduInfo = prNextMsduInfo;
	}

	if (fgSetTx2Hif) {
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
		for (i = 0; i < TX_PORT_NUM; i++)
			QUEUE_CONCATENATE_QUEUES((&(prAdapter->rTxPQueue[i])),
						 (prDataPort[i]));
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}

#endif
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll write frame(PACKET_INFO_T) into HIF
 * when apply multithread.
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Bus access ok.
 * @retval WLAN_STATUS_FAILURE   Bus access fail.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxMsduQueueMthread(IN struct ADAPTER *prAdapter)
{
#if CFG_FIX_2_TX_PORT
	struct QUE qDataPort0, qDataPort1;
	struct QUE *prDataPort0, *prDataPort1;
	uint32_t u4TxLoopCount;

	KAL_SPIN_LOCK_DECLARATION();

	prDataPort0 = &qDataPort0;
	prDataPort1 = &qDataPort1;

	QUEUE_INITIALIZE(prDataPort0);
	QUEUE_INITIALIZE(prDataPort1);

	u4TxLoopCount = prAdapter->rWifiVar.u4HifTxloopCount;

	while (u4TxLoopCount--) {
		while (QUEUE_IS_NOT_EMPTY(&(prAdapter->rTxP0Queue))) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
			QUEUE_MOVE_ALL(prDataPort0, &(prAdapter->rTxP0Queue));
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);

			nicTxMsduQueue(prAdapter, 0, prDataPort0);

			if (QUEUE_IS_NOT_EMPTY(prDataPort0)) {
				KAL_ACQUIRE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_TX_PORT_QUE);
				QUEUE_CONCATENATE_QUEUES_HEAD(
					&(prAdapter->rTxP0Queue),
					prDataPort0);
				KAL_RELEASE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_TX_PORT_QUE);

				break;
			}
		}

		while (QUEUE_IS_NOT_EMPTY(&(prAdapter->rTxP1Queue))) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
			QUEUE_MOVE_ALL(prDataPort1, &(prAdapter->rTxP1Queue));
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);

			nicTxMsduQueue(prAdapter, 1, prDataPort1);

			if (QUEUE_IS_NOT_EMPTY(prDataPort1)) {
				KAL_ACQUIRE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_TX_PORT_QUE);
				QUEUE_CONCATENATE_QUEUES_HEAD(
					&(prAdapter->rTxP1Queue),
					prDataPort1);
				KAL_RELEASE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_TX_PORT_QUE);

				break;
			}
		}
	}
#else

	uint32_t u4TxLoopCount = prAdapter->rWifiVar.u4HifTxloopCount;

	if (halIsHifStateSuspend(prAdapter)) {
		DBGLOG(TX, WARN, "Suspend TxMsduQueueMthread\n");
		return WLAN_STATUS_SUCCESS;
	}

	while (u4TxLoopCount--) {
		if (prAdapter->rWifiVar.ucTxMsduQueue == 1)
			nicTxMsduQueueByRR(prAdapter);
		else
			nicTxMsduQueueByPrio(prAdapter);
	}
#endif
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll write MSDU into HIF by TC priority
 *
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 */
/*----------------------------------------------------------------------------*/
void nicTxMsduQueueByPrio(struct ADAPTER *prAdapter)
{
	struct QUE qDataPort[TX_PORT_NUM];
	struct QUE *prDataPort[TX_PORT_NUM];
	int32_t i;

	KAL_SPIN_LOCK_DECLARATION();

	for (i = 0; i < TX_PORT_NUM; i++) {
		prDataPort[i] = &qDataPort[i];
		QUEUE_INITIALIZE(prDataPort[i]);
	}

	for (i = TC_NUM; i >= 0; i--) {
		while (QUEUE_IS_NOT_EMPTY(&(prAdapter->rTxPQueue[i]))) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
			QUEUE_MOVE_ALL(prDataPort[i],
				&(prAdapter->rTxPQueue[i]));
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);

			nicTxMsduQueue(prAdapter, 0, prDataPort[i]);

			if (QUEUE_IS_NOT_EMPTY(prDataPort[i])) {
				KAL_ACQUIRE_SPIN_LOCK(
					prAdapter,
					SPIN_LOCK_TX_PORT_QUE);
				QUEUE_CONCATENATE_QUEUES_HEAD(
					&(prAdapter->rTxPQueue[i]),
					prDataPort[i]);
				KAL_RELEASE_SPIN_LOCK(prAdapter,
						      SPIN_LOCK_TX_PORT_QUE);

				break;
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll write MSDU into HIF by Round-Robin
 *
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 */
/*----------------------------------------------------------------------------*/
void nicTxMsduQueueByRR(struct ADAPTER *prAdapter)
{
	struct QUE qDataPort0, qDataPort1, arTempQue[TX_PORT_NUM];
	struct QUE *prDataPort0, *prDataPort1, *prDataPort, *prTxQue;
	struct MSDU_INFO *prMsduInfo;
	bool fgIsAllQueneEmpty = false;
	int32_t i;

	KAL_SPIN_LOCK_DECLARATION();

	prDataPort0 = &qDataPort0;
	prDataPort1 = &qDataPort1;
	QUEUE_INITIALIZE(prDataPort0);
	QUEUE_INITIALIZE(prDataPort1);

	for (i = 0; i < TX_PORT_NUM; i++)
		QUEUE_INITIALIZE(&arTempQue[i]);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
	/* Dequeue each TCQ to dataQ by round-robin  */
	/* Check each TCQ is empty or not */
	while (!fgIsAllQueneEmpty) {
		fgIsAllQueneEmpty = true;
		for (i = TC_NUM; i >= 0; i--) {
			prTxQue = &(prAdapter->rTxPQueue[i]);
			if (QUEUE_IS_NOT_EMPTY(prTxQue)) {
				QUEUE_REMOVE_HEAD(
					prTxQue, prMsduInfo,
					struct MSDU_INFO *);
				if (prMsduInfo == NULL)
					break;
				if (halTxRingDataSelect(prAdapter, prMsduInfo))
					prDataPort = prDataPort1;
				else
					prDataPort = prDataPort0;

				QUEUE_INSERT_TAIL(
					prDataPort,
					(struct QUE_ENTRY *) prMsduInfo);
				fgIsAllQueneEmpty = false;
			}
		}
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);

	nicTxMsduQueue(prAdapter, 0, prDataPort0);
	nicTxMsduQueue(prAdapter, 0, prDataPort1);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
	/* Enque from dataQ to TCQ if TX don't finish */
	/* Need to reverse dataQ by TC first */
	while (QUEUE_IS_NOT_EMPTY(prDataPort0)) {
		QUEUE_REMOVE_HEAD(prDataPort0, prMsduInfo, struct MSDU_INFO *);
		QUEUE_INSERT_HEAD(&arTempQue[prMsduInfo->ucTC],
				  (struct QUE_ENTRY *) prMsduInfo);
	}
	while (QUEUE_IS_NOT_EMPTY(prDataPort1)) {
		QUEUE_REMOVE_HEAD(prDataPort1, prMsduInfo, struct MSDU_INFO *);
		QUEUE_INSERT_HEAD(&arTempQue[prMsduInfo->ucTC],
				  (struct QUE_ENTRY *) prMsduInfo);
	}

	for (i = 0; i < TX_PORT_NUM; i++) {
		while (QUEUE_IS_NOT_EMPTY(&arTempQue[i])) {
			QUEUE_REMOVE_HEAD(&arTempQue[i], prMsduInfo,
					  struct MSDU_INFO *);
			QUEUE_INSERT_HEAD(&prAdapter->rTxPQueue[i],
					  (struct QUE_ENTRY *) prMsduInfo);
		}
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
}

uint32_t nicTxGetMsduPendingCnt(IN struct ADAPTER
				*prAdapter)
{
#if CFG_FIX_2_TX_PORT
	return prAdapter->rTxP0Queue.u4NumElem +
		prAdapter->rTxP1Queue.u4NumElem;
#else
	int32_t i;
	uint32_t retValue = 0;

	for (i = 0; i < TX_PORT_NUM; i++)
		retValue += prAdapter->rTxPQueue[i].u4NumElem;
	return retValue;
#endif
}

#endif

void nicTxComposeDescAppend(IN struct ADAPTER *prAdapter,
			    IN struct MSDU_INFO *prMsduInfo,
			    OUT uint8_t *prTxDescBuffer)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

	if (prChipInfo->prTxDescOps->fillNicAppend)
		prChipInfo->prTxDescOps->fillNicAppend(prAdapter,
			prMsduInfo, prTxDescBuffer);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll compose the Tx descriptor of the MSDU.
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfo             Pointer to the Msdu info
 * @param prTxDesc               Pointer to the Tx descriptor buffer
 *
 * @retval VOID
 */
/*----------------------------------------------------------------------------*/
void
nicTxComposeDesc(
	IN struct ADAPTER *prAdapter,
	IN struct MSDU_INFO *prMsduInfo,
	IN uint32_t u4TxDescLength,
	IN u_int8_t fgIsTemplate,
	OUT uint8_t *prTxDescBuffer)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_compose)
		prTxDescOps->nic_txd_compose(
			prAdapter,
			prMsduInfo,
			u4TxDescLength,
			fgIsTemplate,
			prTxDescBuffer);
	else
		DBGLOG(TX, ERROR, "%s:: no nic_txd_compose??\n", __func__);
}

void
nicTxComposeSecurityFrameDesc(
	IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo,
	OUT uint8_t *prTxDescBuffer,
	OUT uint8_t *pucTxDescLength)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_compose_security_frame)
		prTxDescOps->nic_txd_compose_security_frame(
			prAdapter,
			prCmdInfo,
			prTxDescBuffer,
			pucTxDescLength);
	else
		DBGLOG(TX, ERROR, "%s:: no nic_txd_compose_security_frame??\n",
			__func__);
}

void
nicTxForceAmsduForCert(
	struct ADAPTER *prAdapter,
	u_int8_t *prTxDescBuffer)
{
#if (CFG_SUPPORT_802_11AX == 1) && (CFG_SUPPORT_CONNAC2X == 1)
	struct HW_MAC_CONNAC2X_TX_DESC *prTxDesc =
		(struct HW_MAC_CONNAC2X_TX_DESC *) prTxDescBuffer;

	if (prAdapter->rWifiVar.ucHeAmsduInAmpduTx &&
		prAdapter->rWifiVar.ucHeCertForceAmsdu)
		HAL_MAC_CONNAC2X_TXD_SET_HW_AMSDU(prTxDesc);
#endif /* (CFG_SUPPORT_802_11AX == 1) && (CFG_SUPPORT_CONNAC2X == 1) */
}

u_int8_t nicTxIsTXDTemplateAllowed(IN struct ADAPTER
				   *prAdapter, IN struct MSDU_INFO *prMsduInfo,
				   IN struct STA_RECORD *prStaRec)
{
	if (prMsduInfo->fgIsTXDTemplateValid) {
		if (prMsduInfo->fgIs802_1x)
			return FALSE;

		if (prMsduInfo->ucRateMode != MSDU_RATE_MODE_AUTO)
			return FALSE;

		if (!prStaRec)
			return FALSE;

		if (prMsduInfo->ucControlFlag)
			return FALSE;

		if (prMsduInfo->pfTxDoneHandler)
			return FALSE;

		if (prAdapter->rWifiVar.ucDataTxRateMode)
			return FALSE;

		return TRUE;
	}
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll compose the Tx descriptor of the MSDU.
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfo             Pointer to the Msdu info
 * @param prTxDesc               Pointer to the Tx descriptor buffer
 *
 * @retval VOID
 */
/*----------------------------------------------------------------------------*/
void
nicTxFillDesc(IN struct ADAPTER *prAdapter,
	      IN struct MSDU_INFO *prMsduInfo,
	      OUT uint8_t *prTxDescBuffer, OUT uint32_t *pu4TxDescLength)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	void *prTxDesc = prTxDescBuffer;
	void *prTxDescTemplate = NULL;
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(prAdapter,
				      prMsduInfo->ucStaRecIndex);
	uint32_t u4TxDescLength;
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	uint8_t ucChksumFlag = 0;
#endif
	struct TX_DESC_OPS_T *prTxDescOps = prChipInfo->prTxDescOps;

	/* This is to lock the process to preventing */
	/* nicTxFreeDescTemplate while Filling it */
#if defined(_HIF_USB)
	KAL_SPIN_LOCK_DECLARATION();
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif

	/*
	 * -------------------------------------------------------------------
	 * Fill up common fileds
	 * -------------------------------------------------------------------
	 */
	u4TxDescLength = NIC_TX_DESC_LONG_FORMAT_LENGTH;

	/* Get TXD from pre-allocated template */
	if (nicTxIsTXDTemplateAllowed(prAdapter, prMsduInfo,
				      prStaRec)
#if defined(_HIF_USB)
		&& prStaRec->aprTxDescTemplate[prMsduInfo->ucUserPriority]
#endif
		) {
		prTxDescTemplate =
			prStaRec->aprTxDescTemplate[prMsduInfo->ucUserPriority];
	}
	if (prTxDescTemplate) {
		if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA)
			kalMemCopy(prTxDesc, prTxDescTemplate,
				u4TxDescLength + prChipInfo->txd_append_size);
		else
			kalMemCopy(prTxDesc, prTxDescTemplate, u4TxDescLength);

#if defined(_HIF_USB)
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif
		nicTxFillDescByPktOption(prAdapter, prMsduInfo, prTxDesc);
	} else { /* Compose TXD by Msdu info */
#if defined(_HIF_USB)
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif
		DBGLOG_LIMITED(NIC, INFO, "Compose TXD by Msdu info\n");
#if (UNIFIED_MAC_TX_FORMAT == 1)
		if (prMsduInfo->eSrc == TX_PACKET_MGMT)
			prMsduInfo->ucPacketFormat = TXD_PKT_FORMAT_COMMAND;
		else
			prMsduInfo->ucPacketFormat = prChipInfo->ucPacketFormat;
#endif /* UNIFIED_MAC_TX_FORMAT == 1 */
		nicTxComposeDesc(prAdapter, prMsduInfo, u4TxDescLength,
				 FALSE, prTxDescBuffer);

		/* Compose TxD append */
		if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA)
			nicTxComposeDescAppend(prAdapter, prMsduInfo,
					       prTxDescBuffer + u4TxDescLength);
	}

	nicTxForceAmsduForCert(prAdapter, (u_int8_t *)prTxDesc);
	/*
	 * --------------------------------------------------------------------
	 * Fill up remaining parts, per-packet variant fields
	 * --------------------------------------------------------------------
	 */
	if (prTxDescOps->fillTxByteCount)
		prTxDescOps->fillTxByteCount(prAdapter,
				prMsduInfo, prTxDesc);

	/* Checksum offload */
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (prAdapter->fgIsSupportCsumOffload
	    && prMsduInfo->eSrc == TX_PACKET_OS) {
		if (prAdapter->u4CSUMFlags &
		    (CSUM_OFFLOAD_EN_TX_TCP | CSUM_OFFLOAD_EN_TX_UDP |
		     CSUM_OFFLOAD_EN_TX_IP)) {
			ASSERT(prMsduInfo->prPacket);
			ucChksumFlag |= (TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN);
			if (prTxDescOps->nic_txd_chksum_op)
				prTxDescOps->nic_txd_chksum_op(
					prTxDesc, ucChksumFlag);
			else
				DBGLOG(TX, ERROR,
					"%s:: no nic_txd_chksum_op??\n",
					__func__);
		}
	}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	/* Set EtherType & VLAN for non 802.11 frame */
	if (prTxDescOps->nic_txd_header_format_op)
		prTxDescOps->nic_txd_header_format_op(
			prTxDesc, prMsduInfo);
	else
		DBGLOG(TX, ERROR,
			"%s:: no nic_txd_header_format_op??\n",
			__func__);

#if CFG_SUPPORT_NAN
	/* BMC */
	if (prMsduInfo->ucStaRecIndex == STA_REC_INDEX_BMCAST) {
		/* NAN Todo: not using the struct HW_MAC_TX_DESC */
		HAL_MAC_TX_DESC_SET_BMC((struct HW_MAC_TX_DESC *)prTxDesc);
		HAL_MAC_TX_DESC_SET_NO_ACK((struct HW_MAC_TX_DESC *)prTxDesc);
	}
#endif

	if (pu4TxDescLength)
		*pu4TxDescLength = u4TxDescLength;
}

void
nicTxFillDataDesc(IN struct ADAPTER *prAdapter,
		  IN struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucOutputBuf;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

	pucOutputBuf = skb_push((struct sk_buff *)
				prMsduInfo->prPacket,
				NIC_TX_DESC_AND_PADDING_LENGTH +
				prChipInfo->txd_append_size);
	if (pucOutputBuf == NULL)
		return;

	nicTxFillDesc(prAdapter, prMsduInfo, pucOutputBuf, NULL);
	/* dump TXD to debug TX issue */
	if (prAdapter->rWifiVar.ucDataTxDone == 1)
		halDumpTxdInfo(prAdapter, (uint32_t *)pucOutputBuf);
}

void
nicTxCopyDesc(IN struct ADAPTER *prAdapter,
	      IN uint8_t *pucTarTxDesc, IN uint8_t *pucSrcTxDesc,
	      OUT uint8_t *pucTxDescLength)
{
	struct TX_DESC_OPS_T *prTxDescOps;
	uint8_t ucTxDescLength;

	prTxDescOps = prAdapter->chip_info->prTxDescOps;
	if (prTxDescOps->nic_txd_long_format_op(pucSrcTxDesc, FALSE))
		ucTxDescLength = NIC_TX_DESC_LONG_FORMAT_LENGTH;
	else
		ucTxDescLength = NIC_TX_DESC_SHORT_FORMAT_LENGTH;

	kalMemCopy(pucTarTxDesc, pucSrcTxDesc, ucTxDescLength);

	if (pucTxDescLength)
		*pucTxDescLength = ucTxDescLength;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll generate Tx descriptor template for each TID.
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prStaRec              Pointer to the StaRec structure.
 *
 * @retval VOID
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxGenerateDescTemplate(IN struct ADAPTER
				   *prAdapter, IN struct STA_RECORD *prStaRec)
{
	uint8_t ucTid;
	uint8_t ucTc;
	uint32_t u4TxDescSize, u4TxDescAppendSize;
	void *prTxDesc;
	struct MSDU_INFO *prMsduInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	/* Free previous template, first */
	/* nicTxFreeDescTemplate(prAdapter, prStaRec); */
	for (ucTid = 0; ucTid < TX_DESC_TID_NUM; ucTid++)
		prStaRec->aprTxDescTemplate[ucTid] = NULL;

	prMsduInfo = cnmPktAlloc(prAdapter, 0);

	if (!prMsduInfo)
		return WLAN_STATUS_RESOURCES;

	prChipInfo = prAdapter->chip_info;

	/* Fill up MsduInfo template */
	prMsduInfo->eSrc = TX_PACKET_OS;
	prMsduInfo->fgIs802_11 = FALSE;
	prMsduInfo->fgIs802_1x = FALSE;
	prMsduInfo->fgIs802_1x_NonProtected = FALSE;
#if (CFG_SUPPORT_CONNAC3X == 1)
	prMsduInfo->fgIs802_3 = TRUE;
#else
	prMsduInfo->fgIs802_3 = FALSE;
#endif
	prMsduInfo->fgIsVlanExists = FALSE;
	prMsduInfo->pfTxDoneHandler = NULL;
	prMsduInfo->prPacket = NULL;
	prMsduInfo->u2FrameLength = 0;
	prMsduInfo->u4Option = 0;
	prMsduInfo->u4FixedRateOption = 0;
	prMsduInfo->ucRateMode = MSDU_RATE_MODE_AUTO;
	prMsduInfo->ucBssIndex = prStaRec->ucBssIndex;
	prMsduInfo->ucPacketType = TX_PACKET_TYPE_DATA;
	prMsduInfo->ucPacketFormat = prChipInfo->ucPacketFormat;
	prMsduInfo->ucStaRecIndex = prStaRec->ucIndex;
	prMsduInfo->ucPID = NIC_TX_DESC_PID_RESERVED;

	u4TxDescSize = NIC_TX_DESC_LONG_FORMAT_LENGTH;
	u4TxDescAppendSize = prChipInfo->txd_append_size;

	DBGLOG(QM, INFO,
	       "Generate TXD template for STA[%u] QoS[%u]\n",
	       prStaRec->ucIndex, prStaRec->fgIsQoS);

	/* Generate new template */
	if (prStaRec->fgIsQoS) {
		/* For QoS STA, generate 8 TXD template (TID0~TID7) */
		for (ucTid = 0; ucTid < TX_DESC_TID_NUM; ucTid++) {

			if (prAdapter->rWifiVar.ucTcRestrict < TC_NUM)
				ucTc = prAdapter->rWifiVar.ucTcRestrict;
			else
				ucTc = nicTxWmmTc2ResTc(prAdapter,
					prStaRec->ucBssIndex,
					aucTid2ACI[ucTid]);
			if ((uint8_t)ucTc >= (uint8_t)NET_TC_NUM)
				u4TxDescSize =
				nicTxDescLengthByTc(ucTc);
			else
				u4TxDescSize =
				arTcTrafficSettings[ucTc].u4TxDescLength;
			DBGLOG(QM, INFO, "ucTc:%d Size:%d\n",
				ucTc, u4TxDescSize);
			/* Include TxD append */
			prTxDesc = kalMemAlloc(
				u4TxDescSize + u4TxDescAppendSize,
				VIR_MEM_TYPE);
			DBGLOG(QM, TRACE, "STA[%u] TID[%u] TxDTemp[0x%p]\n",
			       prStaRec->ucIndex, ucTid, prTxDesc);
			if (!prTxDesc) {
				rStatus = WLAN_STATUS_RESOURCES;
				break;
			}

			/* Update MsduInfo TID & TC */
			prMsduInfo->ucUserPriority = ucTid;
			prMsduInfo->ucTC = ucTc;

			/* Compose Tx desc template */
			nicTxComposeDesc(
				prAdapter, prMsduInfo, u4TxDescSize, TRUE,
				(uint8_t *) prTxDesc);

			/* Fill TxD append */
			nicTxComposeDescAppend(prAdapter, prMsduInfo,
				((uint8_t *)prTxDesc + u4TxDescSize));

			prStaRec->aprTxDescTemplate[ucTid] = prTxDesc;
		}
	} else {
		/* For non-QoS STA, generate 1 TXD template (TID0) */
		do {
			if (prAdapter->rWifiVar.ucTcRestrict < TC_NUM)
				ucTc = prAdapter->rWifiVar.ucTcRestrict;
			else
				ucTc = nicTxWmmTc2ResTc(prAdapter,
					prStaRec->ucBssIndex,
					NET_TC_WMM_AC_BE_INDEX);

			/* ucTxDescSize =
			 * arTcTrafficSettings[ucTc].ucTxDescLength;
			 */
			u4TxDescSize = NIC_TX_DESC_LONG_FORMAT_LENGTH;

			prTxDesc = kalMemAlloc(
				u4TxDescSize + u4TxDescAppendSize,
				VIR_MEM_TYPE);
			if (!prTxDesc) {
				rStatus = WLAN_STATUS_RESOURCES;
				break;
			}
			/* Update MsduInfo TID & TC */
			prMsduInfo->ucUserPriority = 0;
			prMsduInfo->ucTC = ucTc;

			/* Compose Tx desc template */
			nicTxComposeDesc(
				prAdapter, prMsduInfo, u4TxDescSize, TRUE,
				(uint8_t *) prTxDesc);

			/* Fill TxD append */
			nicTxComposeDescAppend(prAdapter, prMsduInfo,
				((uint8_t *)prTxDesc + u4TxDescSize));

			for (ucTid = 0; ucTid < TX_DESC_TID_NUM; ucTid++) {
				prStaRec->aprTxDescTemplate[ucTid] = prTxDesc;
				DBGLOG(QM, TRACE,
					"TXD template: TID[%u] Ptr[0x%p]\n",
				  ucTid, prTxDesc);
			}
		} while (FALSE);
	}

	nicTxReturnMsduInfo(prAdapter, prMsduInfo);

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll free Tx descriptor template for each TID.
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prStaRec              Pointer to the StaRec structure.
 *
 * @retval VOID
 */
/*----------------------------------------------------------------------------*/
void nicTxFreeDescTemplate(IN struct ADAPTER *prAdapter,
			   IN struct STA_RECORD *prStaRec)
{
	uint8_t ucTid;
	uint8_t ucTxDescSize;
	struct TX_DESC_OPS_T *prTxDescOps;
	struct HW_MAC_TX_DESC *prTxDesc;
	struct HW_MAC_TX_DESC *prFirstTxDesc = NULL;

#if defined(_HIF_USB)
	KAL_SPIN_LOCK_DECLARATION();
#endif

	DBGLOG(QM, TRACE, "Free TXD template for STA[%u] QoS[%u]\n",
	       prStaRec->ucIndex, prStaRec->fgIsQoS);

	for (ucTid = 0; ucTid < TX_DESC_TID_NUM; ucTid++) {
#if defined(_HIF_USB)
		/* This is to lock the process to preventing */
		/* nicTxFreeDescTemplate while Filling it */
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif
		if (ucTid == 0)
			prFirstTxDesc = (struct HW_MAC_TX_DESC *)
				prStaRec->aprTxDescTemplate[0];
		prTxDescOps = prAdapter->chip_info->prTxDescOps;

		prTxDesc = (struct HW_MAC_TX_DESC *)
			prStaRec->aprTxDescTemplate[ucTid];

		if (!prTxDesc) {
#if defined(_HIF_USB)
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif
			continue;
		}

		if (ucTid > 0 && prTxDesc == prFirstTxDesc) {
			/* This partial is for prStaRec->fgIsQoS = 0 case
			 * In this case, prStaRec->aprTxDescTemplate[0:7]'s
			 * value will be same,
			 * so should avoid repeated free.
			 */
			prStaRec->aprTxDescTemplate[ucTid] = NULL;
#if defined(_HIF_USB)
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif
			continue;
		}
		if (prTxDescOps->nic_txd_long_format_op(prTxDesc, FALSE))
			ucTxDescSize = NIC_TX_DESC_LONG_FORMAT_LENGTH;
		else
			ucTxDescSize = NIC_TX_DESC_SHORT_FORMAT_LENGTH;

		prStaRec->aprTxDescTemplate[ucTid] = NULL;
#if defined(_HIF_USB)
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_DESC);
#endif

		kalMemFree(prTxDesc, VIR_MEM_TYPE, ucTxDescSize);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll Update H/W AMSDU filed of TxD template.
 *
 * @param prAdapter             Pointer to the Adapter structure.
 * @param prStaRec              Pointer to the StaRec structure.
 * @param ucTid                 Select target Tid template
 * @param ucSet                 Set or clear
 *
 * @retval VOID
 */
/*----------------------------------------------------------------------------*/
void nicTxSetHwAmsduDescTemplate(
	IN struct ADAPTER *prAdapter,
	IN struct STA_RECORD *prStaRec,
	IN uint8_t ucTid,
	IN u_int8_t fgSet)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_set_hw_amsdu_template)
		prTxDescOps->nic_txd_set_hw_amsdu_template(
			prAdapter,
			prStaRec,
			ucTid,
			fgSet);
	else
		DBGLOG(TX, ERROR,
			"%s:: no nic_txd_set_hw_amsdu_template??\n",
			__func__);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Write data to device done
 *
 * \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
 * \param[in] prQue              msdu info que to be free
 *
 * \retval TRUE          operation success
 * \retval FALSE         operation fail
 */
/*----------------------------------------------------------------------------*/
void nicTxMsduDoneCb(IN struct GLUE_INFO *prGlueInfo,
		     IN struct QUE *prQue)
{
	struct MSDU_INFO *prMsduInfo, *prNextMsduInfo;
	struct QUE rFreeQueue;
	struct QUE *prFreeQueue;
	/* P_NATIVE_PACKET prNativePacket;*/
	struct TX_CTRL *prTxCtrl;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	prFreeQueue = &rFreeQueue;
	QUEUE_INITIALIZE(prFreeQueue);

	if (prQue && prQue->u4NumElem > 0) {
		prMsduInfo = (struct MSDU_INFO *) QUEUE_GET_HEAD(prQue);

		while (prMsduInfo) {
			prNextMsduInfo =
				(struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY(
					&prMsduInfo->rQueEntry);

#if 1
			nicTxFreePacket(prAdapter, prMsduInfo, FALSE);
#else
			prNativePacket = prMsduInfo->prPacket;

			/* Free MSDU_INFO */
			if (prMsduInfo->eSrc == TX_PACKET_OS) {
				wlanTxProfilingTagMsdu(prAdapter, prMsduInfo,
						       TX_PROF_TAG_DRV_DEQUE);
				kalSendComplete(
					prAdapter->prGlueInfo, prNativePacket,
					WLAN_STATUS_SUCCESS);
				prMsduInfo->prPacket = NULL;
			} else if (prMsduInfo->eSrc == TX_PACKET_FORWARDING) {
				GLUE_DEC_REF_CNT(
					prTxCtrl->i4PendingFwdFrameCount);
			}
#endif

			if (!prMsduInfo->pfTxDoneHandler)
				QUEUE_INSERT_TAIL(prFreeQueue,
					(struct QUE_ENTRY *) prMsduInfo);

			prMsduInfo = prNextMsduInfo;
		}

		wlanTxProfilingTagMsdu(prAdapter,
			(struct MSDU_INFO *) QUEUE_GET_HEAD(&rFreeQueue),
			TX_PROF_TAG_DRV_FREE_MSDU);

		nicTxReturnMsduInfo(prAdapter,
		  (struct MSDU_INFO *)
				QUEUE_GET_HEAD(&rFreeQueue));
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief In this function, we'll write frame(PACKET_INFO_T) into HIF.
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param ucPortIdx              Port Number
 * @param prQue                  a link list of P_MSDU_INFO_T
 *
 * @retval WLAN_STATUS_SUCCESS   Bus access ok.
 * @retval WLAN_STATUS_FAILURE   Bus access fail.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxMsduQueue(IN struct ADAPTER *prAdapter,
			uint8_t ucPortIdx, struct QUE *prQue)
{
	struct MSDU_INFO *prMsduInfo;
	struct TX_CTRL *prTxCtrl;

	ASSERT(prAdapter);
	ASSERT(prQue);

	prTxCtrl = &prAdapter->rTxCtrl;

#if CFG_HIF_STATISTICS
	prTxCtrl->u4TotalTxAccessNum++;
	prTxCtrl->u4TotalTxPacketNum += prQue->u4NumElem;
#endif

	while (QUEUE_IS_NOT_EMPTY(prQue)) {
		QUEUE_REMOVE_HEAD(prQue, prMsduInfo, struct MSDU_INFO *);

		if (!prMsduInfo)
			break;

		if (!halTxIsDataBufEnough(prAdapter, prMsduInfo)) {
			QUEUE_INSERT_HEAD(prQue,
				(struct QUE_ENTRY *) prMsduInfo);
			break;
		}

#if !CFG_SUPPORT_MULTITHREAD
		nicTxFillDataDesc(prAdapter, prMsduInfo);
#endif

		if (prMsduInfo->eSrc == TX_PACKET_OS) {
			wlanTxProfilingTagMsdu(prAdapter, prMsduInfo,
					       TX_PROF_TAG_DRV_TX_DONE);
			wlanFillTimestamp(prAdapter, prMsduInfo->prPacket,
					       PHASE_HIF_TX);
		}

		if (prMsduInfo->pfTxDoneHandler) {
			KAL_SPIN_LOCK_DECLARATION();

			/* Record native packet pointer for Tx done log */
			WLAN_GET_FIELD_32(&prMsduInfo->prPacket,
					  &prMsduInfo->u4TxDoneTag);

			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
			QUEUE_INSERT_TAIL(&(prTxCtrl->rTxMgmtTxingQueue),
					  (struct QUE_ENTRY *) prMsduInfo);
			KAL_RELEASE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);

			/* MsduInfo might be cleaned up somewhere
			 * without timer initialization. Initialize timer
			 * starting itcan make sure the timeout function
			 * is properly assigned.
			 */
			cnmTimerInitTimer(prAdapter,
				&prMsduInfo->rLifetimeTimer,
				(PFN_MGMT_TIMEOUT_FUNC)
				 nicTxMsduLifeTimeoutHandler,
				(unsigned long) prMsduInfo);

			cnmTimerStartTimer(prAdapter,
				&prMsduInfo->rLifetimeTimer,
				NIC_TX_REMAINING_LIFE_TIME);
		} else
			wlanTxLifetimeTagPacket(prAdapter, prMsduInfo,
						TX_PROF_TAG_DRV_TX_DONE);

		StatsEnvTxTime2Hif(prAdapter, prMsduInfo);

		HAL_WRITE_TX_DATA(prAdapter, prMsduInfo);
	}

	HAL_KICK_TX_DATA(prAdapter);

	return WLAN_STATUS_SUCCESS;
}


void nicTxMsduLifeTimeoutHandler(IN struct ADAPTER *prAdapter,
	IN unsigned long plParamPtr)
{
	struct MSDU_INFO *prMsduInfo = (struct MSDU_INFO *)plParamPtr;

	DBGLOG(TX, ERROR, "MSDU Life Timeout Tag[0x%08x] WIDX:PID[%u:%u]\n",
		prMsduInfo->u4TxDoneTag, prMsduInfo->ucWlanIndex,
		prMsduInfo->ucPID);

	prMsduInfo = nicGetPendingTxMsduInfo(prAdapter,
			 prMsduInfo->ucWlanIndex, prMsduInfo->ucPID);
	if (prMsduInfo) {
		nicTxFreePacket(prAdapter, prMsduInfo, TRUE);
		nicTxReturnMsduInfo(prAdapter, prMsduInfo);
	} else {
		DBGLOG(TX, ERROR, "Not in pending tx queue\n");
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief In this function, we'll write Command(CMD_INFO_T) into HIF.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prPacketInfo   Pointer of CMD_INFO_T
 * @param ucTC           Specify the resource of TC
 *
 * @retval WLAN_STATUS_SUCCESS   Bus access ok.
 * @retval WLAN_STATUS_FAILURE   Bus access fail.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxCmd(IN struct ADAPTER *prAdapter,
		  IN struct CMD_INFO *prCmdInfo, IN uint8_t ucTC)
{
	struct MSDU_INFO *prMsduInfo;
	struct TX_CTRL *prTxCtrl;
	struct sk_buff *skb;
	struct TX_DESC_OPS_T *prTxDescOps;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	prTxDescOps = prAdapter->chip_info->prTxDescOps;
	prTxCtrl = &prAdapter->rTxCtrl;
#if (CFG_SUPPORT_TRACE_TC4 == 1)
	wlanTraceTxCmd(prCmdInfo);
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	if (halTxGetFreeCmdCnt(prAdapter) <= 0)
		return WLAN_STATUS_RESOURCES;
#endif

	if (prCmdInfo->eCmdType == COMMAND_TYPE_SECURITY_FRAME) {
		prMsduInfo = prCmdInfo->prMsduInfo;

		prCmdInfo->pucTxd = prMsduInfo->aucTxDescBuffer;
		if (prTxDescOps->nic_txd_long_format_op(
			prMsduInfo->aucTxDescBuffer, FALSE))
			prCmdInfo->u4TxdLen = NIC_TX_DESC_LONG_FORMAT_LENGTH;
		else
			prCmdInfo->u4TxdLen = NIC_TX_DESC_SHORT_FORMAT_LENGTH;

		skb = (struct sk_buff *)prMsduInfo->prPacket;
		prCmdInfo->pucTxp = skb->data;
		prCmdInfo->u4TxpLen = skb->len;

		HAL_WRITE_TX_CMD(prAdapter, prCmdInfo, ucTC);

		prMsduInfo->prPacket = NULL;

		if (prMsduInfo->pfTxDoneHandler) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
			QUEUE_INSERT_TAIL(&(prTxCtrl->rTxMgmtTxingQueue),
					  (struct QUE_ENTRY *) prMsduInfo);
			KAL_RELEASE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
		} else {
			/* Only return MSDU_INFO */
			/* NativePacket will be freed at
			 * SEC frame CMD callback
			 */
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
		}

	} else if (prCmdInfo->eCmdType ==
		   COMMAND_TYPE_MANAGEMENT_FRAME) {
		prMsduInfo = prCmdInfo->prMsduInfo;

		ASSERT(prMsduInfo->fgIs802_11 == TRUE);
		ASSERT(prMsduInfo->eSrc == TX_PACKET_MGMT);

		prCmdInfo->pucTxd = prMsduInfo->aucTxDescBuffer;
		if (prTxDescOps->nic_txd_long_format_op(
			prMsduInfo->aucTxDescBuffer, FALSE))
			prCmdInfo->u4TxdLen = NIC_TX_DESC_LONG_FORMAT_LENGTH;
		else
			prCmdInfo->u4TxdLen = NIC_TX_DESC_SHORT_FORMAT_LENGTH;

		prCmdInfo->pucTxp = prMsduInfo->prPacket;
		prCmdInfo->u4TxpLen = prMsduInfo->u2FrameLength;

		if (prMsduInfo->pfTxDoneHandler) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
			QUEUE_INSERT_TAIL(&(prTxCtrl->rTxMgmtTxingQueue),
					  (struct QUE_ENTRY *) prMsduInfo);
			KAL_RELEASE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
			DBGLOG(TX, INFO, "Insert msdu WIDX:PID[%u:%u]\n",
				prMsduInfo->ucWlanIndex, prMsduInfo->ucPID);
		}

		DBGLOG(INIT, TRACE,
		"TX MGMT Frame: BSS[%u] WIDX:PID[%u:%u] SEQ[%u] STA[%u] RSP[%u]\n",
		prMsduInfo->ucBssIndex, prMsduInfo->ucWlanIndex,
		prMsduInfo->ucPID,
		prMsduInfo->ucTxSeqNum, prMsduInfo->ucStaRecIndex,
		prMsduInfo->pfTxDoneHandler ? TRUE : FALSE);

		HAL_WRITE_TX_CMD(prAdapter, prCmdInfo, ucTC);
		/* <4> Management Frame Post-Processing */
		GLUE_DEC_REF_CNT(prTxCtrl->i4TxMgmtPendingNum);

		if (prMsduInfo->pfTxDoneHandler == NULL) {
			cnmMgtPktFree(prAdapter, prMsduInfo);
		}

	} else {
		prCmdInfo->pucTxd = prCmdInfo->pucInfoBuffer;
		prCmdInfo->u4TxdLen = prCmdInfo->u2InfoBufLen;
		prCmdInfo->pucTxp = NULL;
		prCmdInfo->u4TxpLen = 0;

		HAL_WRITE_TX_CMD(prAdapter, prCmdInfo, ucTC);
	}

	return WLAN_STATUS_SUCCESS;
}				/* end of nicTxCmd() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will clean up all the pending frames in internal
 *        SW Queues by return the pending TX packet to the system.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxRelease(IN struct ADAPTER *prAdapter,
		  IN u_int8_t fgProcTxDoneHandler)
{
	struct TX_CTRL *prTxCtrl;
	struct MSDU_INFO *prMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;

	nicTxFlush(prAdapter);

	/* free MSDU_INFO_T from rTxMgmtMsduInfoList */
	do {
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);
		QUEUE_REMOVE_HEAD(&prTxCtrl->rTxMgmtTxingQueue, prMsduInfo,
				  struct MSDU_INFO *);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

		if (prMsduInfo) {
			DBGLOG(TX, TRACE,
				"%s: Get Msdu WIDX:PID[%u:%u] SEQ[%u] from Pending Q\n",
			  __func__, prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
			  prMsduInfo->ucTxSeqNum);

			/* invoke done handler */
			if (prMsduInfo->pfTxDoneHandler && fgProcTxDoneHandler)
				prMsduInfo->pfTxDoneHandler(
					prAdapter, prMsduInfo,
			    TX_RESULT_DROPPED_IN_DRIVER);

			nicTxFreeMsduInfoPacket(prAdapter, prMsduInfo);
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
		} else {
			break;
		}
	} while (TRUE);

}				/* end of nicTxRelease() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process the TX Done interrupt and pull in more pending frames in SW
 *        Queues for transmission.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessTxInterrupt(IN struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_MULTITHREAD
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#endif

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
#if CFG_SUPPORT_WAKEUP_STATISTICS
	if (kalIsWakeupByWlan(prAdapter))
		nicUpdateWakeupStatistics(prAdapter, TX_INT);
#endif
#endif /* fos_change end */

	prAdapter->prGlueInfo->IsrTxCnt++;
	halProcessTxInterrupt(prAdapter);

	if (halIsHifStateSuspend(prAdapter))
		DBGLOG(TX, WARN, "Suspend TX INT\n");

	/* Indicate Service Thread */
	if (kalGetTxPendingCmdCount(prAdapter->prGlueInfo) > 0
	    || (!HAL_IS_TX_DIRECT(prAdapter)
	    && wlanGetTxPendingFrameCount(prAdapter)))
		kalSetEvent(prAdapter->prGlueInfo);

	/* SER break point */
	if (nicSerIsTxStop(prAdapter)) {
		/* Skip following Tx handling */
		return;
	}

	/* TX direct break point */
	if (HAL_IS_TX_DIRECT(prAdapter)) {
		/* In TX direct mode, should not handle HIF-TX here.
		 * For CMD, trigger hif_thread only.
		 * For DATA, do nothing.
		 */
		if (kalGetTxPendingCmdCount(prAdapter->prGlueInfo))
			kalSetTxCmdEvent2Hif(prAdapter->prGlueInfo);
		return;
	}
#if CFG_SUPPORT_MULTITHREAD
	/* TX Commands */
	if (kalGetTxPendingCmdCount(prAdapter->prGlueInfo))
		wlanTxCmdMthread(prAdapter);

	/* Process TX data packet to HIF */
	if (nicTxGetMsduPendingCnt(prAdapter) >=
	    prWifiVar->u4TxIntThCount)
		nicTxMsduQueueMthread(prAdapter);
#endif
} /* end of nicProcessTxInterrupt() */

void nicTxFreePacket(IN struct ADAPTER *prAdapter,
		     IN struct MSDU_INFO *prMsduInfo, IN u_int8_t fgDrop)
{
	void *prNativePacket;
	struct TX_CTRL *prTxCtrl;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	cnmTimerStopTimer(prAdapter,
		&prMsduInfo->rLifetimeTimer);

	prTxCtrl = &prAdapter->rTxCtrl;

	prNativePacket = prMsduInfo->prPacket;

	if (fgDrop)
		rStatus = WLAN_STATUS_FAILURE;

	if (prMsduInfo->eSrc == TX_PACKET_OS) {
		if (prNativePacket)
			kalSendComplete(prAdapter->prGlueInfo, prNativePacket,
					rStatus);
		if (fgDrop)
			wlanUpdateTxStatistics(prAdapter, prMsduInfo,
				TRUE); /*get per-AC Tx drop packets */
	} else if (prMsduInfo->eSrc == TX_PACKET_MGMT) {
		if (prMsduInfo->pfTxDoneHandler)
			prMsduInfo->pfTxDoneHandler(prAdapter, prMsduInfo,
		    TX_RESULT_DROPPED_IN_DRIVER);
		if (prNativePacket)
			cnmMemFree(prAdapter, prNativePacket);
	} else if (prMsduInfo->eSrc == TX_PACKET_FORWARDING) {
		GLUE_DEC_REF_CNT(prTxCtrl->i4PendingFwdFrameCount);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function frees packet of P_MSDU_INFO_T linked-list
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfoList         a link list of P_MSDU_INFO_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxFreeMsduInfoPacket(IN struct ADAPTER *prAdapter,
			     IN struct MSDU_INFO *prMsduInfoListHead)
{
	void *prNativePacket;
	struct MSDU_INFO *prMsduInfo = prMsduInfoListHead;
	struct TX_CTRL *prTxCtrl;

	ASSERT(prAdapter);
	ASSERT(prMsduInfoListHead);

	prTxCtrl = &prAdapter->rTxCtrl;

	while (prMsduInfo) {
		prNativePacket = prMsduInfo->prPacket;

#if 1
		nicTxFreePacket(prAdapter, prMsduInfo, TRUE);
#else
		if (prMsduInfo->eSrc == TX_PACKET_OS) {
			if (prNativePacket)
				kalSendComplete(
					prAdapter->prGlueInfo, prNativePacket,
					WLAN_STATUS_FAILURE);
			/*get per-AC Tx drop packets */
			wlanUpdateTxStatistics(prAdapter, prMsduInfo, TRUE);
		} else if (prMsduInfo->eSrc == TX_PACKET_MGMT) {
			if (prMsduInfo->pfTxDoneHandler)
				prMsduInfo->pfTxDoneHandler(
					prAdapter, prMsduInfo,
					TX_RESULT_DROPPED_IN_DRIVER);
			if (prNativePacket)
				cnmMemFree(prAdapter, prNativePacket);
		} else if (prMsduInfo->eSrc == TX_PACKET_FORWARDING) {
			GLUE_DEC_REF_CNT(prTxCtrl->i4PendingFwdFrameCount);
		}
#endif
		prMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					struct QUE_ENTRY *) prMsduInfo);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function returns P_MSDU_INFO_T of MsduInfoList to
 *        TxCtrl->rfreeMsduInfoList
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfoList         a link list of P_MSDU_INFO_T
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxReturnMsduInfo(IN struct ADAPTER *prAdapter,
			 IN struct MSDU_INFO *prMsduInfoListHead)
{
	struct TX_CTRL *prTxCtrl;
	struct MSDU_INFO *prMsduInfo = prMsduInfoListHead,
				  *prNextMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	while (prMsduInfo) {
		prNextMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					 struct QUE_ENTRY *) prMsduInfo);

		switch (prMsduInfo->eSrc) {
		case TX_PACKET_FORWARDING:
			wlanReturnPacket(prAdapter, prMsduInfo->prPacket);
			break;
		case TX_PACKET_OS:
		case TX_PACKET_OS_OID:
		case TX_PACKET_MGMT:
		default:
			break;
		}

		/* Reset MSDU_INFO fields */
		kalMemZero(prMsduInfo, sizeof(struct MSDU_INFO));

		KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				      SPIN_LOCK_TX_MSDU_INFO_LIST);
		QUEUE_INSERT_TAIL(&prTxCtrl->rFreeMsduInfoList,
				  (struct QUE_ENTRY *) prMsduInfo);
		KAL_RELEASE_SPIN_LOCK(prAdapter,
				      SPIN_LOCK_TX_MSDU_INFO_LIST);
		prMsduInfo = prNextMsduInfo;
	};

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function fills packet information to P_MSDU_INFO_T
 *
 * @param prAdapter              Pointer to the Adapter structure.
 * @param prMsduInfo             P_MSDU_INFO_T
 * @param prPacket               P_NATIVE_PACKET
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/
u_int8_t nicTxFillMsduInfo(IN struct ADAPTER *prAdapter,
			   IN struct MSDU_INFO *prMsduInfo, IN void *prPacket)
{
	struct GLUE_INFO *prGlueInfo;
	u_int8_t fgIsLowestRate = FALSE;
#if CFG_CHANGE_CRITICAL_PACKET_PRIORITY || CFG_SUPPORT_TX_MGMT_USE_DATAQ || \
	CFG_SUPPORT_WIFI_SYSDVT
	u_int8_t fgIsHighPrioQ = FALSE;
#endif

	ASSERT(prAdapter);

	kalMemZero(prMsduInfo, sizeof(struct MSDU_INFO));

	prGlueInfo = prAdapter->prGlueInfo;
	ASSERT(prGlueInfo);

	kalGetEthDestAddr(prAdapter->prGlueInfo, prPacket,
			  prMsduInfo->aucEthDestAddr);

	prMsduInfo->prPacket = prPacket;
	prMsduInfo->ucBssIndex = GLUE_GET_PKT_BSS_IDX(prPacket);
#if CFG_SUPPORT_WIFI_SYSDVT
	if (prAdapter->ucTxTestUP != TX_TEST_UP_UNDEF)
		prMsduInfo->ucUserPriority = prAdapter->ucTxTestUP;
	else
#endif /* CFG_SUPPORT_WIFI_SYSDVT */
	prMsduInfo->ucUserPriority = GLUE_GET_PKT_TID(prPacket);
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
	if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_802_11_MGMT))
		prMsduInfo->ucMacHeaderLength = WLAN_MAC_MGMT_HEADER_LEN;
	else
#endif
		prMsduInfo->ucMacHeaderLength = GLUE_GET_PKT_HEADER_LEN(
							prPacket);
	prMsduInfo->u2FrameLength = (uint16_t)
				    GLUE_GET_PKT_FRAME_LEN(prPacket);
	prMsduInfo->u4PageCount = nicTxGetDataPageCount(prAdapter,
				  prMsduInfo->u2FrameLength, FALSE);

	if (GLUE_IS_PKT_FLAG_SET(prPacket)) {
		prMsduInfo->fgIs802_1x = GLUE_TEST_PKT_FLAG(prPacket,
					 ENUM_PKT_1X) ? TRUE : FALSE;
		prMsduInfo->fgIs802_1x_NonProtected =
			GLUE_TEST_PKT_FLAG(prPacket,
				ENUM_PKT_NON_PROTECTED_1X) ? TRUE : FALSE;
		prMsduInfo->fgIs802_3 = GLUE_TEST_PKT_FLAG(prPacket,
					ENUM_PKT_802_3) ? TRUE : FALSE;
		prMsduInfo->fgIsVlanExists = GLUE_TEST_PKT_FLAG(prPacket,
			ENUM_PKT_VLAN_EXIST) ? TRUE : FALSE;
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
		prMsduInfo->fgIs802_11 = GLUE_TEST_PKT_FLAG(prPacket,
					 ENUM_PKT_802_11_MGMT) ? TRUE : FALSE;
#endif

		if (prMsduInfo->fgIs802_1x)
			prMsduInfo->eEapolKeyType = secGetEapolKeyType(((
			struct sk_buff *)prPacket)->data);

		if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_DHCP)
		    && prAdapter->rWifiVar.ucDhcpTxDone)
			prMsduInfo->ucPktType = ENUM_PKT_DHCP;
		else if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_ARP)
			 && prAdapter->rWifiVar.ucArpTxDone)
			prMsduInfo->ucPktType = ENUM_PKT_ARP;
		else if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_1X))
			prMsduInfo->ucPktType = ENUM_PKT_1X;
		else if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_ICMP))
			prMsduInfo->ucPktType = ENUM_PKT_ICMP;
		else if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_TDLS))
			prMsduInfo->ucPktType = ENUM_PKT_TDLS;
		else if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_DNS))
			prMsduInfo->ucPktType = ENUM_PKT_DNS;
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
		else if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_802_11_MGMT))
			prMsduInfo->ucPktType = ENUM_PKT_802_11_MGMT;
#endif

#if (CFG_SUPPORT_DMASHDL_SYSDVT)
		if (prMsduInfo->ucPktType != ENUM_PKT_ICMP) {
			/* blocking non ICMP packets at DMASHDL DVT items */
			if (DMASHDL_DVT_ALLOW_PING_ONLY(prAdapter))
				return FALSE;
		}
#endif
		if (prMsduInfo->ucPktType != 0) {
			prMsduInfo->pfTxDoneHandler = wlanPktTxDone;
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
			if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_802_11_MGMT))
				prMsduInfo->ucTxSeqNum =
					nicIncreaseTxSeqNum(prAdapter);
			else
#endif
				prMsduInfo->ucTxSeqNum =
					GLUE_GET_PKT_SEQ_NO(prPacket);
		}

		if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_DHCP) ||
		    GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_ARP) ||
		    GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_1X)) {
			fgIsLowestRate = TRUE;
#if CFG_CHANGE_CRITICAL_PACKET_PRIORITY
			fgIsHighPrioQ = TRUE;
#endif
		}

#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
		if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_802_11_MGMT)) {
			fgIsLowestRate = TRUE;
			fgIsHighPrioQ = TRUE;
		}
#endif

#if CFG_SUPPORT_WIFI_SYSDVT
		/* must be the last check action */
		if (prAdapter->ucTxTestUP != TX_TEST_UP_UNDEF) {
			fgIsLowestRate = FALSE;
			fgIsHighPrioQ = FALSE;
		}
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

		if (fgIsLowestRate)
			/* Set BSS/STA lowest basic rate */
			prMsduInfo->ucRateMode = MSDU_RATE_MODE_LOWEST_RATE;

#if CFG_CHANGE_CRITICAL_PACKET_PRIORITY || CFG_SUPPORT_TX_MGMT_USE_DATAQ || \
	CFG_SUPPORT_WIFI_SYSDVT
		if (fgIsHighPrioQ)
			/* Set higher priority */
			prMsduInfo->ucUserPriority =
					NIC_TX_CRITICAL_DATA_TID;
#endif

#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
		if (GLUE_TEST_PKT_FLAG(prPacket, ENUM_PKT_802_11_MGMT))
			prMsduInfo->u8Cookie =
				GLUE_GET_PKT_COOKIE(prPacket);
#endif
	}

	/* Add dummy Tx done */
	if ((prAdapter->rWifiVar.ucDataTxDone == 1)
	    && (prMsduInfo->pfTxDoneHandler == NULL))
		prMsduInfo->pfTxDoneHandler = nicTxDummyTxDone;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function update TCQ values by passing current status to
 *        txAdjustTcQuotas
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Updated successfully
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxAdjustTcq(IN struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_MULTITHREAD
	struct TX_TCQ_ADJUST rTcqAdjust;
	struct TX_CTRL *prTxCtrl;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	qmAdjustTcQuotasMthread(prAdapter, &rTcqAdjust,
				&prTxCtrl->rTc);

#else

	uint32_t u4Num;
	struct TX_TCQ_ADJUST rTcqAdjust;
	struct TX_CTRL *prTxCtrl;
	struct TX_TCQ_STATUS *prTcqStatus;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;
	prTcqStatus = &prAdapter->rTxCtrl.rTc;
	ASSERT(prTxCtrl);

	if (qmAdjustTcQuotas(prAdapter, &rTcqAdjust,
			     &prTxCtrl->rTc)) {

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

		for (u4Num = 0; u4Num < TC_NUM; u4Num++) {
			/* Page count */
			prTxCtrl->rTc.au4FreePageCount[u4Num] +=
				(rTcqAdjust.ai4Variation[u4Num] *
				 NIC_TX_MAX_PAGE_PER_FRAME);
			prTxCtrl->rTc.au4MaxNumOfPage[u4Num] +=
				(rTcqAdjust.ai4Variation[u4Num] *
				 NIC_TX_MAX_PAGE_PER_FRAME);

			/* Buffer count */
			prTxCtrl->rTc.au4FreeBufferCount[u4Num] +=
				rTcqAdjust.ai4Variation[u4Num];
			prTxCtrl->rTc.au4MaxNumOfBuffer[u4Num] +=
				rTcqAdjust.ai4Variation[u4Num];

			ASSERT(prTxCtrl->rTc.au4FreeBufferCount[u4Num] >= 0);
			ASSERT(prTxCtrl->rTc.au4MaxNumOfBuffer[u4Num] >= 0);
		}

		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
#if 0
		DBGLOG(TX, LOUD,
		       "TCQ Status Free Page:Buf[%03u:%02u, %03u:%02u, %03u:%02u, %03u:%02u, %03u:%02u, %03u:%02u]\n",
		       prTcqStatus->au4FreePageCount[TC0_INDEX],
		       prTcqStatus->au4FreeBufferCount[TC0_INDEX],
		       prTcqStatus->au4FreePageCount[TC1_INDEX],
		       prTcqStatus->au4FreeBufferCount[TC1_INDEX],
		       prTcqStatus->au4FreePageCount[TC2_INDEX],
		       prTcqStatus->au4FreeBufferCount[TC2_INDEX],
		       prTcqStatus->au4FreePageCount[TC3_INDEX],
		       prTcqStatus->au4FreeBufferCount[TC3_INDEX],
		       prTcqStatus->au4FreePageCount[TC4_INDEX],
		       prTcqStatus->au4FreeBufferCount[TC4_INDEX],
		       prTcqStatus->au4FreePageCount[TC5_INDEX],
		       prTcqStatus->au4FreeBufferCount[TC5_INDEX]);
#endif
		DBGLOG(TX, LOUD,
		       "TCQ Status Max Page:Buf[%03u:%02u, %03u:%02u, %03u:%02u, %03u:%02u, %03u:%02u]\n",
		       prTcqStatus->au4MaxNumOfPage[TC0_INDEX],
		       prTcqStatus->au4MaxNumOfBuffer[TC0_INDEX],
		       prTcqStatus->au4MaxNumOfPage[TC1_INDEX],
		       prTcqStatus->au4MaxNumOfBuffer[TC1_INDEX],
		       prTcqStatus->au4MaxNumOfPage[TC2_INDEX],
		       prTcqStatus->au4MaxNumOfBuffer[TC2_INDEX],
		       prTcqStatus->au4MaxNumOfPage[TC3_INDEX],
		       prTcqStatus->au4MaxNumOfBuffer[TC3_INDEX],
		       prTcqStatus->au4MaxNumOfPage[TC4_INDEX],
		       prTcqStatus->au4MaxNumOfBuffer[TC4_INDEX]);

	}
#endif
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief this function flushes all packets queued in STA/AC queue
 *
 * @param prAdapter              Pointer to the Adapter structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Flushed successfully
 */
/*----------------------------------------------------------------------------*/

uint32_t nicTxFlush(IN struct ADAPTER *prAdapter)
{
	struct MSDU_INFO *prMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (HAL_IS_TX_DIRECT(prAdapter)) {
		nicTxDirectClearAllStaPsQ(prAdapter);
		nicTxDirectClearAllStaPendQ(prAdapter);
	} else {
		/* ask Per STA/AC queue to be fllushed
		 * and return all queued packets
		 */
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
		prMsduInfo = qmFlushTxQueues(prAdapter);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);

		if (prMsduInfo != NULL) {
			nicTxFreeMsduInfoPacket(prAdapter, prMsduInfo);
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

#if CFG_ENABLE_FW_DOWNLOAD
/*----------------------------------------------------------------------------*/
/*!
 * \brief In this function, we'll write Command(CMD_INFO_T) into HIF.
 *        However this function is used for INIT_CMD.
 *
 *        In order to avoid further maintenance issues, these 2 functions
 *        are separated
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prPacketInfo   Pointer of CMD_INFO_T
 * @param ucTC           Specify the resource of TC
 *
 * @retval WLAN_STATUS_SUCCESS   Bus access ok.
 * @retval WLAN_STATUS_FAILURE   Bus access fail.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxInitCmd(IN struct ADAPTER *prAdapter,
		      IN struct CMD_INFO *prCmdInfo, IN uint16_t u2Port)
{
	uint16_t u2OverallBufferLength;
	/* Pointer to Transmit Data Structure Frame */
	uint8_t *pucOutputBuf = (uint8_t *) NULL;
	struct TX_CTRL *prTxCtrl;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prChipInfo = prAdapter->chip_info;
	prTxCtrl = &prAdapter->rTxCtrl;
	pucOutputBuf = prTxCtrl->pucTxCoalescingBufPtr;
	u2OverallBufferLength = TFCB_FRAME_PAD_TO_DW((
		prCmdInfo->u2InfoBufLen + prChipInfo->u2HifTxdSize) &
		(uint16_t)
		HIF_TX_HDR_TX_BYTE_COUNT_MASK);

	/* <0> Copy HIF TXD if need */
	if (prCmdInfo->ucCID) {
		HAL_WRITE_HIF_TXD(prChipInfo, pucOutputBuf,
			prCmdInfo->u2InfoBufLen,
			TXD_PKT_FORMAT_COMMAND);
	} else {
		/* 0 means firmware download */
		HAL_WRITE_HIF_TXD(prChipInfo, pucOutputBuf,
			prCmdInfo->u2InfoBufLen,
			TXD_PKT_FORMAT_FWDL);
	}

	/* <1> Copy CMD Header to command buffer
	 * (by using pucCoalescingBufCached)
	 */
	kalMemCopy((void *)&pucOutputBuf[prChipInfo->u2HifTxdSize],
		   (void *) prCmdInfo->pucInfoBuffer, prCmdInfo->u2InfoBufLen);

	ASSERT(u2OverallBufferLength <=
	       prAdapter->u4CoalescingBufCachedSize);

	/* <2> Write frame to data port */
	HAL_WRITE_TX_PORT(prAdapter, u2Port/*NIC_TX_INIT_CMD_PORT*/,
			  (uint32_t) u2OverallBufferLength,
			  (uint8_t *) pucOutputBuf,
			  (uint32_t) prAdapter->u4CoalescingBufCachedSize);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief In this function, we'll reset TX resource counter to initial
 *        value used in F/W download state
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @retval WLAN_STATUS_SUCCESS   Reset is done successfully.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxInitResetResource(IN struct ADAPTER
				*prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t ucIdx;
	uint32_t u4MaxDataPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame;

	uint32_t u4MaxCmdPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame;

	DEBUGFUNC("nicTxInitResetResource");

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

	/* Delta page count */
	kalMemZero(prTxCtrl->rTc.au4TxDonePageCount,
		   sizeof(prTxCtrl->rTc.au4TxDonePageCount));
	kalMemZero(prTxCtrl->rTc.au4PreUsedPageCount,
		   sizeof(prTxCtrl->rTc.au4PreUsedPageCount));
	prTxCtrl->rTc.ucNextTcIdx = TC0_INDEX;
	prTxCtrl->rTc.u4AvaliablePageCount = 0;

	/* Page count */
	prTxCtrl->rTc.au4MaxNumOfPage[TC0_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC0;
	prTxCtrl->rTc.au4FreePageCount[TC0_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC0;

	prTxCtrl->rTc.au4MaxNumOfPage[TC1_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC1;
	prTxCtrl->rTc.au4FreePageCount[TC1_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC1;

	prTxCtrl->rTc.au4MaxNumOfPage[TC2_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC2;
	prTxCtrl->rTc.au4FreePageCount[TC2_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC2;

	prTxCtrl->rTc.au4MaxNumOfPage[TC3_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC3;
	prTxCtrl->rTc.au4FreePageCount[TC3_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC3;

	prTxCtrl->rTc.au4MaxNumOfPage[TC4_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC4;
	prTxCtrl->rTc.au4FreePageCount[TC4_INDEX] =
		NIC_TX_INIT_PAGE_COUNT_TC4;

	/* Buffer count */
	for (ucIdx = TC0_INDEX; ucIdx < TC_NUM; ucIdx++) {
		if (ucIdx == TC4_INDEX) {
			prTxCtrl->rTc.au4MaxNumOfBuffer[ucIdx] =
				prTxCtrl->rTc.au4MaxNumOfPage[ucIdx] /
					u4MaxCmdPageCntPerFrame;
			prTxCtrl->rTc.au4FreeBufferCount[ucIdx] =
				prTxCtrl->rTc.au4FreePageCount[ucIdx] /
					u4MaxCmdPageCntPerFrame;
		} else {
			prTxCtrl->rTc.au4MaxNumOfBuffer[ucIdx] =
				prTxCtrl->rTc.au4MaxNumOfPage[ucIdx] /
					u4MaxDataPageCntPerFrame;
			prTxCtrl->rTc.au4FreeBufferCount[ucIdx] =
				prTxCtrl->rTc.au4FreePageCount[ucIdx] /
					u4MaxDataPageCntPerFrame;
		}
	}

	/* Default bitmap */
	prTxCtrl->rTc.au4PseCtrlEnMap = BITS(TC0_INDEX, TC_NUM-1);
	prTxCtrl->rTc.au4PleCtrlEnMap = BITS(TC0_INDEX, TC_NUM-1);

	return WLAN_STATUS_SUCCESS;

}

#endif

u_int8_t nicTxProcessMngPacket(IN struct ADAPTER *prAdapter,
			       IN struct MSDU_INFO *prMsduInfo)
{
#if 0
	uint16_t u2RateCode;
#endif
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	if (prMsduInfo->eSrc != TX_PACKET_MGMT)
		return FALSE;

	/* Sanity check */
	if (!prMsduInfo->prPacket)
		return FALSE;

	if (!prMsduInfo->u2FrameLength)
		return FALSE;

	if (!prMsduInfo->ucMacHeaderLength)
		return FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prMsduInfo->ucBssIndex);
	prStaRec = cnmGetStaRecByIndex(prAdapter,
				       prMsduInfo->ucStaRecIndex);

	/* MMPDU: force stick to TC4 */
	if (prMsduInfo->fgMgmtUseDataQ)
		prMsduInfo->ucTC = TC0_INDEX;
	else
		prMsduInfo->ucTC = TC4_INDEX;

	/* No Tx descriptor template for MMPDU */
	prMsduInfo->fgIsTXDTemplateValid = FALSE;

	/* Fixed Rate */
	if (prMsduInfo->ucRateMode == MSDU_RATE_MODE_AUTO &&
		prMsduInfo->fgMgmtUseDataQ != TRUE) {
#if 0
		prMsduInfo->ucRateMode = MSDU_RATE_MODE_MANUAL_DESC;

		if (prStaRec)
			u2RateCode = prStaRec->u2HwDefaultFixedRateCode;
		else
			u2RateCode = prBssInfo->u2HwDefaultFixedRateCode;

		nicTxSetPktFixedRateOption(prMsduInfo, u2RateCode,
					   FIX_BW_NO_FIXED, FALSE, FALSE);
#else
		nicTxSetPktLowestFixedRate(prAdapter, prMsduInfo);
#endif
	}

	nicTxFillDesc(prAdapter, prMsduInfo,
		      prMsduInfo->aucTxDescBuffer, NULL);
	return TRUE;
}

void nicTxProcessTxDoneEvent(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent)
{
	struct EVENT_TX_DONE *prTxDone;
	struct MSDU_INFO *prMsduInfo;
	struct TX_CTRL *prTxCtrl = &prAdapter->rTxCtrl;

	uint8_t *apucBandwidt[4] = {
		(uint8_t *)"20", (uint8_t *)"40",
		(uint8_t *)"80", (uint8_t *)"160/80+80"};

	prTxDone = (struct EVENT_TX_DONE *) (prEvent->aucBuffer);

/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
	if (prTxDone->ucStatus != WLAN_STATUS_SUCCESS) {
		prAdapter->total_tx_done_fail_count++;
		if (prTxDone->ucStatus < TX_RESULT_NUM)
			prAdapter->tx_done_fail_count[prTxDone->ucStatus]++;
	}
#endif /* fos_change end */

	if (prTxDone->ucFlag & BIT(TXS_WITH_ADVANCED_INFO)) {
		/* Tx Done with advanced info */
		if (prTxDone->ucStatus != 0)
			DBGLOG_LIMITED(NIC, INFO,
				"EVENT_ID_TX_DONE WIDX:PID[%u:%u] Status[%u:%s] SN[%u] TID[%u] CNT[%u] Flush[%u]\n",
				prTxDone->ucWlanIndex, prTxDone->ucPacketSeq,
				prTxDone->ucStatus,
				apucTxResultStr[prTxDone->ucStatus],
				prTxDone->u2SequenceNumber, prTxDone->ucTid,
				prTxDone->ucTxCount, prTxDone->ucFlushReason);
		else
			DBGLOG(NIC, TRACE,
				"EVENT_ID_TX_DONE WIDX:PID[%u:%u] Status[%u:%s] SN[%u] TID[%u] CNT[%u] Flush[%u]\n",
				prTxDone->ucWlanIndex, prTxDone->ucPacketSeq,
				prTxDone->ucStatus,
				apucTxResultStr[prTxDone->ucStatus],
				prTxDone->u2SequenceNumber, prTxDone->ucTid,
				prTxDone->ucTxCount, prTxDone->ucFlushReason);

		if (prTxDone->ucFlag & BIT(TXS_IS_EXIST)) {
			uint8_t ucNss, ucStbc;
			int8_t icTxPwr;
			uint32_t *pu4RawTxs;

			pu4RawTxs = (uint32_t *)&prTxDone->aucRawTxS[0];

			ucNss = (prTxDone->u2TxRate & TX_DESC_NSTS_MASK) >>
				TX_DESC_NSTS_OFFSET;
			ucNss += 1;
			ucStbc = (prTxDone->u2TxRate & TX_DESC_STBC) ?
								TRUE : FALSE;

			if (ucStbc)
				ucNss /= 2;

			if (prTxDone->ucStatus != 0)
				DBGLOG_LIMITED(NIC, INFO,
					"||RATE[0x%04x] BW[%s] NSS[%u] ArIdx[%u] RspRate[0x%02x]\n",
					prTxDone->u2TxRate,
					apucBandwidt[prTxDone->ucBandwidth],
					ucNss,
					prTxDone->ucRateTableIdx,
					prTxDone->ucRspRate);
			else
				DBGLOG(NIC, TRACE,
					"||RATE[0x%04x] BW[%s] NSS[%u] ArIdx[%u] RspRate[0x%02x]\n",
					prTxDone->u2TxRate,
					apucBandwidt[prTxDone->ucBandwidth],
					ucNss,
					prTxDone->ucRateTableIdx,
					prTxDone->ucRspRate);
			icTxPwr = (int8_t)prTxDone->ucTxPower;
			if (icTxPwr & BIT(6))
				icTxPwr |= BIT(7);

			if (prTxDone->ucStatus != 0)
				DBGLOG_LIMITED(NIC, INFO,
					"||AMPDU[%u] PS[%u] IBF[%u] EBF[%u] TxPwr[%d%sdBm] TSF[%u] TxDelay[%uus]\n",
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_IN_AMPDU_FORMAT) ?
						TRUE : FALSE,
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_PS_BIT) ? TRUE : FALSE,
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_IMP_BF) ? TRUE : FALSE,
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_EXP_BF) ? TRUE : FALSE,
					icTxPwr / 2, icTxPwr & BIT(0) ?
						".5" : "",
					prTxDone->u4Timestamp,
					prTxDone->u4TxDelay);
			else
				DBGLOG(NIC, TRACE,
					"||AMPDU[%u] PS[%u] IBF[%u] EBF[%u] TxPwr[%d%sdBm] TSF[%u] TxDelay[%uus]\n",
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_IN_AMPDU_FORMAT) ?
						TRUE : FALSE,
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_PS_BIT) ? TRUE : FALSE,
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_IMP_BF) ? TRUE : FALSE,
					prTxDone->u4AppliedFlag &
					BIT(TX_FRAME_EXP_BF) ? TRUE : FALSE,
					icTxPwr / 2, icTxPwr & BIT(0) ?
						".5" : "",
					prTxDone->u4Timestamp,
					prTxDone->u4TxDelay);
#ifndef CFG_SUPPORT_UNIFIED_COMMAND
			if (prTxDone->ucStatus != 0)
				DBGLOG_LIMITED(NIC, INFO,
					"TxS[%08x %08x %08x %08x %08x %08x %08x]\n",
					*pu4RawTxs,
					*(pu4RawTxs + 1), *(pu4RawTxs + 2),
					*(pu4RawTxs + 3), *(pu4RawTxs + 4),
					*(pu4RawTxs + 5), *(pu4RawTxs + 6));
			else
				DBGLOG(NIC, TRACE,
					"TxS[%08x %08x %08x %08x %08x %08x %08x]\n",
					*pu4RawTxs,
					*(pu4RawTxs + 1), *(pu4RawTxs + 2),
					*(pu4RawTxs + 3), *(pu4RawTxs + 4),
					*(pu4RawTxs + 5), *(pu4RawTxs + 6));
#endif
		}
	} else {
		DBGLOG(NIC, TRACE,
		       "EVENT_ID_TX_DONE WIDX:PID[%u:%u] Status[%u:%s] SN[%u]\n",
		       prTxDone->ucWlanIndex, prTxDone->ucPacketSeq,
		       prTxDone->ucStatus,
		       apucTxResultStr[prTxDone->ucStatus],
		       prTxDone->u2SequenceNumber);
	}

	/* call related TX Done Handler */
	prMsduInfo = nicGetPendingTxMsduInfo(prAdapter,
					     prTxDone->ucWlanIndex,
					     prTxDone->ucPacketSeq);

#if CFG_SUPPORT_802_11V_TIMING_MEASUREMENT
	DBGLOG(NIC, TRACE,
	       "EVENT_ID_TX_DONE u4TimeStamp = %x u2AirDelay = %x\n",
	       prTxDone->au4Reserved1, prTxDone->au4Reserved2);

	wnmReportTimingMeas(prAdapter, prMsduInfo->ucStaRecIndex,
			    prTxDone->au4Reserved1,
			    prTxDone->au4Reserved1 + prTxDone->au4Reserved2);
#endif

#if CFG_SUPPORT_WIFI_SYSDVT
	if (is_frame_test(prAdapter, 1) != 0) {
		prAdapter->auto_dvt->txs.received_pid = prTxDone->ucPacketSeq;
		receive_del_txs_queue(prTxDone->u2SequenceNumber,
			prTxDone->ucPacketSeq, prTxDone->ucWlanIndex,
			prTxDone->u4Timestamp);
		DBGLOG(REQ, LOUD,
			"Done receive_del_txs_queue pid=%d timestamp=%d\n",
			prTxDone->ucPacketSeq, prTxDone->u4Timestamp);
	}
#endif

	if (prMsduInfo) {
		prMsduInfo->pfTxDoneHandler(prAdapter, prMsduInfo,
	    (enum ENUM_TX_RESULT_CODE) (prTxDone->ucStatus));

		if (prMsduInfo->eSrc == TX_PACKET_MGMT)
			cnmMgtPktFree(prAdapter, prMsduInfo);
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
		else if (prMsduInfo->prToken)
			prMsduInfo->pfTxDoneHandler = NULL;
#endif
		else {
			nicTxFreePacket(prAdapter, prMsduInfo, FALSE);
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
		}

		if (prTxDone->ucStatus == 0 &&
			prMsduInfo->ucBssIndex < MAX_BSSID_NUM)
			GET_CURRENT_SYSTIME(
				&prTxCtrl->u4LastTxTime
				[prMsduInfo->ucBssIndex]);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function enqueues MSDU_INFO_T into queue management,
 *        or command queue
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *        prMsduInfo     Pointer to MSDU
 *
 * @retval WLAN_STATUS_SUCCESS   Reset is done successfully.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxEnqueueMsdu(IN struct ADAPTER *prAdapter,
			  IN struct MSDU_INFO *prMsduInfo)
{
	struct TX_CTRL *prTxCtrl;
	struct MSDU_INFO *prNextMsduInfo, *prRetMsduInfo,
		       *prMsduInfoHead;
	struct QUE qDataPort0, qDataPort1;
	struct QUE *prDataPort0, *prDataPort1;
	struct CMD_INFO *prCmdInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	prDataPort0 = &qDataPort0;
	prDataPort1 = &qDataPort1;

	QUEUE_INITIALIZE(prDataPort0);
	QUEUE_INITIALIZE(prDataPort1);

	/* check how many management frame are being queued */
	while (prMsduInfo) {
		prNextMsduInfo = (struct MSDU_INFO *) QUEUE_GET_NEXT_ENTRY((
					 struct QUE_ENTRY *) prMsduInfo);

		QUEUE_GET_NEXT_ENTRY((struct QUE_ENTRY *) prMsduInfo) =
			NULL;

		if (prMsduInfo->eSrc == TX_PACKET_MGMT) {
			if (nicTxProcessMngPacket(prAdapter, prMsduInfo)) {
				/* Valid MGMT */
				QUEUE_INSERT_TAIL(prDataPort1,
					(struct QUE_ENTRY *)
						prMsduInfo);
			} else {
				/* Invalid MGMT */
				DBGLOG(TX, WARN,
				       "Invalid MGMT[0x%p] BSS[%u] STA[%u],free it\n",
				       prMsduInfo, prMsduInfo->ucBssIndex,
				       prMsduInfo->ucStaRecIndex);

				cnmMgtPktFree(prAdapter, prMsduInfo);
			}
		} else {
			QUEUE_INSERT_TAIL(prDataPort0,
					  (struct QUE_ENTRY *) prMsduInfo);
		}

		prMsduInfo = prNextMsduInfo;
	}

	if (prDataPort0->u4NumElem) {
		/* send to QM */
		KAL_SPIN_LOCK_DECLARATION();
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
		prRetMsduInfo = qmEnqueueTxPackets(prAdapter,
			(struct MSDU_INFO *) QUEUE_GET_HEAD(prDataPort0));
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
#if ARP_MONITER_ENABLE
		if (prAdapter->cArpNoResponseIdx >= 0) {
#if CFG_SUPPORT_DATA_STALL
			KAL_REPORT_ERROR_EVENT(prAdapter,
				EVENT_ARP_NO_RESPONSE,
				prAdapter->cArpNoResponseIdx);
#endif /* CFG_SUPPORT_DATA_STALL */
			aisBssBeaconTimeout(prAdapter,
				prAdapter->cArpNoResponseIdx);
			prAdapter->cArpNoResponseIdx = -1;
		}
#endif /* ARP_MONITER_ENABLE */
		/* post-process for dropped packets */
		if (prRetMsduInfo) {	/* unable to enqueue */
			nicTxFreeMsduInfoPacket(prAdapter, prRetMsduInfo);
			nicTxReturnMsduInfo(prAdapter, prRetMsduInfo);
		}
	}

	if (prDataPort1->u4NumElem) {
		prMsduInfoHead = (struct MSDU_INFO *) QUEUE_GET_HEAD(
					 prDataPort1);

		if (nicTxGetFreeCmdCount(prAdapter) <
		    NIC_TX_CMD_INFO_RESERVED_COUNT) {
			/* not enough descriptors for sending */
			u4Status = WLAN_STATUS_FAILURE;

			/* free all MSDUs */
			while (prMsduInfoHead) {
				prNextMsduInfo =
					(struct MSDU_INFO *)
						QUEUE_GET_NEXT_ENTRY(
					&prMsduInfoHead->rQueEntry);

				if (prMsduInfoHead->pfTxDoneHandler != NULL) {
					prMsduInfoHead->pfTxDoneHandler(
						prAdapter, prMsduInfoHead,
						TX_RESULT_DROPPED_IN_DRIVER);
				}

				cnmMgtPktFree(prAdapter, prMsduInfoHead);

				prMsduInfoHead = prNextMsduInfo;
			}
		} else {
			/* send to command queue */
			while (prMsduInfoHead) {
				prNextMsduInfo =
					(struct MSDU_INFO *)
						QUEUE_GET_NEXT_ENTRY(
					&prMsduInfoHead->rQueEntry);

				KAL_ACQUIRE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_CMD_RESOURCE);
				QUEUE_REMOVE_HEAD(
					&prAdapter->rFreeCmdList,
					prCmdInfo,
					struct CMD_INFO *);
				KAL_RELEASE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_CMD_RESOURCE);

				if (prCmdInfo) {
					GLUE_INC_REF_CNT(
						prTxCtrl->i4TxMgmtPendingNum);

					kalMemZero(prCmdInfo,
						sizeof(struct CMD_INFO));

#if CFG_ENABLE_PKT_LIFETIME_PROFILE
					/* Tag MGMT enqueue time */
					GET_CURRENT_SYSTIME(
						&prMsduInfoHead->
						rPktProfile.rEnqueueTimestamp);
#endif
					prCmdInfo->eCmdType =
						COMMAND_TYPE_MANAGEMENT_FRAME;
					prCmdInfo->u2InfoBufLen =
						prMsduInfoHead->u2FrameLength;
					prCmdInfo->pucInfoBuffer = NULL;
					prCmdInfo->prMsduInfo = prMsduInfoHead;
					prCmdInfo->pfCmdDoneHandler = NULL;
					prCmdInfo->pfCmdTimeoutHandler = NULL;
					prCmdInfo->fgIsOid = FALSE;
					prCmdInfo->fgSetQuery = TRUE;
					prCmdInfo->fgNeedResp = FALSE;
					prCmdInfo->ucCmdSeqNum =
						prMsduInfoHead->ucTxSeqNum;

					DBGLOG(TX, TRACE,
						"%s: EN-Q MSDU[0x%p] SEQ[%u] BSS[%u] STA[%u] to CMD Q\n",
					  __func__, prMsduInfoHead,
					  prMsduInfoHead->ucTxSeqNum,
					  prMsduInfoHead->ucBssIndex,
					  prMsduInfoHead->ucStaRecIndex);

					kalEnqueueCommand(prAdapter->prGlueInfo,
						(struct QUE_ENTRY *) prCmdInfo);
				} else {
					/* Cmd free count is larger than
					 * expected, but allocation fail.
					 */
					u4Status = WLAN_STATUS_FAILURE;

					if (prMsduInfoHead->pfTxDoneHandler
							!= NULL) {
						prMsduInfoHead->pfTxDoneHandler(
							prAdapter,
							prMsduInfoHead,
						TX_RESULT_DROPPED_IN_DRIVER);
					}

					cnmMgtPktFree(prAdapter,
						prMsduInfoHead);
				}

				prMsduInfoHead = prNextMsduInfo;
			}
		}
	}

	/* indicate service thread for sending */
	if (prTxCtrl->i4TxMgmtPendingNum > 0
	    || kalGetTxPendingFrameCount(prAdapter->prGlueInfo) > 0)
		kalSetEvent(prAdapter->prGlueInfo);

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function returns WLAN index
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint8_t nicTxGetWlanIdx(struct ADAPTER *prAdapter,
			uint8_t ucBssIdx, uint8_t ucStaRecIdx)
{
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucWlanIndex = prAdapter->ucTxDefaultWlanIndex;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

	if (prStaRec)
		ucWlanIndex = prStaRec->ucWlanIndex;
	else if ((ucStaRecIdx == STA_REC_INDEX_BMCAST)
		 && prBssInfo->fgIsInUse) {
		if (prBssInfo->fgBcDefaultKeyExist) {
			if (prBssInfo->wepkeyUsed[prBssInfo->ucBcDefaultKeyIdx]
				&& prBssInfo->wepkeyWlanIdx
					< prAdapter->ucTxDefaultWlanIndex)
				ucWlanIndex = prBssInfo->wepkeyWlanIdx;
			else if (prBssInfo->ucBMCWlanIndexSUsed[
				prBssInfo->ucBcDefaultKeyIdx])
				ucWlanIndex =
					prBssInfo->ucBMCWlanIndexS[
						prBssInfo->ucBcDefaultKeyIdx];
		} else
			ucWlanIndex = prBssInfo->ucBMCWlanIndex;
	}

	if (ucWlanIndex >= WTBL_SIZE) {
		DBGLOG(TX, WARN,
		       "Unexpected WIDX[%u] BSS[%u] STA[%u], set WIDX to default value[%u]\n",
		       ucWlanIndex, ucBssIdx, ucStaRecIdx,
		       prAdapter->ucTxDefaultWlanIndex);

		ucWlanIndex = prAdapter->ucTxDefaultWlanIndex;
	}

	return ucWlanIndex;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
u_int8_t nicTxIsMgmtResourceEnough(IN struct ADAPTER
				   *prAdapter)
{
	if (nicTxGetFreeCmdCount(prAdapter) >
	    (CFG_TX_MAX_CMD_PKT_NUM / 2))
		return TRUE;
	else
		return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function returns available count in command queue
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxGetFreeCmdCount(IN struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	return prAdapter->rFreeCmdList.u4NumElem;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function returns page count of frame
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param u4FrameLength      frame length
 *
 * @retval page count of this frame
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxGetDataPageCount(IN struct ADAPTER *prAdapter,
			   IN uint32_t u4FrameLength, IN u_int8_t fgIncludeDesc)
{
	return halTxGetDataPageCount(prAdapter, u4FrameLength, fgIncludeDesc);
}

uint32_t nicTxGetCmdPageCount(IN struct ADAPTER *prAdapter,
			      IN struct CMD_INFO *prCmdInfo)
{
	uint32_t u4PageCount;

	switch (prCmdInfo->eCmdType) {
	case COMMAND_TYPE_NETWORK_IOCTL:
	case COMMAND_TYPE_GENERAL_IOCTL:
		u4PageCount = halTxGetCmdPageCount(prAdapter,
						prCmdInfo->u2InfoBufLen, TRUE);
		break;

	case COMMAND_TYPE_SECURITY_FRAME:
	case COMMAND_TYPE_MANAGEMENT_FRAME:
		/* No TxD append field for management packet */
		u4PageCount = halTxGetCmdPageCount(prAdapter,
			prCmdInfo->u2InfoBufLen +
			NIC_TX_DESC_LONG_FORMAT_LENGTH, TRUE);
		break;

	default:
		DBGLOG(INIT, WARN, "Undefined CMD Type(%u)\n",
		       prCmdInfo->eCmdType);
		u4PageCount = halTxGetCmdPageCount(prAdapter,
						prCmdInfo->u2InfoBufLen, FALSE);
		break;
	}

	return u4PageCount;

}

void nicTxSetMngPacket(struct ADAPTER *prAdapter,
		       struct MSDU_INFO *prMsduInfo,
		       uint8_t ucBssIndex, uint8_t ucStaRecIndex,
		       uint8_t ucMacHeaderLength,
		       uint16_t u2FrameLength,
		       PFN_TX_DONE_HANDLER pfTxDoneHandler,
		       uint8_t ucRateMode)
{
#if CFG_SUPPORT_NAN
	struct WLAN_MAC_HEADER *prWifiHdr;
#endif
	ASSERT(prMsduInfo);

	prMsduInfo->ucBssIndex = ucBssIndex;
	prMsduInfo->ucStaRecIndex = ucStaRecIndex;
	prMsduInfo->ucMacHeaderLength = ucMacHeaderLength;
	prMsduInfo->u2FrameLength = u2FrameLength;
	prMsduInfo->pfTxDoneHandler = pfTxDoneHandler;
	prMsduInfo->ucRateMode = ucRateMode;

	/* Reset default value for MMPDU */
	prMsduInfo->fgIs802_11 = TRUE;
	prMsduInfo->fgIs802_1x = FALSE;
	prMsduInfo->fgIs802_1x_NonProtected =
		TRUE; /*For data frame only, no sense for management frame*/
	prMsduInfo->u4FixedRateOption = 0;
	prMsduInfo->cPowerOffset = 0;
	prMsduInfo->u4Option = 0;
	prMsduInfo->ucTxSeqNum = nicIncreaseTxSeqNum(prAdapter);
	prMsduInfo->ucPID = NIC_TX_DESC_PID_RESERVED;
	prMsduInfo->ucPacketType = TX_PACKET_TYPE_MGMT;
	prMsduInfo->ucUserPriority = 0;
	prMsduInfo->eSrc = TX_PACKET_MGMT;
#if CFG_SUPPORT_NAN
	prWifiHdr =
		(struct WLAN_MAC_HEADER *)((uint8_t *)(prMsduInfo->prPacket) +
					       MAC_TX_RESERVED_FIELD);

	if (IS_BMCAST_MAC_ADDR(prWifiHdr->aucAddr1)) {
		prMsduInfo->ucStaRecIndex = STA_REC_INDEX_BMCAST;
		if (pfTxDoneHandler != NULL) {
			prMsduInfo->pfTxDoneHandler = NULL;
			DBGLOG(TX, WARN,
			       "TX done handler can't use for BMC case\n");
		}
	}
#endif
}

void nicTxSetDataPacket(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t ucBssIndex, uint8_t ucStaRecIndex,
	uint8_t ucMacHeaderLength,
	uint16_t u2FrameLength, PFN_TX_DONE_HANDLER pfTxDoneHandler,
	uint8_t ucRateMode, enum ENUM_TX_PACKET_SRC eSrc,
	uint8_t ucTID,
	u_int8_t fgIs802_11Frame, u_int8_t fgIs1xFrame)
{
	ASSERT(prMsduInfo);

	prMsduInfo->ucBssIndex = ucBssIndex;
	prMsduInfo->ucStaRecIndex = ucStaRecIndex;
	prMsduInfo->ucMacHeaderLength = ucMacHeaderLength;
	prMsduInfo->u2FrameLength = u2FrameLength;
	prMsduInfo->pfTxDoneHandler = pfTxDoneHandler;
	prMsduInfo->ucRateMode = ucRateMode;
	prMsduInfo->ucUserPriority = ucTID;
	prMsduInfo->fgIs802_11 = fgIs802_11Frame;
	prMsduInfo->eSrc = eSrc;
	prMsduInfo->fgIs802_1x = fgIs1xFrame;

	/* Reset default value for data packet */
	prMsduInfo->u4FixedRateOption = 0;
	prMsduInfo->cPowerOffset = 0;
	prMsduInfo->u4Option = 0;
	prMsduInfo->ucTxSeqNum = nicIncreaseTxSeqNum(prAdapter);
	prMsduInfo->ucPID = NIC_TX_DESC_PID_RESERVED;
	prMsduInfo->ucPacketType = TX_PACKET_TYPE_DATA;
}

void nicTxFillDescByPktOption(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxDesc)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_fill_by_pkt_option)
		prTxDescOps->nic_txd_fill_by_pkt_option(
			prMsduInfo,
			prTxDesc);
	else
		DBGLOG(TX, ERROR, "%s:: no nic_txd_fill_by_pkt_option??\n",
			__func__);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Extra configuration for Tx packet
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
void nicTxConfigPktOption(struct MSDU_INFO *prMsduInfo,
			  uint32_t u4OptionMask, u_int8_t fgSetOption)
{
	if (fgSetOption)
		prMsduInfo->u4Option |= u4OptionMask;
	else
		prMsduInfo->u4Option &= ~u4OptionMask;
}

void nicTxConfigPktControlFlag(struct MSDU_INFO *prMsduInfo,
			       uint8_t ucControlFlagMask, u_int8_t fgSetFlag)
{
	/* Set control flag */
	if (fgSetFlag)
		prMsduInfo->ucControlFlag |= ucControlFlagMask;
	else
		prMsduInfo->ucControlFlag &=
			~ucControlFlagMask;	/* Clear control flag */
}

void nicTxSetPktLifeTime(struct MSDU_INFO *prMsduInfo,
			 uint32_t u4TxLifeTimeInMs)
{
	prMsduInfo->u4RemainingLifetime = u4TxLifeTimeInMs;
	prMsduInfo->u4Option |= MSDU_OPT_MANUAL_LIFE_TIME;
}

void nicTxSetPktRetryLimit(struct MSDU_INFO *prMsduInfo,
			   uint8_t ucRetryLimit)
{
	prMsduInfo->ucRetryLimit = ucRetryLimit;
	prMsduInfo->u4Option |= MSDU_OPT_MANUAL_RETRY_LIMIT;
}

void nicTxSetPktPowerOffset(struct MSDU_INFO *prMsduInfo,
			    int8_t cPowerOffset)
{
	prMsduInfo->cPowerOffset = cPowerOffset;
	prMsduInfo->u4Option |= MSDU_OPT_MANUAL_POWER_OFFSET;
}

void nicTxSetPktSequenceNumber(struct MSDU_INFO *prMsduInfo,
			       uint16_t u2SN)
{
	prMsduInfo->u2SwSN = u2SN;
	prMsduInfo->u4Option |= MSDU_OPT_MANUAL_SN;
}

void nicTxSetPktMacTxQue(struct MSDU_INFO *prMsduInfo,
			 uint8_t ucMacTxQue)
{
	uint8_t ucTcIdx;

	for (ucTcIdx = TC0_INDEX; ucTcIdx < TC_NUM; ucTcIdx++) {
		if (arTcResourceControl[ucTcIdx].ucDestQueueIndex ==
		    ucMacTxQue)
			break;
	}

	if (ucTcIdx < TC_NUM) {
		prMsduInfo->ucTC = ucTcIdx;
		prMsduInfo->u4Option |= MSDU_OPT_MANUAL_TX_QUE;
	}
}

void nicTxSetPktFixedRateOptionFull(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2RateCode,
	uint8_t ucBandwidth,
	u_int8_t fgShortGI,
	u_int8_t fgLDPC,
	u_int8_t fgDynamicBwRts,
	u_int8_t fgBeamforming,
	uint8_t ucAntennaIndex)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_set_pkt_fixed_rate_option_full)
		prTxDescOps->nic_txd_set_pkt_fixed_rate_option_full(
			prMsduInfo,
			u2RateCode,
			ucBandwidth,
			fgShortGI,
			fgLDPC,
			fgDynamicBwRts,
			fgBeamforming,
			ucAntennaIndex);
	else
		DBGLOG(TX, ERROR,
			"%s:: no nic_txd_set_pkt_fixed_rate_option_full??\n",
			__func__);
}

void nicTxSetPktFixedRateOption(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2RateCode,
	uint8_t ucBandwidth,
	u_int8_t fgShortGI,
	u_int8_t fgDynamicBwRts)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_set_pkt_fixed_rate_option)
		prTxDescOps->nic_txd_set_pkt_fixed_rate_option(
			prMsduInfo,
			u2RateCode,
			ucBandwidth,
			fgShortGI,
			fgDynamicBwRts);
	else
		DBGLOG(TX, ERROR,
			"%s:: no nicTxSetPktFixedRateOption??\n",
			__func__);
}

void nicTxSetPktLowestFixedRate(IN struct ADAPTER
				*prAdapter, IN struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(
					     prAdapter, prMsduInfo->ucBssIndex);
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(prAdapter,
				      prMsduInfo->ucStaRecIndex);
	uint8_t ucRateSwIndex, ucRateIndex, ucRatePreamble;
	uint16_t u2RateCode, u2RateCodeLimit, u2OperationalRateSet;
	uint32_t u4CurrentPhyRate, u4Status;

	/* Not to use TxD template for fixed rate */
	prMsduInfo->fgIsTXDTemplateValid = FALSE;

	/* Fixed Rate */
	prMsduInfo->ucRateMode = MSDU_RATE_MODE_MANUAL_DESC;

	if (prStaRec) {
		u2RateCode = prStaRec->u2HwDefaultFixedRateCode;
		u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	} else {
		u2RateCode = prBssInfo->u2HwDefaultFixedRateCode;
		u2OperationalRateSet = prBssInfo->u2OperationalRateSet;
	}

	/* CoexPhyRateLimit is 0 means phy rate is unlimited */
	if (prBssInfo->u4CoexPhyRateLimit != 0) {

		u4CurrentPhyRate = nicRateCode2PhyRate(u2RateCode,
			FIX_BW_NO_FIXED, MAC_GI_NORMAL, AR_SS_NULL);

		if (prBssInfo->u4CoexPhyRateLimit > u4CurrentPhyRate) {
			nicGetRateIndexFromRateSetWithLimit(
				u2OperationalRateSet,
				prBssInfo->u4CoexPhyRateLimit,
				TRUE, &ucRateSwIndex);

			/* Convert SW rate index to rate code */
			nicSwIndex2RateIndex(ucRateSwIndex, &ucRateIndex,
					     &ucRatePreamble);
			u4Status = nicRateIndex2RateCode(ucRatePreamble,
				ucRateIndex, &u2RateCodeLimit);
			if (u4Status == WLAN_STATUS_SUCCESS) {
				/* Replace by limitation rate */
				u2RateCode = u2RateCodeLimit;
				DBGLOG(NIC, INFO,
				       "Coex RatePreamble=%d, R_SW_IDX:%d, R_CODE:0x%x\n",
				       ucRatePreamble, ucRateIndex, u2RateCode);
			}
		}
	}
	nicTxSetPktFixedRateOption(prAdapter, prMsduInfo, u2RateCode,
				   FIX_BW_NO_FIXED, FALSE, FALSE);
}

void nicTxSetPktMoreData(struct MSDU_INFO
			 *prCurrentMsduInfo, u_int8_t fgSetMoreDataBit)
{
	struct WLAN_MAC_HEADER *prWlanMacHeader = NULL;

	if (prCurrentMsduInfo->fgIs802_11) {
		prWlanMacHeader =
			(struct WLAN_MAC_HEADER *) (((uint8_t *) (
			prCurrentMsduInfo->prPacket)) + MAC_TX_RESERVED_FIELD);
	}

	if (fgSetMoreDataBit) {
		if (!prCurrentMsduInfo->fgIs802_11)
			prCurrentMsduInfo->u4Option |= MSDU_OPT_MORE_DATA;
		else
			prWlanMacHeader->u2FrameCtrl |= MASK_FC_MORE_DATA;
	} else {
		if (!prCurrentMsduInfo->fgIs802_11)
			prCurrentMsduInfo->u4Option &= ~MSDU_OPT_MORE_DATA;
		else
			prWlanMacHeader->u2FrameCtrl &= ~MASK_FC_MORE_DATA;
	}
}

uint8_t nicTxAssignPID(IN struct ADAPTER *prAdapter,
		       IN uint8_t ucWlanIndex)
{
	uint8_t ucRetval;
	uint8_t *pucPidPool;

	ASSERT(prAdapter);

	pucPidPool = &prAdapter->aucPidPool[ucWlanIndex];

	ucRetval = *pucPidPool;

	/* Driver side Tx Sequence number: 1~127 */
	(*pucPidPool)++;

	if (*pucPidPool > NIC_TX_DESC_DRIVER_PID_MAX)
		*pucPidPool = NIC_TX_DESC_DRIVER_PID_MIN;

	return ucRetval;
}

void nicTxSetPktEOSP(struct MSDU_INFO *prCurrentMsduInfo,
		     u_int8_t fgSetEOSPBit)
{
	struct WLAN_MAC_HEADER_QOS *prWlanMacHeader = NULL;
	u_int8_t fgWriteToDesc = TRUE;

	if (prCurrentMsduInfo->fgIs802_11) {
		prWlanMacHeader =
			(struct WLAN_MAC_HEADER_QOS *) (((uint8_t *) (
			prCurrentMsduInfo->prPacket)) + MAC_TX_RESERVED_FIELD);
		fgWriteToDesc = FALSE;
	}

	if (fgSetEOSPBit) {
		if (fgWriteToDesc)
			prCurrentMsduInfo->u4Option |= MSDU_OPT_EOSP;
		else
			prWlanMacHeader->u2QosCtrl |= MASK_QC_EOSP;
	} else {
		if (fgWriteToDesc)
			prCurrentMsduInfo->u4Option &= ~MSDU_OPT_EOSP;
		else
			prWlanMacHeader->u2QosCtrl &= ~MASK_QC_EOSP;
	}
}

uint32_t
nicTxDummyTxDone(IN struct ADAPTER *prAdapter,
		 IN struct MSDU_INFO *prMsduInfo,
		 IN enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct PERF_MONITOR_T *prPerMonitor = &prAdapter->rPerMonitor;

	if (rTxDoneStatus == 0) {
		prPerMonitor->ulTotalTxSuccessCount++;
	} else {
		DBGLOG(TX, INFO,
			"Msdu WIDX:PID[%u:%u] SEQ[%u] Tx Status[%u]\n",
			prMsduInfo->ucWlanIndex, prMsduInfo->ucPID,
			prMsduInfo->ucTxSeqNum, rTxDoneStatus);
		prPerMonitor->ulTotalTxFailCount++;
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Update BSS Tx Params
 *
 * @param prStaRec The peer
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxUpdateBssDefaultRate(struct BSS_INFO *prBssInfo)
{
	uint8_t ucLowestBasicRateIndex;

	prBssInfo->u2HwDefaultFixedRateCode = RATE_OFDM_6M;

	/* 4 <1> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	if (rateGetLowestRateIndexFromRateSet(
		    prBssInfo->u2BSSBasicRateSet, &ucLowestBasicRateIndex)) {
		nicRateIndex2RateCode(PREAMBLE_DEFAULT_LONG_NONE,
				      ucLowestBasicRateIndex,
				      &prBssInfo->u2HwDefaultFixedRateCode);
	} else {
		switch (prBssInfo->ucNonHTBasicPhyType) {
		case PHY_TYPE_ERP_INDEX:
		case PHY_TYPE_OFDM_INDEX:
			prBssInfo->u2HwDefaultFixedRateCode = RATE_OFDM_6M;
			break;

		default:
			prBssInfo->u2HwDefaultFixedRateCode = RATE_CCK_1M_LONG;
			break;
		}
	}

#if (CFG_SUPPORT_HE_ER == 1)
	if (prBssInfo->ucErMode == RA_DCM) {
		prBssInfo->u2HwDefaultFixedRateCode = RATE_HE_ER_DCM_MCS_0;
		DBGLOG_LIMITED(TX, WARN,
		"nicTxUpdateBssDefaultRate:HE_ER DCM\n");
	} else if (prBssInfo->ucErMode == RA_ER_106) {
		prBssInfo->u2HwDefaultFixedRateCode = RATE_HE_ER_TONE_106_MCS_0;
		DBGLOG_LIMITED(TX, WARN,
		"nicTxUpdateBssDefaultRate:HE_ER 106 tone\n");
	} else {
		DBGLOG_LIMITED(TX, WARN,
		"nicTxUpdateBssDefaultRate:HE_ER Disable\n");
	}
#endif

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Update StaRec Tx parameters
 *
 * @param prStaRec The peer
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicTxUpdateStaRecDefaultRate(struct ADAPTER *prAdapter, struct STA_RECORD
				  *prStaRec)
{
	uint8_t ucLowestBasicRateIndex;

	prStaRec->u2HwDefaultFixedRateCode = RATE_OFDM_6M;

	/* 4 <1> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	if (rateGetLowestRateIndexFromRateSet(
		    prStaRec->u2BSSBasicRateSet, &ucLowestBasicRateIndex)) {
		nicRateIndex2RateCode(PREAMBLE_DEFAULT_LONG_NONE,
				      ucLowestBasicRateIndex,
				      &prStaRec->u2HwDefaultFixedRateCode);
	} else {
		if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11B)
			prStaRec->u2HwDefaultFixedRateCode = RATE_CCK_1M_LONG;
		else if (prStaRec->ucDesiredPhyTypeSet &
			 PHY_TYPE_SET_802_11G)
			prStaRec->u2HwDefaultFixedRateCode = RATE_OFDM_6M;
		else if (prStaRec->ucDesiredPhyTypeSet &
			 PHY_TYPE_SET_802_11A)
			prStaRec->u2HwDefaultFixedRateCode = RATE_OFDM_6M;
		else if (prStaRec->ucDesiredPhyTypeSet &
			 PHY_TYPE_SET_802_11N)
			prStaRec->u2HwDefaultFixedRateCode = RATE_MM_MCS_0;
	}

#if (CFG_SUPPORT_HE_ER == 1)
	{
		struct BSS_INFO *prBssInfo;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prStaRec->ucBssIndex);

		if (prBssInfo->ucErMode == RA_DCM) {
			prBssInfo->u2HwDefaultFixedRateCode =
				RATE_HE_ER_DCM_MCS_0;
			DBGLOG_LIMITED(TX, WARN,
			"nicTxUpdateStaRecDefaultRate:HE_ER DCM\n");
		} else if (prBssInfo->ucErMode == RA_ER_106) {
			prBssInfo->u2HwDefaultFixedRateCode =
				RATE_HE_ER_TONE_106_MCS_0;
			DBGLOG_LIMITED(TX, WARN,
			"nicTxUpdateStaRecDefaultRate:HE_ER 106 tone\n");
		} else {
			DBGLOG_LIMITED(TX, WARN,
			"nicTxUpdateStaRecDefaultRate:HE_ER Disable\n");
		}
	}
#endif

}

void nicTxCancelSendingCmd(IN struct ADAPTER *prAdapter,
			   IN struct CMD_INFO *prCmdInfo)
{
	halTxCancelSendingCmd(prAdapter, prCmdInfo);
}

uint32_t nicTxGetMaxCmdPageCntPerFrame(IN struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_CMD_OVER_WFDMA == 1)
	return 1;
#else
	return nicTxGetMaxDataPageCntPerFrame(prAdapter);
#endif
}

uint32_t nicTxGetMaxDataPageCntPerFrame(IN struct ADAPTER *prAdapter)
{

	uint32_t page_size = halGetHifTxDataPageSize(prAdapter);

	/*
	 * want to replace
	 *	#define NIC_TX_MAX_PAGE_PER_FRAME \
	 *	 ((NIC_TX_DESC_AND_PADDING_LENGTH +
	 *     NIC_TX_DESC_HEADER_PADDING_LENGTH + \
	 *	 NIC_TX_MAX_SIZE_PER_FRAME + NIC_TX_PAGE_SIZE - 1)
	 *   / NIC_TX_PAGE_SIZE)
	 */

	return ((NIC_TX_DESC_AND_PADDING_LENGTH +
		 NIC_TX_DESC_HEADER_PADDING_LENGTH +
		 NIC_TX_MAX_SIZE_PER_FRAME + page_size - 1) / page_size);

}

#if (CFG_SUPPORT_TX_TSO_SW == 1)
void nicTxSwTsoClearSkbQ(IN struct ADAPTER *prAdapter)
{
	struct sk_buff *prSkb;

	while (TRUE) {
		prSkb = skb_dequeue(&prAdapter->rTsoQueue);
		if (prSkb == NULL)
			break;
		dev_kfree_skb_any(prSkb);
		GLUE_DEC_REF_CNT(prAdapter->u4TsoQueueCnt);
	}
}
#endif

/* TX Direct functions : BEGIN */

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to start rTxDirectHifTimer to try to
 *        send out packets in
 *        rStaPsQueue[], rBssAbsentQueue[], rTxDirectHifQueue[].
 *
 * \param[in] prAdapter   Pointer of Adapter
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void nicTxDirectStartCheckQTimer(IN struct ADAPTER
				 *prAdapter)
{
	mod_timer(&prAdapter->rTxDirectHifTimer, jiffies + 1);
}

void nicTxDirectClearSkbQ(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct sk_buff *prSkb;

	while (TRUE) {
		spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
		prSkb = skb_dequeue(&prAdapter->rTxDirectSkbQueue);
		spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
		if (prSkb == NULL)
			break;

		kalSendComplete(prGlueInfo, prSkb,
				WLAN_STATUS_NOT_ACCEPTED);
	}
}

void nicTxDirectClearHifQ(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint8_t ucHifTc = 0;
	struct QUE rNeedToFreeQue;
	struct QUE *prNeedToFreeQue = &rNeedToFreeQue;

	QUEUE_INITIALIZE(prNeedToFreeQue);

	for (ucHifTc = 0; ucHifTc < TX_PORT_NUM; ucHifTc++) {
		spin_lock_bh(
			 &prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
		if (QUEUE_IS_NOT_EMPTY(
			    &prAdapter->rTxDirectHifQueue[ucHifTc])) {
			QUEUE_MOVE_ALL(prNeedToFreeQue,
				       &prAdapter->rTxDirectHifQueue[ucHifTc]);
			spin_unlock_bh(
				&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

			wlanProcessQueuedMsduInfo(prAdapter,
				(struct MSDU_INFO *)
					QUEUE_GET_HEAD(prNeedToFreeQue));
		} else {
			spin_unlock_bh(
				&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
		}
	}
}

void nicTxDirectClearStaPsQ(IN struct ADAPTER *prAdapter,
			    uint8_t ucStaRecIndex)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct QUE rNeedToFreeQue;
	struct QUE *prNeedToFreeQue = &rNeedToFreeQue;
	spinlock_t *prSpinLock = &prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT];
	bool fgIrqDisabled = irqs_disabled();

	QUEUE_INITIALIZE(prNeedToFreeQue);
	if (fgIrqDisabled)
		spin_lock(prSpinLock);
	else
		spin_lock_bh(prSpinLock);
	if (QUEUE_IS_NOT_EMPTY(
		    &prAdapter->rStaPsQueue[ucStaRecIndex])) {
		QUEUE_MOVE_ALL(prNeedToFreeQue,
			       &prAdapter->rStaPsQueue[ucStaRecIndex]);
		if (fgIrqDisabled)
			spin_unlock(prSpinLock);
		else
			spin_unlock_bh(prSpinLock);

		wlanProcessQueuedMsduInfo(prAdapter,
			(struct MSDU_INFO *) QUEUE_GET_HEAD(prNeedToFreeQue));
	} else {
		if (fgIrqDisabled)
			spin_unlock(prSpinLock);
		else
			spin_unlock_bh(prSpinLock);
	}
}

void nicTxDirectClearBssAbsentQ(IN struct ADAPTER
				*prAdapter, uint8_t ucBssIndex)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct QUE rNeedToFreeQue;
	struct QUE *prNeedToFreeQue = &rNeedToFreeQue;

	QUEUE_INITIALIZE(prNeedToFreeQue);
	spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
	if (QUEUE_IS_NOT_EMPTY(
		    &prAdapter->rBssAbsentQueue[ucBssIndex])) {
		QUEUE_MOVE_ALL(prNeedToFreeQue,
			       &prAdapter->rBssAbsentQueue[ucBssIndex]);
		spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

		wlanProcessQueuedMsduInfo(prAdapter,
			(struct MSDU_INFO *) QUEUE_GET_HEAD(prNeedToFreeQue));
	} else {
	    spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
	}
}

void nicTxDirectClearStaPendQ(IN struct ADAPTER *prAdapter,
			    uint8_t ucStaRecIdx)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct QUE rNeedToFreeQue;
	struct QUE *prNeedToFreeQue = &rNeedToFreeQue;
	spinlock_t *prSpinLock = &prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT];
	bool fgIrqDisabled = irqs_disabled();

	QUEUE_INITIALIZE(prNeedToFreeQue);

	if (fgIrqDisabled)
		spin_lock(prSpinLock);
	else
		spin_lock_bh(prSpinLock);

	if (QUEUE_IS_NOT_EMPTY(
		    &prAdapter->rStaPendQueue[ucStaRecIdx])) {
		QUEUE_MOVE_ALL(prNeedToFreeQue,
			       &prAdapter->rStaPendQueue[ucStaRecIdx]);
	}

	if (fgIrqDisabled)
		spin_unlock(prSpinLock);
	else
		spin_unlock_bh(prSpinLock);

	if (QUEUE_IS_NOT_EMPTY(prNeedToFreeQue)) {
		wlanProcessQueuedMsduInfo(prAdapter,
			(struct MSDU_INFO *) QUEUE_GET_HEAD(prNeedToFreeQue));
	}

	prAdapter->u4StaPendBitmap &= ~BIT(ucStaRecIdx);
}

void nicTxDirectClearAllStaPsQ(IN struct ADAPTER *prAdapter)
{
	uint8_t ucStaRecIndex;
	uint32_t u4StaPsBitmap;

	u4StaPsBitmap = prAdapter->u4StaPsBitmap;

	if (!u4StaPsBitmap)
		return;

	for (ucStaRecIndex = 0; ucStaRecIndex < CFG_STA_REC_NUM;
		++ucStaRecIndex) {
		if (QUEUE_IS_NOT_EMPTY(
			&prAdapter->rStaPsQueue[ucStaRecIndex])) {
			nicTxDirectClearStaPsQ(prAdapter,
				ucStaRecIndex);
			u4StaPsBitmap &= ~BIT(ucStaRecIndex);
		}
		if (u4StaPsBitmap == 0)
			break;
	}
}

void nicTxDirectClearAllStaPendQ(IN struct ADAPTER *prAdapter)
{
	uint8_t ucIdx; /* StaRec Index */

	for (ucIdx = 0; ucIdx < CFG_STA_REC_NUM; ++ucIdx) {
		if (prAdapter->u4StaPendBitmap == 0)
			break;

		if (QUEUE_IS_NOT_EMPTY(&prAdapter->rStaPendQueue[ucIdx])) {
			nicTxDirectClearStaPendQ(prAdapter, ucIdx);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to check the StaRec is in Ps or not,
 *        and store MsduInfo(s) or sent MsduInfo(s) to the next
 *        stage respectively.
 *
 * \param[in] prAdapter   Pointer of Adapter
 * \param[in] prStaRec    Pointer of StaRec
 * \param[in] prQue       Pointer of MsduInfo queue which to be processed
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
static void nicTxDirectCheckStaPsQ(IN struct ADAPTER
	*prAdapter, struct STA_RECORD *prStaRec, struct QUE *prQue)
{
	struct MSDU_INFO *prMsduInfo;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	uint8_t ucStaRecIndex;
	u_int8_t fgReturnStaPsQ = FALSE;

	if (prStaRec == NULL)
		return;

	ucStaRecIndex = prStaRec->ucIndex;

	QUEUE_CONCATENATE_QUEUES(
		&prAdapter->rStaPsQueue[ucStaRecIndex], prQue);
	QUEUE_REMOVE_HEAD(&prAdapter->rStaPsQueue[ucStaRecIndex],
			  prQueueEntry, struct QUE_ENTRY *);
	prMsduInfo = (struct MSDU_INFO *) prQueueEntry;

	if (prMsduInfo == NULL) {
		DBGLOG(TX, INFO, "prMsduInfo empty\n");
		return;
	}

	if (prStaRec->fgIsInPS) {
		KAL_SPIN_LOCK_DECLARATION();

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
		DBGLOG(TX, INFO, "fgIsInPS!\n");
		while (1) {
			if (prStaRec->fgIsQoS && prStaRec->fgIsUapsdSupported &&
			    (prStaRec->ucBmpTriggerAC
						& BIT(prMsduInfo->ucTC))) {
				if (prStaRec->ucFreeQuotaForDelivery > 0) {
					prStaRec->ucFreeQuotaForDelivery--;
					QUEUE_INSERT_TAIL(
						prQue,
						(struct QUE_ENTRY *)
							prMsduInfo);
				} else {
					fgReturnStaPsQ = TRUE;
					break;
				}
			} else {
				if (prStaRec->ucFreeQuotaForNonDelivery > 0) {
					prStaRec->ucFreeQuotaForNonDelivery--;
					QUEUE_INSERT_TAIL(
						prQue,
						(struct QUE_ENTRY *)
							prMsduInfo);
				} else {
					fgReturnStaPsQ = TRUE;
					break;
				}
			}
			if (QUEUE_IS_NOT_EMPTY(
				    &prAdapter->rStaPsQueue[ucStaRecIndex])) {
				QUEUE_REMOVE_HEAD(
					&prAdapter->rStaPsQueue[ucStaRecIndex],
					prQueueEntry, struct QUE_ENTRY *);
				if (prQueueEntry == NULL)
					break;
				prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
			} else {
				break;
			}
		}
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
		if (fgReturnStaPsQ) {
			QUEUE_INSERT_HEAD(
				&prAdapter->rStaPsQueue[ucStaRecIndex],
				(struct QUE_ENTRY *) prMsduInfo);
			prAdapter->u4StaPsBitmap |= BIT(ucStaRecIndex);
			return;
		}
	} else {
		QUEUE_INSERT_TAIL(prQue, (struct QUE_ENTRY *) prMsduInfo);
		if (QUEUE_IS_NOT_EMPTY(
			    &prAdapter->rStaPsQueue[ucStaRecIndex]))
			QUEUE_CONCATENATE_QUEUES(prQue,
				&prAdapter->rStaPsQueue[ucStaRecIndex]);
	}
	prAdapter->u4StaPsBitmap &= ~BIT(ucStaRecIndex);
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to check the Bss is net absent or not,
 *        and store MsduInfo(s) or sent MsduInfo(s) to the next
 *        stage respectively.
 *
 * \param[in] prAdapter   Pointer of Adapter
 * \param[in] ucBssIndex  Indictate which Bss to be checked
 * \param[in] prQue       Pointer of MsduInfo queue which to be processed
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
static void nicTxDirectCheckBssAbsentQ(IN struct ADAPTER
	*prAdapter, uint8_t ucBssIndex, struct QUE *prQue)
{
	struct BSS_INFO *prBssInfo;
	struct MSDU_INFO *prMsduInfo;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	u_int8_t fgReturnBssAbsentQ = FALSE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	QUEUE_CONCATENATE_QUEUES(
		&prAdapter->rBssAbsentQueue[ucBssIndex], prQue);
	QUEUE_REMOVE_HEAD(&prAdapter->rBssAbsentQueue[ucBssIndex],
			  prQueueEntry, struct QUE_ENTRY *);
	prMsduInfo = (struct MSDU_INFO *) prQueueEntry;

	if (prMsduInfo == NULL) {
		DBGLOG(TX, INFO, "prMsduInfo empty\n");
		return;
	}

	if (prBssInfo->fgIsNetAbsent) {
		KAL_SPIN_LOCK_DECLARATION();

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
		DBGLOG(TX, INFO, "fgIsNetAbsent!\n");
		while (1) {
			if (prBssInfo->ucBssFreeQuota > 0) {
				prBssInfo->ucBssFreeQuota--;
				QUEUE_INSERT_TAIL(
					prQue, (struct QUE_ENTRY *) prMsduInfo);
				DBGLOG(TX, INFO,
					"fgIsNetAbsent Quota Availalbe\n");
			} else {
				fgReturnBssAbsentQ = TRUE;
				DBGLOG(TX, INFO, "fgIsNetAbsent NoQuota\n");
				break;
			}
			if (QUEUE_IS_NOT_EMPTY(
				    &prAdapter->rBssAbsentQueue[ucBssIndex])) {
				QUEUE_REMOVE_HEAD(
					&prAdapter->rBssAbsentQueue[ucBssIndex],
					prQueueEntry, struct QUE_ENTRY *);
				prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
			} else {
				break;
			}
		}
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
		if (fgReturnBssAbsentQ) {
			QUEUE_INSERT_HEAD(
				&prAdapter->rBssAbsentQueue[ucBssIndex],
				(struct QUE_ENTRY *) prMsduInfo);
			prAdapter->u4BssAbsentBitmap |= BIT(ucBssIndex);
			return;
		}
	} else {
		if (prAdapter->u4BssAbsentBitmap)
			DBGLOG(TX, INFO, "fgIsNetAbsent END!\n");
		QUEUE_INSERT_TAIL(prQue, (struct QUE_ENTRY *) prMsduInfo);
		if (QUEUE_IS_NOT_EMPTY(
			&prAdapter->rBssAbsentQueue[ucBssIndex]))
			QUEUE_CONCATENATE_QUEUES(prQue,
				&prAdapter->rBssAbsentQueue[ucBssIndex]);
	}
	prAdapter->u4BssAbsentBitmap &= ~BIT(ucBssIndex);
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is for fgIsTxAllowed == TRUE.
 *        The data frame can start tx when the key is added.
 *
 * \param[in] prAdapter   Pointer of Adapter
 * \param[in] prMsduInfo  The prMsduInfo that is wait for tx
 * \param[in] ucStaRecIndex  Indictate which StaRec to be checked
 * \param[in] prQue       Pointer of MsduInfo queue which to be processed
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
static void nicTxDirectDequeueStaPendQ(IN struct ADAPTER *prAdapter,
				uint8_t ucStaIdx, struct QUE *prQue)
{
	KAL_SPIN_LOCK_DECLARATION();

	/* ucStaIdx has been checked in nicTxDirectCheckStaPsPendQ */

	if (prAdapter == NULL)
		return;

	/* the add key done case (include OPEN security) */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
	if (QUEUE_IS_NOT_EMPTY(
		&prAdapter->rStaPendQueue[ucStaIdx])) {
		DBGLOG(TX, TRACE, "start tx pending q!\n");
		QUEUE_CONCATENATE_QUEUES_HEAD(prQue,
			&prAdapter->rStaPendQueue[ucStaIdx]);
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
	prAdapter->u4StaPendBitmap &= ~BIT(ucStaIdx);
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is for fgIsTxAllowed == FALSE.
 *        The Non-EAPol data frame shouldn't tx without the key added,
 *        if the sta isn't OPEN security.
 *
 * \param[in] prAdapter   Pointer of Adapter
 * \param[in] prMsduInfo  The prMsduInfo that is wait for tx
 * \param[in] ucStaRecIndex  Indictate which StaRec to be checked
 * \param[in] prQue       Pointer of MsduInfo queue which to be processed
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
static void nicTxDirectEnqueueStaPendQ(IN struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint8_t ucStaIdx, struct QUE *prQue)
{
	struct BSS_INFO *prBssInfo;

	KAL_SPIN_LOCK_DECLARATION();

	/* the add key isn't completed case */
	if ((prMsduInfo == NULL) || (prAdapter == NULL))
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsduInfo->ucBssIndex);

	if (prBssInfo == NULL) {
		DBGLOG(TX, INFO, "prBssInfo is NULL\n");
		return;
	}

	if (secIsProtectedBss(prAdapter, prBssInfo) &&
	    (prMsduInfo->fgIs802_1x) && (prMsduInfo->fgIs802_1x_NonProtected)) {
		/* The EAPoL frame can't be blocked. */
		DBGLOG(TX, TRACE, "Is EAPoL frame\n");
	} else {
		DBGLOG(TX, TRACE, "fgIsTxAllowed isn't TRUE!\n");
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);
		QUEUE_CONCATENATE_QUEUES(
			&prAdapter->rStaPendQueue[ucStaIdx], prQue);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_RESOURCE);

		prAdapter->u4StaPendBitmap |= BIT(ucStaIdx);
	}
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to check the StaRec is in pending/PS state or not,
 *        and store MsduInfo(s) or sent MsduInfo(s) to the next
 *        stage respectively.
 *        Avoid the data frame tx before the add key done.
 *
 * \param[in] prAdapter   Pointer of Adapter
 * \param[in] prMsduInfo  The prMsduInfo that is wait for tx
 * \param[in] ucStaRecIndex  Indictate which StaRec to be checked
 * \param[in] prQue       Pointer of MsduInfo queue which to be processed
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
static void nicTxDirectCheckStaPsPendQ(IN struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint8_t ucStaIdx, struct QUE *prQue)
{
	struct STA_RECORD *prStaRec;	/* The current focused STA */

	if (ucStaIdx >= CFG_STA_REC_NUM)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaIdx);

	if (prStaRec == NULL) {
		DBGLOG(TX, INFO, "prStaRec empty\n");
		return;
	}

	if (prStaRec->fgIsTxAllowed == TRUE) {
		/* dequeue pending Queue */
		if (prAdapter->u4StaPendBitmap & BIT(ucStaIdx))
			nicTxDirectDequeueStaPendQ(prAdapter, ucStaIdx, prQue);

		/* handle PS queue */
		nicTxDirectCheckStaPsQ(prAdapter, prStaRec, prQue);
	} else {
		/* enqueue to pending queue */
		nicTxDirectEnqueueStaPendQ(prAdapter, prMsduInfo,
					   ucStaIdx, prQue);
	}
}

/*----------------------------------------------------------------------------*/
/*
 * \brief Get Tc for hif port mapping.
 *
 * \param[in] prMsduInfo  Pointer of the MsduInfo
 *
 * \retval Tc which maps to hif port.
 */
/*----------------------------------------------------------------------------*/
static uint8_t nicTxDirectGetHifTc(struct MSDU_INFO
				   *prMsduInfo)
{
	uint8_t ucHifTc = 0;

	if (prMsduInfo->ucWmmQueSet == DBDC_2G_WMM_INDEX) {
		ucHifTc = TX_2G_WMM_PORT_NUM;
	} else {
		if (prMsduInfo->ucTC < TC_NUM)
			ucHifTc = prMsduInfo->ucTC;
		else
			ASSERT(0);
	}
	return ucHifTc;
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is called by nicTxDirectStartXmit()
 *        and nicTxDirectTimerCheckHifQ().
 *        It is the main function to send skb out on HIF bus.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prMsduInfo  Pointer of the MsduInfo
 * \param[in] prAdapter   Pointer of Adapter
 * \param[in] ucCheckTc   Indictate which Tc HifQ to be checked
 * \param[in] ucStaRecIndex  Indictate which StaPsQ to be checked
 * \param[in] ucBssIndex  Indictate which BssAbsentQ to be checked
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxDirectStartXmitMain(struct sk_buff
		*prSkb, struct MSDU_INFO *prMsduInfo,
		struct ADAPTER *prAdapter,
		uint8_t ucCheckTc, uint8_t ucStaRecIndex,
		uint8_t ucBssIndex)
{
	struct STA_RECORD *prStaRec;	/* The current focused STA */
	struct BSS_INFO *prBssInfo;
	uint8_t ucTC = 0, ucHifTc = 0;
	struct QUE *prTxQue = NULL;
	u_int8_t fgDropPacket = FALSE;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct QUE rProcessingQue;
	struct QUE *prProcessingQue = &rProcessingQue;
	uint8_t ucActivedTspec = 0;


	QUEUE_INITIALIZE(prProcessingQue);

	ucActivedTspec =
		wmmHasActiveTspec(aisGetWMMInfo(prAdapter, ucBssIndex));

	if (prSkb) {
		nicTxFillMsduInfo(prAdapter, prMsduInfo, prSkb);

		/* Tx profiling */
		wlanTxProfilingTagMsdu(prAdapter, prMsduInfo,
				       TX_PROF_TAG_DRV_ENQUE);

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  prMsduInfo->ucBssIndex);

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

		if (fgDropPacket) {
			DBGLOG(QM, TRACE,
				"Drop the Packet for inactive Bss %u\n",
				prMsduInfo->ucBssIndex);
			QM_DBG_CNT_INC(prQM, QM_DBG_CNT_31);
			TX_INC_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_BSS_DROP);

			wlanProcessQueuedMsduInfo(prAdapter, prMsduInfo);
			return WLAN_STATUS_FAILURE;
		}

		qmDetermineStaRecIndex(prAdapter, prMsduInfo);

		wlanUpdateTxStatistics(prAdapter, prMsduInfo,
				       FALSE);	/*get per-AC Tx packets */

		switch (prMsduInfo->ucStaRecIndex) {
		case STA_REC_INDEX_BMCAST:
			ucTC = nicTxWmmTc2ResTc(prAdapter,
				prMsduInfo->ucBssIndex, NET_TC_BMC_INDEX);

			/* Always set BMC packet retry limit to unlimited */
			if (!(prMsduInfo->u4Option
						& MSDU_OPT_MANUAL_RETRY_LIMIT))
				nicTxSetPktRetryLimit(prMsduInfo,
					TX_DESC_TX_COUNT_NO_LIMIT);

			QM_DBG_CNT_INC(prQM, QM_DBG_CNT_23);
			break;
		case STA_REC_INDEX_NOT_FOUND:
			/* Drop packet if no STA_REC is found */
			DBGLOG(QM, TRACE, "Drop the Packet for no STA_REC\n");

			TX_INC_CNT(&prAdapter->rTxCtrl, TX_INACTIVE_STA_DROP);
			QM_DBG_CNT_INC(prQM, QM_DBG_CNT_24);
			wlanProcessQueuedMsduInfo(prAdapter, prMsduInfo);
			return WLAN_STATUS_FAILURE;
		default:
			prTxQue = qmDetermineStaTxQueue(prAdapter, prMsduInfo,
							ucActivedTspec, &ucTC);
			break;	/*default */
		}	/* switch (prMsduInfo->ucStaRecIndex) */

		prMsduInfo->ucTC = ucTC;
		prMsduInfo->ucWmmQueSet =
			prBssInfo->ucWmmQueSet; /* to record WMM Set */

		/* Check the Tx descriptor template is valid */
		qmSetTxPacketDescTemplate(prAdapter, prMsduInfo);

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
			if (prMsduInfo->ucRateMode
				== MSDU_RATE_MODE_LOWEST_RATE)
				nicTxSetPktLowestFixedRate(
					prAdapter, prMsduInfo);
			break;
		}

		/* BMC pkt need limited rate according to coex report*/
		if (prMsduInfo->ucStaRecIndex == STA_REC_INDEX_BMCAST)
			nicTxSetPktLowestFixedRate(prAdapter, prMsduInfo);

		nicTxFillDataDesc(prAdapter, prMsduInfo);

		prStaRec = cnmGetStaRecByIndex(prAdapter,
					       prMsduInfo->ucStaRecIndex);

		QUEUE_INSERT_TAIL(prProcessingQue,
				  (struct QUE_ENTRY *) prMsduInfo);

		/* Power-save & TxAllowed STA handling */
		nicTxDirectCheckStaPsPendQ(prAdapter, prMsduInfo,
				prMsduInfo->ucStaRecIndex, prProcessingQue);

		/* Absent BSS handling */
		nicTxDirectCheckBssAbsentQ(prAdapter,
			prMsduInfo->ucBssIndex, prProcessingQue);

		if (QUEUE_IS_EMPTY(prProcessingQue))
			return WLAN_STATUS_SUCCESS;

		if (prProcessingQue->u4NumElem != 1) {
			while (1) {
				QUEUE_REMOVE_HEAD(prProcessingQue, prQueueEntry,
						  struct QUE_ENTRY *);
				if (prQueueEntry == NULL)
					break;
				prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
				ucHifTc = nicTxDirectGetHifTc(prMsduInfo);
				QUEUE_INSERT_TAIL(
					&prAdapter->rTxDirectHifQueue[ucHifTc],
					(struct QUE_ENTRY *) prMsduInfo);
			}
			nicTxDirectStartCheckQTimer(prAdapter);
			return WLAN_STATUS_SUCCESS;
		}

		QUEUE_REMOVE_HEAD(prProcessingQue, prQueueEntry,
				  struct QUE_ENTRY *);
		prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
		ucHifTc = nicTxDirectGetHifTc(prMsduInfo);

		if (QUEUE_IS_NOT_EMPTY(
			    &prAdapter->rTxDirectHifQueue[ucHifTc])) {
			QUEUE_INSERT_TAIL(
				&prAdapter->rTxDirectHifQueue[ucHifTc],
				(struct QUE_ENTRY *) prMsduInfo);
			QUEUE_REMOVE_HEAD(
				&prAdapter->rTxDirectHifQueue[ucHifTc],
				prQueueEntry, struct QUE_ENTRY *);
			prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
		}
	} else {
		if (ucStaRecIndex != 0xff || ucBssIndex != 0xff) {
			/* Power-save STA handling */
			if (ucStaRecIndex != 0xff) {
				nicTxDirectCheckStaPsPendQ(prAdapter,
						NULL,
						ucStaRecIndex,
						prProcessingQue);
			}

			/* Absent BSS handling */
			if (ucBssIndex != 0xff)
				nicTxDirectCheckBssAbsentQ(
					prAdapter, ucBssIndex,
					prProcessingQue);

			if (QUEUE_IS_EMPTY(prProcessingQue))
				return WLAN_STATUS_SUCCESS;

			if (prProcessingQue->u4NumElem != 1) {
				while (1) {
					QUEUE_REMOVE_HEAD(
						prProcessingQue, prQueueEntry,
						struct QUE_ENTRY *);
					if (prQueueEntry == NULL)
						break;
					prMsduInfo =
						(struct MSDU_INFO *)
							prQueueEntry;
					ucHifTc =
						nicTxDirectGetHifTc(prMsduInfo);
					QUEUE_INSERT_TAIL(
						&prAdapter->
						rTxDirectHifQueue[ucHifTc],
						(struct QUE_ENTRY *)
							prMsduInfo);
				}
				nicTxDirectStartCheckQTimer(prAdapter);
				return WLAN_STATUS_SUCCESS;
			}

			QUEUE_REMOVE_HEAD(prProcessingQue, prQueueEntry,
					  struct QUE_ENTRY *);
			prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
			ucHifTc = nicTxDirectGetHifTc(prMsduInfo);
		} else {
			if (ucCheckTc != 0xff)
				ucHifTc = ucCheckTc;

			if (QUEUE_IS_EMPTY(
				    &prAdapter->rTxDirectHifQueue[ucHifTc])) {
				DBGLOG(TX, INFO,
					"ERROR: no rTxDirectHifQueue (%u)\n",
					ucHifTc);
				return WLAN_STATUS_FAILURE;
			}
			QUEUE_REMOVE_HEAD(
				&prAdapter->rTxDirectHifQueue[ucHifTc],
				prQueueEntry, struct QUE_ENTRY *);
			prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
		}
	}

	if (!prMsduInfo)
		return WLAN_STATUS_SUCCESS;

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	wlanAcquirePowerControl(prAdapter);
#endif
	while (1) {
		if (!halTxIsDataBufEnough(prAdapter, prMsduInfo)) {
			QUEUE_INSERT_HEAD(
				&prAdapter->rTxDirectHifQueue[ucHifTc],
				(struct QUE_ENTRY *) prMsduInfo);
			mod_timer(&prAdapter->rTxDirectHifTimer,
				  jiffies + TX_DIRECT_CHECK_INTERVAL);
			break;
		}

		if (prMsduInfo->pfTxDoneHandler) {
			KAL_SPIN_LOCK_DECLARATION();

			/* Record native packet pointer for Tx done log */
			WLAN_GET_FIELD_32(&prMsduInfo->prPacket,
					  &prMsduInfo->u4TxDoneTag);

			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
			QUEUE_INSERT_TAIL(
				&(prAdapter->rTxCtrl.rTxMgmtTxingQueue),
				(struct QUE_ENTRY *) prMsduInfo);
			KAL_RELEASE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TXING_MGMT_LIST);
		}
		HAL_WRITE_TX_DATA(prAdapter, prMsduInfo);

		if (QUEUE_IS_NOT_EMPTY(
			    &prAdapter->rTxDirectHifQueue[ucHifTc])) {
			QUEUE_REMOVE_HEAD(
				&prAdapter->rTxDirectHifQueue[ucHifTc],
				prQueueEntry, struct QUE_ENTRY *);
			if (prQueueEntry == NULL)
				break;
			prMsduInfo = (struct MSDU_INFO *) prQueueEntry;
		} else {
			break;
		}
	}
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	wlanReleasePowerControl(prAdapter);
#endif
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is the timeout function of timer rTxDirectSkbTimer.
 *        The purpose is to check if rTxDirectSkbQueue has any skb to be sent.
 *
 * \param[in] data  Pointer of GlueInfo
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void nicTxDirectTimerCheckSkbQ(struct timer_list *timer)
#else
void nicTxDirectTimerCheckSkbQ(unsigned long data)
#endif

{

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct ADAPTER *prAdapter =
		from_timer(prAdapter, timer, rTxDirectSkbTimer);
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
#endif

	if (skb_queue_len(&prAdapter->rTxDirectSkbQueue))
		nicTxDirectStartXmit(NULL, prGlueInfo);
	else
		DBGLOG(TX, INFO, "fgHasNoMsdu FALSE\n");
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is the timeout function of timer rTxDirectHifTimer.
 *        The purpose is to check if rStaPsQueue, rBssAbsentQueue,
 *        and rTxDirectHifQueue has any MsduInfo to be sent.
 *
 * \param[in] data  Pointer of GlueInfo
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void nicTxDirectTimerCheckHifQ(struct timer_list *timer)
#else
void nicTxDirectTimerCheckHifQ(unsigned long data)
#endif
{

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct ADAPTER *prAdapter =
		from_timer(prAdapter, timer, rTxDirectHifTimer);
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
#endif

	uint8_t ucHifTc = 0;
	uint32_t u4StaPsBitmap, u4BssAbsentBitmap, u4StaPendBitmap;
	uint8_t ucStaRecIndex, ucBssIndex;

	spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

	u4StaPsBitmap = prAdapter->u4StaPsBitmap;
	u4BssAbsentBitmap = prAdapter->u4BssAbsentBitmap;
	u4StaPendBitmap = prAdapter->u4StaPendBitmap;

	if (u4StaPendBitmap)
		for (ucStaRecIndex = 0; ucStaRecIndex < CFG_STA_REC_NUM;
		     ++ucStaRecIndex) {
			if (u4StaPendBitmap & BIT(ucStaRecIndex)) {
				nicTxDirectStartXmitMain(NULL, NULL, prAdapter,
					0xff, ucStaRecIndex, 0xff);
				DBGLOG(TX, INFO, "Check pending Queue idx=%u\n",
					ucStaRecIndex);
			}
		}

	if (u4StaPsBitmap)
		for (ucStaRecIndex = 0; ucStaRecIndex < CFG_STA_REC_NUM;
		     ++ucStaRecIndex) {
			if (QUEUE_IS_NOT_EMPTY(
				    &prAdapter->rStaPsQueue[ucStaRecIndex])) {
				nicTxDirectStartXmitMain(NULL, NULL, prAdapter,
					0xff, ucStaRecIndex, 0xff);
				u4StaPsBitmap &= ~BIT(ucStaRecIndex);
				DBGLOG(TX, INFO,
					"ucStaRecIndex: %u\n", ucStaRecIndex);
			}
			if (u4StaPsBitmap == 0)
				break;
		}

	if (u4BssAbsentBitmap)
		for (ucBssIndex = 0; ucBssIndex < MAX_BSSID_NUM + 1;
		     ++ucBssIndex) {
			if (QUEUE_IS_NOT_EMPTY(
				    &prAdapter->rBssAbsentQueue[ucBssIndex])) {
				nicTxDirectStartXmitMain(NULL, NULL, prAdapter,
					0xff, 0xff, ucBssIndex);
				u4BssAbsentBitmap &= ~BIT(ucBssIndex);
				DBGLOG(TX, INFO,
					"ucBssIndex: %u\n", ucBssIndex);
			}
			if (u4BssAbsentBitmap == 0)
				break;
		}


	for (ucHifTc = 0; ucHifTc < TX_PORT_NUM; ucHifTc++)
		if (QUEUE_IS_NOT_EMPTY(
			    &prAdapter->rTxDirectHifQueue[ucHifTc]))
			nicTxDirectStartXmitMain(NULL, NULL, prAdapter, ucHifTc,
						 0xff, 0xff);

	spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is have to called by kalHardStartXmit().
 *        The purpose is to let as many as possible TX processing in softirq
 *        instead of in kernel thread to reduce TX CPU usage.
 *        NOTE: Currently only USB interface can use this function.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prGlueInfo  Pointer of prGlueInfo
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t nicTxDirectStartXmit(struct sk_buff *prSkb,
			      struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct MSDU_INFO *prMsduInfo;
	uint32_t ret = WLAN_STATUS_SUCCESS;

	spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);

	if (prSkb) {
		prMsduInfo = cnmPktAlloc(prAdapter, 0);

		if (prMsduInfo == NULL) {
			DBGLOG_LIMITED(TX, INFO, "cnmPktAlloc NULL\n");
			skb_queue_tail(&prAdapter->rTxDirectSkbQueue, prSkb);

			ret = WLAN_STATUS_SUCCESS;
			goto end;
		}
		if (skb_queue_len(&prAdapter->rTxDirectSkbQueue)) {
			skb_queue_tail(&prAdapter->rTxDirectSkbQueue, prSkb);
			prSkb = skb_dequeue(&prAdapter->rTxDirectSkbQueue);
		}
	} else {
		prMsduInfo = cnmPktAlloc(prAdapter, 0);
		if (prMsduInfo != NULL) {
			prSkb = skb_dequeue(&prAdapter->rTxDirectSkbQueue);
			if (prSkb == NULL) {
				DBGLOG(TX, INFO,
					"ERROR: no rTxDirectSkbQueue\n");
				nicTxReturnMsduInfo(prAdapter, prMsduInfo);
				ret = WLAN_STATUS_FAILURE;
				goto end;
			}
		} else {
			ret = WLAN_STATUS_FAILURE;
			goto end;
		}
	}

	while (1) {
		nicTxDirectStartXmitMain(prSkb, prMsduInfo, prAdapter, 0xff,
					 0xff, 0xff);
		prSkb = skb_dequeue(&prAdapter->rTxDirectSkbQueue);
		if (prSkb != NULL) {
			prMsduInfo = cnmPktAlloc(prAdapter, 0);
			if (prMsduInfo == NULL) {
				skb_queue_head(&prAdapter->rTxDirectSkbQueue,
					prSkb);
				break;
			}
		} else {
			break;
		}
	}

end:
	if (skb_queue_len(&prAdapter->rTxDirectSkbQueue))
		mod_timer(&prAdapter->rTxDirectSkbTimer,
			  jiffies + TX_DIRECT_CHECK_INTERVAL);
	spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_TX_DIRECT]);
	return ret;
}
/* TX Direct functions : END */


/*----------------------------------------------------------------------------*/
/*
 * \brief Assign Tc resource to prWifiVar according to firmware's report
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prGlueInfo  Pointer of prGlueInfo
 *
 */
/*----------------------------------------------------------------------------*/

void nicTxResourceUpdate_v1(IN struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_CFG_FILE
	uint8_t string[128], ret = 0;
#endif
	uint8_t idx, i, tc_num;
	uint32_t u4share, u4remains;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t *pau4TcPageCount;
	uint8_t ucMaxTcNum = TC_NUM;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	struct QUE_MGT *prQM = &prAdapter->rQM;
#endif
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	struct TX_TCQ_STATUS *prTc = &prAdapter->rTxCtrl.rTc;

	ucMaxTcNum = TC4_INDEX + 1;
	prTc->au4PseCtrlEnMap = BITS(TC0_INDEX, ucMaxTcNum-1);
	prTc->au4PleCtrlEnMap = BITS(TC0_INDEX, ucMaxTcNum-1) & ~(1<<TC4_INDEX);
#endif



	/*
	 * Use the settings in config file first,
	 * else, use the settings reported from firmware.
	 */


	/*
	 * 1. assign PSE/PLE free page count for each TC
	 */

	tc_num = (ucMaxTcNum - 1); /* except TC4_INDEX */
	for (i = 0; i < 2; i++) {
		if (i == 0) {
			/* PSE CMD*/
			prWifiVar->au4TcPageCount[TC4_INDEX] =
				prAdapter->nicTxReousrce.u4CmdTotalResource;

			/* calculate PSE free page count for each TC,
			 * except TC_4
			 */
			u4share = prAdapter->nicTxReousrce.u4DataTotalResource /
				  tc_num;
			u4remains = prAdapter->nicTxReousrce.
				u4DataTotalResource % tc_num;
			pau4TcPageCount = prWifiVar->au4TcPageCount;
		} else {
			/* PLE CMD*/
			prWifiVar->au4TcPageCountPle[TC4_INDEX] =
				prAdapter->nicTxReousrce.u4CmdTotalResourcePle;

			/* calculate PLE free page count for each TC,
			 * except TC_4
			 */
			u4share = prAdapter->nicTxReousrce.
				u4DataTotalResourcePle / tc_num;
			u4remains = prAdapter->nicTxReousrce.
				u4DataTotalResourcePle % tc_num;
			pau4TcPageCount = prWifiVar->au4TcPageCountPle;
		}

		/* assign free page count for each TC, except TC_4 */
		for (idx = TC0_INDEX; idx < ucMaxTcNum; idx++) {
			if (idx != TC4_INDEX)
				pau4TcPageCount[idx] = u4share;
		}
		/* if there is remaings, give them to TC_3, which is VO */
		pau4TcPageCount[TC3_INDEX] += u4remains;
	}


#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/*
	 * 2. assign guaranteed page count for each TC
	 */

	/* 2 1. update guaranteed page count in QM */
	for (idx = 0; idx < ucMaxTcNum; idx++)
		prQM->au4GuaranteedTcResource[idx] =
			prWifiVar->au4TcPageCount[idx];
#endif



#if CFG_SUPPORT_CFG_FILE
	/*
	 * 3. Use the settings in config file first,
	 *	  else, use the settings reported from firmware.
	 */

	/* 3 1. update for free page count */
	for (idx = 0; idx < ucMaxTcNum; idx++) {

		/* construct prefix: Tc0Page, Tc1Page... */
		memset(string, 0, sizeof(string) / sizeof(uint8_t));
		ret = snprintf(string, sizeof(string) / sizeof(uint8_t),
			 "Tc%xPage", idx);
		if (ret > (sizeof(string) / sizeof(uint8_t))) {
			DBGLOG(NIC, INFO,
			"sprintf failed of page count:%d\n", ret);
		} else {
			/* update the final value */
			prWifiVar->au4TcPageCount[idx] =
				(uint32_t) wlanCfgGetUint32(prAdapter,
				string, prWifiVar->au4TcPageCount[idx]);
		}
	}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 3 2. update for guaranteed page count */
	for (idx = 0; idx < ucMaxTcNum; idx++) {

		/* construct prefix: Tc0Grt, Tc1Grt... */
		memset(string, 0, sizeof(string) / sizeof(uint8_t));
		ret = snprintf(string, sizeof(string) / sizeof(uint8_t),
			 "Tc%xGrt", idx);
		if (ret > (sizeof(string) / sizeof(uint8_t))) {
			DBGLOG(NIC, INFO,
			"sprintf failed of guaranteed page count:%d\n", ret);
		} else {
			/* update the final value */
			prQM->au4GuaranteedTcResource[idx] =
				(uint32_t) wlanCfgGetUint32(prAdapter,
				string, prQM->au4GuaranteedTcResource[idx]);
		}
	}
#endif /* end of #if QM_ADAPTIVE_TC_RESOURCE_CTRL */
#endif /* end of #if CFG_SUPPORT_CFG_FILE */




	/*
	 * 4. Peak throughput settings.
	 *    Give most of the resource to TC1_INDEX.
	 *    Reference to arNetwork2TcResource[], AC_BE uses TC1_INDEX.
	 */
	if (prAdapter->rWifiVar.ucTpTestMode ==
	    ENUM_TP_TEST_MODE_THROUGHPUT) {
		uint32_t u4psePageCnt, u4plePageCnt, u4pseRemain,
			 u4pleRemain;
#define DEFAULT_PACKET_NUM 5


		/* pse */
		u4pseRemain = prAdapter->nicTxReousrce.u4DataTotalResource;
		u4psePageCnt = DEFAULT_PACKET_NUM *
			       nicTxGetMaxDataPageCntPerFrame(prAdapter);

		/* ple */
		u4pleRemain =
			prAdapter->nicTxReousrce.u4DataTotalResourcePle;
		u4plePageCnt = DEFAULT_PACKET_NUM *
			       NIX_TX_PLE_PAGE_CNT_PER_FRAME;

		/* equally giving to each TC */
		for (idx = 0; idx < ucMaxTcNum; idx++) {
			if (idx == TC4_INDEX)
				continue;

			/* pse */
			prWifiVar->au4TcPageCount[idx] = u4psePageCnt;
			u4pseRemain -= u4psePageCnt;

			/* ple */
			prWifiVar->au4TcPageCountPle[idx] = u4plePageCnt;
			u4pleRemain -= u4plePageCnt;
		}

		/* remaings are to TC1_INDEX */
		prWifiVar->au4TcPageCount[TC1_INDEX] += u4pseRemain;
		prWifiVar->au4TcPageCountPle[TC1_INDEX] += u4pleRemain;
	}
}
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
void nicTxResourceUpdate_v2(IN struct ADAPTER *prAdapter)
{
	uint8_t string[32];
	uint8_t ucMaxTcNum = TC_NUM;

	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t idx;
	uint8_t ucPleCtrlNum = 0;
	uint16_t u2PleAvail = 0;
	uint16_t u2PseAvail = 0;
	uint16_t u2PsePerPtk;
	uint16_t u2MaxTxDataLen = 0;
	struct tx_resource_info *prTxRes = &prAdapter->nicTxReousrce;
	struct mt66xx_chip_info *prChipInfo;
	struct TX_TCQ_STATUS *prTc = &prAdapter->rTxCtrl.rTc;

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	struct QUE_MGT *prQM = &prAdapter->rQM;
#endif

	/* Hardcode sanity. TC# should be 14 */
	if (ucMaxTcNum != 14)
		DBGLOG(TX, ERROR,
			"V2 TC_NUM should be 14 (%d)\n",
			ucMaxTcNum);

	/* 1. Remap default PSE/PLE enable bits */
	prTc->au4PseCtrlEnMap = (1<<TC4_INDEX);
	prTc->au4PleCtrlEnMap =
		BITS(TC0_INDEX, ucMaxTcNum-1) & ~(1<<TC4_INDEX);

	/*
	 * 2. assign PSE/PLE free page count for each TC
	 */
	prChipInfo = prAdapter->chip_info;
	/* ETHER_MAX_PKT_SZ=1514 for VLAN case? ETH_802_3_MAX_LEN=1500 */
	u2MaxTxDataLen = ETHER_MAX_PKT_SZ;

	/* Reset all PSE/PLE resource */
	for (idx = TC0_INDEX; idx < ucMaxTcNum; idx++) {
		prWifiVar->au4TcPageCount[idx] = 0;
		prWifiVar->au4TcPageCountPle[idx] = 0;
		if (nicTxResourceIsPleCtrlNeeded(prAdapter, idx))
			ucPleCtrlNum++;
	}
	/* TC4 have PSE only */
	prWifiVar->au4TcPageCount[TC4_INDEX] =
				prAdapter->nicTxReousrce.u4CmdTotalResource;
	/* PLE rearrange */
	u2PseAvail = prTxRes->u4DataTotalResource;
	u2PsePerPtk = (((u2MaxTxDataLen) + (prTxRes->u4DataResourceUnit) - 1)
		/ (prTxRes->u4DataResourceUnit));
	u2PleAvail = u2PseAvail / u2PsePerPtk;

	/* Resource balance to all PLE-TC */
	for (idx = TC0_INDEX; idx < ucMaxTcNum; idx++) {
		if (!nicTxResourceIsPleCtrlNeeded(prAdapter, idx))
			continue;
		prWifiVar->au4TcPageCountPle[idx] = (u2PleAvail / ucPleCtrlNum);
	}
	prWifiVar->au4TcPageCountPle[TC3_INDEX] += u2PleAvail % ucPleCtrlNum;

#if CFG_SUPPORT_CFG_FILE
	/*
	 * 3. Use the settings in config file first,
	 *	  else, use the settings reported from firmware.
	 */

	/* 3 1. update for free page count */
	for (idx = 0; idx < ucMaxTcNum; idx++) {

		/* construct prefix: Tc0Page, Tc1Page... */
		memset(string, 0, sizeof(string) / sizeof(uint8_t));
		if (snprintf(string, sizeof(string) / sizeof(uint8_t),
			 "Tc%dPage", idx) >=  0) {
			/* update the final value */
			prWifiVar->au4TcPageCount[idx] =
				(uint32_t) wlanCfgGetUint32(prAdapter,
				string, prWifiVar->au4TcPageCount[idx]);
		} else
			DBGLOG(TX, ERROR, "Can't get TC Page %d\n", idx);
	}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 3 2. update for guaranteed page count */
	for (idx = 0; idx < ucMaxTcNum; idx++) {

		/* construct prefix: Tc0Grt, Tc1Grt... */
		memset(string, 0, sizeof(string) / sizeof(uint8_t));
		if (snprintf(string, sizeof(string) / sizeof(uint8_t),
			 "Tc%dGrt", idx) >= 0) {
			/* update the final value */
			prQM->au4GuaranteedTcResource[idx] =
				(uint32_t) wlanCfgGetUint32(prAdapter,
				string, prQM->au4GuaranteedTcResource[idx]);
		} else
			DBGLOG(TX, ERROR, "Can't get TC Grt %d\n", idx);
	}
#endif /* end of #if QM_ADAPTIVE_TC_RESOURCE_CTRL */
#endif /* end of #if CFG_SUPPORT_CFG_FILE */


	/*
	 * 4. Peak throughput settings.
	 *    Give most of the resource to TC1_INDEX.
	 *    Reference to arNetwork2TcResource[], AC_BE uses TC1_INDEX.
	 */
	if (prAdapter->rWifiVar.ucTpTestMode ==
	    ENUM_TP_TEST_MODE_THROUGHPUT) {
		uint32_t u4plePageCnt, u4pleRemain;
#define DEFAULT_PACKET_NUM 5

		/* Skip PSE part in V2 */
		/* ple */
		u4pleRemain = u2PleAvail;
		u4plePageCnt = DEFAULT_PACKET_NUM *
			       NIX_TX_PLE_PAGE_CNT_PER_FRAME;

		/* equally giving to each TC */
		for (idx = 0; idx < ucMaxTcNum; idx++) {
			if (idx == TC4_INDEX)
				continue;
			if (!nicTxResourceIsPleCtrlNeeded(prAdapter, idx))
				continue;

			/* ple */
			prWifiVar->au4TcPageCountPle[idx] = u4plePageCnt;
			u4pleRemain -= u4plePageCnt;
		}

		/* remaings are to TC1_INDEX */
		prWifiVar->au4TcPageCountPle[TC1_INDEX] += u4pleRemain;
	}
}
#endif /* (CFG_TX_RSRC_WMM_ENHANCE == 1) */


void nicTxChangeDataPortByAc(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucAci,
	u_int8_t fgToMcu)
{
	struct TX_DESC_OPS_T *prTxDescOps = prAdapter->chip_info->prTxDescOps;

	if (prTxDescOps->nic_txd_change_data_port_by_ac)
		prTxDescOps->nic_txd_change_data_port_by_ac(
			prStaRec,
			ucAci,
			fgToMcu);
}

/* if some msdus are waiting tx done status, but now roaming done, then need to
** change wlan index of these msdus to match tx done status event
** In multi-thread solution, we also need to check if pending tx packets in
** hif_thread tx queue.
*/
void nicTxHandleRoamingDone(struct ADAPTER *prAdapter,
			    struct STA_RECORD *prOldStaRec,
			    struct STA_RECORD *prNewStaRec)
{
	struct MSDU_INFO *prMsduInfo = NULL;
	uint8_t ucOldWlanIndex = prOldStaRec->ucWlanIndex;
	uint8_t ucNewWlanIndex = prNewStaRec->ucWlanIndex;
#if CFG_SUPPORT_MULTITHREAD
	uint8_t ucIndex = 0;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);
	prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_HEAD(
		&prAdapter->rTxCtrl.rTxMgmtTxingQueue);
	while (prMsduInfo) {
		if (prMsduInfo->ucWlanIndex == ucOldWlanIndex)
			prMsduInfo->ucWlanIndex = ucNewWlanIndex;
		prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_NEXT_ENTRY(
			&prMsduInfo->rQueEntry);
	}
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

/* I think any time we disconnect with previous AP, rTxP0Queue and rTxP1Queue
** should be empty.
** because we have stopped dequeue when initial to connect the new roaming AP.
** It is enough time for hif_thread to send out these packets. But anyway, let's
** prepare code for that case to avoid scheduler corner case.
*/
#if CFG_SUPPORT_MULTITHREAD
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
#if CFG_FIX_2_TX_PORT
	prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_HEAD(&prAdapter->rTxP0Queue);
	while (prMsduInfo) {
		if (prMsduInfo->ucWlanIndex == ucOldWlanIndex)
			prMsduInfo->ucWlanIndex = ucNewWlanIndex;
		prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_NEXT_ENTRY(
			&prMsduInfo->rQueEntry);
	}
	prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_HEAD(&prAdapter->rTxP1Queue);
	while (prMsduInfo) {
		if (prMsduInfo->ucWlanIndex == ucOldWlanIndex)
			prMsduInfo->ucWlanIndex = ucNewWlanIndex;
		prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_NEXT_ENTRY(
			&prMsduInfo->rQueEntry);
	}
#else
	for (ucIndex = 0; ucIndex < TX_PORT_NUM; ucIndex++) {
		prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_HEAD(
			&prAdapter->rTxPQueue[ucIndex]);
		while (prMsduInfo) {
			if (prMsduInfo->ucWlanIndex == ucOldWlanIndex)
				prMsduInfo->ucWlanIndex = ucNewWlanIndex;
			prMsduInfo = (struct MSDU_INFO *)QUEUE_GET_NEXT_ENTRY(
				&prMsduInfo->rQueEntry);
		}
	}
#endif
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
#endif
}

int32_t nicTxGetVectorInfo(IN char *pcCommand, IN int i4TotalLen,
				   IN struct TX_VECTOR_BBP_LATCH *prTxV)
{
	uint8_t rate, txmode, frmode, sgi, ldpc, nsts, stbc, txpwr;
	int32_t i4BytesWritten = 0;
#if (CFG_SUPPORT_CONNAC2X == 1)
	uint8_t dcm, ersu106t;
#endif

	rate = TX_VECTOR_GET_TX_RATE(prTxV);
	txmode = TX_VECTOR_GET_TX_MODE(prTxV);
	frmode = TX_VECTOR_GET_TX_FRMODE(prTxV);
	nsts = TX_VECTOR_GET_TX_NSTS(prTxV) + 1;
	sgi = TX_VECTOR_GET_TX_SGI(prTxV);
	ldpc = TX_VECTOR_GET_TX_LDPC(prTxV);
	stbc = TX_VECTOR_GET_TX_STBC(prTxV);
	txpwr = TX_VECTOR_GET_TX_PWR(prTxV);
#if (CFG_SUPPORT_CONNAC2X == 1)
	dcm = TX_VECTOR_GET_TX_DCM(prTxV);
	ersu106t = TX_VECTOR_GET_TX_106T(prTxV);

	if (dcm)
		rate = CONNAC2X_TXV_GET_TX_RATE_UNMASK_DCM(rate);
	if (ersu106t)
		rate = CONNAC2X_TXV_GET_TX_RATE_UNMASK_106T(rate);
#endif

	if (prTxV->u4TxV[0] == 0xFFFFFFFF) {
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s%s\n", "Last TX Rate", " = ", "N/A");
	} else {
		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten,
			"%-20s%s", "Last TX Rate", " = ");

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
					(((rate >= 5) && (rate <= 7)) ?
						HW_TX_RATE_CCK_STR[rate - 4] :
						HW_TX_RATE_CCK_STR[4]));
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", nicHwRateOfdmStr(rate));
		else if ((txmode == TX_RATE_MODE_HTMIX) ||
			 (txmode == TX_RATE_MODE_HTGF))
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"MCS%d, ", rate);
		else
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s%d_MCS%d, ", stbc ? "NSTS" : "NSS",
				nsts, rate);

		i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten, "%s, ",
				frmode < 4 ? HW_TX_RATE_BW[frmode] :
				HW_TX_RATE_BW[4]);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else if ((txmode == TX_RATE_MODE_HTMIX) ||
			 (txmode == TX_RATE_MODE_HTGF) ||
			 (txmode == TX_RATE_MODE_VHT) ||
			 (txmode == TX_RATE_MODE_PLR))
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", sgi == 0 ? "LGI" : "SGI");
		else
			i4BytesWritten += kalScnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%s, ", sgi == 0 ? "SGI" :
				(sgi == 1 ? "MGI" : "LGI"));

		i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, "%s%s%s%s%s\n",
			(txmode < ENUM_TX_MODE_NUM ?
			HW_TX_MODE_STR[txmode] : "N/A"),
#if (CFG_SUPPORT_CONNAC2X == 1)
			dcm ? ", DCM" : "", ersu106t ? ", 106t" : "",
#else
			"", "",
#endif
			stbc ? ", STBC, " : ", ", ldpc == 0 ? "BCC" : "LDPC");
	}

	return i4BytesWritten;
}

uint8_t nicTxGetWmmIdxByTc(uint8_t ucTC)
{
	uint8_t ucAc;

	if (ucTC >= TC_NUM) {
		DBGLOG(TX, ERROR, "Invalid TC%d\n", ucTC);
		return 0;
	}

	if (ucTC == TC4_INDEX) {
		/* TC4 is meaningless for WMM, return 0 only */
		DBGLOG(TX, TRACE, "TC%d fall to WMM0\n", ucTC);
		return 0;
	}

	ucAc = nicTxGetTxDestQIdxByTc(ucTC);
	return (ucAc/WMM_AC_INDEX_NUM);
}
uint8_t nicTxGetAcIdxByTc(uint8_t ucTC)
{
	uint8_t ucAc;

	if (ucTC >= TC_NUM) {
		DBGLOG(TX, ERROR, "Invalid TC%d\n", ucTC);
		return WMM_AC_BE_INDEX;
	}

	ucAc = nicTxGetTxDestQIdxByTc(ucTC);
	return (ucAc%WMM_AC_INDEX_NUM);
}

uint8_t nicTxWmmTc2ResTc(struct ADAPTER *prAdapter,
	uint8_t ucWmmSet, uint8_t ucWmmTC)
{
	uint8_t ucTC;

	if (ucWmmSet >= MAX_BSSID_NUM + 1
		|| ucWmmTC >= NET_TC_NUM) {
		DBGLOG(TX, ERROR, "Invalid WmmSet:%d WmmTC:%d\n",
			ucWmmSet, ucWmmTC);
		return TC0_INDEX;
	}

	ucTC = arNetwork2TcResource[ucWmmSet][ucWmmTC];

	/* If TC is disabled, return WMM0-AC as default */
	if (!NIC_TX_RES_IS_ACTIVE(prAdapter, ucTC))
		ucTC = arNetwork2TcResource[0][ucWmmTC];

	return ucTC;
}

uint8_t nicTxResTc2WmmTc(uint8_t ucResTC)
{
	uint8_t ucSetIdx;
	uint8_t ucTcIdx;

	ucSetIdx = nicTxGetWmmIdxByTc(ucResTC);

	/* TCx is meaningless for BMC, do not need to cover BMC case */
	for (ucTcIdx = 0;
		ucTcIdx < ARRAY_SIZE(arNetwork2TcResource[0]);
		ucTcIdx++) {
		if (ucResTC == arNetwork2TcResource[ucSetIdx][ucTcIdx])
			break;
	}

	return ucTcIdx;
}

