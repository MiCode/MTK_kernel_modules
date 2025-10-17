// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "cnm_mem.c"
 *    \brief  This file contain the management function of packet buffers and
 *    generic memory alloc/free functioin for mailbox message.
 *
 *    A data packet has a fixed size of buffer, but a management
 *    packet can be equipped with a variable size of buffer.
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

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

static uint8_t *apucStaRecType[STA_TYPE_INDEX_NUM] = {
	(uint8_t *) "LEGACY",
	(uint8_t *) "P2P",
	(uint8_t *) "BOW",
#if CFG_SUPPORT_NAN
	(uint8_t *) "NAN",
#endif
};

static uint8_t *apucStaRecRole[STA_ROLE_INDEX_NUM] = {
	(uint8_t *) "ADHOC",
	(uint8_t *) "CLIENT",
	(uint8_t *) "AP",
	(uint8_t *) "DLS"
};

#if CFG_SUPPORT_TDLS
/* The list of valid data rates. */
const uint8_t aucValidDataRate[] = {
	RATE_1M,		/* RATE_1M_INDEX = 0 */
	RATE_2M,		/* RATE_2M_INDEX */
	RATE_5_5M,		/* RATE_5_5M_INDEX */
	RATE_11M,		/* RATE_11M_INDEX */
	RATE_22M,		/* RATE_22M_INDEX */
	RATE_33M,		/* RATE_33M_INDEX */
	RATE_6M,		/* RATE_6M_INDEX */
	RATE_9M,		/* RATE_9M_INDEX */
	RATE_12M,		/* RATE_12M_INDEX */
	RATE_18M,		/* RATE_18M_INDEX */
	RATE_24M,		/* RATE_24M_INDEX */
	RATE_36M,		/* RATE_36M_INDEX */
	RATE_48M,		/* RATE_48M_INDEX */
	RATE_54M,		/* RATE_54M_INDEX */
	RATE_VHT_PHY,		/* RATE_VHT_PHY_INDEX */
	RATE_HT_PHY		/* RATE_HT_PHY_INDEX */
};
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void cnmStaRoutinesForAbort(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

static void cnmStaRecHandleEventPkt(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

static void
cnmStaSendRemoveCmd(struct ADAPTER *prAdapter,
	enum ENUM_STA_REC_CMD_ACTION eActionType, uint8_t ucStaRecIndex,
	uint8_t ucBssIndex);

#if (CFG_SUPPORT_802_11AX == 1)
static void cnmStaRecCmdHeContentFill(
	struct STA_RECORD *prStaRec,
	struct CMD_UPDATE_STA_RECORD *prCmdContent);
#endif
#if (CFG_SUPPORT_802_11BE == 1)
static void cnmStaRecCmdEhtContentFill(
	struct STA_RECORD *prStaRec,
	struct CMD_UPDATE_STA_RECORD *prCmdContent);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
struct MSDU_INFO *cnmPktAllocWrapper(struct ADAPTER *prAdapter,
	uint32_t u4Length, uint8_t *pucStr)
{
	struct MSDU_INFO *prMsduInfo;

#if CFG_DBG_MGT_BUF
	prMsduInfo = cnmPktAllocX(prAdapter, u4Length, pucStr);
#else
	prMsduInfo = cnmPktAlloc(prAdapter, u4Length);
#endif

	log_dbg(MEM, LOUD, "Alloc MSDU_INFO[0x%p] by [%s]\n",
		prMsduInfo, pucStr);

	return prMsduInfo;
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
void cnmPktFreeWrapper(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo,
	uint8_t *pucStr)
{
	log_dbg(MEM, LOUD, "Free MSDU_INFO[0x%p] by [%s]\n",
		prMsduInfo, pucStr);

	cnmPktFree(prAdapter, prMsduInfo);
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
#if CFG_DBG_MGT_BUF
struct MSDU_INFO *cnmPktAllocX(struct ADAPTER *prAdapter, uint32_t u4Length,
	uint8_t *fileAndLine)
#else
struct MSDU_INFO *cnmPktAlloc(struct ADAPTER *prAdapter, uint32_t u4Length)
#endif
{
	struct MSDU_INFO *prMsduInfo = NULL;
	struct QUE *prQueList;
	uint32_t u4TxHeadRoomSize = 0;
	uint8_t *prHead;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	prQueList = &prAdapter->rTxCtrl.rFreeMsduInfoList;

	/* Get a free MSDU_INFO_T */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
	QUEUE_REMOVE_HEAD(prQueList, prMsduInfo, struct MSDU_INFO *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);

	if (!prMsduInfo)
		goto exit;

	kalMemZero(prMsduInfo, sizeof(struct MSDU_INFO));

	if (u4Length) {
		u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH +
			prAdapter->chip_info->txd_append_size;
#if CFG_DBG_MGT_BUF
		prHead = cnmMemAllocX(prAdapter,
			RAM_TYPE_BUF, u4Length + u4TxHeadRoomSize,
			fileAndLine);
#else
		prHead = cnmMemAlloc(prAdapter,
			RAM_TYPE_BUF, u4Length + u4TxHeadRoomSize);
#endif
		if (prHead == NULL) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TX_MSDU_INFO_LIST);
			QUEUE_INSERT_TAIL(prQueList,
				&prMsduInfo->rQueEntry);
			KAL_RELEASE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_TX_MSDU_INFO_LIST);
			prMsduInfo = NULL;
			goto exit;
		}
		prMsduInfo->prHead = prHead;
		prMsduInfo->prPacket = (uint8_t *)
			((uintptr_t)prMsduInfo->prHead +
			u4TxHeadRoomSize);
		prMsduInfo->aucTxDescBuffer = prMsduInfo->prHead;
		prMsduInfo->eSrc = TX_PACKET_MGMT;
		prMsduInfo->u4Option = 0;
		prMsduInfo->ucControlFlag = 0;
		prMsduInfo->fgMgmtUseDataQ = FALSE;
		prMsduInfo->fgIsPacketSkb = FALSE;
	}

	prMsduInfo->pfHifTxMsduDoneCb = nicHifTxMsduDoneCb;

exit:
	if (prMsduInfo == NULL) {
#if CFG_DBG_MGT_BUF
		DBGLOG_LIMITED(MEM, WARN,
			"MgtDesc#=%u rMgtBufInfo: alloc#=%u, free#=%u, null#=%u\n",
			prQueList->u4NumElem,
			prAdapter->rMgtBufInfo.u4AllocCount,
			prAdapter->rMgtBufInfo.u4FreeCount,
			prAdapter->rMgtBufInfo.u4AllocNullCount);
#else /* CFG_DBG_MGT_BUF */
		DBGLOG_LIMITED(MEM, WARN, "MgtDesc#=%u\n",
			prQueList->u4NumElem);
#endif /* CFG_DBG_MGT_BUF */
	}

	return prMsduInfo;
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
void cnmPktFree(struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo)
{
	struct QUE *prQueList;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (!prMsduInfo)
		return;

	prQueList = &prAdapter->rTxCtrl.rFreeMsduInfoList;

	if (prMsduInfo->prHead) {
		cnmMemFree(prAdapter, prMsduInfo->prHead);
		prMsduInfo->prHead = NULL;
		prMsduInfo->prPacket = NULL;
		prMsduInfo->prTxP = NULL;
		prMsduInfo->aucTxDescBuffer = NULL;
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
	QUEUE_INSERT_TAIL(prQueList, &prMsduInfo->rQueEntry);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_MSDU_INFO_LIST);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to initial the MGMT/MSG memory pool.
 *
 * \param (none)
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmMemInit(struct ADAPTER *prAdapter)
{
	struct BUF_INFO *prBufInfo;

	/* Initialize Management buffer pool */
	prBufInfo = &prAdapter->rMgtBufInfo;
	kalMemZero(prBufInfo, sizeof(prAdapter->rMgtBufInfo));
	prBufInfo->pucBuf = prAdapter->pucMgtBufCached;

	/* Setup available memory blocks. 1 indicates FREE */
	prBufInfo->rFreeBlocksBitmap = (uint32_t) BITS(0,
		MAX_NUM_OF_BUF_BLOCKS - 1);

	/* Initialize Message buffer pool */
	prBufInfo = &prAdapter->rMsgBufInfo;
	kalMemZero(prBufInfo, sizeof(prAdapter->rMsgBufInfo));
	prBufInfo->pucBuf = &prAdapter->aucMsgBuf[0];

	/* Setup available memory blocks. 1 indicates FREE */
	prBufInfo->rFreeBlocksBitmap = (uint32_t) BITS(0,
		MAX_NUM_OF_BUF_BLOCKS - 1);

	return;

}	/* end of cnmMemInit() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Allocate MGMT/MSG memory pool.
 *
 * \param[in] eRamType       Target RAM type.
 *                           TCM blk_sz= 16bytes, BUF blk_sz= 256bytes
 * \param[in] u4Length       Length of the buffer to allocate.
 *
 * \retval !NULL    Pointer to the start address of allocated memory.
 * \retval NULL     Fail to allocat memory
 */
/*----------------------------------------------------------------------------*/
#if CFG_DBG_MGT_BUF
void *cnmMemAllocX(struct ADAPTER *prAdapter, enum ENUM_RAM_TYPE eRamType,
	uint32_t u4Length, uint8_t *fileAndLine)
#else
void *cnmMemAlloc(struct ADAPTER *prAdapter, enum ENUM_RAM_TYPE eRamType,
	uint32_t u4Length)
#endif
{
	struct BUF_INFO *prBufInfo;
	uint32_t rRequiredBitmap;
	uint32_t u4BlockNum;
	uint32_t i, u4BlkSzInPower;
	void *pvMemory;
	enum ENUM_SPIN_LOCK_CATEGORY_E eLockBufCat;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (u4Length == 0) {
		log_dbg(MEM, WARN,
			"%s: Length to be allocated is ZERO, skip!\n",
			__func__);
		return NULL;
	}

	if (eRamType == RAM_TYPE_MSG && u4Length <= 256) {
		prBufInfo = &prAdapter->rMsgBufInfo;
		u4BlkSzInPower = MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2;

		u4BlockNum = (u4Length + MSG_BUF_BLOCK_SIZE - 1)
			>> MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2;

		ASSERT(u4BlockNum <= MAX_NUM_OF_BUF_BLOCKS);
	} else {
		eRamType = RAM_TYPE_BUF;

		prBufInfo = &prAdapter->rMgtBufInfo;
		u4BlkSzInPower = MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2;

		u4BlockNum = (u4Length + MGT_BUF_BLOCK_SIZE - 1)
			>> MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2;

		ASSERT(u4BlockNum <= MAX_NUM_OF_BUF_BLOCKS);
	}

	if (eRamType == RAM_TYPE_MSG)
		eLockBufCat = SPIN_LOCK_MSG_BUF;
	else
		eLockBufCat = SPIN_LOCK_MGT_BUF;

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, eLockBufCat);

#if CFG_DBG_MGT_BUF
	prBufInfo->u4AllocCount++;
#endif

	if ((u4BlockNum > 0) && (u4BlockNum <= MAX_NUM_OF_BUF_BLOCKS)) {

		/* Convert number of block into bit cluster */
		rRequiredBitmap = BITS(0, u4BlockNum - 1);

		for (i = 0; i <= (MAX_NUM_OF_BUF_BLOCKS - u4BlockNum); i++) {

			/* Have available memory blocks */
			if ((prBufInfo->rFreeBlocksBitmap & rRequiredBitmap)
				== rRequiredBitmap) {

				/* Clear corresponding bits of allocated
				 * memory blocks
				 */
				prBufInfo->rFreeBlocksBitmap
					&= ~rRequiredBitmap;

				/* Store how many blocks be allocated */
				prBufInfo->aucAllocatedBlockNum[i]
					= (uint8_t) u4BlockNum;

				KAL_RELEASE_SPIN_LOCK(prAdapter, eLockBufCat);

				/* Return the start address of
				 * allocated memory
				 */
				pvMemory = (void *) (prBufInfo->pucBuf
					+ (i << u4BlkSzInPower));
				goto exit;
			}

			rRequiredBitmap <<= 1;
		}
	}

#if CFG_DBG_MGT_BUF
	prBufInfo->u4AllocNullCount++;
#endif

	/* kalMemAlloc() shall not included in spin_lock */
	KAL_RELEASE_SPIN_LOCK(prAdapter, eLockBufCat);

#ifdef LINUX
#if CFG_DBG_MGT_BUF
	pvMemory = kalMemAlloc(u4Length + sizeof(struct MEM_TRACK),
		PHY_MEM_TYPE);
	if (pvMemory) {
		struct MEM_TRACK *prMemTrack = pvMemory;

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MGT_BUF);
		LINK_INSERT_TAIL(
			&prAdapter->rMemTrackLink, &prMemTrack->rLinkEntry);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MGT_BUF);
		prMemTrack->pucFileAndLine = fileAndLine;
		prMemTrack->ucCmdId = 0;
		prMemTrack->ucWhere = 0;
		pvMemory = prMemTrack->aucData;
	} else {
		DBGLOG(MEM, WARN,
			"kalMemAlloc fail, type: %d sz: %lu\n",
			eRamType, u4Length + sizeof(struct MEM_TRACK));
	}
#else
	pvMemory = kalMemAlloc(u4Length, PHY_MEM_TYPE);
	if (!pvMemory)
		DBGLOG(MEM, WARN,
			"kalMemAlloc fail, type: %d sz: %u\n",
			eRamType,
			u4Length);
#endif
#else
	/*
	 * For Windows, it is not supported because of no size argument
	 * in windows cx it supports and common part has massive allocation
	 */
	pvMemory = (void *) kalMemAlloc(u4Length, PHY_MEM_TYPE);
	if (!pvMemory)
		DBGLOG(MEM, WARN,
			"kalMemAlloc fail, type: %d sz: %u\n",
			eRamType,
			u4Length);
#endif

#if CFG_DBG_MGT_BUF
	if (pvMemory)
		GLUE_INC_REF_CNT(prAdapter->u4MemAllocDynamicCount);
#endif

exit:
	if (pvMemory)
		kalMemZero(pvMemory, u4Length);

	return pvMemory;
}	/* end of cnmMemAlloc() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Release memory to MGT/MSG memory pool.
 *
 * \param pucMemory  Start address of previous allocated memory
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmMemFree(struct ADAPTER *prAdapter, void *pvMemory)
{
	struct BUF_INFO *prBufInfo;
	uint32_t u4BlockIndex;
	uint32_t rAllocatedBlocksBitmap;
	enum ENUM_RAM_TYPE eRamType;
	enum ENUM_SPIN_LOCK_CATEGORY_E eLockBufCat;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (!pvMemory)
		return;

	/* Judge it belongs to which RAM type */
	if (((uintptr_t) pvMemory
		>= (uintptr_t)&prAdapter->aucMsgBuf[0])
		&& ((uintptr_t) pvMemory
		<= (uintptr_t)&prAdapter->aucMsgBuf[MSG_BUFFER_SIZE - 1])) {

		prBufInfo = &prAdapter->rMsgBufInfo;
		u4BlockIndex = ((uintptr_t) pvMemory
			- (uintptr_t) prBufInfo->pucBuf)
			>> MSG_BUF_BLOCK_SIZE_IN_POWER_OF_2;
		eRamType = RAM_TYPE_MSG;
	} else if (((uintptr_t) pvMemory
		>= (uintptr_t) prAdapter->pucMgtBufCached)
		&& ((uintptr_t) pvMemory
		<= ((uintptr_t) prAdapter->pucMgtBufCached
		 + MGT_BUFFER_SIZE - 1))) {
		prBufInfo = &prAdapter->rMgtBufInfo;
		u4BlockIndex = ((uintptr_t) pvMemory
			- (uintptr_t) prBufInfo->pucBuf)
			>> MGT_BUF_BLOCK_SIZE_IN_POWER_OF_2;
		eRamType = RAM_TYPE_BUF;
	} else {
#ifdef LINUX
#if CFG_DBG_MGT_BUF
		struct MEM_TRACK *prTrack =
			CONTAINER_OF((uint8_t (*)[])pvMemory,
				     struct MEM_TRACK, aucData);

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MGT_BUF);
		LINK_REMOVE_KNOWN_ENTRY(
			&prAdapter->rMemTrackLink, &prTrack->rLinkEntry);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MGT_BUF);
		kalMemFree(prTrack, PHY_MEM_TYPE, 0);
