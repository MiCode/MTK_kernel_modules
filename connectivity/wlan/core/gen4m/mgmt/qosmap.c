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

	prRxFrame = (struct WLAN_ACTION_FRAME *) prSwRfb->pvHeader;

	switch (prRxFrame->ucAction) {
	case ACTION_ADDTS_REQ:
	case ACTION_ADDTS_RSP:
	case ACTION_SCHEDULE:
		DBGLOG(INIT, INFO, "qos action frame received, action: %d\n",
			prRxFrame->ucAction);
		break;
	case ACTION_QOS_MAP_CONFIGURE:
		qosHandleQosMapConfigure(prAdapter, prSwRfb);
		DBGLOG(INIT, INFO,
			"qos map configure frame received, action: %d\n",
			prRxFrame->ucAction);
		break;
	default:
		DBGLOG(INIT, INFO,
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

	DBGLOG(INIT, INFO,
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

void qosParseQosMapSet(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t *qosMapSet)
{
	uint8_t dscpExcNum = 0;
	int i = 0;
	uint8_t *tempq = qosMapSet + 2;
	uint8_t *qosmapping = prStaRec->qosMapSet;
	uint8_t excTable[64];

	if (IE_ID(qosMapSet) != ELEM_ID_QOS_MAP_SET) {
		DBGLOG(INIT, WARN,
			"Wrong QosMapSet IE ID: %d\n", IE_ID(qosMapSet));
		return;
	}
	if ((IE_LEN(qosMapSet) < 16) || (IE_LEN(qosMapSet) > 58)) {
		DBGLOG(INIT, WARN,
			"Error in QosMapSet IE len: %d\n", IE_LEN(qosMapSet));
		return;
	}

	qosMapSetInit(prStaRec);
	kalMemSet(excTable, 0, 64);

	dscpExcNum = (IE_LEN(qosMapSet) - WMM_UP_INDEX_NUM * 2) / 2;
	for (i = 0; i < dscpExcNum; i++) {
		uint8_t dscp = *tempq++;
		uint8_t up = *tempq++;

		if (dscp < 64 && up < WMM_UP_INDEX_NUM) {
			qosmapping[dscp] = up;
			excTable[dscp] = TRUE;
		}
	}

	for (i = 0; i < WMM_UP_INDEX_NUM; i++) {
		uint8_t lDscp = *tempq++;
		uint8_t hDscp = *tempq++;
		uint8_t dscp;

		if (lDscp == 255 && hDscp == 255) {
			log_dbg(INIT, WARN, "UP %d is not used\n", i);
			continue;
		}

		if (hDscp < lDscp) {
			log_dbg(INIT, WARN, "CHECK: UP %d, h %d, l %d\n",
				i, hDscp, lDscp);
			continue;
		}

		for (dscp = lDscp; dscp < 64 && dscp <= hDscp; dscp++) {
			if (!excTable[dscp])
				qosmapping[dscp] = i;
		}
	}

	DBGLOG(INIT, INFO, "QosMapSet DSCP Exception number: %d\n", dscpExcNum);
}

void qosMapSetInit(struct STA_RECORD *prStaRec)
{
	/**
	 * RFC 8325:
	 * All unused codepoints are RECOMMENDED to be mapped to UP 0.
	 */
	static uint8_t dscp2up[64] = {
		[0 ... 63] = 0,         /* PHB, UP, WMM, AC */
		[0] = WMM_UP_BE_INDEX,  /* DF,   0, BE, AC_BE */
		[8] = WMM_UP_BK_INDEX,  /* CS1,  1, BK, AC_BK */
		[10] = WMM_UP_BE_INDEX, /* AF11, 0, BE, AC_BE */
		[12] = WMM_UP_BE_INDEX, /* AF12, 0, BE, AC_BE */
		[14] = WMM_UP_BE_INDEX, /* AF13, 0, BE, AC_BE */
		[16] = WMM_UP_BE_INDEX, /* CS2,  0, BE, AC_BE */
		[18] = WMM_UP_EE_INDEX, /* AF21, 3, EE, AC_BE */
		[20] = WMM_UP_EE_INDEX, /* AF22, 3, EE, AC_BE */
		[22] = WMM_UP_EE_INDEX, /* AF23, 3, EE, AC_BE */
		[24] = WMM_UP_CL_INDEX, /* CS3,  4, CL, AC_VI */
		[26] = WMM_UP_CL_INDEX, /* AF31, 4, CL, AC_VI */
		[28] = WMM_UP_CL_INDEX, /* AF32, 4, CL, AC_VI */
		[30] = WMM_UP_CL_INDEX, /* AF33, 4, CL, AC_VI */
		[32] = WMM_UP_CL_INDEX, /* CS4,  4, CL, AC_VI */
		[34] = WMM_UP_CL_INDEX, /* AF41, 4, CL, AC_VI */
		[36] = WMM_UP_CL_INDEX, /* AF42, 4, CL, AC_VI */
		[38] = WMM_UP_CL_INDEX, /* AF43, 4, CL, AC_VI */
		[40] = WMM_UP_VI_INDEX, /* CS5,  5, VI, AC_VI */
		[44] = WMM_UP_VO_INDEX, /* VA,   6, VO, AC_VO */
		[46] = WMM_UP_VO_INDEX, /* EF,   6, VO, AC_VO */
		/**
		 * RFC 8325 8.2 Security Recommendations for WLAN QoS:
		 * it is RECOMMENDED that CS6 and CS7 DSCP be mapped to UP 0 in
		 * these Wi-Fi-at-the-edge deployment models.
		 * Keep this to make 0xd0 map to VO to compatilbe with WMM test.
		 */
		[48] = WMM_UP_VO_INDEX, /* CS6,  6, VO, AC_VO */
		[56] = WMM_UP_NC_INDEX, /* CS7,  7, NC, AC_VO */
	};

	kalMemCopy(prStaRec->qosMapSet, dscp2up, 64);
}

/**
 * Return the User Priority according to DSCP in IP header according to
 * the mapping table.
 *
 * NOTE: TID (4-bit) = 0(MSB) + User Priority (3-bit)
 */
uint8_t getUpFromDscp(struct GLUE_INFO *prGlueInfo,
		uint8_t ucBssIndex, int dscp)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
	if (prBssInfo)
		prStaRec = prBssInfo->prStaRecOfAP;
	else
		return 0xFF;

	if (prStaRec && dscp >= 0 && dscp < 64)
		return prStaRec->qosMapSet[dscp];

	return 0xFF;
}
#endif
