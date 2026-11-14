// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if (CFG_SUPPORT_CONNAC5X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#if (DBG_DISABLE_ALL_INFO == 0)
#include "dbg_wtbl_connac5x.h"
#ifdef MT7999
#include "coda/mt7999/wf_wtblon_top.h"
#include "coda/mt7999/wf_uwtbl_top.h"
#include "coda/mt7999/wf_ds_uwtbl.h"
#include "coda/mt7999/wf_ds_lwtbl.h"
#endif

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
static char *RATE_V3_HW_TX_MODE_STR[] = {
	"CCK", "OFDM", "MM", "GF", "VHT", "PLR",
	"N/A", "N/A", "HE_SU", "HE_ER", "HE_TRIG", "HE_MU",
	"N/A", "EHT_ER", "EHT_TRIG", "EHT_MU"};

uint32_t halWtblReadRaw(
	struct ADAPTER *prAdapter,
	uint16_t  u2EntryIdx,
	enum _ENUM_WTBL_TYPE_T eType,
	uint16_t u2StartDW,
	uint16_t u2LenInDW,
	void *pBuffer)
{
	uint32_t *dest_cpy = (uint32_t *)pBuffer;
	uint32_t sizeInDW = u2LenInDW;
	uint32_t u4SrcAddr = 0;

	if (pBuffer == NULL)
		return 0xFF;

	if (eType == WTBL_TYPE_LMAC) {
		HAL_MCR_WR(prAdapter, WF_WTBLON_TOP_WDUCR_ADDR,
			((u2EntryIdx >> 7) & WF_WTBLON_TOP_WDUCR_GROUP_MASK) << WF_WTBLON_TOP_WDUCR_GROUP_SHFT);
		u4SrcAddr = LWTBL_IDX2BASE(u2EntryIdx, u2StartDW);
	} else if (eType == WTBL_TYPE_UMAC) {
		HAL_MCR_WR(prAdapter, WF_UWTBL_TOP_WDUCR_ADDR,
			((u2EntryIdx >> 7) & WF_UWTBL_TOP_WDUCR_GROUP_MASK) << WF_UWTBL_TOP_WDUCR_GROUP_SHFT);
		u4SrcAddr = UWTBL_IDX2BASE(u2EntryIdx, u2StartDW);
	} else if (eType == WTBL_TYPE_KEY) {
		HAL_MCR_WR(prAdapter, WF_UWTBL_TOP_WDUCR_ADDR,
			(WF_UWTBL_TOP_WDUCR_TARGET_MASK | (((u2EntryIdx >> 7) & WF_UWTBL_TOP_WDUCR_GROUP_MASK) << WF_UWTBL_TOP_WDUCR_GROUP_SHFT)));
		u4SrcAddr = KEYTBL_IDX2BASE(u2EntryIdx, u2StartDW);
	} else {
		/* TODO: */
	}

	while (sizeInDW--) {
		uint32_t u4Value = 0;

		HAL_RMCR_RD(WTBL_DBG, prAdapter, u4SrcAddr, &u4Value);
		*dest_cpy++ = u4Value;
		u4SrcAddr += 4;
	}

	return 0;
}

u_int8_t connac5x_wtbl_get_sgi_info(
	struct bwtbl_lmac_struct *pWtbl)
{
	if (!pWtbl)
		return FALSE;

	switch (pWtbl->trx_cap.wtbl_d9.field.fcap) {
	case BW_20:
		return ((pWtbl->trx_cap.wtbl_d2.field.he) ?
			(pWtbl->trx_cap.wtbl_d6.field.g2_he) :
			(pWtbl->trx_cap.wtbl_d6.field.g2));
	case BW_40:
		return ((pWtbl->trx_cap.wtbl_d2.field.he) ?
			(pWtbl->trx_cap.wtbl_d6.field.g4_he) :
			(pWtbl->trx_cap.wtbl_d6.field.g4));
	case BW_80:
		return ((pWtbl->trx_cap.wtbl_d2.field.he) ?
			(pWtbl->trx_cap.wtbl_d6.field.g8_he) :
			(pWtbl->trx_cap.wtbl_d6.field.g8));
	case BW_160:
		return ((pWtbl->trx_cap.wtbl_d2.field.he) ?
			(pWtbl->trx_cap.wtbl_d6.field.g16_he) :
			(pWtbl->trx_cap.wtbl_d6.field.g16));
	case BW_320:
		return pWtbl->trx_cap.wtbl_d5.field.gi_eht;
	default:
		return FALSE;
	}
}

u_int8_t connac5x_wtbl_get_ldpc_info(
	uint8_t ucTxMode,
	struct bwtbl_lmac_struct *pWtbl)
{
	if (!pWtbl)
		return FALSE;

	switch (ucTxMode) {
	case ENUM_TX_MODE_MM:
	case ENUM_TX_MODE_GF:
		return pWtbl->trx_cap.wtbl_d4.field.ldpc_ht;
	case ENUM_TX_MODE_VHT:
		return pWtbl->trx_cap.wtbl_d4.field.ldpc_vht;
#if (CFG_SUPPORT_802_11AX == 1)
	case ENUM_TX_MODE_HE_SU:
	case ENUM_TX_MODE_HE_ER:
	case ENUM_TX_MODE_HE_MU:
		return pWtbl->trx_cap.wtbl_d4.field.ldpc_he;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	case ENUM_TX_MODE_EHT_MU:
		return pWtbl->trx_cap.wtbl_d4.field.ldpc_eht;
#endif
	case ENUM_TX_MODE_CCK:
	case ENUM_TX_MODE_OFDM:
	default:
		return FALSE;
	}
}

