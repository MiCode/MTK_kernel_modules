/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   pasn.h
 *  \brief  The PASN related define, macro and structure are described here.
 */

#ifndef _PASN_H
#define _PASN_H

#if CFG_SUPPORT_PASN
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
#define PASN_MAX_PEERS 10
#define PASN_REQUEST_DONE_TIMEOUT_SEC 2

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
enum PASN_STATUS {
	PASN_STATUS_SUCCESS = 0,
	PASN_STATUS_FAILURE = 1,
};

struct PASN_PEER {
	uint8_t aucOwnAddr[MAC_ADDR_LEN];
	uint8_t aucPeerAddr[MAC_ADDR_LEN];
	int32_t i4NetworkId;
	int32_t i4Akmp;
	int32_t i4Cipher;
	int32_t i4Group;
	uint8_t ucLtfKeyseedRequired;
	enum PASN_STATUS eStatus;
};

enum PASN_ACTION {
	PASN_ACTION_AUTH,
	PASN_ACTION_DELETE_SECURE_RANGING_CONTEXT,
};

struct PASN_AUTH {
	enum PASN_ACTION eAction;
	uint8_t ucNumPeers;
	/* aucOwnAddr, aucPeerAddr, ucLtfKeyseedRequired are mandatory */
	struct PASN_PEER arPeer[PASN_MAX_PEERS];
};

/* Note that only aucOwnAddr, aucPeerAddr, eStatus will be updated */
struct PASN_RESP_EVENT {
	uint8_t ucNumPeers;
	struct PASN_PEER arPeer[PASN_MAX_PEERS];
};

struct PASN_SECURE_RANGING_CTX {
	uint32_t u4Action; /* QCA_WLAN_VENDOR_SECURE_RANGING_CTX_ACTION */
	uint8_t aucOwnAddr[MAC_ADDR_LEN];
	uint8_t aucPeerAddr[MAC_ADDR_LEN];
	uint32_t u4Cipher; /* WPA_CIPHER_* */
	uint32_t u4ShaType; /* QCA_WLAN_VENDOR_SHA_TYPE */
	uint8_t ucTkLen;
	uint8_t aucTk[32]; /* WPA_TK_MAX_LEN */;
	uint8_t ucLtfKeyseedLen;
	uint8_t aucLtfKeyseed[48]; /* WPA_LTF_KEYSEED_MAX_LEN */
};

struct MSG_PASN_RESP {
	struct MSG_HDR rMsgHdr;
	struct PASN_RESP_EVENT rPasnRespEvt;
};

struct MSG_PASN_SECURE_RANGING_CTX {
	struct MSG_HDR rMsgHdr;
	struct PASN_SECURE_RANGING_CTX rRangingCtx;
};

typedef void (*PFN_PASN_DONE_FUNC) (struct ADAPTER *prAdapter,
		enum PASN_STATUS ePasnStatus,
		struct PASN_RESP_EVENT *prPasnRespEvt,
		void *pvUserData);

typedef void (*PFN_SECURE_RANGING_CTX_FUNC) (struct ADAPTER *prAdapter,
		struct PASN_SECURE_RANGING_CTX *prRangingCtx,
		void *pvUserData);


/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct PASN_INFO {
	uint8_t fgIsRunning;
	struct TIMER rPasnDoneTimer;
	PFN_PASN_DONE_FUNC pfPasnDoneCB;
	PFN_SECURE_RANGING_CTX_FUNC pfRangingCtxCB;
	void *pvUserData;
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
extern uint8_t kalIndicatePasnEvent(struct ADAPTER *prAdapter,
		void *pvPasnReq,
		uint8_t ucBssIdx);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void pasnInit(struct ADAPTER *prAdapter);

void pasnUninit(struct ADAPTER *prAdapter);

uint8_t pasnIsRunning(struct ADAPTER *prAdapter);

uint32_t pasnHandlePasnRequest(struct ADAPTER *prAdapter,
	uint8_t ucBssIdx,
	struct PASN_AUTH *prPasnReq,
	PFN_PASN_DONE_FUNC pfPasnDoneCB,
	PFN_SECURE_RANGING_CTX_FUNC pfRangingCtxCB,
	void *pvUserData);

void pasnHandlePasnResp(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

void pasnHandleSecureRangingCtx(struct ADAPTER *prAdapter,
	struct MSG_HDR *prMsgHdr);

#endif /* CFG_SUPPORT_PASN */
#endif /* _PASH_H */
