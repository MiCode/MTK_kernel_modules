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
#if (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1)
#define MBR_WIFI_WKUP_RSN_WKUP_PERIOD_MS_THRESHOLD	1000
#define MBR_WIFI_WKUP_RSN_TPUT_BPS_THRESHOLD		BIT(14)
#define MBR_WIFI_WAKEUP_INFO_MAX_QUE_CNT		32
#endif
#if CFG_SUPPORT_PCIE_MBRAIN
#define PCIE_MBRAIN_DATA_NUM (1)
#endif /* CFG_SUPPORT_PCIE_MBRAIN */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1)
static struct wifi2mbr_WiFiWkUpRsnInfo mbr_wifi_wkuprsn_info;
#endif
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
#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
	{WIFI2MBR_TAG_TXPWR_RPT, sizeof(struct wifi2mbr_txpwr),
		mbr_wifi_txpwr_handler, mbr_wifi_txpwr_get_total_data_num},
#endif
#if CFG_SUPPORT_PCIE_MBRAIN
	{WIFI2MBR_TAG_PCIE, sizeof(struct wifi2mbr_PcieInfo),
		mbrWifiPcieHandler, mbrWifiPcieGetTotalDataNum},
#endif /* CFG_SUPPORT_PCIE_MBRAIN */
#if (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1)
	{WIFI2MBR_TAG_WIFI_WKUP_REASON,
		sizeof(struct wifi2mbr_WiFiWkUpRsnInfo),
		mbr_wifi_wkup_rsn_handler,
		mbr_wifi_wkup_rsn_total_data_num},
#endif
#if CFG_SUPPORT_MBRAIN_TRX_PERF
	{WIFI2MBR_TAG_TRX_PERF, sizeof(struct wifi2mbr_TRxPerfInfo),
		mbrWifiTRxPerfHandler, mbrWifiTRxPerfGetTotalDataNum},
#endif
};

int32_t g_i4CurTag = -1;
uint16_t g_u2LeftLoopNum;
uint16_t g_u2LoopNum;
struct ICCM_T g_rMbrIccm = {0};
#if CFG_SUPPORT_PCIE_MBRAIN
struct PCIE_T g_rMbrPcie = {0};
#endif /* CFG_SUPPORT_PCIE_MBRAIN */

#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
struct TXPWR_MBRAIN_RPT_T g_rMbrTxPwrRpt = {0};
#endif
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

			DBGLOG(REQ, DEBUG, "reason:%u tag:%u loopNum:%u\n",
				reason, tag, g_u2LoopNum);
		}

get_data_retry:
		tmpBuf = kalMemAlloc(g_arMbrHdlr[i].ucExpdLen, VIR_MEM_TYPE);
		if (!tmpBuf) {
			DBGLOG(REQ, WARN,
				"Can not alloc memory for tag:%u\n", tag);
			return WIFI2MBR_FAILURE;
		}

		kalMemZero(tmpBuf, g_arMbrHdlr[i].ucExpdLen);

		eStatus = g_arMbrHdlr[i].pfnHandler(prAdapter, tag,
			g_u2LoopNum - g_u2LeftLoopNum,
			tmpBuf, pu2Len);

		if (eStatus == WIFI2MBR_SUCCESS &&
			*pu2Len <= g_arMbrHdlr[i].ucExpdLen) {
			kalMemCopy(buf, tmpBuf, g_arMbrHdlr[i].ucExpdLen);
		}

		DBGLOG_LIMITED(REQ, TRACE,
			"reason:%u tag:%u status:%u leftLoopNum:%u curLoopIdx:%u retry:%u\n",
			reason, tag, eStatus, g_u2LeftLoopNum,
			g_u2LoopNum - g_u2LeftLoopNum, u2CurTagRetryCnt);

		kalMemFree(tmpBuf, VIR_MEM_TYPE, g_arMbrHdlr[i].ucExpdLen);

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
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

	static enum ENUM_MBMC_BN eLlsBand = ENUM_BAND_0;
	static enum ENUM_STATS_LLS_AC eLlsAc = STATS_LLS_WIFI_AC_VO;
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

			KAL_GET_TS64(&rNowTs);
			dest->hdr.tag = WIFI2MBR_TAG_LLS_RATE;
			dest->hdr.ver = 1;
			dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);

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
		KAL_GET_TS64(&rNowTs);

		dest = (struct wifi2mbr_llsRadioInfo *)buf;
		dest->hdr.tag = WIFI2MBR_TAG_LLS_RADIO;
		dest->hdr.ver = 1;
		dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);
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
		KAL_GET_TS64(&rNowTs);

		dest = (struct wifi2mbr_llsAcInfo *)buf;
		dest->hdr.tag = WIFI2MBR_TAG_LLS_RADIO;
		dest->hdr.ver = 1;
		dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);
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
	uint32_t u4Ret = WLAN_STATUS_FAILURE;
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	dest->hdr.tag = WIFI2MBR_TAG_LP_RATIO;
	dest->hdr.ver = 1;
	KAL_GET_TS64(&rNowTs);
	dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);
	dest->radio = u2CurLoopIdx;

	if (u2CurLoopIdx == 0) {
		GET_MBR_EMI_FIELD(prAdapter, u4Ret, rMbrIccmData, g_rMbrIccm);

		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN, "GET_MBR_EMI fail: 0x%x\n", u4Ret);
			return status;
		}
		DBGLOG(REQ, DEBUG, "[Mbrain ICCM][%llu]-[%d][%d:%d:%d:%d]\n",
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

void mbrIsTxTimeout(struct ADAPTER *prAdapter,
	uint32_t u4TokenId, uint32_t u4TxTimeoutDuration)
{
	struct BSS_INFO *prBssInfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct MSDU_TOKEN_INFO *prTokenInfo;
	struct MSDU_TOKEN_ENTRY *prToken;
	uint32_t u4AvgIdleSlot = 0;
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

	if (!prAdapter)
		return;

	/* Check that only one STA is active */
	if (!aisGetConnectedBssInfo(prAdapter))
		return;

	if (prAdapter->u4TxTimeoutCnt > 0)
		u4AvgIdleSlot = prAdapter->u4SumIdleSlot /
			prAdapter->u4TxTimeoutCnt;

	/* Ignore low idle slow < thr if setting in wifi.cfg */
	if (IS_FEATURE_ENABLED(prWifiVar->fgIgnoreLowIdleSlot)) {
		if (u4AvgIdleSlot < prWifiVar->u4LowIdleSlotThr)
			return;
	}

	prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
	prToken = &prTokenInfo->arToken[u4TokenId];
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prToken->ucBssIndex);
	if (!prBssInfo)
		return;

	if (prAdapter->u4SameTokenCnt > prWifiVar->u4SameTokenThr ||
		u4TxTimeoutDuration >= prWifiVar->u4TxTimeoutWarningThr) {
		KAL_GET_TS64(&rNowTs);
		prBssInfo->u8TxTimeoutTS = KAL_TIME_TO_MSEC(rNowTs);
		prBssInfo->u4TokenId = u4TokenId;
		prBssInfo->u4TxTimeoutDuration = u4TxTimeoutDuration;
	}
}

