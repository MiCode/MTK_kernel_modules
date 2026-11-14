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

#include "gl_os.h"

#include "hif_pdma.h"

#include "precomp.h"

#include "mt66xx_reg.h"
#include "gl_kal.h"

#if CFG_SUPPORT_RX_PAGE_POOL
#if KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE
#include <net/page_pool.h>
#else
#include <net/page_pool/helpers.h>
#endif
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
#include <linux/cma.h>
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define HAL_TX_MAX_SIZE_PER_FRAME         (NIC_TX_MAX_SIZE_PER_FRAME +      \
					   NIC_TX_DESC_AND_PADDING_LENGTH)
#define HAL_TX_CMD_BUFF_SIZE              4096

#if CFG_SUPPORT_RX_PAGE_POOL
#define HAL_PAGE_POOL_PAGE_INC_NUM              512
#define HAL_PAGE_POOL_PAGE_DEC_NUM              256
#define HAL_PAGE_POOL_PAGE_UPDATE_MIN_INTERVAL  500
#endif

#if (CFG_MTK_WIFI_TX_MEM_SLIM == 1)
#define TX_CMA_GROUP_SIZE			(SZ_1M)
#define TX_CMA_ALLOC_RETRY_TIME_TH	10
#endif /* CFG_MTK_WIFI_TX_MEM_SLIM */
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
#define TX_CMA_TOK_SIZE			(SZ_2K)
#define TX_CMA_GROUP_TOK_NUM	(TX_CMA_GROUP_SIZE / TX_CMA_TOK_SIZE)
#define TX_CMA_GROUP_NUM		\
	(HIF_TX_MSDU_TOKEN_NUM / TX_CMA_GROUP_TOK_NUM + 1)
#define TX_CMA_MAX_SIZE			(TX_CMA_GROUP_NUM * SZ_1M)
#define TX_CMA_MAX_DATA_NUM		(TX_CMA_MAX_SIZE / TX_CMA_TOK_SIZE)
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

#if (CFG_SUPPORT_RX_PAGE_POOL == 0) || (CFG_SUPPORT_DYNAMIC_PAGE_POOL == 1)
#define RX_DATA_USE_RSV_MEM		1
#else
#define RX_DATA_USE_RSV_MEM		0
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
	uint32_t u4MsduBufIdx;
#endif
#if CFG_SUPPORT_WIFI_RSV_MEM
	phys_addr_t pucRsvMemBase[WIFI_RSV_MEM_MAX_NUM];
	void *pucRsvMemVirBase[WIFI_RSV_MEM_MAX_NUM];
	uint64_t u4RsvMemSize[WIFI_RSV_MEM_MAX_NUM];
	uint32_t u4Offset[WIFI_RSV_MEM_MAX_NUM];
#endif
#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	struct HIF_MEM rTxCmaMemGroup[CMA_MEM_MAX_SIZE];
	uint32_t u4TxMemGroupIdx;
	uint32_t u4TxMemTokIdx;
	uint32_t u4TxMemGroupRetryTime;
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
};

struct tx_cma_data {
	struct list_head group_list;
	struct list_head free_list;
};

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
struct tx_cma_mem_group {
	struct list_head list;
	uint64_t key;
	uint32_t count;
	void *kaddr;
	uint32_t size;
	bool in_used;
	struct hlist_node node;
};

struct wifi_tx_cma_context {
	struct device *dev;
	struct tx_cma_mem_group groups[TX_CMA_GROUP_NUM];
	struct hlist_head groups_htbl[TX_CMA_GROUP_NUM];
	struct hlist_head free_group_list;
	struct list_head free_data_list;
	uint32_t total_groups_data_num;
	uint32_t max_data_num;
	uint32_t min_data_num;
	bool is_cma_mem;
	uint32_t config_data_num;
	uint32_t cur_data_num;
	spinlock_t data_num_lock;
};
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static struct HIF_PREALLOC_MEM grMem;
#if CFG_SUPPORT_WIFI_RSV_MEM
#ifdef CONFIG_OF
static unsigned long long gWifiRsvMemSize[WIFI_RSV_MEM_MAX_NUM];
#endif /* CONFIG_OF */
/* Assume reserved memory size < BIT(32) */
static struct wifi_rsrv_mem wifi_rsrv_mems[WIFI_RSV_MEM_MAX_NUM][32];
#endif /* CFG_SUPPORT_WIFI_RSV_MEM */

#if CFG_MTK_WIFI_SW_EMI_RING
struct HIF_MEM g_rRsvEmiMem;
#endif

#if CFG_SUPPORT_PAGE_POOL_USE_CMA
static struct sk_buff_head g_rHifSkbList;

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
static uint32_t g_u4HifRsvNum;
static uint32_t g_u4CurPageNum;
static uint32_t g_u4MinPageNum;
static uint32_t g_u4MaxPageNum;
static unsigned long g_ulUpdatePagePoolPagePeriod;
static u_int8_t g_fgPagePoolDelayAlloc;
static struct mutex g_rPageLock;
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */

#if (CFG_MTK_WIFI_TX_MEM_SLIM == 1)
struct platform_device *g_prTxCmaPlatDev;
#endif /* CFG_MTK_WIFI_TX_MEM_SLIM */


/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static int halSetMemOpsTxData(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets);


/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
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
	ret = of_property_read_u64_array(np, "size",
					 &gWifiRsvMemSize[u4RsvMemIdx], 1);
	of_node_put(np);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get rsrv mem size failed(%d).\n", ret);
		return false;
	}
	DBGLOG(INIT, DEBUG, "gWifiRsvMemSize:0x%llx\n",
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
	unsigned int RsvMemSize = 0;

	node = pdev->dev.of_node;
	if (!node) {
		DBGLOG(INIT, ERROR, "WIFI-OF: get wifi device node fail\n");
		of_node_put(node);
		return false;
	}

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

	return ret;
#else
	DBGLOG(INIT, ERROR, "kernel option CONFIG_OF not enabled.\n");
	return -1;
#endif
}

#if CFG_SUPPORT_WIFI_RSV_MEM
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

static bool halFreeRsvMem(uint32_t u4Size,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
	if (u4Size > grMem.u4Offset[u4RsvMemIdx])
		return false;

	grMem.u4Offset[u4RsvMemIdx] -= u4Size;

	return true;
}

static int halInitHifMem(struct platform_device *pdev,
		  struct mt66xx_chip_info *prChipInfo,
		  enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx,
		  gfp_t gfp)
{
	uint32_t i = sizeof(wifi_rsrv_mems[u4RsvMemIdx]) /
		sizeof(struct wifi_rsrv_mem);

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

	DBGLOG(INIT, INFO,
		"pucRsvMemBase[%u][%pa], pucRsvMemVirBase[%u][%pa]\n",
		u4RsvMemIdx, &grMem.pucRsvMemBase[u4RsvMemIdx],
		u4RsvMemIdx, &grMem.pucRsvMemVirBase[u4RsvMemIdx]);

	if (halGetRsvMemSizeRsvedByKernel(pdev, u4RsvMemIdx) == true)
		kalSetDrvEmiMpuProtection(grMem.pucRsvMemBase[u4RsvMemIdx],
					  0, grMem.u4RsvMemSize[u4RsvMemIdx]);


	return 0;
}

