/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "epcs.h"
 *    \brief This file contains the internal used in EPCS modules
 *	 for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _EPCS_H
#define _EPCS_H
#if (CFG_SUPPORT_802_11BE_EPCS == 1)

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
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
#define EPCS_REQ_TX_TIMEOUT 3000
#define EPCS_RETRY_LIMIT 5

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
uint32_t epcsReqTxDoneCb(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t epcsRspTxDoneCb(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t epcsTeardownTxDoneCb(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, enum ENUM_TX_RESULT_CODE rTxDoneStatus);

void epcsTimeout(struct ADAPTER *prAdapter, uintptr_t ulParamPtr);

uint32_t epcsSend(struct ADAPTER *prAdapter, enum PROTECTED_EHT_ACTION eAction,
		struct BSS_INFO *prBssInfo);

void epcsProcessAction(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);

void epcsComposeReq(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken);

void epcsComposeRsp(struct MSDU_INFO *prMsduInfo, uint8_t ucDialogToken,
		uint16_t u2StatusCode);

void epcsMldMUnEdcaBackupRestore(struct MLD_BSS_INFO *prMldBssInfo,
		u_int8_t fgBackup);

void epcsMUnEdcaBackupRestore(struct BSS_INFO *prBssInfo, u_int8_t fgBackup);

void epcsMldMUnEdcaUpdate(struct ADAPTER *prAdapter,
		struct MLD_BSS_INFO *prMldBssInfo);
void epcsMldStaRecUpdate(struct ADAPTER *prAdapter,
		struct MLD_BSS_INFO *prMldBssInfo);
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* CFG_SUPPORT_802_11BE_EPCS */
#endif /* _EPCS_H */
