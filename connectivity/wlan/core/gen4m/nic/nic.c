// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   nic.c
 *  \brief  Functions that provide operation in NIC's (Network Interface Card)
 *          point of view.
 *    This file includes functions which unite multiple hal(Hardware) operations
 *    and also take the responsibility of Software Resource Management in order
 *    to keep the synchronization with Hardware Manipulation.
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

#if (CFG_TWT_SMART_STA == 1)
#include "twt.h"
#endif

#if CFG_SUPPORT_NAN
#include "nan_dev.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
const uint8_t aucPhyCfg2PhyTypeSet[PHY_CONFIG_NUM] = {
	PHY_TYPE_SET_802_11ABG,	/* PHY_CONFIG_802_11ABG */
	PHY_TYPE_SET_802_11BG,	/* PHY_CONFIG_802_11BG */
	PHY_TYPE_SET_802_11G,	/* PHY_CONFIG_802_11G */
	PHY_TYPE_SET_802_11A,	/* PHY_CONFIG_802_11A */
	PHY_TYPE_SET_802_11B,	/* PHY_CONFIG_802_11B */
	PHY_TYPE_SET_802_11ABGN,	/* PHY_CONFIG_802_11ABGN */
	PHY_TYPE_SET_802_11BGN,	/* PHY_CONFIG_802_11BGN */
	PHY_TYPE_SET_802_11AN,	/* PHY_CONFIG_802_11AN */
	PHY_TYPE_SET_802_11GN,	/* PHY_CONFIG_802_11GN */
	PHY_TYPE_SET_802_11AC,
	PHY_TYPE_SET_802_11ANAC,
	PHY_TYPE_SET_802_11ABGNAC,
#if (CFG_SUPPORT_802_11AX == 1)
	PHY_TYPE_SET_802_11ABGNACAX,
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	PHY_TYPE_SET_802_11ABGNACAXBE,
#endif
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

static struct INT_EVENT_MAP arIntEventMapTable[] = {
	{WHISR_ABNORMAL_INT | WHISR_WDT_INT, INT_EVENT_ABNORMAL},
	{WHISR_D2H_SW_INT, INT_EVENT_SW_INT},
	{WHISR_TX_DONE_INT, INT_EVENT_TX},
	{(WHISR_RX0_DONE_INT | WHISR_RX1_DONE_INT), INT_EVENT_RX}
};

static const uint8_t ucIntEventMapSize = (sizeof(
			arIntEventMapTable) / sizeof(struct INT_EVENT_MAP));

static IST_EVENT_FUNCTION apfnEventFuncTable[] = {
	nicProcessAbnormalInterrupt,	/*!< INT_EVENT_ABNORMAL */
	nicProcessSoftwareInterruptEx,	/*!< INT_EVENT_SW_INT   */
	nicProcessTxInterrupt,	/*!< INT_EVENT_TX       */
	nicProcessRxInterrupt,	/*!< INT_EVENT_RX       */
};

struct ECO_INFO g_eco_info = {0xFF};
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#if defined(_HIF_USB)
struct TIMER rSerSyncTimer = {
	.rLinkEntry = {0}
};
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
/*! This macro is used to reduce coding errors inside nicAllocateAdapterMemory()
 * and also enhance the readability.
 */
#define LOCAL_NIC_ALLOCATE_MEMORY(pucMem, u4Size, eMemType, pucComment) \
{ \
	pucMem = kalMemAlloc(u4Size, eMemType); \
	if (pucMem == NULL) { \
		DBGLOG(INIT, ERROR, \
			"Could not allocate %u bytes for %s.\n", \
			u4Size, (char *) pucComment); \
		break; \
	} \
	ASSERT(((uintptr_t)pucMem % 4) == 0); \
	DBGLOG(INIT, TRACE, "Alloc %u bytes, addr = 0x%p for %s.\n", \
		u4Size, (void *) pucMem, (char *) pucComment); \
}

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for the allocation of the data structures
 *        inside the Adapter structure, include:
 *        1. SW_RFB_Ts
 *        2. Common coalescing buffer for TX PATH.
 *
 * @param prAdapter Pointer of Adapter Data Structure
 *
 * @retval WLAN_STATUS_SUCCESS - Has enough memory.
 * @retval WLAN_STATUS_RESOURCES - Memory is not enough.
 */
/*----------------------------------------------------------------------------*/
uint32_t nicAllocateAdapterMemory(struct ADAPTER
				  *prAdapter)
{
	uint32_t status = WLAN_STATUS_RESOURCES;
	struct RX_CTRL *prRxCtrl;
	struct TX_CTRL *prTxCtrl;

	ASSERT(prAdapter);
	prRxCtrl = &prAdapter->rRxCtrl;
	prTxCtrl = &prAdapter->rTxCtrl;

	do {
		/* 4 <0> Reset all Memory Handler */
#if CFG_DBG_MGT_BUF
		prAdapter->u4MemFreeDynamicCount = 0;
		prAdapter->u4MemAllocDynamicCount = 0;
#endif
		prAdapter->pucMgtBufCached = (uint8_t *) NULL;
		prRxCtrl->prRxCached = NULL;

		/* 4 <1> Memory for Management Memory Pool and CMD_INFO_T */
		/* Allocate memory for the struct CMD_INFO
		 * and its MGMT memory pool.
		 */
		prAdapter->u4MgtBufCachedSize = MGT_BUFFER_SIZE;
#ifdef CFG_PREALLOC_MEMORY
		prAdapter->pucMgtBufCached = preallocGetMem(MEM_ID_NIC_ADAPTER);
#else
		LOCAL_NIC_ALLOCATE_MEMORY(prAdapter->pucMgtBufCached,
			prAdapter->u4MgtBufCachedSize, PHY_MEM_TYPE,
			"COMMON MGMT MEMORY POOL");
#endif
		/* 4 <2> Memory for RX Descriptor */
		/* Initialize the number of rx buffers
		 * we will have in our queue.
		 */
		/* <TODO> We may setup ucRxPacketDescriptors by GLUE Layer,
		 * and using this variable directly.
		 */
		/* Allocate memory for the SW receive structures. */
		LOCAL_NIC_ALLOCATE_MEMORY(prRxCtrl->prRxCached,
			sizeof(struct SW_RFB[CFG_RX_MAX_PKT_NUM]),
			VIR_MEM_TYPE, "struct SW_RFB");

		/* 4 <3> Memory for TX DEscriptor */
		prTxCtrl->u4TxCachedSize = CFG_TX_MAX_PKT_NUM * ALIGN_4(
						   sizeof(struct MSDU_INFO));

		LOCAL_NIC_ALLOCATE_MEMORY(prTxCtrl->pucTxCached,
			prTxCtrl->u4TxCachedSize,
			VIR_MEM_TYPE, "struct MSDU_INFO");

		/* 4 <4> Memory for Common Coalescing Buffer */

		/* Get valid buffer size based on config & host capability */
		prAdapter->u4CoalescingBufCachedSize =
			halGetValidCoalescingBufSize(prAdapter);

		/* Allocate memory for the common coalescing buffer. */
#ifdef CFG_PREALLOC_MEMORY
		prAdapter->pucCoalescingBufCached =
			preallocGetMem(MEM_ID_IO_BUFFER);
#else
		prAdapter->pucCoalescingBufCached = kalAllocateIOBuffer(
				prAdapter->u4CoalescingBufCachedSize);
#endif
		if (prAdapter->pucCoalescingBufCached == NULL) {
			DBGLOG(INIT, ERROR,
			       "Could not allocate %u bytes for coalescing buffer.\n",
			       prAdapter->u4CoalescingBufCachedSize);
			break;
		}

		/* <5> Memory for HIF */
		if (halAllocateIOBuffer(prAdapter) != WLAN_STATUS_SUCCESS)
			break;

#if CFG_DBG_MGT_BUF
		LINK_INITIALIZE(&prAdapter->rMemTrackLink);
#endif
		status = WLAN_STATUS_SUCCESS;

	} while (FALSE);

	if (status != WLAN_STATUS_SUCCESS)
		nicReleaseAdapterMemory(prAdapter);

	return status;

}				/* end of nicAllocateAdapterMemory() */

static void checkLeakMemory(struct ADAPTER *prAdapter)
{
#if CFG_DBG_MGT_BUF
	u_int8_t fgUnfreedMem = FALSE;
	struct BUF_INFO *prBufInfo;
	uint32_t u4LeakCount;
	uint32_t u4MsgLeakCount;
	uint32_t u4MgtLeakCount;
	struct MEM_TRACK *prMemTrack;
	uint32_t i;
	struct MEM_TRACK **pLeak;

	/* Dynamic allocated memory from OS */
	u4LeakCount = prAdapter->u4MemAllocDynamicCount -
		      prAdapter->u4MemFreeDynamicCount;
	fgUnfreedMem |= !!u4LeakCount;

	/* MSG buffer */
	prBufInfo = &prAdapter->rMsgBufInfo;
	u4MsgLeakCount = prBufInfo->u4AllocCount -
			 (prBufInfo->u4FreeCount +
			  prBufInfo->u4AllocNullCount);
	fgUnfreedMem |= !!u4MsgLeakCount;

	/* MGT buffer */
	prBufInfo = &prAdapter->rMgtBufInfo;
	u4MgtLeakCount = prBufInfo->u4AllocCount -
				 (prBufInfo->u4FreeCount +
				  prBufInfo->u4AllocNullCount);
	fgUnfreedMem |= !!u4MgtLeakCount;

	/* Check if all allocated memories are free */
	if (fgUnfreedMem) {
		DBGLOG(MEM, ERROR,
		       "Unequal memory alloc/free count! leak=%u(%u-%u) msg=%u mgt=%u, NoAck=%u\n",
		       u4LeakCount,
		       prAdapter->u4MemAllocDynamicCount,
		       prAdapter->u4MemFreeDynamicCount,
		       u4MsgLeakCount, u4MgtLeakCount,
		       wlanIsChipNoAck(prAdapter));

		qmDumpQueueStatus(prAdapter, NULL, 0);
		cnmDumpMemoryStatus(prAdapter, NULL, 0);
	}

	if (wlanIsChipNoAck(prAdapter))
		return;	/* Skip this ASSERT if chip is no ACK */

	if (!u4LeakCount)
		return;

	pLeak = kalMemAlloc(sizeof(struct MEM_TRACK *) *
			    u4LeakCount,
			    VIR_MEM_TYPE);

	i = 0;
	DBGLOG(MEM, ERROR, "----- Memory Leak -----\n");
	LINK_FOR_EACH_ENTRY(prMemTrack,
			    &prAdapter->rMemTrackLink,
			    rLinkEntry,
			    struct MEM_TRACK) {
		DBGLOG(MEM, ERROR,
		       "file:line %s, cmd id: %u, where: %u\n",
		       prMemTrack->pucFileAndLine,
		       prMemTrack->ucCmdId,
		       prMemTrack->ucWhere);
		pLeak[i++] = prMemTrack;
	}
	KAL_WARN_ON(prAdapter->u4MemFreeDynamicCount !=
		    prAdapter->u4MemAllocDynamicCount);

	for (i = 0; i < u4LeakCount; i++)
		cnmMemFree(prAdapter, pLeak[i]->aucData);

	kalMemFree(pLeak, VIR_MEM_TYPE,
		   sizeof(struct MEM_TRACK *) * u4LeakCount);

	DBGLOG(MEM, WARN,
	       "%u leak entries flushed, alloc=%u, free=%u\n",
	       u4LeakCount,
	       prAdapter->u4MemAllocDynamicCount,
	       prAdapter->u4MemFreeDynamicCount);

	ASSERT(prAdapter->u4MemFreeDynamicCount ==
	       prAdapter->u4MemAllocDynamicCount);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for releasing the allocated memory by
 *        nicAllocatedAdapterMemory().
 *
 * @param prAdapter Pointer of Adapter Data Structure
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicReleaseAdapterMemory(struct ADAPTER *prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	struct RX_CTRL *prRxCtrl;
	uint32_t u4Idx;

	if (!prAdapter) {
		DBGLOG(MEM, ERROR, "NULL prAdapter");
		return;
	}

	prTxCtrl = &prAdapter->rTxCtrl;
	prRxCtrl = &prAdapter->rRxCtrl;

	/* 4 <5> Memory for HIF */
	halReleaseIOBuffer(prAdapter);

	/* 4 <4> Memory for Common Coalescing Buffer */
	if (prAdapter->pucCoalescingBufCached) {
#ifndef CFG_PREALLOC_MEMORY
		kalReleaseIOBuffer(prAdapter->pucCoalescingBufCached,
				   prAdapter->u4CoalescingBufCachedSize);
#endif
		prAdapter->pucCoalescingBufCached = NULL;
	}

	/* 4 <3> Memory for TX Descriptor */
	if (prTxCtrl->pucTxCached) {
		kalMemFree(prTxCtrl->pucTxCached, VIR_MEM_TYPE,
			   prTxCtrl->u4TxCachedSize);
		prTxCtrl->pucTxCached = NULL;
	}
	/* 4 <2> Memory for RX Descriptor */
	if (prRxCtrl->prRxCached) {
		kalMemFree(prRxCtrl->prRxCached, VIR_MEM_TYPE,
			   sizeof(struct SW_RFB[CFG_RX_MAX_PKT_NUM]));
		prRxCtrl->prRxCached = NULL;
	}
	/* 4 <1> Memory for Management Memory Pool */
	if (prAdapter->pucMgtBufCached) {
#ifndef CFG_PREALLOC_MEMORY
		kalMemFree(prAdapter->pucMgtBufCached, PHY_MEM_TYPE,
			   prAdapter->u4MgtBufCachedSize);
#endif
		prAdapter->pucMgtBufCached = NULL;
	}

	/* Memory for TX Desc Template */
	for (u4Idx = 0; u4Idx < CFG_STA_REC_NUM; u4Idx++)
		nicTxFreeDescTemplate(prAdapter, &prAdapter->arStaRec[u4Idx]);

#if CFG_SUPPORT_LLS
	prAdapter->pucLinkStatsSrcBufAddr = NULL;
	prAdapter->pu4TxTimePerLevels = NULL;
#endif
	checkLeakMemory(prAdapter);
}

void nicTriggerAHDBG(struct ADAPTER *prAdapter,
	uint32_t u4mod, uint32_t u4reason,
	uint32_t u4BssIdx, uint32_t u4wlanIdx)
{
	prAdapter->u4HifChkFlag |= HIF_TRIGGER_FW_DUMP;
	prAdapter->u4HifDbgMod = u4mod;
	prAdapter->u4HifDbgBss = u4BssIdx;
	prAdapter->u4HifDbgReason = u4wlanIdx;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief disable global interrupt
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicDisableInterrupt(struct ADAPTER *prAdapter)
{
	halDisableInterrupt(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief enable global interrupt
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicEnableInterrupt(struct ADAPTER *prAdapter)
{
	halEnableInterrupt(prAdapter);
}				/* end of nicEnableInterrupt() */

#if 0				/* CFG_SDIO_INTR_ENHANCE */
/*----------------------------------------------------------------------------*/
/*!
 * @brief Read interrupt status from hardware
 *
 * @param prAdapter pointer to the Adapter handler
 * @param the interrupts
 *
 * @return N/A
 *
 */
/*----------------------------------------------------------------------------*/
void nicSDIOReadIntStatus(struct ADAPTER *prAdapter,
			  uint32_t *pu4IntStatus)
{
	struct ENHANCE_MODE_DATA_STRUCT *prSDIOCtrl;

	ASSERT(prAdapter);
	ASSERT(pu4IntStatus);

	prSDIOCtrl = prAdapter->prSDIOCtrl;
	ASSERT(prSDIOCtrl);

	HAL_PORT_RD(prAdapter,
		    MCR_WHISR,
		    sizeof(struct ENHANCE_MODE_DATA_STRUCT),
		    (uint8_t *) prSDIOCtrl,
		    sizeof(struct ENHANCE_MODE_DATA_STRUCT));

	if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
	    || fgIsBusAccessFailed == TRUE) {
		*pu4IntStatus = 0;
		return;
	}

	/* workaround */
	if ((prSDIOCtrl->u4WHISR & WHISR_TX_DONE_INT) == 0
	    && (prSDIOCtrl->rTxInfo.au4WTSR[0]
		| prSDIOCtrl->rTxInfo.au4WTSR[1]
		| prSDIOCtrl->rTxInfo.au4WTSR[2]
		| prSDIOCtrl->rTxInfo.au4WTSR[3]
		| prSDIOCtrl->rTxInfo.au4WTSR[4]
		| prSDIOCtrl->rTxInfo.au4WTSR[5]
		| prSDIOCtrl->rTxInfo.au4WTSR[6]
		| prSDIOCtrl->rTxInfo.au4WTSR[7])) {
		prSDIOCtrl->u4WHISR |= WHISR_TX_DONE_INT;
	}

	if ((prSDIOCtrl->u4WHISR & BIT(31)) == 0 &&
	    HAL_GET_MAILBOX_READ_CLEAR(prAdapter) == TRUE &&
	    (prSDIOCtrl->u4RcvMailbox0 != 0
	     || prSDIOCtrl->u4RcvMailbox1 != 0)) {
		prSDIOCtrl->u4WHISR |= BIT(31);
	}

	*pu4IntStatus = prSDIOCtrl->u4WHISR;

}				/* end of nicSDIOReadIntStatus() */
#endif

#if 0				/*defined(_HIF_PCIE) */
void nicPCIEReadIntStatus(struct ADAPTER *prAdapter,
			  uint32_t *pu4IntStatus)
{
	uint32_t u4RegValue;

	*pu4IntStatus = 0;

	HAL_MCR_RD(prAdapter, WPDMA_INT_STA, &u4RegValue);

	if (HAL_IS_RX_DONE_INTR(u4RegValue))
		*pu4IntStatus |= WHISR_RX0_DONE_INT;

	if (HAL_IS_TX_DONE_INTR(u4RegValue))
		*pu4IntStatus |= WHISR_TX_DONE_INT;

	/* clear interrupt */
	HAL_MCR_WR(prAdapter, WPDMA_INT_STA, u4RegValue);

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief The function used to read interrupt status and then invoking
 *        dispatching procedure for the appropriate functions
 *        corresponding to specific interrupt bits
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t nicProcessIST(struct ADAPTER *prAdapter)
{
	if (prAdapter == NULL) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL!!");
		return WLAN_STATUS_FAILURE;
	}
	return nicProcessISTWithSpecifiedCount(prAdapter,
		prAdapter->rWifiVar.u4HifIstLoopCount);
}

uint32_t nicProcessISTWithSpecifiedCount(struct ADAPTER *prAdapter,
	uint32_t u4HifIstLoopCount)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint32_t u4IntStatus = 0;
	uint32_t i;

	ASSERT(prAdapter);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set nicProcessIST! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	for (i = 0; i < u4HifIstLoopCount; i++) {

		HAL_READ_INT_STATUS(prAdapter, &u4IntStatus);
		/* DBGLOG(INIT, TRACE, "u4IntStatus: 0x%x\n", u4IntStatus); */

		if (u4IntStatus == 0) {
			if (i == 0)
				u4Status = WLAN_STATUS_NOT_INDICATING;
			break;
		}

		nicProcessIST_impl(prAdapter, u4IntStatus);

		if (prAdapter->ulNoMoreRfb)
			break;
	}

	return u4Status;
}				/* end of nicProcessIST() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief The function used to dispatch the appropriate functions for specific
 *        interrupt bits
 *
 * @param prAdapter   pointer to the Adapter handler
 *        u4IntStatus interrupt status bits
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t nicProcessIST_impl(struct ADAPTER *prAdapter,
			    uint32_t u4IntStatus)
{
	uint32_t u4IntCount = 0;
	struct INT_EVENT_MAP *prIntEventMap = NULL;

	ASSERT(prAdapter);

	prAdapter->u4IntStatus = u4IntStatus;

	/* Process each of the interrupt status consequently */
	prIntEventMap = &arIntEventMapTable[0];
	for (u4IntCount = 0; u4IntCount < ucIntEventMapSize;
	     prIntEventMap++, u4IntCount++) {
		if (prIntEventMap->u4Int & prAdapter->u4IntStatus) {
			if (0) {
				/* ignore */
			} else if (apfnEventFuncTable[prIntEventMap->u4Event] !=
				   NULL) {
				apfnEventFuncTable[
				prIntEventMap->u4Event] (prAdapter);
			} else {
				DBGLOG(INTR, WARN,
					"Empty INTR handler! ISAR bit#: %u, event:%u, func: 0x%p\n",
				  prIntEventMap->u4Int,
				  prIntEventMap->u4Event,
				  apfnEventFuncTable[prIntEventMap->u4Event]);

				/* to trap any NULL interrupt handler */
				ASSERT(0);
			}
			prAdapter->u4IntStatus &= ~prIntEventMap->u4Int;
		}
	}

	return WLAN_STATUS_SUCCESS;
}				/* end of nicProcessIST_impl() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Verify the CHIP ID
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 *
 * @retval TRUE          CHIP ID is the same as the setting compiled
 * @retval FALSE         CHIP ID is different from the setting compiled
 */
/*----------------------------------------------------------------------------*/
u_int8_t nicVerifyChipID(struct ADAPTER *prAdapter)
{
	return halVerifyChipID(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the MCR to the appropriate init value, and verify the init
 *        value
 *
 * @param prAdapter      a pointer to adapter private data structure.
 *
 * @return -
 */
/*----------------------------------------------------------------------------*/
void nicMCRInit(struct ADAPTER *prAdapter)
{

	ASSERT(prAdapter);

	/* 4 <0> Initial value */
}

void nicHifInit(struct ADAPTER *prAdapter)
{

	ASSERT(prAdapter);
#if 0
	/* reset event */
	nicPutMailbox(prAdapter, 0, 0x52455345);	/* RESE */
	nicPutMailbox(prAdapter, 1, 0x545F5746);	/* T_WF */
	nicSetSwIntr(prAdapter, BIT(16));
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the Adapter soft variable
 *
 * @param prAdapter pointer to the Adapter handler
 *
 * @return (none)
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t nicInitializeAdapter(struct ADAPTER *prAdapter)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	prAdapter->fgIsIntEnableWithLPOwnSet = FALSE;
	prAdapter->fgIsReadRevID = FALSE;

	do {
		if (!nicVerifyChipID(prAdapter)) {
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}
		/* 4 <1> MCR init */
		nicMCRInit(prAdapter);

		HAL_HIF_INIT(prAdapter);

		/* 4 <2> init FW HIF */
		nicHifInit(prAdapter);
	} while (FALSE);

	return u4Status;
}

#if defined(_HIF_SPI)
/*----------------------------------------------------------------------------*/
/*!
 * \brief Restore the SPI Mode Select to default mode,
 *        this is important while driver is unload, and this must be last mcr
 *        since the operation will let the hif use 8bit mode access
 *
 * \param[in] prAdapter      a pointer to adapter private data structure.
 * \param[in] eGPIO2_Mode    GPIO2 operation mode
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void nicRestoreSpiDefMode(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	HAL_MCR_WR(prAdapter, MCR_WCSR, SPICSR_8BIT_MODE_DATA);

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process Abnormal interrupt w/o callback
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void nicProcessDefaultAbnormalInterrupt(struct ADAPTER *prAdapter)
{
	if (halIsHifStateSuspend(prAdapter))
		DBGLOG(RX, WARN, "suspend Abnormal\n");

	prAdapter->prGlueInfo->IsrAbnormalCnt++;
#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
#if CFG_SUPPORT_WAKEUP_STATISTICS
	if (kalIsWakeupByWlan(prAdapter))
		nicUpdateWakeupStatistics(prAdapter, ABNORMAL_INT);
#endif
#endif /* fos_change end */

	halProcessAbnormalInterrupt(prAdapter);
	GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_PROCESS_ABNORMAL_INT);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Process rx interrupt. When the rx
 *        Interrupt is asserted, it means there are frames in queue.
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessAbnormalInterrupt(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo;

	prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->processAbnormalInterrupt)
		prBusInfo->processAbnormalInterrupt(prAdapter);
	else
		nicProcessDefaultAbnormalInterrupt(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief .
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessFwOwnBackInterrupt(struct ADAPTER
				  *prAdapter)
{

}				/* end of nicProcessFwOwnBackInterrupt() */

void nicProcessSoftwareInterruptEx(struct ADAPTER
				 *prAdapter)
{
	/* Only hif_thread and power off can process sw int */
	if (HAL_IS_RX_DIRECT(prAdapter) && !prAdapter->fgIsPwrOffProcIST)
		kalSetSerIntEvent(prAdapter->prGlueInfo);
	else
		nicProcessSoftwareInterrupt(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief .
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicProcessSoftwareInterrupt(struct ADAPTER
				 *prAdapter)
{
	prAdapter->prGlueInfo->IsrSoftWareCnt++;
	if (halIsHifStateSuspend(prAdapter))
		DBGLOG(RX, WARN, "suspend SW INT\n");
/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
#if CFG_SUPPORT_WAKEUP_STATISTICS
		if (kalIsWakeupByWlan(prAdapter))
			nicUpdateWakeupStatistics(prAdapter, SOFTWARE_INT);
#endif
#endif /* fos_change end */
	halProcessSoftwareInterrupt(prAdapter);
}				/* end of nicProcessSoftwareInterrupt() */

void nicSetSwIntr(struct ADAPTER *prAdapter,
		  uint32_t u4SwIntrBitmap)
{
	/* NOTE:
	 *  SW interrupt in HW bit 16 is mapping to SW bit 0
	 *  (shift 16bit in HW transparancy)
	 *  SW interrupt valid from b0~b15
	 */
	ASSERT((u4SwIntrBitmap & BITS(0, 15)) == 0);
	/* DBGLOG(INIT, TRACE, ("u4SwIntrBitmap: 0x%08x\n", u4SwIntrBitmap)); */

	HAL_MCR_WR(prAdapter, MCR_WSICR, u4SwIntrBitmap);
}

/**
 * __nicGetPendingCmdInfo() - a non-thread-safe function to dequeue a command
 * from prAdapter->rPendingCmdQueue with given sequence number.
 *
 * This function is used internally to avoid deadlock; in most cases, callers
 * shall call nicGetPendingCmdInfo, the thread-safe version.
 *
 * @param    prAdapter   Pointer to struct ADAPTER
 *           ucSeqNum    Sequential Number
 *
 * @retval - Pointer to struct CMD_INFO with matched sequence number, or NULL.
 *
 * NOTE  This function is declared as static to avoid unintended misuse.
 */
static struct CMD_INFO *__nicGetPendingCmdInfo(struct ADAPTER *prAdapter,
					uint8_t ucSeqNum)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct CMD_INFO *prCmdInfo = NULL;

	ASSERT(prAdapter);

	prCmdQue = &prAdapter->rPendingCmdQueue;
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->ucCmdSeqNum == ucSeqNum)
			break;

		QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);

		prCmdInfo = NULL;

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}
	QUEUE_CONCATENATE_QUEUES(prCmdQue, prTempCmdQue);

	return prCmdInfo;
}

/**
 * nicGetPendingCmdInfo() - a thread-safe version of nicGetPendingCmdInfo
 * used to dequeue from prAdapter->rPendingCmdQueue with given sequence number
 *
 * @param    prAdapter   Pointer to struct ADAPTER
 *           ucSeqNum    Sequential Number
 *
 * @retval - Pointer to struct CMD_INFO with matched sequence number, or NULL.
 */
struct CMD_INFO *nicGetPendingCmdInfo(struct ADAPTER *prAdapter,
					uint8_t ucSeqNum)
{
	struct CMD_INFO *prCmdInfo = NULL;
#if CFG_DBG_MGT_BUF
	struct MEM_TRACK *prMemTrack = NULL;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	if (!prAdapter)
		return NULL;

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

	prCmdInfo = __nicGetPendingCmdInfo(prAdapter, ucSeqNum);

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

	if (prCmdInfo) {
		if (wlanIfCmdDbgEn(prAdapter)) {
			DBGLOG(TX, INFO,
				"Get command: %p, %ps, cmd=0x%02X, seq=%u",
				prCmdInfo, prCmdInfo->pfCmdDoneHandler,
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		} else {
			DBGLOG(TX, INFO,
				"Get command: %p, %p, cmd=0x%02X, seq=%u",
				prCmdInfo, prCmdInfo->pfCmdDoneHandler,
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		}
#if CFG_DBG_MGT_BUF
		if (prCmdInfo->pucInfoBuffer &&
		    !IS_FROM_BUF(prAdapter, prCmdInfo->pucInfoBuffer)) {
			prMemTrack = CONTAINER_OF(
					(uint8_t (*)[])prCmdInfo->pucInfoBuffer,
					struct MEM_TRACK, aucData);
		}

		/* 0x60 means the CmdId is in PendingCmdQuene and already
		 * report to module
		 */
		if (prMemTrack)
			prMemTrack->ucWhere = 0x60;
#endif
	}

	return prCmdInfo;
}

/**
 * removeDuplicatePendingCmd() - Remove pending command with duplicate sequence
 */
void removeDuplicatePendingCmd(struct ADAPTER *prAdapter,
				struct CMD_INFO *prCmdInfo)
{
	struct CMD_INFO *prPendingDupCmdInfo;

#if CFG_TX_CMD_SMART_SEQUENCE
	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
#endif /* CFG_TX_CMD_SMART_SEQUENCE */

	prPendingDupCmdInfo = __nicGetPendingCmdInfo(prAdapter,
				prCmdInfo->ucCmdSeqNum);

#if CFG_TX_CMD_SMART_SEQUENCE
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
#endif /* CFG_TX_CMD_SMART_SEQUENCE */

	if (prPendingDupCmdInfo) {
		DBGLOG(TX, ERROR,
			"Remove command: %p, %ps, cmd=0x%02X, seq=%u",
			prPendingDupCmdInfo,
			prPendingDupCmdInfo->pfCmdDoneHandler,
			prPendingDupCmdInfo->ucCID,
			prPendingDupCmdInfo->ucCmdSeqNum);

		cmdBufFreeCmdInfo(prAdapter, prPendingDupCmdInfo);
	}
}

static u_int8_t isPendingTxsData(uint8_t ucPID, struct MSDU_INFO *prMsduInfo)
{
	u_int8_t result;

#if CFG_SUPPORT_SEPARATE_TXS_PID_POOL
	if (!IS_TXS_DATA_PID(ucPID))
		result = FALSE;
	else
		result = TRUE;
#else
		result = FALSE;
#endif

	result = result && IS_TXS_STATELESS_DATA_TYPE(prMsduInfo->ucPktType);

	return result;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to dequeue from
 *        prAdapter->rTxCtrl.rTxMgmtTxingQueue
 *        by matching (wlanIndex, pid) or
 *        (wlanIndex, tid) for DATA if CFG_SUPPORT_SEPARATE_TXS_PID_POOL == 1
 *        if pending for long time (2000ms).
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *           ucWlanInde  Wlan index
 *           ucPID       Packet id
 *           ucTID       Traffic id
 *
 * @retval Pinter to MSDU_INFO
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *nicGetPendingTxMsduInfo(struct ADAPTER *prAdapter,
		uint8_t ucWlanIndex, uint8_t ucPID, uint8_t ucTID)
{
	struct QUE *prTxingQue;
	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct MSDU_INFO *prMsduInfo = NULL;
	OS_SYSTIME now = kalGetTimeTick();

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

	prTxingQue = &(prAdapter->rTxCtrl.rTxMgmtTxingQueue);
	QUEUE_MOVE_ALL(prTempQue, prTxingQue);

	QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry, struct QUE_ENTRY *);
	while (prQueueEntry) {
		prMsduInfo = (struct MSDU_INFO *) prQueueEntry;

		DBGLOG(TX, TEMP,
			"Looking for w/p/t=%u/%u/%u; MSDU info: w/w'/p/t/up/tp=%u/%u/%u/%u/%u/%u\n",
			ucWlanIndex, ucPID, ucTID,
			prMsduInfo->ucWlanIndex,
			prMsduInfo->ucTxdWlanIdx,
			prMsduInfo->ucPID,
			prMsduInfo->ucTC,
			prMsduInfo->ucUserPriority,
			prMsduInfo->ucPktType);

		if (isPendingTxsData(ucPID, prMsduInfo)) {
			/**
			 * Find by a perfect match (widx, tid, pid).
			 * or long-lived (widx, tid) matching.
			 */
			if (prMsduInfo->ucTxdWlanIdx == ucWlanIndex &&
			    prMsduInfo->ucUserPriority == ucTID) {
#if CFG_ENABLE_PKT_LIFETIME_PROFILE
				struct PKT_PROFILE *prPktProfile;
				OS_SYSTIME diff;
				OS_SYSTIME expired_sec =
					prAdapter->u4LongestPending + 2;

				prPktProfile = &prMsduInfo->rPktProfile;
				diff = now - prPktProfile->rHifTxDoneTimestamp;
				if (prPktProfile->rHifTxDoneTimestamp &&
				    diff > expired_sec * 1000)
					break;
#endif
				if (prMsduInfo->ucPID == ucPID)
					break;

				DBGLOG_LIMITED(TX, WARN,
				       "Skipped Msdu WIDX:PID:TID[%u:%u:%u] len=%u\n",
				       ucWlanIndex, prMsduInfo->ucPID, ucTID,
				       QUEUE_LENGTH(prTempQue));
			}
		} else {
			if (prMsduInfo->ucTxdWlanIdx == ucWlanIndex &&
			    prMsduInfo->ucPID == ucPID)
				break;
		}

		QUEUE_INSERT_TAIL(prTxingQue, prQueueEntry);

		prMsduInfo = NULL;

		QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry, struct QUE_ENTRY *);
	}
	QUEUE_CONCATENATE_QUEUES(prTxingQue, prTempQue);

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

	if (prMsduInfo) {
		DBGLOG(TX, TRACE,
		       "Get Msdu TXDWIDX:PID:TID[%u:%u(%u):%u] SEQ[%u] from Pending Q(len=%u)\n",
		       prMsduInfo->ucTxdWlanIdx, prMsduInfo->ucPID, ucPID,
		       prMsduInfo->ucUserPriority, prMsduInfo->ucTxSeqNum,
		       QUEUE_LENGTH(prTxingQue));
	} else {
		DBGLOG(TX, WARN,
		       "Cannot get Target Msdu WIDX:PID:TID[%u:%u:%u] from Pending Q(len=%u)\n",
		       ucWlanIndex, ucPID, ucTID, QUEUE_LENGTH(prTxingQue));
	}

	return prMsduInfo;
}

void nicFreePendingTxMsduInfo(struct ADAPTER *prAdapter,
	uint8_t ucIndex, enum ENUM_REMOVE_BY_MSDU_TPYE ucFreeType)
{
	struct QUE *prTxingQue;
	struct QUE rTempQue;
	struct QUE *prTempQue = &rTempQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct MSDU_INFO *prMsduInfoListHead = (struct MSDU_INFO *)
					       NULL;
	struct MSDU_INFO *prMsduInfoListTail = (struct MSDU_INFO *)
					       NULL;
	struct MSDU_INFO *prMsduInfo = (struct MSDU_INFO *) NULL;
	uint8_t ucRemoveByIndex = 255;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (ucFreeType >= ENUM_REMOVE_BY_MSDU_TPYE_NUM) {
		DBGLOG(TX, WARN, "Wrong remove type: %d\n", ucFreeType);
		return;
	}

	DBGLOG(NIC, TRACE, "ucIndex: %d, ucFreeType: %d\n",
			ucIndex, ucFreeType);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

	prTxingQue = &(prAdapter->rTxCtrl.rTxMgmtTxingQueue);
	QUEUE_MOVE_ALL(prTempQue, prTxingQue);

	QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry,
			  struct QUE_ENTRY *);

	while (prQueueEntry) {
		prMsduInfo = (struct MSDU_INFO *) prQueueEntry;

		switch (ucFreeType) {
		case MSDU_REMOVE_BY_WLAN_INDEX:
			ucRemoveByIndex = prMsduInfo->ucWlanIndex;
			break;
		case MSDU_REMOVE_BY_BSS_INDEX:
			ucRemoveByIndex = prMsduInfo->ucBssIndex;
			break;
		case MSDU_REMOVE_BY_ALL:
			ucRemoveByIndex = 0xFF;
			break;
		default:
			break;
		}

		if (ucRemoveByIndex == ucIndex) {
			DBGLOG(TX, TRACE,
			       "%s: Get Msdu WIDX:PID[%u:%u] SEQ[%u] from Pending Q\n",
			       __func__, prMsduInfo->ucWlanIndex,
			       prMsduInfo->ucPID,
			       prMsduInfo->ucTxSeqNum);

			if (prMsduInfoListHead == NULL) {
				prMsduInfoListHead =
					prMsduInfoListTail = prMsduInfo;
			} else {
				QUEUE_ENTRY_SET_NEXT(prMsduInfoListTail,
						prMsduInfo);
				prMsduInfoListTail = prMsduInfo;
			}
		} else {
			QUEUE_INSERT_TAIL(prTxingQue, prQueueEntry);

			prMsduInfo = NULL;
		}

		QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}
	QUEUE_CONCATENATE_QUEUES(prTxingQue, prTempQue);

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TXING_MGMT_LIST);

	/* free */
	if (prMsduInfoListHead) {
		nicTxFreeMsduInfoPacket(prAdapter, prMsduInfoListHead);
		nicTxReturnMsduInfo(prAdapter, prMsduInfoListHead);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to retrieve a CMD sequence number atomically
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *
 * @retval - UINT_8
 */
/*----------------------------------------------------------------------------*/
uint8_t nicIncreaseCmdSeqNum(struct ADAPTER *prAdapter)
{
	uint8_t ucRetval;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_SEQ_NUM);

	prAdapter->ucCmdSeqNum++;
	/* use 0 as reserved cmd. seq. for skip event seq. check */
	if (prAdapter->ucCmdSeqNum == 0)
		prAdapter->ucCmdSeqNum = 1;
	ucRetval = prAdapter->ucCmdSeqNum;

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_SEQ_NUM);

	return ucRetval;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This procedure is used to retrieve a TX sequence number atomically
 *
 * @param    prAdapter   Pointer of ADAPTER_T
 *
 * @retval - UINT_8
 */
/*----------------------------------------------------------------------------*/
uint8_t nicIncreaseTxSeqNum(struct ADAPTER *prAdapter)
{
	uint8_t ucRetval;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_SEQ_NUM);

	ucRetval = prAdapter->ucTxSeqNum;

	prAdapter->ucTxSeqNum++;

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_SEQ_NUM);

	return ucRetval;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to handle
 *        media state change event
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicMediaStateChange(struct ADAPTER *prAdapter,
		    uint8_t ucBssIndex,
		    struct EVENT_CONNECTION_STATUS *prConnectionStatus)
{
	struct GLUE_INFO *prGlueInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_INFO *prBssInfo;
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;

	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(NIC, ERROR, "prBssInfo is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWlanInfo = &prAdapter->rWlanInfo;

	switch (prBssInfo->eNetworkType) {
	case NETWORK_TYPE_AIS:
		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
		prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
		if (prConnectionStatus->ucMediaStatus ==
		    MEDIA_STATE_DISCONNECTED) {	/* disconnected */
			if (kalGetMediaStateIndicated(prGlueInfo,
				ucBssIndex) !=
			    MEDIA_STATE_DISCONNECTED ||
			    prAisFsmInfo->eCurrentState == AIS_STATE_JOIN) {

				kalIndicateStatusAndComplete(prGlueInfo,
					WLAN_STATUS_MEDIA_DISCONNECT, NULL,
					0, ucBssIndex);

				prWlanInfo->u4SysTime = kalGetTimeTick();
			}

			/* reset buffered link quality information */
			prAdapter->rLinkQuality.rLq[ucBssIndex].
				fgIsLinkQualityValid = FALSE;
			prAdapter->rLinkQuality.rLq[ucBssIndex].
				fgIsLinkRateValid = FALSE;
		} else if (prConnectionStatus->ucMediaStatus ==
			   MEDIA_STATE_CONNECTED) {	/* connected */
			struct PARAM_BSSID_EX *prCurrBssid =
				aisGetCurrBssId(prAdapter, ucBssIndex);
			uint8_t ucAuthorized = FALSE;

			if (EQUAL_SSID(prCurrBssid->rSsid.aucSsid,
			    prCurrBssid->rSsid.u4SsidLen,
			    prConnectionStatus->aucSsid,
			    prConnectionStatus->ucSsidLen) &&
			    EQUAL_MAC_ADDR(prCurrBssid->arMacAddress,
			    prConnectionStatus->aucBssid)) {
				ucAuthorized = TRUE;
				DBGLOG(TX, INFO, "pre-authorized\n");
			}

			prWlanInfo->u4SysTime = kalGetTimeTick();

			/* sanity check */
			if (unlikely(prConnectionStatus->ucSsidLen >
				ELEM_MAX_LEN_SSID))
				prConnectionStatus->ucSsidLen =
					ELEM_MAX_LEN_SSID;

			/* fill information for association result */
			prCurrBssid->rSsid.u4SsidLen =
				prConnectionStatus->ucSsidLen;
			kalMemCopy(
				prCurrBssid->rSsid.aucSsid,
				prConnectionStatus->aucSsid,
				prConnectionStatus->ucSsidLen);
			kalMemCopy(prCurrBssid->arMacAddress,
				   prConnectionStatus->aucBssid, MAC_ADDR_LEN);
			prCurrBssid->u4Privacy =
				prConnectionStatus->ucEncryptStatus;/* @FIXME */
			prCurrBssid->rRssi = 0; /* @FIXME */
			prCurrBssid->eNetworkTypeInUse =
				PARAM_NETWORK_TYPE_AUTOMODE;/* @FIXME */
			prCurrBssid->
			rConfiguration.u4BeaconPeriod
				= prConnectionStatus->u2BeaconPeriod;
			prCurrBssid->
			rConfiguration.u4ATIMWindow
				= prConnectionStatus->u2ATIMWindow;
			prCurrBssid->
				rConfiguration.u4DSConfig =
				prConnectionStatus->u4FreqInKHz;
			prWlanInfo->ucNetworkType[ucBssIndex] =
				prConnectionStatus->ucNetworkType;
			prCurrBssid->eOpMode
				= (enum ENUM_PARAM_OP_MODE)
					prConnectionStatus->ucInfraMode;

			/* always indicate to OS according to
			 * MSDN (re-association/roaming)
			 */
			if (kalGetMediaStateIndicated(prGlueInfo,
				ucBssIndex) !=
			    MEDIA_STATE_CONNECTED) {
				kalIndicateStatusAndComplete(prGlueInfo,
					WLAN_STATUS_MEDIA_CONNECT, NULL,
					0, ucBssIndex);
			} else {
				/* connected -> connected : roaming ? */
				kalIndicateStatusAndComplete(prGlueInfo,
					WLAN_STATUS_ROAM_OUT_FIND_BEST,
					&ucAuthorized,
					sizeof(ucAuthorized), ucBssIndex);
			}
		}
		break;

#if CFG_ENABLE_BT_OVER_WIFI
	case NETWORK_TYPE_BOW:
		break;
#endif

#if CFG_ENABLE_WIFI_DIRECT
	case NETWORK_TYPE_P2P:
		break;
#endif
	default:
		ASSERT(0);
	}

	return WLAN_STATUS_SUCCESS;
}				/* nicMediaStateChange */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to generate a join failure event to OS
 *
 * @param
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
uint32_t nicMediaJoinFailure(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex, uint32_t rStatus)
{
	struct GLUE_INFO *prGlueInfo;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;

	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
		return WLAN_STATUS_FAILURE;
	}

	switch (GET_BSS_INFO_BY_INDEX(prAdapter,
				      ucBssIndex)->eNetworkType) {
	case NETWORK_TYPE_AIS:
		kalIndicateStatusAndComplete(prGlueInfo, rStatus, NULL, 0,
			ucBssIndex);

		break;

	case NETWORK_TYPE_BOW:
	case NETWORK_TYPE_P2P:
	default:
		break;
	}

	return WLAN_STATUS_SUCCESS;
}				/* end of nicMediaJoinFailure() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to convert between
 *        frequency and channel number
 *
 * @param u4ChannelNum
 *
 * @retval - Frequency in unit of KHz, 0 for invalid channel number
 */
/*----------------------------------------------------------------------------*/
uint32_t nicChannelNum2Freq(uint32_t u4ChannelNum, enum ENUM_BAND eBand)
{
	uint32_t u4ChannelInMHz;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		if (u4ChannelNum >= 1 && u4ChannelNum <= 233) {
			if (u4ChannelNum == 2)
				u4ChannelInMHz = 5935;
			else
				u4ChannelInMHz = 5950 + u4ChannelNum * 5;
		} else
			u4ChannelInMHz = 0;
	} else
#endif
	if (u4ChannelNum >= 1 && u4ChannelNum <= 13)
		u4ChannelInMHz = 2412 + (u4ChannelNum - 1) * 5;
	else if (u4ChannelNum == 14)
		u4ChannelInMHz = 2484;
	else if (u4ChannelNum == 133)
		u4ChannelInMHz = 3665;	/* 802.11y */
	else if (u4ChannelNum == 137)
		u4ChannelInMHz = 3685;	/* 802.11y */
	else if ((u4ChannelNum >= 34 && u4ChannelNum <= 181)
		 || (u4ChannelNum == 16))
		u4ChannelInMHz = 5000 + u4ChannelNum * 5;
	else if (u4ChannelNum >= 182 && u4ChannelNum <= 196)
		u4ChannelInMHz = 4000 + u4ChannelNum * 5;
	else if (u4ChannelNum == 201)
		u4ChannelInMHz = 2730;
	else if (u4ChannelNum == 202)
		u4ChannelInMHz = 2498;
	else
		u4ChannelInMHz = 0;

	return 1000 * u4ChannelInMHz;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to convert between
 *        frequency and channel number
 *
 * @param u4FreqInKHz
 *
 * @retval - Frequency Number, 0 for invalid freqency
 */
/*----------------------------------------------------------------------------*/
uint32_t nicFreq2ChannelNum(uint32_t u4FreqInKHz)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint32_t u4FreqInMHz = 0;
#endif

	switch (u4FreqInKHz) {
	case 2412000:
		return 1;
	case 2417000:
		return 2;
	case 2422000:
		return 3;
	case 2427000:
		return 4;
	case 2432000:
		return 5;
	case 2437000:
		return 6;
	case 2442000:
		return 7;
	case 2447000:
		return 8;
	case 2452000:
		return 9;
	case 2457000:
		return 10;
	case 2462000:
		return 11;
	case 2467000:
		return 12;
	case 2472000:
		return 13;
	case 2484000:
		return 14;
	case 3665000:
		return 133;	/* 802.11y */
	case 3685000:
		return 137;	/* 802.11y */
	case 4915000:
		return 183;
	case 4920000:
		return 184;
	case 4925000:
		return 185;
	case 4930000:
		return 186;
	case 4935000:
		return 187;
	case 4940000:
		return 188;
	case 4945000:
		return 189;
	case 4960000:
		return 192;
	case 4980000:
		return 196;
	case 5170000:
		return 34;
	case 5180000:
		return 36;
	case 5190000:
		return 38;
	case 5200000:
		return 40;
	case 5210000:
		return 42;
	case 5220000:
		return 44;
	case 5230000:
		return 46;
	case 5240000:
		return 48;
	case 5250000:
		return 50;
	case 5260000:
		return 52;
	case 5270000:
		return 54;
	case 5280000:
		return 56;
	case 5290000:
		return 58;
	case 5300000:
		return 60;
	case 5310000:
		return 62;
	case 5320000:
		return 64;
	case 5500000:
		return 100;
	case 5510000:
		return 102;
	case 5520000:
		return 104;
	case 5530000:
		return 106;
	case 5540000:
		return 108;
	case 5550000:
		return 110;
	case 5560000:
		return 112;
	case 5570000:
		return 114;
	case 5580000:
		return 116;
	case 5590000:
		return 118;
	case 5600000:
		return 120;
	case 5610000:
		return 122;
	case 5620000:
		return 124;
	case 5630000:
		return 126;
	case 5640000:
		return 128;
	case 5660000:
		return 132;
	case 5670000:
		return 134;
	case 5680000:
		return 136;
	case 5690000:
		return 138;
	case 5700000:
		return 140;
	case 5710000:
		return 142;
	case 5720000:
		return 144;
	case 5745000:
		return 149;
	case 5755000:
		return 151;
	case 5765000:
		return 153;
	case 5775000:
		return 155;
	case 5785000:
		return 157;
	case 5795000:
		return 159;
	case 5805000:
		return 161;
	case 5825000:
		return 165;
	case 5845000:
		return 169;
	case 5865000:
		return 173;
	default:
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (u4FreqInKHz % 5000 == 0) {
			u4FreqInMHz = u4FreqInKHz / 1000;
			if ((u4FreqInMHz > 5950) && (u4FreqInMHz <= 7115))
				return ((u4FreqInMHz - 5950) / 5);
			else if (u4FreqInMHz == 5935)
				return 2;
		}
#endif
		DBGLOG(BSS, INFO, "Return Invalid Channelnum = 0.\n");
		return 0;
	}
}

