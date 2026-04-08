// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#include <linux/dma-direct.h>
#include "gl_os.h"

#include "hif_pdma.h"

#include "precomp.h"

#include "mt66xx_reg.h"
#include "gl_kal.h"


/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define WFDMA_MEMORY_ALIGNMENT		16

#define HAL_TX_MAX_SIZE_PER_FRAME	(NIC_TX_MAX_SIZE_PER_FRAME + \
					NIC_TX_DESC_AND_PADDING_LENGTH)

#define	TX_RING_CMD_SIZE		TX_RING_SIZE
#define HAL_TX_CMD_BUFF_SIZE		4096

#if RX_RING0_SIZE > RX_RING1_SIZE
#define RX_RING_MAX_SIZE RX_RING0_SIZE
#else
#define RX_RING_MAX_SIZE RX_RING1_SIZE
#endif

/* sync from include/chips/mt7961.h */
#define MT7961_HOST_RX_WM_EVENT_FROM_PSE_RX_RING4_SIZE 32
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
struct wifi_rsrv_mem {
	phys_addr_t phy_base;
	void *vir_base;
	unsigned long long size;
};

struct HIF_MEM {
	phys_addr_t pa;
	void *va;
	uint32_t align_size;
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

#if (CFG_HIF_TX_PREALLOC_DATA_BUFFER == 1)
	/* Tx Data */
	struct HIF_MEM rMsduBuf[HIF_TX_MSDU_TOKEN_NUM];
#endif

	dma_addr_t rDmaAddr[WIFI_RSV_MEM_MAX_NUM];
	phys_addr_t pucRsvMemBase[WIFI_RSV_MEM_MAX_NUM];
	void *pucRsvMemVirBase[WIFI_RSV_MEM_MAX_NUM];
	uint64_t u4RsvMemSize[WIFI_RSV_MEM_MAX_NUM];
	uint32_t u4Offset[WIFI_RSV_MEM_MAX_NUM];
};
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */

/*******************************************************************************
 *                              C O N S T A N T S
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
#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
static struct HIF_PREALLOC_MEM grMem;
#ifdef CONFIG_OF
static uint32_t fgHypervisor = 0;
static unsigned long long gWifiRsvMemSize[WIFI_RSV_MEM_MAX_NUM];
#endif /* CONFIG_OF */
/* Assume reserved memory size < BIT(32) */
static struct wifi_rsrv_mem wifi_rsrv_mems[WIFI_RSV_MEM_MAX_NUM][32];
#endif /* CFG_SUPPORT_WIFI_RSV_MEM */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */


/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
static bool halGetRsvMemSizeRsvedByKernel(struct platform_device *pdev,
				enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
#if defined(CONFIG_OF)
	int ret = 0;
	struct device_node *np;

	np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!np) {
		DBGLOG(INIT, ERROR, "can NOT find memory-region.\n");
		return false;
	}
	ret = of_property_read_u64(np, "size",
					 &gWifiRsvMemSize[u4RsvMemIdx]);
	of_node_put(np);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get rsrv mem size failed(%d).\n", ret);
		return false;
	}
	DBGLOG(INIT, INFO, "gWifiRsvMemSize:0x%llx\n",
	       gWifiRsvMemSize[u4RsvMemIdx]);
	return true;
#else
	return false;
#endif
}

