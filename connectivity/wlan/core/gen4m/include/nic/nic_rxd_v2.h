/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   nic_tx.h
 *    \brief  Functions that provide TX operation in NIC's point of view.
 *
 *    This file provides TX functions which are responsible for both Hardware
 *    and Software Resource Management and keep their Synchronization.
 *
 */


#ifndef _NIC_RXD_V2_H
#define _NIC_RXD_V2_H

#if (CFG_SUPPORT_CONNAC2X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

struct HW_MAC_RX_STS_GROUP_1 {
	uint8_t aucPN[16];
};

struct HW_MAC_RX_STS_GROUP_2 {
	uint32_t u4Timestamp;	/* DW 12 */
	uint32_t u4CRC;		/* DW 13 */
};

struct HW_MAC_RX_STS_GROUP_4 {
	/* For HDR_TRAN */
	uint16_t u2FrameCtl;	/* DW 4 */
	uint8_t aucTA[6];	/* DW 4~5 */
	uint16_t u2SeqFrag;	/* DW 6 */
	uint16_t u2Qos;		/* DW 6 */
	uint32_t u4HTC;		/* DW 7 */
};

struct HW_MAC_RX_STS_GROUP_3 {
	/*!  RX Vector Info */
	uint32_t u4RxVector[6];	/* DW 14~19 */
};

struct HW_MAC_RX_STS_GROUP_3_V2 {
	/*  PRXVector Info */
	uint32_t u4RxVector[2];	/* FALCON: DW 16~17 */
};

struct HW_MAC_RX_STS_GROUP_5 {
	/*  CRXVector Info */
	/* FALCON: DW 18~33 for harrier E1,  DW 18~35 for harrier E2
	 * Other project: give group5_size in chip info,
	 * e.g Soc3_0.c
,
	 * or modify prChipInfo->group5_size when doing wlanCheckAsicCap,
	 * e.g. Harrier E1
	 */
	uint32_t u4RxVector[18];
};

struct HW_MAC_RX_STS_HARRIER_E1_GROUP_5 {
	/*  CRXVector Info */
	/* FALCON: DW 18~33 for harrier E1,  DW 18~35 for harrier E2 */
	uint32_t u4RxVector[16];
};

uint16_t nic_rxd_v2_get_rx_byte_count(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_packet_type(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_wlan_idx(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_sec_mode(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_sw_class_error_bit(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_ch_num(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_rf_band(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_tcl(
	void *prRxStatus);
uint8_t nic_rxd_v2_get_ofld(
	void *prRxStatus);
void nic_rxd_v2_fill_rfb(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
u_int8_t nic_rxd_v2_sanity_check(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

uint8_t nic_rxd_v2_get_HdrTrans(void *prRxStatus);

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
void nic_rxd_v2_check_wakeup_reason(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
#endif /* CFG_SUPPORT_WAKEUP_REASON_DEBUG */

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
uint8_t nic_rxd_v2_fill_radiotap(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
#endif

#endif /* CFG_SUPPORT_CONNAC2X == 1 */
#endif /* _NIC_RXD_V2_H */
