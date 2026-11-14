// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"

#if ARP_MONITER_ENABLE
static uint16_t arpMonGetGatewayTxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].arpMoniter;
}

static void arpMonResetGatewayTxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	ad->arArpMonitor[ucBssIdx].arpMoniter = 0;
}

static void arpMonIncGatewayTxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	ad->arArpMonitor[ucBssIdx].arpMoniter++;
}

static uint16_t arpMonGetTxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return GLUE_GET_REF_CNT(ad->arArpMonitor[ucBssIdx].u4TxCnt);
}

static void arpMonResetTxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	GLUE_SET_REF_CNT(0, ad->arArpMonitor[ucBssIdx].u4TxCnt);
}

static void arpMonIncTxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	GLUE_INC_REF_CNT(ad->arArpMonitor[ucBssIdx].u4TxCnt);
}

static uint64_t arpMonGetNudState(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].eNudState;
}

static void arpMonSetNudState(struct ADAPTER *ad, uint8_t ucBssIdx,
	enum ENUM_NUD_STATE eNudState)
{
	ad->arArpMonitor[ucBssIdx].eNudState = eNudState;
}

static uint16_t arpMonGetGatewayRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].u4GatewayRxCnt;
}

static void arpMonResetGatewayRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	ad->arArpMonitor[ucBssIdx].u4GatewayRxCnt = 0;
}

static void arpMonIncGatewayRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	ad->arArpMonitor[ucBssIdx].u4GatewayRxCnt++;
}

static uint8_t arpMonGetCriticalThres(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].arpIsCriticalThres;
}

static void arpMonSetCriticalThres(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint8_t ucThreshold)
{
	ad->arArpMonitor[ucBssIdx].arpIsCriticalThres = ucThreshold;
}

static uint32_t arpMonGetLastRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].LastRxCnt;
}

static void arpMonSetLastRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint32_t LastRxCnt)
{
	ad->arArpMonitor[ucBssIdx].LastRxCnt = LastRxCnt;
}

static uint32_t arpMonGetCurrentRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].CurrentRxCnt;
}

static void arpMonSetCurrentRxCnt(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint32_t CurrentRxCnt)
{
	ad->arArpMonitor[ucBssIdx].CurrentRxCnt = CurrentRxCnt;
}

static uint32_t arpMonGetRxDiff(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return (arpMonGetCurrentRxCnt(ad, ucBssIdx)
		- arpMonGetLastRxCnt(ad, ucBssIdx));
}

static uint32_t arpMonGetLastRxUnicastTime(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].LastRxUnicastTime;
}

static void arpMonSetLastRxUnicastTime(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint32_t LastRxUnicastTime)
{
	ad->arArpMonitor[ucBssIdx].LastRxUnicastTime =
		LastRxUnicastTime;
}

static uint32_t arpMonGetCurrentRxUnicastTime(struct ADAPTER *ad,
	uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].CurrentRxUnicastTime;
}

static void arpMonSetCurrentRxUnicastTime(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint32_t CurrentRxUnicastTime)
{
	ad->arArpMonitor[ucBssIdx].CurrentRxUnicastTime =
		CurrentRxUnicastTime;
}

static uint32_t arpMonGetRxUnicastTimeDiff(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return (arpMonGetCurrentRxUnicastTime(ad, ucBssIdx)
		- arpMonGetLastRxUnicastTime(ad, ucBssIdx));
}

static void arpMonSetApIp(struct ADAPTER *ad, uint8_t ucBssIdx, uint8_t *apIp)
{
	COPY_IP_ADDR(ad->arArpMonitor[ucBssIdx].apIp, apIp);
}

static uint32_t arpMonNotApIpAndGatewayIp(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint8_t *Ip)
{
	return (kalMemCmp(ad->arArpMonitor[ucBssIdx].apIp,
		Ip, sizeof(ad->arArpMonitor[ucBssIdx].apIp))
		&& kalMemCmp(ad->arArpMonitor[ucBssIdx].gatewayIp,
		Ip, sizeof(ad->arArpMonitor[ucBssIdx].gatewayIp)));
}

static uint8_t *arpMonGetApIpPtr(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].apIp;
}

static void arpMonSetGatewayIp(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint8_t *gatewayIp)
{
	COPY_IP_ADDR(ad->arArpMonitor[ucBssIdx].gatewayIp, gatewayIp);
}

static uint8_t *arpMonGetGatewayIpPtr(struct ADAPTER *ad,
	uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].gatewayIp;
}

static void arpMonSetGatewayMac(struct ADAPTER *ad, uint8_t ucBssIdx,
	uint8_t *gatewayMac)
{
	COPY_MAC_ADDR(ad->arArpMonitor[ucBssIdx].gatewayMac, gatewayMac);
}

static uint32_t arpMonEqualGatewayMac(struct ADAPTER *ad,
	uint8_t ucBssIdx, uint8_t *gatewayMac)
{
	return EQUAL_MAC_ADDR(ad->arArpMonitor[ucBssIdx].gatewayMac,
		gatewayMac);
}

static uint8_t *arpMonGetGatewayMacPtr(struct ADAPTER *ad,
	uint8_t ucBssIdx)
{
	return ad->arArpMonitor[ucBssIdx].gatewayMac;
}

static void arpMonReset(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	arpMonResetGatewayTxCnt(ad, ucBssIdx);
	arpMonResetGatewayRxCnt(ad, ucBssIdx);
	arpMonResetTxCnt(ad, ucBssIdx);
	arpMonSetLastRxCnt(ad, ucBssIdx, 0);
	arpMonSetCurrentRxCnt(ad, ucBssIdx, 0);
	arpMonSetLastRxUnicastTime(ad, ucBssIdx, 0);
	arpMonSetCurrentRxUnicastTime(ad, ucBssIdx, 0);
	kalMemZero(ad->arArpMonitor[ucBssIdx].apIp,
		sizeof(ad->arArpMonitor[ucBssIdx].apIp));
}