int halAllocHifMem(struct platform_device *pdev,
		   struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	uint32_t u4Idx, u4Size, u4EvtNum, u4DataNum;
#if HIF_TX_PREALLOC_DATA_BUFFER
	uint32_t u4AllocTokNum = 0;
#endif
#if CFG_MTK_WIFI_SW_EMI_RING
#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
	struct device_node *node = NULL;
#endif
#endif
	uint8_t fgRet = TRUE;
#if (CFG_SUPPORT_WED_PROXY == 1) && (CFG_SUPPORT_RX_ZERO_COPY == 0)
	uint8_t fgNeed32BAlign = 0;
#endif

	prChipInfo = prDriverData->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (halInitHifMem(pdev, prChipInfo, WIFI_RSV_MEM_WFDMA, GFP_DMA) == -1)
		return -1;

#if CFG_MTK_WIFI_SW_EMI_RING
#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi_misc");
	if (!node) {
		DBGLOG(INIT, ERROR,
			   "WIFI-OF: get wifi_misc device node fail\n");
		u4Idx = WIFI_MISC_MEM_BLOCK_NON_MMIO;
		u4Size = prChipInfo->rsvMemWiFiMisc[u4Idx].size;
		if (!halAllocRsvMem(u4Size,
				&prChipInfo->rsvMemWiFiMisc[u4Idx].rRsvEmiMem,
				WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "RsvEmiMem alloc fail\n");
	} else
		; /* reserve non mmio EMI in halAllocHifMemForWiFiMisc */
	of_node_put(node);
#else
	u4Idx = WIFI_MISC_MEM_BLOCK_NON_MMIO;
	u4Size = prChipInfo->rsvMemWiFiMisc[u4Idx].size;
	if (!halAllocRsvMem(u4Size,
			&prChipInfo->rsvMemWiFiMisc[u4Idx].rRsvEmiMem,
			WIFI_RSV_MEM_WFDMA))
		DBGLOG(INIT, ERROR, "RsvEmiMem alloc fail\n");
#endif
#endif

	for (u4Idx = 0; u4Idx < NUM_OF_TX_RING; u4Idx++) {
		if (u4Idx == TX_RING_DATA1 &&
		    !prBusInfo->tx_ring1_data_idx)
			continue;
		else if (u4Idx == TX_RING_DATA2 &&
			 !prBusInfo->tx_ring2_data_idx)
			continue;
		else if (u4Idx == TX_RING_DATA3 &&
			 !prBusInfo->tx_ring3_data_idx)
			continue;
		else if (u4Idx == TX_RING_DATA_PRIO &&
			 !prBusInfo->tx_prio_data_idx)
			continue;
		else if (u4Idx == TX_RING_DATA_ALTX &&
			 !prBusInfo->tx_altx_data_idx)
			continue;

		if (u4Idx == TX_RING_DATA0 ||
		    u4Idx == TX_RING_DATA1 ||
		    u4Idx == TX_RING_DATA2 ||
		    u4Idx == TX_RING_DATA3 ||
		    u4Idx == TX_RING_DATA_PRIO ||
		    u4Idx == TX_RING_DATA_ALTX)
			u4Size = TX_RING_DATA_SIZE;
		else
			u4Size = TX_RING_CMD_SIZE;

		if (!halAllocRsvMem(u4Size * TXD_SIZE,
			&grMem.rTxDesc[u4Idx], WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "TxDesc[%u] alloc fail\n", u4Idx);
	}

	u4DataNum = prBusInfo->rx_data_ring_num;
	u4EvtNum = prBusInfo->rx_evt_ring_num;
	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		if (u4Idx == RX_RING_DATA0 || u4Idx == RX_RING_DATA1 ||
		    u4Idx == RX_RING_DATA2 || u4Idx == RX_RING_DATA3 ||
		    u4Idx == RX_RING_DATA4 || u4Idx == RX_RING_DATA5) {
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
		if (!halAllocRsvMem(u4Size * RXD_SIZE,
				&grMem.rRxDesc[u4Idx], WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "RxDesc[%u] alloc fail\n", u4Idx);
	}

	for (u4Idx = 0; u4Idx < TX_RING_CMD_SIZE; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_CMD_BUFF_SIZE,
				&grMem.rTxCmdBuf[u4Idx], WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "TxCmdBuf[%u] alloc fail\n", u4Idx);
	}

	for (u4Idx = 0; u4Idx < TX_RING_CMD_SIZE; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_CMD_BUFF_SIZE,
				&grMem.rTxFwdlBuf[u4Idx], WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR,
				"TxFwdlBuf[%u] alloc fail\n",
				u4Idx);
	}


#if (RX_DATA_USE_RSV_MEM == 1)
	u4DataNum = prBusInfo->rx_data_ring_num;
#endif /* RX_DATA_USE_RSV_MEM */
	u4EvtNum = prBusInfo->rx_evt_ring_num;
	for (u4Idx = 0; u4Idx < NUM_OF_RX_RING; u4Idx++) {
		uint32_t u4Cnt, u4PktSize;

		if (u4Idx == RX_RING_DATA0 || u4Idx == RX_RING_DATA1 ||
		    u4Idx == RX_RING_DATA2 || u4Idx == RX_RING_DATA3 ||
		    u4Idx == RX_RING_DATA4 || u4Idx == RX_RING_DATA5) {
#if (RX_DATA_USE_RSV_MEM == 1)
			/* copy path using prealloc rx data size */
#if (CFG_SUPPORT_WED_PROXY == 1) && (CFG_SUPPORT_RX_ZERO_COPY == 0)
			u4Size = prBusInfo->rx_data_ring_size;
			fgNeed32BAlign = 1;
#else
			u4Size = prBusInfo->rx_data_ring_prealloc_size;
#endif
			u4PktSize = CFG_RX_MAX_PKT_SIZE;
			u4DataNum--;
#else /* !RX_DATA_USE_RSV_MEM */
			continue;
#endif /* !RX_DATA_USE_RSV_MEM */
		} else {
			if (u4EvtNum == 0)
				continue;
#if (CFG_SUPPORT_WED_PROXY == 1) && (CFG_SUPPORT_RX_ZERO_COPY == 0)
			fgNeed32BAlign = 0;
#endif
			u4Size = prBusInfo->rx_evt_ring_size;
			u4PktSize = RX_BUFFER_AGGRESIZE;
			u4EvtNum--;
		}
		for (u4Cnt = 0; u4Cnt < u4Size; u4Cnt++) {
			/* allocate based on alignment requirement */
#if (CFG_SUPPORT_WED_PROXY == 1) && (CFG_SUPPORT_RX_ZERO_COPY == 0)
			if (fgNeed32BAlign)
				fgRet = halAllocRsvMemAlign(u4PktSize,
					&grMem.rRxMemBuf[u4Idx][u4Cnt],
					WFDMA_RXDMAD_SDP_MEMORY_ALIGNMENT,
					WIFI_RSV_MEM_WFDMA);
			else
#endif
				fgRet = halAllocRsvMem(u4PktSize,
					&grMem.rRxMemBuf[u4Idx][u4Cnt],
					WIFI_RSV_MEM_WFDMA);

			if (!fgRet) {
				DBGLOG(INIT, ERROR,
					"RxMemBuf[%u][%u] alloc fail\n",
					u4Idx, u4Cnt);
			}
		}
	}

#if HIF_TX_PREALLOC_DATA_BUFFER
#if (CFG_MTK_WIFI_TX_MEM_SLIM == 1)
#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	u4AllocTokNum = HIF_TX_MSDU_TOKEN_NUM_MIN;
#else /* !CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
	u4AllocTokNum = 0;
#endif /* !CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
#else /* !CFG_MTK_WIFI_TX_MEM_SLIM */
	u4AllocTokNum = HIF_TX_MSDU_TOKEN_NUM;
#endif /* !CFG_MTK_WIFI_TX_MEM_SLIM */
	for (u4Idx = 0; u4Idx < u4AllocTokNum; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_MAX_SIZE_PER_FRAME +
				prChipInfo->txd_append_size,
				&grMem.rMsduBuf[u4Idx],
				WIFI_RSV_MEM_WFDMA))
			DBGLOG(INIT, ERROR, "MsduBuf[%u] alloc fail\n", u4Idx);
	}
#endif /* HIF_TX_PREALLOC_DATA_BUFFER */

	DBGLOG(INIT, INFO, "grMem.u4Offset[WIFI_RSV_MEM_WFDMA]=[0x%x]\n",
		grMem.u4Offset[WIFI_RSV_MEM_WFDMA]);

	return 0;
}