int halInitResvMem(struct platform_device *pdev,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
#ifdef CONFIG_OF
	int ret = 0;
	struct device_node *node = NULL;
	struct device_node *rmem_node = NULL;
	struct reserved_mem *rmem = NULL;
	unsigned int RsvMemSize = 0;

	node = pdev->dev.of_node;
	if (!node) {
		DBGLOG(INIT, ERROR, "WIFI-OF: get wifi device node fail\n");
		of_node_put(node);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "hypervisor", &fgHypervisor);
	if (ret != 0)
		fgHypervisor = 0;
	DBGLOG(INIT, STATE, "WIFI-OF: hypervisor (%u)\n", fgHypervisor);

	if (halGetRsvMemSizeRsvedByKernel(pdev, u4RsvMemIdx) == false) {
		ret = of_property_read_u32(node, "emi-size", &RsvMemSize);
		if (ret != 0)
			DBGLOG(INIT, ERROR,
				"MPU-in-lk get rsrv mem size failed(%d).\n",
				ret);
		else {
			gWifiRsvMemSize[u4RsvMemIdx] =
				(unsigned long long) RsvMemSize;
			DBGLOG(INIT, INFO,
				"MPU-in-lk gWifiRsvMemSize[%u]: 0x%llx\n",
				u4RsvMemIdx, gWifiRsvMemSize[u4RsvMemIdx]);
		}
	}

	of_node_put(node);


	/* assign reserved memory region to dev */
	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		ret = 0;
		/* no memory region fouund, lookup by name */
#ifndef CFG_MTK_WIFI_RSVMEM_DTS_COMPATIBLE
		rmem_node = of_find_compatible_node(NULL, NULL,
					    "mediatek,combo_wfdma_reserved");
#else
		rmem_node = of_find_compatible_node(NULL, NULL,
					    CFG_MTK_WIFI_RSVMEM_DTS_COMPATIBLE);
#endif
		if (!rmem_node) {
			DBGLOG(INIT, ERROR,
				"WIFI-OF: get rmem of node fail\n");
			return -EINVAL;
		}

		rmem = of_reserved_mem_lookup(rmem_node);
		if (!rmem) {
			DBGLOG(INIT, ERROR,
				"WIFI-OF: lookup rmem node fail\n");
			return -EINVAL;
		}

		grMem.pucRsvMemBase[u4RsvMemIdx] = rmem->base;
		grMem.u4RsvMemSize[u4RsvMemIdx] = rmem->size;
		gWifiRsvMemSize[u4RsvMemIdx] = rmem->size;
		if (request_mem_region(rmem->base, rmem->size,
				       "79xx_wifi_vm") < 0)
			DBGLOG(INIT, STATE, "request_mem_region fail\n");
		grMem.pucRsvMemVirBase[u4RsvMemIdx] =
				ioremap(rmem->base, rmem->size);
		grMem.u4Offset[u4RsvMemIdx] = 0;

		DBGLOG(INIT, STATE,
			"va 0x%llx, pa 0x%llx, size 0x%llx\n",
			grMem.pucRsvMemVirBase[u4RsvMemIdx],
			grMem.pucRsvMemBase[u4RsvMemIdx],
			grMem.u4RsvMemSize[u4RsvMemIdx]);

		grMem.rDmaAddr[u4RsvMemIdx] = grMem.pucRsvMemBase[u4RsvMemIdx];
	} else {
		grMem.rDmaAddr[u4RsvMemIdx] = DMA_MAPPING_ERROR;
		DBGLOG(INIT, ERROR, "get reserve memory failed\n");
	}

	return ret;
#else
	DBGLOG(INIT, ERROR, "kernel option CONFIG_OF not enabled.\n");
	return -EINVAL;
#endif
}

static bool halAllocRsvMemAlign(uint32_t u4Size,
		struct HIF_MEM *prMem, uint32_t u4Align,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
	uint32_t u4ExtSize = 0;

	if (u4Align < WFDMA_MEMORY_ALIGNMENT)
		u4Align = WFDMA_MEMORY_ALIGNMENT;

	if (grMem.u4Offset[u4RsvMemIdx] & (u4Align - 1)) {
		u4ExtSize = u4Align -
			(grMem.u4Offset[u4RsvMemIdx] & (u4Align - 1));
	}
	u4Size += u4ExtSize;

	if ((grMem.u4Offset[u4RsvMemIdx] + u4Size) >
	    gWifiRsvMemSize[u4RsvMemIdx]) {
		prMem->pa = 0;
		prMem->va = NULL;
		prMem->align_size = 0;
		return false;
	}

	prMem->pa = grMem.pucRsvMemBase[u4RsvMemIdx] +
		grMem.u4Offset[u4RsvMemIdx] + u4ExtSize;
	prMem->va = grMem.pucRsvMemVirBase[u4RsvMemIdx] +
		grMem.u4Offset[u4RsvMemIdx] + u4ExtSize;
	prMem->align_size = u4ExtSize;
	grMem.u4Offset[u4RsvMemIdx] += u4Size;

	return prMem->va != NULL;
}

static bool halAllocRsvMem(uint32_t u4Size, struct HIF_MEM *prMem,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
	/* default alignment */
	return halAllocRsvMemAlign(u4Size, prMem,
		WFDMA_MEMORY_ALIGNMENT, u4RsvMemIdx);
}

#if 0
static bool halFreeRsvMem(uint32_t u4Size,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
	if (u4Size > grMem.u4Offset[u4RsvMemIdx])
		return false;

	grMem.u4Offset[u4RsvMemIdx] -= u4Size;

	return true;
}
#endif