static int32_t connac5x_wtbl_rate_to_string(
	char *pcCommand,
	int i4TotalLen,
	uint8_t TxRx,
	struct bwtbl_lmac_struct *pWtbl,
	int32_t i4BytesWritten)
{
	uint8_t i, txmode, rate, stbc, fcap;
	uint8_t nss, gi;
	uint16_t arTxRate[8];
	char prefix[16] = {" "};

	arTxRate[0] = pWtbl->auto_rate_tb.wtbl_d10.field.rate1;
	arTxRate[1] = pWtbl->auto_rate_tb.wtbl_d10.field.rate2;
	arTxRate[2] = pWtbl->auto_rate_tb.wtbl_d11.field.rate3;
	arTxRate[3] = pWtbl->auto_rate_tb.wtbl_d11.field.rate4;
	arTxRate[4] = pWtbl->auto_rate_tb.wtbl_d12.field.rate5;
	arTxRate[5] = pWtbl->auto_rate_tb.wtbl_d12.field.rate6;
	arTxRate[6] = pWtbl->auto_rate_tb.wtbl_d13.field.rate7;
	arTxRate[7] = pWtbl->auto_rate_tb.wtbl_d13.field.rate8;
	fcap = pWtbl->trx_cap.wtbl_d9.field.fcap;
	for (i = 0; i < AUTO_RATE_NUM; i++) {

		txmode = CONNAC5X_HW_TX_RATE_TO_MODE(arTxRate[i]);
#if (CFG_SUPPORT_802_11BE == 0)
		if (txmode >= ENUM_TX_MODE_NUM)
			txmode = ENUM_TX_MODE_NUM - 1;
#endif
		rate = HW_TX_RATE_TO_MCS(arTxRate[i]);
		nss = CONNAC5X_HW_TX_RATE_TO_NSS(arTxRate[i]) + 1;
		stbc = CONNAC5X_HW_TX_RATE_TO_STBC(arTxRate[i]);
		gi = connac5x_wtbl_get_sgi_info(pWtbl);

		if (pWtbl->trx_cap.wtbl_d9.field.rate_idx == i) {
			if (TxRx == 0)
				kalStrnCpy(prefix, "[Last RX Rate] ", 16);
			else
				kalStrnCpy(prefix, "[Last TX Rate] ", 16);
		}
		else
			kalStrnCpy(prefix, "               ", 16);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
				i4BytesWritten,
				"\tRateIdx[%d] %s %s, %s, %s, %s, %s%s\n",
				i,
				prefix,
				rate < 4 ?
					HW_TX_RATE_CCK_STR[rate] :
					HW_TX_RATE_CCK_STR[4],
				(fcap < 5) ?
					HW_TX_RATE_BW[fcap] : HW_TX_RATE_BW[5],
				rate < 4 ? "LP" : "SP",
				RATE_V3_HW_TX_MODE_STR[txmode],
				stbc ? "STBC, " : "",
				connac5x_wtbl_get_ldpc_info(txmode, pWtbl)
					== 0 ? "BCC" : "LDPC");
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
				i4BytesWritten,
				"\tRateIdx[%d] %s %s, %s, %s, %s%s\n",
				i,
				prefix,
				nicHwRateOfdmStr(rate),
				(fcap < 5) ?
					HW_TX_RATE_BW[fcap] : HW_TX_RATE_BW[5],
				RATE_V3_HW_TX_MODE_STR[txmode],
				stbc ? "STBC, " : "",
				connac5x_wtbl_get_ldpc_info(txmode, pWtbl)
					== 0 ? "BCC" : "LDPC");
		else if ((txmode == TX_RATE_MODE_HTMIX) ||
			 (txmode == TX_RATE_MODE_HTGF))
			i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
				i4BytesWritten,
				"\tRateIdx[%d] %s MCS%d, %s, %s, %s, %s%s\n",
				i,
				prefix,
				rate,
				(fcap < 5) ?
					HW_TX_RATE_BW[fcap] : HW_TX_RATE_BW[5],
				gi == 0 ? "LGI" : "SGI",
				RATE_V3_HW_TX_MODE_STR[txmode],
				stbc ? "STBC, " : "",
				connac5x_wtbl_get_ldpc_info(txmode, pWtbl)
					== 0 ? "BCC" : "LDPC");
		else if ((txmode == TX_RATE_MODE_VHT) ||
			 (txmode == TX_RATE_MODE_PLR))
			i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
				i4BytesWritten,
				"\tRateIdx[%d] %s %s%d_MCS%d, %s, %s, %s, %s%s\n",
				i,
				prefix,
				stbc ? "NSTS" : "NSS",
				nss, rate,
				(fcap < 5) ?
					HW_TX_RATE_BW[fcap] : HW_TX_RATE_BW[5],
				gi == 0 ? "LGI" : "SGI",
				RATE_V3_HW_TX_MODE_STR[txmode],
				stbc ? "STBC, " : "",
				connac5x_wtbl_get_ldpc_info(txmode, pWtbl)
					== 0 ? "BCC" : "LDPC");
		else {
			i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
				i4BytesWritten,
				"\tRateIdx[%d] %s %s%d_MCS%d, %s, %s, %s, %s%s\n",
				i,
				prefix,
				stbc ? "NSTS" : "NSS",
				nss, rate,
				(fcap < 5) ?
					HW_TX_RATE_BW[fcap] : HW_TX_RATE_BW[5],
				gi == 0 ? "SGI" : (gi == 1 ? "MGI" : "LGI"),
				RATE_V3_HW_TX_MODE_STR[txmode],
				stbc ? "STBC, " : "",
				connac5x_wtbl_get_ldpc_info(txmode, pWtbl)
					== 0 ? "BCC" : "LDPC");
		}
	}

	return i4BytesWritten;
}

