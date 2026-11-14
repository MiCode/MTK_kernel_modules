// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file dvt_common.c
 *    \brief This file contains the declairations of sys dvt command
 */

/*******************************************************************************
 *                    C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                    F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                    P R I V A T E	 D A T A
 *******************************************************************************
 */
struct AUTOMATION_DVT automation_dvt;
struct TXS_FREE_LIST_POOL TxsFreeEntrylist;

#if CFG_SUPPORT_WIFI_SYSDVT

const struct _MDVT_MODULE_T arMdvtModuleTable[] = {
	{MDVT_MODULE_WFARB,                     "arb"},
	{MDVT_MODULE_AGG,                       "agg"},
	{MDVT_MODULE_DMA,                       "dma"},
	{MDVT_MODULE_WFMIMO,                    "mimo"},
	{MDVT_MODULE_WFCTRL,                    "ctrl"},
	{MDVT_MODULE_WFETXBF,                   "etxbf"},
	{MDVT_MODULE_WFCFG,                     "cfg"},
	{MDVT_MODULE_WFHIF,                     "hif"},
	{MDVT_MODULE_WFOFF,                     "off"},
	{MDVT_MODULE_WFON,                      "on"},
	{MDVT_MODULE_WFPF,                      "pf"},
	{MDVT_MODULE_WFRMAC,                    "rmac"},
	{MDVT_MODULE_WFUMAC_PLE,                "ple"},
	{MDVT_MODULE_WFUMAC_PSE,                "pse"},
	{MDVT_MODULE_WFUMAC_PP,                 "pp"},
	{MDVT_MODULE_WFUMAC_AMSDU,              "amsdu"},
	{MDVT_MODULE_WFSEC,                     "sec"},
	{MDVT_MODULE_WFTMAC,                    "tmac"},
	{MDVT_MODULE_WFTMAC_TXPWR,              "txpwr"},
	{MDVT_MODULE_WFTXCR,                    "txcr"},
	{MDVT_MODULE_WFMIB,                     "mib"},
	{MDVT_MODULE_WFSYSON,                   "syson"},
	{MDVT_MODULE_WFLPON,                    "lpon"},
	{MDVT_MODULE_WFINT,                     "int"},
	{MDVT_MODULE_CONNCFG,                   "conncfg"},
	{MDVT_MODULE_MUCOP,                     "mucop"},
	{MDVT_MODULE_WFMDP,                     "mdp"},
	{MDVT_MODULE_WFRDM_PHYRX,               "rdm_phyrx"},
	{MDVT_MODULE_WFRDM_PHYDFS,              "rdm_phydfs"},
	{MDVT_MODULE_WFRDM_PHYRX_COMM,          "rdm_phyrx_comm"},
	{MDVT_MODULE_WFRDM_WTBLOFF,             "rdm_wtbloff"},
	{MDVT_MODULE_PHYDFE_CTRL_WF_TSSI,       "phydfe_ctrl_wf_tssi"},
	{MDVT_MODULE_PHYDFE_RFINTF_WF_CMM,      "phydfe_rfintf_wf_cmm"},
	{MDVT_MODULE_PHYRX_CTRL_WF_COMM_RDD,    "phyrx_ctrl_wf_comm_rdd"},
	{MDVT_MODULE_PHYRX_CTRL_WF_COMM_CSI,    "phyrx_ctrl_wf_comm_csi"},
	{MDVT_MODULE_PHYRX_CTRL_WF_COMM_CMM,    "phyrx_ctrl_wf_comm_cmm"},
	{MDVT_MODULE_PHYRX_CTRL_WF_COMM_TOAE,   "phyrx_ctrl_wf_comm_toae"},
	{MDVT_MODULE_PHYRX_CSD_WF_COMM_CMM,     "phyrx_csd_wf_comm_cmm"},
	{MDVT_MODULE_PHYRX_POST_CMM,            "phyrx_post_cmm"},
	{MDVT_MODULE_PHYDFS_WF_COMM_RDD,        "phydfs_wf_comm_rdd"},
	{MDVT_MODULE_PHYRX_CTRL_TOAE,           "phyrx_ctrl_toae"},
	{MDVT_MODULE_PHYRX_CTRL_MURU,           "phyrx_ctrl_muru"},
	{MDVT_MODULE_PHYRX_CTRL_RDD,            "phyrx_ctrl_rdd"},
	{MDVT_MODULE_PHYRX_CTRL_MULQ,           "phyrx_ctrl_mulq"},
	{MDVT_MODULE_PHYRX_CTRL_CMM,            "phyrx_ctrl_cmm"},
	{MDVT_MODULE_PHYRX_CTRL_CSI,            "phyrx_ctrl_csi"},
	{MDVT_MODULE_PHYDFE_CTRL_PWR_REGU,      "phydfe_ctrl_pwr_regu"},
	{MDVT_MODULE_PHYRX_CTRL_BF,             "phyrx_ctrl_bf"},
	{MDVT_MODULE_PHYDFE_CTRL_CMM,           "phydfe_ctrl_cmm"},
	{MDVT_MODULE_WFRBIST,                   "rbist"},
	{MDVT_MODULE_WTBL,                      "wtbl"},
	{MDVT_MODULE_RX,                        "rx"},
	{MDVT_MODULE_LPON,                      "lpon"},
	{MDVT_MODULE_MDP_RX,                    "mdprx"},
	{MDVT_MODULE_TXCMD,                     "txcmd"},
	{MDVT_MODULE_SEC_ECC,                   "sec_ecc"},
	{MDVT_MODULE_MIB,                       "mib"},
	{MDVT_MODULE_WFTWT,                     "twt"},
	{MDVT_MODULE_DRR,                       "drr"},
	{MDVT_MODULE_RUOFDMA,                   "ruofdma"},
	{MDVT_MODULE_WFCMDRPTTX,                "cmdrpttx"},
	{MDVT_MODULE_WFCMDRPT_TRIG,             "cmdrpttrig"},
	{MDVT_MODULE_MLO,                       "mlo"},
	{MDVT_MODULE_TXD,                       "txd"},
	{MDVT_MODULE_PH_TPUT,                   "tput"},
	{MDVT_MODULE_SER,                       "ser"},
	{MDVT_MODULE_LIT_WTBL,                  "lit_wtbl"},
	{MDVT_MODULE_LITMIB,                    "lit_mib"},
	{MDVT_MODULE_LIT_WFRMAC,                "lit_rmac"},
	{MDVT_MODULE_PTA_IDC_COEX,              "pta_idc_coex"},
	{MDVT_MODULE_MUMIMO,                    "mumimo"},
	{MDVT_MODULE_PRMBPUNC,                  "prmbpunc"},
	{MDVT_MODULE_CERT,                      "cert"},
	{MDVT_MODULE_SR,                        "sr"},
	{MDVT_MODULE_ARB_COEX,                  "arb_coex"},
	{MDVT_MODULE_BF,                        "bf"},
	{MDVT_MODULE_CMD_DECODER,               "cmd_decoder"},
	{MDVT_MODULE_COSIM,                     "cosim"},
	{MDVT_MODULE_RLM_CMM,                   "rlm_cmm"},
	{MDVT_MODULE_WFDMA,                     "wfdma"},
	{MDVT_MODULE_SDO,                       "sdo"},
	{MDVT_MODULE_RRO,                       "rro"},
	{MDVT_MODULE_AIRTIME,                   "airtime"},
	{MDVT_MODULE_BFTXD,                     "bftxd"},
	{MDVT_MODULE_PRMBPUNC_TXD,              "prmbpunc_txd"},
	{MDVT_MODULE_UMAC_CFG,                  "umac_cfg"},
	{MDVT_MODULE_FDD_COEX,                  "fdd_coex"},
	{MDVT_MODULE_MAX,                       "all"}
};