#if CFG_SUPPORT_PCIE_MBRAIN
enum wifi2mbr_status mbrWifiPcieHandler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;

	struct wifi2mbr_PcieInfo *dest = (struct wifi2mbr_PcieInfo *)buf;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4Ret = WLAN_STATUS_FAILURE;
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "prAdapter is null\n");
		return WIFI2MBR_END;
	}

	if (!prAdapter->pucLinkStatsSrcBufAddr) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return WIFI2MBR_END;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	if (!pu2Len) {
		DBGLOG(REQ, WARN, "pu2Len is null\n");
		return WIFI2MBR_END;
	}

	if (!dest) {
		DBGLOG(REQ, WARN, "dest is null\n");
		return WIFI2MBR_END;
	}

	dest->hdr.tag = WIFI2MBR_TAG_PCIE;
	dest->hdr.ver = 1;
	KAL_GET_TS64(&rNowTs);
	dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);

	if (u2CurLoopIdx == 0) {
		GET_MBR_EMI_FIELD(prAdapter, u4Ret, rMbrPcieData, g_rMbrPcie);

		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN, "GET_MBR_EMI fail: 0x%x\n", u4Ret);
			return status;
		}
		DBGLOG(REQ, DEBUG,
			"[Mbrain PCIe][%llu]-[%u.%u][%u:%u.%u:%u.%u:%u.%u]\n",
			dest->timestamp,
			g_rMbrPcie.u4UpdateTimeUtcSec,
			g_rMbrPcie.u4UpdateTimeUtcUsec,
			g_rMbrPcie.u4ReqRecoveryCount,
			g_rMbrPcie.u4L0TimeS,
			g_rMbrPcie.u4L0TimeUs,
			g_rMbrPcie.u4L1TimeS,
			g_rMbrPcie.u4L1TimeUs,
			g_rMbrPcie.u4L1ssTimeS,
			g_rMbrPcie.u4L1ssTimeUs);
	}

	dest->update_time_utc_sec = g_rMbrPcie.u4UpdateTimeUtcSec;
	dest->update_time_utc_usec = g_rMbrPcie.u4UpdateTimeUtcUsec;
	dest->req_recovery_count = g_rMbrPcie.u4ReqRecoveryCount;
	dest->l0_time_s = g_rMbrPcie.u4L0TimeS;
	dest->l0_time_us = g_rMbrPcie.u4L0TimeUs;
	dest->l1_time_s = g_rMbrPcie.u4L1TimeS;
	dest->l1_time_us = g_rMbrPcie.u4L1TimeUs;
	dest->l1ss_time_s = g_rMbrPcie.u4L1ssTimeS;
	dest->l1ss_time_us = g_rMbrPcie.u4L1ssTimeUs;

	*pu2Len = sizeof(*dest);

	status = WIFI2MBR_SUCCESS;

	return status;
}

uint16_t mbrWifiPcieGetTotalDataNum(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	uint16_t num = 0;

	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	if (!prAdapter->prMbrEmiData) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return 0;
	}

	num = PCIE_MBRAIN_DATA_NUM;

	return num;
}
#endif /* CFG_SUPPORT_PCIE_MBRAIN */

