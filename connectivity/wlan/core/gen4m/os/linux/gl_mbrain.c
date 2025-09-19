// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "gl_mbrain.c"
 *  \brief  This file defines the interface with Mbraink.
 *
 *    Detail description.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#if CFG_SUPPORT_MBRAIN
#include "precomp.h"
#include "gl_mbrain.h"
#include "gl_vendor.h"
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define WIFI2MBR_TAG_RETRY_LIMIT 3
#if CFG_SUPPORT_WIFI_ICCM
#if defined(MT6653)
#define WIFI_ICCM_LIMIT (5)
#else
#define WIFI_ICCM_LIMIT (0)
#endif /* MT6653 */
#else
#define WIFI_ICCM_LIMIT (0)
#endif /* CFG_SUPPORT_WIFI_ICCM */
/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/* Table specified the mapping between tag and funciton pointers */
struct wifi2mbr_handler g_arMbrHdlr[] = {
	{WIFI2MBR_TAG_LLS_RATE, sizeof(struct wifi2mbr_llsRateInfo),
		mbr_wifi_lls_handler, mbr_wifi_lls_get_total_data_num},
	{WIFI2MBR_TAG_LLS_RADIO, sizeof(struct wifi2mbr_llsRadioInfo),
		mbr_wifi_lls_handler, mbr_wifi_lls_get_total_data_num},
	{WIFI2MBR_TAG_LLS_AC, sizeof(struct wifi2mbr_llsAcInfo),
		mbr_wifi_lls_handler, mbr_wifi_lls_get_total_data_num},
	{WIFI2MBR_TAG_LP_RATIO, sizeof(struct wifi2mbr_lpRatioInfo),
		mbr_wifi_lp_handler, mbr_wifi_lp_get_total_data_num},
	{WIFI2MBR_TAG_TXTIMEOUT, sizeof(struct wifi2mbr_TxTimeoutInfo),
		mbrWifiTxTimeoutHandler, mbrWifiTxTimeoutGetTotalDataNum},
};

int32_t g_i4CurTag = -1;
uint16_t g_u2LeftLoopNum;
uint16_t g_u2LoopNum;
struct ICCM_T g_rMbrIccm = {0};

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/* call the corresponding handler and return data to mbraink
 * retry if the handler return failed.
 * if it's still failed after retry WIFI2MBR_TAG_RETRY_LIMIT times,
 * return fail to Mbraink.
 */
enum wifi2mbr_status mbraink2wifi_get_data(void *priv,
	enum mbr2wifi_reason reason,
	enum wifi2mbr_tag tag, void *buf, uint16_t *pu2Len)
{
	struct ADAPTER *prAdapter;
	int i;
	enum wifi2mbr_status eStatus = WIFI2MBR_FAILURE;
	void *tmpBuf;
	uint16_t u2CurTagRetryCnt = 0;

	prAdapter = (struct ADAPTER *)priv;
	if (unlikely(!prAdapter))
		return WIFI2MBR_FAILURE;

	if (g_i4CurTag != -1 && g_u2LeftLoopNum == 0) {
		g_i4CurTag = -1;
		return WIFI2MBR_END;
	}