static void arpMonResetGateway(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	kalMemZero(ad->arArpMonitor[ucBssIdx].gatewayIp,
		sizeof(ad->arArpMonitor[ucBssIdx].gatewayIp));
	kalMemZero(ad->arArpMonitor[ucBssIdx].gatewayMac,
		sizeof(ad->arArpMonitor[ucBssIdx].gatewayMac));
}

static void getSrcMac(uint8_t *pucData, uint16_t u2PacketLen,
	uint8_t *prMacAddr)
{
	uint8_t *pucSaAddr = NULL;

	if (u2PacketLen < ETHER_HEADER_LEN)
		return;

	pucSaAddr = pucData + MAC_ADDR_LEN;
	COPY_MAC_ADDR(prMacAddr, pucSaAddr);
}

static struct ARP_HEADER *getArpPkt(uint8_t *pucData, uint16_t u2PacketLen)
{
	struct ETH_FRAME *prEth = (struct ETH_FRAME *)pucData;
	struct ARP_HEADER *prArp = NULL;

	if (u2PacketLen > ETHER_MAX_PKT_SZ ||
	    u2PacketLen < sizeof(struct ETH_FRAME) + sizeof(struct ARP_HEADER))
		goto end;

	if (unlikely(NTOHS(prEth->u2TypeLen) != ETH_P_ARP)) {
		DBGLOG(TX, ERROR, "wrong ARP type %04x\n",
		       NTOHS(prEth->u2TypeLen));
		goto end;
	}

	prArp = (struct ARP_HEADER *)prEth->aucData;

end:
	return prArp;
}

static u_int8_t arpMonIsIOTIssue(struct ADAPTER *ad, uint32_t ucBssIdx)
{
#if !CFG_EXT_FEATURE
	struct WIFI_VAR *prWifiVar = NULL;
	uint8_t ucArpMonitorUseRule;
	uint32_t uArpMonitorRxPktNum;
#endif

#if CFG_EXT_FEATURE
	return FALSE;
#else

	prWifiVar = &ad->rWifiVar;
	ucArpMonitorUseRule = prWifiVar->ucArpMonitorUseRule;
	uArpMonitorRxPktNum = prWifiVar->uArpMonitorRxPktNum;

	if (ucArpMonitorUseRule == 0) {
		/* use rx packet for IOT checking */
		/* rx cnt less than N after tx arp */
		return (arpMonGetRxDiff(ad, ucBssIdx)
			<= uArpMonitorRxPktNum);
	} else {
		/* use unicast time for IOT checking */
		/* no unicast rx after tx arp */
		return (arpMonGetRxUnicastTimeDiff(ad, ucBssIdx) == 0);
	}
#endif
}

/* Should call inside main_thread */
static void arpMonSetBTOEvent(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(AM, WARN, "Invalid BssIndex %u\n", ucBssIdx);
		return;
	}

#if CFG_SUPPORT_DATA_STALL
	KAL_REPORT_ERROR_EVENT(ad, EVENT_ARP_NO_RESPONSE,
		(uint16_t)sizeof(uint32_t), ucBssIdx, FALSE);
#endif /* CFG_SUPPORT_DATA_STALL */

	aisBssBeaconTimeout(ad, ucBssIdx);
}

#if !CFG_QM_ARP_MONITOR_MSG
static void arpMonSetLegacyBTOEvent(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	if (ucBssIdx >= MAX_BSSID_NUM)
		return;

	ad->ucArpNoRespBitmap |= BIT(ucBssIdx);
}

void arpMonHandleLegacyBTOEvent(struct ADAPTER *ad)
{
	uint8_t i;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (!ad->ucArpNoRespBitmap)
			break;

		if (ad->ucArpNoRespBitmap & BIT(i)) {
			arpMonSetBTOEvent(ad, i);
			ad->ucArpNoRespBitmap &= ~BIT(i);
		}
	}
}
#endif /* !CFG_QM_ARP_MONITOR_MSG */

