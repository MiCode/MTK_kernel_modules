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
/*
 * Id: include/mgmt/rsn.h#1
 */

/*! \file   rsn.h
 *  \brief  The wpa/rsn related define, macro and structure are described here.
 */

#ifndef _RTT_H
#define _RTT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define RTT_REQUEST_DONE_TIMEOUT_SEC 4

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* Ranging status */
enum ENUM_RTT_STATUS {
	RTT_STATUS_SUCCESS       = 0,
	/* general failure status */
	RTT_STATUS_FAILURE       = 1,
	/* target STA does not respond to request */
	RTT_STATUS_FAIL_NO_RSP   = 2,
	/* request rejected. Applies to 2-sided RTT only */
	RTT_STATUS_FAIL_REJECTED = 3,
	RTT_STATUS_FAIL_NOT_SCHEDULED_YET  = 4,
	/* timing measurement times out */
	RTT_STATUS_FAIL_TM_TIMEOUT         = 5,
	/* Target on different channel, cannot range */
	RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL = 6,
	/* ranging not supported */
	RTT_STATUS_FAIL_NO_CAPABILITY  = 7,
	/* request aborted for unknown reason */
	RTT_STATUS_ABORTED             = 8,
	/* Invalid T1-T4 timestamp */
	RTT_STATUS_FAIL_INVALID_TS     = 9,
	/* 11mc protocol failed */
	RTT_STATUS_FAIL_PROTOCOL       = 10,
	/* request could not be scheduled */
	RTT_STATUS_FAIL_SCHEDULE       = 11,
	/* responder cannot collaborate at time of request */
	RTT_STATUS_FAIL_BUSY_TRY_LATER = 12,
	/* bad request args */
	RTT_STATUS_INVALID_REQ         = 13,
	/* WiFi not enabled */
	RTT_STATUS_NO_WIFI             = 14,
	/* Responder overrides param info, cannot range with new params */
	RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE = 15,
	/* Negotiation failure */
	RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE = 16,
	/* concurrency not supported (NDP+RTT) */
	RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED = 17,
};

/* RTT peer type */
enum ENUM_RTT_PEER_TYPE {
	RTT_PEER_AP         = 0x1,
	RTT_PEER_STA        = 0x2,
	RTT_PEER_P2P_GO     = 0x3,
	RTT_PEER_P2P_CLIENT = 0x4,
	RTT_PEER_NAN        = 0x5
};

/* RTT Measurement Bandwidth */
enum ENUM_WIFI_RTT_BW {
	WIFI_RTT_BW_5   = 0x01,
	WIFI_RTT_BW_10  = 0x02,
	WIFI_RTT_BW_20  = 0x04,
	WIFI_RTT_BW_40  = 0x08,
	WIFI_RTT_BW_80  = 0x10,
	WIFI_RTT_BW_160 = 0x20
};

/* RTT Measurement Preamble */
enum ENUM_WIFI_RTT_PREAMBLE {
	WIFI_RTT_PREAMBLE_LEGACY = 0x1,
	WIFI_RTT_PREAMBLE_HT     = 0x2,
	WIFI_RTT_PREAMBLE_VHT    = 0x4
};

/* RTT Type */
enum ENUM_RTT_TYPE {
	RTT_TYPE_1_SIDED = 0x1,
	RTT_TYPE_2_SIDED = 0x2,
};

struct PARAM_RTT_REQUEST {
	uint8_t fgEnable;
	uint8_t ucConfigNum;
	struct RTT_CONFIG arRttConfigs[CFG_RTT_MAX_CANDIDATES];
};


struct RTT_RESULT_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	struct RTT_RESULT rResult;
	uint16_t u2IELen;
	/* Keep it last */
	uint8_t aucIE[0];
};

struct RTT_INFO {
	uint8_t fgIsRunning;
	uint8_t ucSeqNum;
	struct LINK rResultList;
	struct TIMER rRttDoneTimer;
};

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
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void rttInit(struct ADAPTER *prAdapter);

void rttUninit(struct ADAPTER *prAdapter);

uint32_t rttHandleRttRequest(struct ADAPTER *prAdapter,
	struct PARAM_RTT_REQUEST *prRequest,
	uint8_t ucBssIndex);

void rttEventDone(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_DONE *prEvent);

void rttEventResult(struct ADAPTER *prAdapter,
		      struct EVENT_RTT_RESULT *prEvent);

#endif /* _RTT_H */