uint8_t nicChannelInfo2OpClass(struct RF_CHANNEL_INFO *prChannelInfo)
{
	uint8_t ucVhtOpClass;
	uint32_t u4Freq = prChannelInfo->u4CenterFreq1;

	if (u4Freq >= 2412 && u4Freq <= 2472) {
		/* 2.407 GHz, channels 1..13 */
		if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ) {
			if (u4Freq > prChannelInfo->u2PriChnlFreq)
				return 83; /* HT40+ */
			else
				return 84; /* HT40- */
		} else
			return 81;
	}

	if (u4Freq == 2484)
		return 82; /* channel 14 */

	switch (prChannelInfo->ucChnlBw) {
	case MAX_BW_80MHZ:
		ucVhtOpClass = 128;
		break;
	case MAX_BW_160MHZ:
		ucVhtOpClass = 129;
		break;
	case MAX_BW_80_80_MHZ:
		ucVhtOpClass = 130;
		break;
	default:
		ucVhtOpClass = 0;
		break;
	}

	/* 5 GHz, channels 36..48 */
	if (u4Freq >= 5180 && u4Freq <= 5240) {
		if (ucVhtOpClass)
			return ucVhtOpClass;
		else if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ) {
			if (u4Freq > prChannelInfo->u2PriChnlFreq)
				return 116;
			else
				return 117;
		} else
			return 115;
	}

	/* 5 GHz, channels 52..64 */
	if (u4Freq >= 5260 && u4Freq <= 5320) {
		if (ucVhtOpClass)
			return ucVhtOpClass;
		else if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ) {
			if (u4Freq > prChannelInfo->u2PriChnlFreq)
				return 119;
			else
				return 120;
		} else
			return 118;
	}

	/* 5 GHz, channels 100..144 */
	if (u4Freq >= 5500 && u4Freq <= 5720) {
		if (ucVhtOpClass)
			return ucVhtOpClass;
		else if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ) {
			if (u4Freq > prChannelInfo->u2PriChnlFreq)
				return 122;
			else
				return 123;
		} else
			return 121;
	}

	/* 5 GHz, channels 149..169 */
	if (u4Freq >= 5745 && u4Freq <= 5845) {
		if (ucVhtOpClass)
			return ucVhtOpClass;
		else if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ) {
			if (u4Freq > prChannelInfo->u2PriChnlFreq)
				return 126;
			else
				return 127;
		} else if (u4Freq <= 5805)
			return 124;
		else
			return 125;
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* 6 GHz, channels 1..233 */
	if (u4Freq > 5950 && u4Freq <= 7115) {
		switch (prChannelInfo->ucChnlBw) {
		case MAX_BW_80MHZ:
			return 133;
		case MAX_BW_160MHZ:
			return 134;
		case MAX_BW_80_80_MHZ:
			return 135;
		case MAX_BW_320_1MHZ:
		case MAX_BW_320_2MHZ:
			return 137;
		default:
			if (prChannelInfo->ucChnlBw == MAX_BW_40MHZ)
				return 132;
			else
				return 131;
		}
	}

	if (u4Freq == 5935)
		return 136;