void arpMonHandleTxArpPkt(struct ADAPTER *ad,
	struct ARP_MON_PKT_INFO *prArpMonPktInfo)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	void *pvDevHandler = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	struct RX_CTRL	*prRxCtrl = NULL;
	struct BSS_INFO *prAisBssInfo = NULL;
	struct ARP_HEADER *prArp = NULL;
	uint8_t ucBssIdx = prArpMonPktInfo->ucBssIdx;
	uint16_t u2PacketLen = prArpMonPktInfo->u2PacketLen;
	uint8_t *pucData = prArpMonPktInfo->pucData;

	prGlueInfo = ad->prGlueInfo;
	if (!prGlueInfo)
		return;

	prRxCtrl = &ad->rRxCtrl;
	prWifiVar = &ad->rWifiVar;
	if (prWifiVar->uArpMonitorNumber == 0)
		return;

	pvDevHandler = kalGetGlueNetDevHdl(prGlueInfo);
	if (!pvDevHandler) {
		DBGLOG(AM, WARN, "pvDevHandler NULL\n");
		return;
	}

	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(AM, WARN, "Invalid BssIndex %u\n", ucBssIdx);
		return;
	}

	prAisBssInfo = aisGetAisBssInfo(ad, ucBssIdx);
	if (!prAisBssInfo)
		return;

	prArp = getArpPkt(pucData, u2PacketLen);
	if (!prArp)
		return;

	if (NTOHS(prArp->u2OpCode) != ARP_OPERATION_REQUEST)
		return;

	/* If ARP req is neither to apIp nor to gatewayIp, ignore detection */
	if (arpMonNotApIpAndGatewayIp(ad, ucBssIdx, prArp->aucTargetIPaddr))
		return;

	arpMonIncGatewayTxCnt(ad, ucBssIdx);

	if (prWifiVar->ucArpMonitorUseRule == 0) {
		/* Legacy Rule */
		/* Record counts of RX Packets when Tx 1st ARP Req */
		if (!arpMonGetLastRxCnt(ad, ucBssIdx)) {
			arpMonSetLastRxCnt(ad, ucBssIdx,
				kalGetNetDevRxPacket(pvDevHandler));
			arpMonSetCurrentRxCnt(ad, ucBssIdx, 0);
		}

		/* Record counts of RX Packets when TX ARP Req recently */
		arpMonSetCurrentRxCnt(ad, ucBssIdx,
			kalGetNetDevRxPacket(pvDevHandler));
	} else {
		/* New Rule */
		/* Record the time that rx unicast when Tx 1st ARP Req */
		if (!arpMonGetLastRxUnicastTime(ad, ucBssIdx)) {
			arpMonSetLastRxUnicastTime(ad, ucBssIdx,
				prRxCtrl->rLastUnicastRxTime[ucBssIdx]);
			arpMonSetCurrentRxUnicastTime(ad, ucBssIdx, 0);
		}

		/* Record the time that rx unicast when TX ARP Req recently */
		arpMonSetCurrentRxUnicastTime(ad, ucBssIdx,
			prRxCtrl->rLastUnicastRxTime[ucBssIdx]);
	}

	if (arpMonGetGatewayTxCnt(ad, ucBssIdx) >
		prWifiVar->uArpMonitorNumber) {
		if (arpMonIsIOTIssue(ad, ucBssIdx)) {
			DBGLOG(AM, WARN, "IOT issue, arp no resp!\n");
			if (prAisBssInfo)
				prAisBssInfo->u2DeauthReason =
				REASON_CODE_ARP_NO_RESPONSE;
#if CFG_QM_ARP_MONITOR_MSG
			arpMonSetBTOEvent(ad, ucBssIdx);
#else /* CFG_QM_ARP_MONITOR_MSG */
			arpMonSetLegacyBTOEvent(ad, ucBssIdx);
#endif /* CFG_QM_ARP_MONITOR_MSG */
		} else {
			if (prWifiVar->ucArpMonitorUseRule == 0)
				DBGLOG(AM, WARN, "ARP, still have %d pkts\n",
					arpMonGetRxDiff(ad, ucBssIdx));
			else
				DBGLOG(AM, WARN, "ARP, Rx UC time diff %u\n",
					arpMonGetRxUnicastTimeDiff(ad,
						ucBssIdx));
		}

		arpMonReset(ad, ucBssIdx);
	}

	if (prWifiVar->ucArpMonitorUseRule == 0) {
		DBGLOG(AM, LOUD,
			"cfg[%u:%u:%u] tx[%u] rx_cnt[%u:%u]\n",
			prWifiVar->uArpMonitorNumber,
			prWifiVar->ucArpMonitorUseRule,
			prWifiVar->uArpMonitorRxPktNum,
			arpMonGetGatewayTxCnt(ad, ucBssIdx),
			arpMonGetCurrentRxCnt(ad, ucBssIdx),
			arpMonGetLastRxCnt(ad, ucBssIdx));
	} else {
		DBGLOG(AM, LOUD,
			"cfg[%u:%u:%u] tx[%u] rx_unicast_time[%u:%u]\n",
			prWifiVar->uArpMonitorNumber,
			prWifiVar->ucArpMonitorUseRule,
			prWifiVar->uArpMonitorRxPktNum,
			arpMonGetGatewayTxCnt(ad, ucBssIdx),
			arpMonGetCurrentRxUnicastTime(ad, ucBssIdx),
			arpMonGetLastRxUnicastTime(ad, ucBssIdx));
	}
}

void arpMonHandleRxArpPkt(struct ADAPTER *ad,
	struct ARP_MON_PKT_INFO *prArpMonPktInfo)
{
	struct BSS_INFO *prAisBssInfo = NULL;
	struct ARP_HEADER *prArp = NULL;
	uint8_t ucBssIdx = prArpMonPktInfo->ucBssIdx;
	uint16_t u2PacketLen = prArpMonPktInfo->u2PacketLen;
	uint8_t *pucData = prArpMonPktInfo->pucData;
	u_int8_t fgIsFromApIpOrGatewayIp;

	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(AM, WARN, "Invalid BssIndex %u\n", ucBssIdx);
		return;
	}

	prAisBssInfo = aisGetAisBssInfo(ad, ucBssIdx);
	if (!prAisBssInfo)
		return;

	prArp = getArpPkt(pucData, u2PacketLen);
	if (!prArp)
		return;

	if (NTOHS(prArp->u2OpCode) != ARP_OPERATION_RESPONSE)
		return;
	fgIsFromApIpOrGatewayIp = !arpMonNotApIpAndGatewayIp(ad, ucBssIdx,
					prArp->aucSenderIPaddr);

	DBGLOG(AM, LOUD,
		"ArpSrcMac:" MACSTR " ArpSrcIp:" IPV4STR " ArpTaMac:" MACSTR
		" isFromAp/gatewayIP [%d]\n",
		MAC2STR(prArp->aucSenderMACaddr),
		IPV4TOSTR(prArp->aucSenderIPaddr),
		MAC2STR(prArpMonPktInfo->aucTaAddr),
		fgIsFromApIpOrGatewayIp);

	if (prAisBssInfo && prAisBssInfo->prStaRecOfAP) {
		if (EQUAL_MAC_ADDR(prArp->aucSenderMACaddr,
			cnmStaRecAuthAddr(ad, prAisBssInfo->prStaRecOfAP))) {
			arpMonResetTxCnt(ad, ucBssIdx);
			arpMonResetGatewayTxCnt(ad, ucBssIdx);
			arpMonSetApIp(ad, ucBssIdx, prArp->aucSenderIPaddr);
			DBGLOG(AM, TRACE,
				"get arp response from AP " IPV4STR "(SA:"
				MACSTR ")\n",
				IPV4TOSTR(arpMonGetApIpPtr(ad, ucBssIdx)),
				MAC2STR(prArp->aucSenderMACaddr));
		} else if (EQUAL_MAC_ADDR((prArpMonPktInfo->aucTaAddr),
			cnmStaRecAuthAddr(ad, prAisBssInfo->prStaRecOfAP)) &&
			fgIsFromApIpOrGatewayIp) {
			arpMonResetTxCnt(ad, ucBssIdx);
			arpMonResetGatewayTxCnt(ad, ucBssIdx);
			DBGLOG(AM, TRACE,
				"get arp response from AP " IPV4STR "(TA:"
				MACSTR ")\n",
				IPV4TOSTR(prArp->aucSenderIPaddr),
				MAC2STR(prArpMonPktInfo->aucTaAddr));
		}

	}
}

