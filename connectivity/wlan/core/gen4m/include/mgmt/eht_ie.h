/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _EHT_IE_H
#define _EHT_IE_H

#if (CFG_SUPPORT_802_11BE == 1)

#define EHT_MAC_CAP_BYTE_NUM                            (2)
#define EHT_PHY_CAP_BYTE_NUM                            (9)

__KAL_ATTRIB_PACKED_FRONT__
struct IE_EHT_CAP {
	u_int8_t  ucId;
	u_int8_t  ucLength;
	u_int8_t  ucExtId;
	u_int8_t  ucEhtMacCap[EHT_MAC_CAP_BYTE_NUM];
	u_int8_t  ucEhtPhyCap[EHT_PHY_CAP_BYTE_NUM]; /* BIT0 ~ BIT71 */
	u_int8_t  aucVarInfo[];
} __KAL_ATTRIB_PACKED__;

/* EHT Operation Parameters */
#define EHT_OP_PARAM_OP_INFO_PRESENT				BIT(0)
#define EHT_OP_PARAM_OP_INFO_PRESENT_SHIFT			0
#define EHT_OP_PARAM_DIS_SUBCHANNEL_PRESENT			BIT(1)
#define EHT_OP_PARAM_DIS_SUBCHANNEL_PRESENT_SHFT		1
#define EHT_OP_PARAM_DEFAULT_PE_DURATION			BIT(2)
#define EHT_OP_PARAM_DEFAULT_PE_DURATION_SHIFT			2
#define EHT_OP_PARAM_GROUP_ADDR_BU_INDICATION_LIMIT		BIT(3)
#define EHT_OP_PARAM_GROUP_ADDR_BU_INDICATION_SHIFT		3
#define EHT_OP_PARAM_GROUP_ADDR_BU_INDICATION_EXP		BITS(4, 5)
#define EHT_OP_PARAM_GROUP_ADDR_BU_INDICATION_EXP_SHIFT		4
#define EHT_OP_PARAM_MCS_15_DISABLE				BIT(6)
#define EHT_OP_PARAM_MCS_15_DISABLE_SHIFT			6


#define EHT_SET_OP_PARAM_OP_INFO_PRESENT(_aucEhtOpParams) \
	(_aucEhtOpParams |= EHT_OP_PARAM_OP_INFO_PRESENT)

#define EHT_IS_OP_PARAM_OP_INFO_PRESENT(_aucEhtOpParams) \
	((_aucEhtOpParams & EHT_OP_PARAM_OP_INFO_PRESENT) \
	== EHT_OP_PARAM_OP_INFO_PRESENT)

#define EHT_SET_OP_PARAM_DIS_SUBCHANNEL_PRESENT(_aucEhtOpParams) \
	(_aucEhtOpParams |= EHT_OP_PARAM_DIS_SUBCHANNEL_PRESENT)

#define EHT_RESET_OP_PARAM_DIS_SUBCHANNEL_PRESENT(_aucEhtOpParams) \
	(_aucEhtOpParams &= (~EHT_OP_PARAM_DIS_SUBCHANNEL_PRESENT))

#define EHT_IS_OP_PARAM_DIS_SUBCHANNEL_PRESENT(_aucEhtOpParams) \
	((_aucEhtOpParams & EHT_OP_PARAM_DIS_SUBCHANNEL_PRESENT) \
	== EHT_OP_PARAM_DIS_SUBCHANNEL_PRESENT)

#define EHT_SET_OP_PARAM_MCS_15_DISABLE(_aucEhtOpParams) \
	(_aucEhtOpParams |= EHT_OP_PARAM_MCS_15_DISABLE)

#define EHT_RESET_OP_PARAM_MCS_15_DISABLE(_aucEhtOpParams) \
	(_aucEhtOpParams &= (~EHT_OP_PARAM_MCS_15_DISABLE))

#define EHT_IS_OP_PARAM_MCS_15_DISABLE(_aucEhtOpParams) \
	((_aucEhtOpParams & EHT_OP_PARAM_MCS_15_DISABLE) \
	== EHT_OP_PARAM_MCS_15_DISABLE)

#define BW_INDICATION_PARAM_DIS_SUBCHANNEL_PRESENT             BIT(1)

#define BW_INDICATION_SET_DIS_SUBCHANNEL_PRESENT(_ucParam) \
	(_ucParam |= BW_INDICATION_PARAM_DIS_SUBCHANNEL_PRESENT)