uint32_t u4MdvtTableSize = ARRAY_SIZE(arMdvtModuleTable);


#endif /*CFG_SUPPORT_WIFI_SYSDVT*/

/*******************************************************************************
 *                    F U N C T I O N S
 *******************************************************************************
 */

#if CFG_SUPPORT_WIFI_SYSDVT
/*
* This routine is used to init TXS pool for DVT
*/
void TxsPoolInit(void)
{
	if (TxsFreeEntrylist.txs_list_cnt == 0) {
		spin_lock_init(&TxsFreeEntrylist.Lock);
		INIT_LIST_HEAD(&TxsFreeEntrylist.pool_head.List);
		INIT_LIST_HEAD(&TxsFreeEntrylist.head.mList);
	}

	TxsFreeEntrylist.txs_list_cnt++;
}

/*
* This routine is used to uninit TXS pool for DVT
*/
void TxsPoolUnInit(void)
{
	struct list_head *prCur, *prNext;

	TxsFreeEntrylist.txs_list_cnt--;

	if (TxsFreeEntrylist.txs_list_cnt == 0) {
		struct TXS_LIST_POOL *pEntry = NULL;

		list_for_each_safe(prCur, prNext,
		&TxsFreeEntrylist.pool_head.List) {
			pEntry =
				list_entry(prCur,  struct TXS_LIST_POOL, List);
			list_del_init(&pEntry->List);
			list_del(prCur);
			if (pEntry == NULL)
				DBGLOG(REQ, LOUD, "pEntry null\n");
			kfree(pEntry);
		}
	}
}

