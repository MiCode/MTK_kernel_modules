// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "mlo.c"
 *  \brief
 */

#include "precomp.h"
#include "gl_kal.h"
#if (CFG_SUPPORT_802_11BE_MLO == 1)

#define MAX_DUP_IE_COUNT 64

static void mldStarecUpdateMldId(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec);

uint8_t mldSanityCheck(struct ADAPTER *prAdapter, uint8_t *pucPacket,
	uint16_t u2PacketLen, struct STA_RECORD *prStaRec, uint8_t ucBssIndex)
{
	struct BSS_INFO *bss;
	struct STA_RECORD *starec;
	struct MLD_STA_RECORD *mld_starec;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct MULTI_LINK_INFO parse, *info = &parse;
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	const uint8_t *ml;
	struct LINK *links;
	uint16_t frame_ctrl;
	int32_t offset;
	uint8_t i;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!bss) {
		DBGLOG(ML, WARN, "Bss is NULL!\n");
		return FALSE;
	}

	if (!mldSingleLink(prAdapter, prStaRec, ucBssIndex))
		return TRUE;

	offset = sortGetPayloadOffset(prAdapter, pucPacket);
	if (offset < 0) {
		DBGLOG(ML, WARN, "can't get the payload offset\n");
		return FALSE;
	}

	mgmt = (struct WLAN_MAC_MGMT_HEADER *)(pucPacket);
	frame_ctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;

	kalMemSet(info, 0, sizeof(*info));
	/* look for ml ie from frame payload */
	ml = mldFindMlIE(pucPacket + offset,
		u2PacketLen - offset, ML_CTRL_TYPE_BASIC);
	if (ml)
		MLD_PARSE_BASIC_MLIE(info, ml,
			pucPacket + u2PacketLen - (uint8_t *)ml,
			bss->aucOwnMacAddr,
			frame_ctrl);

	if (IS_BSS_APGO(bss)) {
		/* ap mode, check auth/assoc req */
		mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
		mld_bssinfo = mldBssGetByBss(prAdapter, bss);
		if (mld_bssinfo) {
			links =  &mld_bssinfo->rBssList;

			/* reject if sta has unexpected link info */
			if (info->ucValid &&
			   ((frame_ctrl == MAC_FRAME_AUTH &&
			     info->ucProfNum != 0) ||
			   (frame_ctrl != MAC_FRAME_AUTH &&
			    info->ucProfNum > links->u4NumElem))) {
				DBGLOG(ML, ERROR,
					"STA wrong ML ie (valid=%d, num=%d)\n",
					info->ucValid,
					info->ucProfNum);
				return FALSE;
			}

			/* auth doesn't need to check link info */
			if (!info->ucValid || frame_ctrl == MAC_FRAME_AUTH)
				return TRUE;

			if (!prStaRec) {
				DBGLOG(ML, ERROR, "no starec\n");
				return FALSE;
			}

			if (!kalIsZeroEtherAddr(prStaRec->aucMldAddr) &&
			    UNEQUAL_MAC_ADDR(prStaRec->aucMldAddr,
					     info->aucMldAddr)) {
				DBGLOG(ML, ERROR,
					"STA wrong ML ie (addr=" MACSTR
					", num=%d) instead of " MACSTR "\n",
					MAC2STR(info->aucMldAddr),
					info->ucProfNum,
					MAC2STR(prStaRec->aucMldAddr));
				return FALSE;
			}

			/* link not matched, skip it */
			for (i = 0; i < info->ucProfNum; i++) {
				struct STA_PROFILE *profile =
					&info->rStaProfiles[i];
				uint8_t count = 0;
				uint8_t found = FALSE;

				LINK_FOR_EACH_ENTRY(bss, links,
					rLinkEntryMld, struct BSS_INFO) {
					if (count >= MLD_LINK_MAX) {
						DBGLOG(ML, ERROR,
							"too many links!!!\n");
						return FALSE;
					}

					if (profile->ucLinkId ==
					    bss->ucLinkIndex) {
						found = TRUE;
						break;
					}
					count++;
				}
				if (!found || !profile->ucComplete) {
					DBGLOG(ML, ERROR,
					   "STA wrong link (id=%d, addr=" MACSTR
					   ", complete=%d)\n",
					   profile->ucLinkId,
					   MAC2STR(profile->aucLinkAddr),
					   profile->ucComplete);
					return FALSE;
				}
			}
		} else if (ml) {
			DBGLOG(ML, ERROR, "STA should not have ML ie\n");
			return FALSE;
		}
	} else {
		/* sta mode, check auth/assoc resp */
		mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
		if (mld_starec) {
			links =  &mld_starec->rStarecList;

			/* auth is handled in supplicant */
			if (frame_ctrl == MAC_FRAME_AUTH &&
			    prStaRec->eAuthAssocState ==
				SAA_STATE_EXTERNAL_AUTH)
				return TRUE;

			/* sta already send ml ie, expected ml ie in resp */
			if (!info->ucValid ||
			   (frame_ctrl == MAC_FRAME_AUTH &&
			    info->ucProfNum > 0) ||
			   (frame_ctrl != MAC_FRAME_AUTH &&
			    info->ucProfNum > links->u4NumElem) ||
			   UNEQUAL_MAC_ADDR(info->aucMldAddr,
				mld_starec->aucPeerMldAddr)) {
				DBGLOG(ML, ERROR,
					"AP wrong ML ie (addr=" MACSTR
					", valid=%d, num=%d)\n",
					MAC2STR(info->aucMldAddr),
					info->ucValid,
					info->ucProfNum);
				return FALSE;
			}

			/* link not matched, skip it */
			for (i = 0; i < info->ucProfNum; i++) {
				struct STA_PROFILE *profile =
					&info->rStaProfiles[i];
				uint8_t count = 0;
				uint8_t found = FALSE;

				LINK_FOR_EACH_ENTRY(starec, links,
					rLinkEntryMld, struct STA_RECORD) {
					if (count >= MLD_LINK_MAX) {
						DBGLOG(ML, ERROR,
							"too many links!!!\n");
						return FALSE;
					}

					if (profile->ucLinkId ==
						starec->ucLinkIndex &&
					    EQUAL_MAC_ADDR(profile->aucLinkAddr,
						starec->aucMacAddr)) {
						found = TRUE;
						break;
					}
					count++;
				}
				if (!found || !profile->ucComplete) {
					DBGLOG(ML, ERROR,
					   "AP Wrong link (id=%d, addr=" MACSTR
					   "complete=%d)\n",
					   profile->ucLinkId,
					   MAC2STR(profile->aucLinkAddr),
					   profile->ucComplete);
					return FALSE;
				}
				starec->u2StatusCode = profile->u2StatusCode;
			}
		} else if (ml) {
			DBGLOG(ML, ERROR, "AP should not reply ML ie\n");
			return FALSE;
		}
	}

	return TRUE;
}

uint32_t mldCalculateMlIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	/* header: 2, extid: 1, common control: 2, common info: 12
	 * (len:1, mac:6, link id:1, bss change count: 1, mld cap: 2, mld id: 1)
	 */
	return 17;
}

void mldGenerateMlIEImpl(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct MLD_STA_RECORD *mld_starec;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct STA_RECORD *sta;
	struct BSS_INFO *bss;
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	uint8_t ucBssIndex;
	uint16_t frame_ctrl;

	ucBssIndex = prMsduInfo->ucBssIndex;
	sta = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);
	bss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	mld_starec = mldStarecGetByStarec(prAdapter, sta);
	mld_bssinfo = mldBssGetByBss(prAdapter, bss);
	mgmt = (struct WLAN_MAC_MGMT_HEADER *)(prMsduInfo->prPacket);
	frame_ctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;

	if (!bss) {
		DBGLOG(ML, WARN, "Bss is NULL!\n");
		return;
	}

	switch (frame_ctrl) {
	case MAC_FRAME_PROBE_RSP:
	case MAC_FRAME_BEACON:
		if (mldSingleLink(prAdapter, sta, ucBssIndex))
			mldGenerateBasicCommonInfo(prAdapter,
				prMsduInfo, frame_ctrl);
		break;
	case MAC_FRAME_AUTH: {
		struct WLAN_AUTH_FRAME *auth = (struct WLAN_AUTH_FRAME *)mgmt;
		uint16_t seq = auth->u2AuthTransSeqNo;

		/* for AP, mld_starec doesn't exist when auth */
		if (IS_BSS_APGO(bss)) {
			if (sta && !kalIsZeroEtherAddr(sta->aucMldAddr)) {
				DBGLOG(ML, INFO,
					"Start MLO (TranSeq: %d)", seq);
				mldGenerateBasicCommonInfo(prAdapter,
					prMsduInfo, frame_ctrl);
			} else {
				/* NOTE: for wpa3, driver bypass ml ie parsing,
				 * so driver doesn't build mlo and let hostapd
				 * handle it.
				 */
				DBGLOG(ML, INFO,
					"No MLO (TranSeq: %d)", seq);
			}
		} else {
			if (mld_starec) {
				DBGLOG(ML, INFO,
					"Start MLO (TranSeq: %d) linkNum=%d",
					seq, mld_starec->rStarecList.u4NumElem);
				mldGenerateBasicCommonInfo(prAdapter,
					prMsduInfo, frame_ctrl);
			} else {
				DBGLOG(ML, INFO,
					"No MLO (TranSeq: %d)", seq);
			}
		}
	}
		break;
	default:
		DBGLOG(ML, INFO,  "invalid frame_ctrl=%d", frame_ctrl);
		break;
	}
}

void mldGenerateMlIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{

	/* pre-wifi7 device, build in vendor id */
	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_HIDE_INFO)
		return;

	mldGenerateMlIEImpl(prAdapter, prMsduInfo);
}

void mldGenerateAssocIE(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct MSDU_INFO *prMsduInfo,
	PFN_COMPOSE_ASSOC_IE_FUNC pfnComposeIE)
{
	uint8_t *common = NULL, *cur = NULL;
	struct BSS_INFO *bss;
	struct STA_RECORD *starec;
	struct MSDU_INFO *msdu_sta;
	struct MLD_STA_RECORD *mld_starec;
	struct LINK *links;
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	uint16_t frame_ctrl;
	uint32_t len;
	int32_t offset, offset_sta;
	uint8_t count = 0;
	const uint8_t *eht;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
	offset = sortMsduPayloadOffset(prAdapter, prMsduInfo);
	if (offset < 0) {
		DBGLOG(ML, WARN, "Unknown packet\n");
		return;
	}

	len = prMsduInfo->u2FrameLength;
	mgmt = (struct WLAN_MAC_MGMT_HEADER *)(prMsduInfo->prPacket);
	frame_ctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;
	bss = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);

	if (!bss) {
		DBGLOG(ML, WARN, "Bss is NULL!\n");
		return;
	}

	eht = kalFindIeExtIE(ELEM_ID_RESERVED, ELEM_EXT_ID_EHT_CAPS,
		(uint8_t *)prMsduInfo->prPacket + offset, len  - offset);
	if (!eht) {
		DBGLOG(ML, INFO, "Non eht");
		return;
	}

	if (mld_starec) {
		cur = common = mldGenerateBasicCommonInfo(
			prAdapter, prMsduInfo, frame_ctrl);
	} else {
		DBGLOG(ML, INFO, "No MLO (%sAssoc%s)",
			(frame_ctrl & 0x20) ? "Re" : "",
			(frame_ctrl & 0x10) ? "Resp" : "Req");
		goto done;
	}

	links = &mld_starec->rStarecList;

	DBGLOG(ML, INFO, "Start MLO (%sAssoc%s) linkNum=%d",
		(frame_ctrl & 0x20) ? "Re" : "",
		(frame_ctrl & 0x10) ? "Resp" : "Req",
		links->u4NumElem);

	LINK_FOR_EACH_ENTRY(starec, links, rLinkEntryMld,
		struct STA_RECORD) {
		bss = GET_BSS_INFO_BY_INDEX(prAdapter, starec->ucBssIndex);
		if (!bss) {
			DBGLOG(ML, ERROR, "bss is NULL!!!\n");
			goto done;
		}

		if (count >= MLD_LINK_MAX) {
			DBGLOG(ML, ERROR, "too many links!!!\n");
			goto done;
		}

		DBGLOG(ML, INFO,
			"\tsta%s: %d, wlan_idx: %d, bss_idx: %d, mac: "
			MACSTR "\n",
			starec == prStaRec ? "(main)" : "",
			starec->ucIndex,
			starec->ucWlanIndex,
			starec->ucBssIndex,
			MAC2STR(starec->aucMacAddr));

		if (starec != prStaRec) {
			/* compose frame for each starec */
			msdu_sta = pfnComposeIE(prAdapter, starec);
			if (msdu_sta == NULL) {
				DBGLOG(ML, WARN,
					"No PKT_INFO_T for sending MLD STA.\n");
				continue;
			}

			offset_sta = sortMsduPayloadOffset(prAdapter, msdu_sta);
			if (offset_sta < 0) {
				DBGLOG(ML, WARN, "Unknown packet\n");
				return;
			}
			if (offset_sta != offset) {
				DBGLOG(ML, WARN,
					"Payload offset = %d, expected = %d\n",
					offset_sta, offset);
				cnmMgtPktFree(prAdapter, msdu_sta);
				continue;
			}

			cur = mldGenerateBasicCompleteProfile(prAdapter, cur,
				prMsduInfo, offset, len,
				msdu_sta, bss->ucBssIndex);
			cnmMgtPktFree(prAdapter, msdu_sta);
		}

		count++;
	}

done:
	if (common) {
		uint32_t tmp_len, total_len;
		uint8_t *tmp;

		/* ml ie is ahead of eht cap, copy rest of elements first,
		 * tmp buffer to save ies including ml ie.
		 */
		tmp_len = (uint8_t *)prMsduInfo->prPacket +
			   prMsduInfo->u2FrameLength - eht;
		tmp = kalMemAlloc(tmp_len, VIR_MEM_TYPE);
		if (!tmp) {
			DBGLOG(ML, WARN, "No resource");
			return;
		}

		total_len = IE_SIZE(common);
		total_len += (cur == common) ? 0 : IE_SIZE(cur);
		kalMemCopy(tmp, common, total_len);
		kalMemCopy(tmp + total_len, eht, tmp_len - total_len);

		DBGLOG(ML, INFO, "Dump total MLIE\n");
		DBGLOG_MEM8(ML, INFO, tmp, total_len);

		/* copy back to msdu */
		kalMemCopy((uint8_t *)eht, tmp, tmp_len);
		kalMemFree(tmp, VIR_MEM_TYPE, tmp_len);
	}
}

void mldGenerateProbeRspIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo, uint8_t ucBssIdx,
			   struct WLAN_BEACON_FRAME *prProbeRspFrame,
			   PFN_COMPOSE_PROBE_RESP_IE_FUNC pfnComposeIE)
{
	struct MSDU_INFO *msdu_sta;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct LINK *links;
	struct BSS_INFO *bss;
	uint32_t len;
	int32_t offset, offset_sta;
	uint8_t count = 0, *common = NULL, *cur = NULL;
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	uint16_t frame_ctrl;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	mld_bssinfo = mldBssGetByBss(prAdapter, bss);
	offset = sortMsduPayloadOffset(prAdapter, prMsduInfo);
	if (offset < 0) {
		DBGLOG(ML, WARN, "Unknown packet\n");
		return;
	}

	len = prMsduInfo->u2FrameLength;
	mgmt = (struct WLAN_MAC_MGMT_HEADER *)(prMsduInfo->prPacket);
	frame_ctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;

	if (prMsduInfo->ucControlFlag & MSDU_CONTROL_FLAG_HIDE_INFO)
		return;

	if (mldSingleLink(prAdapter, NULL, ucBssIdx)) {
		cur = common = mldGenerateBasicCommonInfo(
			prAdapter, prMsduInfo, frame_ctrl);
	}

	if (!common || !mld_bssinfo)
		return;

	links = &mld_bssinfo->rBssList;
	LINK_FOR_EACH_ENTRY(bss, links, rLinkEntryMld, struct BSS_INFO) {
		if (count >= MLD_LINK_MAX) {
			DBGLOG(ML, ERROR, "too many links!!!\n");
			return;
		}
		DBGLOG(ML, INFO,
			"bss%s: id: %d, mac: " MACSTR "\n",
			bss->ucBssIndex == ucBssIdx ? "(main)" : "",
			bss->ucBssIndex,
			MAC2STR(bss->aucOwnMacAddr));

		if (bss->ucBssIndex != ucBssIdx) {
			/* compose frame for another bss */
			msdu_sta = pfnComposeIE(prAdapter, bss->ucBssIndex,
				TRUE, FALSE, prProbeRspFrame);
			if (msdu_sta == NULL) {
				DBGLOG(ML, WARN,
					"No PKT_INFO_T for sending MLD STA.\n");
				continue;
			}

			offset_sta = sortMsduPayloadOffset(prAdapter, msdu_sta);
			if (offset_sta < 0) {
				DBGLOG(ML, WARN, "Unknown packet\n");
				return;
			}
			if (offset_sta != offset) {
				DBGLOG(ML, WARN,
					"Payload offset = %d, expected = %d\n",
					offset_sta, offset);
				cnmMgtPktFree(prAdapter, msdu_sta);
				continue;
			}

			cur = mldGenerateBasicCompleteProfile(prAdapter, cur,
				prMsduInfo, offset, len,
				msdu_sta, bss->ucBssIndex);
			cnmMgtPktFree(prAdapter, msdu_sta);
		}

		count++;
	}

	DBGLOG(ML, INFO, "Dump total MLIE\n");
	DBGLOG_MEM8(ML, INFO, common, IE_SIZE(common));
}

uint8_t *mldGenerateBasicCommonInfo(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2FrameCtrl)
{
	uint8_t *cp;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct BSS_INFO *bss;
	struct WIFI_VAR *prWifiVar;
	struct IE_MULTI_LINK_CONTROL *common;
	uint16_t present = 0;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	mld_bssinfo = mldBssGetByBss(prAdapter, bss);
	prWifiVar = &prAdapter->rWifiVar;

	if (!bss && !mld_bssinfo)
		return NULL;

	common = (struct IE_MULTI_LINK_CONTROL *)
		(((uint8_t *)prMsduInfo->prPacket) + prMsduInfo->u2FrameLength);

	common->ucId = ELEM_ID_RESERVED;
	common->ucExtId = ELEM_EXT_ID_MLD;

	/* filling control field */
	BE_SET_ML_CTRL_TYPE(common->u2Ctrl, ML_CTRL_TYPE_BASIC);

	/* A Basic Multi-Link element in an Authentication frame:
	 * the STA shall include the MLD MAC address of the MLD
	 * the STA shall set all subfields in the Presence Bitmap subfield of
	 * the Multi-Link Control field of the element to 0
	 */
	if (u2FrameCtrl != MAC_FRAME_AUTH) {
		present |= ML_CTRL_MLD_CAPA_PRESENT;

		if (mld_bssinfo && mld_bssinfo->ucEmlEnabled)
			present |= ML_CTRL_EML_CAPA_PRESENT;

		if (IS_BSS_APGO(bss)) {
			present |= (ML_CTRL_LINK_ID_INFO_PRESENT |
				    ML_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT);

			/* mld id for mbss only
			if (mld_bssinfo)
				present |= ML_CTRL_MLD_ID_PRESENT;
			*/
		}
	}

	BE_SET_ML_CTRL_PRESENCE(common->u2Ctrl, present);

	/* filling common info field*/
	cp = common->aucCommonInfo;
	cp++; /* reserve for common info length */

	if (mld_bssinfo)
		COPY_MAC_ADDR(cp, mld_bssinfo->aucOwnMldAddr);
	else
		COPY_MAC_ADDR(cp, bss->aucOwnMacAddr);

	DBGLOG(ML, TRACE, "\tML common Info MAC addr = "MACSTR"", MAC2STR(cp));
	cp += MAC_ADDR_LEN;

	if (BE_IS_ML_CTRL_PRESENCE_LINK_ID(common->u2Ctrl)) {
		DBGLOG(ML, TRACE, "\tML common Info LinkID = %d ("MACSTR")",
			bss->ucLinkIndex, MAC2STR(bss->aucOwnMacAddr));
		*cp++ = bss->ucLinkIndex;
	}
	if (BE_IS_ML_CTRL_PRESENCE_BSS_PARA_CHANGE_COUNT(common->u2Ctrl)) {
		DBGLOG(ML, TRACE,
			"\tML common Info BssParaChangeCount = %d", 0);
		*cp++ = 0;
	}
	if (BE_IS_ML_CTRL_PRESENCE_EML_CAP(common->u2Ctrl)) {
		if (mld_bssinfo) {
			WLAN_SET_FIELD_16(cp,
				mld_bssinfo->u2EMLCap);
		}
		DBGLOG(ML, TRACE, "\tML common Info EML capa = 0x%x",
			*(uint16_t *)cp);
		cp += 2;
	}
	if (BE_IS_ML_CTRL_PRESENCE_MLD_CAP(common->u2Ctrl)) {
		uint16_t mld_cap = 0;

		/* Set to the maximum number of affiliated STAs in the non-AP
		 * MLD that support simultaneous transmission or reception of
		 * frames minus 1. For an AP MLD, set to the number of
		 * affiliated APs minus 1
		 * According to Table 9-401i in 802.11be D3.0. Set the
		 * TID-to-link mapping negotiation to
		 * 0: TID-to-link mapping is not supported
		 * 1: MLD supports all TIDs to the same link set, both UL and
		 *     DL.
		 * 2: reserved.
		 * 3: MLD supports each TID to the same or different link set.
		 */
		if (mld_bssinfo) {
			BE_SET_MLD_CAP_MAX_SIMULTANEOUS_LINKS(mld_cap,
				mld_bssinfo->ucMaxSimuLinks);
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
			/* SAP not support T2lm currently*/
			if (IS_BSS_APGO(bss)) {
				BE_SET_MLD_CAP_TID_TO_LINK_NEGO(mld_cap,
					T2LM_NO_SUPPORT);
			} else {
				BE_SET_MLD_CAP_TID_TO_LINK_NEGO(mld_cap,
					prWifiVar->ucT2LMNegotiationSupport);
			}
#endif
		} else if (bss) {
			BE_SET_MLD_CAP_MAX_SIMULTANEOUS_LINKS(mld_cap, 0);
			BE_SET_MLD_CAP_TID_TO_LINK_NEGO(mld_cap,
				T2LM_NO_SUPPORT);
		}
		WLAN_SET_FIELD_16(cp, mld_cap);
		DBGLOG(ML, TRACE, "\tML common Info MLD capa = 0x%x",
			*(uint16_t *)cp);
		cp += 2;
	}
	if (BE_IS_ML_CTRL_PRESENCE_MLD_ID(common->u2Ctrl)) {
		DBGLOG(ML, TRACE, "\tML common Info MLD ID = %d", 0);
		*cp++ = 0;
	}

	/* update common info length, ie length, frame length */
	*common->aucCommonInfo = cp - common->aucCommonInfo;
	common->ucLength = cp - (uint8_t *) common - ELEM_HDR_LEN;
	prMsduInfo->u2FrameLength += IE_SIZE(common);

	DBGLOG(ML, LOUD, "Bss%d dump ML common IE\n", bss->ucBssIndex);
	DBGLOG_MEM8(ML, LOUD, common, common->ucLength + ELEM_HDR_LEN);


	return (uint8_t *)common;
}