#define BW_INDICATION_IS_DIS_SUBCHANNEL_PRESENT(_ucParam) \
	((_ucParam & BW_INDICATION_PARAM_DIS_SUBCHANNEL_PRESENT) \
	== BW_INDICATION_PARAM_DIS_SUBCHANNEL_PRESENT)

__KAL_ATTRIB_PACKED_FRONT__
struct EHT_DSCB_INFO {
	u_int16_t u2DisSubChannelBitmap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_EHT_OP {
	u_int8_t  ucId;
	u_int8_t  ucLength;
	u_int8_t  ucExtId;
	u_int8_t  ucEhtOpParams;
	u_int32_t u4BasicEhtMcsNssSet;
	u_int8_t  aucVarInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct EHT_OP_INFO {
	u_int8_t  ucControl;
	u_int8_t  ucCCFS0;
	u_int8_t  ucCCFS1;
	u_int16_t  u2EhtDisSubChanBitmap;
	/* DO NOT use zero-array in middle of other struct
	 * which confuse size of compiler other than gcc
	 * its used in the middle of struct BSS_INFO
	 * if use zero-arry to calculate size with OFFSET_OF
	 * please use packed and sizeof
	 */
	/* u_int8_t  aucVarInfo[]; */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct IE_BW_INDICATION {
	uint8_t ucId;
	uint8_t ucLength;
	uint8_t ucExtId;
	uint8_t ucParam;
	uint8_t aucVarInfo[];
} __KAL_ATTRIB_PACKED__;

#define EHT_RESET_MAC_CAP(_aucMacCapInfo) \
	(memset(_aucMacCapInfo, 0, EHT_MAC_CAP_BYTE_NUM))

#define EHT_RESET_PHY_CAP(_aucPhyCapInfo) \
	(memset(_aucPhyCapInfo, 0, EHT_PHY_CAP_BYTE_NUM))

#define EHT_RESET_OP(_aucOpInfo) \
	(_aucOpInfo = 0)

/* EHT MAC Capabilities Information field */
/*
 * Indicates support for EPCS priority access.
 */
#define EHT_MAC_CAP_EPCS_PRI_ACCESS BIT(0)

#define SET_EHT_MAC_CAP_EPCS_PRI_ACCESS(_aucMacCapInfo) \
	(_aucMacCapInfo[0] |= EHT_MAC_CAP_EPCS_PRI_ACCESS)

/*
 * Indicates support for receiving a frame with an EHT OM Control subfield.
 */
#define EHT_MAC_CAP_OM_CTRL BIT(1)

#define SET_EHT_MAC_CAP_OM_CTRL(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_OM_CTRL)

/*
 * Indicates support for transmitting or responding
 * to a TXOP sharing trigger frame with Triggered TXOP Sharing Mode field
 * equal to 1 that does not solicit TB PPDU.
 */
#define EHT_MAC_CAP_TXOP_SHARING_MODE1 BIT(2)

#define SET_EHT_MAC_CAP_TXOP_SHARING_MODE1(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_TXOP_SHARING_MODE1)

/*
 * Indicates support for transmitting or responding
 * to a TXOP sharing trigger frame with Triggered TXOP Sharing Mode field
 * equal to 2 that does not solicit TB PPDU.
 */
#define EHT_MAC_CAP_TXOP_SHARING_MODE2 BIT(3)

#define SET_EHT_MAC_CAP_TXOP_SHARING_MODE2(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_TXOP_SHARING_MODE2)

/*
 * Indicates support for the restricted TWT operation.
 */
#define EHT_MAC_CAP_RESTRICTED_TWT BIT(4)

#define SET_EHT_MAC_CAP_RESTRICTED_TWT(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_RESTRICTED_TWT)

#define GET_EHT_MAC_CAP_RESTRICTED_TWT(_aucMacCapInfo) \
		(_aucMacCapInfo[0] & EHT_MAC_CAP_RESTRICTED_TWT)


/*
 * Indicates support for transmission and
 * reception of SCS Descriptor elements containing
 * a QoS Characteristics subelement
 */
#define EHT_MAC_CAP_SCS BIT(5)

#define SET_EHT_MAC_CAP_SCS(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_SCS)

#define SET_EHT_MAC_CAP_MPDU_LENGTH_8K(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_MAX_MPDU_LEN_8K)
#define SET_EHT_MAC_CAP_MPDU_LENGTH_11K(_aucMacCapInfo) \
		(_aucMacCapInfo[0] |= EHT_MAC_CAP_MAX_MPDU_LEN_11K)

/*
 * Indicates the maximum MPDU length
 * that the STA is capable of receiving
 */
#define EHT_MAC_CAP_MAX_MPDU_LEN_3K		0
#define EHT_MAC_CAP_MAX_MPDU_LEN_8K		BIT(6)
#define EHT_MAC_CAP_MAX_MPDU_LEN_11K		BIT(7)
#define EHT_MAC_CAP_MAX_MPDU_LEN_MASK		BITS(6, 7)
#define EHT_MAC_CAP_MAX_MPDU_LEN_OFFSET

/* EHT MAC Capablilites byte1 */
/*
 * Indicates support for Unsolicited EPCS priority access parameter update
 * 802.11be D7.0
 */
#define EHT_MAC_CAP_U_EPCS_PRI_ACCESS BIT(6)

#define SET_EHT_MAC_CAP_U_EPCS_PRI_ACC(_aucMacCapInfo) \
	(_aucMacCapInfo[1] |= EHT_MAC_CAP_U_EPCS_PRI_ACCESS)
/* 802.11be D1.3 9.2.4.6.4 A-Control */
#define HTC_EHT_A_CTRL_OM			7

/* 11ax_D3.0 9.2.4.6 HT Control field */
#define EHT_HTC_HE_VARIANT                                 BITS(0, 1)

/* 1st - EHT OM_Ctrl, 2nd - OM_Ctrl */
#define EHT_HTC_HE_1ST_A_CTRL_ID                           BITS(2, 5)
#define EHT_HTC_HE_1ST_A_CTRL_ID_SHIFT                      2

/* 1st - EHT OM_Ctrl, 2nd - OM_Ctrl */
#define EHT_HTC_HE_2ND_A_CTRL_ID                           BITS(12, 15)
#define EHT_HTC_HE_2ND_A_CTRL_ID_SHIFT                     12

/* 802.11be D1.3 9.2.4.7.8 EHT OM Control */
#define HTC_EHT_OM_RX_NSS_EXT			BIT(6)
#define HTC_EHT_OM_RX_NSS_EXT_SHFT		6
#define HTC_EHT_OM_CH_WIDTH_EXT			BIT(7)
#define HTC_EHT_OM_CH_WIDTH_EXT_SHFT		7
#define HTC_EHT_OM_TX_NSTS_EXT			BIT(8)
#define HTC_EHT_OM_TX_NSTS_EXT_SHFT		8
#define HTC_EHT_OM_RESERVED			BITS(9, 11)
#define HTC_EHT_OM_RESERVED_SHFT		9
#define EHT_OM_CH_WIDTH_BW320			0
#define EHT_OM_CH_WIDTH_EXT_LT_BW320		0
#define EHT_OM_CH_WIDTH_EXT_BW320		1

/* 11ax_D3.0 9.2.4.6a.2 OM Control */
#define EHT_HTC_HE_OM_RX_NSS                               BITS(16, 18)
#define EHT_HTC_HE_OM_RX_NSS_SHFT                          16
#define EHT_HTC_HE_OM_CH_WIDTH                             BITS(19, 20)
#define EHT_HTC_HE_OM_CH_WIDTH_SHFT                        19
#define EHT_HTC_HE_OM_UL_MU_DISABLE                        BIT(21)
#define EHT_HTC_HE_OM_UL_MU_DISABLE_SHFT                   21
#define EHT_HTC_HE_OM_TX_NSTS                              BITS(22, 24)
#define EHT_HTC_HE_OM_TX_NSTS_SHFT                         22
#define EHT_HTC_HE_OM_ER_SU_DISABLE                        BIT(25)
#define EHT_HTC_HE_OM_ER_SU_DISABLE_SHFT                   25
#define EHT_HTC_HE_OM_DL_MUMIMO_RESND_RECMD                BIT(26)
#define EHT_HTC_HE_OM_DL_MUMIMO_RESND_RECMD_SHFT           26
#define EHT_HTC_HE_OM_UL_MU_DATA_DISABLE                   BIT(27)
#define EHT_HTC_HE_OM_UL_MU_DATA_DISABLE_SHFT              27

/* MCS-NSS map (except 20MHz only sta) byte0 */
#define EHT_MCS_MAP_RX_MCS0_9_NSS_SHIFT              0
#define EHT_MCS_MAP_RX_MCS0_9_NSS_MASK               BITS(0, 3)
#define EHT_MCS_MAP_TX_MCS0_9_NSS_SHIFT              4
#define EHT_MCS_MAP_TX_MCS0_9_NSS_MASK               BITS(4, 7)
/* MCS-NSS map (except 20MHz only sta) byte1 */
#define EHT_MCS_MAP_RX_MCS10_11_NSS_SHIFT            0
#define EHT_MCS_MAP_RX_MCS10_11_NSS_MASK             BITS(0, 3)
#define EHT_MCS_MAP_TX_MCS10_11_NSS_SHIFT            4
#define EHT_MCS_MAP_TX_MCS10_11_NSS_MASK             BITS(4, 7)
/* MCS-NSS map (except 20MHz only sta) byte2 */
#define EHT_MCS_MAP_RX_MCS12_13_NSS_SHIFT            0
#define EHT_MCS_MAP_RX_MCS12_13_NSS_MASK             BITS(0, 3)
#define EHT_MCS_MAP_TX_MCS12_13_NSS_SHIFT            4
#define EHT_MCS_MAP_TX_MCS12_13_NSS_MASK             BITS(4, 7)

#define EHT_SET_HTC_HE_VARIANT(_u4HTC) \
	(_u4HTC |= EHT_HTC_HE_VARIANT)

/* Control ID */
#define EHT_SET_HTC_1ST_A_CTRL_ID(_u4HTC, _ctrl_id) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_1ST_A_CTRL_ID); \
(_u4HTC) |= (((_ctrl_id) << (EHT_HTC_HE_1ST_A_CTRL_ID_SHIFT)) \
	& (EHT_HTC_HE_1ST_A_CTRL_ID)); \
}
#define EHT_SET_HTC_2ND_A_CTRL_ID(_u4HTC, _ctrl_id) \
{\
	(_u4HTC) &= ~(EHT_HTC_HE_2ND_A_CTRL_ID); \
	(_u4HTC) |= (((_ctrl_id) << (EHT_HTC_HE_2ND_A_CTRL_ID_SHIFT)) \
	& (EHT_HTC_HE_2ND_A_CTRL_ID)); \
}

/* OM - RX NSS */
#define EHT_SET_HTC_HE_OM_RX_NSS(_u4HTC, _rx_nss) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_OM_RX_NSS); \
(_u4HTC) |= (((_rx_nss) << (EHT_HTC_HE_OM_RX_NSS_SHFT)) \
	& (EHT_HTC_HE_OM_RX_NSS)); \
}
/* OM - Channel Width */
#define EHT_SET_HTC_HE_OM_CH_WIDTH(_u4HTC, _bw) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_OM_CH_WIDTH); \
(_u4HTC) |= (((_bw) << (EHT_HTC_HE_OM_CH_WIDTH_SHFT)) \
	& (EHT_HTC_HE_OM_CH_WIDTH)); \
}
/* OM - UL MU Disable */
#define EHT_SET_HTC_HE_OM_UL_MU_DISABLE(_u4HTC, _ul_dis) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_OM_UL_MU_DISABLE); \
(_u4HTC) |= (((_ul_dis) << (EHT_HTC_HE_OM_UL_MU_DISABLE_SHFT)) \
	& (EHT_HTC_HE_OM_UL_MU_DISABLE)); \
}
/* OM - TX NSTS */
#define EHT_SET_HTC_HE_OM_TX_NSTS(_u4HTC, _tx_nsts) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_OM_TX_NSTS); \
(_u4HTC) |= (((_tx_nsts) << (EHT_HTC_HE_OM_TX_NSTS_SHFT)) \
	& (EHT_HTC_HE_OM_TX_NSTS)); \
}
/* OM - ER SU Disable */
#define EHT_SET_HTC_HE_OM_ER_SU_DISABLE(_u4HTC, _er_dis) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_OM_ER_SU_DISABLE); \
(_u4HTC) |= (((_er_dis) << (EHT_HTC_HE_OM_ER_SU_DISABLE_SHFT)) \
	& (EHT_HTC_HE_OM_ER_SU_DISABLE)); \
}
/* OM - UL MU Data Disable */
#define EHT_SET_HTC_HE_OM_UL_MU_DATA_DISABLE(_u4HTC, _ul_data_dis) \
{\
(_u4HTC) &= ~(EHT_HTC_HE_OM_UL_MU_DATA_DISABLE); \
(_u4HTC) |= (((_ul_data_dis) << (EHT_HTC_HE_OM_UL_MU_DATA_DISABLE_SHFT)) \
	& (EHT_HTC_HE_OM_UL_MU_DATA_DISABLE)); \
}
/* OM - EHT RX NSS EXT */
#define EHT_SET_HTC_EHT_OM_RX_NSS_EXT(_u4HTC, _bw) \
{\
(_u4HTC) &= ~(HTC_EHT_OM_RX_NSS_EXT); \
(_u4HTC) |= (((_bw) << (HTC_EHT_OM_RX_NSS_EXT_SHFT)) \
	& (HTC_EHT_OM_RX_NSS_EXT)); \
}
/* OM - EHT Channel Width EXT */
#define EHT_SET_HTC_EHT_OM_CH_WIDTH_EXT(_u4HTC, _bw) \
{\
(_u4HTC) &= ~(HTC_EHT_OM_CH_WIDTH_EXT); \
(_u4HTC) |= (((_bw) << (HTC_EHT_OM_CH_WIDTH_EXT_SHFT)) \
	& (HTC_EHT_OM_CH_WIDTH_EXT)); \
}
/* OM - EHT TX NSTS EXT */
#define EHT_SET_HTC_EHT_OM_TX_NSTS_EXT(_u4HTC, _tx_nsts) \
{\
(_u4HTC) &= ~(HTC_EHT_OM_TX_NSTS_EXT); \
(_u4HTC) |= (((_tx_nsts) << (HTC_EHT_OM_TX_NSTS_EXT_SHFT)) \
	& (HTC_EHT_OM_TX_NSTS_EXT)); \
}