#if (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1)
void mbr_wifi_wkup_rsn_action(struct ADAPTER *prAdapter,
			      enum MBR_WIFI_WKUPRSN_ACTION eAction,
			      enum enum_mbr_wifi_wkup_reason eReason,
			      uint32_t u4WkUpInfo)
{
	uint64_t u8NowMs, u8Temp = 0;
	struct PERF_MONITOR *prPerMonitor = NULL;
	struct MBR_WIFI_WKUPRSN_ENTRY *prWkUpRsnEntry = NULL;

	if (prAdapter == NULL) {
		DBGLOG(REQ, WARN, "NULL adapter\n");
		return;
	}
	if (!wlanIsDriverReady(prAdapter->prGlueInfo,
		WLAN_DRV_READY_CHECK_WLAN_ON | WLAN_DRV_READY_CHECK_RESET)) {
		DBGLOG(REQ, WARN, "driver state not ready\n");
		return;
	}
	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
	prPerMonitor = &prAdapter->rPerMonitor;
	u8NowMs = ktime_to_ms(KAL_GET_SYS_BOOTTIME());
	DBGLOG(REQ, TRACE,
		"eAction:%d, eReason:%d, u4WkUpInfo:%u\n",
		eAction, eReason, u4WkUpInfo);
	switch (eAction) {
	case MBR_WKUP_RSN_UPDATE_RESUME_TIME: {
		mbr_wifi_wkuprsn_info.resume_time = u8NowMs;
		if (u8NowMs > mbr_wifi_wkuprsn_info.suspend_time)
			u8Temp = u8NowMs - mbr_wifi_wkuprsn_info.suspend_time;
		mbr_wifi_wkuprsn_info.total_suspend_period += u8Temp;
		break;
	}
	case MBR_WKUP_RSN_UPDATE_WIFI_WKUP_TIME: {
		mbr_wifi_wkuprsn_info.wifi_wkup_time = u8NowMs;
		mbr_wifi_wkuprsn_info.wkup_reason = eReason;
		mbr_wifi_wkuprsn_info.wkup_info = u4WkUpInfo;
		break;
	}

	case MBR_WKUP_RSN_UPDATE_SUSPEND_TIME: {
		mbr_wifi_wkuprsn_info.suspend_time = u8NowMs;
		if (mbr_wifi_wkuprsn_info.wkup_reason == MBR_WIFI_NO_WKUP)
			break;
		mbr_wifi_wkuprsn_info.wifi_suspend_time = u8NowMs;
		if (u8NowMs > mbr_wifi_wkuprsn_info.wifi_wkup_time)
			u8Temp = u8NowMs -
				mbr_wifi_wkuprsn_info.wifi_wkup_time;
		if (u8Temp < MBR_WIFI_WKUP_RSN_WKUP_PERIOD_MS_THRESHOLD &&
		    prPerMonitor->ulThroughput <
		    MBR_WIFI_WKUP_RSN_TPUT_BPS_THRESHOLD) {
			/* Check if the queue is full first */
			if (prAdapter->rMbrWiFiWkUpRsnQueue.u4NumElem >=
				MBR_WIFI_WAKEUP_INFO_MAX_QUE_CNT) {
				DBGLOG(REQ, TRACE,
					"Wakeup reason queue is full.\n");
				break;
			}
			mbr_wifi_wkuprsn_info.wifi_wkup_period += u8Temp;
			/* Enqueue the result */
			prWkUpRsnEntry = kalMemZAlloc(
					sizeof(struct MBR_WIFI_WKUPRSN_ENTRY),
					PHY_MEM_TYPE);
			if (prWkUpRsnEntry == NULL) {
				DBGLOG(REQ, ERROR,
					"Alloc mem for wakeup rsn failed.\n");
				mbr_wifi_wkuprsn_info.wkup_reason =
							MBR_WIFI_NO_WKUP;
				break;
			}
			kalMemCopy(&prWkUpRsnEntry->rWiFiWkUpRsnInfo,
				   &mbr_wifi_wkuprsn_info,
				   sizeof(struct wifi2mbr_WiFiWkUpRsnInfo));
			prWkUpRsnEntry->rWiFiWkUpRsnInfo.hdr.tag =
					WIFI2MBR_TAG_WIFI_WKUP_REASON;
			prWkUpRsnEntry->rWiFiWkUpRsnInfo.hdr.ver = 1;
			prWkUpRsnEntry->rWiFiWkUpRsnInfo.timestamp = u8NowMs;
			QUEUE_INSERT_TAIL(&prAdapter->rMbrWiFiWkUpRsnQueue,
					&prWkUpRsnEntry->rQueEntry);
			/* Reset Wi-Fi wakeup status */
			mbr_wifi_wkuprsn_info.wkup_reason = MBR_WIFI_NO_WKUP;
		}
		break;
	}
	default: {
		DBGLOG(REQ, WARN, "Unsupport action: %d\n", eAction);
		break;
	}
	}
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
}

