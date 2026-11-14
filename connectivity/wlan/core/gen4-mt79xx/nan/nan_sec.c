// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "wpa_supp/FourWayHandShake.h"
#include "wpa_supp/src/ap/wpa_auth_glue.h"

/* #include "wifi_var.h" */

#include "nan/nan_sec.h"
#include "nan/nan_data_engine.h"
#include "privacy.h"
#include "nan/nan_pairing.h"
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct _NAN_SEC_CTX {
	struct wpa_supplicant rNanWpaSupp;
	struct hostapd_data rNanHapdData;

	struct QUE rNanSecCipherList;
	uint8_t *pu1CsidAttrBuf;
	uint32_t u4CsidAttrLen;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/* struct _NAN_NDP_SUDO        g_rNanNdpSudo[MAX_NDP_NUM]; */
struct _NAN_SEC_CTX g_rNanSecCtx;

struct wpa_supplicant *g_prNanWpaSupp = &g_rNanSecCtx.rNanWpaSupp;
struct hostapd_data *g_prNanHapdData = &g_rNanSecCtx.rNanHapdData;

struct wpa_sm g_arNanWpaSm[NAN_MAX_SUPPORT_NDL_NUM * NAN_MAX_SUPPORT_NDP_NUM];
struct wpa_sm_ctx g_rNanWpaSmCtx;

/* struct sta_info             g_arNanStaInfo[MAX_NDP_NUM]; */
struct wpa_state_machine
	g_arNanWpaAuthSm[NAN_MAX_SUPPORT_NDL_NUM * NAN_MAX_SUPPORT_NDP_NUM];
struct wpa_authenticator g_rNanWpaAuth[NAN_MAX_MULTI_NDI_NUM];

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
struct _NAN_MC_RX_WTBL g_arNanMcRxWtbl[NAN_MAX_MC_RX_WTBL_NUM];
#endif

struct _NAN_KEY_ENTRY_T g_arNanNmiCxtKey[NAN_NUM_NMI_CXT_KEY] = {0};
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
uint8_t g_aucNanSecAttrBuffer[NAN_IE_BUF_MAX_SIZE];

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
uint8_t g_aucNanGtkCipherSuiteList[NAN_MAX_GTK_CIPHER_SUITE_NUM] = {
	NAN_CIPHER_SUITE_ID_NCS_GTK_CCM_128,
	NAN_CIPHER_SUITE_ID_NCS_GTK_GCM_256};
uint8_t g_aucNanIgtkPn[NAN_PACKET_NUMBER_LEN];
uint8_t g_aucNanBigtkPn[NAN_PACKET_NUMBER_LEN];
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/************************************************
 *               Mc Rx Wtbl Maintain Related
 ************************************************
 */

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)

struct _NAN_MC_RX_WTBL *
nanGetMcRxWtblIdx(uint8_t *pucPeerAddr)
{
	size_t szIdx = 0;

	if (pucPeerAddr == NULL)
		return NULL;

	for (szIdx = 0; szIdx < NAN_MAX_MC_RX_WTBL_NUM; szIdx++) {
		if (kalMemCmp(pucPeerAddr,
			g_arNanMcRxWtbl[szIdx].aucPeerAddr,
			MAC_ADDR_LEN) == 0)
			return &(g_arNanMcRxWtbl[szIdx]);
	}

	return NULL;
}
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
uint32_t nanSecCheckGtkKde(struct wpa_sm *sm,
	const u8 *key_data, size_t key_data_len)
{
	struct wpa_eapol_ie_parse ie = {0};
	uint8_t ucIsGTKSA = 0, ucGroupSAUsed = 0;
	uint32_t ret = WLAN_STATUS_SUCCESS;

	/* Skip compose GTK when GTK is disabled */
	ucGroupSAUsed =
		((struct _NAN_NDP_INSTANCE_T *)(sm->pvNdp))->ucGroupSAUsed;
	ucIsGTKSA =
		(ucGroupSAUsed & NAN_GROUP_SECURITY_ASSOC_USE_GTKSA) ?
		TRUE : FALSE;

	if (ucIsGTKSA) {
		if (wpa_supplicant_parse_ies_wpa(key_data,
			key_data_len, &ie) < 0)
			return WLAN_STATUS_FAILURE;

		if (ie.gtk == NULL) {
			DBGLOG(NAN, ERROR,
				"Need to include GTK KDE in the key data\n");
			ret = WLAN_STATUS_FAILURE;
		}
	}

	return ret;
}

uint16_t
nanSecIgtkCipherNanToWfa(uint8_t ucCipherId)
{
	switch (ucCipherId) {
	case CSIA_CAP_IGTKSA_BIGTKSA_NCS_BIP_128:
		return WPA_CIPHER_AES_128_CMAC;
	case CSIA_CAP_IGTKSA_BIGTKSA_NCS_BIP_256:
		return WPA_CIPHER_BIP_GMAC_256;
	default:
		DBGLOG(NAN, ERROR, "Nan Igtk cipher is not supported\n");
		return WPA_CIPHER_NONE;
	}
}

uint16_t
nanSecGtkCipherNanToWfa(uint8_t ucCipherId)
{
	switch (ucCipherId) {
	case NAN_CIPHER_SUITE_ID_NCS_GTK_CCM_128:
		return WPA_CIPHER_CCMP;
	case NAN_CIPHER_SUITE_ID_NCS_GTK_GCM_256:
		return WPA_CIPHER_GCMP_256;

	default:
		DBGLOG(NAN, ERROR, "Nan Gtk cipher is not supported\n");
		return NAN_CIPHER_SUITE_ID_NONE;
	}
}

uint8_t
nanSecGtkCipherWfaToNan(uint16_t u2CipherId)
{
	switch (u2CipherId) {
	case WPA_CIPHER_CCMP:
		return NAN_CIPHER_SUITE_ID_NCS_GTK_CCM_128;
	case WPA_CIPHER_GCMP_256:
		return NAN_CIPHER_SUITE_ID_NCS_GTK_GCM_256;

	default:
		DBGLOG(NAN, ERROR, "Wfa Gtk cipher is not supported\n");
		return NAN_CIPHER_SUITE_ID_NONE;
	}
}

void
nanSecPacketNumberUpdate(uint8_t *pucIgtkPn, uint8_t *pucBigtkPn)
{
	kalMemCpyS(g_aucNanIgtkPn,
		NAN_PACKET_NUMBER_LEN,
		pucIgtkPn,
		NAN_PACKET_NUMBER_LEN);
	kalMemCpyS(g_aucNanBigtkPn,
		NAN_PACKET_NUMBER_LEN,
		pucBigtkPn,
		NAN_PACKET_NUMBER_LEN);
}

void
nanSecSetGtkToSm(
	struct ADAPTER *prAdapter,
	uint8_t *pucMacAddr,
	uint8_t ucCipherId,
	uint8_t ucKeyLen,
	uint8_t *pucKey,
	uint8_t ucNdiIdx)
{
	struct wpa_authenticator *wpa_auth = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	if (ucNdiIdx < NAN_NDI_INDEX_NUM) {
		wpa_auth = &g_rNanWpaAuth[ucNdiIdx];
		wpa_auth->conf.wpa_group = nanSecGtkCipherNanToWfa(ucCipherId);
		wpa_auth->group->GTK_len = ucKeyLen;
		kalMemCpyS(wpa_auth->group->GTK[0], ucKeyLen, pucKey, ucKeyLen);
	} else {
		DBGLOG(NAN, ERROR, "invalid ucNdiIdx: %hhu\n", ucNdiIdx);
	}
}

void
nanSecSetIgtkToSm(
	struct ADAPTER *prAdapter,
	uint8_t *pucMacAddr,
	uint8_t ucCipherId,
	uint8_t ucKeyLen,
	uint8_t *pucKey,
	uint8_t fgSelfKey,
	uint16_t u2WtblIdx)
{
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t aucZeroPn[] = NULL_MAC_ADDR;
	uint8_t ucIgtkAlgoId = CIPHER_SUITE_NONE;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *) NULL;
	struct BSS_INFO *prnanBssInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	uint8_t i = 0;

	DBGLOG(NAN, INFO, "Enter\n");

	if (fgSelfKey) {
		prWifiVar = &prAdapter->rWifiVar;
		prWifiVar->ucNanIgtkCipher = ucCipherId;
		g_prNanHapdData->wpa_auth->conf.ieee80211w =
			MGMT_FRAME_PROTECTION_REQUIRED;
		kalMemCpyS(g_prNanHapdData->wpa_auth->group->IGTK[0],
			ucKeyLen, pucKey, ucKeyLen);
		g_prNanHapdData->wpa_auth->conf.group_mgmt_cipher =
			nanSecIgtkCipherNanToWfa(ucCipherId);

		for (i = 0; i < NAN_BSS_INDEX_NUM; i++) {
			prNanSpecInfo =
				prAdapter->rWifiVar.aprNanSpecificBssInfo[i];
			if (prNanSpecInfo == NULL) {
				DBGLOG(NAN, ERROR,
					"prNanSpecInfo is NULL\n");
				continue;
			}
			prnanBssInfo =
				prAdapter->aprBssInfo
				[prNanSpecInfo->ucBssIndex];
			DBGLOG(NAN, INFO,
				"[NAN R4] Install TX IGTK, Wtbl:%d\n",
				prnanBssInfo->ucBMCWlanIndex);

			nan_sec_wpas_setkey_glue(FALSE,
				prnanBssInfo->ucBssIndex,
				(enum wpa_alg)wpa_cipher_to_alg(
					nanSecIgtkCipherNanToWfa(ucCipherId)),
				aucBCAddr, 4,
				pucKey, ucKeyLen);
		}
	} else {
		if (u2WtblIdx == WTBL_RESERVED_ENTRY) {
			DBGLOG(NAN, ERROR,
				"u2WtblIdx is WTBL_RESERVED_ENTRY\n");
			return;
		}
		ucIgtkAlgoId =
			nanSecGetCipherWpaToHw(
				nanSecIgtkCipherNanToWfa(ucCipherId));
		nanSecManageKeyCmd(prAdapter,
			NAN_KEY_OP_SET_KEY,
			NAN_KEY_TYPE_MC_MGMT_RX_KEY,
			u2WtblIdx,
			aucBCAddr,
			pucMacAddr,
			ucIgtkAlgoId, 4, ucKeyLen,
			pucKey, aucZeroPn,
			FALSE, TRUE);
	}
}

void
nanSecSetBigtkToSm(
	struct ADAPTER *prAdapter,
	uint8_t *pucMacAddr,
	uint8_t ucCipherId,
	uint8_t ucKeyLen,
	uint8_t *pucKey,
	uint8_t fgSelfKey,
	uint16_t u2WtblIdx)
{
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t ucBigtkAlgoId = CIPHER_SUITE_NONE;
	uint8_t aucZeroPn[] = NULL_MAC_ADDR;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecInfo =
		(struct _NAN_SPECIFIC_BSS_INFO_T *) NULL;
	struct BSS_INFO *prnanBssInfo = NULL;
	uint8_t i = 0;

	DBGLOG(NAN, INFO, "Enter\n");

	if (fgSelfKey) {
		g_prNanHapdData->wpa_auth->conf.group_mgmt_cipher =
			nanSecIgtkCipherNanToWfa(ucCipherId);
		g_prNanHapdData->wpa_auth->conf.beacon_prot = TRUE;
		g_prNanHapdData->wpa_auth->conf.ieee80211w =
			MGMT_FRAME_PROTECTION_REQUIRED;
		kalMemCpyS(g_prNanHapdData->wpa_auth->group->BIGTK[0],
			ucKeyLen, pucKey, ucKeyLen);

		for (i = 0; i < NAN_BSS_INDEX_NUM; i++) {
			prNanSpecInfo =
				prAdapter->rWifiVar.aprNanSpecificBssInfo[i];
			if (prNanSpecInfo == NULL) {
				DBGLOG(NAN, ERROR,
					"prNanSpecInfo is NULL\n");
				continue;
			}
			prnanBssInfo =
				prAdapter->aprBssInfo
				[prNanSpecInfo->ucBssIndex];
			DBGLOG(NAN, INFO,
				"[NAN R4] Install TX BIGTK, Wtbl:%d\n",
				prnanBssInfo->ucBMCWlanIndex);

			nan_sec_wpas_setkey_glue(FALSE,
				prnanBssInfo->ucBssIndex,
				(enum wpa_alg)wpa_cipher_to_alg(
					nanSecIgtkCipherNanToWfa(ucCipherId)),
				aucBCAddr, 6,
				pucKey, ucKeyLen);
		}
	} else {
		if (u2WtblIdx == WTBL_RESERVED_ENTRY) {
			DBGLOG(NAN, ERROR,
				"u2WtblIdx is WTBL_RESERVED_ENTRY\n");
			return;
		}
		if (nanSecIgtkCipherNanToWfa(ucCipherId) ==
			WPA_CIPHER_AES_128_CMAC)
			ucBigtkAlgoId = CIPHER_SUITE_BCN_PROT_CMAC_128;
		else
			ucBigtkAlgoId = CIPHER_SUITE_BCN_PROT_GMAC_256;

		nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_SET_KEY,
				NAN_KEY_TYPE_MC_MGMT_RX_KEY,
				u2WtblIdx,
				aucBCAddr,
				pucMacAddr,
				ucBigtkAlgoId, 6, ucKeyLen,
				pucKey, aucZeroPn,
				FALSE, TRUE);
	}
}

uint8_t
nanGetIgtkBigtkInMcRxWtbl(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerNmiAddr)
{
	size_t szIdx = 0;
	uint8_t ucKeyStatus = NO_KEY_EXIST_IND;

	if ((prAdapter == NULL) || (pucPeerNmiAddr == NULL))
		return NO_KEY_EXIST_IND;

	if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA))
		return NO_KEY_EXIST_IND;

	for (szIdx = 0; szIdx < NAN_MAX_MC_RX_WTBL_NUM; szIdx++) {
		if (kalMemCmp(pucPeerNmiAddr,
			g_arNanMcRxWtbl[szIdx].aucPeerAddr,
			MAC_ADDR_LEN) == 0) {
			if (g_arNanMcRxWtbl[szIdx].fgIgtkInstalled)
				ucKeyStatus |= IGTK_EXIST_IND;
			if (g_arNanMcRxWtbl[szIdx].fgBigtkInstalled)
				ucKeyStatus |= BIGTK_EXIST_IND;

			return ucKeyStatus;
		}
	}
	return NO_KEY_EXIST_IND;
}


uint16_t
nanSecCipherVendorToWfa(uint8_t ucCipherId)
{
	switch (ucCipherId) {
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_CCM:
		return WPA_CIPHER_CCMP;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_CCM_256:
		return WPA_CIPHER_CCMP_256;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_GCM:
		return WPA_CIPHER_GCMP;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_GCM_256:
		return WPA_CIPHER_GCMP_256;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_GMAC:
		return WPA_CIPHER_BIP_GMAC_128;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_GMAC_256:
		return WPA_CIPHER_BIP_GMAC_256;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_CMAC:
		return WPA_CIPHER_AES_128_CMAC;
	case NAN_VENDOR_ENCRYPTION_TYPE_AES_BIP_CMAC_256:
		return WPA_CIPHER_BIP_CMAC_256;
	default:
		DBGLOG(NAN, ERROR, "Vendor cipher is not supported\n");
		return WPA_CIPHER_NONE;
	}
}

uint32_t
nanSetIgtkBigtkInMcRxWtbl(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerNmiAddr,
	IN uint8_t ucKeyInstallStatus)
{
	size_t szIdx = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	DBGLOG(NAN, INFO,
		"[MCDATA] IN, IgtkInstalled:%lu, BigtkInstalled:%lu\n",
		(ucKeyInstallStatus & IGTK_EXIST_IND),
		(ucKeyInstallStatus & BIGTK_EXIST_IND));

	if ((prAdapter == NULL) || (pucPeerNmiAddr == NULL))
		return rStatus;

	if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA))
		return WLAN_STATUS_SUCCESS;


	for (szIdx = 0; szIdx < NAN_MAX_MC_RX_WTBL_NUM; szIdx++) {
		if (kalMemCmp(pucPeerNmiAddr,
			g_arNanMcRxWtbl[szIdx].aucPeerAddr,
				MAC_ADDR_LEN) == 0) {
			rStatus = WLAN_STATUS_SUCCESS;
			g_arNanMcRxWtbl[szIdx].fgIgtkInstalled =
				ucKeyInstallStatus & IGTK_EXIST_IND;
			g_arNanMcRxWtbl[szIdx].fgBigtkInstalled =
				ucKeyInstallStatus & BIGTK_EXIST_IND;
			return rStatus;
		}
	}
	return rStatus;
}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

size_t
nanFindAndAllocMcRxWtbl(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerAddr,
	IN uint16_t u2WtblIdx,
	IN uint8_t fgWtblReUsed,
	IN uint8_t fgChkConsistency,
	IN uint8_t fgAllocNew)
{
	size_t szIdx = 0;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecBssInfo = NULL;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;

	if ((prAdapter == NULL) || (pucPeerAddr == NULL))
		return NAN_MAX_MC_RX_WTBL_NUM;

	/* 1. use pucPeerNdiAddr to find
	 * entry that is already acquired
	 */
	for (szIdx = 0; szIdx < NAN_MAX_MC_RX_WTBL_NUM; szIdx++) {
		if (kalMemCmp(pucPeerAddr,
			g_arNanMcRxWtbl[szIdx].aucPeerAddr,
			MAC_ADDR_LEN) == 0)
			break;
	}

	if (fgChkConsistency) {
		if (szIdx < NAN_MAX_MC_RX_WTBL_NUM) {
			if (fgWtblReUsed == FALSE) {
				DBGLOG(NAN, ERROR,
					"[MCDATA]No Consistency\n");
				DBGLOG(NAN, ERROR,
					"[MCDATA]Rx Wtbl,%hu,%hhu\n",
					u2WtblIdx, fgWtblReUsed);
				DBGLOG(NAN, ERROR,
					"[MCDATA]"MACSTR"\n",
					MAC2STR(pucPeerAddr));
				return NAN_MAX_MC_RX_WTBL_NUM;
			}
		} else {
			if (fgWtblReUsed == TRUE) {
				DBGLOG(NAN, ERROR,
					"[MCDATA]No Consistency\n");
				DBGLOG(NAN, ERROR,
					"[MCDATA]Rx Wtbl,%hu,%hhu\n",
					u2WtblIdx, fgWtblReUsed);
				DBGLOG(NAN, ERROR,
					"[MCDATA]"MACSTR"\n",
					MAC2STR(pucPeerAddr));
				return NAN_MAX_MC_RX_WTBL_NUM;
			}
		}
	}

	if (szIdx < NAN_MAX_MC_RX_WTBL_NUM)
		return szIdx;

	if (fgAllocNew == FALSE)
		return szIdx;

	/* if not found, aquire a new entry */
	for (szIdx = 0; szIdx < NAN_MAX_MC_RX_WTBL_NUM; szIdx++) {
		if (g_arNanMcRxWtbl[szIdx].u2WtblIdx == WTBL_RESERVED_ENTRY) {
			g_arNanMcRxWtbl[szIdx].u2WtblIdx = u2WtblIdx;
			kalMemCpyS(g_arNanMcRxWtbl[szIdx].aucPeerAddr,
				MAC_ADDR_LEN,
				pucPeerAddr, MAC_ADDR_LEN);

			prNanSpecBssInfo = nanGetSpecificBssInfo(
				prAdapter, NAN_BSS_INDEX_MAIN);
			if (prNanSpecBssInfo)
				g_arNanMcRxWtbl[szIdx].rStaRec.ucBssIndex =
					prNanSpecBssInfo->ucBssIndex;
			else
				DBGLOG(NAN, ERROR,
					"prNanSpecBssInfo is NULL\n");
			g_arNanMcRxWtbl[szIdx].rStaRec.eStaType =
				STA_TYPE_NAN;
			g_arNanMcRxWtbl[szIdx].rStaRec.ucIndex =
				STA_REC_INDEX_BMCAST;
			g_arNanMcRxWtbl[szIdx].rStaRec.ucWlanIndex =
				(uint8_t)u2WtblIdx;
			atomic_set(&(g_arNanMcRxWtbl[szIdx].mcRxRefCount),
				0);

			nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_ACTIVATE,
				NAN_KEY_TYPE_MC_RX_KEY, u2WtblIdx,
				aucBCAddr, pucPeerAddr,
				WPA_ALG_NONE, 0,
				0, NULL, NULL,
				TRUE, FALSE);
			break;
		}
	}

	return szIdx;
}

/*
 * NAN Use Case:
 * For UC: pucLocalAddr = local NDI/NMI, pucPeerAddr = peer NDI/NMI
 * For MC Rx: pucLocalAddr = BC_MAC_ADDR, pucPeerAddr = peer NDI/NMI
 * For MC Tx: pucLocalAddr = local NDI/NMI, pucPeerAddr = NDC ID
 */
uint32_t
nanRegisterMcRxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerAddr,
	IN uint16_t u2WtblIdx,
	IN uint8_t fgWtblReUsed)
{
	size_t szIdx = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if ((prAdapter == NULL) || (pucPeerAddr == NULL))
		return rStatus;

	if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA)) {
		rStatus = WLAN_STATUS_SUCCESS;
		return rStatus;
	}

	DBGLOG(NAN, INFO,
		"[MCDATA] IN, u2WtblEntry:%hu, fgWtblReUsed:%hhu\n",
		u2WtblIdx, fgWtblReUsed);

	DBGLOG(NAN, INFO,
		"[MCDATA] " MACSTR "\n", MAC2STR(pucPeerAddr));

	szIdx = nanFindAndAllocMcRxWtbl(prAdapter,
		pucPeerAddr, u2WtblIdx, fgWtblReUsed,
		TRUE, TRUE);
	if (szIdx >= NAN_MAX_MC_RX_WTBL_NUM)
		return rStatus;

	atomic_inc(&(g_arNanMcRxWtbl[szIdx].mcRxRefCount));
	rStatus = WLAN_STATUS_SUCCESS;

	return rStatus;
}

uint32_t
nanReleaseMcRxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucPeerAddr)
{
	size_t szIdx = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;

	DBGLOG(NAN, INFO, "[MCDATA] IN\n");

	if ((prAdapter == NULL) || (pucPeerAddr == NULL))
		return rStatus;

	if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA)) {
		rStatus = WLAN_STATUS_SUCCESS;
		return rStatus;
	}

	DBGLOG(NAN, INFO,
		"[MCDATA] " MACSTR "\n", MAC2STR(pucPeerAddr));

	/* use pucPeerNdiAddr to find entry that is already acquired */
	szIdx = nanFindAndAllocMcRxWtbl(prAdapter, pucPeerAddr,
		0, FALSE,
		FALSE, FALSE);
	if (szIdx >= NAN_MAX_MC_RX_WTBL_NUM) {
		DBGLOG(NAN, ERROR,
			"NAN Mc Rx Wtbl isn't consistency with global wtbl\n");
		return rStatus;
	}

	if (atomic_dec_return(&(g_arNanMcRxWtbl[szIdx].mcRxRefCount)) == 0) {
		if (!isNanAtResetFlow()) {
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
			nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_CLS_KEY,
				NAN_KEY_TYPE_MC_RX_KEY,
				g_arNanMcRxWtbl[szIdx].u2WtblIdx,
				aucBCAddr, pucPeerAddr,
				0, 0, 0,
				NULL, NULL,
				FALSE, FALSE);

			if (g_arNanMcRxWtbl[szIdx].fgIgtkInstalled) {
				nanSecConfigNmiIgtk(prAdapter, FALSE,
					g_arNanMcRxWtbl[szIdx].u2WtblIdx,
					aucBCAddr, pucPeerAddr,
					NULL);
				g_arNanMcRxWtbl[szIdx].fgIgtkInstalled = FALSE;
			}

			if (g_arNanMcRxWtbl[szIdx].fgBigtkInstalled) {
				nanSecConfigNmiBigtk(prAdapter, FALSE,
					g_arNanMcRxWtbl[szIdx].u2WtblIdx,
					aucBCAddr, pucPeerAddr,
					NULL);
				g_arNanMcRxWtbl[szIdx].fgBigtkInstalled = FALSE;
			}
#endif

			nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_INACTIVATE,
				NAN_KEY_TYPE_MC_RX_KEY,
				g_arNanMcRxWtbl[szIdx].u2WtblIdx,
				aucBCAddr, pucPeerAddr,
				WPA_ALG_NONE, 0,
				0, NULL, NULL,
				TRUE, FALSE);
		}
		secPrivacyFreeForEntry(prAdapter,
			g_arNanMcRxWtbl[szIdx].u2WtblIdx);
		DBGLOG(NAN, INFO, "[MCDATA] free MC Data Rx WTBL:%hu\n",
			g_arNanMcRxWtbl[szIdx].u2WtblIdx);

		g_arNanMcRxWtbl[szIdx].u2WtblIdx = WTBL_RESERVED_ENTRY;
		kalMemZero(g_arNanMcRxWtbl[szIdx].aucPeerAddr,
			MAC_ADDR_LEN);
		kalMemZero(&(g_arNanMcRxWtbl[szIdx].rStaRec),
			sizeof(struct STA_RECORD));
		g_arNanMcRxWtbl[szIdx].rStaRec.ucWlanIndex =
			WTBL_RESERVED_ENTRY;
	}
	rStatus = WLAN_STATUS_SUCCESS;

	return rStatus;
}

uint32_t
nanRegisterMcTxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucNdiIdx,
	IN struct _NAN_NDC_MGMT_T *prNdcMgmt,
	OUT uint8_t *pfgFirst)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint8_t *pucLocalAddr = NULL;
	uint8_t *pucNdcId = NULL;
	uint16_t u2WtblIdx = 0;
	uint8_t fgWtblReUsed = FALSE;

	if ((prAdapter == NULL) || (prNdcMgmt == NULL))
		return rStatus;

	if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA)) {
		rStatus = WLAN_STATUS_SUCCESS;
		return rStatus;
	}

	pucNdcId = prNdcMgmt->rNdcCtrl.aucNdcId;

	pucLocalAddr = nanDataGetLocalNDIAddr(prAdapter, ucNdiIdx);
	if (!pucLocalAddr) {
		DBGLOG(NAN, ERROR, "[MCDATA] NULL local NDI\n");
		return rStatus;
	}

	u2WtblIdx = secPrivacySeekForNanEntry(prAdapter,
		pucLocalAddr, pucNdcId,
		FALSE, 0, &fgWtblReUsed);

	if (u2WtblIdx == WTBL_RESERVED_ENTRY) {
		DBGLOG(NAN, ERROR, "[MCDATA] MC Tx wtbl alloc fail\n");
		return rStatus;
	}

	DBGLOG(NAN, INFO,
		"[MCDATA] MC Tx wtbl alloc idx[%hu]\n", u2WtblIdx);
	if (!fgWtblReUsed) {
		if (pfgFirst)
			*pfgFirst = TRUE;

		atomic_set(&(prNdcMgmt->mcTxRefCount[ucNdiIdx]), 1);

		nanSecManageKeyCmd(prAdapter,
			NAN_KEY_OP_ACTIVATE,
			NAN_KEY_TYPE_MC_TX_KEY,
			u2WtblIdx,
			pucLocalAddr, pucNdcId,
			CIPHER_SUITE_NONE, 0,
			0, NULL, NULL,
			TRUE, FALSE);
	} else {
		if (pfgFirst)
			*pfgFirst = FALSE;
		atomic_inc(&(prNdcMgmt->mcTxRefCount[ucNdiIdx]));
	}
	prNdcMgmt->au2BmcTxWtblIdx[ucNdiIdx] = u2WtblIdx;

	rStatus = WLAN_STATUS_SUCCESS;

	return rStatus;
}

uint32_t
nanReleaseMcTxWtblIdx(
	IN struct ADAPTER *prAdapter,
	IN uint8_t ucNdiIndex,
	IN struct _NAN_NDC_CTRL_T *prNdcCtrl)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint8_t *pucLocalAddr = NULL;
	uint8_t *pucNdcId = NULL;
	uint16_t u2WtblIdx = 0;
	struct _NAN_NDC_MGMT_T *prNdcMgmt = NULL;

	DBGLOG(NAN, INFO, "[MCDATA] IN\n");

	if ((prAdapter == NULL) || (prNdcCtrl == NULL))
		return rStatus;

	if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA)) {
		rStatus = WLAN_STATUS_SUCCESS;
		return rStatus;
	}

	prNdcMgmt = nanSchedGetNdcMgmtByIdx(prAdapter,
		(size_t)(prNdcCtrl->ucNdcIdx));
	if (prNdcMgmt == NULL) {
		DBGLOG(NAN, ERROR, "[MCDATA] find NDC Mgmt (%u) fail\n",
			prNdcCtrl->ucNdcIdx);
		return rStatus;
	}
	pucLocalAddr = nanDataGetLocalNDIAddr(prAdapter, ucNdiIndex);
	pucNdcId = prNdcCtrl->aucNdcId;

	DBGLOG(NAN, INFO,
		"[MCDATA] " MACSTR "\n", MAC2STR(pucLocalAddr));

	if (atomic_dec_return(&(prNdcMgmt->mcTxRefCount[ucNdiIndex])) == 0) {
		u2WtblIdx = prNdcMgmt->au2BmcTxWtblIdx[ucNdiIndex];
		if (u2WtblIdx == WTBL_RESERVED_ENTRY) {
			DBGLOG(NAN, ERROR, "[MCDATA] mc wtbl release fail\n");
		} else if (!isNanAtResetFlow()) {
			/* Sync to chip to allocate WTBL resource */
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
			if (prNdcMgmt->fgIsKeyExist[ucNdiIndex]) {
				nanSecManageKeyCmd(prAdapter,
					NAN_KEY_OP_CLS_KEY,
					NAN_KEY_TYPE_MC_TX_KEY,
					u2WtblIdx,
					pucLocalAddr, pucNdcId,
					CIPHER_SUITE_NONE, 0,
					0, NULL, NULL,
					FALSE, FALSE);
				prNdcMgmt->fgIsKeyExist[ucNdiIndex] = FALSE;
			}
#endif
			nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_INACTIVATE,
				NAN_KEY_TYPE_MC_TX_KEY,
				u2WtblIdx,
				pucLocalAddr, pucNdcId,
				CIPHER_SUITE_NONE, 0,
				0, NULL, NULL,
				TRUE, FALSE);
		}

		secPrivacyFreeForEntry(prAdapter, u2WtblIdx);
		DBGLOG(NAN, INFO, "[MCDATA] free MC Data Tx WTBL:%hu\n",
			u2WtblIdx);

		prNdcMgmt->au2BmcTxWtblIdx[ucNdiIndex] = WTBL_RESERVED_ENTRY;
	}

	rStatus = WLAN_STATUS_SUCCESS;

	return rStatus;
}

#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */

/************************************************
 *               Set Key Related
 ************************************************
 */