#define STAREC_SET_EHT_RX_160MHZ_MCS0_9_NSS(_prStaRec, _ucNss) \
	do { \
		(_prStaRec)->aucMcsMap160MHz[0] &= \
		(~(EHT_MCS_MAP_RX_MCS0_9_NSS_MASK)); \
		(_prStaRec)->aucMcsMap160MHz[0] |= \
		((uint32_t)(_ucNss << EHT_MCS_MAP_RX_MCS0_9_NSS_SHIFT) & \
		EHT_MCS_MAP_RX_MCS0_9_NSS_MASK); \
	} while (0)

#define STAREC_SET_EHT_RX_160MHZ_MCS10_11_NSS(_prStaRec, _ucNss) \
	do { \
		(_prStaRec)->aucMcsMap160MHz[1] &= \
		(~(EHT_MCS_MAP_RX_MCS10_11_NSS_MASK)); \
		(_prStaRec)->aucMcsMap160MHz[1] |= \
		((uint32_t)(_ucNss << EHT_MCS_MAP_RX_MCS10_11_NSS_SHIFT) & \
		EHT_MCS_MAP_RX_MCS10_11_NSS_MASK); \
	} while (0)

#define STAREC_SET_EHT_RX_160MHZ_MCS12_13_NSS(_prStaRec, _ucNss) \
	do { \
		(_prStaRec)->aucMcsMap160MHz[2] &= \
		(~(EHT_MCS_MAP_RX_MCS12_13_NSS_MASK)); \
		(_prStaRec)->aucMcsMap160MHz[2] |= \
		((uint32_t)(_ucNss << EHT_MCS_MAP_RX_MCS12_13_NSS_SHIFT) & \
		EHT_MCS_MAP_RX_MCS12_13_NSS_MASK); \
	} while (0)

