// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   "hal_wed.c"
 *  \brief
 *
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
#include "gl_os.h"
#include "precomp.h"
#include "hif_pdma.h"
#include "mt66xx_reg.h"
#include "gl_hook_api.h"
#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "coda/mt6639/wf_wfdma_host_dma0.h"
#include "nic_uni_cmd_event.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define CFG_WED_TRX_MAX_PKT_SIZE	1792
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct WED_WO_CMD {
	struct QUE_ENTRY rQueEntry;
	struct WO_CMD_INFO prWoCmdInfo;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

static struct WED_INFO grWedInfo;
static struct dma_token_que grWedToken;
static struct net_device *grNetList[MAX_BSSID_NUM];
static struct net_device *grNetStashList[MAX_BSSID_NUM];
static u_int8_t fgIsWedInSer, fgIsWedInSuspend;
static struct mutex rWedMutex, rWoCmdMutex;
static struct delayed_work rWedWoCmdWork;
static struct QUE rWoCmdQueue;
uint32_t g_u4SuspendCnt;
uint32_t g_u4ResumeCnt;

/* chip dependency */
static uint32_t mt6639_wed_mirror_table[] = {
	WIFI_INT_MSK,
	WIFI_INT_STA,
	WIFI_TX_RING0_BASE,
	WIFI_TX_RING0_CNT,
	WIFI_TX_RING0_CIDX,
	WIFI_TX_RING0_DIDX,
	WIFI_TX_RING1_BASE,
	WIFI_TX_RING1_CNT,
	WIFI_TX_RING1_CIDX,
	WIFI_TX_RING1_DIDX,
	WIFI_RX_RING4_BASE,
	WIFI_RX_RING4_CNT,
	WIFI_RX_RING4_CIDX,
	WIFI_RX_RING4_DIDX,
	WIFI_RX_RING5_BASE,
	WIFI_RX_RING5_CNT,
	WIFI_RX_RING5_CIDX,
	WIFI_RX_RING5_DIDX,
	WIFI_RX_RING6_BASE,
	WIFI_RX_RING6_CNT,
	WIFI_RX_RING6_CIDX,
	WIFI_RX_RING6_DIDX,
	0x0
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static int wedAttachWarp(struct ADAPTER *prAdapter,
	struct net_device *prNetDev, uint8_t AttachType);
static int wedDetachWarp(struct ADAPTER *prAdapter,
	struct net_device *prNetDev, uint8_t DetachType);
static void wedRxTokenInfoRelease(struct ADAPTER *prAdapter);
static void wedSendCommand2WO(struct work_struct *work);
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static void wedNetDevStash(struct net_device *prNetDev)
{
	uint32_t i = 0;

	if (!prNetDev)
		return;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (grNetStashList[i] == prNetDev)
			return;
		if (grNetStashList[i] == NULL) {
			grNetStashList[i] = prNetDev;
			break;
		}
	}
}

static struct net_device *wedNetDevStashPop(void)
{
	uint32_t i = 0;
	struct net_device *prNetDev = NULL;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (grNetStashList[i] != NULL) {
			prNetDev = grNetStashList[i];
			grNetStashList[i] = NULL;
			return prNetDev;
		}
	}
	return NULL;
}

static void wedNetDevStashRemove(struct net_device *prNetDev)
{
	uint32_t i = 0;

	if (!prNetDev)
		return;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (grNetStashList[i] == prNetDev) {
			grNetStashList[i] = NULL;
			break;
		}
	}
}

static void wedUpdateStaRecAndBa(struct ADAPTER *prAdapter)
{
	int i, j, k;
	struct net_device *prNetDev;
	struct STA_RECORD *prStaRec;
	struct RX_BA_ENTRY *prRxBaEntry;
	struct UNI_EVENT_RX_ADDBA ba;

	/* update sta record */
	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = cnmGetStaRecByIndex(prAdapter, i);
		if (!prStaRec)
			continue;
		prNetDev = wlanGetNetDev(prAdapter->prGlueInfo,
					 prStaRec->ucBssIndex);
		if (!prNetDev)
			continue;
		for (j = 0; j < MAX_BSSID_NUM; j++) {
			if (prNetDev != grNetList[j])
				continue;
			wedStaRecUpdate(prAdapter, prStaRec);
			for (k = 0; k < CFG_RX_MAX_BA_TID_NUM; k++) {
				prRxBaEntry = qmLookupRxBaEntry(prAdapter,
							  prStaRec->ucIndex, k);
				if (prRxBaEntry == NULL)
					continue;
				ba.u2WlanIdx = prStaRec->ucWlanIndex;
				ba.u2WinSize = prRxBaEntry->u2WinSize -
					       prAdapter->rWifiVar.u2BaExtSize;
				ba.u2BAStartSeqCtrl =
					(uint16_t)(prRxBaEntry->u2WinStart
							  << OFFSET_BAR_SSC_SN);
				ba.ucTid = prRxBaEntry->ucTid;

				wedStaRecRxAddBaUpdate(prAdapter, &ba);
			}
			break;
		}
	}
}

void wedHwRecoveryFromError(struct ADAPTER *prAdapter, uint32_t status)
{
	int i, ret;
	struct net_device *prNetDev;

	DBGLOG(HAL, DEBUG, "SER(E) hook to warp : %d\n", status);

	mutex_lock(&rWedMutex);
	if (status == WIFI_ERR_RECOV_DETACH) {
		if (fgIsWedInSer == TRUE)
			return;

		for (i = 0; i < MAX_BSSID_NUM; i++) {
			prNetDev = grNetList[i];
			if (!prNetDev)
				continue;
			DBGLOG(HAL, STATE, "ser detach %s!\n", prNetDev->name);
			wedNetDevStash(prNetDev);
			wedDetachWarp(prAdapter, prNetDev, WED_DETACH_IFDOWN);
		}
		fgIsWedInSer = TRUE;
	} else if (status == WIFI_ERR_RECOV_ATTACH) {
		if (fgIsWedInSer == FALSE)
			return;
		fgIsWedInSer = FALSE;
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			prNetDev = wedNetDevStashPop();
			if (!prNetDev)
				break;
			DBGLOG(HAL, STATE, "ser attach %s!\n", prNetDev->name);
			wedAttachWarp(prAdapter, prNetDev, WED_ATTACH_IFON);
		}
		/* update sta record and ba */
		wedUpdateStaRecAndBa(prAdapter);
	} else if (status == WIFI_ERR_RECOV_HIF_INIT) {
		/* wed hif init */
		wedRxTokenInfoRelease(prAdapter);
		if (wedRxTokenInfoSetup(prAdapter) < 0) {
			DBGLOG(HAL, ERROR, "SER(E) wedRxTokenInfoSetup fail\n");
			goto end;
		}
		ret = wedProxyHookCall(PROXY_WLAN_HOOK_HIF_INIT, &grWedInfo);
		if (ret) {
			DBGLOG(HAL, ERROR, "SER(E) HIF_INIT fail\n");
			wedRxTokenInfoRelease(prAdapter);
			goto end;
		}
		ret = wedProxyHookCall(PROXY_WLAN_HOOK_DMA_SET, &grWedInfo);
		if (ret) {
			DBGLOG(HAL, ERROR, "SER(E) DMA_SET fail\n");
			wedRxTokenInfoRelease(prAdapter);
			goto end;
		}
		kalDevRegWrite(prAdapter->prGlueInfo,
			WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR,
			grWedInfo.int_enable_mask);

		/* update sta record and ba */
		wedUpdateStaRecAndBa(prAdapter);
	} else {
		wedProxyHookCall(PROXY_WLAN_HOOK_SER, &status);
	}
end:
	mutex_unlock(&rWedMutex);
}

void wedAttachDetach(struct ADAPTER *prAdapter, struct net_device *prNetDev,
			u_int8_t fgIsAttach)
{
	if (prAdapter == NULL || prNetDev == NULL)
		return;

	mutex_lock(&rWedMutex);
	if (fgIsAttach == TRUE)
		wedAttachWarp(prAdapter, prNetDev, WED_ATTACH_IFON);
	else
		wedDetachWarp(prAdapter, prNetDev, WED_DETACH_IFDOWN);
	mutex_unlock(&rWedMutex);
}

