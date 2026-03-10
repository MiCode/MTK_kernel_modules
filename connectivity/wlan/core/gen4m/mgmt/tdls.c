/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
 * Id: tdls.c#1
 */

/*! \file tdls.c
 *    \brief This file includes IEEE802.11z TDLS support.
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

#if CFG_SUPPORT_TDLS
#include "tdls.h"
#include "queue.h"

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
uint8_t g_arTdlsLink[MAXNUM_TDLS_PEER] = {
		0,
		0,
		0,
		0
	};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static u_int8_t fgIsPtiTimeoutSkip = FALSE;
static u_int8_t fgIsWaitForTxDone = FALSE;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define ELEM_ID_LINK_IDENTIFIER_LENGTH 16

#define	TDLS_KEY_TIMEOUT_INTERVAL 43200

#define TDLS_REASON_CODE_UNREACHABLE  25
#define TDLS_REASON_CODE_UNSPECIFIED  26

#define WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE 25
#define WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED 26

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint8_t TdlsAllowedChannel(
	struct ADAPTER *pAd,
	struct BSS_INFO *bss)
{
	uint8_t fgAvailable = TRUE;

		/* Check VLP */
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (bss->eBand == BAND_6G &&
		bss->ucPrimaryChannel >= 97) {
		DBGLOG(TDLS, TRACE,
			"VLP channel (%d)\n",
			bss->ucPrimaryChannel);
		fgAvailable = FALSE;
	}
#endif

	return fgAvailable;
}

#if CFG_SUPPORT_TDLS_P2P_AUTO
uint8_t TdlsCheckSetup(
	struct ADAPTER *ad,
	struct sta_tdls_info *sta)
{
	uint32_t f = 0;

	if (!ad || !sta)
		return FALSE;

	f = ad->rWifiVar.u4TdlsP2pAuto;

	if (IS_FEATURE_DISABLED(f))
		return FALSE;
	else if (IS_FEATURE_FORCE_ENABLED(f))
		return (sta &&
			sta->u4Throughput >
			TDLS_SETUP_LOW_THD);
	else
		return (sta &&
			sta->u4Throughput >
			TDLS_SETUP_THD);
}

uint8_t TdlsCheckTeardown(
	struct ADAPTER *ad,
	struct sta_tdls_info *sta)
{
	uint32_t f = 0;

	if (!ad || !sta)
		return FALSE;

	f = ad->rWifiVar.u4TdlsP2pAuto;

	if (IS_FEATURE_DISABLED(f))
		return FALSE;
	else if (IS_FEATURE_FORCE_ENABLED(f))
		return (sta &&
			sta->u4Throughput <
			TDLS_TEARDOWN_LOW_THD);
	else
		return (sta &&
			sta->u4Throughput <
			TDLS_TEARDOWN_THD);
}

void
TdlsApStaForEach(struct ADAPTER *pAd,
	uint8_t bss,
	enum STA_TDLS_OP eOp,
	void *prArg,
	void **pprOut)
{
	int i;
	struct sta_tdls_info *p, *r;
	int i4Max_tp = 0;
	struct BSS_INFO *b =
		GET_BSS_INFO_BY_INDEX(
		pAd, bss);

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(pAd, SPIN_LOCK_STA_REC);

	for (i = 0; i < STA_TDLS_HASH_SIZE; i++) {
		r = b->prTdlsHash[i];
		while (r) {
			p = r;
			r = r->pNext;
			switch (eOp) {
			case STA_TDLS_OP_FREE:
#if CFG_SUPPORT_TDLS_LOG
				DBGLOG(TDLS, TRACE,
					"free sta %pM\n", p->aucAddr);
#endif
				kalMemFree(p,
					VIR_MEM_TYPE,
					sizeof(struct sta_tdls_info));
				if (!r)
					b->prTdlsHash[i] = NULL;
				break;
			case STA_TDLS_OP_RESET:
#if CFG_SUPPORT_TDLS_LOG
				DBGLOG(TDLS, TRACE,
					"Reset default\n");
#endif
				p->ulTxBytes = 0;
				p->ulRxBytes = 0;
				p->u4Throughput = 0;
				b->ulLastUpdate =
					kalGetJiffies();
				break;
			case STA_TDLS_OP_GET_MAX_TP:
				p->u4Throughput =
					(p->ulTxBytes * HZ) /
					SAMPLING_UT;
#if CFG_SUPPORT_TDLS_LOG
				DBGLOG(TDLS, TRACE,
					"STA: %pM, TP: %d Bytes/s\n",
					p->aucAddr, p->u4Throughput);
#endif
				/*
				 * exclude:
				 * BSSID, as we are direct link with BSSID
				 * broadcast / multicast
				 */
				if (p->u4Throughput >= i4Max_tp &&
				    kalMemCmp(p->aucAddr,
				    prArg, ETH_ALEN) &&
				    IS_UCAST_MAC_ADDR(p->aucAddr)) {
					DBGLOG(TDLS, TRACE,
						MACSTR", MaxTP: %d Bytes/s\n",
						p->aucAddr,
						p->u4Throughput);
					*pprOut = p;
					i4Max_tp = p->u4Throughput;
				}
				break;
			case STA_TDLS_OP_UPDATE_TX:
			default:
				break;
			}
		}
	}

	KAL_RELEASE_SPIN_LOCK(pAd, SPIN_LOCK_STA_REC);
}

uint8_t TdlsAllowedP2p(
	struct ADAPTER *pAd,
	uint8_t bss)
{
	uint8_t fgAvailable = TRUE;

	if (!TdlsValid(pAd, bss))
		fgAvailable = FALSE;
	else {
		struct BSS_INFO *b =
			GET_BSS_INFO_BY_INDEX(
			pAd, bss);

#if CFG_SUPPORT_TDLS_LOG
		DBGLOG(TDLS, TRACE,
			"STA ch: %d, band: %d, conn state: %d",
			b->ucPrimaryChannel,
			b->eBand,
			b->eConnectionState);
#endif

		fgAvailable =
			IS_BSS_GC(b) &&
			TdlsAllowedChannel(pAd, b);
	}

	return fgAvailable;
}

void TdlsStateTimer(
	struct ADAPTER *ad,
	uintptr_t ulParamPtr)
{
	uint8_t bss = (uint8_t) ulParamPtr;
	struct sta_tdls_info *sta;
	struct BSS_INFO *b;

	if (!ad)
		return;

	b = GET_BSS_INFO_BY_INDEX(ad, bss);
	if (!b)
		return;

	sta = b->prTdlsHash[STA_TDLS_HASH_SIZE];
	if (!sta) {
		DBGLOG(TDLS, INFO, "TDLS: No target station, return\n");
		return;
	}

	if (sta->eTdlsRole == STA_TDLS_ROLE_RESPONDER) {
		sta->u4Throughput = (sta->ulRxBytes * HZ) / TDLS_MONITOR_UT;

		if (sta->u4Throughput < TDLS_TEARDOWN_RX_THD) {
			switch (sta->eTdlsStatus) {
			case STA_TDLS_LINK_ENABLE:
				TdlsAutoTeardown(
					ad,
					bss,
					sta,
					"Low RX Throughput");
				/* fallthrough */
			default:
				return;
			}
		}
		DBGLOG(TDLS, INFO, "TDLS: Rx data stream OK\n");
		sta->ulRxBytes = 0;
		goto start_timer;
	}

	switch (sta->eTdlsStatus) {
	case STA_TDLS_NOT_SETUP:
		DBGLOG(TDLS, INFO, "Last TDLS monitor timer\n");
		return;
	case STA_TDLS_SETUP_INPROCESS:
		DBGLOG(TDLS, INFO, "TDLS: setup timeout\n");
		if (sta->u4SetupFailCount++ > TDLS_SETUP_COUNT)
			TdlsAutoTeardown(
				ad,
				bss,
				sta,
				"Setup Failed");
		break;
	case STA_TDLS_LINK_ENABLE:
		/* go through as we need restart timer to monitor tdls link */
	default:
		if (TIME_AFTER(
			kalGetJiffies(),
			b->ulLastUpdate + 2 * SAMPLING_UT)) {
			TdlsAutoTeardown(
				ad,
				bss,
				sta,
				"No Traffic");
			return;
		}
		DBGLOG(TDLS, TRACE, "TDLS: TX data stream OK\n");
		break;
	}

start_timer:

	DBGLOG(TDLS, TRACE, "Restart tdls monitor timer\n");

	cnmTimerStartTimer(ad,
		&(b->rTdlsStateTimer),
		SEC_TO_MSEC(TDLS_MONITOR_UT));

}