static int halInitHifMem(struct platform_device *pdev,
		  struct mt66xx_chip_info *prChipInfo,
		  enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx,
		  gfp_t gfp)
{
	uint32_t i = sizeof(wifi_rsrv_mems[u4RsvMemIdx]) /
		sizeof(struct wifi_rsrv_mem);

	if (grMem.rDmaAddr[u4RsvMemIdx] != DMA_MAPPING_ERROR) {
		/* already got pa/va/size from dts */
		DBGLOG(INIT, ERROR, "already got pa/va/size from dts\n");
		goto RETURN;
	}

	/* Allocation size should be a power of two */
	while (i > 0) {
		i--;
		if (!(gWifiRsvMemSize[u4RsvMemIdx] & BIT(i)))
			continue;

		wifi_rsrv_mems[u4RsvMemIdx][i].size = BIT(i);
		wifi_rsrv_mems[u4RsvMemIdx][i].vir_base =
			dma_alloc_coherent(&pdev->dev,
				wifi_rsrv_mems[u4RsvMemIdx][i].size,
				&wifi_rsrv_mems[u4RsvMemIdx][i].phy_base,
				gfp);
		if (!wifi_rsrv_mems[u4RsvMemIdx][i].vir_base) {
			DBGLOG(INIT, ERROR,
				"[%u][%d] DMA_ALLOC_COHERENT failed, size: 0x%llx\n",
				u4RsvMemIdx, i,
				wifi_rsrv_mems[u4RsvMemIdx][i].size);
			return -1;
		}
		if (!grMem.pucRsvMemBase[u4RsvMemIdx]) {
			grMem.pucRsvMemBase[u4RsvMemIdx] =
				wifi_rsrv_mems[u4RsvMemIdx][i].phy_base;
			grMem.pucRsvMemVirBase[u4RsvMemIdx] =
				wifi_rsrv_mems[u4RsvMemIdx][i].vir_base;
			grMem.u4RsvMemSize[u4RsvMemIdx] =
				(uint64_t) gWifiRsvMemSize[u4RsvMemIdx];
		}
	}

	if (!grMem.pucRsvMemBase[u4RsvMemIdx])
		return -1;

RETURN:
	DBGLOG(INIT, INFO,
		"pucRsvMemBase[%u][%pa], pucRsvMemVirBase[%u][%pa]\n",
		u4RsvMemIdx, &grMem.pucRsvMemBase[u4RsvMemIdx],
		u4RsvMemIdx, &grMem.pucRsvMemVirBase[u4RsvMemIdx]);

	return 0;
}

int halAllocHifMem(struct platform_device *pdev,
		   struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Idx, u4Size;
#if (CFG_HIF_TX_PREALLOC_DATA_BUFFER == 1)
	uint32_t u4AllocTokNum = 0;
#endif
	uint32_t u4Cnt, u4PktSize;

	prChipInfo = prDriverData->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (halInitHifMem(pdev, prChipInfo, WIFI_RSV_MEM_WFDMA, GFP_DMA) == -1)
		return -1;

        /* TXD */
	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		DBGLOG(INIT, INFO, "Txd alloc[%u] \n", u4Idx);
		u4Size = TX_RING_SIZE;

		if (!halAllocRsvMem(u4Size * TXD_SIZE,
			&grMem.rTxDesc[u4Idx], WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "TxDesc[%u] alloc fail\n", u4Idx);
	}

        /* RXD */
	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		if (u4Idx == RX_RING_DATA_IDX_0)
			u4Size = RX_RING0_SIZE;
		else if (u4Idx == RX_RING_EVT_IDX_1)
			u4Size = RX_RING1_SIZE;
#if (CFG_SUPPORT_CONNAC3X == 0)
		else if (u4Idx == WFDMA0_RX_RING_IDX_2)
			u4Size = RX_RING0_SIZE;
		else if (u4Idx == WFDMA0_RX_RING_IDX_3)
#if (CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE == 1)
			u4Size = MT7961_HOST_RX_WM_EVENT_FROM_PSE_RX_RING4_SIZE;
#else
			u4Size = RX_RING1_SIZE;
#endif
		else if (u4Idx == WFDMA1_RX_RING_IDX_0)
			u4Size = RX_RING1_SIZE;
		else if (u4Idx == WFDMA1_RX_RING_IDX_1)
			u4Size = RX_RING_SIZE;
		else if (u4Idx == WFDMA1_RX_RING_IDX_2)
			u4Size = RX_RING_SIZE;
#endif
		else
			continue;
		DBGLOG(INIT, INFO, "Rxd alloc[%u] \n", u4Idx);
		if (!halAllocRsvMem(u4Size * RXD_SIZE,
				&grMem.rRxDesc[u4Idx], WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "RxDesc[%u] alloc fail\n", u4Idx);
	}

        /* TX BUF */
#if (CFG_HIF_TX_PREALLOC_DATA_BUFFER == 1)
	u4AllocTokNum = HIF_TX_MSDU_TOKEN_NUM;
	for (u4Idx = 0; u4Idx < u4AllocTokNum; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_MAX_SIZE_PER_FRAME +
				prChipInfo->txd_append_size,
				&grMem.rMsduBuf[u4Idx],
				WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "MsduBuf[%u] alloc fail\n", u4Idx);
	}
