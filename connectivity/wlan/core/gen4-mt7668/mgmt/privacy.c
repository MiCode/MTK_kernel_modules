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
** Id: mgmt/privacy.c#1
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
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
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
VOID secInit(IN P_ADAPTER_T prAdapter, IN UINT_8 ucBssIndex)
{
	UINT_8 i;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prBssInfo;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

	DEBUGFUNC("secInit");

	ASSERT(prAdapter);

	prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;

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

	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[0].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_WEP40;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[1].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_TKIP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[2].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_CCMP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[3].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_WEP104;

	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[4].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_WEP40;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[5].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_TKIP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[6].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[7].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_WEP104;
#if CFG_SUPPORT_CFG80211_AUTH
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[8]
		.dot11RSNAConfigPairwiseCipher =
		RSN_CIPHER_SUITE_GROUP_NOT_USED;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[9]
		.dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_GCMP_256;
#endif
	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
		prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[0].dot11RSNAConfigAuthenticationSuite =
	    WPA_AKM_SUITE_NONE;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[1].dot11RSNAConfigAuthenticationSuite =
	    WPA_AKM_SUITE_802_1X;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[2].dot11RSNAConfigAuthenticationSuite =
	    WPA_AKM_SUITE_PSK;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[3].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_NONE;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[4].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_802_1X;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[5].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_PSK;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable
		[6].dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_FT_802_1X;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable
		[7].dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_FT_PSK;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable
		[8].dot11RSNAConfigAuthenticationSuite = RSN_AKM_SUITE_DPP;
#if CFG_SUPPORT_802_11W
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable
		[9].dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_802_1X_SHA256;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable
		[10].dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_PSK_SHA256;
#endif
#if CFG_SUPPORT_CFG80211_AUTH
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[11]
		.dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_8021X_SUITE_B;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[12]
		.dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_8021X_SUITE_B_192;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[13]
		.dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_SAE;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[14]
		.dot11RSNAConfigAuthenticationSuite
		= RSN_AKM_SUITE_OWE;
#endif


	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
		prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[i].dot11RSNAConfigAuthenticationSuiteEnabled =
		    FALSE;
	}

	secClearPmkid(prAdapter);

	cnmTimerInitTimer(prAdapter,
			  &prAisSpecBssInfo->rPreauthenticationTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) rsnIndicatePmkidCand, (ULONG) NULL);

#if CFG_SUPPORT_802_11W
	cnmTimerInitTimer(prAdapter,
			  &prAisSpecBssInfo->rSaQueryTimer, (PFN_MGMT_TIMEOUT_FUNC) rsnStartSaQueryTimer, (ULONG) NULL);
#endif

	prAisSpecBssInfo->fgCounterMeasure = FALSE;
	prAdapter->prAisBssInfo->ucBcDefaultKeyIdx = 0xff;
	prAdapter->prAisBssInfo->fgBcDefaultKeyExist = FALSE;

#if 0
	for (i = 0; i < WTBL_SIZE; i++) {
		g_prWifiVar->arWtbl[i].ucUsed = FALSE;
		g_prWifiVar->arWtbl[i].prSta = NULL;
		g_prWifiVar->arWtbl[i].ucNetTypeIdx = NETWORK_TYPE_INDEX_NUM;

	}
	nicPrivacyInitialize((UINT_8) NETWORK_TYPE_INDEX_NUM);