	for (i = 0; i < ARRAY_SIZE(g_arMbrHdlr); i++) {
		if (g_arMbrHdlr[i].eTag != tag)
			continue;

		if (g_i4CurTag != tag) {
			g_u2LoopNum = g_arMbrHdlr[i].pfnGetDataNum(
				prAdapter, tag);
			if (g_u2LoopNum == 0)
				return WIFI2MBR_END;

			g_u2LeftLoopNum = g_u2LoopNum;
			g_i4CurTag = tag;

			DBGLOG(REQ, INFO, "reason:%u tag:%u loopNum:%u\n",
				reason, tag, g_u2LoopNum);
		}

get_data_retry:
		tmpBuf = kalMemAlloc(g_arMbrHdlr[i].u4ExpdLen, VIR_MEM_TYPE);
		if (!tmpBuf) {
			DBGLOG(REQ, WARN,
				"Can not alloc memory for tag:%u\n", tag);
			return WIFI2MBR_FAILURE;
		}

		kalMemZero(tmpBuf, g_arMbrHdlr[i].u4ExpdLen);

		eStatus = g_arMbrHdlr[i].pfnHandler(prAdapter, tag,
			g_u2LoopNum - g_u2LeftLoopNum,
			tmpBuf, pu2Len);

		if (eStatus == WIFI2MBR_SUCCESS &&
			*pu2Len <= g_arMbrHdlr[i].u4ExpdLen) {
			kalMemCopy(buf, tmpBuf, *pu2Len);
		}

		DBGLOG_LIMITED(REQ, TRACE,
			"reason:%u tag:%u status:%u leftLoopNum:%u curLoopIdx:%u retry:%u\n",
			reason, tag, eStatus, g_u2LeftLoopNum,
			g_u2LoopNum - g_u2LeftLoopNum, u2CurTagRetryCnt);

		kalMemFree(tmpBuf, VIR_MEM_TYPE, g_arMbrHdlr[i].u4ExpdLen);

		if (eStatus == WIFI2MBR_FAILURE) {
			if (u2CurTagRetryCnt < WIFI2MBR_TAG_RETRY_LIMIT) {
				u2CurTagRetryCnt++;
				goto get_data_retry;
			}
		}

		g_u2LeftLoopNum--;
		u2CurTagRetryCnt = 0;

		break;
	}

	return eStatus;
}

/* function to register callback to Mbraink when wifi on */
void glRegCbsToMbraink(struct ADAPTER *prAdapter)
{
	struct mbraink2wifi_ops wifi2mbraink_ops = {
		.get_data = mbraink2wifi_get_data,
		.priv = (void *)prAdapter,
	};

	register_wifi2mbraink_ops(&wifi2mbraink_ops);
}

/* function to unregister callback to Mbraink when wifi off */
void glUnregCbsToMbraink(void)
{
	unregister_wifi2mbraink_ops();
}

#if CFG_SUPPORT_LLS
enum enum_mbr_wifi_ac conv_ac_to_mbr(enum ENUM_STATS_LLS_AC ac)
{
	switch (ac) {
	case STATS_LLS_WIFI_AC_VO:
		return MBR_WIFI_AC_VO;
	case STATS_LLS_WIFI_AC_VI:
		return MBR_WIFI_AC_VI;
	case STATS_LLS_WIFI_AC_BE:
		return MBR_WIFI_AC_BE;
	case STATS_LLS_WIFI_AC_BK:
		return MBR_WIFI_AC_BK;
	default:
		return MBR_WIFI_AC_MAX;
	}
}

uint16_t _lls_get_valid_rate_num(struct ADAPTER *prAdapter)
{
	uint16_t num = 0;
	struct STATS_LLS_PEER_INFO *src_peer;
	struct STATS_LLS_PEER_INFO peer_info = {0};
	struct STATS_LLS_RATE_STAT *src_rate;
	struct PEER_INFO_RATE_STAT *prPeer;
	struct STATS_LLS_RATE_STAT rRate;
	struct STA_RECORD *sta_rec;
	int32_t ofdm_idx, cck_idx;
	uint32_t rateIdx, rxMpduCount;
	uint8_t i;
	uint8_t ucBssIdx = AIS_DEFAULT_INDEX;

	prPeer = prAdapter->prLinkStatsPeerInfo;

	for (i = 0; i < CFG_STA_REC_NUM; i++, prPeer++) {
		src_peer = &prPeer->peer;
		if (src_peer->type >= STATS_LLS_WIFI_PEER_INVALID)
			continue;

		kalMemCopyFromIo(&peer_info, src_peer,
				sizeof(struct STATS_LLS_PEER_INFO));

		sta_rec = find_peer_starec(prAdapter, &peer_info);

		if (!sta_rec)
			continue;

		if (prAdapter->ucLinkStatsBssNum != 1 &&
			sta_rec->ucBssIndex != ucBssIdx)
			continue; /* collect per BSS, not a collecting one */

		ofdm_idx = -1;
		cck_idx = -1;
		src_rate = prPeer->rate;

		for (rateIdx = 0; rateIdx < STATS_LLS_RATE_NUM;
			rateIdx++, src_rate++) {

			kalMemCopyFromIo(&rRate, src_rate,
				sizeof(struct STATS_LLS_RATE_STAT));
			if (unlikely(rRate.rate.preamble == LLS_MODE_OFDM))
				ofdm_idx++;
			if (unlikely(rRate.rate.preamble == LLS_MODE_CCK))
				cck_idx++;

			if (!isValidRate(&rRate, ofdm_idx, cck_idx))
				continue;
			rxMpduCount = receivedMpduCount(sta_rec,
					&rRate, ofdm_idx >= 0 ? ofdm_idx : 0,
					cck_idx >= 0 ? cck_idx : 0);
			if (rRate.tx_mpdu || rRate.mpdu_lost ||
			    rRate.retries || rxMpduCount)
				num++;
		}
		break;
	}

	return num;
}

