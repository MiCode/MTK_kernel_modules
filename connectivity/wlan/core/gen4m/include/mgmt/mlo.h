/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*! \file   "mlo.h"
 *  \brief
 */

#ifndef _MLO_H
#define _MLO_H

#define IS_MLD_BSSINFO_MULTI(__prMldBssInfo) \
	(__prMldBssInfo && __prMldBssInfo->rBssList.u4NumElem > 1)

#define IS_MLD_STAREC_MULTI(__prMldStaRec) \
	(__prMldStaRec && __prMldStaRec->rStarecList.u4NumElem > 1)

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

#define BE_IS_ML_CTRL_PRESENCE_MLD_CAP(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_MLD_CAPA_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_IS_ML_CTRL_PRESENCE_MLD_ID(_u2ctrl) \
	(_u2ctrl & (ML_CTRL_MLD_ID_PRESENT << ML_CTRL_PRE_BMP_SHIFT))

#define BE_SET_MLD_CAP_MAX_SIMULTANEOUS_LINKS(_u2Cap, _num) \
{\
	(_u2Cap) &= ~(MLD_CAP_MAX_SIMULTANEOUS_LINK_MASK); \
	(_u2Cap) |= (((_num) << (MLD_CAP_MAX_SIMULTANEOUS_LINK_SHIFT)) \
	& (MLD_CAP_MAX_SIMULTANEOUS_LINK_MASK)); \
}

#define MLCIE(fp)              ((struct IE_MULTI_LINK_CONTROL *) fp)

__KAL_ATTRIB_PACKED_FRONT__
struct IE_MULTI_LINK_CONTROL {
	u_int8_t  ucId;
	u_int8_t  ucLength;
	u_int8_t  ucExtId;
	u_int16_t u2Ctrl;	/* control field - BITS(0, 2): type, BIT(3): reserved, BITS(4, 15): presence bitmap*/
	u_int8_t aucCommonInfo[0];	/* common field - varied by presence bitmap of control field*/
} __KAL_ATTRIB_PACKED__;

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

#define BE_IS_ML_STA_CTRL_PRESENCE_DTIM(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_DTIM_INFO_PRESENT)

#define BE_IS_ML_STA_CTRL_PRESENCE_NSTR(_u2ctrl) \
	(_u2ctrl & ML_STA_CTRL_NSTR_LINK_PAIR_PRESENT)

__KAL_ATTRIB_PACKED_FRONT__
struct IE_ML_STA_CONTROL {
	u_int8_t ucSubID;	/* 0: Per-STA Profile */
	u_int8_t ucLength;
	u_int16_t u2StaCtrl;	/* control field - BITS(0, 3): link ID, BIT(4): complete profile, BITS(5, 8): presence bitmap, BITS(9): NSTR bitmap size*/
	u_int8_t aucStaInfo[0];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_NON_INHERITANCE {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t aucList[0];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_FRAGMENT {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t pucData[0];
} __KAL_ATTRIB_PACKED__;

struct STA_PROFILE {
	uint16_t u2StaCtrl;
	uint8_t ucComplete;
	uint8_t ucLinkId;
	uint8_t aucLinkAddr[MAC_ADDR_LEN];
	uint16_t u2BcnIntv;
	uint64_t u8TsfOffset;
	uint16_t u2DtimInfo;
	uint16_t u2NstrBmp;
	uint16_t ucBssParaChangeCount;
	struct RF_CHANNEL_INFO rChnlInfo;
	uint8_t ucChangeSeq;
	uint16_t u2CapInfo;
	uint16_t u2StatusCode;
	uint16_t u2IEbufLen;
	uint8_t aucIEbuf[400];
};

struct MULTI_LINK_INFO {
	uint8_t ucValid;
	uint8_t	ucMlCtrlPreBmp;
	uint8_t ucCommonInfoLength;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t ucLinkId;
	uint8_t ucBssParaChangeCount;
	uint16_t u2MediumSynDelayInfo;
	uint16_t u2EmlCap;
	uint16_t u2MldCap;
	uint8_t ucMldId;
	uint16_t u2ValidLinks; /* bitmap of valid MLO link IDs */
	uint8_t ucProfNum;
	struct STA_PROFILE rStaProfiles[MLD_LINK_MAX];
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
	uint8_t *pucIE, uint32_t u4IELength);

uint32_t mldFillScanIE(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc,
	uint8_t *pucIE, uint32_t u4IELength, uint8_t ucBssIndex);

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

const uint8_t *mldFindMlIE(const uint8_t *ies, uint16_t len, uint8_t type);

void mldProcessBeaconAndProbeResp(
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

int8_t mldBssRegister(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBss);

void mldBssUnregister(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct BSS_INFO *prBss);

struct MLD_BSS_INFO *mldBssAlloc(struct ADAPTER *prAdapter);

void mldBssFree(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

uint8_t mldBssAllowReconfig(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

struct MLD_BSS_INFO *mldBssGetByBss(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

struct MLD_BSS_INFO *mldBssGetByIdx(struct ADAPTER *prAdapter,
	uint8_t ucIdx);

int8_t mldBssInit(struct ADAPTER *prAdapter);

void mldBssUninit(struct ADAPTER *prAdapter);

void mldStarecDump(struct ADAPTER *prAdapter);

uint8_t mldStarecExternalMldExist(struct ADAPTER *prAdapter);

void mldBssTeardownAllClients(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

#ifdef CFG_AAD_NONCE_NO_REPLACE
void mldBssDisableAllClients(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

void mldBssEnableAllClients(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo);

#endif

struct MLD_STA_RECORD *mldStarecJoin(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct STA_RECORD *prMainStarec,
	struct STA_RECORD *prStarec, struct BSS_DESC *prBssDesc);

int8_t mldStarecRegister(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec, struct STA_RECORD *prStarec,
	uint8_t ucLinkId);

void mldStarecUnregister(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStarec);

struct MLD_STA_RECORD *mldStarecAlloc(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	uint8_t *aucMacAddr, uint8_t fgMldType);

void mldStarecFree(struct ADAPTER *prAdapter,
	struct MLD_STA_RECORD *prMldStarec);

struct MLD_STA_RECORD *mldStarecGetByStarec(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

uint8_t mldGetPrimaryWlanIdx(struct ADAPTER *prAdapter, uint8_t ucWlanIdx);

struct MLD_STA_RECORD *mldStarecGetByMldAddr(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	uint8_t aucMacAddr[]);

struct MLD_STA_RECORD *mldStarecGetByLinkAddr(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	uint8_t aucMacAddr[]);

int8_t mldStarecSetSetupIdx(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

void mldStarecLogRxData(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucHwBandIdx);

int8_t mldStarecInit(struct ADAPTER *prAdapter);

void mldStarecUninit(struct ADAPTER *prAdapter);

struct BSS_INFO *mldGetBssInfoByLinkID(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, uint8_t ucLinkIndex,
	uint8_t fgPeerSta);

uint8_t mldGetBssIndexByHwBand(struct ADAPTER *prAdapter,
	uint8_t ucHwBandIdx, uint8_t ucBssIndex);

uint8_t mldIsMultiLinkFormed(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec);

uint8_t mldIsMloFeatureEnabled(
	struct ADAPTER *prAdapter,
	enum ENUM_NETWORK_TYPE eNetworkType,
	uint8_t fgIsApMode);

uint8_t mldSingleLink(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, uint8_t ucBssIndex);

uint8_t mldCheckMldType(struct ADAPTER *prAdapter,
	uint8_t *pucIe, uint16_t u2Len);

#endif /* !_MLO_H */
