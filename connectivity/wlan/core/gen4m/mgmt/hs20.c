// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "hs20.c"
 *    \brief  This file including the hotspot 2.0 related function.
 *
 *    This file provided the macros and functions library support for the
 *    protocol layer hotspot 2.0 related function.
 *
 */


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"

#if CFG_SUPPORT_PASSPOINT

/******************************************************************************
 *                              C O N S T A N T S
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
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

void hs20FillExtCapIE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct MSDU_INFO *prMsduInfo)
{
	struct IE_EXT_CAP *prExtCap;
	struct HS20_INFO *prHS20Info;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	ucBssIndex = prMsduInfo->ucBssIndex;
	prHS20Info = aisGetHS20Info(prAdapter, ucBssIndex);
	if (!prHS20Info)
		return;

	/* Add Extended Capabilities IE */
	prExtCap = (struct IE_EXT_CAP *)
	    (((uint8_t *) prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);

	prExtCap->ucId = ELEM_ID_EXTENDED_CAP;
	if (prHS20Info->fgConnectHS20AP == TRUE)
		prExtCap->ucLength = ELEM_MAX_LEN_EXT_CAP;
	else
		prExtCap->ucLength = 3 - ELEM_HDR_LEN;

	kalMemZero(prExtCap->aucCapabilities, ELEM_MAX_LEN_EXT_CAP);

	prExtCap->aucCapabilities[0] = ELEM_EXT_CAP_DEFAULT_VAL;

	if (prBssInfo->eCurrentOPMode != OP_MODE_INFRASTRUCTURE)
		prExtCap->aucCapabilities[0] &= ~ELEM_EXT_CAP_PSMP_CAP;

	if (prHS20Info->fgConnectHS20AP == TRUE) {
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_BSS_TRANSITION_BIT);
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_UTC_TSF_OFFSET_BIT);
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_INTERWORKING_BIT);
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_QOSMAPSET_BIT);

		/* For R2 WNM-Notification */
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP,
			ELEM_EXT_CAP_WNM_NOTIFICATION_BIT);
	}

	pr_info("IE_SIZE(prExtCap) = %d, %d %d\n",
		IE_SIZE(prExtCap), ELEM_HDR_LEN, ELEM_MAX_LEN_EXT_CAP);

	ASSERT(IE_SIZE(prExtCap) <= (ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP));

	prMsduInfo->u2FrameLength += IE_SIZE(prExtCap);
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief    This function is called to fill up
 *            the content of Ext Cap IE bit 31.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[out] pucIE  Pointer of the IE buffer
 *
 * \return VOID
 */
/*---------------------------------------------------------------------------*/
void hs20FillProreqExtCapIE(struct ADAPTER *prAdapter, uint8_t *pucIE)
{
	struct IE_EXT_CAP *prExtCap;
	struct HS20_INFO *prHS20Info;

	ASSERT(prAdapter);

	prHS20Info = aisGetHS20Info(prAdapter,
		0);
	if (!prHS20Info)
		return;

	/* Add Extended Capabilities IE */
	prExtCap = (struct IE_EXT_CAP *) pucIE;

	prExtCap->ucId = ELEM_ID_EXTENDED_CAP;
	if (prHS20Info->fgConnectHS20AP == TRUE)
		prExtCap->ucLength = ELEM_MAX_LEN_EXT_CAP;
	else
		prExtCap->ucLength = 3 - ELEM_HDR_LEN;

	kalMemZero(prExtCap->aucCapabilities, prExtCap->ucLength);

	prExtCap->aucCapabilities[0] = ELEM_EXT_CAP_DEFAULT_VAL;

	if (prHS20Info->fgConnectHS20AP == TRUE) {
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_BSS_TRANSITION_BIT);
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_UTC_TSF_OFFSET_BIT);
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_INTERWORKING_BIT);
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP, ELEM_EXT_CAP_QOSMAPSET_BIT);

		/* For R2 WNM-Notification */
		SET_EXT_CAP(prExtCap->aucCapabilities,
			ELEM_MAX_LEN_EXT_CAP,
			ELEM_EXT_CAP_WNM_NOTIFICATION_BIT);
	}
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief    This function is called to fill up the content of HS2.0 IE.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[out] pucIE  Pointer of the IE buffer
 *
 * \return VOID
 */
/*---------------------------------------------------------------------------*/
void hs20FillHS20IE(struct ADAPTER *prAdapter, uint8_t *pucIE)
{
	struct IE_HS20_INDICATION *prHS20IndicationIe;
	/* P_HS20_INFO_T prHS20Info; */
	uint8_t aucWfaOui[] = VENDOR_OUI_WFA_SPECIFIC;

	/* prHS20Info = &(prAdapter->rWifiVar.rHS20Info); */

	prHS20IndicationIe = (struct IE_HS20_INDICATION *) pucIE;

	prHS20IndicationIe->ucId = ELEM_ID_VENDOR;
	prHS20IndicationIe->ucLength =
		sizeof(struct IE_HS20_INDICATION) - ELEM_HDR_LEN;
	prHS20IndicationIe->aucOui[0] = aucWfaOui[0];
	prHS20IndicationIe->aucOui[1] = aucWfaOui[1];
	prHS20IndicationIe->aucOui[2] = aucWfaOui[2];
	prHS20IndicationIe->ucType = VENDOR_OUI_TYPE_HS20;

	/* For PASSPOINT_R1 */
	/* prHS20IndicationIe->ucHotspotConfig = 0x00; */

	/* For PASSPOINT_R2 */
	prHS20IndicationIe->ucHotspotConfig = 0x10;

}

