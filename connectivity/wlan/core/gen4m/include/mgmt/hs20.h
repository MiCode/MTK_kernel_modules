/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hs20.h
 *  \brief This file contains the function declaration for hs20.c.
 */

#ifndef _HS20_H
#define _HS20_H

#if CFG_SUPPORT_PASSPOINT
/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */
#define BSSID_POOL_MAX_SIZE             8
#define HS20_SIGMA_SCAN_RESULT_TIMEOUT  30	/* sec */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/* Entry of BSSID Pool - For SIGMA Test */
struct BSSID_ENTRY {
	uint8_t aucBSSID[MAC_ADDR_LEN];
};

struct HS20_INFO {
	/*Hotspot 2.0 Information */
	uint8_t aucHESSID[MAC_ADDR_LEN];
	uint8_t ucAccessNetworkOptions;
	uint8_t ucVenueGroup;	/* VenueInfo - Group */
	uint8_t ucVenueType;
	uint8_t ucHotspotConfig;

	/*Roaming Consortium Information */
	/* PARAM_HS20_ROAMING_CONSORTIUM_INFO rRCInfo; */

	/*Hotspot 2.0 dummy AP Info */

	/*Time Advertisement Information */
	/* UINT_32                 u4UTCOffsetTime; */
	/* UINT_8                  aucTimeZone[ELEM_MAX_LEN_TIME_ZONE]; */
	/* UINT_8                  ucLenTimeZone; */

	/* For SIGMA Test */
	/* BSSID Pool */
	struct BSSID_ENTRY arBssidPool[BSSID_POOL_MAX_SIZE];
	uint8_t ucNumBssidPoolEntry;
	u_int8_t fgIsHS2SigmaMode;

	u_int8_t fgConnectHS20AP;

};

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

/*For GTK Frame Filter*/
#if DBG
#define FREE_IPV4_NETWORK_ADDR_LIST(_prAddrList)    \
	{   \
		uint32_t u4Size =  \
				 OFFSET_OF(struct IPV4_NETWORK_ADDRESS_LIST,  \
				 arNetAddr) +  \
				 (((_prAddrList)->ucAddrCount) *  \
				 sizeof(struct CMD_IPV4_NETWORK_ADDRESS));  \
		kalMemFree((_prAddrList), VIR_MEM_TYPE, u4Size);    \
		(_prAddrList) = NULL;   \
	}
#else
#define FREE_IPV4_NETWORK_ADDR_LIST(_prAddrList)    \
	{   \
		kalMemFree((_prAddrList), VIR_MEM_TYPE, 0);    \
		(_prAddrList) = NULL;   \
	}
#endif

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

void hs20FillExtCapIE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct MSDU_INFO *prMsduInfo);

void hs20FillProreqExtCapIE(struct ADAPTER *prAdapter, uint8_t *pucIE);

void hs20FillHS20IE(struct ADAPTER *prAdapter, uint8_t *pucIE);

uint32_t hs20CalculateHS20RelatedIEForProbeReq(struct ADAPTER *prAdapter,
		uint8_t *pucTargetBSSID);

uint32_t hs20GenerateHS20RelatedIEForProbeReq(struct ADAPTER *prAdapter,
		uint8_t *pucTargetBSSID, uint8_t *prIE);

u_int8_t hs20IsGratuitousArp(struct ADAPTER *prAdapter,
		struct SW_RFB *prCurrSwRfb);

u_int8_t hs20IsUnsolicitedNeighborAdv(struct ADAPTER *prAdapter,
		struct SW_RFB *prCurrSwRfb);

u_int8_t hs20IsUnsecuredFrame(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, struct SW_RFB *prCurrSwRfb);

u_int8_t hs20IsFrameFilterEnabled(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

#endif /* CFG_SUPPORT_PASSPOINT */
#endif