#endif

	/* TX CMD BUF */
	for (u4Cnt = 0; u4Cnt < TX_RING_CMD_SIZE; u4Cnt++) {
		if (!halAllocRsvMem(HAL_TX_CMD_BUFF_SIZE,
				    &grMem.rTxCmdBuf[u4Cnt],
				    WIFI_RSV_MEM_WFDMA)) {
			DBGLOG(INIT, ERROR,
				"rTxCmdBuf[%u] alloc fail\n", u4Cnt);
		}
		if (!halAllocRsvMem(HAL_TX_CMD_BUFF_SIZE,
				    &grMem.rTxFwdlBuf[u4Cnt],
				    WIFI_RSV_MEM_WFDMA)) {
			DBGLOG(INIT, ERROR,
				"rTxFwdlBuf[%u] alloc fail\n", u4Cnt);
		}
	}

        /* RX BUF */
	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++)
	{
		if (u4Idx == RX_RING_DATA_IDX_0) {
			u4Size = RX_RING0_SIZE;
			u4PktSize = CFG_RX_MAX_MPDU_SIZE;
		} else if (u4Idx == RX_RING_EVT_IDX_1) {
			u4Size = RX_RING1_SIZE;
			u4PktSize = RX_BUFFER_AGGRESIZE;
#if (CFG_SUPPORT_CONNAC3X == 0)
		} else if (u4Idx == WFDMA0_RX_RING_IDX_2) {
			u4Size = RX_RING0_SIZE;
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
		} else if (u4Idx == WFDMA0_RX_RING_IDX_3) {
#if (CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE == 1)
			u4Size = MT7961_HOST_RX_WM_EVENT_FROM_PSE_RX_RING4_SIZE;
#else
			u4Size = RX_RING1_SIZE;
#endif
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
		} else if (u4Idx == WFDMA1_RX_RING_IDX_0) {
			u4Size = RX_RING1_SIZE;
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
		} else if (u4Idx == WFDMA1_RX_RING_IDX_1) {
			u4Size = RX_RING_SIZE;
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
		} else if (u4Idx == WFDMA1_RX_RING_IDX_2) {
			u4Size = RX_RING_SIZE;
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
#endif
		} else {
				continue;
		}

		for (u4Cnt = 0; u4Cnt < u4Size; u4Cnt++) {
			if (!halAllocRsvMem(u4PktSize,
					&grMem.rRxMemBuf[u4Idx][u4Cnt],
					WIFI_RSV_MEM_WFDMA)) {
				DBGLOG(INIT, ERROR,
					"RxMemBuf[%u][%u] alloc fail\n",
					u4Idx, u4Cnt);
			}
		}
	}

	DBGLOG(INIT, INFO, "grMem.u4Offset[WIFI_RSV_MEM_WFDMA]=[0x%x]\n",
		grMem.u4Offset[WIFI_RSV_MEM_WFDMA]);

	return 0;
}