#endif

	DBGLOG(NIC, ERROR,
		"Get op class failed, band=%d channel=%d bw=%d freq=%d s1=%d s2=%d\n",
		prChannelInfo->eBand,
		prChannelInfo->ucChannelNum,
		prChannelInfo->ucChnlBw,
		prChannelInfo->u2PriChnlFreq,
		prChannelInfo->u4CenterFreq1,
		prChannelInfo->u4CenterFreq2);

	return 0;
}

enum ENUM_CHNL_EXT nicGetSco(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, uint8_t ucPrimaryCh)
{
	enum ENUM_CHNL_EXT eSCO = CHNL_EXT_SCN;
	uint8_t ucSecondChannel;

	if (eBand == BAND_2G4) {
		if (ucPrimaryCh != 14)
			eSCO = (ucPrimaryCh > 7) ? CHNL_EXT_SCB : CHNL_EXT_SCA;
	} else {
		if (regd_is_single_sku_en()) {
			if (rlmDomainIsLegalChannel(prAdapter,
					eBand,
					ucPrimaryCh))
				eSCO = rlmSelectSecondaryChannelType(prAdapter,
						eBand,
						ucPrimaryCh);
		} else {
			struct DOMAIN_INFO_ENTRY *prDomainInfo =
					rlmDomainGetDomainInfo(prAdapter);
			struct DOMAIN_SUBBAND_INFO *prSubband;
			uint8_t i, j;

			for (i = 0; i < MAX_SUBBAND_NUM; i++) {
				prSubband = &prDomainInfo->rSubBand[i];
				if (prSubband->ucBand != eBand)
					continue;
				for (j = 0; j < prSubband->ucNumChannels; j++) {
					if ((prSubband->ucFirstChannelNum +
						j * prSubband->ucChannelSpan) ==
						ucPrimaryCh) {
						eSCO = (j & 1) ?
							CHNL_EXT_SCB :
							CHNL_EXT_SCA;
						break;
					}
				}

				if (j < prSubband->ucNumChannels)
					break;	/* Found */
			}
		}
	}
	/* Check if it is boundary channel
	 * and 40MHz BW is permitted
	*/
	if (eSCO != CHNL_EXT_SCN) {
		ucSecondChannel = (eSCO == CHNL_EXT_SCA)
			? (ucPrimaryCh + CHNL_SPAN_20)
			: (ucPrimaryCh - CHNL_SPAN_20);

		if (!rlmDomainIsLegalChannel(prAdapter,
			eBand,
			ucSecondChannel))
			eSCO = CHNL_EXT_SCN;
	}
	return eSCO;
}

uint8_t nicGetSecCh(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand,
		enum ENUM_CHNL_EXT eSCO,
		uint8_t ucPrimaryCh)
{
	uint8_t ucSecondCh;

	if (eSCO == CHNL_EXT_SCN)
		return 0;

	if (eSCO == CHNL_EXT_SCA)
		ucSecondCh = ucPrimaryCh + CHNL_SPAN_20;
	else
		ucSecondCh = ucPrimaryCh - CHNL_SPAN_20;

	if (!rlmDomainIsLegalChannel(prAdapter, eBand, ucSecondCh))
		ucSecondCh = 0;

	return ucSecondCh;
}

uint32_t nicGetS1Freq(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth)
{
	uint8_t ucS1Channel;
	uint8_t ucSecChannel;
	enum ENUM_CHNL_EXT eSCO;
	uint8_t ucVhtBw;

	if (ucBandwidth == MAX_BW_20MHZ) {
		ucS1Channel = ucPrimaryChannel;
	} else if (ucBandwidth == MAX_BW_40MHZ) {
		eSCO = nicGetSco(prAdapter, eBand, ucPrimaryChannel);
		ucSecChannel = nicGetSecCh(prAdapter, eBand, eSCO,
					      ucPrimaryChannel);
		ucS1Channel = (ucPrimaryChannel + ucSecChannel) / 2;
	} else {
		ucVhtBw = rlmGetVhtOpBwByBssOpBw(ucBandwidth);
		ucS1Channel = nicGetS1(prAdapter, eBand, ucPrimaryChannel,
				       ucVhtBw);
	}

	return nicChannelNum2Freq(ucS1Channel, eBand) / 1000;
}

uint8_t nicGetS2(enum ENUM_BAND eBand,
		uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth,
		uint8_t ucS1)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G)
		return nicGetHe6gS2(ucPrimaryChannel, ucBandwidth, ucS1);
#endif
	return 0;
}

#if (CFG_SUPPORT_WIFI_6G == 1)
uint8_t nicGetHe6gS2(uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth,
		uint8_t ucS1)
{
	if (ucBandwidth == CW_160MHZ) {
		if (ucPrimaryChannel > ucS1)
			return ucS1 + 8;
		else if (ucPrimaryChannel < ucS1)
			return ucS1 - 8;
	}

	return 0;
}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
/* In EHT mode S2 (CCFS1) apply central CH for BW160 and BW320,
 * apply 0 for the rest (e.g. BW20/40/80)
 */
uint8_t nicGetEhtS2(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G)
		return nicGetEht6gS2(prAdapter, ucPrimaryChannel, ucBandwidth);
#endif
	/* 2.4G/5G case: only BW160 to be considered */
	if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_160) {
		return nicGetVhtS1(prAdapter, eBand, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_160);
	} else {
		return 0;
	}
}

#if (CFG_SUPPORT_WIFI_6G == 1)
uint8_t nicGetEht6gS2(struct ADAPTER *prAdapter, uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth)
{
	if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_160) {
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_160);
	} else if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_320_1) {
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_320_1);
	} else if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_320_2) {
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_320_2);
	} else {
		return 0;
	}
}
#endif

/* In EHT mode S1 (CCFS0) apply central CH of BW80 for BW160,
 * central CH of BW160 for BW320 respectively,
 * apply central CH of each for the rest (e.g. BW20/40/80)
 */
uint8_t nicGetEhtS1(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G)
		return nicGetEht6gS1(prAdapter, ucPrimaryChannel, ucBandwidth);
#endif
	/*2.4G/5G case: only BW160 to be considered */
	if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_160) {
		return nicGetVhtS1(prAdapter, eBand, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_80);
	} else {
		return nicGetVhtS1(prAdapter, eBand, ucPrimaryChannel,
			ucBandwidth);
	}
}

#if (CFG_SUPPORT_WIFI_6G == 1)
uint8_t nicGetEht6gS1(struct ADAPTER *prAdapter, uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth)
{
	if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_160) {
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_80);
	} else if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_320_1 ||
		   ucBandwidth == VHT_OP_CHANNEL_WIDTH_320_2) {
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel,
			VHT_OP_CHANNEL_WIDTH_160);
	} else {
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel,
			ucBandwidth);
	}
}
#endif

#endif /* CFG_SUPPORT_802_11BE */

uint8_t nicGetS1(struct ADAPTER *prAdapter,
		 enum ENUM_BAND eBand,
		 uint8_t ucPrimaryChannel,
		 uint8_t ucBandwidth)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G)
		return nicGetHe6gS1(prAdapter, ucPrimaryChannel, ucBandwidth);
#endif
	return nicGetVhtS1(prAdapter, eBand, ucPrimaryChannel, ucBandwidth);
}

uint8_t nicGetVhtS1(struct ADAPTER *prAdapter,
		    enum ENUM_BAND eBand,
		    uint8_t ucPrimaryChannel,
		    uint8_t ucBandwidth)
{
	enum ENUM_CHNL_EXT eSCO;
	uint8_t ucSecChannel;

	if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_20_40) {
		eSCO = nicGetSco(prAdapter, eBand, ucPrimaryChannel);

		if (eSCO == CHNL_EXT_SCN)
			return ucPrimaryChannel;
		else if (eSCO == CHNL_EXT_SCA || eSCO == CHNL_EXT_SCB) {
			ucSecChannel = nicGetSecCh(prAdapter, eBand, eSCO,
						   ucPrimaryChannel);
			return (ucPrimaryChannel + ucSecChannel) / 2;
		}
	} else if ((ucBandwidth == VHT_OP_CHANNEL_WIDTH_80)
	    || (ucBandwidth == VHT_OP_CHANNEL_WIDTH_80P80)) {

		if (ucPrimaryChannel >= 36 && ucPrimaryChannel <= 48)
			return 42;
		else if (ucPrimaryChannel >= 52 && ucPrimaryChannel <= 64)
			return 58;
		else if (ucPrimaryChannel >= 100 && ucPrimaryChannel <= 112)
			return 106;
		else if (ucPrimaryChannel >= 116 && ucPrimaryChannel <= 128)
			return 122;
		else if (ucPrimaryChannel >= 132 && ucPrimaryChannel <= 144)
			return 138;
		else if (ucPrimaryChannel >= 149 && ucPrimaryChannel <= 161)
			return 155;

	} else if (ucBandwidth == VHT_OP_CHANNEL_WIDTH_160) {

		if (ucPrimaryChannel >= 36 && ucPrimaryChannel <= 64)
			return 50;
		else if (ucPrimaryChannel >= 100 && ucPrimaryChannel <= 128)
			return 114;
	} else {

		return 0;
	}

	return 0;

}

#if (CFG_SUPPORT_WIFI_6G == 1)
uint8_t nicGetHe6gS1(struct ADAPTER *prAdapter,
		     uint8_t ucPrimaryChannel,
		     uint8_t ucBandwidth)
{
	enum ENUM_CHNL_EXT eSCO;
	uint8_t ucSecChannel;

	if (ucBandwidth == CW_20_40MHZ) {
		eSCO = nicGetSco(prAdapter, BAND_6G, ucPrimaryChannel);

		if (eSCO == CHNL_EXT_SCN)
			return ucPrimaryChannel;
		else if (eSCO == CHNL_EXT_SCA || eSCO == CHNL_EXT_SCB) {
			ucSecChannel = nicGetSecCh(prAdapter, BAND_6G, eSCO,
						   ucPrimaryChannel);
			return (ucPrimaryChannel + ucSecChannel) / 2;
		}
	} else if ((ucBandwidth == CW_80MHZ)
	    || (ucBandwidth == CW_80P80MHZ)) {

		if (ucPrimaryChannel >= 1 && ucPrimaryChannel <= 13)
			return 7;
		else if (ucPrimaryChannel >= 17 && ucPrimaryChannel <= 29)
			return 23;
		else if (ucPrimaryChannel >= 33 && ucPrimaryChannel <= 45)
			return 39;
		else if (ucPrimaryChannel >= 49 && ucPrimaryChannel <= 61)
			return 55;
		else if (ucPrimaryChannel >= 65 && ucPrimaryChannel <= 77)
			return 71;
		else if (ucPrimaryChannel >= 81 && ucPrimaryChannel <= 93)
			return 87;
		else if (ucPrimaryChannel >= 97 && ucPrimaryChannel <= 109)
			return 103;
		else if (ucPrimaryChannel >= 113 && ucPrimaryChannel <= 125)
			return 119;
		else if (ucPrimaryChannel >= 129 && ucPrimaryChannel <= 141)
			return 135;
		else if (ucPrimaryChannel >= 145 && ucPrimaryChannel <= 157)
			return 151;
		else if (ucPrimaryChannel >= 161 && ucPrimaryChannel <= 173)
			return 167;
		else if (ucPrimaryChannel >= 177 && ucPrimaryChannel <= 189)
			return 183;
		else if (ucPrimaryChannel >= 193 && ucPrimaryChannel <= 205)
			return 199;
		else if (ucPrimaryChannel >= 209 && ucPrimaryChannel <= 221)
			return 215;
		else if (ucPrimaryChannel >= 225 && ucPrimaryChannel <= 237)
			return 231;
		else if (ucPrimaryChannel >= 241 && ucPrimaryChannel <= 253)
			return 249;
	} else if (ucBandwidth == CW_160MHZ) {

		if (ucPrimaryChannel >= 1 && ucPrimaryChannel <= 29)
			return 15;
		else if (ucPrimaryChannel >= 33 && ucPrimaryChannel <= 61)
			return 47;
		else if (ucPrimaryChannel >= 65 && ucPrimaryChannel <= 93)
			return 79;
		else if (ucPrimaryChannel >= 97 && ucPrimaryChannel <= 125)
			return 111;
		else if (ucPrimaryChannel >= 129 && ucPrimaryChannel <= 157)
			return 143;
		else if (ucPrimaryChannel >= 161 && ucPrimaryChannel <= 189)
			return 175;
		else if (ucPrimaryChannel >= 193 && ucPrimaryChannel <= 221)
			return 207;
	} else if (ucBandwidth == CW_320_1MHZ) {
		if (ucPrimaryChannel >= 1 && ucPrimaryChannel <= 61)
			return 31;
		else if (ucPrimaryChannel >= 65 && ucPrimaryChannel <= 125)
			return 95;
		else if (ucPrimaryChannel >= 129 && ucPrimaryChannel <= 189)
			return 159;
	} else if (ucBandwidth == CW_320_2MHZ) {
		if (ucPrimaryChannel >= 33 && ucPrimaryChannel <= 93)
			return 63;
		else if (ucPrimaryChannel >= 97 && ucPrimaryChannel <= 157)
			return 127;
		else if (ucPrimaryChannel >= 161 && ucPrimaryChannel <= 221)
			return 191;
	} else {
		return 0;
	}
	return 0;
}
#endif /* (CFG_SUPPORT_WIFI_6G == 1) */

/* firmware command wrapper */
/* NETWORK (WIFISYS) */

uint32_t nicActivateNetwork(struct ADAPTER *prAdapter,
			    uint8_t ucNetworkIndex)
{
	return nicActivateNetworkEx(prAdapter, ucNetworkIndex, TRUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to activate WIFISYS for specified
 *        network
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eNetworkTypeIdx    Index of network type
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicActivateNetworkEx(struct ADAPTER *prAdapter,
			    uint8_t ucNetworkIndex,
			    uint8_t fgReset40mBw)
{
	struct CMD_BSS_ACTIVATE_CTRL rCmdActivateCtrl;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex = NETWORK_BSS_ID(ucNetworkIndex);
	uint8_t ucLinkIndex = NETWORK_LINK_ID(ucNetworkIndex);

	/*	const UINT_8 aucZeroMacAddr[] = NULL_MAC_ADDR; */

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(NIC, INFO, "prBssInfo is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo->u4PresentTime = 0;
	prBssInfo->tmLastPresent = 0;
	prBssInfo->fgFirstArp = TRUE;

	SET_NET_ACTIVE(prAdapter, ucBssIndex);
#if CFG_SAP_RPS_SUPPORT
	if (prAdapter->rWifiVar.fgSapRpsEnable == 1)
		p2pFuncRpsAisCheck(prAdapter,
			prBssInfo,
			TRUE);
#endif

	if (fgReset40mBw) {
		prBssInfo->fg40mBwAllowed = FALSE;
		prBssInfo->fgAssoc40mBwAllowed = FALSE;
	}

	rCmdActivateCtrl.ucBssIndex = ucBssIndex;
	rCmdActivateCtrl.ucActive = 1;
	rCmdActivateCtrl.ucNetworkType = (uint8_t)
					 prBssInfo->eNetworkType;
	rCmdActivateCtrl.ucOwnMacAddrIndex =
		prBssInfo->ucOwnMacIndex;
	COPY_MAC_ADDR(rCmdActivateCtrl.aucBssMacAddr,
		      prBssInfo->aucOwnMacAddr);

	prBssInfo->ucBMCWlanIndex =
		secPrivacySeekForBcEntry(prAdapter, prBssInfo->ucBssIndex,
					 prBssInfo->aucOwnMacAddr,
					 STA_REC_INDEX_NOT_FOUND,
					 CIPHER_SUITE_NONE, 0xFF);
#if CFG_SUPPORT_LIMITED_PKT_PID
	nicTxInitPktPID(prAdapter, prBssInfo->ucBMCWlanIndex);
#endif /* CFG_SUPPORT_LIMITED_PKT_PID */
	rCmdActivateCtrl.ucBMCWlanIndex = prBssInfo->ucBMCWlanIndex;
	rCmdActivateCtrl.ucMldLinkIdx = ucLinkIndex;

	DBGLOG(RSN, INFO,
	       "[BSS index]=%d Link=%d OwnMac%d=" MACSTR " BSSID=" MACSTR
	       " BMCIndex = %d NetType=%d\n",
	       ucBssIndex, ucLinkIndex,
	       prBssInfo->ucOwnMacIndex,
	       MAC2STR(prBssInfo->aucOwnMacAddr),
	       MAC2STR(prBssInfo->aucBSSID),
	       prBssInfo->ucBMCWlanIndex, prBssInfo->eNetworkType);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_BSS_ACTIVATE_CTRL,
				   TRUE,
				   FALSE,
				   FALSE,
				   NULL, NULL,
				   sizeof(struct CMD_BSS_ACTIVATE_CTRL),
				   (uint8_t *)&rCmdActivateCtrl, NULL, 0);
}

uint32_t nicDeactivateNetwork(struct ADAPTER *prAdapter,
				uint8_t ucNetworkIndex)
{
	return nicDeactivateNetworkEx(prAdapter, ucNetworkIndex, TRUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to deactivate WIFISYS for specified
 *        network
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eNetworkTypeIdx    Index of network type
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicDeactivateNetworkEx(struct ADAPTER *prAdapter,
				uint8_t ucNetworkIndex,
				uint8_t fgClearStaRec)
{
	uint32_t u4Status;
	struct CMD_BSS_ACTIVATE_CTRL rCmdActivateCtrl;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex = NETWORK_BSS_ID(ucNetworkIndex);
	uint8_t ucLinkIndex = NETWORK_LINK_ID(ucNetworkIndex);

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RSN, ERROR, "prBssInfo is null\n");
		return WLAN_STATUS_FAILURE;
	}
	UNSET_NET_ACTIVE(prAdapter, ucBssIndex);
#if CFG_SAP_RPS_SUPPORT
	if (prAdapter->rWifiVar.fgSapRpsEnable == 1)
		p2pFuncRpsAisCheck(prAdapter,
			prBssInfo,
			FALSE);
#endif

	/* FW only supports BMCWlan index 0 ~ 31.
	 * it always checks BMCWlan index validity and triggers
	 * assertion if BMCWlan index is invalid.
	 */
	if (prBssInfo->ucBMCWlanIndex == WTBL_RESERVED_ENTRY) {
		DBGLOG(RSN, WARN,
		       "Network may be deactivated already, ignore\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	kalMemZero(&rCmdActivateCtrl,
		   sizeof(struct CMD_BSS_ACTIVATE_CTRL));

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	if (ucBssIndex < MAX_BSSID_NUM)
		rlmDomain6GPwrModeUpdate(prAdapter, ucBssIndex,
			PWR_MODE_6G_LPI);
#endif

	rCmdActivateCtrl.ucBssIndex = ucBssIndex;
	rCmdActivateCtrl.ucActive = 0;
	rCmdActivateCtrl.ucNetworkType =
		(uint8_t)prBssInfo->eNetworkType;
	rCmdActivateCtrl.ucOwnMacAddrIndex =
		prBssInfo->ucOwnMacIndex;
	rCmdActivateCtrl.ucBMCWlanIndex =
		prBssInfo->ucBMCWlanIndex;
	rCmdActivateCtrl.ucMldLinkIdx = ucLinkIndex;

	DBGLOG(RSN, INFO,
	       "[BSS index]=%d Link=%d OwnMac=" MACSTR " BSSID=" MACSTR
	       " BMCIndex = %d NetType=%d\n",
	       ucBssIndex, ucLinkIndex,
	       MAC2STR(prBssInfo->aucOwnMacAddr),
	       MAC2STR(prBssInfo->aucBSSID),
	       prBssInfo->ucBMCWlanIndex, prBssInfo->eNetworkType);

	u4Status = wlanSendSetQueryCmd(prAdapter,
				       CMD_ID_BSS_ACTIVATE_CTRL,
				       TRUE,
				       FALSE,
				       FALSE,
				       NULL,
				       NULL,
				       sizeof(struct CMD_BSS_ACTIVATE_CTRL),
				       (uint8_t *)&rCmdActivateCtrl, NULL, 0);

	prBssInfo->ucGrantBW = MAX_BW_UNKNOWN;
	prBssInfo->fgFirstArp = FALSE;

	if (fgClearStaRec) {
		prBssInfo->ucGrantTxNss = 0;
		prBssInfo->ucGrantRxNss = 0;
		prBssInfo->eHwBandIdx = ENUM_BAND_AUTO;
		prBssInfo->eBackupHwBandIdx = ENUM_BAND_AUTO;

		secRemoveBssBcEntry(prAdapter, prBssInfo, FALSE);

		/* free all correlated station records */
		cnmStaFreeAllStaByNetwork(prAdapter, ucBssIndex,
				  STA_REC_EXCLUDE_NONE);
		if (HAL_IS_TX_DIRECT(prAdapter))
			nicTxDirectClearBssAbsentQ(prAdapter, ucBssIndex);
		else
			qmFreeAllByBssIdx(prAdapter, ucBssIndex);

#if (CFG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK == 0)
		nicFreePendingTxMsduInfo(prAdapter, ucBssIndex,
			MSDU_REMOVE_BY_BSS_INDEX);
#endif

		cnmFreeWmmIndex(prAdapter, prBssInfo);
	}
	return u4Status;
}

void nicUpdateNetifTxThByBssId(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint32_t u4StopTh, uint32_t u4StartTh)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	prGlueInfo->u4TxStopTh[ucBssIndex] = u4StopTh;
	prGlueInfo->u4TxStartTh[ucBssIndex] = u4StartTh;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void nicMldUpdateNetifTxTh(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct BSS_INFO *prCurrBssInfo;
	uint32_t u4TxStopTh = 0, u4TxStartTh = 0;
	uint8_t ucBitmap11B = 0;
	uint8_t i;

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (!prMldBssInfo) {
		DBGLOG(NIC, WARN, "prMldBssInfo is NULL\n");
		return;
	}

	if (prMldBssInfo->ucBssBitmap == 0) {
		/* should not be empty, just for sanity */
		DBGLOG(NIC, WARN, "ucBssBitmap is 0\n");
		return;
	}

	prBssList = &prMldBssInfo->rBssList;
	/* for MLO, we should use the largest netif threshold */
	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList, rLinkEntryMld,
			struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		if (prCurrBssInfo->u4TxStopTh > u4TxStopTh)
			u4TxStopTh = prCurrBssInfo->u4TxStopTh;

		if (prCurrBssInfo->u4TxStartTh > u4TxStartTh)
			u4TxStartTh = prCurrBssInfo->u4TxStartTh;

		if (prCurrBssInfo->fgIs11B)
			ucBitmap11B |= BIT(prCurrBssInfo->ucBssIndex);
	}

	if (u4TxStartTh > u4TxStopTh) {
		DBGLOG(NIC, WARN,
			"Invalid threshold[%u:%u] for MLD[%u]\n",
			u4TxStartTh, u4TxStopTh,
			prMldBssInfo->ucGroupMldId);
		u4TxStartTh = u4TxStopTh / 2;
	}

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if ((prMldBssInfo->ucBssBitmap & BIT(i)) == 0)
			continue;

		nicUpdateNetifTxThByBssId(prAdapter, i,
			u4TxStopTh, u4TxStartTh);
	}

	/*
	 * turn on HIF adjust control only when all BSS in the same MLD is
	 * under 11B
	 */
	if (prMldBssInfo->ucBssBitmap == ucBitmap11B)
		prAdapter->ucAdjustCtrlBitmap |= ucBitmap11B;
	else {
		prAdapter->ucAdjustCtrlBitmap &=
			~(prMldBssInfo->ucBssBitmap);
	}
}
#else /* CFG_SUPPORT_802_11BE_MLO == 1 */
void nicUpdateNetifTxTh(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	nicUpdateNetifTxThByBssId(prAdapter, prBssInfo->ucBssIndex,
			prBssInfo->u4TxStopTh,
			prBssInfo->u4TxStartTh);

	if (prBssInfo->fgIs11B) {
		prAdapter->ucAdjustCtrlBitmap |=
			BIT(prBssInfo->ucBssIndex);
	} else {
		prAdapter->ucAdjustCtrlBitmap &=
			~BIT(prBssInfo->ucBssIndex);
	}
}
#endif /* CFG_SUPPORT_802_11BE_MLO == 1 */

void nicSetDefaultNetifTxTh(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if CFG_ADJUST_NETIF_TH_BY_BAND
	enum ENUM_BAND eBand;
	uint32_t u4TxStopTh, u4TxStartTh;
	uint8_t ucBw;

	eBand = prBssInfo->eBand;
	if (eBand >= BAND_NUM) {
		DBGLOG(NIC, ERROR, "Invalid eBand:%u\n");
		return;
	}

	if (prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
		return;

	u4TxStopTh = prWifiVar->au4NetifStopTh[eBand];
	u4TxStartTh = prWifiVar->au4NetifStartTh[eBand];

	/* Get DUT BW capability */
	ucBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
	if (prBssInfo->eBand == BAND_2G4 && ucBw > MAX_BW_20MHZ) {
		/* double it for 2.4G BW40 */
		u4TxStopTh <<= 1;
		u4TxStartTh <<= 1;
	}

	if (u4TxStartTh > u4TxStopTh) {
		DBGLOG(NIC, ERROR,
			"Invalid TxTh[%u:%u] for eBand:%u BssIndex:%u\n",
			u4TxStartTh, u4TxStopTh, eBand, prBssInfo->ucBssIndex);
		return;
	}

	prBssInfo->u4TxStopTh = u4TxStopTh;
	prBssInfo->u4TxStartTh = u4TxStartTh;
#else /* CFG_ADJUST_NETIF_TH_BY_BAND */
	prBssInfo->u4TxStopTh = prWifiVar->u4NetifStopTh;
	prBssInfo->u4TxStartTh = prWifiVar->u4NetifStartTh;
#endif /* CFG_ADJUST_NETIF_TH_BY_BAND */
}

void nicAdjustNetifTxTh(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED
		&& NIC_IS_BSS_BELOW_11AC(prBssInfo)) {
		if (NIC_IS_BSS_11B(prBssInfo))
			prBssInfo->fgIs11B = TRUE;
		else
			prBssInfo->fgIs11B = FALSE;

		prBssInfo->u4TxStopTh = NIC_BSS_LOW_RATE_TOKEN_CNT;
		prBssInfo->u4TxStartTh = prBssInfo->u4TxStopTh / 2;
	}
#ifdef CFG_SFD_DYNAMIC_ADJUST_NETIF_TH
	else if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED
		&& NIC_IS_BSS_11AC(prBssInfo)) {
		prBssInfo->u4TxStopTh = NIC_BSS_LOW_RATE_TOKEN_CNT;
		prBssInfo->u4TxStartTh = prBssInfo->u4TxStopTh / 2;
		prBssInfo->fgIs11B = FALSE;
	}
#endif
	else {
		nicSetDefaultNetifTxTh(prAdapter, prBssInfo);
		prBssInfo->fgIs11B = FALSE;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	nicMldUpdateNetifTxTh(prAdapter, prBssInfo);
#else
	nicUpdateNetifTxTh(prAdapter, prBssInfo);
#endif /* CFG_SUPPORT_802_11BE_MLO == 1 */
}

/* BSS-INFO */
uint32_t nicUpdateBss(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	return nicUpdateBssEx(prAdapter, ucBssIndex, TRUE);
}

void nicUpdateQos(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec)
{
	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucQoS))
		return;

	if (prStaRec->fgIsWmmSupported)
		nicQmUpdateWmmParms(prAdapter, prStaRec->ucBssIndex);

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		if (prStaRec->fgIsMuEdcaSupported ||
		    prAdapter->fgMuEdcaOverride) {
			nicQmUpdateMUEdcaParams(prAdapter,
						prStaRec->ucBssIndex);
		}
	}
