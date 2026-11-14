/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "tdls.h"
 *    \brief This file contains the internal used in TDLS modules
 *	 for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */


#ifndef _TDLS_AUTO_H
#define _TDLS_AUTO_H

#if CFG_SUPPORT_TDLS

#if CFG_SUPPORT_TDLS_AUTO

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
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

#define SAMPLING_UT (2 * HZ)
#define TDLS_SETUP_TIMEOUT (4)
#define TDLS_MONITOR_UT (2)
	/* TDLS setup threshold if TX throughput > the THD */
#define TDLS_SETUP_THD (10000)
#define TDLS_SETUP_LOW_THD (90)
#define TDLS_SETUP_COUNT (2)
#define TDLS_TEARDOWN_THD (500)
#define TDLS_TEARDOWN_LOW_THD (40)
#define TDLS_TEARDOWN_RX_THD (100)
#define TDLS_LINK_ENABLED(prSta) (prSta->eTdlsStatus == STA_TDLS_LINK_ENABLE)

#define STA_TDLS_HASH_SIZE (3)
#define STA_TDLS_HASH(prSta) (prSta[5] % STA_TDLS_HASH_SIZE)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum STA_TDLS_STATUS {
	STA_TDLS_NOT_SETUP,
	STA_TDLS_SETUP_INPROCESS,
	STA_TDLS_SETUP_COMPLETE,
	STA_TDLS_SETUP_TEARDOWN,
	STA_TDLS_LINK_ENABLE,
	STA_TDLS_LINK_DOWN,
	STA_TDLS_LINK_DISABLE,
	STA_TDLS_SETUP_NUM
};

enum STA_TDLS_ROLE {
	STA_TDLS_ROLE_IDLE,
	STA_TDLS_ROLE_RESPONDER,
	STA_TDLS_ROLE_INITOR,
	STA_TDLS_ROLE_NUM
};

enum STA_TDLS_OP {
	STA_TDLS_OP_FREE,
	STA_TDLS_OP_RESET,
	STA_TDLS_OP_GET_MAX_TP,
	STA_TDLS_OP_UPDATE_TX,
	STA_TDLS_OP_NUM
};

enum TDLS_AUTO_CONFIG {
	TDLS_AUTO_NONE = 0,
	TDLS_AUTO_P2P = 1,
	TDLS_AUTO_ALL = 2,
	TDLS_AUTO_TESTMODE = 99,
};

struct sta_tdls_info {
	struct sta_tdls_info *pNext; /* next entry in sta list */
	uint8_t aucAddr[TDLS_FME_MAC_ADDR_LEN];
	uint32_t u4Throughput;
	uint32_t u4RxThroughput;
	uint32_t u4SetupFailCount;
	uint64_t ulTxBytes;
	uint64_t ulRxBytes;
	enum STA_TDLS_STATUS eTdlsStatus;
	enum STA_TDLS_ROLE eTdlsRole;
};

struct MSG_AUTO_TDLS_INFO {
	struct MSG_HDR rMsgHdr; /* Must be the first member */
	uint8_t ucBssIndex;
	uint16_t u2FrameLength;
	uint8_t aucEthDestAddr[MAC_ADDR_LEN];
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
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void TdlsProcessPublicAction(
	struct ADAPTER *ad,
	struct SW_RFB *prSwRfb);
uint32_t TdlsAutoSetup(
	struct ADAPTER *ad,
	uint8_t bss,
	struct sta_tdls_info *sta);
uint32_t TdlsAutoTeardown(
	struct ADAPTER *ad,
	uint8_t bss,
	struct sta_tdls_info *sta,
	uint8_t *reason);
void TdlsUpdateTxRxStat(
	struct ADAPTER *pAd,
	uint8_t bss,
	uint64_t tx_bytes,
	uint64_t rx_bytes,
	uint8_t *prAddr);
void TdlsAuto(
	struct ADAPTER *pAd,
	struct MSG_HDR *prMsgHdr);
int32_t TdlsAutoImpl(
	struct ADAPTER *pAd,
	uint8_t bss,
	uint64_t tx_bytes,
	uint64_t rx_bytes,
	uint8_t *prAddr);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* CFG_SUPPORT_TDLS_AUTO */

#endif /* CFG_SUPPORT_TDLS */

#endif /* _TDLS_AUTO_H */