/*---------------------------------------------------------------------------*/
/*!
 * \brief    This function is called while calculating length of
 *             hotspot 2.0 indication IE for Probe Request.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] pucTargetBSSID  Pointer of target HESSID
 *
 * \return the length of composed HS20 IE
 */
/*---------------------------------------------------------------------------*/
uint32_t hs20CalculateHS20RelatedIEForProbeReq(struct ADAPTER *prAdapter,
		uint8_t *pucTargetBSSID)
{
	uint32_t u4IeLength;

	if (0)			/* Todo:: Not HS20 STA */
		return 0;

	u4IeLength = sizeof(struct IE_HS20_INDICATION)
		+ /* sizeof(IE_INTERWORKING_T) */ +
		(ELEM_HDR_LEN + ELEM_MAX_LEN_EXT_CAP);

	if (!pucTargetBSSID) {
		/* Todo:: Nothing */
		/* u4IeLength -= MAC_ADDR_LEN; */
	}

	return u4IeLength;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief    This function is called while composing
 *             hotspot 2.0 indication IE for Probe Request.
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 * \param[in] pucTargetBSSID  Pointer of target HESSID
 * \param[out] prIE  Pointer of the IE buffer
 *
 * \return the wlan status
 */
/*---------------------------------------------------------------------------*/
uint32_t hs20GenerateHS20RelatedIEForProbeReq(struct ADAPTER *prAdapter,
		uint8_t *pucTargetBSSID, uint8_t *prIE)
{
	if (0)			/* Todo:: Not HS20 STA */
		return 0;
#if 0
	struct HS20_INFO *prHS20Info;

	prHS20Info = &(prAdapter->rWifiVar.rHS20Info);

	/*
	 * Generate 802.11u Interworking IE (107)
	 */
	hs20FillInterworkingIE(prAdapter,
		prHS20Info->ucAccessNetworkOptions,
		prHS20Info->ucVenueGroup,
		prHS20Info->ucVenueType,
		pucTargetBSSID, prIE);
	prIE += IE_SIZE(prIE);
#endif
	/*
	 * Generate Ext Cap IE (127)
	 */
	hs20FillProreqExtCapIE(prAdapter, prIE);
	prIE += IE_SIZE(prIE);

	/*
	 * Generate HS2.0 Indication IE (221)
	 */
	hs20FillHS20IE(prAdapter, prIE);
	prIE += IE_SIZE(prIE);

	return WLAN_STATUS_SUCCESS;
}

u_int8_t hs20IsGratuitousArp(struct ADAPTER *prAdapter,
		struct SW_RFB *prCurrSwRfb)
{
	uint8_t *pucSenderIP =
		prCurrSwRfb->pvHeader + ETHER_HEADER_LEN + ARP_SENDER_IP_OFFSET;
	uint8_t *pucTargetIP =
		prCurrSwRfb->pvHeader + ETHER_HEADER_LEN + ARP_TARGET_IP_OFFSET;
	uint8_t *pucSenderMac = ((uint8_t *)
		prCurrSwRfb->pvHeader +
		ETHER_HEADER_LEN + ARP_SENDER_MAC_OFFSET);

#if CFG_HS20_DEBUG && 0
/* UINT_8  aucIpAllZero[4] = {0,0,0,0}; */
/* UINT_8  aucMACAllZero[MAC_ADDR_LEN] = {0,0,0,0,0,0}; */
	uint8_t *pucTargetMac = ((uint8_t *)
		prCurrSwRfb->pvHeader +
		ETHER_HEADER_LEN + ARP_TARGET_MAC_OFFSET);
#endif

#if CFG_HS20_DEBUG && 0
	uint16_t *pu2ArpOper = (uint16_t *) ((uint8_t *)
		prCurrSwRfb->pvHeader +
		ETHER_HEADER_LEN + ARP_OPERATION_OFFSET);

	kalPrint("Recv ARP 0x%04X\n", htons(*pu2ArpOper));
	kalPrint("SENDER[" MACSTR "] [%d:%d:%d:%d]\n",
		MAC2STR(pucSenderMac), *pucSenderIP,
		*(pucSenderIP + 1), *(pucSenderIP + 2), *(pucSenderIP + 3));
	kalPrint("TARGET[" MACSTR "] [%d:%d:%d:%d]\n",
		MAC2STR(pucTargetMac), *pucTargetIP,
		*(pucTargetIP + 1), *(pucTargetIP + 2), *(pucTargetIP + 3));
#endif

	/* IsGratuitousArp */
	if (!kalMemCmp(pucSenderIP, pucTargetIP, 4)) {
		kalPrint(
			"Drop Gratuitous ARP from [" MACSTR "] [%d:%d:%d:%d]\n",
			MAC2STR(pucSenderMac), *pucTargetIP, *(pucTargetIP + 1),
			*(pucTargetIP + 2), *(pucTargetIP + 3));
		return TRUE;
	}
	return FALSE;
}

u_int8_t hs20IsUnsolicitedNeighborAdv(struct ADAPTER *prAdapter,
		struct SW_RFB *prCurrSwRfb)
{
	uint8_t *pucIpv6Protocol = ((uint8_t *)
		prCurrSwRfb->pvHeader +
		ETHER_HEADER_LEN + IPV6_HDR_IP_PROTOCOL_OFFSET);

	/* kalPrint("pucIpv6Protocol [%02X:%02X]\n",
	 * *pucIpv6Protocol, IPV6_PROTOCOL_ICMPV6);
	 */
	if (*pucIpv6Protocol == IPV6_PROTOCOL_ICMPV6) {
		uint8_t *pucICMPv6Type =
		    ((uint8_t *) prCurrSwRfb->pvHeader +
		    ETHER_HEADER_LEN + IPV6_HDR_LEN + ICMPV6_TYPE_OFFSET);
		/* kalPrint("pucICMPv6Type [%02X:%02X]\n",
		 * *pucICMPv6Type, ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT);
		 */
		if (*pucICMPv6Type == ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT) {
			uint8_t *pucICMPv6Flag =
			    ((uint8_t *) prCurrSwRfb->pvHeader +
			    ETHER_HEADER_LEN +
			    IPV6_HDR_LEN + ICMPV6_FLAG_OFFSET);
			uint8_t *pucSrcMAC = ((uint8_t *)
				prCurrSwRfb->pvHeader + MAC_ADDR_LEN);

#if CFG_HS20_DEBUG
			kalPrint("NAdv Flag [%02X] [R(%d)\\S(%d)\\O(%d)]\n",
				*pucICMPv6Flag,
				(uint8_t) (*pucICMPv6Flag
					& ICMPV6_FLAG_ROUTER_BIT) >> 7,
				(uint8_t) (*pucICMPv6Flag
					& ICMPV6_FLAG_SOLICITED_BIT) >> 6,
				(uint8_t) (*pucICMPv6Flag
					& ICMPV6_FLAG_OVERWRITE_BIT) >> 5);
#endif
			if (!(*pucICMPv6Flag & ICMPV6_FLAG_SOLICITED_BIT)) {
				kalPrint(
					"Drop Unsolicited Neighbor Advertisement from ["
					MACSTR "]\n", MAC2STR(pucSrcMAC));
				return TRUE;
			}
		}
	}

	return FALSE;
}

u_int8_t hs20IsUnsecuredFrame(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct SW_RFB *prCurrSwRfb)
{
	uint16_t *pu2PktIpVer = (uint16_t *) ((uint8_t *)
		prCurrSwRfb->pvHeader + (ETHER_HEADER_LEN - ETHER_TYPE_LEN));

	/* kalPrint("IPVER 0x%4X\n", htons(*pu2PktIpVer)); */
#if CFG_HS20_DEBUG & 0
	uint8_t i = 0;

	kalPrint("===============================================");
	for (i = 0; i < 96; i++) {
		if (!(i % 16))
			kalPrint("\n");
		kalPrint("%02X ", *((uint8_t *) prCurrSwRfb->pvHeader + i));
	}
	kalPrint("\n");
#endif

	if (*pu2PktIpVer == htons(ETH_P_ARP))
		return hs20IsGratuitousArp(prAdapter, prCurrSwRfb);
	else if (*pu2PktIpVer == htons(ETH_P_IPV6))
		return hs20IsUnsolicitedNeighborAdv(prAdapter, prCurrSwRfb);

	return FALSE;
}

u_int8_t hs20IsFrameFilterEnabled(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
#if 1
	struct HS20_INFO *prHS20Info;

	prHS20Info = aisGetHS20Info(prAdapter,
		prBssInfo->ucBssIndex);

	if (prHS20Info && prHS20Info->fgConnectHS20AP)
		return TRUE;
#else
	struct PARAM_SSID rParamSsid;
	struct BSS_DESC *prBssDesc;

	rParamSsid.u4SsidLen = prBssInfo->ucSSIDLen;
	COPY_SSID(rParamSsid.aucSsid,
		rParamSsid.u4SsidLen, prBssInfo->aucSSID, prBssInfo->ucSSIDLen);

	prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
		prBssInfo->aucBSSID, TRUE, &rParamSsid);

	if (!prBssDesc)
		return FALSE;

	if (prBssDesc->fgIsSupportHS20) {
		if (!(prBssDesc->ucHotspotConfig
			& ELEM_HS_CONFIG_DGAF_DISABLED_MASK))
			return TRUE;
		/* Disable frame filter only if DGAF == 1 */
		return FALSE;
	}
#endif

	/* For Now, always return true to run hs20 check even for legacy AP */
	return TRUE;
}
#endif /* CFG_SUPPORT_PASSPOINT */