enum wifi2mbr_status _fill_lls_rate(struct ADAPTER *prAdapter,
	uint16_t u2CurLoopIdx, struct PEER_INFO_RATE_STAT *prPeer,
	struct STA_RECORD *sta_rec, struct wifi2mbr_llsRateInfo *dest)
{
	struct STATS_LLS_RATE_STAT *src_rate;
	struct STATS_LLS_RATE_STAT rRate;
	static uint32_t u4LlsRate;
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;

	static int32_t ofdm_idx, cck_idx;
	uint32_t rxMpduCount;

	if (u2CurLoopIdx == 0) {
		u4LlsRate = 0;
		ofdm_idx = -1;
		cck_idx = -1;
	}

	for (u4LlsRate; u4LlsRate < STATS_LLS_RATE_NUM;
		u4LlsRate++) {
		src_rate = prPeer->rate + u4LlsRate;

		kalMemCopyFromIo(&rRate, src_rate,
			sizeof(struct STATS_LLS_RATE_STAT));
		if (unlikely(rRate.rate.preamble == LLS_MODE_OFDM))
			ofdm_idx++;
		if (unlikely(rRate.rate.preamble == LLS_MODE_CCK))
			cck_idx++;

		if (!isValidRate(&rRate, ofdm_idx, cck_idx))
			continue;

		rxMpduCount = receivedMpduCount(sta_rec,
				&rRate, ofdm_idx >= 0 ? ofdm_idx : 0,
				cck_idx >= 0 ? cck_idx : 0);
		if (!(rRate.tx_mpdu || rRate.mpdu_lost ||
			rRate.retries || rxMpduCount))
			continue;

		kalMemCopy(&dest->rate_idx, &rRate.rate,
			sizeof(dest->rate_idx));
		dest->bitrate = rRate.rate.bitrate;
		dest->tx_mpdu = rRate.tx_mpdu;
		dest->rx_mpdu = rxMpduCount;
		dest->mpdu_lost = rRate.mpdu_lost;
		dest->retries = rRate.retries;
		DBGLOG_LIMITED(REQ, TRACE,
			"WIFI2MBR_LLS_RATE: rate=0x%x bitrate=%u tx/rx/lost/retry:%u/%u/%u/%u\n",
			dest->rate_idx, dest->bitrate, dest->tx_mpdu,
			dest->rx_mpdu, dest->mpdu_lost, dest->retries);

		u4LlsRate++;
		status = WIFI2MBR_SUCCESS;
		break;
	}

	return status;
}
#endif /* CFG_SUPPORT_LLS */

enum wifi2mbr_status mbr_wifi_lls_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;

