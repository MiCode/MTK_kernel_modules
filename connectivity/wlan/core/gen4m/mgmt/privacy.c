// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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
void secInit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	uint8_t i;
	struct BSS_INFO *prBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
	struct IEEE_802_11_MIB *prMib;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RSN, ERROR, "prBssInfo is null\n");
		return;
	}

	prAisSpecBssInfo = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prMib = aisGetMib(prAdapter, ucBssIndex);

	prBssInfo->u4RsnSelectedGroupCipher = 0;
	prBssInfo->u4RsnSelectedPairwiseCipher = 0;
	prBssInfo->u4RsnSelectedAKMSuite = 0;

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
	prMib->
	    dot11RSNAConfigPairwiseCiphersTable[10]
						.dot11RSNAConfigPairwiseCipher
	    = RSN_CIPHER_SUITE_GCMP;

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
	    [8].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_OSEN;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [9].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_SAE;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [10].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_OWE;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [11].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_DPP;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [12].dot11RSNAConfigAuthenticationSuite =
		RSN_AKM_SUITE_8021X_SUITE_B_192;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [13].dot11RSNAConfigAuthenticationSuite =
		RSN_AKM_SUITE_SAE_EXT_KEY;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [14].dot11RSNAConfigAuthenticationSuite =
		RSN_AKM_SUITE_802_1X_SHA256;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [15].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_PSK_SHA256;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [16].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_FT_OVER_SAE;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [17].dot11RSNAConfigAuthenticationSuite =
		RSN_AKM_SUITE_FT_SAE_EXT_KEY;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [18].dot11RSNAConfigAuthenticationSuite =
		RSN_AKM_SUITE_FILS_SHA256;
	prMib->dot11RSNAConfigAuthenticationSuitesTable
	    [19].dot11RSNAConfigAuthenticationSuite =
		RSN_AKM_SUITE_FILS_SHA384;

	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
		prMib->dot11RSNAConfigAuthenticationSuitesTable
		    [i].dot11RSNAConfigAuthenticationSuiteEnabled = FALSE;
	}

#if CFG_SUPPORT_802_11W
	cnmTimerInitTimer(prAdapter,
			  &prAisSpecBssInfo->rSaQueryTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) rsnStartSaQueryTimer,
			  (uintptr_t)ucBssIndex);
#endif

	prAisSpecBssInfo->fgCounterMeasure = FALSE;
	prBssInfo->ucBcDefaultKeyIdx = 0xff;
	prBssInfo->fgBcDefaultKeyExist = FALSE;
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
u_int8_t secCheckClassError(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb,
			    struct STA_RECORD *prStaRec)
{
	void *prRxStatus;
	struct RX_DESC_OPS_T *prRxDescOps;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex;
	uint8_t ucClassErr, ucAisDisconnect;

	if (!prStaRec)
		return FALSE;

