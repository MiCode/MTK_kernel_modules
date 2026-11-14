/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  auth.h
 *    \brief This file contains the authentication REQ/RESP of
 *	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */


#ifndef _AUTH_H
#define _AUTH_H

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
 *                         D A T A   T Y P E S
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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/* Routines in auth.c                                                         */
/*----------------------------------------------------------------------------*/
void authAddIEChallengeText(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

uint32_t
authSendAuthFrame(struct ADAPTER *prAdapter,
			  struct STA_RECORD *prStaRec,
			  uint8_t uBssIndex,
			  struct SW_RFB *prFalseAuthSwRfb,
			  uint16_t u2TransactionSeqNum,
			  uint16_t u2StatusCode);

uint32_t authCheckTxAuthFrame(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo,
			uint16_t u2TransactionSeqNum);

uint32_t authCheckRxAuthFrameTransSeq(struct ADAPTER
			*prAdapter, struct SW_RFB *prSwRfb);

uint32_t
authCheckRxAuthFrameStatus(struct ADAPTER *prAdapter,
			   struct SW_RFB *prSwRfb,
			   uint16_t u2TransactionSeqNum,
			   uint16_t *pu2StatusCode);

uint32_t authHandleIEChallengeText(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, struct IE_HDR *prIEHdr);

uint32_t authHandleRSNE(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, struct IE_HDR *prIEHdr);

uint32_t authProcessRxAuth2_Auth4Frame(struct ADAPTER
				       *prAdapter, struct SW_RFB *prSwRfb);

uint32_t
authSendDeauthFrame(struct ADAPTER *prAdapter,
		    struct BSS_INFO *prBssInfo,
		    struct STA_RECORD *prStaRec,
		    struct SW_RFB *prClassErrSwRfb, uint16_t u2ReasonCode,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler);

uint32_t authProcessRxDeauthFrame(struct SW_RFB *prSwRfb,
			uint8_t aucBSSID[], uint16_t *pu2ReasonCode);

uint32_t
authProcessRxAuth1Frame(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb,
			uint8_t aucExpectedBSSID[],
			uint16_t u2ExpectedAuthAlgNum,
			uint16_t u2ExpectedTransSeqNum,
			uint16_t *pu2ReturnStatusCode);
uint32_t
authProcessRxAuthFrame(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb,
			struct BSS_INFO *prBssInfo,
			uint16_t *pu2ReturnStatusCode);

void authAddMDIE(struct ADAPTER *prAdapter,
		 struct MSDU_INFO *prMsduInfo);

uint32_t authCalculateFTIELen(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			     struct STA_RECORD *prStaRec);

void authAddFTIE(struct ADAPTER *prAdapter,
		 struct MSDU_INFO *prMsduInfo);

uint32_t authCalculateRSNIELen(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			       struct STA_RECORD *prStaRec);

void authAddRSNIE(struct ADAPTER *prAdapter,
		  struct MSDU_INFO *prMsduInfo);

uint32_t authAddRSNIE_impl(struct ADAPTER *prAdapter,
		  struct MSDU_INFO *prMsduInfo, uint8_t ucR0R1);

void authHandleFtIEs(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		     struct IE_HDR *prIEHdr);

#if CFG_SUPPORT_AAA
u_int8_t
authFloodingCheck(struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		struct SW_RFB *prSwRfb);
#endif
#endif /* _AUTH_H */
