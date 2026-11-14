/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   rtt.h
 *  \brief  The rtt related define, macro and structure are described here.
 */

#ifndef _RTT_H
#define _RTT_H

#if (CFG_SUPPORT_RTT == 1)
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

/* RTT state */
enum ENUM_RTT_STATE {
	RTT_STATE_IDLE,
	RTT_STATE_START,
	RTT_STATE_DONE,
	RTT_STATE_NUM
};

enum ENUM_RTT_EVENT_TYPE {
	RTT_EVENT_PER_PACKET = 0x1,
	RTT_EVENT_PER_BURST  = 0x2,
	RTT_EVENT_NUM
};

enum ENUM_RTT_BIAS_GROUP {
	RTT_BIAS_5G_GROUP1,
	RTT_BIAS_5G_GROUP2,
	RTT_BIAS_5G_GROUP3,
	RTT_BIAS_NUM
};

struct RTT_INFO {
	uint8_t ucBssIndex;
	uint8_t fgIsRunning;
	uint8_t fgIsContRunning; /* for continuous RTT requests */
	uint8_t ucSeqNum;
	enum ENUM_RTT_STATE ucState;
	enum ENUM_RTT_PEER_TYPE eRttPeerType;
	struct LINK rResultList;
	struct LINK rClientList;
	struct TIMER rRttDoneTimer;
	struct TIMER rRttContTimer; /* for continuous RTT requests */
};

struct RTT_CAL_INFO {
	/* Config Setting, sync with Location IP */
	/*	Byte 0: Version (default: 1, 1: V1.0, 2: +bw320)
	 *	Byte 1: Scale (default : ps, 1: 1ps, 2:256ps)
	 *	Byte 2: WFx Enable (bit 0: wf0, b1:wf1…)
	 *	+ phy indicator(b4: phy0,b5:phy1) 多phy, in-order
	 *	Byte 3~7: Reserved byte
	 */
	uint8_t ucVersion;
	uint8_t ucScale;
	uint8_t ucWF0Enable: 1;
	uint8_t ucWF1Enable: 1;
	uint8_t ucWF2Enable: 1;
	uint8_t ucWF3Enable: 1;
	uint8_t ucWFxReserved: 4;
	uint8_t ucReserved[5];
};

struct RTT_BIAS_TABLE_2G {
	uint32_t u4CBW20DBW20;
	uint32_t u4CBW40DBW20;
	uint32_t u4CBW40DBW40;
};

struct RTT_BIAS_TABLE_56G {
	uint32_t u4CBW20DBW20;
	uint32_t u4CBW40DBW20;
	uint32_t u4CBW40DBW40;
	uint32_t u4CBW80DBW20;
	uint32_t u4CBW80DBW40;
	uint32_t u4CBW80DBW80;
	uint32_t u4CBW160DBW20;
	uint32_t u4CBW160DBW40;
	uint32_t u4CBW160DBW80;
	uint32_t u4CBW160DBW160;
};

struct RTT_BIAS_TABLE {
	struct RTT_CAL_INFO rttCalInfo;
	struct RTT_BIAS_TABLE_2G rttBias2G[14];
	struct RTT_BIAS_TABLE_56G rttBias5G[3]; /* low, medium, high */
	struct RTT_BIAS_TABLE_56G rttBias6G;
};

struct RTT_RESULT_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	struct RTT_RESULT rResult;
	uint16_t u2IELen;
	/* Keep it last */
	uint8_t aucIE[];
};

struct RTT_CLIENT_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	enum ENUM_STA_TYPE eStaType;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucSeqNum;
};

struct PARAM_RTT_REQUEST {
	uint8_t fgEnable;
	uint8_t ucConfigNum;
	struct RTT_CONFIG arRttConfigs[RTT_MAX_CANDIDATES];
};

struct CMD_RTT_REQUEST {
	uint8_t ucSeqNum;
	uint8_t fgEnable;              /* request or cancel */
	uint8_t ucConfigNum;
	uint8_t ucPaddings[5];
	struct RTT_CONFIG arRttConfigs[RTT_MAX_CANDIDATES];
};

struct EVENT_RTT_RESULT {
	struct RTT_RESULT rResult;
	uint16_t u2IELen;
	/* Keep it last */
	uint8_t aucIE[];
};

struct EVENT_RTT_DONE {
	uint8_t ucSeqNum;
};

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

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
struct RTT_INFO *rttGetInfo(struct ADAPTER *prAdapter);

void rttInit(struct ADAPTER *prAdapter);

void rttUninit(struct ADAPTER *prAdapter);

uint8_t rttIsRunning(struct ADAPTER *prAdapter);

uint8_t rttBwToBssBw(uint8_t eRttBw);

uint8_t rttBssBwToRttBw(uint8_t ucBssBw);

uint8_t rttMaxBwToRttBw(uint8_t ucMaxBw);

uint8_t rttRttBwToMaxBw(uint8_t eRttBw);

uint32_t rttHandleRttRequest(struct ADAPTER *prAdapter,
	struct PARAM_RTT_REQUEST *prRequest,
	uint8_t ucBssIndex);

void rttEventGetCapabilities(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void rttEventDone(struct ADAPTER *prAdapter,
	struct EVENT_RTT_DONE *prEvent);

void rttEventResult(struct ADAPTER *prAdapter,
	struct EVENT_RTT_RESULT *prEvent);

#if (CFG_SUPPORT_RTT_RSTA == 1)
void rttProcessPublicAction(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb);
#endif /* CFG_SUPPORT_RTT_RSTA */

#endif /* CFG_SUPPORT_RTT */
#endif /* _RTT_H */