void arpMonHandleRxDhcpPkt(struct ADAPTER *ad,
	struct ARP_MON_PKT_INFO *prArpMonPktInfo)
{
	uint8_t ucBssIdx = prArpMonPktInfo->ucBssIdx;
	uint16_t u2PacketLen = prArpMonPktInfo->u2PacketLen;
	uint8_t *pucData = prArpMonPktInfo->pucData;
	uint8_t rSrcMacAddr[MAC_ADDR_LEN];
	struct DHCP_PROTOCOL *prDhcp;
	uint16_t dhcpLen = 0;
	uint8_t dhcpTypeGot = 0;
	uint8_t dhcpGatewayGot = 0;
	uint32_t i = 0;
	const uint16_t MAX_DHCP_OPT_LEN = ETHER_MAX_PKT_SZ - ETHER_HEADER_LEN -
		IP_HEADER_LEN - UDP_HDR_LEN - sizeof(struct DHCP_PROTOCOL);

	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(AM, WARN, "Invalid BssIndex %u\n", ucBssIdx);
		return;
	}

	/* check if pkt is dhcp from server */
	prDhcp = qmGetDhcpPkt(pucData, u2PacketLen, TRUE, &dhcpLen);
	if (!prDhcp)
		return;

	if (unlikely(dhcpLen > MAX_DHCP_OPT_LEN))
		dhcpLen = MAX_DHCP_OPT_LEN;
	DBGLOG(AM, LOUD, "BssIdx:%u dhcpLen:%u\n", ucBssIdx, dhcpLen);

	/* start from the beginning of dhcp option */
	while (sizeof(struct DHCP_PROTOCOL) + i < dhcpLen) {
		/* Because DHCP is a variant of DHCP identified by the
		 * MAGIC COOKIE at the beginning of option field in
		 * struct DHCP_PROTOCOL,
		 * we define the fixed MAGIC COOKIE outside option field in
		 * DHCP_PROTOCOL to focus on the real DHCP options.
		 */
		switch (prDhcp->aucDhcpOption[i]) {
		case DHCP_OPTION_ROUTER:
			/*  Code  Len      Address 1           Address 2
			 * +----+----+----+----+----+----+----+----+----+----+
			 * |  3 |  n | a1 | a2 | a3 | a4 | a1 | a2 | a3 | a4 |
			 * +----+----+----+----+----+----+----+----+----+----+
			 */
			/* both dhcp ack and offer will update it */
			if (IS_NONZERO_IP_ADDR(&prDhcp->aucDhcpOption[i + 2])) {
				arpMonSetGatewayIp(ad, ucBssIdx,
					&prDhcp->aucDhcpOption[i + 2]);

				DBGLOG(AM, TRACE, "Gateway ip: " IPV4STR "\n",
					IPV4TOSTR(arpMonGetGatewayIpPtr(
							ad, ucBssIdx)));
			};
			dhcpGatewayGot = 1;

			/* Record the MAC address of gateway */
			getSrcMac(pucData, u2PacketLen, rSrcMacAddr);
			arpMonSetGatewayMac(ad, ucBssIdx, rSrcMacAddr);
			break;

		case DHCP_OPTION_MESSAGE_TYPE:
			/*  Code  Len
			 * +----+----+----+
			 * | 53 |  1 | 1-8|
			 * +----+----+----+
			 */
			if (prDhcp->aucDhcpOption[2 + i] != DHCP_OFFER &&
			    prDhcp->aucDhcpOption[2 + i] != DHCP_ACK) {
				DBGLOG(AM, WARN,
					"wrong dhcp message type, type: %d\n",
					prDhcp->aucDhcpOption[i + 6]);
				if (dhcpGatewayGot)
					arpMonResetGateway(ad, ucBssIdx);

				return;
			} else if (prDhcp->aucDhcpOption[2 + i] == DHCP_ACK) {
				/* Check if join timer is ticking, then release
				 * channel privilege and stop join timer.
				 */
				qmReleaseCHAtFinishedDhcp(ad, ucBssIdx);
			}
			dhcpTypeGot = 1;
			break;

		case DHCP_OPTION_PAD:
			i++;
			continue;

		case DHCP_OPTION_END:
			return;

		default:
			break;
		}
		if (dhcpGatewayGot && dhcpTypeGot)
			return;

		/* [1 + i] points to Len field; +2 for Code & Len field  */
		i += prDhcp->aucDhcpOption[1 + i] + 2;
	}

	DBGLOG(AM, WARN,
	       "can't find the dhcp option 255?, need to check the net log\n");
}