uint32_t
nan_sec_wlanSetAddKey(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
		      IN uint32_t u4SetBufferLen) {
	struct CMD_802_11_KEY *prCmdFWKey = NULL;
	struct CMD_INFO *prCmdInfo = NULL;
	struct CMD_802_11_KEY *prCmdKey = NULL;
	uint8_t ucCmdSeqNum = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint16_t cmd_size = 0;
	uint8_t fgWtblReUsed = FALSE;

	prCmdFWKey = (struct CMD_802_11_KEY *)pvSetBuffer;
	DEBUGFUNC("wlanSetAddKey");
	DBGLOG(REQ, LOUD, "\n");

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!pvSetBuffer) {
		DBGLOG(NAN, ERROR, "pvSetBuffer error!\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(RSN, INFO, "wlanoidSetFWAddKey\n");

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prCmdFWKey->ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(REQ, INFO, "BSS Info not exist !!\n");
		return WLAN_STATUS_SUCCESS;
	}

/*        Tx  Rx   KeyType addr
 * STA, GC:
 * case1:  1    1    0    BC addr (no sta record of AP at this moment)  WEP,
 * notice: tx at default key setting WEP key now save to BSS_INFO
 * case2:  0    1    0    BSSID (sta record of AP)     RSN BC key
 * case3:  1    1    1    AP addr (sta record of AP)   RSN STA key
 *
 * GO:
 * case1:  1    1    0    BSSID (no sta record)     WEP -- Not support
 * case2:  1    0    0    BSSID (no sta record)     RSN BC key
 * case3:  1    1    1    STA addr                  STA key
 */

	/* ucKeyType; */
	if (prCmdFWKey->ucKeyType) {
		prStaRec =
			cnmGetStaRecByAddress(prAdapter, prBssInfo->ucBssIndex,
					      prCmdFWKey->aucPeerAddr);
		if (!prStaRec) { /* Already disconnected ? */
			DBGLOG(NAN, INFO,
			       "[wlan] No sta_rec, bssIdx:%d, PeerAddr:" MACSTR
			       "\n",
			       prBssInfo->ucBssIndex,
			       MAC2STR(prCmdFWKey->aucPeerAddr));

			return WLAN_STATUS_SUCCESS;
		}
	}

	prChipInfo = prAdapter->chip_info;

	if (!prChipInfo) {
		DBGLOG(NAN, ERROR, "prChipInfo error!\n");
		return WLAN_STATUS_FAILURE;
	}
	cmd_size = prChipInfo->u2CmdTxHdrSize + sizeof(struct CMD_802_11_KEY);

	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter, cmd_size);

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}
	/* increase command sequence number */
	ucCmdSeqNum = nicIncreaseCmdSeqNum(prAdapter);
	DBGLOG(REQ, INFO, "ucCmdSeqNum = %d\n", ucCmdSeqNum);

	/* compose CMD_802_11_KEY cmd pkt */
	prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
	prCmdInfo->u2InfoBufLen = cmd_size;
#if CFG_SUPPORT_REPLAY_DETECTION
	prCmdInfo->pfCmdDoneHandler = nicCmdEventSetAddKey;
	prCmdInfo->pfCmdTimeoutHandler = nicOidCmdTimeoutSetAddKey;
#else
	prCmdInfo->pfCmdDoneHandler = NULL;
	prCmdInfo->pfCmdTimeoutHandler = NULL;
#endif
	prCmdInfo->fgIsOid = FALSE;
	prCmdInfo->ucCID = CMD_ID_ADD_REMOVE_KEY;
	prCmdInfo->fgSetQuery = TRUE;
	prCmdInfo->fgNeedResp = FALSE;
	prCmdInfo->ucCmdSeqNum = ucCmdSeqNum;
	prCmdInfo->u4SetInfoLen = u4SetBufferLen;
	prCmdInfo->pvInformationBuffer = pvSetBuffer;
	prCmdInfo->u4InformationBufferLength = u4SetBufferLen;

	NIC_FILL_CMD_TX_HDR(prAdapter, prCmdInfo->pucInfoBuffer,
			    prCmdInfo->u2InfoBufLen, prCmdInfo->ucCID,
			    CMD_PACKET_TYPE_ID, &prCmdInfo->ucCmdSeqNum,
			    prCmdInfo->fgSetQuery, &prCmdKey, FALSE, 0,
			    S2D_INDEX_CMD_H2N, 0);

	/* Setup WIFI_CMD_T */
	kalMemZero(prCmdKey, sizeof(struct CMD_802_11_KEY));

	prCmdKey->ucAddRemove = 1; /* Add */

	prCmdKey->ucTxKey = prCmdFWKey->ucTxKey;
	prCmdKey->ucKeyType = prCmdFWKey->ucKeyType;
	prCmdKey->ucIsAuthenticator = prCmdFWKey->ucIsAuthenticator;
	prCmdKey->ucAlgorithmId = prCmdFWKey->ucAlgorithmId;

	prCmdKey->ucBssIdx = prCmdFWKey->ucBssIdx;
	prCmdKey->ucKeyId = prCmdFWKey->ucKeyId;

	/* Note: the key length may not correct for WPA-None */
	prCmdKey->ucKeyLen = prCmdFWKey->ucKeyLen;

	kalMemCopy(prCmdKey->aucKeyMaterial, prCmdFWKey->aucKeyMaterial,
		   prCmdKey->ucKeyLen);

	if (prStaRec) {
		if (prCmdKey->ucKeyType) { /* RSN STA */
			struct WLAN_TABLE *prWtbl;

			prWtbl = prAdapter->rWifiVar.arWtbl;
			prWtbl[prStaRec->ucWlanIndex].ucKeyId =
				prCmdKey->ucKeyId;

			prCmdKey->ucWlanIndex =
				prStaRec->ucWlanIndex;
			prStaRec->fgTransmitKeyExist =
				TRUE; /* wait for CMD Done ? */
			kalMemCopy(prCmdKey->aucPeerAddr,
				   prCmdFWKey->aucPeerAddr,
				   MAC_ADDR_LEN);
		} else {
			DBGLOG(NAN, ERROR,
				"ucKeyType error!\n");
			return WLAN_STATUS_FAILURE;
		}
	} else { /* Overwrite the old one for AP and STA WEP */
		if (prBssInfo->prStaRecOfAP) { /* NAN use this path */
			prCmdKey->ucWlanIndex =
				secPrivacySeekForNanEntry(prAdapter,
					prBssInfo->aucOwnMacAddr,
					prBssInfo->aucOwnMacAddr,
					FALSE,
					prBssInfo->ucBssIndex,
					&fgWtblReUsed);

			kalMemCopy(prCmdKey->aucPeerAddr,
				   prBssInfo->prStaRecOfAP
					   ->aucMacAddr,
				   MAC_ADDR_LEN);
		} else {
			prCmdKey->ucWlanIndex =
				secPrivacySeekForNanEntry(prAdapter,
					prBssInfo->aucOwnMacAddr,
					prBssInfo->aucOwnMacAddr,
					FALSE,
					prBssInfo->ucBssIndex,
					&fgWtblReUsed);
			kalMemCopy(prCmdKey->aucPeerAddr,
				   prBssInfo->aucOwnMacAddr,
				   MAC_ADDR_LEN);
		}
		if (!prBssInfo->prStaRecOfAP) {
			/* AP WPA/RSN */
			prBssInfo->ucBMCWlanIndexS
				[prCmdKey->ucKeyId] =
				prCmdKey->ucWlanIndex;
			prBssInfo->ucBMCWlanIndexSUsed
				[prCmdKey->ucKeyId] = TRUE;
		} else {
			/* STA WPA/RSN, should not have tx
			 * but no sta record
			 */
			prBssInfo->ucBMCWlanIndexS
				[prCmdKey->ucKeyId] =
				prCmdKey->ucWlanIndex;
			prBssInfo->ucBMCWlanIndexSUsed
				[prCmdKey->ucKeyId] = TRUE;
			DBGLOG(RSN, INFO,
			       "BMCWlanIndex kid = %d, index = %d\n",
			       prCmdKey->ucKeyId,
			       prCmdKey->ucWlanIndex);
		}
		if (prCmdKey->ucTxKey) {
			prBssInfo->fgBcDefaultKeyExist = TRUE;
			prBssInfo->ucBcDefaultKeyIdx =
				prCmdKey->ucKeyId;
		}
	}

#if 1 /* DBG */
	DBGLOG(NAN, INFO, "Add key cmd to wlan index %d:",
	       prCmdKey->ucWlanIndex);
	DBGLOG(NAN, INFO, "(BSS = %d) " MACSTR "\n", prCmdKey->ucBssIdx,
	       MAC2STR(prCmdKey->aucPeerAddr));
	DBGLOG(NAN, INFO, "Tx = %d type = %d Auth = %d\n", prCmdKey->ucTxKey,
	       prCmdKey->ucKeyType, prCmdKey->ucIsAuthenticator);
	DBGLOG(NAN, INFO, "cipher = %d keyid = %d keylen = %d\n",
	       prCmdKey->ucAlgorithmId, prCmdKey->ucKeyId, prCmdKey->ucKeyLen);
	DBGLOG_MEM8(RSN, INFO, prCmdKey->aucKeyMaterial, prCmdKey->ucKeyLen);

	DBGLOG(NAN, INFO, "wepkeyUsed = %d\n",
	       prBssInfo->wepkeyUsed[prCmdKey->ucKeyId]);
	DBGLOG(NAN, INFO, "wepkeyWlanIdx = %d:", prBssInfo->wepkeyWlanIdx);
	DBGLOG(NAN, INFO, "ucBMCWlanIndexSUsed = %d\n",
	       prBssInfo->ucBMCWlanIndexSUsed[prCmdKey->ucKeyId]);
	DBGLOG(NAN, INFO, "ucBMCWlanIndexS = %d:",
	       prBssInfo->ucBMCWlanIndexS[prCmdKey->ucKeyId]);
#endif

	/* insert into prCmdQueue */
	kalEnqueueCommand(prAdapter->prGlueInfo, (struct QUE_ENTRY *)prCmdInfo);

	/* wakeup txServiceThread later */
	GLUE_SET_EVENT(prAdapter->prGlueInfo);
	return WLAN_STATUS_PENDING;
}

uint32_t
nan_sec_wlanSetRemoveKey(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
			 IN uint32_t u4SetBufferLen) {
	struct GLUE_INFO *prGlueInfo;
	struct CMD_INFO *prCmdInfo;
	/* P_PARAM_REMOVE_KEY_T prRemovedKey; */
	struct CMD_802_11_KEY *prCmdKey;
	struct CMD_802_11_KEY *prCmdFWKey;
	uint8_t ucCmdSeqNum;
	struct WLAN_TABLE *prWlanTable;
	struct STA_RECORD *prStaRec = NULL;
	struct BSS_INFO *prBssInfo;
	/* UINT_8 i = 0; */
	unsigned char fgRemoveWepKey = FALSE;
	uint32_t ucRemoveBCKeyAtIdx = WTBL_RESERVED_ENTRY;
	uint32_t u4KeyIndex;
	struct mt66xx_chip_info *prChipInfo;
	uint16_t cmd_size;

	prCmdFWKey = (struct CMD_802_11_KEY *)pvSetBuffer;
	DEBUGFUNC("wlanoidSetRemoveKey");
	DBGLOG(RSN, INFO, "wlanSetRemoveKeybyFW\n");
	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (u4SetBufferLen < sizeof(struct PARAM_REMOVE_KEY))
		return WLAN_STATUS_INVALID_LENGTH;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set remove key! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prCmdFWKey->ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(NAN, ERROR, "prBssInfo error!\n");
		return WLAN_STATUS_FAILURE;
	}
	u4KeyIndex = prCmdFWKey->ucKeyId;
#if CFG_SUPPORT_802_11W
	if (u4KeyIndex >= MAX_KEY_NUM) {
		DBGLOG(NAN, ERROR, "u4KeyIndex is over!\n");
		return WLAN_STATUS_FAILURE;
	}
#else
/* ASSERT(prCmdKey->ucKeyId < MAX_KEY_NUM); */
#endif

	if (u4KeyIndex >= 4) {
		DBGLOG(RSN, INFO, "Remove bip key Index : 0x%08lx\n",
		       u4KeyIndex);
		return WLAN_STATUS_SUCCESS;
	}

	/* Clean up the Tx key flag */
	if (prCmdFWKey->ucKeyType) {
		prStaRec =
			cnmGetStaRecByAddress(prAdapter, prCmdFWKey->ucBssIdx,
					      prCmdFWKey->aucPeerAddr);
		if (!prStaRec)
			return WLAN_STATUS_SUCCESS;
	} else {
		if (u4KeyIndex == prBssInfo->ucBcDefaultKeyIdx)
			prBssInfo->fgBcDefaultKeyExist = FALSE;
	}

	if (!prStaRec) {
		if (prBssInfo->wepkeyUsed[u4KeyIndex] == TRUE)
			fgRemoveWepKey = TRUE;

		if (fgRemoveWepKey) {
			DBGLOG(RSN, INFO, "Remove wep key id = %d", u4KeyIndex);
			prBssInfo->wepkeyUsed[u4KeyIndex] = FALSE;
			if (prBssInfo->fgBcDefaultKeyExist &&
			    prBssInfo->ucBcDefaultKeyIdx == u4KeyIndex) {
				prBssInfo->fgBcDefaultKeyExist = FALSE;
				prBssInfo->ucBcDefaultKeyIdx = 0xff;
			}
			if (prBssInfo->wepkeyWlanIdx >= WTBL_SIZE) {
				DBGLOG(NAN, ERROR, "wepkeyWlanIdx is over!\n");
				return WLAN_STATUS_FAILURE;
			}
			ucRemoveBCKeyAtIdx = prBssInfo->wepkeyWlanIdx;
		} else {
			DBGLOG(RSN, INFO, "Remove group key id = %d",
			       u4KeyIndex);

			if (prBssInfo->ucBMCWlanIndexSUsed[u4KeyIndex]) {

				if (prBssInfo->fgBcDefaultKeyExist &&
				    prBssInfo->ucBcDefaultKeyIdx ==
					    u4KeyIndex) {
					prBssInfo->fgBcDefaultKeyExist = FALSE;
					prBssInfo->ucBcDefaultKeyIdx = 0xff;
				}
				if (u4KeyIndex != 0) {
					if (prBssInfo->
						ucBMCWlanIndexS[u4KeyIndex] >=
						WTBL_SIZE) {
						DBGLOG(NAN, ERROR,
							"ucBMCWlanIndexS is over\n");
						return WLAN_STATUS_FAILURE;
					}
				}
				ucRemoveBCKeyAtIdx =
					prBssInfo->ucBMCWlanIndexS[u4KeyIndex];
			}
		}

		DBGLOG(RSN, INFO, "ucRemoveBCKeyAtIdx = %d",
		       ucRemoveBCKeyAtIdx);

		if (ucRemoveBCKeyAtIdx >= WTBL_SIZE)
			return WLAN_STATUS_SUCCESS;
	}

	prChipInfo = prAdapter->chip_info;

	if (!prChipInfo) {
		DBGLOG(NAN, ERROR, "prChipInfo error!\n");
		return WLAN_STATUS_FAILURE;
	}
	cmd_size = prChipInfo->u2CmdTxHdrSize + sizeof(struct CMD_802_11_KEY);

	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter, cmd_size);

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prWlanTable = prAdapter->rWifiVar.arWtbl;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prCmdFWKey->ucBssIdx);

	/* increase command sequence number */
	ucCmdSeqNum = nicIncreaseCmdSeqNum(prAdapter);

	/* compose CMD_802_11_KEY cmd pkt */
	prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
	/* prCmdInfo->ucBssIndex = prRemovedKey->ucBssIdx; */
	prCmdInfo->u2InfoBufLen = cmd_size;
	prCmdInfo->pfCmdDoneHandler = NULL;
	prCmdInfo->pfCmdTimeoutHandler = NULL;
	prCmdInfo->fgIsOid = FALSE;
	prCmdInfo->ucCID = CMD_ID_ADD_REMOVE_KEY;
	prCmdInfo->fgSetQuery = TRUE;
	prCmdInfo->fgNeedResp = FALSE;
	/* prCmdInfo->fgDriverDomainMCR = FALSE; */
	prCmdInfo->ucCmdSeqNum = ucCmdSeqNum;
	prCmdInfo->u4SetInfoLen = sizeof(struct PARAM_REMOVE_KEY);
	prCmdInfo->pvInformationBuffer = pvSetBuffer;
	prCmdInfo->u4InformationBufferLength = u4SetBufferLen;
	/* Setup WIFI_CMD_T */

	NIC_FILL_CMD_TX_HDR(prAdapter, prCmdInfo->pucInfoBuffer,
			    prCmdInfo->u2InfoBufLen, prCmdInfo->ucCID,
			    CMD_PACKET_TYPE_ID, &prCmdInfo->ucCmdSeqNum,
			    prCmdInfo->fgSetQuery, &prCmdKey, FALSE, 0,
			    S2D_INDEX_CMD_H2N, 0);

	kalMemZero((uint8_t *)prCmdKey, sizeof(struct CMD_802_11_KEY));

	prCmdKey->ucAddRemove = 0; /* Remove */
	prCmdKey->ucKeyId = (uint8_t)u4KeyIndex;
	kalMemCopy(prCmdKey->aucPeerAddr, (uint8_t *)prCmdFWKey->aucPeerAddr,
		   MAC_ADDR_LEN);
	prCmdKey->ucBssIdx = prCmdFWKey->ucBssIdx;

	if (prStaRec) {
		prCmdKey->ucKeyType = 1;
		prCmdKey->ucWlanIndex = prStaRec->ucWlanIndex;
		prStaRec->fgTransmitKeyExist = FALSE;
	} else if (ucRemoveBCKeyAtIdx < WTBL_SIZE) {
		prCmdKey->ucWlanIndex = ucRemoveBCKeyAtIdx;
	} else {
		DBGLOG(NAN, ERROR,
			"prStaRec is null or ucRemoveBCKeyAtIdx >= WTBL_SIZE!\n");
		return WLAN_STATUS_FAILURE;
	}

	/* insert into prCmdQueue */
	kalEnqueueCommand(prGlueInfo, (struct QUE_ENTRY *)prCmdInfo);

	/* wakeup txServiceThread later */
	GLUE_SET_EVENT(prGlueInfo);

	return WLAN_STATUS_PENDING;
}

int32_t
nan_sec_wpas_setkey_glue(bool fgIsAp, size_t szBssIdx, enum wpa_alg alg,
		const u8 *addr, int key_idx, const u8 *key, size_t key_len)
{
	struct CMD_802_11_KEY rCmdkey;
	struct CMD_802_11_KEY *prCmdkey = &rCmdkey;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	struct BSS_INFO *prBssInfo = NULL;
	int status = 0;
	/* UINT_8 ucEntry = WTBL_RESERVED_ENTRY; */
	unsigned char fgIsBC = FALSE;

	/* TODO_CJ: every NAN should be STA and currently no GTK */

	DBGLOG(NAN, INFO,
	       "Enter, fgIsAp:%d, u1BssIdx:%d, alg:%d, key_idx:%d, key_len:%d\n",
	       fgIsAp, szBssIdx, alg, key_idx,
	       key_len); /* dump outside */

	/* _wpa_hexdump_ram(MSG_INFO, "addr", addr, 6, 1, 0); */

	/* _wpa_hexdump_ram(MSG_INFO, "key", key, key_len, 1, 0); */

	if (kalMemCmp(addr, aucBCAddr, ETH_ALEN) == 0) {
		DBGLOG(NAN, INFO, "broadcast addr");
		fgIsBC = TRUE;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(g_prAdapter, szBssIdx);
	prStaRec = cnmGetStaRecByAddress(g_prAdapter, prBssInfo->ucBssIndex,
					 (uint8_t *)addr);

	/* Compose the common add key structure */
	kalMemZero(&rCmdkey, sizeof(struct CMD_802_11_KEY));

	rCmdkey.ucAddRemove = key_len ? 1 : 0;
	rCmdkey.ucTxKey = fgIsBC ? 0 : 1;
	rCmdkey.ucKeyType = fgIsBC ? 0 : 1;

	if (fgIsAp) { /* AP */
		rCmdkey.ucIsAuthenticator = TRUE;

		if (fgIsBC) {
			kalMemCopy(&rCmdkey.aucPeerAddr,
				   prBssInfo->aucOwnMacAddr,
				   MAC_ADDR_LEN); /* Own AP */
		} else {
			kalMemCopy(&rCmdkey.aucPeerAddr, addr,
				   MAC_ADDR_LEN); /* Remote STA */
		}
	} else { /* STA */
		rCmdkey.ucIsAuthenticator = FALSE;

		if (fgIsBC) {
			kalMemCopy(&rCmdkey.aucPeerAddr, prBssInfo->aucBSSID,
				   MAC_ADDR_LEN); /* Remote AP */
		} else {
			kalMemCopy(&rCmdkey.aucPeerAddr, addr,
				   MAC_ADDR_LEN); /* Remote AP */
		}
	}

	rCmdkey.ucBssIdx = szBssIdx;

	if (alg == WPA_ALG_CCMP)
		rCmdkey.ucAlgorithmId = CIPHER_SUITE_CCMP;
	/* else if (alg == WPA_ALG_GCMP_256) */
	/* rCmdkey.ucAlgorithmId = CIPHER_SUITE_GCMP_256; */
	else if (alg == WPA_ALG_TKIP)
		rCmdkey.ucAlgorithmId = CIPHER_SUITE_TKIP;
	else if (alg == WPA_ALG_IGTK) {
		rCmdkey.ucAlgorithmId = CIPHER_SUITE_BIP;
		kalMemSet(&rCmdkey.aucPeerAddr, 0, MAC_ADDR_LEN);
	} else if (alg == WPA_ALG_BIP_GMAC_256) {
		rCmdkey.ucAlgorithmId = CIPHER_SUITE_BIP_GMAC_256;
		kalMemSet(&rCmdkey.aucPeerAddr, 0, MAC_ADDR_LEN);
	} else {
		DBGLOG(NAN, ERROR,
		       "Not support the alg=%d, reset to WPA_ALG_CCMP\n", alg);
		alg = WPA_ALG_CCMP;
		rCmdkey.ucAlgorithmId = CIPHER_SUITE_CCMP;
	}
	rCmdkey.ucKeyId = key_idx;
	rCmdkey.ucKeyLen = key_len;

	if (!rCmdkey.ucKeyType) {
		if (prBssInfo->ucBMCWlanIndex >= MAX_WTBL_ENTRY_NUM) {
			DBGLOG(NAN, INFO,
			       "WARN! Unknown ucBMCWlanIndex:%d, szBssIdx:%d",
			       prBssInfo->ucBMCWlanIndex, szBssIdx);
			rCmdkey.ucWlanIndex = (MAX_WTBL_ENTRY_NUM - 1);
			/* ASSERT(FALSE); */
		} else {
			rCmdkey.ucWlanIndex = prBssInfo->ucBMCWlanIndex;
		}
	} else {
		if (prStaRec != NULL)
			rCmdkey.ucWlanIndex = prStaRec->ucWlanIndex;
	}

	if (key != NULL) {
		if ((key_len == 32) &&
		    (rCmdkey.ucAlgorithmId == CIPHER_SUITE_TKIP) &&
		    (rCmdkey.ucIsAuthenticator == FALSE)) {
			/* Do this like driver do : mtk_cfg80211_add_key */
			kalMemCpyS(&rCmdkey.aucKeyMaterial,
				sizeof_field(struct CMD_802_11_KEY,
				aucKeyMaterial),
				key, 16);
			kalMemCpyS(&rCmdkey.aucKeyMaterial[24],
				sizeof_field(struct CMD_802_11_KEY,
				aucKeyMaterial) - 24,
				key + 16, 8);
			kalMemCpyS(&rCmdkey.aucKeyMaterial[16],
				sizeof_field(struct CMD_802_11_KEY,
				aucKeyMaterial) - 16,
				key + 24, 8);
		} else {
			kalMemCpyS(&rCmdkey.aucKeyMaterial,
				sizeof_field(struct CMD_802_11_KEY,
				aucKeyMaterial), key,
				key_len);
		}
	}

	/* End of Compose the common add key structure */

	/* Add Key */
	/* dumpCmdKey(prCmdkey); */
	if (prCmdkey->ucAddRemove) {
		if (prCmdkey->ucWlanIndex >= MAX_WTBL_ENTRY_NUM) {
			/* DBGLOG(RSN, ERROR, ("Wrong wlan index\n")); */
			DBGLOG(NAN, ERROR, "ucWlanIndex is over!\n");
			status = -1;
		} else {
			/* phase1: driver cmd trigger it */
#if 0
			nicPrivacySetKeyEntry(
				prCmdkey, prCmdkey->ucWlanIndex,
				prStaRec);
			dumpCmdKey(prCmdkey);
				_wpas_evt_cfg80211_add_key(prCmdkey);
#endif
			nan_sec_wlanSetAddKey(g_prAdapter, prCmdkey,
					      sizeof(struct CMD_802_11_KEY));
		}
	} else { /* Remove Key */
#if 0
		DBGLOG(RSN, INFO, ("[%s] Remove key\n", __func__));
		dumpCmdKey(prCmdkey);
		_wpas_evt_cfg80211_add_key(prCmdkey);
#endif
		nan_sec_wlanSetRemoveKey(g_prAdapter, prCmdkey,
					 sizeof(struct CMD_802_11_KEY));
	}
	return status;
}

int
nan_sec_wpa_supplicant_set_key(void *_wpa_s, enum wpa_alg alg, const u8 *addr,
			       int key_idx, int set_tx, const u8 *seq,
			       size_t seq_len, const u8 *key, size_t key_len) {
	struct wpa_supplicant *prWpa_s = (struct wpa_supplicant *)_wpa_s;

	DBGLOG(NAN, INFO, "Enter\n");

	nan_sec_wpas_setkey_glue(FALSE, prWpa_s->u1BssIdx, alg, addr, key_idx,
				 key, key_len);
	return 0;
}

int
nan_sec_hostapd_wpa_auth_set_key(void *ctx, int vlan_id, enum wpa_alg alg,
				 const u8 *addr, int idx, u8 *key,
				 size_t key_len) {
	/* struct hostapd_data *hapd = (struct hostapd_data *) ctx; */

	DBGLOG(NAN, INFO, "Enter\n");
#if 0 /* set key after NDP alloc sta_rec */
	nan_sec_wpas_setkey_glue(TRUE, hapd->u1BssIdx, alg,
							 addr, idx,
						 key, key_len);
#endif
	return 0;
}

/************************************************
 *               Tx Related
 ************************************************
 */
int
nan_sec_wpa_eapol_key_mic(const u8 *key, size_t key_len, u32 cipher,
			  const u8 *buf, size_t len, u8 *mic) {
	u8 hash[NAN_SHA384_MAC_LEN];

	DBGLOG(NAN, INFO, "Enter\n");

	DBGLOG(NAN, INFO, "KCK len:%d\n", key_len);
	dumpMemory8((uint8_t *)key, key_len);

	DBGLOG(NAN, INFO, "BUF_len:%d\n", len);
	dumpMemory8((uint8_t *)buf, len);

	if ((cipher == NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256) ||
		(cipher == NAN_CIPHER_SUITE_ID_NCS_PK_PASN_256)) {
		if (hmac_sha384(key, key_len, buf, len, hash)) {
			DBGLOG(NAN, INFO, "ERROR! hmac_sha384() failed");
			return WLAN_STATUS_FAILURE;
		}
		os_memcpy(mic, hash, NCS_SK_256_MIC_LEN);

		DBGLOG(NAN, INFO, "Result MIC:\n");
		dumpMemory8(mic, NCS_SK_256_MIC_LEN);
	} else {
		/* NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128 */
		if (hmac_sha256(key, key_len, buf, len, hash)) {
			DBGLOG(NAN, INFO, "ERROR! hmac_sha256() failed");
			return WLAN_STATUS_FAILURE;
		}
		os_memcpy(mic, hash, NCS_SK_128_MIC_LEN);

		DBGLOG(NAN, INFO, "Result MIC:\n");
		dumpMemory8(mic, NCS_SK_128_MIC_LEN);
	}

	return WLAN_STATUS_SUCCESS;
}

int
nan_sec_wpa_supplicant_send_2_of_4(struct wpa_sm *sm, const unsigned char *dst,
				   const struct wpa_eapol_key *key, int ver,
				   const u8 *nonce, const u8 *wpa_ie,
				   size_t wpa_ie_len, struct wpa_ptk *ptk) {
	size_t mic_len, hdrlen; /*rlen*/
	struct wpa_eapol_key *reply;
	struct wpa_eapol_key_192 *reply192;
	u8 *key_mic; /* *rbuf */
	/* u8 *rsn_ie_buf = NULL; */

	struct _NAN_SEC_KDE_ATTR_HDR *prNanSecKdeAttrHdr = NULL;
	uint32_t u4TotalLen = 0;

	DBGLOG(NAN, INFO, "Enter\n");

#if 0
	if (wpa_ie == NULL) {
		wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
				"WPA: No wpa_ie set-cannot generate msg 2/4");
		return -1;
	}

	wpa_hexdump(MSG_DEBUG, "WPA: WPA IE for msg 2/4", wpa_ie, wpa_ie_len);
#endif

	mic_len = wpa_mic_len(sm->key_mgmt);
	hdrlen = mic_len == 24 ? sizeof(*reply192) : sizeof(*reply);
#if 0
	rbuf = wpa_sm_alloc_eapol(sm, IEEE802_1X_TYPE_EAPOL_KEY,
			NULL, hdrlen + wpa_ie_len,
			&rlen, (void *) &reply);

	if (rbuf == NULL) {
		os_free(rsn_ie_buf);
		return -1;
	}
#endif

	u4TotalLen = sizeof(struct _NAN_SEC_KDE_ATTR_HDR) + hdrlen;

	sm->u4TmpKdeAttrLen = u4TotalLen;

	kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);

	prNanSecKdeAttrHdr =
		(struct _NAN_SEC_KDE_ATTR_HDR *)sm->au1TmpKdeAttrBuf;
	prNanSecKdeAttrHdr->u1AttrId = NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR;
	prNanSecKdeAttrHdr->u2AttrLen = hdrlen + 1; /*1:publishId*/
	prNanSecKdeAttrHdr->u1PublishId =
		((struct _NAN_NDP_INSTANCE_T *)(sm->pvNdp))->ucPublishId;

	reply = (struct wpa_eapol_key *)(sm->au1TmpKdeAttrBuf  +
					 sizeof(struct _NAN_SEC_KDE_ATTR_HDR));

	reply192 = (struct wpa_eapol_key_192 *)reply;

	reply->type =
		(sm->proto == WPA_PROTO_RSN || sm->proto == WPA_PROTO_OSEN)
			? EAPOL_KEY_TYPE_RSN
			: EAPOL_KEY_TYPE_WPA;
	WPA_PUT_BE16(reply->key_info,
				 WPA_KEY_INFO_KEY_TYPE | WPA_KEY_INFO_MIC);
#if 0
	if (sm->proto == WPA_PROTO_RSN || sm->proto == WPA_PROTO_OSEN)
		WPA_PUT_BE16(reply->key_length, 0);
	else
		os_memcpy(reply->key_length, key->key_length, 2);
#endif

	WPA_PUT_BE16(reply->key_length, 0);

	os_memcpy(reply->replay_counter, key->replay_counter,
		  WPA_REPLAY_COUNTER_LEN);
	wpa_hexdump(MSG_DEBUG, "WPA: Replay Counter", reply->replay_counter,
		    WPA_REPLAY_COUNTER_LEN);

	key_mic = reply192->key_mic; /* same offset for reply and reply192 */

#if 0
	if (mic_len == 24) {
		WPA_PUT_BE16(reply192->key_data_length, wpa_ie_len);
		os_memcpy(reply192 + 1, wpa_ie, wpa_ie_len);
	} else {
		WPA_PUT_BE16(reply->key_data_length, wpa_ie_len);
		os_memcpy(reply + 1, wpa_ie, wpa_ie_len);
	}
	os_free(rsn_ie_buf);
#endif

	os_memcpy(reply->key_nonce, nonce, WPA_NONCE_LEN);

	wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG, "WPA: Sending EAPOL-Key 2/4\n");

	nanSecDumpEapolKey(reply);
#if 0
	wpa_eapol_key_send_wpa(sm, ptk->kck,
		   ptk->kck_len, ver, dst, ETH_P_EAPOL,
		   rbuf, rlen, key_mic);
#endif

	sm->u1CurMsg = NAN_SEC_M2;
	nanSecMicCalStaSmStep(sm);

	return 0;
}

