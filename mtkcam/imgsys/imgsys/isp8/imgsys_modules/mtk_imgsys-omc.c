// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Floria Huang <floria.huang@mediatek.com>
 *
 */

#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>

// GCE header
#include <linux/soc/mediatek/mtk-cmdq-ext.h>

#define M4U_PORT_DUMMY_TNR  (0)
#define M4U_PORT_DUMMY_LITE  (1)

#include "mtk_imgsys-omc.h"
#include "mtk-hcp.h"
#include "mtk_imgsys-v4l2-debug.h"
#include "../../cmdq/isp8/mtk_imgsys-cmdq-qof.h"

#define OMC_HW_NUM        (2)
void __iomem *gOmcRegBA[OMC_HW_NUM] = {0L}; //mapped physical addr
unsigned int gOmcRegBase[OMC_HW_NUM] = {0x34540000, 0x34640000};
unsigned int gOmcRegBase_P[OMC_HW_NUM] = {0x15540000, 0x15640000};

//CTL_MOD_EN //TODO:
#define DIP_DL    0x80000
#define TRAW_DL   0x100000

// for CQ_THR*_CTL
#define CQ_THRX_CTL_EN (1L << 0)
#define CQ_THRX_CTL_MODE (1L << 4)//immediately mode
#define CQ_THRX_CTL	(CQ_THRX_CTL_EN | CQ_THRX_CTL_MODE)

// register ofst //TODO:
#define OMC_REG_DBG_SET     (0x3C)
#define OMC_REG_DBG_PORT    (0x40)
#define OMC_REG_DMA_DBG_SET     (0xE68)
#define OMC_REG_DMA_DBG_PORT    (0xE70)
#define OMC_REG_CQ_THR0_CTL (0xC08)
#define OMC_REG_CQ_THR1_CTL (0xC18)
#define OMC_REG_DEC_CTL1    (0xFC0)
#define SW_RST              (0x000C)