uint32_t TdlsAutoSetup(
	struct ADAPTER *ad,
	uint8_t bss,
	struct sta_tdls_info *sta)
{
	struct STA_RECORD s;
	struct BSS_INFO *b;

	if (!ad || !sta)
		return TDLS_STATUS_FAIL;

	b = GET_BSS_INFO_BY_INDEX(
		ad, bss);

	if (!b)
		return TDLS_STATUS_FAIL;

	DBGLOG(TDLS, INFO,
		"[%d] Build up "MACSTR", %d\n",
		bss,
		sta->aucAddr,
		sta->u4Throughput);

	s.ucBssIndex = bss;
	COPY_MAC_ADDR(s.aucMacAddr,
		sta->aucAddr);

	kalTdlsOpReq(
		ad->prGlueInfo,
		&s,
		(uint16_t) TDLS_SETUP,
		0);

	sta->eTdlsStatus = STA_TDLS_SETUP_INPROCESS;
	sta->eTdlsRole = STA_TDLS_ROLE_INITOR;

	b->prTdlsHash[STA_TDLS_HASH_SIZE] = sta;

	cnmTimerInitTimer(ad,
		&(b->rTdlsStateTimer),
		(PFN_MGMT_TIMEOUT_FUNC) TdlsStateTimer,
		(uintptr_t) bss);

	cnmTimerStartTimer(ad,
		&(b->rTdlsStateTimer),
		SEC_TO_MSEC(TDLS_SETUP_TIMEOUT));

	return TDLS_STATUS_SUCCESS;
}

uint32_t TdlsAutoTeardown(
	struct ADAPTER *ad,
	uint8_t bss,
	struct sta_tdls_info *sta,
	uint8_t *reason)
{
	struct STA_RECORD *s;
	struct BSS_INFO *b;

	if (!ad)
		return TDLS_STATUS_FAIL;

	b = GET_BSS_INFO_BY_INDEX(
		ad, bss);

	if (!b)
		return TDLS_STATUS_FAIL;

	if (sta)
		DBGLOG(TDLS, INFO,
			"[%d] Teardown "MACSTR" due to %s, %d\n",
			bss,
			sta->aucAddr,
			reason,
			sta->u4Throughput);
	else
		DBGLOG(TDLS, INFO,
			"[%d] Teardown due to %s\n",
			bss,
			reason);

	if (kalStrCmp(reason, "Disable Link") &&
		sta) {
		s = cnmGetStaRecByAddress(ad,
			bss,
			sta->aucAddr);

		kalTdlsOpReq(
			ad->prGlueInfo,
			s,
			(uint16_t) TDLS_TEARDOWN,
			0);
	}

	b->prTdlsHash[STA_TDLS_HASH_SIZE] = NULL;
	b->i4TdlsLastRx = -1;
	TdlsApStaForEach(ad,
		bss,
		STA_TDLS_OP_FREE,
		NULL,
		NULL);
	cnmTimerStopTimer(ad, &(b->rTdlsStateTimer));

	return TDLS_STATUS_SUCCESS;
}

struct sta_tdls_info *
TdlsGetSta(
	struct ADAPTER *pAd,
	uint8_t bss,
	const u8 *prSta)
{
	struct sta_tdls_info *s;
	struct BSS_INFO *b =
		GET_BSS_INFO_BY_INDEX(
		pAd, bss);

	if (!b)
		return NULL;

	s = b->prTdlsHash[STA_TDLS_HASH(prSta)];

	while (s &&
		kalMemCmp(s->aucAddr,
		prSta,
		TDLS_FME_MAC_ADDR_LEN))
		s = s->pNext;

	return s;
}

void
TdlsHashAdd(
	struct ADAPTER *pAd,
	uint8_t bss,
	struct sta_tdls_info *prSta)
{
	struct BSS_INFO *b =
		GET_BSS_INFO_BY_INDEX(
		pAd, bss);

	if (!b)
		return;

	prSta->pNext =
		b->prTdlsHash[STA_TDLS_HASH(prSta->aucAddr)];
	b->prTdlsHash[STA_TDLS_HASH(prSta->aucAddr)] =
		prSta;
}

struct sta_tdls_info *
TdlsStaAdd(struct ADAPTER *pAd,
	uint8_t bss,
	uint8_t *prAddr)
{
	struct sta_tdls_info *prSta;

	prSta = (struct sta_tdls_info *)
		kalMemZAlloc(
		sizeof(struct sta_tdls_info),
		VIR_MEM_TYPE);
	if (prSta == NULL) {
		DBGLOG(TDLS, WARN, "Alloc ksta failed\n");
		return NULL;
	}

	/* initialize STA info data */
	COPY_MAC_ADDR(prSta->aucAddr, prAddr);
	TdlsHashAdd(pAd, bss, prSta);

	return prSta;
}

void
TdlsAutoSetupTarget(
	struct ADAPTER *pAd,
	uint8_t bss,
	uint8_t *prAddr,
	char *prReason)
{
	struct sta_tdls_info *prTdlsPeer;
	struct BSS_INFO *b =
		GET_BSS_INFO_BY_INDEX(
		pAd, bss);

	if (!b)
		return;

	prTdlsPeer = TdlsGetSta(pAd, bss, prAddr);

	if (!prTdlsPeer)
		prTdlsPeer = TdlsStaAdd(pAd, bss, prAddr);
	if (!prTdlsPeer)
		return;

	DBGLOG(TDLS, INFO,
		"Create TDLS peer["MACSTR"] reason %s\n",
		prTdlsPeer->aucAddr, prReason);

	b->prTdlsHash[STA_TDLS_HASH_SIZE] = prTdlsPeer;
	prTdlsPeer->eTdlsStatus = STA_TDLS_LINK_ENABLE;
}

void
TdlsUpdateTxRxStat(
	struct ADAPTER *pAd,
	uint8_t bss,
	uint64_t tx_bytes,
	uint64_t rx_bytes,
	uint8_t *prAddr)
{
	struct sta_tdls_info *sta;

	KAL_SPIN_LOCK_DECLARATION();

	if (!pAd->rWifiVar.u4TdlsP2pAuto)
		return;

	KAL_ACQUIRE_SPIN_LOCK(pAd, SPIN_LOCK_STA_REC);

	sta = TdlsGetSta(pAd, bss, prAddr);

	if (!sta)
		sta = TdlsStaAdd(pAd, bss, prAddr);
	if (!sta) {
		KAL_RELEASE_SPIN_LOCK(pAd, SPIN_LOCK_STA_REC);
		DBGLOG(TDLS, WARN, "Add sta info failed\n");
		return;
	}

	if (tx_bytes)
		sta->ulTxBytes += tx_bytes;
	else
		sta->ulRxBytes += rx_bytes;

	KAL_RELEASE_SPIN_LOCK(pAd, SPIN_LOCK_STA_REC);

#if CFG_SUPPORT_TDLS_LOG
	DBGLOG(TDLS, INFO,
		"sta["MACSTR"] %s bytes: %ld\n",
		prAddr,
		tx_bytes ? "Tx" : "Rx",
		tx_bytes ? tx_bytes : rx_bytes);
#endif
}

int32_t TdlsP2pAuto(
	struct ADAPTER *pAd,
	uint8_t bss,
	uint64_t tx_bytes,
	uint64_t rx_bytes,
	uint8_t *prAddr)
{
	uint32_t u4PacketLen = tx_bytes;
	uint8_t *pucData = prAddr;
	struct BSS_INFO *b = NULL;
	struct sta_tdls_info *target_sta = NULL;

	if (!pAd ||
		!pAd->rWifiVar.u4TdlsP2pAuto ||
		!TdlsAllowedP2p(pAd, bss) ||
		(u4PacketLen < ETH_HLEN))
		return -1;

	b = GET_BSS_INFO_BY_INDEX(
		pAd, bss);
	if (!b)
		return -1;

	if (TIME_BEFORE(kalGetJiffies(),
		b->ulLastUpdate + SAMPLING_UT)) {
		TdlsUpdateTxRxStat(
			pAd,
			bss,
			u4PacketLen,
			0,
			pucData);
		return 0;
	}

	if (!b->prTdlsHash[STA_TDLS_HASH_SIZE]) {
		TdlsApStaForEach(pAd, bss,
			STA_TDLS_OP_GET_MAX_TP,
			b->aucBSSID,
			(void **)&target_sta);

		if (TdlsCheckSetup(pAd, target_sta)) {
			switch (target_sta->eTdlsStatus) {
			case STA_TDLS_NOT_SETUP:
				TdlsAutoSetup(pAd, bss, target_sta);
				return 1;
			case STA_TDLS_SETUP_INPROCESS:
#if CFG_SUPPORT_TDLS_LOG
				DBGLOG(TDLS, INFO,
					"TDLS setup in Process\n");
#endif
				return 2;
			default:
#if CFG_SUPPORT_TDLS_LOG
				DBGLOG(TDLS, INFO,
					"TDLS setup state %d\n",
					target_sta->eTdlsStatus);
#endif
				return 3;
			}
		}
		/* update one shot to avoid no sta available in sta list */
		TdlsUpdateTxRxStat(pAd,
			bss, u4PacketLen, 0, pucData);
		goto reset;
	}

	/* already has a tdls link */
	target_sta = b->prTdlsHash[STA_TDLS_HASH_SIZE];

	if (target_sta->eTdlsRole !=
		STA_TDLS_ROLE_INITOR)
		return 6;

	target_sta->u4Throughput =
		(target_sta->ulTxBytes * HZ) / SAMPLING_UT;

	if (TdlsCheckTeardown(pAd, target_sta)) {
		switch (target_sta->eTdlsStatus) {
		case STA_TDLS_LINK_ENABLE:
			TdlsAutoTeardown(pAd,
				bss,
				target_sta,
				"Low Tx Throughput");
			/* fallthrough */
		default:
			return 4;
		}
	}

reset:

	TdlsApStaForEach(
		pAd,
		bss,
		STA_TDLS_OP_RESET,
		NULL,
		NULL);

	return 5;
}
#endif

