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

#include "iommu_debug.h"
#ifdef WPE_TF_DUMP_7SP_1
#include <dt-bindings/memory/mt6897-larb-port.h>
#endif

#define M4U_PORT_DUMMY_EIS  (0)
#define M4U_PORT_DUMMY_TNR  (1)
#define M4U_PORT_DUMMY_LITE (2)

#include "mtk_imgsys-wpe.h"
#include "mtk-hcp.h"
#include "mtk_imgsys-v4l2-debug.h"


#define WPE_A_BASE        (0x15200000)
const unsigned int mtk_imgsys_wpe_base_ofst[] = {0x0, 0x300000, 0x400000};
#define WPE_HW_NUM        ARRAY_SIZE(mtk_imgsys_wpe_base_ofst)

//CTL_MOD_EN
#define PQDIP_DL  0x40000
#define DIP_DL    0x80000
#define TRAW_DL   0x100000

// for CQ_THR0_CTL ~ CQ_THR14CTL
#define CQ_THRX_CTL_EN (1L << 0)
#define CQ_THRX_CTL_MODE (1L << 4)//immediately mode
#define CQ_THRX_CTL	(CQ_THRX_CTL_EN | CQ_THRX_CTL_MODE)

// register ofst
#define WPE_REG_DBG_SET     (0x3C)
#define WPE_REG_DBG_PORT    (0x40)
#define WPE_REG_DMA_DBG_SET     (0xE68)
#define WPE_REG_DMA_DBG_PORT    (0xE70)
#define WPE_REG_CQ_THR0_CTL (0xC08)
#define WPE_REG_CQ_THR1_CTL (0xC18)
#define WPE_REG_DEC_CTL1    (0xFC0)
#define SW_RST   (0x000C)
#define WPE_HW_SET    3

