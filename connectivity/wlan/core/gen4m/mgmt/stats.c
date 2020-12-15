/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

/*******************************************************************************
 *            C O M P I L E R	 F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *            E X T E R N A L	R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#if (CFG_SUPPORT_STATISTICS == 1)

enum EVENT_TYPE {
	EVENT_RX,
	EVENT_TX,
};
/*******************************************************************************
 *            C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *            F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *            P R I V A T E  F U N C T I O N S
 *******************************************************************************
 */

uint32_t u4TotalTx;
uint32_t u4NoDelayTx;
uint32_t u4TotalRx;
uint32_t u4NoDelayRx;

static uint8_t g_ucTxRxFlag;
static uint8_t g_ucTxIpProto;
static uint16_t g_u2TxUdpPort;
static uint32_t g_u4TxDelayThreshold;
static uint8_t g_ucRxIpProto;
static uint16_t g_u2RxUdpPort;
static uint32_t g_u4RxDelayThreshold;

void StatsResetTxRx(void)
{
	u4TotalRx = 0;
	u4TotalTx = 0;
	u4NoDelayRx = 0;
	u4NoDelayTx = 0;
}

uint64_t StatsEnvTimeGet(void)
{
	uint64_t u8Clk;

	u8Clk = sched_clock();	/* unit: naro seconds */

	return (uint64_t) u8Clk;	/* sched_clock *//* jiffies size = 4B */
}

void StatsEnvGetPktDelay(OUT uint8_t *pucTxRxFlag,
	OUT uint8_t *pucTxIpProto, OUT uint16_t *pu2TxUdpPort,
	OUT uint32_t *pu4TxDelayThreshold, OUT uint8_t *pucRxIpProto,
	OUT uint16_t *pu2RxUdpPort, OUT uint32_t *pu4RxDelayThreshold)
{
	*pucTxRxFlag = g_ucTxRxFlag;
	*pucTxIpProto = g_ucTxIpProto;
	*pu2TxUdpPort = g_u2TxUdpPort;
	*pu4TxDelayThreshold = g_u4TxDelayThreshold;
	*pucRxIpProto = g_ucRxIpProto;
	*pu2RxUdpPort = g_u2RxUdpPort;
	*pu4RxDelayThreshold = g_u4RxDelayThreshold;
}

void StatsEnvSetPktDelay(IN uint8_t ucTxOrRx, IN uint8_t ucIpProto,
	IN uint16_t u2UdpPort, uint32_t u4DelayThreshold)
{
#define MODULE_RESET 0
#define MODULE_TX 1
#define MODULE_RX 2

	if (ucTxOrRx == MODULE_TX) {
		g_ucTxRxFlag |= BIT(0);
		g_ucTxIpProto = ucIpProto;
		g_u2TxUdpPort = u2UdpPort;
		g_u4TxDelayThreshold = u4DelayThreshold;
	} else if (ucTxOrRx == MODULE_RX) {
		g_ucTxRxFlag |= BIT(1);
		g_ucRxIpProto = ucIpProto;
		g_u2RxUdpPort = u2UdpPort;
		g_u4RxDelayThreshold = u4DelayThreshold;
	} else if (ucTxOrRx == MODULE_RESET) {
		g_ucTxRxFlag = 0;
		g_ucTxIpProto = 0;
		g_u2TxUdpPort = 0;
		g_u4TxDelayThreshold = 0;
		g_ucRxIpProto = 0;
		g_u2RxUdpPort = 0;
		g_u4RxDelayThreshold = 0;
	}
}

