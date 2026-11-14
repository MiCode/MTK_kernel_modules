// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "pasn.c"
 *    \brief  This file implements PASN handler.
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

#if CFG_SUPPORT_PASN
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

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
struct PASN_INFO *pasnGetInfo(struct ADAPTER *prAdapter);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static void pasnRequestDoneTimeOut(struct ADAPTER *prAdapter,
					  unsigned long ulParam)
{
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);

	ASSERT(pasnInfo);

	/* Callback PASN failure */
	if (pasnInfo->pfPasnDoneCB) {
		pasnInfo->pfPasnDoneCB(prAdapter,
			PASN_STATUS_FAILURE,
			NULL,
			pasnInfo->pvUserData);
	}
	pasnInfo->fgIsRunning = FALSE;
}

struct PASN_INFO *pasnGetInfo(struct ADAPTER *prAdapter)
{
	if (prAdapter)
		return &(prAdapter->rWifiVar.rPasnInfo);
	else
		return NULL;
}

void pasnInit(struct ADAPTER *prAdapter)
{
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);

	ASSERT(pasnInfo);

	kalMemSet(pasnInfo, 0, sizeof(struct PASN_INFO));

	pasnInfo->fgIsRunning = false;

	cnmTimerInitTimer(prAdapter,
		  &pasnInfo->rPasnDoneTimer,
		  pasnRequestDoneTimeOut,
		  (uintptr_t)NULL);
}

void pasnUninit(struct ADAPTER *prAdapter)
{
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);

	ASSERT(pasnInfo);

	cnmTimerStopTimer(prAdapter, &pasnInfo->rPasnDoneTimer);
	pasnInfo->fgIsRunning = false;
}

uint8_t pasnIsRunning(struct ADAPTER *prAdapter)
{
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);

	ASSERT(pasnInfo);

	DBGLOG(PASN, LOUD,
		"Running = %d\n",
		pasnInfo->fgIsRunning);

	return pasnInfo->fgIsRunning;
}

uint32_t pasnHandlePasnRequest(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	struct PASN_AUTH *prPasnReq,
	PFN_PASN_DONE_FUNC pfPasnDoneCB,
	PFN_SECURE_RANGING_CTX_FUNC pfRangingCtxCB,
	void *pvUserData)
{
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);
	struct PASN_PEER *prPeer = NULL;
	uint8_t i;
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	uint8_t aucBcAddr[] = BC_MAC_ADDR;

	ASSERT(pasnInfo);

	if (pasnIsRunning(prAdapter)) {
		DBGLOG(PASN, ERROR, "PASN is running!\n");
		return PASN_STATUS_FAILURE;
	}

	if (!prPasnReq || prPasnReq->ucNumPeers == 0) {
		DBGLOG(PASN, ERROR,
			"Invalid PasnReq = %d, NumPeers = %d\n",
			prPasnReq, prPasnReq ? prPasnReq->ucNumPeers : 0);
		return PASN_STATUS_FAILURE;
	}

	for (i = 0; i < prPasnReq->ucNumPeers; i++) {
		prPeer = &prPasnReq->arPeer[i];

		if (prPeer == NULL) {
			DBGLOG(PASN, ERROR, "PeerIdx %d NULL\n", i);
			return PASN_STATUS_FAILURE;
		}

		DBGLOG(PASN, INFO,
			"PeerIdx: %d, OwnMac=" MACSTR ", PeerMac=" MACSTR "\n",
			i,
			MAC2STR(prPeer->aucOwnAddr),
			MAC2STR(prPeer->aucPeerAddr));

		if (EQUAL_MAC_ADDR(prPeer->aucOwnAddr, aucNullAddr) ||
			EQUAL_MAC_ADDR(prPeer->aucPeerAddr, aucNullAddr) ||
			EQUAL_MAC_ADDR(prPeer->aucPeerAddr, aucBcAddr)) {
			DBGLOG(PASN, ERROR, "Invalid PASN address!\n");
			return PASN_STATUS_FAILURE;
		}
	}

	kalIndicatePasnEvent(prAdapter, (void *)prPasnReq, ucBssIdx);

	pasnInfo->pfPasnDoneCB = pfPasnDoneCB;
	pasnInfo->pfRangingCtxCB = pfRangingCtxCB;
	pasnInfo->pvUserData = pvUserData;

	/* Update PASN running state only for AUTH action.
	 * For DELETE action, Supplicant will simply delete security
	 * context and not sending any response back.
	 */
	if (prPasnReq->eAction == PASN_ACTION_AUTH)
		pasnInfo->fgIsRunning = TRUE;

	return PASN_STATUS_SUCCESS;
}

