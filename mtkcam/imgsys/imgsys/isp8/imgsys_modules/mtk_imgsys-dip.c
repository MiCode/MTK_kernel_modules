// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *         Holmes Chiou <holmes.chiou@mediatek.com>
 *
 */

#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include "mtk_imgsys-dip.h"
#include "mtk-hcp.h"
#include "mtk_imgsys-v4l2-debug.h"

// GCE header
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

#include "../../cmdq/isp8/mtk_imgsys-cmdq-qof.h"

const struct mtk_imgsys_init_array mtk_imgsys_dip_init_ary[] = {
	{0x084, 0x00000001}, /* DIPCTL_D1A_DIPCTL_INT1_EN */
	{0x294, 0x00000001}, /* DIPCTL_QOF_CTL */
};

static struct DIPRegDumpInfo g_DIPRegDumpTopIfo[] = {
	{ 0x0000, 0x0364},
	{ 0x10B4, 0x10E4},
	{ 0x1200, 0x138C},
	{ 0x1520, 0x2ABC},
	{ 0x5000, 0x552C},
	{ 0x6000, 0x6440},
	{ 0x684C, 0x6E14},
	{ 0x8400, 0x8628},
	{ 0x9000, 0x9804},
};

static struct DIPRegDumpInfo g_DIPRegDumpNr1Ifo[] = {
	{ 0x4400, 0x4820},
	{ 0x7000, 0x70A4},
	{ 0x8340, 0x8540},
	{ 0x9000, 0x9804},
	{ 0xA000, 0xA080},
	{ 0xEF00, 0xEF08},
};

static struct DIPRegDumpInfo g_DIPRegDumpNr2Ifo[] = {
	{ 0x1200, 0x1F4C},
	{ 0x3000, 0x3BEC},
	{ 0x4000, 0x4960},
	{ 0x5000, 0x5B7C},
	{ 0x6000, 0x6060},
	{ 0x741C, 0x74DC},
	{ 0x7D00, 0x8244},
};

static struct DIPDmaDebugInfo g_DMATopDbgIfo[] = {
	{"IMGI", DIP_ORI_RDMA_UFO_DEBUG, 0x0},
	{"IMGI_N", DIP_ORI_RDMA_UFO_DEBUG, 0x1000},
	{"IMGBI", DIP_ORI_RDMA_UFO_DEBUG, 0x1},
	{"IMGBI_N", DIP_ORI_RDMA_UFO_DEBUG, 0x1001},
	{"IMGCI", DIP_ORI_RDMA_UFO_DEBUG, 0x2},
	{"IMGCI_N", DIP_ORI_RDMA_UFO_DEBUG, 0x1002},
	{"IMGDI", DIP_ORI_RDMA_UFO_DEBUG, 0x3},
	{"IMGDI_N", DIP_ORI_RDMA_UFO_DEBUG, 0x1003},
	{"TNRSI", DIP_ULC_RDMA_DEBUG, 0xB},
	{"TNRMI", DIP_ULC_RDMA_DEBUG, 0x400B},
	{"TNRWI", DIP_ULC_RDMA_DEBUG, 0xC},
	{"TNRLYI", DIP_ULC_RDMA_DEBUG, 0xD},
	{"TNRLCI", DIP_ULC_RDMA_DEBUG, 0xE},
	{"TNRVBI", DIP_ULC_RDMA_DEBUG, 0xF},
	{"SNRGMI", DIP_ULC_RDMA_DEBUG, 0x10},
	{"TNRCI", DIP_ULC_RDMA_DEBUG, 0x11},
	{"TNRCI_N", DIP_ULC_RDMA_DEBUG, 0x1011},
	{"TNRFGMI", DIP_ULC_RDMA_DEBUG, 0x12},
	{"TNRFGMI_N", DIP_ULC_RDMA_DEBUG, 0x1012},
	{"TNRAIMI", DIP_ULC_RDMA_DEBUG, 0x13},
	{"TNRAIMI_N", DIP_ULC_RDMA_DEBUG, 0x1013},
	{"RECI1", DIP_ULC_RDMA_DEBUG, 0x14},
	{"RECI1_N", DIP_ULC_RDMA_DEBUG, 0x1014},
	{"RECBI1", DIP_ULC_RDMA_DEBUG, 0x15},
	{"RECI2", DIP_ULC_RDMA_DEBUG, 0x16},
	{"RECI2_N", DIP_ULC_RDMA_DEBUG, 0x1016},
	{"RECBI2", DIP_ULC_RDMA_DEBUG, 0x17},
	{"RECI3", DIP_ULC_RDMA_DEBUG, 0x18},
	{"RECBI3", DIP_ULC_RDMA_DEBUG, 0x19},
	{"IMG4O", DIP_ORI_WDMA_DEBUG, 0x1C},
	{"IMG4O_N", DIP_ORI_WDMA_DEBUG, 0x101C},
	{"IMGBO", DIP_ORI_WDMA_DEBUG, 0x1D},
	{"IMGCO", DIP_ORI_WDMA_DEBUG, 0x1E},
	{"IMGCO_N", DIP_ORI_WDMA_DEBUG, 0x101E},
	{"IMGDO", DIP_ORI_WDMA_DEBUG, 0x1F},
	{"TNRMO", DIP_ULC_WDMA_DEBUG, 0x27},
	{"TNRMO_N", DIP_ULC_WDMA_DEBUG, 0x1027},
	{"TNRSO", DIP_ULC_WDMA_DEBUG, 0x4027},
	{"TNRWO", DIP_ULC_WDMA_DEBUG, 0x28},
	{"FHO1", DIP_ULC_WDMA_DEBUG, 0x4029},
};

