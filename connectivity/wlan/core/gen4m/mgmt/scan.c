// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#include "gl_kal.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define REPLICATED_BEACON_STRENGTH_THRESHOLD    (32)
#define ROAMING_NO_SWING_RCPI_STEP              (10)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
/* The order of aucScanLogPrefix should be aligned the order
 * of enum ENUM_SCAN_LOG_PREFIX
 */
const char aucScanLogPrefix[][SCAN_LOG_PREFIX_MAX_LEN] = {
	/* Scan */
	"[SCN:100:K2D]",	/* LOG_SCAN_REQ_K2D */
	"[SCN:200:D2F]",	/* LOG_SCAN_REQ_D2F */
	"[SCN:300:F2D]",	/* LOG_SCAN_RESULT_F2D */
	"[SCN:400:D2K]",	/* LOG_SCAN_RESULT_D2K */
	"[SCN:500:F2D]",	/* LOG_SCAN_DONE_F2D */
	"[SCN:600:D2K]",	/* LOG_SCAN_DONE_D2K */

	/* Sched scan */
	"[SCN:700:K2D]",	/* LOG_SCHED_SCAN_REQ_START_K2D */
	"[SCN:800:D2F]",        /* LOG_SCHED_SCAN_REQ_START_D2F */
	"[SCN:750:K2D]",	/* LOG_SCHED_SCAN_REQ_STOP_K2D */
	"[SCN:850:D2F]",	/* LOG_SCHED_SCAN_REQ_STOP_D2F */
	"[SCN:900:F2D]",	/* LOG_SCHED_SCAN_DONE_F2D */
	"[SCN:1000:D2K]",	/* LOG_SCHED_SCAN_DONE_D2K */

	/* Scan abort */
	"[SCN:1100:K2D]",	/* LOG_SCAN_ABORT_REQ_K2D */
	"[SCN:1200:D2F]",	/* LOG_SCAN_ABORT_REQ_D2F */
	"[SCN:1300:D2K]",	/* LOG_SCAN_ABORT_DONE_D2K */

	/* Driver only */
	"[SCN:0:D2D]",		/* LOG_SCAN_D2D */

	/* Last one */
	""			/* LOG_SCAN_MAX */
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static void scanFreeBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used by SCN to initialize its variables
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scnInit(struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;
	struct BSS_DESC *prBSSDesc;
	uint8_t *pucBSSBuff;
	uint32_t i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	pucBSSBuff = &prScanInfo->aucScanBuffer[0];

	log_dbg(SCN, TRACE, "->scnInit()\n");

	/* 4 <1> Reset STATE and Message List */
	prScanInfo->eCurrentState = SCAN_STATE_IDLE;
	prScanInfo->fgWifiOnFirstScan = TRUE;

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	prScanInfo->ucScnZeroMdrdyTimes = 0;
	prScanInfo->ucScnZeroMdrdySerCnt = 0;
	prScanInfo->ucScnZeroMdrdySubsysResetCnt = 0;
	prScanInfo->ucScnTimeoutTimes = 0;
	prScanInfo->ucScnTimeoutSubsysResetCnt = 0;
#if CFG_EXT_SCAN
	prScanInfo->ucScnZeroChannelCnt = 0;
	prScanInfo->ucScnZeroChSubsysResetCnt = 0;
#endif
#endif

	prScanInfo->rLastScanCompletedTime = (OS_SYSTIME) 0;

	LINK_INITIALIZE(&prScanInfo->rPendingMsgList);

	/* 4 <2> Reset link list of BSS_DESC_T */
	kalMemZero((void *) pucBSSBuff, SCN_MAX_BUFFER_SIZE);

	LINK_INITIALIZE(&prScanInfo->rFreeBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rBSSDescList);

	for (i = 0; i < CFG_MAX_NUM_BSS_LIST; i++) {

		prBSSDesc = (struct BSS_DESC *) pucBSSBuff;

		scanInsertBssDescToList(&prScanInfo->rFreeBSSDescList,
			prBSSDesc,
			FALSE);

		pucBSSBuff += ALIGN_4(sizeof(struct BSS_DESC));
	}
	/* Check if the memory allocation consist with
	 * this initialization function
	 */
	ASSERT(((uintptr_t) pucBSSBuff
		- (uintptr_t)&prScanInfo->aucScanBuffer[0])
		== SCN_MAX_BUFFER_SIZE);

	/* reset freest channel information */
	prScanInfo->fgIsSparseChannelValid = FALSE;

	prScanInfo->fgIsScanForFull2Partial = FALSE;

	/* reset Sched scan state */
	prScanInfo->fgSchedScanning = FALSE;
	/*Support AP Selection */
	prScanInfo->u4ScanUpdateIdx = 0;

#if (CFG_SUPPORT_WIFI_RNR == 1)
	LINK_INITIALIZE(&prScanInfo->rNeighborAPInfoList);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	LINK_INITIALIZE(&prScanInfo->rMldAPInfoList);
#endif

	kalMemZero(&(prScanInfo->rSlotInfo), sizeof(struct CHNL_IDLE_SLOT));

#if CFG_SUPPORT_SCAN_LOG
	prScanInfo->fgBcnReport = FALSE;
#endif
}	/* end of scnInit() */

void scnFreeAllPendingScanRquests(struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;
	struct MSG_HDR *prMsgHdr;
	struct MSG_SCN_SCAN_REQ *prScanReqMsg;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	/* check for pending scanning requests */
	while (!LINK_IS_EMPTY(&(prScanInfo->rPendingMsgList))) {

		/* load next message from pending list as scan parameters */
		LINK_REMOVE_HEAD(&(prScanInfo->rPendingMsgList),
			prMsgHdr, struct MSG_HDR *);
		if (prMsgHdr) {
			prScanReqMsg = (struct MSG_SCN_SCAN_REQ *) prMsgHdr;

			log_dbg(SCN, INFO, "Free scan request eMsgId[%d] ucSeqNum [%d] BSSID[%d]!!\n",
				prMsgHdr->eMsgId,
				prScanReqMsg->ucSeqNum,
				prScanReqMsg->ucBssIndex);

			cnmMemFree(prAdapter, prMsgHdr);
		} else {
			/* should not deliver to this function */
			ASSERT(0);
		}
		/* switch to next state */
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used by SCN to uninitialize its variables
 *
 * @param (none)
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scnUninit(struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	log_dbg(SCN, TRACE, "%s()\n", __func__);

	scnFreeAllPendingScanRquests(prAdapter);

	/* 4 <1> Reset STATE and Message List */
	prScanInfo->eCurrentState = SCAN_STATE_IDLE;

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	prScanInfo->ucScnZeroMdrdyTimes = 0;
	prScanInfo->ucScnZeroMdrdySerCnt = 0;
	prScanInfo->ucScnZeroMdrdySubsysResetCnt = 0;
	prScanInfo->ucScnTimeoutTimes = 0;
	prScanInfo->ucScnTimeoutSubsysResetCnt = 0;
#if CFG_EXT_SCAN
	prScanInfo->ucScnZeroChannelCnt = 0;
	prScanInfo->ucScnZeroChSubsysResetCnt = 0;
#endif
#endif

	prScanInfo->rLastScanCompletedTime = (OS_SYSTIME) 0;

	/* NOTE(Kevin): Check rPendingMsgList ? */

	/* 4 <2> Reset link list of BSS_DESC_T */
	LINK_INITIALIZE(&prScanInfo->rFreeBSSDescList);
	LINK_INITIALIZE(&prScanInfo->rBSSDescList);

#if (CFG_SUPPORT_WIFI_RNR == 1)
	while (!LINK_IS_EMPTY(&prScanInfo->rNeighborAPInfoList)) {
		struct NEIGHBOR_AP_INFO *prNeighborAPInfo;

		LINK_REMOVE_HEAD(&prScanInfo->rNeighborAPInfoList,
			prNeighborAPInfo, struct NEIGHBOR_AP_INFO *);
		cnmMemFree(prAdapter, prNeighborAPInfo);
	}
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	while (!LINK_IS_EMPTY(&prScanInfo->rMldAPInfoList)) {
		struct MLD_AP_INFO *prMldAPInfo;

		LINK_REMOVE_HEAD(&prScanInfo->rMldAPInfoList,
			prMldAPInfo, struct MLD_AP_INFO *);
		cnmMemFree(prAdapter, prMldAPInfo);
	}
#endif
}				/* end of scnUninit() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given BSSID
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucBSSID           Given BSSID.
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *scanSearchBssDescByBssid(struct ADAPTER *prAdapter,
	uint8_t aucBSSID[])
{
	return scanSearchBssDescByBssidAndSsid(prAdapter, aucBSSID,
		FALSE, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if the bit of the given bitmap is set
 * The bitmap should be unsigned int based, which means that this function
 * doesn't support other format bitmap, e.g. char array or short array
 *
 * @param[in] bit          which bit to check.
 * @param[in] bitMap       bitmap array
 * @param[in] bitMapSize   bytes of bitmap
 *
 * @return   TRUE if the bit of the given bitmap is set, FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t scanIsBitSet(uint32_t bit, uint32_t bitMap[],
		uint32_t bitMapSize)
{
	if (bit >= bitMapSize * BITS_OF_BYTE) {
		log_dbg(SCN, WARN, "bit %u is out of array range(%u bits)\n",
			bit, bitMapSize * BITS_OF_BYTE);
		return FALSE;
	} else {
		return (bitMap[bit/BITS_OF_UINT] &
			(1 << (bit % BITS_OF_UINT))) ? TRUE : FALSE;
	}
}

#if (CFG_SUPPORT_WIFI_6G == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if the bit of the given bitmap is set,
 *  for 6G PSC channel
 *
 * @param[in] bit          which bit to check.
 * @param[in] bitMap       bitmap array
 * @param[in] bitMapSize   bytes of bitmap
 *
 * @return   TRUE if the bit of the given bitmap is set, FALSE otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t scan6gPscIsBitSet(uint32_t bit, uint32_t bitMap[],
		uint32_t bitMapSize)
{
	if (((bit - 5) % 16) == 0) {
		if (bit >= bitMapSize * BITS_OF_BYTE) {
			log_dbg(SCN, WARN, "bit %u is out of array range(%u bits)\n",
				bit, bitMapSize * BITS_OF_BYTE);
			return FALSE;
		} else {
			return (bitMap[bit/BITS_OF_UINT] &
				(1 << (bit % BITS_OF_UINT))) ? TRUE : FALSE;
		}
	} else
		return FALSE;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set the bit of the given bitmap.
 * The bitmap should be unsigned int based, which means that this function
 * doesn't support other format bitmap, e.g. char array or short array
 *
 * @param[in] bit          which bit to set.
 * @param[out] bitMap      bitmap array
 * @param[in] bitMapSize   bytes of bitmap
 *
 * @return   void
 */
/*----------------------------------------------------------------------------*/
void scanSetBit(uint32_t bit, uint32_t bitMap[], uint32_t bitMapSize)
{
	if (bit >= bitMapSize * BITS_OF_BYTE) {
		log_dbg(SCN, WARN, "set bit %u to array(%u bits) failed\n",
			bit, bitMapSize * BITS_OF_BYTE);
	} else
		bitMap[bit/BITS_OF_UINT] |= 1 << (bit % BITS_OF_UINT);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Return number of bit which is set to 1 in the given bitmap.
 * The bitmap should be unsigned int based, which means that this function
 * doesn't support other format bitmap, e.g. char array or short array
 *
 * @param[in] bitMap       bitmap array
 * @param[in] bitMapSize   bytes of bitmap
 *
 * @return   number of bit which is set to 1
 */
/*----------------------------------------------------------------------------*/

uint32_t scanCountBits(uint32_t bitMap[], uint32_t bitMapSize)
{
	uint32_t count = 0;
	uint32_t value;
	int32_t arrayLen = bitMapSize/sizeof(uint32_t);
	int32_t i;

	for (i = arrayLen - 1; i >= 0; i--) {
		value = bitMap[i];
		log_dbg(SCN, TRACE, "array[%d]:%08X\n", i, value);
		while (value) {
			count += (value & 1);
			value >>= 1;
		}
	}
	return count;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set scan channel to scanReqMsg.
 *
 * @param[in]  prAdapter           Pointer to the Adapter structure.
 * @param[in]  u4ScanChannelNum    number of input channels
 * @param[in]  arChannel           channel list
 * @param[in]  fgIsOnlineScan      online scan or not
 * @param[out] prScanReqMsg        scan request msg. Set channel number and
 *                                 channel list for output
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
void scanSetRequestChannel(struct ADAPTER *prAdapter,
		uint32_t u4ScanChannelNum,
		struct RF_CHANNEL_INFO arChannel[],
		uint32_t u4ScanFlags,
		uint8_t fgIsOnlineScan,
		struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg)
{
	uint32_t i, u4Channel, eBand, u4Index;
	/*print channel info for debugging */
	uint32_t au4ChannelBitMap[SCAN_CHANNEL_BITMAP_ARRAY_LEN];
	struct SCAN_INFO *prScanInfo;
	bool fgIsLowSpanScan = FALSE;
	bool fgIsHighAccuracy = FALSE;
	uint32_t *pau4ChBitMap;
#if CFG_SUPPORT_FULL2PARTIAL_SCAN
	uint8_t fgIsFull2Partial = FALSE;
	OS_SYSTIME rCurrentTime;

	GET_CURRENT_SYSTIME(&rCurrentTime);
#endif /* CFG_SUPPORT_FULL2PARTIAL_SCAN */

	ASSERT(u4ScanChannelNum <= MAXIMUM_OPERATION_CHANNEL_LIST);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	i = u4Index = 0;
	kalMemZero(au4ChannelBitMap, sizeof(au4ChannelBitMap));
	fgIsLowSpanScan = (u4ScanFlags & NL80211_SCAN_FLAG_LOW_SPAN) >> 8;
	fgIsHighAccuracy = (u4ScanFlags & NL80211_SCAN_FLAG_HIGH_ACCURACY) >> 10;

#if CFG_SUPPORT_FULL2PARTIAL_SCAN
	/* fgIsCheckingFull2Partial should be true if it's an online scan.
	 * Next, enable full2partial if channel number to scan is
	 * larger than SCAN_FULL2PARTIAL_CHANNEL_NUM
	 */
	if (fgIsOnlineScan && (u4ScanChannelNum == 0 ||
		u4ScanChannelNum > SCAN_FULL2PARTIAL_CHANNEL_NUM) &&
		(!prAdapter->rWifiVar.fgDisablePartialScan)) {

		/* Do full scan when
		 * 1. did not do full scan yet OR
		 * 2. not APP scan and it's been 60s after last full scan
		 * 3. If high accurary scan, do full scan for huanji app
		 * 4. WifiSetting scan is low latency, so delete the
		 * condition "fgIsLowSpanScan"
		*/
		if (prScanInfo->u4LastFullScanTime == 0 ||
			(CHECK_FOR_TIMEOUT(rCurrentTime,
			prScanInfo->u4LastFullScanTime,
			SEC_TO_SYSTIME(CFG_SCAN_FULL2PARTIAL_PERIOD))) ||
			fgIsHighAccuracy) {
			prScanInfo->fgIsScanForFull2Partial = TRUE;
			prScanInfo->ucFull2PartialSeq = prScanReqMsg->ucSeqNum;
			prScanInfo->u4LastFullScanTime = rCurrentTime;
			kalMemZero(prScanInfo->au4ChannelBitMap,
				sizeof(prScanInfo->au4ChannelBitMap));
			log_dbg(SCN, INFO,
				"Full2partial: 1st full scan start, low span=%d, highA=%d\n",
				fgIsLowSpanScan, fgIsHighAccuracy);
		} else {
			log_dbg(SCN, INFO,
				"Full2partial: enable full2partial, low span=%d, highA=%d\n",
				fgIsLowSpanScan, fgIsHighAccuracy);
			fgIsFull2Partial = TRUE;
		}
	}

	if (u4ScanChannelNum == 0) {
#if CFG_EXT_SCAN
		/* We don't have channel info when u4ScanChannelNum is 0.
		 * directly do full scan
		 */
		uint8_t ucPreferBandExist = FALSE;
		uint32_t start = 1;
		uint32_t end = HW_CHNL_NUM_MAX_4G_5G;

		if (prScanReqMsg->eScanChannel == SCAN_CHANNEL_2G4) {
			end = HW_CHNL_NUM_MAX_2G4;
			ucPreferBandExist = TRUE;
		} else if (prScanReqMsg->eScanChannel == SCAN_CHANNEL_5G) {
			start = HW_CHNL_NUM_MAX_2G4 + 1;
			ucPreferBandExist = TRUE;
		}

		u4Index = 0;

		prScanReqMsg->ucChannelListNum = u4Index;
		/* No need to change eScanChannel if PreferBand exist */
		if (!ucPreferBandExist) {
			log_dbg(SCN, INFO, "No channel to scan, set to full scan\n");
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
		}
#else
		/* We don't have channel info when u4ScanChannelNum is 0.
		 * check full2partial bitmap and set scan channels
		 */
		uint32_t start = 1;
		uint32_t end = HW_CHNL_NUM_MAX_4G_5G;

		if (prScanReqMsg->eScanChannel == SCAN_CHANNEL_2G4)
			end = HW_CHNL_NUM_MAX_2G4;
		else if (prScanReqMsg->eScanChannel == SCAN_CHANNEL_5G)
			start = HW_CHNL_NUM_MAX_2G4 + 1;

		u4Index = 0;
		/* If partial scan list are too old, do full scan */
		if (!CHECK_FOR_TIMEOUT(rCurrentTime,
			prScanInfo->u4LastFullScanTime,
			SEC_TO_SYSTIME(CFG_SCAN_FULL2PARTIAL_PERIOD))) {
			for (u4Channel = start; u4Channel <= end; u4Channel++) {
				/* 2.4G and 5G check */
				if (scanIsBitSet(u4Channel,
					prScanInfo->au4ChannelBitMap,
					sizeof(prScanInfo->au4ChannelBitMap))) {
					eBand = (u4Channel <=
						HW_CHNL_NUM_MAX_2G4) ?
						BAND_2G4 : BAND_5G;
					prScanReqMsg->arChnlInfoList[u4Index].
						ucChannelNum = u4Channel;
					prScanReqMsg->arChnlInfoList[u4Index].
						eBand = eBand;
					scanSetBit(u4Channel, au4ChannelBitMap,
						sizeof(au4ChannelBitMap));
					u4Index++;
				}

#if (CFG_SUPPORT_WIFI_6G == 1)
				/* 6G check PSC channel list*/
				if (scan6gPscIsBitSet(u4Channel,
					&prScanInfo->au4ChannelBitMap[8],
					sizeof(prScanInfo->au4ChannelBitMap)
					>> 1)) {
					prScanReqMsg->arChnlInfoList[u4Index].
						ucChannelNum = u4Channel;
					prScanReqMsg->arChnlInfoList[u4Index].
						eBand = BAND_6G;
					scanSetBit(u4Channel,
						&au4ChannelBitMap[8],
						sizeof(au4ChannelBitMap) >> 1);
					u4Index++;
				}
#endif
			}
		}

		prScanReqMsg->ucChannelListNum = u4Index;
		if (u4Index == 0) {
			log_dbg(SCN, WARN, "No channel to scan, set to full scan\n");
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
		} else
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
#endif /* CFG_EXT_SCAN */
	} else
#endif /* CFG_SUPPORT_FULL2PARTIAL_SCAN */
	{
		u4Index = 0;
		for (i = 0; i < u4ScanChannelNum; i++) {
			u4Channel = arChannel[i].ucChannelNum;
			eBand = arChannel[i].eBand;
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eBand == BAND_6G)
				pau4ChBitMap = &prScanInfo->au4ChannelBitMap[8];
			else
#endif
				pau4ChBitMap = prScanInfo->au4ChannelBitMap;

#if CFG_SUPPORT_NCHO
			if (prAdapter->rNchoInfo.fgNCHOEnabled) {
				if (!(BIT(eBand) &
					prAdapter->rNchoInfo.ucBand))
					continue;
			} else
#endif
			{
			if (prScanReqMsg->eScanChannel == SCAN_CHANNEL_2G4 &&
				eBand != BAND_2G4)
				continue;
			else if (prScanReqMsg->eScanChannel ==
				SCAN_CHANNEL_5G && eBand != BAND_5G)
				continue;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prScanReqMsg->eScanChannel ==
				SCAN_CHANNEL_6G && eBand != BAND_6G)
				continue;
#endif
			}
#if CFG_SUPPORT_FULL2PARTIAL_SCAN
			if (fgIsFull2Partial && !scanIsBitSet(u4Channel,
				pau4ChBitMap,
				sizeof(prScanInfo->au4ChannelBitMap)))
				continue;
#endif /* CFG_SUPPORT_FULL2PARTIAL_SCAN */
			kalMemCopy(&prScanReqMsg->arChnlInfoList[u4Index],
					&arChannel[i],
					sizeof(struct RF_CHANNEL_INFO));
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eBand == BAND_6G) {
				scanSetBit(u4Channel, &au4ChannelBitMap[8],
					sizeof(au4ChannelBitMap) >> 1);
			} else
#endif
			{
			    scanSetBit(u4Channel, au4ChannelBitMap,
				sizeof(au4ChannelBitMap));
			}

			u4Index++;
		}
		if (u4Index == 0) {
			log_dbg(SCN, WARN, "No channel to scan, set to full scan\n");
			prScanReqMsg->ucChannelListNum = 0;
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
		} else {
			prScanReqMsg->ucChannelListNum = u4Index;
			prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
		}
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	log_dbg(SCN, INFO,
		"channel num(%u=>%u) %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
		u4ScanChannelNum, prScanReqMsg->ucChannelListNum,
		au4ChannelBitMap[7], au4ChannelBitMap[6],
		au4ChannelBitMap[5], au4ChannelBitMap[4],
		au4ChannelBitMap[3], au4ChannelBitMap[2],
		au4ChannelBitMap[1], au4ChannelBitMap[0],
		au4ChannelBitMap[15], au4ChannelBitMap[14],
		au4ChannelBitMap[13], au4ChannelBitMap[12],
		au4ChannelBitMap[11], au4ChannelBitMap[10],
		au4ChannelBitMap[9], au4ChannelBitMap[8]);
#else
	log_dbg(SCN, INFO,
		"channel num(%u=>%u) %08X %08X %08X %08X %08X %08X %08X %08X\n",
		u4ScanChannelNum, prScanReqMsg->ucChannelListNum,
		au4ChannelBitMap[7], au4ChannelBitMap[6],
		au4ChannelBitMap[5], au4ChannelBitMap[4],
		au4ChannelBitMap[3], au4ChannelBitMap[2],
		au4ChannelBitMap[1], au4ChannelBitMap[0]);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given BSSID
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucBSSID           Given BSSID.
 * @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID
 *                               with single BSSID cases)
 * @param[in] prSsid             Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *
scanSearchBssDescByBssidAndSsid(struct ADAPTER *prAdapter,
				uint8_t aucBSSID[],
				u_int8_t fgCheckSsid,
				struct PARAM_SSID *prSsid)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc;
	struct BSS_DESC *prDstBssDesc = (struct BSS_DESC *) NULL;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (prBssDesc == NULL) {
			DBGLOG(SCN, WARN,
				"NULL prBssDesc from list %u elem,%p,%p\n",
				prBSSDescList->u4NumElem,
				prBSSDescList->prPrev,
				prBSSDescList->prNext);
			return prBssDesc;
		}

		if (!(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)))
			continue;
		if (fgCheckSsid == FALSE || prSsid == NULL)
			return prBssDesc;
		if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
				prSsid->aucSsid, prSsid->u4SsidLen)) {
			return prBssDesc;
		}
		if (prDstBssDesc == NULL && prBssDesc->fgIsHiddenSSID == TRUE) {
			prDstBssDesc = prBssDesc;
			continue;
		}
		if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
			/* 20120206 frog: Equal BSSID but not SSID,
			 * SSID not hidden, SSID must be updated.
			 */
			COPY_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
				prSsid->aucSsid, (uint8_t) (prSsid->u4SsidLen));
			return prBssDesc;
		}
	}

	return prDstBssDesc;
}	/* end of scanSearchBssDescByBssid() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given Transmitter Address.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucSrcAddr         Given Source Address(TA).
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *scanSearchBssDescByTA(struct ADAPTER *prAdapter,
	uint8_t aucSrcAddr[])
{
	return scanSearchBssDescByTAAndSsid(prAdapter, aucSrcAddr, FALSE, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given Transmitter Address.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucSrcAddr         Given Source Address(TA).
 * @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID
 *                               with single BSSID cases)
 * @param[in] prSsid             Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *
scanSearchBssDescByTAAndSsid(struct ADAPTER *prAdapter,
			     uint8_t aucSrcAddr[],
			     u_int8_t fgCheckSsid,
			     struct PARAM_SSID *prSsid)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc;
	struct BSS_DESC *prDstBssDesc = (struct BSS_DESC *) NULL;

	ASSERT(prAdapter);
	ASSERT(aucSrcAddr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (prBssDesc == NULL) {
			DBGLOG(SCN, WARN,
				"NULL prBssDesc from list %u elem,%p,%p\n",
				prBSSDescList->u4NumElem,
				prBSSDescList->prPrev,
				prBSSDescList->prNext);
			return prBssDesc;
		}

		if (EQUAL_MAC_ADDR(prBssDesc->aucSrcAddr, aucSrcAddr)) {
			if (fgCheckSsid == FALSE || prSsid == NULL)
				return prBssDesc;
			if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
					prSsid->aucSsid, prSsid->u4SsidLen)) {
				return prBssDesc;
			} else if (prDstBssDesc == NULL
				&& prBssDesc->fgIsHiddenSSID == TRUE) {
				prDstBssDesc = prBssDesc;
			}
		}
	}

	return prDstBssDesc;

}	/* end of scanSearchBssDescByTA() */


#if (CFG_SUPPORT_802_11BE_MLO == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given linkid, mld Address, or ssid.
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] ucLinkId           Given Mld Link Id.
 * @param[in] aucMldAddr         Given Mld Address.
 * @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID
 *                               with single BSSID cases)
 * @param[in] prSsid             Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *
