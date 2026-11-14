// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   cmm_asic_common.c
 *    \brief  Internal driver stack will export the required procedures
 *            here for GLUE Layer.
 *
 *    This file contains all routines which are exported from MediaTek 802.11
 *    Wireless LAN driver stack to GLUE Layer.
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
#include "mt_dmac.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */


/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
u_int8_t g_fgWlanOnOffHoldRtnlLock;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
uint32_t asicGetFwDlInfo(struct ADAPTER *prAdapter,
			 char *pcBuf, int i4TotalLen)
{
	uint32_t u4Offset = 0;
#if !CFG_WLAN_LK_FWDL_SUPPORT
	struct TAILER_COMMON_FORMAT_T *prComTailer;
	uint8_t aucBuf[32];
#endif

	DBGLOG(HAL, TRACE, "enter asicGetFwDlInfo\n");
#if CFG_WLAN_LK_FWDL_SUPPORT
	u4Offset += snprintf(pcBuf + u4Offset, i4TotalLen - u4Offset,
				     "Release manifest: %s\n",
				     prAdapter->rVerInfo.aucReleaseManifest);
#else
	prComTailer = &prAdapter->rVerInfo.rCommonTailer;

	kalSnprintf(aucBuf, sizeof(aucBuf), "%10s", prComTailer->aucRamVersion);
	u4Offset += snprintf(pcBuf + u4Offset, i4TotalLen - u4Offset,
			     "Tailer Ver[%u:%u] %s (%s) info %u:E%u\n",
			     prComTailer->ucFormatVer,
			     prComTailer->ucFormatFlag,
			     aucBuf,
			     prComTailer->aucRamBuiltDate,
			     prComTailer->ucChipInfo,
			     prComTailer->ucEcoCode + 1);

	if (prComTailer->ucFormatFlag) {
		u4Offset += snprintf(pcBuf + u4Offset, i4TotalLen - u4Offset,
				     "Release manifest: %s\n",
				     prAdapter->rVerInfo.aucReleaseManifest);
	}
#endif
	return u4Offset;
}

uint32_t asicGetChipID(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4ChipID = 0;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);

	/* Compose chipID from chip ip version
	 *
	 * BIT(30, 31) : Coding type, 00: compact, 01: index table
	 * BIT(24, 29) : IP config (6 bits)
	 * BIT(8, 23)  : IP version
	 * BIT(0, 7)   : A die info
	 */

	u4ChipID = (0x0 << 30) |
		   ((prChipInfo->u4ChipIpConfig & 0x3F) << 24) |
		   ((prChipInfo->u4ChipIpVersion & 0xF0000000) >>  8) |
		   ((prChipInfo->u4ChipIpVersion & 0x000F0000) >>  0) |
		   ((prChipInfo->u4ChipIpVersion & 0x00000F00) <<  4) |
		   ((prChipInfo->u4ChipIpVersion & 0x0000000F) <<  8) |
		   (prChipInfo->u2ADieChipVersion & 0xFF);

	log_dbg(HAL, DEBUG, "ChipID = [0x%08x]\n", u4ChipID);
	return u4ChipID;
}

void fillNicTxDescAppend(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo,
			 uint8_t *prTxDescBuffer)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;

	/* Fill TxD append */
	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
			   prTxDescBuffer;
	kalMemZero(prHwTxDescAppend, prChipInfo->txd_append_size);
}