void arpMonHandleNudBTO(struct ADAPTER *prAdapter,
	struct ARP_MON_PKT_INFO *prArpMonPktInfo)
{
#if CFG_QM_ARP_MONITOR_MSG
	arpMonSetBTOEvent(prAdapter, prArpMonPktInfo->ucBssIdx);
#else /* CFG_QM_ARP_MONITOR_MSG */
	arpMonSetLegacyBTOEvent(prAdapter, prArpMonPktInfo->ucBssIdx);
#endif /* CFG_QM_ARP_MONITOR_MSG */
}

const struct ARP_MON_HANDLER arArpMonHandler[ARP_MON_TYPE_MAX] = {
	{ARP_MON_TYPE_TX_ARP, arpMonHandleTxArpPkt},
	{ARP_MON_TYPE_RX_ARP, arpMonHandleRxArpPkt},
	{ARP_MON_TYPE_RX_DHCP, arpMonHandleRxDhcpPkt},
	{ARP_MON_TYPE_NUD_BTO, arpMonHandleNudBTO},
};

void arpMonHandlePkt(struct ADAPTER *ad, enum ENUM_ARP_MON_TYPE eType,
	struct ARP_MON_PKT_INFO *prArpMonPktInfo)
{
	uint32_t u4Idx;

	for (u4Idx = 0; u4Idx < ARP_MON_TYPE_MAX; u4Idx++) {
		if (eType == arArpMonHandler[u4Idx].eType) {
			arArpMonHandler[u4Idx].pfnHandler(ad, prArpMonPktInfo);
			break;
		}
	}
}

#if CFG_QM_ARP_MONITOR_MSG
static void arpMonSendMsg(struct ADAPTER *ad, enum ENUM_ARP_MON_TYPE eType,
	struct ARP_MON_PKT_INFO *prArpMonPktInfo)
{
	uint8_t *pucData = prArpMonPktInfo->pucData;
	uint16_t u2PacketLen = prArpMonPktInfo->u2PacketLen;
	struct MSG_ARP_MON *prArpMonitorMsg;

	if (u2PacketLen >= ETHER_MAX_PKT_SZ) {
		DBGLOG(AM, WARN, "Invalid Pkt size %u\n", u2PacketLen);
		return;
	}

	prArpMonitorMsg = (struct MSG_ARP_MON *) cnmMemAlloc(ad,
		RAM_TYPE_MSG, sizeof(struct MSG_ARP_MON));
	if (!prArpMonitorMsg) {
		DBGLOG(AM, WARN, "cnmMemAlloc Fail\n");
		return;
	}

	prArpMonitorMsg->rMsgHdr.eMsgId = MID_QM_ARP_MONITOR;
	prArpMonitorMsg->eType = eType;
	kalMemCopy(prArpMonitorMsg->arData, pucData, u2PacketLen);
	/* Set pointer to data buffer holding copied data */
	prArpMonPktInfo->pucData = prArpMonitorMsg->arData;
	prArpMonitorMsg->rArpMonPktInfo = *prArpMonPktInfo;

	DBGLOG(AM, LOUD,
		"Send Msg eMsgId:%u eType:%u ucBssIdx:%u u2PacketLen:%u\n",
		prArpMonitorMsg->rMsgHdr.eMsgId,
		prArpMonitorMsg->eType,
		prArpMonitorMsg->rArpMonPktInfo.ucBssIdx,
		prArpMonitorMsg->rArpMonPktInfo.u2PacketLen);

	mboxSendMsg(ad, MBOX_ID_0,
		(struct MSG_HDR *) prArpMonitorMsg, MSG_SEND_METHOD_BUF);
}

void arpMonHandleMsg(struct ADAPTER *ad, struct MSG_HDR *prMsgHdr)
{
	struct MSG_ARP_MON *prArpMonitorMsg;

	prArpMonitorMsg = (struct MSG_ARP_MON *)prMsgHdr;

	DBGLOG(AM, LOUD,
		"Handle Msg eMsgId:%u eType:%u ucBssIdx:%u u2PacketLen:%u\n",
		prArpMonitorMsg->rMsgHdr.eMsgId,
		prArpMonitorMsg->eType,
		prArpMonitorMsg->rArpMonPktInfo.ucBssIdx,
		prArpMonitorMsg->rArpMonPktInfo.u2PacketLen);

	arpMonHandlePkt(ad, prArpMonitorMsg->eType,
				&prArpMonitorMsg->rArpMonPktInfo);

	cnmMemFree(ad, prMsgHdr);
}
#endif /* CFG_QM_ARP_MONITOR_MSG */