scanSearchBssDescByLinkIdMldAddrSsid(struct ADAPTER *prAdapter,
				  uint8_t ucLinkId,
				  uint8_t aucMldAddr[],
				  u_int8_t fgCheckSsid,
				  struct PARAM_SSID *prSsid)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc;
	struct BSS_DESC *prDstBssDesc = (struct BSS_DESC *) NULL;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (prBssDesc == NULL) {
			DBGLOG(SCN, WARN,
				"NULL prBssDesc from list %u elem,%p,%p\n",
				prBSSDescList->u4NumElem,
				prBSSDescList->prPrev,
				prBSSDescList->prNext);
			return prBssDesc;
		}

		if (!prBssDesc->rMlInfo.fgValid)
			continue;

		if (prBssDesc->rMlInfo.ucLinkIndex == ucLinkId &&
		    EQUAL_MAC_ADDR(prBssDesc->rMlInfo.aucMldAddr, aucMldAddr)) {
			if (fgCheckSsid == FALSE || prSsid == NULL)
				return prBssDesc;
			if (EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
					prSsid->aucSsid, prSsid->u4SsidLen)) {
				return prBssDesc;
			} else if (prDstBssDesc == NULL
				&& prBssDesc->fgIsHiddenSSID == TRUE) {
				prDstBssDesc = prBssDesc;
			}
		}
	}

	return prDstBssDesc;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given eBSSType, BSSID and Transmitter Address
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] eBSSType   BSS Type of incoming Beacon/ProbeResp frame.
 * @param[in] aucBSSID   Given BSSID of Beacon/ProbeResp frame.
 * @param[in] aucSrcAddr Given source address (TA) of Beacon/ProbeResp frame.
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *
scanSearchExistingBssDesc(struct ADAPTER *prAdapter,
			  enum ENUM_BSS_TYPE eBSSType,
			  uint8_t aucBSSID[],
			  uint8_t aucSrcAddr[])
{
	return scanSearchExistingBssDescWithSsid(prAdapter, eBSSType, aucBSSID,
		aucSrcAddr, FALSE, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to
 *        given eBSSType, BSSID and Transmitter Address
 *
 * @param[in] prAdapter   Pointer to the Adapter structure.
 * @param[in] eBSSType    BSS Type of incoming Beacon/ProbeResp frame.
 * @param[in] aucBSSID    Given BSSID of Beacon/ProbeResp frame.
 * @param[in] aucSrcAddr  Given source address (TA) of Beacon/ProbeResp frame.
 * @param[in] fgCheckSsid Need to check SSID or not. (for multiple SSID with
 *                        single BSSID cases)
 * @param[in] prSsid      Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *
scanSearchExistingBssDescWithSsid(struct ADAPTER *prAdapter,
				  enum ENUM_BSS_TYPE eBSSType,
				  uint8_t aucBSSID[],
				  uint8_t aucSrcAddr[],
				  u_int8_t fgCheckSsid,
				  struct PARAM_SSID *prSsid)
{
	struct SCAN_INFO *prScanInfo;
	struct BSS_DESC *prBssDesc, *prIBSSBssDesc;

	ASSERT(prAdapter);
	ASSERT(aucSrcAddr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	switch (eBSSType) {
	case BSS_TYPE_P2P_DEVICE:
		fgCheckSsid = FALSE;
		kal_fallthrough;
	case BSS_TYPE_INFRASTRUCTURE:
		kal_fallthrough;
	case BSS_TYPE_BOW_DEVICE:
		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
			aucBSSID, fgCheckSsid, prSsid);

		/* if (eBSSType == prBssDesc->eBSSType) */

		return prBssDesc;
	case BSS_TYPE_IBSS:
		prIBSSBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
			aucBSSID, fgCheckSsid, prSsid);
		prBssDesc = scanSearchBssDescByTAAndSsid(prAdapter,
			aucSrcAddr, fgCheckSsid, prSsid);

		/* NOTE(Kevin):
		 * Rules to maintain the SCAN Result:
		 * For AdHoc -
		 *    CASE I    We have TA1(BSSID1), but it change its
		 *              BSSID to BSSID2
		 *              -> Update TA1 entry's BSSID.
		 *    CASE II   We have TA1(BSSID1), and get TA1(BSSID1) again
		 *              -> Update TA1 entry's contain.
		 *    CASE III  We have a SCAN result TA1(BSSID1), and
		 *              TA2(BSSID2). Sooner or later, TA2 merge into
		 *              TA1, we get TA2(BSSID1)
		 *              -> Remove TA2 first and then replace TA1 entry's
		 *                 TA with TA2, Still have only one entry
		 *                 of BSSID.
		 *    CASE IV   We have a SCAN result TA1(BSSID1), and another
		 *              TA2 also merge into BSSID1.
		 *              -> Replace TA1 entry's TA with TA2, Still have
		 *                 only one entry.
		 *    CASE V    New IBSS
		 *              -> Add this one to SCAN result.
		 */
		if (prBssDesc) {
			if ((!prIBSSBssDesc) ||	/* CASE I */
			    (prBssDesc == prIBSSBssDesc)) {	/* CASE II */

				return prBssDesc;
			}

			scanFreeBssDesc(prAdapter, prBssDesc);

			return prIBSSBssDesc;
		}

		if (prIBSSBssDesc) {	/* CASE IV */

			return prIBSSBssDesc;
		}
		/* CASE V */
		break;	/* Return NULL; */
	default:
		break;
	}

	return (struct BSS_DESC *) NULL;

}	/* end of scanSearchExistingBssDesc() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Delete BSS Descriptors from current list according
 * to given Remove Policy.
 *
 * @param[in] u4RemovePolicy     Remove Policy.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescsByPolicy(struct ADAPTER *prAdapter,
				uint32_t u4RemovePolicy)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;

#if 0 /* TODO: Remove this */
	log_dbg(SCN, TRACE, ("Before Remove - Number Of SCAN Result = %ld\n",
		prBSSDescList->u4NumElem));
#endif

	if (u4RemovePolicy & SCN_RM_POLICY_TIMEOUT) {
		struct BSS_DESC *prBSSDescNext;
		OS_SYSTIME rCurrentTime;

		GET_CURRENT_SYSTIME(&rCurrentTime);

		if (LINK_IS_INVALID(prBSSDescList)) {
			DBGLOG(SCN, WARN,
				"prBSSDescList is invalid\n");
			return;
		}

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext,
			prBSSDescList, rLinkEntry, struct BSS_DESC) {
#if CFG_EXT_SCAN
			uint8_t i, fgSameSsid;
#endif

			if (prBssDesc == NULL) {
				DBGLOG(SCN, WARN,
					"NULL prBssDesc from list %u elem,%p,%p\n",
					prBSSDescList->u4NumElem,
					prBSSDescList->prPrev,
					prBSSDescList->prNext);
				return;
			}

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED)
				&& (prBssDesc->fgIsConnected
				|| prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently we
				 * are connected.
				 */
				continue;
			}

#if CFG_EXT_SCAN
			fgSameSsid = FALSE;

			/* Not remove BssDesc that has
			 * same SSID with current
			 * connected AP, for roaming.
			 */
			for (i = 0; i < KAL_AIS_NUM; i++) {
				struct BSS_INFO *prAisBssInfo = NULL;

				if (!IS_BSS_INDEX_AIS(prAdapter, i))
					continue;
				prAisBssInfo =
					aisGetAisBssInfo(prAdapter, i);

				if (kalGetMediaStateIndicated(
					prAdapter->prGlueInfo, i) !=
					MEDIA_STATE_CONNECTED)
					continue;

				if ((!prBssDesc->fgIsHiddenSSID) &&
					(EQUAL_SSID(prBssDesc->aucSSID,
					prBssDesc->ucSSIDLen,
					prAisBssInfo->aucSSID,
					prAisBssInfo->ucSSIDLen))) {
					fgSameSsid = TRUE;
					break;
				}
			}
			if (fgSameSsid)
				continue;
#endif

			if (CHECK_FOR_TIMEOUT(rCurrentTime,
				prBssDesc->rUpdateTime,
				SEC_TO_SYSTIME(wlanWfdEnabled(prAdapter) ?
					SCN_BSS_DESC_STALE_SEC_WFD :
					SCN_BSS_DESC_STALE_SEC))) {

#if 0 /* TODO: Remove this */
				log_dbg(SCN, TRACE, "Remove TIMEOUT BSS DESC(%#x):MAC: "
				MACSTR
				", Current Time = %08lx, Update Time = %08lx\n",
					prBssDesc,
					MAC2STR(prBssDesc->aucBSSID),
					rCurrentTime, prBssDesc->rUpdateTime));
#endif

				scanFreeBssDesc(prAdapter, prBssDesc);
			}
		}
	}
	if (u4RemovePolicy & SCN_RM_POLICY_OLDEST_HIDDEN) {
		struct BSS_DESC *prBssDescOldest = (struct BSS_DESC *) NULL;

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
			rLinkEntry, struct BSS_DESC) {

			if (prBssDesc == NULL) {
				DBGLOG(SCN, WARN,
					"NULL prBssDesc from list %u elem,%p,%p\n",
					prBSSDescList->u4NumElem,
					prBSSDescList->prPrev,
					prBSSDescList->prNext);
				return;
			}

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED)
				&& (prBssDesc->fgIsConnected
				|| prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently
				 * we are connected.
				 */
				continue;
			}

			if (!prBssDesc->fgIsHiddenSSID)
				continue;

			if (!prBssDescOldest) {	/* 1st element */
				prBssDescOldest = prBssDesc;
				continue;
			}

			if (TIME_BEFORE(prBssDesc->rUpdateTime,
				prBssDescOldest->rUpdateTime))
				prBssDescOldest = prBssDesc;
		}

		if (prBssDescOldest) {
#if 0 /* TODO: Remove this */
			log_dbg(SCN, TRACE, "Remove OLDEST HIDDEN BSS DESC(%#x): MAC: "
			MACSTR
			", Update Time = %08lx\n",
				prBssDescOldest,
				MAC2STR(prBssDescOldest->aucBSSID),
				prBssDescOldest->rUpdateTime);
#endif
			scanFreeBssDesc(prAdapter, prBssDescOldest);
		}
	}
	if (u4RemovePolicy & SCN_RM_POLICY_SMART_WEAKEST) {
		struct BSS_DESC *prBssDescWeakest = (struct BSS_DESC *) NULL;
		struct BSS_DESC *prBssDescWeakestSameSSID
			= (struct BSS_DESC *) NULL;
		uint32_t u4SameSSIDCount = 0;
		uint8_t j;
		uint8_t fgIsSameSSID;

		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
			rLinkEntry, struct BSS_DESC) {

			if (prBssDesc == NULL) {
				DBGLOG(SCN, WARN,
					"NULL prBssDesc from list %u elem,%p,%p\n",
					prBSSDescList->u4NumElem,
					prBSSDescList->prPrev,
					prBSSDescList->prNext);
				return;
			}

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED)
				&& (prBssDesc->fgIsConnected
				|| prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently
				 * we are connected.
				 */
				continue;
			}

			fgIsSameSSID = FALSE;
			for (j = 0; j < KAL_AIS_NUM; j++) {
				uint8_t ucBssIndex;
				struct CONNECTION_SETTINGS *prConnSettings;

				if (!AIS_MAIN_BSS_INFO(prAdapter, j))
					continue;

				ucBssIndex = AIS_MAIN_BSS_INDEX(prAdapter, j);
				prConnSettings =
				      aisGetConnSettings(prAdapter, ucBssIndex);
				if (!prConnSettings)
					continue;

				if ((!prBssDesc->fgIsHiddenSSID) &&
					(EQUAL_SSID(prBssDesc->aucSSID,
					prBssDesc->ucSSIDLen,
					prConnSettings->aucSSID,
					prConnSettings->ucSSIDLen))) {

					u4SameSSIDCount++;

					if (!prBssDescWeakestSameSSID)
						prBssDescWeakestSameSSID =
							prBssDesc;
					else if (prBssDesc->ucRCPI
					< prBssDescWeakestSameSSID->ucRCPI)
						prBssDescWeakestSameSSID =
							prBssDesc;

					fgIsSameSSID = TRUE;
				}
			}

			if (fgIsSameSSID &&
				u4SameSSIDCount
				< SCN_BSS_DESC_SAME_SSID_THRESHOLD)
				continue;

			if (!prBssDescWeakest) {	/* 1st element */
				prBssDescWeakest = prBssDesc;
				continue;
			}

			if (prBssDesc->ucRCPI < prBssDescWeakest->ucRCPI)
				prBssDescWeakest = prBssDesc;

		}

		if ((u4SameSSIDCount >= SCN_BSS_DESC_SAME_SSID_THRESHOLD)
			&& (prBssDescWeakestSameSSID))
			prBssDescWeakest = prBssDescWeakestSameSSID;

		if (prBssDescWeakest) {
#if 0 /* TODO: Remove this */
			log_dbg(SCN, TRACE, "Remove WEAKEST BSS DESC(%#x): MAC: "
			MACSTR
			", Update Time = %08lx\n",
				prBssDescOldest,
				MAC2STR(prBssDescOldest->aucBSSID),
				prBssDescOldest->rUpdateTime);
#endif

			scanFreeBssDesc(prAdapter, prBssDescWeakest);
		}
	}
	if (u4RemovePolicy & SCN_RM_POLICY_ENTIRE) {
		struct BSS_DESC *prBSSDescNext;

		if (LINK_IS_INVALID(prBSSDescList)) {
			DBGLOG(SCN, WARN,
				"prBSSDescList is invalid\n");
			return;
		}

		LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext,
			prBSSDescList, rLinkEntry, struct BSS_DESC) {

			if (prBssDesc == NULL) {
				DBGLOG(SCN, WARN,
					"NULL prBssDesc from list %u elem,%p,%p\n",
					prBSSDescList->u4NumElem,
					prBSSDescList->prPrev,
					prBSSDescList->prNext);
				return;
			}

			if ((u4RemovePolicy & SCN_RM_POLICY_EXCLUDE_CONNECTED)
				&& (prBssDesc->fgIsConnected
				|| prBssDesc->fgIsConnecting)) {
				/* Don't remove the one currently
				 * we are connected.
				 */
				continue;
			}

			scanFreeBssDesc(prAdapter, prBssDesc);
		}

	}
}	/* end of scanRemoveBssDescsByPolicy() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Delete BSS Descriptors from current list according to given BSSID.
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] aucBSSID   Given BSSID.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescByBssid(struct ADAPTER *prAdapter,
			      uint8_t aucBSSID[])
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc = (struct BSS_DESC *) NULL;
	struct BSS_DESC *prBSSDescNext;
	uint8_t ucTargetChNum = 0;
	enum ENUM_BAND eTargetBand = BAND_NULL;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;

	if (LINK_IS_INVALID(prBSSDescList)) {
		DBGLOG(SCN, WARN,
			"prBSSDescList is invalid\n");
		return;
	}

	/* Check if such BSS Descriptor exists in a valid list */
	LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			/* Because BSS descriptor will be cleared in next step,
			 * so we need to keep them temporarily.
			 */
			ucTargetChNum = prBssDesc->ucChannelNum;
			eTargetBand = prBssDesc->eBand;

			scanFreeBssDesc(prAdapter, prBssDesc);

			/* We should notify kernel to unlink BSS */
			kalRemoveBss(
				prAdapter->prGlueInfo,
				aucBSSID,
				ucTargetChNum,
				eTargetBand);

			/* BSSID is not unique, so need to traverse
			 * whols link-list
			 */
		}
	}
}	/* end of scanRemoveBssDescByBssid() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Delete BSS Descriptors from current list according to given
 * band configuration
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] eBand      Given band
 * @param[in] ucBssIndex     AIS - Remove IBSS/Infrastructure BSS
 *                           BOW - Remove BOW BSS
 *                           P2P - Remove P2P BSS
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescByBandAndNetwork(struct ADAPTER *prAdapter,
				       enum ENUM_BAND eBand,
				       uint8_t ucBssIndex)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc = (struct BSS_DESC *) NULL;
	struct BSS_DESC *prBSSDescNext;
	struct BSS_INFO *prBssInfo = NULL;
	u_int8_t fgToRemove;

	ASSERT(prAdapter);
	ASSERT(eBand <= BAND_NUM);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (eBand == BAND_NULL || prBssInfo == NULL) {
		/* no need to do anything, keep all scan result */
		return;
	}

	if (LINK_IS_INVALID(prBSSDescList)) {
		DBGLOG(SCN, WARN,
			"prBSSDescList is invalid\n");
		return;
	}

	/* Check if such BSS Descriptor exists in a valid list */
	LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {
		fgToRemove = FALSE;

		if (prBssDesc->eBand == eBand) {
			switch (prBssInfo->eNetworkType) {
			case NETWORK_TYPE_AIS:
				if ((prBssDesc->eBSSType
				    == BSS_TYPE_INFRASTRUCTURE)
				    || (prBssDesc->eBSSType == BSS_TYPE_IBSS)) {
					fgToRemove = TRUE;
				}
				break;

			case NETWORK_TYPE_P2P:
				if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE)
					fgToRemove = TRUE;
				break;

			case NETWORK_TYPE_BOW:
				if (prBssDesc->eBSSType == BSS_TYPE_BOW_DEVICE)
					fgToRemove = TRUE;
				break;

			default:
				ASSERT(0);
				break;
			}
		}

		if (fgToRemove == TRUE)
			scanFreeBssDesc(prAdapter, prBssDesc);
	}
}	/* end of scanRemoveBssDescByBand() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Clear the CONNECTION FLAG of a specified BSS Descriptor.
 *
 * @param[in] aucBSSID   Given BSSID.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveConnFlagOfBssDescByBssid(struct ADAPTER *prAdapter,
					uint8_t aucBSSID[],
					uint8_t ucBssIndex)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc = (struct BSS_DESC *) NULL;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
		rLinkEntry, struct BSS_DESC) {

		if (prBssDesc == NULL) {
			DBGLOG(SCN, WARN,
				"NULL prBssDesc from list %u elem,%p,%p\n",
				prBSSDescList->u4NumElem,
				prBSSDescList->prPrev,
				prBSSDescList->prNext);
			return;
		}

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			prBssDesc->fgIsConnected &= ~BIT(ucBssIndex);
			prBssDesc->fgIsConnecting &= ~BIT(ucBssIndex);

			/* BSSID is not unique, so need to
			 * traverse whols link-list
			 */
		}
	}
}	/* end of scanRemoveConnectionFlagOfBssDescByBssid() */

uint8_t scanCopySSID(uint8_t *pucIE, struct PARAM_SSID *prSsid)
{
	uint8_t ucSSIDChar = '\0';
	uint8_t fgIsValidSsid = FALSE;
	uint32_t i;

	if (!pucIE || IE_LEN(pucIE) > ELEM_MAX_LEN_SSID || IE_LEN(pucIE) == 0)
		return FALSE;

	for (i = 0; i < IE_LEN(pucIE); i++)
		ucSSIDChar |= SSID_IE(pucIE)->aucSSID[i];

	if (ucSSIDChar)
		fgIsValidSsid = TRUE;

	/* Update SSID to BSS Descriptor only if
	 * SSID is not hidden.
	 */
	if (fgIsValidSsid == TRUE && prSsid)
		COPY_SSID(prSsid->aucSsid,
			 prSsid->u4SsidLen,
			 SSID_IE(pucIE)->aucSSID,
			 SSID_IE(pucIE)->ucLength);

	return fgIsValidSsid;
}

#if (CFG_SUPPORT_802_11V_MBSSID == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Allocate new BSS_DESC structure
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @return   Pointer to BSS Descriptor, if has
 *           free space. NULL, if has no space.
 */
/*----------------------------------------------------------------------------*/
void scanParsingMBSSIDSubelement(struct ADAPTER *prAdapter,
	struct BSS_DESC *prTransBSS, struct IE_MBSSID *prMbssidIe)
{
	uint8_t *pucIE;
	uint16_t u2IELength, u2Offset;
	uint8_t *pucProfileIE;
	uint16_t u2ProfileLen, u2ProfileOffset;
	struct BSS_DESC *prBssDesc;
	struct IE_MBSSID_INDEX *prMbssidIdxIe;
	uint8_t aucBSSID[MAC_ADDR_LEN];
	int8_t ucBssidLsb;
	uint8_t fgIsValidSsid = FALSE;
	struct PARAM_SSID rSsid;

	if (prMbssidIe->ucMaxBSSIDIndicator == 0) /* invalid mbssid ie*/
		return;

	prTransBSS->ucMaxBSSIDIndicator = prMbssidIe->ucMaxBSSIDIndicator;
	u2Offset = 0;
	pucIE = &prMbssidIe->ucSubelements[0];
	u2IELength = IE_SIZE(prMbssidIe) - sizeof(struct IE_MBSSID);
	IE_FOR_EACH(pucIE, u2IELength, u2Offset)
	{
		fgIsValidSsid = FALSE;
		pucProfileIE = NULL;
		/*search for each nontransmitted bssid profile*/
		if (IE_ID(pucIE) == NON_TX_BSSID_PROFILE) {
			pucProfileIE = &((struct IE_HDR *)pucIE)->aucInfo[0];
			u2ProfileLen = IE_LEN(pucIE);
			u2ProfileOffset = 0;
		}
		if (pucProfileIE == NULL)
			continue;

		/*search for mbssid index ie in each profile*/
		prMbssidIdxIe = NULL;
		IE_FOR_EACH(pucProfileIE, u2ProfileLen, u2ProfileOffset) {
			if (IE_ID(pucProfileIE) == ELEM_ID_MBSSID_INDEX)
				prMbssidIdxIe =	MBSSID_INDEX_IE(pucProfileIE);

			if (IE_ID(pucProfileIE) == ELEM_ID_SSID) {
				if (!fgIsValidSsid)
					fgIsValidSsid = scanCopySSID(
						pucProfileIE, &rSsid);
			}
		}
		if (prMbssidIdxIe == NULL)
			continue;

		/*calculate BSSID of this profile*/
		kalMemCopy(aucBSSID, &prTransBSS->aucBSSID[0], MAC_ADDR_LEN);
		ucBssidLsb = aucBSSID[5] &
				((1 << prMbssidIe->ucMaxBSSIDIndicator) - 1);
		aucBSSID[5] &= ~((1 << prMbssidIe->ucMaxBSSIDIndicator) - 1);
		aucBSSID[5] |= (ucBssidLsb + prMbssidIdxIe->ucBSSIDIndex) %
				  (1 << prMbssidIe->ucMaxBSSIDIndicator);

		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
					aucBSSID, fgIsValidSsid, &rSsid);
		if (prBssDesc) {
			prBssDesc->ucMaxBSSIDIndicator =
				prMbssidIe->ucMaxBSSIDIndicator;
			prBssDesc->ucMBSSIDIndex =
				prMbssidIdxIe->ucBSSIDIndex;

			DBGLOG(SCN, TRACE, "MBSS["MACSTR
				"][%s] MaxBSSIDIndicator=%d, ucMBSSIDIndex=%d\n",
				MAC2STR(aucBSSID), rSsid.aucSsid,
				prBssDesc->ucMaxBSSIDIndicator,
				prBssDesc->ucMBSSIDIndex);
		}
	}
}
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @return   NULL, if has no space.
 */
/*----------------------------------------------------------------------------*/
void scanParseMldIE(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc,
	const uint8_t *pucIE, uint16_t u2FrameCtrl)
{
	struct MULTI_LINK_INFO rMlInfo;
	struct MULTI_LINK_INFO *prMlInfo = &rMlInfo;

	if (BE_IS_ML_CTRL_TYPE(pucIE, ML_CTRL_TYPE_BASIC)) {
		MLD_PARSE_BASIC_MLIE(prMlInfo, pucIE,
				IE_SIZE(pucIE), /* no need fragment */
				prBssDesc->aucBSSID,
				u2FrameCtrl);

		prBssDesc->rMlInfo.fgValid = prMlInfo->ucValid;

		if (!prMlInfo->ucValid)
			return;

		COPY_MAC_ADDR(prBssDesc->rMlInfo.aucMldAddr,
			prMlInfo->aucMldAddr);

		/* Check ML control that which common info exist */
		if (rMlInfo.ucMlCtrlPreBmp & ML_CTRL_LINK_ID_INFO_PRESENT)
			prBssDesc->rMlInfo.ucLinkIndex = rMlInfo.ucLinkId;

		if (rMlInfo.ucMlCtrlPreBmp & ML_CTRL_EML_CAPA_PRESENT)
			prBssDesc->rMlInfo.u2EmlCap = rMlInfo.u2EmlCap;

		if (rMlInfo.ucMlCtrlPreBmp & ML_CTRL_MLD_ID_PRESENT)
			prBssDesc->rMlInfo.ucMldId = rMlInfo.ucMldId;

		if (rMlInfo.ucMlCtrlPreBmp & ML_CTRL_MLD_CAPA_PRESENT) {
			prBssDesc->rMlInfo.u2MldCap = rMlInfo.u2MldCap;
			prBssDesc->rMlInfo.ucMaxSimuLinks =
				(rMlInfo.u2MldCap & BITS(0, 3));
		}

		prBssDesc->rMlInfo.u2ValidLinks = rMlInfo.u2ValidLinks;

		DBGLOG(ML, TRACE,
			"MldAddr="MACSTR",BSS="MACSTR
			",LinkID=%d,MaxSimu=%d,EmlCap=0x%x,MldCap=0x%x,MldType=%d\n",
			MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->rMlInfo.ucLinkIndex,
			prBssDesc->rMlInfo.ucMaxSimuLinks,
			prBssDesc->rMlInfo.u2EmlCap,
			prBssDesc->rMlInfo.u2MldCap,
			prBssDesc->rMlInfo.fgMldType);
	} else if (BE_IS_ML_CTRL_TYPE(pucIE, ML_CTRL_TYPE_RECONFIG)) {
#if (CFG_SUPPORT_ML_RECONFIG == 1)
		uint8_t i;

		MLD_PARSE_RECONFIG_MLIE(prMlInfo, pucIE, prBssDesc->aucBSSID);

		if (!prMlInfo->ucValid)
			return;

		for (i = 0; i < prMlInfo->ucProfNum; i++) {
			struct STA_PROFILE *sta = &prMlInfo->rStaProfiles[i];

			if (prBssDesc->rMlInfo.ucLinkIndex == sta->ucLinkId)
				prBssDesc->rMlInfo.u2ApRemovalTimer =
					sta->u2ApRemovalTimer;
		}
#endif /* CFG_SUPPORT_ML_RECONFIG */
	}

	return;
}
#endif /* CFG_SUPPORT_802_11BE_MLO */

#if (CFG_SUPPORT_WIFI_RNR == 1)
void scanHandleRnrSsid(struct NEIGHBOR_AP_PARAM *prScanParam,
	struct SCAN_PARAM *prAdapterScanParam,
	struct BSS_DESC *prBssDesc, uint8_t ucBssidNum)
{
	uint8_t i = 0, fgHasEqualSsid = FALSE;

	if (prAdapterScanParam->ucSSIDType &
		(SCAN_REQ_SSID_SPECIFIED | SCAN_REQ_SSID_SPECIFIED_ONLY)) {
		prScanParam->ucSSIDType = prAdapterScanParam->ucSSIDType;
	} else {
		prScanParam->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
	}

	/* For coverity check, ucBssidNum shall not smaller than 1 */
	if (ucBssidNum < 1)
		ucBssidNum = 1;

	/* Check this SSID has recorded or not */
	for (i = 0; i < prScanParam->ucSSIDNum; i++) {
		if (EQUAL_SSID(prScanParam->aucSpecifiedSSID[i],
				prScanParam->ucSpecifiedSSIDLen[i],
				prBssDesc->aucSSID, prBssDesc->ucSSIDLen)) {
			fgHasEqualSsid = TRUE;
			break;
		}
	}
	/* If no recorded, record the SSID and matching BSSID index */
	if (!fgHasEqualSsid) {
		if (prScanParam->ucSSIDNum >= CFG_SCAN_SSID_MAX_NUM) {
			DBGLOG(SCN, ERROR,
			"The ucSSIDNum has reached the maximum\n");
		} else {
			COPY_SSID(prScanParam->aucSpecifiedSSID
					[prScanParam->ucSSIDNum],
				prScanParam->ucSpecifiedSSIDLen
					[prScanParam->ucSSIDNum],
				prBssDesc->aucSSID, prBssDesc->ucSSIDLen);

			prScanParam->ucBssidMatchSsidInd[ucBssidNum - 1] =
				prScanParam->ucSSIDNum;
			log_dbg(SCN, INFO, "[%x],SSID[%s]\n",
				prScanParam->ucSSIDNum,
				HIDE(prBssDesc->aucSSID));
			prScanParam->ucSSIDNum++;
		}
	} else {
		/* If has recorded, only record matching SSID index */
		prScanParam->ucBssidMatchSsidInd[ucBssidNum - 1] = i;
	}
}