	ucBssIndex = prStaRec->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(RX, ERROR, "Invalid bssidx:%d\n", ucBssIndex);
		return FALSE;
	}

	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	prRxStatus = prSwRfb->prRxStatus;
	ucClassErr = prRxDescOps->nic_rxd_get_sw_class_error_bit(prRxStatus);
	ucAisDisconnect = !!(IS_BSS_INFO_IN_AIS(prBssInfo)
			&& kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex) == MEDIA_STATE_DISCONNECTED);

	if (ucClassErr || ucAisDisconnect) {
		if (EAPOL_KEY_NOT_KEY !=
			secGetEapolKeyType((uint8_t *) prSwRfb->pvHeader)) {
			DBGLOG(RSN, WARN,
			       "EAPOL key found, return TRUE back");
			return TRUE;
		}

		DBGLOG_LIMITED(RSN, WARN,
			"RX_CLASSERR: prStaRec=%p PktTYpe=0x%x, WlanIdx=%d,",
			prStaRec,
			prSwRfb->ucPacketType, prSwRfb->ucWlanIdx);
		DBGLOG_LIMITED(RSN, WARN,
			"StaRecIdx=%d, eDst=%d, prStaRec->eStaType=%d\n",
			prSwRfb->ucStaRecIdx,
			prSwRfb->eDst, prStaRec->eStaType);

		if (!ucAisDisconnect) {
			DBGLOG_MEM8(RX, ERROR, prSwRfb->pucRecvBuff,
				    (prSwRfb->u2RxByteCount > 64) ? 64 :
				    prSwRfb->u2RxByteCount);
		}

		if (prSwRfb->ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
			DBGLOG_LIMITED(RX, ERROR,
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
void secSetPortBlocked(struct ADAPTER *prAdapter,
		       struct STA_RECORD *prSta, u_int8_t fgPortBlock)
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
u_int8_t secGetPortStatus(struct ADAPTER *prAdapter,
			  struct STA_RECORD *prSta,
			  u_int8_t *pfgPortStatus)
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
secTxPortControlCheck(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo,
		      struct STA_RECORD *prStaRec)
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
		if (IS_STA_IN_AIS(prAdapter, prStaRec)) {
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
u_int8_t secRxPortControlCheck(struct ADAPTER *prAdapter,
			       struct SW_RFB *prSWRfb)
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
void secSetCipherSuite(struct ADAPTER *prAdapter,
		       uint32_t u4CipherSuitesFlags,
		       uint8_t ucBssIndex)
{

	uint32_t i;
	struct DOT11_RSNA_CONFIG_PAIRWISE_CIPHERS_ENTRY *prEntry;
	struct IEEE_802_11_MIB *prMib;

	prMib = aisGetMib(prAdapter, ucBssIndex);

	if (u4CipherSuitesFlags == CIPHER_FLAG_NONE) {
		/* Disable all the pairwise cipher suites. */
		prMib->dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_NONE;
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
			if (u4CipherSuitesFlags & CIPHER_FLAG_GCMP256) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
					TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;
		case RSN_CIPHER_SUITE_GCMP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_GCMP128) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
					TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;
		case WPA_CIPHER_SUITE_WEP40:
		case RSN_CIPHER_SUITE_WEP40:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP40) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;

		case WPA_CIPHER_SUITE_TKIP:
		case RSN_CIPHER_SUITE_TKIP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_TKIP) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;

		case WPA_CIPHER_SUITE_CCMP:
		case RSN_CIPHER_SUITE_CCMP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_CCMP) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;

		case RSN_CIPHER_SUITE_GROUP_NOT_USED:
			if (u4CipherSuitesFlags &
			    (CIPHER_FLAG_CCMP | CIPHER_FLAG_TKIP)) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;

		case WPA_CIPHER_SUITE_WEP104:
		case RSN_CIPHER_SUITE_WEP104:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP104) {
				prEntry->dot11RSNAConfigPairwiseCipherEnabled =
				    TRUE;
				prMib->dot11RSNAConfigPairwiseCipher |=
					rsnCipherToBit(
					prEntry->dot11RSNAConfigPairwiseCipher);
			}
			break;
		default:
			break;
		}
	}
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
u_int8_t secEnabledInAis(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex)
{
	enum ENUM_WEP_STATUS eEncStatus;

	eEncStatus = aisGetEncStatus(prAdapter, ucBssIndex);

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
u_int8_t secIsProtectedFrame(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsdu,
			     struct STA_RECORD *prStaRec)
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

u_int8_t secIsProtectedBss(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo)
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

u_int8_t secIsRobustMgmtFrame(struct ADAPTER *prAdapter, void *prPacket)
{
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	uint16_t u2TxFrameCtrl;

	if (!prPacket)
		return FALSE;

	prWlanHeader = (struct WLAN_MAC_HEADER *)
		((uintptr_t) prPacket + MAC_TX_RESERVED_FIELD);
	u2TxFrameCtrl = prWlanHeader->u2FrameCtrl & MASK_FRAME_TYPE;
	if (u2TxFrameCtrl == MAC_FRAME_DISASSOC
	    || u2TxFrameCtrl == MAC_FRAME_DEAUTH)
		return TRUE;
	return FALSE;
}

u_int8_t secIsRobustActionFrame(struct ADAPTER *prAdapter, void *prPacket)
{
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	struct WLAN_ACTION_FRAME *prActFrame = NULL;
	uint16_t u2TxFrameCtrl;

	if (!prPacket)
		return FALSE;

	prWlanHeader = (struct WLAN_MAC_HEADER *)
		((uintptr_t) prPacket + MAC_TX_RESERVED_FIELD);
	u2TxFrameCtrl = prWlanHeader->u2FrameCtrl & MASK_FRAME_TYPE;
	if (u2TxFrameCtrl != MAC_FRAME_ACTION)
		return FALSE;

	prActFrame = (struct WLAN_ACTION_FRAME *)prWlanHeader;
	switch (prActFrame->ucCategory) {
	case CATEGORY_SPEC_MGT:
	case CATEGORY_QOS_ACTION:
	case CATEGORY_DLS_ACTION:
	case CATEGORY_BLOCK_ACK_ACTION:
	case CATEGORY_RM_ACTION:
	case CATEGORY_FT_ACTION:
	case CATEGORY_SA_QUERY_ACTION:
	case CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION:
	case CATEGORY_WNM_ACTION:
	case CATEGORY_TDLS_ACTION:
	case CATEGORY_MESH_ACTION:
	case CATEGORY_MULTIHOP_ACTION:
	case CATEGORY_FILS_ACTION:
	case CATEGORY_DMG_ACTION:
	case CATEGORY_FST_ACTION:
	case CATEGORY_ROBUST_AV_STREAMING_ACTION:
	case CATEGORY_VENDOR_SPECIFIC_PROTECTED_ACTION:
	case CATEGORY_PROTECTED_EHT_ACTION:
		DBGLOG(RSN, TRACE, "category %d\n", prActFrame->ucCategory);
		return TRUE;
	default:
		return FALSE;
	}
}

u_int8_t secIsWepBss(struct ADAPTER *prAdapter,
		     struct BSS_INFO *prBssInfo)
{
	enum ENUM_WEP_STATUS eEncStatus;

	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
		eEncStatus = aisGetEncStatus(prAdapter,
			prBssInfo->ucBssIndex);
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
				struct ADAPTER *prAdapter,
				struct STA_RECORD *prSta)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
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
	if (!prP2pBssInfo) {
		DBGLOG(RSN, ERROR, "prP2pBssInfo is null\n");
		return FALSE;
	}
	ucRoleIdx = prP2pBssInfo->u4PrivateData;

	prWtbl = prAdapter->rWifiVar.arWtbl;

#if (CFG_WIFI_IGTK_GTK_SEPARATE == 1)
	/* reserve wtbl IDX 0~3 for BIP*/
	ucStartIDX = 4;
#else
	ucStartIDX = 0;
#endif
	if (prChipInfo->is_support_dmashdl_lite)
		ucStartIDX = 16;

	ucMaxIDX = prAdapter->ucTxDefaultWlanIndex - 1;

	for (i = ucStartIDX; i <= ucMaxIDX; i++) {
#if CFG_WIFI_SW_WTBL_SEARCH_FAIL
	if (i % 8 == 0)
		continue;
#endif
		if (prWtbl[i].ucUsed
		    && EQUAL_MAC_ADDR(prSta->aucMacAddr, prWtbl[i].aucMacAddr)
		    && prWtbl[i].ucPairwise
		    /* This function for ucPairwise only */
		    && prWtbl[i].ucBssIndex == prSta->ucBssIndex) {
			ucEntry = i;
			DBGLOG(RSN, TRACE,
			       "[Wlan index]: Reuse entry #%d\n", i);
			break;
		}
	}

	if (i == (ucMaxIDX + 1)) {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
#if CFG_WIFI_SW_WTBL_SEARCH_FAIL
			if (i % 8 == 0)
				continue;
#endif
			if (prWtbl[i].ucUsed == FALSE) {
				ucEntry = i;
				DBGLOG(RSN, TRACE,
				       "[Wlan index]: Assign entry #%d\n", i);
				break;
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

#if CFG_ENABLE_WIFI_DIRECT
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
#endif
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
void secPrivacyFreeForEntry(struct ADAPTER *prAdapter, uint8_t ucEntry)
{
	struct WLAN_TABLE *prWtbl;

	if (ucEntry >= WTBL_SIZE)
		return;

	DBGLOG(RSN, TRACE, "secPrivacyFreeForEntry %d", ucEntry);

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucEntry].ucUsed) {
		prWtbl[ucEntry].ucUsed = FALSE;
		prWtbl[ucEntry].ucKeyId = 0xff;
		prWtbl[ucEntry].ucBssIndex = prAdapter->ucSwBssIdNum + 1;
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
void secPrivacyFreeSta(struct ADAPTER *prAdapter,
		       struct STA_RECORD *prStaRec)
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
		    && prWtbl[entry].ucPairwise
		    && prWtbl[entry].ucBssIndex == prStaRec->ucBssIndex) {
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
 * \brief This routine is used for remove the BC entry of the BSS
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] ucBssIndex The BSS index
 *
 * \note
 */
/*----------------------------------------------------------------------------*/
void secRemoveBssBcEntry(struct ADAPTER *prAdapter,
			 struct BSS_INFO *prBssInfo, u_int8_t fgRoam)
{
	if (!prBssInfo)
		return;

	DBGLOG(RSN, TRACE, "remove all the key related with BSS!");

	if (fgRoam) {
		struct CONNECTION_SETTINGS *prConnSettings =
			aisGetConnSettings(prAdapter,
			prBssInfo->ucBssIndex);

		if (IS_BSS_AIS(prBssInfo) &&
		    prBssInfo->prStaRecOfAP
		    && (prConnSettings->eAuthMode >= AUTH_MODE_WPA &&
			prConnSettings->eAuthMode != AUTH_MODE_WPA_NONE)) {

			prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
			secPrivacyFreeForEntry(prAdapter,
						prBssInfo->ucBMCWlanIndex);
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
secPrivacySeekForBcEntry(struct ADAPTER *prAdapter,
			 uint8_t ucBssIndex,
			 uint8_t *pucAddr, uint8_t ucStaIdx,
			 uint8_t ucAlg, uint8_t ucKeyId)
{
	uint8_t ucEntry = WTBL_ALLOC_FAIL;
	uint8_t ucStartIDX = 0, ucMaxIDX = 0;
	uint8_t i;
	u_int8_t fgCheckKeyId = TRUE;
	struct WLAN_TABLE *prWtbl;
	struct BSS_INFO *prBSSInfo =
	    GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBSSInfo) {
		DBGLOG(RSN, ERROR, "prBssInfo is null\n");
		return ucEntry;
	}
	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (ucAlg == CIPHER_SUITE_WPI ||	/* CIPHER_SUITE_GCM_WPI || */
	    ucAlg == CIPHER_SUITE_WEP40 ||
	    ucAlg == CIPHER_SUITE_WEP104
	    || ucAlg == CIPHER_SUITE_WEP128 || ucAlg == CIPHER_SUITE_NONE)
		fgCheckKeyId = FALSE;

	if (ucKeyId == 0xFF ||
	    ucAlg == CIPHER_SUITE_BIP ||
	    ucAlg == CIPHER_SUITE_BIP_GMAC_256)
		fgCheckKeyId = FALSE;

	if (prBSSInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		fgCheckKeyId = FALSE;


#if CFG_SINGLE_WTBL_GTK_REKEY
	if (prBSSInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		prBSSInfo->eNetworkType == NETWORK_TYPE_AIS) {
		fgCheckKeyId = FALSE;
		DBGLOG(RSN, WARN, "Always install gtk in same wtbl\n");
	}
#endif

#if (CFG_WIFI_IGTK_GTK_SEPARATE == 1)
	/* reserve wtbl IDX 0~3 for BIP*/
	ucStartIDX = 4;
#else
	ucStartIDX = 0;
#endif
	ucMaxIDX = prAdapter->ucTxDefaultWlanIndex - 1;

#if (CFG_WIFI_IGTK_GTK_SEPARATE == 1)
	if (ucAlg == CIPHER_SUITE_BIP || ucAlg == CIPHER_SUITE_BIP_GMAC_256)
		ucEntry = ucBssIndex;
	else
#endif
	{
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

				if (fgCheckKeyId
					&& (prWtbl[i].ucKeyId == ucKeyId
						|| prWtbl[i].ucKeyId == 0xFF)) {
					ucEntry = i;
					DBGLOG(RSN, TRACE,
						"[Wlan index]: Reuse entry #%d\n",
						i);
					break;
				}
			}
		}

		if (i == (ucMaxIDX + 1)) {
			for (i = ucStartIDX; i <= ucMaxIDX; i++) {
				if (prWtbl[i].ucUsed == FALSE) {
					ucEntry = i;
					DBGLOG(RSN, TRACE,
					       "[Wlan index]: Assign entry #%d\n",
						i);
					break;
				}
			}
		}
	}

	if (ucEntry < prAdapter->ucTxDefaultWlanIndex) {
#if (CFG_WIFI_IGTK_GTK_SEPARATE == 0)
		if (ucAlg == CIPHER_SUITE_BIP ||
		    ucAlg == CIPHER_SUITE_BIP_GMAC_256) {
			/* BIP no need to dump secCheckWTBLAssign */
			return ucEntry;
		}
#endif
		prWtbl[ucEntry].ucUsed = TRUE;
		prWtbl[ucEntry].ucKeyId = ucKeyId;
		prWtbl[ucEntry].ucBssIndex = ucBssIndex;
		prWtbl[ucEntry].ucPairwise = 0;
		kalMemCopy(prWtbl[ucEntry].aucMacAddr, pucAddr,
				MAC_ADDR_LEN);
		prWtbl[ucEntry].ucStaIndex = ucStaIdx;

		DBGLOG(RSN, TRACE,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d "
		       MACSTR
		       " (OpMode:%d, NetworkType:%d, CheckKeyId:%d)\n",
		       ucBssIndex, ucKeyId, prWtbl[ucEntry].ucPairwise,
		       ucEntry, ucStaIdx, MAC2STR(pucAddr),
		       prBSSInfo->eCurrentOPMode, prBSSInfo->eNetworkType,
		       fgCheckKeyId);

		/* DBG */
		secCheckWTBLAssign(prAdapter);

	} else {
		secCheckWTBLAssign(prAdapter);
		DBGLOG(RSN, ERROR,
			"[Wlan index] No more wlan entry available!!!! (OpMode:%d, NetworkType:%d, CheckKeyId:%d)\n",
			prBSSInfo->eCurrentOPMode, prBSSInfo->eNetworkType,
			fgCheckKeyId);
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
u_int8_t secCheckWTBLAssign(struct ADAPTER *prAdapter)
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

	if (ucWlanIdx >= WTBL_SIZE)
		return WTBL_RESERVED_ENTRY;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucBssIndex;
	else
		return WTBL_RESERVED_ENTRY;
}

uint8_t secGetBssIdxByRfb(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{

	if (!prAdapter)
		return 0;

	if (prSwRfb) {
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

	return aisGetDefaultLinkBssIndex(prAdapter);
}

struct BSS_INFO *secGetBssByRfb(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb)
{
	if (prAdapter && prSwRfb) {
		uint8_t ucBssIndex = secGetBssIdxByWlanIdx(prAdapter,
			prSwRfb->ucWlanIdx);

		if (ucBssIndex != WTBL_RESERVED_ENTRY)
			return GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	}

	return NULL;
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
void secPrivacyDumpWTBL(struct ADAPTER *prAdapter)
{
	struct WLAN_TABLE *prWtbl;
	uint8_t i;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	DBGLOG(RSN, TRACE, "The Wlan index\n");

	for (i = 0; i < WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed)
			DBGLOG(RSN, INFO,
			       "#%d Used=%d  BSSIdx=%d keyid=%d P=%d STA=%d Addr="
			       MACSTR "\n", i, prWtbl[i].ucUsed,
			       prWtbl[i].ucBssIndex, prWtbl[i].ucKeyId,
			       prWtbl[i].ucPairwise, prWtbl[i].ucStaIndex,
			       MAC2STR(prWtbl[i].aucMacAddr));
	}
}

uint8_t secCheckWTBLwlanIdxInUseByOther(struct ADAPTER *prAdapter,
	uint8_t ucWlanIdx, uint8_t ucBssIndex)
{
	struct WLAN_TABLE *prWtbl;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucWlanIdx].ucUsed) {
		if (ucBssIndex == prWtbl[ucWlanIdx].ucBssIndex)
			return FALSE;

		DBGLOG(RSN, INFO,
			"The Wlan index #%d is in use by other bss! BSSIdx=%d keyid=%d P=%d STA=%d Addr="
			MACSTR "\n", ucWlanIdx,
			prWtbl[ucWlanIdx].ucBssIndex,
			prWtbl[ucWlanIdx].ucKeyId,
			prWtbl[ucWlanIdx].ucPairwise,
			prWtbl[ucWlanIdx].ucStaIndex,
			MAC2STR(prWtbl[ucWlanIdx].aucMacAddr));
		return TRUE;
	}

	return FALSE;
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
void secPostUpdateAddr(struct ADAPTER *prAdapter,
		       struct BSS_INFO *prBssInfo)
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
uint8_t *secGetEthBody(uint8_t *pucPkt)
{
	uint8_t *pucEthBody = NULL;
	uint8_t ucEapolType;
	uint16_t u2EtherTypeLen;
	uint8_t ucEthTypeLenOffset = ETHER_HEADER_LEN - ETHER_TYPE_LEN;

	WLAN_GET_FIELD_BE16(&pucPkt[ucEthTypeLenOffset],
			    &u2EtherTypeLen);
	if (u2EtherTypeLen == ETH_P_VLAN) {
		ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
		WLAN_GET_FIELD_BE16(&pucPkt[ucEthTypeLenOffset],
				    &u2EtherTypeLen);
	}
	if (u2EtherTypeLen != ETH_P_1X)
		return NULL;
	pucEthBody = &pucPkt[ucEthTypeLenOffset + ETHER_TYPE_LEN];
	ucEapolType = pucEthBody[1];
	if (ucEapolType != 3)	/* eapol key type */
		return NULL;

	return pucEthBody;
}

enum ENUM_EAPOL_KEY_TYPE_T secGetEapolKeyType(uint8_t *pucPkt)
{
	uint8_t *pucEthBody = NULL;
	uint16_t u2KeyInfo = 0;

	do {
		ASSERT_BREAK(pucPkt != NULL);
		pucEthBody = secGetEthBody(pucPkt);
		if (!pucEthBody)
			break;
		WLAN_GET_FIELD_BE16(&pucEthBody[5], &u2KeyInfo);

		if ((u2KeyInfo & 0x1100) == 0x0000 ||
			(u2KeyInfo & 0x0008) == 0x0000)
			return EAPOL_KEY_1_OF_4;
		else if ((u2KeyInfo & 0xfff0) == 0x0100)
			return EAPOL_KEY_2_OF_4;
		else if ((u2KeyInfo & 0xfff0) == 0x13c0)
			return EAPOL_KEY_3_OF_4;
		else if ((u2KeyInfo & 0xfff0) == 0x0300)
			return EAPOL_KEY_4_OF_4;
	} while (FALSE);

	return EAPOL_KEY_NOT_KEY;
}

void secHandleNoWtbl(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	/* Wtbl error handling. if no Wtbl */
	struct WLAN_MAC_MGMT_HEADER *prMgmtHdr = prSwRfb->pvHeader;

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

void secCheckRxEapolPacketEncryption(struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb,
	struct STA_RECORD *prStaRec)
{
	uint8_t *pucPkt = NULL;
	uint16_t u2EtherType;
	uint16_t u2FrameCtrl = 0;

	if (!prStaRec)
		return;

	if (prRetSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return;

	pucPkt = prRetSwRfb->pvHeader;
	if (!pucPkt)
		return;

	u2EtherType = (pucPkt[ETH_TYPE_LEN_OFFSET] << 8)
		| (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);
	if (u2EtherType != ETH_P_1X)
		return;

	if (prRetSwRfb->fgHdrTran == FALSE)
		u2FrameCtrl = ((struct WLAN_MAC_HEADER *)
			prRetSwRfb->pvHeader)->u2FrameCtrl;
	else if (prRetSwRfb->prRxStatusGroup4)
		u2FrameCtrl = HAL_RX_STATUS_GET_FRAME_CTL_FIELD(
			prRetSwRfb->prRxStatusGroup4);

	prStaRec->fgIsEapEncrypt = RXM_IS_PROTECTED_FRAME(u2FrameCtrl);
}