static void arpMonDetectNoResponse(struct ADAPTER *ad,
	struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIdx;
	struct BSS_INFO *prBssInfo;
	uint8_t *pucData = NULL;
	struct ARP_HEADER *prArp = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	struct ARP_MON_PKT_INFO rArpMonPktInfo = {0};

	if (!ad ||
		!ad->prGlueInfo) {
		DBGLOG(AM, WARN, "Param is invalid\n");
		return;
	}

	prWifiVar = &ad->rWifiVar;
	if (prWifiVar->uArpMonitorNumber == 0)
		return;

	/* We need to disable arp monitor in CTIA mode */
	if (ad->fgDisBcnLostDetection == TRUE)
		return;

	/* skip forwarding pkt */
	if (prMsduInfo->eSrc != TX_PACKET_OS)
		return;

	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(
		ad, prMsduInfo->ucStaRecIndex);
	if (!prStaRec)
		return;

	/* store it in local variable to prevent timing issue */
	ucBssIdx = prStaRec->ucBssIndex;
	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(AM, WARN, "Invalid BssIndex %u\n", ucBssIdx);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(ad, ucBssIdx);
	if (!prBssInfo)
		return;

	if (!IS_BSS_INFO_IN_AIS(prBssInfo))
		return;

	/* save CriticalThres */
	arpMonSetCriticalThres(ad, ucBssIdx,
		prWifiVar->uArpMonitorCriticalThres);

	if (prMsduInfo->eSrc != TX_PACKET_OS)
		return;

	if (!(prMsduInfo->prPacket)
		|| (kalQueryPacketLength(prMsduInfo->prPacket)
				<= ETHER_HEADER_LEN))
		return;

	kalGetPacketBuf(prMsduInfo->prPacket, &pucData);

	if (!pucData)
		return;

	prArp = getArpPkt(pucData, kalQueryPacketLength(prMsduInfo->prPacket));
	if (!prArp)
		return;

	if (NTOHS(prArp->u2OpCode) != ARP_OPERATION_REQUEST)
		return;

	DBGLOG(AM, LOUD,
		"apIp:" IPV4STR " gatewayIp:" IPV4STR " TarIp:" IPV4STR "\n",
		IPV4TOSTR(arpMonGetApIpPtr(ad, ucBssIdx)),
		IPV4TOSTR(arpMonGetGatewayIpPtr(ad, ucBssIdx)),
		IPV4TOSTR(prArp->aucTargetIPaddr));

	arpMonIncTxCnt(ad, ucBssIdx);

	/* If ARP req is neither to apIp nor to gatewayIp, ignore detection */
	if (arpMonNotApIpAndGatewayIp(ad, ucBssIdx, prArp->aucTargetIPaddr))
		return;

	rArpMonPktInfo.ucBssIdx = ucBssIdx;
	rArpMonPktInfo.u2PacketLen = kalQueryPacketLength(prMsduInfo->prPacket);
	rArpMonPktInfo.pucData = pucData;

#if CFG_QM_ARP_MONITOR_MSG
	arpMonSendMsg(ad, ARP_MON_TYPE_TX_ARP, &rArpMonPktInfo);
#else /* CFG_QM_ARP_MONITOR_MSG */
	arpMonHandlePkt(ad, ARP_MON_TYPE_TX_ARP, &rArpMonPktInfo);
#endif /* CFG_QM_ARP_MONITOR_MSG */
}

static void arpMonHandleRxArpPacket(struct ADAPTER *ad, struct SW_RFB *prSwRfb)
{
	uint8_t *pucData;
	struct ARP_HEADER *prArp = NULL;
	uint8_t ucBssIdx;
	struct ARP_MON_PKT_INFO rArpMonPktInfo = {0};

	if (!GLUE_TEST_PKT_FLAG(prSwRfb->pvPacket, ENUM_PKT_ARP))
		return;

	pucData = prSwRfb->pvHeader;
	if (!pucData)
		return;

	prArp = getArpPkt(pucData, prSwRfb->u2PacketLen);
	if (!prArp)
		return;

	if (NTOHS(prArp->u2OpCode) != ARP_OPERATION_RESPONSE)
		return;

	ucBssIdx = secGetBssIdxByRfb(ad, prSwRfb);

	/* 802.11 header TA */
	if (prSwRfb->fgHdrTran) {
		HAL_RX_STATUS_GET_TA(prSwRfb->prRxStatusGroup4,
				     rArpMonPktInfo.aucTaAddr);
	} else {
		COPY_MAC_ADDR(rArpMonPktInfo.aucTaAddr,
			      ((struct WLAN_MAC_HEADER *)pucData)->aucAddr2);
	}

	rArpMonPktInfo.ucBssIdx = ucBssIdx;
	rArpMonPktInfo.u2PacketLen = prSwRfb->u2PacketLen;
	rArpMonPktInfo.pucData = pucData;

#if CFG_QM_ARP_MONITOR_MSG
	arpMonSendMsg(ad, ARP_MON_TYPE_RX_ARP, &rArpMonPktInfo);
#else /* CFG_QM_ARP_MONITOR_MSG */
	arpMonHandlePkt(ad, ARP_MON_TYPE_RX_ARP, &rArpMonPktInfo);
#endif /* CFG_QM_ARP_MONITOR_MSG */
}

static void arpMonHandleRxDhcpPacket(struct ADAPTER *ad, struct SW_RFB *prSwRfb)
{
	uint8_t *pucData;
	struct DHCP_PROTOCOL *prDhcp;
	uint16_t dhcpLen = 0;
	uint8_t ucBssIdx;
	struct ARP_MON_PKT_INFO rArpMonPktInfo = {0};

	if (!GLUE_TEST_PKT_FLAG(prSwRfb->pvPacket, ENUM_PKT_DHCP))
		return;

	pucData = prSwRfb->pvHeader;
	if (!pucData)
		return;

	/* check if pkt is DHCP from server */
	prDhcp = qmGetDhcpPkt(pucData, prSwRfb->u2PacketLen, TRUE, &dhcpLen);
	if (!prDhcp)
		return;

	ucBssIdx = secGetBssIdxByRfb(ad, prSwRfb);

	rArpMonPktInfo.ucBssIdx = ucBssIdx;
	rArpMonPktInfo.u2PacketLen = prSwRfb->u2PacketLen;
	rArpMonPktInfo.pucData = pucData;

#if CFG_QM_ARP_MONITOR_MSG
	arpMonSendMsg(ad, ARP_MON_TYPE_RX_DHCP, &rArpMonPktInfo);
#else /* CFG_QM_ARP_MONITOR_MSG */
	arpMonHandlePkt(ad, ARP_MON_TYPE_RX_DHCP, &rArpMonPktInfo);
#endif /* CFG_QM_ARP_MONITOR_MSG */
}

static void arpMonHandleNudBTOMsg(struct ADAPTER *prAdapter, uint8_t ucBssIdx)
{
	struct ARP_MON_PKT_INFO rArpMonPktInfo = {0};

	if (!prAdapter)
		return;

	if (ucBssIdx >= MAX_BSSID_NUM)
		return;

	rArpMonPktInfo.ucBssIdx = ucBssIdx;
	rArpMonPktInfo.u2PacketLen = 0;
	rArpMonPktInfo.pucData = NULL;

#if CFG_QM_ARP_MONITOR_MSG
	arpMonSendMsg(prAdapter, ARP_MON_TYPE_NUD_BTO, &rArpMonPktInfo);
#else /* CFG_QM_ARP_MONITOR_MSG */
	arpMonHandlePkt(prAdapter, ARP_MON_TYPE_NUD_BTO, &rArpMonPktInfo);
#endif /* CFG_QM_ARP_MONITOR_MSG */
}