#else
		/* For Linux, it is supported because size is not needed */
		kalMemFree(pvMemory, PHY_MEM_TYPE, 0);
#endif
#else
		/*
		 * For Windows, it is not supported because of no size argument
		 * in windows cx it supports and
		 * common part has massive allocation
		 */
		kalMemFree(pvMemory, PHY_MEM_TYPE, 0);
		/* ASSERT(0); */
#endif

#if CFG_DBG_MGT_BUF
		GLUE_INC_REF_CNT(prAdapter->u4MemFreeDynamicCount);
#endif
		return;
	}

	if (u4BlockIndex >= MAX_NUM_OF_BUF_BLOCKS) {
		DBGLOG(MEM, ERROR,
			"u4BlockIndex %d>=[%d]\n",
			u4BlockIndex, MAX_NUM_OF_BUF_BLOCKS);
		ASSERT(0);
		return;
	}

	if (eRamType == RAM_TYPE_MSG)
		eLockBufCat = SPIN_LOCK_MSG_BUF;
	else
		eLockBufCat = SPIN_LOCK_MGT_BUF;

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, eLockBufCat);

#if CFG_DBG_MGT_BUF
	prBufInfo->u4FreeCount++;
#endif

	/* Convert number of block into bit cluster */
	ASSERT(prBufInfo->aucAllocatedBlockNum[u4BlockIndex] > 0);

	rAllocatedBlocksBitmap
		= BITS(0, prBufInfo->aucAllocatedBlockNum[u4BlockIndex] - 1);
	rAllocatedBlocksBitmap <<= u4BlockIndex;

	/* Clear saved block count for this memory segment */
	prBufInfo->aucAllocatedBlockNum[u4BlockIndex] = 0;

	/* Set corresponding bit of released memory block */
	prBufInfo->rFreeBlocksBitmap |= rAllocatedBlocksBitmap;

	KAL_RELEASE_SPIN_LOCK(prAdapter, eLockBufCat);

	return;

}	/* end of cnmMemFree() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void cnmStaRecInit(struct ADAPTER *prAdapter)
{
	struct STA_RECORD *prStaRec;
	uint16_t i;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];

		prStaRec->ucIndex = (uint8_t) i;
		prStaRec->fgIsInUse = FALSE;
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
struct STA_RECORD *cnmStaRecAlloc(struct ADAPTER *prAdapter,
	enum ENUM_STA_TYPE eStaType, uint8_t ucBssIndex, uint8_t *pucMacAddr)
{
	struct STA_RECORD *prStaRec = NULL;
	uint16_t i, k;
	const uint8_t offset = pucMacAddr[5] % CFG_STA_REC_NUM;
	uint8_t idx;

	ASSERT(prAdapter);

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		/* A naive hash to find a free starec from offset */
		idx = (i + offset) % CFG_STA_REC_NUM;
		prStaRec = &prAdapter->arStaRec[idx];

		if (!prStaRec->fgIsInUse) {
			kalMemZero(prStaRec, sizeof(struct STA_RECORD));
			prStaRec->ucIndex = (uint8_t) idx;
			prStaRec->ucBssIndex = ucBssIndex;
			prStaRec->fgIsInUse = TRUE;
			prStaRec->eStaType = eStaType;
			prStaRec->u2StatusCode = STATUS_CODE_INVALID;

			/* Initialize the SN caches for duplicate detection */
			for (k = 0; k < TID_NUM + 1; k++) {
				prStaRec->au2CachedSeqCtrl[k] = 0xFFFF;
				prStaRec->afgIsIgnoreAmsduDuplicate[k] = FALSE;
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
				prStaRec->au2AmsduInvalidSN[k] = 0xFFFF;
				prStaRec->afgIsAmsduInvalid[k] = FALSE;
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
			}

			/* Initialize SW TX queues in STA_REC */
			for (k = 0; k < STA_WAIT_QUEUE_NUM; k++)
				LINK_INITIALIZE(&prStaRec->arStaWaitQueue[k]);

			LINK_INITIALIZE(&prStaRec->rMscsMonitorList);
			LINK_INITIALIZE(&prStaRec->rMscsTcpMonitorList);
			DBGLOG(MEM, WARN,
				"LINK_INITIALIZE list=%p, BssIdx=%d, StaRecIdx=%d\n",
				&prStaRec->rMscsMonitorList, ucBssIndex, idx);
#if CFG_ENABLE_PER_STA_STATISTICS && CFG_ENABLE_PKT_LIFETIME_PROFILE
			prStaRec->u4TotalTxPktsNumber = 0;
			prStaRec->u4TotalTxPktsTime = 0;
			prStaRec->u4TotalRxPktsNumber = 0;
			prStaRec->u4MaxTxPktsTime = 0;
#endif
#if CFG_AP_80211KVR_INTERFACE
			prStaRec->u8TotalTxBytes = 0;
			prStaRec->u8TotalRxBytes = 0;
			prStaRec->u8TotalRxPkts = 0;
			prStaRec->u8GetDataRateTime = 0;
#endif

			for (k = 0; k < NUM_OF_PER_STA_TX_QUEUES; k++) {
				QUEUE_INITIALIZE(
					&prStaRec->arTxQueue[k]);
				QUEUE_INITIALIZE(
					&prStaRec->arPendingTxQueue[k]);
				/* Default should be no-TX.
				 * Switch when allow to TX.
				 */
				prStaRec->aprTargetQueue[k]
					= &prStaRec->arPendingTxQueue[k];
			}

			prStaRec->ucAmsduEnBitmap = 0;
			prStaRec->ucMaxMpduCount = 0;
			prStaRec->u4MaxMpduLen = 0;
			prStaRec->u4MinMpduLen = 0;

			qosMapSetInit(prStaRec);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			prStaRec->ucMldStaIndex = MLD_GROUP_NONE;
			prStaRec->ucULTidBitmap = 0xff;
			prStaRec->ucDLTidBitmap = 0xff;
#endif

			/* Default QM RX BA timeout */
			prStaRec->u4QmRxBaMissTimeout =
				prAdapter->rWifiVar.u4BaMissTimeoutMs;

			break;
		}
	}

	/* Sync to chip to allocate WTBL resource */
	if (i < CFG_STA_REC_NUM) {
		COPY_MAC_ADDR(prStaRec->aucMacAddr, pucMacAddr);
		if (secPrivacySeekForEntry(prAdapter, prStaRec)) {
#if CFG_SUPPORT_LIMITED_PKT_PID
			nicTxInitPktPID(prAdapter, prStaRec->ucWlanIndex);
#endif /* CFG_SUPPORT_LIMITED_PKT_PID */
		}
#if DBG
		else {
			prStaRec->fgIsInUse = FALSE;
			prStaRec = NULL;
			ASSERT(FALSE);
		}
#endif
	} else {
		prStaRec = NULL;
	}

	return prStaRec;
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
void cnmStaRecFree(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec)
{
	ASSERT(prAdapter);

	if (!prStaRec)
		return;

	log_dbg(CNM, INFO, "BssIdx=%d, StaRecIdx=%d, InUse=%d\n",
		prStaRec->ucBssIndex, prStaRec->ucIndex, prStaRec->fgIsInUse);

	if (prStaRec->fgIsInUse) {
#if CFG_SUPPORT_802_11W
		if (timerPendingTimer(&(prStaRec->rPmfCfg.rSAQueryTimer)))
			cnmTimerStopTimer(prAdapter,
				&(prStaRec->rPmfCfg.rSAQueryTimer));
#endif

		nicFreePendingTxMsduInfo(prAdapter, prStaRec->ucWlanIndex,
				MSDU_REMOVE_BY_WLAN_INDEX);

		cnmStaSendRemoveCmd(prAdapter, STA_REC_CMD_ACTION_STA,
			prStaRec->ucIndex, prStaRec->ucBssIndex);

		cnmStaRoutinesForAbort(prAdapter, prStaRec);
	} else {
		log_dbg(CNM, ERROR, "prStaRec is not in use\n");
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
static void cnmStaRoutinesForAbort(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	ASSERT(prAdapter);

	if (!prStaRec)
		return;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldStarecUnregister(prAdapter, prStaRec);
#endif

	/* To do: free related resources, e.g. timers, buffers, etc */
	cnmTimerStopTimer(prAdapter, &prStaRec->rTxReqDoneOrRxRespTimer);
	cnmTimerStopTimer(prAdapter, &prStaRec->rDeauthTxDoneTimer);
#if CFG_AP_80211V_SUPPORT
	cnmTimerStopTimer(prAdapter, &prStaRec->rBTMReqDisassocTimer);
#endif /* CFG_AP_80211V_SUPPORT */
	prStaRec->fgTransmitKeyExist = FALSE;

	prStaRec->fgSetPwrMgtBit = FALSE;

	if (prStaRec->pucAssocReqIe) {
		kalMemFree(prStaRec->pucAssocReqIe,
			VIR_MEM_TYPE, prStaRec->u2AssocReqIeLen);
		prStaRec->pucAssocReqIe = NULL;
		prStaRec->u2AssocReqIeLen = 0;
	}

	qmDeactivateStaRec(prAdapter, prStaRec);

	/* Update the driver part table setting */
	secPrivacyFreeSta(prAdapter, prStaRec);

	prStaRec->fgIsInUse = FALSE;

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (prStaRec->prFilsInfo) {
		kalMemZFree(prStaRec->prFilsInfo, VIR_MEM_TYPE,
			sizeof(struct FILS_INFO));
		prStaRec->prFilsInfo = NULL;
	}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
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
void cnmStaFreeAllStaByNetwork(struct ADAPTER *prAdapter, uint8_t ucBssIndex,
	uint8_t ucStaRecIndexExcluded)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint16_t i;
	enum ENUM_STA_REC_CMD_ACTION eAction;

	log_dbg(CNM, INFO, "BssIdx=%d, StaRecIndexExcluded=%d\n",
		ucBssIndex, ucStaRecIndexExcluded);

	if (ucBssIndex >= prAdapter->ucSwBssIdNum)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (ucStaRecIndexExcluded < CFG_STA_REC_NUM)
		eAction = STA_REC_CMD_ACTION_BSS_EXCLUDE_STA;
	else
		eAction = STA_REC_CMD_ACTION_BSS;

	cnmStaSendRemoveCmd(prAdapter,
		eAction,
		ucStaRecIndexExcluded, ucBssIndex);

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = (struct STA_RECORD *) &prAdapter->arStaRec[i];

		if (prStaRec->fgIsInUse && prStaRec->ucBssIndex == ucBssIndex
			&& i != ucStaRecIndexExcluded)
			cnmStaRoutinesForAbort(prAdapter, prStaRec);
	}	/* end of for loop */

#if CFG_ENABLE_WIFI_DIRECT
	/* To do: Confirm if it is invoked here or other location, but it should
	 *        be invoked after state sync of STA_REC
	 * Update system operation parameters for AP mode
	 */
	if (prAdapter->fgIsP2PRegistered &&
	    prBssInfo &&
	    prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
		rlmUpdateParamsForAP(prAdapter, prBssInfo, FALSE);
	}
#endif
}

struct STA_RECORD *cnmGetStaRecByIndexWithoutInUseCheck(
	struct ADAPTER *prAdapter,
	uint8_t ucIndex)
{
	struct STA_RECORD *prStaRec;

	ASSERT(prAdapter);

	if (ucIndex < CFG_STA_REC_NUM)
		prStaRec = &prAdapter->arStaRec[ucIndex];
	else
		prStaRec = NULL;

	return prStaRec;
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
struct STA_RECORD *cnmGetStaRecByIndex(struct ADAPTER *prAdapter,
	uint8_t ucIndex)
{
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndexWithoutInUseCheck(prAdapter, ucIndex);

	if (prStaRec && prStaRec->fgIsInUse == FALSE)
		prStaRec = NULL;

	return prStaRec;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get STA_RECORD_T by Peer Wlan Index
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
struct STA_RECORD *cnmGetStaRecByWlanIndex(struct ADAPTER *prAdapter,
	uint8_t ucWlanIndex)
{
	return cnmGetStaRecByIndex(prAdapter,
		secGetStaIdxByWlanIdx(prAdapter, ucWlanIndex));
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get STA_RECORD_T by Peer MAC Address(Usually TA).
 *
 * @param[in] ucBssIndex	  Given BSS index, or
 *				  ANY_BSS_INDEX if don't need to match BSS index
 * @param[in] pucPeerMacAddr      Given Peer MAC Address.
 *
 * @retval   Pointer to STA_RECORD_T, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct STA_RECORD *cnmGetStaRecByAddress(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, const uint8_t *pucPeerMacAddr)
{
	struct STA_RECORD *prStaRec = NULL;
	uint16_t i;
	uint16_t offset;
	uint8_t ucLastByte;
	uint8_t idx;

	ASSERT(prAdapter);

	if (!pucPeerMacAddr)
		return NULL;

	/* A naive hash to find a match starec by peer MAC address */
	ucLastByte = pucPeerMacAddr[5];
	offset = ucLastByte % CFG_STA_REC_NUM;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		idx = (i + offset) % CFG_STA_REC_NUM;
		prStaRec = &prAdapter->arStaRec[idx];

		if (prStaRec->fgIsInUse &&
		    (ucBssIndex == ANY_BSS_INDEX ||
		     prStaRec->ucBssIndex == ucBssIndex) &&
		    prStaRec->aucMacAddr[5] == ucLastByte &&
		    EQUAL_MAC_ADDR(prStaRec->aucMacAddr, pucPeerMacAddr)) {
			break;
		}
	}

	return (i < CFG_STA_REC_NUM) ? prStaRec : NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function will change the ucStaState of STA_RECORD_T and also do
 *        event indication to HOST to sync the STA_RECORD_T in driver.
 *
 * @param[in] prStaRec       Pointer to the STA_RECORD_T
 * @param[in] u4NewState     New STATE to change.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmStaRecChangeState(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec
	, uint8_t ucNewState)
{
	u_int8_t fgNeedResp;

	if (!prAdapter)
		return;

	if (!prStaRec) {
		log_dbg(MEM, WARN, "%s: StaRec is NULL, skip!\n", __func__);
		return;
	}

	if (!prStaRec->fgIsInUse) {
		log_dbg(MEM, WARN, "%s: StaRec[%u] is not in use, skip!\n",
			__func__, prStaRec->ucIndex);
		return;
	}

	/* Do nothing when following state transitions happen,
	 * other 6 conditions should be sync to FW, including 1-->1, 3-->3
	 */
	if ((ucNewState == STA_STATE_2 && prStaRec->ucStaState != STA_STATE_3)
		|| (ucNewState == STA_STATE_1
		&& prStaRec->ucStaState == STA_STATE_2)) {
		prStaRec->ucStaState = ucNewState;
		return;
	}

#if CFG_SUPPORT_STA_INFO
	prStaRec->u4RxRetryCnt = 0;
#endif

	fgNeedResp = FALSE;
	if (ucNewState == STA_STATE_3) {
		/* secFsmEventStart(prAdapter, prStaRec); */
		if (ucNewState != prStaRec->ucStaState) {
			fgNeedResp = TRUE;
			cnmDumpStaRec(prAdapter, prStaRec->ucIndex);
		}
	} else {
		if (ucNewState != prStaRec->ucStaState
			&& prStaRec->ucStaState == STA_STATE_3)
			qmDeactivateStaRec(prAdapter, prStaRec);
		fgNeedResp = FALSE;
	}
	prStaRec->ucStaState = ucNewState;

	cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, fgNeedResp);

#if 1	/* Marked for MT6630 */
#if CFG_ENABLE_WIFI_DIRECT
	/* To do: Confirm if it is invoked here or other location, but it should
	 *        be invoked after state sync of STA_REC
	 * Update system operation parameters for AP mode
	 */
	if (IS_BSS_INDEX_VALID(prStaRec->ucBssIndex) &&
		prAdapter->fgIsP2PRegistered &&
		(IS_STA_IN_P2P(prAdapter, prStaRec))) {
		struct BSS_INFO *prBssInfo;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prStaRec->ucBssIndex);

		if (prBssInfo &&
		    prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
			rlmUpdateParamsForAP(prAdapter, prBssInfo, FALSE);
	}
#endif
#endif
}

uint8_t *cnmStaRecAuthAddr(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (mldIsMultiLinkFormed(prAdapter, prStaRec))
		return prStaRec->aucMldAddr;
#endif

	return prStaRec->aucMacAddr;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void cnmStaRecHandleEventPkt(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct EVENT_ACTIVATE_STA_REC *prEventContent;
	struct STA_RECORD *prStaRec;

	prEventContent = (struct EVENT_ACTIVATE_STA_REC *) pucEventBuf;
	prStaRec = cnmGetStaRecByIndex(prAdapter, prEventContent->ucStaRecIdx);

	if (prStaRec && prStaRec->ucStaState == STA_STATE_3 &&
		!kalMemCmp(&prStaRec->aucMacAddr[0],
			&prEventContent->aucMacAddr[0], MAC_ADDR_LEN)) {

		qmActivateStaRec(prAdapter, prStaRec);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
#if CFG_SUPPORT_TX_BF
void cnmStaSendUpdateCmd(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	struct TXBF_PFMU_STA_INFO *prTxBfPfmuStaInfo, u_int8_t fgNeedResp)
#else
void cnmStaSendUpdateCmd(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	 void *prTxBfPfmuStaInfo, u_int8_t fgNeedResp)
#endif
{
	struct CMD_UPDATE_STA_RECORD *prCmdContent;
	uint32_t rStatus;
	struct BSS_DESC *prBssDesc = NULL;

	if (!prAdapter)
		return;

	if (!prStaRec) {
		log_dbg(MEM, WARN, "%s: StaRec is NULL, skip!\n", __func__);
		return;
	}

	if (!prStaRec->fgIsInUse) {
		log_dbg(MEM, WARN, "%s: StaRec[%u] is not in use, skip!\n",
			__func__, prStaRec->ucIndex);
		return;
	}

	/* To do: come out a mechanism to limit one STA_REC sync once for AP
	 *        mode to avoid buffer empty case when many STAs are associated
	 *        simultaneously.
	 */

	/* To do: how to avoid 2 times of allocated memory. Use Stack?
	 *        One is here, the other is in wlanSendQueryCmd()
	 */
	prCmdContent = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
		sizeof(struct CMD_UPDATE_STA_RECORD));

	/* To do: exception handle */
	if (!prCmdContent) {
		log_dbg(MEM, WARN, "%s: CMD_ID_UPDATE_STA_RECORD command allocation failed\n",
			__func__);

		return;
	}

	/* Reset command buffer */
	kalMemZero(prCmdContent, sizeof(struct CMD_UPDATE_STA_RECORD));

#if CFG_SUPPORT_TX_BF
	if (prTxBfPfmuStaInfo) {
		memcpy(&prCmdContent->u2PfmuId, prTxBfPfmuStaInfo,
			sizeof(struct TXBF_PFMU_STA_INFO));
	}
#endif
	prCmdContent->ucStaIndex = prStaRec->ucIndex;
	prCmdContent->ucStaType = (uint8_t) prStaRec->eStaType;
	kalMemCopy(&prCmdContent->aucMacAddr[0], &prStaRec->aucMacAddr[0],
		MAC_ADDR_LEN);
	prCmdContent->u2AssocId = prStaRec->u2AssocId;
	prCmdContent->u2ListenInterval = prStaRec->u2ListenInterval;
	prCmdContent->ucBssIndex = prStaRec->ucBssIndex;

	prCmdContent->ucDesiredPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;
	prCmdContent->u2DesiredNonHTRateSet = prStaRec->u2DesiredNonHTRateSet;
	prCmdContent->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;
	prCmdContent->ucMcsSet = prStaRec->ucMcsSet;
	prCmdContent->ucSupMcs32 = (uint8_t) prStaRec->fgSupMcs32;
	prCmdContent->u2HwDefaultFixedRateCode
		= prStaRec->u2HwDefaultFixedRateCode;

	/* Size is SUP_MCS_RX_BITMASK_OCTET_NUM */
	kalMemCopy(prCmdContent->aucRxMcsBitmask, prStaRec->aucRxMcsBitmask,
		sizeof(prCmdContent->aucRxMcsBitmask));
	prCmdContent->u2RxHighestSupportedRate
		= prStaRec->u2RxHighestSupportedRate;
	prCmdContent->u4TxRateInfo = prStaRec->u4TxRateInfo;

	prCmdContent->u2HtCapInfo = prStaRec->u2HtCapInfo;
	prCmdContent->ucNeedResp = (uint8_t) fgNeedResp;

#if !CFG_SLT_SUPPORT
	if (prAdapter->rWifiVar.eRateSetting != FIXED_RATE_NONE) {
		/* override rate configuration */
		nicUpdateRateParams(prAdapter,
			prAdapter->rWifiVar.eRateSetting,
			&(prCmdContent->ucDesiredPhyTypeSet),
			&(prCmdContent->u2DesiredNonHTRateSet),
			&(prCmdContent->u2BSSBasicRateSet),
			&(prCmdContent->ucMcsSet),
			&(prCmdContent->ucSupMcs32),
			&(prCmdContent->u2HtCapInfo));
	}
#endif

	prCmdContent->ucIsQoS = prStaRec->fgIsQoS;
	prCmdContent->ucIsUapsdSupported = prStaRec->fgIsUapsdSupported;
	prCmdContent->ucStaState = prStaRec->ucStaState;
	prCmdContent->ucAmpduParam = prStaRec->ucAmpduParam;
	prCmdContent->u2HtExtendedCap = prStaRec->u2HtExtendedCap;
	prCmdContent->u4TxBeamformingCap = prStaRec->u4TxBeamformingCap;
	prCmdContent->ucAselCap = prStaRec->ucAselCap;
	prCmdContent->ucRCPI = prStaRec->ucRCPI;

	prCmdContent->u4VhtCapInfo = prStaRec->u4VhtCapInfo;
	prCmdContent->u2VhtRxMcsMap = prStaRec->u2VhtRxMcsMap;
	prCmdContent->u2VhtRxHighestSupportedDataRate
		= prStaRec->u2VhtRxHighestSupportedDataRate;
	prCmdContent->u2VhtTxMcsMap = prStaRec->u2VhtTxMcsMap;
	prCmdContent->u2VhtTxHighestSupportedDataRate
		= prStaRec->u2VhtTxHighestSupportedDataRate;
	prCmdContent->ucVhtOpMode = prStaRec->ucVhtOpMode;

	prCmdContent->ucUapsdAc
		= prStaRec->ucBmpTriggerAC | (prStaRec->ucBmpDeliveryAC << 4);
	prCmdContent->ucUapsdSp = prStaRec->ucUapsdSp;

	prCmdContent->ucWlanIndex = prStaRec->ucWlanIndex;
	prCmdContent->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;

	prCmdContent->ucTrafficDataType = prStaRec->ucTrafficDataType;
	prCmdContent->ucTxGfMode = prStaRec->ucTxGfMode;
	prCmdContent->ucTxSgiMode = prStaRec->ucTxSgiMode;
	prCmdContent->ucTxStbcMode = prStaRec->ucTxStbcMode;
	prCmdContent->u4FixedPhyRate = prStaRec->u4FixedPhyRate;
	prCmdContent->u2MaxLinkSpeed = prStaRec->u2MaxLinkSpeed;
	prCmdContent->u2MinLinkSpeed = prStaRec->u2MinLinkSpeed;
	prCmdContent->u4Flags = prStaRec->u4Flags;
#if CFG_SUPPORT_MTK_SYNERGY
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucMtkOui)) {
		if (IS_FEATURE_ENABLED(
			prAdapter->rWifiVar.ucGbandProbe256QAM)) {
			prCmdContent->u4Flags
				|= MTK_SYNERGY_CAP_SUPPORT_24G_MCS89_PROBING;
		}
	}
#endif
	prCmdContent->ucTxAmpdu = prAdapter->rWifiVar.ucAmpduTx;
	prCmdContent->ucRxAmpdu = prAdapter->rWifiVar.ucAmpduRx;

	/* AMSDU in AMPDU global configuration */
	prCmdContent->ucTxAmsduInAmpdu = prAdapter->rWifiVar.ucAmsduInAmpduTx;
	prCmdContent->ucRxAmsduInAmpdu = prAdapter->rWifiVar.ucAmsduInAmpduRx;

#if (CFG_SUPPORT_802_11BE == 1)
	if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11BE) {
		/* EHT peer AMSDU in AMPDU configuration */
		prCmdContent->ucTxAmsduInAmpdu &=
			prAdapter->rWifiVar.ucEhtAmsduInAmpduTx;
		prCmdContent->ucRxAmsduInAmpdu &=
		prAdapter->rWifiVar.ucEhtAmsduInAmpduRx;
	} else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	/* prStaRec->ucDesiredPhyTypeSet firm in */
		/* bssDetermineStaRecPhyTypeSet() in advance */
	if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AX) {
		/* HE peer AMSDU in AMPDU configuration */
		prCmdContent->ucTxAmsduInAmpdu &=
			prAdapter->rWifiVar.ucHeAmsduInAmpduTx;
		prCmdContent->ucRxAmsduInAmpdu &=
		prAdapter->rWifiVar.ucHeAmsduInAmpduRx;
	} else
#endif
	if ((prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AC) ||
		(prStaRec->u4Flags & MTK_SYNERGY_CAP_SUPPORT_24G_MCS89)) {
		/* VHT pear AMSDU in AMPDU configuration */
		prCmdContent->ucTxAmsduInAmpdu
			&= prAdapter->rWifiVar.ucVhtAmsduInAmpduTx;
		prCmdContent->ucRxAmsduInAmpdu
			&= prAdapter->rWifiVar.ucVhtAmsduInAmpduRx;
	} else if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11N) {
		/* HT peer AMSDU in AMPDU configuration */
		prCmdContent->ucTxAmsduInAmpdu
			&= prAdapter->rWifiVar.ucHtAmsduInAmpduTx;
		prCmdContent->ucRxAmsduInAmpdu
			&= prAdapter->rWifiVar.ucHtAmsduInAmpduRx;
	}

	if (IS_BSS_INDEX_AIS(prAdapter, prStaRec->ucBssIndex))
		prBssDesc = aisGetTargetBssDesc(prAdapter,
			prStaRec->ucBssIndex);
	else if (IS_BSS_INDEX_P2P(prAdapter, prStaRec->ucBssIndex))
		prBssDesc = p2pGetTargetBssDesc(prAdapter,
			prStaRec->ucBssIndex);

	if (prBssDesc && bssIsIotAp(prAdapter, prBssDesc,
		WLAN_IOT_AP_DIS_TX_AMSDU)) {
		prCmdContent->ucTxAmsduInAmpdu = 0;
		DBGLOG(NIC, INFO, "IoT AP: DISABLE AMSDU\n");
	}

#if CFG_SUPPORT_WED_PROXY
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (IS_MLD_STAREC_MULTI(mldStarecGetByStarec(
			prAdapter, prStaRec)))
		prCmdContent->ucRxAmsduInAmpdu = 0;
#endif
#endif
	if ((prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11BE) ||
	    (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AX))
		prCmdContent->u4TxMaxAmsduInAmpduLen =
			prAdapter->rWifiVar.u4TxMaxAmsduInAmpduLen;
	else if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AC)
		prCmdContent->u4TxMaxAmsduInAmpduLen =
			prAdapter->rWifiVar.u4VhtTxMaxAmsduInAmpduLen;
	else if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11N)
		prCmdContent->u4TxMaxAmsduInAmpduLen =
			prAdapter->rWifiVar.u4HtTxMaxAmsduInAmpduLen;