const struct mtk_imgsys_init_array
			mtk_imgsys_wpe_init_ary[] = {
	{0x0014, 0x80000000}, /* WPE_TOP_CTL_INT_EN, en w-clr */
	{0x001C, 0xFFFFFFFF}, /* WPE_TOP_CTL_INT_STATUSX, w-clr */
	{0x00C8, 0x80000000}, /* WPE_TOP_CQ_IRQ_EN, en w-clr */
	{0x00D8, 0xFFFFFFFF}, /* WPE_TOP_CQ_IRQ_STX, w-clr */
	{0x00E0, 0xFFFFFFFF}, /* WPE_TOP_CQ_IRQ_STX2, w-clr */
	{0x00E8, 0xFFFFFFFF}, /* WPE_TOP_CQ_IRQ_STX3, w-clr */
	{0x0118, 0x080A0A82}, /* WPE_TOP_STG_CTL */
	{0x011C, 0x00010010}, /* WPE_TOP_STG_CTL_RANGE_UD */
	{0x0124, 0x000C0002}, /* WPE_TOP_STG_CTL_RANGE_LR */
	{0x0660, 0x10000040}, /* VECI_CON, fifo size 0x40 */
	{0x0664, 0x10400040}, /* VECI_CON2, set pre-ultra */
	{0x0668, 0x00400040}, /* VECI_CON3, disable ultra */
	{0x06E0, 0x10000040}, /* VEC2I_CON, fifo size 0x40 */
	{0x06E4, 0x10400040}, /* VEC2I_CON2, set pre-ultra */
	{0x06E8, 0x00400040}, /* VEC2I_CON3, disable ultra */
	{0x0760, 0x10000040}, /* VEC3I_CON, fifo size 0x40 */
	{0x0764, 0x10400040}, /* VEC3I_CON2, set pre-ultra */
	{0x0768, 0x00400040}, /* VEC3I_CON3, disable ultra */
	{0x0B04, 0x00000002}, /* WPE_CACHE_RWCTL_CTL */
	{0x0B48, 0x00400040}, /* WPE_CACHE_CACHI_CON2_0 */
	{0x0B4C, 0x00400040}, /* WPE_CACHE_CACHI_CON3_0 */
	{0x07E0, 0x10000040}, /* WPEO_CON, fifo size 0x40 */
	{0x07E4, 0x10400040}, /* WPEO_CON2, set pre-ultra */
	{0x07E8, 0x00400040}, /* WPEO_CON3, disable ultra */
	{0x08A0, 0x10000040}, /* WPEO2_CON, fifo size 0x40 */
	{0x08A4, 0x10400040}, /* WPEO2_CON2, set pre-ultra */
	{0x08A8, 0x00400040}, /* WPEO2_CON3, disable ultra */
	{0x0960, 0x10000040}, /* MSKO_CON, fifo size 0x40 */
	{0x0964, 0x10400040}, /* MSKO_CON2, set pre-ultra */
	{0x0968, 0x00400040}, /* MSKO_CON3, disable ultra */
	{0x0A80, 0x00000000}, /* WPE_STG_EN_CTRL */
	{0x0E60, 0x80000000}, /* WPE_DMA_DMA_ERR_CTRL */
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
#define WPE_INIT_ARRAY_COUNT  ARRAY_SIZE(mtk_imgsys_wpe_init_ary)


const struct mtk_imgsys_init_array
			mtk_imgsys_wpe_init_ary_2p[] = {
	{0x0014, 0x80000000}, /* WPE_TOP_CTL_INT_EN, en w-clr */
	{0x001C, 0xFFFFFFFF}, /* WPE_TOP_CTL_INT_STATUSX, w-clr */
	{0x00C8, 0x80000000}, /* WPE_TOP_CQ_IRQ_EN, en w-clr */
	{0x00D8, 0xFFFFFFFF}, /* WPE_TOP_CQ_IRQ_STX, w-clr */
	{0x00E0, 0xFFFFFFFF}, /* WPE_TOP_CQ_IRQ_STX2, w-clr */
	{0x00E8, 0xFFFFFFFF}, /* WPE_TOP_CQ_IRQ_STX3, w-clr */
	{0x0118, 0x080A0A82}, /* WPE_TOP_STG_CTL */
	{0x011C, 0x00010010}, /* WPE_TOP_STG_CTL_RANGE_UD */
	{0x0124, 0x000C0002}, /* WPE_TOP_STG_CTL_RANGE_LR */
	{0x0660, 0x10000040}, /* VECI_CON, fifo size 0x40 */
	{0x0664, 0x10400040}, /* VECI_CON2, set pre-ultra */
	{0x0668, 0x00400040}, /* VECI_CON3, disable ultra */
	{0x06E0, 0x10000040}, /* VEC2I_CON, fifo size 0x40 */
	{0x06E4, 0x10400040}, /* VEC2I_CON2, set pre-ultra */
	{0x06E8, 0x00400040}, /* VEC2I_CON3, disable ultra */
	{0x0760, 0x10000040}, /* VEC3I_CON, fifo size 0x40 */
	{0x0764, 0x10400040}, /* VEC3I_CON2, set pre-ultra */
	{0x0768, 0x00400040}, /* VEC3I_CON3, disable ultra */
	{0x0B04, 0x00000002}, /* WPE_CACHE_RWCTL_CTL */
	{0x0B48, 0x00400040}, /* WPE_CACHE_CACHI_CON2_0 */
	{0x0B4C, 0x00400040}, /* WPE_CACHE_CACHI_CON3_0 */
	{0x07E0, 0x10000040}, /* WPEO_CON, fifo size 0x40 */
	{0x07E4, 0x10400040}, /* WPEO_CON2, set pre-ultra */
	{0x07E8, 0x00400040}, /* WPEO_CON3, disable ultra */
	{0x08A0, 0x10000040}, /* WPEO2_CON, fifo size 0x40 */
	{0x08A4, 0x10400040}, /* WPEO2_CON2, set pre-ultra */
	{0x08A8, 0x00400040}, /* WPEO2_CON3, disable ultra */
	{0x0960, 0x10000040}, /* MSKO_CON, fifo size 0x40 */
	{0x0964, 0x10400040}, /* MSKO_CON2, set pre-ultra */
	{0x0968, 0x00400040}, /* MSKO_CON3, disable ultra */
	{0x0A80, 0x00000000}, /* WPE_STG_EN_CTRL */
	{0x0E60, 0x80000000}, /* WPE_DMA_DMA_ERR_CTRL */
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
#define WPE_INIT_ARRAY_COUNT_2P  ARRAY_SIZE(mtk_imgsys_wpe_init_ary_2p)

struct imgsys_reg_range {
	uint32_t str;
	uint32_t end;
};
const struct imgsys_reg_range wpe_regs[] = {
	{0x0000, 0x07A4}, /* TOP,VECI,VEC2I,SVECI,SVEC2I */
	{0x07C0, 0x085C}, /* WPEO */
	{0x0880, 0x091C}, /* WPEO2 */
	{0x0940, 0x09DC}, /* MSKO */
	{0x0A00, 0x0A5C}, /* VGEN */
	{0x0B00, 0x0B5C}, /* CACHE */
	{0x0C00, 0x0C24}, /* CQ */
	{0x0E60, 0x0E84}, /* DMA */
	{0x0F00, 0x0FC4}, /* DEC */

};
#define WPE_REG_ARRAY_COUNT	ARRAY_SIZE(wpe_regs)

struct mtk_imgsys_wpe_dtable {
	uint32_t empty;
	uint32_t addr;
	uint32_t addr_msb;
};

void __iomem *gWpeRegBA[WPE_HW_NUM] = {0L};
unsigned int gWpeRegBase[WPE_HW_NUM] = {0x15200000, 0x15500000, 0x15600000};
unsigned int gWpeRegBaseAddr[3] = { 0x15220000, 0x15520000, 0x15620000 };

int imgsys_wpe_tfault_callback(int port,
	dma_addr_t mva, void *data)
{
	void __iomem *wpeRegBA = 0L;
	unsigned larb = 0;
	unsigned int i =0, j = 0;
	unsigned int wpeBase = 0;
	unsigned int engine = 0;

	pr_debug("%s: +\n", __func__);

	/* port: [10:5] larb / larb11: wpe_eis; larb22: wpe_tnr; larb23: wpe_lite */
	larb = ((port>>5) & 0x3F);

	pr_info("%s: iommu port:0x%x, larb:%d, idx:%d, addr:0x%08lx, +\n", __func__,
		port, larb, (port & 0x1F), (unsigned long)mva);

	/* iomap registers */
	engine = (larb == 11) ? REG_MAP_E_WPE_EIS : ((larb == 22) ? REG_MAP_E_WPE_TNR : REG_MAP_E_WPE_LITE);
	wpeRegBA = gWpeRegBA[engine - REG_MAP_E_WPE_EIS];
	if (!wpeRegBA) {
		pr_info("%s: WPE_%d, RegBA=0", __func__, port);
		return 1;
	}

	pr_info("%s: ==== Dump WPE_%d, TF port: 0x%x =====",
		__func__, (engine - REG_MAP_E_WPE_EIS), port);

	//
	wpeBase = WPE_A_BASE + mtk_imgsys_wpe_base_ofst[(engine - REG_MAP_E_WPE_EIS)];
	for (j = 0; j < WPE_REG_ARRAY_COUNT; j++) {
		for (i = wpe_regs[j].str; i <= wpe_regs[j].end; i += 0x10) {
			pr_info("%s: [0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X", __func__,
				(unsigned int)(wpeBase + i),
				(unsigned int)ioread32((void *)(wpeRegBA + i)),
				(unsigned int)ioread32((void *)(wpeRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(wpeRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(wpeRegBA + i + 0xC)));
		}
	}

	return 1;
}

void imgsys_wpe_set_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned int hw_idx = 0, ary_idx = 0;

    if (imgsys_wpe_7sp_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s: +\n", __func__);

	for (hw_idx = REG_MAP_E_WPE_EIS; hw_idx <= REG_MAP_E_WPE_LITE; hw_idx++) {
		/* iomap registers */
		ary_idx = hw_idx - REG_MAP_E_WPE_EIS;
		gWpeRegBA[ary_idx] = of_iomap(imgsys_dev->dev->of_node, hw_idx);
		if (!gWpeRegBA[ary_idx]) {
			dev_info(imgsys_dev->dev,
				"%s: error: unable to iomap wpe_%d registers, devnode(%s).\n",
				__func__, hw_idx, imgsys_dev->dev->of_node->name);
			continue;
		}
	}

	dev_info(imgsys_dev->dev, "%s: -\n", __func__);
}

void imgsys_wpe_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *ofset = NULL;
	unsigned int i = 0;
	unsigned int hw_idx = 0, ary_idx = 0;

	if (imgsys_wpe_7sp_dbg_enable())
		dev_dbg(imgsys_dev->dev, "%s: +\n", __func__);

	for (hw_idx = REG_MAP_E_WPE_EIS; hw_idx <= REG_MAP_E_WPE_LITE; hw_idx++) {
		/* iomap registers */
		ary_idx = hw_idx - REG_MAP_E_WPE_EIS;
		if (hw_idx < REG_MAP_E_WPE_LITE) {
			for (i = 0 ; i < WPE_INIT_ARRAY_COUNT ; i++) {
				ofset = gWpeRegBA[ary_idx] + mtk_imgsys_wpe_init_ary[i].ofset;
				writel(mtk_imgsys_wpe_init_ary[i].val, ofset);
			}
		} else {
			for (i = 0 ; i < WPE_INIT_ARRAY_COUNT_2P ; i++) {
				ofset = gWpeRegBA[ary_idx] + mtk_imgsys_wpe_init_ary_2p[i].ofset;
				writel(mtk_imgsys_wpe_init_ary_2p[i].val, ofset);
			}
		}

	}

	if (imgsys_wpe_7sp_dbg_enable())
		dev_dbg(imgsys_dev->dev, "%s: -\n", __func__);
}

bool imgsys_wpe_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine)
{
	void __iomem *wpeRegBA = 0L;
	unsigned int hw_idx = 0, ofst_idx;
	unsigned int wpeBase = 0;
	unsigned int startHw = REG_MAP_E_WPE_EIS, endHW = REG_MAP_E_WPE_TNR;
	bool ret = true; //true: done
	uint32_t value = 0;
 	uint32_t reg_ofst = 0x1c; //WPE_E1A_WPE_TOP_CTL_INT_STATUSX

	if ((engine & IMGSYS_ENG_WPE_EIS) && !(engine & IMGSYS_ENG_WPE_TNR))
		endHW = REG_MAP_E_WPE_EIS;

	if (!(engine & IMGSYS_ENG_WPE_EIS) && (engine & IMGSYS_ENG_WPE_TNR))
		startHw = REG_MAP_E_WPE_TNR;

	if ((engine & IMGSYS_ENG_WPE_LITE))
		startHw = endHW = REG_MAP_E_WPE_LITE;

	/* iomap registers */
	for (hw_idx = startHw; hw_idx <= endHW; hw_idx++) {
		ofst_idx = hw_idx - REG_MAP_E_WPE_EIS;
		if (ofst_idx >= WPE_HW_NUM)
			continue;

		wpeBase = WPE_A_BASE + mtk_imgsys_wpe_base_ofst[ofst_idx];
		wpeRegBA = gWpeRegBA[ofst_idx];
		if (!wpeRegBA) {
			pr_info("%s: WPE_%d, RegBA = 0", __func__, hw_idx);
			continue;
		}

		value = (uint32_t)ioread32((void *)(wpeRegBA + reg_ofst));

		if (!(value & 0x1)) {
			ret = false;
			pr_info(
			"%s: hw_comb:0x%x, polling WPE done fail!!! [0x%08x] 0x%x",
			__func__, engine,
			(unsigned int)(wpeBase + reg_ofst), value);
		}
	}

	return ret;
}

void imgsys_wpe_updatecq(struct mtk_imgsys_dev *imgsys_dev,
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
	struct mtk_imgsys_wpe_dtable *dtable = NULL;
	unsigned int tun_ofst = 0;
	struct flush_buf_info wpe_buf_info;

	for (i = IMGSYS_WPE_EIS; i <= IMGSYS_WPE_LITE; i++) {
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
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_wpe_mem_virt(imgsys_dev->scp_pdev, mode) +
			#else
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_wpe_mem_virt(imgsys_dev->scp_pdev) +
			#endif
						user_info->priv[i].desc_offset + (WPE_UFOD_P2_DESC_OFST
						* (sizeof(struct mtk_imgsys_wpe_dtable)))));

			dtable = (struct mtk_imgsys_wpe_dtable *)u_cq_desc;
			dtable->addr = u_iova_addr & 0xFFFFFFFF;
			dtable->addr_msb = (u_iova_addr >> 32) & 0xF;
            if (imgsys_wpe_7sp_dbg_enable()) {
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
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_wpe_mem_virt(imgsys_dev->scp_pdev, mode) +
					user_info->priv[i].desc_offset));
			#else
			u_cq_desc = (u64 *)((void *)(mtk_hcp_get_wpe_mem_virt(imgsys_dev->scp_pdev) +
						user_info->priv[i].desc_offset));
			#endif
			dtable = (struct mtk_imgsys_wpe_dtable *)u_cq_desc;
			for (j = 0; j < WPE_CQ_DESC_NUM; j++) {
				if ((dtable->addr_msb & PSEUDO_DESC_TUNING) == PSEUDO_DESC_TUNING) {
					tun_ofst = dtable->addr;
					dtable->addr = (tun_ofst + tuning_iova) & 0xFFFFFFFF;
					dtable->addr_msb = ((tun_ofst + tuning_iova) >> 32) & 0xF;
                    if (imgsys_wpe_7sp_dbg_enable()) {
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
		wpe_buf_info.fd = mtk_hcp_get_wpe_mem_cq_fd(imgsys_dev->scp_pdev, mode);
		#else
		wpe_buf_info.fd = mtk_hcp_get_wpe_mem_cq_fd(imgsys_dev->scp_pdev);
		#endif
		wpe_buf_info.offset = user_info->priv[i].desc_offset;
		wpe_buf_info.len =
			((sizeof(struct mtk_imgsys_wpe_dtable) * WPE_CQ_DESC_NUM) + WPE_REG_SIZE);
		wpe_buf_info.mode = mode;
		wpe_buf_info.is_tuning = false;
        if (imgsys_wpe_7sp_dbg_enable()) {
		pr_debug("imgsys_fw cq wpe_buf_info (%d/%d/%d), mode(%d)",
			wpe_buf_info.fd, wpe_buf_info.len,
			wpe_buf_info.offset, wpe_buf_info.mode);
        }
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &wpe_buf_info);
	}

	for (i = IMGSYS_WPE_EIS; i <= IMGSYS_WPE_LITE; i++) {
		if (!user_info->priv[i].need_flush_tdr)
			continue;

		// tdr buffer
		#if SMVR_DECOUPLE
		wpe_buf_info.fd = mtk_hcp_get_wpe_mem_tdr_fd(imgsys_dev->scp_pdev, mode);
		#else
		wpe_buf_info.fd = mtk_hcp_get_wpe_mem_tdr_fd(imgsys_dev->scp_pdev);
		#endif
		wpe_buf_info.offset = user_info->priv[i].tdr_offset;
		wpe_buf_info.len = WPE_TDR_BUF_MAXSZ;
		wpe_buf_info.mode = mode;
		wpe_buf_info.is_tuning = false;
        if (imgsys_wpe_7sp_dbg_enable()) {
		pr_debug("imgsys_fw tdr wpe_buf_info (%d/%d/%d), mode(%d)",
			wpe_buf_info.fd, wpe_buf_info.len,
			wpe_buf_info.offset, wpe_buf_info.mode);
        }
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &wpe_buf_info);
	}
}