uint8_t TdlsEnabled(struct ADAPTER *pAd)
{
	uint8_t fgEnabled = TRUE;

	if (pAd->rWifiVar.fgTdlsDisable) {
		DBGLOG(TDLS, INFO, "TDLS is disabled\n");
		fgEnabled = FALSE;
	}

	return fgEnabled;
}

uint8_t TdlsValid(
	struct ADAPTER *pAd,
	uint8_t ucBssIndex)
{
	uint8_t fgValid = TRUE;
	struct BSS_INFO *bss =
		GET_BSS_INFO_BY_INDEX(
		pAd, ucBssIndex);

	if (!pAd || !bss) {
#if CFG_SUPPORT_TDLS_LOG
		DBGLOG(TDLS, TRACE, "bss is not active\n");
#endif
		fgValid = FALSE;
	} else if (bss->eCurrentOPMode !=
		OP_MODE_INFRASTRUCTURE) {
#if CFG_SUPPORT_TDLS_LOG
		DBGLOG(TDLS, TRACE, "bss is not client\n");
#endif
		fgValid = FALSE;
	} else if (
		bss->eConnectionState !=
		MEDIA_STATE_CONNECTED) {
#if CFG_SUPPORT_TDLS_LOG
		DBGLOG(TDLS, TRACE, "bss is not connected\n");
#endif
		fgValid = FALSE;
	}

	return fgValid;
}

uint8_t TdlsAllowed(
	struct ADAPTER *pAd,
	uint8_t ucBssIndex)
{
	uint8_t fgAvailable = TRUE;

	if (!TdlsValid(pAd, ucBssIndex))
		fgAvailable = FALSE;
	else {
		struct BSS_INFO *bss =
			GET_BSS_INFO_BY_INDEX(
			pAd, ucBssIndex);

#if CFG_SUPPORT_TDLS_LOG
		DBGLOG(TDLS, TRACE,
			"STA ch: %d, band: %d, conn state: %d",
			bss->ucPrimaryChannel,
			bss->eBand,
			bss->eConnectionState);
#endif

		fgAvailable =
			TdlsAllowedChannel(pAd, bss);
	}

	return fgAvailable;
}

uint8_t TdlsNeedAdjustBw(
	struct ADAPTER *pAd,
	uint8_t ucBssIndex)
{
	uint8_t fgEnable = FALSE;

	if (!TdlsValid(pAd, ucBssIndex))
		fgEnable = FALSE;
	else {
		struct BSS_INFO *bss =
			GET_BSS_INFO_BY_INDEX(
			pAd, ucBssIndex);

#if CFG_SUPPORT_TDLS_LOG
		DBGLOG(TDLS, TRACE,
			"STA ch: %d, band: %d, conn state: %d",
			bss->ucPrimaryChannel,
			bss->eBand,
			bss->eConnectionState);
#endif

		if (rlmDomainIsLegalDfsChannel(pAd,
			bss->eBand, bss->ucPrimaryChannel)) {
			fgEnable = TRUE;
		}
	}

	return fgEnable;
}

uint8_t TdlsAdjustBw(
	struct ADAPTER *pAd,
	struct STA_RECORD *sta,
	uint8_t bss,
	uint8_t bw)
{
	if (TdlsValid(pAd, bss) &&
		TdlsNeedAdjustBw(pAd, bss) &&
		sta &&
		IS_DLS_STA(sta)) {
		uint8_t newbw =
			rlmGetBssOpBwByVhtAndHtOpInfo(
			GET_BSS_INFO_BY_INDEX(pAd,
			bss));

		DBGLOG(TDLS, INFO,
			"Adjust bw %d to %d\n",
			bw,
			newbw);
		return newbw;
	}

	return bw;
}

#if CFG_SUPPORT_TDLS_11AX
uint16_t _TdlsComposeCapIE(
	struct ADAPTER *ad,
	struct BSS_INFO *bss,
	struct STA_RECORD *sta,
	uint8_t *buf)
{
	struct MSDU_INFO *prMsduInfo;
	uint16_t u2EstimatedFrameLen;
	uint16_t u2EstimatedExtraIELen;
	uint16_t u2FrameLength;

	if (!ad || !bss || !sta || !buf) {
		DBGLOG(TDLS, ERROR, "ad is NULL!\n");
		return 0;
	}

	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD +
	    WLAN_MAC_MGMT_HEADER_LEN +
	    CAP_INFO_FIELD_LEN +
	    STATUS_CODE_FIELD_LEN +
	    AID_FIELD_LEN +
	    (ELEM_HDR_LEN + ELEM_MAX_LEN_SUP_RATES) +
	    (ELEM_HDR_LEN + (RATE_NUM_SW - ELEM_MAX_LEN_SUP_RATES)) +
	    sizeof(uint64_t); /* reserved for cookie */

	/* + Extra IE Length */
	u2EstimatedExtraIELen = 0;

#if CFG_SUPPORT_TDLS_11AX
	if (RLM_NET_IS_11AX(bss))
		u2EstimatedExtraIELen +=
			heRlmCalculateHeCapIELen(
			ad,
			bss->ucBssIndex,
			sta);
#endif
#if CFG_SUPPORT_TDLS_11BE
	if (RLM_NET_IS_11BE(bss))
		u2EstimatedExtraIELen +=
			ehtRlmCalculateCapIELen(
			ad,
			bss->ucBssIndex,
			sta);
#endif

	if (u2EstimatedExtraIELen == 0)
		return 0;

	u2EstimatedFrameLen += u2EstimatedExtraIELen;

	prMsduInfo = cnmMgtPktAlloc(ad, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(TDLS, WARN, "No PKT_INFO_T.\n");
		return 0;
	}

	/* Append IE */
#if CFG_SUPPORT_TDLS_11AX
	if (RLM_NET_IS_11AX(bss))
		heRlmFillHeCapIE(
			ad,
			bss,
			prMsduInfo);
#endif
#if CFG_SUPPORT_TDLS_11BE
	if (RLM_NET_IS_11BE(bss))
		ehtRlmFillCapIE(
			ad,
			bss,
			prMsduInfo);
#endif

	DBGLOG(TDLS, TRACE, "Dump cap ie\n");

	if (aucDebugModule[DBG_TDLS_IDX] & DBG_CLASS_TRACE) {
		dumpMemory8((uint8_t *) prMsduInfo->prPacket,
			(uint32_t) prMsduInfo->u2FrameLength);
	}

	kalMemCopy(buf,
		prMsduInfo->prPacket,
		prMsduInfo->u2FrameLength);

	u2FrameLength = prMsduInfo->u2FrameLength;

	cnmMgtPktFree(ad, prMsduInfo);

	return u2FrameLength;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to hadel TDLS link oper from nl80211.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t TdlsexLinkMgt(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen)
{
	uint32_t rResult = TDLS_STATUS_SUCCESS;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	struct TDLS_CMD_LINK_MGT *prCmd;

	prCmd = (struct TDLS_CMD_LINK_MGT *) pvSetBuffer;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prCmd->ucBssIdx);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prCmd->ucBssIdx);
		return -EINVAL;
	}

	DBGLOG(TDLS, INFO, "u4SetBufferLen=%d", u4SetBufferLen);