enum ENUM_NUD_STATE arpMonNudStateConvert(uint64_t state)
{
	enum ENUM_NUD_STATE eState;

	switch (state) {
	case NUD_INCOMPLETE:
		eState = ENUM_NUD_INCOMPLETE;
		break;
	case NUD_REACHABLE:
		eState = ENUM_NUD_REACHABLE;
		break;
	case NUD_STALE:
		eState = ENUM_NUD_STALE;
		break;
	case NUD_DELAY:
		eState = ENUM_NUD_DELAY;
		break;
	case NUD_PROBE:
		eState = ENUM_NUD_PROBE;
		break;
	case NUD_FAILED:
		eState = ENUM_NUD_FAILED;
		break;
	case NUD_NOARP:
		eState = ENUM_NUD_NOARP;
		break;
	case NUD_PERMANENT:
		eState = ENUM_NUD_PERMANENT;
		break;
	case NUD_NONE:
		eState = ENUM_NUD_NONE;
		break;
	default:
		eState = ENUM_NUD_STATE_NUM;
		break;
	}

	return eState;
}


void arpMonHandleNudState(struct ADAPTER *prAdapter, uint8_t state,
	uint8_t ucBssIndex)
{
	enum ENUM_NUD_STATE eNudState;

	if (prAdapter->rWifiVar.fgArpMonitorNudDetectEn == 0)
		return;

	eNudState = arpMonNudStateConvert(state);

	switch (eNudState) {
	case ENUM_NUD_FAILED:
		if (arpMonGetNudState(prAdapter, ucBssIndex)
			!= ENUM_NUD_FAILED) {
			DBGLOG(AM, INFO,
				"State[%s] BssIdx[%u] NUD Tx:%lu Rx:%lu, reset TRX Cnt\n",
				apucNudStateStr[eNudState],
				ucBssIndex,
				arpMonGetTxCnt(prAdapter, ucBssIndex),
				arpMonGetGatewayRxCnt(prAdapter, ucBssIndex));

			arpMonSetNudState(prAdapter, ucBssIndex, eNudState);
			arpMonResetTxCnt(prAdapter, ucBssIndex);
			arpMonResetGatewayRxCnt(prAdapter, ucBssIndex);
			break;
		}

		if (arpMonGetTxCnt(prAdapter, ucBssIndex) >
			prAdapter->rWifiVar.u4NudMonitorTxNumber) {
			DBGLOG(AM, INFO,
				"State[%s] BssIdx[%u] NUD Tx:%lu>%lu  Rx:%lu\n",
				apucNudStateStr[eNudState],
				ucBssIndex,
				arpMonGetTxCnt(prAdapter, ucBssIndex),
				prAdapter->rWifiVar.u4NudMonitorTxNumber,
				arpMonGetGatewayRxCnt(prAdapter, ucBssIndex));

			if (arpMonGetGatewayRxCnt(prAdapter, ucBssIndex) == 0) {
				DBGLOG(AM, WARN, "IOT issue, arp no resp!\n");
				arpMonHandleNudBTOMsg(prAdapter, ucBssIndex);
			}

			arpMonResetTxCnt(prAdapter, ucBssIndex);
			arpMonResetGatewayRxCnt(prAdapter, ucBssIndex);
		}
		break;
	case ENUM_NUD_INCOMPLETE:
	case ENUM_NUD_PROBE:
	case ENUM_NUD_REACHABLE:
		arpMonResetTxCnt(prAdapter, ucBssIndex);
		arpMonResetGatewayRxCnt(prAdapter, ucBssIndex);
		kal_fallthrough;
	case ENUM_NUD_STALE:
	case ENUM_NUD_DELAY:
	case ENUM_NUD_NOARP:
	case ENUM_NUD_PERMANENT:
	case ENUM_NUD_NONE:
		arpMonSetNudState(prAdapter, ucBssIndex, eNudState);
		DBGLOG(AM, TRACE,
			"State[%s] BssIdx[%u] NUD Gateway Tx:%lu Rx:%lu, reset TRX Cnt\n",
			apucNudStateStr[eNudState],
			ucBssIndex,
			arpMonGetGatewayTxCnt(prAdapter, ucBssIndex),
			arpMonGetGatewayRxCnt(prAdapter, ucBssIndex));
		break;
	default:
		break;
	}
}

