// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Shih-Fang Chuang <shih-fang.chuang@mediatek.com>
 *
 */

// Standard C header file

// kernel header file
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include "mtk-hcp.h"
#include "mtk_imgsys-v4l2-debug.h"

// GCE header
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

// drivers/misc/mediatek/iommu/
#include "iommu_debug.h"

// mtk imgsys local header file

// Local header file
#include "mtk_imgsys-traw.h"

/********************************************************************
 * Global Define
 ********************************************************************/
#define TRAW_INIT_ARRAY_COUNT	1

#define TRAW_CTL_ADDR_END		0x530
#define TRAW_DMA_ADDR_OFST		0x4000
#define TRAW_DMA_ADDR_END		0x573C
#define TRAW_MOD_ADDR_OFST		0x8000
#define TRAW_MAX_ADDR_OFST		0xECA8

#define TRAW_HW_SET		2

#define IMG_MAIN_BASE		(0x15000000)
#define TRAW_TOP_BASE		(0x15710000)
#define TRAW_BASE			(0x15700000)
#define LTRAW_BASE			(0x15040000)
#define TRAW_DL_RST			(0x260)
#define SW_RST				(0x000C)
/********************************************************************
 * Global Variable
 ********************************************************************/
const struct mtk_imgsys_init_array
			mtk_imgsys_traw_init_ary[TRAW_INIT_ARRAY_COUNT] = {
	{0x00D4, 0x80000000}, /* TRAWCTL_INT1_EN */
};