enum wifi2mbr_status mbr_wifi_wkup_rsn_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	struct MBR_WIFI_WKUPRSN_ENTRY *prWkUpRsnEntry = NULL;
	enum wifi2mbr_status eRet = WIFI2MBR_END;
	struct wifi2mbr_WiFiWkUpRsnInfo *prDst =
		(struct wifi2mbr_WiFiWkUpRsnInfo *)buf;

	if (prAdapter == NULL ||
	    !wlanIsDriverReady(prAdapter->prGlueInfo,
	    WLAN_DRV_READY_CHECK_WLAN_ON | WLAN_DRV_READY_CHECK_RESET))
		return eRet;

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
	if (QUEUE_IS_NOT_EMPTY(&prAdapter->rMbrWiFiWkUpRsnQueue)) {
		QUEUE_REMOVE_HEAD(&prAdapter->rMbrWiFiWkUpRsnQueue,
			prWkUpRsnEntry, struct MBR_WIFI_WKUPRSN_ENTRY *);
		if (prWkUpRsnEntry == NULL) {
			DBGLOG(REQ, ERROR, "NULL wakeup reason entry\n");
		} else {
			kalMemCopy(prDst, &prWkUpRsnEntry->rWiFiWkUpRsnInfo,
				sizeof(struct wifi2mbr_WiFiWkUpRsnInfo));
			*pu2Len = sizeof(*prDst);
			kalMemFree(prWkUpRsnEntry, PHY_MEM_TYPE,
				sizeof(MBR_WIFI_WKUPRSN_ENTRY));
			eRet = WIFI2MBR_SUCCESS;
		}
	}
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
	return eRet;
}

uint16_t mbr_wifi_wkup_rsn_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	uint16_t u2Ret = 0;

	if (prAdapter == NULL ||
	    !wlanIsDriverReady(prAdapter->prGlueInfo,
	    WLAN_DRV_READY_CHECK_WLAN_ON | WLAN_DRV_READY_CHECK_RESET))
		return u2Ret;

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
	u2Ret = (uint16_t)prAdapter->rMbrWiFiWkUpRsnQueue.u4NumElem;
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
	return u2Ret;
}

void mbr_wifi_wkup_rsn_clear_queue(struct ADAPTER *prAdapter)
{
	struct MBR_WIFI_WKUPRSN_ENTRY *prWkUpRsnEntry = NULL;

	if (prAdapter == NULL) {
		DBGLOG(REQ, WARN,
			"NULL prAdapter, do nothing.\n");
		return;
	}
	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
	while (QUEUE_IS_NOT_EMPTY(&prAdapter->rMbrWiFiWkUpRsnQueue)) {
		QUEUE_REMOVE_HEAD(&prAdapter->rMbrWiFiWkUpRsnQueue,
			prWkUpRsnEntry, struct MBR_WIFI_WKUPRSN_ENTRY *);
		if (prWkUpRsnEntry != NULL)
			kalMemFree(prWkUpRsnEntry, VIR_MEM_TYPE,
				sizeof(struct MBR_WIFI_WKUP_RSN_ENTRY));
	}
	KAL_RELEASE_SPIN_LOCK_BH(prAdapter, SPIN_LOCK_MBR_WKUP_RSN);
}
#endif /* (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1) */

#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
enum wifi2mbr_status mbr_wifi_txpwr_info_fill_hanler(struct ADAPTER *prAdapter,
	uint8_t ucBnIdx, uint8_t ucAntIdx,
	struct TXPWR_MBRAIN_RPT_T *prSrc, struct wifi2mbr_txpwr *dest)
{
	dest->rpt_type =
		prSrc->ucRptType;
	dest->max_bn_num =
		prSrc->ucMaxBnNum;
	dest->max_ant_num =
		prSrc->ucMaxAntNum;
	/* scenario info */
	dest->info[ucBnIdx][ucAntIdx].epa_support =
		prSrc->arInfo[ucBnIdx][ucAntIdx].fgEpaSupport;
	dest->info[ucBnIdx][ucAntIdx].cal_type =
		prSrc->arInfo[ucBnIdx][ucAntIdx].ucCalTpye;
	dest->info[ucBnIdx][ucAntIdx].center_ch =
		prSrc->arInfo[ucBnIdx][ucAntIdx].ucCenterCh;
	dest->info[ucBnIdx][ucAntIdx].mcc_idx =
		prSrc->arInfo[ucBnIdx][ucAntIdx].ucMccIdx;
	dest->info[ucBnIdx][ucAntIdx].rf_band =
		prSrc->arInfo[ucBnIdx][ucAntIdx].u4RfBand;
	dest->info[ucBnIdx][ucAntIdx].temp =
		prSrc->arInfo[ucBnIdx][ucAntIdx].i4Temp;
	dest->info[ucBnIdx][ucAntIdx].antsel =
		prSrc->arInfo[ucBnIdx][ucAntIdx].u4Antsel;
	/* coex info */
	dest->info[ucBnIdx][ucAntIdx].coex.bt_on =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rCoex.fgBtOn;
	dest->info[ucBnIdx][ucAntIdx].coex.lte_on =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rCoex.fgLteOn;
	dest->info[ucBnIdx][ucAntIdx].coex.bt_profile =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rCoex.u4BtProfile;
	dest->info[ucBnIdx][ucAntIdx].coex.pta_grant =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rCoex.u4PtaGrant;
	dest->info[ucBnIdx][ucAntIdx].coex.pta_req =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rCoex.u4PtaReq;
	dest->info[ucBnIdx][ucAntIdx].coex.curr_op_mode =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rCoex.u4CurrOpMode;
	/* d die info */
	dest->info[ucBnIdx][ucAntIdx].d_die_info.delta =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rDdieInfo.i4Delta;
	dest->info[ucBnIdx][ucAntIdx].d_die_info.target_pwr =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rDdieInfo.icTargetPwr;
	dest->info[ucBnIdx][ucAntIdx].d_die_info.comp_grp =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rDdieInfo.ucCompGrp;
	dest->info[ucBnIdx][ucAntIdx].d_die_info.fe_gain_mode =
		prSrc->arInfo[ucBnIdx][ucAntIdx].rDdieInfo.ucFeGainMode;