void mldHandleRnrMlParam(struct IE_RNR *rnr,
	struct MULTI_LINK_INFO *prMlInfo, uint8_t fgOverride,
	uint16_t u2PayloadLength)
{
	uint8_t i, j, band;
	uint8_t *pos = NULL;
	uint8_t ucMldParamOffset, ucMldId, ucMldLinkId, ucBssParamChangeCount;
	uint16_t u2TbttInfoCount, u2TbttInfoLength;
	uint32_t u4MldParam = 0;
	struct STA_PROFILE *prProfile = NULL;

	pos = rnr->aucInfoField;
	do {
		struct NEIGHBOR_AP_INFO_FIELD *prNeighborAPInfoField =
			(struct NEIGHBOR_AP_INFO_FIELD *)pos;

		/* get channel number for this neighborAPInfo */
		scanOpClassToBand(prNeighborAPInfoField->ucOpClass, &band);
		u2TbttInfoCount = ((prNeighborAPInfoField->u2TbttInfoHdr &
					TBTT_INFO_HDR_COUNT)
					>> TBTT_INFO_HDR_COUNT_OFFSET)
					+ 1;
		u2TbttInfoLength = (prNeighborAPInfoField->u2TbttInfoHdr &
					TBTT_INFO_HDR_LENGTH)
					>> TBTT_INFO_HDR_LENGTH_OFFSET;

		DBGLOG(ML, LOUD, "dump RNR AP info field\n");

		/* [FUZZ] TBTT information length should
		 *        smaller than SW_RFB payload.
		 */
		if (u2PayloadLength != 0
			&& ((4 + u2TbttInfoCount * u2TbttInfoLength)
			> u2PayloadLength)) {
			DBGLOG(ML, WARN,
				"TBTT information(%u), RNR IE(%u)\n",
				(4 + u2TbttInfoCount * u2TbttInfoLength),
				IE_SIZE(rnr));
			goto Next_NeighborAPInfoFileds;
		}

		DBGLOG_MEM8(ML, LOUD, pos,
				4 + u2TbttInfoCount * u2TbttInfoLength);

		for (i = 0; i < u2TbttInfoCount; i++) {
			j = i * u2TbttInfoLength;

			/* 10: Neighbor AP TBTT Offset + BSSID + MLD Para */
			if (u2TbttInfoLength == 10) {
				ucMldParamOffset = 7;
			} else if (u2TbttInfoLength >= 16 &&
			  u2TbttInfoLength <= 255) {
			/* 16: Neighbor AP TBTT Offset + BSSID + Short SSID +
			 * BSS parameters + 20MHz PSD + MLD Parameter
			 */
				ucMldParamOffset = 13;
			} else {
			/* only handle neighbor AP info that MLD parameter
			 * and BSSID both exist
			 */
				continue;
			}

			DBGLOG(ML, LOUD, "RnrIe[%x][" MACSTR "]\n", i,
			  MAC2STR(&prNeighborAPInfoField->
			  aucTbttInfoSet[j + 1]));

			/* Directly copy 4 bytes content, but MLD param is only
			 * 3 bytes actually. We will only use 3 bytes content.
			 */
			kalMemCopy(&u4MldParam, &prNeighborAPInfoField->
				aucTbttInfoSet[j + ucMldParamOffset],
				sizeof(u4MldParam));
			ucMldId = (u4MldParam & MLD_PARAM_MLD_ID_MASK);
			ucMldLinkId = (u4MldParam & MLD_PARAM_LINK_ID_MASK) >>
				MLD_PARAM_LINK_ID_SHIFT;
			ucBssParamChangeCount =
			  (u4MldParam &
				MLD_PARAM_BSS_PARAM_CHANGE_COUNT_MASK) >>
			  MLD_PARAM_BSS_PARAM_CHANGE_COUNT_SHIFT;

			DBGLOG(ML, TRACE,
				"MldId=%d, MldLinkId=%d, BssParChangeCount=%d\n",
				ucMldId, ucMldLinkId, ucBssParamChangeCount);

			if (ucMldId != prMlInfo->ucMldId)
				continue;

			if (!fgOverride) {
				for (j = 0; j < prMlInfo->ucProfNum; j++) {
					prProfile = &prMlInfo->rStaProfiles[j];
					if (prProfile->ucLinkId == ucMldLinkId)
						break;
				}
				if (j >= prMlInfo->ucProfNum) {
					DBGLOG(ML, WARN, "invalid link%d",
						ucMldLinkId);
					continue;
				}
			} else {
				if (prMlInfo->ucProfNum >= MLD_LINK_MAX) {
					DBGLOG(ML, WARN,
						"no space for link_id: %d",
						ucMldLinkId);
					continue;
				}
				prProfile = &prMlInfo->
				  rStaProfiles[prMlInfo->ucProfNum++];
				prProfile->ucLinkId = ucMldLinkId;
			}
			prProfile->rChnlInfo.eBand = band;
			prProfile->rChnlInfo.ucChannelNum =
				prNeighborAPInfoField->ucChannelNum;

			prProfile->rChnlInfo.ucChnlBw =
			  rlmOpClassToBandwidth(prNeighborAPInfoField->
			  ucOpClass);

			prProfile->rChnlInfo.u4CenterFreq1 = 0;
			prProfile->rChnlInfo.u4CenterFreq2 = 0;
			DBGLOG(ML, TRACE,
				"link_id:%d, op:%d, rfband:%d, ch:%d, bw:%d, s1:%d, s2:%d\n",
				prProfile->ucLinkId,
				prNeighborAPInfoField->ucOpClass,
				prProfile->rChnlInfo.eBand,
				prProfile->rChnlInfo.ucChannelNum,
				prProfile->rChnlInfo.ucChnlBw,
				prProfile->rChnlInfo.u4CenterFreq1,
				prProfile->rChnlInfo.u4CenterFreq2);
		}
Next_NeighborAPInfoFileds:
		pos += (4 + (u2TbttInfoCount * u2TbttInfoLength));
	} while (pos < ((uint8_t *)rnr) + IE_SIZE(rnr));
}

uint32_t mldGenerateMlProbeReqIE(struct BSS_DESC *prBssDesc, uint8_t *pucIE,
	uint32_t u4IELength, uint8_t fgPerSta, uint8_t ucMldId)
{
	uint16_t u2Offset = 0;
	uint8_t *ie;
	uint16_t ie_len;
	struct MULTI_LINK_INFO parse, *info = &parse;
	struct IE_RNR *rnr;
	uint8_t *pos;
	struct IE_MULTI_LINK_CONTROL *common;
	uint8_t i;

	/* parsing rnr & ml */
	kalMemSet(info, 0, sizeof(*info));
	info->ucMldId = ucMldId;

	if (fgPerSta) {
		ie = prBssDesc->pucIeBuf;
		ie_len = prBssDesc->u2IELength;
		IE_FOR_EACH(ie, ie_len, u2Offset) {
			if (IE_ID(ie) != ELEM_ID_RNR)
				continue;

			rnr = (struct IE_RNR *)ie;

			mldHandleRnrMlParam(rnr, info, TRUE, ie_len);
		}
	}

	if (u4IELength < 7 + info->ucProfNum * 4) {
		DBGLOG(ML, INFO, "no space for ml prob req link info\n");
		return 0;
	}

	common = (struct IE_MULTI_LINK_CONTROL *)pucIE;

	common->ucId = ELEM_ID_RESERVED;
	common->ucExtId = ELEM_EXT_ID_MLD;

	/* EID 1byte + ML Ctrl 2byte + CommonInfo 2byte */
	common->ucLength = 5;

	/* filling control field */
	BE_SET_ML_CTRL_TYPE(common->u2Ctrl, ML_CTRL_TYPE_PROBE_REQ);
	BE_SET_ML_CTRL_PRESENCE(common->u2Ctrl, ML_PRBREQ_CTRL_MLD_ID_PRESENT);

	/* Common Info Length = 2 */
	*common->aucCommonInfo = 2;
	/* Assign MLD ID*/
	*(common->aucCommonInfo + 1) = info->ucMldId;

	pos = common->aucCommonInfo + 2;
	for (i = 0; i < info->ucProfNum; i++) {
		struct IE_ML_STA_CONTROL *sta_ctrl;
		struct STA_PROFILE *prProfile = &info->rStaProfiles[i];

		if (prProfile->rChnlInfo.eBand == prBssDesc->eBand &&
		    prProfile->rChnlInfo.ucChannelNum ==
			prBssDesc->ucChannelNum)
			continue;

		sta_ctrl = (struct IE_ML_STA_CONTROL *)pos;
		/* Subelement ID 0: Per-STA Profile */
		sta_ctrl->ucSubID = 0;
		/* Length = 2, only contain 2bytes STA control, no STA prof */
		sta_ctrl->ucLength = 2;
		/* STA control, bits[0:3] is Link ID */

		BE_SET_ML_STA_CTRL_LINK_ID(sta_ctrl->u2StaCtrl,
			prProfile->ucLinkId);
		/* STA control, bit[4] is Complete Profile flag */
		BE_SET_ML_STA_CTRL_COMPLETE_PROFILE(sta_ctrl->u2StaCtrl);

		common->ucLength += IE_SIZE(sta_ctrl);
		pos += IE_SIZE(sta_ctrl);
	}

	DBGLOG(ML, INFO, "Dump ML probe req IE\n");
	DBGLOG_MEM8(ML, INFO, common, IE_SIZE(common));

	return IE_SIZE(common);
}

uint32_t mldFillScanIE(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc,
	uint8_t *pucIE, uint32_t u4IELength, uint8_t fgPerSta, uint8_t ucMldId)
{
	uint16_t len = 0;

	len += mldGenerateMlProbeReqIE(prBssDesc, pucIE,
		u4IELength, fgPerSta, ucMldId);

	return len;
}

uint8_t mldDupProfileSkipIE(uint8_t *pucBuf)
{
	return IE_ID(pucBuf) == ELEM_ID_MBSSID ||
	       IE_ID(pucBuf) == ELEM_ID_MBSSID_INDEX ||
	       BE_IS_ML_CTRL_TYPE(pucBuf, ML_CTRL_TYPE_BASIC);
}

uint8_t mldDupStaProfileSkipIE(uint8_t *pucBuf)
{
	/* 80211be d2.3, 35.3.3.4*/
	return IE_ID(pucBuf) == ELEM_ID_SSID ||
	       IE_ID(pucBuf) == ELEM_ID_TIM ||
	       IE_ID(pucBuf) == ELEM_ID_BSS_MAX_IDLE_PERIOD ||
	       IE_ID(pucBuf) == ELEM_ID_RNR ||
	       IE_ID(pucBuf) == ELEM_ID_NEIGHBOR_REPORT ||
	       IE_ID(pucBuf) == ELEM_ID_MBSSID ||
	       IE_ID(pucBuf) == ELEM_ID_MBSSID_INDEX ||
	       (IE_ID(pucBuf) == ELEM_ID_RESERVED &&
		IE_ID_EXT(pucBuf) == ELEM_EXT_ID_MLD) ||
	       (IE_ID(pucBuf) == ELEM_ID_RESERVED &&
		IE_ID_EXT(pucBuf) == ELEM_EXT_ID_MBSS_CONFIG);
}

void mldDumpIE(uint8_t *pucBuf, uint16_t u2IEsBufLen, uint8_t *pucDesc)
{
	uint16_t u2Offset = 0;

	if (pucDesc)
		DBGLOG(ML, LOUD, "%s  === BEGIN ===\n", pucDesc);

	IE_FOR_EACH(pucBuf, u2IEsBufLen, u2Offset) {
		DBGLOG(ML, LOUD, "IE(%d, %d) size=%d",
			IE_ID(pucBuf), IE_ID(pucBuf) == ELEM_ID_RESERVED ?
			IE_ID_EXT(pucBuf) : 0, IE_SIZE(pucBuf));
		DBGLOG_MEM8(ML, LOUD, pucBuf, IE_SIZE(pucBuf));
	}

	if (pucDesc)
		DBGLOG(ML, LOUD, "%s  === END ===\n", pucDesc);
}

uint8_t mldIsValidForCompleteProfile(
	uint16_t fctrl,
	struct STA_RECORD *starec)
{
	/* Only Management frames belonging to subtypes (Re)Association Request,
	 * (Re)Association Response, or Probe Response that is an ML probe
	 * response can carry complete profile of a reported STA
	 */
	if (fctrl != MAC_FRAME_ASSOC_REQ &&
	    fctrl != MAC_FRAME_ASSOC_RSP &&
	    fctrl != MAC_FRAME_REASSOC_REQ &&
	    fctrl != MAC_FRAME_REASSOC_RSP &&
	    fctrl != MAC_FRAME_PROBE_RSP) {
		DBGLOG(ML, WARN,
			"frame_ctrl=%x not allowed to carry sta profile\n",
			fctrl);
		return FALSE;
	}

	if (fctrl != MAC_FRAME_PROBE_RSP) {
		if (!starec) {
			DBGLOG(ML, WARN,
				"frame_ctrl=%x without starec\n", fctrl);
			return FALSE;
		}
	}

	return TRUE;
}

uint8_t *mldInsertFragmentHdr(uint8_t eid, uint8_t *start, uint8_t *end)
{
	uint8_t *tmp;
	uint32_t tmp_len;

	/* backup original content */
	tmp_len = end - start;
	tmp = kalMemAlloc(tmp_len, VIR_MEM_TYPE);
	if (!tmp) {
		DBGLOG(ML, WARN, "no resource for fragment %d", eid);
		return start;
	}

	kalMemCopy(tmp, start, tmp_len);

	start[0] = eid;
	start[1] = tmp_len;
	kalMemCopy(&start[2], tmp, tmp_len);

	kalMemFree(tmp, VIR_MEM_TYPE, tmp_len);

	DBGLOG(ML, LOUD, "build fragment");
	DBGLOG_MEM8(ML, LOUD, start, IE_SIZE(start));

	return start;
}

uint32_t mldProfileCopyIe(struct MSDU_INFO *prMsduInfo,
	uint8_t **prContainer, uint8_t **prSta, uint8_t **prFragment,
	uint8_t **prPos, uint8_t *prTarget)
{
	uint8_t *ie = *prContainer, *sta = *prSta, *frag = *prFragment;
	uint8_t *cp = *prPos, *pos, *buf = prTarget;
	uint16_t ie_len_sum = 0, sta_len_sum = 0;

	ie_len_sum = IE_LEN(ie) + IE_SIZE(buf);
	sta_len_sum = IE_LEN(sta) + IE_SIZE(buf);

	pos = cp;

	if (ie_len_sum > 255 &&
	    sta_len_sum > 255) {
		if (IE_ID(ie) == ELEM_ID_FRAGMENT) {
			DBGLOG(ML, WARN, "no space");
			return WLAN_STATUS_RESOURCES;
		}

		/* primary not found, copy it */
		kalMemCopy(cp, buf, IE_SIZE(buf));
		cp += IE_SIZE(buf);

		IE_LEN(sta) = 255;
		/* insert sub fragment hdr */
		sta = mldInsertFragmentHdr(SUB_IE_MLD_FRAGMENT,
					   IE_TAIL(sta), cp);
		cp += ELEM_HDR_LEN;

		IE_LEN(ie) = 255;
		/* insert fragment hdr */
		ie = mldInsertFragmentHdr(ELEM_ID_FRAGMENT,
					  IE_TAIL(ie), cp);
		cp += ELEM_HDR_LEN;
		frag = ie;

		/* if frag hdr ahead sta, offset sta */
		if (frag <= sta)
			sta += ELEM_HDR_LEN;
	} else if (ie_len_sum > 255) {
		if (IE_ID(ie) == ELEM_ID_FRAGMENT) {
			DBGLOG(ML, WARN, "no space");
			return WLAN_STATUS_RESOURCES;
		}

		/* primary not found, copy it */
		kalMemCopy(cp, buf, IE_SIZE(buf));
		cp += IE_SIZE(buf);

		IE_LEN(ie) = 255;
		/* insert fragment hdr */
		ie = mldInsertFragmentHdr(ELEM_ID_FRAGMENT,
					  IE_TAIL(ie), cp);
		cp += ELEM_HDR_LEN;
		frag = ie;

		/* sta len exclude fragment hdr */
		IE_LEN(sta) += cp - pos - ELEM_HDR_LEN;
	} else if (sta_len_sum > 255) {
		DBGLOG(ML, WARN, "impossible");
	} else {
		/* primary not found, copy it */
		kalMemCopy(cp, buf, IE_SIZE(buf));
		cp += IE_SIZE(buf);

		/* update ie & msdu len */
		IE_LEN(sta) += cp - pos;
		IE_LEN(ie) += cp - pos;
	}

	/* update input */
	*prContainer = ie;
	*prSta = sta;
	*prFragment = frag;
	*prPos = cp;
	prMsduInfo->u2FrameLength += cp - pos;

	return WLAN_STATUS_SUCCESS;
}

uint8_t *mldGenerateBasicCompleteProfile(
	struct ADAPTER *prAdapter,
	uint8_t *prIe,
	struct MSDU_INFO *prMsduInfo,
	uint32_t u4BeginOffset,
	uint32_t u4PrimaryLength,
	struct MSDU_INFO *prMsduInfoSta,
	uint8_t ucBssIndex)
{
	struct IE_ML_STA_CONTROL *sta_ctrl;
	struct IE_NON_INHERITANCE *non_inh;
	struct STA_RECORD *starec;
	struct BSS_INFO *bss;
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	uint8_t i, link, *cp, *pucBuf, *sta, *pos, *frag = NULL;
	uint16_t fctrl, control = 0, cap = 0, u2Offset = 0, u2IEsBufLen;
	const uint8_t *primary, *start, *end;
	uint8_t neid_arr[ELEM_ID_MAX_NUM], neid = 0;
	uint8_t nexid_arr[ELEM_ID_MAX_NUM], nexid = 0;
	uint32_t status;

	if (!prIe) {
		DBGLOG(ML, WARN, "No ie to compose");
		return NULL;
	}

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!bss) {
		DBGLOG(ML, WARN, "Bss is NULL!");
		return NULL;
	}

	starec = cnmGetStaRecByIndex(prAdapter, prMsduInfoSta->ucStaRecIndex);
	pos = (uint8_t *) prMsduInfo->prPacket + prMsduInfo->u2FrameLength;
	sta = pos;
	sta_ctrl = (struct IE_ML_STA_CONTROL *) pos;
	link = starec ? starec->ucLinkIndex : bss->ucLinkIndex;
	mgmt = (struct WLAN_MAC_MGMT_HEADER *)(prMsduInfo->prPacket);
	fctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;

	if (!mldIsValidForCompleteProfile(fctrl, starec))
		return NULL;

	control |= ML_STA_CTRL_COMPLETE_PROFILE;

	/* A non-AP STA sets the Beacon Interval//DTIM Info Present subfield
	 * to 0. An AP sets this subfield to 1 when the element carries
	 * complete profile.
	 */
	if (IS_BSS_APGO(bss))
		control |= (ML_STA_CTRL_BCN_INTV_PRESENT |
			    ML_STA_CTRL_DTIM_INFO_PRESENT |
			    ML_STA_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT);

	sta_ctrl->ucSubID = SUB_IE_MLD_PER_STA_PROFILE;
	sta_ctrl->ucLength = 0;

	/* filling STA control field (fixed length) */
	BE_SET_ML_STA_CTRL_LINK_ID(control, link);

	/*
	 * meaningful if NSTR Link Pair Present is 1
	 * Bitmap subfield: 0 = 1 octet, 1 = 2 octets
	 */
	control |= (ML_STA_CTRL_MAC_ADDR_PRESENT);

	BE_SET_ML_STA_CTRL_FIELD(sta_ctrl->u2StaCtrl, control);

	/* filling STA info field (varied length) */
	cp = sta_ctrl->aucStaInfo;
	cp++; /* reserved for sta info length */

	DBGLOG(ML, TRACE, "\tLinkID=%d Ctrl=0x%x(COMPLETE)", link, control);

	if (BE_IS_ML_STA_CTRL_PRESENCE_MAC(sta_ctrl->u2StaCtrl)) {
		DBGLOG(ML, TRACE, "\tLinkID=%d, LinkAddr="MACSTR"",
			link, MAC2STR(bss->aucOwnMacAddr));
		COPY_MAC_ADDR(cp, bss->aucOwnMacAddr);
		cp += MAC_ADDR_LEN;
	}

	if (BE_IS_ML_STA_CTRL_PRESENCE_BCN_INTV(sta_ctrl->u2StaCtrl)) {
		DBGLOG(ML, TRACE, "\tLinkID=%d, BCN_INTV = %d",
			link, bss->u2BeaconInterval);
		WLAN_SET_FIELD_16(cp, bss->u2BeaconInterval);
		cp += 2;
	}

	if (BE_IS_ML_STA_CTRL_PRESENCE_DTIM(sta_ctrl->u2StaCtrl)) {
		DBGLOG(ML, TRACE, "\tLinkID=%d, DTIM_INFO = 0x%x%x",
			link, bss->ucDTIMCount, bss->ucDTIMPeriod);
		*cp++ = bss->ucDTIMCount;
		*cp++ = bss->ucDTIMPeriod;
	}

	if (BE_IS_ML_STA_CTRL_PRESENCE_BSS_PARA_CHANGE_COUNT(
		sta_ctrl->u2StaCtrl)) {
		DBGLOG(ML, TRACE, "\tLinkID=%d, BSS_PARA_CHANGE_COUNT = 0x%x",
			link, 0x0);
		*cp++ = 0x0;
	}

	if (BE_IS_ML_STA_CTRL_PRESENCE_NSTR(sta_ctrl->u2StaCtrl)) {
		DBGLOG(ML, TRACE, "\tLinkID=%d, NSTR=0x%x", link, 0);
		/* 1 octet for nstr bmp */
		*cp++ = 0;
	}

	/* upadte sta info len */
	*sta_ctrl->aucStaInfo = cp - sta_ctrl->aucStaInfo;

	/* update ie & msdu len */
	IE_LEN(sta) = cp - pos - ELEM_HDR_LEN;
	IE_LEN(prIe) += cp - pos;
	prMsduInfo->u2FrameLength += cp - pos;
	pos = cp;

	/* PER-STA profile carry field(s) & ie(s) */

	/* Start to fill the Capability Information field. */
	if (fctrl == MAC_FRAME_PROBE_RSP)
		cap = bss->u2CapInfo;
	else
		cap = assocBuildCapabilityInfo(prAdapter, starec);
	DBGLOG(ML, TRACE, "\tLinkID=%d, CAP_INFO = 0x%x", link, cap);
	WLAN_SET_FIELD_16(cp, cap);
	cp += 2;

	/* Fill the Status Code field */
	if (fctrl == MAC_FRAME_ASSOC_RSP || fctrl == MAC_FRAME_REASSOC_RSP) {
		if (starec) {
			DBGLOG(ML, TRACE, "\tLinkID=%d, Status = 0x%x",
					link, starec->u2StatusCode);
			/* Fill the Status Code field. */
			WLAN_SET_FIELD_16(cp, starec->u2StatusCode);
			cp += 2;
		} else {
			DBGLOG(ML, WARN, "Starec is NULL!");
		}
	}

	/* update ie & msdu len */
	IE_LEN(sta) += cp - pos;
	IE_LEN(prIe) += cp - pos;
	prMsduInfo->u2FrameLength += cp - pos;
	pos = cp;

	/* primary can skip filling ie info because it inherits all */
	if (prMsduInfoSta == prMsduInfo)
		goto done;

	/* handle inheritance ie */
	start = (uint8_t *) prMsduInfo->prPacket + u4BeginOffset;
	end = (uint8_t *) prMsduInfo->prPacket + u4PrimaryLength;
	pucBuf = (uint8_t *)prMsduInfoSta->prPacket + u4BeginOffset;
	u2IEsBufLen = prMsduInfoSta->u2FrameLength - u4BeginOffset;

	mldDumpIE((uint8_t *)start, u4PrimaryLength - u4BeginOffset, "Primary");
	mldDumpIE((uint8_t *)pucBuf, u2IEsBufLen, "Secondary");

	DBGLOG(ML, LOUD, "Bss%d compose ML Link%d profile\n", ucBssIndex, link);
	IE_FOR_EACH(pucBuf, u2IEsBufLen, u2Offset) {
		if (mldDupStaProfileSkipIE(pucBuf))
			continue;

		if (IE_ID(pucBuf) == ELEM_ID_VENDOR) {
			uint32_t oui;

			/* For vendor ie, compare OUI + type to
			 * determine if they are the same ie. Moreover,
			 * vendor ie might not be well sorted by ie order,
			 * so always search from ies head
			 */
			WLAN_GET_FIELD_BE24(WFA_IE(pucBuf)->aucOui, &oui);
			primary = kalFindVendorIe(
				oui,
				WFA_IE(pucBuf)->ucOuiType,
				(uint8_t *)prMsduInfo->prPacket + u4BeginOffset,
				u4PrimaryLength - u4BeginOffset);
		} else {
			primary = kalFindIeMatchMask(
					IE_ID(pucBuf),
					start, end - start,
					pucBuf + 2, IE_LEN(pucBuf),
					IE_LEN(pucBuf) > 0 ? 2 : 0, NULL);
		}

		if (!primary || kalMemCmp(pucBuf, primary, IE_LEN(pucBuf))) {
			DBGLOG_MEM8(ML, LOUD, pucBuf, IE_SIZE(pucBuf));
			status = mldProfileCopyIe(prMsduInfo, &prIe,
						  &sta, &frag, &cp, pucBuf);
			if (status != WLAN_STATUS_SUCCESS) {
				DBGLOG(ML, WARN, "fail to copy");
				goto done;
			}
		} else {
			/* found same ie, move to next primary */
			start = primary + IE_SIZE(primary);
		}
	}

	/* handle noninheritance ie */
	start = (uint8_t *) prMsduInfoSta->prPacket + u4BeginOffset;
	end = (uint8_t *) prMsduInfoSta->prPacket +
				prMsduInfoSta->u2FrameLength;
	pucBuf = (uint8_t *)prMsduInfo->prPacket + u4BeginOffset;
	u2IEsBufLen = u4PrimaryLength - u4BeginOffset;
	neid = 0;
	nexid = 0;

	IE_FOR_EACH(pucBuf, u2IEsBufLen, u2Offset) {
		if (mldDupStaProfileSkipIE(pucBuf) ||
		    IE_ID(pucBuf) == ELEM_ID_VENDOR)
			continue;

		primary = kalFindIeExtIE(IE_ID(pucBuf), IE_ID_EXT(pucBuf),
			start, end - start);

		if (!primary) {
			/* noninheritance */
			if (IE_ID(pucBuf) == ELEM_ID_RESERVED)
				nexid_arr[nexid++] = IE_ID_EXT(pucBuf);
			else
				neid_arr[neid++] = IE_ID(pucBuf);
		} else {
			start = primary + IE_SIZE(primary);
		}
	}

	/* To avoid the rejection of the 5+5 connection, AIS always carring */
	if (neid > 0 || nexid > 0 || IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		uint8_t *buf, *p;
		uint16_t len = 3 + neid + nexid;

		buf = kalMemAlloc(ELEM_HDR_LEN + len, VIR_MEM_TYPE);
		if (!buf) {
			DBGLOG(ML, WARN, "no mem for inheritance IE\n");
			goto done;
		}
		non_inh = (struct IE_NON_INHERITANCE *) buf;
		non_inh->ucId = ELEM_ID_RESERVED;
		non_inh->ucLength = len;
		non_inh->ucExtId = ELEM_EXT_ID_NON_INHERITANCE;
		p = non_inh->aucList;
		*p++ = neid;
		for (i = 0; i < neid; i++)
			*p++ = neid_arr[i];
		*p++ = nexid;
		for (i = 0; i < nexid; i++)
			*p++ = nexid_arr[i];
		DBGLOG(ML, LOUD, "Bss%d compose ML Link%d non inheritance IE\n",
			ucBssIndex, link);
		DBGLOG_MEM8(ML, LOUD, non_inh, IE_SIZE(non_inh));

		status = mldProfileCopyIe(prMsduInfo, &prIe,
					  &sta, &frag, &cp, buf);
		if (status != WLAN_STATUS_SUCCESS)
			DBGLOG(ML, WARN, "fail to copy IE_NON_INHERITANCE");

		kalMemFree(buf, VIR_MEM_TYPE, ELEM_HDR_LEN + len);
	}