static int32_t connac5x_dump_helper_wtbl_info(
	struct ADAPTER *prAdapter,
	char *pcCommand,
	int i4TotalLen,
	struct bwtbl_lmac_struct *pWtbl,
	uint32_t u4Index)
{
	int32_t i4BytesWritten = 0;
	uint8_t aucPA[MAC_ADDR_LEN];
	uint8_t cipher_suit_igtk, cipher_suit_bigtk;

	if (!pcCommand) {
		DBGLOG(HAL, ERROR, "%s: pcCommand is NULL.\n",
			__func__);
		return i4BytesWritten;
	}

	aucPA[0] =
		pWtbl->peer_basic_info.wtbl_d1.field.peer_addr & 0xff;
	aucPA[1] =
		((pWtbl->peer_basic_info.wtbl_d1.field.peer_addr &
			0xff00) >> 8);
	aucPA[2] =
		((pWtbl->peer_basic_info.wtbl_d1.field.peer_addr &
			0xff0000) >> 16);
	aucPA[3] =
		((pWtbl->peer_basic_info.wtbl_d1.field.peer_addr &
			0xff000000) >> 24);
	aucPA[4] =
		pWtbl->peer_basic_info.wtbl_d0.field.peer_addr & 0xff;
	aucPA[5] =
		((pWtbl->peer_basic_info.wtbl_d0.field.peer_addr &
			0xff00) >> 8);
	cipher_suit_igtk =
		pWtbl->auto_rate_counters.wtbl_d14.field_v2.cipher_suit_igtk;
	cipher_suit_bigtk =
		pWtbl->auto_rate_counters.wtbl_d14.field_v2.cipher_suit_bigtk;

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
			"Dump WTBL info of WLAN_IDX	    = %d\n",
		u4Index);

	DBGLOG(REQ, DEBUG, "====DW0~1====\n");
	/* DW0~DW1 */
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tADDR="MACSTR"\n",
		MAC2STR(aucPA));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tMUAR/RCA1/KID/RCID:%d/%d/%d/%d\n",
		pWtbl->peer_basic_info.wtbl_d0.field.muar_idx,
		pWtbl->peer_basic_info.wtbl_d0.field.rc_a1,
		pWtbl->peer_basic_info.wtbl_d0.field.kid,
		pWtbl->peer_basic_info.wtbl_d0.field.rc_id);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tBN/RV/RCA2/WPI_FLAG:%d/%d/%d/%d\n",
		pWtbl->peer_basic_info.wtbl_d0.field.band,
		pWtbl->peer_basic_info.wtbl_d0.field.rv,
		pWtbl->peer_basic_info.wtbl_d0.field.rc_a2,
		pWtbl->peer_basic_info.wtbl_d0.field.wpi_flg);

	/* DW2~4 */
	DBGLOG(REQ, DEBUG, "====DW2~4====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tAID12/GID_SU/DUAL_PTEC_EN/DUAL_CTS_EN:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d2.field.aid12,
		pWtbl->trx_cap.wtbl_d2.field.gid_su,
		pWtbl->trx_cap.wtbl_d2.field.dual_ptec_en,
		pWtbl->trx_cap.wtbl_d2.field.dual_cts_cap);

	/* DUMP DW14 for BMC entry only */
	if (pWtbl->peer_basic_info.wtbl_d0.field.muar_idx
			== MUAR_INDEX_OWN_MAC_ADDR_BC_MC)
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tFD/TD/CIPHER[PGKT/IGTK/GIBTK]:%d/%d/%d/%d/%d\n",
			pWtbl->trx_cap.wtbl_d2.field.fd,
			pWtbl->trx_cap.wtbl_d2.field.td,
			pWtbl->trx_cap.wtbl_d2.field.cipher_suit_pgkt,
			cipher_suit_igtk,
			cipher_suit_bigtk);
	else
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tFD/TD/CIPHER_PGKT:%d/%d/%d\n",
			pWtbl->trx_cap.wtbl_d2.field.fd,
			pWtbl->trx_cap.wtbl_d2.field.td,
			pWtbl->trx_cap.wtbl_d2.field.cipher_suit_pgkt);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tSW/UL/TXPS/QoS/MESH:%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d2.field.sw,
		pWtbl->trx_cap.wtbl_d2.field.ul,
		pWtbl->trx_cap.wtbl_d2.field.tx_ps,
		pWtbl->trx_cap.wtbl_d2.field.qos,
		pWtbl->trx_cap.wtbl_d2.field.mesh);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tHT/VHT/HE/EHT:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d2.field.ht,
		pWtbl->trx_cap.wtbl_d2.field.vht,
		pWtbl->trx_cap.wtbl_d2.field.he,
		pWtbl->trx_cap.wtbl_d2.field.eht);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tLDPC[HT/VHT/HE/EHT]:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d4.field.ldpc_ht,
		pWtbl->trx_cap.wtbl_d4.field.ldpc_vht,
		pWtbl->trx_cap.wtbl_d4.field.ldpc_he,
		pWtbl->trx_cap.wtbl_d4.field.ldpc_eht);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tWMMQ/EHT_SIG_MCS/HDRT_MODE/BEAM_CHG:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d3.field.wmm_q,
		pWtbl->trx_cap.wtbl_d3.field.eht_sig_mcs,
		pWtbl->trx_cap.wtbl_d3.field.hdrt_mode,
		pWtbl->trx_cap.wtbl_d3.field.beam_chg);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tEHT_LTF_SYM_NUM/PFMU_IDX/ULPF_IDX:%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d3.field.eht_ltf_sym_num_opt,
		pWtbl->trx_cap.wtbl_d3.field.pfmu_index,
		pWtbl->trx_cap.wtbl_d3.field.ulpf_index);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tIGB_FBK/TBF[HT/VHT/HE/EHT]:%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d3.field.ign_fbk,
		pWtbl->trx_cap.wtbl_d3.field.tbf_ht,
		pWtbl->trx_cap.wtbl_d3.field.tbf_vht,
		pWtbl->trx_cap.wtbl_d3.field.tbf_he,
		pWtbl->trx_cap.wtbl_d3.field.tbf_eht);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tRIBF/ULPF/BYPASS_TXSMM:%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d3.field.ribf,
		pWtbl->trx_cap.wtbl_d3.field.ulpf,
		pWtbl->trx_cap.wtbl_d3.field.bypass_txSMM);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tNEGO_WINSZ [0~7]:%d/%d/%d/%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize0,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize1,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize2,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize3,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize4,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize5,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize6,
		pWtbl->trx_cap.wtbl_d4.field.nego_winsize7);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tPE/DIS_RHTR/BA_MODE:%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d4.field.pe,
		pWtbl->trx_cap.wtbl_d4.field.dis_rhtr,
		pWtbl->trx_cap.wtbl_d4.field.ba_mode);
	/* DW5 */
	DBGLOG(REQ, DEBUG, "====DW5====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tAF/BSA_EN/RTS/SMPS/DYNBW/MMSS:%d/%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d5.field.af,
		pWtbl->trx_cap.wtbl_d5.field.bsa_en,
		pWtbl->trx_cap.wtbl_d5.field.rts,
		pWtbl->trx_cap.wtbl_d5.field.smps,
		pWtbl->trx_cap.wtbl_d5.field.dyn_bw,
		pWtbl->trx_cap.wtbl_d5.field.mmss);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tUSR/SR_R/SR_A/TXPWR_OFST:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d5.field.usr,
		pWtbl->trx_cap.wtbl_d5.field.sr_r,
		pWtbl->trx_cap.wtbl_d5.field.sr_abort,
		pWtbl->trx_cap.wtbl_d5.field.tx_power_offset);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tEHT[LTF/GL]/DOPPL/TXOP_PS_CAP:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d5.field.ltf_eht,
		pWtbl->trx_cap.wtbl_d5.field.gi_eht,
		pWtbl->trx_cap.wtbl_d5.field.doppl,
		pWtbl->trx_cap.wtbl_d5.field.txop_ps_cap);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tDU_I_PSM/I_PSM/PSM/SKIP_TX:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d5.field.du_i_psm,
		pWtbl->trx_cap.wtbl_d5.field.i_psm,
		pWtbl->trx_cap.wtbl_d5.field.psm,
		pWtbl->trx_cap.wtbl_d5.field.skip_tx);

	/* DW6 */
	DBGLOG(REQ, DEBUG, "====DW6====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tCBRN/DBNSS_EN/BAFEN/RDGBA/R:%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d6.field.cbrn,
		pWtbl->trx_cap.wtbl_d6.field.dbnss_en,
		pWtbl->trx_cap.wtbl_d6.field.baf_en,
		pWtbl->trx_cap.wtbl_d6.field.rdgba,
		pWtbl->trx_cap.wtbl_d6.field.r);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tG2/G4/G8/G16/SPE:%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d6.field.g2,
		pWtbl->trx_cap.wtbl_d6.field.g4,
		pWtbl->trx_cap.wtbl_d6.field.g8,
		pWtbl->trx_cap.wtbl_d6.field.g16,
		pWtbl->trx_cap.wtbl_d6.field.spe_idx);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tHE[G2/G4/G8/G16]:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d6.field.g2_he,
		pWtbl->trx_cap.wtbl_d6.field.g4_he,
		pWtbl->trx_cap.wtbl_d6.field.g8_he,
		pWtbl->trx_cap.wtbl_d6.field.g16_he);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tLTF[G2/G4/G8/G16]:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d6.field.g2_ltf,
		pWtbl->trx_cap.wtbl_d6.field.g4_ltf,
		pWtbl->trx_cap.wtbl_d6.field.g8_ltf,
		pWtbl->trx_cap.wtbl_d6.field.g16_ltf);

	/* DW7 */
	DBGLOG(REQ, DEBUG, "====DW7====\n");
	if (pWtbl->trx_cap.wtbl_d2.field.qos)
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tBaWinSize TID[0~7]:%d/%d/%d/%d/%d/%d/%d/%d\n",
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid0),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid1),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid2),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid3),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid4),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid5),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid6),
			(uint32_t)
			(pWtbl->trx_cap.wtbl_d7.field.ba_win_size_tid7));

	/* DW8 */
	DBGLOG(REQ, DEBUG, "====DW8====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tCHK_PER/P_AID:%d/%d\n",
		pWtbl->trx_cap.wtbl_d8.field.chk_per,
		pWtbl->trx_cap.wtbl_d8.field.partial_aid);


	/* DW9 */
	DBGLOG(REQ, DEBUG, "====DW9====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tPRITX[SW_M/ERSU/PLR/DCM/ER106T]:%d/%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d9.field.pritx_sw_mode,
		pWtbl->trx_cap.wtbl_d9.field.pritx_ersu,
		pWtbl->trx_cap.wtbl_d9.field.pritx_plr,
		pWtbl->trx_cap.wtbl_d9.field.pritx_dcm,
		pWtbl->trx_cap.wtbl_d9.field.pritx_er106t);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRX_AVG_MPDU_SIZE/FCAP/MPDU[OK/FAIL]:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d9.field.rx_avg_mpdu_size,
		pWtbl->trx_cap.wtbl_d9.field.fcap,
		pWtbl->trx_cap.wtbl_d9.field.mpdu_fail_cnt,
		pWtbl->trx_cap.wtbl_d9.field.mpdu_ok_cnt);

	/* DW28 */
	DBGLOG(REQ, DEBUG, "====DW28====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRELATED[IDX0/BN0/IDX1/BN1]:%d/%d/%d/%d\n",
		pWtbl->mlo_info.wtbl_d28.field.related_idx0,
		pWtbl->mlo_info.wtbl_d28.field.related_band0,
		pWtbl->mlo_info.wtbl_d28.field.related_idx1,
		pWtbl->mlo_info.wtbl_d28.field.related_band1);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tMLD[PRI_BN/SEC_BN]:%d/%d\n",
		pWtbl->mlo_info.wtbl_d28.field.pri_mld_band,
		pWtbl->mlo_info.wtbl_d28.field.sec_mld_band);

	/* DW29 */
	DBGLOG(REQ, DEBUG, "====DW29~30====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tDISP_POL[0~7]:%d/%d/%d/%d/%d/%d/%d/%d\n",
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy0,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy1,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy2,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy3,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy4,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy5,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy6,
		pWtbl->mlo_info.wtbl_d29.field.dispatch_policy7);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tEML[SR0/MR0/SR1/MR1/SR2/MR2]:%d/%d/%d/%d/%d/%d\n",
		pWtbl->mlo_info.wtbl_d29.field.emlsr0,
		pWtbl->mlo_info.wtbl_d29.field.emlmr0,
		pWtbl->mlo_info.wtbl_d29.field.emlsr1,
		pWtbl->mlo_info.wtbl_d29.field.emlmr1,
		pWtbl->mlo_info.wtbl_d29.field.emlsr2,
		pWtbl->mlo_info.wtbl_d29.field.emlmr2);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tMLD_ID:%d STR_BMAP:0x%x/\n",
		pWtbl->mlo_info.wtbl_d29.field.own_mld_id,
		pWtbl->mlo_info.wtbl_d29.field.str_bitmap);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tMGF:%d DISP[ORDER/RATIO]:%d/%d\n",
		pWtbl->mlo_info.wtbl_d30.field.link_mgf,
		pWtbl->mlo_info.wtbl_d30.field.dispatch_order,
		pWtbl->mlo_info.wtbl_d30.field.dispatch_ratio);

	/* DW34 */
	DBGLOG(REQ, DEBUG, "====DW34~35====\n");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tRSSI= %d %d %d %d\n",
		RCPI_TO_dBm(pWtbl->rx_stat.wtbl_d34.field.resp_rcpi_0),
		RCPI_TO_dBm(pWtbl->rx_stat.wtbl_d34.field.resp_rcpi_1),
		RCPI_TO_dBm(pWtbl->rx_stat.wtbl_d34.field.resp_rcpi_2),
		RCPI_TO_dBm(pWtbl->rx_stat.wtbl_d34.field.resp_rcpi_3));
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tSNR= %d %d %d %d\n",
		WTBL_SNR_MAPPING(pWtbl->rx_stat.wtbl_d35.field.snr_rx0),
		WTBL_SNR_MAPPING(pWtbl->rx_stat.wtbl_d35.field.snr_rx1),
		WTBL_SNR_MAPPING(pWtbl->rx_stat.wtbl_d35.field.snr_rx2),
		WTBL_SNR_MAPPING(pWtbl->rx_stat.wtbl_d35.field.snr_rx3));

	i4BytesWritten = connac5x_wtbl_rate_to_string(
		pcCommand, i4TotalLen, 1, pWtbl, i4BytesWritten);

	return i4BytesWritten;
}