static struct DIPDmaDebugInfo g_DMANrDbgIfo[] = {
	{"VIPI", DIP_ULC_RDMA_DEBUG, 0x0},
	{"VIPI_N", DIP_ULC_RDMA_DEBUG, 0x1000},
	{"VIPBI", DIP_ULC_RDMA_DEBUG, 0x1},
	{"VIPBI_N", DIP_ULC_RDMA_DEBUG, 0x1001},
	{"VIPCI", DIP_ULC_RDMA_DEBUG, 0x2},
	{"VIPCI_N", DIP_ULC_RDMA_DEBUG, 0x1002},
	{"SNRCSI", DIP_ULC_RDMA_DEBUG, 0xD},
	{"SNRCSI_N", DIP_ULC_RDMA_DEBUG, 0x100D},
	{"SNRAIMI", DIP_ULC_RDMA_DEBUG, 0xE},
	{"SNRAIMI_N", DIP_ULC_RDMA_DEBUG, 0x100E},
	{"SNRGMI", DIP_ULC_RDMA_DEBUG, 0xF},
	{"EECSI", DIP_ULC_RDMA_DEBUG, 0x10},
	{"CSMCSI", DIP_ULC_RDMA_DEBUG, 0x11},
	{"CSMCSI_N", DIP_ULC_RDMA_DEBUG, 0x1011},
	{"CSMCSTI", DIP_ULC_RDMA_DEBUG, 0x4011},
	{"CSMCI", DIP_ULC_RDMA_DEBUG, 0x12},
	{"BOKMI", DIP_ULC_RDMA_DEBUG, 0x16},
	{"BOKMI_N", DIP_ULC_RDMA_DEBUG, 0x1016},
	{"BOKPYI", DIP_ULC_RDMA_DEBUG, 0x17},
	{"BOKPYI_N", DIP_ULC_RDMA_DEBUG, 0x1017},
	{"BOKPCI", DIP_ULC_RDMA_DEBUG, 0x18},
	{"DMGI", DIP_ULC_RDMA_DEBUG, 0x19},
	{"DMGI_N", DIP_ULC_RDMA_DEBUG, 0x1019},
	{"DEPI", DIP_ULC_RDMA_DEBUG, 0x1A},
	{"IMG3O", DIP_ORI_WDMA_DEBUG, 0x1F},
	{"IMG3O_N", DIP_ORI_WDMA_DEBUG, 0x101F},
	{"IMG3BO", DIP_ORI_WDMA_DEBUG, 0x20},
	{"IMG3BO_N", DIP_ORI_WDMA_DEBUG, 0x1020},
	{"IMG3CO", DIP_ORI_WDMA_DEBUG, 0x21},
	{"IMG3CO_N", DIP_ORI_WDMA_DEBUG, 0x1021},
	{"IMG3DO", DIP_ORI_WDMA_DEBUG, 0x22},
	{"IMG3DO_N", DIP_ORI_WDMA_DEBUG, 0x1022},
	{"IMG7O", DIP_ORI_WDMA_DEBUG, 0x23},
	{"IMG7O_N", DIP_ORI_WDMA_DEBUG, 0x1023},
	{"IMG7BO", DIP_ORI_WDMA_DEBUG, 0x24},
	{"IMG7CO", DIP_ORI_WDMA_DEBUG, 0x25},
	{"IMG7CO_N", DIP_ORI_WDMA_DEBUG, 0x1025},
	{"IMG7DO", DIP_ORI_WDMA_DEBUG, 0x26},
	{"IMG5O", DIP_ORI_WDMA_DEBUG, 0x29},
	{"IMG5BO", DIP_ORI_WDMA_DEBUG, 0x2A},
	{"IMG6O", DIP_ORI_WDMA_DEBUG, 0x2B},
	{"IMG6BO", DIP_ORI_WDMA_DEBUG, 0x2C},
	{"SNRACTO", DIP_ULC_WDMA_DEBUG, 0x37},
	{"SNRACTO_N", DIP_ULC_WDMA_DEBUG, 0x1037},
	{"CNRO", DIP_ULC_WDMA_DEBUG, 0x3A},
	{"CNRO_N", DIP_ULC_WDMA_DEBUG, 0x103A},
	{"CNRBO", DIP_ULC_WDMA_DEBUG, 0x3B},
	{"BOKMO", DIP_ULC_WDMA_DEBUG, 0x3C},
	{"BOKMO_N", DIP_ULC_WDMA_DEBUG, 0x103C},
	{"FEO", DIP_ULC_WDMA_DEBUG, 0x3D},
	{"FHO2", DIP_ULC_WDMA_DEBUG, 0x403E},
	{"CSMCSO", DIP_ULC_WDMA_DEBUG, 0x403F},
	{"FHO3", DIP_ULC_WDMA_DEBUG, 0x4040},

};

