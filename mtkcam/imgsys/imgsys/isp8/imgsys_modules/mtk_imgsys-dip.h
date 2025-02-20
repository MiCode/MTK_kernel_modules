/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *
 */

#ifndef _MTK_DIP_DIP_H_
#define _MTK_DIP_DIP_H_

// Standard C header file

// kernel header file

// mtk imgsys local header file
#include "./../mtk_imgsys-dev.h"


// Local header file
#include "./../mtk_imgsys-engine-isp8.h"
#include "./../mtk_imgsys-debug.h"

#include <mtk_imgsys-cmdq.h>


/********************************************************************
 * Global Define
 ********************************************************************/
/* DIP */
#define DIP_TOP_ADDR	0x34100000
#define DIP_TOP_ADDR_P	0x15100000
#define IMGSYS_DIP_BASE		(0x34110000)
#define IMGSYS_DIP_BASE_P	(0x15110000)

#define DIP_DBG_SEL		0x230
#define DIP_DBG_OUT		0x234
#define DIP_DMATOP_DBG_SEL	0x1088
#define DIP_DMATOP_DBG_PORT	0x108C
#define DIP_DMANR2_DBG_SEL	0x1090
#define DIP_DMANR2_DBG_PORT	0x1094

#define DIP_NR3D_DBG_SEL	0x501C
#define DIP_NR3D_DBG_CNT	0x5020
#define DIP_NR3D_DBG_ST		0x5024
#define DIP_NR3D_DBG_POINTS	64
#define DIP_YUFD_DBG_SEL	0x912C

/* DIP NR1 */
#define DIP_NR1_ADDR		0x34150000
#define DIP_NR1_ADDR_P		0x15150000

/* DIP NR2 */
#define DIP_NR2_ADDR		0x34160000
#define DIP_NR2_ADDR_P		0x15160000

#define DIP_DMA_NAME_MAX_SIZE	20

#define DIP_IMGI_STATE_CHECKSUM		(0x00100)
#define DIP_IMGI_LINE_PIX_CNT_TMP	(0x00200)
#define DIP_IMGI_LINE_PIX_CNT		(0x00300)
#define DIP_IMGI_IMPORTANT_STATUS	(0x00400)
#define DIP_IMGI_SMI_DEBUG_DATA_CASE0	(0x00500)
#define DIP_IMGI_TILEX_BYTE_CNT		(0x00600)
#define DIP_IMGI_TILEY_CNT			(0x00700)
#define DIP_IMGI_BURST_LINE_CNT		(0x00800)
#define DIP_IMGI_XFER_Y_CNT			(0x00900)
#define DIP_IMGI_UFO_STATE_CHECKSUM		(0x00A00)
#define DIP_IMGI_UFO_LINE_PIX_CNT_TMP	(0x00B00)
#define DIP_IMGI_UFO_LINE_PIX_CNT		(0x00C00)
#define DIP_IMGI_FIFO_DEBUG_DATA_CASE1		(0x10600)
#define DIP_IMGI_FIFO_DEBUG_DATA_CASE3		(0x30600)
#define DIP_YUVO_T1_FIFO_DEBUG_DATA_CASE1	(0x10700)
#define DIP_YUVO_T1_FIFO_DEBUG_DATA_CASE3	(0x30700)

#define DIP_CQ_DESC_NUM		280 // align with userspace
#define DIP_REG_SIZE		(0x1D000) // align with userspace
#define DIP_TDR_BUF_MAXSZ 163840 // align with userspace
/********************************************************************
 * Enum Define
 ********************************************************************/
enum DIPDmaDebugType {
	DIP_ORI_RDMA_DEBUG,
	DIP_ORI_RDMA_UFO_DEBUG,
	DIP_ORI_WDMA_DEBUG,
	DIP_ULC_RDMA_DEBUG,
	DIP_ULC_WDMA_DEBUG,
};

/********************************************************************
 * Structure Define
 ********************************************************************/
struct DIPRegDumpInfo {
	unsigned int oft;
	unsigned int end;
};

struct DIPDmaDebugInfo {
	char DMAName[DIP_DMA_NAME_MAX_SIZE];
	enum DIPDmaDebugType DMADebugType;
	unsigned int DMAIdx;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void imgsys_dip_set_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_dip_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_dip_cmdq_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev,
		void *pkt, int hw_idx);
void imgsys_dip_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine);

void imgsys_dip_uninit(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_dip_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode);
int imgsys_dip_tfault_callback(int port,
			dma_addr_t mva, void *data);
bool imgsys_dip_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine);
#endif /* _MTK_DIP_DIP_H_ */
