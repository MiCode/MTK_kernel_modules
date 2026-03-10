/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
/*
 * Id: @(#) hif_mem.c@@
 */
/*  \file   hif_mem.c
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of.h>

#include "gl_os.h"

#include "hif_pdma.h"

#include "precomp.h"

#include "mt66xx_reg.h"
#include "gl_kal.h"

#if CFG_SUPPORT_RX_PAGE_POOL
#include <net/page_pool.h>
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define HAL_TX_MAX_SIZE_PER_FRAME         (NIC_TX_MAX_SIZE_PER_FRAME +      \
					   NIC_TX_DESC_AND_PADDING_LENGTH)
#define HAL_TX_CMD_BUFF_SIZE              4096

#if CFG_SUPPORT_RX_PAGE_POOL
#define HAL_PAGE_POOL_PAGE_INC_NUM              512
#define HAL_PAGE_POOL_PAGE_DEC_NUM              32
#define HAL_PAGE_POOL_PAGE_UPDATE_MIN_INTERVAL  500
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct wifi_rsrv_mem {
	phys_addr_t phy_base;
	void *vir_base;
	unsigned long long size;
};

struct HIF_PREALLOC_MEM {
	struct HIF_MEM rTxDesc[NUM_OF_TX_RING];
	struct HIF_MEM rRxDesc[NUM_OF_RX_RING];
	/* Tx Command */
	struct HIF_MEM rTxCmdBuf[TX_RING_CMD_SIZE];
	/* FWDL */
	struct HIF_MEM rTxFwdlBuf[TX_RING_CMD_SIZE];
	/* Rx Data/Event */
	struct HIF_MEM rRxMemBuf[NUM_OF_RX_RING][RX_RING_MAX_SIZE];

#if HIF_TX_PREALLOC_DATA_BUFFER
	/* Tx Data */
	struct HIF_MEM rMsduBuf[HIF_TX_MSDU_TOKEN_NUM];
#endif
	phys_addr_t pucRsvMemBase;
	void *pucRsvMemVirBase;
	uint64_t u4RsvMemSize;
	uint32_t u4Offset;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static struct HIF_PREALLOC_MEM grMem;
static unsigned long long gWifiRsvMemSize;
/* Assume reserved memory size < BIT(32) */
static struct wifi_rsrv_mem wifi_rsrv_mems[32];

#if CFG_MTK_WIFI_SW_EMI_RING
struct HIF_MEM g_rRsvEmiMem;
#endif

#if CFG_SUPPORT_RX_PAGE_POOL
static struct sk_buff_head g_rHifSkbList;

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
static uint32_t g_u4HifRsvNum;
static uint32_t g_u4CurPageNum;
static uint32_t g_u4MinPageNum;
static uint32_t g_u4MaxPageNum;
static unsigned long g_ulUpdatePagePoolPagePeriod;
static struct mutex g_rPageLock;
#endif
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static bool halGetRsvMemSizeRsvedByKernel(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	int ret = 0;
	struct device_node *np;

	np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!np) {
		DBGLOG(INIT, ERROR, "can NOT find memory-region.\n");
		return false;
	}

	ret = of_property_read_u64_array(np, "size", &gWifiRsvMemSize, 1);
	if (ret != 0)
		DBGLOG(INIT, ERROR, "get rsrv mem size failed(%d).\n", ret);
	else
		DBGLOG(INIT, INFO, "gWifiRsvMemSize: 0x%x\n", gWifiRsvMemSize);

	of_node_put(np);
	if (ret != 0)
		return false;
	else
		return true;
#else
	return false;
#endif
}

int halInitResvMem(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	int ret = 0;
	struct device_node *node = NULL;
	unsigned int RsvMemSize = 0;

	node = pdev->dev.of_node;
	if (!node) {
		DBGLOG(INIT, ERROR, "WIFI-OF: get wifi device node fail\n");
		of_node_put(node);
		return false;
	}

	if (halGetRsvMemSizeRsvedByKernel(pdev) == false) {
		ret = of_property_read_u32(node, "emi-size", &RsvMemSize);
		if (ret != 0)
			DBGLOG(INIT, ERROR,
				"MPU-in-lk get rsrv mem size failed(%d).\n",
				ret);
		else {
			gWifiRsvMemSize = (unsigned long long) RsvMemSize;
			DBGLOG(INIT, INFO, "MPU-in-lk gWifiRsvMemSize: 0x%x\n",
				gWifiRsvMemSize);
		}
	}

	of_node_put(node);

	return ret;
#else
	DBGLOG(INIT, ERROR, "kernel option CONFIG_OF not enabled.\n");
	return -1;
#endif
}

static bool halAllocRsvMem(uint32_t u4Size, struct HIF_MEM *prMem)
{
	/* 8 bytes alignment */
	if (u4Size & 7)
		u4Size += 8 - (u4Size & 7);

	if ((grMem.u4Offset + u4Size) >= gWifiRsvMemSize) {
		prMem->pa = 0;
		prMem->va = NULL;
		return false;
	}

	prMem->pa = grMem.pucRsvMemBase + grMem.u4Offset;
	prMem->va = grMem.pucRsvMemVirBase + grMem.u4Offset;
	grMem.u4Offset += u4Size;

	return prMem->va != NULL;
}