void StatsEnvRxTime2Host(IN struct ADAPTER *prAdapter, struct sk_buff *prSkb)
{
	uint8_t *pucEth = prSkb->data;
	uint16_t u2EthType = 0;
	uint8_t ucIpVersion = 0;
	uint8_t ucIpProto = 0;
	uint16_t u2IPID = 0;
	uint16_t u2UdpDstPort = 0;
	uint16_t u2UdpSrcPort = 0;
	uint64_t u8IntTime = 0;
	uint64_t u8RxTime = 0;
	uint32_t u4Delay = 0;
	struct timeval tval;
	struct rtc_time tm;

	if ((g_ucTxRxFlag & BIT(1)) == 0)
		return;

	if (prSkb->len <= 24 + ETH_HLEN)
		return;
	u2EthType = (pucEth[ETH_TYPE_LEN_OFFSET] << 8)
		| (pucEth[ETH_TYPE_LEN_OFFSET + 1]);
	pucEth += ETH_HLEN;
	if (u2EthType != ETH_P_IPV4)
		return;
	ucIpProto = pucEth[9];
	if (g_ucRxIpProto && (ucIpProto != g_ucRxIpProto))
		return;
	ucIpVersion = (pucEth[0] & IPVH_VERSION_MASK) >> IPVH_VERSION_OFFSET;
	if (ucIpVersion != IPVERSION)
		return;
	u2IPID = pucEth[4] << 8 | pucEth[5];
	u8IntTime = GLUE_RX_GET_PKT_INT_TIME(prSkb);
	u4Delay = ((uint32_t)(sched_clock() - u8IntTime))/NSEC_PER_USEC;
	u8RxTime = GLUE_RX_GET_PKT_RX_TIME(prSkb);
	do_gettimeofday(&tval);
	rtc_time_to_tm(tval.tv_sec, &tm);

	switch (ucIpProto) {
	case IP_PRO_TCP:
	case IP_PRO_UDP:
		u2UdpSrcPort = (pucEth[20] << 8) | pucEth[21];
		u2UdpDstPort = (pucEth[22] << 8) | pucEth[23];
		if (g_u2RxUdpPort && (u2UdpSrcPort != g_u2RxUdpPort))
			break;
	case IP_PRO_ICMP:
		u4TotalRx++;
		if (g_u4RxDelayThreshold && (u4Delay <= g_u4RxDelayThreshold)) {
			u4NoDelayRx++;
			break;
		}
		DBGLOG(RX, INFO,
	"IPID 0x%04x src %d dst %d UP %d,delay %u us,int2rx %lu us,IntTime %llu,%u/%u,leave at %02d:%02d:%02d.%06ld\n",
			u2IPID, u2UdpSrcPort, u2UdpDstPort,
			((pucEth[1] & IPTOS_PREC_MASK) >> IPTOS_PREC_OFFSET),
			u4Delay,
			((uint32_t)(u8RxTime - u8IntTime))/NSEC_PER_USEC,
			u8IntTime, u4NoDelayRx, u4TotalRx,
			tm.tm_hour, tm.tm_min, tm.tm_sec, tval.tv_usec);
		break;
	default:
		break;
	}
}