	DBGLOG(REQ, TRACE,
		"bn[%d]ant[%d]ver[%d]tag[%d]rpt[%d]max_bn[%d]max_ant[%d]epa[%d]cal[%d]ch[%d]rf[%d]mcc[%d]temp[%d]ant[0X%X]\n",
		ucBnIdx,
		ucAntIdx,
		dest->hdr.ver,
		dest->hdr.tag,
		dest->rpt_type,
		dest->max_bn_num,
		dest->max_ant_num,
		dest->info[ucBnIdx][ucAntIdx].epa_support,
		dest->info[ucBnIdx][ucAntIdx].cal_type,
		dest->info[ucBnIdx][ucAntIdx].center_ch,
		dest->info[ucBnIdx][ucAntIdx].mcc_idx,
		dest->info[ucBnIdx][ucAntIdx].rf_band,
		dest->info[ucBnIdx][ucAntIdx].temp,
		dest->info[ucBnIdx][ucAntIdx].antsel);

	DBGLOG(REQ, TRACE,
		"bn[%d]ant[%d]bt[%d]lte[%d]bt_pro[0X%X]pta_grant[0x%X]pta_req[0x%X]mode[%d]\n",
		ucBnIdx,
		ucAntIdx,
		dest->info[ucBnIdx][ucAntIdx].coex.bt_on,
		dest->info[ucBnIdx][ucAntIdx].coex.bt_profile,
		dest->info[ucBnIdx][ucAntIdx].coex.lte_on,
		dest->info[ucBnIdx][ucAntIdx].coex.pta_grant,
		dest->info[ucBnIdx][ucAntIdx].coex.pta_req,
		dest->info[ucBnIdx][ucAntIdx].coex.curr_op_mode);

	DBGLOG(REQ, TRACE,
		"bn[%d]ant[%d]delta[%d]target[%d]CompGrp[%d]FeGainMode[%d]\n",
		ucBnIdx,
		ucAntIdx,
		dest->info[ucBnIdx][ucAntIdx].d_die_info.delta,
		dest->info[ucBnIdx][ucAntIdx].d_die_info.target_pwr,
		dest->info[ucBnIdx][ucAntIdx].d_die_info.comp_grp,
		dest->info[ucBnIdx][ucAntIdx].d_die_info.fe_gain_mode);

	return WIFI2MBR_SUCCESS;
}
enum wifi2mbr_status mbr_wifi_txpwr_mbrain_notify(struct ADAPTER *prAdapter,
	struct TXPWR_MBRAIN_RPT_T *prSrc, struct wifi2mbr_txpwr *dest

)
{
	uint8_t ucBnIdx = 0;
	uint8_t ucAntIdx = 0;
	uint8_t ucMaxBn = ENUM_BAND_NUM;
	uint8_t ucMaxAnt = 0;
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

	kalMemZero(dest, sizeof(struct wifi2mbr_txpwr));

	dest->hdr.tag = WIFI2MBR_TAG_TXPWR_RPT;
	KAL_GET_TS64(&rNowTs);
	dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);
	dest->hdr.ver = prSrc->u1Ver;

	if (prSrc->ucMaxBnNum < ENUM_BAND_NUM)
		ucMaxBn = prSrc->ucMaxBnNum;
	else
		ucMaxBn = ENUM_BAND_NUM;

	if (prSrc->ucMaxAntNum < TXPWR_MBRAIN_ANT_NUM)
		ucMaxAnt = prSrc->ucMaxAntNum;
	else
		ucMaxAnt = TXPWR_MBRAIN_ANT_NUM;

	for (ucBnIdx = 0; ucBnIdx < ucMaxBn; ucBnIdx++) {
		for (ucAntIdx = 0; ucAntIdx < ucMaxAnt; ucAntIdx++) {
			status = mbr_wifi_txpwr_info_fill_hanler(
				prAdapter,
				ucBnIdx,
				ucAntIdx,
				prSrc,
				dest);

			if (status != WIFI2MBR_SUCCESS)
				return status;
		}
	}
	/* Not only send info by cmd, but also let mbrain to get EMI info */
	wifi2mbrain_notify(WIFI2MBR_TAG_TXPWR_RPT, dest,
				sizeof(struct wifi2mbr_txpwr), 2);

	return WIFI2MBR_SUCCESS;
}
enum wifi2mbr_status mbr_wifi_txpwr_uni_event_handler(struct ADAPTER *prAdapter,
	struct TXPWR_MBRAIN_RPT_T *prMbrRpt
)
{
	struct wifi2mbr_txpwr dest = {0};
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	if (!prAdapter->prMbrEmiData) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return WIFI2MBR_END;
	}

	/* From Event */
	mbr_wifi_txpwr_mbrain_notify(prAdapter, prMbrRpt, &dest);

	status = WIFI2MBR_SUCCESS;

	return status;
}
#endif

