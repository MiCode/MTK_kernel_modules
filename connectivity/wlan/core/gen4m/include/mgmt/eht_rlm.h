/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "eht_rlm.h"
 *  \brief
 */

#ifndef _EHT_RLM_H
#define _EHT_RLM_H

#if (CFG_SUPPORT_802_11BE == 1)

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

u_int32_t ehtRlmCalculateCapIELen(
	struct ADAPTER *prAdapter,
	u_int8_t ucBssIndex,
	struct STA_RECORD *prStaRec);
void ehtRlmFillCapIE(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct MSDU_INFO *prMsduInfo);
u_int32_t ehtRlmCalculateOpIELen(
	struct ADAPTER *prAdapter,
	u_int8_t ucBssIndex,
	struct STA_RECORD *prStaRec);
void ehtRlmReqGenerateCapIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
void ehtRlmRspGenerateCapIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
void ehtRlmRspGenerateCapIEImpl(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
void ehtRlmRspGenerateOpIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
void ehtRlmRspGenerateOpIEImpl(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
void ehtRlmRecCapInfo(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	const uint8_t *pucIE);
void ehtRlmRecOperation(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct BSS_INFO *prBssInfo,
	const uint8_t *pucIE);
uint8_t ehtRlmGetVhtOpBwByEhtOpBw(
	struct EHT_OP_INFO *op);
void ehtRlmInit(
	struct ADAPTER *prAdapter);
void ehtRlmInitHtcACtrlOM(
	struct ADAPTER *prAdapter);
uint32_t ehtRlmFillBwIndicationIe(struct ADAPTER *prAdapter,
				  uint8_t *pucBuffer);
uint32_t ehtRlmCalMlTrafficIndiIELen(struct ADAPTER *prAdapter,
				     uint8_t ucBssIndex,
				     struct STA_RECORD *prStaRec);
void ehtRlmGenMlTrafficIndiIE(struct ADAPTER *prAdapter,
			      struct MSDU_INFO *prMsduInfo);
#if (CFG_SUPPORT_NAN_11BE == 1)
uint16_t ehtRlmNANFillCapIE(
	struct ADAPTER *ad,
	struct BSS_INFO *bss,
	uint8_t *buf);
uint16_t ehtRlmNANFillOpIE(
	struct ADAPTER *ad,
	struct BSS_INFO *bss,
	uint8_t *buf);
#endif /* CFG_SUPPORT_NAN_11BE == 1 */
#endif /* CFG_SUPPORT_802_11BE == 1 */
#endif /* !_EHT_RLM_H */