void StatsEnvTxTime2Hif(IN struct ADAPTER *prAdapter,
	IN struct MSDU_INFO *prMsduInfo)
{
	uint64_t u8SysTime, u8SysTimeIn;
	uint32_t u4TimeDiff;
	uint8_t *pucEth = ((struct sk_buff *)prMsduInfo->prPacket)->data;
	uint32_t u4PacketLen = ((struct sk_buff *)prMsduInfo->prPacket)->len;
	uint8_t ucIpVersion = 0;
	uint8_t ucIpProto = 0;
	uint8_t *pucEthBody = NULL;
	uint16_t u2EthType = 0;
	uint8_t *pucAheadBuf = NULL;
	uint16_t u2IPID = 0;
	uint16_t u2UdpDstPort = 0;
	uint16_t u2UdpSrcPort = 0;
	uint16_t u4TxHeadRoomSize = 0;
	struct mt66xx_chip_info *prChipInfo;

	u8SysTime = StatsEnvTimeGet();
	u8SysTimeIn = GLUE_GET_PKT_XTIME(prMsduInfo->prPacket);
	prChipInfo = prAdapter->chip_info;
	u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH +
		prChipInfo->txd_append_size;

	if ((g_ucTxRxFlag & BIT(0)) == 0)
		return;

	if ((u8SysTimeIn == 0) || (u8SysTime <= u8SysTimeIn))
		return;

	/* units of u4TimeDiff is micro seconds (us) */
	if (u4PacketLen < 24 + ETH_HLEN)
		return;
	pucAheadBuf = &pucEth[u4TxHeadRoomSize];
	u2EthType = (pucAheadBuf[ETH_TYPE_LEN_OFFSET] << 8)
		| (pucAheadBuf[ETH_TYPE_LEN_OFFSET + 1]);
	pucEthBody = &pucAheadBuf[ETH_HLEN];
	if (u2EthType != ETH_P_IPV4)
		return;
	ucIpProto = pucEthBody[9];
	if (g_ucTxIpProto && (ucIpProto != g_ucTxIpProto))
		return;
	ucIpVersion = (pucEthBody[0] & IPVH_VERSION_MASK)
		>> IPVH_VERSION_OFFSET;
	if (ucIpVersion != IPVERSION)
		return;
	u2IPID = pucEthBody[4]<<8 | pucEthBody[5];
	u8SysTime = u8SysTime - u8SysTimeIn;
	u4TimeDiff = (uint32_t) u8SysTime;
	u4TimeDiff = u4TimeDiff / 1000;	/* ns to us */

	switch (ucIpProto) {
	case IP_PRO_TCP:
	case IP_PRO_UDP:
		u2UdpDstPort = (pucEthBody[22] << 8) | pucEthBody[23];
		u2UdpSrcPort = (pucEthBody[20] << 8) | pucEthBody[21];
		if (g_u2TxUdpPort && (u2UdpDstPort != g_u2TxUdpPort))
			break;
	case IP_PRO_ICMP:
		u4TotalTx++;
		if (g_u4TxDelayThreshold
			&& (u4TimeDiff <= g_u4TxDelayThreshold)) {
			u4NoDelayTx++;
			break;
		}
		DBGLOG(TX, INFO,
			"IPID 0x%04x src %d dst %d UP %d,delay %u us,u8SysTimeIn %llu, %u/%u\n",
			u2IPID, u2UdpSrcPort, u2UdpDstPort,
			((pucEthBody[1] & IPTOS_PREC_MASK)
				>> IPTOS_PREC_OFFSET),
			u4TimeDiff, u8SysTimeIn, u4NoDelayTx, u4TotalTx);
		break;
	default:
		break;
	}
}

void statsParseARPInfo(struct ADAPTER *prAdapter, struct sk_buff *skb,
			uint8_t *pucEthBody, uint8_t eventType)
{
	uint16_t u2OpCode = (pucEthBody[6] << 8) | pucEthBody[7];

	switch (eventType) {
	case EVENT_RX:
		GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
		if (u2OpCode == ARP_PRO_REQ)
			DBGLOG_LIMITED(RX, TRACE,
				"<RX> Arp Req From IP: %d.%d.%d.%d\n",
				pucEthBody[14], pucEthBody[15],
				pucEthBody[16], pucEthBody[17]);
		else if (u2OpCode == ARP_PRO_RSP)
			DBGLOG_LIMITED(RX, TRACE,
				"<RX> Arp Rsp from IP: %d.%d.%d.%d\n",
				pucEthBody[14], pucEthBody[15],
				pucEthBody[16], pucEthBody[17]);
		break;
	case EVENT_TX:
		if (u2OpCode == ARP_PRO_REQ)
			DBGLOG_LIMITED(TX, TRACE,
				"<TX> Arp Req to IP: %d.%d.%d.%d\n",
				pucEthBody[24], pucEthBody[25],
				pucEthBody[26], pucEthBody[27]);
		else if (u2OpCode == ARP_PRO_RSP)
			DBGLOG_LIMITED(TX, TRACE,
				"<TX> Arp Rsp to IP: %d.%d.%d.%d\n",
				pucEthBody[24], pucEthBody[25],
				pucEthBody[26], pucEthBody[27]);
		break;
	}
}

