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


#ifndef _NIC_RXD_v5_H
#define _NIC_RXD_v5_H

#if (CFG_SUPPORT_CONNAC5X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

struct HW_MAC_RX_STS_GROUP_1 {
	uint8_t aucPN[16];		/* DW12 - DW15 */
};

struct HW_MAC_RX_STS_GROUP_2 {
	uint32_t u4Timestamp;		/* DW16 */
	uint32_t u4CRC;			/* DW17 */
	uint32_t u4PPFTimestamp;	/* DW18 */
	uint32_t ucPhyInfo;		/* DW19 */
};

struct HW_MAC_RX_STS_GROUP_4 {
	/* For HDR_TRAN */
	uint16_t u2FrameCtl;		/* DW8 */
	uint8_t aucTA[6];		/* DW8 - DW9 */
	uint16_t u2SeqFrag;		/* DW 10 */
	uint16_t u2Qos;			/* DW 10 */
	uint32_t u4HTC;			/* DW 11 */
};

struct HW_MAC_RX_STS_GROUP_3 {
	/*!  RX Vector Info */
	uint32_t u4RxVector[8];		/* DW 20 - DW 27 */
};

struct HW_MAC_RX_STS_GROUP_3_V2 {
	/*  PRXVector Info */
	uint32_t u4RxVector[4];		/* DW20 - DW23 */
	uint32_t u4Rcpi;		/* DW24 */
	uint16_t u2Reserved;		/* DW25 */
	uint16_t u2RxInfo;		/* DW25 */
	uint32_t u4Reserved2[2];	/* DW26 - DW 27 */
};

struct HW_MAC_RX_STS_GROUP_5 {		/* DW28 - */
	uint32_t u4RxVector[44];
};

struct tx_free_done_rpt {
	uint32_t dw0;
	uint32_t dw1;
	void *arIdSets[];
};

uint16_t nic_rxd_v5_get_rx_byte_count(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_packet_type(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_wlan_idx(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_sec_mode(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_sw_class_error_bit(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_ch_num(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_rf_band(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_tcl(
	void *prRxStatus);
uint8_t nic_rxd_v5_get_ofld(
	void *prRxStatus);
void nic_rxd_v5_fill_rfb(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
u_int8_t nic_rxd_v5_sanity_check(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

uint8_t nic_rxd_v5_get_HdrTrans(void *prRxStatus);

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
void nic_rxd_v5_check_wakeup_reason(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
#endif /* CFG_SUPPORT_WAKEUP_REASON_DEBUG */
void nic_rxd_v5_handle_host_rpt(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	struct QUE *prFreeQueue);

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
uint8_t nic_rxd_v5_fill_radiotap(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);
#endif

#endif /* CFG_SUPPORT_CONNAC5X == 1 */
#endif /* _NIC_RXD_v5_H */