void wedSuspendResume(u_int8_t fgIsSuspend)
{
	mutex_lock(&rWedMutex);
	if (fgIsSuspend == TRUE)
		wedSuspendTrigger();
	else
		wedResumeTrigger();
	mutex_unlock(&rWedMutex);
}

void wedTriggerReset(void)
{
	uint32_t u4CmdId = SER_USER_CMD_L1_RECOVER;
	uint32_t ret;

	kalIoctl(grWedInfo.pAdAdapter,
		wlanoidSetSer, (void *)&u4CmdId,
		sizeof(u4CmdId), &ret);
}

void wedProxyIoRead(struct GLUE_INFO *prGlueInfo,
	uint32_t u4BusAddr, uint32_t *pu4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WED_CR_ACCESS rWedCr;

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo) {
		DBGLOG(HAL, WARN, "Chip info get fail\n");
		return;
	}

	/* Check mirror first */
	if (wedMirrorAddrCheck(u4BusAddr)) {
		rWedCr.reg_addr = u4BusAddr;
		rWedCr.reg_val = 0;
		wedProxyHookCall(PROXY_WLAN_HOOK_READ, &rWedCr);
		*pu4Value = rWedCr.reg_val;
	} else {
		RTMP_IO_READ32(prChipInfo, u4BusAddr, pu4Value);
	}
}

void wedProxyIoWrite(struct GLUE_INFO *prGlueInfo,
	uint32_t u4BusAddr, uint32_t u4Value)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WED_CR_ACCESS rWedCr;

	glGetChipInfoByGlue(prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo) {
		DBGLOG(HAL, WARN, "Chip info get fail\n");
		return;
	}

	/* Check mirror first */
	if (wedMirrorAddrCheck(u4BusAddr)) {
		rWedCr.reg_addr = u4BusAddr;
		rWedCr.reg_val = u4Value;
		wedProxyHookCall(PROXY_WLAN_HOOK_WRITE, &rWedCr);
	} else {
		RTMP_IO_WRITE32(prChipInfo, u4BusAddr, u4Value);
	}
}

int wedInitAdapterInfo(struct ADAPTER *prAdapter)
{
	if (prAdapter->prWedInfo != NULL) {
		DBGLOG(HAL, WARN, "WED already initialized before\n");
		return -1;
	}

	prAdapter->prWedInfo = &grWedInfo;
	grWedInfo.pAdAdapter = prAdapter;
	grWedInfo.prGlueInfo = prAdapter->prGlueInfo;

	DBGLOG(HAL, STATE, "WED adapter info initialized\n");
	return 0;
}

int wedInitial(void)
{
	memset(&grWedInfo, 0, sizeof(struct WED_INFO));
	fgIsWedInSer = FALSE;
	fgIsWedInSuspend = FALSE;
	mutex_init(&rWedMutex);
	mutex_init(&rWoCmdMutex);
	QUEUE_INITIALIZE(&rWoCmdQueue);
	INIT_DELAYED_WORK(&rWedWoCmdWork, wedSendCommand2WO);

	DBGLOG(HAL, STATE, "WED info initialized\n");
	return 0;
}

void wedInterruptSwap(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct pci_dev *pdev = NULL;

	prGlueInfo = (struct GLUE_INFO *) dev_instance;
	prHifInfo = &prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;

	/* Notify wifi host to handle the irq belongs to WED/PCIE */
	prHifInfo->u4IrqId = irq;
	pdev->irq = irq;
}

void wedUpdateIntMask(uint32_t mask)
{
	grWedInfo.int_enable_mask = mask;
}

/* chip dependency */
int wedInfoSetup(struct ADAPTER *prAdapter)
{
	struct WED_INFO *prwedinfo;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct pcie_msi_info *prMsiInfo = NULL;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	glGetChipInfoByGlue(prAdapter->prGlueInfo, (void **)&prChipInfo);

	if (!prChipInfo) {
		DBGLOG(HAL, WARN, "Chip info get fail\n");
		return -1;
	}

	prwedinfo = &grWedInfo;
	prBusInfo = prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	prwedinfo->prGlueInfo = prAdapter->prGlueInfo;
	prwedinfo->pAdAdapter = prAdapter;
	prwedinfo->ChipID = prChipInfo->chip_id;
	prwedinfo->infType = BUS_TYPE_PCIE;
	prwedinfo->u4IrqId = prHifInfo->u4IrqId;
	prwedinfo->fgMirrorEnable = FALSE;
	prwedinfo->macVer = MAC_TYPE_BMAC; /* MAC type to be parsed by WO-CPU */
	prwedinfo->fgMsiEnable = (prMsiInfo && prMsiInfo->fgMsiEnabled) ?
		true : false;
	prwedinfo->pcie_msi_msg_addr_lo = prMsiInfo->address_lo;
	prwedinfo->pcie_msi_msg_addr_hi = prMsiInfo->address_hi;
	prwedinfo->whnat_en = TRUE;
	prwedinfo->wed_idx = 0;

	prwedinfo->pci_dev = prChipInfo->pdev;
	prwedinfo->base_addr = (unsigned long)prChipInfo->CSRBaseAddress;

	prwedinfo->dma_offset = WPDMA_OFFSET;
	prwedinfo->int_sta = WIFI_INT_STA;
	prwedinfo->int_mask = WIFI_INT_MSK;

	/* trigger WED's interrupt enable config */
	kalDevRegRead(prAdapter->prGlueInfo,
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR,
		&prwedinfo->int_enable_mask);

	DBGLOG(HAL, STATE, "prwedinfo->int_enable_mask is 0x%x\n",
		prwedinfo->int_enable_mask);
	/* error recoverying(pending for now) */
	prwedinfo->int_ser = 0;
	prwedinfo->int_ser_value = 0;

	prwedinfo->tx_dma_glo_cfg = WIFI_HOST_DMA0_WPDMA_GLO_CFG;
	prwedinfo->rx_dma_glo_cfg = WIFI_HOST_DMA0_WPDMA_GLO_CFG;
	prwedinfo->txd_size = TXD_SIZE;
	prwedinfo->rxd_size = MAX_RXD_SIZE;
	prwedinfo->tx_pkt_size = CFG_WED_TRX_MAX_PKT_SIZE;
	prwedinfo->rx_pkt_size = CFG_WED_TRX_MAX_PKT_SIZE;
	prwedinfo->tx_ring_size = TX_RING_SIZE;
	prwedinfo->rx_ring_size = RX_RING_MAX_SIZE;
	prwedinfo->tx_token_nums = HIF_TX_MSDU_TOKEN_NUM; /* 0x2F00 */

	prwedinfo->ring_offset = WIFI_RING_OFFSET;
	prwedinfo->fbuf_size = 128;

	/* Tx Ring, Ring0 for TxData(band0), Ring1 for TxData(band1) */
	prwedinfo->tx_ring[0].base = WIFI_TX_RING0_BASE; /* 4300 */
	prwedinfo->tx_ring[0].cnt = WIFI_TX_RING0_CNT;	 /* 4304 */
	prwedinfo->tx_ring[0].cidx = WIFI_TX_RING0_CIDX; /* 4308 */
	prwedinfo->tx_ring[0].didx = WIFI_TX_RING0_DIDX; /* 430C */
	prwedinfo->tx_ring[1].base = WIFI_TX_RING1_BASE; /* 4310 */
	prwedinfo->tx_ring[1].cnt = WIFI_TX_RING1_CNT;	 /* 4314 */
	prwedinfo->tx_ring[1].cidx = WIFI_TX_RING1_CIDX; /* 4318 */
	prwedinfo->tx_ring[1].didx = WIFI_TX_RING1_DIDX; /* 431C */

	/* Rx Ring, Ring4 for RxData(band0) and Ring5 for RxData(band1) */
	prwedinfo->rx_ring[0].base = WIFI_RX_RING4_BASE; /* 4540 */
	prwedinfo->rx_ring[0].cnt = WIFI_RX_RING4_CNT;	 /* 4544 */
	prwedinfo->rx_ring[0].cidx = WIFI_RX_RING4_CIDX; /* 4548 */
	prwedinfo->rx_ring[0].didx = WIFI_RX_RING4_DIDX; /* 454C */
	prwedinfo->rx_ring[1].base = WIFI_RX_RING5_BASE; /* 4550 */
	prwedinfo->rx_ring[1].cnt = WIFI_RX_RING5_CNT;	 /* 4554 */
	prwedinfo->rx_ring[1].cidx = WIFI_RX_RING5_CIDX; /* 4558 */
	prwedinfo->rx_ring[1].didx = WIFI_RX_RING5_DIDX; /* 455C */

	/* Event Ring, Ring6 for Event/TxFreeDone */
	prwedinfo->event.base = WIFI_RX_RING6_BASE; /* 4560 */
	prwedinfo->event.cnt = WIFI_RX_RING6_CNT;	/* 4564 */
	prwedinfo->event.cidx = WIFI_RX_RING6_CIDX; /* 4568 */
	prwedinfo->event.didx = WIFI_RX_RING6_DIDX; /* 456C */

	/* Interrupt status bit set */
	prwedinfo->wfdma_tx_done_trig0_bit =
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_0_SHFT;
	prwedinfo->wfdma_tx_done_trig1_bit =
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_1_SHFT;
	prwedinfo->wfdma_rx_done_trig0_bit =
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_SHFT;
	prwedinfo->wfdma_rx_done_trig1_bit =
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_SHFT;
	prwedinfo->wfdma_tx_done_free_notify_trig_bit =
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_SHFT;

	/* Let proxy driver could pass IRQ process flow back to gen4m */
	prwedinfo->irq_handler = prHifInfo->irq_handler;
	prwedinfo->irq_handler_thread = prHifInfo->irq_handler_thread;
	prwedinfo->irq_swap = wedInterruptSwap;

	/* wedRxTokenInit callback*/
	prwedinfo->wedRxTokenInit = wedRxTokenInit;
	/* enable WED's DMA TX/RX */
	prwedinfo->wed_dma_ctrl = DMA_TX_RX;

	/* ser */
	prwedinfo->wifi_reset = wedTriggerReset;

	return 0;
}