#if IF_0_DEFINE //YWTBD K DBG
static struct TRAWDmaDebugInfo g_DMADbgIfo[] = {
	{"IMGI", TRAW_ORI_RDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"IMGI_UFD", TRAW_ORI_RDMA_UFD_DEBUG, EVEN_ODD_MODE(0)},
	{"UFDI", TRAW_ORI_RDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"IMGBI", TRAW_ORI_RDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"IMGBI_UFD", TRAW_ORI_RDMA_UFD_DEBUG, EVEN_ODD_MODE(0)},
	{"IMGCI", TRAW_ORI_RDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"IMGCI_UFD", TRAW_ORI_RDMA_UFD_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTI_T1", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTI_T2", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTI_T3", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTI_T4", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTI_T5", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"CACI_T1", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTI_T1", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTI_T2", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTI_T3", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTI_T4", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTI_T5", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVO_T1", TRAW_ORI_WDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"YUVBO_T1", TRAW_ORI_WDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"TIMGO_T1", TRAW_ORI_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVCO_T1", TRAW_ORI_WDMA_DEBUG, EVEN_ODD_MODE(1)},
	{"YUVDO_T1", TRAW_ORI_WDMA_DEBUG, EVEN_ODD_MODE(1)}, // 22
	{"YUVO_T2", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVBO_T2", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVO_T3", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVBO_T3", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVO_T4", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVBO_T4", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"YUVO_T5", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSO_D1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSBO_D1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSHO_D1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSYO_D1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},  // 33
	{"SMTO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTO_T2", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTO_T3", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTO_T4", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTO_T5", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTO_T2", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"TNCSTO_T3", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTCI_T1", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTCI_T4", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"SMTCO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},  // 44
	{"SMTCO_T4", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"LTMSTI_T1", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"LTMSTI_T2", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"LTMSO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"LTMSHO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"LTMSTO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"LTMSTO_T2", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"DRZS4NO_T1", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
	{"IMGI_N_UFD", TRAW_ORI_RDMA_UFD_DEBUG, EVEN_ODD_MODE(1)},
	{"IMGBI_N_UFD", TRAW_ORI_RDMA_UFD_DEBUG, EVEN_ODD_MODE(1)},
	{"SMTI_T6", TRAW_ULC_RDMA_DEBUG, EVEN_ODD_MODE(0)}, // 55
	{"SMTO_T6", TRAW_ULC_WDMA_DEBUG, EVEN_ODD_MODE(0)},
};
#endif

static unsigned int g_RegBaseAddr = TRAW_A_BASE_ADDR;

static void __iomem *g_trawRegBA, *g_ltrawRegBA, *g_ispMainRegBA;

static unsigned int g_IOMMUDumpPort;

#if IF_0_DEFINE //YWTBD K DBG
static unsigned int ExeDbgCmd(struct mtk_imgsys_dev *a_pDev,
			void __iomem *a_pRegBA,
			unsigned int a_DdbSel,
			unsigned int a_DbgOut,
			unsigned int a_DbgCmd)
{
	unsigned int DbgData = 0;
	unsigned int DbgOutReg = g_RegBaseAddr + a_DbgOut;
	void __iomem *pDbgSel = (void *)(a_pRegBA + a_DdbSel);
	void __iomem *pDbgPort = (void *)(a_pRegBA + a_DbgOut);

	iowrite32(a_DbgCmd, pDbgSel);
	DbgData = (unsigned int)ioread32(pDbgPort);
	pr_info("[0x%08X](0x%08X,0x%08X)\n",
		a_DbgCmd, DbgOutReg, DbgData);

	return DbgData;
}

static void imgsys_traw_dump_dma(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int Idx = 0;
	unsigned int DbgCmd = 0;
	unsigned int DmaDegInfoSize = sizeof(struct TRAWDmaDebugInfo);
	unsigned int DebugCnt = sizeof(g_DMADbgIfo)/DmaDegInfoSize;
	enum TRAWDmaDebugType DbgTy = TRAW_ORI_RDMA_DEBUG;
	char even_odd = 0;
	/* Dump DMA Debug Info */
	for (Idx = 0; Idx < DebugCnt; Idx++) {
		/* state_checksum */
		DbgCmd = TRAW_IMGI_STATE_CHECKSUM + Idx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		/* line_pix_cnt_tmp */
		DbgCmd = TRAW_IMGI_LINE_PIX_CNT_TMP + Idx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		/* line_pix_cnt */
		DbgCmd = TRAW_IMGI_LINE_PIX_CNT + Idx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

		/* even_odd */
		even_odd = g_DMADbgIfo[Idx].even_odd_mode;
		if (g_DMADbgIfo[Idx].even_odd_mode) {
			/* state_checksum */
			DbgCmd = (TRAW_IMGI_STATE_CHECKSUM|EVEN_ODD_SEL) + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
			/* line_pix_cnt_tmp */
			DbgCmd = (TRAW_IMGI_LINE_PIX_CNT_TMP|EVEN_ODD_SEL) + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
			/* line_pix_cnt */
			DbgCmd = (TRAW_IMGI_LINE_PIX_CNT|EVEN_ODD_SEL) + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		}

		DbgTy = g_DMADbgIfo[Idx].DMADebugType;

		/* important_status */
		if (DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_IMPORTANT_STATUS + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
			DbgCmd);
		}

		/* smi_debug_data (case 0) or cmd_data_cnt */
		if (DbgTy == TRAW_ORI_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_SMI_DEBUG_DATA_CASE0 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
			DbgCmd);
			if (even_odd) {
				DbgCmd = (TRAW_IMGI_SMI_DEBUG_DATA_CASE0|EVEN_ODD_SEL) + Idx;
				ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
			}
		}

		/* ULC_RDMA or ULC_WDMA */
		if (DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_TILEX_BYTE_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = TRAW_IMGI_TILEY_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* smi_dbg_data(case 0) or burst_line_cnt or input_v_cnt */
		if (DbgTy == TRAW_ORI_WDMA_DEBUG ||
			DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_BURST_LINE_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* ORI_RDMA */
		if (DbgTy == TRAW_ORI_RDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_FIFO_DEBUG_DATA_CASE1 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = TRAW_IMGI_FIFO_DEBUG_DATA_CASE3 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			if (even_odd) {
				DbgCmd = (TRAW_IMGI_FIFO_DEBUG_DATA_CASE1|EVEN_ODD_SEL) + Idx;
				ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
				DbgCmd = (TRAW_IMGI_FIFO_DEBUG_DATA_CASE3|EVEN_ODD_SEL) + Idx;
				ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
			}
		}

		/* ORI_WDMA */
		if (DbgTy == TRAW_ORI_WDMA_DEBUG) {
			DbgCmd = TRAW_YUVO_T1_FIFO_DEBUG_DATA_CASE1 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = TRAW_YUVO_T1_FIFO_DEBUG_DATA_CASE3 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* xfer_y_cnt */
		if (DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_XFER_Y_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}
	}
}

static void imgsys_traw_dump_cq(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	void __iomem *pCQEn = (void *)(a_pRegBA + TRAW_DIPCQ_CQ_EN);

	/* arx/atx/drx/dtx_state */
	DbgCmd = 0x00000005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* Thr(0~3)_state */
	DbgCmd = 0x00010005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

	/* Set DIPCQ_CQ_EN[28] to 1 */
	iowrite32(0x10000000, pCQEn);
	/* cqd0_checksum0 */
	DbgCmd = 0x00000005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd0_checksum1 */
	DbgCmd = 0x00010005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd0_checksum2 */
	DbgCmd = 0x00020005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd1_checksum0 */
	DbgCmd = 0x00040005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd1_checksum1 */
	DbgCmd = 0x00050005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd1_checksum2 */
	DbgCmd = 0x00060005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa0_checksum0 */
	DbgCmd = 0x00080005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa0_checksum1 */
	DbgCmd = 0x00090005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa0_checksum2 */
	DbgCmd = 0x000A0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa1_checksum0 */
	DbgCmd = 0x000C0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa1_checksum1 */
	DbgCmd = 0x000D0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa1_checksum2 */
	DbgCmd = 0x000E0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

}
static void imgsys_traw_dump_drzh2nt1(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int times = 16; //set sel 16 times

	for (int i = 0; i < times; i++) {
		DbgCmd = i;
		pr_info("[drzh2nt1 internal] %d\n",DbgCmd);
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
        }
}
static void imgsys_traw_dump_drzh2n(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;

	/* drzh2n_t1 state_checksum  */
	pr_info("[drzh2n_t1]\n");
	DbgCmd = 0x00018001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t1 line_pix_cnt_tmp */
	DbgCmd = 0x00028001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t1 line_pix_cnt */
	DbgCmd = 0x00038001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t1 other status */
	DbgCmd = 0x00048001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 state_checksum */
	pr_info("[drzh2n_t2]\n");
	DbgCmd = 0x00018101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 line_pix_cnt_tmp */
	DbgCmd = 0x00028101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 line_pix_cnt */
	DbgCmd = 0x00038101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 other status */
	DbgCmd = 0x00048101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3  state_checksum */
	pr_info("[drzh2n_t3]\n");
	DbgCmd = 0x00018601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3 line_pix_cnt_tmp */
	DbgCmd = 0x00028601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3 line_pix_cnt */
	DbgCmd = 0x00038601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3 other status */
	DbgCmd = 0x00048601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 state_checksum */
	pr_info("[drzh2n_t4]\n");
	DbgCmd = 0x00018701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 line_pix_cnt_tmp */
	DbgCmd = 0x00028701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 line_pix_cnt */
	DbgCmd = 0x00038701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 other status */
	DbgCmd = 0x00048701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 state_checksum */
	pr_info("[drzh2n_t5]\n");
	DbgCmd = 0x00018801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 line_pix_cnt_tmp */
	DbgCmd = 0x00028801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 line_pix_cnt */
	DbgCmd = 0x00038801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 other status */
	DbgCmd = 0x00048801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t6 state_checksum */
	pr_info("[drzh2n_t6]\n");
	DbgCmd = 0x00018901;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

	/* drzh2n_t6 line_pix_cnt_tmp */
	DbgCmd = 0x00028901;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t6 line_pix_cnt */
	DbgCmd = 0x00038901;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t6 other status */
	DbgCmd = 0x00048901;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

}

static void imgsys_traw_dump_smto(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;

	/* smto_t3 line_pix_cnt_tmp */
	DbgCmd = 0x0002C401;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* smto_t3 line_pix_cnt */
	DbgCmd = 0x0003C401;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* smto_t3 handshake signal */
	DbgCmd = 0x0004C401;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
}

static void imgsys_traw_tdr_dbg(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;

	/* wpe_wif_t1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x80005;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	pr_info("[imgsys_traw_tdr_dbg] DbgCmd(%d), Data(0x%X)\n",
		DbgCmd, DbgData & 0xFFFFFFFF);

	/* wpe_wif_t1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x10005;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	pr_info("[imgsys_traw_tdr_dbg] DbgCmd(%d), Data(0x%X)\n",
		DbgCmd, DbgData & 0xFFFFFFFF);

	/* wpe_wif_t1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x40005;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	pr_info("[imgsys_traw_tdr_dbg] DbgCmd(%d), Data(0x%X)\n",
		DbgCmd, DbgData & 0xFFFFFFFF);
}

static void imgsys_traw_dump_dl(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int DbgLineCnt = 0, DbgRdy = 0, DbgReq = 0;
	unsigned int DbgLineCntReg = 0;

	/* wpe_wif_t1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000107;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t1_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000108;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t1_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000109;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t1_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_t2_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000207;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t2_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000208;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t2_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000209;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t2_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* traw_dip_d1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000007;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[traw_dip_d1_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000008;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[traw_dip_d1_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000009;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[traw_dip_d1_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_t3_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000307;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t3_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000308;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t3_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000309;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t3_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_t4_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000407;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t4_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000408;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t4_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000409;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t4_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_t4n_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000507;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t4n_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000508;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t4n_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000509;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t4n_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_t5_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000607;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t5_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000608;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t5_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000609;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t5_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);
}
#endif

int imgsys_traw_tfault_callback(int port, dma_addr_t mva, void *cb_data)
{
	unsigned int i = 0;
	char DbgStr[128];


	for (i = TRAW_DMA_ADDR_OFST; i <= TRAW_DMA_ADDR_END; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(TRAW_A_BASE_ADDR + i),
			(unsigned int)ioread32((void *)(g_trawRegBA + i)),
			(unsigned int)ioread32((void *)(g_trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(g_trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(g_trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}

	for (i = 0x0; i <= TRAW_CTL_ADDR_END; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(TRAW_A_BASE_ADDR + i),
			(unsigned int)ioread32((void *)(g_trawRegBA + i)),
			(unsigned int)ioread32((void *)(g_trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(g_trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(g_trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}

	return 0;
}

int imgsys_ltraw_tfault_callback(int port, dma_addr_t mva, void *cb_data)
{
	unsigned int i = 0;
	char DbgStr[128];

	/* Dma registers */
	for (i = TRAW_DMA_ADDR_OFST; i <= TRAW_DMA_ADDR_END; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(TRAW_B_BASE_ADDR + i),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i)),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}

	for (i = 0x0; i <= TRAW_CTL_ADDR_END; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(TRAW_B_BASE_ADDR + i),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i)),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(g_ltrawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}

	return 0;
}

void imgsys_traw_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode)
{
	u64 iova_addr = tuning_iova;
	u64 *cq_desc = NULL;
	struct mtk_imgsys_traw_dtable *dtable = NULL;
	unsigned int i = 0, j = 0, tun_ofst = 0;
	struct flush_buf_info traw_buf_info;
        #if SMVR_DECOUPLE
	void *virt_mem_base = mtk_hcp_get_traw_mem_virt(imgsys_dev->scp_pdev, mode);
	#else
	void *virt_mem_base = mtk_hcp_get_traw_mem_virt(imgsys_dev->scp_pdev);
        #endif
	size_t dtbl_sz = sizeof(struct mtk_imgsys_traw_dtable);

	/* HWID defined in hw_definition.h */
	for (i = IMGSYS_TRAW; i <= IMGSYS_LTR; i++) {
		if (user_info->priv[i].need_update_desc) {
			if (iova_addr) {
				cq_desc = (u64 *)
					((void *)(virt_mem_base + user_info->priv[i].desc_offset));
				for (j = 0; j < TRAW_CQ_DESC_NUM; j++) {
					dtable = (struct mtk_imgsys_traw_dtable *)cq_desc + j;
					if ((dtable->addr_msb & PSEUDO_DESC_TUNING) ==
						PSEUDO_DESC_TUNING) {
						tun_ofst = dtable->addr;
						dtable->addr = (tun_ofst + iova_addr) & 0xFFFFFFFF;
						dtable->addr_msb =
							((tun_ofst + iova_addr) >> 32) & 0xF;
                        if (imgsys_traw_7sp_dbg_enable()) {
						pr_debug(
							"%s: tuning_buf_iova(0x%llx) des_ofst(0x%08x) cq_kva(0x%p) dtable(0x%x/0x%x/0x%x)\n",
							__func__, iova_addr,
							user_info->priv[i].desc_offset,
							cq_desc, dtable->empty, dtable->addr,
							dtable->addr_msb);
					}
				}
			}
			}
			//
			#if SMVR_DECOUPLE
			traw_buf_info.fd = mtk_hcp_get_traw_mem_cq_fd(imgsys_dev->scp_pdev, mode);
			#else
			traw_buf_info.fd = mtk_hcp_get_traw_mem_cq_fd(imgsys_dev->scp_pdev);
			#endif
			traw_buf_info.offset = user_info->priv[i].desc_offset;
			traw_buf_info.len =
				((dtbl_sz * TRAW_CQ_DESC_NUM) + TRAW_REG_SIZE);
			traw_buf_info.mode = mode;
			traw_buf_info.is_tuning = false;
            if (imgsys_traw_7sp_dbg_enable()) {
			pr_debug("imgsys_fw cq traw_buf_info (%d/%d/%d), mode(%d)",
				traw_buf_info.fd, traw_buf_info.len,
				traw_buf_info.offset, traw_buf_info.mode);
            }
			mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &traw_buf_info);
		}
	}

	for (i = IMGSYS_TRAW; i <= IMGSYS_LTR; i++) {
		if (user_info->priv[i].need_flush_tdr) {
			// tdr buffer
			#if SMVR_DECOUPLE
			traw_buf_info.fd = mtk_hcp_get_traw_mem_tdr_fd(imgsys_dev->scp_pdev, mode);
			#else
			traw_buf_info.fd = mtk_hcp_get_traw_mem_tdr_fd(imgsys_dev->scp_pdev);
			#endif
			traw_buf_info.offset = user_info->priv[i].tdr_offset;
			traw_buf_info.len = TRAW_TDR_BUF_MAXSZ;
			traw_buf_info.mode = mode;
			traw_buf_info.is_tuning = false;
            if (imgsys_traw_7sp_dbg_enable()) {
			pr_debug("imgsys_fw tdr traw_buf_info (%d/%d/%d), mode(%d)",
				traw_buf_info.fd, traw_buf_info.len,
				traw_buf_info.offset, traw_buf_info.mode);
            }
			mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &traw_buf_info);
		}
	}
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void imgsys_traw_set_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	/* iomap reg base */
	g_trawRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_TRAW);
	g_ltrawRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_LTRAW);
	g_ispMainRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_TOP);
	//imgsys_traw_reg_iommu_cb();
	/* Register IOMMU Callback */
	g_IOMMUDumpPort = 0;
}