#if (CFG_SUPPORT_802_11BE == 1)
	if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11BE) {
		prCmdContent->rBaSize.rEhtBaSize.u2RxBaSize =
				prAdapter->rWifiVar.u2RxEhtBaSize;
		prCmdContent->rBaSize.rEhtBaSize.u2TxBaSize =
				prAdapter->rWifiVar.u2TxEhtBaSize;
	} else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AX) {
		prCmdContent->rBaSize.rHeBaSize.u2RxBaSize =
				prAdapter->rWifiVar.u2RxHeBaSize;
		prCmdContent->rBaSize.rHeBaSize.u2TxBaSize =
				prAdapter->rWifiVar.u2TxHeBaSize;
	} else
#endif
	{
		prCmdContent->rBaSize.rHtVhtBaSize.ucTxBaSize
			= prAdapter->rWifiVar.ucTxBaSize;

		if (prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AC)
			prCmdContent->rBaSize.rHtVhtBaSize.ucRxBaSize
				= prAdapter->rWifiVar.ucRxVhtBaSize;
		else
			prCmdContent->rBaSize.rHtVhtBaSize.ucRxBaSize
				= prAdapter->rWifiVar.ucRxHtBaSize;
	}

	/* RTS Policy */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucSigTaRts)) {
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucDynBwRts))
			prCmdContent->ucRtsPolicy = RTS_POLICY_DYNAMIC_BW;
		else
			prCmdContent->ucRtsPolicy = RTS_POLICY_STATIC_BW;
	} else
		prCmdContent->ucRtsPolicy = RTS_POLICY_LEGACY;

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
	cnmStaRecCmdHeContentFill(prStaRec, prCmdContent);
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	prCmdContent->u2He6gBandCapInfo = prStaRec->u2He6gBandCapInfo;
#endif
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	cnmStaRecCmdEhtContentFill(prStaRec, prCmdContent);
#endif

