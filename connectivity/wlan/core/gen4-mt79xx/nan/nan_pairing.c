// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */
#if CFG_SUPPORT_NAN
#ifndef INCLUDE_FROM_NAN
#define INCLUDE_FROM_NAN
#include "precomp.h"
#undef INCLUDE_FROM_NAN
#endif
#include "wpa_supp/FourWayHandShake.h"
#include "wpa_supp/src/ap/wpa_auth_glue.h"
#include "precomp.h"
#include "nan_pairing.h"
#include "nan_sec.h"
#if (CFG_SUPPORT_NAN_R4_VENDOR_PAIRING == 1)
struct PAIRING_INFO *
nanPairingInfoAlloc(struct ADAPTER *prAdapter, uint8_t *pucPeerAddr)
{
	size_t szIdx = 0;
	struct PAIRING_INFO *prPairingInfo = NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter NULL\n");
		return NULL;
	}
	DBGLOG(NAN, INFO, "Enter\n");
	for (szIdx = 0; szIdx < NAN_MAX_NDP_SESSIONS; szIdx++) {
		prPairingInfo = &prAdapter->rWifiVar.arPairingInfo[szIdx];
		if (prPairingInfo == NULL) {
			DBGLOG(NAN, ERROR, "Alloc failed\n");
			return NULL;
		}
		if (!prPairingInfo->fgIsInUse) {
			DBGLOG(NAN, INFO, "Alloc PairingFSM\n");
			prPairingInfo->ucIndex = (uint8_t)szIdx;
			nanPairingInfoInit(prAdapter, prPairingInfo,
				pucPeerAddr);
			return prPairingInfo;
		}
	}
	DBGLOG(NAN, ERROR, "No available PairingFSM\n");
	return NULL;
}

void
nanPairingInfoRelease(struct ADAPTER *prAdapter,
	struct PAIRING_INFO *prPairingInfo)
{
	if (prPairingInfo == NULL) {
		DBGLOG(NAN, INFO, "prPairingInfo NULL and return\n");
		return;
	}
	DBGLOG(NAN, INFO, "Enter\n");
	if (prPairingInfo->prNmiCxtKey) {
		DBGLOG(NAN, INFO, "Release PeerNMI " MACSTR "\n",
			MAC2STR(prPairingInfo->prNmiCxtKey->aucNmiAddr));
		nanSecReleaseNmiCxtKey(prAdapter, prPairingInfo->prNmiCxtKey);
	}

	prPairingInfo->fgIsInUse = FALSE;
	kalMemZero(&prPairingInfo->rPtk, sizeof(struct nan_ptk));
	prPairingInfo->u4SelCipherType = NAN_CIPHER_SUITE_ID_NONE;
	prPairingInfo->prNmiCxtKey = NULL;
}

void
nanPairingInfoInit(struct ADAPTER *prAdapter,
	struct PAIRING_INFO *prPairingInfo,
	uint8_t *pucPeerAddr)
{
	if (prPairingInfo == NULL) {
		DBGLOG(NAN, INFO, "FSM NULL and return\n");
		return;
	}
	DBGLOG(NAN, INFO, "Enter Nan Key Mgmt Enable[%u]\n",
		prAdapter->rWifiVar.fgEnNanKeyMgmt);
	prPairingInfo->fgIsInUse = TRUE;
	kalMemZero(&prPairingInfo->rPtk, sizeof(struct nan_ptk));
	prPairingInfo->u4SelCipherType = NAN_CIPHER_SUITE_ID_NONE;

	if (prAdapter->rWifiVar.fgEnNanKeyMgmt)
		prPairingInfo->prNmiCxtKey =
			nanSecAcquireNmiCxtKey(prAdapter,
				pucPeerAddr);

	if (prPairingInfo->prNmiCxtKey)
		DBGLOG(NAN, INFO, "PeerNMI: %02x:%02x:%02x:%02x:%02x:%02x\n",
			prPairingInfo->prNmiCxtKey->aucNmiAddr[0],
			prPairingInfo->prNmiCxtKey->aucNmiAddr[1],
			prPairingInfo->prNmiCxtKey->aucNmiAddr[2],
			prPairingInfo->prNmiCxtKey->aucNmiAddr[3],
			prPairingInfo->prNmiCxtKey->aucNmiAddr[4],
			prPairingInfo->prNmiCxtKey->aucNmiAddr[5]);
	else
		DBGLOG(NAN, ERROR,
			"Failed to allocate prNmiCxtKey\n");
}