#if 1
	/* AIS only */
	if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
		prStaRec = prBssInfo->prStaRecOfAP;
		if (prStaRec == NULL)
			return 0;
#if CFG_SUPPORT_TDLS_11AX
		if (!TdlsAllowed(prAdapter, prCmd->ucBssIdx))
			return 0;
#endif
	} else {
		return -EINVAL;
	}
#endif

	DBGLOG(TDLS, INFO, "prCmd->ucActionCode=%d, prCmd->ucDialogToken=%d",
		prCmd->ucActionCode, prCmd->ucDialogToken);

	prStaRec = prBssInfo->prStaRecOfAP;

	switch (prCmd->ucActionCode) {

	case TDLS_FRM_ACTION_DISCOVERY_REQ:
		if (prStaRec == NULL)
			return 0;
		rResult = TdlsDataFrameSend_DISCOVERY_REQ(prAdapter,
					    prStaRec,
					    prCmd->aucPeer,
					    prCmd->ucActionCode,
					    prCmd->ucDialogToken,
					    prCmd->u2StatusCode,
					    (uint8_t *) (prCmd->aucSecBuf),
					    prCmd->u4SecBufLen);
		break;

	case TDLS_FRM_ACTION_SETUP_REQ:
		if (prStaRec == NULL)
			return 0;
		prStaRec = cnmGetTdlsPeerByAddress(prAdapter,
				prBssInfo->ucBssIndex,
				prCmd->aucPeer);
		g_arTdlsLink[prStaRec->ucTdlsIndex] = 0;
		rResult = TdlsDataFrameSend_SETUP_REQ(prAdapter,
					prStaRec,
					prCmd->aucPeer,
					prCmd->ucActionCode,
					prCmd->ucDialogToken,
					prCmd->u2StatusCode,
					(uint8_t *) (prCmd->aucSecBuf),
					prCmd->u4SecBufLen);
		break;

	case TDLS_FRM_ACTION_SETUP_RSP:
		/* fix sigma bug 5.2.4.2, 5.2.4.7, we sent Status code decline,
		 * but the sigma recogniezis it as scucess, and it will fail
		 */
		/* if(prCmd->u2StatusCode != 0) */
		if (prBssInfo->fgTdlsIsProhibited)
			return 0;

		rResult = TdlsDataFrameSend_SETUP_RSP(prAdapter,
					prStaRec,
					prCmd->aucPeer,
					prCmd->ucActionCode,
					prCmd->ucDialogToken,
					prCmd->u2StatusCode,
					(uint8_t *) (prCmd->aucSecBuf),
					prCmd->u4SecBufLen);
		break;

	case TDLS_FRM_ACTION_DISCOVERY_RSP:
		rResult = TdlsDataFrameSend_DISCOVERY_RSP(prAdapter,
					prStaRec,
					prCmd->aucPeer,
					prCmd->ucActionCode,
					prCmd->ucDialogToken,
					prCmd->u2StatusCode,
					(uint8_t *) (prCmd->aucSecBuf),
					prCmd->u4SecBufLen);
		break;

	case TDLS_FRM_ACTION_CONFIRM:
		rResult = TdlsDataFrameSend_CONFIRM(prAdapter,
					prStaRec,
					prCmd->aucPeer,
					prCmd->ucActionCode,
					prCmd->ucDialogToken,
					prCmd->u2StatusCode,
					(uint8_t *) (prCmd->aucSecBuf),
					prCmd->u4SecBufLen);
		break;

	case TDLS_FRM_ACTION_TEARDOWN:
		prStaRec = cnmGetTdlsPeerByAddress(prAdapter,
				prBssInfo->ucBssIndex,
				prCmd->aucPeer);
		if (prCmd->u2StatusCode == TDLS_REASON_CODE_UNREACHABLE)
			g_arTdlsLink[prStaRec->ucTdlsIndex] = 0;

		rResult = TdlsDataFrameSend_TearDown(prAdapter,
					prStaRec,
					prCmd->aucPeer,
					prCmd->ucActionCode,
					prCmd->ucDialogToken,
					prCmd->u2StatusCode,
					(uint8_t *) (prCmd->aucSecBuf),
					prCmd->u4SecBufLen);
		break;

	default:
		DBGLOG(TDLS, INFO, "default=%d", prCmd->ucActionCode);
		return -EINVAL;
	}

	if (rResult == TDLS_STATUS_PENDING)
		fgIsWaitForTxDone = TRUE;

	DBGLOG(TDLS, INFO, "rResult=%d", rResult);

	return rResult;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to hadel TDLS link mgt from nl80211.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t TdlsexLinkOper(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen)
{
	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */
	uint16_t i;
	struct STA_RECORD *prStaRec;

	struct TDLS_CMD_LINK_OPER *prCmd;

	struct BSS_INFO *prBssInfo;

	prCmd = (struct TDLS_CMD_LINK_OPER *) pvSetBuffer;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prCmd->ucBssIdx);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prCmd->ucBssIdx);
		return 0;
	}

	DBGLOG(TDLS, INFO, "prCmd->oper=%d, u4SetBufferLen=%d",
		prCmd->oper, u4SetBufferLen);

	switch (prCmd->oper) {

	case TDLS_ENABLE_LINK:

		for (i = 0; i < MAXNUM_TDLS_PEER; i++) {
			if (!g_arTdlsLink[i]) {
				g_arTdlsLink[i] = 1;
				prStaRec =
				cnmGetTdlsPeerByAddress(prAdapter,
					prBssInfo->ucBssIndex,
					prCmd->aucPeerMac);
				prStaRec->ucTdlsIndex = i;
#if CFG_SUPPORT_TDLS_P2P_AUTO
				TdlsAutoSetupTarget(
					prAdapter,
					prBssInfo->ucBssIndex,
					prCmd->aucPeerMac,
					"Enable Link");
#endif
				break;
			}
		}

		break;
	case TDLS_DISABLE_LINK:

		prStaRec = cnmGetTdlsPeerByAddress(prAdapter,
				prBssInfo->ucBssIndex,
				prCmd->aucPeerMac);

		g_arTdlsLink[prStaRec->ucTdlsIndex] = 0;
		if (IS_DLS_STA(prStaRec))
			cnmStaRecFree(prAdapter, prStaRec);
#if CFG_SUPPORT_TDLS_P2P_AUTO
		TdlsAutoTeardown(prAdapter,
			prBssInfo->ucBssIndex,
			NULL,
			"Disable Link");
#endif
		break;
	default:
		return 0;
	}

	/* count total TDLS link */
	prAdapter->u4TdlsLinkCount = 0;
	for (i = 0; i < MAXNUM_TDLS_PEER; i++)
		prAdapter->u4TdlsLinkCount += g_arTdlsLink[i];
	DBGLOG(TDLS, INFO, "TDLS total link = %d", prAdapter->u4TdlsLinkCount);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to append general IEs.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 *
 * \retval append length
 */
/*----------------------------------------------------------------------------*/
uint32_t TdlsFrameGeneralIeAppend(struct ADAPTER *prAdapter,
				  struct STA_RECORD *prStaRec, uint8_t *pPkt)
{
	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	uint16_t u2SupportedRateSet;
	uint8_t aucAllSupportedRates[RATE_NUM_SW] = { 0 };
	uint8_t ucAllSupportedRatesLen;
	uint8_t ucSupRatesLen;
	uint8_t ucExtSupRatesLen;
	uint32_t u4PktLen, u4IeLen;