#if CFG_SUPPORT_MLR
	if (MLR_IS_BOTH_SUPPORT(prAdapter, prStaRec) &&
	    mlrCanEnterMlrStart(prAdapter, prStaRec,
	    MLR_GET_BAND(prAdapter, prStaRec))) {
		prCmdContent->ucMlrMode = (prStaRec->ucMlrSupportBitmap &
			prAdapter->u4MlrSupportBitmap);
		prCmdContent->ucMlrState = MLR_STATE_START;
	} else {
		prCmdContent->ucMlrMode = (prStaRec->ucMlrSupportBitmap &
			prAdapter->u4MlrSupportBitmap);
		prCmdContent->ucMlrState = MLR_STATE_IDLE;
	}

	MLR_DBGLOG(prAdapter, REQ, INFO,
		"MLR updatestarec StaRec[%u] WIDX[%u] ucStaState[%u] MLR[0x%04x, 0x%02x] ucMlrMode[0x%02x] ucMlrState[%u] RCPI=%d(RSSI=%d)\n",
		prCmdContent->ucStaIndex,
		prCmdContent->ucWlanIndex,
		prCmdContent->ucStaState,
		prAdapter->u4MlrSupportBitmap,
		prStaRec->ucMlrSupportBitmap,
		prCmdContent->ucMlrMode,
		prCmdContent->ucMlrState,
		prStaRec->ucRCPI,
		RCPI_TO_dBm(prStaRec->ucRCPI));