void halFreeHifMem(struct platform_device *pdev,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx)
{
	uint32_t i = 0;
	uint32_t count = 0;

	if (u4RsvMemIdx >= WIFI_RSV_MEM_MAX_NUM) {
		DBGLOG(INIT, ERROR, "index out-of-range[%d]\n",
			u4RsvMemIdx);
		return;
	}

	count = sizeof(wifi_rsrv_mems[u4RsvMemIdx]) /
		sizeof(struct wifi_rsrv_mem);

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
#endif /* CFG_SUPPORT_WIFI_RSV_MEM */

#if (CFG_MTK_ANDROID_WMT == 1)
void halCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Align)
{
	if (!halAllocRsvMemAlign(prDescRing->AllocSize,
				 &prDescRing->rMem,
				 u4Align, WIFI_RSV_MEM_WFDMA)) {
		DBGLOG(INIT, ERROR, "Ext Buf alloc failed!\n");
		prDescRing->AllocPa = 0;
		prDescRing->AllocVa = NULL;
		return;
	}

	prDescRing->AllocVa = prDescRing->rMem.va;
	prDescRing->AllocPa = prDescRing->rMem.pa;
	if (prDescRing->AllocVa)
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

void halCopyPathFreeExtBuf(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing)
{
	if (prDescRing->AllocVa == NULL)
		return;

	halFreeRsvMem(prDescRing->AllocSize + prDescRing->rMem.align_size,
		WIFI_RSV_MEM_WFDMA);
	memset(prDescRing, 0, sizeof(struct RTMP_DMABUF));
}
#endif /* CFG_MTK_ANDROID_WMT */

#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
int halAllocHifMemForWiFiMisc(struct platform_device *pdev,
		struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM *prMem = NULL;
	uint32_t u4idx = 0, u4Size;

	prChipInfo = prDriverData->chip_info;

	if (halInitHifMem(pdev, prChipInfo, WIFI_RSV_MEM_WIFI_MISC,
			GFP_DMA) == -1)
		return -1;


	if (prChipInfo->rsvMemWiFiMisc == NULL) {
		DBGLOG(INIT, ERROR, "rsvMemWiFiMisc is NULL\n");
		return -1;
	}

	/* alloc all memory blocks in wifi_misc */
	for (u4idx = 0; u4idx < prChipInfo->rsvMemWiFiMiscSize; u4idx++) {
		prMem = &prChipInfo->rsvMemWiFiMisc[u4idx].rRsvEmiMem;
		u4Size = prChipInfo->rsvMemWiFiMisc[u4idx].size;
		if (!halAllocRsvMem(u4Size, prMem, WIFI_RSV_MEM_WIFI_MISC)) {
			DBGLOG(INIT, ERROR, "RsvEmiMem alloc fail\n");
			continue;
		}
		kalMemZero(prMem->va, u4Size);
	}

	DBGLOG(INIT, DEBUG,
		"grMemWiFiMisc.u4Offset[WIFI_RSV_MEM_WIFI_MISC] = [0x%x], size[%u]\n",
		grMem.u4Offset[WIFI_RSV_MEM_WIFI_MISC],
		prChipInfo->rsvMemWiFiMiscSize);

	return 0;
}
#endif

#if (CFG_MTK_WIFI_TX_MEM_SLIM == 1)
static void halSetTxCmaDataPlatDev(
	struct platform_device *pdev)
{
	g_prTxCmaPlatDev = pdev;
}

static struct platform_device *halGetTxCmaDataPlatDev(void)
{
	return g_prTxCmaPlatDev;
}
#endif /* CFG_MTK_WIFI_TX_MEM_SLIM */

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
static inline uint64_t txCmaAddr2key(void *kaddr)
{
	return ((uint64_t)virt_to_phys(kaddr)) &
		~(TX_CMA_GROUP_SIZE - 1);
}

static struct tx_cma_mem_group *search_tx_cma_group(
	struct wifi_tx_cma_context *prTxCmaCtx, uint64_t key)
{
	struct tx_cma_mem_group *group;

	hash_for_each_possible_rcu(prTxCmaCtx->groups_htbl,
				   group, node, key) {
		if (group->key == key)
			return group;
	}
	return NULL;
}

static struct tx_cma_mem_group *new_tx_cma_group(
	struct wifi_tx_cma_context *prTxCmaCtx, uint64_t key)
{
	struct tx_cma_mem_group *group =
		search_tx_cma_group(prTxCmaCtx, key);

	if (group) {
		DBGLOG(INIT, ERROR, "new group fail, key exist");
		return NULL;
	}

	if (hlist_empty(&prTxCmaCtx->free_group_list)) {
		DBGLOG(INIT, ERROR, "free_group_list is empty");
		return NULL;
	}

	group = hlist_entry(prTxCmaCtx->free_group_list.first,
			    struct tx_cma_mem_group, node);
	hlist_del(prTxCmaCtx->free_group_list.first);
	group->key = key;
	hash_add_rcu(prTxCmaCtx->groups_htbl, &group->node, key);

	return group;
}

static void init_tx_cma_mem_group(
	struct tx_cma_mem_group *group)
{
	INIT_LIST_HEAD(&group->list);
	group->key = 0;
	group->in_used = false;
	group->count = 0;
	group->kaddr = NULL;
	group->size = 0;
}

static bool del_tx_cma_group(
	struct wifi_tx_cma_context *prTxCmaCtx, uint64_t key)
{
	struct tx_cma_mem_group *group =
		search_tx_cma_group(prTxCmaCtx, key);

	if (group) {
		hash_del_rcu(&group->node);
		init_tx_cma_mem_group(group);
		hlist_add_head(&group->node, &prTxCmaCtx->free_group_list);
	}

	return group != NULL;
}

static void inc_tx_cma_cur_data_num(
	struct wifi_tx_cma_context *prTxCmaCtx, uint32_t inc_num)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&prTxCmaCtx->data_num_lock, flags);
	prTxCmaCtx->cur_data_num += inc_num;
	spin_unlock_irqrestore(&prTxCmaCtx->data_num_lock, flags);
}

static void dec_tx_cma_cur_data_num(
	struct wifi_tx_cma_context *prTxCmaCtx, uint32_t dec_num)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&prTxCmaCtx->data_num_lock, flags);
	if (prTxCmaCtx->cur_data_num < dec_num)
		prTxCmaCtx->cur_data_num = 0;
	else
		prTxCmaCtx->cur_data_num -= dec_num;
	spin_unlock_irqrestore(&prTxCmaCtx->data_num_lock, flags);
}

static struct tx_cma_mem_group *alloc_tx_cma_mem_group_data(
	struct wifi_tx_cma_context *prTxCmaCtx)
{
	struct tx_cma_mem_group *group = NULL;
	struct page *pages = NULL;
	void *kaddr = NULL;
	uint32_t tok_cnt = TX_CMA_GROUP_TOK_NUM;
	uint32_t u4Idx = 0;
	struct tx_cma_data *tx_data = NULL;

	pages = cma_alloc(prTxCmaCtx->dev->cma_area, tok_cnt,
			  get_order(TX_CMA_GROUP_SIZE), GFP_KERNEL);
	if (!pages)
		return NULL;

	kaddr = page_address(pages);
	group = new_tx_cma_group(prTxCmaCtx, txCmaAddr2key(kaddr));
	if (!group) {
		DBGLOG(INIT, ERROR, "no free mem group");
		cma_release(prTxCmaCtx->dev->cma_area, pages, tok_cnt);
		return NULL;
	}
	group->in_used = true;
	group->kaddr = kaddr;
	group->size = TX_CMA_GROUP_SIZE;
	group->count += TX_CMA_GROUP_TOK_NUM;

	kaddr = group->kaddr;
	for (u4Idx = 0; u4Idx < tok_cnt; u4Idx++) {
		tx_data = (struct tx_cma_data *)kaddr;
		list_add_tail(&tx_data->group_list, &group->list);
		list_add_tail(&tx_data->free_list,
			&prTxCmaCtx->free_data_list);
		prTxCmaCtx->total_groups_data_num++;
		group->count++;
		kaddr += TX_CMA_TOK_SIZE;
	}

	inc_tx_cma_cur_data_num(prTxCmaCtx, tok_cnt);

	return group;
}

static bool alloc_wifi_tx_cma_mem(
	struct wifi_tx_cma_context *prTxCmaCtx,
	struct ADAPTER *prAdapter)
{
	struct tx_cma_mem_group *group = NULL;
	uint32_t alloc_num = 0, req_num = 0, alloc_cnt = 0;
	struct MSDU_TOKEN_ENTRY *prToken = NULL;
	struct MSDU_TOKEN_INFO *prTokenInfo = NULL;
	unsigned long flags = 0;
	struct list_head *listptr, *n;

	if (!prTxCmaCtx->dev) {
		DBGLOG(INIT, ERROR, "dev null");
		return FALSE;
	}

	if (!prTxCmaCtx->dev->cma_area) {
		DBGLOG(INIT, ERROR, "no cma_area");
		return FALSE;
	}

	if (prTxCmaCtx->cur_data_num >= prTxCmaCtx->config_data_num)
		return TRUE;

	req_num = prTxCmaCtx->config_data_num -
		prTxCmaCtx->cur_data_num;

	while (req_num > alloc_num) {
		group = alloc_tx_cma_mem_group_data(prTxCmaCtx);
		if (!group)
			break;

		alloc_num += TX_CMA_GROUP_TOK_NUM;
	}

	DBGLOG(INIT, DEBUG,
		"req_size: %u, config_data_num: %u, cur_data_num: %u",
		req_num, prTxCmaCtx->config_data_num,
		prTxCmaCtx->cur_data_num);

	if (prAdapter) {
		prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
		spin_lock_irqsave(&prTokenInfo->rTokenLock, flags);

		alloc_cnt = 0;
		list_for_each_safe(listptr, n, &prTokenInfo->init_msdu_list) {
			if (alloc_cnt >= alloc_num)
				break;

			prToken = list_entry(listptr,
				struct MSDU_TOKEN_ENTRY, msdu_list);

			halCopyPathAllocTxCmaTxDataBuf(prToken, 0);

			list_del(&prToken->msdu_list);
			list_add_tail(&prToken->msdu_list,
				&prTokenInfo->free_msdu_list);
			alloc_cnt++;
		}
		spin_unlock_irqrestore(&prTokenInfo->rTokenLock, flags);
	}

