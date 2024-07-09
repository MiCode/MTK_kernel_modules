/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/mgmt/auth.h#1
*/

/*! \file  auth.h
*    \brief This file contains the authentication REQ/RESP of
*	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
*/


#ifndef _AUTH_H
#define _AUTH_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                         D A T A   T Y P E S
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
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/* Routines in auth.c                                                         */
/*----------------------------------------------------------------------------*/
VOID authAddIEChallengeText(IN P_ADAPTER_T prAdapter, IN OUT P_MSDU_INFO_T prMsduInfo);

#if !CFG_SUPPORT_AAA
WLAN_STATUS authSendAuthFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec, IN UINT_16 u2TransactionSeqNum);
#else
WLAN_STATUS
authSendAuthFrame(IN P_ADAPTER_T prAdapter,
		  IN P_STA_RECORD_T prStaRec,
		  IN UINT_8 uBssIndex,
		  IN P_SW_RFB_T prFalseAuthSwRfb, IN UINT_16 u2TransactionSeqNum, IN UINT_16 u2StatusCode);
#endif /* CFG_SUPPORT_AAA */

WLAN_STATUS authCheckTxAuthFrame(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN UINT_16 u2TransactionSeqNum);

WLAN_STATUS authCheckRxAuthFrameTransSeq(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

WLAN_STATUS
authCheckRxAuthFrameStatus(IN P_ADAPTER_T prAdapter,
			   IN P_SW_RFB_T prSwRfb, IN UINT_16 u2TransactionSeqNum, OUT PUINT_16 pu2StatusCode);

VOID authHandleIEChallengeText(P_ADAPTER_T prAdapter, P_SW_RFB_T prSwRfb, P_IE_HDR_T prIEHdr);

WLAN_STATUS authProcessRxAuth2_Auth4Frame(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb);

WLAN_STATUS
authSendDeauthFrame(IN P_ADAPTER_T prAdapter,
		    IN P_BSS_INFO_T prBssInfo,
		    IN P_STA_RECORD_T prStaRec,
		    IN P_SW_RFB_T prClassErrSwRfb, IN UINT_16 u2ReasonCode, IN PFN_TX_DONE_HANDLER pfTxDoneHandler);

WLAN_STATUS authProcessRxDeauthFrame(IN P_SW_RFB_T prSwRfb, IN UINT_8 aucBSSID[], OUT PUINT_16 pu2ReasonCode);

#if CFG_SUPPORT_SOFTAP_WPA3
WLAN_STATUS
authProcessRxAuthFrame(IN P_ADAPTER_T prAdapter,
					   IN P_SW_RFB_T prSwRfb,
					   IN P_BSS_INFO_T prP2pBssInfo,
					   OUT PUINT_16 pu2ReturnStatusCode);
#else
WLAN_STATUS
authProcessRxAuth1Frame(IN P_ADAPTER_T prAdapter,
			IN P_SW_RFB_T prSwRfb,
			IN UINT_8 aucExpectedBSSID[],
			IN UINT_16 u2ExpectedAuthAlgNum,
			IN UINT_16 u2ExpectedTransSeqNum, OUT PUINT_16 pu2ReturnStatusCode);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
BOOLEAN
authFloodingCheck(IN P_ADAPTER_T prAdapter,
			IN P_BSS_INFO_T prP2pBssInfo,
			IN P_SW_RFB_T prSwRfb);
#endif /* _AUTH_H */