/*
* This routine is used to test TXS function.
* init TXS DVT structure and start to test
*/
bool TxsInit(void)
{
	uint32_t i;
	struct TXS_LIST *list = &automation_dvt.txs.txs_list;

	if (automation_dvt.txs.isinit)
		return TRUE;

	automation_dvt.txs.isinit = FALSE;
	automation_dvt.txs.total_req = 0;
	automation_dvt.txs.total_rsp = 0;
	automation_dvt.txs.stop_send_test = TRUE;
	automation_dvt.txs.test_type = 0;
	automation_dvt.txs.pid = 1;

	spin_lock_init(&list->lock);

	for (i = 0; i < PID_SIZE; i++) {
		INIT_LIST_HEAD(&list->pHead[i].mList);
		automation_dvt.txs.check_item[i].time_stamp = 0;
	}

	list->Num = 0;
	TxsPoolInit();

	if (list_empty(&TxsFreeEntrylist.pool_head.List)) {
		struct TXS_LIST_POOL *Pool = NULL;
		struct TXS_LIST_POOL *pFreepool = NULL;
		struct TXS_LIST_ENTRY *pEntry = NULL;
		struct TXS_LIST_ENTRY *newEntry = NULL;

		Pool = kmalloc(sizeof(struct TXS_LIST_POOL), GFP_ATOMIC);
		pFreepool = &TxsFreeEntrylist.pool_head;
		list_add(&Pool->List, &pFreepool->List);
		pEntry = &TxsFreeEntrylist.head;

		for (i = 0; i < TXS_LIST_ELEM_NUM; i++) {
			newEntry = &Pool->Entry[i];
			list_add(&newEntry->mList, &pEntry->mList);
		}
	}

	list->pFreeEntrylist = &TxsFreeEntrylist;
	automation_dvt.txs.isinit = TRUE;
	return TRUE;
}

/*
* This routine is used to test TXS function.
* destroy TXS DVT structure
*/
bool TxsExit(void)
{
	uint32_t i = 0;
	unsigned long IrqFlags = 0;
	uint16_t wait_cnt = 0;
	struct TXS_LIST *list = &automation_dvt.txs.txs_list;

	automation_dvt.txs.isinit = FALSE;
	automation_dvt.txs.total_req = 0;
	automation_dvt.txs.total_rsp = 0;
	automation_dvt.txs.stop_send_test = TRUE;
	automation_dvt.txs.test_type = 0;
	automation_dvt.txs.pid = 1;

	while (automation_dvt.txs.txs_list.Num > 0) {
		DBGLOG(REQ, LOUD, "wait entry to be deleted\n");
		kalMsleep(100);/* OS_WAIT(10); */
		wait_cnt++;

		if (wait_cnt > 100)
			break;
	}

	spin_lock_irqsave(&list->lock, IrqFlags);

	for (i = 0; i < PID_SIZE; i++) {
		INIT_LIST_HEAD(&list->pHead[i].mList);
		automation_dvt.txs.check_item[i].time_stamp = 0;
	}

	spin_unlock_irqrestore(&list->lock, IrqFlags);
	list->Num = 0;
	TxsPoolUnInit();
	DBGLOG(REQ, LOUD, "TxsPoolUnInit done\n");

	return TRUE;
}

/*
* This routine is used to initial DVT of automation.
*/
bool AutomationInit(struct ADAPTER *pAd, int32_t auto_type)
{
	bool ret;

	ret = TRUE;
	if (!pAd)
		return FALSE;

	DBGLOG(REQ, LOUD, "In AutomationInit\n");

	if (pAd->auto_dvt == NULL) {
		kalMemZero(&automation_dvt, sizeof(struct AUTOMATION_DVT));
		pAd->auto_dvt = &automation_dvt;
		DBGLOG(REQ, LOUD, "AutomationInit\n");
	}

	switch (auto_type) {
	case TXS:
		ret = TxsInit();
		break;
	case RXV:
		break;
#if (CFG_SUPPORT_DMASHDL_SYSDVT)
	case DMASHDL:
		break;
#endif
	case CSO:
		break;
	case SKIP_CH:
		break;
	}

	return ret;
}