enum wifi2mbr_status mbr_wifi_txpwr_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;
#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
	struct wifi2mbr_txpwr *dest = (struct wifi2mbr_txpwr *)buf;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint32_t u4Ret = WLAN_STATUS_FAILURE;
	uint8_t ucBnIdx = 0;
	uint8_t ucAntIdx = 0;
	uint8_t ucMaxBn = ENUM_BAND_NUM;
	uint8_t ucMaxAnt = 0;
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return WIFI2MBR_END;
	}

	if (!prAdapter->prMbrEmiData) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return WIFI2MBR_END;
	}

	dest->hdr.tag = WIFI2MBR_TAG_TXPWR_RPT;
	KAL_GET_TS64(&rNowTs);
	dest->timestamp = KAL_TIME_TO_MSEC(rNowTs);

	if (u2CurLoopIdx == 0) {
		GET_MBR_EMI_FIELD(prAdapter, u4Ret,
				rMbrTxPwrRpt, g_rMbrTxPwrRpt);
		DBGLOG(REQ, INFO, "Get EMI done max_bn[%d]max_ant[%d]",
				g_rMbrTxPwrRpt.ucMaxBnNum,
				g_rMbrTxPwrRpt.ucMaxAntNum);

		if (u4Ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN, "GET_MBR_EMI fail: 0x%x\n", u4Ret);
			return status;
		}
	}

	dest->hdr.ver = g_rMbrTxPwrRpt.u1Ver;

	if (g_rMbrTxPwrRpt.ucMaxBnNum < ENUM_BAND_NUM)
		ucMaxBn = g_rMbrTxPwrRpt.ucMaxBnNum;
	else
		ucMaxBn = ENUM_BAND_NUM;

	if (g_rMbrTxPwrRpt.ucMaxAntNum < TXPWR_MBRAIN_ANT_NUM)
		ucMaxAnt = g_rMbrTxPwrRpt.ucMaxAntNum;
	else
		ucMaxAnt = TXPWR_MBRAIN_ANT_NUM;


	for (ucBnIdx = 0; ucBnIdx < ucMaxBn; ucBnIdx++) {
		for (ucAntIdx = 0; ucAntIdx < ucMaxAnt; ucAntIdx++) {
			status = mbr_wifi_txpwr_info_fill_hanler(
				prAdapter,
				ucBnIdx,
				ucAntIdx,
				&g_rMbrTxPwrRpt,
				dest);

			if (status != WIFI2MBR_SUCCESS)
				return status;
		}
	}

	*pu2Len = sizeof(*dest);

	status = WIFI2MBR_SUCCESS;
#endif /* CFG_SUPPORT_MBRAIN_TXPWR_RPT */
	return status;
}

uint16_t mbr_wifi_txpwr_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	uint16_t num = 0;
#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	if (!prAdapter->pucLinkStatsSrcBufAddr) {
		DBGLOG(REQ, WARN, "EMI mapping not done");
		return 0;
	}

	/* get all band / all antenna txpower info by one time */
	num = 1;
#endif /* CFG_SUPPORT_MBRAIN_TXPWR_RPT */
	return num;
}

#if CFG_SUPPORT_MBRAIN_TRX_PERF
void mbrGetLatencyData(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
	struct wifi2mbr_TRxPerfInfo *prTRxPerfInfo)
{
#if CFG_SUPPORT_MBRAIN_BIGDATA
	struct PARAM_QUERY_STA_BIG_DATA rStaParam = {0};
	struct PARAM_QUERY_TRX_LATENCY_BIG_DATA rBigDataParam = {0};
	struct BSS_INFO *prBssInfo = NULL;
	uint32_t u4QueryInfoLen;
	int32_t rStatus;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo)
		return;

	/* under BTO */
	if (!prBssInfo->prStaRecOfAP) {
		DBGLOG(REQ, DEBUG, "prStaRecOfAP is NULL\n");
		return;
	}

	rStaParam.ucWlanIdx = prBssInfo->prStaRecOfAP->ucWlanIndex;
	COPY_MAC_ADDR(rStaParam.aucMacAddr, prBssInfo->aucOwnMacAddr);
	rStatus =  wlanQueryStaBigDataByWidx(prAdapter, &rStaParam,
		sizeof(rStaParam), &u4QueryInfoLen, FALSE, ucBssIdx);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN,
			"wlanQueryStaBigDataByWidx Status=%x",
			rStatus);

	DBGLOG(REQ, TRACE, "wtbl:%u snr:%u/%u u2TxLinkSpeed:%u\n",
		rStaParam.ucWlanIdx, rStaParam.aucSnr[0],
		rStaParam.aucSnr[1], rStaParam.u2TxLinkSpeed);

	memcpy(prTRxPerfInfo->snr, &rStaParam.aucSnr,
		sizeof(rStaParam.aucSnr));

	rStatus = wlanQueryTrxLatBigDataByBssIdx(prAdapter, &rBigDataParam,
		sizeof(rBigDataParam), &u4QueryInfoLen, FALSE, ucBssIdx);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN,
			"wlanQueryTrxLatBigDataByBssIdx Status=%x",
			rStatus);

	DBGLOG(REQ, TRACE,
		"rts fail rate:%u nbi:%u cu all/not me:%u/%u\n",
		rBigDataParam.u4RtsFailRate, rBigDataParam.u4Nbi,
		rBigDataParam.ucCuAll, rBigDataParam.ucCuNotMe);

	prTRxPerfInfo->rts_fail_rate = rBigDataParam.u4RtsFailRate;
	prTRxPerfInfo->nbi = rBigDataParam.u4Nbi;
	prTRxPerfInfo->cu_all = rBigDataParam.ucCuAll;
	prTRxPerfInfo->cu_not_me = rBigDataParam.ucCuNotMe;
	memcpy(prTRxPerfInfo->ipi_hist, &rBigDataParam.au4IPIHist,
		sizeof(rBigDataParam.au4IPIHist));