static void wedRxTokenInfoRelease(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo;
	struct WED_DMABUF *prWedDmaBuf;
	struct RTMP_RX_RING *pRxRing;
	struct RTMP_DMACB *prRxCell;
	struct RTMP_DMABUF *pDmaBuf;
	struct RXD_STRUCT *pRxD;
	uint32_t u4token_id;
	uint32_t u4Idx;

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	/* Release the RxBM + WiFi buffer by the individual way */
	for (u4token_id = 0; u4token_id < grWedToken.u4MaxSize; u4token_id++) {
		prWedDmaBuf = &grWedToken.pkt_token[u4token_id];
		if (!prWedDmaBuf->fgIsSKB && prWedDmaBuf->pkt != NULL) {
			page_frag_free(prWedDmaBuf->pkt);
			prWedDmaBuf->pkt = NULL;
		}

		/* skb would be freed by kernel, no need to free it again */
	}

	/* free all memory allocated at WARP attach phase */
	kalMemFree(grWedToken.pkt_token, VIR_MEM_TYPE,
		grWedToken.u4MaxSize*sizeof(struct WED_DMABUF));
	grWedToken.pkt_token = NULL;

	grWedToken.u4FreeIdx = 0;
	grWedToken.u4MaxSize = 0;

	/* Flush tokenID to gen4m ring1's buffer */
	pRxRing = &prHifInfo->RxRing[RX_RING_DATA0];
	for (u4Idx = 0; u4Idx < pRxRing->u4RingSize; u4Idx++) {
		prRxCell = &pRxRing->Cell[u4Idx];
		pDmaBuf = &prRxCell->DmaBuf;
		pRxD = (struct RXD_STRUCT *) prRxCell->AllocVa;

		pRxD->SDPtr1 &= ~(RXDMAD_TOKEN_ID_MASK);
		pRxD->SDLen1 &= ~(BIT(8));
	}

	/* Flush tokenID to gen4m ring2's buffer */
	pRxRing = &prHifInfo->RxRing[RX_RING_DATA1];
	for (u4Idx = 0; u4Idx < pRxRing->u4RingSize; u4Idx++) {
		prRxCell = &pRxRing->Cell[u4Idx];
		pDmaBuf = &prRxCell->DmaBuf;
		pRxD = (struct RXD_STRUCT *) prRxCell->AllocVa;

		pRxD->SDPtr1 &= ~(RXDMAD_TOKEN_ID_MASK);
		pRxD->SDLen1 &= ~(BIT(8));
	}

}

int wedRxTokenInfoSetup(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo;
	struct RTMP_RX_RING *pRxRing;
	struct RTMP_DMACB *prRxCell;
	struct RTMP_DMABUF *pDmaBuf;
	struct RXD_STRUCT *pRxD;
	uint32_t u4token_id;
	uint32_t u4MaxSize;
	uint32_t u4Idx;

	DBGLOG(HAL, STATE, "WED Rx Token set up\n");

	/* Total token number would be gen4m's
	 * ring1 + ring2 + buffer number in warp
	 */
	u4MaxSize = 0;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	pRxRing = &prHifInfo->RxRing[RX_RING_DATA0];
	u4MaxSize += pRxRing->u4RingSize;
	pRxRing = &prHifInfo->RxRing[RX_RING_DATA1];
	u4MaxSize += pRxRing->u4RingSize;

	/* hard code number in warp driver */
	u4MaxSize += WARP_RXBM_CB_NUM;

	grWedToken.u4MaxSize = u4MaxSize;
	grWedToken.u4FreeIdx = 0;

	grWedToken.pkt_token = kalMemZAlloc(
		u4MaxSize * sizeof(struct WED_DMABUF), VIR_MEM_TYPE);
	if (grWedToken.pkt_token == NULL) {
		DBGLOG(HAL, ERROR, "WED RX token[%d] allocate fail\n",
			u4MaxSize);
		return -1;
	}

	DBGLOG(HAL, STATE, "Total WED RX token[%d] allocated\n",
		u4MaxSize);

	/* Assign tokenID to gen4m ring1's buffer */
	pRxRing = &prHifInfo->RxRing[RX_RING_DATA0];
	for (u4Idx = 0; u4Idx < pRxRing->u4RingSize; u4Idx++) {
		prRxCell = &pRxRing->Cell[u4Idx];
		pDmaBuf = &prRxCell->DmaBuf;
		pRxD = (struct RXD_STRUCT *) prRxCell->AllocVa;
#if CFG_SUPPORT_RX_ZERO_COPY
		u4token_id = wedRxTokenInit(NULL, TRUE,
			prRxCell->pPacket, pDmaBuf->AllocSize,
			pDmaBuf->AllocVa, pDmaBuf->AllocPa);
#else
		u4token_id = wedRxTokenInit(NULL, FALSE,
			NULL, pDmaBuf->AllocSize,
			pDmaBuf->AllocVa, pDmaBuf->AllocPa);
#endif
		pRxD->SDPtr1 = (u4token_id << RXDMAD_TOKEN_ID_SHIFT);
		pRxD->SDLen1 = BIT(8);
	}

	/* Assign tokenID to gen4m ring2's buffer */
	pRxRing = &prHifInfo->RxRing[RX_RING_DATA1];
	for (u4Idx = 0; u4Idx < pRxRing->u4RingSize; u4Idx++) {
		prRxCell = &pRxRing->Cell[u4Idx];
		pDmaBuf = &prRxCell->DmaBuf;
		pRxD = (struct RXD_STRUCT *)prRxCell->AllocVa;
#if CFG_SUPPORT_RX_ZERO_COPY
		u4token_id = wedRxTokenInit(NULL, TRUE,
			prRxCell->pPacket, pDmaBuf->AllocSize,
			pDmaBuf->AllocVa, pDmaBuf->AllocPa);
#else
		u4token_id = wedRxTokenInit(NULL, FALSE,
			NULL, pDmaBuf->AllocSize,
			pDmaBuf->AllocVa, pDmaBuf->AllocPa);
#endif
		pRxD->SDPtr1 = (u4token_id << RXDMAD_TOKEN_ID_SHIFT);
		pRxD->SDLen1 = BIT(8);
	}

	return 0;
}

static void wedNetDevSet(struct net_device *dev)
{
	uint32_t i = 0;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (grNetList[i] == dev)
			return;

		if (grNetList[i] == NULL) {
			grNetList[i] = dev;
			break;
		}
	}
}