void scanHandleRnrShortSsid(
	struct NEIGHBOR_AP_PARAM *prScanParam,
	struct NEIGHBOR_AP_INFO_FIELD *prNeighborAPInfoField,
	uint8_t ucTbttInfoCnt, uint8_t ucShortSsidOffset,
	uint8_t ucBssidNum)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint8_t i = 0, ucShortSsidIdx = 0;
	uint8_t ucShortSsidNum =
		prScanParam->ucShortSSIDNum;
	uint8_t fgHasEqualShortSsid = FALSE;

	/* For coverity check, ucBssidNum shall not smaller than 1 */
	if (ucBssidNum < 1)
		ucBssidNum = 1;

	/* Check this Short SSID has recorded or not */
	for (i = 0; i < ucShortSsidNum; i++) {
		if (kalMemCmp(&prScanParam->aucShortSSID[i],
			&prNeighborAPInfoField->aucTbttInfoSet[
				ucShortSsidOffset],
			MAX_SHORT_SSID_LEN) == 0) {
			fgHasEqualShortSsid = TRUE;
			log_dbg(SCN, TRACE, "Same Short SSID, Idx[%d]\n", i);
			break;
		}
	}

	/* If no recorded, record Short SSID and matching BSSID index */
	if (!fgHasEqualShortSsid) {
		if (prScanParam->ucShortSSIDNum >= CFG_SCAN_OOB_MAX_NUM) {
			DBGLOG(SCN, ERROR,
			"The ucShortSSIDNum has reached the maximum\n");
		} else {
			kalMemCopy(&prScanParam->aucShortSSID[ucShortSsidNum],
				&prNeighborAPInfoField->aucTbttInfoSet[
					ucShortSsidOffset],
				MAX_SHORT_SSID_LEN);

			prScanParam->ucBssidMatchShortSsidInd[ucBssidNum - 1] =
				ucShortSsidNum;
			ucShortSsidIdx = ucShortSsidNum;
			prScanParam->ucShortSSIDNum++;
		}
	} else {
		/* If has recorded, record matching short SSID index */
		prScanParam->ucBssidMatchShortSsidInd[ucBssidNum - 1] = i;
		ucShortSsidIdx = i;
	}

	log_dbg(SCN, TRACE,
		"TbttInfoCnt[%x],short SSID[%x %x %x %x]\n",
		ucTbttInfoCnt,
		prScanParam->aucShortSSID[ucShortSsidIdx][0],
		prScanParam->aucShortSSID[ucShortSsidIdx][1],
		prScanParam->aucShortSSID[ucShortSsidIdx][2],
		prScanParam->aucShortSSID[ucShortSsidIdx][3]);
#endif
}

uint8_t scanGetRnrChannel(
	struct NEIGHBOR_AP_INFO_FIELD *prNeighborAPInfoField)
{
	uint8_t ucRnrChNum, ucBand;
	uint32_t u4FreqInKHz;

	/* get channel number for this neighborAPInfo */
	scanOpClassToBand(prNeighborAPInfoField->ucOpClass, &ucBand);
	switch (ucBand) {
	case BAND_2G4:
		ucBand = KAL_BAND_2GHZ;
		break;
	case BAND_5G:
		ucBand = KAL_BAND_5GHZ;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BAND_6G:
		ucBand = KAL_BAND_6GHZ;
		break;
#endif
	default:
		log_dbg(SCN, WARN, "Band%d illegal, set to 2G\n",
			ucBand);
		ucBand = KAL_BAND_2GHZ;
		break;
	}
	u4FreqInKHz =
		kalGetChannelFrequency(
		prNeighborAPInfoField->ucChannelNum,
		ucBand);
	ucRnrChNum = nicFreq2ChannelNum(u4FreqInKHz * 1000);
	return ucRnrChNum;
}

uint8_t scanProcessRnrChannel(uint8_t ucRnrChNum,
	uint8_t ucOpClass,
	struct SCAN_INFO *prScanInfo,
	struct NEIGHBOR_AP_PARAM *prScanParam,
	struct SCAN_PARAM *prAdapterScanParam)
{
	uint8_t i, ucHasSameCh = FALSE;
	enum ENUM_BAND eBand;
	uint8_t ucBand = 0;

	prScanParam->eScanChannel = SCAN_CHANNEL_SPECIFIED;

	scanOpClassToBand(ucOpClass, &ucBand);
	eBand = ucBand;

	if (prAdapterScanParam->ucSSIDType &
		(SCAN_REQ_SSID_SPECIFIED | SCAN_REQ_SSID_SPECIFIED_ONLY)) {
		struct NEIGHBOR_AP_INFO *prNeighborAPInfo = NULL;
		struct NEIGHBOR_AP_PARAM *prExistScanParam;

		LINK_FOR_EACH_ENTRY(prNeighborAPInfo,
					&prScanInfo->rNeighborAPInfoList,
					rLinkEntry, struct NEIGHBOR_AP_INFO) {
			prExistScanParam = &prNeighborAPInfo->rNeighborParam;

			for (i = 0; i < prExistScanParam->ucChannelListNum;
				i++) {
				if (ucRnrChNum == prExistScanParam->
					arChnlInfoList[i].ucChannelNum) {
					ucHasSameCh = TRUE;
					break;
				}
			}
		}
	} else {
		for (i = 0; i < prScanParam->ucChannelListNum; i++) {
			if (ucRnrChNum ==
				prScanParam->arChnlInfoList[i].ucChannelNum) {
				ucHasSameCh = TRUE;
				break;
			}
		}
	}
	if (!ucHasSameCh) {
		struct RF_CHANNEL_INFO *prRfChnlInfo;

		if (prScanParam->ucChannelListNum >= CFG_SCAN_SSID_MAX_NUM) {
			DBGLOG(SCN, ERROR,
				"The ChannelListNum has reached the maximumn");
		} else {
			prRfChnlInfo = &prScanParam->arChnlInfoList[
					prScanParam->ucChannelListNum];
			prScanParam->ucChannelListNum++;
			prRfChnlInfo->eBand = eBand;
			prRfChnlInfo->ucChannelNum = ucRnrChNum;
		}
	}
	log_dbg(SCN, LOUD, "RnrCh=%d\n", ucRnrChNum);
	return ucHasSameCh;
}

uint8_t scanValidRnrTbttInfo(uint16_t u2TbttInfoLength)
{
	uint8_t ucValidInfo;

	/*
	 * change to if/else for other compiler
	 * do NOT support ... in switch case
	 * valid case.
	 * case 1, 2, 5, 6, 7, 8, 9, 11, 12, 13 ... 255
	 */
	if (u2TbttInfoLength <= 255 &&
		u2TbttInfoLength != 0 &&
		u2TbttInfoLength != 3 &&
		u2TbttInfoLength != 4 &&
		u2TbttInfoLength != 10) {
		ucValidInfo = TRUE;
	} else {
		ucValidInfo = FALSE;
	}

	return ucValidInfo;
}

uint8_t scanSearchBssidInCurrentList(
	struct SCAN_INFO *prScanInfo, uint8_t aucBSSID[],
	struct NEIGHBOR_AP_PARAM *prCurScanParam, uint8_t ucNewLink)
{
	uint8_t i;
	struct NEIGHBOR_AP_INFO *prNeighborAPInfo = NULL;
	struct NEIGHBOR_AP_PARAM *prExistScanParam;

	/* Current prNeighborAPInfo has not insert to
	 * rNeighborAPInfoList
	 */
	if (ucNewLink)
		for (i = 0; i < CFG_SCAN_OOB_MAX_NUM; i++)
			if (EQUAL_MAC_ADDR(prCurScanParam->aucBSSID[i],
				aucBSSID))
				return TRUE;

	LINK_FOR_EACH_ENTRY(prNeighborAPInfo,
		&prScanInfo->rNeighborAPInfoList,
		rLinkEntry, struct NEIGHBOR_AP_INFO) {
		prExistScanParam = &prNeighborAPInfo->rNeighborParam;

		for (i = 0; i < CFG_SCAN_OOB_MAX_NUM; i++)
			if (EQUAL_MAC_ADDR(prExistScanParam->aucBSSID[i],
				aucBSSID))
				return TRUE;
	}

	return FALSE;
}

uint8_t scanRnrChnlIsNeedScan(struct ADAPTER *prAdapter,
	uint8_t ucRnrChNum, uint8_t ucOpClass)
{
	struct SCAN_INFO *prScanInfo;
	struct SCAN_PARAM *prScanParam;
	uint32_t i;
	struct RF_CHANNEL_INFO *prCnlInfo;
	enum ENUM_BAND eRfBand;
	uint8_t ucBand = 0;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &(prScanInfo->rScanParam);
	scanOpClassToBand(ucOpClass, &ucBand);
	eRfBand = ucBand;

	/* sanity check */
	if (ucRnrChNum == 0)
		return FALSE;

	if (!rlmDomainIsLegalChannel(prAdapter, eRfBand, ucRnrChNum))
		return FALSE;

	/* Check RNR scan channel is in current scan list or not,
	 * if RNR scan channel is 2.4G or 5G, ignore it. 6G needs
	 * to send probe request with BSSID, so keep it.
	 */
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		for (i = 0; i < prScanParam->ucChannelListNum; i++) {
			prCnlInfo = &prScanParam->arChnlInfoList[i];
			if (eRfBand == prCnlInfo->eBand &&
			    ucRnrChNum == prCnlInfo->ucChannelNum
#if (CFG_SUPPORT_WIFI_6G == 1)
			    && eRfBand != BAND_6G
#endif
			) {
				log_dbg(SCN, TRACE,
					"[ch:%d][band:%d] already in scan chnl list\n",
						ucRnrChNum, eRfBand);
				return FALSE;
			}
		}
	} else if (prScanParam->eScanChannel == SCAN_CHANNEL_FULL) {
#if (CFG_SUPPORT_WIFI_6G == 1)
		if (eRfBand != BAND_6G)
#endif
		{
			log_dbg(SCN, TRACE,
					"[ch:%d][band:%d] already in scan chnl list\n",
						ucRnrChNum, eRfBand);
			return FALSE;
		}
	}

	return TRUE;
}

uint8_t scanIsNeedRnrScan(struct ADAPTER *prAdapter,
	struct SCAN_INFO *prScanInfo)
{
	/* To speed up wifi on first scan, not to handle RNR.
	 * except WIFI 7 certification.
	 */
	if (prAdapter->rWifiVar.u4SwTestMode != ENUM_SW_TEST_MODE_SIGMA_BE
		&& prScanInfo->fgWifiOnFirstScan) {
		return FALSE;
	} else if (!prScanInfo->rScanParam.fgOobRnrParseEn
		|| prAdapter->rWifiVar.u4SwTestMode
		   == ENUM_SW_TEST_MODE_SIGMA_OCE
		|| prScanInfo->eCurrentState != SCAN_STATE_SCANNING) {
		DBGLOG(SCN, TRACE, "Skip oob scan Rnr parsing\n");
		return FALSE;
	}
	return TRUE;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Allocate new NEIGHBOR_AP_INFO structure
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @return   NULL, if has no space.
 */
/*----------------------------------------------------------------------------*/
void scanParsingRnrElement(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc, uint8_t *pucIE)
{
	uint8_t i = 0, j = 0, ucNewLink = FALSE, ucRnrChNum;
	uint8_t ucShortSsidOffset, ucBssParamOffset;
	uint8_t ucMldParamOffset = 0;
	uint8_t ucBssidNum = 0, ucOpClass = 0;
	uint8_t fgHasBssid = FALSE, fgScanEnable = FALSE;
	uint8_t fgParseBSSID = TRUE;
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	uint16_t u2TbttInfoCount, u2TbttInfoLength, u2CurrentLength = 0;
	uint8_t ucHasMlo = FALSE, ucNeedMlo = FALSE, ucHasSameCh = FALSE;
	struct NEIGHBOR_AP_INFO *prNeighborAPInfo = NULL;
	struct NEIGHBOR_AP_INFO_FIELD *prNeighborAPInfoField;
	struct NEIGHBOR_AP_PARAM *prScanParam;
	struct SCAN_PARAM *prAdapterScanParam;
	struct BSS_DESC *prBssDescTemp = NULL;
	struct SCAN_INFO *prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	struct IE_RNR *prRnr = (struct IE_RNR *) pucIE;

	if (!scanIsNeedRnrScan(prAdapter, prScanInfo))
		return;

#if CFG_SUPPORT_802_11BE_MLO
	ucNeedMlo = (prAdapter->rWifiVar.ucMldLinkMax > 1);
#endif

	while (u2CurrentLength < IE_LEN(pucIE)) {
		prNeighborAPInfoField =	(struct NEIGHBOR_AP_INFO_FIELD *)
					(prRnr->aucInfoField + u2CurrentLength);
		ucOpClass = prNeighborAPInfoField->ucOpClass;

		/* get TBTT information count and length for
		*  this neighborAPInfo
		*  Count 0 indicates one TBTT Information field is present
		*/
		u2TbttInfoCount = ((prNeighborAPInfoField->u2TbttInfoHdr &
						TBTT_INFO_HDR_COUNT)
						>> TBTT_INFO_HDR_COUNT_OFFSET)
						+ 1;
		u2TbttInfoLength = (prNeighborAPInfoField->u2TbttInfoHdr &
						TBTT_INFO_HDR_LENGTH)
						>> TBTT_INFO_HDR_LENGTH_OFFSET;

		/* Check Tbtt Info length is valid or not*/
		if (!scanValidRnrTbttInfo(u2TbttInfoLength)) {
			DBGLOG(SCN, ERROR, "Invalid TBTT info length = %d\n",
				u2TbttInfoLength);
			return;
		}

		if (u2TbttInfoLength == 7) {
			/* 7: Neighbor AP TBTT Offset + BSSID */
			ucShortSsidOffset = 0;
			ucBssParamOffset = 0;
			fgHasBssid = TRUE;
		} else if (u2TbttInfoLength == 8 ||
				   u2TbttInfoLength == 9) {
			/* 8: Neighbor AP TBTT Offset + BSSID + BSS parameters
			 * 9: Neighbor AP TBTT Offset + BSSID + BSS parameters
			 *	  + 20MHz PSD
			 */
			ucShortSsidOffset = 0;
			ucBssParamOffset = 7;
			fgHasBssid = TRUE;
		} else if (u2TbttInfoLength == 10) {
			/* 10: Neighbor AP TBTT Offset + BSSID + MLD Para */
			ucShortSsidOffset = 0;
			ucBssParamOffset = 0;
			ucMldParamOffset = 7;
			fgHasBssid = TRUE;
#if CFG_SUPPORT_802_11BE_MLO
			ucHasMlo = TRUE;
#endif
		} else if (u2TbttInfoLength == 11) {
			/* 11: Neighbor AP TBTT Offset + BSSID + Short SSID */
			ucShortSsidOffset = 7;
			ucBssParamOffset = 0;
			fgHasBssid = TRUE;
		} else if (u2TbttInfoLength == 12 ||
					u2TbttInfoLength == 13 ||
					(u2TbttInfoLength >= 16 &&
					u2TbttInfoLength <= 255)) {
			/* 12: Neighbor AP TBTT Offset + BSSID + Short SSID
			 *	   + BSS parameters
			 * 13: Neighbor AP TBTT Offset + BSSID + Short SSID
			 *	   + BSS parameters + 20MHz PSD
			 */
			 /* 16: Neighbor AP TBTT Offset + BSSID + Short SSID
			 *     + BSS parameters + 20MHz PSD + MLD Parameter
			 */
			ucShortSsidOffset = 7;
			ucBssParamOffset = 11;
			ucMldParamOffset = 13;
			fgHasBssid = TRUE;
#if CFG_SUPPORT_802_11BE_MLO
			ucHasMlo = TRUE;
#endif
		} else {
			/* only support neighbor AP info with
			*  BSSID
			*/
			DBGLOG(SCN, WARN,
				"RNR w/o BSSID, length(%d,%d),TBTT(%d,%d)\n",
				IE_LEN(pucIE), u2CurrentLength,
				u2TbttInfoCount, u2TbttInfoLength);
			u2CurrentLength += SCAN_TBTT_INFO_SET_OFFSET +
				(u2TbttInfoCount * u2TbttInfoLength);
			continue;
		}

		/* If opClass is not 6G, no need to do extra scan
		 * directly check next neighborAPInfo if exist
		 */
		if (!IS_6G_OP_CLASS(ucOpClass) && (!ucNeedMlo || !ucHasMlo)) {
			DBGLOG(SCN, TRACE,
				"No need RNR, op(%d) mlo(need=%d,has=%d)\n",
				ucOpClass, ucNeedMlo, ucHasMlo);

			/* Calculate next NeighborAPInfo's index if exists */
			u2CurrentLength += SCAN_TBTT_INFO_SET_OFFSET +
				(u2TbttInfoCount * u2TbttInfoLength);
			continue;
		} else {
			/* RNR bring 6G channel, but chip not support 6G */
			/* Calculate next NeighborAPInfo's index if exists */
#if !(CFG_SUPPORT_WIFI_6G) && (CFG_SUPPORT_802_11BE_MLO == 0)
			u2CurrentLength += SCAN_TBTT_INFO_SET_OFFSET +
				(u2TbttInfoCount * u2TbttInfoLength);
			continue;
#endif
		}

		ucNewLink = FALSE;
		/* peek tail NeighborAPInfo from list to save information */
		prNeighborAPInfo = LINK_PEEK_TAIL(
		    &prScanInfo->rNeighborAPInfoList, struct NEIGHBOR_AP_INFO,
		    rLinkEntry);

		/* Check current NeighborAPInfo recorded BSSID count*/
		if (prNeighborAPInfo) {
			for (i = 0; i < CFG_SCAN_OOB_MAX_NUM; i++)
				if (EQUAL_MAC_ADDR(prNeighborAPInfo->
					rNeighborParam.aucBSSID[i],
						aucNullAddr)) {
					ucBssidNum = i;
					break;
				}
		}
		/* If list is empty or tail NeighborAPInfo BssidNum = MAX,
		*  generate a new NeighborAPInfo.
		*/
		if (prNeighborAPInfo == NULL || i == CFG_SCAN_OOB_MAX_NUM ||
			prNeighborAPInfo->rNeighborParam.ucChannelListNum >=
			CFG_SCAN_SSID_MAX_NUM) {
			prNeighborAPInfo =
			    (struct NEIGHBOR_AP_INFO *)cnmMemAlloc(prAdapter,
			    RAM_TYPE_BUF, sizeof(struct NEIGHBOR_AP_INFO));

			if (prNeighborAPInfo == NULL) {
				DBGLOG(SCN, ERROR,
					"cnmMemAlloc for prNeighborAPInfo failed!\n");
				return;
			}
			kalMemZero(prNeighborAPInfo,
					sizeof(struct NEIGHBOR_AP_INFO));
			ucNewLink = TRUE;
		}
		prScanParam = &prNeighborAPInfo->rNeighborParam;
		prAdapterScanParam =
			&(prAdapter->rWifiVar.rScanInfo.rScanParam);

		/* If NeighborAPInfo is new generated, init some variables */
		if (ucNewLink) {
			ucBssidNum = 0;
			prScanParam->ucSSIDNum = 0;
			prScanParam->ucShortSSIDNum = 0;
			/* Init value = CFG_SCAN_OOB_MAX_NUM, if init value = 0
			*  will let FW confuse to match SSID ind 0.
			*/
			kalMemSet(prScanParam->ucBssidMatchSsidInd,
				CFG_SCAN_OOB_MAX_NUM,
				sizeof(prScanParam->ucBssidMatchSsidInd));
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			kalMemSet(prScanParam->ucBssidMatchShortSsidInd,
				CFG_SCAN_OOB_MAX_NUM,
				sizeof(prScanParam->ucBssidMatchShortSsidInd));
#endif

			if (prAdapterScanParam->ucSSIDType &
					(SCAN_REQ_SSID_SPECIFIED |
					SCAN_REQ_SSID_SPECIFIED_ONLY)) {
				for (i = 0; i < prAdapterScanParam->ucSSIDNum &&
					i < CFG_SCAN_SSID_MAX_NUM; i++) {
					prScanParam->ucSSIDNum++;
					COPY_SSID(
						prScanParam->
							aucSpecifiedSSID[i],
						prScanParam->
							ucSpecifiedSSIDLen[i],
						prAdapterScanParam->
							aucSpecifiedSSID[i],
						prAdapterScanParam->
							ucSpecifiedSSIDLen[i]);
				}
				prScanParam->ucSSIDType =
					prAdapterScanParam->ucSSIDType;
				DBGLOG(SCN, TRACE,
					"OOB scan specific SSIDNum %d\n",
					prScanParam->ucSSIDNum);
			} else {
				prScanParam->ucScnFuncMask |=
					ENUM_SCN_USE_PADDING_AS_BSSID;
				prScanParam->ucSSIDType =
					SCAN_REQ_SSID_WILDCARD;
			}
			/* If current scan has random MAC,
			 * then RNR scan should use random MAC also.
			 */
			if (kalIsValidMacAddr(prAdapterScanParam
						->aucRandomMac)) {
				prScanParam->ucScnFuncMask |=
					(ENUM_SCN_RANDOM_MAC_EN |
					ENUM_SCN_RANDOM_SN_EN);
				COPY_MAC_ADDR(prScanParam->aucRandomMac,
					prAdapterScanParam->aucRandomMac);
			}
		}

		/* Get RNR channel */
		ucRnrChNum = scanGetRnrChannel(prNeighborAPInfoField);
		if (!scanRnrChnlIsNeedScan(prAdapter, ucRnrChNum, ucOpClass)) {
			DBGLOG(SCN, TRACE, "Ignore RNR chnl(%d) OpClass(%d)!\n",
				ucRnrChNum, ucOpClass);
			if (ucNewLink)
				cnmMemFree(prAdapter, prNeighborAPInfo);
			/* Calculate next NeighborAPInfo's index if exists */
			u2CurrentLength += SCAN_TBTT_INFO_SET_OFFSET +
				(u2TbttInfoCount * u2TbttInfoLength);
			continue;
		}

		for (i = 0; i < u2TbttInfoCount; i++) {
			j = i * u2TbttInfoLength;

			/* If this BSSID existed and update time diff is
			 * smaller than 20s, or existed in current scan request,
			 * bypass it.
			 */
			fgParseBSSID = TRUE;
			if (prScanParam->ucScnFuncMask &
					ENUM_SCN_USE_PADDING_AS_BSSID)
				prBssDescTemp = scanSearchBssDescByBssid(
						prAdapter,
						&prNeighborAPInfoField->
						aucTbttInfoSet[j + 1]);
			if ((prBssDescTemp && prBssDescTemp->ucChannelNum
				== ucRnrChNum &&
			    !CHECK_FOR_TIMEOUT(kalGetTimeTick(),
				prBssDescTemp->rUpdateTime,
				SEC_TO_SYSTIME(SCN_BSS_DESC_STALE_SEC))) ||
			    scanSearchBssidInCurrentList(prScanInfo,
				&prNeighborAPInfoField->aucTbttInfoSet[j + 1],
				prScanParam, ucNewLink))
				fgParseBSSID = FALSE;

			if (EQUAL_MAC_ADDR(&prNeighborAPInfoField->
					aucTbttInfoSet[j + 1], aucNullAddr))
				fgParseBSSID = FALSE;

			if (!fgParseBSSID)
				continue;

			fgScanEnable = TRUE;
			if (ucBssidNum < CFG_SCAN_OOB_MAX_NUM) {
				if (prScanParam->ucScnFuncMask &
					ENUM_SCN_USE_PADDING_AS_BSSID) {
					kalMemCopy(prScanParam->
						aucBSSID[ucBssidNum],
						&prNeighborAPInfoField->
						aucTbttInfoSet[j + 1],
						MAC_ADDR_LEN);
					prScanParam->
						ucBssidMatchCh[ucBssidNum] =
						ucRnrChNum;
					ucBssidNum++;
				}
			} else {
				/* This NeighborAPInfo saved BSSID = MAX,
				*  re-generate one. Remaining TBTT Info in this
				*  neighbor AP Info will be handled in next
				*  time.
				*/
				break;
			}

			if ((ucShortSsidOffset != 0) &&
				(prScanParam->ucScnFuncMask &
				 ENUM_SCN_USE_PADDING_AS_BSSID) &&
				(prScanParam->ucShortSSIDNum
				< CFG_SCAN_SSID_MAX_NUM)) {
				scanHandleRnrShortSsid(prScanParam,
					prNeighborAPInfoField,
					i, j + ucShortSsidOffset,
					ucBssidNum);
			}
			if (ucBssParamOffset != 0 &&
				prScanParam->ucSSIDNum <
					CFG_SCAN_SSID_MAX_NUM &&
				(prScanParam->ucScnFuncMask &
				 ENUM_SCN_USE_PADDING_AS_BSSID) &&
				(prNeighborAPInfoField->aucTbttInfoSet[j +
				ucBssParamOffset] &
				TBTT_INFO_BSS_PARAM_SAME_SSID))
				scanHandleRnrSsid(prScanParam,
						prAdapterScanParam,
						prBssDesc, ucBssidNum);

			if (ucMldParamOffset != 0) {
				uint32_t u4MldParam = 0;
				uint8_t ucMldId, ucMldLinkId;
				uint8_t	ucBssParamChangeCount, ucDisabledLink;

				kalMemCopy(&u4MldParam,
					&prNeighborAPInfoField->aucTbttInfoSet[
					j + ucMldParamOffset],
					sizeof(u4MldParam));
				ucMldId = (u4MldParam & MLD_PARAM_MLD_ID_MASK);
				ucMldLinkId = (u4MldParam &
					MLD_PARAM_LINK_ID_MASK) >>
					MLD_PARAM_LINK_ID_SHIFT;
				ucBssParamChangeCount = (u4MldParam &
				       MLD_PARAM_BSS_PARAM_CHANGE_COUNT_MASK) >>
				       MLD_PARAM_BSS_PARAM_CHANGE_COUNT_SHIFT;
				ucDisabledLink = !!(u4MldParam &
					MLD_PARAM_DISABLED_LINK);
#if CFG_SUPPORT_802_11BE_MLO
				if (ucDisabledLink)
					prBssDesc->rMlInfo.u2DisabledLinks |=
						BIT(ucMldLinkId);
				else
					prBssDesc->rMlInfo.u2DisabledLinks &=
						~BIT(ucMldLinkId);
#endif /* CFG_SUPPORT_802_11BE_MLO */
			}
		}
		/* Calculate next NeighborAPInfo's index if exists */
		u2CurrentLength += SCAN_TBTT_INFO_SET_OFFSET +
			(u2TbttInfoCount * u2TbttInfoLength);

		if ((fgHasBssid && fgScanEnable) ||
			prScanParam->ucSSIDType
			& (SCAN_REQ_SSID_SPECIFIED |
			SCAN_REQ_SSID_SPECIFIED_ONLY)) {
			ucHasSameCh = scanProcessRnrChannel(
					ucRnrChNum, ucOpClass,
					prScanInfo, prScanParam,
					prAdapterScanParam);

			/* If SSIDType is SCAN_REQ_SSID_SPECIFIED or
			 *  SCAN_REQ_SSID_SPECIFIED_ONLY,
			 *  ucScnFuncMask will not be ENUM_SCN_USE_
			 *  PADDING_AS_BSSID and therefore will not
			 *  copy BSSID, so fgScanEnable may be FALSE
			 */
			if (ucNewLink && !ucHasSameCh) {
				LINK_INSERT_TAIL(
					&prScanInfo->rNeighborAPInfoList,
					&prNeighborAPInfo->rLinkEntry);
				ucNewLink = FALSE;
			}

			log_dbg
			(SCN, TRACE, "RnR ch[%d,%d,%d,%d,%d,%d,%d,%d]\n",
				prScanParam->arChnlInfoList[0].ucChannelNum,
				prScanParam->arChnlInfoList[1].ucChannelNum,
				prScanParam->arChnlInfoList[2].ucChannelNum,
				prScanParam->arChnlInfoList[3].ucChannelNum,
				prScanParam->arChnlInfoList[4].ucChannelNum,
				prScanParam->arChnlInfoList[5].ucChannelNum,
				prScanParam->arChnlInfoList[6].ucChannelNum,
				prScanParam->arChnlInfoList[7].ucChannelNum);
		}

		/* Only handle RnR with BSSID */
		if (fgHasBssid && fgScanEnable) {
			if (ucNewLink) {
				LINK_INSERT_TAIL(
					&prScanInfo->rNeighborAPInfoList,
					&prNeighborAPInfo->rLinkEntry);
				ucNewLink = FALSE;
			}

			log_dbg(SCN, TRACE,
			"RnR for Match[%d %d %d %d] (Len:%d) Num(%d)\n",
				prScanParam->ucBssidMatchCh[0],
				prScanParam->ucBssidMatchCh[1],
				prScanParam->ucBssidMatchCh[2],
				prScanParam->ucBssidMatchCh[3],
				prScanParam->u2IELen,
				prScanInfo->rNeighborAPInfoList.u4NumElem);
			log_dbg(SCN, TRACE,
				"RnR for Match[%d %d %d %d]\n",
				prScanParam->ucBssidMatchSsidInd[0],
				prScanParam->ucBssidMatchSsidInd[1],
				prScanParam->ucBssidMatchSsidInd[2],
				prScanParam->ucBssidMatchSsidInd[3]);
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			log_dbg(SCN, TRACE,
				"RnR for Match[%d %d %d %d]\n",
				prScanParam->ucBssidMatchShortSsidInd[0],
				prScanParam->ucBssidMatchShortSsidInd[1],
				prScanParam->ucBssidMatchShortSsidInd[2],
				prScanParam->ucBssidMatchShortSsidInd[3]);
#endif
			log_dbg(SCN, TRACE,
					"RnrIe " MACSTR " " MACSTR " " MACSTR
					" " MACSTR "\n",
					MAC2STR(prScanParam->aucBSSID[0]),
					MAC2STR(prScanParam->aucBSSID[1]),
					MAC2STR(prScanParam->aucBSSID[2]),
					MAC2STR(prScanParam->aucBSSID[3]));
		}
		fgScanEnable = FALSE;

		if (ucNewLink)
			cnmMemFree(prAdapter, prNeighborAPInfo);
	}
}
#endif /* CFG_SUPPORT_WIFI_RNR */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Allocate new BSS_DESC structure
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 *
 * @return   Pointer to BSS Descriptor, if has
 *           free space. NULL, if has no space.
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *scanAllocateBssDesc(struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prFreeBSSDescList;
	struct BSS_DESC *prBssDesc;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

	LINK_REMOVE_HEAD(prFreeBSSDescList, prBssDesc, struct BSS_DESC *);

	if (prBssDesc) {
		struct LINK *prBSSDescList;

		prBSSDescList = &prScanInfo->rBSSDescList;

		/* NOTE(Kevin): In current design, this new empty
		 * struct BSS_DESC will be inserted to BSSDescList immediately.
		 */
		scanInsertBssDescToList(prBSSDescList,
			prBssDesc,
			TRUE);

		log_dbg(SCN, LOUD, "Alloc Bss(%p)\n", prBssDesc);
	}

	return prBssDesc;

}	/* end of scanAllocateBssDesc() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Free BSS_DESC structure
 *
 * @param[in] prAdapter		Pointer to the Adapter structure.
 * @param[in] prBssDesc		Pointer to BSS Descriptor
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
static void scanFreeBssDesc(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc)
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prFreeBSSDescList;
	struct LINK *prBSSDescList;

	if (!prBssDesc)
		return;

	log_dbg(SCN, LOUD, "Free Bss(%p): " MACSTR "\n",
		prBssDesc, MAC2STR(prBssDesc->aucBSSID));

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prBSSDescList = &prScanInfo->rBSSDescList;
	prFreeBSSDescList = &prScanInfo->rFreeBSSDescList;

	prBssDesc->fgIsInUse = FALSE;

	/* Remove this BSS Desc from the BSS Desc list */
	scanRemoveBssDescFromList(prAdapter,
		prBSSDescList,
		prBssDesc);