#endif
}

#if CFG_SUPPORT_802_PP_DSCB
uint32_t nicUpdateDscb(struct ADAPTER *prAdapter,
			struct BSS_INFO *prBssInfo,
			uint8_t	 u1PreDscbPresent,
			uint16_t u2PreDscBitmap)
{
	if ((u1PreDscbPresent != prBssInfo->fgIsEhtDscbPresent)
			|| (u2PreDscBitmap != prBssInfo->u2EhtDisSubChanBitmap))
		return nicUpdateBss(prAdapter, prBssInfo->ucBssIndex);
	else
		return WLAN_STATUS_SUCCESS;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to sync bss info with firmware
 *        when a new BSS has been connected or disconnected
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicUpdateBssEx(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			uint8_t fgClearStaRec)
{
	uint32_t u4Status;
	struct BSS_INFO *prBssInfo;
	struct CMD_SET_BSS_INFO rCmdSetBssInfo;

	ASSERT(prAdapter);
	if (ucBssIndex > prAdapter->ucSwBssIdNum) {
		DBGLOG(BSS, ERROR, "BSS index %d is invalid\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

	nicAdjustNetifTxTh(prAdapter, prBssInfo);
	halUpdateBssTokenCnt(prAdapter, ucBssIndex);

	kalMemZero(&rCmdSetBssInfo,
		   sizeof(struct CMD_SET_BSS_INFO));

	rCmdSetBssInfo.ucBssIndex = ucBssIndex;
	rCmdSetBssInfo.ucConnectionState = (uint8_t)
					   prBssInfo->eConnectionState;
	rCmdSetBssInfo.ucCurrentOPMode = (uint8_t)
					 prBssInfo->eCurrentOPMode;
	rCmdSetBssInfo.ucSSIDLen = (uint8_t) prBssInfo->ucSSIDLen;
	kalMemCopy(rCmdSetBssInfo.aucSSID, prBssInfo->aucSSID,
		   prBssInfo->ucSSIDLen);
	COPY_MAC_ADDR(rCmdSetBssInfo.aucBSSID, prBssInfo->aucBSSID);
	rCmdSetBssInfo.ucIsQBSS = (uint8_t) prBssInfo->fgIsQBSS;
	rCmdSetBssInfo.ucNonHTBasicPhyType =
		prBssInfo->ucNonHTBasicPhyType;
	rCmdSetBssInfo.u2OperationalRateSet =
		prBssInfo->u2OperationalRateSet;
	rCmdSetBssInfo.u2BSSBasicRateSet =
		prBssInfo->u2BSSBasicRateSet;
	rCmdSetBssInfo.u2HwDefaultFixedRateCode =
		prBssInfo->u2HwDefaultFixedRateCode;
	rCmdSetBssInfo.ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
	rCmdSetBssInfo.u4PrivateData = prBssInfo->u4PrivateData;
#if	CFG_SUPPORT_DBDC
	/*
	 *To do: In fact, this is not used anymore and could be removed now.
	 *But command structure change has driver and firmware dependency.
	 *So currently using ENUM_BAND_AUTO is a temporary solution.
	 */
	rCmdSetBssInfo.ucDBDCBand = ENUM_BAND_AUTO;
#endif
	rCmdSetBssInfo.ucWmmSet = prBssInfo->ucWmmQueSet;
	rCmdSetBssInfo.ucNss = prBssInfo->ucOpTxNss; /* Backward compatible */

	if (prBssInfo->fgBcDefaultKeyExist) {
		if (prBssInfo->wepkeyWlanIdx <
		    prAdapter->ucTxDefaultWlanIndex)
			rCmdSetBssInfo.ucBMCWlanIndex =
				prBssInfo->wepkeyWlanIdx;
		else if (prBssInfo->ucBMCWlanIndexSUsed[
			prBssInfo->ucBcDefaultKeyIdx])
			rCmdSetBssInfo.ucBMCWlanIndex =
				prBssInfo->ucBMCWlanIndexS[
				prBssInfo->ucBcDefaultKeyIdx];
	} else
		rCmdSetBssInfo.ucBMCWlanIndex = prBssInfo->ucBMCWlanIndex;

	if ((prBssInfo->eConnectionState ==
		MEDIA_STATE_DISCONNECTED ||
		prBssInfo->eConnectionState ==
	    MEDIA_STATE_ROAMING_DISC_PREV) &&
	    secCheckWTBLwlanIdxInUseByOther(prAdapter,
		rCmdSetBssInfo.ucBMCWlanIndex, ucBssIndex)) {
		rCmdSetBssInfo.ucBMCWlanIndex =
			secPrivacySeekForBcEntry(
				prAdapter, prBssInfo->ucBssIndex,
				prBssInfo->aucOwnMacAddr,
				STA_REC_INDEX_NOT_FOUND,
				CIPHER_SUITE_NONE, 0xFF);
	}

	DBGLOG(RSN, TRACE, "Update BSS BMC WlanIdx %u\n",
	       rCmdSetBssInfo.ucBMCWlanIndex);

#if CFG_ENABLE_WIFI_DIRECT
	rCmdSetBssInfo.ucHiddenSsidMode =
		prBssInfo->eHiddenSsidType;
#endif
	rlmFillSyncCmdParam(&rCmdSetBssInfo.rBssRlmParam,
			    prBssInfo);

	rCmdSetBssInfo.ucWapiMode = (uint8_t) FALSE;

	if (IS_BSS_AIS(prBssInfo)) {
		struct CONNECTION_SETTINGS *prConnSettings =
			aisGetConnSettings(prAdapter, ucBssIndex);
#if CFG_SUPPORT_PASSPOINT
		/* mapping OSEN to WPA2,
		 * due to firmware no need to know current is OSEN
		 */
		if (prConnSettings->eAuthMode == AUTH_MODE_WPA_OSEN)
			rCmdSetBssInfo.ucAuthMode = AUTH_MODE_WPA2;
		else
#endif
		rCmdSetBssInfo.ucAuthMode = (uint8_t) prConnSettings->eAuthMode;
		rCmdSetBssInfo.ucEncStatus = (uint8_t)
					     prConnSettings->eEncStatus;
		rCmdSetBssInfo.ucWapiMode = (uint8_t)
					    prConnSettings->fgWapiMode;
	}
#if CFG_ENABLE_BT_OVER_WIFI
	else if (IS_BSS_BOW(prBssInfo)) {
		rCmdSetBssInfo.ucAuthMode = (uint8_t) AUTH_MODE_WPA2_PSK;
		rCmdSetBssInfo.ucEncStatus = (uint8_t)
					     ENUM_ENCRYPTION3_KEY_ABSENT;
	}
#endif
	else {
#if CFG_ENABLE_WIFI_DIRECT
		if (prAdapter->fgIsP2PRegistered) {
			if (kalP2PGetCcmpCipher(prAdapter->prGlueInfo,
				(uint8_t) prBssInfo->u4PrivateData)) {
				rCmdSetBssInfo.ucAuthMode =
				(uint8_t) AUTH_MODE_WPA2_PSK;
				rCmdSetBssInfo.ucEncStatus =
					(uint8_t)
					ENUM_ENCRYPTION3_ENABLED;
			} else if (kalP2PGetTkipCipher(prAdapter->prGlueInfo,
					(uint8_t) prBssInfo->u4PrivateData)) {
				rCmdSetBssInfo.ucAuthMode =
					(uint8_t) AUTH_MODE_WPA_PSK;
				rCmdSetBssInfo.ucEncStatus =
					(uint8_t) ENUM_ENCRYPTION2_ENABLED;
			} else if (kalP2PGetWepCipher(prAdapter->prGlueInfo,
					(uint8_t) prBssInfo->u4PrivateData)) {
				rCmdSetBssInfo.ucAuthMode =
					(uint8_t) AUTH_MODE_OPEN;
				rCmdSetBssInfo.ucEncStatus =
					(uint8_t) ENUM_ENCRYPTION1_ENABLED;
			} else {
				rCmdSetBssInfo.ucAuthMode =
					(uint8_t) AUTH_MODE_OPEN;
				rCmdSetBssInfo.ucEncStatus =
					(uint8_t) ENUM_ENCRYPTION_DISABLED;
			}
			/* Need the probe response to detect the PBC overlap */
			rCmdSetBssInfo.ucIsApMode =
				p2pFuncIsAPMode(
					prAdapter->rWifiVar.prP2PConnSettings[
					prBssInfo->u4PrivateData]);

		}
#else
		rCmdSetBssInfo.ucAuthMode = (uint8_t) AUTH_MODE_WPA2_PSK;
		rCmdSetBssInfo.ucEncStatus = (uint8_t)
					     ENUM_ENCRYPTION3_KEY_ABSENT;
#endif
	}
	rCmdSetBssInfo.ucDisconnectDetectThreshold = 0;

	if (IS_BSS_AIS(prBssInfo) &&
	    (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
	    (prBssInfo->prStaRecOfAP != NULL)) {
#if CFG_SUPPORT_IOT_AP_BLOCKLIST
		struct BSS_DESC *prBssDesc = aisGetTargetBssDesc(prAdapter,
								 ucBssIndex);

		if (prBssDesc != NULL && prBssDesc->ucIotVer == 1)
			rCmdSetBssInfo.u8IotApAct = prBssDesc->u8IotApAct;
		else if (prBssDesc != NULL && prBssDesc->ucIotVer == 0)
			rCmdSetBssInfo.ucIotApAct = prBssDesc->u8IotApAct;
#endif
		rCmdSetBssInfo.ucStaRecIdxOfAP =
			prBssInfo->prStaRecOfAP->ucIndex;
		cnmAisInfraConnectNotify(prAdapter);
#if CFG_SUPPORT_SMART_GEAR
		DBGLOG(SW4, INFO, "[SG]cnmAisInfraConnectNotify,%d\n",
		       prBssInfo->eConnectionState);
		if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
			uint8_t ucSGEnable = TRUE, ucDutNss = 0;
			struct STA_RECORD *prCurStaRec;

			ucDutNss = wlanGetSupportNss(prAdapter, ucBssIndex);
			DBGLOG(SW4, INFO, "[SG]SG Get Dut NSS %d\n", ucDutNss);
			if (rCmdSetBssInfo.ucIotApAct == WLAN_IOT_AP_DIS_SG) {
				DBGLOG(SW4, INFO,
					"[SG]Hit SG blocklist, disable SG\n");
				ucSGEnable = FALSE;
			}

			prCurStaRec = prBssInfo->prStaRecOfAP;

			/* Check peer Rx Nss Cap */
#if CFG_SUPPORT_802_11AC
			if (RLM_NET_IS_11AC(prBssInfo)) { /* VHT */
				if (ucDutNss == 2 &&
				((prCurStaRec->u2VhtRxMcsMap &
				VHT_CAP_INFO_MCS_2SS_MASK) >>
				VHT_CAP_INFO_MCS_2SS_OFFSET) ==
				VHT_CAP_INFO_MCS_NOT_SUPPORTED) {
					DBGLOG(RLM, INFO,
						"[SG] VHT peer doesn't support 2ss\n");
					ucSGEnable = FALSE;
				}
			} else
#endif
			if (RLM_NET_IS_11N(prBssInfo)) { /* HT */
				if (ucDutNss == 2 &&
				(prCurStaRec->aucRxMcsBitmask[1]
				== 0)) {
					DBGLOG(RLM, INFO,
						   "[SG] HT peer doesn't support 2ss\n");
					ucSGEnable = FALSE;
				}
			}
			/*Send Event  to Enable/Disable SG*/
			wlandioSetSGStatus(prAdapter,
			ucSGEnable, 0xFF, ucDutNss);
		}
#endif
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if ((prAdapter->fgIsP2PRegistered) &&
		 (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) &&
		 (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE)
		 && (prBssInfo->prStaRecOfAP != NULL)) {
		rCmdSetBssInfo.ucStaRecIdxOfAP =
			prBssInfo->prStaRecOfAP->ucIndex;
	}
#endif

#if CFG_ENABLE_BT_OVER_WIFI
/* disabled for BOW to finish ucBssIndex migration */
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW &&
		 prBssInfo->eCurrentOPMode == OP_MODE_BOW
		 && prBssInfo->prStaRecOfAP != NULL) {
		rCmdSetBssInfo.ucStaRecIdxOfAP =
			prBssInfo->prStaRecOfAP->ucIndex;
	}
#endif
	else
		rCmdSetBssInfo.ucStaRecIdxOfAP = STA_REC_INDEX_NOT_FOUND;

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		memcpy(rCmdSetBssInfo.ucHeOpParams, prBssInfo->ucHeOpParams,
				HE_OP_BYTE_NUM);
		rCmdSetBssInfo.ucBssColorInfo = prBssInfo->ucBssColorInfo;
		rCmdSetBssInfo.u2HeBasicMcsSet =
			prBssInfo->u2HeBasicMcsSet;
	}
#endif

#if (CFG_SUPPORT_802_11V_MBSSID == 1)
	rCmdSetBssInfo.ucMaxBSSIDIndicator = prBssInfo->ucMaxBSSIDIndicator;
	rCmdSetBssInfo.ucMBSSIDIndex = prBssInfo->ucMBSSIDIndex;
#endif

#define TEMP_LOG_TEMPLATE \
	"Update Bss[%u] OMAC[%u] WMM[%u] ConnState[%u] OPmode[%u] " \
	"BSSID[" MACSTR "] AuthMode[%u] EncStatus[%u] IotAct[%u:%u] " \
	"eBand[%u] Bw[%u] NetIfTh[%u:%u]\n"

	DBGLOG(BSS, INFO,
	       TEMP_LOG_TEMPLATE,
	       ucBssIndex,
	       prBssInfo->ucOwnMacIndex,
	       rCmdSetBssInfo.ucWmmSet,
	       prBssInfo->eConnectionState,
	       prBssInfo->eCurrentOPMode,
	       MAC2STR(prBssInfo->aucBSSID),
	       rCmdSetBssInfo.ucAuthMode,
	       rCmdSetBssInfo.ucEncStatus,
	       rCmdSetBssInfo.ucIotApAct,
	       rCmdSetBssInfo.u8IotApAct,
	       prBssInfo->eBand,
	       cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex),
	       prBssInfo->u4TxStopTh,
	       prBssInfo->u4TxStartTh);
#undef TEMP_LOG_TEMPLATE

	u4Status = wlanSendSetQueryCmd(prAdapter,
				       CMD_ID_SET_BSS_INFO,
				       TRUE,
				       FALSE,
				       FALSE,
				       NULL, NULL,
				       sizeof(struct CMD_SET_BSS_INFO),
				       (uint8_t *)&rCmdSetBssInfo, NULL, 0);

	/* if BSS-INFO is going to be disconnected state,
	 * free all correlated station records
	 */
	if (prBssInfo->eConnectionState ==
	    MEDIA_STATE_DISCONNECTED && fgClearStaRec) {

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
		prBssInfo->ucMLSRPausedLink = FALSE;
#endif

#if CFG_ENABLE_WIFI_DIRECT
		/* clear client list */
		bssInitializeClientList(prAdapter, prBssInfo);
#endif
#if DBG
		DBGLOG(BSS, TRACE, "nicUpdateBss for disconnect state\n");
#endif
		/* free all correlated station records */
		cnmStaFreeAllStaByNetwork(prAdapter, ucBssIndex,
					  STA_REC_EXCLUDE_NONE);
		if (HAL_IS_TX_DIRECT(prAdapter))
			nicTxDirectClearBssAbsentQ(prAdapter, ucBssIndex);
		else
			qmFreeAllByBssIdx(prAdapter, ucBssIndex);
#if CFG_SUPPORT_DBDC
		cnmDbdcRuntimeCheckDecision(prAdapter,
						ucBssIndex,
						FALSE);
#endif
	}

#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	cnmCtrlDynamicMaxQuota(prAdapter);
#endif /* CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1 */
#if (CFG_SUPPORT_DBDC == 1 && CFG_UPDATE_STATIC_DBDC_QUOTA == 1)
	if (prAdapter->rWifiVar.eDbdcMode ==
		ENUM_DBDC_MODE_STATIC)
		cnmUpdateStaticDbdcQuota(prAdapter);
#endif
	return u4Status;
}

/* BSS-INFO Indication (PM) */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate PM that
 *        a BSS has been created. (for AdHoc / P2P-GO)
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicPmIndicateBssCreated(struct ADAPTER
				 *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct CMD_INDICATE_PM_BSS_CREATED rCmdIndicatePmBssCreated = {0};

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	rCmdIndicatePmBssCreated.ucBssIndex = ucBssIndex;
	rCmdIndicatePmBssCreated.ucDtimPeriod =
		prBssInfo->ucDTIMPeriod;
	rCmdIndicatePmBssCreated.u2BeaconInterval =
		prBssInfo->u2BeaconInterval;
	rCmdIndicatePmBssCreated.u2AtimWindow =
		prBssInfo->u2ATIMWindow;

	return wlanSendSetQueryCmd(prAdapter,
	   CMD_ID_INDICATE_PM_BSS_CREATED,
	   TRUE,
	   FALSE,
	   FALSE,
	   NULL,
	   NULL,
	   sizeof(struct CMD_INDICATE_PM_BSS_CREATED),
	   (uint8_t *)&rCmdIndicatePmBssCreated, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate PM that
 *        a BSS has been connected
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eNetworkTypeIdx    Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicPmIndicateBssConnected(struct ADAPTER
				   *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct CMD_INDICATE_PM_BSS_CONNECTED rCmdIndicatePmBssConnected = {0};

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	rCmdIndicatePmBssConnected.ucBssIndex = ucBssIndex;
	rCmdIndicatePmBssConnected.ucDtimPeriod =
		prBssInfo->ucDTIMPeriod;
	rCmdIndicatePmBssConnected.u2AssocId = prBssInfo->u2AssocId;
	rCmdIndicatePmBssConnected.u2BeaconInterval =
		prBssInfo->u2BeaconInterval;
	rCmdIndicatePmBssConnected.u2AtimWindow =
		prBssInfo->u2ATIMWindow;

	rCmdIndicatePmBssConnected.ucBmpDeliveryAC =
		prBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC;
	rCmdIndicatePmBssConnected.ucBmpTriggerAC =
		prBssInfo->rPmProfSetupInfo.ucBmpTriggerAC;

	/* rCmdIndicatePmBssConnected.ucBmpDeliveryAC, */
	/* rCmdIndicatePmBssConnected.ucBmpTriggerAC); */

	if ((GET_BSS_INFO_BY_INDEX(prAdapter,
		ucBssIndex)->eNetworkType == NETWORK_TYPE_AIS)
#if CFG_ENABLE_WIFI_DIRECT
	    || ((GET_BSS_INFO_BY_INDEX(prAdapter,
				ucBssIndex)->eNetworkType == NETWORK_TYPE_P2P)
		&& (prAdapter->fgIsP2PRegistered))
#endif
	   ) {
		if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		    prBssInfo->prStaRecOfAP) {
			uint8_t ucUapsd = wmmCalculateUapsdSetting(prAdapter,
				ucBssIndex);

			/* should sync Tspec uapsd settings */
			rCmdIndicatePmBssConnected.ucBmpDeliveryAC =
				(ucUapsd >> 4) & 0xf;
			rCmdIndicatePmBssConnected.ucBmpTriggerAC =
				ucUapsd & 0xf;
			rCmdIndicatePmBssConnected.fgIsUapsdConnection =
				(uint8_t) prBssInfo->prStaRecOfAP->
				fgIsUapsdSupported;
		} else {
			rCmdIndicatePmBssConnected.fgIsUapsdConnection =
				0;	/* @FIXME */
		}
	} else {
		rCmdIndicatePmBssConnected.fgIsUapsdConnection = 0;
	}

	DBGLOG(INIT, INFO,
		"Bss%d dtim=%d,aid=%d,bcn_int=%d,atim=%d,bmp_d=%d,bmp_t=%d,uapsd=%d\n",
		rCmdIndicatePmBssConnected.ucBssIndex,
		rCmdIndicatePmBssConnected.ucDtimPeriod,
		rCmdIndicatePmBssConnected.u2AssocId,
		rCmdIndicatePmBssConnected.u2BeaconInterval,
		rCmdIndicatePmBssConnected.u2AtimWindow,
		rCmdIndicatePmBssConnected.ucBmpDeliveryAC,
		rCmdIndicatePmBssConnected.ucBmpTriggerAC,
		rCmdIndicatePmBssConnected.fgIsUapsdConnection);

	return wlanSendSetQueryCmd(prAdapter,
	   CMD_ID_INDICATE_PM_BSS_CONNECTED,
	   TRUE,
	   FALSE,
	   FALSE,
	   NULL,
	   NULL,
	   sizeof(struct CMD_INDICATE_PM_BSS_CONNECTED),
	   (uint8_t *)&rCmdIndicatePmBssConnected, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate PM that
 *        a BSS has been disconnected
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicPmIndicateBssAbort(struct ADAPTER *prAdapter,
			       uint8_t ucBssIndex)
{
	struct CMD_INDICATE_PM_BSS_ABORT rCmdIndicatePmBssAbort = {0};

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	rCmdIndicatePmBssAbort.ucBssIndex = ucBssIndex;

	DBGLOG(INIT, INFO, "Bss%d aborted\n",
		rCmdIndicatePmBssAbort.ucBssIndex);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_INDICATE_PM_BSS_ABORT,
				   TRUE,
				   FALSE,
				   FALSE,
				   NULL,
				   NULL,
				   sizeof(struct CMD_INDICATE_PM_BSS_ABORT),
				   (uint8_t *)&rCmdIndicatePmBssAbort, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to set power save bit map
 *
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *        ucSet              enter power save or not(1 PS, 0 not PS)
 *        ucCaller           index of bit map for caller
 * @retval -
 */
/*----------------------------------------------------------------------------*/
void
nicPowerSaveInfoMap(struct ADAPTER *prAdapter,
		    uint8_t ucBssIndex,
		    enum PARAM_POWER_MODE ePowerMode,
		    enum POWER_SAVE_CALLER ucCaller)
{
	uint32_t u4Flag;
	struct BSS_INFO *prBssInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return;
	}

	/* max caller is 24 */
	if (ucCaller >= PS_CALLER_MAX_NUM) {
		DBGLOG(INIT, ERROR, "ucCaller:%d not accepted\n", ucCaller);
		return;
	}

	u4Flag = prBssInfo->u4PowerSaveFlag;

	if (ucCaller == PS_CALLER_WOW) {
		/* For WOW,
		 * must switch to sleep mode for low power
		 */
		if (ePowerMode == Param_PowerModeCAM) {
			u4Flag &= ~BIT(ucCaller);
			/* WOW disable, if other callers request to CAM --> sync to FW */
			if ((u4Flag & PS_CALLER_ACTIVE) != 0)
				u4Flag |= PS_SYNC_WITH_FW;
		} else {
			/* WOW enable */
			u4Flag |= BIT(ucCaller);
			u4Flag |= PS_SYNC_WITH_FW;
		}
	} else {
		/* set send command flag */
		if (ePowerMode != Param_PowerModeCAM) {
			u4Flag &= ~BIT(ucCaller);
			if ((u4Flag & PS_CALLER_ACTIVE) == 0)
				u4Flag |= PS_SYNC_WITH_FW;
		} else {
			if ((u4Flag & PS_CALLER_ACTIVE) == 0)
				u4Flag |= PS_SYNC_WITH_FW;
			u4Flag |= BIT(ucCaller);
		}
	}

	DBGLOG(NIC, INFO,
		"Flag=0x%04x, Caller=%d, PM=%d, PSFlag[%d]=0x%04x\n",
		u4Flag, ucCaller, ePowerMode, ucBssIndex,
		prBssInfo->u4PowerSaveFlag);

	prBssInfo->u4PowerSaveFlag = u4Flag;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to set power save profile
 *
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *        ucSet              enter power save or not(1 PS, 0 not PS)
 *        fgEnCmdEvent       Enable the functions when command done and timeout
 *        ucCaller           index of bit map for caller
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_PENDING
 * @retval WLAN_STATUS_FAILURE
 * @retval WLAN_STATUS_NOT_SUPPORTED
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicConfigPowerSaveProfileEntry(struct ADAPTER *prAdapter,
			  uint8_t ucBssIndex,
			  enum PARAM_POWER_MODE ePwrMode,
			  u_int8_t fgEnCmdEvent,
			  enum POWER_SAVE_CALLER ucCaller)
{
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
#endif

	DBGLOG(INIT, INFO,
		"ucBssIndex:%d, ePwrMode:%d, fgEnCmdEvent:%d\n",
		ucBssIndex, ePwrMode, fgEnCmdEvent);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (prMldBssInfo) {
		struct BSS_INFO *bss;

		LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
			rLinkEntryMld, struct BSS_INFO) {
			if (bss->eNetworkType >= NETWORK_TYPE_NUM ||
			    bss->eNetworkType != prBssInfo->eNetworkType) {
				DBGLOG(INIT, WARN,
					   "Bss%d invalid eNetworkType: %d\n",
					   bss->ucBssIndex,
					   bss->eNetworkType);
			} else if (prAdapter->rWifiVar.ucPresetLinkId ==
							MLD_LINK_ID_NONE ||
			    prAdapter->rWifiVar.ucPresetLinkId ==
							bss->ucLinkIndex) {
				nicConfigPowerSaveProfile(prAdapter,
					bss->ucBssIndex,
					ePwrMode,
					fgEnCmdEvent,
					ucCaller);
			}
		}

		prAdapter->rWifiVar.ucPresetLinkId = MLD_LINK_ID_NONE;
	} else
#endif
	{
		nicConfigPowerSaveProfile(prAdapter, ucBssIndex,
			ePwrMode, fgEnCmdEvent, ucCaller);
	}

	return WLAN_STATUS_SUCCESS;
} /* end of nicConfigPowerSaveProfile */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to set power save profile
 *
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *        ucSet              enter power save or not(1 PS, 0 not PS)
 *        fgEnCmdEvent       Enable the functions when command done and timeout
 *        ucCaller           index of bit map for caller
 *
 * @retval WLAN_STATUS_SUCCESS
 * @retval WLAN_STATUS_PENDING
 * @retval WLAN_STATUS_FAILURE
 * @retval WLAN_STATUS_NOT_SUPPORTED
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicConfigPowerSaveProfile(struct ADAPTER *prAdapter,
			  uint8_t ucBssIndex,
			  enum PARAM_POWER_MODE ePwrMode,
			  u_int8_t fgEnCmdEvent,
			  enum POWER_SAVE_CALLER ucCaller)
{
	struct BSS_INFO *prBssInfo;

	DBGLOG(INIT, INFO,
		"ucBssIndex:%d, ePwrMode:%d, fgEnCmdEvent:%d\n",
		ucBssIndex, ePwrMode, fgEnCmdEvent);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	nicPowerSaveInfoMap(prAdapter, ucBssIndex, ePwrMode, ucCaller);
	prBssInfo->ePwrMode = ePwrMode;

#ifdef CFG_SUPPORT_TWT_EXT
	if (ucCaller == PS_CALLER_COMMON && ePwrMode == Param_PowerModeCAM) {
		if (IS_FEATURE_ENABLED(
			prAdapter->rWifiVar.ucTWTRequester))
			twtPlannerCheckTeardownSuspend(prAdapter,
				TRUE, TRUE);
	}
#endif

	if (PS_SYNC_WITH_FW & prBssInfo->u4PowerSaveFlag) {
		struct CMD_PS_PROFILE ps = {0};
		uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

		ps.ucBssIndex = ucBssIndex;
		ps.ucPsProfile = (uint8_t) ePwrMode;

		prBssInfo->u4PowerSaveFlag &= ~PS_SYNC_WITH_FW;

		DBGLOG(NIC, TRACE,
			"SYNC_WITH_FW u4PowerSaveFlag[%d]=0x%04x\n",
			ucBssIndex,
			prBssInfo->u4PowerSaveFlag);

		rWlanStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_POWER_SAVE_MODE,		/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			fgEnCmdEvent,	/* fgIsOid */

			/* pfCmdDoneHandler */
			(fgEnCmdEvent ? nicCmdEventSetCommon : NULL),

			/* pfCmdTimeoutHandler */
			(fgEnCmdEvent ? nicOidCmdTimeoutCommon : NULL),

			/* u4SetQueryInfoLen */
			sizeof(struct CMD_PS_PROFILE),

			/* pucInfoBuffer */
			(uint8_t *) &ps,

			/* pvSetQueryBuffer */
			NULL,

			/* u4SetQueryBufferLen */
			0
			);

		if (fgEnCmdEvent)
			return rWlanStatus;
	}
	return WLAN_STATUS_SUCCESS;
} /* end of nicConfigPowerSaveProfile */

uint32_t
nicConfigProcSetCamCfgWrite(struct ADAPTER *prAdapter,
	u_int8_t enabled, uint8_t ucBssIndex)
{
	enum PARAM_POWER_MODE ePowerMode;
	struct CMD_PS_PROFILE rPowerSaveMode = {0};
	struct BSS_INFO *prBssInfo;
	struct WLAN_INFO *prWlanInfo;

	if ((!prAdapter))
		return WLAN_STATUS_FAILURE;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	prWlanInfo = &prAdapter->rWlanInfo;

	rPowerSaveMode.ucBssIndex = ucBssIndex;

	if (enabled) {
		prWlanInfo->fgEnSpecPwrMgt = TRUE;
		ePowerMode = Param_PowerModeCAM;
		rPowerSaveMode.ucPsProfile = (uint8_t) ePowerMode;
		DBGLOG(INIT, INFO, "Enable CAM BssIndex:%d, PowerMode:%d\n",
		       ucBssIndex, rPowerSaveMode.ucPsProfile);
	} else {
		prWlanInfo->fgEnSpecPwrMgt = FALSE;
		rPowerSaveMode.ucPsProfile = (uint8_t) prBssInfo->ePwrMode;
		DBGLOG(INIT, INFO,
		       "Disable CAM BssIndex:%d, PowerMode:%d\n",
		       ucBssIndex, rPowerSaveMode.ucPsProfile);
	}

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_POWER_SAVE_MODE,
				   TRUE,
				   FALSE,
				   FALSE,
				   NULL,
				   NULL,
				   sizeof(struct CMD_PS_PROFILE),
				   (uint8_t *) &rPowerSaveMode,
				   NULL, 0);
}

uint32_t nicEnterCtiaMode(struct ADAPTER *prAdapter,
			  u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	struct CMD_SW_DBG_CTRL rCmdSwCtrl;
	/* CMD_ACCESS_REG rCmdAccessReg; */
	uint32_t rWlanStatus;
	uint8_t ucAisIdx;
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	uint32_t u4Level = 0;
#endif
	u_int8_t fgEnCmdEvtSetting = 0;

	DBGLOG(INIT, TRACE, "nicEnterCtiaMode: %d\n", fgEnterCtia);

	ASSERT(prAdapter);

	rWlanStatus = WLAN_STATUS_SUCCESS;
	kalMemZero(&rCmdSwCtrl, sizeof(struct CMD_SW_DBG_CTRL));

	if (fgEnterCtia) {
		/* 1. Disable On-Lin Scan */
		prAdapter->fgEnOnlineScan = FALSE;

		/* 2. Disable FIFO FULL no ack */
		/* 3. Disable Roaming */
		/* 4. Disalbe auto tx power */
		rCmdSwCtrl.u4Id = 0xa0100003;
		rCmdSwCtrl.u4Data = 0x0;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE, NULL, NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *)&rCmdSwCtrl, NULL, 0);

		/* 2. Keep at CAM mode */
		for (ucAisIdx = 0; ucAisIdx < KAL_AIS_NUM; ucAisIdx++) {
			enum PARAM_POWER_MODE ePowerMode;

			prAdapter->u4CtiaPowerMode = 0;
			prAdapter->fgEnCtiaPowerMode = TRUE;

			ePowerMode = Param_PowerModeCAM;
			fgEnCmdEvtSetting =
				(ucAisIdx + 1 == KAL_AIS_NUM)
				? fgEnCmdEvent : FALSE;
			rWlanStatus = nicConfigPowerSaveProfile(
				prAdapter,
				AIS_MAIN_BSS_INDEX(prAdapter, ucAisIdx),
				ePowerMode,
				fgEnCmdEvtSetting,
				PS_CALLER_CTIA);
		}

		/* 5. Disable Beacon Timeout Detection */
		prAdapter->fgDisBcnLostDetection = TRUE;

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
		/* 6. Disable Connsys Power Throttling feature. */
		conn_pwr_register_event_cb(CONN_PWR_DRV_WIFI, NULL);
		u4Level = CONN_PWR_THR_LV_0;
		connsys_power_event_notification(CONN_PWR_EVENT_LEVEL,
							&u4Level);
#endif

#if (CFG_SUPPORT_TWT == 1)
		/* 7. Disable TWT under CTIA mode */
		prAdapter->rWifiVar.ucTWTRequester = 0;
#endif
	} else {
		/* 1. Enaable On-Lin Scan */
		prAdapter->fgEnOnlineScan = TRUE;

		/* 2. Enable FIFO FULL no ack */
		/* 3. Enable Roaming */
		/* 4. Enable auto tx power */
		/*  */

		rCmdSwCtrl.u4Id = 0xa0100003;
		rCmdSwCtrl.u4Data = 0x1;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE, NULL, NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *)&rCmdSwCtrl, NULL, 0);

		/* 2. Keep at Fast PS */
		for (ucAisIdx = 0; ucAisIdx < KAL_AIS_NUM; ucAisIdx++) {
			enum PARAM_POWER_MODE ePowerMode;

			prAdapter->u4CtiaPowerMode = 2;
			prAdapter->fgEnCtiaPowerMode = TRUE;

			ePowerMode = Param_PowerModeFast_PSP;
			fgEnCmdEvtSetting =
				(ucAisIdx + 1 == KAL_AIS_NUM)
				? fgEnCmdEvent : FALSE;
			rWlanStatus = nicConfigPowerSaveProfile(
				prAdapter,
				AIS_MAIN_BSS_INDEX(prAdapter, ucAisIdx),
				ePowerMode,
				fgEnCmdEvtSetting,
				PS_CALLER_CTIA);
		}

		/* 5. Enable Beacon Timeout Detection */
		prAdapter->fgDisBcnLostDetection = FALSE;

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
		/* 6. Enable Connsys Power Throttling feature. */
		conn_pwr_register_event_cb(CONN_PWR_DRV_WIFI,
			(CONN_PWR_EVENT_CB)connsys_power_event_notification);
		conn_pwr_drv_pre_on(CONN_PWR_DRV_WIFI,
						&u4Level);
		prAdapter->u4PwrLevel = u4Level;
		connsys_power_event_notification(CONN_PWR_EVENT_LEVEL,
						&(prAdapter->u4PwrLevel));