void statsParseUDPInfo(struct ADAPTER *prAdapter,  struct sk_buff *skb,
			uint8_t *pucEthBody, uint8_t eventType,
			uint16_t u2IpId)
{
	/* the number of DHCP packets is seldom so we print log here */
	uint8_t *pucUdp = &pucEthBody[20];
	uint8_t *pucBootp = &pucUdp[UDP_HDR_LEN];
	struct BOOTP_PROTOCOL *prBootp = NULL;
	uint32_t udpLength = 0;
	uint32_t i = 0;
	uint16_t u2UdpDstPort;
	uint16_t u2UdpSrcPort;
	uint32_t u4TransID;

	prBootp = (struct BOOTP_PROTOCOL *) &pucUdp[UDP_HDR_LEN];
	u2UdpDstPort = (pucUdp[2] << 8) | pucUdp[3];
	u2UdpSrcPort = (pucUdp[0] << 8) | pucUdp[1];
	if (u2UdpDstPort == UDP_PORT_DHCPS || u2UdpDstPort == UDP_PORT_DHCPC) {
		WLAN_GET_FIELD_BE32(&prBootp->u4TransId, &u4TransID);
		switch (eventType) {
		case EVENT_RX:
			GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
			DBGLOG_LIMITED(RX, INFO,
				"<RX> DHCP: IPID 0x%02x, MsgType 0x%x, TransID 0x%04x\n",
				u2IpId, prBootp->aucOptions[6],
				u4TransID);
#if CFG_SUPPORT_REPORT_MISC
			prBootp = (struct BOOTP_PROTOCOL *) pucBootp;
			udpLength = pucUdp[4] << 8 | pucUdp[5];
			if (udpLength <= 248) {
				DBGLOG(RX, INFO,
				       "Length of DHCP less than 248!\n");
				break;
			}
			while (i < (udpLength - 248) &&
			       i < (NORMAL_DHCP_UDP_LEN - 248)) {
				/* option end */
				if (prBootp->aucOptions[i + 4] == 255) {
					DBGLOG(RX, WARN, "i:%d, udpLength:%d\n",
					       i, udpLength);
					break;
				}
				if (prBootp->aucOptions[i + 4] == 53 &&
				    prBootp->aucOptions[i + 6] == 5 &&
				    prAdapter->rReportMiscSet.eQueryNum
					== REPORT_DHCP_START) {
					wlanGetReportMisc(prAdapter);
					prAdapter->rReportMiscSet.eQueryNum
							= REPORT_DHCP_END;
					break;
				}
				i += prBootp->aucOptions[i + 5] + 2;
			}
#endif
			break;
		case EVENT_TX:
			DBGLOG_LIMITED(TX, INFO,
				"<TX> DHCP: IPID 0x%02x, MsgType 0x%x, TransID 0x%04x\n",
				u2IpId, prBootp->aucOptions[6],
				u4TransID);
			break;
		}
	} else if (u2UdpSrcPort == UDP_PORT_DNS) { /* tx dns */
		uint16_t u2TransId =
			(pucBootp[0] << 8) | pucBootp[1];
			if (eventType == EVENT_RX)
				GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
				DBGLOG_LIMITED(RX, INFO,
					"<RX> DNS: IPID 0x%02x, TransID 0x%04x\n",
					u2IpId, u2TransId);
	}
}