void pasnHandlePasnResp(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct MSG_PASN_RESP *prMsg = (struct MSG_PASN_RESP *) NULL;
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);

	prMsg = (struct MSG_PASN_RESP *) prMsgHdr;

	do {
		if (!pasnInfo || !prMsg)
			return;

		/* Callback PASN done */
		if (pasnInfo->pfPasnDoneCB) {
			pasnInfo->pfPasnDoneCB(prAdapter,
				PASN_STATUS_SUCCESS,
				&prMsg->rPasnRespEvt,
				pasnInfo->pvUserData);
		}

		cnmTimerStopTimer(prAdapter, &pasnInfo->rPasnDoneTimer);
		pasnInfo->fgIsRunning = FALSE;
	} while (0);

	cnmMemFree(prAdapter, prMsgHdr);
}

void pasnHandleSecureRangingCtx(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr)
{
	struct MSG_PASN_SECURE_RANGING_CTX *prMsg =
		(struct MSG_PASN_SECURE_RANGING_CTX *) NULL;
	struct PASN_INFO *pasnInfo = pasnGetInfo(prAdapter);
	struct PASN_SECURE_RANGING_CTX *prRangingCtx;
	struct CMD_RTT_INSTALL_LTF_KEYSEED param;
	struct STA_RECORD *prStaRec = NULL;
	uint32_t status;

	prMsg = (struct MSG_PASN_SECURE_RANGING_CTX *) prMsgHdr;

	if (!pasnInfo || !prMsg)
		goto exit;

	/* Update secure ranging context */
	if (pasnInfo->pfRangingCtxCB) {
		pasnInfo->pfRangingCtxCB(prAdapter,
			&prMsg->rRangingCtx,
			pasnInfo->pvUserData);
	} else {
		/* In case there is no RTT request but STA connects to an AP.
		 * The LTF keyseed will be updated when supplicant does 4-way
		 * handshaking and we will need to update LTF keyseed to FW
		 * for PHY security to work.
		 */
		prRangingCtx = &prMsg->rRangingCtx;

		prStaRec = cnmGetStaRecByAddress(prAdapter,
			ANY_BSS_INDEX, prRangingCtx->aucPeerAddr);

		if (!prStaRec) {
			DBGLOG(PASN, ERROR, "prStaRec is NULL\n");
			goto exit;
		}

		if (prRangingCtx->u4Action ==
			QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION_ADD) {
			kalMemZero(&param,
				sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED));

			param.ucAddRemove = 1; /* add */
			param.u2WlanIdx = (uint16_t)prStaRec->ucWlanIndex;
			param.ucLtfKeyseedLen = prRangingCtx->ucLtfKeyseedLen;
			kalMemCopy(param.aucLtfKeyseed,
				prRangingCtx->aucLtfKeyseed,
				prRangingCtx->ucLtfKeyseedLen);

			status = wlanSendSetQueryCmd(prAdapter,
				CMD_ID_RTT_INSTALL_LTF_KEYSEED,
				TRUE,
				FALSE,
				FALSE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED),
				(uint8_t *) &param, NULL, 0);
		} else {
			kalMemZero(&param,
				sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED));

			param.ucAddRemove = 0; /* remove */
			param.u2WlanIdx = (uint16_t)prStaRec->ucWlanIndex;

			status = wlanSendSetQueryCmd(prAdapter,
				CMD_ID_RTT_INSTALL_LTF_KEYSEED,
				TRUE,
				FALSE,
				FALSE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_RTT_INSTALL_LTF_KEYSEED),
				(uint8_t *) &param, NULL, 0);
		}

		DBGLOG(PASN, INFO,
			"action=%d, PeerMac="MACSTR", wlanIdx=%d, status=%d\n",
			prRangingCtx->u4Action,
			MAC2STR(prRangingCtx->aucPeerAddr),
			prStaRec->ucWlanIndex, status);
	}

exit:
	cnmMemFree(prAdapter, prMsgHdr);
}
#endif