#define STAREC_SET_EHT_RX_320MHZ_MCS0_9_NSS(_prStaRec, _ucNss) \
	do { \
		(_prStaRec)->aucMcsMap320MHz[0] &= \
		(~(EHT_MCS_MAP_RX_MCS0_9_NSS_MASK)); \
		(_prStaRec)->aucMcsMap320MHz[0] |= \
		((uint32_t)(_ucNss << EHT_MCS_MAP_RX_MCS0_9_NSS_SHIFT) & \
		EHT_MCS_MAP_RX_MCS0_9_NSS_MASK); \
	} while (0)

#define STAREC_SET_EHT_RX_320MHZ_MCS10_11_NSS(_prStaRec, _ucNss) \
	do { \
		(_prStaRec)->aucMcsMap320MHz[1] &= \
		(~(EHT_MCS_MAP_RX_MCS10_11_NSS_MASK)); \
		(_prStaRec)->aucMcsMap320MHz[1] |= \
		((uint32_t)(_ucNss << EHT_MCS_MAP_RX_MCS10_11_NSS_SHIFT) & \
		EHT_MCS_MAP_RX_MCS10_11_NSS_MASK); \
	} while (0)

#define STAREC_SET_EHT_RX_320MHZ_MCS12_13_NSS(_prStaRec, _ucNss) \
	do { \
		(_prStaRec)->aucMcsMap320MHz[2] &= \
		(~(EHT_MCS_MAP_RX_MCS12_13_NSS_MASK)); \
		(_prStaRec)->aucMcsMap320MHz[2] |= \
		((uint32_t)(_ucNss << EHT_MCS_MAP_RX_MCS12_13_NSS_SHIFT) & \
		EHT_MCS_MAP_RX_MCS12_13_NSS_MASK); \
	} while (0)