void halFreeHifMem(struct platform_device *pdev,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
	uint32_t i = 0;
	uint32_t count = sizeof(wifi_rsrv_mems[u4RsvMemIdx]) /
		sizeof(struct wifi_rsrv_mem);

	if (grMem.rDmaAddr[u4RsvMemIdx] != DMA_MAPPING_ERROR) {
		iounmap(grMem.pucRsvMemVirBase[u4RsvMemIdx]);
		release_mem_region(grMem.pucRsvMemBase[u4RsvMemIdx],
				   grMem.u4RsvMemSize[u4RsvMemIdx]);
		grMem.rDmaAddr[u4RsvMemIdx] = DMA_MAPPING_ERROR;
		return;
	}
	for (i = 0; i < count; i++) {
		if (!wifi_rsrv_mems[u4RsvMemIdx][i].vir_base)
			continue;
		dma_free_coherent(
			&pdev->dev,
			wifi_rsrv_mems[u4RsvMemIdx][i].size,
			wifi_rsrv_mems[u4RsvMemIdx][i].vir_base,
			(dma_addr_t) wifi_rsrv_mems[u4RsvMemIdx][i].phy_base);
	}
}
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */



#if (CFG_SUPPORT_WIFI_RSV_MEM ==1)
static void pcieRsvMemAllocTxDesc(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing,
			   uint32_t u4Num)
{
	prDescRing->AllocVa = grMem.rTxDesc[u4Num].va;
	prDescRing->AllocPa = grMem.rTxDesc[u4Num].pa;
	if (prDescRing->AllocVa == NULL)
		DBGLOG(HAL, ERROR, "prDescRing->AllocVa is NULL\n");
	else
		memset_io(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

static void pcieRsvMemAllocRxDesc(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing,
			   uint32_t u4Num)
{
	prDescRing->AllocVa = grMem.rRxDesc[u4Num].va;
	prDescRing->AllocPa = grMem.rRxDesc[u4Num].pa;
	if (prDescRing->AllocVa == NULL)
		DBGLOG(HAL, ERROR, "prDescRing->AllocVa is NULL\n");
	else
		memset_io(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

static bool pcieRsvMemoryAllocTxCmdBuf(struct RTMP_DMABUF *prDmaBuf,
			     uint32_t u4Num, uint32_t u4Idx)
{
	if (u4Num != TX_RING_CMD_IDX_2 && u4Num != TX_RING_FWDL_IDX_3)
		return true;

	prDmaBuf->AllocSize = HAL_TX_CMD_BUFF_SIZE;
	if (u4Num == TX_RING_CMD_IDX_2) {
		prDmaBuf->AllocPa = grMem.rTxCmdBuf[u4Idx].pa;
		prDmaBuf->AllocVa = grMem.rTxCmdBuf[u4Idx].va;
	} else if (u4Num == TX_RING_FWDL_IDX_3) {
		prDmaBuf->AllocPa = grMem.rTxFwdlBuf[u4Idx].pa;
		prDmaBuf->AllocVa = grMem.rTxFwdlBuf[u4Idx].va;
	}
	if (prDmaBuf->AllocVa  == NULL) {
		DBGLOG(HAL, ERROR, "AllocVa is NULL[%u][%u]\n", u4Num, u4Idx);
		return false;
	}
	memset_io(prDmaBuf->AllocVa, 0, prDmaBuf->AllocSize);

	return true;
}

static void pcieRsvMemAllocTxDataBuf(struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx)
{
	prToken->prPacket = grMem.rMsduBuf[u4Idx].va;
	prToken->rDmaAddr = grMem.rMsduBuf[u4Idx].pa;
}

static void *pcieRsvMemAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDmaBuf,
			    uint32_t u4Num, uint32_t u4Idx)
{
	if (u4Num >= NUM_OF_RX_RING ) {
		DBGLOG(RX, ERROR, "RX alloc fail error number=%d idx=%d\n",
		       u4Num, u4Idx);
		return prDmaBuf->AllocVa;
	}

	prDmaBuf->AllocPa = grMem.rRxMemBuf[u4Num][u4Idx].pa;
	prDmaBuf->AllocVa = grMem.rRxMemBuf[u4Num][u4Idx].va;
	/*prDmaBuf->fgIsCopyPath = TRUE;*/

	if (prDmaBuf->AllocVa == NULL)
		DBGLOG(HAL, ERROR, "AllocVa is NULL[%u][%u]\n", u4Num, u4Idx);
	else
		memset_io(prDmaBuf->AllocVa, 0, prDmaBuf->AllocSize);

	return prDmaBuf->AllocVa;
}

static bool pcieRsvMemCopyCmd(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_DMACB *prTxCell, void *pucBuf,
		       void *pucSrc1, uint32_t u4SrcLen1,
		       void *pucSrc2, uint32_t u4SrcLen2)
{
	struct RTMP_DMABUF *prDmaBuf = &prTxCell->DmaBuf;

	memcpy_toio(prDmaBuf->AllocVa, pucSrc1, u4SrcLen1);
	if (pucSrc2 != NULL && u4SrcLen2 > 0)
		memcpy_toio(prDmaBuf->AllocVa + u4SrcLen1, pucSrc2, u4SrcLen2);
	prTxCell->PacketPa = prDmaBuf->AllocPa;

	return true;
}

static bool pcieRsvMemCopyEvent(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMACB *pRxCell,
			 struct RXD_STRUCT *pRxD,
			 struct RTMP_DMABUF *prDmaBuf,
			 uint8_t *pucDst, uint32_t u4Len)
{
	memcpy_fromio(pucDst, prDmaBuf->AllocVa, u4Len);

	return true;
}

static bool pcieRsvMemCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			  void *pucSrc, uint32_t u4Len, uint32_t u4offset)
{
	memcpy_toio((uint8_t *)prToken->prPacket + u4offset, pucSrc, u4Len);

	return true;
}

static bool pcieRsvMemCopyRxData(struct GL_HIF_INFO *prHifInfo,
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

	memcpy_fromio(prSkb->data, prDmaBuf->AllocVa, u4Size);

	return true;
}

static void pcieRsvMemDumpTx(struct GL_HIF_INFO *prHifInfo,
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

static void pcieRsvMemDumpRx(struct GL_HIF_INFO *prHifInfo,
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

static void halSetMemOpsRsvMem(struct HIF_MEM_OPS *prMemOps)
{
	prMemOps->allocTxDesc = pcieRsvMemAllocTxDesc;
	prMemOps->allocRxDesc = pcieRsvMemAllocRxDesc;
	prMemOps->allocTxCmdBuf = pcieRsvMemoryAllocTxCmdBuf;
	prMemOps->allocTxDataBuf = pcieRsvMemAllocTxDataBuf;
	prMemOps->allocRxBuf = pcieRsvMemAllocRxBuf;
	prMemOps->allocRuntimeMem = NULL;
	prMemOps->copyCmd = pcieRsvMemCopyCmd;
	prMemOps->copyEvent = pcieRsvMemCopyEvent;
	prMemOps->copyTxData = pcieRsvMemCopyTxData;
	prMemOps->copyRxData = pcieRsvMemCopyRxData;
	prMemOps->flushCache = NULL;
	prMemOps->mapTxBuf = NULL;
	prMemOps->mapRxBuf = NULL;
	prMemOps->unmapTxBuf = NULL;
	prMemOps->unmapRxBuf = NULL;
	prMemOps->freeDesc = NULL;
	prMemOps->freeBuf = NULL;
	prMemOps->freePacket = NULL;
	prMemOps->dumpTx = pcieRsvMemDumpTx;
	prMemOps->dumpRx = pcieRsvMemDumpRx;
}
#else
static void pcieAllocDesc(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMABUF *prDescRing,
			  uint32_t u4Num)
{
	dma_addr_t rAddr = 0;

	prDescRing->AllocVa = KAL_DMA_ALLOC_COHERENT(
		prHifInfo->prDmaDev, prDescRing->AllocSize, &rAddr);
	prDescRing->AllocPa = (phys_addr_t)rAddr;
	if (prDescRing->AllocVa)
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

static void pcieAllocTxDataBuf(struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx)
{
	prToken->prPacket = kalMemAlloc(prToken->u4DmaLength, PHY_MEM_TYPE);
	prToken->rDmaAddr = 0;
}

static void *pcieAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
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

#if (CFG_SUPPORT_SNIFFER_RADIOTAP == 1)
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
	return (void *)pkt;
}

static void *pcieAllocRuntimeMem(uint32_t u4SrcLen)
{
	return kalMemAlloc(u4SrcLen, PHY_MEM_TYPE);
}

static bool pcieCopyCmd(struct GL_HIF_INFO *prHifInfo,
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

static bool pcieCopyEvent(struct GL_HIF_INFO *prHifInfo,
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

static bool pcieCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			   void *pucSrc, uint32_t u4Len, uint32_t u4offset)
{
	memcpy((uint8_t *)prToken->prPacket + u4offset, pucSrc, u4Len);
	return true;
}

static bool pcieCopyRxData(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMACB *pRxCell,
			   struct RTMP_DMABUF *prDmaBuf,
			   struct SW_RFB *prSwRfb)
{
	void *pRxPacket = NULL;
	dma_addr_t rAddr;

	pRxPacket = pRxCell->pPacket;
	ASSERT(pRxPacket);

	pRxCell->pPacket = prSwRfb->pvPacket;

	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)prDmaBuf->AllocPa,
			     prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);
	prSwRfb->pvPacket = pRxPacket;

	prDmaBuf->AllocVa = ((struct sk_buff *)pRxCell->pPacket)->data;
	rAddr = KAL_DMA_MAP_SINGLE(prHifInfo->prDmaDev,
		prDmaBuf->AllocVa, prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);
	if (KAL_DMA_MAPPING_ERROR(prHifInfo->prDmaDev, rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		ASSERT(0);
		return false;
	}
	prDmaBuf->AllocPa = (phys_addr_t)rAddr;

	return true;
}

static phys_addr_t pcieMapTxBuf(struct GL_HIF_INFO *prHifInfo,
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

static phys_addr_t pcieMapRxBuf(struct GL_HIF_INFO *prHifInfo,
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

static void pcieUnmapTxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len)
{
	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)rDmaAddr,
			     u4Len, KAL_DMA_TO_DEVICE);
}

static void pcieUnmapRxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len)
{
	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)rDmaAddr,
			     u4Len, KAL_DMA_FROM_DEVICE);
}