	/* init */
	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prStaRec->ucBssIndex);
		return 0;
	}

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;

	/* 3. Frame Formation - (5) Supported Rates element */
	/* use all sup rate we can support */
	u2SupportedRateSet = prStaRec->u2OperationalRateSet;
	rateGetDataRatesFromRateSet(u2SupportedRateSet, 0,
				    aucAllSupportedRates,
				    &ucAllSupportedRatesLen);

	ucSupRatesLen = ((ucAllSupportedRatesLen >
			  ELEM_MAX_LEN_SUP_RATES) ?
			 ELEM_MAX_LEN_SUP_RATES : ucAllSupportedRatesLen);

	ucExtSupRatesLen = ucAllSupportedRatesLen - ucSupRatesLen;

	if (ucSupRatesLen) {
		SUP_RATES_IE(pPkt)->ucId = ELEM_ID_SUP_RATES;
		SUP_RATES_IE(pPkt)->ucLength = ucSupRatesLen;
		kalMemCopy(SUP_RATES_IE(pPkt)->aucSupportedRates,
			   aucAllSupportedRates, ucSupRatesLen);

		u4IeLen = IE_SIZE(pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}

	/* 3. Frame Formation - (7) Extended sup rates element */
	if (ucExtSupRatesLen) {

		EXT_SUP_RATES_IE(pPkt)->ucId = ELEM_ID_EXTENDED_SUP_RATES;
		EXT_SUP_RATES_IE(pPkt)->ucLength = ucExtSupRatesLen;

		kalMemCopy(EXT_SUP_RATES_IE(pPkt)->aucExtSupportedRates,
			   &aucAllSupportedRates[ucSupRatesLen],
			   ucExtSupRatesLen);

		u4IeLen = IE_SIZE(pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}

	/* 3. Frame Formation - (8) Supported channels element */
	SUPPORTED_CHANNELS_IE(pPkt)->ucId = ELEM_ID_SUP_CHS;
	SUPPORTED_CHANNELS_IE(pPkt)->ucLength = 2;
	SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[0] = 1;
	SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[1] = 13;

	if (prAdapter->fgEnable5GBand == TRUE) {
		SUPPORTED_CHANNELS_IE(pPkt)->ucLength = 10;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[2] = 36;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[3] = 4;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[4] = 52;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[5] = 4;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[6] = 149;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[7] = 4;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[8] = 165;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[9] = 1;
	}

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	return u4PktLen;
}

/*!
 * \brief This routine is called to transmit a TDLS data frame.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
TdlsDataFrameSend_TearDown(struct ADAPTER *prAdapter,
			   struct STA_RECORD *prStaRec,
			   uint8_t *pPeerMac,
			   uint8_t ucActionCode,
			   uint8_t ucDialogToken, uint16_t u2StatusCode,
			   uint8_t *pAppendIe, uint32_t AppendIeLen)
{

	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	void *pvPacket = NULL;
	uint8_t *pPkt = NULL;
	uint32_t u4PktLen, u4IeLen;
	uint16_t ReasonCode;

	/* allocate/init packet */
	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prStaRec->ucBssIndex);
		return 0;
	}

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	pvPacket = kalPacketAllocWithHeadroom(prGlueInfo, 1600, &pPkt);

	if (pvPacket == NULL)
		return TDLS_STATUS_RESOURCES;
	kalSetPacketDev(prGlueInfo, prStaRec->ucBssIndex, pvPacket);
	if (kalGetPacketDev(pvPacket) == NULL) {
		kalPacketFree(prGlueInfo, pvPacket);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr,
		   TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	*(uint16_t *) pPkt = HTONS(TDLS_FRM_PROT_TYPE);
	pPkt += 2;
	u4PktLen += TDLS_FME_MAC_ADDR_LEN * 2 + 2;

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (1) Category */
	*pPkt = TDLS_FRM_CATEGORY;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - status code */

	ReasonCode = u2StatusCode;


	kalMemCopy(pPkt, &ReasonCode, 2);
	pPkt = pPkt + 2;
	u4PktLen = u4PktLen + 2;

	if (pAppendIe != NULL) {
		kalMemCopy(pPkt, pAppendIe, AppendIeLen);
		LR_TDLS_FME_FIELD_FILL(AppendIeLen);
	}

	/* 7. Append Supported Operating Classes IE */
	if (ucActionCode != TDLS_FRM_ACTION_TEARDOWN) {
		/* Note: if we do not put the IE, Marvell STA will
		 *		decline our TDLS setup request
		 */
		u4IeLen = rlmDomainSupOperatingClassIeFill(pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
	}

	/* 3. Frame Formation - (16) Link identifier element */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId =
		ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID,
		   prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator,
		   prBssInfo->aucOwnMacAddr, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder,
		   pPeerMac, 6);

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 4. Update packet length */
	kalSetPacketLength(pvPacket, u4PktLen);

	/* 5. send the data frame */
	kalWlanHardStartXmit(pvPacket, kalGetPacketDev(pvPacket));

	return TDLS_STATUS_PENDING;
}

/*!
 * \brief This routine is called to transmit a TDLS data frame.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t			/* TDLS_STATUS */
TdlsDataFrameSend_SETUP_REQ(struct ADAPTER *prAdapter,
			    struct STA_RECORD *prStaRec,
			    uint8_t *pPeerMac,
			    uint8_t ucActionCode,
			    uint8_t ucDialogToken, uint16_t u2StatusCode,
			    uint8_t *pAppendIe, uint32_t AppendIeLen)
{

	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	void *pvPacket = NULL;
	uint8_t *pPkt = NULL;
	uint32_t u4PktLen, u4IeLen;
	uint16_t u2CapInfo;

	/* allocate/init packet */
	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prStaRec->ucBssIndex);
		return 0;
	}

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	pvPacket = kalPacketAllocWithHeadroom(prGlueInfo, 512, &pPkt);
	if (pvPacket == NULL)
		return TDLS_STATUS_RESOURCES;
	kalSetPacketDev(prGlueInfo, prStaRec->ucBssIndex, pvPacket);
	if (kalGetPacketDev(pvPacket) == NULL) {
		kalPacketFree(prGlueInfo, pvPacket);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	*(uint16_t *) pPkt = HTONS(TDLS_FRM_PROT_TYPE);
	LR_TDLS_FME_FIELD_FILL(2);

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	LR_TDLS_FME_FIELD_FILL(1);

	/*
	 * 3.1 Category
	 * 3.2 Action
	 * 3.3 Dialog Token
	 * 3.4 Capability
	 * 3.5 Supported rates
	 * 3.6 Country
	 * 3.7 Extended supported rates
	 * 3.8 Supported Channels (optional)
	 * 3.9 RSNIE (optional)
	 * 3.10 Extended Capabilities
	 * 3.11 QoS Capability
	 * 3.12 FTIE (optional)
	 * 3.13 Timeout Interval (optional)
	 * 3.14 Supported Regulatory Classes (optional)
	 * 3.15 HT Capabilities
	 * 3.16 20/40 BSS Coexistence
	 * 3.17 Link Identifier
	 */

	/* 3.1 Category */
	*pPkt = TDLS_FRM_CATEGORY;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.2 Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.3 Dialog Token */
	*pPkt = ucDialogToken;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.4 Capability */
	u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
	WLAN_SET_FIELD_16(pPkt, u2CapInfo);
	LR_TDLS_FME_FIELD_FILL(2);

	/*
	 * 3.5 Supported rates
	 * 3.7 Extended supported rates
	 * 3.8 Supported Channels (optional)
	 */
	u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 3.6 Country */
	/* 3.14 Supported Regulatory Classes (optional) */
	/* Note: if we do not put the IE, Marvell STA will decline
	 * our TDLS setup request
	 */
	u4IeLen = rlmDomainSupOperatingClassIeFill(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 3.10 Extended Capabilities */
	EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
	EXT_CAP_IE(pPkt)->ucLength = 5;

	EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00; /* bit0 ~ bit7 */
	EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00; /* bit8 ~ bit15 */
	EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00; /* bit16 ~ bit23 */
	EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00; /* bit24 ~ bit31 */
	EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0x00; /* bit32 ~ bit39 */

	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));
	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
	EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* Append extra IEs */
	/*
	 * 3.9 RSNIE (optional)
	 * 3.12 FTIE (optional)
	 * 3.13 Timeout Interval (optional)
	 */
	if (pAppendIe != NULL) {
		kalMemCopy(pPkt, pAppendIe, AppendIeLen);
		LR_TDLS_FME_FIELD_FILL(AppendIeLen);
	}

	/* 3.11 QoS Capability */
	/* HT WMM IE append */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {
		u4IeLen = mqmGenerateWmmInfoIEByStaRec(prAdapter, prBssInfo,
							prStaRec, pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
		u4IeLen = mqmGenerateWmmParamIEByParam(prAdapter, prBssInfo,
							pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
	}

	/* 3.15 HT Capabilities */
	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet &
		PHY_TYPE_SET_802_11N)) {
		u4IeLen = rlmFillHtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
	}
#if CFG_SUPPORT_802_11AC
	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
		PHY_TYPE_SET_802_11AC) {
		u4IeLen = rlmFillVhtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
	}
#endif
#if (CFG_SUPPORT_TDLS_11AX == 1)
	u4IeLen = _TdlsComposeCapIE(prAdapter,
		prBssInfo, prStaRec, pPkt);
	if (u4IeLen)
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
#endif

	/* 3.16 20/40 BSS Coexistence */
	BSS_20_40_COEXIST_IE(pPkt)->ucId = ELEM_ID_20_40_BSS_COEXISTENCE;
	BSS_20_40_COEXIST_IE(pPkt)->ucLength = 1;
	BSS_20_40_COEXIST_IE(pPkt)->ucData = 0x01;
	LR_TDLS_FME_FIELD_FILL(3);

	/* 3.17 Link Identifier */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID,
		prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator,
		prBssInfo->aucOwnMacAddr, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder,
		pPeerMac, 6);

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 4. Update packet length */
	kalSetPacketLength(pvPacket, u4PktLen);
	DBGLOG(TDLS, INFO, "wlanHardStartXmit, u4PktLen=%d", u4PktLen);

	/* 5. send the data frame */
	kalWlanHardStartXmit(pvPacket, kalGetPacketDev(pvPacket));

	return TDLS_STATUS_PENDING;
}