void imgsys_traw_set_initial_value_hw(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *trawRegBA = 0L;
	void __iomem *ofset = NULL;
	unsigned int i = 0;

	trawRegBA = g_trawRegBA;
	if (!trawRegBA) {
		pr_info("%s: TRAW null reg base\n", __func__);
		return;
	}

	for (i = 0 ; i < TRAW_INIT_ARRAY_COUNT ; i++) {
		ofset = trawRegBA + mtk_imgsys_traw_init_ary[i].ofset;
		writel(mtk_imgsys_traw_init_ary[i].val, ofset);
	}
}

void imgsys_ltraw_set_initial_value_hw(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *trawRegBA = 0L;
	void __iomem *ofset = NULL;
	unsigned int i = 0;

	trawRegBA = g_ltrawRegBA;
	if (!trawRegBA) {
		pr_info("%s: LTRAW hw null reg base\n", __func__);
		return;
	}

	for (i = 0 ; i < TRAW_INIT_ARRAY_COUNT ; i++) {
		ofset = trawRegBA + mtk_imgsys_traw_init_ary[i].ofset;
		writel(mtk_imgsys_traw_init_ary[i].val, ofset);
	}
}

void imgsys_traw_cmdq_set_initial_value_hw(struct mtk_imgsys_dev *imgsys_dev,
		void *pkt, int hw_idx)
{
	unsigned int ofset;
	unsigned int i = 0;
	struct cmdq_pkt *package = NULL;

	if (imgsys_dev == NULL || pkt == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}
	package = (struct cmdq_pkt *)pkt;

	/* reset traw macro */
	cmdq_pkt_write(package, NULL,
		    	(IMG_MAIN_BASE + SW_RST) /*address*/, 0x30,
		    	0xffffffff);
	cmdq_pkt_write(package, NULL,
		    	(IMG_MAIN_BASE + SW_RST) /*address*/, 0x0,
		    	0xffffffff);

	/* module reset */
	cmdq_pkt_write(package, NULL,
		      (TRAW_TOP_BASE + SW_RST) /*address*/, 0x3C,
		       0xffffffff);
	cmdq_pkt_write(package, NULL,
		       (TRAW_TOP_BASE + SW_RST) /*address*/, 0x0,
		       0xffffffff);

	/* ori traw set */
	for (i = 0 ; i < TRAW_INIT_ARRAY_COUNT ; i++) {
		ofset = TRAW_BASE + mtk_imgsys_traw_init_ary[i].ofset;
		cmdq_pkt_write(package, NULL, ofset /*address*/,
				mtk_imgsys_traw_init_ary[i].val, 0xffffffff);
	}
}