static bool halFreeRsvMem(uint32_t u4Size)
{
	/* 8 bytes alignment */
	if (u4Size & 7)
		u4Size += 8 - (u4Size & 7);

	if (u4Size > grMem.u4Offset)
		return false;

	grMem.u4Offset -= u4Size;

	return true;
}

static int halInitHifMem(struct platform_device *pdev,
		  struct mt66xx_chip_info *prChipInfo)
{
	uint32_t i = sizeof(wifi_rsrv_mems) / sizeof(struct wifi_rsrv_mem);

	/* Allocation size should be a power of two */
	while (i > 0) {
		i--;
		if (!(gWifiRsvMemSize & BIT(i)))
			continue;

		wifi_rsrv_mems[i].size = BIT(i);
		wifi_rsrv_mems[i].vir_base =
			dma_alloc_coherent(&pdev->dev,
					   wifi_rsrv_mems[i].size,
					   &wifi_rsrv_mems[i].phy_base,
					   GFP_DMA);
		if (!wifi_rsrv_mems[i].vir_base) {
			DBGLOG(INIT, ERROR,
				"[%d] DMA_ALLOC_COHERENT failed, size: 0x%x\n",
				i, wifi_rsrv_mems[i].size);
			return -1;
		}
		if (!grMem.pucRsvMemBase) {
			grMem.pucRsvMemBase = wifi_rsrv_mems[i].phy_base;
			grMem.pucRsvMemVirBase = wifi_rsrv_mems[i].vir_base;
			grMem.u4RsvMemSize = (uint64_t) gWifiRsvMemSize;
		}
	}

	if (!grMem.pucRsvMemBase)
		return -1;

	DBGLOG(INIT, INFO, "pucRsvMemBase[%pa], pucRsvMemVirBase[%pa]\n",
	       &grMem.pucRsvMemBase,
	       &grMem.pucRsvMemVirBase);

	return 0;
}

int halAllocHifMem(struct platform_device *pdev,
		   struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Idx, u4Size, u4EvtNum, u4DataNum;

	prChipInfo = prDriverData->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (halInitHifMem(pdev, prChipInfo) == -1)
		return -1;

#if CFG_MTK_WIFI_SW_EMI_RING
	if (!halAllocRsvMem(SW_EMI_MEMORY_SIZE, &g_rRsvEmiMem))
		DBGLOG(INIT, ERROR, "g_rRsvEmiMem alloc fail\n");
#endif

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		if (u4Idx == TX_RING_DATA1 &&
		    !prBusInfo->tx_ring1_data_idx)
			continue;
		else if (u4Idx == TX_RING_DATA_PRIO &&
			 !prBusInfo->tx_ring2_data_idx)
			continue;
		else if (u4Idx == TX_RING_DATA_ALTX &&
			 !prBusInfo->tx_ring3_data_idx)
			continue;

		if (u4Idx == TX_RING_DATA0 ||
		    u4Idx == TX_RING_DATA1 ||
		    u4Idx == TX_RING_DATA_PRIO ||
		    u4Idx == TX_RING_DATA_ALTX)
			u4Size = TX_RING_DATA_SIZE;
		else
			u4Size = TX_RING_CMD_SIZE;

		if (!halAllocRsvMem(u4Size * TXD_SIZE, &grMem.rTxDesc[u4Idx]))
			DBGLOG(INIT, ERROR, "TxDesc[%u] alloc fail\n", u4Idx);
	}

	u4DataNum = prBusInfo->rx_data_ring_num;
	u4EvtNum = prBusInfo->rx_evt_ring_num;
	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		if (u4Idx == RX_RING_DATA0 || u4Idx == RX_RING_DATA1 ||
		    u4Idx == RX_RING_DATA2) {
			if (u4DataNum == 0)
				continue;

			u4Size = prBusInfo->rx_data_ring_size;
			u4DataNum--;
		} else {
			if (u4EvtNum == 0)
				continue;

			u4Size = prBusInfo->rx_evt_ring_size;
			u4EvtNum--;
		}
		if (!halAllocRsvMem(u4Size * RXD_SIZE, &grMem.rRxDesc[u4Idx]))
			DBGLOG(INIT, ERROR, "RxDesc[%u] alloc fail\n", u4Idx);
	}

	for (u4Idx = 0; u4Idx < TX_RING_CMD_SIZE; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_CMD_BUFF_SIZE,
				    &grMem.rTxCmdBuf[u4Idx]))
			DBGLOG(INIT, ERROR, "TxCmdBuf[%u] alloc fail\n", u4Idx);
	}

	for (u4Idx = 0; u4Idx < TX_RING_CMD_SIZE; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_CMD_BUFF_SIZE,
				    &grMem.rTxFwdlBuf[u4Idx]))
			DBGLOG(INIT, ERROR,
				"TxFwdlBuf[%u] alloc fail\n",
				u4Idx);
	}