#endif /* CFG_SUPPORT_MBRAIN_BIGDATA */
}

uint32_t mbrGetRxReorderDropCnt(struct ADAPTER *prAdapter)
{
	uint32_t u4RxReorderDropCnt = 0;

	u4RxReorderDropCnt =
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_REORDER_BEHIND_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl, RX_BAR_DROP_COUNT);

	return u4RxReorderDropCnt;
}

uint32_t mbrGetRxSanityDropCnt(struct ADAPTER *prAdapter)
{
	uint32_t u4RxSanityDropCnt = 0;

	u4RxSanityDropCnt =
		RX_GET_CNT(&prAdapter->rRxCtrl, RX_MIC_ERROR_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl, RX_CLASS_ERR_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl, RX_FCS_ERR_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl, RX_DAF_ERR_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl, RX_ICV_ERR_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl, RX_TKIP_MIC_ERROR_DROP_COUNT)
		+ RX_GET_CNT(&prAdapter->rRxCtrl,
				RX_CIPHER_MISMATCH_DROP_COUNT);

	return u4RxSanityDropCnt;
}


void mbrTRxPerfDataReset(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	if (!prBssInfo)
		return;

	prBssInfo->u8TxStopTS = 0;
	prBssInfo->u8TxStartTS = 0;
	prBssInfo->u8BTOTS = 0;
	prBssInfo->u8TxTimeoutTS = 0;
	prBssInfo->u4TokenId = 0;
	prBssInfo->u4TxTimeoutDuration = 0;
}

void mbrTRxPerfEnqueue(struct ADAPTER *prAdapter)
{
	struct MBRAIN_TRXPERF_ENTRY *prTRxPerfEntry = NULL;
	struct wifi2mbr_TRxPerfInfo *prTRxPerfInfo = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo = NULL;
	static uint32_t u4LastRxDropTotal, u4LastRxDropReorder;
	static uint32_t u4LastRxDropSanity, u4LastRxNapiFull;
	static uint32_t u4LastTRxPerfEnQTime;
	/* timespec64 only for internal debug */
	struct timespec64 rNowTs;

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

	prBssInfo = aisGetConnectedBssInfo(prAdapter);
	if (!prBssInfo)
		return;

	/* under BTO */
	if (!prBssInfo->prStaRecOfAP) {
		DBGLOG(REQ, DEBUG, "prStaRecOfAP is NULL\n");
		return;
	}


	/* The time interval to record trx performance must be greater
	 * than interval time. If current time < (last time + interval)
	 * will not record trx performance index.
	 */
	if (TIME_BEFORE(kalGetTimeTick(),
		(u4LastTRxPerfEnQTime + MBR_TRX_PERF_TIMEOUT_INTERVAL)))
		return;

	u4LastTRxPerfEnQTime = kalGetTimeTick();

	prTRxPerfEntry = kalMemAlloc(sizeof(struct MBRAIN_TRXPERF_ENTRY),
				VIR_MEM_TYPE);
	if (prTRxPerfEntry == NULL) {
		DBGLOG(REQ, ERROR,
			"[MBrain] TRxPerf alloc memory fail %u\n",
			sizeof(struct MBRAIN_TRXPERF_ENTRY));
		return;
	}

	kalMemZero(prTRxPerfEntry, sizeof(struct MBRAIN_TRXPERF_ENTRY));

	prLinkQualityInfo = &(prAdapter->rLinkQualityInfo);

	prTRxPerfInfo = &prTRxPerfEntry->rTRxPerfInfo;

	KAL_GET_TS64(&rNowTs);
	prTRxPerfInfo->hdr.tag = WIFI2MBR_TAG_TRX_PERF;
	prTRxPerfInfo->hdr.ver = 1;
	prTRxPerfInfo->timestamp = KAL_TIME_TO_MSEC(rNowTs);

	prTRxPerfInfo->bss_index = prBssInfo->ucBssIndex;
	prTRxPerfInfo->wlan_index = prBssInfo->prStaRecOfAP->ucWlanIndex;

	prTRxPerfInfo->tx_stop_timestamp = prBssInfo->u8TxStopTS;
	prTRxPerfInfo->tx_resume_timestamp = prBssInfo->u8TxStartTS;
	prTRxPerfInfo->bto_timestamp = prBssInfo->u8BTOTS;

	prTRxPerfInfo->tx_timeout_timestamp = prBssInfo->u8TxTimeoutTS;
	prTRxPerfInfo->token_id = prBssInfo->u4TokenId;
	prTRxPerfInfo->timeout_duration = prBssInfo->u4TxTimeoutDuration;
	prTRxPerfInfo->operation_mode = prBssInfo->eCurrentOPMode;

	prTRxPerfInfo->rx_drop_total =
			((uint32_t) RX_GET_CNT(&prAdapter->rRxCtrl,
					RX_DROP_TOTAL_COUNT)) -
				u4LastRxDropTotal;
	prTRxPerfInfo->rx_drop_reorder =
			mbrGetRxReorderDropCnt(prAdapter) -
				u4LastRxDropReorder;
	prTRxPerfInfo->rx_drop_sanity =
			mbrGetRxSanityDropCnt(prAdapter) -
				u4LastRxDropSanity;
	prTRxPerfInfo->rx_napi_full =
			((uint32_t) RX_GET_CNT(&prAdapter->rRxCtrl,
					RX_NAPI_FIFO_FULL_COUNT)) -
				u4LastRxNapiFull;

	u4LastRxDropTotal = (uint32_t) RX_GET_CNT(&prAdapter->rRxCtrl,
						RX_DROP_TOTAL_COUNT);
	u4LastRxDropReorder = mbrGetRxReorderDropCnt(prAdapter);
	u4LastRxDropSanity = mbrGetRxSanityDropCnt(prAdapter);
	u4LastRxNapiFull = (uint32_t) RX_GET_CNT(&prAdapter->rRxCtrl,
						RX_NAPI_FIFO_FULL_COUNT);

	prTRxPerfInfo->tput = prAdapter->rPerMonitor.ulThroughput;
	prTRxPerfInfo->idle_slot = prLinkQualityInfo->u8DiffIdleSlotCount;

	prTRxPerfInfo->tx_per = prLinkQualityInfo->u4CurTxPer;
	prTRxPerfInfo->tx_rate = prLinkQualityInfo->u4CurTxRate;

	prTRxPerfInfo->rx_rssi =
		prAdapter->rLinkQuality.rLq[prBssInfo->ucBssIndex].cRssi;
	prTRxPerfInfo->rx_rate = prLinkQualityInfo->u4CurRxRate;

	mbrGetLatencyData(prAdapter, prBssInfo->ucBssIndex, prTRxPerfInfo);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TRXPERF);
	QUEUE_INSERT_TAIL(&prAdapter->rMbrTRxPerfQueue,
				&prTRxPerfEntry->rQueEntry);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TRXPERF);

	mbrTRxPerfDataReset(prAdapter, prBssInfo);

	if (prAdapter->rMbrTRxPerfQueue.u4NumElem >
		MBR_TRX_PERF_QUE_CNT_MAX) {
		prTRxPerfEntry = mbrTRxPerfDequeue(prAdapter);
		kalMemFree(prTRxPerfEntry, VIR_MEM_TYPE,
			sizeof(struct MBRAIN_TRXPERF_ENTRY));
	}
}

