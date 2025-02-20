// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Marvin Lin <Marvin.Lin@mediatek.com>
 *
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include "./../mtk_imgsys-engine-isp8.h"
#include "mtk_imgsys-me.h"
#include "iommu_debug.h"
#include "mtk_imgsys-v4l2-debug.h"
#include "mtk-hcp.h"

struct clk_bulk_data imgsys_isp7_me_clks[] = {
	{ .id = "ME_CG_IPE" },
	{ .id = "ME_CG_IPE_TOP" },
	{ .id = "ME_CG" },
	{ .id = "ME_CG_LARB12" },
};

struct mtk_imgsys_me_dtable {
	uint32_t empty;
	uint32_t addr;
	uint32_t addr_msb;
};

void imgsys_me_updatecq(struct mtk_imgsys_dev *imgsys_dev,
			struct img_swfrm_info *user_info, int req_fd, u64 tuning_iova,
			unsigned int mode)
{
	u64 iova_addr = tuning_iova;
	u64 *cq_desc = NULL;
	struct mtk_imgsys_me_dtable *dtable = NULL;
	unsigned int i = 0, tun_ofst = 0;
	struct flush_buf_info me_buf_info;

	/* HWID defined in hw_definition.h */
	if (user_info->priv[IMGSYS_ME].need_update_desc) {
		if (iova_addr) {
			#if SMVR_DECOUPLE
			cq_desc = (u64 *)((void *)(mtk_hcp_get_me_mem_virt(imgsys_dev->scp_pdev, mode) +
					user_info->priv[IMGSYS_ME].desc_offset));
			#else
			cq_desc = (u64 *)((void *)(mtk_hcp_get_me_mem_virt(imgsys_dev->scp_pdev) +
					user_info->priv[IMGSYS_ME].desc_offset));
			#endif
			for (i = 0; i < ME_CQ_DESC_NUM; i++) {
				dtable = (struct mtk_imgsys_me_dtable *)cq_desc + i;
				if ((dtable->addr_msb & PSEUDO_DESC_TUNING) == PSEUDO_DESC_TUNING) {
					tun_ofst = dtable->addr;
					dtable->addr = (tun_ofst + iova_addr) & 0xFFFFFFFF;
					dtable->addr_msb = ((tun_ofst + iova_addr) >> 32) & 0xF;
					if (imgsys_me_7sp_dbg_enable()) {
						pr_debug(
							"%s: tuning_buf_iova(0x%llx) des_ofst(0x%08x) cq_kva(0x%p) dtable(0x%x/0x%x/0x%x)\n",
							__func__, iova_addr,
							user_info->priv[IMGSYS_ME].desc_offset,
							cq_desc, dtable->empty, dtable->addr,
							dtable->addr_msb);
					}
				}
			}
		}
		//
		me_buf_info.fd = mtk_hcp_get_dip_mem_cq_fd(imgsys_dev->scp_pdev, mode);
		me_buf_info.offset = user_info->priv[IMGSYS_ME].desc_offset;
		me_buf_info.len =
			(sizeof(struct mtk_imgsys_me_dtable) * ME_CQ_DESC_NUM) + ME_REG_SIZE;
		me_buf_info.mode = mode;
		me_buf_info.is_tuning = false;
		if (imgsys_me_7sp_dbg_enable()) {
			pr_debug("imgsys_fw cq me_buf_info (%d/%d/%d), mode(%d)",
				me_buf_info.fd, me_buf_info.len,
				me_buf_info.offset, me_buf_info.mode);
		}
		mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &me_buf_info);
	}
}


//static struct ipesys_me_device *me_dev;
static void __iomem *g_meRegBA;
static void __iomem *g_mmgRegBA;

int ME_TranslationFault_callback(int port, dma_addr_t mva, void *data)
{

	void __iomem *meRegBA = 0L;
	unsigned int i;

	/* iomap registers */
	meRegBA = g_meRegBA;
	if (!meRegBA) {
		pr_info("%s Unable to ioremap me registers\n",
		__func__);
	}

	for (i = ME_CTL_OFFSET; i <= ME_CTL_OFFSET + ME_CTL_RANGE_TF; i += 0x10) {
		pr_info("%s: 0x%08X %08X, %08X, %08X, %08X", __func__,
		(unsigned int)(0x34070000 + i),
		(unsigned int)ioread32((void *)(meRegBA + i)),
		(unsigned int)ioread32((void *)(meRegBA + (i+0x4))),
		(unsigned int)ioread32((void *)(meRegBA + (i+0x8))),
		(unsigned int)ioread32((void *)(meRegBA + (i+0xC))));
	}

	return 1;
}

int MMG_TranslationFault_callback(int port, dma_addr_t mva, void *data)
{

	void __iomem *mmgRegBA = 0L;
	unsigned int i;

	/* iomap registers */
	mmgRegBA = g_mmgRegBA;
	if (!mmgRegBA) {
		pr_info("%s Unable to ioremap mmg registers\n",
		__func__);
	}

	for (i = MMG_CTL_OFFSET; i <= MMG_CTL_OFFSET + MMG_CTL_RANGE_TF; i += 0x10) {
		pr_info("%s: 0x%08X %08X, %08X, %08X, %08X", __func__,
		(unsigned int)(0x34080000 + i),
		(unsigned int)ioread32((void *)(mmgRegBA + i)),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0x4))),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0x8))),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0xC))));
	}

	return 1;
}

void imgsys_me_set_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	#ifdef ME_CLK_CTRL
	int ret;
	#endif

	pr_info("%s: +\n", __func__);

	g_meRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_ME);
	g_mmgRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_ME_MMG);

	pr_info("%s: -\n", __func__);
}
//EXPORT_SYMBOL(imgsys_me_set_initial_value);

