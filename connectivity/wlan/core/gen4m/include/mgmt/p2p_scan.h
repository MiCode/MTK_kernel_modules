/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "scan.h"
 *  \brief
 *
 */

#ifndef _P2P_SCAN_H
#define _P2P_SCAN_H

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

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */
#define P2P_MLD_SCAN_DEFAULT_DWELL_TIME			100
#define P2P_MLD_SCAN_DEFAULT_MIN_DWELL_TIME		42

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
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

void scnEventReturnChannel(struct ADAPTER *prAdapter,
		uint8_t ucScnSeqNum);

u_int8_t scanUpdateP2pDeviceDesc(struct ADAPTER *prAdapter,
		struct BSS_DESC *prBssDesc);

void
scanP2pProcessBeaconAndProbeResp(struct ADAPTER *prAdapter,
		struct SW_RFB *prSwRfb,
		uint32_t *prStatus,
		struct BSS_DESC *prBssDesc,
		struct WLAN_BEACON_FRAME *prWlanBeaconFrame);

struct BSS_DESC *scanP2pSearchDesc(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct P2P_CONNECTION_REQ_INFO *prConnReqInfo,
		struct P2P_JOIN_INFO *prJoinInfo);

#endif