void statsParseIPV4Info(struct ADAPTER *prAdapter, struct sk_buff *skb,
			uint8_t *pucEthBody, uint8_t eventType)
{
	/* IP header without options */
	uint8_t ucIpProto = pucEthBody[9];
	uint8_t ucIpVersion =
		(pucEthBody[0] & IPVH_VERSION_MASK)
			>> IPVH_VERSION_OFFSET;
	uint16_t u2IpId = *(uint16_t *) &pucEthBody[4];

	if (ucIpVersion != IPVERSION)
		return;
	switch (ucIpProto) {
	case IP_PRO_ICMP:
	{
		/* the number of ICMP packets is seldom so we print log here */
		uint8_t ucIcmpType;
		uint16_t u2IcmpId, u2IcmpSeq;
		uint8_t *pucIcmp = &pucEthBody[20];

		ucIcmpType = pucIcmp[0];
		/* don't log network unreachable packet */
		if (ucIcmpType == 3)
			break;
		u2IcmpId = *(uint16_t *) &pucIcmp[4];
		u2IcmpSeq = *(uint16_t *) &pucIcmp[6];
		switch (eventType) {
		case EVENT_RX:
			GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
			DBGLOG_LIMITED(RX, TRACE,
				"<RX> ICMP: Type %d, Id BE 0x%04x, Seq BE 0x%04x\n",
				ucIcmpType, u2IcmpId, u2IcmpSeq);
			break;
		case EVENT_TX:
			DBGLOG_LIMITED(TX, TRACE,
				"<TX> ICMP: Type %d, Id 0x%04x, Seq BE 0x%04x\n",
				ucIcmpType, u2IcmpId, u2IcmpSeq);
			break;
		}
		break;
	}
	case IP_PRO_UDP:
		statsParseUDPInfo(prAdapter, skb,
					pucEthBody, eventType, u2IpId);
	}
}