#endif

#if (CFG_SUPPORT_TWT == 1)
		/* 7. Enable TWT support after CTIA mode */
		prAdapter->rWifiVar.ucTWTRequester = 1;
#endif
	}

	return rWlanStatus;
}				/* end of nicEnterCtiaMode() */

uint32_t nicEnterCtiaModeOfScan(struct ADAPTER
	*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	uint32_t rWlanStatus;

	ASSERT(prAdapter);
	DBGLOG(INIT, INFO, "nicEnterCtiaModeOfScan: %d\n",
	       fgEnterCtia);

	rWlanStatus = WLAN_STATUS_SUCCESS;

	if (fgEnterCtia) {
		/* Disable On-Line Scan */
		prAdapter->fgEnOnlineScan = FALSE;
	} else {
		/* Enable On-Line Scan */
		prAdapter->fgEnOnlineScan = TRUE;
	}

	return rWlanStatus;
}

uint32_t nicEnterCtiaModeOfRoaming(struct ADAPTER
	*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	struct CMD_SW_DBG_CTRL rCmdSwCtrl;
	uint32_t rWlanStatus;

	ASSERT(prAdapter);
	DBGLOG(INIT, INFO, "nicEnterCtiaModeOfRoaming: %d\n",
	       fgEnterCtia);

	rWlanStatus = WLAN_STATUS_SUCCESS;
	kalMemZero(&rCmdSwCtrl, sizeof(struct CMD_SW_DBG_CTRL));

	if (fgEnterCtia) {
		/* Disable Roaming */
		rCmdSwCtrl.u4Id = 0x55660000;
		rCmdSwCtrl.u4Data = 0x0;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE, NULL, NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *) &rCmdSwCtrl, NULL, 0);
	} else {
		/* Enable Roaming */
		rCmdSwCtrl.u4Id = 0x55660000;
		rCmdSwCtrl.u4Data = 0x1;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE, NULL, NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *) &rCmdSwCtrl, NULL, 0);
	}

	return rWlanStatus;
}

uint32_t nicEnterCtiaModeOfCAM(struct ADAPTER *prAdapter,
			       u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	uint32_t rWlanStatus;
	uint8_t ucAisIdx;

	ASSERT(prAdapter);
	DBGLOG(INIT, INFO, "nicEnterCtiaModeOfCAM: %d\n",
	       fgEnterCtia);

	rWlanStatus = WLAN_STATUS_SUCCESS;

	if (fgEnterCtia) {
		/* Keep at CAM mode */
		for (ucAisIdx = 0; ucAisIdx < KAL_AIS_NUM; ucAisIdx++) {
			enum PARAM_POWER_MODE ePowerMode;

			prAdapter->u4CtiaPowerMode = 0;
			prAdapter->fgEnCtiaPowerMode = TRUE;

			ePowerMode = Param_PowerModeCAM;
			rWlanStatus = nicConfigPowerSaveProfile(prAdapter,
				AIS_MAIN_BSS_INDEX(prAdapter, ucAisIdx),
				ePowerMode, fgEnCmdEvent, PS_CALLER_CTIA_CAM);
		}
	} else {
		/* Keep at Fast PS */
		for (ucAisIdx = 0; ucAisIdx < KAL_AIS_NUM; ucAisIdx++) {
			enum PARAM_POWER_MODE ePowerMode;

			prAdapter->u4CtiaPowerMode = 2;
			prAdapter->fgEnCtiaPowerMode = TRUE;

			ePowerMode = Param_PowerModeFast_PSP;
			rWlanStatus = nicConfigPowerSaveProfile(prAdapter,
				AIS_MAIN_BSS_INDEX(prAdapter, ucAisIdx),
				ePowerMode, fgEnCmdEvent, PS_CALLER_CTIA_CAM);
		}
	}

	return rWlanStatus;
}

uint32_t nicEnterCtiaModeOfBCNTimeout(struct ADAPTER
	*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	uint32_t rWlanStatus;

	ASSERT(prAdapter);
	DBGLOG(INIT, INFO, "nicEnterCtiaModeOfBCNTimeout: %d\n",
	       fgEnterCtia);

	rWlanStatus = WLAN_STATUS_SUCCESS;

	if (fgEnterCtia) {
		/* Disable Beacon Timeout Detection */
		prAdapter->fgDisBcnLostDetection = TRUE;
	} else {
		/* Enable Beacon Timeout Detection */
		prAdapter->fgDisBcnLostDetection = FALSE;
	}

	return rWlanStatus;
}

uint32_t nicEnterCtiaModeOfAutoTxPower(struct ADAPTER
	*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	struct CMD_SW_DBG_CTRL rCmdSwCtrl = {0};
	uint32_t rWlanStatus;

	ASSERT(prAdapter);
	DBGLOG(INIT, INFO, "nicEnterCtiaModeOfAutoTxPower: %d\n",
	       fgEnterCtia);

	rWlanStatus = WLAN_STATUS_SUCCESS;

	if (fgEnterCtia) {
		/* Disalbe auto tx power */
		rCmdSwCtrl.u4Id = 0x55670003;
		rCmdSwCtrl.u4Data = 0x0;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *) &rCmdSwCtrl,
			NULL, 0);
	} else {
		/* Enable auto tx power */
		rCmdSwCtrl.u4Id = 0x55670003;
		rCmdSwCtrl.u4Data = 0x1;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *) &rCmdSwCtrl,
			NULL, 0);
	}

	return rWlanStatus;
}

uint32_t nicEnterCtiaModeOfFIFOFullNoAck(struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent)
{
	struct CMD_SW_DBG_CTRL rCmdSwCtrl = {0};
	uint32_t rWlanStatus;

	ASSERT(prAdapter);
	DBGLOG(INIT, INFO, "nicEnterCtiaModeOfFIFOFullNoAck: %d\n",
	       fgEnterCtia);

	rWlanStatus = WLAN_STATUS_SUCCESS;

	if (fgEnterCtia) {
		/* Disable FIFO FULL no ack */
		rCmdSwCtrl.u4Id = 0x55680000;
		rCmdSwCtrl.u4Data = 0x0;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *) &rCmdSwCtrl,
			NULL, 0);
	} else {
		/* Enable FIFO FULL no ack */
		rCmdSwCtrl.u4Id = 0x55680000;
		rCmdSwCtrl.u4Data = 0x1;
		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SW_DBG_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct CMD_SW_DBG_CTRL),
			(uint8_t *) &rCmdSwCtrl,
			NULL, 0);
	}

	return rWlanStatus;
}

