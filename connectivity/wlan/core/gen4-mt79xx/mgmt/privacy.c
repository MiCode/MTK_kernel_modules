/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 * Id: mgmt/privacy.c#1
 */

/*! \file   "privacy.c"
 *    \brief  This file including the protocol layer privacy function.
 *
 *    This file provided the macros and functions library support for the
 *    protocol layer security setting from rsn.c and nic_privacy.c
 *
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

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to initialize the privacy-related
 *        parameters.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucNetTypeIdx  Pointer to netowrk type index
 *
 * \retval NONE
 */
/*----------------------------------------------------------------------------*/
void secInit(IN struct ADAPTER *prAdapter, IN uint8_t ucBssIndex)
{
	uint8_t i;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
	struct IEEE_802_11_MIB *prMib;

	DEBUGFUNC("secInit");

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prAisSpecBssInfo =
		aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prMib = aisGetMib(prAdapter, ucBssIndex);

	prBssInfo->u4RsnSelectedGroupCipher = 0;
	prBssInfo->u4RsnSelectedPairwiseCipher = 0;
	prBssInfo->u4RsnSelectedAKMSuite = 0;

#if 0				/* CFG_ENABLE_WIFI_DIRECT */
	prBssInfo = &prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_P2P];

	prBssInfo->u4RsnSelectedGroupCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedAKMSuite = RSN_AKM_SUITE_PSK;
#endif

#if 0				/* CFG_ENABLE_BT_OVER_WIFI */
	prBssInfo = &prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_BOW];

	prBssInfo->u4RsnSelectedGroupCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedAKMSuite = RSN_AKM_SUITE_PSK;
#endif

	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[0].dot11RSNAConfigPairwiseCipher
	    = WPA_CIPHER_SUITE_WEP40;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[1].dot11RSNAConfigPairwiseCipher
	    = WPA_CIPHER_SUITE_TKIP;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[2].dot11RSNAConfigPairwiseCipher
	    = WPA_CIPHER_SUITE_CCMP;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[3].dot11RSNAConfigPairwiseCipher
	    = WPA_CIPHER_SUITE_WEP104;

	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[4].dot11RSNAConfigPairwiseCipher
	    = RSN_CIPHER_SUITE_WEP40;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[5].dot11RSNAConfigPairwiseCipher
	    = RSN_CIPHER_SUITE_TKIP;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[6].dot11RSNAConfigPairwiseCipher
	    = RSN_CIPHER_SUITE_CCMP;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[7].dot11RSNAConfigPairwiseCipher
	    = RSN_CIPHER_SUITE_WEP104;
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[8].dot11RSNAConfigPairwiseCipher
	    = RSN_CIPHER_SUITE_GROUP_NOT_USED;
	prMib->
	dot11RSNAConfigPairwiseCiphersTable[9].dot11RSNAConfigPairwiseCipher
		= RSN_CIPHER_SUITE_GCMP_256;

	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
		prMib->dot11RSNAConfigPairwiseCiphersTable
		    [i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [0].dot11RSNAConfigAuthenticationSuite = WPA_AKM_SUITE_NONE;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [1].dot11RSNAConfigAuthenticationSuite = WPA_AKM_SUITE_802_1X;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [2].dot11RSNAConfigAuthenticationSuite = WPA_AKM_SUITE_PSK;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [3].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_NONE;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [4].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_802_1X;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [5].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_PSK;

	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [6].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_FT_802_1X;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [7].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_FT_PSK;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [8].dot11RSNAConfigAuthenticationSuite = WFA_AKM_SUITE_OSEN;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [9].dot11RSNAConfigAuthenticationSuite =
					RSN_AKM_SUITE_8021X_SUITE_B;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [10].dot11RSNAConfigAuthenticationSuite =
					RSN_AKM_SUITE_8021X_SUITE_B_192;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [11].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_SAE;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [12].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_OWE;
#if CFG_SUPPORT_802_11W
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [13].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_802_1X_SHA256;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [14].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_PSK_SHA256;
#endif
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [15].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_DPP;

	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
		prMib->dot11RSNAConfigAuthenticationSuitesTable
		    [i].dot11RSNAConfigAuthenticationSuiteEnabled = FALSE;
	}

#if CFG_SUPPORT_802_11W
	cnmTimerInitTimer(prAdapter,
			  &prAisSpecBssInfo->rSaQueryTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) rsnStartSaQueryTimer,
			  (unsigned long)ucBssIndex);
#endif

	prAisSpecBssInfo->fgCounterMeasure = FALSE;
	prBssInfo->ucBcDefaultKeyIdx = 0xff;
	prBssInfo->fgBcDefaultKeyExist = FALSE;

#if 0
	for (i = 0; i < WTBL_SIZE; i++) {
		g_prWifiVar->arWtbl[i].ucUsed = FALSE;
		g_prWifiVar->arWtbl[i].prSta = NULL;
		g_prWifiVar->arWtbl[i].ucNetTypeIdx = NETWORK_TYPE_INDEX_NUM;

	}
	nicPrivacyInitialize((uint8_t) NETWORK_TYPE_INDEX_NUM);
#endif

}				/* secInit */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will indicate an Event of "Rx Class Error" to SEC_FSM
 *        for JOIN Module.
 *
 * \param[in] prAdapter     Pointer to the Adapter structure
 * \param[in] prSwRfb       Pointer to the SW RFB.
 *
 * \return FALSE                Class Error
 */