#if (CFG_SUPPORT_RX_PAGE_POOL == 0) || (CFG_SUPPORT_DYNAMIC_PAGE_POOL == 1)
	u4DataNum = prBusInfo->rx_data_ring_num;
#else
	u4DataNum = 0;
#endif /* CFG_SUPPORT_RX_PAGE_POOL == 0 || CFG_SUPPORT_DYNAMIC_PAGE_POOL == 1 */
	u4EvtNum = prBusInfo->rx_evt_ring_num;
	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		uint32_t u4Cnt, u4PktSize;

		if (u4Idx == RX_RING_DATA0 || u4Idx == RX_RING_DATA1 ||
		    u4Idx == RX_RING_DATA2) {
			if (u4DataNum == 0)
				continue;

			/* copy path using prealloc rx data size */
			u4Size = prBusInfo->rx_data_ring_prealloc_size;
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
			u4DataNum--;
		} else {
			if (u4EvtNum == 0)
				continue;

			u4Size = prBusInfo->rx_evt_ring_size;
			u4PktSize = RX_BUFFER_AGGRESIZE;
			u4EvtNum--;
		}
		for (u4Cnt = 0; u4Cnt < u4Size; u4Cnt++) {
			if (!halAllocRsvMem(u4PktSize,
					    &grMem.rRxMemBuf[u4Idx][u4Cnt])) {
				DBGLOG(INIT, ERROR,
				       "RxMemBuf[%u][%u] alloc fail\n",
				       u4Idx, u4Cnt);
			}
		}
	}

#if HIF_TX_PREALLOC_DATA_BUFFER
	for (u4Idx = 0; u4Idx < HIF_TX_MSDU_TOKEN_NUM; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_MAX_SIZE_PER_FRAME +
				    prChipInfo->txd_append_size,
				    &grMem.rMsduBuf[u4Idx]))
			DBGLOG(INIT, ERROR, "MsduBuf[%u] alloc fail\n", u4Idx);
	}
#endif

	DBGLOG(INIT, INFO, "grMem.u4Offset[0x%x]\n", grMem.u4Offset);

	return 0;
}

void halFreeHifMem(struct platform_device *pdev)
{
	uint32_t i = 0;
	uint32_t count = sizeof(wifi_rsrv_mems) / sizeof(struct wifi_rsrv_mem);

	for (i = 0; i < count; i++) {
		if (!wifi_rsrv_mems[i].vir_base)
			continue;
		dma_free_coherent(
			&pdev->dev,
			wifi_rsrv_mems[i].size,
			wifi_rsrv_mems[i].vir_base,
			(dma_addr_t) wifi_rsrv_mems[i].phy_base);
	}
}