	/* Return this BSS Desc to the free BSS Desc list. */
	scanInsertBssDescToList(prFreeBSSDescList,
		prBssDesc,
		FALSE);
}

void scanSetChannelAndRCPI(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
	enum ENUM_BAND eHwBand, uint8_t ucIeDsChannelNum,
	uint8_t ucIeHtChannelNum, struct BSS_DESC *prBssDesc)
{
	uint8_t ucRxRCPI;
	uint8_t ucHwChannelNum;

	/* 4 <4> Update information from HIF RX Header */
	/* 4 <4.1> Get TSF comparison result */
	prBssDesc->fgIsLargerTSF = prSwRfb->ucTcl;

	/* 4 <4.2> Get Band information */
	prBssDesc->eBand = eHwBand;

	/* 4 <4.2> Get channel and RCPI information */
	ucHwChannelNum = prSwRfb->ucChnlNum;

	ucRxRCPI = nicRxGetRcpiValueFromRxv(prAdapter, RCPI_MODE_MAX, prSwRfb);
	if (prBssDesc->eBand == BAND_2G4) {
		/* Update RCPI if in right channel */
		if (ucIeDsChannelNum >= 1 && ucIeDsChannelNum <= 14) {

			/* Receive Beacon/ProbeResp frame
			 * from adjacent channel.
			 */
			if ((ucIeDsChannelNum == ucHwChannelNum)
				|| (ucRxRCPI > prBssDesc->ucRCPI))
				prBssDesc->ucRCPI = ucRxRCPI;
			/* trust channel information brought by IE */
			prBssDesc->ucChannelNum = ucIeDsChannelNum;
		} else if (ucIeHtChannelNum >= 1
			&& ucIeHtChannelNum <= 14) {
			/* Receive Beacon/ProbeResp frame
			 * from adjacent channel.
			 */
			if ((ucIeHtChannelNum == ucHwChannelNum)
				|| (ucRxRCPI > prBssDesc->ucRCPI))
				prBssDesc->ucRCPI = ucRxRCPI;
			/* trust channel information brought by IE */
			prBssDesc->ucChannelNum = ucIeHtChannelNum;
		} else {
			prBssDesc->ucRCPI = ucRxRCPI;
			prBssDesc->ucChannelNum = ucHwChannelNum;
		}
	}
	/* 5G Band */
	else if (prBssDesc->eBand == BAND_5G) {
		if (ucIeHtChannelNum >= 36 && ucIeHtChannelNum < 200) {
			/* Receive Beacon/ProbeResp frame
			 * from adjacent channel.
			 */
			if ((ucIeHtChannelNum == ucHwChannelNum)
				|| (ucRxRCPI > prBssDesc->ucRCPI))
				prBssDesc->ucRCPI = ucRxRCPI;
			/* trust channel information brought by IE */
			prBssDesc->ucChannelNum = ucIeHtChannelNum;
		} else {
			/* Always update RCPI */
			prBssDesc->ucRCPI = ucRxRCPI;
			prBssDesc->ucChannelNum = ucHwChannelNum;
		}
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssDesc->eBand == BAND_6G) {
		log_dbg(SCN, TRACE,
			"RxRSSI=%d IE_PriCh:%d & RXD_ChNum:%d\n",
			RCPI_TO_dBm(ucRxRCPI), prBssDesc->ucChannelNum,
			ucHwChannelNum);
		/* update RCPI when ChNum and HwChNum is matched */
		if (prBssDesc->ucChannelNum != ucHwChannelNum) {
			log_dbg(SCN, INFO,
				"IE_PriCh:%d mismatch with RXD_ChNum:%d\n",
				prBssDesc->ucChannelNum, ucHwChannelNum);

			if (!prBssDesc->fgIsHE6GPresent) {
				prBssDesc->ucRCPI = ucRxRCPI;
				prBssDesc->ucChannelNum = ucHwChannelNum;
			}
		} else
		    prBssDesc->ucRCPI = ucRxRCPI;
	}
#endif

	/* set low RCPI if this swrfb is duplicated by driver */
	if (prBssDesc->fgDriverGen) {
		prBssDesc->ucRCPI = RCPI_FOR_DONT_ROAM;
		log_dbg(SCN, INFO,
			MACSTR" is driver gen, set rssi %d\n",
			MAC2STR(prBssDesc->aucBSSID),
			RCPI_TO_dBm(prBssDesc->ucRCPI));
	}
}

void scanParseExtCapIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc)
{
	struct IE_EXT_CAP *prExtCap = NULL;

	prExtCap = (struct IE_EXT_CAP *) pucIE;

	if (!prExtCap)
		return;

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
	GET_EXT_CAP(prExtCap->aucCapabilities, prExtCap->ucLength,
		ELEM_EXT_CAP_BSS_TRANSITION_BIT, prBssDesc->fgSupportBTM);
#endif

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
	/* Get extended spectrum management capable */
	GET_EXT_CAP(prExtCap->aucCapabilities, prExtCap->ucLength,
		ELEM_EXT_CAP_EXT_SPEC_MGMT_CAPABLE_BIT,
		prBssDesc->fgExtSpecMgmtCap);

	DBGLOG(SCN, TRACE,
		"BSSID[" MACSTR "] SSID:%s: Ext Spec Mgmt Cap[%d]\n",
		MAC2STR(prBssDesc->aucBSSID),
		prBssDesc->aucSSID,
		prBssDesc->fgExtSpecMgmtCap);
#else
	DBGLOG(SCN, TRACE,
		"Extented capabilities IE present,BSSID[" MACSTR "] SSID:%s\n",
		MAC2STR(prBssDesc->aucBSSID),
		prBssDesc->aucSSID);

	DBGLOG_MEM8(SCN, LOUD, prExtCap, IE_SIZE(prExtCap));

#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This API parses Beacon/ProbeResp frame and insert extracted
 *        BSS_DESC structure with IEs into
 *        prAdapter->rWifiVar.rScanInfo.aucScanBuffer
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] prSwRfb        Pointer to the receiving frame buffer.
 *
 * @return   Pointer to BSS Descriptor
 *           NULL if the Beacon/ProbeResp frame is invalid
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *scanAddToBssDesc(struct ADAPTER *prAdapter,
				  struct SW_RFB *prSwRfb)
{
	struct BSS_DESC *prBssDesc = NULL;
	struct SCAN_PARAM *prScanParam;
	uint16_t u2CapInfo;
	enum ENUM_BSS_TYPE eBSSType = BSS_TYPE_INFRASTRUCTURE;

	uint8_t *pucIE;
	uint16_t u2IELength;
	int iPayloadOffset = 0;
	uint16_t u2Offset = 0;

	struct WLAN_BEACON_FRAME *prWlanBeaconFrame = NULL;
	struct IE_SSID *prIeSsid = NULL;
	struct IE_SUPPORTED_RATE_IOT *prIeSupportedRate = NULL;
	struct IE_EXT_SUPPORTED_RATE *prIeExtSupportedRate = NULL;
	uint8_t ucIeDsChannelNum = 0;
	uint8_t ucIeHtChannelNum = 0;
	u_int8_t fgIsValidSsid = FALSE;
	struct PARAM_SSID rSsid;
	uint64_t u8Timestamp;
	u_int8_t fgIsNewBssDesc = FALSE;

	uint32_t i;
	uint8_t ucSSIDChar;
	/* PUINT_8 pucDumpIE; */
	enum ENUM_BAND eHwBand = BAND_NULL;
	u_int8_t fgBandMismatch = FALSE;
	uint8_t ucSubtype;
	u_int8_t fgIsProbeResp = FALSE;
	u_int8_t ucPowerConstraint = 0;
	u_int8_t ucChnlNum = 0;
	uint16_t u2CurrCountryCode = COUNTRY_CODE_NULL;
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
	struct IE_TX_PWR_ENV_FRAME *prTxPwrEnvIE
		= (struct IE_TX_PWR_ENV_FRAME *) NULL;
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	uint8_t fgIsHE6GPresent = FALSE;
	uint8_t uc6GHeRegInfo = 0;
	uint8_t fgPwrMode6GSupport = TRUE;
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeCurr = PWR_MODE_6G_LPI;
	uint8_t fg6GPwrModeValid = FALSE;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo;
#endif

	struct IE_COUNTRY *prCountryIE = NULL;
	struct RX_DESC_OPS_T *prRxDescOps;
#if (CFG_SUPPORT_802_11AX == 1)
#if (CFG_SUPPORT_HE_ER == 1)
	struct _IE_HE_OP_T *prHeOp;
#endif
	struct _IE_HE_CAP_T *prHeCap = NULL;
#endif

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	prScanParam = &prAdapter->rWifiVar.rScanInfo.rScanParam;

	eHwBand = prSwRfb->eRfBand;
	prWlanBeaconFrame = prSwRfb->pvHeader;
	ucSubtype = (*(uint8_t *) (prSwRfb->pvHeader) &
			MASK_FC_SUBTYPE) >> OFFSET_OF_FC_SUBTYPE;

	ucChnlNum = prSwRfb->ucChnlNum;

	WLAN_GET_FIELD_16(&prWlanBeaconFrame->u2CapInfo, &u2CapInfo);
	WLAN_GET_FIELD_64(&prWlanBeaconFrame->au4Timestamp[0], &u8Timestamp);

	/* decide BSS type */
	switch (u2CapInfo & CAP_INFO_BSS_TYPE) {
	case CAP_INFO_ESS:
		/* It can also be Group Owner of P2P Group. */
		eBSSType = BSS_TYPE_INFRASTRUCTURE;
		break;

	case CAP_INFO_IBSS:
		eBSSType = BSS_TYPE_IBSS;
		break;
	case 0:
		/* The P2P Device shall set the ESS bit of
		 * the Capabilities field in the Probe Response fame to 0
		 * and IBSS bit to 0. (3.1.2.1.1)
		 */
		eBSSType = BSS_TYPE_P2P_DEVICE;
		break;

#if CFG_ENABLE_BT_OVER_WIFI
		/* @TODO: add rule to identify BOW beacons */
#endif

	default:
		log_dbg(SCN, WARN, "Skip unknown bss type(%u)\n", u2CapInfo);
		return NULL;
	}

	/* 4 <1.1> Pre-parse SSID IE and channel info */
	pucIE = prWlanBeaconFrame->aucInfoElem;
	u2IELength = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
	    (uint16_t) OFFSET_OF(struct WLAN_BEACON_FRAME_BODY, aucInfoElem[0]);

	kalMemZero(&rSsid, sizeof(rSsid));
	rSsid.aucSsid[ELEM_MAX_LEN_SSID-1] = '\0';
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		/* Error handling for disorder IE that IE length is 0 */
		if (IE_ID(pucIE) != ELEM_ID_SSID && IE_LEN(pucIE) == 0)
			continue;
		switch (IE_ID(pucIE)) {
		case ELEM_ID_SSID:
			if (!fgIsValidSsid)
				fgIsValidSsid = scanCopySSID(pucIE, &rSsid);
			break;
		case ELEM_ID_DS_PARAM_SET:
			if (IE_LEN(pucIE)
				== ELEM_MAX_LEN_DS_PARAMETER_SET) {
				ucIeDsChannelNum
					= DS_PARAM_IE(pucIE)->ucCurrChnl;
			}
			break;

		case ELEM_ID_HT_OP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eHwBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore HT OP IE at 6G\n");
				break;
			}
#endif
			if (IE_LEN(pucIE) == (sizeof(struct IE_HT_OP) - 2))
				ucIeHtChannelNum = ((struct IE_HT_OP *) pucIE)
					->ucPrimaryChannel;
			break;

		case ELEM_ID_COUNTRY_INFO:
		{
			prCountryIE = (struct IE_COUNTRY *) pucIE;

			u2CurrCountryCode =
			(((uint16_t) prCountryIE->aucCountryStr[0]) << 8) |
			((uint16_t) prCountryIE->aucCountryStr[1]);

			break;
		}
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		case ELEM_ID_RESERVED:
		{
			struct _6G_OPER_INFOR_T *pr6gOperInfor = NULL;
			struct _IE_HE_OP_T *prHeOp = NULL;
			uint32_t u4Offset = 0;

			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP) {
				prHeOp = (struct _IE_HE_OP_T *) pucIE;
				u4Offset = OFFSET_OF(struct _IE_HE_OP_T,
						aucVarInfo[0]);
				if ((eHwBand == BAND_6G) &&
					HE_IS_6G_OP_INFOR_PRESENT(
					prHeOp->ucHeOpParams)) {
					fgIsHE6GPresent = TRUE;
					pr6gOperInfor =
						(struct _6G_OPER_INFOR_T *)
						(pucIE + u4Offset);
					uc6GHeRegInfo =
						pr6gOperInfor->rControl
						.bits.RegulatoryInfo;
				} else {
					fgIsHE6GPresent = FALSE;
				}
			}
			break;
		}
#endif
		default:
			break;
		}
	}

	/**
	 * Set band mismatch flag if we receive Beacon/ProbeResp in 2.4G band,
	 * but the channel num in IE info is 5G, and vice versa
	 * We can get channel num from different IE info, we select
	 * ELEM_ID_DS_PARAM_SET first, and then ELEM_ID_HT_OP
	 * If we don't have any channel info, we set it as HW channel, which is
	 * the channel we get this Beacon/ProbeResp from.
	 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eHwBand == BAND_6G && ucIeDsChannelNum > 0) {
		if (ucIeDsChannelNum > UNII8_UPPER_BOUND)
			fgBandMismatch = TRUE;
	} else
#endif
	if (ucIeDsChannelNum > 0) {
		if (ucIeDsChannelNum <= HW_CHNL_NUM_MAX_2G4)
			fgBandMismatch = (eHwBand != BAND_2G4);
		else if (ucIeDsChannelNum < HW_CHNL_NUM_MAX_4G_5G)
			fgBandMismatch = (eHwBand != BAND_5G);
	} else if (ucIeHtChannelNum > 0) {
		if (ucIeHtChannelNum <= HW_CHNL_NUM_MAX_2G4)
			fgBandMismatch = (eHwBand != BAND_2G4);
		else if (ucIeHtChannelNum < HW_CHNL_NUM_MAX_4G_5G)
			fgBandMismatch = (eHwBand != BAND_5G);
	}

	if (fgBandMismatch) {
		log_dbg(SCN, INFO, MACSTR "Band mismatch, HW band %d, DS chnl %d, HT chnl %d\n",
		       MAC2STR(prWlanBeaconFrame->aucBSSID), eHwBand,
		       ucIeDsChannelNum, ucIeHtChannelNum);
		return NULL;
	}
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	/* In force mode, not update 6G power mode by beacon info */
	if ((eHwBand == BAND_6G) && (prAdapter->fg6GPwrModeForce) != TRUE) {
		e6GPwrModeCurr = rlmDomain6GPwrModeDecision(
					prAdapter,
					fgIsHE6GPresent,
					uc6GHeRegInfo);
		fg6GPwrModeValid = TRUE;

		u4Status = rlmDomain6GPwrModeCountrySupportChk(
				eHwBand,
				ucChnlNum,
				prAdapter->rWifiVar.u2CountryCode,
				e6GPwrModeCurr,
				&fgPwrMode6GSupport);

		if (u4Status == WLAN_STATUS_SUCCESS &&
			fgPwrMode6GSupport == FALSE) {

			DBGLOG(SCN, WARN, "Skip scan, BSSID["MACSTR
				"] SSID:%s non support 6G pwr mode[%d],0x%08x",
				MAC2STR(prWlanBeaconFrame->aucBSSID),
				rSsid.aucSsid,
				e6GPwrModeCurr,
				u4Status);
			return NULL;
		}
	}
#endif

	/* 4 <1.2> Replace existing BSS_DESC structure or allocate a new one */
	prBssDesc = scanSearchExistingBssDescWithSsid(
		prAdapter,
		eBSSType,
		(uint8_t *) prWlanBeaconFrame->aucBSSID,
		(uint8_t *) prWlanBeaconFrame->aucSrcAddr,
		fgIsValidSsid, fgIsValidSsid == TRUE ? &rSsid : NULL);

	log_dbg(SCN, TRACE, "Receive type %u in chnl %u %u %u (" MACSTR
		") valid(%u) found(%u),band=%d\n",
		ucSubtype, ucIeDsChannelNum, ucIeHtChannelNum,
		ucChnlNum,
		MAC2STR((uint8_t *)prWlanBeaconFrame->aucBSSID), fgIsValidSsid,
		(prBssDesc != NULL) ? 1 : 0,
		eHwBand);

	if ((prWlanBeaconFrame->u2FrameCtrl & MASK_FRAME_TYPE)
			== MAC_FRAME_PROBE_RSP)
		fgIsProbeResp = TRUE;

	if (prBssDesc == (struct BSS_DESC *) NULL) {
		fgIsNewBssDesc = TRUE;

		do {
			/* 4 <1.2.1> First trial of allocation */
			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (prBssDesc)
				break;
			/* 4 <1.2.2> Hidden is useless, remove the oldest
			 * hidden ssid. (for passive scan)
			 */
			scanRemoveBssDescsByPolicy(prAdapter,
				(SCN_RM_POLICY_EXCLUDE_CONNECTED
				| SCN_RM_POLICY_OLDEST_HIDDEN
				| SCN_RM_POLICY_TIMEOUT));

			/* 4 <1.2.3> Second tail of allocation */
			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (prBssDesc)
				break;
			/* 4 <1.2.4> Remove the weakest one */
			/* If there are more than half of BSS which has the
			 * same ssid as connection setting, remove the weakest
			 * one from them. Else remove the weakest one.
			 */
			scanRemoveBssDescsByPolicy(prAdapter,
				(SCN_RM_POLICY_EXCLUDE_CONNECTED
				 | SCN_RM_POLICY_SMART_WEAKEST));

			/* 4 <1.2.5> reallocation */
			prBssDesc = scanAllocateBssDesc(prAdapter);
			if (prBssDesc)
				break;
			/* 4 <1.2.6> no space, should not happen */
			log_dbg(SCN, WARN, "alloc new BssDesc for "
				MACSTR " failed\n",
				MAC2STR((uint8_t *)
				prWlanBeaconFrame->aucBSSID));
			return NULL;

		} while (FALSE);

	} else {
		OS_SYSTIME rCurrentTime;

		/* WCXRP00000091 */
		/* if the received strength is much weaker than
		 * the original one, ignore it due to it might be received
		 * on the folding frequency
		 */

		GET_CURRENT_SYSTIME(&rCurrentTime);

		ASSERT(prSwRfb->prRxStatusGroup3);

#if CFG_EXT_SCAN
		prBssDesc->fgIsInBTO = FALSE;
#endif
		if (prBssDesc->eBSSType != eBSSType) {
			prBssDesc->eBSSType = eBSSType;
		}

		/* if Timestamp has been reset, re-generate BSS
		 * DESC 'cause AP should have reset itself
		 */
		if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE
			&& u8Timestamp < prBssDesc->u8TimeStamp.QuadPart
			&& prBssDesc->fgIsConnecting == FALSE) {
			u_int8_t fgIsConnected, fgIsConnecting;
			struct AIS_BLOCKLIST_ITEM *prBlock;
			uint32_t u4PairwiseCipher = 0;
			uint32_t u4GroupCipher = 0;
			uint32_t u4GroupMgmtCipher = 0;
			uint32_t u4AkmSuite = 0;
			uint8_t u4MgmtProtection = 0;
			enum ENUM_PARAM_AUTH_MODE eAuthMode;

			DBGLOG_LIMITED(SCN, INFO, "Reset BssDesc "MACSTR
				"AP Timestamp: %llu BssDesc Timestamp: %llu\n",
				MAC2STR(prBssDesc->aucBSSID),
				u8Timestamp,
				prBssDesc->u8TimeStamp.QuadPart);

			/* set flag for indicating this is a new BSS-DESC */
			fgIsNewBssDesc = TRUE;

			/* backup for APs which reset timestamp unexpectedly */
			fgIsConnected = prBssDesc->fgIsConnected;
			fgIsConnecting = prBssDesc->fgIsConnecting;
			prBlock = prBssDesc->prBlock;
			u4PairwiseCipher
				= prBssDesc->u4RsnSelectedPairwiseCipher;
			u4GroupCipher = prBssDesc->u4RsnSelectedGroupCipher;
			u4GroupMgmtCipher
				= prBssDesc->u4RsnSelectedGroupMgmtCipher;
			u4AkmSuite = prBssDesc->u4RsnSelectedAKMSuite;
			eAuthMode = prBssDesc->eRsnSelectedAuthMode;
			u4MgmtProtection = prBssDesc->u4RsnSelectedPmf;

			/* Connected BSS descriptor still be used by other
			 * functions. Thus, we should re-initialize the BSS_DESC
			 * structure instead of re-allocating the BSS_DESC
			 * structure.
			 */
			scanResetBssDesc(prAdapter, prBssDesc);

			/* restore */
			prBssDesc->fgIsConnected = fgIsConnected;
			prBssDesc->fgIsConnecting = fgIsConnecting;
			prBssDesc->prBlock = prBlock;
			prBssDesc->u4RsnSelectedPairwiseCipher
						= u4PairwiseCipher;
			prBssDesc->u4RsnSelectedGroupCipher = u4GroupCipher;
			prBssDesc->u4RsnSelectedGroupMgmtCipher
						= u4GroupMgmtCipher;
			prBssDesc->u4RsnSelectedAKMSuite = u4AkmSuite;
			prBssDesc->eRsnSelectedAuthMode = eAuthMode;
			prBssDesc->u4RsnSelectedPmf = u4MgmtProtection;
		}
	}

	/* 2018/04/17 Frog: always update IE is not a good choice */
	/* Because of not considering hidden BSS */
	/* Hidden BSS Beacon v.s. hidden BSS probe response */
	if ((prBssDesc->u2RawLength == 0) || (fgIsValidSsid)) {
		prBssDesc->u2RawLength = prSwRfb->u2PacketLen;
		if (prBssDesc->u2RawLength > CFG_RAW_BUFFER_SIZE) {
			prBssDesc->u2RawLength = CFG_RAW_BUFFER_SIZE;
			/* Give an warning msg when content is going to be
			 * truncated.
			 */
			DBGLOG(SCN, WARN,
				"Pkt len(%u) > Max RAW buffer size(%u), truncate it!\n",
				prSwRfb->u2PacketLen, CFG_RAW_BUFFER_SIZE);
		}
		kalMemCopy(prBssDesc->aucRawBuf,
			prWlanBeaconFrame, prBssDesc->u2RawLength);

		iPayloadOffset = sortGetPayloadOffset(prAdapter,
							prBssDesc->aucRawBuf);
		if (iPayloadOffset < 0) {
			DBGLOG(SCN, WARN, "Unknown packet\n");
			return NULL;
		}
		prBssDesc->pucIeBuf = prBssDesc->aucRawBuf + iPayloadOffset;
		prBssDesc->u2IELength = prBssDesc->u2RawLength - iPayloadOffset;
		u2IELength = prBssDesc->u2IELength;
	}

	/* update driver gen if bss is undiscoverd before or really found */
	if (fgIsNewBssDesc || !prSwRfb->fgDriverGen)
		prBssDesc->fgDriverGen = prSwRfb->fgDriverGen;

	/* NOTE: Keep consistency of Scan Record during JOIN process */
	if (fgIsNewBssDesc == FALSE && prBssDesc->fgIsConnecting) {
		log_dbg(SCN, TRACE, "we're connecting this BSS("
			MACSTR ") now, don't update it\n",
			MAC2STR(prBssDesc->aucBSSID));
		return prBssDesc;
	}
	/* 4 <2> Get information from Fixed Fields */
	/* Update the latest BSS type information. */
	prBssDesc->eBSSType = eBSSType;

	COPY_MAC_ADDR(prBssDesc->aucSrcAddr, prWlanBeaconFrame->aucSrcAddr);

	COPY_MAC_ADDR(prBssDesc->aucBSSID, prWlanBeaconFrame->aucBSSID);

	prBssDesc->u8TimeStamp.QuadPart = u8Timestamp;

	WLAN_GET_FIELD_16(&prWlanBeaconFrame->u2BeaconInterval,
		&prBssDesc->u2BeaconInterval);

	prBssDesc->u2CapInfo = u2CapInfo;

	/* 4 <2.1> reset prBssDesc variables in case that AP
	 * has been reconfigured
	 */