/*----------------------------------------------------------------------------*/
u_int8_t secCheckClassError(IN struct ADAPTER *prAdapter,
			    IN struct SW_RFB *prSwRfb,
			    IN struct STA_RECORD *prStaRec)
{
	void *prRxStatus;
	struct RX_DESC_OPS_T *prRxDescOps;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	ASSERT(prRxDescOps->nic_rxd_get_sw_class_error_bit);

	prRxStatus = prSwRfb->prRxStatus;

	if (prRxDescOps->nic_rxd_get_sw_class_error_bit(prRxStatus)
	    || (IS_STA_IN_AIS(prStaRec)
		&& aisGetAisBssInfo(prAdapter,
		prStaRec->ucBssIndex)->eConnectionState ==
		MEDIA_STATE_DISCONNECTED)) {

		DBGLOG_LIMITED(RSN, WARN,
			"RX_CLASSERR: prStaRec=%p PktTYpe=0x%x, WlanIdx=%d,",
			prStaRec,
			prSwRfb->ucPacketType, prSwRfb->ucWlanIdx);
		DBGLOG_LIMITED(RSN, WARN,
			"StaRecIdx=%d, eDst=%d, prStaRec->eStaType=%d\n",
			prSwRfb->ucStaRecIdx,
			prSwRfb->eDst, prStaRec->eStaType);

		DBGLOG_MEM8(RX, ERROR, prSwRfb->pucRecvBuff,
			    (prSwRfb->u2RxByteCount > 64) ? 64 :
			    prSwRfb->u2RxByteCount);

		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
			DBGLOG(RX, TRACE,
				"secchk:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
				prSwRfb->prRxStatusGroup4->u2FrameCtl,
				prSwRfb->prRxStatusGroup4->aucTA[0],
				prSwRfb->prRxStatusGroup4->aucTA[1],
				prSwRfb->prRxStatusGroup4->aucTA[2],
				prSwRfb->prRxStatusGroup4->aucTA[3],
				prSwRfb->prRxStatusGroup4->aucTA[4],
				prSwRfb->prRxStatusGroup4->aucTA[5],
				prSwRfb->prRxStatusGroup4->u2SeqFrag,
				prSwRfb->prRxStatusGroup4->u2Qos,
				prSwRfb->prRxStatusGroup4->u4HTC);
		}

		if (EAPOL_KEY_NOT_KEY !=
			secGetEapolKeyType((uint8_t *) prSwRfb->pvHeader)) {
			DBGLOG(RSN, WARN,
			       "EAPOL key found, return TRUE back");

			return TRUE;
		}

		/* if (IS_NET_ACTIVE(prAdapter, ucBssIndex)) { */
		authSendDeauthFrame(prAdapter,
				    NULL, NULL, prSwRfb,
				    REASON_CODE_CLASS_3_ERR,
				    (PFN_TX_DONE_HANDLER) NULL);
		return FALSE;
		/* } */
	}

	return TRUE;

}				/* end of secCheckClassError() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to setting the sta port status.
 *
 * \param[in]  prAdapter Pointer to the Adapter structure
 * \param[in]  prSta Pointer to the sta
 * \param[in]  fgPortBlock The port status
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/
void secSetPortBlocked(IN struct ADAPTER *prAdapter,
		       IN struct STA_RECORD *prSta, IN u_int8_t fgPortBlock)
{
#if 0				/* Marked for MT6630 */
	if (prSta == NULL)
		return;

	prSta->fgPortBlock = fgPortBlock;

	DBGLOG(RSN, TRACE,
	       "The STA " MACSTR " port %s\n", MAC2STR(prSta->aucMacAddr),
	       fgPortBlock == TRUE ? "BLOCK" : " OPEN");
#endif
}

#if 0				/* Marked for MT6630 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to report the sta port status.
 *
 * \param[in]  prAdapter Pointer to the Adapter structure
 * \param[in]  prSta Pointer to the sta
 * \param[out]  fgPortBlock The port status
 *
 * \return TRUE sta exist, FALSE sta not exist
 *
 */