#if CFG_SUPPORT_LLS
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint8_t ucBssIdx = AIS_DEFAULT_INDEX;

	static enum ENUM_MBMC_BN eLlsBand = ENUM_BAND_0;
	static enum ENUM_STATS_LLS_AC eLlsAc = STATS_LLS_WIFI_AC_VO;
	struct timespec64 tv;
	uint8_t i;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	if (!prAdapter->pucLinkStatsSrcBufAddr) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return WIFI2MBR_END;
	}

	switch (eTag) {
	case WIFI2MBR_TAG_LLS_RATE: {
		struct wifi2mbr_llsRateInfo *dest;
		struct PEER_INFO_RATE_STAT *prPeer;
		struct STA_RECORD *sta_rec;
		struct STATS_LLS_PEER_INFO *src_peer;
		struct STATS_LLS_PEER_INFO peer_info = {0};

		dest = (struct wifi2mbr_llsRateInfo *)buf;
		prPeer = prAdapter->prLinkStatsPeerInfo;
		for (i = 0; i < CFG_STA_REC_NUM; i++, prPeer++) {
			src_peer = &prPeer->peer;
			if (src_peer->type >= STATS_LLS_WIFI_PEER_INVALID)
				continue;

			kalMemCopyFromIo(&peer_info, src_peer,
					sizeof(struct STATS_LLS_PEER_INFO));

			sta_rec = find_peer_starec(prAdapter, &peer_info);
			if (!sta_rec)
				continue;

			/* collect per BSS, not a collecting one */
			if (prAdapter->ucLinkStatsBssNum != 1 &&
				sta_rec->ucBssIndex != ucBssIdx)
				continue;

			status = _fill_lls_rate(prAdapter, u2CurLoopIdx, prPeer,
				sta_rec, dest);
			if (status != WIFI2MBR_SUCCESS)
				continue;

			ktime_get_ts64(&tv);
			dest->hdr.tag = WIFI2MBR_TAG_LLS_RATE;
			dest->hdr.ver = 1;
			dest->timestamp = KAL_TIME_TO_MSEC(tv);

			*pu2Len = sizeof(*dest);
			break;
		}
	}
		break;
	case WIFI2MBR_TAG_LLS_RADIO: {
		struct WIFI_RADIO_CHANNEL_STAT *src;
		struct STATS_LLS_WIFI_RADIO_STAT rRadio;
		struct wifi2mbr_llsRadioInfo *dest;

		src = prAdapter->prLinkStatsRadioInfo;
		if (u2CurLoopIdx == 0)
			eLlsBand = ENUM_BAND_0;

		if (eLlsBand >= ENUM_BAND_NUM)
			goto lls_hdlr_end;

		kalMemCopyFromIo(&rRadio, src + eLlsBand,
			sizeof(struct STATS_LLS_WIFI_RADIO_STAT));
		ktime_get_ts64(&tv);

		dest = (struct wifi2mbr_llsRadioInfo *)buf;
		dest->hdr.tag = WIFI2MBR_TAG_LLS_RADIO;
		dest->hdr.ver = 1;
		dest->timestamp = KAL_TIME_TO_MSEC(tv);
		dest->radio = rRadio.radio;
		dest->on_time = rRadio.on_time;
		dest->tx_time = rRadio.tx_time;
		dest->rx_time = rRadio.rx_time;
		dest->on_time_scan = rRadio.on_time_scan;
		dest->on_time_roam_scan = rRadio.on_time_roam_scan;
		dest->on_time_pno_scan = rRadio.on_time_pno_scan;
		*pu2Len = sizeof(*dest);
		DBGLOG_LIMITED(REQ, TRACE,
			"WIFI2MBR_LLS_RADIO: radio=%u on/tx/rx/scan:%u/%u/%u/%u\n",
			dest->radio, dest->on_time, dest->tx_time,
			dest->rx_time, dest->on_time_scan);

		eLlsBand++;
		status = WIFI2MBR_SUCCESS;
	}
		break;
	case WIFI2MBR_TAG_LLS_AC: {
		struct STATS_LLS_WMM_AC_STAT rAc;
		struct wifi2mbr_llsAcInfo *dest;
		struct BSS_INFO *prBssInfo;

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
		if (u2CurLoopIdx == 0)
			eLlsAc = STATS_LLS_WIFI_AC_VO;

		if (!prBssInfo || eLlsAc >= STATS_LLS_WIFI_AC_MAX)
			goto lls_hdlr_end;

		kalMemCopyFromIo(&rAc,
			&prAdapter->prLinkStatsIface[ucBssIdx].ac[eLlsAc],
			sizeof(struct STATS_LLS_WMM_AC_STAT));
		ktime_get_ts64(&tv);

		dest = (struct wifi2mbr_llsAcInfo *)buf;
		dest->hdr.tag = WIFI2MBR_TAG_LLS_RADIO;
		dest->hdr.ver = 1;
		dest->timestamp = KAL_TIME_TO_MSEC(tv);
		dest->ac = conv_ac_to_mbr(eLlsAc);
		dest->tx_mpdu = rAc.tx_mpdu;
		dest->rx_mpdu = prBssInfo->u4RxMpduAc[eLlsAc];
		dest->tx_mcast = rAc.tx_mcast;
		dest->tx_ampdu = rAc.tx_ampdu;
		dest->mpdu_lost = rAc.mpdu_lost;
		dest->retries = rAc.retries;
		dest->contention_time_min = rAc.contention_time_min;
		dest->contention_time_max = rAc.contention_time_max;
		dest->contention_time_avg = rAc.contention_time_avg;
		dest->contention_num_samples = rAc.contention_num_samples;
		*pu2Len = sizeof(*dest);
		DBGLOG_LIMITED(REQ, TRACE,
			"WIFI2MBR_LLS_AC: ac=%u tx/rx/lost/retry:%u/%u/%u/%u\n",
			dest->ac, dest->tx_mpdu, dest->rx_mpdu,
			dest->mpdu_lost, dest->retries);

		eLlsAc++;
		status = WIFI2MBR_SUCCESS;
	}
		break;
	default:
		break;
	}