struct TXS_LIST_ENTRY *GetTxsEntryFromFreeList(void)
{
	struct TXS_LIST_ENTRY *pEntry = NULL;
	struct TXS_LIST_ENTRY *pheadEntry = NULL;
	struct TXS_FREE_LIST_POOL *pFreeEntrylist = NULL;
	unsigned long IrqFlags = 0;
	uint32_t i;

	pFreeEntrylist =
		automation_dvt.txs.txs_list.pFreeEntrylist;
	if (pFreeEntrylist == NULL)
		return NULL;

	spin_lock_irqsave(&pFreeEntrylist->Lock, IrqFlags);

	if (list_empty(&pFreeEntrylist->head.mList)) {
		struct TXS_LIST_POOL *Pool = NULL;
		struct TXS_LIST_POOL *pFreepool = NULL;

		DBGLOG(REQ, LOUD, "allocated new pool\n");
		Pool = kmalloc(sizeof(struct TXS_LIST_POOL), GFP_ATOMIC);
		pFreepool = &pFreeEntrylist->pool_head;
		list_add(&Pool->List, &pFreepool->List);
		pheadEntry = &pFreeEntrylist->head;

		for (i = 0; i < TXS_LIST_ELEM_NUM; i++) {
			pEntry = &Pool->Entry[i];
			list_add(&pEntry->mList, &pheadEntry->mList);
		}
		pFreeEntrylist->entry_number += TXS_LIST_ELEM_NUM;
	}

	pheadEntry = &pFreeEntrylist->head;
	if (!list_empty(&pheadEntry->mList)) {
		pEntry = list_entry(&pheadEntry->mList,
			struct TXS_LIST_ENTRY, mList);
		list_del(&pEntry->mList);
	}

	if (pEntry != NULL)
		pFreeEntrylist->entry_number -= 1;

	spin_unlock_irqrestore(&pFreeEntrylist->Lock, IrqFlags);
	return pEntry;
}

/*
* This routine is used to test TXS function.
* Send RTS
*/
int SendRTS(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _FRAME_RTS *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;
	unsigned long duration = 0;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ sizeof(struct _FRAME_RTS);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)cnmMgtPktAlloc(prAdapter,
		u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(REQ, WARN,
			"No MSDU_INFO_T for sending dvt RTS Frame.\n");
		return -1;
	}

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_RTS;
	duration = 192 + (uint16_t)((sizeof(struct _FRAME_RTS)<<4)/2);
	if (((sizeof(struct _FRAME_RTS)) << 4)%2)
		duration++;
	prTxFrame->u2Duration = 16 + (uint16_t)duration;
	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			16, sizeof(struct _FRAME_RTS),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* Enqueue the frame to send this control frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
	DBGLOG(REQ, DEBUG, "RTS - Send RTS\n");
	return WLAN_STATUS_SUCCESS;
}

/*
* This routine is used to test TXS function.
* Send BA packet
*/
int SendBA(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	PFN_TX_DONE_HANDLER pfTxDoneHandler)
{
	struct MSDU_INFO *prMsduInfo;
	struct _FRAME_BA *prTxFrame;
	struct BSS_INFO *prBssInfo;
	uint16_t u2EstimatedFrameLen;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	ASSERT(prBssInfo);

	/* Calculate MSDU buffer length */
	u2EstimatedFrameLen = MAC_TX_RESERVED_FIELD
		+ sizeof(struct _FRAME_BA);

	/* Alloc MSDU_INFO */
	prMsduInfo = (struct MSDU_INFO *)
		cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);

	if (!prMsduInfo) {
		DBGLOG(REQ, WARN,
			"No MSDU_INFO_T for sending dvt RTS Frame.\n");
		return -1;
	}

	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	prTxFrame = prMsduInfo->prPacket;

	/* Fill frame ctrl */
	prTxFrame->u2FrameCtrl = MAC_FRAME_BLOCK_ACK;

	COPY_MAC_ADDR(prTxFrame->aucDestAddr, prStaRec->aucMacAddr);
	COPY_MAC_ADDR(prTxFrame->aucSrcAddr, prBssInfo->aucOwnMacAddr);

	/* Compose the frame body's frame */
	prTxFrame->BarControl.ACKPolicy = 1;
	prTxFrame->BarControl.Compressed = 1;
	/* prTxFrame->StartingSeq.field.StartSeq = pBAEntry->LastIndSeq; */

	/* Update information of MSDU_INFO_T */
	TX_SET_MMPDU(prAdapter,
			prMsduInfo,
			prBssInfo->ucBssIndex,
			prStaRec->ucIndex,
			16, sizeof(struct _FRAME_BA),
			pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	/* Enqueue the frame to send this control frame */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);
	DBGLOG(REQ, DEBUG, "BA - Send BA\n");
	return WLAN_STATUS_SUCCESS;
}