void imgsys_wpe_cmdq_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev,
		void *pkt, int hw_idx)
{
	unsigned int ofset;
	unsigned int i = 0;
	unsigned int idx = 0, ary_idx = 0;
	struct cmdq_pkt *package = NULL;
	unsigned int HwIdx = 0;
	unsigned int WpeRegBA = 0L;
	unsigned int pWpeCtrl = 0L;

	if (imgsys_dev == NULL || pkt == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}
	package = (struct cmdq_pkt *)pkt;

	dev_dbg(imgsys_dev->dev, "%s: +\n", __func__);

	for (HwIdx = 0; HwIdx < WPE_HW_SET; HwIdx++) {
		if (HwIdx == 0)
			WpeRegBA = gWpeRegBaseAddr[0];
		else if (HwIdx == 1)
			WpeRegBA = gWpeRegBaseAddr[1];
		else
			WpeRegBA = gWpeRegBaseAddr[2];

		/* Wpe Macro HW Reset */
		pWpeCtrl = WpeRegBA + SW_RST;
		cmdq_pkt_write(package, NULL, pWpeCtrl /*address*/,
			       0xF, 0xffffffff);
		/* Clear HW Reset */
		cmdq_pkt_write(package, NULL, pWpeCtrl /*address*/,
			       0x0, 0xffffffff);
	}

	for (idx = REG_MAP_E_WPE_EIS; idx <= REG_MAP_E_WPE_LITE; idx++) {
		/* iomap registers */
		ary_idx = idx - REG_MAP_E_WPE_EIS;
		if (idx < REG_MAP_E_WPE_LITE) {
			for (i = 0 ; i < WPE_INIT_ARRAY_COUNT ; i++) {
				ofset = gWpeRegBase[ary_idx] + mtk_imgsys_wpe_init_ary[i].ofset;
				cmdq_pkt_write(package, NULL, ofset /*address*/,
						mtk_imgsys_wpe_init_ary[i].val, 0xffffffff);
			}
		} else {
			for (i = 0 ; i < WPE_INIT_ARRAY_COUNT_2P ; i++) {
				ofset = gWpeRegBase[ary_idx] + mtk_imgsys_wpe_init_ary_2p[i].ofset;
				cmdq_pkt_write(package, NULL, ofset /*address*/,
						mtk_imgsys_wpe_init_ary_2p[i].val, 0xffffffff);
			}
		}

	}

	dev_dbg(imgsys_dev->dev, "%s: -\n", __func__);
}