#endif

	log_dbg(REQ, TRACE, "Update StaRec[%u] WIDX[%u] State[%u] Type[%u] BssIdx[%u] AID[%u]\n",
		prCmdContent->ucStaIndex,
		prCmdContent->ucWlanIndex,
		prCmdContent->ucStaState,
		prCmdContent->ucStaType,
		prCmdContent->ucBssIndex,
		prCmdContent->u2AssocId);

	log_dbg(REQ, TRACE, "Update StaRec[%u] QoS[%u] UAPSD[%u]\n",
		prCmdContent->ucStaIndex,
		prCmdContent->ucIsQoS,
		prCmdContent->ucIsUapsdSupported);

#if CFG_SUPPORT_WED_PROXY
	wedStaRecUpdate(prAdapter, prStaRec);
#endif

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
		CMD_ID_UPDATE_STA_RECORD,		/* ucCID */
		TRUE,					/* fgSetQuery */
		fgNeedResp,				/* fgNeedResp */
		FALSE,					/* fgIsOid */
		fgNeedResp ? cnmStaRecHandleEventPkt : NULL, NULL,
		/* pfCmdTimeoutHandler */
		sizeof(struct CMD_UPDATE_STA_RECORD),	/* u4SetQueryInfoLen */
		(uint8_t *) prCmdContent,	/* pucInfoBuffer */
		NULL,				/* pvSetQueryBuffer */
		0				/* u4SetQueryBufferLen */
	);

	cnmMemFree(prAdapter, prCmdContent);

	if (rStatus != WLAN_STATUS_PENDING) {
		log_dbg(MEM, WARN,
			"%s: CMD_ID_UPDATE_STA_RECORD result 0x%08x\n",
			__func__, rStatus);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
static void
cnmStaSendRemoveCmd(struct ADAPTER *prAdapter,
	enum ENUM_STA_REC_CMD_ACTION eActionType,
	uint8_t ucStaRecIndex, uint8_t ucBssIndex)
{
	struct CMD_REMOVE_STA_RECORD rCmdContent;
	uint32_t rStatus;

	ASSERT(prAdapter);

	rCmdContent.ucActionType = (uint8_t) eActionType;
	rCmdContent.ucStaIndex = ucStaRecIndex;
	rCmdContent.ucBssIndex = ucBssIndex;
	rCmdContent.ucReserved = 0;

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
		CMD_ID_REMOVE_STA_RECORD,		/* ucCID */
		TRUE,	/* fgSetQuery */
		FALSE,	/* fgNeedResp */
		FALSE,	/* fgIsOid */
		NULL,	/* pfCmdDoneHandler */
		NULL,	/* pfCmdTimeoutHandler */
		sizeof(struct CMD_REMOVE_STA_RECORD),	/* u4SetQueryInfoLen */
		(uint8_t *) &rCmdContent,		/* pucInfoBuffer */
		NULL,	/* pvSetQueryBuffer */
		0	/* u4SetQueryBufferLen */
	    );

	if (rStatus != WLAN_STATUS_PENDING) {
		log_dbg(MEM, WARN,
			"%s: CMD_ID_REMOVE_STA_RECORD result 0x%08x\n",
			__func__, rStatus);
	}
}

uint8_t *cnmStaRecGetTypeString(enum ENUM_STA_TYPE eStaType)
{
	uint8_t *pucTypeString = NULL;

	if (eStaType & STA_TYPE_LEGACY_MASK)
		pucTypeString = apucStaRecType[STA_TYPE_LEGACY_INDEX];
	if (eStaType & STA_TYPE_P2P_MASK)
		pucTypeString = apucStaRecType[STA_TYPE_P2P_INDEX];
	if (eStaType & STA_TYPE_BOW_MASK)
		pucTypeString = apucStaRecType[STA_TYPE_BOW_INDEX];
#if CFG_SUPPORT_NAN
	if (eStaType & STA_TYPE_NAN)
		pucTypeString = apucStaRecType[STA_TYPE_NAN_INDEX];
#endif

	return pucTypeString;
}

uint8_t *cnmStaRecGetRoleString(enum ENUM_STA_TYPE eStaType)
{
	uint8_t *pucRoleString = NULL;

	if (eStaType & STA_TYPE_ADHOC_MASK) {
		pucRoleString = apucStaRecRole[
			STA_ROLE_ADHOC_INDEX - STA_ROLE_BASE_INDEX];
	}
	if (eStaType & STA_TYPE_CLIENT_MASK) {
		pucRoleString = apucStaRecRole[
			STA_ROLE_CLIENT_INDEX - STA_ROLE_BASE_INDEX];
	}
	if (eStaType & STA_TYPE_AP_MASK) {
		pucRoleString = apucStaRecRole[
			STA_ROLE_AP_INDEX - STA_ROLE_BASE_INDEX];
	}
	if (eStaType & STA_TYPE_DLS_MASK) {
		pucRoleString = apucStaRecRole[
			STA_ROLE_DLS_INDEX - STA_ROLE_BASE_INDEX];
	}

	return pucRoleString;
}

int cnmShowBssInfo(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo,
	char *pcCommand, int i4TotalLen)
{
	int32_t i4BytesWritten = 0;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tBSS[%d][RF_BAND=%d][OMAC="MACSTR"][LINK_ID=%u]:\n",
		prBssInfo->ucBssIndex, prBssInfo->eBand,
		MAC2STR(prBssInfo->aucOwnMacAddr),
		prBssInfo->ucLinkIndex);
#else
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tBSS[%d][RF_BAND=%d][OMAC="MACSTR"]:\n",
		prBssInfo->ucBssIndex, prBssInfo->eBand,
		MAC2STR(prBssInfo->aucOwnMacAddr));
#endif

	if (!prBssInfo->fgIsInUse) {
		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\t[ERROR] NOT IN USE!!\n");
		return i4BytesWritten;
	}

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		 "\tACTIVE/HW_BAND/OMAC_IDX: %u/%u/%u\n",
		prBssInfo->fgIsNetActive,
		prBssInfo->eHwBandIdx,
		prBssInfo->ucOwnMacIndex);

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		 "\tBMC/OP_MODE/WMM/CONN_STATE/BSSID: %u/%u/%u/%u/"MACSTR"\n",
		prBssInfo->ucBMCWlanIndex,
		prBssInfo->eCurrentOPMode,
		prBssInfo->ucWmmQueSet,
		prBssInfo->eConnectionState,
		MAC2STR(prBssInfo->aucBSSID));

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		 "\tSSID/QBSS/PHY/PWR_STATE: %d %s/%u/0x%x/%d\n",
		prBssInfo->ucSSIDLen,
		prBssInfo->aucSSID,
		prBssInfo->fgIsQBSS,
		prBssInfo->ucPhyTypeSet,
		prAdapter->rWifiVar.aePwrState[prBssInfo->ucBssIndex]);
#if 0
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		 "AID/BCN_INT/RATE/PROT:%d/%d/0x%x/%u\n",
		prBssInfo->u2AssocId,
		prBssInfo->u2BeaconInterval,
		prBssInfo->u2BSSBasicRateSet,
		secIsProtectedBss(prAdapter, prBssInfo));
#endif
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tCHANNEL/WIDTH/S1/S2: %u/%u/%u/%u\n",
		prBssInfo->ucPrimaryChannel,
		prBssInfo->ucVhtChannelWidth,
		prBssInfo->ucVhtChannelFrequencyS1,
		prBssInfo->ucVhtChannelFrequencyS2);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tGRP_MLD_ID/OWN_MLD_ID: %u/%u\n",
		prBssInfo->ucGroupMldId,
		prBssInfo->ucOwnMldId);
#endif

	return i4BytesWritten;
}

int cnmShowStaRec(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
	char *pcCommand, int i4TotalLen)
{
	int32_t i4BytesWritten = 0;

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tSTA[%d][WIDX=%d][BSS_IDX=%d]:\n",
		prStaRec->ucIndex,
		prStaRec->ucWlanIndex,
		prStaRec->ucBssIndex);

	if (!prStaRec->fgIsInUse) {
		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\t[ERROR] NOT IN USE!!\n");
		return i4BytesWritten;
	}

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tSTATE/RCPI/MAC: %u/%u/"MACSTR"\n",
		prStaRec->ucStaState,
		prStaRec->ucRCPI,
		MAC2STR(prStaRec->aucMacAddr));
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tPHY/DESIRE_PHY/BASIC_PHY/WMM/UAPSD: 0x%x/0x%x/0x%x/%u/%u\n",
		prStaRec->ucPhyTypeSet,
		prStaRec->ucDesiredPhyTypeSet,
		prStaRec->ucNonHTBasicPhyType,
		prStaRec->fgIsWmmSupported,
		prStaRec->fgIsUapsdSupported);
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tAID/OP_RATE/DESIRE_RATE/DEFAULT_RATE: %d/0x%x/0x%x/0x%x\n",
		prStaRec->u2AssocId,
		prStaRec->u2OperationalRateSet,
		prStaRec->u2DesiredNonHTRateSet,
		prStaRec->u2HwDefaultFixedRateCode);
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tPS/TX_ALLOWED/KEY_READY/TX_AMPDU/RX_AMPDU: %u/%u/%u/%u/%u\n",
		prStaRec->fgIsInPS,
		prStaRec->fgIsTxAllowed,
		prStaRec->fgIsTxKeyReady,
		prStaRec->fgTxAmpduEn,
		prStaRec->fgRxAmpduEn);
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tHT_CAP/HT_EXT_CAP/TX_BF_CAP/VHT_CAP: 0x%x/0x%x/0x%x/0x%x\n",
		prStaRec->u2HtCapInfo,
		prStaRec->u2HtExtendedCap,
		prStaRec->u4TxBeamformingCap,
		prStaRec->u4VhtCapInfo);
