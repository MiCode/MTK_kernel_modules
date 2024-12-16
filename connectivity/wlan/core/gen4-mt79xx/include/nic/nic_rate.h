/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/nic/nic_rate.h#1
 */

/*! \file  nic_rate.h
 *    \brief This file contains the rate utility function of
 *	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
 */


#ifndef _NIC_RATE_H
#define _NIC_RATE_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
extern char *HW_TX_MODE_STR[];
extern char *HW_TX_RATE_CCK_STR[];
extern char *HW_TX_RATE_OFDM_STR[];
extern char *HW_TX_RATE_BW[];

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define BW_20		0
#define BW_40		1
#define BW_80		2
#define BW_160		3
#define BW_10		4
#define BW_5		6
#define BW_8080		7
#define BW_ALL		0xFF

#define TX_RATE_MODE_CCK	0
#define TX_RATE_MODE_OFDM	1
#define TX_RATE_MODE_HTMIX	2
#define TX_RATE_MODE_HTGF	3
#define TX_RATE_MODE_VHT	4
#define TX_RATE_MODE_PLR	5
#define TX_RATE_MODE_HE_SU      8
#define TX_RATE_MODE_HE_ER      9
#define TX_RATE_MODE_HE_TRIG    10
#define TX_RATE_MODE_HE_MU      11
#define TX_RATE_MODE_EHT        12
#define TX_RATE_MODE_EHT_ER     13
#define TX_RATE_MODE_EHT_TRIG   14
#define TX_RATE_MODE_EHT_MU     15

#define RATE_VER_1	0	/* AC */
#define RATE_VER_2	1	/* HE */

/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
enum ENUM_TX_MODE_STR_IDX {
	ENUM_TX_MODE_CCK = 0,
	ENUM_TX_MODE_OFDM,
	ENUM_TX_MODE_MM,
	ENUM_TX_MODE_GF,
	ENUM_TX_MODE_VHT,
#if (CFG_SUPPORT_802_11AX == 1)
	ENUM_TX_MODE_PLR,
	ENUM_TX_MODE_HE_SU = 8,
	ENUM_TX_MODE_HE_ER,
	ENUM_TX_MODE_HE_TRIG,
	ENUM_TX_MODE_HE_MU,
#endif
	ENUM_TX_MODE_NUM
};

struct FIXED_RATE_INFO {
	uint32_t u4Mode;
	uint32_t u4Bw;
	uint32_t u4Mcs;
	uint32_t u4VhtNss;
	uint32_t u4SGI;
	uint32_t u4Preamble;
	uint32_t u4STBC;
	uint32_t u4LDPC;
	uint32_t u4SpeEn;
	uint32_t u4HeLTF;
	uint32_t u4HeErDCM;
	uint32_t u4HeEr106t;
};

enum GI_HE {
	GI_HE_SGI = 0,
	GI_HE_MGI,
	GI_HE_LGI,
	GI_HE_NUM
};

enum HE_LTF {
	HE_LTF_1X = 0,
	HE_LTF_2X,
	HE_LTF_4X
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if (CFG_SUPPORT_CONNAC3X == 1)
#define HW_TX_RATE_TO_MODE(_x)		CONNAC3X_HW_TX_RATE_TO_MODE(_x)
#define HW_TX_RATE_TO_NSS(_x)		CONNAC3X_HW_TX_RATE_TO_NSS(_x)
#define HW_TX_RATE_TO_STBC(_x)		CONNAC3X_HW_TX_RATE_TO_STBC(_x)
#define HW_TX_RATE_TO_MCS(_x)		CONNAC3X_HW_TX_RATE_TO_MCS(_x)
#elif (CFG_SUPPORT_CONNAC2X == 1)
#define HW_TX_RATE_TO_MODE(_x)		CONNAC2X_HW_TX_RATE_TO_MODE(_x)
#define HW_TX_RATE_TO_NSS(_x)		CONNAC2X_HW_TX_RATE_TO_NSS(_x)
#define HW_TX_RATE_TO_STBC(_x)		CONNAC2X_HW_TX_RATE_TO_STBC(_x)
#define HW_TX_RATE_TO_DCM(_x)		CONNAC2X_HW_TX_RATE_TO_DCM(_x)
#define HW_TX_RATE_TO_106T(_x)		CONNAC2X_HW_TX_RATE_TO_106T(_x)
#define HW_TX_RATE_TO_MCS(_x)		((_x) & (0x3f))
#else
#define HW_TX_RATE_TO_MODE(_x)		(((_x) & (0x7 << 6)) >> 6)
#define HW_TX_RATE_TO_NSS(_x)		(((_x) & (0x3 << 9)) >> 9)
#define HW_TX_RATE_TO_STBC(_x)		(((_x) & (0x1 << 11)) >> 11)
#define HW_TX_RATE_TO_MCS(_x)		((_x) & (0x3f))
#endif

#if (CFG_SUPPORT_CONNAC2X == 1)
#define TX_VECTOR_GET_TX_RATE(_txv)	CONNAC2X_TXV_GET_TX_RATE(_txv)
#define TX_VECTOR_GET_TX_LDPC(_txv)	CONNAC2X_TXV_GET_TX_LDPC(_txv)
#define TX_VECTOR_GET_TX_STBC(_txv)	CONNAC2X_TXV_GET_TX_STBC(_txv)
#define TX_VECTOR_GET_TX_FRMODE(_txv)	CONNAC2X_TXV_GET_TX_FRMODE(_txv)
#define TX_VECTOR_GET_TX_MODE(_txv)	CONNAC2X_TXV_GET_TX_MODE(_txv)
#define TX_VECTOR_GET_TX_NSTS(_txv)	CONNAC2X_TXV_GET_TX_NSTS(_txv)
#define TX_VECTOR_GET_TX_PWR(_txv)	CONNAC2X_TXV_GET_TX_PWR(_txv)
#define TX_VECTOR_GET_TX_SGI(_txv)	CONNAC2X_TXV_GET_TX_SGI(_txv)
#define TX_VECTOR_GET_TX_DCM(_txv)	CONNAC2X_TXV_GET_TX_DCM(_txv)
#define TX_VECTOR_GET_TX_106T(_txv)	CONNAC2X_TXV_GET_TX_106T(_txv)
#else
#define TX_VECTOR_GET_TX_RATE(_txv)     (((_txv)->u4TxV[0]) & BITS(0, 6))
#define TX_VECTOR_GET_TX_LDPC(_txv)     ((((_txv)->u4TxV[0]) >> 7) & BIT(0))
#define TX_VECTOR_GET_TX_STBC(_txv)     ((((_txv)->u4TxV[0]) >> 8) & \
		BITS(0, 1))