void imgsys_wpe_debug_ufo_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *wpeRegBA)
{
	unsigned int i;
	unsigned int debug_value[55] = {0x0};
	unsigned int sel_value = 0x0;

	writel((0xB<<12), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	for (i = 0; i < 55; i++) {
		writel((i + 0xC00), (wpeRegBA + WPE_REG_DEC_CTL1));
		debug_value[i] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));
	}

	pr_info("%s: [0x%x]dbg_sel: 0x%X, [0x%x]dec_ctrl1 [0x%x]ufo_st",
	  __func__, WPE_REG_DBG_SET, sel_value, WPE_REG_DEC_CTL1, WPE_REG_DBG_PORT);

	for (i = 0; i <= 10; i++) {
		pr_info("%s: [l][0x%x] 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X",
		  __func__, (unsigned int)(0xC00+i*5),
		  debug_value[i*5+0], debug_value[i*5+1], debug_value[i*5+2],
		  debug_value[i*5+3], debug_value[i*5+4]);
	}

}

void imgsys_wpe_debug_dl_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *wpeRegBA)
{
	unsigned int dbg_sel_value[3] = {0x0, 0x0, 0x0};
	unsigned int debug_value[3] = {0x0, 0x0, 0x0};
	unsigned int sel_value[3] = {0x0, 0x0, 0x0};

	dbg_sel_value[0] = (0xC << 12); //pqdip
	dbg_sel_value[1] = (0xD << 12); //DIP
	dbg_sel_value[2] = (0xE << 12); //TRAW

	//line & pix cnt
	writel((dbg_sel_value[0] | (0x1 << 8)), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value[1] | (0x1 << 8)), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value[2] | (0x1 << 8)), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x](31:16)LnCnt(15:0)PixCnt:PQDIP[0x%x]0x%x,DIP[0x%x]0x%x,TRAW[0x%x]0x%x",
	  __func__, WPE_REG_DBG_SET, WPE_REG_DBG_PORT,
	  sel_value[0], debug_value[0], sel_value[1], debug_value[1],
	  sel_value[2], debug_value[2]);

	//req/rdy status (output)
	writel((dbg_sel_value[0] | (0x0 << 8)), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value[1] | (0x0 << 8)), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value[2] | (0x0 << 8)), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]val/REQ/RDY:PQDIP[0x%x]0x%x/%d/%d,DIP[0x%x]0x%x/%d/%d,TRAW[0x%x]0x%x/%d/%d",
	  __func__, WPE_REG_DBG_SET, WPE_REG_DBG_PORT,
	  sel_value[0], debug_value[0],
	   ((debug_value[0] >> 24) & 0x1), ((debug_value[0] >> 23) & 0x1),
	  sel_value[1], debug_value[1],
	   ((debug_value[1] >> 24) & 0x1), ((debug_value[1] >> 23) & 0x1),
	  sel_value[2], debug_value[2],
	   ((debug_value[2] >> 24) & 0x1), ((debug_value[2] >> 23) & 0x1));
}