static void wedNetDevDel(struct net_device *dev)
{
	uint32_t i = 0;

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (grNetList[i] == dev) {
			grNetList[i] = NULL;
			break;
		}
	}
}

static void wedHwnatSet(struct ADAPTER *prAdapter,
	bool fgEnable, struct net_device *prNetDev)
{
	if (!prNetDev) {
		DBGLOG(HAL, ERROR, "net_device is NULL!\n");
		return;
	}

	if (!ppe_dev_register_hook || !ppe_dev_unregister_hook) {
		DBGLOG(HAL, ERROR, "HWNAT driver got error! fail on %s\n",
			   fgEnable ? "Register" : "Unregister");
		return;
	}

	DBGLOG(HAL, STATE, "%s device %s to hw_nat\n",
		   fgEnable ? "Register" : "Unregister", prNetDev->name);

	if (fgEnable) {
		wedNetDevSet(prNetDev);
		ppe_dev_register_hook(prNetDev);
	} else {
		wedNetDevDel(prNetDev);
		ppe_dev_unregister_hook(prNetDev);
	}
}

static int wedAttachWarp(struct ADAPTER *prAdapter, struct net_device *prNetDev,
						uint8_t AttachType)
{
	uint32_t val = 0;
	int ret = 0;
	struct WED_INFO *prwedinfo;
	uint32_t u4Tick;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgEnableWed)) {
		DBGLOG(HAL, WARN, "WED disabled by wifi.cfg\n");
		return ret;
	}
	if (grWedInfo.proxy_ops == NULL) {
		DBGLOG(HAL, WARN, "warp_proxy not regisgered\n");
		wedNetDevStash(prNetDev);
		return -1;
	}
	/* step.1 Register net_dev to hwnat */
	/* Direct call to cover DBDC */
	if (fgIsWedInSer == TRUE || fgIsWedInSuspend == TRUE) {
		wedNetDevStash(prNetDev);
		DBGLOG(HAL, WARN,
			"wed in ser or str, add ndev to stash list\n");
		return 0;
	}
	wedHwnatSet(prAdapter, TRUE, prNetDev);

	if (IsWedAttached()) {
		DBGLOG(HAL, WARN, "WED already attached before\n");
		return -1;
	}

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter, DRV_OWN_SRC_WED_ATTACH);
	DBGLOG(HAL, STATE, "WED proxy attaching, reason: %u\n", AttachType);

	prwedinfo = &grWedInfo;

	/* Before hooking up with warp's HW,
	 * initial local ring's buffer with tokenID
	 */
	ret = wedRxTokenInfoSetup(prAdapter);
	if (ret < 0)
		goto error;

	/* step.2 fill all necessary information for warp proxy driver */
	ret = wedInfoSetup(prAdapter);
	if (ret < 0)
		goto error_release_token;

	/* step.3 Ask WARP driver to perform basic WED HW initialization
	 * with diff flow between interface up and resume
	 */
	if (AttachType == WED_ATTACH_IFON)
		ret = wedProxyHookCall(PROXY_WLAN_HOOK_SYS_UP, &grWedInfo);
	else
		ret = wedProxyHookCall(PROXY_WLAN_HOOK_RESUME, &grWedInfo);

	if (ret < 0)
		goto error_release_token;

	/* Wait warp driver porbe done */
	u4Tick = kalGetTimeTick();
	while (!grWedInfo.fgAttached) {
		kalUsleep_range(900, 1000);
		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Tick,
				      MSEC_TO_SYSTIME(100))) {
			DBGLOG(HAL, ERROR, "wait warp probe timeout\n");
			break;
		}
	}

	grWedInfo.fgMirrorEnable = 1;
	disable_irq_nosync(prwedinfo->u4IrqId);
	DBGLOG(HAL, STATE, "disable irq for %d\n", prwedinfo->u4IrqId);

	/* step.4 Force reset WFDMA HW */
	kalDevRegWrite(prAdapter->prGlueInfo, 0x7c024100, 0x00000020);
	kalDevRegWrite(prAdapter->prGlueInfo, 0x7c024100, 0x00000030);

	/* step.5 Reinitial TX/RX rings in WFDMA */
	halWpdmaInitRing(prAdapter->prGlueInfo, false);

	/* step.6 Disable WFDMA's RX(warp driver will enable it later)
	 * and keep RXDMAD reserved context which used by WED
	 */
	kalDevRegRead(prAdapter->prGlueInfo,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR, &val);
	val &= ~(BIT(2));
	kalDevRegWrite(prAdapter->prGlueInfo,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR, val);

	kalDevRegRead(prAdapter->prGlueInfo,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR, &val);
	val |= BIT(10);
	kalDevRegWrite(prAdapter->prGlueInfo,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR, val);

	/* step.7  trigger warp driver to take control on WFDMA */
	if (prwedinfo->wed_ver <= 0) {
		DBGLOG(HAL, WARN, "WED attach failed by system kernel %d\n");
		ret = -1;
		goto error_release_token;
	}
	ret = wedProxyHookCall(PROXY_WLAN_HOOK_HIF_INIT, &grWedInfo);
	if (ret < 0)
		goto error_release_token;
	ret = wedProxyHookCall(PROXY_WLAN_HOOK_DMA_SET, &grWedInfo);
	if (ret < 0)
		goto error_release_token;

	/* step.8 Enable INT again(mapping to warp) Read from gen4m, and write
	 * to WED. Direct use halEnableInterrupt may cause Kernel warnning
	 * about enable irq twice
	 */
	kalDevRegWrite(prAdapter->prGlueInfo,
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR,
		grWedInfo.int_enable_mask);

	/* step.9 Enable PCIE irq to WED */
	wedProxyHookCall(PROXY_WLAN_HOOK_SWAP_IRQ, &grWedInfo);

	grWedInfo.fgAttached = TRUE;

	/*this will enable wed irq*/
	enable_irq(prwedinfo->u4IrqId);
	DBGLOG(HAL, STATE, "enable irq for %d\n", prwedinfo->u4IrqId);

	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_WED_ATTACH);
	return 0;

error_release_token:
	wedRxTokenInfoRelease(prAdapter);
error:
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_WED_ATTACH);
	return ret;
}

static int wedDetachWarp(struct ADAPTER *prAdapter, struct net_device *prNetDev,
	uint8_t DetachType)
{
	struct BUS_INFO *prBusInfo;
	struct WED_INFO *prwedinfo;
	struct mt66xx_chip_info *prChipInfo;
	struct GL_HIF_INFO *prHifInfo = NULL;
	uint32_t i = 0;
	int ret = 0;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.fgEnableWed)) {
		DBGLOG(HAL, WARN, "WED disabled by wifi.cfg\n");
		return ret;
	}
	if (grWedInfo.proxy_ops == NULL) {
		DBGLOG(HAL, WARN, "warp_proxy not regisgered\n");
		wedNetDevStashRemove(prNetDev);
		return ret;
	}

	/* step.1 Unregister net_dev to hwnat */
	/* Direct call to cover DBDC */
	if (fgIsWedInSer == TRUE || fgIsWedInSuspend == TRUE) {
		wedNetDevStashRemove(prNetDev);
		DBGLOG(HAL, WARN,
			"wed in ser or str, remove from stash list\n");
		return 0;
	}
	wedHwnatSet(prAdapter, FALSE, prNetDev);

	/* Detach is required only when all interfaces go down */
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (grNetList[i] != NULL)
			return -1;
	}

	if (!IsWedAttached()) {
		DBGLOG(HAL, WARN, "WED not attached yet\n");
		return -1;
	}

	flush_delayed_work(&rWedWoCmdWork);
	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter, DRV_OWN_SRC_WED_DEATTACH);

	DBGLOG(HAL, STATE, "WED proxy detaching, reason: %u\n", DetachType);

	prwedinfo = &grWedInfo;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prBusInfo = prAdapter->chip_info->bus_info;
	prChipInfo = prAdapter->chip_info;

	/* step.3 Release the ring control on WED and CR mirror*/
	prwedinfo->fgMirrorEnable = FALSE;
	wedProxyHookCall(PROXY_WLAN_HOOK_HIF_EXIT, &grWedInfo);

	/* step.4 Release the RXBM buffer which is allocated by WARP Nothing
	 * but WiFi host know the status of this buffer Also release the
	 * gen4m's one
	 */
	halUninitMsduTokenInfo(prAdapter);
	halWpdmaFreeRing(prAdapter->prGlueInfo);
	wedRxTokenInfoRelease(prAdapter);

	/* step.5 Force WFDMA reinit after take over from WED to CPU
	 * Reallocate the Ring Buffer to avoid buffer free error when rmmod
	 */
	if (!halWpdmaAllocRing(prAdapter->prGlueInfo, true)) {
		DBGLOG(HAL, ERROR, "halWpdmaAllocRing fail\n");
		ret = -1;
		goto error;
	}

	halWpdmaInitRing(prAdapter->prGlueInfo, true);
	halInitMsduTokenInfo(prAdapter);
	/* Initialize wfdma reInit handshake parameters */
	if ((prChipInfo->asicWfdmaReInit)
	    && (prChipInfo->asicWfdmaReInit_handshakeInit))
		prChipInfo->asicWfdmaReInit_handshakeInit(prAdapter);

	/* step.6 WARP & WO resource release and IRQ swap back */
	/* with diff flow between interface down and suspend */
	if (DetachType == WED_DETACH_IFDOWN)
		wedProxyHookCall(PROXY_WLAN_HOOK_SYS_DOWN, &grWedInfo);
	else
		wedProxyHookCall(PROXY_WLAN_HOOK_SUSPEND, &grWedInfo);

	prwedinfo->whnat_en = FALSE;

	/* step.7 Enable HIF side PDMA TX/RX */
	if (prBusInfo->pdmaStop)
		prBusInfo->pdmaStop(prAdapter->prGlueInfo, FALSE);
	else
		DBGLOG(HAL, ERROR, "PDMA config API didn't register\n");


	grWedInfo.fgAttached = FALSE;
	ret = 0;