done:
	DBGLOG(ML, LOUD, "Bss%d dump ML Link%d IE\n", ucBssIndex, link);
	if (frag) {
		DBGLOG_MEM8(ML, LOUD, sta_ctrl, frag - (uint8_t *)sta_ctrl);
		DBGLOG_MEM8(ML, LOUD, frag, cp - frag);
	} else {
		DBGLOG_MEM8(ML, LOUD, sta_ctrl, cp - (uint8_t *)sta_ctrl);
	}

	return prIe;
}

uint32_t mldCalculateRnrIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *bss;
	struct MLD_BSS_INFO *mld_bssinfo;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	mld_bssinfo = mldBssGetByBss(prAdapter, bss);

	if (!IS_MLD_BSSINFO_MULTI(mld_bssinfo))
		return 0;
	if (!mld_bssinfo->rBssList.u4NumElem)
		return 0;

	/* num - 1 for skipping self,
	 * 16: Neighbor AP TBTT Offset + BSSID + short-ssid +
	 * Bss Param + PSD + MLD Para
	 */
	return sizeof(struct IE_RNR) + (mld_bssinfo->rBssList.u4NumElem - 1) *
		(sizeof(struct NEIGHBOR_AP_INFO_FIELD) + 16);
}

void mldGenerateRnrIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct BSS_INFO *bss;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct LINK *links;
	struct IE_RNR *rnr;
	struct NEIGHBOR_AP_INFO_FIELD *info;
	uint8_t *cp;
	uint8_t count = 0;
	uint32_t sssid;
	enum ENUM_CHNL_EXT eSco = CHNL_EXT_SCN;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, prMsduInfo->ucBssIndex);
	mld_bssinfo = mldBssGetByBss(prAdapter, bss);

	if (!IS_MLD_BSSINFO_MULTI(mld_bssinfo))
		return;
	if (!mld_bssinfo->rBssList.u4NumElem)
		return;

	rnr = (struct IE_RNR *)	((uint8_t *)prMsduInfo->prPacket +
						prMsduInfo->u2FrameLength);
	rnr->ucId = ELEM_ID_RNR;
	rnr->ucLength = (mld_bssinfo->rBssList.u4NumElem - 1) *
		(sizeof(struct NEIGHBOR_AP_INFO_FIELD) + 16);
	cp = rnr->aucInfoField;

	links = &mld_bssinfo->rBssList;
	LINK_FOR_EACH_ENTRY(bss, links, rLinkEntryMld,
		struct BSS_INFO) {
		if (count >= MLD_LINK_MAX) {
			DBGLOG(ML, ERROR, "too many links!!!\n");
			return;
		}

		if (bss->ucBssIndex == prMsduInfo->ucBssIndex)
			continue;

		info = (struct NEIGHBOR_AP_INFO_FIELD *) cp;

		/* count is default 0. no need to set,
		 * 16: Neighbor AP TBTT Offset + BSSID + short-ssid +
		 * Bss Param + PSD + MLD Para
		 */
		info->u2TbttInfoHdr = 16 << TBTT_INFO_HDR_LENGTH_OFFSET;
		if (bss->eBand == BAND_2G4 || bss->eBand == BAND_5G)
			eSco = (enum ENUM_CHNL_EXT)
				(bss->ucHtOpInfo1 & HT_OP_INFO1_SCO);
		info->ucOpClass =
			rlmGetOpClassForChannel(
				bss->ucPrimaryChannel,
				bss->eBand, eSco, bss->ucVhtChannelWidth,
				COUNTRY_CODE_NULL);
		info->ucChannelNum = bss->ucPrimaryChannel;

		cp = info->aucTbttInfoSet;

		/* Neighbor AP TBTT Offset (1) */
		*cp++ = 0xff;

		/* BSSID (6) */
		COPY_MAC_ADDR(cp, bss->aucOwnMacAddr);
		cp += MAC_ADDR_LEN;

		/* short ssid (4) */
		sssid = wlanCRC32(bss->aucSSID, bss->ucSSIDLen);
		WLAN_SET_FIELD_32(cp, sssid);
		cp += 4;

		/* Bss Parameters (1) */
		*cp++ = (TBTT_INFO_BSS_PARAM_SAME_SSID |
			 TBTT_INFO_BSS_PARAM_CO_LOCATED_AP);

		/* 20 Mhz PSD (1) */
		*cp++ = 0x22; /* 17 dBm/Mhz */

		/* MLD Para (3) */
		*cp++ = 0; /* MLD ID */
		*cp++ = bss->ucLinkIndex; /* Link ID */
		*cp++ = 0; /* BSS para change count */

		DBGLOG(ML, LOUD,
			"bss_idx: %d, link: %d: ch: %d, mac: " MACSTR "\n",
			bss->ucBssIndex,
			bss->ucLinkIndex,
			bss->ucPrimaryChannel,
			MAC2STR(bss->aucOwnMacAddr));

		count++;
	}

	prMsduInfo->u2FrameLength += IE_SIZE(rnr);
}

void mldParseBasicMlIE(struct MULTI_LINK_INFO *prMlInfo,
	const uint8_t *pucIE, uint16_t u2Left, const uint8_t *paucBssId,
	uint16_t u2FrameCtrl, const char *pucDesc)
{
	const uint8_t *pos, *end;
	uint8_t ucMlCtrlType, ucMlCtrlPreBmp;
	uint8_t show_info;
	uint8_t *tmp = NULL;
	u_int16_t u2Ctrl;
	u_int8_t *aucCommonInfo;

	show_info = !!(aucDebugModule[DBG_ML_IDX] & DBG_CLASS_LOUD) ||
		u2FrameCtrl == MAC_FRAME_ASSOC_REQ ||
		u2FrameCtrl == MAC_FRAME_ASSOC_RSP ||
		u2FrameCtrl == MAC_FRAME_REASSOC_REQ ||
		u2FrameCtrl == MAC_FRAME_REASSOC_RSP;

	if (show_info) {
		DBGLOG(ML, INFO, "[%s] ML BASIC IE, IE_LEN = %d\n",
			pucDesc, IE_LEN(pucIE));
		DBGLOG_MEM8(ML, INFO, (uint8_t *)pucIE, IE_SIZE(pucIE));
	}

	kalMemSet(prMlInfo, 0, sizeof(struct MULTI_LINK_INFO));

	if (IE_SIZE(pucIE) > u2Left) {
		DBGLOG(ML, INFO, "invalid IE_SIZE=%d, left=%d\n",
			IE_SIZE(pucIE), u2Left);
		return;
	}

	end = pucIE + IE_SIZE(pucIE);

	if (BE_IS_ML_CTRL_TYPE(pucIE, ML_CTRL_TYPE_BASIC)) {
		struct IE_MULTI_LINK_CONTROL *prMlInfoIe;

		if (u2Left < sizeof(struct IE_MULTI_LINK_CONTROL))
			return;

		prMlInfoIe = (struct IE_MULTI_LINK_CONTROL *)pucIE;
		u2Ctrl = prMlInfoIe->u2Ctrl;
		aucCommonInfo = prMlInfoIe->aucCommonInfo;
		pos = prMlInfoIe->aucCommonInfo;
	} else if (IE_ID(pucIE) == ELEM_ID_NR_BASIC_MULTI_LINK) {
		struct SUB_IE_MULTI_LINK_CONTROL *prMlInfoIe;

		if (u2Left < sizeof(struct SUB_IE_MULTI_LINK_CONTROL))
			return;

		prMlInfoIe = (struct SUB_IE_MULTI_LINK_CONTROL *)pucIE;
		u2Ctrl = prMlInfoIe->u2Ctrl;
		aucCommonInfo = prMlInfoIe->aucCommonInfo;
		pos = prMlInfoIe->aucCommonInfo;
	} else {
		DBGLOG(ML, INFO, "[%s] UNKNOWN IE, IE_LEN = %d\n",
			pucDesc, IE_LEN(pucIE));
		DBGLOG_MEM8(ML, INFO, (uint8_t *)pucIE, IE_SIZE(pucIE));
		return;
	}

	/* common info len:1 + mld mac:6 */
	if (pos + 7 > end)
		return;

	/* ML control bits[4,15] is presence bitmap */
	ucMlCtrlPreBmp = (u2Ctrl & ML_CTRL_PRE_BMP_MASK)
				>> ML_CTRL_PRE_BMP_SHIFT;
	/* ML control bits[0,2] is type */
	ucMlCtrlType = (u2Ctrl & ML_CTRL_TYPE_MASK);

	/* It shall be Basic variant ML element*/
	if (ucMlCtrlType != ML_CTRL_TYPE_BASIC) {
		prMlInfo->ucValid = FALSE;
		DBGLOG(ML, WARN, "invalid ML control type:%d\n", ucMlCtrlType);
		return;
	}

	prMlInfo->ucMlCtrlType = ucMlCtrlType;
	prMlInfo->ucMlCtrlPreBmp = ucMlCtrlPreBmp;
	prMlInfo->ucCommonInfoLength = *pos++;

	if (show_info)
		DBGLOG(ML, INFO, "\tML common Info Len = %d\n",
			prMlInfo->ucCommonInfoLength);

	if (aucCommonInfo + prMlInfo->ucCommonInfoLength > end) {
		DBGLOG(ML, WARN,
			"invalid common info len=%d, IE len=%d\n",
			prMlInfo->ucCommonInfoLength,
			IE_LEN(pucIE));
		return;
	}

	/* Check ML control that which common info exist */
	COPY_MAC_ADDR(prMlInfo->aucMldAddr, pos);
	if (show_info)
		DBGLOG(ML, INFO,
			"\tML common Info Mld addr = "MACSTR" (src="MACSTR")\n",
			MAC2STR(prMlInfo->aucMldAddr), MAC2STR(paucBssId));
	pos += MAC_ADDR_LEN;

	if (ucMlCtrlPreBmp & ML_CTRL_LINK_ID_INFO_PRESENT) {
		if (pos + 1 > end) {
			DBGLOG(ML, WARN, "invalid pos=%p end=%p\n", pos, end);
			return;
		}

		prMlInfo->ucLinkId = (*pos & BITS(0, 3));
		prMlInfo->u2ValidLinks |= BIT(prMlInfo->ucLinkId);
		if (show_info)
			DBGLOG(ML, INFO,
				"\tML common Info LinkID = %d\n", *pos);
		pos += 1;
	}
	if (ucMlCtrlPreBmp & ML_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT) {
		if (pos + 1 > end) {
			DBGLOG(ML, WARN, "invalid pos=%p end=%p\n", pos, end);
			return;
		}

		prMlInfo->ucBssParaChangeCount = *pos;
		if (show_info)
			DBGLOG(ML, INFO,
				"\tML common Info BssParaChangeCount = %d\n",
				*pos);
		pos += 1;
	}
	if (ucMlCtrlPreBmp & ML_CTRL_MEDIUM_SYN_DELAY_INFO_PRESENT) {
		if (pos + 2 > end)
			return;

		/* todo: handle 2byte MEDIUM_SYN_DELAY_INFO_PRESENT */
		kalMemCopy(&prMlInfo->u2MediumSynDelayInfo, pos, 2);
		if (show_info)
			DBGLOG(ML, INFO,
				"\tML common Info MediumSynDelayInfo = 0x%x\n",
				prMlInfo->u2MediumSynDelayInfo);
		pos += 2;
	}
	if (ucMlCtrlPreBmp & ML_CTRL_EML_CAPA_PRESENT) {
		if (pos + 2 > end) {
			DBGLOG(ML, WARN, "invalid pos=%p end=%p\n", pos, end);
			return;
		}

		kalMemCopy(&prMlInfo->u2EmlCap, pos, 2);
		if (show_info)
			DBGLOG(ML, INFO, "\tML common Info EML capa = 0x%x\n",
				prMlInfo->u2EmlCap);
		pos += 2;
	}
	if (ucMlCtrlPreBmp & ML_CTRL_MLD_CAPA_PRESENT) {
		if (pos + 2 > end) {
			DBGLOG(ML, WARN, "invalid pos=%p end=%p\n", pos, end);
			return;
		}

		kalMemCopy(&prMlInfo->u2MldCap, pos, 2);
		if (show_info)
			DBGLOG(ML, INFO, "\tML common Info MLD capa = 0x%x\n",
				prMlInfo->u2MldCap);
		pos += 2;
	}
	if (ucMlCtrlPreBmp & ML_CTRL_MLD_ID_PRESENT) {
		if (pos + 1 > end) {
			DBGLOG(ML, WARN, "invalid pos=%p end=%p\n", pos, end);
			return;
		}

		prMlInfo->ucMldId = *pos;
		if (show_info)
			DBGLOG(ML, INFO, "\tML common Info MLD ID = %d\n",
				prMlInfo->ucMldId);
		pos += 1;
	}
	if (ucMlCtrlPreBmp & ML_CTRL_EXT_MLD_CAP_OP_PRESENT) {
		if (pos + 2 > end) {
			DBGLOG(ML, WARN, "invalid pos=%p end=%p\n", pos, end);
			return;
		}

		prMlInfo->u2ExtMldCap = *pos;
		if (show_info)
			DBGLOG(ML, INFO, "\tML common Info MLD ID = 0x%x\n",
				prMlInfo->u2ExtMldCap);
		pos += 2;
	}

	if (prMlInfo->ucCommonInfoLength < (pos - aucCommonInfo)) {
		DBGLOG(ML, ERROR,
			"abnormal ML control len: expected %d < real %ld\n",
			prMlInfo->ucCommonInfoLength,
			pos - aucCommonInfo);
		return;
	}

	if (pos - aucCommonInfo != prMlInfo->ucCommonInfoLength) {
		DBGLOG(ML, WARN,
			"invalid ML control len: real %ld != expected %d\n",
			pos - aucCommonInfo,
			prMlInfo->ucCommonInfoLength);
		pos = aucCommonInfo + prMlInfo->ucCommonInfoLength;
	}

	if ((u2Left > IE_SIZE(pucIE)) && (u2Left < CONTROL_BUFFER_SIZE)) {
		const uint8_t *tmp_pos, *tmp_end;
		uint8_t *p;
		uint8_t found = FALSE;

		/* fragement no need */
		if (IE_LEN(pucIE) != 0xff)
			goto link_info;

		tmp_pos = end; /* traverse original buffer */
		tmp_end = pucIE + u2Left;
		while (tmp_end - tmp_pos >= 2 &&
		       IE_ID(tmp_pos) == ELEM_ID_FRAGMENT &&
		       IE_SIZE(tmp_pos) <= tmp_end - tmp_pos) {
			found = TRUE;
			tmp_pos += IE_SIZE(tmp_pos);
			break;
		}

		if (!found)
			goto link_info;

		tmp = kalMemAlloc(u2Left, VIR_MEM_TYPE);
		if (!tmp) {
			DBGLOG(ML, WARN, "No resource left=%d\n", u2Left);
			goto link_info;
		}

		/* copy rest of ml element first */
		kalMemCopy(tmp, pos, end - pos);

		/* pos to copy fragement */
		p = tmp + (uint32_t)(end - pos);
		tmp_pos = end; /* traverse original buffer */
		tmp_end = pucIE + u2Left;

		/* Add possible fragments */
		while (tmp_end - tmp_pos >= 2 &&
		       IE_ID(tmp_pos) == ELEM_ID_FRAGMENT &&
		       IE_SIZE(tmp_pos) <= tmp_end - tmp_pos) {
			kalMemCopy(p, IE_DATA(tmp_pos), IE_LEN(tmp_pos));
			p += IE_LEN(tmp_pos);
			tmp_pos += IE_SIZE(tmp_pos);
		}

		/* parsing tmp buffer to find per-sta profiles */
		pos = tmp;
		end = p;

		DBGLOG(ML, LOUD, "Found fragment\n");
		DBGLOG_MEM8(ML, LOUD, pos, end - pos);
	}

link_info:
	/* pos point to link info, recusive parse it */
	while (pos < end) {
		struct IE_ML_STA_CONTROL *prIeSta =
			(struct IE_ML_STA_CONTROL *)pos;
		const uint8_t *tail = pos + IE_SIZE(pos);
		const uint8_t *next_sta = pos + IE_SIZE(pos);
		struct STA_PROFILE *prStaProfile;
		uint8_t ucLinkId, ucStaInfoLen;
		uint16_t u2StaControl, tmp_len = 0;
		const uint8_t *tmp_pos, *tmp_end;
		uint8_t *tmp_buf = NULL, *p;
		uint8_t found = FALSE;

		if (prIeSta->ucSubID != SUB_IE_MLD_PER_STA_PROFILE ||
		    IE_SIZE(prIeSta) < sizeof(struct IE_ML_STA_CONTROL) ||
		    IE_LEN(prIeSta) == 0 ||
		    tail > end ||
		    prMlInfo->ucProfNum >= MLD_LINK_MAX)
			goto next;

		tmp_pos = tail; /* traverse original buffer */
		tmp_end = end;

		/* fragement no need */
		if (IE_LEN(pucIE) != 0xff)
			goto sta;

		while (tmp_end - tmp_pos >= 2 &&
		       IE_ID(tmp_pos) == SUB_IE_MLD_FRAGMENT &&
		       IE_SIZE(tmp_pos) <= tmp_end - tmp_pos) {
			found = TRUE;
			tmp_pos += IE_SIZE(tmp_pos);
			break;
		}

		if (!found)
			goto sta;

		tmp_len = end - pos;
		tmp_buf = kalMemAlloc(tmp_len, VIR_MEM_TYPE);
		if (!tmp_buf) {
			DBGLOG(ML, WARN, "No resource len=%d\n", tmp_len);
			goto sta;
		}

		/* copy per-sta sub element first */
		kalMemCopy(tmp_buf, pos, tail - pos);

		/* pos to copy fragement */
		p = tmp_buf + (uint32_t)(tail - pos);
		tmp_pos = tail; /* traverse original buffer */
		tmp_end = end;

		/* Add possible fragments */
		while (tmp_end - tmp_pos >= 2 &&
		       IE_ID(tmp_pos) == SUB_IE_MLD_FRAGMENT &&
		       IE_SIZE(tmp_pos) <= tmp_end - tmp_pos) {
			kalMemCopy(p, IE_DATA(tmp_pos), IE_LEN(tmp_pos));
			p += IE_LEN(tmp_pos);
			tmp_pos += IE_SIZE(tmp_pos);
		}

		/* parsing tmp buffer for a per-sta profile */
		pos = tmp_buf;
		prIeSta = (struct IE_ML_STA_CONTROL *)pos;
		tail = p;
		next_sta = tmp_pos;

		DBGLOG(ML, LOUD, "Found sub fragment\n");
		DBGLOG_MEM8(ML, LOUD, pos, tail - pos);

		if (IE_SIZE(pos) < sizeof(struct IE_ML_STA_CONTROL)) {
			DBGLOG(ML, WARN, "invalid sta control len=%d\n",
			       IE_SIZE(pos));
			goto next;
		}
sta:
		u2StaControl = prIeSta->u2StaCtrl;
		ucLinkId = (u2StaControl & ML_STA_CTRL_LINK_ID_MASK);
		if (prMlInfo->u2ValidLinks & BIT(ucLinkId)) {
			DBGLOG(ML, WARN, "dup sta profile, LinkID=%d\n",
				ucLinkId);
			goto next;
		}

		prMlInfo->u2ValidLinks |= BIT(ucLinkId);
		prStaProfile = &prMlInfo->rStaProfiles[prMlInfo->ucProfNum++];
		prStaProfile->ucLinkId = ucLinkId;
		prStaProfile->u2StaCtrl = u2StaControl;
		prStaProfile->ucComplete =
			!!(u2StaControl & ML_STA_CTRL_COMPLETE_PROFILE);

		if (show_info)
			DBGLOG(ML, INFO,
				"\tLinkID=%d Ctrl=0x%x(%s) Total=%d\n",
				ucLinkId, u2StaControl,
				prStaProfile->ucComplete ?
				"COMPLETE" : "PARTIAL",
				prMlInfo->ucProfNum);

		pos = prIeSta->aucStaInfo;
		if (pos + 1 > tail) {
			DBGLOG(ML, WARN,
				"invalid STA profile len=%td\n", tail - pos);
			prMlInfo->ucProfNum--;
			goto next;
		}

		ucStaInfoLen = *pos++;

		if (prIeSta->aucStaInfo + ucStaInfoLen > tail) {
			DBGLOG(ML, WARN,
				"invalid STA profile len=%d\n", ucStaInfoLen);
			prMlInfo->ucProfNum--;
			goto next;
		}

		if (u2StaControl & ML_STA_CTRL_MAC_ADDR_PRESENT) {
			if (pos + MAC_ADDR_LEN > tail) {
				DBGLOG(ML, WARN,
					"invalid STA profile len=%td\n",
					tail - pos);
				prMlInfo->ucProfNum--;
				goto next;
			}

			COPY_MAC_ADDR(prStaProfile->aucLinkAddr, pos);
			if (show_info)
				DBGLOG(ML, INFO,
					"\tLinkID=%d, LinkAddr="MACSTR"\n",
					ucLinkId,
					MAC2STR(prStaProfile->aucLinkAddr));
			pos += MAC_ADDR_LEN;
		}
		if (u2StaControl & ML_STA_CTRL_BCN_INTV_PRESENT) {
			if (pos + 2 > tail) {
				DBGLOG(ML, WARN,
					"invalid STA profile len=%td\n",
					tail - pos);
				prMlInfo->ucProfNum--;
				goto next;
			}

			kalMemCopy(&prStaProfile->u2BcnIntv, pos, 2);
			if (show_info)
				DBGLOG(ML, INFO,
					"\tLinkID=%d, BCN_INTV = %d\n",
					ucLinkId, prStaProfile->u2BcnIntv);
			pos += 2;
		}
		if (u2StaControl & ML_STA_CTRL_TSF_OFFSET_PRESENT) {
			if (pos + 8 > tail) {
				DBGLOG(ML, WARN,
					"invalid STA profile len=%td\n",
					tail - pos);
				prMlInfo->ucProfNum--;
				goto next;
			}

			kalMemCopy(&prStaProfile->i8TsfOffset, pos, 8);
			if (show_info)
				DBGLOG(ML, INFO,
					"\tLinkID=%d, TSF_OFFSET = %ld\n",
					ucLinkId, prStaProfile->i8TsfOffset);
			pos += 8;
		}
		if (u2StaControl & ML_STA_CTRL_DTIM_INFO_PRESENT) {
			if (pos + 2 > tail) {
				DBGLOG(ML, WARN,
					"invalid STA profile len=%td\n",
					tail - pos);
				prMlInfo->ucProfNum--;
				goto next;
			}

			kalMemCopy(&prStaProfile->u2DtimInfo, pos, 2);
			if (show_info)
				DBGLOG(ML, INFO,
					"\tLinkID=%d, DTIM_INFO = 0x%x\n",
					ucLinkId, prStaProfile->u2DtimInfo);
			pos += 2;
		}

		/* If the Complete Profile subfield = 1 and
		 * NSTR Link Pair Present = 1, then NSTR Indication Bitmap exist
		 * NSTR Bitmap Size = 1 if the length of the corresponding
		 * NSTR Indication Bitmap is 2 bytes, and = 0 if the
		 * length of the corresponding NSTR Indication Bitmap = 1 byte
		 */
		if ((u2StaControl & ML_STA_CTRL_COMPLETE_PROFILE) &&
			(u2StaControl & ML_STA_CTRL_NSTR_LINK_PAIR_PRESENT)) {
			if (((u2StaControl & ML_STA_CTRL_NSTR_BMP_SIZE) >>
				ML_STA_CTRL_NSTR_BMP_SIZE_SHIFT) == 0) {
				if (pos + 1 > tail) {
					DBGLOG(ML, WARN,
						"invalid STA profile len=%td\n",
						tail - pos);
					prMlInfo->ucProfNum--;
					goto next;
				}

				prStaProfile->u2NstrBmp = *pos;
				if (show_info)
					DBGLOG(ML, INFO,
					     "\tLinkID=%d, NSTR_BMP0=0x%x\n",
					     ucLinkId, prStaProfile->u2NstrBmp);
				pos += 1;
			} else {
				if (pos + 2 > tail) {
					DBGLOG(ML, WARN,
						"invalid STA profile len=%td\n",
						tail - pos);
					prMlInfo->ucProfNum--;
					goto next;
				}

				kalMemCopy(&prStaProfile->u2NstrBmp, pos, 2);
				if (show_info)
					DBGLOG(ML, INFO,
					     "\tLinkID=%d, NSTR_BMP1=0x%x\n",
					     ucLinkId, prStaProfile->u2NstrBmp);
				pos += 2;
			}
		}

		if (u2StaControl & ML_STA_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT) {
			if (pos + 1 > tail) {
				DBGLOG(ML, WARN,
					"invalid STA profile len=%td\n",
					tail - pos);
				prMlInfo->ucProfNum--;
				goto next;
			}

			prStaProfile->ucBssParaChangeCount = *pos++;
			if (show_info)
				DBGLOG(ML, INFO,
				  "\tLinkID=%d, BSS_PARA_CHANGE_COUNT=0x%x\n",
				  ucLinkId, prStaProfile->ucBssParaChangeCount);
		}

		if (ucStaInfoLen < (pos - prIeSta->aucStaInfo)) {
			DBGLOG(ML, ERROR,
				"abnormal STA info len: expected %d < real %ld\n",
				ucStaInfoLen,
				pos - prIeSta->aucStaInfo);
			prMlInfo->ucProfNum--;
			goto next;
		}

		if (ucStaInfoLen != pos - prIeSta->aucStaInfo) {
			DBGLOG(ML, WARN,
				"invalid STA info len: real %ld != expected %d\n",
				pos - prIeSta->aucStaInfo,
				ucStaInfoLen);
			pos = prIeSta->aucStaInfo + ucStaInfoLen;
		}

		/* Only Management frames belonging to subtypes (Re)Association
		 * Request, (Re)Association Response, or Probe Response that is
		 * an ML probe response can carry complete profile of
		 * a reported STA
		 */
		if (prStaProfile->ucComplete &&
			    u2FrameCtrl != MAC_FRAME_ASSOC_REQ &&
			    u2FrameCtrl != MAC_FRAME_ASSOC_RSP &&
			    u2FrameCtrl != MAC_FRAME_REASSOC_REQ &&
			    u2FrameCtrl != MAC_FRAME_REASSOC_RSP &&
			    u2FrameCtrl != MAC_FRAME_PROBE_RSP) {
			DBGLOG(ML, WARN,
				"frame_ctrl=%x not allowed to carry complete sta profile\n",
				u2FrameCtrl);
			prMlInfo->ucProfNum--;
			goto next;
		}

		if (pos == tail)
			goto next;

		if (pos + 2 > tail) {
			DBGLOG(ML, WARN,
				"invalid STA profile len=%td\n", tail - pos);
			prMlInfo->ucProfNum--;
			goto next;
		}

		WLAN_GET_FIELD_16(pos, &prStaProfile->u2CapInfo);
		if (show_info)
			DBGLOG(ML, INFO,
				"\tLinkID=%d, CAP_INFO = 0x%x\n",
				ucLinkId, prStaProfile->u2CapInfo);
		pos += 2;

		if (u2FrameCtrl == MAC_FRAME_ASSOC_RSP ||
		    u2FrameCtrl == MAC_FRAME_REASSOC_RSP) {
			if (pos + 2 > tail) {
				DBGLOG(ML, WARN,
					"invalid STA profile len=%td\n",
					tail - pos);
				prMlInfo->ucProfNum--;
				goto next;
			}

			WLAN_GET_FIELD_16(pos,
				&prStaProfile->u2StatusCode);
			if (show_info)
				DBGLOG(ML, INFO,
				  "\tLinkID=%d, Status = 0x%x\n",
				  ucLinkId, prStaProfile->u2StatusCode);
			pos += 2;
		}

		if (pos > tail) {
			DBGLOG(ML, WARN,
				"invalid STA profile len=%td\n", tail - pos);
			prMlInfo->ucProfNum--;
			goto next;
		}

		/* (tail - pos) is length of STA Profile
		 * copy STA profile in Per-STA profile subelement.
		 */
		prStaProfile->u2IEbufLen = 0;
		if (tail - pos < sizeof(prStaProfile->aucIEbuf)) {
			if (show_info)
				DBGLOG(ML, INFO, "\tcopy sta profile len=%td\n",
					tail - pos);
			kalMemCopy(prStaProfile->aucIEbuf,
				pos, tail - pos);
			prStaProfile->u2IEbufLen = tail - pos;
		} else {
			DBGLOG(ML, WARN,
				"sta profile ie len too long %ld!!\n",
				tail - pos);
			kalMemCopy(prStaProfile->aucIEbuf,
				pos, sizeof(prStaProfile->aucIEbuf));
			prStaProfile->u2IEbufLen =
				sizeof(prStaProfile->aucIEbuf);
		}

next:
		if (tmp_buf)
			kalMemFree(tmp_buf, tmp_len, VIR_MEM_TYPE);
		/* point to next Per-STA profile*/
		pos = next_sta;
	}

	if (tmp)
		kalMemFree(tmp, u2Left, VIR_MEM_TYPE);
	prMlInfo->ucValid = TRUE;
}

