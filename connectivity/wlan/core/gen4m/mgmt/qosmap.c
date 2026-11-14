// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "qosmap.c"
 *    \brief  This file including the qosmap related function.
 *
 *    This file provided the macros and functions library support for the
 *    protocol layer qosmap related function.
 *
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#if CFG_SUPPORT_PPR2

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/**
 * RFC 8325:
 * All unused codepoints are RECOMMENDED to be mapped to UP 0.
 *
 * DSCP PHB, UP, WMM, AC
 * 0    DF,   0, BE, AC_BE
 * 8    CS1,  1, BK, AC_BK
 * 10   AF11, 0, BE, AC_BE
 * 12   AF12, 0, BE, AC_BE
 * 14   AF13, 0, BE, AC_BE
 * 16   CS2,  0, BE, AC_BE
 * 18   AF21, 3, EE, AC_BE
 * 20   AF22, 3, EE, AC_BE
 * 22   AF23, 3, EE, AC_BE
 * 24   CS3,  4, CL, AC_VI
 * 26   AF31, 4, CL, AC_VI
 * 28   AF32, 4, CL, AC_VI
 * 30   AF33, 4, CL, AC_VI
 * 32   CS4,  4, CL, AC_VI
 * 34   AF41, 4, CL, AC_VI
 * 36   AF42, 4, CL, AC_VI
 * 38   AF43, 4, CL, AC_VI
 * 40   CS5,  5, VI, AC_VI
 * 44   VA,   6, VO, AC_VO
 * 46   EF,   6, VO, AC_VO
 * 48   CS6,  7, NC, AC_VO (NOTE)
 * 56   CS7,  7, NC, AC_VO (NOTE)
 *
 * NOTE:
 * RFC 8325 8.2 Security Recommendations for WLAN QoS:
 * it is RECOMMENDED that CS6 and CS7 DSCP be mapped to UP 0 in
 * these Wi-Fi-at-the-edge deployment models.
 *
 */

#if QOS_MAP_LEGACY_DSCP_TABLE
/* a legacy QoS Map for DSCP to TID */
static const uint8_t dscp2up[64] = {
	[0] = WMM_UP_BE_INDEX,
	[8] = WMM_UP_BK_INDEX,
	[10] = WMM_UP_BE_INDEX,
	[12] = WMM_UP_BE_INDEX,
	[14] = WMM_UP_BE_INDEX,
	[16] = WMM_UP_BE_INDEX,
	[18] = WMM_UP_EE_INDEX,
	[20] = WMM_UP_EE_INDEX,
	[22] = WMM_UP_EE_INDEX,
	[24] = WMM_UP_CL_INDEX,
	[26] = WMM_UP_CL_INDEX,
	[28] = WMM_UP_CL_INDEX,
	[30] = WMM_UP_CL_INDEX,
	[32] = WMM_UP_CL_INDEX,
	[34] = WMM_UP_CL_INDEX,
	[36] = WMM_UP_CL_INDEX,
	[38] = WMM_UP_CL_INDEX,
	[40] = WMM_UP_VI_INDEX,
	[44] = WMM_UP_VO_INDEX,
	[46] = WMM_UP_VO_INDEX,
#if !CFG_WIFI_AT_THE_EDGE_QOS
	[48] = WMM_UP_NC_INDEX,
	[56] = WMM_UP_NC_INDEX,
#endif
};
#endif