void halCopyPathAllocTxDesc(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing,
			   uint32_t u4Num)
{
	prDescRing->AllocVa = grMem.rTxDesc[u4Num].va;
	prDescRing->AllocPa = grMem.rTxDesc[u4Num].pa;
	if (prDescRing->AllocVa == NULL)
		DBGLOG(HAL, ERROR, "prDescRing->AllocVa is NULL\n");
	else
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

void halCopyPathAllocRxDesc(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing,
			   uint32_t u4Num)
{
	prDescRing->AllocVa = grMem.rRxDesc[u4Num].va;
	prDescRing->AllocPa = grMem.rRxDesc[u4Num].pa;
	if (prDescRing->AllocVa == NULL)
		DBGLOG(HAL, ERROR, "prDescRing->AllocVa is NULL\n");
	else
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

void halCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing)
{
	struct HIF_MEM rMem = {0};

	if (!halAllocRsvMem(prDescRing->AllocSize, &rMem)) {
		DBGLOG(INIT, ERROR, "Ext Buf alloc failed!\n");
		prDescRing->AllocPa = 0;
		prDescRing->AllocVa = NULL;
		return;
	}

	prDescRing->AllocVa = rMem.va;
	prDescRing->AllocPa = rMem.pa;
	if (prDescRing->AllocVa)
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

bool halCopyPathAllocTxCmdBuf(struct RTMP_DMABUF *prDmaBuf,
			     uint32_t u4Num, uint32_t u4Idx)
{
	if (u4Num != TX_RING_CMD && u4Num != TX_RING_FWDL)
		return true;

	prDmaBuf->AllocSize = HAL_TX_CMD_BUFF_SIZE;
	if (u4Num == TX_RING_CMD) {
		prDmaBuf->AllocPa = grMem.rTxCmdBuf[u4Idx].pa;
		prDmaBuf->AllocVa = grMem.rTxCmdBuf[u4Idx].va;
	} else if (u4Num == TX_RING_FWDL) {
		prDmaBuf->AllocPa = grMem.rTxFwdlBuf[u4Idx].pa;
		prDmaBuf->AllocVa = grMem.rTxFwdlBuf[u4Idx].va;
	}
	if (prDmaBuf->AllocVa  == NULL) {
		DBGLOG(HAL, ERROR, "AllocVa is NULL[%u][%u]\n", u4Num, u4Idx);
		return false;
	}
	kalMemZero(prDmaBuf->AllocVa, prDmaBuf->AllocSize);

	return true;
}

void halCopyPathAllocTxDataBuf(struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx)
{
	prToken->prPacket = grMem.rMsduBuf[u4Idx].va;
	prToken->rDmaAddr = grMem.rMsduBuf[u4Idx].pa;
}

void *halCopyPathAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDmaBuf,
			    uint32_t u4Num, uint32_t u4Idx)
{
	if (u4Num >= NUM_OF_RX_RING || u4Idx >= RX_RING_MAX_SIZE) {
		DBGLOG(RX, ERROR, "RX alloc fail error number=%d idx=%d\n",
		       u4Num, u4Idx);
		return prDmaBuf->AllocVa;
	}

	prDmaBuf->AllocPa = grMem.rRxMemBuf[u4Num][u4Idx].pa;
	prDmaBuf->AllocVa = grMem.rRxMemBuf[u4Num][u4Idx].va;
	prDmaBuf->fgIsCopyPath = TRUE;

	if (prDmaBuf->AllocVa == NULL)
		DBGLOG(HAL, ERROR, "AllocVa is NULL[%u][%u]\n", u4Num, u4Idx);
	else
		memset(prDmaBuf->AllocVa, 0, prDmaBuf->AllocSize);

	return prDmaBuf->AllocVa;
}

bool halCopyPathCopyCmd(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_DMACB *prTxCell, void *pucBuf,
		       void *pucSrc1, uint32_t u4SrcLen1,
		       void *pucSrc2, uint32_t u4SrcLen2)
{
	struct RTMP_DMABUF *prDmaBuf = &prTxCell->DmaBuf;

	memcpy(prDmaBuf->AllocVa, pucSrc1, u4SrcLen1);
	if (pucSrc2 != NULL && u4SrcLen2 > 0)
		memcpy(prDmaBuf->AllocVa + u4SrcLen1, pucSrc2, u4SrcLen2);
	prTxCell->PacketPa = prDmaBuf->AllocPa;

	return true;
}

bool halCopyPathCopyEvent(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMACB *pRxCell,
			 struct RXD_STRUCT *pRxD,
			 struct RTMP_DMABUF *prDmaBuf,
			 uint8_t *pucDst, uint32_t u4Len)
{
	memcpy(pucDst, prDmaBuf->AllocVa, u4Len);

	return true;
}

bool halCopyPathCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			  void *pucSrc, uint32_t u4Len)
{
	memcpy(prToken->prPacket, pucSrc, u4Len);

	return true;
}

bool halCopyPathCopyRxData(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMACB *pRxCell,
			  struct RTMP_DMABUF *prDmaBuf,
			  struct SW_RFB *prSwRfb)
{
	struct RXD_STRUCT *pRxD = (struct RXD_STRUCT *)pRxCell->AllocVa;
	struct sk_buff *prSkb = ((struct sk_buff *)prSwRfb->pvPacket);
	uint32_t u4Size = pRxD->SDLen0;

	if (prSkb == NULL) {
		DBGLOG(RX, ERROR, "prSkb == NULL\n");
		return false;
	}

	if (prSkb->data == NULL) {
		DBGLOG(RX, ERROR, "prSkb->data == NULL\n");
		return false;
	}

	if (u4Size > CFG_RX_MAX_PKT_SIZE) {
		DBGLOG(RX, ERROR, "Rx Data too large[%u]\n", u4Size);
		return false;
	}

	memcpy(prSkb->data, prDmaBuf->AllocVa, u4Size);

	return true;
}

void halCopyPathFreeExtBuf(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMABUF *prDescRing)
{
	if (prDescRing->AllocVa == NULL)
		return;

	halFreeRsvMem(prDescRing->AllocSize);
	memset(prDescRing, 0, sizeof(struct RTMP_DMABUF));
}

void halCopyPathDumpTx(struct GL_HIF_INFO *prHifInfo,
		      struct RTMP_TX_RING *prTxRing,
		      uint32_t u4Idx, uint32_t u4DumpLen)
{
	struct RTMP_DMACB *prTxCell;
	struct RTMP_DMABUF *prDmaBuf;
	void *prAddr = NULL;

	prTxCell = &prTxRing->Cell[u4Idx];
	prDmaBuf = &prTxCell->DmaBuf;

	if (prTxCell->prToken)
		prAddr = prTxCell->prToken->prPacket;
	else if (prDmaBuf->AllocVa)
		prAddr = prDmaBuf->AllocVa;

	if (prAddr)
		DBGLOG_MEM32(HAL, INFO, prAddr, u4DumpLen);
}

void halCopyPathDumpRx(struct GL_HIF_INFO *prHifInfo,
		      struct RTMP_RX_RING *prRxRing,
		      uint32_t u4Idx, uint32_t u4DumpLen)
{
	struct RTMP_DMACB *prRxCell;
	struct RTMP_DMABUF *prDmaBuf;

	prRxCell = &prRxRing->Cell[u4Idx];
	prDmaBuf = &prRxCell->DmaBuf;

	if (prRxCell->pPacket)
		DBGLOG_MEM32(HAL, INFO, prRxCell->pPacket, u4DumpLen);
}