/*----------------------------------------------------------------------------*/
u_int8_t secGetPortStatus(IN struct ADAPTER *prAdapter,
			  IN struct STA_RECORD *prSta,
			  OUT u_int8_t *pfgPortStatus)
{
	if (prSta == NULL)
		return FALSE;

	*pfgPortStatus = prSta->fgPortBlock;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to handle Peer device Tx Security process MSDU.
 *
 * \param[in] prMsduInfo pointer to the packet info pointer
 *
 * \retval TRUE Accept the packet
 * \retval FALSE Refuse the MSDU packet due port blocked
 *
 */
/*----------------------------------------------------------------------------*/
u_int8_t			/* ENUM_PORT_CONTROL_RESULT */
secTxPortControlCheck(IN struct ADAPTER *prAdapter,
		      IN struct MSDU_INFO *prMsduInfo,
		      IN struct STA_RECORD *prStaRec)
{
	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(prStaRec);

	if (prStaRec) {

		/* Todo:: */
		if (prMsduInfo->fgIs802_1x)
			return TRUE;

		if (prStaRec->fgPortBlock == TRUE) {
			DBGLOG(INIT, TRACE,
			       "Drop Tx packet due Port Control!\n");
			return FALSE;
		}
#if CFG_SUPPORT_WAPI
		if (prAdapter->rWifiVar.rConnSettings.fgWapiMode)
			return TRUE;
#endif
		if (IS_STA_IN_AIS(prStaRec)) {
			if (!prAdapter->rWifiVar.
			    rAisSpecificBssInfo.fgTransmitKeyExist
			    && (prAdapter->rWifiVar.rConnSettings.eEncStatus ==
				ENUM_ENCRYPTION1_ENABLED)) {
				DBGLOG(INIT, TRACE,
				       "Drop Tx packet due the key is removed!!!\n");
				return FALSE;
			}
		}
	}

	return TRUE;
}
#endif /* Marked for MT6630 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to handle The Rx Security process MSDU.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] prSWRfb SW rfb pinter
 *
 * \retval TRUE Accept the packet
 * \retval FALSE Refuse the MSDU packet due port control
 */
/*----------------------------------------------------------------------------*/
u_int8_t secRxPortControlCheck(IN struct ADAPTER *prAdapter,
			       IN struct SW_RFB *prSWRfb)
{
	ASSERT(prSWRfb);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine will enable/disable the cipher suite
 *
 * \param[in] prAdapter Pointer to the adapter object data area.
 * \param[in] u4CipherSuitesFlags flag for cipher suite
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void secSetCipherSuite(IN struct ADAPTER *prAdapter,
		       IN uint32_t u4CipherSuitesFlags,
		       IN uint8_t ucBssIndex)
{

	uint32_t i;
	struct DOT11_RSNA_CONFIG_PAIRWISE_CIPHERS_ENTRY *prEntry;
	struct IEEE_802_11_MIB *prMib;

	ASSERT(prAdapter);

	prMib = aisGetMib(prAdapter, ucBssIndex);

	ASSERT(prMib);

	if (u4CipherSuitesFlags == CIPHER_FLAG_NONE) {
		/* Disable all the pairwise cipher suites. */
		for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
			prMib->dot11RSNAConfigPairwiseCiphersTable
			    [i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

		/* Update the group cipher suite. */
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

		return;
	}

	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++) {
		prEntry = &prMib->dot11RSNAConfigPairwiseCiphersTable[i];

		switch (prEntry->dot11RSNAConfigPairwiseCipher) {
		case RSN_CIPHER_SUITE_GCMP_256:
			if (u4CipherSuitesFlags & CIPHER_FLAG_GCMP256)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
					TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
					FALSE;
		break;
		case WPA_CIPHER_SUITE_WEP40:
		case RSN_CIPHER_SUITE_WEP40:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP40)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    FALSE;
			break;

		case WPA_CIPHER_SUITE_TKIP:
		case RSN_CIPHER_SUITE_TKIP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_TKIP)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    FALSE;
			break;

		case WPA_CIPHER_SUITE_CCMP:
		case RSN_CIPHER_SUITE_CCMP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_CCMP)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    FALSE;
			break;

		case RSN_CIPHER_SUITE_GROUP_NOT_USED:
			if (u4CipherSuitesFlags &
			    (CIPHER_FLAG_CCMP | CIPHER_FLAG_TKIP))
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    FALSE;
			break;

		case WPA_CIPHER_SUITE_WEP104:
		case RSN_CIPHER_SUITE_WEP104:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP104)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    FALSE;
			break;
		default:
			break;
		}
	}

	/* Update the group cipher suite. */
	if (rsnSearchSupportedCipher(prAdapter,
		WPA_CIPHER_SUITE_CCMP, &i, ucBssIndex))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_CCMP;
	else if (rsnSearchSupportedCipher(prAdapter,
		WPA_CIPHER_SUITE_TKIP, &i, ucBssIndex))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_TKIP;
	else if (rsnSearchSupportedCipher(prAdapter,
		WPA_CIPHER_SUITE_WEP104, &i, ucBssIndex))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP104;
	else if (rsnSearchSupportedCipher(prAdapter,
		WPA_CIPHER_SUITE_WEP40, &i, ucBssIndex))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP40;
	else if (rsnSearchSupportedCipher(prAdapter,
		RSN_CIPHER_SUITE_GROUP_NOT_USED, &i, ucBssIndex))
		prMib->dot11RSNAConfigGroupCipher =
		    RSN_CIPHER_SUITE_GROUP_NOT_USED;
	else if (rsnSearchSupportedCipher(prAdapter,
		RSN_CIPHER_SUITE_GCMP_256, &i, ucBssIndex))
		prMib->dot11RSNAConfigGroupCipher =
			RSN_CIPHER_SUITE_GCMP_256;
	else
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

}				/* secSetCipherSuite */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Whether 802.11 privacy is enabled.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \retval BOOLEAN
 */
/*----------------------------------------------------------------------------*/
u_int8_t secEnabledInAis(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex)
{
	enum ENUM_WEP_STATUS eEncStatus;

	eEncStatus = aisGetEncStatus(prAdapter, ucBssIndex);

	DEBUGFUNC("secEnabledInAis");

	switch (eEncStatus) {
	case ENUM_ENCRYPTION_DISABLED:
		return FALSE;
	case ENUM_ENCRYPTION1_ENABLED:
	case ENUM_ENCRYPTION2_ENABLED:
	case ENUM_ENCRYPTION3_ENABLED:
	case ENUM_ENCRYPTION4_ENABLED:
		return TRUE;
	default:
		DBGLOG(RSN, TRACE, "Unknown encryption setting %d\n",
		       eEncStatus);
		break;
	}
	return FALSE;

}				/* secEnabledInAis */

u_int8_t secIsProtected1xFrame(IN struct ADAPTER *prAdapter,
			       IN struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);

	if (prStaRec) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  prStaRec->ucBssIndex);
		if (prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_WAPI
			if (aisGetWapiMode(prAdapter,
				prStaRec->ucBssIndex))
				return FALSE;
#endif
		}

		return prStaRec->fgTransmitKeyExist;
	}
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the privacy bit at mac header for TxM
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] prMsdu the msdu for known the sta record
 *
 * \return TRUE the privacy need to set
 *            FALSE the privacy no need to set
 */
/*----------------------------------------------------------------------------*/
u_int8_t secIsProtectedFrame(IN struct ADAPTER *prAdapter,
			     IN struct MSDU_INFO *prMsdu,
			     IN struct STA_RECORD *prStaRec)
{
#if CFG_SUPPORT_NAN
	struct BSS_INFO *prBssInfo;
#endif

#if CFG_SUPPORT_802_11W
	if (rsnCheckBipKeyInstalled(prAdapter, prStaRec) &&
	    (secIsRobustActionFrame(prAdapter, prMsdu->prPacket)
	    || (IS_BSS_INDEX_AIS(prAdapter, prMsdu->ucBssIndex) &&
	    secIsRobustMgmtFrame(prAdapter, prMsdu->prPacket))))
		return TRUE;
#endif
	if (prMsdu->ucPacketType == TX_PACKET_TYPE_MGMT)
		return FALSE;
#if CFG_SUPPORT_NAN
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsdu->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prBssInfo is null\n");
		return FALSE;
	}
	if (prBssInfo->eNetworkType == NETWORK_TYPE_NAN) {
		if (prStaRec && (prStaRec->fgTransmitKeyExist == TRUE))
			return TRUE;
		else
			return FALSE;
	}
#endif

	return secIsProtectedBss(prAdapter,
				 GET_BSS_INFO_BY_INDEX(prAdapter,
						       prMsdu->ucBssIndex));
}

