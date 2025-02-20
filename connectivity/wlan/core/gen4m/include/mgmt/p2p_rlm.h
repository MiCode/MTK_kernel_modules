/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


/*! \file   "rlm.h"
 *  \brief
 */

#ifndef _P2P_RLM_H
#define _P2P_RLM_H

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

#define CHANNEL_SPAN_20 20

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

#if (CFG_SUPPORT_WIFI_6G == 1)
void rlmUpdate6GOpInfo(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);
#endif

#if CFG_ENABLE_WIFI_DIRECT
void rlmBssInitForAP(struct ADAPTER *prAdapter, struct BSS_INFO *prBssInfo);

u_int8_t rlmUpdateBwByChListForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

u_int8_t rlmUpdateParamsForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, u_int8_t fgUpdateBeacon);
#endif
void rlmBssUpdateChannelParams(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

void rlmFuncInitialChannelList(struct ADAPTER *prAdapter);

void
rlmFuncCommonChannelList(struct ADAPTER *prAdapter,
		struct CHANNEL_ENTRY_FIELD *prChannelEntryII,
		uint8_t ucChannelListSize);

uint8_t rlmFuncFindOperatingClass(struct ADAPTER *prAdapter,
		uint8_t ucChannelNum);

u_int8_t
rlmFuncFindAvailableChannel(struct ADAPTER *prAdapter,
		uint8_t ucCheckChnl,
		uint8_t *pucSuggestChannel,
		u_int8_t fgIsSocialChannel, u_int8_t fgIsDefaultChannel);

enum ENUM_CHNL_EXT rlmDecideScoForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

enum ENUM_CHNL_EXT rlmGetScoForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

enum ENUM_CHNL_EXT rlmGetScoByChnInfo(struct ADAPTER *prAdapter,
		struct RF_CHANNEL_INFO *prChannelInfo);

uint8_t rlmGetVhtS1ForAP(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

void rlmGetChnlInfoForCSA(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucCh,
	uint8_t ucBssIdx,
	struct RF_CHANNEL_INFO *prRfChnlInfo);

#endif