#if (CFG_SUPPORT_802_11AX == 1)
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tHE_MAC_CAP/HE_PHY_CAP: 0x%04x%08x/0x%02x%04x%016llx\n",
		*(uint16_t *)(prStaRec->ucHeMacCapInfo + 4),
		*(uint32_t *)(prStaRec->ucHeMacCapInfo),
		*(uint8_t *)(prStaRec->ucHePhyCapInfo + 10),
		*(uint16_t *)(prStaRec->ucHePhyCapInfo + 8),
		*(uint64_t *)(prStaRec->ucHePhyCapInfo));
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tEHT_MAC_CAP/EHT_PHY_CAP: 0x%04x/0x%016llx\n",
		(*(uint16_t *)(prStaRec->ucEhtMacCapInfo)),
		(*(uint64_t *)(prStaRec->ucEhtPhyCapInfo)));
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tMLD_STA/LINK_ID/TID_BMAP/AP_RM/MLD_ADDR: %u/%u/0x%x/%u/"
		MACSTR "\n",
		prStaRec->ucMldStaIndex,
		prStaRec->ucLinkIndex,
		prStaRec->ucULTidBitmap,
		prStaRec->fgApRemoval,
		MAC2STR(prStaRec->aucMldAddr));
#endif

	return i4BytesWritten;
}

void cnmDumpBssInfo(struct ADAPTER *prAdapter, uint8_t ucBssIdx)
{
	struct BSS_INFO *prBssInfo;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (!prBssInfo->fgIsInUse)
		return;

	log_dbg(MEM, INFO, "============= DUMP BSS[%u] ===========\n",
		ucBssIdx);

	/*
	 * [1] BASIC [BSS_IDX, ACTIVE, TYPE, PRIVATE_DATA, OMAC_IDX, OMAC_ADDR, BMC_IDX, MODE]
	 * [2] CONNECTION [STATE, BSSID, SSID, AID, BCN_INT, PHY_TYPE]
	 * [3] RLM [BAND, CHANNEL, WIDTH, S1, S2]
	 * [4] MLO [GROUP_IDX, OWN_MLD_ID]
	 * [5] TRX [ABSENT, QBSS]
	 */
	log_dbg(MEM, INFO, "\tBASIC [%u %d %u %u %u %u " MACSTR " %u %u]\n",
		prBssInfo->ucBssIndex,
		prBssInfo->fgIsNetActive,
		prBssInfo->eNetworkType,
		prBssInfo->u4PrivateData,
		prBssInfo->eHwBandIdx,
		prBssInfo->ucOwnMacIndex,
		MAC2STR(prBssInfo->aucOwnMacAddr),
		prBssInfo->ucBMCWlanIndex,
		prBssInfo->eCurrentOPMode);
	log_dbg(MEM, INFO, "\tCONNECTION [%u " MACSTR " %u %s 0x%x %u 0x%x]\n",
		prBssInfo->eConnectionState,
		MAC2STR(prBssInfo->aucBSSID),
		prBssInfo->ucSSIDLen,
		prBssInfo->aucSSID,
		prBssInfo->u2AssocId,
		prBssInfo->u2BeaconInterval,
		prBssInfo->ucPhyTypeSet);
	log_dbg(MEM, INFO, "\tRLM [%u %u %u %u %u]\n",
		prBssInfo->eBand,
		prBssInfo->ucPrimaryChannel,
		prBssInfo->ucVhtChannelWidth,
		prBssInfo->ucVhtChannelFrequencyS1,
		prBssInfo->ucVhtChannelFrequencyS2);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	log_dbg(MEM, INFO, "\tMLO [%u %u]\n",
		prBssInfo->ucGroupMldId,
		prBssInfo->ucOwnMldId);
#endif
	log_dbg(MEM, INFO, "\tTRX [%u %u]\n",
		prBssInfo->fgIsNetAbsent,
		prBssInfo->fgIsQBSS);

	log_dbg(MEM, INFO, "============= DUMP END ===========\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmDumpStaRec(struct ADAPTER *prAdapter, uint8_t ucStaRecIdx)
{
	uint8_t ucWTEntry;
	uint32_t i;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);

	if (!prStaRec) {
		log_dbg(SW4, INFO, "Invalid StaRec index[%u], skip dump!\n",
			ucStaRecIdx);
		return;
	}

	ucWTEntry = prStaRec->ucWlanIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);

	log_dbg(SW4, INFO, "============= DUMP STA[%u] ===========\n",
		ucStaRecIdx);
	/* [1]STA_IDX                  [2]BSS_IDX
	 * [3]MAC                      [4]TYPE
	 * [5]WTBL                     [6]USED
	 * [7]State                    [8]QoS
	 * [9]HT/VHT                   [10]AID
	 * [11]WMM                     [12]UAPSD
	 * [13]SEC                     [14]PhyTypeSet
	 * [15]Desired                 [16]NonHtBasic
	 * [17]BssBasic                [18]Operational
	 * [19]DesiredNonHT            [20]Df FixedRate
	 * [21]HT Cap                  [22]ExtCap
	 * [23]BeemCap                 [24]MCS
	 * [25]MCS32                   [26]VHT Cap
	 * [27]TxMCS                   [28]RxMCS
	 * [29]VhtOpMode               [30]RCPI
	 * [31]InPS                    [32]TxAllowed
	 * [33]KeyRdy                  [34]AMPDU
	 * [35]TxQLEN TC               [36]BMP AC Delivery/Trigger
	 * [37]FreeQuota:Total         [38]Delivery/NonDelivery
	 * [39]aucRxMcsBitmask
	 */

	log_dbg(SW4, INFO, "[1][%u],[2][%u],[3][" MACSTR
			"],[4][%s %s],[5][%u],[6][%u],[7][%u],[8][%u],[9][%u/%u],[10][%u]\n",
		prStaRec->ucIndex,
		prStaRec->ucBssIndex,
		MAC2STR(prStaRec->aucMacAddr),
		cnmStaRecGetTypeString(prStaRec->eStaType),
		cnmStaRecGetRoleString(prStaRec->eStaType),
		ucWTEntry, prStaRec->fgIsInUse,
		prStaRec->ucStaState, prStaRec->fgIsQoS,
		(prStaRec->ucDesiredPhyTypeSet
			& PHY_TYPE_SET_802_11N) ? TRUE : FALSE,
		(prStaRec->ucDesiredPhyTypeSet
			& PHY_TYPE_SET_802_11AC) ? TRUE : FALSE,
		prStaRec->u2AssocId);

	log_dbg(SW4, INFO, "[11][%u],[12][%u],[13][%u],[14][0x%x],[15][0x%x],[16][0x%x],[17][0x%x],[18][0x%x],[19][0x%x],[20][0x%x]\n",
		prStaRec->fgIsWmmSupported,
		prStaRec->fgIsUapsdSupported,
		secIsProtectedBss(prAdapter, prBssInfo),
		prBssInfo->ucPhyTypeSet,
		prStaRec->ucDesiredPhyTypeSet,
		prStaRec->ucNonHTBasicPhyType,
		prBssInfo->u2BSSBasicRateSet,
		prStaRec->u2OperationalRateSet,
		prStaRec->u2DesiredNonHTRateSet,
		prStaRec->u2HwDefaultFixedRateCode);

	log_dbg(SW4, INFO, "[21][0x%x],[22][0x%x],[23][0x%x],[24][0x%x],[25][%u],[26][0x%x],[27][0x%x],[28][0x%x],[29][0x%x],[30][%u]\n",
		prStaRec->u2HtCapInfo,
		prStaRec->u2HtExtendedCap,
		prStaRec->u4TxBeamformingCap,
		prStaRec->ucMcsSet,
		prStaRec->fgSupMcs32,
		prStaRec->u4VhtCapInfo,
		prStaRec->u2VhtTxMcsMap,
		prStaRec->u2VhtRxMcsMap,
		prStaRec->ucVhtOpMode,
		prStaRec->ucRCPI);

	log_dbg(SW4, INFO, "[31][%u],[32][%u],[33][%u],[34][%u/%u],[35][%u:%u:%u:%u],[36][%x/%x],[37][%u],[38][%u/%u],[39][0x%x][0x%x]\n",
		prStaRec->fgIsInPS,
		prStaRec->fgIsTxAllowed,
		prStaRec->fgIsTxKeyReady,
		prStaRec->fgTxAmpduEn,
		prStaRec->fgRxAmpduEn,
		prStaRec->aprTargetQueue[0]->u4NumElem,
		prStaRec->aprTargetQueue[1]->u4NumElem,
		prStaRec->aprTargetQueue[2]->u4NumElem,
		prStaRec->aprTargetQueue[3]->u4NumElem,
		prStaRec->ucBmpDeliveryAC,
		prStaRec->ucBmpTriggerAC,
		prStaRec->ucFreeQuota,
		prStaRec->ucFreeQuotaForDelivery,
		prStaRec->ucFreeQuotaForNonDelivery,
		prStaRec->aucRxMcsBitmask[0],
		prStaRec->aucRxMcsBitmask[1]);

	log_dbg(SW4, INFO, "[CapInfo][0x%x],[SupOpClass][0x%x],[SupChnl2g][0x%x],[SupChnl5g_0][0x%x],[SupChnl5g_1][0x%x]\n",
		prStaRec->u2CapInfo,
		prStaRec->u4SupportedOpClassBits,
		prStaRec->u2SupportedChnlBits_2g,
		prStaRec->u4SupportedChnlBits_5g_0,
		prStaRec->u2SupportedChnlBits_5g_1);

#if (CFG_SUPPORT_802_11AX == 1)
	log_dbg(SW4, INFO, "[HeMacCap][0x%04x%08x],[HePhyCap][0x%02x%04x%016llx]\n",
		*(uint16_t *)(prStaRec->ucHeMacCapInfo + 4),
		*(uint32_t *)(prStaRec->ucHeMacCapInfo),
		*(uint8_t *)(prStaRec->ucHePhyCapInfo + 10),
		*(uint16_t *)(prStaRec->ucHePhyCapInfo + 8),
		*(uint64_t *)(prStaRec->ucHePhyCapInfo));
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	log_dbg(SW4, INFO, "[EhtMacCap][0x%04x],[EhtPhyCap][0x%016llx],[EhtPhyCapExt][0x%016llx]\n",
		(*(uint16_t *)(prStaRec->ucEhtMacCapInfo)),
		(*(uint64_t *)(prStaRec->ucEhtPhyCapInfo)),
		(*(uint64_t *)(prStaRec->ucEhtPhyCapInfoExt)));
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	log_dbg(SW4, INFO, "[MldStaIndex][%u], [LinkIndex][%u], [TidBitmap][%u], [MldAddr][" MACSTR "]\n",
		prStaRec->ucMldStaIndex,
		prStaRec->ucLinkIndex,
		prStaRec->ucULTidBitmap,
		MAC2STR(prStaRec->aucMldAddr));
#endif
#endif

	for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++) {
		if (prStaRec->aprRxReorderParamRefTbl[i]) {
			log_dbg(SW4, INFO, "TID[%u],Valid[%u],WinStart/End[%u/%u],WinSize[%u],ReOrderQueLen[%u],Bubble Exist[%u],SN[%u]\n",
				prStaRec->aprRxReorderParamRefTbl[i]
					->ucTid,
				prStaRec->aprRxReorderParamRefTbl[i]
					->fgIsValid,
				prStaRec->aprRxReorderParamRefTbl[i]
					->u2WinStart,
				prStaRec->aprRxReorderParamRefTbl[i]
					->u2WinEnd,
				prStaRec->aprRxReorderParamRefTbl[i]
					->u2WinSize,
				prStaRec->aprRxReorderParamRefTbl[i]
					->rReOrderQue.u4NumElem,
				prStaRec->aprRxReorderParamRefTbl[i]
					->fgHasBubble,
				prStaRec->aprRxReorderParamRefTbl[i]
					->u2FirstBubbleSn);
		}
	}
	log_dbg(SW4, INFO, "============= DUMP END ===========\n");
}