int
nan_sec_wpa_supplicant_send_4_of_4(struct wpa_sm *sm, const unsigned char *dst,
				   const struct wpa_eapol_key *key, u16 ver,
				   u16 key_info, struct wpa_ptk *ptk) {
	size_t mic_len, hdrlen; /*rlen*/
	struct wpa_eapol_key *reply;
	struct wpa_eapol_key_192 *reply192;
	u8 *key_mic; /* *rbuf, */

	struct _NAN_SEC_KDE_ATTR_HDR *prNanSecKdeAttrHdr = NULL;
	uint32_t u4TotalLen = 0;

	DBGLOG(NAN, INFO, "Enter\n");

	mic_len = wpa_mic_len(sm->key_mgmt);
	hdrlen = mic_len == 24 ? sizeof(*reply192) : sizeof(*reply);
#if 0
	rbuf = wpa_sm_alloc_eapol(sm, IEEE802_1X_TYPE_EAPOL_KEY, NULL,
			  hdrlen, &rlen, (void *) &reply);

	if (rbuf == NULL)
		return -1;
#endif
	u4TotalLen = sizeof(struct _NAN_SEC_KDE_ATTR_HDR) + hdrlen;

	sm->u4TmpKdeAttrLen = u4TotalLen;

	kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
	prNanSecKdeAttrHdr =
		(struct _NAN_SEC_KDE_ATTR_HDR *)sm->au1TmpKdeAttrBuf;
	prNanSecKdeAttrHdr->u1AttrId = NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR;
	prNanSecKdeAttrHdr->u2AttrLen = hdrlen + 1; /* 1:publishId */
	prNanSecKdeAttrHdr->u1PublishId =
		((struct _NAN_NDP_INSTANCE_T *)(sm->pvNdp))->ucPublishId;

	reply = (struct wpa_eapol_key *)(sm->au1TmpKdeAttrBuf +
					 sizeof(struct _NAN_SEC_KDE_ATTR_HDR));

	reply192 = (struct wpa_eapol_key_192 *)reply;

	reply->type =
		(sm->proto == WPA_PROTO_RSN || sm->proto == WPA_PROTO_OSEN)
			? EAPOL_KEY_TYPE_RSN
			: EAPOL_KEY_TYPE_WPA;
	key_info &= WPA_KEY_INFO_SECURE;
	key_info |= WPA_KEY_INFO_KEY_TYPE | WPA_KEY_INFO_MIC;
	key_info |= WPA_KEY_INFO_INSTALL;
	WPA_PUT_BE16(reply->key_info, key_info);

#if 0
	if (sm->proto == WPA_PROTO_RSN || sm->proto == WPA_PROTO_OSEN)
		WPA_PUT_BE16(reply->key_length, 0);
	else
		os_memcpy(reply->key_length, key->key_length, 2);
#endif
	WPA_PUT_BE16(reply->key_length, 0);

	os_memcpy(reply->replay_counter, key->replay_counter,
		  WPA_REPLAY_COUNTER_LEN);

	key_mic = reply192->key_mic; /* same offset for reply and reply192 */
	if (mic_len == 24)
		WPA_PUT_BE16(reply192->key_data_length, 0);
	else
		WPA_PUT_BE16(reply->key_data_length, 0);

	wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG, "WPA: Sending EAPOL-Key 4/4");
#if 0
	wpa_eapol_key_send_wpa(sm,
		   ptk->kck, ptk->kck_len, ver, dst, ETH_P_EAPOL,
		   rbuf, rlen, key_mic);
#endif

	sm->u1CurMsg = NAN_SEC_M4;
	nanSecMicCalStaSmStep(sm);

	return 0;
}

int
nan_sec_wpa_send_eapol(
	struct wpa_authenticator *wpa_auth, /* AP: KDE compose, MIC, and send */
	struct wpa_state_machine *sm, int key_info, const u8 *key_rsc,
	const u8 *nonce, const u8 *kde, size_t kde_len, int keyidx, int encr,
	int force_version) {
	/* struct ieee802_1x_hdr *hdr; */
	struct wpa_eapol_key *key;
	struct wpa_eapol_key_192 *key192;
	size_t mic_len, keyhdrlen; /* len */
	int alg;
	int key_data_len, pad_len = 0;
	u8 *buf, *pos;
	int version, pairwise;
	int i;
	u8 *key_data;
	u32 key_len = 0;
	struct _NAN_SEC_KDE_ATTR_HDR *prNanSecKdeAttrHdr = NULL;
	uint32_t u4TotalLen = 0;

	DBGLOG(NAN, INFO, "Enter\n");

	mic_len = wpa_mic_len(sm->wpa_key_mgmt);
	keyhdrlen = mic_len == 24 ? sizeof(*key192) : sizeof(*key);

/* len = sizeof(struct ieee802_1x_hdr) + keyhdrlen; */
	/*
	 * 7.1.3.5 NAN Shared Key Cipher Suite
	 * The version of 802.11 key descriptor shall be zero,
	 * indicating the use of AES CMAC and NIST AES Key Wrap
	 */
#if 0 /* (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1) */
	if (force_version)
		version = force_version;
	else if (sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN ||
			 wpa_key_mgmt_suite_b(sm->wpa_key_mgmt))
		version = WPA_KEY_INFO_TYPE_AKM_DEFINED;
	else if (wpa_use_aes_cmac(sm))
		version = WPA_KEY_INFO_TYPE_AES_128_CMAC;
	else if (sm->pairwise != WPA_CIPHER_TKIP)
		version = WPA_KEY_INFO_TYPE_HMAC_SHA1_AES;
	else
		version = WPA_KEY_INFO_TYPE_HMAC_MD5_RC4;
#else

	version = WPA_KEY_INFO_TYPE_AKM_DEFINED;
#endif
	pairwise = !!(key_info & WPA_KEY_INFO_KEY_TYPE);

	key_data_len = kde_len;

#if 0
if ((version == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES ||
		sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN ||
		wpa_key_mgmt_suite_b(sm->wpa_key_mgmt) ||
		version == WPA_KEY_INFO_TYPE_AES_128_CMAC) &&
		encr) {
#endif /* NAN's version only use AKM */
	if ((version == WPA_KEY_INFO_TYPE_AKM_DEFINED) && encr) {
		pad_len = key_data_len % 8;
		if (pad_len)
			pad_len = 8 - pad_len;
		key_data_len += pad_len + 8;
	}
#if 0
	len += key_data_len;

	hdr = os_zalloc(len);
	if (hdr == NULL)
		return;
#endif

	u4TotalLen =
		sizeof(struct _NAN_SEC_KDE_ATTR_HDR) + keyhdrlen + key_data_len;

	if (u4TotalLen > NAN_KDE_ATTR_BUF_SIZE)
		DBGLOG(NAN, ERROR, "Invalid length\n");

	sm->u4TmpKdeAttrLen = u4TotalLen;

	kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
	prNanSecKdeAttrHdr =
		(struct _NAN_SEC_KDE_ATTR_HDR *)sm->au1TmpKdeAttrBuf;
	prNanSecKdeAttrHdr->u1AttrId = NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR;
	prNanSecKdeAttrHdr->u2AttrLen =
		keyhdrlen + key_data_len + 1; /*1:publishId*/
	prNanSecKdeAttrHdr->u1PublishId =
		((struct _NAN_NDP_INSTANCE_T *)(sm->pvNdp))->ucPublishId;

	DBGLOG(NAN, INFO, "u4TotalLen:%d\n", u4TotalLen);

	key = (struct wpa_eapol_key *)(sm->au1TmpKdeAttrBuf +
				       sizeof(struct _NAN_SEC_KDE_ATTR_HDR));
	key_len = sizeof_field(
		struct wpa_state_machine, au1TmpKdeAttrBuf) -
		sizeof(struct _NAN_SEC_KDE_ATTR_HDR);
	key192 = (struct wpa_eapol_key_192 *)key;

	key_data = ((u8 *)key) + keyhdrlen;
#if 0
	hdr->version = wpa_auth->conf.eapol_version;
	hdr->type = IEEE802_1X_TYPE_EAPOL_KEY;
	hdr->length = host_to_be16(len  - sizeof(*hdr));
	key = (struct wpa_eapol_key *) (hdr + 1);
	key192 = (struct wpa_eapol_key_192 *) (hdr + 1);
	key_data = ((u8 *) (hdr + 1)) + keyhdrlen;
#endif

	key->type = sm->wpa == WPA_VERSION_WPA2 ? EAPOL_KEY_TYPE_RSN
						: EAPOL_KEY_TYPE_WPA;
	key_info |= version;
	if (encr && sm->wpa == WPA_VERSION_WPA2)
		key_info |= WPA_KEY_INFO_ENCR_KEY_DATA;
	if (sm->wpa != WPA_VERSION_WPA2)
		key_info |= keyidx << WPA_KEY_INFO_KEY_INDEX_SHIFT;
	WPA_PUT_BE16(key->key_info, key_info);

	alg = pairwise ? sm->pairwise : wpa_auth->conf.wpa_group;
	/* WPA_PUT_BE16(key->key_length, wpa_cipher_key_len(alg)); */
	WPA_PUT_BE16(key->key_length, 0);

	if (key_info & WPA_KEY_INFO_SMK_MESSAGE)
		WPA_PUT_BE16(key->key_length, 0);

	/* FIX: STSL: what to use as key_replay_counter? */
	for (i = RSNA_MAX_EAPOL_RETRIES - 1; i > 0; i--) {
		sm->key_replay[i].valid = sm->key_replay[i - 1].valid;
		os_memcpy(sm->key_replay[i].counter,
			  sm->key_replay[i - 1].counter,
			  WPA_REPLAY_COUNTER_LEN);
	}
	inc_byte_array(sm->key_replay[0].counter, WPA_REPLAY_COUNTER_LEN);
	os_memcpy(key->replay_counter, sm->key_replay[0].counter,
		  WPA_REPLAY_COUNTER_LEN);
	wpa_hexdump(MSG_DEBUG, "WPA: Replay Counter", key->replay_counter,
		    WPA_REPLAY_COUNTER_LEN);
	sm->key_replay[0].valid = TRUE;

	if (nonce)
		os_memcpy(key->key_nonce, nonce, WPA_NONCE_LEN);

	if (key_rsc)
		os_memcpy(key->key_rsc, key_rsc, WPA_KEY_RSC_LEN);

	if (kde && !encr) {
		os_memcpy(key_data, kde, kde_len);
		if (mic_len == 24)
			WPA_PUT_BE16(key192->key_data_length, kde_len);
		else
			WPA_PUT_BE16(key->key_data_length, kde_len);
	} else if (encr && kde) {
		buf = os_zalloc(key_data_len);
		if (buf == NULL) {
			/* os_free(hdr); */
			return WLAN_STATUS_FAILURE;
		}
		pos = buf;
		os_memcpy(pos, kde, kde_len);
		pos += kde_len;

		if (pad_len)
			*pos++ = 0xdd;

		wpa_hexdump_key(MSG_DEBUG, "Plaintext EAPOL-Key Key Data", buf,
				key_data_len);
#if 0
	if (version == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES ||
			sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN ||
			wpa_key_mgmt_suite_b(sm->wpa_key_mgmt) ||
			version == WPA_KEY_INFO_TYPE_AES_128_CMAC) {
#endif /* NAN's version only use AKM */
		if (version == WPA_KEY_INFO_TYPE_AKM_DEFINED) {
			if (aes_wrap(sm->PTK.kek, sm->PTK.kek_len,
				(key_data_len - 8) / 8, buf, key_data,
				key_len - keyhdrlen)) {
				/* os_free(hdr); */
				os_free(buf);
				return WLAN_STATUS_FAILURE;
			}
			if (mic_len == 24)
				WPA_PUT_BE16(key192->key_data_length,
					     key_data_len);
			else
				WPA_PUT_BE16(key->key_data_length,
					     key_data_len);
#ifndef CONFIG_NO_RC4
		} else if (sm->PTK.kek_len == 16) {
			u8 ek[32];

			os_memcpy(key->key_iv,
				  sm->group->Counter + WPA_NONCE_LEN - 16, 16);
			inc_byte_array(sm->group->Counter, WPA_NONCE_LEN);
			os_memcpy(ek, key->key_iv, 16);
			os_memcpy(ek + 16, sm->PTK.kek, sm->PTK.kek_len);
			os_memcpy(key_data, buf, key_data_len);
			rc4_skip(ek, 32, 256, key_data, key_data_len);
			if (mic_len == 24)
				WPA_PUT_BE16(key192->key_data_length,
					     key_data_len);
			else
				WPA_PUT_BE16(key->key_data_length,
					     key_data_len);
#endif /* CONFIG_NO_RC4 */
		} else {
			/* os_free(hdr); */
			os_free(buf);
			return WLAN_STATUS_FAILURE;
		}
		os_free(buf);
	}

	if (key_info & WPA_KEY_INFO_MIC) {
		/* u8 *key_mic; */

		if (!sm->PTK_valid) {
			wpa_auth_logger(
				wpa_auth, sm->addr, LOGGER_DEBUG,
				"PTK not valid when sending EAPOL-Key frame");
			/* os_free(hdr); */
			return WLAN_STATUS_FAILURE;
		}

		/* same offset for key and key192 */
#if 0
		key_mic = key192->key_mic;

		wpa_eapol_key_mic_wpa(sm->PTK.kck, sm->PTK.kck_len,
			  sm->wpa_key_mgmt, version,
			  (u8 *) hdr, len, key_mic);
#endif
	}

	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_inc_EapolFramesTx,
			   1);

#if (ENABLE_SEC_UT_LOG == 1)
	nanSecDumpEapolKey(key);
#endif
#if 0
	wpa_auth_send_eapol(wpa_auth, sm->addr, (u8 *) hdr, len,
		    sm->pairwise_set);
#endif
	nanSecMicCalApSmStep(sm);
#if 0
	os_free(hdr);
#endif

	return WLAN_STATUS_SUCCESS;
}

/************************************************
 *               Rx Related
 ************************************************
 */
int
nan_sec_wpa_verify_key_mic(int akmp, struct wpa_ptk *PTK, u8 *data,
			   size_t data_len, struct wpa_state_machine *sm) /*AP*/
{
	/* struct ieee802_1x_hdr *hdr; */
	struct wpa_eapol_key_192 *key192;
	u16 key_info;
	int ret = 0;
	u8 mic[WPA_EAPOL_KEY_MIC_MAX_LEN];
	size_t mic_len = wpa_mic_len(akmp);
	u32 cipher;
	/* UINT_8  u1RxMsg = 0; */

	struct _NAN_SEC_KDE_ATTR_HDR *hdr;

	DBGLOG(NAN, INFO, "Enter\n");

	if (data_len < sizeof(*hdr) + sizeof(struct wpa_eapol_key)) {
		DBGLOG(NAN, ERROR,
		       "ERROR! size mis-match, data_len:%d, hdr+key:%d",
		       data_len,
		       sizeof(*hdr) + sizeof(struct wpa_eapol_key));
		return -1;
	}

	/* hdr = (struct ieee802_1x_hdr *) data; */
	/* hdr = (struct _NAN_SEC_KDE_ATTR_HDR *) data; */
	hdr = (struct _NAN_SEC_KDE_ATTR_HDR *)sm->pu1GetRxMsgKdeBuf;
	key192 = (struct wpa_eapol_key_192 *)(hdr + 1);
	key_info = WPA_GET_BE16(key192->key_info);
	os_memcpy(mic, key192->key_mic, mic_len);
	os_memset(key192->key_mic, 0, mic_len);

	/* M2, M4 */
	if (mic_len == 24)
		cipher = NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256;
	else
		cipher = NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128;

	ret = nan_sec_wpa_eapol_key_mic(PTK->kck, PTK->kck_len, cipher,
					sm->pu1GetRxMsgBodyBuf,
					sm->u4GetRxMsgBodyLen, key192->key_mic);
	if (os_memcmp_const(mic, key192->key_mic, mic_len) != 0) {
		DBGLOG(NAN, WARN, "WARN! MIC mis-match, remote MIC:\n");
		dumpMemory8(mic, mic_len);
		return WLAN_STATUS_FAILURE;
	}

	os_memcpy(key192->key_mic, mic, mic_len);
	return ret;
}

/**
 * nan_sec_wpa_sm_rx_eapol - Process received NAN WPA EAPOL frames
 * @wpa_state_machine: Pointer to WPA state machin
 * (for composing TX GTK/IGTK/BIGTK in M4)
 * @sm: Pointer to WPA state machine data from wpa_sm_init()
 * @src_addr: Source MAC address of the EAPOL packet
 * Returns: WLAN_STATUS_SUCCESS = WPA EAPOL-Key processed,
 * WLAN_STATUS_INVALID_DATA = not a WPA EAPOL-Key,
 * WLAN_STATUS_FAILURE failure
 *
 * This function is called for each received EAPOL frame. Other than EAPOL-Key
 * frames can be skipped if filtering is done elsewhere. wpa_sm_rx_eapol() is
 * only processing WPA and WPA2 EAPOL-Key frames.
 *
 * The received EAPOL-Key packets are validated and valid packets are replied
 * to. In addition, key material (PTK, GTK) is configured at the end of a
 * successful key handshake.
 */
uint32_t
nan_sec_wpa_sm_rx_eapol(struct wpa_state_machine *wpaStateMachine,
	struct wpa_sm *sm, const u8 *src_addr)
{
	size_t plen, data_len, key_data_len;
	/* const struct ieee802_1x_hdr *hdr; */
	struct _NAN_SEC_KDE_ATTR_HDR *prNanSecKdeHdr;
	struct wpa_eapol_key *key;
	struct wpa_eapol_key_192 *key192;
	u16 key_info, ver;
	u8 *tmp = NULL;
	int ret = WLAN_STATUS_FAILURE;
	struct wpa_peerkey *peerkey = NULL;
	u8 *key_data;
	size_t mic_len, keyhdrlen;

	u8 *buf = sm->pu1GetRxMsgKdeBuf;
	size_t len = sm->u4GetRxMsgKdeLen;

	mic_len = wpa_mic_len(sm->key_mgmt);
	keyhdrlen = mic_len == 24 ? sizeof(*key192) : sizeof(*key);

	DBGLOG(NAN, INFO, "Enter\n");

	if (len < sizeof(*prNanSecKdeHdr) + keyhdrlen) {
		DBGLOG(NAN, INFO,
		       "Quit1. len:%d, sizeof(*prNanSecKdeHdr):%d, keyhdrlen:%d\n",
		       len, sizeof(*prNanSecKdeHdr), keyhdrlen);

		return WLAN_STATUS_FAILURE;
	}

	/* hdr = (const struct ieee802_1x_hdr *) buf; */
	/* plen = be_to_host16(hdr->length); */
	/* data_len = plen + sizeof(*hdr); */

	prNanSecKdeHdr = (struct _NAN_SEC_KDE_ATTR_HDR *)buf;
	plen = prNanSecKdeHdr->u2AttrLen;
	data_len = plen + sizeof(*prNanSecKdeHdr);

#if 0
	DBGLOG(NAN, INFO, "[%s] IEEE 802.1X RX: version=%d type=%d length=%lu",
		   __func__, hdr->version, hdr->type, (unsigned long) plen);

	if (hdr->version < EAPOL_VERSION)
		/* TODO: backwards compatibility */

		if (hdr->type != IEEE802_1X_TYPE_EAPOL_KEY) {
			/*
			*wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
			*	"WPA: EAPOL frame (type %u) discarded,
			*	not a Key frame", hdr->type);
			*/
			ret = 0;
			goto out;
		}
#endif

	if (prNanSecKdeHdr->u1AttrId != NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR) {
		DBGLOG(NAN, INFO,
		       "Error! attribute id is not for KDE:0x%x\n",
		       prNanSecKdeHdr->u1AttrId);
		goto out;
	}

	wpa_hexdump(MSG_MSGDUMP, "WPA: RX EAPOL-Key", buf, len);

#if 0
	if (plen > len - (sizeof(*prNanSecKdeHdr)+1) || plen < keyhdrlen) {
		/* Publish ID */
		/* wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
		 *		"WPA: EAPOL frame payload size %lu
		 *		invalid (frame size %lu)",
		 *		(unsigned long) plen, (unsigned long) len);
		 */
		ret = WLAN_STATUS_INVALID_LENGTH;
		goto out;
	}
#endif
	if (data_len < len) {
		wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
			"WPA: ignoring %lu bytes after the IEEE 802.1X data",
			(unsigned long)len - data_len);
	}

	/* Make a copy of the frame since we need to modify the buffer during
	 * MAC validation and Key Data decryption.
	 */
	/* tmp = os_malloc(data_len); */
	tmp = buf; /* In NAN, directly edit the buffer content from NDP */
	if (tmp == NULL)
		goto out;
#if 0
	os_memcpy(tmp, buf, data_len);

	key = (struct wpa_eapol_key *) (tmp +
			sizeof(struct ieee802_1x_hdr));

	key192 = (struct wpa_eapol_key_192 *)
		(tmp + sizeof(struct ieee802_1x_hdr));

#endif
	key = (struct wpa_eapol_key *)(tmp +
				       sizeof(struct _NAN_SEC_KDE_ATTR_HDR));
	key192 = (struct wpa_eapol_key_192
			  *)(tmp + sizeof(struct _NAN_SEC_KDE_ATTR_HDR));

	nanSecDumpEapolKey(key);

	if (mic_len == 24)
		key_data = (u8 *)(key192 + 1);
	else
		key_data = (u8 *)(key + 1);

	if (key->type != EAPOL_KEY_TYPE_WPA &&
	    key->type != EAPOL_KEY_TYPE_RSN) {
		wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
			"WPA: EAPOL-Key type (%d) unknown, discarded",
			key->type);
		ret = WLAN_STATUS_INVALID_DATA;
		goto out;
	}

	if (mic_len == 24)
		key_data_len = WPA_GET_BE16(key192->key_data_length);
	else
		key_data_len = WPA_GET_BE16(key->key_data_length);
#if 0
	wpa_eapol_key_dump(sm, key, key_data_len,
		key192->key_mic, mic_len);
#endif

	if (key_data_len > plen - keyhdrlen) {
		DBGLOG(NAN, INFO,
		       "Quit2. key_data_len:%d, plen:%d, keyhdrlen:%d\n",
		       key_data_len, plen, keyhdrlen);
		goto out;
	}

	eapol_sm_notify_lower_layer_success(sm->eapol, 0);
	key_info = WPA_GET_BE16(key->key_info);
	ver = key_info & WPA_KEY_INFO_TYPE_MASK;

#if 0
	if (ver != WPA_KEY_INFO_TYPE_HMAC_MD5_RC4 &&
#if defined(CONFIG_IEEE80211R) || defined(CONFIG_IEEE80211W)
		ver != WPA_KEY_INFO_TYPE_AES_128_CMAC &&
#endif /* CONFIG_IEEE80211R || CONFIG_IEEE80211W */
		ver != WPA_KEY_INFO_TYPE_HMAC_SHA1_AES &&
		!wpa_key_mgmt_suite_b(sm->key_mgmt) &&
		sm->key_mgmt != WPA_KEY_MGMT_OSEN) {
		wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			"WPA: Unsupported EAPOL-Key descriptor version %d",
			ver);
		goto out;
	}

	if (sm->key_mgmt == WPA_KEY_MGMT_OSEN &&
		ver != WPA_KEY_INFO_TYPE_AKM_DEFINED) {
		wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			"OSEN: Unsupported EAPOL-Key descriptor version %d",
			ver);
		goto out;
	}

	if (wpa_key_mgmt_suite_b(sm->key_mgmt) &&
		ver != WPA_KEY_INFO_TYPE_AKM_DEFINED) {
		wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			"RSN: Unsupported EAPOL-Key descriptor version %d (expected AKM defined = 0)",
			ver);
		goto out;
	}

#ifdef CONFIG_IEEE80211W
	if (wpa_key_mgmt_sha256(sm->key_mgmt)) {
		if (ver != WPA_KEY_INFO_TYPE_AES_128_CMAC &&
			sm->key_mgmt != WPA_KEY_MGMT_OSEN &&
			!wpa_key_mgmt_suite_b(sm->key_mgmt)) {
			goto out;
		}
	} else
#endif /* CONFIG_IEEE80211W */
#endif

#if 0
		if (sm->pairwise_cipher == WPA_CIPHER_CCMP &&
			!wpa_key_mgmt_suite_b(sm->key_mgmt) &&
			ver != WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
			/* wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			 *	       "WPA: CCMP is used, but EAPOL-Key
			 *	       descriptor version (%d) is not 2", ver);
			 */
			if (sm->group_cipher != WPA_CIPHER_CCMP &&
				!(key_info & WPA_KEY_INFO_KEY_TYPE)) {
			/* Earlier versions of IEEE 802.11i did not
			 * explicitly
			 * require version 2 descriptor for all EAPOL-Key
			 * packets, so allow group keys to use version 1 if
			 * CCMP is not used for them.
			 */
			/* wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			 * 		"WPA: Backwards compatibility:
			 * 		allow invalid version for
			 *		non-CCMP group keys");
			 */
			} else if (ver == WPA_KEY_INFO_TYPE_AES_128_CMAC) {
				wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
					"WPA: Interoperability workaround: allow incorrect (should have been HMAC-SHA1), but stronger (is AES-128-CMAC), descriptor version to be used");
			} else
				goto out;
		} else if (sm->pairwise_cipher == WPA_CIPHER_GCMP &&
				   !wpa_key_mgmt_suite_b(sm->key_mgmt) &&
				   ver != WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
			/* wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			 *		"WPA: GCMP is used, but EAPOL-Key
			 *		descriptor version (%d) is not 2", ver);
			 */
			goto out;
		}

	if (!peerkey && sm->rx_replay_counter_set &&
		os_memcmp(key->replay_counter, sm->rx_replay_counter,
				  WPA_REPLAY_COUNTER_LEN) <= 0) {
		/* wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
		 *		"WPA: EAPOL-Key Replay Counter did not
		 * 		increase - dropping packet");
		 */
		goto out;
	}
#endif

	if (!(key_info & (WPA_KEY_INFO_ACK | WPA_KEY_INFO_SMK_MESSAGE))) {
		wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			"WPA: No Ack bit in key_info");
		goto out;
	}

	if (key_info & WPA_KEY_INFO_REQUEST) {
		wpa_msg(sm->ctx->msg_ctx, MSG_DEBUG,
			"WPA: EAPOL-Key with Request bit - dropped");
		goto out;
	}

	/* MIC verification: zero MIC body */
	if ((key_info & WPA_KEY_INFO_MIC) && !peerkey &&
	    wpa_supplicant_verify_eapol_key_mic(sm, key192, ver, tmp,
						data_len)) {
		DBGLOG(NAN, INFO, "Quit3. MIC error!\n");
		goto out;
	}

	if ((sm->proto == WPA_PROTO_RSN || sm->proto == WPA_PROTO_OSEN) &&
	    (key_info & WPA_KEY_INFO_ENCR_KEY_DATA)) {
		if (wpa_supplicant_decrypt_key_data(sm, key, ver, key_data,
						    &key_data_len)) {
			DBGLOG(NAN, INFO, "Quit4. decrypt data error!\n");
			goto out;
		}
	}

	if (key_info & WPA_KEY_INFO_KEY_TYPE) {
		if (key_info & WPA_KEY_INFO_KEY_INDEX_MASK) {
			/* wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
			 *		"WPA: Ignored EAPOL-Key (Pairwise) with
			 *		non-zero key index");
			 */
			DBGLOG(NAN, INFO, "Quit5. non-zero key index\n");
			goto out;
		}
		if (key_info & WPA_KEY_INFO_MIC) {
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
			ret = nanSecCheckGtkKde(sm,
				key_data, (size_t)key_data_len);
			if (ret != WLAN_STATUS_SUCCESS)
				return ret;
#endif
			/* 3/4 4-Way Handshake */
			wpa_supplicant_process_3_of_4(wpaStateMachine,
				sm, key, ver, key_data,
				(size_t)key_data_len);
		} else {
			/* 1/4 4-Way Handshake */
			wpa_supplicant_process_1_of_4(sm, src_addr,
				key, ver,
				key_data, (size_t)key_data_len);
		}
	} else {
		if (key_info & WPA_KEY_INFO_MIC) {
			/* 1/2 Group Key Handshake */
			wpa_supplicant_process_1_of_2(
				sm, src_addr, key,
				key_data, key_data_len, ver);
		} else {
			/* wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
			 *		"WPA: EAPOL-Key (Group) without
			 * 		Mic bit - dropped");
			 */
			DBGLOG(NAN, INFO, "Quit6. Group without Mic bit\n");
		}
	}

	ret = WLAN_STATUS_SUCCESS;

out:
	/*bin_clear_free(tmp, data_len);*/
	return ret;
}

uint32_t
nan_sec_wpa_receive(struct wpa_authenticator *wpa_auth, /* AP */
		    struct wpa_state_machine *sm,
		    struct wpa_sm *wpaSm,
		    u8 *data, size_t data_len) {
	/* struct ieee802_1x_hdr *hdr; */
	struct wpa_eapol_key *key;
	struct wpa_eapol_key_192 *key192;
	u16 key_info, key_data_length;
	enum { PAIRWISE_2,
	       PAIRWISE_4,
	       GROUP_2,
	       REQUEST,
	       SMK_M1,
	       SMK_M3,
	       SMK_ERROR } msg;
	char *msgtxt;
#if 0
	struct wpa_eapol_ie_parse kde;
#endif
	/* int ft; */
	/* const u8 *eapol_key_ie, *key_data; */
	/* size_t eapol_key_ie_len, keyhdrlen, mic_len; */
	const u8 *key_data;
	size_t keyhdrlen, mic_len;

	struct _NAN_SEC_KDE_ATTR_HDR *hdr;

	u8 zero_nonce[WPA_NONCE_LEN] = { 0 };

	if (wpa_auth == NULL || !wpa_auth->conf.wpa || sm == NULL) {
		DBGLOG(NAN, WARN, "Lacking of condition, exit");
		return WLAN_STATUS_FAILURE;
	}

/* TODO_CJ:remove IE */
#if 0
	kde.rsn_ie = NULL;
	kde.rsn_ie_len = 0;
	kde.wpa_ie = NULL;
	kde.wpa_ie_len = 0;
	kde.mac_addr = NULL;
#endif

	mic_len = wpa_mic_len(sm->wpa_key_mgmt);
	keyhdrlen = mic_len == 24 ? sizeof(*key192) : sizeof(*key);

	if (data_len < sizeof(*hdr) + keyhdrlen)
		return WLAN_STATUS_FAILURE;

	/* hdr = (struct ieee802_1x_hdr *) data; */
	hdr = (struct _NAN_SEC_KDE_ATTR_HDR *)data;
	key = (struct wpa_eapol_key *)(hdr + 1);
	key192 = (struct wpa_eapol_key_192 *)(hdr + 1);
	key_info = WPA_GET_BE16(key->key_info);
	if (mic_len == 24) {
		key_data = (const u8 *)(key192 + 1);
		key_data_length = WPA_GET_BE16(key192->key_data_length);
	} else {
		key_data = (const u8 *)(key + 1);
		key_data_length = WPA_GET_BE16(key->key_data_length);
	}
	if (key_data_length > data_len - sizeof(*hdr) - keyhdrlen) {
		DBGLOG(NAN, INFO,
		       "Quit1. key_data_length:%d, data_len:%d, sizeof(*hdr):%d, keyhdrlen:%d\n",
		       key_data_length, data_len, sizeof(*hdr),
		       sizeof(*hdr));
		return WLAN_STATUS_FAILURE;
	}

	if (sm->wpa == WPA_VERSION_WPA2) {
		if (key->type == EAPOL_KEY_TYPE_WPA) {
			/* Some deployed station implementations seem to send
			 * msg 4/4 with incorrect type value in WPA2 mode.
			 */
		} else if (key->type != EAPOL_KEY_TYPE_RSN) {
			DBGLOG(NAN, INFO, "Quit2. key->type:%d\n",
			       key->type);
			return WLAN_STATUS_FAILURE;
		}
	} else {
		if (key->type != EAPOL_KEY_TYPE_WPA) {
			DBGLOG(NAN, INFO, "Quit3. key->type:%d\n",
			       key->type);
			return WLAN_STATUS_FAILURE;
		}
	}

	wpa_hexdump(MSG_DEBUG, "WPA: Received Key Nonce", key->key_nonce,
		    WPA_NONCE_LEN);
	DBGLOG(NAN, INFO, "Received Key Nonce\n");
	dumpMemory8(key->key_nonce, WPA_NONCE_LEN);

	wpa_hexdump(MSG_DEBUG, "WPA: Received Replay Counter",
		    key->replay_counter, WPA_REPLAY_COUNTER_LEN);
	DBGLOG(NAN, INFO, "Received Replay Counter\n");
	dumpMemory8(key->replay_counter, WPA_REPLAY_COUNTER_LEN);

	/* FIX: verify that the EAPOL-Key frame was encrypted if pairwise keys
	 *		are set
	 */
#if 0
	if ((key_info & (WPA_KEY_INFO_SMK_MESSAGE | WPA_KEY_INFO_REQUEST)) ==
		(WPA_KEY_INFO_SMK_MESSAGE | WPA_KEY_INFO_REQUEST)) {
		if (key_info & WPA_KEY_INFO_ERROR) {
			msg = SMK_ERROR;
			msgtxt = "SMK Error";
		} else {
			msg = SMK_M1;
			msgtxt = "SMK M1";
		}
	} else if (key_info & WPA_KEY_INFO_SMK_MESSAGE) {
		msg = SMK_M3;
		msgtxt = "SMK M3";
	} else if (key_info & WPA_KEY_INFO_REQUEST) {
		msg = REQUEST;
		msgtxt = "Request";
	} else if (!(key_info & WPA_KEY_INFO_KEY_TYPE)) {
		msg = GROUP_2;
		msgtxt = "2/2 Group";
	} else if (key_data_length == 0) {
		msg = PAIRWISE_4;
		msgtxt = "4/4 Pairwise";
	} else {
		msg = PAIRWISE_2;
		msgtxt = "2/4 Pairwise";
	}
#endif
	if (os_memcmp(zero_nonce, key->key_nonce, WPA_NONCE_LEN)) {
		msg = PAIRWISE_2;
		msgtxt = "2/4 Pairwise";
		DBGLOG(NAN, INFO, "Judge as M2\n");
	} else {
		msg = PAIRWISE_4;
		msgtxt = "4/4 Pairwise";
		DBGLOG(NAN, INFO, "Judge as M4\n");
	}

#if 0 /* Skip eapol version check for NAN special case */
	/* TODO: key_info type validation for PeerKey */
	if (msg == REQUEST || msg == PAIRWISE_2 || msg == PAIRWISE_4 ||
		msg == GROUP_2) {
		u16 ver = key_info & WPA_KEY_INFO_TYPE_MASK;

		if (sm->pairwise == WPA_CIPHER_CCMP ||
			sm->pairwise == WPA_CIPHER_GCMP) {
			if (wpa_use_aes_cmac(sm) &&
				sm->wpa_key_mgmt != WPA_KEY_MGMT_OSEN &&
				!wpa_key_mgmt_suite_b(sm->wpa_key_mgmt) &&
				ver != WPA_KEY_INFO_TYPE_AES_128_CMAC) {
				/* wpa_auth_logger(wpa_auth, sm->addr,
				 *	LOGGER_WARNING,
				 *	"advertised support for
				 *	AES-128-CMAC, but did not use it");
				 */
				return;
			}

			if (!wpa_use_aes_cmac(sm) &&
				ver != WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
				/* wpa_auth_logger(wpa_auth, sm->addr,
				 *		LOGGER_WARNING,
				 *		"did not use HMAC-SHA1-AES
				 *		with CCMP/GCMP");
				 */
				return;
			}
		}

		if (wpa_key_mgmt_suite_b(sm->wpa_key_mgmt) &&
			ver != WPA_KEY_INFO_TYPE_AKM_DEFINED) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_WARNING,
				"did not use EAPOL-Key descriptor version 0 as required for AKM-defined cases");
			return;
		}
	}