void
nanPairingInit(struct ADAPTER *prAdapter)
{
	DBGLOG(NAN, INFO, "Enter\n");

	kalMemZero(prAdapter->rWifiVar.arPairingInfo,
		sizeof(prAdapter->rWifiVar.arPairingInfo));
}

struct PAIRING_INFO *
nanPairingInfoSearch(struct ADAPTER *prAdapter, uint8_t *pucPeerAddr)
{
	size_t szIdx = 0;
	struct PAIRING_INFO *prPairingInfo = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return NULL;
	}
	for (szIdx = 0; szIdx < NAN_MAX_NDP_SESSIONS; szIdx++) {
		prPairingInfo = &prAdapter->rWifiVar.arPairingInfo[szIdx];
		if (prPairingInfo && prPairingInfo->fgIsInUse) {
			if (prPairingInfo->prNmiCxtKey &&
				EQUAL_MAC_ADDR(
					prPairingInfo->prNmiCxtKey->aucNmiAddr,
					pucPeerAddr)) {
				DBGLOG(NAN, INFO,
					"idx=%zu, addr=%x:%x:%x:%x:%x:%x\n",
					szIdx,
					pucPeerAddr[0],
					pucPeerAddr[1],
					pucPeerAddr[2],
					pucPeerAddr[3],
					pucPeerAddr[4],
					pucPeerAddr[5]);
					return prPairingInfo;
			}
		}
	}
	DBGLOG(NAN, ERROR, "return NULL\n");
	return NULL;
}

void
nanPairingInstallTk(struct ADAPTER *prAdapter,
	struct nan_ptk *prPtk,
	uint32_t u4SelCipherType,
	uint8_t *pucPeerNMI,
	uint16_t u2WtblEntry)
{
	uint8_t *pu1Tk = NULL;
	size_t szTkLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Cipher = 0;
	uint8_t ucAlgoId = 0;

	DBGLOG(NAN, INFO,
		"Enter, WtblEntry:%u, CipherType:%u\n",
		u2WtblEntry, u4SelCipherType);

	pu1Tk = prPtk->tk;
	szTkLen = prPtk->tk_len;
	u4Cipher = u4SelCipherType;
	DBGLOG_MEM8(NAN, INFO, pu1Tk, szTkLen);
	/* TODO_CJ: dynamic chiper, dynamic keyID */
	if ((u4Cipher == NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256) ||
		(u4Cipher == NAN_CIPHER_SUITE_ID_NCS_PK_PASN_256))
		ucAlgoId = CIPHER_SUITE_GCMP_256;
	else
		ucAlgoId = CIPHER_SUITE_CCMP;

	rStatus = nanSecManageKeyCmd(prAdapter,
		NAN_KEY_OP_SET_KEY, NAN_KEY_TYPE_NMI_CXT_MGMT_KEY,
		u2WtblEntry,
		prAdapter->rDataPathInfo.aucLocalNMIAddr, pucPeerNMI,
		ucAlgoId, 0, (uint8_t)szTkLen, pu1Tk, NULL,
		FALSE, TRUE);

	if (rStatus == WLAN_STATUS_SUCCESS)
		prPtk->installed = 1;
}
#endif /* CFG_SUPPORT_NAN_R4_VENDOR_PAIRING */
#endif /* CFG_SUPPORT_NAN */