uint32_t cnmDumpMemoryStatus(struct ADAPTER *prAdapter, uint8_t *pucBuf,
	uint32_t u4Max)
{
	uint32_t u4Len = 0;
#if CFG_DBG_MGT_BUF
	struct BUF_INFO *prBufInfo;

	LOGBUF(pucBuf, u4Max, u4Len, "\n");
	LOGBUF(pucBuf, u4Max, u4Len,
		"============= DUMP Memory Status =============\n");

	LOGBUF(pucBuf, u4Max, u4Len,
		"Dynamic alloc OS memory count: alloc[%u] free[%u]\n",
		prAdapter->u4MemAllocDynamicCount,
		prAdapter->u4MemFreeDynamicCount);

	prBufInfo = &prAdapter->rMsgBufInfo;

	LOGBUF(pucBuf, u4Max, u4Len,
		"MSG memory count: alloc[%u] free[%u] null[%u] bitmap[0x%08x]\n"
		,
		prBufInfo->u4AllocCount,
		prBufInfo->u4FreeCount,
		prBufInfo->u4AllocNullCount,
		(uint32_t) prBufInfo->rFreeBlocksBitmap);

	prBufInfo = &prAdapter->rMgtBufInfo;

	LOGBUF(pucBuf, u4Max, u4Len,
		"MGT memory count: alloc[%u] free[%u] null[%u] bitmap[0x%08x]\n"
		,
		prBufInfo->u4AllocCount,
		prBufInfo->u4FreeCount,
		prBufInfo->u4AllocNullCount,
		(uint32_t) prBufInfo->rFreeBlocksBitmap);

	LOGBUF(pucBuf, u4Max, u4Len, "============= DUMP END =============\n");

#endif

	return u4Len;
}

#if CFG_SUPPORT_TDLS
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to add a peer record.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t	/* TDLS_STATUS, prStaRec->ucNetTypeIndex */
cnmPeerAdd(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct CMD_PEER_ADD *prCmd;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec, *prStaRecOfAp;

	/* sanity check */

	if ((prAdapter == NULL) || (pvSetBuffer == NULL)
		|| (pu4SetInfoLen == NULL))
		return TDLS_STATUS_FAIL;

	/* init */
	*pu4SetInfoLen = sizeof(struct CMD_PEER_ADD);
	prCmd = (struct CMD_PEER_ADD *) pvSetBuffer;

	prBssInfo
		= GET_BSS_INFO_BY_INDEX(prAdapter, prCmd->ucBssIdx);
	if (prBssInfo == NULL) {
		log_dbg(MEM, ERROR, "prBssInfo %d is NULL!\n"
				, prCmd->ucBssIdx);
		return TDLS_STATUS_FAIL;
	}

	prStaRec = cnmGetStaRecByAddress(prAdapter,
		(uint8_t) prBssInfo->ucBssIndex,
		prCmd->aucPeerMac);

	if (prStaRec == NULL) {
		prStaRec =
		cnmStaRecAlloc(prAdapter, STA_TYPE_DLS_PEER,
			(uint8_t) prBssInfo->ucBssIndex,
			prCmd->aucPeerMac);

		if (prStaRec == NULL)
			return TDLS_STATUS_RESOURCES;

		if (prBssInfo->ucBssIndex)
			prStaRec->ucBssIndex = prBssInfo->ucBssIndex;

		/* init the prStaRec */
		/* prStaRec will be zero first in cnmStaRecAlloc() */
		COPY_MAC_ADDR(prStaRec->aucMacAddr, prCmd->aucPeerMac);

		prStaRec->u2BSSBasicRateSet = prBssInfo->u2BSSBasicRateSet;
		prStaRec->ucDesiredPhyTypeSet
			= prAdapter->rWifiVar.ucAvailablePhyTypeSet;
		prStaRec->u2DesiredNonHTRateSet
			= prAdapter->rWifiVar.ucAvailablePhyTypeSet;

		prStaRec->u2OperationalRateSet
			= prBssInfo->u2OperationalRateSet;
		prStaRec->ucPhyTypeSet = prBssInfo->ucPhyTypeSet;
		prStaRec->eStaType = prCmd->eStaType;

		/* align setting with AP */
		prStaRecOfAp = prBssInfo->prStaRecOfAP;
		if (prStaRecOfAp) {
			prStaRec->u2DesiredNonHTRateSet
				= prStaRecOfAp->u2DesiredNonHTRateSet;
		}

		/* Init lowest rate to prevent CCK in 5G band */
		nicTxUpdateStaRecDefaultRate(prAdapter, prStaRec);

		/* Better to change state here, not at TX Done */
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

	} else {
		if ((prStaRec->ucStaState > STA_STATE_1)
			&& (IS_DLS_STA(prStaRec))) {
			/* TODO: Teardown the peer */
			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
		}
	}
	return TDLS_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to update a peer record.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t	/* TDLS_STATUS */
cnmPeerUpdate(struct ADAPTER *prAdapter, void *pvSetBuffer,
		uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{

	struct CMD_PEER_UPDATE *prCmd;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucNonHTPhyTypeSet;

	uint16_t u2OperationalRateSet = 0;

	uint8_t ucRate;
	uint16_t i, j;

#if CFG_SUPPORT_802_11AC
	uint8_t ucRxNss = 1;
	struct WIFI_VAR *prWifiVar;
#endif /* CFG_SUPPORT_802_11AC */

	/* sanity check */
	if ((!prAdapter) || (!pvSetBuffer) || (!pu4SetInfoLen))
		return TDLS_STATUS_FAIL;

	/* init */
	*pu4SetInfoLen = sizeof(struct CMD_PEER_ADD);
	prCmd = (struct CMD_PEER_UPDATE *) pvSetBuffer;

	prBssInfo
		= GET_BSS_INFO_BY_INDEX(prAdapter, prCmd->ucBssIdx);

	if (prBssInfo == NULL) {
		log_dbg(MEM, ERROR, "prBssInfo %d is NULL!\n"
				, prCmd->ucBssIdx);
		return TDLS_STATUS_FAIL;
	}
	prStaRec = cnmGetStaRecByAddress(prAdapter,
		(uint8_t) prBssInfo->ucBssIndex,
		prCmd->aucPeerMac);

	if ((!prStaRec) || !(prStaRec->fgIsInUse))
		return TDLS_STATUS_FAIL;

	if (!IS_DLS_STA(prStaRec))
		return TDLS_STATUS_FAIL;

	if (prBssInfo) {
		if (prBssInfo->ucBssIndex)
			prStaRec->ucBssIndex = prBssInfo->ucBssIndex;
	}

	/* update the record join time. */
	GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

	/* update Station Record - Status/Reason Code */
	prStaRec->u2StatusCode = prCmd->u2StatusCode;
	prStaRec->u2AssocId = 0;		/* no use */
	prStaRec->u2ListenInterval = 0;		/* unknown */
	prStaRec->fgIsQoS = TRUE;
	prStaRec->fgIsUapsdSupported
		= ((1 << 4) & prCmd->aucExtCap[3]) ? TRUE : FALSE;
	prStaRec->u4TxBeamformingCap = 0;	/* no use */
	prStaRec->ucAselCap = 0;		/* no use */
	prStaRec->ucRCPI = 120;
	prStaRec->ucBmpTriggerAC = prCmd->UapsdBitmap;
	prStaRec->ucBmpDeliveryAC = prCmd->UapsdBitmap;
	prStaRec->ucUapsdSp = prCmd->UapsdMaxSp;
	prStaRec->eStaType = prCmd->eStaType;

	/* ++ support rate */
	if (prCmd->u2SupRateLen) {
		for (i = 0; i < prCmd->u2SupRateLen; i++) {
			if (prCmd->aucSupRate[i]) {
				ucRate = prCmd->aucSupRate[i] & RATE_MASK;
				/* Search all valid data rates */
				for (j = 0; j < sizeof(aucValidDataRate)
					/ sizeof(uint8_t); j++) {
					if (ucRate == aucValidDataRate[j]) {
						u2OperationalRateSet |= BIT(j);
						break;
					}
				}
			}

		}

		prStaRec->u2OperationalRateSet = u2OperationalRateSet;
	}

	prStaRec->u2BSSBasicRateSet = prBssInfo->u2BSSBasicRateSet;
	/* 4     <5> PHY type setting */

	prStaRec->ucPhyTypeSet = 0;
	if (prBssInfo->eBand == BAND_2G4) {
		if (prCmd->fgIsSupHt)
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

		/* if not 11n only */
		if (!(prStaRec->u2BSSBasicRateSet
			& RATE_SET_BIT_HT_PHY)) {
			/* check if support 11g */
			if ((prStaRec->u2OperationalRateSet
				& RATE_SET_OFDM)) {
				prStaRec->ucPhyTypeSet
					|= PHY_TYPE_BIT_ERP;
			}

			/* if not 11g only */
			if (!(prStaRec->u2BSSBasicRateSet
				& RATE_SET_OFDM)) {
				/* check if support 11b */
				if ((prStaRec->u2OperationalRateSet
					 & RATE_SET_HR_DSSS)) {
					prStaRec->ucPhyTypeSet
						|= PHY_TYPE_BIT_HR_DSSS;
				}
			}
		}
#if CFG_SUPPORT_TDLS_11AX
		if (prCmd->fgIsSupHe)
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HE;

		if (prCmd->fgIsSupVht)
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;
#endif
	} else {
#if CFG_SUPPORT_TDLS_11AX
		if (prCmd->fgIsSupHe)
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HE;
#endif

		if (prCmd->fgIsSupVht)
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;

		if (prCmd->fgIsSupHt)
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

		/* if not 11n only */
		if (!(prStaRec->u2BSSBasicRateSet
			& RATE_SET_BIT_HT_PHY)) {
			/* Support 11a definitely */
			prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_OFDM;
		}
	}
	if (IS_STA_IN_AIS(prAdapter, prStaRec)) {
		struct CONNECTION_SETTINGS *prConnSettings;
		enum ENUM_WEP_STATUS eEncStatus;

		prConnSettings =
			aisGetConnSettings(prAdapter,
			prStaRec->ucBssIndex);

		eEncStatus = prConnSettings->eEncStatus;

		if (!((eEncStatus == ENUM_ENCRYPTION3_ENABLED)
			|| (eEncStatus == ENUM_ENCRYPTION3_KEY_ABSENT)
			|| (eEncStatus == ENUM_ENCRYPTION_DISABLED)
		    )) {

			prStaRec->ucPhyTypeSet &= ~PHY_TYPE_BIT_HT;
		}
	}
	prStaRec->ucDesiredPhyTypeSet = prStaRec->ucPhyTypeSet
		& prAdapter->rWifiVar.ucAvailablePhyTypeSet;
	ucNonHTPhyTypeSet = prStaRec->ucDesiredPhyTypeSet
		& PHY_TYPE_SET_802_11ABG;

	/* Check for Target BSS's non HT Phy Types */
	if (ucNonHTPhyTypeSet) {
		if (ucNonHTPhyTypeSet & PHY_TYPE_BIT_ERP)
			prStaRec->ucNonHTBasicPhyType
				= PHY_TYPE_ERP_INDEX;
		else if (ucNonHTPhyTypeSet & PHY_TYPE_BIT_OFDM)
			prStaRec->ucNonHTBasicPhyType
				= PHY_TYPE_OFDM_INDEX;
		else
			prStaRec->ucNonHTBasicPhyType
			= PHY_TYPE_HR_DSSS_INDEX;

		prStaRec->fgHasBasicPhyType = TRUE;
	} else {
		/* Use mandatory for 11N only BSS */
		ASSERT(prStaRec->ucPhyTypeSet & PHY_TYPE_SET_802_11N);
		{
			/* TODO(Kevin): which value should we set
			 * for 11n ? ERP ?
			 */
			prStaRec->ucNonHTBasicPhyType
				= PHY_TYPE_HR_DSSS_INDEX;
		}

		prStaRec->fgHasBasicPhyType = FALSE;
	}

	/* ++HT capability */
	if (prCmd->fgIsSupHt) {
		prStaRec->ucDesiredPhyTypeSet |= PHY_TYPE_BIT_HT;
		prStaRec->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
		prStaRec->u2HtCapInfo = prCmd->rHtCap.u2CapInfo;
		if (!IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucRxLdpc))
			prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_LDPC_CAP;
		prStaRec->ucAmpduParam = prCmd->rHtCap.ucAmpduParamsInfo;
		prStaRec->u2HtExtendedCap = prCmd->rHtCap.u2ExtHtCapInfo;
		prStaRec->u4TxBeamformingCap = prCmd->rHtCap.u4TxBfCapInfo;
		prStaRec->ucAselCap = prCmd->rHtCap.ucAntennaSelInfo;
		prStaRec->ucMcsSet = prCmd->rHtCap.rMCS.arRxMask[0];
		if (prCmd->rHtCap.rMCS.arRxMask[32 / 8] & BIT(0))
			prStaRec->fgSupMcs32 = TRUE;
		else
			prStaRec->fgSupMcs32 = FALSE;
		kalMemCopy(prStaRec->aucRxMcsBitmask,
			prCmd->rHtCap.rMCS.arRxMask,
			sizeof(prStaRec->aucRxMcsBitmask));
	}

#if CFG_SUPPORT_802_11AC
	prWifiVar = &prAdapter->rWifiVar;
	/* ++VHT capability */
	if (prCmd->fgIsSupVht) {
		prStaRec->u4VhtCapInfo = prCmd->rVHtCap.u4CapInfo;

		/* Set Tx LDPC capability */
		if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxLdpc))
			prStaRec->u4VhtCapInfo |= VHT_CAP_INFO_RX_LDPC;
		else if (IS_FEATURE_DISABLED(prWifiVar->ucTxLdpc))
			prStaRec->u4VhtCapInfo &= ~VHT_CAP_INFO_RX_LDPC;

		/* Set Tx STBC capability */
		if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxStbc))
			prStaRec->u4VhtCapInfo |=
				VHT_CAP_INFO_RX_STBC_MASK;
		else if (IS_FEATURE_DISABLED(prWifiVar->ucTxStbc))
			prStaRec->u4VhtCapInfo &=
				~VHT_CAP_INFO_RX_STBC_MASK;

		/* Set Tx TXOP PS capability */
		if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxopPsTx))
			prStaRec->u4VhtCapInfo |=
				VHT_CAP_INFO_VHT_TXOP_PS;
		else if (IS_FEATURE_DISABLED(prWifiVar->ucTxopPsTx))
			prStaRec->u4VhtCapInfo &=
				~VHT_CAP_INFO_VHT_TXOP_PS;

		/* Set Tx Short GI capability */
		if (IS_FEATURE_FORCE_ENABLED(prWifiVar->ucTxShortGI)) {
			prStaRec->u4VhtCapInfo |=
				VHT_CAP_INFO_SHORT_GI_80;
			prStaRec->u4VhtCapInfo |=
				VHT_CAP_INFO_SHORT_GI_160_80P80;
		} else if (IS_FEATURE_DISABLED(
					prWifiVar->ucTxShortGI)) {
			prStaRec->u4VhtCapInfo &=
				~VHT_CAP_INFO_SHORT_GI_80;
			prStaRec->u4VhtCapInfo &=
				~VHT_CAP_INFO_SHORT_GI_160_80P80;
		}

		prStaRec->u2VhtRxMcsMap = prCmd->rVHtCap.rVMCS.u2RxMcsMap;
		prStaRec->u2VhtRxHighestSupportedDataRate =
					prCmd->rVHtCap.rVMCS.u2RxHighest;

		prStaRec->u2VhtTxMcsMap = prCmd->rVHtCap.rVMCS.u2TxMcsMap;
		prStaRec->u2VhtTxHighestSupportedDataRate =
					prCmd->rVHtCap.rVMCS.u2TxHighest;

		prStaRec->ucVhtOpMode =
			VHT_OP_MODE_CHANNEL_WIDTH_20 |
			VHT_OP_MODE_CHANNEL_WIDTH_80;
		/* no op mode IE, use HT/VHT cap to check BW */
		if (prCmd->fgIsSupHt &&
			prBssInfo->fg40mBwAllowed &&
			(prCmd->rHtCap.u2CapInfo & HT_CAP_INFO_SUP_CHNL_WIDTH))
			prStaRec->ucVhtOpMode |= VHT_OP_MODE_CHANNEL_WIDTH_40;
		if ((prCmd->rVHtCap.u4CapInfo &
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160) ||
		    (prCmd->rVHtCap.u4CapInfo &
			VHT_CAP_INFO_MAX_SUP_CHANNEL_WIDTH_SET_160_80P80))
			prStaRec->ucVhtOpMode |=
				VHT_OP_MODE_CHANNEL_WIDTH_160_80P80;

		/* no op mode IE, use MCS set to check NSS */
		if (((prCmd->rVHtCap.rVMCS.u2RxMcsMap &
			VHT_CAP_INFO_MCS_2SS_MASK) >>
			VHT_CAP_INFO_MCS_2SS_OFFSET)
			!= VHT_CAP_INFO_MCS_NOT_SUPPORTED)
			ucRxNss = 2;
		prStaRec->ucVhtOpMode |=
			((ucRxNss - 1)	<< VHT_OP_MODE_RX_NSS_OFFSET) &
			VHT_OP_MODE_RX_NSS;
	}