static void connac5x_print_wtbl_info(
	struct ADAPTER *prAdapter,
	int32_t idx,
	struct bwtbl_lmac_struct *pWtbl)
{
	LOG_FUNC(
	"====DW8====\n"
	"\tRtsFailCnt AC[0~3]:%d/%d/%d/%d\n",
		pWtbl->trx_cap.wtbl_d8.field.rts_fail_cnt_ac0,
		pWtbl->trx_cap.wtbl_d8.field.rts_fail_cnt_ac1,
		pWtbl->trx_cap.wtbl_d8.field.rts_fail_cnt_ac2,
		pWtbl->trx_cap.wtbl_d8.field.rts_fail_cnt_ac3
	);

	LOG_FUNC(
	"====DW14~19====\n"
	"\tRate1[TX/FAIL] TXOK[RATE2/RATE3]= %d/%d/%d/%d\n"
	"\tCUR_BW[TX/FAIL] OTHER_BW[TX/FAIL]= %d/%d/%d/%d\n"
	"\tRTS[OK/FAIL] RETRY[DATA/MGNT]= %d/%d/%d/%d\n",
		pWtbl->auto_rate_counters.wtbl_d14.field.rate_1_tx_cnt,
		pWtbl->auto_rate_counters.wtbl_d14.field.rate_1_fail_cnt,
		pWtbl->auto_rate_counters.wtbl_d15.field.rate_2_ok_cnt,
		pWtbl->auto_rate_counters.wtbl_d15.field.rate_3_ok_cnt,
		pWtbl->auto_rate_counters.wtbl_d16.field.current_bw_tx_cnt,
		pWtbl->auto_rate_counters.wtbl_d16.field.current_bw_fail_cnt,
		pWtbl->auto_rate_counters.wtbl_d17.field.other_bw_tx_cnt,
		pWtbl->auto_rate_counters.wtbl_d17.field.other_bw_fail_cnt,
		pWtbl->ppdu_counters.wtbl_d18.field.rts_ok_cnt,
		pWtbl->ppdu_counters.wtbl_d18.field.rts_fail_cnt,
		pWtbl->ppdu_counters.wtbl_d19.field.data_retry_cnt,
		pWtbl->ppdu_counters.wtbl_d19.field.mgnt_retry_cnt
	);