void mldParseReconfigMlIE(struct MULTI_LINK_INFO *prMlInfo,
	const uint8_t *pucIE, const uint8_t *paucBssId, const char *pucDesc)
{
	const uint8_t *pos, *end;
	uint8_t ucMlCtrlType, ucMlCtrlPreBmp;
	struct IE_MULTI_LINK_CONTROL *prMlInfoIe;
	uint8_t show_info = !!(aucDebugModule[DBG_ML_IDX] & DBG_CLASS_LOUD);

	if (show_info) {
		DBGLOG(ML, INFO, "[%s] ML RECONFIG IE, IE_LEN = %d\n",
			pucDesc, IE_LEN(pucIE));
		DBGLOG_MEM8(ML, INFO, (uint8_t *)pucIE, IE_SIZE(pucIE));
	}

	kalMemSet(prMlInfo, 0, sizeof(struct MULTI_LINK_INFO));

	end = pucIE + IE_SIZE(pucIE);
	prMlInfoIe = (struct IE_MULTI_LINK_CONTROL *)pucIE;
	pos = prMlInfoIe->aucCommonInfo;

	/* ML control bits[4,15] is presence bitmap */
	ucMlCtrlPreBmp = ((prMlInfoIe->u2Ctrl & ML_CTRL_PRE_BMP_MASK)
				>> ML_CTRL_PRE_BMP_SHIFT);
	/* ML control bits[0,2] is type */
	ucMlCtrlType = (prMlInfoIe->u2Ctrl & ML_CTRL_TYPE_MASK);

	/* It shall be Reconfiguration variant ML element*/
	if (ucMlCtrlType != ML_CTRL_TYPE_RECONFIG) {
		prMlInfo->ucValid = FALSE;
		DBGLOG(ML, WARN, "invalid ML control type:%d\n", ucMlCtrlType);
		return;
	}

	prMlInfo->ucMlCtrlType = ucMlCtrlType;
	prMlInfo->ucMlCtrlPreBmp = ucMlCtrlPreBmp;
	prMlInfo->ucCommonInfoLength = *pos++;

	/* Check ML control that which common info exist */
	if (ucMlCtrlPreBmp & ML_RECFG_MLD_ADDR_PRESENT) {
		COPY_MAC_ADDR(prMlInfo->aucMldAddr, pos);
		if (show_info)
			DBGLOG(ML, INFO,
			"\tML common Info Mld addr = "MACSTR" (src="MACSTR")\n",
			MAC2STR(prMlInfo->aucMldAddr), MAC2STR(paucBssId));
		pos += MAC_ADDR_LEN;
	}
	if (ucMlCtrlPreBmp & ML_RECFG_EML_CAP_PRESENT) {
		kalMemCopy(&prMlInfo->u2EmlCap, pos, 2);
		if (show_info)
			DBGLOG(ML, INFO, "\tML common Info EML capa = 0x%x\n",
				prMlInfo->u2EmlCap);
		pos += 2;
	}
	if (ucMlCtrlPreBmp & ML_RECFG_MLD_CAP_OP_PRESENT) {
		kalMemCopy(&prMlInfo->u2MldCap, pos, 2);
		if (show_info)
			DBGLOG(ML, INFO, "\tML common Info MLD capa = 0x%x\n",
				prMlInfo->u2MldCap);
		pos += 2;
	}

	if (pos - prMlInfoIe->aucCommonInfo !=
			prMlInfo->ucCommonInfoLength) {
		DBGLOG(ML, WARN,
			"invalid ML control len: real %ld != expected %d\n",
			pos - prMlInfoIe->aucCommonInfo,
			prMlInfo->ucCommonInfoLength);
		pos = prMlInfoIe->aucCommonInfo + prMlInfo->ucCommonInfoLength;
	}

	/* pos point to link info, recusive parse it */
	while (pos < end) {
		struct IE_ML_STA_CONTROL *prIeSta =
			(struct IE_ML_STA_CONTROL *)pos;
		const uint8_t *tail = pos + IE_SIZE(pos);
		const uint8_t *next_sta = pos + IE_SIZE(pos);
		struct STA_PROFILE *prStaProfile;
		uint8_t ucLinkId, ucStaInfoLen;
		uint16_t u2StaControl;
		const uint8_t *opTypeStr[] = {"AP_REMOVVAL", "OP_UPDATE",
			"ADD_LINK", "DEL_LINK"};


		if (prIeSta->ucSubID != SUB_IE_MLD_PER_STA_PROFILE ||
		    IE_SIZE(prIeSta) < sizeof(struct IE_ML_STA_CONTROL) ||
		    prMlInfo->ucProfNum >= MLD_LINK_MAX)
			goto next;

		u2StaControl = prIeSta->u2StaCtrl;
		ucLinkId = (u2StaControl & ML_RECFG_STA_CTRL_LINK_ID_MASK);
		if (prMlInfo->u2ValidLinks & BIT(ucLinkId)) {
			DBGLOG(ML, WARN, "dup sta profile, LinkID=%d\n",
				ucLinkId);
			goto next;
		}

		prMlInfo->u2ValidLinks |= BIT(ucLinkId);
		prStaProfile = &prMlInfo->rStaProfiles[prMlInfo->ucProfNum++];
		prStaProfile->ucLinkId = ucLinkId;
		prStaProfile->u2StaCtrl = u2StaControl;
		prStaProfile->ucComplete =
			!!(u2StaControl & ML_RECFG_STA_CTRL_COMPLETE_PROFILE);
		prStaProfile->ucOpType =
			(u2StaControl & ML_RECFG_STA_CTRL_OP_TYPE_MASK) >>
				ML_RECFG_STA_CTRL_OP_TYPE_SHIFT;

		if (show_info)
			DBGLOG(ML, INFO,
				"\tLinkID=%d Ctrl=0x%x(%s,%s) Total=%d\n",
				ucLinkId, u2StaControl,
				prStaProfile->ucComplete ?
				"COMPLETE" : "PARTIAL",
				prStaProfile->ucOpType < 4 ?
				opTypeStr[prStaProfile->ucOpType] :
				(const uint8_t *)"UNKNOWN",
				prMlInfo->ucProfNum);

		pos = prIeSta->aucStaInfo;
		ucStaInfoLen = *pos++;

		if (u2StaControl & ML_RECFG_STA_CTRL_MAC_ADDR_PRESENT) {
			COPY_MAC_ADDR(prStaProfile->aucLinkAddr, pos);
			if (show_info)
				DBGLOG(ML, INFO,
					"\tLinkID=%d, LinkAddr="MACSTR"\n",
					ucLinkId,
					MAC2STR(prStaProfile->aucLinkAddr));
			pos += MAC_ADDR_LEN;
		}
		if (u2StaControl & ML_RECFG_STA_CTRL_DELETE_TIMER_PRESENT) {
			kalMemCopy(&prStaProfile->u2ApRemovalTimer, pos, 2);
			if (show_info)
				DBGLOG(ML, INFO,
				      "\tLinkID=%d, AP_REMOVAL_TIMER = %d\n",
				      ucLinkId, prStaProfile->u2ApRemovalTimer);
			pos += 2;
		}
		if (u2StaControl & ML_RECFG_STA_CTRL_OP_PARAM_PRESENT) {
			kalMemCopy(&prStaProfile->u4OpParam, pos, 3);
			if (show_info)
				DBGLOG(ML, INFO,
					"\tLinkID=%d, OP_PARAM = 0x%x\n",
					ucLinkId, prStaProfile->u4OpParam);
			pos += 3;
		}

		if (pos > tail) {
			DBGLOG(ML, WARN,
				"invalid STA profile len=%td\n", tail - pos);
			goto next;
		}

		/* The Per-STA Profile shall not include a STA Profile */
next:
		/* point to next Per-STA profile*/
		pos = next_sta;
	}

	prMlInfo->ucValid = TRUE;
}

void mldParsePriorityAccessLinkInfo(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb, struct MULTI_LINK_INFO *prMlInfo,
		const uint8_t *pos, const uint8_t *end, uint16_t u2IELength)
{
	struct IE_ML_STA_CONTROL *prIeSta;
	const uint8_t *next_sta;
	uint16_t u2StaControl;
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	uint8_t ucLinkId;
#endif

	next_sta = pos;

	while (next_sta < end) {
		pos = next_sta;
		next_sta = pos + IE_SIZE(pos);

		prIeSta = (struct IE_ML_STA_CONTROL *)pos;

		if (prIeSta->ucSubID != SUB_IE_MLD_PER_STA_PROFILE ||
			IE_SIZE(prIeSta) < sizeof(struct IE_ML_STA_CONTROL) ||
			prMlInfo->ucProfNum >= MLD_LINK_MAX)
			continue;

		u2StaControl = prIeSta->u2StaCtrl;
		pos = prIeSta->aucStaInfo;

		switch (prMlInfo->ucMlCtrlType) {
		case ML_CTRL_TYPE_PRIORITY_ACCESS:
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
			ucLinkId = (u2StaControl &
					ML_PRIACC_STA_CTRL_LINK_ID_MASK);
			mldParseStaProfilePriorityAccess(prAdapter, prMlInfo,
					prSwRfb, ucLinkId, u2StaControl, pos,
					u2IELength);
#endif
			break;
		default:
			DBGLOG(ML, ERROR, "ucMlCtrlType %u not implmented\n",
					prMlInfo->ucMlCtrlType);
			return;
		}

		if (pos > next_sta) {
			DBGLOG(ML, WARN, "invalid STA profile len=%td\n",
					next_sta - pos);
			continue;
		}


	} /* while (next_sta < end) */

	prMlInfo->ucValid = TRUE;

}

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
void mldApplyPriorityAccessAllLinks(struct MLD_BSS_INFO *prMldBssInfo,
		struct BSS_INFO *prSrcBssInfo)
{
	struct BSS_INFO *bss;

	LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList, rLinkEntryMld,
			struct BSS_INFO) {
		if (prSrcBssInfo == bss)
			continue;
		bss->ucWmmParamSetCount =
				prSrcBssInfo->ucBackupWmmParamSetCount;
		kalMemCopy(bss->arACQueParms, prSrcBssInfo->arACQueParms,
				sizeof(prSrcBssInfo->arACQueParms));
		bss->ucMUEdcaUpdateCnt = prSrcBssInfo->ucBackupMUEdcaUpdateCnt;
		kalMemCopy(bss->arMUEdcaParams, prSrcBssInfo->arMUEdcaParams,
				sizeof(prSrcBssInfo->arMUEdcaParams));
	}
}

void mldParseStaProfilePriorityAccess(struct ADAPTER *prAdapter,
		struct MULTI_LINK_INFO *prMlInfo, struct SW_RFB *prSwRfb,
		uint8_t ucLinkId, uint16_t u2StaControl, const uint8_t *pos,
		uint16_t u2IELength)
{
	struct STA_PROFILE *prStaProfile;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct MLD_STA_RECORD *prMldStaRec;
	struct MLD_BSS_INFO *prMldBssInfo;

	if (!prAdapter || !prSwRfb)
		return;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec)
		return;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

	prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);

	if (!prMldStaRec || !prMldBssInfo) {
		DBGLOG(RX, ERROR,
			"prMldStaRec or prMldBssInfo equal to NULL\n");
		return;
	}

	if (prMlInfo->u2ValidLinks & BIT(ucLinkId))
		DBGLOG(ML, WARN, "dup sta profile, LinkID=%d\n", ucLinkId);
	prMlInfo->u2ValidLinks |= BIT(ucLinkId);
	prStaProfile = &prMlInfo->rStaProfiles[prMlInfo->ucProfNum++];
	prStaProfile->ucLinkId = ucLinkId;
	prStaProfile->u2StaCtrl = u2StaControl;

	DBGLOG(ML, TRACE, "\tLinkID=%d Ctrl=0x%x Total=%d\n",
			prStaProfile->ucLinkId, prStaProfile->u2StaCtrl,
			prMlInfo->ucProfNum);

	/* Parse (MU)Edca parameters */
	mqmIsBssEdcaParamsUpdated(prAdapter, prSwRfb, pos, u2IELength, TRUE);
	mqmParseMUEdcaParams(prAdapter, prSwRfb, pos, u2IELength, TRUE);
	mldApplyPriorityAccessAllLinks(prMldBssInfo, prBssInfo);

}


void mldParsePriorityAccessMlIE(struct ADAPTER *prAdapter,
		struct MULTI_LINK_INFO *prMlInfo, struct SW_RFB *prSwRfb,
		const uint8_t *pucIE, uint16_t u2IELength, const char *pucDesc)
{
	const uint8_t *pos, *end;
	uint8_t ucMlCtrlType;
	struct IE_MULTI_LINK_CONTROL *prMlInfoIe;

	DBGLOG(ML, INFO, "[%s] ML PRIORITY ACCESS IE, IE_LEN = %d\n",
		pucDesc, IE_LEN(pucIE));
	DBGLOG_MEM8(ML, INFO, (uint8_t *)pucIE, IE_SIZE(pucIE));

	kalMemSet(prMlInfo, 0, sizeof(struct MULTI_LINK_INFO));

	end = pucIE + IE_SIZE(pucIE);
	prMlInfoIe = (struct IE_MULTI_LINK_CONTROL *)pucIE;
	pos = prMlInfoIe->aucCommonInfo;

	ucMlCtrlType = ML_GET_CTRL_TYPE(prMlInfoIe);
	/* It shall be Priority Access ML element*/
	if (ucMlCtrlType != ML_CTRL_TYPE_PRIORITY_ACCESS) {
		prMlInfo->ucValid = FALSE;
		DBGLOG(ML, WARN, "invalid ML control type:%u\n", ucMlCtrlType);
		return;
	}

	prMlInfo->ucMlCtrlType = ucMlCtrlType;
	prMlInfo->ucCommonInfoLength = *pos++;

	/* AP MLD MAC Address is mandatory in common info field */
	/* BE D3.0 Figure 9-1002ad Common Info Field of the Priority Access
	 * Multi-Link element format
	 */
	COPY_MAC_ADDR(prMlInfo->aucMldAddr, pos);
	pos += MAC_ADDR_LEN;
	DBGLOG(ML, TRACE, "\tML common Info AP Mld addr = "MACSTR"\n",
		MAC2STR(prMlInfo->aucMldAddr));

	/* pos point to link info, recursively parse it */
	mldParsePriorityAccessLinkInfo(prAdapter, prSwRfb, prMlInfo, pos, end,
			u2IELength);
}
#endif

const uint8_t *mldFindMlIE(const uint8_t *ies, uint16_t len, uint8_t type)
{
	const uint8_t aucMtkOui[] = VENDOR_OUI_MTK;
	uint16_t u2Offset;
	uint8_t *ie, *sub;
	uint16_t ie_len, ie_offset;
	uint16_t sub_len, sub_offset;

	_Static_assert(sizeof(struct IE_HDR) ==
		       OFFSET_OF(struct IE_HDR, aucInfo),
		       "Need to define aucInfo[] in struct IE_HDR");
	_Static_assert(sizeof(struct IE_MTK_OUI) ==
		       OFFSET_OF(struct IE_MTK_OUI, aucInfoElem),
		       "Need to define aucInfoElem[] in struct IE_MTK_OUI");
	_Static_assert(sizeof(struct IE_MTK_PRE_WIFI7) ==
		       OFFSET_OF(struct IE_MTK_PRE_WIFI7, aucInfoElem),
		       "Need to define aucInfoElem[] in struct IE_MTK_PRE_WIFI7");

	IE_FOR_EACH(ies, len, u2Offset) {
		if (BE_IS_ML_CTRL_TYPE(ies, type))
			return ies;

		/* only check tlv */
		if (IE_ID(ies) != ELEM_ID_VENDOR ||
		    IE_SIZE(ies) < sizeof(struct IE_MTK_OUI) ||
		    kalMemCmp(IE_DATA(ies), aucMtkOui, sizeof(aucMtkOui)) ||
		    !(MTK_OUI_IE(ies)->aucCapability[0] &
				MTK_SYNERGY_CAP_SUPPORT_TLV))
			continue;

		ie = MTK_OUI_IE(ies)->aucInfoElem;
		ie_len = MTK_OUI_IE_INFO_SIZE(ies);

		IE_FOR_EACH(ie, ie_len, ie_offset) {
			if (IE_ID(ie) != MTK_OUI_ID_PRE_WIFI7)
				continue;

			if (IE_SIZE(ie) < sizeof(struct IE_MTK_PRE_WIFI7))
				return NULL;

			sub = MTK_PRE_WIFI7_IE(ie)->aucInfoElem;
			sub_len = MTK_PRE_WIFI7_IE_INFO_SIZE(ie);

			IE_FOR_EACH(sub, sub_len, sub_offset) {
				if (BE_IS_ML_CTRL_TYPE(sub, type))
					return sub;
			}
		}
	}

	return NULL;
}

uint8_t mldSameElement(uint8_t *ie1, uint8_t *ie2)
{
	if (!ie1 || !ie2)
		return FALSE;

	if (IE_ID(ie1) == IE_ID(ie2)) {
		if (IE_ID(ie1) == ELEM_ID_RESERVED) {
			if (IE_ID_EXT(ie1) == IE_ID_EXT(ie2))
				return TRUE;
		} else if (IE_ID(ie1) == ELEM_ID_VENDOR) {
			if (!kalMemCmp(WFA_IE(ie1)->aucOui,
				WFA_IE(ie2)->aucOui, 3) &&
			    WFA_IE(ie1)->ucOuiType == WFA_IE(ie2)->ucOuiType)
				return TRUE;
		} else {
			return TRUE;
		}
	}

	return FALSE;
}

uint8_t mldNonInheritElement(uint8_t *ie, struct IE_NON_INHERITANCE *ninh_elem)
{
	uint8_t i, *cp;
	uint8_t *ninh = NULL, ninh_num = 0;
	uint8_t	*ninh_ext = NULL, ninh_ext_num = 0;

	if (!ninh_elem)
		return FALSE;

	cp = ninh_elem->aucList;
	ninh_num = *cp++;
	if (ninh_num > 0) {
		ninh = cp;
		cp += ninh_num;
	}
	ninh_ext_num = *cp++;
	if (ninh_ext_num > 0) {
		ninh_ext = cp;
		cp += ninh_ext_num;
	}

	if (IE_ID(ie) == ELEM_ID_RESERVED) {
		for (i = 0; i < ninh_ext_num; i++) {
			if (ninh_ext[i] == IE_ID_EXT(ie)) {
				DBGLOG(ML, LOUD, "IE(%d, %d) non inh\n",
					IE_ID(ie), IE_ID_EXT(ie));
				return TRUE;
			}
		}
	} else {
		for (i = 0; i < ninh_num; i++) {
			if (ninh[i] == IE_ID(ie)) {
				DBGLOG(ML, LOUD, "IE(%d, %d) non inh\n",
					IE_ID(ie), 0);
				return TRUE;
			}
		}
	}

	return FALSE;
}