const struct mtk_imgsys_init_array
			mtk_imgsys_omc_init_ary[] = {
	// TODO:
	{0x0014, 0x80000000}, /* OMC_TOP_CTL_INT_EN, en w-clr */
	{0x001C, 0xFFFFFFFF}, /* OMC_TOP_CTL_INT_STATUSX, w-clr */
	{0x00C8, 0x80000000}, /* OMC_TOP_CQ_IRQ_EN, en w-clr */
	{0x00D8, 0xFFFFFFFF}, /* OMC_TOP_CQ_IRQ_STX, w-clr */
	{0x00E0, 0xFFFFFFFF}, /* OMC_TOP_CQ_IRQ_STX2, w-clr */
	{0x00E8, 0xFFFFFFFF}, /* OMC_TOP_CQ_IRQ_STX3, w-clr */
	{0x0118, 0x080A0A82}, /* OMC_TOP_STG_CTL */
	{0x011C, 0x00010010}, /* OMC_TOP_STG_CTL_RANGE_UD */
	{0x0124, 0x000C0002}, /* OMC_TOP_STG_CTL_RANGE_LR */
	{0x0660, 0x10000040}, /* VECI_CON, fifo size 0x40 */
	{0x0664, 0x10400040}, /* VECI_CON2, set pre-ultra */
	{0x0668, 0x00400040}, /* VECI_CON3, disable ultra */
	{0x06E0, 0x10000040}, /* VEC2I_CON, fifo size 0x40 */
	{0x06E4, 0x10400040}, /* VEC2I_CON2, set pre-ultra */
	{0x06E8, 0x00400040}, /* VEC2I_CON3, disable ultra */
	{0x0760, 0x10000040}, /* VEC3I_CON, fifo size 0x40 */
	{0x0764, 0x10400040}, /* VEC3I_CON2, set pre-ultra */
	{0x0768, 0x00400040}, /* VEC3I_CON3, disable ultra */
	{0x0B04, 0x00000002}, /* OMC_CACHE_RWCTL_CTL */
	{0x0B48, 0x00400040}, /* OMC_CACHE_CACHI_CON2_0 */
	{0x0B4C, 0x00400040}, /* OMC_CACHE_CACHI_CON3_0 */
	{0x07E0, 0x10000040}, /* OMCO_CON, fifo size 0x40 */
	{0x07E4, 0x10400040}, /* OMCO_CON2, set pre-ultra */
	{0x07E8, 0x00400040}, /* OMCO_CON3, disable ultra */
	{0x08A0, 0x10000040}, /* OMCO2_CON, fifo size 0x40 */
	{0x08A4, 0x10400040}, /* OMCO2_CON2, set pre-ultra */
	{0x08A8, 0x00400040}, /* OMCO2_CON3, disable ultra */
	{0x0960, 0x10000040}, /* MSKO_CON, fifo size 0x40 */
	{0x0964, 0x10400040}, /* MSKO_CON2, set pre-ultra */
	{0x0968, 0x00400040}, /* MSKO_CON3, disable ultra */
	{0x0A80, 0x00000000}, /* OMC_STG_EN_CTRL */
	{0x0E60, 0x80000000}, /* OMC_DMA_DMA_ERR_CTRL */
	{0x0C08, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR0_CTL */
	{0x0C18, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR1_CTL */
	{0x0C28, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR2_CTL */
	{0x0C38, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR3_CTL */
	{0x0C48, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR4_CTL */
	{0x0C58, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR5_CTL */
	{0x0C68, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR6_CTL */
	{0x0C78, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR7_CTL */
	{0x0C88, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR8_CTL */
	{0x0C98, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR9_CTL */
	{0x0CA8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR10_CTL */
	{0x0CB8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR11_CTL */
	{0x0CC8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR12_CTL */
	{0x0CD8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR13_CTL */
	{0x0CE8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR14_CTL */
};
#define OMC_INIT_ARRAY_COUNT  ARRAY_SIZE(mtk_imgsys_omc_init_ary)


const struct mtk_imgsys_init_array
			mtk_imgsys_omc_init_ary_2p[] = {
	// TODO:
	{0x0014, 0x80000000}, /* OMC_TOP_CTL_INT_EN, en w-clr */
	{0x001C, 0xFFFFFFFF}, /* OMC_TOP_CTL_INT_STATUSX, w-clr */
	{0x00C8, 0x80000000}, /* OMC_TOP_CQ_IRQ_EN, en w-clr */
	{0x00D8, 0xFFFFFFFF}, /* OMC_TOP_CQ_IRQ_STX, w-clr */
	{0x00E0, 0xFFFFFFFF}, /* OMC_TOP_CQ_IRQ_STX2, w-clr */
	{0x00E8, 0xFFFFFFFF}, /* OMC_TOP_CQ_IRQ_STX3, w-clr */
	{0x0118, 0x080A0A82}, /* OMC_TOP_STG_CTL */
	{0x011C, 0x00010010}, /* OMC_TOP_STG_CTL_RANGE_UD */
	{0x0124, 0x000C0002}, /* OMC_TOP_STG_CTL_RANGE_LR */
	{0x0660, 0x10000040}, /* VECI_CON, fifo size 0x40 */
	{0x0664, 0x10400040}, /* VECI_CON2, set pre-ultra */
	{0x0668, 0x00400040}, /* VECI_CON3, disable ultra */
	{0x06E0, 0x10000040}, /* VEC2I_CON, fifo size 0x40 */
	{0x06E4, 0x10400040}, /* VEC2I_CON2, set pre-ultra */
	{0x06E8, 0x00400040}, /* VEC2I_CON3, disable ultra */
	{0x0760, 0x10000040}, /* VEC3I_CON, fifo size 0x40 */
	{0x0764, 0x10400040}, /* VEC3I_CON2, set pre-ultra */
	{0x0768, 0x00400040}, /* VEC3I_CON3, disable ultra */
	{0x0B04, 0x00000002}, /* OMC_CACHE_RWCTL_CTL */
	{0x0B48, 0x00400040}, /* OMC_CACHE_CACHI_CON2_0 */
	{0x0B4C, 0x00400040}, /* OMC_CACHE_CACHI_CON3_0 */
	{0x07E0, 0x10000040}, /* OMCO_CON, fifo size 0x40 */
	{0x07E4, 0x10400040}, /* OMCO_CON2, set pre-ultra */
	{0x07E8, 0x00400040}, /* OMCO_CON3, disable ultra */
	{0x08A0, 0x10000040}, /* OMCO2_CON, fifo size 0x40 */
	{0x08A4, 0x10400040}, /* OMCO2_CON2, set pre-ultra */
	{0x08A8, 0x00400040}, /* OMCO2_CON3, disable ultra */
	{0x0960, 0x10000040}, /* MSKO_CON, fifo size 0x40 */
	{0x0964, 0x10400040}, /* MSKO_CON2, set pre-ultra */
	{0x0968, 0x00400040}, /* MSKO_CON3, disable ultra */
	{0x0A80, 0x00000000}, /* OMC_STG_EN_CTRL */
	{0x0E60, 0x80000000}, /* OMC_DMA_DMA_ERR_CTRL */
	{0x0C08, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR0_CTL */
	{0x0C18, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR1_CTL */
	{0x0C28, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR2_CTL */
	{0x0C38, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR3_CTL */
	{0x0C48, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR4_CTL */
	{0x0C58, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR5_CTL */
	{0x0C68, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR6_CTL */
	{0x0C78, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR7_CTL */
	{0x0C88, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR8_CTL */
	{0x0C98, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR9_CTL */
	{0x0CA8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR10_CTL */
	{0x0CB8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR11_CTL */
	{0x0CC8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR12_CTL */
	{0x0CD8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR13_CTL */
	{0x0CE8, CQ_THRX_CTL}, /*DIPCQ_W1A_DIPCQ_CQ_THR14_CTL */
};
#define OMC_INIT_ARRAY_COUNT_2P  ARRAY_SIZE(mtk_imgsys_omc_init_ary_2p)

struct imgsys_reg_range {
	uint32_t str;
	uint32_t end;
};
const struct imgsys_reg_range omc_regs[] = {
	{0x0000, 0x04E4}, /* TOP,VECI,VEC2I,SVECI,SVEC2I */
	{0x0500, 0x053C}, /* OMCO */
	{0x05C0, 0x05FC}, /* OMCO2 */
	{0x0680, 0x06BC}, /* MSKO */
	{0x0A00, 0x0ABC}, /* STG */
	{0x0B40, 0x0BA0}, /* CACHE, ROI */
	{0x0BC0, 0x0BCC}, /* PAK */
	{0x0C00, 0x0C0C}, /* PAK2 */
	{0x0C40, 0x0C64}, /* CQ */
	{0x0E60, 0x0ED8}, /* DMA */
	{0x0F00, 0x0FC4}, /* DEC */
};

#define OMC_REG_ARRAY_COUNT	ARRAY_SIZE(omc_regs)

struct mtk_imgsys_omc_dtable {
	uint32_t empty;
	uint32_t addr;
	uint32_t addr_msb;
};

int imgsys_omc_tfault_callback(int port,
	dma_addr_t mva, void *data)
{
	void __iomem *omcRegBA = 0L;
	unsigned larb = 0;
	unsigned int i =0, j = 0;
	unsigned int omcBase = 0;
	unsigned int engine = 0;
	int ret = 0;
	bool is_qof = false;

	pr_debug("%s: +\n", __func__);

	/* port: [10:5] larb / larb22: omc_tnr; larb23: omc_lite */
	larb = ((port>>5) & 0x3F);

	pr_info("%s: iommu port:0x%x, larb:%d, idx:%d, addr:0x%08lx, +\n", __func__,
		port, larb, (port & 0x1F), (unsigned long)mva);

	/* iomap registers */
	engine = (larb == 22) ? REG_MAP_E_OMC_TNR : REG_MAP_E_OMC_LITE;
	omcRegBA = gOmcRegBA[engine - REG_MAP_E_OMC_TNR];
	if (!omcRegBA) {
		pr_info("%s: OMC_%d, RegBA=0", __func__, port);
		return 1;
	}

	ret = smi_isp_wpe3_lite_get_if_in_use((void *)&is_qof);
	if (ret == -1) {
		pr_info("smi_isp_wpe3_lite_get_if_in_use = -1, stop dump\n");
		return 1;
	}

	pr_info("%s: ==== Dump OMC_%d, TF port: 0x%x =====",
		__func__, (engine - REG_MAP_E_OMC_TNR), port);

	//
	omcBase = gOmcRegBase[(engine - REG_MAP_E_OMC_TNR)];
	for (j = 0; j < (unsigned int) OMC_REG_ARRAY_COUNT; j++) {
		for (i = omc_regs[j].str; i <= omc_regs[j].end; i += 0x10) {
			pr_info("%s: [0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X", __func__,
				(unsigned int)(omcBase + i),
				(unsigned int)ioread32((void *)(omcRegBA + i)),
				(unsigned int)ioread32((void *)(omcRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(omcRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(omcRegBA + i + 0xC)));
		}
	}

	smi_isp_wpe3_lite_put((void *)&is_qof);
	return 1;
}

void imgsys_omc_set_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned int hw_idx = 0, ary_idx = 0;

    if (imgsys_omc_8_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s: +\n", __func__);

	for (hw_idx = REG_MAP_E_OMC_TNR; hw_idx <= REG_MAP_E_OMC_LITE; hw_idx++) {
		/* iomap registers */
		ary_idx = hw_idx - REG_MAP_E_OMC_TNR;
		gOmcRegBA[ary_idx] = of_iomap(imgsys_dev->dev->of_node, hw_idx);
		if (!gOmcRegBA[ary_idx]) {
			dev_info(imgsys_dev->dev,
				"%s: error: unable to iomap omc_%d registers, devnode(%s).\n",
				__func__, hw_idx, imgsys_dev->dev->of_node->name);
			continue;
		}
	}

	dev_info(imgsys_dev->dev, "%s: -\n", __func__);
}

void imgsys_omc_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *ofset = NULL;
	unsigned int i = 0;
	unsigned int hw_idx = 0, ary_idx = 0;

	if (imgsys_omc_8_dbg_enable())
		dev_dbg(imgsys_dev->dev, "%s: +\n", __func__);

	for (hw_idx = REG_MAP_E_OMC_TNR; hw_idx <= REG_MAP_E_OMC_LITE; hw_idx++) {
		/* iomap registers */
		ary_idx = hw_idx - REG_MAP_E_OMC_TNR;
		if (hw_idx < REG_MAP_E_OMC_LITE) {
			for (i = 0 ; i < OMC_INIT_ARRAY_COUNT ; i++) {
				ofset = gOmcRegBA[ary_idx] + mtk_imgsys_omc_init_ary[i].ofset;
				writel(mtk_imgsys_omc_init_ary[i].val, ofset);
			}
		} else {
			for (i = 0 ; i < OMC_INIT_ARRAY_COUNT_2P ; i++) {
				ofset = gOmcRegBA[ary_idx] + mtk_imgsys_omc_init_ary_2p[i].ofset;
				writel(mtk_imgsys_omc_init_ary_2p[i].val, ofset);
			}
		}

	}

	if (imgsys_omc_8_dbg_enable())
		dev_dbg(imgsys_dev->dev, "%s: -\n", __func__);
}

bool imgsys_omc_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine)
{
	void __iomem *omcRegBA = 0L;
	unsigned int hw_idx = 0, ofst_idx;
	unsigned int omcBase = 0;
	bool ret = true; //true: done
	uint32_t value = 0;
 	uint32_t reg_ofst = 0x1c; //OMC_E1A_OMC_TOP_CTL_INT_STATUSX

	if ((engine & IMGSYS_ENG_OMC_TNR))
		hw_idx = (unsigned int) REG_MAP_E_OMC_TNR;
	else
		hw_idx = (unsigned int) REG_MAP_E_OMC_LITE;

	/* iomap registers */
	ofst_idx = hw_idx - (unsigned int) REG_MAP_E_OMC_TNR;
	if (ofst_idx >= OMC_HW_NUM) {
		pr_info("%s: OMC_%d, ofst_idx(%d) >= OMC_HW_NUM(%d)",
			__func__, hw_idx, ofst_idx, OMC_HW_NUM);
		return false;
	}

	if (imgsys_dev->dev_ver == 1)
		omcBase = gOmcRegBase_P[ofst_idx];
	else
		omcBase = gOmcRegBase[ofst_idx];

	omcRegBA = gOmcRegBA[ofst_idx];
	if (!omcRegBA) {
		pr_info("%s: OMC_%d, RegBA = 0", __func__, hw_idx);
		return false;
	}

	value = (uint32_t)ioread32((void *)(omcRegBA + reg_ofst));

	if (!(value & 0x1)) {
		ret = false;
		pr_info(
		"%s: hw_comb:0x%x, polling OMC done fail!!! [0x%08x] 0x%x",
		__func__, engine,
		(unsigned int)(omcBase + reg_ofst), value);
	}

	return ret;
}

void imgsys_omc_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode)
{
	unsigned int i = 0, j = 0;
	u64 u_iova_addr = 0;
	struct mtk_imgsys_req_fd_info *fd_info = NULL;
	struct dma_buf *dbuf = NULL;
	struct mtk_imgsys_request *req = NULL;
	struct mtk_imgsys_dev_buffer *dev_b = 0;
	u64 *u_cq_desc = NULL;
	struct mtk_imgsys_omc_dtable *dtable = NULL;
	unsigned int tun_ofst = 0;
	struct flush_buf_info omc_buf_info;

	for (i = IMGSYS_OMC_TNR; i <= IMGSYS_OMC_LITE; i++) {
		if (!user_info->priv[i].need_update_desc)
			continue;

		if (user_info->priv[i].buf_fd) {
		dbuf = dma_buf_get(user_info->priv[i].buf_fd);
		fd_info = &imgsys_dev->req_fd_cache.info_array[req_fd];
		req = (struct mtk_imgsys_request *) fd_info->req_addr_va;
		dev_b = req->buf_map[imgsys_dev->is_singledev_mode(req)];
			u_iova_addr = imgsys_dev->imgsys_get_iova(dbuf,
					user_info->priv[i].buf_fd,
					imgsys_dev, dev_b) + user_info->priv[i].buf_offset;
			#if SMVR_DECOUPLE
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_omc_mem_virt(imgsys_dev->scp_pdev, mode) +
			#else
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_omc_mem_virt(imgsys_dev->scp_pdev) +
			#endif
						user_info->priv[i].desc_offset + (OMC_UFOD_P2_DESC_OFST
						* (sizeof(struct mtk_imgsys_omc_dtable)))));

			dtable = (struct mtk_imgsys_omc_dtable *)u_cq_desc;
			dtable->addr = u_iova_addr & 0xFFFFFFFF;
			dtable->addr_msb = (u_iova_addr >> 32) & 0xF;
            if (imgsys_omc_8_dbg_enable()) {
			pr_debug(
				"%s: buf_fd(0x%08x) buf_ofst(0x%08x) buf_iova(0x%llx) des_ofst(0x%08x) cq_kva(0x%p) dtable(0x%x/0x%x/0x%x)\n",
				__func__, user_info->priv[i].buf_fd,
				user_info->priv[i].buf_offset,
				u_iova_addr, user_info->priv[i].desc_offset,
				u_cq_desc, dtable->empty,
				dtable->addr, dtable->addr_msb);
		}
		}

		if (tuning_iova) {
			#if SMVR_DECOUPLE
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_omc_mem_virt(imgsys_dev->scp_pdev, mode) +
					user_info->priv[i].desc_offset));
			#else
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_omc_mem_virt(imgsys_dev->scp_pdev) +
						user_info->priv[i].desc_offset));
			#endif
			dtable = (struct mtk_imgsys_omc_dtable *)u_cq_desc;
			for (j = 0; j < OMC_CQ_DESC_NUM; j++) {
				if ((dtable->addr_msb & PSEUDO_DESC_TUNING) == PSEUDO_DESC_TUNING) {
					tun_ofst = dtable->addr;
					dtable->addr = (tun_ofst + tuning_iova) & 0xFFFFFFFF;
					dtable->addr_msb = ((tun_ofst + tuning_iova) >> 32) & 0xF;
                    if (imgsys_omc_8_dbg_enable()) {
					pr_debug(
						"%s: tuning_buf_iova(0x%llx) tun_ofst(0x%08x) des_ofst(0x%08x) cq_kva(0x%p) dtable(0x%x/0x%x/0x%x)\n",
						__func__, tuning_iova, tun_ofst,
						user_info->priv[i].desc_offset,
						u_cq_desc, dtable->empty,
						dtable->addr, dtable->addr_msb);
				}
				}
				dtable++;
			}
		}
		//
		#if SMVR_DECOUPLE
		omc_buf_info.fd = mtk_hcp_get_omc_mem_cq_fd(imgsys_dev->scp_pdev, mode);
		#else
		omc_buf_info.fd = mtk_hcp_get_omc_mem_cq_fd(imgsys_dev->scp_pdev);
		#endif
		omc_buf_info.offset = user_info->priv[i].desc_offset;
		omc_buf_info.len =
			((sizeof(struct mtk_imgsys_omc_dtable) * OMC_CQ_DESC_NUM) + OMC_REG_SIZE);
		omc_buf_info.mode = mode;
		omc_buf_info.is_tuning = false;
        if (imgsys_omc_8_dbg_enable()) {
		pr_debug("imgsys_fw cq omc_buf_info (%d/%d/%d), mode(%d)",
			omc_buf_info.fd, omc_buf_info.len,
			omc_buf_info.offset, omc_buf_info.mode);
        }
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &omc_buf_info);
	}

	for (i = IMGSYS_OMC_TNR; i <= IMGSYS_OMC_LITE; i++) {
		if (!user_info->priv[i].need_flush_tdr)
			continue;

		// tdr buffer
		#if SMVR_DECOUPLE
		omc_buf_info.fd = mtk_hcp_get_omc_mem_tdr_fd(imgsys_dev->scp_pdev, mode);
		#else
		omc_buf_info.fd = mtk_hcp_get_omc_mem_tdr_fd(imgsys_dev->scp_pdev);
		#endif
		omc_buf_info.offset = user_info->priv[i].tdr_offset;
		omc_buf_info.len = OMC_TDR_BUF_MAXSZ;
		omc_buf_info.mode = mode;
		omc_buf_info.is_tuning = false;
        if (imgsys_omc_8_dbg_enable()) {
		pr_debug("imgsys_fw tdr omc_buf_info (%d/%d/%d), mode(%d)",
			omc_buf_info.fd, omc_buf_info.len,
			omc_buf_info.offset, omc_buf_info.mode);
        }
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &omc_buf_info);
	}
}

void imgsys_omc_cmdq_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev,
		void *pkt, int hw_idx)
{
	unsigned int ofset;
	unsigned int i = 0;
	unsigned int ary_idx = 0;
	struct cmdq_pkt *package = NULL;
	unsigned int omcBase = 0;

	if (imgsys_dev == NULL || pkt == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}
	package = (struct cmdq_pkt *)pkt;

	dev_dbg(imgsys_dev->dev, "%s: +\n", __func__);

	ary_idx = hw_idx - REG_MAP_E_OMC_TNR;
	if (imgsys_dev->dev_ver == 1)
		omcBase = gOmcRegBase_P[ary_idx];
	else
		omcBase = gOmcRegBase[ary_idx];

	if (hw_idx < REG_MAP_E_OMC_LITE) {
		for (i = 0 ; i < OMC_INIT_ARRAY_COUNT ; i++) {
			ofset = omcBase + mtk_imgsys_omc_init_ary[i].ofset;
			cmdq_pkt_write(package, NULL, ofset /*address*/,
				mtk_imgsys_omc_init_ary[i].val, 0xffffffff);
		}
	} else {
		for (i = 0 ; i < OMC_INIT_ARRAY_COUNT_2P ; i++) {
			ofset = omcBase + mtk_imgsys_omc_init_ary_2p[i].ofset;
			cmdq_pkt_write(package, NULL, ofset /*address*/,
					mtk_imgsys_omc_init_ary_2p[i].val, 0xffffffff);
		}
	}
	dev_dbg(imgsys_dev->dev, "%s: -\n", __func__);
}

void imgsys_omc_debug_ufo_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *omcRegBA)
{
	unsigned int i;
	unsigned int debug_value[55] = {0x0};
	unsigned int sel_value = 0x0;

	writel((0x1<<16), (omcRegBA + OMC_REG_DBG_SET));
	sel_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	for (i = 0; i < 55; i++) {
		writel((i + 0x0), (omcRegBA + OMC_REG_DEC_CTL1));
		debug_value[i] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));
	}

	pr_info("%s: [0x%x]dbg_sel: 0x%X, [0x%x]dec_ctrl1 [0x%x]ufo_st",
	  __func__, OMC_REG_DBG_SET, sel_value, OMC_REG_DEC_CTL1, OMC_REG_DBG_PORT);

	for (i = 0; i <= 10; i++) {
		pr_info("%s: [l][0x%x] 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X",
		  __func__, (unsigned int)(0x0+i*5),
		  debug_value[i*5+0], debug_value[i*5+1], debug_value[i*5+2],
		  debug_value[i*5+3], debug_value[i*5+4]);
	}

}

void imgsys_omc_debug_dl_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *omcRegBA)
{
	unsigned int dbg_sel_value = 0x0;
	unsigned int debug_value = 0x0;
	unsigned int sel_value = 0x0;

	dbg_sel_value = (0xC << 12);  /* DL */

	/* line & pix cnt */
	writel((dbg_sel_value | (0x2 << 8)), (omcRegBA + OMC_REG_DBG_SET));
	sel_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]current(31:16)LnCnt(15:0)PixCnt:DL[0x%x]0x%x",
	  __func__, OMC_REG_DBG_SET, OMC_REG_DBG_PORT,
	  sel_value, debug_value);

	//line & pix cnt
	writel((dbg_sel_value | (0x1 << 8)), (omcRegBA + OMC_REG_DBG_SET));
	sel_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x](31:16)LnCnt(15:0)PixCnt:DL[0x%x]0x%x",
	  __func__, OMC_REG_DBG_SET, OMC_REG_DBG_PORT,
	  sel_value, debug_value);

	//req/rdy status (output)
	writel((dbg_sel_value | (0x0 << 8)), (omcRegBA + OMC_REG_DBG_SET));
	sel_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]val/REQ/RDY:DL[0x%x]0x%x/%d/%d",
	  __func__, OMC_REG_DBG_SET, OMC_REG_DBG_PORT,
	  sel_value, debug_value,
	   ((debug_value >> 24) & 0x1), ((debug_value >> 23) & 0x1));
}

void imgsys_omc_debug_module_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *omcRegBA, unsigned int ofst)
{
	unsigned int i = 0;
	unsigned int debug_value[16] = {0x0};
	unsigned int sel_value[16] = {0x0};

	/* debug data */
	for (i = 0; i < 16; i++) {
		writel((i<<12), (omcRegBA + OMC_REG_DBG_SET));
		sel_value[i] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
		debug_value[i] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));
	}
	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, debug data[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3],
		sel_value[4], debug_value[4], sel_value[5], debug_value[5],
		sel_value[6], debug_value[6], sel_value[7], debug_value[7],
		sel_value[8], debug_value[8], sel_value[9], debug_value[9],
		sel_value[10], debug_value[10], sel_value[11], debug_value[11],
		sel_value[12], debug_value[12], sel_value[13], debug_value[13],
		sel_value[14], debug_value[14], sel_value[15], debug_value[15]);

	/* cache: [b'0] = 0 ~ 3 */
for (i = 0; i <= 3; i += 2) {
	writel(((0x2<<12) | (0x0<<8) | (i<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | (i<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x0<<8) | ((i+1)<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | ((i+1)<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, cache[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
	}

	/* cache: [b'0] = 0xc ~ 0xf */
for (i = 0xc; i <= 0xf; i += 2) {
	writel(((0x2<<12) | (0x0<<8) | (i<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | (i<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x0<<8) | ((i+1)<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | ((i+1)<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, cache[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
	}

for (i = 0x1d; i <= 0x20; i += 2) {
	writel(((0x2<<12) | (0x0<<8) | (i<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | (i<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x0<<8) | ((i+1)<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | ((i+1)<<0)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, cache[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
	}

	/* top */
	writel(((0x4<<12) | (0x2<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x4<<12) | (0x3<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x4<<12) | (0x4<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, top[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	/* core debug (0~8) */
	for (i = 0; i < 9; i++) {
		writel((0x5<<12) | (i<<8), (omcRegBA + OMC_REG_DEC_CTL1));
		sel_value[i] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
		debug_value[i] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));
	}
	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, core[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3],
		sel_value[4], debug_value[4], sel_value[5], debug_value[5],
		sel_value[6], debug_value[6], sel_value[7], debug_value[7],
		sel_value[8], debug_value[8]);

	//veci_chksum
	writel(0xE100, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE200, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE300, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, veci_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DMA_DBG_SET + ofst), (OMC_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//vec2i_chksum
	writel(0xE101, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE201, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE301, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, vec2i_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DMA_DBG_SET + ofst), (OMC_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//omco_chksum
	writel(0xE105, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE205, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE305, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, omco_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DMA_DBG_SET + ofst), (OMC_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//omco2_chksum
	writel(0xE106, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE206, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE306, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, omco2_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DMA_DBG_SET + ofst), (OMC_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);


	//masko_chksum
	writel(0xE107, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE207, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	writel(0xE307, (omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, masko_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DMA_DBG_SET + ofst), (OMC_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	/* pq2tif, core_pcrop, dl_pcrop, wdma_pcrop */
	writel(((0xa<<12) | (0x0<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xa<<12) | (0x1<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xa<<12) | (0x2<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, pq2tif_crop[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	writel(((0xb<<12) | (0x0<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xb<<12) | (0x1<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xb<<12) | (0x2<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, core_crop[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	writel(((0xc<<12) | (0x0<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xc<<12) | (0x1<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xc<<12) | (0x2<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, dl_crop[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	writel(((0xd<<12) | (0x0<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xd<<12) | (0x1<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0xd<<12) | (0x2<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, wdma_crop[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//pak_c, pak_y
	writel(((0x12<<12) | (0x0<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x12<<12) | (0x1<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x13<<12) | (0x0<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	writel(((0x13<<12) | (0x1<<8)), (omcRegBA + OMC_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, pak_c[0x%x]0x%x, [0x%x]0x%x, pak_y[0x%x]0x%x, [0x%x]0x%x",
		__func__, (OMC_REG_DBG_SET + ofst), (OMC_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
}

void imgsys_omc_debug_cq_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *omcRegBA)
{
	unsigned int dbg_sel_value = 0x0;
	unsigned int debug_value[5] = {0x0};
	unsigned int sel_value[5] = {0x0};

	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_CQ_THR0_CTL));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_CQ_THR1_CTL));
	if (!debug_value[0] || !debug_value[1]) {
		pr_info("%s: No cq_thr enabled! cq0:0x%x, cq1:0x%x",
			__func__, debug_value[0], debug_value[1]);
		return;
	}

	dbg_sel_value = (0x18 << 12);//cq_p2_eng

	//line & pix cnt
	writel((dbg_sel_value | 0x0), (omcRegBA + OMC_REG_DBG_SET));
	sel_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value[0] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	writel((dbg_sel_value | 0x1), (omcRegBA + OMC_REG_DBG_SET));
	sel_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value[1] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	writel((dbg_sel_value | 0x2), (omcRegBA + OMC_REG_DBG_SET));
	sel_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value[2] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	writel((dbg_sel_value | 0x3), (omcRegBA + OMC_REG_DBG_SET));
	sel_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value[3] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	writel((dbg_sel_value | 0x4), (omcRegBA + OMC_REG_DBG_SET));
	sel_value[4] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_SET));
	debug_value[4] = (unsigned int)ioread32((void *)(omcRegBA + OMC_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]cq_st[0x%x]0x%x,dma_dbg[0x%x]0x%x,dma_req[0x%x]0x%x,dma_rdy[0x%x]0x%x,dma_valid[0x%x]0x%x",
		__func__, OMC_REG_DBG_SET, OMC_REG_DBG_PORT,
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3],
		sel_value[4], debug_value[4]);

}


void imgsys_omc_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine)
{
	void __iomem *omcRegBA = 0L;
	unsigned int i, j, ctl_en;
	unsigned int hw_idx = REG_MAP_E_OMC_TNR, ofst_idx;
	unsigned int omcBase = 0;

	pr_info("%s: +\n", __func__);

	if (engine & IMGSYS_ENG_OMC_TNR)
		hw_idx = REG_MAP_E_OMC_TNR;
	else
		hw_idx = REG_MAP_E_OMC_LITE;


	/* iomap registers */
	ofst_idx = hw_idx - REG_MAP_E_OMC_TNR;
	if (ofst_idx >= OMC_HW_NUM) {
		pr_info("%s: OMC_%d, ofst idx(%d) >= OMC_HW_NUM(%d)",
			__func__, hw_idx, ofst_idx, OMC_HW_NUM);
		return;
	}

	if (imgsys_dev->dev_ver == 1)
		omcBase = gOmcRegBase_P[ofst_idx];
	else
		omcBase = gOmcRegBase[ofst_idx];

	omcRegBA = gOmcRegBA[ofst_idx];
	if (!omcRegBA) {
		pr_info("%s: OMC_%d, RegBA = 0", __func__, hw_idx);
		return;
	}
	pr_info("%s: ==== Dump OMC_%d =====",
		__func__, ofst_idx);

	//DL
	ctl_en = (unsigned int)ioread32((void *)(omcRegBA + 0x4));
	if (ctl_en & (DIP_DL|TRAW_DL)) {
		pr_info("%s: OMC Done: %d", __func__,
			!(ioread32((void *)(omcRegBA))) &&
			(ioread32((void *)(omcRegBA + 0x24)) & 0x1));
		pr_info("%s: OMC_DL: PDIP(%d), TRAW(%d)", __func__,
			(ctl_en & DIP_DL) > 0, (ctl_en & TRAW_DL) > 0);
		imgsys_omc_debug_dl_dump(imgsys_dev, omcRegBA);
	}

	imgsys_omc_debug_cq_dump(imgsys_dev, omcRegBA);
	imgsys_omc_debug_module_dump(imgsys_dev, omcRegBA, 0);

	//
	for (j = 0; j < OMC_REG_ARRAY_COUNT; j++) {
		for (i = omc_regs[j].str; i <= omc_regs[j].end; i += 0x10) {
			pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(omcBase + i),
			(unsigned int)ioread32((void *)(omcRegBA + i)),
			(unsigned int)ioread32((void *)(omcRegBA + i + 0x4)),
			(unsigned int)ioread32((void *)(omcRegBA + i + 0x8)),
			(unsigned int)ioread32((void *)(omcRegBA + i + 0xC)));
		}
	}

	//UFO
	if (ctl_en & 0x400) {
		imgsys_omc_debug_ufo_dump(imgsys_dev, omcRegBA);
		imgsys_omc_debug_ufo_dump(imgsys_dev, omcRegBA);
	}

	//
	pr_info("%s: -\n", __func__);
}

void imgsys_omc_uninit(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned int i;

	for (i = 0; i < OMC_HW_NUM; i++) {
		iounmap(gOmcRegBA[i]);
		gOmcRegBA[i] = 0L;
	}

}
MODULE_IMPORT_NS(DMA_BUF);