	return true;
}

static void free_tx_cma_mem_group_data(
	struct tx_cma_mem_group *group)
{
	struct page *pages = NULL;
	uint32_t tok_cnt = TX_CMA_GROUP_TOK_NUM;
	struct list_head *listptr, *n;
	struct tx_cma_data *data_entry = NULL;
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	prTxCmaCtx = platform_get_drvdata(halGetTxCmaDataPlatDev());

	if (!prTxCmaCtx->dev)
		return;

	list_for_each_safe(listptr, n, &group->list) {
		data_entry = list_entry(listptr,
			struct tx_cma_data, group_list);
		list_del(&data_entry->free_list);
		list_del(&data_entry->group_list);
		prTxCmaCtx->total_groups_data_num--;
		group->count--;
	}

	pages = virt_to_page(group->kaddr);
	if (del_tx_cma_group(prTxCmaCtx, group->key)) {
		cma_release(prTxCmaCtx->dev->cma_area, pages,
			    tok_cnt);

		dec_tx_cma_cur_data_num(prTxCmaCtx, tok_cnt);
	} else {
		DBGLOG(INIT, ERROR, "del group fail!");
	}
}

static struct tx_cma_mem_group *free_tx_data_to_mem_group(
	struct tx_cma_data *tx_data)
{
	struct tx_cma_mem_group *group;
	void *kaddr = (void *)tx_data;
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	prTxCmaCtx = platform_get_drvdata(halGetTxCmaDataPlatDev());

	group = search_tx_cma_group(prTxCmaCtx, txCmaAddr2key(kaddr));
	if (group == NULL) {
		DBGLOG(INIT, DEBUG, "can't find mem group");
		return NULL;
	}

	list_add_tail(&tx_data->group_list, &group->list);
	list_add_tail(&tx_data->free_list, &prTxCmaCtx->free_data_list);
	group->count++;
	prTxCmaCtx->total_groups_data_num++;

	if (group->count == TX_CMA_GROUP_TOK_NUM)
		free_tx_cma_mem_group_data(group);

	return group;
}


static void free_wifi_tx_cma_mem(
	struct wifi_tx_cma_context *prTxCmaCtx,
	struct ADAPTER *prAdapter)
{
	uint32_t release_cnt = 0, req_cnt = 0;
	struct MSDU_TOKEN_INFO *prTokenInfo = NULL;
	struct list_head *listptr, *n;
	struct MSDU_TOKEN_ENTRY *prToken = NULL;
	unsigned long flags = 0;

	if (prAdapter == NULL)
		return;

	if (!prTxCmaCtx->dev)
		return;

	if (prTxCmaCtx->config_data_num >= prTxCmaCtx->cur_data_num)
		return;

	req_cnt = prTxCmaCtx->cur_data_num -
		prTxCmaCtx->config_data_num;
	DBGLOG(INIT, DEBUG, "req_cnt: %u, config num: %u, cur num: %u",
		req_cnt, prTxCmaCtx->config_data_num,
		prTxCmaCtx->cur_data_num);

	prTokenInfo = &prAdapter->prGlueInfo->rHifInfo.rTokenInfo;
	spin_lock_irqsave(&prTokenInfo->rTokenLock, flags);
	list_for_each_safe(listptr, n, &prTokenInfo->free_msdu_list) {
		if (release_cnt >= req_cnt)
			break;

		prToken = list_entry(listptr,
			struct MSDU_TOKEN_ENTRY, msdu_list);
		list_del(&prToken->msdu_list);
		list_add_tail(&prToken->msdu_list,
			&prTokenInfo->init_msdu_list);
		halUninitOneMsduTokenInfo(prAdapter, prToken);
		release_cnt++;
	}
	spin_unlock_irqrestore(&prTokenInfo->rTokenLock, flags);
}

static void wifi_tx_cma_update_mem_data_num(
	struct wifi_tx_cma_context *prTxCmaCtx,
	struct ADAPTER *prAdapter)
{
	if (prTxCmaCtx->config_data_num > prTxCmaCtx->cur_data_num)
		alloc_wifi_tx_cma_mem(prTxCmaCtx, prAdapter);
	else if (prTxCmaCtx->config_data_num < prTxCmaCtx->cur_data_num)
		free_wifi_tx_cma_mem(prTxCmaCtx, prAdapter);
}

void wifi_tx_cma_set_mem_data_num(
	struct ADAPTER *prAdapter,
	uint32_t data_num)
{
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	prTxCmaCtx = platform_get_drvdata(halGetTxCmaDataPlatDev());
	if (data_num > prTxCmaCtx->max_data_num)
		data_num = prTxCmaCtx->max_data_num;
	prTxCmaCtx->config_data_num = data_num;

	if (prTxCmaCtx->is_cma_mem)
		wifi_tx_cma_update_mem_data_num(prTxCmaCtx, prAdapter);
	else
		prTxCmaCtx->cur_data_num = data_num;
}

uint32_t wifi_tx_cma_get_mem_data_num(void)
{
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	prTxCmaCtx = platform_get_drvdata(halGetTxCmaDataPlatDev());
	DBGLOG_LIMITED(INIT, DEBUG, "cur data num: %lu",
		prTxCmaCtx->cur_data_num);
	return prTxCmaCtx->cur_data_num;
}

int halInitTxCmaMem(struct platform_device *pdev)
{
	uint32_t rsv_mem_size = 0;
	uint32_t ret = 0, i = 0;
	struct device_node *node = NULL;
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	node = pdev->dev.of_node;
	if (!node) {
		DBGLOG(INIT, ERROR, "get wifi device node fail.");
		of_node_put(node);
		ret = -1;
		goto exit;
	}
	of_node_put(node);

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"of_reserved_mem_device_init failed(%d).", ret);
	}

	prTxCmaCtx = devm_kzalloc(&pdev->dev,
		sizeof(struct wifi_tx_cma_context), GFP_KERNEL);
	if (!prTxCmaCtx)
		return -ENOMEM;
	platform_set_drvdata(pdev, prTxCmaCtx);
	halSetTxCmaDataPlatDev(pdev);

	prTxCmaCtx->is_cma_mem = pdev->dev.cma_area ? true : false;

	ret = of_property_read_u32(node, "emi-size", &rsv_mem_size);
	if (!ret) {
		prTxCmaCtx->max_data_num =
			(rsv_mem_size / TX_CMA_TOK_SIZE) < TX_CMA_MAX_DATA_NUM ?
			(rsv_mem_size / TX_CMA_TOK_SIZE) : TX_CMA_MAX_DATA_NUM;
	} else
		prTxCmaCtx->max_data_num = TX_CMA_MAX_DATA_NUM;

	prTxCmaCtx->min_data_num = HIF_TX_MSDU_TOKEN_NUM_MIN;

	DBGLOG(INIT, DEBUG,
		"reserved memory size[0x%08x] cma[%u] max[%u] min[%u]",
		rsv_mem_size, prTxCmaCtx->is_cma_mem,
		prTxCmaCtx->max_data_num, prTxCmaCtx->min_data_num);

	spin_lock_init(&prTxCmaCtx->data_num_lock);

	hash_init(prTxCmaCtx->groups_htbl);
	INIT_HLIST_HEAD(&prTxCmaCtx->free_group_list);
	INIT_LIST_HEAD(&prTxCmaCtx->free_data_list);

	for (i = 0; i < TX_CMA_GROUP_NUM; i++) {
		init_tx_cma_mem_group(&prTxCmaCtx->groups[i]);
		hlist_add_head(&prTxCmaCtx->groups[i].node,
			       &prTxCmaCtx->free_group_list);
	}

exit:
	prTxCmaCtx->dev = &pdev->dev;

	return 0;
}

void halFreeTxCmaMem(struct platform_device *pdev)
{
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	if (pdev == NULL) {
		DBGLOG(INIT, ERROR, "pdev is NULL\n");
		return;
	}

	prTxCmaCtx = platform_get_drvdata(pdev);

	devm_kfree(&pdev->dev, prTxCmaCtx);
	wifi_tx_cma_set_mem_data_num(NULL, 0);
}

bool halTxDataCmaIsCmaMem(void)
{
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	prTxCmaCtx = platform_get_drvdata(halGetTxCmaDataPlatDev());
	return prTxCmaCtx->is_cma_mem;
}