#if (CFG_SUPPORT_HE_ER == 1)
	prBssDesc->fgIsERSUDisable = TRUE;
	prBssDesc->ucDCMMaxConRx = 0;
#endif
	prBssDesc->fgIsERPPresent = FALSE;
	prBssDesc->fgIsHTPresent = FALSE;
	prBssDesc->fgIsVHTPresent = FALSE;
#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1)
		prBssDesc->fgIsHEPresent = FALSE;
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prBssDesc->fgIsEHTPresent = FALSE;
	if (fgIsProbeResp)
		kalMemSet(&prBssDesc->rMlInfo, 0, sizeof(prBssDesc->rMlInfo));
#endif
	prBssDesc->eSco = CHNL_EXT_SCN;
	prBssDesc->fgIEWAPI = FALSE;
	prBssDesc->fgIERSN = FALSE;
	prBssDesc->fgIEWPA = FALSE;
	prBssDesc->fgIERSNX = FALSE;

	/*Reset VHT OP IE relative settings */
	prBssDesc->eChannelWidth = CW_20_40MHZ;

	prBssDesc->ucCenterFreqS1 = 0;
	prBssDesc->ucCenterFreqS2 = 0;
	prBssDesc->ucCenterFreqS3 = 0;

	/* Support AP Selection */
	prBssDesc->fgExistBssLoadIE = FALSE;
	prBssDesc->fgMultiAnttenaAndSTBC = FALSE;
	prBssDesc->fgIsMCC = FALSE;
	prBssDesc->u2MaximumMpdu = 0;
	prBssDesc->fgExistTxPwr = FALSE;
	prBssDesc->cTransmitPwr = 0;
	prBssDesc->fgIsDisallowed = FALSE;
	prBssDesc->fgExistEspIE = FALSE;
	prBssDesc->fgExistEspOutIE = FALSE;
	memset(prBssDesc->u4EspInfo, 0, sizeof(prBssDesc->u4EspInfo));
	memset(prBssDesc->ucEspOutInfo, 0, sizeof(prBssDesc->ucEspOutInfo));
	prBssDesc->fgIsRWMValid = FALSE;
	prBssDesc->u2ReducedWanMetrics = 0;

	/* Support 802.11k rrm */
	prBssDesc->u2CurrCountryCode = COUNTRY_CODE_NULL;

	if (fgIsProbeResp == FALSE) {
		/* Probe response doesn't have TIM IE. Thus, we should
		 * reset TIM when handling beacon frame only.
		 */
		prBssDesc->fgTIMPresent = FALSE;
		prBssDesc->ucDTIMPeriod = 0;
	}

	/* The cPowerLimit should be set as invalid value while BSS description
	 * is reallocated
	 */
	if (fgIsNewBssDesc) {
		prBssDesc->cPowerLimit = RLM_INVALID_POWER_LIMIT;
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
		prBssDesc->fgIsTxPwrEnvPresent = FALSE;
		prBssDesc->ucTxPwrEnvPwrLmtNum = 0;
		rlmTxPwrEnvMaxPwrInit(prBssDesc->aicTxPwrEnvMaxTxPwr);
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		/* Set default 6G Pwr mode LPI */
		prBssDesc->e6GPwrMode = PWR_MODE_6G_LPI;
#endif
		DBGLOG(SCN, LOUD,
			"LM: New reallocated BSSDesc [" MACSTR "]\n",
			MAC2STR(prBssDesc->aucBSSID));
	}

	scanSetChannelAndRCPI(prAdapter, prSwRfb, eHwBand,
		ucIeDsChannelNum, ucIeHtChannelNum, prBssDesc);

	/* 4 <3.1> Full IE parsing on SW_RFB_T */
	pucIE = prWlanBeaconFrame->aucInfoElem;
	/* pucDumpIE = pucIE; */

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		/* Error handling for disorder IE that IE length is 0*/
		if (IE_ID(pucIE) != ELEM_ID_SSID && IE_LEN(pucIE) == 0)
			continue;
		switch (IE_ID(pucIE)) {
		case ELEM_ID_SSID:
			if ((!prIeSsid) && /* NOTE(Kevin): for Atheros IOT #1 */
			    (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID)) {
				u_int8_t fgIsHiddenSSID = FALSE;

				ucSSIDChar = '\0';

				prIeSsid = (struct IE_SSID *) pucIE;

				/* D-Link DWL-900AP+ */
				if (IE_LEN(pucIE) == 0)
					fgIsHiddenSSID = TRUE;
				/* Cisco AP1230A -
				 * (IE_LEN(pucIE) == 1)
				 * && (SSID_IE(pucIE)->aucSSID[0] == '\0')
				 */
				/* Linksys WRK54G/WL520g -
				 * (IE_LEN(pucIE) == n)
				 * && (SSID_IE(pucIE)->aucSSID[0~(n-1)] == '\0')
				 */
				else {
					for (i = 0; i < IE_LEN(pucIE); i++) {
						ucSSIDChar
							|= SSID_IE(pucIE)
								->aucSSID[i];
					}

					if (!ucSSIDChar)
						fgIsHiddenSSID = TRUE;
				}

				/* Update SSID to BSS Descriptor only if
				 * SSID is not hidden.
				 */
				if (!fgIsHiddenSSID) {
					COPY_SSID(prBssDesc->aucSSID,
						  prBssDesc->ucSSIDLen,
						  SSID_IE(pucIE)->aucSSID,
						  SSID_IE(pucIE)->ucLength);
					if (rSsid.u4SsidLen <
							ELEM_MAX_LEN_SSID) {
						rSsid.aucSsid[rSsid.u4SsidLen]
							= '\0';
					}
				} else if (fgIsProbeResp) {
#if (CFG_SUPPORT_WIFI_6G == 1)
			/* Don't clear SSID for 6G, 6G AP may send unsolicited
			 * probe response every 20TU. Therefore, for search
			 * hidden 6G AP case, the hidden ssid prob response may
			 * come after the prob response with Target AP SSID,
			 * it will clear BSS Desc's SSID causes connection
			 * failed.
			 */
					if ((eHwBand != BAND_6G) ||
						(eHwBand == BAND_6G
						&& prBssDesc->u2RawLength
						== 0))
#endif
					{
						kalMemZero(prBssDesc->aucSSID,
						sizeof(prBssDesc->aucSSID));
						prBssDesc->ucSSIDLen = 0;
					}
				}
			}
			break;

		case ELEM_ID_SUP_RATES:
			/* NOTE(Kevin): Buffalo WHR-G54S's supported rate set
			 * IE exceed 8.
			 * IE_LEN(pucIE) == 12, "1(B), 2(B), 5.5(B), 6(B), 9(B),
			 * 11(B), 12(B), 18(B), 24(B), 36(B), 48(B), 54(B)"
			 */
			/* TP-LINK will set extra and incorrect ie
			 * with ELEM_ID_SUP_RATES
			 */
			if ((!prIeSupportedRate)
				&& (IE_LEN(pucIE) <= RATE_NUM_SW))
				prIeSupportedRate = SUP_RATES_IOT_IE(pucIE);
			break;

		case ELEM_ID_TIM:
			if (IE_LEN(pucIE) <= ELEM_MAX_LEN_TIM) {
				prBssDesc->fgTIMPresent = TRUE;
				prBssDesc->ucDTIMPeriod
					= TIM_IE(pucIE)->ucDTIMPeriod;
			}
			break;

		case ELEM_ID_IBSS_PARAM_SET:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_IBSS_PARAMETER_SET) {
				prBssDesc->u2ATIMWindow
					= IBSS_PARAM_IE(pucIE)->u2ATIMWindow;
			}
			break;

		case ELEM_ID_ERP_INFO:
			if (IE_LEN(pucIE) == ELEM_MAX_LEN_ERP)
				prBssDesc->fgIsERPPresent = TRUE;
			break;

		case ELEM_ID_EXTENDED_SUP_RATES:
			if (!prIeExtSupportedRate)
				prIeExtSupportedRate = EXT_SUP_RATES_IE(pucIE);
			break;

		case ELEM_ID_RSN:
			if (rsnParseRsnIE(prAdapter, RSN_IE(pucIE),
				&prBssDesc->rRSNInfo)) {
				uint8_t i;

				prBssDesc->fgIERSN = TRUE;
				prBssDesc->u2RsnCap
					= prBssDesc->rRSNInfo.u2RsnCap;

				for (i = 0; i < KAL_AIS_NUM; i++)
					aisCheckPmkidCache(
						prAdapter, prBssDesc, i);
			}
			break;

		case ELEM_ID_RSNX:
			if (rsnParseRsnxIE(prAdapter, RSNX_IE(pucIE),
				&prBssDesc->rRSNXInfo)) {

				prBssDesc->fgIERSNX = TRUE;
				prBssDesc->u2RsnxCap
					= prBssDesc->rRSNXInfo.u2Cap;
			}
			break;

		case ELEM_ID_HT_CAP:
		{
			/* Support AP Selection */
			struct IE_HT_CAP *prHtCap = (struct IE_HT_CAP *)pucIE;
			uint8_t ucSpatial = 0;
			uint8_t i = 0;
			/* end Support AP Selection */

#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eHwBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore HT CAP IE at 6G\n");
				break;
			}
#endif
			if (IE_LEN(pucIE) != (sizeof(struct IE_HT_CAP) - 2)) {
				DBGLOG(SCN, WARN,
					"HT_CAP wrong length(%lu)->(%d)\n",
					(sizeof(struct IE_HT_CAP) - 2),
					IE_LEN(prHtCap));
				break;
			}
			prBssDesc->fgIsHTPresent = TRUE;

			prBssDesc->u2MaximumMpdu = (prHtCap->u2HtCapInfo &
				HT_CAP_INFO_MAX_AMSDU_LEN);

			/* Support AP Selection */
			if (prBssDesc->fgMultiAnttenaAndSTBC)
				break;

			for (; i < 4; i++) {
				if (prHtCap->rSupMcsSet.aucRxMcsBitmask[i] > 0)
					ucSpatial++;
			}

			prBssDesc->fgMultiAnttenaAndSTBC =
				((ucSpatial > 1) &&
				(prHtCap->u2HtCapInfo & HT_CAP_INFO_TX_STBC));
			/* end Support AP Selection */

			break;
		}
		case ELEM_ID_HT_OP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eHwBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore HT OP IE at 6G\n");
				break;
			}
#endif
			if (IE_LEN(pucIE) != (sizeof(struct IE_HT_OP) - 2))
				break;

			if ((((struct IE_HT_OP *) pucIE)->ucInfo1
				& HT_OP_INFO1_SCO) != CHNL_EXT_RES) {
				prBssDesc->eSco = (enum ENUM_CHNL_EXT)
				    (((struct IE_HT_OP *) pucIE)->ucInfo1
				    & HT_OP_INFO1_SCO);
			}

			prBssDesc->ucCenterFreqS3 = (uint8_t)
				HT_GET_OP_INFO2_CH_CENTER_FREQ_SEG2(
				((struct IE_HT_OP *) pucIE)->u2Info2);
			if (prBssDesc->ucCenterFreqS3 != 0)
				DBGLOG(SCN, LOUD, "Find valid HT S2 %d\n",
					prBssDesc->ucCenterFreqS3);

			break;
		case ELEM_ID_VHT_CAP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eHwBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore VHT CAP IE at 6G\n");
				break;
			}
#endif
			scanParseVHTCapIE(pucIE, prBssDesc);
			break;

		case ELEM_ID_VHT_OP:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eHwBand == BAND_6G) {
				DBGLOG(SCN, WARN, "Ignore VHT OP IE at 6G\n");
				break;
			}
#endif
			scanParseVHTOpIE(pucIE, prBssDesc);
			break;

#if CFG_SUPPORT_WAPI
		case ELEM_ID_WAPI:
			if (wapiParseWapiIE(WAPI_IE(pucIE),
				&prBssDesc->rIEWAPI))
				prBssDesc->fgIEWAPI = TRUE;
			break;
#endif

		case ELEM_ID_BSS_LOAD:
		{
			struct IE_BSS_LOAD *prBssLoad =
				(struct IE_BSS_LOAD *)pucIE;

			if (IE_LEN(prBssLoad) !=
				(sizeof(struct IE_BSS_LOAD) - 2)) {
				DBGLOG(SCN, WARN,
					"BSS_LOAD IE_LEN err(%d)->(%d)!\n",
					(sizeof(struct IE_BSS_LOAD) - 2),
					IE_LEN(prBssLoad));
				break;
			}
			prBssDesc->u2StaCnt = prBssLoad->u2StaCnt;
			prBssDesc->ucChnlUtilization =
				prBssLoad->ucChnlUtilizaion;
			prBssDesc->u2AvaliableAC = prBssLoad->u2AvailabeAC;
			prBssDesc->fgExistBssLoadIE = TRUE;

			updateLinkStatsApRec(prAdapter, prBssDesc);

		}
			break;

		case ELEM_ID_MOBILITY_DOMAIN:
		{
			struct IE_MOBILITY_DOMAIN *prMDIE =
				(struct IE_MOBILITY_DOMAIN *)pucIE;

			if (IE_LEN(prMDIE) !=
				(sizeof(struct IE_MOBILITY_DOMAIN) - 2)) {
				DBGLOG(SCN, WARN,
					"MD IE_LEN err(%lu)->(%d)!\n",
					(sizeof(struct IE_MOBILITY_DOMAIN) - 2),
					IE_LEN(prMDIE));
				break;
			}

			prBssDesc->fgIsFtOverDS = !!(prMDIE->ucBitMap & BIT(0));
		}
			break;

		case ELEM_ID_EXTENDED_CAP:
			scanParseExtCapIE(pucIE, prBssDesc);
			break;

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
		case ELEM_ID_FILS_INDICATION:
		{
			uint16_t info;

			if (IE_LEN(pucIE) < 4)
				break;

			WLAN_GET_FIELD_16(pucIE + 2, &info);
			prBssDesc->ucIsFilsSkSupport =
				!!(info & FILS_INFO_SK_SUPPORTED);
		}
			break;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

		case ELEM_ID_VENDOR:	/* ELEM_ID_P2P, ELEM_ID_WMM */
		{
			uint8_t ucOuiType;
			uint16_t u2SubTypeVersion;

			if (rsnParseCheckForWFAInfoElem(prAdapter,
				pucIE, &ucOuiType, &u2SubTypeVersion)) {
				if ((ucOuiType == VENDOR_OUI_TYPE_WPA)
					&& (u2SubTypeVersion
					== VERSION_WPA)
					&& (rsnParseWpaIE(prAdapter,
						WPA_IE(pucIE),
						&prBssDesc
							->rWPAInfo))) {
					prBssDesc->fgIEWPA = TRUE;
				}
			}

			if (prBssDesc->fgIsVHTPresent == FALSE)
				scanCheckEpigramVhtIE(pucIE, prBssDesc);
#if CFG_SUPPORT_PASSPOINT
			/* since OSEN is mutual exclusion with RSN, so
			 * we reuse RSN here
			 */
			if ((pucIE[1] >= 10)
				&& (kalMemCmp(pucIE+2,
					"\x50\x6f\x9a\x12", 4) == 0)
				&& (rsnParseOsenIE(prAdapter,
					(struct IE_WFA_OSEN *)pucIE,
						&prBssDesc->rRSNInfo)))
				prBssDesc->fgIEOsen = TRUE;
#endif
#if CFG_ENABLE_WIFI_DIRECT
			if (prAdapter->fgIsP2PRegistered) {
				if ((p2pFuncParseCheckForP2PInfoElem(
					prAdapter, pucIE, &ucOuiType))
					&& (ucOuiType
					== VENDOR_OUI_TYPE_P2P)) {
					prBssDesc->fgIsP2PPresent
						= TRUE;
				}
			}
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_SUPPORT_MBO
			if (pucIE[1] >= 4 &&
			    kalMemCmp(pucIE + 2, "\x50\x6f\x9a\x16", 4) == 0) {
				struct IE_MBO_OCE *mbo =
					(struct IE_MBO_OCE *)pucIE;
				const uint8_t *disallow = NULL;
				const uint8_t *rwm = NULL;
				const uint8_t *txpwr = NULL;
				uint32_t u4lenParam = mbo->ucLength - 4;

				if (u4lenParam <= u2IELength) {
					disallow = kalFindIeMatchMask(
						MBO_ATTR_ID_ASSOC_DISALLOW,
						mbo->aucSubElements,
						u4lenParam,
						NULL, 0, 0, NULL);

					rwm = kalFindIeMatchMask(
						OCE_ATTR_ID_REDUCED_WAN_METRICS,
						mbo->aucSubElements,
						u4lenParam,
						NULL, 0, 0, NULL);

					txpwr = kalFindIeMatchMask(
						OCE_ATTR_ID_TRANSMIT_POWER,
						mbo->aucSubElements,
						u4lenParam,
						NULL, 0, 0, NULL);
				}

				if (disallow && disallow[1] >= 1) {
					prBssDesc->fgIsDisallowed = TRUE;
					DBGLOG(SCN, INFO,
						MACSTR " disallow reason %d\n",
						MAC2STR(prBssDesc->aucBSSID),
						disallow[2]);
				}
				if (rwm && rwm[1] >= 1) {
					prBssDesc->fgIsRWMValid = TRUE;
					prBssDesc->u2ReducedWanMetrics =
					(rwm[2] &
					OCE_ATTIBUTE_REDUCED_WAN_METRICS_MASK);
				}
				if (txpwr && txpwr[1] >= 1) {
					prBssDesc->fgExistTxPwr = TRUE;
					prBssDesc->cTransmitPwr = txpwr[2];
					DBGLOG(SCN, INFO,
						MACSTR " Transmit power %d\n",
						MAC2STR(prBssDesc->aucBSSID),
						prBssDesc->cTransmitPwr);
				}
			}
#endif

			scanCheckAdaptive11rIE(pucIE, prBssDesc);

			scanParseCheckMTKOuiIE(prAdapter,
				pucIE, prBssDesc, eHwBand,
				prWlanBeaconFrame->u2FrameCtrl &
				MASK_FRAME_TYPE);

			scanParseWMMIE(prAdapter,
				pucIE, prBssDesc);
			break;
		}
#if (CFG_SUPPORT_802_11AX == 1)
		case ELEM_ID_RESERVED:
#if (CFG_SUPPORT_802_11BE == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_CAPS)
				scanParseEhtCapIE(pucIE, prBssDesc);

			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_EHT_OP)
				scanParseEhtOpIE(prAdapter, pucIE, prBssDesc,
						 eHwBand);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_MLD)
				scanParseMldIE(prAdapter, prBssDesc,
					(const uint8_t *)pucIE,
					prWlanBeaconFrame->u2FrameCtrl &
					MASK_FRAME_TYPE);
#endif
#endif
			if (fgEfuseCtrlAxOn == 1) {
				if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP) {
					prHeCap = (struct _IE_HE_CAP_T *) pucIE;
					if (IE_SIZE(prHeCap)
					    < (sizeof(struct _IE_HE_CAP_T))) {
						DBGLOG(SCN, WARN,
						    "HE_CAP IE_LEN err(%d)!\n",
						    IE_LEN(prHeCap));
						break;
					}
					prBssDesc->fgIsHEPresent = TRUE;
					memcpy(prBssDesc->ucHePhyCapInfo,
						prHeCap->ucHePhyCap,
						HE_PHY_CAP_BYTE_NUM);
				}
#if (CFG_SUPPORT_HE_ER == 1)
				if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_CAP) {
					prBssDesc->ucDCMMaxConRx =
					HE_GET_PHY_CAP_DCM_MAX_CONSTELLATION_RX(
						prHeCap->ucHePhyCap);
					DBGLOG(SCN, LOUD,
						"ER: BSSID:" MACSTR
						" SSID:%s,rx:%x, er:%x\n",
						MAC2STR(prBssDesc->aucBSSID),
						prBssDesc->aucSSID,
						prBssDesc->ucDCMMaxConRx,
						prBssDesc->fgIsERSUDisable);
				}
				if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP) {
					prHeOp = (struct _IE_HE_OP_T *) pucIE;
					if (IE_SIZE(prHeOp)
					    < (sizeof(struct _IE_HE_OP_T))) {
						DBGLOG(SCN, WARN,
						    "HE_OP IE_LEN err(%d)!\n",
						    IE_LEN(prHeOp));
						break;
					}
					prBssDesc->fgIsERSUDisable =
					HE_IS_ER_SU_DISABLE(
						prHeOp->ucHeOpParams);

					DBGLOG(SCN, LOUD,
						"ER: BSSID:" MACSTR
						" SSID:%s,rx:%x, er:%x\n",
						MAC2STR(prBssDesc->aucBSSID),
						prBssDesc->aucSSID,
						prBssDesc->ucDCMMaxConRx,
						prBssDesc->fgIsERSUDisable);
				}
#endif /* CFG_SUPPORT_HE_ER == 1 */

#if (CFG_SUPPORT_WIFI_6G == 1)
				if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_OP)
					scanParseHEOpIE(prAdapter, pucIE,
						prBssDesc, eHwBand);
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
			}

#if CFG_SUPPORT_MBO
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_ESP) {
				uint8_t infoNum = 0;
				uint8_t *infoList = pucIE + 3;

				DBGLOG(SCN, INFO, "ESP Inbound IE\n");
				dumpMemory8(pucIE, IE_LEN(pucIE));

				prBssDesc->fgExistEspIE = TRUE;
				infoNum = (IE_LEN(pucIE) - 1) / 3;
				for (i = 0; i < infoNum; i++) {
					uint8_t *info = infoList + i * 3;
					uint8_t ac = (*info) & 0x3;

					if (ac < ESP_AC_NUM)
						WLAN_GET_FIELD_24(info,
						    &prBssDesc->u4EspInfo[ac]);
				}
			}

			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_ESP_OUTBOUND) {
				uint8_t infoNum = 0;
				uint8_t present;
				uint8_t *infoList = pucIE + 4;

				DBGLOG(SCN, INFO, "ESP Outbound IE\n");
				dumpMemory8(pucIE, IE_LEN(pucIE));

				prBssDesc->fgExistEspOutIE = TRUE;
				present = pucIE[3];
				for (i = 0; i < ESP_AC_NUM; i++) {
					if (present & BIT(i) &&
					    infoNum + 2 < IE_LEN(pucIE))
						prBssDesc->ucEspOutInfo[i] =
							infoList[infoNum++];
				}
			}
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (IE_ID_EXT(pucIE) == ELEM_EXT_ID_HE_6G_BAND_CAP) {
				uint16_t u2CapInfo =
					((struct _IE_HE_6G_BAND_CAP_T *)pucIE)->
					u2CapInfo;

				prBssDesc->u2MaximumMpdu = (u2CapInfo &
					HE_6G_CAP_INFO_MAX_MPDU_LEN_MASK);
			}
#endif
			break;
#endif /* CFG_SUPPORT_802_11AX == 1 */
		case ELEM_ID_PWR_CONSTRAINT:
		{
			struct IE_POWER_CONSTRAINT *prPwrConstraint =
				(struct IE_POWER_CONSTRAINT *)pucIE;

			if (IE_LEN(pucIE) != 1)
				break;
			ucPowerConstraint =
				prPwrConstraint->ucLocalPowerConstraint;
			break;
		}
		case ELEM_ID_RRM_ENABLED_CAP:
			if (IE_LEN(pucIE) == 5) {
				/* RRM Capability IE is always 5 bytes */
				kalMemZero(prBssDesc->aucRrmCap,
					   sizeof(prBssDesc->aucRrmCap));
				kalMemCopy(prBssDesc->aucRrmCap, pucIE + 2,
					   sizeof(prBssDesc->aucRrmCap));
			}
			break;
#if (CFG_SUPPORT_802_11V_MBSSID == 1)
		case ELEM_ID_MBSSID:
			if (MBSSID_IE(pucIE)->ucMaxBSSIDIndicator <= 0 ||
				MBSSID_IE(pucIE)->ucMaxBSSIDIndicator > 8) {
				/* invalid Multiple BSSID ie
				* (must in range 1~8)
				*/
				break;
			}

			scanParsingMBSSIDSubelement(prAdapter,
						prBssDesc, MBSSID_IE(pucIE));
			break;
#endif

#if (CFG_SUPPORT_WIFI_RNR == 1)
		case ELEM_ID_RNR:
			scanParsingRnrElement(prAdapter, prBssDesc, pucIE);
			break;
#endif

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
		case ELEM_ID_TX_PWR_ENVELOPE:
			prTxPwrEnvIE = (struct IE_TX_PWR_ENV_FRAME *) pucIE;
			break;
#endif
			/* no default */
		}
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	/* reset mldtype if no ml ie */
	if (!prBssDesc->rMlInfo.fgValid)
		prBssDesc->rMlInfo.fgMldType = MLD_TYPE_INVALID;

	if (prBssDesc->rMlInfo.fgValid &&
		prBssDesc->rMlInfo.fgMldType == MLD_TYPE_INVALID)
		prBssDesc->rMlInfo.fgMldType = MLD_TYPE_EXTERNAL;