struct MBRAIN_TRXPERF_ENTRY *mbrTRxPerfDequeue(struct ADAPTER *prAdapter)
{
	struct MBRAIN_TRXPERF_ENTRY *prTRxPerfEntry = NULL;

	KAL_SPIN_LOCK_DECLARATION();
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TRXPERF);
	if (QUEUE_IS_NOT_EMPTY(&prAdapter->rMbrTRxPerfQueue))
		QUEUE_REMOVE_HEAD(&prAdapter->rMbrTRxPerfQueue,
			prTRxPerfEntry, struct MBRAIN_TRXPERF_ENTRY *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_MBR_TRXPERF);

	return prTRxPerfEntry;
}

enum wifi2mbr_status mbrWifiTRxPerfHandler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len)
{
	enum wifi2mbr_status status = WIFI2MBR_FAILURE;
	struct wifi2mbr_TRxPerfInfo *prPerfInfoDest =
		(struct wifi2mbr_TRxPerfInfo *) buf;
	struct MBRAIN_TRXPERF_ENTRY *prTRxPerfEntry = NULL;
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

	if (prAdapter->rMbrTRxPerfQueue.u4NumElem == 0) {
		DBGLOG(REQ, DEBUG, "TxTimeout Queue is empty\n");
		return WIFI2MBR_END;
	}

	prTRxPerfEntry = mbrTRxPerfDequeue(prAdapter);
	if (!prTRxPerfEntry)
		return WIFI2MBR_END;

	kalMemCopy(prPerfInfoDest, &prTRxPerfEntry->rTRxPerfInfo,
		sizeof(struct wifi2mbr_TRxPerfInfo));
	*pu2Len = sizeof(*prPerfInfoDest);

	kalMemFree(prTRxPerfEntry, VIR_MEM_TYPE,
			sizeof(struct MBRAIN_TRXPERF_ENTRY));

	status = WIFI2MBR_SUCCESS;

	return status;
}

uint16_t mbrWifiTRxPerfGetTotalDataNum(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo || prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

	return prAdapter->rMbrTRxPerfQueue.u4NumElem;
}

void mbrWifiTRxPerfFlushQueue(struct ADAPTER *prAdapter)
{
	struct MBRAIN_TRXPERF_ENTRY *prTRxPerfEntry = NULL;

	do {
		prTRxPerfEntry = mbrTRxPerfDequeue(prAdapter);
		if (prTRxPerfEntry) {
			kalMemFree(prTRxPerfEntry, VIR_MEM_TYPE,
				sizeof(struct MBRAIN_TRXPERF_ENTRY));
		} else {
			break;
		}
	} while (TRUE);
}
#endif /* CFG_SUPPORT_MBRAIN_TRX_PERF */
#endif /* CFG_SUPPORT_MBRAIN */