lls_hdlr_end:
#endif /* CFG_SUPPORT_LLS */
	return status;
}

uint16_t mbr_wifi_lls_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	uint16_t num = 0;

#if CFG_SUPPORT_LLS
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	if (!prAdapter->pucLinkStatsSrcBufAddr) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return 0;
	}

	switch (eTag) {
	case WIFI2MBR_TAG_LLS_RATE:
		num = _lls_get_valid_rate_num(prAdapter);
		break;
	case WIFI2MBR_TAG_LLS_RADIO:
		num = ENUM_BAND_NUM;
		break;
	case WIFI2MBR_TAG_LLS_AC:
		num = MBR_WIFI_AC_MAX;
		break;
	default:
		break;
	}
#endif /* CFG_SUPPORT_LLS */

	return num;
}

enum wifi2mbr_status mbr_wifi_lp_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;

#if CFG_SUPPORT_WIFI_ICCM
	struct wifi2mbr_lpRatioInfo *dest = (struct wifi2mbr_lpRatioInfo *)buf;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct timespec64 tv;
	uint32_t u4Ret = WLAN_STATUS_FAILURE;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	dest->hdr.tag = WIFI2MBR_TAG_LP_RATIO;
	dest->hdr.ver = 1;
	ktime_get_ts64(&tv);
	dest->timestamp = KAL_TIME_TO_MSEC(tv);
	dest->radio = u2CurLoopIdx;

	if (u2CurLoopIdx == 0) {
		GET_MBR_EMI_FIELD(prAdapter, u4Ret, rMbrIccmData, g_rMbrIccm);

		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN, "GET_MBR_EMI fail: 0x%x\n", u4Ret);
			return status;
		}
		DBGLOG(REQ, INFO, "[Mbrain ICCM][%llu]-[%d][%d:%d:%d:%d]\n",
			dest->timestamp,
			g_rMbrIccm.u4TotalTime,
			g_rMbrIccm.u4BandRatio[4].u4TxTime,
			g_rMbrIccm.u4BandRatio[4].u4RxTime,
			g_rMbrIccm.u4BandRatio[4].u4RxListenTime,
			g_rMbrIccm.u4BandRatio[4].u4SleepTime);
	}

	dest->tx_time =
		g_rMbrIccm.u4BandRatio[u2CurLoopIdx].u4TxTime;
	dest->rx_time =
		g_rMbrIccm.u4BandRatio[u2CurLoopIdx].u4RxTime;
	dest->rx_listen_time =
		g_rMbrIccm.u4BandRatio[u2CurLoopIdx].u4RxListenTime;
	dest->sleep_time =
		g_rMbrIccm.u4BandRatio[u2CurLoopIdx].u4SleepTime;
	dest->total_time = g_rMbrIccm.u4TotalTime;

	*pu2Len = sizeof(*dest);

	status = WIFI2MBR_SUCCESS;