void imgsys_wpe_debug_module_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *wpeRegBA, unsigned int ofst)
{
	unsigned int i = 0;
	unsigned int debug_value[4] = {0x0};
	unsigned int sel_value[4] = {0x0};

	//cache
for (i = 0; i <= 3; i += 2) {
	writel(((0x2<<12) | (0x0<<8) | (i<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | (i<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x0<<8) | ((i+1)<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | ((i+1)<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, cache[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
	}

for (i = 0xc; i <= 0xf; i += 2) {
	writel(((0x2<<12) | (0x0<<8) | (i<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | (i<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x0<<8) | ((i+1)<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | ((i+1)<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, cache[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
	}

for (i = 0x1d; i <= 0x20; i += 2) {
	writel(((0x2<<12) | (0x0<<8) | (i<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | (i<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x0<<8) | ((i+1)<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x2<<12) | (0x1<<8) | ((i+1)<<0)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, cache[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
	}

	//psp, top
	writel(((0x4<<12) | (0x2<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x4<<12) | (0x3<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x4<<12) | (0x4<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel((0x5<<12), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, psp[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x, top[0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);

	//traw_chksum
	writel(((0xe<<12) | (0x0<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0xe<<12) | (0x1<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0xe<<12) | (0x2<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, traw_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//veci_chksum
	writel(0x100, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x200, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x300, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, veci_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DMA_DBG_SET + ofst), (WPE_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//vec2i_chksum
	writel(0x101, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x201, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x301, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, vec2i_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DMA_DBG_SET + ofst), (WPE_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//wpeo_chksum
	writel(0x103, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x203, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x303, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, wpeo_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DMA_DBG_SET + ofst), (WPE_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//wpeo2_chksum
	writel(0x104, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x204, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x304, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, wpeo2_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DMA_DBG_SET + ofst), (WPE_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);


	//masko_chksum
	writel(0x105, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x205, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	writel(0x305, (wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DMA_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, masko_chksum[0x%x]0x%x, [0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DMA_DBG_SET + ofst), (WPE_REG_DMA_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);


	//wpeo_crop, wpeo2_crop, traw_crop
	writel(((0x15<<12) | (0x1<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x16<<12) | (0x1<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x17<<12) | (0x1<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, wpeo_crop[0x%x]0x%x, wpeo2_crop[0x%x]0x%x, traw_crop[0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2]);

	//pak_c, pak_y
	writel(((0x1a<<12) | (0x1<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x1a<<12) | (0x2<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x1b<<12) | (0x1<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	writel(((0x1b<<12) | (0x2<<8)), (wpeRegBA + WPE_REG_DBG_SET + ofst));
	sel_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET + ofst));
	debug_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT + ofst));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]dbg_port, pak_c[0x%x]0x%x, [0x%x]0x%x, pak_y[0x%x]0x%x, [0x%x]0x%x",
		__func__, (WPE_REG_DBG_SET + ofst), (WPE_REG_DBG_PORT + ofst),
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3]);
}

void imgsys_wpe_debug_cq_dump(struct mtk_imgsys_dev *imgsys_dev,
							void __iomem *wpeRegBA)
{
	unsigned int dbg_sel_value = 0x0;
	unsigned int debug_value[5] = {0x0};
	unsigned int sel_value[5] = {0x0};

	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_CQ_THR0_CTL));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_CQ_THR1_CTL));
	if (!debug_value[0] || !debug_value[1]) {
		pr_info("%s: No cq_thr enabled! cq0:0x%x, cq1:0x%x",
			__func__, debug_value[0], debug_value[1]);
		return;
	}

	dbg_sel_value = (0x18 << 12);//cq_p2_eng

	//line & pix cnt
	writel((dbg_sel_value | 0x0), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[0] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value | 0x1), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[1] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value | 0x2), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[2] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value | 0x3), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[3] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	writel((dbg_sel_value | 0x4), (wpeRegBA + WPE_REG_DBG_SET));
	sel_value[4] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_SET));
	debug_value[4] = (unsigned int)ioread32((void *)(wpeRegBA + WPE_REG_DBG_PORT));

	pr_info("%s:[0x%x]dbg_sel,[0x%x]cq_st[0x%x]0x%x,dma_dbg[0x%x]0x%x,dma_req[0x%x]0x%x,dma_rdy[0x%x]0x%x,dma_valid[0x%x]0x%x",
		__func__, WPE_REG_DBG_SET, WPE_REG_DBG_PORT,
		sel_value[0], debug_value[0], sel_value[1], debug_value[1],
		sel_value[2], debug_value[2], sel_value[3], debug_value[3],
		sel_value[4], debug_value[4]);

}


void imgsys_wpe_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine)
{
	void __iomem *wpeRegBA = 0L;
	unsigned int i, j, ctl_en;
	unsigned int hw_idx = 0, ofst_idx;
	unsigned int wpeBase = 0;
	unsigned int startHw = REG_MAP_E_WPE_EIS, endHW = REG_MAP_E_WPE_TNR;

	pr_info("%s: +\n", __func__);

	if ((engine & IMGSYS_ENG_WPE_EIS) && !(engine & IMGSYS_ENG_WPE_TNR))
		endHW = REG_MAP_E_WPE_EIS;

	if (!(engine & IMGSYS_ENG_WPE_EIS) && (engine & IMGSYS_ENG_WPE_TNR))
		startHw = REG_MAP_E_WPE_TNR;

	if ((engine & IMGSYS_ENG_WPE_LITE))
		startHw = endHW = REG_MAP_E_WPE_LITE;


	/* iomap registers */
	for (hw_idx = startHw; hw_idx <= endHW; hw_idx++) {
		ofst_idx = hw_idx - REG_MAP_E_WPE_EIS;
		if (ofst_idx >= WPE_HW_NUM)
			continue;

		wpeBase = WPE_A_BASE + mtk_imgsys_wpe_base_ofst[ofst_idx];
		wpeRegBA = gWpeRegBA[ofst_idx];
		if (!wpeRegBA) {
			pr_info("%s: WPE_%d, RegBA = 0", __func__, hw_idx);
			continue;
		}
		pr_info("%s: ==== Dump WPE_%d =====",
		  __func__, ofst_idx);

		//DL
		ctl_en = (unsigned int)ioread32((void *)(wpeRegBA + 0x4));
		if (ctl_en & (PQDIP_DL|DIP_DL|TRAW_DL)) {
			pr_info("%s: WPE Done: %d", __func__,
			  !(ioread32((void *)(wpeRegBA))) &&
			  (ioread32((void *)(wpeRegBA + 0x24)) & 0x1));
			pr_info("%s: WPE_DL: PQDIP(%d), DIP(%d), TRAW(%d)", __func__,
			  (ctl_en & PQDIP_DL) > 0, (ctl_en & DIP_DL) > 0, (ctl_en & TRAW_DL) > 0);
			imgsys_wpe_debug_dl_dump(imgsys_dev, wpeRegBA);
		}

		imgsys_wpe_debug_cq_dump(imgsys_dev, wpeRegBA);
		imgsys_wpe_debug_module_dump(imgsys_dev, wpeRegBA, 0);

		//
		for (j = 0; j < WPE_REG_ARRAY_COUNT; j++) {
			for (i = wpe_regs[j].str; i <= wpe_regs[j].end; i += 0x10) {
				pr_info("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
				(unsigned int)(wpeBase + i),
				(unsigned int)ioread32((void *)(wpeRegBA + i)),
				(unsigned int)ioread32((void *)(wpeRegBA + i + 0x4)),
				(unsigned int)ioread32((void *)(wpeRegBA + i + 0x8)),
				(unsigned int)ioread32((void *)(wpeRegBA + i + 0xC)));
			}
		}

		//UFO
		if (ctl_en & 0x400) {
			imgsys_wpe_debug_ufo_dump(imgsys_dev, wpeRegBA);
			imgsys_wpe_debug_ufo_dump(imgsys_dev, wpeRegBA);
		}

	}
	//
	pr_info("%s: -\n", __func__);
}

void imgsys_wpe_uninit(struct mtk_imgsys_dev *imgsys_dev)
{
	unsigned int i;

	for (i = 0; i < WPE_HW_NUM; i++) {
		iounmap(gWpeRegBA[i]);
		gWpeRegBA[i] = 0L;
	}

}
MODULE_IMPORT_NS(DMA_BUF);