struct mtk_imgsys_dip_dtable {
	uint32_t empty;
	uint32_t addr;
	uint32_t addr_msb;
};

#define DIP_HW_SET 3
#define SW_RST   (0x000C)

static void __iomem *gdipRegBA[DIP_HW_SET] = {0L};
static unsigned int g_RegBaseAddr = DIP_TOP_ADDR;
static unsigned int g_RegBaseAddrTop = DIP_TOP_ADDR;
static unsigned int g_RegBaseAddrNr1 = DIP_NR1_ADDR;
static unsigned int g_RegBaseAddrNr2 = DIP_NR2_ADDR;

int imgsys_dip_tfault_callback(int port,
	dma_addr_t mva, void *data)
{
	void __iomem *dipRegBA = 0L;
	unsigned larb = 0;
	unsigned int i, j, k;
	int ret = 0;
	bool is_qof = false;

	pr_debug("%s: +\n", __func__);

	ret = smi_isp_dip_get_if_in_use((void *)&is_qof);
	if (ret == -1) {
		pr_info("smi_isp_dip_get_if_in_use = -1.stop dump. return\n");
		return 1;
	}

	larb = ((port>>5) & 0x3F);
	pr_info("%s: iommu port:0x%x, larb:%d, idx:%d, addr:0x%08lx\n", __func__,
		port, larb, (port & 0x1F), (unsigned long)mva);

	/* 0x15100000~ */
	dipRegBA = gdipRegBA[0];

	/* top reg */
	for (j = 0; j < sizeof(g_DIPRegDumpTopIfo)/sizeof(struct DIPRegDumpInfo); j++) {
        k = g_DIPRegDumpTopIfo[j].oft & 0xFFF0;
		for (i = k; i <= g_DIPRegDumpTopIfo[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(g_RegBaseAddrTop + i),
				(unsigned int)ioread32((void *)(dipRegBA + i)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}
	}

	/* 0x15150000~ */
	dipRegBA = gdipRegBA[1];

	/* nr1 reg */
	for (j = 0; j < sizeof(g_DIPRegDumpNr1Ifo)/sizeof(struct DIPRegDumpInfo); j++) {
        k = g_DIPRegDumpNr1Ifo[j].oft & 0xFFF0;
		for (i = k; i <= g_DIPRegDumpNr1Ifo[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(g_RegBaseAddrNr1 + i),
				(unsigned int)ioread32((void *)(dipRegBA + i)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}
	}

	/* 0x15160000~ */
	dipRegBA = gdipRegBA[2];

	/* nr2 reg */
	for (j = 0; j < sizeof(g_DIPRegDumpNr2Ifo)/sizeof(struct DIPRegDumpInfo); j++) {
        k = g_DIPRegDumpNr2Ifo[j].oft & 0xFFF0;
		for (i = k; i <= g_DIPRegDumpNr2Ifo[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(g_RegBaseAddrNr2 + i),
				(unsigned int)ioread32((void *)(dipRegBA + i)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}
	}

	smi_isp_dip_put((void *)&is_qof);
	pr_info("%s: -\n", __func__);
	return 1;
}

void imgsys_dip_set_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned int hw_idx = 0, ary_idx = 0;

	for (hw_idx = REG_MAP_E_DIP; hw_idx <= REG_MAP_E_DIP_NR2; hw_idx++) {
		/* iomap registers */
		ary_idx = hw_idx - REG_MAP_E_DIP;
		gdipRegBA[ary_idx] = of_iomap(imgsys_dev->dev->of_node, hw_idx);
		if (!gdipRegBA[ary_idx]) {
			pr_info("%s:unable to iomap dip_%d reg, devnode()\n",
				__func__, hw_idx);
			continue;
		}
	}

	if (imgsys_dev->dev_ver == 1) {
		g_RegBaseAddrTop = DIP_TOP_ADDR_P;
		g_RegBaseAddrNr1 = DIP_NR1_ADDR_P;
		g_RegBaseAddrNr2 = DIP_NR2_ADDR_P;
	}

}

void imgsys_dip_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *dipRegBA = 0L;
	void __iomem *ofset = NULL;
	unsigned int i;

	/* iomap registers */
	dipRegBA = gdipRegBA[0];

	for (i = 0; i < ARRAY_SIZE(mtk_imgsys_dip_init_ary); i++) {
		ofset = dipRegBA + mtk_imgsys_dip_init_ary[i].ofset;
		writel(mtk_imgsys_dip_init_ary[i].val, ofset);
	}

}

void imgsys_dip_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode)
{
	u64 iova_addr = tuning_iova;
	u64 *cq_desc = NULL;
	struct mtk_imgsys_dip_dtable *dtable = NULL;
	unsigned int i = 0, tun_ofst = 0;
	struct flush_buf_info dip_buf_info;


	/* HWID defined in hw_definition.h */
	if (user_info->priv[IMGSYS_DIP].need_update_desc) {
		if (iova_addr) {
			#if SMVR_DECOUPLE
			cq_desc = (u64 *)((void *)(mtk_hcp_get_dip_mem_virt(imgsys_dev->scp_pdev, mode) +
					user_info->priv[IMGSYS_DIP].desc_offset));
			#else
			cq_desc = (u64 *)((void *)(mtk_hcp_get_dip_mem_virt(imgsys_dev->scp_pdev) +
					user_info->priv[IMGSYS_DIP].desc_offset));
			#endif
			for (i = 0; i < DIP_CQ_DESC_NUM; i++) {
				dtable = (struct mtk_imgsys_dip_dtable *)cq_desc + i;
				if ((dtable->addr_msb & PSEUDO_DESC_TUNING) == PSEUDO_DESC_TUNING) {
					tun_ofst = dtable->addr;
					dtable->addr = (tun_ofst + iova_addr) & 0xFFFFFFFF;
					dtable->addr_msb = ((tun_ofst + iova_addr) >> 32) & 0xF;
                    if (imgsys_dip_7sp_dbg_enable()) {
					pr_debug(
						"%s: tuning_buf_iova(0x%llx) des_ofst(0x%08x) cq_kva(0x%p) dtable(0x%x/0x%x/0x%x)\n",
						__func__, iova_addr,
						user_info->priv[IMGSYS_DIP].desc_offset,
						cq_desc, dtable->empty, dtable->addr,
						dtable->addr_msb);
				    }
			    }
		    }
		}
		//
		dip_buf_info.fd = mtk_hcp_get_dip_mem_cq_fd(imgsys_dev->scp_pdev, mode);
		dip_buf_info.offset = user_info->priv[IMGSYS_DIP].desc_offset;
		dip_buf_info.len =
			(sizeof(struct mtk_imgsys_dip_dtable) * DIP_CQ_DESC_NUM) + DIP_REG_SIZE;
		dip_buf_info.mode = mode;
		dip_buf_info.is_tuning = false;
        if (imgsys_dip_7sp_dbg_enable()) {
		pr_debug("imgsys_fw cq dip_buf_info (%d/%d/%d), mode(%d)",
			dip_buf_info.fd, dip_buf_info.len,
			dip_buf_info.offset, dip_buf_info.mode);
        }
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &dip_buf_info);
	}

	if (user_info->priv[IMGSYS_DIP].need_flush_tdr) {
		// tdr buffer
		dip_buf_info.fd = mtk_hcp_get_dip_mem_tdr_fd(imgsys_dev->scp_pdev, mode);
		dip_buf_info.offset = user_info->priv[IMGSYS_DIP].tdr_offset;
		dip_buf_info.len = DIP_TDR_BUF_MAXSZ;
		dip_buf_info.mode = mode;
		dip_buf_info.is_tuning = false;
        if (imgsys_dip_7sp_dbg_enable()) {
		pr_debug("imgsys_fw tdr dip_buf_info (%d/%d/%d), mode(%d)",
			dip_buf_info.fd, dip_buf_info.len,
			dip_buf_info.offset, dip_buf_info.mode);
        }
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &dip_buf_info);
    }
}

void imgsys_dip_cmdq_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev, void *pkt, int hw_idx)
{
	unsigned int ofset;
	unsigned int i;
	unsigned int imgsys_dip_base = IMGSYS_DIP_BASE;
	unsigned int dip_base = DIP_TOP_ADDR;
	struct cmdq_pkt *package = NULL;

	if (imgsys_dev == NULL || pkt == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}
	package = (struct cmdq_pkt *)pkt;

	if (imgsys_dev->dev_ver == 1) {
		imgsys_dip_base = IMGSYS_DIP_BASE_P;
		dip_base = DIP_TOP_ADDR_P;
	}

	cmdq_pkt_write(package, NULL, (imgsys_dip_base + SW_RST) /*address*/,
			       0x3FC03, 0xffffffff);
	cmdq_pkt_write(package, NULL, (imgsys_dip_base + SW_RST) /*address*/,
			       0x0, 0xffffffff);

	/* iomap registers */
	for (i = 0; i < ARRAY_SIZE(mtk_imgsys_dip_init_ary); i++) {
		ofset = dip_base + mtk_imgsys_dip_init_ary[i].ofset;
		cmdq_pkt_write(package, NULL, ofset /*address*/,
				mtk_imgsys_dip_init_ary[i].val, 0xffffffff);
	}
}

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

