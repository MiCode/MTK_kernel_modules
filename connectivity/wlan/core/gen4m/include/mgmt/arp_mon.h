/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _ARP_MON_H
#define _ARP_MON_H
#if ARP_MONITER_ENABLE
struct ARP_MON {
	/* ARP Req Tx Cnt (Not yet Recv ARP Rsp) */
	uint16_t arpMoniter;
	uint8_t apIp[IPV4_ADDR_LEN];
	uint8_t gatewayIp[IPV4_ADDR_LEN];
	uint8_t gatewayMac[MAC_ADDR_LEN];
	uint32_t LastRxCnt;
	uint32_t CurrentRxCnt;
	uint32_t LastRxUnicastTime;
	uint32_t CurrentRxUnicastTime;
	uint8_t arpIsCriticalThres;
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

#if CFG_QM_ARP_MONITOR_MSG
void arpMonHandleMsg(struct ADAPTER *ad, struct MSG_HDR *prMsgHdr);
#endif /* CFG_QM_ARP_MONITOR_MSG */
void arpMonProcessRxPacket(struct ADAPTER *ad, struct BSS_INFO *prBssInfo,
	struct SW_RFB *prSwRfb);
void arpMonProcessTxPacket(struct ADAPTER *ad, struct MSDU_INFO *prMsduInfo);
#else /* ARP_MONITER_ENABLE */

/* No valid IP address information, always returns TRUE */
static inline u_int8_t arpMonIpIsCritical(struct ADAPTER *ad,
					  struct MSDU_INFO *prMsduInfo)
{ return TRUE; }
#endif /* ARP_MONITER_ENABLE */
#endif /* _ARP_MON_H */