u_int8_t secIsProtectedBss(IN struct ADAPTER *prAdapter,
			   IN struct BSS_INFO *prBssInfo)
{
	uint8_t ucBssIndex = 0;

	ucBssIndex = prBssInfo->ucBssIndex;

	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_WAPI
		if (aisGetWapiMode(prAdapter, ucBssIndex))
			return TRUE;
#endif
		return secEnabledInAis(prAdapter,
			ucBssIndex);
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
		return kalP2PGetCipher(prAdapter->prGlueInfo,
				       (uint8_t) prBssInfo->u4PrivateData);
#endif
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW)
		return TRUE;

	return FALSE;
}

u_int8_t secIsRobustMgmtFrame(IN struct ADAPTER *prAdapter, IN void *prPacket)
{
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	uint16_t u2TxFrameCtrl;

	if (!prPacket)
		return FALSE;

	prWlanHeader = (struct WLAN_MAC_HEADER *)
		((unsigned long) prPacket + MAC_TX_RESERVED_FIELD);
	u2TxFrameCtrl = prWlanHeader->u2FrameCtrl & MASK_FRAME_TYPE;
	if (u2TxFrameCtrl == MAC_FRAME_DISASSOC
	    || u2TxFrameCtrl == MAC_FRAME_DEAUTH)
		return TRUE;
	return FALSE;
}

u_int8_t secIsRobustActionFrame(IN struct ADAPTER *prAdapter, IN void *prPacket)
{
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	struct WLAN_ACTION_FRAME *prActFrame = NULL;
	uint8_t ucCategory;
	uint16_t u2TxFrameCtrl;

	if (!prPacket)
		return FALSE;

	prWlanHeader = (struct WLAN_MAC_HEADER *)
		((unsigned long) prPacket + MAC_TX_RESERVED_FIELD);
	u2TxFrameCtrl = prWlanHeader->u2FrameCtrl & MASK_FRAME_TYPE;
	if (u2TxFrameCtrl != MAC_FRAME_ACTION)
		return FALSE;

	prActFrame = (struct WLAN_ACTION_FRAME *)prWlanHeader;
	ucCategory = prActFrame->ucCategory;
	return ucCategory == CATEGORY_SPEC_MGT ||
	       ucCategory == CATEGORY_QOS_ACTION ||
	       ucCategory == CATEGORY_DLS_ACTION ||
	       ucCategory == CATEGORY_BLOCK_ACK_ACTION ||
	       ucCategory == CATEGORY_RM_ACTION ||
	       ucCategory == CATEGORY_FT_ACTION ||
	       ucCategory == CATEGORY_SA_QUERY_ACTION ||
	       ucCategory == CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION ||
	       ucCategory == CATEGORY_WNM_ACTION ||
	       ucCategory == CATEGORY_TDLS_ACTION ||
	       ucCategory == CATEGORY_MESH_ACTION ||
	       ucCategory == CATEGORY_MULTIHOP_ACTION ||
	       ucCategory == CATEGORY_DMG_ACTION ||
	       ucCategory == CATEGORY_FST_ACTION ||
	       ucCategory == CATEGORY_ROBUST_AV_STREAMING_ACTION ||
	       ucCategory == CATEGORY_VENDOR_SPECIFIC_PROTECTED_ACTION
	       ? TRUE : FALSE;
}

u_int8_t secIsWepBss(IN struct ADAPTER *prAdapter,
		     IN struct BSS_INFO *prBssInfo)
{
	enum ENUM_WEP_STATUS eEncStatus;

	ASSERT(prBssInfo);

	eEncStatus = aisGetEncStatus(prAdapter,
		prBssInfo->ucBssIndex);

	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
		if (eEncStatus ==
		    ENUM_ENCRYPTION1_ENABLED)
			return TRUE;
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
		return kalP2PGetWepCipher(prAdapter->prGlueInfo,
					  (uint8_t) prBssInfo->u4PrivateData);
#endif

	return FALSE;
}