static void ExeDbgCmdNr3d(struct mtk_imgsys_dev *a_pDev,
			void __iomem *a_pRegBA,
			unsigned int a_DdbSel,
			unsigned int a_DbgCnt,
			unsigned int a_DbgSt,
			unsigned int a_DbgCmd)
{
	unsigned int DbgCntData = 0, DbgStData = 0;
	unsigned int DbgSelReg = g_RegBaseAddr + a_DdbSel;
	void __iomem *pDbgSel = (void *)(a_pRegBA + a_DdbSel);
	void __iomem *pDbgCnt = (void *)(a_pRegBA + a_DbgCnt);
	void __iomem *pDbgSt = (void *)(a_pRegBA + a_DbgSt);

	iowrite32(a_DbgCmd, pDbgSel);
	DbgCntData = (unsigned int)ioread32(pDbgCnt);
	DbgStData = (unsigned int)ioread32(pDbgSt);
	pr_info("[0x%08X](0x%08X,0x%08X,0x%08X)\n",
		a_DbgCmd, DbgSelReg, DbgCntData, DbgStData);
}

static void imgsys_dip_dump_dma(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut,
				char a_DMANrPort)
{
	unsigned int Idx = 0, DMAIdx = 0;
	unsigned int DbgCmd = 0;
	unsigned int DmaDegInfoSize = sizeof(struct DIPDmaDebugInfo);
	unsigned int DebugCnt = sizeof(g_DMATopDbgIfo)/DmaDegInfoSize;
	enum DIPDmaDebugType DbgTy = DIP_ORI_RDMA_DEBUG;

	/* DMA NR */
	if (a_DMANrPort == 1) {
		DebugCnt = sizeof(g_DMANrDbgIfo)/DmaDegInfoSize;
		pr_info("DIP_NR dump DMA port\n");
	} else {
		pr_info("DIP_TOP dump DMA port\n");
	}

	/* Dump DMA Debug Info */
	for (Idx = 0; Idx < DebugCnt; Idx++) {
		if (a_DMANrPort == 1) {
			DbgTy = g_DMANrDbgIfo[Idx].DMADebugType;
			DMAIdx = g_DMANrDbgIfo[Idx].DMAIdx;
		} else {
			DbgTy = g_DMATopDbgIfo[Idx].DMADebugType;
			DMAIdx = g_DMATopDbgIfo[Idx].DMAIdx;
		}

		/* state_checksum */
		DbgCmd = DIP_IMGI_STATE_CHECKSUM + DMAIdx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		/* line_pix_cnt_tmp */
		DbgCmd = DIP_IMGI_LINE_PIX_CNT_TMP + DMAIdx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		/* line_pix_cnt */
		DbgCmd = DIP_IMGI_LINE_PIX_CNT + DMAIdx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

		/* important_status */
		if (DbgTy == DIP_ULC_RDMA_DEBUG ||
			DbgTy == DIP_ULC_WDMA_DEBUG) {
			DbgCmd = DIP_IMGI_IMPORTANT_STATUS + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
			DbgCmd);
		}

		/* smi_debug_data (case 0) or cmd_data_cnt */
		if (DbgTy == DIP_ORI_RDMA_DEBUG ||
			DbgTy == DIP_ORI_RDMA_UFO_DEBUG ||
			DbgTy == DIP_ULC_RDMA_DEBUG ||
			DbgTy == DIP_ULC_WDMA_DEBUG) {
			DbgCmd = DIP_IMGI_SMI_DEBUG_DATA_CASE0 + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
			DbgCmd);
		}

		/* ULC_RDMA or ULC_WDMA */
		if (DbgTy == DIP_ULC_RDMA_DEBUG ||
			DbgTy == DIP_ULC_WDMA_DEBUG) {
			DbgCmd = DIP_IMGI_TILEX_BYTE_CNT + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = DIP_IMGI_TILEY_CNT + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* smi_dbg_data(case 0) or burst_line_cnt or input_v_cnt */
		if (DbgTy == DIP_ORI_WDMA_DEBUG ||
			DbgTy == DIP_ULC_RDMA_DEBUG ||
			DbgTy == DIP_ULC_WDMA_DEBUG) {
			DbgCmd = DIP_IMGI_BURST_LINE_CNT + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* xfer_y_cnt */
		if (DbgTy == DIP_ULC_WDMA_DEBUG) {
			DbgCmd = DIP_IMGI_XFER_Y_CNT + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* ORI_RDMA */
		if (DbgTy == DIP_ORI_RDMA_DEBUG ||
			DbgTy == DIP_ORI_RDMA_UFO_DEBUG) {
			DbgCmd = DIP_IMGI_FIFO_DEBUG_DATA_CASE1 + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = DIP_IMGI_FIFO_DEBUG_DATA_CASE3 + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* ORI_RDMA_UFO or ULC_WDMA */
		if (DbgTy == DIP_ORI_RDMA_UFO_DEBUG ||
			DbgTy == DIP_ULC_WDMA_DEBUG) {
			DbgCmd = DIP_IMGI_UFO_STATE_CHECKSUM + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = DIP_IMGI_UFO_LINE_PIX_CNT_TMP + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = DIP_IMGI_UFO_LINE_PIX_CNT + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* ORI_WDMA */
		if (DbgTy == DIP_ORI_WDMA_DEBUG) {
			DbgCmd = DIP_YUVO_T1_FIFO_DEBUG_DATA_CASE1 + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = DIP_YUVO_T1_FIFO_DEBUG_DATA_CASE3 + DMAIdx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

	}
}

static void imgsys_dip_dump_nr3d(struct mtk_imgsys_dev *a_pDev,
				 void __iomem *a_pRegBA)
{
	unsigned int DdbSel = DIP_NR3D_DBG_SEL;
	unsigned int DbgCnt = DIP_NR3D_DBG_CNT;
	unsigned int DbgSt = DIP_NR3D_DBG_ST;
	unsigned int Idx = 0;

	pr_info("dump nr3d debug\n");

	/* Dump NR3D Debug Info */
	for (Idx = 0; Idx < DIP_NR3D_DBG_POINTS; Idx++)
		ExeDbgCmdNr3d(a_pDev, a_pRegBA, DdbSel, DbgCnt, DbgSt, Idx);

}

static void imgsys_dip_dump_dl(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int DbgLineCnt = 0, DbgRdy = 0, DbgReq = 0;
	unsigned int DbgLineCntReg = 0;

	pr_info("dump dl debug\n");

	/* wpe_wif_d1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000007;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_d1_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000008;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[wpe_wif_d1_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000009;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[wpe_wif_d1_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_d2_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000107;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_d2_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000108;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[wpe_wif_d2_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000109;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[wpe_wif_d2_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* wpe_wif_d3_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000207;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_d3_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000208;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[wpe_wif_d3_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000209;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[wpe_wif_d3_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* mcrp_d1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000307;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[mcrp_d1_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000308;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[mcrp_d1_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000309;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) >> 16;
	pr_info("[mcrp_d1_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

}

static void imgsys_dip_dump_snr(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int Idx = 0;
	unsigned int CmdOft = 0x10000;

	pr_info("dump snr debug\n");

	/* snr_d1 debug */
	DbgCmd = 0x8501;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}
	DbgCmd = 0x8601;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}

}

static void imgsys_dip_dump_eecnr(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int Idx = 0;
	unsigned int CmdOft = 0x10000;

	pr_info("dump eecnr debug\n");

	/* eecnr debug */
	DbgCmd = 0xC201;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}

}

static void imgsys_dip_dump_ans(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int Idx = 0;
	unsigned int CmdOft = 0x10000;

	pr_info("dump ans debug\n");

	/* ans debug */
	DbgCmd = 0xC801;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}

}