void fillTxDescAppendByHostV2(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint16_t u4MsduId,
	dma_addr_t rDmaAddr, uint32_t u4Idx,
	u_int8_t fgIsLast,
	uint8_t *pucBuffer)
{
	union HW_MAC_TX_DESC_APPEND *prHwTxDescAppend;
	struct TXD_PTR_LEN *prPtrLen;
	uint64_t u8Addr = (uint64_t)rDmaAddr;

	uint16_t u2Len = 0;
#if (CFG_SUPPORT_TX_SG == 1)
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	int i;
	struct MSDU_TOKEN_ENTRY *prToken = prMsduInfo->prToken;
#endif
#endif
	prHwTxDescAppend = (union HW_MAC_TX_DESC_APPEND *)
		(pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prHwTxDescAppend->CONNAC_APPEND.au2MsduId[u4Idx] =
		u4MsduId | TXD_MSDU_ID_VLD;
	prPtrLen = &prHwTxDescAppend->CONNAC_APPEND.arPtrLen[u4Idx >> 1];
	u2Len = nicTxGetFrameLength(prMsduInfo);

#if (CFG_SUPPORT_TX_SG == 1)
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	if (prToken && prToken->nr_frags)
		u2Len = nicTxGetFrameLength(prMsduInfo) - prToken->len_frags;
#endif
#endif
	u2Len = (u2Len & TXD_LEN_MASK_V2) |
		((u8Addr >> TXD_ADDR2_OFFSET) & TXD_ADDR2_MASK);
	u2Len |= TXD_LEN_ML_V2;

	if ((u4Idx & 1) == 0) {
		prPtrLen->u4Ptr0 = (uint32_t)u8Addr;
		prPtrLen->u2Len0 = u2Len;
	} else {
		prPtrLen->u4Ptr1 = (uint32_t)u8Addr;
		prPtrLen->u2Len1 = u2Len;
	}

#if (CFG_SUPPORT_TX_SG == 1)
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	/*
	* set multiple data section for single MPDU
	* 1. NOT SW-AMPDU mode (u4Idx>0)
	* 2. There are frag-memories in skb
	*/
	if (u4Idx || !prToken || !prToken->nr_frags)
		return;

	prPtrLen->u2Len0 &= ~TXD_LEN_ML_V2;
	for (i = 0; i < prToken->nr_frags; i++) {
		uint8_t nr_idx = i + 1;
		/* need to support 84 bit */
		u8Addr = prToken->rPktDmaAddr_nr[i];
		u2Len = prToken->u4PktDmaLength_nr[i];
		u2Len = (u2Len & TXD_LEN_MASK_V2) |
			((u8Addr >> TXD_ADDR2_OFFSET) & TXD_ADDR2_MASK);
		if (nr_idx == prToken->nr_frags)
			u2Len |= TXD_LEN_ML_V2;
		prHwTxDescAppend->CONNAC_APPEND.au2MsduId[nr_idx] = 0;
		prPtrLen = &prHwTxDescAppend->
			CONNAC_APPEND.arPtrLen[nr_idx >> 1];
		if ((nr_idx & 1) == 0) {
			prPtrLen->u4Ptr0 = (uint32_t)u8Addr;
			prPtrLen->u2Len0 = u2Len;
			DBGLOG(HAL, TRACE, "[TX] u4Ptr0=0x%x\n",
						prPtrLen->u4Ptr0);
			DBGLOG(HAL, TRACE, "[TX] u2Len0=0x%x\n",
						prPtrLen->u2Len0);
		} else {
			prPtrLen->u4Ptr1 = (uint32_t)u8Addr;
			prPtrLen->u2Len1 = u2Len;
			DBGLOG(HAL, TRACE, "[TX] u4Ptr1=0x%x\n",
						prPtrLen->u4Ptr1);
			DBGLOG(HAL, TRACE, "[TX] u2Len1=0x%x\n",
						prPtrLen->u2Len1);
		}
	}
#endif
#endif

	NIC_DUMP_TXD_HEADER(prAdapter, "Dump DATA TXD:\n");
	NIC_DUMP_TXD(prAdapter, pucBuffer, NIC_TX_DESC_LONG_FORMAT_LENGTH);

	NIC_DUMP_TXP_HEADER(prAdapter, "Dump DATA TXP: append=%zu, len=%u\n",
			sizeof(prHwTxDescAppend->CONNAC_APPEND),
			nicTxGetFrameLength(prMsduInfo));
	NIC_DUMP_TXP(prAdapter, (uint8_t *)prHwTxDescAppend,
			sizeof(prHwTxDescAppend->CONNAC_APPEND),
			nicTxGetFrameLength(prMsduInfo));
}

static char *q_idx_mcu_str[] = {"RQ0", "RQ1", "RQ2", "RQ3", "Invalid"};
static char *pkt_ft_str[] = {"cut_through", "store_forward",
	"cmd", "PDA_FW_Download"};
static char *hdr_fmt_str[] = {
	"Non-80211-Frame",
	"Command-Frame",
	"Normal-80211-Frame",
	"enhanced-80211-Frame",
};
static char *p_idx_str[] = {"LMAC", "MCU"};
static char *q_idx_lmac_str[] = {"WMM0_AC0", "WMM0_AC1", "WMM0_AC2", "WMM0_AC3",
	"WMM1_AC0", "WMM1_AC1", "WMM1_AC2", "WMM1_AC3",
	"WMM2_AC0", "WMM2_AC1", "WMM2_AC2", "WMM2_AC3",
	"WMM3_AC0", "WMM3_AC1", "WMM3_AC2", "WMM3_AC3",
	"Band0_ALTX", "Band0_BMC", "Band0_BNC", "Band0_PSMP",
	"Band1_ALTX", "Band1_BMC", "Band1_BNC", "Band1_PSMP",
	"Invalid"};

void halDumpTxdInfo(struct ADAPTER *prAdapter, uint8_t *tmac_info)
{
	struct TMAC_TXD_S *txd_s;
	struct TMAC_TXD_0 *txd_0;
	struct TMAC_TXD_1 *txd_1;
	uint8_t q_idx = 0;

	txd_s = (struct TMAC_TXD_S *)tmac_info;
	txd_0 = &txd_s->TxD0;
	txd_1 = &txd_s->TxD1;

	DBGLOG(HAL, DEBUG, "TMAC_TXD Fields:\n");
	DBGLOG(HAL, DEBUG, "\tTMAC_TXD_0:\n");
	DBGLOG(HAL, DEBUG, "\t\tPortID=%d(%s)\n",
			txd_0->p_idx, p_idx_str[txd_0->p_idx]);

	if (txd_0->p_idx == P_IDX_LMAC)
		q_idx = txd_0->q_idx % 0x18;
	else
		q_idx = ((txd_0->q_idx == TxQ_IDX_MCU_PDA) ?
			txd_0->q_idx : (txd_0->q_idx % 0x4));

	DBGLOG(HAL, DEBUG, "\t\tQueID=0x%x(%s %s)\n", txd_0->q_idx,
			 (txd_0->p_idx == P_IDX_LMAC ? "LMAC" : "MCU"),
			 txd_0->p_idx == P_IDX_LMAC ?
				q_idx_lmac_str[q_idx] : q_idx_mcu_str[q_idx]);
	DBGLOG(HAL, DEBUG, "\t\tTxByteCnt=%d\n", txd_0->TxByteCount);
	DBGLOG(HAL, DEBUG, "\t\tIpChkSumOffload=%d\n", txd_0->IpChkSumOffload);
	DBGLOG(HAL, DEBUG, "\t\tUdpTcpChkSumOffload=%d\n",
						txd_0->UdpTcpChkSumOffload);
	DBGLOG(HAL, DEBUG, "\t\tEthTypeOffset=%d\n", txd_0->EthTypeOffset);

	DBGLOG(HAL, DEBUG, "\tTMAC_TXD_1:\n");
	DBGLOG(HAL, DEBUG, "\t\twlan_idx=%d\n", txd_1->wlan_idx);
	DBGLOG(HAL, DEBUG, "\t\tHdrFmt=%d(%s)\n",
			 txd_1->hdr_format, hdr_fmt_str[txd_1->hdr_format]);
	DBGLOG(HAL, DEBUG, "\t\tHdrInfo=0x%x\n", txd_1->hdr_info);

	switch (txd_1->hdr_format) {
	case TMI_HDR_FT_NON_80211:
		DBGLOG(HAL, DEBUG,
			"\t\t\tMRD=%d, EOSP=%d, RMVL=%d, VLAN=%d, ETYP=%d\n",
			txd_1->hdr_info & (1 << TMI_HDR_INFO_0_BIT_MRD),
			txd_1->hdr_info & (1 << TMI_HDR_INFO_0_BIT_EOSP),
			txd_1->hdr_info & (1 << TMI_HDR_INFO_0_BIT_RMVL),
			txd_1->hdr_info & (1 << TMI_HDR_INFO_0_BIT_VLAN),
			txd_1->hdr_info & (1 << TMI_HDR_INFO_0_BIT_ETYP));
		break;

	case TMI_HDR_FT_CMD:
		DBGLOG(HAL, DEBUG, "\t\t\tRsvd=0x%x\n", txd_1->hdr_info);
		break;

	case TMI_HDR_FT_NOR_80211:
		DBGLOG(HAL, DEBUG, "\t\t\tHeader Len=%d(WORD)\n",
				 txd_1->hdr_info & TMI_HDR_INFO_2_MASK_LEN);
		break;

	case TMI_HDR_FT_ENH_80211:
		DBGLOG(HAL, DEBUG, "\t\t\tEOSP=%d, AMS=%d\n",
			txd_1->hdr_info & (1 << TMI_HDR_INFO_3_BIT_EOSP),
			txd_1->hdr_info & (1 << TMI_HDR_INFO_3_BIT_AMS));
		break;
	}

	DBGLOG(HAL, DEBUG, "\t\tTxDFormatType=%d(%s format)\n", txd_1->ft,
		(txd_1->ft == TMI_FT_LONG ?
		"Long - 8 DWORD" : "Short - 3 DWORD"));
	DBGLOG(HAL, DEBUG, "\t\ttxd_len=%d page(%d DW)\n",
		txd_1->txd_len == 0 ? 1 : 2, (txd_1->txd_len + 1) * 16);
	DBGLOG(HAL, DEBUG,
		"\t\tHdrPad=%d(Padding Mode: %s, padding bytes: %d)\n",
		txd_1->hdr_pad,
		((txd_1->hdr_pad & (TMI_HDR_PAD_MODE_TAIL << 1)) ?
		"tail" : "head"), (txd_1->hdr_pad & 0x1 ? 2 : 0));
	DBGLOG(HAL, DEBUG, "\t\tUNxV=%d\n", txd_1->UNxV);
	DBGLOG(HAL, DEBUG, "\t\tamsdu=%d\n", txd_1->amsdu);
	DBGLOG(HAL, DEBUG, "\t\tTID=%d\n", txd_1->tid);
	DBGLOG(HAL, DEBUG, "\t\tpkt_ft=%d(%s)\n",
			 txd_1->pkt_ft, pkt_ft_str[txd_1->pkt_ft]);
	DBGLOG(HAL, DEBUG, "\t\town_mac=%d\n", txd_1->OwnMacAddr);

	if (txd_s->TxD1.ft == TMI_FT_LONG) {
		struct TMAC_TXD_L *txd_l = (struct TMAC_TXD_L *)tmac_info;
		struct TMAC_TXD_2 *txd_2 = &txd_l->TxD2;
		struct TMAC_TXD_3 *txd_3 = &txd_l->TxD3;
		struct TMAC_TXD_4 *txd_4 = &txd_l->TxD4;
		struct TMAC_TXD_5 *txd_5 = &txd_l->TxD5;
		struct TMAC_TXD_6 *txd_6 = &txd_l->TxD6;

		DBGLOG(HAL, DEBUG, "\tTMAC_TXD_2:\n");
		DBGLOG(HAL, DEBUG, "\t\tsub_type=%d\n", txd_2->sub_type);
		DBGLOG(HAL, DEBUG, "\t\tfrm_type=%d\n", txd_2->frm_type);
		DBGLOG(HAL, DEBUG, "\t\tNDP=%d\n", txd_2->ndp);
		DBGLOG(HAL, DEBUG, "\t\tNDPA=%d\n", txd_2->ndpa);
		DBGLOG(HAL, DEBUG, "\t\tSounding=%d\n", txd_2->sounding);
		DBGLOG(HAL, DEBUG, "\t\tRTS=%d\n", txd_2->rts);
		DBGLOG(HAL, DEBUG, "\t\tbc_mc_pkt=%d\n", txd_2->bc_mc_pkt);
		DBGLOG(HAL, DEBUG, "\t\tBIP=%d\n", txd_2->bip);
		DBGLOG(HAL, DEBUG, "\t\tDuration=%d\n", txd_2->duration);
		DBGLOG(HAL, DEBUG, "\t\tHE(HTC Exist)=%d\n", txd_2->htc_vld);
		DBGLOG(HAL, DEBUG, "\t\tFRAG=%d\n", txd_2->frag);
		DBGLOG(HAL, DEBUG, "\t\tReamingLife/MaxTx time=%d\n",
			txd_2->max_tx_time);
		DBGLOG(HAL, DEBUG, "\t\tpwr_offset=%d\n", txd_2->pwr_offset);
		DBGLOG(HAL, DEBUG, "\t\tba_disable=%d\n", txd_2->ba_disable);
		DBGLOG(HAL, DEBUG, "\t\ttiming_measure=%d\n",
			txd_2->timing_measure);
		DBGLOG(HAL, DEBUG, "\t\tfix_rate=%d\n", txd_2->fix_rate);
		DBGLOG(HAL, DEBUG, "\tTMAC_TXD_3:\n");
		DBGLOG(HAL, DEBUG, "\t\tNoAck=%d\n", txd_3->no_ack);
		DBGLOG(HAL, DEBUG, "\t\tPF=%d\n", txd_3->protect_frm);
		DBGLOG(HAL, DEBUG, "\t\ttx_cnt=%d\n", txd_3->tx_cnt);
		DBGLOG(HAL, DEBUG, "\t\tremain_tx_cnt=%d\n",
			txd_3->remain_tx_cnt);
		DBGLOG(HAL, DEBUG, "\t\tsn=%d\n", txd_3->sn);
		DBGLOG(HAL, DEBUG, "\t\tpn_vld=%d\n", txd_3->pn_vld);
		DBGLOG(HAL, DEBUG, "\t\tsn_vld=%d\n", txd_3->sn_vld);
		DBGLOG(HAL, DEBUG, "\tTMAC_TXD_4:\n");
		DBGLOG(HAL, DEBUG, "\t\tpn_low=0x%x\n", txd_4->pn_low);
		DBGLOG(HAL, DEBUG, "\tTMAC_TXD_5:\n");
		DBGLOG(HAL, DEBUG, "\t\ttx_status_2_host=%d\n",
			txd_5->tx_status_2_host);
		DBGLOG(HAL, DEBUG, "\t\ttx_status_2_mcu=%d\n",
			txd_5->tx_status_2_mcu);
		DBGLOG(HAL, DEBUG, "\t\ttx_status_fmt=%d\n",
			txd_5->tx_status_fmt);

		if (txd_5->tx_status_2_host || txd_5->tx_status_2_mcu)
			DBGLOG(HAL, DEBUG, "\t\tpid=%d\n", txd_5->pid);

		if (txd_2->fix_rate)
			DBGLOG(HAL, DEBUG,
				"\t\tda_select=%d\n", txd_5->da_select);

		DBGLOG(HAL, DEBUG, "\t\tpwr_mgmt=0x%x\n", txd_5->pwr_mgmt);
		DBGLOG(HAL, DEBUG, "\t\tpn_high=0x%x\n", txd_5->pn_high);

		if (txd_2->fix_rate) {
			DBGLOG(HAL, DEBUG, "\tTMAC_TXD_6:\n");
			DBGLOG(HAL, DEBUG, "\t\tfix_rate_mode=%d\n",
				txd_6->fix_rate_mode);
			DBGLOG(HAL, DEBUG, "\t\tGI=%d(%s)\n", txd_6->gi,
				(txd_6->gi == 0 ? "LONG" : "SHORT"));
			DBGLOG(HAL, DEBUG, "\t\tldpc=%d(%s)\n", txd_6->ldpc,
				(txd_6->ldpc == 0 ? "BCC" : "LDPC"));
			DBGLOG(HAL, DEBUG, "\t\tTxBF=%d\n", txd_6->TxBF);
			DBGLOG(HAL, DEBUG,
			       "\t\ttx_rate=0x%x\n", txd_6->tx_rate);
			DBGLOG(HAL, DEBUG, "\t\tant_id=%d\n", txd_6->ant_id);
			DBGLOG(HAL, DEBUG, "\t\tdyn_bw=%d\n", txd_6->dyn_bw);
			DBGLOG(HAL, DEBUG, "\t\tbw=%d\n", txd_6->bw);
		}
	}
}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
void asicWakeUpWiFi(struct ADAPTER *prAdapter)
{
	u_int8_t fgResult;

	ASSERT(prAdapter);

	HAL_LP_OWN_RD(prAdapter, &fgResult);

	if (fgResult) {
		prAdapter->fgIsFwOwn = FALSE;
		DBGLOG(HAL, WARN,
			"Already DriverOwn, set flag only\n");
	}
	else
		HAL_LP_OWN_CLR(prAdapter, &fgResult);
}
#endif /* _HIF_PCIE || _HIF_AXI */

#if defined(_HIF_USB)
void fillUsbHifTxDesc(uint8_t **pDest, uint16_t *pInfoBufLen,
	uint8_t ucPacketType)
{
	/*USB TX Descriptor (4 bytes)*/
	/* BIT[15:0] - TX Bytes Count
	 * (Not including USB TX Descriptor and 4-bytes zero padding.
	 */
	kalMemZero((void *)*pDest, sizeof(uint32_t));
	kalMemCopy((void *)*pDest, (void *) pInfoBufLen,
		   sizeof(uint16_t));
}
#endif

#if CFG_MTK_ANDROID_WMT
#if !CFG_SUPPORT_CONNAC1X
static int wlan_func_on_by_chrdev(void)
{
#define WAKE_LOCK_ON_TIMEOUT	15000	/* ms */
#define MAX_RETRY_COUNT		100

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T * prWlanOnOffWakeLock;
#endif
	int retry = 0;
	int ret = 0;

	while (TRUE) {
		if (retry >= MAX_RETRY_COUNT) {
			DBGLOG(INIT, WARN,
				"Timeout, pre_cal_flow: %d, locked: %d.\n",
				is_cal_flow_finished(),
				wfsys_is_locked());
			ret = -EBUSY;
			goto exit;
		}

		if (is_cal_flow_finished() && !wfsys_is_locked())
			break;

		retry++;
		kalMdelay(100);
	}

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(NULL,
		prWlanOnOffWakeLock, "WIFI_on");
	KAL_WAKE_LOCK_TIMEOUT(NULL, prWlanOnOffWakeLock,
		MSEC_TO_JIFFIES(WAKE_LOCK_ON_TIMEOUT));
#endif

	wfsys_lock();
	ret = wlanFuncOn();
	wfsys_unlock();

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_UNLOCK(NULL, prWlanOnOffWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prWlanOnOffWakeLock);
#endif

exit:
	return ret;
}

static int wlan_func_off_by_chrdev(void)
{
	wfsys_lock();
	wlanFuncOff();
	wfsys_unlock();

	return 0;
}

void unregister_chrdev_cbs(void)
{
	mtk_wcn_wlan_unreg();
}

void register_chrdev_cbs(void)
{
	struct MTK_WCN_WLAN_CB_INFO rWlanCb;

	kalMemZero(&rWlanCb, sizeof(struct MTK_WCN_WLAN_CB_INFO));
	rWlanCb.wlan_probe_cb = wlan_func_on_by_chrdev;
	rWlanCb.wlan_remove_cb = wlan_func_off_by_chrdev;
	mtk_wcn_wlan_reg(&rWlanCb);
}
#endif
#endif

int wlan_test_mode_on(bool uIsSwtichTestMode)
{
	int32_t ret = 0;
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	DBGLOG(INIT, DEBUG, "uIsSwtichTestMode: %d\n", uIsSwtichTestMode);

	if (kalIsResetOnEnd() == TRUE) {
		DBGLOG(INIT, DEBUG, "now is resetting\n");
		ret = WLAN_STATUS_FAILURE;
		return ret;
	}

	if (!wfsys_trylock()) {
		DBGLOG(INIT, DEBUG, "now is write processing\n");
		ret = WLAN_STATUS_FAILURE;
		return ret;
	}

	set_wifi_in_switch_mode(1);
	g_fgWlanOnOffHoldRtnlLock = 1;

	wlanFuncOff();
	if (uIsSwtichTestMode)
		set_wifi_test_mode_fwdl(1);
	ret = wlanFuncOn();
	if (uIsSwtichTestMode)
		set_wifi_test_mode_fwdl(0);

	g_fgWlanOnOffHoldRtnlLock = 0;
	set_wifi_in_switch_mode(0);
	wfsys_unlock();
#endif
	return ret;
}
#if defined(_HIF_SDIO)
void fillSdioHifTxDesc(uint8_t **pDest, uint16_t *pInfoBufLen,
	uint8_t ucPacketType)
{
	/* SDIO TX Descriptor (4 bytes)*/

	/* BIT[15:00] - TX Bytes Count
	 * BIT[17:16] - Packet Type
	 * BIT[31:18] - Reserved
	 */
	struct SDIO_HIF_TX_HEADER sdio_hif_header = {0};

	sdio_hif_header.InfoBufLen = (*pInfoBufLen + SDIO_HIF_TXD_LEN);
	sdio_hif_header.Type =
		(ucPacketType & SDIO_HIF_TXD_PKG_TYPE_MASK)
				<< SDIO_HIF_TXD_PKG_TYPE_SHIFT;

	kalMemZero((void *)*pDest, SDIO_HIF_TXD_LEN);
	kalMemCopy((void *)*pDest, &sdio_hif_header, SDIO_HIF_TXD_LEN);
}
#endif /* _HIF_SDIO */