/* a structure for cfg80211_classify8021d */
static const struct QOS_MAP defaultQosMap = {
#if CFG_WIFI_AT_THE_EDGE_QOS
	15,
#else
	17,
#endif
	{
		{8, 1},
		{18, 3}, {20, 3}, {22, 3}, {24, 4}, {26, 4}, {28, 4},
		{30, 4}, {32, 4},
		{34, 4}, {36, 4}, {38, 4},
		{40, 5},
		{44, 6}, {46, 6},
#if !CFG_WIFI_AT_THE_EDGE_QOS
		/* Extend for backward compatibility traffic generation.
		 * Allow to set 48, 56 to UP 7, intended to now following
		 * RECOMMENDATION in RFC 8325 Sec 8.2.
		 */
		{48, 7}, {56, 7},
#endif
	}, {
		{0, 63},
	}
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */



/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 *
 * \brief This routine is called to process the qos category action frame.
 *
 *
 * \note
 *      Called by: Handle Rx mgmt request
 */
/*----------------------------------------------------------------------------*/
void handleQosMapConf(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{
	struct WLAN_ACTION_FRAME *prRxFrame;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxFrame = (struct WLAN_ACTION_FRAME *)prSwRfb->pvHeader;

	switch (prRxFrame->ucAction) {
	case ACTION_ADDTS_REQ:
	case ACTION_ADDTS_RSP:
	case ACTION_SCHEDULE:
		DBGLOG(INIT, DEBUG, "qos action frame received, action: %d\n",
			prRxFrame->ucAction);
		break;
	case ACTION_QOS_MAP_CONFIGURE:
		qosHandleQosMapConfigure(prAdapter, prSwRfb);
		DBGLOG(INIT, DEBUG,
			"qos map configure frame received, action: %d\n",
			prRxFrame->ucAction);
		break;
	default:
		DBGLOG(INIT, DEBUG,
			"qos action frame: %d, try to send to supplicant\n",
			prRxFrame->ucAction);
		break;
	}
}

int qosHandleQosMapConfigure(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct _ACTION_QOS_MAP_CONFIGURE_FRAME *prRxFrame = NULL;
	struct STA_RECORD *prStaRec;
	uint16_t u2IELength = 0;

	prRxFrame =
		(struct _ACTION_QOS_MAP_CONFIGURE_FRAME *) prSwRfb->pvHeader;
	if (!prRxFrame)
		return -1;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return -1;

	DBGLOG(INIT, DEBUG,
		"IEEE 802.11: Received Qos Map Configure Frame from "
		MACSTR "\n",
		MAC2STR(prStaRec->aucMacAddr));

	u2IELength = (prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen) -
		(uint16_t)
		(OFFSET_OF(struct _ACTION_QOS_MAP_CONFIGURE_FRAME, qosMapSet[0])
			- WLAN_MAC_HEADER_LEN);

	if (u2IELength < ELEM_HDR_LEN ||
		u2IELength < ELEM_HDR_LEN + IE_LEN(prRxFrame->qosMapSet)) {
		DBGLOG(INIT, WARN, "QosMapSet IE: insufficient length %d\n",
			u2IELength);
		return -1;
	}

	qosParseQosMapSet(prAdapter, prStaRec, prRxFrame->qosMapSet);

	return 0;
}

/**
 * Copy received QoS Map request frame to cached structure in struct QOS_MAP.
 */
static void updateCachedQosMap(struct STA_RECORD *prStaRec, uint8_t ucDscpExNum,
		const uint8_t *dscp_exception, const uint8_t *dscp_range)
{
	prStaRec->rQosMap.ucDscpExNum = ucDscpExNum;
	kalMemCopy(&prStaRec->rQosMap.arDscpException, dscp_exception,
			dscp_range - dscp_exception);
	kalMemCopy(&prStaRec->rQosMap.arDscpRange, dscp_range,
			sizeof(prStaRec->rQosMap.arDscpRange));
}

static void qosBuildQosMapTable(struct STA_RECORD *prStaRec,
	const uint8_t *dscp_exception,
	const uint8_t *dscp_range)
{
#if QOS_MAP_LEGACY_DSCP_TABLE
	uint8_t *qosmapping = prStaRec->qosMapTable;
	uint8_t *p;
	uint8_t dscp;
	uint8_t up;
	uint8_t lDscp;
	uint8_t hDscp;

	qosMapSetInit(prStaRec);

	/* DSCP range */
	for (p = dscp_range, up = 0; up < WMM_UP_INDEX_NUM; up++) {
		lDscp = *p++;
		hDscp = *p++;

		if (lDscp == 255 && hDscp == 255) {
			DBGLOG(INIT, DEBUG, "UP %d is not specified\n", up);
			continue;
		}

		if (hDscp < lDscp) {
			DBGLOG(INIT, WARN, "CHECK: UP %d, l %d, h %d\n",
				up, lDscp, hDscp);
			continue;
		}

		for (dscp = lDscp; dscp < 64 && dscp <= hDscp; dscp++)
			qosmapping[dscp] = up;
	}

	/* DSCP exception, overwrite the table to be used first */
	for (p = dscp_exception; p < dscp_range; ) {
		dscp = *p++;
		up = *p++;

		if (dscp < 64 && up < WMM_UP_INDEX_NUM)
			qosmapping[dscp] = up;
	}
#endif
}

void qosParseQosMapSet(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
			const uint8_t *qosMapSet)
{
	const uint8_t *dscp_exception = qosMapSet + 2;
	const uint8_t *dscp_range = qosMapSet + 2 + IE_LEN(qosMapSet)
				- WMM_UP_INDEX_NUM * 2;
	uint8_t ie_len;

	if (IE_ID(qosMapSet) != ELEM_ID_QOS_MAP_SET) {
		DBGLOG(INIT, WARN,
			"Wrong QosMapSet IE ID: %d\n", IE_ID(qosMapSet));
		return;
	}

	ie_len = IE_LEN(qosMapSet);
	if (ie_len < 16 || ie_len > 58 || ie_len % 2 != 0) {
		DBGLOG(INIT, WARN,
			"Error in QosMapSet IE len: %d\n", IE_LEN(qosMapSet));
		return;
	}

	qosBuildQosMapTable(prStaRec, dscp_exception, dscp_range);

	/* Copy for struct QOS_MAP */
	updateCachedQosMap(prStaRec, (dscp_range - dscp_exception) / 2,
			dscp_exception, dscp_range);
	DBGLOG(INIT, DEBUG, "QosMapSet DSCP Exception number: %td\n",
			(dscp_range - dscp_exception) / 2);
}

/* Prepare a DSCP-to-UP table lookup in driver to be queried from kernel
 * protocol thread context on calling ndo_select_queue() to mark user priority
 * according to DSCP field in IP header.
 */
void qosMapSetInit(struct STA_RECORD *prStaRec)
{
#if QOS_MAP_LEGACY_DSCP_TABLE
	kalMemCopy(prStaRec->qosMapTable, dscp2up, 64);
#endif

	/* Copy for struct QOS_MAP, used by cfg80211_classify8021d() */
	kalMemCopy(&prStaRec->rQosMap, &defaultQosMap, sizeof(struct QOS_MAP));
}
#endif