#endif

	/* 4 <3.2> Save information from IEs - SSID */
	/* Update Flag of Hidden SSID for used in SEARCH STATE. */

	/* NOTE(Kevin): in current driver, the ucSSIDLen == 0 represent
	 * all cases of hidden SSID.
	 * If the fgIsHiddenSSID == TRUE, it means we didn't get the
	 * ProbeResp with valid SSID.
	 */
	if (prBssDesc->ucSSIDLen == 0)
		prBssDesc->fgIsHiddenSSID = TRUE;
	else
		prBssDesc->fgIsHiddenSSID = FALSE;

	/* 4 <3.3> Check rate information in related IEs. */
	if (prIeSupportedRate || prIeExtSupportedRate) {
		rateGetRateSetFromIEs(prIeSupportedRate,
				      prIeExtSupportedRate,
				      &prBssDesc->u2OperationalRateSet,
				      &prBssDesc->u2BSSBasicRateSet,
				      &prBssDesc->fgIsUnknownBssBasicRate);
	}

	/* 4 <5> Check IE information corret or not */
	if (!rlmDomainIsValidRfSetting(prAdapter, prBssDesc->eBand,
				       prBssDesc->ucChannelNum, prBssDesc->eSco,
				       prBssDesc->eChannelWidth,
				       prBssDesc->ucCenterFreqS1,
				       prBssDesc->ucCenterFreqS2)) {
#if 0 /* TODO: Remove this */
		/* Dump IE Inforamtion */
		log_dbg(RLM, WARN, "ScanAddToBssDesc IE Information\n");
		log_dbg(RLM, WARN, "IE Length = %d\n", u2IELength);
		log_mem8_dbg(RLM, WARN, pucDumpIE, u2IELength);
#endif

		/* Error Handling for Non-predicted IE - Fixed to set 20MHz */
		prBssDesc->eChannelWidth = CW_20_40MHZ;
		prBssDesc->ucCenterFreqS1 = 0;
		prBssDesc->ucCenterFreqS2 = 0;
		prBssDesc->eSco = CHNL_EXT_SCN;
	}

#if CFG_SUPPORT_802_11K
	if (prCountryIE) {
		prBssDesc->u2CurrCountryCode = u2CurrCountryCode;
		DBGLOG(SCN, LOUD,
			"Country IE present,BSSID[" MACSTR "] SSID:%s(%c%c)\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->aucSSID,
			((u2CurrCountryCode & 0xff00) >> 8),
			(u2CurrCountryCode & 0x00ff));
		DBGLOG_MEM8(SCN, LOUD, prCountryIE, IE_SIZE(prCountryIE));
		/* Update TxPower limit for Country IE & Power Constraint IE */
		rlmRegTxPwrLimitUpdate(prAdapter,
			prBssDesc,
			eHwBand,
			prCountryIE,
			ucPowerConstraint);
	}
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	if (eHwBand == BAND_6G && fg6GPwrModeValid == TRUE) {
		prBssDesc->e6GPwrMode = e6GPwrModeCurr;
		if (prBssDesc->fgIsConnected) {
			for (ucBssIdx = 0; ucBssIdx < prAdapter->ucHwBssIdNum;
			     ucBssIdx++) {
				prBssInfo = prAdapter->aprBssInfo[ucBssIdx];
				if (EQUAL_MAC_ADDR(prBssInfo->aucBSSID,
							prBssDesc->aucBSSID)) {
					rlmDomain6GPwrModeUpdate(prAdapter,
						ucBssIdx,
						prBssDesc->e6GPwrMode);
					break;
				}
			}
		}
	}
#endif
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
	if (prTxPwrEnvIE) {
		DBGLOG(SCN, TRACE,
			"TPE present,BSSID[" MACSTR "] SSID:%s\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->aucSSID);

		DBGLOG_MEM8(SCN, TRACE, prTxPwrEnvIE, IE_SIZE(prTxPwrEnvIE));

		rlmTxPwrEnvMaxPwrUpdate(
			prAdapter,
			prBssDesc,
			eHwBand,
			prTxPwrEnvIE);
	}
#endif

	/* 4 <6> PHY type setting */
	prBssDesc->ucPhyTypeSet = 0;

	/* check if support 11n */
	if (prBssDesc->fgIsVHTPresent)
		prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_VHT;

	if (prBssDesc->fgIsHTPresent)
		prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HT;

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		if (prBssDesc->fgIsHEPresent)
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HE;
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prBssDesc->fgIsHE6GPresent)
		prBssDesc->ucPhyTypeSet |=
			(PHY_TYPE_BIT_HE | PHY_TYPE_BIT_OFDM);
#endif
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (prBssDesc->fgIsEHTPresent)
		prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_EHT;
#endif

#if WLAN_INCLUDE_SYS
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eHwBand == BAND_6G &&
		prAdapter->fg6eOffSpecNotShow) {
		u_int8_t fgIsOffSpec6G = TRUE;

		if (!prBssDesc->fgIERSN)
			fgIsOffSpec6G = TRUE;
		else {
			/* SAE H2E AP */
			if (prBssDesc->fgIERSNX &&
				prBssDesc->rRSNXInfo.u2Cap &
				BIT(WLAN_RSNX_CAPAB_SAE_H2E))
				fgIsOffSpec6G = FALSE;

			if (prBssDesc->rRSNInfo.au4AuthKeyMgtSuite[0]
				== RSN_AKM_SUITE_OWE)
				fgIsOffSpec6G = FALSE;
		}

		if (fgIsOffSpec6G) {
			DBGLOG(SCN, INFO,
				"filter out-of-spec 6G AP: SSID %s" MACSTR "\n",
				prBssDesc->aucSSID,
				MAC2STR(prBssDesc->aucBSSID));
			return NULL;
		}
	}
#endif
#endif

	/* if not 11n only */
	if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_BIT_HT_PHY)) {
		if (prBssDesc->eBand == BAND_2G4) {
			/* check if support 11g */
			if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM)
			    || prBssDesc->fgIsERPPresent)
				prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_ERP;

			/* if not 11g only */
			if (!(prBssDesc->u2BSSBasicRateSet & RATE_SET_OFDM)) {
				/* check if support 11b */
				if (prBssDesc->u2OperationalRateSet
				    & RATE_SET_HR_DSSS)
					prBssDesc->ucPhyTypeSet
						|= PHY_TYPE_BIT_HR_DSSS;
			}
		} else {
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_OFDM;
		}
	} else {
		if (prBssDesc->eBand == BAND_2G4) {
			if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM)
			    || prBssDesc->fgIsERPPresent)
				prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_ERP;
		} else {
			prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_OFDM;
		}

		prBssDesc->ucPhyTypeSet |= PHY_TYPE_BIT_HT;
	}

	/* Support AP Selection */
	/* update update-index and reset seen-probe-response */
	if (prBssDesc->u4UpdateIdx !=
		prAdapter->rWifiVar.rScanInfo.u4ScanUpdateIdx) {
		prBssDesc->fgSeenProbeResp = FALSE;
		prBssDesc->u4UpdateIdx =
			prAdapter->rWifiVar.rScanInfo.u4ScanUpdateIdx;
	}

	/* check if it is a probe response frame */
	if (fgIsProbeResp)
		prBssDesc->fgSeenProbeResp = TRUE;
	/* end Support AP Selection */
	/* 4 <7> Update BSS_DESC_T's Last Update TimeStamp. */
	if (fgIsProbeResp || fgIsValidSsid)
		GET_CURRENT_SYSTIME(&prBssDesc->rUpdateTime);

#if CFG_SUPPORT_802_11K
	if (prBssDesc->fgIsConnected)
		rrmUpdateBssTimeTsf(prAdapter, prBssDesc);
#endif
	return prBssDesc;
}

/* clear all ESS scan result */
void scanInitEssResult(struct ADAPTER *prAdapter)
{
	struct WLAN_INFO *prWlanInfo;

	prWlanInfo = &prAdapter->rWlanInfo;

	prWlanInfo->u4ScanResultEssNum = 0;
	prWlanInfo->u4ScanDbgTimes1 = 0;
	prWlanInfo->u4ScanDbgTimes2 = 0;
	prWlanInfo->u4ScanDbgTimes3 = 0;
	prWlanInfo->u4ScanDbgTimes4 = 0;
	kalMemZero(prWlanInfo->arScanResultEss,
		sizeof(prWlanInfo->arScanResultEss));
}

/* print all ESS into log system once scan done
 * it is useful to log that, otherwise, we have no information to
 * identify if hardware has seen a specific AP,
 * if user complained some AP were not found in scan result list
 */
void scanLogEssResult(struct ADAPTER *prAdapter)
{
	struct WLAN_INFO *prWlanInfo = &prAdapter->rWlanInfo;
	struct ESS_SCAN_RESULT_T *prEssResult =
		prWlanInfo->arScanResultEss;
	uint32_t u4ResultNum = prWlanInfo->u4ScanResultEssNum;
	uint32_t u4Index = 0;
	char *strbuf = NULL, *pos = NULL, *end = NULL;
	int slen = 0;
	u_int8_t first = TRUE;

	if (u4ResultNum == 0) {
		scanlog_dbg(LOG_SCAN_DONE_D2K, INFO, "0 Bss is found, %d, %d, %d, %d\n",
			prWlanInfo->u4ScanDbgTimes1,
			prWlanInfo->u4ScanDbgTimes2,
			prWlanInfo->u4ScanDbgTimes3,
			prWlanInfo->u4ScanDbgTimes4);
		return;
	}

	for (u4Index = 0; u4Index < u4ResultNum; u4Index++) {
		if (prEssResult[u4Index].u2SSIDLen > 0)
			slen += prEssResult[u4Index].u2SSIDLen + 2; /* _ssid;*/
	}

	slen = KAL_MIN(slen + 1, SCAN_LOG_MSG_MAX_LEN); /* 1 for null end*/
	pos = strbuf = kalMemAlloc(slen, VIR_MEM_TYPE);
	if (strbuf == NULL) {
		scanlog_dbg(LOG_SCAN_DONE_D2K, INFO, "Can't allocate memory\n");
		return;
	}
	end = strbuf + slen;
	for (u4Index = 0; u4Index < u4ResultNum; u4Index++) {
		uint8_t len = prEssResult[u4Index].u2SSIDLen;
		char ssid[PARAM_MAX_LEN_SSID + 1] = {0};

		if (len == 0)
			continue;
		if (pos + len + 3 > end) { /* _ssid;nul */
			pos = strbuf;
			if (first) {
				scanlog_dbg(LOG_SCAN_DONE_D2K, INFO,
					"Total:%u/%u %s", u4ResultNum,
					prWlanInfo->u4ScanResultNum,
					strbuf);
				first = FALSE;
			} else {
				scanlog_dbg(LOG_SCAN_DONE_D2K, INFO,
					"%s", strbuf);
			}
		}
		kalStrnCpy(ssid, prEssResult[u4Index].aucSSID, sizeof(ssid));
		ssid[sizeof(ssid) - 1] = '\0';
		pos += kalSnprintf(pos, end - pos, " %s;", ssid);
	}
	if (pos != strbuf) {
		if (first)
			scanlog_dbg(LOG_SCAN_DONE_D2K, INFO,
				"Total:%u/%u %s", u4ResultNum,
				prWlanInfo->u4ScanResultNum, strbuf);
		else
			scanlog_dbg(LOG_SCAN_DONE_D2K, INFO, "%s", strbuf);
	}
	kalMemFree(strbuf, VIR_MEM_TYPE, slen);
}

/* record all Scanned ESS, only one BSS was saved for each ESS, and AP who
 * is hidden ssid was excluded.
 */
/* maximum we only support record 64 ESSes */
static void scanAddEssResult(struct ADAPTER *prAdapter,
			     struct BSS_DESC *prBssDesc)
{
	struct WLAN_INFO *prWlanInfo = &prAdapter->rWlanInfo;
	struct ESS_SCAN_RESULT_T *prEssResult =
		prWlanInfo->arScanResultEss;
	uint32_t u4Index = 0;

	if (prBssDesc->fgIsHiddenSSID)
		return;
	if (prWlanInfo->u4ScanResultEssNum >= CFG_MAX_NUM_BSS_LIST)
		return;
	for (; u4Index < prWlanInfo->u4ScanResultEssNum; u4Index++) {
		if (EQUAL_SSID(prEssResult[u4Index].aucSSID,
			(uint8_t)prEssResult[u4Index].u2SSIDLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen))
			return;
	}

	COPY_SSID(prEssResult[u4Index].aucSSID, prEssResult[u4Index].u2SSIDLen,
		prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	COPY_MAC_ADDR(prEssResult[u4Index].aucBSSID, prBssDesc->aucBSSID);
	prWlanInfo->u4ScanResultEssNum++;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Convert the Beacon or ProbeResp Frame in SW_RFB_T to scan
 * result for query
 *
 * @param[in] prSwRfb            Pointer to the receiving SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   It is a valid Scan Result and been sent
 *                               to the host.
 * @retval WLAN_STATUS_FAILURE   It is not a valid Scan Result.
 */
/*----------------------------------------------------------------------------*/
uint32_t scanAddScanResult(struct ADAPTER *prAdapter,
			   struct BSS_DESC *prBssDesc,
			   struct SW_RFB *prSwRfb)
{
	struct SCAN_INFO *prScanInfo;
	uint8_t aucRatesEx[PARAM_MAX_LEN_RATES_EX];
	struct WLAN_BEACON_FRAME *prWlanBeaconFrame;
	uint8_t rMacAddr[PARAM_MAC_ADDR_LEN];
	struct PARAM_SSID rSsid;
	enum ENUM_PARAM_NETWORK_TYPE eNetworkType = PARAM_NETWORK_TYPE_NUM;
	struct PARAM_802_11_CONFIG rConfiguration;
	enum ENUM_PARAM_OP_MODE eOpMode;
	uint8_t ucRateLen = 0;
	uint32_t i;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	kalMemZero(&rConfiguration, sizeof(rConfiguration));
	kalMemZero(&rSsid, sizeof(rSsid));

	if (prBssDesc->eBand == BAND_2G4) {
		if ((prBssDesc->u2OperationalRateSet & RATE_SET_OFDM)
		    || prBssDesc->fgIsERPPresent) {
			eNetworkType = PARAM_NETWORK_TYPE_OFDM24;
		} else {
			eNetworkType = PARAM_NETWORK_TYPE_DS;
		}
	} else if (prBssDesc->eBand == BAND_5G) {
		/*ASSERT(prBssDesc->eBand == BAND_5G);*/
		eNetworkType = PARAM_NETWORK_TYPE_OFDM5;
	}

	if (prBssDesc->eBSSType == BSS_TYPE_P2P_DEVICE) {
		/* NOTE(Kevin): Not supported by WZC(TBD) */
		log_dbg(SCN, INFO, "Bss Desc type is P2P\n");
		return WLAN_STATUS_FAILURE;
	}

	prWlanBeaconFrame = (struct WLAN_BEACON_FRAME *) prSwRfb->pvHeader;
	COPY_MAC_ADDR(rMacAddr, prWlanBeaconFrame->aucBSSID);
	COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
		prBssDesc->aucSSID, prBssDesc->ucSSIDLen);

	rConfiguration.u4Length = sizeof(struct PARAM_802_11_CONFIG);
	rConfiguration.u4BeaconPeriod
		= (uint32_t) prWlanBeaconFrame->u2BeaconInterval;
	rConfiguration.u4ATIMWindow = prBssDesc->u2ATIMWindow;
	rConfiguration.u4DSConfig = nicChannelNum2Freq(
		prBssDesc->ucChannelNum, prBssDesc->eBand);
	rConfiguration.rFHConfig.u4Length
		= sizeof(struct PARAM_802_11_CONFIG_FH);

	rateGetDataRatesFromRateSet(prBssDesc->u2OperationalRateSet, 0,
		aucRatesEx, &ucRateLen);

	/* NOTE(Kevin): Set unused entries, if any, at the end of the
	 * array to 0 from OID_802_11_BSSID_LIST
	 */
	for (i = ucRateLen; i < ARRAY_SIZE(aucRatesEx); i++)
		aucRatesEx[i] = 0;

	switch (prBssDesc->eBSSType) {
	case BSS_TYPE_IBSS:
		eOpMode = NET_TYPE_IBSS;
		break;

	case BSS_TYPE_INFRASTRUCTURE:
	case BSS_TYPE_P2P_DEVICE:
	case BSS_TYPE_BOW_DEVICE:
	default:
		eOpMode = NET_TYPE_INFRA;
		break;
	}

	log_dbg(SCN, LOUD, "ind %s %d %d\n", prBssDesc->aucSSID,
		prBssDesc->ucChannelNum, prBssDesc->ucRCPI);

	scanAddEssResult(prAdapter, prBssDesc);
	if (prAdapter->rWifiVar.rScanInfo.fgSchedScanning &&
		KAL_TEST_BIT(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
			prAdapter->ulSuspendFlag)) {
		uint8_t i = 0;
		struct BSS_DESC **pprPendBssDesc
			= &prScanInfo->rSchedScanParam.
				aprPendingBssDescToInd[0];

		for (; i < SCN_SSID_MATCH_MAX_NUM; i++) {
			if (pprPendBssDesc[i])
				continue;
			log_dbg(SCN, INFO, "indicate bss["
				MACSTR
				"] before wiphy resume, need to indicate again after wiphy resume\n",
				MAC2STR(prBssDesc->aucBSSID));
			pprPendBssDesc[i] = prBssDesc;
			break;
		}
	}

	if (prBssDesc->u2RawLength != 0) {
		kalIndicateBssInfo(prAdapter->prGlueInfo,
			   prBssDesc->aucRawBuf,
			   prBssDesc->u2RawLength,
			   prBssDesc->ucChannelNum,
			   prBssDesc->eBand,
			   RCPI_TO_dBm(prBssDesc->ucRCPI));
	}

	nicAddScanResult(prAdapter,
		rMacAddr,
		&rSsid,
		prWlanBeaconFrame->u2CapInfo,
		RCPI_TO_dBm(prBssDesc->ucRCPI),
		eNetworkType,
		&rConfiguration,
		eOpMode,
		aucRatesEx,
		prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen,
		(uint8_t *) ((uintptr_t) (prSwRfb->pvHeader)
			+ WLAN_MAC_MGMT_HEADER_LEN));

	return WLAN_STATUS_SUCCESS;

}	/* end of scanAddScanResult() */

u_int8_t scanCheckBssIsLegal(struct ADAPTER *prAdapter,
			     struct BSS_DESC *prBssDesc)
{
	u_int8_t fgAddToScanResult = FALSE;
	enum ENUM_BAND eBand;
	uint8_t ucChannel;

	ASSERT(prAdapter);
	/* check the channel is in the legal doamin */
	if (rlmDomainIsLegalChannel(prAdapter, prBssDesc->eBand,
		prBssDesc->ucChannelNum) == TRUE) {
		/* check ucChannelNum/eBand for adjacement channel filtering */
		if (cnmAisInfraChannelFixed(prAdapter,
			&eBand, &ucChannel) == TRUE &&
			(eBand != prBssDesc->eBand
			|| ucChannel != prBssDesc->ucChannelNum)) {
			fgAddToScanResult = FALSE;
		} else {
			fgAddToScanResult = TRUE;
		}
	}

	return fgAddToScanResult;

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Parse the content of given Beacon or ProbeResp Frame.
 *
 * @param[in] prSwRfb            Pointer to the receiving SW_RFB_T structure.
 *
 * @retval WLAN_STATUS_SUCCESS   if not report this SW_RFB_T to host
 * @retval WLAN_STATUS_PENDING   if report this SW_RFB_T to host as scan result
 */
/*----------------------------------------------------------------------------*/
uint32_t scanProcessBeaconAndProbeResp(struct ADAPTER *prAdapter,
				       struct SW_RFB *prSwRfb)
{
	struct SCAN_INFO *prScanInfo;
	struct BSS_DESC *prBssDesc = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t *pau4ChBitMap;
	struct WLAN_BEACON_FRAME *prWlanBeaconFrame = NULL;
#if CFG_SLT_SUPPORT
	struct SLT_INFO *prSltInfo = NULL;
#endif
	uint32_t u4Idx = 0;
	struct WLAN_INFO *prWlanInfo;
	struct BSS_INFO *prBssInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	/* 4 <0> Ignore invalid Beacon and Probe Response*/
	if (prSwRfb->u2PacketLen < prSwRfb->u2HeaderLen ||
		(prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) <
		(TIMESTAMP_FIELD_LEN + BEACON_INTERVAL_FIELD_LEN
		+ CAP_INFO_FIELD_LEN) ||
		prSwRfb->u2HeaderLen != sizeof(struct WLAN_MAC_HEADER)) {
		log_dbg(SCN, ERROR,
			"Ignore invalid Beacon or Probe Response\n");
		return rStatus;
	}

	if (prSwRfb->prStaRec)
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prSwRfb->prStaRec->ucBssIndex);
	if (prBssInfo &&
	    ((IS_BSS_P2P(prBssInfo) && prBssInfo->fgIsSwitchingChnl) ||
	     IS_AIS_CH_SWITCH(prBssInfo))) {
		log_dbg(SCN, TRACE,
			"drop GO/AP beaon during CSA [" MACSTR
			"],BssIdx = %d\n",
			MAC2STR(prBssInfo->prStaRecOfAP->aucMacAddr),
			prBssInfo->ucBssIndex);
		return rStatus;
	}

	scanResultLog(prAdapter, prSwRfb);

#if CFG_SLT_SUPPORT
	prSltInfo = &prAdapter->rWifiVar.rSltInfo;

	if (prSltInfo->fgIsDUT) {
		log_dbg(P2P, INFO, "\n\rBCN: RX\n");
		prSltInfo->u4BeaconReceiveCnt++;
		return WLAN_STATUS_SUCCESS;
	} else {
		return WLAN_STATUS_SUCCESS;
	}
#endif

	prWlanBeaconFrame = (struct WLAN_BEACON_FRAME *) prSwRfb->pvHeader;
	/* Ignore MC probe resp which is unexpected.
	 * MC probe resp with wrong content will result in
	 * MLO disconnect.
	 */
	if (prSwRfb->fgIsMC)
		return WLAN_STATUS_SUCCESS;

	prWlanInfo = &prAdapter->rWlanInfo;

	/* 4 <1> Parse and add into BSS_DESC_T */
	prBssDesc = scanAddToBssDesc(prAdapter, prSwRfb);
	prWlanInfo->u4ScanDbgTimes1++;

	if (prBssDesc) {
		/* Full2Partial: save channel info for later scan */
		if (prScanInfo->fgIsScanForFull2Partial) {
			log_dbg(SCN, TRACE, "Full2Partial: set channel=%d\n",
					prBssDesc->ucChannelNum);
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (prBssDesc->eBand == BAND_6G)
				pau4ChBitMap = &prScanInfo->au4ChannelBitMap[8];
			else
#endif
				pau4ChBitMap = prScanInfo->au4ChannelBitMap;

			scanSetBit(prBssDesc->ucChannelNum,
				pau4ChBitMap,
				sizeof(prScanInfo->au4ChannelBitMap));
		}

	for (u4Idx = 0; u4Idx < prAdapter->ucSwBssIdNum; u4Idx++) {
		struct BSS_INFO *prAisBssInfo = prAdapter->aprBssInfo[u4Idx];
		struct CONNECTION_SETTINGS *prConnSettings;

		if (!IS_BSS_AIS(prAisBssInfo))
			continue;

		prConnSettings =
			aisGetConnSettings(prAdapter, prAisBssInfo->ucBssIndex);

		/* 4 <1.1> Beacon Change Detection for Connected BSS */
		if ((prAisBssInfo->eConnectionState ==
		     MEDIA_STATE_CONNECTED) &&
		    ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE
		    && prConnSettings->eOPMode != NET_TYPE_IBSS)
		    || (prBssDesc->eBSSType == BSS_TYPE_IBSS
		    && prConnSettings->eOPMode != NET_TYPE_INFRA))
		    && EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
		    prAisBssInfo->aucBSSID)
		    && EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
		    prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen)) {

#if CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE
			if (rsnCheckSecurityModeChanged(prAdapter,
				prAisBssInfo, prBssDesc)
#if CFG_SUPPORT_WAPI
				|| (aisGetWapiMode(prAdapter,
				ucBssIndex) &&
				!wapiPerformPolicySelection(prAdapter,
					prBssDesc, ucBssIndex))
#endif
				) {

				log_dbg(SCN, INFO, "Beacon security mode change detected\n");
				log_mem8_dbg(SCN, INFO,
					prSwRfb->pvHeader,
					prSwRfb->u2PacketLen);
				if (!prConnSettings
					->fgSecModeChangeStartTimer) {
					cnmTimerStartTimer(prAdapter,
						aisGetSecModeChangeTimer(
						prAdapter,
						ucBssIndex),
						SEC_TO_MSEC(3));
					prConnSettings
						->fgSecModeChangeStartTimer
							= TRUE;
				}
			} else {
				if (prConnSettings->fgSecModeChangeStartTimer) {
					cnmTimerStopTimer(prAdapter,
						aisGetSecModeChangeTimer(
						prAdapter,
						ucBssIndex));
					prConnSettings
						->fgSecModeChangeStartTimer
							= FALSE;
				}
			}
#endif
		}
		/* 4 <1.1> Update AIS_BSS_INFO */
#if (CFG_SUPPORT_WIFI_6G == 1)
		prAisBssInfo->He6gRegInfo = prBssDesc->He6gRegInfo;
#endif
		if ((prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE &&
		      prConnSettings->eOPMode != NET_TYPE_IBSS)
		     || (prBssDesc->eBSSType == BSS_TYPE_IBSS
		     && prConnSettings->eOPMode != NET_TYPE_INFRA)) {
			if (prAisBssInfo->eConnectionState
				== MEDIA_STATE_CONNECTED) {

				/* *not* checking prBssDesc->fgIsConnected
				 * anymore, due to Linksys AP uses " " as
				 * hidden SSID, and would have different
				 * BSS descriptor
				 */
				log_dbg(SCN, TRACE, "BSS%d DTIMPeriod[%u] Present[%u] BSSID["
					MACSTR "] BeaconInterval[%u]\n",
				       prAisBssInfo->ucBssIndex,
				       prAisBssInfo->ucDTIMPeriod,
				       prAisBssInfo->fgTIMPresent,
				       MAC2STR(prBssDesc->aucBSSID),
					   prAisBssInfo->u2BeaconInterval);
				if ((!prAisBssInfo->ucDTIMPeriod) &&
					prAisBssInfo->fgTIMPresent &&
					EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
						prAisBssInfo->aucBSSID) &&
					(prAisBssInfo->eCurrentOPMode
					== OP_MODE_INFRASTRUCTURE) &&
					((prWlanBeaconFrame->u2FrameCtrl
					& MASK_FRAME_TYPE)
					== MAC_FRAME_BEACON)) {
					prAisBssInfo->ucDTIMPeriod
						= prBssDesc->ucDTIMPeriod;
					prAisBssInfo->fgTIMPresent
						= prBssDesc->fgTIMPresent;
#if CFG_SUPPORT_BALANCE_MLR
					prAisBssInfo->u2BeaconInterval
						= prBssDesc->u2BeaconInterval;

					log_dbg(SCN, WARN,
						"Update Beacon interval [%u]\n",
						prAisBssInfo->u2BeaconInterval);
#endif /* CFG_SUPPORT_BALANCE_MLR */

					/* Handle No TIM IE information case */
					if (!prAisBssInfo->fgTIMPresent) {
						enum PARAM_POWER_MODE ePwrMode
							= Param_PowerModeCAM;

						log_dbg(SCN, WARN, "IE TIM absence, set to CAM mode!\n");
						nicConfigPowerSaveProfile(
							prAdapter,
							prAisBssInfo->
							ucBssIndex, ePwrMode,
							FALSE,
							PS_CALLER_NO_TIM);
					}
					/* sync with firmware for
					 * beacon information
					 */
					nicPmIndicateBssConnected(prAdapter,
						prAisBssInfo->ucBssIndex);
				}
			}