error:
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE,
		DRV_OWN_SRC_WED_DEATTACH);
	return ret;
}

int wedShowDebugInfo(void)
{
	return wedProxyHookCall(PROXY_WLAN_HOOK_DEBUG, &grWedInfo);
}

int wedProxyHookCall(uint16_t hook, void *priv)
{
	struct proxy_wlan_hook_ops *ops;
	static const char *const hook_str[] = {
		"PROXY_WLAN_HOOK_HIF_INIT",
		"PROXY_WLAN_HOOK_HIF_EXIT",		/* 1 */
		"PROXY_WLAN_HOOK_TX",			/* 2 */
		"PROXY_WLAN_HOOK_RX",			/* 3 */
		"PROXY_WLAN_HOOK_SYS_UP",		/* 4 */
		"PROXY_WLAN_HOOK_SYS_DOWN",		/* 5 */
		"PROXY_WLAN_HOOK_ISR",			/* 6 */
		"PROXY_WLAN_HOOK_DMA_SET",		/* 7 */
		"PROXY_WLAN_HOOK_SER",			/* 8 */
		"PROXY_WLAN_HOOK_SUSPEND",		/* 9 */
		"PROXY_WLAN_HOOK_RESUME",		/* 10 */
		"PROXY_WLAN_HOOK_READ",			/* 11 */
		"PROXY_WLAN_HOOK_WRITE",		/* 12 */
		"PROXY_WLAN_HOOK_SEND_CMD",		/* 13 */
		"PROXY_WLAN_HOOK_SWAP_IRQ",		/* 14 */
		"PROXY_WLAN_HOOK_DEBUG",		/* 15 */
		"PROXY_WLAN_HOOK_END"			/* 16 */
	};

	ops = grWedInfo.proxy_ops;
	if (!ops) {
		DBGLOG(HAL, WARN, "warp_proxy not regisgered\n");
		return -1;
	}

	if (ops->hooks & BIT(hook)) {
		DBGLOG(SW1, LOUD, "hook: %s\n", hook_str[hook]);
		return ops->fun(hook, &grWedInfo, priv);
	}

	DBGLOG(HAL, WARN, "WED proxy ops invalid\n");
	return -1;
}


int wedProxyHookRegister(struct proxy_wlan_hook_ops *ops)
{
	struct net_device *prNetDev;

	if (!ops)
		return -1;

	if (grWedInfo.proxy_ops != NULL) {
		DBGLOG(HAL, WARN, "WED proxy ops registered before\n");
		return -1;
	}

	DBGLOG(HAL, DEBUG, "WED proxy ops registered\n");
	grWedInfo.proxy_ops = ops;

	if (grWedInfo.pAdAdapter == NULL) {
		DBGLOG(HAL, DEBUG, "grWedInfo.pAdAdapter is null\n");
		return 0;
	}

	mutex_lock(&rWedMutex);
	while (prNetDev = wedNetDevStashPop(), prNetDev != NULL)
		wedAttachWarp(grWedInfo.pAdAdapter, prNetDev, WED_ATTACH_IFON);
	mutex_unlock(&rWedMutex);

	return 0;
}
EXPORT_SYMBOL(wedProxyHookRegister);

int wedProxyHookUnregister(struct proxy_wlan_hook_ops *ops)
{
	uint32_t i = 0;

	if (!ops)
		return -1;

	if (grWedInfo.proxy_ops != ops) {
		DBGLOG(HAL, DEBUG, "WED proxy ops mismatch\n");
		return -1;
	}

	/* Error handle to avoid unregister before detach */
	if (IsWedAttached()) {
		dump_stack();
		DBGLOG(HAL, ERROR, "WED not yet Detach!\n");
		mutex_lock(&rWedMutex);
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			if (grNetList[i] != NULL) {
				DBGLOG(HAL, ERROR, "need disable %s!\n",
					grNetList[i]->name);
				wedDetachWarp(grWedInfo.pAdAdapter,
					grNetList[i], WED_DETACH_IFDOWN);
			}
		}
		mutex_unlock(&rWedMutex);
	}

	DBGLOG(HAL, DEBUG, "WED proxy ops unregistered\n");
	grWedInfo.proxy_ops = NULL;

	return 0;
}
EXPORT_SYMBOL(wedProxyHookUnregister);

uint32_t wedMirrorAddrCheck(uint32_t u4BusAddr)
{
	uint32_t *pu4AddrList;

	if (grWedInfo.proxy_ops == NULL)
		return 0;

	if (grWedInfo.fgMirrorEnable == FALSE)
		return 0;

	pu4AddrList = mt6639_wed_mirror_table;
	while ((*pu4AddrList != 0) && (*pu4AddrList != u4BusAddr))
		pu4AddrList++;

	return *pu4AddrList;
}

static uint32_t wedRxInfoGet(struct RXD_STRUCT *pRxD, struct SW_RFB *prSwRfb)
{
	struct WED_RX_INFO *prWedRxInfo = NULL;
	uint32_t DW1 = *(uint32_t *)((uint32_t *)pRxD + 1);
	uint32_t DW2 = *(uint32_t *)((uint32_t *)pRxD + 2);
	uint32_t DW3 = *(uint32_t *)((uint32_t *)pRxD + 3);
	uint8_t fgDrop = FALSE;
	struct sk_buff *prSkb = (struct sk_buff *)prSwRfb->pvPacket;

	if (prSwRfb->prWedRxInfo) {
		DBGLOG(HAL, ERROR, "prWedRxInfo memory not released!!\n");
		return 0;
	}

	if (DW3 & BIT(RXDMAD_PPE_VLD)) {
		prWedRxInfo = kalMemAlloc(sizeof(struct WED_RX_INFO),
					  PHY_MEM_TYPE);
		if (!prWedRxInfo) {
			DBGLOG(HAL, WARN, "Failed to alloc WED RX INFO!!\n");
			return 0;
		}

		prWedRxInfo->pPacket = prSwRfb->pvPacket;
		prWedRxInfo->u2PpeEntry = ((DW3 & RXDMAD_PPE_ENTRY_MASK) >>
			RXDMAD_PPE_ENTRY_SHIFT);
		if (grWedInfo.fgCsrnExt)
			prWedRxInfo->ucCsrn =
			(((DW3 & RXDMAD_CSRN_MASK) >> RXDMAD_CSRN_SHIFT) |
			((DW2 & RXDMAD_CSRN_EXT_MASK) >>
				(RXDMAD_CSRN_EXT_SHIFT - 5)));
		else
			prWedRxInfo->ucCsrn =
			((DW3 & RXDMAD_CSRN_MASK) >> RXDMAD_CSRN_SHIFT);
		prSwRfb->prWedRxInfo = prWedRxInfo;
		WED_SET_PPE_TYPE(prSkb, RX_PPE_VALID);

		/* only extract info from rxdmad here, but not call warp hook
		 * PROXY_WLAN_HOOK_RX due the headroom space of pvPacket here
		 * is 0 and expand manually may cause kernel panic
		 */
	} else {
		WED_SET_PPE_TYPE(prSkb, RX_PPE_UNVALID);
	}

	/* do not drop packet even fdDrop is true for now*/
	fgDrop = (DW1 & (RXDMAD_RXD_DROP | RXDMAD_TO_HOST_A)) ? 1 : 0;

	return fgDrop;
}