void imgsys_ltraw_cmdq_set_initial_value_hw(struct mtk_imgsys_dev *imgsys_dev,
		void *pkt, int hw_idx)
{
	unsigned int ofset;
	unsigned int i = 0;
	struct cmdq_pkt *package = NULL;

	if (imgsys_dev == NULL || pkt == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}
	package = (struct cmdq_pkt *)pkt;

	/* reset ltraw macro */
	cmdq_pkt_write(package, NULL,
		    	(IMG_MAIN_BASE + SW_RST) /*address*/, 0xC0,
		    	0xffffffff);
	cmdq_pkt_write(package, NULL,
		    	(IMG_MAIN_BASE + SW_RST) /*address*/, 0x0,
		    	0xffffffff);

	/* ori traw set */
	for (i = 0 ; i < TRAW_INIT_ARRAY_COUNT ; i++) {
		ofset = LTRAW_BASE + mtk_imgsys_traw_init_ary[i].ofset;
		cmdq_pkt_write(package, NULL, ofset /*address*/,
				mtk_imgsys_traw_init_ary[i].val, 0xffffffff);
	}
}

void imgsys_traw_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine)
{
	void __iomem *trawRegBA = 0L;
	unsigned int i;
#if IF_0_DEFINE //YWTBD K DBG
	unsigned int Drzh2nt1DdbSel = DRZH2N_T1_DBG_SEL;
	unsigned int Drzh2nt1DbgOut = DRZH2N_T1_DBG_RDATA;
	unsigned int DMADdbSel = TRAW_DMA_DBG_SEL;
	unsigned int DMADbgOut = TRAW_DMA_DBG_PORT;
	unsigned int CtlDdbSel = TRAW_CTL_DBG_SEL;
	unsigned int CtlDbgOut = TRAW_CTL_DBG_PORT;
	unsigned int TdrDdbSel = TRAW_TDR_DBG_SEL;
	unsigned int TdrDbgOut = TRAW_TDR_DBG_PORT;
#endif
	unsigned int RegMap = REG_MAP_E_TRAW;
	char DbgStr[128];

	pr_info("%s: +\n", __func__);

	/* ltraw */
	if (engine & IMGSYS_ENG_LTR) {
		RegMap = REG_MAP_E_LTRAW;
		g_RegBaseAddr = TRAW_B_BASE_ADDR;
		trawRegBA = g_ltrawRegBA;
	}
	/* traw */
	else {
		g_RegBaseAddr = TRAW_A_BASE_ADDR;
		trawRegBA = g_trawRegBA;
	}

	if (!trawRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap regmap(%d)\n",
			__func__, RegMap);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
			__func__, imgsys_dev->dev->of_node->name);
		goto err_debug_dump;
	}