#if CFG_SUPPORT_ADHOC
			if (EQUAL_SSID(prBssDesc->aucSSID,
				prBssDesc->ucSSIDLen,
				prConnSettings->aucSSID,
				prConnSettings->ucSSIDLen) &&
				(prBssDesc->eBSSType == BSS_TYPE_IBSS)
				&& (prAisBssInfo->eCurrentOPMode
				== OP_MODE_IBSS)) {

				ASSERT(prSwRfb->prRxStatusGroup3);

				ibssProcessMatchedBeacon(prAdapter,
					prAisBssInfo,
					prBssDesc,
					nicRxGetRcpiValueFromRxv(
						prAdapter,
						RCPI_MODE_MAX,
						prSwRfb));
			}
#endif /* CFG_SUPPORT_ADHOC */
		}
#if CFG_SUPPORT_SCAN_LOG
		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID,
			prAisBssInfo->aucBSSID) &&
			(prAisBssInfo->eConnectionState ==
			MEDIA_STATE_CONNECTED) &&
			(prWlanBeaconFrame->u2FrameCtrl &
			MASK_FRAME_TYPE) == MAC_FRAME_BEACON)
			scanUpdateBcn(
				prAdapter,
				prBssDesc,
				prAisBssInfo->ucBssIndex);
#endif
	}

		rlmProcessBcn(prAdapter,
			prSwRfb,
			((struct WLAN_BEACON_FRAME *) (prSwRfb->pvHeader))
				->aucInfoElem,
			(prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
			(uint16_t) (OFFSET_OF(struct WLAN_BEACON_FRAME_BODY,
				aucInfoElem[0])));

		mqmProcessBcn(prAdapter,
			prSwRfb,
			((struct WLAN_BEACON_FRAME *) (prSwRfb->pvHeader))
				->aucInfoElem,
			(prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
			(uint16_t) (OFFSET_OF(struct WLAN_BEACON_FRAME_BODY,
				aucInfoElem[0])));

		prWlanInfo->u4ScanDbgTimes2++;

		/* 4 <3> Send SW_RFB_T to HIF when we perform SCAN for HOST */
		if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE
			|| prBssDesc->eBSSType == BSS_TYPE_IBSS) {
			/* for AIS, send to host */
			prWlanInfo->u4ScanDbgTimes3++;
			if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING
				|| prScanInfo->fgSchedScanning) {
				u_int8_t fgAddToScanResult = FALSE;

				fgAddToScanResult
					= scanCheckBssIsLegal(prAdapter,
						prBssDesc);
				prWlanInfo->u4ScanDbgTimes4++;

				if (fgAddToScanResult == TRUE) {
					rStatus = scanAddScanResult(prAdapter,
						prBssDesc, prSwRfb);
				}
			}
		}
#if CFG_ENABLE_WIFI_DIRECT
		if (prAdapter->fgIsP2PRegistered) {
			scanP2pProcessBeaconAndProbeResp(prAdapter, prSwRfb,
				&rStatus, prBssDesc, prWlanBeaconFrame);
		}
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		mldProcessBeaconAndProbeResp(prAdapter, prSwRfb);
#endif
	}

	return rStatus;
}	/* end of scanProcessBeaconAndProbeResp() */

void scanReportBss2Cfg80211(struct ADAPTER *prAdapter,
			    enum ENUM_BSS_TYPE eBSSType,
			    struct BSS_DESC *SpecificprBssDesc)
{
	struct SCAN_INFO *prScanInfo = NULL;
	struct LINK *prBSSDescList = NULL;
	struct BSS_DESC *prBssDesc = NULL;
#if CFG_ENABLE_WIFI_DIRECT
	struct RF_CHANNEL_INFO rChannelInfo;
#endif

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	log_dbg(SCN, TRACE, "eBSSType: %d\n", eBSSType);

	if (SpecificprBssDesc) {
		{
			/* check BSSID is legal channel */
			if (!scanCheckBssIsLegal(prAdapter,
				SpecificprBssDesc)) {
				log_dbg(SCN, TRACE,
					"Remove specific SSID[%s %d]\n",
					HIDE(SpecificprBssDesc->aucSSID),
					SpecificprBssDesc->ucChannelNum);
				return;
			}

			log_dbg(SCN, TRACE, "Report specific SSID[%s]\n",
				SpecificprBssDesc->aucSSID);

			if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
				if (SpecificprBssDesc->u2RawLength != 0) {
					kalIndicateBssInfo(
						prAdapter->prGlueInfo,
						(uint8_t *)
						SpecificprBssDesc->aucRawBuf,
						SpecificprBssDesc->u2RawLength,
						SpecificprBssDesc->ucChannelNum,
						SpecificprBssDesc->eBand,
						RCPI_TO_dBm(
						SpecificprBssDesc->ucRCPI));
				}
			} else {
#if CFG_ENABLE_WIFI_DIRECT
				rChannelInfo.ucChannelNum
					= SpecificprBssDesc->ucChannelNum;
				rChannelInfo.eBand = SpecificprBssDesc->eBand;
				kalP2PIndicateBssInfo(prAdapter->prGlueInfo,
					(uint8_t *)
						SpecificprBssDesc->aucRawBuf,
					SpecificprBssDesc->u2RawLength,
					&rChannelInfo,
					RCPI_TO_dBm(SpecificprBssDesc->ucRCPI));
#endif
			}

#if CFG_ENABLE_WIFI_DIRECT
			SpecificprBssDesc->fgIsP2PReport = FALSE;
#endif
		}
	} else {
		/* Search BSS Desc from current SCAN result list. */
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList,
			rLinkEntry, struct BSS_DESC) {

			if (prBssDesc == NULL) {
				DBGLOG(SCN, WARN,
					"NULL prBssDesc from list %u elem,%p,%p\n",
					prBSSDescList->u4NumElem,
					prBSSDescList->prPrev,
					prBSSDescList->prNext);
				return;
			}

			/* check BSSID is legal channel */
			if (!scanCheckBssIsLegal(prAdapter, prBssDesc)) {
				log_dbg(SCN, TRACE, "Remove SSID[%s %d]\n",
					HIDE(prBssDesc->aucSSID),
					prBssDesc->ucChannelNum);
				continue;
			}

			if ((prBssDesc->eBSSType == eBSSType)
#if CFG_ENABLE_WIFI_DIRECT
			    || ((eBSSType == BSS_TYPE_P2P_DEVICE)
			    && (prBssDesc->fgIsP2PReport == TRUE))
#endif
			    ) {
#if CFG_ENABLE_WIFI_DIRECT
#define TEMP_LOG_TEMPLATE "Report " MACSTR " SSID[%s %u] eBSSType[%d] " \
		"u2RawLength[%d] fgIsP2PReport[%d]\n"

			log_dbg(SCN, TRACE, TEMP_LOG_TEMPLATE,
					MAC2STR(prBssDesc->aucBSSID),
					HIDE(prBssDesc->aucSSID),
					prBssDesc->ucChannelNum,
					prBssDesc->eBSSType,
					prBssDesc->u2RawLength,
					prBssDesc->fgIsP2PReport);
#else
#define TEMP_LOG_TEMPLATE "Report " MACSTR " SSID[%s %u] eBSSType[%d] " \
		"u2RawLength[%d]\n"

			log_dbg(SCN, TRACE, TEMP_LOG_TEMPLATE,
					MAC2STR(prBssDesc->aucBSSID),
					HIDE(prBssDesc->aucSSID),
					prBssDesc->ucChannelNum,
					prBssDesc->eBSSType,
					prBssDesc->u2RawLength);
#endif

#undef TEMP_LOG_TEMPLATE

				if (eBSSType == BSS_TYPE_INFRASTRUCTURE) {
					if (prBssDesc->u2RawLength != 0) {
						kalIndicateBssInfo(
							prAdapter->prGlueInfo,
							(uint8_t *)
							prBssDesc->aucRawBuf,
							prBssDesc->u2RawLength,
							prBssDesc->ucChannelNum,
							prBssDesc->eBand,
							RCPI_TO_dBm(
							prBssDesc->ucRCPI));
					}
					prBssDesc->u2RawLength = 0;
#if CFG_ENABLE_WIFI_DIRECT
					prBssDesc->fgIsP2PReport = FALSE;
#endif
				} else {
#if CFG_ENABLE_WIFI_DIRECT
					if ((prBssDesc->fgIsP2PReport == TRUE)
					    && prBssDesc->u2RawLength != 0) {
						rChannelInfo.ucChannelNum
							= prBssDesc
								->ucChannelNum;
						rChannelInfo.eBand
							 = prBssDesc->eBand;

						kalP2PIndicateBssInfo(
							prAdapter->prGlueInfo,
							(uint8_t *)
							prBssDesc->aucRawBuf,
							prBssDesc->u2RawLength,
							&rChannelInfo,
							RCPI_TO_dBm(
							prBssDesc->ucRCPI));

						/* do not clear it then we can
						 * pass the bss in
						 * Specific report
						 */
#if 0 /* TODO: Remove this */
						kalMemZero(prBssDesc->aucRawBuf,
							CFG_RAW_BUFFER_SIZE);
#endif

						/* the BSS entry will not be
						 * cleared after scan done.
						 * So if we dont receive the BSS
						 * in next scan, we cannot pass
						 * it. We use u2RawLength for
						 * the purpose.
						 */
#if 0
						prBssDesc->u2RawLength = 0;
#endif

						prBssDesc->fgIsP2PReport
							= FALSE;
					}
#endif
				}
			}

		}
#if CFG_ENABLE_WIFI_DIRECT
		p2pFunCalAcsChnScores(prAdapter);
#endif
	}

}

#if CFG_SUPPORT_PASSPOINT
/*----------------------------------------------------------------------------*/
/*!
 * @brief Find the corresponding BSS Descriptor according to given BSSID
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @param[in] aucBSSID           Given BSSID.
 * @param[in] fgCheckSsid        Need to check SSID or not. (for multiple SSID
 *                               with single BSSID cases)
 * @param[in] prSsid             Specified SSID
 *
 * @return   Pointer to BSS Descriptor, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct BSS_DESC *scanSearchBssDescByBssidAndLatestUpdateTime(
	struct ADAPTER *prAdapter, uint8_t aucBSSID[])
{
	struct SCAN_INFO *prScanInfo;
	struct LINK *prBSSDescList;
	struct BSS_DESC *prBssDesc;
	struct BSS_DESC *prDstBssDesc = (struct BSS_DESC *) NULL;
	OS_SYSTIME rLatestUpdateTime = 0;

	ASSERT(prAdapter);
	ASSERT(aucBSSID);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prBSSDescList = &prScanInfo->rBSSDescList;

	/* Search BSS Desc from current SCAN result list. */
	LINK_FOR_EACH_ENTRY(
		prBssDesc, prBSSDescList, rLinkEntry, struct BSS_DESC) {

		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, aucBSSID)) {
			if (!rLatestUpdateTime
				|| CHECK_FOR_EXPIRATION(prBssDesc->rUpdateTime,
					rLatestUpdateTime)) {
				prDstBssDesc = prBssDesc;
				COPY_SYSTIME(rLatestUpdateTime,
					prBssDesc->rUpdateTime);
			}
		}
	}

	return prDstBssDesc;

}	/* end of scanSearchBssDescByBssid() */

#endif /* CFG_SUPPORT_PASSPOINT */

#if CFG_SUPPORT_AGPS_ASSIST
void scanReportScanResultToAgps(struct ADAPTER *prAdapter)
{
	struct LINK *prBSSDescList =
			&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	struct BSS_DESC *prBssDesc = NULL;
	struct AGPS_AP_LIST *prAgpsApList =
			kalMemAlloc(sizeof(struct AGPS_AP_LIST), VIR_MEM_TYPE);
	struct AGPS_AP_INFO *prAgpsInfo = &prAgpsApList->arApInfo[0];
	struct SCAN_INFO *prScanInfo = &prAdapter->rWifiVar.rScanInfo;
	uint8_t ucIndex = 0;

	LINK_FOR_EACH_ENTRY(
		prBssDesc, prBSSDescList, rLinkEntry, struct BSS_DESC) {

		if (prBssDesc->rUpdateTime < prScanInfo->rLastScanCompletedTime)
			continue;
		COPY_MAC_ADDR(prAgpsInfo->aucBSSID, prBssDesc->aucBSSID);
		prAgpsInfo->ePhyType = AGPS_PHY_G;
		prAgpsInfo->u2Channel = prBssDesc->ucChannelNum;
		prAgpsInfo->i2ApRssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
		prAgpsInfo++;
		ucIndex++;
		if (ucIndex == SCN_AGPS_AP_LIST_MAX_NUM)
			break;
	}
	prAgpsApList->ucNum = ucIndex;
	GET_CURRENT_SYSTIME(&prScanInfo->rLastScanCompletedTime);
	/* log_dbg(SCN, INFO, ("num of scan list:%d\n", ucIndex)); */
	kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_AP_LIST,
		(uint8_t *) prAgpsApList, sizeof(struct AGPS_AP_LIST));
	kalMemFree(prAgpsApList, VIR_MEM_TYPE, sizeof(struct AGPS_AP_LIST));
}
#endif /* CFG_SUPPORT_AGPS_ASSIST */

void scanReqLog(struct CMD_SCAN_REQ_V2 *prCmdScanReq)
{
	struct CMD_SCAN_REQ_V2 *req = prCmdScanReq;
	char *strbuf = NULL, *pos = NULL, *end = NULL;
	uint32_t slen = 0;
	int i, j;
	int snum[2] = {req->ucSSIDNum, req->ucSSIDExtNum};
	struct PARAM_SSID *slist[2] = {req->arSSID, req->arSSIDExtend};
	int cnum[2] = {req->ucChannelListNum, req->ucChannelListExtNum};
	struct CHANNEL_INFO *clist[2] =	{
		req->arChannelList, req->arChannelListExtend};

	/* ssid and space */
	for (i = 0; i < 2; ++i)
		for (j = 0; j < snum[i]; ++j)
			slen += slist[i][j].u4SsidLen + 1;

	/* The length should be added 6 + 10 + 4 + 8 + 1 for the format
	 * ",Ssid:", * ",Ext ssid:", ",Ch:", ",Ext Ch:" and null byte.
	 */
	slen += 29 + 4 * (req->ucChannelListNum + req->ucChannelListExtNum);
	pos = strbuf = kalMemAlloc(slen, VIR_MEM_TYPE);
	if (strbuf == NULL) {
		scanlog_dbg(LOG_SCAN_REQ_K2D, INFO, "Can't allocate memory\n");
		return;
	}
	end = strbuf + slen;

	for (i = 0; i < 2; ++i) {
		if (snum[i] > 0) {
			pos += kalSnprintf(pos, end - pos, "%s",
				i == 0 ? ",Ssid:" : ",Ext Ssid:");
			for (j = 0; j < snum[i]; ++j) {
				char ssid[PARAM_MAX_LEN_SSID + 1] = {0};

				kalStrnCpy(ssid,
					slist[i][j].aucSsid, sizeof(ssid));
				ssid[sizeof(ssid) - 1] = '\0';
				pos += kalSnprintf(pos, end - pos, " %s", ssid);
			}
		}
	}

	for (i = 0; i < 2; ++i) {
		if (cnum[i] > 0) {
			pos += kalSnprintf(pos, end - pos, "%s",
				i == 0 ? ",Ch:" : ",Ext Ch:");
			for (j = 0; j < cnum[i]; ++j)
				pos += kalSnprintf(pos, end - pos, " %u",
					clist[i][j].ucChannelNum % 1000);
		}
	}
#define TEMP_LOG_TEMPLATE \
	"ScanReqV2: ScanType=%d,BSS=%u,SSIDType=%d,Num=%u,Ext=%u," \
	"ChannelType=%d,Num=%d,Ext=%u,Seq=%u,Ver=%u,Dw=%u,Min=%u," \
	"Func=(0x%X,0x%X),D=%u,Mac="MACSTR",BSSID="MACSTR"," \
	"OpT=%d,DfsT=%d,ChCnt=%d %s\n"

	scanlog_dbg(LOG_SCAN_REQ_D2F, INFO, TEMP_LOG_TEMPLATE,
		prCmdScanReq->ucScanType,
		prCmdScanReq->ucBssIndex,
		prCmdScanReq->ucSSIDType,
		prCmdScanReq->ucSSIDNum,
		prCmdScanReq->ucSSIDExtNum,
		prCmdScanReq->ucChannelType,
		prCmdScanReq->ucChannelListNum,
		prCmdScanReq->ucChannelListExtNum,
		prCmdScanReq->ucSeqNum, prCmdScanReq->auVersion[0],
		prCmdScanReq->u2ChannelDwellTime,
		prCmdScanReq->u2ChannelMinDwellTime,
		prCmdScanReq->ucScnFuncMask,
		prCmdScanReq->u4ScnFuncMaskExtend,
		prCmdScanReq->u2ProbeDelayTime,
		MAC2STR(prCmdScanReq->aucRandomMac),
		MAC2STR(prCmdScanReq->aucBSSID),
		prCmdScanReq->u2OpChStayTimeMs,
		prCmdScanReq->ucDfsChDwellTimeMs,
		prCmdScanReq->ucPerScanChannelCnt,
		strbuf != pos ? strbuf : "");
#undef TEMP_LOG_TEMPLATE
	kalMemFree(strbuf, VIR_MEM_TYPE, slen);
}

void scanResultLog(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct WLAN_BEACON_FRAME *pFrame =
		(struct WLAN_BEACON_FRAME *) prSwRfb->pvHeader;
	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_FW);
	scanLogCacheAddBSS(
		&(prAdapter->rWifiVar.rScanInfo.rScanLogCache.rBSSListFW),
		prAdapter->rWifiVar.rScanInfo.rScanLogCache.arBSSListBufFW,
		LOG_SCAN_RESULT_F2D,
		pFrame->aucBSSID,
		pFrame->u2SeqCtrl);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_FW);
}

void scanLogCacheAddBSS(struct LINK *prList,
	struct SCAN_LOG_ELEM_BSS *prListBuf,
	enum ENUM_SCAN_LOG_PREFIX prefix,
	uint8_t bssId[], uint16_t seq)
{
	struct SCAN_LOG_ELEM_BSS *pSavedBss = NULL;
	struct SCAN_LOG_ELEM_BSS *pBss = NULL;

	if (LINK_IS_INVALID(prList)) {
		LINK_INITIALIZE(prList);
	}

	LINK_FOR_EACH_ENTRY(pSavedBss, prList,
		rLinkEntry, struct SCAN_LOG_ELEM_BSS) {
		if (pSavedBss && bssId) {
			if (EQUAL_MAC_ADDR(pSavedBss->aucBSSID, bssId))
				return;
		} else {
			scanlog_dbg(prefix, ERROR,
				"pSavedBss(%p) or bssid(%p) is NULL\n",
				pSavedBss, bssId);
			return;
		}
	}

	if (prList->u4NumElem < SCAN_LOG_BUFF_SIZE) {
		if (prListBuf != NULL) {
			pBss = &(prListBuf[prList->u4NumElem]);
		} else {
			scanlog_dbg(prefix, INFO, "Buffer is NULL\n");
			return;
		}
	} else {
		scanlog_dbg(prefix, INFO, "Need more buffer\n");
		return;
	}
	kalMemZero(pBss, sizeof(struct SCAN_LOG_ELEM_BSS));

	COPY_MAC_ADDR(pBss->aucBSSID, bssId);
	pBss->u2SeqCtrl = seq;

	LINK_INSERT_TAIL(prList, &(pBss->rLinkEntry));
}

void scanLogCacheFlushBSS(struct LINK *prList, enum ENUM_SCAN_LOG_PREFIX prefix)
{
	char arlogBuf[SCAN_LOG_MSG_MAX_LEN];
	uint32_t idx = 0;
	struct SCAN_LOG_ELEM_BSS *pBss = NULL;
#if CFG_SHOW_FULL_MACADDR
	/* XXXXXXXXXXXX */
	const uint8_t dataLen = 12;
#else
	/* XXXXsumXX */
	const uint8_t dataLen = 9;
#endif

	if (LINK_IS_INVALID(prList)) {
		LINK_INITIALIZE(prList);
	}

	if (LINK_IS_EMPTY(prList))
		return;

	kalMemZero(arlogBuf, SCAN_LOG_MSG_MAX_LEN);
	/* The maximum characters of uint32_t could be 10. Thus, the
	 * mininum size should be 10+3 for the format "%u: ".
	 */
	if (dataLen + 1 > SCAN_LOG_MSG_MAX_LEN) {
		scanlog_dbg(prefix, INFO, "Scan log buffer is too small.\n");
		while (!LINK_IS_EMPTY(prList)) {
			LINK_REMOVE_HEAD(prList,
				pBss, struct SCAN_LOG_ELEM_BSS *);
		}
		return;
	}
	idx += kalSnprintf(arlogBuf, SCAN_LOG_MSG_MAX_LEN, "%u: ",
			prList->u4NumElem);

	while (!LINK_IS_EMPTY(prList)) {
		if (idx+dataLen+1 > SCAN_LOG_MSG_MAX_LEN) {
			arlogBuf[idx] = 0; /* terminating null byte */
			if (prefix != LOG_SCAN_D2D)
				scanlog_dbg(prefix, INFO, "%s\n", arlogBuf);
			idx = 0;
		}

		LINK_REMOVE_HEAD(prList,
			pBss, struct SCAN_LOG_ELEM_BSS *);

#if CFG_SHOW_FULL_MACADDR
		idx += kalSnprintf(arlogBuf+idx, dataLen+1,
			"%02x%02x%02x%02x%02x%02x",
			((uint8_t *)pBss->aucBSSID)[0],
			((uint8_t *)pBss->aucBSSID)[1],
			((uint8_t *)pBss->aucBSSID)[2],
			((uint8_t *)pBss->aucBSSID)[3],
			((uint8_t *)pBss->aucBSSID)[4],
			((uint8_t *)pBss->aucBSSID)[5]);
#else
		idx += kalSnprintf(arlogBuf+idx, dataLen+1,
			"%02x%02x%03x%02x",
			((uint8_t *)pBss->aucBSSID)[0],
			((uint8_t *)pBss->aucBSSID)[1],
			((uint8_t *)pBss->aucBSSID)[2] +
			((uint8_t *)pBss->aucBSSID)[3] +
			((uint8_t *)pBss->aucBSSID)[4],
			((uint8_t *)pBss->aucBSSID)[5]);
#endif

	}
	if (idx != 0) {
		arlogBuf[idx] = 0; /* terminating null byte */
		if (prefix != LOG_SCAN_D2D)
			scanlog_dbg(prefix, INFO, "%s\n", arlogBuf);
		idx = 0;
	}
}

void scanLogCacheFlushAll(struct ADAPTER *prAdapter,
	struct SCAN_LOG_CACHE *prScanLogCache,
	enum ENUM_SCAN_LOG_PREFIX prefix)
{
	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_FW);
	scanLogCacheFlushBSS(&(prScanLogCache->rBSSListFW),
		prefix);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_FW);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_CFG);
	scanLogCacheFlushBSS(&(prScanLogCache->rBSSListCFG),
		prefix);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_BSSLIST_CFG);
}

