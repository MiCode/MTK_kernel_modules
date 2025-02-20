/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_TXM_H_
#define _NAN_TXM_H_

#if CFG_SUPPORT_NAN

#if 0
/* The macro to check if the MAC address is B/MCAST Address */
#define IS_BMCAST_MAC_ADDR(_pucDestAddr)
	((BOOLEAN) (((PUINT_8)(_pucDestAddr))[0] & BIT(0)))
#endif

#define TXM_UT_CONTENT_LEN 20

__KAL_ATTRIB_PACKED_FRONT__
struct _WLAN_TX_UT_FRAME_T {
	/* TX UT MAC header */
	uint16_t u2FrameCtrl;		   /* Frame Control */
	uint16_t u2DurationID;		   /* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];  /* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];   /* SA */
	uint8_t aucClusterId[MAC_ADDR_LEN]; /* Cluster Id */
	uint16_t u2SeqCtrl;		   /* Sequence Control */
	/* TX UT frame body */
	uint8_t ucCategory;
	uint8_t ucAction;
	uint8_t aucOUI[VENDOR_OUI_LEN];
	uint8_t ucOUItype;
	uint8_t ucOUISubtype;
	uint8_t aucTxmUtContent[TXM_UT_CONTENT_LEN];
} __KAL_ATTRIB_PACKED__;

uint32_t nanTxUtTxDone(struct ADAPTER *prAdapter,
		       struct MSDU_INFO *prMsduInfo,
		       enum ENUM_TX_RESULT_CODE rTxDoneStatus);

#endif
#endif /*_NAN_TXM_H_ */