uint32_t
TdlsDataFrameSend_SETUP_RSP(struct ADAPTER *prAdapter,
			    struct STA_RECORD *prStaRec,
			    uint8_t *pPeerMac,
			    uint8_t ucActionCode,
			    uint8_t ucDialogToken, uint16_t u2StatusCode,
			    uint8_t *pAppendIe, uint32_t AppendIeLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	void *pvPacket = NULL;
	uint8_t *pPkt = NULL;
	uint32_t u4PktLen, u4IeLen;
	uint16_t u2CapInfo;

	/* allocate/init packet */
	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prStaRec->ucBssIndex);
		return 0;
	}
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	pvPacket = kalPacketAllocWithHeadroom(prGlueInfo, 512, &pPkt);
	if (pvPacket == NULL)
		return TDLS_STATUS_RESOURCES;
	kalSetPacketDev(prGlueInfo, prStaRec->ucBssIndex, pvPacket);
	if (kalGetPacketDev(pvPacket) == NULL) {
		kalPacketFree(prGlueInfo, pvPacket);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	*(uint16_t *) pPkt = HTONS(TDLS_FRM_PROT_TYPE);
	LR_TDLS_FME_FIELD_FILL(2);

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	LR_TDLS_FME_FIELD_FILL(1);

	/*
	 * 3.1 Category
	 * 3.2 Action
	 * 3.3 Status Code
	 * 3.4 Dialog Token
	 *** Below IE should be only included for Status Code=0 (Successful)
	 * 3.5 Capability
	 * 3.6 Supported rates
	 * 3.7 Country
	 * 3.8 Extended supported rates
	 * 3.9 Supported Channels (optional)
	 * 3.10 RSNIE (optional)
	 * 3.11 Extended Capabilities
	 * 3.12 QoS Capability
	 * 3.13 FTIE (optional)
	 * 3.14 Timeout Interval (optional)
	 * 3.15 Supported Regulatory Classes (optional)
	 * 3.16 HT Capabilities
	 * 3.17 20/40 BSS Coexistence
	 * 3.18 Link Identifier
	 */

	/* 3.1 Category */
	*pPkt = TDLS_FRM_CATEGORY;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.2 Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.3 Status Code */
	kalMemCopy(pPkt, &u2StatusCode, 2);
	LR_TDLS_FME_FIELD_FILL(2);

	/* 3.4 Dialog Token */
	*pPkt = ucDialogToken;
	LR_TDLS_FME_FIELD_FILL(1);

	if (u2StatusCode == 0) {
		/* 3.5 Capability */
		u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
		WLAN_SET_FIELD_16(pPkt, u2CapInfo);
		LR_TDLS_FME_FIELD_FILL(2);

		/*
		 * 3.6 Supported rates
		 * 3.8 Extended supported rates
		 * 3.9 Supported Channels (optional)
		 */
		u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);

		/* 3.7 Country */
		/* 3.15 Supported Regulatory Classes (optional) */
		/* Note: if we do not put the IE, Marvell STA will decline
		 * our TDLS setup request
		 */
		u4IeLen = rlmDomainSupOperatingClassIeFill(pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);

		/* 3.11 Extended Capabilities */
		EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
		EXT_CAP_IE(pPkt)->ucLength = 5;

		EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00; /* bit0 ~ bit7 */
		EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00; /* bit8 ~ bit15 */
		EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00; /* bit16 ~ bit23 */
		EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00; /* bit24 ~ bit31 */
		EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0x00; /* bit32 ~ bit39 */

		EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));
		EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
		EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));

		u4IeLen = IE_SIZE(pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);

		/* 3.12 QoS Capability */
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {
			/* Add WMM IE *//* try to reuse p2p path */
			u4IeLen = mqmGenerateWmmInfoIEByStaRec(prAdapter,
						prBssInfo, prStaRec, pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);

			u4IeLen = mqmGenerateWmmParamIEByParam(prAdapter,
						prBssInfo, pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
		}

		/* 3.16 HT Capabilities */
		if (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
		     PHY_TYPE_SET_802_11N) {
			u4IeLen = rlmFillHtCapIEByAdapter(prAdapter, prBssInfo,
							  pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
		}
#if CFG_SUPPORT_802_11AC
		if (prAdapter->rWifiVar.ucAvailablePhyTypeSet &
		    PHY_TYPE_SET_802_11AC) {
			u4IeLen = rlmFillVhtCapIEByAdapter(prAdapter, prBssInfo,
							   pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
		}
#endif
#if (CFG_SUPPORT_TDLS_11AX == 1)
		u4IeLen = _TdlsComposeCapIE(prAdapter,
			prBssInfo, prStaRec, pPkt);
		if (u4IeLen)
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
#endif

		/* 3.17 20/40 BSS Coexistence */
		BSS_20_40_COEXIST_IE(pPkt)->ucId =
			ELEM_ID_20_40_BSS_COEXISTENCE;
		BSS_20_40_COEXIST_IE(pPkt)->ucLength = 1;
		BSS_20_40_COEXIST_IE(pPkt)->ucData = 0x01;
		LR_TDLS_FME_FIELD_FILL(3);

		/* 3.18 Link Identifier */
		TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
		TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

		kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID,
			   prBssInfo->aucBSSID, 6);
		kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator,
			   pPeerMac, 6);
		kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder,
			   prBssInfo->aucOwnMacAddr, 6);

		u4IeLen = IE_SIZE(pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);

		/* Append extra IEs */
		/*
		 * 3.10 RSNIE (optional)
		 * 3.13 FTIE (optional)
		 * 3.14 Timeout Interval (optional)
		 */
		if (pAppendIe != NULL) {
			kalMemCopy(pPkt, pAppendIe, AppendIeLen);
			LR_TDLS_FME_FIELD_FILL(AppendIeLen);
		}
	}

	/* 4. Update packet length */
	kalSetPacketLength(pvPacket, u4PktLen);

	/* 5. send the data frame */
	kalWlanHardStartXmit(pvPacket, kalGetPacketDev(pvPacket));

	return TDLS_STATUS_PENDING;
}

uint32_t
TdlsDataFrameSend_CONFIRM(struct ADAPTER *prAdapter,
			  struct STA_RECORD *prStaRec,
			  uint8_t *pPeerMac,
			  uint8_t ucActionCode,
			  uint8_t ucDialogToken, uint16_t u2StatusCode,
			  uint8_t *pAppendIe, uint32_t AppendIeLen)
{

	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	void *pvPacket = NULL;
	uint8_t *pPkt = NULL;
	uint32_t u4PktLen, u4IeLen;

	/* allocate/init packet */
	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prStaRec->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
			, prStaRec->ucBssIndex);
		return 0;
	}

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	pvPacket = kalPacketAllocWithHeadroom(prGlueInfo, 512, &pPkt);
	if (pvPacket == NULL)
		return TDLS_STATUS_RESOURCES;
	kalSetPacketDev(prGlueInfo, prStaRec->ucBssIndex, pvPacket);
	if (kalGetPacketDev(pvPacket) == NULL) {
		kalPacketFree(prGlueInfo, pvPacket);
		return TDLS_STATUS_FAIL;
	}
	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	*(uint16_t *) pPkt = HTONS(TDLS_FRM_PROT_TYPE);
	LR_TDLS_FME_FIELD_FILL(2);

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	LR_TDLS_FME_FIELD_FILL(1);

	/*
	 * 3.1 Category
	 * 3.2 Action
	 * 3.3 Status Code
	 * 3.4 Dialog Token
	 *** 3.5 - 3.9 should be only included for Status Code=0 (Successful)
	 * 3.5 RSNIE (optional)
	 * 3.6 EDCA Parameter Set
	 * 3.7 FTIE (optional)
	 * 3.8 Timeout Interval (optional)
	 * 3.9 HT Operation (optional)
	 * 3.10 Link Identifier
	 */

	/* 3.1 Category */
	*pPkt = TDLS_FRM_CATEGORY;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.2 Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.3 Status Code */
	kalMemCopy(pPkt, &u2StatusCode, 2);
	LR_TDLS_FME_FIELD_FILL(2);

	/* 3.4 Dialog Token */
	*pPkt = ucDialogToken;
	LR_TDLS_FME_FIELD_FILL(1);

	if (u2StatusCode == 0) {
		/*
		 * 3.5 RSNIE (optional)
		 * 3.7 FTIE (optional)
		 * 3.8 Timeout Interval (optional)
		 */
		if (pAppendIe) {
			kalMemCopy(pPkt, pAppendIe, AppendIeLen);
			LR_TDLS_FME_FIELD_FILL(AppendIeLen);
		}

		/* 3.6 EDCA Parameter Set */
		/* HT WMM IE append */
		if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {
			u4IeLen = mqmGenerateWmmInfoIEByStaRec(prAdapter,
					prBssInfo, prStaRec, pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
			u4IeLen = mqmGenerateWmmParamIEByParam(prAdapter,
					prBssInfo, pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
		}
	}

	/* 3.10 Link Identifier */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID,
		prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator,
		prBssInfo->aucOwnMacAddr, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder,
		pPeerMac, 6);

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 4. Update packet length */
	kalSetPacketLength(pvPacket, u4PktLen);

	/* 5. send the data frame */
	kalWlanHardStartXmit(pvPacket, kalGetPacketDev(pvPacket));

	return TDLS_STATUS_PENDING;
}