uint32_t nicEnterTPTestMode(struct ADAPTER *prAdapter,
			    uint8_t ucFuncMask)
{
	struct CMD_SW_DBG_CTRL rCmdSwCtrl = {0};
	uint32_t rWlanStatus;
	uint8_t ucBssIdx;
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);

	rWlanStatus = WLAN_STATUS_SUCCESS;

	if (ucFuncMask) {
		/* 1. Disable On-Lin Scan */
		if (ucFuncMask & TEST_MODE_DISABLE_ONLINE_SCAN)
			prAdapter->fgEnOnlineScan = FALSE;

		/* 2. Disable Roaming */
		if (ucFuncMask & TEST_MODE_DISABLE_ROAMING) {
			rCmdSwCtrl.u4Id = 0xa0210000;
			rCmdSwCtrl.u4Data = 0x0;
			wlanSendSetQueryCmd(prAdapter, CMD_ID_SW_DBG_CTRL, TRUE,
				FALSE, FALSE,
				NULL, NULL, sizeof(struct CMD_SW_DBG_CTRL),
				(uint8_t *)&rCmdSwCtrl, NULL, 0);
		}
		/* 3. Keep at CAM mode */
		if (ucFuncMask & TEST_MODE_FIXED_CAM_MODE)
			for (ucBssIdx = 0; ucBssIdx < prAdapter->ucSwBssIdNum;
			     ucBssIdx++) {
				prBssInfo =
					GET_BSS_INFO_BY_INDEX(prAdapter,
						ucBssIdx);
				if (prBssInfo && prBssInfo->fgIsInUse
				    && (prBssInfo->eCurrentOPMode
				    == OP_MODE_INFRASTRUCTURE))
					nicConfigPowerSaveProfile(prAdapter,
						ucBssIdx, Param_PowerModeCAM,
						FALSE, PS_CALLER_TP);
			}

		/* 4. Disable Beacon Timeout Detection */
		if (ucFuncMask & TEST_MODE_DISABLE_BCN_LOST_DET)
			prAdapter->fgDisBcnLostDetection = TRUE;
	} else {
		/* 1. Enaable On-Lin Scan */
		prAdapter->fgEnOnlineScan = TRUE;

		/* 2. Enable Roaming */
		rCmdSwCtrl.u4Id = 0xa0210000;
		rCmdSwCtrl.u4Data = 0x1;
		wlanSendSetQueryCmd(prAdapter, CMD_ID_SW_DBG_CTRL, TRUE,
				    FALSE, FALSE,
				    NULL, NULL, sizeof(struct CMD_SW_DBG_CTRL),
				    (uint8_t *)&rCmdSwCtrl, NULL, 0);

		/* 3. Keep at Fast PS */
		for (ucBssIdx = 0; ucBssIdx < prAdapter->ucSwBssIdNum;
		     ucBssIdx++) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
			if (prBssInfo && prBssInfo->fgIsInUse
			    && (prBssInfo->eCurrentOPMode
						== OP_MODE_INFRASTRUCTURE))
				nicConfigPowerSaveProfile(prAdapter, ucBssIdx,
					Param_PowerModeFast_PSP,
					FALSE, PS_CALLER_TP);
		}

		/* 4. Enable Beacon Timeout Detection */
		prAdapter->fgDisBcnLostDetection = FALSE;
	}

	return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to indicate firmware domain
 *        for beacon generation parameters
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        eIeUpdMethod,      Update Method
 *        ucBssIndex         Index of BSS-INFO
 *        u2Capability       Capability
 *        aucIe              Pointer to buffer of IEs
 *        u2IELen            Length of IEs
 *
 * @retval - WLAN_STATUS_SUCCESS
 *           WLAN_STATUS_FAILURE
 *           WLAN_STATUS_PENDING
 *           WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicUpdateBeaconIETemplate(struct ADAPTER *prAdapter,
			  enum ENUM_IE_UPD_METHOD eIeUpdMethod,
			  uint8_t ucBssIndex, uint16_t u2Capability,
			  uint8_t *aucIe, uint16_t u2IELen)
{
	struct CMD_BEACON_TEMPLATE_UPDATE rCmdBcnUpdate;
	uint16_t u2CmdBufLen = 0;
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;

	if (u2IELen > MAX_IE_LENGTH)
		return WLAN_STATUS_INVALID_DATA;

	if (eIeUpdMethod == IE_UPD_METHOD_UPDATE_RANDOM
		|| eIeUpdMethod == IE_UPD_METHOD_UPDATE_ALL
#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
		|| eIeUpdMethod == IE_UPD_METHOD_UPDATE_PROBE_RSP
#endif
		|| eIeUpdMethod == IE_UPD_METHOD_UNSOL_PROBE_RSP) {
		u2CmdBufLen = OFFSET_OF(struct CMD_BEACON_TEMPLATE_UPDATE,
					aucIE) + u2IELen;
	} else if (eIeUpdMethod == IE_UPD_METHOD_DELETE_ALL) {
		u2CmdBufLen = OFFSET_OF(struct CMD_BEACON_TEMPLATE_UPDATE,
					u2IELen);
	} else {
		DBGLOG(INIT, ERROR, "Unknown IeUpdMethod.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* fill beacon updating command */
	rCmdBcnUpdate.ucUpdateMethod = (uint8_t) eIeUpdMethod;
	rCmdBcnUpdate.ucBssIndex = ucBssIndex;
	rCmdBcnUpdate.u2Capability = u2Capability;
	rCmdBcnUpdate.u2IELen = u2IELen;
	if (u2IELen > 0)
		kalMemCopy(rCmdBcnUpdate.aucIE, aucIe, u2IELen);

	return wlanSendSetQueryCmd(prAdapter,
		CMD_ID_UPDATE_BEACON_CONTENT,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		u2CmdBufLen,
		(uint8_t *)&rCmdBcnUpdate,
		NULL,
		0);
}

uint32_t
nicUpdateFilsDiscIETemplate(struct ADAPTER *prAdapter,
			    uint8_t ucBssIndex,
			    uint32_t u4MaxInterval,
			    uint32_t u4MinInterval,
			    uint8_t *aucIe,
			    uint16_t u2IELen)
{
	if (!aucIe || !u2IELen)
		return WLAN_STATUS_INVALID_DATA;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return nicUniCmdFilsDiscovery(prAdapter, ucBssIndex, u4MaxInterval,
		u4MinInterval, aucIe, u2IELen);
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to initialization PHY related
 *        varaibles
 *
 * @param prAdapter  Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicSetAvailablePhyTypeSet(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return;

#if (CFG_SUPPORT_802_11BE == 1)
	prAdapter->rWifiVar.eDesiredPhyConfig
		= PHY_CONFIG_802_11ABGNACAXBE;
#elif (CFG_SUPPORT_802_11AX == 1)
	prAdapter->rWifiVar.eDesiredPhyConfig
		= PHY_CONFIG_802_11ABGNACAX;
#elif CFG_SUPPORT_802_11AC
	prAdapter->rWifiVar.eDesiredPhyConfig
		= PHY_CONFIG_802_11ABGNAC;
#else
	prAdapter->rWifiVar.eDesiredPhyConfig
		= PHY_CONFIG_802_11ABGN;
#endif

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 0)
		prAdapter->rWifiVar.eDesiredPhyConfig = PHY_CONFIG_802_11ABGNAC;
#endif

	if (prAdapter->rWifiVar.ucHwNotSupportAC)
		prAdapter->rWifiVar.eDesiredPhyConfig = PHY_CONFIG_802_11ABGN;

	/* Set default bandwidth modes */
	prAdapter->rWifiVar.uc2G4BandwidthMode =
		(prAdapter->rWifiVar.ucSta2gBandwidth == MAX_BW_40MHZ)
		? CONFIG_BW_20_40M
		: CONFIG_BW_20M;
	prAdapter->rWifiVar.uc5GBandwidthMode = CONFIG_BW_20_40M;
	prAdapter->rWifiVar.uc6GBandwidthMode = CONFIG_BW_20_40_80M;

	if (prAdapter->rWifiVar.eDesiredPhyConfig >= PHY_CONFIG_NUM)
		return;

	prAdapter->rWifiVar.ucAvailablePhyTypeSet =
		aucPhyCfg2PhyTypeSet[prAdapter->rWifiVar.eDesiredPhyConfig];

	DBGLOG(P2P, TRACE, "Avail Phy type: 0x%x, %d\n",
		prAdapter->rWifiVar.ucAvailablePhyTypeSet,
		prAdapter->rWifiVar.eDesiredPhyConfig);

	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
	    PHY_TYPE_BIT_ERP)
		prAdapter->rWifiVar.eNonHTBasicPhyType2G4 =
			PHY_TYPE_ERP_INDEX;
	/* NOTE(Kevin): Because we don't have N only mode, TBD */
	else
		prAdapter->rWifiVar.eNonHTBasicPhyType2G4 =
			PHY_TYPE_HR_DSSS_INDEX;

}

static u_int8_t nicIsWmmPriorityInverse(struct AC_QUE_PARMS arACQueParms[])
{
	enum ENUM_WMM_ACI eLowPrioAc;
	enum ENUM_WMM_ACI eHighPrioAc;

	for (eLowPrioAc = WMM_AC_BE_INDEX;
	     eLowPrioAc < WMM_AC_VI_INDEX; eLowPrioAc++) {
		for (eHighPrioAc = WMM_AC_VI_INDEX;
		     eHighPrioAc < WMM_AC_INDEX_NUM; eHighPrioAc++) {
			if (arACQueParms[eLowPrioAc].u2Aifsn <=
			    arACQueParms[eHighPrioAc].u2Aifsn)
				return TRUE;

			if (arACQueParms[eLowPrioAc].u2CWmin <=
			    arACQueParms[eHighPrioAc].u2CWmin)
				return TRUE;
		}
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        ucBssIndex         Index of BSS-INFO
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t nicQmUpdateWmmParms(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct CMD_UPDATE_WMM_PARMS rCmdUpdateWmmParms = {0};
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4TxHifRes = 0, u4Idx = 0;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	DBGLOG(QM, TRACE, "Update WMM parameters for BSS[%u]\n",
	       ucBssIndex);

	DBGLOG(QM, TRACE, "sizeof(struct AC_QUE_PARMS): %zu\n",
	       sizeof(struct AC_QUE_PARMS));
	DBGLOG(QM, TRACE, "sizeof(CMD_UPDATE_WMM_PARMS): %zu\n",
	       sizeof(struct CMD_UPDATE_WMM_PARMS));
	DBGLOG(QM, TRACE, "u2CmdTxHdrSize: %u\n",
	       prChipInfo->u2CmdTxHdrSize);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	rCmdUpdateWmmParms.ucBssIndex = (uint8_t) ucBssIndex;
	kalMemCopy(&rCmdUpdateWmmParms.arACQueParms[0],
		   &prBssInfo->arACQueParms[0],
		   sizeof(struct AC_QUE_PARMS) * WMM_AC_INDEX_NUM);

	rCmdUpdateWmmParms.fgIsQBSS = prBssInfo->fgIsQBSS;
	rCmdUpdateWmmParms.ucWmmSet = (uint8_t) prBssInfo->ucWmmQueSet;

	DBGLOG(QM, INFO,
	       "WMM[%d], [AC/Aifsn/CWmin]: [0/%u/%u] [1/%u/%u] [2/%u/%u]\n",
		rCmdUpdateWmmParms.ucWmmSet,
		rCmdUpdateWmmParms.arACQueParms[AC0].u2Aifsn,
		rCmdUpdateWmmParms.arACQueParms[AC0].u2CWmin,
		rCmdUpdateWmmParms.arACQueParms[AC1].u2Aifsn,
		rCmdUpdateWmmParms.arACQueParms[AC1].u2CWmin,
		rCmdUpdateWmmParms.arACQueParms[AC2].u2Aifsn,
		rCmdUpdateWmmParms.arACQueParms[AC2].u2CWmin);

	if (nicIsWmmPriorityInverse(rCmdUpdateWmmParms.arACQueParms)) {
		/* Use round-robbin queuing in HIF to enqueue data to HW
		 * Should revise if HIF can have separate queue for each AC
		 */
		prAdapter->rWifiVar.ucTxMsduQueue = 1;

		/* The ratio of each AC is 1:1:1:1 in this case */
		u4TxHifRes = 0x00111111;
	} else {
		/* Use default setting when wifi init */
		prAdapter->rWifiVar.ucTxMsduQueue =
			prAdapter->rWifiVar.ucTxMsduQueueInit;
		u4TxHifRes = prAdapter->rWifiVar.u4TxHifRes;
	}

	DBGLOG_LIMITED(QM, INFO, "ucTxMsduQueue:[%u], u4TxHifRes[0x%08x]",
		prAdapter->rWifiVar.ucTxMsduQueue, u4TxHifRes);

	for (u4Idx = 0; u4Idx < TC_NUM && u4TxHifRes; u4Idx++) {
		prAdapter->au4TxHifResCtl[u4Idx] = u4TxHifRes & BITS(0, 3);
		u4TxHifRes = u4TxHifRes >> 4;
	}

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_UPDATE_WMM_PARMS,
				   TRUE,
				   FALSE,
				   FALSE,
				   NULL, NULL,
				   sizeof(struct CMD_UPDATE_WMM_PARMS),
				   (uint8_t *)&rCmdUpdateWmmParms, NULL, 0);
}

#if (CFG_SUPPORT_802_11AX == 1)
uint32_t nicQmUpdateMUEdcaParams(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct _CMD_MQM_UPDATE_MU_EDCA_PARMS_T rCmdUpdateMUEdcaParms = {0};

	ASSERT(prAdapter);

	DBGLOG(QM, INFO, "Update MU EDCA parameters for BSS[%u]\n", ucBssIndex);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	rCmdUpdateMUEdcaParms.ucBssIndex = (uint8_t) ucBssIndex;

	if (prAdapter->fgMuEdcaOverride) {

		enum ENUM_WMM_ACI eAci;
		struct _CMD_MU_EDCA_PARAMS_T *prMUEdca;

		for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {

			prMUEdca =
				&(rCmdUpdateMUEdcaParms.arMUEdcaParams[eAci]);

			prMUEdca->ucECWmin = 15;
			prMUEdca->ucECWmax = 15;
			prMUEdca->ucAifsn = 0;
			prMUEdca->ucIsACMSet = 0;
			prMUEdca->ucMUEdcaTimer = 0xff;
		}
	} else {
		kalMemCopy(&rCmdUpdateMUEdcaParms.arMUEdcaParams[0],
			&prBssInfo->arMUEdcaParams[0],
			sizeof(struct _CMD_MU_EDCA_PARAMS_T) *
				WMM_AC_INDEX_NUM);
	}

	rCmdUpdateMUEdcaParms.fgIsQBSS = prBssInfo->fgIsQBSS;
	rCmdUpdateMUEdcaParms.ucWmmSet = (uint8_t) prBssInfo->ucWmmQueSet;

	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_MQM_UPDATE_MU_EDCA_PARMS,
				TRUE,
				FALSE,
				FALSE,
				NULL, NULL,
				sizeof(struct _CMD_MQM_UPDATE_MU_EDCA_PARMS_T),
				(uint8_t *)&rCmdUpdateMUEdcaParms, NULL, 0);
}

uint32_t nicRlmUpdateSRParams(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	struct _CMD_RLM_UPDATE_SR_PARMS_T rCmdUpdateSRParms = {0};

	ASSERT(prAdapter);

	DBGLOG(RLM, INFO, "Update Spatial Reuse parameters for BSS[%u]\n",
		ucBssIndex);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}
	rCmdUpdateSRParms.ucBssIndex = ucBssIndex;
	rCmdUpdateSRParms.ucSRControl = prBssInfo->ucSRControl;
	rCmdUpdateSRParms.ucNonSRGObssPdMaxOffset =
		prBssInfo->ucNonSRGObssPdMaxOffset;
	rCmdUpdateSRParms.ucSRGObssPdMinOffset =
		prBssInfo->ucSRGObssPdMinOffset;
	rCmdUpdateSRParms.ucSRGObssPdMaxOffset =
		prBssInfo->ucSRGObssPdMaxOffset;
	rCmdUpdateSRParms.u4SRGBSSColorBitmapLow = CPU_TO_LE32(
		(uint32_t)(prBssInfo->u8SRGBSSColorBitmap & 0xFFFFFFFF));
	rCmdUpdateSRParms.u4SRGBSSColorBitmapHigh = CPU_TO_LE32(
		(uint32_t)(prBssInfo->u8SRGBSSColorBitmap >> 32));
	rCmdUpdateSRParms.u4SRGPartialBSSIDBitmapLow = CPU_TO_LE32(
		(uint32_t)(prBssInfo->u8SRGPartialBSSIDBitmap & 0xFFFFFFFF));
	rCmdUpdateSRParms.u4SRGPartialBSSIDBitmapHigh = CPU_TO_LE32(
		(uint32_t)(prBssInfo->u8SRGPartialBSSIDBitmap >> 32));

	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_RLM_UPDATE_SR_PARAMS,
				TRUE,
				FALSE,
				FALSE,
				NULL, NULL,
				sizeof(struct _CMD_RLM_UPDATE_SR_PARMS_T),
				(uint8_t *)&rCmdUpdateSRParms, NULL, 0);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update TX power gain corresponding to
 *        each band/modulation combination
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prTxPwrParam       Pointer of TX power parameters
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t nicUpdateTxPower(struct ADAPTER *prAdapter,
			  struct CMD_TX_PWR *prTxPwrParam)
{
	ASSERT(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_TX_PWR,
				   TRUE,
				   FALSE, FALSE, NULL, NULL,
				   sizeof(struct CMD_TX_PWR),
				   (uint8_t *) prTxPwrParam, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to set auto tx power parameter
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prTxPwrParam       Pointer of Auto TX power parameters
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t nicSetAutoTxPower(struct ADAPTER *prAdapter,
			   struct CMD_AUTO_POWER_PARAM *prAutoPwrParam)
{
	ASSERT(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_AUTOPWR_CTRL,
				   TRUE,
				   FALSE,
				   FALSE,
				   NULL, NULL,
				   sizeof(struct CMD_AUTO_POWER_PARAM),
				   (uint8_t *) prAutoPwrParam, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update TX power gain corresponding to
 *        each band/modulation combination
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prTxPwrParam       Pointer of TX power parameters
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t nicSetAutoTxPowerControl(struct ADAPTER
	*prAdapter, struct CMD_TX_PWR *prTxPwrParam)
{
	ASSERT(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_TX_PWR,
				   TRUE,
				   FALSE, FALSE, NULL, NULL,
				   sizeof(struct CMD_TX_PWR),
				   (uint8_t *) prTxPwrParam, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update power offset around 5GHz band
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        pr5GPwrOffset      Pointer of 5GHz power offset parameter
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t nicUpdate5GOffset(struct ADAPTER *prAdapter,
			   struct CMD_5G_PWR_OFFSET *pr5GPwrOffset)
{
#if 0				/* It is not needed anymore */
	ASSERT(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_5G_PWR_OFFSET,
				   TRUE,
				   FALSE,
				   FALSE, NULL, NULL,
				   sizeof(struct CMD_5G_PWR_OFFSET),
				   (uint8_t *) pr5GPwrOffset, NULL, 0);
#else
	return 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update DPD calibration result
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        pr5GPwrOffset      Pointer of parameter for DPD calibration result
 *
 * @retval WLAN_STATUS_PENDING
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t nicUpdateDPD(struct ADAPTER *prAdapter,
		      struct CMD_PWR_PARAM *prDpdCalResult)
{
	ASSERT(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_PWR_PARAM,
				   TRUE,
				   FALSE,
				   FALSE, NULL, NULL,
				   sizeof(struct CMD_PWR_PARAM),
				   (uint8_t *) prDpdCalResult, NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function starts system service such as timer and
 *        memory pools
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicInitSystemService(struct ADAPTER *prAdapter,
				   const u_int8_t bAtResetFlow)
{
	ASSERT(prAdapter);

	/* <1> Initialize MGMT Memory pool and STA_REC */
	if (!bAtResetFlow) {
		cnmMemInit(prAdapter);
#if CFG_SUPPORT_NAN
		nanResetMemory();
#endif
		cnmStaRecInit(prAdapter);
	}

	cmdBufInitialize(prAdapter);

	if (!bAtResetFlow) {
		/* <2> Mailbox Initialization */
		mboxInitialize(prAdapter);
	}

	/* <3> Timer Initialization */
	cnmTimerInitialize(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function reset some specific system service,
 *        such as STA-REC
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicResetSystemService(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicUninitSystemService(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	/* Timer Destruction */
	cnmTimerDestroy(prAdapter);

	/* Mailbox Destruction */
	mboxDestroy(prAdapter);

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicInitMGMT(struct ADAPTER *prAdapter,
		 struct REG_INFO *prRegInfo)
{
	uint8_t i;

	ASSERT(prAdapter);
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	/* register for power level control */
	kalPwrLevelHdlrRegister(prAdapter, cnmPowerControl);
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldBssInit(prAdapter);
	mldStarecInit(prAdapter);
#endif

	/* CNM Module - initialization */
	cnmInit(prAdapter);

	/* SCN Module - initialization */
	scnInit(prAdapter);

	wlanDfsChannelsReqInit(prAdapter);

	if (prAdapter->u4UapsdAcBmp == 0) {
		prAdapter->u4UapsdAcBmp = CFG_INIT_UAPSD_AC_BMP;
	}

	for (i = 0; i < KAL_AIS_NUM; i++) {
		/* AIS Module - intiailization */
		aisFsmInit(prAdapter, prRegInfo, i);
	}

	/* RLM Module - initialization */
	rlmFsmEventInit(prAdapter);

	/* Support AP Selection */
	LINK_MGMT_INIT(&prAdapter->rWifiVar.rBlockList);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	LINK_MGMT_INIT(&prAdapter->rWifiVar.rMldBlockList);
#endif

#if CFG_SUPPORT_SWCR
	swCrDebugInit(prAdapter);
#endif /* CFG_SUPPORT_SWCR */
#if CFG_SUPPORT_RTT
	rttInit(prAdapter);
#endif

#if CFG_SUPPORT_CCM && CFG_ENABLE_WIFI_DIRECT
	/* CCM Module - initialization */
	ccmInit(prAdapter);
#endif /* CFG_SUPPORT_CCM && CFG_ENABLE_WIFI_DIRECT */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to update WMM Parms
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicUninitMGMT(struct ADAPTER *prAdapter)
{
	uint8_t i;

	ASSERT(prAdapter);

#if CFG_SUPPORT_SWCR
	swCrDebugUninit(prAdapter);
#endif /* CFG_SUPPORT_SWCR */

	for (i = 0; i < KAL_AIS_NUM; i++) {
		/* AIS Module - unintiailization */
		aisFsmUninit(prAdapter, i);
	}

	/* Support AP Selection */
	LINK_MGMT_UNINIT(&prAdapter->rWifiVar.rBlockList,
			 struct AIS_BLOCKLIST_ITEM, VIR_MEM_TYPE);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	LINK_MGMT_UNINIT(&prAdapter->rWifiVar.rMldBlockList,
			struct MLD_BLOCKLIST_ITEM, VIR_MEM_TYPE);
#endif

	/* SCN Module - unintiailization */
	scnUninit(prAdapter);

	wlanDfsChannelsReqDeInit(prAdapter);

	/* RLM Module - uninitialization */
	rlmFsmEventUninit(prAdapter);

	/* CNM Module - uninitialization */
	cnmUninit(prAdapter);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldStarecUninit(prAdapter);
	mldBssUninit(prAdapter);
#endif
#if CFG_SUPPORT_RTT
	rttUninit(prAdapter);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is invoked to buffer scan result
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param rMacAddr           BSSID
 * @param prSsid             Pointer to SSID
 * @param u2CapInfo          Capability settings
 * @param rRssi              Received Strength (-10 ~ -200 dBm)
 * @param eNetworkType       Network Type (a/b/g)
 * @param prConfiguration    Network Parameter
 * @param eOpMode            Infra/Ad-Hoc
 * @param rSupportedRates    Supported basic rates
 * @param u2IELength         IE Length
 * @param pucIEBuf           Pointer to Information Elements(IEs)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
nicAddScanResult(struct ADAPTER *prAdapter,
		 uint8_t rMacAddr[PARAM_MAC_ADDR_LEN],
		 struct PARAM_SSID *prSsid,
		 uint16_t u2CapInfo,
		 int32_t rRssi,
		 enum ENUM_PARAM_NETWORK_TYPE eNetworkType,
		 struct PARAM_802_11_CONFIG *prConfiguration,
		 enum ENUM_PARAM_OP_MODE eOpMode,
		 uint8_t rSupportedRates[PARAM_MAX_LEN_RATES_EX],
		 uint16_t u2IELength, uint8_t *pucIEBuf)
{
	u_int8_t bReplace;
	uint32_t i;
	uint32_t u4IdxWeakest = 0;
	int32_t rWeakestRssi;
	uint32_t u4BufferSize;
	/* Privicy setting 0: Open / 1: WEP/WPA/WPA2 enabled */
	uint32_t u4Privacy = u2CapInfo & CAP_INFO_PRIVACY ? 1 : 0;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;

	ASSERT(prAdapter);

	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	rWeakestRssi = (int32_t) INT_MAX;
	u4BufferSize = ARRAY_SIZE(prWlanInfo->aucScanIEBuf);

	bReplace = FALSE;

	/* decide to replace or add */
	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
		uint8_t j;
		u_int8_t bUnMatch = TRUE;

		for (j = 0; j < KAL_AIS_NUM; j++) {
			struct PARAM_BSSID_EX *prCurrBssid;

			if (!AIS_MAIN_BSS_INFO(prAdapter, j))
				continue;

			prCurrBssid = aisGetCurrBssId(prAdapter,
				AIS_MAIN_BSS_INDEX(prAdapter, j));
			if (EQUAL_MAC_ADDR(prWlanInfo->
				arScanResult[i].arMacAddress,
				prCurrBssid->arMacAddress)) {
				bUnMatch = FALSE;
				break;
			}
		}

		/* find weakest entry && not connected one */
		if (bUnMatch && prScanResult[i].rRssi < rWeakestRssi) {
			u4IdxWeakest = i;
			rWeakestRssi = prScanResult[i].rRssi;
		}

		if (prScanResult[i].eOpMode == eOpMode &&
		    EQUAL_MAC_ADDR(&prScanResult[i].arMacAddress, rMacAddr) &&
		    (EQUAL_SSID(prScanResult[i].rSsid.aucSsid,
				prScanResult[i].rSsid.u4SsidLen,
				prSsid->aucSsid, prSsid->u4SsidLen) ||
		     prScanResult[i].rSsid.u4SsidLen == 0)) {
			/* replace entry */
			bReplace = TRUE;

			/* free IE buffer then zero */
			nicFreeScanResultIE(prAdapter, i);
			/* prScanResult[i].pucIE will be set later */
			kalMemZero(&prScanResult[i],
				   sizeof(struct PARAM_BSSID_EX));

			/* then fill buffer */
			prScanResult[i].u4Length =
				sizeof(struct PARAM_BSSID_EX) + u2IELength;
			COPY_MAC_ADDR(prScanResult[i].arMacAddress, rMacAddr);
			COPY_SSID(prScanResult[i].rSsid.aucSsid,
				  prScanResult[i].rSsid.u4SsidLen,
				  prSsid->aucSsid, prSsid->u4SsidLen);
			prScanResult[i].u4Privacy = u4Privacy;
			prScanResult[i].rRssi = rRssi;
			prScanResult[i].eNetworkTypeInUse = eNetworkType;
			kalMemCopy(&prScanResult[i].rConfiguration,
				   prConfiguration,
				   sizeof(struct PARAM_802_11_CONFIG));
			prScanResult[i].eOpMode = eOpMode;
			kalMemCopy(prScanResult[i].rSupportedRates,
				   rSupportedRates,
				   sizeof(uint8_t) * PARAM_MAX_LEN_RATES_EX);
			prScanResult[i].u4IELength = (uint32_t) u2IELength;

			/* IE - allocate buffer and update pointer */
			if (u2IELength > 0) {
				if (ALIGN_4(u2IELength) +
				    prWlanInfo->u4ScanIEBufferUsage
						<= u4BufferSize) {
					kalMemCopy(&prWlanInfo->aucScanIEBuf[
					       prWlanInfo->u4ScanIEBufferUsage],
					       pucIEBuf,
					       u2IELength);

					prScanResult[i].pucIE =
					       &prWlanInfo->aucScanIEBuf[
					       prWlanInfo->u4ScanIEBufferUsage];

					prWlanInfo->u4ScanIEBufferUsage
						+= ALIGN_4(u2IELength);
				} else {
					/* buffer is not enough */
					prScanResult[i].u4Length -= u2IELength;
					prScanResult[i].u4IELength = 0;
					prScanResult[i].pucIE = NULL;
				}
			} else {
				prScanResult[i].pucIE = NULL;
			}

			break;
		}
	}

	if (bReplace == FALSE) {
		if (prWlanInfo->u4ScanResultNum < (CFG_MAX_NUM_BSS_LIST - 1)) {
			i = prWlanInfo->u4ScanResultNum;

			/* zero */
			/* prScanResult[i].pucIE will be set later */
			kalMemZero(&prScanResult[i],
				   sizeof(struct PARAM_BSSID_EX));

			/* then fill buffer */
			prScanResult[i].u4Length =
				sizeof(struct PARAM_BSSID_EX) + u2IELength;
			COPY_MAC_ADDR(prScanResult[i].arMacAddress, rMacAddr);
			COPY_SSID(prScanResult[i].rSsid.aucSsid,
				  prScanResult[i].rSsid.u4SsidLen,
				  prSsid->aucSsid, prSsid->u4SsidLen);
			prScanResult[i].u4Privacy = u4Privacy;
			prScanResult[i].rRssi = rRssi;
			prScanResult[i].eNetworkTypeInUse = eNetworkType;
			kalMemCopy(&prScanResult[i].rConfiguration,
				   prConfiguration,
				   sizeof(struct PARAM_802_11_CONFIG));
			prScanResult[i].eOpMode = eOpMode;
			kalMemCopy(prScanResult[i].rSupportedRates,
				   rSupportedRates,
				   sizeof(uint8_t) * PARAM_MAX_LEN_RATES_EX);
			prScanResult[i].u4IELength = (uint32_t) u2IELength;

			/* IE - allocate buffer and update pointer */
			if (u2IELength > 0) {
				if (ALIGN_4(u2IELength) +
				    prWlanInfo->u4ScanIEBufferUsage
							<= u4BufferSize) {
					kalMemCopy(&prWlanInfo->aucScanIEBuf[
					       prWlanInfo->u4ScanIEBufferUsage],
					       pucIEBuf,
					       u2IELength);

					prScanResult[i].pucIE =
					       &prWlanInfo->aucScanIEBuf[
					       prWlanInfo->u4ScanIEBufferUsage];

					prWlanInfo->u4ScanIEBufferUsage +=
						ALIGN_4(u2IELength);
				} else {
					/* buffer is not enough */
					prScanResult[i].u4Length -= u2IELength;
					prScanResult[i].u4IELength = 0;
					prScanResult[i].pucIE = NULL;
				}
			} else {
				prScanResult[i].pucIE = NULL;
			}

			prWlanInfo->u4ScanResultNum++;
		} else if (rWeakestRssi != (int32_t) INT_MAX) {
			/* replace weakest one */
			i = u4IdxWeakest;

			/* free IE buffer then zero */
			nicFreeScanResultIE(prAdapter, i);
			/* prScanResult[i].pucIE will be set later */
			kalMemZero(&prScanResult[i],
				   sizeof(struct PARAM_BSSID_EX));

			/* then fill buffer */
			prScanResult[i].u4Length =
				sizeof(struct PARAM_BSSID_EX) + u2IELength;
			COPY_MAC_ADDR(prScanResult[i].arMacAddress, rMacAddr);
			COPY_SSID(prScanResult[i].rSsid.aucSsid,
				  prScanResult[i].rSsid.u4SsidLen,
				  prSsid->aucSsid, prSsid->u4SsidLen);
			prScanResult[i].u4Privacy = u4Privacy;
			prScanResult[i].rRssi = rRssi;
			prScanResult[i].eNetworkTypeInUse = eNetworkType;
			kalMemCopy(&prScanResult[i].rConfiguration,
				   prConfiguration,
				   sizeof(struct PARAM_802_11_CONFIG));
			prScanResult[i].eOpMode = eOpMode;
			kalMemCopy(prScanResult[i].rSupportedRates,
				   rSupportedRates,
				   (sizeof(uint8_t) * PARAM_MAX_LEN_RATES_EX));
			prScanResult[i].u4IELength = (uint32_t) u2IELength;

			if (u2IELength > 0) {
				/* IE - allocate buffer and update pointer */
				if (ALIGN_4(u2IELength) +
				    prWlanInfo->u4ScanIEBufferUsage
							<= u4BufferSize) {
					kalMemCopy(&prWlanInfo->aucScanIEBuf[
					       prWlanInfo->u4ScanIEBufferUsage],
					       pucIEBuf,
					       u2IELength);

					prScanResult[i].pucIE =
					       &prWlanInfo->aucScanIEBuf[
					       prWlanInfo->u4ScanIEBufferUsage];

					prWlanInfo->u4ScanIEBufferUsage +=
						ALIGN_4(u2IELength);
				} else {
					/* buffer is not enough */
					prScanResult[i].u4Length -= u2IELength;
					prScanResult[i].u4IELength = 0;
					prScanResult[i].pucIE = NULL;
				}
			} else {
				prScanResult[i].pucIE = NULL;
			}
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is invoked to free IE buffer for dedicated scan result
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param u4Idx              Index of Scan Result
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void nicFreeScanResultIE(struct ADAPTER *prAdapter, uint32_t u4Idx)
{
	uint32_t i;
	uint8_t *pucPivot, *pucMovePivot;
	uint32_t u4MoveSize, u4FreeSize, u4ReserveSize;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;

	ASSERT(prAdapter);
	if (u4Idx >= CFG_MAX_NUM_BSS_LIST) {
		DBGLOG(SCN, ERROR, "u4Idx %d is invalid\n", u4Idx);
		return;
	}

	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	if (prScanResult[u4Idx].u4IELength == 0 || !prScanResult[u4Idx].pucIE)
		return;

	u4FreeSize = ALIGN_4(prScanResult[u4Idx].u4IELength);

	pucPivot = prScanResult[u4Idx].pucIE;
	pucMovePivot = (uint8_t *)
		((uintptr_t) (prScanResult[u4Idx].pucIE) + u4FreeSize);

	u4ReserveSize = ((uintptr_t) pucPivot) -
		(uintptr_t) (&(prWlanInfo->aucScanIEBuf[0]));
	u4MoveSize = prWlanInfo->u4ScanIEBufferUsage -
		     u4ReserveSize - u4FreeSize;

	/* 1. rest of buffer to move forward */
	kalMemCopy(pucPivot, pucMovePivot, u4MoveSize);

	/* 1.1 modify pointers */
	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
		if (i != u4Idx) {
			if (prScanResult[i].pucIE >= pucMovePivot) {
				prScanResult[i].pucIE =
					(uint8_t *) ((uintptr_t) (
						prScanResult[i].pucIE)
						- u4FreeSize);
			}
		}
	}

	/* 1.2 reset the freed one */
	prScanResult[u4Idx].u4IELength = 0;
	prScanResult[i].pucIE = NULL; /* TODO: redundant? */

	/* 2. reduce IE buffer usage */
	prWlanInfo->u4ScanIEBufferUsage -= u4FreeSize;

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to hack parameters for WLAN TABLE for
 *        fixed rate settings
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param eRateSetting
 * @param pu2DesiredNonHTRateSet,
 * @param pu2BSSBasicRateSet,
 * @param pucMcsSet
 * @param pucSupMcs32
 * @param pu2HtCapInfo
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
nicUpdateRateParams(struct ADAPTER *prAdapter,
		    enum ENUM_REGISTRY_FIXED_RATE eRateSetting,
		    uint8_t *pucDesiredPhyTypeSet,
		    uint16_t *pu2DesiredNonHTRateSet,
		    uint16_t *pu2BSSBasicRateSet,
		    uint8_t *pucMcsSet, uint8_t *pucSupMcs32,
		    uint16_t *pu2HtCapInfo)
{
	ASSERT(prAdapter);
	ASSERT(eRateSetting > FIXED_RATE_NONE
	       && eRateSetting < FIXED_RATE_NUM);

	switch (prAdapter->rWifiVar.eRateSetting) {
	case FIXED_RATE_1M:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_1M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_1M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_2M:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_2M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_2M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_5_5M:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_5_5M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_5_5M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_11M:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HR_DSSS;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_11M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_11M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_6M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_6M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_6M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_9M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_9M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_9M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_12M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_12M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_12M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_18M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_18M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_18M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_24M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_24M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_24M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_36M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_36M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_36M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_48M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_48M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_48M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_54M:
		if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_ERP)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_ERP;
		else if ((*pucDesiredPhyTypeSet) & PHY_TYPE_BIT_OFDM)
			*pucDesiredPhyTypeSet = PHY_TYPE_BIT_OFDM;

		*pu2DesiredNonHTRateSet = RATE_SET_BIT_54M;
		*pu2BSSBasicRateSet = RATE_SET_BIT_54M;
		*pucMcsSet = 0;
		*pucSupMcs32 = 0;
		*pu2HtCapInfo = 0;
		break;

	case FIXED_RATE_MCS0_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS1_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS2_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS3_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS4_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS5_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS6_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS7_20M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH
			| HT_CAP_INFO_SHORT_GI_20M | HT_CAP_INFO_SHORT_GI_40M |
			HT_CAP_INFO_HT_GF);
		break;

	case FIXED_RATE_MCS0_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS1_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS2_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS3_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS4_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS5_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS6_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS7_20M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SUP_CHNL_WIDTH |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SHORT_GI_20M;
		break;

	case FIXED_RATE_MCS0_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS1_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS2_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS3_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS4_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS5_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS6_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS7_40M_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS32_800NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = 0;
		*pucSupMcs32 = 1;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
			HT_CAP_INFO_SHORT_GI_40M | HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= HT_CAP_INFO_SUP_CHNL_WIDTH;
		break;

	case FIXED_RATE_MCS0_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS0_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS1_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS1_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS2_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS2_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS3_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS3_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS4_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS4_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS5_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS5_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS6_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS6_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS7_40M_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = BIT(HT_RATE_MCS7_INDEX - 1);
		*pucSupMcs32 = 0;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	case FIXED_RATE_MCS32_400NS:
		*pucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;
		*pu2DesiredNonHTRateSet = RATE_SET_BIT_HT_PHY;
		*pu2BSSBasicRateSet = RATE_SET_BIT_HT_PHY;
		*pucMcsSet = 0;
		*pucSupMcs32 = 1;
		(*pu2HtCapInfo) &= ~(HT_CAP_INFO_SHORT_GI_20M |
				     HT_CAP_INFO_HT_GF);
		(*pu2HtCapInfo) |= (HT_CAP_INFO_SUP_CHNL_WIDTH |
				    HT_CAP_INFO_SHORT_GI_40M);
		break;

	default:
		ASSERT(0);
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to write the register
 *
 * @param u4Address         Register address
 *        u4Value           the value to be written
 *
 * @retval WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/

uint32_t nicWriteMcr(struct ADAPTER *prAdapter,
		     uint32_t u4Address, uint32_t u4Value)
{
	struct CMD_ACCESS_REG rCmdAccessReg;

	rCmdAccessReg.u4Address = u4Address;
	rCmdAccessReg.u4Data = u4Value;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_ACCESS_REG,
				   TRUE,
				   FALSE,
				   FALSE, NULL, NULL,
				   sizeof(struct CMD_ACCESS_REG),
				   (uint8_t *) &rCmdAccessReg, NULL, 0);

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to modify the auto rate parameters
 *
 * @param u4ArSysParam0  see description below
 *        u4ArSysParam1
 *        u4ArSysParam2
 *        u4ArSysParam3
 *
 *
 * @retval WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 *
 * @note
 *   ArSysParam0[0:3] -> auto rate version (0:disable 1:version1 2:version2)
 *   ArSysParam0[4:5]-> auto bw version (0:disable 1:version1 2:version2)
 *   ArSysParam0[6:7]-> auto gi version (0:disable 1:version1 2:version2)
 *   ArSysParam0[8:15]-> HT rate clear mask
 *   ArSysParam0[16:31]-> Legacy rate clear mask
 *   ArSysParam1[0:7]-> Auto Rate check weighting window
 *   ArSysParam1[8:15]-> Auto Rate v1 Force Rate down
 *   ArSysParam1[16:23]-> Auto Rate v1 PerH
 *   ArSysParam1[24:31]-> Auto Rate v1 PerL
 *
 *   Examples
 *   ArSysParam0 = 1,
 *   Enable auto rate version 1
 *
 *   ArSysParam0 = 983041,
 *   Enable auto rate version 1
 *   Remove CCK 1M, 2M, 5.5M, 11M
 *
 *   ArSysParam0 = 786433
 *   Enable auto rate version 1
 *   Remove CCK 5.5M 11M
 */
/*----------------------------------------------------------------------------*/

uint32_t
nicRlmArUpdateParms(struct ADAPTER *prAdapter,
		    uint32_t u4ArSysParam0,
		    uint32_t u4ArSysParam1, uint32_t u4ArSysParam2,
		    uint32_t u4ArSysParam3)
{
	uint8_t ucArVer, ucAbwVer, ucAgiVer;
	uint16_t u2HtClrMask;
	uint16_t u2LegacyClrMask;
	uint8_t ucArCheckWindow;
	uint8_t ucArPerL;
	uint8_t ucArPerH;
	uint8_t ucArPerForceRateDownPer;

	ucArVer = (uint8_t) (u4ArSysParam0 & BITS(0, 3));
	ucAbwVer = (uint8_t) ((u4ArSysParam0 & BITS(4, 5)) >> 4);
	ucAgiVer = (uint8_t) ((u4ArSysParam0 & BITS(6, 7)) >> 6);
	u2HtClrMask = (uint16_t) ((u4ArSysParam0 & BITS(8,
				   15)) >> 8);
	u2LegacyClrMask = (uint16_t) ((u4ArSysParam0 & BITS(16,
				       31)) >> 16);

#if 0
	ucArCheckWindow = (uint8_t) (u4ArSysParam1 & BITS(0, 7));
	ucArPerH = (uint8_t) ((u4ArSysParam1 & BITS(16, 23)) >> 16);
	ucArPerL = (uint8_t) ((u4ArSysParam1 & BITS(24, 31)) >> 24);
#endif

	ucArCheckWindow = (uint8_t) (u4ArSysParam1 & BITS(0, 7));
	ucArPerForceRateDownPer = (uint8_t) (((u4ArSysParam1 >> 8) &
					      BITS(0, 7)));
	ucArPerH = (uint8_t) (((u4ArSysParam1 >> 16) & BITS(0, 7)));
	ucArPerL = (uint8_t) (((u4ArSysParam1 >> 24) & BITS(0, 7)));

	DBGLOG(INIT, INFO, "ArParam %u %u %u %u\n", u4ArSysParam0,
	       u4ArSysParam1, u4ArSysParam2, u4ArSysParam3);
	DBGLOG(INIT, INFO, "ArVer %u AbwVer %u AgiVer %u\n",
	       ucArVer, ucAbwVer, ucAgiVer);
	DBGLOG(INIT, INFO, "HtMask %x LegacyMask %x\n", u2HtClrMask,
	       u2LegacyClrMask);
	DBGLOG(INIT, INFO,
	       "CheckWin %u RateDownPer %u PerH %u PerL %u\n",
	       ucArCheckWindow,
	       ucArPerForceRateDownPer, ucArPerH, ucArPerL);

#define SWCR_DATA_ADDR(MOD, ADDR) (0x90000000+(MOD<<8)+(ADDR))
#define SWCR_DATA_CMD(CATE, WRITE, INDEX, OPT0, OPT1) \
	((CATE<<24) | (WRITE<<23) | (INDEX<<16) | (OPT0 << 8) | OPT1)
#define SWCR_DATA0 0x0
#define SWCR_DATA1 0x4
#define SWCR_DATA2 0x8
#define SWCR_DATA3 0xC
#define SWCR_DATA4 0x10
#define SWCR_WRITE 1
#define SWCR_READ 0

	if (ucArVer > 0) {
		/* dummy = WiFi.WriteMCR(&h90000104, &h00000001) */
		/* dummy = WiFi.WriteMCR(&h90000100, &h00850000) */

		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), 1);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 5, 0, 0));
	} else {
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), 0);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 5, 0, 0));
	}

	/* ucArVer 0: none 1:PER 2:Rcpi */
	nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
		SWCR_DATA1), ucArVer);
	nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
		SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 7, 0, 0));

	/* Candidate rate Ht mask */
	nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
		SWCR_DATA1), u2HtClrMask);
	nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
		SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0x1c, 0, 0));

	/* Candidate rate legacy mask */
	nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
		SWCR_DATA1), u2LegacyClrMask);
	nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
		SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0x1d, 0, 0));