static void arpMonGetUnicastPktTime(struct ADAPTER *ad, struct SW_RFB *prSwRfb,
	OS_SYSTIME rCurrentTime)
{
	struct WIFI_VAR *prWifiVar = NULL;
	struct RX_CTRL *prRxCtrl;
	uint8_t *pucEthDestAddr;
	u_int8_t fgIsBMC;
	uint8_t rSrcMacAddr[MAC_ADDR_LEN];
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIdx;
	OS_SYSTIME rLastUnicastRxTime;

	if (!ad)
		return;

	prWifiVar = &ad->rWifiVar;
	/* no need to record unicast pkt time if use old rule */
	if (prWifiVar->ucArpMonitorUseRule == 0)
		return;

	if (!prSwRfb->pvHeader || !prSwRfb->pvPacket)
		return;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN + IP_HEADER_LEN)
		return;

	prRxCtrl = &ad->rRxCtrl;

	/* check if bmc */
	pucEthDestAddr = prSwRfb->pvHeader;
	fgIsBMC = (prSwRfb->fgIsBC || prSwRfb->fgIsMC ||
			IS_BMCAST_MAC_ADDR(pucEthDestAddr));

	if (fgIsBMC)
		return;

	prStaRec = cnmGetStaRecByIndex(ad,
			prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	/* update last rx unicast time when it is unicast and from gateway */
	ucBssIdx = prStaRec->ucBssIndex;
	if (ucBssIdx >= MAX_BSSID_NUM)
		return;

	getSrcMac(prSwRfb->pvHeader, prSwRfb->u2PacketLen, rSrcMacAddr);
	DBGLOG(AM, LOUD, "RX GatewayMac:" MACSTR " SrcMac:" MACSTR "\n",
		MAC2STR(arpMonGetGatewayMacPtr(ad, ucBssIdx)),
		MAC2STR(rSrcMacAddr));

	rLastUnicastRxTime = prRxCtrl->rLastUnicastRxTime[ucBssIdx];
	if (!arpMonEqualGatewayMac(ad, ucBssIdx, rSrcMacAddr))
		return;

	arpMonIncGatewayRxCnt(ad, ucBssIdx);

	prRxCtrl->rLastUnicastRxTime[ucBssIdx] = rCurrentTime;
	DBGLOG(AM, LOUD,
		"RX UNICAST [IPID=0x%04x] update %u/%u\n",
		GLUE_GET_PKT_IP_ID(prSwRfb->pvPacket),
		rLastUnicastRxTime,
		prRxCtrl->rLastUnicastRxTime[ucBssIdx]);
}

/**
 * To avoid massive TXS for test applications that send ARP request to whole
 * subnet
 *
 * If CFG_ONLY_CRITICAL_ARP_SET_TXS_LOWRATE == 1 &&
 * ARP_MONITER_ENABLE == 1 (since it needs to check the IP address)
 * Target IP |	To AP/GW  |  To others
 * ----------+------------+----------------
 * Request   |	Critical  |  Non-critical
 * Reply     |	Critical  |  Critical
 *
 * If CFG_ONLY_CRITICAL_ARP_SET_TXS_LOWRATE == 0 ||
 * ARP_MONITER_ENABLE == 0,
 * the default design sets all ARP message with TXS and send at low rate
 * Target IP |	To AP/GW  |  To others
 * ----------+------------+----------------
 * Request   |	Critical  |  Critical
 * Reply     |	Critical  |  Critical
 */
u_int8_t arpMonIpIsCritical(struct ADAPTER *ad, struct MSDU_INFO *prMsduInfo)
{
	uint8_t *pucData = NULL;
	struct ARP_HEADER *prArp;

	kalGetPacketBuf(prMsduInfo->prPacket, &pucData);
	if (!pucData)
		return FALSE;

	prArp = getArpPkt(pucData, kalQueryPacketLength(prMsduInfo->prPacket));
	if (!prArp)
		return FALSE;

	if (NTOHS(prArp->u2OpCode) == ARP_OPERATION_REQUEST) {
		if (prMsduInfo->ucBssIndex >= MAX_BSSID_NUM) {
			DBGLOG(TX, WARN, "Invalid ucBssIndex = %u\n",
				prMsduInfo->ucBssIndex);
			return FALSE;
		} else if (arpMonNotApIpAndGatewayIp(ad,
			prMsduInfo->ucBssIndex, prArp->aucTargetIPaddr)) {
			DBGLOG(TX, TRACE,
				"ARP to " IPV4STR " is non-critical\n",
				IPV4TOSTR(prArp->aucTargetIPaddr));
			return FALSE;
		}
	}

	DBGLOG(TX, TRACE, "ARP to " IPV4STR " is critical\n",
	       IPV4TOSTR(prArp->aucTargetIPaddr));
	return TRUE;
}


u_int8_t arpMonIsCritical(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	if (!ad) {
		DBGLOG(AM, WARN, "ad is NULL\n");
		return FALSE;
	}

	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(AM, WARN, "arpMoniter invalid Bssidx[%u]\n",
			ucBssIdx);
		return FALSE;
	}

	DBGLOG(AM, LOUD, "[%u] arpMoniter:[Mon, Thres][%u, %u]\n",
			ucBssIdx,
			arpMonGetGatewayTxCnt(ad, ucBssIdx),
			arpMonGetCriticalThres(ad, ucBssIdx));

	return (arpMonGetGatewayTxCnt(ad, ucBssIdx) >
			arpMonGetCriticalThres(ad, ucBssIdx));
}

void arpMonResetArpDetect(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	if (!ad)
		return;

	if (ucBssIdx >= MAX_BSSID_NUM)
		return;

	arpMonReset(ad, ucBssIdx);

	/* Don't reset the gatewayip while roaming or processing BTO */
	if (!(roamingFsmCheckIfRoaming(ad, ucBssIdx))) {
		arpMonResetGateway(ad, ucBssIdx);
		DBGLOG(AM, TRACE, "Reset gatewayIp and gatewayMac\n");
	}
}

void arpMonProcessRxPacket(struct ADAPTER *ad, struct BSS_INFO *prBssInfo,
	struct SW_RFB *prSwRfb, OS_SYSTIME rCurrentTime)
{
	if (!ad)
		return;

	if (IS_BSS_INFO_IN_AIS(prBssInfo))
		arpMonHandleRxArpPacket(ad, prSwRfb);

	/* STA or GC */
	arpMonHandleRxDhcpPacket(ad, prSwRfb);

	arpMonGetUnicastPktTime(ad, prSwRfb, rCurrentTime);
}

void arpMonProcessTxPacket(struct ADAPTER *ad, struct MSDU_INFO *prMsduInfo)
{
	if (prMsduInfo->ucPktType != ENUM_PKT_ARP)
		return;

	arpMonDetectNoResponse(ad, prMsduInfo);
}
#endif /* ARP_MONITER_ENABLE */