	LOG_FUNC(
	"====DW31====\n"
	"\tBFTX_TB/CASCAD/ALL_ACK/DROP/MPDU_SZ:%d/%d/%d/%d/%d\n"
	"\tRXD_DUP_MODE/ACK_EN:%d/%d\n",
		pWtbl->resp_info.wtbl_d31.field.bftx_tb,
		pWtbl->resp_info.wtbl_d31.field.cascad,
		pWtbl->resp_info.wtbl_d31.field.all_ack,
		pWtbl->resp_info.wtbl_d31.field.drop,
		pWtbl->resp_info.wtbl_d31.field.mpdu_size,
		pWtbl->resp_info.wtbl_d31.field.rxd_dup_mode,
		pWtbl->resp_info.wtbl_d31.field.ack_en
	);
	LOG_FUNC(
	"====DW32====\n"
	"\tOM_INFO_HE:%d OM_INFO_EHT:%d\n"
	"\tRXD_DUP[W_LIST/FROM_OM_CHG]:%d/%d\n",
		pWtbl->rx_dup_info.wtbl_d32.field.om_info,
		pWtbl->rx_dup_info.wtbl_d32.field.om_info_eht,
		pWtbl->rx_dup_info.wtbl_d32.field.rxd_dup_white_list,
		pWtbl->rx_dup_info.wtbl_d32.field.rxd_dup_from_om_chg
	);

	LOG_FUNC(
	"====DW33====\n"
	"\tUSER_RSSI:%d USER_SNR:%d\n"
	"\tRAPID_REACTION_RATE:%d HT_AMSDU:%d AMSDU_CROSS_LG:%d\n",
		pWtbl->rx_stat.wtbl_d33.field.user_rssi,
		pWtbl->rx_stat.wtbl_d33.field.user_snr,
		pWtbl->rx_stat.wtbl_d33.field.rapid_reaction_rate,
		pWtbl->rx_stat.wtbl_d33.field.ht_amsdu,
		pWtbl->rx_stat.wtbl_d33.field.amsdu_cross_lg);
}

int32_t connac5x_show_wtbl_info(
	struct ADAPTER *prAdapter,
	uint32_t u4Index,
	char *pcCommand,
	int i4TotalLen)
{
	uint8_t *wtbl_raw_dw = NULL;
	struct bwtbl_lmac_struct *pwtbl;
	int32_t i4BytesWritten = 0;

	wtbl_raw_dw = (uint8_t *)kalMemAlloc(
		sizeof(struct bwtbl_lmac_struct), VIR_MEM_TYPE);
	if (!wtbl_raw_dw) {
		DBGLOG(REQ, ERROR, "WTBL : Memory alloc failed\n");
		return 0;
	}

	connac5x_get_lwtbl(prAdapter, u4Index, wtbl_raw_dw);

	pwtbl = (struct bwtbl_lmac_struct *)wtbl_raw_dw;
	i4BytesWritten = connac5x_dump_helper_wtbl_info(
		prAdapter,
		pcCommand,
		i4TotalLen,
		pwtbl,
		u4Index);

	/* print more info in log */
	connac5x_print_wtbl_info(prAdapter, u4Index, pwtbl);

	kalMemFree(wtbl_raw_dw, VIR_MEM_TYPE,
			sizeof(struct bwtbl_lmac_struct));

	return i4BytesWritten;
}