void halZeroCopyPathAllocDesc(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMABUF *prDescRing,
			  uint32_t u4Num)
{
	dma_addr_t rAddr;

	prDescRing->AllocVa = KAL_DMA_ALLOC_COHERENT(
		prHifInfo->prDmaDev, prDescRing->AllocSize, &rAddr);
	prDescRing->AllocPa = (phys_addr_t)rAddr;

	if (prDescRing->AllocVa)
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

void halZeroCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing)
{
	halZeroCopyPathAllocDesc(prHifInfo, prDescRing, 0);
}

void halZeroCopyPathAllocTxDataBuf(struct MSDU_TOKEN_ENTRY *prToken,
				   uint32_t u4Idx)
{
	prToken->prPacket = kalMemAlloc(prToken->u4DmaLength, PHY_MEM_TYPE);
	prToken->rDmaAddr = 0;
}

static u_int8_t halDmaMapSingleRetry(struct GL_HIF_INFO *prHifInfo,
				     void *AllocVa, unsigned long AllocSize,
				     int i4Dir, dma_addr_t *prAddr)
{
#define DMA_MAP_RETRY_COUNT 10
	dma_addr_t rAddr;
	u_int8_t fgRet = TRUE;
	int i4Res = 0, u4Cnt;

	for (u4Cnt = 0; u4Cnt < DMA_MAP_RETRY_COUNT; u4Cnt++) {
		rAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, AllocVa,
					   AllocSize, i4Dir);
		i4Res = KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr);
		if (!i4Res)
			break;

		if (prHifInfo->pdev) {
			DBGLOG(INIT, WARN,
			       "va: 0x%llx, mask: 0x%llx, limit: 0x%llx\n",
			       AllocVa,
			       *prHifInfo->pdev->dev.dma_mask,
			       prHifInfo->pdev->dev.bus_dma_limit);
		}

		kalMsleep(1);
	}
	if (u4Cnt == DMA_MAP_RETRY_COUNT) {
		DBGLOG(HAL, ERROR,
		       "dma mapping error![0x%llx][0x%llx][%u][%d]\n",
		       (uint64_t)rAddr, AllocVa, AllocSize, i4Res);
		fgRet = FALSE;
	}

	*prAddr = rAddr;
	return fgRet;
}

void *halZeroCopyPathAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDmaBuf,
			    uint32_t u4Num, uint32_t u4Idx)
{
	struct sk_buff *pkt = dev_alloc_skb(prDmaBuf->AllocSize);
	dma_addr_t rAddr;

	if (!pkt) {
		DBGLOG(HAL, ERROR, "can't allocate rx %lu size packet\n",
		       prDmaBuf->AllocSize);
		prDmaBuf->AllocPa = 0;
		prDmaBuf->AllocVa = NULL;
		return NULL;
	}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	skb_reserve(pkt, CFG_RADIOTAP_HEADROOM);
#endif
	prDmaBuf->AllocVa = (void *)pkt->data;
	memset(prDmaBuf->AllocVa, 0, prDmaBuf->AllocSize);

	rAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, prDmaBuf->AllocVa,
				   prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr)) {
		DBGLOG(HAL, ERROR, "sk_buff dma mapping error!\n");
		dev_kfree_skb(pkt);
		return NULL;
	}
	prDmaBuf->AllocPa = (phys_addr_t)rAddr;
	prDmaBuf->fgIsCopyPath = FALSE;
	return (void *)pkt;
}

void *halZeroCopyPathAllocRuntimeMem(uint32_t u4SrcLen)
{
	return kalMemAlloc(u4SrcLen, PHY_MEM_TYPE);
}

bool halZeroCopyPathCopyCmd(struct GL_HIF_INFO *prHifInfo,
			struct RTMP_DMACB *prTxCell, void *pucBuf,
			void *pucSrc1, uint32_t u4SrcLen1,
			void *pucSrc2, uint32_t u4SrcLen2)
{
	dma_addr_t rAddr;
	uint32_t u4TotalLen = u4SrcLen1 + u4SrcLen2;

	prTxCell->pBuffer = pucBuf;

	memcpy(pucBuf, pucSrc1, u4SrcLen1);
	if (pucSrc2 != NULL && u4SrcLen2 > 0)
		memcpy(pucBuf + u4SrcLen1, pucSrc2, u4SrcLen2);
	rAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, pucBuf,
				   u4TotalLen, KAL_DMA_TO_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		return false;
	}

	prTxCell->PacketPa = (phys_addr_t)rAddr;

	return true;
}