void imgsys_me_uninit(struct mtk_imgsys_dev *imgsys_dev)
{
	if (g_meRegBA) {
		iounmap(g_meRegBA);
		g_meRegBA = 0L;
	}
	if (g_mmgRegBA) {
		iounmap(g_mmgRegBA);
		g_mmgRegBA = 0L;
	}
}
//EXPORT_SYMBOL(ipesys_me_uninit);

void imgsys_me_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
			unsigned int engine)
{
	void __iomem *meRegBA = 0L;
	void __iomem *mmgRegBA = 0L;
	unsigned int i;

	/* iomap registers */
	meRegBA = g_meRegBA;
	if (!meRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap ME registers\n",
			__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
			__func__, imgsys_dev->dev->of_node->name);
	}
	mmgRegBA = g_mmgRegBA;
	if (!mmgRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap MMG registers\n",
			__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
			__func__, imgsys_dev->dev->of_node->name);
	}


	dev_info(imgsys_dev->dev, "%s: dump me regs\n", __func__);
	for (i = ME_CTL_OFFSET; i <= ME_CTL_OFFSET + ME_CTL_RANGE; i += 0x10) {
		dev_info(imgsys_dev->dev, "%s: 0x%08X %08X, %08X, %08X, %08X", __func__,
		(unsigned int)(0x34070000 + i),
		(unsigned int)ioread32((void *)(meRegBA + i)),
		(unsigned int)ioread32((void *)(meRegBA + (i+0x4))),
		(unsigned int)ioread32((void *)(meRegBA + (i+0x8))),
		(unsigned int)ioread32((void *)(meRegBA + (i+0xC))));
	}
	dev_info(imgsys_dev->dev, "%s: dump mmg regs\n", __func__);
	for (i = MMG_CTL_OFFSET; i <= MMG_CTL_OFFSET + MMG_CTL_RANGE; i += 0x10) {
		dev_info(imgsys_dev->dev, "%s: 0x%08X %08X, %08X, %08X, %08X", __func__,
		(unsigned int)(0x34080000 + i),
		(unsigned int)ioread32((void *)(mmgRegBA + i)),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0x4))),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0x8))),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0xC))));
	}
}
//EXPORT_SYMBOL(ipesys_me_debug_dump);

void ipesys_me_debug_dump_local(void)
{
	void __iomem *meRegBA = 0L;
	void __iomem *mmgRegBA = 0L;
	unsigned int i;

	/* iomap registers */
	meRegBA = g_meRegBA;
	if (!meRegBA) {
		pr_info("imgsys %s Unable to ioremap me registers\n",
			__func__);
	}
	mmgRegBA = g_mmgRegBA;
	if (!mmgRegBA) {
		pr_info("imgsys %s Unable to ioremap mmg registers\n",
			__func__);
	}
	pr_info("imgsys %s: dump me regs\n", __func__);
	for (i = ME_CTL_OFFSET; i <= ME_CTL_OFFSET + ME_CTL_RANGE; i += 0x10) {
		pr_info("imgsys %s: 0x%08X %08X, %08X, %08X, %08X", __func__,
		(unsigned int)(0x34070000 + i),
		(unsigned int)ioread32((void *)(meRegBA + i)),
		(unsigned int)ioread32((void *)(meRegBA + (i+0x4))),
		(unsigned int)ioread32((void *)(meRegBA + (i+0x8))),
		(unsigned int)ioread32((void *)(meRegBA + (i+0xC))));
	}
	pr_info("imgsys %s: dump mmg regs\n", __func__);
	for (i = MMG_CTL_OFFSET; i <= MMG_CTL_OFFSET + MMG_CTL_RANGE; i += 0x10) {
		pr_info("imgsys %s: 0x%08X %08X, %08X, %08X, %08X", __func__,
		(unsigned int)(0x34080000 + i),
		(unsigned int)ioread32((void *)(mmgRegBA + i)),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0x4))),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0x8))),
		(unsigned int)ioread32((void *)(mmgRegBA + (i+0xC))));
	}
}
//EXPORT_SYMBOL(ipesys_me_debug_dump_local);


void imgsys_me_set_hw_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{

	void __iomem *meRegBA = 0L;
	void __iomem *mmgRegBA = 0L;

	/* iomap registers */
	meRegBA = g_meRegBA;
	if (!meRegBA) {
		pr_info("imgsys %s Unable to ioremap me registers\n",
			__func__);
	}
	mmgRegBA = g_mmgRegBA;
	if (!mmgRegBA) {
		pr_info("imgsys %s Unable to ioremap mmg registers\n",
			__func__);
	}
		/* ME HW mode ddren */
		iowrite32(0x00000001, (void *)(meRegBA + 0x0000017c));

		/* MMG HW mode ddren */
		iowrite32(0x00000001, (void *)(mmgRegBA + 0x00000060));

}

bool imgsys_me_done_chk(struct mtk_imgsys_dev *imgsys_dev, uint32_t engine)
{
	bool ret = true; //true: done

	uint32_t value = 0;

	if (engine & IMGSYS_ENG_ME) {
		value = (uint32_t)ioread32((void *)(g_meRegBA + 0x174));
		if (!(value & 0x1)) {
			ret = false;
			pr_info(
			"%s: hw_comb:0x%x, polling ME done fail!!! [0x%08x] 0x%x",
			__func__, engine,
			(uint32_t)(ME_BASE + 0xec), value);
		}
	}

	return ret;
}