#endif /* CFG_SUPPORT_WIFI_ICCM */
	return status;
}

uint16_t mbr_wifi_lp_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	uint16_t num = 0;

#if CFG_SUPPORT_WIFI_ICCM
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	if (!prAdapter->prMbrEmiData) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return 0;
	}

	num = WIFI_ICCM_LIMIT;

#endif /* CFG_SUPPORT_WIFI_ICCM */

	return num;
}

void mbrTxTimeoutEnqueue(struct ADAPTER *prAdapter,
	uint32_t u4TokenId, uint32_t u4TxTimeoutDuration,
	uint32_t u4AvgIdleSlot)
{
	struct MSDU_TOKEN_INFO *prTokenInfo;
	struct MSDU_TOKEN_ENTRY *prToken;
	struct MBRAIN_TXTIMEOUT_ENTRY *prTxTimeoutEntry = NULL;
	struct wifi2mbr_TxTimeoutInfo *prTxTimeoutInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	enum ENUM_OP_MODE eOPMode = OP_MODE_NUM;
	struct timespec64 rNowTs;
	struct GLUE_INFO *prGlueInfo = NULL;

	KAL_SPIN_LOCK_DECLARATION();

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "prAdapter is null\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}

	prTxTimeoutEntry = kalMemAlloc(sizeof(struct MBRAIN_TXTIMEOUT_ENTRY),
				VIR_MEM_TYPE);
	if (prTxTimeoutEntry == NULL) {
		DBGLOG(REQ, ERROR,
			"[MBrain] txtimeout alloc memory fail %u\n",
			sizeof(struct MBRAIN_TXTIMEOUT_ENTRY));
		return;
	}

	kalMemZero(prTxTimeoutEntry, sizeof(struct MBRAIN_TXTIMEOUT_ENTRY));

	prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
	prToken = &prTokenInfo->arToken[u4TokenId];

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prToken->ucBssIndex);
	if (prBssInfo)
		eOPMode = prBssInfo->eCurrentOPMode;

	prTxTimeoutInfo = &prTxTimeoutEntry->rTxTimeoutInfo;

	ktime_get_ts64(&rNowTs);
	prTxTimeoutInfo->hdr.tag = WIFI2MBR_TAG_TXTIMEOUT;
	prTxTimeoutInfo->hdr.ver = 1;
	prTxTimeoutInfo->timestamp = KAL_TIME_TO_MSEC(rNowTs);
	prTxTimeoutInfo->token_id = u4TokenId;
	prTxTimeoutInfo->wlan_index = prToken->ucWlanIndex;
	prTxTimeoutInfo->bss_index = prToken->ucBssIndex;
	prTxTimeoutInfo->timeout_duration = u4TxTimeoutDuration;
	prTxTimeoutInfo->operation_mode = eOPMode;
	prTxTimeoutInfo->idle_slot_diff_cnt = u4AvgIdleSlot;

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TXTIMEOUT);
	QUEUE_INSERT_TAIL(&prAdapter->rMbrTxTimeoutQueue,
				&prTxTimeoutEntry->rQueEntry);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TXTIMEOUT);

	if (prAdapter->rMbrTxTimeoutQueue.u4NumElem >
		MBR_TXTIMEOUT_QUE_CNT_MAX) {
		prTxTimeoutEntry = mbrTxTimeoutDequeue(prAdapter);
		kalMemFree(prTxTimeoutEntry, VIR_MEM_TYPE,
			sizeof(struct MBRAIN_TXTIMEOUT_ENTRY));
	}
}