#endif

}				/* secInit */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "Rx Class Error" to SEC_FSM for
*        JOIN Module.
*
* \param[in] prAdapter     Pointer to the Adapter structure
* \param[in] prSwRfb       Pointer to the SW RFB.
*
* \return FALSE                Class Error
*/
/*----------------------------------------------------------------------------*/
BOOL secCheckClassError(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, IN P_STA_RECORD_T prStaRec)
{
	P_HW_MAC_RX_DESC_T prRxStatus;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxStatus = prSwRfb->prRxStatus;

#if 1
	if (!prStaRec || (prRxStatus->u2StatusFlag & RXS_DW2_RX_CLASSERR_BITMAP) == RXS_DW2_RX_CLASSERR_VALUE) {

		DBGLOG(RSN, TRACE,
		       "prStaRec=%p RX Status = 0x%x RX_CLASSERR check!\n",
			   prStaRec,
			   prRxStatus->u2StatusFlag);

		/* if (IS_NET_ACTIVE(prAdapter, ucBssIndex)) { */
		authSendDeauthFrame(prAdapter,
				    NULL, NULL, prSwRfb, REASON_CODE_CLASS_3_ERR, (PFN_TX_DONE_HANDLER) NULL);
		return FALSE;
		/* } */
	}
#else
	if ((prStaRec) && 1 /* RXM_IS_DATA_FRAME(prSwRfb) */) {
		UINT_8 ucBssIndex = prStaRec->ucBssIndex;

		if (IS_NET_ACTIVE(prAdapter, ucBssIndex)) {
			/* P_BSS_INFO_T prBssInfo; */
			/* prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex); */

			if ((prRxStatus->u2StatusFlag & RXS_DW2_RX_CLASSERR_BITMAP) == RXS_DW2_RX_CLASSERR_VALUE) {
				/*if ((STA_STATE_3 != prStaRec->ucStaState) &&
				 * IS_BSS_ACTIVE(prBssInfo) &&
				 * prBssInfo->fgIsNetAbsent == FALSE) {
				 * (IS_AP_STA(prStaRec) || IS_CLIENT_STA(prStaRec))) {
				 */
				if (authSendDeauthFrame(prAdapter,
						       prStaRec,
						       NULL,
						       REASON_CODE_CLASS_3_ERR,
						       (PFN_TX_DONE_HANDLER)
						       NULL) == WLAN_STATUS_SUCCESS) {
					DBGLOG(RSN, TRACE,
					       ("Send Deauth to MAC:[" MACSTR
						"] for Rx Class 3 Error.\n", MAC2STR(prStaRec->aucMacAddr)));
				}

				return FALSE;
			}

			return secRxPortControlCheck(prAdapter, prSwRfb);
		}
	}
#endif
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
VOID secSetPortBlocked(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta, IN BOOLEAN fgPortBlock)
{
#if 0				/* Marked for MT6630 */
	if (prSta == NULL)
		return;

	prSta->fgPortBlock = fgPortBlock;

	DBGLOG(RSN, TRACE,
	       "The STA " MACSTR " port %s\n", MAC2STR(prSta->aucMacAddr), fgPortBlock == TRUE ? "BLOCK" : " OPEN");
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
BOOLEAN secGetPortStatus(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta, OUT PBOOLEAN pfgPortStatus)
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
BOOL				/* ENUM_PORT_CONTROL_RESULT */
secTxPortControlCheck(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN P_STA_RECORD_T prStaRec)
{
	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(prStaRec);

	if (prStaRec) {

		/* Todo:: */
		if (prMsduInfo->fgIs802_1x)
			return TRUE;

		if (prStaRec->fgPortBlock == TRUE) {
			DBGLOG(INIT, TRACE, "Drop Tx packet due Port Control!\n");
			return FALSE;
		}
#if CFG_SUPPORT_WAPI
		if (prAdapter->rWifiVar.rConnSettings.fgWapiMode)
			return TRUE;
#endif
		if (IS_STA_IN_AIS(prStaRec)) {
			if (!prAdapter->rWifiVar.rAisSpecificBssInfo.fgTransmitKeyExist &&
			    (prAdapter->rWifiVar.rConnSettings.eEncStatus == ENUM_ENCRYPTION1_ENABLED)) {
				DBGLOG(INIT, TRACE, "Drop Tx packet due the key is removed!!!\n");
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
BOOLEAN secRxPortControlCheck(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSWRfb)
{
	ASSERT(prSWRfb);

#if 0
	/* whsu:Todo: Process MGMT and DATA */
	if (prSWRfb->prStaRec) {
		if (prSWRfb->prStaRec->fgPortBlock == TRUE) {
			if (1 /* prSWRfb->fgIsDataFrame and not 1x */  &&
			    (g_prWifiVar->rConnSettings.eAuthMode >= AUTH_MODE_WPA)) {
				/* DBGLOG(SEC, WARN, ("Drop Rx data due port control !\r\n")); */
				return TRUE;	/* Todo: whsu FALSE; */
			}
			/* if (!RX_STATUS_IS_PROTECT(prSWRfb->prRxStatus)) { */
			/* DBGLOG(RSN, WARN, ("Drop rcv non-encrypted data frame!\n")); */
			/* return FALSE; */
			/* } */
		}
	} else {
	}
#endif
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
VOID secSetCipherSuite(IN P_ADAPTER_T prAdapter, IN UINT_32 u4CipherSuitesFlags)
{

	UINT_32 i;
	P_DOT11_RSNA_CONFIG_PAIRWISE_CIPHERS_ENTRY prEntry;
	P_IEEE_802_11_MIB_T prMib;

	ASSERT(prAdapter);

	prMib = &prAdapter->rMib;

	ASSERT(prMib);

	if (u4CipherSuitesFlags == CIPHER_FLAG_NONE) {
		/* Disable all the pairwise cipher suites. */
		for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
			prMib->dot11RSNAConfigPairwiseCiphersTable[i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

		/* Update the group cipher suite. */
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

		return;
	}

	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++) {
		prEntry = &prMib->dot11RSNAConfigPairwiseCiphersTable[i];

		switch (prEntry->dot11RSNAConfigPairwiseCipher) {
#if CFG_SUPPORT_SUITB
		case RSN_CIPHER_SUITE_GCMP_256:
			if (u4CipherSuitesFlags & CIPHER_FLAG_GCMP256)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled
					= TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled
					= FALSE;
			break;
#endif
		case WPA_CIPHER_SUITE_WEP40:
		case RSN_CIPHER_SUITE_WEP40:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP40)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;

		case WPA_CIPHER_SUITE_TKIP:
		case RSN_CIPHER_SUITE_TKIP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_TKIP)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;

		case WPA_CIPHER_SUITE_CCMP:
		case RSN_CIPHER_SUITE_CCMP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_CCMP)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;

		case WPA_CIPHER_SUITE_WEP104:
		case RSN_CIPHER_SUITE_WEP104:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP104)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;
		default:
			break;
		}
	}

	/* Update the group cipher suite. */
	if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_CCMP, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_CCMP;
	else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_TKIP, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_TKIP;
	else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_WEP104, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP104;
	else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_WEP40, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP40;
#if CFG_SUPPORT_SUITB
	else if (rsnSearchSupportedCipher(prAdapter,
		RSN_CIPHER_SUITE_GROUP_NOT_USED, &i))
		prMib->dot11RSNAConfigGroupCipher =
		RSN_CIPHER_SUITE_GROUP_NOT_USED;
	else if (rsnSearchSupportedCipher(prAdapter,
		RSN_CIPHER_SUITE_GCMP_256, &i))
		prMib->dot11RSNAConfigGroupCipher =
		RSN_CIPHER_SUITE_GCMP_256;
#endif
	else
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

}				/* secSetCipherSuite */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to initialize the pmkid parameters.
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
VOID secClearPmkid(IN P_ADAPTER_T prAdapter)
{
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

	DEBUGFUNC("secClearPmkid");

	prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
	DBGLOG(RSN, TRACE, "secClearPmkid\n");
	prAisSpecBssInfo->u4PmkidCandicateCount = 0;
	prAisSpecBssInfo->u4PmkidCacheCount = 0;
	kalMemZero((PVOID) prAisSpecBssInfo->arPmkidCandicate, sizeof(PMKID_CANDICATE_T) * CFG_MAX_PMKID_CACHE);
	kalMemZero((PVOID) prAisSpecBssInfo->arPmkidCache, sizeof(PMKID_ENTRY_T) * CFG_MAX_PMKID_CACHE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Whether 802.11 privacy is enabled.
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \retval BOOLEAN
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secEnabledInAis(IN P_ADAPTER_T prAdapter)
{
	DEBUGFUNC("secEnabledInAis");

	ASSERT(prAdapter->rWifiVar.rConnSettings.eEncStatus <
		ENUM_ENCRYPTION_NUM);

	if ((prAdapter->rWifiVar.rConnSettings.eEncStatus
		== ENUM_ENCRYPTION1_ENABLED)
		|| (prAdapter->rWifiVar.rConnSettings.eEncStatus
		== ENUM_ENCRYPTION2_ENABLED)
		|| (prAdapter->rWifiVar.rConnSettings.eEncStatus
		== ENUM_ENCRYPTION3_ENABLED)
#if CFG_SUPPORT_SUITB
		|| (prAdapter->rWifiVar.rConnSettings.eEncStatus
		== ENUM_ENCRYPTION4_ENABLED)
#endif
		)
		return TRUE;
	else if (prAdapter->rWifiVar.rConnSettings.eEncStatus
		== ENUM_ENCRYPTION_DISABLED)
		DBGLOG(RSN, TRACE, "Unknown encryption setting %d\n",
			prAdapter->rWifiVar.rConnSettings.eEncStatus);
	return FALSE;
}				/* secEnabledInAis */

BOOLEAN secIsProtected1xFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	P_BSS_INFO_T prBssInfo;

	ASSERT(prAdapter);

	if (prStaRec) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
		if (prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_WAPI
			if (wlanQueryWapiMode(prAdapter))
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
BOOLEAN secIsProtectedFrame(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsdu, IN P_STA_RECORD_T prStaRec)
{
	/* P_BSS_INFO_T prBssInfo; */

	ASSERT(prAdapter);
	ASSERT(prMsdu);
	/* ASSERT(prStaRec); */

#if CFG_SUPPORT_802_11W
	if (prMsdu->ucPacketType == TX_PACKET_TYPE_MGMT) {
#if 0				/* Decide by Compose module */
		BOOL fgRobustActionWithProtect = FALSE;
		P_BSS_INFO_T prBssInfo;

		if (prStaRec) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
			ASSERT(prBssInfo);
			if ((prBssInfo->eNetworkType == NETWORK_TYPE_AIS) &&
			    prAdapter->rWifiVar.rAisSpecificBssInfo.fgMgmtProtection /* Use MFP */) {

				fgRobustActionWithProtect = TRUE;
				return TRUE; /* AIS & Robust action frame */
			}
		}
#endif
		return FALSE;
	}
#else
	if (prMsdu->ucPacketType == TX_PACKET_TYPE_MGMT)
		return FALSE;
#endif

	return secIsProtectedBss(prAdapter, GET_BSS_INFO_BY_INDEX(prAdapter, prMsdu->ucBssIndex));
}

BOOLEAN secIsProtectedBss(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
	ASSERT(prBssInfo);

	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_WAPI
		if (wlanQueryWapiMode(prAdapter))
			return TRUE;
#endif
		return secEnabledInAis(prAdapter);
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
		return kalP2PGetCipher(prAdapter->prGlueInfo, (UINT_8) prBssInfo->u4PrivateData);
#endif
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW)
		return TRUE;

	ASSERT(FALSE);
	return FALSE;
}

BOOLEAN secIsUsedByStaRecEntry(IN P_ADAPTER_T prAdapter, IN UINT_8 Index)
{
	P_STA_RECORD_T prStaRec = NULL;
	UINT_8 i = 0;
	BOOLEAN fgIsUsed = FALSE;

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
BOOL secPrivacySeekForEntry(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_BSS_INFO_T prP2pBssInfo;
	UINT_8 ucEntry = WTBL_RESERVED_ENTRY;
	UINT_8 i;
	UINT_8 ucStartIDX = 0, ucMaxIDX = 0;
	P_WLAN_TABLE_T prWtbl;
	UINT_8 ucRoleIdx = 0;

	ASSERT(prSta);

	if (!prSta->fgIsInUse)
		ASSERT(FALSE);

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
	ucRoleIdx = prP2pBssInfo->u4PrivateData;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	ucStartIDX = 1;
	ucMaxIDX = NIC_TX_DEFAULT_WLAN_INDEX - 1;

	DBGLOG(RSN, INFO, "secPrivacySeekForEntry\n");

	for (i = ucStartIDX; i <= ucMaxIDX; i++) {
		if (prWtbl[i].ucUsed && EQUAL_MAC_ADDR(prSta->aucMacAddr, prWtbl[i].aucMacAddr)
		    && prWtbl[i].ucPairwise /* This function for ucPairwise only */) {
			ucEntry = i;
			DBGLOG(RSN, INFO, "[Wlan index]: Reuse entry #%d\n", i);
			break;
		}
	}

	if (i == (ucMaxIDX + 1)) {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
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
	if (ucEntry < NIC_TX_DEFAULT_WLAN_INDEX) {

		prWtbl[ucEntry].ucUsed = TRUE;
		prWtbl[ucEntry].ucBssIndex = prSta->ucBssIndex;
		prWtbl[ucEntry].ucKeyId = 0xFF;
		prWtbl[ucEntry].ucPairwise = 1;
		COPY_MAC_ADDR(prWtbl[ucEntry].aucMacAddr, prSta->aucMacAddr);
		prWtbl[ucEntry].ucStaIndex = prSta->ucIndex;

		prSta->ucWlanIndex = ucEntry;

		{
			P_BSS_INFO_T prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
			/* for AP mode , if wep key exist, peer sta should also fgTransmitKeyExist */
			if (IS_BSS_P2P(prBssInfo) && kalP2PGetRole(prAdapter->prGlueInfo, ucRoleIdx) == 2) {
				if (prBssInfo->fgBcDefaultKeyExist &&
				    !(kalP2PGetCcmpCipher(prAdapter->prGlueInfo, ucRoleIdx) ||
				      kalP2PGetTkipCipher(prAdapter->prGlueInfo, ucRoleIdx))) {
					prSta->fgTransmitKeyExist = TRUE;
					prWtbl[ucEntry].ucKeyId = prBssInfo->ucBcDefaultKeyIdx;
					DBGLOG(RSN, INFO, "peer sta set fgTransmitKeyExist\n");
				}
			}
		}

		DBGLOG(RSN, INFO,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d " MACSTR
		       " staType=%x\n", prSta->ucBssIndex, 0, prWtbl[ucEntry].ucPairwise, ucEntry,
		       prSta->ucIndex, MAC2STR(prSta->aucMacAddr), prSta->eStaType);
#if 1				/* DBG */
		secCheckWTBLAssign(prAdapter);
#endif
		return TRUE;
	}
#if DBG
	secCheckWTBLAssign(prAdapter);
#endif
	DBGLOG(RSN, WARN, "[Wlan index] No more wlan table entry available!!!!\n");
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
VOID secPrivacyFreeForEntry(IN P_ADAPTER_T prAdapter, IN UINT_8 ucEntry)
{
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);

	if (ucEntry >= WTBL_SIZE)
		return;

	DBGLOG(RSN, INFO, "secPrivacyFreeForEntry %d", ucEntry);

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucEntry].ucUsed) {
		prWtbl[ucEntry].ucUsed = FALSE;
		prWtbl[ucEntry].ucKeyId = 0xff;
		prWtbl[ucEntry].ucBssIndex = MAX_BSS_INDEX + 1;
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
VOID secPrivacyFreeSta(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	UINT_32 entry;
	P_WLAN_TABLE_T prWtbl;

	if (!prStaRec)
		return;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	for (entry = 0; entry < WTBL_SIZE; entry++) {
		/* Consider GTK case !! */
		if (prWtbl[entry].ucUsed &&
		    EQUAL_MAC_ADDR(prStaRec->aucMacAddr, prWtbl[entry].aucMacAddr) && prWtbl[entry].ucPairwise) {
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
VOID secRemoveBssBcEntry(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo, IN BOOL fgRoam)
{
	int i;
	P_CONNECTION_SETTINGS_T prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	if (!prBssInfo)
		return;

	DBGLOG(RSN, INFO, "remove all the key related with BSS!");

	if (fgRoam) {
		if (IS_BSS_AIS(prBssInfo) &&
		    prBssInfo->prStaRecOfAP && (prConnSettings->eAuthMode >= AUTH_MODE_WPA &&
						prConnSettings->eAuthMode != AUTH_MODE_WPA_NONE)) {

			for (i = 0; i < MAX_KEY_NUM; i++) {
				if (prBssInfo->ucBMCWlanIndexSUsed[i])
					secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndexS[i]);

#if 0
				/* move to cfg delete cb function for sync. */
				prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
				prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
#endif
			}

			prBssInfo->fgBcDefaultKeyExist = FALSE;
			prBssInfo->ucBcDefaultKeyIdx = 0xff;
		}
	} else {
		/* According to wh.su's comment, it's ok to change to
		 * reserved_entry here so that the entry is _NOT_ freed at all.
		 * In this way, the same BSS(ucBssIndex) could reuse the same
		 * entry next time in secPrivacySeekForBcEntry(), and we could
		 * see the following log: "[Wlan index]: Reuse entry ...".
		 */
		prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
		secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndex);

		for (i = 0; i < MAX_KEY_NUM; i++) {
		/* Not to remove BMC WTBL entries to
		 *   sync with FW's behavior
		 *
		 *	if (prBssInfo->ucBMCWlanIndexSUsed[i])
		 *		secPrivacyFreeForEntry(prAdapter,
		 *				prBssInfo->ucBMCWlanIndexS[i]);
		 */
#if 0
			/* move to cfg delete cb function for sync. */
			prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
			prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
#endif
		}
		for (i = 0; i < MAX_KEY_NUM; i++) {
			if (prBssInfo->wepkeyUsed[i])
				secPrivacyFreeForEntry(prAdapter, prBssInfo->wepkeyWlanIdx);
			prBssInfo->wepkeyUsed[i] = FALSE;
		}
		prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
		prBssInfo->fgBcDefaultKeyExist = FALSE;
		prBssInfo->ucBcDefaultKeyIdx = 0xff;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used for adding the broadcast key used, to assign a wlan table entry
*         for reserved the specific entry for these key for
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] ucBssIndex The BSS index
* \param[in] ucNetTypeIdx The Network index
* \param[in] ucAlg the entry assign related with algorithm
* \param[in] ucKeyId The key id
* \param[in] ucTxRx The Type of the key
*
* \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation fail
*
* \note
*/
/*----------------------------------------------------------------------------*/
UINT_8
secPrivacySeekForBcEntry(IN P_ADAPTER_T prAdapter,
			 IN UINT_8 ucBssIndex,
			 IN PUINT_8 pucAddr, IN UINT_8 ucStaIdx, IN UINT_8 ucAlg, IN UINT_8 ucKeyId)
{
	UINT_8 ucEntry = WTBL_ALLOC_FAIL;
	UINT_8 ucStartIDX = 0, ucMaxIDX = 0;
	UINT_8 i;
	BOOLEAN fgCheckKeyId = TRUE;
	P_WLAN_TABLE_T prWtbl;
	P_BSS_INFO_T prBSSInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	prWtbl = prAdapter->rWifiVar.arWtbl;
	ASSERT(prAdapter);
	ASSERT(pucAddr);

	if (ucAlg == CIPHER_SUITE_WPI ||	/* CIPHER_SUITE_GCM_WPI || */
	    ucAlg == CIPHER_SUITE_WEP40 ||
	    ucAlg == CIPHER_SUITE_WEP104 || ucAlg == CIPHER_SUITE_WEP128 || ucAlg == CIPHER_SUITE_NONE)
		fgCheckKeyId = FALSE;

	if (ucKeyId == 0xFF || ucAlg == CIPHER_SUITE_BIP)
		fgCheckKeyId = FALSE;

	if (prBSSInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT)
		fgCheckKeyId = FALSE;

	if ((prBSSInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) &&
		(prBSSInfo->eNetworkType == NETWORK_TYPE_AIS)) {
		fgCheckKeyId = FALSE;
		DBGLOG(RSN, WARN, "Always install gtk in same wtbl\n");
	}

	ucStartIDX = 1;
	ucMaxIDX = NIC_TX_DEFAULT_WLAN_INDEX - 1;

	DBGLOG(INIT, INFO, "secPrivacySeekForBcEntry\n");
	DBGLOG(INIT, INFO, "OpMode:%d, NetworkType:%d, CheckKeyId:%d\n",
		prBSSInfo->eCurrentOPMode, prBSSInfo->eNetworkType, fgCheckKeyId);

	if (ucAlg == CIPHER_SUITE_BIP) {
		ucEntry = 0;
	} else {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
			if (!(prWtbl[i].ucUsed &&
				!prWtbl[i].ucPairwise &&
				prWtbl[i].ucBssIndex == ucBssIndex))
				continue;
			if (!fgCheckKeyId) {
				ucEntry = i;
				DBGLOG(RSN,
					INFO,
					"[Wlan index]: Reuse entry #%d for open/wep/wpi\n",
					i);
				break;
			}

			if (fgCheckKeyId &&
				(prWtbl[i].ucKeyId == ucKeyId ||
				prWtbl[i].ucKeyId == 0xFF)) {
				ucEntry = i;
				DBGLOG(RSN,
					INFO,
					"[Wlan index]: Reuse entry #%d\n",
					i);
				break;
			}
		}
		if (i == (ucMaxIDX + 1)) {
			for (i = ucStartIDX; i <= ucMaxIDX; i++) {
				if (prWtbl[i].ucUsed == TRUE)
					continue;
				ucEntry = i;
				DBGLOG(RSN,
					INFO,
					"[Wlan index]: Assign entry #%d\n",
					i);
				break;
			}
		}
	}

	if (ucEntry < NIC_TX_DEFAULT_WLAN_INDEX) {
		prWtbl[ucEntry].ucUsed = TRUE;
		prWtbl[ucEntry].ucKeyId = ucKeyId;
		prWtbl[ucEntry].ucBssIndex = ucBssIndex;
		prWtbl[ucEntry].ucPairwise = 0;
		kalMemCopy(prWtbl[ucEntry].aucMacAddr, pucAddr,
				MAC_ADDR_LEN);
		prWtbl[ucEntry].ucStaIndex = ucStaIdx;

		DBGLOG(RSN, INFO,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d " MACSTR
		       "\n", ucBssIndex, ucKeyId, prWtbl[ucEntry].ucPairwise, ucEntry, ucStaIdx, MAC2STR(pucAddr));

		/* DBG */
		secCheckWTBLAssign(prAdapter);

	} else {
		secCheckWTBLAssign(prAdapter);
		DBGLOG(RSN, ERROR, "[Wlan index] No more wlan entry available!!!!\n");
	}

	return ucEntry;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation fail
*
* \note
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secCheckWTBLAssign(IN P_ADAPTER_T prAdapter)
{

	BOOLEAN fgCheckFail = FALSE;

	secPrivacyDumpWTBL(prAdapter);

	/* AIS STA should just has max 2 entry */
	/* Max STA check */
	if (fgCheckFail)
		ASSERT(FALSE);

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
UINT_8 secGetStaIdxByWlanIdx(P_ADAPTER_T prAdapter, UINT_8 ucWlanIdx)
{
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);

	if (ucWlanIdx >= WTBL_SIZE)
		return STA_REC_INDEX_NOT_FOUND;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	/* DBGLOG(RSN, TRACE, ("secGetStaIdxByWlanIdx=%d "MACSTR" used=%d\n", ucWlanIdx,
	*			MAC2STR(prWtbl[ucWlanIdx].aucMacAddr), prWtbl[ucWlanIdx].ucUsed));
	*/

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucStaIndex;
	else
		return STA_REC_INDEX_NOT_FOUND;
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
UINT_8 secGetBssIdxByWlanIdx(P_ADAPTER_T prAdapter, UINT_8 ucWlanIdx)
{
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);

	if (ucWlanIdx >= WTBL_SIZE)
		return WTBL_RESERVED_ENTRY;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucBssIndex;
	else
		return WTBL_RESERVED_ENTRY;
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
UINT_8 secLookupStaRecIndexFromTA(P_ADAPTER_T prAdapter, PUINT_8 pucMacAddress)
{
	int i;
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);
	prWtbl = prAdapter->rWifiVar.arWtbl;

	for (i = 0; i < WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed) {
			if (EQUAL_MAC_ADDR(pucMacAddress, prWtbl[i].aucMacAddr) && prWtbl[i].ucPairwise)
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
void secPrivacyDumpWTBL(IN P_ADAPTER_T prAdapter)
{
	P_WLAN_TABLE_T prWtbl;
	UINT_8 i;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	DBGLOG(RSN, INFO, "The Wlan index\n");

	for (i = 0; i < WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed)
			DBGLOG(RSN, INFO,
				"#%d Used=%d  BSSIdx=%d keyid=%d P=%d STA=%d Addr=" MACSTR "\n", i,
				prWtbl[i].ucUsed, prWtbl[i].ucBssIndex, prWtbl[i].ucKeyId,
				prWtbl[i].ucPairwise, prWtbl[i].ucStaIndex, MAC2STR(prWtbl[i].aucMacAddr));
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
void secPostUpdateAddr(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	P_WLAN_TABLE_T prWtbl;

	if (IS_BSS_AIS(prBssInfo) && prBssInfo->prStaRecOfAP) {

		if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED) {

			if (prBssInfo->fgBcDefaultKeyExist) {

				prWtbl = &prAdapter->rWifiVar.arWtbl[prBssInfo->wepkeyWlanIdx];

				kalMemCopy(prWtbl->aucMacAddr, prBssInfo->prStaRecOfAP->aucMacAddr, MAC_ADDR_LEN);
				prWtbl->ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;
				DBGLOG(RSN, INFO, "secPostUpdateAddr at [%d] " MACSTR "= STA Index=%d\n",
				       prBssInfo->wepkeyWlanIdx, MAC2STR(prWtbl->aucMacAddr),
				       prBssInfo->prStaRecOfAP->ucIndex);

				/* Update the wlan table of the prStaRecOfAP */
				prWtbl = &prAdapter->rWifiVar.arWtbl[prBssInfo->prStaRecOfAP->ucWlanIndex];
				prWtbl->ucKeyId = prBssInfo->ucBcDefaultKeyIdx;
				prBssInfo->prStaRecOfAP->fgTransmitKeyExist = TRUE;
			}
		}
		if (prConnSettings->eEncStatus == ENUM_ENCRYPTION_DISABLED) {
			prWtbl = &prAdapter->rWifiVar.arWtbl[prBssInfo->ucBMCWlanIndex];

			kalMemCopy(prWtbl->aucMacAddr, prBssInfo->prStaRecOfAP->aucMacAddr, MAC_ADDR_LEN);
			prWtbl->ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;
			DBGLOG(RSN, INFO, "secPostUpdateAddr at [%d] " MACSTR "= STA Index=%d\n",
			       prBssInfo->ucBMCWlanIndex, MAC2STR(prWtbl->aucMacAddr),
			       prBssInfo->prStaRecOfAP->ucIndex);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Check if set key procedure is done or not
*
* \param[in] prStaRec which needs to be checked
*
* \note
*/
/*----------------------------------------------------------------------------*/
BOOL secCheckSetKeyDone(IN P_STA_RECORD_T prStaRec)
{
	if (!prStaRec) {
		DBGLOG(RSN, INFO, "prStaRec is NULL, return FALSE.\n");
		return FALSE;
	}

	/* Check TDLS case */
	if (prStaRec->eStaType == STA_TYPE_DLS_PEER) {
		if (prStaRec->fgIsTxPairwiseKeyReady == TRUE) {
			DBGLOG(RSN, INFO, "TDLS case without GTK.\n");
			return TRUE;
		}
	}
	/* Check WEP flow */
	if (prStaRec->fgIsTxPairwiseKeyReady == FALSE &&
		prStaRec->fgIsTxGroupKeyReady == TRUE &&
		prStaRec->ucGroupKeyCount == 2)
		return TRUE;
	/* Check TKIP/CCMP flow */
	else if (prStaRec->fgIsTxPairwiseKeyReady == TRUE &&
		prStaRec->fgIsTxGroupKeyReady == TRUE)
		return TRUE;
	else
		return FALSE;
}