int mldParseProfile(uint8_t *ie, uint32_t len, uint8_t *prof,
	uint32_t prof_len, uint8_t *out[], uint8_t *out_num,
	int max_num, uint8_t fgParseMbss)
{
	struct IE_NON_TX_CAP *cap = NULL;
	struct IE_MBSSID_INDEX *idx = NULL;
	struct IE_NON_INHERITANCE *ninh = NULL;
	uint8_t **prof_ies;
	uint8_t ie_count = 0, profile_count = 0;
	int8_t i, need_profile, need_add;
	uint16_t offset = 0;
	int ret = 0;

	prof_ies = kalMemAlloc(sizeof(uint8_t *) * max_num, VIR_MEM_TYPE);
	if (!prof_ies) {
		DBGLOG(ML, WARN, "no mem\n");
		ret = -1;
		goto done;
	}
	kalMemSet(prof_ies, 0, sizeof(uint8_t *) * max_num);

	/* Check profile IE for MBSSID index, and Nontransmitted cap */
	IE_FOR_EACH(prof, prof_len, offset) {
		switch (IE_ID(prof)) {
		case ELEM_ID_NON_TX_CAP:
			cap = NON_TX_CAP_IE(prof);
			break;
		case ELEM_ID_MBSSID_INDEX:
			idx = MBSSID_INDEX_IE(prof);
			break;
		case ELEM_ID_RESERVED:
			if (IE_ID_EXT(prof) == ELEM_EXT_ID_NON_INHERITANCE) {
				ninh = NON_INHERITANCE_IE(prof);
				break;
			}
			kal_fallthrough;
		default:
			prof_ies[profile_count++] = prof;
			break;
		}
		if (profile_count >= max_num)
			break;
	}

	/*not a valid profile*/
	if (fgParseMbss &&
	   (cap == NULL || idx == NULL || idx->ucBSSIDIndex == 0)) {
		DBGLOG(ML, WARN, "mbss but with cap=%p, idx=%p\n", cap, idx);
		ret = -1;
	        goto done;
	}
	IE_FOR_EACH(ie, len, offset) {
		need_profile = FALSE;
		need_add = FALSE;

		/* 80211be D2.3, 9.4.2.45 */
		if (fgParseMbss) {
			switch (IE_ID(ie)) {
			case ELEM_ID_MBSSID:
			case ELEM_ID_MBSSID_INDEX:
				break;
			case ELEM_ID_TIM:
			case ELEM_ID_DS_PARAM_SET:
			case ELEM_ID_IBSS_PARAM_SET:
			case ELEM_ID_COUNTRY_INFO:
			case ELEM_ID_CH_SW_ANNOUNCEMENT:
			case ELEM_ID_EX_CH_SW_ANNOUNCEMENT:
			case ELEM_ID_WIDE_BAND_CHANNEL_SWITCH:
			case ELEM_ID_TX_PWR_ENVELOPE:
			case ELEM_ID_SUP_OPERATING_CLASS:
			case ELEM_ID_IBSS_DFS:
			case ELEM_ID_ERP_INFO:
			case ELEM_ID_HT_CAP:
			case ELEM_ID_HT_OP:
			case ELEM_ID_VHT_CAP:
			case ELEM_ID_VHT_OP:
			case ELEM_ID_S1G_CAP:
			case ELEM_ID_S1G_OP:
				need_add = TRUE;
				break;
			case ELEM_ID_RESERVED:
				/* Check Element ID Extension */
				switch (IE_ID_EXT(ie)) {
				case ELEM_EXT_ID_MBSS_CONFIG:
					break;
				case ELEM_EXT_ID_HE_CAP:
				case ELEM_EXT_ID_HE_OP:
				case ELEM_EXT_ID_HE_6G_BAND_CAP:
				case ELEM_EXT_ID_SR_PARAM:
				case ELEM_EXT_ID_BSS_COLOR_CHANGE:
				case ELEM_EXT_ID_EHT_OP:
				case ELEM_EXT_ID_EHT_CAPS:
					need_add = TRUE;
					break;
				default:
					need_profile = TRUE;
					break;
				}
				break;
			default:
				need_profile = TRUE;
				break;
			}
		} else if (!mldDupProfileSkipIE(ie)) {
			need_profile = TRUE;
		}

		if (need_add) {
			out[ie_count++] = ie;
		} else if (need_profile) {
			/*check if profile has same ie*/
			for (i = 0; i < profile_count; i++) {
				if (mldSameElement(prof_ies[i], ie))
					break;
			}

			if (i < profile_count) {
				/* found ie in profile, use profile version */
				out[ie_count++] = prof_ies[i];
				prof_ies[i] = NULL;
			} else { /* not found, use ie from transmitted beacon */
				if (!mldNonInheritElement(ie, ninh))
					out[ie_count++] = ie;
			}
		}

		if (ie_count == max_num) {
			ret = -1;
			goto done;
		}
	}

	/*check if the ie is in profile but not in transmitted beacon,
	should keep this ie */
	for (i = 0; i < profile_count && ie_count < max_num; i++) {
		if (prof_ies[i] != NULL && IE_ID(prof_ies[i])) {
		    out[ie_count++] = prof_ies[i];
		}
	}
done:
	kalMemFree(prof_ies, VIR_MEM_TYPE, sizeof(uint8_t *) * max_num);
	*out_num = ie_count;
	return ret;
}

int mldDupMbssNonTxProfileImpl(struct ADAPTER *prAdapter,
	struct SW_RFB *prSrc, uint8_t *pucProf, uint8_t u2ProfLen,
	struct SW_RFB *prDst)
{
	int padding;
	uint8_t new_bssid[MAC_ADDR_LEN], lsb;
	struct WLAN_BEACON_FRAME *mgmt;
	struct IE_MBSSID_INDEX *idx = NULL;
	struct IE_MBSSID *mbss = NULL;
	struct IE_NON_TX_CAP *cap = NULL;
	uint8_t i, ie_count, *ie, *ies[MAX_DUP_IE_COUNT] = {0}, *pos, *end;
	size_t len;

	padding = sortGetPayloadOffset(prAdapter, prSrc->pvHeader);
	if (padding < 0)
		return -1;

	mgmt = (struct WLAN_BEACON_FRAME *)prSrc->pvHeader;
	ie = (uint8_t *)prSrc->pvHeader + padding;
	len = prSrc->u2PacketLen - padding;

	if (mldParseProfile(ie, len, pucProf, u2ProfLen,
		ies, &ie_count, MAX_DUP_IE_COUNT, TRUE) < 0)
		return -1;

	mbss = (struct IE_MBSSID *) kalFindIeExtIE(ELEM_ID_MBSSID, 0,
		ie, len);
	idx = (struct IE_MBSSID_INDEX *) kalFindIeExtIE(ELEM_ID_MBSSID_INDEX, 0,
		pucProf, u2ProfLen);
	cap = (struct IE_NON_TX_CAP *) kalFindIeExtIE(ELEM_ID_NON_TX_CAP, 0,
		pucProf, u2ProfLen);

	if (!mbss || !idx) {
		DBGLOG(ML, ERROR, "mbss=%p, idx=%p",
				mbss, idx, cap);
		return -1;
	}

	/* calculate new BSSID */
	COPY_MAC_ADDR(new_bssid, &mgmt->aucBSSID[0]);
	lsb = new_bssid[5] & ((1 << mbss->ucMaxBSSIDIndicator) - 1);
	new_bssid[5] &= ~((1 << mbss->ucMaxBSSIDIndicator) - 1);
	new_bssid[5] |= (lsb + idx->ucBSSIDIndex) &
	              ((1 << mbss->ucMaxBSSIDIndicator) - 1);

	DBGLOG(ML, TRACE, "MBSS new mac: "MACSTR "\n", MAC2STR(new_bssid));

	/* compose RXD, mac header, payload(fixed field)*/
	nicRxCopyRFB(prAdapter, prDst, prSrc);
	pos = (uint8_t *)prDst->pvHeader;
	end = prDst->pucRecvBuff + CFG_RX_MAX_MPDU_SIZE;
	mgmt = (struct WLAN_BEACON_FRAME *)prDst->pvHeader;
	COPY_MAC_ADDR(mgmt->aucSrcAddr, new_bssid);
	COPY_MAC_ADDR(mgmt->aucBSSID, new_bssid);
	mgmt->u2CapInfo = cap ? cap->u2Cap : 0;

	/* compose IE */
	for (i = 0; i < ie_count; i++) {
		len = kal_min_t(size_t, IE_SIZE(ies[i]), IE_SIZE_MAX);

		if (pos + padding + len > end) {
			DBGLOG(ML, WARN, "no rx packet space left\n");
			break;
		}

		kalMemCopy(pos + padding, ies[i], len);

		/* replace dtim count and dtim period of the MBSS to the TIM */
		if (IE_ID(ies[i]) == ELEM_ID_TIM && IE_LEN(idx) == 3) {
			struct IE_TIM *tmp = (struct IE_TIM *)(pos + padding);

			tmp->ucDTIMCount = idx->ucDtimCount;
			tmp->ucDTIMPeriod = idx->ucDtimPeriod;
		}
		padding += len;
	}

	prDst->u2PacketLen = padding;
	prDst->u2RxByteCount = ((uint8_t *)prDst->pvHeader) +
		padding - prDst->pucRecvBuff;

	return 0;
}

struct SW_RFB *mldDupMbssNonTxProfile(struct ADAPTER *prAdapter,
	struct SW_RFB *prSrc)
{
	struct QUE tmp, *que = &tmp;
	uint8_t *pucSubIE, *pucIE;
	uint16_t u2IELen, u2SubIElen, u2SubOffset;
	struct IE_MBSSID *mbss;
	struct SW_RFB *rfb;
	uint8_t ret;
	int offset = sortGetPayloadOffset(prAdapter, prSrc->pvHeader);

	if (offset < 0 || prSrc->u2PacketLen < offset)
		return NULL;

	QUEUE_INITIALIZE(que);
	pucIE = (uint8_t *)prSrc->pvHeader + offset;
	u2IELen = prSrc->u2PacketLen - offset;
	IE_FOR_EACH(pucIE, u2IELen, offset) {
		if (IE_ID(pucIE) != ELEM_ID_MBSSID)
			continue;

		mbss = (struct IE_MBSSID *)pucIE;
		pucSubIE = mbss->ucSubelements;

		if (IE_SIZE(mbss) < sizeof(struct IE_MBSSID))
			continue;

		u2SubIElen = IE_SIZE(mbss) - sizeof(struct IE_MBSSID);
		IE_FOR_EACH(pucSubIE, u2SubIElen, u2SubOffset) {
			if (IE_ID(pucSubIE) != NON_TX_BSSID_PROFILE)
				continue;

			rfb = NIC_RX_ACQUIRE_RFB(prAdapter, 1, RFB_TRACK_MLO);
			if (!rfb)
				break;

			ret = mldDupMbssNonTxProfileImpl(prAdapter,
				prSrc, pucSubIE + 2, IE_LEN(pucSubIE), rfb);
			if (ret == WLAN_STATUS_SUCCESS) {
				QUEUE_INSERT_TAIL(que, &rfb->rQueEntry);
			} else
				nicRxReturnRFB(prAdapter, rfb);
		}
	}

	return QUEUE_GET_HEAD(que);
}

uint32_t mldDupByMlStaProfile(struct ADAPTER *prAdapter, struct SW_RFB *prDst,
			      struct SW_RFB *prSrc, const uint8_t *ml,
			      struct STA_PROFILE *prSta,
			      struct BSS_DESC *prBssDesc,
			      struct STA_RECORD *prStaRec, const char *pucDesc)
{
	int offset;
	struct WLAN_MAC_MGMT_HEADER *mgmt;
	uint8_t i, ie_count, *ie = NULL, *ies[MAX_DUP_IE_COUNT] = {0}, *pos;
	uint16_t fctrl, ie_len;
	uint8_t *addr;

	if (prBssDesc && prBssDesc->fgIsConnected) {
		offset = OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem[0]);
		fctrl = prBssDesc->fgSeenProbeResp ? MAC_FRAME_PROBE_RSP :
			MAC_FRAME_BEACON;
		ie = prBssDesc->pucIeBuf;
		ie_len = prBssDesc->u2IELength;
		addr = prBssDesc->aucBSSID;
	} else if (prSta->ucComplete) {
		offset = sortGetPayloadOffset(prAdapter, prSrc->pvHeader);
		if (offset < 0 || offset > prSrc->u2PacketLen)
			return WLAN_STATUS_INVALID_PACKET;
		mgmt = (struct WLAN_MAC_MGMT_HEADER *)prSrc->pvHeader;
		fctrl = mgmt->u2FrameCtrl & MASK_FRAME_TYPE;
		ie = (uint8_t *)prSrc->pvHeader + offset;
		ie_len = prSrc->u2PacketLen - offset;
		addr = prSta->aucLinkAddr;
	}

	if (!ie) {
		DBGLOG(ML, WARN, "%s no target, complete=%d\n",
			pucDesc, prSta->ucComplete);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	if (pucDesc)
		DBGLOG(ML, TRACE, "%s complete=%d\n",
			pucDesc, prSta->ucComplete);

	if (mldParseProfile(ie, ie_len, prSta->aucIEbuf, prSta->u2IEbufLen,
		ies, &ie_count, MAX_DUP_IE_COUNT, FALSE) < 0)
		return WLAN_STATUS_INVALID_PACKET;

	/* compose RXD, mac header, payload(fixed field)*/
	nicRxCopyRFB(prAdapter, prDst, prSrc);
	pos = (uint8_t *)prDst->pvHeader;
	mgmt = (struct WLAN_MAC_MGMT_HEADER *)prDst->pvHeader;
	COPY_MAC_ADDR(mgmt->aucSrcAddr, addr);
	COPY_MAC_ADDR(mgmt->aucBSSID, addr);
	mgmt->u2FrameCtrl = fctrl;

	if (fctrl == MAC_FRAME_PROBE_RSP || fctrl == MAC_FRAME_BEACON) {
		struct WLAN_BEACON_FRAME *bcn = prDst->pvHeader;

		if (prSta->ucComplete) {
			uint64_t u8Timestamp;

			WLAN_GET_FIELD_64(bcn->au4Timestamp, &u8Timestamp);
			bcn->u2CapInfo = prSta->u2CapInfo;
			WLAN_SET_FIELD_64(bcn->au4Timestamp,
				u8Timestamp + prSta->i8TsfOffset * 2);
			bcn->u2BeaconInterval = prSta->u2BcnIntv;

			DBGLOG(ML, TRACE,
				"tsf=%llu offset=%ld new=%llu bcnInt=%d\n",
				u8Timestamp, prSta->i8TsfOffset,
				*((uint64_t *)bcn->au4Timestamp),
				bcn->u2BeaconInterval);
		} else if (prBssDesc) {
			bcn->u2CapInfo = prBssDesc->u2CapInfo;
			WLAN_SET_FIELD_64(bcn->au4Timestamp,
				prBssDesc->u8TimeStamp.QuadPart);
			bcn->u2BeaconInterval = prBssDesc->u2BeaconInterval;

			DBGLOG(ML, TRACE, "tsf=%llu bcnInt=%d\n",
				*((uint64_t *)bcn->au4Timestamp),
				bcn->u2BeaconInterval);
		}
	} else if (prStaRec) {
		struct BSS_INFO *bss =
			GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

		if (!bss) {
			DBGLOG(ML, WARN, "Bss is NULL!");
			goto done;
		}

		if (fctrl == MAC_FRAME_ASSOC_REQ ||
		    fctrl == MAC_FRAME_REASSOC_REQ)
			COPY_MAC_ADDR(mgmt->aucBSSID, bss->aucOwnMacAddr);
		else if (fctrl == MAC_FRAME_ASSOC_RSP ||
			 fctrl == MAC_FRAME_REASSOC_RSP)
			COPY_MAC_ADDR(mgmt->aucBSSID, bss->aucBSSID);
		COPY_MAC_ADDR(mgmt->aucDestAddr, bss->aucOwnMacAddr);
		prDst->ucWlanIdx = prStaRec->ucWlanIndex;
		prDst->ucStaRecIdx = prStaRec->ucIndex;
		prDst->prStaRec = prStaRec;
	}

	DBGLOG(ML, LOUD, "header total %d ies\n", ie_count);
	DBGLOG_MEM8(ML, LOUD, pos, offset);

	/* compose IE */
	for (i = 0; i < ie_count; i++) {
		DBGLOG_MEM8(ML, LOUD, ies[i], IE_SIZE(ies[i]));
		kalMemCopy(pos + offset, ies[i], IE_SIZE(ies[i]));
		offset += IE_SIZE(ies[i]);
	}

	DBGLOG(ML, LOUD, "total len=%d\n", offset);

	/* copy common info for scan result, don't include per sta profile other
	 * scanProcessBeaconAndProbeResp will enter infinite loop
	 */
	if (fctrl == MAC_FRAME_PROBE_RSP || fctrl == MAC_FRAME_BEACON) {
		struct IE_MULTI_LINK_CONTROL *src, *dst;

		src = (struct IE_MULTI_LINK_CONTROL *) ml;
		dst = (struct IE_MULTI_LINK_CONTROL *)(pos + offset);

		if (!src) {
			DBGLOG(ML, ERROR, "no ml ie\n");
			goto done;
		}

		kalMemCopy(dst, src, sizeof(*src) + src->aucCommonInfo[0]);
		/* ext + ctrl + common info length */
		dst->ucLength = 3 + src->aucCommonInfo[0];
		if (BE_IS_ML_CTRL_PRESENCE_LINK_ID(src->u2Ctrl))
			dst->aucCommonInfo[1 + MAC_ADDR_LEN] = prSta->ucLinkId;


		DBGLOG(ML, LOUD, "dst ml len=%d\n", IE_SIZE(dst));
		DBGLOG_MEM8(ML, LOUD, (uint8_t *)dst, IE_SIZE(dst));

		offset += IE_SIZE(dst);
	}

done:
	prDst->u2PacketLen = offset;
	prDst->u2RxByteCount = ((uint8_t *)prDst->pvHeader) +
		offset - prDst->pucRecvBuff;
	prDst->ucChnlNum = prBssDesc ? prBssDesc->ucChannelNum :
		prSta->rChnlInfo.ucChannelNum;
	prDst->eRfBand = prBssDesc ? prBssDesc->eBand :
		prSta->rChnlInfo.eBand;

	DBGLOG(ML, INFO,
		"Duplicated SwRFB for id=%d addr="
		MACSTR " len=%d, chnl=%d, band=%d\n",
		prSta->ucLinkId, MAC2STR(addr),
		offset, prDst->ucChnlNum, prDst->eRfBand);
	DBGDUMP_MEM8(ML, TRACE, "Duplicated SwRFB\n", pos, offset);

	return WLAN_STATUS_SUCCESS;
}

struct SW_RFB *mldDupProbeRespSwRfb(struct ADAPTER *prAdapter,
				  struct SW_RFB *prSrc)
{
	uint16_t u2Offset = 0;
	uint8_t *ie;
	uint16_t ie_len;
	struct QUE tmp, *que = &tmp;
	struct STA_PROFILE *sta;
	struct MULTI_LINK_INFO parse, *info = &parse;
	struct IE_RNR *rnr;
	struct WLAN_MAC_MGMT_HEADER *mgmt =
		(struct WLAN_MAC_MGMT_HEADER *)prSrc->pvHeader;
	struct SW_RFB *rfb;
	int offset = sortGetPayloadOffset(prAdapter, prSrc->pvHeader);
	uint8_t i, ret;
	const uint8_t *ml, *ssid, *start, *end;
	struct PARAM_SSID rSsid;

	if (offset < 0 || offset > prSrc->u2PacketLen)
		return NULL;

	QUEUE_INITIALIZE(que);

	start = (uint8_t *)prSrc->pvHeader + offset;
	end = (uint8_t *)prSrc->pvHeader + prSrc->u2PacketLen;

	/* ML probe resp for MLO + MBSSID can have 2 ML elem */
	for (i = 0; i < 2; i++) {
		ml = mldFindMlIE(start, end - start, ML_CTRL_TYPE_BASIC);
		if (!ml)
			return NULL;

		/* parsing rnr & ml */
		MLD_PARSE_BASIC_MLIE(info, ml, end - ml,
			mgmt->aucBSSID, mgmt->u2FrameCtrl & MASK_FRAME_TYPE);

		/* complete ml elem found */
		if (info->ucProfNum != 0)
			break;

		/* next start after ml ie*/
		start = ml + IE_SIZE(ml);
	}

	if (info->ucProfNum == 0) {
		DBGLOG(ML, LOUD, "no per sta profile\n");
		return NULL;
	}

	ie = (uint8_t *)prSrc->pvHeader + offset;
	ie_len = prSrc->u2PacketLen - offset;
	IE_FOR_EACH(ie, ie_len, u2Offset) {
		if (IE_ID(ie) != ELEM_ID_RNR)
			continue;

		rnr = (struct IE_RNR *)ie;
		mldHandleRnrMlParam(rnr, info, FALSE, prSrc->u2PayloadLength);
	}

	kalMemZero(&rSsid, sizeof(rSsid));
	ssid = kalFindIeMatchMask(ELEM_ID_SSID,
	       (uint8_t *)prSrc->pvHeader + offset,
	       prSrc->u2PacketLen - offset,
	       NULL, 0, 0, NULL);
	if (ssid)
		COPY_SSID(rSsid.aucSsid,
			  rSsid.u4SsidLen,
			  SSID_IE(ssid)->aucSSID,
			  SSID_IE(ssid)->ucLength);

	for (i = 0; i < info->ucProfNum; i++) {
		struct BSS_DESC *prBssDesc;

		/* no need to dup again if profile is for trainsmiting ap */
		sta = &info->rStaProfiles[i];
		if (EQUAL_MAC_ADDR(mgmt->aucBSSID, sta->aucLinkAddr))
			continue;

		/* if already exist, try to update bssdesc */
		prBssDesc = scanSearchBssDescByLinkIdMldAddrSsid(prAdapter,
			sta->ucLinkId, info->aucMldAddr,
			ssid ? TRUE : FALSE, &rSsid);
		/* already have complete profile, no need to dup */
		if (prBssDesc &&
		    wlanNumBitSet(prBssDesc->rMlInfo.u2ValidLinks) > 1)
			continue;

		rfb = NIC_RX_ACQUIRE_RFB(prAdapter, 1, RFB_TRACK_MLO);
		if (!rfb)
			break;

		ret = mldDupByMlStaProfile(prAdapter, rfb, prSrc, ml,
			sta, prBssDesc, NULL, __func__);
		if (ret == WLAN_STATUS_SUCCESS) {
			rfb->fgDriverGen = TRUE;
			QUEUE_INSERT_TAIL(que, &rfb->rQueEntry);
		} else {
			nicRxReturnRFB(prAdapter, rfb);
		}
	}

	return QUEUE_GET_HEAD(que);
}

uint8_t mldProcessBeaconAndProbeResp(
		struct ADAPTER *prAdapter, struct SW_RFB *prSrc)
{
	struct QUE tmp, *que = &tmp;
	struct SW_RFB *rfb = NULL;
	uint8_t fgHasMLElement = FALSE;

	QUEUE_INITIALIZE(que);

	rfb = mldDupProbeRespSwRfb(prAdapter, prSrc);
	QUEUE_INSERT_TAIL_ALL(que, rfb);

	if (rfb) {
		fgHasMLElement = TRUE;
	}

#if CFG_SUPPORT_802_11V_MBSSID && !CFG_SUPPORT_802_11V_MBSSID_OFFLOAD
	/* duplicate after ml probe resp. if done, skip mbss */
	if (!rfb) {
		rfb = mldDupMbssNonTxProfile(prAdapter, prSrc);
		QUEUE_INSERT_TAIL_ALL(que, rfb);
	}
#endif

	while(QUEUE_IS_NOT_EMPTY(que)) {
		QUEUE_REMOVE_HEAD(que, rfb, struct SW_RFB *);
		scanProcessBeaconAndProbeResp(prAdapter, rfb);
		nicRxReturnRFB(prAdapter, rfb);
	}

	return fgHasMLElement;
}

struct SW_RFB *mldDupAssocSwRfb(struct ADAPTER *prAdapter,
	struct SW_RFB *prSrc, struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *bss;
	struct STA_PROFILE *sta = NULL;
	struct MULTI_LINK_INFO parse, *info = &parse;
	struct SW_RFB *rfb = NULL;
	struct WLAN_MAC_MGMT_HEADER *mgmt =
		(struct WLAN_MAC_MGMT_HEADER *)prSrc->pvHeader;
	int offset = sortGetPayloadOffset(prAdapter, prSrc->pvHeader);
	const uint8_t *ml;
	uint8_t i;
	uint32_t ret;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!bss) {
		DBGLOG(ML, INFO, "AA but bss is null\n");
		goto fail;
	}

	if (offset < 0) {
		DBGLOG(ML, INFO, "Can't get the valid payload offset\n");
		goto fail;
	}

	ml = mldFindMlIE((uint8_t *)prSrc->pvHeader + offset,
		prSrc->u2PacketLen - offset, ML_CTRL_TYPE_BASIC);

	if (!ml) {
		DBGLOG(ML, INFO, "AA but no ml ie\n");
		goto fail;
	}

	MLD_PARSE_BASIC_MLIE(info, ml,
		(uint8_t *)prSrc->pvHeader + prSrc->u2PacketLen - (uint8_t *)ml,
		mgmt->aucBSSID,
		mgmt->u2FrameCtrl & MASK_FRAME_TYPE);

	for (i = 0; i < info->ucProfNum; i++) {
		sta = &info->rStaProfiles[i];
		if (sta->ucLinkId == prStaRec->ucLinkIndex &&
		    EQUAL_MAC_ADDR(sta->aucLinkAddr, prStaRec->aucMacAddr)) {
			break;
		}
	}

	if (i >= info->ucProfNum) {
		DBGLOG(ML, INFO, "AA but no matched id\n");
		goto fail;
	}

	/* skip if no complete info */
	if (!sta->ucComplete) {
		DBGLOG(ML, INFO, "not complete\n");
		goto fail;
	}

	sta->rChnlInfo.ucChannelNum = bss->ucPrimaryChannel;
	sta->rChnlInfo.eBand = bss->eBand;

	rfb = NIC_RX_ACQUIRE_RFB(prAdapter, 1, RFB_TRACK_MLO);
	if (!rfb) {
		DBGLOG(ML, INFO, "no rfb\n");
		goto fail;
	}

	ret = mldDupByMlStaProfile(prAdapter, rfb, prSrc, ml,
		sta, NULL, prStaRec, __func__);
	if (ret == WLAN_STATUS_SUCCESS)
		return rfb;