void halCopyPathAllocTxCmaTxDataBuf(
	struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx)
{
	struct tx_cma_data *data_entry;
	struct tx_cma_mem_group *group = NULL;
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;

	prTxCmaCtx = platform_get_drvdata(halGetTxCmaDataPlatDev());

	if (list_empty(&prTxCmaCtx->free_data_list)) {
		DBGLOG_LIMITED(INIT, DEBUG, "free_data_list empty");
		return;
	}

	data_entry = list_first_entry(&prTxCmaCtx->free_data_list,
		struct tx_cma_data, free_list);
	group = search_tx_cma_group(prTxCmaCtx,
		txCmaAddr2key((void *)data_entry));
	if (group == NULL) {
		DBGLOG(INIT, DEBUG, "can't find mem group");
		return;
	}
	list_del(&data_entry->free_list);
	list_del(&data_entry->group_list);
	prTxCmaCtx->total_groups_data_num--;
	group->count--;

	if (!data_entry)
		DBGLOG(INIT, ERROR, "data_entry is null\n");

	prToken->prPacket = data_entry;
	prToken->rDmaAddr = 0;
}

phys_addr_t halCopyPathMapTxCmaTxBuf(struct GL_HIF_INFO *prHifInfo,
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

void halCopyPathUnmapTxCmaTxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len)
{
	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)rDmaAddr,
			     u4Len, KAL_DMA_TO_DEVICE);
}

void halCopyPathFreeTxCmaBuf(void *pucSrc, uint32_t u4Len,
	phys_addr_t rDmaAddr, uint32_t u4Idx)
{
	free_tx_data_to_mem_group((struct tx_cma_data *)pucSrc);
}
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
void halAllocNonCacheCmaFromMemGroup(
	struct MSDU_TOKEN_ENTRY *prToken, struct platform_device *pdev)
{
	uint32_t u4GroupIdx = grMem.u4TxMemGroupIdx;
	uint32_t u4GroupTokNum = 0;

	if (u4GroupIdx == CMA_MEM_MAX_SIZE) {
		prToken = NULL;
		goto end;
	}

	if (grMem.u4TxMemGroupRetryTime >=
		TX_CMA_ALLOC_RETRY_TIME_TH) {
		prToken = NULL;
		goto end;
	}

	if (grMem.u4TxMemTokIdx == 0) {
		grMem.rTxCmaMemGroup[u4GroupIdx].va =
			dma_alloc_coherent(&pdev->dev,
				TX_CMA_GROUP_SIZE,
				&grMem.rTxCmaMemGroup[u4GroupIdx].pa,
				GFP_KERNEL);
		if (!grMem.rTxCmaMemGroup[u4GroupIdx].va) {
			grMem.u4TxMemGroupRetryTime++;
			prToken = NULL;
			goto end;
		}
		memset(grMem.rTxCmaMemGroup[u4GroupIdx].va,
			0, TX_CMA_GROUP_SIZE);
		grMem.u4Offset[WIFI_RSV_MEM_WIFI_CMA_NON_CACHE]
			+= TX_CMA_GROUP_SIZE;
	}

	prToken->prPacket =
		grMem.rTxCmaMemGroup[u4GroupIdx].va +
		(uint64_t)prToken->u4DmaLength * grMem.u4TxMemTokIdx;
	prToken->rDmaAddr =
		grMem.rTxCmaMemGroup[u4GroupIdx].pa +
		(uint64_t)prToken->u4DmaLength * grMem.u4TxMemTokIdx;

	u4GroupTokNum = TX_CMA_GROUP_SIZE / prToken->u4DmaLength;
	grMem.u4TxMemTokIdx = (grMem.u4TxMemTokIdx + 1) %
		u4GroupTokNum;
	if (grMem.u4TxMemTokIdx == 0)
		grMem.u4TxMemGroupIdx++;

end:
	return;
}

void halCopyPathAllocNonCacheTxDataBuf(
	struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx)
{
	struct platform_device *pdev = halGetTxCmaDataPlatDev();

	if (u4Idx < HIF_TX_MSDU_TOKEN_NUM_MIN) {
		prToken->prPacket = grMem.rMsduBuf[u4Idx].va;
		prToken->rDmaAddr = grMem.rMsduBuf[u4Idx].pa;
	} else
		halAllocNonCacheCmaFromMemGroup(prToken, pdev);

	if (!prToken->prPacket)
		DBGLOG_LIMITED(INIT, ERROR,
			"alloc tx buf fail u4Idx: %u\n", u4Idx);
}

int halInitTxCmaNonCacheMem(struct platform_device *pdev)
{
	halSetTxCmaDataPlatDev(pdev);
	pdev->dev.dma_coherent = false;

	return 0;
}


int halUninitTxCmaNonCacheMem(void)
{
	struct platform_device *pdev = halGetTxCmaDataPlatDev();
	uint32_t u4Idx = 0;

	grMem.u4TxMemGroupRetryTime = 0;
	grMem.u4TxMemTokIdx = 0;
	grMem.u4TxMemGroupIdx = 0;

	for (u4Idx = 0; u4Idx < CMA_MEM_MAX_SIZE; u4Idx++) {
		if (grMem.rTxCmaMemGroup[u4Idx].va) {
			dma_free_coherent(&pdev->dev,
				TX_CMA_GROUP_SIZE,
				grMem.rTxCmaMemGroup[u4Idx].va,
				(dma_addr_t)grMem.rTxCmaMemGroup[u4Idx].pa);
			grMem.rTxCmaMemGroup[u4Idx].va = NULL;
			grMem.rTxCmaMemGroup[u4Idx].pa = 0;
			grMem.u4Offset[WIFI_RSV_MEM_WIFI_CMA_NON_CACHE]
				-= TX_CMA_GROUP_SIZE;
		}
	}
	return 0;
}


int halAllocHifMemForTxCmaNonCache(
	struct platform_device *pdev,
	struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4Idx = 0;

	prChipInfo = prDriverData->chip_info;

	if (halInitHifMem(pdev, prChipInfo,
			WIFI_RSV_MEM_WIFI_CMA_NON_CACHE, GFP_KERNEL) == -1)
		return -1;

	for (u4Idx = HIF_TX_MSDU_TOKEN_NUM_MIN;
			u4Idx < HIF_TX_MSDU_TOKEN_NUM; u4Idx++) {
		if (!halAllocRsvMem(HAL_TX_MAX_SIZE_PER_FRAME +
				prChipInfo->txd_append_size,
				&grMem.rMsduBuf[u4Idx],
				WIFI_RSV_MEM_WIFI_CMA_NON_CACHE))
			DBGLOG(INIT, ERROR, "MsduBuf[%u] alloc fail\n", u4Idx);
	}

	DBGLOG(INIT, DEBUG,
		"grMem.u4Offset[WIFI_RSV_MEM_WIFI_CMA_NON_CACHE] = [0x%x]\n",
		grMem.u4Offset[WIFI_RSV_MEM_WIFI_CMA_NON_CACHE]);

	return 0;
}


void halGetTxCmaNonCacheMemUsage(void)
{
	DBGLOG(INIT, DEBUG,
		"grMem.u4Offset[WIFI_RSV_MEM_WIFI_CMA_NON_CACHE] = [0x%x]\n",
		grMem.u4Offset[WIFI_RSV_MEM_WIFI_CMA_NON_CACHE]);
}
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */


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

#if CFG_DEDICATED_TXD
static uint32_t halCopyPathCopyTxd(struct MSDU_TOKEN_ENTRY *prToken)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct MSDU_INFO *prMsduInfo = prToken->prMsduInfo;
	uint32_t u4CopyLen = 0;

	glGetChipInfo((void **)&prChipInfo);

	/* copy txd */
	memcpy(prToken->prPacket, prMsduInfo->aucDedicatedTxd,
			NIC_TX_DESC_AND_PADDING_LENGTH);
	u4CopyLen += NIC_TX_DESC_AND_PADDING_LENGTH;
	/* skip txd_append, fill in halTxUpdateCutThroughDesc */
	if (prChipInfo)
		u4CopyLen += prChipInfo->txd_append_size;

	return u4CopyLen;
}
#endif /* CFG_DEDICATED_TXD */

bool halCopyPathCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			  void *pucSrc, uint32_t u4Len)
{
	uint32_t u4CopyLen = 0;

#if CFG_DEDICATED_TXD
	u4CopyLen = halCopyPathCopyTxd(prToken);
#endif /* CFG_DEDICATED_TXD */

	memcpy(prToken->prPacket + u4CopyLen, pucSrc, u4Len);
	return true;
}