#if 0
	if (ucArCheckWindow != 0) {
		/* TX DONE MCS INDEX CHECK STA RATE DOWN TH */
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), ucArCheckWindow);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0x14, 0, 0));
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), ucArCheckWindow);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0xc, 0, 0));
	}

	if (ucArPerForceRateDownPer != 0) {
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), ucArPerForceRateDownPer);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0x18, 0, 0));
	}
	if (ucArPerH != 0) {
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), ucArPerH);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0x1, 0, 0));
	}
	if (ucArPerL != 0) {
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA1), ucArPerL);
		nicWriteMcr(prAdapter, SWCR_DATA_ADDR(1 /*MOD*/,
			SWCR_DATA0), SWCR_DATA_CMD(0, SWCR_WRITE, 0x2, 0, 0));
	}
#endif

	return WLAN_STATUS_SUCCESS;
}

#if (CFG_TWT_SMART_STA == 1)
void nicUpdateLinkQualityForTwt(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, int8_t cRssi)
{
	struct _MSG_TWT_PARAMS_SET_T *prTWTParamSetMsg = NULL;
	struct _TWT_CTRL_T rTWTCtrl;

	DBGLOG(RLM, INFO, "smarttwtreq=%d, smarttwtact=%d(%d)\n",
		g_TwtSmartStaCtrl.fgTwtSmartStaReq,
		g_TwtSmartStaCtrl.fgTwtSmartStaActivated,
		g_TwtSmartStaCtrl.eState);

	if ((cRssi >= (-35)) &&
		(g_TwtSmartStaCtrl.u4TwtSwitch == 0) &&
		((g_TwtSmartStaCtrl.fgTwtSmartStaReq == TRUE) &&
		 (g_TwtSmartStaCtrl.fgTwtSmartStaActivated == FALSE) &&
		 (g_TwtSmartStaCtrl.eState == TWT_SMART_STA_STATE_IDLE))
	) {
		rTWTCtrl.ucBssIdx = ucBssIndex;
		rTWTCtrl.ucCtrlAction = 4;
		rTWTCtrl.ucTWTFlowId = 0;
		rTWTCtrl.rTWTParams.fgReq = TRUE;
		rTWTCtrl.rTWTParams.ucSetupCmd = 1;
		rTWTCtrl.rTWTParams.fgTrigger = 0;
		rTWTCtrl.rTWTParams.fgUnannounced = 1;
		rTWTCtrl.rTWTParams.ucWakeIntvalExponent = 10;
		rTWTCtrl.rTWTParams.fgProtect = 0;
		rTWTCtrl.rTWTParams.ucMinWakeDur = 255;
		rTWTCtrl.rTWTParams.u2WakeIntvalMantiss = 512;
		g_TwtSmartStaCtrl.eState = TWT_SMART_STA_STATE_REQUESTING;

		prTWTParamSetMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(struct _MSG_TWT_REQFSM_RESUME_T));
	}

	if (((cRssi <= (-40)) ||
		(g_TwtSmartStaCtrl.fgTwtSmartStaTeardownReq == TRUE))
		&& (g_TwtSmartStaCtrl.fgTwtSmartStaActivated == TRUE)
	) {
		rTWTCtrl.ucBssIdx = ucBssIndex;
		rTWTCtrl.ucCtrlAction = 5;
		rTWTCtrl.ucTWTFlowId = g_TwtSmartStaCtrl.ucFlowId;

		DBGLOG(RLM, INFO, "twtswitch=%d, rxrate=%d\n",
			g_TwtSmartStaCtrl.u4TwtSwitch,
			g_TwtSmartStaCtrl.u4LastTp);

		prTWTParamSetMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(struct _MSG_TWT_REQFSM_RESUME_T));
	}

	if (prTWTParamSetMsg) {
		prTWTParamSetMsg->rMsgHdr.eMsgId = MID_TWT_PARAMS_SET;
		kalMemCopy(&prTWTParamSetMsg->rTWTCtrl, &rTWTCtrl,
			sizeof(rTWTCtrl));

		mboxSendMsg(prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prTWTParamSetMsg,
			MSG_SEND_METHOD_BUF);
	}
}
#endif /* CFG_TWT_SMART_STA */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update Link Quality information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        ucBssIndex
 *        cRssi
 *        cLinkQuality
 *		  u2LinkSpeed
 *		  ucMediumBusyPercentage,
 *		  ucIsLQ0Rdy
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicUpdateLinkQuality(struct ADAPTER *prAdapter,
			  uint8_t ucBssIndex,
			  int8_t cRssi,
			  int8_t cLinkQuality,
			  uint16_t u2LinkSpeed,
			  uint8_t ucMediumBusyPercentage,
			  uint8_t ucIsLQ0Rdy)
{
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
		return;

	if (!ucIsLQ0Rdy)
		return;

	switch (prBssInfo->eNetworkType) {
	case NETWORK_TYPE_AIS:
#if (CFG_TWT_SMART_STA == 1)
		nicUpdateLinkQualityForTwt(prAdapter, ucBssIndex, cRssi);
#endif
		/*
		 * fallthrough
		 * update RSSI/LinkSpeed for both STA and P2P
		 */
	case NETWORK_TYPE_P2P:
		nicUpdateRSSI(prAdapter, ucBssIndex, cRssi, cLinkQuality);

		nicUpdateLinkSpeed(prAdapter, ucBssIndex, u2LinkSpeed);
		break;

	default:
		break;

	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update RSSI and Link Quality information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        ucBssIndex
 *        cRssi
 *        cLinkQuality
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicUpdateRSSI(struct ADAPTER *prAdapter,
		   uint8_t ucBssIndex, int8_t cRssi,
		   int8_t cLinkQuality)
{
	struct BSS_INFO *prBssInfo;
	struct LINK_SPEED_EX_ *prLq;

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;

	if (prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
		return;

	prLq = &prAdapter->rLinkQuality.rLq[ucBssIndex];

	switch (prBssInfo->eNetworkType) {
	case NETWORK_TYPE_AIS:
	case NETWORK_TYPE_P2P:
		prLq->fgIsLinkQualityValid = TRUE;
		prLq->rLinkQualityUpdateTime = kalGetTimeTick();

		prLq->cRssi = cRssi;
		prLq->cLinkQuality = cLinkQuality;

		/* indicate to glue layer */
		kalUpdateRSSI(prAdapter->prGlueInfo,
			ucBssIndex, cRssi, cLinkQuality);
		break;
	default:
		break;

	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update Link Quality information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        ucBssIndex
 *        prEventLinkQuality
 *        cRssi
 *        cLinkQuality
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicUpdateLinkSpeed(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint16_t u2TxLinkSpeed)
{
	struct BSS_INFO *prBssInfo;
	uint32_t u4CurRxRate, u4MaxRxRate;
	struct RxRateInfo rRxRateInfo = {0};

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	if (ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;

	if (prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED)
		return;

	if ((prBssInfo->eNetworkType != NETWORK_TYPE_AIS &&
		prBssInfo->eNetworkType != NETWORK_TYPE_P2P))
		return;

	/*Fill Rx Rate in unit of 100bps*/
	if (wlanGetRxRateByBssid(prAdapter->prGlueInfo, ucBssIndex,
			&u4CurRxRate, &u4MaxRxRate, &rRxRateInfo) == 0) {
		prAdapter->rLinkQuality.rLq[ucBssIndex].
			u2RxLinkSpeed = u4CurRxRate * 1000;
		prAdapter->rLinkQuality.rLq[ucBssIndex].
			u4RxBw = rRxRateInfo.u4Bw;
	} else {
		prAdapter->rLinkQuality.rLq[ucBssIndex].u2RxLinkSpeed = 0;
		prAdapter->rLinkQuality.rLq[ucBssIndex].u4RxBw = 0;
	}
	/* change to unit of 100 bps */
	/*
	 *  FW report in 500kbps because u2TxLinkSpeed is 16 bytes
	 *  TODO:
	 *    driver and fw should change u2TxLinkSpeed to u4
	 *    because it will overflow in wifi7
	 */
	prAdapter->rLinkQuality.rLq[ucBssIndex].
		u2TxLinkSpeed = u2TxLinkSpeed * 5000;

	prAdapter->rLinkQuality.rLq[ucBssIndex].fgIsLinkRateValid = TRUE;
	prAdapter->rLinkQuality.rLq[ucBssIndex].rLinkRateUpdateTime
		= kalGetTimeTick();
	DBGLOG(NIC, TRACE, "bss:%u linkspeed:%u/%u LRValid:%u updateTime:%u\n",
		ucBssIndex,
		prAdapter->rLinkQuality.rLq[ucBssIndex].u2TxLinkSpeed,
		prAdapter->rLinkQuality.rLq[ucBssIndex].u2RxLinkSpeed,
		prAdapter->rLinkQuality.rLq[ucBssIndex].fgIsLinkRateValid,
		prAdapter->rLinkQuality.rLq[ucBssIndex].rLinkRateUpdateTime);
}

#if CFG_SUPPORT_RDD_TEST_MODE
uint32_t nicUpdateRddTestMode(struct ADAPTER *prAdapter,
			      struct CMD_RDD_CH *prRddChParam)
{
	ASSERT(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_RDD_CH,
				   TRUE,
				   FALSE, FALSE, NULL, NULL,
				   sizeof(struct CMD_RDD_CH),
				   (uint8_t *) prRddChParam, NULL, 0);
}
#endif
#if CFG_ENABLE_WIFI_DIRECT
void nicApplyP2pNetworkAddress(struct ADAPTER *prAdapter)
{
	uint8_t i, j;
	u_int8_t fgIsMacDuplicate;
	uint8_t *aucMacAddr;

	for (i = 0; i < KAL_P2P_NUM; i++) {
		aucMacAddr = prAdapter->rWifiVar.aucP2pDeviceAddress[i];
		COPY_MAC_ADDR(aucMacAddr, prAdapter->rMyMacAddr);
		aucMacAddr[0] = MAC_ADDR_LOCAL_ADMIN;
		do {
			kalRandomGetBytes(&aucMacAddr[3], 3);

			fgIsMacDuplicate = FALSE;
			if (EQUAL_MAC_ADDR(aucMacAddr,
			    prAdapter->rWifiVar.aucDeviceAddress))
				fgIsMacDuplicate = TRUE;
			for (j = 0; j < i; j++)
				if (EQUAL_MAC_ADDR(aucMacAddr,
				    prAdapter->rWifiVar.aucP2pDeviceAddress[j]))
					fgIsMacDuplicate = TRUE;
		} while (fgIsMacDuplicate);

		/* Let p2p group interface MAC addr same as P2P Device */
		aucMacAddr = prAdapter->rWifiVar.aucP2pInterfaceAddress[i];
		COPY_MAC_ADDR(aucMacAddr, prAdapter->rWifiVar
			.aucP2pDeviceAddress[i]);

		DBGLOG(NIC, INFO,
			"P2P[%u] DEV mac:" MACSTR " INF mac:" MACSTR "\n",
			i, MAC2STR(prAdapter->rWifiVar.aucP2pDeviceAddress[i]),
			MAC2STR(prAdapter->rWifiVar.aucP2pInterfaceAddress[i]));
	}

}

void nicApplyP2pNetworkFixAddress(struct ADAPTER *prAdapter)
{
	uint8_t i;
	uint8_t *aucMacAddr;

	for (i = 0; i < KAL_P2P_NUM; i++) {
		aucMacAddr = prAdapter->rWifiVar.aucP2pDeviceAddress[i];
		COPY_MAC_ADDR(aucMacAddr, prAdapter->rMyMacAddr);

		aucMacAddr[0] |= 0x2;
		aucMacAddr[0] ^=
			i << MAC_ADDR_LOCAL_ADMIN;

		/* Let p2p group interface MAC addr same as P2P Device */
		aucMacAddr = prAdapter->rWifiVar.aucP2pInterfaceAddress[i];
		COPY_MAC_ADDR(aucMacAddr, prAdapter->rWifiVar
			.aucP2pDeviceAddress[i]);

		DBGLOG(NIC, INFO,
			"P2P[%u] DEV mac:" MACSTR " INF mac:" MACSTR "\n",
			i, MAC2STR(prAdapter->rWifiVar.aucP2pDeviceAddress[i]),
			MAC2STR(prAdapter->rWifiVar.aucP2pInterfaceAddress[i]));
	}
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to apply network address setting to
 *        both OS side and firmware domain
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
uint32_t nicApplyNetworkAddress(struct ADAPTER *prAdapter)
{
	uint8_t i;

	_Static_assert(KAL_AIS_NUM < 8,
		"Large KAL_AIS_NUM would cause out-of-range bit XOR");

	ASSERT(prAdapter);

	/* copy to adapter */
	COPY_MAC_ADDR(prAdapter->rMyMacAddr,
		      prAdapter->rWifiVar.aucMacAddress);
	DBGLOG(NIC, INFO, "WLAN0 mac: " MACSTR "\n",
		MAC2STR(prAdapter->rMyMacAddr));

	/* 4 <3> Update new MAC address to all 3 networks */
	COPY_MAC_ADDR(prAdapter->rWifiVar.aucDeviceAddress,
		      prAdapter->rMyMacAddr);
	prAdapter->rWifiVar.aucDeviceAddress[0] = MAC_ADDR_LOCAL_ADMIN;

#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->rWifiVar.ucP2pMacAddrOverride == FALSE)
		nicApplyP2pNetworkAddress(prAdapter);
	else
		nicApplyP2pNetworkFixAddress(prAdapter);
#endif

#if CFG_ENABLE_BT_OVER_WIFI
	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		if (prAdapter->rWifiVar.arBssInfoPool[i].eNetworkType ==
		    NETWORK_TYPE_BOW) {
			COPY_MAC_ADDR(
				prAdapter->rWifiVar.arBssInfoPool[i].
				aucOwnMacAddr,
				prAdapter->rWifiVar.aucDeviceAddress);
		}
	}
#endif

#if CFG_TEST_WIFI_DIRECT_GO
	if (prAdapter->rWifiVar.prP2pFsmInfo->eCurrentState ==
	    P2P_STATE_IDLE) {
		wlanEnableP2pFunction(prAdapter);

		wlanEnableATGO(prAdapter);
	}
#endif

	kalUpdateMACAddress(prAdapter->prGlueInfo,
			    prAdapter->rWifiVar.aucMacAddress[0]);

	for (i = 1; i < KAL_AIS_NUM; i++) {
		/* Generate from wlan0 MAC */
		COPY_MAC_ADDR(prAdapter->rWifiVar.aucMacAddress[i],
			prAdapter->rWifiVar.aucMacAddress);
		/* Update wlan#i address */
		prAdapter->rWifiVar.aucMacAddress[i][3] ^= BIT(i);
		DBGLOG(NIC, INFO, "WLAN%d mac: " MACSTR "\n",
			i, MAC2STR(prAdapter->rWifiVar.aucMacAddress[i]));
	}

	return WLAN_STATUS_SUCCESS;
}

void nicApplyLinkAddress(struct ADAPTER *prAdapter,
	uint8_t *pucSrcMAC,
	uint8_t *pucDestMAC,
	uint8_t ucLinkIdx)
{
	uint8_t src[MAC_ADDR_LEN];

	COPY_MAC_ADDR(src, pucSrcMAC);
	COPY_MAC_ADDR(pucDestMAC, src);
	pucDestMAC[5] ^= ucLinkIdx;

	DBGLOG(NIC, INFO, "ucLinkIdx: %d, src mac: " MACSTR ", dest mac: " MACSTR "\n",
		ucLinkIdx,
		MAC2STR(src),
		MAC2STR(pucDestMAC));
}

#if 1
uint8_t nicGetChipHwVer(void)
{
	return g_eco_info.ucHwVer;
}

uint8_t nicGetChipSwVer(void)
{
	return g_eco_info.ucRomVer;
}

uint8_t nicGetChipFactoryVer(void)
{
	return g_eco_info.ucFactoryVer;
}

uint8_t nicSetChipHwVer(uint8_t value)
{
	g_eco_info.ucHwVer = value;
	return 0;
}

uint8_t nicSetChipSwVer(uint8_t value)
{
	g_eco_info.ucRomVer = value;
	return 0;
}

uint8_t nicSetChipFactoryVer(uint8_t value)
{
	g_eco_info.ucFactoryVer = value;
	return 0;
}

#else
uint8_t nicGetChipHwVer(void)
{
	return mtk_wcn_wmt_ic_info_get(WMTCHIN_HWVER) & BITS(0, 7);
}

uint8_t nicGetChipSwVer(void)
{
	return mtk_wcn_wmt_ic_info_get(WMTCHIN_FWVER) & BITS(0, 7);
}

uint8_t nicGetChipFactoryVer(void)
{
	return (mtk_wcn_wmt_ic_info_get(WMTCHIN_FWVER) & BITS(8,
			11)) >> 8;
}
#endif

uint8_t nicGetChipEcoVer(struct ADAPTER *prAdapter)
{
	struct ECO_INFO *prEcoInfo;
	uint8_t ucEcoVer;
	uint8_t ucCurSwVer, ucCurHwVer, ucCurFactoryVer;

	ucCurSwVer = nicGetChipSwVer();
	ucCurHwVer = nicGetChipHwVer();
	ucCurFactoryVer = nicGetChipFactoryVer();

	ucEcoVer = 0;

	while (TRUE) {
		/* Get ECO info from table */
		prEcoInfo = (struct ECO_INFO *) &
			    (prAdapter->chip_info->eco_info[ucEcoVer]);

		if ((prEcoInfo->ucRomVer == 0) &&
		    (prEcoInfo->ucHwVer == 0) &&
		    (prEcoInfo->ucFactoryVer == 0)) {

			/* last ECO info */
			if (ucEcoVer > 0)
				ucEcoVer--;

			/* End of table */
			break;
		}

		if ((prEcoInfo->ucRomVer == ucCurSwVer) &&
		    (prEcoInfo->ucHwVer == ucCurHwVer) &&
		    (prEcoInfo->ucFactoryVer == ucCurFactoryVer)) {
			break;
		}

		ucEcoVer++;
	}

#if 0
	DBGLOG(INIT, INFO,
	       "Cannot get ECO version for SwVer[0x%02x]HwVer[0x%02x]FactoryVer[0x%1x],recognize as latest version[E%u]\n",
	       ucCurSwVer, ucCurHwVer, ucCurFactoryVer,
	       prAdapter->chip_info->eco_info[ucEcoVer].ucEcoVer);
#endif
	return prAdapter->chip_info->eco_info[ucEcoVer].ucEcoVer;
}

u_int8_t nicIsEcoVerEqualTo(struct ADAPTER *prAdapter,
			    uint8_t ucEcoVer)
{
	if (ucEcoVer == prAdapter->chip_info->eco_ver)
		return TRUE;
	else
		return FALSE;
}

u_int8_t nicIsEcoVerEqualOrLaterTo(struct ADAPTER
				   *prAdapter, uint8_t ucEcoVer)
{
	if (ucEcoVer <= prAdapter->chip_info->eco_ver)
		return TRUE;
	else
		return FALSE;
}

void nicSerStopTxRx(struct ADAPTER *prAdapter)
{
#if defined(_HIF_USB)
	KAL_SPIN_LOCK_DECLARATION();

	/*TODO: multiple spinlocks seems risky.
	* http://www.linuxgrill.com/anonymous/fire/netfilter/
	*			    kernel-hacking-HOWTO-5.html
	*/
	/* 1. Make sure ucSerState is accessed sequentially.
	*  2. Two scenario for race condition:
	*    - When hif_thread is doing usb_submit_urb, SER occurs.
	*	hif_thread acquires the lock first,
	*	so nicSerSyncTimerHandler must wait hif_thread
	*	until it completes current usb_submit_urb.
	*	Then, nicSerSyncTimerHandler acquires the lock,
	*	change ucSerState to prevent subsequent usb_submit_urb and
	*	cancel ALL TX BULK OUT URB.
	*    - When SER is triggered and executed,
	*	hif_thread is prepared to do usb_submit_urb.
	*	nicSerSyncTimerHandler acquires the lock first,
	*	which guarantees ucSerState is accessed sequentially.
	*	Then, hif_thread acquires the lock, knows that SER is ongoing,
	*	and bypass usb_submit_urb.
	*/
	KAL_HIF_STATE_LOCK(prAdapter->prGlueInfo);
#endif

	DBGLOG(NIC, WARN, "[SER] host set STOP_TRX!\n");

	prAdapter->ucSerState = SER_STOP_HOST_TX_RX;
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	{
		struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

		if (prBusInfo->setDmaIntMask)
			prBusInfo->setDmaIntMask(prAdapter->prGlueInfo,
				BIT(DMA_INT_TYPE_TRX),
				FALSE);
	}
#endif

	/* Force own to FW as ACK and stop HIF */
	prAdapter->fgWiFiInSleepyState = TRUE;

#if defined(_HIF_USB)
	KAL_HIF_STATE_UNLOCK(prAdapter->prGlueInfo);
#endif

}

void nicSerStopTx(struct ADAPTER *prAdapter)
{
	DBGLOG(NIC, WARN, "[SER] Stop HIF Tx!\n");

	prAdapter->ucSerState = SER_STOP_HOST_TX;
}

void nicSerStartTxRx(struct ADAPTER *prAdapter)
{
	DBGLOG(NIC, WARN, "[SER] Start HIF T/R!\n");
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	{
		struct BUS_INFO *prBusInfo = prAdapter->chip_info->bus_info;

		if (prBusInfo->setDmaIntMask)
			prBusInfo->setDmaIntMask(prAdapter->prGlueInfo,
				BIT(DMA_INT_TYPE_TRX),
				TRUE);
	}
#endif
	prAdapter->ucSerState = SER_IDLE_DONE;
}

u_int8_t nicSerIsWaitingReset(struct ADAPTER *prAdapter)
{
	if (prAdapter->ucSerState == SER_STOP_HOST_TX_RX)
		return TRUE;
	else
		return FALSE;
}

u_int8_t nicSerIsTxStop(struct ADAPTER *prAdapter)
{
	switch (prAdapter->ucSerState) {
	case SER_STOP_HOST_TX:
	case SER_STOP_HOST_TX_RX:
	case SER_REINIT_HIF:
		return TRUE;

	case SER_IDLE_DONE:
	default:
		return FALSE;
	}
}

u_int8_t nicSerIsRxStop(struct ADAPTER *prAdapter)
{
	switch (prAdapter->ucSerState) {
	case SER_STOP_HOST_TX_RX:
	case SER_REINIT_HIF:
		return TRUE;

	case SER_STOP_HOST_TX:
	case SER_IDLE_DONE:
	default:
		return FALSE;
	}
}

#if (CFG_SUPPORT_ADHOC) || (CFG_ENABLE_WIFI_DIRECT)

void nicSerReInitBeaconFrame(struct ADAPTER *prAdapter)
{
	struct MSG_HDR *prBeaconReinitMsg;

	prBeaconReinitMsg = cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, sizeof(struct MSG_HDR));
	if (!prBeaconReinitMsg) {
		DBGLOG(HAL, WARN, "Allocate memory fail\n");
	} else {
		/* To manipulate p2p role in main_thread */
		prBeaconReinitMsg->eMsgId =
			MID_MNY_P2P_BEACON_REINIT;
		mboxSendMsg(prAdapter,
			MBOX_ID_0,
			prBeaconReinitMsg,
			MSG_SEND_METHOD_BUF);
	}
}
#endif

#if defined(_HIF_USB)
void nicSerTimerHandler(struct ADAPTER *prAdapter,
	uintptr_t plParamPtr)
{
	halSerSyncTimerHandler(prAdapter);
	cnmTimerStartTimer(prAdapter,
		&rSerSyncTimer,
		WIFI_SER_SYNC_TIMER_TIMEOUT_IN_MS);
}
#endif

void nicSerInit(struct ADAPTER *prAdapter, const u_int8_t bAtResetFlow)
{
	if (prAdapter->chip_info->asicSerInit)
		prAdapter->chip_info->asicSerInit(prAdapter, bAtResetFlow);

#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT
	if (prAdapter->chip_info->fgIsSupportL0p5Reset) {
		if (!bAtResetFlow) {
			glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);
			INIT_WORK(&prAdapter->prGlueInfo->rWfsysResetWork,
				  WfsysResetHdlr);
		}
		if (prAdapter->rWifiVar.eEnableSerL0p5 ==
		    FEATURE_OPT_SER_DISABLE)
			wlanoidSerExtCmd(prAdapter, SER_ACTION_L0P5_CTRL,
					 SER_ACTION_L0P5_CTRL_PAUSE_WDT, 0);
	}
#endif

	/* if SER L1 is not enabled, disable this feature in FW */
	if (prAdapter->rWifiVar.eEnableSerL1 == FEATURE_OPT_SER_DISABLE)
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET,
				SER_SET_DISABLE, 0);

#if defined(_HIF_USB)
	if (prAdapter->chip_info->u4SerUsbMcuEventAddr != 0) {
		if (!bAtResetFlow) {
			cnmTimerInitTimer(prAdapter,
					  &rSerSyncTimer,
			      (PFN_MGMT_TIMEOUT_FUNC) nicSerTimerHandler,
					  (uintptr_t) NULL);
		}
		cnmTimerStartTimer(prAdapter,
				   &rSerSyncTimer,
				   WIFI_SER_SYNC_TIMER_TIMEOUT_IN_MS);
	}
#endif /* _HIF_USB */
}

void nicSerDeInit(struct ADAPTER *prAdapter)
{
#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT
	if (prAdapter->chip_info->fgIsSupportL0p5Reset)
		cancel_work_sync(&prAdapter->prGlueInfo->rWfsysResetWork);
#endif

#if defined(_HIF_USB)
	cnmTimerStopTimer(prAdapter, &rSerSyncTimer);
#endif
}
/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
void nicUpdateWakeupStatistics(struct ADAPTER *prAdapter,
	enum WAKEUP_TYPE intType)
{
	struct WAKEUP_STATISTIC *prWakeupSta =
		&prAdapter->arWakeupStatistic[intType];
	OS_SYSTIME rCurrent = 0;

	prWakeupSta->u2Count++;
	if (prWakeupSta->u2Count % 100 == 0) {
		if (prWakeupSta->u2Count > 0) {
			GET_CURRENT_SYSTIME(&rCurrent);
			prWakeupSta->u2TimePerHundred =
				rCurrent-prWakeupSta->rStartTime;
		}
		GET_CURRENT_SYSTIME(&prWakeupSta->rStartTime);
		DBGLOG(RX, INFO, "wakeup frequency: %d",
			prWakeupSta->u2TimePerHundred);
	}
}
#endif /* fos_change end */

#if CFG_SUPPORT_PKT_OFLD

void nicRXDataModeConfig(struct ADAPTER *prAdapter)
{
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	uint8_t cmd[30] = {0};
	uint8_t strLen = 0;
	uint32_t strOutLen = 0;

	strLen = kalSnprintf(cmd, sizeof(cmd),
			"PktOfldRxDataMode %d",
			prAdapter->ucRxDataMode);
	DBGLOG(RX, INFO, "Notify FW %s, strlen=%d", cmd, strLen);

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
	rChipConfigInfo.u2MsgSize = strLen;
	kalStrnCpy(rChipConfigInfo.aucCmd, cmd, strLen);
	wlanSetChipConfig(prAdapter, &rChipConfigInfo,
			sizeof(rChipConfigInfo), &strOutLen, FALSE);
}

#endif

uint8_t nicRxdChNumTranslate(enum ENUM_BAND eBand, uint8_t ucHwChannelNum)
{
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand != BAND_6G)
		return ucHwChannelNum;

	if (ucHwChannelNum != HW_CHNL_NUM_MAX_2G4 + 1)
		return (((ucHwChannelNum - CHNL_NUM_BASE_6G) << 2) + 1);
	/* 6 GHz channel 2, RXV channel will assign to 15 */
	else
		return 2;
#endif
	return ucHwChannelNum;
}

void nicDumpMsduInfo(struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucData = NULL;

	if (!prMsduInfo) {
		DBGLOG(NIC, ERROR, "Invalid MsduInfo, skip dump.");
		return;
	}

	/* [1]prPacket(txd)            [2]eSrc
	 * [3]ucUserPriority           [4]ucTC
	 * [5]ucPacketType             [6]ucStaRecIndex
	 * [7]ucBssIndex               [8]ucWlanIndex
	 * [9]ucPacketFormat           [10]fgIs802_1x
	 * [11]fgIs802_1x_NonProtected [12]fgIs802_11
	 * [13]fgIs802_3               [14]fgIsVlanExists
	 * [15]u4Option                [16]cPowerOffset
	 * [17]u2SwSN                  [18]ucRetryLimit
	 * [19]u4RemainingLifetime     [20]ucControlFlag
	 * [21]ucRateMode              [22]u4FixedRateOption
	 * [23]fgIsTXDTemplateValid    [24]ucMacHeaderLength
	 * [25]ucLlcLength             [26]u2FrameLength
	 * [27]aucEthDestAddr          [28]u4PageCount
	 * [29]ucTxSeqNum              [30]ucPID
	 * [31]ucWmmQueSet             [32]pfTxDoneHandler
	 * [33]u4TxDoneTag             [34]ucPktType
	 */

#define TEMP_LINE1 \
	"[1][%p], [2][%u], [3][%u], [4][%u], [5][%u], " \
	"[6][%u], [7][%u], [8][%u], [9][%u], [10][%u]\n"

#define TEMP_LINE2 \
	"[11][%u], [12][%u], [13][%u], [14][%u], [15][%u], " \
	"[16][%d], [17][%u], [18][%u], [19][%u], [20][%u]\n"

#define TEMP_LINE3 \
	"[21][%u], [22][%u], [23][%u], [24][%u], [25][%u], " \
	"[26][%u], [27][" MACSTR "], [28][%u], [29][%u], [30][%u]\n"

#define TEMP_LINE4 \
	"[31][%u], [32][%p], [33][%u], [34][%u]\n"

	DBGLOG(NIC, INFO, TEMP_LINE1,
		prMsduInfo->prPacket, prMsduInfo->eSrc,
		prMsduInfo->ucUserPriority, prMsduInfo->ucTC,
		prMsduInfo->ucPacketType, prMsduInfo->ucStaRecIndex,
		prMsduInfo->ucBssIndex, prMsduInfo->ucWlanIndex,
		prMsduInfo->ucPacketFormat, prMsduInfo->fgIs802_1x
		);

	DBGLOG(NIC, INFO, TEMP_LINE2,
		prMsduInfo->fgIs802_1x_NonProtected, prMsduInfo->fgIs802_11,
		prMsduInfo->fgIs802_3, prMsduInfo->fgIsVlanExists,
		prMsduInfo->u4Option, prMsduInfo->cPowerOffset,
		prMsduInfo->u2SwSN, prMsduInfo->ucRetryLimit,
		prMsduInfo->u4RemainingLifetime, prMsduInfo->ucControlFlag
		);

	DBGLOG(NIC, INFO, TEMP_LINE3,
		prMsduInfo->ucRateMode, prMsduInfo->u4FixedRateOption,
		prMsduInfo->fgIsTXDTemplateValid, prMsduInfo->ucMacHeaderLength,
		prMsduInfo->ucLlcLength, prMsduInfo->u2FrameLength,
		prMsduInfo->aucEthDestAddr, prMsduInfo->u4PageCount,
		prMsduInfo->ucTxSeqNum, prMsduInfo->ucPID
		);

	DBGLOG(NIC, INFO, TEMP_LINE4,
		prMsduInfo->ucWmmQueSet, prMsduInfo->pfTxDoneHandler,
		prMsduInfo->u4TxDoneTag, prMsduInfo->ucPktType
		);
#undef TEMP_LINE1
#undef TEMP_LINE2
#undef TEMP_LINE3
#undef TEMP_LINE4

	/* dump txd */
	if (prMsduInfo->ucPacketType == TX_PACKET_TYPE_DATA
		&& prMsduInfo->prPacket) {
		kalGetPacketBuf(prMsduInfo->prPacket, &pucData);
		DBGLOG_MEM8(NIC, INFO, pucData, 64);
	}
}

#if (CFG_COALESCING_INTERRUPT == 1)
uint32_t nicSetCoalescingInt(struct ADAPTER *prAdapter,
			u_int8_t fgPktThEn, u_int8_t fgTmrThEn)
{
	struct CMD_PF_CF_COALESCING_INT rCmdSetCoalescingInt;
	struct CMD_PF_CF_COALESCING_INT *prCmdSetCoalescingInt;

	ASSERT(prAdapter);
	prCmdSetCoalescingInt = &rCmdSetCoalescingInt;
	prCmdSetCoalescingInt->ucPktThEn = fgPktThEn;
	prCmdSetCoalescingInt->ucTmrThEn = fgTmrThEn;
	prCmdSetCoalescingInt->u2MaxPkt =
		prAdapter->rWifiVar.u2CoalescingIntMaxPk;
	prCmdSetCoalescingInt->u2MaxTime =
		prAdapter->rWifiVar.u2CoalescingIntMaxTime;
	prCmdSetCoalescingInt->u2FilterMask =
		prAdapter->rWifiVar.u2CoalescingIntuFilterMask;

	return wlanSendSetQueryCmd(prAdapter,
	    CMD_ID_PF_CF_COALESCING_INT,
	    TRUE,
	    FALSE,
	    FALSE,
	    NULL,
	    NULL,
	    sizeof(struct CMD_PF_CF_COALESCING_INT),
	    (uint8_t *)prCmdSetCoalescingInt, NULL, 0);
}
#endif

uint8_t nicGetActiveTspec(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	uint8_t ucActivedTspec = 0;

	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		ucActivedTspec = wmmHasActiveTspec(
			aisGetWMMInfo(prAdapter, ucBssIndex));
	}

	return ucActivedTspec;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void nicEtherMAT_M2L(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2ForceTxWlanId)
{
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	struct ETH_FRAME *prFrameHeader;
	uint8_t *prSrcMac, *prDestMac;

	if ((prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_FORCE_LINK) == 0) {
		DBGLOG(TX, WARN, "Only support force link packets.\n");
		return;
	}

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		secGetStaIdxByWlanIdx(prAdapter, u2ForceTxWlanId));
	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

	kalGetPacketBuf(prMsduInfo->prPacket,
			(uint8_t **)&prFrameHeader);
	prSrcMac = prFrameHeader->aucSrcAddr;
	prDestMac = prFrameHeader->aucDestAddr;

	if (UNEQUAL_MAC_ADDR(prSrcMac, prBssInfo->aucOwnMacAddr)) {
		DBGLOG(TX, TRACE,
			"Change Src addr from [" MACSTR " to " MACSTR "]\n",
			MAC2STR(prSrcMac),
			MAC2STR(prBssInfo->aucOwnMacAddr));
		prMsduInfo->ucBssIndex = prBssInfo->ucBssIndex;
		COPY_MAC_ADDR(prSrcMac, prBssInfo->aucOwnMacAddr);
	}

	if (UNEQUAL_MAC_ADDR(prDestMac, prStaRec->aucMacAddr)) {
		DBGLOG(TX, TRACE,
			"Change Dest addr from [" MACSTR " to " MACSTR "]\n",
			MAC2STR(prDestMac),
			MAC2STR(prStaRec->aucMacAddr));
		prMsduInfo->ucStaRecIndex = prStaRec->ucIndex;
		COPY_MAC_ADDR(prDestMac, prStaRec->aucMacAddr);
	}
}

void nicMgmtMAT_M2L(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t ucGroupMldId,
	uint8_t ucForceTxWlanId)
{
	struct WLAN_MAC_HEADER *prWlanHdr;
	struct MLD_BSS_INFO *prMldBss;
	struct MLD_STA_RECORD *prMldSta;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	prWlanHdr = (struct WLAN_MAC_HEADER *)
		((uintptr_t) prMsduInfo->prPacket +
		MAC_TX_RESERVED_FIELD);

	prMldBss = mldBssGetByIdx(prAdapter, ucGroupMldId);
	prMldSta = mldStarecGetByMldAddr(prAdapter, prMldBss,
					 prWlanHdr->aucAddr1);
	if (!prMldBss || !prMldSta)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter,
		secGetStaIdxByWlanIdx(prAdapter,
			ucForceTxWlanId));
	if (!prStaRec) {
		DBGLOG(NIC, WARN,
			"NULL prStaRec, u2ForceTxWlanId=%u\n",
			ucForceTxWlanId);
		return;
	} else if (prStaRec->ucMldStaIndex != prMldSta->ucIdx) {
		DBGLOG(NIC, WARN,
			"mld sta idx mismatch, %u %u\n",
			prStaRec->ucMldStaIndex,
			prMldSta->ucIdx);
		return;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(NIC, WARN,
			"NULL prBssInfo, ucBssIndex=%u\n",
			prStaRec->ucBssIndex);
		return;
	}

	DBGLOG(NIC, TRACE,
		"Before A1/A2/A3 ["MACSTR"/"MACSTR"/"MACSTR"]\n",
		MAC2STR(prWlanHdr->aucAddr1),
		MAC2STR(prWlanHdr->aucAddr2),
		MAC2STR(prWlanHdr->aucAddr3));

	COPY_MAC_ADDR(prWlanHdr->aucAddr1, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prWlanHdr->aucAddr2, prBssInfo->aucOwnMacAddr);
	if (IS_BSS_APGO(prBssInfo))
		COPY_MAC_ADDR(prWlanHdr->aucAddr3,
			prBssInfo->aucBSSID);
	else
		COPY_MAC_ADDR(prWlanHdr->aucAddr3,
			prStaRec->aucMacAddr);

	DBGLOG(NIC, TRACE,
		"After A1/A2/A3 ["MACSTR"/"MACSTR"/"MACSTR"]\n",
		MAC2STR(prWlanHdr->aucAddr1),
		MAC2STR(prWlanHdr->aucAddr2),
		MAC2STR(prWlanHdr->aucAddr3));
}