bool secIsUsedByStaRecEntry(IN struct ADAPTER *prAdapter, IN uint8_t Index)
{
	struct STA_RECORD *prStaRec = NULL;
	uint8_t i = 0;
	bool fgIsUsed = FALSE;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];
		if (prStaRec->fgIsInUse && prStaRec->ucWlanIndex == Index) {
			fgIsUsed = TRUE;
			DBGLOG(RSN, INFO,
				"[Wlan index]: Duplicated entry #%d\n", Index);
			break;
		}
	}
	return fgIsUsed;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used before add/update a WLAN entry.
 *        Info the WLAN Table has available entry for this request
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in]  prSta the P_STA_RECORD_T for store
 *
 * \return TRUE Free Wlan table is reserved for this request
 *            FALSE No free entry for this request
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
u_int8_t secPrivacySeekForEntry(
				IN struct ADAPTER *prAdapter,
				IN struct STA_RECORD *prSta)
{
	struct BSS_INFO *prP2pBssInfo;
	uint8_t ucEntry = WTBL_RESERVED_ENTRY;
	uint8_t i;
	uint8_t ucStartIDX = 0, ucMaxIDX = 0;
	struct WLAN_TABLE *prWtbl;
	uint8_t ucRoleIdx = 0;

	if (!prSta->fgIsInUse) {
		DBGLOG(RSN, ERROR, "sta is not in use\n");
		return FALSE;
	}

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
	ucRoleIdx = prP2pBssInfo->u4PrivateData;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	ucStartIDX = 0;
	ucMaxIDX = prAdapter->ucTxDefaultWlanIndex - 1;

	for (i = ucStartIDX; i <= ucMaxIDX; i++) {
#if CFG_WIFI_WORKAROUND_HWITS00012836_WTBL_SEARCH_FAIL
	if (i % 8 == 0)
		continue;
#endif
		if (prWtbl[i].ucUsed
		    && EQUAL_MAC_ADDR(prSta->aucMacAddr, prWtbl[i].aucMacAddr)
		    && prWtbl[i].ucPairwise
		    /* This function for ucPairwise only */) {
			ucEntry = i;
			DBGLOG(RSN, TRACE,
			       "[Wlan index]: Reuse entry #%d\n", i);
			break;
		}
	}

	if (i == (ucMaxIDX + 1)) {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
#if CFG_WIFI_WORKAROUND_HWITS00012836_WTBL_SEARCH_FAIL
			if (i % 8 == 0)
				continue;
#endif
			if (prWtbl[i].ucUsed == FALSE) {
				if (!secIsUsedByStaRecEntry(prAdapter, i)) {
					ucEntry = i;
					DBGLOG(RSN, INFO,
						"[Wlan index]: Assign entry #%d\n", i);
					break;
				}
			}
		}
	}

	/* Save to the driver maintain table */
	if (ucEntry < prAdapter->ucTxDefaultWlanIndex) {

		prWtbl[ucEntry].ucUsed = TRUE;
		prWtbl[ucEntry].ucBssIndex = prSta->ucBssIndex;
		prWtbl[ucEntry].ucKeyId = 0xFF;
		prWtbl[ucEntry].ucPairwise = 1;
		COPY_MAC_ADDR(prWtbl[ucEntry].aucMacAddr, prSta->aucMacAddr);
		prWtbl[ucEntry].ucStaIndex = prSta->ucIndex;

		prSta->ucWlanIndex = ucEntry;

		{
			struct BSS_INFO *prBssInfo =
			    GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
			/* for AP mode , if wep key exist, peer sta should also
			 * fgTransmitKeyExist
			 */
			if (IS_BSS_P2P(prBssInfo)
			    && kalP2PGetRole(prAdapter->prGlueInfo,
					     ucRoleIdx) == 2) {
				if (prBssInfo->fgBcDefaultKeyExist
				    &&
				    !(kalP2PGetCcmpCipher
				      (prAdapter->prGlueInfo, ucRoleIdx)
				      ||
				      kalP2PGetTkipCipher(prAdapter->prGlueInfo,
							  ucRoleIdx))) {
					prSta->fgTransmitKeyExist = TRUE;
					prWtbl[ucEntry].ucKeyId =
					    prBssInfo->ucBcDefaultKeyIdx;
					DBGLOG(RSN, INFO,
					       "peer sta set fgTransmitKeyExist\n");
				}
			}
		}

		DBGLOG(RSN, INFO,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d "
		       MACSTR
		       " staType=%x\n", prSta->ucBssIndex, 0,
		       prWtbl[ucEntry].ucPairwise, ucEntry,
		       prSta->ucIndex, MAC2STR(prSta->aucMacAddr),
		       prSta->eStaType);
#if 1				/* DBG */
		secCheckWTBLAssign(prAdapter);
#endif
		return TRUE;
	}
#if DBG
	secCheckWTBLAssign(prAdapter);