#endif /* CFG_SUPPORT_802_11AC */

#if CFG_SUPPORT_TDLS_11AX
	/* ++HE capability */
	if (prCmd->fgIsSupHe) {
		kalMemCopy(prStaRec->ucHeMacCapInfo,
			prCmd->rHeCap.ucHeMacCapInfo, HE_MAC_CAP_BYTE_NUM);
		kalMemCopy(prStaRec->ucHePhyCapInfo,
			prCmd->rHeCap.ucHePhyCapInfo, HE_PHY_CAP_BYTE_NUM);
#if (CFG_SUPPORT_WIFI_6G == 1)
		prStaRec->u2He6gBandCapInfo = HE_6G_CAP_INFO_DEFAULT_VAL;
#endif

		for (i = 0; i < 8; i++) {
			uint8_t ucOffset = i * 2;
			uint8_t ucMcsMap;

			if (i < wlanGetSupportNss(prAdapter,
				prBssInfo->ucBssIndex))
				ucMcsMap = HE_CAP_INFO_MCS_MAP_MCS11;
			else
				ucMcsMap = HE_CAP_INFO_MCS_NOT_SUPPORTED;

			prStaRec->u2HeRxMcsMapBW80 |= (ucMcsMap << ucOffset);
			prStaRec->u2HeTxMcsMapBW80 |= (ucMcsMap << ucOffset);
		}
	}
#endif

	cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

	return TDLS_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Get TDLS peer STA_RECORD_T by Peer MAC Address(Usually TA).
 *
 * @param[in] pucPeerMacAddr      Given Peer MAC Address.
 *
 * @retval   Pointer to STA_RECORD_T, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct STA_RECORD *cnmGetTdlsPeerByAddress(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t aucPeerMACAddress[])
{
	struct STA_RECORD *prStaRec = NULL;
	uint16_t i;

	ASSERT(prAdapter);
	ASSERT(aucPeerMACAddress);

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];
		if (prStaRec) {
			if (prStaRec->fgIsInUse
				&& prStaRec->eStaType == STA_TYPE_DLS_PEER
				&& EQUAL_MAC_ADDR(prStaRec->aucMacAddr,
					aucPeerMACAddress)) {
				break;
			}
		}
	}

	return prStaRec;
}

#endif

#if (CFG_SUPPORT_802_11AX == 1)
static void cnmStaRecCmdHeContentFill(
	struct STA_RECORD *prStaRec,
	struct CMD_UPDATE_STA_RECORD *prCmdContent)
{
	prCmdContent->ucVersion = CMD_UPDATE_STAREC_VER1;
	memcpy(prCmdContent->ucHeMacCapInfo, prStaRec->ucHeMacCapInfo,
		HE_MAC_CAP_BYTE_NUM);
	memcpy(prCmdContent->ucHePhyCapInfo, prStaRec->ucHePhyCapInfo,
		HE_PHY_CAP_BYTE_NUM);

	prCmdContent->u2HeRxMcsMapBW80 =
		CPU_TO_LE16(prStaRec->u2HeRxMcsMapBW80);
	prCmdContent->u2HeTxMcsMapBW80 =
		CPU_TO_LE16(prStaRec->u2HeTxMcsMapBW80);
	prCmdContent->u2HeRxMcsMapBW160 =
		CPU_TO_LE16(prStaRec->u2HeRxMcsMapBW160);
	prCmdContent->u2HeTxMcsMapBW160 =
		CPU_TO_LE16(prStaRec->u2HeTxMcsMapBW160);
	prCmdContent->u2HeRxMcsMapBW80P80 =
		CPU_TO_LE16(prStaRec->u2HeRxMcsMapBW80P80);
	prCmdContent->u2HeTxMcsMapBW80P80 =
		CPU_TO_LE16(prStaRec->u2HeTxMcsMapBW80P80);
}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
static void cnmStaRecCmdEhtContentFill(
	struct STA_RECORD *prStaRec,
	struct CMD_UPDATE_STA_RECORD *prCmdContent)
{
	prCmdContent->ucVersion = CMD_UPDATE_STAREC_VER1;
	memcpy(prCmdContent->ucEhtMacCapInfo, prStaRec->ucEhtMacCapInfo,
		EHT_MAC_CAP_BYTE_NUM);
	memcpy(prCmdContent->ucEhtPhyCapInfo, prStaRec->ucEhtPhyCapInfo,
		EHT_PHY_CAP_BYTE_NUM);
	memcpy(prCmdContent->ucEhtPhyCapInfoExt, prStaRec->ucEhtPhyCapInfoExt,
		EHT_PHY_CAP_BYTE_NUM);
	memcpy(prCmdContent->aucMcsMap20MHzSta, prStaRec->aucMcsMap20MHzSta,
		sizeof(prCmdContent->aucMcsMap20MHzSta));
	memcpy(prCmdContent->aucMcsMap80MHz, prStaRec->aucMcsMap80MHz,
		sizeof(prCmdContent->aucMcsMap80MHz));
	memcpy(prCmdContent->aucMcsMap160MHz, prStaRec->aucMcsMap160MHz,
		sizeof(prCmdContent->aucMcsMap160MHz));
	memcpy(prCmdContent->aucMcsMap320MHz, prStaRec->aucMcsMap320MHz,
		sizeof(prCmdContent->aucMcsMap320MHz));
}
#endif