fail:
	nicRxReturnRFB(prAdapter, rfb);
	return NULL;
}

int mldDump(struct ADAPTER *prAdapter, uint8_t ucIndex,
	char *pcCommand, int i4TotalLen)
{
	int32_t i4BytesWritten = 0;
	uint8_t i = 0, j = 0;

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\nMldLinkMax:%d\nStaMldLinkMax:%d\nApMldLinkMax:%d\nP2pMldLinkMax:%d\nEnableMlo:%d\nStaMldEMLCap:0x%x\nApMldEMLCap:0x%x\nEmlsrLinkWeight:%d\n",
		prAdapter->rWifiVar.ucMldLinkMax,
		prAdapter->rWifiVar.ucStaMldLinkMax,
		prAdapter->rWifiVar.ucApMldLinkMax,
		prAdapter->rWifiVar.ucP2pMldLinkMax,
		prAdapter->rWifiVar.ucEnableMlo,
		prAdapter->rWifiVar.u2NonApMldEMLCap,
		prAdapter->rWifiVar.u2ApMldEMLCap,
		prAdapter->rWifiVar.ucEmlsrLinkWeight);

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"StaEHT:%d\nApEHT:%d\nP2pGoEHT:%d\nP2pGcEHT:%d\n",
		prAdapter->rWifiVar.ucStaEht,
		prAdapter->rWifiVar.ucApEht,
		prAdapter->rWifiVar.ucP2pGoEht,
		prAdapter->rWifiVar.ucP2pGcEht);

	i4BytesWritten += kalSnprintf(
		pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"BSS:");

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (i != MAX_BSSID_NUM - 1)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%d,", prAdapter->aprBssInfo[i]->fgIsInUse);
		else
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"%d\n\n", prAdapter->aprBssInfo[i]->fgIsInUse);
		/* log only */
		cnmDumpBssInfo(prAdapter, i);
	}

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldBssInfo); i++) {
		struct MLD_BSS_INFO *prMldBssInfo;
		struct BSS_INFO *prBssInfo;
		struct LINK *prBssList;

		prMldBssInfo = &prAdapter->aprMldBssInfo[i];

		/* only dump specefic mld bssinfo */
		if (!prMldBssInfo->fgIsInUse ||
		    (ucIndex != MLD_GROUP_NONE &&
		     ucIndex != prMldBssInfo->ucGroupMldId))
			continue;

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"MldBssInfo[%d]:\n", i);

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"OMAC_ID/GRP_MLD_ID/MAX_SIMU/OWN_EML_CAP/OM_REMAP_ID:%u/%u/%u/0x%x/%u\n",
			prMldBssInfo->ucOmacIdx,
			prMldBssInfo->ucGroupMldId,
			prMldBssInfo->ucMaxSimuLinks,
			prMldBssInfo->ucEmlEnabled ? prMldBssInfo->u2EMLCap : 0,
			prMldBssInfo->ucOmRemapIdx);

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"BSS_BITMAP/OWN_MLD_ADDR:0x%02x/" MACSTR "\n{\n",
			prMldBssInfo->ucBssBitmap,
			MAC2STR(prMldBssInfo->aucOwnMldAddr));

		prBssList = &prMldBssInfo->rBssList;
		LINK_FOR_EACH_ENTRY(prBssInfo, prBssList, rLinkEntryMld,
		    struct BSS_INFO) {
			cnmDumpBssInfo(prAdapter, prBssInfo->ucBssIndex);
			bssDumpBssInfo(prAdapter, prBssInfo->ucBssIndex);
			i4BytesWritten += cnmShowBssInfo(prAdapter,
				prBssInfo, pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten);
			if (prBssInfo != LINK_PEEK_TAIL(
				prBssList, struct BSS_INFO, rLinkEntryMld))
				i4BytesWritten += kalSnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"\n");
		}

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"}\n\n");
	}

	if (ucIndex == MLD_GROUP_NONE) {
		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"Skip showing MldStaRec ...\n");
		goto done;
	}

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldStarec); i++) {
		struct MLD_STA_RECORD *prMldStarec =
			&prAdapter->aprMldStarec[i];
		struct STA_RECORD *prCurrStarec;
		struct LINK *prStarecList;

		/* only dump specefic mld starec */
		if (!prMldStarec->fgIsInUse ||
		    (ucIndex != MLD_GROUP_NONE &&
		     ucIndex != prMldStarec->ucGroupMldId))
			continue;

		i4BytesWritten += kalSnprintf(
		       pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		       "MldStaRec[%d]:\n", prMldStarec->ucIdx);

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"PRI/SEC/SETUP/ACT_BMP/PEER_MLD:%d/%d/%d/%d/"MACSTR"\n",
			prMldStarec->u2PrimaryMldId,
			prMldStarec->u2SecondMldId,
			prMldStarec->u2SetupWlanId,
			prMldStarec->u4ActiveStaBitmap,
			MAC2STR(prMldStarec->aucPeerMldAddr));
		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"EML_CAP/STR/TYPE:0x%04x/0x%02x%04x/%d\n",
			prMldStarec->u2EmlCap,
			*(uint8_t *)(prMldStarec->aucStrBitmap + 2),
			*(uint16_t *)(prMldStarec->aucStrBitmap),
			prMldStarec->fgMldType);
		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"RX_PKT_COUNT:\n");
		for (j = 0; j < ARRAY_SIZE(prMldStarec->aucRxPktCnt); j++)
			i4BytesWritten += kalSnprintf(
				pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"   BAND%d:0x%llx\n",
				j, prMldStarec->aucRxPktCnt[j]);

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"{\n");

		prStarecList = &prMldStarec->rStarecList;
		LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList, rLinkEntryMld,
		    struct STA_RECORD) {
			cnmDumpStaRec(prAdapter, prCurrStarec->ucIndex);
			i4BytesWritten += cnmShowStaRec(prAdapter,
				prCurrStarec, pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten);
			if (prCurrStarec != LINK_PEEK_TAIL(
				prStarecList, struct STA_RECORD, rLinkEntryMld))
				i4BytesWritten += kalSnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"\n");
		}

		i4BytesWritten += kalSnprintf(
			pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"}\n\n");
	}

done:
	return i4BytesWritten;
}

void mldBssAddClient(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct MLD_STA_RECORD *prMldStaRec)
{
	struct LINK *prClientList;
	struct MLD_STA_RECORD *prCurrMldStaRec;

	if (!prMldBssInfo)
		return;

	prClientList = &prMldBssInfo->rMldStaRecOfClientList;
	LINK_FOR_EACH_ENTRY(prCurrMldStaRec, prClientList, rLinkEntry,
			    struct MLD_STA_RECORD) {
		if (prCurrMldStaRec->ucIdx == prMldStaRec->ucIdx) {
			DBGLOG(ML, INFO,
			       "MldBssInfo%d already contain MLD_STA_RECORD["
			       MACSTR " idx=%d grpMldId=%d] before removing.\n",
			       prMldBssInfo->ucGroupMldId,
			       MAC2STR(prMldStaRec->aucPeerMldAddr),
			       prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId);
			return;
		}
	}

	LINK_ENTRY_INITIALIZE(&prMldStaRec->rLinkEntry);
	LINK_INSERT_TAIL(prClientList, &prMldStaRec->rLinkEntry);

	DBGLOG(ML, INFO,
		"MldBssInfo%d add client["MACSTR
		" idx=%d grpMldId=%d], total=%d\n",
		prMldBssInfo->ucGroupMldId,
		MAC2STR(prMldStaRec->aucPeerMldAddr),
		prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId,
		prClientList->u4NumElem);
}

uint8_t mldBssRemoveClient(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct MLD_STA_RECORD *prMldStaRec)
{
	struct LINK *prClientList;
	struct MLD_STA_RECORD *prCurrMldStaRec, *prNextMldStaRec;

	if (!prMldBssInfo)
		return TRUE;

	prClientList = &prMldBssInfo->rMldStaRecOfClientList;
	LINK_FOR_EACH_ENTRY_SAFE(prCurrMldStaRec, prNextMldStaRec, prClientList,
		rLinkEntry, struct MLD_STA_RECORD) {
		if (prCurrMldStaRec->ucIdx == prMldStaRec->ucIdx) {
			LINK_REMOVE_KNOWN_ENTRY(prClientList,
						&prMldStaRec->rLinkEntry);
			DBGLOG(ML, INFO,
				"MldBssInfo%d remove client["MACSTR
				" idx=%d grpMldId=%d], total=%d\n",
				prMldBssInfo->ucGroupMldId,
				MAC2STR(prMldStaRec->aucPeerMldAddr),
				prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId,
				prClientList->u4NumElem);
			return TRUE;
		}
	}

	DBGLOG(ML, INFO,
	       "MldBssInfo%d didn't contain MLD_STA_RECORD["
	       MACSTR " idx=%d grpMldId=%d] before removing.\n",
	       prMldBssInfo->ucGroupMldId,
	       MAC2STR(prMldStaRec->aucPeerMldAddr),
	       prMldStaRec->ucIdx, prMldStaRec->ucGroupMldId);
	return FALSE;
}

struct MLD_STA_RECORD *mldBssGetPeekClient(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo)
{
	struct LINK *prClientList;

	if (!prMldBssInfo)
		return NULL;

	prClientList = &prMldBssInfo->rMldStaRecOfClientList;

	return LINK_PEEK_HEAD(prClientList, struct MLD_STA_RECORD, rLinkEntry);
}

void mldBssDump(struct ADAPTER *prAdapter)
{
	struct MLD_BSS_INFO *prMldBssInfo;
	uint8_t i = 0;

	DBGLOG(ML, INFO, "========== START ==========\n");

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldBssInfo); i++) {
		struct BSS_INFO *prBssInfo;
		struct LINK *prBssList;

		prMldBssInfo = &prAdapter->aprMldBssInfo[i];

		if (!prMldBssInfo->fgIsInUse)
			continue;

		DBGLOG(ML, INFO, "[%d] om:%d, group:%d, om_remap:%d, mld_mac:" MACSTR "\n",
			i,
			prMldBssInfo->ucOmacIdx,
			prMldBssInfo->ucGroupMldId,
			prMldBssInfo->ucOmRemapIdx,
			MAC2STR(prMldBssInfo->aucOwnMldAddr));

		DBGLOG(ML, INFO, "\tBss list:\n");
		prBssList = &prMldBssInfo->rBssList;
		LINK_FOR_EACH_ENTRY(prBssInfo, prBssList, rLinkEntryMld,
		    struct BSS_INFO) {
			cnmDumpBssInfo(prAdapter, prBssInfo->ucBssIndex);
			bssDumpBssInfo(prAdapter, prBssInfo->ucBssIndex);
		}
		DBGLOG(ML, INFO, "\n");
	}

	DBGLOG(ML, INFO, "========== END ==========\n");
}

void mldBssUpdateMldAddr(
	struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	const uint8_t *paucBssId)
{
	if (!prAdapter || !prMldBssInfo)
		return;

	DBGLOG(ML, INFO,
		"prMldBssInfo: %d, macAddr: " MACSTR "\n",
		prMldBssInfo->ucGroupMldId, MAC2STR(paucBssId));

	COPY_MAC_ADDR(prMldBssInfo->aucOwnMldAddr, paucBssId);
}

void mldBssUpdateMldAddrByMainBss(
	struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo)
{
	struct BSS_INFO *prBssInfo = NULL;

	if (!prAdapter || !prMldBssInfo ||
	    LINK_IS_EMPTY(&prMldBssInfo->rBssList))
		return;

	prBssInfo = LINK_PEEK_HEAD(&(prMldBssInfo->rBssList),
					struct BSS_INFO,
					rLinkEntryMld);

	if (prBssInfo)
		mldBssUpdateMldAddr(prAdapter,
			prMldBssInfo, prBssInfo->aucOwnMacAddr);
	else
		DBGLOG(ML, ERROR,
			"bssinfo not found with NumElem=%d\n",
			prMldBssInfo->rBssList.u4NumElem);
}

void mldBssUpdateOmacIdx(
	struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBssInfo)
{
	if (!prAdapter || !prMldBssInfo ||
	    LINK_IS_EMPTY(&prMldBssInfo->rBssList))
		return;

	if (prMldBssInfo->ucOmacIdx == INVALID_OMAC_IDX) {
		struct BSS_INFO *prMainBssInfo =
			LINK_PEEK_HEAD(&(prMldBssInfo->rBssList),
					struct BSS_INFO, rLinkEntryMld);

		prMldBssInfo->ucOmacIdx = prMainBssInfo->ucOwnMacIndex;
	}

#if (CFG_SUPPORT_MLO_HYBRID == 1)
	if (prMldBssInfo->ucHmloEnabled) {
		DBGLOG(ML, INFO, "Hybird MLO use BssInfo omac idx %d\n",
			prBssInfo->ucOwnMacIndex);
		return;
	}
#endif

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
	if (prMldBssInfo->fgIsSbMlsr)
		return;
#endif /* CFG_SINGLE_BAND_MLSR_56 */

	DBGLOG(ML, INFO, "Use mld omac idx %d instead\n",
		prMldBssInfo->ucOmacIdx);
	prBssInfo->ucOwnMacIndex = prMldBssInfo->ucOmacIdx;
}

/**
 * Mark prMldBssInfo->ucHwBandBitmap with all the affiliated links with
 * eHwBandIdx bit mask.
 */
void mldBssUpdateBandIdxBitmap(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo)
{
	struct MLD_BSS_INFO *prMldBssInfo;
	struct BSS_INFO *prCurrBssInfo;
	struct MLD_STA_RECORD *prMldCurrStaRec;
	struct LINK *prBssList = NULL;
	struct LINK *prClientList = NULL;

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (!prMldBssInfo)
		return;

	prBssList = &prMldBssInfo->rBssList;
	prClientList = &prMldBssInfo->rMldStaRecOfClientList;

	prMldBssInfo->ucHwBandBitmap = 0;
	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList, rLinkEntryMld,
			struct BSS_INFO) {
		if (prCurrBssInfo->eHwBandIdx >= ENUM_BAND_NUM)
			continue;
		prMldBssInfo->ucHwBandBitmap |= BIT(prCurrBssInfo->eHwBandIdx);
	}

	/* update mld starec str bitmap */
	LINK_FOR_EACH_ENTRY(prMldCurrStaRec, prClientList, rLinkEntry,
			    struct MLD_STA_RECORD) {
		mldStarecUpdateMldId(prAdapter, prMldCurrStaRec);
	}
}

void mldBssUpdateCap(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	void *pvParam)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prMldBssInfo || !prMldBssInfo->fgIsInUse)
		return;

	prBssInfo = LINK_PEEK_HEAD(&(prMldBssInfo->rBssList),
				struct BSS_INFO, rLinkEntryMld);
	if (!prBssInfo)
		goto done;

	if (!IS_BSS_AIS(prBssInfo)) {
		/* update max simu links num */
		if (prMldBssInfo->rBssList.u4NumElem == 0)
			prMldBssInfo->ucMaxSimuLinks = 0;
		else
			prMldBssInfo->ucMaxSimuLinks =
				prMldBssInfo->rBssList.u4NumElem - 1;

		/* update eml cap */
		prMldBssInfo->ucEmlEnabled = FALSE;
		prMldBssInfo->u2EMLCap = 0;
	} else {
		struct BSS_DESC_SET *prBssDescSet =
			(struct BSS_DESC_SET *)pvParam;

		if (!prBssDescSet)
			goto done;

		/* update max simu links num */
		prMldBssInfo->ucMaxSimuLinks =
			prBssDescSet->ucMaxSimuLinks;

		/* update eml cap */
		if (prBssDescSet->eMloMode == MLO_MODE_EMLSR ||
			prBssDescSet->eMloMode == MLO_MODE_HYEMLSR) {
			prMldBssInfo->ucEmlEnabled = TRUE;
			prMldBssInfo->u2EMLCap =
				prAdapter->rWifiVar.u2NonApMldEMLCap;
		} else {
			prMldBssInfo->ucEmlEnabled = FALSE;
			prMldBssInfo->u2EMLCap = 0;
		}

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
		if (prBssDescSet->eMloMode == MLO_MODE_SB_MLSR)
			prMldBssInfo->fgIsSbMlsr = TRUE;
		else
			prMldBssInfo->fgIsSbMlsr = FALSE;
#endif /* CFG_SINGLE_BAND_MLSR_56 */

#if (CFG_SUPPORT_MLO_HYBRID == 1)
		if (prBssDescSet->eMloMode == MLO_MODE_HYMLO ||
			prBssDescSet->eMloMode == MLO_MODE_HYEMLSR) {
			prMldBssInfo->ucHmloEnabled = TRUE;
			prMldBssInfo->ucOmRemapIdx = prMldBssInfo->ucOmacIdx;
		} else {
			prMldBssInfo->ucHmloEnabled = FALSE;
		}
#endif
	}

done:
	/* set by config strictly */
	if (prWifiVar->ucMaxSimuLinks != 0xff)
		prMldBssInfo->ucMaxSimuLinks = prWifiVar->ucMaxSimuLinks;

	prMldBssInfo->ucMaxSimuLinks =
		KAL_MIN(prWifiVar->ucMaxSimuLinksCap,
			prMldBssInfo->ucMaxSimuLinks);

	DBGLOG(ML, INFO, "EmlEnable: %d, Hybird Enable:%d,MaxSimuLinks:%d\n",
		prMldBssInfo->ucEmlEnabled,
		prMldBssInfo->ucHmloEnabled,
		prMldBssInfo->ucMaxSimuLinks);
}

void mldBssRestoreCap(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo)
{
	struct MLD_STA_RECORD *prMldStaRec =
		mldBssGetPeekClient(prAdapter, prMldBssInfo);

	if (!prMldStaRec)
		return;

	/* update max simu links num */
	prMldBssInfo->ucMaxSimuLinks = prMldStaRec->ucMaxSimuLinks;

	/* update eml cap */
	if (prMldStaRec->ucEmlEnabled) {
		prMldBssInfo->ucEmlEnabled = TRUE;
		prMldBssInfo->u2EMLCap =
			prAdapter->rWifiVar.u2NonApMldEMLCap;
	} else {
		prMldBssInfo->ucEmlEnabled = FALSE;
		prMldBssInfo->u2EMLCap = 0;
	}
}

int8_t mldBssRegister(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBssInfo)
{
	struct LINK *prBssList = NULL;

	if (!prMldBssInfo ||
	    prMldBssInfo->ucGroupMldId == prBssInfo->ucGroupMldId)
		return -EINVAL;

	prBssList = &prMldBssInfo->rBssList;

	DBGLOG(ML, INFO, "bss=%d,type=%d,omac=%d,OwnMldId=%d,GrpMlId=%d\n",
		prBssInfo->ucBssIndex,
		prBssInfo->eNetworkType,
		prBssInfo->ucOwnMacIndex,
		prBssInfo->ucOwnMldId,
		prMldBssInfo->ucGroupMldId);

	prBssInfo->ucGroupMldId = prMldBssInfo->ucGroupMldId;
	prMldBssInfo->ucBssBitmap |= BIT(prBssInfo->ucBssIndex);
	LINK_INSERT_TAIL(prBssList, &prBssInfo->rLinkEntryMld);

	mldBssUpdateCap(prAdapter, prMldBssInfo, NULL);
	mldBssUpdateOmacIdx(prAdapter, prMldBssInfo, prBssInfo);

	return 0;
}

void mldBssUnregister(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBss)
{
	struct BSS_INFO *prCurrBssInfo, *prNextBssInfo;
	struct LINK *prBssList = NULL;

	if (!prMldBssInfo)
		return;

	prBssList = &prMldBssInfo->rBssList;

	DBGLOG(ML, INFO, "prMldBssInfo: %d, prBss: %d\n",
		prMldBssInfo->ucGroupMldId, prBss->ucBssIndex);

	prMldBssInfo->ucBssBitmap &= ~BIT(prBss->ucBssIndex);
	if (prBss->eHwBandIdx < ENUM_BAND_NUM)
		prMldBssInfo->ucHwBandBitmap &= ~BIT(prBss->eHwBandIdx);
	LINK_FOR_EACH_ENTRY_SAFE(prCurrBssInfo, prNextBssInfo, prBssList,
			rLinkEntryMld, struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		if (prBss != prCurrBssInfo)
			continue;

		LINK_REMOVE_KNOWN_ENTRY(prBssList,
			&prCurrBssInfo->rLinkEntryMld);
	}

	mldBssUpdateCap(prAdapter, prMldBssInfo, NULL);
}

struct MLD_BSS_INFO *mldBssAlloc(struct ADAPTER *prAdapter,
	const uint8_t aucMldMacAddr[])
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldBssInfo); i++) {
		if (prAdapter->aprMldBssInfo[i].fgIsInUse)
			continue;

		prMldBssInfo = &prAdapter->aprMldBssInfo[i];
		kalMemZero(prMldBssInfo, sizeof(*prMldBssInfo));
		LINK_INITIALIZE(&prMldBssInfo->rBssList);
		LINK_INITIALIZE(&prMldBssInfo->rMldStaRecOfClientList);
		prMldBssInfo->fgIsInUse = TRUE;
		prMldBssInfo->ucGroupMldId = i;
		prMldBssInfo->ucOmRemapIdx = OM_REMAP_IDX_NONE;
		prMldBssInfo->ucOmacIdx = INVALID_OMAC_IDX;

		COPY_MAC_ADDR(prMldBssInfo->aucOwnMldAddr, aucMldMacAddr);

		DBGLOG(ML, INFO,
			"ucGroupMldId: %d, ucOmRemapIdx: %d, MldMacAddr: "
			MACSTR "\n",
			prMldBssInfo->ucGroupMldId,
			prMldBssInfo->ucOmRemapIdx,
			MAC2STR(aucMldMacAddr));

		break;
	}

	return prMldBssInfo;
}

void mldBssFree(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo)
{
	struct BSS_INFO *prCurrBssInfo, *prNextBssInfo;
	struct LINK *prBssList = NULL;

	if (!prMldBssInfo)
		return;

	prBssList = &prMldBssInfo->rBssList;

	DBGLOG(ML, INFO, "ucGroupMldId: %d\n",
		prMldBssInfo->ucGroupMldId);

	LINK_FOR_EACH_ENTRY_SAFE(prCurrBssInfo, prNextBssInfo, prBssList,
			rLinkEntryMld, struct BSS_INFO) {
		if (!prCurrBssInfo)
			break;

		LINK_REMOVE_KNOWN_ENTRY(prBssList,
			&prCurrBssInfo->rLinkEntryMld);
	}
	prMldBssInfo->fgIsInUse = FALSE;
	prMldBssInfo->ucOmacIdx = INVALID_OMAC_IDX;
}

uint8_t mldBssAllowReconfig(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo)
{
	uint8_t i;

	/* already multi link */
	if (prMldBssInfo && prMldBssInfo->rBssList.u4NumElem > 1)
		return TRUE;

	/* currently, support only 1 multi-link mlo */
	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldBssInfo); i++) {
		struct MLD_BSS_INFO *mld = &prAdapter->aprMldBssInfo[i];

		if (mld->fgIsInUse && mld->rBssList.u4NumElem > 1)
			return FALSE;
	}

	return TRUE;
}

struct MLD_BSS_INFO *mldBssGetByBss(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;

	if (!prBssInfo || prBssInfo->ucGroupMldId == MLD_GROUP_NONE)
		return NULL;

	prMldBssInfo = &prAdapter->aprMldBssInfo[prBssInfo->ucGroupMldId];

	if (prMldBssInfo->fgIsInUse)
		return prMldBssInfo;
	else
		return NULL;
}

struct MLD_BSS_INFO *mldBssGetByIdx(struct ADAPTER *prAdapter,
	uint8_t ucIdx)
{
	struct MLD_BSS_INFO *prMldBssInfo = NULL;

	if (ucIdx == MLD_GROUP_NONE || ucIdx >= MAX_BSSID_NUM)
		return NULL;

	prMldBssInfo = &prAdapter->aprMldBssInfo[ucIdx];

	if (prMldBssInfo->fgIsInUse)
		return prMldBssInfo;
	else
		return NULL;
}