#endif

	if (key_info & WPA_KEY_INFO_REQUEST) {
		if (sm->req_replay_counter_used &&
		    os_memcmp(key->replay_counter, sm->req_replay_counter,
			      WPA_REPLAY_COUNTER_LEN) <= 0) {
			/* wpa_auth_logger(wpa_auth, sm->addr, LOGGER_WARNING,
			 *		"received EAPOL-Key request with
			 *		replayed counter");
			 */
			DBGLOG(NAN, INFO, "Quit4. replayed counter\n");

			return WLAN_STATUS_FAILURE;
		}
	}

	if (!(key_info & WPA_KEY_INFO_REQUEST) &&
	    !wpa_replay_counter_valid(sm->key_replay, key->replay_counter)) {
		int i;

		if (msg == PAIRWISE_2 &&
		    wpa_replay_counter_valid(sm->prev_key_replay,
					     key->replay_counter) &&
		    sm->wpa_ptk_state == WPA_PTK_PTKINITNEGOTIATING &&
		    os_memcmp(sm->SNonce, key->key_nonce, WPA_NONCE_LEN) != 0) {
			/* Some supplicant implementations (e.g., Windows XP
			 * WZC) update SNonce for each EAPOL-Key 2/4. This
			 * breaks the workaround on accepting any of the
			 * pending requests, so allow the SNonce to be updated
			 * even if we have already sent out EAPOL-Key 3/4.
			 */
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
			 *	 "Process SNonce update from STA
			 *	 based on retransmitted EAPOL-Key 1/4");
			 */
			sm->update_snonce = 1;
			os_memcpy(sm->alt_SNonce, sm->SNonce, WPA_NONCE_LEN);
			sm->alt_snonce_valid = TRUE;
			os_memcpy(sm->alt_replay_counter,
				  sm->key_replay[0].counter,
				  WPA_REPLAY_COUNTER_LEN);
			goto continue_processing;
		}

		if (msg == PAIRWISE_4 && sm->alt_snonce_valid &&
		    sm->wpa_ptk_state == WPA_PTK_PTKINITNEGOTIATING &&
		    os_memcmp(key->replay_counter, sm->alt_replay_counter,
			      WPA_REPLAY_COUNTER_LEN) == 0) {
			/* Supplicant may still be using the old SNonce since
			 * there was two EAPOL-Key 2/4 messages and they had
			 * different SNonce values.
			 */
			wpa_auth_vlogger(
				wpa_auth, sm->addr, LOGGER_DEBUG,
				"Try to process received EAPOL-Key 4/4 based on old Replay Counter and SNonce from an earlier EAPOL-Key 1/4");
			goto continue_processing;
		}

		if (msg == PAIRWISE_2 &&
		    wpa_replay_counter_valid(sm->prev_key_replay,
					     key->replay_counter) &&
		    sm->wpa_ptk_state == WPA_PTK_PTKINITNEGOTIATING) {
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
			 *	 "ignore retransmitted EAPOL-Key %s -
			 *	 SNonce did not change", msgtxt);
			 */
		} else {
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
			 *	 "received EAPOL-Key %s with
			 *	 unexpected replay counter", msgtxt);
			 */
		}
		for (i = 0; i < RSNA_MAX_EAPOL_RETRIES; i++) {
			if (!sm->key_replay[i].valid)
				break;
			wpa_hexdump(MSG_DEBUG, "pending replay counter",
				    sm->key_replay[i].counter,
				    WPA_REPLAY_COUNTER_LEN);
		}
		wpa_hexdump(MSG_DEBUG, "received replay counter",
			    key->replay_counter, WPA_REPLAY_COUNTER_LEN);

		DBGLOG(NAN, INFO, "Quit5. replay counter invalid\n");

		return WLAN_STATUS_FAILURE;
	}

continue_processing:
	switch (msg) {
	case PAIRWISE_2:
		if (sm->wpa_ptk_state != WPA_PTK_PTKSTART &&
		    sm->wpa_ptk_state != WPA_PTK_PTKCALCNEGOTIATING &&
		    (!sm->update_snonce ||
		     sm->wpa_ptk_state != WPA_PTK_PTKINITNEGOTIATING)) {
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
			 *	 "received EAPOL-Key msg 2/4 in
			 *	 invalid state (%d) - dropped",
			 *	 sm->wpa_ptk_state);
			 */
			DBGLOG(NAN, INFO,
			       "Quit6. invalid wpa_ptk_state:%d\n",
			       sm->wpa_ptk_state);

			return WLAN_STATUS_FAILURE;
		}
#if 0
		random_add_randomness(key->key_nonce, WPA_NONCE_LEN);
		if (sm->group->reject_4way_hs_for_entropy) {
			/* The system did not have enough entropy to generate
			 * strong random numbers. Reject the first 4-way
			 * handshake(s) and collect some entropy based on the
			 * information from it. Once enough entropy is
			 * available, the next atempt will trigger GMK/Key
			 * Counter update and the station will be allowed to
			 * continue.
			 */

			random_mark_pool_ready();
			/* wpa_sta_disconnect(wpa_auth, sm->addr); */
			/* TODO_CJ:terminate NDP */
			return WLAN_STATUS_FAILURE;
		}
#endif
#if 0
		if (wpa_parse_kde_ies(key_data, key_data_length, &kde) < 0) {
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
			 *	 "received EAPOL-Key msg 2/4 with
			 *	 invalid Key Data contents");
			 */
			return WLAN_STATUS_FAILURE;
		}
		if (kde.rsn_ie) {
			eapol_key_ie = kde.rsn_ie;
			eapol_key_ie_len = kde.rsn_ie_len;
		} else {
			eapol_key_ie = kde.wpa_ie;
			eapol_key_ie_len = kde.wpa_ie_len;
		}
		ft = sm->wpa == WPA_VERSION_WPA2 &&
			 wpa_key_mgmt_ft(sm->wpa_key_mgmt);
		if (sm->wpa_ie == NULL ||
			wpa_compare_rsn_ie(ft,
					   sm->wpa_ie, sm->wpa_ie_len,
					   eapol_key_ie, eapol_key_ie_len)) {
			/* wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
			 *	"WPA IE from (Re)AssocReq did not
			 *	match with msg 2/4");
			 */
			if (sm->wpa_ie) {
				wpa_hexdump(MSG_DEBUG, "WPA IE in AssocReq",
					sm->wpa_ie, sm->wpa_ie_len);
			}
			wpa_hexdump(MSG_DEBUG, "WPA IE in msg 2/4",
					eapol_key_ie, eapol_key_ie_len);
			/* MLME-DEAUTHENTICATE.request */
			/* wpa_sta_disconnect(wpa_auth, sm->addr); */
			/* TODO_CJ: NDP terminate */
			return WLAN_STATUS_FAILURE;
		}
#endif
		break;
	case PAIRWISE_4:
		if (sm->wpa_ptk_state != WPA_PTK_PTKINITNEGOTIATING ||
		    !sm->PTK_valid) {
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
			 *	 "received EAPOL-Key msg 4/4 in
			 *	 invalid state (%d) - dropped",
			 *	 sm->wpa_ptk_state);
			 */
			DBGLOG(NAN, INFO,
			       "Quit7. invalid wpa_ptk_state:%d\n",
			       sm->wpa_ptk_state);

			return WLAN_STATUS_FAILURE;
		}
		break;
	case GROUP_2:
		if (sm->wpa_ptk_group_state != WPA_PTK_GROUP_REKEYNEGOTIATING ||
		    !sm->PTK_valid) {
			/* wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
			 *		 "received EAPOL-Key msg 2/2 in
			 *		 invalid state (%d) - dropped",
			 *		 sm->wpa_ptk_group_state);
			 */
			DBGLOG(NAN, INFO,
			       "Quit8. invalid wpa_ptk_state:%d\n",
			       sm->wpa_ptk_state);

			return WLAN_STATUS_FAILURE;
		}
		break;
	case SMK_M1:
	case SMK_M3:
	case SMK_ERROR:
		return WLAN_STATUS_FAILURE;
		/* STSL disabled - ignore SMK messages */
	case REQUEST:
		break;
	}

	wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
			 "received EAPOL-Key frame (%s)", msgtxt);

	if (key_info & WPA_KEY_INFO_ACK) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
				"received invalid EAPOL-Key: Key Ack set");
		return WLAN_STATUS_FAILURE;
	}

	if (!(key_info & WPA_KEY_INFO_MIC)) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
				"received invalid EAPOL-Key: Key MIC not set");
		return WLAN_STATUS_FAILURE;
	}

	sm->MICVerified = FALSE;
	if (sm->PTK_valid && !sm->update_snonce) {
		if (nan_sec_wpa_verify_key_mic(sm->wpa_key_mgmt, &sm->PTK, data,
					       data_len, sm)) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"received EAPOL-Key with invalid MIC");
			return WLAN_STATUS_FAILURE;
		}
		sm->MICVerified = TRUE;
		eloop_cancel_timeout(wpa_send_eapol_timeout, wpa_auth, sm);
		sm->pending_1_of_4_timeout = 0;
	}

	if (key_info & WPA_KEY_INFO_REQUEST) {
		if (sm->MICVerified) {
			sm->req_replay_counter_used = 1;
			os_memcpy(sm->req_replay_counter, key->replay_counter,
				  WPA_REPLAY_COUNTER_LEN);
		} else {
			/* wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
			 *	"received EAPOL-Key request with
			 *	invalid MIC");
			 */
			DBGLOG(NAN, INFO, "Quit9. invalid MIC\n");

			return WLAN_STATUS_FAILURE;
		}

		/* TODO: should decrypt key data field if encryption was used;
		 * even though MAC address KDE is not normally encrypted,
		 * supplicant is allowed to encrypt it.
		 */
#if 0
		if (msg == SMK_ERROR) {
			return;
		}
#endif
		if (key_info & WPA_KEY_INFO_ERROR) {
			if (wpa_receive_error_report(
				    wpa_auth, sm,
				    !(key_info & WPA_KEY_INFO_KEY_TYPE)) > 0) {

				DBGLOG(NAN, INFO,
				       "Quit10. STA entry was removed\n");
				return WLAN_STATUS_FAILURE;
				/* STA entry was removed */
			}
		} else if (key_info & WPA_KEY_INFO_KEY_TYPE) {
			/* wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
			 *		"received EAPOL-Key Request for new
			 *		4-Way Handshake");
			 */
			wpa_request_new_ptk(sm);
#if 0
		} else if (key_data_length > 0 &&
			   wpa_parse_kde_ies(key_data, key_data_length, &kde) ==
				   0 &&
			   kde.mac_addr) {
#endif
		} else {
			/* wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
			 *		"received EAPOL-Key Request for GTK
			 *		rekeying");
			 */
			eloop_cancel_timeout(wpa_rekey_gtk, wpa_auth, NULL);
			wpa_rekey_gtk(wpa_auth, NULL);
		}
	} else {
		/* Do not allow the same key replay counter to be reused. */
		wpa_replay_counter_mark_invalid(sm->key_replay,
						key->replay_counter);

		if (msg == PAIRWISE_2) {
			/* Maintain a copy of the pending EAPOL-Key frames in
			 * case the EAPOL-Key frame was retransmitted. This is
			 * needed to allow EAPOL-Key msg 2/4 reply to another
			 * pending msg 1/4 to update the SNonce to work around
			 * unexpected supplicant behavior.
			 */
			os_memcpy(sm->prev_key_replay, sm->key_replay,
				  sizeof(sm->key_replay));
		} else {
			os_memset(sm->prev_key_replay, 0,
				  sizeof(sm->prev_key_replay));
		}

		/* Make sure old valid counters are not accepted anymore and
		 * do not get copied again.
		 */
		wpa_replay_counter_mark_invalid(sm->key_replay, NULL);
	}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	if (msg == PAIRWISE_4) {
		if (nan_sec_wpa_rx_eapol_4(sm, wpaSm, key)
			!= WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR,
				"Quit11. Check M4 KDE fail\n");
			return WLAN_STATUS_FAILURE;
		}
	}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

	os_free(sm->last_rx_eapol_key);
	sm->last_rx_eapol_key = os_malloc(data_len);
	if (sm->last_rx_eapol_key == NULL)
		return WLAN_STATUS_FAILURE;
	os_memcpy(sm->last_rx_eapol_key, data, data_len);
	sm->last_rx_eapol_key_len = data_len;

	sm->rx_eapol_key_secure = !!(key_info & WPA_KEY_INFO_SECURE);
	sm->EAPOLKeyReceived = TRUE;
	sm->EAPOLKeyPairwise = !!(key_info & WPA_KEY_INFO_KEY_TYPE);
	sm->EAPOLKeyRequest = !!(key_info & WPA_KEY_INFO_REQUEST);
	os_memcpy(sm->SNonce, key->key_nonce, WPA_NONCE_LEN);
	wpa_sm_step(sm);

	return WLAN_STATUS_SUCCESS;
}
/************************************************
 *               STA Init Related
 ************************************************
 */
struct wpa_sm *
nan_sec_wpa_sm_init(struct wpa_sm_ctx *ctx, struct _NAN_NDP_INSTANCE_T *prNdp) {
	struct wpa_sm *sm;

	DBGLOG(NAN, INFO, "Enter\n");

	sm = prNdp->prResponderSecSmInfo;

	if (sm == NULL)
		return NULL;

	sm->renew_snonce = 1;
	sm->ctx = ctx;

	sm->dot11RSNAConfigPMKLifetime = 43200;
	sm->dot11RSNAConfigPMKReauthThreshold = 70;
	sm->dot11RSNAConfigSATimeout = 60;

	/* hard-code first, should from driver.
	 * NAN: set from nanSecSetCipherType()
	 */
#if 0
	sm->key_mgmt = WPA_KEY_MGMT_PSK;
	sm->pairwise_cipher = WPA_CIPHER_CCMP;
	sm->group_cipher = WPA_CIPHER_CCMP;
	sm->proto = WPA_PROTO_RSN;
	sm->mgmt_group_cipher = WPA_CIPHER_AES_128_CMAC;
#endif

	return sm;
}

int
nan_sec_wpa_supplicant_init_wpa(struct wpa_supplicant *wpa_s) {
	struct wpa_sm_ctx *ctx;

	ctx = &g_rNanWpaSmCtx; /* call-back should okay for keeping only one */
	os_memset(ctx, 0, sizeof(struct wpa_sm_ctx));

	ctx->ctx = wpa_s;
	ctx->msg_ctx = wpa_s;
	ctx->set_state = _wpa_supplicant_set_state;
	ctx->get_state = _wpa_supplicant_get_state;
	ctx->deauthenticate = _wpa_supplicant_deauthenticate;
	ctx->set_key = nan_sec_wpa_supplicant_set_key;
	ctx->get_bssid = wpa_supplicant_get_bssid;
	ctx->cancel_auth_timeout = _wpa_supplicant_cancel_auth_timeout;

#if 0
	wpa_s->wpa = wpa_sm_init(ctx);    /*TODO_CJ: Per NDP link*/
	if (wpa_s->wpa == NULL) {
		DBGLOG(NAN, ERROR, "Failed to initialize WPA state machine");
		os_free(ctx);
		return -1;
	}
#endif
	return 0;
}

void
nan_sec_wpa_supplicant_init_iface(void) {
	/* wpa init */
	if (nan_sec_wpa_supplicant_init_wpa(g_prNanWpaSupp) < 0)
		return;

	/* update settings */
	wpa_supplicant_set_state(g_prNanWpaSupp, WPA_DISCONNECTED);

	/* wpa_supplicant_set_bssid() */
	/* wpa_supplicant_set_ownmac() */
}

/************************************************
*               AP Init Related                 *
*************************************************/
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
uint32_t
nan_sec_hostapd_wpa_auth_conf_group_cipher(
	struct wpa_auth_config *wconf)
{
	int32_t i4TmpGroupMgmtCipher = 0, i4TmpGroupCipher = 0;
	uint8_t fgGtk = FALSE, fgIgtk = FALSE, fgBigtk = FALSE;
	uint8_t ucCap = 0;
	uint8_t ucGtkCipherType = 0;

	ucCap = nanSecGetGroupSecurityCap(g_prAdapter, NULL);
	ucGtkCipherType = g_aucNanGtkCipherSuiteList[0];

	nanSecGetGroupCipherType(ucCap, ucGtkCipherType,
		&fgGtk, &fgIgtk, &fgBigtk,
		&i4TmpGroupCipher, &i4TmpGroupMgmtCipher);

	DBGLOG(NAN, INFO,
		"Cap:%u,Gtk:%u,Igtk:%u,Bigtk:%u,GtkCipher:%d,Igtk:%d\n",
		ucCap,
		fgGtk, fgIgtk, fgBigtk,
		i4TmpGroupCipher,
		i4TmpGroupMgmtCipher);

	wconf->wpa_group =
		(int)i4TmpGroupCipher;

	wconf->group_mgmt_cipher =
		(int)i4TmpGroupMgmtCipher;

	if (fgIgtk || fgBigtk)
		wconf->ieee80211w = MGMT_FRAME_PROTECTION_REQUIRED;
	wconf->beacon_prot = (int)fgBigtk;

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

void
nan_sec_hostapd_wpa_auth_conf(struct hostapd_bss_config *conf,
			      struct hostapd_config *iconf,
			      struct wpa_auth_config *wconf) {
	os_memset(wconf, 0, sizeof(*wconf));
	wconf->wpa = 2;
	wconf->wpa_key_mgmt = WPA_KEY_MGMT_PSK;
	wconf->wpa_pairwise = WPA_CIPHER_CCMP;
	wconf->wpa_group = WPA_CIPHER_CCMP;
	wconf->wpa_group_rekey = 600;
	/* wconf->wpa_strict_rekey = conf->wpa_strict_rekey; */
	wconf->wpa_gmk_rekey = 86400;
	/* wconf->wpa_ptk_rekey = conf->wpa_ptk_rekey; */
	wconf->rsn_pairwise = 0x10; /*CCMP*/
	wconf->eapol_version = EAPOL_VERSION;
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	nan_sec_hostapd_wpa_auth_conf_group_cipher(wconf);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
}

struct wpa_authenticator *
nan_sec_wpa_init(const u8 *addr, struct wpa_auth_config *conf,
		 struct wpa_auth_callbacks *cb, int u1BssIdx, size_t szNdiIdx) {
	struct wpa_authenticator *wpa_auth;

	DBGLOG(NAN, INFO, "Enter\n");

	if (szNdiIdx >= NAN_MAX_MULTI_NDI_NUM) {
		DBGLOG(NAN, INFO, "NDI error[%u]\n",
			szNdiIdx);
		return NULL;
	}

	wpa_auth = &g_rNanWpaAuth[szNdiIdx];
	os_memset(wpa_auth, 0, sizeof(*wpa_auth));

	nan_os_memcpyS(wpa_auth->addr,
		sizeof_field(struct wpa_authenticator, addr),
		addr, ETH_ALEN);
	nan_os_memcpyS(&wpa_auth->conf,
		sizeof_field(struct wpa_authenticator, conf),
		conf, sizeof(*conf));
	nan_os_memcpyS(&wpa_auth->cb,
		sizeof_field(struct wpa_authenticator, cb),
		cb, sizeof(*cb));

#if 1 /* TODO_CJ: GTK remove? */
	wpa_auth->group = wpa_group_init(wpa_auth, 0, 1);
	if (wpa_auth->group == NULL) {
		os_free(wpa_auth->wpa_ie);
		return NULL;
	}

#if 0 /* Disable GTK timer until SPEC update */
	if (wpa_auth->conf.wpa_gmk_rekey) {
		eloop_register_timeout(wpa_auth->conf.wpa_gmk_rekey, 0,
					   wpa_rekey_gmk, wpa_auth, NULL);
	}

	if (wpa_auth->conf.wpa_group_rekey) {
		eloop_register_timeout(wpa_auth->conf.wpa_group_rekey, 0,
					   wpa_rekey_gtk, wpa_auth, NULL);
	}
#endif

#endif

	return wpa_auth;
}

int
nan_sec_hostapd_setup_wpa(struct hostapd_data *hapd) {
	struct wpa_auth_config _conf;
	struct wpa_auth_callbacks cb;
	struct BSS_INFO *prnanBssInfo = (struct BSS_INFO *)NULL;
	u8 own_addr[ETH_ALEN] = {0};

	nan_sec_hostapd_wpa_auth_conf(hapd->conf, hapd->iconf, &_conf);
	if (hapd->iface->drv_flags & WPA_DRIVER_FLAGS_EAPOL_TX_STATUS)
		_conf.tx_status = 1;
	if (hapd->iface->drv_flags & WPA_DRIVER_FLAGS_AP_MLME)
		_conf.ap_mlme = 1;
	os_memset(&cb, 0, sizeof(cb));
	cb.ctx = hapd;

	/* cb.disconnect = hostapd_wpa_auth_disconnect; */
	/* TODO_CJ: whether need to terminate? */

	cb.get_psk = hostapd_wpa_auth_get_psk;
	cb.set_key = nan_sec_hostapd_wpa_auth_set_key;
	cb.get_seqnum =
		hostapd_wpa_auth_get_seqnum;

	cb.for_each_sta =
		hostapd_wpa_auth_for_each_sta;
		/* TODO_CJ: consider remove for GTK */

	hapd->wpa_auth =
		nan_sec_wpa_init(hapd->own_addr, &_conf, &cb,
		hapd->u1BssIdx, NAN_NDI_INDEX_0);
	if (hapd->wpa_auth == NULL) {
		DBGLOG(NAN, ERROR, "WPA initialization failed.");
		return -1;
	}

	hapd->wpa_auth->u1BssIdx = hapd->u1BssIdx;

	/* Set own mac */
	prnanBssInfo = GET_BSS_INFO_BY_INDEX(g_prAdapter,
		nanGetSpecificBssInfo(
			g_prAdapter, NAN_BSS_INDEX_MAIN)->ucBssIndex);
	COPY_MAC_ADDR(own_addr, prnanBssInfo->aucOwnMacAddr);
	hostapd_wpa_auth_set_ownmac(hapd, own_addr);

	DBGLOG(NAN, INFO, "hapd->wpa_auth->u1BssIdx:%d, hapd->u1BssIdx:%d",
		hapd->wpa_auth->u1BssIdx, hapd->u1BssIdx);

	return 0;
}

int
nan_sec_hostapd_setup_bss(struct hostapd_data *hapd) {
	hostapd_wpa_auth_hapd_data_alloc(
		hapd, nanGetSpecificBssInfo(g_prAdapter, NAN_BSS_INDEX_MAIN)
			      ->ucBssIndex);
	nan_sec_hostapd_setup_wpa(hapd);
	wpa_init_keys(hapd->wpa_auth); /* TODO_CJ: Need GTK init? */

	return 0;
}

void
nan_sec_hostapd_init(void) {
	/* nan_sec_ap_sta_init(); */

	nan_sec_hostapd_setup_bss(g_prNanHapdData);
}
void
nan_sec_hostapd_deinit(void) {
	hostapd_deinit_wpa(g_prNanHapdData);
}

/************************************************
 *               Total Init Related
 ************************************************
 */
void nan_sec_wpa_supplicant_start(struct GLUE_INFO *prGlueInfo)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
#if CFG_SUPPORT_MULTI_CARD
	struct net_device *prNetDev = prGlueInfo->prDevHandler;
	struct ADAPTER **pprAdapter = &prGlueInfo->prAdapter;
#else
	struct net_device *prNetDev = gPrDev;
	struct ADAPTER **pprAdapter = &g_prAdapter;
#endif

	DBGLOG(NAN, INFO, "Enter\n");

	/* Get prAdapter */
	prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *)netdev_priv(prNetDev);
	if (prNetDevPrivate != NULL)
		*pprAdapter = prNetDevPrivate->prGlueInfo->prAdapter;

	/* CTX */
	kalMemZero(&g_rNanSecCtx, sizeof(struct _NAN_SEC_CTX));

	/* STA */
	nan_sec_wpa_supplicant_init_iface();

	/* AP */
	kalMemZero(g_rNanWpaAuth, sizeof(g_rNanWpaAuth));
	nan_sec_hostapd_init();

#if (CFG_NAN_SEC_UT == 1)
	/* UT */
	nanSecUtMain();
#endif
}

/************************************************
 *               Export API Related
 ************************************************
 */
uint32_t
nanSecGetNdpCsidAttr(IN struct _NAN_NDP_INSTANCE_T *prNdp,
		     OUT uint32_t *pu4CsidAttrLen,
		     OUT uint8_t **ppu1CsidAttrBuf) {
	struct _NAN_SEC_CSID_ATTR_HDR *prCsidAttrHdr = NULL;
	struct _NAN_SEC_CSID_ATTR_LIST *prCsidAttrListHdr = NULL;
	uint32_t u4TotalLen = 0;
	uint8_t *pucBuf;

	DBGLOG(NAN, INFO, "Enter\n");

	u4TotalLen = sizeof(struct _NAN_SEC_CSID_ATTR_HDR) +
		     sizeof(struct _NAN_SEC_CSID_ATTR_LIST);

	pucBuf = g_aucNanSecAttrBuffer;
	kalMemZero(pucBuf, NAN_IE_BUF_MAX_SIZE);

	prCsidAttrHdr = (struct _NAN_SEC_CSID_ATTR_HDR *)pucBuf;
	prCsidAttrHdr->u1AttrId = NAN_ATTR_ID_CIPHER_SUITE_INFO;
	prCsidAttrHdr->u2AttrLen =
		sizeof(struct _NAN_SEC_CSID_ATTR_LIST) + 1;
	/* Capabilities */
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	prCsidAttrHdr->u1Cap |= nanSecGetGroupSecurityCap(g_prAdapter, prNdp);
#else
	prCsidAttrHdr->u1Cap = 0;
#endif

	/* Fill-in static Cipher list */
	prCsidAttrListHdr =
		(struct _NAN_SEC_CSID_ATTR_LIST
			 *)(pucBuf + sizeof(struct _NAN_SEC_CSID_ATTR_HDR));
	prCsidAttrListHdr->u1CipherType = prNdp->ucCipherType;
	prCsidAttrListHdr->u1PublishId = prNdp->ucPublishId;

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	if (prNdp->fgTxGtkRequired && prNdp->ucTxGtkCipherType) {
		prCsidAttrListHdr = prCsidAttrListHdr + 1;
		prCsidAttrListHdr->u1CipherType = prNdp->ucTxGtkCipherType;
		prCsidAttrListHdr->u1PublishId = prNdp->ucPublishId;
		prCsidAttrHdr->u2AttrLen +=
			sizeof(struct _NAN_SEC_CSID_ATTR_LIST);
		u4TotalLen += sizeof(struct _NAN_SEC_CSID_ATTR_LIST);
	}
	DBGLOG(NAN, INFO,
	       "GtkReq:%u, Cipher:%u, szTotalLen:%lu\n",
	       prNdp->fgTxGtkRequired, prNdp->ucTxGtkCipherType, u4TotalLen);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

	*ppu1CsidAttrBuf = pucBuf;
	*pu4CsidAttrLen = u4TotalLen;

	DBGLOG(NAN, INFO,
	       "output, *ppu1CsidAttrBuf:0x%p,  *pu4CsidAttrLen:%d\n",
	       *ppu1CsidAttrBuf, *pu4CsidAttrLen);
#if (ENABLE_SEC_UT_LOG == 1)
	dumpMemory8((uint8_t *)prCsidAttrHdr, u4TotalLen);
#endif

	return 0;
}