void connac5x_get_lwtbl(
	struct ADAPTER *prAdapter,
	uint32_t u4Index,
	uint8_t *wtbl_raw_dw
)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4Value = 0;
	uint32_t wtbl_lmac_baseaddr;
	uint32_t wtbl_offset, addr;

	prChipInfo = prAdapter->chip_info;
	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter,
		DRV_OWN_SRC_GET_LWTBL);
	CONNAC5X_LWTBL_CONFIG(prAdapter, prChipInfo->u4LmacWtblDUAddr, u4Index);
	wtbl_lmac_baseaddr = CONNAC5X_LWTBL_IDX2BASE(
		prChipInfo->u4LmacWtblDUAddr, u4Index, 0);

	HAL_RMCR_RD(WTBL_DBG, prAdapter, prChipInfo->u4LmacWtblDUAddr,
				&u4Value);

	DBGLOG(REQ, DEBUG, "LMAC WTBL Addr: group: 0x%x=0x%x addr: 0x%x\n",
		prChipInfo->u4LmacWtblDUAddr,
		u4Value,
		wtbl_lmac_baseaddr);

	/* Read LWTBL Entries */
	for (wtbl_offset = 0; wtbl_offset <
		sizeof(struct bwtbl_lmac_struct);
		wtbl_offset += 4) {
		addr = wtbl_lmac_baseaddr + wtbl_offset;
		HAL_RMCR_RD(WTBL_DBG, prAdapter, addr,
			   &u4Value);
		kalMemCopy(
			(uint32_t *)&wtbl_raw_dw[wtbl_offset],
			&u4Value, sizeof(uint32_t));
	}
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_GET_LWTBL);
}

void connac5x_get_rssi_from_wtbl(
	struct ADAPTER *prAdapter, uint32_t u4Index,
	int32_t *pi4Rssi0, int32_t *pi4Rssi1,
	int32_t *pi4Rssi2, int32_t *pi4Rssi3
)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo;

	struct CMD_ACCESS_REG rCmdAccessReg = {0};
	uint32_t rStatus;
	uint32_t u4BufLen = 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	DBGLOG(REQ, DEBUG, "WTBL : index = %d\n", u4Index);

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter,
		DRV_OWN_SRC_GET_RSSI_FROM_WTBL);
	rCmdAccessReg.u4Address = CONNAC5X_LWTBL_IDX2BASE(
		prChipInfo->u4LmacWtblDUAddr, u4Index, 34);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_GET_RSSI_FROM_WTBL);

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryMcrRead,
			   &rCmdAccessReg, sizeof(rCmdAccessReg),
			   &u4BufLen);
	if (rStatus == WLAN_STATUS_SUCCESS) {
		if (pi4Rssi0 != NULL)
			*pi4Rssi0 = RCPI_TO_dBm((rCmdAccessReg.u4Data &
				WTBL_RESP_RCPI0_MASK) >>
				WTBL_RESP_RCPI0_OFFSET);
		if (pi4Rssi1 != NULL)
			*pi4Rssi1 = RCPI_TO_dBm((rCmdAccessReg.u4Data &
				WTBL_RESP_RCPI1_MASK) >>
				WTBL_RESP_RCPI1_OFFSET);
		if (pi4Rssi2 != NULL)
			*pi4Rssi2 = RCPI_TO_dBm((rCmdAccessReg.u4Data &
				WTBL_RESP_RCPI2_MASK) >>
				WTBL_RESP_RCPI2_OFFSET);
		if (pi4Rssi3 != NULL)
			*pi4Rssi3 = RCPI_TO_dBm((rCmdAccessReg.u4Data &
				WTBL_RESP_RCPI3_MASK) >>
				WTBL_RESP_RCPI3_OFFSET);
	} else {
		if (pi4Rssi0 != NULL)
			*pi4Rssi0 = 0;
		if (pi4Rssi1 != NULL)
			*pi4Rssi1 = 0;
		if (pi4Rssi2 != NULL)
			*pi4Rssi2 = 0;
		if (pi4Rssi3 != NULL)
			*pi4Rssi3 = 0;
	}
}

static bool is_wtbl_bigtk_exist(struct ADAPTER *prAdapter, uint32_t u4Index)
{
#ifdef WF_DS_LWTBL_BASE
	struct mt66xx_chip_info *prChipInfo;
	uint32_t wtbl_lmac_baseaddr;
	uint32_t dw_value = 0;

	prChipInfo = prAdapter->chip_info;
	/* LMAC */
	CONNAC5X_LWTBL_CONFIG(prAdapter, prChipInfo->u4LmacWtblDUAddr, u4Index);
	wtbl_lmac_baseaddr = CONNAC5X_LWTBL_IDX2BASE(
		prChipInfo->u4LmacWtblDUAddr, u4Index, 0);
	HAL_RMCR_RD(WTBL_DBG, prAdapter,
		prChipInfo->u4LmacWtblDUAddr + WF_LWTBL_MUAR_DW * 4,
				&dw_value);

	if (((dw_value & WF_LWTBL_MUAR_MASK) >> WF_LWTBL_MUAR_SHIFT) ==
					MUAR_INDEX_OWN_MAC_ADDR_BC_MC) {
		HAL_RMCR_RD(WTBL_DBG, prAdapter,
			(prChipInfo->u4LmacWtblDUAddr +
				WF_LWTBL_CIPHER_SUIT_BIGTK_DW * 4),
			&dw_value);
		if (((dw_value & WF_LWTBL_CIPHER_SUIT_BIGTK_MASK) >>
			WF_LWTBL_CIPHER_SUIT_BIGTK_SHIFT) !=
			IGTK_CIPHER_SUIT_NONE)
			return TRUE;
	}
#endif

	return FALSE;
}