int8_t mldBssInit(struct ADAPTER *prAdapter)
{
	DBGLOG(ML, INFO, "Total %lu MldBssInfo\n",
		ARRAY_SIZE(prAdapter->aprMldBssInfo));
	kalMemZero(prAdapter->aprMldBssInfo, sizeof(prAdapter->aprMldBssInfo));
	return 0;
}

void mldBssUninit(struct ADAPTER *prAdapter)
{
	DBGLOG(ML, TRACE, "\n");
}

void mldStarecDump(struct ADAPTER *prAdapter)
{
	uint8_t i = 0, j = 0;

	DBGLOG(ML, INFO, "========== START ==========\n");

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldStarec); i++) {
		struct MLD_STA_RECORD *prMldStarec = &prAdapter->aprMldStarec[i];
		struct STA_RECORD *prCurrStarec;
		struct LINK *prStarecList;

		if (!prMldStarec->fgIsInUse)
			continue;

		DBGLOG(ML, INFO,
			"[%d] pri:%d, sec:%d, setup:%d, eml:0x%04x, str:0x%02x%04x, mac:"
			MACSTR "\n",
			prMldStarec->ucIdx,
			prMldStarec->u2PrimaryMldId,
			prMldStarec->u2SecondMldId,
			prMldStarec->u2SetupWlanId,
			prMldStarec->u2EmlCap,
			*(uint8_t *)(prMldStarec->aucStrBitmap + 2),
			*(uint16_t *)(prMldStarec->aucStrBitmap),
			MAC2STR(prMldStarec->aucPeerMldAddr));

		DBGLOG(ML, INFO, "\tRX pkt count:\n");
		for (j = 0; j < ARRAY_SIZE(prMldStarec->aucRxPktCnt); j++)
			DBGLOG(ML, INFO, "\t\tband%d:0x%llx\n",
				j, prMldStarec->aucRxPktCnt[j]);

		DBGLOG(ML, INFO, "\tSta list:\n");
		prStarecList = &prMldStarec->rStarecList;
		LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList, rLinkEntryMld,
		    struct STA_RECORD)
			cnmDumpStaRec(prAdapter, prCurrStarec->ucIndex);

		DBGLOG(ML, INFO, "\n");
	}
	DBGLOG(ML, INFO, "========== END ==========\n");
}

struct MLD_STA_RECORD *mldStarecGetByMldAddr(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	const uint8_t aucMacAddr[])
{
	struct MLD_STA_RECORD *prMldSta;
	struct LINK *prClientList;
	const uint8_t offset =
			aucMacAddr[5] % ARRAY_SIZE(prAdapter->aprMldStarec);

	if (!prMldBssInfo)
		return NULL;

	/* Try hash index first, if miss, fallback to original traversal */
	prMldSta = &prAdapter->aprMldStarec[offset];
	if (prMldSta->fgIsInUse &&
	    prMldSta->ucGroupMldId == prMldBssInfo->ucGroupMldId &&
	    EQUAL_MAC_ADDR(prMldSta->aucPeerMldAddr, aucMacAddr))
		return prMldSta;

	prClientList = &prMldBssInfo->rMldStaRecOfClientList;
	LINK_FOR_EACH_ENTRY(prMldSta, prClientList, rLinkEntry,
			    struct MLD_STA_RECORD) {
		if (EQUAL_MAC_ADDR(prMldSta->aucPeerMldAddr, aucMacAddr))
			return prMldSta;
	}

	return NULL;
}

struct MLD_STA_RECORD *mldStarecGetByLinkAddr(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	uint8_t aucLinkAddr[])
{
	const uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	uint8_t i = 0;

	if (!prMldBssInfo)
		return NULL;

	if (EQUAL_MAC_ADDR(aucZeroMacAddr, aucLinkAddr))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldStarec); i++) {
		struct MLD_STA_RECORD *prMldStarec =
			&prAdapter->aprMldStarec[i];

		if (prMldStarec->fgIsInUse &&
		    prMldStarec->ucGroupMldId == prMldBssInfo->ucGroupMldId) {
			struct STA_RECORD *prCurrStarec;
			struct LINK *prStarecList;

			prStarecList = &prMldStarec->rStarecList;
			LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList,
				rLinkEntryMld, struct STA_RECORD) {
				if (EQUAL_MAC_ADDR(prCurrStarec->aucMacAddr,
						   aucLinkAddr))
					return prMldStarec;
			}
		}
	}

	return NULL;
}

uint8_t mldStarecNum(struct ADAPTER *prAdapter)
{
	uint8_t i, num = 0;
	struct MLD_STA_RECORD *prMldStarec;

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldStarec); i++) {
		prMldStarec = &prAdapter->aprMldStarec[i];

		if (prMldStarec->fgIsInUse)
			num++;
	}

	return num;
}

void mldBssTeardownAllClients(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo)
{
	struct MLD_STA_RECORD *prMldStarec = NULL;
	struct LINK *prStarecList;
	uint8_t i = 0;

	if (!prMldBssInfo)
		return;

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldStarec); i++) {
		prMldStarec = &prAdapter->aprMldStarec[i];
		prStarecList = &prMldStarec->rStarecList;

		if (prMldStarec->fgIsInUse &&
		    prMldStarec->ucGroupMldId == prMldBssInfo->ucGroupMldId &&
		    !LINK_IS_EMPTY(prStarecList))
			/* sync with FW */
			nicUniCmdMldStaTeardown(prAdapter,
				LINK_PEEK_HEAD(prStarecList,
				struct STA_RECORD, rLinkEntryMld));
	}
}

static void mldStarecUpdateMldId(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec)
{
	struct MLD_BSS_INFO *prMldBssInfo;
	struct LINK *prStarecList = &prMldStarec->rStarecList;
	struct STA_RECORD *prStarec;
	uint8_t i;

	prMldBssInfo = mldBssGetByIdx(prAdapter, prMldStarec->ucGroupMldId);

	if (!prMldBssInfo) {
		DBGLOG(ML, WARN, "null MldBssInfo! GroupMldId:%u\n",
			prMldStarec->ucGroupMldId);
		return;
	}

	/* get the primary and second link mldid */
	i = 0;
	prMldStarec->u2PrimaryMldId = 0;
	prMldStarec->u2SecondMldId = 0;
	LINK_FOR_EACH_ENTRY(prStarec, prStarecList,
			rLinkEntryMld, struct STA_RECORD) {
		if (i == 0) {
			prMldStarec->u2PrimaryMldId = prStarec->ucWlanIndex;
			/*Second MldId is same as Primary MldId in the
			 *case of single link.
			 */
			prMldStarec->u2SecondMldId = prStarec->ucWlanIndex;
		} else if (i == 1) {
			prMldStarec->u2SecondMldId = prStarec->ucWlanIndex;
			break;
		}
		i++;
	}

	kalMemZero(prMldStarec->aucStrBitmap, UNI_MLD_LINK_MAX);
	for (i = 0; i < UNI_MLD_LINK_MAX; i++) {
		if (prMldBssInfo->ucHwBandBitmap & BIT(i))
			prMldStarec->aucStrBitmap[i] =
				prMldBssInfo->ucHwBandBitmap;
	}
}

struct MLD_STA_RECORD *mldStarecJoin(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct STA_RECORD *prMainStarec,
	struct STA_RECORD *prStarec, struct BSS_DESC *prBssDesc)
{
	struct MLD_STA_RECORD *prMldStaRec = NULL;

	if (prMldBssInfo == NULL) {
		DBGLOG(ML, ERROR, "no prMldBssInfo\n");
		return NULL;
	}

	if (prMainStarec == prStarec)
		prMldStaRec = mldStarecAlloc(prAdapter, prMldBssInfo,
			prBssDesc->rMlInfo.aucMldAddr,
			prBssDesc->rMlInfo.fgMldType,
			prBssDesc->rMlInfo.u2EmlCap,
			prBssDesc->rMlInfo.u2MldCap);
	else
		prMldStaRec = mldStarecGetByStarec(prAdapter, prMainStarec);

	if (prMldStaRec == NULL) {
		DBGLOG(ML, ERROR, "MldBss%d can't alloc prMldStaRec\n",
			prMldBssInfo->ucGroupMldId);
		return NULL;
	}

	mldStarecRegister(prAdapter, prMldStaRec, prStarec,
		prBssDesc->rMlInfo.ucLinkIndex);

	return prMldStaRec;
}

int8_t mldStarecRegister(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec, struct STA_RECORD *prStarec,
	uint8_t ucLinkId)
{
	int8_t rStatus = 0;
	struct LINK *prStarecList = NULL;

	if (!prStarec->fgIsInUse) {
		DBGLOG(ML, WARN, "starec(idx=%d, widx=%d) not in use",
			prStarec->ucIndex, prStarec->ucWlanIndex);
		return -EINVAL;
	} else if (ucLinkId >= MAX_NUM_MLO_LINKS) {
		DBGLOG(ML, WARN, "wrong linkid=%d >= %d",
			ucLinkId, MAX_NUM_MLO_LINKS);
		return -EINVAL;
	}

	if (prMldStarec->u4StaBitmap & BIT(prStarec->ucIndex)) {
		DBGLOG(ML, WARN,
			"starec(%d) already in mld_starec(id=%d, "
			MACSTR ", stabitmap=%x)\n",
			prStarec->ucIndex, prMldStarec->ucIdx,
			MAC2STR(prMldStarec->aucPeerMldAddr),
			prMldStarec->u4StaBitmap);
		rStatus = -EINVAL;
		goto exit;
	}

	/* fill link info */
	prStarec->ucLinkIndex = ucLinkId;
	COPY_MAC_ADDR(prStarec->aucMldAddr, prMldStarec->aucPeerMldAddr);
	prStarec->ucMldStaIndex = prMldStarec->ucIdx;

	prStarecList = &prMldStarec->rStarecList;
	LINK_INSERT_TAIL(prStarecList, &prStarec->rLinkEntryMld);
	prMldStarec->u4StaBitmap |= BIT(prStarec->ucIndex);
	prMldStarec->u4ActiveStaBitmap |= BIT(prStarec->ucIndex);
	prMldStarec->u2ValidLinks |= BIT(ucLinkId);

	mldStarecUpdateMldId(prAdapter, prMldStarec);

	DBGLOG(ML, INFO,
		"MldStaRec: %d, StaRec: %d, link: %d, widx: %d, bss: %d, pri_mld: %d, sec_mld: %d, mld_mac: "
		MACSTR " mld_type: %d, str[0x%x,0x%x,0x%x]\n",
		prMldStarec->ucIdx,
		prStarec->ucIndex,
		prStarec->ucLinkIndex,
		prStarec->ucWlanIndex,
		prStarec->ucBssIndex,
		prMldStarec->u2PrimaryMldId,
		prMldStarec->u2SecondMldId,
		MAC2STR(prMldStarec->aucPeerMldAddr),
		prMldStarec->fgMldType,
		prMldStarec->aucStrBitmap[0],
		prMldStarec->aucStrBitmap[1],
		prMldStarec->aucStrBitmap[2]);

exit:
	return rStatus;
}

void mldStarecUnregister(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStarec)
{
	struct MLD_STA_RECORD *prMldStarec;
	struct LINK *prStarecList;
	struct STA_RECORD *prCurrStarec, *prNextStarec;

	if (!prStarec || prStarec->ucMldStaIndex == MLD_GROUP_NONE)
		return;

	prMldStarec = mldStarecGetByStarec(prAdapter, prStarec);
	if (!prMldStarec)
		return;

	prStarecList = &prMldStarec->rStarecList;

	DBGLOG(ML, INFO, "prMldStarec: %d, prStarec: %d\n",
		prMldStarec->ucIdx, prStarec->ucIndex);

	LINK_FOR_EACH_ENTRY_SAFE(prCurrStarec, prNextStarec, prStarecList,
			rLinkEntryMld, struct STA_RECORD) {
		if (prStarec != prCurrStarec)
			continue;

		prCurrStarec->ucMldStaIndex = MLD_GROUP_NONE;
		LINK_REMOVE_KNOWN_ENTRY(prStarecList,
			&prCurrStarec->rLinkEntryMld);
		break;
	}

	mldStarecUpdateMldId(prAdapter, prMldStarec);

	prMldStarec->u4StaBitmap &= ~BIT(prStarec->ucIndex);
	prMldStarec->u4ActiveStaBitmap &= ~BIT(prStarec->ucIndex);
	prMldStarec->u2ValidLinks &= ~BIT(prStarec->ucLinkIndex);

	if (LINK_IS_EMPTY(prStarecList))
		mldStarecFree(prAdapter, prMldStarec, prStarec);
}

struct MLD_STA_RECORD *mldStarecAlloc(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	uint8_t *aucMacAddr, uint8_t fgMldType,
	uint16_t u2EmlCap, uint16_t u2MldCap)
{
	struct MLD_STA_RECORD *prMldStarec = NULL;
	const uint8_t offset =
			aucMacAddr[5] % ARRAY_SIZE(prAdapter->aprMldStarec);
	uint8_t i = 0;
	uint8_t idx;

	for (i = 0; i < ARRAY_SIZE(prAdapter->aprMldStarec); i++) {
		idx = (i + offset) % ARRAY_SIZE(prAdapter->aprMldStarec);

		if (prAdapter->aprMldStarec[idx].fgIsInUse)
			continue;

		prMldStarec = &prAdapter->aprMldStarec[idx];
		kalMemZero(prMldStarec, sizeof(*prMldStarec));
		LINK_INITIALIZE(&prMldStarec->rStarecList);
		prMldStarec->fgIsInUse = TRUE;
		prMldStarec->ucIdx = idx;
		prMldStarec->fgMldType = fgMldType;
		prMldStarec->ucGroupMldId = prMldBssInfo->ucGroupMldId;

		prMldStarec->fgEPCS = FALSE;
		prMldStarec->u2EmlCap = u2EmlCap;
		prMldStarec->u2MldCap = u2MldCap;
		prMldStarec->ucEmlEnabled = prMldBssInfo->ucEmlEnabled;
		prMldStarec->ucMaxSimuLinks = prMldBssInfo->ucMaxSimuLinks;
		COPY_MAC_ADDR(prMldStarec->aucPeerMldAddr, aucMacAddr);

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
		prMldStarec->fgIsSbMlsr = prMldBssInfo->fgIsSbMlsr;
#endif /* CFG_SINGLE_BAND_MLSR_56 */

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
		cnmTimerInitTimer(prAdapter,
				&prMldStarec->rEpcsTimer,
				(PFN_MGMT_TIMEOUT_FUNC) epcsTimeout,
				(uintptr_t) prMldStarec);
#endif
#if (CFG_SUPPORT_802_11BE_T2LM == 1)
		prMldStarec->eT2LMState = T2LM_STATE_IDLE;
		cnmTimerInitTimer(prAdapter,
			&prMldStarec->rT2LMTimer,
			(PFN_MGMT_TIMEOUT_FUNC) t2lmTimeout,
			(uintptr_t) prMldStarec);
#endif
		mldBssAddClient(prAdapter, prMldBssInfo, prMldStarec);
		DBGLOG(ML, INFO, "ucIdx: %d, aucMacAddr: " MACSTR "\n",
				prMldStarec->ucIdx,
				MAC2STR(prMldStarec->aucPeerMldAddr));
		break;
	}

	if (i == ARRAY_SIZE(prAdapter->aprMldStarec))
		prMldStarec = NULL;

	return prMldStarec;
}

void mldStarecFree(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec, struct STA_RECORD *prStarec)
{
	struct MLD_BSS_INFO *prMldBssInfo;

	DBGLOG(ML, INFO, "MldStarec=%d, ucGroupMldId=%d\n",
		prMldStarec->ucIdx, prMldStarec->ucGroupMldId);

	prMldBssInfo = mldBssGetByIdx(prAdapter, prMldStarec->ucGroupMldId);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	nicUniCmdMldStaTeardown(prAdapter, prStarec);
#endif

#if (CFG_SUPPORT_802_11BE_T2LM == 1)
	cnmTimerStopTimer(prAdapter, &prMldStarec->rT2LMTimer);
#endif
	mldBssRemoveClient(prAdapter, prMldBssInfo, prMldStarec);
	kalMemZero(prMldStarec, sizeof(struct MLD_STA_RECORD));
}

struct MLD_STA_RECORD *mldStarecGetByStarec(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	struct MLD_STA_RECORD *prMldStarec;

	if (!prAdapter || !prStaRec)
		return NULL;

	if (prStaRec->ucMldStaIndex == MLD_GROUP_NONE ||
	    prStaRec->ucMldStaIndex >= ARRAY_SIZE(prAdapter->aprMldStarec))
		return NULL;

	prMldStarec = &prAdapter->aprMldStarec[prStaRec->ucMldStaIndex];
	if (!prMldStarec->fgIsInUse)
		return NULL;

	return prMldStarec;
}

uint8_t mldGetWlanIdxByBand(struct ADAPTER *prAdapter, uint8_t ucHwBandIdx,
			    uint8_t ucWlanIdx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t ucBssIndex;
	uint8_t ucStaIndex;

	ucBssIndex = secGetBssIdxByWlanIdx(prAdapter, ucWlanIdx);
	if (ucBssIndex != WTBL_RESERVED_ENTRY)
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo)
		return ucWlanIdx;

	if (prBssInfo && prBssInfo->eHwBandIdx == ucHwBandIdx)
		return ucWlanIdx; /* hit */

	/* check alternative */
	ucStaIndex = secGetStaIdxByWlanIdx(prAdapter, ucWlanIdx);
	/* primary */
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaIndex);

	/* link associated with the ucHwBandIdx */
	prStaRec = mldGetStaRecByBandIdx(prAdapter, prStaRec, ucHwBandIdx);
	if (prStaRec)
		return prStaRec->ucWlanIndex;
#endif

	return ucWlanIdx;
}

uint8_t mldGetPrimaryWlanIdx(struct ADAPTER *prAdapter, uint8_t ucWlanIdx)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *prMldSta = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t ucStaIndex;

	ucStaIndex = secGetStaIdxByWlanIdx(prAdapter, ucWlanIdx);
	if (ucStaIndex == STA_REC_INDEX_NOT_FOUND)
		return ucWlanIdx;

	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, ucStaIndex);
	if (!prStaRec)
		return ucWlanIdx;

	prMldSta = mldStarecGetByStarec(prAdapter, prStaRec);
	if (prMldSta)
		return prMldSta->u2PrimaryMldId;
#endif

	return ucWlanIdx;
}


int8_t mldStarecSetSetupIdx(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	struct MLD_STA_RECORD *prMldStarec = mldStarecGetByStarec(prAdapter,
		prStaRec);

	if (!prAdapter || !prMldStarec)
		return -EINVAL;

	DBGLOG(ML, INFO, "prMldStarec: %d, ucIdx: %d\n",
			prMldStarec->ucIdx, prStaRec->ucWlanIndex);

	prMldStarec->u2SetupWlanId = prStaRec->ucWlanIndex;

	return 0;
}