void scanFillChnlIdleSlot(struct ADAPTER *ad, enum ENUM_BAND eBand,
	uint8_t ucChNum, uint16_t u2IdleTime)
{
	struct CHNL_IDLE_SLOT *prSlotInfo =
		&(ad->rWifiVar.rScanInfo.rSlotInfo);
	uint8_t index = 0;

	if (eBand == BAND_2G4) {
		if (ucChNum < 1 || ucChNum > 14)
			return;

		index = (ucChNum - 1);
		prSlotInfo->au2ChIdleTime2G4[index] = u2IdleTime;
	} else if (eBand == BAND_5G) {
		if (ucChNum < 36 || ucChNum > 165)
			return;

		if (ucChNum >= 36 && ucChNum <= 64)
			index = (ucChNum - 36) / 4;
		else if (ucChNum >= 100 && ucChNum <= 144)
			index = (ucChNum - 68) / 4;
		else if (ucChNum >= 149 && ucChNum <= 165)
			index = (ucChNum - 69) / 4;
		prSlotInfo->au2ChIdleTime5G[index] = u2IdleTime;
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (eBand == BAND_6G) {
		if (ucChNum < 1 || ucChNum > 233)
			return;

		index = (ucChNum - 1) / 4;
		prSlotInfo->au2ChIdleTime6G[index] = u2IdleTime;
#endif
	}
}

uint16_t scanGetChnlIdleSlot(struct ADAPTER *ad, enum ENUM_BAND eBand,
	uint8_t ucChNum)
{
	struct CHNL_IDLE_SLOT *prSlotInfo =
		&(ad->rWifiVar.rScanInfo.rSlotInfo);
	uint8_t index = 0;
	uint16_t u2Slot = 0;

	if (eBand == BAND_2G4) {
		if (ucChNum < 1 || ucChNum > 14)
			return 0;

		index = (ucChNum - 1);
		u2Slot = prSlotInfo->au2ChIdleTime2G4[index];
	} else if (eBand == BAND_5G) {
		if (ucChNum < 36 || ucChNum > 165)
			return 0;

		if (ucChNum >= 36 && ucChNum <= 64)
			index = (ucChNum - 36) / 4;
		else if (ucChNum >= 100 && ucChNum <= 144)
			index = (ucChNum - 68) / 4;
		else if (ucChNum >= 149 && ucChNum <= 165)
			index = (ucChNum - 69) / 4;
		u2Slot = prSlotInfo->au2ChIdleTime5G[index];
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (eBand == BAND_6G) {
		if (ucChNum < 1 || ucChNum > 233)
			return 0;

		index = (ucChNum - 1) / 4;
		u2Slot = prSlotInfo->au2ChIdleTime6G[index];
#endif
	}

	return u2Slot;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief Remove and clean BSS Descriptors from the list.
 *
 * @param[in] prBSSDescList Pointer to the LINK structure.
 * @param[in] prBssDesc     Pointer to the BSS_DESC structure.
 * @param[in] prAdapter     Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanRemoveBssDescFromList(struct ADAPTER *prAdapter,
			       struct LINK *prBSSDescList,
			       struct BSS_DESC *prBssDesc)
{
	if (prAdapter != NULL && prBssDesc != NULL) {
		/* Remove this BSS Desc from the BSS Desc list */
		if (prBSSDescList != NULL)
			LINK_REMOVE_KNOWN_ENTRY(prBSSDescList, prBssDesc);
	}
}	/* end of scanRemoveBssDescFromList() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Insert and initialize the BSS Descriptors to the list.
 *
 * @param[in] prBSSDescList Pointer to the LINK structure.
 * @param[in] prBssDesc     Pointer to the BSS_DESC structure.
 * @param[in] init          TRUE for initializing the BSS_DESC.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanInsertBssDescToList(struct LINK *prBSSDescList,
			     struct BSS_DESC *prBssDesc,
			     u_int8_t init)
{
	if (prBssDesc != NULL) {
		if (init == TRUE) {
			/* This will reset the link relationship */
			kalMemZero(prBssDesc, sizeof(struct BSS_DESC));
			prBssDesc->fgIsInUse = TRUE;
#if CFG_ENABLE_WIFI_DIRECT
			LINK_INITIALIZE(&(prBssDesc->rP2pDeviceList));
			prBssDesc->fgIsP2PPresent = FALSE;
#endif /* CFG_ENABLE_WIFI_DIRECT */
		}

		/* Insert this BSS Desc to the BSS Desc list */
		if (prBSSDescList != NULL)
			LINK_INSERT_TAIL(prBSSDescList, &prBssDesc->rLinkEntry);
	}
}	/* end of scanInsertBssDescToList() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Reset the BSS Descriptors.
 *
 * @param[in] prAdapter  Pointer to the Adapter structure.
 * @param[in] prBssDesc  Pointer to the BSS_DESC structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanResetBssDesc(struct ADAPTER *prAdapter,
		      struct BSS_DESC *prBssDesc)
{
	struct LINK *prBSSDescList =
		&prAdapter->rWifiVar.rScanInfo.rBSSDescList;

	scanRemoveBssDescFromList(prAdapter,
		prBSSDescList,
		prBssDesc);
	scanInsertBssDescToList(prBSSDescList,
		prBssDesc,
		TRUE);
}	/* end of scanResetBssDesc() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if VHT IE exists in Vendor Epigram IE.
 *
 * @param[in] pucBuf     Pointer to the Vendor IE.
 * @param[in] prBssDesc  Pointer to the BSS_DESC structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanCheckEpigramVhtIE(uint8_t *pucBuf, struct BSS_DESC *prBssDesc)
{
	uint32_t u4EpigramOui;
	uint16_t u2EpigramVendorType;
	struct IE_VENDOR_EPIGRAM_IE *prEpiIE;
	uint8_t *pucIE;
	uint16_t u2IELength;
	uint16_t u2Offset = 0;

	if (pucBuf == NULL) {
		DBGLOG(RLM, WARN, "[Epigram] pucBuf is NULL, skip!\n");
		return;
	}
	if (prBssDesc == NULL) {
		DBGLOG(RLM, WARN, "[Epigram] prBssDesc is NULL, skip!\n");
		return;
	}

	prEpiIE = (struct IE_VENDOR_EPIGRAM_IE *) pucBuf;
	u2IELength = prEpiIE->ucLength -
		(uint16_t) OFFSET_OF(struct IE_VENDOR_EPIGRAM_IE, pucData[0]);
	WLAN_GET_FIELD_BE24(prEpiIE->aucOui, &u4EpigramOui);
	WLAN_GET_FIELD_BE16(prEpiIE->aucVendorType, &u2EpigramVendorType);
	if (u4EpigramOui != VENDOR_IE_EPIGRAM_OUI)
		return;
	if (u2EpigramVendorType != VENDOR_IE_EPIGRAM_VHTTYPE1 &&
	    u2EpigramVendorType != VENDOR_IE_EPIGRAM_VHTTYPE2 &&
	    u2EpigramVendorType != VENDOR_IE_EPIGRAM_VHTTYPE3)
		return;

	pucIE = prEpiIE->pucData;
	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		switch (IE_ID(pucIE)) {
		case ELEM_ID_VHT_CAP:
			scanParseVHTCapIE(pucIE, prBssDesc);
			break;
		case ELEM_ID_VHT_OP:
			scanParseVHTOpIE(pucIE, prBssDesc);
			break;
		default:
			break;
		}
	}
}

void scanParseVHTCapIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc)
{
	struct IE_VHT_CAP *prVhtCap = NULL;
	uint16_t u2TxMcsSet = 0;
	uint8_t ucSpatial = 0;
	uint8_t j = 0;

	prVhtCap = (struct IE_VHT_CAP *) pucIE;
	/* Error handling */
	if (IE_LEN(prVhtCap) != (sizeof(struct IE_VHT_CAP) - 2)) {
		DBGLOG(SCN, WARN,
			"VhtCap wrong length!(%lu)->(%d)\n",
			(sizeof(struct IE_VHT_CAP) - 2),
			IE_LEN(prVhtCap));
		return;
	}

	u2TxMcsSet = prVhtCap->rVhtSupportedMcsSet.u2TxMcsMap;
	prBssDesc->fgIsVHTPresent = TRUE;
#if CFG_SUPPORT_BFEE
#define __LOCAL_VAR__ \
VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_OFFSET

	prBssDesc->ucVhtCapNumSoundingDimensions =
		(prVhtCap->u4VhtCapInfo
		& VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS)
		>> __LOCAL_VAR__;
#undef __LOCAL_VAR__
#endif

	prBssDesc->u2MaximumMpdu = (prVhtCap->u4VhtCapInfo &
		VHT_CAP_INFO_MAX_MPDU_LEN_MASK);

	/* Support AP Selection*/
	if (prBssDesc->fgMultiAnttenaAndSTBC)
		return;

	for (; j < 8; j++) {
		if ((u2TxMcsSet & BITS(2 * j, 2 * j + 1)) != 3)
			ucSpatial++;
	}
	prBssDesc->fgMultiAnttenaAndSTBC =
		((ucSpatial > 1) && (prVhtCap->u4VhtCapInfo &
			VHT_CAP_INFO_TX_STBC));
}

void scanParseVHTOpIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc)
{
	struct IE_VHT_OP *prVhtOp = NULL;

	prVhtOp = (struct IE_VHT_OP *) pucIE;
	if (IE_LEN(prVhtOp) != (sizeof(struct IE_VHT_OP) - 2))
		return;
	prBssDesc->eChannelWidth = (enum ENUM_CHANNEL_WIDTH)
		(prVhtOp->ucVhtOperation[0]);
	prBssDesc->ucCenterFreqS1 = (enum ENUM_CHANNEL_WIDTH)
		(prVhtOp->ucVhtOperation[1]);
	prBssDesc->ucCenterFreqS2 = (enum ENUM_CHANNEL_WIDTH)
		(prVhtOp->ucVhtOperation[2]);

	/*add IEEE BW160 patch*/
	rlmModifyVhtBwPara(&prBssDesc->ucCenterFreqS1,
			   &prBssDesc->ucCenterFreqS2,
			   prBssDesc->ucCenterFreqS3,
			   (uint8_t *)&prBssDesc->eChannelWidth);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if Adaptive 11r  IE exists in Vendor Cisco IE.
 *
 * @param[in] pucBuf     Pointer to the Vendor IE.
 * @param[in] prBssDesc  Pointer to the BSS_DESC structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void scanCheckAdaptive11rIE(uint8_t *pucBuf, struct BSS_DESC *prBssDesc)
{
	uint32_t oui;
	struct IE_VENDOR_ADAPTIVE_11R_IE *ie;
	uint8_t data;
	uint16_t len;

	if (pucBuf == NULL || prBssDesc == NULL) {
		DBGLOG(SCN, WARN, "adp11r pucBuf %p, prBssDesc %p, skip!\n",
			pucBuf, prBssDesc);
		return;
	}

	ie = (struct IE_VENDOR_ADAPTIVE_11R_IE *) pucBuf;
	len = ie->ucLength;
	if (len < 5 || len > 8)
		return;

	WLAN_GET_FIELD_BE24(ie->aucOui, &oui);
	if (oui != VENDOR_IE_CISCO_OUI ||
	    *(ie->aucVendorType) != VENDOR_IE_CISCO_TYPE)
		return;

	data = *(ie->pucData);
	prBssDesc->ucIsAdaptive11r = data & BIT(0);
	DBGLOG(SCN, TRACE, "BSSDesc [" MACSTR "] adaptive11r = %d\n",
		MAC2STR(prBssDesc->aucBSSID), prBssDesc->ucIsAdaptive11r);
}

void scanParseCheckMTKOuiIE(struct ADAPTER *prAdapter,
	uint8_t *pucIE, struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand, uint16_t u2FrameCtrl)
{
	uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint8_t *aucCapa = MTK_OUI_IE(pucIE)->aucCapability;
	uint8_t *ie, *sub;
	uint16_t ie_len, ie_offset, sub_len, sub_offset;

	/* only check tlv */
	if (IE_LEN(pucIE) < ELEM_MIN_LEN_MTK_OUI ||
	    kalMemCmp(pucIE + 2, aucMtkOui, sizeof(aucMtkOui)) ||
	    !(aucCapa[0] & MTK_SYNERGY_CAP_SUPPORT_TLV))
		return;

	ie = MTK_OUI_IE(pucIE)->aucInfoElem;
	ie_len = IE_LEN(pucIE) - 7;

	IE_FOR_EACH(ie, ie_len, ie_offset) {
#if CFG_SUPPORT_MLR
		if (IE_ID(ie) == MTK_OUI_ID_MLR) {
			struct IE_MTK_MLR *prMLR = (struct IE_MTK_MLR *)ie;
			/* MLR Type = 0x01 */
			prBssDesc->ucMlrType = prMLR->ucId;
			/* MLR Length = 0x01 */
			prBssDesc->ucMlrLength = prMLR->ucLength;
			/* LR bitmap:
			 * BIT[0]-MLR_V1,
			 * BIT[1]->MLR_V2,
			 * BIT[2]MLR+,
			 * BIT[3]->ALR,
			 * BIT[4]->DUAL_CTS
			 */
			prBssDesc->ucMlrSupportBitmap = prMLR->ucLRBitMap;

			/* For 2.4G AP foolproof, only MLRv1 */
			if ((prAdapter->u4MlrSupportBitmap
				& prBssDesc->ucMlrSupportBitmap)
				== MLR_MODE_MLR_V1)
				prBssDesc->ucMlrSupportBitmap =
					(prMLR->ucLRBitMap &
					(!MLR_BAND_IS_SUPPORT(prBssDesc->eBand)
					? MLR_MODE_NOT_SUPPORT : ~0));

			MLR_DBGLOG(prAdapter, SCN, INFO,
				"MLR beacon - BSSID:" MACSTR
#if (CFG_SUPPORT_802_11BE_MLO == 1)
				" ,MLIE Valid:%d|LinkId:%d|Mld Addr:" MACSTR
#endif
				" ,MLRIE Type|Len|B[0x%02x, 0x%02x, 0x%02x]\n",
				MAC2STR(prBssDesc->aucBSSID),
#if (CFG_SUPPORT_802_11BE_MLO == 1)
				prBssDesc->rMlInfo.fgValid,
				prBssDesc->rMlInfo.ucLinkIndex,
				MAC2STR(prBssDesc->rMlInfo.aucMldAddr),
#endif
				prBssDesc->ucMlrType,
				prBssDesc->ucMlrLength,
				prBssDesc->ucMlrSupportBitmap);
		}
#endif
		if (IE_ID(ie) == MTK_OUI_ID_PRE_WIFI7) {
			struct IE_MTK_PRE_WIFI7 *prPreWifi7 =
				(struct IE_MTK_PRE_WIFI7 *)ie;

			DBGLOG(SCN, LOUD, "MTK_OUI_PRE_WIFI7 %d.%d",
				prPreWifi7->ucVersion1, prPreWifi7->ucVersion0);
			DBGLOG_MEM8(SCN, LOUD, ie, IE_SIZE(ie));

			sub = prPreWifi7->aucInfoElem;
			sub_len = IE_LEN(prPreWifi7) - 2;

			IE_FOR_EACH(sub, sub_len, sub_offset) {
#if (CFG_SUPPORT_802_11BE == 1)
				if (IE_ID_EXT(sub) == ELEM_EXT_ID_EHT_CAPS)
					scanParseEhtCapIE(sub, prBssDesc);

				if (IE_ID_EXT(sub) == ELEM_EXT_ID_EHT_OP)
					scanParseEhtOpIE(prAdapter, sub,
						prBssDesc, eHwBand);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
				if (IE_ID_EXT(sub) == ELEM_EXT_ID_MLD)
					scanParseMldIE(prAdapter, prBssDesc,
						(const uint8_t *)sub,
						u2FrameCtrl);
#endif
#endif
			}
		}

		if (IE_ID(ie) == MTK_OUI_ID_CHIP_CAP && IE_LEN(ie) == 8) {
			struct IE_MTK_CHIP_CAP *prCapIe =
				(struct IE_MTK_CHIP_CAP *)ie;

			DBGLOG(SCN, LOUD, "MTK_OUI_CHIP_CAP");
			DBGLOG_MEM8(SCN, LOUD, prCapIe, IE_SIZE(prCapIe));
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (prCapIe->u8ChipCap & CHIP_CAP_ICV_V1)
				prBssDesc->rMlInfo.fgMldType =
					MLD_TYPE_ICV_METHOD_V1;
			if (prCapIe->u8ChipCap & CHIP_CAP_ICV_V1_1)
				prBssDesc->rMlInfo.fgMldType =
					MLD_TYPE_ICV_METHOD_V1_1;
			if (prCapIe->u8ChipCap & CHIP_CAP_ICV_V2)
				prBssDesc->rMlInfo.fgMldType =
					MLD_TYPE_ICV_METHOD_V2;
#endif
		}
	}
}

void scanParseWMMIE(struct ADAPTER *prAdapter,
	uint8_t *pucIE, struct BSS_DESC *prBssDesc)
{
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA;

	if ((WMM_IE_OUI_TYPE(pucIE) == VENDOR_OUI_TYPE_WMM) &&
	    (!kalMemCmp(WMM_IE_OUI(pucIE), aucWfaOui, 3))) {
		struct IE_WMM_PARAM *prWmmParam =
			(struct IE_WMM_PARAM *)pucIE;

		switch (WMM_IE_OUI_SUBTYPE(pucIE)) {
		case VENDOR_OUI_SUBTYPE_WMM_PARAM:
			/* WMM Param IE with a wrong length */
			if (IE_LEN(pucIE) != 24)
				break;
			prBssDesc->fgIsUapsdSupported =
				!!(prWmmParam->ucQosInfo &
				   WMM_QOS_INFO_UAPSD);
			break;

		case VENDOR_OUI_SUBTYPE_WMM_INFO:
			/* WMM Info IE with a wrong length */
			if (IE_LEN(pucIE) != 7)
				break;
			prBssDesc->fgIsUapsdSupported =
				(((((struct IE_WMM_INFO *)
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
}

void scanHandleOceIE(struct SCAN_PARAM *prScanParam,
	struct CMD_SCAN_REQ_V2 *prCmdScanReq)
{
	uint16_t u2Offset = 0, u2IEsBufLen = prScanParam->u2IELen;
	uint8_t *pucBuf = prScanParam->aucIE;
	uint8_t *pucBufAppend = NULL;
	uint8_t aucOceOui[] = VENDOR_OUI_WFA_SPECIFIC;
	struct IE_FILS_REQ_FRAME *prFilsReqIe;

	DBGLOG(SCN, INFO, "before OCE IE, length = %d\n", u2IEsBufLen);
	dumpMemory8(pucBuf, u2IEsBufLen);
	/* Find MaxChannelTime in FILS request parameter,
	 * it shall > 10 and not equal to 255 (TUs)
	 */
	IE_FOR_EACH(pucBuf, u2IEsBufLen, u2Offset) {
		if (IE_ID(pucBuf) == ELEM_ID_RESERVED &&
		    IE_ID_EXT(pucBuf) == ELEM_EXT_ID_FILS_REQUEST_PARA) {
			prFilsReqIe = (struct IE_FILS_REQ_FRAME *) pucBuf;

			if (prFilsReqIe->ucMaxChannelTime < 10 ||
			    prFilsReqIe->ucMaxChannelTime == 255) {
				prFilsReqIe->ucMaxChannelTime =
				    SCAN_CHANNEL_DWELL_TIME_OCE;
				prCmdScanReq->u2ChannelMinDwellTime =
				    SCAN_CHANNEL_DWELL_TIME_MIN_MSEC;
				prCmdScanReq->u2ChannelDwellTime =
				    SCAN_CHANNEL_DWELL_TIME_OCE;
			}
		/* Append OCE_ATTR_ID_SUPPRESSION_BSSID to OCE IE tail,
		 * length = 0. So total 2bytes(ID and Length).
		 * FOR OCE CERTIFICATION 5.3.1
		 */
		} else if (IE_ID(pucBuf) == ELEM_ID_VENDOR) {
			if ((OCE_IE_OUI_TYPE(pucBuf) == VENDOR_OUI_TYPE_MBO) &&
			      (!kalMemCmp(OCE_IE_OUI(pucBuf), aucOceOui, 3))) {
				/* point to OCE IE's tail */
				pucBufAppend =
					(uint8_t *)(pucBuf + IE_SIZE(pucBuf));

				/* If OCE IE is not last IE, we need to add 2
				 * byte offset after OCE IE. In order to insert
				 * 2 bytes OCE_ATTR_ID_SUPPRESSION_BSSID.
				 */
				if (u2IEsBufLen > (u2Offset + IE_SIZE(pucBuf)))
					kalMemMove(pucBufAppend + 2,
						pucBufAppend,
						u2IEsBufLen -
						(u2Offset + IE_SIZE(pucBuf)));

				OCE_OUI_SUP_BSSID(pucBufAppend)->ucAttrId =
						OCE_ATTR_ID_SUPPRESSION_BSSID;
				OCE_OUI_SUP_BSSID(pucBufAppend)->ucAttrLength
						= 0;
				prScanParam->u2IELen += 2;
				IE_LEN(pucBuf) += 2;
			}
		}
	}

	pucBuf = prScanParam->aucIE;
	DBGLOG(SCN, INFO, "After OCE IE, length = %d\n", prScanParam->u2IELen);
	dumpMemory8(pucBuf, prScanParam->u2IELen);
}

uint8_t	*scanGetFilsCacheIdFromBssDesc(struct BSS_DESC *bss)
{
	uint8_t *ie;
	uint16_t ie_len, offset;

	if (!bss)
		return NULL;

	ie = bss->pucIeBuf;
	ie_len = bss->u2IELength;

	IE_FOR_EACH(ie, ie_len, offset) {
		if (IE_ID(ie) == ELEM_ID_FILS_INDICATION && IE_LEN(ie) >= 4) {
			uint16_t info;

			WLAN_GET_FIELD_16(ie + 2, &info);
			if (info & FILS_INFO_CACHE_ID_INCLUDED)
				return ie + 4;
		}
	}

	return NULL;
}

#if (CFG_SUPPORT_WIFI_6G == 1)
void scanParseHEOpIE(struct ADAPTER *prAdapter, uint8_t *pucIE,
		     struct BSS_DESC *prBssDesc, enum ENUM_BAND eHwBand)
{
	struct _IE_HE_OP_T *prHeOp = (struct _IE_HE_OP_T *) pucIE;
	uint32_t u4Offset = OFFSET_OF(struct _IE_HE_OP_T, aucVarInfo[0]);
	struct _6G_OPER_INFOR_T *pr6gOperInfor = NULL;

	log_dbg(SCN, LOUD,
			"HEOpIE6gParam:%x,%x,%x\n",
			prHeOp->ucHeOpParams[0],
			prHeOp->ucHeOpParams[1],
			prHeOp->ucHeOpParams[2]);

	if (HE_IS_VHT_OP_INFO_PRESENT(prHeOp->ucHeOpParams) &&
		(eHwBand == BAND_6G))
		return;

	if (HE_IS_CO_HOSTED_BSS(prHeOp->ucHeOpParams))
		prBssDesc->fgIsCoHostedBssPresent = TRUE;
	else
		prBssDesc->fgIsCoHostedBssPresent = FALSE;

	if (HE_IS_6G_OP_INFOR_PRESENT(prHeOp->ucHeOpParams) &&
		(eHwBand == BAND_6G)) {
		prBssDesc->fgIsHE6GPresent = TRUE;

		if (prBssDesc->fgIsCoHostedBssPresent)
			u4Offset += sizeof(uint8_t);

		pr6gOperInfor = (struct _6G_OPER_INFOR_T *)
			(((uint8_t *) pucIE) + u4Offset);

		prBssDesc->ucChannelNum =
			pr6gOperInfor->ucPrimaryChannel;

		prBssDesc->ucCenterFreqS1 =
			pr6gOperInfor->ucChannelCenterFreqSeg0;

		prBssDesc->ucCenterFreqS2 =
			pr6gOperInfor->ucChannelCenterFreqSeg1;

		/* central channel is above primary channel */
		if (prBssDesc->ucCenterFreqS1 > prBssDesc->ucChannelNum)
			prBssDesc->eSco = CHNL_EXT_SCA;
		/* central channel is below primary channel */
		else if (prBssDesc->ucCenterFreqS1 < prBssDesc->ucChannelNum)
			prBssDesc->eSco = CHNL_EXT_SCB;
		else if (prBssDesc->ucCenterFreqS1 == prBssDesc->ucChannelNum)
			prBssDesc->eSco = CHNL_EXT_SCN;

		prBssDesc->He6gRegInfo =
			pr6gOperInfor->rControl.bits.RegulatoryInfo;

		rlmTransferHe6gOpInfor(prAdapter, prBssDesc->ucChannelNum,
			(uint8_t)pr6gOperInfor->rControl.bits.ChannelWidth,
			(uint8_t *)&prBssDesc->eChannelWidth,
			&prBssDesc->ucCenterFreqS1,
			&prBssDesc->ucCenterFreqS2,
			&prBssDesc->eSco);

		log_dbg(SCN, TRACE,
			"BSSID:" MACSTR
			" SSID:%s HE6G_OPINFOR:%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->aucSSID,
			prBssDesc->ucChannelNum,
			(uint8_t)pr6gOperInfor->rControl.bits.ChannelWidth,
			prBssDesc->eChannelWidth,
			pr6gOperInfor->ucChannelCenterFreqSeg0,
			pr6gOperInfor->ucChannelCenterFreqSeg1,
			pr6gOperInfor->ucMinimumRate,
			prBssDesc->ucCenterFreqS1,
			prBssDesc->ucCenterFreqS2,
			prBssDesc->eSco);
	} else
		prBssDesc->fgIsHE6GPresent = FALSE;
}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
void scanParseEhtCapIE(uint8_t *pucIE, struct BSS_DESC *prBssDesc)
{
	struct IE_EHT_CAP *ehtCap = NULL;

	ehtCap = (struct IE_EHT_CAP *) pucIE;
	prBssDesc->fgIsEHTPresent = TRUE;
	prBssDesc->u2MaximumMpdu = (ehtCap->ucEhtMacCap[0] &
		EHT_MAC_CAP_MAX_MPDU_LEN_MASK);
	memcpy(prBssDesc->ucEhtPhyCapInfo, ehtCap->ucEhtPhyCap,
		EHT_PHY_CAP_BYTE_NUM);

	DBGLOG(SCN, TRACE,
		"BSSID:" MACSTR
		" SSID:%s, EHT CAP IE\n",
		MAC2STR(prBssDesc->aucBSSID),
		prBssDesc->aucSSID);
	DBGLOG_MEM8(SCN, LOUD, pucIE, IE_SIZE(pucIE));
}

void scanParseEhtOpIE(struct ADAPTER *prAdapter, uint8_t *pucIE,
		      struct BSS_DESC *prBssDesc, enum ENUM_BAND eHwBand)
{
	struct IE_EHT_OP *prEhtOp;
	struct EHT_OP_INFO *prEhtOpInfo;
	uint8_t ucVhtOpBw = 0;
	uint8_t ucBssOpBw = 0;

	prEhtOp = (struct IE_EHT_OP *) pucIE;

	if (EHT_IS_OP_PARAM_OP_INFO_PRESENT(prEhtOp->ucEhtOpParams)) {
		prEhtOpInfo = (struct EHT_OP_INFO *) prEhtOp->aucVarInfo;
		ucVhtOpBw = ehtRlmGetVhtOpBwByEhtOpBw(prEhtOpInfo);
		if (ucVhtOpBw == VHT_MAX_BW_INVALID) {
			DBGLOG(SCN, WARN, "invalid Bss OP BW, control: %d\n",
				prEhtOpInfo->ucControl);
			return;
		}

		prBssDesc->eChannelWidth = ucVhtOpBw;
		prBssDesc->ucCenterFreqS1 = nicGetS1(prAdapter,
			prBssDesc->eBand, prBssDesc->ucChannelNum,
			prBssDesc->eChannelWidth);
		prBssDesc->ucCenterFreqS2 = 0;

		ucBssOpBw = prEhtOpInfo->ucControl & BITS(0, 2);

		if (ucBssOpBw == EHT_MAX_BW_20)
			prBssDesc->eSco = CHNL_EXT_SCN;
		else if (ucBssOpBw == EHT_MAX_BW_40)
			/* central channel is above primary channel */
			if (prBssDesc->ucCenterFreqS1 >
			    prBssDesc->ucChannelNum)
				prBssDesc->eSco = CHNL_EXT_SCA;
			/* central channel is below primary channel */
			else if (prBssDesc->ucCenterFreqS1 <
				 prBssDesc->ucChannelNum)
				prBssDesc->eSco = CHNL_EXT_SCB;

		DBGLOG(SCN, TRACE,
			"[EHT OP IE] BSSID:" MACSTR
			" SSID:%s CH: %u, BW: %u S1: %u S2: %u fixed s1: %u fixed s2: %u Sco: %u\n",
			MAC2STR(prBssDesc->aucBSSID),
			prBssDesc->aucSSID,
			prBssDesc->ucChannelNum,
			prBssDesc->eChannelWidth,
			prEhtOpInfo->ucCCFS0,
			prEhtOpInfo->ucCCFS1,
			prBssDesc->ucCenterFreqS1,
			prBssDesc->ucCenterFreqS2,
			prBssDesc->eSco);
	}
	DBGLOG_MEM8(SCN, LOUD, pucIE, IE_SIZE(pucIE));
}
#endif

void scanOpClassToBand(uint8_t ucOpClass, uint8_t *band)
{
	switch (ucOpClass) {
	case 112:
	case 115:
	case 116:
	case 117:
	case 118:
	case 119:
	case 120:
	case 121:
	case 122:
	case 123:
	case 124:
	case 125:
	case 126:
	case 127:
	case 128:
	case 129:
	case 130:
		*band = BAND_5G;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case 131:
	case 132:
	case 133:
	case 134:
	case 135:
	case 136:
	case 137:
		*band = BAND_6G;
		break;
#endif
	case 81:
	case 82:
	case 83:
	case 84:
		*band = BAND_2G4;
		break;
	/* not support 60Ghz */
	case 180:
	default:
		*band = BAND_NULL;
		log_dbg(SCN, WARN, "OpClass%d illegal\n", ucOpClass);
		break;
	}
}