/* EHT PHY Capabilities Information field */

/* Support Of MCS 15 subfield */
enum EHT_MRU_SET_MCS15 {
	MRU_106TONE_OR_52TONE_W_26TONE = 1,
	MRU_484TONE_W_242TONE_IN_80M = (1 << 1),
	MRU_996TONE_TO_242TONE_IN_160M = (1 << 2),
	MRU_TRIPPLE_996TONE_IN_320M = (1 << 3),
};

/* Common Nominal Packet Padding */
enum EHT_COMMON_NOMINAL_PADD_US {
	COMMON_NOMINAL_PAD_0_US,
	COMMON_NOMINAL_PAD_8_US,
	COMMON_NOMINAL_PAD_16_US,
	COMMON_NOMINAL_PAD_20_US,
};

/* Maximum Number Of Supported EHT-LTFs */
enum EHT_MAX_LTF {
	MAX_EHT_LTF_NUM_4,
	MAX_EHT_LTF_NUM_8,
};

enum EHT_BF_CAPS {
	EHT_SU_BFER = 1,
	EHT_SU_BFEE = 1 << 1,
	EHT_MU_BFER = 1 << 2,
	EHT_BFEE_NG16_SU_FEEDBACK = 1 << 3,
	EHT_BFEE_NG32_SU_FEEDBACK = 1 << 4,
	EHT_BFEE_CODEBOOK_4_2_SU_FEEDBACK = 1 << 5,
	EHT_BFEE_CODEBOOK_7_5_SU_FEEDBACK = 1 << 6,
	EHT_TRIGED_SU_BF_FEEDBACK = 1 << 7,
	EHT_TRIGED_MU_BF_PATIAL_BW_FEEDBACK = 1 << 8,
};

