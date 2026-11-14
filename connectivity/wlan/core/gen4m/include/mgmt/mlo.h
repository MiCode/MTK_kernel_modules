/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "mlo.h"
 *  \brief
 */

#ifndef _MLO_H
#define _MLO_H

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
#define MLSR_REMAIN_RSSI_TH                -50 /*dbm*/
#endif

#define IS_MLD_BSSINFO_MULTI(__prMldBssInfo) \
	(__prMldBssInfo && __prMldBssInfo->rBssList.u4NumElem > 1)

#define IS_MLD_STAREC_MULTI(__prMldStaRec) \
	(__prMldStaRec && __prMldStaRec->rStarecList.u4NumElem > 1)

#define IS_NON_AP_EML_ENABLED(__ad) \
	(IS_FEATURE_ENABLED(__ad->rWifiVar.ucNonApMldEMLSupport) && \
	 BE_IS_EML_CAP_SUPPORT_EMLSR(__ad->rWifiVar.u2NonApMldEMLCap))

#define IS_MLC_ENABLED(__ad) \
	(IS_FEATURE_ENABLED(__ad->rWifiVar.fgMlcSupport) && \
	 (BIT(0) & (__ad->rWifiVar.ucMlcSupportCap)) || \
	 IS_FEATURE_FORCE_ENABLED(__ad->rWifiVar.fgMlcSupport))

#define IS_MLC_CAPABLE(__ad) \
	(BIT(0) & (__ad->rWifiVar.ucMlcSupportCap))

#define BE_IS_ML_CTRL_TYPE(__pucIE, __TYPE) \
	(IE_ID(__pucIE) == ELEM_ID_RESERVED && IE_LEN(__pucIE) >= 3 && \
	 IE_ID_EXT(__pucIE) == ELEM_EXT_ID_MLD && \
	 (__pucIE[3] & ML_CTRL_TYPE_MASK) == __TYPE)

#define BE_SET_ML_CTRL_TYPE(_u2ctrl, _ctrl_type) \
{\
	(_u2ctrl) &= ~(ML_CTRL_TYPE_MASK); \
	(_u2ctrl) |= (((_ctrl_type) << (ML_CTRL_TYPE_SHIFT)) \
	& (ML_CTRL_TYPE_MASK)); \
}

#define BE_GET_ML_CTRL_TYPE(_u2ctrl) \
	((_u2ctrl & ML_CTRL_TYPE_MASK) >> ML_CTRL_TYPE_SHIFT)

#define BE_SET_ML_CTRL_PRESENCE(_u2ctrl, _ctrl_type) \
{\
	(_u2ctrl) &= ~(ML_CTRL_PRE_BMP_MASK); \
	(_u2ctrl) |= (((_ctrl_type) << (ML_CTRL_PRE_BMP_SHIFT)) \
	& (ML_CTRL_PRE_BMP_MASK)); \
}

#define BE_SET_MLD_ID_PRESENCE(_u2ctrl, _ctrl_type) \
{\
	(_u2ctrl) &= ~(ML_CTRL_PRE_BMP_MASK); \
	(_u2ctrl) |= (((_ctrl_type) << (ML_CTRL_PRE_BMP_SHIFT)) \
	& (ML_CTRL_PRE_BMP_MASK)); \
}

#define BE_GET_ML_CTRL_PRESENCE(_u2ctrl) \
	((_u2ctrl & ML_CTRL_PRE_BMP_MASK) >> ML_CTRL_PRE_BMP_SHIFT)