void wedHwRxInfoFree(struct SW_RFB *prSwRfb)
{
	if (!prSwRfb)
		return;

	if (prSwRfb->prWedRxInfo) {
		kalMemFree(prSwRfb->prWedRxInfo, PHY_MEM_TYPE,
			   sizeof(struct WED_RX_INFO));
		prSwRfb->prWedRxInfo = NULL;
	}
}

uint32_t wedHwRxInfoWrapper(struct SW_RFB *prSwRfb)
{
	struct WED_RX_INFO *prWedRxInfo = NULL;
	struct sk_buff *prSkb = NULL;

	if ((prSwRfb == NULL) || (prSwRfb->prWedRxInfo == NULL))
		return 0;
	prWedRxInfo = (struct WED_RX_INFO *)prSwRfb->prWedRxInfo;

	if (!IsWedAttached())
		goto FREE_WED_RX_INFO;

	if (WED_GET_PPE_TYPE(prSwRfb->pvPacket) != RX_PPE_VALID)
		goto FREE_WED_RX_INFO;

	prSkb = (struct sk_buff *)prWedRxInfo->pPacket;

	if (skb_headroom(prSkb) < FOE_INFO_LEN) {
		DBGLOG(HAL, WARN,
			"SKB has no enough headroom (%d -> %d) bytes!\n",
			skb_headroom(prSkb), FOE_INFO_LEN);
		goto FREE_WED_RX_INFO;
	}

	wedProxyHookCall(PROXY_WLAN_HOOK_RX, prWedRxInfo);

FREE_WED_RX_INFO:
	wedHwRxInfoFree(prSwRfb);

	return 0;
}

uint32_t wedHwRxRequest(void *prSkb)
{
	struct WED_INFO *prwedinfo;

	if (!IsWedAttached())
		return 0;

	if (ra_sw_nat_hook_rx) {

		prwedinfo = &grWedInfo;

		if (prwedinfo->whnat_en &&
			WED_GET_PPE_TYPE(prSkb) == RX_PPE_VALID) {
			ra_sw_nat_hook_rx(prSkb);
		}
	}
	return 0;
}

bool wedRxSkbGen(struct GLUE_INFO *prGlueInfo,
	struct WED_DMABUF *prWedDmaBuf, void **prPacket)
{
	struct sk_buff *prSKB;
	uint32_t u4Size;

	/* This buffer belong to a SKB already */
	if (prWedDmaBuf->fgIsSKB) {
		*prPacket = prWedDmaBuf->pkt;
		return TRUE;
	}

	/* Generate SKB for this buffer */
	u4Size = SKB_DATA_ALIGN(prWedDmaBuf->AllocSize + NET_SKB_PAD) +
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	prSKB = build_skb(prWedDmaBuf->AllocVa, u4Size);
	if (prSKB == NULL) {
		DBGLOG(HAL, ERROR, "SKB build fail!\n");
		return FALSE;
	}
	*prPacket = (void *) prSKB;

	return TRUE;
}

bool wedRxBufferSwap(struct GLUE_INFO *prGlueInfo, uint32_t u4tokeID,
	struct WED_DMABUF *prWedDmaBuf, struct RXD_STRUCT *pRxD,
	struct RTMP_DMABUF *prDmaBuf, struct SW_RFB *prSwRfb)
{
#if CFG_SUPPORT_RX_ZERO_COPY
	struct GL_HIF_INFO *prHifInfo = NULL;
	dma_addr_t rAddr;
	void *prPacket;

	prHifInfo = &prGlueInfo->rHifInfo;

	/* Unmap RX buffer from warp BM*/
	KAL_DMA_UNMAP_SINGLE_ATTRS(prHifInfo->prDmaDev,
		(dma_addr_t) prWedDmaBuf->AllocPa,
		prWedDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);
	/* Generate SKB if needed */
	if (wedRxSkbGen(prGlueInfo, prWedDmaBuf, &prPacket) == FALSE)
		return FALSE;

	/* Swap buffer from SwRfb & WedDmaBuf */
	prWedDmaBuf->fgIsSKB = TRUE;
	prWedDmaBuf->pkt = prSwRfb->pvPacket;
	prWedDmaBuf->AllocVa = ((struct sk_buff *)prSwRfb->pvPacket)->data;
	prSwRfb->pvPacket = prPacket;

	rAddr = KAL_DMA_MAP_SINGLE_ATTRS(prHifInfo->prDmaDev,
		prWedDmaBuf->AllocVa, prWedDmaBuf->AllocSize,
		KAL_DMA_FROM_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		ASSERT(0);
		return FALSE;
	}
	prWedDmaBuf->AllocPa = rAddr;
#else
	kalMemCopy(((struct sk_buff *)prSwRfb->pvPacket)->data,
		   prWedDmaBuf->AllocVa,
		   pRxD->SDLen0);
#endif

	/* Align information to gen4m's DmaBuf & DMAD content */
	prDmaBuf->AllocPa = prWedDmaBuf->AllocPa;
	prDmaBuf->AllocVa = prWedDmaBuf->AllocVa;
	prDmaBuf->AllocSize = prWedDmaBuf->AllocSize;

	pRxD->SDPtr0 = prWedDmaBuf->AllocPa;
	pRxD->SDLen0 = prWedDmaBuf->AllocSize;
	pRxD->DMADONE = 0;
	pRxD->SDPtr1 = (u4tokeID << RXDMAD_TOKEN_ID_SHIFT);
	pRxD->SDLen1 = BIT(8);

	prSwRfb->pucRecvBuff = ((struct sk_buff *) prSwRfb->pvPacket)->data;
	prSwRfb->prRxStatus = (void *) prSwRfb->pucRecvBuff;

	return TRUE;
}

bool wedDevReadData(struct GLUE_INFO *prGlueInfo, uint16_t u2Port,
		    struct SW_RFB *prSwRfb)
{
	struct ADAPTER *prAdapter = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps;
	struct RXD_STRUCT *pRxD;
	struct RTMP_RX_RING *prRxRing;
	struct RTMP_DMACB *pRxCell;
	struct RTMP_DMABUF *prDmaBuf;
	struct WED_DMABUF *prWedDmaBuf;
	u_int8_t fgRet = TRUE;
	uint32_t u4CpuIdx = 0;
	uint32_t u4tokeID;

	ASSERT(prGlueInfo);

	if (!IsWedAttached())
		return kalDevReadData(prGlueInfo, u2Port, prSwRfb);

	prAdapter = prGlueInfo->prAdapter;
	prHifInfo = &prGlueInfo->rHifInfo;
	prMemOps = &prHifInfo->rMemOps;
	prRxRing = &prHifInfo->RxRing[u2Port];
	u4CpuIdx = prRxRing->RxCpuIdx;
	INC_RING_INDEX(u4CpuIdx, prRxRing->u4RingSize);

	pRxCell = &prRxRing->Cell[u4CpuIdx];
	pRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;

	u4tokeID = pRxD->SDPtr1 >> RXDMAD_TOKEN_ID_SHIFT;
	prWedDmaBuf = wedRxtokenGet(NULL, u4tokeID);
	if (!prWedDmaBuf)
		return FALSE;

	prDmaBuf = &pRxCell->DmaBuf;
	wedRxBufferSwap(prGlueInfo, u4tokeID,
		prWedDmaBuf, pRxD, prDmaBuf, prSwRfb);

	pRxCell->pPacket = prWedDmaBuf->pkt;

	prRxRing->RxCpuIdx = u4CpuIdx;
	prRxRing->fgIsDumpLog = false;