static void dump_key_table(
	struct ADAPTER *prAdapter,
	uint16_t keyloc0,
	uint16_t keyloc1,
	uint16_t keyloc2)
{
	uint8_t keytbl[ONE_KEY_ENTRY_LEN_IN_DW*4] = {0};
	uint16_t x;
	uint32_t u4Value = 0;

	DBGLOG(HAL, DEBUG, "\t\n");
	DBGLOG(HAL, DEBUG, "\t%s:%d\n", "keyloc0", keyloc0);
	if (keyloc0 != INVALID_KEY_ENTRY) {

		/* Don't swap below two lines, halWtblReadRaw will
		* write new value WF_WTBLON_TOP_WDUCR_ADDR
		*/
		halWtblReadRaw(prAdapter, keyloc0,
			WTBL_TYPE_KEY, 0, ONE_KEY_ENTRY_LEN_IN_DW, keytbl);
		HAL_RMCR_RD(WTBL_DBG, prAdapter,
			       WF_UWTBL_TOP_WDUCR_ADDR, &u4Value);
		DBGLOG(HAL, DEBUG,
		       "\t\tKEY WTBL Addr: group:0x%x=0x%x addr: 0x%x\n",
		       WF_UWTBL_TOP_WDUCR_ADDR,
		       u4Value,
		       KEYTBL_IDX2BASE(keyloc0, 0));
		for (x = 0; x < ONE_KEY_ENTRY_LEN_IN_DW; x++) {
			DBGLOG(HAL, DEBUG, "\t\tDW%02d: %02x %02x %02x %02x\n",
				x,
				keytbl[x * 4 + 3],
				keytbl[x * 4 + 2],
				keytbl[x * 4 + 1],
				keytbl[x * 4]);
		}
	}

	DBGLOG(HAL, DEBUG, "\t%s:%d\n", "keyloc1", keyloc1);
	if (keyloc1 != INVALID_KEY_ENTRY) {
		/* Don't swap below two lines, halWtblReadRaw will
		* write new value WF_WTBLON_TOP_WDUCR_ADDR
		*/
		halWtblReadRaw(prAdapter, keyloc1,
			WTBL_TYPE_KEY, 0, ONE_KEY_ENTRY_LEN_IN_DW, keytbl);
		HAL_RMCR_RD(WTBL_DBG, prAdapter,
			       WF_UWTBL_TOP_WDUCR_ADDR, &u4Value);
		DBGLOG(HAL, DEBUG,
		       "\t\tKEY WTBL Addr: group:0x%x=0x%x addr: 0x%x\n",
		       WF_UWTBL_TOP_WDUCR_ADDR,
		       u4Value,
		       KEYTBL_IDX2BASE(keyloc1, 0));
		for (x = 0; x < ONE_KEY_ENTRY_LEN_IN_DW; x++) {
			DBGLOG(HAL, DEBUG, "\t\tDW%02d: %02x %02x %02x %02x\n",
				x,
				keytbl[x * 4 + 3],
				keytbl[x * 4 + 2],
				keytbl[x * 4 + 1],
				keytbl[x * 4]);
		}
	}

	DBGLOG(HAL, DEBUG, "\t%s:%d\n", "keyloc2", keyloc2);
	if (keyloc2 != INVALID_KEY_ENTRY) {
		/* Don't swap below two lines, halWtblReadRaw will
		* write new value WF_WTBLON_TOP_WDUCR_ADDR
		*/
		halWtblReadRaw(prAdapter, keyloc2,
			WTBL_TYPE_KEY, 0, ONE_KEY_ENTRY_LEN_IN_DW, keytbl);
		HAL_RMCR_RD(WTBL_DBG, prAdapter,
			       WF_UWTBL_TOP_WDUCR_ADDR, &u4Value);
		DBGLOG(HAL, DEBUG,
		       "\t\tKEY WTBL Addr: group:0x%x=0x%x addr: 0x%x\n",
		       WF_UWTBL_TOP_WDUCR_ADDR,
		       u4Value,
		       KEYTBL_IDX2BASE(keyloc2, 0));
		for (x = 0; x < ONE_KEY_ENTRY_LEN_IN_DW; x++) {
			DBGLOG(HAL, DEBUG, "\t\tDW%02d: %02x %02x %02x %02x\n",
				x,
				keytbl[x * 4 + 3],
				keytbl[x * 4 + 2],
				keytbl[x * 4 + 1],
				keytbl[x * 4]);
		}
	}
}