#if IF_0_DEFINE //YWTBD K DBG
	imgsys_traw_tdr_dbg(imgsys_dev, trawRegBA, TdrDdbSel, TdrDbgOut);
	/* DL debug data */
	imgsys_traw_dump_dl(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
#endif
	/* Ctrl registers */
	for (i = 0x0; i <= TRAW_CTL_ADDR_END; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(g_RegBaseAddr + i),
			(unsigned int)ioread32((void *)(trawRegBA + i)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}
	/* Dma registers */
	for (i = TRAW_DMA_ADDR_OFST; i <= TRAW_DMA_ADDR_END; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(g_RegBaseAddr + i),
			(unsigned int)ioread32((void *)(trawRegBA + i)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}
	/* Data registers */
	for (i = TRAW_MOD_ADDR_OFST; i <= TRAW_MAX_ADDR_OFST; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(g_RegBaseAddr + i),
			(unsigned int)ioread32((void *)(trawRegBA + i)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}
	if (sprintf(DbgStr, "[0x%08X] 0x%08X [0x%08X] 0x%08X\n",
			(unsigned int)0x15000000,
			(unsigned int)ioread32((void *)(g_ispMainRegBA)),
			(unsigned int)0x15001200,
			(unsigned int)ioread32((void *)(g_ispMainRegBA + 0x1200))) > 0)
			pr_info("%s\n", DbgStr);
#if IF_0_DEFINE //YWTBD K DBG
	/* Drzh2nt1 debug data */
	imgsys_traw_dump_drzh2nt1(imgsys_dev, trawRegBA, Drzh2nt1DdbSel, Drzh2nt1DbgOut);
	/* DMA debug data */
	imgsys_traw_dump_dma(imgsys_dev, trawRegBA, DMADdbSel, DMADbgOut);
	/* CQ debug data */
	imgsys_traw_dump_cq(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
	/* DRZH2N debug data */
	imgsys_traw_dump_drzh2n(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
	/* SMTO debug data */
	imgsys_traw_dump_smto(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
#endif
err_debug_dump:
	pr_info("%s: -\n", __func__);
}

bool imgsys_traw_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine)
{
	bool ret = true; //true: done
	void __iomem *trawRegBA = 0L;
	unsigned int value = 1;

	/* ltraw */
	if (engine & IMGSYS_ENG_LTR) {
		g_RegBaseAddr = TRAW_B_BASE_ADDR;
		trawRegBA = g_ltrawRegBA;
	}
	/* traw */
	else {
		g_RegBaseAddr = TRAW_A_BASE_ADDR;
		trawRegBA = g_trawRegBA;
	}


	value = (unsigned int)ioread32((void *)(trawRegBA + 0x100));
	if (!(value & 0x1)) {
		ret = false;
		pr_info(
		"%s: hw_comb:0x%x, polling done fail!!! [0x%08x] 0x%x",
		__func__, engine, (uint32_t)(g_RegBaseAddr + 0x100), value);

	}

	return ret;
}

void imgsys_traw_uninit(struct mtk_imgsys_dev *imgsys_dev)
{
	if (g_trawRegBA) {
		iounmap(g_trawRegBA);
		g_trawRegBA = 0L;
	}
	if (g_ltrawRegBA) {
		iounmap(g_ltrawRegBA);
		g_ltrawRegBA = 0L;
	}
	if (g_ispMainRegBA) {
		iounmap(g_ispMainRegBA);
		g_ispMainRegBA = 0L;
	}
}