enum EHT_GI_CAPS {
	EHT_NDP_4X_EHT_LTF_3DOT2US_GI = 1,
	EHT_MU_PPDU_4X_EHT_LTF_DOT8US_GI = 1 << 1,
};

enum EHT_MCS15_MRU_SUPPORT {
	EHT_MCS15_MRU_106_or_52_w_26_tone = 1,
	EHT_MCS15_MRU_484_w_242_tone_80M = 1 << 1,
	EHT_MCS15_MRU_996_to_242_tone_160M = 1 << 2,
	EHT_MCS15_MRU_3x996_tone_320M = 1 << 3,
};

enum EHT_PHY_CAPS {
	EHT_320M_6G = 1,
	EHT_242_TONE_RU_WT_20M = 1 << 1,
	EHT_PARTIAL_BW_UL_MI_MIMO  = 1 << 2,
	EHT_TRIGED_CQI_FEEDBACK = 1 << 3,
	EHT_PARTIAL_BW_DL_MU_MIMO = 1 << 4,
	EHT_PSR_BASED_SR = 1 << 5,
	EHT_POWER_BOOST_FACTOR = 1 << 6,
	EHT_NON_TRIGED_CQI_FEEDBACK = 1 << 7,
	EHT_TX_1024QAM_4096QAM_LE_242_TONE_RU = 1 << 8,
	EHT_RX_1024QAM_4096QAM_LE_242_TONE_RU = 1 << 9,
	EHT_PPE_THRLD_PRESENT = 1 << 10,
	EHT_EXTRA_LTF_SUPPORT = 1 << 11,
	EHT_DUP_6G = 1 << 12,
	EHT_20M_RX_NDP_W_WIDER_BW = 1 << 13,
};

/* EHT Operation Information subfields - Channel Width */
enum EHT_OP_MAX_BW {
	EHT_MAX_BW_20 = 0,
	EHT_MAX_BW_40 = 1,
	EHT_MAX_BW_80 = 2,
	EHT_MAX_BW_160 = 3,
	EHT_MAX_BW_320 = 4,
};