/*
 * \brief This routine is called to transmit a TDLS data frame.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t			/* TDLS_STATUS */
TdlsDataFrameSend_DISCOVERY_REQ(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				uint8_t *pPeerMac,
				uint8_t ucActionCode,
				uint8_t ucDialogToken, uint16_t u2StatusCode,
				uint8_t *pAppendIe, uint32_t AppendIeLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	void *pvPacket = NULL;
	uint8_t *pPkt = NULL, *pucInitiator, *pucResponder;
	uint32_t u4PktLen, u4IeLen;

	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;

	if (prStaRec != NULL) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prStaRec->ucBssIndex);
		if (prBssInfo == NULL) {
			DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
				, prStaRec->ucBssIndex);
			return TDLS_STATUS_FAIL;
		}
	} else
		return TDLS_STATUS_FAIL;

	/* allocate/init packet */
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	/* make up frame content */
	pvPacket = kalPacketAllocWithHeadroom(prGlueInfo, 512, &pPkt);
	if (pvPacket == NULL)
		return TDLS_STATUS_RESOURCES;
	kalSetPacketDev(prGlueInfo, prStaRec->ucBssIndex, pvPacket);
	if (kalGetPacketDev(pvPacket) == NULL) {
		kalPacketFree(prGlueInfo, pvPacket);
		return TDLS_STATUS_FAIL;
	}
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
	LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
	*(uint16_t *) pPkt = HTONS(TDLS_FRM_PROT_TYPE);
	LR_TDLS_FME_FIELD_FILL(2);

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	LR_TDLS_FME_FIELD_FILL(1);

	/*
	 * 3.1 Category
	 * 3.2 Action
	 * 3.3 Dialog Token
	 * 3.4 Link Identifier
	 */

	/* 3.1 Category */
	*pPkt = TDLS_FRM_CATEGORY;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.2 Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.3 Dialog Token */
	*pPkt = ucDialogToken;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.4 Link Identifier */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID,
			prBssInfo->aucBSSID, 6);
	pucInitiator = prBssInfo->aucOwnMacAddr;
	pucResponder = pPeerMac;
	prStaRec->flgTdlsIsInitiator = TRUE;
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, pucInitiator, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, pucResponder, 6);
	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 4. Update packet length */
	kalSetPacketLength(pvPacket, u4PktLen);

	/* 5. send the data frame */
	kalWlanHardStartXmit(pvPacket, kalGetPacketDev(pvPacket));

	return TDLS_STATUS_PENDING;
}

uint32_t
TdlsDataFrameSend_DISCOVERY_RSP(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				uint8_t *pPeerMac,
				uint8_t ucActionCode,
				uint8_t ucDialogToken, uint16_t u2StatusCode,
				uint8_t *pAppendIe, uint32_t AppendIeLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct BSS_INFO *prBssInfo;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	struct MSDU_INFO *prMsduInfoMgmt;
	uint8_t *pPkt, *pucInitiator, *pucResponder;
	uint32_t u4PktLen, u4IeLen;
	uint16_t u2CapInfo;
	struct WLAN_MAC_HEADER *prHdr;

	prGlueInfo = (struct GLUE_INFO *) prAdapter->prGlueInfo;

	/* sanity check */
	if (prStaRec != NULL) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prStaRec->ucBssIndex);
		if (prBssInfo == NULL) {
			DBGLOG(TDLS, ERROR, "prBssInfo %d is NULL!\n"
				, prStaRec->ucBssIndex);
			return TDLS_STATUS_FAIL;
		}
	} else
		return TDLS_STATUS_FAIL;

	/* allocate/init packet */
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	prMsduInfoMgmt = NULL;

	/* make up frame content */
	prMsduInfoMgmt = (struct MSDU_INFO *) cnmMgtPktAlloc(prAdapter,
					PUBLIC_ACTION_MAX_LEN);
	if (prMsduInfoMgmt == NULL)
		return TDLS_STATUS_RESOURCES;

	pPkt = (uint8_t *) prMsduInfoMgmt->prPacket;
	prHdr = (struct WLAN_MAC_HEADER *) pPkt;

	/* 1. 802.11 header */
	prHdr->u2FrameCtrl = MAC_FRAME_ACTION;
	prHdr->u2DurationID = 0;
	kalMemCopy(prHdr->aucAddr1, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	kalMemCopy(prHdr->aucAddr2, prBssInfo->aucOwnMacAddr,
		TDLS_FME_MAC_ADDR_LEN);
	kalMemCopy(prHdr->aucAddr3, prBssInfo->aucBSSID, TDLS_FME_MAC_ADDR_LEN);
	prHdr->u2SeqCtrl = 0;
	LR_TDLS_FME_FIELD_FILL(sizeof(struct WLAN_MAC_HEADER));


	/*
	 * 3.1 Category
	 * 3.2 Action
	 * 3.3 Dialog Token
	 * 3.4 Capability
	 * 3.5 Supported rates
	 * 3.6 Extended supported rates
	 * 3.7 Supported Channels (optional)
	 * 3.8 RSNIE
	 * 3.9 Extended Capabilities
	 * 3.10 FTIE
	 * 3.11 Timeout Interval (optional)
	 * 3.12 Supported Regulatory Classes (optional)
	 * 3.13 HT Capabilities
	 * 3.14 20/40 BSS Coexistence
	 * 3.15 Link Identifier
	 */

	/* 3.1 Category */
	*pPkt = CATEGORY_PUBLIC_ACTION;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.2 Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3.3 Dialog Token */
	*pPkt = ucDialogToken;
	LR_TDLS_FME_FIELD_FILL(1);


	/* 3.4 Capability */
	u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
	WLAN_SET_FIELD_16(pPkt, u2CapInfo);
	LR_TDLS_FME_FIELD_FILL(2);

	/*
	 * 3.5 Supported rates
	 * 3.6 Extended supported rates
	 * 3.7 Supported Channels (optional)
	 */
	u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 3.9 Extended Capabilities */
	EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
	EXT_CAP_IE(pPkt)->ucLength = 5;

	EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00; /* bit0 ~ bit7 */
	EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00; /* bit8 ~ bit15 */
	EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00; /* bit16 ~ bit23 */
	EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00; /* bit24 ~ bit31 */
	EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0x00; /* bit32 ~ bit39 */

	/* bit 28 TDLS_EX_CAP_PEER_UAPSD */
	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));
	/* bit 30 TDLS_EX_CAP_CHAN_SWITCH */
	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
	/* bit 37 TDLS_EX_CAP_TDLS */
	EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/*
	 * 3.8 RSNIE
	 * 3.10 FTIE
	 * 3.11 Timeout Interval (optional)
	 */
	if (pAppendIe != NULL) {
		kalMemCopy(pPkt, pAppendIe, AppendIeLen);
		LR_TDLS_FME_FIELD_FILL(AppendIeLen);
	}

	/* 3.12 Supported Regulatory Classes (optional) */
	/* Note: if we do not put the IE, Marvell STA will
	 * decline our TDLS setup request
	 */
	u4IeLen = rlmDomainSupOperatingClassIeFill(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 3.13 HT Capabilities */
	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11N) {
		u4IeLen = rlmFillHtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
	}

#if CFG_SUPPORT_802_11AC
	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11AC) {
		u4IeLen = rlmFillVhtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
	}