#if CFG_SW_TSO
bool halCopyPathCopyTxDataHandleGsoPkt(struct MSDU_TOKEN_ENTRY *prToken,
	uint32_t u4CopyLen)
{
	struct MSDU_INFO *prMsduInfo = prToken->prMsduInfo;

	kalTxProcessTsoSw(&prMsduInfo->rTsoSw, prMsduInfo->prPacket,
				prToken->prPacket, &u4CopyLen);

	return true;
}
#endif /* CFG_SW_TSO */

#if CFG_TX_GSO
bool halCopyPathCopyTxDataSG(struct MSDU_TOKEN_ENTRY *prToken,
			  void *pucSrc, uint32_t u4Len)
{
	struct MSDU_INFO *prMsduInfo = prToken->prMsduInfo;
	struct sk_buff *prSkb = (struct sk_buff *)prMsduInfo->prPacket;
	uint32_t u4CopyLen = 0;
	uint32_t i;

#if CFG_DEDICATED_TXD
	u4CopyLen = halCopyPathCopyTxd(prToken);
#endif /* CFG_DEDICATED_TXD */

#if CFG_SW_TSO
	if (skb_is_gso(prSkb))
		return halCopyPathCopyTxDataHandleGsoPkt(prToken, u4CopyLen);
#endif /* CFG_SW_TSO */

	/*
	 * Please note that skb->data only have header after SG is enabled.
	 * For Non-SG, prSkb->data_len == 0
	 *   => skb_headlen(prSkb) = prSkb->len + 0
	 *
	 * For SG, prSkb->data_len != 0,
	 *   => skb_headlen(prSkb) => only header
	 *   => __skb_pagelen(prSkb) => other payload.
	 */
	/* copy header */
	memcpy(prToken->prPacket + u4CopyLen, prSkb->data, skb_headlen(prSkb));
	u4CopyLen += skb_headlen(prSkb);
	if (!skb_is_nonlinear(prSkb))
		goto skip;

	for (i = 0; i < skb_shinfo(prSkb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(prSkb)->frags[i];
		void *addr = skb_frag_address(frag);

		if (!addr)
			continue;

		memcpy(prToken->prPacket + u4CopyLen, addr,
		       skb_frag_size(frag));
		u4CopyLen += skb_frag_size(frag);
	}

skip:
	return true;
}
#endif /* CFG_TX_GSO */

bool halCopyPathCopyRxData(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMACB *pRxCell,
			  struct RXD_STRUCT *pRxD,
			  struct SW_RFB *prSwRfb,
			  u_int8_t fgUpdatePtr)
{
	struct RTMP_DMABUF *prDmaBuf = &pRxCell->DmaBuf;
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

	if (fgUpdatePtr)
		halUpdateRxDmadSdp(pRxD, prDmaBuf);

	return true;
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
	dma_addr_t rAddr = 0;

	prDescRing->AllocVa = KAL_DMA_ALLOC_COHERENT(
		prHifInfo->prDmaDev, prDescRing->AllocSize, &rAddr);
	prDescRing->AllocPa = (phys_addr_t)rAddr;

	if (prDescRing->AllocVa)
		memset(prDescRing->AllocVa, 0, prDescRing->AllocSize);
}

void halZeroCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Align)
{
	prDescRing->rMem.align_size = 0;
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
#if KERNEL_VERSION(5, 5, 0) <= LINUX_VERSION_CODE
			DBGLOG(INIT, WARN,
			       "va: 0x%llx, mask: 0x%llx, limit: 0x%llx\n",
			       (uint64_t)AllocVa,
			       *prHifInfo->pdev->dev.dma_mask,
			       prHifInfo->pdev->dev.bus_dma_limit);
#else
			DBGLOG(INIT, WARN,
			       "va: 0x%llx, mask: 0x%llx\n",
			       (uint64_t)AllocVa,
			       *prHifInfo->pdev->dev.dma_mask);
#endif
		}

		kalMsleep(1);
	}
	if (u4Cnt == DMA_MAP_RETRY_COUNT) {
		DBGLOG(HAL, ERROR,
		       "dma mapping error![0x%llx][0x%llx][%lu][%d]\n",
		       (uint64_t)rAddr, (uint64_t)AllocVa, AllocSize, i4Res);
		fgRet = FALSE;
	}

	*prAddr = rAddr;
	return fgRet;
}