bool send_add_txs_queue(uint8_t pid, uint8_t wlan_idx)
{
	struct TXS_LIST *list = &automation_dvt.txs.txs_list;
	uint32_t idx = 0;
	unsigned long IrqFlags = 0;
	struct TXS_LIST_ENTRY *pEntry;
	struct TXS_LIST_ENTRY *pheadEntry;

	automation_dvt.txs.total_req++;

	if (!list || !automation_dvt.txs.isinit) {
		DBGLOG(REQ, WARN, "txs_list doesnot init\n");
		return FALSE;
	}

	spin_lock_irqsave(&list->lock, IrqFlags);

	pEntry = GetTxsEntryFromFreeList();

	if (!pEntry) {
		spin_unlock_irqrestore(&list->lock, IrqFlags);
		DBGLOG(REQ, LOUD, "pEntry is null!!!\n");
		return FALSE;
	}

	idx = automation_dvt.txs.pid % PID_SIZE;
	pheadEntry = &list->pHead[idx];
	pEntry->wlan_idx = wlan_idx;
	list_add(&pEntry->mList, &pheadEntry->mList);
	list->Num++;
	automation_dvt.txs.pid++;

	spin_unlock_irqrestore(&list->lock, IrqFlags);

	return TRUE;
}

bool receive_del_txs_queue(
	uint32_t sn,
	uint8_t pid,
	uint8_t wlan_idx,
	uint32_t time_stamp)
{
	struct TXS_LIST *list = &automation_dvt.txs.txs_list;
	unsigned long IrqFlags = 0;
	unsigned long IrqFlags2 = 0;
	struct TXS_FREE_LIST_POOL *pFreeEntrylist = NULL;
	struct TXS_LIST_ENTRY *pheadEntry = NULL;
	struct TXS_LIST_ENTRY *pEntry = NULL;

	automation_dvt.txs.total_rsp++;

	if (!list || !automation_dvt.txs.isinit) {
		DBGLOG(REQ, LOUD, "txs_list doesnot init\n");
		return FALSE;
	}

	pFreeEntrylist = list->pFreeEntrylist;
	spin_lock_irqsave(&list->lock, IrqFlags);

	list_for_each_entry(pEntry, &list->pHead[pid].mList, mList) {
		if (pEntry->wlan_idx == wlan_idx) {
			if (automation_dvt.txs.check_item[pid].time_stamp
				== time_stamp)
				automation_dvt.txs.duplicate_txs = TRUE;

			automation_dvt.txs.check_item[pid].time_stamp =
				time_stamp;
			list_del_init(&pEntry->mList);
			list->Num--;
			spin_lock_irqsave(&pFreeEntrylist->Lock, IrqFlags2);
			pheadEntry = &pFreeEntrylist->head;
			list_add_tail(&pEntry->mList, &pheadEntry->mList);
			pFreeEntrylist->entry_number += 1;
			spin_unlock_irqrestore(&pFreeEntrylist->Lock,
				IrqFlags2);
			break;
		}
	}
	spin_unlock_irqrestore(&list->lock, IrqFlags);
	return pEntry;
}

/*
* return 0 : No Need Test
* 1: Check Data frame
* 2: Check management and control frame
*/
int is_frame_test(struct ADAPTER *pAd, uint8_t send_received)
{
	if (!pAd || (pAd->auto_dvt == NULL))
		return 0;

	if (send_received == 0 && automation_dvt.txs.stop_send_test == TRUE)
		return 0;

	switch (automation_dvt.txs.test_type) {
	case TXS_COUNT_TEST:
		return 1;
	case TXS_BAR_TEST:
	case TXS_DEAUTH_TEST:
	case TXS_RTS_TEST:
	case TXS_BA_TEST:
		return 2;

	default:
		return 0;
	}
}