bool halZeroCopyPathCopyEvent(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMACB *pRxCell,
			  struct RXD_STRUCT *pRxD,
			  struct RTMP_DMABUF *prDmaBuf,
			  uint8_t *pucDst, uint32_t u4Len)
{
	struct sk_buff *prSkb = NULL;
	void *pRxPacket = NULL;
	dma_addr_t rAddr;

	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)prDmaBuf->AllocPa,
			     prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);

	pRxPacket = pRxCell->pPacket;
	ASSERT(pRxPacket)

	prSkb = (struct sk_buff *)pRxPacket;
	memcpy(pucDst, (uint8_t *)prSkb->data, u4Len);

	prDmaBuf->AllocVa = ((struct sk_buff *)pRxCell->pPacket)->data;
	rAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, prDmaBuf->AllocVa,
				   prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		return false;
	}
	prDmaBuf->AllocPa = (phys_addr_t)rAddr;
	return true;
}

bool halZeroCopyPathCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			   void *pucSrc, uint32_t u4Len)
{
	memcpy(prToken->prPacket, pucSrc, u4Len);
	return true;
}

bool halZeroCopyPathCopyRxData(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMACB *pRxCell,
			   struct RTMP_DMABUF *prDmaBuf,
			   struct SW_RFB *prSwRfb)
{
	struct sk_buff *prSkb = (struct sk_buff *)prSwRfb->pvPacket;
	void *pRxPacket = NULL;
	dma_addr_t rAddr;

	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)prDmaBuf->AllocPa,
			     prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);

#if CFG_SUPPORT_RX_PAGE_POOL
	if (!prSkb->pp_recycle) {
		halCopyPathCopyRxData(prHifInfo, pRxCell, prDmaBuf, prSwRfb);
		goto dma_map;
	}
#endif

	pRxPacket = pRxCell->pPacket;
	ASSERT(pRxPacket);

	pRxCell->pPacket = prSkb;
	prSwRfb->pvPacket = pRxPacket;
	prDmaBuf->AllocVa = ((struct sk_buff *)pRxCell->pPacket)->data;

#if CFG_SUPPORT_RX_PAGE_POOL
dma_map:
#endif
	if (!halDmaMapSingleRetry(prHifInfo, prDmaBuf->AllocVa,
				  prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE,
				  &rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		return false;
	}
	prDmaBuf->AllocPa = (phys_addr_t)rAddr;

	return true;
}

phys_addr_t halZeroCopyPathMapTxBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len)
{
	dma_addr_t rDmaAddr;

	rDmaAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, pucBuf + u4Offset,
				      u4Len, KAL_DMA_TO_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rDmaAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		return 0;
	}

	return (phys_addr_t)rDmaAddr;
}

phys_addr_t halZeroCopyPathMapRxBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len)
{
	dma_addr_t rDmaAddr;

	rDmaAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, pucBuf + u4Offset,
				      u4Len, KAL_DMA_FROM_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rDmaAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		return 0;
	}

	return (phys_addr_t)rDmaAddr;
}

void halZeroCopyPathUnmapTxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len)
{
	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)rDmaAddr,
			     u4Len, KAL_DMA_TO_DEVICE);
}

void halZeroCopyPathUnmapRxBuf(struct GL_HIF_INFO *prHifInfo,
			       phys_addr_t rDmaAddr, uint32_t u4Len)
{
	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)rDmaAddr,
			     u4Len, KAL_DMA_FROM_DEVICE);
}

void halZeroCopyPathFreeDesc(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMABUF *prDescRing)
{
	if (prDescRing->AllocVa == NULL)
		return;

	KAL_DMA_FREE_COHERENT(prHifInfo->prDmaDev,
			      prDescRing->AllocSize,
			      prDescRing->AllocVa,
			      (dma_addr_t)prDescRing->AllocPa);
	memset(prDescRing, 0, sizeof(struct RTMP_DMABUF));
}

void halZeroCopyPathFreeBuf(void *pucSrc, uint32_t u4Len)
{
	kalMemFree(pucSrc, PHY_MEM_TYPE, u4Len);
}

void halZeroCopyPathFreePacket(struct GL_HIF_INFO *prHifInfo,
			   void *pvPacket, uint32_t u4Num)
{
	kalPacketFree(NULL, pvPacket);
}

void halZeroCopyPathDumpTx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_TX_RING *prTxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen)
{
	struct RTMP_DMACB *prTxCell;
	void *prAddr = NULL;

	prTxCell = &prTxRing->Cell[u4Idx];

	if (prTxCell->prToken)
		prAddr = prTxCell->prToken->prPacket;
	else
		prAddr = prTxCell->pBuffer;

	if (prAddr)
		DBGLOG_MEM32(HAL, INFO, prAddr, u4DumpLen);
}