	if (prSwRfb->pvPacket)
		wedRxInfoGet(pRxD, prSwRfb);

	return fgRet;
}

bool IsWedAttached(void)
{
	return grWedInfo.fgAttached;
}

uint32_t wedRxTokenInit(void *priv, uint8_t fgIsSKB, void *pkt,
	unsigned long alloc_size, void *alloc_va, phys_addr_t alloc_pa)
{
	uint32_t u4token_id;
	struct WED_DMABUF *prWedDmaBuf;

	u4token_id = grWedToken.u4FreeIdx;
	prWedDmaBuf = &grWedToken.pkt_token[u4token_id];

	prWedDmaBuf->AllocSize = alloc_size;
	prWedDmaBuf->AllocPa = alloc_pa;
	prWedDmaBuf->AllocVa = alloc_va;
	prWedDmaBuf->pkt = pkt;
	prWedDmaBuf->fgIsSKB = fgIsSKB;

	grWedToken.u4FreeIdx++;

	if (grWedToken.u4FreeIdx == grWedToken.u4MaxSize)
		DBGLOG(HAL, STATE, "WED RX token full [%d]\n",
			grWedToken.u4MaxSize);

	return u4token_id;
}

struct WED_DMABUF *wedRxtokenGet(void *priv, uint32_t u4token_id)
{
	struct WED_DMABUF *prDmaBuf;

	if (u4token_id >= grWedToken.u4FreeIdx) {
		DBGLOG(HAL, STATE, "fail to get token with ID[%d]\n",
			u4token_id);
		return NULL;
	}
	if (grWedToken.pkt_token == NULL) {
		DBGLOG(HAL, STATE, "wed rx token not ready\n");
		return NULL;
	}
	prDmaBuf = &grWedToken.pkt_token[u4token_id];

	return prDmaBuf;
}

/* For debug purpose */
uint32_t wedMirrorRevert(void)
{
	grWedInfo.fgMirrorEnable = !grWedInfo.fgMirrorEnable;

	return 0;
}

static void wedSendCommand2WO(struct work_struct *work)
{
	struct QUE rTmpQue;
	struct QUE *prTmpQue = &rTmpQue;
	struct QUE *prWoCmdQueue = &rWoCmdQueue;
	struct WED_WO_CMD *rCmdInfo;
	int ret;

	mutex_lock(&rWoCmdMutex);
	QUEUE_MOVE_ALL(prTmpQue, prWoCmdQueue);
	mutex_unlock(&rWoCmdMutex);

	while (QUEUE_IS_NOT_EMPTY(prTmpQue)) {
		QUEUE_REMOVE_HEAD(prTmpQue, rCmdInfo, struct WED_WO_CMD *);
		if (!rCmdInfo)
			continue;
		if (IsWedAttached()) {
			ret = wedProxyHookCall(PROXY_WLAN_HOOK_SEND_CMD,
						&rCmdInfo->prWoCmdInfo);
			if (ret)
				DBGLOG(HAL, ERROR,
					"send wo cmd(%d) timeout\n",
					rCmdInfo->prWoCmdInfo.wo_cmd_id);
		}

		cnmMemFree(grWedInfo.pAdAdapter, rCmdInfo->prWoCmdInfo.pMsg);
		cnmMemFree(grWedInfo.pAdAdapter, rCmdInfo);
	}
}

uint32_t wedHwTxRequest(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	struct WED_MSDU_INFO *prWedTxInfo;
	uint8_t ucWlanIndex = 0;
	uint32_t u2Port = TX_RING_DATA0;
	struct BSS_INFO *prBssInfo;
	uint32_t ret;

	if (!IsWedAttached())
		return 0;

	prWedTxInfo = kalMemAlloc(sizeof(struct WED_MSDU_INFO),
				  PHY_MEM_TYPE);
	if (!prWedTxInfo) {
		DBGLOG(HAL, WARN, "Failed to alloc WED Tx Info!!\n");
		return -1;
	}

	prWedTxInfo->fgDrop = FALSE;

	/* prMsduInfo->ucWlanIndex not yet to be set, get wlan_idx here */
	ucWlanIndex = nicTxGetWlanIdx(prAdapter,
		prMsduInfo->ucBssIndex, prMsduInfo->ucStaRecIndex);

	/* wmmIdx to indicate different ring index */
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prMsduInfo->ucBssIndex);
	if (prBssInfo) {
		u2Port = (prBssInfo->ucWmmQueSet % 2) ?
			TX_RING_DATA1 : TX_RING_DATA0;
	}

	/* extract something from MSDU_INFO needed by PPE entry */
	prWedTxInfo->pPacket = prMsduInfo->prPacket; /* ptr to sk_buff */
	/* HIF_TxD v0.1 DW9[7:0] */
	prWedTxInfo->ucBssIndex = prMsduInfo->ucBssIndex;
	prWedTxInfo->ucWlanIndex = ucWlanIndex; /* HIF_TxD v0.1 DW9[15:8] */
	prWedTxInfo->ringIdx = u2Port;
	ret = wedProxyHookCall(PROXY_WLAN_HOOK_TX, prWedTxInfo);

	kalMemFree(prWedTxInfo, PHY_MEM_TYPE, sizeof(struct WED_MSDU_INFO));

	return ret;
}

static uint32_t wedStaRecUpdateBasic(
	struct ADAPTER *prAdapter,
	uint8_t *pMsgBuf,
	void *args)
{
	struct STA_RECORD *pStaRecCfg = (struct STA_RECORD *) args;
	struct STAREC_COMMON_WO StaRecCommon = {0};

	/* Fill TLV format */
	StaRecCommon.u2Tag = STA_REC_BASIC;
	StaRecCommon.u2Length = sizeof(struct STAREC_COMMON_WO);
	StaRecCommon.u4ConnectionType = CPU_TO_LE32(
		nicUniCmdStaRecConnType(prAdapter, pStaRecCfg->eStaType));
	/* OR pStaRecCfg->ucStaState ? */
	StaRecCommon.ucConnectionState = STATE_CONNECTED;
	StaRecCommon.ucIsQBSS = pStaRecCfg->fgIsQoS;
	StaRecCommon.u2AID = CPU_TO_LE16(pStaRecCfg->u2AssocId);
	kalMemCopy(&StaRecCommon.aucPeerMacAddr[0],
			   &pStaRecCfg->aucMacAddr[0], MAC_ADDR_LEN);

	/* New info to indicate this is new way to update STAREC */
	StaRecCommon.u2ExtraInfo = STAREC_COMMON_EXTRAINFO_V2;

	if (pStaRecCfg->ucStaState == STA_STATE_1)
		StaRecCommon.u2ExtraInfo |= STAREC_COMMON_EXTRAINFO_NEWSTAREC;

	/* Append this feature */
	kalMemCopy(pMsgBuf, (char *)&StaRecCommon,
			   sizeof(struct STAREC_COMMON_WO));
	return 0;
}

uint32_t wedStaRecUpdate(struct ADAPTER *prAdapter,
	struct STA_RECORD *pStaRecCfg)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t size;
	struct CMD_STAREC_UPDATE_WO *prCmdContent;
	struct WED_WO_CMD *rCmdInfo;
	struct QUE *prWoCmdQueue = &rWoCmdQueue;

	if (!IsWedAttached())
		return 0;

	if (!prAdapter)
		return 0;

	if (!pStaRecCfg) {
		DBGLOG(HAL, WARN, "STA_REC is NULL, skip!\n");
		return 0;
	}

	size = sizeof(struct CMD_STAREC_UPDATE_WO);
	size += sizeof(struct STAREC_COMMON_WO);

	prCmdContent = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, size);
	if (!prCmdContent) {
		DBGLOG(HAL, WARN, "command allocation failed!\n");
		return WLAN_STATUS_RESOURCES;
	}

	prCmdContent->ucBssIndex = pStaRecCfg->ucBssIndex;
	prCmdContent->ucWlanIdx = pStaRecCfg->ucWlanIndex;
	prCmdContent->u2TotalElementNum = 1;
	prCmdContent->ucAppendCmdTLV = TRUE;
	/* Multicast Index */
	prCmdContent->ucMuarIdx = 0;
	/* WCID_SET_H_L(prCmdContent->ucWlanIdxHnVer,
	 * prCmdContent->ucWlanIdx, pStaRecCfg->ucWlanIndex);
	 */
	prCmdContent->ucWlanIdxHnVer = 0;
	prCmdContent->ucWlanIdx = pStaRecCfg->ucWlanIndex;

	wedStaRecUpdateBasic(prAdapter,
		(uint8_t *)prCmdContent + sizeof(struct CMD_STAREC_UPDATE_WO),
		(void *)pStaRecCfg);

	rCmdInfo = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
				sizeof(struct WED_WO_CMD));
	if (!rCmdInfo) {
		DBGLOG(HAL, WARN, "rCmdInfo allocation failed!\n");
		cnmMemFree(prAdapter, prCmdContent);
		return WLAN_STATUS_RESOURCES;
	}
	rCmdInfo->prWoCmdInfo.pMsg = prCmdContent;
	rCmdInfo->prWoCmdInfo.u4MsgLen = size;
	rCmdInfo->prWoCmdInfo.wo_cmd_id = WO_CMD_STA_REC;

	mutex_lock(&rWoCmdMutex);
	QUEUE_INSERT_TAIL(prWoCmdQueue, rCmdInfo);
	mutex_unlock(&rWoCmdMutex);

	schedule_delayed_work(&rWedWoCmdWork, 0);

	return rWlanStatus;
}