struct MBRAIN_TXTIMEOUT_ENTRY *mbrTxTimeoutDequeue(struct ADAPTER *prAdapter)
{
	struct MBRAIN_TXTIMEOUT_ENTRY *prTxTimeoutEntry = NULL;

	KAL_SPIN_LOCK_DECLARATION();
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TXTIMEOUT);
	if (QUEUE_IS_NOT_EMPTY(&prAdapter->rMbrTxTimeoutQueue))
		QUEUE_REMOVE_HEAD(&prAdapter->rMbrTxTimeoutQueue,
			prTxTimeoutEntry, struct MBRAIN_TXTIMEOUT_ENTRY *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TXTIMEOUT);

	return prTxTimeoutEntry;
}

enum wifi2mbr_status mbrWifiTxTimeoutHandler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;
	struct wifi2mbr_TxTimeoutInfo *prTTInfoDest =
		(struct wifi2mbr_TxTimeoutInfo *) buf;
	struct MBRAIN_TXTIMEOUT_ENTRY *prTxTimeoutEntry = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "prAdapter is null\n");
		return WIFI2MBR_END;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	if (prAdapter->rMbrTxTimeoutQueue.u4NumElem <= 0) {
		DBGLOG(REQ, INFO, "TxTimeout Queue is empty\n");
		return WIFI2MBR_END;
	}

	prTxTimeoutEntry = mbrTxTimeoutDequeue(prAdapter);
	if (!prTxTimeoutEntry)
		return WIFI2MBR_END;

	kalMemCopy(prTTInfoDest, &prTxTimeoutEntry->rTxTimeoutInfo,
		sizeof(struct wifi2mbr_TxTimeoutInfo));
	*pu2Len = sizeof(*prTTInfoDest);

	kalMemFree(prTxTimeoutEntry, VIR_MEM_TYPE,
			sizeof(struct MBRAIN_TXTIMEOUT_ENTRY));

	status = WIFI2MBR_SUCCESS;

	return status;
}

uint16_t mbrWifiTxTimeoutGetTotalDataNum(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	return prAdapter->rMbrTxTimeoutQueue.u4NumElem;
}

void mbrIsTxTimeout(struct ADAPTER *prAdapter,
	uint32_t u4TokenId, uint32_t u4TxTimeoutDuration)
{
	struct BSS_INFO *prBssInfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	static uint32_t u4LastTxTimeoutTime;
	uint32_t u4AvgIdleSlot = 0;
	uint8_t i;

	if (!prAdapter)
		return;

	/* Check that only one STA is active */
	for (i = 0; i < prAdapter->ucSwBssIdNum; i++) {
		prBssInfo = prAdapter->aprBssInfo[i];
		if (prBssInfo &&
		    prBssInfo->eNetworkType != NETWORK_TYPE_AIS &&
		    prBssInfo->fgIsInUse) {
			return;
		}
	}

	/* The time interval to record txtimeout must be greater than
	  * interval time.If (last time + interval) > current time will not
	  * record txtimeout.
	  */
	if (TIME_BEFORE(kalGetTimeTick(), u4LastTxTimeoutTime))
		return;

	if (prAdapter->u4TxTimeoutCnt > 0)
		u4AvgIdleSlot = prAdapter->u4SumIdleSlot /
			prAdapter->u4TxTimeoutCnt;

	/* Ignore low idle slow < thr if setting in wifi.cfg */
	if (IS_FEATURE_ENABLED(prWifiVar->fgIgnoreLowIdleSlot)) {
		if (u4AvgIdleSlot < prWifiVar->u4LowIdleSlotThr)
			return;
	}

	if (prAdapter->u4SameTokenCnt > prWifiVar->u4SameTokenThr ||
		u4TxTimeoutDuration >= prWifiVar->u4TxTimeoutWarningThr) {
		u4LastTxTimeoutTime = kalGetTimeTick() +
			MBR_TXTIMEOUT_INTERVAL;
		mbrTxTimeoutEnqueue(prAdapter, u4TokenId,
			u4TxTimeoutDuration, u4AvgIdleSlot);
	}
}

#endif /* CFG_SUPPORT_MBRAIN */