uint32_t
nanSecGetNdpScidAttr(IN struct _NAN_NDP_INSTANCE_T *prNdp,
		     OUT uint32_t *pu4ScidAttrLen,
		     OUT uint8_t **ppu1ScidAttrBuf) {
	struct _NAN_SEC_SCID_ATTR_HDR *prScidAttrHdr = NULL;
	struct _NAN_SEC_SCID_ATTR_ENTRY *pr1ScidAttrListHdr = NULL;
	uint32_t u4TotalLen = 0;
	uint8_t *pu1ScidPtr = NULL;
	uint8_t *pucBuf;

	DBGLOG(NAN, INFO, "Enter\n");

	u4TotalLen = sizeof(struct _NAN_SEC_SCID_ATTR_HDR) +
		     sizeof(struct _NAN_SEC_SCID_ATTR_ENTRY) +
		     sizeof(prNdp->au1Scid);

	pucBuf = g_aucNanSecAttrBuffer;
	kalMemZero(pucBuf, NAN_IE_BUF_MAX_SIZE);

	prScidAttrHdr = (struct _NAN_SEC_SCID_ATTR_HDR *)pucBuf;
	prScidAttrHdr->u1AttrId = NAN_ATTR_ID_SECURITY_CONTEXT_INFO;
	prScidAttrHdr->u2AttrLen =
		u4TotalLen - sizeof(struct _NAN_SEC_SCID_ATTR_HDR);

	pr1ScidAttrListHdr =
		(struct _NAN_SEC_SCID_ATTR_ENTRY
			 *)(pucBuf + sizeof(struct _NAN_SEC_SCID_ATTR_HDR));
	pr1ScidAttrListHdr->u2ScidLen = sizeof(prNdp->au1Scid);
	pr1ScidAttrListHdr->u1ScidType = 1; /* PMKID */
	pr1ScidAttrListHdr->u1PublishId = prNdp->ucPublishId;

	/* pu1ScidPtr = &pr1ScidAttrListHdr->u1PublishId + 1; */
	pu1ScidPtr = pucBuf +
		sizeof(struct _NAN_SEC_SCID_ATTR_HDR) +
		sizeof(struct _NAN_SEC_SCID_ATTR_ENTRY);
	kalMemCpyS(pu1ScidPtr,
		   NAN_IE_BUF_MAX_SIZE - sizeof(struct _NAN_SEC_SCID_ATTR_HDR) -
		   sizeof(struct _NAN_SEC_SCID_ATTR_ENTRY),
		   prNdp->au1Scid, sizeof(prNdp->au1Scid));

	*ppu1ScidAttrBuf = pucBuf;
	*pu4ScidAttrLen = u4TotalLen;

	DBGLOG(NAN, INFO,
	       "output, *ppu1ScidAttrBuf:0x%p,  *pu4ScidAttrLen:%d\n",
	       *ppu1ScidAttrBuf, *pu4ScidAttrLen);

#if (ENABLE_SEC_UT_LOG == 1)
	dumpMemory8((uint8_t *)prScidAttrHdr, u4TotalLen);
#endif

	return 0;
}

uint32_t
nanSecGetCsidAttr(uint32_t *pu4CsidAttrLen, uint8_t **ppu1CsidAttrBuf) {
	struct _NAN_SEC_CSID_ATTR_HDR *prCsidAttrHdr = NULL;
	struct _NAN_SEC_CSID_ATTR_LIST *prCsidAttrListHdr = NULL;
	uint32_t u4TotalLen = 0;
	uint32_t u4CipherListLen = 0;
	struct _NAN_SEC_CIPHER_ENTRY *prCipherEntry = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	u4CipherListLen = sizeof(struct _NAN_SEC_CSID_ATTR_LIST) *
			  (g_rNanSecCtx.rNanSecCipherList.u4NumElem);
	u4TotalLen = sizeof(struct _NAN_SEC_CSID_ATTR_HDR) + u4CipherListLen;

#if (ENABLE_SEC_UT_LOG == 1)
	DBGLOG(NAN, INFO,
	       "len_ATTR_LIST:%d, len_ATTR_HDR:%d, u4NumElem:%d, u4CipherListLen:%d, u4TotalLen:%d\n",
	       sizeof(struct _NAN_SEC_CSID_ATTR_LIST),
	       sizeof(struct _NAN_SEC_CSID_ATTR_HDR),
	       (g_rNanSecCtx.rNanSecCipherList.u4NumElem), u4CipherListLen,
	       u4TotalLen);
#endif

	/* g_rNanSecCtx.pu1CsidAttrBuf = os_zalloc(u4TotalLen); */
	kalMemZero(g_aucNanSecAttrBuffer, NAN_IE_BUF_MAX_SIZE);
	g_rNanSecCtx.pu1CsidAttrBuf = g_aucNanSecAttrBuffer;

	if (g_rNanSecCtx.pu1CsidAttrBuf == NULL) {
		DBGLOG(NAN, ERROR,
		       "ERROR! os_zalloc failed for pu1CsidAttrBuf\n");
		return -1;
	}
	g_rNanSecCtx.u4CsidAttrLen = u4TotalLen;

	prCsidAttrHdr =
		(struct _NAN_SEC_CSID_ATTR_HDR *)g_rNanSecCtx.pu1CsidAttrBuf;
	prCsidAttrHdr->u1AttrId = NAN_ATTR_ID_CIPHER_SUITE_INFO;
	prCsidAttrHdr->u2AttrLen = u4CipherListLen + 1;
	/* Capabilities */
	prCsidAttrHdr->u1Cap = 0;

	/* Fill-in static Cipher list */
	prCsidAttrListHdr = (struct _NAN_SEC_CSID_ATTR_LIST
				     *)(g_rNanSecCtx.pu1CsidAttrBuf +
					sizeof(struct _NAN_SEC_CSID_ATTR_HDR));
	prCipherEntry = (struct _NAN_SEC_CIPHER_ENTRY *)QUEUE_GET_HEAD(
		&g_rNanSecCtx.rNanSecCipherList);

	while (prCipherEntry != NULL) {
		prCsidAttrListHdr->u1CipherType = prCipherEntry->u4CipherType;
		prCsidAttrListHdr->u1PublishId = prCipherEntry->u2PublishId;

		prCsidAttrListHdr =
			prCsidAttrListHdr +
			1; /* sizeof(struct _NAN_SEC_CSID_ATTR_LIST) */
		prCipherEntry =
			(struct _NAN_SEC_CIPHER_ENTRY *)QUEUE_GET_NEXT_ENTRY(
				&prCipherEntry->rQueEntry);
	}

	*ppu1CsidAttrBuf = g_rNanSecCtx.pu1CsidAttrBuf;
	*pu4CsidAttrLen = g_rNanSecCtx.u4CsidAttrLen;

	DBGLOG(NAN, INFO,
	       "output, *ppu1CsidAttrBuf:0x%p,  *pu4CsidAttrLen:%d\n",
	       *ppu1CsidAttrBuf, *pu4CsidAttrLen);

#if (ENABLE_SEC_UT_LOG == 1)
	dumpMemory8((uint8_t *)prCsidAttrHdr, u4TotalLen);
#endif

	return 0;
}

uint32_t
nanSecInsertCipherList(IN uint32_t u4CipherType, IN uint16_t u2PublishId) {
	struct _NAN_SEC_CIPHER_ENTRY *prCipherEntry = NULL;

	DBGLOG(NAN, INFO, "Enter, u4CipherType:0x%x, u2PublishId:0x%x\n",
	       u4CipherType, u2PublishId);

	if (u4CipherType == 0)
		return WLAN_STATUS_NOT_ACCEPTED;

	/* Duplicate case handling */
	prCipherEntry = (struct _NAN_SEC_CIPHER_ENTRY *)QUEUE_GET_HEAD(
		&g_rNanSecCtx.rNanSecCipherList);

	while (prCipherEntry != NULL) {
		if (prCipherEntry->u2PublishId == u2PublishId) {
			DBGLOG(NAN, INFO,
			       "Find duplicate, old u4CipherType:0x%x, old u2PublishId:0x%x\n",
			       prCipherEntry->u4CipherType,
			       prCipherEntry->u2PublishId);

			prCipherEntry->u4CipherType = u4CipherType;
			return 0;
		}

		prCipherEntry =
			(struct _NAN_SEC_CIPHER_ENTRY *)QUEUE_GET_NEXT_ENTRY(
				&prCipherEntry->rQueEntry);
	}

	/* Insert the new one */
	prCipherEntry = (struct _NAN_SEC_CIPHER_ENTRY *)os_zalloc(
		sizeof(struct _NAN_SEC_CIPHER_ENTRY));
	if (prCipherEntry != NULL) {
		prCipherEntry->u4CipherType = u4CipherType;
		prCipherEntry->u2PublishId = u2PublishId;

		QUEUE_INSERT_TAIL(&g_rNanSecCtx.rNanSecCipherList,
				  &prCipherEntry->rQueEntry);
	} else {
		DBGLOG(NAN, ERROR, "os_zalloc failed for prCipherEntry\n");
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSecFlushCipherList(void) {
	struct _NAN_SEC_CIPHER_ENTRY *prCipherEntry = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	while (QUEUE_IS_NOT_EMPTY(&g_rNanSecCtx.rNanSecCipherList)) {
		QUEUE_REMOVE_HEAD(&g_rNanSecCtx.rNanSecCipherList,
				  prCipherEntry,
				  struct _NAN_SEC_CIPHER_ENTRY *);
		if (prCipherEntry != NULL) {
			os_free(prCipherEntry);
			prCipherEntry = NULL;
		} else {
			DBGLOG(NAN, WARN,
			       "rNanSecCipherList is not empty but dequeue nothing, num:%d\n",
			       g_rNanSecCtx.rNanSecCipherList.u4NumElem);
			return -1;
		}
	}

	return 0;
}

uint32_t
nanSecSetCipherType(IN struct _NAN_NDP_INSTANCE_T *prNdp,
		    IN uint32_t u4CipherType) {
	/* UINT_8  i; */
	int32_t i4TmpKeyMgmt = 0, i4TmpCipher = 0, i4TmpProto = 0,
	       i4TmpAuthAlg = 0, i4TmpKeyInfo = 0;

	DBGLOG(NAN, INFO, "Enter, eNDPRole:%d, u4CipherType:%d\n",
	       prNdp->eNDPRole, u4CipherType);

	/* Select chipher suit */
	switch (u4CipherType) {
	case NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128:
		i4TmpKeyMgmt = WPA_KEY_MGMT_PSK_SHA256;
		i4TmpCipher = WPA_CIPHER_CCMP;
		i4TmpProto = WPA_PROTO_RSN;
		i4TmpAuthAlg = WPA_AUTH_ALG_OPEN;
		/* Seems not necessary */
		i4TmpKeyInfo =
			WPA_KEY_INFO_TYPE_AES_128_CMAC;
		break;

	case NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256:
		i4TmpKeyMgmt = WPA_KEY_MGMT_PSK_SHA384;
		i4TmpCipher = WPA_CIPHER_GCMP_256;
		i4TmpProto = WPA_PROTO_RSN;
		i4TmpAuthAlg = WPA_AUTH_ALG_OPEN;
		/* Seems not necessary */
		i4TmpKeyInfo =
			WPA_KEY_INFO_TYPE_AES_128_CMAC;
		break;

	default:
		u4CipherType = NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128;
		i4TmpKeyMgmt = WPA_KEY_MGMT_PSK_SHA256;
		i4TmpCipher = WPA_CIPHER_CCMP;
		i4TmpProto = WPA_PROTO_RSN;
		i4TmpAuthAlg = WPA_AUTH_ALG_OPEN;
		/* Seems not necessary */
		i4TmpKeyInfo =
			WPA_KEY_INFO_TYPE_AES_128_CMAC;

		break;
	}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	/* Authenticator */
	prNdp->prInitiatorSecSmInfo->u4SelCipherType = u4CipherType;
	prNdp->prInitiatorSecSmInfo->wpa_key_mgmt = i4TmpKeyMgmt;
	prNdp->prInitiatorSecSmInfo->pairwise = i4TmpCipher;
	prNdp->prInitiatorSecSmInfo->wpa = WPA_VERSION_WPA2;
	/* Supplicant */
	prNdp->prResponderSecSmInfo->u4SelCipherType = u4CipherType;
	prNdp->prResponderSecSmInfo->key_mgmt = (unsigned int)i4TmpKeyMgmt;
	prNdp->prResponderSecSmInfo->pairwise_cipher =
		(unsigned int)i4TmpCipher;
	prNdp->prResponderSecSmInfo->proto = (unsigned int)i4TmpProto;
#else
	/* Assign into state machine */
	if (prNdp->eNDPRole ==
	    NAN_PROTOCOL_INITIATOR) {
	    /* TODO: integrate with nan_base defines */
		prNdp->prInitiatorSecSmInfo->u4SelCipherType = u4CipherType;
		prNdp->prInitiatorSecSmInfo->wpa_key_mgmt = i4TmpKeyMgmt;
		prNdp->prInitiatorSecSmInfo->pairwise = i4TmpCipher;
		prNdp->prInitiatorSecSmInfo->wpa = WPA_VERSION_WPA2;
	} else {
		prNdp->prResponderSecSmInfo->u4SelCipherType = u4CipherType;
		prNdp->prResponderSecSmInfo->key_mgmt = i4TmpKeyMgmt;
		prNdp->prResponderSecSmInfo->pairwise_cipher = i4TmpCipher;
		prNdp->prResponderSecSmInfo->proto = i4TmpProto;
	}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
	return 0;
}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
u_int8_t
nanSecIsGroupCipherSupported(uint8_t ucGtkCipherType)
{
	size_t szGtkCipherIdx = 0;

	for (szGtkCipherIdx = 0;
		szGtkCipherIdx < NAN_MAX_GTK_CIPHER_SUITE_NUM;
		szGtkCipherIdx++) {
		if (ucGtkCipherType ==
			g_aucNanGtkCipherSuiteList[szGtkCipherIdx])
			break;
	}

	if (szGtkCipherIdx >= NAN_MAX_GTK_CIPHER_SUITE_NUM) {
		DBGLOG(NAN, ERROR,
			"Unsupported Gtk cipher: %u\n",
			ucGtkCipherType);
		return FALSE;
	}

	return TRUE;
}

uint8_t
nanSecIsDevSupportGroupSecurity(IN struct ADAPTER *prAdapter)
{
	struct WIFI_VAR *prWifiVar = NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "Null prAdapter\n");
		return FALSE;
	}

	prWifiVar = &prAdapter->rWifiVar;

	if ((prWifiVar->ucNanGroupSecCap ==
		CSIA_CAP_GTKSA_IGTKSA_SUPPORTED_BIGTKSA_UNSUPPORTED) ||
		(prWifiVar->ucNanGroupSecCap ==
		CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORTED))
		return TRUE;

	return FALSE;
}

uint8_t
nanSecGetGroupSecurityCap(IN struct ADAPTER *prAdapter,
	struct _NAN_NDP_INSTANCE_T *prNDP)
{
	struct WIFI_VAR *prWifiVar = NULL;
	uint8_t ucSecurityCap = 0;
	uint8_t ucGroupSASupport = 0;
	uint8_t ucIgtkCipher = 0;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "Null prAdapter\n");
		return 0;
	}

	prWifiVar = &prAdapter->rWifiVar;

	if ((prWifiVar->ucNanGroupSecCap >=
		CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_RESERVED))
		ucGroupSASupport = CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_UNSUPPORTED;
	else
		ucGroupSASupport = prWifiVar->ucNanGroupSecCap;

	if ((ucGroupSASupport ==
		CSIA_CAP_GTKSA_IGTKSA_SUPPORTED_BIGTKSA_UNSUPPORTED) ||
		(ucGroupSASupport ==
		CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORTED)) {
		ucIgtkCipher =
			((prWifiVar->ucNanIgtkCipher <<
			CSIA_CAP_IGTKSA_BIGTKSA_CS_SHFT)
			& CSIA_CAP_IGTKSA_BIGTKSA_CS_MASK);
	}

	ucGroupSASupport =
		((ucGroupSASupport <<
			CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORT_SHFT)
		& CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORT_MASK);

	ucSecurityCap = (ucGroupSASupport | ucIgtkCipher);

	return ucSecurityCap;
}

uint8_t
nanSecGetCipherWpaToHw(IN uint16_t u2GroupCipher)
{
	switch (u2GroupCipher) {
	case WPA_CIPHER_CCMP:
		return CIPHER_SUITE_CCMP;
	case WPA_CIPHER_GCMP_256:
		return CIPHER_SUITE_GCMP_256;
	case WPA_CIPHER_AES_128_CMAC:
		return CIPHER_SUITE_BIP;
	case WPA_CIPHER_BIP_GMAC_256:
		return CIPHER_SUITE_BIP_GMAC_256;
	default:
		return CIPHER_SUITE_NONE;
	}
}

uint32_t
nanSecGetGroupCipherType(
	IN uint8_t ucCap, uint8_t ucGtkCipherType,
	OUT u_int8_t *pfgGtk, OUT u_int8_t *pfgIgtk, OUT u_int8_t *pfgBigtk,
	OUT int32_t *pi4TmpGroupCipher, OUT int32_t *pi4TmpGroupMgmtCipher)
{
	uint8_t ucGroupSA = 0;
	uint8_t ucBigtkCipher = 0;

	if ((pfgGtk == NULL) || (pfgIgtk == NULL) || (pfgBigtk == NULL) ||
		(pi4TmpGroupCipher == NULL) ||
		(pi4TmpGroupMgmtCipher == NULL)) {
		DBGLOG(NAN, ERROR, "Null input\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	ucGroupSA = (ucCap &
		CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORT_MASK)
		>> CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORT_SHFT;
	ucBigtkCipher = (ucCap &
		CSIA_CAP_IGTKSA_BIGTKSA_CS_MASK)
		>> CSIA_CAP_IGTKSA_BIGTKSA_CS_SHFT;

	if (ucGroupSA == CSIA_CAP_GTKSA_IGTKSA_BIGTKSA_SUPPORTED) {
		*pfgGtk = TRUE;
		*pfgIgtk = TRUE;
		*pfgBigtk = TRUE;
	} else if (ucGroupSA ==
		CSIA_CAP_GTKSA_IGTKSA_SUPPORTED_BIGTKSA_UNSUPPORTED) {
		*pfgGtk = TRUE;
		*pfgIgtk = TRUE;
		*pfgBigtk = FALSE;
	}

	/* Select chipher suit */
	if (*pfgGtk) {
		if (ucGtkCipherType ==
			NAN_CIPHER_SUITE_ID_NCS_GTK_GCM_256)
			*pi4TmpGroupCipher = WPA_CIPHER_GCMP_256;
		else if (ucGtkCipherType ==
			NAN_CIPHER_SUITE_ID_NCS_GTK_CCM_128)
			*pi4TmpGroupCipher = WPA_CIPHER_CCMP;

	} else {
		*pi4TmpGroupCipher = WPA_CIPHER_GTK_NOT_USED;
	}

	if (*pfgIgtk || *pfgBigtk) {
		if (ucBigtkCipher ==
			CSIA_CAP_IGTKSA_BIGTKSA_NCS_BIP_256) {
			*pi4TmpGroupMgmtCipher = WPA_CIPHER_BIP_GMAC_256;
		} else {
			*pi4TmpGroupMgmtCipher = WPA_CIPHER_AES_128_CMAC;
		}
	} else {
		*pi4TmpGroupMgmtCipher = WPA_CIPHER_NONE;
	}

	return WLAN_STATUS_SUCCESS;
}
uint32_t
nanSecSetGroupCipherType(
	IN struct _NAN_NDP_INSTANCE_T *prNdp, u_int8_t fgIsTx)
{
	int32_t i4TmpGroupCipher = 0, i4TmpGroupMgmtCipher = 0;
	u_int8_t fgGtk = FALSE, fgIgtk = FALSE, fgBigtk = FALSE;
	uint8_t ucCap = 0;
	uint8_t ucGtkCipherType = 0;
	struct wpa_auth_config *conf =  NULL;
	struct wpa_sm *wpaSm = NULL;
	struct wpa_state_machine *wpaStateMachine = NULL;

	if (fgIsTx) {
		ucGtkCipherType = prNdp->ucTxGtkCipherType;
		ucCap = prNdp->ucTxSecurityCap;
	} else {
		ucGtkCipherType = prNdp->ucRxGtkCipherType;
		ucCap = prNdp->ucRxSecurityCap;
	}

	nanSecGetGroupCipherType(ucCap, ucGtkCipherType,
		&fgGtk, &fgIgtk, &fgBigtk,
		&i4TmpGroupCipher, &i4TmpGroupMgmtCipher);

	DBGLOG(NAN, INFO,
		"Cap:%u,Tx:%u,Gtk:%u,Igtk:%u,Bigtk:%u,GtkCipher:%d,%d,Igtk:%d\n",
		ucCap, fgIsTx,
		fgGtk, fgIgtk, fgBigtk,
		ucGtkCipherType, i4TmpGroupCipher,
		i4TmpGroupMgmtCipher);

	/* Assign into state machine */
	if (fgIsTx) {
		/* group data protection use NDI configuration */
		/* AP TODO */
		conf = &g_rNanWpaAuth[NAN_NDI_INDEX_0].conf;
		wpaStateMachine = prNdp->prInitiatorSecSmInfo;

		/* Disable GTK if not supported or not required */
		if ((fgGtk == 0) || (prNdp->fgTxGtkRequired == 0)) {
			prNdp->fgTxGtkRequired = 0;
		} else {
			if (conf->wpa_group != (int)i4TmpGroupCipher) {
				DBGLOG(NAN, ERROR,
					"Group Cipher mismatch %d, %d\n",
					conf->wpa_group, i4TmpGroupCipher);
			}
		}
		if (fgIgtk || fgBigtk)
			wpaStateMachine->mgmt_frame_prot = 1;
		else
			wpaStateMachine->mgmt_frame_prot = 0;
	} else {
		wpaSm = prNdp->prResponderSecSmInfo;
		wpaSm->group_cipher =
			(unsigned int)i4TmpGroupCipher;
		wpaSm->mgmt_group_cipher =
			(unsigned int)i4TmpGroupMgmtCipher;
		if (fgBigtk)
			wpaSm->beacon_prot = 1;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSecSetGroupSA(
	IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	struct wpa_auth_config *nmiConf =  NULL;
	struct wpa_sm *wpaSm = NULL;
	struct wpa_state_machine *wpaStateMachine = NULL;
	u_int8_t fgUseGTK = FALSE;

	if (!prNdp)
		return WLAN_STATUS_FAILURE;

	wpaSm = prNdp->prResponderSecSmInfo;
	wpaStateMachine = prNdp->prInitiatorSecSmInfo;
	if (!wpaStateMachine || !wpaSm)
		return WLAN_STATUS_FAILURE;

	/* mgmt and beacon protection always use NMI configuration */
	nmiConf = &g_rNanWpaAuth[NAN_NDI_INDEX_0].conf;

	if (prNdp->fgTxGtkRequired && prNdp->fgRxGtkRequired) {
		prNdp->ucGroupSAUsed |= NAN_GROUP_SECURITY_ASSOC_USE_GTKSA;
		fgUseGTK = TRUE;
	} else {
		prNdp->ucGroupSAUsed &= ~NAN_GROUP_SECURITY_ASSOC_USE_GTKSA;
	}

	if ((nmiConf->ieee80211w != NO_MGMT_FRAME_PROTECTION) &&
		(wpaSm->mgmt_group_cipher &&
		(wpaSm->mgmt_group_cipher != WPA_CIPHER_NONE)) &&
		fgUseGTK)
		prNdp->ucGroupSAUsed |= NAN_GROUP_SECURITY_ASSOC_USE_IGTKSA;
	else
		prNdp->ucGroupSAUsed &= ~NAN_GROUP_SECURITY_ASSOC_USE_IGTKSA;

	if ((nmiConf->beacon_prot && wpaSm->beacon_prot) && fgUseGTK)
		prNdp->ucGroupSAUsed |= NAN_GROUP_SECURITY_ASSOC_USE_BIGTKSA;
	else
		prNdp->ucGroupSAUsed &= ~NAN_GROUP_SECURITY_ASSOC_USE_BIGTKSA;

	DBGLOG(NAN, INFO,
		"GtkReq:%u,%d,Mgmt:%u,0x%x,BcnProt:%d,%d,GroupSA:0x%x\n",
		prNdp->fgTxGtkRequired, prNdp->fgRxGtkRequired,
		nmiConf->ieee80211w, wpaSm->mgmt_group_cipher,
		nmiConf->beacon_prot, wpaSm->beacon_prot,
		prNdp->ucGroupSAUsed);
	return WLAN_STATUS_SUCCESS;
}

uint32_t
nan_sec_ndi_wpa_auth_conf_gtk_cipher(
	struct wpa_auth_config *wconf,
	uint8_t ucGtkCipherType)
{
	int32_t i4TmpGroupCipher = 0;

	if (ucGtkCipherType ==
		NAN_CIPHER_SUITE_ID_NCS_GTK_GCM_256)
		i4TmpGroupCipher = WPA_CIPHER_GCMP_256;
	else if (ucGtkCipherType ==
		NAN_CIPHER_SUITE_ID_NCS_GTK_CCM_128)
		i4TmpGroupCipher = WPA_CIPHER_CCMP;
	else
		i4TmpGroupCipher = WPA_CIPHER_NONE;

	wconf->wpa_group =
		(int)i4TmpGroupCipher;

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

int32_t
nan_sec_ndi_wpa_auth_init(size_t szNdiIdx, uint8_t ucGtkCipherType)
{
	struct wpa_authenticator *wpa_auth = NULL;
	struct wpa_auth_config conf = {0};
	struct wpa_auth_callbacks cb = {0};
	size_t szBssIdx = 0;
	uint8_t ucNDIAddr[MAC_ADDR_LEN] = {0};

	DBGLOG(NAN, INFO, "szNdiIdx[%u]\n", szNdiIdx);

	wpa_auth = &g_rNanWpaAuth[szNdiIdx];

	/* skip if wpa_auth already initialized */
	if (kalMemCmp(wpa_auth->addr, ucNDIAddr, MAC_ADDR_LEN) != 0)
		return 0;

	szBssIdx = nanGetSpecificBssInfo(g_prAdapter,
		NAN_BSS_INDEX_MAIN)->ucBssIndex;
	nanDevGetNdiAddress(g_prAdapter,
		(uint8_t)szNdiIdx,
		ucNDIAddr);

	cb.set_key = nan_sec_hostapd_wpa_auth_set_key;
	cb.get_seqnum =
		hostapd_wpa_auth_get_seqnum;

	cb.for_each_sta =
		hostapd_wpa_auth_for_each_sta;

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	nan_sec_ndi_wpa_auth_conf_gtk_cipher(&conf, ucGtkCipherType);
	conf.group_mgmt_cipher = (int)WPA_CIPHER_NONE;
	conf.ieee80211w = NO_MGMT_FRAME_PROTECTION;
	conf.beacon_prot = 0;
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
	conf.wpa = 2;

	wpa_auth = nan_sec_wpa_init(
				ucNDIAddr, &conf, &cb,
				szBssIdx, szNdiIdx);
	if (wpa_auth == NULL) {
		DBGLOG(NAN, ERROR, "WPA initialization failed.");
		return -1;
	}
	wpa_auth->u1BssIdx = szBssIdx;

	wpa_init_keys(wpa_auth);

	return 0;
}

uint32_t
nanSecSetPmk(IN struct _NAN_NDP_INSTANCE_T *prNdp, IN uint32_t u4PmkLen,
	     IN uint8_t *pu1Pmk)
{
	DBGLOG(NAN, INFO, "Enter, u4PmkLen:%d, eNDPRole:%d\n",
	       u4PmkLen, prNdp->eNDPRole);

	if (u4PmkLen == PMK_LEN) {
		if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
			kalMemZero(prNdp->prInitiatorSecSmInfo->au1Psk,
				   PMK_LEN);
			kalMemCopy(prNdp->prInitiatorSecSmInfo->au1Psk, pu1Pmk,
				   u4PmkLen);
			prNdp->prInitiatorSecSmInfo->u4PskLen = u4PmkLen;
		} else {
			kalMemZero(prNdp->prResponderSecSmInfo->au1Psk,
				   PMK_LEN);
			kalMemCopy(prNdp->prResponderSecSmInfo->au1Psk, pu1Pmk,
				   u4PmkLen);
			prNdp->prResponderSecSmInfo->u4PskLen = u4PmkLen;
		}
	} else {
		/* PKCS5_PBKDF2_HMAC */
	}

	return 0;
}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)

uint32_t
nanSecNotify4wayBegin(IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	struct wpa_authenticator *wpa_auth = NULL;

	DBGLOG(NAN, INFO, "eNDPRole:%u, NDPID:%d\n",
	       prNdp->eNDPRole, prNdp->ucNDPID);

	/* Update wpa state machine to both authenticator and supplicant */
	g_prNanWpaSupp->wpa = prNdp->prResponderSecSmInfo;
	nanSecUpdatePmk(prNdp);

	/* authenticator */
	prNdp->prInitiatorSecSmInfo->u1MicCalState =
		NAN_SEC_MIC_CAL_IDLE;
	prNdp->prInitiatorSecSmInfo->wpa_auth_nmi =
		&g_rNanWpaAuth[NAN_NDI_INDEX_0];

	wpa_auth = prNdp->prInitiatorSecSmInfo->wpa_auth
		= &g_rNanWpaAuth[NAN_NDI_INDEX_0];

	prNdp->prInitiatorSecSmInfo->pvNdp = (void *)prNdp;
	wpa_auth->pvNdp = (void *)prNdp;

	nanSecSetGroupSA(prNdp);

	wpa_auth_sta_init(wpa_auth,
			  prNdp->prInitiatorSecSmInfo->addr, NULL);

	hostapd_wpa_auth_set_bssid(
		g_prNanHapdData,
		nanGetSpecificBssInfo(g_prAdapter, NAN_BSS_INDEX_MAIN)
			->aucClusterId);

	kalMemCpyS(wpa_auth->addr,
		sizeof_field(struct wpa_authenticator, addr),
		prNdp->aucLocalNDIAddr,
		sizeof_field(struct _NAN_NDP_INSTANCE_T, aucLocalNDIAddr));

	kalMemCpyS(prNdp->prInitiatorSecSmInfo->addr,
		sizeof_field(struct wpa_state_machine, addr),
		prNdp->aucPeerNDIAddr,
		sizeof_field(struct _NAN_NDP_INSTANCE_T, aucPeerNDIAddr));

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		wpa_auth_sta_associated(wpa_auth,
					prNdp->prInitiatorSecSmInfo);
	} else if (prNdp->eNDPRole == NAN_PROTOCOL_RESPONDER) {
		/* Re-initialize GTK for responder
		 * (initiator is done in
		 * wpa_auth_sta_associated -> ...
		 * -> SM_STATE(WPA_PTK, AUTHENTICATION2)
		 */
		if (prNdp->fgTxGtkRequired) {
			wpa_group_ensure_init(wpa_auth,
				prNdp->prInitiatorSecSmInfo->group);
		}
	}

	/* supplicant */
	prNdp->prResponderSecSmInfo->u1MicCalState =
		NAN_SEC_MIC_CAL_IDLE;
	prNdp->prResponderSecSmInfo->pvNdp = (void *)prNdp;

	nanSecUpdatePeerNDI(prNdp, prNdp->aucPeerNDIAddr);

	/* TODO_CJ: concurrent 4-way */
	wpa_supplicant_set_ownmac(g_prNanWpaSupp,
				  prNdp->aucLocalNDIAddr);

	nan_sec_wpa_sm_init(&g_rNanWpaSmCtx,
				prNdp); /* In Trooper, only 1 sta sm */

	/* Sigma workaround: g_prNanWpaSupp is not assigned yet */
	wpa_supplicant_set_bssid(g_prNanWpaSupp, prNdp->aucPeerNDIAddr);

	wpa_SYSrand_Gen_Rand_Seed(prNdp->aucLocalNDIAddr);

	return 0;
}

uint32_t
nanSecNotify4wayTerminate(IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	DBGLOG(NAN, INFO, "Enter, eNDPRole:%u, NDPID:%d\n",
	       prNdp->eNDPRole, prNdp->ucNDPID);

	/* wpa sm back to disconnect */
	prNdp->prInitiatorSecSmInfo->Disconnect = TRUE;

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		wpa_sm_step(prNdp->prInitiatorSecSmInfo);

		/* Original clean up */
		wpa_auth_sta_deinit(prNdp->prInitiatorSecSmInfo);
	}

	/* NAN clean up */
	nanSecApSmBufReset(prNdp->prInitiatorSecSmInfo);

	/* Keep NDP index info */
	prNdp->prInitiatorSecSmInfo->pvNdp = (void *)prNdp;

	os_free(g_prNanHapdData->conf->ssid.wpa_psk);
	g_prNanHapdData->conf->ssid.wpa_psk = NULL;

	/* Original clean up */
	if (g_prNanWpaSupp == NULL || g_prNanWpaSupp->wpa == NULL) {
		DBGLOG(NAN, ERROR,
			"g_prNanWpaSupp is NULL\n");
		return 0;
	}

	g_prNanWpaSupp->wpa->rx_replay_counter_set = 0;
	nan_os_memset(g_prNanWpaSupp->wpa->rx_replay_counter, 0,
		  WPA_REPLAY_COUNTER_LEN);
	g_prNanWpaSupp->wpa->msg_3_of_4_ok = 0;

	g_prNanWpaSupp->wpa->ptk_set = 0;
	nan_os_memset(&g_prNanWpaSupp->wpa->ptk, 0,
		  sizeof(g_prNanWpaSupp->wpa->ptk));
	g_prNanWpaSupp->wpa->tptk_set = 0;
	nan_os_memset(&g_prNanWpaSupp->wpa->tptk, 0,
		  sizeof(g_prNanWpaSupp->wpa->tptk));
	nan_os_memset(&g_prNanWpaSupp->wpa->gtk, 0,
		  sizeof(g_prNanWpaSupp->wpa->gtk));

	/* NAN clean up */
	nanSecStaSmBufReset(prNdp->prResponderSecSmInfo);
	kalMemZero(prNdp->prResponderSecSmInfo, sizeof(struct wpa_sm));

	/* Keep NDP index info */
	prNdp->prResponderSecSmInfo->pvNdp = (void *)prNdp;

	/* Common */

	return 0;
}