#define TX_VECTOR_GET_TX_FRMODE(_txv)   ((((_txv)->u4TxV[0]) >> 10) & \
		BITS(0, 1))
#define TX_VECTOR_GET_TX_MODE(_txv)     ((((_txv)->u4TxV[0]) >> 12) & \
		BITS(0, 2))
#define TX_VECTOR_GET_TX_NSTS(_txv)     ((((_txv)->u4TxV[0]) >> 21) & \
		BITS(0, 1))
#define TX_VECTOR_GET_TX_PWR(_txv)      ((((_txv)->u4TxV[0]) >> 24) & \
		BITS(0, 6))
#define TX_VECTOR_GET_TX_SGI(_txv)      ((((_txv)->u4TxV[2]) >> 27) & BIT(0))
#endif

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint32_t
nicGetPhyRateByMcsRate(
	IN uint8_t ucIdx,
	IN uint8_t ucBw,
	IN uint8_t ucGI
);

uint32_t
nicGetHwRateByPhyRate(
	IN uint8_t ucIdx
);

uint32_t
nicSwIndex2RateIndex(
	IN uint8_t ucRateSwIndex,
	OUT uint8_t *pucRateIndex,
	OUT uint8_t *pucPreambleOption
);

uint32_t
nicRateIndex2RateCode(
	IN uint8_t ucPreambleOption,
	IN uint8_t ucRateIndex,
	OUT uint16_t *pu2RateCode
);

uint32_t
nicRateCode2PhyRate(
	IN uint16_t u2RateCode,
	IN uint8_t ucBandwidth,
	IN uint8_t ucGI,
	IN uint8_t ucRateNss
);

uint32_t
nicRateCode2DataRate(
	IN uint16_t u2RateCode,
	IN uint8_t ucBandwidth,
	IN uint8_t ucGI
);

u_int8_t
nicGetRateIndexFromRateSetWithLimit(
	IN uint16_t u2RateSet,
	IN uint32_t u4PhyRateLimit,
	IN u_int8_t fgGetLowest,
	OUT uint8_t *pucRateSwIndex
);

char *nicHwRateOfdmStr(
	uint16_t ofdm_idx);

uint32_t nicSetFixedRateData(
	struct FIXED_RATE_INFO *pFixedRate,
	uint32_t *pu4Data);

uint32_t nicRateHeLtfCheckGi(
	struct FIXED_RATE_INFO *pFixedRate);

uint8_t nicGetTxSgiInfo(
	IN struct PARAM_PEER_CAP *prWtblPeerCap,
	IN uint8_t u1TxMode);

uint8_t nicGetTxLdpcInfo(
	IN uint8_t ucTxMode,
	IN struct PARAM_TX_CONFIG *prWtblTxConfig);

int32_t nicGetTxRateInfo(IN char *pcCommand, IN int i4TotalLen,
			u_int8_t fgDumpAll,
			struct PARAM_HW_WLAN_INFO *prHwWlanInfo,
			struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics);

int32_t nicGetRxRateInfo(struct ADAPTER *prAdapter, IN char *pcCommand,
			IN int i4TotalLen, IN uint8_t ucWlanIdx);

uint16_t nicGetStatIdxInfo(IN struct ADAPTER *prAdapter,
			IN uint8_t ucWlanIdx);

uint16_t
nicRateInfo2RateCode(IN uint32_t  u4TxMode,
	IN uint32_t  u4Rate);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _NIC_RATE_H */