void halZeroCopyPathDumpRx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen)
{
	struct RTMP_DMACB *prRxCell;
	struct RTMP_DMABUF *prDmaBuf;

	prRxCell = &prRxRing->Cell[u4Idx];
	prDmaBuf = &prRxCell->DmaBuf;

	if (!prRxCell->pPacket || !prDmaBuf)
		return;

	if (prDmaBuf->fgIsCopyPath) {
		halCopyPathDumpRx(prHifInfo, prRxRing, u4Idx, u4DumpLen);
		return;
	}

	halZeroCopyPathUnmapRxBuf(prHifInfo, prDmaBuf->AllocPa,
				  prDmaBuf->AllocSize);

	DBGLOG_MEM32(HAL, INFO, ((struct sk_buff *)prRxCell->pPacket)->data,
		     u4DumpLen);

	prDmaBuf->AllocPa = halZeroCopyPathMapRxBuf(
		prHifInfo, prDmaBuf->AllocVa,
		0, prDmaBuf->AllocSize);
}

#if CFG_MTK_WIFI_SW_EMI_RING
struct HIF_MEM *halGetRsvEmi(struct GL_HIF_INFO *prHifInfo)
{
	return &g_rRsvEmiMem;
}
#endif

#if CFG_SUPPORT_RX_PAGE_POOL
void *halZeroCopyPathAllocPagePoolRxBuf(struct GL_HIF_INFO *prHifInfo,
					struct RTMP_DMABUF *prDmaBuf,
					uint32_t u4Num, uint32_t u4Idx)
{
	struct sk_buff *prSkb;
	dma_addr_t rAddr;

	prSkb = kalAllocHifSkb();
	if (!prSkb) {
		DBGLOG(HAL, ERROR, "can't allocate rx %u size packet\n",
		       prDmaBuf->AllocSize);
		prDmaBuf->AllocPa = 0;
		prDmaBuf->AllocVa = NULL;
		return NULL;
	}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	if (skb_headroom(prSkb) == CFG_RADIOTAP_HEADROOM)
		goto skip;

	if (skb_headroom(prSkb) != 0) {
		/* Reset skb */
		prSkb->data = prSkb->head;
		skb_reset_tail_pointer(prSkb);
	}
	skb_reserve(prSkb, CFG_RADIOTAP_HEADROOM);
skip:
#endif

	prDmaBuf->AllocVa = (void *)prSkb->data;
	memset(prDmaBuf->AllocVa, 0, prDmaBuf->AllocSize);

	rAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev, prDmaBuf->AllocVa,
				   prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr)) {
		DBGLOG(HAL, ERROR, "sk_buff dma mapping error!\n");
		dev_kfree_skb(prSkb);
		return NULL;
	}
	prDmaBuf->AllocPa = (phys_addr_t)rAddr;
	return (void *)prSkb;
}

void halZeroCopyPathFreePagePoolPacket(struct GL_HIF_INFO *prHifInfo,
				   void *pvPacket, uint32_t u4Num)
{
	kalFreeHifSkb((struct sk_buff *)pvPacket);
}

void kalSkbMarkForRecycle(struct sk_buff *pkt)
{
#if KERNEL_VERSION(5, 15, 0) <= CFG80211_VERSION_CODE
	skb_mark_for_recycle(pkt);
#else
	skb_mark_for_recycle(pkt, page, page->pp);
#endif
}

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
void kalSetupPagePoolPageMaxMinNum(uint32_t u4Min, uint32_t u4Max)
{
	g_u4MinPageNum = u4Min;
	g_u4MaxPageNum = u4Max;
}

uint32_t kalGetPagePoolPageNum(void)
{
	return wifi_page_pool_get_page_num();
}

u_int8_t kalSetPagePoolPageMaxNum(void)
{
	return kalSetPagePoolPageNum(g_u4MaxPageNum);
}

u_int8_t kalIncPagePoolPageNum(void)
{
	uint32_t u4Num = 0;

	mutex_lock(&g_rPageLock);

	if (g_u4CurPageNum < g_u4HifRsvNum) {
		mutex_unlock(&g_rPageLock);
		return FALSE;
	}

	u4Num = (g_u4CurPageNum - g_u4HifRsvNum) +
		HAL_PAGE_POOL_PAGE_INC_NUM;
	if (u4Num > g_u4MaxPageNum)
		u4Num = g_u4MaxPageNum;

	mutex_unlock(&g_rPageLock);

	return kalSetPagePoolPageNum(u4Num);
}

u_int8_t kalDecPagePoolPageNum(void)
{
	uint32_t u4Num = 0;

	mutex_lock(&g_rPageLock);

	if (g_u4CurPageNum < g_u4HifRsvNum) {
		mutex_unlock(&g_rPageLock);
		return FALSE;
	}

	if ((g_u4CurPageNum - g_u4HifRsvNum) <
	    HAL_PAGE_POOL_PAGE_DEC_NUM) {
		mutex_unlock(&g_rPageLock);
		return FALSE;
	}

	if (time_before(jiffies, g_ulUpdatePagePoolPagePeriod)) {
		mutex_unlock(&g_rPageLock);
		return FALSE;
	}

	u4Num =	(g_u4CurPageNum - g_u4HifRsvNum) -
		HAL_PAGE_POOL_PAGE_DEC_NUM;
	if (u4Num < g_u4MinPageNum)
		u4Num = g_u4MinPageNum;

	mutex_unlock(&g_rPageLock);

	return kalSetPagePoolPageNum(u4Num);
}