#else
uint32_t
nanSecNotify4wayBegin(IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	DBGLOG(NAN, INFO, "Enter, eNDPRole:%d, NDPID:%d\n",
	       prNdp->eNDPRole, prNdp->ucNDPID);

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		nanSecUpdatePmk(prNdp);

		prNdp->prInitiatorSecSmInfo->u1MicCalState =
			NAN_SEC_MIC_CAL_IDLE;
		prNdp->prInitiatorSecSmInfo->wpa_auth =
			&g_rNanWpaAuth[NAN_NDI_INDEX_0];
		prNdp->prInitiatorSecSmInfo->pvNdp = (void *)prNdp;
		prNdp->prInitiatorSecSmInfo->wpa_auth->pvNdp = (void *)prNdp;

		wpa_auth_sta_init(prNdp->prInitiatorSecSmInfo->wpa_auth,
				  prNdp->prInitiatorSecSmInfo->addr, NULL);

		hostapd_wpa_auth_set_bssid(
			g_prNanHapdData,
			nanGetSpecificBssInfo(g_prAdapter, NAN_BSS_INDEX_MAIN)
				->aucClusterId);

		/* TODO_CJ: concurrent 4-way */
		hostapd_wpa_auth_set_ownmac(g_prNanHapdData,
					    prNdp->aucLocalNDIAddr);
		kalMemCopy(prNdp->prInitiatorSecSmInfo->addr,
			   prNdp->aucPeerNDIAddr, MAC_ADDR_LEN);

		wpa_auth_sta_associated(&g_rNanWpaAuth[NAN_NDI_INDEX_0],
					prNdp->prInitiatorSecSmInfo);
		} else {
		/* NAN_NDP_RESPONDER */
		prNdp->prResponderSecSmInfo->u1MicCalState =
			NAN_SEC_MIC_CAL_IDLE;
		prNdp->prResponderSecSmInfo->pvNdp = (void *)prNdp;

		g_prNanWpaSupp->wpa = prNdp->prResponderSecSmInfo;

		/* wpa_supplicant_set_bssid(
		 *	g_prNanWpaSupp, prNdp->aucPeerNDIAddr);
		 */
		nanSecUpdatePeerNDI(prNdp, prNdp->aucPeerNDIAddr);

		/* TODO_CJ: concurrent 4-way */
		wpa_supplicant_set_ownmac(g_prNanWpaSupp,
					  prNdp->aucLocalNDIAddr);

		nan_sec_wpa_sm_init(&g_rNanWpaSmCtx,
				    prNdp); /* In Trooper, only 1 sta sm */

		nanSecUpdatePmk(prNdp);

		/* Sigma workaround: g_prNanWpaSupp is not assigned yet */
		wpa_supplicant_set_bssid(g_prNanWpaSupp, prNdp->aucPeerNDIAddr);

	}

	wpa_SYSrand_Gen_Rand_Seed(prNdp->aucLocalNDIAddr);

	return 0;
}

uint32_t
nanSecNotify4wayTerminate(IN struct _NAN_NDP_INSTANCE_T *prNdp) {
	DBGLOG(NAN, INFO, "Enter, eNDPRole:%d, NDPID:%d\n",
	       prNdp->eNDPRole, prNdp->ucNDPID);

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		/* wpa sm back to disconnect */
		prNdp->prInitiatorSecSmInfo->Disconnect = TRUE;
		wpa_sm_step(prNdp->prInitiatorSecSmInfo);

		/* Orignal clean up */
		wpa_auth_sta_deinit(prNdp->prInitiatorSecSmInfo);

		/* NAN clean up */
		nanSecApSmBufReset(prNdp->prInitiatorSecSmInfo);

		/* Keep NDP index info */
		prNdp->prInitiatorSecSmInfo->pvNdp = (void *)prNdp;

		os_free(g_prNanHapdData->conf->ssid.wpa_psk);
		g_prNanHapdData->conf->ssid.wpa_psk = NULL;
	} else { /* NAN_NDP_RESPONDER */
		if (g_prNanWpaSupp == NULL || g_prNanWpaSupp->wpa == NULL) {
			DBGLOG(NAN, ERROR,
				"g_prNanWpaSupp is NULL\n");
			return 0;
		}

		/* Orignal clean up */
		g_prNanWpaSupp->wpa->rx_replay_counter_set = 0;
		os_memset(g_prNanWpaSupp->wpa->rx_replay_counter, 0,
			  WPA_REPLAY_COUNTER_LEN);
		g_prNanWpaSupp->wpa->msg_3_of_4_ok = 0;

		g_prNanWpaSupp->wpa->ptk_set = 0;
		os_memset(&g_prNanWpaSupp->wpa->ptk, 0,
			  sizeof(g_prNanWpaSupp->wpa->ptk));
		g_prNanWpaSupp->wpa->tptk_set = 0;
		os_memset(&g_prNanWpaSupp->wpa->tptk, 0,
			  sizeof(g_prNanWpaSupp->wpa->tptk));
		os_memset(&g_prNanWpaSupp->wpa->gtk, 0,
			  sizeof(g_prNanWpaSupp->wpa->gtk));

		/* NAN clean up */
		nanSecStaSmBufReset(prNdp->prResponderSecSmInfo);
		kalMemZero(prNdp->prResponderSecSmInfo, sizeof(struct wpa_sm));

		/* Keep NDP index info */
		prNdp->prResponderSecSmInfo->pvNdp = (void *)prNdp;
	}

	/* Common */

	return 0;
}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

uint32_t
nanSecTxKdeAttrDone(IN struct _NAN_NDP_INSTANCE_T *prNdp, IN uint8_t u1DstMsg) {
	u8 u1SmCurMsg = 0;
	u8 *pu1SmTmpKdeAttrBuf = NULL;
	u32 *pu4SmTmpKdeAttrLen = NULL;
	bool *pfgIsTxDone = NULL;

	DBGLOG(NAN, INFO, "Enter, eNDPRole:%d, u1DstMsg:%d\n",
	       prNdp->eNDPRole, u1DstMsg);

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		u1SmCurMsg = prNdp->prInitiatorSecSmInfo->u1CurMsg;
		pu1SmTmpKdeAttrBuf =
			prNdp->prInitiatorSecSmInfo->au1TmpKdeAttrBuf;
		pu4SmTmpKdeAttrLen =
			&prNdp->prInitiatorSecSmInfo->u4TmpKdeAttrLen;
		pfgIsTxDone = &prNdp->prInitiatorSecSmInfo->fgIsTxDone;
	} else { /* NAN_NDP_RESPONDER */
		u1SmCurMsg = prNdp->prResponderSecSmInfo->u1CurMsg;
		pu1SmTmpKdeAttrBuf =
			prNdp->prResponderSecSmInfo->au1TmpKdeAttrBuf;
		pu4SmTmpKdeAttrLen =
			&prNdp->prResponderSecSmInfo->u4TmpKdeAttrLen;
		pfgIsTxDone = &prNdp->prResponderSecSmInfo->fgIsTxDone;
	}

	if (u1SmCurMsg != u1DstMsg) {
		DBGLOG(NAN, ERROR, "ERROR! Msg mismatch, u1SmCurMsg:%d",
		       u1SmCurMsg);
		return -1;
	}

	kalMemZero(pu1SmTmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
	*pu4SmTmpKdeAttrLen = 0;
	*pfgIsTxDone = TRUE;

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		nanSecMicCalApSmStep(prNdp->prInitiatorSecSmInfo);
	} else { /* NAN_NDP_RESPONDER */
		nanSecMicCalStaSmStep(prNdp->prResponderSecSmInfo);
	}

	*pfgIsTxDone = FALSE;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSecRxKdeAttr(IN struct _NAN_NDP_INSTANCE_T *prNdp, IN uint8_t u1SrcMsg,
		IN uint32_t u4KdeAttrLen, IN uint8_t *pu1KdeAttrBuf,
		IN uint32_t u4RxMsgLen, IN uint8_t *pu1RxMsgBuf) {
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(NAN, INFO,
	       "Enter, eNDPRole:%d, u1SrcMsg:%d, u4KdeAttrLen:%d\n",
	       prNdp->eNDPRole, u1SrcMsg, u4KdeAttrLen);

#if (ENABLE_SEC_UT_LOG == 1)
	dumpMemory8(pu1KdeAttrBuf, u4KdeAttrLen);
	dumpMemory8(pu1RxMsgBuf, u4RxMsgLen);
#endif

	if (u1SrcMsg == NAN_SEC_END) {
		DBGLOG(NAN, INFO, "Rcv NDP terminate, return\n");
		return rStatus;
	}

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		/* M2, M4 */
		prNdp->prInitiatorSecSmInfo->pu1GetRxMsgBodyBuf = pu1RxMsgBuf;
		prNdp->prInitiatorSecSmInfo->u4GetRxMsgBodyLen = u4RxMsgLen;

		prNdp->prInitiatorSecSmInfo->pu1GetRxMsgKdeBuf = pu1KdeAttrBuf;
		prNdp->prInitiatorSecSmInfo->u4GetRxMsgKdeLen = u4KdeAttrLen;

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
		if (nanSecIsDevSupportGroupSecurity(g_prAdapter)) {
			/*
			 * GTK required and GTK cipher type is from
			 * different attribute ID
			 * therefore update here
			 */
			nanSecSetGroupCipherType(prNdp, FALSE);
			nanSecSetGroupSA(prNdp);
		}
		rStatus = nan_sec_wpa_receive(&g_rNanWpaAuth[NAN_NDI_INDEX_0],
					      prNdp->prInitiatorSecSmInfo,
					      prNdp->prResponderSecSmInfo,
					      pu1KdeAttrBuf, u4KdeAttrLen);

#else
		rStatus = nan_sec_wpa_receive(&g_rNanWpaAuth[NAN_NDI_INDEX_0],
					      prNdp->prInitiatorSecSmInfo,
					      prNdp->prResponderSecSmInfo,
					      pu1KdeAttrBuf, u4KdeAttrLen);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
	} else { /* NAN_NDP_RESPONDER */

		if (u1SrcMsg == NAN_SEC_M1) {
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
			nanSecApSmBufReset(prNdp->prInitiatorSecSmInfo);
			nanSecStaSmBufReset(prNdp->prResponderSecSmInfo);
			if (nanSecIsDevSupportGroupSecurity(g_prAdapter)) {
				/* GTK required and GTK cipher
				 * type are from different attribute ID,
				 * therefore update here
				 */
				nanSecSetGroupCipherType(prNdp, FALSE);
				nanSecSetGroupSA(prNdp);
			}
#endif
			if (prNdp->prResponderSecSmInfo->pu1GetRxMsgBodyBuf !=
			    NULL)
				os_free(prNdp->prResponderSecSmInfo
						->pu1GetRxMsgBodyBuf);
			prNdp->prResponderSecSmInfo->pu1GetRxMsgBodyBuf =
				os_zalloc(u4RxMsgLen);
			kalMemCopy(
				prNdp->prResponderSecSmInfo->pu1GetRxMsgBodyBuf,
				pu1RxMsgBuf, u4RxMsgLen);
			prNdp->prResponderSecSmInfo->u4GetRxMsgBodyLen =
				u4RxMsgLen;
			prNdp->prResponderSecSmInfo->fgIsAllocRxMsgForM1 = TRUE;

			if (prNdp->prResponderSecSmInfo->pu1GetRxMsgKdeBuf !=
			    NULL)
				os_free(prNdp->prResponderSecSmInfo
						->pu1GetRxMsgKdeBuf);
			prNdp->prResponderSecSmInfo->pu1GetRxMsgKdeBuf =
				os_zalloc(u4KdeAttrLen);
			kalMemCopy(
				prNdp->prResponderSecSmInfo->pu1GetRxMsgKdeBuf,
				pu1KdeAttrBuf, u4KdeAttrLen);
			prNdp->prResponderSecSmInfo->u4GetRxMsgKdeLen =
				u4KdeAttrLen;

			DBGLOG(NAN, INFO,
			       "prResponderSecSmInfo:0x%p, u4GetRxKdeAttrLen:%d\n",
			       prNdp->prResponderSecSmInfo,
			       u4KdeAttrLen);
		} else if (u1SrcMsg == NAN_SEC_M3) {
#if 1
			if (prNdp->prResponderSecSmInfo->fgIsAllocRxMsgForM1) {
				os_free(prNdp->prResponderSecSmInfo
						->pu1GetRxMsgBodyBuf);
				os_free(prNdp->prResponderSecSmInfo
						->pu1GetRxMsgKdeBuf);

				prNdp->prResponderSecSmInfo
					->fgIsAllocRxMsgForM1 = FALSE;
			}
#endif
			prNdp->prResponderSecSmInfo->pu1GetRxMsgBodyBuf =
				pu1RxMsgBuf;
			prNdp->prResponderSecSmInfo->u4GetRxMsgBodyLen =
				u4RxMsgLen;

			prNdp->prResponderSecSmInfo->pu1GetRxMsgKdeBuf =
				pu1KdeAttrBuf;
			prNdp->prResponderSecSmInfo->u4GetRxMsgKdeLen =
				u4KdeAttrLen;

			rStatus = nan_sec_wpa_sm_rx_eapol(
				prNdp->prInitiatorSecSmInfo,
				prNdp->prResponderSecSmInfo,
				prNdp->aucPeerNDIAddr);
		}
	}

	return rStatus;
}

uint32_t
nanSecNotifyMsgBodyRdy(IN struct _NAN_NDP_INSTANCE_T *prNdp,
		IN uint8_t u1SrcMsg, IN OUT uint32_t u4TxMsgLen,
		IN OUT uint8_t *pu1TxMsgBuf) {
	u8 u1SmCurMsg = 0;
	u8 **ppu1SmGetMsgBodyBuf = NULL;
	u32 *pu4SmGetMsgBodyLen = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(NAN, INFO,
	       "Enter, eNDPRole:%d, u1SrcMsg:%d, u4TxMsgLen:%d\n",
	       prNdp->eNDPRole, u1SrcMsg, u4TxMsgLen);

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		u1SmCurMsg = prNdp->prInitiatorSecSmInfo->u1CurMsg;
		ppu1SmGetMsgBodyBuf =
			&prNdp->prInitiatorSecSmInfo->pu1GetTxMsgBodyBuf;
		pu4SmGetMsgBodyLen =
			&prNdp->prInitiatorSecSmInfo->u4GetTxMsgBodyLen;
	} else { /* NAN_NDP_RESPONDER */
		u1SmCurMsg = prNdp->prResponderSecSmInfo->u1CurMsg;
		ppu1SmGetMsgBodyBuf =
			&prNdp->prResponderSecSmInfo->pu1GetTxMsgBodyBuf;
		pu4SmGetMsgBodyLen =
			&prNdp->prResponderSecSmInfo->u4GetTxMsgBodyLen;
	}

	if (u1SmCurMsg != u1SrcMsg) {
		DBGLOG(NAN, ERROR, "ERROR! Msg mismatch, u1SmCurMsg:%d",
		       u1SmCurMsg);
		return -1;
	}

	*ppu1SmGetMsgBodyBuf = pu1TxMsgBuf;
	*pu4SmGetMsgBodyLen = u4TxMsgLen;

	if (u1SrcMsg == NAN_SEC_M1) {
		if (prNdp->prInitiatorSecSmInfo->pu1AuthTokenBuf != NULL)
			os_free(prNdp->prInitiatorSecSmInfo->pu1AuthTokenBuf);

		prNdp->prInitiatorSecSmInfo->pu1AuthTokenBuf =
			os_zalloc(NAN_AUTH_TOKEN_LEN);
		if (prNdp->prInitiatorSecSmInfo->pu1AuthTokenBuf == NULL) {
			DBGLOG(NAN, ERROR,
			       "os_zalloc failed for pu1AuthTokenBuf\n");
			return WLAN_STATUS_FAILURE;
		}

		rStatus = nanSecGenAuthToken(
			prNdp->prInitiatorSecSmInfo->u4SelCipherType,
			pu1TxMsgBuf, u4TxMsgLen,
			prNdp->prInitiatorSecSmInfo->pu1AuthTokenBuf);

	} else { /* M2, M3, M4 */
		if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {

			rStatus = nanSecMicCalApSmStep(
				prNdp->prInitiatorSecSmInfo);
		} else { /* NAN_NDP_RESPONDER */

			/* M1 Auth token pre-calculation */
			if (u1SrcMsg == NAN_SEC_M2) {
				if (prNdp->prResponderSecSmInfo
					    ->fgIsAllocRxMsgForM1 == FALSE) {
					DBGLOG(NAN, ERROR,
					       "no M1 msg for pu1AuthTokenBuf\n");
					return WLAN_STATUS_FAILURE;
				}

				if (prNdp->prResponderSecSmInfo
					    ->pu1AuthTokenBuf != NULL)
					os_free(prNdp->prResponderSecSmInfo
							->pu1AuthTokenBuf);

				prNdp->prResponderSecSmInfo->pu1AuthTokenBuf =
					os_zalloc(NAN_AUTH_TOKEN_LEN);
				if (prNdp->prResponderSecSmInfo
					    ->pu1AuthTokenBuf == NULL) {
					DBGLOG(NAN, ERROR,
					       "os_zalloc failed for pu1AuthTokenBuf\n");
					return WLAN_STATUS_FAILURE;
				}

				rStatus = nanSecGenAuthToken(
					prNdp->prResponderSecSmInfo
						->u4SelCipherType,
					prNdp->prResponderSecSmInfo
						->pu1GetRxMsgBodyBuf,
					prNdp->prResponderSecSmInfo
						->u4GetRxMsgBodyLen,
					prNdp->prResponderSecSmInfo
						->pu1AuthTokenBuf);

				if (rStatus == WLAN_STATUS_FAILURE)
					return rStatus;
			}

			rStatus = nanSecMicCalStaSmStep(
				prNdp->prResponderSecSmInfo);
		}
	}

	return rStatus;
}

/************************************************
 *               Self-Use API Related
 ************************************************
 */
uint32_t
nanSecUpdatePmk(struct _NAN_NDP_INSTANCE_T *prNdp) {
	DBGLOG(NAN, INFO, "Enter\n");

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	/* Update PMK for both authenticator and supplicant */
	/* authenticator */
	if (g_prNanHapdData->conf->ssid.wpa_psk == NULL)
		g_prNanHapdData->conf->ssid.wpa_psk =
			os_zalloc(sizeof(struct hostapd_wpa_psk));

	nan_os_memset(g_prNanHapdData->conf->ssid.wpa_psk->psk, 0, PMK_LEN);
	kalMemCpyS(g_prNanHapdData->conf->ssid.wpa_psk->psk,
		   PMK_LEN,
		   prNdp->prInitiatorSecSmInfo->au1Psk,
		   prNdp->prInitiatorSecSmInfo->u4PskLen);
	g_prNanHapdData->conf->ssid.wpa_psk_set = 1;

	/* supplicant */
	nan_os_memset(g_prNanWpaSupp->wpa->pmk, 0, PMK_LEN);
	kalMemCpyS(g_prNanWpaSupp->wpa->pmk,
		   PMK_LEN,
		   prNdp->prResponderSecSmInfo->au1Psk,
		   prNdp->prResponderSecSmInfo->u4PskLen);
	g_prNanWpaSupp->wpa->pmk_len =
		prNdp->prResponderSecSmInfo->u4PskLen;

#else
	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		if (g_prNanHapdData->conf->ssid.wpa_psk == NULL)
			g_prNanHapdData->conf->ssid.wpa_psk =
				os_zalloc(sizeof(struct hostapd_wpa_psk));

		os_memset(g_prNanHapdData->conf->ssid.wpa_psk->psk, 0, PMK_LEN);
		kalMemCopy(g_prNanHapdData->conf->ssid.wpa_psk->psk,
			   prNdp->prInitiatorSecSmInfo->au1Psk,
			   prNdp->prInitiatorSecSmInfo->u4PskLen);
		g_prNanHapdData->conf->ssid.wpa_psk_set = 1;
	} else {
		os_memset(g_prNanWpaSupp->wpa->pmk, 0, PMK_LEN);
		kalMemCopy(g_prNanWpaSupp->wpa->pmk,
			   prNdp->prResponderSecSmInfo->au1Psk,
			   prNdp->prResponderSecSmInfo->u4PskLen);
		g_prNanWpaSupp->wpa->pmk_len =
			prNdp->prResponderSecSmInfo->u4PskLen;
	}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
	return 0;
}

uint8_t
nanSecSelPtkKeyId(struct _NAN_NDP_INSTANCE_T *prNdp, uint8_t *pu1PeerAddr) {
	int8_t i1CurMaxPtkKeyId = -1;
	uint8_t i;

	/* DBGLOG(NAN, INFO, "[%s] Enter, u1NdpIdx:%d", __func__, u1NdpIdx); */

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		for (i = 0; i < MAX_NDP_NUM; i++) {
			if (!kalMemCmp(g_arNanWpaAuthSm[i].au1RmtAddr,
				       pu1PeerAddr, ETH_ALEN)) {
				if ((g_arNanWpaAuthSm[i].fgPtkKeyIdSet) &&
				    (g_arNanWpaAuthSm[i].u1PtkKeyId >
				     i1CurMaxPtkKeyId)) {
					i1CurMaxPtkKeyId =
						g_arNanWpaAuthSm[i].u1PtkKeyId;
				}
			}
		}
	} else {
		for (i = 0; i < MAX_NDP_NUM; i++) {
			if (!kalMemCmp(g_arNanWpaSm[i].bssid, pu1PeerAddr,
				       ETH_ALEN)) {
				if ((g_arNanWpaSm[i].fgPtkKeyIdSet) &&
				    (g_arNanWpaSm[i].u1PtkKeyId >
				     i1CurMaxPtkKeyId)) {
					i1CurMaxPtkKeyId =
						g_arNanWpaSm[i].u1PtkKeyId;
				}
			}
		}
	}

	if (i1CurMaxPtkKeyId >= NAN_MAX_KEY_ID)
		return -1;
	else
		return (i1CurMaxPtkKeyId + 1);

	return 0;
}

uint32_t
nanSecMicCalStaSmStep(struct wpa_sm *sm) /* Send M2, M4 */
{
	struct wpa_eapol_key_192 *reply;
	uint8_t *pu1Kck = NULL;
	uint8_t u1KckLen = 0;

	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(NAN, INFO,
	       "Enter, state:%d, u4TmpKdeAttrLen:%d, u4GetTxMsgBodyLen:%d\n",
	       sm->u1MicCalState, sm->u4TmpKdeAttrLen,
	       sm->u4GetTxMsgBodyLen);

	if (sm->u1MicCalState == NAN_SEC_MIC_CAL_ERROR) { /*unlock until reset*/
		return WLAN_STATUS_FAILURE;
	}

	switch (sm->u1MicCalState) {
	case NAN_SEC_MIC_CAL_IDLE: {
		sm->u1MicCalState = NAN_SEC_MIC_CAL_WAIT;
		break;
	}

	case NAN_SEC_MIC_CAL_WAIT: {
		DBGLOG(NAN, INFO, "CAL_MIC_BEGIN\n");

		reply = (struct wpa_eapol_key_192
				 *)(sm->au1TmpKdeAttrBuf +
				    sizeof(struct _NAN_SEC_KDE_ATTR_HDR));

		if (sm->u1CurMsg == NAN_SEC_M2) {
			pu1Kck = sm->tptk.kck;
			u1KckLen = sm->tptk.kck_len;
		} else {
			pu1Kck = sm->ptk.kck;
			u1KckLen = sm->ptk.kck_len;
		}

		if (nan_sec_wpa_eapol_key_mic(
			    pu1Kck, u1KckLen, sm->u4SelCipherType,
			    sm->pu1GetTxMsgBodyBuf, sm->u4GetTxMsgBodyLen,
			    reply->key_mic)) {
			DBGLOG(NAN, INFO,
			       "ERROR! nan_wpa_eapol_key_mic_wpa() failed\n");
			sm->u1MicCalState = NAN_SEC_MIC_CAL_ERROR;
			return WLAN_STATUS_FAILURE;
		}

		/* Fill-in KDE for NDP */
		kalMemCopy(sm->pu1GetTxMsgKdeBuf, sm->au1TmpKdeAttrBuf,
			   sm->u4TmpKdeAttrLen);

		sm->u1MicCalState = NAN_SEC_MIC_CAL_DONE;

		/* rStatus = nanNdpNotifySecAttrRdy(sm->u1NdpIdx); */
		/* TODO_CJ */
		rStatus = WLAN_STATUS_SUCCESS; /* notify NDP */

		DBGLOG(NAN, INFO, "CAL_MIC_DONE\n");

		break;
	}

	case NAN_SEC_MIC_CAL_DONE: {
		if (!sm->fgIsTxDone) {
			DBGLOG(NAN, INFO,
			       "Quit this time. Step Done must after TxDone\n");
			rStatus = WLAN_STATUS_PENDING;
			break;
		}

		kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
		sm->u4TmpKdeAttrLen = 0;
		sm->u1MicCalState = NAN_SEC_MIC_CAL_IDLE;

		sm->pu1GetTxMsgBodyBuf = NULL;
		sm->u4GetTxMsgBodyLen = 0;
		sm->pu1GetTxMsgKdeBuf = NULL;

		rStatus = WLAN_STATUS_SUCCESS;

		break;
	}

	default:
		break;
	}

	return rStatus;
}

