/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  assoc.h
 *    \brief This file contains the ASSOC REQ/RESP of
 *	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */


#ifndef _ASSOC_H
#define _ASSOC_H

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
/* Routines in assoc.c                                                        */
/*----------------------------------------------------------------------------*/
uint32_t assocSendReAssocReqFrame(struct ADAPTER
				*prAdapter, struct STA_RECORD *prStaRec);

uint32_t assocCheckTxReAssocReqFrame(struct ADAPTER
				*prAdapter, struct MSDU_INFO *prMsduInfo);

uint32_t assocCheckTxReAssocRespFrame(struct ADAPTER
				*prAdapter, struct MSDU_INFO *prMsduInfo);

uint32_t
assocCheckRxReAssocRspFrameStatus(struct ADAPTER
				*prAdapter, struct SW_RFB *prSwRfb,
				uint16_t *pu2StatusCode);

uint32_t
assocProcessRxDisassocFrame(struct ADAPTER *prAdapter,
				struct SW_RFB *prSwRfb,
				uint8_t aucBSSID[],
				uint16_t *pu2ReasonCode);

uint32_t assocProcessRxAssocReqFrame(struct ADAPTER
				*prAdapter, struct SW_RFB *prSwRfb,
				uint16_t *pu2StatusCode);

uint32_t assocSendReAssocRespFrame(struct ADAPTER
				*prAdapter, struct STA_RECORD *prStaRec);

struct MSDU_INFO *assocComposeReAssocRespFrame(struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec);

uint16_t assocBuildCapabilityInfo(struct ADAPTER
				*prAdapter, struct STA_RECORD *prStaRec);

uint16_t assoc_get_nonwfa_vend_ie_len(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

void assoc_build_nonwfa_vend_ie(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void assocGenerateMDIE(struct ADAPTER *prAdapter,
		       struct MSDU_INFO *prMsduInfo);

uint32_t assocCalculateConnIELen(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			     struct STA_RECORD *prStaRec);

void assocGenerateConnIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

#endif /* _ASSOC_H */