static uint32_t wedStaRecRxAddBaUpdateBasic(
	struct ADAPTER *prAdapter,
	uint8_t *pMsgBuf,
	void *args)
{
	struct UNI_EVENT_RX_ADDBA *prRxAddBa = NULL;
	struct STAREC_BA_WO StaRecBa = {0};

	prRxAddBa = (struct UNI_EVENT_RX_ADDBA *) args;
	/* Fill TLV format */
	StaRecBa.u2Tag = STA_REC_BA;
	StaRecBa.u2Length = sizeof(struct STAREC_BA_WO);

	/* content */
	StaRecBa.ucTid = prRxAddBa->ucTid;
	StaRecBa.ucBaDirection = BA_SESSION_RECP;
	StaRecBa.ucAmsduCap = TRUE;
	StaRecBa.ucBaEenable = TRUE;
	StaRecBa.u2BaStartSeq = prRxAddBa->u2BAStartSeqCtrl;
	StaRecBa.u2BaWinSize = prRxAddBa->u2WinSize;

	kalMemCopy(pMsgBuf, (char *) &StaRecBa,
		sizeof(struct STAREC_BA_WO));
	return 0;
}
uint32_t wedStaRecRxAddBaUpdate(struct ADAPTER *prAdapter, void *ba)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct UNI_EVENT_RX_ADDBA *prRxAddBa = NULL;
	struct CMD_STAREC_UPDATE_WO *prCmdContent = NULL;
	uint8_t ucBssIndex = 0;
	uint32_t size;
	struct WED_WO_CMD *rCmdInfo;
	struct QUE *prWoCmdQueue = &rWoCmdQueue;

	if (!IsWedAttached())
		return 0;

	if (!prAdapter)
		return 0;

	if (!ba) {
		DBGLOG(HAL, WARN, "RX_ADDBA is NULL, skip!\n");
		return 0;
	}

	size = sizeof(struct CMD_STAREC_UPDATE_WO);
	size += sizeof(struct STAREC_BA_WO);

	prRxAddBa = (struct UNI_EVENT_RX_ADDBA *)ba;

	prCmdContent = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, size);

	if (!prCmdContent) {
		DBGLOG(HAL, WARN, "command allocation failed!\n");
		return WLAN_STATUS_RESOURCES;
	}

	ucBssIndex = secGetBssIdxByWlanIdx(prAdapter, prRxAddBa->u2WlanIdx);

	prCmdContent->ucBssIndex = ucBssIndex;
	prCmdContent->ucWlanIdx = prRxAddBa->u2WlanIdx;
	prCmdContent->u2TotalElementNum = 1;
	prCmdContent->ucAppendCmdTLV = TRUE;
	/* Multicast Index */
	prCmdContent->ucMuarIdx = 0;
	WCID_SET_H_L(prCmdContent->ucWlanIdxHnVer,
		prCmdContent->ucWlanIdx, prRxAddBa->u2WlanIdx);

	wedStaRecRxAddBaUpdateBasic(prAdapter,
		(uint8_t *) prCmdContent + sizeof(struct CMD_STAREC_UPDATE_WO),
		(void *) prRxAddBa);

	rCmdInfo = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
				sizeof(struct WED_WO_CMD));
	if (!rCmdInfo) {
		DBGLOG(HAL, WARN, "rCmdInfo allocation failed!\n");
		cnmMemFree(prAdapter, prCmdContent);
		return WLAN_STATUS_RESOURCES;
	}
	rCmdInfo->prWoCmdInfo.pMsg = prCmdContent;
	rCmdInfo->prWoCmdInfo.u4MsgLen = size;
	rCmdInfo->prWoCmdInfo.wo_cmd_id = WO_CMD_STA_REC;

	mutex_lock(&rWoCmdMutex);
	QUEUE_INSERT_TAIL(prWoCmdQueue, rCmdInfo);
	mutex_unlock(&rWoCmdMutex);

	schedule_delayed_work(&rWedWoCmdWork, 0);

	return rWlanStatus;
}

static u_int8_t wedIsAPUp(struct GLUE_INFO *prGlueInfo)
{
	struct GL_P2P_INFO *prP2pInfo = NULL;
	uint8_t ucIdx = 0;
	int ret = 0;

	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL!\n");
		return ret;
	}

	for (ucIdx = 0; ucIdx < KAL_P2P_NUM; ucIdx++) {
		prP2pInfo = prGlueInfo->prP2PInfo[ucIdx];
		/* Expect that only create the new dev with the ap0 */
		if (prP2pInfo == NULL || prP2pInfo->prWdev == NULL)
			continue;

		if (prP2pInfo->prDevHandler == NULL)
			continue;

		if (prP2pInfo->prWdev->iftype != NL80211_IFTYPE_AP)
			continue;

		if (netif_carrier_ok(prP2pInfo->prDevHandler)) {
			ret = 1;
			break;
		}
	}

	return ret;
}

void wedSuspendTrigger(void)
{
	struct WED_INFO *prwedinfo;
	struct ADAPTER *prAdapter;
	struct net_device *prNetDev;
	int i;

	DBGLOG(HAL, STATE, "WED Suspend Start!\n");

	prwedinfo = &grWedInfo;
	prAdapter = prwedinfo->pAdAdapter;

	if (fgIsWedInSuspend == TRUE) {
		DBGLOG(HAL, STATE, "WED not in Resume!\n");
		return;
	}
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		prNetDev = grNetList[i];
		if (!prNetDev)
			continue;
		DBGLOG(HAL, STATE, "suspend detach %s!\n", prNetDev->name);
		wedNetDevStash(prNetDev);
		wedDetachWarp(prAdapter, prNetDev, WED_DETACH_SUSPEND);
	}
	fgIsWedInSuspend = TRUE;

	INC_CNT(g_u4SuspendCnt);
	DBGLOG(HAL, STATE, "WED Suspend Done! CNT: %u\n", g_u4SuspendCnt);
}

void wedResumeTrigger(void)
{
	struct WED_INFO *prwedinfo;
	struct ADAPTER *prAdapter;
	struct net_device *prNetDev;
	int i;

	DBGLOG(HAL, STATE, "WED Resume Start!\n");

	prwedinfo = &grWedInfo;
	prAdapter = prwedinfo->pAdAdapter;

	if (fgIsWedInSuspend == FALSE) {
		DBGLOG(HAL, STATE, "WED not in Suspend!\n");
		return;
	}
	fgIsWedInSuspend = FALSE;
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		prNetDev = wedNetDevStashPop();
		if (!prNetDev)
			break;
		DBGLOG(HAL, STATE, "resume attach %s!\n", prNetDev->name);
		wedAttachWarp(prAdapter, prNetDev, WED_ATTACH_RESUME);
		if (!netif_carrier_ok(prNetDev))
			DBGLOG(HAL, STATE, "%s is down ???\n", prNetDev->name);
	}
	INC_CNT(g_u4ResumeCnt);
	wedUpdateStaRecAndBa(prAdapter);
	DBGLOG(HAL, STATE, "WED Resume Done! CNT: %u\n", g_u4ResumeCnt);
}