uint32_t
nanSecStaSmBufReset(struct wpa_sm *sm) {
	DBGLOG(NAN, INFO, "Enter\n");

	os_free(sm->pu1AuthTokenBuf);
	sm->pu1AuthTokenBuf = NULL;

	os_free(sm->pu1M3MicMaterialBuf);
	sm->pu1M3MicMaterialBuf = NULL;
	sm->u4M3MicMaterialLen = 0;

	/* os_free(sm->pu1GetTxMsgBodyBuf); */
	/* Buf from NDP */
	sm->pu1GetTxMsgBodyBuf = NULL;
	sm->u4GetTxMsgBodyLen = 0;
	sm->pu1GetTxMsgKdeBuf = NULL;

#if 1
	if (sm->fgIsAllocRxMsgForM1) {
		os_free(sm->pu1GetRxMsgBodyBuf);

		os_free(sm->pu1GetRxMsgKdeBuf);

		sm->fgIsAllocRxMsgForM1 = FALSE;
	}
#endif

	sm->pu1GetRxMsgBodyBuf = NULL;
	sm->u4GetRxMsgBodyLen = 0;

	sm->pu1GetRxMsgKdeBuf = NULL;
	sm->u4GetRxMsgKdeLen = 0;

	kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
	sm->u4TmpKdeAttrLen = 0;

	kalMemZero(sm, sizeof(struct wpa_sm));

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSecMicCalApSmStep(struct wpa_state_machine *sm) /* Send M1, M3 */
{
	struct wpa_eapol_key_192 *reply;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(NAN, INFO,
	       "Enter, state:%d, u4TmpKdeAttrLen:%d, u4GetTxMsgBodyLen:%d\n",
	       sm->u1MicCalState, sm->u4TmpKdeAttrLen,
	       sm->u4GetTxMsgBodyLen);

	if (sm->u1MicCalState == NAN_SEC_MIC_CAL_ERROR) {
		/* unlock until reset */
		return WLAN_STATUS_FAILURE;
	}

	switch (sm->u1MicCalState) {
	case NAN_SEC_MIC_CAL_IDLE: {
		if (sm->u1CurMsg == NAN_SEC_M1) {
			DBGLOG(NAN, INFO, "for M1, direct send out\n");
			sm->u1MicCalState = NAN_SEC_MIC_CAL_DONE;
		} else
			sm->u1MicCalState = NAN_SEC_MIC_CAL_WAIT;
		break;
	}

	case NAN_SEC_MIC_CAL_WAIT: {
		reply = (struct wpa_eapol_key_192
				 *)(sm->au1TmpKdeAttrBuf +
				    sizeof(struct _NAN_SEC_KDE_ATTR_HDR));

		/* Gen (auth token||M3 body) */
		if (sm->pu1M3MicMaterialBuf != NULL)
			os_free(sm->pu1M3MicMaterialBuf);

		rStatus = nanSecGenM3MicMaterial(
			sm->pu1AuthTokenBuf, sm->pu1GetTxMsgBodyBuf,
			sm->u4GetTxMsgBodyLen, &sm->pu1M3MicMaterialBuf,
			&sm->u4M3MicMaterialLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return WLAN_STATUS_FAILURE;

		/* MIC calulation */
		if (nan_sec_wpa_eapol_key_mic(
			    sm->PTK.kck, sm->PTK.kck_len, sm->u4SelCipherType,
			    sm->pu1M3MicMaterialBuf, sm->u4M3MicMaterialLen,
			    reply->key_mic)) {
			DBGLOG(NAN, INFO,
			       "ERROR! nan_wpa_eapol_key_mic_wpa() failed");
			sm->u1MicCalState = NAN_SEC_MIC_CAL_ERROR;
			return WLAN_STATUS_FAILURE;
		}

		/* Fill-in KDE for NDP */
		kalMemCopy(sm->pu1GetTxMsgKdeBuf, sm->au1TmpKdeAttrBuf,
			   sm->u4TmpKdeAttrLen);

		sm->u1MicCalState = NAN_SEC_MIC_CAL_DONE;

		/* rStatus = nanNdpNotifySecAttrRdy(sm->u1NdpIdx); */
		/* TODO_CJ: integrate with NDP */

		break;
	}

	case NAN_SEC_MIC_CAL_DONE: {
		if (!sm->fgIsTxDone) {
			DBGLOG(NAN, INFO,
			       "Quit this time. Step Done must after TxDone\n");
			rStatus = WLAN_STATUS_PENDING;
			break;
		}

		sm->u1MicCalState = NAN_SEC_MIC_CAL_IDLE;

		kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
		sm->u4TmpKdeAttrLen = 0;

		sm->pu1GetTxMsgBodyBuf = NULL;
		sm->u4GetTxMsgBodyLen = 0;
		sm->pu1GetTxMsgKdeBuf = NULL;

		os_free(sm->pu1M3MicMaterialBuf);
		sm->pu1M3MicMaterialBuf = NULL;
		sm->u4M3MicMaterialLen = 0;

		rStatus = WLAN_STATUS_SUCCESS;

		break;
	}

	default:
		break;
	}

	return rStatus;
}

uint32_t
nanSecApSmBufReset(struct wpa_state_machine *sm) {
	DBGLOG(NAN, INFO, "Enter, sm:0x%p\n", sm);

	DBGLOG(NAN, INFO, "pu1AuthTokenBuf:0x%p\n",
	       sm->pu1AuthTokenBuf);
	if (sm->pu1AuthTokenBuf != NULL)
		dumpMemory8(sm->pu1AuthTokenBuf, NAN_AUTH_TOKEN_LEN);

	DBGLOG(NAN, INFO, "pu1M3MicMaterialBuf:0x%p\n",
	       sm->pu1M3MicMaterialBuf);
	if (sm->pu1M3MicMaterialBuf != NULL)
		dumpMemory8(sm->pu1M3MicMaterialBuf, sm->u4M3MicMaterialLen);

	DBGLOG(NAN, INFO, "dump au1TmpKdeAttrBuf:\n");
	DBGLOG_MEM8(NAN, INFO, sm->au1TmpKdeAttrBuf, sm->u4TmpKdeAttrLen);

	os_free(sm->pu1AuthTokenBuf);
	sm->pu1AuthTokenBuf = NULL;

	os_free(sm->pu1M3MicMaterialBuf);
	sm->pu1M3MicMaterialBuf = NULL;
	sm->u4M3MicMaterialLen = 0;

	/* os_free(sm->pu1GetTxMsgBodyBuf); */
	/* Buf from NDP */
	sm->pu1GetTxMsgBodyBuf = NULL;
	sm->u4GetTxMsgBodyLen = 0;
	sm->pu1GetTxMsgKdeBuf = NULL;

	/* os_free(sm->pu1GetRxMsgBodyBuf); */
	/* Buf from NDP */
	sm->pu1GetRxMsgBodyBuf = NULL;
	sm->u4GetRxMsgBodyLen = 0;

	/* os_free(sm->pu1GetRxMsgKdeBuf); */
	/* Buf from NDP */
	sm->pu1GetRxMsgKdeBuf = NULL;
	sm->u4GetRxMsgKdeLen = 0;

	kalMemZero(sm->au1TmpKdeAttrBuf, NAN_KDE_ATTR_BUF_SIZE);
	sm->u4TmpKdeAttrLen = 0;

	kalMemZero(sm,
		   sizeof(struct wpa_state_machine));
	/* TODO_CJ: better place */

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSecGenAuthToken(u32 cipher, const u8 *auth_token_data,
		   size_t auth_token_data_len, u8 *auth_token) {
	u8 hash[SHA384_MAC_LEN];

	DBGLOG(NAN, INFO, "Enter, cipher:%d\n", cipher);

	if ((auth_token_data == NULL) || (auth_token_data_len == 0)) {
		DBGLOG(NAN, INFO, "ERROR! auth_token_data is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

#if (ENABLE_SEC_UT_LOG == 1)
	dumpMemory8((uint8_t *)auth_token_data, auth_token_data_len);
#endif

	if (cipher == NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256) {
		if (sha384_vector(1, &auth_token_data, &auth_token_data_len,
				  hash)) {
			DBGLOG(NAN, INFO, "ERROR! sha256_vector() failed");
			return WLAN_STATUS_FAILURE;
		}
		nan_os_memcpyS(auth_token,
			NAN_AUTH_TOKEN_LEN, hash,
			NAN_AUTH_TOKEN_LEN);

#if (ENABLE_SEC_UT_LOG == 1)
		dumpMemory8((uint8_t *)auth_token, NAN_AUTH_TOKEN_LEN);
#endif
	} else {
		/* NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128 */
		if (sha256_vector(1, &auth_token_data, &auth_token_data_len,
				  hash)) {
			DBGLOG(NAN, INFO, "ERROR! sha256_vector() failed");
			return WLAN_STATUS_FAILURE;
		}
		nan_os_memcpyS(auth_token,
			NAN_AUTH_TOKEN_LEN, hash,
			NAN_AUTH_TOKEN_LEN);

#if (ENABLE_SEC_UT_LOG == 1)
		dumpMemory8((uint8_t *)auth_token, NAN_AUTH_TOKEN_LEN);
#endif
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSecGenM3MicMaterial(IN uint8_t *pu1AuthTokenBuf, IN const u8 *pu1M3bodyBuf,
		       IN uint32_t u4M3BodyLen,
		       OUT uint8_t **ppu1M3MicMaterialBuf,
		       OUT uint32_t *pu4M3MicMaterialLen) {
	uint32_t u4TotalLen = 0;
	uint8_t *pu1MicMaterialBuf = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	if (pu1AuthTokenBuf == NULL) {
		DBGLOG(NAN, ERROR, "ERROR! pu1MicMaterialBuf is NULL");
		return WLAN_STATUS_FAILURE;
	}

	u4TotalLen = u4M3BodyLen + NAN_AUTH_TOKEN_LEN;
	pu1MicMaterialBuf = os_zalloc(u4TotalLen);

	if (pu1MicMaterialBuf == NULL) {
		DBGLOG(NAN, ERROR,
		       "ERROR! os_zalloc failed for pu1MicMaterialBuf");
		return WLAN_STATUS_FAILURE;
	}

	*ppu1M3MicMaterialBuf = pu1MicMaterialBuf;
	*pu4M3MicMaterialLen = u4TotalLen;

	kalMemCopy(pu1MicMaterialBuf, pu1AuthTokenBuf, NAN_AUTH_TOKEN_LEN);
	kalMemCopy(pu1MicMaterialBuf + NAN_AUTH_TOKEN_LEN, pu1M3bodyBuf,
		   u4M3BodyLen);

	DBGLOG(NAN, INFO, "pu1AuthTokenBuf:\n");
	dumpMemory8(pu1AuthTokenBuf, NAN_AUTH_TOKEN_LEN);

	DBGLOG(NAN, INFO, "pu1M3bodyBuf:\n");
	dumpMemory8((uint8_t *)pu1M3bodyBuf, u4M3BodyLen);

	DBGLOG(NAN, INFO, "pu1MicMaterialBuf:\n");
	dumpMemory8(pu1MicMaterialBuf, u4TotalLen);

	return WLAN_STATUS_SUCCESS;
}

uint16_t
nanSecCalKdeAttrLenFunc(struct _NAN_NDP_INSTANCE_T *prNdp) {
	if (prNdp->eNDPRole ==
	    NAN_PROTOCOL_INITIATOR) {
		/* TODO: integrate with nan_base defines */
		return prNdp->prInitiatorSecSmInfo->u4TmpKdeAttrLen;
	} else {
		return prNdp->prResponderSecSmInfo->u4TmpKdeAttrLen;
	}
}

void
nanSecAppendKdeAttrFunc(struct _NAN_NDP_INSTANCE_T *prNdp,
			struct MSDU_INFO *prMsduInfo) {
	uint8_t *pu1TmpKdeAttrBuf = NULL;
	uint32_t u4TmpKdeAttrLen = 0;

	uint8_t *pu1MsduKdePos = NULL;

	/* Pivot the KDE beginning ptr */
	pu1MsduKdePos = prMsduInfo->prPacket + prMsduInfo->u2FrameLength;

	if (prNdp->eNDPRole ==
	    NAN_PROTOCOL_INITIATOR) {
	    /* TODO: integrate with nan_base defines */
		pu1TmpKdeAttrBuf =
			prNdp->prInitiatorSecSmInfo->au1TmpKdeAttrBuf;
		u4TmpKdeAttrLen = prNdp->prInitiatorSecSmInfo->u4TmpKdeAttrLen;

		prNdp->prInitiatorSecSmInfo->pu1GetTxMsgKdeBuf = pu1MsduKdePos;
	} else {
		pu1TmpKdeAttrBuf =
			prNdp->prResponderSecSmInfo->au1TmpKdeAttrBuf;
		u4TmpKdeAttrLen = prNdp->prResponderSecSmInfo->u4TmpKdeAttrLen;

		prNdp->prResponderSecSmInfo->pu1GetTxMsgKdeBuf = pu1MsduKdePos;
	}

	kalMemCopy(pu1MsduKdePos, pu1TmpKdeAttrBuf, u4TmpKdeAttrLen);
	prMsduInfo->u2FrameLength += u4TmpKdeAttrLen;
}

struct wpa_state_machine *
nanSecGetInitiatorSm(uint8_t u1Index) {
	return &g_arNanWpaAuthSm[u1Index];
}

struct wpa_sm *
nanSecGetResponderSm(uint8_t u1Index) {
	return &g_arNanWpaSm[u1Index];
}

void nanInitSecResponderSm(struct wpa_sm *prSm)
{
	kalMemZero(prSm, sizeof(struct wpa_sm));
}

void nanInitSecInitiatorSm(struct wpa_state_machine *prStaMac)
{
	kalMemZero(prStaMac, sizeof(struct wpa_state_machine));
}

void
nanSecResetTk(struct STA_RECORD *prStaRec) {
	prStaRec->rPmfCfg.fgApplyPmf = FALSE;

	nan_sec_wpas_setkey_glue(FALSE, prStaRec->ucBssIndex, 0,
				 prStaRec->aucMacAddr, 0, NULL, 0);
}

void
nanSecInstallTk(struct _NAN_NDP_INSTANCE_T *prNdp,
		struct STA_RECORD *prStaRec) {
	uint8_t *pu1Tk = NULL;
	uint8_t u1TkLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Cipher;
	enum wpa_alg alg;

	DBGLOG(NAN, INFO, "Enter, StaIdx:%d, BssIdx:%d\n",
	       prStaRec->ucIndex, prStaRec->ucBssIndex);

	prStaRec->rPmfCfg.fgApplyPmf = TRUE;

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
		pu1Tk = prNdp->prInitiatorSecSmInfo->PTK.tk;
		u1TkLen = prNdp->prInitiatorSecSmInfo->PTK.tk_len;
		u4Cipher = prNdp->prInitiatorSecSmInfo->u4SelCipherType;
	} else {
		pu1Tk = prNdp->prResponderSecSmInfo->ptk.tk;
		u1TkLen = prNdp->prResponderSecSmInfo->ptk.tk_len;
		u4Cipher = prNdp->prResponderSecSmInfo->u4SelCipherType;
	}

	dumpMemory8(pu1Tk, u1TkLen);

	/* TODO_CJ: dynamic chiper, dynamic keyID */
	if (u4Cipher == NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256)
		alg = WPA_ALG_GCMP_256;
	else
		alg = WPA_ALG_CCMP;
	rStatus = nan_sec_wpas_setkey_glue(FALSE, prStaRec->ucBssIndex, alg,
					   prStaRec->aucMacAddr, 0, pu1Tk,
					   u1TkLen);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR)
			prNdp->prInitiatorSecSmInfo->PTK.installed = 1;
		else
			prNdp->prResponderSecSmInfo->ptk.installed = 1;
	}
}

void
nanSecUnload(void) {
	/* TODO_CJ: counter nan_sec_wpa_supplicant_start() */
}

void
nanSecDumpEapolKey(struct wpa_eapol_key *key) {
	DBGLOG(NAN, INFO, "Enter\n");
	DBGLOG(NAN, INFO, "type:0x%x\n", key->type);
	DBGLOG(NAN, INFO, "key_info:0x%x, 0x%x\n",
	       key->key_info[0], key->key_info[1]);
	DBGLOG(NAN, INFO, "key_length:0x%x, 0x%x\n",
	       key->key_length[0], key->key_length[1]);

	DBGLOG(NAN, INFO, "replay counter:\n");
	dumpMemory8(key->replay_counter, WPA_REPLAY_COUNTER_LEN);

	DBGLOG(NAN, INFO, "key_nonce:\n");
	dumpMemory8(key->key_nonce, WPA_NONCE_LEN);

	DBGLOG(NAN, INFO, "key_iv:\n");
	dumpMemory8(key->key_iv, 16);

	DBGLOG(NAN, INFO, "key_rsc:\n");
	dumpMemory8(key->key_rsc, WPA_KEY_RSC_LEN);

	DBGLOG(NAN, INFO, "key_id:\n");
	dumpMemory8(key->key_id, 8);

	DBGLOG(NAN, INFO, "key_mic:\n");
	dumpMemory8(key->key_mic, 16);

	DBGLOG(NAN, INFO, "key_data_length:0x%x, 0x%x\n",
	       key->key_data_length[0], key->key_data_length[1]);
}

void
nanSecUpdateAttrCmd(IN struct ADAPTER *prAdapter, uint8_t aucAttrId,
		    uint8_t *aucAttrBuf, uint16_t u2AttrLen) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_UPDATE_ATTR_STRUCT *prCmdNanCmdUpdateAttr = NULL;

	DBGLOG(NAN, INFO, "Enter, aucAttrId:0x%x, aucAttrLen:%d\n",
	       aucAttrId, u2AttrLen);

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_UPDATE_ATTR_STRUCT);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(
		NAN_CMD_UPDATE_ATTR, sizeof(struct _NAN_CMD_UPDATE_ATTR_STRUCT),
		u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prCmdNanCmdUpdateAttr =
		(struct _NAN_CMD_UPDATE_ATTR_STRUCT *)prTlvElement->aucbody;
	prCmdNanCmdUpdateAttr->ucAttrId = aucAttrId;
	prCmdNanCmdUpdateAttr->u2AttrLen = u2AttrLen;
	kalMemCopy(&prCmdNanCmdUpdateAttr->aucAttrBuf[0], aucAttrBuf,
		   u2AttrLen);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, NULL, u4CmdBufferLen,
				      (uint8_t *)prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);
}

void
nanSecUpdatePeerNDI(struct _NAN_NDP_INSTANCE_T *prNdp,
		uint8_t *au1PeerNdiAddr) {
	DBGLOG(NAN, INFO, "Enter, role:%d\n", prNdp->eNDPRole);

	if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR)
		kalMemCopy(prNdp->prInitiatorSecSmInfo->addr, au1PeerNdiAddr,
			   MAC_ADDR_LEN);
}

int32_t
nanSecCompareSA(IN struct ADAPTER *prAdapter,
		IN struct _NAN_NDP_INSTANCE_T *prNdp1,
		IN struct _NAN_NDP_INSTANCE_T *prNdp2) {
	uint32_t au4Rank[2];
	struct _NAN_NDP_INSTANCE_T *aprNdp[2];
	uint32_t u4Idx;

	aprNdp[0] = prNdp1;
	aprNdp[1] = prNdp2;
	for (u4Idx = 0; u4Idx < 2; u4Idx++) {
		au4Rank[u4Idx] = 0;

		if (aprNdp[u4Idx]->fgSecurityRequired == TRUE) {
			au4Rank[u4Idx] += 1;

			if (aprNdp[u4Idx]->ucCipherType ==
			    NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256)
				au4Rank[u4Idx] += 2;
			else if (aprNdp[u4Idx]->ucCipherType ==
				 NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128)
				au4Rank[u4Idx] += 1;
			else
				DBGLOG(NAN, ERROR, "ucCipherType error!\n");
		}
	}

	if (au4Rank[0] < au4Rank[1])
		return -1;
	else if (au4Rank[0] == au4Rank[1])
		return 0;
	else
		return 1;
}

/************************************************
 *               NDP Sudo Related
 ************************************************
 */
uint32_t
nanNdpGetMsgBody(IN uint8_t u1NdpIdx, IN uint8_t u1Msg, IN uint8_t u1MicMode,
		 OUT uint32_t *pu4MsgBodyLen, OUT uint8_t **ppu1MsgBody) {
	return 0;
}

uint32_t
nanNdpNotifySecAttrRdy(IN uint8_t u1NdpIdx) {
	return 0;
}

uint32_t
nanNdpGetNdiAddr(uint8_t u1NdpIdx, uint8_t u1Role, uint8_t *pu1MacAddr) {
	return 0;
}

uint32_t
nanNdpGetPublishId(uint8_t *u1NdpIdx) {
	return 0;
}

uint32_t
nanNdpNotifySecStatus(uint8_t u1NdpIdx, uint8_t u1Status, uint8_t u1Reason,
		      uint8_t u1Msg) {
	return 0;
}

uint8_t
nanNdpGetWlanIdx(uint8_t u1NdpIdx) {
	return 0;
}

void
nanSecConfigNmiTk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalNMI, IN uint8_t *pucPeerNMI,
	IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	uint8_t *pu1Tk = NULL;
	size_t szTkLen = 0;
	uint32_t u4Cipher = 0;
	uint8_t ucAlgoId = 0;

	if (fgSet) {
		if (prNdp->eNDPRole == NAN_PROTOCOL_INITIATOR) {
			pu1Tk = prNdp->prInitiatorSecSmInfo->PTK.tk;
			szTkLen = prNdp->prInitiatorSecSmInfo->PTK.tk_len;
			u4Cipher = prNdp->prInitiatorSecSmInfo->u4SelCipherType;
		} else {
			pu1Tk = prNdp->prResponderSecSmInfo->ptk.tk;
			szTkLen = prNdp->prResponderSecSmInfo->ptk.tk_len;
			u4Cipher = prNdp->prResponderSecSmInfo->u4SelCipherType;
		}

		if (u4Cipher == NAN_CIPHER_SUITE_ID_NCS_SK_GCM_256)
			ucAlgoId = WPA_ALG_GCMP_256;
		else
			ucAlgoId = WPA_ALG_CCMP;

		nanSecManageKeyCmd(prAdapter,
			NAN_KEY_OP_SET_KEY,
			NAN_KEY_TYPE_NMI_CXT_MGMT_KEY,
			u2WtblEntry,
			pucLocalNMI,
			pucPeerNMI,
			ucAlgoId, 0, szTkLen, pu1Tk, NULL,
			FALSE, TRUE);
	} else {
		nanSecManageKeyCmd(prAdapter,
			NAN_KEY_OP_CLS_KEY,
			NAN_KEY_TYPE_NMI_CXT_MGMT_KEY,
			u2WtblEntry,
			pucLocalNMI, pucPeerNMI,
			0, 0, 0, NULL, NULL,
			FALSE, FALSE);
	}
}

u_int8_t
nanSecIsPMFApply(struct ADAPTER *prAdapter,
	uint8_t *pucLocalAddr, uint8_t *pucPeerAddr,
	uint8_t *pucWtblEntry)
{
	struct STA_RECORD *prStaRec = nanDataEngineSearchStaRec(prAdapter,
			NULL, pucLocalAddr, pucPeerAddr);
	struct _NAN_KEY_ENTRY_T *prNmiCxtKey = NULL;

	DBGLOG(NAN, INFO, "PeerAddr=" MACSTR ",LocalAddr=" MACSTR "\n",
		MAC2STR(pucPeerAddr), MAC2STR(pucLocalAddr));
	if (pucWtblEntry)
		*pucWtblEntry = WTBL_RESERVED_ENTRY;

	if (prAdapter->rWifiVar.fgUseNmiCxtKey &&
		prAdapter->rWifiVar.fgEnNanKeyMgmt &&
		nanDataEngineIsNmiContext(prAdapter,
		pucLocalAddr, pucPeerAddr)) {
		prNmiCxtKey = nanSecGetNmiCxtKey(prAdapter,
			pucPeerAddr);
		if (prNmiCxtKey && prNmiCxtKey->u2WtblIdx !=
			WTBL_RESERVED_ENTRY) {
			DBGLOG(NAN, INFO, "[PMF][NmiCxt] wtbl:%u,keyExist:%u\n",
				prNmiCxtKey->u2WtblIdx,
				prNmiCxtKey->fgIsKeyExist);

			if (pucWtblEntry)
				*pucWtblEntry = prNmiCxtKey->u2WtblIdx;
			return prNmiCxtKey->fgIsKeyExist;
		}
	} else if (prStaRec != NULL) {
		DBGLOG(NAN, INFO, "[PMF][STA] keyExist:%u, fgApplyPmf:%u\n",
			nanIsStaKeyExist(prStaRec),
			prStaRec->rPmfCfg.fgApplyPmf);
#if CFG_SUPPORT_802_11W
		if (prStaRec->rPmfCfg.fgApplyPmf == FALSE)
			return FALSE;
#endif

		if (pucWtblEntry)
			*pucWtblEntry = prStaRec->ucWlanIndex;
		return nanIsStaKeyExist(prStaRec);
	}
	return FALSE;
}

uint32_t
nanSecManageKeyCmd(IN struct ADAPTER *prAdapter,
	IN enum NAN_KEY_OPERATION eKeyOp,
	IN enum NAN_KEY_TYPE eKeyType, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalAddr, IN uint8_t *pucPeerAddr,
	IN uint8_t ucAlgoId, IN uint8_t ucKeyId,
	IN uint8_t ucKeyLen, IN uint8_t *pucKeyData, IN uint8_t *pucPn,
	IN uint8_t fgInit, IN uint8_t fgKeyExist)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	void *prCmdBuffer = NULL;
	size_t szCmdBufferLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_KEY_MANAGEMENT_T *prCmdManageKey = NULL;
	struct _NAN_KEY_ENTRY_T *prNmiCxtKey = NULL;
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
	struct _NAN_NDC_CTRL_T *prNdcCtrl = NULL;
	size_t szIdx = 0;
#endif

	DBGLOG(NAN, INFO, "[NmiCxt] OP:%u,Type:%u\n",
		eKeyOp, eKeyType);
	DBGLOG(NAN, INFO, "[NmiCxt] Wtbl:%u,Init:%u,KeyExist:%u\n",
		u2WtblEntry, fgInit, fgKeyExist);

	if ((prAdapter == NULL) || (pucLocalAddr == NULL) ||
		  (pucPeerAddr == NULL) || (eKeyType >= NAN_KEY_TYPE_NUM) ||
		  (u2WtblEntry == WTBL_RESERVED_ENTRY) ||
		  ((eKeyOp == NAN_KEY_OP_SET_KEY) && (pucKeyData == NULL))) {
		DBGLOG(NAN, ERROR, "Something error during key operation.\n");
		return WLAN_STATUS_FAILURE;
	}

	szCmdBufferLen =
		sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
		sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
		sizeof(struct _NAN_CMD_KEY_MANAGEMENT_T);
	prCmdBuffer = cnmMemAlloc(prAdapter,
		RAM_TYPE_BUF, (uint32_t)szCmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;
	rStatus = nicAddNewTlvElement(
		NAN_CMD_KEY_MGMT,
		sizeof(struct _NAN_CMD_KEY_MANAGEMENT_T),
		(uint32_t)szCmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prCmdManageKey =
		(struct _NAN_CMD_KEY_MANAGEMENT_T *)
			prTlvElement->aucbody;

	prCmdManageKey->ucOp = (uint8_t)eKeyOp;
	prCmdManageKey->ucKeyType = (uint8_t)eKeyType;
	prCmdManageKey->u2WtblIdx = u2WtblEntry;
	prCmdManageKey->fgInit = fgInit;
	prCmdManageKey->fgKeyExist = fgKeyExist;
	prCmdManageKey->fgIsNmiTk = FALSE;
	COPY_MAC_ADDR(prCmdManageKey->aucLocalAddr, pucLocalAddr);
	COPY_MAC_ADDR(prCmdManageKey->aucPeerAddr, pucPeerAddr);
	switch (eKeyType) {
	case NAN_KEY_TYPE_NMI_CXT_MGMT_KEY:
		prNmiCxtKey = nanSecGetNmiCxtKey(prAdapter, pucPeerAddr);
		if (!prNmiCxtKey) {
			DBGLOG(NAN, ERROR, "[NmiCxt] No NMK key matched\n");
			goto CMD_ERR;
		}
		if ((prNmiCxtKey->u2WtblIdx != WTBL_RESERVED_ENTRY) &&
		  (prNmiCxtKey->u2WtblIdx != prCmdManageKey->u2WtblIdx)) {
			DBGLOG(NAN, ERROR,
				"[NmiCxt] u2NmiCxtWidx unequal (%u,%u)\n",
				prNmiCxtKey->u2WtblIdx,
				prCmdManageKey->u2WtblIdx);
			goto CMD_ERR;
		}
		prCmdManageKey->ucNmiKeyIdx = (uint8_t)prNmiCxtKey->ucIdx;
		prCmdManageKey->fgIsNmiTk = prNmiCxtKey->fgIsNmiTk;
		break;

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
	case NAN_KEY_TYPE_MC_TX_KEY:
		if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
			WLAN_CAPABILITIES_NAN_MC_DATA))
			goto CMD_ERR;
		prNdcCtrl = nanSchedGetNdcCtrl(prAdapter, pucPeerAddr);
		if (prNdcCtrl != NULL)
			prCmdManageKey->ucNdcIdx = prNdcCtrl->ucNdcIdx;
		else {
			DBGLOG(NAN, ERROR, "NDC ID not match prNdcCtrl\n");
			goto CMD_ERR;
		}

		prCmdManageKey->ucNdiIdx =
			nanDataEngineGetNdiIdxByAddr(prAdapter,
			pucLocalAddr);
		break;

	case NAN_KEY_TYPE_MC_RX_KEY:
	case NAN_KEY_TYPE_MC_MGMT_RX_KEY:
		if (IS_NAN_CAPABILITY_DISABLED(prAdapter,
			WLAN_CAPABILITIES_NAN_MC_DATA))
			goto CMD_ERR;
		for (szIdx = 0; szIdx < NAN_MAX_MC_RX_WTBL_NUM; szIdx++) {
			if (g_arNanMcRxWtbl[szIdx].u2WtblIdx == u2WtblEntry) {
				prCmdManageKey->ucMcRxIdx = (uint8_t)szIdx;
				break;
			}
		}
		if (szIdx == NAN_MAX_MC_RX_WTBL_NUM) {
			DBGLOG(NAN, ERROR, "No MC Rx WTBL matched\n");
			goto CMD_ERR;
		}
		break;
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */

	default:
		goto CMD_ERR;
	}

	if (eKeyOp == NAN_KEY_OP_SET_KEY) {
		prCmdManageKey->ucAlgorithmId = ucAlgoId;
		prCmdManageKey->ucKeyId = ucKeyId;
		prCmdManageKey->ucKeyLen = ucKeyLen;
		DBGLOG(NAN, INFO,
			"[NmiCxt] Set_Key ucNmiKeyIdx:%u,cipher:%u KeyID:%u, KeyLen:%u\n",
			prCmdManageKey->ucNmiKeyIdx,
			prCmdManageKey->ucAlgorithmId,
			prCmdManageKey->ucKeyId,
			prCmdManageKey->ucKeyLen);
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
		if (eKeyType == NAN_KEY_TYPE_MC_MGMT_RX_KEY) {
			dumpMemory8((uint8_t *)pucPn, NAN_PACKET_NUMBER_LEN);
			kalMemCpyS(&prCmdManageKey->aucRsvd,
			sizeof_field(struct _NAN_CMD_KEY_MANAGEMENT_T,
			aucRsvd),
			pucPn, NAN_PACKET_NUMBER_LEN);
		}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */
		kalMemCpyS(&prCmdManageKey->aucKeyMaterial,
			sizeof_field(struct _NAN_CMD_KEY_MANAGEMENT_T,
			aucKeyMaterial),
			pucKeyData, ucKeyLen);
		DBGLOG_MEM32(NAN, WARN,
			prCmdManageKey->aucKeyMaterial, ucKeyLen);
	}

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, NULL,
				      (uint32_t)szCmdBufferLen,
					  (uint8_t *)prCmdBuffer,
				      NULL, 0);
	if (rStatus != WLAN_STATUS_FAILURE)
		DBGLOG(NAN, LOUD, "success\n");

CMD_ERR:
	if (prCmdBuffer)
		cnmMemFree(prAdapter, prCmdBuffer);

	return rStatus;

}

#if 0
void testSecCaller(void)
{
	nanSecAllocCsidAttr(NULL, NULL);
	nanSecFreeCsidAttr(NULL);
	nanSecAllocScidAttr(NULL, NULL);
	nanSecFreeScidAttr(NULL);

	nanSecSetCipherType(0, 0);
	nanSecSetPmk(0, 0, NULL);
	nanSecSetScid(0, NULL);

	nanSecNotify4wayBegin(0);
	nanSecNotify4wayTerminate(0);
	nanSecTxKdeAttrDone(0, 0, 0);
	nanSecRxKdeAttr(0, 0, 0, NULL);
	nanSecNotifyMsgBodyRdy(0, 0, 0, 0, NULL);

	nanSecSelPtkKeyId(0, NULL);
	nanSecUpdatePmk(0);

	nan_sec_wpa_eapol_key_mic(NULL, 0, 0,
					  NULL, 0, NULL);
	nan_sec_wpa_supplicant_send_2_of_4(NULL, NULL,
					   NULL,
					   0, NULL,
					   NULL, 0,
					   NULL);

	nan_sec_wpa_supplicant_send_4_of_4(NULL, NULL,
					   NULL,
					   0, 0,
					   NULL);

	nan_sec_wpa_send_eapol(NULL,     /*AP: KDE compose, MIC, and send*/
						   NULL, 0,
						   NULL, NULL,
						   NULL, 0,
						   0, 0, 0);

	nanSecMicCalStaSmStep(NULL);
	nanSecMicCalApSmStep(NULL);

	nanSecStaSmBufReset(NULL);
	nanSecApSmBufReset(NULL);

	nanSecGenAuthToken(NULL, 0, 0,
					   NULL, 0, NULL);
	nanSecGenM3MicMaterial(NULL, NULL, 0,
						   NULL, NULL);
}
#endif

/************************************************
 *               UT Related
 ************************************************
 */
#if (CFG_NAN_SEC_UT == 1)
uint32_t
nanSecUtStaKdeAttr(void) {
	struct wpa_sm *sm = &g_arNanWpaSm[0];
	unsigned char *dst = NULL;
	struct wpa_eapol_key *key = NULL;
	int ver = WPA_KEY_INFO_TYPE_AES_128_CMAC;
	u8 *nonce = NULL;
	u8 *wpa_ie = NULL;
	size_t wpa_ie_len = 0;
	struct wpa_ptk *ptk = NULL;

	uint32_t rStatus = WLAN_STATUS_FAILURE;

	/* Prepare settings */
	g_rNanNdpSudo[0].u1Role = NAN_NDP_RESPONDER;
	nanSecSetCipherType(0, NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128);

	g_rNanNdpSudo[0].u2PublishId = 0x78;

	key = os_zalloc(sizeof(struct wpa_eapol_key));
	key->replay_counter[0] = 0x56;

	random_get_bytes(sm->snonce, WPA_NONCE_LEN);

	/* Compose KDE */
	rStatus = nan_sec_wpa_supplicant_send_2_of_4(
		sm, dst, key, ver, sm->snonce, wpa_ie, wpa_ie_len, ptk);

	/* Dump */
	if (rStatus == WLAN_STATUS_SUCCESS) {
		wpa_hexdump_dbg(MSG_INFO, "Dump STA KDE Attr",
				sm->au1TmpKdeAttrBuf, sm->u4TmpKdeAttrLen);
	}

	return rStatus;
}

uint32_t
nanSecUtApKdeAttr(void) {
	struct wpa_authenticator *wpa_auth = &g_rNanWpaAuth[NAN_NDI_INDEX_0];
	struct wpa_state_machine *sm = &g_arNanWpaAuthSm[0];
	int key_info = WPA_KEY_INFO_ACK | WPA_KEY_INFO_KEY_TYPE;
	u8 *key_rsc = NULL;
	u8 nonce[WPA_NONCE_LEN];
	u8 *kde = NULL;
	size_t kde_len = 0;
	int keyidx = 1;
	int encr = 0;
	int force_version = 0;

	uint32_t rStatus = WLAN_STATUS_FAILURE;

	/* Prepare settings */
	g_rNanNdpSudo[0].u1Role = NAN_NDP_INITIATOR;
	nanSecSetCipherType(0, NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128);

	g_rNanNdpSudo[0].u2PublishId = 0x78;

	random_get_bytes(nonce, WPA_NONCE_LEN);

	g_arNanWpaAuthSm[0].wpa_auth = &g_rNanWpaAuth[NAN_NDI_INDEX_0];

	/* Compose KDE */
	rStatus = nan_sec_wpa_send_eapol(wpa_auth, sm, key_info, key_rsc, nonce,
					 kde, kde_len, keyidx, encr,
					 force_version);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		wpa_hexdump_dbg(MSG_INFO, "Dump AP KDE Attr",
				sm->au1TmpKdeAttrBuf, sm->u4TmpKdeAttrLen);
	} else {
		DBGLOG(NAN, INFO, "[%s] gen KDE failed!\n", __func__);
	}
}

void
nanSecUtPbkdf256(void) {
	/* unsigned char passphrase[] = "NAN"; */
	unsigned char passphrase[] = "NAN2";
	unsigned char salt[] = { 0x00, 0x01, 0x2b, 0x9c, 0x45, 0x0f, 0x66,
				 0x71, 0x02, 0x90, 0x4c, 0x12, 0xd0, 0x01 };

	unsigned char *key = os_zalloc(32); /* key_len:32 */

	DBGLOG(NAN, INFO, "[%s] Enter, p_len:%d, s_len:%d\n", __func__,
	       sizeof(passphrase), sizeof(salt));

	dumpMemory8(passphrase, sizeof(passphrase));
	dumpMemory8(salt, sizeof(salt));

	PKCS5_PBKDF2_HMAC((unsigned char *)passphrase, sizeof(passphrase) - 1,
			  (unsigned char *)salt, sizeof(salt), 4096, 32,
			  (unsigned char *)key);

	dumpMemory8(key, 32);
}