uint32_t mldUpdateTidBitmap(struct ADAPTER *prAdapter,
	 struct MLD_STA_RECORD *prMldStaRec)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_T2LM *tag;
	struct UNI_CMD_STAREC_LINK_INFO *link;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
			       sizeof(struct UNI_CMD_STAREC_T2LM);
	struct LINK *prStarecList = &prMldStaRec->rStarecList;
	struct STA_RECORD *prStaRec;
	uint16_t widx = 0;

	prStaRec = LINK_PEEK_HEAD(prStarecList,
			struct STA_RECORD, rLinkEntryMld);
	if (!prStaRec) {
		DBGLOG(ML, ERROR,
		       "prStaRec is Null ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	max_cmd_len += sizeof(*link) * prStarecList->u4NumElem;
	uni_cmd = (struct UNI_CMD_STAREC *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(ML, ERROR,
		       "Allocate UNI_CMD_STAREC ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd->ucBssInfoIdx = prStaRec->ucBssIndex;
	widx = (uint16_t) prStaRec->ucWlanIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);

	tag = (struct UNI_CMD_STAREC_T2LM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_T2LM;
	tag->u2Length = sizeof(*tag) + sizeof(*link) * prStarecList->u4NumElem;
	tag->ucLinkNumber = prStarecList->u4NumElem;

	DBGLOG(ML, INFO, "[%d] bssidx=%d,widx=%d,num=%d,mac=" MACSTR "\n",
		prStaRec->ucIndex,
		prStaRec->ucBssIndex,
		prStaRec->ucWlanIndex,
		tag->ucLinkNumber,
		MAC2STR(prMldStaRec->aucPeerMldAddr));

	link = (struct UNI_CMD_STAREC_LINK_INFO *)tag->aucLinkInfo;
	LINK_FOR_EACH_ENTRY(prStaRec, prStarecList, rLinkEntryMld,
			struct STA_RECORD) {
		link->ucBssIdx = prStaRec->ucBssIndex;
		link->u2WlanIdx = prStaRec->ucWlanIndex;
		link->ucTidBitmap = prStaRec->ucULTidBitmap;
		DBGLOG(ML, INFO, "\tbss=%d,wlan_idx=%d,tid=0x%x\n",
			link->ucBssIdx, link->u2WlanIdx, link->ucTidBitmap);
		link++;
	}

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_STAREC_INFO,
			     TRUE,
			     FALSE,
			     FALSE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif

}

#if (CFG_MLD_INFO_PRESETUP == 1)
static uint32_t mldUpdatePerLinkMlo(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec)
{
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(prStaRec);
	if (prStaRec->ucBssIndex > prAdapter->ucSwBssIdNum) {
		DBGLOG(ML, INFO, "BSS index is invalid\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!IS_NET_ACTIVE(prAdapter, prStaRec->ucBssIndex)) {
		DBGLOG(ML, INFO, "Network is not activated\n");
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(ML, INFO, "prBssInfo is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!IS_BSS_ACTIVE(prBssInfo)) {
		DBGLOG(ML, INFO, "prBssInfo is not activated\n");
		return WLAN_STATUS_FAILURE;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	if (nicUniCmdSetBssMld(prAdapter, prBssInfo) !=
			WLAN_STATUS_SUCCESS) {
		DBGLOG(ML, ERROR, "BSS MLD update failed.\n");
		return WLAN_STATUS_FAILURE;
	}
	if (nicUniCmdSetStarecMld(prAdapter, prStaRec) !=
			WLAN_STATUS_SUCCESS) {
		DBGLOG(ML, ERROR, "STA_REC ML INFO update failed.\n");
		return WLAN_STATUS_FAILURE;
	}
	return WLAN_STATUS_SUCCESS;
#else
	DBGLOG(ML, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to setup MLD info and form MLO.
 *        This only used in STA mode currently.
 *
 * @param prAdapter          Pointer of ADAPTER_T
 *        prStaRec           Pointer of STA_RECORD
 *
 * @retval 0
 * @retval -EINVAL
 */
/*----------------------------------------------------------------------------*/
int8_t mldSetupMlInfo(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec)
{
	int8_t status = 0;
	struct MLD_STA_RECORD *prMldStarec;
	struct STA_RECORD *prCurrStarec;
	struct LINK *prStarecList;

	prMldStarec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStarec)
		return -EINVAL;

	prStarecList = &prMldStarec->rStarecList;
	if (!prStarecList)
		return -EINVAL;

	LINK_FOR_EACH_ENTRY(prCurrStarec, prStarecList,
			rLinkEntryMld, struct STA_RECORD) {
		if (!prCurrStarec)
			break;
		if (mldUpdatePerLinkMlo(prAdapter, prCurrStarec) !=
				WLAN_STATUS_SUCCESS) {
			status = -EINVAL;
			break;
		}
	}

	return status;
}
#endif /* CFG_MLD_INFO_PRESETUP */

void mldStarecLogRxData(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucHwBandIdx)
{
	struct MLD_STA_RECORD *mld_sta = mldStarecGetByStarec(prAdapter,
		prStaRec);

	if (!mld_sta)
		return;

	if (ucHwBandIdx >= ENUM_BAND_NUM)
		return;

	mld_sta->aucRxPktCnt[ucHwBandIdx]++;
}

int8_t mldStarecInit(struct ADAPTER *prAdapter)
{
	DBGLOG(ML, INFO, "Total %lu MldStaRec\n",
		ARRAY_SIZE(prAdapter->aprMldStarec));
	kalMemZero(prAdapter->aprMldStarec, sizeof(prAdapter->aprMldStarec));
	return 0;
}

void mldStarecUninit(struct ADAPTER *prAdapter)
{
	DBGLOG(ML, TRACE, "\n");
}

struct BSS_INFO *mldGetBssInfoByLinkID(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, uint8_t ucLinkIndex,
	uint8_t fgPeerSta)
{
	struct BSS_INFO *prCurrBssInfo = NULL;
	struct LINK *prBssList = NULL;
	struct STA_RECORD *prStaRecOfAP = NULL;

	if (!prMldBssInfo)
		return NULL;

	prBssList = &prMldBssInfo->rBssList;

	LINK_FOR_EACH_ENTRY(prCurrBssInfo, prBssList,
			rLinkEntryMld,
			struct BSS_INFO) {

		if (fgPeerSta == TRUE) {
			/* Match with peer STA(AP)'s link ID */
			prStaRecOfAP = prCurrBssInfo->prStaRecOfAP;

			if ((prStaRecOfAP) &&
				(prStaRecOfAP->ucLinkIndex == ucLinkIndex))
				 return prCurrBssInfo;
		} else {
			/* Match with local STA(SAP)'s link ID */
			if (prCurrBssInfo->ucLinkIndex == ucLinkIndex)
				  return prCurrBssInfo;
		}
	}

	return NULL;
}

uint8_t mldIsMultiLinkFormed(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec)
{
	struct MLD_STA_RECORD *mld_starec;

	mld_starec = mldStarecGetByStarec(prAdapter, prStaRec);
	return (mld_starec != NULL &&
		mld_starec->rStarecList.u4NumElem >= 1);
}

uint8_t mldIsMultiLinkEnabled(
	struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType,
	uint8_t ucParam)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t ret = TRUE;
	uint8_t linkMax = 0;
	uint8_t fgIsApMode = FALSE;

	if (eNetworkType == NETWORK_TYPE_AIS) {
		linkMax = kal_min_t(uint8_t,
				prWifiVar->ucMldLinkMax,
				prWifiVar->ucStaMldLinkMax);
	} else if (eNetworkType == NETWORK_TYPE_P2P &&
		   p2pGetMode() == RUNNING_P2P_DEV_MODE) {
		fgIsApMode = ucParam;
		if (fgIsApMode) {
			linkMax = kal_min_t(uint8_t,
					    prWifiVar->ucMldLinkMax,
					    prWifiVar->ucApMldLinkMax);
		} else {
			linkMax = kal_min_t(uint8_t,
					    prWifiVar->ucMldLinkMax,
					    prWifiVar->ucP2pMldLinkMax);
		}
	}


	/* mlo is disable when one of these is true
	 * 1. eht disabled
	 * 2. max link num < 2
	 * 3. EnableMlo 0 (disabled)
	 */
	if (!mldIsSingleLinkEnabled(prAdapter, eNetworkType, ucParam) ||
	    linkMax < 2)
		ret = FALSE;

	return ret;
}

uint8_t mldIsSingleLinkEnabled(
	struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType,
	uint8_t ucParam)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint8_t ret = TRUE;
	uint8_t ucEhtOption = FEATURE_ENABLED;
	uint8_t fgIsApMode = FALSE;

	if (eNetworkType == NETWORK_TYPE_AIS) {
		uint8_t ucBssIndex = ucParam;

		if (AIS_INDEX(prAdapter, ucBssIndex) <
		    prWifiVar->u4AisEHTNumber)
			ucEhtOption = prWifiVar->ucStaEht;
		else
			ucEhtOption = FEATURE_DISABLED;
	} else if (eNetworkType == NETWORK_TYPE_P2P) {
		fgIsApMode = ucParam;
		if (fgIsApMode) {
			ucEhtOption = prWifiVar->ucApEht;
		} else {
			if (IS_FEATURE_DISABLED(prWifiVar->ucP2pGoEht) &&
			    IS_FEATURE_DISABLED(prWifiVar->ucP2pGcEht))
				ucEhtOption = FEATURE_DISABLED;
			else
				ucEhtOption = FEATURE_ENABLED;
		}
	}

	/* mlo is disable when one of these is true
	 * 1. eht disabled
	 * 2. EnableMlo 0 (disabled)
	 */
	if (IS_FEATURE_DISABLED(ucEhtOption) ||
	    IS_FEATURE_DISABLED(prWifiVar->ucEnableMlo)) {
		ret = FALSE;

		DBGLOG(ML, TRACE,
			"ucMldLinkMax:%d,(sta=%d,ap=%d,p2p=%d) ucEnableMlo:%d, EhtOption:%d, eNetworkType:%d, p2pMode:%d Param:%d => mlo feature disabled\n",
			prWifiVar->ucMldLinkMax,
			prWifiVar->ucStaMldLinkMax,
			prWifiVar->ucApMldLinkMax,
			prWifiVar->ucP2pMldLinkMax,
			prWifiVar->ucEnableMlo,
			ucEhtOption,
			eNetworkType,
			p2pGetMode(),
			ucParam);
	}

	return ret;
}

uint8_t mldSingleLink(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint8_t ucBssIndex)
{
	struct BSS_INFO *bss;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecBssInfo;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	u_int8_t fgIsApMode = FALSE;
	uint8_t enable;

	bss = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!bss) {
		DBGLOG(ML, WARN, "Bss is NULL!\n");
		return FALSE;
	}

	mld_bssinfo = mldBssGetByBss(prAdapter, bss);
	if (!mld_bssinfo) {
		DBGLOG(ML, LOUD, "Mld Bss is NULL!\n");
		return FALSE;
	}

	enable = IS_FEATURE_ENABLED(prWifiVar->ucEnableMlo);
	fgIsApMode = p2pFuncIsAPMode(prWifiVar->prP2PConnSettings[
		bss->u4PrivateData]);
	prP2pSpecBssInfo = prWifiVar->prP2pSpecificBssInfo[
		bss->u4PrivateData];

	if (IS_BSS_APGO(bss)) {
#ifdef MLD_SECURITY_RESTRICTIONS
		/* ap-mld must support rsne & pmf */
		enable &= secIsProtectedBss(prAdapter, bss);
		enable &= !!(bss->u2RsnSelectedCapInfo & ELEM_WPA_CAP_MFPC);
#endif
		enable &= !!(bss->ucPhyTypeSet & PHY_TYPE_BIT_EHT);
		if (!CFG_SAP_SKIP_CHECK_HOSTAPD_ML_IE &&
		    prP2pSpecBssInfo && fgIsApMode)
			enable &= prP2pSpecBssInfo->fgMlIeExist;
	} else if (prStaRec) {
		enable &= !!(prStaRec->ucDesiredPhyTypeSet & PHY_TYPE_BIT_EHT);

		if (IS_BSS_AIS(bss)) {
			struct BSS_DESC *prBssDesc = NULL;
			struct AIS_FSM_INFO *ais =
				aisGetAisFsmInfo(prAdapter, ucBssIndex);

			if (!ais) {
				DBGLOG(ML, WARN, "NULL prAisFsmInfo\n");
				return FALSE;
			}

			prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
			if (!prBssDesc) {
				DBGLOG(ML, WARN, "NULL prTargetBssDesc\n");
				return FALSE;
			}
			enable &= prBssDesc->rMlInfo.fgValid;
		}
#if CFG_ENABLE_WIFI_DIRECT
		else if (IS_BSS_P2P(bss)) {
			struct BSS_DESC *prBssDesc = p2pGetTargetBssDesc(
				prAdapter, ucBssIndex);

			if (!prBssDesc) {
				DBGLOG(ML, WARN, "NULL prTargetBssDesc\n");
				return FALSE;
			}
			enable &= prBssDesc->rMlInfo.fgValid;
		}
#endif
	} else {
		enable = FALSE;
	}

	DBGLOG(ML, LOUD, "%s MLO for single link\n",
		enable ? "Enable" : "Disable");
	return enable;
}

uint8_t mldCheckMldType(struct ADAPTER *prAdapter,
	uint8_t *pucIe, uint16_t u2Len)
{
	uint16_t offset = 0;
	uint8_t mld = FALSE;

	IE_FOR_EACH(pucIe, u2Len, offset) {
		if (BE_IS_ML_CTRL_TYPE(pucIe, ML_CTRL_TYPE_BASIC))
			mld = TRUE;

		if (rlmCheckMtkOuiChipCap(pucIe, CHIP_CAP_ICV_V1))
			return MLD_TYPE_ICV_METHOD_V1;

		if (rlmCheckMtkOuiChipCap(pucIe, CHIP_CAP_ICV_V1_1))
			return MLD_TYPE_ICV_METHOD_V1_1;

		if (rlmCheckMtkOuiChipCap(pucIe, CHIP_CAP_ICV_V2))
			return MLD_TYPE_ICV_METHOD_V2;

	}

	if (mld)
		return MLD_TYPE_EXTERNAL;

	return MLD_TYPE_INVALID;
}

/**
 * mldGetStaRecByBandIdx() - find a STA_RECORD in same MLD by HW band index
 *
 * @prAdapter: adapter pointer to look up required information
 * @prStaRec: a STA_RECORD points to the same MLD to be queried
 * @ucHwBandIdx: band index to match a STA_RECORD
 */
struct STA_RECORD *mldGetStaRecByBandIdx(struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec, uint8_t ucHwBandIdx)
{
	struct MLD_STA_RECORD *prMldStarec;
	struct STA_RECORD *sta_rec = NULL;
	struct BSS_INFO *prBssInfo;

	if (!prStaRec)
		return prStaRec;

	prMldStarec = mldStarecGetByStarec(prAdapter, prStaRec);
	if (!prMldStarec)
		return prStaRec;

	LINK_FOR_EACH_ENTRY(sta_rec, &prMldStarec->rStarecList,
					rLinkEntryMld, struct STA_RECORD) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  sta_rec->ucBssIndex);
		if (prBssInfo && prBssInfo->eHwBandIdx == ucHwBandIdx)
			return sta_rec;
	}

	/* not matched */
	return prStaRec;
}

void mldCheckApRemoval(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, const uint8_t *pucIE)
{
#if (CFG_SUPPORT_ML_RECONFIG == 1)
	struct MULTI_LINK_INFO rMlInfo;
	struct MULTI_LINK_INFO *prMlInfo = &rMlInfo;
	uint8_t ucBssIndex, i;

	ucBssIndex = prStaRec->ucBssIndex;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex) ||
	    !mldSingleLink(prAdapter, prStaRec, prStaRec->ucBssIndex))
		return;

	MLD_PARSE_RECONFIG_MLIE(prMlInfo, pucIE, prStaRec->aucMacAddr);

	for (i = 0; i < prMlInfo->ucProfNum; i++) {
		struct STA_PROFILE *sta = &prMlInfo->rStaProfiles[i];

		if (prStaRec->ucLinkIndex == sta->ucLinkId)
			aisCheckApRemoval(prAdapter, prStaRec,
					  sta->u2ApRemovalTimer);
	}
#endif /* CFG_SUPPORT_ML_RECONFIG */
}

/* Req CH Type:
 * single link/STR MLO: CH_REQ_TYPE_JOIN
 * EMLSR:CH_REQ_TYPE_MLO_MLSR_AG_JOIN
 * or CH_REQ_TYPE_MLO_MLSR_AA_JOIN
 */
enum ENUM_CH_REQ_TYPE mldDecideCnmReqCHType(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *mld_bssinfo)
{
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	struct BSS_DESC *prBssDesc = NULL;
	uint8_t ucBssIndex = 0xff;
	uint8_t ucHas2GBand = FALSE;
#endif /* CFG_MLO_CONCURRENT_SINGLE_PHY */

	if (!mld_bssinfo) {
		DBGLOG(ML, INFO, "mld_bssinfo is NULL\n");
		return CH_REQ_TYPE_JOIN;
	}

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
	if (mld_bssinfo->fgIsSbMlsr)
		return CH_REQ_TYPE_MLO_MLSR_SINGLE_BAND_JOIN;

	if (mld_bssinfo->ucEmlEnabled)
		return CH_REQ_TYPE_MLO_EMLSR_JOIN;
#endif /* CFG_SINGLE_BAND_MLSR_56 */

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
#if (CFG_SUPPORT_MLO_HYBRID == 1)
	if (mld_bssinfo->rBssList.u4NumElem == MLD_HYBRID_MLO_LINK_NUM)
		return CH_REQ_TYPE_HYBRID_MLO_MLSR_JOIN;
#endif /* CFG_SUPPORT_MLO_HYBRID */

	/*The CH Req type need set CH_REQ_TYPE_JOIN
	 *in the case of single link/STR/MLSR
	 */
	if (!IS_MLD_BSSINFO_MULTI(mld_bssinfo) ||
	    (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
	    (mld_bssinfo->ucMaxSimuLinks >= 1 ||
	    (mld_bssinfo->ucMaxSimuLinks == 0 &&
		 mld_bssinfo->ucEmlEnabled == FALSE &&
		 mld_bssinfo->ucHmloEnabled == FALSE)
		 )))
		return CH_REQ_TYPE_JOIN;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {
		if (mld_bssinfo->ucBssBitmap & BIT(ucBssIndex)) {
			prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
			if (prBssDesc && prBssDesc->eBand == BAND_2G4)
				ucHas2GBand = TRUE;
		}
	}
	DBGLOG(ML, INFO, "MLSR case, ucHas2GBand = %d\n", ucHas2GBand);

	if (ucHas2GBand)
		return CH_REQ_TYPE_MLO_MLSR_AG_JOIN;
	else
		return CH_REQ_TYPE_MLO_MLSR_AA_JOIN;
#endif /* CFG_MLO_CONCURRENT_SINGLE_PHY */

	return CH_REQ_TYPE_JOIN;
}

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
uint8_t mldNeedSingleBandMlsr56(struct ADAPTER *prAdapter,
	enum ENUM_MLO_LINK_PLAN eLinkPlan)
{
	/* cert & no str & no emlsr */
	return prAdapter->rWifiVar.u4SwTestMode == ENUM_SW_TEST_MODE_SIGMA_BE &&
	    IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucNonApMldEMLSupport) &&
	    prAdapter->rWifiVar.ucMaxSimuLinks == 0
#if (CFG_SUPPORT_WIFI_6G == 1)
	&& eLinkPlan == MLO_LINK_PLAN_5_6;
#endif
	;
}
#endif /* CFG_SINGLE_BAND_MLSR_56 */

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
enum ENUM_MLO_MODE mldCheckMLSRType(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0xff;
	struct BSS_INFO *prBssInfo = NULL;
	struct MLD_BSS_INFO *mld_bssinfo = NULL;
	enum ENUM_MLO_MODE eMloType = MLO_MODE_NUM;

	if (prAdapter == NULL)
		return MLO_MODE_NUM;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;

		mld_bssinfo = mldBssGetByBss(prAdapter, prBssInfo);
		if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
			mld_bssinfo->ucMaxSimuLinks == 0) {
			if (mld_bssinfo->ucHmloEnabled)
				eMloType = MLO_MODE_HYMLO;
			else if (mld_bssinfo->ucEmlEnabled)
				eMloType = MLO_MODE_EMLSR;
			else
				eMloType = MLO_MODE_MLSR;
		}
	}

	DBGLOG(ML, INFO, "MLSR Type %d\n", eMloType);
	return eMloType;
}

/*none MLO Bss or Single link MLO Bss*/
uint8_t mldHasSingleLinkBss(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0xff;
	struct BSS_INFO *prBssInfo = NULL;
	struct MLD_BSS_INFO *mld_bssinfo = NULL;

	if (prAdapter == NULL)
		return FALSE;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;

		mld_bssinfo = mldBssGetByBss(prAdapter, prBssInfo);
		if (!IS_MLD_BSSINFO_MULTI(mld_bssinfo)) {
			DBGLOG(ML, INFO, "has legacy bss\n");
			return TRUE;
		}
	}
	DBGLOG(ML, INFO, "has none legacy bss\n");
	return FALSE;
}

/* Check the new connection type(As follow) */
enum ENUM_MLO_MODE mldNewConnectionType(struct ADAPTER *prAdapter,
	struct DBDC_DECISION_INFO *prDbdcDecisionInfo)
{
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucBssIndex = 0xff;
	struct MLD_BSS_INFO *mld_bssinfo = NULL;

	if (!prDbdcDecisionInfo)
		return MLO_MODE_NUM;

	if (prDbdcDecisionInfo->ucLinkNum <= 1)
		return MLO_MODE_SLSR;

	ucBssIndex = prDbdcDecisionInfo->dbdcElem[0].ucBssIndex;
	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	mld_bssinfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
		mld_bssinfo->ucMaxSimuLinks == 0 &&
		mld_bssinfo->ucHmloEnabled == TRUE)
		return MLO_MODE_HYMLO;
	else if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
		mld_bssinfo->ucMaxSimuLinks == 0 &&
		mld_bssinfo->ucEmlEnabled == TRUE)
		return MLO_MODE_EMLSR;
	else if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
		mld_bssinfo->ucMaxSimuLinks == 0)
		return MLO_MODE_MLSR;
	else if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
			mld_bssinfo->ucMaxSimuLinks >= 1)
		return MLO_MODE_STR;

	return MLO_MODE_NUM;
}

void mldClearMLSRPausedLinkFlag(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIndex = 0xff;
	struct BSS_INFO *prBssInfo = NULL;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (prBssInfo->ucMLSRPausedLink)
			prBssInfo->ucMLSRPausedLink = FALSE;
	}
}

/* Decision which link need remain when MLSR & legacy Bss Concurrent
 * return the Remain MLSR BssIndex
 */
void mldMLSRDecisionLinkRemain(struct ADAPTER *prAdapter,
	struct DBDC_DECISION_INFO *prDbdcDecisionInfo)
{
	uint32_t rStatus;
	uint8_t ucMLSRBandCount[BAND_NUM] = {0};
	uint8_t ucMLSRBssIndex[BAND_NUM] = {-1};
	uint8_t ucLegacyBssBand = BAND_NULL;
	uint8_t ucMLSRRemainBssIndex = 0xff;
	uint8_t ucMLSRPauseBssIndex = 0xff;
	struct BSS_DESC *prBssDesc = NULL;
	struct BSS_INFO *prPauseBssInfo = NULL;
	struct MLD_BSS_INFO *mld_bssinfo = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucBssIndex = 0xff;
	uint32_t u4Tick;

	prAdapter->ucNeedWaitFWMlsrSWDone = FALSE;

	ucLegacyBssBand = prDbdcDecisionInfo->dbdcElem[0].eRfBand;

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {

		prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
		if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo))
			continue;
		mld_bssinfo = mldBssGetByBss(prAdapter, prBssInfo);
		if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
			mld_bssinfo->ucMaxSimuLinks == 0)
			break;
	}

	if (!mld_bssinfo) {
		DBGLOG(ML, ERROR, "mld_bssinfo not found\n");
		return;
	}

	mldClearMLSRPausedLinkFlag(prAdapter);

	for (ucBssIndex = 0;
		ucBssIndex < prAdapter->ucSwBssIdNum; ucBssIndex++) {
		if (mld_bssinfo->ucBssBitmap & BIT(ucBssIndex)) {
			prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
			if (prBssDesc &&
				prBssDesc->eBand > BAND_NULL &&
				prBssDesc->eBand < BAND_NUM) {
				ucMLSRBandCount[prBssDesc->eBand]++;
				/*map band to BssIndex*/
				ucMLSRBssIndex[prBssDesc->eBand] = ucBssIndex;
			}
		}
	}

#if (CFG_SUPPORT_MLO_HYBRID == 1)
	if (mld_bssinfo->ucHmloEnabled &&
		mld_bssinfo->rBssList.u4NumElem ==
			MLD_HYBRID_MLO_LINK_NUM) {
		/* if 5G band Rssi > TH,select 5G link,
		 * otherwise select 2G Link.
		 */
		prBssDesc = aisGetTargetBssDesc(prAdapter,
					ucMLSRBssIndex[BAND_5G]);
		if (prBssDesc &&
			(RCPI_TO_dBm(prBssDesc->ucRCPI) >
			MLSR_REMAIN_RSSI_TH)) {
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_5G];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_2G4];
			DBGLOG(ML, INFO, "Remain 5G,Pause 2G&6G\n");
		} else {
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_2G4];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_5G];
			DBGLOG(ML, INFO, "Remain 2G,Pause 5G&6G\n");
		}
		/*set 6G link pause flag*/
		prPauseBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						ucMLSRBssIndex[BAND_6G]);
		if (prPauseBssInfo)
			prPauseBssInfo->ucMLSRPausedLink = TRUE;
	} else if (ucMLSRBandCount[BAND_2G4] > 0 &&
		ucMLSRBandCount[BAND_5G] > 0)
#else
	if (ucMLSRBandCount[BAND_2G4] > 0 &&
		ucMLSRBandCount[BAND_5G] > 0)
#endif
	{
		if (ucLegacyBssBand == BAND_6G) {
		/*EMLSR remain 2G Link*/
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_2G4];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_5G];
		} else {
		/*if A band Rssi > TH, select A band,otherwise select G band*/
			prBssDesc = aisGetTargetBssDesc(prAdapter,
						ucMLSRBssIndex[BAND_5G]);
			if (prBssDesc &&
				(RCPI_TO_dBm(prBssDesc->ucRCPI) >
				MLSR_REMAIN_RSSI_TH)) {
				ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_5G];
				ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_2G4];
				DBGLOG(ML, INFO, "Remain 5G,Pause 2G\n");
			} else {
				ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_2G4];
				ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_5G];
				DBGLOG(ML, INFO, "Remain 2G,Pause 5G\n");
			}
		}
	} else if (ucMLSRBandCount[BAND_2G4] > 0 &&
		ucMLSRBandCount[BAND_6G] > 0) {
	/*if A band Rssi>-50, select A band,otherwise select G band*/
		prBssDesc = aisGetTargetBssDesc(prAdapter,
						ucMLSRBssIndex[BAND_6G]);
		if (prBssDesc &&
			(RCPI_TO_dBm(prBssDesc->ucRCPI) >
			MLSR_REMAIN_RSSI_TH)) {
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_6G];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_2G4];
			DBGLOG(ML, INFO, "Remain 6G,Pause 2G\n");
		} else {
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_2G4];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_6G];
			DBGLOG(ML, INFO, "Remain 2G,Pause 6G\n");
		}

	} else if (ucMLSRBandCount[BAND_5G] > 0 &&
		ucMLSRBandCount[BAND_6G] > 0) {
		if (ucLegacyBssBand == BAND_2G4) {
		/*EMLSR Remain 6G Link*/
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_6G];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_5G];
			DBGLOG(ML, INFO, "Remain 6G,Pause 5G\n");
		} else {
		/*EMLSR Remain 5G Link*/
			ucMLSRRemainBssIndex = ucMLSRBssIndex[BAND_5G];
			ucMLSRPauseBssIndex = ucMLSRBssIndex[BAND_6G];
			DBGLOG(ML, INFO, "Remain 5G,Pause 6G\n");
		}
	}

	DBGLOG(ML, INFO, "Remain BssIndex: %d, Pause BssIndex: %d\n",
				ucMLSRRemainBssIndex, ucMLSRPauseBssIndex);
	prPauseBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucMLSRPauseBssIndex);
	if (prPauseBssInfo)
		prPauseBssInfo->ucMLSRPausedLink = TRUE;

	if (!IS_BSS_INDEX_VALID(ucMLSRRemainBssIndex)) {
		DBGLOG(ML, INFO, "Remain BssIndex invalid\n");
		return;
	}

	rStatus = mldSetRemainMLSRBssIndex(prAdapter, ucMLSRRemainBssIndex);
	if (rStatus == WLAN_STATUS_SUCCESS ||
		rStatus == WLAN_STATUS_PENDING)
		/*send cmd ok*/
		prAdapter->ucNeedWaitFWMlsrSWDone = TRUE;
	else
		DBGLOG(ML, WARN, "send cmd error:%x\n", rStatus);

	u4Tick = kalGetTimeTick();
	/* Need wait MLSR ready, but it will block main_thread at this,
	 * so we need to process RX RFBs, otherwirs the event will not
	 * process at this time. The wait timeout is 1S.
	 */
	while (prAdapter->ucNeedWaitFWMlsrSWDone) {
		if ((kalGetTimeTick() - u4Tick) > 1000) {
			DBGLOG(HAL, ERROR,
				"Wait MLSR Ready timeout\n");
			break;
		}
		nicRxProcessRFBs(prAdapter);
		kalUsleep_range(1000, 2000);
	}
	DBGLOG(ML, STATE, "Wait MLSR Ready\n");

}

uint32_t mldSetRemainMLSRBssIndex(struct ADAPTER *prAdapter,
	uint8_t ucRemainBssIndex)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_MLO *uni_cmd;
	struct UNI_CMD_MLD_MLSR_CONCURENT_PRECONNECT *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MLO) +
			sizeof(struct UNI_CMD_MLD_MLSR_CONCURENT_PRECONNECT);


	uni_cmd = (struct UNI_CMD_MLO *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_MLO ==> FAILED\n");
		return WLAN_STATUS_FAILURE;
	}

	tag =
	(struct UNI_CMD_MLD_MLSR_CONCURENT_PRECONNECT *)uni_cmd->au1TlvBuffer;
	tag->u2Tag = UNI_CMD_MLO_TAG_MLD_MLSR_CONCURENT_PRECONNECT;
	tag->u2Length = sizeof(*tag);
	tag->ucMlsrRemainBssIndex = ucRemainBssIndex;

	status = wlanSendSetQueryUniCmdAdv(prAdapter,
			     UNI_CMD_ID_MLO,
			     TRUE,
			     FALSE,
			     FALSE,
			     NULL, /*nicUniEventMLSRSwitchDone*/
			     NULL,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0,
			     CMD_SEND_METHOD_REQ_RESOURCE);

	cnmMemFree(prAdapter, uni_cmd);

	return status;
}
#endif
#endif /* CFG_SUPPORT_802_11BE_MLO == 1 */