int32_t connac5x_show_umac_wtbl_info(
	struct ADAPTER *prAdapter,
	uint32_t u4Index,
	char *pcCommand,
	int i4TotalLen)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4Value = 0;
	uint32_t wtbl_umac_baseaddr;
	uint32_t wtbl_offset, addr;
	unsigned char *wtbl_raw_dw = NULL;
	struct bwtbl_umac_struct *puwtbl;
	int32_t i4BytesWritten = 0;
	uint8_t aucPA[MAC_ADDR_LEN];
	unsigned long long pn = 0;
	uint16_t keyloc2 = INVALID_KEY_ENTRY;
	uint32_t amsdu_len = 0;

	DBGLOG(HAL, DEBUG, "UMAC WTBL Addr: group:0x%x=0x%x addr: 0x%x\n",
		WF_UWTBL_TOP_WDUCR_ADDR,
		u4Value,
		UWTBL_IDX2BASE(u4Index, 0));

	prChipInfo = prAdapter->chip_info;
	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter,
		DRV_OWN_SRC_SHOW_UMAC_WTBL_INFO);
	/* UMAC */
	CONNAC5X_UWTBL_CONFIG(prAdapter, prChipInfo->u4UmacWtblDUAddr, u4Index);
	wtbl_umac_baseaddr = CONNAC5X_UWTBL_IDX2BASE(
		prChipInfo->u4UmacWtblDUAddr, u4Index, 0);
	HAL_RMCR_RD(WTBL_DBG, prAdapter,
		       prChipInfo->u4UmacWtblDUAddr, &u4Value);
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
		"UMAC WTBL Addr: group: 0x%x=0x%x addr: 0x%x\n",
		prChipInfo->u4UmacWtblDUAddr,
		u4Value,
		wtbl_umac_baseaddr);

	wtbl_raw_dw = (unsigned char *)kalMemAlloc(
		sizeof(struct bwtbl_umac_struct), VIR_MEM_TYPE);
	if (!wtbl_raw_dw) {
		DBGLOG(REQ, ERROR, "WTBL : Memory alloc failed\n");
		return 0;
	}
	/* Read UWTBL Entries */
	for (wtbl_offset = 0; wtbl_offset <
		sizeof(struct bwtbl_umac_struct);
		wtbl_offset += 4) {
		addr = wtbl_umac_baseaddr + wtbl_offset;
		HAL_RMCR_RD(WTBL_DBG, prAdapter, addr,
			   &u4Value);
		kalMemCopy(
			(uint32_t *)&wtbl_raw_dw[wtbl_offset],
			&u4Value, sizeof(uint32_t));
	}
	puwtbl = (struct bwtbl_umac_struct *)wtbl_raw_dw;

	/* UMAC WTBL DW 0,1 */
	aucPA[0] =
		puwtbl->mlo_info.wtbl_d1.field.peer_mld_addr & 0xff;
	aucPA[1] =
		((puwtbl->mlo_info.wtbl_d1.field.peer_mld_addr &
			0xff00) >> 8);
	aucPA[2] =
		((puwtbl->mlo_info.wtbl_d1.field.peer_mld_addr &
			0xff0000) >> 16);
	aucPA[3] =
		((puwtbl->mlo_info.wtbl_d1.field.peer_mld_addr &
			0xff000000) >> 24);
	aucPA[4] =
		puwtbl->mlo_info.wtbl_d0.field.peer_mld_addr & 0xff;
	aucPA[5] =
		((puwtbl->mlo_info.wtbl_d0.field.peer_mld_addr &
			0xff00) >> 8);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"\tPEER_MLD_ADDR="MACSTR"\n",
		MAC2STR(aucPA));

	pn = (((unsigned long long)(puwtbl->pn_sn.wtbl_d3.field.pn1) << 32)
		| puwtbl->pn_sn.wtbl_d2.field.pn0);

	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s\n",
		"UWTBL DW 2~3");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
		i4BytesWritten,
		"\tpn:%llu com_sn:%d\n",
		pn,
		(uint32_t)puwtbl->pn_sn.wtbl_d3.field.com_sn);

	/*DW4*/
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s\n",
		"UWTBL DW 4~6");
	if (is_wtbl_bigtk_exist(prAdapter, u4Index)) {
		keyloc2 = puwtbl->pn_sn.wtbl_d6.field_v2.key_loc2;
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRX_BIPN:%llu Key_lock2:%d\n",
			((unsigned long long)(
				puwtbl->pn_sn.wtbl_d5.field_v2.rx_bipn1) <<
					32) |
				puwtbl->pn_sn.wtbl_d4.field_v2.rx_bipn0,
			puwtbl->pn_sn.wtbl_d6.field_v2.key_loc2);
	} else {
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tSN TID0:%d TID1:%d TID2:%d TID3:%d\n",
			puwtbl->pn_sn.wtbl_d4.field.ac0_sn,
			puwtbl->pn_sn.wtbl_d4.field.ac1_sn,
			puwtbl->pn_sn.wtbl_d4.field.ac2_sn |
				puwtbl->pn_sn.wtbl_d5.field.ac2_sn << 8,
			puwtbl->pn_sn.wtbl_d5.field.ac3_sn);
		i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tSN TID4:%d TID5:%d TID6:%d TID7:%d\n",
			puwtbl->pn_sn.wtbl_d5.field.ac4_sn,
			puwtbl->pn_sn.wtbl_d5.field.ac5_sn |
				puwtbl->pn_sn.wtbl_d6.field.ac5_sn << 4,
			puwtbl->pn_sn.wtbl_d6.field.ac6_sn,
			puwtbl->pn_sn.wtbl_d6.field.ac7_sn);
	}

	/* UMAC WTBL DW 7 */
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s\n",
		"UWTBL DW 7");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tKey_loc0:%d Key_loc1:%d\n",
			puwtbl->key_msdu_mlo.wtbl_d7.field.key_loc0,
			puwtbl->key_msdu_mlo.wtbl_d7.field.key_loc1);

	/* Dump key table in log */
	dump_key_table(prAdapter,
		puwtbl->key_msdu_mlo.wtbl_d7.field.key_loc0,
		puwtbl->key_msdu_mlo.wtbl_d7.field.key_loc1,
		keyloc2);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_SHOW_UMAC_WTBL_INFO);

	/* UMAC WTBL DW 8 */
	amsdu_len = puwtbl->key_msdu_mlo.wtbl_d8.field.amsdu_len;
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s\n",
		"UWTBL DW 8");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tAMSDU_CFG EN:%d NUM:%d(WTBL value=0x%x)\n",
			puwtbl->key_msdu_mlo.wtbl_d8.field.amsdu_en,
			puwtbl->key_msdu_mlo.wtbl_d8.field.amsdu_num + 1,
			puwtbl->key_msdu_mlo.wtbl_d8.field.amsdu_num);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\t%s:%d~%d (WTBL value=0x%x)\n",
			"HW AMSDU Len",
			256 * amsdu_len,
			256 * (amsdu_len + 1) - 1,
			amsdu_len);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tSEC_ADDR_MODE:%d SPP_EN:%d WPI_EVEN:%d AAD_OM:%d\n",
			puwtbl->key_msdu_mlo.wtbl_d8.field.sec_addr_mode,
			puwtbl->key_msdu_mlo.wtbl_d8.field.spp_en,
			puwtbl->key_msdu_mlo.wtbl_d8.field.wpi_even,
			puwtbl->key_msdu_mlo.wtbl_d8.field.aad_om);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tWMM_Q:%d QoS:%d HT:%d HDRT_MODE:%d\n",
			puwtbl->key_msdu_mlo.wtbl_d8.field.wmm_q,
			puwtbl->key_msdu_mlo.wtbl_d8.field.qos,
			puwtbl->key_msdu_mlo.wtbl_d8.field.ht,
			puwtbl->key_msdu_mlo.wtbl_d8.field.hdrt_mode);

	/* UMAC WTBL DW 9 */
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten,
		"%s\n",
		"UWTBL DW 9");
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRELATED_IDX0:%d RELATED_BN0:%d PRI_MLD_BN:%d\n",
			puwtbl->key_msdu_mlo.wtbl_d9.field.related_idx0,
			puwtbl->key_msdu_mlo.wtbl_d9.field.related_band0,
			puwtbl->key_msdu_mlo.wtbl_d9.field.pri_mld_band);
	i4BytesWritten = SHOW_DBGLOG(pcCommand, i4TotalLen,
			i4BytesWritten,
			"\tRELATED_IDX1:%d RELATED_BN1:%d SEC_MLD_BN:%d\n",
			puwtbl->key_msdu_mlo.wtbl_d9.field.related_idx1,
			puwtbl->key_msdu_mlo.wtbl_d9.field.related_band1,
			puwtbl->key_msdu_mlo.wtbl_d9.field.sec_mld_band);

	kalMemFree(wtbl_raw_dw, VIR_MEM_TYPE,
		sizeof(struct bwtbl_umac_struct));

	return i4BytesWritten;
}

#endif /* DBG_DISABLE_ALL_INFO */
#endif /* CFG_SUPPORT_CONNAC5X */