uint32_t
nanSecUtMain(void) {
	DBGLOG(NAN, INFO, "[%s] Enter\n", __func__);

	nanSecUtStaKdeAttr();
	nanSecUtApKdeAttr();
	nanSecUtPbkdf256();

	return 0;
}
#endif

struct _NAN_KEY_ENTRY_T *nanSecAcquireNmiCxtKey(
	struct ADAPTER *prAdapter,
	uint8_t *pucPeerNmiAddr)
{
	uint8_t ucIdx = 0;
	struct _NAN_KEY_ENTRY_T *prNmiCxtKey = NULL;
	uint8_t fgWtblReUsed = FALSE;

	prNmiCxtKey = nanSecGetNmiCxtKey(prAdapter, pucPeerNmiAddr);
	if (!prNmiCxtKey) {
		for (ucIdx = 0; ucIdx < NAN_NUM_NMI_CXT_KEY; ucIdx++) {
			prNmiCxtKey = &g_arNanNmiCxtKey[ucIdx];
			if (prNmiCxtKey->fgValid == FALSE)
				break;
		}
	} else
		ucIdx = prNmiCxtKey->ucIdx;

	if (ucIdx < NAN_NUM_NMI_CXT_KEY) {
		prNmiCxtKey->fgValid = TRUE;
		kalMemCpyS(prNmiCxtKey->aucNmiAddr,
			MAC_ADDR_LEN,
			pucPeerNmiAddr,
			MAC_ADDR_LEN);

		if (prNmiCxtKey->u2WtblIdx == WTBL_RESERVED_ENTRY) {
			prNmiCxtKey->u2WtblIdx =
				secPrivacySeekForNanEntry(prAdapter,
				prAdapter->rDataPathInfo.aucLocalNMIAddr,
				pucPeerNmiAddr,
				1, 0, &fgWtblReUsed);
			prNmiCxtKey->fgIsKeyExist = FALSE;
			prNmiCxtKey->fgIsNmiTk = FALSE;
			if (prNmiCxtKey->u2WtblIdx != WTBL_RESERVED_ENTRY) {
				nanSecManageKeyCmd(prAdapter,
					NAN_KEY_OP_ACTIVATE,
					NAN_KEY_TYPE_NMI_CXT_MGMT_KEY,
					prNmiCxtKey->u2WtblIdx,
					prAdapter->rDataPathInfo.
						aucLocalNMIAddr,
					pucPeerNmiAddr,
					0, 0, 0,
					NULL, NULL,
					TRUE,
					prNmiCxtKey->fgIsKeyExist);
			} else {
				DBGLOG(NAN, ERROR,
					"[NmiCxt] u2NmiCxtWidx error\n");
				prNmiCxtKey->fgValid = FALSE;
				return NULL;
			}
		}
		atomic_inc(&(prNmiCxtKey->ulRefCount));
		return prNmiCxtKey;
	}

	return NULL;
}

void nanSecReleaseNmiCxtKey(struct ADAPTER *prAdapter,
	struct _NAN_KEY_ENTRY_T *prNmiCxtKey)
{
	uint8_t fgRelease = FALSE;

	if (!prNmiCxtKey || !prNmiCxtKey->fgValid)
		return;

	if (atomic_read(&(prNmiCxtKey->ulRefCount)) == 0) {
		DBGLOG(NAN, ERROR, "[NmiCxt] NMI WTBL:%u RefCnt Err\n",
		prNmiCxtKey->u2WtblIdx);
		fgRelease = TRUE;
	} else if (atomic_dec_return(&(prNmiCxtKey->ulRefCount)) == 0) {
		fgRelease = TRUE;
	}

	if (fgRelease) {
		if (prNmiCxtKey->u2WtblIdx != WTBL_RESERVED_ENTRY) {
			nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_INACTIVATE,
				NAN_KEY_TYPE_NMI_CXT_MGMT_KEY,
				prNmiCxtKey->u2WtblIdx,
				prAdapter->rDataPathInfo.aucLocalNMIAddr,
				prNmiCxtKey->aucNmiAddr,
				0, 0, 0, NULL, NULL,
				TRUE, FALSE);

			secPrivacyFreeForEntry(prAdapter,
				prNmiCxtKey->u2WtblIdx);
			DBGLOG(NAN, INFO, "[NmiCxt] free NMI WTBL:%u\n",
				prNmiCxtKey->u2WtblIdx);

			prNmiCxtKey->u2WtblIdx = WTBL_RESERVED_ENTRY;
			prNmiCxtKey->fgIsKeyExist = FALSE;
			prNmiCxtKey->fgIsNmiTk = FALSE;
		} else {
			DBGLOG(NAN, ERROR, "[NmiCxt] NMI WTBL Err\n");
		}

		prNmiCxtKey->fgValid = FALSE;
		kalMemZero(prNmiCxtKey->aucNmiAddr, MAC_ADDR_LEN);
	}
}

void nanSecReleaseAllNmiCxtKey(struct ADAPTER *prAdapter)
{
	uint8_t ucIdx = 0;
	struct _NAN_KEY_ENTRY_T *prNmiCxtKey = NULL;

	for (ucIdx = 0; ucIdx < NAN_NUM_NMI_CXT_KEY; ucIdx++) {
		prNmiCxtKey = &g_arNanNmiCxtKey[ucIdx];
		if (prNmiCxtKey->fgValid == TRUE) {
			/*
			 * This API is used to release all existing NMI key
			 * in case if any module doesn't release this resources
			 * when NAN stack is unload so it will
			 * directly reset ref
			 * count to 1 then call nanSecReleaseNmiCxtKey.
			 */
			atomic_set(&(prNmiCxtKey->ulRefCount), 1);
			nanSecReleaseNmiCxtKey(prAdapter, prNmiCxtKey);
		}
	}
}

struct _NAN_KEY_ENTRY_T *nanSecGetNmiCxtKey(
	struct ADAPTER *prAdapter,
	uint8_t *pucPeerNMI)
{
	uint8_t ucIdx = 0;
	struct _NAN_KEY_ENTRY_T *prNmiKey = NULL;

	for (ucIdx = 0; ucIdx < NAN_NUM_NMI_CXT_KEY; ucIdx++) {
		prNmiKey = &g_arNanNmiCxtKey[ucIdx];
		if (prNmiKey->fgValid && kalMemCmp(
			prNmiKey->aucNmiAddr, pucPeerNMI,
			MAC_ADDR_LEN) == 0)
			return prNmiKey;
	}

	return NULL;
}

void nanSecNmiCxtKeyInit(IN struct ADAPTER *prAdapter)
{
	uint8_t ucIdx = 0;
	struct _NAN_KEY_ENTRY_T *prNmiKey = NULL;

	kalMemZero(g_arNanNmiCxtKey, sizeof(g_arNanNmiCxtKey));

	for (ucIdx = 0; ucIdx < NAN_NUM_NMI_CXT_KEY; ucIdx++) {
		prNmiKey = &g_arNanNmiCxtKey[ucIdx];

		prNmiKey->ucIdx = ucIdx;
		prNmiKey->u2WtblIdx = WTBL_RESERVED_ENTRY;
		atomic_set(&(prNmiKey->ulRefCount), 0);
	}
}

void
nanSecConfigNdiGtk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalAddr, IN uint8_t *pucPeerAddr,
	IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	uint8_t *pu1Gtk = NULL;
	size_t szGtkLen = 0;
	uint32_t u4Cipher = 0;
	uint8_t ucAlgoId = 0;

	if (fgSet) {
		pu1Gtk = prNdp->prResponderSecSmInfo->gtk.gtk;
		szGtkLen = prNdp->prResponderSecSmInfo->gtk.gtk_len;
		u4Cipher = prNdp->prResponderSecSmInfo->group_cipher;

		if (u4Cipher == NAN_CIPHER_SUITE_ID_NCS_GTK_GCM_256)
			ucAlgoId = CIPHER_SUITE_GCMP_256;
		else
			ucAlgoId = CIPHER_SUITE_CCMP;

		nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_SET_KEY, NAN_KEY_TYPE_MC_RX_KEY,
				u2WtblEntry,
				pucLocalAddr, pucPeerAddr,
				ucAlgoId, 1, szGtkLen, pu1Gtk, NULL,
				FALSE, TRUE);
	} else {
		nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_CLS_KEY, NAN_KEY_TYPE_MC_RX_KEY,
				u2WtblEntry,
				pucLocalAddr, pucPeerAddr,
				0, 0, 0, NULL, NULL,
				FALSE, FALSE);
	}
}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
#if (CFG_SUPPORT_NAN_R4_VENDOR_PAIRING == 1)
uint16_t
nanSecEncryptKde(uint8_t *kek, size_t kek_len,
	uint8_t *kde, size_t kde_len,
	uint8_t *ciphertext)
{
	size_t key_data_len = 0, pad_len = 0;
	uint8_t *buf = NULL, *pos = NULL;
	uint8_t *key_data = NULL;

	key_data_len = kde_len;
	pad_len = key_data_len % 8;
	if (pad_len)
		pad_len = 8 - pad_len;
	key_data_len += pad_len + 8;

	if (kde) {
		buf = os_zalloc(key_data_len);
		key_data = os_zalloc(key_data_len);
		if (buf == NULL) {
			DBGLOG(NAN, ERROR, "Buffur allocate fail\n");
			return 0;
		}
		pos = buf;
		nan_os_memcpyS(pos, key_data_len, kde, kde_len);
		pos += kde_len;

		if (pad_len)
			*pos++ = 0xdd;

		DBGLOG(NAN, INFO,
			"key_data_len=%zu, pad_len=%zu\n",
			key_data_len, pad_len);

		if (aes_wrap(kek, kek_len,
			(int)((key_data_len - 8) / 8),
			buf, key_data,
			key_data_len)) {
			DBGLOG(NAN, ERROR, "AES Wrap fail\n");
			os_free(buf);
			os_free(key_data);
			return 0;
		}
		kalMemCpyS(ciphertext, key_data_len, key_data, key_data_len);
		os_free(buf);
		os_free(key_data);
	}

	return key_data_len;
}

uint8_t *
nanSecComposeNikKde(uint8_t *kde, uint8_t nik_length,
	uint8_t *nik, uint8_t cipher_version)
{
	struct NIK_KDE_INFO *prNikKde = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	prNikKde = (struct NIK_KDE_INFO *)kde;
	prNikKde->type = 0xDD;
	prNikKde->length = 21;
	prNikKde->oui[0] = 0x50;
	prNikKde->oui[1] = 0x6F;
	prNikKde->oui[2] = 0x9A;
	prNikKde->data_type = NAN_KDE_NIK;
	prNikKde->cipher_ver = cipher_version;
	kalMemZero(prNikKde->nik, NAN_NIK_LEN);
	kalMemCpyS(prNikKde->nik, nik_length, nik, nik_length);

	return kde + sizeof(struct NIK_KDE_INFO);
}

uint8_t *
nanSecComposeNikLifetimeKde(uint8_t *kde,
	uint16_t key_bitmap,
	uint32_t key_lifttime)
{
	struct NIK_LIFETIME_KDE_INFO *prNikLifetimeKde = NULL;

	DBGLOG(NAN, INFO, "Enter\n");

	prNikLifetimeKde = (struct NIK_LIFETIME_KDE_INFO *)kde;
	prNikLifetimeKde->type = 0xDD;
	prNikLifetimeKde->length = 10;
	prNikLifetimeKde->oui[0] = 0x50;
	prNikLifetimeKde->oui[1] = 0x6F;
	prNikLifetimeKde->oui[2] = 0x9A;
	prNikLifetimeKde->data_type = NAN_KDE_KEY_LIFETIME;
	prNikLifetimeKde->key_bitmap = key_bitmap;
	prNikLifetimeKde->lifetime = key_lifttime;

	return kde + sizeof(struct NIK_LIFETIME_KDE_INFO);
}

uint16_t
nanSecAppendKde(uint8_t fgNik, uint8_t nik_length,
	uint8_t *nik, uint8_t cipher_version,
	uint8_t fgNikLifetime, uint16_t key_bitmap,
	uint32_t key_lifttime,
	uint8_t fgGroupKey, uint8_t *kek,
	size_t kek_len, uint8_t *payload)
{
	uint8_t *kde = NULL, *pos = NULL;
	size_t kde_len = 0;
	uint16_t key_data_len = 0;
	struct wpa_state_machine *sm = &g_arNanWpaAuthSm[0];

	DBGLOG(NAN, INFO, "Append KDEs\n");
	sm->mgmt_frame_prot = TRUE;
	sm->wpa_auth_nmi = g_prNanHapdData->wpa_auth;

	if (fgNik)
		kde_len += sizeof(struct NIK_KDE_INFO);
	if (fgNikLifetime)
		kde_len += sizeof(struct NIK_LIFETIME_KDE_INFO);
	if (fgGroupKey)
		kde_len += ieee80211w_kde_len(sm);

	DBGLOG(NAN, INFO, "KDEs Length = %lu\n", kde_len);
	kde = os_malloc(kde_len);
	pos = kde;

	if ((kde_len == 0) || (kde == NULL) || (pos == NULL)) {
		DBGLOG(NAN, INFO, "return!\n");
		return 0;
	}

	if (fgNik)
		pos = nanSecComposeNikKde(pos, nik_length, nik, cipher_version);
	if (fgNikLifetime)
		pos = nanSecComposeNikLifetimeKde(pos,
			key_bitmap, key_lifttime);
	if (fgGroupKey)
		pos = ieee80211w_kde_add(sm, pos);

	DBGLOG(NAN, INFO, "Plain text KDEs\n");
	dumpMemory8((uint8_t *)kde, kde_len);
	key_data_len = nanSecEncryptKde(kek, kek_len, kde, kde_len, payload);
	DBGLOG(NAN, INFO, "Cipher text KDEs\n");
	dumpMemory8((uint8_t *)payload, key_data_len);

	os_free(kde);

	return key_data_len;
}


void
nanSecComposeEapolKey(struct _NAN_ATTR_SKDA_T *prAttrSkda,
	uint8_t *prKdes, uint16_t u2KeyLength,
	uint8_t *kck, size_t kck_len, uint32_t u4CipherType)
{
	struct wpa_eapol_key *eapol_key = NULL;
	struct wpa_eapol_key_192 *eapol_key_192 = NULL;
	uint16_t key_info = 0;
	uint16_t u2TotalLength = 0;

	DBGLOG(NAN, INFO, "Compose Eapol Key Descriptor\n");
	DBGLOG(NAN, INFO, "Cipher[%u] KeyLength[%u]\n",
		u4CipherType, u2KeyLength);

	key_info = WPA_KEY_INFO_ENCR_KEY_DATA |
			WPA_KEY_INFO_KEY_TYPE |
			WPA_KEY_INFO_MIC |
			WPA_KEY_INFO_SECURE;

	if (u4CipherType == NAN_CIPHER_SUITE_ID_NCS_PK_PASN_128) {
		eapol_key = (struct wpa_eapol_key *)
			(prAttrSkda->aucKeyDescriptor);
		kalMemZero(eapol_key, sizeof(*eapol_key));
		eapol_key->type = EAPOL_KEY_TYPE_RSN;
		WPA_PUT_BE16(eapol_key->key_info, key_info);
		WPA_PUT_BE16(eapol_key->key_data_length, u2KeyLength);
		kalMemCpyS(
			prAttrSkda->aucKeyDescriptor + sizeof(*eapol_key),
			u2KeyLength, prKdes,
			u2KeyLength);
		u2TotalLength = u2KeyLength + sizeof(*eapol_key);
		nan_sec_wpa_eapol_key_mic(kck, kck_len, u4CipherType,
			(u8 *)prAttrSkda->aucKeyDescriptor,
			u2TotalLength, eapol_key->key_mic);
	} else {
		eapol_key_192 = (struct wpa_eapol_key_192 *)
			(prAttrSkda->aucKeyDescriptor);
		kalMemZero(eapol_key_192, sizeof(*eapol_key_192));
		eapol_key_192->type = EAPOL_KEY_TYPE_RSN;
		WPA_PUT_BE16(eapol_key_192->key_info, key_info);
		WPA_PUT_BE16(eapol_key_192->key_data_length, u2KeyLength);
		kalMemCpyS(
			prAttrSkda->aucKeyDescriptor + sizeof(*eapol_key_192),
			u2KeyLength, prKdes,
			u2KeyLength);
		u2TotalLength = u2KeyLength + sizeof(*eapol_key_192);
		nan_sec_wpa_eapol_key_mic(kck, kck_len, u4CipherType,
			(u8 *)prAttrSkda->aucKeyDescriptor,
			u2TotalLength, eapol_key_192->key_mic);
	}

	prAttrSkda->u2Len = u2TotalLength +
		sizeof_field(struct _NAN_ATTR_SKDA_T, ucPublishID);
}

void
nanSecRxSkdaToHostFormat(uint8_t *prKeyDescriptor, uint32_t u4CipherType)
{
	struct wpa_eapol_key *eapol_key = NULL;
	struct wpa_eapol_key_192 *eapol_key_192 = NULL;

	DBGLOG(NAN, INFO,
		"followup Convert the SKDA endianness to host format[%u]\n",
		u4CipherType);

	if (u4CipherType == NAN_CIPHER_SUITE_ID_NCS_PK_PASN_128) {
		eapol_key = (struct wpa_eapol_key *)prKeyDescriptor;
		WPA_PUT_LE16(eapol_key->key_info,
			WPA_GET_BE16(eapol_key->key_info));
		WPA_PUT_LE16(eapol_key->key_data_length,
			WPA_GET_BE16(eapol_key->key_data_length));
	} else {
		eapol_key_192 = (struct wpa_eapol_key_192 *)prKeyDescriptor;
		WPA_PUT_LE16(eapol_key_192->key_info,
			WPA_GET_BE16(eapol_key_192->key_info));
		WPA_PUT_LE16(eapol_key_192->key_data_length,
			WPA_GET_BE16(eapol_key_192->key_data_length));
	}
}
#endif /* CFG_SUPPORT_NAN_R4_VENDOR_PAIRING  */
void
nanSecGetTxIgtkBigtk(uint32_t *pucIgtkCipher,
	uint8_t *pucIgtkKey, size_t *pucIgtkLen,
	uint32_t *pucBigtkCipher, uint8_t *pucBigtkKey,
	size_t *pucBigtkLen)
{
	*pucIgtkCipher =
		(uint32_t)g_prNanHapdData->wpa_auth->conf.group_mgmt_cipher;
	*pucIgtkLen = wpa_cipher_key_len(
		g_prNanHapdData->wpa_auth->conf.group_mgmt_cipher);
	kalMemCpyS(pucIgtkKey, *pucIgtkLen,
		g_prNanHapdData->wpa_auth->group->IGTK[0],
		*pucIgtkLen);

	if (g_prNanHapdData->wpa_auth->conf.beacon_prot) {
		*pucBigtkCipher =
			(uint32_t)g_prNanHapdData->wpa_auth->
			conf.group_mgmt_cipher;
		*pucBigtkLen =
			wpa_cipher_key_len(
				g_prNanHapdData->wpa_auth->
					conf.group_mgmt_cipher);
		kalMemCpyS(pucBigtkKey,
			*pucBigtkLen,
			g_prNanHapdData->wpa_auth->group->BIGTK[0],
			*pucBigtkLen);
	}
}

void
nanSecConfigNmiIgtk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalNMI, IN uint8_t *pucPeerNMI,
	IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	uint8_t *pu1Igtk = NULL;
	uint8_t *pucIgtkPn = NULL;
	size_t szIgtkLen = 0;
	uint32_t u4Cipher = 0;
	uint8_t ucIgtkAlgoId = 0;

	if (fgSet) {
		pu1Igtk = prNdp->prResponderSecSmInfo->igtk.igtk;
		szIgtkLen = prNdp->prResponderSecSmInfo->igtk.igtk_len;
		u4Cipher = prNdp->prResponderSecSmInfo->mgmt_group_cipher;
		pucIgtkPn = prNdp->prResponderSecSmInfo->igtk_pn;

		if (u4Cipher == WPA_CIPHER_AES_128_CMAC)
			ucIgtkAlgoId = CIPHER_SUITE_BIP;
		else
			ucIgtkAlgoId = CIPHER_SUITE_BIP_GMAC_256;

		nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_SET_KEY, NAN_KEY_TYPE_MC_MGMT_RX_KEY,
				u2WtblEntry,
				pucLocalNMI, pucPeerNMI,
				ucIgtkAlgoId, 4, szIgtkLen, pu1Igtk, pucIgtkPn,
				FALSE, TRUE);
	} else {
		nanSecManageKeyCmd(prAdapter,
				NAN_KEY_OP_CLS_KEY, NAN_KEY_TYPE_MC_MGMT_RX_KEY,
				u2WtblEntry,
				pucLocalNMI, pucPeerNMI,
				ucIgtkAlgoId, 0, 0, NULL, NULL,
				FALSE, FALSE);
	}
}

void
nanSecConfigNmiBigtk(IN struct ADAPTER *prAdapter,
	IN uint8_t fgSet, IN uint16_t u2WtblEntry,
	IN uint8_t *pucLocalNMI, IN uint8_t *pucPeerNMI,
	IN struct _NAN_NDP_INSTANCE_T *prNdp)
{
	uint8_t *pu1Bigtk = NULL;
	uint8_t *pucBigtkPn = NULL;
	size_t szBigtkLen = 0;
	uint32_t u4Cipher = 0;
	uint8_t ucBigtkAlgoId = 0;

	if (fgSet) {
		pu1Bigtk = prNdp->prResponderSecSmInfo->bigtk.bigtk;
		szBigtkLen = prNdp->prResponderSecSmInfo->bigtk.bigtk_len;
		u4Cipher = prNdp->prResponderSecSmInfo->mgmt_group_cipher;
		pucBigtkPn = prNdp->prResponderSecSmInfo->bigtk_pn;

		if (u4Cipher == WPA_CIPHER_AES_128_CMAC)
			ucBigtkAlgoId = CIPHER_SUITE_BCN_PROT_CMAC_128;
		else
			ucBigtkAlgoId = CIPHER_SUITE_BCN_PROT_GMAC_256;

		nanSecManageKeyCmd(prAdapter,
			NAN_KEY_OP_SET_KEY, NAN_KEY_TYPE_MC_MGMT_RX_KEY,
			u2WtblEntry,
			pucLocalNMI, pucPeerNMI,
			ucBigtkAlgoId, 6, szBigtkLen,
			pu1Bigtk, pucBigtkPn,
			FALSE, TRUE);
	} else {
		nanSecManageKeyCmd(prAdapter,
			NAN_KEY_OP_CLS_KEY, NAN_KEY_TYPE_MC_MGMT_RX_KEY,
			u2WtblEntry,
			pucLocalNMI, pucPeerNMI,
			ucBigtkAlgoId, 0, 0,
			NULL, NULL,
			FALSE, FALSE);
	}
}

uint8_t
nanSecGetGtkCipherByNdi(uint8_t ucNdiIdx)
{
	struct wpa_authenticator *wpa_auth = NULL;

	wpa_auth = &g_rNanWpaAuth[ucNdiIdx];

	return nanSecGtkCipherWfaToNan((uint16_t)wpa_auth->conf.wpa_group);
}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */


#if (CFG_SUPPORT_NAN_R4_VENDOR_PAIRING == 1)
int32_t
nanSecSetKeyHandler(struct ADAPTER *prAdapter,
	struct NanPairingSetKey *prNanPairingKey)
{
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucSelfNdiIdx = NAN_NDI_INDEX_0;
	uint8_t fgSelfKey = FALSE;
	uint16_t u2WtblIdx = WTBL_RESERVED_ENTRY;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t ucKeyInstallStatus = NO_KEY_EXIST_IND;
	uint8_t i = 0;
	struct NanPairingKeyInfo *nan_key_info = NULL;
	struct PAIRING_INFO *prPairingInfo = NULL;
	uint8_t fgWtblReUsed = FALSE;

	DBGLOG(NAN, INFO, "Enter\n");

	/* Get BSS info */
	prNanSpecificBssInfo = nanGetSpecificBssInfo(
		prAdapter,
		NAN_BSS_INDEX_MAIN);
	if (prNanSpecificBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prNanSpecificBssInfo is null\n");
		return -ENOMEM;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(
		prAdapter,
		prNanSpecificBssInfo->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prBssInfo is null\n");
		return -ENOMEM;
	}

	dumpMemory8((uint8_t *)prNanPairingKey,
		sizeof(struct NanPairingSetKey));

	ucSelfNdiIdx = nanDataEngineGetNdiIdxByAddr(prAdapter,
		prNanPairingKey->peer_address);
	if (ucSelfNdiIdx != NAN_MAX_MULTI_NDI_NUM) {
		DBGLOG(NAN, INFO, "ucSelfNdiIdx = %d\n", ucSelfNdiIdx);
		fgSelfKey = TRUE;
	} else if ((prNanPairingKey->igtk.key_length != 0) ||
		(prNanPairingKey->bigtk.key_length != 0)) {
		u2WtblIdx = secPrivacySeekForNanEntry(prAdapter,
			aucBCAddr, prNanPairingKey->peer_address,
			FALSE, 0, &fgWtblReUsed);
		if (u2WtblIdx == WTBL_RESERVED_ENTRY) {
			DBGLOG(NAN, ERROR,
				"[MCDATA] MC Rx wtbl alloc fail\n");
		} else {
			DBGLOG(NAN, INFO,
				"[MCDATA] MC Rx wtbl alloc idx[%hu] during nik exchange\n",
				u2WtblIdx);
			nanRegisterMcRxWtblIdx(prAdapter,
				prNanPairingKey->peer_address,
				u2WtblIdx, fgWtblReUsed);
			ucKeyInstallStatus =
				nanGetIgtkBigtkInMcRxWtbl(prAdapter,
				prNanPairingKey->peer_address);
			if (prNanPairingKey->igtk.key_length != 0)
				ucKeyInstallStatus |= IGTK_EXIST_IND;
			if (prNanPairingKey->bigtk.key_length != 0)
				ucKeyInstallStatus |= BIGTK_EXIST_IND;
			nanSetIgtkBigtkInMcRxWtbl(prAdapter,
				prNanPairingKey->peer_address,
				ucKeyInstallStatus);
		}
	}
	for (i = ENCRYPTION_KEY_NM_TK; i < ENCRYPTION_KEY_NUM; i++) {
		switch (i) {
		case ENCRYPTION_KEY_NM_TK:
		{
			struct _NAN_KEY_ENTRY_T *prNmiCxtKey = NULL;
			/* Set key */
			nan_key_info = &prNanPairingKey->nm_tk;

			if (nan_key_info->key_length == 0)
				break;

			DBGLOG(NAN, INFO, "peerNMI:" MACSTR "\n",
				MAC2STR(prNanPairingKey->peer_address));

			prPairingInfo = nanPairingInfoSearch(prAdapter,
				prNanPairingKey->peer_address);

			if (prPairingInfo == NULL)
				prPairingInfo =
					nanPairingInfoAlloc(prAdapter,
					prNanPairingKey->peer_address);
			else
				DBGLOG(NAN, INFO, "prPairingInfo exist\n");

			if (prPairingInfo) {
				/* NAN_CIPHER_SUITE_ID_NCS_PK_PASN_128 */
				prPairingInfo->u4SelCipherType =
					nan_key_info->cipher_suite_id;

				kalMemCopy(prPairingInfo->rPtk.tk,
					nan_key_info->key, NAN_TK_MAX_LEN);
				prPairingInfo->rPtk.tk_len =
					nan_key_info->key_length;

				prNmiCxtKey = prPairingInfo->prNmiCxtKey;
				if (prNmiCxtKey) {
					prNmiCxtKey->fgIsKeyExist = TRUE;
					prNmiCxtKey->fgIsNmiTk = TRUE;

					nanPairingInstallTk(prAdapter,
						&prPairingInfo->rPtk,
						prPairingInfo->u4SelCipherType,
						prNmiCxtKey->aucNmiAddr,
						prNmiCxtKey->u2WtblIdx);
				} else {
					DBGLOG(NAN, ERROR,
						"Failed to get prNmiCxtKey\n");
				}
			} else {
				return -ENOMEM;
			}
		}
		break;

		case ENCRYPTION_KEY_NM_KEK:
			nan_key_info = &prNanPairingKey->nm_kek;
			if (nan_key_info->key_length == 0)
				break;

			prPairingInfo = nanPairingInfoSearch(prAdapter,
				prNanPairingKey->peer_address);

			if (prPairingInfo == NULL)
				prPairingInfo = nanPairingInfoAlloc(prAdapter,
					prNanPairingKey->peer_address);
			else
				DBGLOG(NAN, INFO, "prPairingInfo exist\n");

			if (prPairingInfo) {
				kalMemCopy(prPairingInfo->rPtk.kek,
					nan_key_info->key,
					NAN_TK_MAX_LEN);
				prPairingInfo->rPtk.kek_len =
					nan_key_info->key_length;
			} else {
				return -ENOMEM;
			}
			break;

		case ENCRYPTION_KEY_NM_KCK:
			nan_key_info = &prNanPairingKey->nm_kck;
			if (nan_key_info->key_length == 0)
				break;

			prPairingInfo = nanPairingInfoSearch(prAdapter,
				prNanPairingKey->peer_address);

			if (prPairingInfo == NULL)
				prPairingInfo = nanPairingInfoAlloc(prAdapter,
					prNanPairingKey->peer_address);
			else
				DBGLOG(NAN, INFO, "prPairingInfo exist\n");

			if (prPairingInfo) {
				kalMemCopy(prPairingInfo->rPtk.kck,
					nan_key_info->key,
					NAN_TK_MAX_LEN);
				prPairingInfo->rPtk.kck_len =
					nan_key_info->key_length;
			} else {
				return -ENOMEM;
			}

			break;

		case ENCRYPTION_KEY_IGTK:
			nan_key_info = &prNanPairingKey->igtk;
			if (nan_key_info->key_length == 0)
				break;
			nanSecSetIgtkToSm(prAdapter,
				prNanPairingKey->peer_address,
				nan_key_info->cipher_suite_id,
				nan_key_info->key_length,
				nan_key_info->key,
				fgSelfKey,
				u2WtblIdx);
			break;

		case ENCRYPTION_KEY_BIGTK:
			nan_key_info = &prNanPairingKey->bigtk;
			if (nan_key_info->key_length == 0)
				break;
			nanSecSetBigtkToSm(prAdapter,
				prNanPairingKey->peer_address,
				nan_key_info->cipher_suite_id,
				nan_key_info->key_length,
				nan_key_info->key,
				fgSelfKey,
				u2WtblIdx);
			break;

		case ENCRYPTION_KEY_GTK:
			nan_key_info = &prNanPairingKey->gtk;
			if (nan_key_info->key_length == 0)
				break;
			nanSecSetGtkToSm(prAdapter,
				prNanPairingKey->peer_address,
				nan_key_info->cipher_suite_id,
				nan_key_info->key_length,
				nan_key_info->key,
				ucSelfNdiIdx);
			break;
		case ENCRYPTION_KEY_ND_TK:
			nan_key_info = &prNanPairingKey->nd_tk;
			if (nan_key_info->key_length == 0)
				break;
			DBGLOG(NAN, INFO, "ND-TK TBD...\n");
			break;
		default:
			break;
		}

		if ((nan_key_info == NULL) ||
			(nan_key_info->key_length == 0)) {
			continue;
		}

		DBGLOG(NAN, INFO,
			"key type: %u, cipher_suite_id: %u, key_length: %u\n",
			i, nan_key_info->cipher_suite_id,
			nan_key_info->key_length);
		dumpMemory8((uint8_t *)nan_key_info->key,
			nan_key_info->key_length);
	}
	return 0;
}
#endif /* CFG_SUPPORT_NAN_R4_VENDOR_PAIRING */