u_int8_t kalSetPagePoolPageNum(uint32_t u4Num)
{
	uint32_t u4SetNum = g_u4HifRsvNum + u4Num;
	uint32_t u4MaxNum = wifi_page_pool_get_max_page_num();
	uint32_t u4CurPageNum = g_u4CurPageNum;
	u_int8_t fgRet = TRUE;

	if (u4SetNum > u4MaxNum)
		u4SetNum = u4MaxNum;

	if (u4SetNum == u4CurPageNum)
		return TRUE;

	mutex_lock(&g_rPageLock);

	wifi_page_pool_set_page_num(u4SetNum);
	g_u4CurPageNum = wifi_page_pool_get_page_num();

	if (g_u4CurPageNum < u4SetNum) {
		DBGLOG(HAL, ERROR, "page pool alloc fail[req:%u alloc:%u->%u]",
		       u4SetNum, u4CurPageNum, g_u4CurPageNum);
		fgRet = FALSE;
	} else {
		DBGLOG(HAL, INFO, "set page pool[req:%u alloc:%u->%u]",
		       u4SetNum, u4CurPageNum, g_u4CurPageNum);
	}

	g_ulUpdatePagePoolPagePeriod = jiffies +
		HAL_PAGE_POOL_PAGE_UPDATE_MIN_INTERVAL * HZ / 1000;

	mutex_unlock(&g_rPageLock);

	return fgRet;
}
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */

struct sk_buff *kalAllocRxSkb(uint8_t **ppucData)
{
	struct page *page;
	struct sk_buff *pkt;

	page = wifi_page_pool_alloc_page();
#if CFG_SUPPORT_RETURN_WORK
	if (!page) {
		kalIncPagePoolPageNum();
		page = wifi_page_pool_alloc_page();
	}
#endif /* CFG_SUPPORT_RETURN_WOR */
	if (!page) {
		DBGLOG_LIMITED(HAL, ERROR, "allocate page fail\n");
		return NULL;
	}

	pkt = build_skb(page_to_virt(page), PAGE_SIZE); /* ptr to sk_buff */
	if (!pkt) {
		page_pool_recycle_direct(page->pp, page);
		DBGLOG(HAL, ERROR, "allocate skb fail\n");
		return NULL;
	}
	kmemleak_not_leak(pkt); /* Omit memleak check */
	kalSkbMarkForRecycle(pkt);

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	skb_reserve(pkt, CFG_RADIOTAP_HEADROOM);
#endif

	*ppucData = (uint8_t *) (pkt->data);

	return pkt;
}

u_int8_t kalCreateHifSkbList(struct mt66xx_chip_info *prChipInfo)
{
	struct BUS_INFO *prBusInfo = prChipInfo->bus_info;
	struct sk_buff *prSkb;
	uint8_t *pucRecvBuff;
	uint32_t u4Num = 0, u4Idx;
	u_int8_t fgRet = TRUE;

	skb_queue_head_init(&g_rHifSkbList);
	u4Num = (prBusInfo->rx_data_ring_size * prBusInfo->rx_data_ring_num) +
		(prBusInfo->rx_evt_ring_size * prBusInfo->rx_evt_ring_num);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	if (kalIsSupportRro())
		u4Num += prBusInfo->rx_data_ring_size;
#endif
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	mutex_init(&g_rPageLock);
	g_u4HifRsvNum = u4Num;
	kalSetPagePoolPageNum(0);
#endif

	for (u4Idx = 0; u4Idx < u4Num; u4Idx++) {
		prSkb = kalAllocRxSkb(&pucRecvBuff);
		if (!prSkb) {
			DBGLOG(HAL, ERROR, "hif skb reserve fail[%u]!\n",
			       u4Idx);
			kalReleaseHifSkbList();
			fgRet = FALSE;
			goto exit;
		}
		skb_queue_tail(&g_rHifSkbList, prSkb);
	}
	DBGLOG(HAL, INFO, "hif skb reserve count[%u]!\n", u4Num);

exit:
	return fgRet;
}

void kalReleaseHifSkbList(void)
{
	struct sk_buff *prSkb;

	while (!skb_queue_empty(&g_rHifSkbList)) {
		prSkb = skb_dequeue(&g_rHifSkbList);
		if (!prSkb) {
			DBGLOG(HAL, ERROR, "hif skb is NULL!\n");
			break;
		}
		kalPacketFree(NULL, (void *)prSkb);
	}

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	g_u4HifRsvNum = 0;
	kalSetPagePoolPageNum(0);
#endif
}

struct sk_buff *kalAllocHifSkb(void)
{
	if (skb_queue_empty(&g_rHifSkbList)) {
		DBGLOG(HAL, ERROR, "hif skb list is empty\n");
		return NULL;
	}

	return skb_dequeue(&g_rHifSkbList);
}

void kalFreeHifSkb(struct sk_buff *prSkb)
{
	skb_queue_tail(&g_rHifSkbList, prSkb);
}
#endif /* CFG_SUPPORT_RX_PAGE_POOL */