void *halZeroCopyPathAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDmaBuf,
			    uint32_t u4Num, uint32_t u4Idx)
{
	struct sk_buff *pkt = NULL;
	dma_addr_t rAddr;

	pkt = (struct sk_buff *)__nicRxPacketAlloc(
		prHifInfo->prGlueInfo, (uint8_t **)&prDmaBuf->AllocVa, -1);

	if (!pkt) {
		DBGLOG(HAL, ERROR, "can't allocate rx %lu size packet\n",
		       prDmaBuf->AllocSize);
		prDmaBuf->AllocPa = 0;
		prDmaBuf->AllocVa = NULL;
		return NULL;
	}

	memset(prDmaBuf->AllocVa, 0, prDmaBuf->AllocSize);

	if (!halDmaMapSingleRetry(prHifInfo, prDmaBuf->AllocVa,
				  prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE,
				  &rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
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

bool halZeroCopyPathCopyRxData(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMACB *pRxCell,
			   struct RXD_STRUCT *pRxD,
			   struct SW_RFB *prSwRfb,
			   u_int8_t fgUpdatePtr)
{
	struct RTMP_DMABUF *prDmaBuf = &pRxCell->DmaBuf;
	struct sk_buff *prSkb = (struct sk_buff *)prSwRfb->pvPacket;
	void *pRxPacket = NULL;
	dma_addr_t rAddr;

	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)prDmaBuf->AllocPa,
			     prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE);

#if (CFG_SUPPORT_PAGE_POOL_USE_CMA == 1)
	if (!prSkb->pp_recycle) {
		halCopyPathCopyRxData(prHifInfo, pRxCell, pRxD, prSwRfb, FALSE);
		goto dma_map;
	}
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */

	pRxPacket = pRxCell->pPacket;
	ASSERT(pRxPacket);

	pRxCell->pPacket = prSkb;
	prSwRfb->pvPacket = pRxPacket;
	prDmaBuf->AllocVa = ((struct sk_buff *)pRxCell->pPacket)->data;

#if (CFG_SUPPORT_PAGE_POOL_USE_CMA == 1)
dma_map:
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */

	if (!halDmaMapSingleRetry(prHifInfo, prDmaBuf->AllocVa,
				  prDmaBuf->AllocSize, KAL_DMA_FROM_DEVICE,
				  &rAddr)) {
		DBGLOG(HAL, ERROR, "KAL_DMA_MAP_SINGLE() error!\n");
		return false;
	}
	prDmaBuf->AllocPa = (phys_addr_t)rAddr;

	if (fgUpdatePtr)
		halUpdateRxDmadSdp(pRxD, prDmaBuf);

	return true;
}

phys_addr_t halZeroCopyPathMapTxDataBuf(struct GL_HIF_INFO *prHifInfo,
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


phys_addr_t halZeroCopyPathMapTxCmdBuf(struct GL_HIF_INFO *prHifInfo,
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

void halZeroCopyPathUnmapTxDataBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len)
{
	KAL_DMA_UNMAP_SINGLE(prHifInfo->prDmaDev,
			     (dma_addr_t)rDmaAddr,
			     u4Len, KAL_DMA_TO_DEVICE);
}


void halZeroCopyPathUnmapTxCmdBuf(struct GL_HIF_INFO *prHifInfo,
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

void halZeroCopyPathFreeDataBuf(void *pucSrc, uint32_t u4Len,
	phys_addr_t rDmaAddr, uint32_t u4Idx)
{
	kalMemFree(pucSrc, PHY_MEM_TYPE, u4Len);
}

void halZeroCopyPathFreeCmdBuf(void *pucSrc, uint32_t u4Len)
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

struct HIF_MEM *halGetWiFiMiscRsvEmi(
	struct mt66xx_chip_info *prChipInfo,
	enum WIFI_MISC_MEM_BLOCK_NAME u4name)
{
	uint32_t u4i = 0;

	if (prChipInfo->rsvMemWiFiMisc == NULL ||
			prChipInfo->rsvMemWiFiMiscSize == 0)
		return NULL;

	for (u4i = 0; u4i < prChipInfo->rsvMemWiFiMiscSize; u4i++) {
		if (prChipInfo->rsvMemWiFiMisc[u4i].block_name == u4name)
			return &prChipInfo->rsvMemWiFiMisc[u4i].rRsvEmiMem;
	}

	return NULL;
}

#if CFG_SUPPORT_PAGE_POOL_USE_CMA
void *halZeroCopyPathAllocPagePoolRxBuf(struct GL_HIF_INFO *prHifInfo,
					struct RTMP_DMABUF *prDmaBuf,
					uint32_t u4Num, uint32_t u4Idx)
{
	struct sk_buff *prSkb;
	dma_addr_t rAddr;

	prSkb = kalAllocHifSkb();
	if (!prSkb) {
		DBGLOG(HAL, ERROR, "can't allocate rx %lu size packet\n",
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

	if (g_fgPagePoolDelayAlloc &&
	    time_before(jiffies, g_ulUpdatePagePoolPagePeriod)) {
		mutex_unlock(&g_rPageLock);
		return FALSE;
	}
	g_fgPagePoolDelayAlloc = FALSE;

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
		DBGLOG(HAL, ERROR,
		       "page pool alloc fail[req:%u alloc:%u->%u]\n",
		       u4SetNum, u4CurPageNum, g_u4CurPageNum);
		fgRet = FALSE;
		/* enable delay alloc if page alloc fail */
		if (u4CurPageNum == g_u4CurPageNum)
			g_fgPagePoolDelayAlloc = TRUE;
	} else {
		DBGLOG(HAL, TRACE, "set page pool[req:%u alloc:%u->%u]\n",
		       u4SetNum, u4CurPageNum, g_u4CurPageNum);
	}

	g_ulUpdatePagePoolPagePeriod = jiffies +
		HAL_PAGE_POOL_PAGE_UPDATE_MIN_INTERVAL * HZ / 1000;

	mutex_unlock(&g_rPageLock);

	return fgRet;
}
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */

struct sk_buff *kalAllocRxSkbFromCmaPp(
	struct GLUE_INFO *prGlueInfo, uint8_t **ppucData)
{
	struct page *page;
	struct sk_buff *pkt = NULL;

	page = wifi_page_pool_alloc_page();
#if (CFG_SUPPORT_RETURN_WORK && CFG_SUPPORT_DYNAMIC_PAGE_POOL)
	if (!page) {
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
		kalIncPagePoolPageNum();
#endif
		page = wifi_page_pool_alloc_page();
	}
#endif /* CFG_SUPPORT_RETURN_WOR */
	if (!page)
		goto fail;

	pkt = build_skb(page_to_virt(page), PAGE_SIZE); /* ptr to sk_buff */
	if (!pkt) {
		page_pool_recycle_direct(page->pp, page);
		DBGLOG(HAL, ERROR, "allocate skb fail\n");
		goto fail;
	}
	kmemleak_not_leak(pkt); /* Omit memleak check */
	kalSkbMarkForRecycle(pkt);

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	skb_reserve(pkt, CFG_RADIOTAP_HEADROOM);
#endif

	*ppucData = (uint8_t *) (pkt->data);

fail:
#if (CFG_SUPPORT_HOST_OFFLOAD == 0)
	if (!pkt) {
		pkt = kalPacketAlloc(
			prGlueInfo, CFG_RX_MAX_MPDU_SIZE,
			FALSE, ppucData);
	}
#endif
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
#else
	wifi_page_pool_set_page_num(wifi_page_pool_get_max_page_num());
#endif

	for (u4Idx = 0; u4Idx < u4Num; u4Idx++) {
		prSkb = kalAllocRxSkbFromCmaPp(NULL, &pucRecvBuff);
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
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */

static int halSetMemOpsTrxDesc(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_TRX_DESC_ZERO_COPY_PATH) {
		prMemOps->allocTxDesc = halZeroCopyPathAllocDesc;
		prMemOps->allocRxDesc = halZeroCopyPathAllocDesc;
		prMemOps->freeDesc = halZeroCopyPathFreeDesc;
	} else if (op_sets == WF_MEM_OP_TRX_DESC_COPY_PATH) {
		prMemOps->allocTxDesc = halCopyPathAllocTxDesc;
		prMemOps->allocRxDesc = halCopyPathAllocRxDesc;
		prMemOps->freeDesc = NULL;
	} else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsTxData(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
#if CFG_TX_GSO
	prMemOps->copyTxData = halCopyPathCopyTxDataSG;
#else /* CFG_TX_GSO */
	prMemOps->copyTxData = halCopyPathCopyTxData;
#endif /* CFG_TX_GSO */

	if (op_sets == WF_MEM_OP_TX_DATA_ZERO_COPY_PATH) {
		prMemOps->allocTxDataBuf = halZeroCopyPathAllocTxDataBuf;
		prMemOps->freeDataBuf = halZeroCopyPathFreeDataBuf;
		prMemOps->mapTxDataBuf = halZeroCopyPathMapTxDataBuf;
		prMemOps->unmapTxDataBuf = halZeroCopyPathUnmapTxDataBuf;
	} else if (op_sets == WF_MEM_OP_TX_DATA_COPY_PATH) {
		prMemOps->allocTxDataBuf = halCopyPathAllocTxDataBuf;
		prMemOps->freeDataBuf = NULL;
		prMemOps->mapTxDataBuf = NULL;
		prMemOps->unmapTxDataBuf = NULL;
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
	} else if (op_sets == WF_MEM_OP_TX_DATA_COPY_PATH_TX_DYN_CMA) {
		prMemOps->allocTxDataBuf = halCopyPathAllocTxCmaTxDataBuf;
		prMemOps->freeDataBuf = halCopyPathFreeTxCmaBuf;
		prMemOps->mapTxDataBuf = halCopyPathMapTxCmaTxBuf;
		prMemOps->unmapTxDataBuf = halCopyPathUnmapTxCmaTxBuf;
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */
#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	} else if (op_sets == WF_MEM_OP_TX_DATA_COPY_PATH_TX_NON_CACHE) {
		prMemOps->allocTxDataBuf = halCopyPathAllocNonCacheTxDataBuf;
		prMemOps->freeDataBuf = NULL;
		prMemOps->mapTxDataBuf = NULL;
		prMemOps->unmapTxDataBuf = NULL;
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
	} else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsTxCmd(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_TX_CMD_ZERO_COPY_PATH) {
		prMemOps->allocTxCmdBuf = NULL;
		prMemOps->copyCmd = halZeroCopyPathCopyCmd;
		prMemOps->mapTxCmdBuf = halZeroCopyPathMapTxCmdBuf;
		prMemOps->unmapTxCmdBuf = halZeroCopyPathUnmapTxCmdBuf;
		prMemOps->freeCmdBuf = halZeroCopyPathFreeCmdBuf;
	} else if (op_sets == WF_MEM_OP_TX_CMD_COPY_PATH) {
		prMemOps->allocTxCmdBuf = halCopyPathAllocTxCmdBuf;
		prMemOps->copyCmd = halCopyPathCopyCmd;
		prMemOps->mapTxCmdBuf = NULL;
		prMemOps->unmapTxCmdBuf = NULL;
		prMemOps->freeCmdBuf = NULL;
	} else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsRxData(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_RX_DATA_ZERO_COPY_PATH) {
		prMemOps->allocRxDataBuf = halZeroCopyPathAllocRxBuf;
		prMemOps->copyRxData = halZeroCopyPathCopyRxData;
		prMemOps->freePacket = halZeroCopyPathFreePacket;
		prMemOps->mapRxBuf = halZeroCopyPathMapRxBuf;
		prMemOps->unmapRxBuf = halZeroCopyPathUnmapRxBuf;
	} else if (op_sets == WF_MEM_OP_RX_DATA_COPY_PATH) {
		prMemOps->allocRxDataBuf = halCopyPathAllocRxBuf;
		prMemOps->copyRxData = halCopyPathCopyRxData;
		prMemOps->freePacket = NULL;
		prMemOps->mapRxBuf = NULL;
		prMemOps->unmapRxBuf = NULL;
#if CFG_SUPPORT_PAGE_POOL_USE_CMA
	} else if (op_sets == WF_MEM_OP_RX_DATA_ZERO_COPY_PATH_PAGE_POOL) {
		prMemOps->allocRxDataBuf = halZeroCopyPathAllocPagePoolRxBuf;
		prMemOps->copyRxData = halZeroCopyPathCopyRxData;
		prMemOps->freePacket = halZeroCopyPathFreePagePoolPacket;
		prMemOps->mapRxBuf = halZeroCopyPathMapRxBuf;
		prMemOps->unmapRxBuf = halZeroCopyPathUnmapRxBuf;
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */
	} else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsRxEvt(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_RX_EVT_ZERO_COPY_PATH) {
		prMemOps->allocRxEvtBuf = halZeroCopyPathAllocRxBuf;
		prMemOps->copyEvent = halZeroCopyPathCopyEvent;
	} else if (op_sets == WF_MEM_OP_RX_EVT_COPY_PATH) {
		prMemOps->allocRxEvtBuf = halCopyPathAllocRxBuf;
		prMemOps->copyEvent = halCopyPathCopyEvent;
	} else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsTxDump(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_TX_DUMP_ZERO_COPY_PATH)
		prMemOps->dumpTx = NULL;
	else if (op_sets == WF_MEM_OP_TX_DUMP_COPY_PATH)
		prMemOps->dumpTx = halCopyPathDumpTx;
	else if (op_sets == WF_MEM_OP_TX_DUMP_NULL)
		prMemOps->dumpTx = NULL;
	else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsRxDump(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_RX_DUMP_ZERO_COPY_PATH)
		prMemOps->dumpRx = halZeroCopyPathDumpRx;
	else if (op_sets == WF_MEM_OP_RX_DUMP_COPY_PATH)
		prMemOps->dumpRx = halCopyPathDumpRx;
	else if (op_sets == WF_MEM_OP_RX_DUMP_NULL)
		prMemOps->dumpRx = NULL;
	else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsExtBuf(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_EXT_BUF_ZERO_COPY_PATH) {
		prMemOps->allocExtBuf = halZeroCopyPathAllocExtBuf;
		prMemOps->freeExtBuf = halZeroCopyPathFreeDesc;
#if (CFG_MTK_ANDROID_WMT == 1)
	} else if (op_sets == WF_MEM_OP_EXT_BUF_COPY_PATH) {
		prMemOps->allocExtBuf = halCopyPathAllocExtBuf;
		prMemOps->freeExtBuf = halCopyPathFreeExtBuf;
#endif /* CFG_MTK_ANDROID_WMT */
	} else if (op_sets == WF_MEM_OP_EXT_BUF_NULL) {
		prMemOps->allocExtBuf = NULL;
		prMemOps->freeExtBuf = NULL;
	} else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsRuntimeMem(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_RUNTIME_MEM_ZERO_COPY_PATH)
		prMemOps->allocRuntimeMem = halZeroCopyPathAllocRuntimeMem;
	else if (op_sets == WF_MEM_OP_RUNTIME_MEM_NULL)
		prMemOps->allocRuntimeMem = NULL;
	else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsWifiMiscEmi(
	struct HIF_MEM_OPS *prMemOps,
	enum WIFI_MEM_OPER_SETS op_sets)
{
	if (op_sets == WF_MEM_OP_WIFI_MISC_EMI_ENABLE)
		prMemOps->getWifiMiscRsvEmi = halGetWiFiMiscRsvEmi;
	else if (op_sets == WF_MEM_OP_WIFI_MISC_EMI_NULL)
		prMemOps->getWifiMiscRsvEmi = NULL;
	else
		DBGLOG(INIT, ERROR, "Operation Set undefined\n");

	return 0;
}

static int halSetMemOpsAndroid(
	struct HIF_MEM_OPS *prMemOps)
{
	/* with pre-allocated memory (DTS) */
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
	struct wifi_tx_cma_context *prTxCmaCtx = NULL;
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

	halSetMemOpsTrxDesc(prMemOps, WF_MEM_OP_TRX_DESC_COPY_PATH);

#if (CFG_MTK_WIFI_TX_MEM_SLIM == 1)
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
	prTxCmaCtx = platform_get_drvdata(pdev);
	if (prTxCmaCtx->is_cma_mem) {
		halSetMemOpsTxData(prMemOps,
		WF_MEM_OP_TX_DATA_COPY_PATH_TX_DYN_CMA);
	} else {
		halSetMemOpsTxData(prMemOps,
			WF_MEM_OP_TX_DATA_ZERO_COPY_PATH);
	}
#elif (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	halSetMemOpsTxData(prMemOps,
		WF_MEM_OP_TX_DATA_COPY_PATH_TX_NON_CACHE);
#else /* !CFG_MTK_WIFI_TX_CMA_MEM */
	halSetMemOpsTxData(prMemOps, WF_MEM_OP_TX_DATA_ZERO_COPY_PATH);
#endif /* !CFG_MTK_WIFI_TX_CMA_MEM */
#else /* !CFG_MTK_WIFI_TX_MEM_SLIM */
	halSetMemOpsTxData(prMemOps, WF_MEM_OP_TX_DATA_COPY_PATH);
#endif /* !CFG_MTK_WIFI_TX_MEM_SLIM */

	halSetMemOpsTxCmd(prMemOps, WF_MEM_OP_TX_CMD_COPY_PATH);

#if (CFG_MTK_WIFI_TX_MEM_SLIM == 1)
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
	halSetMemOpsTxDump(prMemOps, WF_MEM_OP_TX_DUMP_NULL);
#else /* !CFG_MTK_WIFI_TX_CMA_MEM */
	halSetMemOpsTxDump(prMemOps, WF_MEM_OP_TX_DUMP_COPY_PATH);
#endif /* !CFG_MTK_WIFI_TX_CMA_MEM */
#else /* !CFG_MTK_WIFI_TX_MEM_SLIM */
	halSetMemOpsTxDump(prMemOps, WF_MEM_OP_TX_DUMP_COPY_PATH);
#endif /* !CFG_MTK_WIFI_TX_MEM_SLIM */

#if CFG_SUPPORT_PAGE_POOL_USE_CMA
	halSetMemOpsRxData(prMemOps,
		WF_MEM_OP_RX_DATA_ZERO_COPY_PATH_PAGE_POOL);
#elif (CFG_SUPPORT_RX_ZERO_COPY == 1)
	halSetMemOpsRxData(prMemOps, WF_MEM_OP_RX_DATA_ZERO_COPY_PATH);
#else
	halSetMemOpsRxData(prMemOps, WF_MEM_OP_RX_DATA_COPY_PATH);
#endif

	halSetMemOpsRxEvt(prMemOps, WF_MEM_OP_RX_EVT_COPY_PATH);

#if (CFG_SUPPORT_RX_ZERO_COPY == 1)
	halSetMemOpsRxDump(prMemOps, WF_MEM_OP_RX_DUMP_ZERO_COPY_PATH);
#else
	halSetMemOpsRxDump(prMemOps, WF_MEM_OP_RX_DUMP_COPY_PATH);
#endif

#if (CFG_MTK_ANDROID_WMT == 1)
	halSetMemOpsExtBuf(prMemOps, WF_MEM_OP_EXT_BUF_COPY_PATH);
#endif

	halSetMemOpsRuntimeMem(prMemOps, WF_MEM_OP_RUNTIME_MEM_NULL);

	halSetMemOpsWifiMiscEmi(prMemOps, WF_MEM_OP_WIFI_MISC_EMI_ENABLE);

	return 0;
}

static int halSetMemOpsPC(
	struct HIF_MEM_OPS *prMemOps)
{
	/* w/o pre-allocated memory (DTS) */
	halSetMemOpsTrxDesc(prMemOps,
		WF_MEM_OP_TRX_DESC_ZERO_COPY_PATH);

	halSetMemOpsTxData(prMemOps,
		WF_MEM_OP_TX_DATA_ZERO_COPY_PATH);

	halSetMemOpsTxCmd(prMemOps,
		WF_MEM_OP_TX_CMD_ZERO_COPY_PATH);

	halSetMemOpsTxDump(prMemOps,
		WF_MEM_OP_TX_DUMP_NULL);

	halSetMemOpsRxData(prMemOps,
		WF_MEM_OP_RX_DATA_ZERO_COPY_PATH);

	halSetMemOpsRxEvt(prMemOps,
		WF_MEM_OP_RX_EVT_ZERO_COPY_PATH);

	halSetMemOpsRxDump(prMemOps,
		WF_MEM_OP_RX_DUMP_NULL);

	halSetMemOpsExtBuf(prMemOps,
		WF_MEM_OP_EXT_BUF_ZERO_COPY_PATH);

	halSetMemOpsRuntimeMem(prMemOps,
		WF_MEM_OP_RUNTIME_MEM_ZERO_COPY_PATH);

	halSetMemOpsWifiMiscEmi(prMemOps,
		WF_MEM_OP_WIFI_MISC_EMI_NULL);

	return 0;
}

int halSetMemOps(
	struct platform_device *prPlatDev,
	struct HIF_MEM_OPS *prMemOps)
{

	if (prPlatDev)
		/* with pre-allocated memory (DTS) */
		halSetMemOpsAndroid(prMemOps);
	else
		/* w/o pre-allocated memory (DTS) */
		halSetMemOpsPC(prMemOps);

	return 0;
}

void glUpdateRxCopyMemOps(struct HIF_MEM_OPS *prMemOps)
{
	halSetMemOpsRxData(prMemOps, WF_MEM_OP_RX_DATA_COPY_PATH);
	halSetMemOpsRxDump(prMemOps, WF_MEM_OP_RX_DUMP_COPY_PATH);
}