struct EHT_BF_INFO {
	enum EHT_BF_CAPS bf_cap;
	uint8_t bfee_ss_le_eq_bw80;
	uint8_t bfee_ss_bw160;
	uint8_t bfee_ss_bw320;
	uint8_t snd_dim_le_eq_bw80;
	uint8_t snd_dim_bw160;
	uint8_t snd_dim_bw320;
	uint8_t bfee_max_nc;
	uint8_t max_ltf_num_su_non_ofdma;
	uint8_t max_ltf_num_mu;
};

__KAL_ATTRIB_PACKED_FRONT__
struct EHT_PHY_CAP_INFO {
	uint32_t phy_capinfo_1;
	uint32_t phy_capinfo_2;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct EHT_SUPPORTED_MCS_BW20_FIELD {
	u_int8_t eht_bw20_mcs_0_7;
	u_int8_t eht_bw20_mcs_8_9;
	u_int8_t eht_bw20_mcs_10_11;
	u_int8_t eht_bw20_mcs_12_13;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct EHT_SUPPORTED_MCS_BW80_160_320_FIELD {
	u_int8_t eht_mcs_0_9;
	u_int8_t eht_mcs_10_11;
	u_int8_t eht_mcs_12_13;
} __KAL_ATTRIB_PACKED__;

#define SET_CAP_BITS(_cap, _capinfo, _value)\
{\
	_capinfo &= ~_cap##_MASK;\
	_capinfo |= ((_value << _cap##_SHIFT)\
			& _cap##_MASK);\
}

#define GET_CAP_BITS(_cap, _capinfo)\
	(((_capinfo) & _cap##_MASK)\
		>> _cap##_SHIFT)

#define CAP_BITS_MASK(_cap)\
	BITS(_cap##_SHIFT, (_cap##_SHIFT + _cap##_BITS - 1))

/* phy_capinfo_1: bit0..bit31 */
#define DOT11BE_PHY_CAP_320M_6G BIT(1)
#define DOT11BE_PHY_CAP_242_TONE_RU_WT_20M BIT(2)
#define DOT11BE_PHY_CAP_NDP_4X_EHT_LTF_3DOT2US_GI BIT(3)
#define DOT11BE_PHY_CAP_PARTIAL_BW_UL_MU_MIMO BIT(4)
#define DOT11BE_PHY_CAP_SU_BFER BIT(5)
#define DOT11BE_PHY_CAP_SU_BFEE BIT(6)
#define DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M_BITS 3
#define DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M_SHIFT 7
#define DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M_MASK	\
	CAP_BITS_MASK(DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M)
#define SET_DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_BFEE_SS_LE_EQ_80M, _capinfo)
#define DOT11BE_PHY_CAP_BFEE_SS_160M_BITS 3
#define DOT11BE_PHY_CAP_BFEE_SS_160M_SHIFT 10
#define DOT11BE_PHY_CAP_BFEE_SS_160M_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_BFEE_SS_160M)
#define SET_DOT11BE_PHY_CAP_BFEE_160M(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_BFEE_SS_160M, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_BFEE_160M(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_BFEE_SS_160M, _capinfo)
#define DOT11BE_PHY_CAP_BFEE_SS_320M_BITS 3
#define DOT11BE_PHY_CAP_BFEE_SS_320M_SHIFT 13
#define DOT11BE_PHY_CAP_BFEE_SS_320M_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_BFEE_SS_320M)
#define SET_DOT11BE_PHY_CAP_BFEE_320M(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_BFEE_SS_320M, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_BFEE_320M(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_BFEE_SS_320M, _capinfo)
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M_BITS 3
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M_SHIFT 16
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M)
#define SET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_SOUND_DIM_NUM_LE_EQ_80M, _capinfo)
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M_BITS 3
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M_SHIFT 19
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M)
#define SET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_SOUND_DIM_NUM_160M, _capinfo)
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M_BITS 3
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M_SHIFT 22
#define DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M)
#define SET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_SOUND_DIM_NUM_320M, _capinfo)
#define DOT11BE_PHY_CAP_NG16_SU_FEEDBACK BIT(25)
/* TODO: name and definition do not match, table 9-322ap */
#define DOT11BE_PHY_CAP_NG16_MU_FEEDBACK BIT(26)
#define DOT11BE_PHY_CAP_CODEBOOK_4_2_SU_FEEDBACK BIT(27)
/* TODO: name and definition do not match, table 9-322ap */
#define DOT11BE_PHY_CAP_CODEBOOK_7_5_MU_FEEDBACK BIT(28)
#define DOT11BE_PHY_CAP_TRIGED_SU_BF_FEEDBACK BIT(29)
#define DOT11BE_PHY_CAP_TRIGED_MU_BF_PARTIAL_BW_FEEDBACK BIT(30)
#define DOT11BE_PHY_CAP_TRIGED_CQI_FEEDBACK BIT(31)


/* phy_capinfo_2: bit32..bit63 */
#define DOT11BE_PHY_CAP_PARTIAL_BW_DL_MU_MIMO BIT(0)
#define DOT11BE_PHY_CAP_PSR_BASED_SR BIT(1)
#define DOT11BE_PHY_CAP_POWER_BOOST_FACTOR BIT(2)
#define DOT11BE_PHY_CAP_EHT_MU_PPDU_4X_EHT_LTF_DOT8US_GI BIT(3)
#define DOT11BE_PHY_CAP_MAX_NC_BITS 4
#define DOT11BE_PHY_CAP_MAX_NC_SHIFT 4
#define DOT11BE_PHY_CAP_MAX_NC_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_MAX_NC)
#define SET_DOT11BE_PHY_CAP_MAX_NC(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_MAX_NC, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_MAX_NC(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_MAX_NC, _capinfo)
#define DOT11BE_PHY_CAP_NON_TRIGED_CQI_FEEDBACK BIT(8)
#define DOT11BE_PHY_CAP_TX_1024QAM_4096QAM_LE_242_TONE_RU BIT(9)
#define DOT11BE_PHY_CAP_RX_1024QAM_4096QAM_LE_242_TONE_RU BIT(10)
#define DOT11BE_PHY_CAP_PPE_THRLD_PRESENT BIT(11)
#define DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD_BITS 2
#define DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD_SHIFT 12
#define DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD)
#define SET_DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_COMMON_NOMINAL_PKT_PAD, _capinfo)
#define DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM_BITS 5
#define DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM_SHIFT 14
#define DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM)
#define SET_DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_MAX_EHT_LTF_NUM, _capinfo)
#define DOT11BE_PHY_CAP_MCS_15_BITS 4
#define DOT11BE_PHY_CAP_MCS_15_SHIFT 19
#define DOT11BE_PHY_CAP_MCS_15_MASK \
	CAP_BITS_MASK(DOT11BE_PHY_CAP_MCS_15)
#define SET_DOT11BE_PHY_CAP_MCS_15(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_PHY_CAP_MCS_15, _capinfo, _value)
#define GET_DOT11BE_PHY_CAP_MCS_15(_capinfo) \
	GET_CAP_BITS(DOT11BE_PHY_CAP_MCS_15, _capinfo)
#define DOT11BE_PHY_CAP_EHT_DUP_6G BIT(23)
#define DOT11BE_PHY_CAP_20M_RX_NDP_W_WIDER_BW BIT(24)
#define DOT11BE_PHY_CAP_NON_OFDMA_UL_MU_MIMO_80M BIT(25)
#define DOT11BE_PHY_CAP_NON_OFDMA_UL_MU_MIMO_160M BIT(26)
#define DOT11BE_PHY_CAP_NON_OFDMA_UL_MU_MIMO_320M BIT(27)
#define DOT11BE_PHY_CAP_MU_BFER_80M BIT(28)
#define DOT11BE_PHY_CAP_MU_BFER_160M BIT(29)
#define DOT11BE_PHY_CAP_MU_BFER_320M BIT(30)
#define DOT11BE_PHY_CAP_TB_SND_FB_RATE_LIMIT BIT(31)


#define MAX_SS_MASK 0xF
#define GET_TX_MAX_SS(_x) ((_x>>4)&MAX_SS_MASK)
#define GET_RX_MAX_SS(_x) (_x&MAX_SS_MASK)
#define SET_TX_MAX_SS(_mcs, _x) (_mcs |= ((_x&MAX_SS_MASK) << 4))
#define SET_RX_MAX_SS(_mcs, _x) (_mcs |= (_x&MAX_SS_MASK))


#define DOT11BE_OP_CHAN_WIDTH (0)

#define EHT_SET_OP_CHAN_WIDTH(_aucInfo, _val) \
	(_aucInfo[DOT11BE_OP_CHAN_WIDTH] = _val)

#define EHT_GET_OP_CHAN_WIDTH(_aucInfo) \
	(_aucInfo[DOT11BE_OP_CHAN_WIDTH])


#endif /* CFG_SUPPORT_802_11BE == 1 */
#endif /* !_EHT_IE_H */