void nicMgmtMAT_L2M(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct WLAN_MAC_HEADER *prWlanHdr;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct MLD_BSS_INFO *prMldBss;
	struct MLD_STA_RECORD *prMldSta;

	if (!prAdapter || !prSwRfb)
		return;

	prWlanHdr = (struct WLAN_MAC_HEADER *)prSwRfb->pvHeader;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(NIC, WARN,
			"NULL prStaRec, ucStaRecIdx=%u\n",
			prSwRfb->ucStaRecIdx);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	prMldBss = mldBssGetByBss(prAdapter, prBssInfo);
	prMldSta = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldBss || !prMldSta)
		return;

	DBGLOG(NIC, TRACE,
		"Before A1["MACSTR"] A2["MACSTR"] A3["MACSTR"]\n",
		MAC2STR(prWlanHdr->aucAddr1),
		MAC2STR(prWlanHdr->aucAddr2),
		MAC2STR(prWlanHdr->aucAddr3));

	COPY_MAC_ADDR(prWlanHdr->aucAddr1, prMldBss->aucOwnMldAddr);
	COPY_MAC_ADDR(prWlanHdr->aucAddr2, prMldSta->aucPeerMldAddr);
	COPY_MAC_ADDR(prWlanHdr->aucAddr3, prMldBss->aucOwnMldAddr);

	DBGLOG(NIC, TRACE,
		"After A1["MACSTR"] A2["MACSTR"] A3["MACSTR"]\n",
		MAC2STR(prWlanHdr->aucAddr1),
		MAC2STR(prWlanHdr->aucAddr2),
		MAC2STR(prWlanHdr->aucAddr3));
}
#endif