static void pcieFreeDesc(struct GL_HIF_INFO *prHifInfo,
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

static void pcieFreeBuf(void *pucSrc, uint32_t u4Len)
{
	kalMemFree(pucSrc, PHY_MEM_TYPE, u4Len);
}

static void pcieFreePacket(void *pvPacket)
{
	kalPacketFree(NULL, pvPacket);
}

static void pcieDumpTx(struct GL_HIF_INFO *prHifInfo,
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

static void pcieDumpRx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen)
{
	struct RTMP_DMACB *prRxCell;
	struct RTMP_DMABUF *prDmaBuf;

	prRxCell = &prRxRing->Cell[u4Idx];
	prDmaBuf = &prRxCell->DmaBuf;

	if (!prRxCell->pPacket)
		return;

	pcieUnmapRxBuf(prHifInfo, prDmaBuf->AllocPa, prDmaBuf->AllocSize);

	DBGLOG_MEM32(HAL, INFO, ((struct sk_buff *)prRxCell->pPacket)->data,
		     u4DumpLen);

	prDmaBuf->AllocPa = pcieMapRxBuf(prHifInfo, prDmaBuf->AllocVa,
					0, prDmaBuf->AllocSize);
}

static void halSetMemOpsNormal(struct HIF_MEM_OPS *prMemOps)
{
        prMemOps->allocTxDesc = pcieAllocDesc;
	prMemOps->allocRxDesc = pcieAllocDesc;
	prMemOps->allocTxCmdBuf = NULL;
	prMemOps->allocTxDataBuf = pcieAllocTxDataBuf;
	prMemOps->allocRxBuf = pcieAllocRxBuf;
	prMemOps->allocRuntimeMem = pcieAllocRuntimeMem;
	prMemOps->copyCmd = pcieCopyCmd;
	prMemOps->copyEvent = pcieCopyEvent;
	prMemOps->copyTxData = pcieCopyTxData;
	prMemOps->copyRxData = pcieCopyRxData;
	prMemOps->flushCache = NULL;
	prMemOps->mapTxBuf = pcieMapTxBuf;
	prMemOps->mapRxBuf = pcieMapRxBuf;
	prMemOps->unmapTxBuf = pcieUnmapTxBuf;
	prMemOps->unmapRxBuf = pcieUnmapRxBuf;
	prMemOps->freeDesc = pcieFreeDesc;
	prMemOps->freeBuf = pcieFreeBuf;
	prMemOps->freePacket = pcieFreePacket;
	prMemOps->dumpTx = pcieDumpTx;
	prMemOps->dumpRx = pcieDumpRx;
}
#endif

/* normal memory ops end*/
/* reserved memory ops begin*/
/* add memops functions here*/
int halSetMemOps(struct platform_device *prPlatDev,
		 struct HIF_MEM_OPS *prMemOps)
{

#if (CFG_SUPPORT_WIFI_RSV_MEM ==1)
	halSetMemOpsRsvMem(prMemOps);
#else
	halSetMemOpsNormal(prMemOps);
#endif

	return 0;
}