#endif
#if (CFG_SUPPORT_TDLS_11AX == 1)
	u4IeLen = _TdlsComposeCapIE(prAdapter,
		prBssInfo, prStaRec, pPkt);
	if (u4IeLen)
		LR_TDLS_FME_FIELD_FILL(u4IeLen);
#endif

	/* 3.14 20/40 BSS Coexistence */
	BSS_20_40_COEXIST_IE(pPkt)->ucId = ELEM_ID_20_40_BSS_COEXISTENCE;
	BSS_20_40_COEXIST_IE(pPkt)->ucLength = 1;
	BSS_20_40_COEXIST_IE(pPkt)->ucData = 0x01;
	LR_TDLS_FME_FIELD_FILL(3);

	/* 3.15 Link Identifier */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID,
		   prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator,
		   pPeerMac, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder,
		   prBssInfo->aucOwnMacAddr, 6);

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);
	pucInitiator = pPeerMac;
	pucResponder = prBssInfo->aucOwnMacAddr;
	if (prStaRec != NULL)
		prStaRec->flgTdlsIsInitiator = FALSE;

	if (prMsduInfoMgmt != NULL) {
		prMsduInfoMgmt->ucPacketType = TX_PACKET_TYPE_MGMT;
		prMsduInfoMgmt->ucStaRecIndex =
		prBssInfo->prStaRecOfAP->ucIndex;
		prMsduInfoMgmt->ucBssIndex = prBssInfo->ucBssIndex;
		prMsduInfoMgmt->ucMacHeaderLength =
				WLAN_MAC_MGMT_HEADER_LEN;
		prMsduInfoMgmt->fgIs802_1x = FALSE;
		prMsduInfoMgmt->fgIs802_11 = TRUE;
		prMsduInfoMgmt->u2FrameLength = u4PktLen;
		prMsduInfoMgmt->ucTxSeqNum = nicIncreaseTxSeqNum(prAdapter);
		prMsduInfoMgmt->pfTxDoneHandler = NULL;

		/* Send them to HW queue */
		nicTxEnqueueMsdu(prAdapter, prMsduInfoMgmt);
	}

	return TDLS_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to send a command to TDLS module.
 *
 * \param[in] prGlueInfo	Pointer to the Adapter structure
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[in] u4InBufLen	The length of the buffer
 * \param[out] None
 *
 * \retval None
 */
/*----------------------------------------------------------------------------*/
void TdlsexEventHandle(struct GLUE_INFO *prGlueInfo,
		       uint8_t *prInBuf, uint32_t u4InBufLen)
{
	uint32_t u4EventId;

	DBGLOG(TDLS, INFO, "TdlsexEventHandle\n");

	/* sanity check */
	if ((prGlueInfo == NULL) || (prInBuf == NULL))
		return;		/* shall not be here */

	/* handle */
	u4EventId = *(uint32_t *) prInBuf;
	u4InBufLen -= 4;

	switch (u4EventId) {
	case TDLS_HOST_EVENT_TEAR_DOWN:
		DBGLOG(TDLS, INFO, "TDLS_HOST_EVENT_TEAR_DOWN\n");
		TdlsEventTearDown(prGlueInfo, prInBuf + 4, u4InBufLen);
		break;

	case TDLS_HOST_EVENT_TX_DONE:

		break;
	}
}

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to do tear down.
 *
 * \param[in] prGlueInfo	Pointer to the Adapter structure
 * \param[in] prInBuf		A pointer to the command string buffer,
 *					from u4EventSubId
 * \param[in] u4InBufLen	The length of the buffer
 * \param[out] None
 *
 * \retval None
 *
 */
/*----------------------------------------------------------------------------*/
void TdlsEventTearDown(struct GLUE_INFO *prGlueInfo,
		       uint8_t *prInBuf, uint32_t u4InBufLen)
{
	struct STA_RECORD *prStaRec;
	uint16_t u2ReasonCode = TDLS_REASON_CODE_NONE;
	uint32_t u4TearDownSubId;
	uint8_t *pMac, aucZeroMac[6];

	/* init */
	u4TearDownSubId = *(uint32_t *) prInBuf;
	kalMemZero(aucZeroMac, sizeof(aucZeroMac));
	pMac = aucZeroMac;

	prStaRec = cnmGetStaRecByIndex(prGlueInfo->prAdapter,
				       *(prInBuf + 4));

	/* sanity check */
	if (prStaRec == NULL)
		return;

	pMac = prStaRec->aucMacAddr;

	if (fgIsPtiTimeoutSkip == TRUE) {
		/* skip PTI timeout event */
		if (u4TearDownSubId == TDLS_HOST_EVENT_TD_PTI_TIMEOUT)
			return;
	}


	if (u4TearDownSubId == TDLS_HOST_EVENT_TD_PTI_TIMEOUT) {
		DBGLOG(TDLS, INFO,
	       "TDLS_HOST_EVENT_TD_PTI_TIMEOUT TDLS_REASON_CODE_UNSPECIFIED\n");
		u2ReasonCode = TDLS_REASON_CODE_UNSPECIFIED;

		kalTdlsOpReq(prGlueInfo, prStaRec,
			(uint16_t)TDLS_TEARDOWN,
			WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE
			);
	}

	if (u4TearDownSubId == TDLS_HOST_EVENT_TD_AGE_TIMEOUT) {
		DBGLOG(TDLS, INFO,
	       "TDLS_HOST_EVENT_TD_AGE_TIMEOUT TDLS_REASON_CODE_UNREACHABLE\n");
		u2ReasonCode = TDLS_REASON_CODE_UNREACHABLE;

		kalTdlsOpReq(prGlueInfo, prStaRec,
			(uint16_t)TDLS_TEARDOWN,
			WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE
			);
	}

	DBGLOG(TDLS, INFO, "\n\n u2ReasonCode = %u\n\n",
	       u2ReasonCode);
}

void TdlsBssExtCapParse(struct STA_RECORD *prStaRec,
			uint8_t *pucIE)
{
	uint8_t *pucIeExtCap;

	/* sanity check */
	if ((prStaRec == NULL) || (pucIE == NULL))
		return;

	if (IE_ID(pucIE) != ELEM_ID_EXTENDED_CAP)
		return;

	/*
	 *  from bit0 ~
	 *  bit 38: TDLS Prohibited
	 *  The TDLS Prohibited subfield indicates whether the use of TDLS is
	 *	prohibited.
	 *  The field is set to 1 to indicate that TDLS is prohibited
	 *	and to 0 to indicate that TDLS is allowed.
	 */
	if (IE_LEN(pucIE) < 5)
		return;		/* we need 39/8 = 5 bytes */

	/* init */
	prStaRec->fgTdlsIsProhibited = FALSE;
	prStaRec->fgTdlsIsChSwProhibited = FALSE;

	/* parse */
	pucIeExtCap = pucIE + 2;
	pucIeExtCap += 4;	/* shift to the byte we care about */

	if ((*pucIeExtCap) & BIT(38 - 32))
		prStaRec->fgTdlsIsProhibited = TRUE;
	if ((*pucIeExtCap) & BIT(39 - 32))
		prStaRec->fgTdlsIsChSwProhibited = TRUE;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Generate CMD_ID_SET_TDLS_CH_SW command
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint32_t
TdlsSendChSwControlCmd(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen)
{

	struct CMD_TDLS_CH_SW rCmdTdlsChSwCtrl;
	struct BSS_INFO *prBssInfo;

	prBssInfo = aisGetDefaultLinkBssInfo(prAdapter);

	/* send command packet for scan */
	kalMemZero(&rCmdTdlsChSwCtrl,
		   sizeof(struct CMD_TDLS_CH_SW));

	rCmdTdlsChSwCtrl.fgIsTDLSChSwProhibit =
		prBssInfo->fgTdlsIsChSwProhibited;

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_TDLS_CH_SW,
			    TRUE,
			    FALSE, FALSE, NULL, NULL,
			    sizeof(struct CMD_TDLS_CH_SW),
			    (uint8_t *)&rCmdTdlsChSwCtrl, NULL, 0);
	return TDLS_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to handle TX done for TDLS data packet.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] rTxDoneStatus TX Done status
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void TdlsHandleTxDoneStatus(struct ADAPTER *prAdapter,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	DBGLOG(TDLS, INFO, "TdlsHandleTxDoneStatus=%d, fgIsWaitForTxDone=%d",
				rTxDoneStatus, fgIsWaitForTxDone);
	if (fgIsWaitForTxDone == TRUE) {
		kalOidComplete(prAdapter->prGlueInfo, 0, 0,
			WLAN_STATUS_SUCCESS);
		fgIsWaitForTxDone = FALSE;
	}
}

#endif /* CFG_SUPPORT_TDLS */

/* End of tdls.c */