static void imgsys_dip_dump_bok(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int Idx = 0;
	unsigned int CmdOft = 0x10000;

	pr_info("dump bok debug\n");

	/* snr_d1 debug */
	DbgCmd = 0xF301;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}
	DbgCmd = 0xF401;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}

}

static void imgsys_dip_dump_yufdd1(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int Idx = 0;
	unsigned int CmdOft = 0x10000;
	unsigned int yufdCmdOft = 0x1;

	pr_info("dump yufd_d1 debug\n");

	/* yufd_d1 debug */
	DbgCmd = 0x0401;
	for (Idx = 0; Idx < 0x10; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += CmdOft;
	}
	/* yufd_d1_dbg_sel debug */
	a_DdbSel = DIP_YUFD_DBG_SEL;
	DbgCmd = 0x0;
	for (Idx = 0; Idx < 0x20; Idx++) {
		DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		DbgCmd += yufdCmdOft;
	}

}

void imgsys_dip_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine)
{
	void __iomem *dipRegBA = 0L;
	unsigned int i, j, k;
	unsigned int DMADdbSel = DIP_DMATOP_DBG_SEL;
	unsigned int DMADbgOut = DIP_DMATOP_DBG_PORT;
	unsigned int CtlDdbSel = DIP_DBG_SEL;
	unsigned int CtlDbgOut = DIP_DBG_OUT;
	char DMANrPort = 0;

	pr_info("%s: +\n", __func__);

	/* 0x15100000~ */
	dipRegBA = gdipRegBA[0];

	/* DL debug data */
	imgsys_dip_dump_dl(imgsys_dev, dipRegBA, CtlDdbSel, CtlDbgOut);

	/* top reg */
	for (j = 0; j < sizeof(g_DIPRegDumpTopIfo)/sizeof(struct DIPRegDumpInfo); j++) {
        k = g_DIPRegDumpTopIfo[j].oft & 0xFFF0;
		for (i = k; i <= g_DIPRegDumpTopIfo[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(g_RegBaseAddrTop + i),
				(unsigned int)ioread32((void *)(dipRegBA + i)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}
	}

	/* 0x15150000~ */
	dipRegBA = gdipRegBA[1];

	/* nr1 reg */
	for (j = 0; j < sizeof(g_DIPRegDumpNr1Ifo)/sizeof(struct DIPRegDumpInfo); j++) {
        k = g_DIPRegDumpNr1Ifo[j].oft & 0xFFF0;
		for (i = k; i <= g_DIPRegDumpNr1Ifo[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(g_RegBaseAddrNr1 + i),
				(unsigned int)ioread32((void *)(dipRegBA + i)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}
	}

	/* 0x15160000~ */
	dipRegBA = gdipRegBA[2];

	/* nr2 reg */
	for (j = 0; j < sizeof(g_DIPRegDumpNr2Ifo)/sizeof(struct DIPRegDumpInfo); j++) {
        k = g_DIPRegDumpNr2Ifo[j].oft & 0xFFF0;
		for (i = k; i <= g_DIPRegDumpNr2Ifo[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(g_RegBaseAddrNr2 + i),
				(unsigned int)ioread32((void *)(dipRegBA + i)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}
	}

	/* DMA_TOP debug data */
	DMANrPort = 0;
	dipRegBA = gdipRegBA[0];
	DMADdbSel = DIP_DMATOP_DBG_SEL;
	DMADbgOut = DIP_DMATOP_DBG_PORT;
	g_RegBaseAddr = g_RegBaseAddrTop;
	imgsys_dip_dump_dma(imgsys_dev, dipRegBA, DMADdbSel, DMADbgOut, DMANrPort);
	/* DMA_NR debug data */
	DMANrPort = 1;
	dipRegBA = gdipRegBA[2];
	DMADdbSel = DIP_DMANR2_DBG_SEL;
	DMADbgOut = DIP_DMANR2_DBG_PORT;
	g_RegBaseAddr = g_RegBaseAddrNr2;
	imgsys_dip_dump_dma(imgsys_dev, dipRegBA, DMADdbSel, DMADbgOut, DMANrPort);

	dipRegBA = gdipRegBA[0];
	g_RegBaseAddr = g_RegBaseAddrTop;
	/* NR3D debug data */
	imgsys_dip_dump_nr3d(imgsys_dev, dipRegBA);
	/* SNR debug data */
	imgsys_dip_dump_snr(imgsys_dev, dipRegBA, CtlDdbSel, CtlDbgOut);
	/* EECNR debug data */
	imgsys_dip_dump_eecnr(imgsys_dev, dipRegBA, CtlDdbSel, CtlDbgOut);
	/* ANS debug data */
	imgsys_dip_dump_ans(imgsys_dev, dipRegBA, CtlDdbSel, CtlDbgOut);
	/* BOK debug data */
	imgsys_dip_dump_bok(imgsys_dev, dipRegBA, CtlDdbSel, CtlDbgOut);
	/* YUFD_D1 debug data */
	imgsys_dip_dump_yufdd1(imgsys_dev, dipRegBA, CtlDdbSel, CtlDbgOut);

	pr_info("%s: -\n", __func__);

}

void imgsys_dip_uninit(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned int i;

	for (i = 0; i < DIP_HW_SET; i++) {
		iounmap(gdipRegBA[i]);
		gdipRegBA[i] = 0L;
	}

}

bool imgsys_dip_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine)
{
	bool ret = true; //true: done
	uint32_t value = 0;
	void __iomem *dipRegBA = 0L;

	dipRegBA = gdipRegBA[0];
	value = (uint32_t)ioread32((void *)(dipRegBA + 0xC8));

	if (!(value & 0x1)) {
		ret = false;
		pr_info(
		"%s: hw_comb:0x%x, polling DIP done fail!!! [0x%08x] 0x%x",
		__func__, engine,
		(uint32_t)(DIP_TOP_ADDR + 0xC8), value);
	}

	return ret;
}