static void statsParsePktInfo(IN struct ADAPTER *prAdapter, uint8_t *pucPkt,
			      struct sk_buff *skb, uint8_t status,
			      uint8_t eventType)
{
	/* get ethernet protocol */
	uint16_t u2EtherType =
		(pucPkt[ETH_TYPE_LEN_OFFSET] << 8)
			| (pucPkt[ETH_TYPE_LEN_OFFSET + 1]);
	uint8_t *pucEthBody = &pucPkt[ETH_HLEN];

	switch (u2EtherType) {
	case ETH_P_ARP:
		statsParseARPInfo(prAdapter, skb, pucEthBody, eventType);
		if (eventType == EVENT_RX)
			wlanLogRxData(WLAN_WAKE_ARP);
		break;
	case ETH_P_IPV4:
		statsParseIPV4Info(prAdapter, skb, pucEthBody, eventType);
		if (eventType == EVENT_RX)
			wlanLogRxData(WLAN_WAKE_IPV4);
		break;
	case ETH_P_IPV6:
	{
		/* IPv6 header without options */
		uint8_t ucIpv6Proto =
			pucEthBody[IPV6_HDR_PROTOCOL_OFFSET];
		uint8_t ucIpVersion =
			(pucEthBody[0] & IPVH_VERSION_MASK)
				>> IPVH_VERSION_OFFSET;

		if (ucIpVersion != IP_VERSION_6)
			break;

		if (eventType == EVENT_RX) {
			wlanLogRxData(WLAN_WAKE_IPV6);
		}

		switch (ucIpv6Proto) {
		case 0x06:/*tcp*/
			switch (eventType) {
			case EVENT_RX:
				DBGLOG(RX, TRACE, "<RX><IPv6> tcp packet\n");
				break;
			case EVENT_TX:
				DBGLOG(TX, TRACE, "<TX><IPv6> tcp packet\n");
				break;
			}
			break;

		case 0x11:/*UDP*/
			switch (eventType) {
			case EVENT_RX:
			{
				uint16_t ucIpv6UDPSrcPort = 0;

				/* IPv6 header without options */
				ucIpv6UDPSrcPort = pucEthBody[IPV6_HDR_LEN];
				ucIpv6UDPSrcPort = ucIpv6UDPSrcPort << 8;
				ucIpv6UDPSrcPort +=
					pucEthBody[IPV6_HDR_LEN + 1];

				switch (ucIpv6UDPSrcPort) {
				case 53:/*dns port*/
					DBGLOG(RX, TRACE,
						"<RX><IPv6> dns packet\n");
					GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
					break;
				case 547:/*dhcp*/
				case 546:
					DBGLOG_LIMITED(RX, TRACE,
						"<RX><IPv6> dhcp packet\n");
					GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
					break;
				case 123:/*ntp port*/
					DBGLOG_LIMITED(RX, TRACE,
						"<RX><IPv6> ntp packet\n");
					GLUE_SET_INDEPENDENT_PKT(skb, TRUE);
					break;
				default:
					DBGLOG(RX, TRACE,
					"<RX><IPv6> other packet srtport=%u\n",
						ucIpv6UDPSrcPort);
					break;
				}
			}
				break;
			case EVENT_TX:
				DBGLOG_LIMITED(TX, TRACE,
					"<TX><IPv6> UDP packet\n");
				break;
			}
			break;

		case 0x00:/*IPv6  hop-by-hop*/
			switch (eventType) {
			case EVENT_RX:
				/*need chech detai pakcet type*/
				/*130 mlti listener query*/
				/*143 multi listener report v2*/
				GLUE_SET_INDEPENDENT_PKT(skb, TRUE);

				DBGLOG_LIMITED(RX, TRACE,
					"<RX><IPv6> hop-by-hop packet\n");
				break;
			case EVENT_TX:
				DBGLOG_LIMITED(TX, TRACE,
					"<TX><IPv6> hop-by-hop packet\n");
				break;
			}
			break;

		case 0x3a:/*ipv6 ICMPV6*/
			switch (eventType) {
			case EVENT_RX:
			{
				uint8_t ucICMPv6Type = 0;

				/* IPv6 header without options */
				ucICMPv6Type = pucEthBody[IPV6_HDR_LEN];
				GLUE_SET_INDEPENDENT_PKT(skb, TRUE);

				switch (ucICMPv6Type) {
				case 0x85: /*ICMPV6_TYPE_ROUTER_SOLICITATION*/
					DBGLOG_LIMITED(RX, TRACE,
				"<RX><IPv6> ICMPV6 Router Solicitation\n");
					break;

				case 0x86: /*ICMPV6_TYPE_ROUTER_ADVERTISEMENT*/
					DBGLOG_LIMITED(RX, TRACE,
				"<RX><IPv6> ICMPV6 Router Advertisement\n");
					break;

				case ICMPV6_TYPE_NEIGHBOR_SOLICITATION:
					DBGLOG_LIMITED(RX, TRACE,
				"<RX><IPv6> ICMPV6 Neighbor Solicitation\n");
					break;

				case ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT:
					DBGLOG_LIMITED(RX, TRACE,
				"<RX><IPv6> ICMPV6 Neighbor Advertisement\n");
					break;
				default:
					DBGLOG_LIMITED(RX, TRACE,
						"<RX><IPv6> ICMPV6 type=%u\n",
						ucICMPv6Type);
					break;
				}
			}
				break;
			case EVENT_TX:
				DBGLOG(TX, INFO, "<TX><IPv6> ICMPV6 packet\n");
				break;
			}
			break;
		default:
			if (eventType == EVENT_RX)
				DBGLOG_LIMITED(RX, TRACE,
				"<RX><IPv6> default protocol=%u\n",
				ucIpv6Proto);
			break;
		}
		break;
	}
	case ETH_P_1X:
	{
		uint8_t *pucEapol = pucEthBody;
		uint8_t ucEapolType = pucEapol[1];
		uint16_t u2KeyInfo = 0;

		if (eventType == EVENT_RX) {
			wlanLogRxData(WLAN_WAKE_1X);
		}

		switch (ucEapolType) {
		case 0: /* eap packet */
			switch (eventType) {
			case EVENT_RX:
				DBGLOG_LIMITED(RX, INFO,
					"<RX> EAP Packet: code %d, id %d, type %d\n",
					pucEapol[4], pucEapol[5], pucEapol[8]);
				break;
			case EVENT_TX:
				DBGLOG_LIMITED(TX, INFO,
					"<TX> EAP Packet: code %d, id %d, type %d\n",
					pucEapol[4], pucEapol[5],
					pucEapol[8]);
				break;
			}
			break;
		case 1: /* eapol start */
			switch (eventType) {
			case EVENT_RX:
				DBGLOG_LIMITED(RX, TRACE,
					"<RX> EAPOL: start\n");
				break;
			case EVENT_TX:
				DBGLOG_LIMITED(TX, TRACE,
					"<TX> EAPOL: start\n");
				break;
			}
			break;
		case 3: /* key */
			switch (eventType) {
			case EVENT_RX:
				DBGLOG_LIMITED(RX, INFO,
					"<RX> EAPOL: key, KeyInfo 0x%04x\n",
					*((uint16_t *)(&pucEapol[5])));
#if CFG_SUPPORT_REPORT_MISC
				WLAN_GET_FIELD_BE16(&pucEapol[5], &u2KeyInfo);
				if ((u2KeyInfo & 0x388) == 0x88) {
					/* 1/4 key, init report */
					if (prAdapter->rReportMiscSet.eQueryNum
					    != REPORT_4WAYHS_START) {
						wlanGetReportMisc(prAdapter);
						prAdapter->rReportMiscSet
							.eQueryNum =
							REPORT_4WAYHS_START;
					}
				}
#endif
				break;
			case EVENT_TX:
				DBGLOG_LIMITED(TX, INFO,
					"<TX> EAPOL: key, KeyInfo 0x%04x\n",
					*((uint16_t *)(&pucEapol[5])));
				break;
			}

			break;
		}
		break;
	}
#if CFG_SUPPORT_WAPI
	case ETH_WPI_1X:
	{
		uint8_t ucSubType = pucEthBody[3]; /* sub type filed*/
		uint16_t u2Length = *(uint16_t *)&pucEthBody[6];
		uint16_t u2Seq = *(uint16_t *)&pucEthBody[8];

		switch (eventType) {
		case EVENT_RX:
			DBGLOG_LIMITED(RX, INFO,
				"<RX> WAPI: subType %d, Len %d, Seq %d\n",
				ucSubType, u2Length, u2Seq);
			break;
		case EVENT_TX:
			DBGLOG_LIMITED(TX, INFO,
				"<TX> WAPI: subType %d, Len %d, Seq %d\n",
				ucSubType, u2Length, u2Seq);
			break;
		}
		break;
	}
#endif
	case ETH_PRO_TDLS:

		switch (eventType) {
		case EVENT_RX:
			wlanLogRxData(WLAN_WAKE_TDLS);
			DBGLOG_LIMITED(RX, INFO,
				"<RX> TDLS type %d, category %d, Action %d, Token %d\n",
				pucEthBody[0], pucEthBody[1],
				pucEthBody[2], pucEthBody[3]);
			break;
		case EVENT_TX:
			DBGLOG_LIMITED(TX, INFO,
				"<TX> TDLS type %d, category %d, Action %d, Token %d\n",
				pucEthBody[0], pucEthBody[1],
				pucEthBody[2], pucEthBody[3]);
			break;
		}
		break;
	default:
		if (eventType == EVENT_RX)
			wlanLogRxData(WLAN_WAKE_OTHER);
		break;
	}
}
/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to display rx packet information.
 *
 * \param[in] pPkt			Pointer to the packet
 * \param[out] None
 *
 * \retval None
 */
/*----------------------------------------------------------------------------*/
void StatsRxPktInfoDisplay(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	uint8_t *pPkt = NULL;
	struct sk_buff *skb = NULL;

	if (prSwRfb->u2PacketLen <= ETHER_HEADER_LEN)
		return;

	pPkt = prSwRfb->pvHeader;
	if (!pPkt)
		return;

	skb = (struct sk_buff *)(prSwRfb->pvPacket);
	if (!skb)
		return;

	statsParsePktInfo(prAdapter, pPkt, skb, 0, EVENT_RX);
}

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to display tx packet information.
 *
 * \param[in] pPkt			Pointer to the packet
 * \param[out] None
 *
 * \retval None
 */
/*----------------------------------------------------------------------------*/
void StatsTxPktInfoDisplay(uint8_t *pPkt)
{
	uint16_t u2EtherTypeLen;

	u2EtherTypeLen =
		(pPkt[ETH_TYPE_LEN_OFFSET] << 8)
			| (pPkt[ETH_TYPE_LEN_OFFSET + 1]);
	statsParsePktInfo(NULL, pPkt, NULL, 0, EVENT_TX);
}

#endif /* CFG_SUPPORT_STATISTICS */

/* End of stats.c */
