/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _ARP_MON_H
#define _ARP_MON_H
#if ARP_MONITER_ENABLE

#define NEIGH_STATE_NUM	9

enum ENUM_NUD_STATE {
	ENUM_NUD_INCOMPLETE,
	ENUM_NUD_REACHABLE,
	ENUM_NUD_STALE,
	ENUM_NUD_DELAY,
	ENUM_NUD_PROBE,
	ENUM_NUD_FAILED,
	ENUM_NUD_NOARP,
	ENUM_NUD_PERMANENT,
	ENUM_NUD_NONE,
	ENUM_NUD_STATE_NUM
};

static const char * const apucNudStateStr[ENUM_NUD_STATE_NUM] = {
	[ENUM_NUD_INCOMPLETE] = "incomplete",
	[ENUM_NUD_REACHABLE] = "reachable",
	[ENUM_NUD_STALE] = "stale",
	[ENUM_NUD_DELAY] = "delay",
	[ENUM_NUD_PROBE] = "probe",
	[ENUM_NUD_FAILED] = "failed",
	[ENUM_NUD_NOARP] = "noarp",
	[ENUM_NUD_PERMANENT] = "permanent",
	[ENUM_NUD_NONE] = "none"
};

struct ARP_MON {
	/* ARP Req Tx Cnt (Not yet Recv ARP Rsp) */
	uint16_t arpMoniter;
	uint8_t apIp[IPV4_ADDR_LEN];
	uint8_t gatewayIp[IPV4_ADDR_LEN];
	uint8_t gatewayMac[MAC_ADDR_LEN];
	uint32_t u4TxCnt;
	uint32_t u4GatewayRxCnt;
	uint32_t LastRxCnt;
	uint32_t CurrentRxCnt;
	uint32_t LastRxUnicastTime;
	uint32_t CurrentRxUnicastTime;
	uint8_t arpIsCriticalThres;
	enum ENUM_NUD_STATE eNudState;
};

struct ARP_MON_PKT_INFO {
	uint8_t ucBssIdx;
	uint16_t u2PacketLen;
	uint8_t aucTaAddr[MAC_ADDR_LEN];
	uint8_t *pucData;
};

enum ENUM_ARP_MON_TYPE {
	ARP_MON_TYPE_TX_ARP = 0,
	ARP_MON_TYPE_RX_ARP,
	ARP_MON_TYPE_RX_DHCP,
	ARP_MON_TYPE_NUD_BTO,
	ARP_MON_TYPE_MAX
};

struct MSG_ARP_MON {
	struct MSG_HDR rMsgHdr; /* Must be the first member */
	enum ENUM_ARP_MON_TYPE eType;
	struct ARP_MON_PKT_INFO rArpMonPktInfo;
	uint8_t arData[ETHER_MAX_PKT_SZ];
};

typedef void(*ARP_MON_HANDLE_FUNCTION) (struct ADAPTER *,
	struct ARP_MON_PKT_INFO *);

struct ARP_MON_HANDLER {
	enum ENUM_ARP_MON_TYPE eType;
	ARP_MON_HANDLE_FUNCTION pfnHandler;
};

u_int8_t arpMonIpIsCritical(struct ADAPTER *ad, struct MSDU_INFO *prMsduInfo);
u_int8_t arpMonIsCritical(struct ADAPTER *ad, uint8_t ucBssIdx);
void arpMonResetArpDetect(struct ADAPTER *ad, uint8_t ucBssIdx);
void arpMonHandleLegacyBTOEvent(struct ADAPTER *ad);
void arpMonHandleNudState(struct ADAPTER *prAdapter, uint8_t state,
	uint8_t ucBssIndex);

#if CFG_QM_ARP_MONITOR_MSG
void arpMonHandleMsg(struct ADAPTER *ad, struct MSG_HDR *prMsgHdr);
#endif /* CFG_QM_ARP_MONITOR_MSG */
void arpMonProcessRxPacket(struct ADAPTER *ad, struct BSS_INFO *prBssInfo,
	struct SW_RFB *prSwRfb, OS_SYSTIME rCurrentTime);
void arpMonProcessTxPacket(struct ADAPTER *ad, struct MSDU_INFO *prMsduInfo);
#else /* ARP_MONITER_ENABLE */

/* No valid IP address information, always returns TRUE */
static inline u_int8_t arpMonIpIsCritical(struct ADAPTER *ad,
					  struct MSDU_INFO *prMsduInfo)
{ return TRUE; }
#endif /* ARP_MONITER_ENABLE */
#endif /* _ARP_MON_H */