uint32_t AutomationTxDone(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	DBGLOG(REQ, LOUD, "AutomationTxDone!\n");
	if (rTxDoneStatus)
		DBGLOG(REQ, LOUD,
			"EVENT-TX DONE [status: %d][seq: %d]: Current Time = %d\n",
			rTxDoneStatus, prMsduInfo->ucTxSeqNum,
			kalGetTimeTick());
	return WLAN_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_CONNAC2X == 1)
/*
* This routine is used to test RXV function.
* It will check RXV of incoming packets if match pre-setting from iwpriv
* Note. This is FALCON RXV format
*/
void connac2x_rxv_correct_test(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	uint32_t prxv1, crxv1;
	uint32_t txmode, rate, frmode, sgi, nsts, ldpc, stbc;

	automation_dvt.rxv.rx_count++;

	/* P-RXV1 */
	prxv1 = ((struct HW_MAC_RX_STS_GROUP_3_V2 *)
				prSwRfb->prRxStatusGroup3)->u4RxVector[0];
	rate = (prxv1 & CONNAC2X_RX_VT_RX_RATE_MASK)
				>> CONNAC2X_RX_VT_RX_RATE_OFFSET;
	nsts = (prxv1 & CONNAC2X_RX_VT_NSTS_MASK)
				>> CONNAC2X_RX_VT_NSTS_OFFSET;
	ldpc = prxv1 & CONNAC2X_RX_VT_LDPC;
	/* C-RXV1 */
	crxv1 = prSwRfb->prRxStatusGroup5->u4RxVector[0];
	stbc = (crxv1 & CONNAC2X_RX_VT_STBC_MASK)
				>> CONNAC2X_RX_VT_STBC_OFFSET;
	txmode = (crxv1 & CONNAC2X_RX_VT_RX_MODE_MASK)
				>> CONNAC2X_RX_VT_RX_MODE_OFFSET;
	frmode = (crxv1 & CONNAC2X_RX_VT_FR_MODE_MASK)
				>> CONNAC2X_RX_VT_FR_MODE_OFFSET;
	sgi = (crxv1 & CONNAC2X_RX_VT_SHORT_GI_MASK)
				>> CONNAC2X_RX_VT_SHORT_GI_OFFSET;

	if (txmode != automation_dvt.rxv.rx_mode) {
		automation_dvt.rxv.rxv_test_result = FALSE;
		DBGLOG(RX, ERROR, "[%s]Receive TxMode=%d, Check RxMode=%d\n",
		__func__, txmode, automation_dvt.rxv.rx_mode);
	}
	if (rate != automation_dvt.rxv.rx_rate) {
		automation_dvt.rxv.rxv_test_result = FALSE;
		DBGLOG(RX, ERROR, "[%s]Receive TxRate=%d, Check RxRate=%d\n",
		__func__, rate, automation_dvt.rxv.rx_rate);
	}
	if (frmode != automation_dvt.rxv.rx_bw) {
		automation_dvt.rxv.rxv_test_result = FALSE;
		DBGLOG(RX, ERROR, "[%s]Receive BW=%d, Check BW=%d\n",
		__func__, frmode, automation_dvt.rxv.rx_bw);
	}
	if (sgi != automation_dvt.rxv.rx_sgi) {
		automation_dvt.rxv.rxv_test_result = FALSE;
		DBGLOG(RX, ERROR, "[%s]Receive Sgi=%d, Check Sgi=%d\n",
		__func__, sgi, automation_dvt.rxv.rx_sgi);
	}
	if (stbc != automation_dvt.rxv.rx_stbc) {
		automation_dvt.rxv.rxv_test_result = FALSE;
		DBGLOG(RX, ERROR, "[%s]Receive Stbc=%d, Check Stbc=%d\n",
		__func__, stbc, automation_dvt.rxv.rx_stbc);
	}
	if (ldpc != automation_dvt.rxv.rx_ldpc) {
		automation_dvt.rxv.rxv_test_result = FALSE;
		DBGLOG(RX, ERROR, "[%s]Receive Ldpc=%d, Check Ldpc=%d\n",
		__func__, ldpc, automation_dvt.rxv.rx_ldpc);
	}

	DBGLOG(RX, DEBUG,
	"\n================ RXV Automation end ================\n");
}
#endif
#endif /* CFG_SUPPORT_WIFI_SYSDVT */