#define BE_IS_ML_CTRL_PRESENCE_LINK_ID(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_LINK_ID_INFO_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_ML_CTRL_PRESENCE_BSS_PARA_CHANGE_COUNT(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT << \
		ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_ML_CTRL_PRESENCE_MEDIUM_SYN_DELAY_INFO(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_MEDIUM_SYN_DELAY_INFO_PRESENT << \
		ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_ML_CTRL_PRESENCE_EML_CAP(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_EML_CAPA_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_EML_CAP_SUPPORT_EMLSR(_u2Cap) \
	((_u2Cap) & ML_CTRL_EML_CAPA_EMLSR_SUPPORT_MASK)

#define BE_IS_ML_CTRL_PRESENCE_MLD_CAP(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_MLD_CAPA_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_ML_CTRL_PRESENCE_MLD_ID(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_MLD_ID_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_ML_CTRL_PRESENCE_EXT_MLD(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_EXT_MLD_CAP_OP_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_SET_MLD_CAP_MAX_SIMULTANEOUS_LINKS(_u2Cap, _num) \
{\
	(_u2Cap) &= ~(MLD_CAP_MAX_SIMULTANEOUS_LINK_MASK); \
	(_u2Cap) |= (((_num) << (MLD_CAP_MAX_SIMULTANEOUS_LINK_SHIFT)) \
	& (MLD_CAP_MAX_SIMULTANEOUS_LINK_MASK)); \
}

#define BE_SET_MLD_CAP_TID_TO_LINK_NEGO(_u2Cap, _num) \
{\
	(_u2Cap) &= ~(MLD_CAP_TID_TO_LINK_NEGO_MASK); \
	(_u2Cap) |= (((_num) << (MLD_CAP_TID_TO_LINK_NEGO_SHIFT)) \
	& (MLD_CAP_TID_TO_LINK_NEGO_MASK)); \
}

#define BE_SET_MLD_CAP_LINK_RECFG_OP(_u2Cap) \
	((_u2Cap) |= MLD_CAP_LINK_RECFG_OP_SUPPORT)

#define BE_SET_EXT_MLD_CAP_BTM_MLD_RECOMM(_u2Cap) \
	((_u2Cap) |= EXT_MLD_CAP_BTM_MLD_RECOMM_MULTI_AP)

#define MLCIE(fp)              ((struct IE_MULTI_LINK_CONTROL *) fp)

/* BE D3.0 Figure 9-1002f - Multi-Link Control field */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_MULTI_LINK_CONTROL {
	uint8_t  ucId;
	uint8_t  ucLength;
	uint8_t  ucExtId;
	/* control field -
	 * BITS(0, 2): type
	 * BIT(3): reserved
	 * BITS(4, 15): presence bitmap
	 */
	uint16_t u2Ctrl;
	/* common field - varied by presence bitmap of control field*/
	uint8_t aucCommonInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct SUB_IE_MULTI_LINK_CONTROL {
	u_int8_t  ucId;
	u_int8_t  ucLength;
	u_int16_t u2Ctrl;
	u_int8_t aucCommonInfo[];
} __KAL_ATTRIB_PACKED__;


/* Basic Multi-Link element */

#define BE_SET_ML_STA_CTRL_LINK_ID(_u2ctrl, _val) \
{\
	(_u2ctrl) &= ~(ML_STA_CTRL_LINK_ID_MASK); \
	(_u2ctrl) |= (((_val) << (ML_STA_CTRL_LINK_ID_SHIFT)) \
	& (ML_STA_CTRL_LINK_ID_MASK)); \
}

#define BE_SET_ML_STA_CTRL_FIELD(_u2ctrl, _ctrl_type) \
	(_u2ctrl = _ctrl_type)

#define BE_SET_ML_STA_CTRL_COMPLETE_PROFILE(_u2ctrl) \
	(_u2ctrl |= ML_STA_CTRL_COMPLETE_PROFILE)

#define BE_UNSET_ML_STA_CTRL_COMPLETE_PROFILE(_u2ctrl) \
	(_u2ctrl &= ~ML_STA_CTRL_COMPLETE_PROFILE)

#define BE_SET_ML_STA_CTRL_NSTR_BITMAP_SIZE(_u2ctrl, _val) \
{\
	(_u2ctrl) &= ~(ML_STA_CTRL_NSTR_BMP_SIZE); \
	(_u2ctrl) |= (((_val) << (ML_STA_CTRL_NSTR_BMP_SIZE_SHIFT)) \
	& (ML_STA_CTRL_NSTR_BMP_SIZE)); \
}

#define BE_SET_ML_STA_CTRL_PRESENCE_MAC(_u2ctrl) \
	(_u2ctrl |= ML_STA_CTRL_MAC_ADDR_PRESENT)

#define BE_UNSET_ML_STA_CTRL_PRESENCE_MAC(_u2ctrl) \
	(_u2ctrl &= ~ML_STA_CTRL_MAC_ADDR_PRESENT)

#define BE_IS_ML_STA_CTRL_COMPLETE(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_COMPLETE_PROFILE)

#define BE_IS_ML_STA_CTRL_PRESENCE_MAC(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_MAC_ADDR_PRESENT)

#define BE_IS_ML_STA_CTRL_PRESENCE_BCN_INTV(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_BCN_INTV_PRESENT)

#define BE_IS_ML_STA_CTRL_PRESENCE_TSF_OFFSET(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_TSF_OFFSET_PRESENT)

#define BE_IS_ML_STA_CTRL_PRESENCE_DTIM(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_DTIM_INFO_PRESENT)

#define BE_IS_ML_STA_CTRL_PRESENCE_NSTR(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_NSTR_LINK_PAIR_PRESENT)

#define BE_IS_ML_STA_CTRL_PRESENCE_BSS_PARA_CHANGE_COUNT(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT)

/* Reconfiguration Multi-Link element */
#define BE_SET_ML_RECFG_STA_CTRL_LINK_ID(_u2ctrl, _val) \
{\
	(_u2ctrl) &= ~(ML_RECFG_STA_CTRL_LINK_ID_MASK); \
	(_u2ctrl) |= (((_val) << (ML_RECFG_STA_CTRL_LINK_ID_SHIFT)) \
	& (ML_RECFG_STA_CTRL_LINK_ID_MASK)); \
}

#define BE_SET_ML_RECFG_STA_CTRL_OP_TYPE(_u2ctrl, _val) \
{\
	(_u2ctrl) &= ~(ML_RECFG_STA_CTRL_OP_TYPE_MASK); \
	(_u2ctrl) |= (((_val) << (ML_RECFG_STA_CTRL_OP_TYPE_SHIFT)) \
	& (ML_RECFG_STA_CTRL_OP_TYPE_MASK)); \
}

#define BE_SET_M_RECFGL_STA_CTRL_FIELD(_u2ctrl, _ctrl_type) \
	(_u2ctrl = _ctrl_type)

/* BE D3.0 Figure 9-1002n - STA Control field format of the Basic Multi-Link
 * Element
 */
__KAL_ATTRIB_PACKED_FRONT__
struct IE_ML_STA_CONTROL {
	uint8_t ucSubID;	/* 0: Per-STA Profile */
	uint8_t ucLength;
	/*  control field
	 *  BITS(0, 3): link ID
	 *  BIT(4): complete profile
	 *  BITS(5, 8): presence bitmap
	 *  BITS(9): NSTR bitmap size
	 */
	uint16_t u2StaCtrl;
	uint8_t aucStaInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_NON_INHERITANCE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucList[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FRAGMENT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t pucData[];
} __KAL_ATTRIB_PACKED__;

/*802.11be D3.0 Figure 9-1002ao TID-to-Link Mapping element format*/
__KAL_ATTRIB_PACKED_FRONT__
struct IE_TID_TO_LINK_MAPPING {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t ucCtrl;
	uint8_t ucOptCtrl[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MULTI_LINK_TRAFFIC_INDICATION {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	/* Control field:
	 *     BITS(0, 3): Bitmap Size
	 *     BITS(4, 14): AID Offset
	 *     BIT(15): Reserved
	 */
	uint16_t u2Ctrl;
	uint8_t aucList[1];
} __KAL_ATTRIB_PACKED__;

/* 802.11be D6.0 9.6.38.13 Link Reconfiguration Request frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_LR_REQ_ACTION_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	uint8_t ucAction;	/* Action: 11 Link Reconfiguration Request */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t aucMultiLink[]; /* Reconfiguration Multi-Link Element */
} __KAL_ATTRIB_PACKED__;

/* 802.11be D6.0 9.6.38.14 Link Reconfiguration Response frame format */
__KAL_ATTRIB_PACKED_FRONT__
struct ACTION_LR_RESP_ACTION_FRAME {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	uint8_t aucBSSID[MAC_ADDR_LEN];	/* BSSID */
	uint16_t u2SeqCtrl;	/* Sequence Control */
	/* Action frame body */
	uint8_t ucCategory;	/* Category: 37 protected EHT */
	uint8_t ucAction;	/* Action: 12 Link Reconfiguration Response */
	uint8_t ucDialogToken;	/* Dialog Token */
	uint8_t ucCount;	/* Number of reconfiguration status duple */
	uint8_t aucReconfigStatus[]; /* Reconfiguration Status List */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct RECONFIG_STATUS {
	uint8_t ucLinkId;
	uint16_t u2Status;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct GROUP_KEY_DATA {
	uint8_t ucLength;
	uint8_t aucKeyData[];
} __KAL_ATTRIB_PACKED__;

#define MLO_GTK_MAX_LEN 32
#define MLO_IGTK_MAX_LEN 32
#define MLO_BIGTK_MAX_LEN 32


#ifndef RSN_SELECTOR
#define RSN_SELECTOR(a, b, c, d)                                               \
	((((u32)(a)) << 24) | (((u32)(b)) << 16) | (((u32)(c)) << 8) | (u32)(d))
#endif

#define RSN_KEY_DATA_MLO_GTK RSN_SELECTOR(0x00, 0x0f, 0xac, 16)
#define RSN_KEY_DATA_MLO_IGTK RSN_SELECTOR(0x00, 0x0f, 0xac, 17)
#define RSN_KEY_DATA_MLO_BIGTK RSN_SELECTOR(0x00, 0x0f, 0xac, 18)

#define MLO_GTK_KDE_PREFIX_LEN (1 + 6)
__KAL_ATTRIB_PACKED_FRONT__
struct MLO_GTK_KDE {
	uint8_t info; /* KeyId 2 | Tx 1 | Reserved 1 | LinkId 4 */
	uint8_t pn[6];
	uint8_t gtk[MLO_GTK_MAX_LEN];
} __KAL_ATTRIB_PACKED__;

#define MLO_IGTK_KDE_PREFIX_LEN (2 + 6 + 1)
__KAL_ATTRIB_PACKED_FRONT__
struct MLO_IGTK_KDE {
	uint8_t keyid[2];
	uint8_t pn[6];
	uint8_t info; /* Reserved 4 | LinkId 4 */
	uint8_t igtk[MLO_IGTK_MAX_LEN];
} __KAL_ATTRIB_PACKED__;

#define MLO_BIGTK_KDE_PREFIX_LEN (2 + 6 + 1)
__KAL_ATTRIB_PACKED_FRONT__
struct MLO_BIGTK_KDE {
	uint8_t keyid[2];
	uint8_t pn[6];
	uint8_t info; /* Reserved 4 | LinkId 4 */
	uint8_t bigtk[MLO_BIGTK_MAX_LEN];
} __KAL_ATTRIB_PACKED__;

struct STA_PROFILE {
	uint16_t u2StaCtrl;
	uint8_t ucComplete;
	uint8_t ucLinkId;
	uint8_t aucLinkAddr[MAC_ADDR_LEN];
	uint16_t u2BcnIntv;
	int64_t i8TsfOffset;
	uint16_t u2DtimInfo;
	uint16_t u2NstrBmp;
	uint16_t ucBssParaChangeCount;
	struct RF_CHANNEL_INFO rChnlInfo;
	uint8_t ucChangeSeq;
	uint16_t u2CapInfo;
	uint16_t u2StatusCode;
	uint16_t u2ApRemovalTimer;
	uint8_t ucOpType;
	uint32_t u4OpParam;
	uint16_t u2IEbufLen;
	uint8_t aucIEbuf[500];
};

struct MULTI_LINK_INFO {
	uint8_t ucValid;
	uint8_t	ucMlCtrlType;
	uint8_t	ucMlCtrlPreBmp;
	uint8_t ucCommonInfoLength;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t ucLinkId;
	uint8_t ucBssParaChangeCount;
	uint16_t u2MediumSynDelayInfo;
	uint16_t u2EmlCap;
	uint16_t u2MldCap;
	uint8_t ucMldId;
	uint16_t u2ExtMldCap;
	uint16_t u2ValidLinks; /* bitmap of valid MLO link IDs */
	uint8_t ucProfNum;
	struct STA_PROFILE rStaProfiles[MLD_LINK_MAX];
};

struct LR_RESP_INFO {
	struct MULTI_LINK_INFO rMlInfo;

	uint16_t u2ValidReconfigStatus; /* bitmap of valid reconfig status */
	uint16_t u2Status[MAX_NUM_MLO_LINKS];

	uint16_t u2ValidMloGTK; /* bitmap of valid link GTK KDEs */
	struct PARAM_KEY rMloGTK[MAX_NUM_MLO_LINKS];

	uint16_t u2ValidMloIGTK; /* bitmap of valid link IGTK KDEs */
	struct PARAM_KEY rMloIGTK[MAX_NUM_MLO_LINKS];

	uint16_t u2ValidMloBIGTK; /* bitmap of valid link BIGTK KDEs */
	struct PARAM_KEY rMloBIGTK[MAX_NUM_MLO_LINKS];
};

typedef struct MSDU_INFO* (*PFN_COMPOSE_ASSOC_IE_FUNC) (struct ADAPTER *,
	struct STA_RECORD *);

typedef struct MSDU_INFO* (*PFN_COMPOSE_PROBE_RESP_IE_FUNC) (
	struct ADAPTER *, uint8_t, uint8_t, uint8_t,
	struct WLAN_BEACON_FRAME *);

void mldGenerateAssocIE(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct MSDU_INFO *prMsduInfo,
	PFN_COMPOSE_ASSOC_IE_FUNC pfnComposeIE);

void mldGenerateProbeRspIE(
	struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo,
	uint8_t ucBssIdx, struct WLAN_BEACON_FRAME *prProbeRspFrame,
	PFN_COMPOSE_PROBE_RESP_IE_FUNC pfnComposeIE);

uint8_t *mldGenerateBasicCommonInfo(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2FrameCtrl);

uint32_t mldGenerateMlProbeReqIE(struct BSS_DESC *prBssDesc,
	uint8_t *pucIE, uint32_t u4IELength, uint8_t fgPerSta, uint8_t ucMldId);

uint32_t mldFillScanIE(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc,
	uint8_t *pucIE, uint32_t u4IELength, uint8_t fgPerSta, uint8_t ucMldId);

uint8_t *mldGenerateBasicCompleteProfile(
	struct ADAPTER *prAdapter,
	uint8_t *prIe,
	struct MSDU_INFO *prMsduInfo,
	uint32_t u4BeginOffset,
	uint32_t u4PrimaryLength,
	struct MSDU_INFO *prMsduInfoSta,
	uint8_t ucBssIndex);

uint32_t mldCalculateMlIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec);

void mldGenerateMlIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void mldGenerateMlIEImpl(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

uint32_t mldCalculateRnrIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec);

void mldGenerateRnrIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void mldParseBasicMlIE(struct MULTI_LINK_INFO *prMlInfo,
	const uint8_t *pucIE, uint16_t u2Left,
	const uint8_t *paucBssId, uint16_t u2FrameCtrl,
	const char *pucDesc);

#if (CFG_SUPPORT_ML_RECONFIG == 1)
void mldParseReconfigMlIE(struct MULTI_LINK_INFO *prMlInfo,
	const uint8_t *pucIE, const uint8_t *paucBssId,
	const char *pucDesc);

uint8_t *mldGenerateReconfigMlIE(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct MSDU_INFO *prMsduInfo,
	PFN_COMPOSE_ASSOC_IE_FUNC pfnComposeIE);

uint32_t mldParseLRRespActionFrame(struct ADAPTER *prAdapter,
	struct SW_RFB *prSrc, struct LR_RESP_INFO *prLrInfo,
	uint8_t ucBssIndex);

struct SW_RFB *mldReconfigBuildAssocSwRfb(struct ADAPTER *prAdapter,
	struct SW_RFB *prSrc, struct LR_RESP_INFO *prLrInfo,
	struct BSS_DESC *prBssDesc, struct STA_RECORD *prStaRec);
#endif /* CFG_SUPPORT_ML_RECONFIG */

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
void mldParsePriorityAccessMlIE(struct ADAPTER *prAdapter,
		struct MULTI_LINK_INFO *prMlInfo, struct SW_RFB *prSwRfb,
		const uint8_t *pucIE, uint16_t u2IELength, const char *pucDesc);

void mldParseStaProfilePriorityAccess(struct ADAPTER *prAdapter,
		struct MULTI_LINK_INFO *prMlInfo, struct SW_RFB *prSwRfb,
		uint8_t ucLinkId, uint16_t u2StaControl, const uint8_t *pos,
		uint16_t u2IELength);
#endif /* CFG_SUPPORT_802_11BE_EPCS */

const uint8_t *mldFindMlIE(const uint8_t *ies, uint16_t len, uint8_t type);

uint8_t mldProcessBeaconAndProbeResp(
	struct ADAPTER *prAdapter, struct SW_RFB *prSrc);

struct SW_RFB *mldDupAssocSwRfb(
	struct ADAPTER *prAdapter, struct SW_RFB *prSrc,
	struct STA_RECORD *prStaRec);

uint8_t mldSanityCheck(struct ADAPTER *prAdapter, uint8_t *pucPacket,
		uint16_t u2PacketLen, struct STA_RECORD *prStaRec,
		uint8_t ucBssIndex);

int mldDump(struct ADAPTER *prAdapter, uint8_t ucIndex,
	char *pcCommand, int i4TotalLen);

void mldBssDump(struct ADAPTER *prAdapter);

void mldBssUpdateMldAddrByMainBss(
	struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

void mldBssUpdateMldAddr(
	struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	const uint8_t *paucBssId);

void mldBssUpdateBandIdxBitmap(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

void mldBssUpdateCap(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	void *pvParam);

void mldBssRestoreCap(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

void mldBssSyncCap(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

int8_t mldBssRegister(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBss);

void mldBssUnregister(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBss);

struct MLD_BSS_INFO *mldBssAlloc(struct ADAPTER *prAdapter,
	const uint8_t aucMldMacAddr[]);

void mldBssFree(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

uint8_t mldBssAllowReconfig(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

struct MLD_BSS_INFO *mldBssGetByBss(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

struct MLD_BSS_INFO *mldBssGetByIdx(struct ADAPTER *prAdapter,
	uint8_t ucIdx);

uint16_t mldBssGetGrpFrameSn(struct ADAPTER *prAdapter,
			     struct MLD_BSS_INFO *prMldBssInfo);

int8_t mldBssInit(struct ADAPTER *prAdapter);

void mldBssUninit(struct ADAPTER *prAdapter);

struct MLD_STA_RECORD *mldBssGetPeekClient(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

void mldStarecDump(struct ADAPTER *prAdapter);

void mldBssTeardownAllClients(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

struct MLD_STA_RECORD *mldStarecJoin(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct STA_RECORD *prMainStarec,
	struct STA_RECORD *prStarec, struct BSS_DESC *prBssDesc);

int8_t mldStarecRegister(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec, struct STA_RECORD *prStarec,
	uint8_t ucLinkId);

void mldStarecUnregister(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStarec);

struct MLD_STA_RECORD *mldStarecAlloc(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct ML_INFO *prMlInfo);

void mldStarecFree(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec, struct STA_RECORD *prStarec);

uint8_t mldStarecGetValidLinkNum(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStaRec);

uint16_t mldStarecGetValidLinks(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStaRec);

struct MLD_STA_RECORD *mldStarecGetByStarec(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

uint8_t mldGetWlanIdxByBandAndBssid(struct ADAPTER *prAdapter,
	uint8_t ucHwBandIdx, uint8_t ucHwBssIdx, uint8_t ucWlanIdx);

uint8_t mldGetPrimaryWlanIdx(struct ADAPTER *prAdapter, uint8_t ucWlanIdx);

struct MLD_STA_RECORD *mldStarecGetByMldAddr(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	const uint8_t aucMacAddr[]);

struct MLD_STA_RECORD *mldStarecGetByLinkAddr(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	uint8_t aucMacAddr[]);

int8_t mldStarecSetSetupIdx(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

uint32_t mldUpdateTidBitmap(struct ADAPTER *prAdapter,
	 struct MLD_STA_RECORD *prMldStaRec);

#if (CFG_MLD_INFO_PRESETUP == 1)
int8_t mldSetupMlInfo(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);
#endif

void mldStarecLogRxData(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucHwBandIdx);

int8_t mldStarecInit(struct ADAPTER *prAdapter);

void mldStarecUninit(struct ADAPTER *prAdapter);

struct BSS_INFO *mldGetBssInfoByLinkID(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, uint8_t ucLinkIndex,
	uint8_t fgPeerSta);

struct STA_RECORD *mldGetStaRecByLinkId(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStaRec, uint8_t ucLinkId);

uint8_t mldIsMultiLinkFormed(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

uint8_t mldIsMultiLinkEnabled(
	struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType,
	uint8_t ucParam);

uint8_t mldIsSingleLinkEnabled(
	struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType,
	uint8_t ucParam);

uint8_t mldSingleLink(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint8_t ucBssIndex);

uint8_t mldCheckMldType(struct ADAPTER *prAdapter,
	uint8_t *pucIe, uint16_t u2Len);

struct STA_RECORD *mldGetStaRecByBandAndBssid(struct ADAPTER *prAdapter,
	uint8_t ucHwBandIdx, uint8_t ucHwBssIdx, uint8_t ucWlanIdx);

void mldCheckApRemoval(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, const uint8_t *pucIE);

enum ENUM_CH_REQ_TYPE mldDecideCnmReqCHType(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *mld_bssinfo);

#if (CFG_SINGLE_BAND_MLSR_56 == 1)
uint8_t mldNeedSingleBandMlsr56(struct ADAPTER *prAdapter,
	enum ENUM_MLO_LINK_PLAN eLinkPlan);
#endif /* CFG_SINGLE_BAND_MLSR_56 */

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
enum ENUM_MLO_MODE mldCheckMLSRType(struct ADAPTER *prAdapter);

uint8_t mldHasSingleLinkBss(struct ADAPTER *prAdapter);

enum ENUM_MLO_MODE mldNewConnectionType(struct ADAPTER *prAdapter,
	struct DBDC_DECISION_INFO *prDbdcDecisionInfo);

void mldClearMLSRPausedLinkFlag(struct ADAPTER *prAdapter);

void mldMLSRDecisionLinkRemain(struct ADAPTER *prAdapter,
	struct DBDC_DECISION_INFO *prDbdcDecisionInfo);

uint32_t mldSetRemainMLSRBssIndex(struct ADAPTER *prAdapter,
	uint8_t ucRemainBssIndex);

uint8_t mldNeedSTRAsMLSR(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *mld_bssinfo, uint8_t ucMloType);

uint8_t mldNeedEMLSRAsMLSR(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *mld_bssinfo, uint8_t ucMloType);
#endif

#if (CFG_SUPPORT_SAP_BCN_CRI_UPD == 1)
void mldIncBssParamChangeCount(struct ADAPTER *prAdapter,
			       struct MSDU_INFO *prMsduInfo);

u_int8_t mldCheckCriticalUpdate(struct ADAPTER *prAdapter,
				struct MSDU_INFO *prOldMsduInfo,
				struct MSDU_INFO *prNewMsduInfo);

void mldTriggerCriticalUpdate(struct ADAPTER *prAdapter,
			      uint8_t ucBssidx);
#endif /* CFG_SUPPORT_SAP_BCN_CRI_UPD */

#if (CFG_SUPPORT_MLC == 1)
uint32_t mldSendMlcRequest(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct PARAM_MLC_REQ *prMlcReq);
#endif /* CFG_SUPPORT_MLC */

uint8_t mldIsEmlsrPermittedAP(uint8_t *aucOui);

#endif /* !_MLO_H */