#endif
	DBGLOG(RSN, WARN,
	       "[Wlan index] No more wlan table entry available!!!!\n");
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used free a WLAN entry.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in]  ucEntry the wlan table index to free
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void secPrivacyFreeForEntry(IN struct ADAPTER *prAdapter, IN uint8_t ucEntry)
{
	struct WLAN_TABLE *prWtbl;

	ASSERT(prAdapter);

	if (ucEntry >= WTBL_SIZE)
		return;

	DBGLOG(RSN, TRACE, "secPrivacyFreeForEntry %d", ucEntry);

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucEntry].ucUsed) {
		prWtbl[ucEntry].ucUsed = FALSE;
		prWtbl[ucEntry].ucKeyId = 0xff;
		prWtbl[ucEntry].ucBssIndex = prAdapter->ucHwBssIdNum + 1;
		prWtbl[ucEntry].ucPairwise = 0;
		kalMemZero(prWtbl[ucEntry].aucMacAddr, MAC_ADDR_LEN);
		prWtbl[ucEntry].ucStaIndex = STA_REC_INDEX_NOT_FOUND;
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used free a STA WLAN entry.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in]  prStaRec the sta which want to free
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void secPrivacyFreeSta(IN struct ADAPTER *prAdapter,
		       IN struct STA_RECORD *prStaRec)
{
	uint32_t entry;
	struct WLAN_TABLE *prWtbl;

	if (!prStaRec)
		return;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	for (entry = 0; entry < WTBL_SIZE; entry++) {
		/* Consider GTK case !! */
		if (prWtbl[entry].ucUsed &&
		    EQUAL_MAC_ADDR(prStaRec->aucMacAddr,
				   prWtbl[entry].aucMacAddr)
		    && prWtbl[entry].ucPairwise) {
#if 1				/* DBG */
			DBGLOG(RSN, INFO, "Free STA entry (%d)!\n", entry);
#endif
			secPrivacyFreeForEntry(prAdapter, entry);
			prStaRec->ucWlanIndex = WTBL_RESERVED_ENTRY;
			/* prStaRec->ucBMCWlanIndex = WTBL_RESERVED_ENTRY; */
		}
	}
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear the group WEP key
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 * \param[in] u4KeyId The key index
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
static inline uint32_t secRemoveBmcWepKey(IN struct ADAPTER *prAdapter,
					  IN uint8_t ucBssIndex,
					  IN uint32_t u4KeyId)
{
	struct PARAM_REMOVE_KEY rRemoveKey;
	uint32_t u4SetLen = 0;
	uint32_t u4Ret;

	DBGLOG(RSN, INFO, "BssIdx=%d, KeyId=%d\n", ucBssIndex, u4KeyId);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set remove WEP! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (u4KeyId > MAX_KEY_NUM - 1) {
		DBGLOG(REQ, ERROR, "invalid WEP key ID %u\n", u4KeyId);
		return WLAN_STATUS_INVALID_DATA;
	}

	kalMemZero(&rRemoveKey, sizeof(struct PARAM_REMOVE_KEY));
	rRemoveKey.u4Length = sizeof(struct PARAM_REMOVE_KEY);
	rRemoveKey.u4KeyIndex = u4KeyId;
	rRemoveKey.ucBssIdx = ucBssIndex;
	/* Should set FLAG_RM_KEY_CTRL_WO_OID for not OID operation */
	rRemoveKey.ucCtrlFlag = FLAG_RM_KEY_CTRL_WO_OID;

	u4Ret = wlanoidSetRemoveKey(prAdapter, (void *)&rRemoveKey,
			    sizeof(struct PARAM_REMOVE_KEY), &u4SetLen);

	if (u4Ret != WLAN_STATUS_PENDING)
		DBGLOG(RSN, WARN, "Can't send remove bmc wep key cmd\n");

	return u4Ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used for remove the BC entry of the BSS
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secRemoveBssBcEntry(IN struct ADAPTER *prAdapter,
			 IN struct BSS_INFO *prBssInfo, IN u_int8_t fgRoam)
{
	int i;

	if (!prBssInfo)
		return;

	DBGLOG(RSN, INFO, "remove all the key related with BSS!");

	if (fgRoam) {
		struct CONNECTION_SETTINGS *prConnSettings =
			aisGetConnSettings(prAdapter,
			prBssInfo->ucBssIndex);

		if (IS_BSS_AIS(prBssInfo) &&
		    prBssInfo->prStaRecOfAP
		    && (prConnSettings->eAuthMode >= AUTH_MODE_WPA &&
			prConnSettings->eAuthMode != AUTH_MODE_WPA_NONE)) {

			for (i = 0; i < MAX_KEY_NUM; i++) {
				if (prBssInfo->ucBMCWlanIndexSUsed[i])
					secPrivacyFreeForEntry(prAdapter,
						prBssInfo->ucBMCWlanIndexS[i]);

			/* move to wlanoidSetRemoveKey( )
			 *
			 * prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
			 * prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
			 */
			}

			prBssInfo->fgBcDefaultKeyExist = FALSE;
			prBssInfo->ucBcDefaultKeyIdx = 0xff;
		}
	} else {
		/* According to discussion, it's ok to change to
		 * reserved_entry here so that the entry is _NOT_ freed at all.
		 * In this way, the same BSS(ucBssIndex) could reuse the same
		 * entry next time in secPrivacySeekForBcEntry(), and we could
		 * see the following log: "[Wlan index]: Reuse entry ...".
		 */
		prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
		secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndex);

		for (i = 0; i < MAX_KEY_NUM; i++) {
			/*
			 * As comment in FW secCmdProcAddRemoveKey( ),
			 * Remove Key, just remove the key info, not entry
			 *
			 * if (prBssInfo->ucBMCWlanIndexSUsed[i])
			 *	secPrivacyFreeForEntry(prAdapter,
			 *	prBssInfo->ucBMCWlanIndexS[i]);
			 */

			/* move to wlanoidSetRemoveKey( )
			 *
			 * prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
			 * prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
			 */
		}
		for (i = 0; i < MAX_KEY_NUM; i++) {
			if (prBssInfo->wepkeyUsed[i] == FALSE)
				continue;
			/* remove key to avoid that cfg80211_del_key is called
			 * after nicDeactivateNetwork.
			 */
			secRemoveBmcWepKey(prAdapter, prBssInfo->ucBssIndex, i);
			prBssInfo->wepkeyUsed[i] = FALSE;
		}
		/* wlanoidSetRemoveKey would clear prBssInfo->wepkeyUsed[],
		 * but won't call secPrivacyFreeForEntry.
		 * For the case that the cfg80211_del_key is called before
		 * nicDeactivateNetwork, check wepkeyWlanIdx to do
		 * secPrivacyFreeForEntry.
		 */
		if (prBssInfo->wepkeyWlanIdx != WTBL_RESERVED_ENTRY)
			secPrivacyFreeForEntry(prAdapter,
					       prBssInfo->wepkeyWlanIdx);
		prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
		prBssInfo->fgBcDefaultKeyExist = FALSE;
		prBssInfo->ucBcDefaultKeyIdx = 0xff;
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used for adding the broadcast key used, to assign
 *         a wlan table entry for reserved the specific entry for these key for
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 * \param[in] ucNetTypeIdx The Network index
 * \param[in] ucAlg the entry assign related with algorithm
 * \param[in] ucKeyId The key id
 * \param[in] ucTxRx The Type of the key
 *
 * \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation
 *                      fail
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
uint8_t
secPrivacySeekForBcEntry(IN struct ADAPTER *prAdapter,
			 IN uint8_t ucBssIndex,
			 IN uint8_t *pucAddr, IN uint8_t ucStaIdx,
			 IN uint8_t ucAlg, IN uint8_t ucKeyId)
{
	uint8_t ucEntry = WTBL_ALLOC_FAIL;
	uint8_t ucStartIDX = 0, ucMaxIDX = 0;
	uint8_t i;
	u_int8_t fgCheckKeyId = TRUE;
	struct WLAN_TABLE *prWtbl;
	struct BSS_INFO *prBSSInfo =
	    GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	prWtbl = prAdapter->rWifiVar.arWtbl;
	ASSERT(prAdapter);
	ASSERT(pucAddr);

	if (ucAlg == CIPHER_SUITE_WPI ||	/* CIPHER_SUITE_GCM_WPI || */
	    ucAlg == CIPHER_SUITE_WEP40 ||
	    ucAlg == CIPHER_SUITE_WEP104
	    || ucAlg == CIPHER_SUITE_WEP128 || ucAlg == CIPHER_SUITE_NONE)
		fgCheckKeyId = FALSE;

	if (ucKeyId == 0xFF || ucAlg == CIPHER_SUITE_BIP)
		fgCheckKeyId = FALSE;

	if (prBSSInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		fgCheckKeyId = FALSE;

	if (prBSSInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		  prBSSInfo->eNetworkType == NETWORK_TYPE_AIS) {
		fgCheckKeyId = FALSE;
		DBGLOG(RSN, WARN, "Always install gtk in same wtbl\n");
	}

	ucStartIDX = 0;
	ucMaxIDX = prAdapter->ucTxDefaultWlanIndex - 1;

	DBGLOG(INIT, INFO, "OpMode:%d, NetworkType:%d, CheckKeyId:%d\n",
	       prBSSInfo->eCurrentOPMode, prBSSInfo->eNetworkType,
	       fgCheckKeyId);

	for (i = ucStartIDX; i <= ucMaxIDX; i++) {

		if (prWtbl[i].ucUsed && !prWtbl[i].ucPairwise
		    && prWtbl[i].ucBssIndex == ucBssIndex) {

			if (!fgCheckKeyId) {
				ucEntry = i;
				DBGLOG(RSN, TRACE,
				       "[Wlan index]: Reuse entry #%d for open/wep/wpi\n",
				       i);
				break;
			}

			if (fgCheckKeyId && (prWtbl[i].ucKeyId == ucKeyId
					     || prWtbl[i].ucKeyId == 0xFF)) {
				ucEntry = i;
				DBGLOG(RSN, TRACE,
				       "[Wlan index]: Reuse entry #%d\n", i);
				break;
			}
		}
	}

	if (i == (ucMaxIDX + 1)) {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
			if (prWtbl[i].ucUsed == FALSE) {
				ucEntry = i;
				DBGLOG(RSN, TRACE,
				       "[Wlan index]: Assign entry #%d\n", i);
				break;
			}
		}
	}

	if (ucEntry < prAdapter->ucTxDefaultWlanIndex) {
		if (ucAlg != CIPHER_SUITE_BIP) {
			prWtbl[ucEntry].ucUsed = TRUE;
			prWtbl[ucEntry].ucKeyId = ucKeyId;
			prWtbl[ucEntry].ucBssIndex = ucBssIndex;
			prWtbl[ucEntry].ucPairwise = 0;
			kalMemCopy(prWtbl[ucEntry].aucMacAddr, pucAddr,
				   MAC_ADDR_LEN);
			prWtbl[ucEntry].ucStaIndex = ucStaIdx;
		} else {
			/* BIP no need to dump secCheckWTBLAssign */
			return ucEntry;
		}

		DBGLOG(RSN, INFO,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d "
		       MACSTR
		       "\n", ucBssIndex, ucKeyId, prWtbl[ucEntry].ucPairwise,
		       ucEntry, ucStaIdx, MAC2STR(pucAddr));

		/* DBG */
		secCheckWTBLAssign(prAdapter);

	} else {
		secCheckWTBLAssign(prAdapter);
		DBGLOG(RSN, ERROR,
			"[Wlan index] No more wlan entry available!!!!\n");
	}

	return ucEntry;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation
 *                      fail
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
u_int8_t secCheckWTBLAssign(IN struct ADAPTER *prAdapter)
{
	secPrivacyDumpWTBL(prAdapter);
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Got the STA record index by wlan index
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucWlanIdx The Rx wlan index
 *
 * \return The STA record index, 0xff for invalid sta index
 */
/*----------------------------------------------------------------------------*/
uint8_t secGetStaIdxByWlanIdx(struct ADAPTER *prAdapter, uint8_t ucWlanIdx)
{
	struct WLAN_TABLE *prWtbl;

	ASSERT(prAdapter);

	if (ucWlanIdx >= WTBL_SIZE)
		return STA_REC_INDEX_NOT_FOUND;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	/* DBGLOG(RSN, TRACE, ("secGetStaIdxByWlanIdx=%d "MACSTR" used=%d\n",
	 *   ucWlanIdx, MAC2STR(prWtbl[ucWlanIdx].aucMacAddr),
	 *   prWtbl[ucWlanIdx].ucUsed));
	 */

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucStaIndex;
	else
		return STA_REC_INDEX_NOT_FOUND;
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
/*----------------------------------------------------------------------------*/
/*!
 * \brief  Got the wlan index by STA record index
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucStaIndex The sta record index
 *
 * \return The wlan index, WTBL_SIZE for invalid sta index
 */
/*----------------------------------------------------------------------------*/
uint8_t secGetWlanIdxByStaIdx(struct ADAPTER *prAdapter, uint8_t ucStaIndex)
{
	struct STA_RECORD *prStaRec =
		cnmGetStaRecByIndex(prAdapter, ucStaIndex);

	if (prStaRec != NULL)
		return prStaRec->ucWlanIndex;

	return WTBL_SIZE;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief  At Sw wlan table, got the BSS index by wlan index
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucWlanIdx The Rx wlan index
 *
 * \return The BSS index, 0xff for invalid bss index
 */
/*----------------------------------------------------------------------------*/
uint8_t secGetBssIdxByWlanIdx(struct ADAPTER *prAdapter, uint8_t ucWlanIdx)
{
	struct WLAN_TABLE *prWtbl;

	ASSERT(prAdapter);

	if (ucWlanIdx >= WTBL_SIZE)
		return WTBL_RESERVED_ENTRY;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucBssIndex;
	else
		return WTBL_RESERVED_ENTRY;
}

uint8_t secGetBssIdxByRfb(IN struct ADAPTER *prAdapter,
	IN struct SW_RFB *prSwRfb) {

	if (prAdapter && prSwRfb) {
		uint8_t	ucBssIndex =
			secGetBssIdxByWlanIdx(prAdapter,
			prSwRfb->ucWlanIdx);
		struct STA_RECORD *prStaRec =
			cnmGetStaRecByIndex(prAdapter,
			prSwRfb->ucStaRecIdx);


		if (ucBssIndex != WTBL_RESERVED_ENTRY)
			return ucBssIndex;


		if (prStaRec) {
			DBGLOG(RSN, LOUD,
				"prStaRec->ucBssIndex = %d\n",
				prStaRec->ucBssIndex);
			return prStaRec->ucBssIndex;
		}
	}

	DBGLOG(RSN, LOUD, "Return default index\n");

	return AIS_DEFAULT_INDEX;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Got the STA record index by mac addr
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] pucMacAddress MAC Addr
 *
 * \return The STA record index, 0xff for invalid sta index
 */
/*----------------------------------------------------------------------------*/
uint8_t secLookupStaRecIndexFromTA(
			struct ADAPTER *prAdapter, uint8_t *pucMacAddress)
{
	int i;
	struct WLAN_TABLE *prWtbl;

	ASSERT(prAdapter);
	prWtbl = prAdapter->rWifiVar.arWtbl;

	for (i = 0; i < WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed) {
			if (EQUAL_MAC_ADDR(pucMacAddress, prWtbl[i].aucMacAddr)
			    && prWtbl[i].ucPairwise)
				return prWtbl[i].ucStaIndex;
		}
	}

	return STA_REC_INDEX_NOT_FOUND;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secPrivacyDumpWTBL(IN struct ADAPTER *prAdapter)
{
	struct WLAN_TABLE *prWtbl;
	uint8_t i;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	DBGLOG(RSN, TRACE, "The Wlan index\n");

	for (i = 0; i < WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed)
			DBGLOG(RSN, TRACE,
			       "#%d Used=%d  BSSIdx=%d keyid=%d P=%d STA=%d Addr="
			       MACSTR "\n", i, prWtbl[i].ucUsed,
			       prWtbl[i].ucBssIndex, prWtbl[i].ucKeyId,
			       prWtbl[i].ucPairwise, prWtbl[i].ucStaIndex,
			       MAC2STR(prWtbl[i].aucMacAddr));
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Assin the wlan table with the join AP info
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secPostUpdateAddr(IN struct ADAPTER *prAdapter,
		       IN struct BSS_INFO *prBssInfo)
{
	struct WLAN_TABLE *prWtbl;

	if (IS_BSS_AIS(prBssInfo) && prBssInfo->prStaRecOfAP) {
		struct CONNECTION_SETTINGS *prConnSettings =
			aisGetConnSettings(prAdapter,
			prBssInfo->ucBssIndex);

		if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED) {

			if (prBssInfo->fgBcDefaultKeyExist) {

				prWtbl =
				    &prAdapter->rWifiVar.
				    arWtbl[prBssInfo->wepkeyWlanIdx];

				kalMemCopy(prWtbl->aucMacAddr,
					   prBssInfo->prStaRecOfAP->aucMacAddr,
					   MAC_ADDR_LEN);
				prWtbl->ucStaIndex =
				    prBssInfo->prStaRecOfAP->ucIndex;
				DBGLOG(RSN, INFO,
				       "secPostUpdateAddr at [%d] " MACSTR
				       "= STA Index=%d\n",
				       prBssInfo->wepkeyWlanIdx,
				       MAC2STR(prWtbl->aucMacAddr),
				       prBssInfo->prStaRecOfAP->ucIndex);

				/* Update the wlan table of the prStaRecOfAP */
				prWtbl =
				    &prAdapter->rWifiVar.arWtbl
				    [prBssInfo->prStaRecOfAP->ucWlanIndex];
				prWtbl->ucKeyId = prBssInfo->ucBcDefaultKeyIdx;
				prBssInfo->prStaRecOfAP->fgTransmitKeyExist =
				    TRUE;
			}
		}
		if (prConnSettings->eEncStatus == ENUM_ENCRYPTION_DISABLED) {
			prWtbl =
			    &prAdapter->rWifiVar.
			    arWtbl[prBssInfo->ucBMCWlanIndex];

			kalMemCopy(prWtbl->aucMacAddr,
				   prBssInfo->prStaRecOfAP->aucMacAddr,
				   MAC_ADDR_LEN);
			prWtbl->ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;
			DBGLOG(RSN, INFO, "secPostUpdateAddr at [%d] " MACSTR
			       "= STA Index=%d\n",
			       prBssInfo->ucBMCWlanIndex,
			       MAC2STR(prWtbl->aucMacAddr),
			       prBssInfo->prStaRecOfAP->ucIndex);
		}
	}
}

/* return the type of Eapol frame. */
enum ENUM_EAPOL_KEY_TYPE_T secGetEapolKeyType(uint8_t *pucPkt)
{
	uint8_t *pucEthBody = NULL;
	uint8_t ucEapolType;
	uint16_t u2EtherTypeLen;
	uint8_t ucEthTypeLenOffset = ETHER_HEADER_LEN - ETHER_TYPE_LEN;
	uint16_t u2KeyInfo = 0;

	do {
		ASSERT_BREAK(pucPkt != NULL);
		WLAN_GET_FIELD_BE16(&pucPkt[ucEthTypeLenOffset],
				    &u2EtherTypeLen);
		if (u2EtherTypeLen == ETH_P_VLAN) {
			ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
			WLAN_GET_FIELD_BE16(&pucPkt[ucEthTypeLenOffset],
					    &u2EtherTypeLen);
		}
		if (u2EtherTypeLen != ETH_P_1X)
			break;
		pucEthBody = &pucPkt[ucEthTypeLenOffset + ETHER_TYPE_LEN];
		ucEapolType = pucEthBody[1];
		if (ucEapolType != 3)	/* eapol key type */
			break;
		u2KeyInfo = *((uint16_t *) (&pucEthBody[5]));
		switch (u2KeyInfo) {
		case 0x8a00:
			return EAPOL_KEY_1_OF_4;
		case 0x0a01:
			return EAPOL_KEY_2_OF_4;
		case 0xca13:
			return EAPOL_KEY_3_OF_4;
		case 0x0a03:
			return EAPOL_KEY_4_OF_4;
		}
	} while (FALSE);

	return EAPOL_KEY_NOT_KEY;
}

void secHandleNoWtbl(IN struct ADAPTER *prAdapter,
	IN struct SW_RFB *prSwRfb)
{
	/* Wtbl error handling. if no Wtbl */
	struct WLAN_ACTION_FRAME *prMgmtHdr =
		(struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;

	prSwRfb->ucStaRecIdx =
		secLookupStaRecIndexFromTA(prAdapter, prMgmtHdr->aucSrcAddr);

	prSwRfb->prStaRec =
		cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);

	if (prSwRfb->prStaRec) {
		prSwRfb->ucWlanIdx = prSwRfb->prStaRec->ucWlanIndex;
		DBGLOG(RX, INFO,
			"[%d] current wlan index is %d\n",
			prSwRfb->ucStaRecIdx,
			prSwRfb->ucWlanIdx);
	} else
		DBGLOG(RX, TRACE,
			"not find station record base on TA\n");
}
